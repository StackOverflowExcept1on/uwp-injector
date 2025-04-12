#include <windows.h>
#include <tlhelp32.h>
#include <aclapi.h>

#include <cstdint>

/// This enums represents possible errors to hide it from others
/// useful for debugging
enum class InjectionResult : uint8_t {
    Success,
    CommandLineToArgvWFailed,
    GetStdHandleFailed,
    WriteConsoleWFailed,
    IncorrectArguments,
    CreateToolhelp32SnapshotFailed,
    Process32FirstWFailed,
    ProcessNotFound,
    GetFullPathNameWFailed,
    Module32FirstWFailed,
    CopyFileWFailed,
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

    operator HANDLE() const {
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

    operator T() const {
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

    operator LPVOID() const {
        return address;
    }

    bool write(LPWSTR buffer, SIZE_T size) {
        SIZE_T written = 0;
        return WriteProcessMemory(hProcess, address, buffer, size, &written) && written == size;
    }

    ~ProcessMemory() {
        VirtualFreeEx(hProcess, address, 0, MEM_RELEASE);
    }
};

#pragma code_seg(".text")

__declspec(align(1), allocate(".text")) const WCHAR usage[] = L"Usage: injector process.exe library.dll\n";
__declspec(align(1), allocate(".text")) const WCHAR filePrefix[] = L"injected_";
__declspec(align(1), allocate(".text")) const WCHAR kernel32[] = L"KERNEL32.DLL";
__declspec(align(1), allocate(".text")) const char procLoadLibraryW[] = "LoadLibraryW";
__declspec(align(1), allocate(".text")) const char procFreeLibrary[] = "FreeLibrary";

extern "C" /*DWORD*/ InjectionResult WINAPI mainCRTStartup() {
    int argc;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) {
        return InjectionResult::CommandLineToArgvWFailed;
    }

    auto hArgvOwned = LocalMemoryHandle(argv);

    if (argc != 3) {
        HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hConsoleOutput == INVALID_HANDLE_VALUE || !hConsoleOutput) {
            return InjectionResult::GetStdHandleFailed;
        }

        const DWORD usageLen = sizeof(usage) / sizeof(WCHAR) - 1;
        DWORD written = 0;
        if (!(WriteConsoleW(hConsoleOutput, usage, usageLen, &written, nullptr) && written == usageLen)) {
            return InjectionResult::WriteConsoleWFailed;
        }

        return InjectionResult::IncorrectArguments;
    }

    LPWSTR processName = hArgvOwned[1];
    LPWSTR libraryName = hArgvOwned[2];

    DWORD processId;

    {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            return InjectionResult::CreateToolhelp32SnapshotFailed;
        }

        Handle hSnapshotOwned = Handle(hSnapshot);

        PROCESSENTRY32W processEntry;
        processEntry.dwSize = sizeof(processEntry);

        if (!Process32FirstW(hSnapshotOwned, &processEntry)) {
            return InjectionResult::Process32FirstWFailed;
        }

        processId = 0;
        do {
            if (!lstrcmpW(processEntry.szExeFile, processName)) {
                processId = processEntry.th32ProcessID;
                break;
            }
        } while (Process32NextW(hSnapshotOwned, &processEntry));

