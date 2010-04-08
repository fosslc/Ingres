/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: csll.ibm.asm
**
** Description:
**	This is the machine-specific CS code for S390 Linux platforms
**
**	Note: this file was created by Sam Somashekar, Thomas Lazicky,
**	and Michael Touloumtzis (i.e., by a committee ;).  The critical
**	CS_swuser was written by Tom (a roughed-in version that did
**	not actually work) and then cleaned up (by Mike) by stepping
**	through with the gdb debugger.  To do this, the "make gdb happy"
**	lines must be uncommented.  The resulting routine may not be
**	100% correct or efficient, but works.  Other routines, such as
**	CS_tas, were created by Sam from the assembler output of the
**	C compiler.  (If so, why do they need to be assembler at all?)
**	Such routines lack optimization; this could be improved by
**	compiling with a higher gcc -O level or by hand.  These routines
**	have not been examined in detail at the assembler level, but
**	seem to work okay.  Finally, CSget_stk_pages was coded from
**	scratch using the Intel equivalent code in csll.lnx.asm as
**	a model.  This routine is needed only when running a version of
**	Ingres compiled for OS threading with II_THREAD_TYPE=INTERNAL.
**
** History:
**      17-aug-2000 (somsa01)
**	    Created from AMDAHL version, with the help of lazto01.
**	05-oct-2000 (toumi01)
**	    Created CSget_stk_pages for ibm_lnx so that dbms can be
**	    built with OS threads and run with II_THREAD_TYPE=INTERNAL.
**	18-oct-2000 (toumi01)
**	    Clean up of CS_swuser; especially, avoid stack usage before
**	    INKERNEL test because it seems to be non-safe.
**	    Add note re module authorship and history.
**	28-oct-2000 (toumi01)
**	    In CSget_stk_pages allocate 16*ME_PAGESIZE for primary stack
**	    to avoid dmfrcp segv caused by yacc recursion stack overflow.
*/

/*
** offsets into structures and other program constants
** if there is a problem, check to see these are correct
*/
.text
	.align 4
$EXSP		= 32	/* CS_SCB.cs_exsp - CS_SCB */
$GPREGS		= 36	/* CS_SCB.cs_gpregptr - CS_SCB */
/* $FPREGS	= 40	 CS_SCB.cs_fpregs[0] - CS_SCB */
/* $CONDCODS	= 72	 CS_SCB.cs_condcods - CS_SCB */
$CS_CURRENT	= 184	/* Cs_srv_block.cs_current - Cs_srv_block */
$CS_INKERNEL	= 204	/* Cs_srv_block.cs_inkernel - Cs_srv_block */
$CS_swuser_len	= 104	/* amount of frame space used by function
			/* 0 - 63 = save gp regs here */
			/* 64 - 79 = save fp regs here ??? */
			/* 79 -    = allocated space for locals */

	.type    CS_swuser,@function
        .globl   CS_swuser
CS_swuser:
	BRAS    3,.swuser_entry
.swuser_entry:
	L	1,.a_Cs_srv_block-.swuser_entry(3)
	L	0,$CS_INKERNEL(1)
	LTR	0,0
	BNZR	14
	STM	7,15,28(15)
	BRAS	13,.LTN0_0

.LT0_0:
.a_Cs_srv_block:
        .long   Cs_srv_block
.a_ex_sptr:
        .long   ex_sptr
.a_Cs_idle_scb:
        .long   Cs_idle_scb
.a_CS_swuser_len:
        .long   $CS_swuser_len
.a_CS_xchng_thread:
        .long   CS_xchng_thread

.LTN0_0:
	LR	0,15
	AHI	15,-104
	ST	0,0(15)
	LR	11,15
	LR	9,7

	L	1,.a_Cs_srv_block-.LT0_0(13)

	L	2,$CS_CURRENT(1)
	LTR	2,2
	JZ	.SKIPSAVE

	STM	2,6,$GPREGS+(2*4)(2)
	L	1,0(15)
	MVC	$GPREGS+(7*4)((((15-7)+1)*4),2),(7*4)(1)

	L	1,.a_ex_sptr-.LT0_0(13)
	L	1,0(1)
	ST	1,$EXSP(2)

.SKIPSAVE:
	L	1,.a_Cs_idle_scb-.LT0_0(13)
	L	15,$GPREGS+(15*4)(1)

#	S	15,.a_CS_swuser_len

	L	0,$EXSP(1)
	L	1,.a_ex_sptr-.LT0_0(13)
	ST	0,0(1)

	ST	2,96(15)
	L	8,.a_CS_xchng_thread-.LT0_0(13)
#	LR	7,15		/* make gdb happy */
#	LR	11,15		/* make gdb happy */
	BASR	14,8
	LR	1,2
	ST	1,100(11)

	L	2,.a_ex_sptr-.LT0_0(13)
	L	0,$EXSP(1)
	ST	0,0(2)

	LM	2,15,$GPREGS+(2*4)(1)
	BR	14
