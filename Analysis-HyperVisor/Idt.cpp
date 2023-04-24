#include "includes.h"
#include "Func_defs.hpp"
#include "Undocumented.hpp"


auto seh_handler_ecode(pidt_regs_ecode_t regs) -> void
{
    vcpu Vcpu = Hv.vcpus[KeGetCurrentProcessorNumber()];
    Vcpu.ErrorCode = regs->error_code;
    const auto rva = regs->rip - reinterpret_cast<uint64_t>(Idt::image_base);
    const auto nt_headers = reinterpret_cast<IMAGE_NT_HEADERS64*>(reinterpret_cast<uint64_t>(Idt::image_base) + reinterpret_cast<IMAGE_DOS_HEADER*>(Idt::image_base)->e_lfanew);

    const auto exception =  &nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];

    const auto functions = reinterpret_cast<RUNTIME_FUNCTION*>(reinterpret_cast<uint64_t>(Idt::image_base) + exception->VirtualAddress);

    for (auto idx = 0; idx < exception->Size / sizeof(RUNTIME_FUNCTION); ++idx)
    {
        const auto function = &functions[idx];
        if (!(rva >= function->BeginAddress && rva < function->EndAddress))
            continue;

        const auto unwind_info =
            reinterpret_cast<UNWIND_INFO*>(
                reinterpret_cast<uint64_t>(Idt::image_base) + function->UnwindData);

        if (!(unwind_info->Flags & UNW_FLAG_EHANDLER))
            continue;

        const auto scope_table =
            reinterpret_cast<SCOPE_TABLE*>(
                reinterpret_cast<uint64_t>(&unwind_info->UnwindCode[
                    (unwind_info->CountOfCodes + 1) & ~1]) + sizeof(uint32_t));

        for (UINT32 entry = 0; entry < scope_table->Count; ++entry)
        {
            const auto scope_record = &scope_table->ScopeRecords[entry];
            if (rva >= scope_record->BeginAddress && rva < scope_record->EndAddress)
            {
                regs->rip = reinterpret_cast<uint64_t>(Idt::image_base) + scope_record->JumpTarget;
                return;
            }
        }
    }
}

auto seh_handler(pidt_regs_t regs) -> void
{
    const auto rva = regs->rip - reinterpret_cast<uint64_t>(Idt::image_base);
    const auto nt_headers = reinterpret_cast<IMAGE_NT_HEADERS64*>(
        reinterpret_cast<uint64_t>(Idt::image_base) +
        reinterpret_cast<IMAGE_DOS_HEADER*>(Idt::image_base)->e_lfanew);

    const auto exception =
        &nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];

    const auto functions =
        reinterpret_cast<RUNTIME_FUNCTION*>(
            reinterpret_cast<uint64_t>(Idt::image_base) + exception->VirtualAddress);

    for (auto idx = 0; idx < exception->Size / sizeof(RUNTIME_FUNCTION); ++idx)
    {
        const auto function = &functions[idx];
        if (!(rva >= function->BeginAddress && rva < function->EndAddress))
            continue;

        const auto unwind_info =
            reinterpret_cast<UNWIND_INFO*>(
                reinterpret_cast<uint64_t>(Idt::image_base) + function->UnwindData);

        if (!(unwind_info->Flags & UNW_FLAG_EHANDLER))
            continue;

        const auto scope_table =
            reinterpret_cast<SCOPE_TABLE*>(
                reinterpret_cast<uint64_t>(&unwind_info->UnwindCode[
                    (unwind_info->CountOfCodes + 1) & ~1]) + sizeof(uint32_t));

        for (UINT32 entry = 0; entry < scope_table->Count; ++entry)
        {
            const auto scope_record = &scope_table->ScopeRecords[entry];
            if (rva >= scope_record->BeginAddress && rva < scope_record->EndAddress)
            {
                regs->rip = reinterpret_cast<uint64_t>(Idt::image_base) + scope_record->JumpTarget;
                return;
            }
        }
    }
}

void nmi_handler()
{
    DBG_LOG("Nmi occured");
    ia32_vmx_procbased_ctls_register procbased_ctls;
    __vmx_vmread(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, &procbased_ctls.flags);

    procbased_ctls.nmi_window_exiting = true;
    __vmx_vmwrite(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, procbased_ctls.flags);
}

namespace Idt
{
    idt_entry_t create_entry(idt_addr_t idt_handler, uint8_t ist_index)
    {
        UNREFERENCED_PARAMETER(ist_index);
        idt_entry_t result{};
        result.segment_selector = GetCs().flags;
        result.gate_type = SEGMENT_DESCRIPTOR_TYPE_INTERRUPT_GATE;
        result.present = true;

        result.offset_high = idt_handler.offset_high;
        result.offset_middle = idt_handler.offset_middle;
        result.offset_low = idt_handler.offset_low;
        return result;
    }
}