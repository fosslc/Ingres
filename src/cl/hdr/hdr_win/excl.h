/*
** Copyright (c) 1995, 2003 Ingres Corporation
*/
# ifndef EX_included
# define EX_included

# include   <setjmp.h>
# include   <signal.h>

/**
** Name: EX.H - Global definitions for the EX compatibility library.
**
** Description:
**      The file contains the type used by EX and the definition of the
**      EX functions.  These are used for exception handling.
**
**	13-may-95 (emmag)
**		File branched for NT port of OpenINGRES.
**	15-aug-1997 (canor01)
**	    Fixed compiler warnings.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	15-nov-1999 (somsa01)
**	    Changed EXCONTINUE to EXCONTINUES.
**	08-feb-2001 (somsa01)
**	    On i64_win, EX_CONTEXT needs to be aligned on a 16-byte boundary.
**	    Also, changed the types of the marker structure members to
**	    SIZE_TYPE as well as the type of the argument array in
**	    EX_ARGS.
**	17-mar-2003 (mcgem01)
**	    Include signal.h for the Windows platform.
**	14-mar-2003 (somsa01)
**	    Changed EXexit() into EX_TLSCleanup().
**	02-oct-2003 (somsa01)
**	    Ported to NT_AMD64.
**/


/*
** Defined Constants
*/

/*
**      Minimum length of buffer for EXsys_report().
*/
#define         EX_MAX_SYS_REP          256

/*
**  Client types for EXsetclient.
*/
# define        EX_INGRES_DBMS          0       /* client is Ingres dbms */
# define        EX_INGRES_TOOL          1       /* client is an Ingres tool */
# define        EX_USER_APPLICATION     2       /* client is user app */

/*
**  Return status if call to EXsetclient
**  is too late and will have no effect.
*/
# define        EXSETCLIENT_LATE        (E_CL_MASK + E_EX_MASK + 0xA0)
# define        EXSETCLIENT_BADCLIENT   (E_CL_MASK + E_EX_MASK + 0xA1)

/* EX return status codes. */

/*  Standard Exceptions */

# define    EX_OK           0
# define    EXEXIT          (E_CL_MASK + E_EX_MASK + 0x01)
# define    EXBREAKQUERY    (E_CL_MASK + E_EX_MASK + 0x02)
# define    EXHANGUP        (E_CL_MASK + E_EX_MASK + 0x03)
# define    EXKILLBACKEND   (E_CL_MASK + E_EX_MASK + 0x03)

# define    EXQUIT       (E_CL_MASK + E_EX_MASK + 0x04)
# define    EXSTOP       (E_CL_MASK + E_EX_MASK + 0x05)
# define    EXFATER      (E_CL_MASK + E_EX_MASK + 0x06)
# define    EXRDB_LJMP   (E_CL_MASK + E_EX_MASK + 0x07)
# define    EXVFLJMP     (E_CL_MASK + E_EX_MASK + 0x08)
# define    EXVALJMP     (E_CL_MASK + E_EX_MASK + 0x09)
# define    EXCFJMP      (E_CL_MASK + E_EX_MASK + 0x0A)
# define    EX_MON_LJMP  (E_CL_MASK + E_EX_MASK + 0x0B)
# define    EX_CM_LJMP   (E_CL_MASK + E_EX_MASK + 0x0C)
# define    EX_JNP_LJMP  (E_CL_MASK + E_EX_MASK + 0x0D)
# define    EX_BT_LJMP   (E_CL_MASK + E_EX_MASK + 0x0E)
# define    EXNOTEX      (E_CL_MASK + E_EX_MASK + 0x0F)

# define    EXFEBUG      (E_CL_MASK + E_EX_MASK + 0x10)
# define    EXABFJMP     (E_CL_MASK + E_EX_MASK + 0x11)
# define    EXRWJMP      (E_CL_MASK + E_EX_MASK + 0x12)
# define    EXFEMEM      (E_CL_MASK + E_EX_MASK + 0x13)
# define    EXDIDBG      (E_CL_MASK + E_EX_MASK + 0x14)
# define    EXWINCONS    (E_CL_MASK + E_EX_MASK + 0x15)
# define    EXOINCONS    (E_CL_MASK + E_EX_MASK + 0x16)
# define    EX_PSF_LJMP  (E_CL_MASK + E_EX_MASK + 0x17)
# define    EX_DMF_FATAL (E_CL_MASK + E_EX_MASK + 0x18)

/* Define ADF exceptions, numbers 0x30 thru 0x5F */
# define    EXMNYDIV    (E_CL_MASK + E_EX_MASK + 0x30)
# define    EXMNYOVF    (E_CL_MASK + E_EX_MASK + 0x31)
# define    EXDECDIV    (E_CL_MASK + E_EX_MASK + 0x40)
# define    EXDECOVF    (E_CL_MASK + E_EX_MASK + 0x41)

/*  Standard Exceptions */

# define    EXFLTDIV    (E_CL_MASK + E_EX_MASK + 0x60)
# define    EXFLTOVF    (E_CL_MASK + E_EX_MASK + 0x61)
# define    EXFLTUND    (E_CL_MASK + E_EX_MASK + 0x62)
# define    EXINTDIV    (E_CL_MASK + E_EX_MASK + 0x63)
# define    EXINTOVF    (E_CL_MASK + E_EX_MASK + 0x64)

# define    EXSEGVIO    (E_CL_MASK + E_EX_MASK + 0x65)
                    /* access violation on vms */
# define    EXBUSERR    (E_CL_MASK + E_EX_MASK + 0x66)
                    /* access violation on vms */
