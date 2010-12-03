##Copyright (c) 2004 Ingres Corporation
#if defined(sgi_us5)
 # The machine specific code for the SGI MIPS processor (sgi_us5)
 # or for MIPS ABI (mip_us5)
 # 
 # 19-oct-92 (gmcquary)
 # initial creation.
 # 11-jan-93 (deastman)
 # Slight tweaking of the assembly code.
 # 10-feb-93 (mohana)
 # Added .cprestore pseudo-op to restore the gp register after a
 # call to the CS_xchng_thread C function.
 # 28-apr-93 (gmcquary)
 # Change to using cpalias for ABI-compliance in the same manner as the
 # compiler. Restores gp register properly.
 # 16-aug-94 (erickson)
 #    Adopts for mip_us5 (6.4/05 MIPS ABI).
 #    Note that Shelby started with the same code for 6.4/04(mip.us5) non-abi,
 #    which exists in csll.mip.asm (and which I just submitted into Piccolo).
 #    Since the reason for diffs between them aren't clear, I'm starting with
 #    this master source, with commented ifdef's as needed.
 #    No need to proliferate files, when they are almost identical!
 # 24-jul-95 (pursch)
 #    Fix a nasty bug: ".cpalias $30" was clobbering s8 before it was saved!
 #    This damaged a saved register at random points in execution.
 # 15-dec-95 (morayf)
 #    Add SNI RMx00 (rmx_us5) Pyramid clone to platforms supported.
 # 11-Oct-1996 (walro03)
 #    Updated for Tandem Nonstop (ts2_us5)
 # 11-Oct-1997 (allmi01)
 #    Updated for Silicon Graphics (sgi_us5)
 # 03-jul-99 (podni01)
 #    Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
 # 26-Nov-2002 (bonro01)
 #    Copied from csll.pyrmr4.asm
 #    Added 64-bit support for sgi_us5
 # 22-Jun-2009 (kschendel) SIR 122138
 #    Probably pointless, but update hybrid symbols.
 #


#if defined(BUILD_ARCH64)

#define _ABIO32 1
#define _ABIN32 2
#define _ABI64  3
#define _MIPS_SIM _ABI64
#include <sys/regdef.h>

#define CS_MACH_DEP	56
#define CS_SP		CS_MACH_DEP 
#define CS_PC		CS_MACH_DEP + 8
#define CS_EXSPTR  	CS_MACH_DEP + 16
#define CS_REGISTERS	CS_MACH_DEP + 24
#define CS_FLOATS	CS_MACH_DEP + 96
#define CS_CURRENT	20
#define CS_INKERNEL	16

#define CS_COLLIDE	2


 #
 # CS_swuser()
 #
	.text
	.align	2
	.globl	CS_swuser
	.ent 	CS_swuser 2
CS_swuser:
	.set noreorder
	.cpload	$25
	.set reorder
	.frame 	sp, 0, $31

	# check if server is in kernel state, no task switching, just return
	lw	t2, Cs_srv_block+CS_INKERNEL
	# nop
	bne	t2, zero, Return_from_CS_swuser

	# put Cs_srv_block.cs_current (scb) address into register t3
	ld	t3, Cs_srv_block+CS_CURRENT	
	# nop

	# if no current session, don't save pc, sp, exsptr, or registers
	beq	t3, zero, Pick_new_thread	## branch to Pick_new_thread

 	# save exisiting exception stack, ex_sptr 
	ld	t2, ex_sptr
	# nop
	sd	t2, CS_EXSPTR(t3)		## ex_sptr

 	# save the pc, sp, registers: s0-s8, and floats: f24-f31
	sd	sp, CS_SP(t3) 			## sp
	sd	ra, CS_PC(t3) 			## pc
	sd	s0, CS_REGISTERS+0(t3) 		## s0
	sd	s1, CS_REGISTERS+8(t3) 		## s1
	sd	s2, CS_REGISTERS+16(t3) 	## s2
	sd	s3, CS_REGISTERS+24(t3) 	## s3
	sd	s4, CS_REGISTERS+32(t3) 	## s4
	sd	s5, CS_REGISTERS+40(t3) 	## s5
	sd	s6, CS_REGISTERS+48(t3) 	## s6
	sd	s7, CS_REGISTERS+56(t3) 	## s7
	sd	s8, CS_REGISTERS+64(t3) 	## s8
	sdc1	$f24, CS_FLOATS+0(t3) 		## f24
	sdc1	$f25, CS_FLOATS+8(t3) 		## f25
	sdc1	$f26, CS_FLOATS+16(t3) 		## f26
	sdc1	$f27, CS_FLOATS+24(t3) 		## f27
	sdc1	$f28, CS_FLOATS+32(t3) 		## f28
	sdc1	$f29, CS_FLOATS+40(t3) 		## f29
	sdc1	$f30, CS_FLOATS+48(t3) 		## f30
	sdc1	$f31, CS_FLOATS+56(t3) 		## f31
	# nop
 
