#ifndef __BADDISK_H__
#define __BADDISK_H__

#include <ntddk.h>

#define DEVICE_NAME         L"\\Device\\VOLMJHK"
#define DOS_DEVICE_NAME     L"\\DosDevices\\VOLMJHK"

#define MAX_DEVICE_NAME     256

#define DBG_INFO(_m_, ...)  DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_INFO_LEVEL, "[VOLMJHK] INFO [%s|0x%p] "##_m_, __FUNCTION__, KeGetCurrentThread(), __VA_ARGS__)

typedef struct _DEVICE_EXTENSION
{
    DEVICE_OBJECT DeviceObject;

    PDRIVER_OBJECT DriverObject;
    PDRIVER_OBJECT TargetDriverObject;

    LIST_ENTRY  VolumeListHead;
    LIST_ENTRY  HookedListHead;

    EX_SPIN_LOCK  HookedSpinLock;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct _VOLUME_ELEMENT
{
    LIST_ENTRY  VolumeEntry;
    LIST_ENTRY  HookEntry;

    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING DeviceName;
    WCHAR DeviceNameBuffer[64];
} VOLUME_ELEMENT, *PVOLUME_ELEMENT;


#if defined(_WIN64)
extern POBJECT_TYPE NTSYSAPI *IoDriverObjectType;
#else
extern POBJECT_TYPE *IoDriverObjectType;
#endif

NTKERNELAPI
NTSTATUS
ObReferenceObjectByName(
    IN PUNICODE_STRING  ObjectName,
    IN ULONG            Attributes,
    IN PACCESS_STATE    PassedAccessState OPTIONAL,
    IN ACCESS_MASK      DesiredAccess OPTIONAL,
    IN POBJECT_TYPE     ObjectType OPTIONAL,
    IN KPROCESSOR_MODE  AccessMode,
    IN OUT PVOID        ParseContext OPTIONAL,
    OUT PVOID           *Object
);

NTSTATUS VOLMJHKDispatchRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
NTSTATUS VOLMJHKOpenClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS DeviceInit(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID OnUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS DriverFilter(
    IN DRIVER_OBJECT *old_DriverObject,
    IN BOOLEAN b_hook
    );

NTSTATUS	VOLMJHKDriverDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP irp
    );

NTSTATUS
_GetDeviceName(
    __in PDEVICE_OBJECT DeviceObject,
    __out PWCHAR Buffer,
    __in ULONG BufferLength);

int
_makeupVolumeList(
    __in PDEVICE_OBJECT DeviceObject);

int
_cleanupVolumeList();
#endif __BADDISK_H__