.Lfe1:
        .size    CS_swuser,.Lfe1-CS_swuser


/*
**  Name:	CS_tas	-   Atomic test and set operation
**
**	granted = CS_test_set(&value)
**
** Functional description:
**	Do test and set operation on given address.  Needed for cross process
**	semaphores.
**
**	Return TRUE if semaphore is granted, FALSE otherwise.
**
** Inputs:
**	    4(ap)  - Address of value to test/set
** Outputs:
**	    4(ap)  - Value at address is set on return.
** Returns:
**	TRUE if value was not set previously.
**	FALSE if value was already set.
**
** Exceptions:
**	none.
**
**   Side Effects:
**	none
*/

.text
	.align 4
.globl CS_tas
	.type	 CS_tas,@function
CS_tas:
#	leaf function           0
#	automatics              16
#	outgoing args           0
#	need frame pointer      1
#	call alloca             0
#	has varargs             0
#	incoming args (stack)   0
#	function length         68
	STM	7,15,28(15)
	BRAS	13,.LTN1_0
.LT1_0:
.LC1:
	.long	atomic_compare_and_swap
.LTN1_0:
	LR	0,15
	AHI	15,-112
	ST	0,0(15)
	LR	11,15
	LR    9,7
	ST    2,96(11)
	SLR   1,1
	ST    1,100(11)
	LHI   1,1
	ST    1,104(11)
	L     2,100(11)
	L     3,104(11)
	L     4,96(11)
	L     8,.LC1-.LT1_0(13)
	BASR  14,8
	LR    1,2
	LTR   1,1
	JNE   .L18
	LHI   2,1
	J     .L17
	J     .L19
.L18:
	SLR   2,2
	J     .L17
.L19:
.L17:
	L	15,0(15)
	L	1,56(15)
	LM	7,15,28(15)
	BR	1
.Lfe2:
	.size	 CS_tas,.Lfe2-CS_tas
	.align 4
	.type	 atomic_compare_and_swap,@function
atomic_compare_and_swap:
#	leaf function           1
#	automatics              8
#	outgoing args           0
#	need frame pointer      1
#	call alloca             0
#	has varargs             0
#	incoming args (stack)   0
#	function length         40
	STM	7,15,28(15)
	LR	0,15
	AHI	15,-104
	ST	0,0(15)
	LR	11,15
	LR    1,7
	LR    5,2
#APP
	  cs   5,3,0(4)
  ipm  2
  srl  2,28
0:
#NO_APP
	LR    0,2
	ST    0,96(11)
	L     0,96(11)
	LR    2,0
	J     .L13
.L13:
	L	15,0(15)
	L	1,56(15)
	LM	7,15,28(15)
	BR	1
.Lfe3:
	.size	 atomic_compare_and_swap,.Lfe3-atomic_compare_and_swap


/*
** Name: CS_getspin()
**
** Description:
**	Get spin lock
**
** Inputs:
**	    CS_SPIN  - Address of spinlock counter
** Outputs:
**	    CS_SPIN  - Value at address is updated on return.
** Returns:
**	none
**
** Exceptions:
**	none.
**
**   Side Effects:
**	none
*/

.text
	.align 4
.globl CS_getspin
	.type	 CS_getspin,@function
CS_getspin:
#	leaf function           0
#	automatics              16
#	outgoing args           0
#	need frame pointer      1
#	call alloca             0
#	has varargs             0
#	incoming args (stack)   0
#	function length         40
	STM	7,15,28(15)
	BRAS	13,.LTN2_0
.LT2_0:
.LC2:
	.long	atomic_compare_and_swap_spin
.LTN2_0:
	LR	0,15
	AHI	15,-112
	ST	0,0(15)
	LR	11,15
	LR    9,7
	ST    2,96(11)
	SLR   1,1
	ST    1,100(11)
	LHI   1,1
	ST    1,104(11)
	L     2,100(11)
	L     3,104(11)
	L     4,96(11)
	L     8,.LC2-.LT2_0(13)
	BASR  14,8
.L20:
	L	15,0(15)
	L	1,56(15)
	LM	7,15,28(15)
	BR	1
.Lfe4:
	.size	 CS_getspin,.Lfe4-CS_getspin
	.align 4
	.type	 atomic_compare_and_swap_spin,@function
atomic_compare_and_swap_spin:
#	leaf function
#	has varargs             0
#	incoming args (stack)   0
#	function length         24
	STM	7,14,28(15)
	LR	11,15
	LR    5,7
#APP
	0: lr  1,2
   cs  1,3,0(4)
   jl  0b

#NO_APP
.L14:
	L	1,56(15)
	LM	7,14,28(15)
	BR	1
