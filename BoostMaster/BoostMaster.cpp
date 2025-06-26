// IMPORTANT: pch.h must be the very first include.
#include "pch.h"

// Fallback: If the current ImGui version does not define ImGuiKey_F2, we define it here.
#ifndef ImGuiKey_F2
#define ImGuiKey_F2 113  // Windows Virtual-Key code for F2.
#endif

#include "BoostMaster.h"
#include "bakkesmod/wrappers/includes.h"
#include "GuiBase.h"
#include "logging.h"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <mutex>
#include <string>
#include <cstring>

// Define _globalCvarManager to satisfy linker references.
// (Remove this definition if your BakkesMod environment provides it via a linked library.)
std::shared_ptr<CVarManagerWrapper> _globalCvarManager = nullptr;

namespace {
    constexpr float BOOST_LOG_INTERVAL = 10.0f; // Seconds between efficiency logs.
    constexpr float NO_BOOST_THRESHOLD = 0.01f; // Boost is considered zero.
    constexpr float FULL_BOOST = 100.0f;  // Full boost (in %).
    constexpr size_t MAX_LOG_SIZE = 100;    // Maximum log entries.
}

// ---------------------------------------------------------
// onLoad:
//  - Registers the HUD callback.
//  - Hooks events (match end, boost pad pickup).
//  - Registers ConVars for dynamic control.
//  - Loads previous efficiency stats.
// ---------------------------------------------------------
void BoostMaster::onLoad() {
    // Register our HUD drawable.
    this->gameWrapper->RegisterDrawable(std::bind(&BoostMaster::RenderImGui, this, std::placeholders::_1));

    // Hook event for match end (to save match summary).
    this->gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded",
        [this](std::string) { this->SaveMatchSummary(); });

    // Register ConVars via _globalCvarManager if available.
    if (_globalCvarManager) {
        _globalCvarManager->registerCvar("boostmaster_lowboostthreshold",
            std::to_string(cvarLowBoostThreshold),
            "Percentage threshold below which boost is considered low.",
            true, true, 0, true, 100.0f);

        _globalCvarManager->registerCvar("boostmaster_maxboosttime",
            std::to_string(cvarMaxBoostTime),
            "Time in seconds at full boost before a warning is issued.",
            true, true, 0, true, 60.0f);

        _globalCvarManager->registerCvar("boostmaster_lowboosttime",
            std::to_string(cvarLowBoostTime),
            "Time in seconds at low boost before a warning is issued.",
            true, true, 0, true, 60.0f);

        _globalCvarManager->registerCvar("boostmaster_boostpadevent",
            boostPadEventName,
            "The boost pad event name to hook (default: Function TAGame.Car_TA.EventBoostPadPickup).",
            true, false, 0, false, 0);

        // Override boostPadEventName from the cvar value (using the dot operator)
        boostPadEventName = _globalCvarManager->getCvar("boostmaster_boostpadevent").getStringValue();
    }

    // Hook the boost pad event using the (possibly overridden) event name.
    this->gameWrapper->HookEventWithCaller<CarWrapper>(
        boostPadEventName,
        [this](CarWrapper car, void* params, std::string eventName) {
            this->onBoostPadPickup(car, params, eventName);
        });

    // Load previously saved efficiency stats.
    this->LoadProgressStats();
}

// ---------------------------------------------------------
// onUnload:
//  - Cleanup on plugin unload (none needed in this example).
// ---------------------------------------------------------
void BoostMaster::onUnload() {
    // No cleanup is necessary.
}

