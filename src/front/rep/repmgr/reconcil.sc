/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <st.h>
# include <cv.h>
# include <er.h>
# include <ex.h>
# include <me.h>
# include <pc.h>
# include <ci.h>
# include <gl.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>
# include <uigdata.h>
# include <rpu.h>
# include <tblobjs.h>
# include "errm.h"

# include <si.h>

/**
** Name:	reconcil.sc - reconcile replicated database
**
** Description:
**	Defines
**		main		- main routine
**		reconcile	- move unreplicated rows to dd_distrib_queue
**
**  History:
**	16-dec-96 (joea)
**		Created based on reconcil.sc in replicator library.
**      15-jan-97 (hanch04)
**              Added missing NEEDLIBS
**	27-mar-98 (joea)
**		Discontinue use of RPing_ver_ok.
**	20-apr-98 (mcgem01)
**		Product name change to Ingres.
**	07-jul-98 (abbjo03)
**		Remove command line arguments confirmation prompt.
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
**/

/*
PROGRAM =	reconcil

NEEDLIBS =      REPMGRLIB REPCOMNLIB SHFRAMELIB SHQLIB \
		SHCOMPATLIB SHEMBEDLIB 

UNDEFS =	II_copyright
*/

static	char	*db_name = NULL;
static	i2	target_db = 0;
static	i4	target_db_nat = 0;
static	char	*cdds_values = ERx("");
static	char	*start_time = ERx("");
static	char	*user_name = ERx("");

static	ARG_DESC args[] =
{
	/* required */
	{ERx("database"), DB_CHR_TYPE, FARG_PROMPT, (PTR)&db_name},
	{ERx("target_db"), DB_INT_TYPE, FARG_PROMPT, (PTR)&target_db_nat},
	{ERx("cdds_values"), DB_CHR_TYPE, FARG_PROMPT, (PTR)&cdds_values},
	{ERx("start_time"), DB_CHR_TYPE, FARG_PROMPT, (PTR)&start_time},
	/* optional */
	{ERx("user"), DB_CHR_TYPE, FARG_FAIL, (PTR)&user_name},
	NULL
};


static STATUS reconcile(i2 target_db, char *start_time, char *cdds_buff);


FUNC_EXTERN STATUS FEhandler(EX_ARGS *ex);


/*{
** Name:	main	- main routine of Reconciler utility
**
** Description:
**
** Inputs:
**	argc - the number of arguments on the command line
**	argv - the command line arguments
**		argv[1] - database to connect to
**		argv[2] - target database number
**		argv[3] - CDDS number
**		argv[4] - start time
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
	char	ans[12];
	char	cddsbuff[128];
	EX_CONTEXT	context;

	/* Tell EX this is an INGRES tool */
	(void)EXsetclient(EX_INGRES_TOOL);

	/* Use the INGRES memory allocator */
	(void)MEadvise(ME_INGRES_ALLOC);

	/* Initialize character set attribute table */
	if (IIUGinit() != OK)
		PCexit(FAIL);

	/* Parse the command line arguments */
	if (IIUGuapParse(argc, argv, ERx("reconcil"), args) != OK)
		PCexit(FAIL);

	/* Set up the generic exception handler */
	if (EXdeclare(FEhandler, &context) != OK)
	{
		EXdelete();
		PCexit(FAIL);
	}

	SIprintf("Ingres Replicator Reconciler\n");
	SIflush(stdout);

	CVupper(cdds_values);
	if (STcompare(cdds_values, ERx("ALL")) == 0)
	{
		*cddsbuff = EOS;
	}
	else
	{
		if (*cdds_values == '(')
			STprintf(cddsbuff, ERx("AND cdds_no IN %s"),
				cdds_values);
		else
			STprintf(cddsbuff, ERx("AND cdds_no = %s"),
				cdds_values);
	}

	/* Open the database */
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

	target_db = (i2)target_db_nat;
	/* FIXME:  should validate target db */
	if (reconcile(target_db, start_time, cddsbuff) != OK)
	{
		EXEC SQL ROLLBACK;
		FEing_exit();

		SIprintf("Error reconciling database '%s'.\n", db_name);
		EXdelete();
		PCexit(FAIL);
	}

	EXEC SQL COMMIT;
	FEing_exit();

	SIprintf("Replicator Reconciler completed successfully.\n");

	EXdelete();
	PCexit(OK);
}


/*{
** Name:	reconcile - move unreplicated rows to dd_distrib_queue
**
** Description:
**	Moves unreplicated rows to dd_distrib_queue.
**
** Inputs:
**	target_db	- target database number
**	start_time	- transaction time
**	cddsbuff	- cdds number
**
** Outputs:
**	none
**
** Returns:
**	OK or FAIL
*/
STATUS
reconcile(
i2	target_db,
char	*start_time,
char	*cddsbuff)
{
	DBEC_ERR_INFO	errinfo;
	EXEC SQL BEGIN DECLARE SECTION;
	i4	tbl_no;
	char	tbl_name[DB_MAXNAME+1];
	char	tbl_owner[DB_MAXNAME+1];
	char	stmt[2048];
	EXEC SQL END DECLARE SECTION;
	char	shadow_name[DB_MAXNAME+1];

	EXEC SQL DECLARE tbl_cursor CURSOR FOR
		SELECT	table_name, table_owner, table_no
		FROM	dd_regist_tables;
	EXEC SQL OPEN tbl_cursor FOR READONLY;
	if (RPdb_error_check(0, NULL) != OK)
		return (FAIL);

	while (TRUE)
	{
		EXEC SQL FETCH tbl_cursor
			INTO	:tbl_name, :tbl_owner, :tbl_no;
		if (RPdb_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != OK)
			return (FAIL);
		if (errinfo.rowcount != 1)
			break;

		STtrmwhite(tbl_owner);
		STtrmwhite(tbl_name);
		RPtblobj_name(tbl_name, tbl_no, TBLOBJ_SHA_TBL, shadow_name);
		STprintf(stmt, ERx("\
INSERT INTO dd_distrib_queue (sourcedb, transaction_id, sequence_no, \
	table_no, old_sourcedb, old_transaction_id, old_sequence_no, \
	trans_time, trans_type, cdds_no, dd_priority, targetdb) \
SELECT	sourcedb, transaction_id, sequence_no, \
	%d, old_sourcedb, old_transaction_id, old_sequence_no, \
	trans_time, trans_type, cdds_no, dd_priority, %d \
FROM	%s s \
WHERE	trans_time > '%s' \
%s \
AND	NOT EXISTS ( \
	SELECT * \
	FROM	dd_distrib_queue t \
	WHERE	s.sourcedb = t.sourcedb \
	AND	s.transaction_id = t.transaction_id \
	AND	s.sequence_no = t.sequence_no \
	AND	s.trans_type = t.trans_type)"),
			(i4)tbl_no, (i4)target_db, shadow_name, start_time,
			cddsbuff);

		EXEC SQL EXECUTE IMMEDIATE :stmt;
		if (RPdb_error_check(DBEC_ZERO_ROWS_OK, &errinfo) != OK)
			return (FAIL);

		if (errinfo.rowcount)
			SIprintf("%d records inserted into dd_distrib_queue for '%s.%s'\n",
				errinfo.rowcount, tbl_owner, tbl_name);
	}
	EXEC SQL CLOSE tbl_cursor;
	if (RPdb_error_check(0, NULL) != OK)
		return (FAIL);

	return (OK);
}
