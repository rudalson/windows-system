// EnumeratingVolumeGUIDPaths.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>

#define BUFSIZE MAX_PATH

int _tmain(int argc, _TCHAR* argv[])
{
	BOOL bFlag;
	TCHAR Buf[BUFSIZE];           // temporary buffer for volume name
	TCHAR Drive[] = TEXT("c:\\"); // template drive specifier
	TCHAR I;                      // generic loop counter

	// Walk through legal drive letters, skipping floppies.
	for (I = TEXT('c'); I < TEXT('z'); I++)
	{
		// Stamp the drive for the appropriate letter.
		Drive[0] = I;

		bFlag = GetVolumeNameForVolumeMountPoint(
			Drive,     // input volume mount point or directory
			Buf,       // output volume name buffer
			BUFSIZE); // size of volume name buffer

		if (bFlag)
		{
			_tprintf(TEXT("The ID of drive \"%s\" is \"%s\"\n"), Drive, Buf);
		}
	}

	return 0;
}

