public GetCs
public GetDs
public GetEs
public GetSs
public GetFs
public GetGs
public GetLdtr
public GetTr
public	__load_ar 
public __reloadgdtr
public __reloadidtr
public __invept 
public __vmx_vmcall


.code


GetCs proc

	mov		rax, cs	
	ret

GetCs endp

;------------------------------------------------------------------------

GetDs proc

	mov		rax, ds	
	ret

GetDs endp

;------------------------------------------------------------------------

GetEs proc

	mov		rax, es	
	ret

GetEs endp

;------------------------------------------------------------------------

GetSs proc

	mov		rax, ss	
	ret

GetSs endp

;------------------------------------------------------------------------

GetFs proc

	mov		rax, fs	
	ret

GetFs endp

;------------------------------------------------------------------------

GetGs proc

	mov		rax, gs
	ret

GetGs endp

;------------------------------------------------------------------------

GetLdtr proc

	sldt	rax
	ret

GetLdtr endp

;------------------------------------------------------------------------

GetTr proc

	str		rax
	ret

GetTr endp

;------------------------------------------------------------------------

__reloadgdtr proc
	push	rbx
	shl		rcx, 48
	push	rcx
	lgdt	fword ptr [rsp+6]
	pop		rax
	pop		rax
	ret
__reloadgdtr endp

__reloadidtr proc
	push	rcx
	shl		rdx, 48
	push	rdx
	lidt	fword ptr [rsp+6]
	pop		rax
	pop		rax
	ret
__reloadidtr endp

__load_ar  proc
        lar     rax, rcx
        jz      no_error
        xor     rax, rax
no_error:
        ret
__load_ar  endp

__invept proc
	invept	rcx, oword ptr [rdx]
	jz @FailedWithStatus
	jc @FailedWithoutStatus
	xor rax, rax
	ret

	@FailedWithStatus:
		mov	rax, 1
		ret

	@FailedWithoutStatus:
		mov rax, 2
		ret
__invept endp

__invvpid proc
	invvpid rcx, oword ptr [rdx]
	jz @FailedWithStatus
	jc @FailedWithoutStatus
	xor rax, rax
	ret

	@FailedWithStatus:
		mov	rax, 1
		ret

	@FailedWithoutStatus:
		mov rax, 2
		ret
__invvpid endp

__vmx_vmcall proc
	vmcall	
	ret
__vmx_vmcall endp


end