#pragma once

#include "GuiBase.h"
#include <optional>

// Forward declarations
class BoostMaster;
enum class PadType;

class BoostSettingsWindow : public GuiBase {
public:
    BoostSettingsWindow(BoostMaster* plugin) : plugin(plugin) {}
    void Render() override;

    // Static methods for accessing settings
    static bool ShouldShowPads();
    static std::optional<PadType> GetPadTypeFilter();
    static const float* GetOverlayColor();
    static float GetOverlaySize();
    static void ToggleShowPads();

private:
    BoostMaster* plugin;
};
