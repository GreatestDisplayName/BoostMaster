#pragma once

#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include "bakkesmod/plugin/pluginwindow.h"

/*
 * GuiBase:
 * Simple base class for UI components.
 * Provides a virtual Render() method that derived classes should implement.
 */
class GuiBase
{
public:
    virtual ~GuiBase() = default;
    
    // Pure virtual function: Derived classes must implement this to render their content.
    virtual void Render() = 0;
};

/*
 * SettingsWindowBase:
 * Base class for plugin settings windows.
 * Provides a default plugin name and an interface for setting the ImGui context.
 */
class SettingsWindowBase : public BakkesMod::Plugin::PluginSettingsWindow
{
public:
    // Returns the plugin name.
    std::string GetPluginName() override;

    // Sets the active ImGui context.
    void SetImGuiContext(uintptr_t ctx) override;
};

/*
 * PluginWindowBase:
 * Base class for on-screen plugin windows.
 * Implements common functionality such as window state handling, input blocking,
 * and providing a menu name and title. Derived classes must implement RenderWindow()
 * to supply window-specific content.
 */
class PluginWindowBase : public BakkesMod::Plugin::PluginWindow
{
public:
    virtual ~PluginWindowBase() = default;

    // Whether the window is open.
    bool isWindowOpen_ = false;

    // The title shown in the window's title bar.
    std::string menuTitle_ = "BoostMaster";

    // Returns the name used to toggle this window (and used in commands).
    std::string GetMenuName() override;

    // Returns the title that is displayed at the top of the window.
    std::string GetMenuTitle() override;

    // Sets the ImGui context.
    void SetImGuiContext(uintptr_t ctx) override;

    // Returns true if input should be blocked by the window (i.e. if ImGui is capturing mouse or keyboard).
    bool ShouldBlockInput() override;

    // Indicates whether this window is considered an active overlay.
    bool IsActiveOverlay() override;

    // Called when the window is opened.
    void OnOpen() override;

    // Called when the window is closed.
    void OnClose() override;

    // Renders the window; this method is called by BakkesMod.
    void Render() override;

    // Pure virtual function: Derived classes must implement this to render window-specific content.
    virtual void RenderWindow() = 0;
};
