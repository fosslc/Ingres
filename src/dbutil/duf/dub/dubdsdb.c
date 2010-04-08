/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <pc.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <duf.h>
#include    <me.h>
#include    <er.h>
#include    <duerr.h>

#include    <cs.h>
#include    <lk.h>
#include    <dudbms.h>
#include    <duenv.h>

#include    <di.h>
#include    <dub.h>

#include    <lo.h>
#include    <st.h>

/**
**
**  Name: DUBDSDB.C -	DESTROYDB support routines that are owned by the
**			DBMS group.
**
**  Description:
**        The routines in this module are owned by the DBMS group.
**      They are used exclusively by the database utility, destroydb.
**	These routines are used primarily to delete DBMS files or
**	DBMS directories containing at most files.
**
**	    dub_del_dbms_dirs -	Delete all of the directories under the general
**				of the DBMS for a given database.
**          dub_dir_del()   -	Delete a directory and its file contents.
**	    dub_file_del()  _	Delete a file.
**
**
**  History:
**      29-Oct-86 (ericj)
**          Initial creation.
**      16-Apr-87 (ericj)
**          Updated to delete DBMS directories according to Jupiter specs.
**      19-Jul-88 (ericj)
**          Made Lint changes.
**	3-Nov-88 (teg)
**	    Changed interface to du_xxx_ingpath() routines
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	08-jul-1993 (shailaja)
**	    Fixed prototype incompatibilities.  
**	8-aug-93 (ed)
**	    unnest dbms.h
**	31-jan-94 (mikem)
**	    Added include of <cs.h> before <lk.h>.  <lk.h> now references
**	    a CS_SID type.
[@history_line@]...
[@history_template@]...
**/


/*
[@forward_type_references@]
[@forward_function_references@]
[@#defines_of_other_constants@]
[@type_definitions@]
[@global_variable_definitions@]
[@static_variable_or_function_definitions@]
*/


/*{
** Name: dub_del_dbms_files() -   delete DBMS files and directories associated with
**				  a DB.
**
** Description:
**        This routine is used to delete all the DBMS files and directories
**	associated with the database that is being destroyed.  DBMS files
**	and directories are those that are operated on by DI calls and are
**	generally under the control of the DBMS.  Specifically, these would
**	be the data (both default and extended), checkpointing, and
**	various jounaling directories.
**	  This routine is designed to be idempotent.  That is, if it is
**	called with the same parameters after multiple failures or a
**	successful completion and it completes successfully, it will be
**	equivalent to having been called just once successfully.
**
** Inputs:
**      dub_dbenv                        Ptr to an DU_ENV which describes the
**					environment of the database being
**					destroyed.
**	    .du_db_locs			Ptr to a DU_LOC_LIST which contains
**					a list of extended db locations
**					associated with this database.
**	dub_errcb			Error-handling control block.
**
** Outputs:
**	*dub_errcb			If an error occurs, this block
**					will be set by a call to du_error().
**	Returns:
**	    E_DU_OK			Complete successfully.
**	    E_DU_UERROR			One of the directories could not
**					be deleted.
**
** Side Effects:
**	      Destroys all of the DBMS directories and files associated with a
**	    database.
**
** History:
**      03-Sep-86 (ericj)
**          Initial creation.
**	15-Feb-87 (ericj)
**	    Insured idempotent characteristics and updated to match new
**	    DI specs.
[@history_line@]...
[@history_template@]...
*/

