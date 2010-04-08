/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <duf.h>
#include    <er.h>
#include    <lo.h>
#include    <si.h>
#include    <ex.h>
#include    <me.h>
#include    <duerr.h>
#include    <duvfiles.h>
#include    <duve.h>
#include    <dudbms.h>
#include    <generr.h>
/* 
MODE =		SETUID

NEEDLIBS =	DBUTILLIB SQLCALIB LIBQLIB LIBQGCALIB GCFLIB \
		UGLIB FMTLIB AFELIB ADFLIB ULFLIB CUFLIB COMPATLIB MALLOCLIB

OWNER =		INGUSER

PROGRAM =	verifydb
*/


/**
**
**  Name: DUVERIFY.C - main driver for verifydb
**
**  Description:
**      Verifydb is a utility that operates on databases to assure their 
**      consistency.  Verifydb performs the following functions: 
**          - Force an inconsistent database to appear consistent to the dbms
**	    - Perform checks (and optionally associated repairs) on dbms catalogs
**	    - Perform checks (and optionally associated repairs) on a table
**	    - Purge temporary and/or exired files from a database.
**
**	verifydb is evoked with the following parameters:
**	    -m<ode> -s<cope> -o<peration> -u<ser> -<special flags>
**	Explanation of verifydb parameters:
**	    -mode : specifies the mode in which \f3verifydb\f1 runs.  Use this 
**	            mandatory flag with one of the following qualifiers:
**		report - verifydb will not perform any actions.  It will merely
**			 diagnose and report what actions should be done in any
**			 of the run modes
**		run    - verifydb will perform the actions and notify user of 
**			 what actions it performed
**		runsilent - verifydb will perform actions, but will not notify
**			 the user.
**		runinteractive - verifydb will determine the action to take, 
**			 then prompt the user for permission before taking that
**			 action.
**	    -scope : specifies the scope of the verifydb action.  This flag is
**		     mandatory.
**		dbname  - verifydb operates on the databases specified on the
**			  input line. (NOTE: all databases specified in this
**			  list must be owned by the same user)
**	        dba     - Operates on all databases for which the user is the 
**			  database administrator.  Or an INGRES super user may 
**			  use this qualifier with the -user flag.
**		installation - Operates on all of an installation's local
**			  databases.
**	    -operation : specifies the verifydb operation to be performed.  This
**		         mandatory flag has the following qualifiers:
**		dbms_catalogs - dbms system catalogs is checked and, if any of 
**				the run f1 qualifiers are selected, corrected. 
**		force_consistent - make inconsistent database look consistent
**				to the dbms
**		purge - purge all temporary or expired files
**		temp_perge - purge all temporary files
**		expired_purge - purge all expired files
**		table - check internal storage structure of a specified table.
**			if any of the runs are specified, correct any problems
**			in table (forcing it to a heap)
**		xtable - same as table, but uses stricter algorythm.
**		drop_tbl - drop a table from the system catalogs without 
**		        checking or dropping the disk files associated with
**			the table. [must specify table name, as well]
**		accesscheck - verify it is possible to connect to the dbs in
**			the scopelist.
**		refresh_ldbs - assure that DDB's iiddb_ldb_dbcaps catalog
**			reflects the correct level for level specific capabilites
**			from the LDB's iidbcapabilities.
**	    -user : Used by an INGRES system administrator or an INGRES super 
**		    user to enter the database as a specific INGRES user
**	    -debug : Cause informative messages to be output to screen for
**		     debugging purposes
**	    -rel : run only the portion of the system catalog checks that
**		   clears the optimizer statistics information
**	    -nolog : skip opening and writting to the verifydb log file
**		    (not recommended)
**
**          main - main verifydb driver
**	    duve_issue_legal_warning - issue any legal warnings for verifydb
**          duve_exit - exit routine
**          duve_exception - verifydb exception handeler
**          duve_ingerr - determine if INGRES error is fatal or recoverable.
**
**  History:
**      2-aug-88 (teg)
**          Initial creation.
**      25-aug-88 (teg)
**          bring up to RELATIONAL TECHNOLOGY coding standards
**	2-Nov-88 (teg)
**	    added drop_tbl operation
**	26-jun-89 (dds)
**	    Added ULFLIB to ming hints.
**      14-Sep-89 (teg)
**	    add table and xtable operations.
**	5-mar-1990 (teg)
**	    change duve_ingerr to not abort on snytax errors from dropping an
**	    expired table.
**	28-mar-1990 (teg)
**	    added logic to support accesscheck operation
**      20-nov-92 (bonobo)
**          Updated MING hint to include CUFLIB.
**	17-jul-93 (teresa)
**	    added support for refresh_ldbs
**	8-aug-93 (ed)
**	    unnest dbms.h
**	04-apr-1996 (canor01)
**	    For -oFORCE_CONSISTENT, 1) assure that we use
**	    duvecb.duve_list->du_dblist rather than du_scopelst and 2) walk
**	    all the databases in the list.
**	13-sep-96 (nick)
**	    Tell EX we are an Ingres tool ( and add in above comment which
**	    Orlan missed ).
**	19-dec-96 (chech02)
**	    changed GLOBALDEF to GLOBALREF.
**      21-apr-1999 (hanch04)
**	    Added st.h
**      10-Jul-2008 (hanal04) Bug 120615
**          If we just processed the IIDBDB database PCsleep() before
**          connecting to another database to avoid lock timeout on
**          iidbdb lookup for the next database. Disconnects are asynchronous
**          a must be given time to complete if a subsequent connect might
**          conflict.
**	13-Nov-2009 (kschendel) SIR 122882
**	    cmptlvl is now interpreted as an integer.
**/

/*
**  Forward and/or External function references.
*/

