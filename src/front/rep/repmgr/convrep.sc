/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <cm.h>
# include <st.h>
# include <si.h>
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
# include <tbldef.h>
# include "errm.h"

/**
** Name:	convrep.sc - convert Replicator data dictionary
**
** Description:
**	Defines
**		main		- convert data dictionary
**		error_exit	- exit on error
**		drop_old_rules	- drops old rules
**		drop_object	- drops a database object
**		save_old_tables	- saves neccessary old tables
**		convert_tables	- converts old tables
**		convert_supp_objs - converts support objects
**		drop_old_dict	- drops old data dictionary
**
** History:
**	09-jan-97 (joea)
**		Created based on convrep.sc in replicator library.
**	14-jan-97 (joea)
**		Remove dd_target_types.
**	22-sep-97 (joea) bug 85701
**		Call RMcheck_cat_level to verify catalog compatibility instead
**		of doing an incomplete query here.  Add capability to drop 1.1
**		rules and local procedures, drop tables discontinued from 1.1
**		to 2.0 and convert 1.1 dbprocs.
**	27-mar-98 (joea)
**		Discontinue use of RPing_ver_ok.
**	16-jun-98 (abbjo03)
**		Add indexes argument to modify_tables. Replace load_dict by
**		load_tables.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**	29-Sep-2004 (drivi01)
**	    Updated NEEDLIBS to link dynamic library SHQLIB and SHFRAMELIB 
**	    to replace its static libraries.
**      06-feb-2009 (stial01)
**          Fix stack buffers dependent on DB_MAXNAME
**/

/*
PROGRAM =	convrep

NEEDLIBS =	REPMGRLIB REPCOMNLIB SHFRAMELIB SHQLIB \
		SHCOMPATLIB SHEMBEDLIB

UNDEFS =	II_copyright
*/

# define DIM(a)		(sizeof(a) / sizeof(a[0]))

# define OBJ_TABLE	1
# define OBJ_DBPROC	2
# define OBJ_RULE	3

# define STMT_SIZE	10000

# define REP1_0		10
# define REP1_1		11

# define TRUNC_TBLNAME_LEN	10

EXEC SQL BEGIN DECLARE SECTION;
# define CAT_LEVEL_SESSION	1
EXEC SQL END DECLARE SECTION;


GLOBALREF TBLDEF *RMtbl;


/* These are the names of the Replicator catalogs in 1.0 releases */
static char *replic_tbls[] =
{
	ERx("dd_installation"),
	ERx("dd_connections"),
	ERx("dd_distribution"),
	ERx("dd_registered_tables"),
	ERx("dd_registered_columns"),
	ERx("dd_distrib_transaction"),
	ERx("dd_distribution_queue"),
	ERx("dd_quiet_installations"),
	ERx("dd_replicated_tables"),
	ERx("dd_target_type"),
	ERx("dd_messages"),		/* discontinued in 1.0/04 */
	ERx("dd_replica_target"),	/* discontinued in 1.0/04 */
	ERx("dd_mobile_opt"),		/* introduced in 1.0/05 */
};

static char *replic_tbls2[] =
{
	ERx("dd_target_types"),		/* discontinued in 2.0 */
	ERx("dd_transaction_id"),	/* discontinued in 2.0 */
};

static char *replic_tbls_same[] =
{
	ERx("dd_input_queue"),
	ERx("dd_mail_alert"),
	ERx("dd_servers"),
	ERx("dd_events"),
	ERx("dd_flag_values"),
	ERx("dd_server_flags")
};

/* REP 1.0 suffixes for rules and dbprocs */
static char *rule_suffixes[] =
{
	ERx("1"), ERx("2"), ERx("3")
};

static char *proc_suffixes[] =
{
	ERx("i"), ERx("u"), ERx("d"),
	ERx("i2"), ERx("u2"), ERx("d2")
};

/* REP 1.1 suffixes for rules and dbprocs */
static char *rule_suffixes11[] =
{
	ERx("ins"), ERx("upd"), ERx("del")
};

static char *proc_suffixes11[] =
{
	ERx("loi"), ERx("lou"), ERx("lod")
};

static char *old_shadow_cols =
	ERx("database_no, transaction_id, sequence_no, trans_time, \
distribution_time, in_archive, dd_routing, dd_priority, trans_type, new_key, \
old_database_no, old_transaction_id, old_sequence_no");

static char *new_shadow_cols =
	ERx("sourcedb, transaction_id, sequence_no, trans_time, \
distribution_time, in_archive, cdds_no, dd_priority, trans_type, new_key, \
old_sourcedb, old_transaction_id, old_sequence_no");

