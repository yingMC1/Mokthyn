#pragma once

#include "GameArchive.h"
#include "GameState.h"
#include <algorithm>
#include <charconv>
#include <dpapi.h>
#include <sstream>
#include <string>
#include <utility>
#include <windows.h>

namespace mokthyn {
namespace {
std::wstring archivePath() {
    wchar_t appData[MAX_PATH]{};
    const DWORD length = GetEnvironmentVariableW(L"APPDATA", appData, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) return {};
    return std::wstring(appData) + L"\\mokthyn\\save.dat";
}

bool validArchiveValue(int value, int low, int high) {
    return value >= low && value <= high;
}

bool readArchiveInt(std::istream &input, int &value) {
    std::string token;
    if (!(input >> token)) return false;
    const auto result =
        std::from_chars(token.data(), token.data() + token.size(), value);
    return result.ec == std::errc{} && result.ptr == token.data() + token.size();
}

bool loadVersion5Archive(std::istream &input, int version) {
    std::string marker;
    int storedEnglish = english ? 1 : 0;
    int storedUiScalePercent = uiScalePercent;
    int storedWindowWidth = windowWidth;
    int storedWindowHeight = windowHeight;
    if (!(input >> marker) || marker != "SETTINGS") {
        archiveStatus = L"存档设置区块无效";
        return false;
    }

    bool validSettings = true;
    int value = 0;
    if (readArchiveInt(input, value) && validArchiveValue(value, 0, 1))
        storedEnglish = value;
    else validSettings = false;
    if (readArchiveInt(input, value) &&
        validArchiveValue(value, MinUiScalePercent, MaxUiScalePercent) &&
        (value - MinUiScalePercent) % UiScaleStepPercent == 0)
        storedUiScalePercent = value;
    else validSettings = false;
    if (readArchiveInt(input, value) &&
        validArchiveValue(value, MinWindowWidth, MaxWindowWidth) &&
        (value - MinWindowWidth) % WindowWidthStep == 0)
        storedWindowWidth = value;
    else validSettings = false;
    if (readArchiveInt(input, value) &&
        validArchiveValue(value, MinWindowHeight, MaxWindowHeight) &&
        (value - MinWindowHeight) % WindowHeightStep == 0)
        storedWindowHeight = value;
    else validSettings = false;

    english = storedEnglish != 0;
    uiScalePercent = storedUiScalePercent;
    windowWidth = storedWindowWidth;
    windowHeight = storedWindowHeight;

    if (!(input >> marker) || marker != "RUN") {
        archiveStatus = L"存档行动区块无效，已保留显示设置";
        return false;
    }
    int savedRun = 0, storedHealth = 0, storedAmmo = 0, storedMedkits = 0,
        storedMoney = 0;
    int storedArmorTier = 0, storedArmorDurability = 0, storedArmorOn = 0;
    int storedPlayerX = 0, storedPlayerY = 0, storedKills = 0,
        storedTotalEnemies = 0;
    if (!(readArchiveInt(input, savedRun) && readArchiveInt(input, storedHealth) &&
          readArchiveInt(input, storedAmmo) &&
          readArchiveInt(input, storedMedkits) &&
          readArchiveInt(input, storedMoney) &&
          readArchiveInt(input, storedArmorTier) &&
          readArchiveInt(input, storedArmorDurability) &&
          readArchiveInt(input, storedArmorOn) &&
          readArchiveInt(input, storedPlayerX) &&
          readArchiveInt(input, storedPlayerY) &&
          readArchiveInt(input, storedKills) &&
          readArchiveInt(input, storedTotalEnemies)) ||
        !validArchiveValue(savedRun, 0, 1) ||
        !validArchiveValue(storedHealth, 0, MaxHealth) ||
        !validArchiveValue(storedAmmo, 0, 100000) ||
        !validArchiveValue(storedMedkits, 0, 10000) ||
        !validArchiveValue(storedMoney, 0, 100000000) ||
        !validArchiveValue(storedArmorTier, 0, 4) ||
        !validArchiveValue(storedArmorDurability, 0,
                           ArmorDurability[storedArmorTier]) ||
        !validArchiveValue(storedArmorOn, 0, 1) ||
        !validArchiveValue(storedPlayerX, 1, MapColumns - 2) ||
        !validArchiveValue(storedPlayerY, 1, MapRows - 2) ||
        !validArchiveValue(storedKills, 0, 100000) ||
        !validArchiveValue(storedTotalEnemies, 0, 100000)) {
        archiveStatus = L"存档行动数据无效，已保留显示设置";
        return false;
    }

    if (!(input >> marker) || marker != "MAP") {
        archiveStatus = L"存档地图区块无效，已保留显示设置";
        return false;
    }
    std::array<std::array<int, MapColumns>, MapRows> storedWorld{};
    for (auto &row : storedWorld) {
        for (int &tile : row) {
            if (!readArchiveInt(input, tile) ||
                !validArchiveValue(tile, Floor, MedkitPickup)) {
                archiveStatus = L"存档地图数据无效，已保留显示设置";
                return false;
            }
        }
    }

    if (!(input >> marker) || marker != "ENEMIES") {
        archiveStatus = L"存档敌人区块无效，已保留显示设置";
        return false;
    }
    int enemyCount = 0;
    if (!readArchiveInt(input, enemyCount) ||
        !validArchiveValue(enemyCount, 0, MaxEnemyCount)) {
        archiveStatus = L"存档敌人数据无效，已保留显示设置";
        return false;
    }
    std::vector<Enemy> storedEnemies;
    storedEnemies.reserve(static_cast<size_t>(enemyCount));
    for (int i = 0; i < enemyCount; ++i) {
        Enemy enemy;
        int alive = 0, awake = 0, boss = 0;
        int cooldownRemaining = 0;
        if (!readArchiveInt(input, enemy.x) || !readArchiveInt(input, enemy.y) ||
            !readArchiveInt(input, enemy.hp) || !readArchiveInt(input, alive) ||
            !readArchiveInt(input, awake) || !readArchiveInt(input, boss) ||
            (version >= 6 && !readArchiveInt(input, cooldownRemaining)) ||
            !validArchiveValue(enemy.x, 1, MapColumns - 2) ||
            !validArchiveValue(enemy.y, 1, MapRows - 2) ||
            !validArchiveValue(enemy.hp, 0, boss != 0 ? BossHealth : EnemyHealth) ||
            !validArchiveValue(alive, 0, 1) || !validArchiveValue(awake, 0, 1) ||
            !validArchiveValue(boss, 0, 1) ||
            !validArchiveValue(cooldownRemaining, 0, EnemyAttackIntervalMs)) {
            archiveStatus = L"存档敌人数据无效，已保留显示设置";
            return false;
        }
        enemy.alive = alive != 0;
        enemy.awake = awake != 0;
        enemy.boss = boss != 0;
        enemy.nextAttack =
            Clock::now() + std::chrono::milliseconds(cooldownRemaining);
        storedEnemies.push_back(enemy);
    }

    hasSavedRun = savedRun != 0;
    health = storedHealth;
    ammo = storedAmmo;
    magazineAmmo = 0;
    reloading = false;
    weaponMode = WeaponMode::Gun;
    nextMelee = {};
    if (hasSavedRun) {
        magazineAmmo = std::min(MagazineSize, ammo);
        ammo -= magazineAmmo;
    }
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
    if (!validSettings) archiveStatus = L"存档显示设置有损坏，已保留未损坏的设置";
    return true;
}

constexpr char EncryptedArchiveHeader[] = "MOKTHYN_DPAPI\n";

bool protectArchive(const std::string &plainText, std::string &encrypted) {
    DATA_BLOB input{};
    input.pbData = reinterpret_cast<BYTE *>(const_cast<char *>(plainText.data()));
    input.cbData = static_cast<DWORD>(plainText.size());
    DATA_BLOB output{};
    if (!CryptProtectData(&input, L"Mokthyn save", nullptr, nullptr, nullptr,
                          CRYPTPROTECT_UI_FORBIDDEN, &output))
        return false;
    encrypted.assign(EncryptedArchiveHeader, sizeof(EncryptedArchiveHeader) - 1);
    encrypted.append(reinterpret_cast<const char *>(output.pbData), output.cbData);
    LocalFree(output.pbData);
    return true;
}

bool unprotectArchive(const std::string &encrypted, std::string &plainText) {
    const size_t headerSize = sizeof(EncryptedArchiveHeader) - 1;
    if (encrypted.size() <= headerSize) return false;
    DATA_BLOB input{};
    input.pbData =
        reinterpret_cast<BYTE *>(const_cast<char *>(encrypted.data() + headerSize));
    input.cbData = static_cast<DWORD>(encrypted.size() - headerSize);
    DATA_BLOB output{};
    if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr,
                            CRYPTPROTECT_UI_FORBIDDEN, &output))
        return false;
    plainText.assign(reinterpret_cast<const char *>(output.pbData), output.cbData);
    LocalFree(output.pbData);
    return true;
}
}    // namespace

