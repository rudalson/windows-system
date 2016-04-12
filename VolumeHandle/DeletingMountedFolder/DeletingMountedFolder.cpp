#define _WIN32_WINNT 0x0501

#include "stdafx.h"
#include <windows.h>

void
Syntax(TCHAR *argv)
{
	_tprintf(TEXT("%s unmounts a volume from a volume mount point\n"),
		argv);
	_tprintf(TEXT("For example: \"%s c:\\mnt\\fdrive\\\"\n"), argv);
}

int _tmain(int argc, _TCHAR* argv[])
{
	BOOL bFlag;

	if (argc != 2)
	{
		Syntax(argv[0]);
		return (-1);
	}

	// We should do some error checking on the path argument, such as
	// ensuring that there is a trailing backslash.

	bFlag = DeleteVolumeMountPoint(
		argv[1] // Path of the volume mount point
		);

	_tprintf(TEXT("\n%s %s in unmounting the volume at %s\n"), argv[0],
		bFlag ? TEXT("succeeded") : TEXT("failed"), argv[1]);

	return (bFlag);
}

