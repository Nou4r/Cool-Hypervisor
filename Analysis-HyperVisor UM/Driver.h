#pragma once
#include "includesUM.h"
#include "Calling_StructsUM.h"

#define TrampolineSize 100
#define PAGE_SIZE 0x1000
#define HyperCallencrypt(_VAR_) (_VAR_ ^ 0x135A)
#define HyperCalldecrypt(_VAR_) (0x135A ^ _VAR_)
#define HyperCallSignature 0x6672643067
#define HypercallCopyMemory 0x2
#define HypercallModuleBase 0x3
#define HypercallEptUnHookSinglePage 0x4
#define HypercallEptUnHookAllPages 0x5
#define HypercallEptHook 0x6
#define HypercallEptUsermodeHook 0x7
#define HypercallTurnoff 0x10


namespace Driver {

	void Unhook();
	bool HookUsermodePage(void* TargetFunc, uint8_t* Patch, uint64_t PatchLen);

	bool Read(void* Buffer, void* Addr, uint64_t Size);//Only difference between the two is that you read buffer data on write
	bool Write(void* Buffer, void* Addr, uint64_t Size);//and on read you write to the buffer data
	uint64_t GetModuleBase(const wchar_t* ProcessName, const wchar_t* ModuleName);

}