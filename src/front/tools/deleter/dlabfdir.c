/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<nm.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<lo.h>
# include	<er.h>
# include	<uigdata.h>

/*
** Some static globals for this file
*/

static char	abDbdir[MAX_LOC]	= {0};
static char	abObjdir[MAX_LOC]	= {0};

/*
**	abdirinit
**		Move to the object directory for the application.
**
**	Parameters:
**		NONE
**
**	Returns:
**		NOTHING
**
**	Called by:
**		defapp
**
**	Side Effects:
**		Changes the directory.
**
**	Trace Flags:
**		NONE
**
**	Error Messages:
**		NONE
**
**	History:
**		Written 7/19/82 (jrc)
**
**		29-jul-86 (mgw)
**			Changed abDbname to dabDbname to facilitate 4.0/04 the
**			4.0/04 build. abDbname became defined in frame.exe and
**			so was multiply defined in dlabfdir.c
**		07-nov-89 (mgw)
**			Changed LOaddpath() call to LOfaddpath() call to
**			facilitate using loc as head and result and making
**			a location that might not exist yet. This as per
**			Mike S.
**	28-aug-1990 (Joe)
**	    Changed references to IIUIgdata to the UIDBDATA structure
**	    returned from IIUIdbdata().
**	23-aug-91 (leighb) DeskTop Porting Changes:
**	    Added return type VOID to all 3 functions;
**	    Removed unreferenced argument from abdirdestroy().
*/
VOID							 
abdirinit()
{
	LOCATION	loc;
	char		node_name[256];
	char		dbname[256];
	char		*cp;
	char		tbuf[256];
	char		lbuf[256];

	NMgtAt(ERx("ING_ABFDIR"), &cp);
	STcopy(cp, lbuf);
	LOfroms(PATH, lbuf, &loc);
	STcopy(IIUIdbdata()->database, tbuf);
	FEnetname(tbuf,node_name,dbname);
	LOfaddpath(&loc, dbname, &loc);
	LOtos(&loc, &cp);
	STcopy(cp, abDbdir);
}

/*
**	abdirset
**		Set the directory to the object directory.
**
**	Parameters:
**		objdir		{char *}
**			The name of the objdir.
**
**	Returns:
**		NOTHING
**
**	Called by:
**		abdefapp
**
**	Side Effects:
**		Changes the working directory.
**
**	Trace Flags:
**		NONE
**
**	Error Messages:
**		NONE
**
**	History:
**		Written 7/19/82 (jrc)
**		07-nov-89 (mgw)
**			Changed LOaddpath() call to LOfaddpath() call to
**			facilitate using dbdir as head and result and making
**			a location that might not exist yet. This as per
**			Mike S.
*/
VOID							 
abdirset(objdir)
char	*objdir;
{
	LOCATION	dbdir;
	char		*cp;
	char		dbbuf[256];
	char		buf[256];

	STcopy(abDbdir, dbbuf);
	LOfroms(PATH, dbbuf, &dbdir);
	STcopy(objdir, buf);
	LOfaddpath(&dbdir, buf, &dbdir);
	LOtos(&dbdir, &cp);
	STcopy(cp, abObjdir);
}


/*
**	abdirdestroy
**		Destroy the directory for a particular application.
**		When done the directory will be ABFDIR.
**
**	Parameters:
**		app		{char *}
**			The name of the application.
**
**	Returns:
**		NOTHING
**
**	Called by:
**		defapp
**
**	Side Effects:
**		Removes all the files for the application.
**
**	Trace Flags:
**
**
**	Error Messages:
**
**
**	History:
**		Written 9/24/82 (jrc)
*/
VOID							 
abdirdestroy()						 
{
	LOCATION	dir;
	char		buf[256];

	STcopy(abObjdir, buf);
	LOfroms(PATH, buf, &dir);
	LOdelete(&dir);
}
