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

int battleMapLeft() { return 30; }

int battleMapTop() { return 90; }

int battleHudLeft() { return visibleDesignWidth() - 406; }

int battleHudRight() { return visibleDesignWidth() - 24; }

int battleMapAreaWidth() { return battleHudLeft() - 24 - battleMapLeft(); }

int battleMapAreaHeight() { return visibleDesignHeight() - 50 - battleMapTop(); }

int battleHudWidth() { return battleHudRight() - battleHudLeft(); }

int battleTileSize() {
    const int byWidth = battleMapAreaWidth() / BattleTilesX;
    const int byHeight = battleMapAreaHeight() / BattleTilesY;
    return std::clamp(std::min(byWidth, byHeight), 18, 40);
}

int battleTilesX() { return battleMapAreaWidth() / battleTileSize(); }

int battleTilesY() { return battleMapAreaHeight() / battleTileSize(); }

int battleCameraX() {
    const int tx = battleTilesX();
    if (tx >= MapColumns) return -(tx - MapColumns) / 2;
    return std::clamp(playerX - tx / 2, 0, MapColumns - tx);
}

int battleCameraY() {
    const int ty = battleTilesY();
    if (ty >= MapRows) return -(ty - MapRows) / 2;
    return std::clamp(playerY - ty / 2, 0, MapRows - ty);
}

int tileScreenX(int x) { return battleMapLeft() + (x - battleCameraX()) * battleTileSize(); }

int tileScreenY(int y) { return battleMapTop() + (y - battleCameraY()) * battleTileSize(); }

// ===== 鼠标按钮化 UI 工具 =====
struct MenuRect {
    int left, top, right, bottom;
};

bool mouseDesignPos(int px, int py, int& dx, int& dy) {
    dx = static_cast<int>(static_cast<long long>(px) * DesignWidth * 100 /
                          (windowWidth * uiScalePercent));
    dy = static_cast<int>(static_cast<long long>(py) * DesignHeight * 100 /
                          (windowHeight * uiScalePercent));
    return dx >= 0 && dy >= 0;
}

bool designPointInRect(int dx, int dy, const MenuRect& rect) {
    return dx >= rect.left && dx <= rect.right && dy >= rect.top && dy <= rect.bottom;
}

bool pollMouseClick(int& dx, int& dy) {
    while (mousemsg()) {
        const mouse_msg message = getmouse();
        if (message.is_left() && message.is_down() &&
            mouseDesignPos(message.x, message.y, dx, dy))
            return true;
    }
    return false;
}

int textWidthEstimate(const std::wstring& value, int size) {
    int width = 0;
    for (const wchar_t character : value) {
        if (character >= 0x4E00 && character <= 0x9FFF)
            width += size;
        else if (character == L' ')
            width += size / 2;
        else
            width += size * 6 / 10;
    }
    return width;
}

void drawTextCentered(const MenuRect& rect, const std::wstring& value, int size,
                      ege::color_t color) {
    const int textWidth = textWidthEstimate(value, size);
    drawText((rect.left + rect.right) / 2 - textWidth / 2,
             (rect.top + rect.bottom) / 2 - size * 3 / 4, value, size, color);
}

void drawButton(const MenuRect& rect, const std::wstring& label, int fontSize,
                ege::color_t accent, bool enabled, int hoverX, int hoverY) {
    const bool hovered = enabled && designPointInRect(hoverX, hoverY, rect);
    const ege::color_t fillColor =
        enabled ? (hovered ? EGERGB(40, 56, 64) : PanelLight) : EGERGB(18, 25, 30);
    const ege::color_t borderColor =
        enabled ? (hovered ? accent : EGERGB(64, 82, 88)) : Muted;
    const int radius = scaledLength(12);
    const int left = scaledX(rect.left);
    const int top = scaledY(rect.top);
    const int right = scaledX(rect.right);
    const int bottom = scaledY(rect.bottom);
    setfillcolor(fillColor);
    fillroundrect(left, top, right, bottom, radius, radius);
    setlinecolor(borderColor);
    roundrect(left, top, right, bottom, radius, radius);
    drawTextCentered(rect, label, fontSize,
                     enabled ? (hovered ? accent : Text) : Muted);
}

// ===== 界面按钮布局（设计坐标系 1600×900）=====
MenuRect menuLeftButton(int index) { return {440, 268 + index * 88, 780, 334 + index * 88}; }
MenuRect menuRightButton(int index) { return {820, 268 + index * 88, 1160, 334 + index * 88}; }
MenuRect menuBottomButton(int index) { return {450 + index * 320, 760, 700 + index * 320, 830}; }
MenuRect menuLanguageButton() { return {1420, 140, 1560, 200}; }
MenuRect shopItemRect(int index) { return {155, 240 + index * 74, 1440, 296 + index * 74}; }
MenuRect shopBackButton() { return {155, 700, 430, 766}; }
MenuRect helpPrevButton() { return {150, 655, 330, 720}; }
MenuRect helpNextButton() { return {390, 655, 570, 720}; }
MenuRect helpBackButton() { return {620, 655, 880, 720}; }
MenuRect settingsRowRect(int index) { return {130, 215 + index * 100, 1470, 265 + index * 100}; }
MenuRect settingsMinusButton(int index) { return {1180, 215 + index * 100, 1300, 265 + index * 100}; }
MenuRect settingsPlusButton(int index) { return {1330, 215 + index * 100, 1450, 265 + index * 100}; }
MenuRect settingsResetButton() { return {1150, 540, 1420, 600}; }
MenuRect settingsBackButton() { return {130, 620, 420, 690}; }
MenuRect pauseResumeButton() { return {620, 460, 840, 530}; }
MenuRect pauseExitButton() { return {860, 460, 1080, 530}; }
MenuRect resultBackButton() { return {620, 520, 980, 590}; }
MenuRect lobbyStartButton() { return {620, 600, 980, 670}; }
MenuRect lobbyBackButton() { return {180, 680, 460, 750}; }

