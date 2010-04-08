/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <cm.h>
# include <st.h>
# include <tm.h>
# include <er.h>
# include <ex.h>
# include <me.h>
# include <pc.h>
# include <ci.h>
# include <gl.h>
# include <iicommon.h>
# include <fe.h>
EXEC SQL INCLUDE <ui.sh>;
# include <ug.h>
# include <uigdata.h>
# include <rpu.h>
# include <tblobjs.h>
# include "errm.h"

# include <si.h>

/**
**  Name:	arcclean.sc - archive clean
**
**  Description:
**	Defines
**		main		- archive clean main routine
**		sqlerr_check	- SQL error check
**		clean_tables	- clean the shadow and archive tables
**		delete_rows	- delete the old rows
**		modify_table	- remodify a table
**		build_shadow_stmt - build statement for shadow table
**		error_exit	- exit on error
**
** FIXME:
**	1. needs lock timeout and deadlock handling
**	2. Make use of is_compressed, key_is_compressed and duplicate_rows
**	   columns.
**	3. The modify_table doesn't work for tables owned by a non-DBA.
**
** History:
**	16-dec-96 (joea)
**		Created based on arcclean.sc in replicator library.
**      15-jan-97 (hanch04)
**              Added missing NEEDLIBS
**	25-jun-97 (joea)
**		Add in_archive to shadow table index, but not as part of key.
**	19-nov-97 (joea)
**		Set dba_name as a separate variable and copy IIuiDBA into it
**		since the command line argument still has the "-u" prefix.
**	01-dec-97 (padni01) bug 87279
**		In declaration of tbl_cursor in cleanTables(), select from
**		dd_databases where local_db = 1.
**	27-mar-98 (joea)
**		Discontinue use of RPing_ver_ok.
**	04-jun-98 (abbjo03) bug 91185
**		Check if the shadow index is persistent before attempting to
**		recreate it.
**	15-jun-98 (padni01) bug 89367
**		Add support for SQL 92 installations. Convert to lowercase 
**		before doing comparisons in where clauses.
**	02-nov-99 (gygro01) bug 99359 / pb ingrep 68
**		Arcclean deletes on a transaction basis, which can leave
**		base table entries without any shadow table entry.
**		This is mainly due to not checking the in_archive flag
**		during delete.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-may-02 (padni01) bug 107820
**              Add support for SQL 92 installations. Convert to lowercase
**              (dba_name and sha_idx_name) before doing comparisons in
**              where clauses.
**	11-Dec-2003 (inifa01) BUG 95231 INGREP 51
**		'arcclean' does not work correctly when a database contains
**		multiple CDDS's with different target types eg CDDS 0 with 
**		target type 1 (FULL PEER) and CDDS 1 with target type 2 (PRO)
**		Records from the FULL PEER archive table are not cleared and
**		there is an attempt to modify the non-existent PRO archive  
**		table resulting in error; Could not modify <archive table-name> 
**		- no iitables row found.
**		FIX: loop that stores rows to be deleted in dd_arcclean_tx_id 
**		sets 'cdds_type' to the value of the last row stored. added
**		'target_type' field to dd_arcclean_tx_id so that 'cdds_type'
**		for each row can be set before delete_rows() is called.
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**	29-Sep-2004 (drivi01)
**	    Updated NEEDLIBS to link dynamic library SHQLIB and SHFRAMELIB
**	    to replace its static libraries.
**	10-May-2007 (kibro01) b118062
**	    Update logic to delete rows where they exist in the shadow
**	    table but not in the archive, but also not any more in the
**	    base table, since that is the condition when they have been
**	    deleted - otherwise arcclean never removes such rows.
**	    Also ensure that statement lengths cannot be exceeded.
**	16-Apr-2008 (kibro01) b120264
**	    Re-evaluate the additional "where not exists" clause of the
**	    archive cleaning with each row.
**      06-feb-2009 (stial01)
**          Fix stack buffers dependent on DB_MAXNAME
**/

/*
PROGRAM =	arcclean

NEEDLIBS =      REPMGRLIB REPCOMNLIB SHFRAMELIB SHQLIB SHCOMPATLIB \
		SHEMBEDLIB 

UNDEFS =	II_copyright
*/

# define SHADOW_TABLE		1
# define ARCHIVE_TABLE		2
# define TM_SIZE_STR		27
EXEC SQL BEGIN DECLARE SECTION;
# define PURGE_SESSION		1
EXEC SQL END DECLARE SECTION;

#define BASE_STMT_LEN	4096

static	char	*prog_name = ERx("arcclean");
static	char	time_buf[TM_SIZE_STR];
static	SYSTIME	now;

EXEC SQL BEGIN DECLARE SECTION;
static	i2	cdds_type;

static	char	dba_name[DB_MAXNAME+1];
static	char	*db_name = NULL;
EXEC SQL END DECLARE SECTION;
static	char	*user_name = ERx("");
static	char	*before_time = ERx("");

