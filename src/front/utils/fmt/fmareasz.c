
/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<adf.h>
# include	<afe.h>
# include	<fmt.h>
# include	"format.h"
# include	<cm.h>
# include	<st.h>

/**
** Name:	fmareasz.c	This file contains the routine that returns
**				the size of the area needed to build a FMT
**				structure.
**
** Description:
**	This file defines:
**
**	fmt_areasize		Returns the size of the area needed
**				to build a FMT structure.
**
** History:
**	19-dec-86 (grant)	implemented.
**	08/16/91 (tom) support for input masking of dates, and string templates
**   
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */
/* extern's */
/* static's */

/*{
** Name:	fmt_areasize	-    Returns the size of the area needed
**				     to build a FMT structure.
**
** Description:
**	Given a format string, this routine returns the size
**	of the memory area needed to build a FMT structure.
**
** Inputs:
**	cb		Points to the current ADF_CB
**			control block.
**
**	fmtstr		The format string for which to build a FMT structure.
**
**	size		Must point to a nat.
**
** Outputs:
**	size		Will point to an allocated FMT structure that
**			corresponds to the format string.
**
**	Returns:
**		E_DB_OK		If it completed successfully.
**		E_DB_ERROR	If some error occurred.
**
**		If an error occurred, the caller can look in the field
**		cb->adf_errcb.ad_errcode to determine the specific error.
**		The following is a list of possible error codes that can be
**		returned by this routine:
**
**		E_FM6000_NULL_ARGUMENT		One of the pointer parameters
**						to this routine was NULL.
** **
** History:
**	19-dec-86 (grant)	implemented.
**	9/8/89 (elein)
**		Add single quote recognition
**	08/16/91 (tom) support for input masking of dates, and string templates
** 12/23/91 (elein) 41634
**    Date and String templates are valid with capital and lower case 'D'
**    and 'S' and may have white space between the letter and the quoted
**    template. fsp[-1] (sic) referenced the space before the quote and
**    not the 'd' for date templates. Capitals weren't tested for at all.
**    This caused some of the date and string templates to not have the
**    width set properly to include the input masking requirements.  
**    Inadequate space was then allocated, and subsequently overrun.
**    This was causing obscure memory allocation problems later in the
**    (this case) report.
** 12/23/91 (elein) 41634 addendum
**    going too fast...Tom points out that there can be a leading alignment
**    character...rework the way we decide if it is a date or string template.
** 19-mar-1998 (rodjo04)
**    Bug 89793  Strings can begin with either a single quote or double 
**    quote and therefore we must check to see which is the first. We must do 
**    this inorder to allocate enough space for FMT.
*/

STATUS
fmt_areasize(cb, fmtstr, size)
ADF_CB		*cb;
char		*fmtstr;
i4		*size;
{
    char	*fsp;
	char    *fsptmp; 
    char	*tptr;
    i4		width;
    i4		ccsize = 0;

    if (fmtstr == NULL || size == NULL)
    {
	return afe_error(cb, E_FM6000_NULL_ARGUMENT, 0);
    }

    *size = sizeof(FMT);

    fsp = STindex(fmtstr, ERx("\""), 0);
	fsptmp = STindex(fmtstr, ERx("\'"), 0);
    if ( fsp == NULL )
    {
		if ( fsptmp != NULL)
			fsp = fsptmp;
	}
	else
	{
		if (fsptmp != NULL)
			if (fsp > fsptmp)
				fsp = fsptmp;
	}
    if (fsp != NULL)	/* found start of template */
    {
	/*
	** Add length of template to area size.
	** The delimiting quotes aren't included, but we need one extra char
	** for EOS and another char in case the template doesn't have the
	** terminating quote.
	*/

	*size += STlength(fsp);

	/* if we are not at the start of the buffer, then it could be
	   a string or absolute date template... if so we see if we
	   have a template we could use for input masking */

	for( tptr = fmtstr; tptr < fsp; CMnext(tptr))
	{
		if (  (*(tptr) == 'd' || *(tptr) == 'D' )
		&& f_dtcheck(fsp + 1, &width) == TRUE)
		{
			/* size required includes 2 fmt_width sized mask arrays 
			emask = 1 char per position, cmask = 2 char per pos. */
			*size += width * 3;
			break;
		}
		else if (  (*(tptr) == 's' || *(tptr) == 'S' )
		&& f_stcheck(fsp + 1, &width, &ccsize) == OK)
		{
			/* size required includes 2 fmt_width sized mask arrays,
		   	emask = 1 char per position, cmask = 2 char per pos.,
		   	and an array of pointers to character class definitions
		   	that may have been specified inside the template. */
			*size += width * 3 + ccsize * sizeof(char*) 
				+ sizeof(ALIGN_RESTRICT);
			break;
		}
	}
   }
   return OK;
}
