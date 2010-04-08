/******************************************************************************
**
** Copyright (c) 1987, Ingres Corporation
**
******************************************************************************/

#define    INCL_DOSERRORS

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
** Name: DIdelete - Deletes a file.
**
** Description:
**      The DIdelete routine is used to delete a direct access file.
**      The file does not have to be open to delete it.  If the file is
**      open it will be deleted when the last user perfroms a DIclose.
**
** Inputs:
**      notused              NULL if private file, else DI_IO ptr (not used)
**      path                 Pointer to the area containing
**                           the path name for this file.
**      pathlength           Length of path name.
**      filename             Pointer to the area containing
**                           the file name.
**      filelength           Length of file name.
** Outputs:
**      err_code             Pointer to a variable used
**                           to return operating system
**                           errors.
**    Returns:
**        OK
**        DI_BADFILE         Bad file context.
**        DI_BADOPEN         Error openning file.
**        DI_DIRNOTFOUND     Path not found.
**        DI_BADDELETE       Error deleting file.
**        DI_BADPARAM        Parameter(s) in error.
**	  DI_EXCEED_LIMIT    Open file limit exceeded.
**        DI_FILENOTFOUND    File not found.
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
**	7-Mar-2007 (kschendel) SIR 122512
**	    Comment update re possibility of null DI_IO pointer.
**
******************************************************************************/
STATUS
DIdelete(DI_IO      *notused,
         char       *path,
         u_i4       pathlength,
         char       *filename,
         u_i4       filelength,
         CL_SYS_ERR *err_code)
{
	STATUS dos_call;
	STATUS ret_val = OK;
	char   pathbuf[DI_PATH_MAX];

	CLEAR_ERR(err_code);

	/* Check input parameters. */

	if (pathlength > DI_PATH_MAX || pathlength == 0 ||
	    filelength > DI_FILENAME_MAX || filelength == 0) 
	{
		return (DI_BADPARAM);
	}

	MEcopy(path, pathlength, pathbuf);
	pathbuf[pathlength] = '\\';
	MEcopy(filename, filelength, pathbuf + pathlength + 1);
	pathbuf[pathlength + filelength + 1] = '\0';

	if (!DeleteFile(pathbuf)) 
	{
		dos_call = GetLastError();
		SETWIN32ERR(err_code, dos_call, ER_delete);
		switch (dos_call) 
		{
		case ERROR_ACCESS_DENIED:
			if (SetFileAttributes(pathbuf,
		                      FILE_ATTRIBUTE_NORMAL) == TRUE)
				DeleteFile(pathbuf);
			break;
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
