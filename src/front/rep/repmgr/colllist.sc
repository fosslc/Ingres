/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <st.h>
# include <me.h>
# include <er.h>
# include <gl.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>
# include <generr.h>
# include <rpu.h>
# include "collisn.h"
# include "errm.h"

EXEC SQL INCLUDE sqlda;
EXEC SQL INCLUDE <tbldef.sh>;
/*
# include <tbldef.h>
*/

/*
** Name:	colllist.sc - build list of collisions
**
** Description:
**	Defines
**		get_collisions		- build list of collisions
**		check_insert		- check for insert collisions
**		check_update_delete	- check for update/delete collisions
**		init_sessions		- init db sessions for targets
**		close_sessions		- close sessions
**
** History:
**	09-jan-97 (joea)
**		Created based on rslvmann.sc in replicator library.
**	20-jan-97 (joea)
**		Add msg_func parameter to create_key_clauses.
**	22-jun-97 (joea)
**		Discontinue using the ingrep role.
**	27-May-98 (kinte01)
**		Add include of me.h as the code references MEreqmem. In
**		me.h IIMEreqmem is defined as MEreqmem and it is the
**		function IIMEreqmem that is provided in the VMS compatlib
**		shared library.
**	01-oct-98 (abbjo03)
**		Replace ddba_messageit with IIUGerr. Make cursor select from
**		distribution queue consistent with RepServer's select.
**	30-apr-99 (abbjo03)
**		Change Dist_queue_empty message to error.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

EXEC SQL BEGIN DECLARE SECTION;

typedef struct
{
	bool	connected;			/* flag for open session */
	i2	database_no;			/* database number */
	char	vnode_name[DB_MAXNAME+1];	/* vnode */
	char	database_name[DB_MAXNAME+1];	/* database name */
	char	dba[DB_MAXNAME+1];		/* dba name */
	char	full_name[DB_MAXNAME*2+3];	/* full connection name */
} DBSESSION;

EXEC SQL END DECLARE SECTION;

typedef void MSG_FUNC(i4 msg_level, i4  msg_num, ...);


GLOBALDEF COLLISION_FOUND collision_list[1000];
GLOBALDEF i4	unresolved_collisions;

GLOBALREF TBLDEF	*RMtbl;


static i4	NumCollisions;
EXEC SQL BEGIN DECLARE SECTION;
static DBSESSION	*Session;
static i4		NumSessions;
static i4		initial_session;
EXEC SQL END DECLARE SECTION;


static void check_insert(i4 *collision_count, i4  localdb, i2 targetdb,
	i2 sourcedb, i4 transaction_id, i4 sequence_no, i4 table_no,
	char *local_owner);
static void check_update_delete(i4 trans_type, i4  *collision_count,
	i4 localdb, i2 targetdb, i2 old_sourcedb, i4 old_transaction_id,
	i4 old_sequence_no, i2 sourcedb, i4 transaction_id, i4 sequence_no);
STATUS init_sessions(char *localdbname, char *vnode_name, i4  localdb);
STATUS close_sessions(void);


FUNC_EXTERN void ddba_messageit(i4 msg_level, i4  msg_num, ...);
FUNC_EXTERN STATUS create_key_clauses(char *where_clause, char *name_clause,
	char *insert_value_clause, char *update_clause, char *table_name,
	char *table_owner, i4 table_no, char *sha_name, i2 database_no,
	i4 transaction_id, i4 sequence_no, MSG_FUNC *msg_func);


