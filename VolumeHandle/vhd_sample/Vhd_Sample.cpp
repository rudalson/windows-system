// Vhd_Sample.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <string>
#include <virtdisk.h>

#pragma comment(lib, "virtdisk.lib")

// Fix unresolved link error
static const GUID VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT = { 0xEC984AEC, 0xA0F9, 0x47e9, 0x90, 0x1F, 0x71, 0x41, 0x5A, 0x66, 0x34, 0x5B };

#define ARRAY_SIZE(a)                               \
  ((sizeof(a) / sizeof(*(a))) /                     \
  static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))

DWORD CreateDisk(PCWSTR virtualDiskFilePath, HANDLE *handle)
{
	VIRTUAL_STORAGE_TYPE storageType =
	{
		VIRTUAL_STORAGE_TYPE_DEVICE_VHD,
		VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT
	};

	CREATE_VIRTUAL_DISK_PARAMETERS parameters = {};
	parameters.Version = CREATE_VIRTUAL_DISK_VERSION_1;
	parameters.Version1.MaximumSize = 1024 * 1024 * 1024;
	parameters.Version1.BlockSizeInBytes = CREATE_VIRTUAL_DISK_PARAMETERS_DEFAULT_BLOCK_SIZE;
	parameters.Version1.SectorSizeInBytes = CREATE_VIRTUAL_DISK_PARAMETERS_DEFAULT_SECTOR_SIZE;
	parameters.Version1.SourcePath = NULL;

	int result = ::CreateVirtualDisk(
		&storageType,
		virtualDiskFilePath,
		VIRTUAL_DISK_ACCESS_ALL,
		NULL,
		CREATE_VIRTUAL_DISK_FLAG_NONE,
		0,
		&parameters,
		NULL,
		handle);

	return result;
}

DWORD OpenDisk(PCWSTR virtualDiskFilePath, HANDLE *handle)
{
	VIRTUAL_STORAGE_TYPE storageType =
	{
		VIRTUAL_STORAGE_TYPE_DEVICE_VHD,
		VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT
	};

	OPEN_VIRTUAL_DISK_PARAMETERS parameters =
	{
		OPEN_VIRTUAL_DISK_VERSION_1
	};

	parameters.Version1.RWDepth = 1024;

	return ::OpenVirtualDisk(
		&storageType,
		virtualDiskFilePath,
		VIRTUAL_DISK_ACCESS_ALL,
		OPEN_VIRTUAL_DISK_FLAG_NONE,
		&parameters,
		handle);
}

