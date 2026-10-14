#pragma once
#include "GameArchive.h"
#include "GameLogic.h"
#include "GameNetwork.h"
#include "GameRender.h"
#include "GameState.h"
#include "GameTypes.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <sstream>
#include <string>
#include <ws2tcpip.h>

namespace mokthyn {
int randomInt(int low, int high) {
    return std::uniform_int_distribution<int>(low, high)(randomEngine);
}

void setMessage(const std::wstring& text, int milliseconds) {
    message = text;
    messageUntil = Clock::now() + std::chrono::milliseconds(milliseconds);
}

void resetRunProgress() {
    healing = false;
    extracting = false;
    won = false;
    healStarted = {};
    extractStarted = {};
    peerExtracting = false;
    peerExtractStarted = {};
    peerExtractConfirmed = false;
    lastEvacSend = {};
    networkPause = false;
    pausedAt = {};
}

void cancelExtraction() {
    extracting = false;
    peerExtractConfirmed = false;
    extractStarted = {};
    lastEvacSend = {};
    if (networkActive && !networkHost) sendNetworkPacket("EVAC 0");
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
    if (networkActive) {
        beginNetworkArchiveSession();
    } else {
        networkRun = false;
        hasSavedRun = true;
    }
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
    const int bossX = MapColumns - 3;
    const int bossY = MapRows - 3;
    for (int y = bossY - 1; y <= bossY + 1; ++y)
        for (int x = bossX - 1; x <= bossX + 1; ++x)
            if (x > 0 && y > 0 && x < MapColumns - 1 && y < MapRows - 1)
                world[y][x] = Floor;
    for (int x = playerX; x <= bossX; ++x) world[playerY][x] = Floor;
    for (int y = playerY; y <= bossY; ++y) world[y][bossX] = Floor;
    world[MapRows - 2][MapColumns - 2] = ExitTile;
    for (int i = 0; i < 16; ++i) {
        for (int attempt = 0; attempt < 400; ++attempt) {
            const int x = randomInt(1, MapColumns - 2);
            const int y = randomInt(1, MapRows - 2);
            if (world[y][x] == Floor && !hasEnemyAt(x, y) &&
                std::abs(x - playerX) + std::abs(y - playerY) >= 8 &&
                std::abs(x - bossX) + std::abs(y - bossY) >= 3) {
                enemies.push_back({x, y, EnemyHealth, true, false, false, Clock::now()});
                break;
            }
        }
    }
    enemies.push_back({bossX, bossY, BossHealth, true, false, true, Clock::now()});
    if (networkActive && networkHost) ++networkMapRevision;
    totalEnemies = static_cast<int>(enemies.size());
    for (int i = 0; i < 7; ++i)
        placePickup(AmmoPickup, 4);
    for (int i = 0; i < 4; ++i)
        placePickup(MedkitPickup, 5);
    kills = 0;
    health = 100;
    armorOn = armorTier > 0 && armorDurability > 0;
    resetRunProgress();
    lastMove = lastEnemyTurn = Clock::now();
    nextPlayerHit = nextPeerHit = Clock::now();
    screen = Screen::Playing;
    setMessage(L"清除敌人并击败区域 Boss，然后前往撤离点");
    saveArchive();
    if (networkActive && networkHost && peerConnected) {
        sendNetworkPacket("START");
        sendNetworkState();
    }
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
            const int targetX = found->x;
            const int targetY = found->y;
            if (networkActive && !networkHost) {
                sendReliableNetworkPacket("HIT " + std::to_string(targetX) + " " +
                                          std::to_string(targetY) + " 15");
                setMessage(L"命中确认中");
                return;
            }
            found->hp -= 15;
            found->awake = true;
            if (found->hp <= 0) {
                found->alive = false;
                ++kills;
                money += found->boss ? 100 : 10;
                setMessage(found->boss ? L"区域 Boss 已击败  +100 金币" : L"目标消灭  +10 金币");
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
    if (networkActive && !networkHost) return;
    if (now - lastEnemyTurn < std::chrono::milliseconds(650)) return;
    lastEnemyTurn = now;

    for (Enemy& enemy : enemies) {
        if (!enemy.alive) continue;
        int targetX = playerX;
        int targetY = playerY;
        bool targetPeer = false;
        if (networkActive && networkHost && peerConnected && peerHealth > 0 && peerX > 0 &&
            peerY > 0 &&
            std::abs(enemy.x - peerX) + std::abs(enemy.y - peerY) <
                std::abs(enemy.x - playerX) + std::abs(enemy.y - playerY)) {
            targetX = peerX;
            targetY = peerY;
            targetPeer = true;
        }
        const int distance = std::abs(enemy.x - targetX) + std::abs(enemy.y - targetY);
        if (!enemy.awake && distance <= 6 && lineOfSight(enemy.x, enemy.y, targetX, targetY))
            enemy.awake = true;

        const bool bossEnraged = enemy.boss && enemy.hp <= BossHealth / 2;
        const int attackRange = enemy.boss ? (bossEnraged ? 6 : 5) : 1;
        if (enemy.awake && distance > attackRange) {
            const int dx = (targetX > enemy.x) - (targetX < enemy.x);
            const int dy = (targetY > enemy.y) - (targetY < enemy.y);
            const std::array<std::array<int, 2>, 3> directions = {{{dx, 0}, {0, dy}, {0, 0}}};
            for (const auto& direction : directions) {
                const int nextX = enemy.x + direction[0];
                const int nextY = enemy.y + direction[1];
                if (world[nextY][nextX] == Floor && !hasEnemyAt(nextX, nextY) &&
                    (nextX != playerX || nextY != playerY) && (nextX != peerX || nextY != peerY)) {
                    enemy.x = nextX;
                    enemy.y = nextY;
                    break;
                }
            }
        }

        const int currentDistance = std::abs(enemy.x - targetX) + std::abs(enemy.y - targetY);
        const bool bossCanSeeTarget =
            !enemy.boss || lineOfSight(enemy.x, enemy.y, targetX, targetY);
        if (currentDistance <= attackRange && bossCanSeeTarget &&
            now >= enemy.nextAttack &&
            now >= (targetPeer ? nextPeerHit : nextPlayerHit)) {
            if (targetPeer) {
                int damage = enemy.boss ? (bossEnraged ? 20 : 15) : 9;
                if (peerArmorOn && peerArmorDurability > 0) {
                    damage = std::max(1, damage - ArmorReduction[peerArmorTier]);
                    peerArmorDurability = std::max(0, peerArmorDurability - 4);
                    if (peerArmorDurability == 0) peerArmorOn = false;
                }
                peerHealth = std::max(0, peerHealth - damage);
                if (peerHealth == 0) {
                    won = false;
                    screen = Screen::Result;
                }
            } else {
                health -= takeDamage(enemy.boss ? (bossEnraged ? 20 : 15) : 9);
            }
            enemy.nextAttack =
                now + std::chrono::milliseconds(enemy.boss ? (bossEnraged ? 1000 : 1500) : 1800);
            if (targetPeer)
                nextPeerHit = now + std::chrono::milliseconds(850);
            else
                nextPlayerHit = now + std::chrono::milliseconds(850);
            if (targetPeer) {
                peerHealing = false;
                peerHealStarted = {};
                if (peerExtracting) {
                    peerExtracting = false;
                    peerExtractStarted = {};
                }
            }
            if (!targetPeer && (healing || extracting)) {
                healing = false;
                if (extracting) cancelExtraction();
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
        const int pickupX = playerX;
        const int pickupY = playerY;
        const int found = randomInt(8, 16);
        ammo += found;
        tile = Floor;
        if (networkActive && !networkHost)
            sendReliableNetworkPacket("PICKUP " + std::to_string(pickupX) + " " +
                                      std::to_string(pickupY) + " " +
                                      std::to_string(AmmoPickup));
        setMessage(L"拾取弹药  +" + std::to_wstring(found));
    } else if (tile == MedkitPickup) {
        const int pickupX = playerX;
        const int pickupY = playerY;
        ++medkits;
        tile = Floor;
        if (networkActive && !networkHost)
            sendReliableNetworkPacket("PICKUP " + std::to_string(pickupX) + " " +
                                      std::to_string(pickupY) + " " +
                                      std::to_string(MedkitPickup));
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
        if (healing && networkActive && !networkHost)
            sendReliableNetworkPacket("HEAL_CANCEL");
        healing = false;
        if (extracting) cancelExtraction();
        setMessage(L"动作已取消");
    }
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
    saveArchive();
}

bool keyPressed(int key) { return keypress(key) > 0; }

void updatePlaying() {
    const auto now = Clock::now();
    if (keyPressed(VK_ESCAPE)) {
        if (networkActive) {
            sendNetworkPacket("LEAVE");
            closeNetwork();
        }
        if (networkRun)
            restoreNetworkArchiveSession();
        else {
            resetRunProgress();
            screen = Screen::Menu;
        }
        return;
    }
    if (keyPressed('P')) {
        if (networkActive && !networkHost) {
            setMessage(L"联机行动由主机控制暂停");
        } else {
            pausedAt = now;
            screen = Screen::Paused;
            return;
        }
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
            if (!networkActive || networkHost ||
                sendReliableNetworkPacket("HEAL_START")) {
                --medkits;
                healing = true;
                healStarted = now;
                setMessage(L"治疗中，请保持安全");
            } else {
                setMessage(L"治疗请求发送失败");
            }
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
        if (networkActive && !networkHost) {
            if (sendReliableNetworkPacket("HEAL_DONE"))
                setMessage(L"治疗完成，等待主机确认");
            else
                setMessage(L"治疗确认发送失败");
        } else {
            health = std::min(100, health + 20);
            setMessage(L"治疗完成  +20 生命");
        }
    }
    if (health <= 0) {
        health = 0;
        won = false;
        hasSavedRun = false;
        screen = Screen::Result;
        saveArchive();
        return;
    }

    if (extracting && !(networkActive && !networkHost) &&
        now - extractStarted >= std::chrono::seconds(5)) {
        won = true;
        hasSavedRun = false;
        screen = Screen::Result;
        saveArchive();
        return;
    }
    if (keyPressed('Q')) {
        if (world[playerY][playerX] != ExitTile)
            setMessage(L"前往地图上的撤离点");
        else if (std::any_of(enemies.begin(), enemies.end(), [](const Enemy& enemy) {
                     return enemy.boss && enemy.alive;
                 }))
            setMessage(L"击败区域 Boss 后才能撤离");
        else {
            if (healing && networkActive && !networkHost)
                sendReliableNetworkPacket("HEAL_CANCEL");
            healing = false;
            extracting = true;
            extractStarted = now;
            peerExtractConfirmed = false;
            lastEvacSend = {};
            setMessage(L"撤离开始，保持位置 5 秒", 5200);
        }
    }
}

} // namespace mokthyn