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
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, file, -1, NULL, 0, NULL, NULL);
        std::string result(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, file, -1, &result[0], size_needed, NULL, NULL);
        return result;
    }
    return "";
}

#else
#include <string>
#include <cstdlib>

inline std::string OpenFileDialog(const char* filter = "*.obj") {
    const char* cmd =
        "zenity --file-selection --title=\"Select a File\" --file-filter=\"OBJ files (\*.obj) | *.obj\"";
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return "";

    char buffer[1024];
    std::string result = "";
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }
    pclose(pipe);

    // Remove trailing newline if present
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    return result;
}
#endif
