/*
** Copyright (c) 1986, 2008  Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <lo.h>
#include    <di.h>
#include    <me.h>
#include    <st.h>
#include    <ck.h>
#include    <ssdef.h>
#include    <descrip.h>
#include    <syidef.h>
#include    <rms.h>
#include    <er.h>
#include    <si.h>
#include    <lib$routines.h>
#include    <starlet.h>

#include    <astjacket.h>

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
**	    CKbegin - Mark the beginning of a checkpoint.
**	    CKend - Mark the end of a checkpoint.
**
**
**  History:    $Log-for RCS$
**      01-nov-1986 (Derek)
**	    Revised for Jupiter.
**      27-mar-1987 (Derek)
**          Added new routines to manipulate directories.
**	10-mar-1989 (Greg)
**	    Fixed specification errors and insure CL_ERR_DESC member
**	    error is always initialized.
**	24-oct-1989 (rogerk)
**	    Added fix for online backup problem where directory entries
**	    for temp tables may be added/deleted while directory is being
**	    backed up.  Added command qualifier to backup command to
**	    exclude the temporary tables.
**	23-mar-1990 (walt)
**	    Changed CKsave, CKrestore, CKbegin, CKend to work the way that
**	    UNIX now works - get the commands from a user-provided file
**	    rather than manufacture them here. CKbegin and CKend were taken
**	    whole from the UNIX cl.  CKsave and CKrestore here in VMS were
**	    kept because they contain VMS file-handling statements.  The
**	    appropriate calls to CK_subst and CK_spawn from the UNIX cl
**	    replace lines that manufactured and spawned commands.
**	30-may-1990 (walt)
**	    Re-instate code in CKbegin, CKsave, CKrestore, CKend that adds
**	    a colon to tape device names that don't have one.
**	25-sep-1990 (walt)
**	    (To fix bug 33419)
**	    Remove SYSPRV-setting code from CKrestore, CKsave and move it
**	    into CK_spawn [ckspawn.c].  This is being done because CKbegin
**	    and CKend need to have the same functionality.  Rather than
**	    replicate the code two more times here in this file, I'm just
**	    moving it to a common place for them all to use.
**	26-apr-1993 (bryanp)
**	    Added prototypes for 6.5.
**	26-apr-1993 (bryanp)
**	    arg_list is a PTR, not a (PTR *).
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**	19-jul-2000 (kinte01)
**	    Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**		Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**		from VMS CL as the use is no longer allowed
**	07-feb-01 (kinte01)
**	    Add casts to quiet compiler warnings
**	11-nov-2008 (joea)
**	    Use CL_CLEAR_ERR to initialize CL_ERR_DESC.
**      22-dec-2008 (stegr01)
**          Itanium VMS port.
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
**      This routine deletes the named checkpoint file.
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
*/
STATUS
CKdelete(
char                *path,
i4		    l_path,
char		    *filename,
i4		    l_filename,
CL_ERR_DESC	    *sys_err)
{
    i4	          s;                    /* Request return status. */
    struct FAB    fab;               /* File access block. */
    char	  esa[128];
  
    CL_CLEAR_ERR(sys_err);
    
    /* Check input parameters. */

    if (l_path + l_filename > sizeof(esa)) 
	return (CK_BAD_PARAM);		

    /* Create full lookup name for old file name. */

    MEcopy(path, l_path, esa);
    MEcopy(filename, l_filename, &esa[l_path]);

    MEfill(sizeof(fab), 0, (char *)&fab);
    fab.fab$b_fns = l_path + l_filename;
    fab.fab$l_fna = esa;
    fab.fab$b_bid = FAB$C_BID;
    fab.fab$b_bln = FAB$C_BLN;

    /* Now delete the file. */    

    s = sys$erase(&fab);
    if ((s & 1) == 0)
    {
        sys_err->error = s;
	if (s == RMS$_DNF)
	    return(CK_BADPATH);
	if (s == RMS$_FNF)
	    return (CK_NOTFOUND);
	return (CK_BADDELETE);
    }
    return (OK);
}

