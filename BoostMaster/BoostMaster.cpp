#include "pch.h"
#include "BoostMaster.h"
#include "BoostMasterUI.h"
#include <filesystem>
#include <fstream>
#include <sstream>

BAKKESMOD_PLUGIN(BoostMaster, "Boost usage tracker and trainer", "1.0", PLUGINTYPE_FREEPLAY)

void BoostMaster::onLoad()
{
    cvarManager->log("BoostMaster: Loaded");

    // Register ConVars for runtime tweaking
    cvarManager->registerCvar("boostmaster_lowthreshold",
        std::to_string(cvarLowBoostThresh),
        "Threshold (%) below which boost is considered low",
        true, true, 0, true, 100.0f);
    cvarManager->registerCvar("boostmaster_lowtime",
        std::to_string(cvarLowBoostTime),
        "Seconds at low boost before warning",
        true, true);
    cvarManager->registerCvar("boostmaster_maxtime",
        std::to_string(cvarMaxBoostTime),
        "Seconds at full boost before warning",
        true, true);

    // Console command to reset stats mid-game
    cvarManager->registerNotifier(
        "boostmaster_reset",
        [this](std::vector<std::string> params) { ResetStats(); },
        "Reset the current session stats",
        PERMISSION_ALL
    );

    // Hook events
    gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded",
        [this](std::string) { saveMatch(); });

    // Create UI windows
    hudWindow = std::make_shared<BoostHUDWindow>(this);
    settingsWindow = std::make_shared<BoostSettingsWindow>(this);

    // Load historical CSV data
    loadHistory();
}

void BoostMaster::onUnload()
{
    cvarManager->log("BoostMaster: Unloaded");
}

void BoostMaster::RenderFrame()
{
    // This function is now a stub, as CarWrapper and boost tracking are not available in this SDK
}

void BoostMaster::saveMatch()
{
    std::filesystem::create_directories("data");
    std::ofstream out("data/boost_history.csv", std::ios::app);
    // PriWrapper and match score not available, stub out
    int score = 0;
    float eff = 0.f;
    out << score << "," << totalBoostUsed << "," << eff << "\n";
    out.close();
}

void BoostMaster::loadHistory()
{
    std::ifstream in("data/boost_history.csv");
    std::string line;
    while (std::getline(in, line)) {
        std::stringstream ss(line);
        float s, b, e;
        char c;
        ss >> s >> c >> b >> c >> e;
        historyLog.push_back(e);
    }
}

void BoostMaster::ResetStats()
{
    totalBoostUsed = totalBoostTime = timeWithNoBoost = 0;
    timeAtLowBoost = timeAtMaxBoost = 0;
    bigPads = smallPads = 0;
    efficiencyLog.clear();
    if (cvarManager)
        cvarManager->log("BoostMaster: Stats reset (via UI)");
}
