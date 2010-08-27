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
** Name:    unexdbmain.sc - main procedure for UNEXTENDDB.
**
** Description:
**	This file defines:
**
**	main		main procedure for UNEXTENDDB utility
**			which includes the operations required 
**			to unextend a database from a location.
**
** Input params:
**			-l loc=location	Location name.
**			dbname	List of database names 
**			-u user= Specifies the effective user for the session. 
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
**	15-Apr-2004 (gorvi01)
**	    Created for Jupiter.
**	10-Jun-2004 (gorvi01)
**	    Modified check_loc_exists() for BUG# 112426. The modification 
**	    includes check of stat during lookup of locations. Added the 
**	    status information in the same function.
**	10-Sep-2004 (gorvi01)
**	    Modified usages for all types of location instead of only work(16).
**	    Created for Jupiter
**	11-Nov-2004 (gorvi01)
**	    Modified error message of unextenddb failure. New error message
**	    of E_IC004C_Errors_unextending_db added. 
**	15-May-2005 (hanje04)
**	    GLOBALREF Iidatabase and Iiuser as they're GLOBALDEF's in
**	    ingdata.sc and Mac OS X doesn't like multiple defn's.
**      17-Jun-2010 (coomi01) b123913
**          Add new error message E_IC0052 when an attempt to delete
**          last remaining work location for a database.
*/

/*
**	MKMING Hints
**
PROGRAM =	unextenddb

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
		i4  li_deleted; /* TRUE if location deleted */
		i4	li_usage;	/* iilocations.status */		
} LOC_INFO;
EXEC SQL END DECLARE SECTION;

static STATUS check_loc_exists(char *locname, LOC_INFO *linfo);

