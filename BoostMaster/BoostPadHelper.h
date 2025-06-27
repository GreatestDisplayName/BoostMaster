// BoostPadHelper.h
#pragma once
#include "pch.h"
#include <vector>
#include "bakkesmod/wrappers/GameWrapper.h"
#include "bakkesmod/wrappers/GameEvent/ServerWrapper.h"
#include "bakkesmod/wrappers/GameObject/BoostPickupWrapper.h"
#include "bakkesmod/wrappers/WrapperStructs.h"

class BoostPadHelper {
public:
    // Returns all boost pads in the current game (empty if not in game)
    static std::vector<BoostPickupWrapper> GetAllPads(GameWrapper* gw) {
        std::vector<BoostPickupWrapper> pads;
        if (!gw || !gw->IsInGame()) return pads;
        auto arr = BoostPickupWrapper::GetAll(gw); // ArrayWrapper<BoostPickupWrapper>
        for (int i = 0; i < arr.Count(); ++i) {
            auto pad = arr.Get(i);
            if (!pad.IsNull())
                pads.push_back(pad);
        }
        return pads;
    }
};
