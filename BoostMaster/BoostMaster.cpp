#include "pch.h"
#include "BoostMaster.h"
#include "BoostMasterUI.h"
#include "BoostPadHelper.h"
#include "BoostHUDWindow.h"
#include "BoostSettingsWindow.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include "thirdparty/json.hpp"

using nlohmann::json;

BAKKESMOD_PLUGIN(BoostMaster, "Boost usage tracker and trainer", "1.0", PLUGINTYPE_FREEPLAY)

constexpr float TICK_INTERVAL = 1.0f / 120.0f; // Assume 120Hz tick rate

void BoostMaster::onLoad() {
    try {
        cvarManager->registerCvar("boostmaster_lowthreshold", std::to_string(cvarLowBoostThresh), "% below which boost is low", true, true, 0.0f, true, 100.0f);
        cvarManager->registerCvar("boostmaster_lowtime", std::to_string(cvarLowBoostTime), "seconds low before warning", true, true, 0.1f, true, 30.0f);
        cvarManager->registerCvar("boostmaster_maxtime", std::to_string(cvarMaxBoostTime), "seconds full before warning", true, true, 0.1f, true, 30.0f);

        cvarManager->registerNotifier("boostmaster_reset", [this](const std::vector<std::string>&) {
            ResetStats();
            cvarManager->log("[BoostMaster] Stats reset.");
        }, "Reset stats", PERMISSION_ALL);
        cvarManager->registerNotifier("boostmaster_printpath", [this](const std::vector<std::string>&) { PrintPadPath(); }, "Prints the current boost pad path", PERMISSION_ALL);
        cvarManager->registerNotifier("boostmaster_showpads", [](const std::vector<std::string>&) {
            BoostSettingsWindow::ToggleShowPads();
        }, "Toggle boost pad visualization", PERMISSION_ALL);
        cvarManager->registerNotifier("boostmaster_help", [this](const std::vector<std::string>&) {
            cvarManager->log("BoostMaster Console Commands:");
            cvarManager->log("  boostmaster_help         - Show this help message");
            cvarManager->log("  boostmaster_reset        - Reset boost stats");
            cvarManager->log("  boostmaster_printpath    - Print and draw the current boost pad path");
            cvarManager->log("  boostmaster_showpads     - Toggle boost pad visualization");
            cvarManager->log("  boostmaster_togglebigpads   - Toggle showing only big boost pads");
            cvarManager->log("  boostmaster_togglesmallpads - Toggle showing only small boost pads");
            cvarManager->log("  boostmaster_config         - Show or set config options");
        }, "Show BoostMaster console command help", PERMISSION_ALL);

        // Add a config command for user customization
        cvarManager->registerNotifier("boostmaster_config", [this](const std::vector<std::string>& args) {
            if (args.empty() || (args.size() == 1 && args[0] == "help")) {
                cvarManager->log("[BoostMaster] Config options:");
                cvarManager->log("  boostmaster_lowthreshold = " + std::to_string(cvarLowBoostThresh) + " (% below which boost is low)");
                cvarManager->log("  boostmaster_lowtime = " + std::to_string(cvarLowBoostTime) + " (seconds low before warning)");
                cvarManager->log("  boostmaster_maxtime = " + std::to_string(cvarMaxBoostTime) + " (seconds full before warning)");
                cvarManager->log("  Usage: boostmaster_config <option> <value>");
                cvarManager->log("  Example: boostmaster_config lowthreshold 15");
                cvarManager->log("  For help: boostmaster_config help");
                return;
            } else if (args.size() == 2) {
                if (args[0] == "lowthreshold") {
                    cvarLowBoostThresh = std::stof(args[1]);
                    cvarManager->log("[BoostMaster] Set lowthreshold to " + args[1]);
                } else if (args[0] == "lowtime") {
                    cvarLowBoostTime = std::stof(args[1]);
                    cvarManager->log("[BoostMaster] Set lowtime to " + args[1]);
                } else if (args[0] == "maxtime") {
                    cvarMaxBoostTime = std::stof(args[1]);
                    cvarManager->log("[BoostMaster] Set maxtime to " + args[1]);
                } else {
                    cvarManager->log("[BoostMaster] Unknown config option: " + args[0]);
                }
            } else {
                cvarManager->log("[BoostMaster] Usage: boostmaster_config <option> <value>");
                cvarManager->log("[BoostMaster] For help: boostmaster_config help");
            }
        }, "Show or set BoostMaster config options", PERMISSION_ALL);

        gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded", [this](const std::string&) { saveMatch(); });

        hudWindow = std::make_shared<BoostHUDWindow>(this);
        settingsWindow = std::make_shared<BoostSettingsWindow>(this);
        RegisterDrawables();

        LoadAllTrainingDrills();
        cvarManager->registerNotifier("boostmaster_savetraining", [this](const std::vector<std::string>& args) {
            if (args.empty()) {
                cvarManager->log("[BoostMaster] Usage: boostmaster_savetraining <name>");
                return;
            }
            SaveTrainingDrill(args[0]);
        }, "Save current car/ball as a custom training drill", PERMISSION_ALL);
        cvarManager->registerNotifier("boostmaster_loadtraining", [this](const std::vector<std::string>& args) {
            if (args.empty()) {
                cvarManager->log("[BoostMaster] Usage: boostmaster_loadtraining <name>");
                return;
            }
            LoadTrainingDrill(args[0]);
        }, "Load a saved custom training drill", PERMISSION_ALL);
        cvarManager->registerNotifier("boostmaster_listtraining", [this](const std::vector<std::string>&) {
            ListTrainingDrills();
        }, "List all saved custom training drills", PERMISSION_ALL);
        cvarManager->registerNotifier("boostmaster_deltraining", [this](const std::vector<std::string>& args) {
            if (args.empty()) {
                cvarManager->log("[BoostMaster] Usage: boostmaster_deltraining <name>");
                return;
            }
            DeleteTrainingDrill(args[0]);
        }, "Delete a saved custom training drill", PERMISSION_ALL);

        loadHistory();
    } catch (const std::exception& ex) {
        cvarManager->log(std::string("[BoostMaster] Exception in onLoad: ") + ex.what());
    }
}