i4
main(i4	argc,char **argv)
{
    STATUS	status;
    i4		    flagWord;
    i4		    getdbnames=TRUE;

EXEC SQL BEGIN DECLARE SECTION;
    char	*locname= ERx("");
    char        *database_arr[MAX_DBNAMES];
    i4		dbloc_stat=0;
    i4		exusage;
    i4		stat;
    i4          count;
    i4          workArea = (DU_EXT_OPERATIVE|DU_EXT_WORK);
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
    bool        workCheckFailed;

    /* Tell EX this is an ingres tool. */
    (void) EXsetclient(EX_INGRES_TOOL);

    MEadvise(ME_INGRES_ALLOC);

    /* Call IIUGinit to initialize character set attribute table */
    if ( IIUGinit() != OK)
	PCexit(FAIL);

    progname = ERget(F_IC0072_UNEXTENDDB);
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
	if (FEutaopen(argc, argv, ERx("unextenddb")) != OK)
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
	** Unextenddb allows -u flag, so the DBMS might think that the username
	** is the effective user (rather than the real user). So, must find
	** real user and check what priviledges they have.
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
			/* user is trying to run Unextenddb without SYSADMIN priv */

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

	/* Lookup location name in iilocations */
	if( check_loc_exists(locname, linfo) == OK )
	{
	    if ( linfo->li_exists )
	    {
			if (FEutaget(ERx("database"), 0, FARG_FAIL, &rarg, &pos) == OK )
			    fmode = FARG_FAIL;
			else
				fmode = FARG_OPROMPT;

			/* get database names, if raw, can only be one */
			database = &(database_arr[0]);
			numdbs = 0;
			while ( getdbnames &&
				FEutaget(ERx("database"), numdbs, fmode, &rarg, &pos) == OK &&
				*rarg.dat.name != EOS )
			{
			    database[numdbs++] = rarg.dat.name;
		
				/* Check that database exists */
				if ( iiicsdSelectDatabase(database[numdbs-1], &Iidatabase) )
					PCexit(FAIL);
			}

			/* terminate the list of databases */
			database[numdbs] = NULL;

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

			FEutaclose();
		
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

		/* Any new usages? */
			if ( (linfo->li_usage | dbloc_stat) != linfo->li_usage )
			{
				linfo->li_usage |= dbloc_stat;
			}

			if (EXdeclare(FEhandler, &context) != OK)
				PCexit(FAIL);

			if ( linfo->li_exists == FALSE )
			{
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
			}
	
			if (sqlca.sqlcode < 0 && sqlca.sqlcode != (-E_GE0032_WARNING))
			{
				EXdelete();
				PCexit(FAIL);
			}
			EXEC SQL COMMIT WORK;
		}
	    else
		{
			/* Location does not exist */
			IIUGerr(E_IC0167_Nonexist_Loc,
				UG_ERR_ERROR, 1, locname);
			PCexit(FAIL);
		}

	        /*
		** We are about to count work locations, before
		** deleting some. We want to ensure no other process
		** undermines the count before we get to the delete stage.
		*/
	        EXEC SQL COMMIT WORK;
	        EXEC SQL SET AUTOCOMMIT OFF;
		EXEC SQL SET LOCKMODE SESSION 
			     WHERE LEVEL    = TABLE, 
                                   READLOCK = EXCLUSIVE, 
                                   TIMEOUT  = 10;  

		/*
		** First walk over all deletions and make sure
		** none will leave our database(s) without at least
		** one work location each.
		*/
	        workCheckFailed = FALSE;
		for( j = 0 ; (!workCheckFailed) && numdbs && j < numdbs ; j++ )
		{
			for (i = 0; Loc_usages[i].name != NULL; i++)
			{
			    /* 
			    ** Checkup on work locations only 
			    */
			    if ((dbloc_stat & DU_WORK_LOC) && (dbloc_stat & Loc_usages[i].id))
			    {
				/* Count work locations OTHER than
				** the one we hope to remove
				*/
				exec sql SELECT count(*) into :count 
				         FROM   iiextend_info 
				         WHERE  status = :workArea
				         AND    database_name = :database_arr[j]
				         AND    location_name != :locname;

				if ( sqlca.sqlcode < 0 &&
				     sqlca.sqlcode != (-E_GE0032_WARNING) )
				{
				    workCheckFailed = TRUE;

				    /* 
				    ** On SQL failure proceed no further with loops
				    */
				    break;
				}


				/* 
				** Produce an error report  for each problem encountered
				*/
				if ( 0 == count )
				{
				    IIUGerr(E_IC0052_Error_last_db_workloc,    
					    UG_ERR_ERROR,
					    2, database_arr[j], locname);

				    workCheckFailed = TRUE;
				}

			    }
			}			
		}

		/* 
		** Do not proceed on request if a problem was detected.
		*/
		if (!workCheckFailed)
		{

		    /* The lock is still there, 
		    ** Now unextend location from one or more databases 
		    */
		    for( j = 0 ; numdbs && j < numdbs ; j++ )
		    {
			for (i = 0; Loc_usages[i].name != NULL; i++)
			{
				if (dbloc_stat & Loc_usages[i].id)
				{
					exusage = Loc_usages[i].id;

					EXEC SQL EXECUTE PROCEDURE iiQEF_del_location
					( database_name = :database_arr[j],
						location_name = :locname,
						access = :exusage,
						need_dbdir_flg = 1 );
		
					if ( sqlca.sqlcode < 0 &&
						sqlca.sqlcode != (-E_GE0032_WARNING) )
					{
					    IIUGerr(E_IC004C_Errors_unextending_db, UG_ERR_ERROR,
						    2, database_arr[j], locname);
					}
				}
			}
		    }
		}

	} /* If loc exists */
	else
	{
	    IIUGerr(E_IC0045_Error_retrieving_area,
			UG_ERR_ERROR, 1, locname);

		PCexit(FAIL);
	}
    }

    if ( numdbs )
    {
	EXEC SQL COMMIT WORK;
    }

    FEing_exit();

    EXdelete();

    if (workCheckFailed)
        PCexit(FAIL);
    else
	PCexit(OK);
}

/* check_loc_exists -- check a location name for validity. */

static STATUS
check_loc_exists(char *locname, LOC_INFO *linfo)
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
    EXEC SQL SELECT location_area, status
	INTO :linfo->li_area, :linfo->li_usage
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
	        linfo->li_usage = DU_DBS_LOC;
    }
    return OK;
}
