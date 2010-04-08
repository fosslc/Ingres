/*
** Copyright (c) 1993, 2008 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<cv.h>		/* 6-x_PC_80x86 */
#include	<lo.h> 
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	<abfcnsts.h>
#include	<fdesc.h>
#include	<abfrts.h>
#include	<abclass.h>
#include	"abffile.h"
#include	<abfglobs.h>
#include	"erab.h"

/**
** Name:	abffile.c -	ABF File Handling Module.
**
** Description:
**	Contains some routines that build file locations, etc.  Defines:
**
**	iiabMkLoc()	make source-code file location.
**	abfilsrc()	make source-code file location from ABFILE structure.
**	iiabMkFormLoc() make source-code file location for compiled form.
**	iiabCkSrcFile() check legality of file name.
**	iiabMkObjLoc()	make object-code file location.
**	iiabRmObjLoc()  remove (delete) object file.
**	iiabRiRmInterp() remove (delete) interpreter executable file.
**
** History:
**
**	20-May-1993 (fredb)
**		IFDEF my prior changes to LO buffer sizes.  The relationship
**		between the defined sizes is different between MPE & UNIX.
**		Bug #51560
**	04-Feb-93 (fredb)
**		Changed length of base buffer to avoid overruns in 
**		iiabMkFormLoc().
**	14-Jan-93 (fredb) hp9_mpe
**	Added more documentation.
**	MPE/iX specific porting changes to iiabMkFormLoc and iiabCkSrcFile.
**
**	Revision 6.3  89/11  wong
**	Added new file name check routine, 'iiabCkSrcFile()'.  JupBug #8627.
**
**	Revision 6.2  89/02  wong
**	Completely rewrote.
**      21-apr-1999 (hanch04)
**        Replace STrindex with STrchr
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	24-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

FUNC_EXTERN LOCATION *iiabMkObjLoc();

/*{
** Name:	iiabMkLoc() -	Build Source-Code File Location.
**
** Description:
**	Makes a location for a source file.  The source directory
**	is pointed to by the file.
** Input:
**      src_path        {char *}  The source path (i.e. account) name.
**      filename        {char *}  The name of the file, including extension.
**
** Output:
**      iiabMkLoc()     {LOCATION *} Returns a location pointer to a static
**                                   location, if successful.
**
** Description:
**      This function takes as input a source path and a filename (file.ext)
**      and returns a pointer to a location containing the full filename.
**      
** Side Effects:
**      Static locations are used; therefore, after this function is called,
**      the location information should be LOcopy()'d to preserve it, as this
**      location information will be overwritten at the next iiabMkLoc() call.
**
** History:
**      11-Dec-89 (mrelac) (hp9_mpe)
**              Modified this function to work properly for MPE/XL.  The
**              current algorithm for this function doesn't work for MPE/XL;
**              trying to LO[f]stfile() a location with a NULL group (which is
**              what this code produces for us) causes the resulting path and
**              filename to be combined into a single entity (i.e. 'abfurts.h'
**              becomes 'abfurth').  Instead of this approach, we just build
**              the path and filename from the string parameters supplied, then
**              call LOfroms() to convert the full path & filename to a loc.
**              Note: the "." is the MPE/XL file.group.account delimeter.
**      19-Dec-89 (mrelac)
**              EEK!  Sometimes this function is called with a NULL src_path!
**              Changed code to handle that case (i.e. no path delimiter
**              hanging off the end).
*/

# define NO_LOCS 5
static LOCATION Locs[NO_LOCS] ZERO_FILL;
static char	Lbufs[NO_LOCS][MAX_LOC + 1] ZERO_FILL;
static i4	cur_loc = 0;

