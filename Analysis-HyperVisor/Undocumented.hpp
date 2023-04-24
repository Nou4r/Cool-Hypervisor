#pragma once
#include "includes.h"

extern "C" void _sgdt(void* gdtr);
extern "C" void _lgdt(void* gdtr);

typedef struct _SYSTEM_MODULE_ENTRY
{
    HANDLE Section;
    PVOID MappedBase;
    PVOID ImageBase;
    ULONG ImageSize;
    ULONG Flags;
    USHORT LoadOrderIndex;
    USHORT InitOrderIndex;
    USHORT LoadCount;
    USHORT OffsetToFileName;
    UCHAR FullPathName[256];
} SYSTEM_MODULE_ENTRY, * PSYSTEM_MODULE_ENTRY;

typedef struct _SYSTEM_MODULE_INFORMATION
{
    ULONG Count;
    SYSTEM_MODULE_ENTRY Module[1];
} SYSTEM_MODULE_INFORMATION, * PSYSTEM_MODULE_INFORMATION;

typedef struct _SYSTEM_THREAD_INFORMATION {
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER CreateTime;
    ULONG WaitTime;
    PVOID StartAddress;
    CLIENT_ID ClientId;
    KPRIORITY Priority;
    LONG BasePriority;
    ULONG ContextSwitchCount;
    ULONG State;
    KWAIT_REASON WaitReason;
} SYSTEM_THREAD_INFORMATION, * PSYSTEM_THREAD_INFORMATION;

typedef struct _SYSTEM_PROCESS_INFORMATION {
    ULONG NextEntryOffset;
    ULONG NumberOfThreads;
    LARGE_INTEGER WorkingSetPrivateSize;
    ULONG HardFaultCount;
    ULONG NumberOfThreadsHighWatermark;
    ULONGLONG CycleTime;
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER KernelTime;
    UNICODE_STRING ImageName;
    ULONG BasePriority;
    HANDLE UniqueProcessId;
    HANDLE InheritedFromUniqueProcessId;
    SIZE_T HandleCount;
    ULONG SessionId;
    ULONG_PTR UniqueProcessKey;
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivatePageCount;
    LARGE_INTEGER ReadOperationCount;
    LARGE_INTEGER WriteOperationCount;
    LARGE_INTEGER OtherOperationCount;
    LARGE_INTEGER ReadTransferCount;
    LARGE_INTEGER WriteTransferCount;
    LARGE_INTEGER OtherTransferCount;
} SYSTEM_PROCESS_INFORMATION, * PSYSTEM_PROCESS_INFORMATION;

#define ImageFileName 0x5a8 // EPROCESS::ImageFileName
#define ActiveThreads 0x5f0 // EPROCESS::ActiveThreads
#define ThreadListHead 0x5e0 // EPROCESS::ThreadListHead
#define ActiveProcessLinks 0x448 // EPROCESS::ActiveProcessLinks

template <typename str_type, typename str_type_2>
__forceinline bool crt_strcmp(str_type str, str_type_2 in_str, bool two)
{
    if (!str || !in_str)
        return false;

    wchar_t c1, c2;
#define to_lower(c_char) ((c_char >= 'A' && c_char <= 'Z') ? (c_char + 32) : c_char)

    do
    {
        c1 = *str++; c2 = *in_str++;
        c1 = to_lower(c1); c2 = to_lower(c2);

        if (!c1 && (two ? !c2 : 1))
            return true;

    } while (c1 == c2);

    return false;
}

extern "C" NTKERNELAPI PPEB NTAPI PsGetProcessPeb(IN PEPROCESS Process);

typedef enum _SYSTEM_INFORMATION_CLASS
{
    SystemProcessInformation = 5,
    SystemModuleInformation = 11,
    SystemKernelDebuggerInformation = 35
} SYSTEM_INFORMATION_CLASS, * PSYSTEM_INFORMATION_CLASS;

typedef NTSTATUS(NTAPI* ZWQUERYSYSTEMINFORMATION)(
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );

typedef struct _SSDTStruct
{
    LONG* pServiceTable;
    PVOID  pCounterTable;
#ifdef _WIN64
    UINT64 NumberOfServices;
#else
    ULONG NumberOfServices;
#endif
    PCHAR pArgumentTable;
} SSDTStruct, * PSSDTStruct;

