/*
**    Copyright (c) 1986, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <di.h>
#include    <me.h>
#include    <st.h>
#include    <jf.h>
#include    <iodef.h>
#include    <efndef.h>
#include    <iosbdef.h>
#include    <fibdef.h>
#include    <eventflag.h>
#include    <descrip.h>
#include    <atrdef.h>
#include    <fatdef.h>
#include    <fabdef.h>
#include    <namdef.h>
#include    <xabfhcdef.h>
#include    <rmsdef.h>
#include    <ssdef.h>
#include    <lib$routines.h>
#include    <starlet.h>

/**
**
**  Name: JF.C - Journal file I/O routines
**
**  Description:
**
**          JFclose - Close journal file.
**	    JFcreate - Create journal file.
**	    JFdelete - Delete a journal file.
**	    JFdircreate - Create a journal directory.
**	    JFdirdelete - Delete a journal directory.
**	    JFlistfiles - List files in a journal directory.
**	    JFopen - Open journal file.
**	    JFread - Read journal file.
**	    JFwrite - Write journal file.
**	    JFupdate - Update journal file.
**	    JFtruncate - Truncate previous journal series files.
**
**  History:    $Log-for RCS$
**      24-sep-1986 (Derek)
**	    Created.
**      26-mar-1987 (Derek)
**          Added new routines to manipulate directories.
**	01-nov-1988 (Sandyh)
**	    Added new JFtruncate routine & FAB$M_TRN to JFopen.
**	22-feb-1989 (greg)
**	    CL_ERR_DESC member error must be initialized to OK
**	11-dec-1989 (rogerk)
**	    Fixed error handling bug in iiJFallocate.
**	6-feb-1992 (bryanp)
**	    B42269: When JFcreate returns JF_BAD_ALLOC, delete JF file first.
**	26-apr-1993 (bryanp)
**	    Prototyping JF code for 6.5.
**	26-apr-1993 (jnash)
**	    Add support for journal file block size of 65536 bytes.
**	26-apr-1993 (bryanp)
**	    arg_list is a PTR, not a (PTR *).
**	24-jun-1993 (wolf)
**	    Change JFdelete arguments from i4 to u_i4 to match CL spec
**	    and FUNC_EXTERN declaration.
**      25-jun-1993 (huffman)
**          make it match the jpt_gl_hdr:jf.h
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**	22-jun-1998 (kinte01)
**	    cross-integrate change 436215 from oping12
**          22-jun-1998 (horda03) X-Integration of change 421535.
**             Bug 66331. Rollforwarddb fails on AXP/VMS when the -e
**             flag is specified.
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	02-sep-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**	20-jan-2004 (abbjo03)
**	    Correct change to JFupdate from 02-sep-2003.
**	04-Apr-2008 (jonj)
**	    Get event flag using lib$get_ef() instead of
**	    non-thread-safe CACHE_IO_EF.
**	29-oct-2008 (joea)
**	    Use EFN$C_ENF and IOSB when calling a synchronous system service.
**	11-nov-2008 (joea)
**	    Use CL_CLEAR_ERR to initialize CL_ERR_DESC.
**      04-jun-2009 (horda03) Bug 111687
**          Use fib->fib$l_exsz to determine how many blocks a file has been
**          extended by.
**/

static STATUS	iiJFallocate(
		    JFIO		*jfio,
		    i4		count,
		    CL_ERR_DESC		*err_code);

/*{
** Name: JFclose	- Close journal or dump file.
**
** Description:
**      This routine closes the journal or dump file described by the JFIO context
**	passed to this routine.
**
** Inputs:
**      jfio                            Pointer to journal or dump file context.
**
** Outputs:
**      err_code                        Operating system error.
**	Returns:
**	    OK
**	    JF_BAD_FILE		    Bad file context.
**	    JF_BAD_CLOSE	    Error closing file.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      24-sep-1986 (Derek)
**	    Created.
*/
STATUS
JFclose(
JFIO                *jfio,
CL_ERR_DESC	    *err_code)
{
    JFIO	    *j = jfio;		    /* Local copy. */
    int		    s;                      /* Request return status. */

    CL_CLEAR_ERR(err_code);

    /*	Check for valid control block. */

    if (j->jf_ascii_id == JF_ASCII_ID) 
    {
	/*	Close the file. */

	s = sys$dassgn(j->jf_channel);
	if ((s & 1))
	{
	    j->jf_ascii_id = 0;
	    return (OK);
	}

	err_code->error = s;
	return (JF_BAD_CLOSE);
    }

    err_code->error = OK;
    return (JF_BAD_FILE);
}

