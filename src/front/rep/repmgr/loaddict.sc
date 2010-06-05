/*
** Copyright (c) 1996, 2008 Ingres Corporation
*/
# include <compat.h>
# include <st.h>
# include <nm.h>
# include <lo.h>
# include <pc.h>
# include <me.h>
# include <er.h>
# include <gl.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>
# include <rpu.h>
# include "errm.h"

/**
** Name:	loaddict.sc - Load data dictionary objects
**
** Description:
**	Defines
**		load_tables		- create and load dictionary tables
**		update_cat_level	- update Replicator catalog level
**		RMcheck_cat_level	- check Replicator catalog level
**
** History:
**	16-dec-96 (joea)
**		Created based on loaddict.sc in replicator library.
**	14-jan-97 (joea)
**		Remove dd_target_types.
**	04-feb-97 (joea)
**		In update_cat_level, also update if level is current.
**	08-apr-97 (mcgem01)
**		Changed CA-OpenIngres to OpenIngres.
**	20-apr-98 (mcgem01)
**		Product name change to Ingres.
**	16-jun-98 (abbjo03)
**		Move individual calls in load_dict to repcat and convrep, and
**		eliminate load_dict.
**	23-sep-98 (abbjo03)
**		Replace ddba_messageit with IIUGerr.
**	03-sep-99 (abbjo03) sir 98644
**		Remove NODUPLICATES clause from input and distribution queues.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jul-2007 (kibro01) b118702
**	    Ensure internal dd_x tables always use ingresdate
**	08-may-2008 (joea)
**	    Update list of DBMS types.
**      08-jan-2009 (stial01)
**          Build replicator create statements using DB_MAXNAME for name cols.
**          Don't ERx replicator catalog names (no translation needed)
**      15-jan-2009 (stial01)
**          create_rep_catalog() fix for loop condition
**	26-Aug-2009 (kschendel) b121804
**	    Bool prototypes to keep gcc 4.3 happy.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/

EXEC SQL BEGIN DECLARE SECTION;
# define CURR_CATALOG_LEVEL	'00200'
EXEC SQL END DECLARE SECTION;

# define	MAX_SERVERS		10
# define	SERVER_NAME		ERx("dd_server")
# define	DIM(a)			(sizeof(a) / sizeof(a[0]))


STATUS update_cat_level(void);

static STATUS create_rep_catalog(char *repcat);
static VOID rep_fix_maxname(char *b);


EXEC SQL BEGIN DECLARE SECTION;
static struct
{
	char	*type;
	char	*descrip;
	i2	gateway;
} dbms_types[] =
{
	{ERx("db2"),		ERx("IBM DB2 Gateway"),		1},
	{ERx("db2udb"),		ERx("IBM DB2 UDB Gateway"),	1},
	{ERx("dcom"),		ERx("CA-Datacom Gateway"),	1},
	{ERx("idms"),		ERx("IDMS Gateway"),		1},
	{ERx("ims"),		ERx("IMS Gateway"),		1},
	{ERx("informix"),	ERx("Informix Gateway"),	1},
	{ERx("ingres"),		ERx("Ingres"),			0},
	{ERx("mssql"),		ERx("SQL Server Gateway"),	1},
	{ERx("oracle"),		ERx("Oracle Gateway"),		1},
	{ERx("rdb"),		ERx("Rdb Gateway"),		1},
	{ERx("rms"),		ERx("HP RMS Gateway"),		1},
	{ERx("sybase"),		ERx("Sybase Gateway"),		1},
	{ERx("vsam"),		ERx("VSAM Gateway"),		1},
};
EXEC SQL END DECLARE SECTION;

static struct
{
    char *repcat;		/* replicator catalog name */
    char *repcreate;		/* replicator CREATE TABLE statement */
} supp_tbls[] = 
{
{
"dd_nodes",
"CREATE TABLE dd_nodes ("
" vnode_name CHAR(##DB_NODE_MAXNAME##) NOT NULL,"
" node_type  SMALLINT NOT NULL WITH DEFAULT)"
" WITH NODUPLICATES, JOURNALING",
},

{
"dd_databases",
"CREATE TABLE dd_databases ("
" database_no   SMALLINT NOT NULL,"
" vnode_name    char(##DB_NODE_MAXNAME##) NOT NULL,"
" database_name char(##DB_DB_MAXNAME##) NOT NULL,"
" database_owner char(##DB_OWN_MAXNAME##) NOT NULL WITH DEFAULT,"
" dbms_type CHAR(8) NOT NULL WITH DEFAULT,"
" security_level CHAR(2) NOT NULL WITH DEFAULT,"
" local_db SMALLINT NOT NULL WITH DEFAULT,"
" config_changed CHAR(25) NOT NULL WITH DEFAULT,"
" remark VARCHAR(80) NOT NULL WITH DEFAULT)"
" WITH NODUPLICATES, JOURNALING"
},

{
"dd_paths",
"CREATE TABLE dd_paths ("
" cdds_no SMALLINT NOT NULL,"
" localdb SMALLINT NOT NULL,"
" sourcedb SMALLINT NOT NULL,"
" targetdb SMALLINT NOT NULL,"
" final_target SMALLINT NOT NULL WITH DEFAULT)"
" WITH NODUPLICATES, JOURNALING"
},

{
/* Note A cdds_name is just a name for a CDDS, which is an Ingres
** Replicator invention.  At best it could be considered analogous to a
** database name since a CDDS is a collection of tables (or parts of
** tables) that is supposed to be kept consistent across multiple databases.
*/
"dd_cdds",
"CREATE TABLE dd_cdds ("
" cdds_no SMALLINT NOT NULL,"
" cdds_name char(##DB_DB_MAXNAME##) NOT NULL," /* keep same as dbname*/
" control_db SMALLINT NOT NULL WITH DEFAULT,"
" collision_mode SMALLINT NOT NULL WITH DEFAULT,"
" error_mode SMALLINT NOT NULL WITH DEFAULT)"
" WITH NODUPLICATES, JOURNALING"
},

{
"dd_db_cdds",
"CREATE TABLE dd_db_cdds ("
" cdds_no SMALLINT NOT NULL,"
" database_no SMALLINT NOT NULL,"
" target_type SMALLINT NOT NULL,"
" is_quiet SMALLINT NOT NULL WITH DEFAULT,"
" server_no SMALLINT NOT NULL WITH DEFAULT)"
" WITH NODUPLICATES, JOURNALING"
},

{
"dd_regist_tables",
"CREATE TABLE dd_regist_tables ("
" table_no INTEGER NOT NULL,"
" table_name char(##DB_TAB_MAXNAME##) NOT NULL,"
" table_owner char(##DB_OWN_MAXNAME##) NOT NULL WITH DEFAULT,"
" columns_registered CHAR(25) NOT NULL WITH DEFAULT,"
" supp_objs_created CHAR(25) NOT NULL WITH DEFAULT,"
" rules_created	CHAR(25) NOT NULL WITH DEFAULT,"
" cdds_no SMALLINT NOT NULL WITH DEFAULT,"
" cdds_lookup_table char(##DB_TAB_MAXNAME##) NOT NULL WITH DEFAULT,"
" prio_lookup_table char(##DB_TAB_MAXNAME##) NOT NULL WITH DEFAULT,"
" index_used char(##DB_TAB_MAXNAME##) NOT NULL WITH DEFAULT)"
" WITH NODUPLICATES, JOURNALING"
},

{
"dd_regist_columns",
" CREATE TABLE dd_regist_columns ("
" table_no INTEGER NOT NULL,"
" column_name char(##DB_ATT_MAXNAME##) NOT NULL,"
" column_sequence INTEGER NOT NULL,"
" key_sequence INTEGER NOT NULL WITH DEFAULT)"
" WITH NODUPLICATES, JOURNALING"
},

{
"dd_input_queue",
"CREATE TABLE dd_input_queue ("
" sourcedb SMALLINT NOT NULL,"
" transaction_id INTEGER NOT NULL,"
" sequence_no INTEGER NOT NULL,"
" trans_type SMALLINT NOT NULL,"
" table_no INTEGER NOT NULL,"
" old_sourcedb SMALLINT NOT NULL,"
" old_transaction_id INTEGER NOT NULL,"
" old_sequence_no INTEGER NOT NULL,"
" trans_time INGRESDATE WITH NULL,"
" cdds_no SMALLINT NOT NULL WITH DEFAULT,"
" dd_priority SMALLINT NOT NULL WITH DEFAULT)"
" WITH JOURNALING"
},

{
"dd_distrib_queue",
"CREATE TABLE dd_distrib_queue ("
" targetdb SMALLINT NOT NULL,"
" sourcedb SMALLINT NOT NULL,"
" transaction_id INTEGER NOT NULL,"
" sequence_no INTEGER NOT NULL,"
" trans_type SMALLINT NOT NULL,"
" table_no INTEGER NOT NULL,"
" old_sourcedb SMALLINT NOT NULL,"
" old_transaction_id INTEGER NOT NULL,"
" old_sequence_no INTEGER NOT NULL,"
" trans_time INGRESDATE NOT NULL,"
" cdds_no SMALLINT NOT NULL WITH DEFAULT,"
" dd_priority SMALLINT NOT NULL WITH DEFAULT)"
" WITH JOURNALING"
},

{
"dd_mail_alert",
"CREATE TABLE dd_mail_alert ("
" mail_username VARCHAR(80) NOT NULL)"
" WITH NODUPLICATES, JOURNALING"
},

{
"dd_servers",
"CREATE TABLE dd_servers ("
" server_no SMALLINT NOT NULL,"
" server_name VARCHAR(24) NOT NULL,"
" pid VARCHAR(12) NOT NULL WITH DEFAULT)"
" WITH NODUPLICATES, JOURNALING"
},

{
"dd_support_tables",
"CREATE TABLE dd_support_tables ("
" table_name char(##DB_TAB_MAXNAME##) NOT NULL)"
" WITH NODUPLICATES, JOURNALING"
},

{
"dd_last_number",
"CREATE TABLE dd_last_number ("
" column_name char(##DB_ATT_MAXNAME##) NOT NULL,"
" last_number INTEGER NOT NULL,"
" filler VARCHAR(1500) NOT NULL WITH DEFAULT)"
" WITH NODUPLICATES"
},

{
/* intentionally leaving replicator event name 32 */
"dd_events",
"CREATE TABLE dd_events ("
" dbevent char(32) NOT NULL,"
" action SMALLINT NOT NULL,"
" sort_order INTEGER NOT NULL WITH DEFAULT,"
" s_flag SMALLINT NOT NULL,"
" event_desc VARCHAR(80) NOT NULL WITH DEFAULT)"
" WITH NODUPLICATES, JOURNALING"
},

{
"dd_flag_values",
"CREATE TABLE dd_flag_values ("
" startup_flag VARCHAR(6) NOT NULL,"
" startup_value VARCHAR(8) NOT NULL,"
" flag_description VARCHAR(500)	NOT NULL)"
" WITH NODUPLICATES, JOURNALING"
},

{
"dd_option_values",
"CREATE TABLE dd_option_values ("
" option_type CHAR(1) NOT NULL,"
" option_name CHAR(20) NOT NULL,"
" numeric_value	INTEGER NOT NULL,"
" alpha_value CHAR(20) NOT NULL,"
" option_desc VARCHAR(250) NOT NULL WITH DEFAULT,"
" startup_flag CHAR(1) NOT NULL,"
" flag_name CHAR(6) NOT NULL NOT DEFAULT)"
" WITH NODUPLICATES, JOURNALING"
},

{
"dd_server_flags",
"CREATE TABLE dd_server_flags ("
" flag_name CHAR(6) NOT NULL,"
" option_name CHAR(20) NOT NULL,"
" short_description CHAR(20) NOT NULL,"
" flag_description VARCHAR(500) NOT NULL WITH DEFAULT,"
" startup_flag CHAR(1) NOT NULL)"
" WITH NODUPLICATES, JOURNALING"
},

{
"dd_dbms_types",
"CREATE TABLE dd_dbms_types ("
" dbms_type CHAR(8) NOT NULL,"
" short_description VARCHAR(20)	NOT NULL,"
" gateway SMALLINT NOT NULL WITH DEFAULT)"
" WITH NODUPLICATES, JOURNALING"
}
};

