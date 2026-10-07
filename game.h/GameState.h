#pragma once

#include "GameTypes.h"
#include <random>
#include <winsock2.h>

namespace mokthyn {
inline std::array<std::array<int, MapColumns>, MapRows> world{};
inline std::array<std::array<bool, MapColumns>, MapRows> visible{};
inline std::vector<Enemy> enemies;
inline std::mt19937
    randomEngine(static_cast<unsigned int>(Clock::now().time_since_epoch().count()));

inline Screen screen = Screen::Menu;
inline int playerX = MapColumns / 2;
inline int playerY = MapRows / 2;
inline int health = 100;
inline int ammo = 30;
inline int medkits = 2;
inline int money = 200;
inline int armorTier = 0;
inline int armorDurability = 0;
inline bool english = false;
inline bool hasSavedRun = false;
inline bool helpInputReady = false;
inline int helpPage = 0;
inline int kills = 0;
inline int totalEnemies = 0;
inline bool armorOn = false;
inline bool healing = false;
inline bool extracting = false;
inline bool won = false;
inline bool networkRun = false;
inline ArchiveSnapshot networkArchiveSnapshot{};
inline bool peerArmorInitialized = false;
inline bool peerExtractConfirmed = false;
inline bool networkPause = false;
inline Clock::time_point healStarted{};
inline Clock::time_point extractStarted{};
inline Clock::time_point pausedAt{};
inline Clock::time_point lastMove{};
inline Clock::time_point lastEnemyTurn{};
inline Clock::time_point nextPlayerHit{};
inline Clock::time_point messageUntil{};
inline Clock::time_point lastArchiveSave{};
inline std::wstring message;
inline std::wstring archiveStatus;
inline int shopSelection = 0;
inline int uiScalePercent = 120;
inline int windowWidth = DefaultWindowWidth;
inline int windowHeight = DefaultWindowHeight;
inline int settingsSelection = 0;
inline PIMAGE frameBuffer = nullptr;

inline SOCKET networkSocket = INVALID_SOCKET;
inline sockaddr_in peerAddress{};
inline bool networkStarted = false;
inline bool networkActive = false;
inline bool networkHost = false;
inline bool peerConnected = false;
inline int peerX = -1;
inline int peerY = -1;
inline int peerHealth = 100;
inline int peerArmorTier = 0;
inline int peerArmorDurability = 0;
inline bool peerArmorOn = false;
inline std::string hostIpInput;
inline std::wstring networkStatus;
inline Clock::time_point lastNetworkSend{};
inline Clock::time_point lastNetworkReceive{};
inline Clock::time_point lastConnectAttempt{};
inline Clock::time_point lastEvacSend{};
inline Clock::time_point peerExtractStarted{};
inline bool peerExtracting = false;
} // namespace mokthyn
