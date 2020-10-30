#pragma once

// https://docs.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-ioctl_storage_query_property
NTSTATUS HandleStorage(PDEVICE_OBJECT device_object, PIRP irp, PVOID context) {
	if (!context)
		return STATUS_SUCCESS;

	const auto request = (REQUEST_STRUCT*)context;
	ExFreePool(context);

	if (request->OutputBufferLength >= sizeof(STORAGE_DEVICE_DESCRIPTOR)) {
		const auto buffer = (PSTORAGE_DEVICE_DESCRIPTOR)request->SystemBuffer;

		if (buffer->SerialNumberOffset > 0 && buffer->SerialNumberOffset < request->OutputBufferLength) {
			auto serialNumber = (char*)buffer + buffer->SerialNumberOffset;
			if (null_serial) {
				memset(serialNumber, 0, strlen(serialNumber));
			}
			else if (use_random_serial || use_custom_serial) {
				strcpy(serialNumber, spoofed_serial);
			}
			else if (const_change_serial) {
				Utils::RandomString(spoofed_serial, 15);
				strcpy(serialNumber, spoofed_serial);
			}
		}
		if (buffer->ProductRevisionOffset > 0 && buffer->ProductRevisionOffset < request->OutputBufferLength) {
			auto diskdriveProductRevision = (char*)buffer + buffer->ProductRevisionOffset;
			memset(diskdriveProductRevision, 0, strlen(diskdriveProductRevision));
		}
		if (buffer->BusType) {
			auto diskdriveBusType = (STORAGE_BUS_TYPE*)buffer + buffer->BusType;
			memset(diskdriveBusType, 0, sizeof(diskdriveBusType));
		}
		if (buffer->DeviceType) {
			auto diskdriveDeviceType = (UCHAR*)buffer + buffer->DeviceType;
			memset(diskdriveDeviceType, 0, sizeof(diskdriveDeviceType));
		}
		if (buffer->Version > 0) {
			auto diskdriveVersion = (char*)buffer + buffer->Version;
			memset(diskdriveVersion, 0, strlen(diskdriveVersion));
		}
		if (buffer->ProductIdOffset > 0 && buffer->ProductIdOffset < request->OutputBufferLength) {
			auto diskdriveProductId = (char*)buffer + buffer->ProductIdOffset;
			memset(diskdriveProductId, 0, strlen(diskdriveProductId));
		}
		if (buffer->VendorIdOffset > 0 && buffer->VendorIdOffset < request->OutputBufferLength) {
			auto diskdriveVendorId = (char*)buffer + buffer->VendorIdOffset;
			memset(diskdriveVendorId, 0, strlen(diskdriveVendorId));
		}
	}

	if (request->OldRoutine && irp->StackCount > 1)
		return request->OldRoutine(device_object, irp, request->OldContext);

	return STATUS_SUCCESS;
}

