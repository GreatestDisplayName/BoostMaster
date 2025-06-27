#include "pch.h"
#include "BoostPadGraph.h"
#include "BoostPadHelper.h"
#include <queue>
#include <unordered_map>
#include <cmath>

std::vector<PadNode> BoostPadGraph::Build(GameWrapper* gw) {
    std::vector<PadNode> g;
    auto pads = BoostPadHelper::GetAllPads(gw);
    for (size_t i = 0; i < pads.size(); ++i) {
        PadNode n{ pads[i], {} };
        g.push_back(n);
    }
    int n = static_cast<int>(g.size());
    for (int i = 0; i < n; ++i)
        for (int j = i + 1; j < n; ++j) {
            g[i].neighbors.push_back(j);
            g[j].neighbors.push_back(i);
        }
    return g;
}

int BoostPadGraph::Nearest(const std::vector<PadNode>& g, const Vector& loc) {
    int best = -1;
    float bd = FLT_MAX;
    for (int i = 0; i < (int)g.size(); ++i) {
        auto p = g[i].pad.GetLocation();
        float dx = p.X - loc.X, dy = p.Y - loc.Y, dz = p.Z - loc.Z;
        float d = sqrt(dx * dx + dy * dy + dz * dz);
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

std::vector<int> BoostPadGraph::FindPath(const std::vector<PadNode>& g, int start, int goal) {
    std::vector<int> path;
    if (start < 0 || goal < 0 || start >= (int)g.size() || goal >= (int)g.size()) return path;
    auto heuristic = [&](int a, int b) {
        auto pa = g[a].pad.GetLocation(), pb = g[b].pad.GetLocation();
        float dx = pa.X - pb.X, dy = pa.Y - pb.Y, dz = pa.Z - pb.Z;
        return sqrt(dx * dx + dy * dy + dz * dz);
    };
    std::unordered_map<int, float> gS;
    std::unordered_map<int, int> cf;
    auto cmp = [&](int l, int r) { return (gS[l] + heuristic(l, goal)) > (gS[r] + heuristic(r, goal)); };
    std::priority_queue<int, std::vector<int>, decltype(cmp)> open(cmp);
    gS[start] = 0;
    open.push(start);
    while (!open.empty()) {
        int cur = open.top();
        open.pop();
        if (cur == goal) {
            int n = goal;
            while (cf.count(n)) {
                path.push_back(n);
                n = cf[n];
            }
            path.push_back(start);
            std::reverse(path.begin(), path.end());
            return path;
        }
        for (int nb : g[cur].neighbors) {
            float t = gS[cur] + heuristic(cur, nb);
            if (!gS.count(nb) || t < gS[nb]) {
                cf[nb] = cur;
                gS[nb] = t;
                open.push(nb);
            }
        }
    }
    return path;
}