/*
** Copyright (c) 2004 Ingres Corporation
*/
# ifndef REPEVENT_H_INCLUDED
# define REPEVENT_H_INCLUDED
# include <compat.h>
# include "conn.h"

/**
** Name:	repevent.h - Replicator dbevents
**
** Description:
**	Defines, globals and prototypes for Replicator dbevents.
**
** History:
**	16-dec-96 (joea)
**		Created based on repevent.sh in replicator library.
**	03-feb-98 (joea)
**		Rename RSdb_event_timeout to RSdbevent_timeout.
**	21-apr-98 (joea)
**		Add defines for monitor dbevents. Rename some functions.
**	29-jun-98 (abbjo03)
**		Correct defines for monitor dbevents.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

# define RS_OUT_INSERT		1
# define RS_OUT_UPDATE		2
# define RS_OUT_DELETE		3
# define RS_OUT_TRANS		4
# define RS_INC_INSERT		5
# define RS_INC_UPDATE		6
# define RS_INC_DELETE		7
# define RS_INC_TRANS		8


/* how long to wait for a dbevent */

GLOBALREF i4	RSdbevent_timeout;

/* send monitor DB Events */

GLOBALREF bool	RSmonitor;


FUNC_EXTERN void RSping(bool override);
FUNC_EXTERN STATUS RSdbevents_register(void);
FUNC_EXTERN STATUS RSdbevent_get(void);
FUNC_EXTERN STATUS RSdbevent_get_nowait(void);
FUNC_EXTERN void RSmonitor_notify(RS_CONN *conn, i4  mon_type, i2 db_no,
	char *table_name, char *table_owner);

# endif
