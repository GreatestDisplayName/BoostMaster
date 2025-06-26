#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include "bakkesmod/plugin/PluginWindow.h"
#include <vector>

// Forward declare our two UI windows
class BoostHUDWindow;
class BoostSettingsWindow;

class BoostMaster : public BakkesMod::Plugin::BakkesModPlugin
{
public:
    void onLoad() override;
    void onUnload() override;

    // Expose this so windows can pull stats & logs:
    float  totalBoostUsed{ 0 }, totalBoostTime{ 0 }, timeWithNoBoost{ 0 };
    float  timeAtLowBoost{ 0 }, timeAtMaxBoost{ 0 }, lastBoostAmt{ -1 };
    float  tickInterval{ 1 / 120.f }, timeAccumulator{ 0 };
    int    bigPads{ 0 }, smallPads{ 0 };
    std::vector<float> efficiencyLog, historyLog;

    // Config vars - made public for UI access
    float cvarLowBoostThresh{ 20.0f };
    float cvarLowBoostTime{ 3.0f };
    float cvarMaxBoostTime{ 5.0f };

    // New: Average boost per minute
    float avgBoostPerMinute{ 0 };

    // UI windows
    std::shared_ptr<BoostHUDWindow>      hudWindow;
    std::shared_ptr<BoostSettingsWindow> settingsWindow;

    // Internal helpers
    void RenderFrame(); // Now a stub
    void saveMatch();
    void loadHistory();
    void ResetStats();
};
