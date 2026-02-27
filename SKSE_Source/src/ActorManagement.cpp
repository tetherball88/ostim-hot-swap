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
        if (!actor) return false;
        uint32_t formID = actor->GetFormID();

        // Already in THIS thread?
        if (api->GetActorPosition(threadID, formID) != -1) return false;

        // Locked in another thread?
        if (api->IsActorInAnyThread(formID)) return false;

        // Would a compatible scene exist for the new actor set?
        OstimNG_API::Thread::ActorData buf[32];
        uint32_t count = api->GetActors(threadID, buf, 32);

        uint32_t ids[33];
        for (uint32_t i = 0; i < count; i++) ids[i] = buf[i].formID;
        ids[count] = formID;

        return api->HasCompatibleNode(threadID, ids, count + 1);
    }

    int32_t addActorToThread(uint32_t threadID, RE::Actor* actor, API* api,
                             void (*onComplete)(int32_t, void*), void* context) {
        if (!canAddActor(threadID, actor, api)) {
            if (onComplete) onComplete(-1, context);
            return -1;
        }

        OstimNG_API::Thread::ActorData buf[32];
        uint32_t count = api->GetActors(threadID, buf, 32);

        uint32_t ids[33];
        for (uint32_t i = 0; i < count; i++) ids[i] = buf[i].formID;
        ids[count] = actor->GetFormID();

        return api->MigrateThread(threadID, ids, count + 1, onComplete, context, HotSwap::Settings::GetSingleton()->GetMigrationDelayMs());
    }

    bool canRemoveActor(uint32_t threadID, uint32_t position, API* api) {
        OstimNG_API::Thread::ActorData buf[32];
        uint32_t count = api->GetActors(threadID, buf, 32);

        if (position >= count || count <= 1) return false;

        uint32_t ids[32];
        uint32_t newCount = 0;
        for (uint32_t i = 0; i < count; i++)
            if (i != position) ids[newCount++] = buf[i].formID;

        return api->HasCompatibleNode(threadID, ids, newCount);
    }

    int32_t removeActorFromThread(uint32_t threadID, uint32_t position, API* api,
                                  void (*onComplete)(int32_t, void*), void* context) {
        if (!canRemoveActor(threadID, position, api)) {
            if (onComplete) onComplete(-1, context);
            return -1;
        }

        OstimNG_API::Thread::ActorData buf[32];
        uint32_t count = api->GetActors(threadID, buf, 32);

        uint32_t ids[32];
        uint32_t newCount = 0;
        for (uint32_t i = 0; i < count; i++)
            if (i != position) ids[newCount++] = buf[i].formID;

        return api->MigrateThread(threadID, ids, newCount, onComplete, context, HotSwap::Settings::GetSingleton()->GetMigrationDelayMs());
    }

    bool canSwapActors(uint32_t threadID, uint32_t posA, uint32_t posB, API* api) {
        if (posA == posB) return false;

        OstimNG_API::Thread::ActorData buf[32];
        uint32_t count = api->GetActors(threadID, buf, 32);
        if (posA >= count || posB >= count) return false;

        // Don't allow swapping with self
        if (posA == posB) {
            SKSE::log::warn("Cannot swap actors: same position");
            return false;
        }

        if (api->IsUnrestrictedNavigation()) return true;

        // HasCompatibleNode sorts actors internally, so passing swapped IDs would just
        // produce the same sorted order — it can't validate position-specific sex roles.
        // Check intendedSexOnly explicitly: actors must share the same sex to swap roles.
        if (api->IsIntendedSexOnly() && buf[posA].isFemale != buf[posB].isFemale)
            return false;

        uint32_t ids[32];
        for (uint32_t i = 0; i < count; i++) ids[i] = buf[i].formID;
        std::swap(ids[posA], ids[posB]);

        return api->HasCompatibleNode(threadID, ids, count);
    }

    std::vector<uint32_t> getSwapPartners(uint32_t threadID, RE::Actor* actor, API* api) {
        if (!actor) return {};
        int32_t pos = api->GetActorPosition(threadID, actor->GetFormID());
        if (pos == -1) return {};

        OstimNG_API::Thread::ActorData buf[32];
        uint32_t count = api->GetActors(threadID, buf, 32);

        std::vector<uint32_t> partners;
        for (uint32_t i = 0; i < count; i++) {
            if (i != static_cast<uint32_t>(pos) && canSwapActors(threadID, static_cast<uint32_t>(pos), i, api))
                partners.push_back(i);
        }
        return partners;
    }

    int32_t swapActors(uint32_t threadID, uint32_t posA, uint32_t posB, API* api,
                       void (*onComplete)(int32_t, void*), void* context) {
        if (!canSwapActors(threadID, posA, posB, api)) {
            if (onComplete) onComplete(-1, context);
            return -1;
        }

        OstimNG_API::Thread::ActorData buf[32];
        uint32_t count = api->GetActors(threadID, buf, 32);

        uint32_t ids[32];
        for (uint32_t i = 0; i < count; i++) ids[i] = buf[i].formID;
        std::swap(ids[posA], ids[posB]);

        return api->MigrateThread(threadID, ids, count, onComplete, context, HotSwap::Settings::GetSingleton()->GetMigrationDelayMs());
    }


    // ---- UI flows (CommonLibSSE message boxes) ----

    void addActorWithUI(uint32_t threadID, API* api) {
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) return;

        std::vector<RE::Actor*> candidates;
        RE::ProcessLists::GetSingleton()->ForEachHighActor([&](RE::Actor* a) {
            if (a && !a->IsPlayerRef() && a->GetPosition().GetDistance(player->GetPosition()) <= 2000.f)
                if (canAddActor(threadID, a, api)) candidates.push_back(a);
            return RE::BSContainer::ForEachResult::kContinue;
        });

        if (candidates.empty()) {
            SendNotification("No valid actors nearby to add");
            return;
        }

        std::vector<std::string> names;
        for (auto* a : candidates) names.push_back(a->GetName());

        HotSwap::Utils::ShowMessageBox("Select actor to add", names,
            [threadID, candidates, api](uint32_t idx) {
                if (idx >= candidates.size()) return;
                addActorToThread(threadID, candidates[idx], api, [](int32_t newID, void*) {
                    SKSE::GetTaskInterface()->AddTask([newID]() {
                        SendNotification(newID >= 0 ? "Actor added" : "Failed to add actor");
                    });
                });
            });
    }

    void removeActorWithUI(uint32_t threadID, API* api) {
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

        if (positions.empty()) {
            SendNotification("No actors can be removed");
            return;
        }

        HotSwap::Utils::ShowMessageBox("Select actor to remove", names,
            [threadID, positions, api](uint32_t idx) {
                if (idx >= positions.size()) return;
                removeActorFromThread(threadID, positions[idx], api, [](int32_t newID, void*) {
                    SKSE::GetTaskInterface()->AddTask([newID]() {
                        SendNotification(newID >= 0 ? "Actor removed" : "Failed to remove actor");
                    });
                });
            });
    }

    void swapActorsWithUI(uint32_t threadID, API* api) {
        OstimNG_API::Thread::ActorData buf[32];
        uint32_t count = api->GetActors(threadID, buf, 32);
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

                std::vector<std::string> partnerNames;
                std::vector<uint32_t> partnerPositions;
                for (uint32_t i = 0; i < count; i++) {
                    if (i != posA && canSwapActors(threadID, posA, i, api)) {
                        auto* a = RE::TESForm::LookupByID<RE::Actor>(buf[i].formID);
                        partnerNames.push_back(a ? a->GetName() : "Unknown");
                        partnerPositions.push_back(i);
                    }
                }

                if (partnerPositions.empty()) {
                    SendNotification("No valid swap partners");
                    return;
                }

                HotSwap::Utils::ShowMessageBox("Swap: select second actor", partnerNames,
                    [threadID, posA, partnerPositions, api](uint32_t idx) {
                        if (idx >= partnerPositions.size()) return;
                        swapActors(threadID, posA, partnerPositions[idx], api, [](int32_t newID, void*) {
                            SKSE::GetTaskInterface()->AddTask([newID]() {
                                SendNotification(newID >= 0 ? "Actors swapped" : "Failed to swap actors");
                            });
                        });
                    });
            });
    }
}