/*{
** Name: JFcreate	- Create a journal or dump file.
**
** Description:
**      This routine creates and allocates a journal or dump file for processing.
**      This file is created in the area specified by device/path name.
**
**	JFcreate first creates the file, then allocates the indicated amount
**	of disk space. There are circumstances under which the file creation
**	succeeds, but the disk space allocation fails (for example, the disk
**	may be too full to allow the allocation, but may have enough room in
**	the directory to allow the creation of the file).
**
**	In this case, we return the error code JF_BAD_ALLOC. However, it is
**	important that in this case we destroy the file before returning. Our
**	caller (the ACP, usually), assumes that if JFcreate returns with an
**	error, then the journal file was not created. We must ensure that this
**	is true (B42269 is an example of what happens if we don't).
**
** Inputs:
**	jf_io				Journal or dump file context.
**	device				Pointer to device/path name.
**	l_device			Length of device/path name.
**	file				Pointer to file name.
**	l_file				Length of file name.
**      bksz                            Block size.  Must be one of:
**					4096, 8192, 16384, 32768 or 65536 for 
**					journal file; 2048 for dump file.
**	blkcnt				Number of blocks to pre-allocate.
**                                      This may be extended.
**				        
** Outputs:
**      sys_err                         Reason for error return status.
**	Returns:
**	    OK
**          JF_BAD_ALLOC                Error allocating space.
**	    JF_BAD_CREATE		Error creating file.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      08-nov-1986 (Derek)
**          Created.
**	6-feb-1992 (bryanp)
**	    Delete journal file before returning JF_BAD_ALLOC (B42269).
*/
STATUS
JFcreate(
JFIO		    *jf_io,
char		    *device,
u_i4		    l_device,
char		    *file,
u_i4		    l_file,
u_i4		    bksz,
i4		    blkcnt,
CL_ERR_DESC	    *err_code)
{
    JFIO		    jfio;	    /* Journal file context. */
    IOSB		    local_iosb;	    /* Operation return status. */
    i4		    s;		    /* Request return status. */
    i4		    status;
    FABDEF		    fab;	    /* RMS File Access Block. */
    NAMDEF		    nam;	    /* RMS NAMe block. */
    char		    filename[128];   /* Construct file name. */

    CL_CLEAR_ERR(err_code);

    /* Initialize JFIO and prepare for RMS open. */

    MEcopy(device, l_device, filename);
    MEcopy(file, l_file, &filename[l_device]);
    MEfill(sizeof(fab), 0, (PTR)&fab);
    MEfill(sizeof(nam), 0, (PTR)&nam);
    fab.fab$b_bid = FAB$C_BID;
    fab.fab$b_bln = FAB$C_BLN;
    fab.fab$w_mrs = bksz;
    fab.fab$b_rfm = FAB$C_FIX;
    fab.fab$l_fna = filename;
    fab.fab$b_fns = l_device + l_file;
    fab.fab$b_fac = FAB$M_BIO | FAB$M_GET | FAB$M_PUT;
    fab.fab$l_fop = FAB$M_UFO | FAB$M_CBT;
    fab.fab$b_shr = FAB$M_UPI | FAB$M_SHRUPD;
    fab.fab$l_nam = &nam;
    nam.nam$b_bid = NAM$C_BID;
    nam.nam$b_bln = NAM$C_BLN;
    s = sys$create(&fab);
    if (s & 1)
    {
	/*  Allocate some disk space. */

	jfio.jf_channel = fab.fab$l_stv;
	jfio.jf_fid[0] = nam.nam$w_fid[0];
	jfio.jf_fid[1] = nam.nam$w_fid[1];
	jfio.jf_fid[2] = nam.nam$w_fid[2];
	status = iiJFallocate(&jfio, blkcnt * (bksz / 512), err_code);

	/*  Deassign channel and return. */

	s = sys$dassgn(jfio.jf_channel);
	if (status == OK && (s & 1))
	    return (OK);

	if (status != OK)
	{
	    s = sys$erase(&fab);
	    if ((s & 1) == 0)
	    {
		err_code->error = s;
		return (JF_BAD_DELETE);
	    }
	    return (JF_BAD_ALLOC);
	}
    }
    err_code->error = s;
    return (JF_BAD_CREATE);
}

