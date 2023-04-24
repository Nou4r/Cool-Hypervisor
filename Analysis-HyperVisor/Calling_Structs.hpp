#pragma once
#include "includes.h"


struct EptHookType {
	bool Read;
	bool Write;
	bool Execute;
};

struct EptUsermodeHookStruct {
	void* TargetFunc;
	uint8_t* Patch;
	uint64_t PatchLen;

	uint64_t ProcessId;
};

struct EptHookStruct {
	uint64_t TargetFunc;
	uint64_t ReplacementFunc;
	void* OrigData;
	uint64_t ProcessId;

	EptHookType HookType;
};

struct EptUnHookStruct {
	uint64_t PhysAddr;
};


struct CopyMemoryRequest {
	void* Buffer; //You read from here
	uint64_t Addr;
	uint64_t Size;

	uint64_t SourceProcessId;
};

struct GetPidRequest {
	uint64_t Pid;

	const wchar_t* ProcessNameWchar;
	uint64_t ProcessNameWcharSize;
};

struct GetModuleBaseRequest {
	uint64_t Pid;
	uint64_t ModuleBase;

	const wchar_t* ProcessNameWchar;
	uint64_t ProcessNameWcharSize;

	const wchar_t* ModuleNameWchar;
	uint64_t ModuleNameWcharSize;

};
typedef struct CommandStruct {

	bool Result;
	uint64_t HyperCallCode;

	EptUsermodeHookStruct EptUsermodeHookCommand;
	EptHookStruct EptHookCommand;
	EptUnHookStruct EptUnHookCommand;
	CopyMemoryRequest CopyMemoryCommand;
	GetModuleBaseRequest ModuleBaseCommand;
	GetPidRequest GetPidCommand;
}CommandStruct, * PCommandStruct;