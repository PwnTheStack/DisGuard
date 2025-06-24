#pragma once

#include <string>

// Get full executable path by process ID
std::wstring GetProcessPath(unsigned long pid);

// Check if file is digitally signed and trusted
bool IsFileSignedAndTrusted(const std::wstring& filePath);

// Terminate process and add firewall block rules, returns true if terminated
bool TerminateAndBlock(unsigned long pid, const std::wstring& path);

// Check if process path looks suspicious (based on keywords)
bool IsSuspiciousProcess(const std::wstring& path);

// Main monitoring loop for suspicious processes (blocking call)
void MonitorProcesses();
