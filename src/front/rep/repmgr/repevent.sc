/*
** Copyright (c) 2004 Ingres Corporation
*/
EXEC SQL INCLUDE sqlca;		/* to silence DROP EVENT not found errors */

# include <compat.h>
# include <st.h>
# include <er.h>
# include <generr.h>
# include <rpu.h>

/**
** Name:	repevent.sc - Replicator events
**
** Description:
**	Defines
**		create_events	- Drop DBevents
**		drop_events	- Drop DBevents
**
** History:
**	16-dec-96 (joea)
**		Created based on repevent.sc in replicator library.
**	22-oct-97 (joea) bug 86614
**		Adjust server loops to start at 1, since server 0 (distserv) is
**		no longer used.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

# define DIM(a)			(sizeof(a) / sizeof(a[0]))
# define MAX_SERVERS		10


static char	*general_events[] =
{
	ERx("dd_set_server"),	/* set a server startup flag */
	ERx("dd_server"),	/* general status event */
	ERx("dd_go_server"),	/* process the replication */
	ERx("dd_ping"),		/* monitor ping of the server */
	ERx("dd_stop_server"),	/* stop all servers */
	ERx("dd_distribute"),	/* activity alert into distr_queue */
	ERx("dd_transaction"),	/* outcoming transaction event */
	ERx("dd_insert"),	/* outcoming insert event */
	ERx("dd_update"),	/* outcoming update event */
	ERx("dd_delete"),	/* outcoming delete event */
	ERx("dd_transaction2"),	/* incoming transaction event */
	ERx("dd_insert2"),	/* incoming insert event */
	ERx("dd_update2"),	/* incoming update event */
	ERx("dd_delete2"),	/* incoming delete event */
};

static char	*per_server_events[] =
{
	ERx("dd_set_server"),	/* set a server startup flag */
	ERx("dd_server"),	/* status event from each server */
	ERx("dd_go_server"),	/* process the replication */
	ERx("dd_stop_server"),	/* stop a server */
};


/*{
** Name:	create_events - Create Replicator DBevents
**
** Description:
**	Creates the Replicator database events.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	OK on success, FAIL if there is a database error.
*/
STATUS
create_events()
{
	i4	i, j;
	EXEC SQL BEGIN DECLARE SECTION;
	char	stmt[128];
	EXEC SQL END DECLARE SECTION;
	DBEC_ERR_INFO	errinfo;

	if (drop_events() != OK)
		return (FAIL);

	for (i = 0; i < DIM(general_events); ++i)
	{
		STcopy(ERx("CREATE DBEVENT "), stmt);
		STcat(stmt, general_events[i]);
		EXEC SQL EXECUTE IMMEDIATE :stmt;
		RPdb_error_check(0, &errinfo);
		if (errinfo.errorno)
			return (FAIL);
	}

	for (i = 0; i < DIM(per_server_events); ++i)
	{
		for (j = 1; j <= MAX_SERVERS; ++j)
		{
			STprintf(stmt, ERx("CREATE DBEVENT %s%d"),
				per_server_events[i], j);
			EXEC SQL EXECUTE IMMEDIATE :stmt;
			RPdb_error_check(0, &errinfo);
			if (errinfo.errorno)
				return (FAIL);
		}
	}

	return (OK);
}


/*{
** Name:	drop_events - Drop Replicator DBevents
**
** Description:
**	Drops the Replicator database events.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	OK on success, FAIL otherwise
*/
STATUS
drop_events()
{
	i4	i, j;
	EXEC SQL BEGIN DECLARE SECTION;
	char	stmt[128];
	EXEC SQL END DECLARE SECTION;
	DBEC_ERR_INFO	errinfo;

	for (i = 0; i < DIM(general_events); ++i)
	{
		STcopy(ERx("DROP DBEVENT "), stmt);
		STcat(stmt, general_events[i]);
		EXEC SQL EXECUTE IMMEDIATE :stmt;
		RPdb_error_check(0, &errinfo);
		if (errinfo.errorno && errinfo.errorno != GE_NOT_FOUND)
			return (FAIL);
	}

	for (i = 0; i < DIM(per_server_events); ++i)
	{
		for (j = 1; j <= MAX_SERVERS; ++j)
		{
			STprintf(stmt, ERx("DROP DBEVENT %s%d"),
				per_server_events[i], j);
			EXEC SQL EXECUTE IMMEDIATE :stmt;
			RPdb_error_check(0, &errinfo);
			if (errinfo.errorno && errinfo.errorno != GE_NOT_FOUND)
				return (FAIL);
		}
	}

	return (OK);
}
