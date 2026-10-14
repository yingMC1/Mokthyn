#define WIN32_LEAN_AND_MEAN
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
#include <graphics.h>
#include <shellapi.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
int main() {
    using namespace mokthyn;
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
        case Screen::Menu:
            {
            const bool newRunKeyPressed = keyPressed('1');
            const bool enterPressed = keyPressed(VK_RETURN);
            if (keyPressed('L')) {
                english = !english;
                saveArchive();
            } else if (keyPressed('6') && hasSavedRun) {
                resetRunProgress();
                screen = Screen::Playing;
                lastMove = lastEnemyTurn = nextPlayerHit = nextPeerHit = Clock::now();
                setMessage(L"已继续存档行动");
            } else if (newRunKeyPressed || enterPressed) {
                startRun();
            } else if (keyPressed('2'))
                screen = Screen::Shop;
            else if (keyPressed('3')) {
                helpInputReady = false;
                screen = Screen::Help;
            }
            else if (keyPressed('4')) {
                if (!beginHostNetwork()) setMessage(L"主机启动失败，检查 UDP 端口是否被占用");
            } else if (keyPressed('5')) {
                if (!beginJoinNetwork()) setMessage(L"网络初始化失败");
            } else if (keyPressed('7')) {
                screen = Screen::Settings;
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
        case Screen::HostLobby:
            if (keyPressed(VK_ESCAPE)) {
                closeNetwork();
                screen = Screen::Menu;
            } else if (peerConnected && keyPressed(VK_RETURN)) {
                startRun();
            }
            drawNetworkLobby();
            break;
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
            if (screen == Screen::JoinIp) drawNetworkLobby();
            break;
        case Screen::JoinLobby:
            if (keyPressed(VK_ESCAPE)) {
                sendNetworkPacket("LEAVE");
                closeNetwork();
                screen = Screen::Menu;
            }
            drawNetworkLobby();
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
                    if (result <= 32)
                        setMessage(L"下载页面无法打开");
                }
            }
            drawHelp();
            break;
        case Screen::Settings:
            {
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
                const int direction = rightPressed ? 1 : -1;
                bool settingApplied = true;
                if (settingsSelection == 0) {
                    uiScalePercent = std::clamp(
                        uiScalePercent + direction * UiScaleStepPercent,
                        MinUiScalePercent, MaxUiScalePercent);
                } else if (settingsSelection == 1) {
                    settingApplied = resizeGameWindow(
                        std::clamp(windowWidth + direction * WindowWidthStep,
                                   MinWindowWidth, MaxWindowWidth),
                        windowHeight);
                } else {
                    settingApplied = resizeGameWindow(
                        windowWidth,
                        std::clamp(windowHeight + direction * WindowHeightStep,
                                   MinWindowHeight, MaxWindowHeight));
                }
                if (settingApplied) saveArchive();
            } else if (resetPressed) {
                const bool resizeSucceeded =
                    resizeGameWindow(DefaultWindowWidth, DefaultWindowHeight);
                uiScalePercent = 120;
                const std::wstring resizeError = resizeSucceeded ? L"" : archiveStatus;
                saveArchive();
                if (!resizeSucceeded) archiveStatus = resizeError;
            }
            drawSettings();
            break;
            }
        case Screen::Playing:
            updatePlaying();
            drawPlaying();
            break;
        case Screen::Paused:
            drawPlaying();
            fillRect(0, 0, DesignWidth, DesignHeight, EGEARGB(170, 5, 10, 13));
            drawText(630, 365, L"已暂停", 48, Text);
            drawText(594, 435,
                     networkPause ? L"主机暂停中  /  ESC 返回菜单"
                                  : L"按 P 继续  /  ESC 返回菜单",
                     20, Muted);
            if (keyPressed('P') && !networkPause) {
                const auto pausedFor = Clock::now() - pausedAt;
                if (healing) healStarted += pausedFor;
                if (extracting) extractStarted += pausedFor;
                if (networkHost && peerExtracting) peerExtractStarted += pausedFor;
                lastMove += pausedFor;
                lastEnemyTurn += pausedFor;
                nextPlayerHit += pausedFor;
                nextPeerHit += pausedFor;
                if (peerHealing) peerHealStarted += pausedFor;
                pausedAt = {};
                screen = Screen::Playing;
            } else if (keyPressed(VK_ESCAPE)) {
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
            }
            break;
        case Screen::Result:
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
    saveArchive();
    delimage(frameBuffer);
    closegraph();
    return 0;
}