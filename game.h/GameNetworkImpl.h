#pragma once
#include "GameArchive.h"
#include "GameTypes.h"
#include "GameState.h"
#include "GameLogic.h"
#include "GameNetwork.h"
#include "GameRender.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <sstream>
#include <string>
#include <ws2tcpip.h>

namespace mokthyn {
void closeNetwork() {
    if (networkSocket != INVALID_SOCKET) {
        closesocket(networkSocket);
        networkSocket = INVALID_SOCKET;
    }
    if (networkStarted) {
        WSACleanup();
        networkStarted = false;
    }
    networkActive = false;
    networkHost = false;
    peerConnected = false;
    peerX = peerY = -1;
    healing = false;
    extracting = false;
    peerExtracting = false;
    peerExtractConfirmed = false;
    peerArmorInitialized = false;
    networkPause = false;
    pausedAt = {};
    peerArmorTier = peerArmorDurability = 0;
    peerArmorOn = false;
    peerAddress = {};
    lastNetworkReceive = {};
    lastConnectAttempt = {};
    lastEvacSend = {};
}

bool initializeNetwork() {
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) return false;
    networkStarted = true;
    networkSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (networkSocket == INVALID_SOCKET) {
        closeNetwork();
        return false;
    }
    u_long nonBlocking = 1;
    if (ioctlsocket(networkSocket, FIONBIO, &nonBlocking) == SOCKET_ERROR) {
        closeNetwork();
        return false;
    }
    return true;
}

bool beginHostNetwork() {
    saveArchive();
    if (!initializeNetwork()) return false;
    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_port = htons(8888);
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(networkSocket, reinterpret_cast<sockaddr*>(&local), sizeof(local)) == SOCKET_ERROR) {
        closeNetwork();
        return false;
    }
    networkHost = true;
    networkActive = true;
    peerConnected = false;
    peerAddress = {};
    peerArmorInitialized = false;
    lastNetworkSend = {};
    lastNetworkReceive = {};
    networkStatus = L"等待玩家连接 · UDP 8888";
    screen = Screen::HostLobby;
    return true;
}

bool beginJoinNetwork() {
    saveArchive();
    if (!initializeNetwork()) return false;
    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_port = 0;
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(networkSocket, reinterpret_cast<sockaddr*>(&local), sizeof(local)) == SOCKET_ERROR) {
        closeNetwork();
        return false;
    }
    networkHost = false;
    networkActive = true;
    peerConnected = false;
    peerAddress = {};
    lastNetworkSend = {};
    lastNetworkReceive = {};
    hostIpInput.clear();
    networkStatus = L"输入主机的局域网 IPv4 地址";
    screen = Screen::JoinIp;
    return true;
}

bool sendNetworkPacket(const std::string& packet) {
    return networkSocket != INVALID_SOCKET && peerConnected &&
           sendto(networkSocket, packet.c_str(), static_cast<int>(packet.size()), 0,
                  reinterpret_cast<const sockaddr*>(&peerAddress),
                  sizeof(peerAddress)) != SOCKET_ERROR;
}

std::wstring getLocalIPv4() {
    char hostName[256]{};
    if (gethostname(hostName, sizeof(hostName)) == SOCKET_ERROR)
        return L"请使用 ipconfig 查看本机 IP";
    addrinfo hints{};
    hints.ai_family = AF_INET;
    addrinfo* addresses = nullptr;
    if (getaddrinfo(hostName, nullptr, &hints, &addresses) != 0)
        return L"请使用 ipconfig 查看本机 IP";
    std::wstring result;
    for (addrinfo* entry = addresses; entry != nullptr; entry = entry->ai_next) {
        const auto* address = reinterpret_cast<const sockaddr_in*>(entry->ai_addr);
        const unsigned long hostOrder = ntohl(address->sin_addr.s_addr);
        if ((hostOrder >> 24) == 127 || (hostOrder >> 16) == 0xA9FE) continue;
        wchar_t text[INET_ADDRSTRLEN]{};
        if (InetNtopW(AF_INET, const_cast<in_addr*>(&address->sin_addr), text, INET_ADDRSTRLEN)) {
            result = text;
            break;
        }
    }
    freeaddrinfo(addresses);
    return result.empty() ? L"请使用 ipconfig 查看本机 IP" : result;
}

