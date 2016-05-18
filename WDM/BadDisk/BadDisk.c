///////////////////////////////////////////////////////////////////////////
//				VOLMJ Hook Filter Driver (Win2K)
//					2015.5.18.
//					sekim@mantech.co.kr
///////////////////////////////////////////////////////////////////////////

#include <Ntifs.h>

#include "BadDisk.h"
#include "ctl_code.h"

//
// Device driver routine declarations.
//
DRIVER_INITIALIZE   DriverEntry;
PDEVICE_EXTENSION    gDeviceExtension;

DRIVER_OBJECT g_old_DriverObject;
PFAST_IO_DEVICE_CONTROL old_FastIoDeviceControl;

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry )
#endif ALLOC_PRAGMA

NTSTATUS DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    // Create device
    status = DeviceInit(DriverObject, RegistryPath);

    if (status != STATUS_SUCCESS)
    {
        DBG_INFO("DeviceInit status : 0x%x\n", status);
        goto FAIL;
    }

    status = DriverFilter(&g_old_DriverObject, TRUE);

    return status;

FAIL:
    if (status != STATUS_SUCCESS)
    {
    }

    return status;
}

NTSTATUS DeviceInit(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    UNREFERENCED_PARAMETER(RegistryPath);

    UNICODE_STRING	DeviceName;
    UNICODE_STRING	SymbolicLinkName;

    NTSTATUS		Status = STATUS_SUCCESS;

    // Initialize device name string
    RtlInitUnicodeString(&DeviceName, DEVICE_NAME);

    // Create new device
    Status = IoCreateDevice(
        DriverObject,
        sizeof(DEVICE_EXTENSION),
        &DeviceName,
        FILE_DEVICE_NETWORK,
        0,
        TRUE,
        &(PDEVICE_OBJECT)gDeviceExtension
        );

    if (Status == STATUS_SUCCESS)
    {
        // Create symbolic link for device
        RtlInitUnicodeString(&SymbolicLinkName, DOS_DEVICE_NAME);
        IoCreateSymbolicLink(&SymbolicLinkName, &DeviceName);

        DriverObject->MajorFunction[IRP_MJ_CREATE] = VOLMJHKOpenClose;
        DriverObject->MajorFunction[IRP_MJ_CLOSE] = VOLMJHKOpenClose;
        DriverObject->MajorFunction[IRP_MJ_CLEANUP] = VOLMJHKOpenClose;
        DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = VOLMJHKDispatchRequest;

        DriverObject->DriverUnload = OnUnload;

        //
        gDeviceExtension->DriverObject = DriverObject;
        InitializeListHead(&gDeviceExtension->VolumeListHead);
        InitializeListHead(&gDeviceExtension->HookedListHead);
        gDeviceExtension->HookedSpinLock = 0;
    }

    DBG_INFO("DriverObject(0x%p)\n", DriverObject);

    return Status;
}

NTSTATUS DriverFilter(DRIVER_OBJECT *old_DriverObject, BOOLEAN b_hook)
{
    UNICODE_STRING drv_name;
    NTSTATUS status;
    PDRIVER_OBJECT new_DriverObject;
    int i;

    RtlInitUnicodeString(&drv_name, L"\\Driver\\volmgr");

    status = ObReferenceObjectByName(&drv_name, OBJ_CASE_INSENSITIVE, (ULONG)NULL, 0,
        *IoDriverObjectType, KernelMode, (ULONG)NULL, &new_DriverObject);
    if (status != STATUS_SUCCESS) {
        DBG_INFO("ObReferenceObjectByName fail\n");
        return status;
    }

    if (IsListEmpty(&gDeviceExtension->VolumeListHead))
    {
        _makeupVolumeList(new_DriverObject->DeviceObject);
    }

    for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
    {
        if (b_hook) {
            old_DriverObject->MajorFunction[i] = new_DriverObject->MajorFunction[i];
            new_DriverObject->MajorFunction[i] = VOLMJHKDriverDispatch;
        }
        else
            new_DriverObject->MajorFunction[i] = old_DriverObject->MajorFunction[i];
    }

    ObDereferenceObject(new_DriverObject);

    return STATUS_SUCCESS;
}

