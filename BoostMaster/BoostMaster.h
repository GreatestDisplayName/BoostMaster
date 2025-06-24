#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/wrappers/includes.h"
#include "imgui/imgui.h"

#include <vector>
#include <string>
#include <filesystem>
#include <mutex>
#include "logging.h"

class BoostMaster : public BakkesMod::Plugin::BakkesModPlugin
{
public:
    // Plugin lifecycle methods.
    void onLoad() override;
    void onUnload() override;

    // ImGui rendering and event handling.
    void RenderImGui(CanvasWrapper canvas);
    // In this example, boost tracking updates occur in RenderImGui;
    // alternatively, you might implement an onTick() callback.
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
    float tickInterval = 1.0f / 120.0f;  // Assumes game ticks at 120Hz.
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
    // These are registered at runtime so you can change settings via the console.
    float cvarLowBoostThreshold = 20.0f; // (%)
    float cvarMaxBoostTime = 5.0f;        // (seconds)
    float cvarLowBoostTime = 3.0f;        // (seconds)
};