inline unsigned long long lastSavedArchiveHash = ~0ULL;

// 自动存档脏检测：把会写进存档的状态压成 64 位哈希，与上次成功保存一致就跳过整次
// 磁盘写（DPAPI 加密 + FlushFileBuffers + 备份复制每 2 秒一次，跳过可消除暂停/挂机时的
// 周期卡顿）。刻意排除敌人攻击计时 nextAttack：它随挂钟变化，纳入会让定时存档永远判
// “脏”；它落盘时本就按 0..EnemyAttackIntervalMs 钳制，读档后最多让敌人晚一点攻击。
inline unsigned long long archiveStateHash() {
    unsigned long long hash = 1469598103934665603ULL;
    const auto mix = [&hash](unsigned long long value) {
        hash ^= value;
        hash *= 1099511628211ULL;
    };
    mix(static_cast<unsigned long long>(playerX));
    mix(static_cast<unsigned long long>(playerY));
    mix(static_cast<unsigned long long>(health));
    // 存档里存的是弹药总量（ammo + magazineAmmo），换弹在两者间搬运不该触发写盘
    mix(static_cast<unsigned long long>(ammo + magazineAmmo));
    mix(static_cast<unsigned long long>(medkits));
    mix(static_cast<unsigned long long>(money));
    mix(static_cast<unsigned long long>(armorTier));
    mix(static_cast<unsigned long long>(armorDurability));
    mix(static_cast<unsigned long long>(armorOn));
    mix(static_cast<unsigned long long>(kills));
    mix(static_cast<unsigned long long>(totalEnemies));
    mix(static_cast<unsigned long long>(hasSavedRun));
    mix(static_cast<unsigned long long>(english));
    mix(static_cast<unsigned long long>(uiScalePercent));
    mix(static_cast<unsigned long long>(windowWidth));
    mix(static_cast<unsigned long long>(windowHeight));
    for (const auto &row : world)
        for (const int tile : row) mix(static_cast<unsigned long long>(tile));
    mix(static_cast<unsigned long long>(enemies.size()));
    for (const Enemy &enemy : enemies) {
        mix(static_cast<unsigned long long>(enemy.x));
        mix(static_cast<unsigned long long>(enemy.y));
        mix(static_cast<unsigned long long>(enemy.hp));
        mix(static_cast<unsigned long long>(enemy.alive));
        mix(static_cast<unsigned long long>(enemy.awake));
        mix(static_cast<unsigned long long>(enemy.boss));
    }
    return hash;
}

