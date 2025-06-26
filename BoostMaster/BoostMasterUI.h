// BoostMasterUI.h
#pragma once
#include "GuiBase.h"
#include "BoostMaster.h"

class BoostHUDWindow : public PluginWindowBase {
public:
    BoostHUDWindow(BoostMaster* p): plugin(p){}
    void RenderWindow() override;
private:
    BoostMaster* plugin;
};

class BoostSettingsWindow : public BakkesMod::Plugin::PluginSettingsWindow {
public:
    BoostSettingsWindow(BoostMaster* p): plugin(p){}
    std::string GetPluginName() override { return "BoostMaster"; }
    void RenderSettings() override;
    void SetImGuiContext(uintptr_t ctx) override {}
private:
    BoostMaster* plugin;
};
