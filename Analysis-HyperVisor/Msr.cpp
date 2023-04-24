#include "includes.h"
#include "Undocumented.hpp"
#include "Func_defs.hpp"
#include "Calling_Structs.hpp"

namespace ExitRoutines {

	bool HandleRdmsr(pguest_registers  GuestRegs) {
		__try
		{
			msr_split Result = { __readmsr((unsigned long)GuestRegs->rcx) };

			GuestRegs->rdx = Result.high;
			GuestRegs->rax = Result.low;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			Util::InjectInterupt(hardware_exception, general_protection, true, 0);
			return false;
		}

		return true;
	}

	bool HandleWrmsr(pguest_registers  GuestRegs) {
		msr_split Value;
		Value.low = GuestRegs->rax;
		Value.high = GuestRegs->rdx;

		//Pray that the guest doesn't modify Mtrr regs, as my dumbass just sets all of those to wb, and I don't update them at all

		__try
		{
			__writemsr((unsigned long)GuestRegs->rcx, Value.value);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			Util::InjectInterupt(hardware_exception, general_protection, true, 0);
			return false;
		}

		return true;
	}
};
