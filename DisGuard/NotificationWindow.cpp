#include "NotificationWindow.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <queue>
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <iostream>

#include "Globals.h"

const wchar_t g_notifyClassName[] = L"DisGuardNotifyWindow";

LRESULT CALLBACK NotifyWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rect;
        GetClientRect(hwnd, &rect);

        // Background
        HBRUSH bgBrush = CreateSolidBrush(RGB(240, 240, 240));
        FillRect(hdc, &rect, bgBrush);
        DeleteObject(bgBrush);

        // Rounded border
        HPEN pen = CreatePen(PS_SOLID, 2, RGB(0, 120, 215));
        SelectObject(hdc, pen);
        RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, 15, 15);
        DeleteObject(pen);

        // Draw text stored in window user data
        std::wstring* pText = reinterpret_cast<std::wstring*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        if (pText)
        {
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(0, 0, 0));

            HFONT hFont = CreateFontW(
                20, 0, 0, 0,
                FW_BOLD,
                FALSE, FALSE, FALSE,
                DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS,
                CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY,
                DEFAULT_PITCH | FF_DONTCARE,
                L"Segoe UI");

            HFONT oldFont = reinterpret_cast<HFONT>(SelectObject(hdc, hFont));

            RECT textRect = rect;
            textRect.left += 15;
            textRect.right -= 15;
            textRect.top += 20;
            textRect.bottom -= 20;

            DrawTextW(hdc, pText->c_str(), static_cast<int>(pText->length()), &textRect, DT_WORDBREAK);

            SelectObject(hdc, oldFont);
            DeleteObject(hFont);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_NCHITTEST:
        return HTTRANSPARENT; // Allow click-through
    case WM_DESTROY:
        return 0;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

void NotifyThread()
{
    HINSTANCE hInst = GetModuleHandle(nullptr);

    WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
    wc.lpfnWndProc = NotifyWndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = g_notifyClassName;
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    RegisterClassEx(&wc);

    HWND hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        g_notifyClassName,
        L"DisGuard Notify Pump",
        WS_POPUP,
        0, 0, 0, 0,
        nullptr, nullptr, hInst, nullptr);
    ShowWindow(hwnd, SW_HIDE);

    MSG msg;
    while (g_notifyRunning)
    {
        std::wstring text;

        {
            std::unique_lock<std::mutex> lock(g_notifyMutex);
            if (g_notifyQueue.empty())
                g_notifyCv.wait(lock, [] { return !g_notifyQueue.empty() || !g_notifyRunning; });

            if (!g_notifyRunning)
                break;

            if (!g_notifyQueue.empty())
            {
                text = std::move(g_notifyQueue.front());
                g_notifyQueue.pop();
            }
        }

        if (text.empty())
            continue;

        const int width = 320;
        const int height = 90;

        RECT workArea;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);

        int x = workArea.right - width - 20;
        int y = workArea.bottom - height - 40;

        HWND notifyWnd = CreateWindowEx(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_LAYERED,
            g_notifyClassName,
            L"DisGuard Notification",
            WS_POPUP,
            x, y, width, height,
            nullptr, nullptr, hInst, nullptr);

        if (!notifyWnd)
        {
            std::cerr << "[NotifyThread] Failed to create notify window\n";
            continue;
        }

        SetWindowLongPtr(notifyWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(new std::wstring(text)));

        SetLayeredWindowAttributes(notifyWnd, 0, 255, LWA_ALPHA);

        ShowWindow(notifyWnd, SW_SHOW);
        UpdateWindow(notifyWnd);

        auto start = std::chrono::steady_clock::now();
        int alpha = 255;

        MSG notifyMsg;
        bool done = false;
        while (!done)
        {
            while (PeekMessage(&notifyMsg, notifyWnd, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&notifyMsg);
                DispatchMessage(&notifyMsg);
            }

            auto elapsed = std::chrono::steady_clock::now() - start;

            if (elapsed > std::chrono::seconds(3))
            {
                alpha -= 15;
                if (alpha <= 0)
                    done = true;
                else
                    SetLayeredWindowAttributes(notifyWnd, 0, static_cast<BYTE>(alpha), LWA_ALPHA);

                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
            }
        }

        auto ptr = reinterpret_cast<std::wstring*>(GetWindowLongPtr(notifyWnd, GWLP_USERDATA));
        if (ptr)
            delete ptr;

        DestroyWindow(notifyWnd);
    }

    DestroyWindow(hwnd);
    UnregisterClass(g_notifyClassName, hInst);
}