/*{
** Name: JFdelete	- Delete a journal or dump file.
**
** Description:
**      This routine deletes a journal or dump file.
**
** Inputs:
**	jf_io				Journal or dump file context.
**      path                            Pointer to device path.
**	l_path				Length of path.
**	filename			Pointer to filename.
**	l_filename			Length of filename.
**
** Outputs:
**      sys_err                         Operating system error.
**	Returns:
**	    OK
**	    JF_BAD_PARAM
**	    JF_
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-mar-1987 (Derek)
**          Created.
**      25-jun-1993 (huffman)
**          make it match the jpt_gl_hdr:jf.h
*/
STATUS
JFdelete(
JFIO		    *jf_io,
char                *path,
u_i4		    l_path,
char		    *filename,
u_i4		    l_filename,
CL_ERR_DESC	    *sys_err)
{
    i4	          s;                    /* Request return status. */
    FABDEF	  fab;               /* File access block. */
    char	  esa[128];
  
    sys_err->error = OK;
    
    /* Check input parameters. */

    if (l_path + l_filename > sizeof(esa)) 
	return (JF_BAD_PARAM);		

    /* Create full lookup name for old file name. */

    MEcopy(path, l_path, esa);
    MEcopy(filename, l_filename, &esa[l_path]);

    MEfill(sizeof(fab), 0, (PTR)&fab);
    fab.fab$b_fns = l_path + l_filename;
    fab.fab$l_fna = esa;
    fab.fab$b_bid = FAB$C_BID;
    fab.fab$b_bln = FAB$C_BLN;

    /* Now delete the file. */    

    s = sys$erase(&fab);
    if ((s & 1) == 0)
    {
        sys_err->error = s;
	if (s == RMS$_FNF)
	    return (JF_NOT_FOUND);
	return (JF_BAD_DELETE);
    }
    return (OK);
}