// https://docs.microsoft.com/en-us/previous-versions/windows/hardware/drivers/ff566204(v=vs.85)
NTSTATUS HandleSmart(PDEVICE_OBJECT device, PIRP irp, PVOID context) {
	UNREFERENCED_PARAMETER(device); UNREFERENCED_PARAMETER(irp);
	if (!context)
		return STATUS_SUCCESS;

	const auto request = (REQUEST_STRUCT*)context;
	ExFreePool(context);

	if (request->OutputBufferLength >= (sizeof(SENDCMDOUTPARAMS) - 1 + 512)) {
		const auto buffer = (SENDCMDOUTPARAMS*)request->SystemBuffer;
		const auto info = (_IDENTIFY_DATA*)buffer->bBuffer;
		if (info) {
			if (info->SerialNumber) {
				if (null_serial) {
					memset(info->SerialNumber, 0, sizeof(info->SerialNumber));
				}
				else if (use_random_serial || use_custom_serial) {
					strcpy((char*)info->SerialNumber, spoofed_serial);
				}
				else if (const_change_serial) {
					Utils::RandomString(spoofed_serial, 15);
					strcpy((char*)info->SerialNumber, spoofed_serial);
				}
			}

			if (info->FirmwareRevision)
				memset(info->FirmwareRevision, 0, sizeof(info->FirmwareRevision));

			if (info->ModelNumber)
				memset(info->ModelNumber, 0, sizeof(info->ModelNumber));

			if (info->UnformattedBytesPerSector > 0)
				info->UnformattedBytesPerSector = 0;

			if (info->UnformattedBytesPerTrack > 0)
				info->UnformattedBytesPerTrack = 0;

			if (info->MinorRevision)
				info->MinorRevision = 0;

			if (info->NumberOfCurrentCylinders > 0)
				info->NumberOfCurrentCylinders = 0;

			if (info->NumberOfCurrentHeads > 0)
				info->NumberOfCurrentHeads = 0;

			if (info->NumberOfCylinders > 0)
				info->NumberOfCylinders = 0;

			if (info->NumberOfHeads > 0)
				info->NumberOfHeads = 0;

			if (info->NumberOfEccBytes > 0)
				info->NumberOfEccBytes = 0;

			if (info->SectorsPerTrack > 0)
				info->SectorsPerTrack = 0;

			if (info->CurrentSectorsPerTrack > 0)
				info->CurrentSectorsPerTrack = 0;

			if (info->UserAddressableSectors > 0)
				info->UserAddressableSectors = 0;

			if (info->CurrentSectorCapacity > 0)
				info->CurrentSectorCapacity = 0;

			if (info->BufferSectorSize > 0)
				info->BufferSectorSize = 0;

			if (info->Reserved1 > 0)
				info->Reserved1 = 0;

			if (info->Reserved2 > 0)
				info->Reserved2 = 0;

			if (info->Reserved3 > 0)
				info->Reserved3 = 0;

			if (info->Reserved4 > 0)
				info->Reserved4 = 0;

			if (info->Reserved5 > 0)
				memset(info->Reserved5, 0, sizeof(info->Reserved5));

			if (info->Reserved6 > 0)
				memset(info->Reserved6, 0, sizeof(info->Reserved6));

			if (info->Reserved7 > 0)
				memset(info->Reserved7, 0, sizeof(info->Reserved7));

			if (info->VendorUnique1 > 0)
				memset(info->VendorUnique1, 0, sizeof(info->VendorUnique1));

			if (info->Capabilities > 0)
				info->Capabilities = 0;
		}
	}

	if (request->OldRoutine && irp->StackCount > 1)
		return request->OldRoutine(device, irp, request->OldContext);

	return STATUS_SUCCESS;
}

// https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddscsi/ni-ntddscsi-ioctl_ata_pass_through
NTSTATUS HandleATA(PDEVICE_OBJECT device, PIRP irp, PVOID context) {
	if (!context)
		return STATUS_SUCCESS;

	auto* request = (REQUEST_STRUCT*)context;
	ExFreePool(context);

	ULONG buffsize = sizeof(ATA_PASS_THROUGH_EX) + sizeof(PIDENTIFY_DEVICE_DATA);
	if (request->OutputBufferLength >= buffsize) {
		ULONG databuffer_offset = static_cast<PATA_PASS_THROUGH_EX>(request->SystemBuffer)->DataBufferOffset;
		if (databuffer_offset > 0 && databuffer_offset < request->OutputBufferLength) {
			auto serialnumber = reinterpret_cast<PIDENTIFY_DEVICE_DATA>((char*)request->SystemBuffer + databuffer_offset)->SerialNumber;
			if (null_serial) {
				memset(serialnumber, 0, sizeof(serialnumber));
			}
			else if (use_random_serial || use_custom_serial) {
				strcpy((char*)serialnumber, spoofed_serial);
			}
			else if (const_change_serial) {
				Utils::RandomString(spoofed_serial, 15);
				strcpy((char*)serialnumber, spoofed_serial);
			}
		}
	}

	if (request->OldRoutine && irp->StackCount > 1)
		return request->OldRoutine(device, irp, request->OldContext);

	return STATUS_SUCCESS;
}

