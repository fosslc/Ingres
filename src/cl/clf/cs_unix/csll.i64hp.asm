	.type	CS_swap_thread,@function

	.radix	C

#if defined(BUILD_ARCH32)
	.psr	abi32
#else
	.psr	abi64
#endif
	.psr	msb

	.section .text = "ax", "progbits"
	.proc	CS_swap_thread
..L0:
//	***StartOp***	CMid904 = 		;; // A

..L2:
CS_swap_thread::
	br.ret.sptk.clr	b0

..L1:
//	***EndOp***				;; // A

	.endp	CS_swap_thread

	.type	CS_tas,@function
	.proc	CS_tas

CS_tas::
	mov		ar.ccv = r0
	mov		r29 = 1;;
	ld8		r3 = [r32]
	cmpxchg8.acq	r8 = [r32], r29, ar.ccv;;
	br.ret.sptk.clr	b0

	.endp	CS_tas
