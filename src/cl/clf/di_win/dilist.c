/******************************************************************************
**
** Copyright (c) 1987, 1998 Ingres Corporation
**
******************************************************************************/

#define	INCL_DOSERRORS

#include   <compat.h>
#include   <cs.h>
#include   <di.h>
#include   <er.h>
#include   <me.h>
#include   <st.h>

/* # defines */

#define DIRECTORY 1
#define FIL       0

/* typedefs */

/* forward references */

static STATUS DI_list(char       *path,
                      u_i4       pathlength,
                      i4         list_type,
                      STATUS     (*func) (),
                      PTR        arg_list,
                      CL_SYS_ERR *err_code);

/* externs */

/* statics */


/******************************************************************************
**
** Name: DIlistdir - List all directories in a directory.
**
** Description:
**      The DIlistdir routine will list all directories
**      that exist in the directory(path) specified.
**      The directories are not listed in any specific order.
**      This routine expects
**      as input a function to call for each directory found.  This
**      insures that all resources tied up by this routine will
**      eventually be freed.  The function passed to this routine
**      must return OK if it wants to continue with more directories,
**      or a value not equal to OK if it wants to stop listing.
**	If the function returns anything by OK, then DIlistdir
**	will return DI_ENDFILE.
**      The function must have the following call format:
**
**          STATUS
**          funct_name(arg_list, dirname, dirlength, err_code)
**          i4         *arg_list;       Pointer to argument list.
**          char       *dirname;        Pointer to directory name.
**          i4         dirlength;       Length of directory name.
**          CL_SYS_ERR *err_code;       Pointer to operating system
**                                      error codes (if applicable).
**
** Inputs:
**      f                    Pointer to file context area.
**      path                 Pointer to the path name.
**      pathlength           Length of path name.
**      fnct                 Function to pass the
**                           directory entries to.
**      arg_list             Pointer to function argument
**                           list.
** Outputs:
**      err_code             Pointer to a variable used
**                           to return operating system
**                           errors.
**    Returns:
**        DI_BADDIR          Path specification error.
**        DI_ENDFILE         Error returned from client handler
**                           or all files listed.
**        DI_BADPARAM        Parameters in error.
**        DI_DIRNOTFOUND     Path not found.
**        DI_BADLIST         Error trying to list objects.
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**
******************************************************************************/
STATUS
DIlistdir(DI_IO      *f,
          char       *path,
          u_i4       pathlength,
          STATUS     (*func) (),
          PTR        arg_list,
          CL_SYS_ERR *err_code)
{
	STATUS          result;

	result = DI_list(path, pathlength, DIRECTORY, func, arg_list, err_code);
	return (result);
}

/******************************************************************************
**
** Name: DIlistfile - List all files in a directory.
**
** Description:
**      The DIlistfile routine will list all files that exist
**      in the directory (path) specified.  This routine expects
**      as input a function to call for each file found.  This
**      insures that all resources tied up by this routine will
**      eventually be freed.  The files are not listed in any
**      specific order. The function passed to this routine
**      must return OK if it wants to continue with more files,
**      or a value not equal to OK if it wants to stop listing.
**	     If the function returns anything by OK, then DIlistfile
**	     will return DI_ENDFILE.
**
**      The function must have the following call format:
**
**          STATUS
**          funct_name(arg_list, filename, filelength, err_code)
**          i4         *arg_list;       Pointer to argument list
**          char       *filename;       Pointer to directory name.
**          i4         filelength;      Length of directory name.
**          CL_SYS_ERR *err_code;       Pointer to operating system
**                                      error codes (if applicable).
**
** Inputs:
**      f                    Pointer to file context area.
**      path                 Pointer to the path name.
**      pathlength           Length of path name.
**      fnct                 Function to pass the
**                           directory entries to.
**      arg_list             Pointer to function argument
**                           list.
** Outputs:
**      err_code             Pointer to a variable used
**                           to return operating system
**                           errors.
**    Returns:
**        DI_BADDIR          Path specification error.
**        DI_ENDFILE         Error returned from client handler or
**                           all files listed.
**        DI_BADPARAM        Parameter(s) in error.
**        DI_DIRNOTFOUND     Path not found.
**        DI_BADLIST         Error trying to list objects.
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
******************************************************************************/
STATUS
DIlistfile(DI_IO      *f,
           char       *path,
           u_i4       pathlength,
           STATUS     (*func) (),
           PTR        arg_list,
           CL_SYS_ERR *err_code)
{
	STATUS          result;

	result = DI_list(path, pathlength, FIL, func, arg_list, err_code);

	return (result);
}

