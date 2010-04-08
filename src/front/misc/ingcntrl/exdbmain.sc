/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<pc.h>		/* 6-x_PC_80x86 */
# include	<cv.h>
# include	<ex.h>
# include	<me.h>
# include	<st.h>
# include	<si.h>
# include	<gl.h>
# include	<sl.h>
# include	<cm.h>
# include	<iicommon.h>
# include 	<er.h>
# include	<fe.h>
# include	<fedml.h>
# include	<uigdata.h>
# include	<ug.h>
# include	<ui.h>
# include       <adf.h>
# include       <afe.h>
# include	<xf.h>
# include	<generr.h>
# include 	"eric.h"
#include        "global.h"
#include        "ictables.h"


/**
** Name:    exdbmain.sc - main procedure for EXTENDDB.
**
** Description:
**	This file defines:
**
**	main		main procedure for EXTENDDB utility
**			which tries to do WAY too much by
**			mixing CREATE/ALTER/DROP LOCATION
**			with the operations required to extend
**			a Database to that location.
**
**			Editorial aside: Should be replaced by
**	 		new SQL syntax,
**
**			    EXTEND <dbname> TO LOCATION <loc_name>
**			        [WITH USAGE = (usage...)]
**
** Input params:
**			-loc=location	Location name.
**			-area=directory	Directory to create location.
**			dbname|-nodb	List of database names or
**						-nodb for no databases.
**			-usage=data,ckp,jnl,dmp,work|awork
**					The usage type for the location
**			-alter		Flag to alter existing location.
**			-drop		Flag to drop existing location.
**			-rawpct=	Percent of raw area to allocate
**					to location, 0 or omitted 
**					if cooked.
**							
**
** Output params:
**
** Returns:
**
** Exceptions:
**
** Side Effects:
**
** History:
**
**	10-jun-1999 (hanch04)
**	    Created.
**	14-mar-2000 (hanch04)
**	    Copy newarea to mkdir area allow the appending of ingres/default
**	    Alter should not add usages, it should make the usages only
**	    what is specified.
**	28-mar-2000 (hanch04)
**	    Select location name in char array, not into pointer.
**	14-jul-2000 (hanch04)
**	    Don't call EXdelete if EXdeclare has not been called.
**	    bug 102113
**	14-jul-2000 (hanch04)
**	    Check to see if a database is already extended to a location.
**	    bug 102114
**      12-jun-2000 (gupsh01)
**          Added static function check_database to check that the database
**          exists.
**	17-nov-2000 (gupsh01)
**	    Corrected the database index referenced for the check_database 
**	   cfunction call.
**	09-Apr-2001 (jenjo02)
**	    Added support for raw locations.
**	08-May-2001 (jenjo02)
**	    Don't prompt for rawblocks, accept 0 to mean cooked.
**	11-May-2001 (jenjo02)
**	    "rawblocks" moved from iilocations to iiextend, "rawpct"
**	    in iilocations replaces "rawblocks" as input field.
**	23-May-2001 (jenjo02)
**	  o Replaced chk_loc_area() functionality with new internal
**	    procedure, iiQEF_check_loc_area, to allow privileged
**	    non-Ingres users to run this utility, although only
**	    Ingres can use the -mkdir flag, for now.
**	  o Check iiuser for SysAdmin privileges; if none, can't
**	    run this utility.
**	  o Remove check that user must be owner of database.
**	01-Oct-2001 (inifa01) bug 105932 INGDBA 98
**	    extenddb -loc=locname -area=directory -usage={data,ckp,jnl,dmp,work|
**	    awork} -mkdir -nodb fails with SIGBUS and core dump in LOfroms().
**	    Allocated memory for buffer; Loc_usages[i].dir. 
**	15-Oct-2001 (jenjo02)
**	    Area directories are now created by the server during
**	    CREATE/ALTER LOCATION, removed -mkdir flag.
**	24-may-2002 (abbjo03)
**	    Change 1-oct-01 fix: increase dir to allow for null terminators.
**	31-May-2002 (gupsh01)
**	    Added check for -U flag when obtaining the usage for the 
**	    extended location. 
**	27-Aug-2002 (inifa01) Sir 108589.
**	    Running extenddb as; extenddb -lexistent_locname -aexistent_loc 
**	    -U{...} -alter -r0 -nodb, for example fails b/cos -a and -r should
**	    not be specified on an existent location.  Made error message more 
**	    specific as to reason for failure.  
**      07-May-2004 (jammo01) prob INGSRV 2784 bug 112115
**         Runnning extenddb as; extenddb -lwk02 -a/ii_work/I3/02 -Uwork -r0 
**	   -nodb along with the work location, the data location is also
**	   created. Replaced DU_DBS_LOC with 0 in the code line: 
**	   linfo->li_usage = DU_DBS_LOC;
**	15-May-2005 (hanje04)
**	    GLOBALREF Iidatabase and Iiuser as they're GLOBALDEF's in
**	    ingdata.sc and Mac OS X doesn't like multiple defn's.
*/