static char *old_archive_cols =
	ERx("database_no, transaction_id, sequence_no");

static char *new_archive_cols =
	ERx("sourcedb, transaction_id, sequence_no");

static struct tagTbl
{
	i4	no;
	char	name[DB_MAXNAME+1];
} *tbls = NULL;
static i4	curr_level = REP1_0;


EXEC SQL BEGIN DECLARE SECTION;
static	i4	NumTables = 0;

static	char	*db_name = NULL;
static	char	*dba_name = ERx("");
EXEC SQL END DECLARE SECTION;

static	ARG_DESC args[] =
{
	/* required */
	{ERx("database"), DB_CHR_TYPE, FARG_PROMPT, (PTR)&db_name},
	/* optional */
	{ERx("user"), DB_CHR_TYPE, FARG_FAIL, (PTR)&dba_name},
	NULL
};


static STATUS drop_old_rules(void);
static STATUS save_old_tables(void);
static STATUS convert_tables(void);
static STATUS convert_supp_objs(void);
static STATUS convert_dbprocs(void);
static STATUS drop_old_dict(void);
static void error_exit(void);
static STATUS drop_object(char *obj_name, i4  obj_type);


FUNC_EXTERN STATUS FEhandler(EX_ARGS *ex);
FUNC_EXTERN STATUS load_tables(void);
FUNC_EXTERN STATUS modify_tables(bool indexes);


/*{
** Name:	main - convert data dictionary
**
** Description:
**	Converts Replicator data dictionary.  Convrep does its job in three
**	independent stages (transactions) to ensure catalogs integrity and
**	recoverability: (1) disable the Change Recorder rules, (2) convert
**	the catalogs, and (3) convert the support objects.  It runs with an
**	exclusive table level lock level.  Convrep exits if current release
**	catalogs already exist.
**
** Inputs:
**	argc	- the number of arguments on the command line
**	argv	- the command line arguments
**		argv[1]	- the database to connect to
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
	EXEC SQL BEGIN DECLARE SECTION;
	i4	exist;
	EXEC SQL END DECLARE SECTION;
	EX_CONTEXT	context;

	/* Tell EX this is an INGRES tool */
	(void)EXsetclient(EX_INGRES_TOOL);

	/* Use the INGRES memory allocator */
	(void)MEadvise(ME_INGRES_ALLOC);

	/* Initialize character set attribute table */
	if (IIUGinit() != OK)
		PCexit(FAIL);

	/* Parse the command line arguments */
	if (IIUGuapParse(argc, argv, ERx("convrep"), args) != OK)
		PCexit(FAIL);

	/* Set up the generic exception handler */
	if (EXdeclare(FEhandler, &context) != OK)
	{
		EXdelete();
		PCexit(FAIL);
	}

	/* Open the database - normal UI_SESSION */
	if (FEningres(NULL, (i4)0, db_name, dba_name, NULL) != OK)
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
	if (*dba_name == EOS)
		dba_name = IIuiDBA;

	SIprintf("Converting Replicator catalogs on database '%s' ...\n",
		db_name);

	if (RMcheck_cat_level())
	{
		EXEC SQL ROLLBACK;
		FEing_exit();
		SIprintf("Replicator catalogs already at the current release level.  --  Exiting.\n");
		EXdelete();
		PCexit(FAIL);
	}
	EXEC SQL COMMIT;

	EXEC SQL SELECT COUNT(*)
		INTO	:exist
		FROM	iitables
		WHERE	LOWERCASE(table_name) = 'dd_regist_tables'
		AND	table_owner = :dba_name;
	if (RPdb_error_check(0, NULL) != OK)
		error_exit();
	if (exist)
		curr_level = REP1_1;

	EXEC SQL COMMIT;
	EXEC SQL SET LOCKMODE SESSION WHERE LEVEL = TABLE,
		READLOCK = EXCLUSIVE;

	SIprintf("\n    Dropping Change Recorder rules ...\n");
	if (drop_old_rules() != OK)
		error_exit();

	if (curr_level == REP1_0)
	{
		SIprintf("    Creating new catalogs ...\n");
		if (save_old_tables() != OK)
			error_exit();
		if (load_tables() != OK)
			error_exit();

		SIprintf("    Converting old catalogs ...\n");
		if (convert_tables() != OK)
			error_exit();
	}

	SIprintf("    Removing old catalogs ...\n");
	if (drop_old_dict() != OK)
		error_exit();

	SIprintf("    Remodifying catalogs ...\n");
	if (modify_tables(TRUE) != OK)
		error_exit();

	EXEC SQL CONNECT :db_name SESSION :CAT_LEVEL_SESSION
		IDENTIFIED BY '$ingres';
	if (RPdb_error_check(0, NULL) != OK)
		error_exit();

	if (update_cat_level() != OK)
		error_exit();

	EXEC SQL SET_SQL (SESSION = :UI_SESSION);
	if (RPdb_error_check(0, NULL) != OK)
		error_exit();
	EXEC SQL COMMIT;
	EXEC SQL SET_SQL (SESSION = :CAT_LEVEL_SESSION);
	if (RPdb_error_check(0, NULL) != OK)
		error_exit();
	EXEC SQL COMMIT;
	SIprintf("    Catalog conversion complete.\n");
	EXEC SQL DISCONNECT;

	EXEC SQL SET_SQL (SESSION = :UI_SESSION);
	if (RPdb_error_check(0, NULL) != OK)
		error_exit();
	SIprintf("\n    Converting support objects ...\n");
	if (curr_level == REP1_0)
	{
		if (convert_supp_objs() != OK)
			error_exit();
	}
	else if (curr_level == REP1_1)
	{
		if (convert_dbprocs() != OK)
			error_exit();
	}
	EXEC SQL COMMIT;
	FEing_exit();

	/* free memory */
	if (tbls != NULL)
	{
		MEfree((PTR)tbls);
		tbls = NULL;
	}

	SIprintf("Database '%s' converted successfully.\n", db_name);

	EXdelete();
	PCexit(OK);
}