static	ARG_DESC args[] =
{
	/* required */
	{ERx("database"), DB_CHR_TYPE, FARG_PROMPT, (PTR)&db_name},
	{ERx("before_time"), DB_CHR_TYPE, FARG_PROMPT, (PTR)&before_time},
	/* optional */
	{ERx("user"), DB_CHR_TYPE, FARG_FAIL, (PTR)&user_name},
	NULL
};


static void clean_tables(void);
static void delete_rows(char *tbl_name, char *tbl_owner, i4 tbl_no, i2 db_no,
	i4 tx_id, char *cond_list);
static void modify_table(char *tbl_name, char *sha_idx_name, i4  tbl_type);
static void build_shadow_stmt(char *tbl_name, char *idx_name, char *str);
static i4 sqlerr_check(char *id_string, i4 num_rows_expected);
static void error_exit(void);


FUNC_EXTERN STATUS FEhandler(EX_ARGS *ex);


/*{
** Name:	main - archive clean
**
** Description:
**	remove unwanted rows from the shadow and archibe tables. An unwanted
**	row is one whose transaction date precedes a user-specified parameter
**	and which does not have a corresponding row in the input or
**	distribution queues.
**
** Inputs:
**	argc	- the number of arguments on the command line
**	argv	- the command line arguments
**		argv[1]	- the database to connect to
**		argv[2]	- the database to connect to
**
** Outputs:
**	none
**
** Returns:
**	OK or FAIL
*/
i4
main(
i4	argc,
char	*argv[])
{
	EX_CONTEXT	context;

	/* Tell EX this is an INGRES tool */
	(void)EXsetclient(EX_INGRES_TOOL);

	/* Use the INGRES memory allocator */
	(void)MEadvise(ME_INGRES_ALLOC);

	/* Initialize character set attribute table */
	if (IIUGinit() != OK)
		PCexit(FAIL);

	/* Parse the command line arguments */
	if (IIUGuapParse(argc, argv, prog_name, args) != OK)
		PCexit(FAIL);

	/* Set up the generic exception handler */
	if (EXdeclare(FEhandler, &context) != OK)
	{
		EXdelete();
		PCexit(FAIL);
	}

	/* Open the database - normal UI_SESSION */
	if (FEningres(NULL, (i4)0, db_name, user_name, NULL) != OK)
	{
		EXdelete();
		PCexit(FAIL);
	}
	EXEC SQL SET AUTOCOMMIT OFF;
	EXEC SQL SET_SQL (ERRORTYPE = 'genericerror');

	if (!IIuiIsDBA)
	{
		EXdelete();
		IIUGerr(E_RM0002_Must_be_DBA, UG_ERR_FATAL, 0);
	}

	if (!RMcheck_cat_level())
	{
		EXdelete();
		IIUGerr(E_RM0003_No_rep_catalogs, UG_ERR_FATAL, 0);
	}

	/*
	** connect two sessions - one to build the temp table and one to
	** do the deletion.
	*/
	STcopy(IIuiDBA, dba_name);
	EXEC SQL CONNECT :db_name SESSION :PURGE_SESSION
		IDENTIFIED BY :dba_name;
	if (RPdb_error_check(0, NULL) != OK)
	{
		SIprintf("%s: Cannot connect to database %s (session 2)\n",
			prog_name, db_name);
		SIflush(stdout);
		error_exit();
	}
	EXEC SQL SET LOCKMODE SESSION WHERE TIMEOUT = 0;

	EXEC SQL SET_SQL (SESSION = :UI_SESSION);
	TMnow(&now);
	TMstr(&now, time_buf);
	SIprintf("\n%s starting on '%s' at %s\n", prog_name, db_name,
		time_buf);
	SIflush(stdout);

	clean_tables();
	FEing_exit();

	TMnow(&now);
	TMstr(&now, time_buf);
	SIprintf("%s completed on %s at %s\n", prog_name, db_name,
		time_buf);
	SIflush(stdout);

	EXdelete();
	PCexit(OK);
}


/*{
**	sqlerr_check
**
**	check for an INGRES error - and the expectd number of rows:
**	-1 indicates that the caller does not care about the number
**	of rows. On an error - disconnect and exit.
*/
static i4
sqlerr_check(
char	*id_string,
i4	num_rows_expected)
{
	DBEC_ERR_INFO	errinfo;

	(void)RPdb_error_check(0, &errinfo);
	if (errinfo.errorno)
	{
		SIprintf("\n%s: DBMS error on %s\n", prog_name, id_string);
		SIflush(stdout);
		error_exit();
	}

	switch (num_rows_expected)
	{
	case -1:			/* don't care */
		break;

	case 0:
		if (errinfo.rowcount == 0)
			return;
		break;

	default:
		if (num_rows_expected != errinfo.rowcount)
		{
			SIprintf("%s: %s expected %d rows, but found %d\n",
				prog_name, id_string, num_rows_expected,
				errinfo.rowcount);
			SIflush(stdout);
			error_exit();
		}
		break;
	}
	return ((i4)errinfo.rowcount);
}


