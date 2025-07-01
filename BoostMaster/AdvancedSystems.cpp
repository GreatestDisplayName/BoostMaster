#include "pch.h"
#include "BoostMaster.h"
#include "BoostPadHelper.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>

// Static member definitions
LogLevel Logger::currentLogLevel = LogLevel::INFO;
std::map<std::string, std::vector<int64_t>> PerformanceProfiler::timings_;

// Logger Implementation
void Logger::Log(LogLevel level, const std::string& category, const std::string& message) {
    if (level < currentLogLevel) return;
    
    std::string prefix = GetLogPrefix(level);
    std::string timestamp = GetTimestamp();
    std::string fullMessage = "[" + timestamp + "] " + prefix + " [" + category + "] " + message;
    
    if (_globalCvarManager) {
        _globalCvarManager->log(fullMessage);
    }
    
    WriteToLogFile(fullMessage);
}

std::string Logger::GetLogPrefix(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "[DEBUG]";
        case LogLevel::INFO: return "[INFO]";
        case LogLevel::WARNING: return "[WARNING]";
        case LogLevel::ERROR: return "[ERROR]";
        default: return "[UNKNOWN]";
    }
}

std::string Logger::GetTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
    return ss.str();
}

void Logger::WriteToLogFile(const std::string& message) {
    try {
        std::filesystem::create_directories("data");
        std::ofstream logFile("data/boostmaster.log", std::ios::app);
        if (logFile.is_open()) {
            logFile << message << std::endl;
        }
    }
    catch (...) {
        // Fail silently to avoid infinite logging loops
    }
}

// PerformanceProfiler Implementation
PerformanceProfiler::ScopedTimer::ScopedTimer(const std::string& name) 
    : name_(name), start_(std::chrono::high_resolution_clock::now()) {}

PerformanceProfiler::ScopedTimer::~ScopedTimer() {
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count();
    PerformanceProfiler::RecordTiming(name_, duration);
}

void PerformanceProfiler::RecordTiming(const std::string& name, int64_t microseconds) {
    timings_[name].push_back(microseconds);
    if (timings_[name].size() > 1000) {
        timings_[name].erase(timings_[name].begin());
    }
}

void PerformanceProfiler::PrintReport() {
    for (const auto& [name, times] : timings_) {
        if (times.empty()) continue;
        
        int64_t total = 0;
        int64_t minTime = times[0];
        int64_t maxTime = times[0];
        
        for (auto time : times) {
            total += time;
            minTime = std::min(minTime, time);
            maxTime = std::max(maxTime, time);
        }
        
        int64_t average = total / times.size();
        
        Logger::Log(LogLevel::INFO, "Performance", 
            name + " - Avg: " + std::to_string(average) + "μs, Min: " + 
            std::to_string(minTime) + "μs, Max: " + std::to_string(maxTime) + "μs");
    }
}

void PerformanceProfiler::ClearTimings() {
    timings_.clear();
}

// NotificationManager Implementation
void NotificationManager::ShowNotification(const Notification& notif) {
    // Remove old notifications if we have too many
    while (activeNotifications.size() >= MAX_NOTIFICATIONS) {
        activeNotifications.erase(activeNotifications.begin());
    }
    
    activeNotifications.push_back(notif);
    Logger::Log(LogLevel::INFO, "Notifications", "Showing: " + notif.message);
}

void NotificationManager::RegisterCustomTrigger(std::function<bool()> condition, Notification notif) {
    customTriggers.push_back({condition, notif});
}

void NotificationManager::Update(float deltaTime) {
    // Update active notifications
    for (auto& notif : activeNotifications) {
        notif.timeShown += deltaTime;
    }
    
    // Remove expired notifications
    activeNotifications.erase(
        std::remove_if(activeNotifications.begin(), activeNotifications.end(),
            [](const Notification& n) { return n.timeShown >= n.duration; }),
        activeNotifications.end()
    );
    
    // Check custom triggers
    for (const auto& [condition, notif] : customTriggers) {
        try {
            if (condition()) {
                ShowNotification(notif);
            }
        }
        catch (...) {
            Logger::Log(LogLevel::ERROR, "Notifications", "Error in custom trigger");
        }
    }
}