FUNC_EXTERN DU_STATUS duve_catalogs();  /* check dbms system catalogs */
FUNC_EXTERN DU_STATUS duve_tbldrp();	/* drop specified table from catalogs */
FUNC_EXTERN VOID    duve_close();	/* close open database, if any */
FUNC_EXTERN VOID    duve_exit();        /* exit verifydb cleanly */
FUNC_EXTERN DU_STATUS duve_fclose();	/* close verifydb log file */
FUNC_EXTERN DU_STATUS duve_force();     /* force database consistent */
FUNC_EXTERN DU_STATUS duve_get_args();  /* parse input command line */
FUNC_EXTERN DU_STATUS duve_init();      /* initialize verifydb */
FUNC_EXTERN DU_STATUS duve_issue_legal_warning(); /* verifydb legal warnings */
FUNC_EXTERN DU_STATUS duve_talk();      /* verifydb communications to user */
FUNC_EXTERN DU_STATUS duve_patchtable();/* patch a table's disk file */
/*
** Definition of all global variables owned by this file.
*/
GLOBALREF DUVE_CB	duvecb;	/* verifydb control block */
GLOBALREF i4		dbdbidx; /* du_dblist[] entry for iidbdb */

/*{
** Name: verifydb main	- main driver for verifydb
**
** Description:
**      executive for verifydb utility.  This routine causes verifydb
**	to be initialized, parses the input parameters, performed the
**	correct action on the specified database(s), then exits cleanly. 
**
** Inputs:
**      argv                            array of input parameter strings
**      argc                            number of input parameter strings
**
** Outputs:
**	none
**	Returns:
**	    exits via duve_exit() with status = OK
**	Exceptions:
**	    none.
**
** Side Effects:
**	    tables may disappear from user database
**	    verifydb cannot guarentee to recover all databases or tables
**	    verifydb log file is created if it does not already exist:
**		    ii_config:iivdb.log
**
** History:
**      2-aug-88 (teg)
**          Initial creation.
**      25-aug-88 (teg)
**          bring up to RELATIONAL TECHNOLOGY coding standards
**	2-Nov-88
**	    implemented drop_tbl operation
**	19-Jan-89 (teg)
**	    added legal warning to verifydb
**	14-sep-89 (teg)
**	    add table and xtable operation.
**	08-nov-89 (teg)
**	    removed restriction that marked Check Table as not implemented.
**	29-jan-90 (teg)
**	    zerofill duvecb before initializing to keep porting folks sane.
**	05-feb-90 (teg)
**	    added logic to check user authority before allowing a user into a db.
**	28-mar-1990 (teg)
**	    added logic to support accesscheck operation
**	29-mar-1990 (teg)
**	    added logic to check for DDB and refuse to operate on it rather
**	    than reporting error that DDB is unaccessable.  This fixes bug 20971.
**	  Also
**	    fix bug 20972  by 1) assuring that we use 
**	    duvecb.duve_list->du_dblist rather than du_scopelst and 2) assure
**	    that there are really databases in the duve_list before attempting
**	    to walk it.
**      21-apr-92 (teresa)
**          Reworked exception handling handle exceptions from within exceptions
**	    for bug 42660.
**	17-jul-93 (teresa)
**	    added support for refresh_ldbs
**	04-apr-1996 (canor01)
**	    For -oFORCE_CONSISTENT, 1) assure that we use
**	    duvecb.duve_list->du_dblist rather than du_scopelst and 2) walk
**	    all the databases in the list.
**	13-sep-96 (nick)
**	    Call EXsetclient() else we don't catch user interrupts.
[@history_template@]...
*/

main(argc, argv)
int    argc;
char   *argv[];