/*{
** Name:	get_collisions	- build list of collisions
**
** Description:
**	Populate the collision_list[] array with any queue collisions found by
**	analyzing the local dd_distrib_queue.
** Inputs:
**	localdbname	- char string containing local dbname
**	vnode_name	- char string containing local vnode
**	localdb		- local database number
**	local_owner	- local database owner
**
** Outputs:
**	collision_count	- integer count of # of found collisions
**	records		- integer count of # of distribution queue records
**			  inspected.
*/
STATUS
get_collisions(
char	*localdbname,
char	*vnode_name,
i2	localdb,
char	*local_owner,
i4	*collision_count,
i4	*records)
# if 0
EXEC SQL BEGIN DECLARE SECTION;
char	*localdbname;
char	*vnode_name;
i2	localdb;
EXEC SQL END DECLARE SECTION;
# endif
{
	DBEC_ERR_INFO errinfo;
	EXEC SQL BEGIN DECLARE SECTION;
	STATUS	err;
	i2	targetdb;
	i2	sourcedb;
	i4	transaction_id;
	i4	sequence_no;
	i2	trans_type;
	char	trans_time[26];
	i2	cdds_no;
	i4	table_no = 0;
	i2	old_sourcedb;
	i4	old_transaction_id = 0;
	i4	old_sequence_no = 0;
	i4	done;
	EXEC SQL END DECLARE SECTION;
	IISQLDA	_sqlda;
	IISQLDA	*sqlda = &_sqlda;

	EXEC SQL INQUIRE_SQL (:initial_session = SESSION);
	if (err = init_sessions(localdbname, vnode_name, localdb))
	{
		EXEC SQL SET_SQL (SESSION = :initial_session);
		EXEC SQL COMMIT;
		if (err == E_RM0123_Dist_queue_empty)
			return (OK);	/* no entries to process */
		else
			return (err);
	}

	EXEC SQL SET_SQL (SESSION = :initial_session);

	*collision_count = 0;
	*records = 0;

	EXEC SQL DECLARE c3 CURSOR FOR
		SELECT	sourcedb, transaction_id, sequence_no,
			table_no, old_sourcedb,  old_transaction_id,
			old_sequence_no, trans_time, trans_type,
			q.cdds_no, targetdb
		FROM	dd_distrib_queue q, dd_db_cdds c, dd_databases d
		WHERE	q.cdds_no = c.cdds_no
		AND	targetdb = c.database_no
		AND	c.database_no = d.database_no
		AND	c.target_type in (1, 2)
		AND	LOWERCASE(d.vnode_name) != 'mobile'
		ORDER	BY trans_time, targetdb, sourcedb, transaction_id,
			cdds_no, sequence_no;

	EXEC SQL OPEN c3 FOR READONLY;
	if (RPdb_error_check(0, &errinfo))
	{
		EXEC SQL ROLLBACK;
		IIUGerr(E_RM0116_Err_open_queue_curs, UG_ERR_ERROR, 0);
		EXEC SQL SET_SQL (SESSION = :initial_session);
		return (E_RM0116_Err_open_queue_curs);
	}

	done = FALSE;
	while (!done)
	{
		EXEC SQL SET_SQL (SESSION = :initial_session);
		EXEC SQL FETCH c3
			INTO	:sourcedb, :transaction_id, :sequence_no,
				:table_no, :old_sourcedb, :old_transaction_id,
				:old_sequence_no, :trans_time, :trans_type,
				:cdds_no, :targetdb;
		EXEC SQL INQUIRE_SQL (:done = ENDQUERY);
		if (RPdb_error_check(DBEC_ZERO_ROWS_OK, &errinfo))
		{
			EXEC SQL SET_SQL (SESSION = :initial_session);
			EXEC SQL CLOSE c3;
			EXEC SQL ROLLBACK;
			IIUGerr(E_RM0117_Err_fetch_queue_curs, UG_ERR_ERROR, 0);
			EXEC SQL SET_SQL (SESSION = :initial_session);
			return (E_RM0117_Err_fetch_queue_curs);
		}

		if (!done)
		{
			*records += 1;

			err = RMtbl_fetch(table_no, TRUE);
			if (err)
			{
				EXEC SQL SET_SQL (SESSION = :initial_session);
				EXEC SQL CLOSE c3;
				EXEC SQL ROLLBACK;
				return (err);
			}

			switch(trans_type)
			{
			case TX_INSERT:
				check_insert(collision_count, localdb, targetdb,
					sourcedb, transaction_id,
					sequence_no, table_no, local_owner);
				break;

			case TX_UPDATE:
			case TX_DELETE:
				check_update_delete((i4)trans_type,
					collision_count, localdb, targetdb,
					old_sourcedb, old_transaction_id,
					old_sequence_no, sourcedb,
					transaction_id, sequence_no);
				break;
			}
		}
	}

	EXEC SQL SET_SQL (SESSION = :initial_session);
	EXEC SQL CLOSE c3;
	EXEC SQL COMMIT;

	/* Initialize count of unresolved collisions */

	unresolved_collisions = *collision_count;
	NumCollisions = *collision_count;
	if (NumCollisions == 0)
	{
		if (close_sessions() != 0)
			return (-1);
	}

	return (OK);
}


