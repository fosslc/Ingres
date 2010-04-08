/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<me.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<cm.h>
# include	<afe.h>
# include       <er.h>

/**
** Name:	aferrtostr.c - 	Extract ADF_CB Error Message into String.
**
** Description:
**	This file contains and defines:
**
** 	afe_errtostr()	extracts ADF_CB error message into a string.
**
** History:
**	Revision 6.0  87/02/06  daver
**	Initial revision.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name:	afe_errtostr() - 	Extract ADF_CB Error Message into String.
**
** Description:
**	Given an ADF_CB (loaded up with an error message), this routine
**	will extract the error message string out of the ADF_CB, and
**	place it in a string provided by the caller.
**	Useful if an FDerror message wishes to include the adf or afe error
**	message string too.
**
** Inputs:
**	cb	{ADF_CB *}  An ADF_CB loaded up with an error message.
**
**	buf	{char *}  String to hold the error message.
**
**	len	{nat *}  Size in bytes of string.
** 
** Outputs: 
**	buf	{char []}  filled in with the ADF_CB error message.
**	
**	*len	{nat}  Length of the error message.
**
** Returns:
**	{DB_STATUS}  OK
**		     E_AF6005_SMALL_BUFFER - buffer not big enough to hold message
** History:
**	Written	2/6/87	(dpr)
**	10/90 (jhw) Added message look up if error code only was set.  #33851.
*/
DB_STATUS
afe_errtostr (cb, buf, len)
ADF_CB	*cb;
char	*buf;
i4	*len;	
{
	i4 emsglen = cb->adf_errcb.ad_emsglen;

	if ( emsglen == 0 )
	{ /* no message, look it up . . . */
		afe_error(cb, cb->adf_errcb.ad_errcode, 0);
		emsglen = cb->adf_errcb.ad_emsglen;
	}

	/* put at most 'len' bytes from cb->ad_errmsgp into buf,
	** then terminate
	*/

	MEcopy(cb->adf_errcb.ad_errmsgp, min(emsglen, *len), buf);
	/* should check if we've over-ran our buffer, and reset len */
	if (emsglen >= *len)
		return afe_error(cb, E_AF6005_SMALL_BUFFER, 0);

	*len = emsglen;	/* length of string is length of cb's buffer + EOS */
	buf[emsglen] = EOS; /* null terminate */
	return OK;
}
