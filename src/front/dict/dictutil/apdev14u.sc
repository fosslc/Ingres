/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/
EXEC SQL include sqlca;

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

static VOID	err_check();
static VOID	so_clean_up();

/**
** Name:	main.c -	Main Routine
**
** Description:
**	main		The main program.
**
** History:
**	18-jan-1993 (jpk)
**		Written.
**		To upgrade from 1.3 to 1.4 is simple: three queries.
**		They were written by davel.
**	8-Sept-1993 (jpk)
**		add support for new ii_reports column.
**	17-Mar-1997 (kitch01)
**		Bug 80734. Upgradefe performed on a remote database fails when
**		run from a client install. This is because we try to connect to
**		the local iidbdb, rather than the remote one, to find out if 
**		the database we are upgrading is a star database. I amended the
**		code to connect to the remote idbdb using the vnode passed as 
**		part of the remote database name.

**	9/24/93 (dkh) - Added "with default" for column replevel in ii_reports.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2004 (drivi01)
**	    Updated NEEDLIBS to link dynamic library SHQLIB to replace
**	    its static libraries.
**	29-Sep-2004 (drivi01)
**	    Updated NEEDLIBS to link dynamic library SHQLIB to replace
**	    its static libraries.
**/


/*{
** Name:	main() -	Main Entry Point for apdev1, v3-->4 upgrade.
**
** Description:
**	Performs upgrade of APDEV1 from version 3 to version 4.
**
** Side Effects:
**	Updates system catalogs.
**
** History:
**	18-jan-1993 (jpk) written.
**	 8-Sept-1993 (jpk) added new ii_reports column rptlevel
**	15-Sept-1993 (jpk) make that replevel, not rptlevel
**	9/24/93 (dkh) - Added "with default" for column replevel in ii_reports.
**      19-jun-95 (lawst01)
**        if upgrading DDB connect to CDB to do table modifications
**	17-Mar-1997 (kitch01)
**		Bug 80734. Upgradefe performed on a remote database fails when
**		run from a client install. This is because we try to connect to
**		the local iidbdb, rather than the remote one, to find out if
**		the database we are upgrading is a star database. I amended the
**		code to connect to the remote iidbdb using the vnode passed as
**		part of the remote database name.
**	23-dec-99 (hayke02)
**	    Check for a non-null chp before calling STbcompare() in order
**	    to prevent a SIGSEGV. The preceding call to STindex() will return
**	    null if the database name does not contain a '/' eg. ad104uin
**	    b99825. This change fixes bug 99825.
**      25-aug-2000 (chash01) 08-jun-99 (chash01)
**        Do not confuse gateway (i.e dbname/RMS) database with STAR database
**        by determine if extension is /STAR.
*/

/*
**	MKMFIN Hints
PROGRAM =	ad104uin

NEEDLIBS =	DICTUTILLIB SHQLIB COMPATLIB SHEMBEDLIB

UNDEFS =	II_copyright
*/
 
