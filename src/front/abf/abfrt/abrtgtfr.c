/*
** Copyright (c) 1991, 2008 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ooclass.h>
#include	<fdesc.h>
#include	<abfrts.h>

/**
** Name:	abrtgtfr.c -	ABF Run-time Get Frame Name Routine.
**
** Description:
**	Contains the routine that implements the get frame name built-in
**	procedure for the ABF run-time system.  Defines:
**
**	IIARgtFrmname()	get frame name
**
** History:
**	Revision 6.4
**	04/07/91 (emerson)
**		Modified IIARgtFrmName for local procedures.
**
**	Revision 6.3/03/00  90/05  wong
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
*/

GLOBALREF ABRTSSTK	*IIarRtsStack;

/*{
** Name:	IIARgtFrmName() -	Get Frame Name.
**
** Description:
**	Returns the requested frame name.  This can either be the "current",
**	the "parent", or the "entry" frame.  An empty name is returned on error.
**
** Inputs:
**	frmtype	{char *}  Requested frame name.
**
** Returns:
**	{char *}  The name of the frame, empty on error.
**
** History:
**	05/90 (jhw) -- Written.
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Handle OC_OSLLOCALPROC.
*/

static const char	_Current[]	= ERx("current"),
			_Entry[]	= ERx("entry"),
			_Parent[]	= ERx("parent"),
			_Empty[]	= ERx("");

char *
IIARgtFrmName ( frmtype )
char		*frmtype;
{
	register ABRTSSTK	*stk;
	register i4		depth;

	if ( STtrmwhite(frmtype) <= 0 )
		return _Empty;	/* error */
	else if ( STbcompare(_Current, 0, frmtype, 0, TRUE) == 0 )
		depth = 1;	
	else if ( STbcompare(_Entry, 0, frmtype, 0, TRUE) == 0 )
		depth = MAXI2;
	else if ( STbcompare(_Parent, 0, frmtype, 0, TRUE) == 0 )
		depth = 2;
	else
		return _Empty;	/* error */

	for ( stk = IIarRtsStack ; stk != NULL ; stk = stk->abrfsnext )
	{
		switch (stk->abrfsusage)
		{
		  case OC_OSLFRAME:
		  case OC_MUFRAME:
		  case OC_APPFRAME:
		  case OC_UPDFRAME:
		  case OC_BRWFRAME:
		  case OC_OLDOSLFRAME:
			if ( --depth == 0 || ( depth > 1
						&& stk->abrfsnext == NULL ) )
				return stk->abrfsname;
			break;
		  case OC_OSLPROC:
		  case OC_OSLLOCALPROC:
			/* 4GL procedures could also be handled, but aren't */
			break;
		  default:
		  case OC_QBFFRAME:
		  case OC_RWFRAME:
		  case OC_GRFRAME:
		  case OC_HLPROC:
			/* these can never call anything, so skip them */
			return _Empty;
		}
	} /* end for */
	return _Empty;
}
