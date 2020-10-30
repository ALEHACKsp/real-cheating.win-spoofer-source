#pragma once

char spoofed_serial[] = "...............";
bool const_change_serial = false, use_custom_serial = false, null_serial = false, use_random_serial = false, spoof_initiated = false;

// device control dispatch's
PDRIVER_DISPATCH disk_original_device_control, partmgr_original_device_control, nsi_original_device_control, other_driver_original_device_control,
// close dispatch's
disk_original_close_dispatch, partmgr_original_close_dispatch, nsi_original_close_dispatch, other_driver_original_close_dispatch,
// create dispatch's
disk_original_create_dispatch, partmgr_original_create_dispatch, nsi_original_create_dispatch, other_driver_original_create_dispatch;

namespace Utils
{
    bool CheckMask(const char* base, const char* pattern, const char* mask)
    {
        for (; *mask; base++, pattern++, mask++)
        {
            if (XorString("x")[0] == *mask && *base != *pattern)
            {
                return false;
            }
        }

        return true;
    }

    PVOID FindPattern(PVOID base, int length, const char* pattern, const char* mask)
    {
        length -= static_cast<int>(strlen(mask));
        for (auto i = 0; i <= length; ++i)
        {
            const auto* data = static_cast<char*>(base);
            const auto* address = &data[i];
            if (CheckMask(address, pattern, mask))
                return PVOID(address);
        }

        return nullptr;
    }

    PVOID FindPatternImage(PVOID base, const char* pattern, const char* mask)
    {
        PVOID match = nullptr;
        auto* headers = reinterpret_cast<PIMAGE_NT_HEADERS>(static_cast<char*>(base) + static_cast<PIMAGE_DOS_HEADER>(base)->e_lfanew);
        auto* sections = IMAGE_FIRST_SECTION(headers);
        for (auto i = 0; i < headers->FileHeader.NumberOfSections; i++)
        {
            auto* section = &sections[i];
            if (1162297680 == *reinterpret_cast<int*>(section->Name) || memcmp(section->Name, XorString(".text"), 5) == 0)
            {
                match = FindPattern(static_cast<char*>(base) + section->VirtualAddress, section->Misc.VirtualSize, pattern, mask);
                if (match)
                    break;
            }
        }
        return match;
    }

    PVOID ReturnModuleBase(const char* moduleName)
    {
        PVOID address = nullptr;
        ULONG size = 0;

        auto status = ZwQuerySystemInformation(SystemModuleInformation, &size, 0, &size);
        if (status != STATUS_INFO_LENGTH_MISMATCH)
            return nullptr;

        auto* moduleList = static_cast<PSYSTEM_MODULE_INFORMATION>(ExAllocatePool(NonPagedPool, size));
        if (!moduleList)
            return nullptr;

        status = ZwQuerySystemInformation(SystemModuleInformation, moduleList, size, nullptr);
        if (!NT_SUCCESS(status)) {
            ExFreePool(moduleList); return nullptr;
        }

        for (auto i = 0; i < moduleList->ulModuleCount; i++)
        {
            auto module = moduleList->Modules[i];
            if (strstr(module.ImageName, moduleName))
            {
                address = module.Base;
                break;
            }
        }
        ExFreePool(moduleList);
        return address;
    }

    void RandomString(char* s, const int len) {
        static const char alphanum[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

        for (int i = 0; i < len; ++i) {
            s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
        }

        s[len] = 0;
    }

    void SetCompletionRoutine(PIRP irp, PIO_STACK_LOCATION ioc, PIO_COMPLETION_ROUTINE routine) {
        auto* context = (REQUEST_STRUCT*)ExAllocatePool(NonPagedPool, sizeof(REQUEST_STRUCT));
        if (!context)
            return;

        context->SystemBuffer = irp->AssociatedIrp.SystemBuffer;
        context->OutputBufferLength = ioc->Parameters.DeviceIoControl.OutputBufferLength;
        context->OldContext = ioc->Context;
        context->OldRoutine = ioc->CompletionRoutine;

        ioc->Control = SL_INVOKE_ON_SUCCESS;
        ioc->Context = context;
        ioc->CompletionRoutine = routine;
    }

    PDEVICE_OBJECT GetDeviceObject(UNICODE_STRING driverName) {
        PDEVICE_OBJECT deviceObject; PFILE_OBJECT file;
        NTSTATUS getdevice_status = IoGetDeviceObjectPointer(&driverName, FILE_ALL_ACCESS, &file, &deviceObject);
        if (!NT_SUCCESS(getdevice_status))
            return deviceObject;
        else {
            ObDereferenceObject(file);
            return deviceObject;
        }
        return deviceObject;
    }

    PWCH GetDeviceName(PDEVICE_OBJECT device_object, AnsiString ansistring) {
        NTSTATUS status = STATUS_UNSUCCESSFUL; ULONG ReturnLength; OBJECT_NAME_INFORMATION objName = { 0 };
        ObQueryNameString((PVOID)device_object, NULL, NULL, &ReturnLength);
        status = ObQueryNameString((PVOID)device_object, &objName, sizeof(OBJECT_NAME_INFORMATION) + ReturnLength, &ReturnLength);
        if (NT_SUCCESS(status)) {
            WideString wStr1(objName.Name.Buffer);
            ansistring = wStr1.ToLowerCase().GetAnsi();
            return objName.Name.Buffer;
        }
        else {
            return nullptr;
        }
    }
}