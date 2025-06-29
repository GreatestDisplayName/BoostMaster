#include "pch.h"
#include "BoostMaster.h"
#include "BoostMasterUI.h"
#include "BoostPadHelper.h"
#include "BoostPadGraph.h"
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

void BoostMaster::PrintPadPath() {
    cvarManager->log("[BoostMaster] PrintPadPath invoked");
    if (!gameWrapper || !gameWrapper->IsInFreeplay()) return;

    auto car = gameWrapper->GetLocalCar();
    auto server = gameWrapper->GetGameEventAsServer();
    if (car.IsNull() || server.IsNull()) return;
    auto ball = server.GetBall();
    if (ball.IsNull()) return;

    const auto& pads = BoostPadHelper::GetCachedPads(this);
    auto graph = BoostPadGraph::Build(pads);
    int start = BoostPadGraph::Nearest(graph, car.GetLocation());
    int goal = BoostPadGraph::Nearest(graph, ball.GetLocation());

    cvarManager->log("Path start=" + std::to_string(start) + " goal=" + std::to_string(goal));
    lastPath = BoostPadGraph::FindPath(graph, start, goal, pathAlgo == 1);

    std::ostringstream oss;
    oss << "[BoostMaster] Path result:";
    for (int idx : lastPath) oss << " -> " << idx;
    cvarManager->log(oss.str());

    BoostPadHelper::DrawPath(this, lastPath);
}

void BoostMaster::RegisterDrawables() {
    cvarManager->log("[BoostMaster] RegisterDrawables invoked");
    if (!gameWrapper) return;
    gameWrapper->RegisterDrawable([this](CanvasWrapper canvas) {
        if (!lastPath.empty())
            BoostPadHelper::DrawPathOverlayCanvas(this, canvas, lastPath);
        });
}

void BoostMaster::UnregisterDrawables() {
    cvarManager->log("[BoostMaster] UnregisterDrawables invoked");
    if (!gameWrapper) return;
    gameWrapper->UnregisterDrawables();
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
            ResetStats();
            }, "Reset stats", PERMISSION_ALL);

        // Print pad path
        cvarManager->registerNotifier("boostmaster_printpath", [this](const std::vector<std::string>&) {
            PrintPadPath();
            }, "Print the current boost pad path", PERMISSION_ALL);

        // Toggle pad visualization
        cvarManager->registerNotifier("boostmaster_showpads", [this](const std::vector<std::string>&) {
            BoostSettingsWindow::ToggleShowPads();
            }, "Toggle boost pad visualization", PERMISSION_ALL);

        // Training drill commands
        cvarManager->registerNotifier("boostmaster_savetraining", [this](const std::vector<std::string>& args) {
            if (!args.empty()) SaveTrainingDrill(args[0]);
            }, "Save training drill", PERMISSION_ALL);
        cvarManager->registerNotifier("boostmaster_loadtraining", [this](const std::vector<std::string>& args) {
            if (!args.empty()) LoadTrainingDrill(args[0]);
            }, "Load training drill", PERMISSION_ALL);
        cvarManager->registerNotifier("boostmaster_listtraining", [this](const std::vector<std::string>&) {
            ListTrainingDrills();
            }, "List training drills", PERMISSION_ALL);
        cvarManager->registerNotifier("boostmaster_deltraining", [this](const std::vector<std::string>& args) {
            if (!args.empty()) DeleteTrainingDrill(args[0]);
            }, "Delete training drill", PERMISSION_ALL);

        // Help command
        cvarManager->registerNotifier("boostmaster_help", [this](const std::vector<std::string>&) {
            cvarManager->log("Available commands:");
            cvarManager->log("  boostmaster_reset");
            cvarManager->log("  boostmaster_printpath");
            cvarManager->log("  boostmaster_showpads");
            cvarManager->log("  boostmaster_config");
            }, "Show help message", PERMISSION_ALL);

        // Config handler
        cvarManager->registerNotifier("boostmaster_config", [this](const std::vector<std::string>& args) {
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
                else return;
                cvarManager->log("[BoostMaster] " + args[0] + " set to " + args[1]);
            }
            else {
                cvarManager->log("Usage: boostmaster_config <option> <value>");
            }
            }, "Configure thresholds", PERMISSION_ALL);

        // Match hook
        gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded", [this](const std::string&) {
            saveMatch();
            });

        // UI
        hudWindow = std::make_shared<BoostHUDWindow>(this);
        settingsWindow = std::make_shared<BoostSettingsWindow>(this);

        RegisterDrawables();
        LoadAllTrainingDrills();
        loadHistory();
    }
    catch (const std::exception& ex) {
        cvarManager->log("[BoostMaster] Error in onLoad: " + std::string(ex.what()));
    }
}

void BoostMaster::onUnload() {
    UnregisterDrawables();
    cvarManager->log("BoostMaster: Unloaded");
}
