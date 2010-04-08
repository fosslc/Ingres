# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<cm.h>
# include	<me.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <er.h> 
# include        <ug.h>
# include        <errw.h>
# include	 <rtype.h> 
# include	 <rglob.h> 



/*
**      rxctrl.c - Maintenance routines to manage the CTRL link list.
**
**      Copyright (c) 2004 Ingres Corporation
**      All rights reserved.
*/


/*
** Name: IIRWpct_put_ctrl
**
** Description:
**	Puts a CTRL block into a sorted linked list.  CTRL blocks are sorted
**	based on the sequence number (cr_number).  If the sequence number
**	already exists in the list, put current block AFTER all blocks of
**	the same number.
**
** History:
**	27-sep-1989 (sylviap)
**		Initial reversion.
**	23-feb-1993 (rdrane)
**		Re-parameterize to accept full LN structure.  We
**		now track q0 sequences on a per-line basis (46393 and
**		46713).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

VOID
IIRWpct_put_ctrl(cblk,cur_ln)
CTRL	*cblk;
LN	*cur_ln;
{
	CTRL *head;
	CTRL *tail;

	/* Is this the first control sequence? */
	if (cur_ln->ln_ctrl == (CTRL *)NULL)
	{
		cur_ln->ln_ctrl = cblk;
		cblk->cr_below = (CTRL *)NULL;
	}
	else
	{
		/* 
		** Insert control block into a sorted list.  If the cr_number
		** already exists, put current block AFTER all the blocks
		** with the same number.
		*/
		head = cur_ln->ln_ctrl;
		if (head->cr_number > cblk->cr_number)
		{
			cur_ln->ln_ctrl = cblk;
			cblk->cr_below = head;
		}
		else
		{
			tail = head;
			head = tail->cr_below;
			while ((head != (CTRL *)NULL) &&
			       (head->cr_number <= cblk->cr_number))
			{
			   	tail = head; 
			   	head = tail->cr_below;
			}
			tail->cr_below = cblk;
			cblk->cr_below = head;
		}
	}

	return;
}	


/*
** Name: IIRWmct_make_ctrl
**
** Description:
**	Returns a pointer to a string that contains both the printable 
**	characters and the control sequences.  First it determines how much
**	memory it needs by finding the total length of the string and all
**	the control sequences.  Then it steps through each character in the 
**	printable string, inserting a control sequence based on the cr_number.
**
**
** History:
**	27-sep-1989 (sylviap)
**		Initial reversion.
**	3/23/90 (elein) performance
**		Remove len parameter.  Do STlength here instead of
**		in calling function to get length.  This is the less
**		traveled path.
**	23-feb-1993 (rdrane)
**		Re-parameterize to accept full LN structure.  We
**		now track q0 sequences on a per-line basis (46393 and
**		Recognize a fatal error if the t_line buffer allocation fails.
**	19-may-1993 (rdrane)
**		Re-parameterize call to IIRWmct_make_ctrl() include explicit
**		parameter in which to return the computed total length of the
**		resultant line.  This is to ensure that a control sequence
**		containing an embedded ASCII NUL does not prematurely end the
**		line (clients were using STlength() to compute the resultant
**		line's length).  For this same reason, replace instances of
**		STlcopy() with MEcopy().
*/

char *
IIRWmct_make_ctrl(str,cur_ln,t_len)
char	*str;			/* printable line text */
LN	*cur_ln;		/* line's control structure */
i4	*t_len;			/*
				** pointer to field to receive length of
				** resultant line, including any control
				** sequences
				*/
{
	char	*t_line;	/* total line, including control sequence */
	char	*tmp_line;	/* temp line, including control sequence */
	char	*cp;		/* printable string character pointer */
	i4	len;		/* length of string passed in */
	i4	t_count;	/* tracks the position */
	i4	p_count;	/* tracks position in the printable string */
	CTRL	*t_ctrl;	/* temp ctrl block */


	/*
	** if no control sequence, then return str
	** as is, and compute length using STlength()
	*/
	if (cur_ln->ln_ctrl == (CTRL *)NULL)
	{
		*t_len = STlength(str);
		return (str);
	}

	/* 
	** Find the total length of the line including printable and 
	** non-printable characters.  Go through linked list to get the total
	** lengths of all the control sequences.  Then add the greater of 
	** either the length of the printable string (len) or the position 
	** of the last control sequence (t_count).  If the control sequence 
	** is greater than the length, then the string will need to be 
	** blank padded from the end of the printable string to the position 
	** of the last control sequence.
	*/
	t_ctrl = cur_ln->ln_ctrl;	/* start at head */
	*t_len = 0;
	while (t_ctrl != (CTRL *)NULL)
	{
		*t_len += t_ctrl->cr_len;  
		t_count = t_ctrl->cr_number;		/* get position */
		t_ctrl = t_ctrl->cr_below;		/* go to next one */
	}
	len = STlength(str);
	*t_len += max(len, t_count);

	/*
	** Reset our tmp CTRL pointer and allocate/clear space for
	** total line.  Even though we don't need to NULL terminate the string,
	** STcopy() may add one ...
	*/
	t_ctrl = cur_ln->ln_ctrl;
	if  ((t_line = FEreqmem(t_ctrl->cr_tag,(u_i4)((*t_len) + 1),
				TRUE,(STATUS *)NULL)) == NULL)
        {
            IIUGbmaBadMemoryAllocation(ERx("IIRWmct_make_ctrl - t_line"));
        }

	cp = str;
	p_count = 0;
	tmp_line = t_line;
	while (*cp)
	{
		if (t_ctrl->cr_number <=  p_count)
		{
			/* need to copy a control sequence */
			MEcopy ((PTR)t_ctrl->cr_buf, (u_i2)t_ctrl->cr_len,
				(PTR)tmp_line);
			tmp_line += t_ctrl->cr_len;
			t_ctrl = t_ctrl->cr_below;
			if (t_ctrl == (CTRL *)NULL)
			{
				/* 
				** No more control sequences, so just copy
				** the rest of the string 
				*/
				STcopy(cp, tmp_line);
				break;
			}
		}
		else
		{
			p_count += CMbytecnt(cp);
			CMcpyinc(cp, tmp_line);
		}
	}
	/* 
	** Check if any control sequences go beyond the printable string.  If 
	** so, then need to pad with blanks.
	*/
	while (t_ctrl != (CTRL *)NULL)
	{
		/* fill in blanks */
		t_count = t_ctrl->cr_number - p_count;
		if (t_count > 0)
		{
			MEfill (t_count, ' ', tmp_line);
			tmp_line += t_count;
			p_count += t_count;
		}

		/* copy control sequence */
		MEcopy ((PTR)t_ctrl->cr_buf, (u_i2)t_ctrl->cr_len,
			(PTR)tmp_line);
		tmp_line += t_ctrl->cr_len;
		p_count += t_ctrl->cr_len;
		t_ctrl = t_ctrl->cr_below;
	}

	return (t_line);
}

