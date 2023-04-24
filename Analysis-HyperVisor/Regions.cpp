#include "includes.h"
#include "Common.hpp"
#include "ia32.hpp"
#include "Func_defs.hpp"

namespace Regions {

	volatile LONG LockForReadingPool = 0;

	bool AllocateVmxOn(vcpu* Vcpu){

		PAGED_CODE()
		ia32_vmx_basic_register VmxBasic = { 0 };
		PHYSICAL_ADDRESS PhysicalMax = { 0 };
		uint64_t dummy = 0;
		PhysicalMax.QuadPart = MAXULONG64;

		Vcpu->VMXONRegion = (uint64_t)MmAllocateContiguousMemory(VMXON_SIZE, PhysicalMax);

		if (Vcpu->VMXONRegion == NULL) {

			DBG_LOG("Couldn't allocate Vmxon");
			return false;
		}

		RtlZeroMemory((void*)Vcpu->VMXONRegion, VMXON_SIZE);

		VmxBasic.flags = __readmsr(IA32_VMX_BASIC);
		dummy = Vcpu->VMXONRegion;

		*(uint64_t*)dummy = VmxBasic.vmcs_revision_id;

		Vcpu->PhysicalVMXONRegion = Util::VirtualAddressToPhysicalAddress((void*)Vcpu->VMXONRegion);


		DBG_LOG("Vmx On Region Virtual Address : 0x%llx", Vcpu->VMXONRegion);
		DBG_LOG("Vmx On Region Physical Address : 0x%llx", Vcpu->PhysicalVMXONRegion);

		return true;
	}

	bool AllocateVmcs(vcpu* Vcpu){

		PAGED_CODE()
		ia32_vmx_basic_register VmxBasic = { 0 };
		PHYSICAL_ADDRESS PhysicalMax = { 0 };
		uint64_t dummy = 0;
		PhysicalMax.QuadPart = MAXULONG64;

		Vcpu->VMCSRegion = (uint64_t)MmAllocateContiguousMemory(VMCS_SIZE, PhysicalMax);

		if (Vcpu->VMCSRegion == NULL) {

			DBG_LOG("Couldn't allocate Vmcs");
			return false;
		}

		RtlZeroMemory((void*)Vcpu->VMCSRegion, VMCS_SIZE);

		VmxBasic.flags = __readmsr(IA32_VMX_BASIC);
		dummy = Vcpu->VMCSRegion;

		*(uint64_t*)dummy = VmxBasic.vmcs_revision_id;

		Vcpu->PhysicalVMCSRegion = Util::VirtualAddressToPhysicalAddress((void*)Vcpu->VMCSRegion);

		DBG_LOG("Vmcs Region Virtual Address : 0x%llx", Vcpu->VMCSRegion);
		DBG_LOG("Vmcs Region Physical Address : 0x%llx", Vcpu->PhysicalVMCSRegion);

		return true;
	}

	bool AllocateVmmStack(vcpu* Vcpu){

		PAGED_CODE()
		// Allocate stack for the VM Exit Handler.
		Vcpu->VMMStack = (uint64_t)ExAllocatePool2(POOL_FLAG_NON_PAGED, VMM_STACK_SIZE, (ULONG)POOL_TAG);

		if (Vcpu->VMMStack == NULL)
		{
			DBG_LOG("Insufficient memory in allocationg Vmm stack");
			return false;
		}
		RtlZeroMemory((void*)Vcpu->VMMStack, VMM_STACK_SIZE);

		DBG_LOG("Vmm Stack for logical processor : 0x%llx\n", Vcpu->VMMStack);

		return true;
	}


	bool AllocateMsrBitmap(vcpu* Vcpu){
		PAGED_CODE()
		// Allocate memory for MSRBitMap
		Vcpu->MsrBitmap = (uint64_t)ExAllocatePool2(POOL_FLAG_NON_PAGED, PAGE_SIZE, (ULONG)POOL_TAG);  // should be aligned

		if (Vcpu->MsrBitmap == 0)
		{
			DBG_LOG("Insufficient memory in allocationg Msr bitmaps");
			return false;
		}

		RtlZeroMemory((void*)Vcpu->MsrBitmap, PAGE_SIZE);

		Vcpu->PhysicalMsrBitmap = (uint64_t)Util::VirtualAddressToPhysicalAddress((void*)Vcpu->MsrBitmap);

		DBG_LOG("Msr Bitmap Virtual Address : 0x%llx", Vcpu->MsrBitmap);
		DBG_LOG("Msr Bitmap Physical Address : 0x%llx", Vcpu->PhysicalMsrBitmap);


		return true;
	}