.Lfe5:
	.size	 atomic_compare_and_swap_spin,.Lfe5-atomic_compare_and_swap_spin


/*
** Name: CS_relspin()
**
** Description:
**	Release spin lock.
**
** Inputs:
**	    CS_SPIN  - Address of spinlock counter
** Outputs:
**	    CS_SPIN  - Value at address is updated on return.
** Returns:
**	none
**
** Exceptions:
**	none.
**
**   Side Effects:
**	none
*/

.text
	.align 4
.globl CS_relspin
	.type	 CS_relspin,@function
CS_relspin:
#	leaf function           0
#	automatics              8
#	outgoing args           0
#	need frame pointer      1
#	call alloca             0
#	has varargs             0
#	incoming args (stack)   0
#	function length         28
	STM	7,15,28(15)
	BRAS	13,.LTN3_0
.LT3_0:
.LC3:
	.long	atomic_set
.LTN3_0:
	LR	0,15
	AHI	15,-104
	ST	0,0(15)
	LR	11,15
	LR    9,7
	ST    2,96(11)
	SLR   1,1
	ST    1,100(11)
	L     2,96(11)
	L     3,100(11)
	L     8,.LC3-.LT3_0(13)
	BASR  14,8
.L21:
	L	15,0(15)
	L	1,56(15)
	LM	7,15,28(15)
	BR	1
.Lfe6:
	.size	 CS_relspin,.Lfe6-CS_relspin
	.align 4
	.type	 atomic_set,@function
atomic_set:
#	leaf function
#	has varargs             0
#	incoming args (stack)   0
#	function length         14
	STM	7,14,28(15)
	LR	11,15
	LR    1,7
#APP
	st  3,0(2)
	bcr 15,0
#NO_APP
.L3:
	L	1,56(15)
	LM	7,14,28(15)
	BR	1
.Lfe7:
	.size	 atomic_set,.Lfe7-atomic_set


/*
** CSget_stk_pages(npages)
**	This routine was created in imitation of the equivalent routine
**	for int_lnx.
** nat npages;
** 	Allocate contiguous stack memory from the process' own stack.
**	The size of one page is ME_MPAGESIZE = 8192 bytes.
*/

.text
	.align 4
.globl	CSget_stk_pages
	.type	 CSget_stk_pages,@function
CSget_stk_pages:
	lr	0,13			/* save base register		*/
	bras	13,.LTN4_0
.LT4_0:
.LC4:
CSstk_pg_base:
	.long	0x0
.LTN4_0:
	slr	1,1			/* zero out register to store	*/
/*
** First time through this routine?
*/
	l	5,CSstk_pg_base-.LT4_0(13)
	ltr	5,5
	bnz	initialized-.LT4_0(13)
/*
** Yes; initialize CSstk_pg_base signifying the top of allocated memory within
** within the stack.  Align this to the next unallocated ME_MPAGESIZE boundary.
** Find the base address that all thread stacks will built on top of:
**	(current stack pointer) - (16 * ME_MPAGESIZE) [i.e. 128k]
*/
	lr	3,15			/* %r3 = sp			*/
	lhi	4,8192
	sla	4,4			/* %r4 = 128k			*/
	sr	3,4			/* compute sp - 128k		*/
	lhi	4,-8192			/* load mask for alignment	*/
	nr	3,4			/* align stack address		*/
/*
** Calculate the number of words that need to be pushed onto the stack.
*/
	lr	4,15			/* %r4 = old sp			*/
	lr	5,3			/* %r5 = new sp			*/
	sr	4,3			/* %r4 = difference (bytes)	*/
	srl	4,2			/* %r4 = difference (words)	*/
	lr	3,15			/* scratch sp			*/
.initloop1:
	ahi	3,-4			/* decrement sp			*/
	st	1,0(0,3)		/* zero word in storage		*/
	bct	4,.initloop1-.LT4_0(13)	/* loop until we're done	*/
	st	5,CSstk_pg_base-.LT4_0(13)	/* store computed sp	*/
initialized:
/*
** Get the number of pages from the arg list in the stack and find out how
** many bytes this is.  This will be our displacement when we calculate the
** top of memory allocated.
*/
	lr	4,2			/* %r4 = npages argument	*/
	sla	4,13			/* %r4 = %r4 * 8192		*/
	srl	4,2			/* %r4 = difference (words)	*/
	lr	2,5			/* top of stack return argument	*/
.initloop2:
	ahi	5,-4			/* decrement sp			*/
	st	1,0(0,5)		/* zero word in storage		*/
	bct	4,.initloop2-.LT4_0(13)	/* loop until we're done	*/
	st	5,CSstk_pg_base-.LT4_0(13)	 /* store updated sp	*/
/*
** Restore and return.
*/
	lr	13,0			/* restore base register	*/
	br	14
