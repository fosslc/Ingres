/*
**Copyright (c) 2004 Ingres Corporation
*/
# if defined(sqs_u42) || defined(sqs_us5) || defined(odt_us5) || \
     defined(ps2_us5) || defined(dr3_us5) || defined(sco_us5) || \
     defined(sur_u42) || defined(ix3_us5) || defined(sqs_ptx) || \
     defined(dra_us5) || defined(usl_us5) || defined(sui_us5) || \
     defined(nc4_us5) || defined(sos_us5)
/*
** Sequent Symmetry
** History:
**	5-june-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Add ps2_us5, sco_us5, dr3_us5, sur_u42, ix3_us5;
**		Add code to allocate thread stack memory on server stack.
**		Needed only on SysV machines which check the stack pointer
**		after an exception.
**	4-sep-90 (jkb)
**		Add sqs_ptx to list of if defines
**	02-jul-92 (swm)
**	    Added dra_us5 to list of if defines.
**	    Increase dra_us5 free space to allow server stack growth to
**	    32k (from 8k) to prevent server stack area growing into thread
**	    stack area.
**	08-jan-93 (sweeney)
**	    Yet another Intel x86 port.
**	15-dec-93 (nrb)
**	    Bug #58890
**	    Increase space for usl_us5 stack growth to more than 8k, as per
**	    dra_us5, and increase growth space to 64k as 32k was inadequate.
**	15-jun-95 (popri01)
**	    Yet another Intel x86 port: nc4_us5
**	14-jul-95 (morayf)
**		Add sos_us5 to list of if defines.
**	14-nov-1995 (murf)
**		Added sui_us5 support. Note that this platform required the
**		stack size to be altered.
**	07-may-1997 (popri01)
**		For OS_THREADS for Unixware (usl_us5), add CS_aclr.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	12-Nov-2010 (kschendel) SIR 124685
**	    Moved inkernel and current to front of CS_SYSTEM, fix here.
*/

/* Values for locking */
# define L__UNLOCKED	0
# define L__LOCKED	1

/* offsets into a CS_SPIN structure */
# define CS__COLLIDE	1

# define CS__MACHINE_SPECIFIC	0x20		
# define CS__SP            CS__MACHINE_SPECIFIC    /* 0 Stack pointer */
# define CS__FP    	   0x24 	           /* +4 frame pointer */
# define CS__EBX	   0x28 		   /* +8 non-scratch reg */
# define CS__ESI	   0x2c 		   /* +0xc source index */
# define CS__EDI	   0x30 	    	   /* +0x10 destination index */
# define CS__EXSP	   0x34 		   /* + 0x14 Exception stk ptr*/
# define CS__CURRENT	   0x14 		   /* Current thread SCB */
# define CS__INKERNEL	   0x10 		   /* In kernel lock */

.text
.globl	 _CS_tas
.globl	 _CS_getspin
.globl	 _CS_relspin
.globl	 CS_aclr
.globl	_CS_swuser
.globl   CS_get_sp

_CS_swuser:
/* If server in kernel state, no task switching */
	cmpl	$L__UNLOCKED, _Cs_srv_block + CS__INKERNEL
	jne	sw_ret
/* if there is no current session, don't save registers */
	movl	_Cs_srv_block + CS__CURRENT, %eax
	cmpl	$0x0,%eax
	je	CS__pick_new
/* At this point %eax points to the current session control block.  First save  
 * the state.  Note that there are no memory to memory moves, and this
 * restriction adds extra instructions.  We don't worry about PC as that
 * is saved on the stack  */
	movl	_ex_sptr,%ecx
	movl	%ecx, [CS__EXSP](%eax)  	/* exception stack ptr*/
	movl	%ebx, [CS__EBX](%eax)		/* non-scrach reg */
	movl	%esp, [CS__SP](%eax)		/* stack pointer */
	movl	%ebp, [CS__FP](%eax)		/* frame pointer */
	movl	%esi, [CS__ESI](%eax)		/* source index */
	movl	%edi, [CS__EDI](%eax)		/* destination index */
