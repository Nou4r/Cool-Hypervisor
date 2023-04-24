#pragma once

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

#define POOL_TAG  0x6672643067
#define VMXON_SIZE  PAGE_SIZE
#define VMCS_SIZE  PAGE_SIZE
#define LAPIC_SIZE  PAGE_SIZE
#define VMM_STACK_SIZE  KERNEL_STACK_SIZE
#define MSR_BITMAP_SIZE PAGE_SIZE
#define TrampolineSize 100

#define CpuidFeatureIdentifier          0x00000001
#define CpuidVendorandMaxFuncs          0x40000000
#define CpuidInterface                  0x40000001
#define HypervisorPresentBit            0x80000000


#define NUM_APIC_REGISTERS 256
#define EPT_MTRR_RANGE_DESCRIPTOR_MAX 9

#define Pml4Count 512
#define Pml3Count 512
#define Pml2Count 512 
#define Pml1Count 512 

#define TwoMbPageSize (uint64_t)(512 * PAGE_SIZE)
// Offset into the 1st paging structure (4096 byte)
#define ADDRMASK_EPT_PML1_OFFSET(_VAR_) (_VAR_ & 0xFFFULL)

// Index of the 1st paging structure (4096 byte)
#define ADDRMASK_EPT_PML1_INDEX(_VAR_) ((_VAR_ & 0x1FF000ULL) >> 12)

// Index of the 2nd paging structure (2MB)
#define ADDRMASK_EPT_PML2_INDEX(_VAR_) ((_VAR_ & 0x3FE00000ULL) >> 21)

// Index of the 3rd paging structure (1GB)
#define ADDRMASK_EPT_PML3_INDEX(_VAR_) ((_VAR_ & 0x7FC0000000ULL) >> 30)

// Index of the 4th paging structure (512GB)
#define ADDRMASK_EPT_PML4_INDEX(_VAR_) ((_VAR_ & 0xFF8000000000ULL) >> 39)


#define F_INVALID       0x01
#define F_PREFIX        0x02
#define F_REX           0x04
#define F_MODRM         0x08
#define F_SIB           0x10
#define F_DISP          0x20
#define F_IMM           0x40
#define F_RELATIVE      0x80

#define RESERVED_MSR_RANGE_LOW 0x40000000
#define RESERVED_MSR_RANGE_HI  0x400000FF

#define IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG    10

#define UNW_FLAG_NHANDLER  0
#define UNW_FLAG_EHANDLER  1
#define UNW_FLAG_UHANDLER  2
#define UNW_FLAG_CHAININFO 4