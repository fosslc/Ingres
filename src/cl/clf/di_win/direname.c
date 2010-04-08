/******************************************************************************
**
** Copyright (c) 1987, 2003 Ingres Corporation
**
******************************************************************************/

#define	INCL_DOSERRORS

#include   <compat.h>
#include   <cs.h>
#include   <di.h>
#include   <er.h>
#include   <me.h>
#include   "dilru.h"

/******************************************************************************
**
** Name: DIrename - Renames a file.
**
** Description:
**      The DIrename will change the name of a file.
**	The file need not be closed.  The file can be renamed
**      but the path cannot be changed.  A fully qualified
**      filename must be provided for old and new names.
**      This includes the type qualifier extension.
**
** Inputs:
**      f                    Pointer to file context.
**      path                 Pointer to the path name.
**      pathlength           Length of path name.
**      oldfilename          Pointer to old file name.
**      oldlength            Length of old file name.
**      newfilename          Pointer to new file name.
**      newlength            Length of new file name.
** Outputs:
**      err_code             Pointer to a variable used
**                           to return operating system
**                           errors.
**    Returns:
**        OK
**        DI_BADRNAME        Any i/o error during rename.
**        DI_BADPARAM        Parameter(s) in error.
**        DI_DIRNOTFOUND     Path not found.
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
**	20-apr-96 (emmag)
**	    If the rename fails because of a sharing violation, we should
**	    report a DI_BADRNAME and not DI_EXCEED_LIMIT which is 
**	    misleading.   Since DI_BADRNAME is the default, allow this
**	    to fall through.
**	29-apr-1996 (canor01)
**	    On a sharing violation, attempt the copy/delete combination.
**	    Failure of a delete does not affect the results, since we can
**	    reuse files later.
**	02-may-1996 (canor01)
**	    Copy/delete combination also needed when destination file is
**	    not writable via MoveFileEx().  CopyFile() will work, however.
**      06-Aug-96 (fanra01)
**          Added case for file already in existence to use copy/delete logic
**          and changed access denied case to reuturn BADRNAME since access
**          denied normally means file is in use exclusively.
**      23-Sep-96 (fanra01)
**          Modified the ERROR_ACCESS_DENIED case back to following the copy/
**          delete path.  Returning error causes errors during aborts and
**          rollbacks.
**      08-dec-1997 (canor01)
**          Implement LRU for open files (initial copy from Unix).
**	07-jan-1998 (canor01)
**	    If rename fails due to open file sharing, close files until it
**	    succeeds.
**	28-jan-1998 (canor01)
**	    If we close files successfully, retry the operation.  Otherwise,
**	    bail out with an error.
**      04-mar-1998 (canor01)
**          Parameters to DIlru_flush were changed on Unix side.  Change
**          them here too.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**      06-dec-1999 (somsa01)
**	    Replaced MoveFileEx() call with CopyFile()/DeleteFile()
**	    sequence. MoveFileEx(), when used in conjunction with
**	    opening/closing files with/without FILE_FLAG_NO_BUFFERING,
**	    files may not get copied over properly (i.e., some page
**	    writes can be lost).
**	12-sep-2003 (somsa01)
**	    Undid replacement of MoveFileEx() call with CopyFile()/DeleteFile()
**	    sequence, as the original bug seems to have gone away. Also,
**	    MoveFileEx() is definitely faster than the CopyFile()/DeleteFile()
**	    sequence.
**
******************************************************************************/
STATUS
DIrename(DI_IO      *f,
         char       *path,
         u_i4       pathlength,
         char       *oldfilename,
         u_i4       oldlength,
         char       *newfilename,
         u_i4       newlength,
         CL_SYS_ERR *err_code)
{
	char   oldfile[DI_PATH_MAX];
	char   newfile[DI_PATH_MAX];
	STATUS dos_call;
	STATUS ret_val = OK;

	/* default returns */

	CLEAR_ERR(err_code);

	if ((pathlength > DI_PATH_MAX) ||
	    (pathlength == 0) ||
	    (oldlength > DI_FILENAME_MAX) ||
	    (oldlength == 0) ||
	    (newlength > DI_FILENAME_MAX) ||
	    (newlength == 0))
		return (DI_BADPARAM);

	/* get null terminated path and filename for old file */

	MEcopy(path, pathlength, oldfile);
	oldfile[pathlength] = '\\';
	MEcopy(oldfilename, oldlength, &oldfile[pathlength + 1]);
	oldfile[pathlength + oldlength + 1] = '\0';

	/* get null terminated path and filename for new file */

	MEcopy(path, pathlength, newfile);
	newfile[pathlength] = '\\';
	MEcopy(newfilename, newlength, &newfile[pathlength + 1]);
	newfile[pathlength + newlength + 1] = '\0';

	/* Now rename the file. */

	while (!MoveFileEx(oldfile, newfile,
			   MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) 
	{
		dos_call = GetLastError();
		switch (dos_call) 
		{
		case ERROR_PATH_NOT_FOUND:
			SETWIN32ERR(err_code, dos_call, ER_rename);
			ret_val = DI_DIRNOTFOUND;
			break;
		case ERROR_ACCESS_DENIED:
			/* 
			** Destination file exists and is in use exclusively.
			*/
		case ERROR_SHARING_VIOLATION:
			/* 
			** Source file opened writable.
			** Close some files and re-try.
			*/
			if (DIlru_flush( err_code ) == OK)
			    continue;
                case ERROR_ALREADY_EXISTS:
			/*
			** Destination file exists.
			** Fall into the copy/delete logic below.
			*/
			if (!CopyFile(oldfile, newfile, FALSE))
			{
			    SETWIN32ERR(err_code, dos_call, ER_rename);
			    ret_val = DI_BADRNAME;
			}
			else
			{
			    /*
			    ** This will probably fail, being the
			    ** likely reason for the sharing violation,
			    ** so don't bother to check return value.
			    */
			    (void)DeleteFile(oldfile);
			}
			break;
		default:
			SETWIN32ERR(err_code, dos_call, ER_rename);
			ret_val = DI_BADRNAME;
			break;
		}
		break;
	}

	return (ret_val);
}
