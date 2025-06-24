#pragma once

#include <string>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <windows.h>

// Global synchronization primitives and flags for notification and main program control
extern std::mutex g_notifyMutex;
extern std::condition_variable g_notifyCv;
extern std::queue<std::wstring> g_notifyQueue;
extern std::atomic<bool> g_notifyRunning;
extern std::atomic<bool> running;
extern std::wstring g_DiscordPath;
extern HANDLE g_hExitEvent;

// Initialize global variables and resources
void InitGlobals();

// Clean up global resources before exit
void CleanupGlobals();
