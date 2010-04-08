/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<st.h>
# include       <clconfig.h>
# include	<nm.h>
# include	<me.h>
# include	<meprivate.h>
# include	<lo.h>
# include	"lolocal.h"

/**
**
**  Name: LOINGPATH.C - Build ingres path.
**
**  Description:
**	Build a path to an ingres object.
**
**          LOingpath() - build a path to the ingres object.
**	    LOmkingpath() - make the directories to that path.
**
**
**  History:
**      26-jul-88 (mmm)
**	    new coding standards / slash fix problem concerning unix specific
**	    way of mounting alterate locations beneath the standard location
**	    place (ie. mounting the filesystem altloc in:
**		$II_DATABASE/ingres/data/altloc.
**	02/23/89 (daveb, seng)
**	    rename LO.h to lolocal.h
**	08-aug-89 (greg)
**	    add support for LO_DMP
**	23-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Add "include <clconfig.h>".
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	16-aug-93 (ed)
**	    add <st.h>
**      19-dec-94 (sarjo01) from 30-aug-94 (nick)
**          fix bug 64457 - LOingpath() trashes slash when passed '/'
**	03-jun-1996 (canor01)
**	    When using operating-system threads, use a different static 
**	    buffer for each thread.
**      22-nov-1996 (canor01)
**          Changed names of CL-specific thread-local storage functions
**          from "MEtls..." to "ME_tls..." to avoid conflict with new
**          functions in GL.  Included <meprivate.h> for function prototypes.
**      07-mar-1997 (canor01)
**          Allow functions to be called from programs not linked with
**          threading libraries.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	25-Mar-2001 (jenjo02)
**	    Added support for special RAW location paths.
**	    See added comments.
**	10-May-2001 (jenjo02)
**	    Redid RAW location detection. Use normal location
**	    path, but look for special raw filename.
**	11-May-2001 (jenjo02)
**	    Check return code from LOexist, distinguish permissions
**	    problems from file-not-found.
**	15-Oct-2001 (jenjo02)
**	    Added LOmkingpath() to create missing location area
**	    directories.
**	15-Jun-2004 (schka24)
**	    Safer env variable handling.
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**/


/*
**  Globals
*/
static char		buf[MAX_LOC + 1];
# ifdef OS_THREADS_USED
static ME_TLS_KEY	LOingpathkey = 0;
# endif /* OS_THREADS_USED */
/*
**  Defines of other constants.
*/

/*  macro for efficient comparison of strings (avoid proc calls if possible) */

# define STEQ_MACRO( a, b ) ( a[0] == b[0] && !STcompare(a, b) )


