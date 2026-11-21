#pragma once

#include "GameState.h"

namespace mokthyn {
struct MenuRect;
inline void fillRect(int left, int top, int right, int bottom, ege::color_t color);
inline void outlineRect(int left, int top, int right, int bottom,
                        ege::color_t color);
inline void fillCircle(int x, int y, int radius, ege::color_t color);
inline void fillEllipse(int x, int y, int radiusX, int radiusY, ege::color_t color);
// 俯视小人（头盔/躯干/双臂/双腿），玩家与联机队友共用
inline void drawHumanoid(int cx, int cy, int tileSize, ege::color_t body,
                         ege::color_t dark);
inline void drawText(int x, int y, const std::wstring &value, int size = 20,
                     ege::color_t color = Text);
// 绘制已本地化文本（跳过翻译查找）：居中排版需要先量宽再画，避免重复本地化
inline void drawTextLocalized(int x, int y, const std::wstring &displayed, int size,
                              ege::color_t color);
inline const std::wstring &localize(const std::wstring &value);
inline void drawBar(int x, int y, int width, int height, int value, int maximum,
                    ege::color_t color);
inline bool mouseDesignPos(int px, int py, int &dx, int &dy);
inline bool pollMouseClick(int &dx, int &dy);
inline void drawMap();
inline void drawHud();
inline void drawMenu();
inline void drawNetworkLobby();
inline void drawShop();
inline void drawHelp();
inline void drawSettings();
inline void drawResult();
inline void drawPlaying();
}    // namespace mokthyn
