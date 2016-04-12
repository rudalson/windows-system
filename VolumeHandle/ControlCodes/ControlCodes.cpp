// ControlCodes.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define DrivePath	_T("\\\\.\\PhysicalDrive0")
#define M_Drive	_T("\\\\.\\PhysicalDrive0")

HANDLE
OpenDevice(LPCTSTR devicename)
{
	HANDLE	handle = INVALID_HANDLE_VALUE;

	handle = CreateFile(devicename, GENERIC_READ, FILE_SHARE_READ,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (handle == INVALID_HANDLE_VALUE)
	{
		printf("LOG_ERROR: OpenDevice: cannot open %s\n", devicename);
	}

	return handle;
}

BOOL GetDriveGeometry(LPCTSTR Path, DISK_GEOMETRY_EX *pdg)
{
	HANDLE hDevice = INVALID_HANDLE_VALUE;	// handle to the drive to be examined 
	BOOL bResult = FALSE;					// results flag
	DWORD junk = 0;							// discard results

	hDevice = CreateFile(Path,				// drive to open
		0,									// no access to the drive
		FILE_SHARE_READ | FILE_SHARE_WRITE,	// share mode
		NULL,								// default security attributes
		OPEN_EXISTING,						// disposition
		0,									// file attributes
		NULL);								// do not copy file attributes

	if (hDevice == INVALID_HANDLE_VALUE)    // cannot open the drive
	{
		return (FALSE);
	}

	bResult = DeviceIoControl(hDevice,		// device to be queried
		IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,		// operation to perform
		NULL, 0,							// no input buffer
		pdg, sizeof(*pdg),					// output buffer
		&junk,								// # bytes returned
		(LPOVERLAPPED)NULL);				// synchronous I/O

	CloseHandle(hDevice);

	return (bResult);
}

void ShowUsage(LPCTSTR AppName)
{
	_tprintf(TEXT("%s <device_path>\n\n"), AppName);
	_tprintf(TEXT("Example :\n"));
	_tprintf(TEXT("  %s \\\\.\\PhysicalDrive0\n"), AppName);
	_tprintf(TEXT("  %s \\\\.\\C:\n"), AppName);
	_tprintf(TEXT("  %s \\\\.\\HarddiskVolume1\n"), AppName);
}

int _tmain(int argc, _TCHAR* argv[])
{
	DISK_GEOMETRY_EX pdg = { 0 }; // disk drive geometry structure
	BOOL bResult = FALSE;      // generic results flag
	ULONGLONG DiskSize = 0;    // size of the drive, in bytes

	if (argc < 2)
	{
		ShowUsage(argv[0]);
		return 1;
	}

	bResult = GetDriveGeometry(argv[1], &pdg);

	if (bResult)
	{
		_tprintf(TEXT("Drive path      = %s\n"), argv[1]);
		_tprintf(TEXT("Cylinders       = %I64d\n"), pdg.Geometry.Cylinders);
		_tprintf(TEXT("Tracks/cylinder = %ld\n"), (ULONG)pdg.Geometry.TracksPerCylinder);
		_tprintf(TEXT("Sectors/track   = %ld\n"), (ULONG)pdg.Geometry.SectorsPerTrack);
		_tprintf(TEXT("Bytes/sector    = %ld\n"), (ULONG)pdg.Geometry.BytesPerSector);

		DiskSize = pdg.Geometry.Cylinders.QuadPart * (ULONG)pdg.Geometry.TracksPerCylinder *
			(ULONG)pdg.Geometry.SectorsPerTrack * (ULONG)pdg.Geometry.BytesPerSector;
		_tprintf(TEXT("Disk size       = %I64d (Bytes)\n")
			TEXT("                = %.2f (Gb)\n"),
			DiskSize, (double)DiskSize / (1024 * 1024 * 1024));
	}
	else
	{
		_tprintf(TEXT("GetDriveGeometry failed. Error %ld.\n"), GetLastError());
	}

	return ((int)bResult);
}

