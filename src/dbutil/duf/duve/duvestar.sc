/*
**Copyright (c) 1993, 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<gl.h>
#include        <sl.h>
#include	<iicommon.h>
#include	<dbdbms.h>
#include	<cs.h>
#include	<cv.h>
#include	<dm.h>
#include	<er.h>
#include	<lo.h>
#include	<si.h>
#include	<sl.h>
#include	<me.h>
#include	<st.h>
#include	<duerr.h>
#include	<duvfiles.h>
#include	<dudbms.h>
#include	<duve.h>
#include	<duustrings.h>

    exec sql include SQLCA;	/* Embedded SQL Communications Area */


/**
**
**  Name: DUVESTAR.SC -   Star specific verifydb functionality
**
**  Description:
**	This module contains star-specific verifydb functionality.  Currently,
**	this includes the following functions:
**
**	    duve_refresh    -	performs the refresh_ldbs operation
**
**	All of the functions in this module rely on data contained in the
**	verifydb control block (duve_cb).  This control block contains the
**	following information:
**	    information about input parameters:
**		mode	(RUN, RUNSILENT, RUNINTERACTIVE, REPORT)
**		scope	(DBNAME, DBA, INSTALLATION)
**		lists of databases to operate on
**		operation (DBMS_CATALOGS, FORCE_CONSISTENT, etc)
**		user code/name (-U followed by user name)
**	        special (undocumented) flags (-r,-d)
**	    information about the verifydb task environment:
**		name of user who evoked task
**		name of utility (VERIFYDB)
**		error message block
**	    ESQL specific information:
**		name of current open database
**	    system catalog check specific information:
**		pointer to duverel array
**		pointer to duvedpnds array
**		number of elements of duverel actually containing valid data
**		DUVEREL element contains iirelation info:
**		    table id (base and index id)
**		    table name
**		    owner name
**		    table's status word
**		    # attributes in table
**		    flag indicating if this table is to be dropped from database
**
**  History:
**      08-Jul-93 (teresa)
**	    Initial Creation
**      28-sep-96 (mcgem01)
**              Global data moved to duvedata.c
**	18-sep-2003 (abbjo03)
**	    Remove unnecessary lg.h header.
**	19-jan-2005 (abbjo03)
**	    Remove dmf_svcb. Change duvecb to GLOBALREF.
**/


/*
**  Forward and/or External typedef/struct references.
*/
    GLOBALREF DUVE_CB duvecb;


/*
**  Forward and/or External function references.
*/
    /* external */
FUNC_EXTERN DU_STATUS duve_log();	/* write string to log file */
FUNC_EXTERN DU_STATUS duve_talk();      /* verifydb communications routine */

    /* forward */
FUNC_EXTERN DU_STATUS duve_refresh();	/* perform refresh_ldbs operation */
FUNC_EXTERN VOID set_session();
FUNC_EXTERN VOID set_cap();
FUNC_EXTERN VOID get_cap();

