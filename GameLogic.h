#pragma once

#include "GameState.h"

namespace mokthyn {
int scaled(int value);
int randomInt(int low, int high);
void setMessage(const std::wstring& text, int milliseconds = 1800);
bool lineOfSight(int fromX, int fromY, int toX, int toY);
void computeVisibility();
bool hasEnemyAt(int x, int y);
void placePickup(int type, int minimumDistance);
void startRun();
void shoot(int dx, int dy);
int takeDamage(int damage);
void updateEnemies(Clock::time_point now);
void collectPickup();
void movePlayer(int dx, int dy);
void buySelectedItem();
bool keyPressed(int key);
void updatePlaying();
} // namespace mokthyn
