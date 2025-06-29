#pragma once

#include <vector>
#include <string>
#include <map>
#include <memory>
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/wrappers/CanvasWrapper.h"
#include "BoostHUDWindow.h"
#include "BoostSettingsWindow.h"
#include "BoostPadHelper.h"

struct TrainingDrill {
    std::string name;
    float carX, carY, carZ;
    float carPitch, carYaw, carRoll;
    float ballX, ballY, ballZ;
    float ballVelX, ballVelY, ballVelZ;
};

class BoostMaster : public BakkesMod::Plugin::BakkesModPlugin {
public:
    virtual void onLoad() override;
    virtual void onUnload() override;

    void ResetStats();
    void PrintPadPath();
    void saveMatch();

    void SaveTrainingDrill(const std::string& name);
    void LoadTrainingDrill(const std::string& name);
    void ListTrainingDrills();
    void DeleteTrainingDrill(const std::string& name);
    void LoadAllTrainingDrills();
    void SaveAllTrainingDrills();

    void RegisterDrawables();
    void UnregisterDrawables();

    std::shared_ptr<BoostHUDWindow> hudWindow;
    std::shared_ptr<BoostSettingsWindow> settingsWindow;

    std::vector<int> lastPath;

    float cvarLowBoostThresh = 20.0f;
    float cvarLowBoostTime = 5.0f;
    float cvarMaxBoostTime = 5.0f;

    float totalBoostUsed = 0.0f;
    float totalBoostTime = 0.0f;

    int bigPads = 0;
    int smallPads = 0;

    int lastBoostAmt = -1;

    std::vector<float> efficiencyLog;
    std::vector<float> historyLog;

    std::map<std::string, TrainingDrill> drills;
};
