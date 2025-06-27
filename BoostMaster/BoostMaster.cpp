#include "pch.h"
#include "BoostMaster.h"
#include "BoostMasterUI.h"
#include "BoostPadHelper.h"
#include <filesystem>
#include <fstream>
#include <sstream>

BAKKESMOD_PLUGIN(BoostMaster, "Boost usage tracker and trainer", "1.0", PLUGINTYPE_FREEPLAY)

void BoostMaster::onLoad() {
    cvarManager->registerCvar("boostmaster_lowthreshold", std::to_string(cvarLowBoostThresh), "% below which boost is low", true, true, 0.0f, true, 100.0f);
    cvarManager->registerCvar("boostmaster_lowtime", std::to_string(cvarLowBoostTime), "seconds low before warning", true, true, 0.1f, true, 30.0f);
    cvarManager->registerCvar("boostmaster_maxtime", std::to_string(cvarMaxBoostTime), "seconds full before warning", true, true, 0.1f, true, 30.0f);

    cvarManager->registerNotifier("boostmaster_reset", [this](auto&) { ResetStats(); }, "Reset stats", PERMISSION_ALL);
    gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded", [this](auto&) { saveMatch(); });
    hudWindow = std::make_shared<BoostHUDWindow>(this);
    settingsWindow = std::make_shared<BoostSettingsWindow>(this);
    loadHistory();
}

void BoostMaster::onUnload() {
    cvarManager->log("BoostMaster: Unloaded");
}

void BoostMaster::RenderFrame() {
    if (totalBoostTime > 0)
        avgBoostPerMinute = (totalBoostUsed / totalBoostTime) * 60.0f;
    else
        avgBoostPerMinute = 0;
    BoostPadHelper::DrawAllPads(gameWrapper.get());
}

void BoostMaster::saveMatch() {
    std::filesystem::create_directories("data");
    std::ofstream out("data/boost_history.csv", std::ios::app);
    out << 0 << ',' << totalBoostUsed << ',' << 0.0f << "\n";
}

void BoostMaster::loadHistory() {
    std::ifstream in("data/boost_history.csv");
    std::string line;
    while (std::getline(in, line)) {
        std::stringstream ss(line);
        float s, u, e;
        char c;
        ss >> s >> c >> u >> c >> e;
        historyLog.push_back(e);
    }
}

void BoostMaster::ResetStats() {
    totalBoostUsed = totalBoostTime = timeWithNoBoost = 0;
    timeAtLowBoost = timeAtMaxBoost = 0;
    bigPads = smallPads = 0;
    efficiencyLog.clear();
    cvarManager->log("BoostMaster: Stats reset");
}

void BoostMaster::PrintPadPath() {
    cvarManager->log("Pad pathfinding not supported.");
}