# define    EXINTR      (E_CL_MASK + E_EX_MASK + 0x67)
                    /* system-s-controlc -- operation
                    ** completed under ctl-c */

/* UNIX specific for now */

# define    EXFLOAT     (E_CL_MASK + E_EX_MASK + 0x68)
# define    EXRESVOP    (E_CL_MASK + E_EX_MASK + 0x69)
# define    EXTRACE     (E_CL_MASK + E_EX_MASK + 0x6a)
# define    EXTIMEOUT   (E_CL_MASK + E_EX_MASK + 0x6b)
# define    EX_NO_FD    (E_CL_MASK + E_EX_MASK + 0x6d)
# define    ME_00_ARG   (E_CL_MASK + E_EX_MASK + 0x6e)
# define    EXCOMMFAIL  (E_CL_MASK + E_EX_MASK + 0x6f)
# define    EXCHLDDIED  (E_CL_MASK + E_EX_MASK + 0x70)
# define    EXUNKNOWN   (E_CL_MASK + E_EX_MASK + 0x71)

/* you can't EXCONTINUES from these "hard" exceptions */
# define    EXHFLTDIV       (E_CL_MASK + E_EX_MASK + 0x72)
# define    EXHFLTOVF       (E_CL_MASK + E_EX_MASK + 0x73)
# define    EXHFLTUND       (E_CL_MASK + E_EX_MASK + 0x74)
# define    EXHINTDIV       (E_CL_MASK + E_EX_MASK + 0x75)
# define    EXHINTOVF       (E_CL_MASK + E_EX_MASK + 0x76)

/* i386 specific */

# define        EXBNDVIO        ( (EX) (E_CL_MASK + E_EX_MASK + 0x80))
# define        EXOPCOD         ( (EX) (E_CL_MASK + E_EX_MASK + 0x81))
# define        EXNONDP         ( (EX) (E_CL_MASK + E_EX_MASK + 0x82))
# define        EXNDPERR        ( (EX) (E_CL_MASK + E_EX_MASK + 0x83))

/* Values handlers can return to effect processing of the exception. */

/*
**  THESE THREE SYMBOLS ARE HARDCODED IN MACRO FILES. Change at your own risk
**  EX_ is used here so that the # define does not conflict with the function
**  definition of EXdeclare()
*/
# define    EX_RESIGNAL (E_CL_MASK + E_EX_MASK + 0xFD)
# define    EX_CONTINUE (E_CL_MASK + E_EX_MASK + 0xFE)
# define    EX_DECLARE  (E_CL_MASK + E_EX_MASK + 0xFF)

# define    EXRESIGNAL  (E_CL_MASK + E_EX_MASK + 0xFD)
# define    EXCONTINUES (E_CL_MASK + E_EX_MASK + 0xFE)

# define    EXDECLARE   (E_CL_MASK + E_EX_MASK + 0xFF)
# define    EX_UNWIND   (E_CL_MASK + E_EX_MASK + 0xFC)


/*  Values for EXmath and Exinterrupt   */

# define    EX_OFF      (i4) 0
# define    EX_ON       (i4) 1
# define    EX_RESET    (i4) 2
/*  EXinterrupt only            */
# define    EX_DELIVER  (i4) 3

/* added for FE use */
# define    EXKILL      EXKILLBACKEND
# define    EXTEEOF     (-1)

/*
** These routines may no longer be necessary so make references to them no-ops.
*/
# define        EXen_kybd_intrpt()      /* Null expansion */
# define        EXds_kybd_intrpt()      /* Null expansion */


/*
**Type Definitions.
*/

#define EXMAXARGS   16
typedef ULONG       EX;

/*
** Arguments to handlers are passed in this structure.
*/
typedef struct _EX_ARGS EX_ARGS;
struct _EX_ARGS
{
    EX_ARGS     *next;      /* for queueing purpose */
    EX          exarg_num;  /* the exception being raised */
    i4          exarg_count;    /* count of arguments for the handler */
    SIZE_TYPE   exarg_array[EXMAXARGS]; /* argument array */
};
/*
** This is the "context" structure used to take a non-local goto
** when a handler returns EXDECLARE.
**/
#define  EX_MARKER_1 0x00123cde
// Last 4 hex digits will contain Thread ID.
#define  EX_MARKER_2 0xedc30000

/*
** On IA64, this needs to be aligned on a 16-byte boundary, since it
** contains jmp_buf.
*/
#if defined(NT_IA64) || defined(NT_AMD64)
typedef __declspec(align(16)) struct _EX_CONTEXT EX_CONTEXT;
#else
typedef struct _EX_CONTEXT EX_CONTEXT;
#endif

struct _EX_CONTEXT
{
    SIZE_TYPE   marker_1;
    EX_CONTEXT  *next;  /* for queueing purpose */
    STATUS      (*handler)(EX_ARGS *);
    jmp_buf	jmpbuf;
    SIZE_TYPE   marker_2;
};


/*
**  Forward and/or External function references.
*/

#define EXdeclare(x,y)  (EXsetup(x,y), setjmp((y)->jmpbuf))

FUNC_EXTERN VOID    EXinterrupt(i4 new_state);
FUNC_EXTERN i4      EXsetup(STATUS (*handler)(EX_ARGS *), EX_CONTEXT *context);
FUNC_EXTERN VOID    EXstartup();
FUNC_EXTERN VOID    EX_TLSCleanup();
FUNC_EXTERN DWORD   EXfilter();
FUNC_EXTERN VOID    EXdump( u_i4  error, u_i4  *stack);


# endif /* EX_included */
