/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <er.h>
#include    <pc.h>
#include    <st.h>
#include    <si.h>
#include    <di.h>
#include    <ck.h>
#include    <cm.h>
#include    <errno.h>

/**
**
**  Name: CK.C - Checkpoint Save/Restore routines.
**
**  Description:
**      This routine implement checkpointing DI files.
**
**	    CKdelete - Delete a checkpoint.
**	    CKdircreate - Create a checkpoint directory.
**	    CKdirdelete - Delete a checkpoint directory.
**	    CKlistfiles - List file in checkpoint directory.
**          CKrestore - Restore DI files from a checkpoint.
**	    CKsave - Write DI file to a checkpoint.
**	    CKbegin - mark beginning of a checkpoint.
**	    CKend - mark ending of a checkpoint.
**
**
** History:
**      01-nov-1986 (Derek)
**	    Revised for Jupiter.
**      27-mar-1987 (Derek)
**          Added new routines to manipulate directories.
**      24-jul-1987 (mikem)
**          Updated to meet jupiter coding specs and to install stubs in some
**          routines until they can be written.
**      09-may-1988 (anton)
**          TRdisplay implementation
**      08-jun-1988 (anton)
**          checkpoint and rolldb on disk files works
** 	05-aug-1988 (roger)
** 	    UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
**      08-dec-1988 (jeff)
** 	    add CKbegin and CKend
**      24-jan-1989 (roger)
** 	    Make use of new CL error descriptor.
**      24-jan-1989 (mikem)
** 	    SETCLERRD -> SETCLERR
**	18-sep-89 (russ)
**		Add ODT specific code for checkpoint to floppy.
**	17-jan-90 (russ)
**		Add sco_us5 to odt_us5 case for special checkpoint to
**		floppy.
**	09-may-90 (blaise)
**		Integrated 61 changes into 63p library.
**	29-oct-1991 (mikem)
**	   Deleted SIprintf() from CKdelete(), as this routine is now called
**	   from the server where SIprintf's will not be sent to the user thread
**	   which issued them.
**	26-apr-1993 (bryanp)
**	    Prototyping CK for 6.5.
**	26-apr-1993 (bryanp)
**	    arg_list is a PTR, not a (PTR *)
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      04-aug-1995 (harpa06)
**          Change '/' to '\' for Windows NT in CKdelete().
**	05-dec-95 (emmag)
**	    CKdelete: Only need to add a trailing slash to the path if a file
**	    is to be appended. 
**	17-may-96 (hanch04)
**	    Added <pc.h>
**      14-may-97 (mcgem01)
**          Clean up compiler warnings.
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	10-may-1999 (walro03)
**	    Remove obsolete version string odt_us5.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	4-Dec-2008 (kschendel) b122041
**	    Declare errno correctly (i.e. via errno.h).
**/

/*
**  Definitions of local static variables.
*/

static	i4	CK_numlocs, CK_loccnt;

/*
**  Definitions of local constants.
*/

#define	MAXCOMMAND	5120

/*{
** Name: CKdelete	- Delete a CK checkpoint.
**
** Description:
**      This routine deletes a name checkpoint file.
**
** Inputs:
**      path                            Pointer to device path.
**	l_path				Length of device path.
**	filename			Pointer to file name.
**	l_filename			Length of file name.
**
** Outputs:
**      sys_err                         Reason for error return.
**	Returns:
**	    OK
**	    CK_NOTFOUND
**	    CK_BADPATH
**	    CK_BAD_PARAM
**	    CK_BADDELETE
**	    
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-mar-1987 (Derek)
**          Created.
**	29-oct-1991 (mikem)
**	   Deleted SIprintf() from this routine, as this routine is now called
**	   from the server in the case of destroydb.
**	04-aug-1995 (harpa06)
**	   Change '/' to '\' for Windows NT.
**	05-dec-1995 (emmag)
**	    Only append a trailing slash to the directory name if a 
**	    file is to be appended to it.  (BUG 70655 on NT 3.51)
*/
STATUS
CKdelete(
char                *path,
i4		    l_path,
char		    *filename,
i4		    l_filename,
CL_ERR_DESC	    *sys_err)
{
    char	ckpfile[142];
  
    /* Check input parameters. */

    if (l_path + l_filename > sizeof(ckpfile)) 
        return (CK_BAD_PARAM);

    STncpy(ckpfile, path, l_path);
    ckpfile[ l_path ] = EOS;
    if (l_filename > 0)
    {
# ifdef DESKTOP
	STcat(ckpfile, "\\");
# else 
	STcat(ckpfile, "/");
# endif 
        STncat(ckpfile, filename, l_filename);
    }


    if (unlink(ckpfile))
    {
	SETCLERR(sys_err, 0, ER_unlink);
	return(CK_NOTFOUND);
    }
    return (OK);
}

