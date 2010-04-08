/*
**Copyright (c) 2004 Ingres Corporation
*/
/****************************************************************************
This file contains assembly routines for INGRES 2.6 on HP PA-RISC 2.x


But first, an overview of what was done on this port:

rofix
-----
    Edits data declarations to move them into shared text instead of data.
    (Probably not needed when HP implements copy-on-write)

      o Delete all space and subspace declarations.
      o Declare the space $TEXT$ and subspace $LIT$
      o Add ",data" to all export statements.  This is the default in
        the data subspace, but not in $LIT$.
      o Hand compile gver.roc as a convention c program.  There is a very
        long string constant that the assembler won't assemble.  As an
        alternative, we could modify the edit script to break up large string 
        declarations into smaller ones.


Shared Memory
-------------  
    On the HP S800, the shmat() call always chooses a address.  The application
    may not choose the address.  On the other hand, all processes will always
    attach the same shared memory at the same address.  Thus, pointers located
    in shared memory will be valid.

   The changes included:
       
      o Specify an attach address of "0" for LG???.
      o Remove the check in ME?? that prohibits "specified" addresses of 0.

    As an additional restriction, the HP s800 will only attach a shared segment
    once to a particular process.  This restriction is not a problem with
    Ingres, but it causes some of the test programs to fail.

    The workaround consists of:

      o Create an intermediate attach routine hp8_us5_shmat() that replaces
        the standard shmat() routine used by ME_shared.c.
      o The intermediate attach routine detaches the segment first, then
        reattaches.  Unfortunately, this routine is not portable to other
        machines.
      o In the test program ????,  allow the segment to be attached at the
        same address both times.  This workaround defeats some of the features
        of this particular test.  A better solution is to redesign the test.

Test and Set
------------
    Instead of a "test and set" instruction, the HP s800 has a "load and clear
    word" instruction.  This instruction is equivalent to "test and set"
    except that:

      o The values are reversed.  With "load and clear word", zero means
        the word is locked and one means it is free.  This is not a problem
        if the code always invokes the CS_ACLR macro to initialize the word.
        (Had to add this to csev.c)

      o The word must be 16 byte aligned.  To get around this limitation,
        the data type is declared as int[4], and the access routines
        always round up to a 16 byte address.

    As a note,  these routines should be written as subroutines and not
    as in-line macros.   Global optimizers in general, and HP's in
    particular, will often move data into registers and not check memory 
    to see if the values have changed.  Making a subroutine call forces the 
    shared data to be posted from registers into memory.  (Actually, it
    forces all externals and pointer data to be posted, which generally
    includes all shared memory.)
  
SIGCHILD Exception Handling
---------------------------
    The HP s800 does some special handling for SIGCHILD that causes the
    exception handler to go into an infinite, recursive loop.  According
    to the "man" page, the special handling was done for compatibility with
    other UNIX operating systems.  But the other systems seem to work OK.

    At any rate, the problem is as follows:

      o When a declaring a SIGCHILD handler, the kernel checks to see if
        there are any zombie processes already waiting.  If so, it immediately
        generates another SIGCHILD signal.
      o The Ingres signal handler takes the following steps.
         1) It reinstates the original signal handler (In this case, ignore).
         2) Using kill(), it sends SIGCHILD to the process.
         3) It reinstates itself as the signal handler and continues.
      o The problem arises in step 3) above.  HP-UX generates an immediate
        SIG_CHILD, which then invokes the handler recursively.
        
    As a workaround, we commented out the one place where the SIGCHILD
    handler is invoked.  There doesn't appear to be any other references 
    to SIGCHILD anywhere else in Ingres.  A better "fix" is to just ignore
    SIGCHILD if a specific handler hasn't been set up. (and display a warning
    if the previous handler wasn't the default?)

Floating Pt exceptions
----------------------
    When returning from a floating pt exception, you must clear the "T-bit"
    in the floating pt register or the exception will repeat itself forever.
    Likewise, when returning from an integer divide by zero, must set the
    "nullify bit".  

    Added an additional routine to enable floating pt exceptions.

History:
	16-sep-2002 (somsa01)
	    Created for PA-RISC 2.0W (64-bit).
     
*****************************************************************************/





.LEVEL 2.0W

