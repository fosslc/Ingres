/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/* # include's */
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<afe.h>

/**
** Name:	afdplen.c	-	Compute the display length for a type.
**
** Description:
**	This file contains afe_dplen and defines:
**
** 	afe_dplen	Compute the display length for a type.
**
** History:
**	Written	2/6/87	(dpr)
**	6/25/87 (danielt) removed unused variable in afe_dplen()
**
**	06/27/92 (dkh) - Updated code to handle changed interface for
**			 afe_ficoerce().  Change was needed to handle
**			 decimal datatypes for 6.5.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */

/* extern's */

FUNC_EXTERN	DB_STATUS	afe_ficoerce();

/* static's */

/*{
** Name:	afe_dplen	-	Compute the display length for a type.
**
** Description:
**	Given an ADF type and its length, calculate the size of needed to
**	display the value in.  It is the size of the DB_LTXT needed to
**	hold the display value.
**
** Inputs:
**	cb			A pointer to the current ADF_CB 
**				control block.
**
**	value			A DB_DATA_VALUE that contains the type and
**				the length.  Only the type and length fields
**				of value will be used by this routine.
**
**	    .db_datatype	The type id of the ADF type to find the
**				display length for.
**
**	    .db_length		The length of the ADF type to find the
**				display length for.
**
** Outputs:
**	displen		The calculated display length for the ADF type.
**			This in only valid if OK is returned.
**
**	Returns:
**		OK		If the display length was successfully
**				determined.
**
**		Legal returns values from:
**			afe_ficoerce
*/
DB_STATUS
afe_dplen(cb, value, displen)
ADF_CB		*cb;
DB_DATA_VALUE	*value;
i4		*displen;
{
	DB_DATA_VALUE	ltxt;
	ADI_FI_ID	fid;
	DB_STATUS	rtval;
	
	ltxt.db_datatype = DB_LTXT_TYPE;
	ltxt.db_length = 0; 		/* unused, but set for safety */
	ltxt.db_prec = 0;

	/* 
	** just find the result length of the coercion between our
	** DB_DATA_VALUE passed in and longtext. we can do this by
	** a single call to afe_ficoerce. in addition, we get back
	** the function instance id to actually perform the coercion,
	** but we don't care about it, and neither does (do?) our clients.
	*/

	rtval = afe_ficoerce(cb, value, &ltxt, &fid);
	if (rtval == OK)
	{
		*displen = ltxt.db_length;
	}

	return(rtval);
}