void sendNetworkState() {
    if (!networkActive || !networkHost || !peerConnected || networkSocket == INVALID_SOCKET) return;
    std::ostringstream packet;
    packet << "STATE " << playerX << ' ' << playerY << ' ' << health << ' ' << peerX << ' ' << peerY
           << ' ' << peerHealth << ' ' << peerArmorTier << ' ' << peerArmorDurability << ' '
           << (peerArmorOn ? 1 : 0) << ' ' << kills << ' ' << totalEnemies << ' ' << money << ' '
           << (extracting ? 1 : 0) << ' ' << (peerExtracting ? 1 : 0) << ' '
           << (screen == Screen::Paused ? 1 : 0) << ' '
           << (screen == Screen::Result ? 1 : 0) << ' ' << (won ? 1 : 0) << ' ' << enemies.size()
           << ' ';
    for (const Enemy& enemy : enemies)
        packet << enemy.x << ' ' << enemy.y << ' ' << enemy.hp << ' ' << (enemy.alive ? 1 : 0)
               << ' ' << (enemy.awake ? 1 : 0) << ' ' << (enemy.boss ? 1 : 0) << ' ';
    packet << "MAP ";
    for (const auto& row : world)
        for (int tile : row)
            packet << tile << ' ';
    sendNetworkPacket(packet.str());
    lastNetworkSend = Clock::now();
}