LOCATION *
iiabMkLoc ( src_path, filename )
char	*src_path;
char	*filename;
{
	register LOCATION *Locptr;
	register char 	  *Lbufptr;

	cur_loc = (cur_loc + 1) % NO_LOCS;
	Locptr = Locs + cur_loc;
	Lbufptr = &Lbufs[cur_loc][0];

#ifdef hp9_mpe
        STcopy (filename, Lbufptr);
        if ( (src_path != NULL) && (*src_path != EOS) )
                STprintf (Lbufptr, "%s.%s", Lbufptr, src_path);
 
        return ( (LOfroms (PATH & FILENAME, Lbufptr, Locptr) != OK )
                        ? (LOCATION *)NULL : Locptr);
# else
	STlcopy(src_path, Lbufptr, MAX_LOC + 1);
	return ( LOfroms(PATH, Lbufptr, Locptr) != OK ||
				LOfstfile(filename, Locptr) != OK )
			? (LOCATION *)NULL : Locptr;
# endif
}

static LOCATION	Src ZERO_FILL;
static char	Sbuf[MAX_LOC + 1] ZERO_FILL;

LOCATION *
abfilsrc(file)
register ABFILE *file;
{
	LOCATION	*loc;

	loc = iiabMkLoc(file->fisdir, file->fisname);

	if ( loc != NULL )
	{
		LOcopy(loc, Sbuf, &Src);
		loc = &Src;
	}
	return loc;
}

/*{
** Name:	iiabMkFormLoc() -	Build Source-Code File Location for
**						Compiled Form.
** Description:
**	Makes a source-code file location for a compiled form given the
**	source-code pathname and the form file name (minus extension.)
**	This routine will append the appropriate system-dependent extension.
**
** Inputs:
**
** Returns:
**	{LOCATION *}  The source-code file location.
**
** History:
**	09/89 (jhw) -- Written.
**	09/90 (Mike S) -- Put compiled forms in object directory
**	14-Jan-93 (fredb) hp9_mpe
**		MPE/iX porting changes: Limited name space and flat 'directory'
**		structure force us to make different LO calls to generate the
**		file location.
**	04-Feb-93 (fredb)
**		Changed length of base buffer to avoid overruns.
**	20-May-1993 (fredb)
**		IFDEF my prior changes to LO buffer sizes.  The relationship
**		between the defined sizes is different between MPE & UNIX.
**		Bug #51560
*/
LOCATION *
iiabMkFormLoc ( formfile )
char	*formfile;
{
#ifdef hp9_mpe
	char		*Lbufptr;
	char            base[LO_FPREFIX_MAX+1];
	char            path[MAX_LOC + 1];
	char		*cptr;
#else
	char            base[LO_NM_LEN+1];
#endif
	LOCATION 	*oloc;
	char            junk[MAX_LOC + 1];

	oloc = iiabMkObjLoc(formfile);
	if (oloc == NULL)
		return NULL;
#ifdef hp9_mpe
	if (LOdetail(oloc, junk, path, base, junk, junk) != OK)
#else
	if (LOdetail(oloc, junk, junk, base, junk, junk) != OK)
#endif
		return NULL;

#ifdef hp9_mpe
	/*
	** Set a pointer to the buffer for "oloc"; set a pointer to the
	** account name in "path"; build a file.group.account string;
	** and convert it to a location which will be returned to the
	** caller.
	*/
	Lbufptr = &Lbufs[cur_loc] [0];
	if (cptr = STrchr(path, '.'))
	{
		cptr++;		/* advance past "." */
	}
	else
	{
		cptr = path;	/* path was account only */
	}
	_VOID_ STprintf(Lbufptr, ERx("%s.%s.%s"), base, ABFORM_EXT, cptr);
	if (LOfroms (PATH & FILENAME, Lbufptr, oloc) != OK)
		return NULL;
#else
	_VOID_ STprintf(junk, ERx("%s.%s"), base, ABFORM_EXT);
	_VOID_ LOfstfile(junk, oloc);
#endif
	return oloc;
}

