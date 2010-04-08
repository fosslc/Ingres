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
**	28-Oct-1992 (daveb)
**		moved/adapted to VMS
**	13-Jul-1995 (dougb)
**		Added CS_lastquant (was quantum_ticks, though not in here).
**		Corrected name in CS_xchng_thread prototype.  Note:  This
**		CL does not include nor use CS_event_shutdown(),
**		CS_find_events(), CS_parse_option(), CS_set_server_connect(),
**		CS_tas(), CS_mkframe(), CS_swuser(), CSsigchld(),
**		iiCLintrp(), CSsigterm(), or CSslave_handler().  Left in to
**		reduce differences between CLs.
**		Made change applied a while ago to Unix version of this file:
**	    08-aug-93 (swm)
**		Altered comment to say that a sid is a CS_SID, not a nat.
**	01-dec-2000	(kinte01)
**		Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**		from VMS CL as the use is no longer allowed
**	04-Oct-2000 (jenjo02)
**		Moved prototypes of CS_find_scb(), CSdump_statistics()
**		to cs.h.
**	03-dec-2008 (joea)
**	    Add CSdiag_server_link.
**      04-Nov-2009 (horda03) Bug 122849
**              Corrected CS_lastquant name, added reference for CS_sched_quantum
**              array. Added CS_NO_SCHED.
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

/* cshl.c */

GLOBALREF i4	      CS_lastquant;
GLOBALREF i4          CS_sched_quantum [];
#define CS_NO_SCHED -1    /* No previous COMP threads for this PRIORITY */

/* func externs -- should be prototyped later */

FUNC_EXTERN CS_SCB *CS_xchng_thread();
FUNC_EXTERN STATUS CS_admin_task();
FUNC_EXTERN STATUS CS_event_shutdown();
FUNC_EXTERN STATUS CS_find_events();
FUNC_EXTERN STATUS CS_parse_option();
FUNC_EXTERN STATUS CS_set_server_connect();
FUNC_EXTERN STATUS CS_setup();
FUNC_EXTERN STATUS CS_tas();
FUNC_EXTERN STATUS cs_handler();
FUNC_EXTERN VOID CSdiag_server_link(i4 type, VOID (*output)(),
				    VOID (*error)(), PTR filename);
FUNC_EXTERN VOID CS_dump_stack();
FUNC_EXTERN VOID CS_eradicate();
FUNC_EXTERN VOID CS_fmt_scb();
FUNC_EXTERN VOID CS_mkframe();
FUNC_EXTERN VOID CS_mo_init(void);
FUNC_EXTERN VOID CS_move_async();
FUNC_EXTERN VOID CS_swuser();
FUNC_EXTERN VOID CS_toq_scan();
FUNC_EXTERN VOID CSsigchld();
FUNC_EXTERN VOID iiCLintrp();
FUNC_EXTERN i4 CS_quantum();
FUNC_EXTERN i4 CSsigterm();
FUNC_EXTERN i4 CSslave_handler();

