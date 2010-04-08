/*
** Copyright (c) 1992, 2008 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<me.h>
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ui.h>
#include	<uigdata.h>

/**
** Name:	feuidata.c -	Front-End DBMS Environment Data Definitions.
**
** Description:
**	Contains the definitions of the data that describes the current DBMS
**	environment.  Defines
**
**	IIUIfedata	          - Returns structure containing non-DBMS 
**				    information.
**	IIUIdbdata	          - Return structure describing current DBMS 
**				    connection.
**	IIUIcddCheckDBdata        - same as IIUIdbdata(), but return NULL if no
**				    structure exists yet.
**	IIUIssdStoreSessionData   - Store DBMS connection info.
**	IIUIpsdPartialSessionData - Store DBMS connection info about a 
**				    partial session.
**
** History:
**	Revision 6.5
**	21-sep-92 (davel)
**		Add IIUIcddCheckDBdata() to return connection info if it
**		exists (as IIUIdbdata() would), but return NULL if it does not
**		exist yet to let client take appropriate action.
**
**	Revision 6.4  1991/05/05  wong
**	Added capability version levels.
**	1991/08  wong  Set application flag for sessions initialized through
**		'IIUIdbdata()'.  Bug #38671.
**
**	Revision 6.3
**	30-nov-1989 (Mike S)
**		move global data into structure
**	25-feb-1990 (Mike S)
**		Add prompt_func entry
**	16-aug-1990 (Joe)
**	    Made two separate structures UIGDATA and UIDBDATA for non-DBMS
**	    and DBMS information.  Changed to use global function 'IIUIdbdata()'
**	    to get at the DBMS information this supports multiple DBMS sessions.
**	27-aug-1990 (Joe)
**	    Added support for ROstatus for DB/SQL.
**	28-sep-1990 (Joe)
**	    Changed the meaning of firstAutoCom to be what IIui1stAutoCom
**	    used to be.  Added entryAC to hold the autocommit state on
**	    entry to a shared session.
**
**	Revision 6.2  88/12  wong
**	Added 'IIUIisdba'.
**
**	Revision 6.0  88/01/18  wong
**	Extracted definitions from "feingres.qsc".
**	9-mar-1988 (danielt)
**		added  IIUIsuser, IIUIsdba, IIUIcatowner
**	24-apr-a996 (chech02)
**		added function prototypes for windows 3.1 port.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      16-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**/

/*
** This constant is the value for the UI directory found in erui.msg as
** _UI_CLASS.
*/

# define	_UI_CLASS	34

VOID	IIUGprompt();
#ifdef WIN16
PTR	IILQgsdGetSessionData(i4);
STATUS	IILQpsdPutSessionData(i4, PTR);
#endif /* WIN16 */


static UIGDATA sIIUIgdata =
{
    FALSE,	/* testing */
    IIUGprompt	/* prompt_func */
};

/*{
** Name:	IIUIfedata	- Returns pointer to structure of non-DBMS info
**
** Description:
**	The returns a pointer to a UIGDATA structure for this FE session.
**
** Outputs:
**	Returns:	Pointer to a UIGDATA structure.
**
** History:
**	28-aug-1990 (Joe)
**	    First written
**	10-jun-92 (rdrane)
**	    Add defaulting of delimited identifier casing (dlm_Case) for 6.5.
**		removed initializer for obsolete IIUIDBDATA member "version".
**	31-dec-93 (rdrane)
**	    Change defaulting of delimited identifier casing to "undefined".
**	05-feb-1997 (sandyd)
**	    Added initialization of new max_cat_col_wid member of UIDBDATA.
*/
UIGDATA *
IIUIfedata()
{
    return &sIIUIgdata;
}