bool resizeGameWindow(int width, int height) {
    if (width == windowWidth && height == windowHeight) return true;
    PIMAGE resizedBuffer = newimage(width, height);
    if (resizedBuffer == nullptr) {
        archiveStatus = L"无法分配新窗口画布";
        return false;
    }

    settarget(nullptr);
    resizewindow(width, height);
    PIMAGE previousBuffer = frameBuffer;
    frameBuffer = resizedBuffer;
    ++frameBufferGeneration;
    windowWidth = width;
    windowHeight = height;
    setbkmode(TRANSPARENT, frameBuffer);
    setfontbkcolor(Background, frameBuffer);
    ege_enable_aa(true, frameBuffer);
    delimage(previousBuffer);
    settarget(frameBuffer);
    return true;
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
        {L"存档解密失败，文件可能已损坏或由其他 Windows 用户创建",
         L"Could not decrypt save; it may be damaged or belong to another Windows user"},
        {L"存档加密失败", L"Could not encrypt save file"},
        {L"网络消息确认超时", L"Network message acknowledgement timed out"},
        {L"可靠网络消息队列已满", L"Reliable network message queue is full"},
        {L"命中确认中", L"Hit sent; awaiting confirmation"},
        {L"射击确认中", L"Shot sent; awaiting confirmation"},
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
        {L"存档设置区块无效", L"Invalid save settings block"},
        {L"存档显示设置有损坏，已保留未损坏的设置",
         L"Some display settings were damaged; valid settings were preserved"},
        {L"存档行动区块无效，已保留显示设置",
         L"Invalid run data; display settings were preserved"},
        {L"存档行动数据无效，已保留显示设置",
         L"Invalid run data; display settings were preserved"},
        {L"存档地图区块无效，已保留显示设置",
         L"Invalid map block; display settings were preserved"},
        {L"存档地图数据无效，已保留显示设置",
         L"Invalid map data; display settings were preserved"},
        {L"存档敌人区块无效，已保留显示设置",
         L"Invalid enemy block; display settings were preserved"},
        {L"存档敌人数据无效，已保留显示设置",
         L"Invalid enemy data; display settings were preserved"},
        {L"存档已读取，但自动加密迁移失败，下次启动将重试",
         L"Save loaded, but encryption migration failed; it will be retried next launch"},
        {L"保存存档失败", L"Could not save game"},
        {L"无法创建存档目录", L"Could not create save directory"},
        {L"无法写入存档文件", L"Could not write save file"},
        {L"无法读取 APPDATA 环境变量，存档功能不可用",
         L"APPDATA is unavailable; saving is disabled"},
        {L"主机启动失败，检查 UDP 端口是否被占用",
         L"Could not host; check whether the UDP port is in use"},
        {L"网络初始化失败", L"Network initialization failed"},
        {L"下载页面无法打开", L"Could not open download page"},
        {L"下载地址：Mokthyn - Download", L"Download: Mokthyn - Download"},
        {L"等待主机开始", L"Waiting for host"},
        {L"主机 IPv4 地址", L"Host IPv4 address"},
        {L"将此地址发给同一局域网内的队友。", L"Share this address with a teammate on the same LAN."},
        {L"按 ENTER 开始合作行动", L"Press ENTER to start the co-op run"},
        {L"输入主机的局域网 IPv4 地址", L"Enter the host's LAN IPv4 address"},
        {L"请使用 ipconfig 查看本机 IP", L"Run ipconfig to find this computer's IP"},
        {L"网络接收错误", L"Network receive error"},
        {L"玩家已连接，按 ENTER 开始", L"Player connected; press ENTER to start"},
        {L"已连接，等待主机开始", L"Connected; waiting for host to start"},
        {L"队友已离开", L"Teammate left"},
        {L"队友已重新连接", L"Teammate reconnected"},
        {L"队友连接中断", L"Ally disconnected"},
        {L"对手已断开连接", L"Opponent disconnected"},
        {L"主存档损坏，已从备份恢复", L"Main save corrupted; recovered from backup"},
        {L"生命危险，尽快治疗！", L"CRITICAL health; heal now!"},
        {L"PVP 模式下战斗至最后一人，无法撤离", L"PVP: fight to the last player; extraction disabled"},
        {L"对局未完成直接退出，已按失败结算", L"Run exited unfinished; scored as a loss"},
        {L"难度：简单", L"Difficulty: Easy"}, {L"难度：普通", L"Difficulty: Normal"},
        {L"难度：困难", L"Difficulty: Hard"}, {L"快速开始", L"Quick start"},
        {L"PVP 对战", L"PVP battle"}, {L"语言 L", L"Language L"},
        {L"切换语言 L", L"Toggle language L"},
        {L"开始行动  ENTER", L"Start run  ENTER"}, {L"返回  ESC", L"Back  ESC"},
        {L"返回  0", L"Back  0"}, {L"← 上一页", L"← Prev page"},
        {L"下一页 →", L"Next page →"}, {L"返回  ENTER / ESC", L"Back  ENTER / ESC"},
        {L"恢复默认  0", L"Restore defaults  0"}, {L"继续  P", L"Resume  P"},
        {L"返回菜单  ESC", L"Exit to menu  ESC"}, {L"返回主菜单  ENTER", L"Back to menu  ENTER"},
        {L"换弹完成", L"Reload complete"},
        {L"备弹不足", L"Out of reserve ammo"},
        {L"弹匣已空，开始换弹", L"Magazine empty; reloading"},
        {L"开始换弹", L"Reloading"},
        {L"挥刀", L"Knife swing"},
        {L"挥空", L"Whiffed"},
        {L"挥刀请求发送失败", L"Melee request failed"},
        {L"命中对手", L"Opponent hit"},
        {L"目标消灭  +10 金币", L"Target eliminated  +10 coins"},
        {L"刀杀敌人  +10 金币", L"Knife kill  +10 coins"},
        {L"刀杀 Boss  +100 金币", L"Knife kill on boss  +100 coins"},
        {L"射击请求发送失败", L"Shot request failed"},
        {L"换弹请求发送失败", L"Reload request failed"},
        {L"治疗请求发送失败", L"Heal request failed"},
        {L"治疗确认发送失败", L"Heal confirmation failed"},
        {L"正在请求主机确认弹药拾取", L"Requesting host confirmation for ammo pickup"},
        {L"弹药拾取请求发送失败", L"Ammo pickup request failed"},
        {L"正在请求主机确认医疗包拾取", L"Requesting host confirmation for medkit pickup"},
        {L"医疗包拾取请求发送失败", L"Medkit pickup request failed"},
        {L"联机行动由主机控制暂停", L"Host controls pause in co-op"},
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
        {L"E               拾取脚下的弹药或医疗包", L"E               Pick up ammo or a medkit underfoot"},
        {L"H               消耗医疗包；安全等待 3 秒恢复 20 点生命",
         L"H               Use a medkit; stay safe for 3 seconds to restore 20 health"},
        {L"K               穿上或脱下已购买的护甲", L"K               Equip or unequip purchased armor"},
        {L"Q               到达撤离点后即可开始撤离",
         L"Q               Start extracting once you reach the extraction point"},
        {L"P / ESC         暂停行动 / 返回主菜单", L"P / ESC         Pause / return to the main menu"},
        {L"敌人靠近或进入视线后会苏醒并追击你。",
         L"Enemies wake up and pursue you when nearby or in sight."},
        {L"视野以外的地图区域会变暗。", L"Areas outside your line of sight are darkened."},
        {L"按 ← → 或 ↑ ↓ 翻页", L"Use ← → or ↑ ↓ to change pages"},
        {L"按 D 打开下载页面", L"Press D to open the download page"},
        {L"击败右下区域的 Boss，再前往地图右下角的绿色撤离点。",
         L"Defeat the area boss, then reach the green extraction point in the bottom-right."},
        {L"站在撤离点上按 Q，保持 5 秒不移动且不受攻击即可撤离。",
         L"Press Q at the extraction point. Hold for 5 seconds without moving or taking damage."},
        {L"普通敌人奖励 10 金币，Boss 奖励 100；Boss 半血后会进入狂暴阶段。",
         L"Regular enemies reward 10 coins; the boss rewards 100 and enrages below half health."},
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
        {L"商店购买会立即保存；行动进度会自动存入 %APPDATA%\\mokthyn\\save.dat。",
         L"Shop purchases save immediately; run progress autosaves to %APPDATA%\\mokthyn\\save.dat."},
        {L"重新启动游戏后，主菜单按 6 继续最近的单人行动；按 1 开始新行动。",
         L"After restarting, press 6 to resume the last solo run or 1 to start a new run."},
        {L"按 L 切换语言；主菜单按 7 调整并保存界面缩放比例。",
         L"Press L to switch language; press 7 on the main menu to adjust and save interface scale."},
        {L"联机：主机按 4，队友按 5 并输入主机 IPv4；双方需在同一局域网。",
         L"Co-op: host presses 4; teammate presses 5 and enters the host IPv4. Same LAN required."},
        {L"按 ← → 或 ↑ ↓ 翻页    按 D 打开下载页面    ENTER / ESC 返回",
         L"← → or ↑ ↓ pages    D opens download    ENTER / ESC returns"},
        {L"撤离成功", L"Extraction Successful"}, {L"行动结束", L"Run Over"},
        {L"本次击杀", L"Kills this run"}, {L"剩余弹药", L"Ammo remaining"},
        {L"按 ENTER 返回主菜单", L"Press ENTER to return to the main menu"},
        {L"已暂停", L"Paused"}, {L"按 P 继续  /  ESC 返回菜单", L"P resume  /  ESC menu"},
        {L"清除敌人并击败区域 Boss，然后前往撤离点",
         L"Clear the enemies and defeat the area boss, then extract"},
        {L"击败区域 Boss 后才能撤离", L"Defeat the area boss before extracting"},
        {L"区域 Boss", L"Area Boss"},
        {L"区域 Boss · 狂暴", L"Area Boss · Enraged"},
        {L"Boss 已击败", L"Boss defeated"},
        {L"区域 Boss 已击败  +100 金币", L"Area boss defeated  +100 coins"},
        {L"弹药不足，寻找弹药箱", L"Out of ammo; find an ammo cache"},
        {L"命中目标", L"Target hit"}, {L"未命中", L"Miss"},
        {L"护甲已损坏", L"Armor broken"}, {L"受到攻击，动作已中断", L"Hit taken; action interrupted"},
        {L"受到攻击", L"Under attack"},
        {L"拾取医疗包  +1", L"Medkit picked up  +1"},
        {L"这里没有可拾取的物资", L"No supplies to pick up here"},
        {L"动作已取消", L"Action cancelled"}, {L"金币不足", L"Not enough coins"},
        {L"护甲已装备", L"Armor equipped"}, {L"弹药补给完成", L"Ammo refilled"},
        {L"医疗包已补充", L"Medkit purchased"}, {L"没有可用护甲", L"No usable armor"},
        {L"护甲已穿上", L"Armor equipped"}, {L"护甲已脱下", L"Armor unequipped"},
        {L"治疗中，请保持安全", L"Healing; stay safe"},
        {L"现在无法治疗", L"Cannot heal right now"},
        {L"治疗完成  +20 生命", L"Healing complete  +20 health"},
        {L"前往地图上的撤离点", L"Reach the extraction point on the map"},
        {L"撤离开始，保持位置 5 秒", L"Extraction started; hold position for 5 seconds"},
        {L"治疗中  ", L"Healing  "}, {L"撤离中  ", L"Extracting  "},
        {L"下载：Mokthyn - Download", L"Download: Mokthyn - Download"}
    };
    const auto found = translations.find(value);
    if (found != translations.end()) return found->second;
    const std::wstring ammoPrefix = L"拾取弹药  +";
    if (value.rfind(ammoPrefix, 0) == 0)
        return L"Ammo picked up  +" + value.substr(ammoPrefix.size());
    const std::wstring networkWaitPrefix = L"等待玩家连接 · UDP ";
    if (value.rfind(networkWaitPrefix, 0) == 0)
        return L"Waiting for players · UDP " + value.substr(networkWaitPrefix.size());
    const std::wstring networkHelpPrefix = L"联机使用 UDP ";
    if (value.rfind(networkHelpPrefix, 0) == 0) {
        const size_t separator = value.find(L"；", networkHelpPrefix.size());
        if (separator != std::wstring::npos)
            return L"Co-op uses UDP " +
                   value.substr(networkHelpPrefix.size(), separator - networkHelpPrefix.size()) +
                   L"; check firewall and router settings if connections fail.";
    }
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
    static PIMAGE fontTarget = nullptr;
    static int fontHeight = 0;
    static unsigned int fontGeneration = static_cast<unsigned int>(-1);
    const int requestedFontHeight = std::max(1, scaledLength(size));
    if (fontTarget != frameBuffer || fontHeight != requestedFontHeight ||
        fontGeneration != frameBufferGeneration) {
        setfont(requestedFontHeight, 0, L"Microsoft YaHei UI", frameBuffer);
        fontTarget = frameBuffer;
        fontHeight = requestedFontHeight;
        fontGeneration = frameBufferGeneration;
    }
    settextcolor(color);
    outtextxy(scaledX(x), scaledY(y), displayed.c_str());
}