/*{
** Name: CKdircreate	- Create a checkpoint directory.
**
** Description:
**      The CKdircreate creates the 'dirname' directory
**	at the location 'path'.
**      
**   
** Inputs:
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
CL_ERR_DESC    *err_code)
{
    DI_IO	d;
    STATUS	status;

    CL_CLEAR_ERR(err_code);
    
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
**	DI calls). This call should fail with CK_BADDELETE if any files 
**	still exists in the directory.
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

    CL_CLEAR_ERR(err_code);
    
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
**	If the function returns anything but OK, then CKlistfile
**	will return CK_BADLIST.
**      The function must have the following call format:
**
**          STATUS
**          funct_name(arg_list, filename, filelength, err_code)
**          PTR        arg_list;       Pointer to argument list 
**          char       *filename;       Pointer to directory name.
**          i4        filelength;      Length of directory name. 
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

    CL_CLEAR_ERR(err_code);
    
    status = DIlistfile(&d,path, pathlength, func, arg_list, err_code);
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
** Name: CKrestore	- Restore a database from a (set of) save file.
**
** Description:
**      This routine takes a CKsave file or tape, containing a set of DI files
**	and restores the contents to the given database directory.
**      Duplicate files should not occur since there should be no duplicate 
**      file names in an INGRES directory.  If duplicate files are found 
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
**          and should start with CK_.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      01-nov-1986 (Derek)
**          Revised.
**	14-nov-1988 (Sandyh)
**	    Added VMS 5.0 support and rollforward from tape fixes.
**	23-mar-1990 (walt)
**	    Changed to use CK_subst/CK_spawn like the UNIX cl.  This allows
**	    the user to specify the commands.
**	30-may-1990 (walt)
**	    Re-instate code that adds a colon to tape device names that
**	    don't have one.
**	25-sep-1990 (walt)
**	    (To fix bug 33419)
**	    Remove SYSPRV-setting code because it's now in CK_spawn [ckspawn.c].
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
    char	    command[256];
    i4	    status;
    i4	    lib_status;
    i4	    backup_status;
    i4	    ast_status;
    i4	    spawn_flag = 2;
    i4	    ctrl_y_mask = 0x02000000;
    $DESCRIPTOR(nulldev, "NLA0:");
    struct
    {
	i4	    length;
	char	    *ptr;
    }		    cmd_desc = { 0, command };
    char	bck_str[256];
    i4	bck_len;
    char	devname[80];
    LOCATION	outpath;
    static int	first_pass = 1;
    bool	vms_5;

    STATUS	ret_val = OK;
    
    CL_CLEAR_ERR(sys_err);
    
    /*	Check arguments. */

    if ((type != CK_DISK_FILE && type != CK_TAPE_FILE) ||
	(ckp_l_path + ckp_l_file + di_l_path + 46 > sizeof(command)))
    {
	return (CK_BAD_PARAM);
    }

    /*	Disable AST delivery and DCL control-Y interrupts. */
    
    ast_status = sys$setast(0);
    lib$disable_ctrl(&ctrl_y_mask, 0);

    for (;;)
    {
        ++CK_loccnt;

	/* Add colon to tape device specified, if missing. */

	if (type == CK_TAPE_FILE)
	{
	    char	*colon;

	    colon = STindex(ckp_path, ERx(":"), ckp_l_path);
	    if (colon == NULL)
	    {
		ckp_path[ckp_l_path] = ':';
		ckp_l_path++;
	    }
	}
	
        ret_val = CK_subst(command, MAXCOMMAND, 'W',
			   type == CK_TAPE_FILE ? 'T' : 'D', 'R',
			   type == CK_TAPE_FILE, CK_loccnt, di_l_path, di_path,
			   ckp_l_path, ckp_path, ckp_l_file, ckp_file);

	if (!ret_val)
	{
	    ret_val = CK_spawn(command, sys_err);
	}

	break;
    }

    /*  Enable control-Y and control-C interrupts. */

    lib$enable_ctrl(&ctrl_y_mask, 0);
    if (ast_status == SS$_WASSET)
	sys$setast(1);

    sys_err->error = ret_val;
    return (ret_val);
}

