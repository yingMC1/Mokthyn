#pragma once

#include <array>
#include <chrono>
#include <graphics.h>
#include <string>
#include <vector>

namespace mokthyn {
using Clock = std::chrono::steady_clock;

inline constexpr int DefaultWindowWidth = 1200;
inline constexpr int DefaultWindowHeight = 675;
inline constexpr int MinWindowWidth = 960;
inline constexpr int MaxWindowWidth = 1920;
inline constexpr int WindowWidthStep = 120;
inline constexpr int MinWindowHeight = 540;
inline constexpr int MaxWindowHeight = 1080;
inline constexpr int WindowHeightStep = 45;
inline constexpr int DesignWidth = 1600;
inline constexpr int DesignHeight = 900;
inline constexpr int MinUiScalePercent = 100;
inline constexpr int MaxUiScalePercent = 130;
inline constexpr int UiScaleStepPercent = 10;
inline constexpr int MapColumns = 72;
inline constexpr int MapRows = 48;
inline constexpr int ViewRadius = 28;
inline constexpr int EnemyHealth = 45;
inline constexpr int BossHealth = 300;
inline constexpr unsigned short NetworkPort = 8888;
inline constexpr int RegularEnemyCount = 24;
inline constexpr int BossEnemyCount = 2;
inline constexpr int AmmoPickupCount = 24;
inline constexpr int MedkitPickupCount = 14;
inline constexpr wchar_t DownloadUrl[] = L"https://mokthyn-3xya338.maozi.io/";

inline const ege::color_t Background = EGERGB(12, 18, 24);
inline const ege::color_t Panel = EGERGB(21, 30, 37);
inline const ege::color_t PanelLight = EGERGB(30, 42, 49);
inline const ege::color_t Text = EGERGB(226, 235, 230);
inline const ege::color_t Muted = EGERGB(137, 157, 157);
inline const ege::color_t Green = EGERGB(101, 220, 155);
inline const ege::color_t Yellow = EGERGB(245, 194, 90);
inline const ege::color_t Red = EGERGB(235, 99, 91);
inline const ege::color_t Cyan = EGERGB(91, 200, 214);

inline const std::array<int, 5> ArmorPrices = {0, 100, 200, 350, 550};
inline const std::array<int, 5> ArmorDurability = {0, 50, 75, 100, 125};
inline const std::array<int, 5> ArmorReduction = {0, 4, 7, 10, 13};

enum Tile : int { Floor, Wall, AmmoPickup, ExitTile, MedkitPickup };
enum class Screen { Menu, Shop, Help, Settings, Playing, Paused, Result, HostLobby, JoinIp, JoinLobby };
enum class GameMode { Coop, Pvp };
enum class Difficulty { Easy, Normal, Hard };
enum class WeaponMode { Gun, Knife };
inline constexpr int MagazineSize = 10;
inline constexpr int ReloadDurationMs = 1000;
inline constexpr int MeleeCooldownMs = 500;
inline constexpr int MeleeRange = 2;
inline constexpr int MeleeDamage = 12;
inline constexpr int PvpStartAmmo = 50;

struct Enemy {
    int x = 0;
    int y = 0;
    int hp = EnemyHealth;
    bool alive = true;
    bool awake = false;
    bool boss = false;
    Clock::time_point nextAttack = Clock::now();
};

struct ArchiveSnapshot {
    std::array<std::array<int, MapColumns>, MapRows> world{};
    std::vector<Enemy> enemies;
    int playerX = MapColumns / 2;
    int playerY = MapRows / 2;
    int health = 100;
    int ammo = 30;
    int magazineAmmo = 0;
    int medkits = 2;
    int money = 200;
    int armorTier = 0;
    int armorDurability = 0;
    int kills = 0;
    int totalEnemies = 0;
    int uiScalePercent = 120;
    int windowWidth = DefaultWindowWidth;
    int windowHeight = DefaultWindowHeight;
    bool english = false;
    bool hasSavedRun = false;
    bool armorOn = false;
    bool valid = false;
};
} // namespace mokthyn
