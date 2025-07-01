# BakkesMod SDK Guide

## What is BakkesMod?

BakkesMod is a popular mod for Rocket League that started as an enhancement for freeplay training but has evolved into a comprehensive modding platform. It provides:

- Enhanced freeplay features and custom training options
- Dollycam for cinematic replays
- Advanced statistics tracking
- Custom gamemodes and training scenarios
- Plugin system for community-developed features
- Replay analysis tools
- Goal replay POV features

## The BakkesMod SDK

The BakkesMod SDK (Software Development Kit) is an API that interfaces with Rocket League, allowing developers to create plugins that can:

- Manipulate game objects (cars, ball, goals, etc.)
- Hook into game events and functions
- Create custom UI elements with ImGui
- Access game data for statistics and analysis
- Modify game physics and behavior (in training/freeplay)
- Create custom training scenarios

### Key Features

- **C++ Based**: Primary development language
- **Python Support**: Alternative SDK available for Python development
- **Comprehensive API**: Access to virtually all game objects and systems
- **Event Hooks**: React to game events in real-time
- **ImGui Integration**: Create custom user interfaces
- **Console Commands**: Register custom console commands
- **Configuration Variables**: Persistent settings system
- **HTTP Wrapper**: Make web requests from plugins

## System Requirements

- **OS**: Windows 10/11 (Rocket League is Windows-only)
- **IDE**: Visual Studio 2022 (not VS Code)
- **Dependencies**: Microsoft Visual C++ Redistributables
- **BakkesMod**: Latest version installed

## Getting Started

### 1. Install Prerequisites