/*{
** Name: duve_refresh	- executive for refresh_ldbs operation.
**
** Description:
**
**	Star contains information about the LEVEL of each LDB, which it obtained
**	from the LDB's IIDBCAPABILITIES catalog.  The information about each
**	LDB is placed into the star iiddb_ldb_dbcaps catalog.  There are
**	currently the  following capabilities that reflect level that Star puts
**	into iiddb_ldb_dbcaps for each LDB:
**
**	    COMMON/SQL_LEVEL
**	    INGRES/QUEL_LEVEL
**	    INGRES/SQL_LEVEL
**	    STANDARD_CATALOG_LEVEL
**	    OPEN/SQL_LEVEL
**
**	duve_refresh will attempt to connect to each LDB listed in
**	iiddb_ldbids.  It obtains the LDB name, node and server type from
**	star catalog iiddb_ldbids (ldb_database, ldb_node, ldb_dbms).
**	Routine duve_refresh does a direct connect to each database and issues
**	the following queries:
**
**	    select cap_value from iidbcapabilities
**		where cap_capability = 'COMMON/SQL_LEVEL'
**	    select cap_value from iidbcapabilities
**		where cap_capability = 'INGRES/QUEL_LEVEL'
**	    select cap_value from iidbcapabilities
**		where cap_capability = 'INGRES/SQL_LEVEL'
**	    select cap_value from iidbcapabilities
**		where cap_capability = 'STANDARD_CATALOG_LEVEL'
**	    select cap_value from iidbcapabilities
**		where cap_capability = 'OPEN/SQL_LEVEL'
**
**	If the connect or query fails for any reason, refresh_ldbs prints a
**	warning and does not update iiddb_ldb_dbcaps for this database.  If
**	All queries succeed, then refresh_ldbs updates the iiddb_ldb_dbcaps
**	entries for this database.  Transaction consistency assures that the
**	entire set remains consistent (i.e., verifydb will never update part
**	of the set without updating the whole set.
**
** Inputs:
**      database                        name of database to check/fix catalogs
**      duve_cb                         verifydb control block contains
**	    duverel			    array of rel entries
**		rel				info about an iirelation tuple:
**		    du_tbldrop			    flag indicating to drop table
**		    du_id				    table base and index identifiers
**		    du_own			    table's owner
**		    du_tab			    name of table
**
** Outputs:
**	none.
**	Returns:
**	    DUVE_YES
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    tuples may be deleted from any of the dbms catalogs, if verifydb
**	    processing decides that they should be.
**
** History:
**      08-jul-93 (teresa)
**	    Initial Creation
[@history_template@]...
*/
DU_STATUS
duve_refresh(duve_cb, database)
DUVE_CB		*duve_cb;
char		*database;
{
#define	SLASH_STAR	"/star"
#define SLASH_SIZE	6	
	bool		skip_flag;
	bool		upper_flag;
        exec sql begin declare section;
	    i4	sessid;
    	    char	starname[DB_MAXNAME + SLASH_SIZE];
	    char	ldb_name[DB_MAXNAME+1];
	    char	node_name[DB_MAXNAME+1];
	    char	svr_type[DB_MAXNAME+1];
	    i4		ldb_id;
	    char	name_case[DB_MAXNAME+1];
	    char	cmn_sql_lvl[DB_MAXNAME+1];
	    char	open_sql_lvl[DB_MAXNAME+1];
	    char	ingres_sql_lvl[DB_MAXNAME+1];
	    char	std_cat_lvl[DB_MAXNAME+1];
	    char	ingquel_lvl[DB_MAXNAME+1];
        exec sql end declare section;
	EXEC SQL WHENEVER SQLERROR goto SQLerr;


	/* build star database name */
	STpolycat(2, database, SLASH_STAR, starname);

	/* open star database as remote query session */
	sessid = DUVE_SLAVE_SESSION;
	EXEC SQL connect :starname session :sessid identified by '$ingres';
	duve_cb->duve_sessid = sessid;
	IIlq_Protect(TRUE);

	/* open star database as master update session */
	sessid = DUVE_MASTER_SESSION;
	EXEC SQL connect :starname session :sessid identified by '$ingres';
	duve_cb->duve_sessid = sessid;

	EXEC SQL WHENEVER SQLERROR call Clean_Up;

	EXEC SQL DECLARE ldb_cursor CURSOR FOR 
	     select ldb_node, ldb_dbms, ldb_database, ldb_id from iiddb_ldbids;
	EXEC SQL OPEN ldb_cursor;

	for (;;)
	{
	    set_session(DUVE_MASTER_SESSION,duve_cb);
	    skip_flag = FALSE;	/* assume we will operate on this LDB */
	    EXEC SQL FETCH ldb_cursor INTO :node_name, :svr_type,
					   :ldb_name, :ldb_id;
	    if (sqlca.sqlcode == DU_SQL_NONE)
	    {
		EXEC SQL CLOSE ldb_cursor;
		break;
	    }
	    else if (sqlca.sqlcode < DU_SQL_OK)
		Clean_Up();

	    set_session(DUVE_SLAVE_SESSION,duve_cb);

	    exec sql select cap_value into :name_case from iiddb_ldb_dbcaps
		where ldb_id = :ldb_id and cap_capability = 'DB_NAME_CASE';
	    CVlower(name_case);
	    upper_flag = (MEcmp(name_case,"upper",5) == DU_IDENTICAL);

	    exec sql commit;	/* commits required before direct connect */
	    EXEC SQL WHENEVER SQLERROR continue;
	    exec sql direct connect with    node = :node_name,
					    database = :ldb_name,
					    dbms = :svr_type;

	    /* if we could not direct connect to this LDBMS, continue to the
	    **	next one
	    */
	    if (sqlca.sqlcode < 0)
		continue;

	    /* COMMON/SQL_LEVEL */
	    get_cap("COMMON/SQL_LEVEL", cmn_sql_lvl, upper_flag, &skip_flag);

	    /* OPEN/SQL_LEVEL */
	    get_cap("OPEN/SQL_LEVEL", open_sql_lvl, upper_flag, &skip_flag);

	    /* INGRES/SQL_LEVEL */
	    get_cap("INGRES/SQL_LEVEL", ingres_sql_lvl, upper_flag, &skip_flag);
	    
	    /* STANDARD_CATALOG_LEVEL */
	    get_cap("STANDARD_CATALOG_LEVEL", std_cat_lvl, upper_flag,
		    &skip_flag);

	    /* INGRES/QUEL_LEVEL */
	    get_cap("INGRES/QUEL_LEVEL", ingquel_lvl, upper_flag, &skip_flag);

	    exec sql direct disconnect;

	    /*
	    ** now handle each capability in STAR CATALOG IIDDB_LDB_DBCAPS
	    */
	    if (skip_flag)
		continue;

	    /* COMMON/SQL_LEVEL */
	    set_cap(duve_cb, ldb_id, node_name, ldb_name,
		    "COMMON/SQL_LEVEL", cmn_sql_lvl);
	    /* OPEN/SQL_LEVEL */
	    set_cap(duve_cb, ldb_id, node_name, ldb_name,
		    "OPEN/SQL_LEVEL", open_sql_lvl);
	    /* INGRES/SQL_LEVEL */
	    set_cap(duve_cb, ldb_id, node_name, ldb_name,
		    "INGRES/SQL_LEVEL", ingres_sql_lvl);
	    /* STANDARD_CATALOG_LEVEL */
	    set_cap(duve_cb, ldb_id, node_name, ldb_name,
		    "STANDARD_CATALOG_LEVEL", std_cat_lvl);
	    /* INGRES/QUEL_LEVEL */
	    set_cap(duve_cb, ldb_id, node_name, ldb_name,
		    "INGRES/QUEL_LEVEL", ingquel_lvl);

	}   /* end fetch loop */

	return DUVE_YES;	

SQLerr:
    /* if we get here, we couldn't open the Star database. */
    duve_talk(DUVE_ALWAYS, duve_cb, E_DU501A_CANT_CONNECT,2, 0, database);
    return ( (DU_STATUS) DUVE_BAD);
}

