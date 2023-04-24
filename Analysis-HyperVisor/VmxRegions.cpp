#include "includes.h"
#include "Common.h"
#include "ia32.hpp"
#include "Func_defs.h"

bool
AllcoateVmxOn(vcpu* Vcpu)
{
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

	RtlZeroMemory((PVOID)Vcpu->VMXONRegion, VMXON_SIZE);

	VmxBasic.flags = __readmsr(IA32_VMX_BASIC);
	dummy = Vcpu->VMXONRegion;

	*(uint64_t*)dummy = VmxBasic.vmcs_revision_id;

	Vcpu->PhysicalVMXONRegion = VirtualAddressToPhysicalAddress((PVOID)Vcpu->VMXONRegion);


	DBG_LOG("Vmx On Region Virtual Address : 0x%llx", Vcpu->VMXONRegion);
	DBG_LOG("Vmx On Region Physical Address : 0x%llx", Vcpu->PhysicalVMXONRegion);

	return true;
}

bool AllocateVmcs(vcpu* Vcpu)
{
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

	RtlZeroMemory((PVOID)Vcpu->VMCSRegion, VMXON_SIZE);

	VmxBasic.flags = __readmsr(IA32_VMX_BASIC);
	dummy = Vcpu->VMCSRegion;

	*(uint64_t*)dummy = VmxBasic.vmcs_revision_id;

	Vcpu->PhysicalVMCSRegion = VirtualAddressToPhysicalAddress((PVOID)Vcpu->VMCSRegion);

	DBG_LOG("Vmcs Region Virtual Address : 0x%llx", Vcpu->VMCSRegion);
	DBG_LOG("Vmcs Region Physical Address : 0x%llx", Vcpu->PhysicalVMCSRegion);

	return true;
}

bool AllocateVmmStack(vcpu* Vcpu)
{
	PAGED_CODE()
	// Allocate stack for the VM Exit Handler.
	Vcpu->VMMStack = (uint64_t)ExAllocatePool2(POOL_FLAG_NON_PAGED, VMM_STACK_SIZE, (ULONG)POOL_TAG);

	if (Vcpu->VMMStack == NULL)
	{
		DBG_LOG("Insufficient memory in allocationg Vmm stack");
		return false;
	}
	RtlZeroMemory((PVOID)Vcpu->VMMStack, VMM_STACK_SIZE);

	DBG_LOG("Vmm Stack for logical processor : 0x%llx\n", Vcpu->VMMStack);

	return true;
}


bool AllocateMsrBitmap(vcpu * Vcpu)
{
	PAGED_CODE()
	// Allocate memory for MSRBitMap
	Vcpu->MsrBitmap = (uint64_t)ExAllocatePool2(POOL_FLAG_NON_PAGED, PAGE_SIZE, (ULONG)POOL_TAG);  // should be aligned

	if (Vcpu->MsrBitmap == NULL)
	{
		DBG_LOG("Insufficient memory in allocationg Msr bitmaps");
		return false;
	}

	RtlZeroMemory((PVOID)Vcpu->MsrBitmap, PAGE_SIZE);

	Vcpu->PhysicalMsrBitmap = (uint64_t)VirtualAddressToPhysicalAddress((PVOID)Vcpu->MsrBitmap);

	DBG_LOG("Msr Bitmap Virtual Address : 0x%llx", Vcpu->MsrBitmap);
	DBG_LOG("Msr Bitmap Physical Address : 0x%llx", Vcpu->PhysicalMsrBitmap);



	return true;
}


bool AllocateRegions(vcpu* Vcpu) {
	DBG_LOG("Allocating...");

	if (!AllcoateVmxOn(Vcpu))
		return false;

	if (!AllocateVmcs(Vcpu))
		return false;

	if (!AllocateMsrBitmap(Vcpu))
		return false;

	if (!AllocateVmmStack(Vcpu))
		return false;


	return true;
}



bool FreeVmcs(vcpu* Vcpu) {
	PAGED_CODE()
	
	MmFreeContiguousMemory((PVOID)Vcpu->VMCSRegion);

	return true;
}

bool FreeVmxOn(vcpu* Vcpu) {
	PAGED_CODE()
	
	MmFreeContiguousMemory((PVOID)Vcpu->VMXONRegion);

	return true;
}

bool FreeMsrBitmap(vcpu* Vcpu) {

	PAGED_CODE()

	ExFreePoolWithTag((PVOID)Vcpu->MsrBitmap, (ULONG)POOL_TAG);

	return true;
}

bool FreeVmmStack(vcpu* Vcpu) {

	PAGED_CODE()

	ExFreePoolWithTag((PVOID)Vcpu->VMMStack, (ULONG)POOL_TAG);

	return true;
}


bool FreeRegions() {
	DBG_LOG("Freeing...\n");

	vcpu* Vcpu;
	int index = KeGetCurrentProcessorNumber();
	Vcpu = &Hv.vcpus[index];


	if (!FreeVmxOn(Vcpu))
		return false;

	if (!FreeVmcs(Vcpu))
		return false;

	if (!FreeMsrBitmap(Vcpu))
		return false;

	if (!FreeVmmStack(Vcpu))
		return false;

	return true;
}