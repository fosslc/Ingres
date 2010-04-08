/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<lo.h>
#include	<pe.h>
#include	<st.h>
#include	<nm.h>
#ifndef LO_NO_PERM
#define LO_NO_PERM	(E_CL_MASK + E_LO_MASK + 15)
#endif
#include	<si.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	<adf.h> 
#include	<abfcnsts.h> 
#include	<abclass.h>
#include	<abfglobs.h>
#include	"erab.h"

/**
** Name:	abfdir.c -	ABF Work Directories Module.
**
** Description:
**	Contains routines that manipulate the ABF work directories for a
**	database and for its applications.  Defines:
**
**	IIABidirInit()	    initialize the work directory module.
**	IIABsdirSet()	    set the work directory for an application.
**	IIABcdirCreate()    create the work directory for an application.
**	IIABddirDestroy()   destroy the work directory for an application.
**	IIABrdirRename()    rename the work directory for an application.
**	IIABfdirFile()	    set location for file in current work directory.
**
** History:
**	13-jul-93 (connie) Bug #49700
**	   lower case Dbname before building application object code directory 
**	15-Dec-92 (fredb)
**	    Fix coding error in MPE (hp9_mpe) specific code in IIABfdirFile.
**	    Update MPE/iX comments to some extent.
**	15-nov-91 (leighb) DeskTop Porting Change: Interpreted Images
**	    Added global character string 'IIabNamWrk' to hold name pointed
**	    to by global LOCATION 'IIabWrkDir'.
**	18-jan-91 (blaise)
**	    Removed IIABrdirRename(). This function has been replaced by
**	    calls to IIABcdirCreate() and IIABddirDestroy() in IIABrappRename()
**	    (in abcatlog.qsc). Bug #34960.
**
**	Revision 6.3/03/00  90/07  wong
**	Pass location to be set to 'IIABsdirSet()' and 'IIABcdirCreate()' and
**	move call to 'IIABlnxExtFile()' to 'IIABappFetch()' along with former
**	'IIABsdirSet()' error reporting.
**
**      05-Dec-89 (mrelac)
**              NOTES ON MPE/iX (hp9_mpe): [Updated 15-Dec-92 fredb]
**              MPE/iX has a 2-level directory structure consisting of the
**              ACCOUNT (top level) and the GROUP (bottom level).  Beneath
**              this ACCOUNT and GROUP structure lie the files.  Files are
**              not permitted to exist at the account or group level.  Instead,
**              files MUST reside beneath the group, which resides beneath the
**              account.  The period is the delimiter between the file, group
**              and account.  Note that the filename is specified first,
**              followed by the group name, then followed by the account name.
**              Given the path and filename 'file.group.account', here are some
**              examples:
**                      valid path examples:
**                      account   account.group   group
**
**                      valid filename examples:
**                      file      file.group      file.group.account
**
**                      invalid filename examples:
**                      file.account   group.account
**
**              The concept of file extensions as part of the filename does
**              not exist in MPE/iX.  In order to simulate file extensions,
**              which are used throughout INGRES and ABF, we must implement
**              the extension as a group.  Accounts and groups in MPE/iX
**              require special commands and privileges to manipulate them.
**
**              We must use the account name as the name for all of the ABF
**              directory pointers.
**
**              Because of all this, we require the user/ ingres/ABF system
**              administrator to create the required directory structure
**              (including ALL the groups - i.e. file extensions) before
**              creating an application.  As a result of this requirement, we
**              can omit the object directory creation/deletion code.
**              It's not pretty - it's MPE/iX.
**
**	Revision 6.2  89/03  billc
**	Major enhancements to use 'LOinfo()'.
**
**	Revision 6.0  88/02  wong
**	Changed names.  Changed to do more appropriate error checking in
**	'IIABidirInit()'; removed create functionality from 'IIABsdirSet()'.
**	Moved extract file initialization to "ablink.c" as 'IIABlnxExtFile()'.
**
**	Revision 2.0  82/07/19  joe
**	Initial revision.
**
**  History:
**	15-may-97 (mcgem01)
**	    Clean up compiler warnings.
**	23-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**	
*/

/* ABF work directory */

