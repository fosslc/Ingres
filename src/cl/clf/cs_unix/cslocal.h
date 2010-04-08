/*
**Copyright (c) 2004 Ingres Corporation
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
**	21-jul-1997 (canor01)
**	    Remove definition of CS_RESERVED_FDS.  It is defined in
**	    csinternal.h.
**	16-Nov-1998 (jenjo02)
**	    Changed CS_checktime() to return ms time instead of
**	    VOID.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-Oct-2000 (jenjo02)
**	    Moved prototypes of CS_find_scb(), CSdump_statistics()
**	    to cs.h.
**	31-Oct-2002 (hanje04)
**	    If CS_tas is defined elsewhere don't define it here.
**	4-Oct-2007 (bonro01)
**	    Defined CS_dump_stack() for 64bit Solaris stack trace.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**	26-Aug-2009 (kschendel) b121804
**	    Correct CS_event_shutdown type.
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
GLOBALREF CS_SCB	      Cs_repent_scb;
GLOBALREF CS_ADMIN_SCB	      Cs_admin_scb;
GLOBALREF i4		      CSdebug;

/* cshl.c */

GLOBALREF i4	      Cs_lastquant;
GLOBALREF i4		      Cs_incomp;

/* func externs -- should be ptorotyped later */

FUNC_EXTERN CS_SCB *CS_xchg_thread();
FUNC_EXTERN STATUS CS_admin_task();
FUNC_EXTERN void CS_event_shutdown(void);
FUNC_EXTERN STATUS CS_find_events();
FUNC_EXTERN STATUS CS_parse_option();
FUNC_EXTERN STATUS CS_set_server_connect();
FUNC_EXTERN STATUS CS_setup();
#ifndef CS_tas
FUNC_EXTERN STATUS CS_tas();
#endif
FUNC_EXTERN STATUS cs_handler();
FUNC_EXTERN VOID CSdiag_server_link();
FUNC_EXTERN VOID CS_eradicate();
FUNC_EXTERN VOID CS_fmt_scb();
FUNC_EXTERN VOID CS_mkframe();
FUNC_EXTERN VOID CS_mo_init(void);
FUNC_EXTERN VOID CS_move_async();
FUNC_EXTERN VOID CS_swuser();
FUNC_EXTERN VOID CS_toq_scan();
FUNC_EXTERN VOID CSsigchld();
FUNC_EXTERN VOID iiCLintrp();
FUNC_EXTERN i4  CS_quantum();
FUNC_EXTERN i4  CSsigterm();
FUNC_EXTERN i4  CSslave_handler();
# ifndef CS_relspin
FUNC_EXTERN VOID CS_relspin();
# endif

/* defined in cshl.c: */
FUNC_EXTERN VOID CS_change_priority( 
	CS_SCB      *scb, 
	i4         new_priority);
FUNC_EXTERN STATUS CS_breakpoint(void);
FUNC_EXTERN VOID CS_update_cpu(
	i4      *pageflts,
	i4 *cpumeasure);

/* defined in csclock.c */
FUNC_EXTERN void 	CS_clockinit();
FUNC_EXTERN i4 	CS_checktime(void);
FUNC_EXTERN i4 	CS_realtime_update_smclock(void);
GLOBALREF   i4 	Cs_enable_clock;

#if defined(sparc_sol) || defined(a64_sol)
FUNC_EXTERN VOID CS_dump_stack( CS_SCB *scb, ucontext_t *ucp, VOID (*output_fcn)()
, i4 verbose );
#else
FUNC_EXTERN VOID CS_dump_stack();
#endif
