/*
**Copyright (c) 2004 Ingres Corporation
** All Rights Reserved
*/
#include	<compat.h>
#include	<gl.h>
#include	<iicommon.h>
#include	<dbdbms.h>
#include	<adf.h>
#include	<di.h>
#include	<er.h>
#include	<lo.h>
#include	<pc.h>
#include	<si.h>
#include	<me.h>
#include	<st.h>
#include	<duerr.h>
#include	<duvfiles.h>
#include	<duve.h>
#include	<cs.h>
#include	<lg.h>
#include	<lk.h>
#include	<dm.h>
#include	<dmf.h>
#include	<dmp.h>
#include        <dudbms.h>
#include	<cv.h>

    exec sql include SQLCA;	/* Embedded SQL Communications Area */

/*
** constants used for verifydb table operation
*/
#define		VERBOSE		"verbose"
#define		NOVERBOSE	"noverbose"

/**
**
**  Name: DUVEPURG.SC - PURGE DB (A VERIFYDB FUNCTION)
**			ALSO PATCH TABLE and (eventually) CHECK TABLE
**
**  Description:
**      This module contains the higher level functions used to implement
**	the verifydb purge operation.
**
**          duve_purge	      - executive for verifydb PURGE, TEMP_PURGE or 
**			        EXPIRED_PURGE operations
**          duve_0mk_purgelist - Make purgelist for PURGE operations
**	    duve_1purg_setup  -	set up to use iiQEF_list_file procedure
**	    duve_2mk_filelist - obtain a list of files in a specified location
**	    duve_3drop_temp   - destroy a specified file from a specified 
**			        location
**	    duve_4drop_expired- drop expired relations (committing after each
**				one)
**	    duve_4a_drop_expired- drop expired relations, without committing
**				after each one.
**	    duve_patchtable   - patch a table
**	    duve_checktable   - check a table
**	    duve_msg	      - accept msgs from an internal procedure
**
**  History:
**      28-feb-1989  (teg)
**          Initial creation for verifydb phase IV.
**	17-may-1989  (teg)
**	    Complete rewrite to remove DI from verifydb and use Internal 
**	    procedures.
**	14-Sep-1989 (teg)
**	    added routines DUVE_PATCHTABLE and DUVE_MSG for Patch table operation.
**	15-feb-1990 (teg)
**	    added check isam table support.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Include <lg.h> to pick up logging system typedefs
**	26-jul-1993 (bryanp)
**	    Include <lk.h> and <pc.h> to pick up typdefs needed by dmp.h. I
**	    must say that I really don't like the idea that this routine
**	    includes <dmp.h>; it sure would be nice if that got changed.
**	8-aug-93 (ed)
**	    unnest dbms.h
**	31-jan-94 (jrb)
**	    Added support for PURGEDB to clean up work locations as well as
**	    data locations. 
**	11-oct-93 (swm)
**	    Bug #58882
**	    Made cosmetic change to truncating cast in duve_msg() and added
**	    comment to indicate that a lint truncation warning can be ignored
**	    because the code is valid.
**	    Tidied up previous history comment.
**	15-feb-1994 (andyw)
**	    duve_3drop_temp() does not delete from it's internal table 
**	    iiqeflistnnn therefore will try to delete system catalog tables, 
**	    removed vchar() usage. bug 59838
**      19-dec-94 (sarjo01) from 08-nov-94 (nick)
**          Try to find alias locations and ignore these when purging.  Fixes
**          bug 56946.
**      02-feb-1995 (sarjo01)
**          Put back andyw's query from 15-feb-1994. Operation appears to be 
**          correct now re bug 56946.
**	 5-dec-1995 (nick)
**	    Fix query which breaks when II_DECIMAL=','.  #73022, #51936 et al.
**	27-dec-1995 (nick)
**	    Various changes to fix bugs occurring in FIPS installations.
**	26-jun-96 (nick)
**	    Use 'save iiqeflistnnn until ...' rather than update iirelation
**	    directly ; this fixes #73248.
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Include cv.h for function defintions
*/


/*
**  Forward and/or External function references.
*/

FUNC_EXTERN DU_STATUS duve_purge();	    /* purge operations executive */
FUNC_EXTERN DU_STATUS duve_0mk_pathlist();/* Make list of directories for db */
FUNC_EXTERN DU_STATUS duve_1purg_setup();   /* set up to use iiQEF_list_file */
FUNC_EXTERN DU_STATUS duve_2mk_filelist();  /* obtain a list of files in a 
					    ** specified location */
FUNC_EXTERN DU_STATUS duve_3drop_temp();    /* destroy a specified file */
FUNC_EXTERN DU_STATUS duve_4drop_expired(); /* drop expired relations */
FUNC_EXTERN DU_STATUS duve_4a_drop_expired(); /* drop expired relations (no
					    ** commits */
FUNC_EXTERN DU_STATUS duve_patchtable();    /* patch table operation */
FUNC_EXTERN VOID      duve_msg();	    /* handle messages from Internal
					    ** procedures. */
GLOBALREF   DUVE_CB   duvecb;		    /* verifydb control block */

/*
**  LOCAL CONSTANTS
*/
#define		MAXLISTNAME	100
#define		CMDBUFSIZE	512

