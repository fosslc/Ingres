##Copyright (c) 2004 Ingres Corporation
#
# csll.ris.asm
#
# CS for IBM RS/6000
#
# History:
#       (long ago) (vijay)
#               Created.
#	10 Oct 92 (vijay)
#		Save condition code register. Else, on the new compiler, csphil
#		would hang if the source file is compiled with optimization on.
#	20-jun-93 (vijay)
#		Save floating point registers.
#	27-dec-95 (thoro01)
#		changed _CS_swuser to CS_swuser ...  compile errors.
#	
#ifdef ris_us5
# byte offsets into a CS_SCB structure

	.set CS__EX_SP,32
	.set CS__SP,40
	
# offsets into a CS_SYSTEM structure */

	.set CS__INKERNEL,204
	.set CS__CURRENT,184

# Parameters of the stack frame used by CS */

	.set CS_FRAME_SIZE,18*8+19*4+6*4
# 8*nfpr(18*8)+4*ngpr(=19*4) + linkarea(=6*4) + localstkarea(=0)+argarea(=0)
	.set CS_SAVEFPR, 18
	.set CS_SAVEGPR, 19

# if 0 
# /* This code was massaged into the assembler that follows */
# VOID
# CS_swuser(void)
# {
# 	extern CS_SYSTEM Cs_srv_block;
# 	extern CS_SCB Cs_idle_scb;
# 	extern CS_SCB *CS_xchng_thread();
# 	extern PTR ex_sptr;
# 	register CS_SCB *scb;
# 	register nat fakefp;
# 
# 	if( Cs_srv_block.cs_inkernel )
# 		return;
# 
# 	scb = Cs_srv_block.cs_current;
# 	if( scb != NULL )
# 	{
# 	    /* Save existing exception stack */
# 	    scb->cs_exsp = ex_sptr;
# 
# 	    /* Save the FP, LINK, and non-volatile regs for this thread */
# 	    saveregs( scb );
# 	}
# 
# 	/* use idle thread's exception stack while selecting */
# 	ex_sptr = Cs_idle_scb.cs_exsp;
# 
# 	/* run on idle thread's stack too, if possible */
# 	fakefp =  Cs_idle_scb.cs_registers[ CS_SP ];
# 
# 	scb = CS_xchng_thread( scb );
# 
# 	/*
# 	** causes FP and LINK and saved regs to be reloaded,
# 	** returning to another thread
# 	*/
# 	restore_regs( scb );
# }
# endif /* 0 */

	.align	1

CS_swuser:

# We save only nonvolatile registers on the stack.

	S_PROLOG(CS_swuser)

	mfspr	r0,lr			# get link register for return address
	SAVEFPR				# save floating point registers
	stm	13, -8*(CS_SAVEFPR)-4*(CS_SAVEGPR)(r1)
					# save integer registers
	st	r0,8(r1)		# save return addr from lr
	mfcr	r12			# save condition code reg
	st	r12,4(r1)		# 
	stu	r1, -CS_FRAME_SIZE(r1)	# push a frame and save back ptr

# Don't do anything else if in kernel mode

	LTOC(r4,Cs_srv_block,data)
	l	r3,CS__INKERNEL(r4)	# pickup cs_inkernel
	cmpi	0,r3,0			# test it
	bne	sw_ret			# if in kernel, just return

# Don't save if there is no current thread.

	l	r3,CS__CURRENT(r4)	# r3 = current scb, will be param later
	cmpi	0,r3,0			# skip save on first thread.
	beq	sw_no_save

# Save exception stack and frame pointer and the registers
	LTOC(r5,ex_sptr,data)		# loads address from TOC entry
	l	r6,0(r5)		# get current global exception stk ptr
	st	r6,CS__EX_SP(r3)	# save it in scb
	st	r1,CS__SP(r3)		# save fp: actually aren't using this
					# registers already stored in stk

sw_no_save:

# Use idle thread's exception stack and frame.

	LTOC(r5,ex_sptr,data)		# loads address from TOC entry
	LTOC(r6,Cs_idle_scb,data)	
	l	r7,CS__EX_SP(r6)	# get idle ex stack ptr 
	st	r7,0(r5)		# and set global ex stack to it...
	l	r1,CS__SP(r6)		# pick up idle thread FP.

# do we need idle thread's toc? or xchng thread's toc?
# xchng_thread returns the new scb in r3, given the current one in r3.
# Use new fp and exception stack off scb returned...

	bl	.CS_xchng_thread
	cror	0, 0, 0
	l	r1,CS__SP(r3)
	l	r0,CS__EX_SP(r3)
	LTOC(r5,ex_sptr,data)		# loads address from TOC entry
	st	r0,0(r5)		# set global ex stack

#/* Get back to caller's frame and return */
sw_ret:

	ai	r1,r1,CS_FRAME_SIZE	# pop frame
	l	r0,8(r1)		# read saved lr from stk
	mtlr	r0  			# restore return addr
	lm	r13,-8*(CS_SAVEFPR)-4*(CS_SAVEGPR)(r1)
	RESTOREFPR			# restore floating regs from stack
	l       r12,4(r1)               # 
	mtcr    r12                     # restore crs(only need cr2,cr3,cr4,cr5)
	S_EPILOG
	FCNDES(CS_swuser)

#/*** Test-and-set etc are in c for rios */
	.toc
	TOCE(Cs_srv_block,data)
	TOCE(ex_sptr,data)
	TOCE(Cs_idle_scb,data)
	TOCE(CS_xchng_thread,entry)
	TOCE(CS_eradicate,entry)
#endif