DU_STATUS
dub_del_dbms_dirs(dub_dbenv, dub_errcb)
DU_ENV		*dub_dbenv;
DU_ERROR	*dub_errcb;
{
    DU_LOC_LIST	    *arealist;		/* possible locations for data base */
    DU_STATUS	    status;
    char	    dbms_path[DI_PATH_MAX + 1];

    for(;;)
    {
	/*  Delete the config file first, so that if this program does not
	**  successfully complete, no other user will be able to enter the
	**  crippled database.
	*/
	if ((status = du_db_ingpath(dub_dbenv->du_dbloc.du_area,
		      dub_dbenv->du_dbname, dbms_path, dub_errcb)) != E_DB_OK)
	{
	    return(status);
	}

	status = dub_file_del(dbms_path, DUB_CONFIG_FNAME, dub_errcb);
	if (status != E_DU_OK)
	{
	    if (status == W_DU1021_DEL_FILENOTFOUND ||
	        status == W_DU1020_DEL_DIRNOTFOUND
	       )
	    {
		du_reset_err(dub_errcb);
	    }
	    else
	    {
		break;
	    }
	}

    	/*  Now, destroy the rest of the default database directory  */
	if ((status = du_db_ingpath(dub_dbenv->du_dbloc.du_area,
		      (char *) NULL, dbms_path, dub_errcb)) != E_DB_OK)
	{
	    return(status);
	}
	status = dub_dir_del(dbms_path, dub_dbenv->du_dbname, dub_errcb);
	if (status != E_DU_OK)
	{
	    if (status == W_DU1020_DEL_DIRNOTFOUND)
		du_reset_err(dub_errcb);
	    else
		break;
	}
	/*  Destroy the checkpoint directory */
	if ((status = du_ckp_ingpath(dub_dbenv->du_ckploc.du_area, (char *) NULL,
		       dbms_path, dub_errcb)) != E_DB_OK)
	{
	    return(status);
	}
	status = dub_dir_del(dbms_path, dub_dbenv->du_dbname, dub_errcb);
	if (status != E_DU_OK)
	    if (status == W_DU1020_DEL_DIRNOTFOUND)
		du_reset_err(dub_errcb);
	    else
		break;

	/* Destroy the journaling directory */
	if ((status = du_jnl_ingpath(dub_dbenv->du_jnlloc.du_area, (char *)NULL, 
				    dbms_path, dub_errcb)) != E_DB_OK)
	{
	    return(status);
	}
	status = dub_dir_del(dbms_path, dub_dbenv->du_dbname, dub_errcb);
	if (status != E_DU_OK)
	    if (status == W_DU1020_DEL_DIRNOTFOUND)
		du_reset_err(dub_errcb);
	    else
		break;
/*terminator*/
	/* Destroy the dump directory */
	if ((status = du_dmp_ingpath(dub_dbenv->du_dmploc.du_area, (char *)NULL, 
				    dbms_path, dub_errcb)) != E_DB_OK)
	{
	    return(status);
	}
	status = dub_dir_del(dbms_path, dub_dbenv->du_dbname, dub_errcb);
	if (status != E_DU_OK)
	    if (status == W_DU1020_DEL_DIRNOTFOUND)
		du_reset_err(dub_errcb);
	    else
		break;

	/*  Delete all of the extended locations associated with this DB */
	for(arealist = dub_dbenv->du_db_locs, status = E_DU_OK;
	    arealist; arealist = arealist->du_next)
	{
	    if ((status = du_extdb_ingpath(arealist->du_loc.du_area,
			     (char *) NULL, dbms_path, dub_errcb)) != E_DB_OK)
				    return(status);
	    status = dub_dir_del(dbms_path, dub_dbenv->du_dbname, dub_errcb);
	    if (status != E_DU_OK)
	    	if (status == W_DU1020_DEL_DIRNOTFOUND)
		{
		    du_reset_err(dub_errcb);
		    status = E_DU_OK;
		}
		else
		{
		    break;
		}
	}

	/* Exit the for loop */
	break;

    }	    /* end of for(;;) loop */

    return (status);
}



