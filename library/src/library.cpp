#include <windows.h>
#include <winrt/Windows.Storage.h>

#include <mutex>
#include <fstream>
#include <filesystem>
#include <format>

class Logger {
    std::mutex logMutex;
    std::ofstream logFile;

    Logger() {
        winrt::hstring localFolderPath = winrt::Windows::Storage::ApplicationData::Current().LocalFolder().Path();
        std::filesystem::path localFolder(localFolderPath.c_str());
        logFile = std::ofstream(localFolder / "log.txt", std::ofstream::app);
    }

    static Logger &getInstance() {
        // Scott Meyers Singleton is thread-safe for C++11 and above
        static Logger instance;
        return instance;
    }

    void write(const std::string &line) {
        std::lock_guard lock(logMutex);

        logFile << line << std::endl;
        logFile.flush();
    }

public:
    Logger(const Logger &) = delete;

    Logger &operator=(const Logger &) = delete;

    static void log(const std::string &line) {
        getInstance().write(line);
    }

    template<class... Args>
    static void log(std::format_string<Args...> fmt, Args &&... args) {
        log(std::format(fmt, std::forward<Args>(args)...));
    }
};

void onLibraryAttach() {
    Logger::log("loading module...");

    Logger::log("successfully loaded into process memory");
    Logger::log("pid: {}", GetCurrentProcessId());

    // custom code for memory manipulation should be here
}

void onLibraryDetach() {
    Logger::log("unloading module...");

    // custom code for cleanup should be here
}

DWORD WINAPI initializationThread(LPVOID /*lpThreadParameter*/) {
    onLibraryAttach();
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID /*lpReserved*/) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH: {
            DisableThreadLibraryCalls(hModule);
            HANDLE hThread = CreateThread(nullptr, 0, initializationThread, nullptr, 0, nullptr);
            if (hThread) {
                CloseHandle(hThread);
            }
            break;
        }
        case DLL_PROCESS_DETACH: {
            onLibraryDetach();
            break;
        }
    }
    return TRUE;
}
