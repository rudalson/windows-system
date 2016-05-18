#include <windows.h>
#include <winioctl.h>
#include <stdio.h>

#include "main.h"
#include "..\BadDisk\ctl_code.h"

void
usage()
{
    printf("usage: cmds [drive_letter] \n\n"
        "cmds:\n"
        "   /l : show disk's list \n"
        "   /h : hook disk \n"
        "   /uh : uhook disk \n"
        "   /hl : show hooked volume list \n"
        "\n\n"

        "examples:\n"
        "App /l : show disk's list\n"
        "App /h e : hook e drive\n"
        "App /uh e : unhook e drive\n"
        );
}

DWORD
__cdecl
main(
_In_ ULONG argc,
_In_reads_(argc) PCHAR argv[]
)
{
    if (argc < 2)
    {
        usage();
        return ERROR_SUCCESS;
    }

    DWORD res = ERROR_SUCCESS;

    if (!_stricmp(argv[1], "/l"))
    {
        res = showDeviceList();
    }
    else if (!_stricmp(argv[1], "/hl"))
    {
        res = showHookList();
    }
    else if (!_stricmp(argv[1], "/h"))
    {
        res = hookDevice(argv[2]);
    }
    else if (!_stricmp(argv[1], "/uh"))
    {
        res = unhookDevice(argv[2]);
    }
    else
    {
        usage();
    }

    return res;
}

