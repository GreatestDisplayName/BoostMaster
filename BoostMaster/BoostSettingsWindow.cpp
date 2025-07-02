#include "pch.h"
#include "BoostSettingsWindow.h"
#include "BoostMaster.h"
#include "imgui/imgui.h"
#include "BoostPadData.h"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>

// Settings for pad visualization
static bool showPads = true;
static int padTypeFilter = -1; // -1 = all, 0 = big, 1 = small
static float overlayColor[4] = {1.0f, 1.0f, 0.0f, 1.0f}; // Default yellow
static float overlaySize = 1.0f;
extern int pathAlgo; // 0 = Dijkstra, 1 = A*
static char errorLogPath[256] = "error.log";

void ExportHistory(const std::vector<float>& history) {
    std::filesystem::create_directories("data");
    std::ofstream out("data/boost_history_export.csv");
    for (float v : history) out << v << "\n";
}

void ImportHistory(std::vector<float>& history) {
    std::ifstream in("data/boost_history_export.csv");
    if (!in) return;
    std::string line;
    while (std::getline(in, line)) {
        try { history.push_back(std::stof(line)); } catch (...) {}
    }
}

void ExportErrorLog(const char* path) {
    std::ifstream in("error.log");
    if (!in) return;
    std::ofstream out(path);
    out << in.rdbuf();
}

void BoostSettingsWindow::Render() {
    ImGui::Begin("BoostMaster Settings");
    if (ImGui::SliderFloat("Low Threshold (%)", &plugin->cvarLowBoostThresh, 0.0f, 100.0f)) {
        plugin->cvarManager->getCvar("boostmaster_lowthreshold").setValue(plugin->cvarLowBoostThresh);
    }
    if (ImGui::SliderFloat("Low Time (s)", &plugin->cvarLowBoostTime, 0.1f, 30.0f)) {
        plugin->cvarManager->getCvar("boostmaster_lowtime").setValue(plugin->cvarLowBoostTime);
    }
    if (ImGui::SliderFloat("Max Time (s)", &plugin->cvarMaxBoostTime, 0.1f, 30.0f)) {
        plugin->cvarManager->getCvar("boostmaster_maxtime").setValue(plugin->cvarMaxBoostTime);
    }
    ImGui::Separator();
    ImGui::Checkbox("Show Boost Pads", &showPads);
    ImGui::Text("Pad Type Filter:");
    ImGui::RadioButton("All", &padTypeFilter, -1); ImGui::SameLine();
    ImGui::RadioButton("Big", &padTypeFilter, 0); ImGui::SameLine();
    ImGui::RadioButton("Small", &padTypeFilter, 1);
    ImGui::ColorEdit4("Overlay Color", overlayColor);
    ImGui::SliderFloat("Overlay Size", &overlaySize, 0.5f, 3.0f);
    if (ImGui::SliderFloat("Font Scale", &ImGui::GetIO().FontGlobalScale, 0.5f, 2.0f)) {
        // Font scale will update globally
    }
    ImGui::Separator();
    ImGui::Text("Pathfinding Algorithm:");
    ImGui::RadioButton("Dijkstra", &pathAlgo, 0); ImGui::SameLine();
    ImGui::RadioButton("A* (WIP)", &pathAlgo, 1);
    ImGui::Separator();
    if (ImGui::Button("Export History")) ExportHistory(plugin->historyLog);
    ImGui::SameLine();
    if (ImGui::Button("Import History")) ImportHistory(plugin->historyLog);
    ImGui::Separator();
    ImGui::InputText("Export Error Log As", errorLogPath, sizeof(errorLogPath));
    if (ImGui::Button("Export Error Log")) ExportErrorLog(errorLogPath);
    ImGui::End();
}

// Accessors for use in BoostPadHelper
bool BoostSettingsWindow::ShouldShowPads() { return showPads; }
std::optional<PadType> BoostSettingsWindow::GetPadTypeFilter() {
    if (padTypeFilter == 0) return PadType::Big;
    if (padTypeFilter == 1) return PadType::Small;
    return std::nullopt;
}
const float* BoostSettingsWindow::GetOverlayColor() { return overlayColor; }
float BoostSettingsWindow::GetOverlaySize() { return overlaySize; }

void BoostSettingsWindow::ToggleShowPads() {
    showPads = !showPads;
}
