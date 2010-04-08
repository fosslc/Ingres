##Copyright (c) 2004 Ingres Corporation
#ifdef ds3_ulx
 # The following is the machine specific code
 # for the DECstation 3100 (ds3_ulx):
 #
 #      change history:
 #           2-jul-91 (dchan)
 #               In the process of selecting the idle thread's stack,
 #               2 parameters were reversed in a move operation.
 #
 #	CS_swuser()

#include <regdef.h>

#define CS_MACH_DEP	32
#define CS_SP		CS_MACH_DEP + 0
#define CS_PC		CS_MACH_DEP + 4
#define CS_EXSPTR  	CS_MACH_DEP + 8
#define CS_REGISTERS	CS_MACH_DEP + 12
#define CS_FLOATS	CS_MACH_DEP + 48

#define CS_CURRENT	184
#define CS_INKERNEL	204

#define CS_COLLIDE	2


 #
 # CS_swuser()
 #

	.text
	.align	2

	.globl	CS_swuser
	.ent 	CS_swuser

CS_swuser:
	.frame 	sp, 0, ra

	# check if server is in kernel state, no task switching, just return
	lw	t6, Cs_srv_block+CS_INKERNEL
	nop
	bne	t6, zero, Return_from_CS_swuser

	# put Cs_srv_block.cs_current (scb) address into register t7
	lw	t7, Cs_srv_block+CS_CURRENT	
	nop

	# if no current session, don't save pc, sp, exsptr, or registers
	beq	t7, zero, Pick_new_thread	## branch to Pick_new_thread

 	# save exisiting exception stack, ex_sptr 
	lw	t6, ex_sptr
	nop
	sw	t6, CS_EXSPTR(t7)		## ex_sptr

 	# save the pc, sp, registers: s0-s8, and floats: f20-f31
	sw	ra, CS_PC(t7) 			## pc
	sw	sp, CS_SP(t7) 			## sp
	sw	s0, CS_REGISTERS+0(t7) 		## s0
	sw	s1, CS_REGISTERS+4(t7) 		## s1
	sw	s2, CS_REGISTERS+8(t7) 		## s2
	sw	s3, CS_REGISTERS+12(t7) 	## s3
	sw	s4, CS_REGISTERS+16(t7) 	## s4
	sw	s5, CS_REGISTERS+20(t7) 	## s5
	sw	s6, CS_REGISTERS+24(t7) 	## s6
	sw	s7, CS_REGISTERS+28(t7) 	## s7
	sw	s8, CS_REGISTERS+32(t7) 	## s8
	swc1	$f20, CS_FLOATS+0(t7) 		## f20
	swc1	$f21, CS_FLOATS+4(t7) 		## f21
	swc1	$f22, CS_FLOATS+8(t7) 		## f22
	swc1	$f23, CS_FLOATS+12(t7) 		## f23
	swc1	$f24, CS_FLOATS+16(t7) 		## f24
	swc1	$f25, CS_FLOATS+20(t7) 		## f25
	swc1	$f26, CS_FLOATS+24(t7) 		## f26
	swc1	$f27, CS_FLOATS+28(t7) 		## f27
	swc1	$f28, CS_FLOATS+32(t7) 		## f28
	swc1	$f29, CS_FLOATS+36(t7) 		## f29
	swc1	$f30, CS_FLOATS+40(t7) 		## f30
	swc1	$f31, CS_FLOATS+44(t7) 		## f31
	nop
 
Pick_new_thread:
	# use idle thread's exception stack while selecting new thread 
	lw	t6, Cs_idle_scb+CS_EXSPTR
	nop
	sw	t6, ex_sptr

 	# and use idle thread's stack too, if possible
	lw	t6, Cs_idle_scb+CS_SP
	nop
	move	sp, t6

 	# select new thread: scb = CS_xchng_thread(scb)
	move	a0, t7				## put scb into argument list
	nop
	jal	CS_xchng_thread
	nop
	move	t7, v0				## scb =  CS_xchng_thread()
	nop

 	# restore new exception stack 
	lw	t6, CS_EXSPTR(t7)	
	nop
	sw	t6, ex_sptr			## ex_sptr

 	# restore the pc, sp, registers: s0-s8, and floats: f20-f31
	lw	ra, CS_PC(t7) 			## pc
	lw	sp, CS_SP(t7) 			## sp
	lw	s0, CS_REGISTERS+0(t7) 		## s0
	lw	s1, CS_REGISTERS+4(t7) 		## s1
	lw	s2, CS_REGISTERS+8(t7) 		## s2
	lw	s3, CS_REGISTERS+12(t7) 	## s3
	lw	s4, CS_REGISTERS+16(t7) 	## s4
	lw	s5, CS_REGISTERS+20(t7) 	## s5
	lw	s6, CS_REGISTERS+24(t7) 	## s6
	lw	s7, CS_REGISTERS+28(t7) 	## s7
	lw	s8, CS_REGISTERS+32(t7) 	## s8
	lwc1	$f20, CS_FLOATS+0(t7) 		## f20
	lwc1	$f21, CS_FLOATS+4(t7) 		## f21
	lwc1	$f22, CS_FLOATS+8(t7) 		## f22
	lwc1	$f23, CS_FLOATS+12(t7) 		## f23
	lwc1	$f24, CS_FLOATS+16(t7) 		## f24
	lwc1	$f25, CS_FLOATS+20(t7) 		## f25
	lwc1	$f26, CS_FLOATS+24(t7) 		## f26
	lwc1	$f27, CS_FLOATS+28(t7) 		## f27
	lwc1	$f28, CS_FLOATS+32(t7) 		## f28
	lwc1	$f29, CS_FLOATS+36(t7) 		## f29
	lwc1	$f30, CS_FLOATS+40(t7) 		## f30
	lwc1	$f31, CS_FLOATS+44(t7) 		## f31
	nop

Return_from_CS_swuser:
 	# return to new thread
	j	ra
	nop
	.end 	Cs_swuser

#endif /* ds3_ulx */
