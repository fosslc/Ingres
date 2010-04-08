/******************************************************************************
**
** Copyright (c) 1987, Ingres Corporation
**
******************************************************************************/

#define	INCL_DOSERRORS

#include   <compat.h>
#include   <cs.h>
#include   <di.h>
#include   <er.h>
#include   <me.h>

/* # defines */

/* typedefs */

/* forward references */

/* externs */

/* statics */

/******************************************************************************
**
** Name: DIdirdelete  - Deletes a directory.
**
** Description:
**      The DIdirdelete routine is used to delete a direct access
**      directory.  The name of the directory to delete should
**      be specified as path and dirname, where dirname does not
**      include a type qualifier or version and the path does
**	not include the directory name (which it normally does for
**	other DI calls).  The directory name may be  converted to a filename
**	of the directory and use DIdelete to delete it.
**
** Inputs:
**      f                    Pointer to file context.
**      path                 Pointer to the area containing
**                           the path name for this directory.
**      pathlength           Length of path name.
**      dirname              Pointer to the area containing
**                           the directory name.
**      dirlength            Length of directory.
** Outputs:
**      err_code             Pointer to a variable used
**                           to return operating system
**                           errors.
**    Returns:
**        OK
**	  DI_DIRNOTFOUND     Path not found.
**	  DI_FILENOTFOUND    File not found.
**        DI_BADPARAM        Parameter(s) in error.
**        DI_BADDELETE       Error trying to delete directory.
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**      18-jul-95 (reijo01)
**          Changed SETWIN32ERR so that it will populate the CL_ERR_DESC with
**              the proper values.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**
******************************************************************************/
STATUS
DIdirdelete(DI_IO      *f,
            char       *path,
            u_i4       pathlength,
            char       *dirname,
            u_i4       dirlength,
            CL_SYS_ERR *err_code)
{
	STATUS dos_call;
	STATUS ret_val = OK;
	char   full_path[DI_PATH_MAX];

	/* default return values */

	CLEAR_ERR(err_code);

	/* Check input parameters. */

	if (pathlength > DI_PATH_MAX ||
	    pathlength == 0 ||
	    dirlength > DI_FILENAME_MAX) 
	{
		return (DI_BADPARAM);
	}

	/* get null terminated path and filename */

	MEcopy(path, pathlength, full_path);

	if (dirlength) 
	{
		full_path[pathlength++] = '\\';
		MEcopy(dirname, dirlength, full_path + pathlength);
	}

	full_path[pathlength + dirlength] = '\0';

	/* Use the system call to do the delete */

	if (!RemoveDirectory(full_path)) 
	{
		dos_call = GetLastError();
		SETWIN32ERR(err_code, dos_call, ER_delete);
		switch (dos_call) 
		{
		case ERROR_PATH_NOT_FOUND:
			ret_val = DI_DIRNOTFOUND;
			break;
		case ERROR_FILE_NOT_FOUND:
			ret_val = DI_FILENOTFOUND;
			break;
		default:
			ret_val = DI_BADDELETE;
			break;
		}
	}

	return (ret_val);
}