/*{
** Name: LOingpath()	- Build INGRES path 
**
** Description:
**
**      Concatenate "area", "dbname" and "what" to form the "fullpath"
**      location of the database (ckp, dmp, work or jnl) specified.
**      
**      This routine is used to find the locations containing database,
**      checkpoint, dump, work and journal files.  It performs all the
**      remapping implied by system symbols and by the RODV (relation on
**      different volumes) mechanism.
**      
**      The "area" argument is a system dependent string that is stored in
**      the iidbdb.  (The interpretation will differ on various systems because 
**	of inconsistent directory structures.)  The string has a maximum length 
**	of DB_MAXNAME characters. This restriction is imposed by client code
**      client code and the iidatabase catalog, not by LO.  
**      
**      The "dbname" argument is just a string of the database name, again
**	limited to DB_MAXNAME.
**      
**      Standard values for the "what" argument are supplied in LO.h:
**      
**          LO_DB, LO_CKP, LO_DMP, LO_JNL, LO_WORK
**      
**      referring to the database, checkpoint, journal, and work directories
**      respectively.  Only these symbolic constants should be used to identify
**      these directories.  The values of the constants may be system dependent.
** 
**      The "fullpath" argument is the address of a LOCATION that will be
**      filled in with the resolved path.
**      
**	LOingpath() expands the "II_DATABASE", "II_CHECKPOINT", "II_DUMP",
**	"II_WORK", and "II_JOURNAL" symbols (determined by the "what" string)
**	into the destination location in a system dependent manner.  The
**	II_DATABASE etc. symbols are used to locate databases (checkpoints or
**	journals).  These release symbols must be defined in 6.0 or an error
**	will result (previous releases defaulted to the same location as the
**	installation.) These symbols are independent of the ``areas'' used by
**	the RODV mechanism.
**
**	As mentioned above, the interpretation of the "area" argument varies
**      because of the different directory structures in use.  The nature of the
**      constructed location on the current systems is described below.
**      
**      On VMS, "area" is presumed to be the name of a device, and is
**      usually a logical.  The concatenation takes the form
**      "area::path:dbname" where path looks like "[ingres.what]".
**      The routine ensures that devices names are legal (all caps, etc.).
**	No mapping of II_DATABASE is required, since it is mapped by the system
**	when files are opened.
**      
**      CMS emulates the VMS directory scheme and symbol mechanism.
**      No II_DATABASE mapping is needed.
**      
**      On UNIX the concatenation takes the forms described below.
**      It is complicated by the baroque nature of the INGRES/UNIX directory 
**	structure.  In particular, "area" has three radically different 
**	interpretations.
**      
**      1.  If the "area" is a rooted path
**      (limited to DB_MAXNAME characters, as noted above),
**      it is expanded as "/area/ingres/what/default".
**      The "/ingres/" level must be present.
**      
**      2.  If the "area" is one of "II_DATABASE", "II_CHECKPOINT", "II_WORK", 
**	"II_DUMP" or "II_JOURNAL" then it interpreted as a special case and uses
**	the ingres NM routines to determine the symbol value for the area name.
**	This area name is used in the iidbdb to specify the default database,
**	journal, work, or checkpoint area to be used; and that the default 
**	area is determined by one of the appropriate environment variables 
**	"II_DATABASE", "II_CHECKPOINT", "II_DUMP", "II_WORK", or "II_JOURNAL".
**	Note that on unix these symbols are case sensitive (ie. if "ii_system"
**	is passed in as an area the unix LOingpath() routine will interpret it
**	as case #3 described below).
**
**	The symbol must be defined as a rooted path with the same
**	characteristics as case 1.  If the default location for databases is
**	to be /db_disk, then the value of II_DATABASE must be "/db_disk". 
**	The area in this case is expanded to "/db_disk/ingres/data/default".
**      The "/ingres/" level must be present.
**
**	3. If the "area" is not a rooted path or one of the special symbols
**	then the concatenation takes the form "XXX/ingres/what/area".  "XXX" is
**	resolved as follows:  If the "what" requested is one of LO_DB, LO_CKP,
**	LO_DMP, LO_WORK, LO_JNL then NM is used to look up the value of the
**	appropriate symbol for the "what" requested (e.g. "II_DATABASE" when
**	"what" is "LO_DB") and it is used as the "XXX" base; it is an error in
**	this case if the appropriate symbol is not set.  If the "what"
**	requested is not one of LO_DB, LO_CKP, LO_DMP, LO_WORK, LO_JNL then the
**	value of II_SYSTEM is used as "XXX".
**
**      By setting the "area" in the iidbdb, the user may place databases
**      (ckp, journal, dump or work) on separate volumes.
**      By mounting the volume at the same level as the ``default''
**      ones in the default database, journal, and checkpoint locations -
**	{$II_DATABASE,$II_CHECKPOINT,$II_JOURNAL}/ingres/{data,ckp,jnl}/default
**      relative ``areas'' are used.
**      Alternatively, full paths can be used for
**      the ``area'' by building a separate INGRES directory hierarchy
**      (including an "/ingres/" level) on another filesystem, as long as
**      the ``area'' name is less than DB_MAXNAME characters in length.
**      
**      Because UNIX does not have the system level symbol mapping of VMS,
**      II_DATABASE etc. must be expanded when the "fullpath" location is
**      created, rather than deferred to the operating system.  The variables
**      must resolve to a full UNIX directory path.
**      II_DATABASE may be used to work around the arbitrary DB_MAXNAME 
**	character limit on "area" names.
**
**	RAW area implications:
**
**	    Raw location are identified by the existence of a specially
**	    named file, "iirawdevice" (aka LO_RAW), occuring in the 
**	    <area>/ingres/data/default directory.
**
**	    Note that the default data location II_DATABASE can never
**	    map to a raw device, and that only "data" locations
**	    can be defined to raw devices.
**
** Inputs:
**	area				"device" that the database is on
**	dbname				ingres database name
**	what				path to db, ckp or jnl
**	fullpath			set to full path of database.
**	
** Outputs:
**	none.
**
** History:
**	06-27-86 (daveb)
**	    Add support for DB_INGRES, CKP_INGRES and JNL_INGRES,
**      28-jul-88 (mmm)
**          New comments, new coding standards, fix support of relative
**	    alternate areas in default db, jnl, ckp locations.
**	08-aug-89 (greg)
**	    Add support for LO_DMP.
**	07-nov-92 (jrb)
**	    Add support for LO_WORK.
**      19-dec-94 (sarjo01) from 30-aug-94 (nick)
**          fix bug 64457 - LOingpath() trashes slash when passed '/'
**	03-jun-1996 (canor01)
**	    When using operating-system threads, use a different static 
**	    buffer for each thread.
**	18-oct-96 (mcgem01)
**	    Remove hard-coded references to the 'ingres' directory.
**	25-Mar-2001 (jenjo02)
**	    Added RAW location support.
**	10-May-2001 (jenjo02)
**	    Redid RAW location detection. Use normal location
**	    path, but look for special raw filename.
**	17-Oct-2001 (jenjo02)
**	    Can't return fname=LO_RAW, which exposes non-portable
**	    LOCATION structure to generic code.
**	    Now, to detect RAW location, caller must signal intent
**	    by passing "LO_RAW" in "dbname". The RAW test is then 
**	    performed and "LO_IS_RAW" returned in status if true.
**	    Pushed test for RAW vs LO_DB up to DMF.
**	15-Jun-2004 (schka24)
**	    Watch out for buffer overrun.
*/