1. **Install BakkesMod**
   - Download from [bakkesmod.com](https://bakkesmod.com)
   - Run at least once to set up directories

2. **Install Visual Studio 2022**
   - Download from [visualstudio.microsoft.com](https://visualstudio.microsoft.com/)
   - Select "Desktop development with C++" workload during installation

3. **Install Visual C++ Redistributables**
   - Download both x64 and x86 versions from Microsoft
   - Required for proper plugin execution

### 2. Get the SDK

The SDK is automatically included with BakkesMod installation:
- Location: `%appdata%\bakkesmod\bakkesmod\bakkesmodsdk`
- Alternative: Clone from [GitHub](https://github.com/bakkesmodorg/BakkesModSDK)

### 3. Use the Template

For quick setup, use the official plugin template:
- Automatically generates project skeleton
- Includes proper build configuration
- Sets up deployment to BakkesMod plugins folder

## Basic Plugin Structure

### Plugin Header (.h file)
```cpp
#pragma once
#include "bakkesmod/plugin/bakkesmodplugin.h"

class MyPlugin : public BakkesMod::Plugin::BakkesModPlugin {
public:
    void onLoad() override;
    void onUnload() override;
    
    // Custom functions
    void myCustomFunction();
};
```

### Plugin Implementation (.cpp file)
```cpp
#include "pch.h"
#include "MyPlugin.h"

BAKKESMOD_PLUGIN(MyPlugin, "My Plugin", "1.0", PLUGINTYPE_FREEPLAY)

void MyPlugin::onLoad() {
    _globalCvarManager = cvarManager;
    
    // Register console command
    cvarManager->registerNotifier("my_command", [this](std::vector<std::string> args) {
        myCustomFunction();
    }, "", PERMISSION_ALL);
}

void MyPlugin::onUnload() {
    // Cleanup if needed
}

void MyPlugin::myCustomFunction() {
    if (!gameWrapper->IsInFreeplay()) return;
    
    ServerWrapper server = gameWrapper->GetCurrentGameState();
    if (!server) return;
    
    BallWrapper ball = server.GetBall();
    if (!ball) return;
    
    CarWrapper car = gameWrapper->GetLocalCar();
    if (!car) return;
    
    // Example: Put ball on top of car
    Vector carLocation = car.GetLocation();
    float ballRadius = ball.GetRadius();
    ball.SetLocation(carLocation + Vector{0, 0, ballRadius * 2});
}
```

## Key SDK Components

### 1. Game Wrappers
- **ServerWrapper**: Access to game state, players, ball, goals
- **CarWrapper**: Car-specific data and controls
- **BallWrapper**: Ball physics and properties
- **PlayerReplicationInfoWrapper**: Player information
- **GameEventWrapper**: Match/game event data

### 2. Utilities
- **CVarManager**: Configuration variables and console commands
- **GameWrapper**: Game state queries and local player access
- **CanvasWrapper**: Drawing and rendering
- **HttpWrapper**: HTTP requests

### 3. Event Hooks
- Function hooks for game events
- Event notifications (goals, demos, etc.)
- Stat tracking events

### 4. ImGui Integration
- Custom UI creation
- Settings panels
- Real-time data display
- Interactive controls

## Common Plugin Types

### 1. Training Plugins
- Custom training scenarios
- Ball/car manipulation
- Physics modifications
- Shot analysis

### 2. Statistics Plugins
- Performance tracking
- Match analysis
- Skill progression
- Data visualization

### 3. Visual Enhancement Plugins
- Custom overlays
- Information displays
- Camera modifications
- Replay enhancements

### 4. Utility Plugins
- Quality of life improvements
- Automation tools
- Configuration helpers
- Integration with external services

## Development Best Practices

### Safety and Validation
```cpp
// Always null-check wrappers
if (!gameWrapper->IsInGame()) return;

ServerWrapper server = gameWrapper->GetCurrentGameState();
if (!server) return;

BallWrapper ball = server.GetBall();
if (!ball) return;
```

### Plugin Permissions
- Use appropriate permission levels
- `PERMISSION_ALL` for general use
- `PERMISSION_FREEPLAY` for training-only features
- `PERMISSION_OFFLINE` for offline-only features

### Resource Management
- Clean up in `onUnload()`
- Unregister events and commands
- BakkesMod handles most cleanup automatically

## Building and Deployment

### Build Process
1. Build in Visual Studio (Ctrl+B)
2. Plugin DLL is automatically created
3. Template copies to BakkesMod plugins folder

### Loading Plugins
```
// In Rocket League console (F6)
plugin load PluginName
plugin unload PluginName

// List loaded plugins
plugin list
```

### Plugin Manager
- Access via F2 → Plugins tab
- Enable/disable plugins
- Install from bakkesplugins.com
- Manage plugin settings

## Resources

### Official Documentation
- [BakkesMod Programming Wiki](https://wiki.bakkesplugins.com/)
- [GitHub SDK Repository](https://github.com/bakkesmodorg/BakkesModSDK)
- [Plugin Template](https://github.com/ubelhj/BakkesModStarterPlugin)

### Community
- [BakkesMod Programming Discord](https://discord.gg/bakkesplugins)
- [Plugin Repository](https://bakkesplugins.com/)
- [Reddit Community](https://reddit.com/r/bakkesmod)

### API Reference
- Comprehensive class documentation in wiki
- Function scanner for reverse engineering
- Code snippets and examples
- Best practices guide

## Alternative: Python SDK

For developers preferring Python:
- [BakkesModSDK-Python](https://github.com/Stanbroek/BakkesModSDK-Python)
- Python bindings via pybind11
- Similar functionality to C++ SDK
- Easier for rapid prototyping

## Security and Guidelines

### Safe Development
- Only install plugins from official sources
- Verified plugins on bakkesplugins.com
- Never use plugins from unofficial sources
- Report suspicious behavior

### Plugin Submission
- Submit to bakkesplugins.com for verification
- Follow community guidelines
- Open source recommended
- Proper documentation required

## Troubleshooting

### Common Issues
- Antivirus false positives (whitelist BakkesMod folder)
- Missing Visual C++ redistributables
- Injection failures (run as administrator)
- Plugin crashes (check null pointers)

### Debug Tools
- Live debugging support
- Crash dump analysis
- Console logging
- Plugin exclusive logging

## Conclusion

The BakkesMod SDK provides a powerful platform for enhancing Rocket League through custom plugins. Whether you're creating training tools, statistics trackers, or visual enhancements, the SDK offers comprehensive access to game systems while maintaining safety and stability.

The active community, extensive documentation, and robust tooling make it an excellent choice for modding Rocket League and creating innovative gameplay enhancements.