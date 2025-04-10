#ifndef ASSETPATH_H
#define ASSETPATH_H

#include <string>
#include <filesystem>

#if defined(_WIN32)
#include <windows.h>
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif
#elif defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#include <limits.h>
#endif

class AssetPath {
public:
    static std::string GetExecutableDir() {
        char path[PATH_MAX];

#if defined(_WIN32)
        DWORD length = GetModuleFileNameA(NULL, path, PATH_MAX);
        if (length == 0 || length == PATH_MAX)
            return "";
#elif defined(__linux__) || defined(__APPLE__)
        ssize_t length = readlink("/proc/self/exe", path, PATH_MAX);
        if (length == -1 || length == PATH_MAX)
            return "";
        path[length] = '\0';
#endif

        return std::filesystem::path(path).parent_path().string();
    }

    static std::string Resolve(const std::string& relativeAssetPath) {
        // Use std::filesystem to join and normalize
        std::filesystem::path fullPath = std::filesystem::path(GetExecutableDir()) / "assets" / relativeAssetPath;
        return fullPath.generic_string();
    }
};

#endif // ASSETPATH_H