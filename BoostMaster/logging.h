// ReSharper disable CppNonExplicitConvertingConstructor
#pragma once
#include <string>
#include <source_location>
#include <format>
#include <memory>
#include <fstream>
#include <iostream>
#include <ctime>

#include "bakkesmod/wrappers/cvarmanagerwrapper.h"

extern std::shared_ptr<CVarManagerWrapper> _globalCvarManager;
constexpr bool DEBUG_LOG = true;

struct FormatString
{
    std::string_view str;
    std::source_location loc{};

    FormatString(const char* s, const std::source_location& l = std::source_location::current())
        : str(s), loc(l) {
    }
    FormatString(const std::string&& s, const std::source_location& l = std::source_location::current())
        : str(s), loc(l) {
    }

    [[nodiscard]] std::string GetLocation() const
    {
        return std::format("[{} ({}:{})]", loc.function_name(), loc.file_name(), loc.line());
    }
};

struct FormatWstring
{
    std::wstring_view str;
    std::source_location loc{};

    FormatWstring(const wchar_t* s, const std::source_location& l = std::source_location::current())
        : str(s), loc(l) {
    }
    FormatWstring(const std::wstring&& s, const std::source_location& l = std::source_location::current())
        : str(s), loc(l) {
    }

    [[nodiscard]] std::wstring GetLocation() const
    {
        auto bs = std::format("[{} ({}:{})]", loc.function_name(), loc.file_name(), loc.line());
        return std::wstring(bs.begin(), bs.end());
    }
};

// Log to BakkesMod console
template <typename... Args>
void LOG(std::string_view fmt, Args&&... args)
{
    _globalCvarManager->log(std::vformat(fmt, std::make_format_args(args...)));
}

template <typename... Args>
void LOG(std::wstring_view fmt, Args&&... args)
{
    _globalCvarManager->log(std::vformat(fmt, std::make_wformat_args(args...)));
}

// Log error to both console and file
inline void LOG_ERROR(const std::string& msg, const std::string& file = "error.log")
{
    _globalCvarManager->log("[ERROR] " + msg);
    std::cerr << "[ERROR] " << msg << std::endl;
    std::ofstream out(file, std::ios::app);
    if (out) {
        std::time_t t = std::time(nullptr);
        char buf[64]{};
        if (std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t))) {
            out << "[" << buf << "] [ERROR] " << msg << std::endl;
        }
        else {
            out << "[ERROR] " << msg << std::endl;
        }
    }
}

// Debug log with location
template <typename... Args>
void DEBUGLOG(const FormatString& fs, Args&&... args)
{
    if constexpr (DEBUG_LOG) {
        auto text = std::vformat(fs.str, std::make_format_args(args...));
        auto loc = fs.GetLocation();
        _globalCvarManager->log(std::format("{} {}", text, loc));
    }
}

template <typename... Args>
void DEBUGLOG(const FormatWstring& fs, Args&&... args)
{
    if constexpr (DEBUG_LOG) {
        auto text = std::vformat(fs.str, std::make_wformat_args(args...));
        auto loc = fs.GetLocation();
        _globalCvarManager->log(std::format(L"{} {}", text, loc));
    }
}
