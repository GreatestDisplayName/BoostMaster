#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/wrappers/includes.h"
#include "imgui/imgui.h"

#include <vector>
#include <string>
#include <filesystem>
#include <mutex>
#include "logging.h"

/*
 * BoostMaster
 *
 * A BakkesMod plugin to track and display boost usage statistics.
 *
 * This version adds configurable ConVars—including one to set the boost pad
 * event name—and a configuration window that can be toggled with F2.
 */
class BoostMaster : public BakkesMod::Plugin::BakkesModPlugin
{
public:
    // Plugin lifecycle methods.
    void onLoad() override;
    void onUnload() override;

    // ImGui rendering and event handling.
    void RenderImGui(CanvasWrapper canvas);
    void RenderConfigWindow();  // New: Renders the config window when toggled.
    void SaveMatchSummary();
    void LoadProgressStats();
    void onBoostPadPickup(CarWrapper car, void* params, std::string eventName);

    // --- Tracking variables ---
    float totalBoostUsed = 0.0f;
    float totalBoostTime = 0.0f;
    float timeWithNoBoost = 0.0f;
    float lastBoostAmount = -1.0f;
    float timeAtLowBoost = 0.0f;
    float timeAtMaxBoost = 0.0f;
    float tickInterval = 1.0f / 120.0f;  // Assumed tick interval (120 Hz)
    float timeAccumulator = 0.0f;
    float prevBoost = -1.0f;
    bool prevBoosting = false;

    // Boost pad stats.
    int bigPads = 0;
    int smallPads = 0;

    // HUD display and efficiency logs.
    bool showBoostStats = true;
    std::vector<float> efficiencyLog;
    std::vector<float> previousEfficiencies;

    // Mutex for thread-safe logging.
    std::mutex logMutex;

    // --- ConVars (configuration variables) ---
    // These can be changed via a boostmaster.cfg file or directly from the config window.
    float cvarLowBoostThreshold = 20.0f; // Boost percentage threshold for low boost (default 20%)
    float cvarMaxBoostTime = 5.0f;        // Seconds at full boost before warning (default 5 sec)
    float cvarLowBoostTime = 3.0f;        // Seconds at low boost before warning (default 3 sec)

    // The boost pad event ConVar.
    // For many modern Rocket League builds, "Function TAGame.Car_TA.EventBoostPadPickup" is preferred.
    std::string boostPadEventName = "Function TAGame.Car_TA.EventBoostPadPickup";

    // --- New: Toggle for configuration window via F2 ---
    bool showConfigWindow = false;
};
