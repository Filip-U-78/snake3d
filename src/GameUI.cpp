#include "../include/GameUI.h"
#include "imgui.h"
#include <glfw3.h> // Potrzebne do zamykania okna

void DrawMainMenu(GameState& currentState, std::function<void()> onStart) {
    // Pobieramy rozmiar ekranu, ¿eby wycentrowaæ okno
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar);
    ImGui::TextColored(ImVec4(0, 1, 0, 1), "SNAKE 3D EXTREME");
    ImGui::Separator();

    if (ImGui::Button("START GRY", ImVec2(220, 45))) {
        onStart(); // Wywo³aj funkcjê resetuj¹c¹ grê
        currentState = GameState::Game;
    }

    if (ImGui::Button("USTAWIENIA", ImVec2(220, 45))) {
        currentState = GameState::Settings;
    }

    if (ImGui::Button("WYJDZ", ImVec2(220, 45))) {
        // Tu jest ma³y hack: pobieramy obecny kontekst, ¿eby zamkn¹æ okno. 
        // W idealnym œwiecie przekazalibyœmy wskaŸnik window, ale tak te¿ zadzia³a.
        exit(0);
    }
    ImGui::End();
}

void DrawSettings(GameState& currentState, int& speedLevel, int& maxApples, CameraMode& camMode, std::function<void()> onAppleChange) {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    ImGui::Begin("Ustawienia", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    // Suwak od 1 do 10
    if (ImGui::SliderInt("Predkosc (1-10)", &speedLevel, 1, 10)) {
        // Logika przeliczania na sekundy (moveInterval) zostanie wykonana w main
    }

    if (ImGui::SliderInt("Liczba jablek", &maxApples, 1, 10)) {
        onAppleChange();
    }

    const char* camModes[] = { "Orbit (myszka)", "Pod¹¿aj¹ca (TPP)" };
    int currentSelection = (camMode == CameraMode::TPP) ? 1 : 0;

    if (ImGui::Combo("Tryb kamery", &currentSelection, camModes, IM_ARRAYSIZE(camModes))) {
        camMode = (currentSelection == 0) ? CameraMode::Orbit : CameraMode::TPP;
    }

    if (ImGui::Button("POWROT", ImVec2(120, 30))) {
        currentState = GameState::Menu;
    }
    ImGui::End();
}

void DrawGameOver(GameState& currentState, int score, std::function<void()> onRestart) {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    ImGui::Begin("Game Over", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar);
    ImGui::TextColored(ImVec4(1, 0, 0, 1), "KONIEC GRY!");
    ImGui::Text("Wynik: %d", score);

    if (ImGui::Button("ZAGRAJ PONOWNIE", ImVec2(220, 45))) {
        onRestart();
        currentState = GameState::Game;
    }

    if (ImGui::Button("MENU", ImVec2(220, 45))) {
        currentState = GameState::Menu;
    }
    ImGui::End();
}

void DrawHUD(int score) {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 20, io.DisplaySize.y - 20), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
    ImGui::Begin("ScoreHUD", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("PUNKTY: %d", score);
    ImGui::End();
}