/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<pc.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ui.h>
# include	<uigdata.h>
# include	<dictutil.h>

/**
** Name:	main.c -	Main Routine and Entry Point for module cleanup
**
** Description:
**	main		The main program.
**
** History:
**	7/90 (mikes) written.
**      3-jan-1990 (pete)
**		Make changes for Star support. If Star, run stmts on CDB.
**      26-feb-1991 (pete)
**          Make changes for Gateways. Replaced call to inline routine:
**          grant_all_to_dba with call to IIDDga_GrantAllToDba.
**      26-feb-1991 (pete)
**          1. Make changes for Gateways. Replaced call to inline routine:
**          grant_all_to_dba with call to IIDDga_GrantAllToDba.
**          2. Don't issue GRANT or MODIFY statements on Gateway; replace
**          MODIFY statements with ~equivalent CREATE INDEX statement.
**      29-jul-1991 (pete)
**          If connected to INGRES DBMS, then get exclusive read lock on each
**          table to be DD_ALTER_TABLE by doing SET LOCKMODE statement.
**          Change made in conjunction with removal of -l flag on connect stmt.
**          Also, removed the COMMIT from temp_tab() -- apparently the logging
**          requirements of "CREATE TABLE tmp AS SELECT * FROM tbl" are small
**          (they are small for MODIFY stmt too); should be ok to add to
**          transaction. Also changed temptable name from ii_temp_apdev23 to
**	    ii_temp_apdev13. SIR 38903.
**	    Changed PCexit(stat), where stat != OK to PCexit(FAIL).
**      16-nov-1992 (larrym)
**          Added CUFLIB to NEEDLIBS.  CUF is the new common utility facility.
**	22-july-1993 (jpk)
**		replaced call to FErelexists with call to cat_exists
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2004 (drivi01)
**	    Updated NEEDLIBS to link dynamic library SHQLIB to replace
**	    its static libraries.
**	23-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/


/*{
** Name:	main() -	Main Entry Point for apdev1, v2-->3 upgrade.
**
** Description:
**	Performs upgrade of APDEV1 from version 2 to version 3.
**
** Side Effects:
**	Updates system catalogs.
**
** History:
**	7/90 (mikes) written.
**	8-jan-1993 (jpk)
**		maintained FErelexists, added second parameter
**		for owner of table (FIPS support)
*/

/*
**	MKMFIN Hints
PROGRAM =	ad103uin

NEEDLIBS =	DICTUTILLIB SHQLIB COMPATLIB SHEMBEDLIB

UNDEFS =	II_copyright
*/

FUNC_EXTERN	STATUS	FErelexists(char *fetable, char *feowner);

FUNC_EXTERN bool        IIDDidIsDistributed();
FUNC_EXTERN bool        IIDDigIsGateway();
FUNC_EXTERN i4          loadStarStmts();
FUNC_EXTERN STATUS      IIDDccCdbConnect();
FUNC_EXTERN STATUS      IIDDcdCdbDisconnect();
FUNC_EXTERN VOID        IIDDga_GrantAllToDba();
FUNC_EXTERN VOID        IIDDserSetExclusiveReadlock();
FUNC_EXTERN VOID        IIDDssrSetSessionReadlock();

static STATUS col_exists();

static VOID class_create();
static VOID seq_create();
static VOID dep_mod();
static VOID seq_mod();
static VOID err_check();
static VOID temp_tab();
static VOID rem_temp();

static char *tempptr;

/*
** drive table for conversion.  We check to see if conversion has been
** done by seeing either if a table exists, or it has a new column.
** if "is_done" does not return OK, we call "convert".  The conversion
** routines must assure idempotency.  6.3 FE's will continue to work
** against partially upgraded DB's - they simply don't query the new
** tables or columns.  Once everything has either happened before or
** works, we're ready for 6.4
*/

DD_DRIVE Tab[] =
{
{ ERx("ii_abfdependencies"), ERx("abfdef_applid"), col_exists, dep_mod,
	 DD_ALTER_TABLE, FALSE },
{ ERx("ii_sequence_values"), ERx("sequence_owner"), col_exists, seq_mod,
	 DD_ALTER_TABLE, FALSE },
};

i4
main(argc, argv)
i4	argc;
char	**argv;
{
	STATUS stat = OK;
	DD_DRIVE *t;
	i4 i;

	if ((stat = IIDDdbcDataBaseConnect(argc,argv)) != OK)
                goto done;

	EXEC SQL SET AUTOCOMMIT OFF;

        if (IIDDidIsDistributed())
        {
            /* We're connected to Star; must do upgrade on CDB. */
            if ((stat = IIDDccCdbConnect()) != OK)
                goto done;
        }

	/* set table level lockmode to READLOCK = EXCLUSIVE */
	IIDDserSetExclusiveReadlock(Tab, (i4) (sizeof(Tab)/sizeof(Tab[0])));

	t = Tab;
	for (i=0; i < sizeof(Tab)/sizeof(Tab[0]); ++t,++i)
	{
		if ((*(t->is_done))(t->tab,t->col) == OK)
			continue;

		/*
		** redundant calls don't matter.  Do it this way so that
		** if a previous conversion routine turned it off, we
		** turn it on again.
		*/
		iiuicw1_CatWriteOn();

		(*(t->convert))();

		EXEC SQL COMMIT WORK;

		t->did_upgrade = TRUE;
	}

	rem_temp();

	EXEC SQL COMMIT WORK;

	/* reset table lockmode to session defaults */
	IIDDssrSetSessionReadlock(Tab, (i4) (sizeof(Tab)/sizeof(Tab[0])));

        if (IIDDidIsDistributed())
        {
            /* Above work was done on CDB; register it with Star. */

	    _VOID_ loadStarStmts( sizeof(Tab)/sizeof(Tab[0]), Tab);

            if ((stat = IIDDcdCdbDisconnect()) != OK)  /* note: this COMMITs */
                goto done;
        }

done:
	iiuicw0_CatWriteOff();

	PCexit(stat == OK ? OK : FAIL);
}

