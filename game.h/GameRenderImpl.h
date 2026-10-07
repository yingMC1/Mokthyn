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
#include <unordered_map>
#include <ws2tcpip.h>

namespace mokthyn {
int scaledLength(int value) {
    const int scalePercent =
        std::min(windowWidth * 100 / DesignWidth, windowHeight * 100 / DesignHeight);
    return value * scalePercent * uiScalePercent / 10000;
}

int scaledX(int value) {
    return value * windowWidth * uiScalePercent / (DesignWidth * 100);
}

int scaledY(int value) {
    return value * windowHeight * uiScalePercent / (DesignHeight * 100);
}

int visibleDesignWidth() { return DesignWidth * 100 / uiScalePercent; }

int visibleDesignHeight() { return DesignHeight * 100 / uiScalePercent; }

int battleTileSize() {
    return std::max(20, std::min((visibleDesignWidth() - 460) / MapColumns,
                                 (visibleDesignHeight() - 140) / MapRows));
}

int battleMapLeft() { return 30; }

int battleMapTop() { return 90; }

int battleHudLeft() { return battleMapLeft() + MapColumns * battleTileSize() + 24; }

int battleHudRight() { return visibleDesignWidth() - 24; }

int battleHudWidth() { return battleHudRight() - battleHudLeft(); }

void resizeGameWindow(int width, int height) {
    if (width == windowWidth && height == windowHeight) return;
    PIMAGE resizedBuffer = newimage(width, height);
    if (resizedBuffer == nullptr) {
        archiveStatus = L"无法分配新窗口画布";
        return;
    }

    settarget(nullptr);
    resizewindow(width, height);
    PIMAGE previousBuffer = frameBuffer;
    frameBuffer = resizedBuffer;
    windowWidth = width;
    windowHeight = height;
    setbkmode(TRANSPARENT, frameBuffer);
    setfontbkcolor(Background, frameBuffer);
    ege_enable_aa(true, frameBuffer);
    delimage(previousBuffer);
    settarget(frameBuffer);
}

std::wstring localize(const std::wstring& value) {
    if (!english) return value;
    static const std::unordered_map<std::wstring, std::wstring> translations = {
        {L"无限枪洲", L"Mokthyn"},
        {L"FIELD OPERATION  /  01", L"FIELD OPERATION  /  01"},
        {L"生命", L"Health"}, {L"弹药", L"Ammo"}, {L"发", L"rounds"},
        {L"医疗包", L"Medkits"}, {L"任务进度", L"Mission progress"},
        {L"目标", L"targets"}, {L"护甲", L"Armor"}, {L"未装备", L"Not equipped"},
        {L"无", L"None"},
        {L"等级 ", L"Tier "}, {L"金币", L"Coins"}, {L"队友 ", L"Ally "},
        {L" WASD 移动    方向键射击", L" WASD move    Arrow keys shoot"},
        {L"WASD 移动    方向键射击", L"WASD move    Arrow keys shoot"},
        {L"E 拾取   H 治疗   K 护甲", L"E loot   H heal   K armor"},
        {L"Q 撤离   P 暂停   ESC 菜单", L"Q extract   P pause   ESC menu"},
        {L"深入失控区域，清除目标并带着物资撤离。",
         L"Enter the hostile zone, eliminate targets, and extract with your loot."},
        {L"开始行动", L"Start a new run"}, {L"黑市商店", L"Black market"},
        {L"操作说明", L"Field manual"}, {L"创建联机主机", L"Host a co-op game"},
        {L"加入局域网游戏", L"Join a LAN game"}, {L"退出", L"Quit"},
        {L"1  开始行动", L"1  Start a new run"},
        {L"2  黑市商店", L"2  Black market"},
        {L"3  操作说明", L"3  Field manual"},
        {L"4  创建联机主机", L"4  Host a co-op game"},
        {L"5  加入局域网游戏", L"5  Join a LAN game"},
        {L"6  继续存档", L"6  Resume saved run"},
        {L"7  设置", L"7  Settings"},
        {L"ESC  退出", L"ESC  Quit"},
        {L"设置", L"Settings"}, {L"界面缩放", L"Interface scale"},
        {L"左右方向键调整比例", L"Use LEFT / RIGHT to adjust"},
        {L"按 0 恢复默认比例", L"Press 0 to restore the default"},
        {L"按 ENTER / ESC 返回主菜单", L"Press ENTER / ESC to return"},
        {L"当前比例", L"Current scale"},
        {L"（窗口尺寸不变）", L"(window size unchanged)"},
        {L"窗口宽度", L"Window width"},
        {L"窗口高度", L"Window height"},
        {L"↑ ↓ 选择项目，← → 调整", L"UP / DOWN select; LEFT / RIGHT adjust"},
        {L"按 0 恢复默认值", L"Press 0 to restore defaults"},
        {L"窗口尺寸已更新", L"Window size updated"},
        {L"无法分配新窗口画布", L"Could not allocate the resized window buffer"},
        {L"存档中的窗口尺寸无效", L"Invalid window dimensions in save file"},
        {L"存档中的界面比例无效", L"Invalid interface scale in save file"},
        {L"弹药库存", L"Ammo reserve"}, {L"持有金币", L"Coins"},
        {L"高清战术视图   /   EGE GRAPHICS", L"TACTICAL VIEW   /   EGE GRAPHICS"},
        {L"继续存档", L"Resume saved run"}, {L"切换语言：L", L"Language: L"},
        {L"存档已载入，可按 6 继续行动", L"Saved run available: press 6 to resume"},
        {L"已继续存档行动", L"Saved run resumed"},
        {L"存档格式无效，已使用默认数据", L"Invalid save file; using default data"},
        {L"存档地图数据无效，已使用默认数据", L"Invalid save map; using default data"},
        {L"存档敌人数据无效，已使用默认数据", L"Invalid saved enemy data; using default data"},
        {L"存档文件大小无效，已使用默认数据", L"Invalid save size; using default data"},
        {L"读取存档失败", L"Could not read save file"},
        {L"保存存档失败", L"Could not save game"},
        {L"无法创建存档目录", L"Could not create save directory"},
        {L"无法写入存档文件", L"Could not write save file"},
        {L"无法读取 APPDATA 环境变量，存档功能不可用",
         L"APPDATA is unavailable; saving is disabled"},
        {L"主机启动失败，检查端口 8888 是否被占用", L"Could not host; check whether UDP port 8888 is in use"},
        {L"网络初始化失败", L"Network initialization failed"},
        {L"下载页面无法打开", L"Could not open download page"},
        {L"创建联机主机", L"Host a co-op game"},
        {L"加入局域网游戏", L"Join a LAN game"}, {L"等待主机开始", L"Waiting for host"},
        {L"主机 IPv4 地址", L"Host IPv4 address"},
        {L"将此地址发给同一局域网内的队友。", L"Share this address with a teammate on the same LAN."},
        {L"按 ENTER 开始合作行动", L"Press ENTER to start the co-op run"},
        {L"等待队友连接 · UDP 端口 8888", L"Waiting for teammate · UDP port 8888"},
        {L"等待玩家连接 · UDP 8888", L"Waiting for players · UDP 8888"},
        {L"输入主机的局域网 IPv4 地址", L"Enter the host's LAN IPv4 address"},
        {L"请使用 ipconfig 查看本机 IP", L"Run ipconfig to find this computer's IP"},
        {L"网络接收错误", L"Network receive error"},
        {L"玩家已连接，按 ENTER 开始", L"Player connected; press ENTER to start"},
        {L"已连接，等待主机开始", L"Connected; waiting for host to start"},
        {L"队友已离开", L"Teammate left"},
        {L"主机连接中断", L"Connection to host lost"},
        {L"正在连接主机...", L"Connecting to host..."},
        {L"IP 格式无效，请输入 IPv4 地址", L"Invalid IP format; enter an IPv4 address"},
        {L"输入主机 IPv4 地址...", L"Enter host IPv4 address..."},
        {L"输入 IP 后按 ENTER 连接，BACKSPACE 删除，ESC 返回。",
         L"Enter the IP and press ENTER to connect. BACKSPACE deletes; ESC returns."},
        {L"双方需在同一局域网，并允许程序通过 Windows 防火墙。",
         L"Both players must share a LAN and allow the game through Windows Firewall."},
        {L"连接成功。等待主机创建地图并开始行动。", L"Connected. Waiting for the host to start the run."},
        {L"ESC 取消连接并返回主菜单。", L"Press ESC to cancel and return to the main menu."},
        {L"黑市", L"Black Market"}, {L"补充装备，为下一次行动做好准备",
         L"Stock up and prepare for your next run"},
        {L"一级护甲       耐久 50 / 减伤 4       100 金",
         L"Tier 1 armor     50 durability / -4 damage     100 coins"},
        {L"二级护甲       耐久 75 / 减伤 7       200 金",
         L"Tier 2 armor     75 durability / -7 damage     200 coins"},
        {L"三级护甲       耐久 100 / 减伤 10     350 金",
         L"Tier 3 armor     100 durability / -10 damage   350 coins"},
        {L"四级护甲       耐久 125 / 减伤 13     550 金",
         L"Tier 4 armor     125 durability / -13 damage   550 coins"},
        {L"弹药补给       +10 发                 5 金", L"Ammo refill     +10 rounds                 5 coins"},
        {L"医疗包         +1 个                  20 金", L"Medkit          +1                         20 coins"},
        {L"↑ / ↓ 选择     ENTER 购买     0 返回", L"↑ / ↓ Select     ENTER Buy     0 Return"},
        {L"护甲升级会替换当前护甲。", L"Buying armor replaces your current armor."},
        {L"行动简报", L"Field Manual"},
        {L"行动简报 · 操作与战斗", L"Field Manual · Controls and Combat"},
        {L"行动简报 · 目标与物资", L"Field Manual · Objectives and Supplies"},
        {L"行动简报 · 商店、存档与联机", L"Field Manual · Shop, Saves, and Co-op"},
        {L"W A S D        在地图中移动", L"W A S D        Move around the map"},
        {L"方向键          朝对应方向射击；子弹遇墙会停止",
         L"Arrow keys      Shoot that way; bullets stop at walls"},
        {L"方向键          朝对应方向射击，子弹可穿过空地",
         L"Arrow keys      Shoot in that direction; shots travel through open ground"},
        {L"E               拾取脚下的弹药或医疗包", L"E               Pick up ammo or a medkit underfoot"},
        {L"H               消耗医疗包，等待 3 秒恢复 20 点生命",
         L"H               Use a medkit; stay safe for 3 seconds to restore 20 health"},
        {L"K               穿上或脱下已购买的护甲", L"K               Equip or unequip purchased armor"},
        {L"Q               到达撤离点后即可开始撤离",
         L"Q               Start extracting once you reach the extraction point"},
        {L"P / ESC         暂停行动 / 返回主菜单", L"P / ESC         Pause / return to the main menu"},
        {L"敌人靠近或进入视线后会苏醒并追击你。",
         L"Enemies wake up and pursue you when nearby or in sight."},
        {L"视野以外的地图区域会变暗。", L"Areas outside your line of sight are darkened."},
        {L"按任意数字键返回", L"Press ESC or ENTER to return"},
        {L"按 ← → 或 ↑ ↓ 翻页", L"Use ← → or ↑ ↓ to change pages"},
        {L"按 D 打开下载页面", L"Press D to open the download page"},
        {L"清除地图中的敌人，随后前往右下角的绿色撤离点。",
         L"Clear the enemies, then reach the green extraction point in the bottom-right."},
        {L"站在撤离点上按 Q，保持 5 秒不移动且不受攻击即可撤离。",
         L"Press Q at the extraction point. Hold for 5 seconds without moving or taking damage."},
        {L"击杀敌人获得 10 金币；搜寻地图上的黄色弹药和蓝色医疗包。",
         L"Earn 10 coins per kill; search for yellow ammo and blue medkits."},
        {L"方向键射击，每发消耗 1 弹；按住 WASD 可连续移动。",
         L"Arrow keys fire (1 round each); hold WASD to keep moving."},
        {L"护甲可减伤并消耗耐久；K 可暂时脱下以保留耐久。",
         L"Armor reduces damage and loses durability; press K to unequip it temporarily."},
        {L"被击中会打断治疗和撤离；移动也会取消正在进行的动作。",
         L"Damage interrupts healing and extraction; moving also cancels these actions."},
        {L"生命归零时行动失败；撤离成功会保留当前携带物资。",
         L"Your run fails at zero health; extracted supplies are kept."},
        {L"主菜单按 2 打开黑市：1–4 购买护甲，5 补弹，6 购买医疗包。",
         L"Press 2 for the market: 1–4 buy armor, 5 buys ammo, and 6 buys a medkit."},
        {L"行动进度每 2 秒自动保存到 %APPDATA%\\mokthyn\\save.dat。",
         L"Run progress autosaves every 2 seconds to %APPDATA%\\mokthyn\\save.dat."},
        {L"重新启动游戏后，主菜单按 6 继续最近的单人行动；按 1 开始新行动。",
         L"After restarting, press 6 to resume the last solo run or 1 to start a new run."},
        {L"按 L 切换语言；主菜单按 7 调整并保存界面缩放比例。",
         L"Press L to switch language; press 7 on the main menu to adjust and save interface scale."},
        {L"联机：主机按 4，队友按 5 并输入主机 IPv4；双方需在同一局域网。",
         L"Co-op: host presses 4; teammate presses 5 and enters the host IPv4. Same LAN required."},
        {L"联机使用 UDP 8888；若无法连接，请检查防火墙和路由器设置。",
         L"Co-op uses UDP 8888; check firewall and router settings if connections fail."},
        {L"下载地址：Mokthyn - Download", L"Download: Mokthyn - Download"},
        {L"按 ← → 或 ↑ ↓ 翻页    按 D 打开下载页面    ENTER / ESC 返回",
         L"← → or ↑ ↓ pages    D opens download    ENTER / ESC returns"},
        {L"撤离成功", L"Extraction Successful"}, {L"行动结束", L"Run Over"},
        {L"本次击杀", L"Kills this run"}, {L"剩余弹药", L"Ammo remaining"},
        {L"按 ENTER 返回主菜单", L"Press ENTER to return to the main menu"},
        {L"已暂停", L"Paused"}, {L"按 P 继续  /  ESC 返回菜单", L"P resume  /  ESC menu"},
        {L"按 P 继续  /  ESC 返回菜单", L"P resume  /  ESC menu"},
        {L"下载地址：Mokthyn - Download", L"Download: Mokthyn - Download"},
        {L"清除区域内的敌人，然后前往撤离点", L"Clear the area, then reach the extraction point"},
        {L"弹药不足，寻找弹药箱", L"Out of ammo; find an ammo cache"},
        {L"目标消灭  +10 金币", L"Target eliminated  +10 coins"},
        {L"命中目标", L"Target hit"}, {L"未命中", L"Miss"},
        {L"护甲已损坏", L"Armor broken"}, {L"受到攻击，动作已中断", L"Hit taken; action interrupted"},
        {L"受到攻击", L"Under attack"}, {L"拾取弹药  +", L"Ammo picked up  +"},
        {L"拾取医疗包  +1", L"Medkit picked up  +1"},
        {L"这里没有可拾取的物资", L"No supplies to pick up here"},
        {L"动作已取消", L"Action cancelled"}, {L"金币不足", L"Not enough coins"},
        {L"护甲已装备", L"Armor equipped"}, {L"弹药补给完成", L"Ammo refilled"},
        {L"医疗包已补充", L"Medkit purchased"}, {L"没有可用护甲", L"No usable armor"},
        {L"护甲已穿上", L"Armor equipped"}, {L"护甲已脱下", L"Armor unequipped"},
        {L"治疗中，请保持安全", L"Healing; stay safe"},
        {L"现在无法治疗", L"Cannot heal right now"},
        {L"治療完成  +20 生命", L"Healing complete  +20 health"},
        {L"治疗完成  +20 生命", L"Healing complete  +20 health"},
        {L"前往地图上的撤离点", L"Reach the extraction point on the map"},
        {L"撤离开始，保持位置 5 秒", L"Extraction started; hold position for 5 seconds"},
        {L"治疗中  ", L"Healing  "}, {L"撤离中  ", L"Extracting  "},
        {L"目标消灭", L"Target eliminated"}
    };
    const auto found = translations.find(value);
    if (found != translations.end()) return found->second;
    if (value.rfind(L"拾取弹药  +", 0) == 0) return L"Ammo picked up  +" + value.substr(7);
    if (value.rfind(L"等级 ", 0) == 0) return L"Tier " + value.substr(3);
    if (value.rfind(L"队友 ", 0) == 0) return L"Ally " + value.substr(3);
    if (value.rfind(L"治疗中  ", 0) == 0) {
        std::wstring seconds = value.substr(5);
        if (seconds.size() >= 2 && seconds.substr(seconds.size() - 2) == L" 秒")
            seconds.resize(seconds.size() - 2);
        return L"Healing  " + seconds + L" sec";
    }
    if (value.rfind(L"撤离中  ", 0) == 0) {
        std::wstring seconds = value.substr(5);
        if (seconds.size() >= 2 && seconds.substr(seconds.size() - 2) == L" 秒")
            seconds.resize(seconds.size() - 2);
        return L"Extracting  " + seconds + L" sec";
    }
    const size_t targetsSuffix = value.rfind(L"  目标");
    if (targetsSuffix != std::wstring::npos && targetsSuffix + 4 == value.size())
        return value.substr(0, targetsSuffix) + L"  targets";
    return value;
}

void fillRect(int left, int top, int right, int bottom, ege::color_t color) {
    setfillcolor(color);
    bar(scaledX(left), scaledY(top), scaledX(right), scaledY(bottom));
}

void outlineRect(int left, int top, int right, int bottom, ege::color_t color) {
    setlinecolor(color);
    rectangle(scaledX(left), scaledY(top), scaledX(right), scaledY(bottom));
}

void fillCircle(int x, int y, int radius, ege::color_t color) {
    setfillcolor(color);
    fillcircle(scaledX(x), scaledY(y), std::max(1, scaledLength(radius)));
}

void drawText(int x, int y, const std::wstring& value, int size, ege::color_t color) {
    const std::wstring displayed = localize(value);
    setbkmode(TRANSPARENT, frameBuffer);
    setfontbkcolor(TRANSPARENT, frameBuffer);
    setfont(std::max(1, scaledLength(size)), 0, L"Microsoft YaHei UI");
    settextcolor(color);
    outtextxy(scaledX(x), scaledY(y), displayed.c_str());
}

void drawBar(int x, int y, int width, int height, int value, int maximum, ege::color_t color) {
    fillRect(x, y, x + width, y + height, EGERGB(44, 57, 62));
    const int filled = maximum > 0 ? width * std::clamp(value, 0, maximum) / maximum : 0;
    if (filled > 0) fillRect(x, y, x + filled, y + height, color);
}

void drawMap() {
    computeVisibility();
    const int tileSize = battleTileSize();
    const int mapLeft = battleMapLeft();
    const int mapTop = battleMapTop();
    fillRect(mapLeft - 4, mapTop - 4, mapLeft + MapColumns * tileSize + 4,
             mapTop + MapRows * tileSize + 4, EGERGB(9, 14, 18));

    for (int y = 0; y < MapRows; ++y) {
        for (int x = 0; x < MapColumns; ++x) {
            const int left = mapLeft + x * tileSize;
            const int top = mapTop + y * tileSize;
            if (!visible[y][x]) {
                fillRect(left, top, left + tileSize - 1, top + tileSize - 1,
                         EGERGB(16, 23, 28));
                continue;
            }

            ege::color_t tileColor = EGERGB(34, 47, 51);
            if (world[y][x] == Wall) tileColor = EGERGB(67, 77, 75);
            if (world[y][x] == ExitTile) tileColor = EGERGB(34, 78, 63);
            fillRect(left, top, left + tileSize - 1, top + tileSize - 1, tileColor);
            outlineRect(left, top, left + tileSize - 1, top + tileSize - 1,
                        EGERGB(24, 35, 39));

            if (world[y][x] == AmmoPickup) {
                fillCircle(left + tileSize / 2, top + tileSize / 2, tileSize / 5, Yellow);
                drawText(left + tileSize / 2 - 7, top + tileSize / 2 - 10, L"+", 19, Background);
            } else if (world[y][x] == MedkitPickup) {
                fillCircle(left + tileSize / 2, top + tileSize / 2, tileSize / 5, Cyan);
                drawText(left + tileSize / 2 - 7, top + tileSize / 2 - 10, L"+", 20, Background);
            } else if (world[y][x] == ExitTile) {
                outlineRect(left + 8, top + 8, left + tileSize - 9, top + tileSize - 9, Green);
                drawText(left + tileSize / 2 - 6, top + tileSize / 2 - 10, L"E", 19, Green);
            }

            for (const Enemy& enemy : enemies) {
                if (!enemy.alive || enemy.x != x || enemy.y != y) continue;
                fillCircle(left + tileSize / 2, top + tileSize / 2, tileSize * 3 / 10, Red);
                fillCircle(left + tileSize / 2 - 3, top + tileSize / 2 - 3, tileSize / 12,
                           EGERGB(255, 174, 137));
                break;
            }
        }
    }

    if (networkActive && peerConnected && peerX > 0 && peerX < MapColumns - 1 && peerY > 0 &&
        peerY < MapRows - 1 && visible[peerY][peerX]) {
        const int peerCenterX = mapLeft + peerX * tileSize + tileSize / 2;
        const int peerCenterY = mapTop + peerY * tileSize + tileSize / 2;
        fillCircle(peerCenterX, peerCenterY, tileSize * 2 / 5, EGERGB(37, 55, 76));
        fillCircle(peerCenterX, peerCenterY, tileSize / 4, Cyan);
    }

    const int playerCenterX = mapLeft + playerX * tileSize + tileSize / 2;
    const int playerCenterY = mapTop + playerY * tileSize + tileSize / 2;
    fillCircle(playerCenterX, playerCenterY, tileSize / 2, EGERGB(37, 70, 62));
    fillCircle(playerCenterX, playerCenterY, tileSize / 3, Green);
    fillCircle(playerCenterX + 3, playerCenterY - 3, tileSize / 10, Text);
}

void drawHud() {
    const int x = battleHudLeft();
    const int right = battleHudRight();
    const int width = battleHudWidth();
    const int padding = 20;
    const int valueX = x + std::max(120, width - 150);
    fillRect(x, 74, right, visibleDesignHeight() - 22, Panel);
    fillRect(x, 74, x + std::min(12, width / 10), visibleDesignHeight() - 22, Green);

    drawText(x + padding, 94, L"无限枪洲", 31, Text);
    drawText(x + padding, 139, L"FIELD OPERATION  /  01", 14, Muted);
    drawText(x + padding, 183, L"生命", 17, Muted);
    drawText(valueX, 181, std::to_wstring(health) + L" / 100", 18,
             health > 60 ? Green : (health > 30 ? Yellow : Red));
    drawBar(x + padding, 211, width - padding * 2, 12, health, 100, health > 30 ? Green : Red);

    drawText(x + padding, 245, L"弹药", 17, Muted);
    drawText(x + padding, 270, std::to_wstring(ammo), 34, Yellow);
    drawText(x + 105, 287, L"发", 16, Muted);
    drawText(x + std::max(180, width / 2), 245, L"医疗包", 17, Muted);
    drawText(x + std::max(180, width / 2), 270, std::to_wstring(medkits), 34, Cyan);

    fillRect(x + padding, 324, right - padding, 325, EGERGB(52, 65, 68));
    drawText(x + padding, 344, L"任务进度", 17, Muted);
    drawText(x + padding, 373,
             std::to_wstring(kills) + L" / " + std::to_wstring(totalEnemies) + L"  目标", 21,
             Text);
    drawBar(x + padding, 410, width - padding * 2, 8, kills, totalEnemies, Green);

    drawText(x + padding, 444, L"护甲", 17, Muted);
    drawText(x + 100, 442, armorOn ? L"等级 " + std::to_wstring(armorTier) : L"未装备", 17,
             armorOn ? Cyan : Muted);
    if (armorOn)
        drawBar(x + padding, 470, width - padding * 2, 8, armorDurability,
                ArmorDurability[armorTier], Cyan);
    drawText(x + padding, 496, L"金币", 17, Muted);
    drawText(x + 100, 494, std::to_wstring(money), 20, Yellow);
    if (networkActive && peerConnected)
        drawText(x + std::max(200, width / 2), 494,
                 L"队友 " + std::to_wstring(peerHealth) + L" HP", 16,
                 peerHealth > 30 ? Cyan : Red);

    fillRect(x + padding, 532, right - padding, 533, EGERGB(52, 65, 68));
    drawText(x + padding, 550, L"WASD 移动    方向键射击", 15, Text);
    drawText(x + padding, 578, L"E 拾取   H 治疗   K 护甲", 15, Muted);
    drawText(x + padding, 606, L"Q 撤离   P 暂停   ESC 菜单", 15, Muted);
    if (Clock::now() < messageUntil)
        drawText(x + padding, 642, message, 16, Yellow);
}

void drawMenu() {
    cleardevice();
    setfillcolor(Background);
    bar(0, 0, windowWidth, windowHeight);

    drawText(500, 210, L"无限枪洲", 58, Text);
    drawText(1190, 225, L"切换语言：L", 16, Muted);

    drawText(500, 330, L"1  开始行动", 23, Green);
    drawText(500, 385, L"2  黑市商店", 23, Yellow);
    drawText(500, 440, L"3  操作说明", 23, Cyan);
    drawText(500, 495, L"4  创建联机主机", 23, Text);
    drawText(500, 550, L"5  加入局域网游戏", 23, Text);
    if (hasSavedRun) {
        drawText(850, 330, L"6  继续存档", 23, Cyan);
    }
    drawText(850, 385, L"7  设置", 23, Cyan);
    drawText(850, 440, L"ESC  退出", 23, Muted);
    if (!archiveStatus.empty()) drawText(500, 650, archiveStatus, 16, Yellow);
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
    fillRect(0, 0, visibleDesignWidth(), visibleDesignHeight(), Panel);
    const std::array<std::array<std::wstring, 8>, 3> pages = {{
        {{L"行动简报 · 操作与战斗", L"W A S D        在地图中移动",
          L"方向键          朝对应方向射击；子弹遇墙会停止",
          L"E               拾取脚下的弹药或医疗包",
          L"H               消耗医疗包；安全等待 3 秒恢复 20 点生命",
          L"K               穿上或脱下已购买的护甲",
          L"敌人靠近或进入视线后会苏醒并追击你。",
          L"视野以外的地图区域会变暗。"}},
        {{L"行动简报 · 目标与物资", L"清除地图中的敌人，随后前往右下角的绿色撤离点。",
          L"站在撤离点上按 Q，保持 5 秒不移动且不受攻击即可撤离。",
          L"击杀敌人获得 10 金币；搜寻地图上的黄色弹药和蓝色医疗包。",
          L"方向键射击，每发消耗 1 弹；按住 WASD 可连续移动。",
          L"护甲可减伤并消耗耐久；K 可暂时脱下以保留耐久。",
          L"被击中会打断治疗和撤离；移动也会取消正在进行的动作。",
          L"生命归零时行动失败；撤离成功会保留当前携带物资。"}},
        {{L"行动简报 · 商店、存档与联机", L"主菜单按 2 打开黑市：1–4 购买护甲，5 补弹，6 购买医疗包。",
          L"商店购买会立即保存；行动进度会自动存入 %APPDATA%\\mokthyn\\save.dat。",
          L"重新启动游戏后，主菜单按 6 继续最近的单人行动；按 1 开始新行动。",
          L"按 L 切换语言；主菜单按 7 调整并保存界面缩放比例。",
          L"联机：主机按 4，队友按 5 并输入主机 IPv4；双方需在同一局域网。",
          L"联机使用 UDP 8888；若无法连接，请检查防火墙和路由器设置。",
          L"下载：Mokthyn - Download"}}
    }};
    drawText(150, 110, pages[helpPage][0], 46, Text);
    for (int i = 1; i < 8; ++i)
        drawText(160, 180 + (i - 1) * 54, pages[helpPage][i], 26, i == 7 ? Green : Text);
    drawText(160, 580, L"下载地址：Mokthyn - Download", 19, Cyan);
    drawText(160, 612, DownloadUrl, 19, Cyan);
    drawText(160, 660, L"按 ← → 或 ↑ ↓ 翻页    按 D 打开下载页面    ENTER / ESC 返回", 19, Muted);
    drawText(visibleDesignWidth() - 110, 115, std::to_wstring(helpPage + 1) + L" / 3", 22,
             Green);
}

void drawSettings() {
    cleardevice();
    setfillcolor(Background);
    bar(0, 0, windowWidth, windowHeight);
    fillRect(80, 80, visibleDesignWidth() - 80, visibleDesignHeight() - 80, Panel);
    drawText(130, 115, L"设置", 42, Text);

    const std::array<std::wstring, 3> labels = {
        L"界面缩放", L"窗口宽度", L"窗口高度"};
    const std::array<std::wstring, 3> values = {
        std::to_wstring(uiScalePercent) + L"%",
        std::to_wstring(windowWidth) + L" px",
        std::to_wstring(windowHeight) + L" px"};
    for (int i = 0; i < static_cast<int>(labels.size()); ++i) {
        const int y = 230 + i * 100;
        const bool selected = settingsSelection == i;
        if (selected) fillRect(130, y - 15, visibleDesignWidth() - 130, y + 55, PanelLight);
        drawText(160, y, labels[i], 25, selected ? Green : Text);
        drawText(650, y, values[i], 27, selected ? Cyan : Muted);
        drawText(visibleDesignWidth() - 220, y, L"←   →", 20, Muted);
    }
    drawText(160, 500, L"↑ ↓ 选择项目，← → 调整", 20, Text);
    drawText(160, 550, L"按 0 恢复默认值", 18, Muted);
    drawText(160, 600, L"按 ENTER / ESC 返回主菜单", 18, Muted);
    if (!archiveStatus.empty()) drawText(160, 650, archiveStatus, 16, Yellow);
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
    setfillcolor(Background);
    bar(0, 0, windowWidth, windowHeight);
    drawText(64, 51, L"OPERATIONS / EXTRACTION ZONE", 16, Muted);
    drawMap();
    drawHud();
    if (healing) {
        const int seconds =
            std::max(0, 3 - static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(
                                                 Clock::now() - healStarted)
                                                 .count()));
        drawText(battleMapLeft() + 12, visibleDesignHeight() - 42,
                 L"治疗中  " + std::to_wstring(seconds) + L" 秒", 18, Cyan);
    } else if (extracting) {
        const int seconds =
            std::max(0, 5 - static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(
                                                 Clock::now() - extractStarted)
                                                 .count()));
            drawText(battleMapLeft() + 12, visibleDesignHeight() - 42,
                     L"撤离中  " + std::to_wstring(seconds) + L" 秒", 18, Green);
    }
}

} // namespace mokthyn