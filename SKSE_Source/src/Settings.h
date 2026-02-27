#pragma once

namespace HotSwap {
    class Settings {
    public:
        static Settings* GetSingleton();

        void Load();

        int GetMigrationDelayMs() const { return m_migrationDelayMs; }

    private:
        Settings() = default;

        int m_migrationDelayMs = 500;
    };
}