/*
**	MKMING Hints
**
PROGRAM =	extenddb

NEEDLIBS =	INGCNTRLLIB \
		GNLIB UFLIB RUNSYSLIB RUNTIMELIB FDLIB FTLIB FEDSLIB \
		UILIB LIBQSYSLIB LIBQLIB UGLIB FMTLIB AFELIB \
		LIBQGCALIB CUFLIB GCFLIB ADFLIB \
		COMPATLIB MALLOCLIB

UNDEFS =	II_copyright
*/

GLOBALREF char  *Real_username;
GLOBALREF bool	IiicPrivSysadmin; /* TRUE if user has SYSTEM_ADMIN privilege */

GLOBALREF	IIUSER 		Iiuser;
GLOBALREF	IIDATABASE 	Iidatabase;

#define MAX_DBNAMES    256

/*}
** Name:        LOC_USAGE - Location usage.
*/
typedef struct{
        i4             id;     /* internal flag value of usage */
        char           *name;  /* CREATE/ALTER LOCATION usage keyword */
        char           *usage; /* -usage input value */
        char           dir[6];   /* LO's equivalent value */
} LOC_USAGE;

static LOC_USAGE Loc_usages[] =
{
        {DU_DBS_LOC,     ERx("DATABASE"),	ERx("data"),	LO_DB   },
        {DU_JNLS_LOC,    ERx("JOURNAL"),	ERx("jnl"),	LO_JNL  },
        {DU_CKPS_LOC,    ERx("CHECKPOINT"),	ERx("ckp"),	LO_CKP  },
        {DU_DMPS_LOC,    ERx("DUMP"),		ERx("dmp"),	LO_DMP  },
        {DU_WORK_LOC,    ERx("WORK"),		ERx("work"),	LO_WORK },
        {DU_AWORK_LOC,   ERx("WORK"),		ERx("awork"),	LO_WORK },
        {0,    NULL}
};

EXEC SQL INCLUDE SQLCA ;
EXEC SQL WHENEVER SQLERROR CALL SQLPRINT ;

EXEC SQL BEGIN DECLARE SECTION;
typedef struct {
		char	li_area[DB_AREA_MAX+1];	/* iilocations.area */
		i4	li_exists;	/* TRUE if location exists */
		i4	li_usage;	/* iilocations.status */
		i4	li_rawpct;	/* iilocations.rawpct */
} LOC_INFO;
EXEC SQL END DECLARE SECTION;

static STATUS check_loc_exists(char *locname, LOC_INFO *linfo);

