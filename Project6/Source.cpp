#include <windows.h>
#include <tchar.h>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <iostream>

const int maxCellsCount = 200;
const int defaultCellsCount = 4;
const int defaultWindowWidth = 320;
const int defaultWindowHeight = 240;

struct Settings {
    int cellsCount;
    int windowWidth;
    int windowHeight;
    COLORREF windowBgColor;
    COLORREF gridLineColor;
    COLORREF currentBgColor;
};

Settings appConfig = { defaultCellsCount, defaultWindowWidth, defaultWindowHeight, RGB(0, 0, 255), RGB(255, 0, 0), RGB(0, 0, 255) };

int grid[maxCellsCount][maxCellsCount] = { 0 };
int cellWidth, cellHeight;
HBRUSH bgBrush;

void LoadSettings();
void SaveSettings(const Settings& settings);
void ModifyGridColor(HDC hdc);
LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

void LoadSettings() {
    FILE* configFile;

    if (fopen_s(&configFile, "config.txt", "r") == 0) {
        if (fscanf_s(configFile, "CellsCount=%d\n", &appConfig.cellsCount) != 1) {
            appConfig.cellsCount = defaultCellsCount;
        }

        if (fscanf_s(configFile, "WindowSize=%d %d\n", &appConfig.windowWidth, &appConfig.windowHeight) != 2) {
            appConfig.windowWidth = defaultWindowWidth;
            appConfig.windowHeight = defaultWindowHeight;
        }

        int red, green, blue;

        if (fscanf_s(configFile, "WindowBgColor=%d %d %d\n", &red, &green, &blue) == 3) {
            appConfig.windowBgColor = RGB(red, green, blue);
            appConfig.currentBgColor = RGB(red, green, blue);
        }
        else {
            red = 255;
            green = 255;
            blue = 255;
            appConfig.windowBgColor = RGB(red, green, blue);
            appConfig.currentBgColor = RGB(red, green, blue);
        }

        if (fscanf_s(configFile, "GridLineColor=%d %d %d\n", &red, &green, &blue) == 3) {
            appConfig.gridLineColor = RGB(red, green, blue);
        }
        else {
            red = 255;
            green = 0;
            blue = 0;
            appConfig.gridLineColor = RGB(red, green, blue);
        }

        fclose(configFile);
    }
    else {
        std::cerr << "Failed to open config file. Using default settings." << std::endl;
    }
}

void SaveSettings(const Settings& settings) {
    FILE* configFile;

    if (fopen_s(&configFile, "config.txt", "w") == 0) {
        fprintf(configFile, "CellsCount=%d\n", settings.cellsCount);
        fprintf(configFile, "WindowSize=%d %d\n", settings.windowWidth, settings.windowHeight);
        fprintf(configFile, "WindowBgColor=%d %d %d\n", GetRValue(settings.currentBgColor),
            GetGValue(settings.currentBgColor), GetBValue(settings.currentBgColor));
        fprintf(configFile, "GridLineColor=%d %d %d\n", GetRValue(settings.gridLineColor),
            GetGValue(settings.gridLineColor), GetBValue(settings.gridLineColor));

        fclose(configFile);
    }
    else {
        std::cerr << "Failed to create or open config file for writing." << std::endl;
    }
}

