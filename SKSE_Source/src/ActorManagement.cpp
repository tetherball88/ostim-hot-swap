#include "ActorManagement.h"
#include "Settings.h"
#include "Utils/MessageBox.h"
#include <SKSE/SKSE.h>

namespace {
    void SendNotification(const char* text) {
        auto* queue = RE::UIMessageQueue::GetSingleton();
        if (!queue) return;
        auto* data = static_cast<RE::HUDData*>(
            queue->CreateUIMessageData(RE::InterfaceStrings::GetSingleton()->hudData));
        if (!data) return;
        data->type = RE::HUD_MESSAGE_TYPE::kNotification;
        data->text = text;
        queue->AddMessage(RE::InterfaceStrings::GetSingleton()->hudMenu, RE::UI_MESSAGE_TYPE::kUpdate, data);
    }
}

namespace HotSwap::ActorManagement {

    // ---- Validation ----

    bool canAddActor(uint32_t threadID, RE::Actor* actor, API* api) {
        if (!actor) {
            SKSE::log::warn("canAddActor: null actor");
            return false;
        }
        uint32_t formID = actor->GetFormID();
        SKSE::log::trace("canAddActor: threadID={} actor={:X}", threadID, formID);

        // Already in THIS thread?
        if (api->GetActorPosition(threadID, formID) != -1) {
            SKSE::log::debug("canAddActor: actor {:X} already in thread {}", formID, threadID);
            return false;
        }

        // Locked in another thread?
        if (api->IsActorInAnyThread(formID)) {
            SKSE::log::debug("canAddActor: actor {:X} is locked in another thread", formID, threadID);
            return false;
        }

        // Would a compatible scene exist for the new actor set?
        OstimNG_API::Thread::ActorData buf[32];
        uint32_t count = api->GetActors(threadID, buf, 32);

        uint32_t ids[33];
        for (uint32_t i = 0; i < count; i++) ids[i] = buf[i].formID;
        ids[count] = formID;

        bool compatible = api->HasCompatibleNode(threadID, ids, count + 1);
        SKSE::log::debug("canAddActor: HasCompatibleNode={} for actor {:X} in thread {}", compatible, formID, threadID);
        return compatible;
    }

    int32_t addActorToThread(uint32_t threadID, RE::Actor* actor, API* api,
                             void (*onComplete)(int32_t, void*), void* context) {
        SKSE::log::debug("addActorToThread: threadID={} actor={:X}", threadID, actor ? actor->GetFormID() : 0);
        if (!canAddActor(threadID, actor, api)) {
            SKSE::log::warn("addActorToThread: validation failed for actor {:X} in thread {}",
                actor ? actor->GetFormID() : 0, threadID);
            if (onComplete) onComplete(-1, context);
            return -1;
        }

        OstimNG_API::Thread::ActorData buf[32];
        uint32_t count = api->GetActors(threadID, buf, 32);

        uint32_t ids[33];
        for (uint32_t i = 0; i < count; i++) ids[i] = buf[i].formID;
        ids[count] = actor->GetFormID();

        int32_t result = api->MigrateThread(threadID, ids, count + 1, onComplete, context, HotSwap::Settings::GetSingleton()->GetMigrationDelayMs());
        SKSE::log::debug("addActorToThread: MigrateThread returned {}", result);
        return result;
    }

    bool canRemoveActor(uint32_t threadID, uint32_t position, API* api) {
        SKSE::log::trace("canRemoveActor: threadID={} position={}", threadID, position);
        OstimNG_API::Thread::ActorData buf[32];
        uint32_t count = api->GetActors(threadID, buf, 32);

        if (position >= count || count <= 1) {
            SKSE::log::debug("canRemoveActor: invalid — position={} count={}", position, count);
            return false;
        }

        uint32_t ids[32];
        uint32_t newCount = 0;
        for (uint32_t i = 0; i < count; i++)
            if (i != position) ids[newCount++] = buf[i].formID;

        bool compatible = api->HasCompatibleNode(threadID, ids, newCount);
        SKSE::log::debug("canRemoveActor: HasCompatibleNode={} for position {} in thread {}", compatible, position, threadID);
        return compatible;
    }

    int32_t removeActorFromThread(uint32_t threadID, uint32_t position, API* api,
                                  void (*onComplete)(int32_t, void*), void* context) {
        SKSE::log::debug("removeActorFromThread: threadID={} position={}", threadID, position);
        if (!canRemoveActor(threadID, position, api)) {
            SKSE::log::warn("removeActorFromThread: validation failed for position {} in thread {}", position, threadID);
            if (onComplete) onComplete(-1, context);
            return -1;
        }

        OstimNG_API::Thread::ActorData buf[32];
        uint32_t count = api->GetActors(threadID, buf, 32);

        uint32_t ids[32];
        uint32_t newCount = 0;
        for (uint32_t i = 0; i < count; i++)
            if (i != position) ids[newCount++] = buf[i].formID;

        int32_t result = api->MigrateThread(threadID, ids, newCount, onComplete, context, HotSwap::Settings::GetSingleton()->GetMigrationDelayMs());
        SKSE::log::debug("removeActorFromThread: MigrateThread returned {}", result);
        return result;
    }