/*{
** Name: JFdircreate	- Create a journal or dump directory.
**
** Description:
**      The JFdircreate will take a path name and directory name 
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
**        JF_BAD_DIR         Path specification error.
**        JF_BAD_PARAM       Parameter(s) in error.
**        JF_EXISTS          Already exists.
**        JF_DIR_NOT_FOUND    Path not found.
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
JFdircreate(
JFIO           *f,
char           *path,
u_i4          pathlength,
char           *dirname,
u_i4          dirlength,
CL_ERR_DESC     *err_code)
{
    DI_IO	d;
    STATUS	status;

    CL_CLEAR_ERR(err_code);

    status = DIdircreate(&d, path, pathlength, dirname, dirlength, err_code);
    if (status == OK)
	return (status);
    if (status == DI_BADDIR)
	status = JF_BAD_DIR;
    else if (status == DI_BADPARAM)
	status = JF_BAD_PARAM;
    else if (status == DI_EXISTS)
	status = JF_EXISTS;
    else if (status == DI_DIRNOTFOUND)
	status = JF_DIR_NOT_FOUND;
    else
	status = JF_BAD_PARAM;
    return (status);
}

/*{
** Name: JFdirdelete  - Deletes a journal or dump directory.
**
** Description:
**      The JFdirdelete routine is used to delete a direct access 
**      directory.  The name of the directory to delete should
**      be specified as path and dirname, where dirname does not
**      include a type qualifier or version and the path does
**	not include the directory name (which it normally does for
**	other DI calls).  The directory name may be  converted to a filename
**	of the directory and use DIdelete to delete it.
**      This call should fail with JF_BADDELETE if any files still exists
**      in the directory.
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
**	  JF_DIR_NOT_FOUND   Path not found.
**	  JF_NOT_FOUND	     File not found.
**        JF_BAD_PARAM       Parameter(s) in error.
**        JF_BAD_DELETE      Error trying to delete directory.
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
JFdirdelete(
JFIO          *f,
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
	status = JF_BAD_PARAM;
    else if (status == DI_DIRNOTFOUND)
	status = JF_DIR_NOT_FOUND;
    else if (status == DI_FILENOTFOUND)
	status = JF_NOT_FOUND;
    else if (status == DI_BADDELETE)
	status = JF_BAD_DELETE;
    else
	status = JF_BAD_PARAM;
    return (status);
}

/*{
** Name: JFlistfile - List all files in a journal or dump directory.
**
** Description:
**      The JFlistfile routine will list all files that exist
**      in the directory(path) specified.  This routine expects
**      as input a function to call for each file found.  This
**      insures that all resources tied up by this routine will 
**      eventually be freed.  The files are not listed in any
**      specific order. The function passed to this routine
**      must return OK if it wants to continue with more files,
**      or a value not equal to OK if it wants to stop listing.
**	If the function returns anything by OK, then JFlistfile
**	will return JF_BADLIST.
**      The function must have the following call format:
**
**(         STATUS
**          funct_name(arg_list, filename, filelength, err_code)
**          PTR        arg_list;       Pointer to argument list 
**          char       *filename;       Pointer to directory name.
**          i4        filelength;      Length of directory name. 
**          CL_ERR_DESC *err_code;       Pointer to operating system
**                                      error codes (if applicable). 
**)  
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
**        JF_BAD_DIR         Path specification error.
**        JF_END_FILE        Error returned from client handler or
**                           all files listed.
**        JF_BAD_PARAM       Parameter(s) in error.
**        JF_DIR_NOT_FOUND   Path not found.
**        JF_BAD_LIST        Error trying to list objects.
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
JFlistfile(
JFIO	       *f,
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
	status = JF_BAD_PARAM;
    else if (status == DI_DIRNOTFOUND)
	status = JF_DIR_NOT_FOUND;
    else if (status == DI_BADDIR)
	status = JF_BAD_DIR;
    else if (status == DI_BADLIST)
	status = JF_BAD_LIST;
    else if (status == DI_ENDFILE)
	status = JF_END_FILE;
    else
	status = JF_BAD_PARAM;
    return (status);
}

/*{
** Name: JFopen	- Open a journal or dump file.
**
** Description:
**      This routine will open the journal or dump file described by the device
**	and file name.  The block size specified must match the block
**	size used to create the file.  (This value should never change.)
**      If an implementation can detect different blocksizes, it should
**      return an error.
**      The number of the last block written
**	is returned.  Blocks are numbered starting at 1.  A return value
**	of zero denotes an empty file.  Must be able to continue
**      writing to the next block beyond the current end of file, therefore
**      open must position the file for write to end of file block + 1,
**      and for read to block 1.
**      If the file cannot be found, then JF must return JF_NOT_FOUND
**
** Inputs:
**      jfio                            Journal or dump file I/O context.
**	device				Pointer to device/path name.
**	l_device			Length of device/path name.
**	file				Pointer to file name.
**	l_file				Length of file name.
**      bksz                            Block size.  Must be one of:
**					4096, 8192, 16384, 32768 or 65536.
**
**
** Outputs:
**      eof_block                       Last block number updated by
**                                      JFupdate.
**      err_code                        Operating system error.
**	Returns:
**	    OK
**	    JF_BAD_OPEN		        Error opening file.
**          JF_NOT_FOUND                Could not find file.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      24-sep-1986 (Derek)
**          Created.
**      26-apr-1993 (jnash)
**          Add support for block size of 65536 bytes.
**      22-jun-1998 (horda03)
**          Correctly set the eof_block.
*/
STATUS
JFopen(
JFIO                *jfio,
char		    *device,
u_i4		    l_device,
char		    *file,
u_i4		    l_file,
u_i4		    bksz,
i4		    *eof_block,
CL_ERR_DESC	    *err_code)
{
    JFIO		    *j = jfio;	    /* Local copy. */
    IOSB		    local_iosb;	    /* Operation return status. */
    i4		    s;		    /* Request return status. */
    FABDEF		    fab;	    /* RMS File Access Block. */
    NAMDEF		    nam;	    /* RMS NAMe block. */
    XABFHCDEF		    xabfhc;	    /* RMS eXtened attributes. */
    char		    filename[128];   /* Construct file name. */

    CL_CLEAR_ERR(err_code);

    /* Initialize JFIO and prepare for RMS open. */

    MEcopy(device, l_device, filename);
    MEcopy(file, l_file, &filename[l_device]);
    MEfill(sizeof(fab), 0, (PTR)&fab);
    MEfill(sizeof(nam), 0, (PTR)&nam);
    MEfill (sizeof(JFIO), NULL, (PTR)j);
    fab.fab$b_bid = FAB$C_BID;
    fab.fab$b_bln = FAB$C_BLN;
    fab.fab$w_mrs = bksz;
    fab.fab$b_rat = 0;
    fab.fab$b_rfm = FAB$C_FIX;
    fab.fab$l_nam = 0;
    fab.fab$l_fna = filename;
    fab.fab$b_fns = l_device + l_file;
    fab.fab$b_fac = FAB$M_BIO | FAB$M_GET | FAB$M_PUT | FAB$M_TRN;
    fab.fab$l_fop = FAB$M_UFO;
    fab.fab$b_shr = FAB$M_UPI | FAB$M_SHRUPD;
    fab.fab$l_xab = &xabfhc;
    fab.fab$l_nam = &nam;
    nam.nam$b_bid = NAM$C_BID;
    nam.nam$b_bln = NAM$C_BLN;
    xabfhc.xab$b_cod = XAB$C_FHC;
    xabfhc.xab$b_bln = XAB$C_FHCLEN;
    xabfhc.xab$l_nxt = 0;
    s = sys$open(&fab);
    if (s & 1)
    {
	j->jf_ascii_id = JF_ASCII_ID;
	j->jf_channel = fab.fab$l_stv;
	j->jf_fid[0] = nam.nam$w_fid[0];
	j->jf_fid[1] = nam.nam$w_fid[1];
	j->jf_fid[2] = nam.nam$w_fid[2];
	j->jf_blk_size = bksz;
	j->jf_blk_cnt = bksz >> 9;
	j->jf_log2_bksz = 3;
	if (bksz != 4096)
	{
	    j->jf_log2_bksz = 4;
	    if (bksz != 8192)
	    {
		j->jf_log2_bksz = 5;
		if (bksz != 16384)
		{
		    j->jf_log2_bksz = 6;
		    if (bksz != 32768)
			j->jf_log2_bksz = 7;
		}
	    }
	}
	j->jf_allocate = xabfhc.xab$l_hbk;
	if (j->jf_cur_blk = xabfhc.xab$l_ebk)
	    j->jf_cur_blk--;

        /* Calculate last journal block number */
        *eof_block = xabfhc.xab$l_ebk >> j->jf_log2_bksz;
	return (OK);
    }

    err_code->error = s;
    if (s == RMS$_FNF)
	return (JF_NOT_FOUND);
    if (s == RMS$_DNF)
	return (JF_DIR_NOT_FOUND);
    return (JF_BAD_OPEN);
}

