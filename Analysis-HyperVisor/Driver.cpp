#include "includes.h"
#include "Common.hpp"
#include "Func_defs.hpp"
#include "Calling_Structs.hpp"

Hypervisor Hv = { 0 };
EptState* GlobalExtendedPageTableState = { 0 };

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);

	PAGED_CODE();

	DBG_LOG_NOPREFIX("\n");

	DBG_LOG("Hypervisor Entry loaded");

	DriverObject->DriverUnload = DrvUnload;
	
	__try
	{
		memset(&Hv, 0, sizeof(Hv));

		Idt::image_base = DriverObject->DriverStart;

		Timing::PopulateTscOffsetTableBeforeLaunch();

		Hv.KernelBase = EptHook::SyscallHookGetKernelBase(&Hv.KernelSize);
		Hv.GetPidPage = ExAllocatePool2(POOL_FLAG_NON_PAGED_EXECUTE, PAGE_SIZE, (ULONG)POOL_TAG);

		DBG_LOG("Kernel Base: 0x%llx", Hv.KernelBase);
		DBG_LOG("Kernel Size: 0x%llx", Hv.KernelSize);

		Hv.vcpu_count = KeQueryActiveProcessorCount(0);

		auto const ArraySize = sizeof(vcpu) * Hv.vcpu_count;

		Hv.vcpus = reinterpret_cast<vcpu*>(ExAllocatePool2(POOL_FLAG_NON_PAGED, ArraySize, (ULONG)POOL_TAG));
		Hv.ListofPools = reinterpret_cast<LIST_ENTRY*>(ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(LIST_ENTRY), (ULONG)POOL_TAG));
		GlobalExtendedPageTableState = reinterpret_cast<EptState*>(ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(EptState), (ULONG)POOL_TAG));

		if (!Hv.vcpus || !GlobalExtendedPageTableState || !Hv.ListofPools) {
			DBG_LOG("Failed to allocate VCPUs or Extended Page Tables or list of pools.\n");
			return STATUS_UNSUCCESSFUL;
		}


		RtlZeroMemory(Hv.vcpus, ArraySize);
		RtlZeroMemory(Hv.GetPidPage, ArraySize);
		RtlZeroMemory(Hv.ListofPools, sizeof(LIST_ENTRY));
		RtlZeroMemory(GlobalExtendedPageTableState, sizeof(EptState));
		
		InitializeListHead(Hv.ListofPools);
		InitializeListHead(&GlobalExtendedPageTableState->HookedKernelPages);
		InitializeListHead(&GlobalExtendedPageTableState->HookedUsermodePages);

		DBG_LOG("Allocated %u VCPUs (0x%zX bytes).", Hv.vcpu_count, ArraySize);
		DBG_LOG("Allocated Ept-State (0x%zX bytes).", sizeof(EptState));
		DBG_LOG("Allocated List of Pools (0x%zX bytes).\n", sizeof(LIST_ENTRY));

		KAFFINITY AffinityMask;

		if (!Regions::AllocateGlobalRegions()) {
			DrvUnload(DriverObject);
			return STATUS_UNSUCCESSFUL;
		}

		//Allocate everything needed per Vcpu
		for (uint64_t i = 0; i < Hv.vcpu_count; i++) {
			DBG_LOG("--------------------------- Allocating %d ---------------------------\n", (int)i);

			AffinityMask = Util::MathPower(2, i);

			KeSetSystemAffinityThread(AffinityMask);

			//Allocate Regions
			if (!Regions::AllocateRegions(&Hv.vcpus[i])) {
				DrvUnload(DriverObject);
				return STATUS_UNSUCCESSFUL;
			}
		}

		if (!Ept::SetupEpt()) {
			DrvUnload(DriverObject);
			return STATUS_UNSUCCESSFUL;
		}

		for (uint64_t i = 0; i < Hv.vcpu_count; i++) {
			DBG_LOG("--------------------------- Starting %d ---------------------------\n", (int)i);

			AffinityMask = Util::MathPower(2, i);

			KeSetSystemAffinityThread(AffinityMask);

			//Start
			if (!Vmx::Start(&Hv.vcpus[i], GlobalExtendedPageTableState->EptPointer)) {
				DrvUnload(DriverObject);
			}

			DBG_LOG("Launching...\n");

			Hv.vcpus[KeGetCurrentProcessorNumber()].HasLaunched = true;
			
			if (!Launch()) {
				DBG_LOG("Vm Launch failed");
				DrvUnload(DriverObject);
			}
		}

		DBG_LOG("--------------------------- Vmx inited ---------------------------\n");

		DBG_LOG("--------------------------- Testing Phase ---------------------------\n");

		EptHookType Type = { 0 };
		Type.Execute = true;
		uint64_t Pid = (uint64_t)PsGetCurrentProcessId();
		void* NtCreateFileAddr = EptHook::GetSysCallAddress(0x0055);
		EptHook::EptHookPageKernel(NtCreateFileAddr, NtCreateFileHook, Pid, Type);

		DBG_LOG("Finishing timing measurements\n");

		Timing::PopulateTscOffsetTableAfterLaunch();
		Timing::FinishTscTablePopulating();

		DBG_LOG_NOPREFIX("\n");

		DBG_LOG("Fully loaded\n");

		return STATUS_SUCCESS;
		
	}
	__except (GetExceptionCode())
	{
		DBG_LOG("Failed to Load with Error Code %d", GetExceptionCode());
		return STATUS_UNSUCCESSFUL;
	}
}

void DrvUnload(PDRIVER_OBJECT Obj) {

	int NumofProcessors = KeQueryActiveProcessorCount(0);

	KAFFINITY AffinityMask;

	CommandStruct Cmd = { 0 };
	Cmd.HyperCallCode = HyperCallencrypt(HypercallEptUnHookAllPages);
	__vmx_vmcall(HyperCallSignature, (uint64_t)&Cmd, (uint64_t)PsGetCurrentProcessId(), 0);

	for (uint64_t i = 0; i < Hv.vcpu_count; i++) {
		DBG_LOG("--------------------------- Stopping Vmx %d ---------------------------\n", (int)i);

		AffinityMask = Util::MathPower(2, i);

		KeSetSystemAffinityThread(AffinityMask);
		Cmd.HyperCallCode = HyperCallencrypt(HypercallTurnoff);
		__vmx_vmcall(HyperCallSignature, (uint64_t)&Cmd, (uint64_t)PsGetCurrentProcessId(), 0);
	}

	for (uint64_t i = 0; i < NumofProcessors; i++) {
		DBG_LOG("--------------------------- Unloading %d ---------------------------", i);

		AffinityMask = Util::MathPower(2, i);

		KeSetSystemAffinityThread(AffinityMask);

		Regions::FreeRegions();

	}

	ExFreePoolWithTag(Hv.GetPidPage, (ULONG)POOL_TAG);
	ExFreePoolWithTag(Hv.vcpus, (ULONG)POOL_TAG);
	MmFreeContiguousMemory(GlobalExtendedPageTableState->EptPageTable);
	ExFreePoolWithTag(GlobalExtendedPageTableState, (ULONG)POOL_TAG);
	Regions::FreeGlobalRegions();

	DBG_LOG("Finished\n");

	UNREFERENCED_PARAMETER(Obj);
}
