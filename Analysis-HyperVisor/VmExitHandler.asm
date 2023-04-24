public  AsmVmexitHandler
public	Launch

extern VmexitHandler:PROC
extern VmResumeFunc:PROC

extern GuestRsp:QWORD   
extern GuestRip:QWORD   

.code


SaveGeneralRegs macro

        push    rax
        push    rcx
        push    rdx
        push    rbx
        push    rbp
        push    rsi
        push    rdi
        push    r8
        push    r9
        push    r10
        push    r11
        push    r12
        push    r13
        push    r14
        push    r15
endm

RestoreGeneralRegs macro
        pop     r15
        pop     r14
        pop     r13
        pop     r12
        pop     r11
        pop     r10
        pop     r9
        pop     r8
        pop     rdi
        pop     rsi
        pop     rbp
        pop     rbx
        pop     rdx
        pop     rcx
        pop     rax
endm

Launch proc
  ; set VMCS_GUEST_RSP to the current value of RSP
  mov rax, 681Ch
  vmwrite rax, rsp

  ; set VMCS_GUEST_RIP to the address of <successful_launch>
  mov rax, 681Eh
  mov rdx, successful_launch
  vmwrite rax, rdx

  vmlaunch

  ; if we reached here, then we failed to launch
  xor al, al
  ret
  
successful_launch:
  mov al, 1
  ret
Launch endp

OffHandler proc 

    vmxoff 

    movups xmm0, [rsp]
	movups xmm1, [rsp + 010h]
	movups xmm2, [rsp + 020h]
	movups xmm3, [rsp + 030h]
	movups xmm4, [rsp + 040h]
	movups xmm5, [rsp + 050h]
	movups xmm6, [rsp + 060h]
	movups xmm7, [rsp + 070h]
	movups xmm8, [rsp + 080h]
	movups xmm9, [rsp + 090h]
	movups xmm10, [rsp + 0A0h]
	movups xmm11, [rsp + 0B0h]
	movups xmm12, [rsp + 0C0h]
	movups xmm13, [rsp + 0D0h]
	movups xmm14, [rsp + 0E0h]
	movups xmm15, [rsp + 0F0h]
	add rsp, 0108h ; 16 xmm registers... and +8 bytes for alignment...


    RestoreGeneralRegs

    mov rsp, GuestRsp; Not a problem if there are not more than 1 vmxstops at a time

    jmp GuestRip
OffHandler endp 


AsmVmexitHandler proc


    SaveGeneralRegs
	
    sub rsp, 0108h ; 16 per xmm register  and +8 bytes for alignment

	movaps [rsp], xmm0
	movaps [rsp + 010h], xmm1
	movaps [rsp + 020h], xmm2
	movaps [rsp + 030h], xmm3
	movaps [rsp + 040h], xmm4
	movaps [rsp + 050h], xmm5
	movaps [rsp + 060h], xmm6
	movaps [rsp + 070h], xmm7
	movaps [rsp + 080h], xmm8
	movaps [rsp + 090h], xmm9
	movaps [rsp + 0A0h], xmm10
	movaps [rsp + 0B0h], xmm11
	movaps [rsp + 0C0h], xmm12
	movaps [rsp + 0D0h], xmm13
	movaps [rsp + 0E0h], xmm14
	movaps [rsp + 0F0h], xmm15

	mov rcx, rsp
	sub rsp, 20h
	call VmexitHandler
	add rsp, 20h
    cmp al, 1
    je OffHandler

	movups xmm0, [rsp]
	movups xmm1, [rsp + 010h]
	movups xmm2, [rsp + 020h]
	movups xmm3, [rsp + 030h]
	movups xmm4, [rsp + 040h]
	movups xmm5, [rsp + 050h]
	movups xmm6, [rsp + 060h]
	movups xmm7, [rsp + 070h]
	movups xmm8, [rsp + 080h]
	movups xmm9, [rsp + 090h]
	movups xmm10, [rsp + 0A0h]
	movups xmm11, [rsp + 0B0h]
	movups xmm12, [rsp + 0C0h]
	movups xmm13, [rsp + 0D0h]
	movups xmm14, [rsp + 0E0h]
	movups xmm15, [rsp + 0F0h]
	add rsp, 0108h ; 16 per xmm register  and +8 bytes for alignment

    RestoreGeneralRegs

    jmp VmResumeFunc

AsmVmexitHandler endp

end 