/*{
** Name: DUVE_PURGE	- executive for verifydb purge operation.
**
** Description:
**      This routine is the high level control for verifydb's purge function.
**	It is used to purge a single Database.  If there are multiple databases
**	to purge, this routine must be called once for each database.
**	The basic logic behind the PURGE opreation is:
**	    1.  get a list of all files used by this directory
**	    2.  trim that list by removing all files that INGRES expects to
**		find in that directory.
**	    3.  walk the trimmed list and destroy any temporary files.  (Be
**		careful NOT to destroy the config file (aaaaaaaa.cnf) or
**		recovery config file (aaaaaaaa.rfc).
**	    4.  now walk iirelation for expired relations, deleting any
**		expired relations we find.  We could not do this until the
**		temp files were handled because deletion of expired relations
**		will create new temp files that INGRES is still using (so
**		we dont want to delete them.
**
** Inputs:
**      duve_cb				verifydb control block
**      dbname                          name of database (null terminated)
**	dba				owner of database
**
** Outputs:
**      duve_cb				verifydb control block
**	Returns:
**	    DUVE_YES			success
**	    DUVE_NO			user may not purge this db
**	    DUVE_BAD			error
**	Exceptions:
**	    none
**
** Side Effects:
**	    system catalogs will be modified to remove entries for expired
**		relations
**	    temporary/expired table files will be deleted from disk
**
** History:
**      28-feb-1989 (teg)
**          initial creation.
**	17-May-1989 (teg)
**	    complete rewrite to remove DI from verifydb and use Internal
**	    procedures instead.
**	29-apr-91 (teresa)
**	    fix bug where purge did not properly check user's authorization to
**	    operate on target db.
**	18-may-1993 (robf)
**	    SUPER -> SECURITY
**      19-dec-94 (sarjo01) from 08-nov-94 (nick)
**          Skip alias locations when purging files - this prevents us from
**          erroring when attempting to delete a file for the second time.
**	27-dec-1995 (nick)
**	    We need to keep track of the owner of the temporary table we
**	    create as well as its name ( as the case may differ ).
[@history_template@]...
*/
DU_STATUS
duve_purge(duve_cb,dbname,dba)
DUVE_CB		*duve_cb;
char		*dbname;
char		*dba;
{
    DU_STATUS	retstat;
    i4	i;
    char	tbl_buffer[DB_MAXNAME+1];
    char	user_buffer[DB_MAXNAME+1];
    char	*list_tbl;
    char	*list_own;

    list_tbl = tbl_buffer;
    list_own = user_buffer;

    /*
    ** verify this user is owns this database, or is an ingres superuser.
    ** if neither of these conditions are met, then issue an error and give up.
    */
    if (STcompare(dba, duve_cb->duveowner) != DU_IDENTICAL)
    {
	if (STcompare(dba, duve_cb->duve_user) != DU_IDENTICAL)
	{
            /* see if task owner is a  security user.  If not, generate error */
	    if (ck_authority(duve_cb, 0)  == DUVE_NO)
	    {
		if ( duve_cb->duve_user[0] == '\0' )
		    duve_talk( DUVE_ALWAYS,duve_cb,S_DU1700_NO_PURGE_AUTHORITY, 6,
			       0, duve_cb->duveowner, 0, dbname, 0, dba);
		else
		    duve_talk( DUVE_ALWAYS,duve_cb,S_DU1700_NO_PURGE_AUTHORITY, 6,
			       0, duve_cb->duve_user, 0, dbname, 0, dba);
		return ( (DU_STATUS) DUVE_NO);
	    }
	}
    }

    /* 
    **	connect to database  (as user $ingres)
    */
    if ( duve_opendb(duve_cb, dbname) == DUVE_BAD)
	return ( (DU_STATUS) DUVE_BAD);


    /* search for temp files, destroy any that we find */
    if ( (duve_cb->duve_operation == DUVE_PURGE) ||
	 (duve_cb->duve_operation == DUVE_TEMP) )
    {
	retstat = duve_talk (DUVE_MODESENS, duve_cb, 
			     S_DU04C8_TEMPPURGE_START, NULL);
	if (retstat == DUVE_BAD)
	    return (retstat);

	/*
	**  now walk directory list cache to see which directories apply to the 
	**  specified db.  This loop finds all filenames in directories used
	**  by this database, and places them into a temp table specified by
	**  list_tbl
	*/
	if (duve_1purg_setup( &list_tbl, &list_own, duve_cb ) == DUVE_BAD)
	{
	    duve_close(duve_cb, TRUE);
	    return ( (DU_STATUS) DUVE_BAD);
	}

	for ( i = 0; i<duve_cb->duve_pcnt; i++ )
	{
            /* skip alias locations */
            if (duve_cb->duve_dirs->dir[i].duvedirflags & DUVE_ALIAS)
                continue;

            if (STcompare(duve_cb->duve_dirs->dir[i].duvedb,dbname)
                == DU_IDENTICAL)
	    {

		/*
		**  this location is associated with target db --
		**  first build list of files for this directory
		*/
		if ( duve_2mk_filelist( &duve_cb->duve_dirs->dir[i],
					list_tbl, list_own, duve_cb) 
		== DUVE_BAD)
		{
		    duve_close(duve_cb, TRUE);
		    return ( (DU_STATUS) DUVE_BAD);
		}	
	    }  /* end if this location is used by this db */
	} /* end loop for each location */

	/* now that we have the list of files known to this db, determine
	** which files are temporary, and delete them. 
	*/
	if (duve_3drop_temp(list_tbl,duve_cb) == DUVE_BAD)
	{
	    duve_close(duve_cb, TRUE);
	    return ( (DU_STATUS) DUVE_BAD);
	}

    }  /* end deletion of temporary files */

    /* search for expired relations, drop any that we find */
    if ( (duve_cb->duve_operation == DUVE_PURGE) ||
	 (duve_cb->duve_operation == DUVE_EXPIRED) )
    {

	retstat = duve_talk (DUVE_MODESENS, duve_cb, 
			     S_DU04C9_EXPPURGE_START, NULL);
	if (retstat == DUVE_BAD)
	    return (retstat);


	if (duve_4drop_expired(duve_cb) == DUVE_BAD)
	{
	    duve_close(duve_cb, TRUE);
	    return ( (DU_STATUS) DUVE_BAD);
	}

    }

    /* 
    **	now close the database without aborting.
    */
    duve_close(duve_cb, FALSE);

    return ( (DU_STATUS) DUVE_YES);
}        

/*{
** Name: duve_0mk_purgelist	- Make purgelist for PURGE operation
**
** Description:
**      The PURGE operations walk all database directories associated with
**	all databases in scopelist, and get a list of all files in each
**	directory.  Then verifydb operates on that list to determine which
**	files should be deleted.  Verifydb gets information about the
**	directories associated with each database from the iidbdb.
**
**	Connecting to a database is time consuming, so this routine has been
**	designed to be called from duve_dbdb (which is already connected to
**	the iidbdb).
**
**	This routine retrieves the cross-reference of which databases
**	use which locations.  That info is placed into the duve_dirs
**	array of the DUVE_CB.  All databases in the verifydb scope are
**	recorded.
**
** Inputs:
**      duve_cb                         verifydb control block
**
** Outputs:
**      duve_cb                         verifydb control block
**	Returns:
**	    DUVE_YES			success
**	    DUVE_BAD			error
**	Exceptions:
**	    ESQL will raise exception and exit via Clean_Up() if an SQL
**	    error is encountered.
**
** Side Effects:
**
** History:
**      28-Feb-1989 (teg)
**          initial creation
**	05-jan-94 (teresa)
**	    Fix A bug 58401 where the join on iilocations was not restricting
**	    the query to just return data locations.
**      31-jan-94 (jrb)
**	    Fill in new field "duve_type" in the DU_DIRLST for MLSort.
**      19-dec-1994 (sarjo01) from 08-Nov-1994 (nick)
**          Altered to check for alias locations and mark these as such - note
**          that alias locations as defined here are those which have the
**          same path name - sym linked alias locations won't be picked up
**          as this requires writing markers in each location we've tested
**          and then noticing that they are there when testing another.
**     29-Mar-2005 (chopr01) (b113078)
**         Added support to handle raw directories. When retrieveing directory
**         information, retrieve "status" value from iiextend table, which 
**         shows whether the directory is in a raw-location or not
**	15-Sep-2010 (thaju02) B124344
**	   Dbms path may contain spaces. Do not remove spaces within the 
**	   path rather only trailing whitespace.
[@history_template@]...
*/
duve_0mk_purgelist(duve_cb)
DUVE_CB		*duve_cb;
{
    u_i4	size;
    STATUS	mem_stat;
    i4	i,j;
    u_i4	len;

    EXEC SQL BEGIN DECLARE SECTION;
	char	dbname[DB_MAXNAME+1];
	char	db[DB_MAXNAME+1];
	char	dba[DB_MAXNAME+1];
	char	area[DB_AREA_MAX+1];
	char	loc[DB_MAXNAME+1];
	char	dbdev[DB_MAXNAME+1];
	i4	loc_type;
	i4      stat; /* To store information of iiextend.status */
	u_i4	cnt;
	i4	l_status;
        char    *parea;
    EXEC SQL END DECLARE SECTION;

    EXEC SQL WHENEVER SQLERROR call Clean_Up;


    /* 
    ** allocate memory to hold array of directory information 
    */
    EXEC SQL select count(lname) into :cnt from iiextend;
    size = sizeof(DU_DIRLST) * cnt;
    duve_cb->duve_dirs = (DUVE_DIRLST *) MEreqmem( 0, size,
						    TRUE, &mem_stat);
    if ( (mem_stat != OK) || ( duve_cb->duve_dirs == NULL) )
    {
	duve_talk (DUVE_ALWAYS, duve_cb, E_DU3400_BAD_MEM_ALLOC, NULL);
	return ( (DU_STATUS) DUVE_BAD);
    }

    /* 
    ** now go get directory information for all databases in VERIFYDB's scope
    */
    l_status = DU_DBS_LOC;
    for(i=0; i<duve_cb->duve_ncnt; i++)
    {
	STcopy(duve_cb->duve_list->du_dblist[i].du_db, dbname);
        parea = NULL;
	EXEC SQL select	dbdev into :dbdev from iidatabase where name = :dbname;
	len = STzapblank(dbdev,dbdev);
	EXEC SQL select d.name, e.lname, l.area, d.own, l.status, e.status
	into :db, :loc, :area, :dba, :loc_type, :stat
	from
	  iidatabase d, iiextend e, iilocations l
	where
	  d.name = e.dname and
	  l.lname = e.lname and
	  e.dname = :dbname
/*	  e.dname = :dbname and
	  l.status = :l_status */
	order by d.name, l.area;
	EXEC SQL BEGIN;
	    j = duve_cb->duve_pcnt;
	    len = STzapblank(dbname, duve_cb->duve_dirs->dir[j].duvedb);
	    len = STzapblank(dba, duve_cb->duve_dirs->dir[j].duvedba);
	    STcopy(area, duve_cb->duve_dirs->dir[j].duve_area);
	    len = STtrmwhite(duve_cb->duve_dirs->dir[j].duve_area);
	    len = STzapblank(loc, duve_cb->duve_dirs->dir[j].duveloc);
	    if (STcompare(dbdev, duve_cb->duve_dirs->dir[j].duveloc)
	    == DU_IDENTICAL)
            {
                duve_cb->duve_dirs->dir[j].duvedirflags |= DUVE_DEFAULT;
            }
            if (parea)
                if (STcompare(parea, duve_cb->duve_dirs->dir[j].duve_area)
                    == DU_IDENTICAL)
                {
                    duve_cb->duve_dirs->dir[j].duvedirflags |= DUVE_ALIAS;
                }

            parea = duve_cb->duve_dirs->dir[j].duve_area;
	    duve_cb->duve_dirs->dir[j].duve_type = loc_type;
            if (stat & DU_EXT_RAW)
	    {
	      duve_cb->duve_dirs->dir[j].duvedirflags |= DU_EXT_RAW;
	    }
	    duve_cb->duve_pcnt++;
	EXEC SQL END;
    } /* end loop for each database in scopelist */
    
    return ( (DU_STATUS) DUVE_YES);

}