{
DU_ERROR	    duve_msg;		/* message control block */
i4		    err;		/* error value returned */
EX_CONTEXT	    context;
STATUS		    duve_exception();	/* verifydb exception handler */
i4		    index=0;		
i4		    cmptminor = DU_1CUR_DBCMPTMINOR;
i4		    cur_level = DU_CUR_DBCMPTLVL;
bool		    exception = FALSE;

    (void) EXsetclient(EX_INGRES_TOOL);

    MEadvise(ME_INGRES_ALLOC);

    /* Call IIUGinit to initialize CM attribute table */
    if( IIUGinit() != OK)
    {
        PCexit(FAIL);
    }

    if ( EXdeclare (duve_exception, &context) != OK)
    {
	EXdelete();			/* set up new exception to avoid
                                        ** recursive exceptions */
    	if ( EXdeclare(duve_exception, &context) != OK)
	{
	   /* if exception handler called again then do not do anything
	    ** except exit as quickly as possible*/
	    EXdelete();
	    duve_exit(&duvecb);
	} /* end of exception from within an exception */

	/* standard exception handling is to report the abort in error case,
	** (or user interrupt in user interrupt case) and then exit.  It is
	** possible that the error reporting routine could encounter another
	** exception, which will be caught by the newly declared exception
	** handler.  This (see above) will error exit without trying to output
	** any messages.
	*/
	exception = TRUE;
	switch (duvecb.du_exception_stat)
	{

	case DUVE_USR_INTERRUPT:
	    duve_talk(DUVE_ALWAYS,&duvecb, E_DU3600_INTERRUPT, 0);
	    break;
	case DUVE_EXCEPTION:
	default:
	    duve_talk(DUVE_ALWAYS,&duvecb, W_DU1010_UTIL_ABORT, 4,
			0, "Verifying", 0, &duvecb.du_opendb[0]);
	}  /* endcase */
    }   /* end of exception handling logic */

    for (;exception==FALSE;)	/* error handling loop */
    {
	MEfill ( (u_i2) sizeof(duvecb), '\0', (PTR) &duvecb);
	if ( duve_init (&duvecb, &duve_msg, argc, argv) != DUVE_YES)
	    break;

	if ( duve_get_args( argc, argv, &duvecb, &err) != DUVE_YES)
	    break;

        if ( duve_issue_legal_warning( &duvecb ) != DUVE_YES)
	    break;

	if ( duve_dbdb( &duvecb ) != DUVE_YES )
	    break;

	if (duvecb.duve_operation == DUVE_ACCESS)
	{
	    char	user[DB_MAXNAME+1];

	    /* assure that there atleast 1 valid database name in scopelist */
	    if (duvecb.duve_ncnt == 0)
		break;

	    for (index = 0; index < duvecb.duve_ncnt; index++)
	    {
		/* see if user has authority to check databases */
		if (ck_authority(&duvecb, index) == DUVE_NO)
		{
		    duve_talk( DUVE_ALWAYS, &duvecb, E_DU5032_NO_AUTHORITY,
				6, 0, duvecb.duveowner,
				0, duvecb.duve_list->du_dblist[index].du_db,
				0, duvecb.duve_list->du_dblist[index].du_dba);
		    continue;
		}
		if (ck_distributed(&duvecb, index) == DUVE_YES)
		    continue;

		/* figure out who verifydb should attempt to connect as.  The
		** Rule is:  
		**   1) If -au flag is specified, use it
		**   2) If -au not specified, but -u flag is specified, use it
		**   3) otherwise, use the login id of verifydb user.
		**  (REMEMBER the -au flag is undocumented)
		*/
		if (duvecb.duve_auser[0] != '\0')
		    STcopy(duvecb.duve_auser, user);
		else if (duvecb.duve_user[0] !='\0')
		    STcopy(duvecb.duve_user, user);
		else
		    STcopy(duvecb.duveowner, user);

		/* walk all dbs in scope and see if its possible to connect to
		** each db in list */
		duve_talk(DUVE_MODESENS, &duvecb, S_DU04CD_ACCESSCHECK_START, 4,
		  0, duvecb.duve_list->du_dblist[index].du_db, 0,user);
		if ( duve_hcheck(&duvecb, 
				 duvecb.duve_list->du_dblist[index].du_db,user) 
		    == DUVE_YES)
		    duve_talk(DUVE_MODESENS, &duvecb, S_DU04CE_ACCESSIBLE, 4,
				0, duvecb.duve_list->du_dblist[index].du_db,
				0,user);
		else
		    duve_talk(DUVE_MODESENS, &duvecb, S_DU04CF_INACCESSIBLE, 4,
				0, duvecb.duve_list->du_dblist[index].du_db,
				0,user);

                if(index == dbdbidx)
                {
                    /* Connection to the next DB may timeout on connection
                    ** to iidbdb because the disconnect is asynchronous.
                    ** Give the DBMS time to complete the disconnect.
                    */
                    PCsleep(500);
                }
	    } /* end loop for each DB in scope list */
	}

	else if (duvecb.duve_operation == DUVE_MKCONSIST)
	{   
	    /* assure that there really is a valid database to check */
	    if (duvecb.duve_ncnt == 0)
		break;
	    for (index = 0; index < duvecb.duve_ncnt; index++)
	    {
		/* see if user has authority to force db consistent */
		if (ck_authority(&duvecb, 0) == DUVE_NO)
		{
		    duve_talk( DUVE_ALWAYS, &duvecb, E_DU5032_NO_AUTHORITY,
			       6, 0, duvecb.duveowner,
			       0, duvecb.duve_list->du_dblist[index].du_db,
			       0, duvecb.duve_list->du_dblist[index].du_dba);
		    continue;
		}
		if (ck_distributed(&duvecb, index) == DUVE_YES)
		    continue;
		duve_talk(DUVE_MODESENS, &duvecb, S_DU04C2_FORCING_CONSISTENT,
	    	          2, 0, duvecb.duve_list->du_dblist[index].du_db);
		if ( duve_force(&duvecb, 
				duvecb.duve_list->du_dblist[index].du_db) 
	           == DUVE_YES)
		    duve_talk(DUVE_MODESENS, &duvecb, 
			      S_DU04C3_DATABASE_PATCHED, 2,
			      0, duvecb.duve_list->du_dblist[index].du_db);
		else
		    duve_talk(DUVE_MODESENS, &duvecb, E_DU5020_DB_PATCH_ERR, 2,
			      0, duvecb.duve_list->du_dblist[index].du_db);

                if(index == dbdbidx)
                {
                    /* Connection to the next DB may timeout on connection
                    ** to iidbdb because the disconnect is asynchronous.
                    ** Give the DBMS time to complete the disconnect.
                    */
                    PCsleep(500);
                }
	    } /* end for */
	    break;
	}

	else if (duvecb.duve_operation == DUVE_CATALOGS)
	{
	    /* assure that there atleast 1 valid database name in scopelist */
	    if (duvecb.duve_ncnt == 0)
		break;

	    for (index = 0; index < duvecb.duve_ncnt; index++)
	    {
		if (ck_distributed(&duvecb, index) == DUVE_YES)
		    continue;
		/* 
		** assure that verifydb is the correct level for this db 
		*/
		if (duvecb.duve_list->du_dblist[index].du_lvl != DU_CUR_DBCMPTLVL)
		{
		    duve_talk( DUVE_ALWAYS, &duvecb, E_DU5027_CMPTLVL_MISMATCH,
			    6, 0, duvecb.duve_list->du_dblist[index].du_db,
			    sizeof(cur_level), &cur_level,
			    sizeof(u_i4), &duvecb.duve_list->du_dblist[index].du_lvl);
		    continue;
		}
		else if (duvecb.duve_list->du_dblist[index].du_minlvl >
			 DU_1CUR_DBCMPTMINOR)
		{
		    duve_talk(DUVE_ALWAYS, &duvecb, E_DU5028_CMPTMINOR_MISMATCH,
			6, 0, duvecb.duve_list->du_dblist[index].du_db,
			sizeof(cmptminor), &cmptminor,
			sizeof(duvecb.duve_list->du_dblist[index].du_minlvl),
			&duvecb.duve_list->du_dblist[index].du_minlvl);
		    continue;
		}
		/* see if user has authority to check dbms catalogs */
		if (ck_authority(&duvecb, index) == DUVE_NO)
		{
		    duve_talk( DUVE_ALWAYS, &duvecb, E_DU5032_NO_AUTHORITY,
				6, 0, duvecb.duveowner,
				0, duvecb.duve_list->du_dblist[index].du_db,
				0, duvecb.duve_list->du_dblist[index].du_dba);
		    continue;
		}

		/* 
		**  check this db
		*/
		duve_talk( DUVE_MODESENS, &duvecb, S_DU04C0_CKING_CATALOGS, 2,
			    0, duvecb.duve_list->du_dblist[index].du_db);
		if (duve_catalogs(&duvecb, 
				  duvecb.duve_list->du_dblist[index].du_db)
	        ==DUVE_YES)
		    duve_talk( DUVE_MODESENS, &duvecb, S_DU04C1_CATALOGS_CHECKED, 2,
			       0,duvecb.duve_list->du_dblist[index].du_db);
		else
		    duve_talk( DUVE_MODESENS, &duvecb, E_DU501F_CATALOG_CHECK_ERR, 
			       2, 0, duvecb.duve_list->du_dblist[index].du_db);

                if(index == dbdbidx)
                {
                    /* Connection to the next DB may timeout on connection
                    ** to iidbdb because the disconnect is asynchronous.
                    ** Give the DBMS time to complete the disconnect.
                    */
                    PCsleep(500);
                }
	    } /* end for */
	    break;
	} /* end if */
	else if ( (duvecb.duve_operation == DUVE_TABLE) ||
		   (duvecb.duve_operation == DUVE_XTABLE) )
	{
	    /* assure that there really is a valid database to check */
	    if (duvecb.duve_ncnt == 0)
		break;
	    if (ck_distributed(&duvecb, index) == DUVE_YES)
		break;
	    /* 
	    ** assure that verifydb is the correct level for this db 
	    */
	    if (duvecb.duve_list->du_dblist[0].du_lvl != DU_CUR_DBCMPTLVL)
	    {
		duve_talk( DUVE_ALWAYS, &duvecb, E_DU5027_CMPTLVL_MISMATCH,
			    6, 0, duvecb.duve_list->du_dblist[0].du_db,
			    sizeof(cur_level), &cur_level,
			    sizeof(u_i4), &duvecb.duve_list->du_dblist[0].du_lvl);
		break;
	    }
	    else if (duvecb.duve_list->du_dblist[0].du_minlvl < 
		     DU_1CUR_DBCMPTMINOR)
	    {
		duve_talk(DUVE_ALWAYS, &duvecb, E_DU5028_CMPTMINOR_MISMATCH,
		    6, 0, duvecb.duve_list->du_dblist[0].du_db,
		    sizeof(cmptminor), &cmptminor,
		    sizeof(duvecb.duve_list->du_dblist[0].du_minlvl),
		    &duvecb.duve_list->du_dblist[0].du_minlvl);
		continue;
	    }
	    if (duvecb.duve_mode == DUVE_REPORT)
	    {
		/*
		** go check the table
		*/
		duve_talk(DUVE_MODESENS, &duvecb, S_DU04CB_START_TABLE_OP, 4,
			  0, duvecb.duve_ops[0], 
			  0, duvecb.duve_list->du_dblist[0].du_db);
		if ( duve_checktable(&duvecb, 
			      duvecb.duve_list->du_dblist[0].du_db,
			      duvecb.duve_ops[0]) != DUVE_BAD)
		    duve_talk(DUVE_MODESENS, &duvecb, S_DU04CC_TABLE_OP_DONE, 4,
			  0, duvecb.duve_ops[0], 
			  0, duvecb.duve_list->du_dblist[0].du_db);
		else
		    duve_talk(DUVE_MODESENS, &duvecb, E_DU502E_TABLE_OP_ERROR,4,
			  0, duvecb.duve_ops[0], 
			  0, duvecb.duve_list->du_dblist[0].du_db);
		break;
	    }
	    else
	    {
		/* Only INGRES SECURITY user or DBA are allowed to patch a
		** table */
		if (ck_authority(&duvecb, 0) == DUVE_NO)
		{
		    duve_talk( DUVE_ALWAYS, &duvecb, E_DU5032_NO_AUTHORITY,
				6, 0, duvecb.duveowner,
				0, duvecb.duve_list->du_dblist[index].du_db,
				0, duvecb.duve_list->du_dblist[index].du_dba);
		    break;
		}
		/*
		** go patch the table
		*/
		duve_talk(DUVE_MODESENS, &duvecb, S_DU04CB_START_TABLE_OP, 4,
			  0, duvecb.duve_ops[0], 
			0, duvecb.duve_list->du_dblist[0].du_db);
		if ( duve_patchtable(&duvecb, 
			      duvecb.duve_list->du_dblist[0].du_db,
			      duvecb.duve_ops[0]) != DUVE_BAD)
		    duve_talk(DUVE_MODESENS, &duvecb, S_DU04CC_TABLE_OP_DONE, 4,
			  0, duvecb.duve_ops[0], 
			0, duvecb.duve_list->du_dblist[0].du_db);
		else
		    duve_talk(DUVE_MODESENS, &duvecb, E_DU502E_TABLE_OP_ERROR,4,
			  0, duvecb.duve_ops[0], 
			  0, duvecb.duve_list->du_dblist[0].du_db);
		break;
	    }

	}
	else if (duvecb.duve_operation == DUVE_DROPTBL)
	{
	    /* assure that there really is a valid database to check */
	    if (duvecb.duve_ncnt == 0)
		break;
	    if (ck_distributed(&duvecb, index) == DUVE_YES)
		break;
	    /* 
	    ** assure that verifydb is the correct level for this db 
	    */
	    if (duvecb.duve_list->du_dblist[0].du_lvl != DU_CUR_DBCMPTLVL)
	    {
		duve_talk( DUVE_ALWAYS, &duvecb, E_DU5027_CMPTLVL_MISMATCH,
			    6, 0, duvecb.duve_list->du_dblist[0].du_db,
			    sizeof(cur_level), &cur_level,
			    sizeof(u_i4), &duvecb.duve_list->du_dblist[0].du_lvl);
		break;
	    }
	    else if (duvecb.duve_list->du_dblist[0].du_minlvl < 
		     DU_1CUR_DBCMPTMINOR)
	    {
		duve_talk(DUVE_ALWAYS, &duvecb, E_DU5028_CMPTMINOR_MISMATCH,
		    6, 0, duvecb.duve_list->du_dblist[0].du_db,
		    sizeof(cmptminor), &cmptminor,
		    sizeof(duvecb.duve_list->du_dblist[0].du_minlvl),
		    &duvecb.duve_list->du_dblist[0].du_minlvl);
		continue;
	    }
	    /* assure user is allowed to operate on this DB. */	    
	    if (ck_authority(&duvecb, 0) == DUVE_NO)
	    {
		duve_talk( DUVE_ALWAYS, &duvecb, E_DU5032_NO_AUTHORITY,
			    6, 0, duvecb.duveowner,
			    0, duvecb.duve_list->du_dblist[index].du_db,
			    0, duvecb.duve_list->du_dblist[index].du_dba);
		break;
	    }
	    /*
	    ** drop the table
	    */
	    duve_talk(DUVE_MODESENS, &duvecb, S_DU04C4_DROPPING_TABLE, 4,
		      0, duvecb.duve_ops[0], 
		      0, duvecb.duve_list->du_dblist[0].du_db);
	    if ( duve_tbldrp(&duvecb, duvecb.duve_list->du_dblist[0].du_db,
			  duvecb.duve_ops[0]) == DUVE_YES)
		duve_talk(DUVE_MODESENS, &duvecb, S_DU04C5_TABLE_DROPPED, 4,
		      0, duvecb.duve_ops[0], 
		      0, duvecb.duve_list->du_dblist[0].du_db);
	    else
		duve_talk(DUVE_MODESENS, &duvecb, E_DU5024_TBL_DROP_ERR, 4,
		      0, duvecb.duve_ops[0], 
		      0, duvecb.duve_list->du_dblist[0].du_db);
	    break;

	}
	else if (duvecb.duve_operation == DUVE_REFRESH)
	{
	    /* assure that there atleast 1 valid database name in scopelist */
	    if (duvecb.duve_ncnt == 0)
		break;

	    for (index = 0; index < duvecb.duve_ncnt; index++)
	    {
		if (ck_distributed(&duvecb, index) == DUVE_NO)
		{
		    duve_talk(DUVE_MODESENS, &duvecb, S_DU1706_NOT_STARDB, 2,
				0, duvecb.duve_list->du_dblist[index].du_db);
		    continue;
		}
		/* 
		** assure that verifydb is the correct level for this db.
		** NOTE: we do not check for user authority here, as the
		**       duve_purge routine checks it.
		*/
		if (duvecb.duve_list->du_dblist[index].du_lvl != DU_CUR_DBCMPTLVL)
		{
		    duve_talk( DUVE_ALWAYS, &duvecb, E_DU5027_CMPTLVL_MISMATCH,
			    6, 0, duvecb.duve_list->du_dblist[index].du_db,
			    sizeof(cur_level), &cur_level,
			    sizeof(u_i4), &duvecb.duve_list->du_dblist[index].du_lvl);
		    continue;
		}
		else if (duvecb.duve_list->du_dblist[index].du_minlvl >
			 DU_1CUR_DBCMPTMINOR)
		{
		    duve_talk(DUVE_ALWAYS, &duvecb, E_DU5028_CMPTMINOR_MISMATCH,
			6, 0, duvecb.duve_list->du_dblist[index].du_db,
			sizeof(cmptminor), &cmptminor,
			sizeof(duvecb.duve_list->du_dblist[index].du_minlvl),
			&duvecb.duve_list->du_dblist[index].du_minlvl);
		    continue;
		}

		/* 
		**  Call the REFRESH_LDBS executive 
		*/
		duve_talk( DUVE_MODESENS, &duvecb, S_DU04D2_REFRESH_START, 2,
			    0, duvecb.duve_list->du_dblist[index].du_db);
		if (duve_refresh ( &duvecb, 
				  duvecb.duve_list->du_dblist[index].du_db)
	        ==DUVE_YES)
		    duve_talk( DUVE_MODESENS, &duvecb, S_DU04D3_REFRESH_DONE, 2,
			       0,duvecb.duve_list->du_dblist[index].du_db);
		else
		    duve_talk( DUVE_MODESENS, &duvecb, E_DU5039_REFRESH_ERR, 
			       2, 0, duvecb.duve_list->du_dblist[index].du_db);

                if(index == dbdbidx)
                {
                    /* Connection to the next DB may timeout on connection
                    ** to iidbdb because the disconnect is asynchronous.
                    ** Give the DBMS time to complete the disconnect.
                    */
                    PCsleep(500);
                }
	    } /* end for */
	    break;
	} /* end if */
	else if ( (duvecb.duve_operation == DUVE_PURGE) ||
		  (duvecb.duve_operation == DUVE_TEMP) ||
		  (duvecb.duve_operation == DUVE_EXPIRED) )
	{
	    /* assure that there atleast 1 valid database name in scopelist */
	    if (duvecb.duve_ncnt == 0)
		break;

	    for (index = 0; index < duvecb.duve_ncnt; index++)
	    {
		if (ck_distributed(&duvecb, index) == DUVE_YES)
		    continue;
		/* 
		** assure that verifydb is the correct level for this db.
		** NOTE: we do not check for user authority here, as the
		**       duve_purge routine checks it.
		*/
		if (duvecb.duve_list->du_dblist[index].du_lvl != DU_CUR_DBCMPTLVL)
		{
		    duve_talk( DUVE_ALWAYS, &duvecb, E_DU5027_CMPTLVL_MISMATCH,
			    6, 0, duvecb.duve_list->du_dblist[index].du_db,
			    sizeof(cur_level), &cur_level,
			    sizeof(u_i4), &duvecb.duve_list->du_dblist[index].du_lvl);
		    continue;
		}
		else if (duvecb.duve_list->du_dblist[index].du_minlvl >
			 DU_1CUR_DBCMPTMINOR)
		{
		    duve_talk(DUVE_ALWAYS, &duvecb, E_DU5028_CMPTMINOR_MISMATCH,
			6, 0, duvecb.duve_list->du_dblist[index].du_db,
			sizeof(cmptminor), &cmptminor,
			sizeof(duvecb.duve_list->du_dblist[index].du_minlvl),
			&duvecb.duve_list->du_dblist[index].du_minlvl);
		    continue;
		}

		/* 
		**  Call the executive that does all types of purges.
		*/
		duve_talk( DUVE_MODESENS, &duvecb, S_DU04C7_PURGE_START, 2,
			    0, duvecb.duve_list->du_dblist[index].du_db);
		if (duve_purge ( &duvecb, 
				  duvecb.duve_list->du_dblist[index].du_db,
				  duvecb.duve_list->du_dblist[index].du_dba)
	        ==DUVE_YES)
		    duve_talk( DUVE_MODESENS, &duvecb, S_DU04CA_PURGE_DONE, 2,
			       0,duvecb.duve_list->du_dblist[index].du_db);
		else
		    duve_talk( DUVE_MODESENS, &duvecb, E_DU502A_PURGE_ERR, 
			       2, 0, duvecb.duve_list->du_dblist[index].du_db);

                if(index == dbdbidx)
                {
                    /* Connection to the next DB may timeout on connection
                    ** to iidbdb because the disconnect is asynchronous.
                    ** Give the DBMS time to complete the disconnect.
                    */
                    PCsleep(500);
                }
	    } /* end for */
	    break;
	} /* end if */
	else
	    duve_talk( DUVE_ALWAYS, &duvecb, E_DU501E_OP_NOT_IMPLEMENTED, 0);
	break;

    } /* end for */

    EXdelete();


    duve_exit(&duvecb);

}

