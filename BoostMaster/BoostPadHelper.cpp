#include "pch.h"
#include "BoostPadHelper.h"
#include "BoostPadData.h"
#include "BoostMaster.h"
#include "BoostSettingsWindow.h"
#include <vector>
#include <optional>

namespace {
    // Cache last map name and its boost pads to avoid reloading every call
    std::string cachedMapName;
    const std::vector<StaticBoostPad>* cachedPads = nullptr;
}

const std::vector<StaticBoostPad>& BoostPadHelper::GetCachedPads(BoostMaster* plugin) {
    if (!plugin || !plugin->gameWrapper) {
        static const std::vector<StaticBoostPad> emptyPads;
        return emptyPads;
    }

    std::string currentMap = plugin->gameWrapper->GetCurrentMap();
    plugin->cvarManager->log("[BoostMaster] Current map: " + currentMap);

    if (!cachedPads || cachedMapName != currentMap) {
        cachedMapName = currentMap;
        cachedPads = &GetStaticBoostPadsForMap(currentMap);
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

        // TODO: Replace with actual 3D drawing code if available
        plugin->cvarManager->log(
            "[BoostMaster] Pad at (" +
            std::to_string(pad.location.X) + ", " +
            std::to_string(pad.location.Y) + ", " +
            std::to_string(pad.location.Z) + ") Type: " +
            (pad.type == PadType::Big ? "Big" : "Small")
        );
    }
}

void BoostPadHelper::DrawPath(BoostMaster* plugin, const std::vector<int>& path) {
    if (!plugin || !plugin->gameWrapper) return;

    const auto& pads = GetCachedPads(plugin);
    if (pads.empty() || path.size() < 2) return;

    for (size_t i = 1; i < path.size(); ++i) {
        int idxA = path[i - 1];
        int idxB = path[i];

        if (idxA < 0 || idxA >= static_cast<int>(pads.size()) ||
            idxB < 0 || idxB >= static_cast<int>(pads.size()))
            continue;

        const Vector& a = pads[idxA].location;
        const Vector& b = pads[idxB].location;

        // TODO: Replace with actual line drawing, fallback logs path segment
        plugin->cvarManager->log(
            "[BoostMaster] Path segment: (" +
            std::to_string(a.X) + "," + std::to_string(a.Y) + ") -> (" +
            std::to_string(b.X) + "," + std::to_string(b.Y) + ")"
        );
    }
}

void BoostPadHelper::DrawPathOverlayCanvas(BoostMaster* plugin, CanvasWrapper canvas, const std::vector<int>& path) {
    if (!plugin || !plugin->gameWrapper) return;

    const auto& pads = GetCachedPads(plugin);
    if (pads.empty() || path.size() < 2) return;

    std::vector<Vector2> screenPoints;
    for (int idx : path) {
        if (idx < 0 || idx >= static_cast<int>(pads.size())) continue;

        Vector worldPos = pads[idx].location;
        Vector2 screenPos = canvas.Project(worldPos);
        screenPoints.push_back(screenPos);
    }

    if (screenPoints.size() < 2) return;

    float thickness = BoostSettingsWindow::GetOverlaySize();
    for (size_t i = 1; i < screenPoints.size(); ++i) {
        Vector2 a = screenPoints[i - 1];
        Vector2 b = screenPoints[i];
        canvas.DrawLine(a, b, thickness);
    }
}