/*{
** Name:  DUVE_1PURG_SETUP
**
** Description:
**
**	This routine finds a unique table name and makes a temporary table
**	to contain a list of files in the database directories.  It provides
**	the unique table name back to the calling routine.  It also creates the
**	intenral procedures to list/delete files.  It also creates a cursor
**	statement that must know the name of the temp table.
**
** Inputs:
**      duve_cb				verifydb control block
**	list_tbl			pointer to buffer to put list table name
**					   into
**	list_own			pointer to buffer to put list table
**					owner into
**
** Outputs:
**	list_tbl			name of list table
**	list_own			name of list table owner
**      duve_cb				verifydb control block
**	Returns:
**	    DUVE_YES			success
**	    DUVE_NO			user may not purge this db
**	    DUVE_BAD			error
**	Exceptions:
**	    none
**
** Side Effects:
**	    A temporary table will be created.
**	    Two internal procedures will be created.
**
** History:
**	17-May-1989 (teg)
**	    Initial Creation
**	04-Aug-1989 (teg)
**	    Increased size of path_name parameter to internal procedures for
**	    unix (cuz UNIX has longer db pathnames)
**	04-jan-1993 (teresa)
**	    fix bugs 39397, 57492.
**	27-dec-1995 (nick)
**	    Case of our table to hold the files to delete may be uppercase ;
**	    also return the owner of this table as well as its name.
**	26-jun-96 (nick)
**	    Modify method of setting expiry date on the table we create here.
[@history_template@]...
*/
duve_1purg_setup( list_tbl, list_own, duve_cb )
char		**list_tbl;
char		**list_own;
DUVE_CB		*duve_cb;
{
	int		i;
	char		file_len[DB_MAXNAME],path_len[DB_MAXNAME];
	static char *lst_names[] =
	{	"iiqeflist000",	"iiqeflist001",	"iiqeflist002",	"iiqeflist003",
		"iiqeflist004",	"iiqeflist005",	"iiqeflist006",	"iiqeflist007",
		"iiqeflist008",	"iiqeflist009",	"iiqeflist010",	"iiqeflist011",
		"iiqeflist012", "iiqeflist013",	"iiqeflist014",	"iiqeflist015",
		"iiqeflist016",	"iiqeflist017",	"iiqeflist018",	"iiqeflist019",
		"iiqeflist020",	"iiqeflist021",	"iiqeflist022",	"iiqeflist023",
		"iiqeflist024",	"iiqeflist025",	"iiqeflist026",	"iiqeflist027",
		"iiqeflist028",	"iiqeflist029",	"iiqeflist030",	"iiqeflist031",
		"iiqeflist032",	"iiqeflist033", "iiqeflist034",	"iiqeflist035",
		"iiqeflist036",	"iiqeflist037", "iiqeflist038",	"iiqeflist039",
		"iiqeflist040",	"iiqeflist041",	"iiqeflist042",	"iiqeflist043",
		"iiqeflist044",	"iiqeflist045", "iiqeflist046",	"iiqeflist047",
		"iiqeflist048",	"iiqeflist049", "iiqeflist050",	"iiqeflist051",
		"iiqeflist052",	"iiqeflist053", "iiqeflist054",	"iiqeflist055",
		"iiqeflist056",	"iiqeflist057", "iiqeflist058",	"iiqeflist059",
		"iiqeflist060",	"iiqeflist061",	"iiqeflist062",	"iiqeflist063",
		"iiqeflist064",	"iiqeflist065",	"iiqeflist066",	"iiqeflist067",
		"iiqeflist068",	"iiqeflist069",	"iiqeflist070",	"iiqeflist071",
		"iiqeflist072",	"iiqeflist073",	"iiqeflist074",	"iiqeflist075",
		"iiqeflist076",	"iiqeflist077",	"iiqeflist078",	"iiqeflist079",
		"iiqeflist080",	"iiqeflist081",	"iiqeflist082",	"iiqeflist083",
		"iiqeflist084",	"iiqeflist085",	"iiqeflist086",	"iiqeflist087",
		"iiqeflist088",	"iiqeflist089",	"iiqeflist090",	"iiqeflist091",
		"iiqeflist092",	"iiqeflist093",	"iiqeflist094",	"iiqeflist095",
		"iiqeflist096",	"iiqeflist097",	"iiqeflist098",	"iiqeflist099"
	};
	
	bool	isfipsid   = FALSE;
	bool	isfipsuser = FALSE;

	exec sql begin declare section;
		char		*cmd,buf[CMDBUFSIZE];
		char		tblnam[DB_MAXNAME+1];
		char		fipsbuf[6];
		i4		numcnt;
	exec sql end declare section;

	EXEC SQL WHENEVER SQLERROR call Clean_Up;

	/*
	** decide if this installation is FIPS or not 
	*/
	exec sql select dbmsinfo('db_name_case') into :fipsbuf;
	if (STcompare(fipsbuf, ERx("UPPER")) == DU_IDENTICAL)
	    isfipsid = TRUE;

	exec sql select dbmsinfo('db_real_user_case') into :fipsbuf;
	if (STcompare(fipsbuf, ERx("UPPER")) == DU_IDENTICAL)
	    isfipsuser = TRUE;

	/* loop to search for a list table name.  Usually there will not be
	** any tables by this name in a DB, and the first will be selected.
	** However, we cannot guarantee that the user did not become $ingres
	** and create a table by this name, so we have 100 names to choose
	** from
	*/
	for (i=0; i<	MAXLISTNAME; i++)
	{
	    (VOID) STcopy (lst_names[i], tblnam);

	    if (isfipsid)
		CVupper(tblnam);

	    exec sql select count (relid) into :numcnt from iirelation
                  where relid = :tblnam;
	    if (numcnt == 0)
		break;

	}
	if (i == MAXLISTNAME)
	{
	    /* there were no unique list names in the DB -- this will probably 
	    ** never happen, but if it does, user needs to free up one of
	    ** the names to purge
	    */
	    duve_talk(DUVE_ALWAYS, duve_cb,  E_DU502D_NOFREE_PURGETABLE, 4,
			0, lst_names[0], 0, lst_names[MAXLISTNAME-1] );
	    return ( (DU_STATUS) DUVE_BAD);
	}

	/*
	**  now we know which table name to use, so create the table.
	*/
	*list_tbl = lst_names[i];
	if (isfipsid)
	    CVupper(*list_tbl);
	STcopy(ERx("$ingres"), *list_own);
	if (isfipsuser)
	    CVupper(*list_own);

	(VOID) CVla ( (i4) DI_FILENAME_MAX, file_len);
	(VOID) CVla ( (i4) DI_PATH_MAX, path_len);

	cmd = STpolycat(9,"create table ",*list_tbl," (filnam char(",file_len,
		"), locnam char(",file_len,"), area char(",path_len,") )",
		buf);
	exec sql execute immediate :cmd;

	/* bugs 39397, 57492 require that the table verifydb creates be
	** marked as expired so subsequent verifydb expired_purge operations
	** can get rid of it
	**
	** Use the 'save' command rather than directly update iirelation ;
	** if we don't do this, the purge option can't be run by anyone who
	** doesn't have security user privs which is silly.  The parser has
	** been modified to permit a save on the 'iiqeflistnnn' tables.
	*/
	cmd = STpolycat(3, "save ", *list_tbl, " until July 14 1990", buf);
	exec sql execute immediate :cmd;
	
	/* 
	** create the QEF internal procedure to list files and the one to
	** destroy files
	*/
	EXEC SQL WHENEVER SQLERROR continue;
	    exec sql drop procedure iiQEF_list_file;
	    exec sql drop procedure iiQEF_drop_file;
	EXEC SQL WHENEVER SQLERROR call Clean_Up;

        exec sql create procedure
            iiQEF_list_file(
                        directory char(256) not null not default,
                        tab char(24) not null not default,
                        owner char(24) not null not default,
                        col char(24) not null not default ) as
            BEGIN
                execute internal;
            END;

        exec sql create procedure
            iiQEF_drop_file(
                        directory char(256) not null not default,
                        file char(256) not null not default ) as
            BEGIN
                execute internal;
            END;

	/*
	** create cursor statement-- cursor name will be "temp_cursor"
	*/
	cmd = STpolycat(2, "select filnam, locnam, area from ",*list_tbl, buf);
	EXEC SQL PREPARE s1 FROM :cmd;
	EXEC SQL DECLARE temp_cursor CURSOR FOR s1;

	return ( (DU_STATUS) DUVE_YES);		
}

