	TITLE	$csll

	.386
DGROUP  GROUP   CONST, _BSS, _DATA
PUBLIC	_CS_swuser
_DATA   SEGMENT  DWORD USE32 PUBLIC 'DATA'
EXTRN	_ex_sptr:DWORD		; external reference to _ex_sptr
EXTRN	_Cs_srv_block:BYTE	; external reference to _Cs_srv_block
_DATA      ENDS
_BSS    SEGMENT  DWORD USE32 PUBLIC 'BSS'
_BSS      ENDS
CONST   SEGMENT  DWORD USE32 PUBLIC 'CONST'
CONST      ENDS
_TEXT   SEGMENT  DWORD USE32 PUBLIC 'CODE'
        ASSUME   CS: _TEXT, DS: DGROUP, SS: DGROUP, ES: DGROUP

CS_CURRENT =		0b8h	; Cs_srv_block.cs_current (the current thread)
CS_INKERNEL =		0c8h	; Cs_srv_block.cs_kernel (is switching on?)
CS_MACHINE_SPECIFIC =	020h	; offset of machine dependent information

CS_SP   = 020h
CS_FP   = 024h
CS_EBX  = 028h
CS_ESI  = 02ch
CS_EDI  = 030h
CS_EXSP = 034h
;
; Define external procs.
;

EXTRN	_CS_xchng_thread:PROC

;
; CS_swuser.  Switch user.
;

_CS_swuser	PROC NEAR

;
; If server is in kernel state, no task switching (return immediately).
;
	mov	eax, DWORD PTR _Cs_srv_block + CS_INKERNEL
	cmp	eax, 0h
	jne	sw_ret
;
; If there is no current session, don't save registers, just
; pick a new thread.  This happens when a thread terminates.
;
	mov	eax, DWORD PTR _Cs_srv_block + CS_CURRENT
	cmp	eax, 0h
	je	CS_pick_new
;
; Save the state of the current thread.
;
	mov	ecx, _ex_sptr			; save EX stack pointer
	mov	DWORD PTR [eax + CS_EXSP], ecx

	mov	DWORD PTR [eax + CS_EBX], ebx
	mov	DWORD PTR [eax + CS_SP], esp
	mov	DWORD PTR [eax + CS_FP], ebp
	mov	DWORD PTR [eax + CS_ESI], esi
	mov	DWORD PTR [eax + CS_EDI], edi

;
; Call the scheduler to pick a new thread.  %eax contains
; the addr of current scb, and is passed as arg[0].
;

CS_pick_new:
	push	eax			; set up arg, addr of current scb
	call	_CS_xchng_thread

;
; Resume the new thread.
;

	mov	ebx, DWORD PTR [eax + CS_EXSP]	; restore EX stack pointer
	mov	_ex_sptr, ebx
	mov	ebx, DWORD PTR [eax + CS_EBX]
	mov	esp, DWORD PTR [eax + CS_SP]
	mov	ebp, DWORD PTR [eax + CS_FP]
	mov	esi, DWORD PTR [eax + CS_ESI]
	mov	edi, DWORD PTR [eax + CS_EDI]

;
; return
;

sw_ret:
	ret

_CS_swuser	ENDP

_TEXT	ENDS

DGROUP  GROUP   CONST, _BSS, _DATA
PUBLIC	_CS_tas
_DATA   SEGMENT  DWORD USE32 PUBLIC 'DATA'
_DATA      ENDS
_BSS    SEGMENT  DWORD USE32 PUBLIC 'BSS'
_BSS      ENDS
CONST   SEGMENT  DWORD USE32 PUBLIC 'CONST'
CONST      ENDS
_TEXT   SEGMENT  DWORD USE32 PUBLIC 'CODE'
        ASSUME   CS: _TEXT, DS: DGROUP, SS: DGROUP, ES: DGROUP

;
; CS_tas.  Test and set.
;

_CS_tas	PROC NEAR

	push	ebp

; get addr of tasbit

	mov	ebp, esp
	mov	ebp, DWORD PTR [ebp + 8]

; test and set

	mov	eax, 1
	xchg	al, BYTE PTR [ebp]

; compute return value

	xor	eax, 1

; return

	pop	ebp
	ret

_CS_tas	ENDP

_TEXT	ENDS

DGROUP  GROUP   CONST, _BSS, _DATA
PUBLIC	_CS_getspin
_DATA   SEGMENT  DWORD USE32 PUBLIC 'DATA'
_DATA      ENDS
_BSS    SEGMENT  DWORD USE32 PUBLIC 'BSS'
_BSS      ENDS
CONST   SEGMENT  DWORD USE32 PUBLIC 'CONST'
CONST      ENDS
_TEXT   SEGMENT  DWORD USE32 PUBLIC 'CODE'
        ASSUME   CS: _TEXT, DS: DGROUP, SS: DGROUP, ES: DGROUP

;
; CS_getspin.  Get spin lock.
;

_CS_getspin	PROC NEAR

	push	ebp

; get addr of cs_spin struct

	mov	ebp, esp
	mov	ebp, DWORD PTR [ebp + 8]

; tas on the cssp_bit

	mov	eax, 1
	xchg	al, BYTE PTR [ebp]

; quit if we got it

	cmp	eax, 0
	je	byebye

; spin spin spin

csspinit:
	xchg	al, BYTE PTR [ebp]
	cmp	eax, 0
	jne	csspinit

; increment collision counter

	inc	WORD PTR [ebp + 2]

; return

byebye:
	pop	ebp
	ret

_CS_getspin	ENDP

_TEXT	ENDS
END
