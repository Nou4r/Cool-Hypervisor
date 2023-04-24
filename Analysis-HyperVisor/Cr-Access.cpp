#include "includes.h"
#include "Undocumented.hpp"
#include "Func_defs.hpp"
#include "Calling_Structs.hpp"

namespace ExitRoutines {

	void HandleCr0Write(uint64_t Reg) {

		cr0 GuestCr0 = { 0 };

		GuestCr0.flags = Reg;
		__vmx_vmread(VMCS_GUEST_CR0, &GuestCr0.flags);

		GuestCr0.reserved1 = 0;
		GuestCr0.reserved2 = 0;
		GuestCr0.reserved3 = 0;

		GuestCr0.extension_type = 1;

		if (GuestCr0.reserved4) {
			Util::InjectInterupt(hardware_exception, general_protection, true, 0);
			return;
		}

		if (GuestCr0.paging_enable && !GuestCr0.protection_enable) {
			Util::InjectInterupt(hardware_exception, general_protection, true, 0);
			return;
		}

		if (GuestCr0.cache_disable && !GuestCr0.not_write_through) {
			Util::InjectInterupt(hardware_exception, general_protection, true, 0);
			return;
		}

		if (GuestCr0.paging_enable) {
			Util::InjectInterupt(hardware_exception, general_protection, true, 0);
			return;
		}

		//Ignore Paging structure writes as lack of motivation

		__vmx_vmwrite(VMCS_GUEST_CR0, Vmx::AdjustCr0(GuestCr0).flags);

	}

	void HandleCr3Write(uint64_t Reg) {

		bool FlushTlb = true;
		cr3 GuestCr3 = { 0 };
		cr4 GuestCr4 = { 0 };

		GuestCr3.flags = Reg;
		__vmx_vmread(VMCS_GUEST_CR4, &GuestCr4.flags);

		if (GuestCr4.pcid_enable && (GuestCr3.flags & (1ull << 63))) {
			GuestCr3.flags &= ~(1ull << 63);
			FlushTlb = false;
		}

		__vmx_vmwrite(VMCS_GUEST_CR3, GuestCr3.flags);

		Vpid::InvvpidSingleContext(0x1); //Since we set the vpid to 1 for all processors when in non root

	}

	void HandleCr4Write(uint64_t Reg) {
		//To do: Finish implementing this whole thing

		cr3 GuestCr3 = { 0 };
		cr4 GuestCr4 = { 0 };
		cr4 OldCr4 = { 0 };

		GuestCr4.flags = Reg;
		__vmx_vmread(VMCS_GUEST_CR3, &GuestCr3.flags);
		__vmx_vmread(VMCS_GUEST_CR4, &OldCr4.flags);

		if (GuestCr4.smx_enable) { //Just don't allow it in general and pretend it is not supported
			Util::InjectInterupt(hardware_exception, general_protection, true, 0);
			return;
		}

		if (GuestCr4.reserved1 || GuestCr4.reserved2){
			Util::InjectInterupt(hardware_exception, general_protection, true, 0);
			return;
		}

		if (GuestCr4.pcid_enable && !OldCr4.pcid_enable && (GuestCr3.flags & 0xFFF)) {
			Util::InjectInterupt(hardware_exception, general_protection, true, 0);
			return;
		}

		if (!GuestCr4.physical_address_extension) {
			Util::InjectInterupt(hardware_exception, general_protection, true, 0);
			return;
		}

		if (GuestCr4.page_global_enable != OldCr4.page_global_enable || 
			GuestCr4.pcid_enable && OldCr4.pcid_enable || 
			GuestCr4.smep_enable && !OldCr4.smep_enable) {
			Vpid::InvvpidSingleContext(0x1); //Since we set the vpid to 1 for all processors when in non root
		}
		
		__vmx_vmwrite(VMCS_GUEST_CR4, Vmx::AdjustCr4(GuestCr4).flags);
	}

	void HandleCr0Read(uint64_t* Reg) {

		__vmx_vmread(VMCS_GUEST_CR0, Reg);

	}

	void HandleCr3Read(uint64_t* Reg) {

		__vmx_vmread(VMCS_GUEST_CR3, Reg);

	}

	void HandleCr4Read(uint64_t* Reg) {

		__vmx_vmread(VMCS_GUEST_CR4, Reg);

	}

	void HandleCrAccess(pguest_registers GuestRegs) {

		vmx_exit_qualification_mov_cr ExitQualification = { 0 };
		uint64_t* Reg = 0;

		Reg = Util::SelectEffectiveRegister(GuestRegs, ExitQualification.general_purpose_register);

		__vmx_vmread(VMCS_EXIT_QUALIFICATION, &ExitQualification.flags);

		switch (ExitQualification.access_type)
		{
		case 0: //Mov to cr (write)
		{
			switch (ExitQualification.control_register)
			{
			case VMX_EXIT_QUALIFICATION_REGISTER_CR0:
			{
				HandleCr0Write(*Reg);
				break;
			}
			case VMX_EXIT_QUALIFICATION_REGISTER_CR3:
			{
				HandleCr3Write(*Reg);
				break;
			}
			case VMX_EXIT_QUALIFICATION_REGISTER_CR4:
			{
				HandleCr4Write(*Reg);
				break;
			}
			default:
			{
				DBG_LOG("Unsupported Cr Reg Write");
				break;
			}

			}
			break;
		}

		case 1: // Move from cr (read)
		{
			switch (ExitQualification.control_register)
			{
			case VMX_EXIT_QUALIFICATION_REGISTER_CR0:
			{
				HandleCr0Read(Reg);
				break;
			}
			case VMX_EXIT_QUALIFICATION_REGISTER_CR3:
			{
				HandleCr3Read(Reg);
				break;
			}
			case VMX_EXIT_QUALIFICATION_REGISTER_CR4:
			{
				HandleCr4Read(Reg);
				break;
			}
			default:
			{
				DBG_LOG("Unsupported Cr Reg Read");
				break;
			}
			}
			break;
		}
		default:
		{
			DBG_LOG("Unsupported operation: %d", ExitQualification.access_type);
			break;
		}
		}
	}
};