i4
main(argc, argv)
i4	argc;
char	**argv;
{
    STATUS	status;
    i4		    flagWord;
    i4		    alterloc=FALSE;
    i4		    droploc=FALSE;
    i4		    getdbnames=TRUE;

EXEC SQL BEGIN DECLARE SECTION;
    char	*locname= ERx("");
    char        *database_arr[MAX_DBNAMES];
    i4		dbloc_stat=0;
    i4		exusage;
    i4		stat;
    char	sbuffer[1024];
    char	usagebuf[512];
    LOC_INFO	loc_info, *linfo = (LOC_INFO*)&loc_info;
EXEC SQL END DECLARE SECTION;

    char	*usage= ERx("");
    char	*usagelist= ERx("");
    char	*next= ERx("");
    char	*loc_path= ERx("");

    i4          max_dbs = MAX_DBNAMES;
    char        **database;
    i4		numdbs;
    i4		i,j;

    char	*progname;
    char	*dbname = ERx("iidbdb");
    char        *user = ERx("");
    char        *password = ERx("");
    char        *groupid = ERx("");

    CL_ERR_DESC sys_err;
    i4  	err_code, lo_status;
    EX_CONTEXT	context;
    EX		FEhandler();
    char	errmsg[256];

    char	Ingres_uname[FE_MAXNAME+1];
    char	temp_name[FE_MAXNAME+1];
    
    /* Tell EX this is an ingres tool. */
    (void) EXsetclient(EX_INGRES_TOOL);

    MEadvise(ME_INGRES_ALLOC);

    /* Call IIUGinit to initialize character set attribute table */
    if ( IIUGinit() != OK)
	PCexit(FAIL);

    progname = ERget(F_IC0071_EXTENDDB);
    FEcopyright(progname, ERx("1999"));

    /*
    ** Get arguments from command line
    **
    ** This block of code retrieves the parameters from the command
    ** line.  The required parameters are retrieved first and the
    ** procedure returns if they are not.
    ** The optional parameters are then retrieved.
    */
    {
	ARGRET	rarg;
	i4	pos;
	i4	fmode;

	if (FEutaopen(argc, argv, ERx("extenddb")) != OK)
	    PCexit(FAIL);

        if (FEutaget(ERx("password"), 0, FARG_FAIL, &rarg, &pos) == OK)
        {
            char *IIUIpassword();

            if ((password = IIUIpassword(ERx("-P"))) == NULL)
            {
                FEutaerr(BADARG, 1, ERx(""));
                PCexit(FAIL);
            }
        }

        if (FEutaget(ERx("user"), 0, FARG_FAIL, &rarg, &pos) == OK)
            user = rarg.dat.name;

	/* Connect to IIDBDB, AUTOCOMMIT=ON */
	err_code = FEningres(NULL, (i4) 0, dbname, user, password, (char *)NULL);
	if (err_code != OK)
	{
	    if (err_code == 1)
	    {
                IIUGerr(E_IC0035_Cannot_start_INGRES, UG_ERR_FATAL, 0);
                /*NOTREACHED*/
	    }
	    else if (err_code == 2)
	    {
                IIUGerr(E_IC0036_no_authority, UG_ERR_FATAL, 0);
                /*NOTREACHED*/
	    }
	    else
	    {
                IIUGerr(E_IC0061_Other_startup_err, UG_ERR_FATAL, 1,
                        (PTR) &err_code );
                /*NOTREACHED*/
	    }
	}

	if ( ! IIUIdcn_ingres() )
	{
	    IIUGerr(E_IC0060_Not_INGRES_DBMS, UG_ERR_FATAL, 0);
	    /*NOTREACHED*/
	}

	/* Back-end Version Compatability check */
	if (STcompare(IIUIscl_StdCatLevel(), UI_LEVEL_65) < 0)
	{
	    IIUGerr(E_IC0067_BadIngresVersion, UG_ERR_FATAL, 2,
		    (PTR) ERx("II2.6"), (PTR) IIUIscl_StdCatLevel());
	    /* NOTREACHED */
	}

	/*
	** Extenddb allows -u flag, so the DBMS might think that the username
	** is the Effective user (rather than the Real user). So, must find
	** Real user and check what privs they have.
	*/
	IDname(&Real_username);     /* find Real user */

	/*
	** Make sure we have privilieges from default set active
	*/
	exec sql set session with privileges=all;

	/*
	** Set up to handle delimited user id's, but only if it contains
	** embedded spaces or starts with a numeric character.
	*/  

	if ((STindex(Real_username, ERx(" "), 0) != NULL) ||
	    (CMdigit(Real_username)))
	{
	    STpolycat(3, ERx("\""), Real_username, ERx("\""), temp_name);
	    STcopy(temp_name, Real_username);
	}

	/* select user info into the 'Iiuser' structure */
	if (((stat = iiicsuSelectUser(Real_username, &Iiuser)) == OK) &&
		(FEinqrows() > 0))
	{
	    if ( Iiuser.status & U_SYSADMIN )
		IiicPrivSysadmin = TRUE;
	    else
	    {
		/* user is trying to run Extenddb without SYSADMIN priv */

		FEing_exit();
		IIUGerr(E_IC007B_Insufficient_Privs, UG_ERR_FATAL, 1, Real_username);
		/*NOTREACHED*/
	    }
	}
	else
	{
	    IIUGerr(E_IC0073_GetPrivsError, UG_ERR_FATAL, 0);
	    /* NOTREACHED */
	}

	/* Get location name */
	if (FEutaget(ERx("locname"), 0, FARG_PROMPT, &rarg, &pos) != OK)
	    PCexit(FAIL);
	locname= rarg.dat.name;

	/* "Drop" location? */
	if (FEutaget(ERx("drop"), 0, FARG_FAIL, &rarg, &pos) == OK)
	{
	    droploc=TRUE;
	    getdbnames=FALSE;
	}

	if (FEutaget(ERx("alter"), 0, FARG_FAIL, &rarg, &pos) == OK)
	    alterloc = TRUE;

	/* Lookup location name in iilocations */
	if( check_loc_exists(locname, linfo) == OK )
	{
	    if ( linfo->li_exists )
	    {
		/* Location exists - can't specify "area" or "rawpct" */
		if ( FEutaget(ERx("area"), 0, FARG_FAIL, &rarg, &pos) == OK ||
		     FEutaget(ERx("rawpct"), 0, FARG_FAIL, &rarg, &pos) == OK )
		{
		    IIUGerr(E_IC0171_Invalid_Usage, UG_ERR_ERROR, 0);
		    PCexit(FAIL);
		}
	    }
	    else
	    {
		/* Location does not exist */

		if ( droploc || alterloc )
		{
		    IIUGerr(E_IC0167_Nonexist_Loc,
			    UG_ERR_ERROR, 1, locname);
		    PCexit(FAIL);
		}

		/* Must supply "area" */
		if ( FEutaget(ERx("area"), 0, FARG_PROMPT, &rarg, &pos) )
		    PCexit(FAIL);

		STcopy(rarg.dat.name, linfo->li_area);

		/* Prompt for RAWPCT */
		linfo->li_rawpct = 0;
		if ( FEutaget(ERx("rawpct"), 0, FARG_PROMPT, &rarg, &pos) == OK )
		{
		    linfo->li_rawpct = rarg.dat.num;

		    /* Must be in range 0-100 */
		    if ( linfo->li_rawpct < 0 ||
			 linfo->li_rawpct > 100 )
		    {
			IIUGerr(E_IC0168_Bad_Rawpct, UG_ERR_ERROR,
				    1, linfo->li_rawpct);
			PCexit(FAIL);
		    }
		}
	    }
	}
	else
	{
	    IIUGerr(E_IC0045_Error_retrieving_area,
		    UG_ERR_ERROR, 1, locname);
	    PCexit(FAIL);
	}

	if (FEutaget(ERx("nodb"), 0, FARG_FAIL, &rarg, &pos) == OK)
	{
	    getdbnames = FALSE;
	}

	if (FEutaget(ERx("database"), 0, FARG_FAIL, &rarg, &pos) == OK )
	    fmode = FARG_FAIL;
	else
	    fmode = FARG_OPROMPT;

	if ( alterloc )
	{
	    if( fmode == FARG_FAIL )
	    {
		IIUGerr(E_IC0165_Alter_With_DBname, UG_ERR_ERROR );
		PCexit(FAIL);
	    }
	    getdbnames = FALSE;
	}

        /* get database names, if raw, can only be one */
        database = &(database_arr[0]);
        numdbs = 0;
	while ( getdbnames &&
		FEutaget(ERx("database"), numdbs, fmode, &rarg, &pos) == OK &&
		*rarg.dat.name != EOS )
        {
	    PTR             pointer;
	    PTR             oldarray;
	    PTR             newarray;
	    u_i4	    size;
	    u_i4	    oldsize;
	    u_i2            cpsize;

	    database[numdbs++] = rarg.dat.name;

            /*
            **  Check to see if we need to allocate a bigger array.
            **  We do it here so that we don't have to do another
            **  check after exiting this loop (due to null termination
            **  of array after we exit).
            */
	    if (numdbs >= max_dbs)
            {
		oldsize = sizeof(char *) * max_dbs;
		size = oldsize + oldsize;
		if ((pointer = (PTR) MEreqmem((u_i4) 0, size,
					      (bool) TRUE, NULL)) == NULL)
		{
                    /*
                    **  Couldn't allocate more memory.  Give
                    **  error and exit.
                    */
		    IIUGbmaBadMemoryAllocation(ERx("main()"));
		    PCexit(FAIL);
		}
		newarray = pointer;
		oldarray = (PTR) database;

		/*
		**  Now copy old array to new array.
		*/
		while(oldsize > 0)
		{
		    cpsize = (oldsize <= MAXI2) ? oldsize : MAXI2;
		    MEcopy(oldarray, cpsize, newarray);
		    oldsize -= cpsize;
		    oldarray = (PTR) ((char *)oldarray + cpsize);
		    newarray = (PTR) ((char *)newarray + cpsize);
		}

		/*
		**  Free old array only if was dynamically allocated.
		*/
		if (database != &(database_arr[0]))
		{
		    MEfree((PTR)database);
		}

		/*
		**  Set new count for maximum databases that
		**  array can hold and reassign pointers.
		*/
		max_dbs += max_dbs;
		database = (char **) pointer;
            }

	    /* Check that database exists */
	    if ( iiicsdSelectDatabase(database[numdbs-1], &Iidatabase) )
                PCexit(FAIL);
	}

	/* terminate the list of databases */
	database[numdbs] = NULL;

	/* Error if raw and more than one database */
	if ( linfo->li_rawpct && numdbs > 1 )
	{
	    IIUGerr(E_IC0170_Raw_Extend, UG_ERR_ERROR );
	    PCexit(FAIL);
	}

	if ( !droploc )
	{
	    if ( FEutaget(ERx("dbusage"), 0, FARG_FAIL, &rarg, &pos) )
	    {
	      if ( FEutaget(ERx("lusage"), 0, FARG_FAIL, &rarg, &pos) )
	      {
		if ( numdbs == 0 )
		{
		    if ( FEutaget(ERx("lusage"), 0, FARG_PROMPT, &rarg, &pos) )
			PCexit(FAIL);
		}
		else
		{
		    if ( FEutaget(ERx("dbusage"), 0, FARG_PROMPT, &rarg, &pos) )
			PCexit(FAIL);
		}
	      }
	    }
	    usagelist= rarg.dat.name;
	}

	FEutaclose();
    }

    /* Parse the -usage list */
    for (usage= usagelist; usage!= NULL; usage = next)
    {
	next = STindex(usage,ERx(","),0);
	if (next != NULL)
	{
	    *next = EOS;
	    next++;
	}
	if (*usage!= EOS)
	{
	    i4	found_usage = FALSE;

	    for (i = 0, usagebuf[0] = EOS; Loc_usages[i].name != NULL; i++)
	    {
		if (STcompare(Loc_usages[i].usage, usage) == 0)
		{
		    dbloc_stat |= Loc_usages[i].id;
		    found_usage = TRUE;
		    break;
		}
	    }
	    if ( !found_usage )
	    {
		IIUGerr(E_IC0161_Invalid_Usage, UG_ERR_ERROR,
			1, usage);
		PCexit(FAIL);
	    }
	}
    }

    /* linfo->li_usage holds current or default usages */

    if (( dbloc_stat & DU_WORK_LOC ) && ( dbloc_stat & DU_AWORK_LOC ) )
    {
	IIUGerr(E_IC0162_Loc_Work_And_Work_Aux, UG_ERR_ERROR, 0 );
	PCexit(FAIL);
    }

    if ( ( (numdbs > 0) && (( dbloc_stat & DU_CKPS_LOC ) ||
			   ( dbloc_stat & DU_JNLS_LOC ) ||
			   ( dbloc_stat & DU_DMPS_LOC) ) ) )
    {
	IIUGerr(E_IC0163_Cant_Extend_Exist_DB, UG_ERR_ERROR, 0 );
	PCexit(FAIL);
    }

    if ( linfo->li_exists && !alterloc && numdbs == 0 && !droploc )
    {
	IIUGerr(E_IC0164_Alter_Flag_Needed, UG_ERR_ERROR, 0 );
	PCexit(FAIL);
    }

    /* Any new usages? */
    if ( (linfo->li_usage | dbloc_stat) != linfo->li_usage )
    {
	linfo->li_usage |= dbloc_stat;
	if ( linfo->li_exists )
	    alterloc = TRUE;
    }
    else if ( linfo->li_exists )
	/* Nothing to alter */
	alterloc = FALSE;

    if (EXdeclare(FEhandler, &context) != OK)
	PCexit(FAIL);

    if ( linfo->li_exists == FALSE || alterloc || droploc )
    {
	if ( droploc )
	    STprintf(sbuffer, ERx("DROP LOCATION %s "), locname );
	else
	{
	    /* Create or Alter: */

	    /* build comma-separated list of LOC_USAGEs */
	    for (i = 0, usagebuf[0] = EOS; Loc_usages[i].name != NULL; i++)
	    {
		if (linfo->li_usage & Loc_usages[i].id)
		{
		    if (usagebuf[0] != EOS)
			STcat(usagebuf, ERx(",")); /*don't do this for first one*/

		    STcat(usagebuf, Loc_usages[i].name);
		}
	    }

	    if ( alterloc )
	    {
		/* ALTER existing location */
		STprintf(sbuffer, ERx("ALTER LOCATION %s WITH "), locname);
	    }
	    else
	    {
		STprintf(sbuffer, ERx("CREATE LOCATION %s WITH AREA = '%s'"),
			 locname, linfo->li_area);

		if ( linfo->li_rawpct )
		{
		    char	crawpct[16];
		    STprintf(crawpct, ERx(", RAWPCT = %d"), linfo->li_rawpct);
		    STcat(sbuffer, crawpct);
		}
		STcat(sbuffer, ERx(","));
	    }

	    STcat(sbuffer, ERx(" USAGE = ("));
	    if (usagebuf[0] != EOS)
		STcat(sbuffer, usagebuf);
	    else
		STcat(sbuffer, ERx("NOUSAGE"));
	    STcat(sbuffer, ERx(")"));
	}

	EXEC SQL EXECUTE IMMEDIATE :sbuffer;

	if (sqlca.sqlcode < 0 && sqlca.sqlcode != (-E_GE0032_WARNING))
	{
	    EXdelete();
	    PCexit(FAIL);
	}
	EXEC SQL COMMIT WORK;
    }

    /* Now extend location to one or more databases */
    for( j = 0 ; numdbs && j < numdbs ; j++ )
    {
	for (i = 0; Loc_usages[i].name != NULL; i++)
	{
	    if (dbloc_stat & Loc_usages[i].id)
	    {
		/* iiQEF_add_location checks for already extended */
		exusage = Loc_usages[i].id;

		EXEC SQL EXECUTE PROCEDURE iiQEF_add_location
		    ( database_name = :database_arr[j],
		      location_name = :locname,
		      access = :exusage,
		      need_dbdir_flg = 1 );

		if ( sqlca.sqlcode < 0 &&
		     sqlca.sqlcode != (-E_GE0032_WARNING) )
		    IIUGerr(E_IC0046_Errors_extending_db, UG_ERR_ERROR,
			    2, database_arr[j], locname);
	    }
	}
    }

    if ( numdbs )
	EXEC SQL COMMIT WORK;

    FEing_exit();

    EXdelete();

    PCexit(OK);
}

