#include "includes.h"
#include "Undocumented.hpp"
#include "Func_defs.hpp"

namespace Util {

    volatile LONG LockForPml1Modification = 0;

    uint64_t VirtualAddressToPhysicalAddress(void* VirtualAddress)
    {
        return MmGetPhysicalAddress(VirtualAddress).QuadPart;
    }

    uint64_t PhysicalAddressToVirtualAddress(uint64_t PhysicalAddress)
    {
        PHYSICAL_ADDRESS PhysicalAddr = { 0 };
        PhysicalAddr.QuadPart = PhysicalAddress;

        return reinterpret_cast<uint64_t>(MmGetVirtualForPhysical(PhysicalAddr));
    }

    cr3
        SwitchOnAnotherProcessMemoryLayoutByCr3(cr3 TargetCr3)
    {
        cr3 CurrentProcessCr3 = { 0 };

        CurrentProcessCr3.flags = __readcr3();

        __writecr3(TargetCr3.flags);

        return CurrentProcessCr3;
    }

    void RestoreToPreviousProcess(cr3 PreviousProcess)
    {
        __writecr3(PreviousProcess.flags);
    }

    cr3 GetCr3FromPid(uint64_t Pid) {
        PEPROCESS TargetEprocess;
        cr3  ProcessCr3 = { 0 };

        if (PsLookupProcessByProcessId((HANDLE)Pid, &TargetEprocess) != STATUS_SUCCESS)
        {
            return ProcessCr3;
        }

        NT_KPROCESS* CurrentProcess = (NT_KPROCESS*)(TargetEprocess);
        ProcessCr3.flags = CurrentProcess->DirectoryTableBase;

        ObDereferenceObject(TargetEprocess);

        return ProcessCr3;
    }

    uint64_t VirtualAddressToPhysicalAddressByCr3(void* VirtualAddress, cr3 TargetCr3)
    {
        cr3 CurrentProcessCr3;
        UINT64   PhysicalAddress;

        CurrentProcessCr3 = SwitchOnAnotherProcessMemoryLayoutByCr3(TargetCr3);

        if (CurrentProcessCr3.flags == NULL)
        {
            return NULL;
        }

        PhysicalAddress = MmGetPhysicalAddress(VirtualAddress).QuadPart;

        RestoreToPreviousProcess(CurrentProcessCr3);

        return PhysicalAddress;
    }

    bool WriteToMemoryByCr3(void* VirtualAddress, void* Buffer, size_t size, cr3 UserCr3)
    {
        PHYSICAL_ADDRESS physAddr = { 0 };
        bool status = false;

        if (!VirtualAddress || !UserCr3.flags || !Buffer || !size) {
            DBG_LOG("Invalid params");
            return status;
        }

        physAddr.QuadPart = VirtualAddressToPhysicalAddressByCr3(VirtualAddress, UserCr3);

        if (!physAddr.QuadPart)
            return status;

        void* kernel_addr = MmMapIoSpace(physAddr, size, MmNonCached);

        if (kernel_addr)
        {
            RtlCopyMemory(kernel_addr, Buffer, size);
            status = true;
            MmUnmapIoSpace(kernel_addr, size);
        }

        return status;
    }

    bool ReadFromMemoryByCr3(void* VirtualAddress, void* Buffer, size_t size, cr3 UserCr3)
    {
        PHYSICAL_ADDRESS physAddr = { 0 };
        bool status = false;

        if (!VirtualAddress || !UserCr3.flags || !Buffer || !size) {
            DBG_LOG("Invalid params");
            return status;
        }

        physAddr.QuadPart = VirtualAddressToPhysicalAddressByCr3(VirtualAddress, UserCr3);

        if (!physAddr.QuadPart)
            return status;

        void* kernel_addr = MmMapIoSpace(physAddr, size, MmNonCached);

        if (kernel_addr)
        {
            RtlCopyMemory(Buffer, kernel_addr, size);
            status = TRUE;
            MmUnmapIoSpace(kernel_addr, size);
        }

        return status;
    }

    uint64_t FindSystemDirectoryTableBase()
    {
        // Return CR3 of the system process.
        NT_KPROCESS* SystemProcess = (NT_KPROCESS*)(PsInitialSystemProcess);
        return SystemProcess->DirectoryTableBase;
    }


    uint64_t MathPower(uint64_t Base, uint64_t Exp) {

        uint64_t result;

        result = 1;

        for (;;)
        {
            if (Exp & 1)
            {
                result *= Base;
            }
            Exp >>= 1;
            if (!Exp)
            {
                break;
            }
            Base *= Base;
        }
        return result;
    }

