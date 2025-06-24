// IMPORTANT: pch.h must be the very first include.
#include "pch.h"
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

// Define _globalCvarManager to satisfy linker references.
// (If your environment supplies it via a library, you might remove this definition.)
std::shared_ptr<CVarManagerWrapper> _globalCvarManager = nullptr;

// Internal constants.
namespace {
    constexpr float BOOST_LOG_INTERVAL = 10.0f; // Seconds between efficiency logs.
    constexpr float NO_BOOST_THRESHOLD = 0.01f; // Threshold for "no boost".
    constexpr float FULL_BOOST = 100.0f;  // Full boost (%).
    constexpr size_t MAX_LOG_SIZE = 100;    // Maximum log entries.
}

// ---------------------------------------------------------
// onLoad:
//  - Registers the main HUD draw callback.
//  - Registers event hooks (match end, boost pad pickup).
//  - Registers configuration variables (ConVars) for easy adjustment.
//  - Loads previous efficiency stats.
// ---------------------------------------------------------
void BoostMaster::onLoad() {
    // Register our HUD drawable. (See wiki for proper usage.)
    this->gameWrapper->RegisterDrawable(std::bind(&BoostMaster::RenderImGui, this, std::placeholders::_1));

    // Hook the event for match ending to save our match summary.
    this->gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded",
        [this](std::string) { this->SaveMatchSummary(); });

    // Hook boost pad pickup events.
    // (Using the event name recommended by recent examples.)
    this->gameWrapper->HookEventWithCaller<CarWrapper>(
        "Function TAGame.Car_TA.EventBoostPadPickup",
        [this](CarWrapper car, void* params, std::string eventName) {
            this->onBoostPadPickup(car, params, eventName);
        });

    // Example: Register ConVars to configure thresholds (per BakkesPlugins Wiki guidelines).
    if (_globalCvarManager) {
        _globalCvarManager->registerCvar("boostmaster_lowboostthreshold",
            std::to_string(cvarLowBoostThreshold),
            "Percentage threshold below which boost is considered low.",
            true, true, 0, true, 100.0f);
        // Similarly, you can register additional cvars.
    }

    // Load previously saved efficiency history.
    this->LoadProgressStats();
}

// ---------------------------------------------------------
// onUnload:
//  - Cleanup if necessary (nothing specific for this plugin).
// ---------------------------------------------------------
void BoostMaster::onUnload() {
    // No cleanup required in this example.
}

// ---------------------------------------------------------
// RenderImGui:
//  - Renders the plugin HUD, including boost status and training tips.
//  - Performs defensive checks to ensure the ImGui context is valid.
//  - Also updates boost tracking calculations.
// ---------------------------------------------------------
void BoostMaster::RenderImGui(CanvasWrapper canvas) {
    // Defensive check: ensure ImGui is available.
    if (ImGui::GetCurrentContext() == nullptr)
        return;

    // Render only if in-game.
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
            // Use plain text for training tips.
            ImGui::Text("Training Tips:");
            if (timeAtLowBoost > cvarLowBoostThreshold)
                ImGui::TextColored(ImVec4(1, 0.6f, 0, 1), "WARNING: Low boost for too long - rotate!");
            if (timeAtMaxBoost > cvarMaxBoostTime)
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "WARNING: You're hoarding boost - use it!");
            ImGui::Separator();
            {
                std::lock_guard<std::mutex> lock(logMutex);
                if (!efficiencyLog.empty())
                    ImGui::PlotLines("Efficiency",
                        efficiencyLog.data(),
                        static_cast<int>(efficiencyLog.size()),
                        0, nullptr,
                        0.0f, 3.0f, ImVec2(300, 80),
                        sizeof(float));
                if (!previousEfficiencies.empty())
                    ImGui::PlotLines("Match History",
                        previousEfficiencies.data(),
                        static_cast<int>(previousEfficiencies.size()),
                        0, nullptr,
                        0.0f, 3.0f, ImVec2(300, 80),
                        sizeof(float));
            }
        }
        ImGui::End();
    }

    // ---------------------------------------------------------
    // Boost tracking update logic (executed outside ImGui block).
    // ---------------------------------------------------------
    if (!car.IsNull()) {
        float boostRaw = car.GetBoostComponent().GetCurrentBoostAmount();

        // Update total boost used when boost decreases.
        if (lastBoostAmount >= 0.0f && boostRaw < lastBoostAmount)
            totalBoostUsed += (lastBoostAmount - boostRaw);

        // Determine if boosting is active.
        bool isBoosting = (prevBoost >= 0.0f && boostRaw < prevBoost);
        if (isBoosting)
            totalBoostTime += tickInterval;

        // Accumulate time at zero boost.
        if (boostRaw <= NO_BOOST_THRESHOLD)
            timeWithNoBoost += tickInterval;

        // Accumulate time at low boost.
        if (boostRaw < cvarLowBoostThreshold)
            timeAtLowBoost += tickInterval;
        else
            timeAtLowBoost = 0.0f;

        // Accumulate time at max boost (if not boosting).
        if (boostRaw >= FULL_BOOST && !isBoosting)
            timeAtMaxBoost += tickInterval;
        else
            timeAtMaxBoost = 0.0f;

        lastBoostAmount = boostRaw;
        prevBoost = boostRaw;
        prevBoosting = isBoosting;
    }

    // Log efficiency statistics at regular intervals.
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
//  Event handler to update pad counters when a boost pad is picked up.
// ---------------------------------------------------------
void BoostMaster::onBoostPadPickup(CarWrapper car, void* params, std::string eventName) {
    if (car.IsNull())
        return;

    float currentBoost = car.GetBoostComponent().GetCurrentBoostAmount();
    float boostGain = currentBoost - lastBoostAmount;
    constexpr float BOOST_GAIN_THRESHOLD = 0.3f; // Typically, a big pad gives ~0.33 boost.
    if (currentBoost >= FULL_BOOST * 0.98f || boostGain >= BOOST_GAIN_THRESHOLD)
        bigPads++;
    else
        smallPads++;
}

// ---------------------------------------------------------
// SaveMatchSummary:
//  Saves match stats to a CSV file ("data/boost_history.csv").
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
                std::getline(ss, effStr, ','))
            {
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