/*{
** Name: get_cap    -  Reads a cap_value from iidbcapabilities
**
** Description:
**
**	This queries iidbcapabilities to get cap_value for a specified
**	cap_capability.  If an sql error is encountered while querying,
**	the input boolean skip_flag is set to true.
**
**	Some databases expect table and column names in upper case.  If that
**	is the case, then input  parameter upper_flag must be set to TRUE, and
**	it must be set to false if the catalog and column names are to
**	be specified in lower case (the usual case).
**
** Inputs:
**      capability		name of cap_capability to get cap_value for
**	value			ptr to ASCII buffer to hold cap_value
**	upper_flag		True if we must specify table and column
**				    names in upper case
**	err_flag		ptr to a boolean
** Outputs:
**	value			cap_value for user specified capability.
**				 NOTE: element 0 will contain '\0' if specified
**				       cap_capability is not in iidbcapabilities
**	err_flag		Set to TRUE if error is encountered.  Assumed
**				    initialized to false.  NOT SET if no error
**				    is encountered.
**	
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    None.
**
** History:
**      13-jul-93 (teresa)
**	    Initial Creation
[@history_template@]...
*/
VOID
get_cap(capability, value, upper_flag, err_flag)
char	*capability;
char	*value;
bool	upper_flag;
bool	*err_flag;
{

    exec sql begin declare section;
	char	cap_value[DB_MAXNAME+1];
	char	cap_capability[DB_MAXNAME+1];
    exec sql end declare section;
    
    exec sql whenever sqlerror continue;

	STzapblank(capability, cap_capability);

	if (upper_flag)
	{
	    exec sql select CAP_VALUE into :cap_value
	    from IIDBCAPABILITIES where
		CAP_CAPABILITY = :cap_capability;
	}
	else
	{
	    exec sql select cap_value into :cap_value
	    from iidbcapabilities where
		cap_capability = :cap_capability;
	}
	if (sqlca.sqlcode < 0)
	{
	    /* an error occurred during fetch, so abort this LDB */
	    *err_flag = TRUE;
	}
	if (sqlca.sqlcode == DU_SQL_NONE)
	{
	    /* this capability was not listed in iidbcapabilities, so
	    ** clear the entry for this variable */
	    cap_value[0] = '\0';
	}

	STzapblank(cap_value, value);
}

