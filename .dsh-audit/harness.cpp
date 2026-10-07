// Pre-release audit harness for Mokthyn.
// Includes the game's real headers and drives the real functions.
// Never touches the player's save file: APPDATA is redirected to a scratch dir.
// No EGE drawing function is called, so no window is created.

#define WIN32_LEAN_AND_MEAN
#include "game.h/GameArchive.h"
#include "game.h/GameArchiveImpl.h"
#include "game.h/GameLogic.h"
#include "game.h/GameLogicImpl.h"
#include "game.h/GameNetwork.h"
#include "game.h/GameNetworkImpl.h"
#include "game.h/GameState.h"
#include "game.h/GameTypes.h"

#include <shellapi.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace mokthyn;

static int g_pass = 0;
static int g_fail = 0;

static void check(const char* label, bool ok) {
    std::printf("  [%s] %s\n", ok ? "PASS" : "FAIL", label);
    if (ok) ++g_pass; else ++g_fail;
}

static const wchar_t* kScratch = L"D:\\Mokthyn\\.dsh-audit\\appdata";
static const char* kSavePath = "D:\\Mokthyn\\.dsh-audit\\appdata\\mokthyn\\save.dat";

static std::string readSaveFile() {
    std::ifstream in(kSavePath, std::ios::binary);
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

// Reads the two fields that decide whether a solo run survives: the run flag and health.
static bool parseSaveHeader(int& savedRun, int& healthOut, int& enemyCountOut) {
    std::istringstream in(readSaveFile());
    std::string signature;
    int version = 0;
    int english_ = 0;
    int storedHealth = 0, ammo = 0, medkits = 0, money = 0, tier = 0, durability = 0, armorOn = 0;
    int px = 0, py = 0, kills = 0, total = 0;
    if (!(in >> signature >> version >> english_ >> savedRun >> storedHealth >> ammo >> medkits >>
          money >> tier >> durability >> armorOn >> px >> py >> kills >> total))
        return false;
    healthOut = storedHealth;
    for (int i = 0; i < MapColumns * MapRows; ++i) {
        int tile = 0;
        in >> tile;
    }
    in >> enemyCountOut;
    return true;
}

static void resetState() {
    for (auto& row : world) row.fill(Floor);
    for (auto& row : visible) row.fill(false);
    enemies.clear();
    screen = Screen::Menu;
    playerX = MapColumns / 2;
    playerY = MapRows / 2;
    health = 100; ammo = 30; medkits = 2; money = 200;
    armorTier = 0; armorDurability = 0; armorOn = false;
    english = false; hasSavedRun = false; kills = 0; totalEnemies = 0;
    healing = false; extracting = false; won = false;
    uiScalePercent = 120; windowWidth = DefaultWindowWidth; windowHeight = DefaultWindowHeight;
    networkActive = false; networkHost = false; peerConnected = false;
    peerX = peerY = -1; peerHealth = 100;
    peerArmorTier = 0; peerArmorDurability = 0; peerArmorOn = false; peerExtracting = false;
    healStarted = Clock::time_point{};
    extractStarted = Clock::time_point{};
    archiveStatus.clear();
    message.clear();
}

static SOCKET udpBindLoopback(unsigned short port, unsigned short* actualPort) {
    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) return s;
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(port);
    if (bind(s, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR) {
        closesocket(s);
        return INVALID_SOCKET;
    }
    if (actualPort != nullptr) {
        sockaddr_in local{};
        int length = sizeof(local);
        if (getsockname(s, reinterpret_cast<sockaddr*>(&local), &length) == 0)
            *actualPort = ntohs(local.sin_port);
    }
    u_long nonBlocking = 1;
    ioctlsocket(s, FIONBIO, &nonBlocking);
    return s;
}

static bool sendLoopback(SOCKET s, const std::string& packet, unsigned short port) {
    sockaddr_in target{};
    target.sin_family = AF_INET;
    target.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    target.sin_port = htons(port);
    return sendto(s, packet.data(), static_cast<int>(packet.size()), 0,
                  reinterpret_cast<sockaddr*>(&target), sizeof(target)) != SOCKET_ERROR;
}

static int drain(SOCKET s, std::vector<std::string>* out) {
    char buffer[8192];
    int count = 0;
    for (;;) {
        sockaddr_in from{};
        int fromLength = sizeof(from);
        const int received = recvfrom(s, buffer, sizeof(buffer) - 1, 0,
                                      reinterpret_cast<sockaddr*>(&from), &fromLength);
        if (received == SOCKET_ERROR) break;
        buffer[received] = '\0';
        if (out != nullptr) out->push_back(std::string(buffer, static_cast<size_t>(received)));
        ++count;
    }
    return count;
}

// ege.h redefines Sleep() as delay_ms(), which needs an initialized graphics window,
// so wait with a plain spin here instead of calling Sleep.
static void nap(int milliseconds) {
    const Clock::time_point until = Clock::now() + std::chrono::milliseconds(milliseconds);
    while (Clock::now() < until) {
    }
}

// Builds the exact STATE packet the host produces (GameNetworkImpl.h sendNetworkState).
static std::string buildState(int hostX, int hostY, int hostHp, int clientX, int clientY,
                              int clientHp, int kills_, int total, int money_, int hostEvacuating,
                              int peerEvacuating, int finished, int result, int enemyCount) {
    std::ostringstream packet;
    packet << "STATE " << hostX << ' ' << hostY << ' ' << hostHp << ' ' << clientX << ' ' << clientY
           << ' ' << clientHp << ' ' << 0 << ' ' << 0 << ' ' << 0 << ' ' << kills_ << ' ' << total
           << ' ' << money_ << ' ' << hostEvacuating << ' ' << peerEvacuating << ' ' << finished
           << ' ' << result << ' ' << enemyCount << ' ';
    for (int i = 0; i < enemyCount; ++i) packet << (1 + i % 20) << ' ' << (1 + i % 15) << " 45 1 1 ";
    packet << "MAP ";
    for (int i = 0; i < MapColumns * MapRows; ++i) packet << "0 ";
    return packet.str();
}

int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    CreateDirectoryW(L"D:\\Mokthyn\\.dsh-audit", nullptr);
    CreateDirectoryW(kScratch, nullptr);
    SetEnvironmentVariableW(L"APPDATA", kScratch);
    std::printf("APPDATA redirected to scratch; the player's real save is untouched.\n");

    // ---------------------------------------------------------------- T1
    std::printf("\n=== T1  archive round-trip (baseline) ===\n");
    resetState();
    english = true;
    uiScalePercent = 110;
    windowWidth = 1080;
    windowHeight = 630;
    hasSavedRun = true;
    health = 77; ammo = 12; medkits = 3; money = 250;
    armorTier = 2; armorDurability = 40; armorOn = true;
    playerX = 5; playerY = 7; kills = 4; totalEnemies = 9;
    world[3][3] = Wall; world[4][4] = AmmoPickup; world[5][5] = MedkitPickup; world[6][6] = ExitTile;
    enemies.push_back({2, 2, 45, true, true, Clock::now()});
    enemies.push_back({8, 9, 20, false, true, Clock::now()});
    check("saveArchive() succeeds", saveArchive());
    resetState();
    check("loadArchive() succeeds", loadArchive());
    check("english restored", english == true);
    check("uiScalePercent restored", uiScalePercent == 110);
    check("window size restored", windowWidth == 1080 && windowHeight == 630);
    check("hasSavedRun restored", hasSavedRun == true);
    check("player stats restored", health == 77 && ammo == 12 && medkits == 3 && money == 250);
    check("armor restored", armorTier == 2 && armorDurability == 40 && armorOn);
    check("position restored", playerX == 5 && playerY == 7);
    check("map restored", world[3][3] == Wall && world[4][4] == AmmoPickup &&
                              world[5][5] == MedkitPickup && world[6][6] == ExitTile);
    check("enemies restored", enemies.size() == 2 && enemies[1].hp == 20 && !enemies[1].alive);

    // ---------------------------------------------------------------- T2
    std::printf("\n=== T2  save writer accepts more enemies than the loader allows ===\n");
    resetState();
    english = true;
    windowWidth = 1080;
    hasSavedRun = true;
    for (int i = 0; i < 30; ++i)
        enemies.push_back({1 + i % 20, 1 + i % 15, 45, true, false, Clock::now()});
    check("saveArchive() writes a file with 30 enemies", saveArchive());
    int savedRun = -1, savedHealth = -1, savedEnemies = -1;
    check("save file parses", parseSaveHeader(savedRun, savedHealth, savedEnemies));
    check("file records enemyCount = 30 (loader allows at most 9)", savedEnemies == 30);
    english = false;
    windowWidth = DefaultWindowWidth;
    hasSavedRun = false;
    const bool reloaded = loadArchive();
    check("loadArchive() accepts a file saveArchive() itself just wrote", reloaded == true);
    check("saved settings and run are applied on the next launch",
          english == true && windowWidth == 1080 && hasSavedRun == true);

    // ---------------------------------------------------------------- T3
    std::printf("\n=== T3  co-op handshake: one lost ACK traps the client forever ===\n");
    closeNetwork();
    resetState();
    const unsigned short hostPort = 8888;
    check("initializeNetwork()", initializeNetwork());
    sockaddr_in hostBind{};
    hostBind.sin_family = AF_INET;
    hostBind.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    hostBind.sin_port = htons(hostPort);
    check("host binds 127.0.0.1:8888",
          bind(networkSocket, reinterpret_cast<sockaddr*>(&hostBind), sizeof(hostBind)) != SOCKET_ERROR);
    networkHost = true;
    networkActive = true;
    peerConnected = false;
    screen = Screen::HostLobby;

    unsigned short clientPort = 0;
    SOCKET clientSocket = udpBindLoopback(0, &clientPort);
    check("client socket created", clientSocket != INVALID_SOCKET);

    sendLoopback(clientSocket, "CONNECT", hostPort);
    std::vector<std::string> packets;
    int firstAcks = 0;
    for (int i = 0; i < 200 && firstAcks == 0; ++i) {
        processNetwork();
        nap(5);
        packets.clear();
        drain(clientSocket, &packets);
        for (const std::string& packet : packets)
            if (packet == "ACK") ++firstAcks;
    }
    check("host answers the first CONNECT with an ACK", firstAcks == 1);
    check("host marks the peer connected", peerConnected);
    // The ACK we just pulled off the socket is dropped: exactly what UDP loss looks like.

    int retryAcks = 0;
    for (int attempt = 0; attempt < 4; ++attempt) {
        sendLoopback(clientSocket, "CONNECT", hostPort);
        for (int i = 0; i < 20; ++i) {
            processNetwork();
            nap(5);
        }
        packets.clear();
        drain(clientSocket, &packets);
        for (const std::string& packet : packets)
            if (packet == "ACK") ++retryAcks;
    }
    check("host re-ACKs a repeated CONNECT so the client can recover", retryAcks > 0);
    std::printf("      -> ACKs answering 4 repeated CONNECTs: %d\n", retryAcks);

    // ---------------------------------------------------------------- T6
    std::printf("\n=== T6  peer armor durability is refreshed by every POS packet ===\n");
    peerArmorTier = 0;
    peerArmorDurability = 0;
    peerArmorOn = false;
    sendLoopback(clientSocket, "POS 3 3 2 75 1", hostPort);
    for (int i = 0; i < 20; ++i) { processNetwork(); nap(5); }
    check("host stores armor tier/durability from POS",
          peerArmorTier == 2 && peerArmorDurability == 75 && peerArmorOn);
    peerArmorDurability = 71;  // what updateEnemies() writes after one hit (GameLogicImpl.h:229)
    sendLoopback(clientSocket, "POS 3 3 2 75 1", hostPort);
    for (int i = 0; i < 20; ++i) { processNetwork(); nap(5); }
    check("consumed durability is NOT restored by the next POS", peerArmorDurability == 71);

    // ---------------------------------------------------------------- T4
    std::printf("\n=== T4  co-op start: one lost START leaves the client in the lobby ===\n");
    closeNetwork();
    resetState();
    check("initializeNetwork()", initializeNetwork());
    sockaddr_in clientBind{};
    clientBind.sin_family = AF_INET;
    clientBind.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    clientBind.sin_port = 0;
    check("client binds", bind(networkSocket, reinterpret_cast<sockaddr*>(&clientBind),
                               sizeof(clientBind)) != SOCKET_ERROR);
    unsigned short myPort = 0;
    {
        sockaddr_in local{};
        int length = sizeof(local);
        getsockname(networkSocket, reinterpret_cast<sockaddr*>(&local), &length);
        myPort = ntohs(local.sin_port);
    }
    networkHost = false;
    networkActive = true;
    peerConnected = false;
    peerAddress = {};
    peerAddress.sin_family = AF_INET;
    peerAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    peerAddress.sin_port = htons(hostPort);

    SOCKET fakeHost = udpBindLoopback(hostPort, nullptr);
    check("host-side test socket created", fakeHost != INVALID_SOCKET);

    sendLoopback(fakeHost, "ACK", myPort);
    for (int i = 0; i < 200 && screen != Screen::JoinLobby; ++i) { processNetwork(); nap(5); }
    check("client reaches JoinLobby after ACK", screen == Screen::JoinLobby);

    lastNetworkReceive = Clock::now();
    sendLoopback(fakeHost, buildState(6, 6, 100, 7, 7, 100, 0, 9, 200, 0, 0, 0, 0, 9), myPort);
    for (int i = 0; i < 40; ++i) { processNetwork(); nap(5); }
    check("client stays in the lobby when only STATE arrives (START was lost)",
          screen == Screen::JoinLobby);
    check("STATE was applied anyway, so the client is half-synced", totalEnemies == 9);

    sendLoopback(fakeHost, "START", myPort);
    for (int i = 0; i < 200 && screen != Screen::Playing; ++i) { processNetwork(); nap(5); }
    check("client reaches Playing when START does arrive", screen == Screen::Playing);

    // ---------------------------------------------------------------- T5
    std::printf("\n=== T5  host's extraction flag hijacks the client's own extraction ===\n");
    extracting = false;
    extractStarted = Clock::time_point{};
    sendLoopback(fakeHost, buildState(6, 6, 100, 7, 7, 100, 0, 9, 200, 1, 0, 0, 0, 9), myPort);
    for (int i = 0; i < 40; ++i) { processNetwork(); nap(5); }
    check("client's own extracting state is untouched when only the host extracts",
          extracting == false);
    const long long elapsed = static_cast<long long>(std::chrono::duration_cast<std::chrono::seconds>(
                                  Clock::now() - extractStarted).count());
    check("client's extraction timer is not already expired when extracting turns on",
          !(extracting && elapsed >= 5));
    std::printf("      -> elapsed since client extractStarted: %lld s (the 5s check fires at once)\n",
                elapsed);
    closeNetwork();

    // ---------------------------------------------------------------- T7
    std::printf("\n=== T7  starting a co-op run destroys the solo save ===\n");
    resetState();
    english = true;
    windowWidth = 1080;
    health = 55;
    ammo = 44;
    hasSavedRun = true;
    check("solo run saved", saveArchive());
    int beforeRun = -1, beforeHealth = -1, ignored = -1;
    check("save file parses", parseSaveHeader(beforeRun, beforeHealth, ignored));
    check("solo save is marked resumable", beforeRun == 1 && beforeHealth == 55);
    networkActive = true;
    networkHost = true;
    peerConnected = false;
    startRun();
    check("startRun() keeps hasSavedRun in a network run", hasSavedRun == true);
    int afterRun = -1, afterHealth = -1, afterEnemies = -1;
    check("save file parses after the co-op run started",
          parseSaveHeader(afterRun, afterHealth, afterEnemies));
    check("solo run flag survives the co-op run", afterRun == 1);
    check("solo run stats survive the co-op run", afterHealth == 55);
    std::printf("      -> savedRun flag after startRun(): %d, health in file: %d\n", afterRun,
                afterHealth);

    // ---------------------------------------------------------------- T8
    std::printf("\n=== T8  extraction/healing state survives leaving the run ===\n");
    closeNetwork();
    resetState();
    screen = Screen::Playing;
    extracting = true;
    extractStarted = Clock::now() - std::chrono::seconds(6);
    healing = true;
    healStarted = Clock::now() - std::chrono::seconds(6);
    saveArchive();            // Mokthyn.cpp:208  (Paused -> ESC)
    screen = Screen::Menu;    // Mokthyn.cpp:209
    check("leaving the run clears the extraction state", extracting == false);
    check("leaving the run clears the healing state", healing == false);
    screen = Screen::Playing;                                 // Mokthyn.cpp:44 (menu -> '6')
    lastMove = lastEnemyTurn = nextPlayerHit = Clock::now();  // Mokthyn.cpp:45
    check("resuming from the menu does not win instantly",
          !(extracting && Clock::now() - extractStarted >= std::chrono::seconds(5)));
    check("resuming from the menu does not heal instantly",
          !(healing && Clock::now() - healStarted >= std::chrono::seconds(3)));

    // ---------------------------------------------------------------- T9
    std::printf("\n=== T9  malformed UDP traffic must not crash or corrupt ===\n");
    if (clientSocket != INVALID_SOCKET) closesocket(clientSocket);
    if (fakeHost != INVALID_SOCKET) closesocket(fakeHost);
    std::vector<std::string> evil;
    evil.push_back("");
    evil.push_back("X");
    evil.push_back("STATE");
    evil.push_back("STATE 1 2 3");
    evil.push_back("STATE 999999999 999999999 -5 -5 -5 -5 -5 -5 -5 -5 -5 -5 -5 -5 -5 -5 -1 MAP ");
    evil.push_back("STATE 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 30 MAP");
    evil.push_back("STATE 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 -5 MAP " + std::string(432, '0'));
    evil.push_back("STATE 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 2147483647 MAP " + std::string(432, '0'));
    evil.push_back("POS -1 -1 -1 -1 -1");
    evil.push_back("POS 999 999 9 999 9");
    evil.push_back("POS 23 17 4 999 1");
    evil.push_back("HIT -1 -1 -999");
    evil.push_back("HIT 999999 999999 999999");
    evil.push_back("HIT 1 1 2147483647");
    evil.push_back("PICKUP -5 -5 99");
    evil.push_back("PICKUP 1 1 2");
    evil.push_back("EVAC 2147483647");
    evil.push_back("EVAC -1");
    evil.push_back("LEAVE");
    evil.push_back("CONNECT");
    evil.push_back("START");
    evil.push_back("ACK");
    evil.push_back(std::string(8000, 'A'));
    evil.push_back(std::string(9000, 'B'));
    evil.push_back(std::string("POS 1 1 1 1 1\0GARBAGE", 20));

    closeNetwork();
    resetState();
    check("initializeNetwork() for the host fuzz", initializeNetwork());
    sockaddr_in hostBind2{};
    hostBind2.sin_family = AF_INET;
    hostBind2.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    hostBind2.sin_port = htons(hostPort);
    bind(networkSocket, reinterpret_cast<sockaddr*>(&hostBind2), sizeof(hostBind2));
    networkHost = true;
    networkActive = true;
    peerConnected = false;
    screen = Screen::HostLobby;
    SOCKET fuzzClient = udpBindLoopback(0, nullptr);
    sendLoopback(fuzzClient, "CONNECT", hostPort);
    for (int i = 0; i < 300 && !peerConnected; ++i) { processNetwork(); nap(3); }
    check("host connected for the fuzz run", peerConnected);
    for (const std::string& packet : evil) {
        sendLoopback(fuzzClient, packet, hostPort);
        for (int i = 0; i < 5; ++i) { processNetwork(); nap(2); }
    }
    check("host survived all malformed packets", true);
    peerConnected = false;
    sendLoopback(fuzzClient, "CONNECT", hostPort);
    for (int i = 0; i < 300 && !peerConnected; ++i) { processNetwork(); nap(3); }
    drain(fuzzClient, nullptr);
    sendLoopback(fuzzClient, "POS 4 4 3 100 1", hostPort);
    for (int i = 0; i < 40; ++i) { processNetwork(); nap(3); }
    check("host still applies a valid POS after the fuzz",
          peerX == 4 && peerY == 4 && peerArmorTier == 3);
    if (fuzzClient != INVALID_SOCKET) closesocket(fuzzClient);

    closeNetwork();
    resetState();
    check("initializeNetwork() for the client fuzz", initializeNetwork());
    sockaddr_in clientBind2{};
    clientBind2.sin_family = AF_INET;
    clientBind2.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    clientBind2.sin_port = 0;
    bind(networkSocket, reinterpret_cast<sockaddr*>(&clientBind2), sizeof(clientBind2));
    unsigned short myPort2 = 0;
    {
        sockaddr_in local{};
        int length = sizeof(local);
        getsockname(networkSocket, reinterpret_cast<sockaddr*>(&local), &length);
        myPort2 = ntohs(local.sin_port);
    }
    networkHost = false;
    networkActive = true;
    peerConnected = false;
    peerAddress = {};
    peerAddress.sin_family = AF_INET;
    peerAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    peerAddress.sin_port = htons(hostPort);
    SOCKET fakeHost2 = udpBindLoopback(hostPort, nullptr);
    sendLoopback(fakeHost2, "ACK", myPort2);
    for (int i = 0; i < 300 && !peerConnected; ++i) { processNetwork(); nap(3); }
    check("client connected for the fuzz run", peerConnected);
    for (const std::string& packet : evil) {
        sendLoopback(fakeHost2, packet, myPort2);
        for (int i = 0; i < 5; ++i) { processNetwork(); nap(2); }
    }
    check("client survived all malformed packets", true);

    std::string badMap = "STATE 1 1 100 2 2 100 0 0 0 0 9 200 0 0 0 0 0 MAP ";
    for (int i = 0; i < MapColumns * MapRows; ++i) badMap += (i % 3 == 0) ? "99 " : "-7 ";
    sendLoopback(fakeHost2, badMap, myPort2);
    for (int i = 0; i < 40; ++i) { processNetwork(); nap(3); }
    bool sanitized = true;
    for (const auto& row : world)
        for (int tile : row)
            if (tile < Floor || tile > MedkitPickup) sanitized = false;
    check("out-of-range map tiles from a packet are sanitised to Floor", sanitized);

    std::string oddEnemy = "STATE 1 1 100 2 2 100 0 0 0 0 9 200 0 0 0 0 1 0 0 45 1 1 MAP ";
    for (int i = 0; i < MapColumns * MapRows; ++i) oddEnemy += "0 ";
    sendLoopback(fakeHost2, oddEnemy, myPort2);
    for (int i = 0; i < 40; ++i) { processNetwork(); nap(3); }
    check("client does not accept an enemy sitting on the map border",
          enemies.empty() || enemies[0].x != 0);
    if (!enemies.empty()) {
        saveArchive();
        const bool ok = loadArchive();
        check("a save written from synced network state loads back", ok);
    }
    if (fakeHost2 != INVALID_SOCKET) closesocket(fakeHost2);
    closeNetwork();

    std::printf("\n================ SUMMARY ================\n");
    std::printf("checks passed: %d, failed: %d\n", g_pass, g_fail);
    return 0;
}
