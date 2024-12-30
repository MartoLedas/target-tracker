#include <windows.h>
#include <stdlib.h>
#include <time.h>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <string>
#include "gamewin.h"

INT_PTR CALLBACK MenuProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

#define TIMER_ID 1
#define GAME_TIME 20 // Adjust time limit in seconds
#define CIRCLE_RADIUS 22 // Adjust circle size

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int circleX, circleY;
int circleVelocityX, circleVelocityY;
int score = 0;
int remainingTime = GAME_TIME;
bool isCursorInside = false;
int circleRadius = CIRCLE_RADIUS;
int circleLifetime = 0;
const int circleMaxLifetime = 12000;
bool isCircleActivated = false;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    srand((unsigned int)time(NULL)); // Seed RNG

    DialogBox(hInstance, MAKEINTRESOURCE(IDD_MENU), NULL, MenuProc);

    const char CLASS_NAME[] = "AimTrainer";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, "Aim Trainer Game", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, hInstance, NULL);

    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);

    SetTimer(hwnd, TIMER_ID, 1000 / 60, NULL); // 60 FPS

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

void MoveCircle(HWND hwnd) {
    circleX += circleVelocityX;
    circleY += circleVelocityY;

    RECT rect;
    GetClientRect(hwnd, &rect);

    // Reverse direction if the circle hits the window boundaries
    if (circleX - circleRadius < 0 || circleX + circleRadius > rect.right) {
        circleVelocityX = -circleVelocityX;
    }
    if (circleY - circleRadius < 0 || circleY + circleRadius > rect.bottom) {
        circleVelocityY = -circleVelocityY;
    }

    InvalidateRect(hwnd, NULL, TRUE);
}


bool IsCursorInCircle(int mouseX, int mouseY) {
    int dx = mouseX - circleX;
    int dy = mouseY - circleY;
    return (dx * dx + dy * dy) <= (circleRadius * circleRadius);
}

void SaveHighScore(int currentScore) {
    const char* filename = "highscore.txt";
    int highScore = 0;

    std::ifstream inputFile(filename);
    if (inputFile.is_open()) {
        inputFile >> highScore;
        inputFile.close();
    }

    if (currentScore > highScore) {
        std::ofstream outputFile(filename);
        if (outputFile.is_open()) {
            outputFile << currentScore;
            outputFile.close();
        }
    }
}

int GetHighScore() {
    const char* filename = "highscore.txt";
    int highScore = 0;

    std::ifstream inputFile(filename);
    if (inputFile.is_open()) {
        inputFile >> highScore;
        inputFile.close();
    }

    return highScore;
}

INT_PTR CALLBACK MenuProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG:
            {
                HBITMAP hBitmap = (HBITMAP)LoadImage(GetModuleHandle(NULL), TEXT("bowl.bmp"), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
                if (hBitmap) {
                    HWND hwndStatic = GetDlgItem(hDlg, IDC_GAME_LOGO);
                    SendMessage(hwndStatic, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBitmap);
                } else {
                    MessageBox(hDlg, "Failed to load logo bitmap.", "Error", MB_OK | MB_ICONERROR);
                }
            }
            return (INT_PTR)TRUE;

        case WM_CTLCOLORDLG: {
            HDC hdc = (HDC)wParam;
            SetBkColor(hdc, RGB(255, 255, 255));
            SetTextColor(hdc, RGB(0, 0, 0));
            HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));
            return (INT_PTR)hBrush;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_PLAY_BUTTON) {
                EndDialog(hDlg, 0);
                return (INT_PTR)TRUE;
            }
            break;

        case WM_CLOSE:
            EndDialog(hDlg, 0);
            return (INT_PTR)TRUE;
    }
    return (INT_PTR)FALSE;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            srand(time(NULL)); // Seed the random number generator for random positions

            // Set the initial position of the circle randomly
            RECT rect;
            GetClientRect(hwnd, &rect);
            circleX = rand() % (rect.right - 2 * circleRadius) + circleRadius;
            circleY = rand() % (rect.bottom - 2 * circleRadius) + circleRadius;

            circleVelocityX = 5; // Speed in x direction
            circleVelocityY = 5; // Speed in y direction

            SetTimer(hwnd, TIMER_ID, 50, NULL); // Refresh frame every 50 ms
            SetTimer(hwnd, 2, 1000, NULL);
            return 0;


        case WM_TIMER:
            if (wParam == TIMER_ID) {
                POINT cursorPos;
                GetCursorPos(&cursorPos);
                ScreenToClient(hwnd, &cursorPos);

                isCursorInside = IsCursorInCircle(cursorPos.x, cursorPos.y);

                if (isCursorInside && !isCircleActivated) {
                    isCircleActivated = true;
                }

                if (isCircleActivated) {
                    MoveCircle(hwnd);
                }

                if (isCursorInside) {
                    score++;
                }

                circleLifetime += 50;
                if (circleLifetime >= circleMaxLifetime) {
                    RECT rect;
                    GetClientRect(hwnd, &rect);
                    circleX = rand() % (rect.right - 2 * circleRadius) + circleRadius;
                    circleY = rand() % (rect.bottom - 2 * circleRadius) + circleRadius;

                    circleLifetime = 0;
                    isCircleActivated = false;
                }

                InvalidateRect(hwnd, NULL, TRUE);
            } else if (wParam == 2) {
                remainingTime--;

                if (remainingTime <= 0) {
                    KillTimer(hwnd, TIMER_ID);
                    KillTimer(hwnd, 2);

                    SaveHighScore(score);

                    char buffer[100];
                    sprintf(buffer, "Time's up! Your score: %d", score);
                    MessageBox(hwnd, buffer, "Game Over", MB_OK);
                    PostQuitMessage(0); // Quit the game
                }

                InvalidateRect(hwnd, NULL, TRUE);
            }
            return 0;




        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT rect;
            GetClientRect(hwnd, &rect);
            HBRUSH bgBrush = CreateSolidBrush(RGB(255, 255, 255));
            FillRect(hdc, &rect, bgBrush);
            DeleteObject(bgBrush);

            HBRUSH circleBrush = CreateSolidBrush(RGB(255, 0, 255));
            SelectObject(hdc, circleBrush);
            Ellipse(hdc, circleX - circleRadius, circleY - circleRadius,
                    circleX + circleRadius, circleY + circleRadius);
            DeleteObject(circleBrush);

            int highScore = GetHighScore();
            char buffer[100];
            sprintf(buffer, "Score: %d | Time Left: %d | High Score: %d", score, remainingTime, highScore);
            TextOut(hdc, 10, 10, buffer, strlen(buffer));

            EndPaint(hwnd, &ps);
        }
        return 0;


        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