typedef struct _NT_KPROCESS
{
    DISPATCHER_HEADER Header;
    LIST_ENTRY ProfileListHead;
    ULONG_PTR DirectoryTableBase;
    UCHAR Data[1];
}NT_KPROCESS, * PNT_KPROCESS;

typedef struct _CPUID
{
    int eax;
    int ebx;
    int ecx;
    int edx;
} CPUID;

typedef union __segment_selector_t
{
    struct
    {
        unsigned __int16 rpl : 2;
        unsigned __int16 table : 1;
        unsigned __int16 index : 13;
    };
    unsigned __int16 flags;
};

typedef union __segment_access_rights_t
{
    struct
    {
        unsigned __int32 type : 4;
        unsigned __int32 descriptor_type : 1;
        unsigned __int32 dpl : 2;
        unsigned __int32 present : 1;
        unsigned __int32 reserved0 : 4;
        unsigned __int32 available : 1;
        unsigned __int32 long_mode : 1;
        unsigned __int32 default_big : 1;
        unsigned __int32 granularity : 1;
        unsigned __int32 unusable : 1;
        unsigned __int32 reserved1 : 15;
    };
    unsigned __int32 flags;
};

typedef struct _guest_registers
{
    __m128 xmm0;
    __m128 xmm1;
    __m128 xmm2;
    __m128 xmm3;
    __m128 xmm4;
    __m128 xmm5;
    __m128 xmm6;
    __m128 xmm7;
    __m128 xmm8;
    __m128 xmm9;
    __m128 xmm10;
    __m128 xmm11;
    __m128 xmm12;
    __m128 xmm13;
    __m128 xmm14;
    __m128 xmm15;

    uint64_t padding_8b;

    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
} guest_registers, * pguest_registers;

typedef struct _idt_regs_ecode_t
{
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;

    uint64_t error_code;
    uint64_t rip;
    uint64_t cs_selector;
    ::rflags rflags;
    uint64_t rsp;
    uint64_t ss_selector;
} idt_regs_ecode_t, * pidt_regs_ecode_t;

typedef struct _idt_regs_t
{
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;

    uint64_t rip;
    uint64_t cs_selector;
    ::rflags rflags;
    uint64_t rsp;
    uint64_t ss_selector;
} idt_regs_t, * pidt_regs_t;

#pragma warning(push)
#pragma warning(disable : 4200)
#pragma warning(disable : 4201)
#pragma warning(disable : 4214)
typedef union _UNWIND_CODE {
    UINT8 CodeOffset;
    UINT8 UnwindOp : 4;
    UINT8 OpInfo : 4;
    UINT16 FrameOffset;
} UNWIND_CODE;

typedef struct _UNWIND_INFO {
    UINT8 Version : 3;
    UINT8 Flags : 5;
    UINT8 SizeOfProlog;
    UINT8 CountOfCodes;
    UINT8 FrameRegister : 4;
    UINT8 FrameOffset : 4;
    UNWIND_CODE UnwindCode[1];

    union {
        UINT32 ExceptionHandler;
        UINT32 FunctionEntry;
    };

    UINT32 ExceptionData[];
} UNWIND_INFO;
#pragma warning(pop)



typedef struct _SCOPE_RECORD {
    UINT32 BeginAddress;
    UINT32 EndAddress;
    UINT32 HandlerAddress;
    UINT32 JumpTarget;
} SCOPE_RECORD;

typedef struct _SCOPE_TABLE {
    UINT32 Count;
    SCOPE_RECORD ScopeRecords[1];
} SCOPE_TABLE;

typedef struct _RUNTIME_FUNCTION {
    UINT32 BeginAddress;
    UINT32 EndAddress;
    UINT32 UnwindData;
} RUNTIME_FUNCTION;

typedef union _idt_entry_t
{
    __m128 flags;
    struct
    {
        uint64_t offset_low : 16;
        uint64_t segment_selector : 16;
        uint64_t ist_index : 3;
        uint64_t reserved_0 : 5;
        uint64_t gate_type : 5;
        uint64_t dpl : 2;
        uint64_t present : 1;
        uint64_t offset_middle : 16;
        uint64_t offset_high : 32;
        uint64_t reserved_1 : 32;
    };
} idt_entry_t, * pidt_entry_t;