/*{
** Name:	error_exit - exit on error
**
** Description:
**	Rollback, disconnect and exit on error.
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
error_exit()
{
	EXEC SQL ROLLBACK;
	EXEC SQL DISCONNECT ALL;
	SIprintf("Error converting database '%s'.\n", db_name);
	PCexit(FAIL);
}


/*{
** Name:	drop_old_rules - drops REP 1.0 or 1.1 rules
**
** Description:
**	Disable the change recorder by dropping old rules.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	OK or FAIL
*/
static STATUS
drop_old_rules()
{
	char	obj_name[DB_MAXNAME+1];
	i4	i;
	i4	j;
	EXEC SQL BEGIN DECLARE SECTION;
	i4	table_no;
	char	table_name[DB_MAXNAME+1];
	char	stmt[1024];
	EXEC SQL END DECLARE SECTION;
	char	*rt_10 = ERx("dd_registered_tables");
	char	*rt_11 = ERx("dd_regist_tables");

	STprintf(stmt, ERx("SELECT COUNT(*) FROM %s"),
		curr_level == REP1_0 ? rt_10 : rt_11);
	EXEC SQL EXECUTE IMMEDIATE :stmt INTO :NumTables;
	if (RPdb_error_check(0, NULL) != OK)
		return (FAIL);

	if (NumTables == 0)
		return (OK);

	/* allocate memory for table names */
	tbls = (struct tagTbl *)MEreqmem(0, (u_i4)(NumTables *
		sizeof(struct tagTbl)), TRUE, NULL);
	if (tbls == NULL)
		return (FAIL);

	/* get table names */
	i = 0;
	STprintf(stmt, ERx("SELECT table_no, TRIM(%s) FROM %s"),
		curr_level == REP1_0 ? ERx("tablename") : ERx("table_name"),
		curr_level == REP1_0 ? rt_10 : rt_11);
	EXEC SQL EXECUTE IMMEDIATE :stmt
		INTO :table_no, :table_name;
	EXEC SQL BEGIN;
		if (i < NumTables)
		{
			tbls[i].no = table_no;
			STcopy(table_name, tbls[i].name);
		}
		++i;
	EXEC SQL END;
	if (RPdb_error_check(0, NULL) != OK)
		return (FAIL);

	for (i = 0; i < NumTables; ++i)
	{
		for (j = 0; j < DIM(rule_suffixes); ++j)
		{
			if (curr_level == REP1_1)
				STprintf(obj_name, ERx("%.*s%0*d%s"),
					TRUNC_TBLNAME_LEN,
					RPedit_name(EDNM_ALPHA, tbls[i].name,
					NULL), TBLOBJ_TBLNO_NDIGITS,
					(i4)tbls[i].no, rule_suffixes11[j]);
			else
				STprintf(obj_name, ERx("%s__%s"), tbls[i].name,
					rule_suffixes[j]);
			if (drop_object(obj_name, OBJ_RULE) != OK)
				return (FAIL);
		}
	}
	if (curr_level == REP1_1)
	{
		EXEC SQL UPDATE dd_regist_tables
			SET	rules_created = '';
		if (RPdb_error_check(0, NULL) != OK)
			return (FAIL);
	}
	EXEC SQL COMMIT;

	return (OK);
}


