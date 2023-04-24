#include "includes.h"
#include "Common.hpp"
#include "ia32.hpp"
#include "Func_defs.hpp"
#include "Undocumented.hpp"
#include "Calling_Structs.hpp"

namespace EptHook {

	void InitHookThunk(JumpThunkData* pThunk, uint64_t JmpAddr) {

		ULARGE_INTEGER* liTo = (ULARGE_INTEGER*)&JmpAddr;

		pThunk->PushOp = 0x68;
		pThunk->AddressLow = liTo->LowPart;
		pThunk->MovOp = 0x042444C7;
		pThunk->AddressHigh = liTo->HighPart;
		pThunk->RetOp = 0xC3;
	}

	bool CopyOrigCode(void* pFunc, unsigned char* OriginalStore, uint32_t* pSize) {
		// Store original bytes
		PUCHAR src = (PUCHAR)pFunc;
		PUCHAR old = OriginalStore;
		ULONG all_len = 0;
		ldasm_data ld = { 0 };

		do
		{
			ULONG len = ldasm(src, &ld, TRUE);

			// Determine code end
			if (ld.flags & F_INVALID
				|| (len == 1 && (src[ld.opcd_offset] == 0xCC || src[ld.opcd_offset] == 0xC3))
				|| (len == 3 && src[ld.opcd_offset] == 0xC2)
				|| len + all_len > 128)
			{
				break;
			}

			// move instruction 
			memcpy(old, src, len);

			// if instruction has relative offset, calculate new offset 
			if (ld.flags & F_RELATIVE)
			{
				LONG diff = 0;
				const uintptr_t ofst = (ld.disp_offset != 0 ? ld.disp_offset : ld.imm_offset);
				const uintptr_t sz = ld.disp_size != 0 ? ld.disp_size : ld.imm_size;

				memcpy(&diff, src + ofst, sz);
				// exit if jump is greater then 2GB
				if (_abs64(src + len + diff - old) > INT_MAX)
				{
					break;
				}
				else
				{
					diff += (LONG)(src - old);
					memcpy(old + ofst, &diff, sz);
				}
			}

			src += len;
			old += len;
			all_len += len;

		} while (all_len < sizeof(JumpThunkData));

		// Failed to copy old code, use backup plan
		if (all_len < sizeof(JumpThunkData))
		{
			return false;
		}
		else
		{
			InitHookThunk((JumpThunkData*)old, (ULONG64)src);
			*pSize = all_len;
		}

		return true;
	}

