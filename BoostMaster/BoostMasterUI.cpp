#include "pch.h"
#include "BoostMasterUI.h"
#include "imgui/imgui.h"

// ---------------------------------
// BoostHUDWindow
// ---------------------------------

void BoostHUDWindow::RenderWindow()
{
    // No boost tracking available, stub out HUD
    ImGui::Text("BoostMaster HUD (no data available)");
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
