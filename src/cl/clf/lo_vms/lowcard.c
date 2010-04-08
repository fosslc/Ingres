/*
**    Copyright (c) 1989, 2000 Ingres Corporation
*/

# include 	<compat.h>  
# include	<gl.h>
# include	<descrip.h>  
# include	<rms.h> 
# include	<lib$routines.h> 
# include	<st.h>  
# include	<lo.h>  
# include 	<er.h>  
# include	"lolocal.h"

/*LOwcard
**
**	LO wildcard operations.
**
** 	Defines procedures: 
**		LOwcard		-	Start wildcard operation
**		LOwnext		-	Get next file
**		LOwend		-	End of wildcard operation
**
**		History
**			10/89 -- Initial version
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
*/

# define WILD ERx("*")


/*{
** Name:	LOwcard		- 	Start wildcard search
**
** Description:
**	Do a wildcard search of a directory.
**
** Inputs:
**	loc	LOCATION *	NODE and PATH to search.
**	fprefix	char *		filename to look for; NULL to wildcard.
**	fsuffix char *		filetype to look for; NULL to wildcard.
**	version	char *		version to look for; NULL to wildcard.
**
** Outputs:
**	loc	LOCATION *	First file in list
**	lc	LO_DIR_CONTEXT * Directory context for search
**
**	Returns:
**		STATUS	OK or failure.
**
**	Exceptions:
**		none.
**
** Side Effects:
** 		May allocate an I/O channel and RMS resources.
**		LOwend MUST be called to deallocate them.
** History:
**	10/89	(Mike S)	Initial version.
*/
STATUS
LOwcard(loc, fprefix, fsuffix, version, lc)
LOCATION	*loc;
char	*fprefix;
char	*fsuffix;
char	*version;
LO_DIR_CONTEXT	*lc;
{
	i4	length;

	/* Construct the string we will wildcard check */
	if((length = loc->nodelen + loc->devlen + loc->dirlen) == 0)
		return (LO_NULL_ARG);
        STlcopy(loc->string, lc->filename, length);
	
	STpolycat(5,
		  (fprefix == NULL) ? WILD : fprefix,
		  DOT,
		  (fsuffix == NULL) ? WILD : fsuffix,
		  SEMI,
		  (version == NULL) ? WILD : version,
		  lc->filename + STlength(lc->filename));
	
	/* Begin the wildcard search */
	lc->context = 0;
	lc->node = (loc->nodelen > 0);
	return (LOwnext(lc, loc));
}

/*{
** Name:	LOwnext		-	Get next wildcard <proc name>	- <short comment>
**
** Description:
**	Get next file from wildcard search.
**
** Inputs:
**	lc	LOCATION *	search context block
**
** Outputs:
**	loc	LO_DIR_CONTEXT * next location from search
**
**	Returns:
**		STATUS OK or failure
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	10/89 (Mike S)	Initial version
*/
STATUS
LOwnext(lc, loc)
LO_DIR_CONTEXT	*lc;
LOCATION	*loc;
{
	STATUS	status;
	$DESCRIPTOR(file_desc, ERx(""));
	char	result[MAX_LOC];
	$DESCRIPTOR(result_desc, result);
	LOCTYPE	what;

	/* Do the search */
	file_desc.dsc$a_pointer = lc->filename;
	file_desc.dsc$w_length = STlength(lc->filename);
	status = lib$find_file(&file_desc, &result_desc, &lc->context);

	if ((status &1) != 0)
	{
		/* It worked.  Make a new location */
		if (lc->node)
			what = NODE & PATH & FILENAME;
		else
			what = PATH & FILENAME;
		/* This shouldn't fail */
		result[MAX_LOC-1] = EOS;
		_VOID_ STtrmwhite(result);
		STcopy(result, loc->string);
		if ((status = LOfroms(what, loc->string, loc)) != OK)
		{
			_VOID_ LOwend(lc);
			return (status);
		}
		return (OK);
	}
	else
	{	
		/* It didn't work.  Clean up and return a useful status */
		_VOID_ LOwend(lc);
		switch (status)
		{
		    case RMS$_NMF:	/* No more files found */
		    case RMS$_FNF:      /* No files found at all */
		    	return (ENDFILE);

		    case RMS$_DNF:	/* Directory not found */
		    	return(LO_NO_SUCH);

		    default:		/* Any other error */
		    	return(FAIL);
		}
	}
}

/*{
** Name:	LOwend	- End wildcard search
**
** Description:
**		Return system resources after a wildcard search is over.
**		This should always be called to clean up.
**
** Inputs:
**	lc	LO_DIR_CONTEXT *	Context structure
**
** Outputs:
**	none
**
**	Returns:
**		STATUS	OK
**
**	Exceptions:
**		none
**
** Side Effects:
** 	System resources (memory and possibly an I/O channel) are returned.
** History:
**	10/89	(Mike S)	Initial version
*/
STATUS
LOwend(lc)
LO_DIR_CONTEXT	*lc;
{
	/* If there's anything to clean up, clean it up */
	if (lc->context != 0)
		lib$find_file_end(&lc->context);

	lc->context = 0;
	return (OK);
}
