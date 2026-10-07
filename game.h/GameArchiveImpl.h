#pragma once

#include "GameArchive.h"
#include "GameState.h"
#include <windows.h>
#include <algorithm>
#include <sstream>
#include <string>
#include <utility>

namespace mokthyn {
namespace {
std::wstring archivePath() {
    wchar_t appData[MAX_PATH]{};
    const DWORD length = GetEnvironmentVariableW(L"APPDATA", appData, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) return {};
    return std::wstring(appData) + L"\\mokthyn\\save.dat";
}

bool validArchiveValue(int value, int low, int high) { return value >= low && value <= high; }
} // namespace

bool loadArchive() {
    const std::wstring path = archivePath();
    if (path.empty()) {
        archiveStatus = L"无法读取 APPDATA 环境变量，存档功能不可用";
        return false;
    }

    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        const DWORD win32Error = GetLastError();
        if (win32Error == ERROR_FILE_NOT_FOUND || win32Error == ERROR_PATH_NOT_FOUND) return true;
        archiveStatus = L"读取存档失败";
        return false;
    }

    const DWORD size = GetFileSize(file, nullptr);
    if (size == INVALID_FILE_SIZE || size > 65536) {
        CloseHandle(file);
        archiveStatus = L"存档文件大小无效，已使用默认数据";
        return false;
    }
    std::string contents(size, '\0');
    DWORD bytesRead = 0;
    const bool readOk = size == 0 || (ReadFile(file, &contents[0], size, &bytesRead, nullptr) &&
                                      bytesRead == size);
    CloseHandle(file);
    if (!readOk) {
        archiveStatus = L"读取存档失败";
        return false;
    }

    std::istringstream input(contents);
    std::string signature;
    int version = 0;
    int storedEnglish = 0, savedRun = 0, storedHealth = 0, storedAmmo = 0, storedMedkits = 0;
    int storedMoney = 0, storedArmorTier = 0, storedArmorDurability = 0, storedArmorOn = 0;
    int storedPlayerX = 0, storedPlayerY = 0, storedKills = 0, storedTotalEnemies = 0;
    if (!(input >> signature >> version >> storedEnglish >> savedRun >> storedHealth >> storedAmmo >>
          storedMedkits >> storedMoney >> storedArmorTier >> storedArmorDurability >> storedArmorOn >>
          storedPlayerX >> storedPlayerY >> storedKills >> storedTotalEnemies) ||
        signature != "MOKTHYN_SAVE" || (version < 1 || version > 3) ||
        !validArchiveValue(storedEnglish, 0, 1) ||
        !validArchiveValue(savedRun, 0, 1) || !validArchiveValue(storedHealth, 0, 100) ||
        !validArchiveValue(storedAmmo, 0, 100000) || !validArchiveValue(storedMedkits, 0, 10000) ||
        !validArchiveValue(storedMoney, 0, 100000000) ||
        !validArchiveValue(storedArmorTier, 0, 4) ||
        !validArchiveValue(storedArmorDurability, 0, 125) ||
        !validArchiveValue(storedArmorOn, 0, 1) ||
        !validArchiveValue(storedPlayerX, 1, MapColumns - 2) ||
        !validArchiveValue(storedPlayerY, 1, MapRows - 2) ||
        !validArchiveValue(storedKills, 0, 100000) ||
        !validArchiveValue(storedTotalEnemies, 0, 100000)) {
        archiveStatus = L"存档格式无效，已使用默认数据";
        return false;
    }

    std::array<std::array<int, MapColumns>, MapRows> storedWorld{};
    for (auto& row : storedWorld) {
        for (int& tile : row) {
            if (!(input >> tile) || !validArchiveValue(tile, Floor, MedkitPickup)) {
                archiveStatus = L"存档地图数据无效，已使用默认数据";
                return false;
            }
        }
    }

