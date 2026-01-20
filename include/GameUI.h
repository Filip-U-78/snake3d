#pragma once
#ifndef GAME_UI_H
#define GAME_UI_H

#include "Shared.h"
#include "OrbitCamera.h"
#include <functional>

// Funkcja rysuj¹ca Menu G³ówne
// Przyjmuje referencjê do stanu gry i funkcjê "onStart" (co zrobiæ po klikniêciu Start)
void DrawMainMenu(GameState& currentState, std::function<void()> onStart);

// Funkcja rysuj¹ca Ustawienia
// Przyjmuje referencje do zmiennych, które chcemy edytowaæ
void DrawSettings(GameState& currentState, int& speedLevel, int& maxApples, CameraMode& camMode, std::function<void()> onAppleChange);

// Funkcja rysuj¹ca Ekran Koñcowy
void DrawGameOver(GameState& currentState, int score, std::function<void()> onRestart);

// Funkcja rysuj¹ca HUD (punktacjê w trakcie gry)
void DrawHUD(int score);

#endif