/******************************************************************************
**
** Name: DI_list - Use WIN32 NT directory listing interface to list files
**
** Description:
**	List all files/directories in a given directory depending on
**	list_type argument.
**
** Inputs:
**      path                 Pointer to the path name.
**      pathlength           Length of path name.
**	    list_type	         type of object to list (DIRECTORY or FILE)
**	    func		         function to call for each object found.
**	    arg_list	         arguments to call the function with.
**
** Outputs:
**      err_code             Pointer to a variable used
**                           to return operating system
**                           errors.
**
**    Returns:
**        DI_BADDIR          Path specification error.
**        DI_ENDFILE         Error returned from client handler or
**                           all files listed.
**        DI_BADPARAM        Parameter(s) in error.
**        DI_DIRNOTFOUND     Path not found.
**        DI_BADLIST         Error trying to list objects.
**
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**	07-jul-1995 (canor01)
**	    convert filename returned from FindFirstFile() to lowercase, since
**	    other DI functions (via unix) expect lowercase.
**      18-jul-1995 (reijo01)
**          Changed SETWIN32ERR so that it will populate the CL_ERR_DESC with
**              the proper values.
**      08-dec-1997 (canor01)
**          Implement LRU for open files (initial copy from Unix).
**	28-jan-1998 (canor01)
**	    Correct a problem with previous implementation.  If closing
**	    files succeeds, retry, else bail out with error.
**	04-mar-1998 (canor01)
**	    Parameters to DIlru_flush were changed on Unix side.  Change
**	    them here too.
**
******************************************************************************/
static STATUS
DI_list(char       *path,
        u_i4       pathlength,
        i4         list_type,
        STATUS     (*func) (),
        PTR        arg_list,
        CL_SYS_ERR *err_code)
{
	STATUS          ret_val = OK;
	STATUS          dos_call;

	char            input_path[DI_PATH_MAX];
	HANDLE          handle;
	ULONG           count = 1;
	WIN32_FIND_DATA findbuf;

	/* default returns */

	CLEAR_ERR(err_code);

	/* Check input parameters */

	if ((pathlength > DI_PATH_MAX) ||
	    (pathlength == 0))
		return DI_BADPARAM;

	/* get null terminated path to the directory */

	memcpy(input_path, path, pathlength);
	memcpy(input_path + pathlength, "\\*.*", 5);

	/* break out on errors */

	while ( (handle = FindFirstFile(input_path, &findbuf)) == 
		INVALID_HANDLE_VALUE )
	{
		switch (dos_call = GetLastError()) 
		{
            	case ERROR_NO_SYSTEM_RESOURCES:
                case ERROR_TOO_MANY_OPEN_FILES:
                case ERROR_NO_MORE_FILES:
                case ERROR_NOT_ENOUGH_MEMORY:
                case ERROR_OUTOFMEMORY:
                case ERROR_HANDLE_DISK_FULL:
                case ERROR_OUT_OF_STRUCTURES:
                case ERROR_NONPAGED_SYSTEM_RESOURCES:
                case ERROR_PAGED_SYSTEM_RESOURCES:
            		if (DIlru_flush( err_code ) != OK)
            		{
                	    ret_val = DI_BADLIST;
            		}
			else
			{
			    continue;
			}
                	break;
		case ERROR_PATH_NOT_FOUND:
			SETWIN32ERR(err_code, dos_call, ER_list);
			ret_val = DI_DIRNOTFOUND;
			break;
		default:
			SETWIN32ERR(err_code, dos_call, ER_list);
			ret_val = DI_BADLIST;
			break;
		}
		if ( ret_val != OK )
		    return(ret_val);
	}

	/* now loop through all entries in the directory */

	ret_val = DI_ENDFILE;

	do {
		if (!memcmp(findbuf.cFileName, ".", 2) ||
		    !memcmp(findbuf.cFileName, "..", 3)) 
		{
			continue;
		}

		/*
		 * This next check is a leftover from OS/2.  Should dump it
		 * at some point, or enlarge the functionality of this
		 * routine. If we're looking for a directory, verify it is
		 * one...
		 */

		if (list_type == DIRECTORY) 
		{
			if (~findbuf.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				continue;
			}
		}

		/* 
		* other functions expect lowercase name, but DOS
		* returns uppercase
                *
                * bug fix 89213.  
                * Unlike DOS, NT returns upper and lower case
                * names.
		*/
		if ((*func) (arg_list,
		             findbuf.cFileName,
		             STlength(findbuf.cFileName),
		             err_code)) 
		{
			ret_val = DI_ENDFILE;
			break;
		}
	} while (FindNextFile(handle, &findbuf));

	if (!FindClose(handle)) 	
	{
		SETWIN32ERR(err_code, GetLastError(), ER_list);
		ret_val = DI_BADLIST;
	}

	return (ret_val);
}
