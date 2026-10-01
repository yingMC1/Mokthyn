#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#define WIN32_LEAN_AND_MEAN
#include <graphics.h>
#include <random>
#include <sstream>
#include <string>
#include <vector>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

namespace {
using Clock = std::chrono::steady_clock;

constexpr int LogicalWidth = 1600;
constexpr int LogicalHeight = 900;
constexpr int WindowWidth = 1200;
constexpr int WindowHeight = 675;
constexpr float RenderScale = 0.75f;
constexpr int MapColumns = 22;
constexpr int MapRows = 17;
constexpr int TileSize = 40;
constexpr int MapLeft = 64;
constexpr int MapTop = 128;
constexpr int ViewRadius = 10;
constexpr int EnemyHealth = 45;

const ege::color_t Background = EGERGB(12, 18, 24);
const ege::color_t Panel = EGERGB(21, 30, 37);
const ege::color_t PanelLight = EGERGB(30, 42, 49);
const ege::color_t Text = EGERGB(226, 235, 230);
const ege::color_t Muted = EGERGB(137, 157, 157);
const ege::color_t Green = EGERGB(101, 220, 155);
const ege::color_t Yellow = EGERGB(245, 194, 90);
const ege::color_t Red = EGERGB(235, 99, 91);
const ege::color_t Cyan = EGERGB(91, 200, 214);

enum Tile : int { Floor, Wall, AmmoPickup, ExitTile, MedkitPickup };
enum class Screen { Menu, Shop, Help, Playing, Paused, Result, HostLobby, JoinIp, JoinLobby };

struct Enemy {
    int x = 0;
    int y = 0;
    int hp = EnemyHealth;
    bool alive = true;
    bool awake = false;
    Clock::time_point nextAttack = Clock::now();
};

std::array<std::array<int, MapColumns>, MapRows> world{};
std::array<std::array<bool, MapColumns>, MapRows> visible{};
std::vector<Enemy> enemies;
std::mt19937 randomEngine(static_cast<unsigned int>(Clock::now().time_since_epoch().count()));

Screen screen = Screen::Menu;
int playerX = MapColumns / 2;
int playerY = MapRows / 2;
int health = 100;
int ammo = 30;
int medkits = 2;
int money = 200;
int armorTier = 0;
int armorDurability = 0;
int kills = 0;
int totalEnemies = 0;
bool armorOn = false;
bool healing = false;
bool extracting = false;
bool won = false;
Clock::time_point healStarted{};
Clock::time_point extractStarted{};
Clock::time_point lastMove{};
Clock::time_point lastEnemyTurn{};
Clock::time_point nextPlayerHit{};
Clock::time_point messageUntil{};
std::wstring message;
int shopSelection = 0;
PIMAGE frameBuffer = nullptr;
SOCKET networkSocket = INVALID_SOCKET;
sockaddr_in peerAddress{};
bool networkStarted = false;
bool networkActive = false;
bool networkHost = false;
bool peerConnected = false;
int peerX = -1;
int peerY = -1;
int peerHealth = 100;
int peerArmorTier = 0;
int peerArmorDurability = 0;
bool peerArmorOn = false;
std::string hostIpInput;
std::wstring networkStatus;
Clock::time_point lastNetworkSend{};
Clock::time_point lastNetworkReceive{};
Clock::time_point lastConnectAttempt{};
Clock::time_point peerExtractStarted{};
bool peerExtracting = false;

const std::array<int, 5> ArmorPrices = {0, 100, 200, 350, 550};
const std::array<int, 5> ArmorDurability = {0, 50, 75, 100, 125};
const std::array<int, 5> ArmorReduction = {0, 4, 7, 10, 13};

int scaled(int value) { return static_cast<int>(std::lround(value * RenderScale)); }

int randomInt(int low, int high) {
    return std::uniform_int_distribution<int>(low, high)(randomEngine);
}

void setMessage(const std::wstring& text, int milliseconds = 1800) {
    message = text;
    messageUntil = Clock::now() + std::chrono::milliseconds(milliseconds);
}

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
    peerExtracting = false;
    peerArmorTier = peerArmorDurability = 0;
    peerArmorOn = false;
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
    networkStatus = L"等待玩家连接 · UDP 8888";
    screen = Screen::HostLobby;
    return true;
}