union idt_addr_t
{
    void* addr;
    struct
    {
        uint64_t offset_low : 16;
        uint64_t offset_middle : 16;
        uint64_t offset_high : 32;
    };
};

union msr_split
{
    uint64_t value;
    struct
    {
        uint64_t low : 32;
        uint64_t high : 32;
    };
};

typedef enum __msr_access_types {

    MsrRead,
    MsrWrite,

}MsrAccessTypes;


typedef struct _mtrr_range_descripor
{
    SIZE_T PhysicalBaseAddress;
    SIZE_T PhysicalEndAddress;
    UCHAR MemoryType;
} mtrr_range_descripor, * pmtrr_range_descripor;

typedef enum interrupt_type_t
{
    INTERRUPT_TYPE_EXTERNAL_INTERRUPT = 0,
    INTERRUPT_TYPE_RESERVED = 1,
    INTERRUPT_TYPE_NMI = 2,
    INTERRUPT_TYPE_HARDWARE_EXCEPTION = 3,
    INTERRUPT_TYPE_SOFTWARE_INTERRUPT = 4,
    INTERRUPT_TYPE_PRIVILEGED_SOFTWARE_INTERRUPT = 5,
    INTERRUPT_TYPE_SOFTWARE_EXCEPTION = 6,
    INTERRUPT_TYPE_OTHER_EVENT = 7
};

typedef enum AllocationIntention {
    HookPage,
    TrampolinePage,
    SplitPage,
    UsermodeHookPage,
};

typedef struct _ldasm_data
{
    UCHAR  flags;
    UCHAR  rex;
    UCHAR  modrm;
    UCHAR  sib;
    UCHAR  opcd_offset;
    UCHAR  opcd_size;
    UCHAR  disp_offset;
    UCHAR  disp_size;
    UCHAR  imm_offset;
    UCHAR  imm_size;
} ldasm_data;

#define ImageFileName 0x5a8 // EPROCESS::ImageFileName
#define ActiveThreads 0x5f0 // EPROCESS::ActiveThreads
#define ThreadListHead 0x5e0 // EPROCESS::ThreadListHead
#define ActiveProcessLinks 0x448 // EPROCESS::ActiveProcessLinks
typedef struct _PEB_LDR_DATA {
    ULONG Length;
    BOOLEAN Initialized;
    PVOID SsHandle;
    LIST_ENTRY ModuleListLoadOrder;
    LIST_ENTRY ModuleListMemoryOrder;
    LIST_ENTRY ModuleListInitOrder;
} PEB_LDR_DATA, * PPEB_LDR_DATA;

typedef struct _RTL_USER_PROCESS_PARAMETERS {
    BYTE           Reserved1[16];
    PVOID          Reserved2[10];
    UNICODE_STRING ImagePathName;
    UNICODE_STRING CommandLine;
} RTL_USER_PROCESS_PARAMETERS, * PRTL_USER_PROCESS_PARAMETERS;

typedef VOID(WINAPI* PPS_POST_PROCESS_INIT_ROUTINE)(VOID);

typedef struct _PEB {
    BYTE                          Reserved1[2];
    BYTE                          BeingDebugged;
    BYTE                          Reserved2[1];
    PVOID                         Reserved3[2];
    PPEB_LDR_DATA                 Ldr;
    PRTL_USER_PROCESS_PARAMETERS  ProcessParameters;
    PVOID                         Reserved4[3];
    PVOID                         AtlThunkSListPtr;
    PVOID                         Reserved5;
    ULONG                         Reserved6;
    PVOID                         Reserved7;
    ULONG                         Reserved8;
    ULONG                         AtlThunkSListPtr32;
    PVOID                         Reserved9[45];
    BYTE                          Reserved10[96];
    PPS_POST_PROCESS_INIT_ROUTINE PostProcessInitRoutine;
    BYTE                          Reserved11[128];
    PVOID                         Reserved12[1];
    ULONG                         SessionId;
} PEB, * PPEB;


typedef struct _LDR_DATA_TABLE_ENTRY {
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
    PVOID DllBase;
    PVOID EntryPoint;
    ULONG SizeOfImage;  // in bytes
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    ULONG Flags;  // LDR_*
    USHORT LoadCount;
    USHORT TlsIndex;
    LIST_ENTRY HashLinks;
    PVOID SectionPointer;
    ULONG CheckSum;
    ULONG TimeDateStamp;
} LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;

