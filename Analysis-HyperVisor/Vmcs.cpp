#include "includes.h"
#include "Common.hpp"
#include "ia32.hpp"
#include "Func_defs.hpp"
#include "Undocumented.hpp"

namespace Vmx {

	bool ClearVmcs(vcpu* Vcpu) {

		int Status = __vmx_vmclear(&Vcpu->PhysicalVMCSRegion);

		if (Status) {
			DBG_LOG("Failed to clear Vmcs with Status %d", Status);
			return false;
		}

		DBG_LOG("Vmcs cleared");

		return true;
	}

	bool LoadVmcs(vcpu* Vcpu) {

		int Status = __vmx_vmptrld(&Vcpu->PhysicalVMCSRegion);

		if (Status) {
			DBG_LOG("Failed to load Vmcs with Status %d", Status);
			return false;
		}

		DBG_LOG("Vmcs loaded");

		return true;
	}

	/*List of things to setup:

	Host State:
		Register State:
				Cr0
				Cr3
				Cr4
				Rsp
				Rip
				CS:
					Selector
				SS:
					Selector
				DS:
					Selector
				ES:
					Selector
				FS:
					Selector
					Base
				GS:
					Selector
					Base
				TR:
					Selector
					Base
				GDTR:
					Base
				IDTR:
					Base

				List of Msrs:
					IA32_SYSENTER_CS
					IA32_SYSENTER_ESP
					IA32_SYSENTER_EIP

	Guest State:

		Register State:
				Cr0
				Cr3
				Cr4
				Dr7
				Rsp
				Rip
				RFlags
				CS:
					Selector
					Base
					Limit
					Access Rights
				SS:
					Selector
					Base
					Limit
					Access Rights
				DS:
					Selector
					Base
					Limit
					Access Rights
				ES:
					Selector
					Base
					Limit
					Access Rights
				FS:
					Selector
					Base
					Limit
					Access Rights
				GS:
					Selector
					Base
					Limit
					Access Rights
				LDTR:
					Selector
					Base
					Limit
					Access Rights
				TR:
					Selector
					Base
					Limit
					Access Rights
				GDTR:
					Base
					Limit
				IDTR:
					Base
					Limit

				List of Msrs:
					IA32_DEBUGCTL
					IA32_SYSENTER_CS
					IA32_SYSENTER_ESP
					IA32_SYSENTER_EIP
					(Rest of Msr specified in Sdm not necessary?)
		Non Register State:
			Activity State
			Interuptibility State
			Pending Debug Exceptions
			Vmcs Link Pointer to ~0ULL

	Something else:

		Execution Fields:
			Pin Based Vm-Execution Controls
			Primary Processor Based Vm-Execution Controls:
				Use Msr BitMaps (Zero all out, can be done when done setting up vmcs because atm I don't care about msr access)
				Activate Secondary Processor Base Vm-Execution Controls

			Secondary Processor Based Vm-Execution Controls:
				Enable Rdtscp

			Vm-Exit Controls:
				Vm-Exit Msr Load Count
				Vm-Exit Msr Load Address

			Vm-Entry Controls:
				Vm-Entry Msr Load Count
				Vm-Entry Msr Load Address
				Ignore Event injection


				now
	*/

