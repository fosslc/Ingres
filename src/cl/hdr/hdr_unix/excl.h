/*
** Copyright (c) 2004 Ingres Corporation
*/

# ifndef EX_included

# define	EX_included

# include	<systypes.h>
# include	<setjmp.h>

/**
** Name: EX.H - Global definitions for the EX compatibility library.
**
** Description:
**      The file contains the type used by EX and the definition of the
**      EX functions.  These are used for exception handling.
**
**	This is the unix-only header variant.
**
** History: 
 * Revision 1.5  89/02/06  12:53:22  jeff
 * Try using _setjmp once more
 * Discovered that SunOS4.0's dynamically linked _setjmp corrupted register
 * Now we force _setjmp.o to be staticly linked
 * 
 * Revision 1.3  89/01/25  15:53:25  mikem
 * define BAD_SETJMP so that EXdeclare will use setjmp().
 * The current _setjmp() routine in sun OS4.0 does not save and restore
 * some register variables (at least a2).  This will cause spurious bugs
 * on the system.
 * 
 * Revision 1.2  89/01/23  15:13:17  jeff
 * add EX_DELIVER & use _setjmp where possible
 * 
 * Revision 1.5  88/03/11  14:44:24  mikem
 * Added EXKILLBACKEND back into ex.h so that front end
 * group can compile.  They should not be using 
 * EXKILLBACKEND, but instead the new signal EXHANGUP.
 * 
 * Revision 1.4  88/03/04  12:28:09  anton
 * new exceptions for jupiter
 * 
**	23-apr-1987 (Joe)
**	    Change EXOSLJMP to EXFEBUG.
**	13-mar-89 (russ)
**	    Changed BAD_SETJMP to xCL_038_BAD_SETJMP.
**	11-may-89 (daveb)
**		Remove RCS log and useless old history.
**	21-jun-89 (jrb)
**	    Added EXDECDIV and changed EXDEVOVF to be in the ADF range.
**	22-may-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Add defineds for "hard" case remapping;
**		EXen_kybd_intrpt, EXds_kybd_intrpt were removed so make
**		references to them no-ops;
**		Use VOID instead of 'void' for functions, to allow the 
**		redefinition of VOID in compat.h for compilers which don't
**		like handling pointers to functions of type 'void';
**		Only cvx_u42 will call EXmath as a function, put in special
**		for this.
**      07-jan-91 (jkb)
**          Add a floating point no-op to EXmath for sequent.
**          This is necessary to force FP exceptions from the 387 floating
**          point to be returned when they happen rather than with the next
**          floating point expression.
**      10-jan-91 (bonobo)
**          Added EX_MAX_SYS_REP define.
**	16-Mar-91 (seng)
**	    Add section to redefine EXCONTINUE if it was defined already.
**	    RS/6000 AIX 3.1 defines EXCONTINUE in <sys/context.h>
**	    Add <systypes.h> to clear <sys/types.h> include by system
**	    header files on the RS/6000.
**	23-apr-91 (seng)
**	    Remove ris_us5 dependency to define EXCONTINUE for other ports.
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**	7-Jul-1993 (daveb)
**	    Add FUNC_EXTERN for internal EXsetup routine.  It expects 
**	    a handler that returns STATUS.
**      2-Sep-1993 (smc)
**          Made address_check a PTR so that is can continue to be used
**          as an address holder on axp_osf.
**      22-Sep-1993 (smc)
**          Added new Unix CL specific elements to EX_ARGS to allow
**	    passing of exception condition data on boxes where this
**	    won't fit into i4s.
**	    These fields are currently unused, they have been added as
**	    a hook for my next integration.
**      22-Oct-1993 (smc)
**	    Bug #56446
**          Now using new elements to EX_ARGS to allow passing of exception
**          condition data on boxes where this won't fit into i4s.
**	21-mar-94 (dkh)
**		Added support for new entry EXsetclient.
**	25-mar-94 (dkh)
**		Added return STATUS EXSETCLIENT_LATE to indicate
**		that call to EXsetclient was after the EX subsystem
**		has been initialized and will have no effect.  Also,
**		EXSETCLIENT_BADCLIENT will be returned if an unknown
**		client was passed in.
**	 9-oct-1995 (nick)
**	    Added Unix CL only EXaltstack().
**	16-jan-1996 (toumi01; from 1.1 axp_osf port) (schte01)
**		Changed EX_ARG structure elements *exargs_array, signal and code
**		to long to avoid truncation (can hold addresses).
**	23-jan-1997(angusm)
**		Add PCCLEANUP for su4_us5, axp_osf, ds3 (add others
**		if you've tested it and it works): part of fix for
**		bug 62894
**	15-aug-1997 (canor01)
**	    Complete prototype for EXsetup() by adding "EX_ARGS*".
**      18-aug-1997 (hweho01)
**              Renamed EX_OK to EX_OK_DGI for dgi_us5, because   
**              it is defined in /usr/include/sys/_int_unistd.h.
**      13-jan-1999 (hweho01)
**              Changed EX_OK to EX_OK1 for all platforms.  
**	10-may-1999 (walro03)
**		Remove obsolete version string cvx_u42, sqs_us5.
**      02-Aug-1999 (hweho01)
**              1) Changed EXCONTINUE to EXCONTINUES to avoid compile
**                 error of redefinition on AIX 4.3, it is used
**                 in <sys/context.h>.
**              2) Undefined jmpbuf to avoid compile errors from   
**                 the replacement of jmpbuf in ex_context structure. 
**                 In <sys/context.h>, jmpbuf is defined to __jmpbuf.  
**	18-oct-2000 (hayek02)
**		For su4_us5 declare context.jmpbuf as a sigjmp_buf
**		(array of 19 - _SIGJBLEN - ints) rather than a jmp_buf
**		(array of 12 - _JBLEN - ints). This prevents corruption
**		of local variables when setjmp() is called via the
**		EXdeclare() macro, when 76 bytes, rather than 48 bytes,
**		are written from the start of context.jmpbuf. This
**		changes fixes bug 102961.
**	08-May-2000 (bonro01/ahaal01)
**		AIX system header context.h has a conflicting definition
**		for jmpbuf which redefined it to __jmpbuf so we changed 
**		variable jmpbuf to iijmpbuf to avoid compiler errors.
**	18-May-2001 (hanje04)
**		When crossing 447895 from oping20 to main changed iijmpbuf
**		back to jmpbuf. Now change it back to iijmpbuf again
**      28-Dec-2003 (horda03) Bug 111532/INGSRV2649
**              On AXP_OSF the EX_MAX_SYS_REP must be sufficent to
**              display the value of 32 pointers (i.e. greater than
**              512 bytes). To ensure space for additional characters
**              (such as field names), double this size.
**      25-feb-2004 (horda03) Bug 111532/INGSRV2649
**              Typo in previous change, AXP_OSF should be axp_osf.
**	13-Apr-2006 (hanje04)
**	    BUG 118063
**	    EX_MAX_SYS_REP is to small for 64bit platforms (16 character
**	    hex addresses etc). At 256 bytes the buffer in ulx_exception is
**	    easily overrun by EXsys_report causing stack corruption and all
**	    sorts of mess. Increase to 1024 bytes for ALL 64bit platforms,
**	    not just axp_osf.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**	15-Dec-2009 (frima01) Bug 122490
**	    Added prototype for EXsetsig to eliminate gcc 4.3 warnings.
**	    clconfig.h needs to be included as well for the return type.
**	18-Nov-2010 (kschendel) SIR 124685
**	    Not much point in having non-Unix conditionals in a unix only
**	    header.  Delete the noise.
**/

