#include "includes.h"
#include "Undocumented.hpp"
#include "Func_defs.hpp"
#include "Calling_Structs.hpp"

namespace ExitRoutines {

	uint64_t VmCallHandler(pguest_registers GuestRegs) {
		bool ReturnValue = false;
		uint64_t HyperCallSig = 0;
		uint64_t CommandPtr = 0;
		uint64_t TimeForEditingTscOffset = 0;
		uint64_t TscOffset = 0;
		CommandStruct Cmd = { 0 };
		HyperCallSig = GuestRegs->rbx;
		CommandPtr = GuestRegs->rcx;

		if (Hv.Timing.UseTimeDiffs) {
			TimeForEditingTscOffset = __rdtsc();
			__vmx_vmread(VMCS_CTRL_TSC_OFFSET, &TscOffset);

			TscOffset = TscOffset - Hv.Timing.VmxInstructionTimeDiff;

			__vmx_vmwrite(VMCS_CTRL_TSC_OFFSET, TscOffset);
			TimeForEditingTscOffset = __rdtsc() - TimeForEditingTscOffset;
		}

		Cmd = Command::GetCmd(CommandPtr, Util::GetCr3FromPid(GuestRegs->r8));

		if (HyperCallSig != HyperCallSignature) {
			Util::InjectInterupt(hardware_exception, invalid_opcode, false, 0);
			return 0;
		}

		DBG_LOG("Vmcall called with valid Signature");

		switch (HyperCalldecrypt(Cmd.HyperCallCode))
		{
			case HypercallEptHook:
			{
				EptHookStruct HookStruct = Cmd.EptHookCommand;
				if (!EptHook::EptHookPageKernel((void*)HookStruct.TargetFunc, (void*)HookStruct.ReplacementFunc, HookStruct.ProcessId, HookStruct.HookType))
					ReturnValue = false;
				else
				{
					ReturnValue = true;
				}
				goto ReturnSucess;
			}

			case HypercallEptUsermodeHook:
			{
				EptUsermodeHookStruct HookStruct = Cmd.EptUsermodeHookCommand;
				EptHook::EptHookPageUsermode((void*)HookStruct.TargetFunc, HookStruct.Patch, HookStruct.PatchLen, HookStruct.ProcessId);
				ReturnValue = true;
				goto ReturnSucess;
			}

			case HypercallEptUnHookSinglePage:
			{
				EptUnHookStruct HookStruct = Cmd.EptUnHookCommand;
				EptHook::UnhookSinglePage(HookStruct.PhysAddr);
				DBG_LOG("Unhooked single pages");
				ReturnValue = true;
				goto ReturnSucess;
			}

			case HypercallEptUnHookAllPages:
			{
				EptHook::UnhookAllPages();
				DBG_LOG("Unhooked all pages");
				ReturnValue = true;
				goto ReturnSucess;
			}

			case HypercallCopyMemory:
			{
				CopyMemoryRequest WriteStruct = Cmd.CopyMemoryCommand;

				ReturnValue = VmCallUtil::CopyMem(WriteStruct);

				goto ReturnSucess;
			}
			case HypercallModuleBase:
			{
				GetModuleBaseRequest ModuleBaseStruct = Cmd.ModuleBaseCommand;

				//Needs to be done as Module base getting doesn't work in vmcall handler, because maybe different context, not high enough irql or whatever
				Hv.PendingOperations.ModuleBase = true;
				Hv.PendingOperations.Cmd = Cmd;
				Hv.PendingOperations.CmdPtr = (void*)CommandPtr;
				Hv.PendingOperations.UserCr3 = Util::GetCr3FromPid(GuestRegs->r8);

				goto ReturnSucess;
			}

			case HypercallTurnoff:
			{
				DBG_LOG("Turned Vmx off");
				ReturnValue = true;
				goto ReturnTurnoff;
			}

			default:
			{
				DBG_LOG("Not implemented Vmcall");
				ReturnValue = false;
				break;
			}
		}

		//If we reached here vmcall got called with incorrect parameters
		Util::InjectInterupt(hardware_exception, invalid_opcode, false, 0);

		return 0;

	ReturnSucess:
		Command::ReturnCmd(Cmd, CommandPtr, Util::GetCr3FromPid(GuestRegs->r8), ReturnValue);
		DBG_LOG_NOPREFIX("");
		return 0;
	ReturnTurnoff:
		Command::ReturnCmd(Cmd, CommandPtr, Util::GetCr3FromPid(GuestRegs->r8), ReturnValue);
		DBG_LOG_NOPREFIX("");
		return 1;

	}

};