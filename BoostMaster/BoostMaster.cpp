#include "pch.h"
#include "BoostMaster.h"
#include "BoostMasterUI.h"
#include "BoostPadHelper.h"
#include "BoostHUDWindow.h"
#include "BoostSettingsWindow.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "thirdparty/json.hpp"

using nlohmann::json;

extern int pathAlgo; // 0 = Dijkstra, 1 = A*

BAKKESMOD_PLUGIN(BoostMaster, "Boost usage tracker and trainer", "1.0", PLUGINTYPE_FREEPLAY)

constexpr float TICK_INTERVAL = 1.0f / 120.0f;

void BoostMaster::saveMatch() {
    cvarManager->log("[BoostMaster] saveMatch invoked");
    std::filesystem::create_directories("data");
    std::ofstream out("data/boost_history.csv", std::ios::app);
    out << totalBoostUsed << "," << (totalBoostTime / 60.0f) << "\n";
    cvarManager->log("[BoostMaster] Match data saved");
}

void BoostMaster::RegisterDrawables() {
    cvarManager->log("[BoostMaster] RegisterDrawables invoked");
    if (!gameWrapper) return;
    gameWrapper->RegisterDrawable([this](CanvasWrapper canvas) {
        if (!lastPath.empty()) BoostPadHelper::DrawPathOverlayCanvas(this, canvas, lastPath);
        });
}

void BoostMaster::ResetStats() {
    cvarManager->log("[BoostMaster] ResetStats invoked");
    totalBoostUsed = 0;
    totalBoostTime = 0;
    lastBoostAmt = -1;
    bigPads = 0;
    smallPads = 0;
    efficiencyLog.clear();
    lastPath.clear();
    cvarManager->log("[BoostMaster] Stats reset");
}

void BoostMaster::onLoad() {
    try {
        cvarManager->log("[BoostMaster] onLoad called");

        // Register CVars
        cvarManager->registerCvar("boostmaster_lowthreshold", std::to_string(cvarLowBoostThresh), "% below which boost is low", true, true, 0.0f, true, 100.0f);
        cvarManager->registerCvar("boostmaster_lowtime", std::to_string(cvarLowBoostTime), "seconds low before warning", true, true, 0.1f, true, 30.0f);
        cvarManager->registerCvar("boostmaster_maxtime", std::to_string(cvarMaxBoostTime), "seconds full before warning", true, true, 0.1f, true, 30.0f);

        // Reset stats
        cvarManager->registerNotifier("boostmaster_reset", [this](const std::vector<std::string>&) {
            cvarManager->log("[BoostMaster] boostmaster_reset invoked");
            ResetStats();
            }, "Reset stats", PERMISSION_ALL);

        // Print pad path
        cvarManager->registerNotifier("boostmaster_printpath", [this](const std::vector<std::string>&) {
            cvarManager->log("[BoostMaster] boostmaster_printpath invoked");
            PrintPadPath();
            }, "Print the current boost pad path", PERMISSION_ALL);

        // Toggle pad visualization
        cvarManager->registerNotifier("boostmaster_showpads", [this](const std::vector<std::string>&) {
            cvarManager->log("[BoostMaster] boostmaster_showpads invoked");
            BoostSettingsWindow::ToggleShowPads();
            }, "Toggle boost pad visualization", PERMISSION_ALL);

        // Help
        cvarManager->registerNotifier("boostmaster_help", [this](const std::vector<std::string>&) {
            cvarManager->log("[BoostMaster] boostmaster_help invoked");
            cvarManager->log("Available commands:");
            cvarManager->log("  boostmaster_reset");
            cvarManager->log("  boostmaster_printpath");
            cvarManager->log("  boostmaster_showpads");
            cvarManager->log("  boostmaster_config");
            }, "Show help message", PERMISSION_ALL);

        // Config
        cvarManager->registerNotifier("boostmaster_config", [this](const std::vector<std::string>& args) {
            cvarManager->log("[BoostMaster] boostmaster_config invoked");
            if (args.empty() || args[0] == "help") {
                cvarManager->log("Config: lowthreshold=" + std::to_string(cvarLowBoostThresh) +
                    ", lowtime=" + std::to_string(cvarLowBoostTime) +
                    ", maxtime=" + std::to_string(cvarMaxBoostTime));
            }
            else if (args.size() == 2) {
                float val = std::stof(args[1]);
                if (args[0] == "lowthreshold") cvarLowBoostThresh = val;
                else if (args[0] == "lowtime") cvarLowBoostTime = val;
                else if (args[0] == "maxtime") cvarMaxBoostTime = val;
                else { cvarManager->log("Unknown config option: " + args[0]); return; }
                cvarManager->log("[BoostMaster] " + args[0] + " set to " + args[1]);
            }
            else {
                cvarManager->log("Usage: boostmaster_config <option> <value>");
            }
            }, "Show or set config", PERMISSION_ALL);

        // Hook match end
        gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded", [this](const std::string&) {
            cvarManager->log("[BoostMaster] EventMatchEnded hook triggered");
            saveMatch();
            });

        // UI windows
        hudWindow = std::make_shared<BoostHUDWindow>(this);
        settingsWindow = std::make_shared<BoostSettingsWindow>(this);
        RegisterDrawables();

        // Training drills
        LoadAllTrainingDrills();
        cvarManager->registerNotifier("boostmaster_savetraining", [this](const std::vector<std::string>& args) {
            if (!args.empty()) {
                cvarManager->log("[BoostMaster] Saving training drill: " + args[0]);
                SaveTrainingDrill(args[0]);
            }
            }, "Save training drill", PERMISSION_ALL);
        cvarManager->registerNotifier("boostmaster_loadtraining", [this](const std::vector<std::string>& args) {
            if (!args.empty()) {
                cvarManager->log("[BoostMaster] Loading training drill: " + args[0]);
                LoadTrainingDrill(args[0]);
            }
            }, "Load training drill", PERMISSION_ALL);
        cvarManager->registerNotifier("boostmaster_listtraining", [this](const std::vector<std::string>&) {
            cvarManager->log("[BoostMaster] Listing training drills");
            ListTrainingDrills();
            }, "List training drills", PERMISSION_ALL);
        cvarManager->registerNotifier("boostmaster_deltraining", [this](const std::vector<std::string>& args) {
            if (!args.empty()) {
                cvarManager->log("[BoostMaster] Deleting training drill: " + args[0]);
                DeleteTrainingDrill(args[0]);
            }
            }, "Delete training drill", PERMISSION_ALL);

        // History
        loadHistory();
    }
    catch (const std::exception& ex) {
        cvarManager->log("[BoostMaster] Error in onLoad: " + std::string(ex.what()));
    }
}
