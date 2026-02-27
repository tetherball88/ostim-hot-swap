#pragma once
#include "ActorManagement.h"

extern OstimNG_API::Thread::IThreadInterface* g_ostimAPI;

namespace HotSwap::Papyrus::ActorManagement {

    bool CanAddActor(RE::StaticFunctionTag*, int threadID, RE::Actor* actor) {
        if (!g_ostimAPI) return false;
        return HotSwap::ActorManagement::canAddActor(
            static_cast<uint32_t>(threadID), actor, g_ostimAPI);
    }

    bool CanRemoveActor(RE::StaticFunctionTag*, int threadID, int position) {
        if (!g_ostimAPI) return false;
        return HotSwap::ActorManagement::canRemoveActor(
            static_cast<uint32_t>(threadID),
            static_cast<uint32_t>(position),
            g_ostimAPI);
    }

    bool CanSwapActors(RE::StaticFunctionTag*, int threadID, int posA, int posB) {
        if (!g_ostimAPI) return false;
        return HotSwap::ActorManagement::canSwapActors(
            static_cast<uint32_t>(threadID),
            static_cast<uint32_t>(posA),
            static_cast<uint32_t>(posB),
            g_ostimAPI);
    }

    int SwapActors(RE::StaticFunctionTag*, int threadID, int posA, int posB) {
        if (!g_ostimAPI) return -1;
        return HotSwap::ActorManagement::swapActors(
            static_cast<uint32_t>(threadID),
            static_cast<uint32_t>(posA),
            static_cast<uint32_t>(posB),
            g_ostimAPI);
    }

    std::vector<int> GetSwapPartners(RE::StaticFunctionTag*, int threadID, RE::Actor* actor) {
        if (!g_ostimAPI) return {};
        auto v = HotSwap::ActorManagement::getSwapPartners(
            static_cast<uint32_t>(threadID), actor, g_ostimAPI);
        return std::vector<int>(v.begin(), v.end());
    }

    void SwapActorsWithUI(RE::StaticFunctionTag*, int threadID) {
        if (g_ostimAPI)
            HotSwap::ActorManagement::swapActorsWithUI(static_cast<uint32_t>(threadID), g_ostimAPI);
    }

    int AddActorToThread(RE::StaticFunctionTag*, int threadID, RE::Actor* actor) {
        if (!g_ostimAPI) return -1;
        return HotSwap::ActorManagement::addActorToThread(
            static_cast<uint32_t>(threadID), actor, g_ostimAPI);
    }

    void AddActorWithUI(RE::StaticFunctionTag*, int threadID) {
        if (g_ostimAPI)
            HotSwap::ActorManagement::addActorWithUI(static_cast<uint32_t>(threadID), g_ostimAPI);
    }

    int RemoveActorFromThread(RE::StaticFunctionTag*, int threadID, int position) {
        if (!g_ostimAPI) return -1;
        return HotSwap::ActorManagement::removeActorFromThread(
            static_cast<uint32_t>(threadID),
            static_cast<uint32_t>(position),
            g_ostimAPI);
    }

    void RemoveActorWithUI(RE::StaticFunctionTag*, int threadID) {
        if (g_ostimAPI)
            HotSwap::ActorManagement::removeActorWithUI(static_cast<uint32_t>(threadID), g_ostimAPI);
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
        return true;
    }
}
