#include "TrayIcon.h"
#include "Globals.h"
#include "resource.h"

static HWND g_hWnd = nullptr;

static LRESULT CALLBACK TrayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_USER + 1:  // Tray icon callback
        switch (lParam)
        {
        case WM_LBUTTONUP:
        {
            HWND consoleWnd = GetConsoleWindow();
            if (consoleWnd)
            {
                if (IsWindowVisible(consoleWnd))
                    ShowWindow(consoleWnd, SW_HIDE);
                else
                {
                    ShowWindow(consoleWnd, SW_SHOW);
                    SetForegroundWindow(consoleWnd);
                }
            }
        }
        break;

        case WM_RBUTTONUP:
        {
            HWND consoleWnd = GetConsoleWindow();
            const wchar_t* toggleText = L"Show";
            if (consoleWnd && IsWindowVisible(consoleWnd))
                toggleText = L"Hide";

            POINT pt;
            GetCursorPos(&pt);
            HMENU menu = CreatePopupMenu();
            InsertMenu(menu, -1, MF_BYPOSITION | MF_STRING, 2000, toggleText);
            InsertMenu(menu, -1, MF_BYPOSITION | MF_STRING, 1000, L"Exit");
            SetForegroundWindow(hwnd);
            TrackPopupMenu(menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
            DestroyMenu(menu);
        }
        break;
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case 1000:  // Exit command
            SignalExit();
            PostQuitMessage(0);
            break;

        case 2000:  // Toggle console visibility
        {
            HWND consoleWnd = GetConsoleWindow();
            if (consoleWnd)
            {
                if (IsWindowVisible(consoleWnd))
                    ShowWindow(consoleWnd, SW_HIDE);
                else
                {
                    ShowWindow(consoleWnd, SW_SHOW);
                    SetForegroundWindow(consoleWnd);
                }
            }
        }
        break;
        }
        break;

    case WM_DESTROY:
    {
        NOTIFYICONDATA nid = {};
        nid.cbSize = sizeof(nid);
        nid.hWnd = hwnd;
        nid.uID = 1;
        Shell_NotifyIcon(NIM_DELETE, &nid);
        PostQuitMessage(0);
    }
    break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

bool InitializeTrayIcon(HINSTANCE hInstance)
{
    const wchar_t CLASS_NAME[] = L"DisGuardTrayWindow";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = TrayWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    if (!RegisterClassW(&wc))
        return false;

    g_hWnd = CreateWindowExW(0, CLASS_NAME, L"DisGuard Tray Window",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, hInstance, nullptr);

    if (!g_hWnd)
        return false;

    NOTIFYICONDATA nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = g_hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_USER + 1;
    nid.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_DISGUARD_ICON));
    wcscpy_s(nid.szTip, L"DisGuard - Protecting Discord");

    if (!Shell_NotifyIconW(NIM_ADD, &nid))
        return false;

    ShowWindow(g_hWnd, SW_HIDE);  // Keep window hidden, tray icon only

    return true;
}

void CleanupTrayIcon()
{
    if (g_hWnd)
    {
        DestroyWindow(g_hWnd);
        g_hWnd = nullptr;
    }
}

void SignalExit()
{
    CleanupGlobals();
    running = false;
    PostQuitMessage(0);
    ExitProcess(0);
}
