#include "pch.h"
#include "BoostPadGraph.h"
#include <vector>
#include <queue>
#include <limits>
#include <cmath>

// Helper: Euclidean distance
static float dist(const Vector& a, const Vector& b) {
    float dx = a.X - b.X, dy = a.Y - b.Y, dz = a.Z - b.Z;
    return std::sqrt(dx*dx + dy*dy + dz*dz);
}

// Build a simple fully-connected graph (all pads are neighbors)
std::vector<PadNode> BoostPadGraph::Build(const std::vector<StaticBoostPad>& pads) {
    std::vector<PadNode> graph;
    for (size_t i = 0; i < pads.size(); ++i) {
        PadNode node;
        node.index = static_cast<int>(i);
        node.pad = &pads[i];
        // Optionally, only connect to nearby pads (here: all pads are neighbors)
        for (size_t j = 0; j < pads.size(); ++j) {
            if (i != j) node.neighbors.push_back(static_cast<int>(j));
        }
        graph.push_back(node);
    }
    return graph;
}

// Find the nearest pad to a location
int BoostPadGraph::Nearest(const std::vector<PadNode>& graph, const Vector& loc) {
    float bestDist = std::numeric_limits<float>::max();
    int bestIdx = 0;
    for (const auto& node : graph) {
        float d = dist(node.pad->location, loc);
        if (d < bestDist) {
            bestDist = d;
            bestIdx = node.index;
        }
    }
    return bestIdx;
}

// Dijkstra's or A* algorithm for shortest path (by distance)
std::vector<int> BoostPadGraph::FindPath(const std::vector<PadNode>& graph, int start, int goal, bool useAStar) {
    if (graph.empty() || start < 0 || start >= (int)graph.size() || goal < 0 || goal >= (int)graph.size()) {
        LOG_ERROR("FindPath called with empty graph or invalid indices.");
        return {};
    }
    std::vector<float> cost(graph.size(), std::numeric_limits<float>::max());
    std::vector<int> prev(graph.size(), -1);
    cost[start] = 0;
    using Pair = std::pair<float, int>;
    auto heuristic = [&](int idx) {
        if (!useAStar) return 0.0f;
        return dist(graph[idx].pad->location, graph[goal].pad->location);
    };
    std::priority_queue<Pair, std::vector<Pair>, std::greater<Pair>> q;
    q.emplace(Pair(heuristic(start), start));
    while (!q.empty()) {
        auto [c, u] = q.top(); q.pop();
        if (u == goal) break;
        for (int v : graph[u].neighbors) {
            float alt = cost[u] + dist(graph[u].pad->location, graph[v].pad->location);
            if (alt < cost[v]) {
                cost[v] = alt;
                prev[v] = u;
                q.emplace(Pair(alt + heuristic(v), v));
            }
        }
    }
    std::vector<int> path;
    for (int at = goal; at != -1; at = prev[at]) path.push_back(at);
    std::reverse(path.begin(), path.end());
    if (path.empty() || path.front() != start) {
        LOG_ERROR("FindPath failed to find a valid path.");
        path.clear();
    }
    return path;
}