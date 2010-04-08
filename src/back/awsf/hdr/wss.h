/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** History:
**      07-Sep-1998 (fanra01)
**          Corrected case of actsession to match piccolo file. 
**      02-Nov-98 (fanra01)
**          Add remote addr and remote host from HTTP header.
**      19-Nov-1998 (fanra01)
**          Add WSSIgnoreUserCase prototype.
**      28-Apr-2000 (fanra01)
**          Bug 100312
**          Add SSL support.  Modified WSSOpenSession prototype to include
**          scheme string like http; https etc and secure flag to indicate use
**          of a secure port.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
#ifndef WSS_INCLUDED
#define WSS_INCLUDED

#include <actsession.h>

GSTATUS 
WSSInitialize ();

GSTATUS 
WSSPerformCookie (
	char* *cookie, 
	u_i4 userid, 
	char* user);

GSTATUS 
WSSOpenSession (
	u_i4	clientType,
	char *query,
	char *secure,
	char *scheme,
	char *host,
	char *port,
	char *variable,
	char *auth_type,
	char *gateway,
	char *agent,
	char *rmt_addr,
	char *rmt_host,
	USR_PSESSION usr_session,
	ACT_PSESSION *session);

GSTATUS 
WSSCloseSession (
	ACT_PSESSION *session,
	bool force);

GSTATUS 
WSSTerminate ();

GSTATUS 
WSSSetBuffer (
	ACT_PSESSION	session,
	WPS_PBUFFER		buffer,
	char*					page);

GSTATUS
WSSDocAccessibility (
	USR_PSESSION	usr_session,
	u_i4	doc_id,
	i4	right,
	bool	*result);

GSTATUS
WSSUnitAccessibility (
	USR_PSESSION	usr_session,
	u_i4	unit_id,
	i4	right,
	bool	*result);

bool
WSSIgnoreUserCase (void);

#endif