GLOBALDEF LOCATION	IIabWrkDir ZERO_FILL;
GLOBALDEF char		IIabNamWrk[MAX_LOC+1] = {EOS};		 
static char		abfWrkdir[MAX_LOC+1] = {EOS};

static char	*baseDirName = NULL;

typedef struct _d_info
{
	LOCATION	di_loc;		/* the location representation */
	char 		di_lbuf[MAX_LOC + 1]; /* the location buffer */
	LOINFORMATION	di_info;	/* information about di_loc */
	i4		di_liflags;	/* the flags to pass to LOinfo */
	bool		di_exists;	/* does the thing exist? */
} D_INFO;

/* This contains the <ING_ABFDIR>/<dbname> directory. */ 
static D_INFO DbOInfo ZERO_FILL;

static D_INFO wrkInfo ZERO_FILL;
/* This contains the source directory.  */
static D_INFO SrcInfo ZERO_FILL;

/*
** Stripped database name without <NODE>:: and "/d", if any.
*/
static char	Dbname[MAX_LOC+1] = {EOS};

static STATUS	_dir_info();
static char 	*_loctos();

/*{
** Name:	IIABidirInit() -     Initialize the ABF Work Directory Module.
**
** Description:
**	Set the location of the ABF work directory for the database, and return
**	status on whether it exists.  This is set from the ABF base work
**	directory path, which should be specified in the environment logical,
**	"ING_ABFDIR", and the database name.  Make sure that the base path
**	exists, is a directory, and if the database directory does not exist,
**	is writable.
**
** Input:
**	dbname	{char *}  The name of the database.
**
** Called by:
**	'IIabf()', 'abfimage()'.
**
** Side Effects:
**	Saves the database work directory in 'DbOInfo'.
**
**	Exits on error.
**
** Error Messages (FATAL):
**	ABINGDIR	environment logical "ING_ABFDIR" is not set.
**	ABINGEXIST	base work directory does not exist.
**	ABINGNODIR	base work directory is not a directory.
**	ABINGWRT	base or DB work directory is not writable.
**
** History:
**	Written 7/19/82 (jrc)
**	15-jul-1986 (Joe)  BUG 6955
**		Use LOfaddpath instead of LOaddpath.
**	03-mar-1987 (marian)	Bug 9519
**		Use the variable 'BaseObjdir' as argument for the error message.
**		'cp' was being used, but 'cp' gets changed down in nmgetenv().
**	19-mar-87 (mgw)	Bug 11361
**		Use LOcreate() to see if the ING_ABFDIR directory is writable, 
**		not SIopen(), since SIopen() tends to create files with the 
**		same protection as the parent directory on VMS and LOcreate() 
**		uses the users ID, also because LOcreate() is the only 
**		mechanism used to create directories there.
**	03-jun-87 (marian) 
**		Change references to the constant 256 to be MAX_LOC.
**	04-jun-87 (mgw)
**		Corrected fix for 11361 by only checking the ability
**		to create a file in the work directory after it's
**		been successfully created.
**	02/88 (jhw) -- Moved DB name node truncation, here, with DB name now
**		passed in.  Changed to check for DB work directory 
**		existence before checking base work writability.  
**		Abstracted writability check into 'chk_write()'.
**      21-Nov-89 (mrelac) (hp9_mpe)
**              Omitted call to add Dbname to path.  This causes DbOInfo.di_loc
**              to point to the account name.
*/
VOID
IIABidirInit (dbname)
char	*dbname;
{
	char		node_name[MAX_LOC+1];
	D_INFO		baseDir;

	/* base ABF work directory */
	NMgtAt(ABFDIR, &baseDirName);
	if ( baseDirName == NULL || *baseDirName == EOS )
	{ 
		/* It is now an error for ING_ABFDIR to be undefined. */
		IIUGerr(ABBASEPATH, UG_ERR_ERROR, 0);
		abexcexit(FAIL);	/*NOTREACHED*/
		/*NOTREACHED*/
	}

	baseDir.di_exists = FALSE;
	/* Check base ABF work directory */
	_VOID_ STlcopy(baseDirName, baseDir.di_lbuf, sizeof(baseDir.di_lbuf)-1);
	if ( LOfroms(PATH, baseDir.di_lbuf, &baseDir.di_loc) != OK
			|| _dir_info(&baseDir, LO_I_PERMS) != OK )
	{
		IIUGerr(ABBASEDIR, UG_ERR_ERROR, 1, baseDirName);
		abexcexit(FAIL);	
		/*NOTREACHED*/
	}

	/* Get DB Name and Add to Path */
	FEnetname(dbname, node_name, Dbname);
	CVlower(Dbname);
	LOcopy(&baseDir.di_loc, DbOInfo.di_lbuf, &DbOInfo.di_loc);
#ifndef hp9_mpe
	_VOID_ LOfaddpath(&DbOInfo.di_loc, Dbname, &DbOInfo.di_loc);
#endif

	/* Check that the base work directory is writable 
	** only if the DB work directory does not exist, 
	** since only then might it need to be created.
	*/
	if (_dir_info(&DbOInfo, 0) != OK
	  && (baseDir.di_info.li_perms & LO_P_WRITE) == 0)
	{
		IIUGerr(ABBASEWRT, UG_ERR_ERROR, 1, _loctos(&baseDir));
		abexcexit(FAIL);	/*NOTREACHED*/
	}

	_VOID_ LOfroms(PATH, abfWrkdir, &IIabWrkDir);

	/* set up the source directory with empty name. */
	_VOID_ LOfroms(PATH, SrcInfo.di_lbuf, &SrcInfo.di_loc);
}