    int enemyCount = 0;
    if (!(input >> enemyCount) || !validArchiveValue(enemyCount, 0, 30)) {
        archiveStatus = L"存档敌人数据无效，已使用默认数据";
        return false;
    }
    std::vector<Enemy> storedEnemies;
    storedEnemies.reserve(static_cast<size_t>(enemyCount));
    for (int i = 0; i < enemyCount; ++i) {
        Enemy enemy;
        int alive = 0, awake = 0;
        if (!(input >> enemy.x >> enemy.y >> enemy.hp >> alive >> awake) ||
            !validArchiveValue(enemy.x, 1, MapColumns - 2) ||
            !validArchiveValue(enemy.y, 1, MapRows - 2) ||
            !validArchiveValue(enemy.hp, 0, EnemyHealth) || !validArchiveValue(alive, 0, 1) ||
            !validArchiveValue(awake, 0, 1)) {
            archiveStatus = L"存档敌人数据无效，已使用默认数据";
            return false;
        }
        enemy.alive = alive != 0;
        enemy.awake = awake != 0;
        storedEnemies.push_back(enemy);
    }
    int storedUiScalePercent = 120;
    int storedWindowWidth = DefaultWindowWidth;
    int storedWindowHeight = DefaultWindowHeight;
    if (version >= 2 &&
        (!(input >> storedUiScalePercent) ||
         !validArchiveValue(storedUiScalePercent, MinUiScalePercent, MaxUiScalePercent) ||
         (storedUiScalePercent - MinUiScalePercent) % UiScaleStepPercent != 0)) {
        archiveStatus = L"存档中的界面比例无效";
        return false;
    }
    if (version >= 3 &&
        (!(input >> storedWindowWidth >> storedWindowHeight) ||
         !validArchiveValue(storedWindowWidth, MinWindowWidth, MaxWindowWidth) ||
         (storedWindowWidth - MinWindowWidth) % WindowWidthStep != 0 ||
         !validArchiveValue(storedWindowHeight, MinWindowHeight, MaxWindowHeight) ||
         (storedWindowHeight - MinWindowHeight) % WindowHeightStep != 0)) {
        archiveStatus = L"存档中的窗口尺寸无效";
        return false;
    }

    english = storedEnglish != 0;
    uiScalePercent = storedUiScalePercent;
    windowWidth = storedWindowWidth;
    windowHeight = storedWindowHeight;
    hasSavedRun = savedRun != 0;
    health = storedHealth;
    ammo = storedAmmo;
    medkits = storedMedkits;
    money = storedMoney;
    armorTier = storedArmorTier;
    armorDurability = storedArmorDurability;
    armorOn = storedArmorOn != 0 && armorTier > 0 && armorDurability > 0;
    playerX = storedPlayerX;
    playerY = storedPlayerY;
    kills = storedKills;
    totalEnemies = storedTotalEnemies;
    world = storedWorld;
    enemies = std::move(storedEnemies);
    healing = extracting = false;
    if (hasSavedRun) screen = Screen::Menu;
    return true;
}

void beginNetworkArchiveSession() {
    if (networkRun) return;
    networkArchiveSnapshot.world = world;
    networkArchiveSnapshot.enemies = enemies;
    networkArchiveSnapshot.playerX = playerX;
    networkArchiveSnapshot.playerY = playerY;
    networkArchiveSnapshot.health = health;
    networkArchiveSnapshot.ammo = ammo;
    networkArchiveSnapshot.medkits = medkits;
    networkArchiveSnapshot.money = money;
    networkArchiveSnapshot.armorTier = armorTier;
    networkArchiveSnapshot.armorDurability = armorDurability;
    networkArchiveSnapshot.kills = kills;
    networkArchiveSnapshot.totalEnemies = totalEnemies;
    networkArchiveSnapshot.uiScalePercent = uiScalePercent;
    networkArchiveSnapshot.windowWidth = windowWidth;
    networkArchiveSnapshot.windowHeight = windowHeight;
    networkArchiveSnapshot.english = english;
    networkArchiveSnapshot.hasSavedRun = hasSavedRun;
    networkArchiveSnapshot.armorOn = armorOn;
    networkArchiveSnapshot.valid = true;
    networkRun = true;
}

