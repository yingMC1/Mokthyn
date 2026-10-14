#define WIN32_LEAN_AND_MEAN
#include "game.h/GameArchive.h"
#include "game.h/GameArchiveImpl.h"
#include "game.h/GameLogic.h"
#include "game.h/GameLogicImpl.h"
#include "game.h/GameNetwork.h"
#include "game.h/GameNetworkImpl.h"
#include "game.h/GameState.h"
#include "game.h/GameTypes.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <thread>
#include <winsock2.h>

using namespace mokthyn;

static bool check(const char* name, bool passed) {
    std::printf("[%s] %s\n", passed ? "PASS" : "FAIL", name);
    return passed;
}

int main() {
    const wchar_t* appData = L"D:\\Mokthyn\\.dsh-audit\\fix-check-appdata";
    CreateDirectoryW(appData, nullptr);
    const std::wstring saveDirectory = std::wstring(appData) + L"\\mokthyn";
    CreateDirectoryW(saveDirectory.c_str(), nullptr);
    SetEnvironmentVariableW(L"APPDATA", appData);

    std::ostringstream legacy;
    legacy << "MOKTHYN_SAVE 4\n1 1 73 18 4 321 2 40 1 8 9 5 16\n";
    for (int i = 0; i < MapColumns * MapRows; ++i) legacy << "0 ";
    legacy << "\n1\n7 8 45 1 0 0\n110\n1080 630\n";
    const std::wstring savePath = saveDirectory + L"\\save.dat";
    {
        std::ofstream output(savePath, std::ios::binary);
        output << legacy.str();
    }

    english = false;
    uiScalePercent = 120;
    windowWidth = DefaultWindowWidth;
    windowHeight = DefaultWindowHeight;
    bool success = check("loads and migrates a version 4 archive", loadArchive());
    success &= check("restores settings from the version 4 archive",
                     english && uiScalePercent == 110 && windowWidth == 1080 &&
                         windowHeight == 630);
    success &= check("restores version 4 run data",
                     hasSavedRun && health == 73 && medkits == 4 &&
                         playerX == 8 && playerY == 9 && enemies.size() == 1);

    std::ifstream encryptedInput(savePath, std::ios::binary);
    std::string encrypted((std::istreambuf_iterator<char>(encryptedInput)),
                          std::istreambuf_iterator<char>());
    std::string plainText;
    const bool decrypted = unprotectArchive(encrypted, plainText);
    success &= check("migrated archive is encrypted", decrypted);
    if (decrypted) {
        std::istringstream input(plainText);
        std::string token;
        while (input >> token && token != "ENEMIES") {}
        int count = 0;
        input >> count;
        for (int i = 0; i < count; ++i)
            for (int field = 0; field < 6; ++field) input >> token;
        success &= check("version 5 archive has no duplicate trailing settings",
                         !(input >> token));
    }

    success &= check("initializes host networking", initializeNetwork());
    sockaddr_in hostAddress{};
    hostAddress.sin_family = AF_INET;
    hostAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (networkSocket != INVALID_SOCKET &&
        bind(networkSocket, reinterpret_cast<sockaddr*>(&hostAddress), sizeof(hostAddress)) ==
            SOCKET_ERROR) {
        success &= check("binds host test socket", false);
    } else {
        int hostAddressLength = sizeof(hostAddress);
        getsockname(networkSocket, reinterpret_cast<sockaddr*>(&hostAddress), &hostAddressLength);
        SOCKET clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        sockaddr_in clientAddress{};
        clientAddress.sin_family = AF_INET;
        clientAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        bind(clientSocket, reinterpret_cast<sockaddr*>(&clientAddress), sizeof(clientAddress));
        int clientAddressLength = sizeof(clientAddress);
        getsockname(clientSocket, reinterpret_cast<sockaddr*>(&clientAddress), &clientAddressLength);

        networkActive = true;
        networkHost = true;
        peerConnected = true;
        peerAddress = clientAddress;
        screen = Screen::Playing;
        peerX = 5;
        peerY = 5;
        peerHealth = 70;
        peerMedkits = 1;
        peerMedkitsInitialized = true;
        for (auto& row : world) row.fill(Floor);

        const auto sendReliable = [&](unsigned int id, const char* payload) {
            const std::string packet = "REL " + std::to_string(id) + " " + payload;
            sendto(clientSocket, packet.data(), static_cast<int>(packet.size()), 0,
                   reinterpret_cast<const sockaddr*>(&hostAddress), sizeof(hostAddress));
            processNetwork();
        };
        const auto beforeEarlyFinishHealth = peerHealth;
        sendReliable(10, "HEAL_START");
        success &= check("host reserves one medkit for a valid heal request",
                         peerHealing && peerMedkits == 0);
        sendReliable(11, "HEAL_DONE");
        success &= check("host rejects an early heal completion",
                         peerHealth == beforeEarlyFinishHealth && !peerHealing);

        peerMedkits = 1;
        sendReliable(12, "HEAL_START");
        std::this_thread::sleep_for(std::chrono::milliseconds(3100));
        sendReliable(13, "HEAL_DONE");
        success &= check("host applies a heal only after the full treatment duration",
                         peerHealth == beforeEarlyFinishHealth + 20 && peerMedkits == 0);

        sendReliable(14, "HEAL_START");
        success &= check("host rejects treatment without an available medkit",
                         !peerHealing && peerMedkits == 0);
        closesocket(clientSocket);
        closeNetwork();
    }

    return success ? 0 : 1;
}