/*{
**	evaluate_condition
**
**	construct and return a condition string for the archive clean
**	Added (kibro01) b120264
*/
static char *
evaluate_condition(char *tbl_name, i4 tbl_no)
# if 0
EXEC SQL BEGIN DECLARE SECTION;
i4	tbl_no;
EXEC SQL END DECLARE SECTION;
# endif
{
	EXEC SQL BEGIN DECLARE SECTION;
	static char *cond_list = NULL;
	static i4 cond_list_len = 0;
	char	*idx_list;
	i4	idx_list_len;
	char	column_name[DB_MAXNAME+1];
	EXEC SQL END DECLARE SECTION;
	char	*txt_and = ERx(" AND ");
	char	*txt_sdot = ERx("s.");
	char	*txt_tdot = ERx(" = t.");
	char	*txt;
	i4	len;
	static i4 last_tbl_no = -1;

	/* Same as last time - cond_list will be the same */
	if (last_tbl_no == tbl_no)
		return (cond_list);

	last_tbl_no = tbl_no;

	/* Get a list of key column names so that we can do a
	** match between shadow and base table to identify rows
	** which exist in the shadow but not any longer in the 
	** base table.  (kibro01) b118062
	*/
	EXEC SQL DECLARE key_cursor CURSOR FOR
		SELECT	TRIM(column_name)
		FROM	dd_regist_columns
		WHERE	table_no = :tbl_no
		AND	key_sequence != 0
		ORDER BY key_sequence;
	EXEC SQL OPEN key_cursor FOR READONLY;
	idx_list_len = BASE_STMT_LEN;
	idx_list = MEreqmem(0, idx_list_len, TRUE, NULL);
	if (idx_list == NULL)
	{
	    SIprintf("Cannot allocate memory for archive cleaning\n");
	    SIflush(stdout);
	    error_exit();
	}

	STprintf(idx_list,"");
	while (TRUE)
	{
	    EXEC SQL FETCH key_cursor
	    INTO	:column_name;
	    if (sqlerr_check("key_cursor", -1) == 0)
		break;
	    len = STlength(idx_list);
	    if (len)
		STcat(idx_list,txt_and);
	    if (len + STlength(txt_sdot) + STlength(txt_tdot) +
		STlength(column_name)*2 + 1 > idx_list_len)
	    {
		char *new_idx_list;
		idx_list_len += BASE_STMT_LEN;
		new_idx_list = MEreqmem(0, idx_list_len, TRUE, NULL);
		if (new_idx_list == NULL)
		{
		    SIprintf("Cannot allocate memory for arcclean(2)\n");
		    SIflush(stdout);
		    error_exit();
		}
		STcopy(idx_list, new_idx_list);
		MEfree(idx_list);
		idx_list = new_idx_list;
	    }
	    STcat(idx_list,txt_sdot);
	    STcat(idx_list,column_name);
	    STcat(idx_list,txt_tdot);
	    STcat(idx_list,column_name);
	}
	EXEC SQL CLOSE key_cursor;

	/* Now build up the extra segment for the WHERE clause
	** include a check for shadow table entries where no matching
	** record exists in the base table, checked by...
	** NOT EXISTS(SELECT * FROM base t WHERE s.key1=t.key1 AND...)
	** using all the unique key items (kibro01) b118062
	*/
	txt = ERx(" AND (in_archive = 1 OR " \
		"(in_archive = 0 AND new_key = 1) OR " \
		"NOT EXISTS (SELECT * FROM %s t WHERE %s))");
	len = STlength(idx_list) + STlength(txt) + DB_MAXNAME + 1;
	if (len > cond_list_len)
	{
		if (cond_list)
			MEfree(cond_list);
		cond_list = MEreqmem(0, len, TRUE, NULL);
		cond_list_len = len;
	}
	if (cond_list == NULL)
	{
		SIprintf("Cannot allocate memory for archive cleaning(3)\n");
		SIflush(stdout);
		error_exit();
	}
	STprintf(cond_list, txt, tbl_name, idx_list);
	MEfree(idx_list);
	return (cond_list);
}

