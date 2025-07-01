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

int pathAlgo = 0; // 0 = Dijkstra, 1 = A*

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

        // Initialize global cvar manager for logging
        _globalCvarManager = cvarManager;

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
        this->loadHistory();
    }
    catch (const std::exception& ex) {
        cvarManager->log("[BoostMaster] Error in onLoad: " + std::string(ex.what()));
    }
}

void BoostMaster::onUnload() {
    UnregisterDrawables();
    cvarManager->log("BoostMaster: Unloaded");
}

void BoostMaster::SaveTrainingDrill(const std::string& name) {
    cvarManager->log("[BoostMaster] SaveTrainingDrill invoked for: " + name);
    if (!gameWrapper || !gameWrapper->IsInFreeplay()) {
        cvarManager->log("[BoostMaster] Must be in freeplay to save drill");
        return;
    }

    auto car = gameWrapper->GetLocalCar();
    auto server = gameWrapper->GetGameEventAsServer();
    if (car.IsNull() || server.IsNull()) {
        cvarManager->log("[BoostMaster] Car or server is null");
        return;
    }
    
    auto ball = server.GetBall();
    if (ball.IsNull()) {
        cvarManager->log("[BoostMaster] Ball is null");
        return;
    }

    TrainingDrill drill;
    drill.name = name;
    
    auto carLoc = car.GetLocation();
    drill.carX = carLoc.X;
    drill.carY = carLoc.Y;
    drill.carZ = carLoc.Z;
    
    auto carRot = car.GetRotation();
    drill.carPitch = carRot.Pitch;
    drill.carYaw = carRot.Yaw;
    drill.carRoll = carRot.Roll;
    
    auto ballLoc = ball.GetLocation();
    drill.ballX = ballLoc.X;
    drill.ballY = ballLoc.Y;
    drill.ballZ = ballLoc.Z;
    
    auto ballVel = ball.GetVelocity();
    drill.ballVelX = ballVel.X;
    drill.ballVelY = ballVel.Y;
    drill.ballVelZ = ballVel.Z;
    
    drills[name] = drill;
    SaveAllTrainingDrills();
    cvarManager->log("[BoostMaster] Training drill '" + name + "' saved");
}

void BoostMaster::LoadTrainingDrill(const std::string& name) {
    cvarManager->log("[BoostMaster] LoadTrainingDrill invoked for: " + name);
    if (!gameWrapper || !gameWrapper->IsInFreeplay()) {
        cvarManager->log("[BoostMaster] Must be in freeplay to load drill");
        return;
    }

    auto it = drills.find(name);
    if (it == drills.end()) {
        cvarManager->log("[BoostMaster] Training drill '" + name + "' not found");
        return;
    }

    const auto& drill = it->second;
    auto car = gameWrapper->GetLocalCar();
    auto server = gameWrapper->GetGameEventAsServer();
    if (car.IsNull() || server.IsNull()) {
        cvarManager->log("[BoostMaster] Car or server is null");
        return;
    }
    
    auto ball = server.GetBall();
    if (ball.IsNull()) {
        cvarManager->log("[BoostMaster] Ball is null");
        return;
    }

    // Set car position and rotation
    car.SetLocation(Vector{ drill.carX, drill.carY, drill.carZ });
    car.SetRotation(Rotator{ static_cast<int>(drill.carPitch), static_cast<int>(drill.carYaw), static_cast<int>(drill.carRoll) });
    car.SetVelocity(Vector{ 0, 0, 0 });
    car.SetAngularVelocity(Vector{ 0, 0, 0 }, Vector{ 0, 0, 1 });
    
    // Set ball position and velocity
    ball.SetLocation(Vector{ drill.ballX, drill.ballY, drill.ballZ });
    ball.SetVelocity(Vector{ drill.ballVelX, drill.ballVelY, drill.ballVelZ });
    
    cvarManager->log("[BoostMaster] Training drill '" + name + "' loaded");
}

void BoostMaster::ListTrainingDrills() {
    cvarManager->log("[BoostMaster] ListTrainingDrills invoked");
    if (drills.empty()) {
        cvarManager->log("[BoostMaster] No training drills saved");
        return;
    }
    
    cvarManager->log("[BoostMaster] Available training drills:");
    for (const auto& pair : drills) {
        cvarManager->log("  - " + pair.first);
    }
}

