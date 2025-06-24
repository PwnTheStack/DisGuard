#pragma once

#include <windows.h>

// Initializes the system tray icon.
bool InitializeTrayIcon(HINSTANCE hInstance);

// Cleans up tray icon resources
void CleanupTrayIcon();

// Signals the application to exit, stopping background threads
void SignalExit();

// Global flag indicating whether the application is running
extern bool g_running;