void BoostMaster::onUnload() {
    UnregisterDrawables();
    cvarManager->log("BoostMaster: Unloaded");
}

void BoostMaster::ResetStats() {
    totalBoostUsed = totalBoostTime = 0;
    lastBoostAmt = -1;
    bigPads = smallPads = 0;
    efficiencyLog.clear();
    lastPath.clear();
    cvarManager->log("[BoostMaster] Stats have been reset.");
}

void BoostMaster::PrintPadPath() {
    try {
        if (!gameWrapper || !gameWrapper->IsInFreeplay()) {
            cvarManager->log("[BoostMaster] PrintPadPath can only be used in Freeplay mode.");
            return;
        }
        auto car = gameWrapper->GetLocalCar();
        auto server = gameWrapper->GetGameEventAsServer();
        if (car.IsNull() || server.IsNull()) {
            cvarManager->log("[BoostMaster] Car or server is null in PrintPadPath.");
            return;
        }
        auto ball = server.GetBall();
        if (ball.IsNull()) {
            cvarManager->log("[BoostMaster] Ball is null in PrintPadPath.");
            return;
        }
        const auto& pads = BoostPadHelper::GetCachedPads(this);
        auto graph = BoostPadGraph::Build(pads);
        if (graph.empty()) {
            cvarManager->log("[BoostMaster] No boost pads found for this map in PrintPadPath.");
            return;
        }
        int start = BoostPadGraph::Nearest(graph, car.GetLocation());
        int goal = BoostPadGraph::Nearest(graph, ball.GetLocation());
        if (start < 0 || start >= (int)graph.size() || goal < 0 || goal >= (int)graph.size()) {
            cvarManager->log("[BoostMaster] Invalid start or goal index for pathfinding.");
            return;
        }
        // Use adaptive pathfinding: Dijkstra or A* based on settings
        bool useAStar = false;
        extern int pathAlgo; // from BoostSettingsWindow.cpp
        useAStar = (pathAlgo == 1);
        lastPath = BoostPadGraph::FindPath(graph, start, goal, useAStar);
        if (lastPath.empty()) {
            cvarManager->log("[BoostMaster] No path found between car and ball.");
            return;
        }
        std::ostringstream oss;
        oss << "[BoostMaster] Pad path: ";
        for (int idx : lastPath) {
            if (idx < 0 || idx >= (int)pads.size()) continue;
            oss << " -> (" << pads[idx].amount << ")";
        }
        cvarManager->log(oss.str());
        cvarManager->log("[BoostMaster] Path segments are logged below.");
        BoostPadHelper::DrawPath(this, lastPath);
    } catch (const std::exception& ex) {
        cvarManager->log(std::string("[BoostMaster] Exception in PrintPadPath: ") + ex.what());
    }
}

