/*
** Copyright (c) 1996, Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <er.h>
#include    <pc.h>
#include    <lo.h>
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
*/

STATUS
CK_write_error(
PID		*pid)
{
    STATUS	ret_val = OK;
    LOCATION	location;
    char	filename[LO_NM_LEN];
    char        *iipath;
    char        path[LO_PATH_MAX];
    FILE	*fp; 

    NMgtAt( ERx("II_TEMPORARY"), &iipath );
    if( iipath == NULL || *iipath == EOS )
	iipath = ".";
    STcopy ( iipath, path );
    LOfroms( PATH, path, &location);
    STprintf (filename, "%d.err" , *pid);
    LOfstfile( filename, &location );
    SIfopen (&location, "w", SI_TXT, 0, &fp);
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
*/

STATUS
CK_exist_error(
PID		*pid)
{
    STATUS	ret_val = OK;
    LOCATION	location;
    char	filename[LO_NM_LEN];
    char	*iipath;
    char	path[LO_PATH_MAX];

    NMgtAt( ERx("II_TEMPORARY"), &iipath );
    if( iipath == NULL || *iipath == EOS )
	iipath = ".";
    STcopy ( iipath, path );
    LOfroms( PATH, path, &location);
    STprintf (filename, "%d.err" , *pid);
    LOfstfile( filename, &location );
    ret_val = LOexist( &location );
    if (ret_val == OK)
	LOdelete( &location );
    return(ret_val);
}
