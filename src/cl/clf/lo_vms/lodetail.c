# include	  <compat.h>
#include    <gl.h>
# include	  <rms.h>
# include	  <lo.h>
# include	  <st.h>
# include	  "lolocal.h"
/*
 *	Copyright (c) 1983 Ingres Corporation
 *
 *	Name:
 *		LOdetail
 *
 *	Function:
 *		Returns various and sundry parts of a location in string
 *		form.
 *
 *	Arguments:
 *		loc:	a pointer to the location in question
 *		dev:	a pointer to a buffer where the device name will be stored
 *			NULL on UNIX.
 *		path:	a pointer to a buffer where the path will be stored
 *		fprefix:a pointer to a buffer where the part of a filename
 *			before the dot will be stored.
 *		fsufix: a pointer to a buffer where the part of a filename
 *			after the dot will be stored.
 *		version:a pointer to a buffer where the version will be stored.
 *			NULL on UNIX.
 *
 *	Result:
 *		stores the results in the given buffers.  Buffers should
 *		be of large enough size to hold the results.
 *
 *	Side Effects:
 *
 *	History:
 *		4/5/83 -- (mmm) written
 *		9/23/83 -- (dd) VMS CL
 *		9/28/89 -- (Mike S) Use RMS parsing
 *
 *
**      16-jul-93 (ed)
**	    added gl.h
 */
 STATUS
 LOdetail(loc, dev, path, fprefix, fsuffix, version)
 LOCATION	*loc;
 char		*dev;
 char		*path;
 char		*fprefix;
 char		*fsuffix;
 char		*version;
 {
	/* initialize output variables */
	*dev = *path = *fprefix = *fsuffix = *version = NULL;

	/*
 	**  Fill in the pieces which were specifed for the LOCATION. 
	*/
	if (loc->devlen > 1)
	{
		STlcopy(loc->devptr, dev, loc->devlen - 1);    /* Skip colon */
	}
	if (loc->dirlen > 2)
	{
		/* Skip brackets */
		STlcopy(loc->dirptr + 1, path, loc->dirlen - 2);
	}
	if (loc->namelen > 0)
	{
		STlcopy(loc->nameptr, fprefix, loc->namelen);
	}
	if (loc->typelen > 1)
	{
		STlcopy(loc->typeptr + 1, fsuffix, loc->typelen - 1);
	}
	if (loc->verlen > 1)
	{
		STlcopy(loc->verptr + 1, version, loc->verlen - 1);
	}

	return(OK);
}
