#include "includes.h"
#include "Undocumented.hpp"
#include "Func_defs.hpp"
#include "Calling_Structs.hpp"

namespace ExitRoutines {

	bool HandleCpuid(pguest_registers  GuestRegs) {
		uint64_t TscOffset = 0;
		uint64_t TimeForEditingTscOffset = 0;
		int result[4];

		if (Hv.Timing.UseTimeDiffs) {
			TimeForEditingTscOffset = __rdtsc();
			__vmx_vmread(VMCS_CTRL_TSC_OFFSET, &TscOffset);

			TscOffset = TscOffset - Hv.Timing.CpuidTimeDiff;

			__vmx_vmwrite(VMCS_CTRL_TSC_OFFSET, TscOffset);
			TimeForEditingTscOffset = __rdtsc() - TimeForEditingTscOffset;
		}
		else //Used for calibration of the time it takes to edit the tsc
		{
			TimeForEditingTscOffset = __rdtsc();
			__vmx_vmread(VMCS_CTRL_TSC_OFFSET, &TscOffset);

			TscOffset = Hv.Timing.CpuidTimeDiff - Hv.Timing.CpuidTimeDiff;

			__vmx_vmwrite(VMCS_CTRL_TSC_OFFSET, TscOffset);
			TimeForEditingTscOffset = __rdtsc() - TimeForEditingTscOffset;

			Hv.Timing.TimesEditedTsc++;
			Hv.Timing.TimeForEditingTsc += TimeForEditingTscOffset;
			Hv.Timing.AverageEditTime = Hv.Timing.TimeForEditingTsc / Hv.Timing.TimesEditedTsc;
		}

		__cpuidex(result, (int)GuestRegs->rax, (int)GuestRegs->rcx);

		switch (GuestRegs->rax) //Handle Cpuid detection vectors (=
		{
			case CpuidFeatureIdentifier: //Cpuid Processor features
			{
				result[2] |= HypervisorPresentBit; //Hide Presence
				break;
			}

		}

		GuestRegs->rax = result[0];
		GuestRegs->rbx = result[1];
		GuestRegs->rcx = result[2];
		GuestRegs->rdx = result[3];

		return 0;
	}
};