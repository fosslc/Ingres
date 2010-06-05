/*
**Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <st.h>
#include    <me.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <tm.h>
#include    <er.h>
#include    <si.h>
#include    <st.h>
#include    <lo.h>
#include    <duf.h>
#include    <duerr.h>
#include    <duvfiles.h>
#include    <duve.h>
#include    <dudbms.h>
#include    <cui.h>	
#include    <cv.h>

/**
**
**  Name: DUVEUTIL.C - Utility routines for verifydb
**
**  Description:
**      Assorted utility routines used with ESQL utilities.
**	This includes:
**
**	cv_numbuf	- convert a number from ascii hex to DUF message id.
**	duve_basecnt	- count number of secondary indexes for a base table
**	duve_ckargs	- check the arguments on the verifydb input line
**      duve_dbfind	- search duvedbinfo structure for specified db name
** 	duve_d_cnt  	- count number of dependent objects which depend on a
**                        specified independent object
**	duve_d_dpndfind - search duvedpnds structure for specified deid, dtype,
**			  qid
**	duve_findreltid - search duverel structure for entry for specified
**		          iirelation tuple
**	duve_get_args   - get arguements from command input line
**      duve_i_dpndfind - search duvedpnds structure for specified inid, itype, 
**			  i_qid
**	duve_init	- verifydb initialization
**	duve_locfind	- search duve_locs structure for specified location
**			  name.
**	duve_name	- separate the list of names enclosed in quotes to
**			  an array of names (all null terminated)
**	duve_obj_type	- return a string describing a given object type (e.g.
**			  PROCEDURE for DB_DBP)
**	duve_qidfind    - search duverel structure for specified querryid
**	duve_relidfind  - search duverel structure for specified relid, relowner
**	duve_statfind   - search duvestat structure for specified table id and
**			  attribute
**	duve_usrfind	- search du_usrlst cache for user name
**
**
**  History:
**      23-Jun-1988 (teg)
**          Initial creation for verifydb
**	02-FEB-1989 (teg)
**	    Added duve_usrfind routine.
**	06-Feb-1989 (teg)
**	    Added duve_idfind and duve_dbfind routines.
**	20-apr-1990 (teg)
**	    added accesscheck support.
**	16-apr-92 (teresa)
**	    compare against 0 instead of NULL (ingres63p change 265521.
**	8-aug-93 (ed)
**	    unnest dbms.h
**	15-dec-93 (andre)
**	    defined duve_obj_type()
**	05-Jan-93 (teresa)
**	    wrote duve_basecnt()
**	19-may-94 (vijay)
**	    Fix a typo. Should be comparing to 'u' and not '-u'. compiler
**	    complains.
**	27-dec-1995 (nick)
**	    Changes for FIPS ; case sensitivity matters so don't automatically
**	    lowercase the username of the person running 'verifydb'.
**      21-apr-1999 (hanch04)
**          replace STindex with STchr
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Include cv.h for function defintions
**/


/*
**  Forward and/or External function references.
*/

FUNC_EXTERN VOID      cv_numbuf();	/* convert errno to hex from ASCII */
FUNC_EXTERN VOID      du_reset_err();	/* clear error control block */
FUNC_EXTERN i2	      duve_basecnt();	/* count 2ndary indexes for base table*/
FUNC_EXTERN DU_STATUS duve_findreltid();/* search duverel cache for table id */
FUNC_EXTERN DU_STATUS duve_get_args();  /* command line parser */
FUNC_EXTERN DU_STATUS duve_init();      /* initialize verifydb */
FUNC_EXTERN DU_STATUS duve_relidfind(); /* search duverel cache for table name*/
FUNC_EXTERN DU_STATUS duve_qidfind();	/* search duverel cache for querry id */
FUNC_EXTERN DU_STATUS duve_d_dpndfind();/* search duvedpnd cache for depend id*/
FUNC_EXTERN DU_STATUS duve_i_dpndfind();/* search duvedpnd cache for indep id*/
FUNC_EXTERN DU_STATUS duve_statfind();	/* search duvestat cache for table id*/
FUNC_EXTERN DU_STATUS duve_locfind();	/* search duve_locs cache for location*/
FUNC_EXTERN DU_STATUS duve_usrfind();	/* search du_usrlst cache for name*/
FUNC_EXTERN DU_STATUS duve_idfind();	/* search duvedbinfo cache for db id */
FUNC_EXTERN DU_STATUS duve_dbfind();	/* search duvedbinfo cache db name */
FUNC_EXTERN bool      duve_nolog();	/* check for nolog flag */

/*
**  Definition of static variables and forward static functions.
*/

static  DU_STATUS   duve_ckargs();      /* determine numeric value assoc with
					** input parameter
					*/
static  i4          duve_name();        /* parse name string from input
					** command line parameter */
static	bool	    same_installation(); /* all dbnames are in the same
					 ** installation ?
					 */

/*{
** Name: cv_numbuf - convert a number from ascii hex to DUF message id.
**
** Description:
**      converts a number from ascii hex to a DUF message id, allowing for 
**	non-hex characters.
**
** Inputs:
**	str		    string to convert
** Outputs:
**	num		    variable to put converted 
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-dec-1993 (teresa)
**          initial creation
[@history_template@]...
*/
VOID 
cv_numbuf(str, num)
char	    *str;
DU_STATUS   *num;
{
    i4 val;
    i4	    i;
    
    for ( val=0, i=0; str[i] != '\0'; i++)
    {
	val *= 16;
	switch (str[i])
	{
	    case '0':
		break;
	    case '1':
		val += 1;
		break;
	    case '2':
		val += 2;
		break;
	    case '3':
		val += 3;
		break;
	    case '4':
		val += 4;
		break;
	    case '5':
		val += 5;
		break;
	    case '6':
		val += 6;
		break;
	    case '7':
		val += 7;
		break;
	    case '8':
		val += 8;
		break;
	    case '9':
		val += 9;
		break;
	    case 'A':
	    case 'a':
		val += 10;
		break;
	    case 'B':
	    case 'b':
		val += 11;
		break;
	    case 'C':
	    case 'c':
		val += 12;
		break;
	    case 'D':
	    case 'd':
		val += 13;
		break;
	    case 'E':
	    case 'e':
		val += 14;
		break;
	    case 'F':
	    case 'f':
		val += 15;
		break;
	    default:
		/* was not a number, so don't multiply by 10 after all */
		val = val / 16;
	}
    }
    val +=    E_DUF_MASK;
    *num = (DU_STATUS ) val;
}

/*{
** Name: duve_basecnt - count number of secondary indexes that have the same
**		        base table.
**
** Description:
**	Verifydb's sys catalog check function gathers information on each
**	iirelation tuple as it examines them.  It keeps a subset of the
**	iirelation tuple in memory in a structure called the duverel structure.
**	duverel is an array of rel structs, each contains the table id, the
**	table's name, owner and status.  It also contains a flag (du_tbldrop)
**	which indicates if this table is to be dropped from the database.
**	Then entries in duverel are ordered by table id.
**
**	This routine searches duverel for all entries that hvae the specified
**	base id and a non-zero index id.  When it finds a tuple that meets
**	this criteria, it checks to see if the tuple is marked for drop 
**	(du_droptbl = DUVE_DROP).  It will count tuples marked for drop.
**
** Inputs:
**      matchid				table id base identifier (u_i4)
**	duve_cb				DUVERIFY control block, contains:
**	    duve_cnt		        number of entries in duverel with
**					 valid data.
**	    duverel			array of info about iirelation tuples
**	       rel			info about 1 iirelation tuple, contains:
**					    table id
**					    table name
**					    table owner
**					    table status
**					    number attributes (columns) in table
**					    drop_table flag
**
** Outputs:
**	none.
** Returns:
**	count	    (i2) count of the number of valid secondary indexes that
**		    match the specified base table id.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      05-jan-94 (teresa)
**          initial creation
*/
i2
duve_basecnt(matchid, duve_cb)
u_i4		    matchid;
DUVE_CB		    *duve_cb;
{
    i4 i;
    i2	    count = 0;

    for (i= 0; i < duve_cb->duve_cnt; i++)
    {
	if (
	      (duve_cb->duverel->rel[i].du_id.db_tab_base == matchid)
	    &&
	      (duve_cb->duverel->rel[i].du_id.db_tab_index) 
	    &&
	      ( !(duve_cb->duverel->rel[i].du_tbldrop) )
	   )
	      count++;
    } /* end search loop */

    return (count);
}