/*
** this routine !! does NOT !! return for failure to fetch the info.
** we return OK to indicate that the column exists, other to indicate that
** we should try to add it.
*/
static STATUS
col_exists(tab,col)
EXEC SQL BEGIN DECLARE SECTION;
char *tab;
char *col;
EXEC SQL END DECLARE SECTION;
{
	EXEC SQL BEGIN DECLARE SECTION;
	i4 cnt;
	EXEC SQL END DECLARE SECTION;
	STATUS stat;

	EXEC SQL REPEATED SELECT COUNT(*)
		INTO :cnt
		FROM iicolumns
                WHERE table_name = :tab
			AND column_name = :col;

	if ((stat = FEinqerr()) != OK)
		PCexit(FAIL);

	if (cnt > 0)
		return OK;

	return FAIL;
}

static VOID
dep_mod()
{
	temp_tab("ii_abfdependencies");

	EXEC SQL CREATE TABLE ii_abfdependencies (
		object_id integer NOT NULL,
		abfdef_applid integer WITH NULL,
		abfdef_name varchar(32) NOT NULL,
		abfdef_owner varchar(32) NOT NULL,
		object_class integer NOT NULL,
		abfdef_deptype integer NOT NULL,
		abf_linkname varchar(32) WITH NULL,
		abf_deporder integer WITH NULL
		) WITH NODUPLICATES;

	err_check();

	EXEC SQL INSERT INTO ii_abfdependencies (
			object_id,
			abfdef_applid,
			abfdef_name,
			abfdef_owner,
			object_class,
			abfdef_deptype,
			abf_linkname,
			abf_deporder
		) SELECT
			ii_temp_apdev13.object_id,
			ii_abfobjects.applid,
			abfdef_name,
			abfdef_owner,
			object_class,
			abfdef_deptype,
			abf_linkname,
			abf_deporder
		FROM ii_temp_apdev13, ii_abfobjects
		WHERE ii_temp_apdev13.object_id = ii_abfobjects.object_id;

	err_check();

	if (IIDDigIsGateway() == FALSE)
	{
	    /* Can't issue GRANT or MODIFY stmts on Ingres/Gateway */

	    IIDDga_GrantAllToDba(ERx("ii_abfdependencies"), &tempptr);

	    err_check();

	    EXEC SQL MODIFY ii_abfdependencies TO BTREE ON abfdef_applid,
			object_id;

	    err_check();
	}
        else
        {
            /* On Gateways we create an Index equivalent to the MODIFY */

	    /* 18 character name limit; truncate tbl name and add "_mod" */

            EXEC SQL CREATE INDEX ii_abfdependen_mod
                        ON ii_abfdependencies (abfdef_applid, object_id);

            err_check();
        }
}

/*
** We don't actually modify the old ii_sequence_values catalog; we delete it
** and create a new one.  We do this because the new seqeunce_value function
** will populate the catalog correctly if a row is missing, and because the old
** catalog has insufficent information to populate the new one.
*/
static VOID
seq_mod()
{
	EXEC SQL DROP TABLE ii_sequence_values;
	err_check();
	EXEC SQL COMMIT WORK;

	EXEC SQL CREATE TABLE ii_sequence_values (
		sequence_owner varchar(32) NOT NULL,
		sequence_table varchar(32) NOT NULL,
		sequence_column varchar(32) NOT NULL,
		sequence_value integer NOT NULL
		);
	err_check();

	if (IIDDigIsGateway() == FALSE)
	{
	    /* Can't issue GRANT or MODIFY stmts on Ingres/Gateway */

	    IIDDga_GrantAllToDba(ERx("ii_sequence_values"), &tempptr);

	    err_check();

	    EXEC SQL MODIFY ii_sequence_values
		TO HASH UNIQUE ON sequence_owner, sequence_table,sequence_column
		WHERE minpages = 32, fillfactor = 1;

	    err_check();
	}
        else
        {
            /* On Gateways we create an Index equivalent to the MODIFY */

	    /* 18 character name limit; truncate tbl name and add "_mod" */

            EXEC SQL CREATE UNIQUE INDEX ii_sequence_va_mod
                        ON ii_sequence_values
			(sequence_owner, sequence_table, sequence_column);

            err_check();
        }
}

/*
** copy catalog into ii_temp_apdev13 and drop it.
*/
static VOID
temp_tab(tname)
char *tname;
{
	EXEC SQL BEGIN DECLARE SECTION;
	char cmd[120];
	EXEC SQL END DECLARE SECTION;

	rem_temp();

	STprintf(cmd,
		ERx("create table ii_temp_apdev13 as select * from %s"),
		tname);

	EXEC SQL EXECUTE IMMEDIATE :cmd;

	err_check();

	/* keep transaction open */

	STprintf(cmd,"DROP TABLE %s",tname);

	EXEC SQL EXECUTE IMMEDIATE :cmd;

	err_check();
}

static VOID
rem_temp()
{
	if (cat_exists(ERx("ii_temp_apdev13"), "") == OK)
	{
		EXEC SQL DROP TABLE ii_temp_apdev13;

		err_check();
	}
}

static VOID
err_check()
{
	STATUS stat;

	stat = FEinqerr();

	if (stat != OK)
	{
		EXEC SQL ROLLBACK;
		PCexit(FAIL);
	}
}
