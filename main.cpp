#include <windows.h>
#include <shellapi.h>

HINSTANCE hInst;
NOTIFYICONDATA nid;
UINT WM_TASKBARCREATED;

// Function to get battery percentage
int GetBatteryPercent() {
    SYSTEM_POWER_STATUS sps;
    if (GetSystemPowerStatus(&sps)) {
        return (int)sps.BatteryLifePercent;
    }
    return -1;
}

// Map battery percentage to single char/number
char GetBatteryChar(int percent) {
    if (percent <= 0) return 'X';
    else if (percent < 15) return '1';
    else if (percent < 23) return '2';
    else if (percent < 28) return '+';
    else if (percent < 35) return '3';
    else if (percent < 45) return '4';
    else if (percent < 55) return '5';
    else if (percent < 65) return '6';
    else if (percent < 73) return '7';
    else if (percent < 78) return '/';
    else if (percent < 85) return '8';
    else if (percent < 95) return '9';
    else return 'F';
}

// Function to create an icon with battery "char"
HICON CreateBatteryIcon(int percent) {
    HDC hdc = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hbmColor = CreateCompatibleBitmap(hdc, 16, 16);
    HBITMAP hbmMask = CreateBitmap(16, 16, 1, 1, NULL); // Monochrome mask

    SelectObject(hdcMem, hbmColor);

    // White background
    HBRUSH brush = CreateSolidBrush(RGB(255, 255, 255));
    RECT rect = { 0, 0, 16, 16 };
    FillRect(hdcMem, &rect, brush);
    DeleteObject(brush);

    // Draw battery char
    SetBkMode(hdcMem, TRANSPARENT);
    SetTextColor(hdcMem, RGB(0, 0, 0));

    char c = GetBatteryChar(percent);
    char str[2] = { c, '\0' };
    DrawTextA(hdcMem, str, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // Create icon
    ICONINFO ii = {};
    ii.fIcon = TRUE;
    ii.hbmMask = hbmMask;
    ii.hbmColor = hbmColor;

    HICON hIcon = CreateIconIndirect(&ii);

    // Cleanup
    DeleteObject(hbmColor);
    DeleteObject(hbmMask);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdc);

    return hIcon;
}

// Update tray icon with new battery percentage
void UpdateTrayIcon() {
    int percent = GetBatteryPercent();
    if (percent < 0) return;

    HICON hIcon = CreateBatteryIcon(percent);

    nid.hIcon = hIcon;
    Shell_NotifyIcon(NIM_MODIFY, &nid);

    DestroyIcon(hIcon);
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_DESTROY) {
        Shell_NotifyIcon(NIM_DELETE, &nid);
        PostQuitMessage(0);
    }
    else if (msg == WM_USER + 1 && lParam == WM_RBUTTONUP) {
        // Right-click menu
        POINT pt;
        GetCursorPos(&pt);

        HMENU hMenu = CreatePopupMenu();
        AppendMenu(hMenu, MF_STRING, 1, TEXT("Exit"));

        SetForegroundWindow(hwnd);
        int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, NULL);
        if (cmd == 1) {
            DestroyWindow(hwnd);
        }

        DestroyMenu(hMenu);
    }
    else if (msg == WM_TASKBARCREATED) {
        // Re-add icon if explorer restarted
        Shell_NotifyIcon(NIM_ADD, &nid);
        UpdateTrayIcon();
    }
    else if (msg == WM_TIMER) {
        UpdateTrayIcon();
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// WinMain Entry
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    hInst = hInstance;
    WM_TASKBARCREATED = RegisterWindowMessage(TEXT("TaskbarCreated"));

    // Register window class
    WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = TEXT("BatteryTrayClass");
    RegisterClassEx(&wc);

    HWND hwnd = CreateWindow(TEXT("BatteryTrayClass"), TEXT("Battery Tray"), 0,
        0, 0, 0, 0, NULL, NULL, hInstance, NULL);

    // Initialize tray icon
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
    nid.uCallbackMessage = WM_USER + 1;
    lstrcpy(nid.szTip, TEXT("Battery Percentage"));

    Shell_NotifyIcon(NIM_ADD, &nid);
    UpdateTrayIcon(); // Show first icon immediately

    // Set timer to update every 30 seconds
    SetTimer(hwnd, 1, 30000, NULL);

    // Main message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
