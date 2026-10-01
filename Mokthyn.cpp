#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <graphics.h>
#include <random>
#include <string>
#include <vector>
#include <windows.h>

namespace {
using Clock = std::chrono::steady_clock;

constexpr int LogicalWidth = 1600;
constexpr int LogicalHeight = 900;
constexpr int WindowWidth = 1200;
constexpr int WindowHeight = 675;
constexpr float RenderScale = 0.75f;
constexpr int MapColumns = 22;
constexpr int MapRows = 17;
constexpr int TileSize = 40;
constexpr int MapLeft = 64;
constexpr int MapTop = 128;
constexpr int ViewRadius = 10;
constexpr int EnemyHealth = 55;

const ege::color_t Background = EGERGB(12, 18, 24);
const ege::color_t Panel = EGERGB(21, 30, 37);
const ege::color_t PanelLight = EGERGB(30, 42, 49);
const ege::color_t Text = EGERGB(226, 235, 230);
const ege::color_t Muted = EGERGB(137, 157, 157);
const ege::color_t Green = EGERGB(101, 220, 155);
const ege::color_t Yellow = EGERGB(245, 194, 90);
const ege::color_t Red = EGERGB(235, 99, 91);
const ege::color_t Cyan = EGERGB(91, 200, 214);

enum Tile : int { Floor, Wall, AmmoPickup, ExitTile, MedkitPickup };
enum class Screen { Menu, Shop, Help, Playing, Paused, Result };

struct Enemy {
    int x = 0;
    int y = 0;
    int hp = EnemyHealth;
    bool alive = true;
    bool awake = false;
    Clock::time_point nextAttack = Clock::now();
};

std::array<std::array<int, MapColumns>, MapRows> world{};
std::array<std::array<bool, MapColumns>, MapRows> visible{};
std::vector<Enemy> enemies;
std::mt19937 randomEngine(static_cast<unsigned int>(Clock::now().time_since_epoch().count()));

Screen screen = Screen::Menu;
int playerX = MapColumns / 2;
int playerY = MapRows / 2;
int health = 100;
int ammo = 30;
int medkits = 2;
int money = 200;
int armorTier = 0;
int armorDurability = 0;
int kills = 0;
int totalEnemies = 0;
bool armorOn = false;
bool healing = false;
bool extracting = false;
bool won = false;
Clock::time_point healStarted{};
Clock::time_point extractStarted{};
Clock::time_point lastMove{};
Clock::time_point lastEnemyTurn{};
Clock::time_point messageUntil{};
std::wstring message;
int shopSelection = 0;
PIMAGE frameBuffer = nullptr;

const std::array<int, 5> ArmorPrices = {0, 100, 200, 350, 550};
const std::array<int, 5> ArmorDurability = {0, 50, 75, 100, 125};
const std::array<int, 5> ArmorReduction = {0, 4, 7, 10, 13};

int scaled(int value) { return static_cast<int>(std::lround(value * RenderScale)); }

int randomInt(int low, int high) {
    return std::uniform_int_distribution<int>(low, high)(randomEngine);
}

void setMessage(const std::wstring& text, int milliseconds = 1800) {
    message = text;
    messageUntil = Clock::now() + std::chrono::milliseconds(milliseconds);
}

void fillRect(int left, int top, int right, int bottom, ege::color_t color) {
    setfillcolor(color);
    bar(scaled(left), scaled(top), scaled(right), scaled(bottom));
}

void outlineRect(int left, int top, int right, int bottom, ege::color_t color) {
    setlinecolor(color);
    rectangle(scaled(left), scaled(top), scaled(right), scaled(bottom));
}

void fillCircle(int x, int y, int radius, ege::color_t color) {
    setfillcolor(color);
    fillcircle(scaled(x), scaled(y), std::max(1, scaled(radius)));
}

void drawText(int x, int y, const std::wstring& value, int size = 20, ege::color_t color = Text) {
    setfont(std::max(1, scaled(size)), 0, L"Microsoft YaHei UI");
    settextcolor(color);
    outtextxy(scaled(x), scaled(y), value.c_str());
}

bool lineOfSight(int fromX, int fromY, int toX, int toY) {
    int dx = std::abs(toX - fromX);
    int dy = std::abs(toY - fromY);
    int sx = fromX < toX ? 1 : -1;
    int sy = fromY < toY ? 1 : -1;
    int error = dx - dy;
    int x = fromX;
    int y = fromY;

    while (x != toX || y != toY) {
        const int twiceError = error * 2;
        if (twiceError > -dy) {
            error -= dy;
            x += sx;
        }
        if (twiceError < dx) {
            error += dx;
            y += sy;
        }
        if ((x != toX || y != toY) && world[y][x] == Wall) return false;
    }
    return true;
}

void computeVisibility() {
    for (int y = 0; y < MapRows; ++y) {
        for (int x = 0; x < MapColumns; ++x) {
            const int distance = std::abs(x - playerX) + std::abs(y - playerY);
            visible[y][x] = distance <= ViewRadius && lineOfSight(playerX, playerY, x, y);
        }
    }
}

bool hasEnemyAt(int x, int y) {
    return std::any_of(enemies.begin(), enemies.end(), [=](const Enemy& enemy) {
        return enemy.alive && enemy.x == x && enemy.y == y;
    });
}

void placePickup(int type, int minimumDistance) {
    for (int attempt = 0; attempt < 400; ++attempt) {
        const int x = randomInt(1, MapColumns - 2);
        const int y = randomInt(1, MapRows - 2);
        if (world[y][x] == Floor && !hasEnemyAt(x, y) &&
            std::abs(x - playerX) + std::abs(y - playerY) >= minimumDistance) {
            world[y][x] = type;
            return;
        }
    }
}

void startRun() {
    for (auto& row : world)
        row.fill(Floor);
    for (int x = 0; x < MapColumns; ++x)
        world[0][x] = world[MapRows - 1][x] = Wall;
    for (int y = 0; y < MapRows; ++y)
        world[y][0] = world[y][MapColumns - 1] = Wall;

    playerX = MapColumns / 2;
    playerY = MapRows / 2;
    for (int block = 0; block < 9; ++block) {
        const int width = randomInt(2, 4);
        const int height = randomInt(2, 3);
        const int left = randomInt(2, MapColumns - width - 2);
        const int top = randomInt(2, MapRows - height - 2);
        if (std::abs(left - playerX) + std::abs(top - playerY) < 5) continue;
        for (int y = top; y < top + height; ++y)
            for (int x = left; x < left + width; ++x)
                world[y][x] = Wall;
        world[top + height / 2][left + width / 2] = Floor;
    }

    world[MapRows - 2][MapColumns - 2] = ExitTile;
    for (int y = playerY - 2; y <= playerY + 2; ++y)
        for (int x = playerX - 2; x <= playerX + 2; ++x)
            if (x > 0 && y > 0 && x < MapColumns - 1 && y < MapRows - 1) world[y][x] = Floor;

    enemies.clear();
    for (int i = 0; i < 11; ++i) {
        for (int attempt = 0; attempt < 400; ++attempt) {
            const int x = randomInt(1, MapColumns - 2);
            const int y = randomInt(1, MapRows - 2);
            if (world[y][x] == Floor && !hasEnemyAt(x, y) &&
                std::abs(x - playerX) + std::abs(y - playerY) >= 8) {
                enemies.push_back({x, y, EnemyHealth, true, false, Clock::now()});
                break;
            }
        }
    }
    totalEnemies = static_cast<int>(enemies.size());
    for (int i = 0; i < 7; ++i)
        placePickup(AmmoPickup, 4);
    for (int i = 0; i < 4; ++i)
        placePickup(MedkitPickup, 5);
    kills = 0;
    health = 100;
    armorOn = armorTier > 0 && armorDurability > 0;
    healing = extracting = false;
    lastMove = lastEnemyTurn = Clock::now();
    screen = Screen::Playing;
    setMessage(L"清除区域内的敌人，然后前往撤离点");
}

void shoot(int dx, int dy) {
    if (ammo <= 0) {
        setMessage(L"弹药不足，寻找弹药箱");
        return;
    }
    --ammo;
    for (int step = 1; step <= 9; ++step) {
        const int x = playerX + dx * step;
        const int y = playerY + dy * step;
        if (x < 0 || x >= MapColumns || y < 0 || y >= MapRows || world[y][x] == Wall) break;
        auto found = std::find_if(enemies.begin(), enemies.end(), [=](const Enemy& enemy) {
            return enemy.alive && enemy.x == x && enemy.y == y;
        });
        if (found != enemies.end()) {
            found->hp -= 15;
            found->awake = true;
            if (found->hp <= 0) {
                found->alive = false;
                ++kills;
                money += 10;
                setMessage(L"目标消灭  +10 金币");
            } else {
                setMessage(L"命中目标");
            }
            return;
        }
    }
    setMessage(L"未命中");
}

int takeDamage(int damage) {
    if (!armorOn || armorDurability <= 0) return damage;
    const int reduction = ArmorReduction[armorTier];
    armorDurability = std::max(0, armorDurability - std::max(1, damage / 2));
    if (armorDurability == 0) {
        armorOn = false;
        armorTier = 0;
        setMessage(L"护甲已损坏", 2200);
    }
    return std::max(1, damage - reduction);
}

void updateEnemies(Clock::time_point now) {
    if (now - lastEnemyTurn < std::chrono::milliseconds(450)) return;
    lastEnemyTurn = now;

    for (Enemy& enemy : enemies) {
        if (!enemy.alive) continue;
        const int distance = std::abs(enemy.x - playerX) + std::abs(enemy.y - playerY);
        if (!enemy.awake && distance <= 8 && lineOfSight(enemy.x, enemy.y, playerX, playerY))
            enemy.awake = true;

        if (enemy.awake && distance > 1) {
            const int dx = (playerX > enemy.x) - (playerX < enemy.x);
            const int dy = (playerY > enemy.y) - (playerY < enemy.y);
            const std::array<std::array<int, 2>, 4> directions = {
                {{dx, dy}, {dx, 0}, {0, dy}, {0, 0}}};
            for (const auto& direction : directions) {
                const int nextX = enemy.x + direction[0];
                const int nextY = enemy.y + direction[1];
                if (world[nextY][nextX] == Floor && !hasEnemyAt(nextX, nextY) &&
                    (nextX != playerX || nextY != playerY)) {
                    enemy.x = nextX;
                    enemy.y = nextY;
                    break;
                }
            }
        }

        const int currentDistance = std::abs(enemy.x - playerX) + std::abs(enemy.y - playerY);
        if (currentDistance <= 1 && now >= enemy.nextAttack) {
            health -= takeDamage(14);
            enemy.nextAttack = now + std::chrono::milliseconds(1200);
            if (healing || extracting) {
                healing = extracting = false;
                setMessage(L"受到攻击，动作已中断");
            } else {
                setMessage(L"受到攻击");
            }
        }
    }
}

void collectPickup() {
    int& tile = world[playerY][playerX];
    if (tile == AmmoPickup) {
        const int found = randomInt(8, 16);
        ammo += found;
        tile = Floor;
        setMessage(L"拾取弹药  +" + std::to_wstring(found));
    } else if (tile == MedkitPickup) {
        ++medkits;
        tile = Floor;
        setMessage(L"拾取医疗包  +1");
    } else {
        setMessage(L"这里没有可拾取的物资");
    }
}

void movePlayer(int dx, int dy) {
    const int nextX = playerX + dx;
    const int nextY = playerY + dy;
    if (nextX <= 0 || nextX >= MapColumns - 1 || nextY <= 0 || nextY >= MapRows - 1 ||
        world[nextY][nextX] == Wall || hasEnemyAt(nextX, nextY))
        return;
    playerX = nextX;
    playerY = nextY;
    if (healing || extracting) {
        healing = extracting = false;
        setMessage(L"动作已取消");
    }
}

void drawBar(int x, int y, int width, int height, int value, int maximum, ege::color_t color) {
    fillRect(x, y, x + width, y + height, EGERGB(44, 57, 62));
    const int filled = maximum > 0 ? width * std::clamp(value, 0, maximum) / maximum : 0;
    if (filled > 0) fillRect(x, y, x + filled, y + height, color);
}

void drawMap() {
    computeVisibility();
    fillRect(MapLeft - 12, MapTop - 12, MapLeft + MapColumns * TileSize + 12,
             MapTop + MapRows * TileSize + 12, EGERGB(9, 14, 18));

    for (int y = 0; y < MapRows; ++y) {
        for (int x = 0; x < MapColumns; ++x) {
            const int left = MapLeft + x * TileSize;
            const int top = MapTop + y * TileSize;
            if (!visible[y][x]) {
                fillRect(left, top, left + TileSize - 1, top + TileSize - 1, EGERGB(16, 23, 28));
                continue;
            }

            ege::color_t tileColor = EGERGB(34, 47, 51);
            if (world[y][x] == Wall) tileColor = EGERGB(67, 77, 75);
            if (world[y][x] == ExitTile) tileColor = EGERGB(34, 78, 63);
            fillRect(left, top, left + TileSize - 1, top + TileSize - 1, tileColor);
            outlineRect(left, top, left + TileSize - 1, top + TileSize - 1, EGERGB(24, 35, 39));

            if (world[y][x] == AmmoPickup) {
                fillCircle(left + TileSize / 2, top + TileSize / 2, 7, Yellow);
                drawText(left + 15, top + 8, L"+", 19, Background);
            } else if (world[y][x] == MedkitPickup) {
                fillCircle(left + TileSize / 2, top + TileSize / 2, 8, Cyan);
                drawText(left + 14, top + 8, L"+", 20, Background);
            } else if (world[y][x] == ExitTile) {
                outlineRect(left + 8, top + 8, left + TileSize - 9, top + TileSize - 9, Green);
                drawText(left + 12, top + 9, L"E", 19, Green);
            }

            for (const Enemy& enemy : enemies) {
                if (!enemy.alive || enemy.x != x || enemy.y != y) continue;
                fillCircle(left + TileSize / 2, top + TileSize / 2, 11, Red);
                fillCircle(left + TileSize / 2 - 3, top + TileSize / 2 - 3, 3,
                           EGERGB(255, 174, 137));
                break;
            }
        }
    }

    const int playerCenterX = MapLeft + playerX * TileSize + TileSize / 2;
    const int playerCenterY = MapTop + playerY * TileSize + TileSize / 2;
    fillCircle(playerCenterX, playerCenterY, 16, EGERGB(37, 70, 62));
    fillCircle(playerCenterX, playerCenterY, 10, Green);
    fillCircle(playerCenterX + 3, playerCenterY - 3, 3, Text);
}

void drawHud() {
    const int x = 1000;
    fillRect(x, 104, 1535, 806, Panel);
    fillRect(x, 104, 1006, 806, Green);
    drawText(x + 34, 132, L"无限枪洲", 35, Text);
    drawText(x + 35, 184, L"FIELD OPERATION  /  01", 15, Muted);

    drawText(x + 34, 232, L"生命", 18, Muted);
    drawText(x + 420, 228, std::to_wstring(health) + L" / 100", 21,
             health > 60 ? Green : (health > 30 ? Yellow : Red));
    drawBar(x + 34, 263, 452, 13, health, 100, health > 30 ? Green : Red);

    drawText(x + 34, 309, L"弹药", 18, Muted);
    drawText(x + 34, 343, std::to_wstring(ammo), 42, Yellow);
    drawText(x + 167, 362, L"发", 17, Muted);
    drawText(x + 282, 309, L"医疗包", 18, Muted);
    drawText(x + 282, 343, std::to_wstring(medkits), 42, Cyan);

    fillRect(x + 34, 413, x + 486, 414, EGERGB(52, 65, 68));
    drawText(x + 34, 438, L"任务进度", 18, Muted);
    drawText(x + 34, 472,
             std::to_wstring(kills) + L" / " + std::to_wstring(totalEnemies) + L"  目标", 25, Text);
    drawBar(x + 34, 516, 452, 8, kills, totalEnemies, Green);

    drawText(x + 34, 558, L"护甲", 18, Muted);
    drawText(x + 126, 555, armorOn ? L"等级 " + std::to_wstring(armorTier) : L"未装备", 19,
             armorOn ? Cyan : Muted);
    if (armorOn) drawBar(x + 34, 588, 452, 8, armorDurability, ArmorDurability[armorTier], Cyan);
    drawText(x + 34, 626, L"金币", 18, Muted);
    drawText(x + 128, 622, std::to_wstring(money), 24, Yellow);

    fillRect(x + 28, 668, x + 492, 669, EGERGB(52, 65, 68));
    drawText(x + 34, 691, L"WASD 移动    方向键射击", 17, Text);
    drawText(x + 34, 724, L"E 拾取   H 治疗   K 护甲", 17, Muted);
    drawText(x + 34, 757, L"Q 撤离   P 暂停   ESC 菜单", 17, Muted);

    if (Clock::now() < messageUntil) {
        fillRect(x + 24, 824, 1535, 872, PanelLight);
        drawText(x + 42, 837, message, 19, Yellow);
    }
}

void drawMenu() {
    cleardevice();
    fillRect(80, 78, 1520, 822, Panel);
    fillRect(80, 78, 90, 822, Green);
    drawText(150, 158, L"无限枪洲", 66, Text);
    drawText(155, 246, L"TACTICAL EXTRACTION", 20, Green);
    drawText(155, 333, L"深入失控区域，清除目标并带着物资撤离。", 25, Muted);
    fillRect(155, 405, 1442, 406, EGERGB(52, 65, 68));

    drawText(155, 454, L"1", 25, Green);
    drawText(205, 449, L"开始行动", 25, Text);
    drawText(155, 514, L"2", 25, Yellow);
    drawText(205, 509, L"黑市商店", 25, Text);
    drawText(155, 574, L"3", 25, Cyan);
    drawText(205, 569, L"操作说明", 25, Text);
    drawText(155, 634, L"ESC", 18, Muted);
    drawText(205, 629, L"退出", 22, Text);

    drawText(920, 455, L"弹药库存", 18, Muted);
    drawText(920, 489, std::to_wstring(ammo), 34, Yellow);
    drawText(1140, 455, L"医疗包", 18, Muted);
    drawText(1140, 489, std::to_wstring(medkits), 34, Cyan);
    drawText(920, 560, L"护甲", 18, Muted);
    drawText(920, 594, armorTier > 0 ? L"等级 " + std::to_wstring(armorTier) : L"无", 28, Green);
    drawText(1140, 560, L"持有金币", 18, Muted);
    drawText(1140, 594, std::to_wstring(money), 28, Yellow);
    drawText(155, 746, L"高清战术视图   /   EGE GRAPHICS", 17, Muted);
}

void drawShop() {
    cleardevice();
    fillRect(105, 70, 1495, 830, Panel);
    drawText(155, 120, L"黑市", 44, Text);
    drawText(155, 179, L"补充装备，为下一次行动做好准备", 19, Muted);
    drawText(1175, 135, L"持有金币", 17, Muted);
    drawText(1320, 127, std::to_wstring(money), 32, Yellow);

    const std::array<std::wstring, 6> labels = {L"一级护甲       耐久 50 / 减伤 4       100 金",
                                                L"二级护甲       耐久 75 / 减伤 7       200 金",
                                                L"三级护甲       耐久 100 / 减伤 10     350 金",
                                                L"四级护甲       耐久 125 / 减伤 13     550 金",
                                                L"弹药补给       +10 发                 5 金",
                                                L"医疗包         +1 个                  20 金"};
    for (int i = 0; i < static_cast<int>(labels.size()); ++i) {
        const int y = 240 + i * 74;
        fillRect(155, y, 1440, y + 56, shopSelection == i ? PanelLight : EGERGB(25, 36, 42));
        drawText(178, y + 15, std::to_wstring(i + 1) + L".", 20,
                 shopSelection == i ? Green : Muted);
        drawText(230, y + 15, labels[i], 20, Text);
    }
    drawText(155, 720, L"↑ / ↓ 选择     ENTER 购买     0 返回", 18, Muted);
    drawText(155, 766, L"护甲升级会替换当前护甲。", 17, Muted);
}

void buySelectedItem() {
    const int item = shopSelection;
    if (item < 4) {
        const int tier = item + 1;
        if (money < ArmorPrices[tier]) {
            setMessage(L"金币不足");
            return;
        }
        money -= ArmorPrices[tier];
        armorTier = tier;
        armorDurability = ArmorDurability[tier];
        armorOn = true;
        setMessage(L"护甲已装备");
    } else if (item == 4 && money >= 5) {
        money -= 5;
        ammo += 10;
        setMessage(L"弹药补给完成");
    } else if (item == 5 && money >= 20) {
        money -= 20;
        ++medkits;
        setMessage(L"医疗包已补充");
    } else {
        setMessage(L"金币不足");
    }
}

void drawHelp() {
    cleardevice();
    fillRect(130, 90, 1470, 810, Panel);
    drawText(190, 140, L"行动简报", 42, Text);
    const std::array<std::wstring, 7> instructions = {
        L"W A S D        在地图中移动",
        L"方向键          朝对应方向射击，子弹可穿过空地",
        L"E               拾取脚下的弹药或医疗包",
        L"H               消耗医疗包，等待 3 秒恢复 20 点生命",
        L"K               穿上或脱下已购买的护甲",
        L"Q               清除全部目标后，在撤离点开始撤离",
        L"P / ESC         暂停行动 / 返回主菜单"};
    for (int i = 0; i < static_cast<int>(instructions.size()); ++i)
        drawText(200, 230 + i * 62, instructions[i], 22, i == 5 ? Green : Text);
    drawText(200, 700, L"按任意数字键返回", 18, Muted);
}

void drawResult() {
    cleardevice();
    fillRect(250, 155, 1350, 755, Panel);
    drawText(340, 230, won ? L"撤离成功" : L"行动结束", 58, won ? Green : Red);
    drawText(345, 332, L"本次击杀", 20, Muted);
    drawText(345, 370, std::to_wstring(kills) + L" / " + std::to_wstring(totalEnemies), 38, Text);
    drawText(720, 332, L"剩余弹药", 20, Muted);
    drawText(720, 370, std::to_wstring(ammo), 38, Yellow);
    drawText(1040, 332, L"持有金币", 20, Muted);
    drawText(1040, 370, std::to_wstring(money), 38, Yellow);
    drawText(345, 500, L"按 ENTER 返回主菜单", 23, Text);
}

bool keyPressed(int key) { return keypress(key) > 0; }

void updatePlaying() {
    const auto now = Clock::now();
    if (keyPressed(VK_ESCAPE)) {
        screen = Screen::Menu;
        return;
    }
    if (keyPressed('P')) {
        screen = Screen::Paused;
        return;
    }
    if (keyPressed('K')) {
        if (armorTier <= 0 || armorDurability <= 0)
            setMessage(L"没有可用护甲");
        else {
            armorOn = !armorOn;
            setMessage(armorOn ? L"护甲已穿上" : L"护甲已脱下");
        }
    }
    if (keyPressed('E')) collectPickup();
    if (keyPressed('H')) {
        if (medkits > 0 && health < 100 && !healing && !extracting) {
            --medkits;
            healing = true;
            healStarted = now;
            setMessage(L"治疗中，请保持安全");
        } else
            setMessage(L"现在无法治疗");
    }
    if (keyPressed(VK_UP)) shoot(0, -1);
    if (keyPressed(VK_DOWN)) shoot(0, 1);
    if (keyPressed(VK_LEFT)) shoot(-1, 0);
    if (keyPressed(VK_RIGHT)) shoot(1, 0);

    if (now - lastMove >= std::chrono::milliseconds(115)) {
        int dx = 0;
        int dy = 0;
        if (keystate('W'))
            dy = -1;
        else if (keystate('S'))
            dy = 1;
        else if (keystate('A'))
            dx = -1;
        else if (keystate('D'))
            dx = 1;
        if (dx != 0 || dy != 0) {
            movePlayer(dx, dy);
            lastMove = now;
        }
    }

    updateEnemies(now);
    if (healing && now - healStarted >= std::chrono::seconds(3)) {
        healing = false;
        health = std::min(100, health + 20);
        setMessage(L"治療完成  +20 生命");
    }
    if (health <= 0) {
        health = 0;
        won = false;
        screen = Screen::Result;
        return;
    }

    if (extracting && now - extractStarted >= std::chrono::seconds(5)) {
        won = true;
        screen = Screen::Result;
        return;
    }
    if (keyPressed('Q')) {
        if (world[playerY][playerX] != ExitTile)
            setMessage(L"前往地图上的撤离点");
        else if (kills < totalEnemies)
            setMessage(L"区域仍有目标存活");
        else {
            extracting = true;
            extractStarted = now;
            setMessage(L"撤离开始，保持位置 5 秒", 5200);
        }
    }
}

void drawPlaying() {
    cleardevice();
    drawText(64, 51, L"OPERATIONS / EXTRACTION ZONE", 16, Muted);
    drawMap();
    drawHud();
    if (healing) {
        const int seconds =
            std::max(0, 3 - static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(
                                                 Clock::now() - healStarted)
                                                 .count()));
        drawText(MapLeft + 12, 836, L"治疗中  " + std::to_wstring(seconds) + L" 秒", 18, Cyan);
    } else if (extracting) {
        const int seconds =
            std::max(0, 5 - static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(
                                                 Clock::now() - extractStarted)
                                                 .count()));
        drawText(MapLeft + 12, 836, L"撤离中  " + std::to_wstring(seconds) + L" 秒", 18, Green);
    }
}
} // namespace

