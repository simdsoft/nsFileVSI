// nsFileVSI.cpp : Defines the exported functions for the DLL application.
// V1.1

#include <Windows.h>

#include "nsis/pluginapi.h" // nsis plugin

#if defined(_DEBUG)
#pragma comment(lib, "Version.lib")
#pragma comment(lib, "nsis/pluginapi-x86-ansi.lib")
#endif

typedef BOOL(WINAPI* LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

static
BOOL isWOW64()
{
    BOOL bIsWow64 = FALSE;

    //IsWow64Process is not available on all supported versions of Windows.    
    //Use GetModuleHandle to get a handle to the DLL that contains the function    
    //and GetProcAddress to get a pointer to the function if available.    

    LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(
        GetModuleHandle(TEXT("kernel32")), "IsWow64Process");

    if (NULL != fnIsWow64Process)
    {
        if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
        {
            return FALSE;
        }
    }
    return bIsWow64;
}

static
BOOL getProductVersion(LPCTSTR lpszFileName, TCHAR* pszOutputBuffer, DWORD dwOutputSize)
{
    DWORD dwVerHnd = 0;
    DWORD dwVerInfoSize;
    TCHAR szSrcfn[MAX_PATH];
    WORD nVer[4];

    lstrcpy(szSrcfn, lpszFileName);

    dwVerInfoSize = GetFileVersionInfoSize(szSrcfn, &dwVerHnd);
    if (dwVerInfoSize > 0)
    {
        HANDLE hMem;
        LPVOID lpvMem;
        unsigned int uInfoSize = 0;
        VS_FIXEDFILEINFO* pFileInfo;

        hMem = GlobalAlloc(GMEM_MOVEABLE, dwVerInfoSize);
        lpvMem = GlobalLock(hMem);
        GetFileVersionInfo(szSrcfn, dwVerHnd, dwVerInfoSize, lpvMem);
        VerQueryValue(lpvMem, (LPTSTR)_T("\\"), (void**)& pFileInfo, &uInfoSize);


        nVer[0] = HIWORD(pFileInfo->dwProductVersionMS);
        nVer[1] = LOWORD(pFileInfo->dwProductVersionMS);
        nVer[2] = HIWORD(pFileInfo->dwProductVersionLS);
        nVer[3] = LOWORD(pFileInfo->dwProductVersionLS);
        wsprintfA(pszOutputBuffer, _T("%d.%d.%d.%d"), nVer[0], nVer[1], nVer[2], nVer[3]);

        GlobalUnlock(hMem);
        GlobalFree(hMem);

        return TRUE;
    }

    return FALSE;
}

#if defined(__cplusplus)
extern "C" {
#endif
    // To work with Unicode version of NSIS, please use TCHAR-type
    // functions for accessing the variables and the stack.
    void __declspec(dllexport) GetSystemFileProductVersion(HWND hwndParent, int string_size,
        LPTSTR variables, stack_t** stacktop,
        extra_parameters* extra, ...)
    {
        // note if you want parameters from the stack, pop them off in order.
        // i.e. if you are called via exdll::myFunction file.dat read.txt
        // calling popstring() the first time would give you file.dat,
        // and the second time would give you read.txt. 
        // you should empty the stack of your parameters, and ONLY your
        // parameters.
        char fileName[256];
        char fullPath[2048];
        char pszVersion[256] = { 0 };
        int arch = 0;
        BOOL x64os = FALSE;
        BOOL ok = TRUE;
        PVOID OldValue = NULL;

        EXDLL_INIT();
        // g_hwndParent = hwndParent;

        PopStringA(fileName);
        arch = popint();

        GetEnvironmentVariable("WINDIR", fullPath, sizeof(fullPath));

        x64os = isWOW64();

        if ((x64os && arch == 0x64) || (!x64os && arch == 0x86)) {
            lstrcatA(fullPath, "\\System32\\");
        }
        else if (x64os && arch == 0x86) {
            lstrcatA(fullPath, "\\SysWOW64\\");
        }
        else { // Invalid
            ok = FALSE;
        }

        if (ok) {
            lstrcatA(fullPath, fileName);
            Wow64DisableWow64FsRedirection(OldValue);
            ok = getProductVersion(fullPath, pszVersion, sizeof(pszVersion));
            if (OldValue)
                Wow64RevertWow64FsRedirection(OldValue);
        }
        if (ok)
            PushStringA(pszVersion);
        else
            PushStringA("failed");
    }

    void __declspec(dllexport) IsX64OperatingSystem(HWND hwndParent, int string_size,
        LPTSTR variables, stack_t** stacktop,
        extra_parameters* extra, ...)
    {
        // note if you want parameters from the stack, pop them off in order.
        // i.e. if you are called via exdll::myFunction file.dat read.txt
        // calling popstring() the first time would give you file.dat,
        // and the second time would give you read.txt. 
        // you should empty the stack of your parameters, and ONLY your
        // parameters.
        EXDLL_INIT();

        pushint(isWOW64());
    }

    BOOL WINAPI DllMain(HINSTANCE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
    {
        // g_hInstance = hInst;
        return TRUE;
    }
#if defined(__cplusplus)
}
#endif
