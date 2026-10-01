#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <graphics.h>
#include <windows.h>
#include "GameTypes.h"
#include "GameState.h"
#include "GameLogic.h"
#include "GameNetwork.h"
#include "GameRender.h"
#include "GameLogicImpl.h"
#include "GameNetworkImpl.h"
#include "GameRenderImpl.h"
int main() {
    using namespace mokthyn;
    setinitmode(INIT_DEFAULT);
    initgraph(WindowWidth, WindowHeight);
    ege_enable_aa(true);
    setbkmode(TRANSPARENT);
    frameBuffer = newimage(WindowWidth, WindowHeight);
    if (frameBuffer == nullptr) {
        closegraph();
        return 1;
    }
    setbkmode(TRANSPARENT, frameBuffer);
    setfontbkcolor(Background, frameBuffer);
    ege_enable_aa(true, frameBuffer);

    while (is_run()) {
        processNetwork();
        settarget(frameBuffer);
        switch (screen) {
        case Screen::Menu:
            if (keyPressed('1') || keyPressed(VK_RETURN))
                startRun();
            else if (keyPressed('2'))
                screen = Screen::Shop;
            else if (keyPressed('3'))
                screen = Screen::Help;
            else if (keyPressed('4')) {
                if (!beginHostNetwork()) setMessage(L"主机启动失败，检查端口 8888 是否被占用");
            } else if (keyPressed('5')) {
                if (!beginJoinNetwork()) setMessage(L"网络初始化失败");
            } else if (keyPressed(VK_ESCAPE)) {
                settarget(nullptr);
                closeNetwork();
                delimage(frameBuffer);
                closegraph();
                return 0;
            }
            drawMenu();
            break;
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
                    const std::string addressText(hostIpInput.begin(), hostIpInput.end());
                    peerAddress = {};
                    peerAddress.sin_family = AF_INET;
                    peerAddress.sin_port = htons(8888);
                    if (inet_pton(AF_INET, addressText.c_str(), &peerAddress.sin_addr) == 1) {
                        networkStatus = L"正在连接主机...";
                        screen = Screen::JoinLobby;
                        lastConnectAttempt = Clock::time_point{};
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
            fillRect(0, 0, WindowWidth, WindowHeight, EGEARGB(170, 5, 10, 13));
            drawText(630, 365, L"已暂停", 48, Text);
            drawText(594, 435, L"按 P 继续  /  ESC 返回菜单", 20, Muted);
            if (keyPressed('P'))
                screen = Screen::Playing;
            else if (keyPressed(VK_ESCAPE))
                screen = Screen::Menu;
            break;
        case Screen::Result:
            drawResult();
            if (keyPressed(VK_RETURN)) {
                if (networkActive) {
                    sendNetworkPacket("LEAVE");
                    closeNetwork();
                }
                screen = Screen::Menu;
            }
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