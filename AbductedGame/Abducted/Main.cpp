#define _CRT_SECURE_NO_WARNINGS  // Disable sprintf warning
#include "icb_gui.h"
#include "icbytes.h"
#include "ic_media.h"
#include <thread>   // C++ thread library
#include <cstdlib>  // For abs() function
#include <windows.h>
#include <mmsystem.h>

ICBYTES panel; // Main panel
int FRM1, BTN, GAME_OVER_LABEL;
std::thread gameThread; // Track the game thread
bool stopGameThread = false; // Control flag for stopping the thread

// Game variables
int ufoX = 200, ufoY = 50;
int ufoDir = 1; // 1: right, -1: left
int soldierX = 0, soldierY = -100;
bool soldierActive = false;
bool gameOver = false;
int vehicleX = 200, vehicleY = 500;
int vehicleDir = 1;
int score = 0;
int gameRunning = 0, lastKeyPressed = 0;

// Images
ICBYTES ufoImg, soldierImg, vehicleImg, backgroundImg; ;

const int PANEL_WIDTH = 601;
const int PANEL_HEIGHT = 601;
const int SOLDIER_WIDTH = 32;
const int SOLDIER_HEIGHT = 32;

void RunGameLoop();
void ResetGame();

void RunGameLoop()
{
    ICG_SetWindowText(GAME_OVER_LABEL, "SCORE: 000000");
    while (!stopGameThread) {
        if (gameOver) {
            ICG_SetWindowText(GAME_OVER_LABEL, "GAME OVER - PRESS SPACEBAR TO PLAY AGAIN");

            // Wait until game is reset
            while (gameOver && !stopGameThread) {
                Sleep(30);
            }
            continue;
        }

        // --- Input Handling ---
        int key = ICG_LastKeyPressed();
        if (key == 32 && !soldierActive && !gameOver) {
            soldierX = ufoX + 15;
            soldierY = ufoY + 30;
            soldierActive = true;
        }
        if (soldierActive) {
            if (key == 37) soldierX -= 4; // Left arrow
            if (key == 39) soldierX += 4; // Right arrow
        }

        // Ensure soldier does not move out of bounds
        if (soldierX < 0) soldierX = 1;
        if (soldierX > PANEL_WIDTH - SOLDIER_WIDTH) soldierX = PANEL_WIDTH - SOLDIER_WIDTH;

        // UFO Movement
        ufoX += 5 * ufoDir;
        if (ufoX > 550) ufoDir = -1;
        if (ufoX < 10)  ufoDir = 1;

        // Soldier Movement (falling)
        if (soldierActive) {
            soldierY += 3;
            if (soldierY > PANEL_HEIGHT - SOLDIER_HEIGHT) {
                gameOver = true;
                ICG_SetWindowText(GAME_OVER_LABEL, "GAME OVER - PRESS SPACEBAR TO PLAY AGAIN");
                soldierActive = false;
            }
        }

        // Vehicle Movement
        vehicleX += 3 * vehicleDir;
        if (vehicleX > 550) vehicleDir = -1;
        if (vehicleX < 10)  vehicleDir = 1;

        // Collision Detection
        if (soldierActive && abs(soldierX - vehicleX) < 30 &&
            abs(soldierY - vehicleY) < 30) {
            score += 20;
            soldierActive = false;
            // --- UPDATE SCORE ---
            char buffer[100];
            sprintf_s(buffer, sizeof(buffer), "SCORE: %06d", score); //  Ensures 6-digit format (000020, 000040, etc.)
            ICG_SetWindowText(GAME_OVER_LABEL, buffer);
        }

        // --- SCREEN UPDATE (DRAWING) ---
        FillRect(panel, 10, 50, PANEL_WIDTH-10, PANEL_HEIGHT-50, 0x000000); // Clear screen before drawing
        Paste(backgroundImg, 1, 1, panel);
             
        Paste(ufoImg, ufoX, ufoY, panel);   //  Draw objects on top of background
        if (soldierActive) Paste(soldierImg, soldierX, soldierY, panel);
        Paste(vehicleImg, vehicleX, vehicleY, panel);

        DisplayImage(FRM1, panel);
        UpdateWindow(ICG_GetMainWindow()); // Ensure the window refreshes
        Sleep(10); //  Prevents CPU overuse
    }
}



void ResetGame()
{
    // Stop the previous game thread before restarting
    stopGameThread = true;
    if (gameThread.joinable()) {
        gameThread.join();
    }
    
    // Reset game variables
    stopGameThread = false;
    gameOver = false;
    score = 0;
    soldierX = 0;
    soldierY = -100;
    vehicleX = 200;
    vehicleY = 500;
    ufoX = 200;
    soldierActive = false;
    
    ICG_SetWindowText(GAME_OVER_LABEL, ""); // Clear "GAME OVER" text

    // Restart the game thread
    gameThread = std::thread(RunGameLoop);
}


void HandleKeyPress(int k)
{
    lastKeyPressed = k;
    if (gameOver && k == 32) { // Spacebar pressed during game over
        gameOver = false; // Allow the game loop to continue
        ResetGame();
    }
}

void OnExit()
{
    stopGameThread = true; // Stop the game loop safely
    if (gameThread.joinable()) {
        gameThread.join(); // Wait for the thread to stop
    }
}

void ICGUI_Create()
{
    ICG_MWTitle("Abducted Game");
    ICG_MWSize(700, 700);
    
    ICG_SetFont(15, 10, "Arial");
    // Ensure game exits properly when the window is closed
    atexit(OnExit);
}


void ICGUI_main()
{
    PlaySound(TEXT("abducted.wav"), NULL, SND_FILENAME | SND_ASYNC);
    CreateImage(panel, PANEL_WIDTH, PANEL_HEIGHT, ICB_UINT);
    FRM1 = ICG_FrameMedium(15, 15, PANEL_WIDTH, PANEL_HEIGHT);

    ReadImage("background72.bmp", backgroundImg); // Load background
    ReadImage("ufo.bmp", ufoImg);
    ReadImage("soldier.bmp", soldierImg);
    ReadImage("vehicle.bmp", vehicleImg);

    ICDEVICE d;
    ICBYTES i;

    GAME_OVER_LABEL = ICG_Static(30, 0, 500, 18, "");

    ICG_SetOnKeyPressed(HandleKeyPress);

    // Start the initial game loop thread
    gameThread = std::thread(RunGameLoop);
}