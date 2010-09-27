/*
** Copyright (c) 1991, 2009 Ingres Corporation
*/

/**
** Name: EXCL.H - Global definitions for the EX compatibility library.
**
** Description:
**      The file contains the type used by EX and the definition of the
**      EX functions.  These are used for exception handling.
**
** History:
**      01-oct-1985 (jennifer)
**          Updated to codinng standard for 5.0.
**	13-feb-1986 (jeff)
**	    Added EX_PSF_LJMP - Parser facility longjump.
**	20-may-86 (ericj)
**	    Added money math exceptions, EXMNYDIV and EXMNYOVF.
**	15-sep-1986 (fred)
**	    Added EXCONTINUE, EXDECLARE, and EXRESIGNAL.  According
**	    to the spec, there are not supposed to be _'s in these
**	    name, but they are left both ways for now.
**	10-nov-1986 (ericj)
**	    Fixed money math exception definitions so that they wouldn't
**	    collide with GV constants.
**	23-apr-1987 (Joe)
**	    Change EXOSLJMP to EXFEBUG.
**	17-dec-1987 (seputis)
**	    Added EXsys_report, and EX_MAXREPORT
**	19-may-88 (thurston)
**	    Added exception codes EXDECOVF and EXDECDIV.
**	30-nov-90 (jrb)
**	    Added VMS definitions for EXH* symbols.  These were needed for
**	    certain Unix platforms, so we needed dummy VMS counterparts to
**	    keep things uniform.  These exceptions will never be raised on
**	    VMS.
**	10-jan-91 (stec)
**	    Added EX_MAX_SYS_REP define.
**      01-sep-89 (jrb)
**          Added comment about interdependency of code with EXDECDIV.
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**	10-aug-93 (walt)
**	    Added symbolic names for the SS$_HPARITH exception summary word
**	    bits for EXcatch to use.  (HPARITH_BADARG, HPARITH_FLTDIV, ...)
**	23-mar-94 (dkh)
**		Added support for EXsetclient.
**	25-mar-94 (dkh)
**		Added return status for EXsetclient.
**	30-jan-1995 (wolf)
**		Added EXHANGUP as part of Nick's fix for bug 53269.  It has the
**		same value as EXKILLBACKEND, FEs should use EXHANGUP.
**	4-apr-1995 (dougb)
**		For VMS/AXP, use VAXC$ESTABLISH() as recommended by Digital.
**	7-nov-1995 (duursma)
**	   Rearrange EX_CONTEXT structure so that jmpbuf is naturally aligned.
**	   Change to EXdeclare macro: replace lib$get_current_invo_context()
**	   with call to new MACRO routine EXgetctx()
**	23-sep-1995 (kinte01)
**	   Added EX_DELIVER
**	09-nov-1999 (kinte01)
**	   Rename EXCONTINUE to EXCONTINUES per AIX 4.3 porting change
**	08-May-2000 (bonro01/ahaal01)
**	    AIX system header context.h has a conflicting definition
**	    for jmpbuf which redefined it to __jmpbuf so we changed
**	    variable jmpbuf to iijmpbuf to avoid compiler errors.
**	29-Jan-2001 (kinte01)
**	   Add prototypes for VMS specific routines here
**	13-mar-2001 (kinte01)
**	   Add indef EX_included for occasions when this file may be 
**	   include twice
**	18-May-2001 (hanje04)
**	    When crossing 447895 from oping20 to main changed iijmpbuf
**	    back to jmpbuf. Now change it back to iijmpbuf again
**	10-oct-2001 (kinte01)
**	   Keep excl.h from being included more than once
**	18-jun-2008 (joea)
**	   Correct EX_CONTINUES to EX_CONTINUE, as defined on Unix and Windows.
**      19-dec-2008 (stegr01)
**         Itanium VMS port. Removed obsolete VAX declarations.
**	05-jun-2009 (joea)
**	    On Alpha, use jmp_buf definition from setjmp.h.  Complete
**	    EXgetctx prototype.  Make #ifdefs dependent on release identifiers.
**  19-Jun-2009 (stegr01)
**      Changed EXdeclare for VMS Itanium so we can use the setjmp/longjmp
**      mechanism without the requirement that it cannot be used from 
**      exception handlers.
**      Both Alpha and Itanium now use the definition of jmp_buf supplied
**      by setjmp.h (an array of quadwords the size of an ICB)
**	29-jun-2009 (joea)
**	    Properly prototype the handler_address of EX_CONTEXT and EXsetup.
**	    Make address_check a PTR as on Unix.
**      16-jun-2010 (joea)
**          On Itanium, declare iijmpbuf as a pointer to the octaword-aligned
**          jmp_buf block.  Add 16-byte padding after the latter to account
**          for octaword re-alignment.  Use lib$i64_get_curr_invo_context in
**          EXdeclare macro.
**      07-sep-2010 (joea)
**          On i64_vms, replace VMS exception handling by POSIX signals as done
**          on Unix.
**/