/*{
** Name: set_session	-  Assures SQL is attached to specified session
**
** Description:
**
**	Verifydb can have two sessions, a master and a slave.  The
**	duve_cb contains a field indicating which session it is in.  If
**	the caller requests the current sessin, set_session merely returns.
**	Otherwise, it sets up the correct session and sets the
**	duve_cb->duve_sessid to indicate the new session.
**
** Inputs:
**      sess_id			id of desired desired:
**				    DUVE_MASTER_SESSION or DUVE_SLAVE_SESSION
**	duve_cb			verifydb control block
**	    .duve_sessid	value of current session:
**				    DUVE_MASTER_SESSION or DUVE_SLAVE_SESSION
** Outputs:
**	none.
**	Returns:
**	    DUVE_YES
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    Will abort if SQL does not honor set_sql command
**
** History:
**      13-jul-93 (teresa)
**	    Initial Creation
[@history_template@]...
*/
VOID
set_session(sess_id, duve_cb)
i4		sess_id;
DUVE_CB		*duve_cb;
{
    exec sql begin declare section;
	i4	sessid;
    exec sql end declare section;
    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    if (duve_cb->duve_sessid == sess_id)
	return;

    sessid = sess_id;
    exec sql set_sql (session = :sessid);
    duve_cb->duve_sessid = sess_id;
}

