#pragma once

#define _NO_CRT_STDIO_INLINE

#include "defs.hpp"
#include "xor.hpp"
#include "stringapi.hpp"
#include "utils.hpp"
#include "routines.hpp"

NTSTATUS HookedCreateDispatch(_In_ struct _DEVICE_OBJECT* device_object, _Inout_ struct _IRP* irp) {
	AnsiString DriverName;
	if (device_object->DriverObject->DriverName.Buffer) {
		WideString wStr(device_object->DriverObject->DriverName.Buffer);
		DriverName = wStr.ToLowerCase().GetAnsi();
	}

	if (DriverName.Matches(XorString("*disk*"))) {
		return disk_original_create_dispatch(device_object, irp);
	}
	else if (DriverName.Matches(XorString("*partmgr*"))) {
		return partmgr_original_create_dispatch(device_object, irp);
	}
	else if (DriverName.Matches(XorString("*nsiproxy*"))) {
		return nsi_original_create_dispatch(device_object, irp);
	}
}

NTSTATUS HookedCloseDispatch(_In_ struct _DEVICE_OBJECT* device_object, _Inout_ struct _IRP* irp) {

	AnsiString driverName;
	if (device_object->DriverObject->DriverName.Buffer) {
		WideString wStr(device_object->DriverObject->DriverName.Buffer);
		driverName = wStr.ToLowerCase().GetAnsi();
	}

	if (driverName.Matches(XorString("*disk*"))) {
		return disk_original_close_dispatch(device_object, irp);
	}
	else if (driverName.Matches(XorString("*partmgr*"))) {
		return partmgr_original_close_dispatch(device_object, irp);
	}
	else if (driverName.Matches(XorString("*nsiproxy*"))) {
		return nsi_original_close_dispatch(device_object, irp);
	}
}

NTSTATUS HookedDeviceControlDispatch(PDEVICE_OBJECT device_object, PIRP irp) {
	const auto stack = IoGetCurrentIrpStackLocation(irp);
	auto* request = (CustomRequest_Struct*)irp->AssociatedIrp.SystemBuffer;

	AnsiString driverName;
	if (device_object->DriverObject->DriverName.Buffer) {
		WideString WSdrvname(device_object->DriverObject->DriverName.Buffer);
		driverName = WSdrvname.ToLowerCase().GetAnsi();
	}

	switch (stack->Parameters.DeviceIoControl.IoControlCode) {
	case initiatespoof_code:
		spoof_initiated = true;
		break;

		if (spoof_initiated) {
	case IOCTL_STORAGE_QUERY_PROPERTY:
		Utils::SetCompletionRoutine(irp, stack, &HandleStorage);
		break;
	case SMART_RCV_DRIVE_DATA:
		Utils::SetCompletionRoutine(irp, stack, &HandleSmart);
		break;
	case IOCTL_ATA_PASS_THROUGH:
		Utils::SetCompletionRoutine(irp, stack, &HandleATA);
		break;
	case IOCTL_DISK_GET_DRIVE_GEOMETRY:
		Utils::SetCompletionRoutine(irp, stack, &HandleDriveGeometry);
		break;
	case IOCTL_DISK_GET_DRIVE_GEOMETRY_EX:
		Utils::SetCompletionRoutine(irp, stack, &HandleDriveGeometryEx);
		break;
	case IOCTL_DISK_GET_DRIVE_LAYOUT:
		Utils::SetCompletionRoutine(irp, stack, &HandleDriveLayout);
		break;
	case IOCTL_DISK_GET_DRIVE_LAYOUT_EX:
		Utils::SetCompletionRoutine(irp, stack, &HandleDriveLayoutEx);
		break;
	case IOCTL_NSI_PROXY_ARP:
		Utils::SetCompletionRoutine(irp, stack, &HandleARP);
		break;
	case nullserial_code:
		const_change_serial = false;
		use_custom_serial = false;
		use_random_serial = false;
		null_serial = true;
		break;
	case randomizeserial_code:
		Utils::RandomString(spoofed_serial, 15);
		null_serial = false;
		const_change_serial = false;
		use_custom_serial = false;
		use_random_serial = true;
		break;
	case customserial_code:
		if (!request->customserial)
			break;
		strcpy(spoofed_serial, request->customserial);
		null_serial = false;
		use_random_serial = false;
		const_change_serial = false;
		use_custom_serial = true;
		break;
	case changeserialoneveryreq_code:
		null_serial = false;
		use_random_serial = false;
		use_custom_serial = false;
		const_change_serial = true;
		break;
		}
	}

	if (driverName.Matches(XorString("*disk*"))) {
		return disk_original_device_control(device_object, irp);
	}
	else if (driverName.Matches(XorString("*partmgr*"))) {
		return partmgr_original_device_control(device_object, irp);
	}
	else if (driverName.Matches(XorString("*nsiproxy*"))) {
		return nsi_original_device_control(device_object, irp);
	}
}

