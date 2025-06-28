#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "BoostMasterUI.h"
#include "BoostPadGraph.h"
#include <vector>
#include <memory>
#include <string>
#include <map>

class BoostHUDWindow;
class BoostSettingsWindow;

struct TrainingDrill {
    std::string name;
    float carX, carY, carZ, carPitch, carYaw, carRoll;
    float ballX, ballY, ballZ, ballVelX, ballVelY, ballVelZ;
};

class BoostMaster : public BakkesMod::Plugin::BakkesModPlugin {
public:
    void onLoad() override;
    void onUnload() override;
    void RenderFrame();
    void saveMatch();
    void loadHistory();
    void ResetStats();
    void PrintPadPath();

    // Drawable registration for path overlay
    void RegisterDrawables();
    void UnregisterDrawables();

    // Boost tracking stats
    float totalBoostUsed{0}, totalBoostTime{0}, lastBoostAmt{-1};
    float avgBoostPerMinute{0};
    int bigPads{0}, smallPads{0};
    std::vector<float> efficiencyLog, historyLog;
    std::vector<int> lastPath;

    // Config
    float cvarLowBoostThresh{20.0f};
    float cvarLowBoostTime{3.0f};
    float cvarMaxBoostTime{5.0f};

    // Pad time tracking
    float timeAtLowBoost{0};
    float timeAtMaxBoost{0};
    float timeWithNoBoost{0};

    // Low boost warning state
    float lowBoostTimer{0};
    bool showLowBoostWarning{false};

    // UI windows
    std::shared_ptr<BoostHUDWindow> hudWindow;
    std::shared_ptr<BoostSettingsWindow> settingsWindow;

    // Training drills
    std::map<std::string, TrainingDrill> drills;
    void SaveTrainingDrill(const std::string& name);
    void LoadTrainingDrill(const std::string& name);
    void DeleteTrainingDrill(const std::string& name);
    void ListTrainingDrills();
    void LoadAllTrainingDrills();
    void SaveAllTrainingDrills();
};