// https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/ntdddisk/ni-ntdddisk-ioctl_disk_get_drive_geometry
NTSTATUS HandleDriveGeometry(PDEVICE_OBJECT device, PIRP irp, PVOID context) {
	auto* request = (REQUEST_STRUCT*)context;
	ExFreePool(context);

	if (request->OutputBufferLength >= sizeof(DISK_GEOMETRY)) {
		const auto buffer = (DISK_GEOMETRY*)request->SystemBuffer;

		if (buffer->Cylinders.HighPart > 0l)
			buffer->Cylinders.HighPart = 0l;

		if (buffer->Cylinders.LowPart > 0)
			buffer->Cylinders.LowPart = 0;

		if (buffer->Cylinders.u.HighPart > 0l)
			buffer->Cylinders.u.HighPart = 0l;

		if (buffer->Cylinders.u.LowPart > 0)
			buffer->Cylinders.u.LowPart = 0;

		if (buffer->Cylinders.QuadPart > 0ll)
			buffer->Cylinders.QuadPart = 0ll;

		if (buffer->TracksPerCylinder > 0)
			buffer->TracksPerCylinder = 0;

		if (buffer->SectorsPerTrack > 0)
			buffer->SectorsPerTrack = 0;

		if (buffer->BytesPerSector > 0)
			buffer->BytesPerSector = 0;

		if (buffer->MediaType)
			buffer->MediaType = MEDIA_TYPE::Unknown;
	}

	if (request->OldRoutine && irp->StackCount > 1)
		return request->OldRoutine(device, irp, request->OldContext);

	return STATUS_SUCCESS;
}

// https://docs.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-ioctl_disk_get_drive_geometry_ex
NTSTATUS HandleDriveGeometryEx(PDEVICE_OBJECT device, PIRP irp, PVOID context) {
	auto* request = (REQUEST_STRUCT*)context;
	ExFreePool(context);

	const auto buffer = (DISK_GEOMETRY_EX*)request->SystemBuffer;

	if (buffer->Data)
		memset(buffer->Data, 0, sizeof(buffer->Data));

	if (buffer->DiskSize.QuadPart > 0ll)
		buffer->DiskSize.QuadPart = 0;

	if (buffer->DiskSize.HighPart > 0l)
		buffer->DiskSize.HighPart = 0;

	if (buffer->DiskSize.LowPart > 0)
		buffer->DiskSize.LowPart = 0;

	if (buffer->DiskSize.QuadPart > 0ll)
		buffer->DiskSize.QuadPart = 0ll;

	if (buffer->DiskSize.u.HighPart > 0l)
		buffer->DiskSize.u.HighPart = 0l;

	if (buffer->DiskSize.u.LowPart > 0)
		buffer->DiskSize.u.LowPart = 0;

	if (buffer->Geometry.BytesPerSector > 0)
		buffer->Geometry.BytesPerSector = 0;

	if (buffer->Geometry.Cylinders.QuadPart > 0ll)
		buffer->Geometry.Cylinders.QuadPart = 0ll;

	if (buffer->Geometry.Cylinders.HighPart > 0l)
		buffer->Geometry.Cylinders.HighPart = 0l;

	if (buffer->Geometry.Cylinders.LowPart > 0)
		buffer->Geometry.Cylinders.LowPart = 0;

	if (buffer->Geometry.Cylinders.QuadPart > 0ll)
		buffer->Geometry.Cylinders.QuadPart = 0ll;

	if (buffer->Geometry.Cylinders.u.HighPart > 0l)
		buffer->Geometry.Cylinders.u.HighPart = 0l;

	if (buffer->Geometry.Cylinders.u.LowPart > 0)
		buffer->Geometry.Cylinders.u.LowPart = 0;

	if (buffer->Geometry.MediaType)
		buffer->Geometry.MediaType = MEDIA_TYPE::Unknown;

	if (buffer->Geometry.SectorsPerTrack > 0)
		buffer->Geometry.SectorsPerTrack = 0;

	if (buffer->Geometry.TracksPerCylinder > 0)
		buffer->Geometry.TracksPerCylinder = 0;

	if (request->OldRoutine && irp->StackCount > 1)
		return request->OldRoutine(device, irp, request->OldContext);

	return STATUS_SUCCESS;
}