NTSTATUS SMBIOS_SpoofInit() {
	auto* ntoskrnl_base = Utils::ReturnModuleBase(XorString("ntoskrnl.exe"));
	if (!ntoskrnl_base)
		return STATUS_UNSUCCESSFUL;

	auto smbiosphysicaladdy = static_cast<PPHYSICAL_ADDRESS>(Utils::FindPatternImage(ntoskrnl_base, XorString("\x48\x8B\x0D\x00\x00\x00\x00\x48\x85\xC9\x74\x00\x8B\x15"), XorString("xxx????xxxx?xx")));
	if (smbiosphysicaladdy) {
		smbiosphysicaladdy = reinterpret_cast<PPHYSICAL_ADDRESS>(reinterpret_cast<char*>(smbiosphysicaladdy) + 7 + *reinterpret_cast<int*>(reinterpret_cast<char*>(smbiosphysicaladdy) + 3));
		memset(smbiosphysicaladdy, 0, sizeof(PHYSICAL_ADDRESS));
	}
	else
		return STATUS_UNSUCCESSFUL;

	return STATUS_SUCCESS;
}

NTSTATUS StartMainHooking() {
	// get disk
	UNICODE_STRING diskName = RTL_CONSTANT_STRING(L"\\Driver\\disk"); PDRIVER_OBJECT diskObject = nullptr;
	NTSTATUS diskStatus = ObReferenceObjectByName(&diskName, OBJ_CASE_INSENSITIVE, nullptr, 0, *IoDriverObjectType, KernelMode, 0, (PVOID*)&diskObject);
	if (!NT_SUCCESS(diskStatus))
		return STATUS_UNSUCCESSFUL;

	// get partmgr
	UNICODE_STRING partmgrName = RTL_CONSTANT_STRING(L"\\Driver\\partmgr"); PDRIVER_OBJECT partmgrObject = nullptr;
	NTSTATUS partmgrStatus = ObReferenceObjectByName(&partmgrName, OBJ_CASE_INSENSITIVE, nullptr, 0, *IoDriverObjectType, KernelMode, 0, (PVOID*)&partmgrObject);
	if (!NT_SUCCESS(partmgrStatus))
		return STATUS_UNSUCCESSFUL;

	UNICODE_STRING nsiName = RTL_CONSTANT_STRING(L"\\Driver\\nsiproxy"); PDRIVER_OBJECT nsiObject = nullptr;
	NTSTATUS nsiStatus = ObReferenceObjectByName(&nsiName, OBJ_CASE_INSENSITIVE, nullptr, 0, *IoDriverObjectType, KernelMode, 0, (PVOID*)&nsiObject);
	if (!NT_SUCCESS(nsiStatus))
		return STATUS_UNSUCCESSFUL;

	// get other driver
	UNICODE_STRING driverName = RTL_CONSTANT_STRING(L"\\Device\\HwRwDrv"); 
	PDRIVER_OBJECT other_driver_object; PDEVICE_OBJECT deviceObject = Utils::GetDeviceObject(driverName);
	if (deviceObject == NULL)
		return STATUS_UNSUCCESSFUL;
	else
		other_driver_object = deviceObject->DriverObject;

	// swap disk attributes...
	*(PVOID*)&other_driver_original_device_control = _InterlockedExchangePointer((volatile PVOID*)&other_driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL], HookedDeviceControlDispatch);
	*(PVOID*)&disk_original_device_control = _InterlockedExchangePointer((volatile PVOID*)&diskObject->MajorFunction[IRP_MJ_DEVICE_CONTROL], other_driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL]);
	*(PVOID*)&other_driver_original_create_dispatch = _InterlockedExchangePointer((volatile PVOID*)&other_driver_object->MajorFunction[IRP_MJ_CREATE], HookedCreateDispatch);
	*(PVOID*)&disk_original_create_dispatch = _InterlockedExchangePointer((volatile PVOID*)&diskObject->MajorFunction[IRP_MJ_CREATE], other_driver_object->MajorFunction[IRP_MJ_CREATE]);
	*(PVOID*)&other_driver_original_close_dispatch = _InterlockedExchangePointer((volatile PVOID*)&other_driver_object->MajorFunction[IRP_MJ_CLOSE], HookedCloseDispatch);
	*(PVOID*)&disk_original_close_dispatch = _InterlockedExchangePointer((volatile PVOID*)&diskObject->MajorFunction[IRP_MJ_CLOSE], other_driver_object->MajorFunction[IRP_MJ_CLOSE]);
	diskObject->DriverStart = other_driver_object->DriverStart;
	diskObject->DriverSection = other_driver_object->DriverSection;
	diskObject->DeviceObject = other_driver_object->DeviceObject;
	diskObject->DriverExtension = other_driver_object->DriverExtension;
	diskObject->DriverInit = other_driver_object->DriverInit;
	diskObject->DriverSize = other_driver_object->DriverSize;
	diskObject->FastIoDispatch = 0;
	diskObject->DriverStartIo = 0;
	diskObject->DriverUnload = 0;

	// swap partmgr attributes...
	*(PVOID*)&partmgr_original_device_control = _InterlockedExchangePointer((volatile PVOID*)&partmgrObject->MajorFunction[IRP_MJ_DEVICE_CONTROL], other_driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL]);
	*(PVOID*)&partmgr_original_create_dispatch = _InterlockedExchangePointer((volatile PVOID*)&partmgrObject->MajorFunction[IRP_MJ_CREATE], other_driver_object->MajorFunction[IRP_MJ_CREATE]);
	*(PVOID*)&partmgr_original_close_dispatch = _InterlockedExchangePointer((volatile PVOID*)&partmgrObject->MajorFunction[IRP_MJ_CLOSE], other_driver_object->MajorFunction[IRP_MJ_CLOSE]);
	partmgrObject->DriverStart = other_driver_object->DriverStart;
	partmgrObject->DriverSection = other_driver_object->DriverSection;
	partmgrObject->DeviceObject = other_driver_object->DeviceObject;
	partmgrObject->DriverExtension = other_driver_object->DriverExtension;
	partmgrObject->DriverInit = other_driver_object->DriverInit;
	partmgrObject->DriverSize = other_driver_object->DriverSize;
	partmgrObject->FastIoDispatch = 0;
	partmgrObject->DriverStartIo = 0;
	partmgrObject->DriverUnload = 0;

	// swap nsiproxy attributes...
	*(PVOID*)&nsi_original_device_control = _InterlockedExchangePointer((volatile PVOID*)&nsiObject->MajorFunction[IRP_MJ_DEVICE_CONTROL], other_driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL]);
	*(PVOID*)&nsi_original_create_dispatch = _InterlockedExchangePointer((volatile PVOID*)&nsiObject->MajorFunction[IRP_MJ_CREATE], other_driver_object->MajorFunction[IRP_MJ_CREATE]);
	*(PVOID*)&nsi_original_close_dispatch = _InterlockedExchangePointer((volatile PVOID*)&nsiObject->MajorFunction[IRP_MJ_CLOSE], other_driver_object->MajorFunction[IRP_MJ_CLOSE]);
	nsiObject->DriverStart = other_driver_object->DriverStart;
	nsiObject->DriverSection = other_driver_object->DriverSection;
	nsiObject->DeviceObject = other_driver_object->DeviceObject;
	nsiObject->DriverExtension = other_driver_object->DriverExtension;
	nsiObject->DriverInit = other_driver_object->DriverInit;
	nsiObject->DriverSize = other_driver_object->DriverSize;
	nsiObject->FastIoDispatch = 0;
	nsiObject->DriverStartIo = 0;
	nsiObject->DriverUnload = 0;

	return STATUS_SUCCESS;
}

NTSTATUS EntryPoint() {
	if (!NT_SUCCESS(SMBIOS_SpoofInit()))
		return STATUS_UNSUCCESSFUL;

	if (!NT_SUCCESS(StartMainHooking()))
		return STATUS_UNSUCCESSFUL;

	Utils::RandomString(spoofed_serial, 15);

	return STATUS_SUCCESS;
}