/*{
** Name: duve_ckargs	- check the arguments on the verifydb input line
**
** Description:
**      Verifydb is evoked with the following arguements:
**	    -m<mode> where mode is one of:
**		report         - examine, but dont take any corrective action
**		run            - take corrective action
**		runsilent      - take corrective action, but dont report them to
**			         screen
**		runinteractive - display suggested corrective action, then 
**				 prompt for yes/no permission to do so
**		test_runi      - simulate RUNINTERACTIVE mode with "YES"
**			         answers.  To be used only by DEVELOPMENT and
**				 TEST ENGINEERING for test purposes.
**	    -s<scope> where scope is one of:
**		dbname "name [name]" - operate on specified database(s) only
**		dba		     - operate on all databases owned by dba
**		installation	     - operate on all databases in installation
**	    -o<operation> where operation is one of:
**		dbms_catalogs	- operate on dbms catalogs
**		force_consistent- force database to look consistent
**		purge		- purge expired relations and temp/unknown files
**		temp_purge	- purge temp/unknown files
**		expired_purge	- purge expired relations
**		table "name"    - operate on table specified by name.
**		xtable "name"   - operate on table specified by name.
**		drop_table "name" - drops a table from dbms system catalogs.
**		accesscheck	- check that it is possible to connect to
**				  the target dbs   Use the scope command to
**				  specify target dbs.
**		refresh_ldbs	- assure iiddb_ldb_dbcaps correctly reflects
**				  capability levels from LDB's iidbcapabilities.
**	    -u<user_name> is optional -  Allows user to pretend they're another
**					 user
**	    -au<access_user> is optional - specifies which uses to connect as 
**					   for the access check operation.  IF
**					   -au is NOT specified, will defautl to
**					   connecting as user specified by -u.
**					   If -u is also unspecified, will
**					   connect as user evoking VERIFYDB.
**
**
**  this translates into the following common (from v5.0) operations:
**	Refresh LDBS:
**	    verifydb -mreport -sbname "name" -orefresh_ldbs
**	    verifydb -mrun -sbname "name" -orefresh_ldbs
**	    verifydb -mrunsilent -sbname "name" -orefresh_ldbs
**	    verifydb -runinteractive -sbname "name" -orefresh_ldbs
**	    verifydb -mreport -sdba -orefresh_ldbs
**	    verifydb -mrun -sdba -orefresh_ldbs
**	    verifydb -mrunsilent -sdba -orefresh_ldbs
**	    verifydb -runinteractive -sdba -orefresh_ldbs
**	    verifydb -mreport -sinstallation -orefresh_ldbs
**	    verifydb -mrun -sinstallation -orefresh_ldbs
**	    verifydb -mrunsilent -sinstallation -orefresh_ldbs
**	    verifydb -runinteractive -sinstallation -orefresh_ldbs
**	patch database config file: [new]
**	    verifydb -mrun -sdbname "name" -oforce_consistent
**	Check system catalogs: (extended restoredb)
**	    verifydb -mreport -sdbname "name" -odbms_catalogs
**	    verifydb -mreport -sdba -odbms_catalogs
**	    verifydb -mreport -sinstallation -odbms_catalogs
**	Restore system catalogs (extended restoredb):
**	    verifydb -mrun -sdbname "name" -odmbs_catalogs
**	    verifydb -mrun -sdba -odmbs_catalogs
**	    verifydb -mrun -sinstallation -odbms_catalogs
**	    verifydb -mrunsilent -sdbname "name" -odmbs_catalogs
**	    verifydb -mrunsilent -sdba -odmbs_catalogs
**	    verifydb -mrunsilent -sinstallation -odmbs_catalogs
**	    verifydb -mruniteractive -sdbname "name" -odmbs_catalogs
**	Purgedb:
**	    verifydb -mrun -sdbname "name" -opurge
**	    verifydb -mrun -sdbname "name" -otemp_purge
**	    verifydb -mrun -sdbname "name" -oexpired_purge
**	    verifydb -mrun -sdba -opurge
**	    verifydb -mrun -sdba -otemp_purge
**	    verifydb -mrun -sdba -oexpired_purge
**	    verifydb -mrun -sinstallation -opurge
**	    verifydb -mrun -sinstallation -otemp_purge
**	    verifydb -mrun -sinstallation -oexpired_purge
**	    verifydb -mrunsilent -sdbname "name" -opurge
**	    verifydb -mrunsilent -sdbname "name" -otemp_purge
**	    verifydb -mrunsilent -sdbname "name" -oexpired_purge
**	    verifydb -mrunsilent -sdba -opurge
**	    verifydb -mrunsilent -sdba -otemp_purge
**	    verifydb -mrunsilent -sdba -oexpired_purge
**	    verifydb -mrunsilent -sinstallation -opurge
**	    verifydb -mrunsilent -sinstallation -otemp_purge
**	    verifydb -mrunsilent -sinstallation -oexpired_purge
**	    verifydb -mruninteractive -sdbname "name" -opurge
**	    verifydb -mruninteractive -sdbname "name" -otemp_purge
**	    verifydb -mruninteractive -sdbname "name" -oexpired_purge
**	Checklink:
**	    verifydb -mreport -sdbname "name" -otable "name"
**	    verifydb -mreport -sdbname "name" -oxtable "name"
**	Patchlink:
**	    verifydb -mrun -sdbname "name" -otable "name"
**	    verifydb -mrun -sdbname "name" -oxtable "name"
**	    verifydb -mrunsilent -sdbname "name" -otable "name"
**	    verifydb -mrunsilent -sdbname "name" -oxtable "name"
**	Accesscheck:
**	    verifydb -mreport -sdbname name -oaccesscheck
**	    verifydb -mreport -sdbname "name .. name" -oaccesscheck
**	    verifydb -mreport -sdba -oaccesscheck
**	    verifydb -mreport -sinstallation -oaccesscheck
**	    verifydb -mreport -sdba -oaccesscheck -uteg -aufred
**	    verifydb -mreport -sdba -oaccesscheck
**	    verifydb -mreport -sinstallation -oaccesscheck
**
**
**  NOTE:   If any of these flags are specified more than once, only the
**	    last specification will be used.  No error will be reported.
**	    If the -m, -s and -o flags are not specified, an error will be
**	    generated.
**	
**
** Inputs:
**	    
**      arg_type                        type of argument: 
**					    (DU_MODE, DU_SCOPE, DU_OPER)
**	arg_value			string containing keyword
**					    NOTE: arg value is expected in lower
**						  case only.
**
** Outputs:
**      none, but return code
**
**	Returns:
**	    type code:			Is a value indicating the type of key
**					that was supplied with the flag.  
**					Possible values are:
**					    Error Conditions:
**						    DU_INVALID
**						    DU_NOSPECIFY
**					    Mode Values:
**						    DUVE_RUN
**						    DUVE_SILENT
**						    DUVE_IRUN
**						    DUVE_REPORT
**					    Scope Values:
**						    DUVE_SPECIFY
**						    DUVE_DBA
**						    DUVE_INSTLTN
**					    Operation Values:
**						    DUVE_CATALOGS
**						    DUVE_MKCONSIST
**						    DUVE_PURGE
**						    DUVE_TEMP
**						    DUVE_EXPIRED
**						    DUVE_TABLE
**						    DUVE_XTABLE
**						    DUVE_ACCESS
**						    DUVE_REFRESH
**
**	Exceptions:
**	    none.
**
** Side Effects:
**	    none.
**
** History:
**      23-Jun-1988 (teg)
**          initial creation
**	06-Feb-1989 (teg)
**	    added test simulation of RUNI with YES answer (-mTEST_RUNI)
**	14-Sep-1989 (teg)
**	    Added XTABLE support.
**	28-mar-1990 (teg)
**	    added ACCESSCHECK support.
**	13-jul-93 (teresa)
**	    add support for refresh_ldbs
*/
static DU_STATUS
duve_ckargs ( arg_type, arg_val, duve_cb)
i4	arg_type;	    /* type of argument: (DU_MODE, DU_SCOPE, DU_OPER) */
char	*arg_val;	    /* string containing keyword - LOWERCASE */
DUVE_CB *duve_cb;	    /* verifydb control block */
{

    switch (arg_type)
    {

    case DU_MODE:
	if(MEcmp((PTR)arg_val, (PTR)"runs", (u_i2)4)==DU_IDENTICAL)
	    return ( (DU_STATUS) DUVE_SILENT);
	if(MEcmp((PTR)arg_val, (PTR)"runi", (u_i2)4)==DU_IDENTICAL)
	    return ( (DU_STATUS)DUVE_IRUN);
	if(MEcmp((PTR)arg_val, (PTR)"run", (u_i2)3)==DU_IDENTICAL)
	    return ( (DU_STATUS)DUVE_RUN);
	if(MEcmp((PTR)arg_val, (PTR)"rep", (u_i2)3)==DU_IDENTICAL)
	    return( (DU_STATUS)DUVE_REPORT);
	if(MEcmp((PTR)arg_val, (PTR)"test_runi", (u_i2)9)==DU_IDENTICAL)
	{   
	    duve_cb->duve_simulate = TRUE;
	    return ( (DU_STATUS)DUVE_IRUN);
	}
        else
	{
	    duve_talk(DUVE_ALWAYS, duve_cb, 
		    E_DU5001_INVALID_MODE_FLAG, 2, 0, arg_val);
	    return ( (DU_STATUS)DU_INVALID);
	}
	break;
    case DU_SCOPE:
	if(MEcmp((PTR)arg_val, (PTR)"dbn", (u_i2)3)==DU_IDENTICAL)
	    return ( (DU_STATUS)DUVE_SPECIFY);
	else if (!(STcompare(arg_val, "dba")) )
	    return ( (DU_STATUS)DUVE_DBA);
	if(MEcmp((PTR)arg_val, (PTR)"ins", (u_i2)3)==DU_IDENTICAL)
	    return ( (DU_STATUS)DUVE_INSTLTN);
        else
	{
	    duve_talk(DUVE_ALWAYS, duve_cb, 
		    E_DU5002_INVALID_SCOPE_FLAG, 2, 0, arg_val);
	    return ( (DU_STATUS)DU_INVALID);
	}
	break;
    case DU_OPER:
	if(MEcmp((PTR)arg_val, (PTR)"ref", (u_i2)3)==DU_IDENTICAL)
	    return ( (DU_STATUS)DUVE_REFRESH);
	if(MEcmp((PTR)arg_val, (PTR)"dbm", (u_i2)3)==DU_IDENTICAL)
	    return ( (DU_STATUS)DUVE_CATALOGS);
	if(MEcmp((PTR)arg_val, (PTR)"for", (u_i2)3)==DU_IDENTICAL)
	    return ( (DU_STATUS)DUVE_MKCONSIST);
	if(MEcmp((PTR)arg_val, (PTR)"pur", (u_i2)3)==DU_IDENTICAL)
	    return ( (DU_STATUS)DUVE_PURGE);
	if(MEcmp((PTR)arg_val, (PTR)"tem", (u_i2)3)==DU_IDENTICAL)
	    return ( (DU_STATUS)DUVE_TEMP);
	if(MEcmp((PTR)arg_val, (PTR)"exp", (u_i2)3)==DU_IDENTICAL)
	    return ( (DU_STATUS)DUVE_EXPIRED);
	if(MEcmp((PTR)arg_val, (PTR)"tab", (u_i2)3)==DU_IDENTICAL)
	    return ( (DU_STATUS)DUVE_TABLE);
	if(MEcmp((PTR)arg_val, (PTR)"xta", (u_i2)3)==DU_IDENTICAL)
	    return ( (DU_STATUS)DUVE_XTABLE);
	if(MEcmp((PTR)arg_val, (PTR)"dro", (u_i2)3)==DU_IDENTICAL)
	    return ( (DU_STATUS)DUVE_DROPTBL);
	if(MEcmp((PTR)arg_val, (PTR)"acc", (u_i2)3)==DU_IDENTICAL)
	    return ( (DU_STATUS)DUVE_ACCESS);
        else
	{
	    duve_talk(DUVE_ALWAYS, duve_cb, 
		    E_DU5003_INVALID_OPERATION_FLAG, 2, 0, arg_val);
	    return ( (DU_STATUS)DU_INVALID);
	}
	break;
    default:
	{
	    return ( (DU_STATUS)DU_NOSPECIFY);
	}
	break;
    } /* endcase */

}

/*{
**      duve_dbfind	- search duvedbinfo structure for specified db name
**
** Description:
**	Verifydb's iidbdb catalog check function gathers all databases known to
**	the installation from the iidbdb catalog iidatabase.  Pertentent info
**	from the iidatabase catalog is cached in the duvedbinfo cache, which is
**	ordered by database name.
**
**	This routine searches the cache for database name, which is in ascending
**	alphabetical order, so it uses a binary search.  If a match is
**	found, it checks to assure the cache entry is valid (du_badtuple is
**	false.)
**	If the cache entry is valid, it returns the offset to the desired 
**	location in the cache. Otherwise it returns DU_INVALID. 
**
** Inputs:
**      search_dbid			Database id to search for
**	duve_cb				DUVERIFY control block, contains:
**	   duve_dbcnt			    Num of entries in iidatabase cache
**	   duvedbinfo			    IIDATABASE CACHE
**	      du_id				One entry on cache
**		du_database			    database name
**		du_badtuple			    flag indicating invalid entry
**
** Outputs:
**	none, just return
** Returns:
**      pointer to desired duvedbinfo entry -- possible values:  
**		zero, positive : match found and is pointer to duvedbinfo entry
**		DU_INVALID (negative) : no match found
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      06-feb-1989 (teg)
**          initial creation
*/
DU_STATUS
duve_dbfind (searchdb, duve_cb)
char		    *searchdb;
DUVE_CB            *duve_cb;
{
    DU_STATUS	 high, low, middle;
    u_i4	 len;
    i4		 compare_result;

    len = STtrmwhite( searchdb);
    if (len > DB_MAXNAME)
        return ( (DU_STATUS) DU_INVALID);

    /* data is ordered, so use binary search
    ** initialize binary search loop  -- this cache is built with duve_ucnt
    **					 pointing to the 1st empty cache
    **					 entry, so decrement high before using.
    */
    high = duve_cb->duve_dbcnt - 1;
    if (high<0) /* empty cache */
	return ( (DU_STATUS) DU_INVALID);
    low  = 0;

    /* 
    ** binary search loop -- if we drop out, then the search value was 
    ** not found 
    */
    while ( low <= high)
    {
	middle = (low + high) / 2;
	compare_result=STcompare(duve_cb->duvedbinfo->du_db[middle].du_database,
		        searchdb);
	if (compare_result > DU_IDENTICAL)
	{   /* search point was too high, adjust downwards */
	    high = middle - 1;
	    continue;
	}
	else if ( compare_result < DU_IDENTICAL )
	{   /* search point was too low, adjust upwards */
	    low = middle + 1;
	    continue;
	}

	/* if we get here, then we have a match. 
	** Assure the tuple is still valid
	*/
	if (duve_cb->duvedbinfo->du_db[middle].du_badtuple)
	{
	     return ( (DU_STATUS) DU_INVALID);
	}
	else 
	{
	    return ( middle);
	}

    } /* end binary search loop */

    /* 
    ** we only fall out of loop if search value not found, so return not found
    */
    return ( (DU_STATUS) DU_INVALID);
}