/* check_loc_exists -- check a location name for validity. */

static STATUS
check_loc_exists(locname, linfo)
     EXEC SQL BEGIN DECLARE SECTION;
     char                  *locname;
     LOC_INFO		   *linfo;
     EXEC SQL END DECLARE SECTION;
{
    STATUS          stat;

    /* make sure location name is legal */
    if (STtrmwhite(locname) <= 0 ||
	(IIUGdlm_ChkdlmBEobject(locname,NULL,FALSE)==UI_BOGUS_ID))
    {
	i4     maxn = DB_MAXNAME;

	IIUGerr(E_IC002E_bad_name, 0, 2, (PTR) locname, (PTR)&maxn);
	EXdelete();
	PCexit(FAIL);
    }
    IIUGlbo_lowerBEobject(locname);

    /* lookup location name */
    EXEC SQL SELECT location_area, status, raw_pct
	INTO :linfo->li_area, :linfo->li_usage, :linfo->li_rawpct
	FROM iilocation_info
	WHERE location_name = :locname;

    if ((stat = FEinqerr()) != OK)
	return stat;
    if (FEinqrows() > 0)
	linfo->li_exists = TRUE;
    else
    {
	linfo->li_exists = FALSE;

	/* default status information */
	*linfo->li_area = EOS;
	linfo->li_usage = 0;
	linfo->li_rawpct   = 0;
    }
    return OK;
}