/*{
** Name: duve_exit	- verifydb exit
**
** Description:
**	exit neatly by closing the verifydb log file and closing and database 
**      that verifydb may be connected to 
**
** Inputs:
**      duve_cb                         verifydb control block, contains
**	    log file control block
**	    open database information
**	    duve_msg (error message control block)
**		du_status  (error status: info, warning, etc)
**
** Outputs:
**	none
**	Returns:
**	    OK via PC_EXIT
**	Exceptions:
**	    none
**
** Side Effects:
**	    log file is closed
**	    if any ESQL transaction exists, it is aborted
**
** History:
**      2-aug-88 (teg)
**          Initial creation.
**      25-aug-88 (teg)
**          bring up to RELATIONAL TECHNOLOGY coding standards
**	31-oct-88 (teg)
**	    Add logic to exit with failure status if terminating because of err
**	01-Mar-89 (teg)
**	    Added logic to release any locks verifydb holds.
[@history_template@]...
*/
VOID
duve_exit(duve_cb)
DUVE_CB	    *duve_cb;
{
    DU_ERROR	err_cb;
    duve_fclose(duve_cb);

    /* see if there is a database to close */
    if (duve_cb->du_opendb[0] != '\0')
    {
	/* 
	** database would not be open if healthy processing to duve_exit,
	** so, abort transaction
        */
	duve_close( duve_cb, TRUE);
    }

    if ( (duve_cb->duve_msg->du_status == E_DU_IERROR) ||
	 (duve_cb->duve_msg->du_status == E_DU_UERROR) )
	    PCexit(FAIL);
    else
	    PCexit(OK);
}