/*{
** Name: duve_d_cnt  - count number of dependent objects which depend on a 
**		       specified independent object
**
** Description:
**	This function will use iidbdepends cache to compute number of object
**	which depend on a given object.  If the caller supplies type of 
**	dependent object in which he is interested, we will only count objects 
**	of that type; otherwise we will simply count all objects which depend 
**	on the given independent object
**
**	Caller may supply us with offset into independent object info list, in 
**	which case we will disregard values specified in inid1, inid2, itype, 
**	iqid and will use values found in the specified independent object 
**	info element; otherwise, we will invoke duve_i_dpndfind() to find an 
**	element which matches specified inid1, inid2, itype, iqid.
**
** Inputs:
**	i_offset			offset into the independent object info 
**					list at which an element describing the
**					independent object resides; 
**					must be set to DU_INVALID if that 
**					offset is not known
**      inid1, inid2			independent object id; 
**					will be disregarded unless 
**					i_offset == DU_INVALID
**	itype				independent object type;
**                                      will be disregarded unless
**                                      i_offset == DU_INVALID
**	iqid				independent object secondary id;
**                                      will be disregarded unless
**                                      i_offset == DU_INVALID
**	dtype				type of dependent object which is of 
**					interest to us; may be set to DU_INVALID
**					if we are interested in a total number 
**					of objects dependent on a given object
**	duve_cb				DUVERIFY control block, contains:
**	    duvedpnds			structure describing IIDBDEPENDS cache
**	        duve_indep_cnt		number of elements in duve_indep list
**		duve_indep		list of independent object info elements
**		    du_inid		independent object id
**		    du_itype		independent object type
**		    du_iqid		independent object secondary id
**
** Outputs:
**	none, just return
**
** Returns:
**	number of objects (of specified type if dep_type != NULL) 
**		if a description of the independent object was found in the 
**		independent object info list
**	DU_INVALID
**		if a description of the independent object was not found in the
**		independent object info list
**
** Exceptions:
**	none
**
** Side Effects:
**	none
**
** History:
**	21-dec-93 (andre)
**	    written
**	17-jan-94 (anitap)
**	    Count the number of dependent objects which are actually present.
*/
DU_STATUS
duve_d_cnt(i_offset, inid1, inid2, itype, iqid, dtype, duve_cb)
i4		i_offset;
i4		inid1, inid2;
i4		itype;
i4		iqid;
i4		dtype;
DUVE_CB         *duve_cb;
{
    DU_I_DPNDS		*indep;
    DU_STATUS		cnt;

    if (i_offset != DU_INVALID)
    {
        /*
        ** caller specified offset of an independent object info element - 
	** verify that it points to a valid element of the independent object 
	** info list and initialize inid1, inid2, itype, and iqid
        */
	if (i_offset < 0 || i_offset >= duve_cb->duvedpnds.duve_indep_cnt)
	{
	    /* invalid offset - return DU_INVALID */
	    return((DU_STATUS) DU_INVALID);
	}

	/*
	** initialize inid1, inid2, itype, and iqid from the specified 
	** independent object info element
	*/
	indep = duve_cb->duvedpnds.duve_indep + i_offset;

	inid1 = indep->du_inid.db_tab_base;
	inid2 = indep->du_inid.db_tab_index;
	itype = indep->du_itype;
	iqid = indep->du_iqid;

	/*
	** now find the offset of the first element of the independent object 
	** info list corresponding to the object - since we have an offset to 
	** an element, finding the first element requires that we simply scan 
	** backwarss through the list looking for the first element which does 
	** not correspond to the object
	*/
	for (indep--; i_offset > 0; indep--, i_offset--)
	{
	    if (   inid1 != indep->du_inid.db_tab_base
		|| inid2 != indep->du_inid.db_tab_index
		|| itype != indep->du_itype
		|| iqid  != indep->du_iqid)
	    {
		break;
	    }
	}
    }
    else
    {
	/*
	** invoke duve_i_dpndfind() to find the first independent object info 
	** element corresponding to the specified object
	*/
	i_offset = duve_i_dpndfind(inid1, inid2, itype, iqid, duve_cb, 
	    (bool) TRUE);
	if (i_offset == DU_INVALID)
	{
	    /*
	    ** independent object info list does not contain a description for 
	    ** the specified object
	    */
	    return((DU_STATUS) DU_INVALID);
	}
    }

    for (indep = duve_cb->duvedpnds.duve_indep + i_offset, cnt = 0; 
	 i_offset < duve_cb->duvedpnds.duve_indep_cnt; 
	 indep++, i_offset++)
    {
	DU_I_DPNDS	*cur;

	/* 
	** verify that this independent object info element corresponds to the 
	** specified object
	*/
	if (   inid1 != indep->du_inid.db_tab_base
	    || inid2 != indep->du_inid.db_tab_index
	    || itype != indep->du_itype
	    || iqid  != indep->du_iqid)
	{
	    /* 
	    ** ran out of independent object info elements corresponding to the 
	    ** specified object - we are done
	    */
	    break;
	}

	/* 
	** if user wants a total number of dependent objects (regardless of 
	** type) which depend on specified independent object, increment cnt 
	** and proceed to the next element; otherwise, find dependent object 
	** info element with which this independent object info element is 
	** associated and increment cnt only if its type matches dtype
	*/
	if (dtype == DU_INVALID)
	{
	    cnt++;
	    continue;
	}

	/* 
	** find the dependent object info element to which this independent 
	** object info element belongs
	*/
	for (cur = indep; 
	     ~cur->du_iflags & DU_LAST_INDEP; 
	     cur = cur->du_next.du_inext)
	;

	/* 
	** cur now points at the last independent object info element in a ring 
	** associated with a given dependent object info element - 
	** cur->du_next.du_dep points at the dependent object info element
	*/
	/* We want to increment the count only if the dependent object is
        ** present.
        */
	if (dtype == cur->du_next.du_dep->du_dtype &&
	    ((cur->du_next.du_dep->du_dflags & DU_NONEXISTENT_DEP_OBJ) == 0))
	{
	    cnt++;
	}
    }

    return(cnt);
}

/*{
** Name: duve_d_dpndfind  - search duvedpnds structure for specified dependent 
**			    object info element
**
** Description:
**	Verifydb's sys catalog check function gathers information on each
**	iidbdeepnds tuple as it examines them.  It saves the tuple in its
**	internal cache described by duvedpnds structure.
**
**	This routine searches dependent object info list for an entry with a 
**	matching dependent table id, type and secondary id.  If the match is 
**	not found in the cache then duve_d_dpndfind returns DU_INVALID.  If it 
**	is found,  it returns the offset into dependent object info list where 
**	the match record resides.
**
**
** Inputs:
**      deid1, deid2			dependent object id to search for 
**	qtype				dependent object type
**	idnum				dependent object secondary id
**	duve_cb				DUVERIFY control block, contains:
**	    duvedpnds			structure describing IIDBDEPENDS cache
**	        duve_dep_cnt		number of elements in duve_dep list
**		duve_dep		list of dependent object info elements
**		    du_deid		dependent object id
**		    du_dtype		dependent object type
**		    du_dqid 		dependent object secondary id
**
** Outputs:
**	none, just return
** Returns:
**      if desired element was found, offset to desired duvedpnds.duve_dep entry
**	DU_INVALID otherwise
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      6-Sep-88	(teg)
**          initial creation
**	07-dec-93 (andre)
**	    duvedpnds is no longer an array of DU_DPNDS; instead, it is a 
**	    structure describing independent and dependent object info cache
**
**	    renamed duve_dpndfind() to duve_d_dpndfind() to help differentiate 
**	    it from duve_i_dpndfind()
*/
DU_STATUS
duve_d_dpndfind(deid1, deid2, qtype, idnum, duve_cb)
i4		deid1, deid2;
i4		qtype;
i4		idnum;
DUVE_CB         *duve_cb;
{
    i4	 	i;
    DU_D_DPNDS		*dep;
    
    /* data not ordered, so use linear search */
    for (i = 0, dep = duve_cb->duvedpnds.duve_dep; 
	 i < duve_cb->duvedpnds.duve_dep_cnt; 
	 i++, dep++)
    {
	if (   deid1 == dep->du_deid.db_tab_base
	    && deid2 == dep->du_deid.db_tab_index
	    && qtype == dep->du_dtype
	    && idnum == dep->du_dqid)
	{
	    return((DU_STATUS) i);
	}
    } /* end search loop */

    /* let the caller know that the search has failed */
    return((DU_STATUS) DU_INVALID);
}

/*{
**	duve_idfind	- search duvedbinfo structure for specified db id
**
** Description:
**	Verifydb's iidbdb catalog check function gathers all databases known to
**	the installation from the iidbdb catalog iidatabase.  Pertentent info
**	from the iidatabase catalog is cached in the duvedbinfo cache, which is
**	ordered by database name.
**
**	This routine searches the cache for database id, which is not in any
**	specific order.  Therefore, it uses a linear search.  If a match is
**	found,  it returns the offset to the desired location in the cache.
**	Otherwise it returns DU_INVALID. 
**
**	This routine does not validate the iidatabase cache entry because
**      the search is used by a secondary index.  We do NOT want to delete the
**      secondary index tuple independent from the base table, or DMF will not
**      permit us to delete the base table tuple afterwards.
**
** Inputs:
**      search_dbid			Database id to search for
**	start				Location to start search at (Zero for
**					  1st call for this dbid.  Nonzero for
**					  subsequent calls)
**	duve_cb				DUVERIFY control block, contains:
**	   duve_dbcnt			    Num of entries in iidatabase cache
**	   duvedbinfo			    IIDATABASE CACHE
**	      du_id				One entry on cache
**		du_dbid				    database id
**		du_badtuple			    flag indicating invalid entry
**
** Outputs:
**	none, just return
** Returns:
**      pointer to desired duvedbinfo entry -- possible values:  
**		zero, positive : match found and is pointer to duvedbinfo entry
**		DU_INVALID (negative) : no match found
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      06-feb-1989 (teg)
**          initial creation
*/
DU_STATUS
duve_idfind (search_dbid, start, duve_cb)
i4		    search_dbid;
DU_STATUS	    start;
DUVE_CB            *duve_cb;
{
    DU_STATUS    ret_val;
    i4	 i;

    /* data not ordered, so use linear search */
    for ( ret_val = DU_INVALID, i=(i4) start; i < duve_cb->duve_dbcnt; i++)
    {
	if (search_dbid != duve_cb->duvedbinfo->du_db[i].du_dbid)
	    continue;

	/* if we get here, we have a match on table id. */
        ret_val = i;
	break;

    } /* end search loop */

    return ( (DU_STATUS) ret_val);
}

/*{
** Name: duve_i_dpndfind  - search duvedpnds structure for an independent object
**			    info element corresponding to the specified object
**
** Description:
**	Verifydb's sys catalog check function gathers information on each
**	iidbdeepnds tuple as it examines them.  It saves the tuple in its
**	internal cache described by duvedpnds structure.
**
**	This routine uses binary search to search independent object info list 
**	for an entry with a matching independent table id, type and secondary 
**	id.  If the match is not found in the cache then duve_i_dpndfind returns
**	DU_INVALID.  If it is found, it returns the offset into independent 
**	object info list where the match record resides.  Caller may request 
**	that we return an offset to the first independent object info element 
**	corresponding to the given object, in which case we will scan the 
**	independent object info list backwards looking for the first element
**	corresponding to the specified object
**
** Inputs:
**      inid1, inid2			independent object id 
**	itype				independent object type
**	iqid				independent object secondary id
**	duve_cb				DUVERIFY control block, contains:
**	    duvedpnds			structure describing IIDBDEPENDS cache
**	        duve_indep_cnt		number of elements in duve_indep list
**		duve_indep		list of independent object info elements
**		    du_inid		independent object id
**		    du_itype		independent object type
**		    du_iqid		independent object secondary id
**	first				TRUE if the caller wants the offset to 
**					the first element corresponding to the 
**					specified object; 
**					FALSE otherwise
**
** Outputs:
**	none, just return
** Returns:
**      if desired element was found, offset to desired duvedpnds.duve_indep 
**	entry; DU_INVALID otherwise
**
** Exceptions:
**	none
**
** Side Effects:
**	none
**
** History:
**	08-dec-93 (andre)
**	    written
**	21-dec-93 (andre)
**	    added first to the interface; added code to scan the list backwards
**	    if the caller requested that we return offset to the first element 
**	    corresponding to the given object
*/
DU_STATUS
duve_i_dpndfind(inid1, inid2, itype, iqid, duve_cb, first)
i4		inid1, inid2;
i4		itype;
i4		iqid;
DUVE_CB         *duve_cb;
bool		first;
{
    i4		hi, lo, check;
    DU_I_DPNDS		*indep;
    
    /* data is ordered, so use binary search */
    for (hi = duve_cb->duvedpnds.duve_indep_cnt - 1, lo = 0; hi >= lo; )
    {
	indep = duve_cb->duvedpnds.duve_indep + (check = (hi + lo) / 2);
	if (inid1 > indep->du_inid.db_tab_base)
	    lo = check + 1;
	else if (inid1 < indep->du_inid.db_tab_base)
	    hi = check - 1;
	/* inid1 == indep->du_inid.db_tab_base */
	else if (inid2 > indep->du_inid.db_tab_index)
	    lo = check + 1;
	else if (inid2 < indep->du_inid.db_tab_index)
	    hi = check - 1;
	/* inid2 == indep->du_inid.db_tab_index */
	else if (itype > indep->du_itype)
	    lo = check + 1;
	else if (itype < indep->du_itype)
	    hi = check - 1;
	/* itype == indep->du_itype */
	else if (iqid > indep->du_iqid)
	    lo = check + 1;
	else if (iqid < indep->du_iqid)
	    hi = check - 1;
	/* iqid == indep->du_iqid */
	else
	{
	    /* 
	    ** found matching element - if the caller requested that we return
	    ** an offset of the first element corresponding to the specified 
	    ** object, scan the list backwards looking for that element; 
	    ** otherwise return the offset of the current element
	    */
	    if (first)
	    {
		/* 
		** before we enter the loop, "indep" points at an element 
		** corresponding to the specified object; by the time we leave 
		** the loop, it will point at the element immediately preceeding
		** the first element corresponding to the specified object (IF 
		** THE FIRST ELEMENT CORRESPONDING TO THE SPECIFIED OBJECT IS 
		** AT OFFSET 0, BY THE TIME WE LEAVE THE LOOP, "INDEP" WILL NOT
		** POINT AT ANY ELEMENT OF THE INDEPENDENT OBJECT INFO LIST)
		*/
		for (indep--; check > 0; indep--, check--)
		{
		    if (   inid1 != indep->du_inid.db_tab_base
			|| inid2 != indep->du_inid.db_tab_index
			|| itype != indep->du_itype
			|| iqid  != indep->du_iqid)
		    {
			/* 
			** next element is the first one to correspond to the
			** specified object; "check" contains its offset
			*/
			break;
		    }
		}
	    }

	    return ((DU_STATUS) check);
	}
    }

    /* matching element was not found */
    return((DU_STATUS) DU_INVALID);
}

