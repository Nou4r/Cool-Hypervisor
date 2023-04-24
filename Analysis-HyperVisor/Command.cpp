#include "includes.h"
#include "Undocumented.hpp"
#include "Func_defs.hpp"
#include "Calling_Structs.hpp"

namespace Command {

	CommandStruct GetCmd(uint64_t CmdPtr, cr3 UserCr3) {
		CommandStruct Cmd = { 0 };
		PHYSICAL_ADDRESS PhysAddr = { 0 };

		Util::ReadFromMemoryByCr3((void*)CmdPtr, &Cmd, sizeof(CommandStruct), UserCr3);

		return Cmd;
	}

	void ReturnCmd(CommandStruct Cmd, uint64_t CommandPtr, cr3 UserCr3, bool ReturnValue) {

		Cmd.Result = ReturnValue;

		Util::WriteToMemoryByCr3((void*)CommandPtr, &Cmd, sizeof(CommandStruct), UserCr3);

		return;
	}

};