/*{
** Name: duve_exception() -	Exception handler for verifydb.
**
** Description:
**      This is the exception handler for the database utility, verifydb.
**	If the user does a CTRL-C, then this sets duvecb.du_exception_stat
**	to DUVE_USR_INTERRUPT, otherwise it sets duvecb.du_exception_stat
**	to DUVE_EXCEPTION (indicating that an unexpected exception occurred). 
**	This routine does not do any I/O, but returns EXDECLARE, which causes
**	control to go to the verifydb main program just below the EXdecare()
**	call to define this exception handler.
**
**	The first EXdeclare() call has statements to output a warning message
**	to the user.  The second EXdeclare() is only hit if verifydb recieves
**	an exception while processing an exception.  It that event, no message
**	is output.
**
** Inputs:
**      ex_args                         EX_ARGS struct.
**	    .ex_argnum			Tag for the exception that occurred.
**
** Outputs:
**	Returns:
**	    duvecb
**		.du_exception_stat	exception status: either
**					    DUVE_USR_INTERRUPT or
**					    DUVE_EXCEPTION
**	    The signal is resignaled to EXDECLARE.
**	Exceptions:
**	    none
**
** Side Effects:
**  none.
**
** History:
**      2-aug-88 (teg)
**          Initial creation.
**	21-apr-92 (teresa)
**	    rewrote for bug 42660 to stop doing I/O and return control via
**	    EXDECLARE.  Moved the logic to print warnings from this routine
**	    to a block below the EXdeclare() call.  That allows verifydb to
**	    handle the case where it recieves an exception from within an
**	    exception (ie, gets an AV while trying to output the warning that
**	    used to come from this routine).
**	13-sep-96 (nick)
**	    Using an EX type directly as a case label is a no-no.
[@history_template@]...
*/
STATUS
duve_exception(ex_args)
EX_ARGS		    *ex_args;
{
    switch (EXmatch(ex_args->exarg_num, 2, (EX)EXINTR, (EX)EXKILL))
    {
      case 1:
      case 2:
	duvecb.du_exception_stat = DUVE_USR_INTERRUPT;
	break;

      default:
	duvecb.du_exception_stat = DUVE_EXCEPTION;
	break;
    }
    /* Return control to where the exception handler was declared */
    return (EXDECLARE); 
}