NTSTATUS VOLMJHKDispatchRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    //UNREFERENCED_PARAMETER(DeviceObject);

    PAGED_CODE();

    ULONG inBufLength; // Input buffer length
    PCHAR inBuf;
    NTSTATUS status = STATUS_SUCCESS;
    
    PIO_STACK_LOCATION isl = IoGetCurrentIrpStackLocation(Irp);

    switch (isl->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_VOLMJHK_HOOKLIST:
        {
            ULONG offset = 0;
            DBG_INFO("IOCTL_VOLMJHK_HOOKLIST DeviceObject(0x%p)\n", DeviceObject);

            if (!IsListEmpty(&gDeviceExtension->HookedListHead))
            {
                PCHAR outbuf = Irp->AssociatedIrp.SystemBuffer;
                //ULONG outbuf_length = isl->Parameters.DeviceIoControl.OutputBufferLength;

                for (PLIST_ENTRY entry = gDeviceExtension->HookedListHead.Flink;
                    entry != &gDeviceExtension->HookedListHead;
                    entry = entry->Flink)
                {
                    PVOLUME_ELEMENT volume = (PVOLUME_ELEMENT)CONTAINING_RECORD(entry, VOLUME_ELEMENT, HookEntry);
                    RtlCopyMemory(outbuf + offset, volume->DeviceName.Buffer, 64 * sizeof(WCHAR));
                    offset += 64 * sizeof(WCHAR);
                }
                // 범위를 벗어나는 에러처리는 ... skip
            }

            Irp->IoStatus.Information = offset;
            break;
        }

        case IOCTL_VOLMJHK_HOOK:
        {
            inBufLength = isl->Parameters.DeviceIoControl.InputBufferLength;
            inBuf = Irp->AssociatedIrp.SystemBuffer;

            UNICODE_STRING volume_name;
            RtlInitUnicodeString(&volume_name, (PCWSTR)inBuf);

            for (PLIST_ENTRY entry = gDeviceExtension->VolumeListHead.Flink;
                entry != &gDeviceExtension->VolumeListHead;
                entry = entry->Flink)
            {
                PVOLUME_ELEMENT volume = (PVOLUME_ELEMENT)CONTAINING_RECORD(entry, VOLUME_ELEMENT, VolumeEntry);

                if (!RtlCompareUnicodeString(&volume_name, &volume->DeviceName, FALSE))
                {
                    DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_INFO_LEVEL, "****************************\n");
                    DBG_INFO("*** Hooked. DeviceObject(0x%p) DeviceName(%wZ)\n****************************\n", volume->DeviceObject, volume->DeviceName);
                    KIRQL prev_irql = ExAcquireSpinLockExclusive(&gDeviceExtension->HookedSpinLock);
                    InsertTailList(&gDeviceExtension->HookedListHead, &volume->HookEntry);
                    ExReleaseSpinLockExclusive(&gDeviceExtension->HookedSpinLock, prev_irql);
                    break;
                }
            }

            Irp->IoStatus.Information = 0;
            break;
        }

        case IOCTL_VOLMJHK_UNHOOK:
        {
            inBufLength = isl->Parameters.DeviceIoControl.InputBufferLength;
            inBuf = Irp->AssociatedIrp.SystemBuffer;

            UNICODE_STRING volume_name;
            RtlInitUnicodeString(&volume_name, (PCWSTR)inBuf);

            //KIRQL prev_irql = ExAcquireSpinLockShared(&gDeviceExtension->HookedSpinLock);
            if (IsListEmpty(&gDeviceExtension->HookedListHead))
            {
                //ExReleaseSpinLockShared(&gDeviceExtension->HookedSpinLock, prev_irql);
                DBG_INFO("No hooked volume\n");
                break;
            }

            //ExTryConvertSharedSpinLockExclusive(&gDeviceExtension->HookedSpinLock);

            for (PLIST_ENTRY entry = gDeviceExtension->HookedListHead.Flink;
                entry != &gDeviceExtension->HookedListHead;
                entry = entry->Flink)
            {
                PVOLUME_ELEMENT volume = (PVOLUME_ELEMENT)CONTAINING_RECORD(entry, VOLUME_ELEMENT, HookEntry);

                if (!RtlCompareUnicodeString(&volume_name, &volume->DeviceName, FALSE))
                {
                    DBG_INFO("Unhooked. DeviceObject(0x%p) DeviceName(%wZ)\n", volume->DeviceObject, volume->DeviceName);
                    RemoveEntryList(entry);
                    break;
                }
            }

            //ExReleaseSpinLockExclusive(&gDeviceExtension->HookedSpinLock, prev_irql);
            Irp->IoStatus.Information = 0;
            break;
        }
    }

    Irp->IoStatus.Status = status;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}

