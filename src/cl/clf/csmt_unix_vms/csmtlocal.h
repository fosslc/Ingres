/*
** Copyright (c) 1989, 2008 Ingres Corporation
*/

/**
** Name: cslocal.h - defines completely private to CS.
**
** Description:
**      This file contains the definitions of datatypes and constants
**      used exclusively within the CS module.
**
** History:
**	27-jun-89 (daveb)
**		Created
**	08-aug-93 (swm)
**		Altered comment to say that a sid is a CS_SID, not a nat.
**	20-apr-94 (mikem)
**		Added more definitions of cs local routines, to solve 
**		prototype warnings on solaris.
**	21-Apr-1994 (daveb)
**	    Made the CS_relspin FUNC_EXTERN added by mikem above
**	    conditional if CS_relspin isnot defined. Otherwise wouldn't
**	    compile on HP.
**	04-Mar-1996 (prida01)
**	    Add CSdiagDumpSched for shared library support.
**	13-Mch-1996 (prida01)
**	    Change CSdiagDumpSched to CSdiag_server_link.
**	03-jun-1996 (canor01)
**	    Change prototype for CS_setup for operating system thread
**	    support.
**      24-jul-1996 (canor01)
**          integrate changes from cs.
**	28-jan-1997 (wonst02)
**	    Add CS_tls_ routines for the SCB.
**      18-feb-1997 (hanch04)
**          As part of merging of Ingres-threaded and OS-threaded servers,
**          rename file to csmtlocal.c and moved all calls to CSMT...
**	26-feb-1997 (canor01)
**	    If system TLS functions can be used, do not replace them
**	    with the CS_tls functions.
**	21-jul-1997 (canor01)
**	    Remove definition of CS_RESERVED_FDS.  It is defined in
**	    csinternal.h.
**	16-Nov-1998 (jenjo02)
**	    Changed CS_checktime() to return ms time instead of VOID.
**	03-Dec-1999 (vande02)
**	    Added the datatype of i4 and the arguments to CSMTdump_statistics
**	    to match the function definition in csmtinterf.c.
**	04-Apr-2000 (jenjo02)
**	    Function prototype for CSMT_tls_measure type of "tid" should
**	    be CS_THREAD_ID, not CS_SID.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-Oct-2000 (jenjo02)
**	    Moved function prototypes of CSMT_find_scb(),
**	    CSMTdump_statistics() to cs.h.
**	31-Oct-2002 (hanje04)
**	    If CS_tas is defined else where don't declare it here.
**	13-Feb-2007 (kschendel)
**	    Define the ticker task entry point.
**	4-Oct-2007 (bonro01)
**	    Add prototype for CS_dump_stack() for Solaris
**	02-dec-2008 (joea)
**	    On VMS, TMnow should serve the same purpose as TMet.
**	09-Feb-2009 (smeke01) b119586
**	    Added function CSMT_get_thread_stats().
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**	21-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

/*
**  Defines of other constants.
*/

/* csinterface.c */

GLOBALREF CS_SYSTEM	      Cs_srv_block;
GLOBALREF CS_SCB	      Cs_queue_hdrs[CS_LIM_PRIORITY];
GLOBALREF CS_SCB	      Cs_known_list_hdr;
GLOBALREF CS_SCB	      Cs_to_list_hdr;
GLOBALREF CS_SCB	      Cs_wt_list_hdr;
GLOBALREF CS_SCB	      Cs_as_list_hdr;
GLOBALREF CS_STK_CB	      Cs_stk_list_hdr;
GLOBALREF CS_SCB	      Cs_idle_scb;
GLOBALREF CS_ADMIN_SCB	      Cs_admin_scb;
GLOBALREF i4		      CSdebug;

/* cshl.c */

GLOBALREF i4	      Cs_lastquant;
GLOBALREF i4		      Cs_incomp;

/* func externs -- should be ptorotyped later */

FUNC_EXTERN CS_SCB *CS_xchg_thread();
FUNC_EXTERN STATUS CSMT_admin_task();
FUNC_EXTERN void CS_event_shutdown(void);
FUNC_EXTERN STATUS CS_find_events();
FUNC_EXTERN STATUS CS_parse_option();
FUNC_EXTERN STATUS CSMT_set_server_connect();
FUNC_EXTERN VOID CSMT_setup();
#ifndef CS_tas
FUNC_EXTERN STATUS CS_tas();
#endif
FUNC_EXTERN STATUS csmt_handler();
FUNC_EXTERN VOID CSMT_dump_stack();
FUNC_EXTERN VOID CSdiag_server_link();
FUNC_EXTERN VOID CS_eradicate();
FUNC_EXTERN VOID CSMT_fmt_scb();
FUNC_EXTERN VOID CS_mkframe();
FUNC_EXTERN VOID CS_mo_init(void);
FUNC_EXTERN VOID CS_move_async();
FUNC_EXTERN VOID CSMT_swuser();
FUNC_EXTERN VOID CS_toq_scan();
FUNC_EXTERN VOID CSMTsigchld();
FUNC_EXTERN VOID iiCLintrp();
FUNC_EXTERN i4  CS_quantum();
FUNC_EXTERN i4  CSMTsigterm();
FUNC_EXTERN i4  CSslave_handler();
# ifndef CS_relspin
FUNC_EXTERN VOID CS_relspin();
# endif
FUNC_EXTERN void CSMT_ticker(void *);

/* defined in cshl.c: */
FUNC_EXTERN VOID CS_change_priority( 
	CS_SCB      *scb, 
	i4         new_priority);
FUNC_EXTERN STATUS CSMT_breakpoint(void);
FUNC_EXTERN VOID CSMT_update_cpu(
	i4      *pageflts,
	i4 *cpumeasure);

FUNC_EXTERN STATUS CSMT_get_thread_stats(CS_THREAD_STATS_CB *pTSCB);

/* defined in csclock.c */
FUNC_EXTERN void 	CS_clockinit();
FUNC_EXTERN i4 	CS_checktime(void);
FUNC_EXTERN i4 	CS_realtime_update_smclock(void);
GLOBALREF   i4 	Cs_enable_clock;

# ifndef xCL_094_TLS_EXISTS /* no system TLS functions to use */
/* defined in csmttls.c */
FUNC_EXTERN VOID CSMT_tls_init( u_i4 max_sessions, STATUS *status );
FUNC_EXTERN VOID CSMT_tls_set( PTR value, STATUS *status );
FUNC_EXTERN VOID CSMT_tls_get( PTR *value, STATUS *status );
FUNC_EXTERN VOID CSMT_tls_destroy( STATUS *status );
FUNC_EXTERN VOID CSMT_tls_measure( CS_THREAD_ID tid, u_i4 *numtlsslots, 
		   u_i4 *numtlsprobes, i4 *numtlsreads, i4 *numtlsdirty,
		   i4 *numtlswrites );
# endif /* ! xCL_094_TLS_EXISTS */

#if defined(sparc_sol) || defined(a64_sol)
FUNC_EXTERN VOID CS_dump_stack( CS_SCB *scb, ucontext_t *ucp, VOID (*output_fcn)(), i4 verbose );
#else
FUNC_EXTERN VOID CS_dump_stack();
#endif

#ifdef VMS
#define TMet	TMnow
#endif