/*{
** Name: duve_findreltid- find the entry in the duverel structure for
**			  the iirelation tuple specified by this relid
**
** Description:
**	Verifydb's sys catalog check function gathers information on each
**	iirelation tuple as it examines them.  It keeps a subset of the
**	iirelation tuple in memory in a structure called the duverel structure.
**	duverel is an array of rel structs, each contains the table id, the
**	table's name, owner and status.  It also contains a flag (du_tbldrop)
**	which indicates if this table is to be dropped from the database.
**	Then entries in duverel are ordered by table id.
**
**	This routine searches duverel for an entry with a matching table
**	identifier.  If the match is not found in the table, then 
**	duve_findreltid returns NOT FOUND.  If the match is found, but the
**	tuple is marked for drop (du_droptbl = DUVE_DROP), it returns DROPED. 
**	Else it returns the offset into duverel where the match record 
**	resides.
**
** Inputs:
**      matchid				table id (base and index identifiers)
**					 to try and match (or find)
**	duve_cb				DUVERIFY control block, contains:
**	    duve_cnt		        number of entries in duverel with
**					 valid data.
**	    duverel			array of info about iirelation tuples
**	       rel			info about 1 iirelation tuple, contains:
**					    table id
**					    table name
**					    table owner
**					    table status
**					    number attributes (columns) in table
**					    drop_table flag
**
** Outputs:
**	none, just return
** Returns:
**      pointer to desired duverel entry -- possible values:  
**		zero, positive : match found and is pointer to duverel entry
**		negative : no match found
**			    DU_INVALID=> table id was never in iirelation
**			    DUVE_DROP => table has been marked for drop from db
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      8-8-88	(teg -- hey, this is a real date, no joke)
**          initial creation
*/
DU_STATUS
duve_findreltid(matchid, duve_cb)
DB_TAB_ID          *matchid;
DUVE_CB            *duve_cb;
{
    i4 high, low, middle;

    /* a matchid with a base value of zero is nonsense, we dont even bother
    ** searching for it cuz its not a legal base table id value.
    */
    if (matchid->db_tab_base == 0)
        return ((DU_STATUS) DU_INVALID);

    /* initialize binary search loop */
    high = duve_cb->duve_cnt - 1;
    if (high<0) /* empty cache */
	return ( (DU_STATUS) DU_INVALID);
    low  = 0;

    /* 
    ** binary search loop -- if we drop out, then the search value was 
    ** not found 
    */
    while ( low <= high)
    {
	middle = (low + high) / 2;
	if (duve_cb->duverel->rel[middle].du_id.db_tab_base > matchid->db_tab_base)
	{   /* search point was too high, adjust downwards */
	    high = middle - 1;
	    continue;
	}
	else if ( duve_cb->duverel->rel[middle].du_id.db_tab_base 
		  < matchid->db_tab_base)
	{   /* search point was too low, adjust upwards */
	    low = middle + 1;
	    continue;
	}

	/* if we get here, then base id matches, so check for
	** match on index id (which is the 2nd component of a table id) */

	else if (duve_cb->duverel->rel[middle].du_id.db_tab_index 
		 == matchid->db_tab_index)
	{   /* match found -- make sure the record is valid */
	    middle = (duve_cb->duverel->rel[middle].du_tbldrop == DUVE_DROP ) ?
			DUVE_DROP : middle;
	    return ( (DU_STATUS) middle);
	}
	else if (duve_cb->duverel->rel[middle].du_id.db_tab_index 
		 < matchid->db_tab_index)
	{   /* search point was too low, adjust upwards */
	    low = middle + 1;
	    continue;
	}
	else
	{   /* search point was too high, adjust downwards */
	    high = middle - 1;
	    continue;
	}
    } /* end binary search loop */

    /* 
    ** we only fall out of loop if search value not found, so return not found
    */
    return ( (DU_STATUS) DU_INVALID);
}

/*{
** Name: duve_get_args  - get arguments from command input line
**
** Description:
**      The command line is expected to have the following parameters: 
**	    -m(ode)<value>	    -- not optional
**	    -s(cope)<value>	    -- not optional - may have optional list
**						      of database names
**	    -o(perations)<value>    -- not optional - may have optional list
**						      of names
**	    -u(ser)<value>	    -- optional	    - if specified, will have
**						      single user name
**	    -<flag><value>	    -- purely optional
**	     DOCUMENTED FLAGS currently include:
**	        -verbose (used only with table operation)
**	     UNDOCUMENTED flags currently include:
**		-au -> connect as specified user for accesscheck operation.
**		-d -> DEBUG INFO PRINTED
**		-n -> NO LOGGING TO LOG FILE.
**		-lf -> go to specified output log file
**		-r -> CHECK FOR STATISTICS AND REMOVE TCB_ZOPTSTAT FROM
**		      RELSTAT IF NO CURRENT STATISTICS
**		-t -> if selected does not strip date/time stamp from DMF 
**		      check/patch table messages.  Is meaningless if selected 
**		      with any operation other than TABLE or XTABLE.
**		-test->suppress messages to stdout but log to log file.  Also,
**		      assure that all timestamps are identical: TESTDAY 00:00:00
**		-sep->suppresses messages to stdout and redirects log file
**		      messages (which have a different format) to stdout so that
**		      sep tests can trap them. Also, assure that all timestamps 
**		      are identical: TESTDAY 00:00:00
**		-dbms_test -> suppress messages that cause spurious diffs, except
**		      for messages with whatever value is specified with this 
**		      flag.  I.e., any valid warning message id will suppress
**		      all messages except that one.
**
**	get args will find the start of the value for the specified flag
**	(except for purely optional flags) and call the appropriate routine
**	to validate the arguement.  It is structured so that new utility
**	routines can use this function with minimal changes.
**
** Inputs:
**      argc                            number of arguments in command line
**      argv                            array of pointers to character strings
**					that contain the arguments
**	duve_cb				verifydb control block.  Contains:
**	    du_utilid			  ascii identifier of routine that is
**					  using this generic argument getter --
**					  this is used to determine which
**					  argument checking routine to call
**
** Outputs:
**	duve_cb				verifydb control block.  Contains:
**	    du_mode                         code indicating specified mode
**	    du_scope			    code indicating specified scope
**	    du_scopelst			    array of names specified on input 
**					     line in conjunction with -s flag
**	    duve_nscopes		    number of names in du_scopelst
**	    duve_operation		    code indicating specified operation
**	    duve_ops			    array of names specified on input 
**					     line in conjunction with -o flag
**	    duve_numops			    number of names in duve_ops
**	    duve_user			    name specified with -u flag
**	err				error code:
**					    E_DU0006_UNKNOWN_PARAMETER
**					    E_DU0004_TOO_MANY_NAMES_IN_LIST
**					    E_DU0005_SPECIFY_ONLY_1_TABLE
**					    E_DU0007_SPECIFY_ALL_FLAGS
**					    
**
**	Returns:
**	    DUVE_YES
**	    DUVE_BAD
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      24-jun-1988 (teg)
**          initial creation (for verifydb, but may be used by other utilities)
**	14-Sep-1989 (teg)
**	    added support for XTABLE operation
**	09-Dec-1989 (teg)
**	    added -t flag.
**	5-mar-1990  (teg)
**	    disallow -mreport -oforce_consistent combination.
**	28-mar-90 (teg)
**	    add support for access operation.
**	08-aug-90 (teresa)
**	    stop using argv[parmno]==0 as check for end of input arguments.
**	    It used to be true that the input list was null terminated, but this
**	    appears to no longer be the case.
**	23-oct-92 (teresa)
**	    Implement sir 42498.
**	08-jun-93 (teresa)
**	    implemented -lf and -test flags
**	14-jul-93 (teresa)
**	    Added -sep flag for SEP testing.  Also added the following semantic
**	    checks for illegal flag combinations:
**		-n and -lf is illegal
**		-n and -test is illegal
**		-set and -lf is illegal
**		-sep and -test is illegal
**	10-dec-93 (teresa)
**	    added -dbms_test flag support.
**	08-feb-94 (anitap)
**	    added delimited identifier support for "-u" and "-o" flags.
**	19-feb-94 (anitap)
**	    Do not translate -u delimited user names to lower case. But only
**	    normalize and leave the case ASIS.
**	10-jul-97 (nanpr01)
**	    dbms_catalogs operation is not supported with run/run_silent
**	    mode.
**	30-mar-06 (toumi01) BUG 115911
**	    Protect stack against long input string corruption.
**	14-May-2009 (kschendel) b122041
**	    Minor comment fixes.
*/

DU_STATUS
duve_get_args (argc, argv, duve_cb, err)
i4                  argc;               /* number of input arguments */ 
char                **argv;		/* array of strings containing input
					** line arguments */