/*{
** Name:	IIABsdirSet() -	Set the Work Directory for an Application.
**
** Description:
**	Sets the pathname of the ABF work directory for the application
**	if it exists.
**
** Input:
**	dirname	{char *}  The name of the work directory in the DB work
**			 directory (usually the application name.)
** Output:
**	dir	{LOCATION *}  The work directory location.
**
** Returns:
**	{STATUS}  OK, if the directory exists.
**		  FAIL, if the directory does not exist or is inaccessible.
**		  LO_NO_PERM, if the directory is not writeable.
**
** Called by:
**	'IIABappFetch()'.
**
** History:
**	Written 7/19/82 (jrc)
**	02/88 (jhw) -- Changed to no longer create, but to return status.
**		Abstracted writability check into 'chk_write()'.
**      21-Nov-89 (mrelac) (hp9_mpe)
**              Omitted call to add objdir to path.  This causes ObjInfo.di_loc
**              to point to the account name.
**	07/90 (jhw)  Error reporting moved to 'IIABappFetch()' in "abcatget.c".
*/
STATUS
IIABsdirSet ( dirname, dir )
char		*dirname;
LOCATION	*dir;
{
	char	*cp;
	D_INFO	wrkInfo;

	/* 'IIABidirInit()' already checked that DbOInfo exists
	** (and even though it might have disappeared since then) 
	** just check the application work directory.
	*/

	/* Make sure the cache is invalidated.. */
	wrkInfo.di_exists = FALSE;

	/* make location for the work directory */
	LOcopy(&DbOInfo.di_loc, wrkInfo.di_lbuf, &wrkInfo.di_loc);
# ifdef hp9_mpe
	if ( _dir_info(&wrkInfo, LO_I_PERMS) != OK )
# else
	if ( LOfaddpath(&wrkInfo.di_loc, dirname, &wrkInfo.di_loc) != OK
			||  _dir_info(&wrkInfo, LO_I_PERMS) != OK )
# endif
	{ /* work directory for the application does not exist */
		LOtos(dir, &cp);
		LOcopy(&DbOInfo.di_loc, cp, dir);
		return FAIL;
	}

	LOtos(dir, &cp);
	LOcopy(&wrkInfo.di_loc, cp, dir);

	/* Check that the work directory is writable */
	if ((wrkInfo.di_info.li_perms & LO_P_WRITE) == 0)
	{
		return LO_NO_PERM;
	}

	return OK;
}

