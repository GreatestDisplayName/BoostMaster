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
    Logger::Log(LogLevel::INFO, "Rendering", "RegisterDrawables invoked");
    if (!gameWrapper) return;
    gameWrapper->RegisterDrawable([this](CanvasWrapper canvas) {
        // Draw path overlay
        if (!lastPath.empty())
            BoostPadHelper::DrawPathOverlayCanvas(this, canvas, lastPath);
        
        // Draw advanced overlay
        RenderAdvancedOverlay(canvas);
        });
}

void BoostMaster::UnregisterDrawables() {
    cvarManager->log("[BoostMaster] UnregisterDrawables invoked");
    if (!gameWrapper) return;
    gameWrapper->UnregisterDrawables();
}

void BoostMaster::onLoad() {
    try {
        Logger::Log(LogLevel::INFO, "Core", "BoostMaster onLoad called");

        // Initialize global cvar manager for logging
        _globalCvarManager = cvarManager;
        
        // Initialize advanced systems
        InitializeAdvancedSystems();

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

        // Advanced analytics commands
        cvarManager->registerNotifier("boostmaster_report", [this](const std::vector<std::string>&) {
            GenerateSessionReport();
            }, "Generate session performance report", PERMISSION_ALL);
            
        cvarManager->registerNotifier("boostmaster_playstyle", [this](const std::vector<std::string>&) {
            AnalyzePlaystyle();
            }, "Analyze current playstyle", PERMISSION_ALL);
            
        cvarManager->registerNotifier("boostmaster_clearheatmap", [this](const std::vector<std::string>&) {
            if (heatmapGenerator) {
                heatmapGenerator->ClearData();
                Logger::Log(LogLevel::INFO, "Commands", "Heatmap data cleared");
            }
            }, "Clear heatmap data", PERMISSION_ALL);
            
        cvarManager->registerNotifier("boostmaster_exportheatmap", [this](const std::vector<std::string>& args) {
            if (heatmapGenerator) {
                std::string filename = args.empty() ? "session_heatmap" : args[0];
                heatmapGenerator->ExportHeatmap(filename);
            }
            }, "Export heatmap data", PERMISSION_ALL);
            
        cvarManager->registerNotifier("boostmaster_performance", [this](const std::vector<std::string>&) {
            PerformanceProfiler::PrintReport();
            }, "Show performance profiling report", PERMISSION_ALL);

        // Help command
        cvarManager->registerNotifier("boostmaster_help", [this](const std::vector<std::string>&) {
            Logger::Log(LogLevel::INFO, "Help", "Available commands:");
            Logger::Log(LogLevel::INFO, "Help", "  boostmaster_reset - Reset all statistics");
            Logger::Log(LogLevel::INFO, "Help", "  boostmaster_printpath - Show boost pad path");
            Logger::Log(LogLevel::INFO, "Help", "  boostmaster_showpads - Toggle boost pad visualization");
            Logger::Log(LogLevel::INFO, "Help", "  boostmaster_report - Generate performance report");
            Logger::Log(LogLevel::INFO, "Help", "  boostmaster_playstyle - Analyze playstyle");
            Logger::Log(LogLevel::INFO, "Help", "  boostmaster_exportheatmap <name> - Export heatmap");
            Logger::Log(LogLevel::INFO, "Help", "  boostmaster_performance - Show performance stats");
            Logger::Log(LogLevel::INFO, "Help", "  boostmaster_config <option> <value> - Configure settings");
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

        // Register advanced event hooks
        RegisterAdvancedHooks();

        // UI
        hudWindow = std::make_shared<BoostHUDWindow>(this);
        settingsWindow = std::make_shared<BoostSettingsWindow>(this);

        RegisterDrawables();
        LoadAllTrainingDrills();
        loadHistory();
        
        // Setup performance metrics update timer
        gameWrapper->SetTimeout([this](GameWrapper* gw) {
            UpdatePerformanceMetrics();
            CheckCoachingTriggers();
            if (notificationManager) {
                notificationManager->Update(0.1f);
            }
            return false;
        }, 0.1f);
    }
    catch (const std::exception& ex) {
        cvarManager->log("[BoostMaster] Error in onLoad: " + std::string(ex.what()));
    }
}

void BoostMaster::onUnload() {
    CleanupAdvancedSystems();
    UnregisterDrawables();
    Logger::Log(LogLevel::INFO, "Core", "BoostMaster unloaded");
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
    car.SetRotation(Rotator{ drill.carPitch, drill.carYaw, drill.carRoll });
    car.SetVelocity(Vector{ 0, 0, 0 });
    car.SetAngularVelocity(Vector{ 0, 0, 0 }, false);
    
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
    cvarManager->log("[BoostMaster] loadHistory invoked");
    try {
        std::filesystem::create_directories("data");
        std::ifstream file("data/boost_history.csv");
        if (!file.is_open()) {
            cvarManager->log("[BoostMaster] No boost history file found, starting fresh");
            return;
        }
        
        historyLog.clear();
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            
            std::istringstream iss(line);
            std::string boostUsed, playTime;
            
            if (std::getline(iss, boostUsed, ',') && std::getline(iss, playTime)) {
                try {
                    float efficiency = std::stof(boostUsed) / std::max(0.1f, std::stof(playTime));
                    historyLog.push_back(efficiency);
                }
                catch (...) {
                    // Skip invalid lines
                }
            }
        }
        file.close();
        
        cvarManager->log("[BoostMaster] Loaded " + std::to_string(historyLog.size()) + " history entries");
    }
    catch (const std::exception& ex) {
        cvarManager->log("[BoostMaster] Error loading history: " + std::string(ex.what()));
    }
}

// Advanced Systems Implementation
void BoostMaster::InitializeAdvancedSystems() {
    notificationManager = std::make_unique<NotificationManager>();
    heatmapGenerator = std::make_unique<HeatmapGenerator>();
    currentSession.Reset();
    lastUpdateTime = GetGameTime();
    
    Logger::Log(LogLevel::INFO, "Core", "Advanced systems initialized");
}

void BoostMaster::CleanupAdvancedSystems() {
    if (notificationManager) {
        notificationManager.reset();
    }
    if (heatmapGenerator) {
        heatmapGenerator.reset();
    }
    
    Logger::Log(LogLevel::INFO, "Core", "Advanced systems cleaned up");
}

float BoostMaster::GetGameTime() const {
    return std::chrono::duration<float>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

void BoostMaster::UpdatePerformanceMetrics() {
    PerformanceProfiler::ScopedTimer timer("UpdatePerformanceMetrics");
    
    if (!gameWrapper->IsInGame()) return;
    
    auto car = gameWrapper->GetLocalCar();
    if (car.IsNull()) return;
    
    float currentTime = GetGameTime();
    if (currentSession.sessionStartTime == 0.0f) {
        currentSession.sessionStartTime = currentTime;
    }
    
    // Track speed and position
    Vector velocity = car.GetVelocity();
    Vector position = car.GetLocation();
    float speed = velocity.magnitude();
    
    currentSession.speedHistory.push_back(speed);
    currentSession.positionHistory.push_back(position);
    
    // Record heatmap data
    if (heatmapGenerator) {
        heatmapGenerator->RecordPosition(position, 1.0f);
        
        // Track boost usage
        float boostAmount = car.GetBoostComponent().GetCurrentBoostAmount();
        if (lastBoostAmt > 0 && boostAmount < lastBoostAmt) {
            float boostUsed = lastBoostAmt - boostAmount;
            heatmapGenerator->RecordBoostUsage(position, boostUsed);
        }
        lastBoostAmt = (int)boostAmount;
    }
    
    // Calculate distance traveled
    if (currentSession.positionHistory.size() > 1) {
        Vector lastPos = currentSession.positionHistory[currentSession.positionHistory.size() - 2];
        float distance = Vector::Distance(position, lastPos);
        currentSession.totalDistance += distance;
    }
    
    // Calculate average speed
    if (!currentSession.speedHistory.empty()) {
        float sum = 0;
        for (float s : currentSession.speedHistory) sum += s;
        currentSession.averageSpeed = sum / currentSession.speedHistory.size();
    }
    
    // Limit history size
    const size_t MAX_HISTORY_SIZE = 10000;
    if (currentSession.speedHistory.size() > MAX_HISTORY_SIZE) {
        currentSession.speedHistory.erase(
            currentSession.speedHistory.begin(),
            currentSession.speedHistory.begin() + (MAX_HISTORY_SIZE / 2)
        );
        currentSession.positionHistory.erase(
            currentSession.positionHistory.begin(),
            currentSession.positionHistory.begin() + (MAX_HISTORY_SIZE / 2)
        );
    }
    
    // Invalidate cached efficiency
    cachedEfficiency.reset();
}

void BoostMaster::AnalyzePlaystyle() {
    float boostEfficiency = GetCurrentEfficiency();
    
    std::string playstyle = "Balanced";
    if (boostEfficiency > 80.0f && currentSession.averageSpeed > 1200.0f) {
        playstyle = "Aggressive";
    } else if (boostEfficiency < 40.0f && currentSession.averageSpeed < 800.0f) {
        playstyle = "Defensive";
    } else if (currentSession.averageSpeed > 1000.0f) {
        playstyle = "Fast-Paced";
    } else if (boostEfficiency > 60.0f) {
        playstyle = "Efficient";
    }
    
    if (currentSession.detectedPlaystyle != playstyle) {
        currentSession.detectedPlaystyle = playstyle;
        Logger::Log(LogLevel::INFO, "Analytics", "Detected playstyle: " + playstyle);
        
        if (notificationManager) {
            Notification notif{
                NotificationType::Custom,
                "Playstyle detected: " + playstyle,
                5.0f,
                false,
                LinearColor{0.0f, 1.0f, 1.0f, 1.0f}
            };
            notificationManager->ShowNotification(notif);
        }
    }
}

float BoostMaster::GetCurrentEfficiency() const {
    float currentTime = GetGameTime();
    if (!cachedEfficiency || (currentTime - lastEfficiencyUpdate) > 1.0f) {
        float efficiency = 0.0f;
        if (totalBoostTime > 0.0f) {
            efficiency = (totalBoostUsed / totalBoostTime) * 100.0f;
        }
        cachedEfficiency = efficiency;
        lastEfficiencyUpdate = currentTime;
    }
    return *cachedEfficiency;
}

void BoostMaster::GenerateSessionReport() {
    float sessionDuration = GetGameTime() - currentSession.sessionStartTime;
    
    Logger::Log(LogLevel::INFO, "Report", "=== Session Report ===");
    Logger::Log(LogLevel::INFO, "Report", "Duration: " + std::to_string(sessionDuration) + " seconds");
    Logger::Log(LogLevel::INFO, "Report", "Distance: " + std::to_string(currentSession.totalDistance) + " units");
    Logger::Log(LogLevel::INFO, "Report", "Average Speed: " + std::to_string(currentSession.averageSpeed) + " units/s");
    Logger::Log(LogLevel::INFO, "Report", "Ball Touches: " + std::to_string(currentSession.ballTouches));
    Logger::Log(LogLevel::INFO, "Report", "Boost Efficiency: " + std::to_string(GetCurrentEfficiency()) + "%");
    Logger::Log(LogLevel::INFO, "Report", "Playstyle: " + currentSession.detectedPlaystyle);
    Logger::Log(LogLevel::INFO, "Report", "=====================");
}

void BoostMaster::RegisterAdvancedHooks() {
    // Ball touch events
    gameWrapper->HookEvent("Function TAGame.Ball_TA.OnHitGoal", 
        [this](const std::string& eventName) {
            OnGoalScored();
        });
    
    // Car collision events
    gameWrapper->HookEvent("Function TAGame.Car_TA.OnHitBall", 
        [this](const std::string& eventName) {
            OnBallHit();
        });
    
    // Boost pickup events
    gameWrapper->HookEvent("Function TAGame.VehiclePickup_Boost_TA.Pickup", 
        [this](const std::string& eventName) {
            OnBoostPickup();
        });
    
    // Demo events
    gameWrapper->HookEvent("Function TAGame.Car_TA.Demolish", 
        [this](const std::string& eventName) {
            OnCarDemolished();
        });
    
    // Input events for boost tracking
    gameWrapper->HookEventWithCaller<CarWrapper>("Function TAGame.Car_TA.SetVehicleInput",
        [this](CarWrapper caller, void* params, const std::string& eventName) {
            if (caller.IsNull()) return;
            OnBoostInput(caller);
        });
    
    Logger::Log(LogLevel::INFO, "Events", "Advanced event hooks registered");
}

void BoostMaster::OnGoalScored() {
    Logger::Log(LogLevel::INFO, "Events", "Goal scored!");
    
    if (notificationManager) {
        Notification notif{
            NotificationType::GoalScored,
            "GOAL! Great shot!",
            3.0f,
            true,
            LinearColor{0.0f, 1.0f, 0.0f, 1.0f}
        };
        notificationManager->ShowNotification(notif);
    }
}

void BoostMaster::OnBallHit() {
    currentSession.ballTouches++;
    
    if (notificationManager && currentSession.ballTouches % 50 == 0) {
        Notification notif{
            NotificationType::BallHit,
            "Ball touches: " + std::to_string(currentSession.ballTouches),
            2.0f,
            false,
            LinearColor{1.0f, 1.0f, 0.0f, 1.0f}
        };
        notificationManager->ShowNotification(notif);
    }
}

void BoostMaster::OnBoostPickup() {
    // This will be called when player picks up boost pads
    Logger::Log(LogLevel::DEBUG, "Events", "Boost pickup detected");
}

void BoostMaster::OnCarDemolished() {
    currentSession.totalDemos++;
    Logger::Log(LogLevel::INFO, "Events", "Car demolished! Total: " + std::to_string(currentSession.totalDemos));
}

void BoostMaster::OnBoostInput(CarWrapper caller) {
    if (caller.IsNull()) return;
    
    // Track boost usage patterns
    float boostAmount = caller.GetBoostComponent().GetCurrentBoostAmount();
    if (boostAmount > 0) {
        // Player is actively boosting
    }
}

void BoostMaster::CheckCoachingTriggers() {
    if (!gameWrapper->IsInGame()) return;
    
    auto car = gameWrapper->GetLocalCar();
    if (car.IsNull() || !notificationManager) return;
    
    float boostAmount = car.GetBoostComponent().GetCurrentBoostAmount();
    Vector carPos = car.GetLocation();
    
    // Low boost coaching
    if (boostAmount < cvarLowBoostThresh) {
        const auto& pads = BoostPadHelper::GetCachedPads(this);
        float minDistance = FLT_MAX;
        
        for (const auto& pad : pads) {
            float dist = Vector::Distance(carPos, pad.location);
            if (dist < minDistance) {
                minDistance = dist;
            }
        }
        
        if (minDistance < 1000.0f && minDistance > 0.0f) {
            static float lastLowBoostNotif = 0.0f;
            float currentTime = GetGameTime();
            
            if (currentTime - lastLowBoostNotif > 5.0f) { // Don't spam notifications
                Notification notif{
                    NotificationType::LowBoost,
                    "Boost pad nearby - " + std::to_string((int)minDistance) + " units",
                    3.0f,
                    false,
                    LinearColor{1.0f, 1.0f, 0.0f, 1.0f}
                };
                notificationManager->ShowNotification(notif);
                lastLowBoostNotif = currentTime;
            }
        }
    }
    
    // High efficiency praise
    float efficiency = GetCurrentEfficiency();
    if (efficiency > 85.0f) {
        static float lastEfficiencyNotif = 0.0f;
        float currentTime = GetGameTime();
        
        if (currentTime - lastEfficiencyNotif > 30.0f) {
            Notification notif{
                NotificationType::HighEfficiency,
                "Excellent boost efficiency: " + std::to_string((int)efficiency) + "%!",
                4.0f,
                false,
                LinearColor{0.0f, 1.0f, 0.0f, 1.0f}
            };
            notificationManager->ShowNotification(notif);
            lastEfficiencyNotif = currentTime;
        }
    }
}

void BoostMaster::RenderAdvancedOverlay(CanvasWrapper canvas) {
    if (!gameWrapper->IsInGame()) return;
    
    auto car = gameWrapper->GetLocalCar();
    if (car.IsNull()) return;
    
    Vector2 screenSize = canvas.GetSize();
    
    // Draw boost efficiency gauge
    DrawBoostEfficiencyGauge(canvas, screenSize);
    
    // Draw world space indicators
    DrawWorldSpaceIndicators(canvas);
    
    // Render notifications
    if (notificationManager) {
        notificationManager->RenderNotifications(canvas);
    }
}

void BoostMaster::DrawBoostEfficiencyGauge(CanvasWrapper canvas, Vector2 screenSize) {
    float efficiency = GetCurrentEfficiency();
    
    // Gauge position (top-right corner)
    Vector2 gaugePos = {screenSize.X - 200, 50};
    Vector2 gaugeSize = {150, 20};
    
    // Background
    canvas.SetColor(LinearColor{0.2f, 0.2f, 0.2f, 0.8f});
    canvas.DrawRect(gaugePos, gaugeSize);
    
    // Efficiency bar
    float efficiencyPercent = std::min(1.0f, efficiency / 100.0f);
    Vector2 barSize = {gaugeSize.X * efficiencyPercent, gaugeSize.Y};
    
    // Color based on efficiency
    LinearColor barColor = efficiency > 75.0f ? 
        LinearColor{0.0f, 1.0f, 0.0f, 1.0f} :  // Green
        efficiency > 50.0f ?
        LinearColor{1.0f, 1.0f, 0.0f, 1.0f} :  // Yellow
        LinearColor{1.0f, 0.0f, 0.0f, 1.0f};   // Red
    
    canvas.SetColor(barColor);
    canvas.DrawRect(gaugePos, barSize);
    
    // Text overlay
    canvas.SetColor(LinearColor{1.0f, 1.0f, 1.0f, 1.0f});
    canvas.SetPosition(gaugePos + Vector2{5, 2});
    canvas.DrawString("Efficiency: " + std::to_string((int)efficiency) + "%");
}

void BoostMaster::DrawWorldSpaceIndicators(CanvasWrapper canvas) {
    const auto& pads = BoostPadHelper::GetCachedPads(this);
    
    for (const auto& pad : pads) {
        Vector2 screenPos = canvas.Project(pad.location);
        Vector2 screenSize = canvas.GetSize();
        
        if (screenPos.X >= 0 && screenPos.X <= screenSize.X && 
            screenPos.Y >= 0 && screenPos.Y <= screenSize.Y) {
            
            float radius = pad.type == PadType::Big ? 15.0f : 8.0f;
            LinearColor color = pad.type == PadType::Big ? 
                LinearColor{1.0f, 0.5f, 0.0f, 0.8f} :  // Orange for big pads
                LinearColor{0.0f, 0.5f, 1.0f, 0.8f};   // Blue for small pads
            
            canvas.SetColor(color);
            canvas.DrawCircle(screenPos, radius);
        }
    }
}

template<typename T>
std::optional<T> BoostMaster::SafeWrapperCall(std::function<T()> func, const std::string& context) {
    try {
        return func();
    }
    catch (const std::exception& e) {
        Logger::Log(LogLevel::ERROR, context, "Exception: " + std::string(e.what()));
        return std::nullopt;
    }
    catch (...) {
        Logger::Log(LogLevel::ERROR, context, "Unknown exception occurred");
        return std::nullopt;
    }
}
