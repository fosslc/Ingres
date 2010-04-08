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
**	17-aug-2004 (lazro01)
**	    Written.
**	    To upgrade from 1.4 to 1.5.
**	    Changed repacount in ii_reports and rcosequence in ii_rcommands to
**	    i4 to handle values greater than 32K. 
**	3-Dec-2004 (schka24)
**	    Fix typo (ii_commands->ii_rcommands) that prevented Star databases
**	    from upgrading properly.
**      06-Jan-2008 (hanal04) Bug 119874
**          Make changes to allow spoof catalogs in gateway DBs to be
**          upgraded. If we have a gateway do not try to connect to iidbdb
**          to confirm whether this is a star connection (there won't be a DBMS
**          server), or try to update ii_reports (which will not exist), or
**          issue MODIFY syntax that is not supported in the gateways.
**	21-Jul-2008 (shust01)
**	    upgradefe received E_US0826 (DIRECT CONNECT command not allowed...)
**	    when running against a non-distributed database. Problem caused by
**	    querying a database with a database name that contained a vnode.
**	    b116674.
**	26-Aug-2009 (kschendel) b121804
**	    Bool prototypes to keep gcc 4.3 happy.
**/

FUNC_EXTERN bool        IIDDigIsGateway();


/*{
** Name:	main() -	Main Entry Point for apdev1, v4-->5 upgrade.
**
** Description:
**	Performs upgrade of APDEV1 from version 4 to version 5.
**
** Side Effects:
**	Updates system catalogs.
**
** History:
**	17-aug-2004 (lazro01) written.
*/

/*
**	MKMFIN Hints
PROGRAM =	ad105uin

NEEDLIBS =	DICTUTILLIB UILIB LIBQSYSLIB LIBQLIB UGLIB LIBQGCALIB FMTLIB \
		SQLCALIB AFELIB GCFLIB ADFLIB CUFLIB COMPATLIB

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
bool    maybe_star = TRUE;

EXEC SQL begin declare section;
  char     dbname[2*DB_MAXNAME+1]; /* double size since it can contain vnode */
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
            {
		chp = NULL;
                maybe_star = FALSE;
            }
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
		/* vnode saved off.  Remove it from dbname */
	        STcopy (colon_ptr+2, dbname );

		/* The dbname as passed is a quoted string. We need
		** to remove the trailing quote from the dbname, if there
		** is one.
		*/
		if (*(vnode_ptr = (dbname + strlen(dbname) - 1)) == '\'')
		{
		        *vnode_ptr = '\0';
		}
	}
	else
	   vnode[0] = '\0';

	/* IF we got control from upgradedb, the '/star' is not passed
	   therefore, we must connect to the iidbdb and determine if
	   this database is a DDB.                                     */
	    
	if (chp == NULL && maybe_star && (STcompare (argv[1], ERx("iidbdb")) ))
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

        if (chp != NULL)
	{ 
	  exec sql commit;
        if (IIDDccCdbConnect() != OK)
	{
	   err_check();
        }
	}

	/* Alter table ii_reports to modify the datatype of repacount to i4.*/

        /* ii_reports won't exist if this is a non-dbms gateway */
        if(IIDDigIsGateway() == FALSE)
        {
	    exec sql CREATE TABLE iitmp_ii_reports AS SELECT * from ii_reports;
	    err_check();
	    exec sql DROP TABLE ii_reports;
	    err_check();
	    exec sql CREATE TABLE ii_reports
	    (
		    object_id	integer NOT NULL,
		    reptype		char(1) NOT NULL,
		    replevel	integer  NOT NULL WITH DEFAULT,
		    repacount	integer  NOT NULL,
		    repscount	smallint NOT NULL,
		    repqcount	smallint NOT NULL,
		    repbcount	smallint NOT NULL
	    );
	    err_check();
	    exec sql INSERT INTO ii_reports
		    (object_id, reptype, replevel, repacount,
	            repscount, repqcount, repbcount)
	    SELECT
		    object_id, reptype, replevel, repacount,
	            repscount, repqcount, repbcount
	    FROM iitmp_ii_reports;
	    err_check();
	    exec sql MODIFY ii_reports
		    TO BTREE UNIQUE ON object_id;
	    err_check();
	    exec sql DROP TABLE iitmp_ii_reports;
	    err_check();
	    exec sql commit;
        }

	/*
	** Alter table ii_rcommands to modify the datatype of rcosequence to
	** i4.
	*/
	exec sql CREATE TABLE iitmp_ii_rcommands
                AS SELECT * from ii_rcommands;
	err_check();
	exec sql DROP TABLE ii_rcommands;
	err_check();
	exec sql CREATE TABLE ii_rcommands
	(
		object_id       integer NOT NULL,
		rcotype         char(2) NOT NULL,
		rcosequence     integer NOT NULL,
		rcosection      varchar(12) NOT NULL,
		rcoattid        varchar(32) NOT NULL,
		rcocommand      varchar(12) NOT NULL,
		rcotext         varchar(100) NOT NULL
	);
	err_check();
	exec sql INSERT INTO ii_rcommands
		(object_id, rcotype, rcosequence, rcosection,
		rcoattid, rcocommand, rcotext)
	SELECT
		object_id, rcotype, rcosequence, rcosection,
		rcoattid, rcocommand, rcotext
	FROM iitmp_ii_rcommands;
	err_check();

        /* Gateway won't support the modify syntax */
        if(IIDDigIsGateway() == FALSE)
        {
	    exec sql MODIFY ii_rcommands 
		    TO BTREE UNIQUE ON object_id, rcotype, rcosequence
		    WITH COMPRESSION=(DATA);
	    err_check();
        }
	exec sql DROP TABLE iitmp_ii_rcommands;
	err_check();
	exec sql commit;

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
	exec sql commit;
	exec sql remove table ii_rcommands; 
	err_check();
	exec sql register table ii_rcommands as link;
	err_check();
	exec sql commit;
	}

	so_clean_up(OK);
}

static VOID
err_check()
{
	STATUS stat = FEinqerr();

	if (stat != OK)
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
