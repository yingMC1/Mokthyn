#pragma once

#include "GameState.h"

namespace mokthyn {
inline void closeNetwork();
inline bool initializeNetwork();
inline bool beginHostNetwork();
inline bool beginJoinNetwork();
inline bool sendNetworkPacket(const std::string& packet);
inline std::wstring getLocalIPv4();
inline void sendNetworkState();
inline void processNetwork();
} // namespace mokthyn