	bool AllocateHostIdtandGdt(vcpu* Vcpu) {

		PAGED_CODE()
		// Allocate memory for MSRBitMap
		Vcpu->HostIdt = (uint64_t)ExAllocatePool2(POOL_FLAG_NON_PAGED, PAGE_SIZE, (ULONG)POOL_TAG);  // should be aligned
		Vcpu->HostGdt = (uint64_t)ExAllocatePool2(POOL_FLAG_NON_PAGED, PAGE_SIZE, (ULONG)POOL_TAG);  // should be aligned

		if (Vcpu->HostIdt == 0 || Vcpu->HostGdt == 0)
		{
			DBG_LOG("Insufficient memory in allocationg host idt/gdt");
			return false;
		}

		RtlZeroMemory((void*)Vcpu->HostIdt, PAGE_SIZE);
		RtlZeroMemory((void*)Vcpu->HostGdt, PAGE_SIZE);

		DBG_LOG("Host Idt Address : 0x%llx", Vcpu->HostIdt);
		DBG_LOG("Host Gdt Address : 0x%llx", Vcpu->HostGdt);


		return true;
	}

	bool AllocateSplitBuffer(uint64_t Count) {

		PAGED_CODE()

			for (uint64_t i = 0; i < Count; i++)
			{
				PoolTable* Head = (PoolTable*)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(PoolTable), (ULONG)POOL_TAG);

				if (!Head) {
					DBG_LOG("Failed to allocate Head!");
					return false;
				}

				RtlZeroMemory(Head, sizeof(PoolTable));
				PHYSICAL_ADDRESS MaxSize = { 0 };
				MaxSize.QuadPart = MAXULONG64;

				Head->Addr = (uint64_t)MmAllocateContiguousMemory(sizeof(DynamicSplit), MaxSize);

				if (!Head->Addr) {
					DBG_LOG("Failed to allocate Split buffer!");
					return false;
				}

				RtlZeroMemory((void*)Head->Addr, sizeof(DynamicSplit));

				Head->Type = SplitPage;
				Head->Size = sizeof(DynamicSplit);

				InsertHeadList(Hv.ListofPools, &Head->PoolsList);
			}

