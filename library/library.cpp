#include <windows.h>
#include <shlobj.h>

#include <string>
#include <fstream>

std::wstring localAppDataFolder;
std::wofstream logFile;

BOOL APIENTRY DllMain(HMODULE /* hModule */, DWORD ul_reason_for_call, LPVOID /* lpReserved */) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH: {
            PWSTR pLocalAppDataFolder;
            SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &pLocalAppDataFolder);
            localAppDataFolder = pLocalAppDataFolder;
            CoTaskMemFree(pLocalAppDataFolder);

            logFile.open(localAppDataFolder + L"\\log.txt");

            logFile << "successfully loaded into process memory" << std::endl;
            logFile << "pid: " << GetCurrentProcessId() << std::endl;
        }
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