void NotificationManager::RenderNotifications(CanvasWrapper canvas) {
    if (activeNotifications.empty()) return;
    
    Vector2 screenSize = canvas.GetSize();
    float startY = 100.0f;
    float notifHeight = 40.0f;
    float margin = 5.0f;
    
    for (size_t i = 0; i < activeNotifications.size(); ++i) {
        const auto& notif = activeNotifications[i];
        
        Vector2 pos = {screenSize.X - 400, startY + i * (notifHeight + margin)};
        Vector2 size = {350, notifHeight};
        
        // Background with fade based on time remaining
        float alpha = 1.0f - (notif.timeShown / notif.duration);
        alpha = std::max(0.0f, std::min(1.0f, alpha));
        
        LinearColor bgColor = {0.0f, 0.0f, 0.0f, 0.7f * alpha};
        canvas.SetColor(bgColor);
        canvas.DrawRect(pos, size);
        
        // Border with notification color
        LinearColor borderColor = notif.color;
        borderColor.A *= alpha;
        canvas.SetColor(borderColor);
        canvas.DrawRect(pos, {size.X, 3}); // Top border
        
        // Text
        canvas.SetColor({1.0f, 1.0f, 1.0f, alpha});
        canvas.SetPosition({pos.X + 10, pos.Y + 10});
        canvas.DrawString(notif.message);
    }
}

void NotificationManager::ClearAll() {
    activeNotifications.clear();
}

// HeatmapGenerator Implementation
void HeatmapGenerator::RecordPosition(Vector pos, float intensity) {
    float currentTime = std::chrono::duration<float>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    heatmapData.push_back({pos, intensity, currentTime});
    
    int gridX, gridY;
    WorldToGrid(pos, gridX, gridY);
    
    if (gridX >= 0 && gridX < GRID_SIZE && gridY >= 0 && gridY < GRID_SIZE) {
        positionGrid[gridX][gridY] += intensity;
    }
    
    // Limit data size to prevent memory bloat
    if (heatmapData.size() > 50000) {
        heatmapData.erase(heatmapData.begin(), heatmapData.begin() + 10000);
    }
}

void HeatmapGenerator::RecordBoostUsage(Vector pos, float boostUsed) {
    float currentTime = std::chrono::duration<float>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    boostUsageData.push_back({pos, boostUsed, currentTime});
    
    int gridX, gridY;
    WorldToGrid(pos, gridX, gridY);
    
    if (gridX >= 0 && gridX < GRID_SIZE && gridY >= 0 && gridY < GRID_SIZE) {
        boostGrid[gridX][gridY] += boostUsed;
    }
    
    if (boostUsageData.size() > 20000) {
        boostUsageData.erase(boostUsageData.begin(), boostUsageData.begin() + 5000);
    }
}

void HeatmapGenerator::WorldToGrid(Vector worldPos, int& gridX, int& gridY) {
    // Rocket League field dimensions: X: -4096 to 4096, Y: -5120 to 5120
    gridX = (int)((worldPos.X + 4096.0f) / 8192.0f * GRID_SIZE);
    gridY = (int)((worldPos.Y + 5120.0f) / 10240.0f * GRID_SIZE);
}

void HeatmapGenerator::GeneratePositionHeatmap() {
    Logger::Log(LogLevel::INFO, "Heatmap", "Generating position heatmap with " + 
               std::to_string(heatmapData.size()) + " data points");
    // Implementation for generating visual heatmap
}

void HeatmapGenerator::GenerateBoostUsageHeatmap() {
    Logger::Log(LogLevel::INFO, "Heatmap", "Generating boost usage heatmap with " + 
               std::to_string(boostUsageData.size()) + " data points");
    // Implementation for generating boost usage heatmap
}

void HeatmapGenerator::ExportHeatmap(const std::string& filename) {
    try {
        std::filesystem::create_directories("data/heatmaps");
        std::ofstream file("data/heatmaps/" + filename + ".csv");
        
        // Export position grid
        file << "Position Heatmap\n";
        for (int y = 0; y < GRID_SIZE; ++y) {
            for (int x = 0; x < GRID_SIZE; ++x) {
                file << positionGrid[x][y];
                if (x < GRID_SIZE - 1) file << ",";
            }
            file << "\n";
        }
        
        file << "\nBoost Usage Heatmap\n";
        for (int y = 0; y < GRID_SIZE; ++y) {
            for (int x = 0; x < GRID_SIZE; ++x) {
                file << boostGrid[x][y];
                if (x < GRID_SIZE - 1) file << ",";
            }
            file << "\n";
        }
        
        Logger::Log(LogLevel::INFO, "Heatmap", "Exported heatmap to " + filename + ".csv");
    }
    catch (const std::exception& e) {
        Logger::Log(LogLevel::ERROR, "Heatmap", "Failed to export heatmap: " + std::string(e.what()));
    }
}

void HeatmapGenerator::ClearData() {
    heatmapData.clear();
    boostUsageData.clear();
    
    // Clear grids
    for (int x = 0; x < GRID_SIZE; ++x) {
        for (int y = 0; y < GRID_SIZE; ++y) {
            positionGrid[x][y] = 0.0f;
            boostGrid[x][y] = 0.0f;
        }
    }
    
    Logger::Log(LogLevel::INFO, "Heatmap", "Cleared all heatmap data");
}