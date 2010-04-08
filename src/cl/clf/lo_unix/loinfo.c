/*
**  Copyright (c) 1989, 2001 Ingres Corporation
*/

#include        <compat.h>
#include        <gl.h>
#include	<systypes.h>
#include	<clconfig.h>
#include        "lolocal.h"
#include        <er.h>
#include        <lo.h>
#include        <me.h>
#include        <pm.h>
#include        <st.h>
#include        <diracc.h>
#include        <handy.h>
#include	<errno.h>
#include	<sys/stat.h>
#include	<pwd.h>

/*
** Name: LOinfo()	- get information about a LOCATION
**
** Description:
**
**	Given a pointer to a LOCATION and a pointer to an empty
**	LOINFORMATION struct, LOinfo() fills in the struct with information
**	about the LOCATION.  A bitmask passed to LOinfo() indicates which
**	pieces of information are being requested.  This is so LOinfo()
**	can avoid unneccessary work.  LOinfo() clears any of the bits in
**	the request word if the information was unavailable.  If the
**	LOCATION object doesn't exist, all these bits will be clear.
**
**	The recognized information-request bit flags are:
**		LO_I_TYPE	- what is the location's type?
**		LO_I_PERMS	- give process permissions on the location.
**		LO_I_SIZE	- give size of location.
**		LO_I_LAST	- give time of last modification of location.
**		LO_I_ALL	- give all available information.
**
**	Note that LOinfo only gets information about the location itself,
**	NOT about the LOCATION struct representing the location.  That's
**	why LOisfull, LOdetail, and LOwhat are not subsumed by this
**	interface.  
**
**	This interface makes LOexists, LOisdir, LOsize,
**	and LOlast obsolete, and they will be phased out in the usual way.
**	(meaning: they will hang on for years.)
**
** Inputs:
**	loc		The location to check.
**	flags		A word with bits set that indicate which pieces of
**			 information are being requested.  This can save a
**			 lot of work on some machines...
**
** Outputs:
**	locinfo		The LOINFORMATION struct to fill in.  If any 
**			 information was unavailable, then the appropriate
**			 struct member(s) are set to zero (all integers), 
**			 or zero-ed out SYSTIME struct (li_last).
**			 Information availability is also indicated by the
**			 'flags' word... 
**	flags		The same word as input, but bits are set only for
**			 information that was successfully obtained.
**			 That is, if LO_I_TYPE is set and 
**			 .li_type == LO_IS_DIR, then we know the location is 
**			 a directory.  If LO_I_TYPE is not set, then we 
**			 couldn't find out whether or not the location 
**			 is a directory.
**
** Returns:
**	STATUS - OK if the LOinfo call succeeded and the LOCATION
**		exists.  FAIL if the call failed or the LOCATION doesn't exist.
**
** Exceptions:
**	none.
**
** Side Effects:
**	none.
**
** Example Use:
**	LOCATION loc;
**	LOINFORMATION loinf;
**	i4 flagword;
**
**	/% find out if 'loc' is a writable directory. %/
**	flagword = (LO_I_TYPE | LO_I_PERMS);
**	if (LOinfo(&loc, &flagword, &loinf) == OK)
**	{
**		/% LOinfo call succeeded, so 'loc' exists. %/
**		if ( (flagword & LO_I_TYPE) == 0 
**		  || loinf.li_type != LO_IS_DIR )
**			/% error, we're not sure that 'loc' is a directory. %/
**		if ( (flagword & LO_I_PERMS) == 0 
**		  || (loinfo.li_perms & LO_P_WRITE) != 0 )
**			/% error, 'loc' might not be writable by us. %/
**
**		/% 'loc' is an existing directory, and we can write to it. %/
**	}
**
** History:
**	2/17/89 - First written (billc).
**	23-may-90 (blaise)
**		Integrated changes from ingresug:
**		Add "include <clconfig.h>" and "include <me.h>
** 	6/18/90 (Mike S) - Get permissions for current user, not file owner
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	22-mar-1996 (nick)
**	    Return size only for regular files.
**	23-jun-1998(angusm)
**	    Add LOgetowner().
**	26-aug-1998 (hanch04)
**	    Use cl call for getpwuid
**	14-sep-1998 (walro03)
**	    Include diracc.h and handy.h to get the correct function prototype
**	    for iiCLgetpwuid.
**	28-feb-2000 (somsa01)
**	    Use stat64() when LARGEFILE64 is defined.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	09-sep-2000 (somsa01)
**	    For Embedded Ingres, in LOgetowner(), force the owner to be
**	    'ingres'.
**	22-sep-2000 (somsa01)
**	    Amended previous change with proper multiple status check.
**	11-May-2001 (jenjo02)
**	    Return appropriate error if stat() fails rather than just
**	    "FAIL" so that LO users can distinguish between "does not exist"
**	    and problems with permissions.
**	30-mar-2006 (toumi01) BUG 115913
**	    Introduce for LOinfo a new query type of LO_I_XTYPE that can,
**	    without breaking the ancient behavior of LO_I_TYPE, allow for
**	    query of more exact file attributes: LO_IS_LNK, LO_IS_CHR,
**	    LO_IS_BLK, LO_IS_FIFO, LO_IS_SOCK. Default to LO_IS_UNKNOWN
**	    so as to avoid fall-through logic.
*/

