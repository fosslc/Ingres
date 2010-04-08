/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<afe.h>


/**
** Name:	afdclass.c -	Determine class for a datatype.
**
** Description:
**	This file defines:
**
**	IIAFddcDetermineDatatypeClass	Determine class for a datatype.
**
** History:
**	25-nov-87 (bab)	Initial implementation.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */
/* GLOBALDEF's */
/* extern's */
/* static's */

/*{
** Name:	IIAFddcDetermineDatatypeClass	- Determine class for a datatype
**
** Description:
**	For a given datatype, determine whether it is a character, numeric,
**	date, or 'other' datatype.
**
** Inputs:
**	datatype	The datatype for which to determine the class.
**
** Outputs:
**	class		Set to the class of the datatype.
**
**	Returns:
**		VOID
**
**	Exceptions:
**		None
**
** Side Effects:
**	None
**
** History:
**	25-nov-87 (bab)	Initial implementation.
**	07-sep-06 (gupsh01)
**	    Added support for ANSI date/time datatypes.
*/
VOID
IIAFddcDetermineDatatypeClass(datatype, class)
i4	datatype;
i4	*class;
{
	datatype = abs(datatype);
	switch (datatype)
	{
	  case DB_INT_TYPE:
	  case DB_FLT_TYPE:
	  case DB_MNY_TYPE:
		*class = NUM_DTYPE;
		break;

	  case DB_CHR_TYPE:
	  case DB_TXT_TYPE:
	  case DB_CHA_TYPE:
	  case DB_VCH_TYPE:
		*class = CHAR_DTYPE;
		break;

	  case DB_DTE_TYPE:
	  case DB_ADTE_TYPE:
	  case DB_TMWO_TYPE:
	  case DB_TMW_TYPE:
	  case DB_TME_TYPE:
	  case DB_TSWO_TYPE:
	  case DB_TSW_TYPE:
	  case DB_TSTMP_TYPE:
	  case DB_INYM_TYPE:
	  case DB_INDS_TYPE:
		*class = DATE_DTYPE;
		break;

	  default:
		*class = ANY_DTYPE;
		break;
	}
}