/*{
** Name: JFread	- Read block from journal or dump file.
**
** Description:
**      Read next block from the journal or dump file.
**
** Inputs:
**      jfio                            Pointer to journal or dump context.
**	buffer				Pointer to journal or dump block buffer.
**	block				Block number to read.  Ignored
**					if this is a tape device.
**
** Outputs:
**      err_code                        Operating system error on failure.
**	Returns:
**	    OK
**	    JF_BAD_READ		    Error reading file.
**	    JF_BAD_FILE		    Bad file context.
**	    JF_END_FILE		    Read past end of file.
**	Exceptions:
**	    None
**
**  Side Effects:
**	    None
**
** History:
**      24-sep-1986 (Derek)
**          Created.
*/
STATUS
JFread(
JFIO                *jfio,
PTR		    buffer,
i4		    block,
CL_ERR_DESC	    *err_code)
{
    JFIO		*j = jfio;	/* Local copy. */
    int			s;              /* Request return status. */
    IOSB		local_iosb;     /* Operation return status. */ 

    CL_CLEAR_ERR(err_code);

    /* Check file control block pointer, return if bad.*/

    if (j->jf_ascii_id == JF_ASCII_ID) 
    {
	/*  Check for end fo file. */

	if (j->jf_cur_blk < (block << j->jf_log2_bksz))
	    return (JF_END_FILE);

	/* Read a page from disk. */

	s = sys$qiow(EFN$C_ENF, j->jf_channel, 
                 IO$_READVBLK, &local_iosb, 0, 0,
		 buffer, j->jf_blk_size, ((block - 1) << j->jf_log2_bksz) + 1,
		0, 0, 0);

	if ((s & 1) && (local_iosb.iosb$w_status & 1))
	    return (OK);

	err_code->error = s;
	if (s & 1)
	{
	    err_code->error = local_iosb.iosb$w_status;
	    return (JF_BAD_READ);
	}
    }

    err_code->error = OK;    
    return (JF_BAD_FILE);
}