	bool EptHookPageKernel(void* TargetFunction, void* ReplacementFunction, uint64_t ProcessId, EptHookType HookType) {

		uint64_t PhysicalAddress = 0;
		uint64_t SplitBuffer = 0;
		uint64_t AlignedTargetFunction = 0;
		Pml1Entry* TargetPage = 0;
		Pml1Entry OrigPage = { 0 };
		Pml1Entry ChangedPage = { 0 };
		HookedPage* NewHook = 0;
		cr3 CurrentCr3 = { 0 };
		cr3 TargetProcessCr3 = { 0 };

		if (!Hv.vcpus[KeGetCurrentProcessorNumber()].IsInRootMode && Hv.vcpus[KeGetCurrentProcessorNumber()].HasLaunched)
		{
			DBG_LOG("Can't hook from non root mode if launched");
			DBG_LOG("Aquiring root mode through Vmcall...");
			CommandStruct Cmd = { 0 };

			Cmd.EptHookCommand.HookType = HookType;
			Cmd.EptHookCommand.TargetFunc = (uint64_t)TargetFunction;
			Cmd.EptHookCommand.ReplacementFunc = (uint64_t)ReplacementFunction;
			Cmd.EptHookCommand.ProcessId = ProcessId;
			Cmd.HyperCallCode = HyperCallencrypt(HypercallEptHook);

			__vmx_vmcall(HyperCallSignature, (uint64_t)&Cmd, (uint64_t)PsGetCurrentProcessId(), 0);
			return false;
		}

		//Needs to be aligned to page otherwise problems arise
		AlignedTargetFunction = (uint64_t)PAGE_ALIGN(TargetFunction);

		TargetProcessCr3 = Util::GetCr3FromPid(ProcessId);

		if (!TargetProcessCr3.flags)
			return false;

		PhysicalAddress = (uint64_t)Util::VirtualAddressToPhysicalAddressByCr3((void*)AlignedTargetFunction, TargetProcessCr3);

		if (!PhysicalAddress)
		{
			DBG_LOG("Target address could not be mapped to physical memory");
			return false;
		}
		//Important note: Allocate Split as Contiguous Memory otherewise translation failes
		SplitBuffer = Regions::GetPool(SplitPage, sizeof(DynamicSplit)); //Doesn't matter in which type we cast it here soo ye

		if (!Ept::Split2MbPage(GlobalExtendedPageTableState, SplitBuffer, PhysicalAddress))
		{
			DBG_LOG("Could not split page for the address : 0x%llx", PhysicalAddress);
			return false;
		}


		TargetPage = Ept::LocatePml1Entry(GlobalExtendedPageTableState, PhysicalAddress);

		// Ensure the target is valid. 
		if (!TargetPage)
		{
			DBG_LOG("Failed to get Pml1 entry of the target address");
			return false;
		}

		if (EptHook::FindHook(TargetPage)) {
			DBG_LOG("Page already hooked, not more than 1 hook per page possible");
			return false;
		}

		NewHook = (HookedPage*)Regions::GetPool(HookPage, sizeof(HookedPage));

		if (!NewHook) {
			DBG_LOG("Failed aquiring Pool for new hook");
			return false;
		}

		OrigPage = *TargetPage;
		NewHook->RestoreEntry = OrigPage;

		NewHook->TargetEntryAddr = TargetPage;

		NewHook->OrigPfn = TargetPage->page_frame_number;
		NewHook->ExecutePfn = Util::VirtualAddressToPhysicalAddress(&NewHook->ChangedPage[0]) / PAGE_SIZE;


		if (HookType.Read)
			ChangedPage.read_access = 0;

		if (HookType.Write)
			ChangedPage.write_access = 0;

		if (HookType.Execute)
		{
			OrigPage.read_access = 1;
			OrigPage.write_access = 1;
			OrigPage.execute_access = 0;
			NewHook->OrigEntry = OrigPage;

			ChangedPage.flags = 0;
			ChangedPage.read_access = 0;
			ChangedPage.write_access = 0;
			ChangedPage.execute_access = 1;
			ChangedPage.page_frame_number = NewHook->ExecutePfn;
			NewHook->ChangedEntry = ChangedPage;

			Util::ReadFromMemoryByCr3((void*)AlignedTargetFunction, &NewHook->ChangedPage[0], PAGE_SIZE, Util::GetCr3FromPid(ProcessId));

			if (!EptHook::CopyOrigCode(TargetFunction, NewHook->PatchData, (uint32_t*)&NewHook->PatchSize)) {
				DBG_LOG("Failed Copiyng Orig Code");
				return false;
			}

			JumpThunkData Thunk = { 0 };
			EptHook::InitHookThunk(&Thunk, (uint64_t)ReplacementFunction);

			Util::WriteToMemoryByCr3(&NewHook->ChangedPage[(uint64_t)TargetFunction - AlignedTargetFunction], &Thunk, sizeof(Thunk), TargetProcessCr3);
		}


		InsertHeadList(&GlobalExtendedPageTableState->HookedKernelPages, &NewHook->HookedPageEntry);

		Util::SetPml1AndInvept(TargetPage, ChangedPage, invept_all_context); //Start with Execute Page Present

		DBG_LOG("Hooked kernel page\n");

		return true;
	}