bool loadArchiveFromPath(const std::wstring &path) {
    if (path.empty()) {
        archiveStatus = L"无法读取 APPDATA 环境变量，存档功能不可用";
        return false;
    }

    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        const DWORD win32Error = GetLastError();
        if (win32Error == ERROR_FILE_NOT_FOUND || win32Error == ERROR_PATH_NOT_FOUND)
            return true;
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
    const bool readOk =
        size == 0 || (ReadFile(file, &contents[0], size, &bytesRead, nullptr) &&
                      bytesRead == size);
    CloseHandle(file);
    if (!readOk) {
        archiveStatus = L"读取存档失败";
        return false;
    }

    const size_t headerSize = sizeof(EncryptedArchiveHeader) - 1;
    if (contents.size() >= headerSize &&
        contents.compare(0, headerSize, EncryptedArchiveHeader, headerSize) == 0) {
        std::string plainText;
        if (!unprotectArchive(contents, plainText)) {
            archiveStatus = L"存档解密失败，文件可能已损坏或由其他 Windows 用户创建";
            return false;
        }
        contents = std::move(plainText);
    }

    std::istringstream input(contents);
    std::string signature;
    int version = 0;
    if (!(input >> signature >> version) || signature != "MOKTHYN_SAVE" ||
        version < 7 || version > 7) {
        archiveStatus = L"存档格式无效或版本过旧，已使用默认数据";
        return false;
    }
    // version 已被强校验为 ==7（见上），旧版存档迁移解析永远不可达，已删除
    return loadVersion5Archive(input, version);
}

