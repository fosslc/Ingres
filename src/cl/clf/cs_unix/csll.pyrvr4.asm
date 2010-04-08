##Copyright (c) 2004 Ingres Corporation
#if defined(pym_us5) || defined(mip_us5) || defined(rmx_us5) || \
    defined(ts2_us5) || defined(rux_us5)
 # The machine specific code for the Pyramid S series (pym_us5)
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
 #    Moved SGI to csll.sgi.asm
 #

#ifdef mip_us5
/* The MIPS ABI C compiler defines this, but not the assembler;
** it's needed for regdef.h .
*/
#define _MIPS_SIM  _MIPS_SIM_ABI32
#include <sys/regdef.h>
#else
/* he might have done his explicit include path for a reason... */
#include "/usr/include/sys/regdef.h"
#endif

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
#ifdef pym_us5
	.verstamp	9 0
	.abicalls
#endif
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
	lw	t6, Cs_srv_block+CS_INKERNEL
	# nop
	bne	t6, zero, Return_from_CS_swuser

	# put Cs_srv_block.cs_current (scb) address into register t7
	lw	t7, Cs_srv_block+CS_CURRENT	
	# nop

	# if no current session, don't save pc, sp, exsptr, or registers
	beq	t7, zero, Pick_new_thread	## branch to Pick_new_thread

 	# save exisiting exception stack, ex_sptr 
	lw	t6, ex_sptr
	# nop
	sw	t6, CS_EXSPTR(t7)		## ex_sptr

 	# save the pc, sp, registers: s0-s8, and floats: f20-f31
	sw	sp, CS_SP(t7) 			## sp
	sw	ra, CS_PC(t7) 			## pc
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
	# nop
 
Pick_new_thread:
        # No longer need to save s8, so can use it to protect gp
        .cpalias        $30     ## Asm saves gp in s8, restores after calls

	# use idle thread's exception stack while selecting new thread 
	lw	t6, Cs_idle_scb+CS_EXSPTR
	# nop
	sw	t6, ex_sptr

 	# and use idle thread's stack too, if possible
	lw	t6, Cs_idle_scb+CS_SP
	# nop
	move	sp, t6

 	# select new thread: scb = CS_xchng_thread(scb)
	move	a0, t7				## put scb into argument list
	# nop
	jal	CS_xchng_thread
	# nop
	move	t7, v0				## scb =  CS_xchng_thread()
	# nop

 	# restore new exception stack 
	lw	t6, CS_EXSPTR(t7)	
	# nop
	sw	t6, ex_sptr			## ex_sptr

 	# restore the pc, sp, registers: s0-s8, and floats: f20-f31
	lw	sp, CS_SP(t7) 			## sp
	lw	t9, CS_PC(t7) 			## pc
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
	# nop

Return_from_CS_swuser:
 	# return to new thread
	j	t9
	# nop
	.end 	Cs_swuser

#endif /* pym_us5 || mip_us5 || rmx_us5 ts2_us5 */