HANDLE
OpenDevice(const char * device)
{
    HANDLE handle = INVALID_HANDLE_VALUE;

    handle = CreateFile(device,
        GENERIC_READ, // | GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (handle == INVALID_HANDLE_VALUE)
    {
        printf("OpenDevice: cannot open %s\n", device);
    }

    return handle;
}

void DisplayVolumePaths(
    __in PCHAR VolumeName
    )
{
    DWORD  CharCount = MAX_PATH + 1;
    PCHAR Names = NULL;
    PCHAR NameIdx = NULL;
    BOOL Success = FALSE;

    for (;;)
    {
        //
        //  Allocate a buffer to hold the paths.
        Names = (PCHAR) new BYTE[CharCount * sizeof(CHAR)];

        if (!Names)
        {
            //
            //  If memory can't be allocated, return.
            return;
        }

        //
        //  Obtain all of the paths
        //  for this volume.
        Success = GetVolumePathNamesForVolumeName(
            VolumeName, Names, CharCount, &CharCount
            );

        if (Success)
        {
            break;
        }

        if (GetLastError() != ERROR_MORE_DATA)
        {
            break;
        }

        //
        //  Try again with the
        //  new suggested size.
        delete[] Names;
        Names = NULL;
    }

    if (Success)
    {
        //
        //  Display the various paths.
        for (NameIdx = Names;
            NameIdx[0] != L'\0';
            NameIdx += strlen(NameIdx) + 1)
        {
            printf("  %s", NameIdx);
        }
        printf("\n");
    }

    if (Names != NULL)
    {
        delete[] Names;
        Names = NULL;
    }

    return;
}

DWORD showDeviceList()
{
    DWORD  Error = ERROR_SUCCESS;
    HANDLE FindHandle = INVALID_HANDLE_VALUE;
    CHAR  VolumeName[MAX_PATH] = "";
    CHAR  DeviceName[MAX_PATH] = "";

    FindHandle = FindFirstVolume(VolumeName, ARRAYSIZE(VolumeName));

    if (FindHandle == INVALID_HANDLE_VALUE)
    {
        Error = GetLastError();
        printf("FindFirstVolume failed with error code %d\n", Error);
        return Error;
    }

    size_t Index = 0;
    DWORD  CharCount = 0;
    BOOL   Success = FALSE;

    for (;;)
    {
        Index = strlen(VolumeName) - 1;

        if (VolumeName[0] != '\\' ||
            VolumeName[1] != '\\' ||
            VolumeName[2] != '?' ||
            VolumeName[3] != '\\' ||
            VolumeName[Index] != L'\\')
        {
            Error = ERROR_BAD_PATHNAME;
            printf("FindFirstVolume/FindNextVolume returned a bad path: %s\n", VolumeName);
            break;
        }

        VolumeName[Index] = '\0';

        CharCount = QueryDosDevice(&VolumeName[4], DeviceName, ARRAYSIZE(DeviceName));

        VolumeName[Index] = L'\\';

        if (CharCount == 0)
        {
            Error = GetLastError();
            printf("QueryDosDevice failed with error code %d\n", Error);
            break;
        }

        printf("\nFound a device:\n %s", DeviceName);
        //printf("\nVolume name: %s", VolumeName);
        printf("\nPaths:");
        DisplayVolumePaths(VolumeName);

        //
        //  Move on to the next volume.
        Success = FindNextVolume(FindHandle, VolumeName, ARRAYSIZE(VolumeName));

        if (!Success)
        {
            Error = GetLastError();

            if (Error != ERROR_NO_MORE_FILES)
            {
                printf("FindNextVolume failed with error code %d\n", Error);
                break;
            }

            //
            //  Finished iterating
            //  through all the volumes.
            Error = ERROR_SUCCESS;
            break;
        }
    }

    FindVolumeClose(FindHandle);
    FindHandle = INVALID_HANDLE_VALUE;

    return ERROR_SUCCESS;
}

DWORD showHookList()
{
    DWORD result = ERROR_SUCCESS;

    HANDLE device = OpenDevice(DEVICE_NAME);
    if (INVALID_HANDLE_VALUE == device)
    {
        return GetLastError();
    }

    __try
    {
        ULONG iolen = 0;
        WCHAR device_name_array[8][64] = { 0, };

        if (!DeviceIoControl(device, IOCTL_VOLMJHK_HOOKLIST,
            NULL, 0, &device_name_array, 8 * 64 * 2, &iolen, NULL))
        {
            result = GetLastError();
            printf("DeviceIoControl failed! error code(%d)\n", result);
            return result;
        }

        //printf("DeviceIoControl received. iolen(%d)\n", iolen);

        if (iolen)
        {
            WCHAR(*pdev)[64] = device_name_array;

            for (; iolen ; ++pdev)
            {
                printf("hooked : %ws\n", pdev);
                iolen -= 128;
            }
        }
    }
    __finally
    {
        if (INVALID_HANDLE_VALUE != device)
        {
            CloseHandle(device);
        }
    }

    return result;
}

DWORD hookDevice(PCHAR letter)
{
    DWORD result = ERROR_SUCCESS;
    WCHAR drive[] = L"c:";
    WCHAR device_name[MAX_PATH] = { 0, };
    drive[0] = (WCHAR)letter[0];

    DWORD count = QueryDosDeviceW(&drive[0], device_name, ARRAYSIZE(device_name));

    if (0 == count)
    {
        result = GetLastError();
        printf("QueryDosDevice failed! error code(%d)\n", result);
        return result;
    }

    printf("hook %ws\n", device_name);

    HANDLE device = OpenDevice(DEVICE_NAME);
    if (INVALID_HANDLE_VALUE == device)
    {
        return GetLastError();
    }

    __try
    {
        ULONG iolen;

        if (!DeviceIoControl(device, IOCTL_VOLMJHK_HOOK,
            &device_name, ARRAYSIZE(device_name), NULL, 0, &iolen, NULL))
        {
            result = GetLastError();
            printf("DeviceIoControl failed! error code(%d)\n", result);
            return result;
        }
    }
    __finally
    {
        if (INVALID_HANDLE_VALUE != device)
        {
            CloseHandle(device);
        }
    }
    
    return result;
}

DWORD unhookDevice(PCHAR letter)
{
    DWORD result;
    WCHAR drive[] = L"c:";
    WCHAR device_name[MAX_PATH] = { 0, };
    drive[0] = (WCHAR)letter[0];

    DWORD count = QueryDosDeviceW(&drive[0], device_name, ARRAYSIZE(device_name));

    if (0 == count)
    {
        result = GetLastError();
        printf("QueryDosDevice failed! error code(%d)\n", result);
        return result;
    }

    printf("unhook %ws\n", device_name);

    HANDLE device = OpenDevice(DEVICE_NAME);
    if (INVALID_HANDLE_VALUE == device)
    {
        return GetLastError();
    }

    __try
    {
        ULONG iolen;

        if (!DeviceIoControl(device, IOCTL_VOLMJHK_UNHOOK,
            &device_name, ARRAYSIZE(device_name), NULL, 0, &iolen, NULL))
        {
            DWORD res = GetLastError();
            printf("DeviceIoControl failed! error code(%d)\n", res);
            return res;
        }
    }
    __finally
    {
        if (INVALID_HANDLE_VALUE != device)
        {
            CloseHandle(device);
        }
    }

    return ERROR_SUCCESS;
}