/*{
** Name:  duve_2mk_filelist - make a list of filenames found in data directory.
**
** Description:
**
**  caller supplies a datadirectory. This routine calls internal procedure to
**  list all files names found in that directory into a temporary table.
**	
**
** Inputs:
**      duve_cb				verifydb control block
**	list_tbl			name of temp table to list files into
**	list_own			owner of temp table to list files into
**	dirlist				struct containing pathname of dir to
**					  search
**
** Outputs:
**      duve_cb				verifydb control block
**	Returns:
**	    DUVE_YES			success
**	    DUVE_BAD			error
**	Exceptions:
**	    none
**
** Side Effects:
**	    A temp table will be created and populated with data.
**
** History:
**	17-May-1989 (teg)
**	    Initial Creation
**      31-jan-94 (jrb)
**	    Add call to du_work_ingpath for work locations to get properly
**          expanded.
**	27-dec-1995 (nick)
**	    We now take the temp table owner as an input and correctly 
**	    establish the case of the attribute names in this relation.
**      29-Mar-2005 (chopr01) (b113078)
**         Added support to handle extended raw location
**      15-Sep-2010 (thaju02) B124344
**         Dbms path may contain spaces. Do not remove spaces within the
**         path rather only trailing whitespace.
[@history_template@]...
*/
duve_2mk_filelist( dirlist, list_tbl, list_own, duve_cb)
DU_DIRLST	    *dirlist;
char		    *list_tbl;
char		    *list_own;
DUVE_CB		    *duve_cb;
{

    EXEC SQL BEGIN DECLARE SECTION;
        char		dbms_path[DI_PATH_MAX + 1];   
	char		locnam[DB_MAXNAME +1];
	char		tabnam[DB_MAXNAME +1];
	char		ownnam[DB_MAXNAME +1];
	char		colnam[DB_MAXNAME +1];
	char		filnam[DI_FILENAME_MAX +1];
	char		*cmd,buf[CMDBUFSIZE];
	char		fipsbuf[6];
	int		retcnt;
    EXEC SQL END DECLARE SECTION;
    u_i4	    len;

    /* 
    **	get full pathname for this location
    */
    if(dirlist->duvedirflags & DUVE_DEFAULT)
    {
	if (du_db_ingpath(dirlist->duve_area,dirlist->duvedb,dbms_path,
			  duve_cb->duve_msg) != E_DU_OK)
	    return ( (DU_STATUS) DUVE_BAD);
    }
    else if (   dirlist->duve_type == DU_WORK_LOC
	     || dirlist->duve_type == DU_AWORK_LOC)
    {
	if (du_work_ingpath(dirlist->duve_area,dirlist->duvedb,dbms_path,
			  duve_cb->duve_msg) != E_DU_OK)
	    return ( (DU_STATUS) DUVE_BAD);
    }
    else if (dirlist->duvedirflags & DU_EXT_RAW)
    {
	if (du_extdb_ingpath(dirlist->duve_area,LO_RAW,dbms_path,
			  duve_cb->duve_msg) != E_DU_OK)
            return ( (DU_STATUS) DUVE_BAD);
    }
    else
    {
	if (du_extdb_ingpath(dirlist->duve_area,dirlist->duvedb,dbms_path,
			  duve_cb->duve_msg) != E_DU_OK)
	    return ( (DU_STATUS) DUVE_BAD);
    }
    len = STtrmwhite(dbms_path);

    (VOID) STcopy (list_tbl, tabnam);
    (VOID) STcopy (list_own, ownnam);

    exec sql select dbmsinfo('db_name_case') into :fipsbuf;
    if (STcompare(ERx("UPPER"), fipsbuf) == DU_IDENTICAL)
	(VOID) STcopy (ERx("FILNAM"), colnam);
    else
    	(VOID) STcopy (ERx("filnam"), colnam);

    exec sql execute procedure iiQEF_list_file(
	    directory	= :dbms_path,
	    tab		= :tabnam,
	    owner	= :ownnam,
	    col		= :colnam)
        into :retcnt;
/**** WORK AROUND FOR QEF BUG WHERE RETCNT ACCIDENTLY NULLED ***************
    if (retcnt)
***************************************************************************/
    {
	cmd = STpolycat( 7, "update ", list_tbl, " set area = '",dbms_path,
			     "', locnam = '",dirlist->duveloc,
			    "' where area is null", buf);
	EXEC SQL EXECUTE IMMEDIATE :cmd;
    }

    return ( (DU_STATUS) DUVE_YES);
}