/*{
** Name:	load_tables - create and load data dictionary tables
**
** Description:
**	Creates and loads the Replicator data dictionary tables.
**	Replicator DD tables are created.  Some of them are loaded with
**	static information.
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
load_tables()
{
	DBEC_ERR_INFO	errinfo;
	char	*p;
	EXEC SQL BEGIN DECLARE SECTION;
	i4	i;
	char	server_name[25];
	char	datafile[MAX_LOC+1];
	char	*rep_tab_name;
	EXEC SQL END DECLARE SECTION;
	LOCATION	loc;

	if (NMloc(FILES, PATH, NULL, &loc) != OK)
	{
		SIprintf("II_CONFIG not defined\n");
		return (FAIL);
	}
	if (LOfaddpath(&loc, ERx("rep"), &loc) != OK)
	{
		SIprintf("Error getting Replicator files directory\n");
		return (FAIL);
	}

	if (create_rep_catalog("dd_nodes") != OK)
		return (FAIL);

	if (create_rep_catalog("dd_databases") != OK)
		return (FAIL);

	if (create_rep_catalog("dd_paths") != OK)
		return (FAIL);

	if (create_rep_catalog("dd_cdds") != OK)
		return (FAIL);

	EXEC SQL INSERT INTO dd_cdds
		VALUES	(0, 'Default CDDS', 0, 0, 0);
	if (RPdb_error_check(DBEC_SINGLE_ROW, &errinfo) != OK)
	{
		IIUGerr(E_RM00D9_Err_loading_cat, UG_ERR_ERROR, 1, "dd_cdds");
		return (FAIL);
	}

	if (create_rep_catalog("dd_db_cdds") != OK)
		return (FAIL);

	if (create_rep_catalog("dd_regist_tables") != OK)
		return (FAIL);

	if (create_rep_catalog("dd_regist_columns") != OK)
		return (FAIL);

	if (create_rep_catalog("dd_input_queue") != OK)
		return (FAIL);

	if (create_rep_catalog("dd_distrib_queue") != OK)
		return (FAIL);

	if (create_rep_catalog("dd_mail_alert") != OK)
		return (FAIL);

	if (create_rep_catalog("dd_servers") != OK)
		return (FAIL);

	for (i = 1; i <= MAX_SERVERS; ++i)
	{
		STprintf(server_name, ERx("%s%d"), SERVER_NAME, i);
		EXEC SQL INSERT INTO dd_servers
			VALUES	(:i, :server_name, '');
		if (RPdb_error_check(DBEC_SINGLE_ROW, &errinfo) != OK)
		{
			IIUGerr(E_RM00D9_Err_loading_cat, UG_ERR_ERROR, 1,
				"dd_servers");
			return (FAIL);
		}
	}

	if (create_rep_catalog("dd_support_tables") != OK)
		return (FAIL);

	for (i = 0; i < DIM(supp_tbls); ++i)
	{
		rep_tab_name = supp_tbls[i].repcat;
		EXEC SQL INSERT INTO dd_support_tables VALUES (:rep_tab_name);
		if (RPdb_error_check(DBEC_SINGLE_ROW, &errinfo) != OK)
		{
			IIUGerr(E_RM00D9_Err_loading_cat, UG_ERR_ERROR, 1,
				"dd_support_tables");
			return (FAIL);
		}
	}

	if (create_rep_catalog("dd_last_number") != OK)
		return (FAIL);

	EXEC SQL INSERT INTO dd_last_number
		VALUES	('table_no', 0, '');
	if (RPdb_error_check(DBEC_SINGLE_ROW, &errinfo) != OK)
	{
		IIUGerr(E_RM00D9_Err_loading_cat, UG_ERR_ERROR, 1,
			"dd_last_number");
		return (FAIL);
	}

	EXEC SQL INSERT INTO dd_last_number
		VALUES	('next_transaction_id', 1, '');
	if (RPdb_error_check(DBEC_SINGLE_ROW, &errinfo) != OK)
	{
		IIUGerr(E_RM00D9_Err_loading_cat, UG_ERR_ERROR, 1,
			"dd_last_number");
		return (FAIL);
	}

	if (create_rep_catalog("dd_events") != OK)
		return (FAIL);

	LOfstfile(ERx("events.dat"), &loc);
	LOtos(&loc, &p);
	STcopy(p, datafile);
	EXEC SQL COPY TABLE dd_events (
		dbevent		= c0tab,
		action		= c0tab,
		sort_order	= c0tab,
		s_flag		= c0tab,
		event_desc	= c0nl,
		nl		= d1)
	FROM :datafile;
	if (RPdb_error_check(0, &errinfo) != OK)
	{
		IIUGerr(E_RM00D9_Err_loading_cat, UG_ERR_ERROR, 1,
			"dd_events");
		return (FAIL);
	}

	if (create_rep_catalog("dd_flag_values") != OK)
		return (FAIL);

	LOfstfile(ERx("flagvals.dat"), &loc);
	LOtos(&loc, &p);
	STcopy(p, datafile);
	EXEC SQL COPY TABLE dd_flag_values (
		startup_flag		= c0tab,
		startup_value		= c0tab,
		flag_description	= c0nl,
		nl			= d1)
	FROM :datafile;
	if (RPdb_error_check(0, &errinfo) != OK)
	{
		IIUGerr(E_RM00D9_Err_loading_cat, UG_ERR_ERROR, 1,
			"dd_flag_values");
		return (FAIL);
	}

	if (create_rep_catalog("dd_option_values") != OK)
		return (FAIL);

	LOfstfile(ERx("optnvals.dat"), &loc);
	LOtos(&loc, &p);
	STcopy(p, datafile);
	EXEC SQL COPY TABLE dd_option_values (
		option_type	= c0tab,
		option_name	= c0tab,
		numeric_value	= c0tab,
		alpha_value	= c0tab,
		option_desc	= c0tab,
		startup_flag	= c0tab,
		flag_name	= c0nl,
		nl		= d1)
	FROM :datafile;
	if (RPdb_error_check(0, &errinfo) != OK)
	{
		IIUGerr(E_RM00D9_Err_loading_cat, UG_ERR_ERROR, 1,
			"dd_option_values");
		return (FAIL);
	}

	if (create_rep_catalog("dd_server_flags") != OK)
		return (FAIL);

	LOfstfile(ERx("svrflags.dat"), &loc);
	LOtos(&loc, &p);
	STcopy(p, datafile);
	EXEC SQL COPY TABLE dd_server_flags (
		flag_name	= c0tab,
		option_name	= c0tab,
		short_description = c0tab,
		flag_description = c0tab,
		startup_flag	= c0nl,
		nl		= d1)
	FROM :datafile;
	if (RPdb_error_check(0, &errinfo) != OK)
	{
		IIUGerr(E_RM00D9_Err_loading_cat, UG_ERR_ERROR, 1,
			"dd_server_flags");
		return (FAIL);
	}

	if (create_rep_catalog("dd_dbms_types") != OK)
		return (FAIL);

	for (i = 0; i < DIM(dbms_types); ++i)
	{
		EXEC SQL INSERT INTO dd_dbms_types
			VALUES	(:dbms_types[i].type, :dbms_types[i].descrip,
				:dbms_types[i].gateway);
		if (RPdb_error_check(DBEC_SINGLE_ROW, &errinfo) != OK)
		{
			IIUGerr(E_RM00D9_Err_loading_cat, UG_ERR_ERROR, 1,
				"dd_dbms_types");
			return (FAIL);
		}
	}

	return (OK);
}


/*{
** Name:	update_cat_level - Update Replicator catalog level
**
** Description:
**	Inserts/updates Replicator catalog level in iidbcapabilities.
**	Callers are responsible for connecting as $ingres and committing
**	the update/insert.
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
update_cat_level()
{
	DBEC_ERR_INFO	errinfo;

	EXEC SQL UPDATE iidbcapabilities
		SET	cap_value = :CURR_CATALOG_LEVEL
		WHERE	cap_capability = 'REP_CATALOG_LEVEL'
		AND	cap_value <= :CURR_CATALOG_LEVEL;
	if (RPdb_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != OK)
		return (FAIL);
	if (errinfo.rowcount == 0)
	{
		EXEC SQL INSERT INTO iidbcapabilities
			VALUES	('REP_CATALOG_LEVEL', :CURR_CATALOG_LEVEL);
		if (RPdb_error_check(DBEC_SINGLE_ROW, NULL) != OK)
			return (FAIL);
	}

	return (OK);
}


/*{
** Name:	RMcheck_cat_level - check Replicator catalog level
**
** Description:
**	Checks if the data dictionary catalogs are up to the current
**	release level.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	TRUE if release level OK, FALSE otherwise
*/
bool
RMcheck_cat_level(void)
{
	EXEC SQL BEGIN DECLARE SECTION;
	i4	catalogs_exist;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL SELECT	COUNT(*)
		INTO	:catalogs_exist
		FROM	iidbcapabilities
		WHERE	cap_capability = 'REP_CATALOG_LEVEL'
		AND	cap_value >= :CURR_CATALOG_LEVEL;
	if (RPdb_error_check(DBEC_SINGLE_ROW, NULL) != OK ||
			!catalogs_exist)
		return (FALSE);
	else
		return (TRUE);
}