    uint64_t segment_base(
        segment_descriptor_register_64 const& gdtr,
        segment_selector const selector) {

        if (selector.index == 0)
            return 0;

        auto const descriptor = reinterpret_cast<segment_descriptor_64*>(
            gdtr.base_address + static_cast<uint64_t>(selector.index) * 8);

        auto base_address =
            (uint64_t)descriptor->base_address_low |
            ((uint64_t)descriptor->base_address_middle << 16) |
            ((uint64_t)descriptor->base_address_high << 24);

        if (descriptor->descriptor_type == SEGMENT_DESCRIPTOR_TYPE_SYSTEM)
            base_address |= (uint64_t)descriptor->base_address_upper << 32;

        return base_address;
    }


    unsigned __int32 access_right(unsigned __int16 segment_selector)
    {
        __segment_selector_t selector;
        __segment_access_rights_t vmx_access_rights;

        selector.flags = segment_selector;

        if (selector.table == 0
            && selector.index == 0)
        {
            vmx_access_rights.flags = 0;
            vmx_access_rights.unusable = TRUE;
            return vmx_access_rights.flags;
        }

        vmx_access_rights.flags = (__load_ar(segment_selector) >> 8);
        vmx_access_rights.unusable = 0;
        vmx_access_rights.reserved0 = 0;
        vmx_access_rights.reserved1 = 0;

        return vmx_access_rights.flags;
    }

    void InjectInterupt(interruption_type Interupttype, exception_vector ExceptionVector, bool DeliverErrorCode, uint32_t ErrorCode) {

        vmentry_interrupt_information EntryInteruptInfo = { 0 };

        EntryInteruptInfo.flags = 0;
        EntryInteruptInfo.valid = true;
        EntryInteruptInfo.interruption_type = (uint32_t)Interupttype;
        EntryInteruptInfo.vector = ExceptionVector;
        EntryInteruptInfo.deliver_error_code = DeliverErrorCode;

        __vmx_vmwrite(VMCS_CTRL_VMENTRY_INTERRUPTION_INFORMATION_FIELD, EntryInteruptInfo.flags);

        if (DeliverErrorCode)
            __vmx_vmwrite(VMCS_CTRL_VMENTRY_EXCEPTION_ERROR_CODE, ErrorCode);
    }

    uint64_t* SelectEffectiveRegister(pguest_registers GuestContext, uint64_t RegisterIndex) {
        uint64_t* effectiveRegister = 0;

        switch (RegisterIndex)
        {
        case 0: effectiveRegister = &GuestContext->rax; break;
        case 1: effectiveRegister = &GuestContext->rcx; break;
        case 2: effectiveRegister = &GuestContext->rdx; break;
        case 3: effectiveRegister = &GuestContext->rbx; break;
            //Ignore Rsp
        case 5: effectiveRegister = &GuestContext->rbp; break;
        case 6: effectiveRegister = &GuestContext->rsi; break;
        case 7: effectiveRegister = &GuestContext->rdi; break;
        case 8: effectiveRegister = &GuestContext->r8; break;
        case 9: effectiveRegister = &GuestContext->r9; break;
        case 10: effectiveRegister = &GuestContext->r10; break;
        case 11: effectiveRegister = &GuestContext->r11; break;
        case 12: effectiveRegister = &GuestContext->r12; break;
        case 13: effectiveRegister = &GuestContext->r13; break;
        case 14: effectiveRegister = &GuestContext->r14; break;
        case 15: effectiveRegister = &GuestContext->r15; break;
        }

        return effectiveRegister;
    }

    inline BOOLEAN SpinlockTryLock(volatile LONG* Lock)
    {
        return (!(*Lock) && !_interlockedbittestandset(Lock, 0));
    }

    void SpinlockLock(volatile LONG* Lock)
    {
        static unsigned max_wait = 65536;
        unsigned wait = 1;

        while (!SpinlockTryLock(Lock))
        {
            for (unsigned i = 0; i < wait; ++i)
            {
                _mm_pause();
            }

            if (wait * 2 > max_wait)
            {
                wait = max_wait;
            }
            else
            {
                wait = wait * 2;
            }
        }


    }

    void SpinlockUnlock(volatile LONG* Lock)
    {
        *Lock = 0;
    }

    void SetMtf(bool Value) {
        ia32_vmx_procbased_ctls_register CpuPrimaryctls = { 0 };

        __vmx_vmread(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, &CpuPrimaryctls.flags);

        CpuPrimaryctls.monitor_trap_flag = Value;

        __vmx_vmwrite(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, CpuPrimaryctls.flags);

    }

    void SetPml1AndInvept(Pml1Entry* EntryAddress, Pml1Entry EntryValue, invept_type InvalidationType)
    {
        invept_descriptor Descriptor = { 0 };

        SpinlockLock(&LockForPml1Modification);

        EntryAddress->flags = EntryValue.flags;

        if (InvalidationType == invept_single_context)
        {

            Descriptor.ept_pointer = GlobalExtendedPageTableState->EptPointer.flags;
            Descriptor.reserved = 0;
            __invept(1, &Descriptor);
        }
        else
        {
            Descriptor.ept_pointer = 0;
            Descriptor.reserved = 0;
            __invept(2, &Descriptor);
        }

        SpinlockUnlock(&LockForPml1Modification);
    }
};