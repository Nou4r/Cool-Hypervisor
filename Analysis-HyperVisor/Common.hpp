#pragma once
#include "includes.h"
#include "Undocumented.hpp" 
#include "Definitions.hpp"
#include "Calling_Structs.hpp"

typedef struct EptPageTable {

	//512 512Gb memory regions, each of which descrbes a 1Gb Region
	DECLSPEC_ALIGN(PAGE_SIZE) Pml4Pointer Pml4[Pml4Count]; //1 would be enough, but because we have to waste a whole page on it either way it doesn't matter
	
	//512 1Gb memory regions
	DECLSPEC_ALIGN(PAGE_SIZE) Pml3Pointer Pml3[Pml3Count];

	//512x512 2mb Entries for 1:1 mapping (identity mapping)
	DECLSPEC_ALIGN(PAGE_SIZE) Pml2Entry Pml2[Pml3Count][Pml2Count];

	//No Pml1 Paging structures allocated, because smallest paging structure in our Hypervisor are 2mb Entries except dynamic splits

};

typedef struct EptState {

	mtrr_range_descripor MemoryRanges[EPT_MTRR_RANGE_DESCRIPTOR_MAX];
	uint64_t NumofEnabledMemoryRegions;

	ept_pointer EptPointer;
	EptPageTable* EptPageTable;

	LIST_ENTRY HookedKernelPages;
	LIST_ENTRY HookedUsermodePages;

	uint64_t DefaultMemoryType;

};

typedef struct HookedPage {

	DECLSPEC_ALIGN(PAGE_SIZE) unsigned char ChangedPage[PAGE_SIZE];

	unsigned char PatchData[TrampolineSize];
	uint64_t PatchSize;

	LIST_ENTRY HookedPageEntry;

	Pml1Entry RestoreEntry;

	Pml1Entry ChangedEntry;
	Pml1Entry OrigEntry;

	Pml1Entry* TargetEntryAddr;

	uint64_t OrigPfn;
	uint64_t ExecutePfn;
};

typedef struct HookedUsermodePage {

	void* ChangedPage; //Has to be PAGE_SIZE
	uint64_t ChangedPageSize;

	void* PatchData; //Has to be size 100
	uint64_t PatchSize;

	LIST_ENTRY HookedPageEntry;

	Pml1Entry RestoreEntry;

	Pml1Entry ChangedEntry;
	Pml1Entry OrigEntry;

	Pml1Entry* TargetEntryAddr;

	uint64_t OrigPfn;
	uint64_t ExecutePfn;
};

typedef struct vcpu {

	uint64_t VMXONRegion; //Vmxon Region, basically only needed for vmx_on
	uint64_t PhysicalVMXONRegion; //Physaddr of Vmxon Region, basically only needed for vmx_on

	uint64_t VMCSRegion; //Virtual machine control structure; in my opinion the heart of the hypervisor
	uint64_t PhysicalVMCSRegion; //Physaddr of Virtual machine control structure; in my opinion the heart of the hypervisor

	uint64_t MsrBitmap; //Msr Bitmap; Set bits to get exits for reads/writes to specific msrs
	uint64_t PhysicalMsrBitmap;//Physaddr of Msr Bitmap; Set bits to get exits for reads/writes to specific msrs

	uint64_t HostIdt; //Host interupt descriptor table
	uint64_t ErrorCode;
	uint64_t HostGdt; //Host general descripotr table

	uint64_t VMMStack; //Base of stack which is used in vmexit handler

	bool HasLaunched;

	bool IsInRootMode;

	uint64_t PreviousInstruction; //Last instruction that exited

	uint64_t PreviousTsc; //Tsc on last exit	

	bool HideOverhead; //Used for hiding overhead of instructions e.g. cpuid still to be implemented
};

typedef struct TimeDiffs {
	bool UseTimeDiffs;

	uint64_t TimeForEditingTsc;
	uint64_t TimesEditedTsc;
	uint64_t AverageEditTime;

	uint64_t CpuidTimeDiff; // This basically is rdtsc/cpuid/rdstc after vmlaunch - rdtsc/cpuid/rdstc before launch
	uint64_t VmxInstructionTimeDiff;
};

typedef struct FuncAddr {
	void* ZwQSI;
};

typedef struct Pending {

	void* CmdPtr;
	CommandStruct Cmd;
	cr3 UserCr3;

	bool ModuleBase;
};

typedef struct Hypervisor {
	// dynamically allocated array of vcpus
	uint64_t vcpu_count;
	vcpu* vcpus;

	ULONG KernelSize;
	void* KernelBase;
	void* GetPidPage;

	TimeDiffs Timing;

	FuncAddr Funcdefs;

	Pending PendingOperations;

	LIST_ENTRY* ListofPools;
};

typedef struct PoolTable {

	uint64_t Addr;
	uint64_t Type;
	uint64_t Size;
	bool IsInUse;
	LIST_ENTRY PoolsList;
};


typedef struct DynamicSplit {

	DECLSPEC_ALIGN(PAGE_SIZE) Pml1Entry Pml1[Pml1Count];

	union
	{
		Pml2Entry* Pml2Entry;
		Pml2Pointer* Pml2Pointer;
	};

	LIST_ENTRY DynamicSplitList;
};

#pragma pack(push, 1)
typedef struct JumpThunkData
{
	UCHAR PushOp;
	ULONG AddressLow;
	ULONG MovOp;
	ULONG AddressHigh;
	UCHAR RetOp;
};
#pragma pack(pop)

typedef struct  ModuleBaseRet {
	uint64_t ModuleBase;
	uint64_t Pid;
};

//Globally used important variables
extern Hypervisor Hv;
extern EptState* GlobalExtendedPageTableState;

extern "C" uint64_t GuestRsp;
extern "C" uint64_t GuestRip;
extern volatile LONG LockForReadingPool;
extern volatile LONG LockForPml1Modification;
