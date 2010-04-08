# include	<compat.h>  
# include	<gl.h>
# include	<me.h>  
# include	<lo.h>  
# include	"lolocal.h"

/*LOlist
**
**	LOlist will return  the next file or directory in the passed in
**	location.  The initial call gets the first file -- subsequent calls
**	return successive locations.   LOlist will return OK as long as there
**	was a file to return.  If one wishes to stop retrieving locations
**	before the end call LOendlist.
**
**	Parameters:
**
**		inputloc: pointer to location from which to search for files.
**		outputloc:pointer to location where retrieved location is stored.
**
**	Returns:
**		LOlist returns OK as long as there was another file to be found.
**
**	Called by:
**
**	Side Effects:
**		First call opens a file pointer associated with the inputed
**		locations directory file.  Subsequent calls use this file pointer
**		to get the next location.
**
**	Trace flags:
**
**	Error Returns:
**		Returns non zero on error.
**
**	History:
**
**		4/1/83 -- (mmm) written
**		9/26/83 -- (dd) VMS CL
**		10/17/89 (Mike S) Write in terms of LOwcard
**		18-jun-93 (ed) removed status return of LOcopy
**      16-jul-93 (ed)
**	    added gl.h
**	07-feb-01 (kinte01)
**	    Add casts to quiet compiler warnings
*/

STATUS
LOlist(inputloc, outputloc)
register LOCATION	*inputloc;
register LOCATION	*outputloc;
{
	STATUS status;
	
	if (inputloc->wc == NULL)
	{
		/* First time through -- allocate context structure */
		LOcopy(inputloc, outputloc->string, outputloc);
		inputloc->wc = (LO_DIR_CONTEXT *)
				MEreqmem(0, sizeof(LO_DIR_CONTEXT), FALSE,NULL);
		if (inputloc->wc == NULL) return (FAIL);

		status = LOwcard(outputloc, NULL, NULL, NULL, inputloc->wc);
		if (status != OK) LOendlist(inputloc);
		return(status);
	}
	else
	{
		/* We've been here before */
		status = LOwnext(inputloc->wc, outputloc);
		if (status != OK) LOendlist(inputloc);
		return (status);
	}
}

/*
 *	Copyright (c) 1983 Ingres Corporation
 *
 *	Name:LOendlist
 *
 *	Function:
 *		LOendlist is called in conjunction with LOlist.
 *		LOendlist is called when one wishes to end a directory
 *		search before LOlist reaches end of file.
 *
 *	Arguments:
 *		loc : a pointer to the location whose directory LOlist is
 *		      is searching.
 *
 *	Result:
 *		The file pointer opened in order to do the list is closed.
 *
 *	Side Effects:
 *
 *	History:
 *		4/4/83 -- (mmm) written
 *		9/26/83 -- (dd)  VMS CL
**		23-jun-84 (fhc) -- fixed problem where sys$search keeps channels accross
**			the network if sys$parse is not called with the same fab and nam
**		4/19/89 (Mike S) -- Use LOclean_fab
**		10/89	(Mike S) -- Write in terms of LOwend
 *
 */
LOendlist(loc)
register LOCATION	*loc;
{
	STATUS status = OK;

        if (loc->wc != NULL)
	{
		status = LOwend(loc->wc);
		_VOID_ MEfree((PTR)loc->wc);
		loc->wc = NULL;
	}
	return (status);
}
