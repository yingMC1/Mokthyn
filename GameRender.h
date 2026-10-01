#pragma once

#include "GameState.h"

namespace mokthyn {
void fillRect(int left, int top, int right, int bottom, ege::color_t color);
void outlineRect(int left, int top, int right, int bottom, ege::color_t color);
void fillCircle(int x, int y, int radius, ege::color_t color);
void drawText(int x, int y, const std::wstring& value, int size = 20, ege::color_t color = Text);
void drawBar(int x, int y, int width, int height, int value, int maximum, ege::color_t color);
void drawMap();
void drawHud();
void drawMenu();
void drawNetworkLobby();
void drawShop();
void drawHelp();
void drawResult();
void drawPlaying();
} // namespace mokthyn