Pick_new_thread:
        # No longer need to save s8, so can use it to protect gp
        .cpalias        $30     ## Asm saves gp in s8, restores after calls

	# use idle thread's exception stack while selecting new thread 
	ld	t2, Cs_idle_scb+CS_EXSPTR
	# nop
	sd	t2, ex_sptr

 	# and use idle thread's stack too, if possible
	ld	t2, Cs_idle_scb+CS_SP
	# nop
	move	sp, t2

 	# select new thread: scb = CS_xchng_thread(scb)
	move	a0, t3				## put scb into argument list
	# nop
	jal	CS_xchng_thread
	# nop
	move	t3, v0				## scb =  CS_xchng_thread()
	# nop

 	# restore new exception stack 
	ld	t2, CS_EXSPTR(t3)	
	# nop
	sd	t2, ex_sptr			## ex_sptr

 	# restore the pc, sp, registers: s0-s8, and floats: f24-f31
	ld	sp, CS_SP(t3) 			## sp
	ld	t9, CS_PC(t3) 			## pc
	ld	s0, CS_REGISTERS+0(t3) 		## s0
	ld	s1, CS_REGISTERS+8(t3) 		## s1
	ld	s2, CS_REGISTERS+16(t3)		## s2
	ld	s3, CS_REGISTERS+24(t3) 	## s3
	ld	s4, CS_REGISTERS+32(t3) 	## s4
	ld	s5, CS_REGISTERS+40(t3) 	## s5
	ld	s6, CS_REGISTERS+48(t3) 	## s6
	ld	s7, CS_REGISTERS+56(t3) 	## s7
	ld	s8, CS_REGISTERS+64(t3) 	## s8
	ldc1	$f24, CS_FLOATS+0(t3) 		## f24
	ldc1	$f25, CS_FLOATS+8(t3) 		## f25
	ldc1	$f26, CS_FLOATS+16(t3) 		## f26
	ldc1	$f27, CS_FLOATS+24(t3) 		## f27
	ldc1	$f28, CS_FLOATS+32(t3) 		## f28
	ldc1	$f29, CS_FLOATS+40(t3) 		## f29
	ldc1	$f30, CS_FLOATS+48(t3) 		## f30
	ldc1	$f31, CS_FLOATS+56(t3) 		## f31
	# nop

Return_from_CS_swuser:
 	# return to new thread
	j	t9
	# nop
	.end 	Cs_swuser


#else

#define _ABIO32 1
#define _ABIN32 2
#define _ABI64  3
#define _MIPS_SIM _ABIN32
#include <sys/regdef.h>

#define CS_MACH_DEP	32
#define CS_SP		CS_MACH_DEP 
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
	.ent 	CS_swuser 2
