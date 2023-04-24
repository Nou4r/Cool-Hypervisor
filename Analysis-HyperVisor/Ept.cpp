#include "includes.h"
#include "Common.hpp"
#include "ia32.hpp"
#include "Func_defs.hpp"
#include "Undocumented.hpp"

namespace Ept {

	bool IsEptSupported() {
		ia32_vmx_ept_vpid_cap_register VpidCapabilityRegister = { 0 };
		ia32_mtrr_def_type_register MtrrDefTypeRegister = { 0 };

		VpidCapabilityRegister.flags = __readmsr(IA32_VMX_EPT_VPID_CAP);
		MtrrDefTypeRegister.flags = __readmsr(IA32_MTRR_DEF_TYPE);


		DBG_LOG("Default Memory Type: %d", MtrrDefTypeRegister.default_memory_type);

		GlobalExtendedPageTableState->DefaultMemoryType = MtrrDefTypeRegister.default_memory_type;

		if (!VpidCapabilityRegister.page_walk_length_4) {

			DBG_LOG("Page Walk len 4 not supported");
			return false;
		}

		if (!VpidCapabilityRegister.memory_type_write_back) {
			DBG_LOG("Write back memory type is not supported");
			return false;
		}

		if (!VpidCapabilityRegister.pde_2mb_pages) {
			DBG_LOG("2 Mb Pages not supported");
			return false;
		}

		if (!VpidCapabilityRegister.advanced_vmexit_ept_violations_information) {

			DBG_LOG("Advanced Vm-Exit Information is not supported");
			return false;
		}

		if (!MtrrDefTypeRegister.mtrr_enable) {

			DBG_LOG("Dynamic Mtrr Ranges are not supported");
			return false;
		}

		if (!VpidCapabilityRegister.execute_only_pages) {
			DBG_LOG("Execute only pages are not supported, therefore page hooks won't work");
			return false;
		}

		DBG_LOG("Ept is fully supported by your Cores");

		return true;
	}


	EptPageTable* CreateIdentityPageTable() {

		EptPageTable* ExtendedPageTable = { 0 };
		PHYSICAL_ADDRESS MaxAddr = { 0 };
		MaxAddr.QuadPart = MAXULONG;

		ExtendedPageTable = (EptPageTable*)MmAllocateContiguousMemory(sizeof(EptPageTable), MaxAddr);

		if (!ExtendedPageTable) {
			DBG_LOG("Couldn't allocate Ept State");
			return 0;
		}

		RtlZeroMemory(ExtendedPageTable, sizeof(EptPageTable));


		//Only first Pml4 (512 Gb) entry needed, as we only need up to 512Gb of memory
		ExtendedPageTable->Pml4->flags = 0;
		ExtendedPageTable->Pml4->read_access = 1;
		ExtendedPageTable->Pml4->write_access = 1;
		ExtendedPageTable->Pml4->execute_access = 1;
		ExtendedPageTable->Pml4->page_frame_number = Util::VirtualAddressToPhysicalAddress(&ExtendedPageTable->Pml3[0]) >> 12;


		for (uint64_t i = 0; i < Pml3Count; i++) {

			ExtendedPageTable->Pml3[i].flags = 0;
			ExtendedPageTable->Pml3[i].read_access = 1;
			ExtendedPageTable->Pml3[i].write_access = 1;
			ExtendedPageTable->Pml3[i].execute_access = 1;
			ExtendedPageTable->Pml3[i].page_frame_number = Util::VirtualAddressToPhysicalAddress(&ExtendedPageTable->Pml2[i]) >> 12;

			for (uint64_t j = 0; j < Pml2Count; j++) {

				ExtendedPageTable->Pml2[i][j].flags = 0;
				ExtendedPageTable->Pml2[i][j].read_access = 1;
				ExtendedPageTable->Pml2[i][j].write_access = 1;
				ExtendedPageTable->Pml2[i][j].execute_access = 1;
				ExtendedPageTable->Pml2[i][j].large_page = 1;
				ExtendedPageTable->Pml2[i][j].page_frame_number = (i << 9) + j;
				//ExtendedPageTable->Pml2[i][j].memory_type = EptGetMemoryTypeforRange(MtrrData, ExtendedPageTable->Pml2[i][j].page_frame_number << 21, PAGE_SIZE << 9); //physbase btw
				//To do: Fix this and not just set everything wb (0x6)
				ExtendedPageTable->Pml2[i][j].memory_type = GlobalExtendedPageTableState->DefaultMemoryType;
			}
		}

		return ExtendedPageTable;
	}

