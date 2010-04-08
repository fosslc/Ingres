/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rcntlines.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 

/*
**   R_CNT_LINES - count the number of lines in a linked list of TCMD
**	structures, by counting up the NEWLINE commands, and ignoring
**	the rest. Note the special consideration of underlined lines
**	and any commands within the .BLOCK commands.
**
**	Parameters:
**		tcmd - start of linked list of TCMD structures.
**
**	Returns:
**		number of newlines found in list.
**
**	Side Effects:
**		none.
**
**	Called by:
**		r_set_dflt.
**
**	Trace Flags:
**		2.0, 2.9.
**
**	History:
**		4/14/81 (ps) - written.
**		5/10/83 (nl) - added changes to fix bug #937:
**				changed P_CATT and P_PATT references 
**				to P_ATT.
**		12/13/83 (gac)	added IF statement.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**       1-Apr-2009 (hanal04) Bug 135519
**          Return 0 lines from r_cnt_lines() if tcmd is NULL. Reset
**          stale counters from the previous call to avoid unexpected
**          cumulative count results.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


/* external declarations */

static i4	count = 0;		/* temp count of lines */
static bool	extra = FALSE;		/* TRUE if extra line when
					** underlining	*/
static bool	ul_on = FALSE;		/* TRUE if underlining on */
static bool	ln_type = 0;		/* code for type of line:
					**  0 - nothing written.
					**  1 - something written.
					**  2 - written and underlined
					**      on dif line.  */
static i4	maxcount = 0;		/* save old max count for use
					** with .WITHIN commands */
static i4	curcount = 0;		/* temporary count of lines */
static i4	blkcount = 0;		/* block nesting */
static VOID	count_loop();


i4
r_cnt_lines(tcmd)
register TCMD	*tcmd;

{
	/* start of routine */
        if(tcmd == (TCMD *)NULL)
            return(0);

        count = 0;
        extra = FALSE;
        ul_on = FALSE;
        ln_type = 0;
        maxcount = 0;
        curcount = 0;
        blkcount = 0;

	extra = (St_ulchar == '_') ? FALSE : TRUE;

	count_loop(tcmd, (TCMD *)NULL);

	if (ln_type != 0)
	{	/* didn't finish last line */
		curcount++;
	}

	if (maxcount>curcount)
	{
		curcount = maxcount;
	}
	count += curcount;


	return(count);

}

/*
**	COUNT_LOOP - Actual loop to scan through TCMD structure, counting lines.
**		It is recursively called in order to handle IF statements.
**
**	Parameters:
**		tcmd		start of linked list of TCMD structures.
**		end_tcmd	end of linked list of TCMD structures.
*/

VOID
count_loop(tcmd, end_tcmd)
TCMD	*tcmd;
TCMD	*end_tcmd;
{
	IFS		*ifs;

	/* previous values of external variables */
	i4		p_count;
	bool		p_extra;
	bool		p_ul_on;
	bool		p_ln_type;
	i4		p_maxcount;
	i4		p_curcount;
	i4		p_blkcount;

	/* values for left ("then") list of actions in IF statement */
	i4		l_count;
	bool		l_extra;
	bool		l_ul_on;
	bool		l_ln_type;
	i4		l_maxcount;
	i4		l_curcount;
	i4		l_blkcount;

	for (; tcmd != end_tcmd; tcmd = tcmd->tcmd_below)
	{
		switch(tcmd->tcmd_code)
		{
			case(P_UL):
				ul_on = TRUE;
				break;

			case(P_BLOCK):
				blkcount++;
				break;

			case(P_END):
				switch(tcmd->tcmd_val.t_v_long)
				{
					case(P_UL):
						ul_on = FALSE;
						break;

					case(P_BLOCK):
						if (--blkcount<=0)
						{
							if (curcount>maxcount)
							{
								maxcount = curcount;
							}
							count += maxcount;
							maxcount=curcount=0;
						}
						break;

					default:
						break;
				}
				break;

			case(P_PRINT):
				/* something printed */
				ln_type=((ul_on&&extra)||(ln_type>1)) ? 2 : 1;
				break;

			case(P_ULC):
				extra = (tcmd->tcmd_val.t_v_char=='_') ?
						FALSE : TRUE;
				break;

			case(P_NEWLINE):
				curcount += tcmd->tcmd_val.t_v_long;
				if (ln_type>1)
				{	/* add one line for underlining */
					curcount++;
				}
				ln_type = 0;
				break;

			case(P_TOP):
				if (ln_type != 0)
				{	/* didn't finish last line */
					curcount++;
				}
				if (curcount>maxcount)
				{
					maxcount = curcount;
				}
				curcount = 0;
				ln_type = 0;
				break;

			case(P_IF):
				/* save previous values of external variables */
				p_count = count;
				p_extra = extra;
				p_ul_on = ul_on;
				p_ln_type = ln_type;
				p_maxcount = maxcount;
				p_curcount = curcount;
				p_blkcount = blkcount;

				ifs = tcmd->tcmd_val.t_v_ifs;
				count_loop(ifs->ifs_then, tcmd->tcmd_below);

				/* save left ("then") values of external variables */
				l_count = count;
				l_extra = extra;
				l_ul_on = ul_on;
				l_ln_type = ln_type;
				l_maxcount = maxcount;
				l_curcount = curcount;
				l_blkcount = blkcount;

				/* restore the previous values of external variables */
				count = p_count;
				extra = p_extra;
				ul_on = p_ul_on;
				ln_type = p_ln_type;
				maxcount = p_maxcount;
				curcount = p_curcount;
				blkcount = p_blkcount;

				count_loop(ifs->ifs_then, tcmd->tcmd_below);

				count = max(count, l_count);
				extra = extra || l_extra;
				ul_on = ul_on || l_ul_on;
				ln_type = max(ln_type, l_ln_type);
				maxcount = max(maxcount, l_maxcount);
				curcount = max(curcount, l_curcount);
				blkcount = min(blkcount, l_blkcount);

				tcmd = tcmd->tcmd_below;
				break;

			default:
				break;
		}
	}

	return;
}