	bool EptHookPageUsermode(void* TargetFunction, uint8_t* Patch, size_t PatchLen, uint64_t ProcessId) {

		uint64_t PhysicalAddress = 0;
		uint64_t SplitBuffer = 0;
		uint64_t AlignedTargetFunction = 0;
		Pml1Entry* TargetPage = 0;
		Pml1Entry OrigPage = { 0 };
		Pml1Entry ChangedPage = { 0 };
		HookedPage* NewHook = 0;
		cr3 RootCr3 = { 0 };
		cr3 TargetProcessCr3 = { 0 };

		//Needs to be aligned to page otherwise problems arise
		AlignedTargetFunction = (uint64_t)PAGE_ALIGN(TargetFunction);
		RootCr3.flags = __readcr3();
		TargetProcessCr3 = Util::GetCr3FromPid(ProcessId);

		__writecr3(TargetProcessCr3.flags);

		if (!TargetProcessCr3.flags)
			return false;

		PhysicalAddress = (uint64_t)Util::VirtualAddressToPhysicalAddressByCr3((void*)AlignedTargetFunction, TargetProcessCr3);

		if (!PhysicalAddress)
		{
			DBG_LOG("Target address could not be mapped to physical memory");
			return false;
		}
		//Important note: Allocate Split as Contiguous Memory otherewise translation failes
		SplitBuffer = Regions::GetPool(SplitPage, sizeof(DynamicSplit)); //Doesn't matter in which type we cast it here soo ye

		if (!Ept::Split2MbPage(GlobalExtendedPageTableState, SplitBuffer, PhysicalAddress))
		{
			DBG_LOG("Could not split page for the address : 0x%llx", PhysicalAddress);
			return false;
		}


		TargetPage = Ept::LocatePml1Entry(GlobalExtendedPageTableState, PhysicalAddress);

		// Ensure the target is valid. 
		if (!TargetPage)
		{
			DBG_LOG("Failed to get Pml1 entry of the target address");
			return false;
		}

		if (EptHook::FindHook(TargetPage)) {
			DBG_LOG("Page already hooked, not more than 1 hook per page possible");
			return false;
		}

		NewHook = (HookedPage*)Regions::GetPool(HookPage, sizeof(HookedPage));

		if (!NewHook) {
			DBG_LOG("Failed aquiring Pool for new hook");
			return false;
		}

		OrigPage = *TargetPage;
		NewHook->RestoreEntry = OrigPage;

		NewHook->TargetEntryAddr = TargetPage;

		NewHook->OrigPfn = TargetPage->page_frame_number;
		NewHook->ExecutePfn = Util::VirtualAddressToPhysicalAddress(&NewHook->ChangedPage[0]) / PAGE_SIZE;

		OrigPage.read_access = 1;
		OrigPage.write_access = 1;
		OrigPage.execute_access = 0;
		NewHook->OrigEntry = OrigPage;

		ChangedPage.flags = 0;
		ChangedPage.read_access = 0;
		ChangedPage.write_access = 0;
		ChangedPage.execute_access = 1;
		ChangedPage.page_frame_number = NewHook->ExecutePfn;
		NewHook->ChangedEntry = ChangedPage;

		Util::ReadFromMemoryByCr3((void*)AlignedTargetFunction, &NewHook->ChangedPage[0], PAGE_SIZE, Util::GetCr3FromPid(ProcessId));

		uint8_t* Buff = (uint8_t*)ExAllocatePool2(POOL_FLAG_NON_PAGED, PatchLen, (ULONG)POOL_TAG);

		Util::ReadFromMemoryByCr3(Patch, Buff, PatchLen, TargetProcessCr3);

		Util::WriteToMemoryByCr3(&NewHook->ChangedPage[(uint64_t)TargetFunction - AlignedTargetFunction], Buff, PatchLen, TargetProcessCr3);

		ExFreePoolWithTag(Buff, (ULONG)POOL_TAG);

		InsertHeadList(&GlobalExtendedPageTableState->HookedKernelPages, &NewHook->HookedPageEntry);

		Util::SetPml1AndInvept(TargetPage, ChangedPage, invept_all_context); //Start with Execute Page Present

		__writecr3(RootCr3.flags);

		DBG_LOG("Hooked usermode page\n");

		return true;
	}

	bool HandleHookedPage(HookedPage* HookedEntry, HookedUsermodePage* HookedUmEntry) {

		vmx_exit_qualification_ept_violation Qualification = { 0 };

		__vmx_vmread(VMCS_EXIT_QUALIFICATION, &Qualification.flags);

		if (HookedEntry) {

			if (Qualification.read_access || Qualification.write_access) {
				DBG_LOG("Set Pfn from 0x%llx to 0x%llx and made Rw\n", HookedEntry->ExecutePfn, HookedEntry->OrigPfn); //Origentry.pfn = pfn on exit
				Util::SetPml1AndInvept(HookedEntry->TargetEntryAddr, HookedEntry->OrigEntry, invept_all_context);
				return true;
			}

			if (Qualification.execute_access) {
				DBG_LOG("Set Pfn from 0x%llx to 0x%llx and made Execute\n", HookedEntry->OrigPfn, HookedEntry->ExecutePfn); //Origentry.pfn = pfn on exit
				Util::SetPml1AndInvept(HookedEntry->TargetEntryAddr, HookedEntry->ChangedEntry, invept_all_context);
				return true;
			}
		}
		else
		{
			if (Qualification.read_access || Qualification.write_access) {
				DBG_LOG("Set Pfn from 0x%llx to 0x%llx and made Rw\n", HookedUmEntry->ExecutePfn, HookedUmEntry->OrigPfn); //Origentry.pfn = pfn on exit
				Util::SetPml1AndInvept(HookedUmEntry->TargetEntryAddr, HookedUmEntry->OrigEntry, invept_all_context);
				return true;
			}

			if (Qualification.execute_access) {
				DBG_LOG("Set Pfn from 0x%llx to 0x%llx and made Execute\n", HookedUmEntry->OrigPfn, HookedUmEntry->ExecutePfn); //Origentry.pfn = pfn on exit
				Util::SetPml1AndInvept(HookedUmEntry->TargetEntryAddr, HookedUmEntry->ChangedEntry, invept_all_context);
				return true;
			}
		}

		DBG_LOG("Invalid Page hook logic\n");

		return false;
	}

