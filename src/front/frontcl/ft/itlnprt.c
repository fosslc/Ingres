/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<cm.h>
# include	<st.h>
# include	<itline.h>
# include	"itdrloc.h"

/*{
** Name:	ITlnprint	- convert to ASCII characrters
**
** Description:
**	This function converts the internal codes in the input
**	buffer to the ASCII characters ('-', '+' and '|').
**
** Inputs:
**	inbuf	input buffer of internal codes.
**	len	length of inbuf.
**
** Outputs:
**	prbuf	output buffer re[;aced bu ASCII characters.
**
**	Returns:
**		length of the converted string.
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	06-nov-1986 (yamamoto)
**		written
**	06/19/87 (dkh) - Code cleanup.
**	04/14/90 (dkh) - Tuned code to run faster for TM.
**
**	15-nov-91 (leighb) DeskTop Porting Change:
**		Check that inbuf char is not = DRCH_SEC (section marker).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# define INCBUF(c, buf, len)	(buf += drchtbl[c].prlen, len += drchtbl[c].prlen)


i4
ITlnprint(inbuf, prbuf, len)
register char 	*inbuf;
register char	*prbuf;
i4		len;
{
	register i4	outlen = 0;
	register i4	i;
	register char	*p;

	if (dbpflg)
	{
		*prbuf++ = *inbuf++;
		outlen++;
		len--;
		dbpflg = 0;
	}

	while (*inbuf && len > 0)
	{
	    if ((*inbuf >= DRCH_LR && *inbuf <= DRCH_V) 		 
		&& (*inbuf != DRCH_SEC))				 
	    {
		switch (*inbuf)
		{
		case DRCH_LR:		/* lower-right corner of a box	*/
			i = drchtbl[DRQA].prlen;
			p = (char *) drchtbl[DRQA].prchar;
			break;

		case DRCH_UR:		/* upper-right corner of a box	*/
			i = drchtbl[DRQB].prlen;
			p = (char *) drchtbl[DRQB].prchar;
			break;

		case DRCH_UL:		/* upper-left corner of a box	*/
			i = drchtbl[DRQC].prlen;
			p = (char *) drchtbl[DRQC].prchar;
			break;

		case DRCH_LL:		/* lower-left corner of a box	*/
			i = drchtbl[DRQD].prlen;
			p = (char *) drchtbl[DRQD].prchar;
			break;

		case DRCH_X:		/* crossing lines		*/
			i = drchtbl[DRQE].prlen;
			p = (char *) drchtbl[DRQE].prchar;
			break;

		case DRCH_H:		/* horizontal line		*/
			i = drchtbl[DRQF].prlen;
			p = (char *) drchtbl[DRQF].prchar;
			break;

		case DRCH_LT:		/* left 	'T'		*/
			i = drchtbl[DRQG].prlen;
			p = (char *) drchtbl[DRQG].prchar;
			break;

		case DRCH_RT:		/* right 	'T'		*/
			i = drchtbl[DRQH].prlen;
			p = (char *) drchtbl[DRQH].prchar;
			break;

		case DRCH_BT:		/* bottom 	'T'		*/
			i = drchtbl[DRQI].prlen;
			p = (char *) drchtbl[DRQI].prchar;
			break;

		case DRCH_TT:		/* top 		'T'		*/
			i = drchtbl[DRQJ].prlen;
			p = (char *) drchtbl[DRQJ].prchar;
			break;

		case DRCH_V:		/* vertical bar			*/
			i = drchtbl[DRQK].prlen;
			p = (char *) drchtbl[DRQK].prchar;
			break;
		}
		while (i > 0)
		{
			*prbuf++ = *p++;
			i--;
			outlen++;
		}
	    }
	    else
	    {
		CMcpychar(inbuf, prbuf);
		CMbyteinc(outlen, prbuf);
		CMnext(prbuf);
	    }

	    CMbytedec(len, inbuf);
	    CMnext(inbuf);
	}

	if (len < 0)
	{
		prbuf--;
		outlen--;
		dbpflg = 1;
	}

	*prbuf = '\0';

	return (outlen);
}
