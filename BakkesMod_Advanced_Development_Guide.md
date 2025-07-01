# BakkesMod SDK - Advanced Development Guide

## Table of Contents
1. [Continuing BoostMaster Development](#1-continuing-boostmaster-development)
2. [Advanced BakkesMod SDK Techniques](#2-advanced-bakkesmod-sdk-techniques)
3. [Troubleshooting & Performance Optimization](#3-troubleshooting--performance-optimization)

---

## 1. Continuing BoostMaster Development

### Phase 2 Implementation Plan

#### A. Enhanced Analytics System

**Real-time Performance Tracking:**
```cpp
// Add to BoostMaster.h
struct PerformanceMetrics {
    float sessionStartTime;
    float totalDistance;
    float averageSpeed;
    int totalDemos;
    int totalSaves;
    std::vector<float> speedHistory;
    std::vector<Vector> positionHistory;
};

class BoostMaster : public BakkesMod::Plugin::BakkesModPlugin {
    // ... existing code ...
    PerformanceMetrics currentSession;
    
    void UpdatePerformanceMetrics();
    void AnalyzePlaystyle();
    void GenerateSessionReport();
};
```

**Implementation:**
```cpp
// Add to BoostMaster.cpp
void BoostMaster::UpdatePerformanceMetrics() {
    if (!gameWrapper->IsInGame()) return;
    
    auto car = gameWrapper->GetLocalCar();
    if (car.IsNull()) return;
    
    // Track speed and position
    Vector velocity = car.GetVelocity();
    float speed = velocity.magnitude();
    currentSession.speedHistory.push_back(speed);
    currentSession.positionHistory.push_back(car.GetLocation());
    
    // Calculate average speed
    if (!currentSession.speedHistory.empty()) {
        float sum = 0;
        for (float s : currentSession.speedHistory) sum += s;
        currentSession.averageSpeed = sum / currentSession.speedHistory.size();
    }
}

void BoostMaster::AnalyzePlaystyle() {
    // Analyze boost efficiency
    float boostEfficiency = totalBoostUsed / std::max(1.0f, totalBoostTime);
    
    // Determine playstyle based on metrics
    std::string playstyle = "Balanced";
    if (boostEfficiency > 80.0f && currentSession.averageSpeed > 1200.0f) {
        playstyle = "Aggressive";
    } else if (boostEfficiency < 40.0f && currentSession.averageSpeed < 800.0f) {
        playstyle = "Defensive";
    }
    
    cvarManager->log("[BoostMaster] Detected playstyle: " + playstyle);
}
```

#### B. Advanced Notifications System

```cpp
// Add to BoostMaster.h
enum class NotificationType {
    LowBoost,
    HighEfficiency,
    PositioningSuggestion,
    BoostPadTiming,
    Custom
};

struct Notification {
    NotificationType type;
    std::string message;
    float duration;
    bool soundEnabled;
    LinearColor color;
};

class NotificationManager {
public:
    void ShowNotification(const Notification& notif);
    void RegisterCustomTrigger(std::function<bool()> condition, Notification notif);
    void Update(); // Call every tick
    
private:
    std::vector<std::pair<std::function<bool()>, Notification>> customTriggers;
    std::queue<Notification> activeNotifications;
};
```

**Smart Coaching Tips:**
```cpp
void BoostMaster::CheckCoachingTriggers() {
    auto car = gameWrapper->GetLocalCar();
    if (car.IsNull()) return;
    
    float boostAmount = car.GetBoostComponent().GetCurrentBoostAmount();
    Vector carPos = car.GetLocation();
    
    // Low boost near boost pad suggestion
    if (boostAmount < cvarLowBoostThresh) {
        const auto& pads = BoostPadHelper::GetCachedPads(this);
        float minDistance = FLT_MAX;
        Vector nearestPad;
        
        for (const auto& pad : pads) {
            float dist = Vector::Distance(carPos, pad.location);
            if (dist < minDistance) {
                minDistance = dist;
                nearestPad = pad.location;
            }
        }
        
        if (minDistance < 1000.0f) { // Within reasonable distance
            Notification notif{
                NotificationType::LowBoost,
                "Boost pad nearby - " + std::to_string((int)minDistance) + " units",
                3.0f,
                true,
                LinearColor{1.0f, 1.0f, 0.0f, 1.0f}
            };
            notificationManager.ShowNotification(notif);
        }
    }
}
```

#### C. Heatmap Generation

```cpp
// Add to BoostMaster.h
class HeatmapGenerator {
public:
    void RecordPosition(Vector pos, float intensity = 1.0f);
    void GenerateBoostUsageHeatmap();
    void GeneratePositionHeatmap();
    void ExportHeatmap(const std::string& filename);
    
private:
    struct HeatmapPoint {
        Vector position;
        float intensity;
        float timestamp;
    };
    
    std::vector<HeatmapPoint> heatmapData;
    const int GRID_SIZE = 100;
    float heatmapGrid[100][100];
};

// Implementation
void HeatmapGenerator::RecordPosition(Vector pos, float intensity) {
    heatmapData.push_back({pos, intensity, GetTime()});
    
    // Convert world coordinates to grid coordinates
    int gridX = (int)((pos.X + 4096) / 8192 * GRID_SIZE);
    int gridY = (int)((pos.Y + 5120) / 10240 * GRID_SIZE);
    
    if (gridX >= 0 && gridX < GRID_SIZE && gridY >= 0 && gridY < GRID_SIZE) {
        heatmapGrid[gridX][gridY] += intensity;
    }
}
```

---

## 2. Advanced BakkesMod SDK Techniques

### A. Advanced Event Hooking

**Function Hooking Patterns:**
```cpp
// Hook into more specific game events
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
}
```

**Advanced Function Hooking with Parameters:**
```cpp
// Hook with caller parameter access
gameWrapper->HookEventWithCaller<CarWrapper>("Function TAGame.Car_TA.SetVehicleInput",
    [this](CarWrapper caller, void* params, const std::string& eventName) {
        if (caller.IsNull()) return;
        
        // Access the input structure
        struct VehicleInputs {
            float throttle;
            float steer;
            float pitch;
            float yaw;
            float roll;
            bool jump;
            bool boost;
            bool handbrake;
        };
        
        VehicleInputs* inputs = (VehicleInputs*)params;
        if (inputs && inputs->boost) {
            // Player is boosting
            OnBoostInput(caller);
        }
    });
```

### B. Advanced Rendering Techniques

**Custom Canvas Drawing:**
```cpp
void BoostMaster::RenderAdvancedOverlay(CanvasWrapper canvas) {
    if (!gameWrapper->IsInGame()) return;
    
    auto car = gameWrapper->GetLocalCar();
    if (car.IsNull()) return;
    
    // Get screen size
    Vector2 screenSize = canvas.GetSize();
    
    // Draw boost efficiency gauge
    DrawBoostEfficiencyGauge(canvas, screenSize);
    
    // Draw 3D world space indicators
    DrawWorldSpaceIndicators(canvas);
    
    // Draw predictive path
    DrawPredictivePath(canvas, car);
}

void BoostMaster::DrawBoostEfficiencyGauge(CanvasWrapper canvas, Vector2 screenSize) {
    float efficiency = calculateCurrentEfficiency();
    
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
```

**3D World Space Rendering:**
```cpp
void BoostMaster::DrawWorldSpaceIndicators(CanvasWrapper canvas) {
    const auto& pads = BoostPadHelper::GetCachedPads(this);
    
    for (const auto& pad : pads) {
        // Project 3D world position to 2D screen
        Vector2 screenPos = canvas.Project(pad.location);
        
        // Only draw if on screen
        Vector2 screenSize = canvas.GetSize();
        if (screenPos.X >= 0 && screenPos.X <= screenSize.X && 
            screenPos.Y >= 0 && screenPos.Y <= screenSize.Y) {
            
            // Draw boost pad indicator
            float radius = pad.type == PadType::Big ? 15.0f : 8.0f;
            LinearColor color = pad.type == PadType::Big ? 
                LinearColor{1.0f, 0.5f, 0.0f, 0.8f} :  // Orange for big pads
                LinearColor{0.0f, 0.5f, 1.0f, 0.8f};   // Blue for small pads
            
            canvas.SetColor(color);
            canvas.DrawCircle(screenPos, radius);
        }
    }
}
```

### C. Data Management & Persistence

**Advanced Data Structures:**
```cpp
// SQLite integration for advanced data storage
#include <sqlite3.h>

class DatabaseManager {
public:
    bool Initialize(const std::string& dbPath);
    void RecordMatch(const MatchData& match);
    void RecordSession(const SessionData& session);
    std::vector<MatchData> GetMatchHistory(int limit = 100);
    void ExportToCSV(const std::string& filename);
    
private:
    sqlite3* db;
    
    struct MatchData {
        std::string matchId;
        float duration;
        float totalBoost;
        int bigPads;
        int smallPads;
        float efficiency;
        std::string timestamp;
        std::string playlist;
    };
};

// Asynchronous data processing
class DataProcessor {
public:
    void ProcessSessionAsync(const SessionData& data);
    void GenerateReportAsync(std::function<void(SessionReport)> callback);
    
private:
    std::thread processingThread;
    std::queue<SessionData> processingQueue;
    std::mutex queueMutex;
};
```

---

## 3. Troubleshooting & Performance Optimization

### A. Common Issues & Solutions

**Memory Management:**
```cpp
// Avoid memory leaks in event handlers
class BoostMaster : public BakkesMod::Plugin::BakkesModPlugin {
private:
    // Use smart pointers for automatic cleanup
    std::shared_ptr<HeatmapGenerator> heatmapGen;
    std::unique_ptr<NotificationManager> notifManager;
    
    // Proper cleanup in destructor
    ~BoostMaster() {
        if (gameWrapper) {
            gameWrapper->UnregisterDrawables();
            // Unhook events
            gameWrapper->UnhookEvent("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded");
        }
    }
};

// Limit container sizes to prevent memory bloat
void BoostMaster::UpdatePerformanceMetrics() {
    const size_t MAX_HISTORY_SIZE = 10000;
    
    if (currentSession.speedHistory.size() > MAX_HISTORY_SIZE) {
        // Remove oldest entries
        currentSession.speedHistory.erase(
            currentSession.speedHistory.begin(),
            currentSession.speedHistory.begin() + (MAX_HISTORY_SIZE / 2)
        );
    }
}
```

**Performance Optimization:**
```cpp
// Efficient update loops
void BoostMaster::onLoad() {
    // Set up tick-based updates instead of continuous polling
    gameWrapper->SetTimeout([this](GameWrapper* gw) {
        UpdateMetrics();
        return false; // Don't repeat automatically
    }, 0.1f); // Update every 100ms
}

// Cache expensive calculations
class BoostMaster {
private:
    mutable std::optional<float> cachedEfficiency;
    mutable float lastEfficiencyUpdate = 0;
    
public:
    float GetCurrentEfficiency() const {
        float currentTime = GetTime();
        if (!cachedEfficiency || (currentTime - lastEfficiencyUpdate) > 1.0f) {
            cachedEfficiency = CalculateEfficiency();
            lastEfficiencyUpdate = currentTime;
        }
        return *cachedEfficiency;
    }
};
```

### B. Debugging Techniques

**Advanced Logging:**
```cpp
// Structured logging with levels
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

class Logger {
public:
    static void Log(LogLevel level, const std::string& category, const std::string& message) {
        std::string prefix = GetLogPrefix(level);
        std::string timestamp = GetTimestamp();
        
        std::string fullMessage = "[" + timestamp + "] " + prefix + " [" + category + "] " + message;
        
        if (cvarManager) {
            cvarManager->log(fullMessage);
        }
        
        // Also write to file for debugging
        WriteToLogFile(fullMessage);
    }
    
private:
    static std::string GetLogPrefix(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return "[DEBUG]";
            case LogLevel::INFO: return "[INFO]";
            case LogLevel::WARNING: return "[WARNING]";
            case LogLevel::ERROR: return "[ERROR]";
            default: return "[UNKNOWN]";
        }
    }
};

// Usage throughout the plugin
void BoostMaster::SaveTrainingDrill(const std::string& name) {
    Logger::Log(LogLevel::INFO, "DrillManager", "Saving drill: " + name);
    
    try {
        // ... drill saving logic ...
        Logger::Log(LogLevel::INFO, "DrillManager", "Successfully saved drill: " + name);
    }
    catch (const std::exception& e) {
        Logger::Log(LogLevel::ERROR, "DrillManager", "Failed to save drill: " + std::string(e.what()));
    }
}
```

**Error Handling Best Practices:**
```cpp
// Wrapper functions for safe API calls
template<typename T>
std::optional<T> SafeWrapperCall(std::function<T()> func, const std::string& context) {
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

// Usage
void BoostMaster::UpdateCarInfo() {
    auto car = SafeWrapperCall<CarWrapper>([this]() {
        return gameWrapper->GetLocalCar();
    }, "UpdateCarInfo");
    
    if (car && !car->IsNull()) {
        // Safe to use car
        auto location = SafeWrapperCall<Vector>([&]() {
            return car->GetLocation();
        }, "GetCarLocation");
        
        if (location) {
            // Process location
        }
    }
}
```

### C. Testing & Validation

**Unit Testing Framework:**
```cpp
// Simple testing framework for BakkesMod plugins
class PluginTester {
public:
    static void RunTests() {
        TestDrillManagement();
        TestPathfinding();
        TestDataPersistence();
    }
    
private:
    static void TestDrillManagement() {
        Logger::Log(LogLevel::INFO, "Testing", "Starting drill management tests...");
        
        // Test drill creation
        TrainingDrill testDrill;
        testDrill.name = "TestDrill";
        testDrill.carX = 0; testDrill.carY = 0; testDrill.carZ = 17;
        
        // Validate drill data
        assert(testDrill.name == "TestDrill");
        assert(testDrill.carZ == 17);
        
        Logger::Log(LogLevel::INFO, "Testing", "Drill management tests passed!");
    }
};
```

**Performance Profiling:**
```cpp
class PerformanceProfiler {
public:
    class ScopedTimer {
    public:
        ScopedTimer(const std::string& name) : name_(name), start_(std::chrono::high_resolution_clock::now()) {}
        ~ScopedTimer() {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count();
            PerformanceProfiler::RecordTiming(name_, duration);
        }
    private:
        std::string name_;
        std::chrono::high_resolution_clock::time_point start_;
    };
    
    static void RecordTiming(const std::string& name, int64_t microseconds) {
        timings_[name].push_back(microseconds);
        if (timings_[name].size() > 1000) {
            timings_[name].erase(timings_[name].begin());
        }
    }
    
    static void PrintReport() {
        for (const auto& [name, times] : timings_) {
            int64_t total = 0;
            for (auto time : times) total += time;
            int64_t average = total / times.size();
            
            Logger::Log(LogLevel::INFO, "Performance", 
                name + " - Avg: " + std::to_string(average) + "μs");
        }
    }
    
private:
    static std::map<std::string, std::vector<int64_t>> timings_;
};

// Usage
void BoostMaster::ExpensiveFunction() {
    PerformanceProfiler::ScopedTimer timer("ExpensiveFunction");
    
    // ... expensive operations ...
}
```

## Next Steps for Your BoostMaster Plugin

1. **Implement Phase 2 Analytics** - Start with the performance metrics system
2. **Add Advanced Notifications** - Implement the coaching tips system
3. **Create Heatmap Generation** - Begin with position tracking
4. **Optimize Performance** - Add profiling and caching
5. **Implement Testing** - Create validation framework
6. **Community Features** - Build drill sharing platform

This guide provides a comprehensive foundation for advanced BakkesMod development. Would you like me to elaborate on any specific section or help implement any particular feature?