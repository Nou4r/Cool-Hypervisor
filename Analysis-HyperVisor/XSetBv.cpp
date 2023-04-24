#include "includes.h"
#include "Undocumented.hpp"
#include "Func_defs.hpp"
#include "Calling_Structs.hpp"

namespace ExitRoutines {


	bool HandleXSetBv(pguest_registers  GuestRegs) {

		cr4 Cr4Reg = { 0 };
		Cr4Reg.flags = __readcr4();
		xcr0 NewXCr0 = { 0 };
		NewXCr0.flags = (GuestRegs->rdx << 32) | (GuestRegs->rax & 0xFFFFFFFF);

		if (!Cr4Reg.os_xsave) {
			Util::InjectInterupt(hardware_exception, general_protection, true, 0);
			return true;
		}

		cpuid_eax_0d_ecx_00 cpuid_0d;
		__cpuidex(reinterpret_cast<int*>(&cpuid_0d), 0x0D, 0x00);

		uint64_t xcr0_unsupported_mask = ~((static_cast<uint64_t>(cpuid_0d.edx.flags) << 32) | cpuid_0d.eax.flags);

		if (GuestRegs->rcx != 0) {
			Util::InjectInterupt(hardware_exception, general_protection, true, 0);
			return true;
		}

		if (NewXCr0.flags & xcr0_unsupported_mask) {
			Util::InjectInterupt(hardware_exception, general_protection, true, 0);
			return true;
		}
		if (!NewXCr0.x87_support) {
			Util::InjectInterupt(hardware_exception, general_protection, true, 0);
			return true;
		}

		if (!NewXCr0.avx_support && (NewXCr0.opmask_support || NewXCr0.zmm_hi256_support || NewXCr0.hi16_zmm_support)) {
			Util::InjectInterupt(hardware_exception, general_protection, true, 0);
			return true;
		}

		_xsetbv((uint32_t)GuestRegs->rcx, NewXCr0.flags);

		return true;
	}

};