/*{
** Name:	IIABcdirCreate() -	Create the Application Work Directory.
**
** Description:
**	Creates the application work directory relative to the base work
**	directory for the database including the DB work directory if it
**	does not exist.
**
** Input:
**	dirname	{char *}  The name of the work directory
**					  (usually the application name.)
** Output:
**	dir	{LOCATION *}  The work directory location.
**
** Returns:
**	{STATUS}  OK, directory was successfully created
**		  FAIL, directory already exists or could not be created.
**
** Called by:
**	'IIABeditApp()'
**
** Side Effects:
**	Creates the application work directory, and may create the DB
**	work directory, if none existed before.
**
** Error Messages (FATAL):
**	DIRCREATE - unsuccessful LOcreate
**	E_LibExists - disallow create since directory already exists
**	ABOBJWRT - cannot create directories in DB work directory
**
** History:
**	9-11-85 (mgw) Bug # 5340
**		If work directory already exists, don't destroy it,
**		there might be important files there. Just don't try
**		to LOcreate it.
**	15-jul-1986 (Joe) BUG 6955
**		Change LOaddpath to LOfaddpath.
**	10-dec-1986 (marian) bug 6946
**		Make a call to PEworld() to set permissions for world
**		access.	 This call needs to be made since the PEumask()
**		is ignored by LOmkdir().
**      21-Nov-89 (mrelac) (hp9_mpe)
**              Omitted call to add objdir to path.  This causes ObjInfo.di_loc
**              to point to the account name.
**      05-Dec-89 (mrelac) (hp9_mpe)
**              Ifdef'd out the ObjInfo directory checking/building code.
*/
STATUS
IIABcdirCreate ( dirname, dir )
char		*dirname;
LOCATION	*dir;
{
	char	*cp;
	STATUS	rstatus;
	D_INFO	wrkInfo;

	/*
	** BUG 4687
	**
	** Give world write permission to the work directory
	** for the entire database, so that other users can later
	** create applications.
	**
	** Using PEumask() so that both directories are created
	** with world permission.
	*/

	PEsave();
	PEumask(ERx("rwxrwx"));

	if ( _dir_info(&DbOInfo, 0) != OK )
	{ /* create base DB work directory */
		if (LOcreate(&DbOInfo.di_loc) != OK)
		{
			/* Note:  This error unlikely since we already checked
			** writability of base directory in 'IIABidirInit()'.
			*/
			IIUGerr(DIRCREATE, UG_ERR_ERROR, 2, baseDirName,Dbname);
			PEreset();
			return FAIL;
		}

		/* bug 6946
		**	Call 'PEworld()' to set world permission on the
		**	directory.  The 'PEumask()' setting above does not
		**	set the permissions because 'LOmkdir()' ignores them.
		*/

		PEworld(ERx("+r+w+x"), &DbOInfo.di_loc);
	}

# ifdef CMS
	rstatus = OK;
# else
#   ifdef hp9_mpe
	LOcopy(&DbOInfo.di_loc, wrkInfo.di_lbuf, &wrkInfo.di_loc);
	rstatus = OK;
#  else
	/* Make sure the cache is invalidated.. */
	wrkInfo.di_exists = FALSE;

	LOcopy(&DbOInfo.di_loc, wrkInfo.di_lbuf, &wrkInfo.di_loc);
	if ( LOfaddpath(&wrkInfo.di_loc, dirname, &wrkInfo.di_loc) != OK
			|| _dir_info(&wrkInfo, LO_I_PERMS) == OK )
	{ /* it already exists, can't create */
		/*
		** BUG 5340
		** Don't destroy it, the user could have important files there.
		** Just don't try to create it.
		*/
		IIUGerr(E_LibExists, UG_ERR_ERROR, 1, _loctos(&wrkInfo));
		rstatus = FAIL;
	}
	else
	{ /* it doesn't exist, so create it */
		if (LOcreate(&wrkInfo.di_loc) != OK)
		{
			IIUGerr( DIRCREATE, UG_ERR_ERROR,
					2, _loctos(&DbOInfo), dirname
			);
			rstatus = FAIL;
		}
		else
		{
			/* bug 6946
			**	Call 'PEworld()' to set world permission on the
			**	directory.  The 'PEumask()' setting above does
			**	not set the permissions because 'LOmkdir()'
			**	ignores them.
			*/

			PEworld(ERx("+r+w+x"), &wrkInfo.di_loc);
			rstatus = OK;
		}
	}
#  endif  /* hp9_mpe */
# endif /* CMS */
	PEreset();

	LOtos(dir, &cp);
	LOcopy(&wrkInfo.di_loc, cp, dir);

	return rstatus;
}

