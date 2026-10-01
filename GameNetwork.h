#pragma once

#include "GameState.h"

namespace mokthyn {
void closeNetwork();
bool initializeNetwork();
bool beginHostNetwork();
bool beginJoinNetwork();
bool sendNetworkPacket(const std::string& packet);
std::wstring getLocalIPv4();
void sendNetworkState();
void processNetwork();
} // namespace mokthyn