/*{
** Name: CKdircreate	- Create a checkpoint directory.
**
** Description:
**      The CKdircreate will take a path name and directory name 
**      and create a directory in this location.  
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
**        CK_BAD_DIR         Path specification error.
**        CK_BAD_PARAM       Parameter(s) in error.
**        CK_EXISTS          Already exists.
**        CK_DIR_NOT_FOUND    Path not found.
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**	26-mar-1987 (Derek)
**	    Created.
*/
STATUS
CKdircreate(
char           *path,
u_i4          pathlength,
char           *dirname,
u_i4          dirlength,
CL_ERR_DESC     *err_code)
{
    DI_IO	d;
    STATUS	status;

    status = DIdircreate(&d, path, pathlength, dirname, dirlength, err_code);
    if (status == OK)
	return (status);
    if (status == DI_BADDIR)
	status = CK_BADDIR;
    else if (status == DI_BADPARAM)
	status = CK_BAD_PARAM;
    else if (status == DI_EXISTS)
	status = CK_EXISTS;
    else if (status == DI_DIRNOTFOUND)
	status = CK_DIRNOTFOUND;
    else
	status = CK_BAD_PARAM;
    return (status);
}

/*{
** Name: CKdirdelete  - Deletes a checkpoint directory.
**
** Description:
**      The CKdirdelete routine is used to delete a direct access 
**      directory.  The name of the directory to delete should
**      be specified as path and dirname, where dirname does not
**      include a type qualifier or version and the path does
**	not include the directory name (which it normally does for
**	other DI calls).  The directory name may be  converted to a filename
**	of the directory and use DIdelete to delete it.
**      This call should fail with CK_BADDELETE if any files still exists
**      in the directory.
**
** Inputs:
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
**	  CK_DIR_NOT_FOUND   Path not found.
**	  CK_NOT_FOUND	     File not found.
**        CK_BAD_PARAM       Parameter(s) in error.
**        CK_BAD_DELETE      Error trying to delete directory.
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**	26-mar-1987 (Derek)
**	    Created.
*/
STATUS
CKdirdelete(
char           *path,
u_i4          pathlength,
char           *dirname,
u_i4          dirlength,
CL_ERR_DESC     *err_code)
{
    DI_IO	d;
    STATUS	status;

    status = DIdirdelete(&d, path, pathlength, dirname, dirlength, err_code);
    if (status == OK)
	return (status);
    if (status == DI_BADPARAM)
	status = CK_BAD_PARAM;
    else if (status == DI_DIRNOTFOUND)
	status = CK_DIRNOTFOUND;
    else if (status == DI_FILENOTFOUND)
	status = CK_FILENOTFOUND;
    else if (status == DI_BADDELETE)
	status = CK_BADDELETE;
    else
	status = CK_BAD_PARAM;
    return (status);
}