bool loadArchive() {
    if (loadArchiveFromPath(archivePath())) return true;
    const std::wstring backupPath = archivePath() + L".bak";
    if (loadArchiveFromPath(backupPath)) {
        archiveStatus = L"主存档损坏，已从备份恢复";
        return true;
    }
    return false;
}

void beginNetworkArchiveSession() {
    if (networkRun) return;
    networkArchiveSnapshot.world = world;
    networkArchiveSnapshot.enemies = enemies;
    networkArchiveSnapshot.playerX = playerX;
    networkArchiveSnapshot.playerY = playerY;
    networkArchiveSnapshot.health = health;
    networkArchiveSnapshot.ammo = ammo;
    networkArchiveSnapshot.magazineAmmo = magazineAmmo;
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
    magazineAmmo = networkArchiveSnapshot.magazineAmmo;
    reloading = false;
    weaponMode = WeaponMode::Gun;
    nextMelee = {};
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
    const auto now = Clock::now();
    for (Enemy &enemy : enemies) enemy.nextAttack = now;
    lastMove = lastEnemyTurn = nextPlayerHit = nextPeerHit = now;
    pausedAt = {};
    networkPause = false;
    networkRun = false;
    networkArchiveSnapshot.valid = false;
    screen = Screen::Menu;
    lastArchiveSave = now;
}