bool beginJoinNetwork() {
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

void startRun();
void processNetwork();
void drawNetworkLobby();
void sendNetworkState();

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

void drawText(int x, int y, const std::wstring& value, int size = 20, ege::color_t color = Text) {
    setbkmode(TRANSPARENT, frameBuffer);
    setfontbkcolor(Background, frameBuffer);
    setfont(std::max(1, scaled(size)), 0, L"Microsoft YaHei UI");
    settextcolor(color);
    outtextxy(scaled(x), scaled(y), value.c_str());
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
    for (int y = 0; y < MapRows; ++y) {
        for (int x = 0; x < MapColumns; ++x) {
            const int distance = std::abs(x - playerX) + std::abs(y - playerY);
            visible[y][x] = distance <= ViewRadius && lineOfSight(playerX, playerY, x, y);
        }
    }
}

bool hasEnemyAt(int x, int y) {
    return std::any_of(enemies.begin(), enemies.end(), [=](const Enemy& enemy) {
        return enemy.alive && enemy.x == x && enemy.y == y;
    });
}

void placePickup(int type, int minimumDistance) {
    for (int attempt = 0; attempt < 400; ++attempt) {
        const int x = randomInt(1, MapColumns - 2);
        const int y = randomInt(1, MapRows - 2);
        if (world[y][x] == Floor && !hasEnemyAt(x, y) &&
            std::abs(x - playerX) + std::abs(y - playerY) >= minimumDistance) {
            world[y][x] = type;
            return;
        }
    }
}

void startRun() {
    for (auto& row : world)
        row.fill(Floor);
    for (int x = 0; x < MapColumns; ++x)
        world[0][x] = world[MapRows - 1][x] = Wall;
    for (int y = 0; y < MapRows; ++y)
        world[y][0] = world[y][MapColumns - 1] = Wall;

    playerX = MapColumns / 2;
    playerY = MapRows / 2;
    for (int block = 0; block < 9; ++block) {
        const int width = randomInt(2, 4);
        const int height = randomInt(2, 3);
        const int left = randomInt(2, MapColumns - width - 2);
        const int top = randomInt(2, MapRows - height - 2);
        if (std::abs(left - playerX) + std::abs(top - playerY) < 5) continue;
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
    for (int i = 0; i < 7; ++i) {
        for (int attempt = 0; attempt < 400; ++attempt) {
            const int x = randomInt(1, MapColumns - 2);
            const int y = randomInt(1, MapRows - 2);
            if (world[y][x] == Floor && !hasEnemyAt(x, y) &&
                std::abs(x - playerX) + std::abs(y - playerY) >= 8) {
                enemies.push_back({x, y, EnemyHealth, true, false, Clock::now()});
                break;
            }
        }
    }
    totalEnemies = static_cast<int>(enemies.size());
    for (int i = 0; i < 7; ++i)
        placePickup(AmmoPickup, 4);
    for (int i = 0; i < 4; ++i)
        placePickup(MedkitPickup, 5);
    kills = 0;
    health = 100;
    armorOn = armorTier > 0 && armorDurability > 0;
    healing = extracting = false;
    lastMove = lastEnemyTurn = Clock::now();
    nextPlayerHit = Clock::now();
    screen = Screen::Playing;
    setMessage(L"清除区域内的敌人，然后前往撤离点");
    if (networkActive && networkHost && peerConnected) {
        sendNetworkPacket("START");
        sendNetworkState();
    }
}

void sendNetworkState() {
    if (!networkActive || !networkHost || !peerConnected || networkSocket == INVALID_SOCKET) return;
    std::ostringstream packet;
    packet << "STATE " << playerX << ' ' << playerY << ' ' << health << ' ' << peerX << ' ' << peerY
           << ' ' << peerHealth << ' ' << peerArmorTier << ' ' << peerArmorDurability << ' '
           << (peerArmorOn ? 1 : 0) << ' ' << kills << ' ' << totalEnemies << ' ' << money << ' '
           << (extracting ? 1 : 0) << ' ' << (peerExtracting ? 1 : 0) << ' '
           << (screen == Screen::Result ? 1 : 0) << ' ' << (won ? 1 : 0) << ' ' << enemies.size()
           << ' ';
    for (const Enemy& enemy : enemies)
        packet << enemy.x << ' ' << enemy.y << ' ' << enemy.hp << ' ' << (enemy.alive ? 1 : 0)
               << ' ' << (enemy.awake ? 1 : 0) << ' ';
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
    if (!networkHost && !peerConnected &&
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
        buffer[received] = '\0';
        const std::string data(buffer);
        const bool fromPeer = sender.sin_addr.s_addr == peerAddress.sin_addr.s_addr &&
                              sender.sin_port == peerAddress.sin_port;

        if (networkHost && !peerConnected && data == "CONNECT") {
            peerAddress = sender;
            peerConnected = true;
            peerX = peerY = -1;
            peerHealth = 100;
            networkStatus = L"玩家已连接，按 ENTER 开始";
            sendto(networkSocket, "ACK", 3, 0, reinterpret_cast<const sockaddr*>(&peerAddress),
                   sizeof(peerAddress));
            lastNetworkReceive = now;
            continue;
        }
        if (!networkHost && !peerConnected && data == "ACK" &&
            sender.sin_addr.s_addr == peerAddress.sin_addr.s_addr) {
            peerAddress = sender;
            peerConnected = true;
            networkStatus = L"已连接，等待主机开始";
            screen = Screen::JoinLobby;
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
                peerArmorTier = std::clamp(tier, 0, 4);
                peerArmorDurability = std::clamp(durability, 0, ArmorDurability[peerArmorTier]);
                peerArmorOn = armorEnabled != 0 && peerArmorTier > 0 && peerArmorDurability > 0;
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
                        money += 10;
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
            if (input >> status) {
                peerExtracting = status == 1;
                if (peerExtracting) peerExtractStarted = now;
            }
        } else if (command == "LEAVE" && networkHost) {
            peerConnected = false;
            peerX = peerY = -1;
            peerHealth = 0;
            peerArmorTier = peerArmorDurability = 0;
            peerArmorOn = false;
            peerExtracting = false;
            networkStatus = L"队友已离开";
        } else if (command == "START" && !networkHost && screen == Screen::JoinLobby) {
            startRun();
        } else if (command == "STATE" && !networkHost) {
            int hostX, hostY, hostHp, clientX, clientY, clientHp;
            int clientArmorTier, clientArmorDurability, clientArmorOn;
            int syncedKills, syncedTotal, syncedMoney, hostEvacuating, peerEvacuating, finished,
                result;
            int enemyCount;
            if (!(input >> hostX >> hostY >> hostHp >> clientX >> clientY >> clientHp >>
                  clientArmorTier >> clientArmorDurability >> clientArmorOn >> syncedKills >>
                  syncedTotal >> syncedMoney >> hostEvacuating >> peerEvacuating >> finished >>
                  result >> enemyCount) ||
                enemyCount < 0 || enemyCount > 30)
                continue;
            if (hostX >= 0 && hostX < MapColumns && hostY >= 0 && hostY < MapRows) {
                peerX = hostX;
                peerY = hostY;
            }
            peerHealth = std::clamp(hostHp, 0, 100);
            health = std::clamp(clientHp, 0, 100);
            armorTier = std::clamp(clientArmorTier, 0, 4);
            armorDurability = std::clamp(clientArmorDurability, 0, ArmorDurability[armorTier]);
            armorOn = clientArmorOn != 0 && armorTier > 0 && armorDurability > 0;
            kills = std::max(0, syncedKills);
            totalEnemies = std::max(0, syncedTotal);
            money = std::max(0, syncedMoney);
            extracting = hostEvacuating != 0 || peerEvacuating != 0;
            std::vector<Enemy> syncedEnemies;
            syncedEnemies.reserve(enemyCount);
            for (int i = 0; i < enemyCount; ++i) {
                Enemy enemy;
                int alive, awake;
                if (!(input >> enemy.x >> enemy.y >> enemy.hp >> alive >> awake)) break;
                if (enemy.x >= 0 && enemy.x < MapColumns && enemy.y >= 0 && enemy.y < MapRows)
                    syncedEnemies.push_back(
                        {enemy.x, enemy.y, enemy.hp, alive != 0, awake != 0, now});
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

    if (networkHost && peerConnected && (screen == Screen::Playing || screen == Screen::Result) &&
        now - lastNetworkSend >= std::chrono::milliseconds(200))
        sendNetworkState();
    if (!networkHost && peerConnected && screen == Screen::Playing &&
        now - lastNetworkSend >= std::chrono::milliseconds(100)) {
        std::ostringstream packet;
        packet << "POS " << playerX << ' ' << playerY << ' ' << armorTier << ' ' << armorDurability
               << ' ' << (armorOn ? 1 : 0);
        sendNetworkPacket(packet.str());
        lastNetworkSend = now;
    }
    if (networkHost && peerExtracting && screen == Screen::Playing &&
        now - peerExtractStarted >= std::chrono::seconds(5)) {
        won = true;
        screen = Screen::Result;
        peerExtracting = false;
        sendNetworkState();
    }
    if (!networkHost && peerConnected && screen == Screen::Playing &&
        now - lastNetworkReceive > std::chrono::seconds(5)) {
        networkStatus = L"主机连接中断";
        closeNetwork();
        won = false;
        screen = Screen::Result;
    }
}

void shoot(int dx, int dy) {
    if (ammo <= 0) {
        setMessage(L"弹药不足，寻找弹药箱");
        return;
    }
    --ammo;
    for (int step = 1; step <= 9; ++step) {
        const int x = playerX + dx * step;
        const int y = playerY + dy * step;
        if (x < 0 || x >= MapColumns || y < 0 || y >= MapRows || world[y][x] == Wall) break;
        auto found = std::find_if(enemies.begin(), enemies.end(), [=](const Enemy& enemy) {
            return enemy.alive && enemy.x == x && enemy.y == y;
        });
        if (found != enemies.end()) {
            const int targetX = found->x;
            const int targetY = found->y;
            found->hp -= 15;
            found->awake = true;
            if (networkActive && !networkHost) {
                sendNetworkPacket("HIT " + std::to_string(targetX) + " " + std::to_string(targetY) +
                                  " 15");
            }
            if (found->hp <= 0) {
                found->alive = false;
                ++kills;
                money += 10;
                setMessage(L"目标消灭  +10 金币");
            } else {
                setMessage(L"命中目标");
            }
            return;
        }
    }
    setMessage(L"未命中");
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

        if (enemy.awake && distance > 1) {
            const int dx = (targetX > enemy.x) - (targetX < enemy.x);
            const int dy = (targetY > enemy.y) - (targetY < enemy.y);
            const std::array<std::array<int, 2>, 4> directions = {
                {{dx, dy}, {dx, 0}, {0, dy}, {0, 0}}};
            for (const auto& direction : directions) {
                const int nextX = enemy.x + direction[0];
                const int nextY = enemy.y + direction[1];
                if (world[nextY][nextX] == Floor && !hasEnemyAt(nextX, nextY) &&
                    (nextX != playerX || nextY != playerY) && (nextX != peerX || nextY != peerY)) {
                    enemy.x = nextX;
                    enemy.y = nextY;
                    break;
                }
            }
        }

        const int currentDistance = std::abs(enemy.x - targetX) + std::abs(enemy.y - targetY);
        if (currentDistance <= 1 && now >= enemy.nextAttack && now >= nextPlayerHit) {
            if (targetPeer) {
                int damage = 9;
                if (peerArmorOn && peerArmorDurability > 0) {
                    damage = std::max(1, damage - ArmorReduction[peerArmorTier]);
                    peerArmorDurability = std::max(0, peerArmorDurability - 4);
                    if (peerArmorDurability == 0) peerArmorOn = false;
                }
                peerHealth = std::max(0, peerHealth - damage);
                if (peerHealth == 0) {
                    won = false;
                    screen = Screen::Result;
                }
            } else {
                health -= takeDamage(9);
            }
            enemy.nextAttack = now + std::chrono::milliseconds(1800);
            nextPlayerHit = now + std::chrono::milliseconds(850);
            if (targetPeer && peerExtracting) peerExtracting = false;
            if (healing || extracting) {
                healing = extracting = false;
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
        const int found = randomInt(8, 16);
        ammo += found;
        tile = Floor;
        if (networkActive && !networkHost)
            sendNetworkPacket("PICKUP " + std::to_string(pickupX) + " " + std::to_string(pickupY) +
                              " " + std::to_string(AmmoPickup));
        setMessage(L"拾取弹药  +" + std::to_wstring(found));
    } else if (tile == MedkitPickup) {
        const int pickupX = playerX;
        const int pickupY = playerY;
        ++medkits;
        tile = Floor;
        if (networkActive && !networkHost)
            sendNetworkPacket("PICKUP " + std::to_string(pickupX) + " " + std::to_string(pickupY) +
                              " " + std::to_string(MedkitPickup));
        setMessage(L"拾取医疗包  +1");
    } else {
        setMessage(L"这里没有可拾取的物资");
    }
}

void movePlayer(int dx, int dy) {
    const int nextX = playerX + dx;
    const int nextY = playerY + dy;
    if (nextX <= 0 || nextX >= MapColumns - 1 || nextY <= 0 || nextY >= MapRows - 1 ||
        world[nextY][nextX] == Wall || hasEnemyAt(nextX, nextY))
        return;
    playerX = nextX;
    playerY = nextY;
    if (healing || extracting) {
        if (extracting && networkActive && !networkHost) sendNetworkPacket("EVAC 0");
        healing = extracting = false;
        setMessage(L"动作已取消");
    }
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

void drawMenu() {
    cleardevice();
    fillRect(80, 78, 1520, 822, Panel);
    fillRect(80, 78, 90, 822, Green);
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

bool keyPressed(int key) { return keypress(key) > 0; }

void updatePlaying() {
    const auto now = Clock::now();
    if (keyPressed(VK_ESCAPE)) {
        if (networkActive) {
            sendNetworkPacket("LEAVE");
            closeNetwork();
        }
        screen = Screen::Menu;
        return;
    }
    if (keyPressed('P')) {
        screen = Screen::Paused;
        return;
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
            --medkits;
            healing = true;
            healStarted = now;
            setMessage(L"治疗中，请保持安全");
        } else
            setMessage(L"现在无法治疗");
    }
    if (keyPressed(VK_UP)) shoot(0, -1);
    if (keyPressed(VK_DOWN)) shoot(0, 1);
    if (keyPressed(VK_LEFT)) shoot(-1, 0);
    if (keyPressed(VK_RIGHT)) shoot(1, 0);

    if (now - lastMove >= std::chrono::milliseconds(115)) {
        int dx = 0;
        int dy = 0;
        if (keystate('W'))
            dy = -1;
        else if (keystate('S'))
            dy = 1;
        else if (keystate('A'))
            dx = -1;
        else if (keystate('D'))
            dx = 1;
        if (dx != 0 || dy != 0) {
            movePlayer(dx, dy);
            lastMove = now;
        }
    }

    updateEnemies(now);
    if (healing && now - healStarted >= std::chrono::seconds(3)) {
        healing = false;
        health = std::min(100, health + 20);
        if (networkActive && !networkHost) sendNetworkPacket("HEAL");
        setMessage(L"治療完成  +20 生命");
    }
    if (health <= 0) {
        health = 0;
        won = false;
        screen = Screen::Result;
        return;
    }

    if (extracting && now - extractStarted >= std::chrono::seconds(5)) {
        won = true;
        screen = Screen::Result;
        return;
    }
    if (keyPressed('Q')) {
        if (world[playerY][playerX] != ExitTile)
            setMessage(L"前往地图上的撤离点");
        else if (kills < totalEnemies)
            setMessage(L"区域仍有目标存活");
        else {
            extracting = true;
            extractStarted = now;
            if (networkActive && !networkHost) sendNetworkPacket("EVAC 1");
            setMessage(L"撤离开始，保持位置 5 秒", 5200);
        }
    }
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
} // namespace

int main() {
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
            fillRect(0, 0, LogicalWidth, LogicalHeight, EGEARGB(170, 5, 10, 13));
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