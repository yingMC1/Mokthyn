#pragma once

#include <array>
#include <chrono>
#include <graphics.h>
#include <string>
#include <vector>

namespace mokthyn {
using Clock = std::chrono::steady_clock;

inline constexpr int WindowWidth = 1600;
inline constexpr int WindowHeight = 900;
inline constexpr int MapColumns = 22;
inline constexpr int MapRows = 17;
inline constexpr int TileSize = 40;
inline constexpr int MapLeft = 64;
inline constexpr int MapTop = 128;
inline constexpr int ViewRadius = 10;
inline constexpr int EnemyHealth = 45;
inline constexpr unsigned short NetworkPort = 8888;

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
enum class Screen { Menu, Shop, Help, Playing, Paused, Result, HostLobby, JoinIp, JoinLobby };

struct Enemy {
    int x = 0;
    int y = 0;
    int hp = EnemyHealth;
    bool alive = true;
    bool awake = false;
    Clock::time_point nextAttack = Clock::now();
};
} // namespace mokthyn