static STATUS
create_rep_catalog(char *repcat)
{
    DBEC_ERR_INFO	errinfo;
    i4			i;
    i4			tbl;
    bool		created = FALSE;
    char		*fix_maxname;
    char		maxbuf[20];
    char		*buf = NULL;
    STATUS		memstat;
    char		tempbuf[1024];
    i4			qlen;
EXEC SQL BEGIN DECLARE SECTION;
    char		*qry;
EXEC SQL END DECLARE SECTION;

    /* find this catalog in supp_tbls */
    for (i = 0; i < DIM(supp_tbls); ++i)
    {
	if (STcompare(supp_tbls[i].repcat, repcat))
	    continue;

	qry = supp_tbls[i].repcreate;

	/* fix MAXNAME  */
	fix_maxname = STstrindex(qry, "##DB_", 0, 0);

	if (fix_maxname)
	{
	    qlen = STlength(qry);
	    if (qlen + 1 < sizeof(tempbuf))
		buf = tempbuf;
	    else
	    {
		buf = (char *) MEreqmem(0, STlength(qry) + 1, TRUE, &memstat);
		if (!buf || memstat != OK)
		{
		    IIUGerr(E_RM00FE_Err_alloc_col, UG_ERR_ERROR, 0);
		    return (FAIL);
		}
	    }

	    STcopy(qry, buf); 

	    for ( ; ; )
	    {
		fix_maxname = STstrindex(buf, "##DB_", 0, 0);
		if (!fix_maxname)
		    break;
		rep_fix_maxname(fix_maxname);
	    }
	    qry = buf;
	}

	exec sql execute immediate :qry;

	if (RPdb_error_check(DBEC_ZERO_ROWS_OK, &errinfo) == OK)
	    created = TRUE;

	break;
    }

    if (buf && buf != tempbuf)
	MEfree(buf);

    if (!created)
    {
	IIUGerr(E_RM00D8_Err_creating_cat, UG_ERR_ERROR, 1, repcat);
	return (FAIL);
    }

    return (OK);
}  