/*
** Access to the DBMS specific structure is through the global function
** IIUIdbdata.  This function returns a pointer to a UIDBDATA structure.
** The reason a global pointer can't be used is that with multiple sessions,
** when the session is switched, a different structure is needed for in the
** information relavant to the particular DBMS session being run.  If we
** were using a global pointer, when the DBMS session is switched, the
** pointer would be switched to point to a different structure. However,
** the way that global pointers can be supported in VMS transfer vectors,
** means that the global pointer in the transfer vector is actually a
** different piece of memory than the global pointer inside the shared
** library.  In this case there are 2 IIUIgdata's.  When code inside the
** shared library refers to IIUIgdata, it actually references  the piece of
** memory allocated by the GLOBALDEF above.  When code outside the shared
** library refers to IIUIgdata, it references a piece of memory inside
** the transfer vector.  Code inside the shared image can not change the
** value of the pointer in the transfer vector.  Thus, IIUIgdata works
** since it always points at the same piece of memory, but something
** like an IIUIdbdata pointer for the DBMS data would not work.
**
** The above comment is no longer accurate since IIUIgdata was changed
** from a function to a routine, but the principal of how global data
** can be put into a transfer vector is correct.
*/
static UIDBDATA	sIIUIdbdata =
{
    FALSE,		/*firstAutoCom*/
    FALSE,		/*isdba*/
    FALSE,		/* connected */
    ERx(""),		/*dba*/
    ERx(""),		/*user*/
    ERx(""),		/*sdba*/
    ERx(""),		/*suser*/
    ERx(""),		/*database*/
    ERx(""),		/*catowner*/
    UI_LEVEL_NONE,	/* ingsql_level */
    UI_LEVEL_60,	/* opensql_level */
    UI_LEVEL_62,	/* catalog_level */
    UI_LEVEL_NONE,	/* ingquel_level */
    UI_DML_NOLEVEL,	/* sql_level */
    UI_DML_NOLEVEL,	/* quel_level */
    UI_LOWERCASE,	/* name_Case */
    UI_UNDEFCASE,	/* dlm_Case */
    UI_KEY_N,		/* key_req */
    0,			/* max_cat_col_wid */
    FALSE,		/* is_init */
    0,			/* isDist */
    0,			/* isIngres */
    0,			/* useIItables */
    0,			/* hasRules */
    0,			/* hasLogKeys */
    0,			/* hasUADT */
    0,			/* hasGroups */
    0,			/* hasRoles */
    0,			/* useEscape */
    FALSE,		/* sharedSession */
    0,			/* savept_counter */
    FALSE,		/* in_transaction */
    0,			/* xactlabel */
    FALSE,		/* lababort */
    UI_DML_NOLEVEL,	/* dmldef */
    FALSE,		/* ROstatus */
    FALSE		/* entryAC */
};

/*{
** Name:	IIUIdbdata() -	Return pointer to global data.
**
** Description:
**	This function returns a pointer to a UIDBDATA structure containing
**	information for the current DBMS session.
**
**	NOTES TO USERS:
**	    1)  The pointer value returned by this routine may
**		change from call to call depending on the DBMS
**		session that is being executed.  If you know the
**		DBMS session will not switch and the current session
**		will not close, you can keep a local copy of the pointer
**		to the returned structure, but otherwise you should call
**		IIUIdbdata each time you need the information.
**
**	    2)  This routine may cause a query to be executed.
**		You MUST make sure NOT to call this routine while
**		in the middle of a query statement.  It is best
**		to call it before the query and then use the
**		returned structure in the query.
**
**	A standard example of the usage of this routine would be:
**
**		EXEC SQL BEGIN DECLARE SECTION;
**		     UIDBDATA	*uidata;
**		EXEC SQL END DECLARE SECTION;
**
**		uidata = IIUIdbdata();
**		EXEC SQL SELECT c FROM t
**		     WHERE owner = :uidata->user;
**
** Returns:
**	{UIDBDATA *} A reference to a UIDBDATA structure containing the
**			information for the current session.  A value will
**			always be returned even if no session is active.  If no
**			session is active, the returned structure will contain
**			default values (e.g., the strings will be empty).
**
** History:
**	aug-1991 (jhw) - Whenever the call to 'IIUIssdStoreSessionData()' is
**		made through here it is for an application!  Bug #38671.  All
**		Front-Ends should connect via 'FE[n]*ingres()' or equivalent.
*/
UIDBDATA *
IIUIdbdata()
{
    UIDBDATA	*rval;
    bool	dummy;

    PTR		IILQgsdGetSessionData();
    STATUS	IILQpsdPutSessionData();

    if ((rval = (UIDBDATA *) IILQgsdGetSessionData(_UI_CLASS)) != NULL)
	return rval;	/* got session data */
    if ( IIUIssdStoreSessionData((char *)NULL, TRUE, FALSE, &dummy) != OK
	    || (rval = (UIDBDATA *)IILQgsdGetSessionData(_UI_CLASS)) == NULL )
    {
	return &sIIUIdbdata;	/* couldn't get session data; use default */
    }
    return rval;
}

