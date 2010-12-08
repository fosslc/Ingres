/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
 ** Name:	scsint.h    - internal header for scs
**
** Description:
**	header for things known only within scs.
**
** History:
**      24-Feb-1994 (daveb)
**          created
**	03-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Moved MO prototypes here from scsdata.c
**/

/* these are shared between scsmo.c and scs_iformat in scssvc.c */

FUNC_EXTERN MO_GET_METHOD scs_a_user_get;
FUNC_EXTERN MO_GET_METHOD scs_a_host_get;
FUNC_EXTERN MO_GET_METHOD scs_a_pid_get;
FUNC_EXTERN MO_GET_METHOD scs_a_tty_get;
FUNC_EXTERN MO_GET_METHOD scs_a_conn_get;

/* These are shared between scsmo.c and scsdata.c */
FUNC_EXTERN MO_GET_METHOD scs_self_get;
FUNC_EXTERN MO_GET_METHOD scs_pid_get;
FUNC_EXTERN MO_GET_METHOD scs_dblockmode_get;
FUNC_EXTERN MO_GET_METHOD scs_facility_name_get;
FUNC_EXTERN MO_GET_METHOD scs_act_detail_get;
FUNC_EXTERN MO_GET_METHOD scs_activity_get;
FUNC_EXTERN MO_GET_METHOD scs_description_get;
FUNC_EXTERN MO_GET_METHOD scs_query_get;
FUNC_EXTERN MO_GET_METHOD scs_lquery_get;
FUNC_EXTERN MO_GET_METHOD scs_assoc_get;

FUNC_EXTERN MO_SET_METHOD scs_remove_sess_set;
FUNC_EXTERN MO_SET_METHOD scs_crash_sess_set;