DUVE_CB		    *duve_cb;		/* verifydb control block: contains*/
i4		    *err;		/* error value returned */
{
    bool		verifydb;	/* flag indicating du_utilid="verifydb"*/
    i4                  parmno;		/* identifies parameter from input str*/
    i4			i;		/* ye 'ole faithful counter */
    u_i4           idmode = 0;
    u_i4               normal_len;
    DB_STATUS       	status;
    DB_ERROR            error;

    /* 
    ** initialize return args to not specified -- which usually generates an
    ** error condition.  Also, identify which utility we are servicing.
    */

    duve_cb->duve_mode = DU_NOSPECIFY;
    duve_cb->duve_scope = DU_NOSPECIFY;
    duve_cb->duve_operation = DU_NOSPECIFY;
    duve_cb->duve_user[0] = '\0';
    duve_cb->duve_debug = FALSE;
    duve_cb->duve_relstat = FALSE;
    duve_cb->duve_skiplog = FALSE;
    duve_cb->duve_timestamp = TRUE;
    duve_cb->duve_verbose = FALSE;
    duve_cb->duve_altlog = FALSE;
    duve_cb->duve_test = FALSE;
    duve_cb->duve_sep = FALSE;
    duve_cb->duve_dbms_test = FALSE;
    *err = OK;

    verifydb = !(STcompare(duve_cb->du_utilid,"verifydb"));

    /*
    ** Loop for each of the arguments 
    */
    for (parmno = 1; (argc - parmno); parmno++)
    {
	if (argv[parmno][0] == '-')	/* see if its a flag --if so, which? */
	{   
	   /* Do not force to lower case if user name is delimited.
	   ** We will normalise the user name if delimited, but 
	   ** treat the case ASIS.
	   */
	   if (argv[parmno][1] != 'u' &&
	      argv[parmno][2] != '\"')
              CVlower(argv[parmno]);	/* force to lower case */

	    switch (argv[parmno][1])
	    {
	    case 'm':	    /* mode flag */
		i=(MEcmp((PTR)argv[parmno],(PTR)"-mode",(u_i2)5)==DU_IDENTICAL) 
		    ?  5 : 2;
		if (verifydb)
		{
		    duve_cb->duve_mode = duve_ckargs(DU_MODE, &argv[parmno][i],
						     duve_cb);
		    if (duve_cb->duve_debug)
		    {
			duve_talk(DUVE_DEBUG, duve_cb, S_DU04A0_SHOW_MODE, 2, 
				  sizeof(duve_cb->duve_mode), 
				  &duve_cb->duve_mode);
		    }
		} /* _if verifydb */
		break;
	    case 's':	    /* scope flag */
		if (MEcmp((PTR)argv[parmno], (PTR)"-sep", 4)==DU_IDENTICAL)
		{
		    /* this is the SEP flag */
		    duve_cb->duve_sep = TRUE;
		    continue;
		}
		i = (MEcmp((PTR)argv[parmno], (PTR)"-scope", 6)==DU_IDENTICAL) ?
		     6 : 2;
		if (verifydb)
		{   duve_cb->duve_scope = duve_ckargs(DU_SCOPE, 
					             &argv[parmno][i], duve_cb);
		    if (duve_cb->duve_debug)
		    {
			duve_talk(DUVE_DEBUG, duve_cb, S_DU04A1_SHOW_SCOPE, 2, 
				  sizeof(duve_cb->duve_scope), 
				  &duve_cb->duve_scope);
		    }
		    if (duve_cb->duve_scope == DUVE_SPECIFY)
		    {
			if ( parmno+1 < argc) 
			{
			  if ((argv[parmno+1] != 0)&&(argv[parmno+1][0] != '-'))
			  {
			    CVlower(argv[++parmno]);/* force to lower case */
			    duve_cb->duve_nscopes=duve_name(argv[parmno], 
						&duve_cb->du_scopelst, duve_cb);
			    if (duve_cb->duve_nscopes == DU_INVALID)
				duve_cb->duve_scope = DU_INVALID;
			    else if (duve_cb->duve_nscopes > DU_NAMEMAX)
			    {   *err = E_DU5004_TOO_MANY_NAMES_IN_LIST;
				duve_talk(DUVE_ALWAYS, duve_cb,
					  E_DU5004_TOO_MANY_NAMES_IN_LIST, 0);
			    }
			    else if (duve_cb->duve_nscopes == DU_INTRNL_ER)
			    {
				*err = E_DU3400_BAD_MEM_ALLOC;
				duve_talk(DUVE_ALWAYS, duve_cb, 
					  E_DU3400_BAD_MEM_ALLOC, 0);
			    }
			    else if (same_installation(duve_cb->du_scopelst,
				     duve_cb->duve_nscopes)== FALSE)
			    {
			      *err = E_DU503C_TOO_MANY_VNODES;
			      duve_talk(DUVE_ALWAYS, duve_cb,
					E_DU503C_TOO_MANY_VNODES, 0);
                            }
			    else if (duve_cb->duve_debug)
			    {   i4  ctr;
				for (ctr=0; ctr<duve_cb->duve_nscopes; ctr++)
				{
				    duve_talk(DUVE_DEBUG, duve_cb,
					      S_DU04A2_SHOW_SCOPE_NAME,
					      2, 0, duve_cb->du_scopelst[ctr]);
				}
			    } /*_if duve_debug */
			  } /* _if the next parameter is not another flag */
			  else
			  {
			    /* the next arg is not a name, so error */
				duve_talk(DUVE_ALWAYS, duve_cb,
					  E_DU5009_SPECIFY_DB_NAME,0);
				duve_cb->duve_scope = DU_INVALID;
			  }
			}/* _if there is another parameter */
			else
			{   
			    /* there isn't a next parameter and we expect one*/
			    *err = E_DU5009_SPECIFY_DB_NAME;
			    duve_talk(DUVE_ALWAYS, duve_cb,
				      E_DU5009_SPECIFY_DB_NAME, 0);
			    duve_cb->duve_scope = DU_INVALID;
			}
		    } /* _if scope is DUVE_SPECIFY */
		}/* _if verifydb */
		break;
	    case 'o':	    /* operation flag */
		i = (MEcmp((PTR)argv[parmno], (PTR)"-operation", (u_i2)10)
		    ==DU_IDENTICAL) ? 10 : 2;
		if (verifydb)
		{   duve_cb->duve_operation = duve_ckargs(DU_OPER, 
						  &argv[parmno][i],duve_cb);
		    if (duve_cb->duve_debug)
		    {
			duve_talk(DUVE_DEBUG, duve_cb, S_DU04A3_SHOW_OPERATION, 
				  2, sizeof(duve_cb->duve_operation), 
				  &duve_cb->duve_operation);
		    } /*_if duve_debug*/
		    if ( (duve_cb->duve_operation == DUVE_TABLE) ||
			 (duve_cb->duve_operation == DUVE_XTABLE) ||
			 (duve_cb->duve_operation == DUVE_DROPTBL) )
		    {	
			if ( parmno+1 < argc) 
			{
			  if ((argv[parmno+1][0] != '-') && (argv[parmno+1]!=0))
			  {
			     /* We do not want to translate the 
			     ** delimited table name to lower case. 
			     ** We will normalise the delimited table
			     ** name and leave the case ASIS.
			     */
			     if (argv[parmno+1][0] != '\"') 
				/* force to lower case */
			        CVlower(argv[++parmno]); 
			     else
		      	        STcopy(&argv[++parmno][0], 
					duve_cb->duve_unorm_tbl);
			     duve_cb->duve_numops = duve_name(argv[parmno],
						    &duve_cb->duve_ops,duve_cb);
		             
			     if (argv[parmno][0] == '\"')
	           	     {
		                /* We need to normalize the table name */
		                /* i.e., remove the double-quotes 
		      		** around the identifier. 
		      		*/
			        status = cui_idlen(
					(u_char *)duve_cb->duve_unorm_tbl, ',',
					&normal_len, &error); 
					
				if (status != E_DB_OK)
			        {
			  	   return ( (DU_STATUS) DUVE_BAD);
				}
				   
		                status = cui_idxlate(
					(u_char *) duve_cb->duve_unorm_tbl, 
					(u_i4 *) 0, 
					(u_char *) duve_cb->duve_norm_tbl, 
					&normal_len, (u_i4) CUI_ID_DLM_M,
					&idmode, &error);
		      		if (DB_FAILURE_MACRO(status))
		      		{
			  	   return ( (DU_STATUS) DUVE_BAD);
  		            	}

		      		/* null-terminate the string
		      		*/
		      		duve_cb->duve_norm_tbl[normal_len] = EOS;

				STcopy(duve_cb->duve_norm_tbl,
					&duve_cb->duve_ops[0][0]);
		            }
			    else
			    {
			     if (duve_cb->duve_numops == DU_INVALID ) 
			        duve_cb->duve_operation = DU_INVALID;
			     else if (duve_cb->duve_numops != 1 )
			     {
			          *err = E_DU5005_SPECIFY_ONLY_1_TABLE;
				  duve_talk(DUVE_ALWAYS, duve_cb,
				          E_DU5005_SPECIFY_ONLY_1_TABLE, 0);
			     }
			     else if (duve_cb->duve_debug)
			     {
				  duve_talk(DUVE_DEBUG, duve_cb,
					  S_DU04A4_SHOW_TABLE_NAME,
					  2, 0, duve_cb->duve_ops[0]);
			     }
 			   }
			  } /* _if the next parameter is not another flag */
			  else
			  {
			    /* the next arg is not a name, so error */
				duve_talk(DUVE_ALWAYS, duve_cb,
					  E_DU5009_SPECIFY_DB_NAME,0);
				duve_cb->duve_scope = DU_INVALID;
			  }
			}/* _if there is another parameter */
			else
			{
			    /* there isn't a next parameter and we expect one*/
			    *err = E_DU5008_SPECIFY_TABLE_NAME;
			    duve_talk(DUVE_ALWAYS, duve_cb,
				      E_DU5008_SPECIFY_TABLE_NAME,0);
			    duve_cb->duve_operation = DU_INVALID;
			}
		    } /* _if operation is table */
		} /* _if verifydb */
		break;
	    case 'u':	    /* optional user specification */
		i = (MEcmp((PTR)argv[parmno], (PTR)"-user", (u_i2)5)
		    ==DU_IDENTICAL ) ? 5 : 2;
		if (&argv[parmno][i])
		{
		   if (argv[parmno][i] == '\"')
	           {
		      /* We need to normalize the user name */
		      /* i.e., remove the double-quotes 
		      ** around the identifier. 
		      */
		      STcopy(&argv[parmno][i], duve_cb->duve_unorm_usr);
		      normal_len = sizeof(duve_cb->duve_user);
		      status = cui_idxlate(
				(u_char *) duve_cb->duve_unorm_usr, (u_i4 *) 0,
				(u_char *) duve_cb->duve_user, &normal_len,
				(u_i4) CUI_ID_DLM_M,
				&idmode, &error);
		      if (DB_FAILURE_MACRO(status))
		      {
								  
			  /* since can't look up DBMS error returned in
			  ** error.err_code, use default 'bad username' error.
			  */
			  duve_talk(DUVE_ALWAYS, duve_cb->duve_msg, 
				E_DU3000_BAD_USRNAME, 2, 0, duve_cb->duve_user);
			  return ( (DU_STATUS) DUVE_BAD);
  		      }

		      /* null-terminate the string
		      */
		      duve_cb->duve_user[normal_len] = EOS;

    		      if (du_chk2_usrname(duve_cb->duve_user) != OK)
    		      {
			 duve_talk(DUVE_ALWAYS, duve_cb->duve_msg, 
				E_DU3000_BAD_USRNAME, 2, 0, duve_cb->duve_user);
			 return ( (DU_STATUS) DUVE_BAD);
    		      }

		   }
		   else
	           {  
		      CVlower(&argv[parmno][i]);    /* force to lower case */
		      STcopy (&argv[parmno][i], duve_cb->duve_user);
		   } 
		}
		break;
	    case 'a':	    /* optional accesscheck user specification */
		if (MEcmp((PTR)argv[parmno], (PTR)"-au", (u_i2)3)
		    ==DU_IDENTICAL )
		{
		    if (&argv[parmno][3])
			STcopy (&argv[parmno][3], duve_cb->duve_auser);
		}
		else
		if (verifydb)
		{
		    *err = E_DU5006_UNKNOWN_PARAMETER;
		    duve_talk(DUVE_ALWAYS, duve_cb,
			      E_DU5006_UNKNOWN_PARAMETER, 0);
		}
		break;
	    case 'l':	    /* log file name specification */
		if (MEcmp((PTR)argv[parmno], (PTR)"-lf", (u_i2)3)
		    ==DU_IDENTICAL )
		{
		    if (&argv[parmno][3])
		    {
			STcopy (&argv[parmno][3], duve_cb->duve_lgfile);
			duve_cb->duve_altlog = TRUE;
		    }
		}
		else
		if (verifydb)
		{
		    *err = E_DU5006_UNKNOWN_PARAMETER;
		    duve_talk(DUVE_ALWAYS, duve_cb,
			      E_DU5006_UNKNOWN_PARAMETER, 0);
		}
		break;
	    case 'd':	    /* debug flag - should be 1st parameter */
		if (MEcmp((PTR)argv[parmno], (PTR)"-dbms_test", (u_i2)10)
                    ==DU_IDENTICAL )
		{
		    char    numbuf[100];    /* scratch pad to read in number*/
		    /* the DBMS_TEST flag was specified.  See if a numeric
		    ** value was attached to it.  If so, convert to decimal.
		    */
		    if (argv[parmno][10])
		    {
			STlcopy (&argv[parmno][10], numbuf, sizeof(numbuf));
			cv_numbuf(numbuf, &duve_cb->duve_dbms_test);
		    }
		    else
			duve_cb->duve_dbms_test = -1;
		}
		else
		    duve_cb->duve_debug = TRUE;
		break;
	    case 'r':	    /* undocumented DUVE_RELSTAT flag, used in
			    ** conjunction with -odbms_catalogs -- this
			    ** will cause OPTIMIZER STATISTICS bit to
			    ** be cleared from relstat if statisitcs do
			    ** NOT exist.  This flag is not interactive
			    ** and is not mode sensitive. Futhermore,
			    ** it runs silently.  But it has the net 
			    ** effect of speeding up execution time for
			    ** the front-ends */

		if (MEcmp((PTR)argv[parmno], (PTR)"-rel", (u_i2)4)==DU_IDENTICAL)
		    duve_cb->duve_relstat = TRUE;
		break;
	    case 'n':	    /* -n suppresses log file output */
		duve_cb->duve_skiplog = TRUE;
		break;
	    case 't':	    /* suppress stripping of timestamp from dmf msgs*/
		if (MEcmp((PTR)argv[parmno], (PTR)"-test", (u_i2)5)
		    ==DU_IDENTICAL )
		    /* its the -test flag */
		    duve_cb->duve_test = TRUE;
		else
		    /* must be vanella -t flag, so */
		    duve_cb->duve_timestamp = FALSE;
		break;
	    case 'v':	    /* currently the only flag starting with v is
			    ** verbose, so if the V is specified, assume it is
			    ** verbose
			    */
		duve_cb->duve_verbose = TRUE;
		break;
	    default:	    /* nothing else is currently legal */
		if (verifydb)
		{
		    *err = E_DU5006_UNKNOWN_PARAMETER;
		    duve_talk(DUVE_ALWAYS, duve_cb,
			      E_DU5006_UNKNOWN_PARAMETER, 0);
		}
		break;
	    } /*endcase*/

	}/* endif this is a flag */

    } /* end loop for each parameter */


    if (*err == OK)
    {
	/* check for illegal combinations:
	** 1) the force consistent operation has only run modes, 
	**    and is not valid in report mode
	** 2) only report mode may be used with ACCESSCHECK operation
	** 3) -a flag is only meaningful if ACCESSCHECK operation is specifed
	** 4) illegal to have -lf (redirect log file) flag with either
	**    -n (suppress output to log file) or -sep (redirect log file to
	**    STDOUT).
	** 5) illegal to have -test flag (suppress STDOUT) with either -n
	**    (suppress logging) or -sep (redirect log file to STDOUT).
	*/
	if ((duve_cb->duve_operation == DUVE_MKCONSIST) &&
	    (duve_cb->duve_mode == DUVE_REPORT) )
	{
	    * err = E_DU5033_REPORTMODE_INVALID;
	    duve_talk(DUVE_ALWAYS, duve_cb,
		      E_DU5033_REPORTMODE_INVALID, 0);
	}
	else if ((duve_cb->duve_operation == DUVE_ACCESS) &&
	    (duve_cb->duve_mode != DUVE_REPORT) )
	{
	    * err = E_DU5035_REPORTMODE_ONLY;
	    duve_talk(DUVE_ALWAYS, duve_cb,
		      E_DU5035_REPORTMODE_ONLY, 0);
	}
	else if ((duve_cb->duve_mode == DUVE_RUN || 
		  duve_cb->duve_mode == DUVE_SILENT) && 
		 (duve_cb->duve_operation == DUVE_CATALOGS))
	{
	    *err = E_DU503D_RUNMODE_INVALID;
	    duve_talk(DUVE_ALWAYS, duve_cb,
			E_DU503D_RUNMODE_INVALID, 0);
	}
	else if (duve_cb->duve_altlog && duve_cb->duve_skiplog)
	{

	    *err = E_DU503A_ILLEGAL_FLAG_COMBO;
	    duve_talk(DUVE_ALWAYS, duve_cb,
		      E_DU503A_ILLEGAL_FLAG_COMBO, 4,
		      0, ERx("-lf"), 0, ERx("-n"));
	}
	else if (duve_cb->duve_sep && duve_cb->duve_altlog)
	{

	    *err = E_DU503A_ILLEGAL_FLAG_COMBO;
	    duve_talk(DUVE_ALWAYS, duve_cb,
		      E_DU503A_ILLEGAL_FLAG_COMBO, 4,
		      0, ERx("-sep"), 0, ERx("-lf"));
	}
	else if (duve_cb->duve_test && duve_cb->duve_skiplog)
	{

	    *err = E_DU503A_ILLEGAL_FLAG_COMBO;
	    duve_talk(DUVE_ALWAYS, duve_cb,
		      E_DU503A_ILLEGAL_FLAG_COMBO, 4,
		      0, ERx("-test"), 0, ERx("-n"));
	}
	else if (duve_cb->duve_test && duve_cb->duve_sep)
	{

	    *err = E_DU503A_ILLEGAL_FLAG_COMBO;
	    duve_talk(DUVE_ALWAYS, duve_cb,
		      E_DU503A_ILLEGAL_FLAG_COMBO, 4,
		      0, ERx("-test"), 0, ERx("-sep"));
	}
	else if ((duve_cb->duve_mode == DU_INVALID) || 
	    (duve_cb->duve_mode == DU_NOSPECIFY) ||
	    (duve_cb->duve_scope == DU_INVALID) || 
	    (duve_cb->duve_scope == DU_NOSPECIFY) ||
	    (duve_cb->duve_operation == DU_INVALID) || 
	    (duve_cb->duve_operation == DU_NOSPECIFY) )
	    {
		*err = E_DU5007_SPECIFY_ALL_FLAGS;
		duve_talk(DUVE_ALWAYS, duve_cb,
			  E_DU5007_SPECIFY_ALL_FLAGS, 0);
	    }
    }
    if ( (*err == OK) && (duve_cb->duve_operation != DUVE_ACCESS) &&
	 (duve_cb->duve_auser[0] != '\0')  )
	    duve_talk (DUVE_MODESENS, duve_cb, S_DU1705_AFLAG_WARNING, 0);
    
    /* implement verifydb traces */
    {
	i4		    ctr1;
	char                *trace = NULL;

	NMgtAt("II_VERIFYDB_TRACE", &trace);
	if ((trace != NULL) && (*trace != '\0'))
	{
	    char    *comma;
	    char    commavalue = ',';
	    i4 trace_point;

	    /* initialize all trace points to false.  This is redundant since
	    ** the structure is zerofilled, but do it anyhow for clairity */
	    for (ctr1=0; ctr1<MAX_TRACE_PTS; ctr1++)
		duve_cb->duve_trace[ctr1]=FALSE;

	    /* start loop to set trace points */
	    do
	    {
		if (comma = STchr(trace, commavalue))
		    *comma = '\0';
		if (CVal(trace, &trace_point))
		    break;
		trace = &comma[1];
		if (trace_point < 0 ||
		    trace_point >= MAX_TRACE_PTS)
		{
		    continue;
		}
		duve_cb->duve_trace[trace_point]=TRUE;

	    } while (comma);
	}
    }

    /* semantics -- if -sep, then turn on -test to suppress stdout msgs */
    if (duve_cb->duve_sep)
	duve_cb->duve_test = TRUE;

    return  ( (*err == OK) ? (DU_STATUS) DUVE_YES : (DU_STATUS) DUVE_BAD) ;
}

