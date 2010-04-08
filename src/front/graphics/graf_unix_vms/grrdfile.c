/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ergr.h>
# include	<er.h>
# include	<ooclass.h>
# include	<oodefine.h>
# include	<oosymbol.h>
# include	<lo.h>
# include	<grerr.h>
# include	<uigdata.h>

/*
** Description:
**	Contains a routine to read a graph from a file.
**
**	GRreadfile	read graph from file.
**
** History:
**	6/15/87 (peterk) - history deleted
**	4/21/89 (Mike S) -- Made saving a graph into a transaction
**	4/24/89 (Mike S) -- Allocate object id, use UGcat_now, make deletion
**			    into a transaction too..
**	1/21/93 (pghosh) -- Modified _flushAll to ii_flushAll as this was 
**			    modifed by fraser in oosymbol.qsh file.
**	10/28/93 (donc)  -- Cast all OOsndxxx calls to OOID. These functions
**			    are now declared as returning PTR.
**/

/*{
** Name:	GRreadfile() -	Read OC_GRAPH from File.
**
** Description:
**	Calls the ObjectUtility to read a graph from a file location
**	into the database.
**
** Inputs:
**	loc		The file location.
**	name		Name to copy the graph to, NULL to read from file.
**	force		TRUE to force overwrite of existing graph.
**
** Outputs:
**	rname		returned name (for error messages in case name = NULL)
**
** Returns:
**	STATUS	OK, if no error.
**
** History:
**	02/86 (pk) -- Written.
**	09/88 (bobm) - add rname
**	8/90 (Mike S) - ifdef for mac -- porting change.  God knows why.
**		Use LOtos to get location strings.
*/

STATUS
GRreadfile (loc, name, force, rname)
LOCATION	*loc;
char		*name;
i4		force;
char		**rname;
{
	FUNC_EXTERN OOID 	IIOOnewId();
	FUNC_EXTERN bool	OOisTempId();
	FUNC_EXTERN char *	UGcat_now();

	register OOID		eg;	/* Encoding object id */
	register OOID		grid;	/* Graph object id */
	register OO_GRAPH	*gr;	/* Graph object ptr */
	OOID			grold;	/* id of existing Graph */
	OOID			permid; /* Permanent id of new graph */		
	char			*filename;

	_VOID_ LOtos(loc, &filename);
# if defined(mac_us5)
	if ((eg = (OOID)OOsnd(OC_ENCODING, _readfil, loc)) == nil)
# else
	if ((eg = (OOID)OOsnd(OC_ENCODING, _readfile, loc)) == nil)
# endif
	{
	    IIUGerr(BAD_CGFILE, 0, 1, filename);
	    return BAD_CGFILE;
	}

	if ((grid = (OOID)OOsnd(eg, _decode, TRUE)) == nil)
	{
	    IIUGerr(BAD_DECOMPILE, 0, 1, filename);
	    return BAD_DECOMPILE;
	}

	gr = (OO_GRAPH *)OOp(grid);
	if (name)
	    gr->name = name;
	*rname = gr->name;
	gr->owner = IIuiUser;

	IIUIbeginXaction();
	grold = (OOID)OOsnd(OC_GRAPH, _withName, gr->name, IIuiUser, NULL);
	if ( (grold != nil) && force)
		(OOID)OOsnd(grold, _destroy);
	IIUIendXaction();
	if ( (grold != nil) && !force)
		return GR_EXISTS;

	gr->create_date = gr->alter_date = UGcat_now();
	if (OOisTempId(gr->ooid))
	{
		permid = IIOOnewId();
		OOhash(permid,gr);
		gr->ooid = permid;
	}
	IIUIbeginXaction();
	if ((OOID)OOsnd(gr->ooid, ii_flushAll) == nil)
	{
	    IIUIabortXaction();
	    IIUGerr(BAD_SAVE, 0, 1, gr->name);
	    return BAD_SAVE;
	}
	else
	{
		IIUIendXaction();
	}

	return OK;
}