// https://docs.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-ioctl_disk_get_drive_layout
NTSTATUS HandleDriveLayout(PDEVICE_OBJECT device, PIRP irp, PVOID context) {
	if (!context)
		return STATUS_SUCCESS;
	auto* request = (REQUEST_STRUCT*)context;
	ExFreePool(context);

	if (request->OutputBufferLength >= sizeof(DRIVE_LAYOUT_INFORMATION)) {
		auto* buffer = (DRIVE_LAYOUT_INFORMATION*)request->SystemBuffer;

		if (buffer->PartitionCount > 0)
			buffer->PartitionCount = 0;

		if (buffer->PartitionEntry->HiddenSectors > 0)
			buffer->PartitionEntry->HiddenSectors = 0;

		if (buffer->PartitionEntry->PartitionLength.QuadPart > 0ll)
			buffer->PartitionEntry->PartitionLength.QuadPart = 0ll;

		if (buffer->PartitionEntry->PartitionLength.HighPart > 0l)
			buffer->PartitionEntry->PartitionLength.HighPart = 0l;

		if (buffer->PartitionEntry->PartitionLength.LowPart > 0)
			buffer->PartitionEntry->PartitionLength.LowPart = 0;

		if (buffer->PartitionEntry->PartitionLength.u.HighPart > 0l)
			buffer->PartitionEntry->PartitionLength.u.HighPart = 0l;

		if (buffer->PartitionEntry->PartitionLength.u.LowPart > 0)
			buffer->PartitionEntry->PartitionLength.u.LowPart = 0;

		if (buffer->Signature > 0)
			buffer->Signature = 0;
	}

	if (request->OldRoutine && irp->StackCount > 1)
		return request->OldRoutine(device, irp, request->OldContext);

	return STATUS_SUCCESS;
}

