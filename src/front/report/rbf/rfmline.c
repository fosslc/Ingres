/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rfmline.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include       "rbftype.h"
# include       "rbfglob.h"

/*
**   RFM_LINE - generate a .PRINT statement to print out a
**	line of repeating characters.  If the length is too
**	big generate several .PRINT commands.  End with a .NEWLINE.
**
**	Parameters:
**		start	starting position.  .TAB to this.
**		stop	ending position. (line from start - stop - 1).
**		ch	character to use for line.
**
**	Returns:
**		none.
**
**	Called by:
**		rFm_foot, rFm_pbot, rFm_rfoot.
**
**	Side Effects:
**		none.
**
**	History:
**		9/12/82 (ps)	written.
**      9/22/89 (elein) UG changes ingresug change #90045
**	changed <rbftype.h> & <rbfglob.h> to "rbftype.h" & "rbfglob.h"
**	2/20/90 (martym) Changed to return STATUS passed back by the 
**		     SREPORTLIB routines.
**      2-nov-1992 (rdrane)
**		Remove all FUNCT_EXTERN's since they're already
**		declared in included hdr files.  Ensure that all
**		string constants are generated with single, not double quotes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      06-Mar-2003 (hanje04)
**          Cast last '0' in calls to s_w_action() with (char *) so that
**          the pointer is completely nulled out on 64bit platforms.
*/


# define	MAXSTR	50	/* maximum size of single string */


STATUS rFm_line(start,stop,ch)
i4	start;
i4	stop;
char	ch;
{
	/* internal declarations */

	char	ibuf[20];
	char	cbuf[MAXSTR+1];
	register i4	i,j;		/* fast counters */
	STATUS 		stat = OK;

	/* start of routine */

	if ((stop - start) < 0)
	{
		return(OK);
	}

	CVna(start, ibuf);
	RF_STAT_CHK(s_w_action(ERx("tab"), ibuf, (char *)0));

	/* set up print statements for the line */

	MEfill(MAXSTR+1, '\0', cbuf);		/* clear out buffer */
	j = 0;					/* counter of cbuf chars */
	for(i=stop-start+1; i>0; i--) 
	{	/* need another char to string */
		if (j < MAXSTR)
		{
			cbuf[j++] = ch;
		}
		else
		{	/* won't fit. Write this one out */
			RF_STAT_CHK(s_w_action(ERx("print"), 
				ERx("\'"), cbuf, ERx("\'"), (char *)0));
			MEfill(MAXSTR+1, '\0', cbuf);
			j = 0;
		}
	}

	/* No more characters.  Write out the last if it has any in it */

	if (j>0)
	{
		RF_STAT_CHK(s_w_action(ERx("print"), ERx("\'"), 
			cbuf, ERx("\'"), (char *)0));
	}
	RF_STAT_CHK(s_w_action(ERx("endprint"),(char *)0));

	RF_STAT_CHK(s_w_action(ERx("newline"), ERx("1"), (char *)0));

	return(OK);
}