/*{
** Name: JFwrite	- Write block to journal or dump file.
**
** Description:
**      This routine allocates space for the next block(if needed)
**      and writes the block.  The writes can be buffered for delayed
**      writes, i.e. they can be asynchronous.
**
** Inputs:
**      jfio                            Pointer to journal or dump context.
**      buffer                          Pointer to journal or dump block to write.
**      block                           Block number to write.  Ignored if
**					this is a tape device.
**
** Outputs:
**      err_Code                        Operating system error.
**	Returns:
**	    OK
**	    JF_BAD_FILE			Bad file context.
**	    JF_BAD_WRITE		Error writing file.
**	    JF_BAD_ALLOC		Error allocating space.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      24-sep-1986 (Derek)
**          Created.
*/
STATUS
JFwrite(
JFIO                *jfio,
PTR		    buffer,
i4		    block,
CL_ERR_DESC	    *err_code)
{
    JFIO		*j = jfio;	/* Local copy. */
    int			s;              /* Request return status. */
    IOSB		local_iosb;     /* Operation return status. */ 
    i4		last_vbn;

    CL_CLEAR_ERR(err_code);

    /* Check file control block pointer, return if bad.*/

    if (j->jf_ascii_id == JF_ASCII_ID) 
    {
	/*  Check for enough disk space to write the block. */

	last_vbn = block << j->jf_log2_bksz;
	if (last_vbn > j->jf_allocate)
	{
	    s = iiJFallocate(j, 128, err_code);
	    if (s != OK)
		return (s);
	}

	/*  Track the highest block written. */

	if (last_vbn > j->jf_cur_blk)
	    j->jf_cur_blk = last_vbn;

	/*  Perform the write. */

	s = sys$qiow(EFN$C_ENF, j->jf_channel, 
		IO$_WRITEVBLK, &local_iosb, 0, 0,
		buffer, j->jf_blk_size, last_vbn - j->jf_blk_cnt + 1,
		0, 0, 0);

	if ((s & 1) && (local_iosb.iosb$w_status & 1))
	    return (OK);
    
	err_code->error = s;
	if (s & 1)
	    err_code->error = local_iosb.iosb$w_status;
	return (JF_BAD_WRITE);
    }

    err_code->error = OK;    
    return (JF_BAD_FILE);
}

