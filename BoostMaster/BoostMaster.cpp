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

void BoostMaster::onUnload() {
    cvarManager->log("[BoostMaster] onUnload called");
    UnregisterDrawables();
    cvarManager->log("BoostMaster: Unloaded");
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
    for (int idx : lastPath) oss << " -> idx=" << idx;
    cvarManager->log(oss.str());

    BoostPadHelper::DrawPath(this, lastPath);
}

void BoostMaster::saveMatch() {
    cvarManager->log("[BoostMaster] saveMatch invoked");
    std::filesystem::create_directories("data");
    std::ofstream out("data/boost_history.csv", std::ios::app);
    out << totalBoostUsed << "," << (totalBoostTime / 60.0f) << "\n";
    cvarManager->log("[BoostMaster] Match data saved");
}

void BoostMaster::loadHistory() {
    cvarManager->log("[BoostMaster] loadHistory invoked");
    historyLog.clear();
    std::ifstream in("data/boost_history.csv");
    std::string line;
    while (std::getline(in, line)) {
        std::stringstream ss(line);
        float used, perMin; char comma;
        ss >> used >> comma >> perMin;
        historyLog.push_back(perMin);
    }
    cvarManager->log("[BoostMaster] Loaded history entries=" + std::to_string(historyLog.size()));
}

// Save a custom training drill to file
void BoostMaster::SaveTrainingDrill(const std::string& name) {
    cvarManager->log("[BoostMaster] SaveTrainingDrill invoked: " + name);
    auto car = gameWrapper->GetLocalCar();
    auto server = gameWrapper->GetGameEventAsServer();
    if (car.IsNull() || server.IsNull()) { cvarManager->log("Cannot save drill: car/server null"); return; }
    auto ball = server.GetBall(); if (ball.IsNull()) { cvarManager->log("Cannot save drill: ball null"); return; }
    TrainingDrill d;
    d.name = name;
    auto loc = car.GetLocation(); auto rot = car.GetRotation(); auto bl = ball.GetLocation(); auto vel = ball.GetVelocity();
    d.carX = loc.X; d.carY = loc.Y; d.carZ = loc.Z;
    d.carPitch = rot.Pitch; d.carYaw = rot.Yaw; d.carRoll = rot.Roll;
    d.ballX = bl.X; d.ballY = bl.Y; d.ballZ = bl.Z;
    d.ballVelX = vel.X; d.ballVelY = vel.Y; d.ballVelZ = vel.Z;
    drills[name] = d;
    SaveAllTrainingDrills();
}

// Load a saved training drill by name
void BoostMaster::LoadTrainingDrill(const std::string& name) {
    cvarManager->log("[BoostMaster] LoadTrainingDrill invoked: " + name);
    auto it = drills.find(name);
    if (it == drills.end()) { cvarManager->log("Drill not found: " + name); return; }
    auto car = gameWrapper->GetLocalCar(); auto server = gameWrapper->GetGameEventAsServer();
    if (car.IsNull() || server.IsNull()) { cvarManager->log("Cannot load drill: car/server null"); return; }
    auto ball = server.GetBall(); if (ball.IsNull()) { cvarManager->log("Cannot load drill: ball null"); return; }
    const auto& d = it->second;
    car.SetLocation(Vector(d.carX, d.carY, d.carZ));
    car.SetRotation(Rotator(d.carPitch, d.carYaw, d.carRoll));
    ball.SetLocation(Vector(d.ballX, d.ballY, d.ballZ));
    ball.SetVelocity(Vector(d.ballVelX, d.ballVelY, d.ballVelZ));
}

// List saved training drills
void BoostMaster::ListTrainingDrills() {
    cvarManager->log("[BoostMaster] ListTrainingDrills invoked");
    if (drills.empty()) { cvarManager->log("No drills saved"); return; }
    for (const auto& p : drills) cvarManager->log("  " + p.first);
}

// Delete a named training drill
void BoostMaster::DeleteTrainingDrill(const std::string& name) {
    cvarManager->log("[BoostMaster] DeleteTrainingDrill invoked: " + name);
    if (drills.erase(name)) {
        SaveAllTrainingDrills();
    }
    else {
        cvarManager->log("Drill not found: " + name);
    }
}

// Load all drills from JSON
void BoostMaster::LoadAllTrainingDrills() {
    cvarManager->log("[BoostMaster] LoadAllTrainingDrills invoked");
    drills.clear();
    std::ifstream in("data/training_drills.json"); if (!in) return;
    json j; in >> j;
    for (auto it = j.begin(); it != j.end(); ++it) {
        const std::string& name = it.key(); const json& val = it.value();
        TrainingDrill d;
        d.name = name;
        d.carX = val.value("carX", 0.0f);
        d.carY = val.value("carY", 0.0f);
        d.carZ = val.value("carZ", 0.0f);
        d.carPitch = val.value("carPitch", 0.0f);
        d.carYaw = val.value("carYaw", 0.0f);
        d.carRoll = val.value("carRoll", 0.0f);
        d.ballX = val.value("ballX", 0.0f);
        d.ballY = val.value("ballY", 0.0f);
        d.ballZ = val.value("ballZ", 0.0f);
        d.ballVelX = val.value("ballVelX", 0.0f);
        d.ballVelY = val.value("ballVelY", 0.0f);
        d.ballVelZ = val.value("ballVelZ", 0.0f);
        drills[name] = d;
    }
}

// Save all drills to JSON
void BoostMaster::SaveAllTrainingDrills() {
    cvarManager->log("[BoostMaster] SaveAllTrainingDrills invoked");
    std::filesystem::create_directories("data");
    json root;
    for (const auto& p : drills) {
        const auto& d = p.second;
        root[p.first]["carX"] = d.carX;
        root[p.first]["carY"] = d.carY;
        root[p.first]["carZ"] = d.carZ;
        root[p.first]["carPitch"] = d.carPitch;
        root[p.first]["carYaw"] = d.carYaw;
        root[p.first]["carRoll"] = d.carRoll;
        root[p.first]["ballX"] = d.ballX;
        root[p.first]["ballY"] = d.ballY;
        root[p.first]["ballZ"] = d.ballZ;
        root[p.first]["ballVelX"] = d.ballVelX;
        root[p.first]["ballVelY"] = d.ballVelY;
        root[p.first]["ballVelZ"] = d.ballVelZ;
    }
    std::ofstream out("data/training_drills.json");
    out << std::setw(4) << root;
}

// Register in-game drawable overlay
void BoostMaster::RegisterDrawables() {
    cvarManager->log("[BoostMaster] RegisterDrawables invoked");
    if (!gameWrapper) return;
    gameWrapper->RegisterDrawable([this](CanvasWrapper canvas) {
        if (!lastPath.empty()) BoostPadHelper::DrawPathOverlay(this, lastPath);
        });
}

// Unregister drawables
void BoostMaster::UnregisterDrawables() {
    cvarManager->log("[BoostMaster] UnregisterDrawables invoked");
    if (!gameWrapper) return;
    gameWrapper->UnregisterDrawables();
}