static VOID
rep_fix_maxname(char *b)
{
    char tempbuf[100];

    if (!MEcmp(b, "##DB_MAXNAME##", 14))
    {
	CVla((i4)DB_MAXNAME, tempbuf);
	MEmove(STlength(tempbuf), tempbuf, ' ', 14, b);
    }
    else if (!MEcmp(b, "##DB_OWN_MAXNAME##", 18))
    {
	CVla((i4)DB_OWN_MAXNAME, tempbuf);
	MEmove(STlength(tempbuf), tempbuf, ' ', 18, b);
    }
    else if (!MEcmp(b, "##DB_TAB_MAXNAME##", 18))
    {
	CVla((i4)DB_TAB_MAXNAME, tempbuf);
	MEmove(STlength(tempbuf), tempbuf, ' ', 18, b);
    }
    else if (!MEcmp(b, "##DB_ATT_MAXNAME##", 18))
    {
	CVla((i4)DB_ATT_MAXNAME, tempbuf);
	MEmove(STlength(tempbuf), tempbuf, ' ', 18, b);
    }
    else if (!MEcmp(b, "##DB_DB_MAXNAME##", 17))
    {
	CVla((i4)DB_DB_MAXNAME, tempbuf);
	MEmove(STlength(tempbuf), tempbuf, ' ', 17, b);
    }
    else if (!MEcmp(b, "##DB_NODE_MAXNAME##", 19))
    {
	CVla((i4)DB_NODE_MAXNAME, tempbuf);
	MEmove(STlength(tempbuf), tempbuf, ' ', 19, b);
    }
    else
    {
	printf("unexpected format string %s\n", b);
    }
}
