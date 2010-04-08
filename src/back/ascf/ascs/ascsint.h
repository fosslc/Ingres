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
**	12-june-1996 (lewda02)
**	    Stripped from Ingres for Jasmine.
**      12-Feb-99 (fanra01)
**          Renamed functions to ascs versions for Ingres ICE.
**/

/* these are shared between scsmo.c and scs_iformat in scssvc.c */

FUNC_EXTERN MO_GET_METHOD ascs_a_user_get;
FUNC_EXTERN MO_GET_METHOD ascs_a_host_get;
FUNC_EXTERN MO_GET_METHOD ascs_a_pid_get;
FUNC_EXTERN MO_GET_METHOD ascs_a_tty_get;
FUNC_EXTERN MO_GET_METHOD ascs_a_conn_get;
