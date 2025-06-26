#include "pch.h"
#include "BoostMasterUI.h"
#include "imgui/imgui.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include "thirdparty/json.hpp"
#include "bakkesmod/wrappers/GameEvent/ServerWrapper.h"
#include "bakkesmod/wrappers/GameObject/BoostPickupWrapper.h"
#include "bakkesmod/wrappers/GameObject/CarWrapper.h"
using json = nlohmann::json;

// ---------------------------------
// BoostHUDWindow
// ---------------------------------

void BoostHUDWindow::RenderWindow()
{
    ImGui::Text("BoostMaster HUD");
    ImGui::Text("Avg Boost/Min: %.2f", plugin->avgBoostPerMinute);
    if (ImGui::Button("Save Stats to File")) {
        std::ofstream out("boostmaster_stats.txt");
        out << "Total Boost Used: " << plugin->totalBoostUsed << "\n";
        out << "Total Boost Time: " << plugin->totalBoostTime << "\n";
        out << "Avg Boost/Min: " << plugin->avgBoostPerMinute << "\n";
        out.close();
    }
    ImGui::SameLine();
    if (ImGui::Button("Export JSON")) {
        json j;
        j["totalBoostUsed"] = plugin->totalBoostUsed;
        j["totalBoostTime"] = plugin->totalBoostTime;
        j["avgBoostPerMinute"] = plugin->avgBoostPerMinute;
        std::ofstream out("boostmaster_stats.json");
        out << j;
        out.close();
    }
    ImGui::SameLine();
    if (ImGui::Button("Import JSON")) {
        std::ifstream in("boostmaster_stats.json");
        if (in) {
            json j;
            in >> j;
            plugin->totalBoostUsed = j.value("totalBoostUsed", 0.0f);
            plugin->totalBoostTime = j.value("totalBoostTime", 0.0f);
            plugin->avgBoostPerMinute = j.value("avgBoostPerMinute", 0.0f);
        }
    }

    // Draw boost pad locations for trainer
    if (plugin->gameWrapper && plugin->gameWrapper->IsInGame()) {
        auto server = plugin->gameWrapper->GetCurrentGameState();
        if (!server.IsNull()) {
            auto pads = server.GetBoostPads();
            for (int i = 0; i < pads.Count(); ++i) {
                BoostPickupWrapper pad = pads.Get(i);
                if (!pad.IsNull()) {
                    Vector padLoc = pad.GetLocation();
                    // Project 3D world position to 2D screen (fallback: draw at fixed position if Project unavailable)
                    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
                    // NOTE: Replace the following line with actual projection if available in your SDK
                    draw_list->AddCircle(ImVec2(100 + 20 * i, 100), 10.0f, IM_COL32(255, 255, 0, 255), 0, 2.0f);
                }
            }
        }
    }
}

// ---------------------------------
// BoostSettingsWindow
// ---------------------------------

void BoostSettingsWindow::RenderSettings()
{
    ImGui::TextWrapped("Adjust BoostMaster thresholds:");

    if (ImGui::SliderFloat("Low Boost Threshold (%)",
        &plugin->cvarLowBoostThresh,
        0.0f, 100.0f, "%.1f"))
    {
        plugin->cvarManager->getCvar("boostmaster_lowboostthreshold").setValue(plugin->cvarLowBoostThresh);
    }

    if (ImGui::SliderFloat("Low Boost Time (s)",
        &plugin->cvarLowBoostTime,
        0.1f, 30.0f, "%.1f"))
    {
        plugin->cvarManager->getCvar("boostmaster_lowboosttime").setValue(plugin->cvarLowBoostTime);
    }

    if (ImGui::SliderFloat("Max Boost Time (s)",
        &plugin->cvarMaxBoostTime,
        0.1f, 30.0f, "%.1f"))
    {
        plugin->cvarManager->getCvar("boostmaster_maxboosttime").setValue(plugin->cvarMaxBoostTime);
    }

    ImGui::Separator();
    if (ImGui::Button("Reset Stats")) {
        plugin->ResetStats();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("Reset all tracked stats");
}
