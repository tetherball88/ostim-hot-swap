#include <Windows.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include "PCH.h"
#include "src/ActorManagement.h"
#include "src/Papyrus/PapyrusActorManagement.h"
#include "src/Settings.h"
#include "src/Utils/MessageBox.h"

using namespace SKSE;

OstimNG_API::Thread::IThreadInterface* g_ostimAPI = nullptr;

namespace {
    void SetupLogging() {
        auto logDir = SKSE::log::log_directory();
        if (!logDir) {
            if (auto* console = RE::ConsoleLog::GetSingleton()) {
                console->Print("OStimHotSwap: log directory unavailable");
            }
            return;
        }

        std::filesystem::path logPath = *logDir;
        if (!std::filesystem::is_directory(logPath)) {
            logPath = logPath.parent_path();
        }
        logPath /= "OStimHotSwap.log";

        std::error_code ec;
        std::filesystem::create_directories(logPath.parent_path(), ec);
        if (ec) {
            if (auto* console = RE::ConsoleLog::GetSingleton()) {
                console->Print("OStimHotSwap: failed to create log folder (%s)", ec.message().c_str());
            }
            return;
        }

        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logPath.string(), true);
        auto logger = std::make_shared<spdlog::logger>("OStimHotSwap", std::move(sink));
        logger->set_level(spdlog::level::trace);
        logger->flush_on(spdlog::level::trace);
        logger->set_pattern("[%H:%M:%S] [%l] %v");

        spdlog::set_default_logger(std::move(logger));
        spdlog::info("Logging to {}", logPath.string());
    }

    void PrintToConsole(std::string_view message) {
        SKSE::log::info("{}", message);
        if (auto* console = RE::ConsoleLog::GetSingleton()) {
            console->Print("%s", message.data());
        }
    }

    class SceneJoinInputSink : public RE::BSTEventSink<RE::InputEvent*> {
    public:
        static SceneJoinInputSink* GetSingleton() {
            static SceneJoinInputSink s;
            return &s;
        }

        RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_events,
                                              RE::BSTEventSource<RE::InputEvent*>*) override {
            if (!g_ostimAPI || !a_events) return RE::BSEventNotifyControl::kContinue;

            for (auto* event = *a_events; event; event = event->next) {
                auto* btn = event->AsButtonEvent();
                if (!btn || !btn->IsDown()) continue;

                OstimNG_API::Thread::KeyData keys{};
                g_ostimAPI->GetKeyData(&keys);
                if (keys.keySceneStart < 0 || btn->GetIDCode() != static_cast<uint32_t>(keys.keySceneStart))
                    continue;

                SKSE::log::trace("SceneJoin: keySceneStart pressed");
                auto* player = RE::PlayerCharacter::GetSingleton();
                if (!player) continue;

                // Player must not already be in a scene
                if (g_ostimAPI->IsActorInAnyThread(player->GetFormID())) {
                    SKSE::log::debug("SceneJoin: player already in a scene, ignoring");
                    continue;
                }

                // Get crosshair NPC
                auto* crosshairData = RE::CrosshairPickData::GetSingleton();
                if (!crosshairData) {
                    SKSE::log::debug("SceneJoin: no crosshair data");
                    continue;
                }
                auto targetRef = crosshairData->target[0].get();
                if (!targetRef) {
                    SKSE::log::debug("SceneJoin: no crosshair target");
                    continue;
                }
                auto* targetActor = targetRef->As<RE::Actor>();
                if (!targetActor) {
                    SKSE::log::debug("SceneJoin: crosshair target {:X} is not an actor", targetRef->GetFormID());
                    continue;
                }

                // NPC must be in a scene
                if (!g_ostimAPI->IsActorInAnyThread(targetActor->GetFormID())) {
                    SKSE::log::debug("SceneJoin: crosshair actor {:X} is not in a scene", targetActor->GetFormID());
                    continue;
                }

                SKSE::log::debug("SceneJoin: crosshair actor {:X} is in a scene, querying thread ID", targetActor->GetFormID());

                // Get thread ID via OActor.GetThreadID papyrus native
                const auto skyrimVM = RE::SkyrimVM::GetSingleton();
                auto vm = skyrimVM ? skyrimVM->impl : nullptr;
                if (!vm) continue;

                RE::Actor* capturedActor = targetActor;
                RE::Actor* capturedPlayer = player;
                auto args = RE::MakeFunctionArguments(static_cast<RE::Actor*>(capturedActor));
                RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> cb(
                    new GetThreadIDCallback(capturedPlayer));
                vm->DispatchStaticCall("OActor", "GetThreadID", args, cb);
                SKSE::log::debug("SceneJoin: dispatched OActor.GetThreadID for actor {:X}", capturedActor->GetFormID());
            }
            return RE::BSEventNotifyControl::kContinue;
        }

    private:
        class GetThreadIDCallback : public RE::BSScript::IStackCallbackFunctor {
        public:
            explicit GetThreadIDCallback(RE::Actor* player) : _player(player) {}

            void operator()(RE::BSScript::Variable a_result) override {
                if (!g_ostimAPI || !_player) {
                    SKSE::log::error("SceneJoin callback: API or player no longer valid");
                    return;
                }
                if (!a_result.IsInt()) {
                    SKSE::log::error("SceneJoin callback: OActor.GetThreadID returned non-int");
                    return;
                }
                int threadID = a_result.GetSInt();
                SKSE::log::debug("SceneJoin callback: OActor.GetThreadID returned {}", threadID);
                if (threadID < 0) return;
                SKSE::log::info("SceneJoin: prompting player to join thread {}", threadID);
                RE::Actor* capturedPlayer = _player;
                uint32_t capturedThreadID = static_cast<uint32_t>(threadID);
                HotSwap::Utils::ShowMessageBox(
                    "Do you want to join this scene?",
                    {"Yes", "No"},
                    [capturedPlayer, capturedThreadID](uint32_t choice) {
                        if (choice != 0) {
                            SKSE::log::debug("SceneJoin: player declined to join");
                            return;
                        }
                        SKSE::log::info("SceneJoin: adding player to thread {}", capturedThreadID);
                        int result = HotSwap::ActorManagement::addActorToThread(
                            capturedThreadID, capturedPlayer, g_ostimAPI,
                            [](int32_t newID, void*) {
                                SKSE::GetTaskInterface()->AddTask([newID]() {
                                    SKSE::log::info("SceneJoin: migration complete, newThreadID={}", newID);
                                });
                            });
                        SKSE::log::info("SceneJoin: addActorToThread returned {}", result);
                    });
            }

            void SetObject(const RE::BSTSmartPointer<RE::BSScript::Object>&) override {}

        private:
            RE::Actor* _player;
        };
    };
}

