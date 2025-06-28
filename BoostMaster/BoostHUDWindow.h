#pragma once
#include "bakkesmod/plugin/PluginWindow.h"
#include "BoostMaster.h"
#include <string>

class BoostHUDWindow : public BakkesMod::Plugin::PluginWindow {
public:
    explicit BoostHUDWindow(BoostMaster* plugin) { this->plugin = plugin; }
    void Render();
    std::string GetMenuName() override { return "boosthudwindow"; }
    std::string GetMenuTitle() override { return "Boost HUD"; }
    void SetImGuiContext(uintptr_t) override {}
    bool ShouldBlockInput() override { return false; }
    bool IsActiveOverlay() override { return false; }
    void OnOpen() override {}
    void OnClose() override {}
private:
    BoostMaster* plugin;
};
