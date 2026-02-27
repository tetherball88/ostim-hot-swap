#include "Settings.h"
#include <Windows.h>
#include <filesystem>
#include <SKSE/SKSE.h>

namespace {
    std::filesystem::path GetIniPath() {
        wchar_t buf[MAX_PATH];
        HMODULE hm = nullptr;
        GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&HotSwap::Settings::GetSingleton), &hm);
        GetModuleFileNameW(hm, buf, MAX_PATH);
        return std::filesystem::path(buf).replace_extension(L".ini");
    }
}

namespace HotSwap {
    Settings* Settings::GetSingleton() {
        static Settings instance;
        return &instance;
    }

    void Settings::Load() {
        auto iniPath = GetIniPath();
        m_migrationDelayMs = static_cast<int>(
            ::GetPrivateProfileIntW(L"General", L"MigrationDelayMs", 500, iniPath.c_str()));
        SKSE::log::info("Settings loaded from {}: MigrationDelayMs={}", iniPath.string(), m_migrationDelayMs);
    }
}
