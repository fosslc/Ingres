/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<adf.h>
# include	<fe.h>
# include	<afe.h>

/**
** Name:	afnullst.c -	change the null string.
**
** Description:
**	The routine afe_nullstring should be used if the null string
**	within the cb is to be changed.  This file defines:
**
**	afe_nullstring	change the null string within the cb.
**
** History:
**	02-apr-1987	(daver)		First Written.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name:	afe_nullstring - change the null string.
**
** Description:
**	This routine is used to change the null string floating about
**	in the current ADF_CB. Given a null terminated string, 
**	afe_nullstring will takes its length, load it into the ADF_CB,
**	and change the pointer to the current null string.
**
** Inputs:
**	cb		current ADF_CB in use.
**	
**	str		address of new null string to use. caller must
**			supply the memory for this.
**
** Outputs:
**	cb.adf_nullstr.nlst_length	length of new null string.
**	
**	cb.adf_nullstr.nlst_string	points to new null string.
**
** History:
**	02-apr-1987	(daver)		First Written.
*/
VOID
afe_nullstring(cb, str)
ADF_CB	*cb;
char	*str;
{
	i4	len;

	len = STlength(str);
	cb->adf_nullstr.nlst_length = len;
	cb->adf_nullstr.nlst_string = str;
}