// ---------------------------------------------------------
// RenderConfigWindow:
//  - Renders a configuration window that shows and allows modification
//    of plugin configuration variables. This window is toggled by F2.
// ---------------------------------------------------------
void BoostMaster::RenderConfigWindow() {
    if (ImGui::Begin("BoostMaster Configuration", &showConfigWindow, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputFloat("Low Boost Threshold (%)", &cvarLowBoostThreshold, 0.1f, 1.0f, "%.1f");
        ImGui::InputFloat("Low Boost Time (s)", &cvarLowBoostTime, 0.1f, 1.0f, "%.1f");
        ImGui::InputFloat("Max Boost Time (s)", &cvarMaxBoostTime, 0.1f, 1.0f, "%.1f");

        // Allow editing the boost pad event name.
        char buf[128];
        std::memset(buf, 0, sizeof(buf));
        std::strncpy(buf, boostPadEventName.c_str(), sizeof(buf) - 1);
        if (ImGui::InputText("Boost Pad Event", buf, sizeof(buf))) {
            boostPadEventName = std::string(buf);
            // Note: Changing this does not automatically re-hook the event.
        }
    }
    ImGui::End();
}

// ---------------------------------------------------------
// RenderImGui:
//  - Renders the plugin's primary HUD.
//  - Checks if F2 is pressed to toggle the configuration window.
//  - Contains boost tracking and produces CSV log updates.
// ---------------------------------------------------------
void BoostMaster::RenderImGui(CanvasWrapper canvas) {
    // Toggle configuration window with F2.
    if (ImGui::IsKeyPressed(ImGuiKey_F2))
        showConfigWindow = !showConfigWindow;
    if (showConfigWindow)
        RenderConfigWindow();

    // Defensive check: ensure ImGui context is valid.
    if (ImGui::GetCurrentContext() == nullptr)
        return;

    // Only render the HUD if in game.
    if (!this->gameWrapper->IsInGame())
        return;

    CarWrapper car = this->gameWrapper->GetLocalCar();
    if (ImGui::Begin("Boost Master HUD", &showBoostStats, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (car.IsNull()) {
            ImGui::Text("No car detected.");
            ImGui::End();
            return;
        }

        // Calculate boost percentage.
        float boostPercent = car.GetBoostComponent().GetCurrentBoostAmount() * 100.0f;
        ImGui::Checkbox("Show Boost Stats", &showBoostStats);
        if (showBoostStats) {
            ImGui::Text("Boost: %.0f%%", boostPercent);
            ImGui::Text("Time Boosting: %.1fs", totalBoostTime);
            ImGui::Text("Time at 0 Boost: %.1fs", timeWithNoBoost);

            // Display boost pad pickup stats.
            int totalPads = bigPads + smallPads;
            if (totalPads > 0) {
                float bigRatio = static_cast<float>(bigPads) / totalPads;
                float smallRatio = static_cast<float>(smallPads) / totalPads;
                ImGui::Text("Big Pads: %d (%.1f%%)", bigPads, bigRatio * 100.0f);
                ImGui::ProgressBar(bigRatio, ImVec2(200, 15), "Big");
                ImGui::Text("Small Pads: %d (%.1f%%)", smallPads, smallRatio * 100.0f);
                ImGui::ProgressBar(smallRatio, ImVec2(200, 15), "Small");
            }
            ImGui::Separator();
            // Display training tips.
            ImGui::Text("Training Tips:");
            if (timeAtLowBoost > cvarLowBoostThreshold)
                ImGui::TextColored(ImVec4(1, 0.6f, 0, 1), "WARNING: Low boost for too long - rotate!");
            if (timeAtMaxBoost > cvarMaxBoostTime)
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "WARNING: You're hoarding boost - use it!");
            ImGui::Separator();
            {
                std::lock_guard<std::mutex> lock(logMutex);
                if (!efficiencyLog.empty())
                    ImGui::PlotLines("Efficiency", efficiencyLog.data(),
                        static_cast<int>(efficiencyLog.size()),
                        0, nullptr, 0.0f, 3.0f,
                        ImVec2(300, 80), sizeof(float));
                if (!previousEfficiencies.empty())
                    ImGui::PlotLines("Match History", previousEfficiencies.data(),
                        static_cast<int>(previousEfficiencies.size()),
                        0, nullptr, 0.0f, 3.0f,
                        ImVec2(300, 80), sizeof(float));
            }
        }
        ImGui::End();
    }

    // ---------------------------------------------------------
    // Boost tracking update (executed outside the ImGui block):
    // ---------------------------------------------------------
    if (!car.IsNull()) {
        float boostRaw = car.GetBoostComponent().GetCurrentBoostAmount();

        // Update total boost used if boost decreases.
        if (lastBoostAmount >= 0.0f && boostRaw < lastBoostAmount)
            totalBoostUsed += (lastBoostAmount - boostRaw);

        // Determine if boosting is active.
        bool isBoosting = (prevBoost >= 0.0f && boostRaw < prevBoost);
        if (isBoosting)
            totalBoostTime += tickInterval;

        // Accumulate time with zero boost.
        if (boostRaw <= NO_BOOST_THRESHOLD)
            timeWithNoBoost += tickInterval;

        // Accumulate time at low boost using the configurable threshold.
        if (boostRaw < cvarLowBoostThreshold)
            timeAtLowBoost += tickInterval;
        else
            timeAtLowBoost = 0.0f;

        // Accumulate time at full boost (if not actively boosting).
        if (boostRaw >= FULL_BOOST && !isBoosting)
            timeAtMaxBoost += tickInterval;
        else
            timeAtMaxBoost = 0.0f;

        lastBoostAmount = boostRaw;
        prevBoost = boostRaw;
        prevBoosting = isBoosting;
    }

    // Log efficiency data at regular intervals.
    timeAccumulator += tickInterval;
    if (timeAccumulator >= BOOST_LOG_INTERVAL) {
        timeAccumulator = 0.0f;
        CarWrapper carLocal = this->gameWrapper->GetLocalCar();
        if (!carLocal.IsNull()) {
            int score = 0;
            PriWrapper pri = carLocal.GetPRI();
            score = pri.IsNull() ? 0 : pri.GetMatchScore();
            float efficiency = (totalBoostUsed > 0.01f) ? score / (totalBoostUsed * 100.0f) : 0.0f;
            std::lock_guard<std::mutex> lock(logMutex);
            efficiencyLog.push_back(efficiency);
            if (efficiencyLog.size() > MAX_LOG_SIZE)
                efficiencyLog.erase(efficiencyLog.begin());
        }
    }
}

// ---------------------------------------------------------
// onBoostPadPickup:
//  Event handler invoked when a boost pad is picked up.
//  Increments bigPads or smallPads based on the boost gain.
// ---------------------------------------------------------
void BoostMaster::onBoostPadPickup(CarWrapper car, void* params, std::string eventName) {
    if (car.IsNull())
        return;

    float currentBoost = car.GetBoostComponent().GetCurrentBoostAmount();
    float boostGain = currentBoost - lastBoostAmount;
    constexpr float BOOST_GAIN_THRESHOLD = 0.3f; // Typical big pad gives ~0.33 boost.
    if (currentBoost >= FULL_BOOST * 0.98f || boostGain >= BOOST_GAIN_THRESHOLD)
        bigPads++;
    else
        smallPads++;
}

// ---------------------------------------------------------
// SaveMatchSummary:
//  Writes the match score, boost used, and efficiency to a CSV file.
// ---------------------------------------------------------
void BoostMaster::SaveMatchSummary() {
    try {
        std::filesystem::create_directories("data");
        std::ofstream out("data/boost_history.csv", std::ios::app);
        if (!out.is_open())
            return;

        CarWrapper car = this->gameWrapper->GetLocalCar();
        int score = 0;
        if (!car.IsNull()) {
            PriWrapper pri = car.GetPRI();
            score = pri.IsNull() ? 0 : pri.GetMatchScore();
        }
        float efficiency = (totalBoostUsed > 0.01f) ? score / (totalBoostUsed * 100.0f) : 0.0f;
        out << score << "," << totalBoostUsed << "," << efficiency << "\n";
        out.close();
    }
    catch (const std::exception& e) {
        LOG_ERROR(std::string("SaveMatchSummary exception: ") + e.what());
    }
}

// ---------------------------------------------------------
// LoadProgressStats:
//  Reads previous efficiency data from the CSV file into memory.
// ---------------------------------------------------------
void BoostMaster::LoadProgressStats() {
    previousEfficiencies.clear();
    try {
        std::ifstream in("data/boost_history.csv");
        std::string line;
        while (std::getline(in, line)) {
            std::stringstream ss(line);
            std::string scoreStr, boostStr, effStr;
            if (std::getline(ss, scoreStr, ',') &&
                std::getline(ss, boostStr, ',') &&
                std::getline(ss, effStr, ',')) {
                previousEfficiencies.push_back(std::stof(effStr));
                if (previousEfficiencies.size() > MAX_LOG_SIZE)
                    previousEfficiencies.erase(previousEfficiencies.begin());
            }
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR(std::string("LoadProgressStats exception: ") + e.what());
    }
}

// ---------------------------------------------------------
// Plugin Registration:
//  This macro must appear AFTER all function definitions.
// ---------------------------------------------------------
BAKKESMOD_PLUGIN(BoostMaster, "Boost usage tracker and trainer", "1.0", PLUGINTYPE_FREEPLAY)