/*{
** Name:	check_insert - check for insert collisions
**
** Description:
**	check for an insert collision & report it if found.
**
** Inputs:
**	localdb		-
**	target		-
**	database_no	-
**	transaction_id	-
**	sequence_no	-
**	table_no	-
**
** Outputs:
**	collision_count	-
**
** Returns:
**	none
*/
static void
check_insert(
i4	*collision_count,
i4	localdb,
i2	targetdb,
i2	sourcedb,
i4	transaction_id,
i4	sequence_no,
i4	table_no,
char	*local_owner)
# if 0
EXEC SQL BEGIN DECLARE SECTION;
i2	targetdb;
i2	sourcedb;
i4	transaction_id;
i4	sequence_no;
EXEC SQL END DECLARE SECTION;
# endif
{
	DBEC_ERR_INFO errinfo;
	EXEC SQL BEGIN DECLARE SECTION;
	STATUS	err;
	i4	cnt;
	char	select_statement[1024];
	char	where_clause[1024];
	EXEC SQL END DECLARE SECTION;
	i4	i;
	char	*target_owner;

	EXEC SQL SET_SQL (SESSION = :initial_session);

	err = create_key_clauses(where_clause, NULL, NULL, NULL,
		RMtbl->table_name, RMtbl->table_owner, RMtbl->table_no,
		RMtbl->sha_name, sourcedb, transaction_id, sequence_no,
		ddba_messageit);
	if (err)
	{
		EXEC SQL SET_SQL (SESSION = :initial_session);
		return;
	}

	if (STequal(RMtbl->table_owner, local_owner))
	{
		for (i = 1; i < NumSessions; ++i)
			if (Session[i].database_no == targetdb)
				break;
		target_owner = Session[i].dba;
	}
	else
	{
		target_owner = RMtbl->dlm_table_owner;
	}
	/* is the row to be inserted already in the user table on the target */
	STprintf(select_statement,
		ERx("SELECT COUNT(*) FROM %s.%s t WHERE %s"),
		target_owner, RMtbl->dlm_table_name, where_clause);

	EXEC SQL SET_SQL (SESSION = :targetdb);

	EXEC SQL EXECUTE IMMEDIATE :select_statement INTO :cnt;
	if (RPdb_error_check(DBEC_SINGLE_ROW, &errinfo))
	{
		EXEC SQL SET_SQL (SESSION = :initial_session);
		IIUGerr(E_RM0118_Err_ck_targ_row, UG_ERR_ERROR, 0);
		return;
	}
	if (cnt)	/* collision */
	{
		collision_list[*collision_count].local_db = (i2)localdb;
		collision_list[*collision_count].remote_db = targetdb;
		collision_list[*collision_count].type = TX_INSERT;
		collision_list[*collision_count].resolved = FALSE;
		collision_list[*collision_count].db1.sourcedb = sourcedb;
		collision_list[*collision_count].db1.transaction_id =
			transaction_id;
		collision_list[*collision_count].db1.sequence_no = sequence_no;
		collision_list[*collision_count].db1.table_no =
			RMtbl->table_no;
		sourcedb = (i2)0;
		transaction_id = sequence_no = 0;

		STprintf(select_statement,
			ERx("SELECT sourcedb, transaction_id, sequence_no \
FROM %s t WHERE in_archive = 0 AND %s"),
			RMtbl->sha_name, where_clause);

		EXEC SQL SET_SQL (SESSION = :targetdb);

		EXEC SQL EXECUTE IMMEDIATE :select_statement
			INTO	:sourcedb, :transaction_id, :sequence_no;
		if (RPdb_error_check(DBEC_ZERO_ROWS_OK, &errinfo))
		{
			EXEC SQL SET_SQL (SESSION = :initial_session);
			IIUGerr(E_RM0119_Err_rtrv_shad_row, UG_ERR_ERROR, 0);
			return;
		}
		collision_list[*collision_count].db2.sourcedb = sourcedb;
		collision_list[*collision_count].db2.transaction_id =
			transaction_id;
		collision_list[*collision_count].db2.sequence_no = sequence_no;
		collision_list[*collision_count].db2.table_no = RMtbl->table_no;

		*collision_count += 1;
	}

	EXEC SQL SET_SQL (SESSION = :initial_session);
}