NTSTATUS VOLMJHKOpenClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

VOID OnUnload(
    IN PDRIVER_OBJECT DriverObject
    )
{
    NTSTATUS status;
    UNICODE_STRING	SymbolicLinkName;

    status = DriverFilter(&g_old_DriverObject, FALSE);

    int count = _cleanupVolumeList();
    DBG_INFO("%d volumes were released\n", count);

    RtlInitUnicodeString(&SymbolicLinkName, DEVICE_NAME);
    IoDeleteSymbolicLink(&SymbolicLinkName);

    IoDeleteDevice(DriverObject->DeviceObject);
}

int
_makeupVolumeList(__in PDEVICE_OBJECT DeviceObject)
{
    int count = 0;

    for (PDEVICE_OBJECT device = DeviceObject;
        device->NextDevice;
        device = device->NextDevice)
    {
        PVOLUME_ELEMENT volume = (PVOLUME_ELEMENT)ExAllocatePoolWithTag(NonPagedPool, MAX_DEVICE_NAME*sizeof(WCHAR), '2DAB');

        volume->DeviceObject = device;
        memset(volume->DeviceNameBuffer, 0, sizeof(volume->DeviceNameBuffer));
        _GetDeviceName(device, volume->DeviceNameBuffer, 64);
        RtlInitUnicodeString(&volume->DeviceName, volume->DeviceNameBuffer);

        InsertTailList(&gDeviceExtension->VolumeListHead, &volume->VolumeEntry);
        ++count;
        DBG_INFO("DeviceObject(0x%p) DeviceName(%wZ)\n", volume->DeviceObject, volume->DeviceName);
    }

    return count;
}

int
_cleanupVolumeList()
{
    int count = 0;
    PLIST_ENTRY volume_list = &gDeviceExtension->VolumeListHead;

    for (PLIST_ENTRY entry = RemoveHeadList(volume_list);
        entry != volume_list;
        entry = RemoveHeadList(volume_list))
    {
        PVOLUME_ELEMENT volume = (PVOLUME_ELEMENT)CONTAINING_RECORD(entry, VOLUME_ELEMENT, VolumeEntry);
        //DBG_INFO("cleanup DeviceName(%wZ)\n", volume->DeviceName);
        ExFreePool(volume);
        ++count;
    }

    return count;
}

BOOLEAN
_IsHookedDevice(__in PDEVICE_OBJECT DeviceObject)
{
    //KIRQL prev_irql = ExAcquireSpinLockShared(&gDeviceExtension->HookedSpinLock);

    if (IsListEmpty(&gDeviceExtension->HookedListHead))
    {
        //ExReleaseSpinLockShared(&gDeviceExtension->HookedSpinLock, prev_irql);
        return FALSE;
    }

    BOOLEAN found = FALSE;

    for (PLIST_ENTRY entry = gDeviceExtension->HookedListHead.Flink;
        entry != &gDeviceExtension->HookedListHead;
        entry = entry->Flink)
    {
        PVOLUME_ELEMENT volume = (PVOLUME_ELEMENT)CONTAINING_RECORD(entry, VOLUME_ELEMENT, HookEntry);

        if (DeviceObject == volume->DeviceObject)
        {
            found = TRUE;
            break;
        }
    }

    //ExReleaseSpinLockShared(&gDeviceExtension->HookedSpinLock, prev_irql);

    return found;
}