/*{
** Name:	clean_tables - clean the shadow and archive tables
**
** Description:
**	For each table in the dd_regist_tables list, read from the
**	corresponding shadow table all rows before the specified date. If a
**	corresponding IQ or DQ row does not exist, delete from the archive and
**	shadow for this database_no, transaction_id pair.
**
**	In addition, do not delete any rows whose transaction ID matches the
**	old transaction ID in either queue.
**
** FIXME:  Check dd_db_cdds and exit early if database doesn't have any
**	   CDDS's with target_type 1 or 2.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	none
*/
static void
clean_tables()
{
	EXEC SQL BEGIN DECLARE SECTION;
	char	tbl_name[DB_MAXNAME+1];
	char	tbl_owner[DB_MAXNAME+1];
	i4	tbl_no;
	i2	db_no;
	i4	tx_id;
	char	*stmt;
	char	*cond_list;
	EXEC SQL END DECLARE SECTION;
	char	shadow_name[DB_MAXNAME+1];
	char	sha_idx_name[DB_MAXNAME+1];
	char	archive_name[DB_MAXNAME+1];
	i4	len;

	EXEC SQL COMMIT;
	EXEC SQL SET LOCKMODE SESSION WHERE READLOCK = NOLOCK;

	/* check to see if the dd_arcclean_tx_id is present */
	EXEC SQL SELECT COUNT(*)
		INTO	:tx_id
		FROM	iitables
		WHERE	LOWERCASE(table_name) = 'dd_arcclean_tx_id'
		AND	LOWERCASE(table_owner) = LOWERCASE(:dba_name);
	sqlerr_check("id table check", 1);

	if (tx_id != 0)
	{
		SIprintf("%s: The temporary table dd_arcclean_tx_id exists in this database\n",
			prog_name);
		SIprintf("either someone else is running this program, or the table must be dropped.\n");
		SIflush(stdout);
		error_exit();
	}

	EXEC SQL CREATE TABLE dd_arcclean_tx_id (
		table_name	VARCHAR(32) NOT NULL,
		table_owner	VARCHAR(32) NOT NULL,
		table_no	INTEGER NOT NULL,
		target_type	INTEGER NOT NULL,
		database_no	SMALLINT NOT NULL,
		transaction_id	INTEGER NOT NULL);
	sqlerr_check("temp table create", 0);
	EXEC SQL COMMIT;

	EXEC SQL DECLARE tbl_cursor CURSOR FOR
		SELECT	TRIM(table_name), TRIM(table_owner), table_no,
			c.target_type
		FROM	dd_regist_tables t, dd_db_cdds c, dd_databases d
		WHERE	d.local_db = 1
		AND	d.database_no = c.database_no
		AND	t.cdds_no = c.cdds_no
		AND	c.target_type IN (1, 2);
	EXEC SQL OPEN tbl_cursor FOR READONLY;
	sqlerr_check("table cursor", 0);
	while (TRUE)
	{
		EXEC SQL FETCH tbl_cursor
			INTO	:tbl_name, :tbl_owner, :tbl_no,
				:cdds_type;
		if (sqlerr_check("table cursor", -1) == 0)
			break;

		cond_list = evaluate_condition(tbl_name, tbl_no);
			
		/*  build a temp table (of known name) */
		SIprintf("Checking '%s.%s' ...\n", tbl_owner, tbl_name);
		SIflush(stdout);
		RPtblobj_name(tbl_name, tbl_no, TBLOBJ_SHA_TBL, shadow_name);

		/* Allocate enough for query below plus cond-list */
		len = 1000 + STlength(cond_list);
		stmt = MEreqmem(0, len, TRUE, NULL);
		if (stmt == NULL)
		{
			SIprintf("Cannot allocate memory for archive cleaning(4)\n");
			SIflush(stdout);
			error_exit();
		}

		STprintf(stmt,
			ERx("INSERT INTO dd_arcclean_tx_id SELECT DISTINCT "));
		STcat(stmt, RPedit_name(EDNM_SLITERAL, tbl_name, NULL));
		STcat(stmt, ERx(", "));
		STcat(stmt, RPedit_name(EDNM_SLITERAL, tbl_owner, NULL));
		STcat(stmt, ERx(", "));
		STprintf(stmt, ERx("%s%d"), stmt, tbl_no);
		STcat(stmt, ERx(", "));
		STprintf(stmt, ERx("%s%d"), stmt, cdds_type);
		STcat(stmt, ERx(", sourcedb, transaction_id FROM "));
		STcat(stmt, shadow_name);
		STcat(stmt, ERx(" s WHERE trans_time <= '"));
		STcat(stmt, before_time);
		STcat(stmt, ERx("'"));
		STcat(stmt, cond_list);

		EXEC SQL EXECUTE IMMEDIATE :stmt;
		sqlerr_check("tmp table insert", -1);
		MEfree(stmt);
	}
	EXEC SQL CLOSE tbl_cursor;
	EXEC SQL COMMIT;

	TMnow(&now);
	TMstr(&now, time_buf);
	SIprintf("Check complete - beginning clean at %s\n", time_buf);
	SIflush(stdout);
	EXEC SQL DECLARE tx_cursor CURSOR FOR
		SELECT	TRIM(table_name), TRIM(table_owner), table_no, 
			target_type, database_no, transaction_id
		FROM	dd_arcclean_tx_id;
	EXEC SQL OPEN tx_cursor;
	sqlerr_check("tx_cursor open", 0);
	while (TRUE)
	{
		EXEC SQL FETCH tx_cursor
			INTO	:tbl_name, :tbl_owner, :tbl_no, :cdds_type,
				:db_no, :tx_id;
		if (sqlerr_check("tx_cursor fetch", -1) == 0)
			break;

		cond_list = evaluate_condition(tbl_name, tbl_no);
		EXEC SQL SET_SQL (SESSION = :PURGE_SESSION);
		delete_rows(tbl_name, tbl_owner, tbl_no, db_no, tx_id, 
			cond_list);
		EXEC SQL SET_SQL (SESSION = :UI_SESSION);
	}
	EXEC SQL SET_SQL (SESSION = :PURGE_SESSION);
	EXEC SQL COMMIT;
	EXEC SQL SET_SQL (SESSION = :UI_SESSION);
	EXEC SQL CLOSE tx_cursor;
	EXEC SQL COMMIT;

	MEfree(cond_list);

	/*
	** now get the table names from dd_regist_tables again and
	** modify the shadow and archive tables
	*/
	EXEC SQL SET_SQL (SESSION = :UI_SESSION);
	EXEC SQL OPEN tbl_cursor FOR READONLY;
	sqlerr_check("table cursor", 0);
	while (TRUE)
	{
		EXEC SQL FETCH tbl_cursor
			INTO	:tbl_name, :tbl_owner, :tbl_no,
				:cdds_type;
		if (sqlerr_check("table cursor", -1) == 0)
			break;

		RPtblobj_name(tbl_name, tbl_no, TBLOBJ_SHA_TBL, shadow_name);
		RPtblobj_name(tbl_name, tbl_no, TBLOBJ_SHA_INDEX1,
			sha_idx_name);
		if (cdds_type == 1) /* arch tbl only exists for full peer */
			RPtblobj_name(tbl_name, tbl_no, TBLOBJ_ARC_TBL,
				archive_name);

		EXEC SQL SET_SQL (SESSION = :PURGE_SESSION);
		modify_table(shadow_name, sha_idx_name, SHADOW_TABLE);
		if (cdds_type == 1) /* arch tbl only exists for full peer */
			modify_table(archive_name, NULL, ARCHIVE_TABLE);
		EXEC SQL SET_SQL (SESSION = :UI_SESSION);
	}
	EXEC SQL CLOSE tbl_cursor;
	EXEC SQL COMMIT;

	EXEC SQL SET_SQL (SESSION = :PURGE_SESSION);
	modify_table(ERx("dd_input_queue"), NULL, FALSE);
	modify_table(ERx("dd_distrib_queue"), NULL, FALSE);
	EXEC SQL COMMIT;
	EXEC SQL DISCONNECT;

	EXEC SQL SET_SQL (SESSION = :UI_SESSION);
	EXEC SQL DROP TABLE dd_arcclean_tx_id;
	sqlerr_check("tx_id_table drop", 0); 
	EXEC SQL COMMIT;
}