STATUS
LOingpath(area, dbname, what, fullpath)
char		*area;		/* "device" that the database is on */
char		*dbname;	/* ingres data base name */
char		*what;		/* path to db, ckp, dmp, work, or jnl */
LOCATION	*fullpath;	/* set to full path of database */
{
    register char	*c = NULL;
    register i4	length;
    register char	*dbptr = dbname;
    register char	*symbol;
    STATUS		ret_val = OK;
    char		*value = NULL;
    char		*bufptr = NULL;
    STATUS		local_status;
    i4			try_raw = FALSE;

# ifdef OS_THREADS_USED
    if ( LOingpathkey == 0 )
    {
        ME_tls_createkey( &LOingpathkey, &local_status );
        ME_tls_set( LOingpathkey, NULL, &local_status );
    }
    if ( LOingpathkey == 0 )
    {
	/* not linked with threaded libraries */
	LOingpathkey = -1;
    }
    if ( LOingpathkey == -1 )
    {
	bufptr = buf;
    }
    else
    {
        ME_tls_get( LOingpathkey, (PTR *)&bufptr, &local_status );
        if ( bufptr == NULL )
        {
            bufptr = (char *) MEreqmem(0, MAX_LOC+1, TRUE, NULL );
            ME_tls_set( LOingpathkey, (PTR)bufptr, &local_status );
        }
    }
# else /* OS_THREADS_USED */
    bufptr = buf;
# endif /* OS_THREADS_USED */

    /* Only check for RAW if requested by dbname==LO_RAW */
    if ( (try_raw = dbptr && STEQ_MACRO( dbptr, LO_RAW)) )
	dbptr = NULL;

    /* check for legal arguments */

    if ((area == NULL) || (*area == '\0')	||
	((length = STtrmwhite(area)) == 0)	||
	(what == NULL) || (*what == '\0'))
    {
	ret_val = FAIL;
    }
    else
    {
	if (area[length - 1] == SLASH)
	    area[length - 1] = NULLCHAR;

	if (area[0] == SLASH || area[0] == NULLCHAR)
	{
	    /* area is a full path */
	    STlpolycat(6, MAX_LOC, area, SLASHSTRING, SystemLocationSubdirectory,
		    SLASHSTRING, what, "default", bufptr);
	    LOfroms(PATH, bufptr,  fullpath);
	}
	else
	{
	    /* area is a relative path, or a special symbol */

	    /*
	    ** If it's a special known 'what', then
	    ** check the special variables that might
	    ** remap the base directory.  This duplicates VMS
	    ** functionality, and is needed for 3270/FDX
	    ** co-resident installations on UTS. (daveb)
	    */
	    symbol = NULL;

	    /* ordered by estimated match frequency */

	    if ( STEQ_MACRO( what, LO_DB ) )
		symbol = "II_DATABASE";
	    else if ( STEQ_MACRO( what, LO_JNL) )
		symbol = "II_JOURNAL";
	    else if ( STEQ_MACRO( what, LO_CKP ) )
		symbol = "II_CHECKPOINT";
	    else if ( STEQ_MACRO( what, LO_DMP ) )
		symbol = "II_DUMP";
	    else if ( STEQ_MACRO( what, LO_WORK ) )
		symbol = "II_WORK";

	    if ( symbol != NULL )
		    NMgtAt( symbol, &value );

	    if ((STEQ_MACRO(area, "II_DATABASE"))	||
	        (STEQ_MACRO(area, "II_JOURNAL"))	||
	        (STEQ_MACRO(area, "II_WORK"))		||
	        (STEQ_MACRO(area, "II_CHECKPOINT"))	||
		(STEQ_MACRO(area, "II_DUMP")))
	    {
		/* The area is one of the magic "default" symbols, so the
		** base of the path must be gotten from the appropriate
		** symbol - it is an error if the appropriate symbol is not
		** set (the install script sets all these variables in 
		** the "symbol.tbl")
		*/

		/* Default area can't be raw */
		try_raw = FALSE;

		if (value == NULL || *value == '\0')
		{
		    ret_val = FAIL;
		}
		else
		{
		    /* Use symbol value as base directory */

		    STlpolycat(6, MAX_LOC, value, SLASHSTRING,
			SystemLocationSubdirectory, SLASHSTRING,
			what, "default", bufptr);
		    LOfroms( PATH, bufptr, fullpath );
		}
	    }
	    else
	    {
		/* area refers to a "relative" or "what" is not known */
		    

		if ( value == NULL || *value == '\0' )
		{
		    /* Unknown 'what' so use $II_SYSTEM as base directory */

		    STlpolycat(2, MAX_LOC, what, area, bufptr);
		    NMloc(SUBDIR, PATH, bufptr, fullpath);
		}
		else
		{
		    /* Use symbol value as base directory */
		    STlpolycat(6, MAX_LOC, value, SLASHSTRING,
			SystemLocationSubdirectory, SLASHSTRING,
			what, area, bufptr);
		    LOfroms( PATH, bufptr, fullpath );
		}
	    }
	}

	/*
	** Raw locations are identified by the existence of the
	** file "iirawdevice" (LO_RAW).
	*/
	if ( try_raw )
	{
	    char		rbuf[MAX_LOC+1];
	    LOCATION		rloc;
	    LOINFORMATION	info;
	    i4			flags = (LO_I_TYPE | LO_I_PERMS);

	    /* Append special raw filename, check if extant */
	    STpolycat(3, fullpath->string, SLASHSTRING, LO_RAW, &rbuf);
	    LOfroms(PATH, rbuf, &rloc);

	    /* File must exist and be writable */
	    if ( (ret_val = LOinfo(&rloc, &flags, &info)) == OK
		&& info.li_type == LO_IS_FILE
		&& info.li_perms & LO_P_WRITE )
	    {
		ret_val = LO_IS_RAW;
	    }
	    /* It's ok if file doesn't exist, but return other errors */
	    else if ( ret_val == LO_NO_SUCH )
		ret_val = OK;
	}
    }
	
    if ( !ret_val )
    {
	/* add dbname if necessary */

	if ( (dbptr != NULL) && (*dbptr != '\0') )
	{
	    c    = fullpath->fname;
	    *c++ = SLASH;

	    while (*c++ = *dbptr++)
		;

	    c--;		/* position on NULL */
	}

	/* NOTE: c initialized to NULL above */

	fullpath->fname	 = c;
    }

    return(ret_val);
}

