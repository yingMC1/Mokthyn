#pragma once
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
int scaled(int value) { return value; }

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

void drawText(int x, int y, const std::wstring& value, int size, ege::color_t color) {
    setbkmode(TRANSPARENT, frameBuffer);
    setfontbkcolor(TRANSPARENT, frameBuffer);
    setfont(std::max(1, scaled(size)), 0, L"Microsoft YaHei UI");
    settextcolor(color);
    outtextxy(scaled(x), scaled(y), value.c_str());
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

    if (networkActive && peerConnected && peerX > 0 && peerX < MapColumns - 1 && peerY > 0 &&
        peerY < MapRows - 1 && visible[peerY][peerX]) {
        const int peerCenterX = MapLeft + peerX * TileSize + TileSize / 2;
        const int peerCenterY = MapTop + peerY * TileSize + TileSize / 2;
        fillCircle(peerCenterX, peerCenterY, 15, EGERGB(37, 55, 76));
        fillCircle(peerCenterX, peerCenterY, 9, Cyan);
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
    if (networkActive && peerConnected)
        drawText(x + 268, 626, L"队友 " + std::to_wstring(peerHealth) + L" HP", 18,
                 peerHealth > 30 ? Cyan : Red);

    fillRect(x + 28, 668, x + 492, 669, EGERGB(52, 65, 68));
    drawText(x + 34, 691, L"WASD 移动    方向键射击", 17, Text);
    drawText(x + 34, 724, L"E 拾取   H 治疗   K 护甲", 17, Muted);
    drawText(x + 34, 757, L"Q 撤离   P 暂停   ESC 菜单", 17, Muted);

    if (Clock::now() < messageUntil) {
        fillRect(x + 24, 824, 1535, 872, PanelLight);
        drawText(x + 42, 837, message, 19, Yellow);
    }
}

void drawWeaponBackdrop() {
    fillRect(780, 120, 1460, 810, EGERGB(12, 17, 22));
    fillRect(780, 120, 815, 810, EGERGB(42, 54, 60));

    setlinecolor(EGERGB(120, 220, 170));
    setlinestyle(PS_SOLID, 2);
    line(1050, 650, 1180, 650);
    line(1180, 650, 1260, 610);
    line(1260, 610, 1360, 610);
    line(1360, 610, 1420, 545);
    line(1420, 545, 1435, 500);
    line(1435, 500, 1420, 445);
    line(1420, 445, 1345, 430);
    line(1345, 430, 1275, 490);
    line(1275, 490, 1210, 490);
    line(1210, 490, 1155, 550);
    line(1155, 550, 1050, 550);
    line(1050, 550, 980, 610);
    line(980, 610, 1025, 655);
    line(1025, 655, 1050, 650);

    setlinecolor(EGERGB(191, 211, 219));
    line(1105, 615, 1350, 615);
    line(1165, 580, 1288, 580);
    line(1220, 540, 1330, 540);
    line(1290, 505, 1385, 505);

    setfillcolor(EGERGB(24, 31, 38));
    solidellipse(1430, 500, 42, 20);
    setfillcolor(EGERGB(130, 216, 168));
    solidellipse(1430, 500, 10, 10);

    setlinecolor(EGERGB(94, 140, 154));
    setlinestyle(PS_DOT, 1);
    for (int i = 0; i < 6; ++i) {
        line(900 + i * 50, 195 + (i % 2) * 12, 980 + i * 50, 195 + (i % 2) * 12);
    }
    setlinestyle(PS_SOLID, 1);
}

void drawMenu() {
    cleardevice();
    fillRect(80, 78, 1520, 822, Panel);
    fillRect(80, 78, 90, 822, Green);
    drawWeaponBackdrop();

    drawText(150, 158, L"无限枪洲", 66, Text);
    drawText(155, 246, L"TACTICAL EXTRACTION", 20, Green);
    drawText(155, 333, L"深入失控区域，清除目标并带着物资撤离。", 25, Muted);
    fillRect(155, 405, 1442, 406, EGERGB(52, 65, 68));

    drawText(155, 440, L"1", 23, Green);
    drawText(205, 436, L"开始行动", 22, Text);
    drawText(155, 492, L"2", 23, Yellow);
    drawText(205, 488, L"黑市商店", 22, Text);
    drawText(155, 544, L"3", 23, Cyan);
    drawText(205, 540, L"操作说明", 22, Text);
    drawText(155, 596, L"4", 23, Green);
    drawText(205, 592, L"创建联机主机", 22, Text);
    drawText(155, 648, L"5", 23, Yellow);
    drawText(205, 644, L"加入局域网游戏", 22, Text);
    drawText(155, 700, L"ESC", 17, Muted);
    drawText(205, 696, L"退出", 20, Text);

    drawText(920, 455, L"弹药库存", 18, Muted);
    drawText(920, 489, std::to_wstring(ammo), 34, Yellow);
    drawText(1140, 455, L"医疗包", 18, Muted);
    drawText(1140, 489, std::to_wstring(medkits), 34, Cyan);
    drawText(920, 560, L"护甲", 18, Muted);
    drawText(920, 594, armorTier > 0 ? L"等级 " + std::to_wstring(armorTier) : L"无", 28, Green);
    drawText(1140, 560, L"持有金币", 18, Muted);
    drawText(1140, 594, std::to_wstring(money), 28, Yellow);
    drawText(155, 770, L"高清战术视图   /   EGE GRAPHICS", 17, Muted);
}

void drawNetworkLobby() {
    cleardevice();
    fillRect(150, 105, 1450, 795, Panel);
    const std::wstring title = screen == Screen::HostLobby ? L"创建联机主机"
                               : screen == Screen::JoinIp  ? L"加入局域网游戏"
                                                           : L"等待主机开始";
    drawText(220, 160, title, 42, Text);
    drawText(225, 250, networkStatus, 22, Green);
    if (screen == Screen::HostLobby) {
        drawText(225, 330, L"主机 IPv4 地址", 18, Muted);
        static const std::wstring localIp = getLocalIPv4();
        drawText(225, 372, localIp, 34, Cyan);
        drawText(225, 470, L"将此地址发给同一局域网内的队友。", 20, Text);
        drawText(225, 520,
                 peerConnected ? L"按 ENTER 开始合作行动" : L"等待队友连接 · UDP 端口 8888", 20,
                 Yellow);
    } else if (screen == Screen::JoinIp) {
        fillRect(220, 355, 1120, 425, PanelLight);
        drawText(245, 370,
                 hostIpInput.empty() ? L"输入主机 IPv4 地址..."
                                     : std::wstring(hostIpInput.begin(), hostIpInput.end()),
                 28, Text);
        drawText(225, 490, L"输入 IP 后按 ENTER 连接，BACKSPACE 删除，ESC 返回。", 18, Muted);
        drawText(225, 545, L"双方需在同一局域网，并允许程序通过 Windows 防火墙。", 18, Muted);
    } else {
        drawText(225, 365, L"连接成功。等待主机创建地图并开始行动。", 22, Text);
        drawText(225, 475, L"ESC 取消连接并返回主菜单。", 18, Muted);
    }
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

} // namespace mokthyn