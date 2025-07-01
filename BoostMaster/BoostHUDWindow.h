#pragma once

#include "GuiBase.h"

// Forward declaration
class BoostMaster;

class BoostHUDWindow : public GuiBase {
public:
    BoostHUDWindow(BoostMaster* plugin) : plugin(plugin) {}
    void Render() override;

private:
    BoostMaster* plugin;
};