CS__pick_new:
/* Use the idle thread's stack */
	movl	_Cs_idle_scb + CS__SP, %esp
/* and use idle thread's exception stack as well */
	movl	_Cs_idle_scb + CS__EXSP, %ecx
	movl	%ecx, _ex_sptr

/*  so call CS_xchng_thread(old_scb) */
	pushl	%eax		/* %eax still holds the add. of the old scb */ 
	call	_CS_xchng_thread	/*  _CS_xchng_tread(old_scb) */
/* don't bother to pop arguement off stack since we change the sp anyway */
/* at return time, %eax has new current SCB, move it and go */
	movl	[CS__EXSP](%eax),%ecx
	movl	%ecx,_ex_sptr  			/* exception stack ptr*/
	movl	[CS__EBX](%eax), %ebx		/* non-scrach reg */
	movl	[CS__SP](%eax),%esp		/* stack pointer */
	movl	[CS__FP](%eax),%ebp		/* frame pointer */
	movl	[CS__ESI](%eax),%esi		/* source index */
	movl	[CS__EDI](%eax),%edi		/* destination index */
sw_ret:
	ret

/*  Test and Set  We expect to find memory address on top of stack  
 *  This routine assumes memory set to 0x1 when locked, 0x0 when
 *  unlocked, and returns 0x1 when lock was sucessful, 0x0 when
 * unsucessful
 */ 

_CS_tas:
/*  Set lock set code */ 
	movl	$0,%eax		/* Clear return value */
	movb 	$L__LOCKED, %al
/* load the address of lock into register */
	movl	4(%esp), %ecx  
/* Atomically swap current value in lock with our set 
 * If we get a 0 back, we set the lock.   If we get a 1 back,
 * lock was already set.
 */
	xchgb   %al, (%ecx)	
/* flip result bit to get the correct return code, and return in %eax  */
	xorb	$L__LOCKED,%al
endtas:
	ret

/* CS_getspin(memloc)
 * CS_SPIN *memloc
 *  Get spin lock
 *  Note:  We can spin all we want on Symmetry without flooding the
 *  bus on Model B Copyback processors.  Only cost is that of machine  
 *  cycles.
 */
_CS_getspin:
	movl	4(%esp), %ecx  		/* get pointer to CS_SPIN from stack */
	movl	%ecx, %edx  		/* get pointer to CS__COLLIDE */
	addl	$CS__COLLIDE, %edx	/* equals lock location plus offset */
csspinit:
	movb 	$L__LOCKED, %al 	/*  Set lock code. */ 
	xchgb   %al, (%ecx)		/* Atomically swap lower byte of %eax */
				        /* and lock */
/* If we swapped UNLOCKED we got the lock.  If we have LOCKED, lock was 
   already set so try again */
	cmpb	$L__UNLOCKED, %al
	je	endspin 
	incw	(%edx) 			/* Increment spinlock counter */
spin:					/* Spin in cache until unlocked */
	cmpb	$L__UNLOCKED, (%ecx)	/* Reads don't flood the bus */
	je	csspinit 		/*  Then try and get lock again */
	jmp	spin
endspin:
	ret

/* CS_relspin(memloc)
 * CS_SPIN *memloc
 *  Get spin lock
 *  Note:  on Model A and Model D processors we must use atomic clear 
 *  to ensure cache coherrency  */
CS_aclr:
_CS_relspin:
/*  Set lock set code */ 
	movb 	$L__UNLOCKED, %al
/* load the address of lock into register */
	movl	4(%esp), %ecx  
/* Atomically swap current value in lock with our set 
 * lock was already set.  */
	xchgb   %al, (%ecx)	
endrelspin:
	ret

CS_get_sp:
	movl %esp, %eax
	ret

# if defined(ps2_us5) || defined(dr3_us5) || defined(sco_us5) || \
     defined(ix3_us5) || defined(dra_us5) || defined(usl_us5) || \
     defined(sui_us5)

/* 
** Start of CS server stack allocation routines.
*/