void BoostMaster::DeleteTrainingDrill(const std::string& name) {
    cvarManager->log("[BoostMaster] DeleteTrainingDrill invoked for: " + name);
    auto it = drills.find(name);
    if (it == drills.end()) {
        cvarManager->log("[BoostMaster] Training drill '" + name + "' not found");
        return;
    }
    
    drills.erase(it);
    SaveAllTrainingDrills();
    cvarManager->log("[BoostMaster] Training drill '" + name + "' deleted");
}

void BoostMaster::LoadAllTrainingDrills() {
    cvarManager->log("[BoostMaster] LoadAllTrainingDrills invoked");
    try {
        std::filesystem::create_directories("data");
        std::ifstream file("data/training_drills.json");
        if (!file.is_open()) {
            cvarManager->log("[BoostMaster] No training drills file found, starting fresh");
            return;
        }
        
        json j;
        file >> j;
        file.close();
        
        drills.clear();
        for (const auto& item : j["drills"]) {
            TrainingDrill drill;
            drill.name = item["name"];
            drill.carX = item["carX"];
            drill.carY = item["carY"];
            drill.carZ = item["carZ"];
            drill.carPitch = item["carPitch"];
            drill.carYaw = item["carYaw"];
            drill.carRoll = item["carRoll"];
            drill.ballX = item["ballX"];
            drill.ballY = item["ballY"];
            drill.ballZ = item["ballZ"];
            drill.ballVelX = item["ballVelX"];
            drill.ballVelY = item["ballVelY"];
            drill.ballVelZ = item["ballVelZ"];
            drills[drill.name] = drill;
        }
        
        cvarManager->log("[BoostMaster] Loaded " + std::to_string(drills.size()) + " training drills");
    }
    catch (const std::exception& ex) {
        cvarManager->log("[BoostMaster] Error loading training drills: " + std::string(ex.what()));
    }
}

void BoostMaster::SaveAllTrainingDrills() {
    cvarManager->log("[BoostMaster] SaveAllTrainingDrills invoked");
    try {
        std::filesystem::create_directories("data");
        json j;
        j["drills"] = json::array();
        
        for (const auto& pair : drills) {
            const auto& drill = pair.second;
            json drillJson;
            drillJson["name"] = drill.name;
            drillJson["carX"] = drill.carX;
            drillJson["carY"] = drill.carY;
            drillJson["carZ"] = drill.carZ;
            drillJson["carPitch"] = drill.carPitch;
            drillJson["carYaw"] = drill.carYaw;
            drillJson["carRoll"] = drill.carRoll;
            drillJson["ballX"] = drill.ballX;
            drillJson["ballY"] = drill.ballY;
            drillJson["ballZ"] = drill.ballZ;
            drillJson["ballVelX"] = drill.ballVelX;
            drillJson["ballVelY"] = drill.ballVelY;
            drillJson["ballVelZ"] = drill.ballVelZ;
            j["drills"].push_back(drillJson);
        }
        
        std::ofstream file("data/training_drills.json");
        file << j.dump(2);
        file.close();
        
        cvarManager->log("[BoostMaster] Saved " + std::to_string(drills.size()) + " training drills");
    }
    catch (const std::exception& ex) {
        cvarManager->log("[BoostMaster] Error saving training drills: " + std::string(ex.what()));
    }
}

void BoostMaster::loadHistory() {
    this->cvarManager->log("[BoostMaster] loadHistory invoked");
    try {
        std::filesystem::create_directories("data");
        std::ifstream file("data/boost_history.csv");
        if (!file.is_open()) {
            this->cvarManager->log("[BoostMaster] No boost history file found, starting fresh");
            return;
        }
        
        this->historyLog.clear();
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            
            std::istringstream iss(line);
            std::string boostUsed, playTime;
            
            if (std::getline(iss, boostUsed, ',') && std::getline(iss, playTime)) {
                try {
                    float efficiency = std::stof(boostUsed) / std::max(0.1f, std::stof(playTime));
                    this->historyLog.push_back(efficiency);
                }
                catch (...) {
                    // Skip invalid lines
                }
            }
        }
        file.close();
        
        this->cvarManager->log("[BoostMaster] Loaded " + std::to_string(this->historyLog.size()) + " history entries");
    }
    catch (const std::exception& ex) {
        this->cvarManager->log("[BoostMaster] Error loading history: " + std::string(ex.what()));
    }
}
