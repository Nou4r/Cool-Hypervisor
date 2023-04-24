#include "includes.h"
#include "Undocumented.hpp"
#include "Func_defs.hpp"
#include "Calling_Structs.hpp"

namespace ExitRoutines {

	bool HandleEptViolation() {
		Pml2Entry* Pml2 = { 0 };
		Pml2Entry Pml2Dummy = { 0 };
		uint64_t GuestPhysicialAddr = 0;
		vmx_exit_qualification_ept_violation ExitQualification = { 0 };

		__vmx_vmread(VMCS_EXIT_QUALIFICATION, &ExitQualification.flags);
		__vmx_vmread(VMCS_GUEST_PHYSICAL_ADDRESS, &GuestPhysicialAddr);

		if (EptHook::HandleEptHook(GuestPhysicialAddr))
		{
			return true;
		}

		DBG_LOG("Unexpected Ept Violation occured");

		DBG_LOG("Ept Violation occured at Addr  0x%llx with Exit Qualification %d", Util::PhysicalAddressToVirtualAddress(GuestPhysicialAddr), ExitQualification.flags);

		Pml2 = Ept::LocatePml2Entry(GlobalExtendedPageTableState, GuestPhysicialAddr);

		DBG_LOG("Read Access to Page causing violation: %d", Pml2->read_access);
		DBG_LOG("Write Access to Page causing violation: %d", Pml2->write_access);
		DBG_LOG("Execute Access to Page causing violation: %d", Pml2->execute_access);
		DBG_LOG("Was Large Page : %d\n", Pml2->large_page);

		Util::InjectInterupt(hardware_exception, page_fault, true, 0);

		return false;
	}

};