/*{
** Name: dub_dir_del()	-   Delete a DBMS directory and its file contents.
**
** Description:
**        This routine deletes a DBMS directory and all of the files that
**	it contains.  It assumes that the directory only contains files,
**	not other directories.  Note, callers of this routine should
**	explicitly check for the warning error number,
**	W_DU1020_DEL_DIRNOTFOUND, and call du_error() or ignore whichever
**	is approriate.  This routine is idempotent.  That is, if it is
**	called with the same parameters after multiple previous failures
**	or a successful call, the end result will be the same as if it
**	were called only once successfully.
**
** Inputs:
**      path_name                       Buffer containing the path name of
**					the path where the directory can be
**					located.
**	dir_name			Buffer containing the directory name.
**	du_errcb			Error-handling control block.
**
** Outputs:
**      *du_errcb                       If an error occurs, this block will be
**					set by a call to du_error().
**	Returns:
**	    E_DU_OK			Completed successfully.
**	    W_DU1020_DEL_DIRNOTFOUND	The directory to be deleted was not
**					found.  Note, no error message will
**					be formatted with this return.  It is
**					up to the caller to format the message
**					if desired.
**	    E_DU31A0_BAD_DIRFILE_DELETE	One of the files contained in this
**					directory couldn't be deleted.  This
**					error would result if there were a
**					DIlistfile() error.
**	    E_DU31A2_BAD_FILE_DELETE	One of the files contained in this
**					directory couldn't be deleted.
**	    E_DU31A1_BAD_DIR_DELETE	The directory itself couldn't be
**					deleted.
**	Exceptions:
**	    none
**
** Side Effects:
**	      The directory and all of the files it contains will be deleted.
**
** History:
**      29-Oct-86 (ericj)
**          Initial creation.
[@history_template@]...
*/
DU_STATUS
dub_dir_del(path_name, dir_name, du_errcb)
char		path_name[];
char		dir_name[];
DU_ERROR	*du_errcb;
{

    DI_IO	    dirf;
    i4		    *arg_list[64];
    DUB_PDESC	    path;
    STATUS	    dub_0dirfile_del();
    STATUS	    status;
    CL_SYS_ERR	    err_code;
    LOCATION	    LOdirpath;
    char	    dirpath_buf[DI_PATH_MAX + 1];
    char	    *cp;

    /*	 This routine assumes that the directory being deleted contains
    ** only files and not other directories.  This was an assumption
    ** made in previous database utilities for the DBMS file system
    ** and seems to be pretty safe.
    */

    STcopy(path_name, dirpath_buf);
    LOfroms(PATH, dirpath_buf, &LOdirpath);
    LOfaddpath(&LOdirpath, dir_name, &LOdirpath);

    LOtos(&LOdirpath, &cp);    
    STcopy(cp, path.dub_pname);
    path.dub_plength	= STlength(path.dub_pname);
    arg_list[0]	= (i4 *) path.dub_pname;
    arg_list[1]	= (i4 *) du_errcb;

    status = DIlistfile(&dirf, path.dub_pname, STlength(path.dub_pname),
			dub_0dirfile_del, (PTR)arg_list, &err_code);

    if (status == DI_ENDFILE || status == OK)
    {
	/* If an error occurred in deleting any of the files in the directory
	** to be deleted,
	** with the given status, then there will be an associated DUF error
	** status in the .du_status field of the error handling control block.
	** Otherwise it will be equal to E_DU_OK.
	*/
	if (du_errcb->du_status != E_DU_OK)
	{
	    /* An operating system error or CL error has occurred.  Report it
	    ** and take an error exit.
	    */
	    du_errcb->du_clerr    = status;
	    STRUCT_ASSIGN_MACRO(err_code, du_errcb->du_clsyserr);
	    return(du_error(du_errcb, E_DU31A0_BAD_DIRFILE_DELETE, 6,
			    (i4) sizeof (STATUS), &status,
			    (i4) 0, path_name,
			    (i4) 0, dir_name));
	}
	else
	    du_reset_err(du_errcb);
    }
    else if (status == DI_DIRNOTFOUND)
    {
	/* There was no directory to delete.  Send back a warning number
	** to the caller.  It will be up to the caller to format and print
	** a warning message or translate the warning to ok.
	*/
	return(W_DU1020_DEL_DIRNOTFOUND);
    }
    else
    {
	/* An operating system error or CL error of some sort has been
	** encountered.
	*/
	du_errcb->du_clerr    = status;
	STRUCT_ASSIGN_MACRO(err_code, du_errcb->du_clsyserr);
	return(du_error(du_errcb, E_DU31A0_BAD_DIRFILE_DELETE, 6,
			(i4) sizeof (STATUS), &status,
			(i4) 0, path_name,
			(i4) 0, dir_name));
    }
    

    /* Now, delete the directory. */
    du_errcb->du_clerr= DIdirdelete(&dirf, path_name, STlength(path_name),
				    dir_name, STlength(dir_name),
				    &du_errcb->du_clsyserr);

    if (du_errcb->du_clerr != OK)
    {
	return(du_error(du_errcb, E_DU31A1_BAD_DIR_DELETE, 4,
			(i4) 0, path_name,
			(i4) 0, dir_name));
    }

    return(E_DU_OK);
}