/*{
** Name: LOmkingpath()	- Make INGRES location's area path 
**
** Description:
**
**	Makes missing subdirectories in a location's area path.
**
** Inputs:
**	area				"device" that the location is on
**	what				path to db, ckp, dmp, work, jnl
**	loc				set to full path of location.
**	
** Outputs:
**	none.
**
** History:
**	15-Oct-2001 (jenjo02)
**	    Invented for CREATE/ALTER LOCATION.
**	24-nov-2004 (hayke02)
**	    Create directories under data, work etc. with permission 700 rather
**	    than 777.
*/
STATUS
LOmkingpath(area, what, loc)
char		*area;		/* "device" that the database is on */
char		*what;		/* path to db, ckp, dmp, work, or jnl */
LOCATION	*loc;		/* set to full path of location */
{
    i2			len;
    char		*symbol;
    STATUS		status = OK;
    char		*value;
    char		*bufptr;
    STATUS		local_status;
    LOINFORMATION	info;
    i4			flags;
    LOCATION		baseloc, *bloc = &baseloc;
    LOCATION		subloc, *sloc = &subloc;
    char		basepath[MAX_LOC+1], *bpath = &basepath[0];
    char		subpath[MAX_LOC+1], *spath = &subpath[0];
    char		*opath, *npath, *cpath;
    i4			dir_num;
    char		dir[LO_NM_LEN];

# ifdef OS_THREADS_USED
    if ( LOingpathkey == 0 )
    {
        ME_tls_createkey( &LOingpathkey, &local_status );
        ME_tls_set( LOingpathkey, NULL, &local_status );
    }
    if ( LOingpathkey == 0 )
    {
	/* not linked with threaded libraries */
	LOingpathkey = -1;
    }
    if ( LOingpathkey == -1 )
    {
	bufptr = buf;
    }
    else
    {
        ME_tls_get( LOingpathkey, (PTR *)&bufptr, &local_status );
        if ( bufptr == NULL )
        {
            bufptr = (char *) MEreqmem(0, MAX_LOC+1, TRUE, NULL );
            ME_tls_set( LOingpathkey, (PTR)bufptr, &local_status );
        }
    }
# else /* OS_THREADS_USED */
    bufptr = buf;
# endif /* OS_THREADS_USED */

    /* check for legal arguments */

    if ((area == NULL) || (*area == '\0')	||
	((len = STtrmwhite(area)) == 0)	||
	(what == NULL) || (*what == '\0'))
    {
	status = FAIL;
    }
    else
    {
	if (area[len-1] == SLASH)
	    area[len-1] = NULLCHAR;

	if (area[0] == SLASH || area[0] == NULLCHAR)
	{
	    /* area is a full path */
	    STpolycat(3, area, SLASHSTRING, SystemLocationSubdirectory, bpath);
	    LOfroms(PATH, bpath, bloc);
	    STpolycat(2, what, "default", spath);
	}
	else
	{
	    /* area is a relative path, or a special symbol */

	    /*
	    ** It must be a special known "what", so
	    ** check the special variables that might
	    ** remap the base directory.  This duplicates VMS
	    ** functionality, and is needed for 3270/FDX
	    ** co-resident installations on UTS. (daveb)
	    */

	    /* ordered by estimated match frequency */

	    if ( STEQ_MACRO( what, LO_DB ) )
		symbol = "II_DATABASE";
	    else if ( STEQ_MACRO( what, LO_JNL) )
		symbol = "II_JOURNAL";
	    else if ( STEQ_MACRO( what, LO_CKP ) )
		symbol = "II_CHECKPOINT";
	    else if ( STEQ_MACRO( what, LO_DMP ) )
		symbol = "II_DUMP";
	    else if ( STEQ_MACRO( what, LO_WORK ) )
		symbol = "II_WORK";
	    else
		return(LO_NO_SUCH);

	    NMgtAt( symbol, &value );
	    
	    /* The equivalent "what" symbol must be defined */
	    if ( value == NULL || *value == EOS )
		return(LO_NO_SUCH);

	    /* Construct the base path from the symbol value */
	    STlpolycat(3, MAX_LOC, value, SLASHSTRING,
			SystemLocationSubdirectory, bpath);
	    LOfroms(PATH, bpath, bloc);

	    /* Construct the subdirectory path from "area" */
	    if ((STEQ_MACRO(area, "II_DATABASE"))	||
	        (STEQ_MACRO(area, "II_JOURNAL"))	||
	        (STEQ_MACRO(area, "II_WORK"))		||
	        (STEQ_MACRO(area, "II_CHECKPOINT"))	||
		(STEQ_MACRO(area, "II_DUMP")))
	    {
		/* The area is one of the magic "default" symbols */
		STpolycat(2, what, "default", spath);
	    }
	    else
	    {
		/* area refers to a "relative" */
		STpolycat(2, what, area, spath);
	    }
	}

	/* Create subdirectory location */
	LOfroms( PATH, spath, sloc );

	/*
	** We now have two LOCATIONs each describing half
	** of the full path.
	**
	** The "left" one (bpath) is the "base" path, starting at root,
	** ending with .../ingres.
	**
	** The "right" one (spath) is the "what" subdirectory path which
	** will be concatenated to the base path, i.e.,
	** "what/default".
	** 
	** Separating the Ingres subdirectory path from the base
	** path helps us figure out the correct permissions for
	** each subdirectory:
	**
	**	<base_path>	each subdirectory is 0755 (rwxr-xr-x)
	**	<base_path>/ingres/what		  is 0700 (rwx------)
	**	everything below this   	  was 0777 (rwxrwxrwx)
	**	hayke02 (24-nov-2004)
	**      everything below this is now 0700 (rwx------)
	**
	** Assemble the full output path piece by piece. Create each
	** subdirectory if need be, but note that we never create
	** the top-most directory, e.g. "/otherplace" .
	**
	** Once fully assembled, check that the full path leads to
	** a writable directory.
	*/

	/* Start with the base path */
	LOtos(bloc, &bpath);
	cpath = bpath;
	flags = 0;

	while ( status == OK )
	{
	    opath = cpath;
	    len = 0;
	    dir_num = 0;

	    /* Parse next subdirectory */
	    while ( status == OK
		    && *(opath+=len) != '\0'
		    && (status = LOreadfname(opath, &len)) == OK )
	    {
		MEcopy(opath, len, &dir);
		dir[len] = EOS;
		
		if ( dir_num++ == 0 && cpath == bpath )
		{
		    /* Top of base path, init full LO path */
		    STcopy(dir, bufptr);
		    LOfroms(PATH, bufptr, loc);
		}
		else
		    /* Add to full path */
		    if ( status = LOfaddpath(loc, dir, loc) )
			break;

		/* Does path exist? */
		flags = LO_I_TYPE;

		if ( status = LOinfo(loc, &flags, &info) )
		{
		    /* Does not exist, create */
		    if ( cpath == bpath && dir_num <= 2 )
			/* Top base directory ("/otherplace") not creatable */
			status = LO_NO_PERM;
		    else if ( (status = LOcreate(loc)) == OK )
		    {
			/* Change to appropriate permissions */
			LOtos(loc, &npath);
			if ( chmod(npath,
				    /* Base path subdirectories */
				    (cpath == bpath) ? 0755
				    /* "what" */
				  : (dir_num == 1)   ? 0700
				    /* Everything below "what" */
				  :                    0700) )
			{
			    status = LOerrno( (i4)errno );
			}
		    }
		}
		/* Exists, but is it a directory? */
		else if ( info.li_type != LO_IS_DIR )
		    status = LO_NO_SUCH;
	    }

	    if ( status == OK )
	    {
		if ( cpath != spath )
		{
		    /* Finished with base path, on to subdirectory path */
		    LOtos(sloc, &spath);
		    cpath = spath;
		}
		else
		{
		    /* Path complete, check that it's writable */
		    flags = LO_I_PERMS;

		    if ( (status = LOinfo(loc, &flags, &info)) == OK &&
			  (info.li_perms & LO_P_WRITE) == 0 )
		    {
			status = LO_NO_PERM;
		    }
		    break;
		}
	    }
	}
    }

    return(status);
}
