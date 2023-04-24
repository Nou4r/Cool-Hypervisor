#include "includes.h"
#include "Undocumented.hpp"
#include "Func_defs.hpp"
#include "Calling_Structs.hpp"

namespace ExitRoutines {

	void HandleDebug() {
		rflags g_rflags;
		ia32_debugctl_register debugctl;

		__vmx_vmread(VMCS_GUEST_RFLAGS, &g_rflags.flags);
		__vmx_vmread(VMCS_GUEST_DEBUGCTL, &debugctl.flags);

		if (g_rflags.trap_flag && !debugctl.btf)
		{
			vmx_exit_qualification_debug_exception pending_db;
			__vmx_vmread(VMCS_GUEST_PENDING_DEBUG_EXCEPTIONS, &pending_db.flags);
			pending_db.single_instruction = true;
			__vmx_vmwrite(VMCS_GUEST_PENDING_DEBUG_EXCEPTIONS, pending_db.flags);
		}

		vmx_interruptibility_state interrupt_state;
		__vmx_vmread(VMCS_GUEST_INTERRUPTIBILITY_STATE,
			reinterpret_cast<size_t*>(&interrupt_state.flags));

		interrupt_state.blocking_by_mov_ss = false;
		interrupt_state.blocking_by_sti = false;
		interrupt_state.blocking_by_nmi = false;
		interrupt_state.blocking_by_smi = false;

		__vmx_vmwrite(VMCS_GUEST_INTERRUPTIBILITY_STATE, interrupt_state.flags);
	}

};