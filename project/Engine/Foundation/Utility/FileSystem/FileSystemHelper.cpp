#include "FileSystemHelper.h"
#include <Engine/Foundation/Utility/Converter/ConvertString.h>

bool FileSystemHelper::CreateDirectoryPath(const std::string& path) {
    std::error_code ec;
    return std::filesystem::create_directories(path, ec);
}

std::optional<std::string> FileSystemHelper::FindFileRecursive(const std::string& searchDir, const std::string& fileName) {
    if (!std::filesystem::exists(searchDir)) {
        return std::nullopt;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(searchDir)) {
        if (entry.is_regular_file() && entry.path().filename().string() == fileName) {
            return entry.path().string();
        }
    }
    return std::nullopt;
}

std::optional<std::wstring> FileSystemHelper::FindFileRecursiveW(const std::wstring& searchDir, const std::wstring& fileName) {
    if (!std::filesystem::exists(searchDir)) {
        return std::nullopt;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(searchDir)) {
        if (entry.is_regular_file() && entry.path().filename().wstring() == fileName) {
            return entry.path().wstring();
        }
    }
    return std::nullopt;
}

std::unordered_map<std::string, std::string> FileSystemHelper::BuildFileCache(const std::string& searchDir) {
    std::unordered_map<std::string, std::string> cache;

    if (!std::filesystem::exists(searchDir)) {
        return cache;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(searchDir)) {
        if (entry.is_regular_file()) {
            cache[entry.path().filename().string()] = entry.path().string();
        }
    }
    return cache;
}

std::unordered_map<std::wstring, std::wstring> FileSystemHelper::BuildFileCacheW(const std::wstring& searchDir) {
    std::unordered_map<std::wstring, std::wstring> cache;

    if (!std::filesystem::exists(searchDir)) {
        return cache;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(searchDir)) {
        if (entry.is_regular_file()) {
            cache[entry.path().filename().wstring()] = entry.path().wstring();
        }
    }
    return cache;
}