    bool canSwapActors(uint32_t threadID, uint32_t posA, uint32_t posB, API* api) {
        SKSE::log::trace("canSwapActors: threadID={} posA={} posB={}", threadID, posA, posB);
        if (posA == posB) {
            SKSE::log::warn("canSwapActors: same position {}", posA);
            return false;
        }

        OstimNG_API::Thread::ActorData buf[32];
        uint32_t count = api->GetActors(threadID, buf, 32);
        if (posA >= count || posB >= count) {
            SKSE::log::warn("canSwapActors: position out of range — posA={} posB={} count={}", posA, posB, count);
            return false;
        }

        if (api->IsUnrestrictedNavigation()) {
            SKSE::log::trace("canSwapActors: unrestricted navigation, swap allowed");
            return true;
        }

        // HasCompatibleNode sorts actors internally, so passing swapped IDs would just
        // produce the same sorted order — it can't validate position-specific sex roles.
        // Check intendedSexOnly explicitly: actors must share the same sex to swap roles.
        if (api->IsIntendedSexOnly() && buf[posA].isFemale != buf[posB].isFemale) {
            SKSE::log::debug("canSwapActors: sex restriction prevents swap — posA={} posB={}", posA, posB);
            return false;
        }

        uint32_t ids[32];
        for (uint32_t i = 0; i < count; i++) ids[i] = buf[i].formID;
        std::swap(ids[posA], ids[posB]);

        bool compatible = api->HasCompatibleNode(threadID, ids, count);
        SKSE::log::debug("canSwapActors: HasCompatibleNode={}", compatible);
        return compatible;
    }

    std::vector<uint32_t> getSwapPartners(uint32_t threadID, RE::Actor* actor, API* api) {
        SKSE::log::trace("getSwapPartners: threadID={} actor={:X}", threadID, actor ? actor->GetFormID() : 0);
        if (!actor) return {};
        int32_t pos = api->GetActorPosition(threadID, actor->GetFormID());
        if (pos == -1) {
            SKSE::log::warn("getSwapPartners: actor {:X} not found in thread {}", actor->GetFormID(), threadID);
            return {};
        }

        OstimNG_API::Thread::ActorData buf[32];
        uint32_t count = api->GetActors(threadID, buf, 32);

        std::vector<uint32_t> partners;
        for (uint32_t i = 0; i < count; i++) {
            if (i != static_cast<uint32_t>(pos) && canSwapActors(threadID, static_cast<uint32_t>(pos), i, api))
                partners.push_back(i);
        }
        SKSE::log::debug("getSwapPartners: found {} partner(s) for actor {:X}", partners.size(), actor->GetFormID());
        return partners;
    }

    int32_t swapActors(uint32_t threadID, uint32_t posA, uint32_t posB, API* api,
                       void (*onComplete)(int32_t, void*), void* context) {
        SKSE::log::debug("swapActors: threadID={} posA={} posB={}", threadID, posA, posB);
        if (!canSwapActors(threadID, posA, posB, api)) {
            SKSE::log::warn("swapActors: validation failed for posA={} posB={} in thread {}", posA, posB, threadID);
            if (onComplete) onComplete(-1, context);
            return -1;
        }

        OstimNG_API::Thread::ActorData buf[32];
        uint32_t count = api->GetActors(threadID, buf, 32);

        uint32_t ids[32];
        for (uint32_t i = 0; i < count; i++) ids[i] = buf[i].formID;
        std::swap(ids[posA], ids[posB]);

        int32_t result = api->MigrateThread(threadID, ids, count, onComplete, context, HotSwap::Settings::GetSingleton()->GetMigrationDelayMs());
        SKSE::log::debug("swapActors: MigrateThread returned {}", result);
        return result;
    }


    // ---- UI flows (CommonLibSSE message boxes) ----

    void addActorWithUI(uint32_t threadID, API* api) {
        SKSE::log::debug("addActorWithUI: threadID={}", threadID);
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) return;

        std::vector<RE::Actor*> candidates;
        RE::ProcessLists::GetSingleton()->ForEachHighActor([&](RE::Actor* a) {
            if (a && !a->IsPlayerRef() && a->GetPosition().GetDistance(player->GetPosition()) <= 2000.f)
                if (canAddActor(threadID, a, api)) candidates.push_back(a);
            return RE::BSContainer::ForEachResult::kContinue;
        });

