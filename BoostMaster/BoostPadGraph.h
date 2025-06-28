#pragma once
#include <vector>
#include "bakkesmod/wrappers/WrapperStructs.h"
#include "BoostPadData.h"

struct PadNode {
    int index; // index in the static pad list
    const StaticBoostPad* pad;
    std::vector<int> neighbors;
};

class BoostPadGraph {
public:
    // Build a graph from the static pad list for the current map
    static std::vector<PadNode> Build(const std::vector<StaticBoostPad>& pads);
    // Find the nearest pad to a location
    static int Nearest(const std::vector<PadNode>& graph, const Vector& loc);
    // Find a path between two pad indices (Dijkstra or A*)
    static std::vector<int> FindPath(const std::vector<PadNode>& graph, int start, int goal, bool useAStar = false);
};
