/*
** Copyright (c) 2004 Ingres Corporation
*/

#ifndef DIASYNC_H_INCLUDED
#define DIASYNC_H_INCLUDED

/**
** Name: diasync.h - defines to be used with ASYNC_IO
**
** Description:
**      This file contains the definitions of datatypes used with 
**      asynchronous I/O.
**
** History:
**	07-oct-1994 (anb)
**		Created
**	21-sep-1995 (itb)
**	    Moved AIO_LISTIO_MAX to here from diasync.c.
**	20-oct-1995 (itb)
**	    Added a state flag for a thread's AIO list.
**  07-Apr-1997 (bonro01)
**      Include Dummy aio.h structure definition for platforms
**      that don't have aio.h
**	24-sep-1997 (hanch04)
**	    Some OS have aio.h but not xCL_ASYNC_IO
**	11-Nov-1997 (hanch04)
**	    Added aiocb64_t for LARGEFILE64
**	02-feb-1998/03-oct-1997 (popri01)
**	    Add Unixware (usl_us5) to "Some OS" with aio.h, but not ASYNC.
**	06-Mar-1998/07-Oct-1997 (kosma01)
**	    The structure on AIX that is equivelent to the given
**	    aiocb_t is still quite different. First its name: liocb.
**	    second most of the aiocb_t variables are set in a 
**	    substructure called aiocb. Set many defines to hide 
**	    these differences and reduce the number of ifdefs in
**	    diasync.c and diforce.c
**      06-mar-1998 (hweho01)
**          Avoid redefining structure "sigevent" for dgi_us5;
**          it is defined in system include file "signal.h". 
**	27-Mar-1998 (muhpa01)
**	    For hpb_us5 typedef aiocb to aiocb_t
**	09-apr-1998 (fucch01)
**	    Excluded notifyinfo, sigval, & sigevent declarations
**	    for sgi, as they were already declared.
**	20-Apr-1998 (merja01)
**		Add axp_osf to the list of platforms to include aio.h 
**		without async_io.
**   28-Jul-1998 (linke01)
**       include <aio.h> for sui_us5.
**      06-aug-1998 (hweho01)
**          Avoid redefining structure "sigevent" for dg8_us5;
**          it is defined in system include file "signal.h".
**      08-May-1999 (allmi01)
**          Excluded notifyinfo, sigval, sigevent as they are already
**          defined on rmx_us5.
**      03-jul-99 (podni01)
**          Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**      10-Jan-2000 (hanal04) Bug 99965 INGSRV1090.
**              Correct rux_us5 changes that were applied using the rmx_us5
**              label.
**	29-Feb-2000 (toumi01)
**	    Added support for int_lnx (OS threaded build).
**	29-mar-2000 (somsa01)
**	    For hpb_us5, typedef aiocb64 to aiocb64_t if LARGEFILE64 is
**	    defined.
**      31-mar-2000 (hweho01)
**          Added supports for AIX 64-bit (ris_u64) platform:
**          1) Avoid redefining AIO_LISTIO_MAX,  
**          2) Included aio.h file. 
**          3) typedef liocb64 to aiocb64_t.
**	03-May-2000 (ahaal01)
**	    Avoid redefining AIO_LISTIO_MAX for AIX (rs4_us5) platform.
**	15-aug-2000 (somsa01)
**	    Added ibm_lnx as equivalent to int_lnx.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	03-Nov-2000 (hweho01)
**	    Avoid redefining AIO_LISTIO_MAX for axp_osf platform.
**      
**      14-Dec 2000 (wansh01)
**          Added supports for sgi_us5                        
**          Included aio.h file. 
**          typedef liocb64 to aiocb64_t.
**	25-Jan-2001 (ahaal01)
**	    For AIX (rs4_us5), typedef liocb64 to aiocb64_t if LARGEFILE64
**	    is defined.
**	23-jul-2001 (stephenb)
**	    Add support for i64_aix
**	04-Dec-2001 (hanje04)
**	    Added support for IA64 Linux (i64_lnx)
**	08-Oct-2002 (hanje04)
**          As part of AMD x86-64 Linux port, replace individual Linux
**          defines with generic LNX define where apropriate.
**	23-Sep-2003 (hanje04)
**	    Include aio.h for Linux. Temporary fix for building under 
**	    glibc 2.2. Will investigate ASYNC_IO or 2.65
**	10-Dec-2004 (bonro01)
**	    sigval & sigevent are already defined in a64_sol
**      18-Apr-2005 (hanje04)
**          Add support for Max OS X (mac_osx)).
**          Based on initial changes by utlia01 and monda07.
**	    Replace individual linux defines with LNX.
**	08-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with generic OSX
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**	12-Nov-2010 (kschendel) SIR 124685
**	    Multi-inclusion protection.
**	23-Nov-2010 (kschendel)
**	    Drop obsolete ports.
**/

