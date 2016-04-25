// ControlCodes.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define DrivePath	_T("\\\\.\\PhysicalDrive0")
#define M_Drive	_T("\\\\.\\PhysicalDrive0")

HANDLE OpenDevice(LPCTSTR devicename, DWORD dwDesiredAccess)
{
	HANDLE hDevice = CreateFile(devicename,	// drive to open
		dwDesiredAccess,					// no access to the drive
		FILE_SHARE_READ | FILE_SHARE_WRITE,	// share mode
		NULL,								// default security attributes
		OPEN_EXISTING,						// disposition
		0,									// file attributes
		NULL);								// do not copy file attributes


	if (INVALID_HANDLE_VALUE == hDevice)
	{
		printf("OpenDevice: cannot open %s\n", devicename);
	}

	return hDevice;
}

BOOL GetDriveGeometry(LPCTSTR Path, DISK_GEOMETRY_EX *pdg)
{
	BOOL bResult = FALSE;					// results flag
	DWORD junk = 0;							// discard results

	HANDLE hDevice = OpenDevice(Path, 0);

	if (INVALID_HANDLE_VALUE != hDevice)
	{
		bResult = DeviceIoControl(hDevice,		// device to be queried
			IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,		// operation to perform
			NULL, 0,							// no input buffer
			pdg, sizeof(*pdg),					// output buffer
			&junk,								// # bytes returned
			(LPOVERLAPPED)NULL);				// synchronous I/O

		CloseHandle(hDevice);
	}

	return (bResult);
}

void ShowDriveGeometry(LPCTSTR Path, DISK_GEOMETRY_EX *pdg)
{
	_tprintf(TEXT("Drive Geometry\n=============================\n"), Path);
	_tprintf(TEXT("Drive path      = %s\n"), Path);
	_tprintf(TEXT("Cylinders       = %I64d\n"), pdg->Geometry.Cylinders);
	_tprintf(TEXT("Tracks/cylinder = %ld\n"), (ULONG)pdg->Geometry.TracksPerCylinder);
	_tprintf(TEXT("Sectors/track   = %ld\n"), (ULONG)pdg->Geometry.SectorsPerTrack);
	_tprintf(TEXT("Bytes/sector    = %ld\n"), (ULONG)pdg->Geometry.BytesPerSector);

	ULONGLONG DiskSize = pdg->Geometry.Cylinders.QuadPart * (ULONG)pdg->Geometry.TracksPerCylinder *
		(ULONG)pdg->Geometry.SectorsPerTrack * (ULONG)pdg->Geometry.BytesPerSector;
	_tprintf(TEXT("Disk size       = %I64d (Bytes)\n")
		TEXT("                = %.2f (Gb)\n"),
		DiskSize, (double)DiskSize / (1024 * 1024 * 1024));
}

BOOL GetNtfsVolumeData(LPCTSTR Path, NTFS_VOLUME_DATA_BUFFER *pnvdb)
{
	BOOL bResult = FALSE;					// results flag
	DWORD junk = 0;							// discard results

	HANDLE hDevice = OpenDevice(Path, FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES);

	if (INVALID_HANDLE_VALUE != hDevice)
	{
		bResult = DeviceIoControl(hDevice,		// device to be queried
			FSCTL_GET_NTFS_VOLUME_DATA,			// operation to perform
			NULL, 0,							// no input buffer
			pnvdb, sizeof(*pnvdb),				// output buffer
			&junk,								// # bytes returned
			(LPOVERLAPPED)NULL);				// synchronous I/O

		CloseHandle(hDevice);
	}

	return (bResult);
}

void ShowNtfsVolumeData(LPCTSTR Path, NTFS_VOLUME_DATA_BUFFER *pnvdb)
{
	_tprintf(TEXT("\nNTFS Volume Data\n=============================\n"), Path);
	_tprintf(TEXT("Drive path                   = %s\n"), Path);
	_tprintf(TEXT("VolumeSerialNumber           = %llu\n"), pnvdb->VolumeSerialNumber);
	_tprintf(TEXT("NumberSectors                = %llu\n"), pnvdb->NumberSectors);
	_tprintf(TEXT("FreeClusters                 = %llu\n"), pnvdb->FreeClusters);
	_tprintf(TEXT("TotalReserved                = %llu\n"), pnvdb->TotalReserved);

	_tprintf(TEXT("BytesPerSector               = %ld\n"), (ULONG)pnvdb->BytesPerSector);
	_tprintf(TEXT("BytesPerCluster              = %ld\n"), (ULONG)pnvdb->BytesPerCluster);
	_tprintf(TEXT("BytesPerFileRecordSegment    = %ld\n"), (ULONG)pnvdb->BytesPerFileRecordSegment);
	_tprintf(TEXT("ClustersPerFileRecordSegment = %ld\n"), (ULONG)pnvdb->ClustersPerFileRecordSegment);

	_tprintf(TEXT("MftValidDataLength           = %llu\n"), pnvdb->MftValidDataLength);
	_tprintf(TEXT("MftStartLcn                  = %llu\n"), pnvdb->MftStartLcn);
	_tprintf(TEXT("Mft2StartLcn                 = %llu\n"), pnvdb->Mft2StartLcn);
	_tprintf(TEXT("MftZoneStart                 = %llu\n"), pnvdb->MftZoneStart);
	_tprintf(TEXT("MftZoneEnd                   = %llu\n"), pnvdb->MftZoneEnd);
}