/*{
** Name: duve_init	- verifydb control block initialization
**
** Description:
**      initialize verifydb control block 
**
** Inputs:
**      duve_cb                         verifydb control block to initialize
**      du_err                          address of error control block for
**					    verifydb
**	argc				number of arguments input to verifydb
**	argv				array of verifydb input arguments
**
** Outputs:
**      duve_cb                         initialized verifydb control block
**	    duve_cb->duve_msg               message control block is initialized
**	    duve_cb->duveowner              name of owner of verifydb process
**	    duve_cb->duve_log->du_file	    no open verifydb log file
**	    duve_cb->du_opendb		    no open database
**	Returns:
**	    DUVE_YES
**	    DUVE_BAD
**	    
**	Exceptions:
**	    none
**
** Side Effects:
**	    verifydb log file is created if it doesn't already exist
**
** History:
**      25-Jun-1988 (teg)
**          initial creation
**	25-sep-1990 (teresa)
**	    fixed bug 33370 by prescanning for -n or -nolog before opening and
**	    initializing verifydb log file.
**	19-sep-1991 (teresa)
**	    fix portability bug by changing register i -> i4  register i.
**	08-jun-93 (teresa)
**	    handle -lf and -test flags.
**	14-jul-93 (teresa)
**	    Add support for -sep flag.
**	13-dec-93 (teresa)
**	    Add new parameter to duve_log call.
**	19-feb-94 (anitap)
**	    changes for delimited ids. Do not translate -u parameters to lower
**	    case if they are delimited. 
**	27-dec-1995 (nick)
**	    Don't lowercase or trim white the username of the process owner ;
**	    the appropriate action is now taken in duve_dbdb().
[@history_template@]...
*/
DU_STATUS
duve_init(duve_cb,du_err,argc,argv)
DUVE_CB             *duve_cb;
DU_ERROR	    *du_err;
i4                  argc;               /* number of arguments input */ 
char                **argv;		/* array of strings containing input
					** line arguments */
{
    i4  register i;
    char	*nameptr;
    char datestr[TM_SIZE_STAMP+1];
    i4	length;
    bool	open_log_file = TRUE;
    bool	skip_date = FALSE;
    char	*sep = "*************************************************";

    nameptr = duve_cb->duveowner;
    duve_cb->duve_msg = du_err;
    duve_cb->du_utilid = "verifydb";
    du_reset_err(duve_cb->duve_msg);
    duve_cb->duve_log.du_file = NULL;
    duve_cb->du_opendb[0]='\0';
    
    IDname(&nameptr);
    if (du_chk2_usrname(duve_cb->duveowner) != OK)
    {
	duve_talk(DUVE_ALWAYS, duve_cb->duve_msg, E_DU3000_BAD_USRNAME,
			2, 0, duve_cb->duveowner);
	return ( (DU_STATUS) DUVE_BAD);
    }

    /* scan input parameters to see if any of -nolog (or -n) or -test or -lf 
    ** are specified.
    ** If -n, skip opening verifydb log file  [This flag is incompatable with
    **	    -test and -lf.]
    ** If -lf, use specified name when opening log file.
    ** if -test, use timestamp of "TESTDAY 00:00:00"
    */
    for (i= 1; (argc - i); i++)
    {
	if (argv[i][0] == '-')	/* see if its a flag --if so, which? */
	{   
	   /* We do not want to translate the delimited user
	   ** name to lower case. We will just normalize the
	   ** the delimited user name and leave the case ASIS
	   */
	   if (argv[i][1] != 'u' &&
		argv[i][2] != '\"')
	      CVlower(argv[i]);	/* force to lower case */

	    if (argv[i][1]=='n')	/* check for nolog */
	    {
		open_log_file=FALSE;
		break;
	    }
	    if (argv[i][1]=='t')	/* check for nolog */
	    {
		if (MEcmp((PTR)argv[i], (PTR)"-test", (u_i2)5)
		    ==DU_IDENTICAL )
		{
		    duve_cb->duve_test=TRUE;
		    skip_date=TRUE;
		}
	    }
	    if (argv[i][1]=='s')	/* check for sep */
	    {
		if (MEcmp((PTR)argv[i], (PTR)"-sep", (u_i2)4)
		    ==DU_IDENTICAL )
		{
		    duve_cb->duve_sep=TRUE;
		    open_log_file=FALSE;
		    skip_date=TRUE;
		}
	    }
	    if (argv[i][1]=='l')	/* check for -lfXXXX */
	    {
		if (MEcmp((PTR)argv[i], (PTR)"-lf", (u_i2)3)
		    ==DU_IDENTICAL )
		{
		    if (&argv[i][3])
		    {
			STcopy (&argv[i][3], duve_cb->duve_lgfile);
			duve_cb->duve_altlog = TRUE;
		    }
		}
	    }
	}
    }
    if ( (open_log_file) || (duve_cb->duve_sep) )
    {
	if (open_log_file)
	{
	    /* -n or -nolog were not specified, so open verifydb log file */
	    if ( duve_fopen(duve_cb) != OK)
	    {
		if (duve_nolog(duve_cb,argc,argv) != TRUE)
		{
		    SIprintf ("%s\n",duve_cb->duve_msg->du_errmsg);
		    duve_exit(duve_cb);
		}
	    }
	}

	/* put id and timestamp into log */
	if (skip_date)
	{
	    STcopy ( "TESTDAY 00:00:00", datestr);
	}
	else
	{
	    TMcvtsecs ( TMsecs(), datestr);		/* get datestring */
	    length = STnlength ( sizeof(datestr), datestr);
	    if (length == 0)
		--length;   /* convert length to offset in string */
	    datestr[length] = '\0';	/* null terminate */
	}
	duve_log (duve_cb, 0, sep);
	duve_log (duve_cb, 0, duve_cb->du_utilid);
	duve_log (duve_cb, 0, datestr);
	duve_log (duve_cb, 0, sep);
    }

    return ( (DU_STATUS) DUVE_YES);
}