/*{
** Name:	check_update_delete - check for update/delete collisions
**
** Description:
**	check for an update or delete collision & report it if found.
**
** Inputs:
**	trans_type	-
**	localdb		-
**	targetdb	-
**	old_sourcedb	-
**	old_transaction_id -
**	old_sequence_no	-
**	database_no	-
**	transaction_id	-
**	sequence_no	-
**
** Outputs:
**	collision_count	-
**
** Returns:
**	none
*/
static void
check_update_delete(
i4	trans_type,
i4	*collision_count,
i4	localdb,
i2	targetdb,
i2	old_sourcedb,
i4	old_transaction_id,
i4	old_sequence_no,
i2	sourcedb,
i4	transaction_id,
i4	sequence_no)
# if 0
EXEC SQL BEGIN DECLARE SECTION;
i2	targetdb;
i2	sourcedb;
i4	transaction_id;
i4	sequence_no;
EXEC SQL END DECLARE SECTION;
# endif
{
	DBEC_ERR_INFO errinfo;
	EXEC SQL BEGIN DECLARE SECTION;
	i4	cnt;
	char	select_statement[1024];
	EXEC SQL END DECLARE SECTION;

	EXEC SQL SET_SQL (SESSION = :targetdb);

	STprintf(select_statement,
		ERx("SELECT COUNT(*) FROM %s WHERE sourcedb= %d AND \
transaction_id = %d AND sequence_no = %d AND in_archive = 0"),
		RMtbl->sha_name, old_sourcedb, old_transaction_id,
		old_sequence_no);

	EXEC SQL EXECUTE IMMEDIATE :select_statement INTO :cnt;
	if (RPdb_error_check(DBEC_ZERO_ROWS_OK, &errinfo))
	{
		EXEC SQL SET_SQL (SESSION = :initial_session);
		IIUGerr(E_RM011A_Err_ck_targ_shad, UG_ERR_ERROR, 0);
		return;
	}
	if (cnt == 0)	/* possible collision */
	{
		/*
		** check new_key flag in the source shadow table.  if
		** it's set, then we don't have a collision.
		*/
		EXEC SQL SET_SQL (SESSION = :initial_session);

		STprintf(select_statement,
			ERx("SELECT COUNT(*) FROM %s WHERE sourcedb = %d AND \
transaction_id = %d AND sequence_no = %d AND new_key = 1"),
			RMtbl->sha_name, old_sourcedb, old_transaction_id,
			old_sequence_no);

		EXEC SQL EXECUTE IMMEDIATE :select_statement INTO :cnt;
		if (RPdb_error_check(DBEC_ZERO_ROWS_OK, &errinfo))
		{
			EXEC SQL SET_SQL (SESSION = :initial_session);
			IIUGerr(E_RM011B_Err_ck_src_shad, UG_ERR_ERROR, 0);
			return;
		}
		if (cnt == 0) /* collision */
		{
			collision_list[*collision_count].local_db =
					(i2)localdb;
			collision_list[*collision_count].remote_db = targetdb;
			collision_list[*collision_count].type = trans_type;
			collision_list[*collision_count].resolved = FALSE;
			collision_list[*collision_count].db1.sourcedb =
					sourcedb;
			collision_list[*collision_count].db1.transaction_id =
					transaction_id;
			collision_list[*collision_count].db1.sequence_no =
					sequence_no;
			collision_list[*collision_count].db1.table_no =
					RMtbl->table_no;
			*collision_count += 1;
		}
	}

	EXEC SQL SET_SQL (SESSION = :initial_session);
}


