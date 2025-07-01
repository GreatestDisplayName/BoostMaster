#include "pch.h"
#include "BoostHUDWindow.h"
#include "BoostMaster.h"
#include "imgui/imgui.h"
#include "BoostPadHelper.h"
#include <vector>
#include <algorithm>

void DrawHistogram(const std::vector<float>& data, const char* label) {
    if (!data.empty()) {
        ImGui::PlotHistogram(label, data.data(), (int)data.size(), 0, nullptr, 0.0f, *std::max_element(data.begin(), data.end()), ImVec2(0, 60));
    } else {
        ImGui::Text("No data available");
    }
}

void BoostHUDWindow::Render() {
    // Draw path overlay if path exists
    if (!plugin->lastPath.empty()) {
        BoostPadHelper::DrawPathOverlay(plugin);
    }
    ImGui::Begin("Boost Stats");
    ImGui::Text("Total Used: %.1f", plugin->totalBoostUsed);
    ImGui::Text("Avg Boost/Min: %.1f", plugin->avgBoostPerMinute);
    ImGui::Text("Big Pads: %d  Small Pads: %d", plugin->bigPads, plugin->smallPads);
    ImGui::Separator();
    ImGui::Text("Efficiency Log (last session):");
    DrawHistogram(plugin->efficiencyLog, "Efficiency");
    ImGui::Text("History Log (all sessions):");
    DrawHistogram(plugin->historyLog, "History");
    ImGui::End();
}