void drawBar(int x, int y, int width, int height, int value, int maximum, ege::color_t color) {
    fillRect(x, y, x + width, y + height, EGERGB(44, 57, 62));
    const int filled = maximum > 0 ? width * std::clamp(value, 0, maximum) / maximum : 0;
    if (filled > 0) fillRect(x, y, x + filled, y + height, color);
}

// 目标在当前视口外时，在战斗区边缘画一个指向它的方向标记
void drawOffscreenMarker(int targetX, int targetY, ege::color_t color, const wchar_t* label) {
    const int ts = battleTileSize();
    const int sx = tileScreenX(targetX) + ts / 2;
    const int sy = tileScreenY(targetY) + ts / 2;
    const int aL = battleMapLeft();
    const int aT = battleMapTop();
    const int aR = aL + battleMapAreaWidth();
    const int aB = aT + battleMapAreaHeight();
    const int margin = 20;
    const int L = aL + margin;
    const int R = aR - margin;
    const int T = aT + margin;
    const int B = aB - margin;
    if (sx >= L && sx <= R && sy >= T && sy <= B) return;
    const int cx = (aL + aR) / 2;
    const int cy = (aT + aB) / 2;
    const double dx = static_cast<double>(sx - cx);
    const double dy = static_cast<double>(sy - cy);
    double t = 1e9;
    if (dx > 0.001) t = std::min(t, static_cast<double>(R - cx) / dx);
    if (dx < -0.001) t = std::min(t, static_cast<double>(L - cx) / dx);
    if (dy > 0.001) t = std::min(t, static_cast<double>(B - cy) / dy);
    if (dy < -0.001) t = std::min(t, static_cast<double>(T - cy) / dy);
    const int px = static_cast<int>(cx + dx * t);
    const int py = static_cast<int>(cy + dy * t);
    fillCircle(px, py, 12, color);
    drawText(px - 7, py - 11, label, 18, Background);
}