/*{
** Name: duve_ingerr() - INGRES Exception processor
**
** Description:
**  This routine puts the ingres error code into the verifydb control block.
**  It contains logic to distinguish between a fatal and a non-fatal error.  
**  If the error is a non-fatal expected error, it will return DUVE_WARN.
**  Otherwise, it will return DUVE_FATAL or DUVE_SPECIAL, which will cause 
**  verifydb to terminate.
**
** Inputs:
**      ingerr		--  INGRES error number
**
** Outputs:
**	Returns:
**	    DUVE_WARN	-- continue processing, print warning
**	    DUVE_SPECIAL-- Print a special case message instead of the server
**			   message and abort processing)
**	    DUVE_FATAL	-- abort processing
**	Exceptions:
**	    none
** Side Effects
**  none.
**
** History:
**      2-Aug-1988 (teg)
**          Initial creation.
**	5-mar-1990 (teg)
**	    Add logic to recover from syntax error during purge or purge_expired
**	    operation.
**	8-aug-90 (teresa)
**	    added logic to handle purge not being able to drop subdirectories,
**	    also changed return status from DUVE_NO (now DUVE_FATAL or 
**	    DUVE_SPECIAL) and DUVE_YES (now DUVE_WARN)
[@history_template@]...
*/
DU_STATUS
duve_ingerr( ingerr)
i4  ingerr;
{
    duvecb.duve_msg->du_ingerr = ingerr;
    if (ingerr == E_GE94D4_HOST_ERROR)
    {
	/* verifydb assumes that all files in a database default directory
	** are files.  However, it is possible one or more of these files are
	** really subdirectories.  DI does not provide a mechanism for
	** distinguishing between files and directories.  If DIlistfile returns
	** a directory name, there is no way for verifydb to tell this is a
	** directory.  So, if we cannot drop the file, then print a warning 
	** message and continue processing.
	*/
	if ( ((duvecb.duve_operation == DUVE_PURGE) || 
	      (duvecb.duve_operation == DUVE_TEMP) ) &&
	      (duvecb.duve_delflg)  && (duvecb.duve_mode != DUVE_REPORT) )
	{
	    /* yep, we were trying to drop a file.  So print a sorry charlie
	    ** message and quit. */
	    duve_talk(DUVE_MODESENS, &duvecb, S_DU1703_DELETE_ERR, 0);
	    return DUVE_SPECIAL;
	}
    }

    if (ingerr == E_GE7918_SYNTAX_ERROR)
    {
	/* verifydb purge_expired is written in SQL.  It gets an SQL error
	** if it tries to drop a QUEL generated expired table with a name that
	** happens to be an SQL reserved word.  Check for this case.  If
	** encountered, print a warning that we could not drop this table, but
	** allow the purge function to continue.
	*/
	if ( ((duvecb.duve_operation == DUVE_PURGE) || 
	      (duvecb.duve_operation == DUVE_EXPIRED) ) &&
	      (duvecb.duve_mode != DUVE_REPORT) )
	{
	    /* yep.  This is probably an error genereated by dropping a QUEL 
	    ** table with a name that's an SQL reserved word.  The purge
	    ** expired relations routine will report the error */
	    return ( (DU_STATUS) DUVE_WARN);
	}
    }
    else if (duvecb.duve_operation == DUVE_ACCESS)
    {
	/* we are doing an accesscheck.  The goal is to report any errors
	** and continue processing the next db on the list...*/
	return ( (DU_STATUS) DUVE_WARN);
    }
    return ( (DU_STATUS) DUVE_FATAL);
}

