#pragma once

#include "GameTypes.h"
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <random>
#include <winsock2.h>

namespace mokthyn {
inline std::array<std::array<int, MapColumns>, MapRows> world{};
inline std::array<std::array<bool, MapColumns>, MapRows> visible{};
inline int visibilityPlayerX = -1;
inline int visibilityPlayerY = -1;
inline unsigned int visibilityRevision = 0;
inline unsigned int computedVisibilityRevision = static_cast<unsigned int>(-1);
inline std::vector<Enemy> enemies;
inline std::mt19937
    randomEngine(static_cast<unsigned int>(Clock::now().time_since_epoch().count()));

inline Screen screen = Screen::Menu;
inline GameMode gameMode = GameMode::Coop;
inline Difficulty difficulty = Difficulty::Easy;
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
inline bool pickupPending = false;
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
inline Clock::time_point nextPeerHit{};
inline Clock::time_point nextPlayerShot{};
inline Clock::time_point peerNextShot{};
inline WeaponMode weaponMode = WeaponMode::Gun;
inline int magazineAmmo = 0;
inline bool reloading = false;
inline Clock::time_point reloadStarted{};
inline Clock::time_point nextMelee{};
inline int peerMagazineAmmo = 0;
inline bool peerReloading = false;
inline Clock::time_point messageUntil{};
inline Clock::time_point lastArchiveSave{};
inline std::wstring message;
inline std::wstring archiveStatus;
inline int shopSelection = 0;
inline int uiScalePercent = DefaultUiScalePercent;
inline int windowWidth = DefaultWindowWidth;
inline int windowHeight = DefaultWindowHeight;
inline int settingsSelection = 0;inline PIMAGE frameBuffer = nullptr;
inline unsigned int frameBufferGeneration = 0;

inline SOCKET networkSocket = INVALID_SOCKET;
inline sockaddr_in peerAddress{};
inline bool networkStarted = false;
inline bool networkActive = false;
inline bool networkHost = false;
inline bool peerConnected = false;
inline int peerX = -1;
inline int peerY = -1;
inline int peerHealth = 100;
inline int peerAmmo = 0;
inline int peerMedkits = 0;
inline bool peerAmmoInitialized = false;
inline bool peerMagazineInitialized = false;
inline Clock::time_point peerReloadStarted{};
inline Clock::time_point peerNextMelee{};
inline int peerArmorTier = 0;
inline int peerArmorDurability = 0;
inline bool peerArmorOn = false;
inline bool peerMedkitsInitialized = false;
inline bool peerHealing = false;
inline Clock::time_point peerHealStarted{};
inline std::string hostIpInput;
inline std::string networkSessionToken;
inline std::wstring networkStatus;
inline Clock::time_point lastNetworkSend{};
inline Clock::time_point lastNetworkReceive{};
inline Clock::time_point lastConnectAttempt{};
inline Clock::time_point lastEvacSend{};
inline Clock::time_point peerExtractStarted{};
inline bool peerExtracting = false;
inline unsigned int nextReliablePacketId = 1;
struct PendingReliablePacket {
    std::string payload;
    Clock::time_point lastSent{};
    unsigned int attempts = 0;
};
inline std::unordered_map<unsigned int, PendingReliablePacket> pendingReliablePackets;
inline std::unordered_set<unsigned int> receivedReliablePacketIds;
inline std::deque<unsigned int> receivedReliablePacketOrder;
inline unsigned int networkMapRevision = 0;
inline unsigned int peerMapRevision = 0;
} // namespace mokthyn
