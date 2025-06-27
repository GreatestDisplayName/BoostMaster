#include "pch.h"
#include "BoostMasterUI.h"
#include <fstream>
#include <iomanip>
#include "thirdparty/json.hpp"
#include "imgui/imgui.h"

using json = nlohmann::json;

void BoostHUDWindow::RenderWindow()
{
    ImGui::Text("Boost Stats HUD");
    ImGui::Text("Avg Boost/Min: %.2f", plugin->avgBoostPerMinute);

    if (ImGui::Button("Save TXT")) {
        std::ofstream f("boostmaster_stats.txt");
        if (f) {
            f << plugin->totalBoostUsed << ','
                << plugin->totalBoostTime << ','
                << plugin->avgBoostPerMinute;
        }
    }
    ImGui::SameLine();

    if (ImGui::Button("Export JSON")) {
        json j = { {"used", plugin->totalBoostUsed},
                   {"time", plugin->totalBoostTime},
                   {"avgpm", plugin->avgBoostPerMinute} };
        std::ofstream f("boostmaster_stats.json");
        if (f) f << std::setw(2) << j;
    }
    ImGui::SameLine();

    if (ImGui::Button("Import JSON")) {
        std::ifstream f("boostmaster_stats.json");
        if (f) {
            json j; f >> j;
            plugin->totalBoostUsed = j.value("used", 0.0f);
            plugin->totalBoostTime = j.value("time", 0.0f);
            plugin->avgBoostPerMinute = j.value("avgpm", 0.0f);
        }
    }
}

void BoostSettingsWindow::RenderSettings()
{
    ImGui::TextWrapped("Configure thresholds:");
    ImGui::SliderFloat("Low %", &plugin->cvarLowBoostThresh, 0.0f, 100.0f, "%.1f");
    ImGui::SliderFloat("Low Time (s)", &plugin->cvarLowBoostTime, 0.0f, 30.0f, "%.1f");
    ImGui::SliderFloat("Max Time (s)", &plugin->cvarMaxBoostTime, 0.0f, 30.0f, "%.1f");

    if (ImGui::Button("Apply Settings")) {
        plugin->cvarManager->getCvar("boostmaster_lowthreshold").setValue(plugin->cvarLowBoostThresh);
        plugin->cvarManager->getCvar("boostmaster_lowtime").setValue(plugin->cvarLowBoostTime);
        plugin->cvarManager->getCvar("boostmaster_maxtime").setValue(plugin->cvarMaxBoostTime);
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset Stats")) {
        plugin->ResetStats();
    }
}
