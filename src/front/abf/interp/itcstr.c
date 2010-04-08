/*
**Copyright (c) 1987, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<iltypes.h>
# include	"il.h"
# include	"itcstr.h"


/**
** Name:	itcstr.c   -    Interp C string handlers
**
** Description:
**	See the description of this module in itcstr.h.
**
**	Defines:
**		IIITtcsToCStr
**		IIITrcsResetCStr
**
** History:
**	26-oct-1988 (Joe)
**		First Written
**	26-jun-92 (davel)
**		added an argument to IIOgvGetVal() call.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/* # define's */
/* GLOBALDEF's */
/* extern's */
/* static's */

/*{
** Name:	IIITtcsToCStr	- Convert an IL value to a C string.
**
** Description:
**	This routine converts an IL value to a C string.  The IL
**	value must be a character typed value that is either a
**	variable of one of the ADF types, or a constant of type
**	DB_CHR_TYPE or DB_CHA_TYPE.  It can not be a TEXT, VARCHAR
**	or hex constant.   No check is made in this routine for
**	the validity of the argument.  If the argument is a constant
**	then it is assumed to be a CHAR constant.
**
**	A pointer to a NULL-terminated ('\0') C string is returned
**	by this routine.  This returned C string, is the argument
**	converted to a C string.  The C string will be valid until
**	the next call to IIITrcsResetCStr.  See the protocol description
**	in itcstr.h.
**
**	If the val is 0, then a NULL pointer is returned.
**
** Inputs:
**	val		The IL value to convert to a C string.
**
** Outputs:
**	Returns:
**		A pointer to the argument's value converted to a C string.
**
** Side Effects:
**	May call IIARtocToCstr which can allocate dynamic memory.
**
** History:
**	26-oct-1988 (Joe)
**		First Written
*/
char *
IIITtcsToCStr(val)
IL	val;
{
    FUNC_EXTERN char	*IIARtocToCstr();

    if (val == 0)
	return NULL;
    else if (ILisConst(val))
	return (char *) IIOgvGetVal(val, DB_CHR_TYPE, (i4 *)NULL);
    else
	return IIARtocToCstr(IIOFdgDbdvGet((i4) val));
}

/*{
** Name:	IIITrcsResetCStr - Reset the C string handler's memory.
**
** Description:
**	This call reclaims any memory used by previous calls to IIITtcsToCStr.
**	See the protocol description in itcstr.h.
**
** Side Effects:
**	May free some dynamic memory through IIARrcsResetCstr()
**
** History:
**	26-oct-1988 (Joe)
*/
VOID
IIITrcsResetCStr()
{
    IIARrcsResetCstr();
}