/*{
**	duve_locfind	- search duve_locs sturcutre for specified location
**			  name.
**
** Description:
**	Verifydb's sys catalog check function gathers all locations known to
**	the installation from the iidbdb before it  connects to the database
**	it will be examining.  These location names are kept on the duve_locs
**	cache.  Each location name is a null terminated character string, and
**	these are not collected in any order.  This routine searches that
**	cache for a particular null terminated location name.  If a match is
**	found, it returns the offset to the desired location in the cache. 
**	Otherwise it returns DU_INVALID.  duve_locfind uses a linear search
**	algorythm.  It is written using CL routines so that it will be 
**	portable to all environments (includine ones that support Konji)
**
**	NOTE:  the search location name must be nullterminated, or
**	       results are not defined.
**
** Inputs:
**      searchloc			name of locaiton to search for 
**					  must be NULL TERMINATED
**	duve_cb				DUVERIFY control block, contains:
**	    duve_cnt		        number of entries in duverel with
**					 valid data.
**	    duve_locs			array of location info structs
**	       du_locname		    Location info struct
**	          duloc				Location name
**
** Outputs:
**	none, just return
** Returns:
**      pointer to desired duve_locs entry -- possible values:  
**		zero, positive : match found and is pointer to duve_locs entry
**		DU_INVALID (negative) : no match found
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-20-88 (teg)
**          initial creation
**	23-Feb-2009 (kschendel) b122041
**	    Fix confusion over null terminator
*/
DU_STATUS
duve_locfind (searchloc, duve_cb)
char		*searchloc;
DUVE_CB            *duve_cb;
{
    DU_STATUS    ret_val;
    i4	 i;
    u_i4	 len;

    len = STtrmwhite( searchloc );
    if (len > sizeof(DB_LOC_NAME) )
        return ( (DU_STATUS) DU_INVALID);

    /* data not ordered, so use linear search */
    for ( ret_val = DU_INVALID, i  = 0; i < duve_cb->duve_lcnt; i++)
    {
	if ( STcompare (searchloc,
	     duve_cb->duve_locs->du_locname[i].duloc)!=DU_IDENTICAL)
	    continue;
	/* if we get here, we have a match on table name and table owner.
	   Now, see if the record is valid or deleted. */
        if (duve_cb->duve_locs->du_locname[i].du_lockill== 0)
	    ret_val = i;
	break;

    } /* end search loop */

    return ( (DU_STATUS) ret_val);
}

/*{
** Name: duve_name	- separate the list of names enclosed in quotes to
**			  an array of names (all null terminated)
**
** Description:
**      Receives a string containing 1 or more names separeated by whitespace.
**      Breaks the string into a array of names.  If there is an error in the
**	input string, it generates and outputs an error message
**
** Inputs:
**      arg_val                         null terminated string containing one
**					  or more names
**      nameptr                         address of variable that will contain
**					  pointer to array of parsed names
**      duve_cb                         verifydb control block
**
** Outputs:
**      nameptr                         contains address of allocated memory.
**					 the allocated memory contains an array
**					 of name strings parsed from arg_val
**	Returns:
**	    count  - number of names parsed from string
**	Exceptions:
**	    none
**
** Side Effects:
**	    allocates memory of a list of names
**
** History:
**      23-Jun-1988 (teg)
**          initial creation
*/

static i4		    /* returns count */
duve_name( arg_val, nameptr, duve_cb)
char	*arg_val;		    /* str of names enclosed in double quotes */
char	***nameptr;		    /* array of names, parsed from arg_val */
DUVE_CB *duve_cb;		    /* verifydb control block */
{

	u_i4	    last;
	i4	    count=DU_NAMEMAX;
	char	    **names;
	u_i4	    size;	/* number of byes of dynamic memory to alloc*/
	STATUS	    recmem_stat;
	PTR	    mem_ptr;

	    /* allocate memory for the list of names */
	    size = (DU_NAMEMAX+1) * (sizeof(DB_OWN_NAME)+1);
	    names = (char **) MEreqmem(0, size, TRUE, &recmem_stat);
	    if ( (recmem_stat != OK) || (names == NULL) )
	    {
		duve_talk(DUVE_ALWAYS, duve_cb,
			  E_DU3400_BAD_MEM_ALLOC, 0);
		return (DU_INTRNL_ER);
	    }
    
	    *nameptr = names;
	    last = STlength (arg_val);
	    if (last != 0)
	    {
		STgetwords (&arg_val[0], &count, names);
		names[count]='\0';
		return (count);
	    }
	    else
	    {
		duve_talk(DUVE_ALWAYS, duve_cb,
			  E_DU5000_INVALID_INPUT_NAME, 2, 0, arg_val);
		return (DU_INVALID);
	    }
}

/*{
** Name: duve_nolog 	- check for nolog input parameter
**
** Description:
**      check for nolog input parameter (-n).  If exists, set nolog flag
**	in verifydb control block.  
**
** Inputs:
**      duve_cb                         verifydb control block to initialize
**	argc				number of arguments input to verifydb
**	argv				array of verifydb input arguments
**
** Outputs:
**	none
**	Returns:
**	    TRUE
**	    FALSE
**	    
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      6-nov-1988 (teg)
**          initial creation
[@history_template@]...
*/
bool
duve_nolog(duve_cb,argc, argv)
DUVE_CB             *duve_cb;
i4                  argc;               /* number of arguements input */ 
char                **argv;		/* array of strings containing input
					** line arguements */
{
    i4                  parmno;		/* identifies parameter from input str*/
    i4			i;		/* ye 'ole faithful counter */

    /*
    ** Loop for each of the arguments 
    */
    for (parmno = 1; argv[parmno]; parmno++)
    {
	if (argv[parmno][0] == '-')
	{   
	    CVlower(argv[parmno]);	/* force to lower case */
	    switch (argv[parmno][1])
	    {
	    case 'n':	    /* nolog flag */
		duve_cb->duve_skiplog = TRUE;
		return (TRUE);
		break;
	    } /*endcase*/

	}/* endif this is a flag */

    } /* end loop for each parameter */

    return  ( FALSE);
}

/*{
** Name: duve_obj_type 	- produce a string describing object type based on 
**			  object type code
**
** Description:
**	Given a code (e.g. DB_VIEW) describing type of object, this function 
**	will provide caller with a string describing a type and the length of 
**	the string.
**
** Inputs:
**      obj_type_const			number assigned to an object type
**
** Outputs:
**	obj_str				string describing object type
**	str_len				length of the above string
**
** Returns:
**	none
**	    
** Exceptions:
**      none
**
** Side Effects:
**	none
**
** History:
**      16-dec-93 (andre)
**          written
**	12-Sep-2004 (schka24)
**	    Add sequences.
[@history_template@]...
*/
void
duve_obj_type(obj_type_const, obj_str, str_len)
i4	obj_type_const;
char	*obj_str;
i4	*str_len;
{
    switch (obj_type_const)
    {
	case DB_TABLE:
	    STcopy("TABLE", obj_str);
	    *str_len = sizeof("TABLE") - 1;
	    break;
	case DB_VIEW:
	    STcopy("VIEW", obj_str);
	    *str_len = sizeof("VIEW") - 1;
	    break;
	case DB_INDEX:
	    STcopy("INDEX", obj_str);
	    *str_len = sizeof("INDEX") - 1;
	    break;
	case DB_DBP:
	    STcopy("PROCEDURE", obj_str);
	    *str_len = sizeof("PROCEDURE") - 1;
	    break;
	case DB_RULE:
	    STcopy("RULE", obj_str);
	    *str_len = sizeof("RULE") - 1;
	    break;
	case DB_PROT:
	    STcopy("PERMIT", obj_str);
	    *str_len = sizeof("PERMIT") - 1;
	    break;
	case DB_CONS:
	    STcopy("CONSTRAINT", obj_str);
	    *str_len = sizeof("CONSTRAINT") - 1;
	    break;
	case DB_EVENT:
	    STcopy("DBEVENT", obj_str);
	    *str_len = sizeof("DBEVENT") - 1;
	    break;
	case DB_SYNONYM:
	    STcopy("SYNONYM", obj_str);
	    *str_len = sizeof("SYNONYM") - 1;
	    break;
	case DB_PRIV_DESCR:
	    STcopy("PRIVILEGE DESCRIPTOR", obj_str);
	    *str_len = sizeof("PRIVILEGE DESCRIPTOR") - 1;
	    break;
	case DB_SEQUENCE:
	    STcopy("SEQUENCE", obj_str);
	    *str_len = sizeof("SEQUENCE") - 1;
	    break;
	default:
	    STcopy("UNKNOWN", obj_str);
	    *str_len = sizeof("UNKNOWN") - 1;
	    break;
    }

    return;
}

/*{
** Name: duve_qidfind  - search duverel structure for specified querry id
**
** Description:
**	Verifydb's sys catalog check function gathers information on each
**	iirelation tuple as it examines them.  It keeps a subset of the
**	iirelation tuple in memory in a structure called the duverel structure.
**	duverel is an array of rel structs.  Among the information cached in
**	duverel is the querry id that identifies querry text for views, 
**	integrities and permits.  It also contains a flag (du_tbldrop)
**	which indicates if this table is to be dropped from the database.
**	Then entries in duverel are not ordered by querry id, so a linear
**	search algorithm is used.
**
**	This routine searches duverel for an entry with a matching querry id.
**	If the match is not found in the cache then duve_relidfind returns NOT 
**      FOUND.  If it is found but the tuple is marked for deletion, it
**	returns DROPPED.  Else it returns the offset into duverel where the 
**	match record resides.
**
**
** Inputs:
**      qid1, qid2			querry id to search for 
**	duve_cb				DUVERIFY control block, contains:
**	    duve_cnt		        number of entries in duverel with
**					 valid data.
**	    duverel			array of info about iirelation tuples
**	       rel			info about 1 iirelation tuple, contains:
**					    table id
**					    table name
**					    table owner
**					    table status
**					    number of columns in table
**					    tbl_drop flag
**					    querry ids (qid1 and qid2)
**
** Outputs:
**	none, just return
** Returns:
**      pointer to desired duverel entry -- possible values:  
**		zero, positive : match found and is pointer to duverel entry
**		negative : no match found (DU_INVALID-> was never in iirelation,
**	        DUVE_DROP->was in iirelation but is marked for drop)
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      6-Sep-88	(teg)
**          initial creation
*/
DU_STATUS
duve_qidfind (qid1, qid2, duve_cb)
i4		qid1, qid2;
DUVE_CB         *duve_cb;
{
    DU_STATUS    ret_val;
    i4	 i;
    

    /* data not ordered, so use linear search */
    for ( ret_val = DU_INVALID, i  = 0; i < duve_cb->duve_cnt; i++)
    {
	if ( qid1 != duve_cb->duverel->rel[i].du_qid1)
	    continue;
	if ( qid2 != duve_cb->duverel->rel[i].du_qid2)
	    continue;

	/* if we get here, we have a match on both halves of querry id.
	** Now, see if the record is valid or deleted. */

        ret_val = (duve_cb->duverel->rel[i].du_tbldrop == DUVE_DROP ) ?
		  DUVE_DROP : i;
	break;

    } /* end search loop */

    return ( (DU_STATUS) ret_val);
}

/*{
** Name: duve_relidfind  - search duverel structure for specified 
**			   relid, relowner
**
** Description:
**	Verifydb's sys catalog check function gathers information on each
**	iirelation tuple as it examines them.  It keeps a subset of the
**	iirelation tuple in memory in a structure called the duverel structure.
**	duverel is an array of rel structs, each contains the table id, the
**	table's name, owner and status.  It also contains a flag (du_tbldrop)
**	which indicates if this table is to be dropped from the database.
**	Then entries in duverel are ordered by table id.
**
**	This routine searches duverel for an entry with a matching table name
**	and ownername.  If the match is not found in the table, then 
**	duve_relidfind returns NOT FOUND.  If the match is found, but the
**	tuple is marked for drop (du_droptbl = DUVE_DROP), it returns DROPED. 
**	Else it returns the offset into duverel where the match record 
**	resides.
**
**	NOTE:  search values (relid, relowner) must be nullterminated, or
**	       results are not defined.
**
** Inputs:
**      relid				table name to search for NULL TERMINATED
**	relowner		        owner name to search for NULL TERMINATED
**	duve_cb				DUVERIFY control block, contains:
**	    duve_cnt		        number of entries in duverel with
**					 valid data.
**	    duverel			array of info about iirelation tuples
**	       rel			info about 1 iirelation tuple, contains:
**					    table id
**					    table name
**					    table owner
**					    table status
**					    number of columns in table
**					    tbl_drop flag
**					    querry id
**
** Outputs:
**	none, just return
** Returns:
**      pointer to desired duverel entry -- possible values:  
**		zero, positive : match found and is pointer to duverel entry
**		negative : no match found (DU_INVALID-> was never in iirelation,
**	        DUVE_DROP->was in iirelation but is marked for drop)
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      8-10-88	(teg)
**          initial creation
*/
DU_STATUS
duve_relidfind (tabid, tabowner, duve_cb)
char		   *tabid, *tabowner;
DUVE_CB            *duve_cb;
{
    DU_STATUS    ret_val;
    i4	 i;
    

    /* data not ordered, so use linear search */
    for ( ret_val = DU_INVALID, i  = 0; i < duve_cb->duve_cnt; i++)
    {
	if ( STcompare (tabid, duve_cb->duverel->rel[i].du_tab) != DU_IDENTICAL)
	    continue;
	if ( STcompare (tabowner, duve_cb->duverel->rel[i].du_own) != DU_IDENTICAL)
	    continue;
	/* if we get here, we have a match on table name and table owner.
	   Now, see if the record is valid or deleted. */

        ret_val = (duve_cb->duverel->rel[i].du_tbldrop == DUVE_DROP ) ?
		  DUVE_DROP : i;
	break;

    } /* end search loop */

    return ( (DU_STATUS) ret_val);
}