/*{
** Name: JFupdate	- Update journal or dump file position to disk.
**
** Description:
**      Update the position in the journal or dump file. This keeps
**      track of the current block at end of file.  This value
**      is returned to JFopen.  This is a seek position that
**      is valid across system crashes.
**
** Inputs:
**      jfio                             Pointer to journal or dump file context.
** Outputs:
**      err_code                        Operating system error.
**	Returns:
**	    OK
**	    JF_BAD_FILE		    Bad file context.
**	    JF_BAD_UPDATE	    Error updating file.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      24-sep-1986 (Derek)
**          Created.
**	20-jan-2004 (abbjo03)
**	    Correct order of assignment to end-of-file VBN.
*/
STATUS
JFupdate(
JFIO                *jfio,
CL_ERR_DESC	    *err_code)
{
    JFIO		*j = jfio;
    STATUS		status;
    IOSB		iosb;
    struct dsc$descriptor_s fib_desc;
    FIBDEF		fib;
    FAT			rec_attr;
    ATRDEF		attributes[] =
                           {   
                                { ATR$S_RECATTR, ATR$C_RECATTR, (PTR)&rec_attr},
                                { 0, 0, 0}
                           };
    
    CL_CLEAR_ERR(err_code);

    if (j->jf_ascii_id == JF_ASCII_ID)
    {
	union
	{
	    unsigned int	blk;
	    struct { unsigned short int blkl, blkh; } blk_fields;
	} block;

	if ((block.blk = j->jf_cur_blk))
	    ++block.blk;

	MEfill(sizeof(fib), 0, (PTR)&fib);
	MEfill(sizeof(rec_attr), 0, (PTR)&rec_attr);
	fib_desc.dsc$w_length = sizeof(fib);
	fib_desc.dsc$a_pointer = (char *)&fib;
	fib.fib$w_fid[0] = jfio->jf_fid[0];
	fib.fib$w_fid[1] = jfio->jf_fid[1];
	fib.fib$w_fid[2] = jfio->jf_fid[2];
	rec_attr.fat$v_rtype = FAT$C_FIXED;
	rec_attr.fat$w_rsize = j->jf_blk_size;
	rec_attr.fat$w_efblkh = block.blk_fields.blkh;
	rec_attr.fat$w_efblkl= block.blk_fields.blkl;

	status = sys$qiow(EFN$C_ENF, j->jf_channel, IO$_MODIFY,
	    &iosb, 0, 0,
	    &fib_desc, 0, 0, 0, &attributes, 0);

	if ((status & 1) && (iosb.iosb$w_status & 1))
	    return (OK);

	err_code->error = status;
	if (status & 1)
	    err_code->error = iosb.iosb$w_status;
	return (JF_BAD_UPDATE);
    }

    err_code->error = OK;
    return (JF_BAD_FILE);
}

/*{
** Name: JFtruncate -- truncates a journal file at checkpoint time.
**
** Description:
**   
** Inputs:
**      jfio             Pointer to the JFIO file
**                       context needed to do I/O.
**
** Outputs:
**      err_code         Pointer to a variable used
**                       to return operating system 
**                       errors.
**    Returns:
**        OK
**	  JF_BAD_FILE
**        JF_BAD_TRUNCATE
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**	17-oct-1988 (Sandyh)
**	    Created.
*/
STATUS
JFtruncate(
JFIO		*jfio,
CL_ERR_DESC	*err_code)
{
    JFIO	    *j = jfio;		    /* Local copy. */
    i4	            s = 1;                  /* Request return status. */
    IOSB	    iosb;		    /* Operation return status. */
    struct dsc$descriptor_s fib_desc;	    /* Descriptor for FIB. */
    FIBDEF	    fib;		    /* VMS File Information Block. */

    CL_CLEAR_ERR(err_code);

    if (j->jf_ascii_id == JF_ASCII_ID)
    {
	MEfill(sizeof(fib), 0, (PTR)&fib);
	fib_desc.dsc$w_length = sizeof(fib);
	fib_desc.dsc$a_pointer = (char *)&fib;
	fib.fib$w_fid[0] = jfio->jf_fid[0];
	fib.fib$w_fid[1] = jfio->jf_fid[1];
	fib.fib$w_fid[2] = jfio->jf_fid[2];
	fib.fib$w_exctl = FIB$M_TRUNC;
	fib.fib$l_exvbn = j->jf_cur_blk + 1;
	fib.fib$l_exsz = 0;

	s = sys$qiow(EFN$C_ENF, j->jf_channel, IO$_MODIFY, 
	    &iosb, 0, 0, &fib_desc, 0, 0, 0, 0, 0);

	if ((s & 1) && (iosb.iosb$w_status & 1))
	    return(OK);

	err_code->error = s;
	if (s & 1)
	    err_code->error = iosb.iosb$w_status;
	return(JF_BAD_TRUNCATE);
    }
    err_code->error = s;
    return (JF_BAD_FILE);
}