/*{
** Name:	IIUIcddCheckDBdata - check for existence of UIDBDATA structure.
**
** Description:
**	This function returns a pointer to a UIDBDATA structure containing
**	information for the current DBMS session if one exists, and otherwise
**	returns null. Note this is identical to the above-defined IIUIdbdata()
**	except that this function returns NULL if a UIDBDATA structure has not
**	yet been stored for the current session.  This allows clients to 
**	take some other action if they're not ready for a query to be issued
**	(which is a side-effect of IIUIdbdata()).
**
**	This amounts to simply a call to IILQgsdGetSessionData() with the class
**	specified as _UI_CLASS; it's slightly preferable to encapulate this
**	request for the UI LIBQ session data in this UI routine rather than
**	have each client issue the IILQgsdGetSessionData(34) call separately.
**
** Returns:
**	{UIDBDATA *} A reference to a UIDBDATA structure containing the
**			information for the current session, or NULL if
**			no information exists yet.
**
** History:
**	21-sep-92 (davel)
**		Written.
*/
UIDBDATA *
IIUIcddCheckDBdata()
{
    PTR		IILQgsdGetSessionData();

    return (UIDBDATA *) IILQgsdGetSessionData(_UI_CLASS);
}

/*
** Name:	IIUIssdStoreSessionData	- Store data about DBMS session.
**
** Description:
**	This call causes UI to store needed information about the current
**	session.  This call is intended to be used by FE routines
**	that establish DBMS connections.
**
**	Note, IIUIdbdata will invoke this routine if called when no data
**	have been stored for the current session.
**
** Inputs:
**	dbname		The name of the database that is being run.
**
**	appl		If TRUE then this session is for a user's application.
**			FALSE means it is an INGRES FE that is using the
**			session.
**
**	was_xflag	This must be set to TRUE if the xflag was used
**			to start the DBMS session that has been started.
**
** Outputs
**	was_appl	This will be set to TRUE on exit if DBMS session is
**			shared (the xflag was used) and the parent is
**			a user application.   In some cases, whether the
**			parent is a user application has be guessed at.
**			In general, if this flag is set to FALSE, then
**			you can safely assume that your parent is an FE
**			program, otherwise you should assume you don't
**			know anything about how the parent connected to
**			the DBMS.
**
**	Returns:
**		A status value.  OK means that the data for the session
**		was stored and will be returned by subsequent calls to
**		IIUIdbdata when the current DBMS session is executing.
**		A failure code means that no data could be stored for
**		the current session.
**
** Side Effects:
**	This may run some queries, but it will not commit any
**	queries it runs since the caller may be the FRS when it
**	does a forminit, and we don't want to commit a user's transaction.
**	The caller needs to commit after this routine if they don't
**	want an outstanding transaction.
**
** History
**	16-aug-1990 (Joe)
**	    First Written
*/
STATUS
IIUIssdStoreSessionData(dbname, isappl, was_xflag, was_appl)
char	*dbname;
bool	isappl;
bool	was_xflag;
bool	*was_appl;
{
    UIDBDATA	*data;
    STATUS	rval;

    STATUS	IIUIgetData();

    *was_appl = FALSE;
    /*
    ** Note,  this will call IIUIgetData which in turn will call
    ** IIUIdbdata.  Thus, we give the UIDBDATA to LIBQ right away, so
    ** that when IIUIgetData calls IIUIdbdata it will return the
    ** block we allocate here.
    */
    if ( (data = (UIDBDATA *)MEreqmem(0, sizeof(UIDBDATA), TRUE, &rval)) == NULL
	|| (rval = IILQpsdPutSessionData(_UI_CLASS, (PTR)data)) != OK )
    {
	if (data != NULL)
	    MEfree((PTR) data);
	return rval;
    }
    data->connected = TRUE;	/* We assume we are connected. */
    if (dbname != NULL)
    {
	STlcopy(dbname, data->database, FE_MAXDBNM);
	_VOID_ STtrmwhite(data->database);
    }
    if (was_xflag)
    {
	if (IIUIsharedData(was_appl) == OK)
	{
	    if (!*was_appl && isappl)
		IIUIsetData(isappl);
	    return OK;
	}
	/*
	** If the xflag was set, but we couldn't read shared data, there
	** are 2 possible cases.  1) the X flag is from an Embedded program.
	** in this case, no shared data is passed, so we need to set
	** the was_appl flag.  2) we were called by an FE or an ABF application
	** of a different version that has incompatible shared data.
	** In this case, we don't know whether it is an FE or an ABF
	** application.
	**
	** In any case, we set the was_appl flag to tell callers that this
	** if not an INGRES FE.
	*/
	*was_appl = TRUE;
    }
    /*
    ** If we got here, it means was_xflag was FALSE, or we couldn't pick
    ** up the shared data, so we need to grab the data anyway.
    */
    return ( (rval = IIUIdci_initcap()) == OK ) ? IIUIgetData(isappl) : rval;
}