	bool HandleEptHook(uint64_t GuestPhysicialAddr) {
		Pml1Entry* HookedPml1 = 0;

		HookedPml1 = Ept::LocatePml1Entry(GlobalExtendedPageTableState, GuestPhysicialAddr);

		HookedPage* HookedEntry = EptHook::FindHook(HookedPml1);

		if (!HookedEntry) {
			HookedUsermodePage* HookedUmEntry = EptHook::FindUmHook(HookedPml1);
			if (!HookedUmEntry)
				return false;
			HandleHookedPage(0, HookedUmEntry);
			return true;
		}
		
		if (!HandleHookedPage(HookedEntry, 0))
			return false;

		return true;
	}

	bool UnhookSinglePage(uint64_t PhysAddr) {

		HookedPage* Hook = EptHook::FindHook(Ept::LocatePml1Entry(GlobalExtendedPageTableState, PhysAddr));

		if (!Hook)
			return false;

		Util::SetPml1AndInvept(Hook->TargetEntryAddr, Hook->RestoreEntry, invept_all_context);

		return true;
	}

	bool UnhookSingleUsermodePage(uint64_t PhysAddr) {

		HookedUsermodePage* Hook = EptHook::FindUmHook(Ept::LocatePml1Entry(GlobalExtendedPageTableState, PhysAddr));

		if (!Hook)
			return false;

		Util::SetPml1AndInvept(Hook->TargetEntryAddr, Hook->RestoreEntry, invept_all_context);

		return true;
	}

	void UnhookAllPages() {
		LIST_ENTRY* Templist = 0;
		Templist = &GlobalExtendedPageTableState->HookedKernelPages;

		while (&GlobalExtendedPageTableState->HookedKernelPages != Templist->Flink) {

			Templist = Templist->Flink;
			HookedPage* HookedEntry = CONTAINING_RECORD(Templist, HookedPage, HookedPageEntry);

			Util::SetPml1AndInvept(HookedEntry->TargetEntryAddr, HookedEntry->RestoreEntry, invept_all_context);
		}

		Templist = &GlobalExtendedPageTableState->HookedUsermodePages;

		while (&GlobalExtendedPageTableState->HookedUsermodePages != Templist->Flink) {

			Templist = Templist->Flink;
			HookedUsermodePage* HookedEntry = CONTAINING_RECORD(Templist, HookedUsermodePage, HookedPageEntry);

			Util::SetPml1AndInvept(HookedEntry->TargetEntryAddr, HookedEntry->RestoreEntry, invept_all_context);
		}
	}

	HookedPage* FindHook(Pml1Entry* Page) {

		LIST_ENTRY* Templist = 0;
		Templist = &GlobalExtendedPageTableState->HookedKernelPages;

		while (&GlobalExtendedPageTableState->HookedKernelPages != Templist->Flink) {

			Templist = Templist->Flink;
			HookedPage* HookedEntry = CONTAINING_RECORD(Templist, HookedPage, HookedPageEntry);

			if (HookedEntry->OrigPfn == Page->page_frame_number || HookedEntry->ExecutePfn == Page->page_frame_number) { //Upsie check for both pfns
				return HookedEntry;
			}
		}

		return 0;
	}

	HookedUsermodePage* FindUmHook(Pml1Entry* Page) {

		LIST_ENTRY* Templist = 0;
		Templist = &GlobalExtendedPageTableState->HookedUsermodePages;

		while (&GlobalExtendedPageTableState->HookedUsermodePages != Templist->Flink) {

			Templist = Templist->Flink;
			HookedUsermodePage* HookedEntry = CONTAINING_RECORD(Templist, HookedUsermodePage, HookedPageEntry);

			if (HookedEntry->OrigPfn == Page->page_frame_number || HookedEntry->ExecutePfn == Page->page_frame_number) { //Upsie check for both pfns
				return HookedEntry;
			}
		}

		return 0;
	}

};