/*{
** Name:  duve_3drop_temp - drop temporary tables.
**
** Description:
**
**	search the list_tbl (created by an earlier internal procedure call)
**	for any filenames that are NOT found in iifile_info.  If any files
**	meet this condition, then call internal procedure iiQEF_drop_file to
**	remove it from the data directory.
**
** Inputs:
**	list_tbl			temp table containing full pathnames
**					  of files in a database directory
** Outputs:
**      duve_cb				verifydb control block
**	Returns:
**	    DUVE_YES			success
**	    DUVE_BAD			error
**	Exceptions:
**	    none
**
** Side Effects:
**	    A file will be deleted from the database directory.
**
** History:
**	17-May-1989 (teg)
**	    Initial Creation
**	08-aug-90 (teresa)
**	    added logic to set and clear duve_delflg.  This allows the
**	    duve_ingerr routine to know that were were deleting the file when
**	    the ingres error occurred.
**	13-jun-91 (teresa)
**	    sir 37281; check for the existance of the temp table (iirtemp) that
**	    dmf uses when modifying a core catalog.  If this exists, then
**	    destroy it.
**      15-feb-1994 (andyw)
**          duve_3drop_temp() does not delete from it's internal table 
**          iiqeflistnnn therefore will try to delete system catalog tables, 
**          removed vchar() usage. bug 59838
**      02-feb-1995 (sarjo01)
**          Put back andyw's query from 15-feb-1994. Operation appears to be 
**          correct now re bug 56946.
**	 5-dec-1995 (nick)
**	    Add a space following ',' within 'left()' to avoid the 
**	    notorious II_DECIMAL=',' problem.
[@history_template@]...
*/
duve_3drop_temp(list_tbl,duve_cb)
char		    *list_tbl;
DUVE_CB		    *duve_cb;
{

    EXEC SQL BEGIN DECLARE SECTION;
        char		dbms_path[DI_PATH_MAX + 1];   
	char		ownnam[DB_MAXNAME +1];
	char		locnam[DB_MAXNAME +1];
	char		filnam[DI_FILENAME_MAX +1];
	char		*cmd,buf[CMDBUFSIZE];
	u_i4		cnt;
    EXEC SQL END DECLARE SECTION;
    u_i4		len;

    /* first build a command to remove all tuples from the list table that are
    ** in the iifile_info view.  We know that the files are not temp if they
    ** are associated with database tables.
    */
    cmd = STpolycat(6, "delete from ",list_tbl,
      " t where t.filnam in (select t.filnam from ", list_tbl,
      " t,iifile_info f where left(t.filnam, 8) = f.file_name and ",
      "left(right(t.filnam,size(t.filnam)-9), 3) = f.file_ext )",
      buf);

    exec sql execute immediate :cmd;

    /* now remove all tuples from list table that are associated with the
    ** config file (aaaaaaaa.cnf or aaaaaaaa.rfc)
    */
    
    cmd = STpolycat( 3,"delete from ",list_tbl,
	" where filnam='aaaaaaaa.cnf' or filnam='aaaaaaaa.rfc' ", buf);
    exec sql execute immediate :cmd;
    
    /* now what is left is temporary tables, (if anything is left at all).
    ** Open cursor "temp_cursor" that was prepared by duve_1purg_setup()
    ** to walk the temp table and use iiQEF_drop_file to delete the file.
    */
    exec sql open temp_cursor;

    for (;;)	    /* loop to delete temp files */
    {
	exec sql fetch temp_cursor into :filnam, :locnam, :dbms_path;
        if (sqlca.sqlcode == DU_SQL_NONE)
        {       
            EXEC SQL CLOSE temp_cursor;
            break;
        }

	len = STtrmwhite(filnam);
	len = STtrmwhite(locnam);
	duve_talk (DUVE_MODESENS, duve_cb, S_DU1701_TEMP_FILE_FOUND,
		   4, 0, filnam, 0, locnam);
	if (duve_talk (DUVE_ASK, duve_cb, S_DU0400_DEL_TEMP_FILE,
		   4, 0, filnam, 0, locnam) == DUVE_YES)
	{
	    /* set flag so duve_ingerr will know we are doing a drop,
	    ** call internal procedure to drop the file, then clear flag*/
	    duve_cb->duve_delflg = TRUE;
	    exec sql execute procedure iiQEF_drop_file(
		    directory=:dbms_path, file = :filnam) ;
	    duve_cb->duve_delflg = FALSE;

	    duve_talk (DUVE_MODESENS, duve_cb, S_DU0410_DEL_TEMP_FILE,
		       4, 0, filnam, 0, locnam);
	    
	}  /* endif it is ok to delete this temp file. */

    }  /* end loop to delete temporary files */

    /* drop iiQEF_list_file and iiQEF_drop_file procedures.  Also drop the
    ** temp table that was used to list files into.
    */
    
    exec sql drop procedure iiQEF_list_file;
    exec sql drop procedure iiQEF_drop_file;
    cmd = STpolycat (2, "drop table ",list_tbl,buf);
    exec sql execute immediate :cmd;

    /* last but not least, if dmf left a work (ie temp) table named iirtemp
    ** laying around, get rid of it. */
    exec sql select count (relid) into :cnt from iirelation
                  where relid = 'iirtemp';
    if (cnt)
    {
	
	exec sql select relowner into :ownnam from iirelation where
		relid = 'iirtemp';

	cmd = STpolycat( (i4) 3,"drop '", ownnam,"'.iirtemp",buf);
	exec sql execute immediate :cmd;	
    }
	
    return ( (DU_STATUS) DUVE_YES);
}

/*{
** Name:  duve_4drop_expired
**
** Description:
**
**	This routine allocates memory and enters a retrieve loop to build a list
**	of all expired relations in the database.  Then it walks that list,
**	dropping each expired relation and performing a commit to release DMF
**	LOCK resoruces.  Once it has completed execution, it frees the allocated
**	memory.
**
** Inputs:
**      duve_cb				verifydb control block
**
** Outputs:
**      duve_cb				verifydb control block
**	Returns:
**	    DUVE_YES			success
**	    DUVE_BAD			error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Expired relations will be purged
**
** History:
**	05-jan-1993 (teresa)
**	    created to fix bugs 39397 and 57492.
**	26-jun-96 (nick)
**	    drop out immediately if nothing qualifies
[@history_template@]...
*/
duve_4drop_expired(duve_cb)
DUVE_CB		    *duve_cb;
{
    EXEC SQL BEGIN DECLARE SECTION;
	char		tabnam[DB_MAXNAME +1];
	char		ownnam[DB_MAXNAME +1];
	char		*cmd,buf[CMDBUFSIZE];
	i4		relsave, current_time;
	u_i4		cnt;
    EXEC SQL END DECLARE SECTION;
    typedef struct _EXPIRE_LIST	    EXPIRE_LIST;
    struct _EXPIRE_LIST
    {
	char		table[DB_MAXNAME+1];	    /* table name */
	char		owner[DB_MAXNAME+1];	    /* owner name */
    } ;
    u_i4	    len;
    EXPIRE_LIST	    *expired_list;
    u_i4	    size;
    STATUS	    mem_stat;
    i4	    i;


    current_time = TMsecs();
    exec sql select count(relsave) into :cnt from iirelation
	where relsave is not NULL
	      and (relsave < :current_time and relsave > 0);
	      
    if (cnt == 0)
	return((DU_STATUS)DUVE_YES);

    expired_list = NULL;
    size = cnt * sizeof(EXPIRE_LIST);
    expired_list = (EXPIRE_LIST *) MEreqmem(0, size, TRUE, &mem_stat);
    if ( (mem_stat != OK) || (expired_list == NULL) )
    {
	/* could not allocate memory, so fall back to old version of this
	** routine that does not do a commit after each drop.
	*/
	return( duve_4a_drop_expired(duve_cb) );
    }

    i = 0;
    EXEC SQL SELECT relid, relowner into :tabnam, :ownnam
	FROM iirelation 
	where relsave is not NULL
	      and (relsave < :current_time and relsave > 0);
    EXEC SQL BEGIN;
	STcopy (tabnam, expired_list[i].table);
	STcopy (ownnam, expired_list[i++].owner);
    EXEC SQL END;

    for( i = 0; i < cnt; i++)
    {
	/* this relation is expired 
	**  it may be a base table, an index table or a view 
	*/
	STcopy (expired_list[i].table, tabnam);
	STcopy (expired_list[i].owner, ownnam);
	len = STtrmwhite(tabnam);
	len = STtrmwhite(ownnam);
	duve_talk (DUVE_MODESENS, duve_cb,S_DU1702_EXPIRED_TABLE_FOUND, 4, 
	    0, tabnam, 0, ownnam);
	if (duve_talk (DUVE_ASK, duve_cb, S_DU0401_DROP_EXPIRED_RELATION, 4,
	    0, tabnam, 0, ownnam) == DUVE_YES)
	{
	    /* duve_cb->duve_msg->du_ingerr will be set to a non-zero value
	    ** if the execute immediate cmd encounters a server error.*/
	    duve_cb->duve_msg->du_ingerr = 0;
	    
	    cmd = STpolycat( (i4) 4,"drop '", ownnam,"'.",tabnam,buf);
	    exec sql execute immediate :cmd;
	    if (duve_cb->duve_msg->du_ingerr)
	    {
		/* the server generated an error during this drop
		** statement -- probably caused because tablename is an SQL
		** reserved word that was created by QUEL. */
		duve_talk(DUVE_MODESENS, duve_cb,
			  E_DU5034_EXPIRED_SYNTAX_ERROR, 0);
		duve_cb->duve_msg->du_ingerr = 0;
	    }
	    else
	    {
		duve_talk (DUVE_MODESENS, duve_cb,
			   S_DU0411_DROP_EXPIRED_RELATION, 4,
			   0, tabnam, 0, ownnam);
		exec sql commit;
	    }
	} /* endif drop permission given */

    } /* endif loop searching for expired relation */

    /* free up allocated memory */
    MEfree ( (PTR) expired_list);

    return ( (DU_STATUS) DUVE_YES);
}

