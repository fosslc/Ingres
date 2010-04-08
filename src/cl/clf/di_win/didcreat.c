/******************************************************************************
**
** Copyright (c) 1987, Ingres Corporation
**
******************************************************************************/

#define   INCL_DOSERRORS

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
** Name: DIdircreate - Creates a directory.
**
** Description:
**      The DIdircreate will take a path name and directory name
**      and create a directory in this location.
**
** UNIX design:
**	The creation of directories in the dbms presents a special problem
**	which is further complicated by the ingres protection system and the
**	differences in system V and BSD implementation of routines to create
**	directories.
**
**	The directories that are created by this function are database, journal
**	and ckp directories.  The directory in which they are created is owned
**	by INGRES and has permission 700 (to stop all other users from getting
**	to the directories below).  The ingres process which is running
**	executing this command will be installed as a "setuid" program, so that
**	users running the program will have effective uid INGRES and real uid
**	random_user.
**
**	The current BSD (4.2 and later) mkdir command creates a directory
**	given a full pathname (or relative) correctly in these circumstances.
**
**	However system V does not provide an equivalent functionality.  The
**	mknod() command must be run with effective uid root.  The only other
**	routine to make a directory available is the system level command
**	"mkdir".  The system V mkdir uses access() which runs with the real
**	users permission.  Therefore mkdir fails if given a full pathname
**	with the above stated permission characteristics.   To get around
**	this, in the SV environment, we change our cwd to be the "base"
**	directory in which we want to create the new directory.  We then use
**	the user level "mkdir" to create the new directory.  This requires the
**	real user to have write permission in the "base" directory; therefore
** 	the INGRES protection scheme is changed so that the "base" directories
**	under data, ckp, jnl, etc. are PEworld("rwxrwx").  data, ckp jnl, etc.
**	remain PEworld("rwx--") so the data is still protected (modulo UNIX).
**	When DIcreate() is used to create databases (ckps, jnls) it's now
**	necessary to use PEumask("rwxrxw"), otherwise the REAL user's
**	permissions may preclude ingres from accessing the database (ckp, jnl).
**
**	It should also be noted that the database directories will be owned
**	by the REAL user on SV and "ingres" (the effective user) on BSD42.
**	This is not a change but has been a point of confusion.
**
**
** Inputs:
**      f                    Pointer to file context area.
**      path                 Pointer to the path name.
**      pathlength           Length of path name.
**      dirname              Pointer to the directory name.
**      dirlength            Length of directory name.
** Outputs:
**      err_code             Pointer to a variable used
**                           to return operating system
**                           errors.
**    Returns:
**        OK
**        DI_BADDIR          Path specification error.
**        DI_BADPARAM        Parameter(s) in error.
**        DI_EXCEED_LIMIT    Open file limit exceeded.
**        DI_EXISTS          Already exists.
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
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**
******************************************************************************/
STATUS
DIdircreate(DI_IO      *f,
            char       *path,
            u_i4       pathlength,
            char       *dirname,
            u_i4       dirlength,
            CL_SYS_ERR *err_code)
{
	STATUS          ret_val = OK;
	STATUS          dos_call;
	char            full_path[DI_PATH_MAX];

	/* default return values */

	CLEAR_ERR(err_code);

	/* Check input parameters. */

	if (pathlength > DI_PATH_MAX ||
	    pathlength == 0 ||
	    dirlength > DI_FILENAME_MAX ||
	    dirlength == 0) {
		return (DI_BADPARAM);
	}

	/*
	** constuct the full path of directory to create
	*/

	MEcopy(&path[0], pathlength, &full_path[0]);

	if (full_path[pathlength-1] == '\\') 
	{
		pathlength--;
	} 
	else 
	{
		full_path[pathlength] = '\\';
	}

	MEcopy(&dirname[0], dirlength, &full_path[pathlength + 1]);

	full_path[pathlength + dirlength + 1] = '\0';

	/*
	** Create the directory
	*/

	if (!CreateDirectory(full_path, NULL)) 
	{
		dos_call = GetLastError();
		SETWIN32ERR(err_code, dos_call, ER_create);
		switch (dos_call) 
		{
		case ERROR_PATH_NOT_FOUND:
			ret_val = DI_DIRNOTFOUND;
			break;
		case ERROR_ALREADY_EXISTS:
			ret_val = DI_EXISTS;
			break;
		default:
			ret_val = DI_BADCREATE;
			break;
		}
	}

	return (ret_val);
}