int main() {
    setinitmode(INIT_DEFAULT);
    initgraph(WindowWidth, WindowHeight);
    ege_enable_aa(true);
    setbkmode(TRANSPARENT);
    frameBuffer = newimage(WindowWidth, WindowHeight);
    if (frameBuffer == nullptr) {
        closegraph();
        return 1;
    }
    ege_enable_aa(true, frameBuffer);

    while (is_run()) {
        settarget(frameBuffer);
        switch (screen) {
        case Screen::Menu:
            if (keyPressed('1') || keyPressed(VK_RETURN))
                startRun();
            else if (keyPressed('2'))
                screen = Screen::Shop;
            else if (keyPressed('3'))
                screen = Screen::Help;
            else if (keyPressed(VK_ESCAPE)) {
                settarget(nullptr);
                delimage(frameBuffer);
                closegraph();
                return 0;
            }
            drawMenu();
            break;
        case Screen::Shop:
            if (keyPressed(VK_ESCAPE) || keyPressed('0'))
                screen = Screen::Menu;
            else if (keyPressed(VK_UP))
                shopSelection = (shopSelection + 5) % 6;
            else if (keyPressed(VK_DOWN))
                shopSelection = (shopSelection + 1) % 6;
            else if (keyPressed(VK_RETURN))
                buySelectedItem();
            else
                for (int key = '1'; key <= '6'; ++key) {
                    if (keyPressed(key)) {
                        shopSelection = key - '1';
                        buySelectedItem();
                        break;
                    }
                }
            drawShop();
            break;
        case Screen::Help:
            if (kbhit()) {
                getch();
                screen = Screen::Menu;
            }
            drawHelp();
            break;
        case Screen::Playing:
            updatePlaying();
            drawPlaying();
            break;
        case Screen::Paused:
            drawPlaying();
            fillRect(0, 0, LogicalWidth, LogicalHeight, EGEARGB(170, 5, 10, 13));
            drawText(630, 365, L"已暂停", 48, Text);
            drawText(594, 435, L"按 P 继续  /  ESC 返回菜单", 20, Muted);
            if (keyPressed('P'))
                screen = Screen::Playing;
            else if (keyPressed(VK_ESCAPE))
                screen = Screen::Menu;
            break;
        case Screen::Result:
            drawResult();
            if (keyPressed(VK_RETURN)) screen = Screen::Menu;
            break;
        }
        settarget(nullptr);
        putimage(0, 0, frameBuffer);
        delay_fps(60);
    }
    settarget(nullptr);
    delimage(frameBuffer);
    closegraph();
    return 0;
}