/*{
** Name: CKsave	- Save database directory into a DI file.
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
**          and should start with CK_.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      01-nov-1986 (Derek)
**          Revised.
**	14-nov-1988 (Sandyh)
**	    Added VMS 5.0 support and checkpoint to tape fixes.
**	24-oct-1989 (rogerk)
**	    Added fix for online backup problem where directory entries
**	    for temp tables may be added/deleted while directory is being
**	    backed up.  Added command qualifier to backup command to
**	    exclude the temporary tables.  This was done as a quick fix
**	    to the problem.  A better non-cl-dependent solution would be
**	    to add a separate data directory (ingres/tmp) to hold temp files.
**	23-mar-1990 (walt)
**	    Changed to use CK_subst/CK_spawn like the UNIX cl.  This allows
**	    the user to specify the commands.
**	30-may-1990 (walt)
**	    Re-instate code that adds a colon to tape device names that
**	    don't have one.
**	25-sep-1990 (walt)
**	    (To fix bug 33419)
**	    Remove SYSPRV-setting code because it's now in CK_spawn [ckspawn.c].
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
    char	    command[256];
    i4	    status;
    i4	    lib_status;
    i4	    backup_status;
    i4	    ast_status;
    i4	    spawn_flag = 2;
    i4	    ctrl_y_mask = 0x02000000;
    $DESCRIPTOR(nulldev, "NLA0:");
    struct
    {
	i4	    length;
	char	    *ptr;
    }		    cmd_desc = { 0, command };
    char	bck_str[256];
    int		bck_len;
    char	devname[80];
    static int	first_pass = 1;
    bool	vms_5;

    STATUS	ret_val = OK;

    CL_CLEAR_ERR(sys_err);
    
    /*	Check arguments. */

    if ((type != CK_DISK_FILE && type != CK_TAPE_FILE) ||
	(ckp_l_path + ckp_l_file + di_l_path + 88 > sizeof(command)))
    {
	return (CK_BAD_PARAM);
    }

    /*	Disable AST delivery and DCL control-Y interrupts. */
    
    ast_status = sys$setast(0);
    lib$disable_ctrl(&ctrl_y_mask, 0);

    for (;;)
    {
	++CK_loccnt;

	/* Add colon to tape device specified, if missing. */

	if (type == CK_TAPE_FILE)
	{
	    char	*colon;

	    colon = STindex(ckp_path, ERx(":"), ckp_l_path);
	    if (colon == NULL)
	    {
		ckp_path[ckp_l_path] = ':';
		ckp_l_path++;
	    }
	}
	
	ret_val = CK_subst(command, MAXCOMMAND, 'W',
			   type == CK_TAPE_FILE ? 'T' : 'D', 'S',
			   type == CK_TAPE_FILE, CK_loccnt, di_l_path, di_path,
			   ckp_l_path, ckp_path, ckp_l_file, ckp_file);


	if (!ret_val)
	{
	    ret_val = CK_spawn(command, sys_err);
	}

	break;
    }

    /*  Enable control-Y and control-C interrupts. */

    lib$enable_ctrl(&ctrl_y_mask, 0);
    if (ast_status == SS$_WASSET)
	sys$setast(1);

    sys_err->error = ret_val;
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
**	oper				One of 'S' (save) or 'R' (restore)
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
**          and should start with CK_.
**	    CK_FILENOTFOUND		From CK_subst if unable to open
**	    template file.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      19-nov-88 (sandyh)
**          Created. To be used in a later release.
**	23-mar-1990 (walt)
**	    Brought over code from UNIX cl.
**	20-may-1990 (walt)
**	    Nolonger return the ret_val value in sys_err->error.
**	30-may-1990 (walt)
**	    Insert code that adds a colon to tape device names that
**	    don't have one.
*/
STATUS
CKbegin(
u_i4	    type,
char	    oper,
char	    *ckp_path,
u_i4	    ckp_l_path,
u_i4	    numlocs,
CL_ERR_DESC *sys_err)
{
    char	command[MAXCOMMAND];
    STATUS	ret_val = OK;
    
    CK_numlocs = numlocs;
    CK_loccnt = 0;

    /* Add colon to tape device specified, if missing. */

    if (type == CK_TAPE_FILE)
    {
	char	*colon;

	colon = STindex(ckp_path, ERx(":"), ckp_l_path);
	if (colon == NULL)
	{
	    ckp_path[ckp_l_path] = ':';
	    ckp_l_path++;
	}
    }
	
    ret_val = CK_subst(command, MAXCOMMAND, 'B',
		       type == CK_TAPE_FILE ? 'T' : 'D', oper, 
		       type == CK_TAPE_FILE, CK_numlocs, 0, "",
		       ckp_l_path, ckp_path, 0, "");

    if (!ret_val)
    {
	ret_val = CK_spawn(command, sys_err);
    }

    sys_err->error = 0;
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
**	oper				One of 'S' (save) or 'R' (restore)
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
**          and should start with CK_.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      19-nov-88 (sandyh)
**          Created. To be used in a later release.
**	23-mar-1990 (walt)
**	    Brought over code from UNIX cl.
**	30-may-1990 (walt)
**	    Insert code that adds a colon to tape device names that
**	    don't have one.
*/
STATUS
CKend(
u_i4	    type,
char	    oper,
char	    *ckp_path,
u_i4	    ckp_l_path,
CL_ERR_DESC *sys_err)
{
    char	command[MAXCOMMAND];
    STATUS	ret_val = OK;
    
    /* Add colon to tape device specified, if missing. */

    if (type == CK_TAPE_FILE)
    {
	char	*colon;

	colon = STindex(ckp_path, ERx(":"), ckp_l_path);
	if (colon == NULL)
	{
	    ckp_path[ckp_l_path] = ':';
	    ckp_l_path++;
	}
    }
	
    ret_val = CK_subst(command, MAXCOMMAND, 'E',
		       type == CK_TAPE_FILE ? 'T' : 'D', oper, 
		       type == CK_TAPE_FILE, CK_numlocs, 0, "",
		       ckp_l_path, ckp_path, 0, "");

    if (!ret_val)
    {
	ret_val = CK_spawn(command, sys_err);
    }

    sys_err->error = 0;
    return(ret_val);
}