/*{
** Name:	drop_object - drops a database object
**
** Description:
**	Drops a database object if it exists.
**
** Inputs:
**	obj_name - name of the object
**	obj_type - type of the object
**
** Outputs:
**	none
**
** Returns:
**	OK or FAIL
*/
static STATUS
drop_object(
char	*obj_name,
i4	obj_type)
# if 0
EXEC SQL BEGIN DECLARE SECTION;
char	*obj_name;
EXEC SQL END DECLARE SECTION;
# endif
{
	EXEC SQL BEGIN DECLARE SECTION;
	i4	exist;
	char	stmt[80 + 2*DB_MAXNAME];
	EXEC SQL END DECLARE SECTION;
	char	*drop_stmt;

	switch (obj_type)
	{
	case OBJ_TABLE:
		EXEC SQL REPEATED SELECT COUNT(*)
			INTO	:exist
			FROM	iitables
			WHERE	table_name = :obj_name
			AND	table_owner = :dba_name;
		drop_stmt = ERx("DROP TABLE %s");
		break;

	case OBJ_RULE:
		EXEC SQL REPEATED SELECT COUNT(*)
			INTO	:exist
			FROM	iirules
			WHERE	rule_name = :obj_name
			AND	rule_owner = :dba_name;
		drop_stmt = ERx("DROP RULE %s");
		break;

	case OBJ_DBPROC:
		EXEC SQL REPEATED SELECT COUNT(*)
			INTO	:exist
			FROM	iiprocedures
			WHERE	procedure_name = :obj_name
			AND	procedure_owner = :dba_name;
		drop_stmt = ERx("DROP PROCEDURE %s");
		break;

	default:
		return (FAIL);
	}

	if (exist)
	{
		STprintf(stmt, drop_stmt, obj_name);
		EXEC SQL EXECUTE IMMEDIATE :stmt;
		if (RPdb_error_check(0, NULL) != OK)
			return (FAIL);
	}
	return (OK);
}


/*{
** Name:	save_old_tables - saves neccessary old tables
**
** Description:
**	Saves neccessary old tables (ones that have the same names as in
**	this release) to temporary tables.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	OK or FAIL
*/
static STATUS
save_old_tables()
{
	i4	i;
	char	tmp_table[DB_MAXNAME+1];
	EXEC SQL BEGIN DECLARE SECTION;
	char	stmt[80 + 2*DB_MAXNAME];
	char	err_buf[256];
	EXEC SQL END DECLARE SECTION;

	for (i = 0; i < DIM(replic_tbls_same); ++i)
	{
		STprintf(tmp_table, ERx("%s_tmp"), replic_tbls_same[i]);
		if (drop_object(tmp_table, OBJ_TABLE) != OK)
			return (FAIL);

		STprintf(stmt, ERx("CREATE TABLE %s AS SELECT * FROM %s"),
			tmp_table, replic_tbls_same[i]);
		EXEC SQL EXECUTE IMMEDIATE :stmt;
		if (RPdb_error_check(DBEC_ZERO_ROWS_OK, NULL) != OK)
			return (FAIL);

		if (drop_object(replic_tbls_same[i], OBJ_TABLE) != OK)
			return (FAIL);
	}

	return (OK);
}