        SKSE::log::debug("addActorWithUI: {} candidate(s) found", candidates.size());
        if (candidates.empty()) {
            SendNotification("No valid actors nearby to add");
            return;
        }

        std::vector<std::string> names;
        for (auto* a : candidates) names.push_back(a->GetName());

        HotSwap::Utils::ShowMessageBox("Select actor to add", names,
            [threadID, candidates, api](uint32_t idx) {
                if (idx >= candidates.size()) return;
                SKSE::log::debug("addActorWithUI: selected idx={} actor={:X}", idx, candidates[idx]->GetFormID());
                addActorToThread(threadID, candidates[idx], api, [](int32_t newID, void*) {
                    SKSE::GetTaskInterface()->AddTask([newID]() {
                        SKSE::log::debug("addActorWithUI: migration complete, newThreadID={}", newID);
                        SendNotification(newID >= 0 ? "Actor added" : "Failed to add actor");
                    });
                });
            });
    }

    void removeActorWithUI(uint32_t threadID, API* api) {
        SKSE::log::debug("removeActorWithUI: threadID={}", threadID);
        OstimNG_API::Thread::ActorData buf[32];
        uint32_t count = api->GetActors(threadID, buf, 32);

        std::vector<std::string> names;
        std::vector<uint32_t> positions;
        for (uint32_t i = 0; i < count; i++) {
            if (canRemoveActor(threadID, i, api)) {
                auto* actor = RE::TESForm::LookupByID<RE::Actor>(buf[i].formID);
                std::string name = actor ? actor->GetName() : "Unknown";
                names.push_back(name);
                positions.push_back(i);
            }
        }

        SKSE::log::debug("removeActorWithUI: {} removable actor(s)", positions.size());
        if (positions.empty()) {
            SendNotification("No actors can be removed");
            return;
        }

        HotSwap::Utils::ShowMessageBox("Select actor to remove", names,
            [threadID, positions, api](uint32_t idx) {
                if (idx >= positions.size()) return;
                SKSE::log::debug("removeActorWithUI: selected idx={} position={}", idx, positions[idx]);
                removeActorFromThread(threadID, positions[idx], api, [](int32_t newID, void*) {
                    SKSE::GetTaskInterface()->AddTask([newID]() {
                        SKSE::log::debug("removeActorWithUI: migration complete, newThreadID={}", newID);
                        SendNotification(newID >= 0 ? "Actor removed" : "Failed to remove actor");
                    });
                });
            });
    }

    void swapActorsWithUI(uint32_t threadID, API* api) {
        OstimNG_API::Thread::ActorData buf[32];
        uint32_t count = api->GetActors(threadID, buf, 32);
        SKSE::log::debug("swapActorsWithUI: threadID={} actorCount={}", threadID, count);
        if (count < 2) { SendNotification("Not enough actors to swap"); return; }

        std::vector<std::string> names;
        for (uint32_t i = 0; i < count; i++) {
            auto* actor = RE::TESForm::LookupByID<RE::Actor>(buf[i].formID);
            names.push_back(actor ? actor->GetName() : "Unknown");
        }

        // First selection: pick actor A
        HotSwap::Utils::ShowMessageBox("Swap: select first actor", names,
            [threadID, buf, count, api](uint32_t posA) {
                if (posA >= count) return;
                SKSE::log::debug("swapActorsWithUI: selected posA={}", posA);

                std::vector<std::string> partnerNames;
                std::vector<uint32_t> partnerPositions;
                for (uint32_t i = 0; i < count; i++) {
                    if (i != posA && canSwapActors(threadID, posA, i, api)) {
                        auto* a = RE::TESForm::LookupByID<RE::Actor>(buf[i].formID);
                        partnerNames.push_back(a ? a->GetName() : "Unknown");
                        partnerPositions.push_back(i);
                    }
                }

                SKSE::log::debug("swapActorsWithUI: {} valid partner(s) for posA={}", partnerPositions.size(), posA);
                if (partnerPositions.empty()) {
                    SendNotification("No valid swap partners");
                    return;
                }

                HotSwap::Utils::ShowMessageBox("Swap: select second actor", partnerNames,
                    [threadID, posA, partnerPositions, api](uint32_t idx) {
                        if (idx >= partnerPositions.size()) return;
                        SKSE::log::debug("swapActorsWithUI: selected posB={}", partnerPositions[idx]);
                        swapActors(threadID, posA, partnerPositions[idx], api, [](int32_t newID, void*) {
                            SKSE::GetTaskInterface()->AddTask([newID]() {
                                SKSE::log::debug("swapActorsWithUI: migration complete, newThreadID={}", newID);
                                SendNotification(newID >= 0 ? "Actors swapped" : "Failed to swap actors");
                            });
                        });
                    });
            });
    }
}
