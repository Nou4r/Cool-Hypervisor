#pragma once
#pragma warning(disable: 4083 4005 4201 4091 4310 4471 28172) // Warnings that are kicked out cause they are actually flagging the included headers as 
#pragma warning(disable: 6328)							      // possibly flawed code, therefore pragma them away stuff like unnamed union etc.
#pragma comment(lib, "ntoskrnl.lib")

#include <ntifs.h>
#include <ntddk.h>
#include <wdf.h>
#include <wdm.h>
#include <cstdint>
#include <cstddef>
#include <ntimage.h>
#include <windef.h>
#include <intrin.h>
#include  <stdlib.h>
#include "ia32.hpp"



#define DBG_LOG(fmt, ...) DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[HyperVisor][" __FUNCTION__ "] " fmt "\n", ##__VA_ARGS__)
//#define DBG_LOG(fmt, ...) 
#define DBG_LOG_NOPREFIX(fmt, ...) DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "" fmt "\n", ##__VA_ARGS__)
//#define DBG_LOG_NOPREFIX(fmt, ...) 