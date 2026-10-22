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
    pickupPending = false;
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
    if (visibilityPlayerX == playerX && visibilityPlayerY == playerY &&
        computedVisibilityRevision == visibilityRevision)
        return;
    for (int y = 0; y < MapRows; ++y) {
        for (int x = 0; x < MapColumns; ++x) {
            const int distance = std::abs(x - playerX) + std::abs(y - playerY);
            visible[y][x] = distance <= ViewRadius && lineOfSight(playerX, playerY, x, y);
        }
    }
    visibilityPlayerX = playerX;
    visibilityPlayerY = playerY;
    computedVisibilityRevision = visibilityRevision;
}

bool hasEnemyAt(int x, int y) {
    return std::any_of(enemies.begin(), enemies.end(), [=](const Enemy& enemy) {
        return enemy.alive && enemy.x == x && enemy.y == y;
    });
}

bool placePickup(int type, int minimumDistance) {
    for (int attempt = 0; attempt < 400; ++attempt) {
        const int x = randomInt(1, MapColumns - 2);
        const int y = randomInt(1, MapRows - 2);
        if (world[y][x] == Floor && !hasEnemyAt(x, y) &&
            std::abs(x - playerX) + std::abs(y - playerY) >= minimumDistance) {
            world[y][x] = type;
            return true;
        }
    }
    return false;
}

