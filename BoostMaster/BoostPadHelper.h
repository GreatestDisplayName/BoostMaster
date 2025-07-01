#pragma once

#include "BoostPadData.h"
#include "bakkesmod/wrappers/CanvasWrapper.h"
#include <vector>
#include <optional>

// Forward declaration to avoid circular dependency
class BoostMaster;

class BoostPadHelper {
public:
    // Returns cached pads for the current map
    static const std::vector<StaticBoostPad>& GetCachedPads(BoostMaster* plugin);

    // Draw all pads, optionally filtering by type
    static void DrawAllPads(BoostMaster* plugin, std::optional<PadType> filterType = std::nullopt);

    // Draw a path between pad indices
    static void DrawPath(BoostMaster* plugin, const std::vector<int>& path);

    // Draw a path overlay on the HUD (2D screen lines)
    static void DrawPathOverlay(BoostMaster* plugin, const std::vector<int>& path);

    // Draw a path overlay on the HUD (2D screen lines) using CanvasWrapper
    static void DrawPathOverlayCanvas(BoostMaster* plugin, CanvasWrapper canvas, const std::vector<int>& path);
};
