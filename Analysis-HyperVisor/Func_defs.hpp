#pragma once
#include "Common.hpp"
#include "Undocumented.hpp"
#include "Calling_Structs.hpp"
#include "Definitions.hpp"

#pragma section(".idt",    read, write)
#pragma section(".nmi_stk",read, write)
#pragma section(".pf_stk", read, write)
#pragma section(".de_stk", read, write)
#pragma section(".gp_stk", read, write)

//Driver.cpp
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
void DrvUnload(PDRIVER_OBJECT Obj);

//CommunicationHook.cpp
NTSTATUS NtCreateFileHook(
	PHANDLE            FileHandle,
	ACCESS_MASK        DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	PIO_STATUS_BLOCK   IoStatusBlock,
	PLARGE_INTEGER     AllocationSize,
	ULONG              FileAttributes,
	ULONG              ShareAccess,
	ULONG              CreateDisposition,
	ULONG              CreateOptions,
	PVOID              EaBuffer,
	ULONG              EaLength
);

NTSTATUS NTAPI NtLoadDriverHook(PUNICODE_STRING DriverServiceName);

//Idt.cpp
namespace Idt
{
	__declspec(allocate(".nmi_stk"))  inline uint8_t nmi_stk[VMM_STACK_SIZE];
	__declspec(allocate(".pf_stk"))   inline uint8_t pf_stk[VMM_STACK_SIZE];
	__declspec(allocate(".de_stk"))   inline uint8_t de_stk[VMM_STACK_SIZE];
	__declspec(allocate(".gp_stk"))   inline uint8_t gp_stk[VMM_STACK_SIZE];
	__declspec(allocate(".idt"))      inline idt_entry_t table[256];
	enum ist_idx : uint8_t { nmi = 4, de = 5, pf = 6, gp = 7 };

	inline void* image_base = nullptr;
	idt_entry_t create_entry(idt_addr_t idt_handler, uint8_t ist_index);
}

namespace Regions {

	bool AllocateRegions(vcpu* Vcpu);
	bool AllocateGlobalRegions();

	bool FreeRegions();
	bool FreeGlobalRegions();
	uint64_t GetPool(uint64_t Type, uint64_t Size);
};

namespace Vmx {

	//Vmx.cpp
	cr0 AdjustCr0(cr0 Cr0);
	cr4 AdjustCr4(cr4 Cr0);

	bool Start(vcpu* VCpu, ept_pointer EptPointer);

	//Vmcs.cpp
	bool SetupVmcs(vcpu* Vcpu, ept_pointer EptPointer);

};

//Asm Exit Handler
extern "C" void AsmVmexitHandler(void);
extern "C" bool Launch(void);

//Exit Handler
extern "C" uint64_t VmexitHandler(pguest_registers GuestRegs);
extern "C" void VmResumeFunc(void);
void VmxVmxoff();

namespace ExitRoutines {

	//Cpuid.cpp
	bool HandleCpuid(pguest_registers  GuestRegs);
	//CrAccess.cpp
	void HandleCrAccess(pguest_registers GuestRegs);
	//Debug.cpp
	void HandleDebug();
	//EptExitOrutines.cpp
	bool HandleEptViolation();
	//Msr.cpp
	bool HandleRdmsr(pguest_registers  GuestRegs);
	bool HandleWrmsr(pguest_registers  GuestRegs);
	//Nmi.cpp
	void HandleExceptionOrNmi();
	void HandleNmiWindow();
	//VmCall.cpp
	uint64_t VmCallHandler(pguest_registers GuestRegs);
	//VmxInstruction.cpp
	void HandleVmxInstruction();
	//XSetBv.cpp
	bool HandleXSetBv(pguest_registers  GuestRegs);
};

namespace Command {

	CommandStruct GetCmd(uint64_t CmdPtr, cr3 UserCr3);
	void ReturnCmd(CommandStruct Cmd, uint64_t CommandPtr, cr3 UserCr3, bool ReturnValue);
};

namespace Ept {

	bool SetupEpt();

	bool Split2MbPage(EptState* EptPageTable, uint64_t PreAllocatedBuffer, uint64_t Physaddr);
	Pml1Entry* LocatePml1Entry(EptState* ExtendedPageTableState, uint64_t PhysicalAddr);
	Pml2Entry* LocatePml2Entry(EptState* ExtendedPageTableState, uint64_t PhysicalAddr);
};

namespace Vpid {

	void InvvpidIndividualAddress(uint16_t Vpid, uint64_t LinearAddress);
	void InvvpidSingleContext(uint16_t Vpid);
	void InvvpidAllContexts();
	void InvvpidSingleContextRetainingGlobals(uint16_t Vpid);
}