	uint32_t
		AdjustControlValue(uint32_t VmxCapabilityMsr, uint64_t RequestedValue)
	{
		ia32_vmx_true_ctls_register Capabilities = { 0 };
		UINT32 effectiveValue;

		NT_ASSERT((VmxCapabilityMsr == IA32_VMX_PINBASED_CTLS) ||
			(VmxCapabilityMsr == IA32_VMX_PROCBASED_CTLS) ||
			(VmxCapabilityMsr == IA32_VMX_EXIT_CTLS) ||
			(VmxCapabilityMsr == IA32_VMX_ENTRY_CTLS) ||
			(VmxCapabilityMsr == IA32_VMX_TRUE_PINBASED_CTLS) ||
			(VmxCapabilityMsr == IA32_VMX_TRUE_PROCBASED_CTLS) ||
			(VmxCapabilityMsr == IA32_VMX_TRUE_EXIT_CTLS) ||
			(VmxCapabilityMsr == IA32_VMX_TRUE_ENTRY_CTLS) ||
			(VmxCapabilityMsr == IA32_VMX_PROCBASED_CTLS2));

		Capabilities.flags = __readmsr(VmxCapabilityMsr);
		effectiveValue = (uint32_t)RequestedValue;

		effectiveValue |= Capabilities.allowed_0_settings;
		effectiveValue &= Capabilities.allowed_1_settings;

		if ((effectiveValue & RequestedValue) != RequestedValue)
		{
			DBG_LOG("Not all the requested VM control fields are enabled.");
			DBG_LOG("MSR %08x: Requested %08x, Effective %08x",
				VmxCapabilityMsr,
				RequestedValue,
				effectiveValue);
		}
		return effectiveValue;
	}

	void AdjustPinbasedRegister(ia32_vmx_pinbased_ctls_register* Requestctls, ia32_vmx_basic_register BasicMsr) {


		UINT CapabilityMsr = 0;
		CapabilityMsr = BasicMsr.vmx_controls ? IA32_VMX_TRUE_PINBASED_CTLS : IA32_VMX_PINBASED_CTLS;
		Requestctls->flags = AdjustControlValue(CapabilityMsr, Requestctls->flags);;
	}

	void AdjustEntryControls(ia32_vmx_entry_ctls_register* Requestctls, ia32_vmx_basic_register BasicMsr) {


		UINT CapabilityMsr = 0;
		CapabilityMsr = BasicMsr.vmx_controls ? IA32_VMX_TRUE_ENTRY_CTLS : IA32_VMX_ENTRY_CTLS;
		Requestctls->flags = AdjustControlValue(CapabilityMsr, Requestctls->flags);;
	}

	void AdjustExitControls(ia32_vmx_exit_ctls_register* Requestctls, ia32_vmx_basic_register BasicMsr) {


		UINT CapabilityMsr = 0;
		CapabilityMsr = BasicMsr.vmx_controls ? IA32_VMX_TRUE_EXIT_CTLS : IA32_VMX_EXIT_CTLS;
		Requestctls->flags = AdjustControlValue(CapabilityMsr, Requestctls->flags);;
	}

	void AdjustPrimaryCpuBasedControls(ia32_vmx_procbased_ctls_register* Requestctls, ia32_vmx_basic_register BasicMsr) {


		UINT CapabilityMsr = 0;
		CapabilityMsr = BasicMsr.vmx_controls ? IA32_VMX_TRUE_PROCBASED_CTLS : IA32_VMX_PROCBASED_CTLS;
		Requestctls->flags = AdjustControlValue(CapabilityMsr, Requestctls->flags);;
	}

	void AdjustSecondaryCpuBasedControls(ia32_vmx_procbased_ctls2_register* Requestctls) {


		UINT CapabilityMsr = 0;
		CapabilityMsr = IA32_VMX_PROCBASED_CTLS2;
		Requestctls->flags = AdjustControlValue(CapabilityMsr, Requestctls->flags);;
	}