/*{
** Name:	delete_rows - delete the old rows
**
** Description:
**	Purge the rows from the shadow and archive table
**
** Inputs:
**	tbl_name	- table name
**	tbl_owner	- table owner
**	tbl_no		- table number
**	db_no		- database number
**	tx_id		- transaction id
**	cond_list	- WHERE clause for key-column matching with base table
**
** Outputs:
**	none
**
** Returns:
**	none
*/
static void
delete_rows(
char	*tbl_name,
char	*tbl_owner,
i4	tbl_no,
i2	db_no,
i4	tx_id,
char	*cond_list)
# if 0
EXEC SQL BEGIN DECLARE SECTION;
char	*tbl_name;
i2	db_no;
i4	tx_id;
EXEC SQL END DECLARE SECTION;
# endif
{
	EXEC SQL BEGIN DECLARE SECTION;
	char	*stmt;
	i4	stmt_len;
	i4	input_rowcount;
	i4	distrib_rowcount;
	EXEC SQL END DECLARE SECTION;
	i4	rows;
	char	shadow_name[DB_MAXNAME+1];
	char	archive_name[DB_MAXNAME+1];
	char	*sql_qry =
	    ERx("DELETE FROM %s s WHERE sourcedb = ? AND transaction_id = ?");

	stmt_len = STlength(sql_qry) + STlength(cond_list) + (DB_MAXNAME * 3) + 1;
	stmt = MEreqmem(0, stmt_len, TRUE, NULL);
	if (stmt == NULL)
		return;

	EXEC SQL REPEATED SELECT COUNT(*)
		INTO	:input_rowcount
		FROM	dd_input_queue
		WHERE	(sourcedb = :db_no
		AND	transaction_id = :tx_id)
		OR	(old_sourcedb = :db_no
		AND	old_transaction_id = :tx_id);
	sqlerr_check("Input Q row count", 1);
	EXEC SQL COMMIT;

	EXEC SQL REPEATED SELECT COUNT(*)
		INTO	:distrib_rowcount
		FROM	dd_distrib_queue
		WHERE	(sourcedb = :db_no
		AND	transaction_id = :tx_id)
		OR	(old_sourcedb = :db_no
		AND	old_transaction_id = :tx_id);
	sqlerr_check("Distribution Q row count", 1);
	EXEC SQL COMMIT;

	SIprintf("Tx [ '%s.%s' %d %d ] .. ", tbl_owner, tbl_name, db_no,
		tx_id);
	SIflush(stdout);
	if (input_rowcount == 0 && distrib_rowcount == 0)
	{
		RPtblobj_name(tbl_name, tbl_no, TBLOBJ_SHA_TBL, shadow_name);
		STprintf(stmt, sql_qry, shadow_name);
		STcat(stmt, cond_list);

		EXEC SQL PREPARE s_delete FROM :stmt;
		sqlerr_check("s prepare", 0);

		EXEC SQL EXECUTE s_delete USING :db_no, :tx_id;
		rows = sqlerr_check("shadow delete", -1);
		SIprintf("shadow (%d %s) ", (i4)rows, rows == 1 ? "row" :
			"rows");
		SIflush(stdout);

		if (cdds_type == 1)	/* full peer */
		{
			RPtblobj_name(tbl_name, tbl_no, TBLOBJ_ARC_TBL,
				archive_name);
			STprintf(stmt, sql_qry, archive_name);
			EXEC SQL PREPARE a_delete FROM :stmt;
			sqlerr_check("a prepare", 0);

			EXEC SQL EXECUTE a_delete USING :db_no, :tx_id;
			rows = sqlerr_check("archive delete", -1);
			SIprintf("archive (%d %s)", (i4)rows, rows == 1 ?
				"row" : "rows");
			SIflush(stdout);
		}

		SIprintf(" deleted\n");
		SIflush(stdout);
	}
	MEfree(stmt);
}


