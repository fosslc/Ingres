/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 
# include	<er.h>

/*
**   R_SORT_SET - set up the sort variable array.  This processes the
**	SORT lines for this report from the RCOMMANDS relation to fill
**	in the ordinal items.
**
**	It also sets up the En_srt_list if a sort list is specified.
**
**	Parameters:
**		none.
**
**	Returns:
**		FALSE	if error found in sort list.
**		TRUE	if no error found. 
**
**	Side Effects:
**		Sets up an array of SORT structures, pointed to in PTR struct.
**		Sets En_n_sorted field and En_srt_list string.
**
**	Called by:
**		r_rep_load.
**
**	Error messages:
**		6.
**
**	Trace Flags:
**		1.0, 1.7.
**
**	History:
**		3/13/81 (ps)	written.
**		12/29/81 (ps)	modified for two table version.
**		8/29/82 (ps)	add RBF break element.
**		11/30/82 (ps)	add calculation of En_srt_list.
**		8/29/83 (gac)	bug 889 fixed -- now no A.V. after error 7006.
**		12/20/84 (rlm)	SORT / SRT structures merged, made dynamic
**				changed to modify existing array rather than
**				copy one array to another
**		1/8/85 (rlm)	tagged memory allocation
**		7-oct-1992 (rdrane)
**			Converted r_error() err_num value to hex to facilitate
**			lookup in errw.msg file.  Allow for delimited
**			identifiers and normalized them in the sort
**			variable arrays.  Remove unused local variable ibuf.
**		9-dec-1993 (rdrane)
**			Oops!  The sort attribute name is already stored as 
**			normalized (b54950).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



bool
r_srt_set()
{
	/* internal declarations */

	register SORT	*srt;		/* fast ptr to SORT array */
	i4		sequence;	/* sort sequence number */
	ATTRIB		ordinal;	/* attribute ordinal */
	bool		noerr = TRUE;	/* abort if error in sort vars */

	/* start of routine */

	En_n_sorted = En_scount;

	if (En_n_sorted < 1)
	{	/* no sort attributes specified */
		En_n_sorted = 0;
		return(noerr);
	}	/* no sort attributes specified */

	/* go through the database lines and convert */

	for(sequence=1,srt=Ptr_sort_top; sequence<=En_n_sorted; srt++,sequence++)
	{
		CVlower(srt->sort_break);
		CVlower(srt->sort_direct);


		if ((ordinal=r_mtch_att(srt->sort_attid)) < 0)
		{	/* attribute not in data relation */
			r_error(0x06, NONFATAL, srt->sort_attid, NULL);
			noerr = FALSE;
			continue;
		}	/* attribute not in data relation */

		srt->sort_ordinal = ordinal;
	}


	if (!noerr)
	{
		En_n_sorted = 0;
		return(noerr);
	}

	return(noerr);
}