        if (processId == 0) {
            return InjectionResult::ProcessNotFound;
        }
    }

    //drop hSnapshotOwned

    const SIZE_T filePrefixLen = sizeof(filePrefix) / sizeof(WCHAR) - 1;
    const size_t size1 = MAX_PATH - filePrefixLen;
    WCHAR buffer1[size1]; //allocate buffer of smaller size than MAX_PATH (including null-terminator)

    LPWSTR path1 = &buffer1[0];
    LPWSTR ptrFilePart = nullptr;
    DWORD pathLen = GetFullPathNameW(libraryName, size1, path1, &ptrFilePart);
    if (pathLen > size1 || pathLen == 0 || !ptrFilePart) {
        return InjectionResult::GetFullPathNameWFailed;
    }

    LPWSTR ptrEnd = &buffer1[pathLen];
    size_t fileLen = ptrEnd - ptrFilePart;

    const size_t size2 = MAX_PATH;
    WCHAR buffer2[size2]; //allocate larger buffer (including prefix and null-terminator)

    size_t copyLen = pathLen - fileLen;
    for (volatile size_t i = 0; i < copyLen; i = i + 1) { //don't use builtin memcpy
        buffer2[i] = buffer1[i];
    }

    LPWSTR ptr = &buffer2[copyLen];
    for (auto val = filePrefix; *val; ++val, ++ptr) {
        *ptr = *val;
    }

    for (auto val = ptrFilePart; *val; ++val, ++ptr) {
        *ptr = *val;
    }

    *ptr = L'\0';

    LPWSTR path2 = &buffer2[0];

    HMODULE previousModule;

    {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processId);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            return InjectionResult::CreateToolhelp32SnapshotFailed;
        }

        Handle hSnapshotOwned = Handle(hSnapshot);

        MODULEENTRY32W moduleEntry;
        moduleEntry.dwSize = sizeof(moduleEntry);

        if (!Module32FirstW(hSnapshotOwned, &moduleEntry)) {
            return InjectionResult::Module32FirstWFailed;
        }

        previousModule = nullptr;
        do {
            if (!lstrcmpW(moduleEntry.szExePath, path2)) {
                previousModule = moduleEntry.hModule;
                break;
            }
        } while (Module32NextW(hSnapshotOwned, &moduleEntry));
    }

    //drop hSnapshotOwned

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (!hProcess) {
        return InjectionResult::OpenProcessFailed;
    }

    Handle hProcessOwned = Handle(hProcess);

    if (previousModule) {
        auto freeLibrary = GetProcAddress(GetModuleHandleW(kernel32), procFreeLibrary);
        HANDLE hThread = CreateRemoteThread(hProcessOwned, nullptr, 0, (LPTHREAD_START_ROUTINE) freeLibrary,
                                            previousModule, 0, nullptr);
        if (!hThread) {
            return InjectionResult::CreateRemoteThreadFailed;
        }

        Handle hThreadOwned = Handle(hThread);

        if (WaitForSingleObject(hThreadOwned, INFINITE) == WAIT_FAILED) {
            return InjectionResult::WaitForSingleObjectFailed;
        }
    }

    if (!CopyFileW(path1, path2, FALSE)) {
        return InjectionResult::CopyFileWFailed;
    }

    {
        uint8_t pSidData[SECURITY_MAX_SID_SIZE];
        PSID pSid = pSidData;
        DWORD cbSid = sizeof(pSidData);

        if (!CreateWellKnownSid(WELL_KNOWN_SID_TYPE::WinBuiltinAnyPackageSid, nullptr, pSid, &cbSid)) {
            return InjectionResult::CreateWellKnownSidFailed;
        }

        PACL pAcl;
        PSECURITY_DESCRIPTOR pSecurityDescriptor;
        if (GetNamedSecurityInfoW(path2, SE_OBJECT_TYPE::SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
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

        if (SetNamedSecurityInfoW(path2, SE_OBJECT_TYPE::SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr,
                                  hNewAclOwned, nullptr) != ERROR_SUCCESS) {
            return InjectionResult::SetNamedSecurityInfoWFailed;
        }
    }

    //drop hSecurityDescriptorOwned, hNewAclOwned

    SIZE_T pathSize = pathLen * sizeof(WCHAR) + filePrefixLen * sizeof(WCHAR) + sizeof(WCHAR); //null terminated wide string
    LPVOID address = VirtualAllocEx(hProcessOwned, nullptr, pathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!address) {
        return InjectionResult::VirtualAllocExFailed;
    }

    ProcessMemory memoryOwned = ProcessMemory(hProcessOwned, address);
    if (!memoryOwned.write(path2, pathSize)) {
        return InjectionResult::WriteProcessMemoryFailed;
    }

    auto loadLibraryW = GetProcAddress(GetModuleHandleW(kernel32), procLoadLibraryW);
    HANDLE hThread = CreateRemoteThread(hProcessOwned, nullptr, 0, (LPTHREAD_START_ROUTINE) loadLibraryW, memoryOwned,
                                        0, nullptr);
    if (!hThread) {
        return InjectionResult::CreateRemoteThreadFailed;
    }

    Handle hThreadOwned = Handle(hThread);
    if (WaitForSingleObject(hThreadOwned, INFINITE) == WAIT_FAILED) {
        return InjectionResult::WaitForSingleObjectFailed;
    }

    return InjectionResult::Success;
}
