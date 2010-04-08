/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<me.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ergr.h>
# include	<er.h>
# include	<ooclass.h>
# include	<oodefine.h>
# include	<oosymbol.h>

/**
** Name:    grencode.qc	 -  code for compiling/decompiling graph
**		objects.
**
** Description:
**	Compile of Graphs is specialized to include components
**	and related objects in single encoded string.
**
**	This file defines:
**
**	GRencode()	Graph.encode
**	GRCencode()	GrComp.encode
**	GRdecode()	Graph.decode - fetch encoded object
**
** History:
**		28-Oct-93 (donc)
**		Cast references to OOsndxxx routines to OOID,
**		oodefine.h now has them declared as returning PTR.
**
**		4/12/90 (Mike S)
**		Psetbuf now allocates a dynamic buffer
**		Jupbug 21023
**
**		Revision 40.105	 86/04/08  18:59:07  peterk
**
**
**		Revision 40.104	 86/03/31  15:36:36  wong
**		Make 'cbuf' a little larger.
**		***** NOTE:  This should be made dynamic. *****
**
**		Revision 1.1  86/01/31	17:37:00  peterk
**		Initial revision
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
[@history_template@]...
**/


OOID
GRencode(g, level, inDatabase)
register OO_GRAPH *	g;
i4		level;
bool		inDatabase;
{
	register OOID	eg;
	char		*cbuf;

	/* get graph components */
	if (g->encgraph == NULL && g->components == UNKNOWN)
	    g = (OO_GRAPH *)OOsndSelf(g, _decode, FALSE);

	/*
	** create encoded string of graph + components.
	** don't store g->encgraph value as it may change each time.
	*/
	eg = g->encgraph;
	g->encgraph = nil;
	Psetbuf((u_i4)(4*ESTRINGSIZE), (u_i4)(g->data.tag), &cbuf);
	(OOID)OOsndSuper(g, _encode, level, TRUE);
	if (g->components != nil)
	    (OOID)OOsnd(g->components, _do, _encode, 0, FALSE);
	g->encgraph = eg;

	/*
	** create a new encoded object if necessary.
	*/
	if (!level || g->encgraph == nil)
	{
	    eg = (OOID)OOsnd(OC_ENCODING, _new, -1, g->ooid, inDatabase, cbuf);
	    if (level)
		/*
		** level 0 encodes (for export from database)
		** should not be associated with the graph;
		** g->encgraph expects to be a level 1 encode
		*/
		g->encgraph = eg;
	}
	else
	{
	    /* free and replace old encoded string */
	    OO_ENCODING *egobj = (OO_ENCODING *)OOp(eg);
	    MEfree(egobj->estring);
	    egobj->estring = cbuf;
	    egobj->data.inDatabase = inDatabase;
	}

	return eg;
}

GRCencode(comp)
register OO_GRCOMP	*comp;
{
	(OOID)OOsndSuper(comp, _encode, 0, FALSE);
	if (comp->depdats != nil)
	    (OOID)OOsnd(comp->depdats, _do, _encode, 0, FALSE);
	if (comp->axdats != nil)
	    (OOID)OOsnd(comp->axdats, _do, _encode, 0, FALSE);

}

/*
**  GRdecode(OO_GRAPH * g) -- fetch and decode the associated
**	encoded object for OO_GRAPH * g;
*/

OOID
GRdecode(g)
OO_GRAPH	*g;
{
	register OOID	eg = g->encgraph, gid;
	if (eg == nil)
	{
	    eg = (OOID)OOsnd(OC_ENCODING, _alloc, -1);
	    (OOID)OOsnd(eg, _init, g->ooid, g->data.inDatabase, ERx(""));
	}
	gid = (OOID)OOsnd(eg, _decode, FALSE);
	g = (OO_GRAPH *)OOp(gid);
	g->encgraph = eg;
	return gid;
}
