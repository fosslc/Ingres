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
** Name:	ITlndraw	- convert to graphics characters
**
** Description:
**	This function converts the internal codes in the input buffer 
**	to the drawing line (graphics characters).
**
** Inputs:
**	inbuf	input buffer that is represented by internal code.
**	len	length of inbuf.
**
** Outputs:
**	prbuf	output buffer replaced by ESC sequence.
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
**	04/14/90 (dkh) - Tuned up code to run faster for TM.
**
**	15-nov-91 (leighb) DeskTop Porting Change:
**		Check that inbuf char is not = DRCH_SEC (section marker).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# define INCBUF(c, buf, len)	(buf += drchtbl[c].drlen, len += drchtbl[c].drlen)

static	i4	lse_flag = 0;

i4
ITlndraw(inbuf, prbuf, len)
register char	*inbuf;
register char	*prbuf;
i4	len;
{
	register i4	outlen = 0;
	register i4	i;
	register char	*p;
	register i4	lseflg = lse_flag;

	if (dblflg == 1)
	{
		*prbuf++ = *inbuf++;
		outlen++;
		len--;
		dblflg = 0;
	}

	while (*inbuf && len > 0)
	{
	    if ((*inbuf >= DRCH_LR && *inbuf <= DRCH_V) 		 
		&& (*inbuf != DRCH_SEC))				 
	    {
		if (lseflg == 0)
		{
			/*
			** interpret subseq. chars for drawing lines 
			STcopy(drchtbl[DRLS].drchar, prbuf);
			INCBUF(DRLS, prbuf, outlen);
			*/
			i = drchtbl[DRLS].drlen;
			p = (char *) drchtbl[DRLS].drchar;
			while (i > 0)
			{
				*prbuf++ = *p++;
				i--;
				outlen++;
			}
			lseflg++;
		}

		switch (*inbuf)
		{
		case DRCH_LR:		/* lower-right corner of a box	*/
			i = drchtbl[DRQA].drlen;
			p = (char *) drchtbl[DRQA].drchar;
			break;

		case DRCH_UR:		/* upper-right corner of a box	*/
			i = drchtbl[DRQB].drlen;
			p = (char *) drchtbl[DRQB].drchar;
			break;

		case DRCH_UL:		/* upper-left corner of a box	*/
			i = drchtbl[DRQC].drlen;
			p = (char *) drchtbl[DRQC].drchar;
			break;

		case DRCH_LL:		/* lower-left corner of a box	*/
			i = drchtbl[DRQD].drlen;
			p = (char *) drchtbl[DRQD].drchar;
			break;

		case DRCH_X:		/* crossing lines		*/
			i = drchtbl[DRQE].drlen;
			p = (char *) drchtbl[DRQE].drchar;
			break;

		case DRCH_H:		/* horizontal line		*/
			i = drchtbl[DRQF].drlen;
			p = (char *) drchtbl[DRQF].drchar;
			break;

		case DRCH_LT:		/* left 	'T'		*/
			i = drchtbl[DRQG].drlen;
			p = (char *) drchtbl[DRQG].drchar;
			break;

		case DRCH_RT:		/* right 	'T'		*/
			i = drchtbl[DRQH].drlen;
			p = (char *) drchtbl[DRQH].drchar;
			break;

		case DRCH_BT:		/* bottom 	'T'		*/
			i = drchtbl[DRQI].drlen;
			p = (char *) drchtbl[DRQI].drchar;
			break;

		case DRCH_TT:		/* top 		'T'		*/
			i = drchtbl[DRQJ].drlen;
			p = (char *) drchtbl[DRQJ].drchar;
			break;

		case DRCH_V:		/* vertical bar			*/
			i = drchtbl[DRQK].drlen;
			p = (char *) drchtbl[DRQK].drchar;
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
		/*
		**  Process regular characters.  First set back
		**  to normal character set if necessary.
		*/
		if (lseflg != 0)
		{
			/*
			** interpret subseq. chars as regular chars
			STcopy(drchtbl[DRLE].drchar, prbuf);
			INCBUF(DRLE, prbuf, outlen);
			*/
			i = drchtbl[DRLE].drlen;
			p = (char *) drchtbl[DRLE].drchar;
			while (i > 0)
			{
				*prbuf++ = *p++;
				i--;
				outlen++;
			}
			lseflg = 0;
		}

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
		dblflg = 1;
	}

	*prbuf = '\0';

	lse_flag = lseflg;

	return (outlen);
}