.globl    CSget_stk_pages
.globl    CSinit_pg_base

/*
** This may be better allocated in cshl.c to hide assembler directives
** on variable declarations.
*/
.data     
.align 4  

CSstk_pg_base:
        .byte     0x0
        .byte     0x0
        .byte     0x0
        .byte     0x0

.text     
.align 4  

/*
** CSget_stk_pages(npages)
** i4  npages;
** 	Allocate contiguous stack memory from the process' own stack.
**	The size of one page is ME_MPAGESIZE = 8192 bytes.
*/
CSget_stk_pages:
    pushl    %ebp
    movl     %esp, %ebp
    pushl    %ebx
    pushl    %ecx
    pushl    %edx
/*
** If this is the first time through this routine,
** initialize the stack page base.
*/
    cmpl     $0, CSstk_pg_base
    jnz      initialized
    call     CSinit_pg_base
initialized:
/*
** Get the number of pages from the arg list in
** the stack and find out how many bytes this is.
** This will be our displacement when we calculate
** the top of memory allocated.
*/
    movl     0x8(%ebp), %eax
    sall     $13, %eax
/*
**  %ecx will hold the number of long words the
**  above represents.  This is our loop counter.
*/
    movl     %eax, %ecx
    shrl     $2, %ecx
/*
** %edx will hold the address of the top of the
** stack.  This is returned to the calling routine
** by reference.
*/
    movl     CSstk_pg_base, %edx
    subl     %eax, %edx
/*
** Save the current stack pointer and push 0's
** onto the stack.  This action allows the 
** operating system fault to in stack pages
** and initialize the region to zeroes.
*/
    movl     %esp, %eax
    movl     CSstk_pg_base, %esp
loop2:
    pushl    $0
    loop     loop2
    movl     %esp, CSstk_pg_base
    movl     %eax, %esp
/*
** The top address of the allocated region is in %edx.
** Move this value to %eax as a return value.
*/
    movl     %edx, %eax
    popl     %edx
    popl     %ecx
    popl     %ebx
    leave    
    ret      
    nop      

/* 
** CSinit_pg_base()
**	Initialize CSstk_pg_base signifying the top of allocated memory
**	within the stack.  Align this to the next unallocated ME_MPAGESIZE 
**	boundary.
*/
CSinit_pg_base:
    pushl    %ebp
    movl     %esp, %ebp
    pushl    %ebx
    pushl    %ecx
/*
** save the current stack pointer in %ebx.
*/
    movl     %esp, %ebx
/*
** Find the base address that all thread stacks will built on top of.
** Subtract the size of a page (ME_MPAGESIZE), or a multiple of it for
** some platforms.
*/
# if defined(dra_us5) || defined(usl_us5) || defined(sui_us5)
/*
** For dra_us5 and usl_us5, sui_us5 increase this limit from 8k to 64k
*/
    leal     -0x10000(%ebx), %eax
# else
    leal     -0x2000(%ebx), %eax
# endif /* dra_us5 || usl_us5 || sui_us5 */
/*
** Mask the appropriate bits to get align address on ME_PAGESIZE (8192)
** boundary.
*/
    andl     $-8192, %eax
/*
** Calculate the number of long words that need to be pushed onto the stack
** and store this in the %ecx register.
*/
    movl     %esp, %ecx
    subl     %eax, %ecx
    shrl     $2, %ecx
/*
** push long words in, faulting in OS pages along the way.
*/
loop1:
    pushl    $0
    loop     loop1
/*
** Restore the stack pointer and set the page base for stack allocation.
*/
    movl     %ebx, %esp
    movl     %eax, CSstk_pg_base
    popl     %ecx
    popl     %ebx
    leave    
    ret      
    movl     %eax, %eax

# endif /* ps2_us5  dr3_us5 sco_us5 ix3_us5 dra_us5 usl_us5 sui_us5 */

# endif	/* sqs_u42 sqs_us5 odt_us5 dr3_us5 ps2_us5 dra_us5 nc4_us5 */
