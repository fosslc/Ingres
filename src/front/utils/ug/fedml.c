/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<ci.h>
#include	<nm.h>
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<fedml.h>

/**
** Name:	fedml.c -	Front-End Utility Determine DML Type Module.
**
** Description:
**	Contains the routine that determines the available DML types
**	for the front-ends.  Defines:
**
**	FEdml()		get values for available and default DMLs.
**
** History:
**	Revision 4.0  85/07/07  joe
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
*/

/*{
** Name:	FEdml() -	Get Values for Available and Default DMLs.
**
** Description:
**	Get the default and available values for the query language.
**	These are defined by looking at the valid installation options,
**	through the CI interface.
**
** Input:
**	def	{nat *}  Place to put default language value;
**	avail	{nat *}  Place to put available language value.
**
** History:
**	07-jul-1985 (joe)	Written.
**	22-oct-1985 (peter)	Added refs to CI routines.
*/

static i4	FEavail = FEDMLNONE;
static i4	FEdef = FEDMLNONE;

VOID
FEdml ( def, avail )
i4	*def;
i4	*avail;
{
	if ( FEavail == FEDMLNONE )
	{ /* See what languages are available */
		bool	quelavail,
   				sqlavail;

		/* is QUEL is available? */
		quelavail = TRUE;
		/* is SQL is available? */
		sqlavail = TRUE;

		if ( quelavail && !sqlavail )
		{
			FEavail = FEDMLQUEL;
		}
		else if ( sqlavail && !quelavail )
		{
			FEavail = FEDMLSQL;
		}
		else if ( sqlavail && quelavail )
		{
			FEavail = FEDMLBOTH;
		}

		if ( FEavail != FEDMLBOTH )
			FEdef = FEavail;
		else
		{ /* both are available; get default */
			char	*cp;

			NMgtAt(ERx("II_DML_DEF"), &cp);
			FEdef = ( cp != NULL && *cp != EOS &&
				STbcompare(ERx("quel"), 0, cp, 0, TRUE) == 0 )
					? FEDMLQUEL : FEDMLSQL;
		}
	}
	*def = FEdef;
	*avail = FEavail;
}
