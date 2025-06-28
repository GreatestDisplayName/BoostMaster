#pragma once
#include <vector>
#include <string>
#include <algorithm>
#include "bakkesmod/wrappers/WrapperStructs.h"

// PadType for filtering and coloring
enum class PadType { Big, Small };

struct StaticBoostPad {
    Vector location;
    float amount; // 100 for big, 12 for small
    PadType type;
};

// Standard pad layout for all standard maps
// Big pads: corners and sides, Small pads: between big pads
static const std::vector<StaticBoostPad> StandardPads = {
    { Vector(-3584, 0, 70), 100, PadType::Big },
    { Vector(3584, 0, 70), 100, PadType::Big },
    { Vector(0, 5120, 70), 100, PadType::Big },
    { Vector(0, -5120, 70), 100, PadType::Big },
    { Vector(-2048, 2560, 70), 100, PadType::Big },
    { Vector(2048, 2560, 70), 100, PadType::Big },
    { Vector(-2048, -2560, 70), 100, PadType::Big },
    { Vector(2048, -2560, 70), 100, PadType::Big },
    { Vector(-2816, 2816, 70), 12, PadType::Small },
    { Vector(0, 2816, 70), 12, PadType::Small },
    { Vector(2816, 2816, 70), 12, PadType::Small },
    { Vector(-2816, 0, 70), 12, PadType::Small },
    { Vector(2816, 0, 70), 12, PadType::Small },
    { Vector(-2816, -2816, 70), 12, PadType::Small },
    { Vector(0, -2816, 70), 12, PadType::Small },
    { Vector(2816, -2816, 70), 12, PadType::Small },
};

// Hoops (Dunk House) pad layout
static const std::vector<StaticBoostPad> HoopsPads = {
    { Vector(-2048, 0, 70), 100, PadType::Big },
    { Vector(2048, 0, 70), 100, PadType::Big },
    { Vector(0, 2560, 70), 100, PadType::Big },
    { Vector(0, -2560, 70), 100, PadType::Big },
    { Vector(-1024, 1280, 70), 12, PadType::Small },
    { Vector(1024, 1280, 70), 12, PadType::Small },
    { Vector(-1024, -1280, 70), 12, PadType::Small },
    { Vector(1024, -1280, 70), 12, PadType::Small },
};

// Dropshot pad layout (no pads, but included for completeness)
static const std::vector<StaticBoostPad> DropshotPads = {};

// Snow Day (throwback stadium) pad layout
static const std::vector<StaticBoostPad> SnowDayPads = StandardPads;

// Rumble uses StandardPads

// Map lookup function with documentation for each map
inline const std::vector<StaticBoostPad>& GetStaticBoostPadsForMap(const std::string& mapName) {
    // Lowercase map name for robust matching
    std::string lowerMap = mapName;
    std::transform(lowerMap.begin(), lowerMap.end(), lowerMap.begin(), ::tolower);
    // All standard map names and variants (lowercase for comparison)
    static const std::vector<std::string> standardMaps = {
        "stadium_p", "stadium_p_day", "stadium_p_stormy", "stadium_p_night",
        "championsfield_p", "championsfield_p_night",
        "eurostadium_p", "eurostadium_p_night", "eurostadium_p_snowy",
        "trainstation_p", "trainstation_p_night", "trainstation_p_dawn",
        "utopiastadium_p", "utopiastadium_p_dusk", "utopiastadium_p_snowy",
        "beach_p", "beach_p_night", "beach_p_sunset",
        "neotokyo_standard_p", "neotokyo_standard_p_night",
        "haunted_trainstation_p", "chn_stadium_p", "chn_stadium_p_dusk",
        "arc_p", "arc_p_day",
        "wasteland_p", "wasteland_p_night",
        "farm_p", "farm_p_night", "farm_p_snowy",
        "aquadome_p",
        "deadeyecanyon_p", "deadeyecanyon_p_night",
        "sovereignheights_p",
        "estadiovida_p",
        "tokyounderpass_p", "pillars_p", "cosmic_p", "doublegoal_p", "octagon_p", "underpass_p", "utopiaretro_p", "throwbackstadium_p", "rally_p", "rallynight_p", "rallyday_p", "rallysnowy_p"
    };
    for (const auto& name : standardMaps) {
        if (lowerMap == name) return StandardPads;
    }
    // Hoops
    static const std::vector<std::string> hoopsMaps = {
        "hoopsstadium_p", "dunkhouse_p"
    };
    for (const auto& name : hoopsMaps) {
        if (lowerMap == name) return HoopsPads;
    }
    // Dropshot
    static const std::vector<std::string> dropshotMaps = {
        "dropshot_p", "dropshot_doublegoal_p"
    };
    for (const auto& name : dropshotMaps) {
        if (lowerMap == name) return DropshotPads;
    }
    // Snow Day
    static const std::vector<std::string> snowdayMaps = {
        "throwbackstadium_p", "snowystadium_p"
    };
    for (const auto& name : snowdayMaps) {
        if (lowerMap == name) return SnowDayPads;
    }
    static const std::vector<StaticBoostPad> empty;
    return empty;
}