/*{
** Name: dub_0dirfile_del -	interface between dub_dir_del() and
**				dub_file_del()
**
** Description:
**        This routine is just used as an interface to the general purpose
**	file deletion routine, dub_file_del().  Its interface is formatted
**	so that it can be called indirectly by dub_dir_del() through the
**	DIlistfile() routine.
**
** Inputs:
**      arg_list                        Ptr to an array of ptrs for passing
**					various arguments as defined below:
**	    [0]				Ptr to a buffer containing the
**					path of the file to be deleted.
**	    [1]				Ptr to a DU_ERROR struct for error-
**					handling.
**	fname				Name of file to be deleted.
**	flength				Length of fname.
**	err_code			Ptr to a CL_SYS_ERR struct to store
**					operating system error status.
**
** Outputs:
**      *err_code                       If an operating system error occurred.
**					while making a DI call, the
**					corresponding error status is placed
**					here.
**	*arg_list
**	    [1]				If an error occurred, a call to 
**					du_error() will set this error-handling
**					control block.
**	Returns:
**	    OK				Completed successfully.
**	    FAIL			A call to the general purpose file
**					deletion routine, dub_file_del() failed.
**	Exceptions:
**	    none
**
** Side Effects:
**	    A file will be deleted.
**
** History:
**      29-Oct-86 (ericj)
**          Initial creation.
[@history_template@]...
*/
/*ARGSUSED*/
STATIC	    STATUS
dub_0dirfile_del(arg_list, fname, flength, err_code)
i4		*arg_list[];
char		*fname;
i4		flength;
CL_SYS_ERR	*err_code;
{
    DU_ERROR	    *du_errcb;
    char	    filename[DI_FILENAME_MAX + 1];

    du_errcb	= (DU_ERROR *) arg_list[1];
    
    MEcopy((PTR) fname, (u_i2) flength, (PTR) filename);
    filename[flength]	= '\0';
    du_errcb->du_status = dub_file_del((char *) arg_list[0], filename,
				       du_errcb);

    return(du_errcb->du_status == E_DU_OK ? OK : FAIL);
}



/*{
** Name: dub_file_del()	-   delete a DBMS file.
**
** Description:
**        This routine deletes a DBMS file.
**	Note, callers of this routine should explicitly check for the
**	warning error number, W_DU1020_DEL_DIRNOTFOUND, and call
**	du_error() to format a message or du_reset_err() to ignore
**	error, whichever is approriate.
**
** Inputs:
**      path                            Buffer containing the directory path
**					of the file to be deleted.
**	file				Buffer containing the name of the file
**					to be deleted.
**      du_errcb                        Ptr to error-handling control block.
**
** Outputs:
**      *du_errcb                       If an error occurs, this struct will be
**					initialized by a call to du_error().
**	    .du_clerr			Set if this routine encounters a CL
**					error.
**	    .du_clsyserr		Set if this routine calls a CL routine
**					that encouters an operating system
**					error.
**	Returns:
**	    E_DU_OK			Completed successfully.
**	    W_DU1020_DEL_FILENOTFOUND	The file to be deleted was not
**					found.  Note, no error message will
**					be formatted with this return.  It is
**					up to the caller to format the message
**					if desired.
**	    W_DU1021_DEL_DIRNOTFOUND	The file to be deleted was not
**					found because the directory path
**					given did not exist.
**					Note, no error message will
**					be formatted with this return.  It is
**					up to the caller to format the message
**					if desired.
**	    E_DU31A2_BAD_FILE_DELETE	The file couldn't be deleted.
**	Exceptions:
**	    none
**
** Side Effects:
**	    The given DBMS file is deleted.
**
** History:
**      29-Oct-86 (ericj)
**          Initial creation.
**      15-Feb-87 (ericj)
**	    Updated for new DI specs.
[@history_template@]...
*/
DU_STATUS
dub_file_del(path, file, du_errcb)
char		path[];
char		file[];
DU_ERROR	*du_errcb;
{

    DI_IO	    file_io;

    du_errcb->du_clerr	= DIdelete(&file_io, path, STlength(path),
				   file, STlength(file),
				   &du_errcb->du_clsyserr);

    if (du_errcb->du_clerr != OK && du_errcb->du_clerr != DI_FILENOTFOUND
	&& du_errcb->du_clerr != DI_DIRNOTFOUND
       )
    {
	return(du_error(du_errcb, E_DU31A2_BAD_FILE_DELETE, 4,
			0, path, 0, file));
    }
    else if (du_errcb->du_clerr == DI_FILENOTFOUND)
    {
	/* There was no file to delete.  Send back a warning number
	** to the caller.  It will be up to the caller to format and print
	** a warning message or translate the warning to ok.
	*/
	return(W_DU1021_DEL_FILENOTFOUND);
    }
    else if (du_errcb->du_clerr == DI_DIRNOTFOUND)
    {
	/* There was no file to delete because the directory given
	** did not exist.  Send back a warning number
	** to the caller.  It will be up to the caller to format and print
	** a warning message or translate the warning to ok.
	*/
	return(W_DU1020_DEL_DIRNOTFOUND);
    }

    return(E_DU_OK);
}