void drawMap() {
    computeVisibility();
    // 敌人位置索引：按格查找 O(1)，替代每格遍历全部敌人的 O(格数×敌人数)
    std::unordered_map<long long, const Enemy*> enemyByTile;
    for (const Enemy& enemy : enemies) {
        if (enemy.alive)
            enemyByTile[(static_cast<long long>(enemy.y) << 32) |
                        static_cast<unsigned int>(enemy.x)] = &enemy;
    }
    const int tileSize = battleTileSize();
    const int areaLeft = battleMapLeft();
    const int areaTop = battleMapTop();
    const int areaW = battleMapAreaWidth();
    const int areaH = battleMapAreaHeight();
    const int camX = battleCameraX();
    const int camY = battleCameraY();
    const int tilesX = battleTilesX();
    const int tilesY = battleTilesY();
    fillRect(areaLeft - 4, areaTop - 4, areaLeft + areaW + 4, areaTop + areaH + 4,
             EGERGB(9, 14, 18));

    for (int sy = 0; sy < tilesY; ++sy) {
        for (int sx = 0; sx < tilesX; ++sx) {
            const int x = camX + sx;
            const int y = camY + sy;
            const int left = areaLeft + sx * tileSize;
            const int top = areaTop + sy * tileSize;
            if (x < 0 || x >= MapColumns || y < 0 || y >= MapRows) {
                fillRect(left, top, left + tileSize - 1, top + tileSize - 1,
                         EGERGB(16, 23, 28));
                continue;
            }
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

            const auto enemyEntry =
                enemyByTile.find((static_cast<long long>(y) << 32) |
                                 static_cast<unsigned int>(x));
            if (enemyEntry != enemyByTile.end()) {
                const Enemy& enemy = *enemyEntry->second;
                fillCircle(left + tileSize / 2, top + tileSize / 2,
                           enemy.boss ? tileSize * 2 / 5 : tileSize * 3 / 10,
                           enemy.boss
                               ? (enemy.hp <= BossHealth / 2 ? EGERGB(255, 101, 55)
                                                            : EGERGB(185, 54, 73))
                               : Red);
                fillCircle(left + tileSize / 2 - 3, top + tileSize / 2 - 3, tileSize / 12,
                           EGERGB(255, 174, 137));
                if (enemy.boss)
                    outlineRect(left + 2, top + 2, left + tileSize - 3, top + tileSize - 3,
                                Yellow);
                // 受伤敌人迷你血条
                const int fullHp = enemy.boss ? BossHealth : EnemyHealth;
                if (enemy.hp < fullHp) {
                    const int barWidth = std::max(10, tileSize / 2);
                    const int barHeight = std::max(2, tileSize / 8);
                    const int barLeft = left + tileSize / 2 - barWidth / 2;
                    const int barTop = top + 2;
                    fillRect(barLeft, barTop, barLeft + barWidth, barTop + barHeight,
                             EGERGB(20, 26, 30));
                    const int filled = barWidth * std::clamp(enemy.hp, 0, fullHp) / fullHp;
                    if (filled > 0)
                        fillRect(barLeft, barTop, barLeft + filled, barTop + barHeight,
                                 enemy.boss ? Yellow : Green);
                }
            }
        }
    }

    if (networkActive && peerConnected && peerX > 0 && peerX < MapColumns - 1 && peerY > 0 &&
        peerY < MapRows - 1 && visible[peerY][peerX]) {
        const int peerCenterX = tileScreenX(peerX) + tileSize / 2;
        const int peerCenterY = tileScreenY(peerY) + tileSize / 2;
        fillCircle(peerCenterX, peerCenterY, tileSize * 2 / 5, EGERGB(37, 55, 76));
        fillCircle(peerCenterX, peerCenterY, tileSize / 4, Cyan);
    }

    const int playerCenterX = tileScreenX(playerX) + tileSize / 2;
    const int playerCenterY = tileScreenY(playerY) + tileSize / 2;
    fillCircle(playerCenterX, playerCenterY, tileSize / 2, EGERGB(37, 70, 62));
    fillCircle(playerCenterX, playerCenterY, tileSize / 3, Green);
    fillCircle(playerCenterX + 3, playerCenterY - 3, tileSize / 10, Text);

    // 视口外的撤离点、队友和存活 Boss 方向标记（PVP 不显示对手位置，避免透视）
    if (gameMode != GameMode::Pvp) {
        drawOffscreenMarker(MapColumns - 2, MapRows - 2, Green, L"E");
        const auto boss = std::find_if(enemies.begin(), enemies.end(),
                                       [](const Enemy& enemy) { return enemy.boss && enemy.alive; });
        if (boss != enemies.end()) drawOffscreenMarker(boss->x, boss->y, Yellow, L"B");
    }
    if (networkActive && peerConnected && peerX > 0 && peerX < MapColumns - 1 && peerY > 0 &&
        peerY < MapRows - 1 && gameMode != GameMode::Pvp)
        drawOffscreenMarker(peerX, peerY, Cyan, L"P");
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
    drawText(valueX, 181, std::to_wstring(health) + L" / " + std::to_wstring(MaxHealth), 18,
             health > 60 ? Green : (health > 30 ? Yellow : Red));
    drawBar(x + padding, 211, width - padding * 2, 12, health, MaxHealth,
            health > 30 ? Green : Red);
    // 低血量红色闪烁警告
    if (health <= 30) {
        const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   Clock::now().time_since_epoch())
                                   .count();
        if ((elapsedMs / 500) % 2 == 0)
            drawText(x + padding, 233, L"生命危险，尽快治疗！", 18, Red);
    }

    drawText(x + padding, 245, L"弹药", 17, Muted);
    if (weaponMode == WeaponMode::Gun) {
        drawText(x + padding, 270, std::to_wstring(magazineAmmo) + L" / " +
                                       std::to_wstring(MagazineSize),
                 30, reloading ? Muted : Yellow);
        drawText(x + 160, 282, L"备弹 " + std::to_wstring(ammo), 16, Muted);
        if (reloading) {
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                                     Clock::now() - reloadStarted)
                                     .count();
            const int pct = std::clamp(static_cast<int>(elapsed * 100 / ReloadDurationMs), 0, 100);
            drawBar(x + padding, 305, width - padding * 2, 6, pct, 100, Cyan);
            drawText(x + padding, 312, L"换弹中...", 13, Cyan);
        }
    } else {
        drawText(x + padding, 270, L"战术刀", 30, Green);
        drawText(x + 160, 282, L"范围 " + std::to_wstring(MeleeRange) + L"  伤害 " +
                                   std::to_wstring(MeleeDamage),
                 16, Muted);
    }
    drawText(x + std::max(220, width / 2), 245, L"医疗包", 17, Muted);
    drawText(x + std::max(220, width / 2), 270, std::to_wstring(medkits), 34, Cyan);
    drawText(right - padding - 80, 250, weaponMode == WeaponMode::Gun ? L"[F]切刀" : L"[F]切枪",
             14, Muted);

    fillRect(x + padding, 324, right - padding, 325, EGERGB(52, 65, 68));
    if (gameMode == GameMode::Pvp) {
        drawText(x + padding, 344, L"对手", 17, Muted);
        const bool opponentConnected = networkActive && peerConnected;
        const int opponentHp = opponentConnected ? std::clamp(peerHealth, 0, MaxHealth) : MaxHealth;
        drawText(x + padding, 373,
                 opponentConnected ? std::to_wstring(opponentHp) + L" / " +
                                        std::to_wstring(MaxHealth)
                                   : (english ? L"Waiting..." : L"等待对手加入"),
                 21, opponentHp > 30 ? Red : Yellow);
        drawBar(x + padding, 410, width - padding * 2, 8, opponentHp, MaxHealth, Red);
        drawText(x + padding, 426,
                 opponentConnected ? (peerHealth > 0 ? L"目标：击倒对手" : L"对手已被击倒")
                                   : L"等待对手加入后开始",
                 16, opponentConnected ? Yellow : Muted);
    } else {
        drawText(x + padding, 344, L"任务进度", 17, Muted);
        drawText(x + padding, 373,
                 std::to_wstring(kills) + L" / " + std::to_wstring(totalEnemies) + L"  目标", 21,
                 Text);
        drawBar(x + padding, 410, width - padding * 2, 8, kills, totalEnemies, Green);

        const auto boss = std::find_if(enemies.begin(), enemies.end(),
                                       [](const Enemy& enemy) { return enemy.boss && enemy.alive; });
        if (boss != enemies.end()) {
            const bool bossEnraged = boss->hp <= BossHealth / 2;
            drawText(x + padding, 426, bossEnraged ? L"区域 Boss · 狂暴" : L"区域 Boss", 16,
                     bossEnraged ? Yellow : Red);
            drawText(right - padding - 55, 426,
                     std::to_wstring(std::max(0, boss->hp)) + L" / " +
                         std::to_wstring(BossHealth),
                     14, Text);
            drawBar(x + padding, 450, width - padding * 2, 8, boss->hp, BossHealth,
                    bossEnraged ? Yellow : Red);
        } else {
            drawText(x + padding, 426, L"Boss 已击败", 16, Green);
        }
    }

    drawText(x + padding, 468, L"护甲", 17, Muted);
    const bool hasArmor = armorTier > 0 && armorDurability > 0;
    const std::wstring armorDescription =
        hasArmor ? (english ? L"Tier " : L"等级 ") + std::to_wstring(armorTier) +
                       (armorOn ? L"" : (english ? L" (unequipped)" : L"（已脱下）"))
                 : (english ? L"Not equipped" : L"未装备");
    drawText(x + 100, 466, armorDescription, 17, hasArmor ? Cyan : Muted);
    if (hasArmor)
        drawBar(x + padding, 492, width - padding * 2, 8, armorDurability,
                ArmorDurability[armorTier], armorOn ? Cyan : Muted);
    drawText(x + padding, 516, L"金币", 17, Muted);
    drawText(x + 100, 514, std::to_wstring(money), 20, Yellow);
    if (networkActive && peerConnected)
        drawText(x + std::max(200, width / 2), 514,
                 L"队友 " + std::to_wstring(peerHealth) + L" HP", 16,
                 peerHealth > 30 ? Cyan : Red);

    fillRect(x + padding, 542, right - padding, 543, EGERGB(52, 65, 68));
    drawText(x + padding, 558, L"WASD 移动    方向键射击/挥刀", 15, Text);
    drawText(x + padding, 586, L"E 拾取   H 治疗   K 护甲   F 切武器", 15, Muted);
    drawText(x + padding, 614, L"R 换弹   Q 撤离   P 暂停   ESC 菜单", 15, Muted);
    if (Clock::now() < messageUntil)
        drawText(x + padding, visibleDesignHeight() - 52, message, 16, Yellow);
    if (networkActive && !networkStatus.empty())
        drawText(x + padding, visibleDesignHeight() - 30, networkStatus, 15,
                 networkHost && peerConnected ? Cyan : Yellow);
}