void restoreNetworkArchiveSession() {
    if (!networkRun || !networkArchiveSnapshot.valid) return;
    world = networkArchiveSnapshot.world;
    enemies = networkArchiveSnapshot.enemies;
    playerX = networkArchiveSnapshot.playerX;
    playerY = networkArchiveSnapshot.playerY;
    health = networkArchiveSnapshot.health;
    ammo = networkArchiveSnapshot.ammo;
    medkits = networkArchiveSnapshot.medkits;
    money = networkArchiveSnapshot.money;
    armorTier = networkArchiveSnapshot.armorTier;
    armorDurability = networkArchiveSnapshot.armorDurability;
    kills = networkArchiveSnapshot.kills;
    totalEnemies = networkArchiveSnapshot.totalEnemies;
    uiScalePercent = networkArchiveSnapshot.uiScalePercent;
    windowWidth = networkArchiveSnapshot.windowWidth;
    windowHeight = networkArchiveSnapshot.windowHeight;
    english = networkArchiveSnapshot.english;
    hasSavedRun = networkArchiveSnapshot.hasSavedRun;
    armorOn = networkArchiveSnapshot.armorOn;
    healing = extracting = false;
    won = false;
    networkRun = false;
    networkArchiveSnapshot.valid = false;
    screen = Screen::Menu;
    lastArchiveSave = Clock::now();
}

bool saveArchive() {
    if (networkActive || networkRun) {
        lastArchiveSave = Clock::now();
        return true;
    }
    lastArchiveSave = Clock::now();
    const std::wstring path = archivePath();
    if (path.empty()) {
        archiveStatus = L"无法读取 APPDATA 环境变量，存档功能不可用";
        return false;
    }
    const size_t separator = path.find_last_of(L'\\');
    const std::wstring directory = path.substr(0, separator);
    if (!CreateDirectoryW(directory.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) {
        archiveStatus = L"无法创建存档目录";
        return false;
    }

    const int safePlayerX = std::clamp(playerX, 1, MapColumns - 2);
    const int safePlayerY = std::clamp(playerY, 1, MapRows - 2);
    const int safeArmorTier = std::clamp(armorTier, 0, 4);
    const int safeArmorDurability = std::clamp(armorDurability, 0, ArmorDurability[safeArmorTier]);
    const int safeEnemyCount = std::clamp(static_cast<int>(enemies.size()), 0, 9);
    std::ostringstream output;
    output << "MOKTHYN_SAVE 3\n" << (english ? 1 : 0) << ' ' << (hasSavedRun ? 1 : 0) << ' '
           << std::clamp(health, 0, 100) << ' ' << std::clamp(ammo, 0, 100000) << ' '
           << std::clamp(medkits, 0, 10000) << ' ' << std::clamp(money, 0, 100000000) << ' '
           << safeArmorTier << ' ' << safeArmorDurability << ' ' << (armorOn ? 1 : 0) << ' '
           << safePlayerX << ' ' << safePlayerY << ' ' << std::clamp(kills, 0, 100000) << ' '
           << std::clamp(totalEnemies, 0, 100000) << '\n';
    for (const auto& row : world) {
        for (int tile : row) output << std::clamp(tile, static_cast<int>(Floor), static_cast<int>(MedkitPickup)) << ' ';
        output << '\n';
    }
    output << safeEnemyCount << '\n';
    int writtenEnemies = 0;
    for (const Enemy& enemy : enemies) {
        if (writtenEnemies >= safeEnemyCount) break;
        const int safeX = std::clamp(enemy.x, 1, MapColumns - 2);
        const int safeY = std::clamp(enemy.y, 1, MapRows - 2);
        const int safeHp = std::clamp(enemy.hp, 0, EnemyHealth);
        output << safeX << ' ' << safeY << ' ' << safeHp << ' ' << (enemy.alive ? 1 : 0) << ' '
               << (enemy.awake ? 1 : 0) << '\n';
        ++writtenEnemies;
    }
    output << uiScalePercent << '\n';
    output << windowWidth << ' ' << windowHeight << '\n';
    const std::string contents = output.str();

    const std::wstring temporaryPath = path + L".tmp";
    HANDLE file = CreateFileW(temporaryPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        archiveStatus = L"无法写入存档文件";
        return false;
    }
    DWORD bytesWritten = 0;
    const bool writeOk = WriteFile(file, contents.data(), static_cast<DWORD>(contents.size()),
                                   &bytesWritten, nullptr) &&
                         bytesWritten == contents.size() && FlushFileBuffers(file);
    CloseHandle(file);
    if (!writeOk ||
        !MoveFileExW(temporaryPath.c_str(), path.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileW(temporaryPath.c_str());
        archiveStatus = L"保存存档失败";
        return false;
    }
    archiveStatus.clear();
    return true;
}
} // namespace mokthyn
