#include "Globals.h"

std::mutex g_notifyMutex;
std::condition_variable g_notifyCv;
std::queue<std::wstring> g_notifyQueue;
std::atomic<bool> g_notifyRunning{ false };
std::atomic<bool> running{ true };
std::wstring g_DiscordPath;
HANDLE g_hExitEvent = nullptr;

void InitGlobals()
{
    g_notifyRunning = false;
    running = true;
    g_hExitEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
}

void CleanupGlobals()
{
    if (g_hExitEvent)
    {
        CloseHandle(g_hExitEvent);
        g_hExitEvent = nullptr;
    }
}