		return true;
	}

	bool AllocateUsermodePageHookBuffer(uint64_t Count) {

		PAGED_CODE()

			for (uint64_t i = 0; i < Count; i++)
			{
				PoolTable* Head = (PoolTable*)ExAllocatePool2(POOL_FLAG_NON_PAGED_EXECUTE, sizeof(PoolTable), (ULONG)POOL_TAG);

				if (!Head) {
					DBG_LOG("Failed to allocate Head!");
					return false;
				}

				RtlZeroMemory(Head, sizeof(PoolTable));

				Head->Addr = (uint64_t)ExAllocatePool2(POOL_FLAG_NON_PAGED_EXECUTE, sizeof(HookedUsermodePage), (ULONG)POOL_TAG);

				if (!Head->Addr) {
					DBG_LOG("Failed to allocate hooked page buffer!");
					return false;
				}

				RtlZeroMemory((void*)Head->Addr, sizeof(HookedUsermodePage));

				Head->Type = UsermodeHookPage;
				Head->Size = sizeof(HookedUsermodePage);

				InsertHeadList(Hv.ListofPools, &Head->PoolsList);
			}

		return true;
	}

	bool AllocatePageHookBuffer(uint64_t Count) {

		PAGED_CODE()

			for (uint64_t i = 0; i < Count; i++)
			{
				PoolTable* Head = (PoolTable*)ExAllocatePool2(POOL_FLAG_NON_PAGED_EXECUTE, sizeof(PoolTable), (ULONG)POOL_TAG);

				if (!Head) {
					DBG_LOG("Failed to allocate Head!");
					return false;
				}

				RtlZeroMemory(Head, sizeof(PoolTable));

				Head->Addr = (uint64_t)ExAllocatePool2(POOL_FLAG_NON_PAGED_EXECUTE, sizeof(HookedPage), (ULONG)POOL_TAG);

				if (!Head->Addr) {
					DBG_LOG("Failed to allocate hooked page buffer!");
					return false;
				}

				RtlZeroMemory((void*)Head->Addr, sizeof(HookedPage));

				Head->Type = HookPage;
				Head->Size = sizeof(HookedPage);

				InsertHeadList(Hv.ListofPools, &Head->PoolsList);
			}

		return true;
	}

	uint64_t GetPool(uint64_t Type, uint64_t Size) { //To do: find a way to call allocate pools with vmcall communication

		LIST_ENTRY* TempList = { 0 };
		uint64_t Addr = 0;

		TempList = Hv.ListofPools;

		Util::SpinlockLock(&LockForReadingPool);

		while (Hv.ListofPools != TempList->Flink) //Walk doubly linked list
		{
			TempList = TempList->Flink;

			PoolTable* PPoolTable = (PoolTable*)CONTAINING_RECORD(TempList, PoolTable, PoolsList);

			if (PPoolTable->Type == Type && !PPoolTable->IsInUse && PPoolTable->Size == Size) {
				PPoolTable->IsInUse = true;
				Addr = PPoolTable->Addr;
				break;
			}
		}

		Util::SpinlockUnlock(&LockForReadingPool);

		return Addr;
	}

	bool AllocateGlobalRegions() {

		if (!Regions::AllocateSplitBuffer(10))
			return false;

		if (!Regions::AllocatePageHookBuffer(10))
			return false;

		if (!Regions::AllocateUsermodePageHookBuffer(10))
			return false;

		return true;
	}

	bool FreeGlobalRegions() {

		LIST_ENTRY* TempList = { 0 };

		TempList = Hv.ListofPools;

		DBG_LOG("Freeing global regions...\n");

		while (Hv.ListofPools != TempList->Flink) //Walk doubly linked list
		{
			TempList = TempList->Flink;

			PoolTable* PPoolTable = (PoolTable*)CONTAINING_RECORD(TempList, PoolTable, PoolsList);

			if (PPoolTable->Type == SplitPage) {
				MmFreeContiguousMemory((void*)PPoolTable->Addr);
			}
			else
			{
				ExFreePoolWithTag((void*)PPoolTable->Addr, (ULONG)POOL_TAG);
			}


			ExFreePoolWithTag((void*)PPoolTable, (ULONG)POOL_TAG); //Free List Entry
		}

		ExFreePoolWithTag((void*)Hv.ListofPools, (ULONG)POOL_TAG); //Free List itself as it is a pool itself

		return true;
	}

	bool AllocateRegions(vcpu* Vcpu) {

		DBG_LOG("Allocating...\n");

		if (!Regions::AllocateHostIdtandGdt(Vcpu))
			return false;

		if (!Regions::AllocateVmxOn(Vcpu))
			return false;

		if (!Regions::AllocateVmcs(Vcpu))
			return false;

		if (!Regions::AllocateMsrBitmap(Vcpu))
			return false;

		if (!Regions::AllocateVmmStack(Vcpu))
			return false;

		return true;
	}


	bool FreeHostIdtandGdt(vcpu* Vcpu) {

		PAGED_CODE()

		ExFreePoolWithTag((void*)Vcpu->HostIdt, (ULONG)POOL_TAG);

		ExFreePoolWithTag((void*)Vcpu->HostGdt, (ULONG)POOL_TAG);

		return true;
	}

	bool FreeVmcs(vcpu* Vcpu) {

		PAGED_CODE()

			if (!Vcpu->VMCSRegion)
				return true;

		MmFreeContiguousMemory((void*)Vcpu->VMCSRegion);

		return true;
	}

	bool FreeVmxOn(vcpu* Vcpu) {

		PAGED_CODE()

			if (!Vcpu->VMXONRegion)
				return true;

		MmFreeContiguousMemory((void*)Vcpu->VMXONRegion);

		return true;
	}

	bool FreeMsrBitmap(vcpu* Vcpu) {

		PAGED_CODE()

			if (!Vcpu->MsrBitmap)
				return true;

		ExFreePoolWithTag((void*)Vcpu->MsrBitmap, (ULONG)POOL_TAG);

		return true;
	}

	bool FreeVmmStack(vcpu* Vcpu) {

		PAGED_CODE()

			if (!Vcpu->VMMStack)
				return true;

		ExFreePoolWithTag((void*)Vcpu->VMMStack, (ULONG)POOL_TAG);

		return true;
	}

	bool FreeRegions() {

		DBG_LOG("Freeing local regions...\n");

		vcpu* Vcpu;
		int index = KeGetCurrentProcessorNumber();
		Vcpu = &Hv.vcpus[index];


		if (!Regions::FreeVmxOn(Vcpu))
			return false;

		if (!Regions::FreeHostIdtandGdt(Vcpu))
			return false;

		if (!Regions::FreeVmcs(Vcpu))
			return false;

		if (!Regions::FreeMsrBitmap(Vcpu))
			return false;

		if (!Regions::FreeVmmStack(Vcpu))
			return false;

		return true;
	}

};