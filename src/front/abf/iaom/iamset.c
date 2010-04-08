/*
**	Copyright (c) 1987, 2004 Ingres Corporation
*/

#include <compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include <fe.h>
# include	<er.h>
#include "iamstd.h"

/**
** Name:	iamset.c - set AOM states.
**
*/

extern bool Xact;
extern i4  Cvers;
extern bool Timestamp;
extern char *Catowner;

static char Ownbuffer[FE_MAXNAME+1];

/*{
** Name:	IIAMsgSetGlob - set AOM globals
**
** Description:
**	Sets several parameters for AOM catalog record interactions.
**	May be used to selectively set only certain parameters.	 The
**	"care" mask convention will allow new parameters to be added
**	without causing old code to fail (the bits won't be set).
**
** Inputs:
**	care - mask of which arguments matter.	low order bit means
**		second argument (xs), next bit third argument, etc.
**		Arguments corresponding to unset bits are ignored.
**		However, "placeholders" will have to be passed so
**		argument types match to pass the later arguments.
**	xs - transaction state.	 If true, AOM will do transactions
**		when updating catalogs.	 TRUE by default.
**	ts - timestamp state.  If true, AOM will handle timestamps.
**		Otherwise, catalog records will contain callers timestamps.
**	vers - version to stamp records with.  ACAT_VERSION by default.
**	own - owner for new records.  IIUIuser by default.
**
** History:
**	6/87 (bobm)	written
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

IIAMsgSetGlob(care,xs,ts,vers,own)
u_i4 care;
bool xs;
bool ts;
i4  vers;
char *own;
{
	if (care & 1)
		Xact = xs;
	if (care & 2)
		Timestamp = ts;
	if (care & 4)
		Cvers = vers;
	if (care & 8)
	{
		STlcopy (own,Ownbuffer,FE_MAXNAME);
		Ownbuffer[FE_MAXNAME] = EOS;
		Catowner = Ownbuffer;
	}
}