bool saveArchive() {
    if (networkActive || networkRun) {
        lastArchiveSave = Clock::now();
        return true;
    }
    lastArchiveSave = Clock::now();
    // 状态与上次成功保存完全一致就跳过写盘；只有保存成功才更新哈希，
    // 失败下次仍会重试
    const unsigned long long stateHash = archiveStateHash();
    if (stateHash == lastSavedArchiveHash) return true;
    const std::wstring path = archivePath();
    if (path.empty()) {
        archiveStatus = L"无法读取 APPDATA 环境变量，存档功能不可用";
        return false;
    }
    const size_t separator = path.find_last_of(L'\\');
    const std::wstring directory = path.substr(0, separator);
    if (!CreateDirectoryW(directory.c_str(), nullptr) &&
        GetLastError() != ERROR_ALREADY_EXISTS) {
        archiveStatus = L"无法创建存档目录";
        return false;
    }

    const int safePlayerX = std::clamp(playerX, 1, MapColumns - 2);
    const int safePlayerY = std::clamp(playerY, 1, MapRows - 2);
    const int safeArmorTier = std::clamp(armorTier, 0, 4);
    const int safeArmorDurability =
        std::clamp(armorDurability, 0, ArmorDurability[safeArmorTier]);
    if (enemies.size() > MaxEnemyCount) {
        archiveStatus = L"敌人数量超出存档格式上限，未保存以避免丢失数据";
        return false;
    }
    const int safeEnemyCount = static_cast<int>(enemies.size());
    const int savedTotalAmmo = std::clamp(ammo + magazineAmmo, 0, 100000);
    std::ostringstream output;
    output << "MOKTHYN_SAVE 7\nSETTINGS " << (english ? 1 : 0) << ' '
           << uiScalePercent << ' ' << windowWidth << ' ' << windowHeight << "\nRUN "
           << (hasSavedRun ? 1 : 0) << ' ' << std::clamp(health, 0, MaxHealth) << ' '
           << savedTotalAmmo << ' ' << std::clamp(medkits, 0, 10000) << ' '
           << std::clamp(money, 0, 100000000) << ' ' << safeArmorTier << ' '
           << safeArmorDurability << ' ' << (armorOn ? 1 : 0) << ' ' << safePlayerX
           << ' ' << safePlayerY << ' ' << std::clamp(kills, 0, 100000) << ' '
           << std::clamp(totalEnemies, 0, 100000) << '\n';
    output << "MAP\n";
    for (const auto &row : world) {
        for (int tile : row)
            output << std::clamp(tile, static_cast<int>(Floor),
                                 static_cast<int>(MedkitPickup))
                   << ' ';
        output << '\n';
    }
    output << "ENEMIES " << safeEnemyCount << '\n';
    const auto saveTime = Clock::now();
    for (const Enemy &enemy : enemies) {
        const int safeX = std::clamp(enemy.x, 1, MapColumns - 2);
        const int safeY = std::clamp(enemy.y, 1, MapRows - 2);
        const int safeHp =
            std::clamp(enemy.hp, 0, enemy.boss ? BossHealth : EnemyHealth);
        const auto remaining = std::clamp<long long>(
            std::chrono::duration_cast<std::chrono::milliseconds>(enemy.nextAttack -
                                                                  saveTime)
                .count(),
            0, EnemyAttackIntervalMs);
        output << safeX << ' ' << safeY << ' ' << safeHp << ' '
               << (enemy.alive ? 1 : 0) << ' ' << (enemy.awake ? 1 : 0) << ' '
               << (enemy.boss ? 1 : 0) << ' ' << remaining << '\n';
    }
    std::string contents;
    if (!protectArchive(output.str(), contents)) {
        archiveStatus = L"存档加密失败";
        return false;
    }

    const std::wstring temporaryPath = path + L".tmp";
    HANDLE file = CreateFileW(temporaryPath.c_str(), GENERIC_WRITE, 0, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        archiveStatus = L"无法写入存档文件";
        return false;
    }
    DWORD bytesWritten = 0;
    const bool writeOk =
        WriteFile(file, contents.data(), static_cast<DWORD>(contents.size()),
                  &bytesWritten, nullptr) &&
        bytesWritten == contents.size() && FlushFileBuffers(file);
    CloseHandle(file);
    if (!writeOk) {
        DeleteFileW(temporaryPath.c_str());
        archiveStatus = L"保存存档失败";
        return false;
    }
    // 必须在覆盖主档之前备份：若在 MoveFile 之后复制，.bak
    // 拿到的就是刚写入的新文件， 主档一旦写坏备份同样坏掉，备份形同虚设
    CopyFileW(path.c_str(), (path + L".bak").c_str(), FALSE);
    if (!MoveFileExW(temporaryPath.c_str(), path.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileW(temporaryPath.c_str());
        archiveStatus = L"保存存档失败";
        return false;
    }
    lastSavedArchiveHash = stateHash;
    archiveStatus.clear();
    return true;
}
}    // namespace mokthyn
