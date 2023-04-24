#include "includes.h"
#include "Common.hpp"
#include "ia32.hpp"
#include "Func_defs.hpp"
#include "Undocumented.hpp"

extern "C" uint64_t GuestRsp = 0;
extern "C" uint64_t GuestRip = 0;

extern "C" uint64_t VmexitHandler(pguest_registers  GuestRegs) {

	int i = KeGetCurrentProcessorNumber();

	Hv.vcpus[i].IsInRootMode = true;
	uint64_t Exitreason = 0;
	__vmx_vmread(VMCS_EXIT_REASON, &Exitreason);

	switch (Exitreason & 0xffff)
	{
		case VMX_EXIT_REASON_ERROR_INVALID_GUEST_STATE: //Trauma has been induced here )=
		{
			DbgBreakPoint();
			break;
		}

		case VMX_EXIT_REASON_EXECUTE_CPUID:
		{
			ExitRoutines::HandleCpuid(GuestRegs);
			goto advance_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_RDMSR:
		{
			if(ExitRoutines::HandleRdmsr(GuestRegs))
				goto advance_rip;
			
		}

		case VMX_EXIT_REASON_EXECUTE_WRMSR:
		{
			if (ExitRoutines::HandleWrmsr(GuestRegs))
				goto advance_rip;
		}

		case VMX_EXIT_REASON_EPT_VIOLATION:
		{
			ExitRoutines::HandleEptViolation();
			break;
		}

		case VMX_EXIT_REASON_EPT_MISCONFIGURATION:
		{
			DbgBreakPoint();
			break;
		}

		case VMX_EXIT_REASON_EXECUTE_VMCALL: 
		{
			if (ExitRoutines::VmCallHandler(GuestRegs) == 1) {
				VmxVmxoff();
				return 1; //Turn off
			}
			goto advance_rip;
			break;
		}

		case VMX_EXIT_REASON_EXECUTE_VMCLEAR: //set all of them to HandleVmxInstruction since we just
		case VMX_EXIT_REASON_EXECUTE_VMLAUNCH://inject invalid opcode exception
		case VMX_EXIT_REASON_EXECUTE_VMPTRLD:
		case VMX_EXIT_REASON_EXECUTE_VMPTRST:
		case VMX_EXIT_REASON_EXECUTE_VMREAD:
		case VMX_EXIT_REASON_EXECUTE_VMRESUME:
		case VMX_EXIT_REASON_EXECUTE_VMWRITE:
		case VMX_EXIT_REASON_EXECUTE_VMXON:
		case VMX_EXIT_REASON_EXECUTE_VMXOFF:
		{
			ExitRoutines::HandleVmxInstruction();
			goto advance_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_INVD:
		{
			__wbinvd();
			goto advance_rip;
		}

		case VMX_EXIT_REASON_MOV_CR:
		{
			ExitRoutines::HandleCrAccess(GuestRegs);
			goto advance_rip;
		}

		case VMX_EXIT_REASON_NMI_WINDOW:
		{
			ExitRoutines::HandleNmiWindow();
			break;
		}

		case VMX_EXIT_REASON_EXCEPTION_OR_NMI: 
		{
			ia32_vmx_procbased_ctls_register Procbased_Ctls;
			__vmx_vmread(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, &Procbased_Ctls.flags);

			Procbased_Ctls.nmi_window_exiting = true;
			__vmx_vmwrite(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, Procbased_Ctls.flags);
			break;
		}

		case VMX_EXIT_REASON_EXECUTE_XSETBV: 
		{
			if (ExitRoutines::HandleXSetBv(GuestRegs))
				goto advance_rip;
		}

		case VMX_EXIT_REASON_APIC_ACCESS:
		{
			DBG_LOG("Guest tried to access LApic page");
			DbgBreakPoint();
		}

		case VMX_EXIT_REASON_APIC_WRITE: 
		{
			DBG_LOG("Apic got written to");
			DbgBreakPoint();
		}

		case VMX_EXIT_REASON_TRIPLE_FAULT: 
		{
			DBG_LOG("Triple fault occured!");
			uint64_t rip, exec_len = 0;
			__vmx_vmread(VMCS_GUEST_RIP, &rip);
			__vmx_vmread(VMCS_VMEXIT_INSTRUCTION_LENGTH, &exec_len);
			DBG_LOG("Exit Guest Rip: 0x%x Instruction Length: 0x%x", rip, exec_len);
			DbgBreakPoint();
			break;
		}

		default:
		{
			DBG_LOG("Not implemented Exit!");
			DbgBreakPoint();
			break;
		}

	}

	Hv.vcpus[i].IsInRootMode = false;

	return 0;
advance_rip:
	Hv.vcpus[i].IsInRootMode = false;
	uint64_t rip, exec_len;

	__vmx_vmread(VMCS_GUEST_RIP, &rip);
	__vmx_vmread(VMCS_VMEXIT_INSTRUCTION_LENGTH, &exec_len);
	__vmx_vmwrite(VMCS_GUEST_RIP, rip + exec_len);

	ExitRoutines::HandleDebug();

	return 0;
}

extern "C" void VmResumeFunc(void) {


	__vmx_vmresume();

	// Vm resume failed Break into debugger if there is one attached

	uint64_t ErrorCode = 0;
	__vmx_vmread(VMCS_VM_INSTRUCTION_ERROR, &ErrorCode);

	DBG_LOG("VmResume Error : 0x%llx\n", ErrorCode);

	DbgBreakPoint();

}

uint64_t
VmxRead(
	IN uint64_t Field
)
{
	unsigned char result;
	uint64_t fieldValue;

	result = __vmx_vmread(Field, &fieldValue);
	if (result != 0)
	{
		fieldValue = 0;
	}
	return fieldValue;
}
void VmxVmxoff() {
	uint64_t VmexitGuestRsp = 0;
	uint64_t VmexitGuestRip = 0;
	uint64_t Instructionlength = 0;
	segment_descriptor_register_64  gdtr = { 0 };
	segment_descriptor_register_64  idtr = { 0 };

	gdtr.base_address = VmxRead(VMCS_GUEST_GDTR_BASE); //vmx read wrapper for easy usage
	gdtr.limit = (uint16_t)VmxRead(VMCS_GUEST_GDTR_LIMIT);

	_lgdt(&gdtr);

	idtr.base_address = VmxRead(VMCS_GUEST_IDTR_BASE); //vmx read wrapper for easy usage
	idtr.limit = (uint16_t)VmxRead(VMCS_GUEST_IDTR_LIMIT);

	__lidt(&idtr);

	__writecr3(VmxRead(VMCS_GUEST_CR3));

	// Read instruction length
	__vmx_vmread(VMCS_VMEXIT_INSTRUCTION_LENGTH, &Instructionlength);
	__vmx_vmread(VMCS_GUEST_RIP, &VmexitGuestRip);
	__vmx_vmread(VMCS_GUEST_RSP, &VmexitGuestRsp);

	VmexitGuestRip += Instructionlength;
	GuestRip = VmexitGuestRip;
	GuestRsp = VmexitGuestRsp;
}