i4
main(argc, argv)
i4	argc;
char	**argv;
{

char	*largv[10], *chp = NULL;
int i;
char	*colon_ptr;
char	*vnode_ptr;
char	vnode[DB_MAXNAME+1];

EXEC SQL begin declare section;
  char     dbname[DB_MAXNAME+1];
  i4       dbservice;
  char	   iidbdbname[DB_MAXNAME+1];
EXEC SQL end   declare section;

        for (i=0; i< argc; i++)
        {
           largv[i] = argv[i];
        }
        chp = STindex(argv[1], ERx("/"), 0);
        largv[3] = ERx("-u'$ingres'");
        /*
        ** chash01 (08-jum-99) if "/" exists, then we need to know
        **   what follows it, if not STAR, we set chp back to NULL
        */
	if (chp)
	{
	    if ( STbcompare("STAR", 4, chp+1, 4, TRUE) )
		chp = NULL;
	}

	STcopy ((char *)argv[1], dbname );
	if (((colon_ptr = STindex(dbname, ":", 0)) != NULL) &&
	STindex(colon_ptr + 1, ":", 0) == (colon_ptr + 1))
	{
		/* Having found there is a vnode copy it */
		STlcopy(dbname, vnode, colon_ptr - dbname);
		vnode[colon_ptr-dbname] = '\0';
		/* The dbname as passed is a quoted string. We need
		** to remove the leading quote from the vnode, if there
		** is one.
		*/
		if ((vnode_ptr = STindex(vnode, "'", 1)) != NULL)
		{
			STcopy(vnode_ptr+1, vnode);
		}
	}
	else
	   vnode[0] = '\0';

	/* IF we got control from upgradedb, the '/star' is not passed
	   therefore, we must connect to the iidbdb and determine if
	   this database is a DDB.                                     */
	    
	if (chp == NULL && (STcompare (argv[1], ERx("iidbdb")) ))
	{
	  exec sql whenever sqlerror call sqlprint;
	  /* If there is a vnode we are trying to upgrade a remote db,
	  ** So connect to the remote DBs iidbdb to see if this is a 
	  ** star database
	  */
	  if (vnode[0] != '\0')
	     STprintf(iidbdbname, "%s::iidbdb", vnode);
	  else
	     STcopy("iidbdb", iidbdbname);
	  exec sql connect :iidbdbname;
	  err_check();
	  exec sql select dbservice into :dbservice from iidatabase where name
	       = :dbname;
          err_check();
	  exec sql disconnect;
	  if (dbservice & 1)  /* 1 = ddb  2 = cdb */
	     chp = argv[1]; 
	}


	if (IIDDdbcDataBaseConnect(argc,&largv) != OK)
	{
		PCexit(FAIL);
	}
	exec sql commit;
	exec sql set autocommit off;
	exec sql set lockmode session where level = table,
		readlock = exclusive;
	iiuicw1_CatWriteOn();

	exec sql UPDATE
		ii_abfobjects a
	FROM
		ii_objects o
	SET
		retlength = length(a.abf_source),
		rettype = 'char(' + ascii(length(a.abf_source)) + ')'
	WHERE
		o.object_id = a.object_id
	AND
		o.object_class = 2120
	AND
		a.abf_version < 3
	AND
		a.retadf_type = 20;
	err_check();

	exec sql UPDATE
		ii_abfobjects a
	FROM
		ii_objects o
	SET
		retlength = 2 + length(a.abf_source),
		rettype = 'varchar(' + ascii(2 + length(a.abf_source)) + ')'
	WHERE
		o.object_id = a.object_id
	AND
		o.object_class = 2120
	AND
		a.abf_version < 3
	AND
		a.retadf_type = 21;
	err_check();

	exec sql UPDATE
		ii_abfobjects a
	FROM
		ii_objects o
	SET
		abf_version = 3
	WHERE
		o.object_id = a.object_id
	AND
		o.object_class = 2120
	AND
		a.abf_version < 3;
	err_check();

        if (chp != NULL)
	{ 
	  exec sql commit;
        if (IIDDccCdbConnect() != OK)
	{
	   err_check();
        }
	}
	/* usual ersatz ALTER TABLE to add a column */
	exec sql CREATE TABLE iitmp_ii_reports AS SELECT * from ii_reports;
	err_check();
	exec sql DROP TABLE ii_reports;
	err_check();
	exec sql CREATE TABLE ii_reports
	(
		object_id	integer NOT NULL,
		reptype		char(1) NOT NULL,
		replevel	integer  NOT NULL WITH DEFAULT,
		repacount	smallint NOT NULL,
		repscount	smallint NOT NULL,
		repqcount	smallint NOT NULL,
		repbcount	smallint NOT NULL
	);
	err_check();
	exec sql INSERT INTO ii_reports
		(object_id, reptype, replevel, repacount,
	        repscount, repqcount, repbcount)
	SELECT
		object_id, reptype, 0, repacount,
	        repscount, repqcount, repbcount
	FROM iitmp_ii_reports;
	err_check();
	exec sql DROP TABLE iitmp_ii_reports;
	err_check();

        if (chp != NULL)
	{
	/*  commit changes to the CDB */
	exec sql commit;
	 
	/*  disconnect from the CDB  */
	IIDDcd2CdbDisconnect2();
	 
        /* we are now re-connected to the DDB session  */
	exec sql remove table ii_reports; 
	err_check();
	exec sql register table ii_reports as link;
	err_check();
	}

	so_clean_up(OK);
}

static VOID
err_check()
{
	STATUS stat = FEinqerr();

	if (stat == OK)
	{
		exec sql commit;
	}
	else
	{
		exec sql rollback;
		so_clean_up(FAIL);
	}
}

static VOID
so_clean_up(stat)
STATUS	stat;
{
	exec sql set lockmode session where level = page,
		readlock = shared;
	iiuicw0_CatWriteOff();
	PCexit(stat);
}