	bool SetupVmcs(vcpu* Vcpu, ept_pointer EptPointer) {
		if (!Vmx::ClearVmcs(Vcpu))
			return false;

		if (!Vmx::LoadVmcs(Vcpu))
			return false;

		DBG_LOG("Setting fields...");

		//To do: Set up gdt and idt properly to avoid detection

		segment_descriptor_register_64  gdtr = { 0 };

		_sgdt(&gdtr);

		segment_descriptor_register_64  idtr = { 0 };
		__sidt(&idtr);

		RtlCopyMemory(Idt::table, (void*)idtr.base_address, idtr.limit);

		Idt::table[general_protection] = Idt::create_entry(idt_addr_t{ __gp_handler }, Idt::ist_idx::gp);

		Idt::table[page_fault] = Idt::create_entry(idt_addr_t{ __pf_handler }, Idt::ist_idx::pf);

		Idt::table[divide_error] = Idt::create_entry(idt_addr_t{ __de_handler }, Idt::ist_idx::de);

		Idt::table[non_maskable_interrupt] = Idt::create_entry(idt_addr_t{ __nmi_handler }, Idt::ist_idx::nmi);

		//Used to determine true/normal Fields for everything but SecondaryProcBasedVmExecControls
		ia32_vmx_basic_register BasicMsr = { 0 };
		BasicMsr.flags = __readmsr(IA32_VMX_BASIC);

		//To do: Adjust the Control values based on Basic Msr

		ia32_vmx_pinbased_ctls_register Pinbasedctls = { 0 };
		Pinbasedctls.flags = 0;
		Pinbasedctls.nmi_exiting = true;
		Pinbasedctls.virtual_nmi = true;
		Vmx::AdjustPinbasedRegister(&Pinbasedctls, BasicMsr);


		ia32_vmx_entry_ctls_register Entryctls = { 0 };
		Entryctls.flags = 0;
		Entryctls.ia32e_mode_guest = true;
		//Entryctls.conceal_vmx_from_pt = true;
		Vmx::AdjustEntryControls(&Entryctls, BasicMsr);


		ia32_vmx_exit_ctls_register Exitctls = { 0 };
		Exitctls.flags = 0;
		Exitctls.host_address_space_size = true;
		//Exitctls.conceal_vmx_from_pt = true;
		Vmx::AdjustExitControls(&Exitctls, BasicMsr);


		ia32_vmx_procbased_ctls_register CpuPrimaryctls = { 0 };
		CpuPrimaryctls.flags = 0;
		CpuPrimaryctls.use_msr_bitmaps = true;
		CpuPrimaryctls.activate_secondary_controls = true;
		CpuPrimaryctls.use_tsc_offsetting = true;
		Vmx::AdjustPrimaryCpuBasedControls(&CpuPrimaryctls, BasicMsr);

		ia32_vmx_procbased_ctls2_register CpuSecondaryctls = { 0 };
		CpuSecondaryctls.flags = 0;
		CpuSecondaryctls.enable_rdtscp = true;
		CpuSecondaryctls.enable_invpcid = true;
		CpuSecondaryctls.enable_xsaves = true;
		CpuSecondaryctls.enable_ept = true;
		CpuSecondaryctls.enable_vpid = true;
		//CpuSecondaryctls.conceal_vmx_from_pt = true;

		Vmx::AdjustSecondaryCpuBasedControls(&CpuSecondaryctls);

		DBG_LOG("Adjusted Controls");

		//Host

		__vmx_vmwrite(VMCS_HOST_CR0, __readcr0());
		__vmx_vmwrite(VMCS_HOST_CR3, Util::FindSystemDirectoryTableBase());
		__vmx_vmwrite(VMCS_HOST_CR4, __readcr4());

		__vmx_vmwrite(VMCS_HOST_RSP, Vcpu->VMMStack + VMM_STACK_SIZE);
		__vmx_vmwrite(VMCS_HOST_RIP, (uint64_t)AsmVmexitHandler);

		__vmx_vmwrite(VMCS_HOST_CS_SELECTOR, GetCs().flags & 0xf8);
		__vmx_vmwrite(VMCS_HOST_SS_SELECTOR, GetSs().flags & 0xf8);
		__vmx_vmwrite(VMCS_HOST_DS_SELECTOR, GetDs().flags & 0xf8);
		__vmx_vmwrite(VMCS_HOST_ES_SELECTOR, GetEs().flags & 0xf8);
		__vmx_vmwrite(VMCS_HOST_FS_SELECTOR, GetFs().flags & 0xf8);
		__vmx_vmwrite(VMCS_HOST_GS_SELECTOR, GetGs().flags & 0xf8);
		__vmx_vmwrite(VMCS_HOST_TR_SELECTOR, GetTr().flags & 0xf8);


		__vmx_vmwrite(VMCS_HOST_FS_BASE, __readmsr(IA32_FS_BASE));
		__vmx_vmwrite(VMCS_HOST_GS_BASE, __readmsr(IA32_GS_BASE));
		__vmx_vmwrite(VMCS_HOST_TR_BASE, Util::segment_base(gdtr, GetTr()));
		__vmx_vmwrite(VMCS_HOST_GDTR_BASE, gdtr.base_address);
		__vmx_vmwrite(VMCS_HOST_IDTR_BASE, reinterpret_cast<uint64_t>(Idt::table));

		__vmx_vmwrite(VMCS_HOST_SYSENTER_CS, __readmsr(IA32_SYSENTER_CS));
		__vmx_vmwrite(VMCS_HOST_SYSENTER_ESP, __readmsr(IA32_SYSENTER_ESP));
		__vmx_vmwrite(VMCS_HOST_SYSENTER_EIP, __readmsr(IA32_SYSENTER_EIP));

		//Guest

		__vmx_vmwrite(VMCS_GUEST_CR0, __readcr0());
		__vmx_vmwrite(VMCS_GUEST_CR3, __readcr3());
		__vmx_vmwrite(VMCS_GUEST_CR4, __readcr4());
		__vmx_vmwrite(VMCS_GUEST_DR7, __readdr(7));


		__vmx_vmwrite(VMCS_GUEST_RSP, 0);//Gets set up in asm
		__vmx_vmwrite(VMCS_GUEST_RIP, 0);//Gets set up in asm

		__vmx_vmwrite(VMCS_GUEST_RFLAGS, __readeflags());

		//Selector
		__vmx_vmwrite(VMCS_GUEST_ES_SELECTOR, GetEs().flags);
		__vmx_vmwrite(VMCS_GUEST_CS_SELECTOR, GetCs().flags);
		__vmx_vmwrite(VMCS_GUEST_SS_SELECTOR, GetSs().flags);
		__vmx_vmwrite(VMCS_GUEST_DS_SELECTOR, GetDs().flags);
		__vmx_vmwrite(VMCS_GUEST_FS_SELECTOR, GetFs().flags);
		__vmx_vmwrite(VMCS_GUEST_GS_SELECTOR, GetGs().flags);
		__vmx_vmwrite(VMCS_GUEST_LDTR_SELECTOR, GetLdtr().flags);
		__vmx_vmwrite(VMCS_GUEST_TR_SELECTOR, GetTr().flags);

		//Limit
		__vmx_vmwrite(VMCS_GUEST_ES_LIMIT, __segmentlimit(GetEs().flags));
		__vmx_vmwrite(VMCS_GUEST_CS_LIMIT, __segmentlimit(GetCs().flags));
		__vmx_vmwrite(VMCS_GUEST_SS_LIMIT, __segmentlimit(GetSs().flags));
		__vmx_vmwrite(VMCS_GUEST_DS_LIMIT, __segmentlimit(GetDs().flags));
		__vmx_vmwrite(VMCS_GUEST_FS_LIMIT, __segmentlimit(GetFs().flags));
		__vmx_vmwrite(VMCS_GUEST_GS_LIMIT, __segmentlimit(GetGs().flags));
		__vmx_vmwrite(VMCS_GUEST_LDTR_LIMIT, __segmentlimit(GetLdtr().flags));
		__vmx_vmwrite(VMCS_GUEST_TR_LIMIT, __segmentlimit(GetTr().flags));
		__vmx_vmwrite(VMCS_GUEST_GDTR_LIMIT, gdtr.limit);
		__vmx_vmwrite(VMCS_GUEST_IDTR_LIMIT, idtr.limit);

		//Access Rights

		__vmx_vmwrite(VMCS_GUEST_ES_ACCESS_RIGHTS, Util::access_right(GetEs().flags));
		__vmx_vmwrite(VMCS_GUEST_CS_ACCESS_RIGHTS, Util::access_right(GetCs().flags));
		__vmx_vmwrite(VMCS_GUEST_SS_ACCESS_RIGHTS, Util::access_right(GetSs().flags));
		__vmx_vmwrite(VMCS_GUEST_DS_ACCESS_RIGHTS, Util::access_right(GetDs().flags));
		__vmx_vmwrite(VMCS_GUEST_FS_ACCESS_RIGHTS, Util::access_right(GetFs().flags));
		__vmx_vmwrite(VMCS_GUEST_GS_ACCESS_RIGHTS, Util::access_right(GetGs().flags));
		__vmx_vmwrite(VMCS_GUEST_LDTR_ACCESS_RIGHTS, Util::access_right(GetLdtr().flags));
		__vmx_vmwrite(VMCS_GUEST_TR_ACCESS_RIGHTS, Util::access_right(GetTr().flags));

		//Base

		__vmx_vmwrite(VMCS_GUEST_ES_BASE, Util::segment_base(gdtr, GetEs()));
		__vmx_vmwrite(VMCS_GUEST_CS_BASE, Util::segment_base(gdtr, GetCs()));
		__vmx_vmwrite(VMCS_GUEST_SS_BASE, Util::segment_base(gdtr, GetSs()));
		__vmx_vmwrite(VMCS_GUEST_DS_BASE, Util::segment_base(gdtr, GetDs()));
		__vmx_vmwrite(VMCS_GUEST_FS_BASE, __readmsr(IA32_FS_BASE));
		__vmx_vmwrite(VMCS_GUEST_GS_BASE, __readmsr(IA32_GS_BASE));
		__vmx_vmwrite(VMCS_GUEST_LDTR_BASE, Util::segment_base(gdtr, GetLdtr()));
		__vmx_vmwrite(VMCS_GUEST_TR_BASE, Util::segment_base(gdtr, GetTr()));
		__vmx_vmwrite(VMCS_GUEST_GDTR_BASE, gdtr.base_address);
		__vmx_vmwrite(VMCS_GUEST_IDTR_BASE, idtr.base_address);


		__vmx_vmwrite(VMCS_GUEST_SYSENTER_CS, __readmsr(IA32_SYSENTER_CS));
		__vmx_vmwrite(VMCS_GUEST_SYSENTER_ESP, __readmsr(IA32_SYSENTER_ESP));
		__vmx_vmwrite(VMCS_GUEST_SYSENTER_EIP, __readmsr(IA32_SYSENTER_EIP));
		__vmx_vmwrite(VMCS_GUEST_DEBUGCTL, __readmsr(IA32_DEBUGCTL));


		//Guest non reg state
		__vmx_vmwrite(VMCS_GUEST_VMCS_LINK_POINTER, MAXULONG64);

		//Control Fields

		__vmx_vmwrite(VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS, Pinbasedctls.flags);
		__vmx_vmwrite(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, CpuPrimaryctls.flags);
		__vmx_vmwrite(VMCS_CTRL_SECONDARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, CpuSecondaryctls.flags);
		__vmx_vmwrite(VMCS_CTRL_VMENTRY_CONTROLS, Entryctls.flags);
		__vmx_vmwrite(VMCS_CTRL_VMEXIT_CONTROLS, Exitctls.flags);
		__vmx_vmwrite(VMCS_CTRL_MSR_BITMAP_ADDRESS, Vcpu->PhysicalMsrBitmap);
		__vmx_vmwrite(VMCS_CTRL_EPT_POINTER, EptPointer.flags);
		__vmx_vmwrite(VMCS_CTRL_VIRTUAL_PROCESSOR_IDENTIFIER, 1);
		__vmx_vmwrite(VMCS_CTRL_VMENTRY_MSR_LOAD_COUNT, 0);
		__vmx_vmwrite(VMCS_CTRL_TSC_OFFSET, 0);

		return true;
	}
}