/*{
** Name:	convert_tables - converts old tables
**
** Description:
**	Converts old tables.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	OK or FAIL
*/
static STATUS
convert_tables()
{
	DBEC_ERR_INFO	errinfo;
	EXEC SQL BEGIN DECLARE SECTION;
	char	ing_ver[DB_MAXNAME+1];
	i4	rel_pre1003;
	i4	col_count;
	i4	max_table_no;
	char	unique_rule[2];
	char	table_indexes[2];
	short	cdds_null_ind;
	short	prio_null_ind;
	struct
	{
		i4	table_no;
		char	table_name[DB_MAXNAME+1];
		char	table_owner[DB_MAXNAME+1];
		i2	cdds_no;
		char	cdds_lookup_table[DB_MAXNAME+1];
		char	prio_lookup_table[DB_MAXNAME+1];
		char	index_used[DB_MAXNAME+1];
	} reg_tbl;
	EXEC SQL END DECLARE SECTION;

	/* dd_databases */
	EXEC SQL INSERT INTO dd_databases (database_no, database_name,
			vnode_name, dbms_type, local_db, remark)
		SELECT	database_no, database_name,
			node_name, 'ingres', 0, dba_comment
		FROM	dd_connections;
	if (RPdb_error_check(DBEC_ZERO_ROWS_OK, NULL) != OK)
		return (FAIL);

	EXEC SQL UPDATE dd_databases d
		FROM	dd_installation i
		SET	local_db = 1
		WHERE	d.database_no = i.database_no;
	if (RPdb_error_check(DBEC_SINGLE_ROW, NULL) != OK)
		return (FAIL);

	/* dd_regist_columns & dd_regist_tables */
	reg_tbl.table_no = 1;
	EXEC SQL DECLARE reg_tbl_cursor CURSOR FOR
		SELECT	r.tablename, TRIM(t.table_owner),
			r.dd_routing,
			r.rule_lookup_table,
			r.priority_lookup_table,
			t.unique_rule, t.table_indexes
		FROM	dd_registered_tables r, iitables t
		WHERE	r.tablename = t.table_name;
	EXEC SQL OPEN reg_tbl_cursor FOR READONLY;
	if (RPdb_error_check(0, NULL) != OK)
		return (FAIL);
	while (TRUE)
	{
		EXEC SQL FETCH reg_tbl_cursor
			INTO	:reg_tbl.table_name, :reg_tbl.table_owner,
				:reg_tbl.cdds_no,
				:reg_tbl.cdds_lookup_table:cdds_null_ind,
				:reg_tbl.prio_lookup_table:prio_null_ind,
				:unique_rule, :table_indexes;
		if (RPdb_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != OK)
			return (FAIL);
		if (errinfo.rowcount != 1)
			break;

		if (cdds_null_ind == -1)
			*reg_tbl.cdds_lookup_table = EOS;
		if (prio_null_ind == -1)
			*reg_tbl.prio_lookup_table = EOS;

		EXEC SQL INSERT INTO dd_regist_tables
			VALUES	(:reg_tbl.table_no, :reg_tbl.table_name,
				:reg_tbl.table_owner, '',
				'', '', :reg_tbl.cdds_no,
				:reg_tbl.cdds_lookup_table,
				:reg_tbl.prio_lookup_table,
				'');
		if (RPdb_error_check(DBEC_ZERO_ROWS_OK, NULL) != OK)
			return (FAIL);

		EXEC SQL INSERT INTO dd_regist_columns
			SELECT	DISTINCT :reg_tbl.table_no, c.column_name,
				c.column_sequence, 0
			FROM	dd_registered_columns r, iicolumns c
			WHERE	r.tablename = :reg_tbl.table_name
			AND	r.tablename = c.table_name
			AND	r.column_name = c.column_name
			AND	c.table_owner = :reg_tbl.table_owner;
		if (RPdb_error_check(DBEC_ZERO_ROWS_OK, NULL) != OK)
			return (FAIL);

		EXEC SQL UPDATE dd_regist_columns c
			FROM	dd_registered_columns r
			SET	column_sequence = 0
			WHERE	r.tablename = :reg_tbl.table_name
			AND	r.column_name = c.column_name
			AND	r.replicated_column != 'R';
		if (RPdb_error_check(DBEC_ZERO_ROWS_OK, NULL) != OK)
			return (FAIL);

		/*
		** Find out which index we're using and update
		** dd_regist_tables and dd_regist_columns accordingly.
		*/
		if (CMcmpcase(unique_rule, ERx("U")) == 0)
		{	/* unique primary key */
			STcopy(reg_tbl.table_name, reg_tbl.index_used);
			EXEC SQL REPEATED UPDATE dd_regist_columns r
				FROM	iicolumns c
				SET	key_sequence = c.key_sequence
				WHERE	c.column_name = r.column_name
				AND	c.key_sequence > 0
				AND	c.table_name = :reg_tbl.table_name
				AND	c.table_owner = :reg_tbl.table_owner
				AND	r.table_no = :reg_tbl.table_no;
			if (RPdb_error_check(0, NULL) != OK)
				return (FAIL);
		}
		else if (CMcmpcase(table_indexes, ERx("Y")) == 0)
		{	/* secondary index */
			EXEC SQL SELECT COUNT(*)
				INTO	:col_count
				FROM	dd_registered_columns
				WHERE	tablename = :reg_tbl.table_name
				AND	key_column = 'K';
			if (RPdb_error_check(0, NULL) != OK)
				return (FAIL);

			EXEC SQL DECLARE index_cursor CURSOR FOR
				SELECT	index_name
				FROM	iiindexes
				WHERE	base_name = :reg_tbl.table_name
				AND	base_owner =:reg_tbl.table_owner
				AND	unique_rule = 'U';
			EXEC SQL OPEN index_cursor FOR READONLY;
			if (RPdb_error_check(0, NULL) != OK)
				return (FAIL);
			while (TRUE)
			{
				EXEC SQL FETCH index_cursor
					INTO :reg_tbl.index_used;
				if (RPdb_error_check(DBEC_ZERO_ROWS_OK,
						&errinfo) != OK)
					return (FAIL);
				if (errinfo.rowcount != 1)
					break;

				EXEC SQL REPEATED UPDATE dd_regist_columns r
					FROM	iiindexes i,
						iiindex_columns c
					SET	key_sequence = c.key_sequence
					WHERE	r.column_name = c.column_name
					AND	r.table_no = :reg_tbl.table_no
					AND	c.index_name = i.index_name
					AND	c.index_owner = i.index_owner
					AND	i.index_name =
							:reg_tbl.index_used;
				if (RPdb_error_check(0, &errinfo) != OK)
					return (FAIL);
				if (errinfo.rowcount == col_count)
					break;
				else
					*reg_tbl.index_used = EOS;
			}
			EXEC SQL CLOSE index_cursor;
			if (RPdb_error_check(0, NULL) != OK)
				return (FAIL);

			if (*reg_tbl.index_used == EOS)
				return (FAIL);
		}
		else	/* no indexes found */
		{
			return (FAIL);
		}

		EXEC SQL UPDATE dd_regist_tables
			SET	columns_registered = DATE('now'),
				index_used = :reg_tbl.index_used
			WHERE	table_no = :reg_tbl.table_no;
		if (RPdb_error_check(DBEC_SINGLE_ROW, NULL) != OK)
			return (FAIL);

		++reg_tbl.table_no;
	}
	EXEC SQL CLOSE reg_tbl_cursor;
	if (RPdb_error_check(0, NULL) != OK)
		return (FAIL);

	/* dd_paths */
	rel_pre1003 = 0;
	EXEC SQL SELECT INT4(1) INTO :rel_pre1003
		FROM	iicolumns
		WHERE	table_name = 'dd_distribution'
		AND	column_name = 'local';
	if (RPdb_error_check(DBEC_ZERO_ROWS_OK, NULL) != OK)
		return (FAIL);

	if (rel_pre1003)
		EXEC SQL INSERT INTO dd_paths (cdds_no, sourcedb, localdb,
				targetdb)
			SELECT	dd_routing, source, local,
				target
			FROM	dd_distribution;
	else
		EXEC SQL INSERT INTO dd_paths (cdds_no, sourcedb, localdb,
				targetdb)
			SELECT	dd_routing, source, localdb,
				target
			FROM	dd_distribution;
	if (RPdb_error_check(DBEC_ZERO_ROWS_OK, NULL) != OK)
		return (FAIL);

	/* dd_cdds */
	/* don't insert the default CDDS (0) since load_dict() does this */
	EXEC SQL INSERT INTO dd_cdds (cdds_no, cdds_name,
			collision_mode, error_mode)
		SELECT	DISTINCT dd_routing, 'CDDS #' + CHAR(dd_routing),
			0, 0
		FROM	dd_distribution
		WHERE	dd_routing != 0;
	if (RPdb_error_check(DBEC_ZERO_ROWS_OK, NULL) != OK)
		return (FAIL);

	/* dd_db_cdds */
	EXEC SQL INSERT INTO dd_db_cdds (cdds_no, database_no, target_type)
		SELECT	DISTINCT dd_routing, source, 1
		FROM	dd_distribution;
	if (RPdb_error_check(DBEC_ZERO_ROWS_OK, NULL) != OK)
		return (FAIL);

	EXEC SQL INSERT INTO dd_db_cdds (cdds_no, database_no, target_type)
		SELECT	dd_routing, target, 1
		FROM	dd_distribution d
		WHERE	NOT EXISTS
			(SELECT	* FROM dd_db_cdds c
			WHERE	d.target = c.database_no
			AND	d.dd_routing = c.cdds_no);
	if (RPdb_error_check(DBEC_ZERO_ROWS_OK, NULL) != OK)
		return (FAIL);

	EXEC SQL UPDATE dd_db_cdds d
		FROM	dd_connections c
		SET	target_type = c.target_type,
			server_no = c.server_role
		WHERE	d.database_no = c.database_no;
	if (RPdb_error_check(DBEC_ZERO_ROWS_OK, NULL) != OK)
		return (FAIL);

	/* dd_mail_alert */
	EXEC SQL INSERT INTO dd_mail_alert (mail_username)
		SELECT	DISTINCT mail_username
		FROM	dd_mail_alert_tmp;
	if (RPdb_error_check(DBEC_ZERO_ROWS_OK, NULL) != OK)
		return (FAIL);

	/* dd_input_queue */
	EXEC SQL INSERT INTO dd_input_queue
		SELECT	database_no, transaction_id, sequence_no, trans_type,
			r.table_no, old_database_no, old_transaction_id,
			old_sequence_no, trans_time, dd_routing, dd_priority
		FROM	dd_input_queue_tmp i, dd_regist_tables r
		WHERE	i.tablename = r.table_name;
	if (RPdb_error_check(DBEC_ZERO_ROWS_OK, NULL) != OK)
		return (FAIL);

	/* dd_distrib_queue */
	EXEC SQL INSERT INTO dd_distrib_queue
		SELECT	target, database_no, transaction_id, sequence_no,
			trans_type, r.table_no, old_database_no,
			old_transaction_id, old_sequence_no, trans_time,
			dd_routing, dd_priority
		FROM	dd_distribution_queue d, dd_regist_tables r
		WHERE	d.tablename = r.table_name;
	if (RPdb_error_check(DBEC_ZERO_ROWS_OK, NULL) != OK)
		return (FAIL);

	/* dd_last_number */
	EXEC SQL SELECT MAX(table_no)
		INTO	:max_table_no
		FROM	dd_regist_tables;
	EXEC SQL UPDATE dd_last_number
		SET	last_number = :max_table_no
		WHERE	column_name = 'table_no';
	if (RPdb_error_check(DBEC_ZERO_ROWS_OK, NULL) != OK)
		return (FAIL);

	EXEC SQL UPDATE dd_last_number
		FROM	dd_distrib_transaction
		SET	last_number = next_transaction_id
		WHERE	column_name = 'next_transaction_id';
	if (RPdb_error_check(DBEC_ZERO_ROWS_OK, NULL) != OK)
		return (FAIL);

	return (OK);
}