#include "/lib/pcc_prefix.s"
/*************************************************************************

     char stack[stacksize];
     thread thread;
     thread oldthread, newthread;

     CS_create_ingres_thread(thread, start, exit, stack, stacksize);
         -- initializes a new thread

     CS_swap_thread(oldthread, newthread)
         -- switches context to the new thread

   Threads are very similar to processes, but they all take place
   inside one UNIX process.  Since the operating system is not involved,
   the context switch time is very short.

   When a thread is activated the first time, the 'procedure' procedure is 
   invoked using the specified stack area.  When the start procedure finishes,
   the exit procedure is invoked.
****************************************************************************/

    .code
#define thread    arg0
#define procedure arg1
#define exit      arg2
#define stack     arg3
#define size      arg4
#define temp      r31

    .import _setjmp
    .import _longjmp

; offsets within the 'thread' structure
stktop    .equ 0
stkbottom .equ 8 
jmpbuf    .equ 16

; stack frame size
fs        .equ 80

; offset in stack to save the global pointer
save_gp   .equ -32


; void CS_create_ingres_thread(thread, procedure, exit, stack, size);
;************************************************************************
; CS_create_ingres_thread creates a thread environment with a new stack
;************************************************************************
    .proc
    .callinfo caller, save_rp
    .export CS_create_ingres_thread, entry
    .align 32

CS_create_ingres_thread
    .enter

; save the stack definitions
    std stack, stkbottom(thread)
    add stack, size, temp
    std temp, stktop(thread)

; allocate 2 frames and align the new stack area to a 16 byte boundary
    addi  fs+fs+15, stack, stack   ; add 15 to round up and add 160 for 2 frames
    depdi 0, 63, 4, stack          ; clear the low 4 bits, effect is to align

; save the appripriate variables in the current stack frame
    std r0, -fs-16(stack)          ; set the return address (rp) of first frame
                                   ; to zero so debugger trace stops
    std sp, -24(stack)             ; save the current stack pointer in arg7 of
				   ; second frame so it can be restored
    std procedure, -fs-72(stack)   ; save the procedure address in arg1 of
				   ; first frame to invoke later on
    std exit, -fs-80(stack)        ; save the exiter address in arg0 of first
				   ; frame to invoke after the procedure
    std gp, save_gp(stack)         ; save the global pointer in arg6 of second
				   ; frame, restored after each procedure call

; switch to the new stack and save the environment
    copy  stack, sp
    bl   _setjmp, rp
    ldo  jmpbuf(thread), arg0
    ldd  save_gp(sp), gp

if1 comb,=,n  ret0, r0, endif1

; ... invoke the procedure
    addi  -16, sp, r29       ; set the argument pointer to the end of the
			     ; register argument area
    ldd   -fs-72(sp), r18    ; get the procedure address from the stack
    ldd   16(r18), r31       ; get the address of the start routine
    bve,l (r31), rp          ; branch to the start routine
    ldd   24(r18), gp        ; get the gp of the start routine

    addi  -16, sp, r29       ; set the argument pointer to the end of the
			     ; register argument area
    ldd   -fs-80(sp), r18    ; get the exiter address from the stack
    ldd   16(r18), r31       ; get the address of the start routine
    bve,l (r31), rp          ; branch to the start routine
    ldd   24(r18), gp        ; get the gp of the start routine

; the invoked procedure should not return
    break
endif1

; done
    ldd -24(sp), sp   ; restore old stack pointer
    .leave            ; return
    .procend


; CS_swap_thread(oldthread, newthread)
;********************************************************************
; CS_swap_thread switches context from one thread to another
;********************************************************************
    .proc
    .callinfo caller, frame=0, entry_gr=3, save_rp
    .export CS_swap_thread, entry
CS_swap_thread
    .enter

; save pointer to new stack
    std   gp, save_gp(sp)
    copy  arg1, r3

; save the current context with _setjmp
    bl    _setjmp, rp
    ldo   jmpbuf(arg0), arg0
    ldd   save_gp(sp), gp

if2 comib,<>,n   0, ret0, endif2
    bl    _longjmp, rp
    ldo   jmpbuf(r3), arg0
    ldd   save_gp(sp), gp
endif2

; done
    .leave
    .procend


/***********************************************************************
This routine makes the 'Load and clear word' instruction available
  to higher level languages.  It accepts the address of a sixteen byte
  aligned word,  fetches its value, and clears it in one indivisible 
  operation.  In general, the word will be located in shared memory.
****************************************************************************/

#define sem arg0
#define temp r31


