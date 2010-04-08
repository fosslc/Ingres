/*
** Copyright (c) 1983, 2008 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<descrip.h>
# include	<iledef.h>
# include	<lnmdef.h>
# include	<starlet.h>
# include	<st.h>
# include	<lo.h>
# include 	<er.h>  
# include	"lolocal.h"

/*	LOcurnode() -- add the nodename of the current machine to the location.
**
**	Returns:
**		success:	return OK
**		failure:	FAIL -- if current machine not on network
**				LO_NULL_ARG -- if location passed has not been initialized
**
**
**		History
**			03/09/83 -- (mmm)
**				written
**			09/12/83 -- VMS CL (dd)
**			3-may-1984 (fhc) -- add LOcurnode
**			28-sep-1989 (Mike S) clean up
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	09-oct-2008 (stegr01/joea)
**	    Replace ITEMLIST by ILE3.
*/

/* static char	*Sccsid = "@(#)LOaddpath.c	1.1  3/22/83"; */

/*
** Current node is the value of the logical SYS$NODE in the LNM$FILE_DEV
** list of logical name tables.
*/
static $DESCRIPTOR(sysnode, "SYS$NODE");
static $DESCRIPTOR(table_list, "LNM$FILE_DEV");

STATUS
LOcurnode(loc)
LOCATION	*loc;
{
	ILE3		itemlist[2];	/* Itemlist for logical name lookup */
	i4 		osstatus;
	STATUS		sts;

	/* We temporarily construct a new location while adding the nodename */
	LOCATION	nodeloc;		/* Location */
	char		nodestring[MAX_LOC+1];	/* string */
	i2		length;

	/* Check for null argument */
	if (!loc || !loc->string || !(*(loc->string)))
		return(LO_NULL_ARG);

	/* See if it already has a node */
 	if (loc->nodelen != 0)
		return(FAIL);

	/* Fill in itemlist */
	itemlist[0].ile3$w_length = MAX_LOC;
	itemlist[0].ile3$w_code = LNM$_STRING;
	itemlist[0].ile3$ps_bufaddr = nodestring;
	itemlist[0].ile3$ps_retlen_addr = &length;
	itemlist[1].ile3$w_length = itemlist[1].ile3$w_code = 0;

	/* Translate logical name */
	osstatus = sys$trnlnm(0, &table_list, &sysnode, 0, itemlist );
	if (((osstatus & 1) == 0))
		return(FAIL);
	nodestring[length] = EOS;		/* Null-terminate it */

	/* make a new location with the node */
	STcat(nodestring, loc->string);
	if ((sts = LOfroms(loc->desc & NODE, nodestring, &nodeloc)) != OK)
		return sts;
        LOcopy(&nodeloc, loc->string, loc);
	return(OK);
}
