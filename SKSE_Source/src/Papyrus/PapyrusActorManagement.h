#pragma once
#include "ActorManagement.h"
#include "Settings.h"
#include <SKSE/SKSE.h>

extern OstimNG_API::Thread::IThreadInterface* g_ostimAPI;

namespace HotSwap::Papyrus::ActorManagement {

    bool CanAddActor(RE::StaticFunctionTag*, int threadID, RE::Actor* actor) {
        if (!g_ostimAPI) { SKSE::log::error("CanAddActor: OStim API not available"); return false; }
        return HotSwap::ActorManagement::canAddActor(
            static_cast<uint32_t>(threadID), actor, g_ostimAPI);
    }

    bool CanRemoveActor(RE::StaticFunctionTag*, int threadID, int position) {
        if (!g_ostimAPI) { SKSE::log::error("CanRemoveActor: OStim API not available"); return false; }
        return HotSwap::ActorManagement::canRemoveActor(
            static_cast<uint32_t>(threadID),
            static_cast<uint32_t>(position),
            g_ostimAPI);
    }

    bool CanSwapActors(RE::StaticFunctionTag*, int threadID, int posA, int posB) {
        if (!g_ostimAPI) { SKSE::log::error("CanSwapActors: OStim API not available"); return false; }
        return HotSwap::ActorManagement::canSwapActors(
            static_cast<uint32_t>(threadID),
            static_cast<uint32_t>(posA),
            static_cast<uint32_t>(posB),
            g_ostimAPI);
    }

    int SwapActors(RE::StaticFunctionTag*, int threadID, int posA, int posB) {
        if (!g_ostimAPI) { SKSE::log::error("SwapActors: OStim API not available"); return -1; }
        SKSE::log::debug("SwapActors: threadID={} posA={} posB={}", threadID, posA, posB);
        int result = HotSwap::ActorManagement::swapActors(
            static_cast<uint32_t>(threadID),
            static_cast<uint32_t>(posA),
            static_cast<uint32_t>(posB),
            g_ostimAPI,
            [](int32_t newID, void*) { SKSE::log::debug("SwapActors: migration complete, newThreadID={}", newID); });
        SKSE::log::debug("SwapActors: scheduled={}", result);
        return result;
    }

    std::vector<int> GetSwapPartners(RE::StaticFunctionTag*, int threadID, RE::Actor* actor) {
        if (!g_ostimAPI) { SKSE::log::error("GetSwapPartners: OStim API not available"); return {}; }
        auto v = HotSwap::ActorManagement::getSwapPartners(
            static_cast<uint32_t>(threadID), actor, g_ostimAPI);
        return std::vector<int>(v.begin(), v.end());
    }

    void SwapActorsWithUI(RE::StaticFunctionTag*, int threadID) {
        if (!g_ostimAPI) { SKSE::log::error("SwapActorsWithUI: OStim API not available"); return; }
        HotSwap::ActorManagement::swapActorsWithUI(static_cast<uint32_t>(threadID), g_ostimAPI);
    }

    int AddActorToThread(RE::StaticFunctionTag*, int threadID, RE::Actor* actor) {
        if (!g_ostimAPI) { SKSE::log::error("AddActorToThread: OStim API not available"); return -1; }
        SKSE::log::debug("AddActorToThread: threadID={} actor={:X}", threadID, actor ? actor->GetFormID() : 0);
        int result = HotSwap::ActorManagement::addActorToThread(
            static_cast<uint32_t>(threadID), actor, g_ostimAPI,
            [](int32_t newID, void*) { SKSE::log::debug("AddActorToThread: migration complete, newThreadID={}", newID); });
        SKSE::log::debug("AddActorToThread: scheduled={}", result);
        return result;
    }

    void AddActorWithUI(RE::StaticFunctionTag*, int threadID) {
        if (!g_ostimAPI) { SKSE::log::error("AddActorWithUI: OStim API not available"); return; }
        HotSwap::ActorManagement::addActorWithUI(static_cast<uint32_t>(threadID), g_ostimAPI);
    }

    int RemoveActorFromThread(RE::StaticFunctionTag*, int threadID, int position) {
        if (!g_ostimAPI) { SKSE::log::error("RemoveActorFromThread: OStim API not available"); return -1; }
        SKSE::log::debug("RemoveActorFromThread: threadID={} position={}", threadID, position);
        int result = HotSwap::ActorManagement::removeActorFromThread(
            static_cast<uint32_t>(threadID),
            static_cast<uint32_t>(position),
            g_ostimAPI,
            [](int32_t newID, void*) { SKSE::log::debug("RemoveActorFromThread: migration complete, newThreadID={}", newID); });
        SKSE::log::debug("RemoveActorFromThread: scheduled={}", result);
        return result;
    }

    void RemoveActorWithUI(RE::StaticFunctionTag*, int threadID) {
        if (!g_ostimAPI) { SKSE::log::error("RemoveActorWithUI: OStim API not available"); return; }
        HotSwap::ActorManagement::removeActorWithUI(static_cast<uint32_t>(threadID), g_ostimAPI);
    }

    // Replace the thread's actor set with the given ordered actor array.
    // Returns 0 if scheduled, -1 on immediate failure. Migration completes asynchronously.
    int MigrateThread(RE::StaticFunctionTag*, int threadID, std::vector<RE::Actor*> actors) {
        if (!g_ostimAPI || actors.empty()) {
            SKSE::log::error("MigrateThread: OStim API not available or empty actor list");
            return -1;
        }
        SKSE::log::debug("MigrateThread: threadID={} actorCount={}", threadID, actors.size());
        uint32_t ids[32];
        uint32_t count = 0;
        for (auto* actor : actors) {
            if (!actor || count >= 32) {
                SKSE::log::warn("MigrateThread: null actor or actor limit reached at index {}", count);
                return -1;
            }
            ids[count++] = actor->GetFormID();
        }
        int startDelayMs = HotSwap::Settings::GetSingleton()->GetMigrationDelayMs();
        int result = g_ostimAPI->MigrateThread(static_cast<uint32_t>(threadID), ids, count,
            [](int32_t newID, void*) { SKSE::log::debug("MigrateThread: migration complete, newThreadID={}", newID); },
            nullptr, startDelayMs);
        SKSE::log::debug("MigrateThread: scheduled={}", result);
        return result;
    }

    bool Register(RE::BSScript::IVirtualMachine* vm) {
        vm->RegisterFunction("CanAddActor",           "OStimHotSwap", CanAddActor);
        vm->RegisterFunction("CanRemoveActor",        "OStimHotSwap", CanRemoveActor);
        vm->RegisterFunction("CanSwapActors",         "OStimHotSwap", CanSwapActors);
        vm->RegisterFunction("SwapActors",            "OStimHotSwap", SwapActors);
        vm->RegisterFunction("GetSwapPartners",       "OStimHotSwap", GetSwapPartners);
        vm->RegisterFunction("SwapActorsWithUI",      "OStimHotSwap", SwapActorsWithUI);
        vm->RegisterFunction("AddActorToThread",      "OStimHotSwap", AddActorToThread);
        vm->RegisterFunction("AddActorWithUI",        "OStimHotSwap", AddActorWithUI);
        vm->RegisterFunction("RemoveActorFromThread", "OStimHotSwap", RemoveActorFromThread);
        vm->RegisterFunction("RemoveActorWithUI",     "OStimHotSwap", RemoveActorWithUI);
        vm->RegisterFunction("MigrateThread",         "OStimHotSwap", MigrateThread);
        return true;
    }
}