/* Permissions masks */
typedef struct
{
	unsigned int rd_mask;
	unsigned int wr_mask;
	unsigned int ex_mask;
} LOP_MASK_TYPE;

# define U_IS_OWNER 0
# define U_IS_GROUP 1
# define U_IS_WORLD 2

static LOP_MASK_TYPE lop_masks[] = 
{
  {S_IRUSR, S_IWUSR, S_IXUSR},
  {S_IRGRP, S_IWGRP, S_IXGRP},
  {S_IROTH, S_IWOTH, S_IXOTH}
};

static i4 embed_installation = -1;

STATUS
LOinfo(loc, flags, locinfo)
	LOCATION *loc;
	i4 *flags;
	LOINFORMATION *locinfo;
{
#ifdef LARGEFILE64
	struct stat64	statinfo;
#else /* LARGEFILE64 */
	struct stat	statinfo;
#endif /* LARGEFILE64 */
	i4 statstatus;

	MEfill(sizeof(*locinfo), '\0', (PTR) locinfo);

	if ((*flags & LO_I_LSTAT) != 0)
	{
#ifdef LARGEFILE64
	    statstatus = lstat64( loc->string, &statinfo );
#else /* LARGEFILE64 */
	    statstatus = lstat( loc->string, &statinfo );
#endif /* LARGEFILE64 */
	}
	else
	{
#ifdef LARGEFILE64
	    statstatus = stat64( loc->string, &statinfo );
#else /* LARGEFILE64 */
	    statstatus = stat( loc->string, &statinfo );
#endif /* LARGEFILE64 */
	}

	if (statstatus < 0)
	{
		/* loc doesn't exist (or the call failed) */
		*flags = 0;
		return ( LOerrno((i4)errno) );
	}

	if ((*flags & LO_I_SIZE) != 0)
	{
		if ((statinfo.st_mode & S_IFMT) == S_IFREG)
		    locinfo->li_size = (OFFSET_TYPE) statinfo.st_size;
		else
		    locinfo->li_size = 0;
	}
		
	if ((*flags & LO_I_TYPE) != 0)
	{
		/* 
		** We should be handling many different kinds of
		** objects here.  This is definitely a punt.
		*/
		if ((statinfo.st_mode & S_IFDIR) != 0)
			locinfo->li_type = LO_IS_DIR;
		else
			locinfo->li_type = LO_IS_FILE;
	}
		
	if ((*flags & LO_I_XTYPE) != 0)
	{
		/* 
		** The extended info type query overrides the old
		** version, so process the new version last.
		*/
		if (S_ISDIR(statinfo.st_mode))
			locinfo->li_type = LO_IS_DIR;
		else
		if (S_ISLNK(statinfo.st_mode))
			locinfo->li_type = LO_IS_LNK;
		else
		if (S_ISREG(statinfo.st_mode))
			locinfo->li_type = LO_IS_FILE;
		else
		if (S_ISBLK(statinfo.st_mode))
			locinfo->li_type = LO_IS_BLK;
		else
		if (S_ISCHR(statinfo.st_mode))
			locinfo->li_type = LO_IS_CHR;
		else
		if (S_ISFIFO(statinfo.st_mode))
			locinfo->li_type = LO_IS_FIFO;
		else
		if (S_ISSOCK(statinfo.st_mode))
			locinfo->li_type = LO_IS_SOCK;
		else
			locinfo->li_type = LO_IS_UNKNOWN;
	}

	if ((*flags & LO_I_PERMS) != 0)
	{
		int utype;

		if (geteuid() == statinfo.st_uid)
			utype = U_IS_OWNER;
		else if (getegid() == statinfo.st_gid)
			utype = U_IS_GROUP;
		else
			utype = U_IS_WORLD;

		if ((statinfo.st_mode & lop_masks[utype].rd_mask) != 0)
			locinfo->li_perms |= LO_P_READ;
		if ((statinfo.st_mode & lop_masks[utype].wr_mask) != 0)
			locinfo->li_perms |= LO_P_WRITE;
		if ((statinfo.st_mode & lop_masks[utype].ex_mask) != 0)
			locinfo->li_perms |= LO_P_EXECUTE;
	}
		
	if ((*flags & LO_I_LAST) != 0)
	{
		locinfo->li_last.TM_secs = statinfo.st_mtime;
		locinfo->li_last.TM_msecs = 0;
	}

	return ( OK ); 
}