#ifdef xCL_ASYNC_IO
#include <aio.h>
#else
#if defined(sparc_sol) || defined(usl_us5) || defined(any_aix) || \
  defined(axp_osf)  || \
  defined (sgi_us5) || defined(LNX) || defined(a64_sol)
#include <aio.h>
#else
#if !defined(OSX)
union      notifyinfo {
        int             nisigno;        /* signal number */
        void            (*nifunc)(union sigval);
};
union    sigval
    {
    int         sival_int;
    void *      sival_ptr;
    };
struct    sigevent
    {
    int                 sigev_notify;
    union notifyinfo    sigev_notifyinfo;
    union sigval        sigev_value;
    };
#endif

struct     aiocb
        {
        int             aio_fildes;
        void            *aio_buf;
        size_t          aio_nbytes;
        off_t           aio_offset;
        int             aio_reqprio;
        struct sigevent aio_sigevent;
        int             aio_lio_opcode;
        ssize_t         aio_return;
        int             aio_errno;
        int             aio_flags;
        int             aio_pad[2];
        };

#define LIO_READ                        ((int) 1)
#define LIO_WRITE                       ((int) 2)
#define LIO_NOWAIT                      ((int) 0)
#define sigev_signo     sigev_notifyinfo.nisigno
typedef struct aiocb aiocb_t;

#endif /* sol aix lnx etc */
#endif /* xCL_ASYNC_IO */


/*
**  Defines of other constants.
*/

#if defined(any_aix)
typedef struct liocb64  aiocb64_t;
#endif

# if defined(_ICLAIO_H) || defined(thr_hpux) || defined(LNX)
typedef struct aiocb aiocb_t;
#ifdef LARGEFILE64
typedef struct aiocb64 aiocb64_t;
#endif /* LARGEFILE64 */
#endif /* _ICLAIO_H */

#if defined(any_aix)
typedef struct liocb aiocb_t;
#define aio_fildes lio_fildes
#define aio_lio_opcode lio_opcode
#define aio_offset lio_aiocb.aio_offset
#define aio_buf lio_aiocb.aio_buf
#define aio_retval lio_aiocb.aio_return
#define aio_errno lio_aiocb.aio_errno
#define aio_nbytes lio_aiocb.aio_nbytes
#define aio_reqprio lio_aiocb.aio_reqprio
#define aio_sigevent lio_aiocb.aio_event
#define aio_flag lio_aiocb.aio_flag
#endif /* aix */

#if defined(AIO_LISTIO_MAX) && (defined(any_aix) || defined(axp_osf) )
#undef AIO_LISTIO_MAX
#endif

# define AIO_LISTIO_MAX 10

struct _DI_AIOCB {
	struct _DI_AIOCB *next;		/* Ptr to next DI_AIOCB */
	struct _DI_AIOCB *prev;		/* Ptr to previous DI_AIOCB */
	CS_SID		 sid;           /* Thread ID */
        bool             resumed;       /* Indicates if thread marked resumed */
#ifdef LARGEFILE64
        aiocb64_t          aio;           /* Asynchronous I/O control Block */
#else /* LARGEFILE64 */
        aiocb_t          aio;           /* Asynchronous I/O control Block */
#endif /* LARGEFILE64 */
        DI_OP            diop;	        /* DI_OP */
        STATUS           (*evcomp)();   /* caller Completion function ptr */
        PTR              data;          /* ptr to completion data of caller */
# ifdef OS_THREADS_USED
	CS_SEMAPHORE	aio_sem;	/* access from multiple threads */
# endif
};
typedef struct _DI_AIOCB DI_AIOCB;

struct _DI_LISTIOCB {
        struct _DI_LISTIOCB *next;          /* Ptr to next DI_LISTIOCB */
        struct _DI_LISTIOCB *prev;          /* Ptr to previous DI_LISTIOCB */
	CS_SID		    sid;            /* Thread ID */
	i4		    listio_entries; /* Number of ops in listio list 
                                               for this thread */
	i4		    outstanding_aio;/* Number of ops not yet 
                                               completed */
	i4		    state;	    /* State of the list with
					       respect to gather write */
#define			AIOLIST_INACTIVE    0x00000 /* not used */
#define			AIOLIST_ACTIVE	    0x00001 /* gatherwrite in progress*/
#define			AIOLIST_FORCE	    0x00002 /* gatherwrite forcing */
        DI_AIOCB            *list;          /* ptr to this threads listio ops */
#ifdef LARGEFILE64
	aiocb64_t		    *gather[AIO_LISTIO_MAX];
#else /* LARGEFILE64 */
	aiocb_t		    *gather[AIO_LISTIO_MAX];
#endif /* LARGEFILE64 */
					    /* list of IO's for lio_listio */
# ifdef OS_THREADS_USED
	CS_SEMAPHORE        lio_sem;	/* access from multiple threads */
# endif
};
typedef struct _DI_LISTIOCB DI_LISTIOCB;

#endif /* DIASYNC_H_INCLUDED */
