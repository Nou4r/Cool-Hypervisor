#include "includes.h"
#include "Undocumented.hpp"
#include "Func_defs.hpp"
#include "Calling_Structs.hpp"

namespace ExitRoutines {
	//To do: Apic virtualization to render this useless

	void HandleExceptionOrNmi() {
		ia32_vmx_procbased_ctls_register Procbased_Ctls;
		__vmx_vmread(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, &Procbased_Ctls.flags);

		Procbased_Ctls.nmi_window_exiting = true;

		__vmx_vmwrite(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, Procbased_Ctls.flags);
	}

	void HandleNmiWindow() { //Only for guest; host has its own idt
		ia32_vmx_procbased_ctls_register Procbased_Ctls;

		DBG_LOG("Guest Nmi occured");

		Util::InjectInterupt(non_maskable_interrupt, nmi, 0, 0);

		__vmx_vmread(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, &Procbased_Ctls.flags);

		Procbased_Ctls.nmi_window_exiting = false;

		__vmx_vmwrite(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, Procbased_Ctls.flags);
	}
};