/*{
** Name: CKlistfile - List all files in a journal directory.
**
** Description:
**      The CKlistfile routine will list all files that exist
**      in the directory(path) specified.  This routine expects
**      as input a function to call for each file found.  This
**      insures that all resources tied up by this routine will 
**      eventually be freed.  The files are not listed in any
**      specific order. The function passed to this routine
**      must return OK if it wants to continue with more files,
**      or a value not equal to OK if it wants to stop listing.
**	If the function returns anything by OK, then CKlistfile
**	will return CK_BADLIST.
**      The function must have the following call format:
**
**          STATUS
**          funct_name(arg_list, filename, filelength, err_code)
**          PTR        arg_list;       Pointer to argument list 
**          char       *filename;       Pointer to directory name.
**          i4         filelength;      Length of directory name. 
**          CL_ERR_DESC *err_code;       Pointer to operating system
**                                      error codes (if applicable). 
**   
** Inputs:
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
**        CK_BAD_DIR         Path specification error.
**        CK_END_FILE        Error returned from client handler or
**                           all files listed.
**        CK_BAD_PARAM       Parameter(s) in error.
**        CK_DIR_NOT_FOUND   Path not found.
**        CK_BAD_LIST        Error trying to list objects.
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**	26-mar-1987 (Derek)
**	    Created.
**	26-apr-1993 (bryanp)
**	    arg_list is a PTR, not a (PTR *).
*/
STATUS
CKlistfile(
char           *path,
u_i4          pathlength,
STATUS         (*func)(),
PTR            arg_list,
CL_ERR_DESC     *err_code)
{
    DI_IO	d;
    STATUS	status;

    status = DIlistfile(&d, path, pathlength, func, arg_list, err_code);
    if (status == OK)
	return (status);
    if (status == DI_BADPARAM)
	status = CK_BAD_PARAM;
    else if (status == DI_DIRNOTFOUND)
	status = CK_DIRNOTFOUND;
    else if (status == DI_BADDIR)
	status = CK_BADDIR;
    else if (status == DI_BADLIST)
	status = CK_BADLIST;
    else if (status == DI_ENDFILE)
	status = CK_ENDFILE;
    else
	status = CK_BAD_PARAM;
    return (status);
}

/*{
** Name: CKrestore	- Restore a DI directory from a CK file.
**
** Description:
**      This routine takes a CKsave file or tape, containing a set of DI files
**	and restores all the file in the save file to the given DI directory.
**      Duplicates files should not occur since there should be no duplicate 
**      file names in an INGRES directory.  If duplicates files are found 
**      during the restore no specific action is required.  Specifically
**      this condition is not checked by the caller.
**
** Inputs:
**      ckp_path                        Pointer to ckp file path.
**	ckp_l_path			Length of ckp file path.
**	ckp_file			Pointer to ckp file name.
**	ckp_l_file			Length of ckp file name.
**      type                            Checkpoint device type.
**					Either CK_DISK_FILE or CK_TAPE_FILE.
**      di_path                         Pointer to DI path.
**	di_l_path			Length of DI path.
**
** Outputs:
**      sys_err				Reason for error status.
**	Returns:
**	    OK
**	    CK_BAD_PARAM		File name too long, or bad type.
**          Any operating system specific error that can be looked
**          up in the error message file.  These can be any conditions
**          and should start with CK_.  The associated text must be 
**          supplied in the ERCLF.TXT file found in the ER source
**          directory.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      01-nov-1986 (Derek)
**          Revised.
**	27-oct-1988 (anton)
**	    use template to build command
[@history_template@]...
*/
STATUS
CKrestore(
char                *ckp_path,
u_i4		    ckp_l_path,
char		    *ckp_file,
u_i4		    ckp_l_file,
u_i4		    type,
char		    *di_path,
u_i4		    di_l_path,
CL_ERR_DESC	    *sys_err)
{
    char	command[MAXCOMMAND];
    STATUS	ret_val = OK;
    
    ++CK_loccnt;

# if defined(sco_us5)
    if (type == CK_TAPE_FILE)
    {
	if (CMdigit(ckp_path))
	{
		ret_val = CK_subst(command, MAXCOMMAND, 'W',
			'F', 'R', 1, CK_loccnt, di_l_path, di_path,
			ckp_l_path, ckp_path, ckp_l_file, ckp_file);
	}
	else
	{
		ret_val = CK_subst(command, MAXCOMMAND, 'W',
			'T', 'R', 1, CK_loccnt, di_l_path, di_path,
			ckp_l_path, ckp_path, ckp_l_file, ckp_file);
	}
    }
    else
    {
	ret_val = CK_subst(command, MAXCOMMAND, 'W',
		'D', 'R', 0, CK_loccnt, di_l_path, di_path,
		ckp_l_path, ckp_path, ckp_l_file, ckp_file);
    }
# else /* sco_us5 */
    ret_val = CK_subst(command, MAXCOMMAND, 'W',
		       type == CK_TAPE_FILE ? 'T' : 'D', 'R',
		       type == CK_TAPE_FILE, CK_loccnt, di_l_path, di_path,
		       ckp_l_path, ckp_path, ckp_l_file, ckp_file);
# endif /* sco_us5 */

    if (!ret_val)
	ret_val = CK_spawn(command, sys_err);

    return(ret_val);
}

