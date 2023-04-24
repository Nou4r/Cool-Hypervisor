#pragma once
#include "includesUM.h"
//Asm
extern "C" uint64_t inline __vmx_vmcall(uint64_t VmcallNumber, uint64_t OptionalParam1, uint64_t OptionalParam2, uint64_t OptionalParam3);