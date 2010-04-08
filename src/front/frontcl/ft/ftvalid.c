/*
**  ftvalid.c
**
**  Contains variables and routines to handle new validation
**  semantics for 4.0.
**
**  Copyright (c) 2004 Ingres Corporation
**
**
**  History:
**
**	Created - 07/30/85 (dkh)
 * Revision 60.3  87/04/08  00:03:21  joe
 * Added compat, dbms and fe.h
 * 
 * Revision 60.2  86/11/18  21:58:18  dave
 * Initial60 edit of files
 * 
 * Revision 60.1  86/11/18  21:58:07  dave
 * *** empty log message ***
 * 
 * Revision 40.3  85/12/05  20:32:33  dave
 * Added support for set/inquire for activation on a field. (dkh)
 * 
 * Revision 40.2  85/09/07  22:53:35  dave
 * Added support for set and inquire statements. (dkh)
 * 
 * Revision 1.2  85/09/07  20:50:09  dave
 * Added support for set and inquire statements. (dkh)
 * 
 * Revision 1.1  85/08/09  10:56:59  dave
 * Initial revision
 * 
**      23-sep-96 (mcgem01)
**              Global data moved to ftdata.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<frscnsts.h>

GLOBALREF	bool	ftvalnext;
GLOBALREF	bool	ftvalprev;
GLOBALREF	bool	ftvalmenu;
GLOBALREF	bool	ftvalanymenu;
GLOBALREF	bool	ftvalfrskey;
GLOBALREF	bool	ftactnext;
GLOBALREF	bool	ftactprev;


FTinqvalidate(which)
i4	which;
{
	i4	retval = 0;

	switch(which)
	{
		case SETVALNEXT:
			retval = ftvalnext;
			break;

		case SETVALPREV:
			retval = ftvalprev;
			break;

		case SETVALMENU:
			retval = ftvalmenu;
			break;

		case SETVALANMU:
			retval = ftvalanymenu;
			break;

		case SETVALFRSK:
			retval = ftvalfrskey;
			break;

		case SETACTNEXT:
			retval = ftactnext;
			break;

		case SETACTPREV:
			retval = ftactprev;
			break;

		default:
			break;
	}
	return(retval);
}

FTsetvalidate(which, val)
i4	which;
bool	val;
{
	switch(which)
	{
		case SETVALNEXT:
			ftvalnext = val;
			break;

		case SETVALPREV:
			ftvalprev = val;
			break;

		case SETVALMENU:
			ftvalmenu = val;
			break;

		case SETVALANMU:
			ftvalanymenu = val;
			break;

		case SETVALFRSK:
			ftvalfrskey = val;
			break;

		case SETACTNEXT:
			ftactnext = val;
			break;

		case SETACTPREV:
			ftactprev = val;
			break;

		default:
			break;
	}
}
