#include "stdafx.h"

void DisplayAboutMessage(HINSTANCE hInstance)
{
    TCHAR*  Product = TEXT("Unknown");
    TCHAR*  Version = TEXT("Unknown");

    TCHAR   FileName[MAX_PATH];
    GetModuleFileName(hInstance, FileName, ARRAYSIZE(FileName));

    DWORD   Dummy;
    DWORD   Size = GetFileVersionInfoSize(FileName, &Dummy);
    void*   Info = nullptr;

    if (Size > 0)
    {
        Info = malloc(Size);
        // VS_VERSION_INFO   VS_VERSIONINFO  VS_FIXEDFILEINFO 
        GetFileVersionInfo(FileName, Dummy, Size, Info);

        UINT	Length;
        VerQueryValue(Info, TEXT("\\StringFileInfo\\0c0904b0\\ProductName"), (LPVOID *) &Product, &Length);
        VerQueryValue(Info, TEXT("\\StringFileInfo\\0c0904b0\\FileVersion"), (LPVOID *) &Version, &Length);
    }

    _tprintf(_T("%s Version %s\n"), Product, Version);
    _tprintf(_T(" By Adam Gates (adamgates84+software@gmail.com) - http://is.gd/radsoft\n"));

     free(Info);
}