/*{
** Name: CKsave	- Save DI directory into a CK file.
**
** Description:
**	This routine takes a directory of DI files, and writes them to a
**	single CKsave file or tape.
**
** Inputs:
**      di_path                         Pointer to DI directory path.
**      di_l_path                       Length of DI directory path.
**      type                            The type of output device.
**					Either CK_DISK_FILE or CK_TAPE_FILE.
**      ckp_path                        Pointer to checkpoint path.
**	ckp_l_path			Length of checkpoint path.
**	ckp_file			Pointer to checkpoint file name.
**	ckp_l_file			Length of checkpoint file name.
**
** Outputs:
**      sys_err                         Reason for error return status.
**	Returns:
**	    OK
**	    CK_BAD_PARAM		File name too long, or bad type.
**          Any operating system specific error that can be looked
**          up in the error message file.  These can be any conditions
**          and should start with CK_.  The associated text must be 
**          supplied in the ERCLF.TXT file found in the ER source
**          directory.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      01-nov-1986 (Derek)
**          Revised.
**	27-oct-1988 (anton)
**	    use template to build command
[@history_template@]...
*/
STATUS
CKsave(
char		    *di_path,
u_i4		    di_l_path,
u_i4		    type,
char                *ckp_path,
u_i4		    ckp_l_path,
char		    *ckp_file,
u_i4		    ckp_l_file,
CL_ERR_DESC	    *sys_err)
{
    char	command[MAXCOMMAND];
    STATUS	ret_val = OK;
    
    ++CK_loccnt;

# if defined(sco_us5)
    if (type == CK_TAPE_FILE)
    {
	if (CMdigit(ckp_path))
	{
		ret_val = CK_subst(command, MAXCOMMAND, 'W',
			'F', 'S', 1, CK_loccnt, di_l_path, di_path,
			ckp_l_path, ckp_path, ckp_l_file, ckp_file);
	}
	else
	{
		ret_val = CK_subst(command, MAXCOMMAND, 'W',
			'T', 'S', 1, CK_loccnt, di_l_path, di_path,
			ckp_l_path, ckp_path, ckp_l_file, ckp_file);
	}
    }
    else
    {
	ret_val = CK_subst(command, MAXCOMMAND, 'W',
		'D', 'S', 0, CK_loccnt, di_l_path, di_path,
		ckp_l_path, ckp_path, ckp_l_file, ckp_file);
    }
# else /* sco_us5 */
    ret_val = CK_subst(command, MAXCOMMAND, 'W',
		       type == CK_TAPE_FILE ? 'T' : 'D', 'S',
		       type == CK_TAPE_FILE, CK_loccnt, di_l_path, di_path,
		       ckp_l_path, ckp_path, ckp_l_file, ckp_file);
# endif /* sco_us5 */

    if (!ret_val)
	ret_val = CK_spawn(command, sys_err);

    return(ret_val);
}

