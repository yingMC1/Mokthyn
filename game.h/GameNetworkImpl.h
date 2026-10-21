#pragma once
#include "GameArchive.h"
#include "GameTypes.h"
#include "GameState.h"
#include "GameLogic.h"
#include "GameNetwork.h"
#include "GameRender.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <bcrypt.h>
#include <ws2tcpip.h>

namespace mokthyn {
namespace {
bool createNetworkSessionToken(std::string& token) {
    std::array<unsigned char, 16> randomBytes{};
    if (BCryptGenRandom(nullptr, randomBytes.data(), static_cast<ULONG>(randomBytes.size()),
                        BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0)
        return false;
    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for (const unsigned char byte : randomBytes)
        output << std::setw(2) << static_cast<unsigned int>(byte);
    token = output.str();
    return true;
}
} // namespace

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
    peerAmmo = 0;
    peerAmmoInitialized = false;
    healing = false;
    extracting = false;
    pickupPending = false;
    peerExtracting = false;
    peerExtractConfirmed = false;
    peerArmorInitialized = false;
    peerMedkitsInitialized = false;
    peerHealing = false;
    peerHealStarted = {};
    networkPause = false;
    pausedAt = {};
    peerArmorTier = peerArmorDurability = 0;
    peerArmorOn = false;
    peerAddress = {};
    networkSessionToken.clear();
    lastNetworkReceive = {};
    lastConnectAttempt = {};
    lastEvacSend = {};
    pendingReliablePackets.clear();
    receivedReliablePacketIds.clear();
    receivedReliablePacketOrder.clear();
    networkMapRevision = 0;
    peerMapRevision = 0;
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
    BOOL reuseAddress = TRUE;
    if (setsockopt(networkSocket, SOL_SOCKET, SO_REUSEADDR,
                   reinterpret_cast<const char*>(&reuseAddress),
                   sizeof(reuseAddress)) == SOCKET_ERROR) {
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
    const bool archiveSaved = saveArchive();
    const std::wstring archiveError = archiveStatus;
    if (!initializeNetwork()) return false;
    if (!createNetworkSessionToken(networkSessionToken)) {
        closeNetwork();
        networkStatus = L"无法生成联机会话令牌";
        return false;
    }
    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_port = htons(NetworkPort);
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
    networkMapRevision = 0;
    peerMapRevision = 0;
    lastNetworkSend = {};
    lastNetworkReceive = {};
    networkStatus = archiveSaved
                        ? L"等待玩家连接 · UDP " + std::to_wstring(NetworkPort)
                        : L"联机已启动，但存档失败：" + archiveError;
    screen = Screen::HostLobby;
    return true;
}

bool beginJoinNetwork() {
    const bool archiveSaved = saveArchive();
    const std::wstring archiveError = archiveStatus;
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
    networkSessionToken.clear();
    networkMapRevision = 0;
    peerMapRevision = 0;
    lastNetworkSend = {};
    lastNetworkReceive = {};
    hostIpInput.clear();
    networkStatus = archiveSaved ? L"输入主机的局域网 IPv4 地址"
                                 : L"存档失败：" + archiveError;
    screen = Screen::JoinIp;
    return true;
}

bool sendNetworkPacket(const std::string& packet) {
    if (networkSocket == INVALID_SOCKET || !peerConnected) return false;
    const std::string framed = networkSessionToken.empty()
                                   ? packet
                                   : "AUTH " + networkSessionToken + " " + packet;
    return sendto(networkSocket, framed.c_str(), static_cast<int>(framed.size()), 0,
                  reinterpret_cast<const sockaddr*>(&peerAddress), sizeof(peerAddress)) !=
           SOCKET_ERROR;
}

bool sendReliableNetworkPacket(const std::string& packet) {
    if (networkHost || !networkActive || !peerConnected || networkSocket == INVALID_SOCKET)
        return false;
    if (pendingReliablePackets.size() >= 64) {
        networkStatus = L"可靠网络消息队列已满";
        setMessage(networkStatus);
        return false;
    }
    const unsigned int packetId = nextReliablePacketId++;
    const std::string framed = "REL " + std::to_string(packetId) + " " + packet;
    if (!sendNetworkPacket(framed)) return false;
    pendingReliablePackets.emplace(
        packetId, PendingReliablePacket{framed, Clock::now(), 1});
    return true;
}

std::wstring getLocalIPv4() {
    static std::wstring cachedAddress;
    static Clock::time_point refreshedAt{};
    const auto now = Clock::now();
    if (!cachedAddress.empty() && now - refreshedAt < std::chrono::seconds(5))
        return cachedAddress;

    const auto fallback = [&]() {
        cachedAddress = L"请使用 ipconfig 查看本机 IP";
        refreshedAt = now;
        return cachedAddress;
    };
    char hostName[256]{};
    if (gethostname(hostName, sizeof(hostName)) == SOCKET_ERROR)
        return fallback();
    addrinfo hints{};
    hints.ai_family = AF_INET;
    addrinfo* addresses = nullptr;
    if (getaddrinfo(hostName, nullptr, &hints, &addresses) != 0)
        return fallback();
    std::wstring result;
    for (addrinfo* entry = addresses; entry != nullptr; entry = entry->ai_next) {
        const auto* address = reinterpret_cast<const sockaddr_in*>(entry->ai_addr);
        const unsigned long hostOrder = ntohl(address->sin_addr.s_addr);
        if ((hostOrder >> 24) == 127 || (hostOrder >> 16) == 0xA9FE) continue;
        wchar_t text[INET_ADDRSTRLEN]{};
        if (InetNtopW(AF_INET, const_cast<in_addr*>(&address->sin_addr), text, INET_ADDRSTRLEN)) {
            if (!result.empty()) result += L"  |  ";
            result += text;
        }
    }
    freeaddrinfo(addresses);
    cachedAddress = result.empty() ? L"请使用 ipconfig 查看本机 IP" : result;
    refreshedAt = now;
    return cachedAddress;
}

void sendNetworkState() {
    if (!networkActive || !networkHost || !peerConnected || networkSocket == INVALID_SOCKET) return;
    std::ostringstream packet;
    const bool includeMap = peerMapRevision != networkMapRevision;
    packet << "STATE " << playerX << ' ' << playerY << ' ' << health << ' ' << peerX << ' ' << peerY
           << ' ' << peerHealth << ' ' << peerArmorTier << ' ' << peerArmorDurability << ' '
           << (peerArmorOn ? 1 : 0) << ' '
           << (peerMedkitsInitialized ? peerMedkits : -1) << ' ' << kills << ' ' << totalEnemies
           << ' ' << money << ' '
           << (extracting ? 1 : 0) << ' ' << (peerExtracting ? 1 : 0) << ' '
           << (screen == Screen::Paused ? 1 : 0) << ' '
           << (screen == Screen::Result ? 1 : 0) << ' ' << (won ? 1 : 0) << ' '
           << networkMapRevision << ' ' << (includeMap ? 1 : 0) << ' ' << enemies.size() << ' '
           << (peerAmmoInitialized ? peerAmmo : -1) << ' ';
    for (const Enemy& enemy : enemies)
        packet << enemy.x << ' ' << enemy.y << ' ' << enemy.hp << ' ' << (enemy.alive ? 1 : 0)
               << ' ' << (enemy.awake ? 1 : 0) << ' ' << (enemy.boss ? 1 : 0) << ' ';
    if (includeMap) {
        packet << "MAP ";
        for (const auto& row : world)
            for (int tile : row)
                packet << tile << ' ';
    }
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
    for (int packetsProcessed = 0; packetsProcessed < 32; ++packetsProcessed) {
        int senderLength = sizeof(sender);
        const int received = recvfrom(networkSocket, buffer, sizeof(buffer) - 1, 0,
                                      reinterpret_cast<sockaddr*>(&sender), &senderLength);
        if (received == SOCKET_ERROR) {
            const int socketError = WSAGetLastError();
            if (socketError == WSAECONNRESET) continue;
            if (socketError != WSAEWOULDBLOCK) networkStatus = L"网络接收错误";
            break;
        }
        if (received >= static_cast<int>(sizeof(buffer) - 1)) continue;
        buffer[received] = '\0';
        std::string data(buffer);
        const bool fromPeer = sender.sin_addr.s_addr == peerAddress.sin_addr.s_addr &&
                              sender.sin_port == peerAddress.sin_port;

        if (networkHost && data == "CONNECT") {
            const bool samePeer = peerAddress.sin_addr.s_addr == sender.sin_addr.s_addr &&
                                  peerAddress.sin_port == sender.sin_port;
            if (peerConnected && !samePeer) continue;
            if (!peerConnected) {
                if (!createNetworkSessionToken(networkSessionToken)) continue;
                peerAddress = sender;
                peerConnected = true;
                peerX = peerY = -1;
                peerHealth = 100;
                peerAmmo = 0;
                peerAmmoInitialized = false;
                peerNextShot = now;
                peerMedkits = 0;
                peerMedkitsInitialized = false;
                peerHealing = false;
                peerHealStarted = {};
                peerArmorTier = peerArmorDurability = 0;
                peerArmorOn = false;
                peerArmorInitialized = false;
                peerExtracting = false;
                receivedReliablePacketIds.clear();
                receivedReliablePacketOrder.clear();
                networkStatus = L"玩家已连接，按 ENTER 开始";
            }
            const std::string acknowledgment = "ACK " + networkSessionToken;
            sendto(networkSocket, acknowledgment.c_str(),
                   static_cast<int>(acknowledgment.size()), 0,
                   reinterpret_cast<const sockaddr*>(&peerAddress), sizeof(peerAddress));
            lastNetworkReceive = now;
            continue;
        }
        if (!networkHost && data.rfind("ACK ", 0) == 0 && peerAddress.sin_addr.s_addr != 0 &&
            sender.sin_addr.s_addr == peerAddress.sin_addr.s_addr &&
            sender.sin_port == htons(NetworkPort)) {
            const std::string receivedToken = data.substr(4);
            if (receivedToken.size() != 32 ||
                !std::all_of(receivedToken.begin(), receivedToken.end(), [](unsigned char ch) {
                    return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f');
                }))
                continue;
            networkSessionToken = receivedToken;
            peerAddress = sender;
            peerConnected = true;
            networkStatus = L"已连接，等待主机开始";
            if (screen == Screen::JoinIp) screen = Screen::JoinLobby;
            lastNetworkReceive = now;
            continue;
        }
        if (!fromPeer || !peerConnected) continue;
        const std::string authPrefix = "AUTH " + networkSessionToken + " ";
        if (networkSessionToken.empty() || data.rfind(authPrefix, 0) != 0) continue;
        data.erase(0, authPrefix.size());
        lastNetworkReceive = now;
        std::istringstream input(data);
        std::string command;
        input >> command;

        if (command == "RACK" && !networkHost) {
            unsigned int packetId = 0;
            if (input >> packetId) pendingReliablePackets.erase(packetId);
        } else if (command == "REL" && networkHost) {
            unsigned int packetId = 0;
            std::string payload;
            if (!(input >> packetId >> payload)) continue;
            const std::string ack = "RACK " + std::to_string(packetId);
            sendNetworkPacket(ack);
            if (!receivedReliablePacketIds.insert(packetId).second) continue;
            receivedReliablePacketOrder.push_back(packetId);
            if (receivedReliablePacketOrder.size() > 256) {
                receivedReliablePacketIds.erase(receivedReliablePacketOrder.front());
                receivedReliablePacketOrder.pop_front();
            }
            command = std::move(payload);
        }

        if (command == "POS" && networkHost) {
            int x, y, tier, durability, armorEnabled, reportedMedkits, reportedAmmo;
            if (input >> x >> y >> tier >> durability >> armorEnabled >> reportedMedkits >>
                    reportedAmmo &&
                x > 0 && x < MapColumns - 1 && y > 0 && y < MapRows - 1 &&
                world[y][x] != Wall && !hasEnemyAt(x, y) &&
                (peerX < 0 ||
                 (std::abs(x - peerX) <= 1 && std::abs(y - peerY) <= 1 &&
                  (x == peerX || y == peerY ||
                   (world[peerY][x] != Wall && world[y][peerX] != Wall &&
                    !hasEnemyAt(x, peerY) && !hasEnemyAt(peerX, y)))))) {
                if (peerHealing && (x != peerX || y != peerY)) {
                    peerHealing = false;
                    peerHealStarted = {};
                }
                peerX = x;
                peerY = y;
                if (!peerMedkitsInitialized) {
                    peerMedkits = std::clamp(reportedMedkits, 0, 10000);
                    peerMedkitsInitialized = true;
                }
                if (!peerAmmoInitialized) {
                    peerAmmo = std::clamp(reportedAmmo, 0, 100000);
                    peerAmmoInitialized = true;
                }
                const int reportedTier = std::clamp(tier, 0, 4);
                const int reportedDurability =
                    std::clamp(durability, 0, ArmorDurability[reportedTier]);
                if (!peerArmorInitialized) {
                    peerArmorTier = reportedTier;
                    peerArmorDurability = reportedDurability;
                    peerArmorInitialized = true;
                } else if (peerArmorTier == reportedTier) {
                    peerArmorDurability = std::min(peerArmorDurability, reportedDurability);
                }
                peerArmorOn =
                    armorEnabled != 0 && peerArmorTier > 0 && peerArmorDurability > 0;
                if (peerExtracting && world[peerY][peerX] != ExitTile) {
                    peerExtracting = false;
                    peerExtractStarted = {};
                }
            }
        } else if (command == "HEAL_START" && networkHost) {
            if (screen == Screen::Playing && peerHealth > 0 && peerHealth < 100 &&
                peerMedkits > 0 && !peerHealing && !peerExtracting) {
                --peerMedkits;
                peerHealing = true;
                peerHealStarted = now;
            }
        } else if (command == "HEAL_DONE" && networkHost) {
            if (peerHealing && screen == Screen::Playing && peerHealth > 0 &&
                peerHealth < 100) {
                peerHealth = std::min(100, peerHealth + 20);
            }
            peerHealing = false;
            peerHealStarted = {};
        } else if (command == "HEAL_CANCEL" && networkHost) {
            peerHealing = false;
            peerHealStarted = {};
        } else if (command == "SHOT" && networkHost) {
            int dx, dy;
            if (!(input >> dx >> dy) ||
                !((dx == 0 && (dy == 1 || dy == -1)) ||
                  (dy == 0 && (dx == 1 || dx == -1))) ||
                peerX <= 0 || peerY <= 0 || peerAmmo <= 0 || now < peerNextShot)
                continue;
            --peerAmmo;
            peerNextShot = now + std::chrono::milliseconds(250);
            bool hitEnemy = false;
            for (int step = 1; step <= 9; ++step) {
                const int x = peerX + dx * step;
                const int y = peerY + dy * step;
                if (x < 0 || x >= MapColumns || y < 0 || y >= MapRows ||
                    world[y][x] == Wall)
                    break;
                for (Enemy& enemy : enemies) {
                    if (enemy.alive && enemy.x == x && enemy.y == y) {
                        enemy.hp -= 15;
                        enemy.awake = true;
                        if (enemy.hp <= 0) {
                            enemy.alive = false;
                            ++kills;
                            money += enemy.boss ? 100 : 10;
                        }
                        hitEnemy = true;
                        break;
                    }
                }
                if (hitEnemy) break;
            }
        } else if (command == "PICKUP" && networkHost) {
            int x, y, type;
            if (input >> x >> y >> type && peerX > 0 && peerY > 0 &&
                std::abs(x - peerX) + std::abs(y - peerY) <= 1 && x >= 0 &&
                x < MapColumns && y >= 0 && y < MapRows &&
                (type == AmmoPickup || type == MedkitPickup) && world[y][x] == type) {
                world[y][x] = Floor;
                if (type == MedkitPickup)
                    peerMedkits = std::min(10000, peerMedkits + 1);
                else
                    peerAmmo = std::min(100000, peerAmmo + randomInt(8, 16));
                ++networkMapRevision;
            }
        } else if (command == "MAP_ACK" && networkHost) {
            unsigned int revision = 0;
            if (input >> revision && revision == networkMapRevision)
                peerMapRevision = revision;
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
                if (peerExtracting) {
                    peerHealing = false;
                    peerHealStarted = {};
                }
                if (!peerExtracting) peerExtractStarted = {};
            }
        } else if (command == "LEAVE" && networkHost) {
            peerConnected = false;
            peerX = peerY = -1;
            peerHealth = 0;
            peerAmmo = 0;
            peerAmmoInitialized = false;
            peerMedkits = 0;
            peerMedkitsInitialized = false;
            peerHealing = false;
            peerHealStarted = {};
            peerArmorTier = peerArmorDurability = 0;
            peerArmorOn = false;
            peerArmorInitialized = false;
            peerExtracting = false;
            receivedReliablePacketIds.clear();
            receivedReliablePacketOrder.clear();
            networkStatus = L"队友已离开";
        } else if (command == "START" && !networkHost && screen == Screen::JoinLobby) {
            startRun();
        } else if (command == "STATE" && !networkHost) {
            int hostX, hostY, hostHp, clientX, clientY, clientHp;
            int clientArmorTier, clientArmorDurability, clientArmorOn;
            int syncedMedkits, syncedKills, syncedTotal, syncedMoney, hostEvacuating, peerEvacuating,
                hostPaused, finished, result, mapIncluded, syncedPeerAmmo;
            unsigned int mapRevision;
            int enemyCount;
            if (!(input >> hostX >> hostY >> hostHp >> clientX >> clientY >> clientHp >>
                  clientArmorTier >> clientArmorDurability >> clientArmorOn >> syncedMedkits >> syncedKills >>
                  syncedTotal >> syncedMoney >> hostEvacuating >> peerEvacuating >> hostPaused >>
                  finished >> result >> mapRevision >> mapIncluded >> enemyCount >>
                  syncedPeerAmmo) ||
                  enemyCount < 0 || enemyCount > 30 ||
                  syncedPeerAmmo < -1 || syncedPeerAmmo > 100000 ||
                  (mapIncluded != 0 && mapIncluded != 1) ||
                  (hostEvacuating != 0 && hostEvacuating != 1) ||
                  (peerEvacuating != 0 && peerEvacuating != 1) ||
                  (hostPaused != 0 && hostPaused != 1) ||
                  (finished != 0 && finished != 1) ||
                  (result != 0 && result != 1))
                continue;
            beginNetworkArchiveSession();
            pickupPending = false;
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
            if (syncedPeerAmmo >= 0) ammo = syncedPeerAmmo;
            if (syncedMedkits >= 0) medkits = std::clamp(syncedMedkits, 0, 10000);
            if (extracting && health < previousHealth) cancelExtraction();
            if (health < previousHealth && healing) {
                healing = false;
                sendReliableNetworkPacket("HEAL_CANCEL");
            }
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
                nextPeerHit += pausedFor;
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
            if (mapIncluded != 0) {
                std::string mapMarker;
                if (!(input >> mapMarker) || mapMarker != "MAP") continue;
                bool validMap = true;
                std::array<std::array<int, MapColumns>, MapRows> syncedWorld{};
                for (auto& row : syncedWorld)
                    for (int& tile : row) {
                        if (!(input >> tile) || tile < Floor || tile > MedkitPickup)
                            validMap = false;
                    }
                if (!validMap) continue;
                if (mapRevision >= peerMapRevision) {
                    world = syncedWorld;
                    peerMapRevision = mapRevision;
                    ++visibilityRevision;
                }
                sendNetworkPacket("MAP_ACK " + std::to_string(peerMapRevision));
            }
            {
                enemies = std::move(syncedEnemies);
                totalEnemies = std::max(totalEnemies, static_cast<int>(enemies.size()));
            }
            if (finished != 0) {
                won = result != 0;
                screen = Screen::Result;
            }
        }
    }

