#include "ProcessMonitor.h"
#include "NotificationWindow.h"

#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <cwctype>
#include <cstdlib>
#include <chrono>

#include <wincrypt.h>
#include <softpub.h>
#include <wintrust.h>
#include <tchar.h>

#pragma comment(lib, "wintrust.lib")
#pragma comment(lib, "crypt32.lib")

#include <thread>

// Verify the file is digitally signed and trusted by Windows
bool IsFileSignedAndTrusted(const std::wstring& filePath)
{
    WINTRUST_FILE_INFO fileInfo = {};
    fileInfo.cbStruct = sizeof(WINTRUST_FILE_INFO);
    fileInfo.pcwszFilePath = filePath.c_str();

    WINTRUST_DATA winTrustData = {};
    winTrustData.cbStruct = sizeof(WINTRUST_DATA);
    winTrustData.dwUIChoice = WTD_UI_NONE;
    winTrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
    winTrustData.dwUnionChoice = WTD_CHOICE_FILE;
    winTrustData.pFile = &fileInfo;

    GUID policyGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    LONG status = WinVerifyTrust(NULL, &policyGUID, &winTrustData);

    return (status == ERROR_SUCCESS);
}

// Retrieve the full executable path of a process by PID
std::wstring GetProcessPath(unsigned long pid)
{
    std::wstring path = L"<unknown>";
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (hProcess)
    {
        WCHAR buffer[MAX_PATH];
        DWORD size = MAX_PATH;
        if (QueryFullProcessImageNameW(hProcess, 0, buffer, &size))
            path = buffer;
        CloseHandle(hProcess);
    }
    return path;
}

// Generate a simple hash from string (used for firewall rule naming)
static unsigned int HashString(const std::wstring& str)
{
    unsigned int hash = 0;
    for (wchar_t ch : str)
    {
        hash = 31 * hash + ch;
    }
    return hash;
}

// Terminate suspicious process and add firewall block rules
bool TerminateAndBlock(unsigned long pid, const std::wstring& path)
{
    if (IsFileSignedAndTrusted(path))
    {
        std::wcout << L"[-] Process is signed and trusted. Skipping termination: " << path << L"\n";
        return false;
    }

    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    bool terminated = false;
    if (hProcess)
    {
        if (TerminateProcess(hProcess, 1))
        {
            std::wcout << L"[+] Process terminated: PID " << pid << L"\n";
            terminated = true;
        }
        else
        {
            std::wcout << L"[-] Failed to terminate process: PID " << pid << L"\n";
        }
        CloseHandle(hProcess);
    }
    else
    {
        std::wcout << L"[-] Could not open process: PID " << pid << L"\n";
    }

    if (terminated)
    {
        unsigned int ruleHash = HashString(path);
        std::wstring ruleName = L"DisGuardBlock_" + std::to_wstring(ruleHash);

        std::wstring ruleOut = L"netsh advfirewall firewall add rule name=\"" + ruleName +
            L"\" dir=out program=\"" + path + L"\" action=block enable=yes >nul 2>&1";

        std::wstring ruleIn = L"netsh advfirewall firewall add rule name=\"" + ruleName +
            L"\" dir=in program=\"" + path + L"\" action=block enable=yes >nul 2>&1";

        _wsystem(ruleOut.c_str());
        _wsystem(ruleIn.c_str());

        std::wcout << L"[+] Firewall rules added to block executable: " << path << L"\n";

        ShowPopupNotification(L"DisGuard", L"Suspicious process terminated and blocked:\n" + path);
    }

    return terminated;
}

// Check if process path contains suspicious keywords
bool IsSuspiciousProcess(const std::wstring& path)
{
    std::wstring lowered = path;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), ::towlower);
    return lowered.find(L"discordstealer") != std::wstring::npos ||
        lowered.find(L"inject") != std::wstring::npos ||
        lowered.find(L"grabber") != std::wstring::npos;
}

// Main loop monitoring for new suspicious processes
void MonitorProcesses()
{
    static std::vector<unsigned long> knownProcesses;

    while (true)
    {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }

        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(pe);
        if (Process32First(snapshot, &pe))
        {
            do
            {
                unsigned long pid = pe.th32ProcessID;
                if (std::find(knownProcesses.begin(), knownProcesses.end(), pid) == knownProcesses.end())
                {
                    std::wstring path = GetProcessPath(pid);
                    if (IsSuspiciousProcess(path))
                        TerminateAndBlock(pid, path);

                    knownProcesses.push_back(pid);
                }
            } while (Process32Next(snapshot, &pe));
        }

        CloseHandle(snapshot);

        // Remove processes that no longer exist
        knownProcesses.erase(
            std::remove_if(knownProcesses.begin(), knownProcesses.end(), [](unsigned long pid) {
                HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
                if (h) { CloseHandle(h); return false; }
                return true;
                }),
            knownProcesses.end());

        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
}
