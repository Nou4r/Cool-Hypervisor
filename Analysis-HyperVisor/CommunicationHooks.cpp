#include "includes.h"
#include "Undocumented.hpp"
#include "Func_defs.hpp"
#include "Calling_Structs.hpp"



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
)
{
	static WCHAR BlockedFileName[] = L".IRISAKAPERLE"; //A girl (;
	static SIZE_T BlockedFileNameLength = (sizeof(BlockedFileName) / sizeof(BlockedFileName[0])) - 1;

	PWCH NameBuffer;
	USHORT NameLength;

	__try
	{

		ProbeForRead(ObjectAttributes, sizeof(OBJECT_ATTRIBUTES), 1);
		ProbeForRead(ObjectAttributes->ObjectName, sizeof(UNICODE_STRING), 1);

		NameBuffer = ObjectAttributes->ObjectName->Buffer;
		NameLength = ObjectAttributes->ObjectName->Length;

		ProbeForRead(NameBuffer, NameLength, 1);

		NameLength /= sizeof(WCHAR);

		if (Hv.PendingOperations.ModuleBase == true) {
			wchar_t ModuleName[MAX_PATH] = { 0 };
			wchar_t ProcessName[MAX_PATH] = { 0 };
			ModuleBaseRet ModBaseStruct = { 0 };
			Util::ReadFromMemoryByCr3((void*)Hv.PendingOperations.Cmd.ModuleBaseCommand.ProcessNameWchar, ProcessName, Hv.PendingOperations.Cmd.ModuleBaseCommand.ProcessNameWcharSize, Hv.PendingOperations.UserCr3);
			Util::ReadFromMemoryByCr3((void*)Hv.PendingOperations.Cmd.ModuleBaseCommand.ModuleNameWchar, ModuleName, Hv.PendingOperations.Cmd.ModuleBaseCommand.ModuleNameWcharSize, Hv.PendingOperations.UserCr3);

			DBG_LOG("%ws", ProcessName);
			DBG_LOG("%ws", ModuleName);

			ModBaseStruct = VmCallUtil::GetModuleBase(ProcessName, ModuleName);

			Hv.PendingOperations.Cmd.ModuleBaseCommand.Pid = ModBaseStruct.Pid;
			Hv.PendingOperations.Cmd.ModuleBaseCommand.ModuleBase = ModBaseStruct.ModuleBase;

			DBG_LOG("0x%llx", ModBaseStruct.Pid);
			DBG_LOG("0x%llx", ModBaseStruct.ModuleBase);

			Hv.PendingOperations.ModuleBase = false;
			Command::ReturnCmd(Hv.PendingOperations.Cmd, (uint64_t)Hv.PendingOperations.CmdPtr, Hv.PendingOperations.UserCr3, true);
			Hv.PendingOperations = { 0 };
		}

		if (NameLength >= BlockedFileNameLength && _wcsnicmp(&NameBuffer[NameLength - BlockedFileNameLength], BlockedFileName, BlockedFileNameLength) == 0)
		{
			DBG_LOG("Blocked access to a .IRISAKAPERLE file");

			return STATUS_ACCESS_DENIED;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		NOTHING;
	}

	HookedPage* pEntry = EptHook::FindHook(Ept::LocatePml1Entry(GlobalExtendedPageTableState, Util::VirtualAddressToPhysicalAddress((void*)(NtCreateFile))));

	if (pEntry)
		return ((NTSTATUS(*)(PHANDLE,
			ACCESS_MASK,
			POBJECT_ATTRIBUTES,
			PIO_STATUS_BLOCK,
			PLARGE_INTEGER,
			ULONG,
			ULONG,
			ULONG,
			ULONG,
			PVOID,
			ULONG))(ULONG_PTR)pEntry->PatchData)
		(FileHandle,
			DesiredAccess,
			ObjectAttributes,
			IoStatusBlock,
			AllocationSize,
			FileAttributes,
			ShareAccess,
			CreateDisposition,
			CreateOptions,
			EaBuffer,
			EaLength);
	return STATUS_UNSUCCESSFUL;
}

NTSTATUS NTAPI NtLoadDriverHook(PUNICODE_STRING DriverServiceName)
{
	NTSTATUS ret = STATUS_UNSUCCESSFUL;
	bool bLoad = true;
	DBG_LOG("Hi sir");
	DBG_LOG("Loading Driver: %ws\n", DriverServiceName->Buffer);

	if (bLoad)
	{
		HookedPage* pEntry = EptHook::FindHook(Ept::LocatePml1Entry(GlobalExtendedPageTableState, Util::VirtualAddressToPhysicalAddress((void*)(EptHook::GetSysCallAddress(0x0105)))));

		if (pEntry)
			return ((NTSTATUS(*)(PUNICODE_STRING))(ULONG_PTR)pEntry->PatchData)(DriverServiceName);
	}

	return ret;
}