/*{
** Name:  duve_4a_drop_expired
**
** Description:
**
**	This routine creates a dynamic SQL command to drop an expired
**	relation and then executes it.  It is called when duve_4drop_expired()
**	cannot be run because there is not enough memory to allocate the list
**	that duve_4drop_expired() builds.
**
** Inputs:
**      duve_cb				verifydb control block
**
** Outputs:
**      duve_cb				verifydb control block
**	Returns:
**	    DUVE_YES			success
**	    DUVE_BAD			error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Expired relations will be purged
**
** History:
**	17-May-1989 (teg)
**	    Initial Creation
**	05-feb-1990 (teg)
**	    Added logic to NOT terminate if dbms server returns an error
**	    when executing the drop command.  This was added because it is
**	    possible that a QUEL table may have expired.  The name of that
**	    QUEL table may be an SQL reserved word.
**	05-jan-1993 (teresa)
**	    renamed as part of fix for bugs 39397 and 57492.
[@history_template@]...
*/
duve_4a_drop_expired(duve_cb)
DUVE_CB		    *duve_cb;
{
    EXEC SQL BEGIN DECLARE SECTION;
	char		tabnam[DB_MAXNAME +1];
	char		ownnam[DB_MAXNAME +1];
	char		*cmd,buf[CMDBUFSIZE];
	i4		relsave, current_time;
    EXEC SQL END DECLARE SECTION;
    u_i4	    len;

    current_time = TMsecs();

    EXEC SQL DECLARE r_cursor CURSOR FOR 
	select relid, relowner, relsave from iirelation 
	where relsave is not NULL
	      and (relsave < :current_time and relsave > 0);

    EXEC SQL OPEN r_cursor;

    for(;;)
    {

        EXEC SQL FETCH r_cursor INTO :tabnam, :ownnam, :relsave;
	if (sqlca.sqlcode == DU_SQL_NONE)
	{
	    EXEC SQL CLOSE r_cursor;
	    break;
	}

	/* this relation is expired 
	**  it may be a base table, an index table or a view 
	*/
	len = STtrmwhite(tabnam);
	len = STtrmwhite(ownnam);
	duve_talk (DUVE_MODESENS, duve_cb,S_DU1702_EXPIRED_TABLE_FOUND, 4, 
	    0, tabnam, 0, ownnam);
	if (duve_talk (DUVE_ASK, duve_cb, S_DU0401_DROP_EXPIRED_RELATION, 4,
	    0, tabnam, 0, ownnam) == DUVE_YES)
	{
	    /* duve_cb->duve_msg->du_ingerr will be set to a non-zero value
	    ** if the execute immediate cmd encounters a server error.*/
	    duve_cb->duve_msg->du_ingerr = 0;
	    
	    cmd = STpolycat( (i4) 4,"drop '", ownnam,"'.",tabnam,buf);
	    exec sql execute immediate :cmd;
	    if (duve_cb->duve_msg->du_ingerr)
	    {
		/* the server generated an error during this drop
		** statement -- probably caused because tablename is an SQL
		** reserved word that was created by QUEL. */
		duve_talk(DUVE_MODESENS, duve_cb,
			  E_DU5034_EXPIRED_SYNTAX_ERROR, 0);
		duve_cb->duve_msg->du_ingerr = 0;
	    }
	    else
		duve_talk (DUVE_MODESENS, duve_cb,
			   S_DU0411_DROP_EXPIRED_RELATION, 4,
			   0, tabnam, 0, ownnam);
	} /* endif drop permission given */

    } /* endif loop searching for expired relation */

    return ( (DU_STATUS) DUVE_YES);
}

