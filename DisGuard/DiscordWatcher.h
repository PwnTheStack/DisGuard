#pragma once

#include <string>
#include <vector>

// Checks if a filename is considered sensitive and should be monitored
bool IsSensitiveFile(const std::wstring& file);

// Returns all existing Discord installation folders on the system
std::vector<std::wstring> GetAllDiscordFolders();

// Monitors a specific Discord folder for changes to sensitive files (blocking call)
void WatchDiscordFolder(std::wstring folderPath);

// Starts monitoring all detected Discord folders concurrently
void WatchAllDiscordFolders();
