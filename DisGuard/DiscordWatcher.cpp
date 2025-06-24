#include "DiscordWatcher.h"

#include <windows.h>
#include <shlobj.h>
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>

#include "Globals.h"

namespace fs = std::filesystem;

bool IsSensitiveFile(const std::wstring& file)
{
    std::wstring lowered = file;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), ::towlower);

    return lowered.find(L"leveldb") != std::wstring::npos ||
        lowered.find(L".ldb") != std::wstring::npos ||
        lowered.find(L"core.asar") != std::wstring::npos ||
        lowered.find(L"index.js") != std::wstring::npos ||
        lowered.find(L"inject") != std::wstring::npos ||
        lowered.find(L".js") != std::wstring::npos ||
        lowered.find(L".ts") != std::wstring::npos;
}

std::wstring GetKnownFolderPath(REFKNOWNFOLDERID folderId)
{
    PWSTR path = nullptr;
    if (SHGetKnownFolderPath(folderId, 0, NULL, &path) != S_OK)
        return L"";

    std::wstring result = path;
    CoTaskMemFree(path);
    return result;
}

std::vector<std::wstring> GetAllDiscordFolders()
{
    std::vector<std::wstring> paths;

    auto roaming = GetKnownFolderPath(FOLDERID_RoamingAppData);
    auto local = GetKnownFolderPath(FOLDERID_LocalAppData);

    if (!roaming.empty())
    {
        auto p = roaming + L"\\discord";
        if (fs::exists(p))
            paths.push_back(p);
    }

    if (!local.empty())
    {
        const wchar_t* candidates[] = { L"Discord", L"DiscordCanary", L"DiscordPTB" };
        for (auto candidate : candidates)
        {
            auto full = local + L"\\" + candidate;
            if (fs::exists(full))
                paths.push_back(full);
        }
    }

    return paths;
}

void WatchDiscordFolder(std::wstring folderPath)
{
    HANDLE hDir = CreateFileW(folderPath.c_str(), FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

    if (hDir == INVALID_HANDLE_VALUE)
    {
        std::wcerr << L"[-] Failed to open Discord directory: " << folderPath << L"\n";
        return;
    }

    char buffer[4096];
    DWORD bytesReturned;

    while (running)
    {
        if (ReadDirectoryChangesW(hDir, buffer, sizeof(buffer), TRUE,
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION,
            &bytesReturned, NULL, NULL))
        {
            auto fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer);
            do
            {
                std::wstring fileName(fni->FileName, fni->FileNameLength / sizeof(WCHAR));

                if (IsSensitiveFile(fileName))
                {
                    std::wstring fullPath = folderPath + L"\\" + fileName;
                    std::wcout << L"[!] Suspicious file changed: " << fullPath << L"\n";
                    // Consider showing notification here if needed
                }

                if (!fni->NextEntryOffset)
                    break;

                fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                    reinterpret_cast<char*>(fni) + fni->NextEntryOffset);

            } while (true);
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
    }

    CloseHandle(hDir);
}

void WatchAllDiscordFolders()
{
    auto folders = GetAllDiscordFolders();
    std::vector<std::thread> threads;

    for (const auto& folder : folders)
        threads.emplace_back([folder]() { WatchDiscordFolder(folder); });

    for (auto& t : threads)
    {
        if (t.joinable())
            t.join();
    }
}