/*{
** Name:	drop_old_dict - drops old data dictionary
**
** Description:
**	Drops old data dictionary.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	OK or FAIL
*/
static STATUS
drop_old_dict()
{
	char	obj_name[DB_MAXNAME+1];
	i4	i;
	i4	j;

	/* drop 1.1 tables */
	for (i = 0; i < DIM(replic_tbls2); ++i)
		if (drop_object(replic_tbls2[i], OBJ_TABLE) != OK)
			return (FAIL);

	EXEC SQL DROP DBEVENT dd_dispatch;
	if (RPdb_error_check(0, NULL) != OK)
		return (FAIL);

	if (curr_level == REP1_1)
		return (OK);

	/* drop tables */
	for (i = 0; i < DIM(replic_tbls); ++i)
		if (drop_object(replic_tbls[i], OBJ_TABLE) != OK)
			return (FAIL);

	/* drop temp tables */
	for (i = 0; i < DIM(replic_tbls_same); ++i)
	{
		STprintf(obj_name, ERx("%s_tmp"), replic_tbls_same[i]);
		if (drop_object(obj_name, OBJ_TABLE) != OK)
			return (FAIL);
	}

	return (OK);
}


/*{
** Name:	convert_supp_objs - converts support objects
**
** Description:
**	Converts shadow and archive tables and creates database procedures.
**	Old support objects will be dropped whether the conversion went
**	successfully or not.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	OK or FAIL
*/
static STATUS
convert_supp_objs()
{
	DBEC_ERR_INFO	errinfo;
	i4	err;
	i4	i;
	i4	j;
	EXEC SQL BEGIN DECLARE SECTION;
	char	table_name[DB_MAXNAME+1];
	char	obj_name[DB_MAXNAME+1];
	i4	table_no;
	char	stmt[STMT_SIZE];
	char	cols[STMT_SIZE];
	EXEC SQL END DECLARE SECTION;

	if (tbls == NULL)
	{
		if (NumTables == 0)
			return (OK);
		else
			return (FAIL);
	}

	for (i = 0; i < NumTables; ++i)
	{
		STcopy(tbls[i].name, table_name);
		EXEC SQL SELECT table_no
			INTO	:table_no
			FROM	dd_regist_tables
			WHERE	table_name = :table_name;
		if (RPdb_error_check(0, NULL) != OK)
			return (FAIL);

		if (RMtbl_fetch(table_no, TRUE))
			return (FAIL);

		if (*RMtbl->columns_registered == EOS)
		{
			IIUGerr(E_RM0033_Must_reg_before_supp, UG_ERR_ERROR, 0);
			return (FAIL);
		}

		err = create_support_tables(table_no);
		if (err == OK || err == -1)
		{
			COLDEF	*col_p;

			/* convert shadow table */
			*cols = EOS;
			for (j = 0, col_p = RMtbl->col_p; j < RMtbl->ncols;
				++j, *col_p++)
			{
				if (col_p->key_sequence > 0)
				{
					STcat(cols, col_p->dlm_column_name);
					STcat(cols, ERx(", "));
				}
			}

			STprintf(stmt,
				ERx("INSERT INTO %s (%s%s) SELECT %s%s FROM %s_s"),
				RMtbl->sha_name, cols, new_shadow_cols, cols,
				old_shadow_cols, tbls[i].name);
			EXEC SQL EXECUTE IMMEDIATE :stmt;
			if (RPdb_error_check(DBEC_ZERO_ROWS_OK, NULL) != OK)
				return (FAIL);

			/* convert archive table */
			*cols = EOS;
			for (j = 0, col_p = RMtbl->col_p; j < RMtbl->ncols;
				++j, *col_p++)
			{
				if (col_p->column_sequence)
				{
					STcat(cols, col_p->dlm_column_name);
					STcat(cols, ERx(", "));
				}
			}
			STprintf(stmt,
				ERx("INSERT INTO %s (%s%s) SELECT %s%s FROM %s_a"),
				RMtbl->arc_name, cols, new_archive_cols, cols,
				old_archive_cols, tbls[i].name);
			EXEC SQL EXECUTE IMMEDIATE :stmt;
			if (RPdb_error_check(DBEC_ZERO_ROWS_OK, NULL) != OK)
				return (FAIL);
		}

		/* drop old support tables */
		STprintf(obj_name, ERx("%s_s"), tbls[i].name);
		if (drop_object(obj_name, OBJ_TABLE) != OK)
			return (FAIL);
		STprintf(obj_name, ERx("%s_a"), tbls[i].name);
		if (drop_object(obj_name, OBJ_TABLE) != OK)
			return (FAIL);

		/* drop old procedures */
		for (j = 0; j < DIM(proc_suffixes); ++j)
		{
			STprintf(obj_name, ERx("%s__%s"), tbls[i].name,
				proc_suffixes[j]);
			if (drop_object(obj_name, OBJ_DBPROC) != OK)
				return (FAIL);
		}

		/* create new database procedures */
		err = tbl_dbprocs(table_no);
		if (!err)
		{
			EXEC SQL UPDATE dd_regist_tables
				SET	supp_objs_created = DATE('now')
				WHERE	table_no = :table_no;
			if (RPdb_error_check(DBEC_SINGLE_ROW, NULL) != OK)
				return (FAIL);
		}
		EXEC SQL COMMIT;
	}

	return (OK);
}


