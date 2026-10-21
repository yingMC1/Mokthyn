#pragma once

#include "GameState.h"

namespace mokthyn {
inline int randomInt(int low, int high);
inline void setMessage(const std::wstring& text, int milliseconds = 1800);
inline void resetRunProgress();
inline void cancelExtraction();
inline bool lineOfSight(int fromX, int fromY, int toX, int toY);
inline void computeVisibility();
inline bool hasEnemyAt(int x, int y);
inline bool placePickup(int type, int minimumDistance);
inline void startRun();
inline void shoot(int dx, int dy);
inline int takeDamage(int damage);
inline void updateEnemies(Clock::time_point now);
inline void collectPickup();
inline void movePlayer(int dx, int dy);
inline void buySelectedItem();
inline bool keyPressed(int key);
inline void updatePlaying();
} // namespace mokthyn
