#define _WIN32_WINNT 0x0501

#include "stdafx.h"
#include <windows.h>

#define BUFSIZE MAX_PATH

int _tmain(int argc, _TCHAR* argv[])
{
	BOOL bFlag;
	TCHAR Buf[BUFSIZE];     // temporary buffer for volume name

	if (argc != 3)
	{
		_tprintf(TEXT("Usage: %s <mount_point> <volume>\n"), argv[0]);
		_tprintf(TEXT("For example, \"%s c:\\mnt\\fdrive\\ f:\\\"\n"), argv[0]);
		return(-1);
	}

	// We should do some error checking on the inputs. Make sure there 
	// are colons and backslashes in the right places, and so on 

	bFlag = GetVolumeNameForVolumeMountPoint(
		argv[2], // input volume mount point or directory
		Buf, // output volume name buffer
		BUFSIZE  // size of volume name buffer
		);

	if (bFlag != TRUE)
	{
		_tprintf(TEXT("Retrieving volume name for %s failed.\n"), argv[2]);
		return (-2);
	}

	_tprintf(TEXT("Volume name of %s is %s\n"), argv[2], Buf);
	bFlag = SetVolumeMountPoint(
		argv[1], // mount point
		Buf  // volume to be mounted
		);

	if (!bFlag)
		_tprintf(TEXT("Attempt to mount %s at %s failed.\n"), argv[2], argv[1]);

	return (bFlag);
}