/*
**  Forward and/or External typedef/struct references.
*/

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
**      Minimum length of buffer for EXsys_report().
*/
#ifdef LP64
#define			EX_MAX_SYS_REP		1024
#else
#define                 EX_MAX_SYS_REP          256
#endif /* axp_osf */


/* EX return status codes. */

/*	Standard Exceptions	*/

# define        EX_OK1          0
# define	EXEXIT		(E_CL_MASK + E_EX_MASK + 0x01)
# define	EXBREAKQUERY	(E_CL_MASK + E_EX_MASK + 0x02)
# define	EXHANGUP	(E_CL_MASK + E_EX_MASK + 0x03)
/* we should be getting rid of this signal */
# define	EXKILLBACKEND	(E_CL_MASK + E_EX_MASK + 0x03)

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

/* Define ADF exceptions, numbers 0x30 thru 0x5F */
# define	EXMNYDIV	(E_CL_MASK + E_EX_MASK + 0x30)
# define	EXMNYOVF	(E_CL_MASK + E_EX_MASK + 0x31)
# define	EXDECDIV	(E_CL_MASK + E_EX_MASK + 0x40)
# define	EXDECOVF	(E_CL_MASK + E_EX_MASK + 0x41)

/*	Standard Exceptions	*/

# define	EXFLTDIV	(E_CL_MASK + E_EX_MASK + 0x60)
# define	EXFLTOVF	(E_CL_MASK + E_EX_MASK + 0x61)
# define	EXFLTUND	(E_CL_MASK + E_EX_MASK + 0x62)
# define	EXINTDIV	(E_CL_MASK + E_EX_MASK + 0x63)
# define	EXINTOVF	(E_CL_MASK + E_EX_MASK + 0x64)

# define	EXSEGVIO	(E_CL_MASK + E_EX_MASK + 0x65)			
					/* access violation on vms */
# define	EXBUSERR	(E_CL_MASK + E_EX_MASK + 0x66)			
					/* access violation on vms */
