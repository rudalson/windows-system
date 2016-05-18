#ifndef __MAIN_H__
#define __MAIN_H__

#define DEVICE_NAME "\\\\.\\VOLMJHK"

extern HANDLE OpenDevice(const char * device);
extern DWORD showDeviceList();
extern DWORD showHookList();
extern DWORD hookDevice(PCHAR letter);
extern DWORD unhookDevice(PCHAR letter);
#endif __MAIN_H__