/*{
** Name:	convert_dbprocs - converts database procedures
**
** Description:
**	Destroys 1.1 local procedures and recreates remote procedures.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	OK or FAIL
*/
static STATUS
convert_dbprocs()
{
	i4	i;
	i4	j;
	i4	err;
	DBEC_ERR_INFO	errinfo;
	char	obj_name[DB_MAXNAME+1];
	EXEC SQL BEGIN DECLARE SECTION;
	i4	table_no;
	EXEC SQL END DECLARE SECTION;

	if (tbls == NULL)
	{
		if (NumTables == 0)
			return (OK);
		else
			return (FAIL);
	}

	/* force database procedures to be recreated */
	EXEC SQL UPDATE dd_regist_tables
		SET	columns_registered = DATE('now');
	if (RPdb_error_check(0, NULL) != OK)
		return (FAIL);

	for (i = 0; i < NumTables; ++i)
	{
		/* drop local procedures */
		for (j = 0; j < DIM(proc_suffixes11); ++j)
		{
			STprintf(obj_name, ERx("%.*s%0*d%s"), TRUNC_TBLNAME_LEN,
				RPedit_name(EDNM_ALPHA, tbls[i].name, NULL),
				TBLOBJ_TBLNO_NDIGITS, (i4)tbls[i].no, 
				proc_suffixes11[j]);
			if (drop_object(obj_name, OBJ_DBPROC) != OK)
				return (FAIL);
		}

		table_no = tbls[i].no;
		if (RMtbl_fetch(table_no, TRUE))
			return (FAIL);

		/* recreate remote database procedures */
		err = tbl_dbprocs(table_no);
		if (!err)
		{
			EXEC SQL UPDATE dd_regist_tables
				SET	supp_objs_created = DATE('now')
				WHERE	table_no = :table_no;
			if (RPdb_error_check(DBEC_SINGLE_ROW, NULL) != OK)
				return (FAIL);
		}
		EXEC SQL COMMIT;
	}

	return (OK);
}