NTSTATUS
VOLMJHKDriverDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP irp)
{
    // sanity check
    if (irp == NULL) {
        DBG_INFO("irp is null\n");
        return STATUS_SUCCESS;
    }

    PIO_STACK_LOCATION isl = IoGetCurrentIrpStackLocation(irp);

    // whether object is hooking
    if (_IsHookedDevice(DeviceObject))
    {
        // Analyze MajorFunction
        switch (isl->MajorFunction)
        {
            case IRP_MJ_CREATE:
                DBG_INFO("IRP_MJ_CREATE. DeviceObject(0x%p)\n", DeviceObject);
                break;
            case IRP_MJ_CLEANUP:
                DBG_INFO("IRP_MJ_CLEANUP. DeviceObject(0x%p)\n", DeviceObject);
                break;
            case IRP_MJ_CLOSE:
                DBG_INFO("IRP_MJ_CLOSE. DeviceObject(0x%p)\n", DeviceObject);
                break;
            case IRP_MJ_INTERNAL_DEVICE_CONTROL:
                DBG_INFO("IRP_MJ_INTERNAL_DEVICE_CONTROL. DeviceObject(0x%p)\n", DeviceObject);
                break;
            case IRP_MJ_READ:
                //DBG_INFO("IRP_MJ_READ. DeviceObject(0x%p)\n", DeviceObject);
                break;
            case IRP_MJ_WRITE:
                //DBG_INFO("IRP_MJ_WRITE. DeviceObject(0x%p)\n", DeviceObject);
                break;
            case IRP_MJ_DEVICE_CONTROL:
                DBG_INFO("IRP_MJ_DEVICE_CONTROL. DeviceObject(0x%p)\n", DeviceObject);
                break;

            default:
                DBG_INFO("major(0x%x), minor(0x%x) FileObject(0x%p)\n", isl->MajorFunction, isl->MinorFunction, isl->FileObject);
                break;
        }

        irp->IoStatus.Status = STATUS_DISK_CORRUPT_ERROR;
        irp->IoStatus.Information = 0;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return STATUS_UNSUCCESSFUL;
    }

    return g_old_DriverObject.MajorFunction[isl->MajorFunction](DeviceObject, irp);
}

NTSTATUS
_GetDeviceName(PDEVICE_OBJECT DeviceObject, PWCHAR Buffer, ULONG BufferLength)
{
    NTSTATUS					status;
    POBJECT_NAME_INFORMATION	nameInfo = NULL;
    ULONG						size;

    nameInfo = (POBJECT_NAME_INFORMATION)ExAllocatePoolWithTag(NonPagedPool, MAX_DEVICE_NAME * sizeof(WCHAR), '26DW');
    if (!nameInfo)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(nameInfo, MAX_DEVICE_NAME * sizeof(WCHAR));
    status = ObQueryNameString(DeviceObject, nameInfo, MAX_DEVICE_NAME, &size);
    if (!NT_SUCCESS(status))
    {
        DBG_INFO("cannot get device name, err=0x%x\n", status);
        ExFreePool(nameInfo);
        return status;
    }

    if (BufferLength > nameInfo->Name.Length)
    {
        memcpy(Buffer, nameInfo->Name.Buffer, nameInfo->Name.Length);
    }
    else
    {
        memcpy(Buffer, nameInfo->Name.Buffer, BufferLength - 4);
    }

    ExFreePool(nameInfo);
    
    return STATUS_SUCCESS;
}