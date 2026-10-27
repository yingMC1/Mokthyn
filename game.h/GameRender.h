#pragma once

#include "GameState.h"

namespace mokthyn {
struct MenuRect;
inline void fillRect(int left, int top, int right, int bottom, ege::color_t color);
inline void outlineRect(int left, int top, int right, int bottom, ege::color_t color);
inline void fillCircle(int x, int y, int radius, ege::color_t color);
inline void drawText(int x, int y, const std::wstring& value, int size = 20,
                     ege::color_t color = Text);
inline void drawBar(int x, int y, int width, int height, int value, int maximum,
                    ege::color_t color);
inline bool mouseDesignPos(int px, int py, int& dx, int& dy);
inline bool pollMouseClick(int& dx, int& dy);
inline void drawMap();
inline void drawHud();
inline void drawMenu();
inline void drawNetworkLobby();
inline void drawShop();
inline void drawHelp();
inline void drawSettings();
inline void drawResult();
inline void drawPlaying();
} // namespace mokthyn