static void
modify_table(
char	*tbl_name,
char	*sha_idx_name,
i4	tbl_type)
# if 0
EXEC SQL BEGIN DECLARE SECTION;
char	*tbl_name;
char	*sha_idx_name;
EXEC SQL END DECLARE SECTION;
# endif
{
	EXEC SQL BEGIN DECLARE SECTION;
	char	col_name[DB_MAXNAME+1];
	i4	key_seq;
	char	sort_direction[5];
	char	storage_structure[10];
	char	is_compressed[5];
	char	key_is_compressed[5];
	char	duplicate_rows[5];
	char	unique_rule[5];
	char	location_name[DB_MAXNAME+1];
	char	multi_locations[5];
	i4	table_ifillpct;
	i4	table_dfillpct;
	i4	table_lfillpct;
	i4	table_minpages;
	i4	table_maxpages;
	char	persistent[2];
	char	stmt[1024 + 2*DB_MAXNAME];
	char	shadow_stmt[1024 + 2*DB_MAXNAME];
	EXEC SQL END DECLARE SECTION;
	char	convbuf[80];
	char	keybuf[1024 + 32*DB_MAXNAME];
	i4	rows;

	/* build the key clause for the modify statement */
	SIprintf("Modifying %s ...\n", tbl_name);
	SIflush(stdout);
	*keybuf = EOS;
	EXEC SQL REPEATED SELECT TRIM(column_name), key_sequence,
			TRIM(sort_direction)
		INTO	:col_name, :key_seq,
			:sort_direction
		FROM	iicolumns
		WHERE	LOWERCASE(table_name) = LOWERCASE(:tbl_name)
		AND	LOWERCASE(table_owner) = LOWERCASE(:dba_name)
		AND	key_sequence != 0
		ORDER	BY key_sequence;
	EXEC SQL BEGIN;
		if (key_seq != 1)
			STcat(keybuf, ERx(", "));
		STcat(keybuf, RPedit_name(EDNM_DELIMIT, col_name, NULL));
	EXEC SQL END;
	rows = sqlerr_check("iicolumns lookup", -1);

	/*
	** determine the location of the tables and re-modify.
	** if this was the shadow table - record details of the
	** secondary index before destroying it.
	*/
	if (tbl_type == SHADOW_TABLE)
	{
		EXEC SQL REPEATED SELECT persistent
			INTO	:persistent
			FROM	iiindexes
			WHERE	LOWERCASE(index_name) = LOWERCASE(:sha_idx_name)
			AND	LOWERCASE(index_owner) = LOWERCASE(:dba_name);
		sqlerr_check("iiindexes persistent", 1);
		if (CMcmpcase(persistent, ERx("Y")))
			build_shadow_stmt(tbl_name, sha_idx_name, shadow_stmt);
	}

	EXEC SQL REPEATED SELECT TRIM(storage_structure), is_compressed,
			key_is_compressed, duplicate_rows, unique_rule,
			TRIM(location_name), multi_locations, table_ifillpct,
			table_dfillpct,	table_lfillpct, table_minpages,
			table_maxpages
		INTO	:storage_structure, :is_compressed,
			:key_is_compressed, :duplicate_rows, :unique_rule,
			:location_name, :multi_locations, :table_ifillpct,
			:table_dfillpct, :table_lfillpct, :table_minpages,
			:table_maxpages
		FROM	iitables
		WHERE	LOWERCASE(table_name) = LOWERCASE(:tbl_name)
		AND	LOWERCASE(table_owner) = LOWERCASE(:dba_name);
	rows = sqlerr_check("iitables lookup", -1);
	if (rows != 1)
	{
		SIprintf("Could not modify %s - no iitables row found.\n",
			tbl_name);
		SIflush(stdout);
		EXEC SQL COMMIT;
		return;
	}
	EXEC SQL COMMIT;

	/* build the modify statement */
	STprintf(stmt, ERx("MODIFY %s TO %s%s ON %s WITH "), tbl_name,
		storage_structure, *unique_rule == 'U' ? ERx(" UNIQUE") :
		ERx(" "), keybuf);

	/*
	** build the with clause
	**	- minpages and maxpages only apply with hash
	**	- ifillpct (indexfill == nonleaffill) with btree
	**	- lfillpct (leaffill) with btree
	**	- dfillpct (datafill) with btree hash and isam
	*/
	STprintf(convbuf, ERx("FILLFACTOR = %d, "), table_dfillpct);
	STcat(stmt, convbuf);
	if (STcompare(storage_structure, ERx("BTREE")) == 0)
	{
		STprintf(convbuf, ERx("NONLEAFFILL = %d, "), table_ifillpct);
		STcat(stmt, convbuf);
		STprintf(convbuf, ERx("LEAFFILL = %d, "), table_lfillpct);
		STcat(stmt, convbuf);
	}
	else if (STcompare(storage_structure, ERx("HASH")) == 0)
	{
		STprintf(convbuf, ERx("MINPAGES = %d, "), table_minpages);
		STcat(stmt, convbuf);
		STprintf(convbuf, ERx("MAXPAGES = %d, "), table_maxpages);
		STcat(stmt, convbuf);
	}
	/*
	** HEAP and ISAM are not considered - HEAP should
	** not be the structure, and ISAM only has a fillfactor
	*/

	/* now the location clause */
	STcat(stmt, ERx(" LOCATION = ("));
	STcat(stmt, location_name);
	if (*multi_locations == 'Y')
	{
		EXEC SQL SELECT TRIM(location_name), loc_sequence
			INTO	:location_name, :key_seq
			FROM	iimulti_locations
			WHERE	LOWERCASE(table_name) = LOWERCASE(:tbl_name)
			AND	LOWERCASE(table_owner) = LOWERCASE(:dba_name)
			ORDER	BY loc_sequence;
		EXEC SQL BEGIN;
			STcat(stmt, ERx(", "));
			STcat(stmt, location_name);
		EXEC SQL END;
		sqlerr_check("iicolumns lookup", -1);
		EXEC SQL COMMIT;
	}
	STcat(stmt, ERx(")"));
	EXEC SQL EXECUTE IMMEDIATE :stmt;
	rows = sqlerr_check("table modify", -1);
	SIprintf(ERx(" (%d %s)\n"), rows, rows == 1 ? "row" : "rows");
	SIflush(stdout);
	EXEC SQL COMMIT;

	if (tbl_type == SHADOW_TABLE && CMcmpcase(persistent, ERx("Y")))
	{
		if (*shadow_stmt != EOS)
		{
			SIprintf("Creating %s ...\n", sha_idx_name);
			SIflush(stdout);
			EXEC SQL EXECUTE IMMEDIATE :shadow_stmt;
			rows = sqlerr_check("index creation", -1);
			SIprintf(ERx(" (%d %s)\n"), rows, rows == 1 ? "row" :
				"rows");
			SIflush(stdout);
			EXEC SQL COMMIT;
		}
		else
		{
			SIprintf("Could not create %s - no iitables row found.\n",
				sha_idx_name);
			SIflush(stdout);
		}
	}
}


