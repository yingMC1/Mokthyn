#define main mokthyn_game_entrypoint
#include "../Mokthyn.cpp"
#undef main

#include <filesystem>
#include <fstream>
#include <iostream>
#include <queue>

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
    const auto readSave = [&savePath]() {
        std::ifstream file(std::filesystem::path(savePath), std::ios::binary);
        return std::string((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
    };

    startRun();
    const auto boss = std::find_if(enemies.begin(), enemies.end(),
                                   [](const Enemy& enemy) { return enemy.boss; });
    if (MapColumns != 36 || MapRows != 24 || boss == enemies.end() ||
        boss->hp != BossHealth || totalEnemies != static_cast<int>(enemies.size()) ||
        world[MapRows - 2][MapColumns - 2] != ExitTile)
        return 2;
    std::array<std::array<bool, MapColumns>, MapRows> reached{};
    std::queue<std::pair<int, int>> pending;
    pending.push({playerX, playerY});
    reached[playerY][playerX] = true;
    while (!pending.empty()) {
        const auto [x, y] = pending.front();
        pending.pop();
        for (const auto& step : std::array<std::pair<int, int>, 4>{
                 {{1, 0}, {-1, 0}, {0, 1}, {0, -1}}}) {
            const int nextX = x + step.first;
            const int nextY = y + step.second;
            if (nextX >= 0 && nextX < MapColumns && nextY >= 0 && nextY < MapRows &&
                world[nextY][nextX] != Wall && !reached[nextY][nextX]) {
                reached[nextY][nextX] = true;
                pending.push({nextX, nextY});
            }
        }
    }
    if (!reached[MapRows - 2][MapColumns - 2]) return 3;

    health = 73;
    ammo = 44;
    playerX = 17;
    playerY = 9;
    hasSavedRun = true;
    world = {};
    enemies = {{20, 12, BossHealth, true, true, true, Clock::now()}};
    if (!saveArchive()) return 4;
    if (readSave().compare(0, 14, "MOKTHYN_DPAPI\n", 14) != 0) return 5;

    health = 1;
    enemies.clear();
    if (!loadArchive() || health != 73 || ammo != 44 || enemies.size() != 1 ||
        !enemies.front().boss || enemies.front().hp != BossHealth)
        return 6;

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
    std::ofstream legacyFile(std::filesystem::path(savePath),
                             std::ios::binary | std::ios::trunc);
    legacyFile << legacy.str();
    legacyFile.close();

    if (!loadArchive() || playerX != 18 || playerY != 12 || enemies.size() != 1 ||
        enemies.front().x != 20 || enemies.front().y != 12)
        return 7;
    if (readSave().compare(0, 14, "MOKTHYN_DPAPI\n", 14) != 0) return 8;
    if (!loadArchive() || playerX != 18 || playerY != 12 || enemies.size() != 1)
        return 9;

    DeleteFileW(savePath.c_str());
    RemoveDirectoryW(saveDirectory.c_str());
    RemoveDirectoryW(directory.c_str());
    std::cout << "Map/Boss/path, DPAPI roundtrip, legacy migration: PASS\n";
    return 0;
}