/*{
** Name:	IIABchkSrcDir()	-	Check Existence/Writeablity of
**						Source-Code Directory.
** Description:
**	Determines whether the source-code directory exists, and optionally,
**	whether it is writeable.
**
** Input:
**	srcdir	{char *}  The name of the application source-code directory.
**	chkwrite {bool}  Whether to check if the directory is writable.
**
** Returns:
**	{bool}	TRUE	if the directory exists and is writeable
**		FALSE	if the directory does not exist or is not writeable
**
** Called by:
**	'IIABeditApp()'
**
** History:
**	02-mar-87 (marian)	
**		Written to fix bugs 8254, 9986 and 10346.
**	21-mar-87 (mgw)
**		Corrected and renamed srcloc references to make it
**		run on Unix.
**	03-jun-87 (marian)
**		Added check 'dirchk != ISDIR' after call to LOisdir
**		to make sure dirchk is a directory.
**	31-oct-1988 (jhw)  Made writeability check optional.
**	07-dec-1989 (wolf)  Uppercase source-code directory on CMS.
*/

bool
IIABchkSrcDir( char *srcdir, bool chkwrite )
{
	/* bug 8254, 10346, 9986
	**	Verify that the source-code directory is good before
	**	defining frames or procedures.  Also check to see if
	**	the source code directory is writeable.
	*/
	if ( srcdir == NULL || *srcdir == EOS )
	{
		IIUGerr(E_AB0049_NoDir,0,0);
		return FALSE;
	}

	/* If we've already checked this dir, just say yes. */
	if ( SrcInfo.di_exists
		    && STbcompare(srcdir, 0, _loctos(&SrcInfo), 0, TRUE) == 0 )
	{
		if ( !chkwrite || (SrcInfo.di_info.li_perms & LO_P_WRITE) != 0 )
		{
			return TRUE;
		}
	}
	else
	{ /* new src directory.  make the location */
		/* Make sure the cache is invalidated.. */
		SrcInfo.di_exists = FALSE;

		_VOID_ STlcopy(srcdir, SrcInfo.di_lbuf, sizeof(SrcInfo.di_lbuf)-1);
#ifdef CMS
		CVupper(SrcInfo.di_lbuf);
#endif
		if ( LOfroms(PATH, SrcInfo.di_lbuf, &SrcInfo.di_loc) != OK
				|| _dir_info(&SrcInfo, LO_I_PERMS) != OK )
		{
			IIUGerr(E_AB004A_BadDir, 0, 1, srcdir);
			return FALSE;
		}
	}

	if ( chkwrite )
	{
		if ( (SrcInfo.di_info.li_perms & LO_P_WRITE ) == 0 )
		{
			IIUGerr(E_AB004B_NoWrtDir, 0, 1, srcdir);
			return FALSE;
		} 
	}

	return TRUE;
}


/*{
** Name:	IIABddirDestroy() -	Destroy the ABF Work Directory.
**
** Description:
**	Destroy the work directory for an application.
**	When done the directory will be ABFDIR.
**
** Input:
**	appname	{char *}  The name of the application.
**
** Called by:
**	'IIABcatalog()'.
**
** Side Effects:
**	Removes the work directory, and hence, all the object-code files for
**	the application.
**
** History:
**	Written 9/24/82 (jrc)
**	01/89 (jhw) -- Changed name from 'abdirdestroy()' and now builds
**		location from DB ABF directory and application name.
**	10/89 (wolf) -- Ignore 'LOdelete()' return code on CMS; work directory
**			is a minidisk.
**      21-Nov-89 (mrelac) (hp9_mpe)
**              For MPE/XL, we do not destroy anything, since the object
**              code directory can, and probably does, contain files from
**              other applications.
*/
VOID
IIABddirDestroy ( dirname )
char	*dirname;
{
	LOCATION	dir;
	char		dirbuf[MAX_LOC+1];

# ifndef hp9_mpe  
	/* make location for work directory */
	LOcopy(&DbOInfo.di_loc, dirbuf, &dir);
	if ( LOfaddpath(&dir, dirname, &dir) != OK || LOdelete(&dir) != OK )
	{
#  ifndef CMS
		char	*cp;

		LOtos(&dir, &cp);
		IIUGerr(E_NoLibRm, UG_ERR_ERROR, 1, cp);
#  endif /* CMS */
	}
# endif /* hp9_mpe */
}


