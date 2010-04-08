/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** History:
**	07-Sep-1998 (fanra01)
**	    Corrected case for usrsession to match piccolo.
**      28-Apr-2000 (fanra01)
**          Bug 100312
**          Add fields for SSL support.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-Oct-2000 (fanra01)
**          Sir 103096
**          Add fields to the ACT_SESSION structure to hold the base and
**          extension portions of the content type.
*/
#ifndef ACT_SESSION_INCLUDED
#define ACT_SESSION_INCLUDED

#include <ddfcom.h>
#include <wpsbuffer.h>
#include <usrsession.h>

typedef struct __ACT_SESSION 
{
	u_i4		clientType;
	u_i4		type;	 /* type of the session:  */
	char		*name;
        u_i4            secure;  /* request was on secure port */
        char            *scheme; /* scheme for the request */
	char		*host;	 /* http host name which has requested the treatment */
	char		*port;	 /* http port */
        char            *mimebase;
        char            *mimeext;
	USR_PSESSION user_session;
	DDFHASHTABLE vars;
	char		 *query;
	u_i2		 mem_tag;

	u_i4		 error_counter;

	struct __ACT_SESSION *previous;
	struct __ACT_SESSION *next;
} ACT_SESSION, *ACT_PSESSION;

GSTATUS 
WSMReleaseActSession(
	ACT_PSESSION	*act_session);

GSTATUS
WSMRequestActSession(
	u_i4			clientType,
	char*			query,
	char*			secure,
	char*			scheme,
	char*			host,
	char*			port,
	USR_PSESSION	user_session,
	ACT_PSESSION	*act_session);

GSTATUS
WSMBrowseActiveSession(
	PTR		*cursor,
	ACT_PSESSION *session);

#endif
