/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<si.h> 
# include	<adf.h> 
# include	<fmt.h> 
# include	<rcons.h> 
# include	<rtype.h> 
# include	<rglob.h> 
# include	<er.h>

/*
**   R_WRITELN - write out the text string to the referenced file
**	followed by an end of line.
**
**	Parameters:
**		fs - file pointer of file to write to.
**		line - character string to write, ended by end-of-string.
**			If this is NULL, only an end of line is written.
**		cur_ln - Current line's line structure.  This may be NULL.
**	Returns:
**		-1 if bad write, in some way or other.
**
**	Error Messages:
**		none.
**
**	Trace Flags:
**		2.0, 2.13.
**
**	History:
**		6/1/81 (ps) - written.
**		4/11/83(mmm)- changed to use file pointers
**		02-aug-89 (sylviap)
**			If output is the terminal, then do scrollable output.
**		02-oct-89 (sylviap)
**			Added control character sequence support.
**		20-oct-89 (sylviap)
**			Changed FSaddrec call to IIUFadd_AddRec.
**		3/23/90 (elein) performance
**			Remove STlength call and len param to IIRWmct_make_ctr
**		06-dec-90 (sylviap)
**			Only copies the printable string in En_buffer.
**			#34667
**		23-feb-1993 (rdrane)
**			Re-parameterize to accept full LN structure.  We
**			now track q0 sequences on a per-line basis (46393 and
**			46713).  Remove declaration of IIRWmct_make_ctrl() -
**			moved to the hdr file.
**		19-may-1993 (rdrane)
**			Re-parameterize calls to r_wrt() to include explicit
**			length of line to be written, and ensure that we
**			compute that length here.  This is to ensure that
**			a control sequence containing an embedded ASCII NUL
**			does not prematurely end the line.
**		16-april-1997(angusm)
**		We may have many CTRL structures in our list.
**		For reports with many q0 items, this can lead
**		to range of tag ids being exhausted (bug 68456).
**		Call IIUGtagFree() to free tag ids for reuse.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



i4
r_wrt_eol(fp,line,cur_ln)
FILE	*fp;
char	*line;
LN	*cur_ln;

{
	/* external declarations */


	/* internal declarations */

	i4		len;			/* length of line */
	char		*t_line;		/*
						** Line to be printed; has 
						** control sequence if any
						*/

	/* start of routine */


	/* 
	** If any control sequences, then create line with both 
	** printable characters and the control sequences.
	*/
	if ((cur_ln != (LN *)NULL) && (cur_ln->ln_ctrl != (CTRL *)NULL))
	{
		t_line = IIRWmct_make_ctrl(line,cur_ln,&len);		
	}
	else
	{
		t_line = line;
		len = 0;
		if  (line != NULL)
		{
			len = STlength(line);
		}
	}
	
        if (St_to_term)
	{
	 	/* If output is the terminal, then do scrollable output */
	 	IIUFadd_AddRec (En_bcb, t_line, TRUE);

		/* 
		** Pass back the buffer of printable characters
		** because buffer of both printable and non-printable characters
		** (t_line) could exceed En_buffer (En_buffer is En_lmax long). 
		**
		** En_buffer is used to figure out how long the string is, so
		** should only pass back printable characters.(#34667)
		*/
		STcopy (line, En_buffer);
		En_rows_added++;
	}
	else
	{
		if (t_line != NULL)
		{	/* put out the string first */
			if ((len = r_wrt(fp,t_line,len)) < 0)
			{
				return(-1);
			}
		}
		if (r_wrt(fp,ERx("\n"),1) < 0)
		{
			return(-1);
		}
	}

	/*
	** Deallocate any control sequence blocks
	*/
	if ((cur_ln != (LN *)NULL) && (cur_ln->ln_ctrl != (CTRL *)NULL))
	{
		IIUGtagFree(cur_ln->ln_ctrl->cr_tag);
		cur_ln->ln_ctrl = (CTRL *)NULL;
	}

	return(len);
}
