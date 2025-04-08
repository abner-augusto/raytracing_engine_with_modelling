#pragma once

#include <string>

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>

inline std::string OpenFileDialog(const wchar_t* filter) {
    OPENFILENAMEW ofn;
    wchar_t file[MAX_PATH] = L"";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, file, -1, nullptr, 0, nullptr, nullptr);
        std::string result(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, file, -1, &result[0], size_needed, nullptr, nullptr);
        return result;
    }
    return "";
}

#else

#include <cstdlib>
#include <array>

inline std::string OpenFileDialog(const char* filter_description = "All Files (*.*) | *.*") {
    std::string cmd = "zenity --file-selection --title=\"Select a File\"";
    std::array<char, 1024> buffer{};
    std::string result;

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    pclose(pipe);

    // Remove trailing newline
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }

    return result;
}
#endif