void drawMenu() {
    cleardevice();
    setfillcolor(Background);
    bar(0, 0, windowWidth, windowHeight);

    int mouseX = 0, mouseY = 0;
    mousepos(&mouseX, &mouseY);
    int hoverX = 0, hoverY = 0;
    mouseDesignPos(mouseX, mouseY, hoverX, hoverY);

    // 标题 + 副标题 + 装饰线
    drawText(430, 78, L"无限枪洲", 72, Text);
    drawText(438, 158, L"FIELD OPERATION  /  01", 17, Muted);
    fillRect(440, 208, 1160, 211, Green);

    drawButton(menuLanguageButton(), L"语言 L", 18, Cyan, true, hoverX, hoverY);

    drawButton(menuLeftButton(0), L"开始行动", 23, Green, true, hoverX, hoverY);
    drawButton(menuLeftButton(1), L"黑市商店", 23, Yellow, true, hoverX, hoverY);
    drawButton(menuLeftButton(2), L"操作说明", 23, Cyan, true, hoverX, hoverY);
    drawButton(menuLeftButton(3), L"创建联机主机", 21, Text, true, hoverX, hoverY);
    drawButton(menuLeftButton(4), L"加入局域网游戏", 21, Text, true, hoverX, hoverY);

    if (hasSavedRun) drawButton(menuRightButton(0), L"继续存档", 23, Cyan, true, hoverX, hoverY);
    drawButton(menuRightButton(1), L"设置", 23, Cyan, true, hoverX, hoverY);
    drawButton(menuRightButton(2), L"快速开始", 23, Green, true, hoverX, hoverY);
    drawButton(menuRightButton(3), L"PVP 对战", 23, Red, true, hoverX, hoverY);

    const std::wstring diffText =
        difficulty == Difficulty::Easy   ? L"难度：简单"
        : difficulty == Difficulty::Normal ? L"难度：普通"
                                           : L"难度：困难";
    drawButton(menuBottomButton(0), diffText, 19,
               difficulty == Difficulty::Easy   ? Green
               : difficulty == Difficulty::Normal ? Yellow
                                                  : Red,
               true, hoverX, hoverY);
    drawButton(menuBottomButton(1), L"切换语言 L", 19, Cyan, true, hoverX, hoverY);
    drawButton(menuBottomButton(2), L"退出", 19, Red, true, hoverX, hoverY);

    if (!archiveStatus.empty()) drawText(430, 700, archiveStatus, 16, Yellow);
    if (Clock::now() < messageUntil) drawText(430, 730, message, 16, Yellow);
}

