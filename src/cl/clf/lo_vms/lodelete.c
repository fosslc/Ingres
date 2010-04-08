/*
**		Copyright (c) 1983, 2000 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<descrip.h>
# include	<rms.h>
# include	<lib$routines.h>
# include	<lo.h>
# include	<st.h>
# include	<pe.h>
# include 	<er.h>  
# include	"lolocal.h"


/*LOdelete
**
**  	Delete a location.
**
**	The file or directory specified by loc is destroyed if possible.
**
**	success:  LOdelete deletes the file or directory and returns-OK.
**		  WARNING -- when LOdelete is given a directory it deletes the
**		  contents of the directory, and then tries to delete the
**		  directory itself.  This will fail if the directory contains
**		  subdirectories.
**	failure:  LOdelete doesn't delete the file, or can't delete the whole
**		  directory.
**		  LOdelete returns NO_PERM if the process does not have
**		  permissions to delete the given file or directory.  LOdelete
**		  returns NO_SUCH if part of the path, the file, or the direc-
**		  tory does not exist.  LOdelete returns CANT_TELL if some other
**		  situation comes up, including not being able to delete some
**		  file in the directory.
**
**		History
**			03/09/83 -- (mmm)
**				written
**			09/20/83-- VMS CL (dd)
**			11/12/84 -- (dreyfus) cleanup up. recursively delete
**				directories.
**			09/22/86 (Joe)
**				Removed all LO_CONCUR_DE... stuff.
**			10/4/89 (Mike S)
**				Delete all versions of a file
**				General cleanup.
**      16-jul-93 (ed)
**	    added gl.h
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
*/

/* static char	*Sccsid = "@(#)LOdelete.c	1.5  6/1/83"; */

typedef struct NAM	_NAM;

static VOID 	dir_to_file();
static STATUS	error_handler();

STATUS
LOdelete(loc)
register LOCATION	*loc;
{
	STATUS 	clstatus;		/* Translated to CL status */
	char 	filename[MAX_LOC+1];	/* Name of file to delete */
	STATUS	handler_status;         /* delete status from error handler */

	$DESCRIPTOR(desc, filename);

	if ((loc->string == NULL) || (*(loc->string) == NULL))
	{
		/* no path in location */
		return(LO_NO_SUCH);
	}

	if((loc->desc | FILENAME) != FILENAME)
	{
		/* It's a directory.  First delete its contents. */
		STpolycat(2, loc->string, WILD_NTV, filename);
		desc.dsc$w_length = STlength(filename);
		handler_status = RMS$_NORMAL;
		lib$delete_file(&desc, NULL, NULL, NULL, error_handler, NULL, 
			        &handler_status);

		/* return if it failed */
		if((handler_status & 1) == 1)
			;		/* It worked */
		else if	(handler_status == RMS$_FNF)
			;		/* Directory was empty */
		else if (handler_status == RMS$_DNF)
			return(LO_NO_SUCH); 	/* Directory not found */
		else if (handler_status == RMS$_PRV)
			return(LO_NO_PERM);	/* We're not allowed */
		else
			return(LO_CANT_TELL); 	/* Some other error */

		/* Prepare to delete directory */
		LOdir_to_file(loc->string, filename);
	}
	else
	{
		/* 
		** It's a single file.  delete all versions if none 
	        ** is specified.
		*/
		STcopy(loc->string, filename);
		if (loc->verlen == 0)
			STcat(filename, WILD_V);
	}

	/* Delete the file */
	desc.dsc$w_length = STlength(filename);

	handler_status = RMS$_NORMAL;
	lib$delete_file(&desc, NULL, NULL, NULL, error_handler, NULL, 
		        &handler_status);
	if((handler_status & 1) == 1)
		return (OK); 		/* It worked */
	else if	(handler_status == RMS$_FNF)
		return (LO_NO_SUCH);	/* File not found */
	else if (handler_status == RMS$_DNF)
		return(LO_NO_SUCH); 	/* Directory not found */
	if(handler_status == RMS$_PRV)
		return(LO_NO_PERM);	/* We're not allowed */
	return(LO_CANT_TELL);		/* Some other error */
}

/*
** This routine is called by lib$delete_file if there's an error during the
** delete.  It always returns RMS$_NORMAL, which means, "Go on deleting".
*/
static STATUS
error_handler(filespec, rms_sts, rms_stv, error_source, handler_status)
struct dsc$descriptor	*filespec;	/* File which caused error */
STATUS 	 		*rms_sts;	/* Error status */
STATUS 			*rms_stv;	/* Secondary error status */
i4   	  		*error_source;	/* 0 -- error finding file
       					   1 -- error deleting file */
STATUS 			*handler_status;/* status returned to LOdelete */
{
	char	file[MAX_LOC+1];	/* copy of filename */
	LOCATION loc;		/* LOCATION version of filename */
	STATUS status;
	STATUS vmsstatus;

	if (*rms_sts == RMS$_PRV)
	{
		/* 
		** If it's a privilege problem, turn world delete permission
		** on and try again.
		*/
		STlcopy(filespec->dsc$a_pointer, file, filespec->dsc$w_length);
		status = LOfroms(FILENAME & PATH, file, &loc);
		if (status == OK)
		{
			/* 
			** This is a little misleading.  It looks like we're
			** adding write permission.  Actually, we must already
			** have write permission for PEworld to modify the
			** file header successfully; what we're adding is
			** world write and DELETE permission.
			*/			
			PEworld(ERx("+w"), &loc);
			vmsstatus = lib$delete_file(filespec);
			if ((vmsstatus & 1) == 1)
				status = OK;
			else
				status = FAIL;
		}
	}
	else
	{
		status = FAIL;		/* Give up */
	}

	/* If we couldn't fix it, send back the failure status */
	if (status != OK) *handler_status = *rms_sts;

	return RMS$_NORMAL;
}
