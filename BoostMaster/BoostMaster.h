#pragma once

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <queue>
#include <functional>
#include <optional>
#include <chrono>
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/wrappers/CanvasWrapper.h"

// Forward declarations to avoid circular dependencies
class BoostPadHelper;
class BoostHUDWindow;
class BoostSettingsWindow;
class NotificationManager;
class HeatmapGenerator;

struct TrainingDrill {
    std::string name;
    float carX, carY, carZ;
    float carPitch, carYaw, carRoll;
    float ballX, ballY, ballZ;
    float ballVelX, ballVelY, ballVelZ;
};

// Advanced Analytics System
struct PerformanceMetrics {
    float sessionStartTime = 0.0f;
    float totalDistance = 0.0f;
    float averageSpeed = 0.0f;
    int totalDemos = 0;
    int totalSaves = 0;
    int ballTouches = 0;
    std::vector<float> speedHistory;
    std::vector<Vector> positionHistory;
    std::string detectedPlaystyle = "Balanced";
    
    void Reset() {
        sessionStartTime = 0.0f;
        totalDistance = 0.0f;
        averageSpeed = 0.0f;
        totalDemos = 0;
        totalSaves = 0;
        ballTouches = 0;
        speedHistory.clear();
        positionHistory.clear();
        detectedPlaystyle = "Balanced";
    }
};

// Notification System
enum class NotificationType {
    LowBoost,
    HighEfficiency,
    PositioningSuggestion,
    BoostPadTiming,
    GoalScored,
    BallHit,
    Custom
};

struct Notification {
    NotificationType type;
    std::string message;
    float duration;
    bool soundEnabled;
    LinearColor color;
    float timeShown = 0.0f;
};

class NotificationManager {
public:
    void ShowNotification(const Notification& notif);
    void RegisterCustomTrigger(std::function<bool()> condition, Notification notif);
    void Update(float deltaTime);
    void RenderNotifications(CanvasWrapper canvas);
    void ClearAll();
    
private:
    std::vector<std::pair<std::function<bool()>, Notification>> customTriggers;
    std::vector<Notification> activeNotifications;
    const float MAX_NOTIFICATIONS = 5;
};

// Heatmap System
class HeatmapGenerator {
public:
    struct HeatmapPoint {
        Vector position;
        float intensity;
        float timestamp;
    };
    
    void RecordPosition(Vector pos, float intensity = 1.0f);
    void RecordBoostUsage(Vector pos, float boostUsed);
    void GenerateBoostUsageHeatmap();
    void GeneratePositionHeatmap();
    void ExportHeatmap(const std::string& filename);
    void ClearData();
    
    const std::vector<HeatmapPoint>& GetHeatmapData() const { return heatmapData; }
    
private:
    std::vector<HeatmapPoint> heatmapData;
    std::vector<HeatmapPoint> boostUsageData;
    static const int GRID_SIZE = 100;
    float positionGrid[100][100] = {};
    float boostGrid[100][100] = {};
    
    void WorldToGrid(Vector worldPos, int& gridX, int& gridY);
};

// Logging System
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

class Logger {
public:
    static void Log(LogLevel level, const std::string& category, const std::string& message);
    static void SetLogLevel(LogLevel level) { currentLogLevel = level; }
    
private:
    static LogLevel currentLogLevel;
    static std::string GetLogPrefix(LogLevel level);
    static std::string GetTimestamp();
    static void WriteToLogFile(const std::string& message);
};

// Performance Profiler
class PerformanceProfiler {
public:
    class ScopedTimer {
    public:
        ScopedTimer(const std::string& name);
        ~ScopedTimer();
    private:
        std::string name_;
        std::chrono::high_resolution_clock::time_point start_;
    };
    
    static void RecordTiming(const std::string& name, int64_t microseconds);
    static void PrintReport();
    static void ClearTimings();
    
private:
    static std::map<std::string, std::vector<int64_t>> timings_;
};

class BoostMaster : public BakkesMod::Plugin::BakkesModPlugin {
public:
    virtual void onLoad() override;
    virtual void onUnload() override;

    // Core functionality
    void ResetStats();
    void PrintPadPath();
    void saveMatch();
    void loadHistory();

    // Training drill management
    void SaveTrainingDrill(const std::string& name);
    void LoadTrainingDrill(const std::string& name);
    void ListTrainingDrills();
    void DeleteTrainingDrill(const std::string& name);
    void LoadAllTrainingDrills();
    void SaveAllTrainingDrills();

    // Advanced analytics
    void UpdatePerformanceMetrics();
    void AnalyzePlaystyle();
    void GenerateSessionReport();
    float GetCurrentEfficiency() const;

    // Event handlers
    void OnGoalScored();
    void OnBallHit();
    void OnBoostPickup();
    void OnCarDemolished();
    void OnBoostInput(CarWrapper caller);

    // Coaching system
    void CheckCoachingTriggers();
    void RegisterAdvancedHooks();

    // Rendering
    void RegisterDrawables();
    void UnregisterDrawables();
    void RenderAdvancedOverlay(CanvasWrapper canvas);
    void DrawBoostEfficiencyGauge(CanvasWrapper canvas, Vector2 screenSize);
    void DrawWorldSpaceIndicators(CanvasWrapper canvas);

    // UI Windows
    std::shared_ptr<BoostHUDWindow> hudWindow;
    std::shared_ptr<BoostSettingsWindow> settingsWindow;

    // Core data
    std::vector<int> lastPath;
    float cvarLowBoostThresh = 20.0f;
    float cvarLowBoostTime = 5.0f;
    float cvarMaxBoostTime = 5.0f;
    float totalBoostUsed = 0.0f;
    float totalBoostTime = 0.0f;
    float avgBoostPerMinute = 0.0f;
    int bigPads = 0;
    int smallPads = 0;
    int lastBoostAmt = -1;

    std::vector<float> efficiencyLog;
    std::vector<float> historyLog;
    std::map<std::string, TrainingDrill> drills;

    // Advanced systems
    PerformanceMetrics currentSession;
    std::unique_ptr<NotificationManager> notificationManager;
    std::unique_ptr<HeatmapGenerator> heatmapGenerator;
    
    // Performance optimization
    mutable std::optional<float> cachedEfficiency;
    mutable float lastEfficiencyUpdate = 0.0f;
    float lastUpdateTime = 0.0f;

private:
    // Helper methods
    template<typename T>
    std::optional<T> SafeWrapperCall(std::function<T()> func, const std::string& context);
    
    void InitializeAdvancedSystems();
    void CleanupAdvancedSystems();
    float GetGameTime() const;
};