#ifndef		EX_INCLUDED
#define		EX_INCLUDED     1

#if defined(i64_vms)
#define __FAST_SETJMP
#endif
#include <setjmp.h>

/* incremented for each EXdisable call and
** decremented for each EXenable call.  This is
** to prevent a nested disable/enable pair from
** enabling interrupts in the outer scope.
**
** NOTE: This is a hack!!!  EXdisable should
** probably return some indication of whether
** interrupts were already disabled and then
** EXenable could be conditionally called.
*/

GLOBALREF i4	EXintr_count;

/* 
** Defined Constants
*/

/*
**	Minimum length of buffer for EXsys_report().
*/
#define			EX_MAX_SYS_REP		256

/*
**	Maximum size of the buffer which is a parameter
**	to EXsys_report. This define is now OBSOLETE.
*/
#define                 EX_MAXREPORT		256

/* EX return status codes. */

/*	Standard Exceptions	*/

# define	EXFLTDIV	1172
# define        EXFLTINV        1316
# define	EXFLTOVF	1164
# define	EXFLTUND	1180
# define	EXINTDIV	1156
# define	EXINTOVF	1148
# define	EXDECOVF	1188

# define	EXSEGVIO	0x0c			
                                        /* access violation on vms */
# define	EXBUSERR	0x0c			
                                        /* access violation on vms */
# define	EXINTR		0x651			
                                        /* system-s-controlc -- operation 
                                        ** completed under ctl-c
					*/

/* UNIX specific for now */

#define EXRESVOP       (E_CL_MASK + E_EX_MASK + 0x69)
#define EXTRACE        (E_CL_MASK + E_EX_MASK + 0x6a)
#define EXTIMEOUT      (E_CL_MASK + E_EX_MASK + 0x6b)
#define EXCOMMFAIL     (E_CL_MASK + E_EX_MASK + 0x6f)
#define EXCHLDDIED     (E_CL_MASK + E_EX_MASK + 0x70)
#define EXUNKNOWN      (E_CL_MASK + E_EX_MASK + 0x71)

/* The following numbers were generated with the MESSAGE utility so they
** should be exception numbers which are never expected or generated by VMS.
*/
# define	EXHFLTDIV	0x0800800a
# define	EXHFLTOVF	0x08008012
# define	EXHFLTUND	0x0800801a
# define	EXHINTDIV	0x08008022
# define	EXHINTOVF	0x0800802a


# define        EX_OK1          0
# define	EXEXIT		(E_CL_MASK + E_EX_MASK + 0x01)
# define	EXBREAKQUERY	(E_CL_MASK + E_EX_MASK + 0x02)
# define	EXKILLBACKEND	(E_CL_MASK + E_EX_MASK + 0x03)
# define	EXHANGUP	(E_CL_MASK + E_EX_MASK + 0x03)
# define	EXQUIT		(E_CL_MASK + E_EX_MASK + 0x04)
# define	EXSTOP		(E_CL_MASK + E_EX_MASK + 0x05)
# define	EXFATER		(E_CL_MASK + E_EX_MASK + 0x06)
# define	EXRDB_LJMP	(E_CL_MASK + E_EX_MASK + 0x07)
# define	EXVFLJMP	(E_CL_MASK + E_EX_MASK + 0x08)
# define	EXVALJMP	(E_CL_MASK + E_EX_MASK + 0x09)
# define	EXCFJMP		(E_CL_MASK + E_EX_MASK + 0x0A)
# define	EX_MON_LJMP	(E_CL_MASK + E_EX_MASK + 0x0B)
# define	EX_CM_LJMP	(E_CL_MASK + E_EX_MASK + 0x0C)
# define	EX_JNP_LJMP	(E_CL_MASK + E_EX_MASK + 0x0D)
# define	EX_BT_LJMP	(E_CL_MASK + E_EX_MASK + 0x0E)
# define	EXNOTEX		(E_CL_MASK + E_EX_MASK + 0x0F)

# define	EXFEBUG		(E_CL_MASK + E_EX_MASK + 0x10)
# define	EXABFJMP	(E_CL_MASK + E_EX_MASK + 0x11)
# define	EXRWJMP		(E_CL_MASK + E_EX_MASK + 0x12)
# define	EXFEMEM		(E_CL_MASK + E_EX_MASK + 0x13)
# define	EXDIDBG		(E_CL_MASK + E_EX_MASK + 0x14)
# define	EXWINCONS	(E_CL_MASK + E_EX_MASK + 0x15)
# define	EXOINCONS	(E_CL_MASK + E_EX_MASK + 0x16)
# define	EX_PSF_LJMP	(E_CL_MASK + E_EX_MASK + 0x17)
# define	EX_DMF_FATAL	(E_CL_MASK + E_EX_MASK + 0x18)

