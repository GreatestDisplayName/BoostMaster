#include "pch.h"
#include "BoostPadHelper.h"
#include "BoostPadData.h"
#include "BoostMaster.h"
#include "BoostSettingsWindow.h"
#include <vector>
#include <algorithm>
#include <optional>

// Cache for the last used map and its pads
namespace {
    std::string cachedMapName;
    const std::vector<StaticBoostPad>* cachedPads = nullptr;
}

const std::vector<StaticBoostPad>& BoostPadHelper::GetCachedPads(BoostMaster* plugin) {
    std::string mapName = plugin->gameWrapper->GetCurrentMap();
    plugin->cvarManager->log("[BoostMaster] Current map name: " + mapName);
    if (!cachedPads || cachedMapName != mapName) {
        cachedMapName = mapName;
        cachedPads = &GetStaticBoostPadsForMap(mapName);
        plugin->cvarManager->log("[BoostMaster] Loaded " + std::to_string(cachedPads->size()) + " boost pads for this map.");
    }
    return *cachedPads;
}

void BoostPadHelper::DrawAllPads(BoostMaster* plugin, std::optional<PadType> filterType) {
    if (!plugin || !plugin->gameWrapper) return;
    if (!BoostSettingsWindow::ShouldShowPads()) return;
    if (!filterType) filterType = BoostSettingsWindow::GetPadTypeFilter();
    const auto& pads = GetCachedPads(plugin);
    for (const auto& pad : pads) {
        if (filterType && pad.type != *filterType) continue;
        // Ideally draw boost pad spheres or circles here; fallback to logging:
        plugin->cvarManager->log("[BoostMaster] Pad at: (" + std::to_string(pad.location.X) + ", " + std::to_string(pad.location.Y) + ", " + std::to_string(pad.location.Z) + ") type: " + (pad.type == PadType::Big ? "Big" : "Small"));
    }
}

void BoostPadHelper::DrawPath(BoostMaster* plugin, const std::vector<int>& path) {
    if (!plugin || !plugin->gameWrapper) return;
    const auto& pads = GetCachedPads(plugin);
    if (pads.empty() || path.size() < 2) return;
    for (size_t i = 1; i < path.size(); ++i) {
        if (path[i - 1] < 0 || path[i - 1] >= (int)pads.size() || path[i] < 0 || path[i] >= (int)pads.size()) continue;
        const auto& a = pads[path[i - 1]].location;
        const auto& b = pads[path[i]].location;
        plugin->cvarManager->log("[BoostMaster] Path segment: (" + std::to_string(a.X) + "," + std::to_string(a.Y) + ") -> (" + std::to_string(b.X) + "," + std::to_string(b.Y) + ")");
    }
}

void BoostPadHelper::DrawPathOverlay(BoostMaster* plugin, const std::vector<int>& path) {
    if (!plugin || !plugin->gameWrapper) return;
    const auto& pads = GetCachedPads(plugin);
    if (pads.empty() || path.size() < 2) return;

    // Note: ProjectWorldToScreen and DrawLine are not available in this BakkesMod version
    // Fall back to logging the path for debugging
    plugin->cvarManager->log("[BoostMaster] Drawing path overlay with " + std::to_string(path.size()) + " points");
    for (size_t i = 1; i < path.size(); ++i) {
        int idx1 = path[i - 1];
        int idx2 = path[i];
        if (idx1 < 0 || idx1 >= (int)pads.size() || idx2 < 0 || idx2 >= (int)pads.size()) continue;

        Vector v1 = pads[idx1].location;
        Vector v2 = pads[idx2].location;
        
        plugin->cvarManager->log("[BoostMaster] Path overlay segment: (" + std::to_string(v1.X) + "," + std::to_string(v1.Y) + ") -> (" + std::to_string(v2.X) + "," + std::to_string(v2.Y) + ")");
    }
}

void BoostPadHelper::DrawPathOverlayCanvas(BoostMaster* plugin, CanvasWrapper canvas, const std::vector<int>& path) {
    if (!plugin || !plugin->gameWrapper) return;
    const auto& pads = GetCachedPads(plugin);
    if (pads.empty() || path.size() < 2) return;

    std::vector<Vector2> screenPoints;
    for (int idx : path) {
        if (idx < 0 || idx >= (int)pads.size()) continue;
        Vector world = pads[idx].location;
        Vector2 screen = canvas.Project(world);
        screenPoints.push_back(screen);
    }
    if (screenPoints.size() < 2) return;

    float thickness = BoostSettingsWindow::GetOverlaySize();
    for (size_t i = 1; i < screenPoints.size(); ++i) {
        canvas.DrawLine(screenPoints[i - 1], screenPoints[i], thickness);
    }
}