CS_swuser:
	.set noreorder
	.cpload	$25
	.set reorder
	.frame 	sp, 0, $31

	# check if server is in kernel state, no task switching, just return
	lw	t2, Cs_srv_block+CS_INKERNEL
	# nop
	bne	t2, zero, Return_from_CS_swuser

	# put Cs_srv_block.cs_current (scb) address into register t3
	lw	t3, Cs_srv_block+CS_CURRENT	
	# nop

	# if no current session, don't save pc, sp, exsptr, or registers
	beq	t3, zero, Pick_new_thread	## branch to Pick_new_thread

 	# save exisiting exception stack, ex_sptr 
	lw	t2, ex_sptr
	# nop
	sw	t2, CS_EXSPTR(t3)		## ex_sptr

 	# save the pc, sp, registers: s0-s8, and floats: f20-f31
	sw	sp, CS_SP(t3) 			## sp
	sw	ra, CS_PC(t3) 			## pc
	sw	s0, CS_REGISTERS+0(t3) 		## s0
	sw	s1, CS_REGISTERS+4(t3) 		## s1
	sw	s2, CS_REGISTERS+8(t3) 		## s2
	sw	s3, CS_REGISTERS+12(t3) 	## s3
	sw	s4, CS_REGISTERS+16(t3) 	## s4
	sw	s5, CS_REGISTERS+20(t3) 	## s5
	sw	s6, CS_REGISTERS+24(t3) 	## s6
	sw	s7, CS_REGISTERS+28(t3) 	## s7
	sw	s8, CS_REGISTERS+32(t3) 	## s8
	swc1	$f20, CS_FLOATS+0(t3) 		## f20
	swc1	$f21, CS_FLOATS+4(t3) 		## f21
	swc1	$f22, CS_FLOATS+8(t3) 		## f22
	swc1	$f23, CS_FLOATS+12(t3) 		## f23
	swc1	$f24, CS_FLOATS+16(t3) 		## f24
	swc1	$f25, CS_FLOATS+20(t3) 		## f25
	swc1	$f26, CS_FLOATS+24(t3) 		## f26
	swc1	$f27, CS_FLOATS+28(t3) 		## f27
	swc1	$f28, CS_FLOATS+32(t3) 		## f28
	swc1	$f29, CS_FLOATS+36(t3) 		## f29
	swc1	$f30, CS_FLOATS+40(t3) 		## f30
	swc1	$f31, CS_FLOATS+44(t3) 		## f31
	# nop
 
Pick_new_thread:
        # No longer need to save s8, so can use it to protect gp
        .cpalias        $30     ## Asm saves gp in s8, restores after calls

	# use idle thread's exception stack while selecting new thread 
	lw	t2, Cs_idle_scb+CS_EXSPTR
	# nop
	sw	t2, ex_sptr

 	# and use idle thread's stack too, if possible
	lw	t2, Cs_idle_scb+CS_SP
	# nop
	move	sp, t2

 	# select new thread: scb = CS_xchng_thread(scb)
	move	a0, t3				## put scb into argument list
	# nop
	jal	CS_xchng_thread
	# nop
	move	t3, v0				## scb =  CS_xchng_thread()
	# nop

 	# restore new exception stack 
	lw	t2, CS_EXSPTR(t3)	
	# nop
	sw	t2, ex_sptr			## ex_sptr

 	# restore the pc, sp, registers: s0-s8, and floats: f20-f31
	lw	sp, CS_SP(t3) 			## sp
	lw	t9, CS_PC(t3) 			## pc
	lw	s0, CS_REGISTERS+0(t3) 		## s0
	lw	s1, CS_REGISTERS+4(t3) 		## s1
	lw	s2, CS_REGISTERS+8(t3) 		## s2
	lw	s3, CS_REGISTERS+12(t3) 	## s3
	lw	s4, CS_REGISTERS+16(t3) 	## s4
	lw	s5, CS_REGISTERS+20(t3) 	## s5
	lw	s6, CS_REGISTERS+24(t3) 	## s6
	lw	s7, CS_REGISTERS+28(t3) 	## s7
	lw	s8, CS_REGISTERS+32(t3) 	## s8
	lwc1	$f20, CS_FLOATS+0(t3) 		## f20
	lwc1	$f21, CS_FLOATS+4(t3) 		## f21
	lwc1	$f22, CS_FLOATS+8(t3) 		## f22
	lwc1	$f23, CS_FLOATS+12(t3) 		## f23
	lwc1	$f24, CS_FLOATS+16(t3) 		## f24
	lwc1	$f25, CS_FLOATS+20(t3) 		## f25
	lwc1	$f26, CS_FLOATS+24(t3) 		## f26
	lwc1	$f27, CS_FLOATS+28(t3) 		## f27
	lwc1	$f28, CS_FLOATS+32(t3) 		## f28
	lwc1	$f29, CS_FLOATS+36(t3) 		## f29
	lwc1	$f30, CS_FLOATS+40(t3) 		## f30
	lwc1	$f31, CS_FLOATS+44(t3) 		## f31
	# nop

Return_from_CS_swuser:
 	# return to new thread
	j	t9
	# nop
	.end 	Cs_swuser

#endif /* arch */

#endif