/*{
** Name: CKbegin()	- begin checkpointing of a database
**
** Description:
**	Inform CK that checkpoints are about to be written or read from
**	a device.  This allows kind operator intervention and the ability
**	to deal properly with some tape drives.
**
** Inputs:
**      type                            The type of output device.
**					Either CK_DISK_FILE or CK_TAPE_FILE.
**      ckp_path                        Pointer to checkpoint path.
**	ckp_l_path			Length of checkpoint path.
**	numlocs				number of locations begin checkpointed
**
** Outputs:
**      status only
**
**	Returns:
**	    OK
**	    CK_BAD_PARAM		File name too long, or bad type.
**          Any operating system specific error that can be looked
**          up in the error message file.  These can be any conditions
**          and should start with CK_.  The associated text must be 
**          supplied in the ERCLF.TXT file found in the ER source
**          directory.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      19-may-88 (anton)
**          Created.
**	27-oct-1988 (anton)
**	    use template to build command
*/
STATUS
CKbegin(
u_i4	type,
char	oper,
char	*ckp_path,
u_i4	ckp_l_path,
u_i4	numlocs,
CL_ERR_DESC *sys_err)
{
    char	command[MAXCOMMAND];
    STATUS	ret_val = OK;
    
    CK_numlocs = numlocs;
    CK_loccnt = 0;

# if defined(sco_us5)
    if (type == CK_TAPE_FILE)
    {
	if (CMdigit(ckp_path))
	{
		ret_val = CK_subst(command, MAXCOMMAND, 'B',
			'F', oper, 1, CK_numlocs, 0, "",
			ckp_l_path, ckp_path, 0, "");
	}
	else
	{
		ret_val = CK_subst(command, MAXCOMMAND, 'B',
			'T', oper, 1, CK_numlocs, 0, "",
			ckp_l_path, ckp_path, 0, "");
	}
    }
    else
    {
	ret_val = CK_subst(command, MAXCOMMAND, 'B',
		'D', oper, 0, CK_numlocs, 0, "",
		ckp_l_path, ckp_path, 0, "");
    }
# else /* sco_us5 */
    ret_val = CK_subst(command, MAXCOMMAND, 'B',
		       type == CK_TAPE_FILE ? 'T' : 'D', oper, 
		       type == CK_TAPE_FILE, CK_numlocs, 0, "",
		       ckp_l_path, ckp_path, 0, "");
# endif /* sco_us5 */

    if (!ret_val)
	ret_val = CK_spawn(command, sys_err);

    return(ret_val);
}

/*{
** Name: CKend()	- end checkpoint operations on a database
**
** Description:
**	Inform CK that checkpointing of a database have concluded.
**
** Inputs:
**      type                            The type of output device.
**					Either CK_DISK_FILE or CK_TAPE_FILE.
**      ckp_path                        Pointer to checkpoint path.
**	ckp_l_path			Length of checkpoint path.
**
** Outputs:
**      output_1                        output1 does this
**
**	Returns:
**	    OK
**	    CK_BAD_PARAM		File name too long, or bad type.
**          Any operating system specific error that can be looked
**          up in the error message file.  These can be any conditions
**          and should start with CK_.  The associated text must be 
**          supplied in the ERCLF.TXT file found in the ER source
**          directory.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      19-may-88 (anton)
**          Created.
**	27-oct-1988 (anton)
**	    use template to build command
*/
STATUS
CKend(
u_i4	type,
char	oper,
char	*ckp_path,
u_i4	ckp_l_path,
CL_ERR_DESC *sys_err)
{
    char	command[MAXCOMMAND];
    STATUS	ret_val = OK;
    
# if defined(sco_us5)
    if (type == CK_TAPE_FILE)
    {
	if (CMdigit(ckp_path))
	{
		ret_val = CK_subst(command, MAXCOMMAND, 'E',
			'F', oper, 1, CK_numlocs, 0, "",
			ckp_l_path, ckp_path, 0, "");
	}
	else
	{
		ret_val = CK_subst(command, MAXCOMMAND, 'E',
			'T', oper, 1, CK_numlocs, 0, "",
			ckp_l_path, ckp_path, 0, "");
	}
    }
    else
    {
	ret_val = CK_subst(command, MAXCOMMAND, 'E',
		'D', oper, 0, CK_numlocs, 0, "",
		ckp_l_path, ckp_path, 0, "");
    }
# else /* sco_us5 */
    ret_val = CK_subst(command, MAXCOMMAND, 'E',
		       type == CK_TAPE_FILE ? 'T' : 'D', oper, 
		       type == CK_TAPE_FILE, CK_numlocs, 0, "",
		       ckp_l_path, ckp_path, 0, "");
# endif /* sco_us5 */

    if (!ret_val)
	ret_val = CK_spawn(command, sys_err);

    return(ret_val);
}