    for (auto pending = pendingReliablePackets.begin();
         pending != pendingReliablePackets.end();) {
        auto& packet = pending->second;
        if (packet.attempts >= 20) {
            networkStatus = L"网络消息确认超时";
            setMessage(networkStatus);
            pending = pendingReliablePackets.erase(pending);
            continue;
        }
        if (now - packet.lastSent >= std::chrono::milliseconds(250)) {
            sendNetworkPacket(packet.payload);
            packet.lastSent = now;
            ++packet.attempts;
        }
        ++pending;
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
               << ' ' << (armorOn ? 1 : 0) << ' ' << medkits << ' ' << ammo;
        sendNetworkPacket(packet.str());
        lastNetworkSend = now;
    }
    if (!networkHost && peerConnected && screen == Screen::Playing &&
        now - lastEvacSend >= std::chrono::milliseconds(150)) {
        sendNetworkPacket(std::string("EVAC ") + (extracting ? "1" : "0"));
        lastEvacSend = now;
    }
    if (networkHost && peerExtracting && screen == Screen::Playing &&
        now - peerExtractStarted >= std::chrono::seconds(5) && health > 0 && peerX > 0 &&
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
        peerMedkits = 0;
        peerAmmo = 0;
        peerAmmoInitialized = false;
        peerMedkitsInitialized = false;
        peerHealing = false;
        peerHealStarted = {};
        peerArmorTier = peerArmorDurability = 0;
        peerArmorOn = false;
        peerArmorInitialized = false;
        peerExtracting = false;
        receivedReliablePacketIds.clear();
        receivedReliablePacketOrder.clear();
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