SKSEPluginLoad(const LoadInterface* skse) {
    SKSE::Init(skse);

    SetupLogging();
    SKSE::log::info("OStimHotSwap plugin loading...");
    HotSwap::Settings::GetSingleton()->Load();

    auto papyrus = SKSE::GetPapyrusInterface();
    if (!papyrus->Register(HotSwap::Papyrus::ActorManagement::Register)) {
        SKSE::log::critical("Failed to register papyrus callback");
        return false;
    }

    if (const auto* messaging = SKSE::GetMessagingInterface()) {
        if (!messaging->RegisterListener([](SKSE::MessagingInterface::Message* message) {
                switch (message->type) {
                    case SKSE::MessagingInterface::kPreLoadGame:
                        SKSE::log::info("PreLoadGame...");
                        break;

                    case SKSE::MessagingInterface::kPostLoadGame:
                    case SKSE::MessagingInterface::kNewGame:
                        SKSE::log::info("New game/Load...");
                        break;

                    case SKSE::MessagingInterface::kDataLoaded: {
                        SKSE::log::info("Data loaded successfully.");
                        g_ostimAPI = OstimNG_API::Thread::GetAPI(
                            "OStimHotSwap",
                            SKSE::PluginDeclaration::GetSingleton()->GetVersion());
                        if (g_ostimAPI) {
                            SKSE::log::info("OStim API connected successfully");
                            RE::BSInputDeviceManager::GetSingleton()->AddEventSink(SceneJoinInputSink::GetSingleton());
                            SKSE::log::info("Scene join input sink registered");
                        } else {
                            SKSE::log::error("Failed to connect to OStim API - actor management unavailable");
                        }
                        break;
                    }

                    default:
                        break;
                }
            })) {
            SKSE::log::critical("Failed to register messaging listener.");
            return false;
        }
    } else {
        SKSE::log::critical("Messaging interface unavailable.");
        return false;
    }

    return true;
}