/* Define ADF exceptions, numbers 0x30 thru 0x5F 
**
** NOTE: If you change EXDECDIV to something else, you must also change it
** in MACRO code where it is generated.  Currently, it is generated by
** MHpkdiv in MHDEC.MAR
*/
# define	EXMNYDIV	(E_CL_MASK + E_EX_MASK + 0x30)
# define	EXMNYOVF	(E_CL_MASK + E_EX_MASK + 0x31)
# define	EXDECDIV	(E_CL_MASK + E_EX_MASK + 0x32)

/* Values handlers can return to effect processing of the exception. */

/*
**  THESE THREE SYMBOLS ARE HARDCODED IN MACRO FILES. Change at your own risk
**  EX_ is used here so that the # define does not conflict with the function
**  definition of EXdeclare()
*/

# define	EX_RESIGNAL	(E_CL_MASK + E_EX_MASK + 0xFD)
# define	EX_CONTINUE	(E_CL_MASK + E_EX_MASK + 0xFE) 

# define	EX_DECLARE	(E_CL_MASK + E_EX_MASK + 0xFF)

# define	EXRESIGNAL	(E_CL_MASK + E_EX_MASK + 0xFD)
# define	EXCONTINUES	(E_CL_MASK + E_EX_MASK + 0xFE) 

# define	EXDECLARE	(E_CL_MASK + E_EX_MASK + 0xFF)


/*	Values for EXmath and Exinterrupt	*/

# define	EX_ON		(i4) 1
# define	EX_OFF		(i4) 0
# define	EX_RESET	(i4) 2
# define        EX_DELIVER      (i4) 3

/*	Special exception a handler is called with if it is going to be unwound	*/

# define	EX_UNWIND	2336


/* added for FE use */
# define	EXKILL		EXKILLBACKEND
# define	EXTEEOF		(-1)

/* 
**Type Definitions.
*/

/*
** Arguments to handlers are passed in this structure.
*/

# define	EX	i4

typedef struct
{
#if defined(axm_vms)
	i4		exarg_count;		/* Number of i4's in exarg_array */
	EX		exarg_num;		/* The exception being raised */
	i4		exarg_array[1];		/* arguments */
#else
    i4      exarg_count;            /* Number of i4's in exarg_array */
    EX      exarg_num;              /* The exception being raised */
    long    *exarg_array;           /* arguments */
    long    signal;                 /* 0 if not raised by signal */
    long    code;                   /* exception code */
    PTR     scp;                    /* system context pointer
                                       or NULL if raised by user */
#endif
} EX_ARGS;

/*
**  Client types for EXsetclient.
*/
# define	EX_INGRES_DBMS		0	/* client is Ingres dbms */
# define	EX_INGRES_TOOL		1	/* client is an Ingres tool */
# define	EX_USER_APPLICATION	2	/* client is user app */

/*
**  Return status if call to EXsetclient
**  is too late and will have no effect.
*/
# define	EXSETCLIENT_LATE	(E_CL_MASK + E_EX_MASK + 0xA0)
# define	EXSETCLIENT_BADCLIENT	(E_CL_MASK + E_EX_MASK + 0xA1)


# define EX_MAX_ARGS        4	/*  VMS hasn't had this parameter before.  */

typedef struct ex_context       /* same as on UNIX */
{
#if defined(axm_vms)
    jmp_buf                 iijmpbuf;
        PTR			address_check;
        struct ex_context	*prev_context;
        STATUS			(*handler_address)(EX_ARGS *args);
#else
    PTR                 address_check;
    struct ex_context   *prev_context;
    STATUS              (*handler_address)(EX_ARGS *args);
    jmp_buf             iijmpbuf;
#endif
} EX_CONTEXT;

FUNC_EXTERN void EXsetup(STATUS (*handler)(EX_ARGS *args), 
            EX_CONTEXT *context);

#if defined(axm_vms)
int EXcatch();
# define EXdeclare(x,y)		( VAXC$ESTABLISH( EXcatch ),\
				 EXsetup( x, y ),\
                                 EXgetctx( (y)->iijmpbuf ))
# define EXdelete()		( EXunsave_handler(), VAXC$ESTABLISH( NULL ))
#else
#define EXdeclare(x, y)        (EXsetup((x), (y)), setjmp((y)->iijmpbuf))
#endif

/*  Null expansion  - EXmath not available on Alpha  */

# define EXmath(arg)    

# define EXTRACT_FACILITY(cond) ( ((cond) >> 16) & 0xfff )

/*
**  Symbolic names for bits in the SS$_HPARITH signal array status summary
**  word.  Used by EXcatch to change the generic SS$_HPARITH arithmetic
**  exception into some CL standard exception.
*/

# define HPARITH_BADARG	    0x2
# define HPARITH_FLTDIV	    0x4
# define HPARITH_FLTOVF	    0x8
# define HPARITH_FLTUND	    0x10
# define HPARITH_NOTEXACT   0x20
# define HPARITH_INTOVF	    0x40
# define HPARITH_FLTINV     0x80

#if defined(axm_vms)
FUNC_EXTERN STATUS EXgetctx(jmp_buf jmpbuf);
FUNC_EXTERN int    EXunsave_handler(
            void
);
#endif

# endif /* EX_INCLUDED */