int _tmain(int argc, _TCHAR* argv[])
{
	LPTSTR  virtualDiskFilePath = _T("c:\\drive.vhd");
	HANDLE  handle;
	DWORD   result;
	ULONG   bytesUsed;
	bool    vhdCreated = false;

	//  Create or open a virtual disk file
	result = CreateDisk(virtualDiskFilePath, &handle);
	if (result == ERROR_FILE_EXISTS)
	{
		result = OpenDisk(virtualDiskFilePath, &handle);
		if (result != ERROR_SUCCESS)
		{
			std::wcout << "Unable to open virtual disk" << std::endl;
			return result;
		}
	}
	else if (result != ERROR_SUCCESS)
	{
		std::wcout << "Unable to create virtual disk" << std::endl;
		return result;
	}
	else
	{
		vhdCreated = true;
	}

	//  Now that the virtual disk is open we need to mount it.
	//
	//  FROM MSDN:
	//  To attach and detach a virtual disk, you must also have the
	//  SE_MANAGE_VOLUME_NAME privilege present in your token. This privilege
	//  is stripped from an administrator's token when User Account Control is
	//  in use, so you may need to elevate your application to gain access to
	//  the unrestricted token that includes this privilege. 
	result = ::AttachVirtualDisk(
		handle,
		NULL,
		ATTACH_VIRTUAL_DISK_FLAG_PERMANENT_LIFETIME,//ATTACH_VIRTUAL_DISK_FLAG_NO_DRIVE_LETTER,
		0, // no provider-specific flags
		0, // no parameters
		NULL);
	if (result != ERROR_SUCCESS)
	{
		std::wcout << "Unable to attach virtual disk" << std::endl;
		return result;
	}

	if (result == ERROR_SUCCESS && vhdCreated == true)
	{
		std::wcout
			<< "Virtual disk image created. Go into the Computer Management admin panel" << std::endl
			<< "and add a volume and format it.\n" << std::endl;
		system("pause");
	}

	// Now we need to grab the device name \\.\PhysicalDrive#
	TCHAR   physicalDriveName[MAX_PATH];
	DWORD   physicalDriveNameSize = ARRAY_SIZE(physicalDriveName);

	result = ::GetVirtualDiskPhysicalPath(handle, &physicalDriveNameSize, physicalDriveName);
	if (result != ERROR_SUCCESS)
	{
		std::wcout << "Unable to retrieve virtual disk path" << std::endl;
		return result;
	}
	const std::wstring deviceName = physicalDriveName;


	//  HACK!!! Wait for windows to complete the mount.
	Sleep(2500);

	// In order to get the UNC path of the volumes located on the virtual disk we
	// need to enumerate all mounted volumes and check which device they are located
	// on.
	std::wstring volumeName;

	TCHAR volumeNameBuffer[MAX_PATH];
	HANDLE hVol = ::FindFirstVolume(volumeNameBuffer, ARRAY_SIZE(volumeNameBuffer));
	if (hVol == INVALID_HANDLE_VALUE)
	{
		std::wcout << "Unable to find first volume" << std::endl;
		return GetLastError();
	}

	do
	{
		//  Get rid of trailing backslash so we can open the volume
		size_t len = wcslen(volumeNameBuffer);
		if (volumeNameBuffer[len - 1] == '\\')
		{
			volumeNameBuffer[len - 1] = 0;
		}

		HANDLE volumeHandle = ::CreateFile(
			volumeNameBuffer,
			GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
			NULL);
		if (volumeHandle == INVALID_HANDLE_VALUE)
		{
			std::wcout << "Unable to open volume " << volumeNameBuffer << std::endl;
		}
		else
		{
			// We can grab the id of the device and use it to create a
			// proper device name.
			STORAGE_DEVICE_NUMBER deviceInfo = { 0 };
			if (::DeviceIoControl(
				volumeHandle,
				IOCTL_STORAGE_GET_DEVICE_NUMBER,
				NULL,
				0,
				&deviceInfo,
				sizeof(deviceInfo),
				&bytesUsed,
				NULL))
			{
				std::wstring tmpName(
					std::wstring(L"\\\\.\\PhysicalDrive")
					+ std::to_wstring((long long)deviceInfo.DeviceNumber));
				if (_wcsicmp(tmpName.c_str(), deviceName.c_str()) == 0)
				{
					volumeName = std::wstring(volumeNameBuffer) + L"\\\\";
					CloseHandle(volumeHandle);
					break;
				}
			}

			CloseHandle(volumeHandle);
		}
	} while (::FindNextVolume(hVol, volumeNameBuffer, ARRAY_SIZE(volumeNameBuffer)) != FALSE);

	::FindVolumeClose(hVol);

	if (volumeName.size() == 0)
	{
		std::wcout << "Unable to locate a volume on this device" << std::endl;
		return 1;
	}

	std::wcout << "Device: " << physicalDriveName << std::endl;
	std::wcout << "Volume: " << volumeName << std::endl;


	std::wcout << "\n\nSuccess! Now create the file!" << std::endl;

	// Now let's create a file for fits and giggles
	std::ofstream   output;

	output.open(volumeName + L"hello.txt");
	if (output.fail())
	{
		std::wcout << "Unable to open output file." << std::endl;
		return 1;
	}
	output.close();

	// Volume will be unmounted when the application closes
	system("pause");

	return 0;
}

