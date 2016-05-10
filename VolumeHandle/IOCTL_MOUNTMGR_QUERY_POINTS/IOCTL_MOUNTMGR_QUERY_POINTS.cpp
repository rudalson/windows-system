// IOCTL_MOUNTMGR_QUERY_POINTS.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#pragma comment(lib, "Crypt32.lib")

// ref : http://stackoverflow.com/questions/3012828/using-ioctl-mountmgr-query-points


#define MOUNTMGR_DOS_DEVICE_NAME                    L"\\\\.\\MountPointManager"
#define MOUNTMGRCONTROLTYPE                         0x0000006D // 'm'
#define MOUNTDEVCONTROLTYPE                         0x0000004D // 'M'
#define IOCTL_MOUNTDEV_QUERY_DEVICE_NAME    CTL_CODE(MOUNTDEVCONTROLTYPE, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MOUNTMGR_QUERY_POINTS                 CTL_CODE(MOUNTMGRCONTROLTYPE, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _MOUNTDEV_NAME
{
	USHORT  NameLength;
	WCHAR   Name[1];
} MOUNTDEV_NAME, *PMOUNTDEV_NAME;

typedef struct _MOUNTMGR_MOUNT_POINT
{
	ULONG   SymbolicLinkNameOffset;
	USHORT  SymbolicLinkNameLength;
	ULONG   UniqueIdOffset;
	USHORT  UniqueIdLength;
	ULONG   DeviceNameOffset;
	USHORT  DeviceNameLength;
} MOUNTMGR_MOUNT_POINT, *PMOUNTMGR_MOUNT_POINT;

typedef struct _MOUNTMGR_MOUNT_POINTS
{
	ULONG                   Size;
	ULONG                   NumberOfMountPoints;
	MOUNTMGR_MOUNT_POINT    MountPoints[1];
} MOUNTMGR_MOUNT_POINTS, *PMOUNTMGR_MOUNT_POINTS;

int main() {
	TCHAR chDrive = 'N', szUniqueId[128];
	TCHAR szDeviceName[7] = _T("\\\\.\\");
	HANDLE hDevice, hMountMgr;
	BYTE byBuffer[1024];
	PMOUNTDEV_NAME pMountDevName;
	DWORD cbBytesReturned, dwInBuffer, dwOutBuffer, ccb;
	PMOUNTMGR_MOUNT_POINT pMountPoint;
	BOOL bSuccess;
	PBYTE pbyInBuffer, pbyOutBuffer;
	LPTSTR pszLogicalDrives, pszDriveRoot;

	// MOUNTMGR_DOS_DEVICE_NAME is defined as L"\\\\.\\MountPointManager"
	hMountMgr = CreateFile(MOUNTMGR_DOS_DEVICE_NAME,
		0, FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, 0, NULL);
	if (hMountMgr == INVALID_HANDLE_VALUE)
		return 1;

	cbBytesReturned = GetLogicalDriveStrings(0, NULL);
	pszLogicalDrives = (LPTSTR)LocalAlloc(LMEM_ZEROINIT, cbBytesReturned*sizeof(TCHAR));
	cbBytesReturned = GetLogicalDriveStrings(cbBytesReturned, pszLogicalDrives);

	const wchar_t string[] = L"\\Device\\HarddiskVolume3";

	for (pszDriveRoot = pszLogicalDrives; *pszDriveRoot != TEXT('\0'); pszDriveRoot += lstrlen(pszDriveRoot) + 1) {

		szDeviceName[4] = pszDriveRoot[0];
		szDeviceName[5] = _T(':');
		szDeviceName[6] = _T('\0');
		//lstrcpy (&szDeviceName[4], TEXT("\\??\\USBSTOR\\DISK&VEN_SANDISK&PROD_CRUZER&REV_8.01\\1740030578903736&0"));

		hDevice = CreateFile(szDeviceName, 0,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, 0, NULL);
		if (hDevice == INVALID_HANDLE_VALUE)
			return 1;
#if 1
		bSuccess = DeviceIoControl(hDevice,
			IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
			NULL, 0,
			(LPVOID)byBuffer, sizeof(byBuffer),
			&cbBytesReturned,
			(LPOVERLAPPED)NULL);
		pMountDevName = (PMOUNTDEV_NAME)byBuffer;
		_tprintf(TEXT("\n%.*ls\n"), pMountDevName->NameLength / sizeof(WCHAR),
			pMountDevName->Name);
		bSuccess = CloseHandle(hDevice);
#endif
		dwInBuffer = pMountDevName->NameLength + sizeof(MOUNTMGR_MOUNT_POINT);
		pbyInBuffer = (PBYTE)LocalAlloc(LMEM_ZEROINIT, dwInBuffer);
		pMountPoint = (PMOUNTMGR_MOUNT_POINT)pbyInBuffer;
		pMountPoint->DeviceNameLength = pMountDevName->NameLength;
		pMountPoint->DeviceNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
		CopyMemory(pbyInBuffer + sizeof(MOUNTMGR_MOUNT_POINT),
			pMountDevName->Name, pMountDevName->NameLength);

		dwOutBuffer = 1024 + sizeof(MOUNTMGR_MOUNT_POINTS);
		pbyOutBuffer = (PBYTE)LocalAlloc(LMEM_ZEROINIT, dwOutBuffer);
		bSuccess = DeviceIoControl(hMountMgr,
			IOCTL_MOUNTMGR_QUERY_POINTS,
			pbyInBuffer, dwInBuffer,
			(LPVOID)pbyOutBuffer, dwOutBuffer,
			&cbBytesReturned,
			(LPOVERLAPPED)NULL);
		if (bSuccess) {
			ULONG i;
			PMOUNTMGR_MOUNT_POINTS pMountPoints = (PMOUNTMGR_MOUNT_POINTS)pbyOutBuffer;
			for (i = 0; i<pMountPoints->NumberOfMountPoints; i++) {
				_tprintf(TEXT("#%i:\n"), i);
				_tprintf(TEXT("    Device=%.*ls\n"),
					pMountPoints->MountPoints[i].DeviceNameLength / sizeof(WCHAR),
					pbyOutBuffer + pMountPoints->MountPoints[i].DeviceNameOffset);
				_tprintf(TEXT("    SymbolicLink=%.*ls\n"),
					pMountPoints->MountPoints[i].SymbolicLinkNameLength / sizeof(WCHAR),
					pbyOutBuffer + pMountPoints->MountPoints[i].SymbolicLinkNameOffset);
				ccb = sizeof(szUniqueId) / sizeof(szUniqueId[0]);

				if (CryptBinaryToString(pbyOutBuffer + pMountPoints->MountPoints[i].UniqueIdOffset,
					pMountPoints->MountPoints[i].UniqueIdLength,
					CRYPT_STRING_BASE64,
					szUniqueId, &ccb))
					_tprintf(TEXT("    UniqueId=%s\n"), szUniqueId);
				else
					_tprintf(TEXT("    UniqueId=%.*ls\n"),
					pMountPoints->MountPoints[i].UniqueIdLength / sizeof(WCHAR),
					pbyOutBuffer + pMountPoints->MountPoints[i].UniqueIdOffset);
			}
		}
		pbyInBuffer = (PBYTE)LocalFree(pbyInBuffer);
		pbyOutBuffer = (PBYTE)LocalFree(pbyOutBuffer);
	}
	pszLogicalDrives = (LPTSTR)LocalFree(pszLogicalDrives);
	bSuccess = CloseHandle(hMountMgr);

	return 0;
}

