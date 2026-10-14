#define main mokthyn_game_entrypoint
#include "../Mokthyn.cpp"
#undef main

#include <fstream>
#include <iostream>

int main() {
    using namespace mokthyn;
    wchar_t tempPath[MAX_PATH]{};
    if (GetTempPathW(MAX_PATH, tempPath) == 0) return 1;
    std::wstring directory = std::wstring(tempPath) + L"MokthynSaveTest_" +
                             std::to_wstring(GetCurrentProcessId());
    CreateDirectoryW(directory.c_str(), nullptr);
    SetEnvironmentVariableW(L"APPDATA", directory.c_str());
    const std::wstring saveDirectory = directory + L"\\mokthyn";
    const std::wstring savePath = saveDirectory + L"\\save.dat";

    health = 73;
    ammo = 44;
    playerX = 17;
    playerY = 9;
    hasSavedRun = true;
    world = {};
    enemies = {{20, 12, BossHealth, true, true, true, Clock::now()}};
    if (!saveArchive()) return 2;

    std::ifstream encryptedFile(savePath, std::ios::binary);
    const std::string encrypted((std::istreambuf_iterator<char>(encryptedFile)),
                                std::istreambuf_iterator<char>());
    if (encrypted.compare(0, 14, "MOKTHYN_DPAPI\n", 14) != 0) return 3;

    health = 1;
    enemies.clear();
    if (!loadArchive() || health != 73 || ammo != 44 || enemies.size() != 1 ||
        !enemies.front().boss || enemies.front().hp != BossHealth)
        return 4;

    std::ostringstream legacy;
    legacy << "MOKTHYN_SAVE 3\n0 1 64 30 2 200 0 0 0 12 9 1 3\n";
    for (int y = 0; y < 18; ++y) {
        for (int x = 0; x < 24; ++x) {
            const bool edge = x == 0 || x == 23 || y == 0 || y == 17;
            legacy << (edge ? Wall : Floor) << ' ';
        }
        legacy << '\n';
    }
    legacy << "1\n14 9 45 1 0\n120\n1200 675\n";
    std::ofstream legacyFile(savePath, std::ios::binary | std::ios::trunc);
    legacyFile << legacy.str();
    legacyFile.close();

    if (!loadArchive() || playerX != 18 || playerY != 12 || enemies.size() != 1 ||
        enemies.front().x != 20 || enemies.front().y != 12)
        return 5;
    std::ifstream migratedFile(savePath, std::ios::binary);
    const std::string migrated((std::istreambuf_iterator<char>(migratedFile)),
                               std::istreambuf_iterator<char>());
    if (migrated.compare(0, 14, "MOKTHYN_DPAPI\n", 14) != 0) return 6;
    if (!loadArchive() || playerX != 18 || playerY != 12 || enemies.size() != 1)
        return 7;

    DeleteFileW(savePath.c_str());
    RemoveDirectoryW(saveDirectory.c_str());
    RemoveDirectoryW(directory.c_str());
    std::cout << "DPAPI save/load, boss persistence, legacy map migration: PASS\n";
    return 0;
}