/*{
** Name:	iiabCkSrcFile() -	Check Legality of File Name.
**
** Description:
**	Check whether a file name is legal in the context of the current
**	source-code directory.  This will report the error message and
**	return FALSE if it is not legal.
**
** Input:
**	src_path	{char *}  The source-code directory path.
**	filename	{char *}  The file name.
**
** Returns:
**	{bool}  TRUE if the file name is legal.
**		FALSE, otherwise.
**
** Side Effects:
**	Will report an error if it is not legal.
**
** History:
**	11/89 (jhw) -- Written.  JupBug #8627.
**	15-Jan-93 (fredb) hp9_mpe
**		MPE/iX porting changes: Limited name space and flat 'directory'
**		structure force us to make different LO calls to generate the
**		file location.
*/
bool
iiabCkSrcFile ( char *src_path, char *filename )
{
	STATUS		rval;
	LOCATION	loc;
	char		lbuf[MAX_LOC+1];

#ifdef hp9_mpe
	/*
	** Assumes src_path is never NULL or EOS; and that
	** filename includes an extension (group); and that
	** src_path is account only.
	*/
	STprintf(lbuf, "%s.%s", filename, src_path);
	if ( (rval = LOfroms(PATH & FILENAME, lbuf, &loc)) != OK)
#else
	STlcopy(src_path, lbuf, sizeof(lbuf));
	if ( (rval = LOfroms(PATH, lbuf, &loc)) != OK ||
				(rval = LOfstfile(filename, &loc)) != OK )
#endif
	{
		char	errbuf[ER_MAX_LEN+1];

		if ( rval != FAIL )
			ERreport(rval, errbuf);
		else
			errbuf[0] = EOS;
		IIUGerr( E_AB0138_IllegalFileName, UG_ERR_ERROR,
				3, filename, src_path, errbuf
		);
		return FALSE;
	}
	return TRUE;
}

/*{
** Name:	iiabMkObjLoc() -	Build Object-Code File Location.
**
*/
static LOCATION *mkObjLoc();

LOCATION *
iiabMkObjLoc ( filename )
char	*filename;
{
	char	dummy[ABFILSIZE+1];

	return mkObjLoc(filename, dummy);
}

/* 
** iiabRmObjLoc  	- remove object file and clear timestamp.
**
** History:
**	05/90 (jhw) -- Removed call to 'iiabDBclrDate()' since IL dates are now
**		read from the DB not from a time-stamp file.
*/

iiabRmObjLoc ( filename )
char	*filename;
{
	LOCATION	*locp;
	char		basename[ABFILSIZE+1];

	if ( (locp = mkObjLoc(filename, basename)) != NULL )
	{
		LOdelete(locp);
	}
}

static LOCATION *
mkObjLoc ( filename, basename )
char	*filename;
char	*basename;
{
	char		objname[MAX_LOC+1];
	LOCATION	tloc;
	char		tbuf[MAX_LOC+1];
	LOCATION 	*Locptr;
	char 	  	*Lbufptr;

	STlcopy(filename, objname, sizeof(objname));
	if (! LO_NM_CASE)
		CVlower(objname);
	if ( LOfroms(FILENAME, objname, &tloc) != OK  
	  || LOdetail( &tloc, tbuf, tbuf, basename, tbuf, tbuf ) != OK )
	{
		return NULL;
	}

	STlcopy(basename, objname, sizeof(objname));
	STcat(objname, ABOBJEXTENT);
	if ( IIABfdirFile(objname, tbuf, &tloc) != OK )
	{
		IIUGerr(E_AB004C_FileLength, UG_ERR_ERROR, 2, filename,objname);
		return NULL;
	}
	cur_loc = (cur_loc + 1) % NO_LOCS;
	Locptr = Locs + cur_loc;
	Lbufptr = &Lbufs[cur_loc][0];
	LOcopy(&tloc, Lbufptr, Locptr);
	return Locptr;
}

static const char	_interpname[] = "iiinterp";

VOID
iiabRiRmInterp()
{
	LOCATION	loc;
	char		iname[LO_NM_LEN + 1 + LO_EXT_LEN + 1];
	char		buf[MAX_LOC + 1];

	_VOID_ STprintf(iname, "%s%s", _interpname, ABIMGEXT);
	if (IIABfdirFile(iname, buf, &loc) == OK)
	{
		LOdelete(&loc);
	}
}
