#include "includesUM.h"
#include "Calling_StructsUM.h"
#include "Driver.h"
#include "Func_defsUM.h"

namespace Driver {

	CommandStruct Cmd = { 0 };
	uint64_t Pid = 0;

	void Unhook() {

		Cmd = { 0 };

		Cmd.HyperCallCode = HyperCallencrypt(HypercallEptUnHookAllPages);

		__vmx_vmcall(HyperCallSignature, (uint64_t)&Cmd, GetCurrentProcessId(), 0);
	}

	bool HookUsermodePage(void* TargetFunc, uint8_t* Patch, uint64_t PatchLen) {

		Cmd = { 0 };

		Cmd.HyperCallCode = HyperCallencrypt(HypercallEptUsermodeHook);

		Cmd.EptUsermodeHookCommand.Patch = Patch;
		Cmd.EptUsermodeHookCommand.PatchLen = PatchLen;
		Cmd.EptUsermodeHookCommand.ProcessId = GetCurrentProcessId();
		Cmd.EptUsermodeHookCommand.TargetFunc = TargetFunc;

		__vmx_vmcall(HyperCallSignature, (uint64_t)&Cmd, GetCurrentProcessId(), 0);

		return Cmd.Result;
	}

	bool Write(void* Buffer, void* Addr, uint64_t Size) {
		Cmd = { 0 };

		Cmd.HyperCallCode = HyperCallencrypt(HypercallCopyMemory);

		Cmd.CopyMemoryCommand.Buffer = Buffer;
		Cmd.CopyMemoryCommand.Addr = (uint64_t)Addr;
		Cmd.CopyMemoryCommand.Size = Size;
		Cmd.CopyMemoryCommand.SourceProcessId = GetCurrentProcessId();

		__vmx_vmcall(HyperCallSignature, (uint64_t)&Cmd, GetCurrentProcessId(), 0);

		return Cmd.Result;
	}

	bool Read(void* Buffer, void* Addr, uint64_t Size) {
		Cmd = { 0 };

		Cmd.HyperCallCode = HyperCallencrypt(HypercallCopyMemory);

		Cmd.CopyMemoryCommand.Buffer = Addr;
		Cmd.CopyMemoryCommand.Addr = (uint64_t)Buffer;
		Cmd.CopyMemoryCommand.Size = Size;
		Cmd.CopyMemoryCommand.SourceProcessId = GetCurrentProcessId();

		__vmx_vmcall(HyperCallSignature, (uint64_t)&Cmd, GetCurrentProcessId(), 0);

		return Cmd.Result;
	}

	uint64_t GetModuleBase(const wchar_t* WcharProcessName, const wchar_t* WcharModuleName) {
		Cmd = { 0 };

		Cmd.HyperCallCode = HyperCallencrypt(HypercallModuleBase);

		Cmd.ModuleBaseCommand.ProcessNameWcharSize = (wcslen(WcharProcessName) + 1) * sizeof(wchar_t);
		Cmd.ModuleBaseCommand.ModuleNameWcharSize = (wcslen(WcharModuleName) + 1) * sizeof(wchar_t);
		Cmd.ModuleBaseCommand.ProcessNameWchar = WcharProcessName;
		Cmd.ModuleBaseCommand.ModuleNameWchar = WcharModuleName;

		Cmd.ModuleBaseCommand.Pid = GetCurrentProcessId();

		__vmx_vmcall(HyperCallSignature, (uint64_t)&Cmd, GetCurrentProcessId(), 0);

		Sleep(1000); // for now
		
		Pid = Cmd.ModuleBaseCommand.Pid;

		printf("Pid of Overwatch.exe: 0x%llx\n", Pid);

		return Cmd.ModuleBaseCommand.ModuleBase;
	}

}