void drawNetworkLobby() {
    cleardevice();
    fillRect(150, 105, 1450, 795, Panel);
    const std::wstring title = screen == Screen::HostLobby ? L"创建联机主机"
                               : screen == Screen::JoinIp  ? L"加入局域网游戏"
                                                           : L"等待主机开始";
    drawText(220, 160, title, 42, Text);
    drawText(225, 250, networkStatus, 22, Green);

    int mouseX = 0, mouseY = 0;
    mousepos(&mouseX, &mouseY);
    int hoverX = 0, hoverY = 0;
    mouseDesignPos(mouseX, mouseY, hoverX, hoverY);

    if (screen == Screen::HostLobby) {
        drawText(225, 330, L"可用主机 IPv4 地址", 18, Muted);
        const std::wstring localIp = getLocalIPv4();
        drawText(225, 372, localIp, 34, Cyan);
        drawText(225, 470, L"将队友可访问的地址发给同一局域网内的队友。", 20, Text);
        drawText(225, 520,
                 peerConnected ? L"玩家已连接，按 ENTER 开始"
                               : L"等待玩家连接 · UDP " + std::to_wstring(NetworkPort),
                 20,
                 peerConnected ? Green : Yellow);
        drawButton(lobbyStartButton(), L"开始行动  ENTER", 22,
                   peerConnected ? Green : Muted, peerConnected, hoverX, hoverY);
        drawButton(lobbyBackButton(), L"返回  ESC", 20, Cyan, true, hoverX, hoverY);
    } else if (screen == Screen::JoinIp) {
        fillRect(220, 355, 1120, 425, PanelLight);
        drawText(245, 370,
                 hostIpInput.empty() ? L"输入主机 IPv4 地址..."
                                     : std::wstring(hostIpInput.begin(), hostIpInput.end()),
                 28, Text);
        drawText(225, 490, L"输入 IP 后按 ENTER 连接，BACKSPACE 删除，ESC 返回。", 18, Muted);
        drawText(225, 545, L"双方需在同一局域网，并允许程序通过 Windows 防火墙。", 18, Muted);
        drawButton(lobbyBackButton(), L"返回  ESC", 20, Cyan, true, hoverX, hoverY);
    } else {
        drawText(225, 365, L"连接成功。等待主机创建地图并开始行动。", 22, Text);
        drawText(225, 475, L"ESC 取消连接并返回主菜单。", 18, Muted);
        drawButton(lobbyBackButton(), L"返回  ESC", 20, Cyan, true, hoverX, hoverY);
    }
}

