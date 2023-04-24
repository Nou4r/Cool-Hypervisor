#include "includes.h"
#include "Common.hpp"
#include "ia32.hpp"
#include "Func_defs.hpp"
#include "Undocumented.hpp"
#include "Calling_Structs.hpp"

namespace EptHook {

    void* SyscallHookGetKernelBase(ULONG* pImageSize) {
        void* pModuleBase = 0;
        NTSTATUS status;
        ZWQUERYSYSTEMINFORMATION ZwQSI = 0;
        UNICODE_STRING routineName;
        SYSTEM_MODULE_INFORMATION* pSystemInfoBuffer = 0;
        ULONG SystemInfoBufferSize = 0;


        RtlInitUnicodeString(&routineName, L"ZwQuerySystemInformation");
        ZwQSI = (ZWQUERYSYSTEMINFORMATION)MmGetSystemRoutineAddress(&routineName);

        if (!ZwQSI)
            return 0;

        Hv.Funcdefs.ZwQSI = ZwQSI;

        status = ZwQSI(SystemModuleInformation,
            &SystemInfoBufferSize,
            0,
            &SystemInfoBufferSize);

        if (!SystemInfoBufferSize)
        {
            DBG_LOG("ZwQuerySystemInformation failed");
            return 0;
        }

        pSystemInfoBuffer = (SYSTEM_MODULE_INFORMATION*)ExAllocatePool2(POOL_FLAG_NON_PAGED_EXECUTE, SystemInfoBufferSize * 2, (ULONG)POOL_TAG);

        if (!pSystemInfoBuffer)
        {
            DBG_LOG("ExAllocatePool failed");
            return 0;
        }

        RtlZeroMemory(pSystemInfoBuffer, SystemInfoBufferSize * 2);

        status = ZwQSI(SystemModuleInformation,
            pSystemInfoBuffer,
            SystemInfoBufferSize * 2,
            &SystemInfoBufferSize);

        if (NT_SUCCESS(status))
        {
            pModuleBase = pSystemInfoBuffer->Module[0].ImageBase;
            if (pImageSize)
                *pImageSize = pSystemInfoBuffer->Module[0].ImageSize;
        }
        else {
            DBG_LOG("ZwQuerySystemInformation failed");
            return 0;
        }

        ExFreePoolWithTag(pSystemInfoBuffer, (ULONG)POOL_TAG);

        return pModuleBase;
    }

    bool GetSsdt(uint64_t* NtTable) {
        ULONG kernelSize = 0;
        ULONG_PTR kernelBase = 0;
        unsigned char KiSystemServiceStartPattern[] = { 0x8B, 0xF8, 0xC1, 0xEF, 0x07, 0x83, 0xE7, 0x20, 0x25, 0xFF, 0x0F, 0x00, 0x00 };
        ULONG signatureSize = sizeof(KiSystemServiceStartPattern);
        bool found = false;
        LONG relativeOffset = 0;
        ULONG_PTR addressAfterPattern = 0;
        ULONG_PTR address = 0;
        SSDTStruct* shadow = 0;
        void* ntTable = 0;
        void* win32kTable = 0;

        kernelBase = (ULONG_PTR)Hv.KernelBase;
        kernelSize = Hv.KernelSize;

        if (kernelBase == 0 || kernelSize == 0)
            return FALSE;

        ULONG KiSSSOffset;
        for (KiSSSOffset = 0; KiSSSOffset < kernelSize - signatureSize; KiSSSOffset++)
        {
            if (RtlCompareMemory(((unsigned char*)kernelBase + KiSSSOffset), KiSystemServiceStartPattern, signatureSize) == signatureSize)
            {
                //KiSystemServiceStart found
                found = TRUE;
                break;
            }
        }

        if (!found)
            return FALSE;

        addressAfterPattern = kernelBase + KiSSSOffset + signatureSize;
        address = addressAfterPattern + 7; // Skip lea r10,[nt!KeServiceDescriptorTable]

        //
        // lea r11, KeServiceDescriptorTableShadow
        //
        if ((*(unsigned char*)address == 0x4c) &&
            (*(unsigned char*)(address + 1) == 0x8d) &&
            (*(unsigned char*)(address + 2) == 0x1d))
        {
            relativeOffset = *(LONG*)(address + 3);
        }

        if (relativeOffset == 0)
            return FALSE;

        shadow = (SSDTStruct*)(address + relativeOffset + 7);

        ntTable = (void*)shadow;
        win32kTable = (void*)((ULONG_PTR)shadow + 0x20);

        *NtTable = (uint64_t)ntTable;

        return TRUE;
    }

    void* GetSysCallAddress(uint32_t ApiNumber)
    {
        SSDTStruct* SSDT = 0;
        bool Result = false;
        ULONG_PTR SSDTbase = 0;
        uint64_t NtTable = 0;

        Result = GetSsdt(&NtTable);

        if (!Result)
        {
            DBG_LOG("Failed to find Ssdt");
            return 0;
        }

        SSDT = (SSDTStruct*)NtTable;

        SSDTbase = (ULONG_PTR)SSDT->pServiceTable;

        if (!SSDTbase)
        {
            DBG_LOG("Failed to find ServiceTable");
            return 0;
        }
        return (void*)((SSDT->pServiceTable[ApiNumber] >> 4) + SSDTbase);
    }
};