// https://docs.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-ioctl_disk_get_drive_layout_ex
NTSTATUS HandleDriveLayoutEx(PDEVICE_OBJECT device, PIRP irp, PVOID context) {
	if (!context)
		return STATUS_SUCCESS;

	auto* request = (REQUEST_STRUCT*)context;
	ExFreePool(context);

	if (request->OutputBufferLength >= sizeof(DRIVE_LAYOUT_INFORMATION_EX)) {
		auto* buffer = (DRIVE_LAYOUT_INFORMATION_EX*)request->SystemBuffer;

		if (buffer->Gpt.DiskId.Data1 > 0ul)
			buffer->Gpt.DiskId.Data1 = 0ul;

		if (buffer->Gpt.DiskId.Data2 > 0)
			buffer->Gpt.DiskId.Data2 = 0;

		if (buffer->Gpt.DiskId.Data3 > 0)
			buffer->Gpt.DiskId.Data3 = 0;

		if (buffer->Gpt.DiskId.Data4)
			memset(buffer->Gpt.DiskId.Data4, 0, sizeof(buffer->Gpt.DiskId.Data4));

		if (buffer->Gpt.MaxPartitionCount > 0)
			buffer->Gpt.MaxPartitionCount = 0;

		if (buffer->Gpt.StartingUsableOffset.QuadPart > 0ll)
			buffer->Gpt.StartingUsableOffset.QuadPart = 0ll;

		if (buffer->Gpt.StartingUsableOffset.HighPart > 0l)
			buffer->Gpt.StartingUsableOffset.HighPart = 0l;

		if (buffer->Gpt.StartingUsableOffset.LowPart > 0)
			buffer->Gpt.StartingUsableOffset.LowPart = 0;

		if (buffer->Gpt.StartingUsableOffset.u.HighPart > 0l)
			buffer->Gpt.StartingUsableOffset.u.HighPart = 0l;

		if (buffer->Gpt.StartingUsableOffset.u.LowPart > 0)
			buffer->Gpt.StartingUsableOffset.u.LowPart = 0;

		if (buffer->Mbr.Signature > 0)
			buffer->Mbr.Signature = 0;

		if (buffer->PartitionCount > 0)
			buffer->PartitionCount = 0;

		if (buffer->PartitionEntry->Gpt.Attributes > 0)
			buffer->PartitionEntry->Gpt.Attributes = 0;

		if (buffer->PartitionEntry->Gpt.Name)
			memset(buffer->PartitionEntry->Gpt.Name, 0, sizeof(buffer->PartitionEntry->Gpt.Name));

		if (buffer->PartitionEntry->Gpt.PartitionId.Data1 > 0ul)
			buffer->PartitionEntry->Gpt.PartitionId.Data1 = 0ul;

		if (buffer->PartitionEntry->Gpt.PartitionId.Data2 > 0)
			buffer->PartitionEntry->Gpt.PartitionId.Data2 = 0;

		if (buffer->PartitionEntry->Gpt.PartitionId.Data3 > 0)
			buffer->PartitionEntry->Gpt.PartitionId.Data3 = 0;

		if (buffer->PartitionEntry->Gpt.PartitionId.Data4)
			memset(buffer->PartitionEntry->Gpt.PartitionId.Data4, 0, sizeof(buffer->PartitionEntry->Gpt.PartitionId.Data4));

		if (buffer->PartitionEntry->Gpt.PartitionType.Data1 > 0ul)
			buffer->PartitionEntry->Gpt.PartitionType.Data1 = 0ul;

		if (buffer->PartitionEntry->Gpt.PartitionType.Data2 > 0)
			buffer->PartitionEntry->Gpt.PartitionType.Data2 = 0;

		if (buffer->PartitionEntry->Gpt.PartitionType.Data3 > 0)
			buffer->PartitionEntry->Gpt.PartitionType.Data3 = 0;

		if (buffer->PartitionEntry->Gpt.PartitionType.Data4)
			memset(buffer->PartitionEntry->Gpt.PartitionType.Data4, 0, sizeof(buffer->PartitionEntry->Gpt.PartitionType.Data4));

		if (buffer->PartitionEntry->Mbr.HiddenSectors > 0)
			buffer->PartitionEntry->Mbr.HiddenSectors = 0;

		if (buffer->PartitionEntry->PartitionLength.HighPart > 0l)
			buffer->PartitionEntry->PartitionLength.HighPart = 0l;

		if (buffer->PartitionEntry->PartitionLength.LowPart > 0)
			buffer->PartitionEntry->PartitionLength.LowPart = 0;

		if (buffer->PartitionEntry->PartitionLength.u.HighPart > 0l)
			buffer->PartitionEntry->PartitionLength.u.HighPart = 0l;

		if (buffer->PartitionEntry->PartitionLength.u.LowPart > 0)
			buffer->PartitionEntry->PartitionLength.u.LowPart = 0;

		if (buffer->PartitionEntry->PartitionLength.QuadPart > 0ll)
			buffer->PartitionEntry->PartitionLength.QuadPart = 0ll;

		if (buffer->PartitionEntry->PartitionNumber > 0)
			buffer->PartitionEntry->PartitionNumber = 0;

		if (buffer->PartitionEntry->PartitionStyle)
			buffer->PartitionEntry->PartitionStyle = PARTITION_STYLE::PARTITION_STYLE_GPT;

		if (buffer->PartitionEntry->StartingOffset.QuadPart > 0ll)
			buffer->PartitionEntry->StartingOffset.QuadPart = 0ll;

		if (buffer->PartitionEntry->StartingOffset.HighPart > 0l)
			buffer->PartitionEntry->StartingOffset.HighPart = 0l;

		if (buffer->PartitionEntry->StartingOffset.LowPart > 0)
			buffer->PartitionEntry->StartingOffset.LowPart = 0;

		if (buffer->PartitionEntry->StartingOffset.u.HighPart > 0l)
			buffer->PartitionEntry->StartingOffset.u.HighPart = 0l;

		if (buffer->PartitionEntry->StartingOffset.u.LowPart > 0)
			buffer->PartitionEntry->StartingOffset.u.LowPart = 0;
	}

	if (request->OldRoutine && irp->StackCount > 1)
		return request->OldRoutine(device, irp, request->OldContext);

	return STATUS_SUCCESS;
}

NTSTATUS HandleARP(PDEVICE_OBJECT device, PIRP irp, PVOID context) {
	if (!context)
		return STATUS_SUCCESS;

	auto request = (REQUEST_STRUCT*)context;
	ExFreePool(context);
	
	auto buffer = (PNSI_PARAMS)irp->UserBuffer;

	if (buffer && buffer->Type == NSI_PARAMS_ARP) {
		memset(irp->UserBuffer, 0, request->OutputBufferLength);
	}

	if (request->OldRoutine && irp->StackCount > 1)
		return request->OldRoutine(device, irp, request->OldContext);

	return STATUS_SUCCESS;
}