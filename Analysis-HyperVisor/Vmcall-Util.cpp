#include "includes.h"
#include "Undocumented.hpp"
#include "Func_defs.hpp"
#include "Calling_Structs.hpp"

namespace VmCallUtil {

	bool CopyMem(CopyMemoryRequest CopyStruct) {
		bool ReturnValue = false;
		void* Buff = (void*)ExAllocatePool2(POOL_FLAG_NON_PAGED, CopyStruct.Size, (ULONG)POOL_TAG);

		if (!Buff) {
			ReturnValue = false;
			return ReturnValue;
		}

		if (!Util::ReadFromMemoryByCr3(CopyStruct.Buffer, Buff, CopyStruct.Size, Util::GetCr3FromPid(CopyStruct.SourceProcessId))) {
			ReturnValue = false;
			return ReturnValue;
		}
		else
		{
			ReturnValue = true;
		}

		if (!Util::WriteToMemoryByCr3((void*)CopyStruct.Addr, Buff, CopyStruct.Size, Util::GetCr3FromPid(CopyStruct.SourceProcessId))) {
			ReturnValue = false;
			return ReturnValue;
		}
		{
			ReturnValue = true;
		}

		ExFreePoolWithTag(Buff, (ULONG)POOL_TAG);

		DBG_LOG("Addr: 0x%llx", CopyStruct.Addr);
		DBG_LOG("Buffer: 0x%llx", CopyStruct.Buffer);
		DBG_LOG("Size: 0x%llx", CopyStruct.Size);
		ReturnValue = true;

		return ReturnValue;
	}

	HANDLE GetPidByName(const wchar_t* ProcessName)
	{
		CHAR image_name[15] = { 0 };
		PEPROCESS sys_process = PsInitialSystemProcess;
		PEPROCESS cur_entry = sys_process;

		do
		{
			RtlCopyMemory((PVOID)(&image_name), (PVOID)((uintptr_t)cur_entry + ImageFileName), sizeof(image_name));

			if (crt_strcmp(image_name, ProcessName, true)) //Custom String comparison
			{
				DWORD active_threads = 0;
				RtlCopyMemory((PVOID)&active_threads, (PVOID)((uintptr_t)cur_entry + ActiveThreads), sizeof(active_threads));

				if (active_threads) {
					DBG_LOG("Got process");
					return PsGetProcessId(cur_entry);
				}
			}

			PLIST_ENTRY list = (PLIST_ENTRY)((uintptr_t)(cur_entry)+ActiveProcessLinks);
			cur_entry = (PEPROCESS)((uintptr_t)list->Flink - ActiveProcessLinks);

		} while (cur_entry != sys_process);

		DBG_LOG("Process not active");

		return 0;
	}


	ModuleBaseRet GetModuleBase(PCWSTR ProcessName, PCWSTR ModuleName) { //Cannot be called in a vmcall handler, needs a hooked Api for communication
		ModuleBaseRet Ret = { 0 };
		bool Status = false;
		UNICODE_STRING UnicodeProcessName = { 0 };
		HANDLE Pid = 0;
		PEPROCESS Proc = { 0 };
		PPEB PebPtr = 0;
		PEB Peb = { 0 };
		PEB_LDR_DATA LdrData = { 0 };


		RtlInitUnicodeString(&UnicodeProcessName, ProcessName);

		Pid = GetPidByName(ProcessName);

		if (!Pid) {
			return Ret;
		}

		Ret.Pid = (uint64_t)Pid;

		DBG_LOG("0x%llx", Pid);


		PsLookupProcessByProcessId(Pid, &Proc);

		PebPtr = PsGetProcessPeb(Proc);

		ObfDereferenceObject(Proc);

		if (!PebPtr) {
			DBG_LOG("Failed getting Peb");
			return Ret;
		}

		Status = Util::ReadFromMemoryByCr3(PebPtr, &Peb, sizeof(PEB), Util::GetCr3FromPid((uint64_t)Pid));

		if (!Status)
			return Ret;

		Status = Util::ReadFromMemoryByCr3(Peb.Ldr, &LdrData, sizeof(PEB_LDR_DATA), Util::GetCr3FromPid((uint64_t)Pid));

		if (!Status)
			return Ret;

		LIST_ENTRY* FirstModule = LdrData.ModuleListLoadOrder.Flink;
		LIST_ENTRY* CurrentModule = FirstModule;


		//First time using a do while loop; loop body gets executed at least once, therfore ListHead is not = to CurrentNode

		do
		{
			LDR_DATA_TABLE_ENTRY CurrentLdrEntry = { 0 };

			Status = Util::ReadFromMemoryByCr3(CurrentModule, &CurrentLdrEntry, sizeof(LDR_DATA_TABLE_ENTRY), Util::GetCr3FromPid((uint64_t)Pid));

			if (!Status) {
				DBG_LOG("Failed reading LDR_DATA_TABLE_ENTRY");
				return Ret;
			}

			if (CurrentLdrEntry.BaseDllName.Length > 0) {
				WCHAR dllNameBuffer[MAX_PATH] = { 0 };

				Status = Util::ReadFromMemoryByCr3(CurrentLdrEntry.BaseDllName.Buffer, &dllNameBuffer, CurrentLdrEntry.BaseDllName.Length, Util::GetCr3FromPid((uint64_t)Pid));

				if (!Status) {
					DBG_LOG("Failed reading Dll Buffer");
					return Ret;
				}

				dllNameBuffer[CurrentLdrEntry.BaseDllName.Length / sizeof(WCHAR)] = L'\0';

				DBG_LOG("DLL Name: %ws\n", dllNameBuffer);

				if (crt_strcmp(dllNameBuffer, ModuleName, true))
				{
					if (CurrentLdrEntry.DllBase != 0 && CurrentLdrEntry.SizeOfImage != 0)
					{
						DBG_LOG("Got Kernel32.dll Base 0x%llx with size 0x%llx", CurrentLdrEntry.DllBase, CurrentLdrEntry.SizeOfImage);
						Ret.ModuleBase = (uint64_t)CurrentLdrEntry.DllBase;
						return Ret;
					}
				}

			}

			CurrentModule = CurrentLdrEntry.InLoadOrderModuleList.Flink;

		} while (CurrentModule != FirstModule);

		return Ret;
	}


}
