// nlohmann/json single-header library
// See: https://github.com/nlohmann/json
// Version: 3.11.2 (or latest)
//
// This is a placeholder. For full functionality, download the latest json.hpp from:
// https://github.com/nlohmann/json/releases
// and replace this file's contents with the official header.

#pragma once

#include <map>
#include <vector>
#include <string>
#include <initializer_list>
#include <type_traits>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace nlohmann {
    class json {
    public:
        // Minimal stub for demonstration. Replace with full header for real use.
        json() = default;
        json(std::initializer_list<std::pair<const std::string, float>> il) {
            for (const auto& p : il) data[p.first] = p.second;
        }
        float& operator[](const std::string& key) { return data[key]; }
        float value(const std::string& key, float def) const {
            auto it = data.find(key);
            return it != data.end() ? it->second : def;
        }
        friend std::ostream& operator<<(std::ostream& os, const json& j) {
            os << "{";
            bool first = true;
            for (const auto& p : j.data) {
                if (!first) os << ", ";
                os << '\"' << p.first << "\": " << p.second;
                first = false;
            }
            os << "}";
            return os;
        }
        friend std::istream& operator>>(std::istream& is, json& j) {
            // Not a real parser! Replace with official header for real use.
            std::string s;
            std::getline(is, s);
            // No-op stub
            return is;
        }
    private:
        std::map<std::string, float> data;
    };
}