void drawShop() {
    cleardevice();
    fillRect(105, 70, 1495, 830, Panel);
    drawText(155, 120, L"黑市", 44, Text);
    drawText(155, 179, L"补充装备，为下一次行动做好准备", 19, Muted);
    drawText(1175, 135, L"持有金币", 17, Muted);
    drawText(1320, 127, std::to_wstring(money), 32, Yellow);

    const std::array<std::wstring, 4> armorNames = {L"一级护甲", L"二级护甲", L"三级护甲",
                                                    L"四级护甲"};
    std::array<std::wstring, 6> labels{};
    for (int tier = 1; tier <= 4; ++tier) {
        if (english) {
            labels[tier - 1] =
                L"Tier " + std::to_wstring(tier) + L" armor    durability " +
                std::to_wstring(ArmorDurability[tier]) + L" / reduction " +
                std::to_wstring(ArmorReduction[tier]) + L"    " +
                std::to_wstring(ArmorPrices[tier]) + L" coins";
        } else {
            labels[tier - 1] =
                armorNames[tier - 1] + L"       耐久 " +
                std::to_wstring(ArmorDurability[tier]) + L" / 减伤 " +
                std::to_wstring(ArmorReduction[tier]) + L"       " +
                std::to_wstring(ArmorPrices[tier]) + L" 金";
        }
    }
    labels[4] = english ? L"Ammo supply       +10 rounds       5 coins"
                        : L"弹药补给       +10 发       5 金";
    labels[5] = english ? L"Medkit       +1       20 coins" : L"医疗包         +1 个       20 金";

    int mouseX = 0, mouseY = 0;
    mousepos(&mouseX, &mouseY);
    int hoverX = 0, hoverY = 0;
    mouseDesignPos(mouseX, mouseY, hoverX, hoverY);

    for (int i = 0; i < ShopItemCount; ++i)
        drawButton(shopItemRect(i), std::to_wstring(i + 1) + L".  " + labels[i], 20,
                   shopSelection == i ? Green : Text, true, hoverX, hoverY);
    drawButton(shopBackButton(), L"返回  0", 20, Cyan, true, hoverX, hoverY);
    drawText(155, 766, L"护甲升级会替换当前护甲。", 17, Muted);
    if (Clock::now() < messageUntil) drawText(155, 812, message, 18, Yellow);
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
        {{L"行动简报 · 目标与物资", L"击败右下区域的 Boss，再前往地图右下角的绿色撤离点。",
          L"站在撤离点上按 Q，保持 5 秒不移动且不受攻击即可撤离。",
          L"普通敌人奖励 10 金币，Boss 奖励 100；Boss 半血后会进入狂暴阶段。",
          L"方向键射击，每发消耗 1 弹；按住 WASD 可连续移动。",
          L"护甲可减伤并消耗耐久；K 可暂时脱下以保留耐久。",
          L"被击中会打断治疗和撤离；移动也会取消正在进行的动作。",
          L"生命归零时行动失败；撤离成功会保留当前携带物资。"}},
        {{L"行动简报 · 商店、存档与联机", L"主菜单按 2 打开黑市：1–4 购买护甲，5 补弹，6 购买医疗包。",
          L"商店购买会立即保存；行动进度会自动存入 %APPDATA%\\mokthyn\\save.dat。",
          L"重新启动游戏后，主菜单按 6 继续最近的单人行动；按 1 开始新行动。",
          L"按 L 切换语言；主菜单按 7 调整并保存界面缩放比例。",
          L"联机：主机按 4，队友按 5 并输入主机 IPv4；双方需在同一局域网。",
          L"联机使用 UDP " + std::to_wstring(NetworkPort) +
              L"；若无法连接，请检查防火墙和路由器设置。",
          L"下载：Mokthyn - Download"}}
    }};
    drawText(150, 110, pages[helpPage][0], 46, Text);
    for (int i = 1; i < 8; ++i)
        drawText(160, 180 + (i - 1) * 54, pages[helpPage][i], 26, i == 7 ? Green : Text);
    drawText(160, 580, L"下载地址：Mokthyn - Download", 19, Cyan);
    drawText(160, 612, DownloadUrl, 19, Cyan);

    int mouseX = 0, mouseY = 0;
    mousepos(&mouseX, &mouseY);
    int hoverX = 0, hoverY = 0;
    mouseDesignPos(mouseX, mouseY, hoverX, hoverY);

    drawButton(helpPrevButton(), L"← 上一页", 19, Cyan, helpPage > 0, hoverX, hoverY);
    drawButton(helpNextButton(), L"下一页 →", 19, Cyan, helpPage < 2, hoverX, hoverY);
    drawButton(helpBackButton(), L"返回  ENTER / ESC", 19, Green, true, hoverX, hoverY);
    drawText(960, 690, L"按 D 打开下载页面", 18, Muted);
    if (Clock::now() < messageUntil) drawText(160, 730, message, 18, Yellow);
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

    int mouseX = 0, mouseY = 0;
    mousepos(&mouseX, &mouseY);
    int hoverX = 0, hoverY = 0;
    mouseDesignPos(mouseX, mouseY, hoverX, hoverY);

    for (int i = 0; i < SettingsItemCount; ++i) {
        const bool selected = settingsSelection == i;
        drawButton(settingsRowRect(i), labels[i] + L"      " + values[i], 22,
                   selected ? Green : Text, true, hoverX, hoverY);
        drawButton(settingsMinusButton(i), L"−", 24, Cyan, true, hoverX, hoverY);
        drawButton(settingsPlusButton(i), L"+", 24, Cyan, true, hoverX, hoverY);
    }
    drawButton(settingsResetButton(), L"恢复默认  0", 20, Yellow, true, hoverX, hoverY);
    drawButton(settingsBackButton(), L"返回  ENTER / ESC", 20, Green, true, hoverX, hoverY);
    if (!archiveStatus.empty()) drawText(130, 710, archiveStatus, 16, Yellow);
}

