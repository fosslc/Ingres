/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>

/**
** Name:	fenetname.c -	Front-End Net Name Routine.
**
** Descriaption:
**	Routine to break a database entered by the user into two
**	components, a network node name and the name of the
**	database on the node.  If a node name can not be decyphered
**	then the whole name is assumed to be just a database name.
**
**	Bugs:  Does not check for existence of a NULL node or
**	db name.  This doesn't care, since we leave it to
**	INingres to try and start up the database.
**
** History:
**	Revision 6.2  89/06 bobm
**	Strip off /ANYTHING, since we now have "/star" also.  Perhaps the stuff
**	    after the slash should get passed back in nodename, but since the
**	    "/d" wasn't, I won't.
**	89/10/26  terryr  Added length argument to STindex("/") call, omission
**	    worked in some cases but was munging dbname assignment in others.
**	89/10/26  wong  Rewrote to remove unnecessary copy.
**
**	Saturn changes: 11-oct-1986 (sandyd)
**	This routine now strips off the "/d" (if any) from
**	the end of the database name.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

VOID
FEnetname ( src, nodename, dbname )
char	*src;
char	*nodename;
char	*dbname;
{
	register char	*start;
	register char	*end;

	/*
	** Database specification:
	**
	**	[ nodename :: ] dbname [ /.* ]
	*/
	start = STindex(src, ERx(":"), 0);
	if (start == NULL || *(start+1) != ':')
	{ /* dbname only */
		*nodename = EOS;
		start = src;
	}
	else
	{ /* nodename :: dbname */
		STlcopy( src, nodename, (u_i4)(start - src) );
		start += 2;
	}

	/* Chop off the "/.*" (if there is any) */
	if ( (end = STindex(start, ERx("/"), 0)) != NULL )
		STlcopy( start, dbname, (u_i4)(end - start) );
	else
		STcopy( start, dbname );
}
