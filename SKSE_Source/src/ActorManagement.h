#pragma once
#include <vector>
#include "OstimNG-API-Thread.h"

using API = OstimNG_API::Thread::IThreadInterface;

namespace HotSwap::ActorManagement {

    bool canAddActor(uint32_t threadID, RE::Actor* actor, API* api);
    bool addActorToThread(uint32_t threadID, RE::Actor* actor, API* api);

    bool canRemoveActor(uint32_t threadID, uint32_t position, API* api);
    bool removeActorFromThread(uint32_t threadID, uint32_t position, API* api);

    bool canSwapActors(uint32_t threadID, uint32_t posA, uint32_t posB, API* api);
    std::vector<uint32_t> getSwapPartners(uint32_t threadID, RE::Actor* actor, API* api);
    bool swapActors(uint32_t threadID, uint32_t posA, uint32_t posB, API* api);

    void addActorWithUI(uint32_t threadID, API* api);
    void removeActorWithUI(uint32_t threadID, API* api);
    void swapActorsWithUI(uint32_t threadID, API* api);
}
