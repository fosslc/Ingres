/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
**
** CS code for Power6 processor.
**
** History:
**	16-Nov-89 (pholman)
**		First written.
**	29-Nov-89 (apegg)
**		Got working.
*/

/*
** Definitions of SCB.
*/
#define CS__MACHINE_SPECIFIC	0x20 	/* Offset to mach-spec area in SCB   */
#define CS__CURRENT		0xb8 	/* Holds the current SCB             */
#define CS__INKERNEL		0xcc	/* Set if running in kernal	     */
#define CS__SP CS__MACHINE_SPECIFIC + 0xe * 4 	/* SP is reg 14 (0xe) in SCB */

/*
** Various definitions.
** Note that register 0 (r0) is effectively scratch and R13 is the frame
** pointer.
*/
#define R1_to_R13	0x3ffe 		/* Mask for registers 1 to 13     */
#define ONE_ARGUMENT	8 		/* 8 = 4 * Numargs + 4		  */
#define SP_OFFSET	8		/* Offset of SP from FP		  */

	.file "csll.pow6.asm"
	.align 2
/*
** _CS_SWUSER
**
** Handles the context switching from one thread to another. The theory of
** operation is as follows.
**
** Checks for running in kernal and for a current thread. If the inkernal flag
** is set then immeadiatly returns. If there is no current thead then the
** current values of the registers are not saved.
**
** The contents of registers 1 to 14 are then saved. R13 is the frame pointer
** and R14 is the stack pointer. The value of the stack pointer cannot be read
** directly but is calculated by adding 12 (decimal) to the frame pointer.
** This equation works as long as no registers are saved on the entry into the
** procedure.
**
** The stack pointer is then set up for the idle thread and _CS_xchng_thread is
** called to determine the new thread to run. All the registers for this thread
** are then restored and the procedure returns. As R13, the frame pointer, has
** now been altered it will return to the new thread.
*/
	.globl _CS_swuser
_CS_swuser:
	.word   0			/* Don't want to save any registers */

	/*
	** Set up r0 to point to the start of the server block.
	*/
	moval    _Cs_srv_block, r0

	/*
	** if (!kernal)
	**	return;
	*/
	tstl    CS__INKERNEL(r0)
	bneq	tas_ret

	/*
	** if ((r0 = current) == NULL)
	**	goto CS_pick_new;
	*/
	movl    CS__CURRENT(r0), r0
	tstl	r0
	beql	_CS__pick_new

	/*
	**	Store all registers.
	*/
	storer	$R1_to_R13, CS__MACHINE_SPECIFIC + 4(r0)
	subl3	$SP_OFFSET, fp, CS__SP(r0)	/* Save old stack pointer */

_CS__pick_new:
	/*
	** Use the stack of the idle thread.
	*/
	moval    _Cs_idle_scb, r0
	movl    CS__SP(r0), sp		/* Set up new stack pointer */
	loadr	$R1_to_R13, CS__MACHINE_SPECIFIC + 4(r0)/* and frame */

	/*
	** And pass old SCB to CS_xchng_thread routine..
	*/
	moval    _Cs_srv_block, r0
	pushl    CS__CURRENT(r0)
	callf	$ONE_ARGUMENT,_CS_xchng_thread

	/*
	** Restore registers for the new thread. The new SCB is returned in
	** r0. There is no need to restore the stack pointer as it is reset
	** by the return.
	*/
	loadr	$R1_to_R13, CS__MACHINE_SPECIFIC + 4(r0)
tas_ret:	
	ret#2

/*
** CS_TAS
**
**	Essentially performs the following - although *ptr is
**	locked, and the test and assignment form a single
**	atomic action.
**
** CS_tas(ptr)
** PTR *ptr;
** {
**	if(*ptr)
**	   return(0);
**	else
**	{
**	   *ptr = 1;
**	   return(1);
**	}
** }
*/
	.align 2
	.globl _CS_tas
	.text
_CS_tas:
	.word   0
	bbssi 	$0,*4(fp),setalready  	/* Check and set */
	movl 	$1, r0			/* If not set originally, return 1 */
	ret#2

setalready:
	movl 	$0, r0			/* Return 0 */
	ret#2

/*
** CS_GETSPIN
**
**	Loop 'round 'till we get an exclusive lock on the byte
**	in question.
**	Something like..
** CS_getspin(ptr)
** CS_SPIN *ptr;
** {
**	if (ptr->cssp_bit == 0) {
**		ptr->cssp_bit = 1;
**		return;
**	} else {
**		while (ptr->cssp_bit != 0)
**			;
**		++ ptr->cssp_collide;
**	}
** }
*/
	.align 2 
	.globl _CS_getspin
	.text
_CS_getspin:
	.word   0
	movl 	4(fp), r0		/* Get struct pointer into r0 */
	bbssi 	$0,(r0), spin		/* Check and set */
	ret#2
spin:
	bbssi 	$0,(r0), spin		/* Loop till lock is free */
	addw2 	$1, 4(r0)		/* Increment ptr->cssp_collide */
	ret#2