STATUS
init_sessions(
char	*localdbname,
char	*vnode_name,
i4	localdb)
# if 0
EXEC SQL BEGIN DECLARE SECTION;
char	*localdbname;
char	*vnode_name;
i4	localdb;
EXEC SQL END DECLARE SECTION;
# endif
{
	EXEC SQL BEGIN DECLARE SECTION;
	char	localiidbdb[100];
	char	tmp[128];
	i4	initial_session;
	EXEC SQL END DECLARE SECTION;
	DBEC_ERR_INFO errinfo;
	STATUS	mem_stat;
	i4	i;

	EXEC SQL INQUIRE_SQL (:initial_session = SESSION);

	if (*vnode_name == EOS)
		STcopy(localdbname, tmp);
	else
		STprintf(tmp, ERx("%s::%s"), vnode_name, localdbname);

	EXEC SQL SELECT COUNT(DISTINCT targetdb)
		INTO	:NumSessions
		FROM	dd_distrib_queue;
	if (RPdb_error_check(0, NULL))
	{
		IIUGerr(E_RM011C_Err_cnt_conns, UG_ERR_ERROR, 0);
		return (E_RM011C_Err_cnt_conns);
	} 
	if (NumSessions == 0)
	{
		IIUGerr(E_RM0123_Dist_queue_empty, UG_ERR_ERROR, 0);
		return (E_RM0123_Dist_queue_empty);
	}

	Session = (DBSESSION *)MEreqmem(0, (u_i4)(NumSessions + 1) *
		sizeof(DBSESSION), TRUE, &mem_stat);
	if (Session == NULL)
	{
		IIUGerr(E_RM011D_Err_alloc_sess, UG_ERR_ERROR, 0);
		return (E_RM011D_Err_alloc_sess);
	}

	/* Establish a connection to the local db */
	EXEC SQL SELECT DBMSINFO('dba')
		INTO	:Session[0].dba;
	STcopy(vnode_name, Session[0].vnode_name);
	STcopy(localdbname, Session[0].database_name);
	Session[0].database_no = (i2)localdb;
	if (*vnode_name == EOS)
		STcopy(Session[0].database_name, Session[0].full_name);
	else
		STprintf(Session[0].full_name, ERx("%s::%s"),
			Session[0].vnode_name, Session[0].database_name);

	EXEC SQL CONNECT :Session[0].full_name
		SESSION :Session[0].database_no
		IDENTIFIED BY :Session[0].dba;
	if (RPdb_error_check(0, &errinfo))
	{
		EXEC SQL SET_SQL (SESSION = :initial_session);
		return (-1);
	}
	Session[0].connected = TRUE;

	EXEC SQL SET_SQL (SESSION = :initial_session);

	++NumSessions;

	EXEC SQL DECLARE opendb_csr CURSOR FOR
		SELECT	DISTINCT q.targetdb, d.database_name, d.vnode_name
		FROM	dd_distrib_queue q, dd_databases d
		WHERE	q.targetdb = d.database_no;
	EXEC SQL OPEN opendb_csr FOR READONLY;
	if (RPdb_error_check(0, NULL) != OK)
	{
		IIUGerr(E_RM011E_Err_open_sess_curs, UG_ERR_ERROR, 0);
		return (E_RM011E_Err_open_sess_curs);
	} 

	for (i = 1; i < NumSessions; ++i)
	{
		EXEC SQL FETCH opendb_csr
			INTO	:Session[i].database_no,
				:Session[i].database_name,
				:Session[i].vnode_name;
		RPdb_error_check(0, &errinfo);
		STtrmwhite(Session[i].vnode_name);
		STtrmwhite(Session[i].database_name);
		switch (errinfo.errorno)
		{
		case GE_OK:
			if (*Session[i].vnode_name != EOS)
			{
				STprintf(tmp, ERx("%s::%s"),
					Session[i].vnode_name,
					Session[i].database_name);
				STprintf(localiidbdb, ERx("%s::%s"),
					Session[i].vnode_name, ERx("iidbdb"));
			} 
			else
			{
				STcopy(Session[i].database_name, tmp);
				STcopy(ERx("iidbdb"), localiidbdb);
			}
			STcopy(tmp, Session[i].full_name);

			/* Get owner of database */
			EXEC SQL CONNECT :localiidbdb
				SESSION :Session[i].database_no;
			if (RPdb_error_check(0, &errinfo))
			{
				IIUGerr(E_RM0084_Error_connecting, UG_ERR_ERROR,
					1, localiidbdb);
				return (E_RM0084_Error_connecting);
			}
			EXEC SQL SELECT own
				INTO	:Session[i].dba
				FROM	iidatabase
				WHERE	name = :Session[i].database_name;
			if (RPdb_error_check(0, &errinfo))
			{
				EXEC SQL SET_SQL (SESSION = :initial_session);
				return (-1);
			}
			STtrmwhite(Session[i].dba);
			EXEC SQL DISCONNECT SESSION :Session[i].database_no;

			EXEC SQL CONNECT :tmp SESSION :Session[i].database_no
				IDENTIFIED BY :Session[i].dba;
			if (RPdb_error_check(0, &errinfo))
			{
				EXEC SQL SET_SQL (SESSION = :initial_session);
				return (-1);
			}
			Session[i].connected = TRUE;
			EXEC SQL SET_SQL (SESSION = :initial_session);
			break;

		case GE_NO_MORE_DATA:
			goto closecsr;

		default:
			IIUGerr(E_RM011F_Err_fetch_sess_curs, UG_ERR_ERROR, 0);
			return (E_RM011F_Err_fetch_sess_curs);
		}
	}

closecsr:
	EXEC SQL CLOSE opendb_csr;
	EXEC SQL COMMIT;

	EXEC SQL SET_SQL (SESSION = :initial_session);

	return (OK);
}


STATUS
close_sessions()
{
	i4	i;
	EXEC SQL BEGIN DECLARE SECTION;
	i4	curr_session;
	EXEC SQL END DECLARE SECTION;

	if (Session == NULL)
		return (OK);

	EXEC SQL INQUIRE_SQL (:curr_session = SESSION);

	for (i = 0; i < NumSessions; ++i)
	{
		if (Session[i].connected)
		{
			EXEC SQL SET_SQL (SESSION = :Session[i].database_no);
			EXEC SQL DISCONNECT SESSION :Session[i].database_no;
		}
	}
	MEfree((PTR)Session);
	Session = NULL;

	EXEC SQL SET_SQL (SESSION = :curr_session);
	return (OK);
}