	bool Split2MbPage(EptState* EptPageTable, uint64_t PreAllocatedBuffer, uint64_t Physaddr) {


		Pml2Entry* TargetEntry = 0;
		Pml2Pointer NewPml2Pointer = { 0 };
		DynamicSplit* NewDynamicSplit = 0;
		Pml1Entry Pml1 = { 0 };

		TargetEntry = Ept::LocatePml2Entry(EptPageTable, Physaddr);

		if (!TargetEntry) {
			DBG_LOG("Invalid physical addr passed to Split 2Mb Page");
			return false;
		}

		if (!TargetEntry->large_page) {
			return true; //Page already split job already done
		}

		NewDynamicSplit = (DynamicSplit*)PreAllocatedBuffer; //Should be same size as we allocate page size

		if (!NewDynamicSplit) {
			DBG_LOG("Failed to set Dynamic Split");
			return false;
		}

		RtlZeroMemory((void*)NewDynamicSplit, sizeof(DynamicSplit));

		NewDynamicSplit->Pml2Entry = TargetEntry;

		for (uint64_t i = 0; i < Pml1Count; i++) {

			NewDynamicSplit->Pml1[i].flags = 0;
			NewDynamicSplit->Pml1[i].read_access = 1;
			NewDynamicSplit->Pml1[i].write_access = 1;
			NewDynamicSplit->Pml1[i].execute_access = 1;
			NewDynamicSplit->Pml1[i].memory_type = TargetEntry->memory_type;
			NewDynamicSplit->Pml1[i].ignore_pat = TargetEntry->ignore_pat;
			NewDynamicSplit->Pml1[i].suppress_ve = TargetEntry->suppress_ve;
			NewDynamicSplit->Pml1[i].page_frame_number = ((TargetEntry->page_frame_number * TwoMbPageSize) >> 12) + i;
		}

		NewPml2Pointer.flags = 0;
		NewPml2Pointer.read_access = 1;
		NewPml2Pointer.write_access = 1;
		NewPml2Pointer.execute_access = 1;
		NewPml2Pointer.page_frame_number = Util::VirtualAddressToPhysicalAddress(&NewDynamicSplit->Pml1) / PAGE_SIZE;

		//Finally replace Pml2 largepage pointer with "normal" pml2 pointer which points to 512 pml1entries

		RtlCopyMemory(TargetEntry, &NewPml2Pointer, sizeof(NewPml2Pointer));

		return true;
	}

	bool SetupEpt() {

		ept_pointer Eptp = { 0 };
		EptPageTable* PageTable = { 0 };

		DBG_LOG_NOPREFIX("");

		if (!Ept::IsEptSupported()) {
			DBG_LOG("Ept not supported");
			return false;
		}

		PageTable = Ept::CreateIdentityPageTable();

		if (!PageTable) {

			DBG_LOG("Failed to set up Ept");
			return false;
		}

		GlobalExtendedPageTableState->EptPageTable = PageTable;

		Eptp.flags = 0;
		Eptp.memory_type = MEMORY_TYPE_WRITE_BACK; //Wb is cachable --> better performance
		Eptp.enable_access_and_dirty_flags = false;
		Eptp.page_walk_length = 3; // 4-1
		Eptp.page_frame_number = Util::VirtualAddressToPhysicalAddress(PageTable->Pml4) >> 12;

		GlobalExtendedPageTableState->EptPointer = Eptp;

		DBG_LOG("Set up Ept Structures\n");

		return true;
	}

	Pml2Entry* LocatePml2Entry(EptState* ExtendedPageTableState, uint64_t PhysicalAddr) {
		uint64_t Pml4Index, Pml3Index, Pml2Index;


		Pml2Index = ADDRMASK_EPT_PML2_INDEX(PhysicalAddr);
		Pml3Index = ADDRMASK_EPT_PML3_INDEX(PhysicalAddr);
		Pml4Index = ADDRMASK_EPT_PML4_INDEX(PhysicalAddr);

		if (Pml4Index > 0) {

			DBG_LOG("Above 512 gb --> invalid");
			return { 0 };
		}
		return &ExtendedPageTableState->EptPageTable->Pml2[Pml3Index][Pml2Index];
	}



	Pml1Entry* LocatePml1Entry(EptState* ExtendedPageTableState, uint64_t PhysicalAddr) {
		uint64_t Pml4Index, Pml3Index, Pml2Index;

		Pml2Entry* Pml2 = { 0 };
		Pml2Pointer* PML2Pointer = { 0 };
		Pml1Entry* Pml1 = 0;


		Pml2Index = ADDRMASK_EPT_PML2_INDEX(PhysicalAddr);
		Pml3Index = ADDRMASK_EPT_PML3_INDEX(PhysicalAddr);
		Pml4Index = ADDRMASK_EPT_PML4_INDEX(PhysicalAddr);

		if (Pml4Index > 0) {
			DBG_LOG("Above 512gb");
			return { 0 };
		}

		Pml2 = &ExtendedPageTableState->EptPageTable->Pml2[Pml3Index][Pml2Index];

		if (Pml2->large_page == 1) {
			DBG_LOG("Still large page");
			return { 0 };
		}

		PML2Pointer = (Pml2Pointer*)Pml2;

		Pml1 = (Pml1Entry*)Util::PhysicalAddressToVirtualAddress(PML2Pointer->page_frame_number * PAGE_SIZE);

		if (Pml1 == 0) {
			DBG_LOG("Failed to translate to Va");
			return { 0 };
		}

		Pml1 = &Pml1[ADDRMASK_EPT_PML1_INDEX(PhysicalAddr)];

		return Pml1;
	}
};