//Ldasm
unsigned int __fastcall ldasm(void* code, ldasm_data* ld, ULONG is64);

namespace EptHook {
	//Hooking.cpp
	HookedUsermodePage* FindUmHook(Pml1Entry* Page);
	HookedPage* FindHook(Pml1Entry* Page);
	bool HandleEptHook(uint64_t GuestPhysicialAddr);
	bool EptHookPageKernel(void* TargetFunction, void* ReplacementFunction, uint64_t ProcessId, EptHookType HookType);
	bool EptHookPageUsermode(void* TargetFunction, uint8_t* Patch, size_t PatchLen, uint64_t ProcessId);
	bool UnhookSingleUsermodePage(uint64_t PhysAddr);
	bool UnhookSinglePage(uint64_t PhysAddr);
	void UnhookAllPages();

	//SysCallHooking.cpp
	void* SyscallHookGetKernelBase(ULONG* pImageSize);
	void* GetSysCallAddress(uint32_t ApiNumber);
};

//Assembly Common
extern "C" segment_selector  GetCs(void);
extern "C" segment_selector  GetDs(void);
extern "C" segment_selector  GetEs(void);
extern "C" segment_selector  GetSs(void);
extern "C" segment_selector  GetFs(void);
extern "C" segment_selector  GetGs(void);
extern "C" segment_selector  GetLdtr(void);
extern "C" segment_selector  GetTr(void);
extern "C"  unsigned __int32 __load_ar(unsigned __int16 segment_selector);
extern "C" void __reloadgdtr(uint64_t GdtBase, uint64_t GdtLimit);
extern "C" void __reloadidtr(uint64_t GdtBase, uint64_t GdtLimit);
extern "C" unsigned char inline __invept(unsigned long Type, void* Descriptors);
extern "C" unsigned char inline __invvpid(unsigned long Type, void* Descriptors);
extern "C" uint64_t inline __vmx_vmcall(uint64_t VmcallNumber, uint64_t OptionalParam1, uint64_t OptionalParam2, uint64_t OptionalParam3);

//Idt Assembly
extern "C" void __gp_handler(void);
extern "C" void __pf_handler(void);
extern "C" void __de_handler(void);
extern "C" void __nmi_handler(void);
//Idt Handlers
extern "C" void nmi_handler(void);
extern "C" void seh_handler(pidt_regs_t regs);
extern "C" void seh_handler_ecode(pidt_regs_ecode_t regs);

namespace Timing {

	void PopulateTscOffsetTableBeforeLaunch();
	void PopulateTscOffsetTableAfterLaunch();
	void FinishTscTablePopulating();
}

namespace VmCallUtil {

	bool CopyMem(CopyMemoryRequest CopyStruct);
	HANDLE GetPidByName(const wchar_t* ProcessName);
	ModuleBaseRet GetModuleBase(PCWSTR ProcessName, PCWSTR ModuleName);
}

namespace Util {

	bool ReadFromMemoryByCr3(void* VirtualAddress, void* Buffer, size_t size, cr3 UserCr3);
	bool WriteToMemoryByCr3(void* VirtualAddress, void* Buffer, size_t size, cr3 UserCr3);
	uint64_t FindSystemDirectoryTableBase();
	uint64_t MathPower(uint64_t Base, uint64_t Exp);
	uint64_t PhysicalAddressToVirtualAddress(uint64_t PhysicalAddress);
	uint64_t VirtualAddressToPhysicalAddress(void* VirtualAddress);
	cr3 GetCr3FromPid(uint64_t Pid);
	cr3 SwitchOnAnotherProcessMemoryLayoutByCr3(cr3 TargetCr3);
	void RestoreToPreviousProcess(cr3 PreviousProcess);
	uint64_t VirtualAddressToPhysicalAddressByCr3(void* VirtualAddress, cr3 TargetCr3);
	uint64_t segment_base(segment_descriptor_register_64 const& gdtr, segment_selector const selector);
	unsigned __int32 access_right(unsigned __int16 segment_selector);
	void InjectInterupt(interruption_type Interupttype, exception_vector ExceptionVector, bool DeliverErrorCode, uint32_t ErrorCode);
	uint64_t* SelectEffectiveRegister(pguest_registers GuestContext, uint64_t RegisterIndex);
	inline BOOLEAN SpinlockTryLock(volatile LONG* Lock);
	void SpinlockLock(volatile LONG* Lock);
	void SpinlockUnlock(volatile LONG* Lock);
	void SetMtf(bool Value);
	void SetPml1AndInvept(Pml1Entry* EntryAddress, Pml1Entry EntryValue, invept_type InvalidationType);
};