/*{
** Name: duve_issue_legal_warning() - Issue Legal Warning for Verifydb
**
** Description:
**  This routine issues any legal disclaimers for verifydb, and (if appropriate)
**  causes the user be prompted if they wish to continue (in light of the
**  warning).
**
**  This routine is special case, and new cases will be added as the verifydb
**  product line grows.  The current cases are:
**
**    OPERATION = DBMS_CATALOGS
**	This operation can cause loss of user tables when it is run in the
**	RUNINTERACTIVE (DUVE_IRUN) mode.  It cannot cause loss of user tables
**	in any other mode.  Therefore, a warning message will be printed in
**	all modes when DBMS_CATALOGS operation is selected.  If the mode
**	happens to be DUVE_IRUN, then the user will be prompted for permission
**	to continue.  Verifydb will be terminated if the user does not specify
**	YES.
**    OPERATION = FORCE_CONSISTENT
**      This operation is executed, regardless of mode.  Once a database has
**	been forced consistent, the user should use UNLOADDB, runing unload.ing
**	script, destroy the database, recreate it, then run the reload.ing
**	script to repopulate it.  If they dont do so, then the DB may go
**	inconsistent again, and their work may be lost.  A warning message is
**	always printed, regardless of mode.  The user is not prompted for
**	permission to continue.
**    OPERATION = TABLE or XTABLE
**	This operation can cause loss of some/all tuples in a user table if
**	run in any mode except "report".  It will cause the table to be converted
**	to a heap and will cause any secondary indices on the table to be
**	destroyed.  If run in the "report" mode, then no data is lost or
**      reformatted.
** Inputs:
**      duve_cb                         verifydb control block, contains:
**	    duve_operation		    indicates the operation to perform
**					    (FORCE_CONSISTENT, DBMS_CATALOGS,etc)
**
** Outputs:
**	Returns:
**	    DUVE_YES	-- continue processing
**	    DUVE_NO	-- abort processing
**	    DUVE_BAD	-- error encountered
**	Exceptions:
**	    none
** Side Effects
**  none.
**
** History:
**      19-Jan-89
**          Initial creation.
**	14-Sep-89
**	    added warning for XTABLE and TABLE operations.
[@history_template@]...
*/
DU_STATUS
duve_issue_legal_warning( duve_cb )
DUVE_CB		*duve_cb;
{
    DU_STATUS retstat;
    
    if (duve_cb -> duve_operation == DUVE_MKCONSIST)
    {
	retstat = duve_talk (DUVE_LEGAL, duve_cb,
			     S_DU17FE_FORCE_CONSISTENT_WARN, 0);
	if (retstat != DUVE_YES)
	    duve_talk (DUVE_ALWAYS, duve_cb, S_DU04C6_USER_ABORT, 0);
	return (retstat);
		
    }
    if (duve_cb -> duve_operation == DUVE_CATALOGS)
    {
	retstat = duve_talk (DUVE_LEGAL, duve_cb,
			     S_DU17FD_CATALOG_PATCH_WARNING, 0);
	if (retstat != DUVE_YES)
	    duve_talk (DUVE_ALWAYS, duve_cb, S_DU04C6_USER_ABORT, 0);
	return (retstat);
    }
    if ( (duve_cb -> duve_operation == DUVE_TABLE) ||
	 (duve_cb -> duve_operation == DUVE_XTABLE) )
    {
	retstat = duve_talk (DUVE_LEGAL, duve_cb,
			     S_DU17FC_TABLE_PATCH_WARNING, 0);
	if (retstat != DUVE_YES)
	    duve_talk (DUVE_ALWAYS, duve_cb, S_DU04C6_USER_ABORT, 0);
	return (retstat);
    }
    return ( (DU_STATUS) DUVE_YES);
}

