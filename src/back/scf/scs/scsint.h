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
**/

/* these are shared between scsmo.c and scs_iformat in scssvc.c */

FUNC_EXTERN MO_GET_METHOD scs_a_user_get;
FUNC_EXTERN MO_GET_METHOD scs_a_host_get;
FUNC_EXTERN MO_GET_METHOD scs_a_pid_get;
FUNC_EXTERN MO_GET_METHOD scs_a_tty_get;
FUNC_EXTERN MO_GET_METHOD scs_a_conn_get;


