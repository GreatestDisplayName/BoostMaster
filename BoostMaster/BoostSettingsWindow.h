#pragma once
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include "BoostPadData.h"
#include <string>
#include <optional>

extern int pathAlgo; // Expose pathAlgo for adaptive pathfinding

// Forward declaration to avoid circular dependency
class BoostMaster;

class BoostSettingsWindow : public BakkesMod::Plugin::PluginSettingsWindow {
public:
    explicit BoostSettingsWindow(BoostMaster* plugin) { this->plugin = plugin; }
    void Render();
    void RenderSettings() override { Render(); }
    std::string GetPluginName() override { return "BoostMaster"; }
    void SetImGuiContext(uintptr_t) override {}
    static bool ShouldShowPads();
    static std::optional<PadType> GetPadTypeFilter();
    static void ToggleShowPads();
    static const float* GetOverlayColor();
    static float GetOverlaySize();
private:
    BoostMaster* plugin;
};