void startRun() {
    ++visibilityRevision;
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

    const bool pvp = gameMode == GameMode::Pvp;
    if (pvp && networkActive) {
        playerX = networkHost ? 4 : MapColumns - 5;
        playerY = MapRows / 2;
    } else {
        playerX = MapColumns / 2;
        playerY = MapRows / 2;
    }
    const int blockCount = MapColumns * MapRows / 90;
    for (int block = 0; block < blockCount; ++block) {
        const int width = randomInt(2, 5);
        const int height = randomInt(2, 4);
        const int left = randomInt(2, MapColumns - width - 2);
        const int top = randomInt(2, MapRows - height - 2);
        if (std::abs(left - playerX) + std::abs(top - playerY) < 6) continue;
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
    const auto runStarted = Clock::now();
    const double enemyScale = difficulty == Difficulty::Easy   ? 0.6
                              : difficulty == Difficulty::Hard ? 1.4
                                                               : 1.0;
    const int regularCount =
        pvp ? 0 : std::max(8, static_cast<int>(RegularEnemyCount * enemyScale));
    const int bossCount =
        pvp ? 0 : std::max(1, static_cast<int>(BossEnemyCount * enemyScale));
    int placedEnemies = 0;
    for (int i = 0; i < regularCount; ++i) {
        for (int attempt = 0; attempt < 800; ++attempt) {
            const int x = randomInt(1, MapColumns - 2);
            const int y = randomInt(1, MapRows - 2);
            if (world[y][x] == Floor && !hasEnemyAt(x, y) &&
                std::abs(x - playerX) + std::abs(y - playerY) >= 10) {
                enemies.push_back({x, y, EnemyHealth, true, false, false,
                                   runStarted + std::chrono::milliseconds(650)});
                ++placedEnemies;
                break;
            }
        }
    }
    for (int i = 0; i < bossCount; ++i) {
        for (int attempt = 0; attempt < 800; ++attempt) {
            const int x = randomInt(3, MapColumns - 4);
            const int y = randomInt(3, MapRows - 4);
            if (world[y][x] == Floor && !hasEnemyAt(x, y) &&
                std::abs(x - playerX) + std::abs(y - playerY) >= 15) {
                enemies.push_back({x, y, BossHealth, true, false, true,
                                   runStarted + std::chrono::milliseconds(650)});
                break;
            }
        }
    }
    if (networkActive && networkHost) ++networkMapRevision;
    totalEnemies = static_cast<int>(enemies.size());
    int missingPickups = 0;
    const int ammoCount = std::max(8, static_cast<int>(AmmoPickupCount * enemyScale));
    const int medkitCount = std::max(4, static_cast<int>(MedkitPickupCount * enemyScale));
    for (int i = 0; i < ammoCount; ++i)
        if (!placePickup(AmmoPickup, 6)) ++missingPickups;
    for (int i = 0; i < medkitCount; ++i)
        if (!placePickup(MedkitPickup, 7)) ++missingPickups;
    kills = 0;
    health = 100;
    if (pvp) {
        ammo = PvpStartAmmo;
        medkits = 0;
        armorTier = 0;
        armorDurability = 0;
        armorOn = false;
    }
    armorOn = armorTier > 0 && armorDurability > 0;
    magazineAmmo = std::min(MagazineSize, ammo);
    ammo -= magazineAmmo;
    reloading = false;
    weaponMode = WeaponMode::Gun;
    nextMelee = {};
    resetRunProgress();
    lastMove = lastEnemyTurn = runStarted;
    nextPlayerHit = nextPeerHit = Clock::now();
    nextPlayerShot = {};
    screen = Screen::Playing;
    std::wstring startMessage = pvp ? L"消灭对方玩家！物资竞争，生死由命"
                                    : L"清除敌人并击败区域 Boss，然后前往撤离点";
    if (placedEnemies < regularCount || missingPickups > 0)
        startMessage += L"（地图空间不足：敌人 " + std::to_wstring(regularCount - placedEnemies) +
                        L"、物资 " + std::to_wstring(missingPickups) + L" 未生成）";
    setMessage(startMessage);
    saveArchive();
    if (networkActive && networkHost && peerConnected) {
        peerAmmoInitialized = false;
        peerMagazineInitialized = false;
        peerMedkitsInitialized = false;
        peerArmorInitialized = false;
        peerReloading = false;
        peerNextMelee = {};
        sendNetworkPacket("START " + std::string(gameMode == GameMode::Pvp ? "PVP" : "COOP"));
        sendNetworkState();
    }
}

void shoot(int dx, int dy) {
    const auto now = Clock::now();
    if (weaponMode != WeaponMode::Gun) return;
    if (reloading) {
        setMessage(L"换弹中...", 300);
        return;
    }
    if (now < nextPlayerShot) return;
    if (magazineAmmo <= 0) {
        if (ammo <= 0) {
            setMessage(L"弹药不足，寻找弹药箱");
        } else {
            reloading = true;
            reloadStarted = now;
            setMessage(L"弹匣已空，开始换弹", 800);
        }
        return;
    }
    if (networkActive && !networkHost) {
        if (!sendReliableNetworkPacket("SHOT " + std::to_string(dx) + " " +
                                       std::to_string(dy))) {
            setMessage(L"射击请求发送失败");
            return;
        }
        --magazineAmmo;
        nextPlayerShot = now + std::chrono::milliseconds(250);
        if (magazineAmmo == 0 && ammo > 0) {
            reloading = true;
            reloadStarted = now;
        }
        setMessage(L"射击确认中", 500);
        return;
    }
    for (int step = 1; step <= 9; ++step) {
        const int x = playerX + dx * step;
        const int y = playerY + dy * step;
        if (x < 0 || x >= MapColumns || y < 0 || y >= MapRows || world[y][x] == Wall) break;
        if (gameMode == GameMode::Pvp && peerConnected && peerHealth > 0 &&
            x == peerX && y == peerY) {
            --magazineAmmo;
            nextPlayerShot = now + std::chrono::milliseconds(250);
            peerHealth = std::max(0, peerHealth - 15);
            if (magazineAmmo == 0 && ammo > 0) {
                reloading = true;
                reloadStarted = now;
            }
            if (peerHealth == 0) {
                peerAmmo = 0;
                peerMagazineAmmo = 0;
                peerMedkits = 0;
                peerArmorTier = 0;
                peerArmorDurability = 0;
                peerArmorOn = false;
                peerReloading = false;
                won = true;
                screen = Screen::Result;
            } else {
                setMessage(L"命中对手");
            }
            return;
        }
        auto found = std::find_if(enemies.begin(), enemies.end(), [=](const Enemy& enemy) {
            return enemy.alive && enemy.x == x && enemy.y == y;
        });
        if (found != enemies.end()) {
            --magazineAmmo;
            nextPlayerShot = now + std::chrono::milliseconds(250);
            found->hp -= 15;
            found->awake = true;
            if (magazineAmmo == 0 && ammo > 0) {
                reloading = true;
                reloadStarted = now;
            }
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
    --magazineAmmo;
    nextPlayerShot = now + std::chrono::milliseconds(250);
    if (magazineAmmo == 0 && ammo > 0) {
        reloading = true;
        reloadStarted = now;
    }
    setMessage(L"未命中");
}

void meleeAttack() {
    const auto now = Clock::now();
    if (weaponMode != WeaponMode::Knife) return;
    if (now < nextMelee) return;
    nextMelee = now + std::chrono::milliseconds(MeleeCooldownMs);
    if (networkActive && !networkHost) {
        if (sendReliableNetworkPacket("MELEE"))
            setMessage(L"挥刀", 300);
        else
            setMessage(L"挥刀请求发送失败");
        return;
    }
    bool hit = false;
    for (int dy = -MeleeRange; dy <= MeleeRange; ++dy) {
        for (int dx = -MeleeRange; dx <= MeleeRange; ++dx) {
            if (dx == 0 && dy == 0) continue;
            if (std::abs(dx) + std::abs(dy) > MeleeRange) continue;
            const int tx = playerX + dx;
            const int ty = playerY + dy;
            if (tx <= 0 || tx >= MapColumns - 1 || ty <= 0 || ty >= MapRows - 1) continue;
            if (gameMode == GameMode::Pvp && peerConnected && peerHealth > 0 &&
                tx == peerX && ty == peerY) {
                peerHealth = std::max(0, peerHealth - MeleeDamage);
                hit = true;
                if (peerHealth == 0) {
                    won = true;
                    screen = Screen::Result;
                    return;
                }
            }
            for (Enemy& enemy : enemies) {
                if (enemy.alive && enemy.x == tx && enemy.y == ty) {
                    enemy.hp -= MeleeDamage;
                    enemy.awake = true;
                    hit = true;
                    if (enemy.hp <= 0) {
                        enemy.alive = false;
                        ++kills;
                        money += enemy.boss ? 100 : 10;
                        setMessage(enemy.boss ? L"刀杀 Boss  +100 金币" : L"刀杀敌人  +10 金币");
                    }
                }
            }
        }
    }
    if (!hit) setMessage(L"挥空", 300);
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
                if (nextX > 0 && nextX < MapColumns - 1 && nextY > 0 && nextY < MapRows - 1 &&
                    world[nextY][nextX] == Floor && !hasEnemyAt(nextX, nextY) &&
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
                    if (peerArmorDurability == 0) {
                        peerArmorOn = false;
                        peerArmorTier = 0;
                    }
                }
                peerHealth = std::max(0, peerHealth - damage);
                if (peerHealth == 0) {
                    peerAmmo = 0;
                    peerMagazineAmmo = 0;
                    peerMedkits = 0;
                    peerArmorTier = 0;
                    peerArmorDurability = 0;
                    peerArmorOn = false;
                    peerReloading = false;
                    won = (gameMode == GameMode::Pvp);
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
        if (networkActive && !networkHost) {
            if (pickupPending) return;
            if (sendReliableNetworkPacket("PICKUP " + std::to_string(pickupX) + " " +
                                          std::to_string(pickupY) + " " +
                                          std::to_string(AmmoPickup))) {
                pickupPending = true;
                setMessage(L"正在请求主机确认弹药拾取", 500);
            } else {
                setMessage(L"弹药拾取请求发送失败");
            }
            return;
        }
        const int found = randomInt(8, 16);
        ammo += found;
        tile = Floor;
        if (networkActive && networkHost) ++networkMapRevision;
        setMessage(L"拾取弹药  +" + std::to_wstring(found));
    } else if (tile == MedkitPickup) {
        const int pickupX = playerX;
        const int pickupY = playerY;
        if (networkActive && !networkHost) {
            if (pickupPending) return;
            if (sendReliableNetworkPacket("PICKUP " + std::to_string(pickupX) + " " +
                                          std::to_string(pickupY) + " " +
                                          std::to_string(MedkitPickup))) {
                pickupPending = true;
                setMessage(L"正在请求主机确认医疗包拾取", 500);
            } else {
                setMessage(L"医疗包拾取请求发送失败");
            }
            return;
        }
        ++medkits;
        tile = Floor;
        if (networkActive && networkHost) ++networkMapRevision;
        setMessage(L"拾取医疗包  +1");
    } else {
        setMessage(L"这里没有可拾取的物资");
    }
}

void movePlayer(int dx, int dy) {
    const int nextX = playerX + dx;
    const int nextY = playerY + dy;
    if (nextX <= 0 || nextX >= MapColumns - 1 || nextY <= 0 || nextY >= MapRows - 1 ||
        world[nextY][nextX] == Wall || hasEnemyAt(nextX, nextY) ||
        (dx != 0 && dy != 0 &&
         (world[playerY][nextX] == Wall || world[nextY][playerX] == Wall ||
          hasEnemyAt(nextX, playerY) || hasEnemyAt(playerX, nextY))))
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
            if (!saveArchive()) setMessage(archiveStatus, 4000);
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
    if (keyPressed('F')) {
        weaponMode = weaponMode == WeaponMode::Gun ? WeaponMode::Knife : WeaponMode::Gun;
        setMessage(weaponMode == WeaponMode::Gun ? L"武器：枪械" : L"武器：战术刀", 800);
    }
    if (keyPressed('R') && weaponMode == WeaponMode::Gun && !reloading && magazineAmmo < MagazineSize &&
        ammo > 0) {
        if (!networkActive || networkHost || sendReliableNetworkPacket("RELOAD")) {
            reloading = true;
            reloadStarted = now;
            setMessage(L"开始换弹", 600);
        } else {
            setMessage(L"换弹请求发送失败");
        }
    }
    if (reloading && now - reloadStarted >= std::chrono::milliseconds(ReloadDurationMs)) {
        const int take = std::min(MagazineSize, ammo);
        magazineAmmo = take;
        ammo -= take;
        reloading = false;
        setMessage(L"换弹完成", 500);
    }
    if (weaponMode == WeaponMode::Gun) {
        if (keyPressed(VK_UP)) shoot(0, -1);
        else if (keyPressed(VK_DOWN)) shoot(0, 1);
        else if (keyPressed(VK_LEFT)) shoot(-1, 0);
        else if (keyPressed(VK_RIGHT)) shoot(1, 0);
    } else {
        if (keyPressed(VK_UP) || keyPressed(VK_DOWN) || keyPressed(VK_LEFT) ||
            keyPressed(VK_RIGHT))
            meleeAttack();
    }

    if (now - lastMove >= std::chrono::milliseconds(115)) {
        int dx = 0;
        int dy = 0;
        dx = (keystate('D') ? 1 : 0) - (keystate('A') ? 1 : 0);
        dy = (keystate('S') ? 1 : 0) - (keystate('W') ? 1 : 0);
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
        ammo = 0;
        magazineAmmo = 0;
        medkits = 0;
        armorTier = 0;
        armorDurability = 0;
        armorOn = false;
        reloading = false;
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
        if (gameMode == GameMode::Pvp) {
            setMessage(L"PVP 模式下战斗至最后一人，无法撤离");
        } else if (world[playerY][playerX] != ExitTile)
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