# define	EXINTR		(E_CL_MASK + E_EX_MASK + 0x67)
					/* system-s-controlc -- operation 
					** completed under ctl-c */

/* UNIX specific for now */

# define	EXFLOAT		(E_CL_MASK + E_EX_MASK + 0x68)
# define	EXRESVOP	(E_CL_MASK + E_EX_MASK + 0x69)
# define	EXTRACE		(E_CL_MASK + E_EX_MASK + 0x6a)
# define	EXTIMEOUT	(E_CL_MASK + E_EX_MASK + 0x6b)
# define	EX_NO_FD	(E_CL_MASK + E_EX_MASK + 0x6d)
# define	ME_00_ARG	(E_CL_MASK + E_EX_MASK + 0x6e)
# define	EXCOMMFAIL	(E_CL_MASK + E_EX_MASK + 0x6f)
# define	EXCHLDDIED	(E_CL_MASK + E_EX_MASK + 0x70)
# define	EXUNKNOWN	(E_CL_MASK + E_EX_MASK + 0x71)

/* you can't EXCONTINUES from these "hard" exceptions */
# define        EXHFLTDIV       (E_CL_MASK + E_EX_MASK + 0x72)
# define        EXHFLTOVF       (E_CL_MASK + E_EX_MASK + 0x73)
# define        EXHFLTUND       (E_CL_MASK + E_EX_MASK + 0x74)
# define        EXHINTDIV       (E_CL_MASK + E_EX_MASK + 0x75)
# define        EXHINTOVF       (E_CL_MASK + E_EX_MASK + 0x76)

/* exception to indicate that subprocesses to be forcibly terminated */
# if defined(sparc_sol) || defined (axp_osf)
# define        EXCLEANUP       (E_CL_MASK + E_EX_MASK + 0x77)
# endif


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
# ifdef EXCONTINUES
# undef		EXCONTINUES	
# endif 
# define	EXCONTINUES	(E_CL_MASK + E_EX_MASK + 0xFE) 

# define	EXDECLARE	(E_CL_MASK + E_EX_MASK + 0xFF)


# define	EX_UNWIND	(E_CL_MASK + E_EX_MASK + 0xFC)


/*	Values for EXmath and Exinterrupt	*/

# define	EX_OFF		(i4) 0
# define	EX_ON		(i4) 1
# define	EX_RESET	(i4) 2
/*	EXinterrupt only			*/
# define	EX_DELIVER	(i4) 3

/* added for FE use */
# define	EXKILL		EXKILLBACKEND
# define	EXTEEOF		(-1)

/* The "NO_EX" EXdeclare and i_EXreturn are macros */

/* if BAD_SETJMP is defined, it means that _setjmp() can't be called directly
** by our code.  In that case, we will call setjmp() at a performance penalty.
*/
# ifdef	xCL_038_BAD_SETJMP
# define	EXdeclare(x,y)	(EXsetup(x,y), setjmp((y)->iijmpbuf))
# define	i_EXreturn(x,y)	longjmp((x)->iijmpbuf, y)
# else
# define	EXdeclare(x,y)	(EXsetup(x,y), _setjmp((y)->iijmpbuf))
# define	i_EXreturn(x,y)	_longjmp((x)->iijmpbuf, y)
# endif  /* ifdef xCL_038_BAD_SETJMP  */

/* On UNIX, EXmath is a no-op.  save the function call overhead */

# define	EXmath(arg)	/* Null expansion */

/*
** These routines may no longer be necessary so make references to them no-ops.
*/
# define        EXen_kybd_intrpt()      /* Null expansion */
# define        EXds_kybd_intrpt()      /* Null expansion */


/* 
**Type Definitions.
*/

/*
** Arguments to handlers are passed in this structure.
*/

# define	EX	i4

typedef struct
{
	i4	exarg_count;		/* Number of i4's in exarg_array */
	EX	exarg_num;		/* The exception being raised */
	long	*exarg_array;		/* arguments */
	long	signal;                 /* 0 if not raised by signal */
	long	code;                   /* exception code */
	PTR     scp;                    /* system context pointer
					   or NULL if raised by user */
} EX_ARGS;


/*
** This is the "context" structure used to take a non-local goto
** when a handler returns EXDECLARE.
*/

typedef struct ex_context
{
	PTR			address_check;
	struct ex_context	*prev_context;
	STATUS			(*handler_address)();
# ifndef sparc_sol
	jmp_buf			iijmpbuf;
# else
	sigjmp_buf		iijmpbuf;
# endif
} EX_CONTEXT;


FUNC_EXTERN void EXsetup( 
	STATUS (*handler)(EX_ARGS *args), 
	EX_CONTEXT *context 
);

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
# define        EXSETCLIENT_LATE	(E_CL_MASK + E_EX_MASK + 0xA0)
# define        EXSETCLIENT_BADCLIENT	(E_CL_MASK + E_EX_MASK + 0xA1)

# endif	/* EX_included */

