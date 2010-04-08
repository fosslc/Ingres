# include	<compat.h>
# include	<gl.h>
# include	<st.h>
# include	<me.h>
# include	<lo.h>
# include	<er.h>
# include	<fabdef.h>
# include	<xabprodef.h>
# include	<starlet.h>
# include	"lolocal.h"

/*	LOingpath
**
**	Concatenate area, dbname and what (data, ckp jnl, or dmp) to
**		form the fullpath location of the database (data, ckp,
**		jnl, or dmp) specified.
**
**
**	On VMS this routine will insure that newly created location
**		is syntactically legal. The concatentation will take the
**		form area:[ingres.what.dbname]
**
**	Warning
**		The associated location buffer is static. Its contents should
**		be copied upon return
**
**	History
**		VMS CL	9-27-83 -- (dd)
**		10/89 (Mike S)	Don't assume STcopy returns a value
**      16-jul-93 (ed)
**	    added gl.h
**	28-Nov-01 (kinte01)
**	    Added additional handling due to RAW location support of
**	    data locations on Unix that are not supported on VMS
**	22-nov-2002 (abbjo03)
**	    Temporarily add a dummy LOmkingpath.
**	03-dec-2003 (abbjo03)
**	    Implement LOmkingpath.
*/

/* protection for [INGRES...] directories */

#define PROT_SRWE_ORWE_G_WRE	0xAF88


static STATUS setperms(LOCATION *loc, u_i2 perms);


STATUS
LOingpath(
char		*area,		/* "device" that the database is on */
char		*dbname,	/* ingres data base name */
char		*what,		/* path to db, ckp, jnl, or sort */
LOCATION	*fullpath)	/* set to full path of database */
{
	static	char	buffer[255];
	register char       *dbptr = dbname;
 	char 	*strptr = buffer;
	SIZE_TYPE try_raw = FALSE;
	STATUS	ret_val = OK;

	/* Only check for RAW if requested by dbname==LO_RAW */
	if ( (try_raw = (SIZE_TYPE)dbptr && STequal( dbptr, LO_RAW)) )
            dbptr = NULL;
	


	/* check for legal arguments */

	if ((area == NULL) || (*area == NULL) || 
             (what == NULL) || (*what == NULL))
		ret_val = LO_NULL_ARG;
	/* set up location */
	fullpath->string = buffer;

	/* create location */
	STcopy(area, strptr);
        strptr += STlength(strptr);

	/* add colon if not there */
	if(*(strptr - 1) != *COLON)
		*strptr++ = *COLON;

	STcopy("[INGRES.", strptr);
        strptr += STlength(strptr);

	/* add 'what' to path */
	STcopy(what, strptr);
        strptr += STlength(strptr);

	/*
        ** Raw locations are identified by the existence of the
        ** file "iirawdevice" (LO_RAW).
        */
        if ( try_raw )
               ret_val = OK;

	if ( !ret_val )
	{
	   /* if dbname exists then insert it */
	   if(dbptr != NULL && *dbptr != EOS)
		STpolycat(3, DOT, dbname, CBRK, strptr);
	   else		
	   {
		*strptr++ = *CBRK;	
		*strptr = NULL;
	   }
	   LOfroms(PATH, fullpath->string, fullpath);
	}
	return(ret_val);

}

/*
** Name: LOmkingpath	- make INGRES location's area path
**
** Description:
**	Makes missing subdirectories in a location's area path.
**
** Inputs:
**	area	"device" that the location is on
**	what	path to db, ckp, dmp, work, jnl
**	loc	set to full path of location
**	
** Outputs:
**	none
**
** Returns:
**	LO_NULL_ARG	area or what were NULL or empty
**	LO_NO_SUCH	required path component does not exist
**
** History:
**	03-dec-2003 (abbjo03)
**	    Created.
*/
STATUS
LOmkingpath(
char		*area,		/* "device" that the database is on */
char		*what,		/* path to db, ckp, dmp, work, or jnl */
LOCATION	*loc)		/* set to full path of location */
{
    STATUS	status;
    char	path[MAX_LOC+1];
    i4		flags;
    LOINFORMATION	info;

    /* check for legal arguments */
    if (area == NULL || *area == EOS || STtrmwhite(area) == 0 ||
		what == NULL || *what == EOS)
	return LO_NULL_ARG;

    LOdev_to_root(area, path);
    if ((status = LOfroms(PATH, path, loc)) != OK)
	return status;
    flags = LO_I_TYPE;
    if (LOinfo(loc, &flags, &info))
	return LO_NO_SUCH;
    if ((flags & LO_I_TYPE) == 0 || info.li_type != LO_IS_DIR)
	return LO_NO_SUCH;

    LOfaddpath(loc, SystemLocationSubdirectory, loc);
    if ((status = LOfroms(PATH, path, loc)) != OK)
	return status;
    flags = LO_I_TYPE;
    if (LOinfo(loc, &flags, &info))
    {
	if ((status = LOcreate(loc)) != OK)
	    return status;
    }
    /* Set protections */
    if (setperms(loc, PROT_SRWE_ORWE_G_WRE) != OK)
	return FAIL;
	
    LOfaddpath(loc, what, loc);
    if ((status = LOfroms(PATH, path, loc)) != OK)
	return status;
    flags = LO_I_TYPE;
    if (LOinfo(loc, &flags, &info))
    {
	if ((status = LOcreate(loc)) != OK)
	    return status;
    }
    if (setperms(loc, PROT_SRWE_ORWE_G_WRE) != OK)
	return FAIL;

    flags = LO_I_PERMS;
    if ((status = LOinfo(loc, &flags, &info)) == OK &&
		(info.li_perms & LO_P_WRITE) == 0)
	status = LO_NO_PERM;

    return status;
}

/*
** Name: setperms - set permissions on a directory
**
** Description:
**      Sets specific permissions on a directory.
**
** Inputs:
**      loc	pointer to LOCATION describing directory
**      perms   VMS-style permissions mask.
**
** Outputs:
**	none
**
** History:
**      03-dec-2003 (abbjo03)
**          Created.
*/
static STATUS
setperms(
LOCATION	*loc,
u_i2		perms)
{
    FABDEF	fab;
    XABPRODEF1	xab;
    char	dirfile[MAX_LOC];
    char	*path;

    LOtos(loc, &path);
    /* Convert directory format to file format, if need be */
    if ((loc->desc | FILENAME) != FILENAME)
	LOdir_to_file(path, dirfile);

    /* Initialize the RMS fab */
    MEfill(sizeof(fab), 0, (PTR)&fab);
    fab.fab$b_bid = FAB$C_BID;
    fab.fab$b_bln = FAB$C_BLN;
    fab.fab$b_fac = FAB$M_UPD;
    fab.fab$l_fna = dirfile;
    fab.fab$b_fns = STlength(fab.fab$l_fna);
    fab.fab$l_xab = &xab;

    /* Initialize the RMS xab */
    MEfill(sizeof(xab), 0, (PTR)&xab);
    xab.xab$b_cod = XAB$C_PRO;
    xab.xab$b_bln = XAB$C_PROLEN;

    /* See if we can open it */
    if ((sys$open(&fab) & 1) == 0)
    	return FAIL;

    xab.xab$w_pro = perms;
    if((sys$close(&fab) & 1) == 0)
    	return FAIL;

    return OK;
}