static void
build_shadow_stmt(
char	*tbl_name,
char	*idx_name,
char	*str)
# if 0
EXEC SQL BEGIN DECLARE SECTION;
char	*idx_name;
EXEC SQL END DECLARE SECTION;
# endif
{
	EXEC SQL BEGIN DECLARE SECTION;
	char	col_name[DB_MAXNAME+1];
	char	sort_direction[5];
	i4	key_seq;
	char	storage_structure[10];
	char	is_compressed[5];
	char	key_is_compressed[5];
	char	duplicate_rows[5];
	char	unique_rule[5];
	char	location_name[DB_MAXNAME+1];
	char	multi_locations[5];
	i4	table_ifillpct;
	i4	table_dfillpct;
	i4	table_lfillpct;
	i4	table_minpages;
	i4	table_maxpages;
	EXEC SQL END DECLARE SECTION;
	char	convbuf[80];
	char	stmt[1024];
	char	keybuf[1024];
	i4	rows;

	/* build the key clause for the index statement */
	*keybuf = *stmt = EOS;
	EXEC SQL REPEATED SELECT TRIM(column_name), key_sequence,
			TRIM(sort_direction)
		INTO	:col_name, :key_seq,
			:sort_direction
		FROM	iicolumns
		WHERE	LOWERCASE(table_name) = LOWERCASE(:idx_name)
		AND	LOWERCASE(table_owner) = LOWERCASE(:dba_name)
		AND	key_sequence != 0
		AND	LOWERCASE(column_name) != 'tidp'
		ORDER	BY key_sequence;
	EXEC SQL BEGIN;
		if (key_seq != 1)
			STcat(keybuf, ERx(", "));
		STcat(keybuf, RPedit_name(EDNM_DELIMIT, col_name, NULL));
	EXEC SQL END;
	sqlerr_check("iicolumns lookup", -1);

	/* determine the location of the index */
	EXEC SQL REPEATED SELECT TRIM(storage_structure), is_compressed,
			key_is_compressed, duplicate_rows, unique_rule,
			TRIM(location_name), multi_locations, table_ifillpct,
			table_dfillpct, table_lfillpct, table_minpages,
			table_maxpages
		INTO	:storage_structure, :is_compressed,
			:key_is_compressed, :duplicate_rows, :unique_rule,
			:location_name, :multi_locations, :table_ifillpct,
			:table_dfillpct, :table_lfillpct, :table_minpages,
			:table_maxpages
		FROM	iitables
		WHERE	LOWERCASE(table_name) = LOWERCASE(:idx_name)
		AND	LOWERCASE(table_owner) = LOWERCASE(:dba_name);
	rows = sqlerr_check("iitables lookup", -1);
	if (rows != 1)
	{
		EXEC SQL COMMIT;
		STcopy(stmt, str);
		return;
	}

	/* build the create index statement */
	STprintf(stmt,
		ERx("CREATE %sINDEX %s ON %s (%s, in_archive) WITH STRUCTURE \
= %s, KEY = (%s), "),
		*unique_rule == 'U' ? ERx("UNIQUE ") : ERx(""), idx_name,
		tbl_name, keybuf, storage_structure, keybuf);

	/*
	** build the with clause
	**	- minpages and maxpages only apply with hash
	**	- ifillpct (indexfill == nonleaffill) with btree
	**	- lfillpct (leaffill) with btree
	**	- dfillpct (datafill) with btree hash and isam
	*/
	STprintf(convbuf, ERx("FILLFACTOR = %d, "), table_dfillpct);
	STcat(stmt, convbuf);
	if (STcompare(storage_structure, ERx("BTREE")) == 0) {
		STprintf(convbuf, ERx("NONLEAFFILL = %d, "), table_ifillpct);
		STcat(stmt, convbuf);
		STprintf(convbuf, ERx("LEAFFILL = %d, "), table_lfillpct);
		STcat(stmt, convbuf);
	}
	else if (STcompare(storage_structure, ERx("HASH")) == 0)
	{
		STprintf(convbuf, ERx("MINPAGES = %d, "), table_minpages);
		STcat(stmt, convbuf);
		STprintf(convbuf, ERx("MAXPAGES = %d, "), table_maxpages);
		STcat(stmt, convbuf);
	}
	/* ISAM is not considered since it only has a fillfactor */

	/* now the location clause */
	STcat(stmt, ERx(" LOCATION = ("));
	STcat(stmt, location_name);
	if (*multi_locations == 'Y')
	{
		EXEC SQL SELECT TRIM(location_name), loc_sequence
			INTO	:location_name, :key_seq
			FROM	iimulti_locations
			WHERE	LOWERCASE(table_name) = LOWERCASE(:idx_name)
			AND	LOWERCASE(table_owner) = LOWERCASE(:dba_name)
			ORDER	BY loc_sequence;
		EXEC SQL BEGIN;
			STcat(stmt, ERx(", "));
			STcat(stmt, location_name);
		EXEC SQL END;
		sqlerr_check("iicolumns lookup", -1);
		EXEC SQL COMMIT;
	}
	STcat(stmt, ERx(")"));

	/* done building the string.. */
	STcopy(stmt, str);
}


/*
**	disconnect if required and exit, cleaning up the dd_arcclean_tx_id
**	do not check the return from the drop - the table may not
**	exist.
*/
void
error_exit()
{
	EXEC SQL BEGIN DECLARE SECTION;
	i4	curr_session;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL INQUIRE_SQL (:curr_session = SESSION);
	if (curr_session == UI_SESSION)
		EXEC SQL SET_SQL (SESSION = :PURGE_SESSION);

	EXEC SQL ROLLBACK;
	EXEC SQL DISCONNECT;

	EXEC SQL SET_SQL (SESSION = :UI_SESSION);
	EXEC SQL ROLLBACK;
	FEing_exit();
	EXdelete();
	PCexit(FAIL);
}

