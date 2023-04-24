#include "includes.h"
#include "Common.hpp"
#include "ia32.hpp"
#include "Func_defs.hpp"
#include "Undocumented.hpp"

namespace Vmx {

	cr0 AdjustCr0(cr0 Cr0)
	{
		cr0 newCr0 = { 0 };
		cr0	fixed0Cr0 = { 0 };
		cr0 fixed1Cr0 = { 0 };
		cr0 fixed2Cr0 = { 0 };

		newCr0 = Cr0;
		fixed0Cr0.flags = __readmsr(IA32_VMX_CR0_FIXED0);
		fixed1Cr0.flags = __readmsr(IA32_VMX_CR0_FIXED1);
		newCr0.flags &= fixed1Cr0.flags;
		newCr0.flags |= fixed0Cr0.flags;
		return newCr0;
	}

	cr4 AdjustCr4(cr4 Cr4)
	{
		cr4 newCr4 = { 0 };
		cr4	fixed0Cr4 = { 0 };
		cr4 fixed1Cr4 = { 0 };

		newCr4 = Cr4;
		fixed0Cr4.flags = __readmsr(IA32_VMX_CR4_FIXED0);
		fixed1Cr4.flags = __readmsr(IA32_VMX_CR4_FIXED1);
		newCr4.flags &= fixed1Cr4.flags;
		newCr4.flags |= fixed0Cr4.flags;
		return newCr4;
	}

	bool IsVmxSupported() {

		CPUID Data = { 0 };
		ia32_feature_control_register FeatureControlMsr = { 0 };

		// VMX bit
		__cpuid((int*)&Data, 1);
		if ((Data.ecx & (1 << 5)) == 0)
			return false;

		FeatureControlMsr.flags = __readmsr(IA32_FEATURE_CONTROL);

		// BIOS lock check
		if (FeatureControlMsr.lock_bit == 0)
		{
			FeatureControlMsr.lock_bit = TRUE;
			FeatureControlMsr.enable_vmx_outside_smx = TRUE;
			__writemsr(IA32_FEATURE_CONTROL, FeatureControlMsr.flags);
		}
		else if (FeatureControlMsr.enable_vmx_outside_smx == FALSE)
		{
			DBG_LOG("VMX feature set to off in BIOS");
			return false;
		}

		DBG_LOG("VMX supported");

		return true;
	}

	bool Start(vcpu* VCpu, ept_pointer EptPointer) {

		//Check Vmx support, adjust cr0 and cr4, vmxon, vmxprtld, vmxclear, vmlaunch

		cr0 AdjustedCr0 = { 0 };
		cr4 AdjustedCr4 = { 0 };

		if (!IsVmxSupported())
			return false;

		AdjustedCr0.flags = __readcr0();
		AdjustedCr0 = Vmx::AdjustCr0(AdjustedCr0);
		__writecr0(AdjustedCr0.flags);

		AdjustedCr4.flags = __readcr4();
		AdjustedCr4 = Vmx::AdjustCr4(AdjustedCr4);
		__writecr4(AdjustedCr4.flags);

		int Status = __vmx_on(&VCpu->PhysicalVMXONRegion);

		if (Status) {
			DBG_LOG("Vmxon Failed with %d", Status);
			return false;
		}

		DBG_LOG("Vmx On executed");


		SetupVmcs(VCpu, EptPointer);

		VCpu->HasLaunched = true;

		return true;
	}
};