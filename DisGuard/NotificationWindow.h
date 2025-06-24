#pragma once

#include <windows.h>
#include <string>

// Displays a popup notification with optional centering and display duration
void ShowPopupNotification(const std::wstring& title, const std::wstring& message, bool centerScreen = false, int displayDurationMs = 5000);

// Thread function that manages notification window messages and rendering
void NotifyThread();

// Window procedure for the notification popup window
LRESULT CALLBACK NotifyWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