/*{
** Name: duve_statfind - search duvestat structure for specified statistic
**
** Description:
**	Verifydb's sys catalog check function gathers information on each
**	iistatistics tuple as it examines them.  It keeps a subset of these
**	tuples in memory in a structure called the duvestat structure.
**	duvestat is an array of stats structs.  The entries in duvestat are 
**	ordered by table id, then by satno.  These tuples will be used in
**	an ordered fashion, so it is reasonable to check the "next" record 
**	for a match.  If that check fails, then a binary search is used.
**
**	This routine searches duvestat for an entry with a matching table and
**	attribute id.  If the match is not found in the cache then it 
**	returns NOT FOUND.  If it is found but the tuple is marked for 
**	deletion, it returns DROPPED.  Else it returns the offset into 
**	duvestat where the match record resides.
**
**
** Inputs:
**      match				table id to search for 
**	att				attribute id to search for
**	last				last element found in ordered search
**	duve_cb				DUVERIFY control block, contains:
**	    duve_cnt		        number of entries in duverel with
**					 valid data.
**	    duvestat			array of info about iistatistics tuples
**	       stats			info about 1 iistatistics tuple
**
** Outputs:
**	last - pointer to desired duvestat entry from previous call
**	       will be zero or a valid element in duvestat (DU_INVALID and
**	       DUVE_DROP are not valid values)
** Returns:
**      pointer to desired duvestat entry -- possible values:  
**		zero, positive : match found and is pointer to duvedstat entry
**		negative : no match found (DU_INVALID-> was never in 
**		iistatistics, DUVE_DROP->was in iistatistics but is marked 
**		for drop)
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      7-Sep-88	(teg)
**          initial creation
*/
DU_STATUS
duve_statfind ( match, att, last, duve_cb)
DB_TAB_ID	*match;
i4		att;
DU_STATUS	*last;
DUVE_CB         *duve_cb;
{
    i4	low, high;    
    
    ++(*last);
    if (*last >= 0)  /* valid last, so try to use it */
    {
	if ((match->db_tab_base == 
			duve_cb->duvestat->stats[*last].du_sid.db_tab_base) &&
	    (match->db_tab_index ==
			duve_cb->duvestat->stats[*last].du_sid.db_tab_index) &&
	    (att == duve_cb->duvestat->stats[*last].du_satno) )
	{
	    /* we have found the tuple we want */
	    if ( (duve_cb->duvestat->stats[*last].du_rptr == DUVE_DROP) ||
		 (duve_cb->duvestat->stats[*last].du_sdrop == DUVE_DROP) )
	    {
		duve_cb->duvestat->stats[*last].du_hdrop = DUVE_DROP;
		return ( (DU_STATUS) DUVE_DROP);
	    }
	    else
		return (*last);

	} /* endif we found a match */
    }/* endif -- there was no previous search value to start with */


    high = duve_cb-> duve_scnt - 1;
    if (high<0) /* empty cache */
	return ( (DU_STATUS) DU_INVALID);
    low  = 0;

    /* 
    ** binary search loop -- if we drop out, then the search value was 
    ** not found 
    */
    while ( low <= high)
    {
	*last = (low + high) / 2;
	if (duve_cb->duvestat->stats[*last].du_sid.db_tab_base>match->db_tab_base)
	{   /* search point was too high, adjust downwards */
	    high = *last - 1;
	    continue;
	}
	else if ( duve_cb->duvestat->stats[*last].du_sid.db_tab_base 
		  < match->db_tab_base)
	{   /* search point was too low, adjust upwards */
	    low = *last + 1;
	    continue;
	}

	/* if we get here, then base id matches, so check for
	** match on index id (which is the 2nd component of a match id */

	else if (duve_cb->duvestat->stats[*last].du_sid.db_tab_index 
		 == match->db_tab_index)
	{   /* index is matches -- check for attribute */
	    if (duve_cb->duvestat->stats[*last].du_satno == att)
	    {
		/*  match found -- make sure the record is valid */
		if ( (duve_cb->duvestat->stats[*last].du_rptr == DUVE_DROP) ||
		     (duve_cb->duvestat->stats[*last].du_sdrop == DUVE_DROP) )
		{
		    duve_cb->duvestat->stats[*last].du_hdrop = DUVE_DROP;
		    return ( (DU_STATUS) DUVE_DROP);
		}
		else
		{
		    if (duve_cb->duvestat->stats[*last].du_rptr == DU_INVALID)
			return ( (DU_STATUS) DU_INVALID);
		    else
			return (*last);
		}
	    }
	    else if (duve_cb->duvestat->stats[*last].du_satno < att)
		/* search point was too low, adjust upwards */
		low = *last + 1;
	    else
		/* search point was too high, adjust downwards */
		high = *last - 1;
	    continue;

	}
	else if (duve_cb->duvestat->stats[*last].du_sid.db_tab_index 
		 < match->db_tab_index)
	{   /* search point was too low, adjust upwards */
	    low = *last + 1;
	    continue;
	}
	else
	{   /* search point was too high, adjust downwards */
	    high = *last - 1;
	    continue;
	}

    } /* end binary search loop */
    return ( (DU_STATUS) DU_INVALID);
}

/*{
**	duve_usrfind	- search du_loclst sturcutre for specified INGRES user
**			  name.
**
** Description:
**	Verifydb caches validated all validated user names in finds when it
**	walks the iiuser table.  Each name is a null terminated character 
**	string, and these are not collected in alpahbetical order.  
**	This routine searches the cache for a particular null terminated user 
**	name.  If a match is found, it returns the offset to the desired 
**	location in the cache.   Otherwise it returns DU_INVALID.  
**	duve_usrfind uses a binary search algorythm.  
**
**	NOTE:  the search user name must be nullterminated, or
**	       results are not defined.
**
** Inputs:
**      searchname			name of user name to search for 
**					  must be NULL TERMINATED or blank
**					  terminated.
**	duve_cb				DUVERIFY control block, contains:
**	    duve_cnt		        number of entries in duverel with
**					 valid data.
**	    du_usrlst			array of user names
**	       du_user			    User name
**
**	searchspace			describes which cache to search.  There
**					are three possible caches to search, and
**					any combination of the three is legal:
**			DUVE_USER_NAME  - search user cache DUVE_CB.duveuser
**			DUVE_GROUP_NAME - serach usergroup cache DUVE_CB.duvegrp
**			DUVE_ROLE_NAME  - search role cache DUVE_CB.duverole
**					EXAMPLE: to search both usergroup and
**						 role caches, specify
**						 DUVE_GROUP_NAME | DUVE_ROLE_NAME
**
** Outputs:
**	none, just return
** Returns:
**      pointer to desired du_usrlst entry -- possible values:  
**		zero, positive : match found and is pointer to du_usrlst entry
**		DU_INVALID (negative) : no match found
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      02-02-1989 (teg)
**          initial creation
**	06-jun-94 (teresa)
**	    add support to search group and role namespace for bug 60761.
*/
DU_STATUS
duve_usrfind (searchname, duve_cb, searchspace)
char		    *searchname;
DUVE_CB		    *duve_cb;
i4		    searchspace;
{
    DU_STATUS	 high, low, middle;
    u_i4	 len;
    i4		 compare_result;

    len = STtrmwhite( searchname);
    if (len > DB_MAXNAME)
        return ( (DU_STATUS) DU_INVALID);

    if (searchspace | DUVE_USER_NAME)
    {
	/* data is ordered, so use binary search
	** initialize binary search loop  -- this cache is built with duve_ucnt
	**					 pointing to the 1st empty cache
	**					 entry, so decrement high before using.
	*/
	high = duve_cb-> duve_ucnt-1;
	if (high<0) /* empty cache */
	    return ( (DU_STATUS) DU_INVALID);
	low  = 0;

	/* 
	** binary search loop -- if we drop out, then the search value was 
	** not found 
	*/
	while ( low <= high)
	{
	    middle = (low + high) / 2;
	    compare_result =STcompare(duve_cb->duveusr->du_usrlist[middle].du_user,
			    searchname);
	    if (compare_result > DU_IDENTICAL)
	    {   /* search point was too high, adjust downwards */
		high = middle - 1;
		continue;
	    }
	    else if ( compare_result < DU_IDENTICAL )
	    {   /* search point was too low, adjust upwards */
		low = middle + 1;
		continue;
	    }

	    /* if we get here, then we have a match. */
	    return ( middle);

	} /* end binary search loop */
    }
    if (searchspace | DUVE_GROUP_NAME)
    {
	/* data is ordered, so use binary search
	** initialize binary search loop  -- this cache is built with duve_ucnt
	**					 pointing to the 1st empty cache
	**					 entry, so decrement high before using.
	*/
	high = duve_cb-> duve_gcnt-1;
	if (high<0) /* empty cache */
	    return ( (DU_STATUS) DU_INVALID);
	low  = 0;

	/* 
	** binary search loop -- if we drop out, then the search value was 
	** not found 
	*/
	while ( low <= high)
	{
	    middle = (low + high) / 2;
	    compare_result =STcompare(
			      duve_cb->duvegrp->du_grplist[middle].du_usergroup,
			      searchname);
	    if (compare_result > DU_IDENTICAL)
	    {   /* search point was too high, adjust downwards */
		high = middle - 1;
		continue;
	    }
	    else if ( compare_result < DU_IDENTICAL )
	    {   /* search point was too low, adjust upwards */
		low = middle + 1;
		continue;
	    }

	    /* if we get here, then we have a match. */
	    return ( middle);

	} /* end binary search loop */
    }
    if (searchspace | DUVE_ROLE_NAME)
    {
	/* data is ordered, so use binary search
	** initialize binary search loop  -- this cache is built with duve_ucnt
	**					 pointing to the 1st empty cache
	**					 entry, so decrement high before using.
	*/
	high = duve_cb-> duve_rcnt-1;
	if (high<0) /* empty cache */
	    return ( (DU_STATUS) DU_INVALID);
	low  = 0;

	/* 
	** binary search loop -- if we drop out, then the search value was 
	** not found 
	*/
	while ( low <= high)
	{
	    middle = (low + high) / 2;
	    compare_result =STcompare(
			    duve_cb->duverole->du_rolelist[middle].du_role,
			    searchname);
	    if (compare_result > DU_IDENTICAL)
	    {   /* search point was too high, adjust downwards */
		high = middle - 1;
		continue;
	    }
	    else if ( compare_result < DU_IDENTICAL )
	    {   /* search point was too low, adjust upwards */
		low = middle + 1;
		continue;
	    }

	    /* if we get here, then we have a match. */
	    return ( middle);

	} /* end binary search loop */
    }

    /* 
    ** we only fall out of loop if search value not found, so return not found
    */
    return ( (DU_STATUS) DU_INVALID);
}

static
get_vnode(dbname, node)
 char   *dbname;
 char   *node;
{
  char  *colon_ptr;
  i4    idx=0;

  if (((colon_ptr = STchr(dbname, ':')) != NULL) &&
      STchr(colon_ptr + 1, ':') == (colon_ptr + 1))
  {
    idx = colon_ptr - dbname;
    STncpy(node, dbname, idx);
  }
  node[idx]= '\0'; 
}

static bool
same_installation(dbnames, total_names)
 char  *dbnames[];
 i4    total_names;
{

    i4   i;
    char node1[DB_MAXNAME+4], node2[DB_MAXNAME+4];

    get_vnode(dbnames[0], node1);
    for (i=1; i< total_names; i++)
    {
      get_vnode(dbnames[i], node2);
      if (STcompare(node1, node2) != 0)
	  return(FALSE);
    }
   
   return(TRUE);
}