/*{
** Name:	IIABfdirFile() -	Set Location for File in
**						Work Directory.
** Input:
**	name	{char *}  The file name, including extension.
**	lbuf	{char []}  The location buffer.
**	loc	{LOCATION *}  A reference to the location to be set.
**
** Output:
**	loc	{LOCATION *}  The location containing the path and file name
**				for a file in the work directory.
**
** Returns
**	{STATUS}  'LOfstfile()'
**
** History:
**	02/88 (jhw) -- Written.
**      11-Dec-89 (mrelac)
**              Modified for MPE/XL (hp9_mpe).  The algorithm for this function
**              doesn't work for MPE/XL; trying to LOfstfile() a location with
**              a NULL group (which is what this code produces) causes the
**              resulting path & filename to be 'mashed' by a special filter.
**	15-Dec-92 (fredb)
**		Somewhere along the way, the variable "IIabObjdir" disappeared
**		and was replaced in this routine by "IIabWrkDir".  The old
**		variable was a C string, the new one an Ingres LOCATION; not a
**		direct replacement!  I added the call to LOtos assuming that
**		the "IIabWrkDir" location will never contain anything more than
**		the account name.

*/
STATUS
IIABfdirFile (name, lbuf, loc)
char		*name;
char		lbuf[MAX_LOC+1];
LOCATION	*loc;
{
	char	*temp_p;

# ifdef hp9_mpe
	LOtos (&IIabWrkDir, &temp_p);
        STpolycat (3, name, ".", temp_p, lbuf);
        return (LOfroms (PATH & FILENAME, lbuf, loc) );
# else
	LOcopy(&IIabWrkDir, lbuf, loc);
	return LOfstfile(name, loc);
# endif
}

/*
** Name:	_loctos() -	Get string from location.
**
**	WARNING: this routine assumes that LOtos will return a reasonably
**		printable string.  Therefore, don't call this routine unless
**		the LOCATION part of the D_INFO is legitimately constructed.
**		(The directory described by LOCATION doesn't have to exist.)
**
** Input:
**	loc	{LOCATION *}  A reference to the location to stringify
**
** Returns:
**	{char *}	The string.
**
** History:
**	11/88 (billc) -- Written.
*/
static char *
_loctos(dir)
D_INFO *dir;
{
	char *cp;

	LOtos(&dir->di_loc, &cp);
	return cp;
}

/*
** Name:	_dir_info() - Centralize the calls to LOinfo.
**
**	This routine looks up information about a directory.  It returns
**	FAIL if the directory doesn't exist, or if it isn't a directory,
**	or if LOinfo failed to get the information requested by the 'flags'
**	argument.
**
**	_dir_info() tries to cache information in the LOINFORMATION struct 
**	inside the D_INFO struct passed to it.  The caching can be disabled
**	by setting dir->di_exists = FALSE before the call.  This is done in
**	several places, since the user can change directories.
**
** Input:
**	dir	{D_INFO *}  	Struct describing the directory.
**	flags	{nat}		Information requests, in bitflag form.
**
** Returns:
**	{STATUS}	OK if dir exists, is a directory, and we got the
**			requested info.  FAIL otherwise.
**
** History:
**	3/89 (billc) -- Written.
*/

static STATUS
_dir_info(dir, flags)
D_INFO *dir;
i4  flags;
{
	/* 
	** Caching.  If the flags are the same and the info exists, 
	** the D_INFO struct already contains the info.  (Caller can 
	** invalidate the cache by setting di_exists to FALSE.)
	*/
	flags |= LO_I_TYPE;

	if ( dir->di_exists == TRUE
	  && (flags == dir->di_liflags || flags == LO_I_TYPE) )
	{
		return OK;
	}

	dir->di_exists = FALSE;
	dir->di_liflags = flags;

    	if ( LOinfo(&(dir->di_loc), &(dir->di_liflags), &(dir->di_info)) != OK )
    	{ /* the thing doesn't exist (or the call failed). */
		return FAIL;
    	}

	if ((dir->di_liflags & flags) != flags
	  || dir->di_info.li_type != LO_IS_DIR)
	{
		/* 
		** the thing wasn't a directory, or we couldn't get the info.
		*/
		return FAIL;
	}

	dir->di_exists = TRUE;
	return OK;
}
