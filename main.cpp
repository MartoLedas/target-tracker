#include <windows.h>
#include <stdlib.h>
#include <time.h>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <string>
#include "gamewin.h"

typedef int (*CalculateScoreFunc)(bool, int);
typedef int (*SaveHighScoreFunc)(const char*, int);
typedef int (*GetHighScoreFunc)(const char*);

CalculateScoreFunc CalculateScore;
SaveHighScoreFunc SaveHighScore;
GetHighScoreFunc GetHighScore;

HMODULE hGameScoreDLL;

INT_PTR CALLBACK MenuProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

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

bool LoadGameScoreDLL(HMODULE &hDLL) {
    hDLL = LoadLibrary(TEXT("gamewinDLL.dll"));
    if (!hDLL) {
        MessageBox(NULL, TEXT("Failed to load the dll"), TEXT("Error"), MB_ICONERROR);
        return false;
    }

    CalculateScore = (CalculateScoreFunc)GetProcAddress(hDLL, "CalculateScore");
    SaveHighScore = (SaveHighScoreFunc)GetProcAddress(hDLL, "SaveHighScore");
    GetHighScore = (GetHighScoreFunc)GetProcAddress(hDLL, "GetHighScore");

    if (!CalculateScore || !SaveHighScore || !GetHighScore) {
        MessageBox(NULL, TEXT("Failed to find functions in the dll"), TEXT("Error"), MB_ICONERROR);
        FreeLibrary(hDLL);
        return false;
    }

    return true;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

    srand((unsigned int)time(NULL)); // Seed RNG

    DialogBox(hInstance, MAKEINTRESOURCE(IDD_MENU), NULL, MenuProc);

    const char CLASS_NAME[] = "AimTrainer";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_GAMEICON));

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

                HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_GAMEICON));
                if (hIcon) {
                    SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
                    SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
                } else {
                    MessageBox(hDlg, "Failed to load custom icon.", "Error", MB_OK | MB_ICONERROR);
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
            } else if (LOWORD(wParam) == IDC_EXIT_BUTTON) {
                EndDialog(hDlg, -1);
                PostQuitMessage(0);
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
        case WM_CREATE: {
            srand(time(NULL));

            hGameScoreDLL = LoadLibrary(TEXT("gamewinDLL.dll"));
            if (!hGameScoreDLL) {
                MessageBox(hwnd, TEXT("Failed to load the dll"), TEXT("Error"), MB_ICONERROR);
                PostQuitMessage(0);
                return -1;
            }

            CalculateScore = (CalculateScoreFunc)GetProcAddress(hGameScoreDLL, "CalculateScore");
            SaveHighScore = (SaveHighScoreFunc)GetProcAddress(hGameScoreDLL, "SaveHighScore");
            GetHighScore = (GetHighScoreFunc)GetProcAddress(hGameScoreDLL, "GetHighScore");

            if (!CalculateScore || !SaveHighScore || !GetHighScore) {
                MessageBox(hwnd, TEXT("Failed to find functions in the dll"), TEXT("Error"), MB_ICONERROR);
                FreeLibrary(hGameScoreDLL);
                PostQuitMessage(0);
                return -1;
            }

            RECT rect;
            GetClientRect(hwnd, &rect);
            circleX = rand() % (rect.right - 2 * circleRadius) + circleRadius;
            circleY = rand() % (rect.bottom - 2 * circleRadius) + circleRadius;

            circleVelocityX = 5; // Speed in x direction
            circleVelocityY = 5; // Speed in y direction

            SetTimer(hwnd, TIMER_ID, 50, NULL);
            SetTimer(hwnd, 2, 1000, NULL);
            return 0;
        }

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

                score = CalculateScore(isCursorInside, score);

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

                    SaveHighScore("highscore.txt", score);

                    int highScore = GetHighScore("highscore.txt");

                    char buffer[100];
                    sprintf(buffer, "Your score: %d\nHighscore: %d", score, highScore);
                    MessageBox(hwnd, buffer, "Time's up!", MB_OK);
                    TCHAR szFilePath[MAX_PATH];
                    GetModuleFileName(NULL, szFilePath, MAX_PATH);
                    ShellExecute(NULL, TEXT("open"), szFilePath, NULL, NULL, SW_SHOWNORMAL);

                    PostQuitMessage(0);
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

            int highScore = GetHighScore("highscore.txt");

            char buffer[100];
            sprintf(buffer, "High Score: %d | Score: %d | Time Left: %d", highScore, score, remainingTime);
            TextOut(hdc, 10, 10, buffer, strlen(buffer));

            EndPaint(hwnd, &ps);
        }
        return 0;

        case WM_DESTROY:
            if (hGameScoreDLL) {
                FreeLibrary(hGameScoreDLL);
            }

            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