/*
**
** Name: LOgetowner - get owner of given file 
**
** Description:
**	Get owner of given file
**
** Inputs:
**	loc	-	ptr to location
**	buf	-	buffer
**
** Outputs:
**	buf	-	buffer: name or NULL
**
** Returns:
**	FAIL	-	failure to access supplied file
**	OK	-	success
**
** Exceptions:
**	none.
**
** Side Effects:
**	none.
**
** History:
**	12-august-1997(angusm)
**	Initial version.
**	26-aug-1998 (hanch04)
**	    Use cl call for getpwuid
**	09-sep-2000 (somsa01)
**	    For Embedded Ingres, force the owner to be 'ingres'.
*/
STATUS
LOgetowner(loc, name)
LOCATION *loc;
char	**name;
{
#ifdef LARGEFILE64
    struct stat64	statinfo;
#else /* LARGEFILE64 */
    struct stat		statinfo;
#endif /* LARGEFILE64 */
    struct passwd	*p_pswd, pwd;
    char                pwuid_buf[BUFSIZ];
    int                 size = BUFSIZ;

    if (embed_installation == -1)
    {
	char	config_string[256];
	char	*value, *host;
	STATUS	status;

	embed_installation = 0;

	/*
	** Get the host name, formally used iipmhost.
	*/
	host = PMhost();

	/*
	** Build the string we will search for in the config.dat file.
	*/
	STprintf( config_string,
		  ERx("%s.%s.setup.embed_user"),
		  SystemCfgPrefix, host );

	status = PMinit();
	if (status == OK || status == PM_NO_INIT)
	{
	    if (PMget (config_string, &value) == OK)
		if ((value[0] != '\0') &&
		    (STbcompare(value,0,"on",0,TRUE) == 0) )
		    embed_installation = 1;
	}
    }

    if (embed_installation)
	STcopy("ingres", *name);
    else
    {
#ifdef LARGEFILE64
	if( stat64( loc->string, &statinfo ) < 0 )
#else /* LARGEFILE64 */
	if( stat( loc->string, &statinfo ) < 0 )
#endif /* LARGEFILE64 */
	    return ( LOerrno((i4)errno) );

	p_pswd = iiCLgetpwuid(statinfo.st_uid, &pwd, pwuid_buf, size);

	if (p_pswd == NULL)
	    return(FAIL);

	STcopy(p_pswd->pw_name, *name);
    }

    return(OK);
}
