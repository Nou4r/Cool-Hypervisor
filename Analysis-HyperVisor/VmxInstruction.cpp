#include "includes.h"
#include "Undocumented.hpp"
#include "Func_defs.hpp"
#include "Calling_Structs.hpp"

namespace ExitRoutines {

	void HandleVmxInstruction() {

		uint64_t TimeForEditingTscOffset = 0;
		uint64_t TscOffset = 0;

		if (Hv.Timing.UseTimeDiffs) {
			TimeForEditingTscOffset = __rdtsc();
			__vmx_vmread(VMCS_CTRL_TSC_OFFSET, &TscOffset);

			TscOffset = TscOffset - Hv.Timing.VmxInstructionTimeDiff;

			__vmx_vmwrite(VMCS_CTRL_TSC_OFFSET, TscOffset);
			TimeForEditingTscOffset = __rdtsc() - TimeForEditingTscOffset;
		}

		Util::InjectInterupt(hardware_exception, invalid_opcode, false, 0);
	}
};