BOOL GetVolumeBitmap(LPCTSTR Path, VOLUME_BITMAP_BUFFER *pvbb, UINT32 BitmapSize)
{
	BOOL bResult = FALSE;	// results flag
	DWORD junk = 0;			// discard results

	HANDLE hDevice = OpenDevice(Path, FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES);

	if (INVALID_HANDLE_VALUE != hDevice)
	{
		STARTING_LCN_INPUT_BUFFER slib;
		slib.StartingLcn.QuadPart = 0;
		bResult = DeviceIoControl(hDevice,		// device to be queried
			FSCTL_GET_VOLUME_BITMAP,			// operation to perform
			&slib, sizeof(slib),				// input buffer
			pvbb, BitmapSize,					// output buffer
			&junk,								// # bytes returned
			(LPOVERLAPPED)NULL);				// synchronous I/O

		CloseHandle(hDevice);
	}

	return (bResult);
}

int StringfyByte2Bit(BYTE ch, TCHAR str[], int count)
{
	int pc = 0;	// positive count
	for (int i = 0; i < count; ++i)
	{
		BYTE bit = 1;
		bit <<= i;
		str[i] = (!!(ch & bit)) + '0';
		pc += (!!(ch & bit));
	}

	return pc;
}

int ShowByte2Bit(BYTE ch, int count)
{
	int pc = 0;	// positive count
	TCHAR one_byte[9] = { 0, };
	pc = StringfyByte2Bit(ch, one_byte, count);
	_tprintf(TEXT("%s"), one_byte);

	return pc;
}

void ShowVolumeBitmap(LPCTSTR Path, VOLUME_BITMAP_BUFFER *pvbb)
{
	_tprintf(TEXT("\nVolume Bitmap\n=============================\n"), Path);
	_tprintf(TEXT("Drive path            = %s\n"), Path);
	_tprintf(TEXT("StartingLcn           = %llu\n"), pvbb->StartingLcn);
	_tprintf(TEXT("BitmapSize            = %llu\n"), pvbb->BitmapSize);
	_tprintf(TEXT("--------------------------------------------------------------\n"));

	int i, j;
	unsigned int bi = 0;	// bitmap index
	int pc = 0;		// positive count
	int line_round = (pvbb->BitmapSize.QuadPart + 31) / 32;

	BYTE * pch = pvbb->Buffer;

	for (j = 0; j < line_round; ++j)
	{
		for (i = 0; i < 4; ++i, ++pch)
		{
			unsigned int need = ((pvbb->BitmapSize.QuadPart - bi) > 8) ? 8 : (pvbb->BitmapSize.QuadPart - bi);
			pc += ShowByte2Bit(*pch, need);
			bi += need;
			_tprintf(TEXT(" "));
		}
		_tprintf(TEXT("\n"));
	}

	_tprintf(TEXT("--------------------------------------------------------------\n"));
	_tprintf(TEXT("1's count is %d, 0's count is %d\n"), pc, pvbb->BitmapSize.QuadPart - pc);
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
	if (argc < 2)
	{
		ShowUsage(argv[0]);
		return 1;
	}

	BOOL bResult = FALSE;      // generic results flag

	DISK_GEOMETRY_EX pdg = { 0 };
	bResult = GetDriveGeometry(argv[1], &pdg);
	if (bResult)
	{
		ShowDriveGeometry(argv[1], &pdg);
	}
	else
	{
		_tprintf(TEXT("GetDriveGeometry failed. Error %ld.\n"), GetLastError());
	}

	NTFS_VOLUME_DATA_BUFFER nvdb;
	bResult = GetNtfsVolumeData(argv[1], &nvdb);
	if (bResult) 
	{
		ShowNtfsVolumeData(argv[1], &nvdb);
	}
	else
	{
		_tprintf(TEXT("GetNtfsVolumeData failed. Error %ld.\n"), GetLastError());
	}

	UINT32 BitmapSize = sizeof(VOLUME_BITMAP_BUFFER) + 4096 * 512;
	VOLUME_BITMAP_BUFFER * pvbb = (VOLUME_BITMAP_BUFFER *)malloc(BitmapSize);
	bResult = GetVolumeBitmap(argv[1], pvbb, BitmapSize);
	if (bResult)
	{
		ShowVolumeBitmap(argv[1], pvbb);
	}
	else
	{
		_tprintf(TEXT("GetVolumeBitmap failed. Error %ld.\n"), GetLastError());
	}

	return ((int)bResult);
}

