// 存档脏检测哈希的纯逻辑测试：验证 archiveStateHash() 恰好覆盖所有会写进
// 存档的状态，且刻意排除挂钟相关的 nextAttack。只链接纯逻辑，不碰文件/DPAPI。
// 编译（项目根目录，需 -I ege/include 解析 EGE 的 graphics.h，链接 crypt32）：
//   g++ -std=c++17 -Wall -Wextra -static -static-libgcc -static-libstdc++
//       -I ege/include GameArchive_hash_test.cpp -o build\GameArchive_hash_test.exe
//       -lcrypt32
#include <winsock2.h>    // 先于 windows.h（EGE graphics.h 会带进来），消 #warning

#include "game.h/GameArchiveImpl.h"

#include <cstdio>

using namespace mokthyn;

static int failures = 0;

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            ++failures;                                                      \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        }                                                                    \
    } while (0)

int main() {
    world[3][3] = Floor;
    playerX = 10;
    playerY = 12;
    health = 80;
    ammo = 30;
    magazineAmmo = 10;
    medkits = 1;
    money = 250;
    armorTier = 2;
    armorDurability = 30;
    armorOn = true;
    kills = 5;
    totalEnemies = 20;
    hasSavedRun = true;
    english = false;
    uiScalePercent = 110;
    windowWidth = 1200;
    windowHeight = 675;
    enemies.clear();
    enemies.push_back({5, 6, 30, true, true, false, Clock::now()});

    const unsigned long long base = archiveStateHash();

    // 同一状态多次计算必须稳定
    CHECK(archiveStateHash() == base);

    // 每个会写进存档的字段变化都必须改变哈希，还原后必须恢复
    ++playerX;
    CHECK(archiveStateHash() != base);
    --playerX;
    CHECK(archiveStateHash() == base);
    ++playerY;
    CHECK(archiveStateHash() != base);
    --playerY;
    CHECK(archiveStateHash() == base);
    health = 79;
    CHECK(archiveStateHash() != base);
    health = 80;
    CHECK(archiveStateHash() == base);
    ammo = 29;
    CHECK(archiveStateHash() != base);
    ammo = 30;
    CHECK(archiveStateHash() == base);
    medkits = 2;
    CHECK(archiveStateHash() != base);
    medkits = 1;
    CHECK(archiveStateHash() == base);
    money = 251;
    CHECK(archiveStateHash() != base);
    money = 250;
    CHECK(archiveStateHash() == base);
    armorTier = 3;
    CHECK(archiveStateHash() != base);
    armorTier = 2;
    CHECK(archiveStateHash() == base);
    armorDurability = 29;
    CHECK(archiveStateHash() != base);
    armorDurability = 30;
    CHECK(archiveStateHash() == base);
    armorOn = false;
    CHECK(archiveStateHash() != base);
    armorOn = true;
    CHECK(archiveStateHash() == base);
    kills = 6;
    CHECK(archiveStateHash() != base);
    kills = 5;
    CHECK(archiveStateHash() == base);
    totalEnemies = 21;
    CHECK(archiveStateHash() != base);
    totalEnemies = 20;
    CHECK(archiveStateHash() == base);
    hasSavedRun = false;
    CHECK(archiveStateHash() != base);
    hasSavedRun = true;
    CHECK(archiveStateHash() == base);
    english = true;
    CHECK(archiveStateHash() != base);
    english = false;
    CHECK(archiveStateHash() == base);
    uiScalePercent = 120;
    CHECK(archiveStateHash() != base);
    uiScalePercent = 110;
    CHECK(archiveStateHash() == base);
    windowWidth = 1320;
    CHECK(archiveStateHash() != base);
    windowWidth = 1200;
    CHECK(archiveStateHash() == base);
    windowHeight = 720;
    CHECK(archiveStateHash() != base);
    windowHeight = 675;
    CHECK(archiveStateHash() == base);

    // 地图内容（拾取会改格子）
    world[3][3] = Wall;
    CHECK(archiveStateHash() != base);
    world[3][3] = Floor;
    CHECK(archiveStateHash() == base);

    // 敌人字段
    enemies[0].hp = 25;
    CHECK(archiveStateHash() != base);
    enemies[0].hp = 30;
    CHECK(archiveStateHash() == base);
    enemies[0].alive = false;
    CHECK(archiveStateHash() != base);
    enemies[0].alive = true;
    CHECK(archiveStateHash() == base);
    enemies[0].awake = false;
    CHECK(archiveStateHash() != base);
    enemies[0].awake = true;
    CHECK(archiveStateHash() == base);
    enemies[0].boss = true;
    CHECK(archiveStateHash() != base);
    enemies[0].boss = false;
    CHECK(archiveStateHash() == base);
    enemies[0].x = 7;
    CHECK(archiveStateHash() != base);
    enemies[0].x = 5;
    CHECK(archiveStateHash() == base);
    enemies.push_back({9, 9, EnemyHealth, true, false, false, Clock::now()});
    CHECK(archiveStateHash() != base);
    enemies.pop_back();
    CHECK(archiveStateHash() == base);

    // 换弹在 ammo 与 magazineAmmo 之间搬运：存档只存总量，哈希不应变化
    ammo = 29;
    magazineAmmo = 11;
    CHECK(archiveStateHash() == base);
    ammo = 0;
    magazineAmmo = 40;
    CHECK(archiveStateHash() == base);
    ammo = 30;
    magazineAmmo = 10;
    CHECK(archiveStateHash() == base);

    // 刻意排除的挂钟/瞬时状态：这些变化不该触发一次磁盘写入
    enemies[0].nextAttack += std::chrono::seconds(5);
    CHECK(archiveStateHash() == base);
    enemies[0].nextAttack = Clock::now();
    CHECK(archiveStateHash() == base);
    healing = true;
    CHECK(archiveStateHash() == base);
    healing = false;
    CHECK(archiveStateHash() == base);
    extracting = true;
    CHECK(archiveStateHash() == base);
    extracting = false;
    CHECK(archiveStateHash() == base);
    reloading = true;
    CHECK(archiveStateHash() == base);
    reloading = false;
    CHECK(archiveStateHash() == base);
    difficulty = Difficulty::Hard;
    CHECK(archiveStateHash() == base);
    difficulty = Difficulty::Normal;
    CHECK(archiveStateHash() == base);
    screen = Screen::Playing;
    CHECK(archiveStateHash() == base);
    screen = Screen::Menu;
    CHECK(archiveStateHash() == base);

    if (failures == 0) {
        std::printf("ALL PASSED\n");
        return 0;
    }
    std::printf("%d assertion(s) FAILED\n", failures);
    return 1;
}