/*{
** Name: ck_authority - Check that user has authority to use verifydb on 
**			specified db.
**
** Description:
**	If the user has SECURITY or OPERATOR privilege then they can
**	perform any verifydb operation.  If the user does not have these
**	privileges, then that user must be the DBA of that database, or
**	verifydb will not allow the user to operate on it.
**
** Inputs:
**      duve_cb                         verifydb control block, contains:
**	    .duve_list			    array of du_dblist (one for each db
**					    in scope)
**		.du_dblist			contains db, dba names
**		    .du_dba			    name of DBA for this DB.
**	    .du_ownstat			    iiuser.status for executor of 
**					    the verifydb task.
**
** Outputs:
**	Returns:
**	    DUVE_YES	-- User has permission to access this db.
**	    DUVE_NO	-- User does NOT have permission to access this db.
**	Exceptions:
**	    none
** Side Effects
**  none.
**
** History:
**      05-feb-1990 (teg)
**          Initial creation.
**	18-may-1993 (robf)
**	    Replaced SUPERUSER by SECURITY or OPERATOR
[@history_template@]...
*/
DU_STATUS
ck_authority( duve_cb,index)
DUVE_CB		*duve_cb;
i4		index;
{
    DU_STATUS retstat;


    if (duve_cb->du_ownstat & DU_USECURITY )
	retstat = DUVE_YES;
    else if (duve_cb->du_ownstat & DU_UOPERATOR )
	retstat = DUVE_YES;
    else
    {
	if (STcompare(duve_cb->duve_list->du_dblist[index].du_dba,
		      duve_cb->duveowner) 
        == DU_IDENTICAL)
	    retstat = DUVE_YES;
	else
	    retstat = DUVE_NO;
    }
    return (retstat);
}

/*{
** Name: ck_distributed - Check whether DB is a Distributed Database
**
** Description:
**
**	The database is distributed if (du_dbservice & DU_1SER_DDB) is true.
**	If so, check to see if the CDB is an ingres database.  If so, report
**	a message that names the CDB.  If not, report a message that says 
**	verifydb cannot operate on the CDB at this time.
**
** Inputs:
**      duve_cb                         verifydb control block, contains:
**	    .duve_list			    array of du_dblist (one for each db
**					    in scope)
**		.du_dblist			contains db, dba names
**		    .du_dbservice		    contains iidatabase.dbservice
**		    .du_cdb			    name of cdb
**		    .du_dbmstype		    name of server type that
**							created CDB.
**
** Outputs:
**	Returns:
**	    DUVE_YES	-- Database is DDB.
**	    DUVE_NO	-- Database is NOT ddb.
**	Exceptions:
**	    none
** Side Effects
**  none.
**
** History:
**      29-mar-1990 (teg)
**          Initial creation for bug 20971.
**	14-jul-93 (teresa)
**	    Force supression of S_DU04D0_DDB_WITH_INGRES_CDB for REFRESH_LDBS 
**	    operation
[@history_template@]...
*/
DU_STATUS
ck_distributed( duve_cb,index)
DUVE_CB		*duve_cb;
i4		index;
{
    DU_STATUS retstat;


    if (duve_cb->duve_list->du_dblist[index].du_dbservice & DU_1SER_DDB )
    {
	retstat = DUVE_YES;
	
	if (MEcmp ( duve_cb->duve_list->du_dblist[index].du_dbmstype,
		    "INGRES", (u_i2) 6) )
	{
	    /* cdb is not ingres database */
	    duve_talk (DUVE_MODESENS, duve_cb, S_DU04D1_DDB_CDB_NOT_INGRES, 4,
			0, duve_cb->duve_list->du_dblist[index].du_db);
	}
	else if (duvecb.duve_operation != DUVE_REFRESH)
	{
	    /* cdb is ingres database, so suggest running verifydb on CDB */
	    duve_talk (DUVE_MODESENS, duve_cb, S_DU04D0_DDB_WITH_INGRES_CDB, 4,
			0, duve_cb->duve_list->du_dblist[index].du_db,
			0, duve_cb->duve_list->du_dblist[index].du_cdb);
	}

    }
    else
	retstat = DUVE_NO;
    return (retstat);
}