/*{
** Name:  DUVE_PATCHTABLE
**
** Description:
**
**	This routine causes a table to be patched to heap with all structural
**	dammage repaired.  The majority of the work is done in the DBMS server
**	via an internal procedure call.
**
** Inputs:
**      duve_cb				verifydb control block
**	dbname				name of db that table resides in
**	tablename			name of table to patch.
**
** Outputs:
**      duve_cb				verifydb control block
**	Returns:
**	    DUVE_YES			success
**	    DUVE_BAD			error
**	Exceptions:
**	    none
**
** Side Effects:
**	    The patched table will become a heap, regardless of what storage
**	    structure it started as.  Also, any seconday indexes on this table
**	    will go away.
**	    Several messages will be returned/processed by duve_msg during
**	    execution of the internal procedure iiqef_patch_table.
**
** History:
**	08-nov-1989 (teg)
**	    Initial Creation
**      02-dec-1991 (teresa)
**          fix bug 41119
**	23-oct-92 (teresa)
**	    implement sir 42498.
**	16-sep-93 (teresa)
**	    Fix bug 55048.
**	27-jun-1997 (nanpr01)
**	    Daryl of Lucent get syntax error when he tries to run verifydb
**	    with run and table option with unique/referential/primary
**	    constraints.
**	11-Jun-2007 (kibro01) b117215
**	    Don't select partitions as if they were indices - they'll be
**	    dealt with in the main dm1u_verify section.
**       6-Apr-2009 (hanal04) Bug 120517
**          If we are SQL-92 compliant table names must be in upper case.
**          We can't check for this any earlier as we need to issue a
**          dbmsinfo() call.
[@history_template@]...
*/
DU_STATUS
duve_patchtable( duve_cb, dbname, tablename)
DUVE_CB		*duve_cb;
char		*dbname;
char		*tablename;
{
    exec sql begin declare section;
	char	tbl_name[DB_MAXNAME+1];
	char	index_name[DB_MAXNAME+1];
	char	index_owner[DB_MAXNAME+1];
	char	owner_name[DB_MAXNAME+1];
	char	mode[40];
	int 	table_id=0;
	i4	relstat=0;
	i4	relstat2=0;
	char	*cmd, buf[CMDBUFSIZE];
	char	verbose[10];
	char	cons_type[2];
	char	fipsbuf[6];
    exec sql end declare section;
	bool	has_constraint = FALSE;
        bool	isfipsid = FALSE;
	DU_STATUS ask =  DUVE_BAD;

    exec sql whenever sqlerror call Clean_Up;
    EXEC SQL WHENEVER SQLMESSAGE CALL duve_msg;

    STcopy( tablename, tbl_name);
    if (duve_cb->duve_user[0]=='\0')
	STcopy(duve_cb->duveowner, owner_name);
    else
	STcopy(duve_cb->duve_user, owner_name);

    /* determine which mode to used based on OPERATION */
    if (duve_cb->duve_operation == DUVE_XTABLE)
	STcopy( "force",mode);
    else
	STcopy( "save_dba",mode);

    /* 
    **	connect to database  (as user $ingres)
    */
    if ( duve_opendb(duve_cb, dbname) == DUVE_BAD)
	return ( (DU_STATUS) DUVE_BAD);

    /*
    ** decide if this installation is FIPS or not
    */
    exec sql select dbmsinfo('db_name_case') into :fipsbuf;
    if (STcompare(fipsbuf, ERx("UPPER")) == DU_IDENTICAL)
        isfipsid = TRUE;

    if (isfipsid)
        CVupper(tbl_name);

    /* get the tableid, table relstat */
    exec sql select reltid,relstat into :table_id, :relstat from iirelation
	where relid = :tbl_name and relowner = :owner_name;

    /* verifyd that the table/owner pair is found in the catalogs */
    if ((table_id == 0) && (relstat == 0))
    {
	duve_talk(DUVE_MODESENS, duve_cb, E_DU5025_NO_SUCH_TABLE, 4,
			  0, tbl_name, 0, owner_name);
        return ( (DU_STATUS) DUVE_BAD);
    }

    /* verify that the table is not an s_condor catalog or secondary index or
    ** view */
    if (relstat & TCB_CONCUR)
    {
	duve_talk( DUVE_MODESENS, duve_cb, E_DU502F_CANT_PATCH_SCONDUR, 0);
	duve_close(duve_cb, TRUE);
	return ( (DU_STATUS) DUVE_BAD);
    }
    if (relstat & TCB_INDEX)
    {
	duve_talk( DUVE_MODESENS, duve_cb, E_DU5030_CANT_PATCH_INDEX, 0);
	duve_close(duve_cb, TRUE);
	return ( (DU_STATUS) DUVE_BAD);
    }
    if (relstat & TCB_VIEW)
    {
	duve_talk( DUVE_MODESENS, duve_cb, E_DU5038_CANT_PATCH_VIEW, 0);
	duve_close(duve_cb, TRUE);
	return ( (DU_STATUS) DUVE_BAD);
    }

    /* If journaling is enabled, turn it off until next checkpoint */
    if (relstat & TCB_JOURNAL)
    {
	relstat = (relstat & ~TCB_JOURNAL) | TCB_JON;
	exec sql update iirelation set relstat = :relstat
	    where relid = :tbl_name and relowner = :owner_name;
    }

    /*
    ** create the internal procedure to do the patch table operation
    */
    EXEC SQL WHENEVER SQLERROR continue;
	exec sql drop procedure iiQEF_patch_table;
    EXEC SQL WHENEVER SQLERROR call Clean_Up;
    exec sql create procedure
	    iiQEF_patch_table(
			table_id i4 not null not default,
			mode char(8) not null not default,
			verbose char(10) not null not default ) as
	    BEGIN
	        execute internal;
	    END;	    

    /* 
    **	enter a loop to delete all secondary indices assocated with the table 
    */
    EXEC SQL DECLARE rel_cursor CURSOR FOR 
	select relid, relowner , relstat2 from iirelation 
	where reltid = :table_id and reltidx != 0;
    EXEC SQL OPEN rel_cursor;

    for ( ;; )
    {
	/*
	** get entry from iirelation
	*/
	EXEC SQL FETCH rel_cursor INTO :index_name, :index_owner, :relstat2;
	if (sqlca.sqlcode == DU_SQL_NONE)
	{	
	    EXEC SQL CLOSE rel_cursor;
	    break;
        }

	if (relstat2 & TCB2_PARTITION)
	{
	    /* Ignore partitions while removing indices */
	    continue;
	}

	if (relstat2 & TCB_SYSTEM_GENERATED)
	{
	    has_constraint = TRUE;
	    continue;
	}

	(VOID) STzapblank(index_name, index_name);
	(VOID) STzapblank(index_owner, index_owner);

	duve_talk( DUVE_MODESENS, duve_cb, S_DU1710_2NDARY_INDEX, 4,
			0, index_name, 0, index_owner);
	if (duve_talk (DUVE_ASK, duve_cb, S_DU0420_DROP_INDEX, 0)
	     == DUVE_YES)
	{
	    cmd = STpolycat( (i4) 4,"drop ", index_owner,".",index_name, buf);
	    exec sql execute immediate :cmd;
	    duve_talk (DUVE_MODESENS, duve_cb, S_DU0421_DROP_INDEX, 0);
	}
	else
	{
	    duve_close(duve_cb, TRUE);
	    return( (DU_STATUS) DUVE_NO);
	}

    } /* end loop to remove secondary indices. */

    if (has_constraint)
    {
    	/* 
    	**	enter a loop to delete all constraint assocated with the table 
    	*/
    	EXEC SQL DECLARE rel_cons CURSOR FOR 
	select constraint_name, constraint_type from iiconstraints
		where table_name = :tbl_name and schema_name = :owner_name;
    	EXEC SQL OPEN rel_cons;

    	for ( ;; )
    	{
	    /*
	    ** get entry from iirelation
	    */
	    EXEC SQL FETCH rel_cons INTO :index_name, :cons_type;
	    if (sqlca.sqlcode == DU_SQL_NONE)
	    {	
	        EXEC SQL CLOSE rel_cons;
	        break;
       	    }

	    switch (cons_type[0])
	    {
		case 'U' : 
		  duve_talk( DUVE_MODESENS, duve_cb, 
				S_DU1711_UNIQUE_CONSTRAINT, 2,
				0, index_name);
	    	  ask = duve_talk (DUVE_ASK, duve_cb, 
				S_DU0422_DROP_UNIQUE_CONSTRAINT, 0);
		  break;
		case 'C' :
		  duve_talk( DUVE_MODESENS, duve_cb, 
				S_DU1712_CHECK_CONSTRAINT, 2,
				0, index_name);
	    	  ask = duve_talk (DUVE_ASK, duve_cb, 
				S_DU0424_DROP_CHECK_CONSTRAINT, 0);
		  break;
		case 'R' :
		  duve_talk( DUVE_MODESENS, duve_cb, 
				S_DU1713_REFERENCES_CONSTRAINT, 2,
				0, index_name);
	    	  ask = duve_talk (DUVE_ASK, duve_cb, 
				S_DU0426_DROP_REF_CONSTRAINT, 0);
		  break;
		case 'P' :
		  duve_talk( DUVE_MODESENS, duve_cb, 
				S_DU1714_PRIMARY_CONSTRAINT, 2,
				0, index_name);
	    	  ask = duve_talk (DUVE_ASK, duve_cb, 
				S_DU0428_DROP_PRIMARY_CONSTRAINT, 0);
		  break;
	    }
	    if (ask == DUVE_YES)
	    {
	        cmd = STpolycat( (i4) 7, "alter table ", owner_name, ".",
			tbl_name, " drop constraint \"", index_name, 
			"\" cascade", buf);
	        exec sql execute immediate :cmd;
		switch (cons_type[0])
		{
		    case 'U' : 
	        	duve_talk (DUVE_MODESENS, duve_cb, 
				S_DU0423_DROP_UNIQUE_CONSTRAINT, 0);
			break;
		    case 'C' : 
	        	duve_talk (DUVE_MODESENS, duve_cb, 
				S_DU0425_DROP_CHECK_CONSTRAINT, 0);
			break;
		    case 'R' : 
	        	duve_talk (DUVE_MODESENS, duve_cb, 
				S_DU0427_DROP_REF_CONSTRAINT, 0);
			break;
		    case 'P' : 
	        	duve_talk (DUVE_MODESENS, duve_cb, 
				S_DU0429_DROP_PRIMARY_CONSTRAINT, 0);
			break;
		}
	    }
	    else
	    {
	        duve_close(duve_cb, TRUE);
	        return( (DU_STATUS) DUVE_NO);
	    }

    	} /* end loop to remove secondary indices. */
    }

    /* set verbose flag appropriately */
    if (duve_cb->duve_verbose)
	STcopy (VERBOSE, verbose);
    else
	STcopy (NOVERBOSE, verbose);

    /*
    ** now go have DMF do the patch table operation
    */
    EXEC SQL WHENEVER SQLERROR continue;
    exec sql execute procedure iiQEF_patch_table(table_id = :table_id, 
						 mode = :mode,
						 verbose = :verbose);
    if (sqlca.sqlcode < 0)
	Clean_Up();
    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    /* the procedure updates iirelation, but does not update iiattribute.  If
    ** the procedure did not succeed, then we would not get here, as the
    ** WHENEVER SQL ERROR call Clean_Up exit would be taken.  So the procedure
    ** has been successful, and it is time to clear the attkdom field from
    ** iiattribute (if set).  That is because the table has been converted to
    ** a heap, and it is not reasonable to have keys for a heap table.
    */
    EXEC SQL update iiattribute set attkdom = 0 where attkdom > 0 and 
	attrelid = :table_id and attrelidx = 0;

    /* close the DB without aborting */
    duve_close(duve_cb, FALSE);
    return ( (DU_STATUS) DUVE_YES);
}