/*{
** Name: set_cap    -  Update iiddb_ldb_dbcaps
**
** Description:
**
**	Determine whether or not this capability is already in iiddb_ldb_dbcaps.
**	If so, update it to new value.  If not, then insert it.
**
** Inputs:
**	duve_cb			verifydb control block (used for messages)
**	ldb			unique ldb identifier (integer 4)
**	ldb_name		name of registered LDB.
**	node_name		name of node LDB is registered from
**	capability		cap_capability to update (null terminated str)
**	value			new cap_value (null terminated string)
** Outputs:
**	none.
**	Returns:
**	    DUVE_YES
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    Will abort if SQL error is encountered
**
** History:
**      13-jul-93 (teresa)
**	    Initial Creation
[@history_template@]...
*/
VOID
set_cap(duve_cb, ldb, node_name, ldb_name, capability, value)
DUVE_CB		*duve_cb;
i4		ldb;
char		*node_name;
char		*ldb_name;
char		*capability;
char		*value;
{
    exec sql begin declare section;
	char	cap_capability[DB_MAXNAME+1];	/*name of capability to update*/
	char	cap_value[DB_MAXNAME+1];	/*current value of capability */
	char	new_value[DB_MAXNAME+1];       /*value to update capability to*/
	i4	ldb_id;				/* unique identifier for LDB */
	i4	cap_level;			/* numeric representation of
						** new_value*/
    exec sql end declare section;

    exec sql whenever SQLERROR call Clean_Up;

    do
    {	/* control loop */

	ldb_id = ldb;
	_VOID_ STzapblank(capability,cap_capability);
	_VOID_ STzapblank(value, new_value);
	_VOID_ STtrmwhite(ldb_name);
	_VOID_ STtrmwhite(node_name);

	if (new_value[0] == '\0')
	{
	    /* the Local database does not report this capability */
	    duve_talk(DUVE_MODESENS, duve_cb, S_DU1707_LCL_CAP_UNKNOWN, 6,
		     0, cap_capability, 0, ldb_name, 0, node_name);
	    break;
	}
	else
	{
	    if (
		  (STcompare(cap_capability,"COMMON/SQL_LEVEL") == DU_IDENTICAL)
		||
		  (STcompare(cap_capability,"OPEN/SQL_LEVEL") == DU_IDENTICAL)
		||
		  (STcompare(cap_capability,"INGRES/SQL_LEVEL") == DU_IDENTICAL)
		||
		  (STcompare(cap_capability,"STANDARD_CATALOG_LEVEL")==DU_IDENTICAL)
	       )
		CVal(new_value, &cap_level);
	    else	
		cap_level = 0;
	}

	exec sql select cap_value into :cap_value from iiddb_ldb_dbcaps where
	    cap_capability = :cap_capability and ldb_id = :ldb_id;
	if (sqlca.sqlcode == DU_SQL_NONE)
	{
	    /* this capability was not listed in iidbcapabilities, so
	    ** add it now */
	    duve_talk(DUVE_MODESENS, duve_cb, S_DU1708_LCL_CAP_MISSING, 8,
		      0, cap_capability, 0, new_value,
		      0, ldb_name, 0, node_name);
	    if (duve_talk(DUVE_ASK, duve_cb, S_DU0430_INSERT_CAP, 2,
			  0, cap_capability, 0, new_value)
	       == DUVE_YES)
	    {
		exec sql insert into iiddb_ldb_dbcaps(ldb_id, cap_capability,
		     cap_value, cap_level) values (:ldb_id, :cap_capability,
    		     :new_value, :cap_level);
		duve_talk(DUVE_MODESENS, duve_cb, S_DU0431_CAP_INSERTED, 2,
			  0, cap_capability, 0, new_value);
	    }
	}
	else
	{

	    /* its already in iiddb_ldb_dbcaps, so update it if necesssary*/
	    _VOID_ STtrmwhite(cap_value);
	    if (STcompare(cap_value,new_value) != DU_IDENTICAL)
	    {
		duve_talk(DUVE_MODESENS, duve_cb, S_DU1709_LCL_CAP_DIFFERENT,10,
		      0, cap_capability, 0, new_value, 0, ldb_name,
		      0, node_name, 0, cap_value);
		if (duve_talk(DUVE_ASK, duve_cb, S_DU0432_UPDATE_CAP, 2,
			      0, cap_capability, 0, new_value)
		   == DUVE_YES)
		{
		    exec sql update iiddb_ldb_dbcaps set cap_value = :new_value,
			cap_level = :cap_level where
			cap_capability = :cap_capability and ldb_id = :ldb_id;
		    duve_talk(DUVE_MODESENS, duve_cb, S_DU0433_CAP_UPDATED, 2,
			  0, cap_capability, 0, new_value);
		} /* end if we have permission to update */
	    } /* end if the new value is not identical to the old value */
	}
    } while (0);    /* end control loop */

    return;
}
