#include "DiscordWatcher.h"
#include "ProcessMonitor.h"
#include "NotificationWindow.h"

#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <cstdlib>
#include <ctime>

#include "Globals.h"
#include "TrayIcon.h"
#include "StartupMemes.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int)
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    if (!InitializeTrayIcon(hInstance))
    {
        MessageBox(nullptr, L"Failed to initialize system tray icon.", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    InitGlobals();

    AllocConsole();
    FILE* pCout;
    freopen_s(&pCout, "CONOUT$", "w", stdout);

    // Hide console window by default
    if (HWND consoleWnd = GetConsoleWindow())
        ShowWindow(consoleWnd, SW_HIDE);

    auto discordFolders = GetAllDiscordFolders();
    if (discordFolders.empty())
    {
        MessageBox(nullptr, L"Could not locate any Discord folder.", L"Error", MB_ICONERROR);
        return 1;
    }

    // Start background threads
    std::thread notifyThread(NotifyThread);
    std::thread procThread(MonitorProcesses);

    // Start folder watchers asynchronously
    std::thread watcherThread([]() { WatchAllDiscordFolders(); });
    watcherThread.detach();

    std::wcout << L"[+] DisGuard started.\r\n";

    std::wstring message = L"Watching Discord folders for suspicious activity:\n";
    for (const auto& folder : discordFolders)
        message += L"  - " + folder + L"\n";

    std::wcout << message << L"\r\n";

    // Append a random meme to the startup message
    std::wstring memeToShow = startupMemes[std::rand() % startupMemes.size()];
    message += L"\r\n" + memeToShow;

    ShowPopupNotification(L"[BOOT] DisGuard Online", message, false, 10000);

    // Main message loop handles tray icon messages
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Signal threads to exit
    running = false;
    g_notifyRunning = false;

    {
        std::unique_lock<std::mutex> lock(g_notifyMutex);
        g_notifyCv.notify_all();
    }
    SetEvent(g_hExitEvent);

    procThread.join();
    notifyThread.join();

    MessageBox(nullptr, L"DisGuard exiting cleanly.", L"DisGuard", MB_OK | MB_ICONINFORMATION);

    return static_cast<int>(msg.wParam);
}