void processNetwork() {
    if (!networkActive || networkSocket == INVALID_SOCKET) return;
    const auto now = Clock::now();
    if (!networkHost && (!peerConnected || screen == Screen::JoinLobby) &&
        peerAddress.sin_addr.s_addr != 0 &&
        peerAddress.sin_port != 0 &&
        now - lastConnectAttempt >= std::chrono::milliseconds(500)) {
        sendto(networkSocket, "CONNECT", 7, 0, reinterpret_cast<const sockaddr*>(&peerAddress),
               sizeof(peerAddress));
        lastConnectAttempt = now;
    }

    char buffer[8192];
    sockaddr_in sender{};
    int senderLength = sizeof(sender);
    for (;;) {
        const int received = recvfrom(networkSocket, buffer, sizeof(buffer) - 1, 0,
                                      reinterpret_cast<sockaddr*>(&sender), &senderLength);
        if (received == SOCKET_ERROR) {
            if (WSAGetLastError() != WSAEWOULDBLOCK) networkStatus = L"网络接收错误";
            break;
        }
        if (received >= static_cast<int>(sizeof(buffer) - 1)) continue;
        buffer[received] = '\0';
        const std::string data(buffer);
        const bool fromPeer = sender.sin_addr.s_addr == peerAddress.sin_addr.s_addr &&
                              sender.sin_port == peerAddress.sin_port;

        if (networkHost && data == "CONNECT") {
            const bool samePeer = peerAddress.sin_addr.s_addr == sender.sin_addr.s_addr &&
                                  peerAddress.sin_port == sender.sin_port;
            if (peerConnected && !samePeer) continue;
            if (!peerConnected) {
                peerAddress = sender;
                peerConnected = true;
                peerX = peerY = -1;
                peerHealth = 100;
                peerArmorTier = peerArmorDurability = 0;
                peerArmorOn = false;
                peerArmorInitialized = false;
                peerExtracting = false;
                networkStatus = L"玩家已连接，按 ENTER 开始";
            }
            sendto(networkSocket, "ACK", 3, 0, reinterpret_cast<const sockaddr*>(&peerAddress),
                   sizeof(peerAddress));
            lastNetworkReceive = now;
            continue;
        }
        if (!networkHost && data == "ACK" && peerAddress.sin_addr.s_addr != 0 &&
            sender.sin_addr.s_addr == peerAddress.sin_addr.s_addr &&
            sender.sin_port == htons(NetworkPort)) {
            peerAddress = sender;
            peerConnected = true;
            networkStatus = L"已连接，等待主机开始";
            if (screen == Screen::JoinIp) screen = Screen::JoinLobby;
            lastNetworkReceive = now;
            continue;
        }
        if (!fromPeer) continue;
        lastNetworkReceive = now;
        std::istringstream input(data);
        std::string command;
        input >> command;

        if (command == "POS" && networkHost) {
            int x, y, tier, durability, armorEnabled;
            if (input >> x >> y >> tier >> durability >> armorEnabled && x > 0 &&
                x < MapColumns - 1 && y > 0 && y < MapRows - 1 && world[y][x] != Wall) {
                peerX = x;
                peerY = y;
                const int reportedTier = std::clamp(tier, 0, 4);
                const int reportedDurability =
                    std::clamp(durability, 0, ArmorDurability[reportedTier]);
                if (!peerArmorInitialized) {
                    peerArmorTier = reportedTier;
                    peerArmorDurability = reportedDurability;
                    peerArmorInitialized = true;
                } else if (peerArmorTier == reportedTier) {
                    peerArmorDurability = std::min(peerArmorDurability, reportedDurability);
                } else {
                    peerArmorTier = reportedTier;
                    peerArmorDurability = reportedDurability;
                }
                peerArmorOn =
                    armorEnabled != 0 && peerArmorTier > 0 && peerArmorDurability > 0;
                if (peerExtracting && world[peerY][peerX] != ExitTile) {
                    peerExtracting = false;
                    peerExtractStarted = {};
                }
            }
        } else if (command == "HEAL" && networkHost) {
            peerHealth = std::min(100, peerHealth + 20);
        } else if (command == "HIT" && networkHost) {
            int x, y, damage;
            if (!(input >> x >> y >> damage) || x < 0 || x >= MapColumns || y < 0 || y >= MapRows)
                continue;
            for (Enemy& enemy : enemies) {
                if (enemy.alive && enemy.x == x && enemy.y == y) {
                    enemy.hp -= std::clamp(damage, 1, 30);
                    enemy.awake = true;
                    if (enemy.hp <= 0) {
                        enemy.alive = false;
                        ++kills;
                        money += enemy.boss ? 100 : 10;
                    }
                    break;
                }
            }
        } else if (command == "PICKUP" && networkHost) {
            int x, y, type;
            if (input >> x >> y >> type && x >= 0 && x < MapColumns && y >= 0 && y < MapRows &&
                (type == AmmoPickup || type == MedkitPickup) && world[y][x] == type)
                world[y][x] = Floor;
        } else if (command == "EVAC" && networkHost) {
            int status;
            if (input >> status && (status == 0 || status == 1)) {
                const bool requested =
                    status == 1 && peerX > 0 && peerX < MapColumns - 1 && peerY > 0 &&
                    peerY < MapRows - 1 && world[peerY][peerX] == ExitTile && peerHealth > 0 &&
                    std::none_of(enemies.begin(), enemies.end(), [](const Enemy& enemy) {
                        return enemy.boss && enemy.alive;
                    });
                if (requested && !peerExtracting) peerExtractStarted = now;
                peerExtracting = requested;
                if (!peerExtracting) peerExtractStarted = {};
            }
        } else if (command == "LEAVE" && networkHost) {
            peerConnected = false;
            peerX = peerY = -1;
            peerHealth = 0;
            peerArmorTier = peerArmorDurability = 0;
            peerArmorOn = false;
            peerArmorInitialized = false;
            peerExtracting = false;
            networkStatus = L"队友已离开";
        } else if (command == "START" && !networkHost && screen == Screen::JoinLobby) {
            startRun();
        } else if (command == "STATE" && !networkHost) {
            int hostX, hostY, hostHp, clientX, clientY, clientHp;
            int clientArmorTier, clientArmorDurability, clientArmorOn;
            int syncedKills, syncedTotal, syncedMoney, hostEvacuating, peerEvacuating,
                hostPaused, finished, result;
            int enemyCount;
            if (!(input >> hostX >> hostY >> hostHp >> clientX >> clientY >> clientHp >>
                  clientArmorTier >> clientArmorDurability >> clientArmorOn >> syncedKills >>
                  syncedTotal >> syncedMoney >> hostEvacuating >> peerEvacuating >> hostPaused >>
                  finished >> result >> enemyCount) ||
                  enemyCount < 0 || enemyCount > 30 ||
                  (hostEvacuating != 0 && hostEvacuating != 1) ||
                  (peerEvacuating != 0 && peerEvacuating != 1) ||
                  (hostPaused != 0 && hostPaused != 1) ||
                  (finished != 0 && finished != 1) ||
                  (result != 0 && result != 1))
                continue;
            beginNetworkArchiveSession();
            const bool awaitingStart =
                screen == Screen::JoinLobby || screen == Screen::JoinIp;
            if (awaitingStart) startRun();
            const int previousHealth = health;
            peerAddress = sender;
            peerConnected = true;
            if (hostX >= 0 && hostX < MapColumns && hostY >= 0 && hostY < MapRows) {
                peerX = hostX;
                peerY = hostY;
            }
            peerHealth = std::clamp(hostHp, 0, 100);
            health = std::clamp(clientHp, 0, 100);
            if (extracting && health < previousHealth) cancelExtraction();
            armorTier = std::clamp(clientArmorTier, 0, 4);
            armorDurability = std::clamp(clientArmorDurability, 0, ArmorDurability[armorTier]);
            armorOn = clientArmorOn != 0 && armorTier > 0 && armorDurability > 0;
            kills = std::clamp(syncedKills, 0, 100000);
            totalEnemies = std::clamp(syncedTotal, 0, 100000);
            money = std::clamp(syncedMoney, 0, 100000000);
            peerExtracting = hostEvacuating != 0;
            if (extracting && peerEvacuating != 0) {
                peerExtractConfirmed = true;
            } else if (extracting && peerExtractConfirmed && peerEvacuating == 0) {
                cancelExtraction();
            }
            const bool wasNetworkPaused = networkPause;
            networkPause = hostPaused != 0;
            if (networkPause && !wasNetworkPaused) {
                pausedAt = now;
                if (screen == Screen::Playing) screen = Screen::Paused;
            } else if (!networkPause && wasNetworkPaused) {
                const auto pausedFor = now - pausedAt;
                if (healing) healStarted += pausedFor;
                if (extracting) extractStarted += pausedFor;
                lastMove += pausedFor;
                lastEnemyTurn += pausedFor;
                nextPlayerHit += pausedFor;
                pausedAt = {};
                if (screen == Screen::Paused) screen = Screen::Playing;
            }
            if (awaitingStart) {
                setMessage(L"联机对局已开始");
            }
            std::vector<Enemy> syncedEnemies;
            syncedEnemies.reserve(enemyCount);
            for (int i = 0; i < enemyCount; ++i) {
                Enemy enemy;
                int alive, awake, boss;
                if (!(input >> enemy.x >> enemy.y >> enemy.hp >> alive >> awake >> boss)) break;
                if (enemy.x > 0 && enemy.x < MapColumns - 1 && enemy.y > 0 &&
                    enemy.y < MapRows - 1 && enemy.hp >= 0 &&
                    enemy.hp <= (boss != 0 ? BossHealth : EnemyHealth) &&
                    (alive == 0 || alive == 1) && (awake == 0 || awake == 1) &&
                    (boss == 0 || boss == 1))
                    syncedEnemies.push_back(
                        {enemy.x, enemy.y, enemy.hp, alive != 0, awake != 0, boss != 0, now});
            }
            std::string mapMarker;
            input >> mapMarker;
            if (mapMarker == "MAP") {
                for (auto& row : world)
                    for (int& tile : row) {
                        if (!(input >> tile) || tile < Floor || tile > MedkitPickup) tile = Floor;
                    }
                enemies = std::move(syncedEnemies);
                totalEnemies = std::max(totalEnemies, static_cast<int>(enemies.size()));
            }
            if (finished != 0) {
                won = result != 0;
                screen = Screen::Result;
            }
        }
    }

    if (networkHost && peerConnected &&
        (screen == Screen::Playing || screen == Screen::Paused || screen == Screen::Result) &&
        now - lastNetworkSend >= std::chrono::milliseconds(200))
        sendNetworkState();
    if (!networkHost && peerConnected &&
        (screen == Screen::Playing || screen == Screen::Paused) &&
        now - lastNetworkSend >= std::chrono::milliseconds(100)) {
        std::ostringstream packet;
        packet << "POS " << playerX << ' ' << playerY << ' ' << armorTier << ' ' << armorDurability
               << ' ' << (armorOn ? 1 : 0);
        sendNetworkPacket(packet.str());
        lastNetworkSend = now;
    }
    if (!networkHost && peerConnected && screen == Screen::Playing &&
        now - lastEvacSend >= std::chrono::milliseconds(150)) {
        sendNetworkPacket(std::string("EVAC ") + (extracting ? "1" : "0"));
        lastEvacSend = now;
    }
    if (networkHost && peerExtracting && screen == Screen::Playing &&
        now - peerExtractStarted >= std::chrono::seconds(5) && peerX > 0 &&
        peerX < MapColumns - 1 && peerY > 0 && peerY < MapRows - 1 &&
        world[peerY][peerX] == ExitTile && peerHealth > 0) {
        won = true;
        screen = Screen::Result;
        peerExtracting = false;
        sendNetworkState();
    }
    if (networkHost && peerConnected &&
        (screen == Screen::Playing || screen == Screen::Paused) &&
        now - lastNetworkReceive > std::chrono::seconds(5)) {
        peerConnected = false;
        peerX = peerY = -1;
        peerHealth = 0;
        peerArmorTier = peerArmorDurability = 0;
        peerArmorOn = false;
        peerArmorInitialized = false;
        peerExtracting = false;
        networkStatus = L"队友连接中断";
    }
    if (!networkHost && !peerConnected && screen == Screen::JoinLobby &&
        lastNetworkReceive != Clock::time_point{} &&
        now - lastNetworkReceive > std::chrono::seconds(10)) {
        networkStatus = L"连接超时，请检查主机地址和防火墙";
        closeNetwork();
        screen = Screen::Menu;
        setMessage(networkStatus, 4000);
    }
    if (!networkHost && peerConnected &&
        (screen == Screen::Playing || screen == Screen::Paused || screen == Screen::Result ||
         screen == Screen::JoinLobby) &&
        now - lastNetworkReceive > std::chrono::seconds(5)) {
        networkStatus = L"主机连接中断";
        closeNetwork();
        won = false;
        screen = Screen::Result;
    }
}

}