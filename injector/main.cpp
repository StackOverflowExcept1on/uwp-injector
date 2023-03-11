#include <windows.h>
#include <tlhelp32.h>
#include <aclapi.h>

#include <cstdint>

/// This enums represents possible errors to hide it from others
/// useful for debugging
enum class InjectionResult : uint8_t {
    Success,
    CommandLineToArgvWFailed,
    IncorrectArguments,
    CreateToolhelp32SnapshotFailed,
    ProcessNotFound,
    CreateWellKnownSidFailed,
    GetNamedSecurityInfoWFailed,
    SetEntriesInAclWFailed,
    SetNamedSecurityInfoWFailed,
    OpenProcessFailed,
    VirtualAllocExFailed,
    WriteProcessMemoryFailed,
    CreateRemoteThreadFailed,
    WaitForSingleObjectFailed,
};

/// Represents owned `HANDLE`, must be valid!
class Handle {
private:
    HANDLE handle;
public:
    explicit Handle(HANDLE handle) : handle(handle) {}

    HANDLE raw() {
        return handle;
    }

    ~Handle() {
        CloseHandle(handle);
    }
};

/// Represents owned `val`, must be valid! Will be freed with `LocalFree`
template<class T>
class LocalMemoryHandle {
private:
    T val;
public:
    explicit LocalMemoryHandle(T val) : val(val) {}

    T raw() {
        return val;
    }

    ~LocalMemoryHandle() {
        LocalFree(val);
    }
};

/// Represents owned memory created by `VirtualAllocEx`, must be valid!
class ProcessMemory {
private:
    HANDLE hProcess;
    LPVOID address;
public:
    explicit ProcessMemory(HANDLE hProcess, LPVOID address) : hProcess(hProcess), address(address) {}

    HANDLE raw() {
        return address;
    }

    bool write(LPWSTR buffer, SIZE_T size) {
        SIZE_T written = 0;
        return WriteProcessMemory(hProcess, address, (LPCVOID) buffer, size, &written) &&
               written == size;
    }

    ~ProcessMemory() {
        VirtualFreeEx(hProcess, address, 0, MEM_RELEASE);
    }
};

#pragma code_seg(".text")

__declspec(align(1), allocate(".text")) const WCHAR usage[] = L"Usage: injector CalculatorApp.exe library.dll\n";
__declspec(align(1), allocate(".text")) const WCHAR kernel32[] = L"KERNEL32.DLL";
__declspec(align(1), allocate(".text")) const char procLoadLibraryW[] = "LoadLibraryW";

extern "C" /*DWORD*/ InjectionResult mainCRTStartup() {
    int argc;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) {
        return InjectionResult::CommandLineToArgvWFailed;
    }

    auto hArgvOwned = LocalMemoryHandle(argv);
    if (argc != 3) {
        WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), usage, sizeof(usage) / sizeof(WCHAR), nullptr, nullptr);
        return InjectionResult::IncorrectArguments;
    }

    LPWSTR processName = hArgvOwned.raw()[1];
    LPWSTR libraryName = hArgvOwned.raw()[2];

    DWORD processId;

    {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            return InjectionResult::CreateToolhelp32SnapshotFailed;
        }

        Handle hSnapshotOwned = Handle(hSnapshot);

        PROCESSENTRY32W processEntry;
        processEntry.dwSize = sizeof(processEntry);

        processId = 0;
        while (Process32NextW(hSnapshotOwned.raw(), &processEntry)) {
            if (!lstrcmpW(processEntry.szExeFile, processName)) {
                processId = processEntry.th32ProcessID;
                break;
            }
        }

        if (processId == 0) {
            return InjectionResult::ProcessNotFound;
        }
    }

    //drop hSnapshotOwned

    const size_t size = MAX_PATH + 1;
    WCHAR buffer[size];

    LPWSTR path = &buffer[0];
    DWORD len = GetFullPathNameW(libraryName, size, path, nullptr);

    {
        uint8_t pSidData[SECURITY_MAX_SID_SIZE];
        PSID pSid = pSidData;
        DWORD cbSid = sizeof(pSidData);

        if (!CreateWellKnownSid(WELL_KNOWN_SID_TYPE::WinBuiltinAnyPackageSid, nullptr, pSid, &cbSid)) {
            return InjectionResult::CreateWellKnownSidFailed;
        }

        PACL pAcl;
        PSECURITY_DESCRIPTOR pSecurityDescriptor;
        if (GetNamedSecurityInfoW(path, SE_OBJECT_TYPE::SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
                                  nullptr, nullptr, &pAcl, nullptr, &pSecurityDescriptor) != ERROR_SUCCESS) {
            return InjectionResult::GetNamedSecurityInfoWFailed;
        }

        auto hSecurityDescriptorOwned = LocalMemoryHandle(pSecurityDescriptor);

        EXPLICIT_ACCESS_W explicitAccess = {
            .grfAccessPermissions = GENERIC_READ | GENERIC_EXECUTE,
            .grfAccessMode = ACCESS_MODE::SET_ACCESS,
            .grfInheritance = NO_INHERITANCE,
            .Trustee = {
                .pMultipleTrustee = nullptr,
                .MultipleTrusteeOperation = MULTIPLE_TRUSTEE_OPERATION::NO_MULTIPLE_TRUSTEE,
                .TrusteeForm = TRUSTEE_FORM::TRUSTEE_IS_SID,
                .TrusteeType = TRUSTEE_TYPE::TRUSTEE_IS_WELL_KNOWN_GROUP,
                .ptstrName = (LPWCH) pSid,
            }
        };

        PACL pNewAcl;
        if (SetEntriesInAclW(1, &explicitAccess, pAcl, &pNewAcl) != ERROR_SUCCESS) {
            return InjectionResult::SetEntriesInAclWFailed;
        }

        auto hNewAclOwned = LocalMemoryHandle(pNewAcl);

        if (SetNamedSecurityInfoW(path, SE_OBJECT_TYPE::SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
                                  nullptr, nullptr, hNewAclOwned.raw(), nullptr) != ERROR_SUCCESS) {
            return InjectionResult::SetNamedSecurityInfoWFailed;
        }
    }

    //drop hSecurityDescriptorOwned, hNewAclOwned

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (!hProcess) {
        return InjectionResult::OpenProcessFailed;
    }

    Handle hProcessOwned = Handle(hProcess);

    SIZE_T pathSize = len * sizeof(WCHAR) + sizeof(WCHAR); //null terminated wide string
    LPVOID address = VirtualAllocEx(hProcessOwned.raw(), nullptr, pathSize,
                                    MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!address) {
        return InjectionResult::VirtualAllocExFailed;
    }

    ProcessMemory memoryOwned = ProcessMemory(hProcessOwned.raw(), address);
    if (!memoryOwned.write(path, pathSize)) {
        return InjectionResult::WriteProcessMemoryFailed;
    }

    auto loadLibraryW = GetProcAddress(GetModuleHandleW(kernel32), procLoadLibraryW);
    HANDLE hThread = CreateRemoteThread(hProcessOwned.raw(), nullptr, 0,
                                        (LPTHREAD_START_ROUTINE) loadLibraryW, memoryOwned.raw(), 0, nullptr);
    if (!hThread) {
        return InjectionResult::CreateRemoteThreadFailed;
    }

    Handle hThreadOwned = Handle(hThread);
    if (WaitForSingleObject(hThreadOwned.raw(), INFINITE) == WAIT_FAILED) {
        return InjectionResult::WaitForSingleObjectFailed;
    }

    return InjectionResult::Success;
}
