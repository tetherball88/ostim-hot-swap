#include "MessageBox.h"
#include "RE/M/MessageBoxMenu.h"

namespace HotSwap::Utils {

    namespace {
        class MessageBoxCallback : public RE::IMessageBoxCallback {
        public:
            explicit MessageBoxCallback(std::function<void(uint32_t)> callback) : _callback{std::move(callback)} {}
            ~MessageBoxCallback() override = default;

            void Run(RE::IMessageBoxCallback::Message message) override {
                _callback(static_cast<uint32_t>(message));
            }

        private:
            std::function<void(uint32_t)> _callback;
        };

        class UIExtCallbackFunctor : public RE::BSScript::IStackCallbackFunctor {
        public:
            explicit UIExtCallbackFunctor(std::function<void(uint32_t)> callback) : _callback{std::move(callback)} {}

            void operator()(RE::BSScript::Variable a_result) override {
                if (a_result.IsInt()) {
                    int index = a_result.GetSInt();
                    if (index < 0) index = 0;
                    _callback(static_cast<uint32_t>(index));
                }
            }

            void SetObject(const RE::BSTSmartPointer<RE::BSScript::Object>&) override {}

        private:
            std::function<void(uint32_t)> _callback;
        };
    }

    void ShowMessageBox(
        const std::string& body,
        const std::vector<std::string>& buttons,
        std::function<void(uint32_t)> callback)
    {
        RE::TESDataHandler* handler = RE::TESDataHandler::GetSingleton();
        if (handler && handler->GetLoadedModIndex("UIExtensions.esp")) {
            const auto skyrimVM = RE::SkyrimVM::GetSingleton();
            auto vm = skyrimVM ? skyrimVM->impl : nullptr;
            if (vm) {
                RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> functor(
                    new UIExtCallbackFunctor(std::move(callback)));
                auto args = RE::MakeFunctionArguments(std::string(body), std::vector<std::string>(buttons));
                vm->DispatchStaticCall("OSKSE", "UIExtMessageBox", args, functor);
                return;
            }
        }

        SKSE::GetTaskInterface()->AddTask([body, buttons, callback = std::move(callback)]() mutable {
            auto* messagebox = RE::MessageDataFactoryManager::GetSingleton()
                ->GetCreator<RE::MessageBoxData>(RE::InterfaceStrings::GetSingleton()->messageBoxData)
                ->Create();
            messagebox->callback = RE::make_smart<MessageBoxCallback>(std::move(callback));
            messagebox->bodyText = body;
            for (const auto& text : buttons) {
                messagebox->buttonText.push_back(text.c_str());
            }
            RE::MessageBoxMenu::QueueMessage(messagebox);
        });
    }
}
