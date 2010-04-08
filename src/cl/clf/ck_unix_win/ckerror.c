/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <er.h>
#include    <pc.h>
#include    <lo.h>
#include    <me.h>
#include    <si.h>
#include    <st.h>
#include    <ck.h>
#include    <errno.h>
#include    <nm.h>

/**
**
**  Name: CKERROR.C - Handle error from child processes.
**
**  Description:
**
**	    CK_write_error() - create an error file base on pid.
**		       - A file with the extension .err is created
**			 with the suffix being the pid number.  An 
**			 empty file is created in II_TEMPORARY.
**
**	    CK_exist_error() - check to see if an error file base on pid exists.
**
**  History:
**	31-may-96 (hanch04)
**	    Created.
**	15-may-97 (mcgem01)
**	    Clean up compiler warnings.
**	11-nov-1999 (somsa01)
**	    The temporary file will now contain four entries: errno, CL
**	    return code, OS return code, and the command executed.
**	15-Jun-2004 (schka24)
**	    Buffer overrun fixes: replace stcopy with stlcopy.
**/

/*{
** Name: CK_write_error - create an error file base on pid.
**
** Description:
**	Create a file with the filename based on pid
**	in the II_TEMPORARY directory.  A call to 
**	CK_exists_error later on will find this file.
**	The existence of such a file means the process
**	for which the file was created had failed.
**
** Inputs:
**	pid			pid to create error file for.
**	errnum			errno for the process.
**	os_ret			OS return code from the process.
**	command			command line for process.
**
** Outputs:
**	err_code		machine dependent error return
**
**	Returns:
**
**
** Side Effects:
**	    none
**
** History:
**	31-may-96 (hanch04)
**	    Created.
**	11-nov-1999 (somsa01)
**	    The temporary file will now contain four entries: errno, CL
**	    return code, OS return code, and the command executed.
*/

STATUS
CK_write_error(
PID		*pid,
STATUS		retval,
u_i4		errnum,
i4		os_ret,
char		*command)
{
    STATUS	ret_val = OK;
    LOCATION	location;
    char	filename[LO_NM_LEN];
    char        *iipath;
    char        path[LO_PATH_MAX];
    FILE	*fp; 
    i4		numwritten, command_size;

    NMgtAt( ERx("II_TEMPORARY"), &iipath );
    if( iipath == NULL || *iipath == EOS )
	iipath = ".";
    STlcopy ( iipath, path, sizeof(path)-1 );
    LOfroms( PATH, path, &location);
    STprintf (filename, "%d.err" , *pid);
    LOfstfile( filename, &location );

    /*
    ** Write out the necessary error info.
    */
    SIfopen (&location, "w", SI_BIN, 0, &fp);
    SIwrite(sizeof(errnum), (PTR)&errnum, &numwritten, fp);
    SIwrite(sizeof(retval), (PTR)&retval, &numwritten, fp);
    SIwrite(sizeof(os_ret), (PTR)&os_ret, &numwritten, fp);
    command_size = STlength(command);
    SIwrite(sizeof(command_size), (PTR)&command_size, &numwritten, fp);
    SIwrite(command_size+1, command, &numwritten, fp);
    SIclose ( fp );

    return(ret_val);
}

/*{
** Name: CK_exist_error - Does error file base on pid exist?
**
** Description:
**	Check to see if the error file based on the pid exists
**	in the II_TEMPORARY directory.  If ther is a file, then
**	that process failed.  Remove temp file and return
**	OK for error.
**
** Inputs:
**	pid			pid to check error file for.
**
** Outputs:
**	err_code		machine dependent error return
**
**	Returns:
**				OK - file does exist, an error has occurred
**				FAIL - file does not exist
**
**
** Side Effects:
**	    none
**
** History:
**	31-may-96 (hanch04)
**	    Created.
**	11-nov-1999 (somsa01)
**	    The temporary file will now contain four entries: errno, CL
**	    return code, OS return code, and the command executed.
*/

STATUS
CK_exist_error(
PID		*pid,
STATUS		*retval,
u_i4		*errnum,
i4		*os_ret,
char		**command)
{
    STATUS	ret_val = OK;
    LOCATION	location;
    char	filename[LO_NM_LEN];
    char	*iipath;
    char	path[LO_PATH_MAX];
    FILE	*fp; 
    i4		numread, command_size;

    NMgtAt( ERx("II_TEMPORARY"), &iipath );
    if( iipath == NULL || *iipath == EOS )
	iipath = ".";
    STlcopy ( iipath, path, sizeof(path)-1 );
    LOfroms( PATH, path, &location);
    STprintf (filename, "%d.err" , *pid);
    LOfstfile( filename, &location );
    ret_val = LOexist( &location );
    if (ret_val == OK)
    {
	/*
	** Read the necessary error info, then delete the file.
	*/
	SIfopen(&location, "r", SI_BIN, 0, &fp);
	SIread(fp, sizeof(*errnum), &numread, (PTR)errnum);
	SIread(fp, sizeof(*retval), &numread, (PTR)retval);
	SIread(fp, sizeof(*os_ret), &numread, (PTR)os_ret);
	SIread(fp, sizeof(command_size), &numread, (PTR)&command_size);
	*command = (char *)MEreqmem(0, command_size+1, TRUE, NULL);
	SIread(fp, command_size+1, &numread, *command);
	SIclose(fp);

	LOdelete( &location );
    }
    return(ret_val);
}