void BoostMaster::saveMatch() {
    try {
        std::filesystem::create_directories("data");
        std::ofstream out("data/boost_history.csv", std::ios::app);
        if (!out) throw std::runtime_error("Failed to open boost_history.csv for writing.");
        out << 0 << "," << totalBoostUsed << "," << avgBoostPerMinute << "\n";
        cvarManager->log("[BoostMaster] Match stats saved.");
    } catch (const std::exception& ex) {
        cvarManager->log(std::string("[BoostMaster] File write error in saveMatch: ") + ex.what());
    }
}

void BoostMaster::loadHistory() {
    try {
        std::ifstream in("data/boost_history.csv");
        if (!in) throw std::runtime_error("Failed to open boost_history.csv for reading.");
        std::string line;
        while (std::getline(in, line)) {
            std::stringstream ss(line);
            float a, b, c; char comma;
            ss >> a >> comma >> b >> comma >> c;
            historyLog.push_back(c);
        }
        cvarManager->log("[BoostMaster] History loaded.");
    } catch (const std::exception& ex) {
        cvarManager->log(std::string("[BoostMaster] File read error in loadHistory: ") + ex.what());
    }
}

void BoostMaster::SaveTrainingDrill(const std::string& name) {
    auto car = gameWrapper->GetLocalCar();
    auto server = gameWrapper->GetGameEventAsServer();
    if (car.IsNull() || server.IsNull()) {
        cvarManager->log("[BoostMaster] Can't save drill: car or server is null.");
        return;
    }
    auto ball = server.GetBall();
    if (ball.IsNull()) {
        cvarManager->log("[BoostMaster] Can't save drill: ball is null.");
        return;
    }
    auto carLoc = car.GetLocation();
    auto carRot = car.GetRotation();
    auto ballLoc = ball.GetLocation();
    auto ballVel = ball.GetVelocity();
    drills[name] = TrainingDrill(
        name,
        static_cast<float>(carLoc.X), static_cast<float>(carLoc.Y), static_cast<float>(carLoc.Z),
        static_cast<float>(carRot.Pitch), static_cast<float>(carRot.Yaw), static_cast<float>(carRot.Roll),
        static_cast<float>(ballLoc.X), static_cast<float>(ballLoc.Y), static_cast<float>(ballLoc.Z),
        static_cast<float>(ballVel.X), static_cast<float>(ballVel.Y), static_cast<float>(ballVel.Z)
    );
    SaveAllTrainingDrills();
    cvarManager->log("[BoostMaster] Saved drill: " + name);
}