void drawResult() {
    cleardevice();
    fillRect(250, 155, 1350, 755, Panel);
    const bool pvp = gameMode == GameMode::Pvp;
    drawText(340, 230,
             pvp ? (won ? L"胜利" : L"失败") : (won ? L"撤离成功" : L"行动结束"), 58,
             won ? Green : Red);
    if (!pvp) {
        drawText(345, 332, L"本次击杀", 20, Muted);
        drawText(345, 370,
                 std::to_wstring(kills) + L" / " + std::to_wstring(totalEnemies), 38, Text);
    }
    drawText(720, 332, L"剩余弹药", 20, Muted);
    drawText(720, 370, std::to_wstring(ammo + magazineAmmo), 38, Yellow);
    drawText(1040, 332, L"持有金币", 20, Muted);
    drawText(1040, 370, std::to_wstring(money), 38, Yellow);
    drawText(345, 500, L"护甲", 20, Muted);
    const bool hasArmor = armorTier > 0 && armorDurability > 0;
    const std::wstring armorDescription =
        hasArmor ? (english ? L"Tier " : L"等级 ") + std::to_wstring(armorTier) +
                       (english ? L"  ·  " : L"  ·  ") + std::to_wstring(armorDurability) +
                       L" / " + std::to_wstring(ArmorDurability[armorTier]) +
                       (armorOn ? L"" : (english ? L"  (unequipped)" : L"  （已脱下）"))
                 : (english ? L"Not equipped" : L"未装备");
    drawText(440, 440, armorDescription, 20, hasArmor ? Cyan : Muted);

    int mouseX = 0, mouseY = 0;
    mousepos(&mouseX, &mouseY);
    int hoverX = 0, hoverY = 0;
    mouseDesignPos(mouseX, mouseY, hoverX, hoverY);

    drawButton(resultBackButton(), L"返回主菜单  ENTER", 22, Green, true, hoverX, hoverY);
    if (!networkStatus.empty()) drawText(345, 610, networkStatus, 18, Yellow);
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