/*{
** Name:  DUVE_CHECKTABLE
**
** Description:
**
**	This routine obtains the table id, index table id from iirelation for
**	the specified owner.table.  Then it creates/executes the internal
**	procedure to have DMF check the specified table.
**
** Inputs:
**      duve_cb				verifydb control block
**	dbname				name of db that table resides in
**	tablename			name of table to check.
**
** Outputs:
**      duve_cb				verifydb control block
**	Returns:
**	    DUVE_YES			success
**	    DUVE_BAD			error
**	Exceptions:
**	    none
**
** Side Effects:
**	    several messages will be returned/processed by duve_msg during
**	    execution of the internal procedure iiqef_check_table.
**
** History:
**	08-nov-1989 (teg)
**	    Initial Creation
**	02-dec-1991 (teresa)
**	    fix bug 41119
**	23-oct-92 (teresa)
**	    implement sir 42498.
**       6-Apr-2009 (hanal04) Bug 120517
**          If we are SQL-92 compliant table names must be in upper case.
**          We can't check for this any earlier as we need to issue a
**          dbmsinfo() call.
[@history_template@]...
*/
DU_STATUS
duve_checktable( duve_cb, dbname, tablename)
DUVE_CB		*duve_cb;
char		*dbname;
char		*tablename;
{

    exec sql begin declare section;
	char	tbl_name[DB_MAXNAME+1];
	char	owner_name[DB_MAXNAME+1];
	int 	table_id;
	int	index_id;
	int	relspec;
	int	relstat;
	char	*cmd,buf[CMDBUFSIZE];
	char	verbose[10];
	char	fipsbuf[6];
    exec sql end declare section;
        bool    isfipsid = FALSE;

    exec sql whenever sqlerror call Clean_Up;
    EXEC SQL WHENEVER SQLMESSAGE CALL duve_msg;

    STcopy( tablename, tbl_name);
    if (duve_cb->duve_user[0]=='\0')
	STcopy(duve_cb->duveowner, owner_name);
    else
	STcopy(duve_cb->duve_user, owner_name);

    /* 
    **	connect to database  (as user $ingres)
    */
    if ( duve_opendb(duve_cb, dbname) == DUVE_BAD)
	return ( (DU_STATUS) DUVE_BAD);

    /*
    ** decide if this installation is FIPS or not
    */
    exec sql select dbmsinfo('db_name_case') into :fipsbuf;
    if (STcompare(fipsbuf, ERx("UPPER")) == DU_IDENTICAL)
        isfipsid = TRUE;

    if (isfipsid)
        CVupper(tbl_name);

    /* get the tableid, indexid */
    exec sql select reltid,reltidx,relspec,relstat into :table_id, :index_id,
	:relspec, :relstat from iirelation
	where relid = :tbl_name and relowner = :owner_name;
    if (sqlca.sqlcode == DU_SQL_NONE)
    {       
	duve_talk( DUVE_ALWAYS, &duvecb, E_DU5031_TABLE_DOESNT_EXIST, 4,
		   0, tbl_name, 0, owner_name);
	    duve_close(duve_cb, TRUE);
	    return ( (DU_STATUS) DUVE_BAD);
    }

    /* refuse to operate on views */
    if (relstat & TCB_VIEW)
    {
	duve_talk( DUVE_MODESENS, duve_cb, E_DU5038_CANT_PATCH_VIEW, 0);
	duve_close(duve_cb, TRUE);
	return ( (DU_STATUS) DUVE_BAD);
    }

    /*
    ** create the internal procedure to do the check table operation
    */
    EXEC SQL WHENEVER SQLERROR continue;
	exec sql drop procedure iiQEF_check_table;
    EXEC SQL WHENEVER SQLERROR call Clean_Up;
    exec sql create procedure
	    iiQEF_check_table(
			table_id i4 not null not default,
			index_id i4 not null not default,
			verbose char(10) not null not default ) as
	    BEGIN
	        execute internal;
	    END;	    

    /* set verbose flag appropriately */
    if (duve_cb->duve_verbose)
	STcopy (VERBOSE, verbose);
    else
	STcopy (NOVERBOSE, verbose);


    /*
    ** now go have DMF do the check table operation
    */
    EXEC SQL WHENEVER SQLERROR continue;
    exec sql execute procedure iiQEF_check_table(table_id = :table_id, 
						 index_id = :index_id,
						 verbose  = :verbose );
    if (sqlca.sqlcode < 0)
	Clean_Up();
    EXEC SQL WHENEVER SQLERROR call Clean_Up;

    /* close the DB without aborting */
    duve_close(duve_cb, FALSE);
    return ( (DU_STATUS) DUVE_YES);
}

VOID
duve_msg()
{

    EXEC SQL WHENEVER SQLERROR continue;
    char	*msg_ptr;
    char	underscore = '_';
    char	*under_ptr = &underscore;
    char	*offset;

    EXEC SQL BEGIN DECLARE SECTION;
	char msg[2048];
	int msgnum;
    EXEC SQL END DECLARE SECTION;

    EXEC SQL INQUIRE_INGRES (:msgnum = MESSAGENUMBER, :msg = MESSAGETEXT);

    /* the message from the server has a timestamp that should be stripped
    ** unless the -timestamp flag has been specified */
    if (duvecb.duve_timestamp)
    {
	offset = STindex( msg, under_ptr, 2048);
	if (offset==NULL)
	    msg_ptr = &msg[0];
	else
	{
	    i4	delta;

	    /* calculate start of msg, which is two char before underscore,
	    ** unless underscore is at beginning of msg */
	    
	    /* lint truncation warning if size of ptr > int, but code valid */
	    delta = (i4) (offset -  &msg[0]);
	    if (delta >= 2)
	    {
		delta = delta -2;
		msg_ptr = &msg[delta];
	    }
	    else
		msg_ptr = &msg[0];
	}  /* endif */
    }
    else
	msg_ptr = &msg[0];


    /* output the message to USER CRT unless mode is RUNSILENT */
    if (duvecb.duve_mode != DUVE_SILENT)
	    SIprintf("%s\n",msg_ptr);
    /*
    ** Always attempt to output message to log file -- duvelog will prevent
    ** message from being logged if -nolog option was specified.
    */
    duve_log( &duvecb, 0, msg_ptr);
}
