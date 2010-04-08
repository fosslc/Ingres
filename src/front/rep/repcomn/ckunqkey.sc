/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <cm.h>
# include <er.h>
# include <gl.h>
# include <iicommon.h>
# include <rpu.h>

/**
** Name:	ckunqkey.sc - check unique keys
**
** Description:
**	Defines
**		check_unique_keys	- check for unique keys
**		check_indexes		- check for unique indexes
**
** History:
**	30-dec-96 (joea)
**		Created based on ckunqkey.sc in replicator library.
**	20-jan-97 (joea)
**		Add msg_func parameter to check_unique_keys and check_indexes.
**	21-jun-97 (joea)
**		If table doesn't exist, return OK.  This can occur with
**		horizontally partitioned tables.
**	02-oct-97 (joea) bug 86076
**		Do not single-quote delimit table and owner names in
**		check_indexes, since the precompiler does that already.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

typedef void MSG_FUNC(i4 msg_level, i4  msg_num, ...);


static STATUS check_indexes(char *table_name, char *table_owner, i4 table_no,
	MSG_FUNC *msg_func);


/*{
** Name:	check_unique_keys - check for unique keys
**
** Description:
**	Checks to see if keys or indexes force the columns marked as
**	replication keys to be unique.
**
** Inputs:
**	table_name	- the name of the table to be checked
**	table_owner	- the name of the owner
**	table_no	- the table number
**	dbname		- the name of the database in which the table is to be
**			  found
**	msg_func	- function to display messages
**
** Outputs:
**	none
**
** Returns:
**	OK - table has unique keys
**	-1 - no unique keys
**	1523 - INGRES error looking for table
**	1524 - INGRES error looking up columns
**	Other error returns from check_indexes (see below)
*/
STATUS
check_unique_keys(
char	*table_name,
char	*table_owner,
i4	table_no,
char	*dbname,
MSG_FUNC	*msg_func)
# if 0
EXEC SQL BEGIN DECLARE SECTION;
char	*table_name;
char	*table_owner;
i4	table_no;
EXEC SQL END DECLARE SECTION;
# endif
{
	EXEC SQL BEGIN DECLARE SECTION;
	char	unique_rule[2];
	char	table_indexes[2];
	i4	cnt1 = 0;
	i4	cnt2 = 0;
	i4	err = 0;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL REPEATED SELECT unique_rule, table_indexes
		INTO	:unique_rule, :table_indexes
		FROM	iitables
		WHERE	table_name = :table_name
		AND	table_owner = :table_owner;
	EXEC SQL INQUIRE_SQL (:err = ERRORNO, :cnt1 = ROWCOUNT);
	if (err)
	{
		(*msg_func)(1, 1523, err, table_owner, table_name, dbname);
		return (1523);
	}
	else if (cnt1 == 0)
	{
		/* table doesn't exist. Probably horizontal partition */
		return (OK);
	}

	if (CMcmpcase(unique_rule, ERx("U")) == 0)
	{
		EXEC SQL REPEATED SELECT COUNT(*) INTO :cnt1
			FROM	iicolumns
			WHERE	table_name = :table_name
			AND	key_sequence > 0
			AND	table_owner = :table_owner;
		EXEC SQL INQUIRE_SQL (:err = ERRORNO);
		if (err)
		{
			(*msg_func)(1, 1524, err, table_owner, table_name,
				dbname);
			return (1524);
		}
		EXEC SQL REPEATED SELECT COUNT(*) INTO :cnt2
			FROM	iicolumns i, dd_regist_columns d
			WHERE	i.column_name = d.column_name
			AND	i.table_name = :table_name
			AND	i.table_owner = :table_owner
			AND	d.table_no = :table_no
			AND	i.key_sequence > 0
			AND	d.key_sequence > 0;
		EXEC SQL INQUIRE_SQL (:err = ERRORNO);
		if (err)
		{
			(*msg_func)(1, 1524, err, table_owner, table_name,
				dbname);
			return (1524);
		}
		if (cnt1 == cnt2)	/* base table makes keys unique */
			return (OK);
	}
	if (CMcmpcase(table_indexes, ERx("Y")) == 0)
		return (check_indexes(table_name, table_owner, table_no,
			msg_func));
	return (-1);
}


/*{
** Name:	check_indexes - check for unique indexes
**
** Description:
**	Checks to see if indexes force the columns marked as
**	replication keys to be unique.
**
** Inputs:
**	table_name	- the name of the table to be checked
**	table_owner	- the name of the owner
**	table_no	- the table number
**	msg_func	- function to display messages
**
** Outputs:
**	none
**
** Returns:
**	OK - the table has a unique index
**	1527 - Cursor open error
**	1525 - Cursor fetch error
**	-1 - table does not have unique indexes
**/
static STATUS
check_indexes(
char	*table_name,
char	*table_owner,
i4	table_no,
MSG_FUNC	*msg_func)
# if 0
EXEC SQL BEGIN DECLARE SECTION;
char	*table_name;
char	*table_owner;
i4	table_no;
EXEC SQL END DECLARE SECTION;
# endif
{
	EXEC SQL BEGIN DECLARE SECTION;
	char	index_name[DB_MAXNAME+1];
	char	index_owner[DB_MAXNAME+1];
	i4	cnt1 = 0;
	i4	cnt2 = 0;
	i4	err = 0;
	i4	last = 0;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL DECLARE c3 CURSOR FOR
		SELECT	index_name, index_owner
		FROM	iiindexes
		WHERE	base_name = :table_name
		AND	base_owner = :table_owner
		AND	unique_rule = 'U';
	EXEC SQL OPEN c3 FOR READONLY;
	EXEC SQL INQUIRE_SQL (:err = ERRORNO);
	if (err)
	{
		/* open cursor failed */
		(*msg_func)(1, 1527, err);
		return (1527);
	}
	while (!last)
	{
		EXEC SQL FETCH c3 INTO :index_name, :index_owner;
		EXEC SQL INQUIRE_SQL (:err = ERRORNO, :last = ENDQUERY);
		if (err)
		{
			EXEC SQL CLOSE c3;
			/* Fetch of index names for table '%s.%s' failed */
			(*msg_func)(1, 1525, table_owner, table_name, err);
			return (1525);
		}
		if (!last)
		{
			EXEC SQL REPEATED SELECT COUNT(*) INTO :cnt1
				FROM	iiindex_columns
				WHERE	index_name = :index_name
				AND	index_owner = :index_owner;
			EXEC SQL REPEATED SELECT COUNT(*) INTO :cnt2
				FROM	iiindex_columns i, dd_regist_columns d
				WHERE	i.index_name = :index_name
				AND	i.index_owner = :index_owner
				AND	i.column_name = d.column_name
				AND	d.table_no = :table_no
				AND	d.key_sequence > 0;
			/*
			** this index is unique and only uses key replicator
			** columns
			*/
			if (cnt1 == cnt2)
			{
				EXEC SQL CLOSE c3;
				return (OK);
			}
		}
	}
	EXEC SQL CLOSE c3;
	return (-1);	/* no unique indexes */
}
