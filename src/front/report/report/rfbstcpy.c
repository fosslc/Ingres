/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<cm.h>
# include	<er.h>

/* static char	Sccsid[] = "@(#)rfbstrcpy.c	30.1	11/14/84"; */

/*
** Name:    rfbstcpy.c
**
** Description:
**	This file contains two functions.  One escapes single quotes (')
**	and the other de-escapes them.
**
** History:
**    11/14/84 (peter)	created rF_bstrcpy.
**
**    03/15/91 (steveh)	added rF_unbstrcpy as part of bug fix for Bug 9527. 
**			Added this header. Declared return types to be VOID.
**
**    25/08/92 (lucius) Included cm.h and used CM routines to copy source
**			string to dest instead. These take care of the 
**			problem incurred by the shift-jis characters ended
**			with 5c(i.e. '\') as lower byte value.
**			b46271
**
**    02/09/92 (lucius) Recoded rF_bstrcpy to comply with coding standards
**			and indicate clearly the loop terminating condition.
**			Changed rF_unbstrcpy to be in sync with rF_bstrcpy.
**    9-mar-1993 (rdrane)
**			Redefine this module to effect escape/un-escape of
**			single quotes now that RBF only generates single
**			quoted string constants.  Note that .QUERY statements
**			should never go through this routine, and so this
**			change has no impact on QUEL qualifications.
**    9-jul-1993 (rdrane)
**			Add routine to double embedded backslashes.  This
**			has become necessary for the support of user
**			specification of date/time and page number formats.
**    14-jan-1994 (rdrane)
**			Commonize code of rF_bstrcpy() and rF_unbstrcpy()
**			to UI to address a range of bugs regarding delimited
**			identifiers containing embedded single quotes.
**/

static	char	*bs_char = ERx("\\");

/*
** Copies source to dest, preceding single quotes (') with another single
** quote, as per Report-Writer escapement rules for single quote delimited
** strings.
*/


VOID
rF_bstrcpy(dest, source)
register char	*dest;
register char	*source;
{


	IIUIea_escapos(source,dest);
	return;
}

/*
** Copies source to dest, de-escaping single quotes (').
*/

VOID
rF_unbstrcpy(dest, source)
register char	*dest;
register char	*source;
{


	IIUIuea_unescapos(source,dest);
	return;
}

VOID
rF_bsdbl(dest,source)
	char	*dest;
	char	*source;
{


	while (*source != EOS)
	{
		if  (CMcmpnocase(source,bs_char) == 0)
		{
			CMcpychar(bs_char,dest);
			CMnext(dest);
		}
		CMcpyinc(source,dest);
	}
	*dest = EOS;

	return;
}