/*
** Name:	IIUIpsdPartialSessionData - Store data about a partial session.
**
** Description:
**	This is a special entry point for use by the terminal monitor.
**	The terminal monitor sometimes needs to open sessions with
**	databases that aren't completely built.  Because of this,
**	no queries against the standard catalogs can be safely done.
**	The assumption is that this is only used against INGRES DBs.
**
**	This causes session data to be stored, but no queries to be run.
**
** Inputs:
**	dbname		The name of the database that is being run.
**
** Returns:
**	{STATUS}  OK means that the data for the session was stored and will be
**		returned by subsequent calls to 'IIUIdbdata()' when the current
**		DBMS session is executing.  A failure code means that no data
**		could be stored for the current session.
**
** History
**	28-aug-1990 (Joe)
**	    First Written
**	10-jun-92 (rdrane)
**	    Add defaulting of delimited identifier casing (dlm_Case) for 6.5.
**	05-feb-1997 (sandyd)
**	    Added initialization of new max_cat_col_wid member of UIDBDATA.
*/
STATUS
IIUIpsdPartialSessionData(dbname)
char	*dbname;
{
    UIDBDATA	*data;
    STATUS	rval = OK;

    static const char	_dummy[] = ERx("dummy");

    if ((data = (UIDBDATA *) MEreqmem(0, sizeof(UIDBDATA), TRUE,
				   (STATUS *)NULL)) == NULL
	|| IILQpsdPutSessionData(_UI_CLASS, (PTR) data) != OK)
    {
	if (data != NULL)
	    MEfree((PTR) data);
	return FAIL;
    }
    data->connected = TRUE;	/* We assume we are connected. */
    if (dbname != NULL)
    {
	STlcopy(dbname, data->database, FE_MAXDBNM);
	_VOID_ STtrmwhite(data->database);
    }
    /*
    ** Make this look like an INGRES DBMS.
    ** Note, we don't assume things like rules or groups,
    ** but only what INGRES has had since release 6.0.
    */
    STcopy(ERx("$ingres"), data->catowner);
    STcopy(_dummy, data->dba);
    STcopy(_dummy, data->sdba);
    STcopy(_dummy, data->user);
    STcopy(_dummy, data->suser);

    /* Fill in the capabilities to a default value. */

    STcopy(UI_LEVEL_60, data->ingsql_level);
    STcopy(UI_LEVEL_60, data->opensql_level);
    STcopy(UI_LEVEL_62, data->catalog_level);
    STcopy(UI_LEVEL_60, data->ingquel_level);

    data->sql_level = UI_DML_SQL;
    data->quel_level = UI_DML_QUEL;
    data->name_Case = UI_LOWERCASE;
    data->dlm_Case = UI_UNDEFCASE;
    data->key_req = UI_KEY_N;
    data->max_cat_col_wid = 0;
    data->is_init = TRUE;
    data->isIngres = 1;
    data->isdba = TRUE;

    return rval;
}