typedef union xcr0 { //Don't pin me on this struct I am not sure
    struct {
        uint64_t x87_support : 1;
#define XCR0_X87_SUPPORT_BIT                                        0
#define XCR0_X87_SUPPORT_FLAG                                       0x01
#define XCR0_X87_SUPPORT_MASK                                       0x01
#define XCR0_X87_SUPPORT(_)                                         (((_) >> 0) & 0x01)

        uint64_t sse_support : 1;
#define XCR0_SSE_SUPPORT_BIT                                        1
#define XCR0_SSE_SUPPORT_FLAG                                       0x02
#define XCR0_SSE_SUPPORT_MASK                                       0x01
#define XCR0_SSE_SUPPORT(_)                                         (((_) >> 1) & 0x01)

        uint64_t avx_support : 1;
#define XCR0_AVX_SUPPORT_BIT                                        2
#define XCR0_AVX_SUPPORT_FLAG                                       0x04
#define XCR0_AVX_SUPPORT_MASK                                       0x01
#define XCR0_AVX_SUPPORT(_)                                         (((_) >> 2) & 0x01)

        uint64_t bndreg_support : 1;
#define XCR0_BNDREG_SUPPORT_BIT                                     3
#define XCR0_BNDREG_SUPPORT_FLAG                                    0x08
#define XCR0_BNDREG_SUPPORT_MASK                                    0x01
#define XCR0_BNDREG_SUPPORT(_)                                      (((_) >> 3) & 0x01)

        uint64_t bndcsr_support : 1;
#define XCR0_BNDCSR_SUPPORT_BIT                                     4
#define XCR0_BNDCSR_SUPPORT_FLAG                                    0x10
#define XCR0_BNDCSR_SUPPORT_MASK                                    0x01
#define XCR0_BNDCSR_SUPPORT(_)                                      (((_) >> 4) & 0x01)

        uint64_t opmask_support : 1;
#define XCR0_OPMASK_SUPPORT_BIT                                     5
#define XCR0_OPMASK_SUPPORT_FLAG                                    0x20
#define XCR0_OPMASK_SUPPORT_MASK                                    0x01
#define XCR0_OPMASK_SUPPORT(_)                                      (((_) >> 5) & 0x01)

        uint64_t zmm_hi256_support : 1;
#define XCR0_ZMM_HI256_SUPPORT_BIT                                  6
#define XCR0_ZMM_HI256_SUPPORT_FLAG                                 0x40
#define XCR0_ZMM_HI256_SUPPORT_MASK                                 0x01
#define XCR0_ZMM_HI256_SUPPORT(_)                                   (((_) >> 6) & 0x01)

        uint64_t hi16_zmm_support : 1;
#define XCR0_HI16_ZMM_SUPPORT_BIT                                   7
#define XCR0_HI16_ZMM_SUPPORT_FLAG                                  0x80
#define XCR0_HI16_ZMM_SUPPORT_MASK                                  0x01
#define XCR0_HI16_ZMM_SUPPORT(_)                                    (((_) >> 7) & 0x01)

        uint64_t pkru_support : 1;
#define XCR0_PKRU_SUPPORT_BIT                                       9
#define XCR0_PKRU_SUPPORT_FLAG                                      0x200
#define XCR0_PKRU_SUPPORT_MASK                                      0x01
#define XCR0_PKRU_SUPPORT(_)                                        (((_) >> 9) & 0x01)

        uint64_t cet_ia32_support : 1;
#define XCR0_CET_IA32_SUPPORT_BIT                                  11
#define XCR0_CET_IA32_SUPPORT_FLAG                                 0x800
#define XCR0_CET_IA32_SUPPORT_MASK                                 0x01
#define XCR0_CET_IA32_SUPPORT(_)                                   (((_) >> 11) & 0x01)

        uint64_t pconfig_support : 1;
#define XCR0_PCONFIG_SUPPORT_BIT                                   12
#define XCR0_PCONFIG_SUPPORT_FLAG                                  0x1000
#define XCR0_PCONFIG_SUPPORT_MASK                                  0x01
#define XCR0_PCONFIG_SUPPORT(_)                                    (((_) >> 12) & 0x01)

        uint64_t reserved : 51;
    };

    uint64_t flags;
} xcr0;