#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <shellapi.h>
#include <graphics.h>
#include "game.h/GameArchive.h"
#include "game.h/GameArchiveImpl.h"
#include "game.h/GameLogic.h"
#include "game.h/GameLogicImpl.h"
#include "game.h/GameNetwork.h"
#include "game.h/GameNetworkImpl.h"
#include "game.h/GameRender.h"
#include "game.h/GameRenderImpl.h"
#include "game.h/GameState.h"
#include "game.h/GameTypes.h"
int main() {
    using namespace mokthyn;
    SetProcessDPIAware();
    loadArchive();
    setinitmode(INIT_DEFAULT);
    initgraph(windowWidth, windowHeight);
    ege_enable_aa(true);
    setbkmode(TRANSPARENT);
    frameBuffer = newimage(windowWidth, windowHeight);
    if (frameBuffer == nullptr) {
        closegraph();
        return 1;
    }
    ++frameBufferGeneration;
    setbkmode(TRANSPARENT, frameBuffer);
    setfontbkcolor(Background, frameBuffer);
    ege_enable_aa(true, frameBuffer);
    while (is_run()) {
        processNetwork();
        if ((screen == Screen::Playing || screen == Screen::Paused) &&
            Clock::now() - lastArchiveSave >= std::chrono::seconds(2) && !saveArchive())
            setMessage(archiveStatus, 4000);
        settarget(frameBuffer);
        switch (screen) {
        case Screen::Menu: {
            const bool newRunKeyPressed = keyPressed('1');
            const bool enterPressed = keyPressed(VK_RETURN);
            int clickX = 0, clickY = 0;
            if (pollMouseClick(clickX, clickY)) {
                if (designPointInRect(clickX, clickY, menuLeftButton(0)))
                    startRun();
                else if (designPointInRect(clickX, clickY, menuLeftButton(1)))
                    screen = Screen::Shop;
                else if (designPointInRect(clickX, clickY, menuLeftButton(2))) {
                    helpInputReady = false;
                    screen = Screen::Help;
                } else if (designPointInRect(clickX, clickY, menuLeftButton(3))) {
                    gameMode = GameMode::Coop;
                    if (!beginHostNetwork()) setMessage(L"主机启动失败，检查 UDP 端口是否被占用");
                } else if (designPointInRect(clickX, clickY, menuLeftButton(4))) {
                    if (!beginJoinNetwork()) setMessage(L"网络初始化失败");
                } else if (hasSavedRun && designPointInRect(clickX, clickY, menuRightButton(0))) {
                    resetRunProgress();
                    screen = Screen::Playing;
                    lastMove = lastEnemyTurn = nextPlayerHit = nextPeerHit = nextPlayerShot =
                        Clock::now();
                    setMessage(L"已继续存档行动");
                } else if (designPointInRect(clickX, clickY, menuRightButton(1)))
                    screen = Screen::Settings;
                else if (designPointInRect(clickX, clickY, menuRightButton(2)))
                    startRun();
                else if (designPointInRect(clickX, clickY, menuRightButton(3))) {
                    gameMode = GameMode::Pvp;
                    if (!beginHostNetwork()) setMessage(L"PVP 主机启动失败，检查 UDP 端口是否被占用");
                } else if (designPointInRect(clickX, clickY, menuBottomButton(0))) {
                    difficulty = difficulty == Difficulty::Easy   ? Difficulty::Normal
                                 : difficulty == Difficulty::Normal ? Difficulty::Hard
                                                                    : Difficulty::Easy;
                    saveArchive();
                    setMessage(difficulty == Difficulty::Easy   ? L"难度：简单"
                               : difficulty == Difficulty::Normal ? L"难度：普通"
                                                                  : L"难度：困难");
                } else if (designPointInRect(clickX, clickY, menuBottomButton(1)) ||
                           designPointInRect(clickX, clickY, menuLanguageButton())) {
                    english = !english;
                    saveArchive();
                } else if (designPointInRect(clickX, clickY, menuBottomButton(2))) {
                    saveArchive();
                    settarget(nullptr);
                    closeNetwork();
                    delimage(frameBuffer);
                    closegraph();
                    return 0;
                }
            }
            if (keyPressed('L')) {
                english = !english;
                saveArchive();
            } else if (keyPressed('6') && hasSavedRun) {
                resetRunProgress();
                screen = Screen::Playing;
                lastMove = lastEnemyTurn = nextPlayerHit = nextPeerHit = nextPlayerShot = Clock::now();
                setMessage(L"已继续存档行动");
            } else if (newRunKeyPressed || enterPressed) {
                startRun();
            } else if (keyPressed('2'))
                screen = Screen::Shop;
            else if (keyPressed('3')) {
                helpInputReady = false;
                screen = Screen::Help;
            } else if (keyPressed('4')) {
                gameMode = GameMode::Coop;
                if (!beginHostNetwork()) setMessage(L"主机启动失败，检查 UDP 端口是否被占用");
            } else if (keyPressed('5')) {
                if (!beginJoinNetwork()) setMessage(L"网络初始化失败");
            } else if (keyPressed('7')) {
                screen = Screen::Settings;
            } else if (keyPressed('8')) {
                startRun();
            } else if (keyPressed('9')) {
                gameMode = GameMode::Pvp;
                if (!beginHostNetwork()) setMessage(L"PVP 主机启动失败，检查 UDP 端口是否被占用");
            } else if (keyPressed('0')) {
                difficulty = difficulty == Difficulty::Easy   ? Difficulty::Normal
                             : difficulty == Difficulty::Normal ? Difficulty::Hard
                                                                 : Difficulty::Easy;
                saveArchive();
                setMessage(difficulty == Difficulty::Easy   ? L"难度：简单"
                           : difficulty == Difficulty::Normal ? L"难度：普通"
                                                              : L"难度：困难");
            } else if (keyPressed(VK_ESCAPE)) {
                saveArchive();
                settarget(nullptr);
                closeNetwork();
                delimage(frameBuffer);
                closegraph();
                return 0;
            }
            drawMenu();
            break;
        }
        case Screen::HostLobby: {
            int clickX = 0, clickY = 0;
            if (pollMouseClick(clickX, clickY)) {
                if (designPointInRect(clickX, clickY, lobbyStartButton()) && peerConnected)
                    startRun();
                else if (designPointInRect(clickX, clickY, lobbyBackButton())) {
                    closeNetwork();
                    screen = Screen::Menu;
                }
            }
            if (keyPressed(VK_ESCAPE)) {
                closeNetwork();
                screen = Screen::Menu;
            } else if (keyPressed('M')) {
                gameMode = gameMode == GameMode::Pvp ? GameMode::Coop : GameMode::Pvp;
                networkStatus = gameMode == GameMode::Pvp ? L"模式：PVP 对战（按 M 切换）"
                                                          : L"模式：合作 PVE（按 M 切换）";
            } else if (peerConnected && keyPressed(VK_RETURN)) {
                startRun();
            }
            drawNetworkLobby();
            break;
        }
        case Screen::JoinIp:
            if (kbhit()) {
                const int character = getch();
                if (character == 27) {
                    closeNetwork();
                    screen = Screen::Menu;
                } else if (character == 8 && !hostIpInput.empty()) {
                    hostIpInput.pop_back();
                } else if (character == 13 && !hostIpInput.empty()) {
                    peerAddress = {};
                    peerAddress.sin_family = AF_INET;
                    peerAddress.sin_port = htons(NetworkPort);
                    if (inet_pton(AF_INET, hostIpInput.c_str(), &peerAddress.sin_addr) == 1) {
                        networkStatus = L"正在连接主机...";
                        screen = Screen::JoinLobby;
                        lastConnectAttempt = Clock::time_point{};
                        lastNetworkReceive = Clock::now();
                    } else {
                        networkStatus = L"IP 格式无效，请输入 IPv4 地址";
                    }
                } else if (((character >= '0' && character <= '9') || character == '.') &&
                           hostIpInput.size() < 15) {
                    hostIpInput.push_back(static_cast<char>(character));
                }
            }
            {
                int clickX = 0, clickY = 0;
                if (pollMouseClick(clickX, clickY) &&
                    designPointInRect(clickX, clickY, lobbyBackButton())) {
                    closeNetwork();
                    screen = Screen::Menu;
                }
            }
            if (screen == Screen::JoinIp) drawNetworkLobby();
            break;
        case Screen::JoinLobby:
            {
                int clickX = 0, clickY = 0;
                if (pollMouseClick(clickX, clickY) &&
                    designPointInRect(clickX, clickY, lobbyBackButton())) {
                    sendNetworkPacket("LEAVE");
                    closeNetwork();
                    screen = Screen::Menu;
                }
            }
            if (keyPressed(VK_ESCAPE)) {
                sendNetworkPacket("LEAVE");
                closeNetwork();
                screen = Screen::Menu;
            }
            drawNetworkLobby();
            break;
        case Screen::Shop:
            {
                int clickX = 0, clickY = 0;
                if (pollMouseClick(clickX, clickY)) {
                    bool handled = false;
                    for (int i = 0; i < 6 && !handled; ++i) {
                        if (designPointInRect(clickX, clickY, shopItemRect(i))) {
                            shopSelection = i;
                            buySelectedItem();
                            handled = true;
                        }
                    }
                    if (!handled && designPointInRect(clickX, clickY, shopBackButton()))
                        screen = Screen::Menu;
                }
            }
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
            {
                int clickX = 0, clickY = 0;
                if (pollMouseClick(clickX, clickY)) {
                    if (designPointInRect(clickX, clickY, helpPrevButton()) && helpPage > 0)
                        helpPage = (helpPage + 2) % 3;
                    else if (designPointInRect(clickX, clickY, helpNextButton()) && helpPage < 2)
                        helpPage = (helpPage + 1) % 3;
                    else if (designPointInRect(clickX, clickY, helpBackButton()))
                        screen = Screen::Menu;
                }
            }
            if (!helpInputReady) {
                if (!keystate('3')) helpInputReady = true;
            } else {
                const bool escapePressed = keyPressed(VK_ESCAPE);
                const bool enterPressed = keyPressed(VK_RETURN);
                const bool nextPagePressed = keyPressed(VK_RIGHT) || keyPressed(VK_DOWN);
                const bool previousPagePressed = keyPressed(VK_LEFT) || keyPressed(VK_UP);
                const bool downloadPressed = keyPressed('D');
                if (escapePressed || enterPressed) {
                    screen = Screen::Menu;
                } else if (nextPagePressed) {
                    helpPage = (helpPage + 1) % 3;
                } else if (previousPagePressed) {
                    helpPage = (helpPage + 2) % 3;
                } else if (downloadPressed) {
                    const auto result = reinterpret_cast<INT_PTR>(ShellExecuteW(
                        nullptr, L"open", DownloadUrl, nullptr, nullptr, SW_SHOWNORMAL));
                    if (result <= 32) setMessage(L"下载页面无法打开");
                }
            }
            drawHelp();
            break;
        case Screen::Settings: {
            const auto adjustSetting = [&](int direction) {
                bool settingApplied = true;
                if (settingsSelection == 0) {
                    uiScalePercent = std::clamp(uiScalePercent + direction * UiScaleStepPercent,
                                                MinUiScalePercent, MaxUiScalePercent);
                } else if (settingsSelection == 1) {
                    settingApplied =
                        resizeGameWindow(std::clamp(windowWidth + direction * WindowWidthStep,
                                                    MinWindowWidth, MaxWindowWidth),
                                         windowHeight);
                } else {
                    settingApplied = resizeGameWindow(
                        windowWidth, std::clamp(windowHeight + direction * WindowHeightStep,
                                                MinWindowHeight, MaxWindowHeight));
                }
                if (settingApplied) saveArchive();
            };
            const auto resetSettings = [&]() {
                const bool resizeSucceeded =
                    resizeGameWindow(DefaultWindowWidth, DefaultWindowHeight);
                uiScalePercent = DefaultUiScalePercent;
                const std::wstring resizeError = resizeSucceeded ? L"" : archiveStatus;
                saveArchive();
                if (!resizeSucceeded) archiveStatus = resizeError;
            };
            int clickX = 0, clickY = 0;
            if (pollMouseClick(clickX, clickY)) {
                bool handled = false;
                for (int i = 0; i < 3 && !handled; ++i) {
                    if (designPointInRect(clickX, clickY, settingsMinusButton(i))) {
                        settingsSelection = i;
                        handled = true;
                        adjustSetting(-1);
                    } else if (designPointInRect(clickX, clickY, settingsPlusButton(i))) {
                        settingsSelection = i;
                        handled = true;
                        adjustSetting(1);
                    } else if (designPointInRect(clickX, clickY, settingsRowRect(i))) {
                        settingsSelection = i;
                        handled = true;
                    }
                }
                if (!handled) {
                    if (designPointInRect(clickX, clickY, settingsResetButton()))
                        resetSettings();
                    else if (designPointInRect(clickX, clickY, settingsBackButton()))
                        screen = Screen::Menu;
                }
            }
            const bool escapePressed = keyPressed(VK_ESCAPE);
            const bool enterPressed = keyPressed(VK_RETURN);
            const bool upPressed = keyPressed(VK_UP);
            const bool downPressed = keyPressed(VK_DOWN);
            const bool leftPressed = keyPressed(VK_LEFT);
            const bool rightPressed = keyPressed(VK_RIGHT);
            const bool resetPressed = keyPressed('0');
            if (escapePressed || enterPressed) {
                screen = Screen::Menu;
            } else if (upPressed) {
                settingsSelection = (settingsSelection + 2) % 3;
            } else if (downPressed) {
                settingsSelection = (settingsSelection + 1) % 3;
            } else if (leftPressed || rightPressed) {
                adjustSetting(rightPressed ? 1 : -1);
            } else if (resetPressed) {
                resetSettings();
            }
            drawSettings();
            break;
        }
        case Screen::Playing:
            updatePlaying();
            drawPlaying();
            break;
        case Screen::Paused: {
            const auto resumeGame = [&]() {
                advancePausedTimers(Clock::now() - pausedAt);
                pausedAt = {};
                screen = Screen::Playing;
            };
            const auto exitPauseToMenu = [&]() {
                if (networkActive) {
                    sendNetworkPacket("LEAVE");
                    closeNetwork();
                }
                if (networkRun)
                    restoreNetworkArchiveSession();
                else {
                    resetRunProgress();
                    saveArchive();
                    screen = Screen::Menu;
                }
            };
            int clickX = 0, clickY = 0;
            if (pollMouseClick(clickX, clickY)) {
                if (designPointInRect(clickX, clickY, pauseResumeButton()) && !networkPause)
                    resumeGame();
                else if (designPointInRect(clickX, clickY, pauseExitButton()))
                    exitPauseToMenu();
            }
            drawPlaying();
            fillRect(0, 0, DesignWidth, DesignHeight, EGEARGB(170, 5, 10, 13));
            drawText(630, 365, L"已暂停", 48, Text);
            drawText(594, 435,
                     networkPause ? L"主机暂停中  /  ESC 返回菜单" : L"按 P 继续  /  ESC 返回菜单",
                     20, Muted);
            if (keyPressed('P') && !networkPause) resumeGame();
            else if (keyPressed(VK_ESCAPE)) exitPauseToMenu();
            int mouseX = 0, mouseY = 0;
            mousepos(&mouseX, &mouseY);
            int hoverX = 0, hoverY = 0;
            mouseDesignPos(mouseX, mouseY, hoverX, hoverY);
            drawButton(pauseResumeButton(), L"继续  P", 22, Green, !networkPause, hoverX, hoverY);
            drawButton(pauseExitButton(), L"返回菜单  ESC", 20, Red, true, hoverX, hoverY);
            break;
        }
        case Screen::Result:
            {
                int clickX = 0, clickY = 0;
                if (pollMouseClick(clickX, clickY) &&
                    designPointInRect(clickX, clickY, resultBackButton())) {
                    if (networkActive) {
                        sendNetworkPacket("LEAVE");
                        closeNetwork();
                    }
                    if (networkRun)
                        restoreNetworkArchiveSession();
                    else {
                        resetRunProgress();
                        screen = Screen::Menu;
                        saveArchive();
                    }
                }
            }
            drawResult();
            if (keyPressed(VK_RETURN)) {
                if (networkActive) {
                    sendNetworkPacket("LEAVE");
                    closeNetwork();
                }
                if (networkRun)
                    restoreNetworkArchiveSession();
                else {
                    resetRunProgress();
                    screen = Screen::Menu;
                    saveArchive();
                }
            }
            break;
        }
        settarget(nullptr);
        putimage(0, 0, frameBuffer);
        delay_fps(60);
    }
    settarget(nullptr);
    // 对局中直接关闭窗口视为放弃本局，按失败结算
    if (screen == Screen::Playing || screen == Screen::Paused) {
        if (!networkActive && !networkRun) {
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
            archiveStatus = L"对局未完成直接退出，已按失败结算";
        }
    }
    saveArchive();
    delimage(frameBuffer);
    closegraph();
    return 0;
}