void ModifyGridColor(HDC hdc) {
    HPEN newPen = CreatePen(PS_SOLID, 1, appConfig.gridLineColor);
    if (newPen == nullptr) {
        std::cerr << "Failed to create grid color pen." << std::endl;
        return;
    }

    HPEN oldPen = (HPEN)SelectObject(hdc, newPen);

    for (int i = 1; i < appConfig.cellsCount; i++) {
        int y = i * cellHeight;
        MoveToEx(hdc, 0, y, nullptr);
        LineTo(hdc, appConfig.windowWidth, y);
    }

    for (int i = 1; i < appConfig.cellsCount; i++) {
        int x = i * cellWidth;
        MoveToEx(hdc, x, 0, nullptr);
        LineTo(hdc, x, appConfig.windowHeight);
    }

    SelectObject(hdc, oldPen);
    if (!DeleteObject(newPen)) {
        std::cerr << "Failed to delete grid color pen." << std::endl;
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    int cellsCount = defaultCellsCount;
    if (lpCmdLine && lpCmdLine[0] != '\0') {
        cellsCount = atoi(lpCmdLine);
        cellsCount = min(maxCellsCount, max(cellsCount, 1));
    }

    LoadSettings();
    appConfig.cellsCount = cellsCount;

    WNDCLASSEX windowClass = { 0 };
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProcedure;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.lpszClassName = L"WinAPIAppClass";

    if (!RegisterClassEx(&windowClass)) {
        MessageBox(nullptr, L"Failed to register window class!", L"Error", MB_ICONERROR);
        return 0;
    }

    HWND hwnd = CreateWindow(windowClass.lpszClassName, L"Lab 3", WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_THICKFRAME,
        100, 100, appConfig.windowWidth, appConfig.windowHeight, nullptr, nullptr, hInstance, nullptr);

    cellWidth = appConfig.windowWidth / appConfig.cellsCount;
    cellHeight = appConfig.windowHeight / appConfig.cellsCount;

    if (hwnd == nullptr) {
        MessageBox(nullptr, L"Failed to create window!", L"Error", MB_ICONERROR);
        return 0;
    }

    bgBrush = CreateSolidBrush(appConfig.windowBgColor);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = { nullptr };
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    SaveSettings(appConfig);

    // Free resources
    DeleteObject(bgBrush);

    return msg.wParam;
}

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CLOSE:
        PostQuitMessage(0);
        break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        FillRect(hdc, &ps.rcPaint, bgBrush);

        HPEN redPen = CreatePen(PS_SOLID, 1, appConfig.gridLineColor);
        if (redPen == nullptr) {
            std::cerr << "Failed to create grid color pen for painting." << std::endl;
            EndPaint(hwnd, &ps);
            return 0;
        }

        HPEN oldPen = (HPEN)SelectObject(hdc, redPen);

        for (int i = 1; i < appConfig.cellsCount; i++) {
            int y = i * cellHeight;
            MoveToEx(hdc, 0, y, nullptr);
            LineTo(hdc, ps.rcPaint.right, y);
        }

        for (int i = 1; i < appConfig.cellsCount; i++) {
            int x = i * cellWidth;
            MoveToEx(hdc, x, 0, nullptr);
            LineTo(hdc, x, ps.rcPaint.bottom);
        }

        SelectObject(hdc, oldPen);
        if (!DeleteObject(redPen)) {
            std::cerr << "Failed to delete grid color pen after painting." << std::endl;
        }

        for (int row = 0; row < appConfig.cellsCount; row++) {
            for (int col = 0; col < appConfig.cellsCount; col++) {
                int cellValue = grid[row][col];
                if (cellValue == 1) {
                    Ellipse(hdc, col * cellWidth, row * cellHeight, (col + 1) * cellWidth, (row + 1) * cellHeight);
                }
                else if (cellValue == 2) {
                    MoveToEx(hdc, col * cellWidth, row * cellHeight, nullptr);
                    LineTo(hdc, (col + 1) * cellWidth, (row + 1) * cellHeight);
                    MoveToEx(hdc, (col + 1) * cellWidth, row * cellHeight, nullptr);
                    LineTo(hdc, col * cellWidth, (row + 1) * cellHeight);
                }
            }
        }

        EndPaint(hwnd, &ps);
    }
    break;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE || (GetKeyState(VK_LCONTROL) & 0x8000 && wParam == 'Q')) {
            PostQuitMessage(0);
        }
        else if (GetKeyState(VK_SHIFT) & 0x8000 && wParam == 'C') {
            ShellExecute(nullptr, L"open", L"notepad.exe", nullptr, nullptr, SW_SHOWNORMAL);
        }
        break;
    case WM_CHAR:
        if (wParam == VK_RETURN) {
            int red = rand() % 256;
            int green = rand() % 256;
            int blue = rand() % 256;
            appConfig.currentBgColor = RGB(red, green, blue);
            DeleteObject(bgBrush);
            bgBrush = CreateSolidBrush(appConfig.currentBgColor);
            InvalidateRect(hwnd, nullptr, TRUE);
        }
        break;
    case WM_MOUSEWHEEL:
    {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        int red = GetRValue(appConfig.gridLineColor);
        int green = GetGValue(appConfig.gridLineColor);
        int blue = GetBValue(appConfig.gridLineColor);

        if (delta > 0) {
            red = min(255, red + 2);
            green = min(255, green + 2);
            blue = min(255, blue + 2);
        }
        else {
            red = max(0, red - 2);
            green = max(0, green - 2);
            blue = max(0, blue - 2);
        }
        appConfig.gridLineColor = RGB(red, green, blue);
        InvalidateRect(hwnd, nullptr, TRUE);
    }
    break;
    case WM_LBUTTONDOWN:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        int col = cellWidth == 0 ? 0 : x / cellWidth;
        int row = cellHeight == 0 ? 0 : y / cellHeight;

        if (col < appConfig.cellsCount && row < appConfig.cellsCount && grid[row][col] == 0) {
            grid[row][col] = 1;
            InvalidateRect(hwnd, nullptr, TRUE);
        }
    }
    break;
    case WM_RBUTTONDOWN:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);

        int col = cellWidth == 0 ? 0 : x / cellWidth;
        int row = cellHeight == 0 ? 0 : y / cellHeight;

        if (col < appConfig.cellsCount && row < appConfig.cellsCount && grid[row][col] == 0) {
            grid[row][col] = 2;
            InvalidateRect(hwnd, nullptr, TRUE);
        }
    }
    break;
    case WM_SIZE:
    {
        appConfig.windowWidth = LOWORD(lParam);
        appConfig.windowHeight = HIWORD(lParam);

        cellWidth = appConfig.cellsCount == 0 ? 0 : appConfig.windowWidth / appConfig.cellsCount;
        cellHeight = appConfig.cellsCount == 0 ? 0 : appConfig.windowHeight / appConfig.cellsCount;

        // Update grid parameters on window size change
        InvalidateRect(hwnd, nullptr, TRUE);
    }
    break;

    case WM_SIZING:
    {
        // Handle resizing window while maintaining minimum size
        RECT* rect = (RECT*)lParam;
        int newWidth = rect->right - rect->left;
        int newHeight = rect->bottom - rect->top;

        newWidth = max(newWidth, cellWidth * appConfig.cellsCount);
        newHeight = max(newHeight, cellHeight * appConfig.cellsCount);

        rect->right = rect->left + newWidth;
        rect->bottom = rect->top + newHeight;

        return TRUE;
    }
    break;

    case WM_NCHITTEST:
    {
        // Allow resizing from the bottom
        LRESULT hitTest = DefWindowProc(hwnd, message, wParam, lParam);

        if (hitTest == HTBOTTOM || hitTest == HTBOTTOMLEFT || hitTest == HTBOTTOMRIGHT) {
            return HTBOTTOM;
        }

        return hitTest;
    }
    break;

    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0;
}