void ShowPopupNotification(const std::wstring& title, const std::wstring& message, bool centerScreen, int displayDurationMs)
{
    std::thread([=]() {
        MessageBeep(MB_ICONEXCLAMATION);

        constexpr int padding = 20;

        HINSTANCE hInstance = GetModuleHandleW(nullptr);

        static bool classRegistered = false;
        if (!classRegistered)
        {
            WNDCLASSW wc = {};
            wc.lpfnWndProc = DefWindowProcW;
            wc.hInstance = hInstance;
            wc.lpszClassName = L"DisGuardNotification";
            wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
            RegisterClassW(&wc);
            classRegistered = true;
        }

        HWND hwnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
            L"DisGuardNotification",
            L"",
            WS_POPUP,
            CW_USEDEFAULT, CW_USEDEFAULT, 400, 200,
            nullptr, nullptr, hInstance, nullptr
        );

        HDC hdc = GetDC(hwnd);

        HFONT hFontTitle = CreateFontW(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH, L"Segoe UI");

        HFONT hFontMsg = CreateFontW(17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH, L"Segoe UI");

        SelectObject(hdc, hFontTitle);
        RECT rcTitle = { 0,0,300,0 };
        DrawTextW(hdc, title.c_str(), -1, &rcTitle, DT_CALCRECT | DT_LEFT | DT_SINGLELINE);

        SelectObject(hdc, hFontMsg);
        RECT rcMsg = { 0,0,300,0 };
        DrawTextW(hdc, message.c_str(), -1, &rcMsg, DT_CALCRECT | DT_LEFT | DT_WORDBREAK);

        int width = (rcTitle.right > rcMsg.right ? rcTitle.right : rcMsg.right) + 2 * padding;
        int height = rcTitle.bottom + rcMsg.bottom + 3 * padding;

        RECT wa;
        if (centerScreen)
        {
            SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0);
            int x = wa.left + ((wa.right - wa.left) - width) / 2;
            int y = wa.top + ((wa.bottom - wa.top) - height) / 2;
            SetWindowPos(hwnd, HWND_TOPMOST, x, y, width, height, SWP_NOACTIVATE | SWP_SHOWWINDOW);
        }
        else
        {
            SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0);
            int x = wa.right - width - 20;
            int y = wa.bottom - height - 20;
            SetWindowPos(hwnd, HWND_TOPMOST, x, y, width, height, SWP_NOACTIVATE | SWP_SHOWWINDOW);
        }

        // Prepare off-screen DC and bitmap for smooth rendering
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP bmp = CreateCompatibleBitmap(hdc, width, height);
        SelectObject(memDC, bmp);

        HBRUSH bg = CreateSolidBrush(RGB(25, 25, 25));
        RECT full = { 0, 0, width, height };
        FillRect(memDC, &full, bg);
        DeleteObject(bg);

        SetBkMode(memDC, TRANSPARENT);

        SetTextColor(memDC, RGB(255, 255, 255));
        SelectObject(memDC, hFontTitle);
        RECT rTitle = { padding, padding, width - padding, padding + rcTitle.bottom };
        DrawTextW(memDC, title.c_str(), -1, &rTitle, DT_LEFT | DT_SINGLELINE);

        SetTextColor(memDC, RGB(180, 180, 180));
        SelectObject(memDC, hFontMsg);
        RECT rMsg = { padding, rTitle.bottom + padding / 2, width - padding, height - padding };
        DrawTextW(memDC, message.c_str(), -1, &rMsg, DT_LEFT | DT_WORDBREAK);

        BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

        DeleteObject(bmp);
        DeleteDC(memDC);
        ReleaseDC(hwnd, hdc);
        DeleteObject(hFontTitle);
        DeleteObject(hFontMsg);

        std::this_thread::sleep_for(std::chrono::milliseconds(displayDurationMs));
        DestroyWindow(hwnd);
        }).detach();
}
