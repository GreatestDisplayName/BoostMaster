#pragma once
#include <vector>
#include "bakkesmod/wrappers/GameObject/BoostPickupWrapper.h"
#include "bakkesmod/wrappers/GameWrapper.h"

struct PadNode {
    BoostPickupWrapper pad;
    std::vector<int> neighbors;
};

class BoostPadGraph {
public:
    static std::vector<PadNode> Build(GameWrapper* gw);
    static int Nearest(const std::vector<PadNode>&, const Vector&);
    static std::vector<int> FindPath(const std::vector<PadNode>&, int, int);
};