void BoostMaster::LoadTrainingDrill(const std::string& name) {
    auto it = drills.find(name);
    if (it == drills.end()) {
        cvarManager->log("[BoostMaster] Drill not found: " + name);
        return;
    }
    auto car = gameWrapper->GetLocalCar();
    auto server = gameWrapper->GetGameEventAsServer();
    if (car.IsNull() || server.IsNull()) {
        cvarManager->log("[BoostMaster] Can't load drill: car or server is null.");
        return;
    }
    auto ball = server.GetBall();
    if (ball.IsNull()) {
        cvarManager->log("[BoostMaster] Can't load drill: ball is null.");
        return;
    }
    const auto& d = it->second;
    car.SetLocation(Vector(static_cast<float>(d.carX), static_cast<float>(d.carY), static_cast<float>(d.carZ)));
    car.SetRotation(Rotator(static_cast<float>(d.carPitch), static_cast<float>(d.carYaw), static_cast<float>(d.carRoll)));
    ball.SetLocation(Vector(static_cast<float>(d.ballX), static_cast<float>(d.ballY), static_cast<float>(d.ballZ)));
    ball.SetVelocity(Vector(static_cast<float>(d.ballVelX), static_cast<float>(d.ballVelY), static_cast<float>(d.ballVelZ)));
    cvarManager->log("[BoostMaster] Loaded drill: " + name);
}

void BoostMaster::DeleteTrainingDrill(const std::string& name) {
    if (drills.erase(name)) {
        SaveAllTrainingDrills();
        cvarManager->log("[BoostMaster] Deleted drill: " + name);
    } else {
        cvarManager->log("[BoostMaster] Drill not found: " + name);
    }
}

void BoostMaster::ListTrainingDrills() {
    if (drills.empty()) {
        cvarManager->log("[BoostMaster] No drills saved.");
        return;
    }
    cvarManager->log("[BoostMaster] Saved drills:");
    for (const auto& item : drills) {
        cvarManager->log("  " + item.first);
    }
}

void BoostMaster::LoadAllTrainingDrills() {
    drills.clear();
    std::ifstream in("data/training_drills.json");
    if (!in) return;
    json j;
    in >> j;
    if (j.is_object()) {
        for (auto it = j.begin(); it != j.end(); ++it) {
            std::string name = it.key();
            auto& val = it.value();
            TrainingDrill d;
            d.name = name;
            d.carX = static_cast<float>(val["carX"]);
            d.carY = static_cast<float>(val["carY"]);
            d.carZ = static_cast<float>(val["carZ"]);
            d.carPitch = static_cast<float>(val["carPitch"]);
            d.carYaw = static_cast<float>(val["carYaw"]);
            d.carRoll = static_cast<float>(val["carRoll"]);
            d.ballX = static_cast<float>(val["ballX"]);
            d.ballY = static_cast<float>(val["ballY"]);
            d.ballZ = static_cast<float>(val["ballZ"]);
            d.ballVelX = static_cast<float>(val["ballVelX"]);
            d.ballVelY = static_cast<float>(val["ballVelY"]);
            d.ballVelZ = static_cast<float>(val["ballVelZ"]);
            drills[name] = d;
        }
    }
}

void BoostMaster::SaveAllTrainingDrills() {
    std::filesystem::create_directories("data");
    json j;
    for (const auto& pair : this->drills) {
        const std::string& name = pair.first;
        const TrainingDrill& d = pair.second;
        j[name]["carX"] = d.carX;
        j[name]["carY"] = d.carY;
        j[name]["carZ"] = d.carZ;
        j[name]["carPitch"] = d.carPitch;
        j[name]["carYaw"] = d.carYaw;
        j[name]["carRoll"] = d.carRoll;
        j[name]["ballX"] = d.ballX;
        j[name]["ballY"] = d.ballY;
        j[name]["ballZ"] = d.ballZ;
        j[name]["ballVelX"] = d.ballVelX;
        j[name]["ballVelY"] = d.ballVelY;
        j[name]["ballVelZ"] = d.ballVelZ;
    }
    std::ofstream out("data/training_drills.json");
    out << j;
}

void BoostMaster::RegisterDrawables() {
    if (!gameWrapper) return;
    gameWrapper->RegisterDrawable([this](CanvasWrapper canvas) {
        if (!lastPath.empty()) {
            BoostPadHelper::DrawPathOverlayCanvas(this, canvas, lastPath);
        }
    });
}

void BoostMaster::UnregisterDrawables() {
    if (!gameWrapper) return;
    gameWrapper->UnregisterDrawables();
}