; int CS_tas (sem);
/*****************************************************************
CS_tas atomically locks the word and returns true if the word was 
    previously unlocked.
******************************************************************/
    .proc
    .callinfo no_calls
    .export CS_tas, entry
CS_tas

; round the address to a 16 byte boundary
    addi 15, sem, sem
    depi 0, 31, 4, sem

; return after doing a "load and clear word"
    bv      (rp)
    ldcws   (sem), ret0 
    .procend


; void CS_aclr(sem)
;**************************************************************************
; CS_aclr clears the word to the unlocked state.
;**************************************************************************
    .proc
    .callinfo no_calls
    .export CS_aclr, entry
CS_aclr

; round the address to a 16 byte boundary
    addi 15, sem, sem
    depi 0, 31, 4, sem

; return after storing a one in the word
    ldi   1, temp
    bv    (rp)
    stw   temp, (sem)
    .procend



; void CS_isset(sem)
;**************************************************************************
; CS_isset is true if the word is locked
;**************************************************************************
    .proc
    .callinfo no_calls
    .export CS_isset, entry
CS_isset

; round the address to a 16 byte boundary
    addi 15, sem, sem
    depi 0, 31, 4, sem

; get the current value of the word  (0 or 1)
    ldw   (sem), temp

; return   (value == 0)
    bv    (rp)
    subi  1, temp, ret0   ; assumes only values are 0 or 1
    .procend




;*********************************************************************
; Routines to enable and disable floating pt exceptions on the HP S800.
;   These routines enable the IEEE hardware traps that are associated
;   with typical floating pt arithmetic. 
;
;   The enabled traps consist of:
;      invalid   -- the operation or arguments are not valid
;      overflow  -- the result is too large to represent
;      zero divide -- attempted to divide by zero.
;
;   The following traps are NOT enabled:
;      underflow -- the numbers are too close to zero to represent
;      inexact -- some roundoff occurred. (mignt be useful for money routines)
;
;   By default, exceptions are disabled when a process starts.
;
; Be aware of the following quirks of overflow handling:
;
;   o If an overflow occurs while converting from floating pt to integer,
;     an exception will ALWAYS occur, whether exceptions are enabled or not.
;     The IEEE standard recommends that this case be considered an invalid
;     operation which should be masked by the invalid bit.  This appears to
;     be a bug in the S800 floating pt handler. (HP-UX 3.1)
;
;   o Integer overflow and integer divide by zero also generate a SIGFPE
;     signal.  These are distinguished from real floating pt exceptions
;     by examining the "code" variable passed to the handler.  if code == 13,
;     than an integer trap occurred.  (Normally, "C" does not generate
;     integer overflow traps.)
;
;   o When returning from a floating pt exception handler, you must
;     clear the "T-BIT" or the exception will immediately repeat itself.
;     See the hardware reference manual for a description of the T-BIT and
;     look at the sigcontext structure in <signal.h>. 
;
;   o When returning from an integer trap, you must set the "nullification"
;     bit instead of clearing the "T-BIT" to avoid repeating the exception.
;
;   o You should NOT ignore the SIGFPE signal.  The S800 will reexecute the
;     instruction, causing an infinite loop.
; 
;*************************************************************************


/* register assignments */
#define status r1
#define mem    r31


; enable_fpe()
;*********************************************************
; enable turns on floating point exceptions
;*********************************************************
; Note:  The S800 cannot transfer directly from general registers to
;   floating pt registers.  Must store to memory first.
    .proc
    .export enable_fpe, entry
    .callinfo caller, frame=8
enable_fpe
    .enter

; point to a memory location to use 
    ldo      -56(sp), mem

; set the VZO bits (Invalid, Divide by zero, Overflow)
    ldi       0x1c, status

; load the floating pt status register
    stw      status, (mem)  ; save new status into memory
    fldws    (mem), 0       ; load new status into floating pt register

; done
    .leave
    .procend



; disable_fpe()
;*********************************************************
; disable turns off floating point exceptions
;*********************************************************
; Note:  As of hp-ux 3.1, the float->int overflow trap cannot be disabled.
    .proc
    .export disable_fpe, entry
    .callinfo caller, frame=8
disable_fpe
    .enter

; point to a memory location to use 
    ldo      -56(sp), mem

; load the floating pt status register with zeros
    stw      0, (mem)       ; save word of zeros in memory
    fldws    (mem), 0       ; load zeros into floating pt register

; done
    .leave
    .procend