/*
** Name: iiJFallocate - allocates a page to a direct access file.
**
** Description:
**      This routine is used to add pages to a direct
**      access file.  This routines can add more than one page
**      at a time by accepting a count of the number of pages to add.
**      The number of pages is converted to the number of blocks
**      (sectors) to add. If the amount requested cannot 
**      be obtained in large contiguous areas, this routine tries to find
**      as many smaller fragments needed to meet the request.
**   
** Inputs:
**      f                Pointer to the JFIO file
**                       context needed to do I/O.
**	count		 Number of blocks to allocate.
**
** Outputs:
**      err_code         Pointer to a variable used
**                       to return operating system 
**                       errors.
**    Returns:
**        OK
**        JF_ALLOC_ERROR
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**	24-sep-1986 (Derek)
**	    Created.
**	11-dec-1989 (rogerk)
**	    If error in sys$qiow call, set err_code appropriately, using the
**	    correct error value (s or local_iosb).
**      04-jun-2009 (horda03) Bug 111687
**          Use fib->fib$l_exsz to determine how many blocks a file has been
**          extended by.
*/
static STATUS
iiJFallocate(
JFIO		*jfio,
i4		count,
CL_ERR_DESC	*err_code)
{
    JFIO	    *j = jfio;		    /* Local copy. */
    i4	            s;                      /* Request return status. */
    IOSB	    local_iosb;             /* Operation return status. */
    i4              allocated;		    /* Current allocattion . */
    int             blocks;		    /* Number of blocks to alloc. */
    i4              amount;		    /* Last extent size. */
    i4              request;		    /* Requested extent size. */
    struct dsc$descriptor_s fib_desc;	    /* Descriptor for FIB. */
    FIBDEF	    fib;		    /* VMS File Information Block. */

    CL_CLEAR_ERR(err_code);

    /* Convert allocation size from pages to blocks. */

    blocks = count;
    allocated = j->jf_allocate;

    /* 
    ** Set file I/O control block extention parameters 
    ** for the allocation I/O request.
    */

    MEfill(sizeof(fib), '\0', (char *)&fib);
    fib_desc.dsc$w_length = sizeof(FIBDEF);
    fib_desc.dsc$a_pointer = (char *)&fib;
    fib.fib$w_exctl = FIB$M_EXTEND | FIB$M_ALCONB | FIB$M_ALDEF | FIB$M_ALCON;
    fib.fib$w_fid[0] = jfio->jf_fid[0];
    fib.fib$w_fid[1] = jfio->jf_fid[1];
    fib.fib$w_fid[2] = jfio->jf_fid[2];

    /*
    ** Repeat request for extensions until the number of blocks
    ** requested by the caller is allocated or until an error
    ** is returned indicating no more space.  Each request is
    ** for the typical (or maximum - see above) size.
    */

    request = blocks;
    while (blocks > 0)
    {
        fib.fib$l_exsz = request;
	fib.fib$l_exvbn = 0;

        s = sys$qiow(EFN$C_ENF, j->jf_channel, IO$_MODIFY, 
	    &local_iosb, 0, 0,
                     &fib_desc, 0, 0, 0, 0, 0);

	if ((s & 1) && (local_iosb.iosb$w_status & 1))
	{
	    amount = fib.fib$l_exsz;
	    allocated += amount;
	    blocks -= amount;
	    continue;
	}

	err_code->error = s;
	if (s & 1)
	    err_code->error = local_iosb.iosb$w_status;
	return (JF_BAD_ALLOC);
    }

    j->jf_allocate = allocated;
    return (OK);
}
