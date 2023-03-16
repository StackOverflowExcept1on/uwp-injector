#include <windows.h>
#include <shlobj.h>

#include <fstream>
#include <filesystem>

std::ofstream logFile;

BOOL APIENTRY DllMain(HMODULE /* hModule */, DWORD ul_reason_for_call, LPVOID /* lpReserved */) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH: {
            PWSTR pLocalAppDataFolder;
            SHGetKnownFolderPath(FOLDERID_LocalAppData, KNOWN_FOLDER_FLAG::KF_FLAG_DEFAULT, nullptr,
                                 &pLocalAppDataFolder);
            std::filesystem::path localAppDataFolder(pLocalAppDataFolder);
            CoTaskMemFree(pLocalAppDataFolder);

            logFile.open(localAppDataFolder / "log.txt");

            logFile << "successfully loaded into process memory" << std::endl;
            logFile << "pid: " << GetCurrentProcessId() << std::endl;
            break;
        }
        case DLL_THREAD_ATTACH: {
            //TODO
            break;
        }
        case DLL_THREAD_DETACH: {
            //TODO
            break;
        }
        case DLL_PROCESS_DETACH: {
            //TODO
            break;
        }
        default:
            //TODO
            break;
    }
    return TRUE;
}
