/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <di.h>
#include    <jf.h>
#include    <tr.h>
#include    "jflocal.h"
#include    <errno.h>

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
**
**  History:
**      24-sep-1986 (Derek)
**	    Created.
**      26-mar-1987 (Derek)
**          Added new routines to manipulate directories.
**	07-jul-1987 (mmm)
**	    Ported to UNIX.
**	21-jan-1989 (mmm)
**	    always specify DI_SYNC_MASK on journal files.
**	    JFcreate() now call DIclose().
**	04-may-89 (arana)
**	    Remove unused formal parameter "sequence" from JFupdate.
**      21-dec-1989 (mikem)
**	    Always delete the file if JFcreate fails for some reason.
**	    Also make JFdirdelete call DIdirdelete() rather than DIdircreate().
**	25-jan-1990 (mikem)
**	    Fixed a CL_ERR_DESC paramter to DIdelete() to be passed by reference
**	    rather than by value.
**	18-jun-1990 (mikem)
**	    bug #31815 - reading past journal EOF
**	    It is possible for the disk copy of the journal file to have more
**	    blocks on disk than the internally maintained JF eof stored in the
**	    hdr of the file.  JFread() is supposed to return JF_END_FILE if
**	    there is an attempt to read past this EOF marker, even if the disk
**	    copy has more data past this EOF.  Before this bug fix JFread()
**	    ignored the JF EOF marker and just read to end of disk file as 
**	    given by DI.  This fix changes JFread() read up to the current
**	    JFeof (reading the hdr block if necessary), and then return 
**	    JF_END_FILE.
**	30-jul-1991 (bryanp)
**	    Pass DIalloc the number of pages to allocate, not the number of 
**	    bytes.
**	30-dec-1992 (mikem)
**	    Changed for loop to do/while in JFread() to eliminate 
**	    "unreached end-of-loop" compiler warning.
**	26-apr-1993 (bryanp)
**	    Prototyping the code for 6.5.
**	26-apr-1993 (jnash)
**	    ALTERDB -init_jnl_blocks change: implement the 'blkcnt' parameter 
**	        in JFcreate() via a call to DIgalloc() and by tracking the
**		currently allocated amount of file space in newly created
**		jff_alloc_eof JF_FILE_HDR field.
**	    Add '65536' as a legal block size.
**	    Add ANSI function headers and prototypes.
**	26-apr-1993 (bryanp)
**	    arg_list is a PTR, not a (PTR *).
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	23-aug-1993 (andys)
**	    In JFdelete, if we can't find the journal file to be deleted then
**	    return JF_NOT_FOUND as the VMS CL does. This allows ckpdb to 
**	    continue operation after a journal file has been deleted 
**	    erroneously. [bug 49251] [was 18-feb-1993 (andys)]
**	18-apr-1994 (jnash)
**	    fsync project: In JFopen, open journal files DI_USER_SYNC_MASK. 
**	    If JFupdate, force the file.
**	19-apr-95 (canor01)
**	    added <errno.h>
**      27-may-97 (mcgem01)
**          Cleaned up compiler warnings.
**      08-jul-99 (i4jo01)
**        Fixed up typo in JFwrite where preallocated space was being
**        used as a basis for computing how many new pages should be
**        allocated. (b97845)
**	11-Nov-1999 (jenjo02)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	    The latter causes a wasted ___errno function call.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

static STATUS JF_sense(
		JFIO                *jfio,
		CL_ERR_DESC	    *err_code);

/*{
** Name: JFclose	- Close journal file.
**
** Description:
**      This routine closes the journal file described by the JFIO context
**	passed to this routine.
**
** Inputs:
**      jfio                            Pointer to journal file context.
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
    STATUS	    ret_val;

    /* default returns */

    ret_val = OK;

    /*	Check for valid control block. */

    if (j->jf_ascii_id != JF_ASCII_ID) 
    {
	CL_CLEAR_ERR( err_code );
	ret_val = JF_BAD_FILE;
    }
    else
    {
	/*	Close the file. */

	if (ret_val = DIclose(&(j->jf_di_io), err_code))
	{
	    ret_val = JF_BAD_CLOSE;
	}
    }

    return(ret_val);
}

/*{
** Name: JFcreate	- Create a journal file.
**
** Description:
**      This routine creates and allocates a journal file for processing.
**      This file is created in the area specified by device/path name.
**
** Inputs:
**	jf_io				Journal file context.
**	device				Pointer to device/path name.
**	l_device			Length of device/path name.
**	file				Pointer to file name.
**	l_file				Length of file name.
**      bksz                            Block size.  Must be one of:
**					4096, 8192, 16384, 32768.
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
**	21-jan-1989 (mmm)
**	    always specify DI_SYNC_MASK on journal files.
**	    JFcreate() now call DIclose().
**      21-dec-1989 (mmm)
**	    Always delete the file if JFcreate fails for some reason.
**	25-jan-1990 (mikem)
**	    Fixed a CL_ERR_DESC paramter to DIdelete() to be passed by reference
**	    rather than by value.
**	26-apr-1993 (jnash)
**	    ALTERDB -init_jnl_blocks change, initialize 'blkcnt' number 
**	    of blocks in the journal file via DIgalloc.  Also make 65536 a 
**	    legal block size.
**      28-May-2003 (hanal04) Bug 109090 INGSRV 1994
**          DI_USER_SYNC_MASK coupled with DIforce() operations allows
**          journal file corruptions during a hard power failure on
**          Solaris. Use DI_SYNC_MASK during journal file creation to 
**          ensure the OS causes ALL updates to hit disk.
**	25-Jul-2007 (kibro01) b118595
**	    Given that if we fail to open a journal with DIcreate we 
**	    immediately do DIdelete, we might as well retry.
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
    JFIO	    *j = jf_io;	/* local copy */
    STATUS	    ret_val;
    i4	    di_num_pages;
    i4	    di_blockno;
    i4	    tot_pages;
    i4	    eof;
    JF_FILE_HDR	    jf_hdr;
    CL_ERR_DESC     local_errcode;


    /* default returns */

    ret_val = OK;
    CL_CLEAR_ERR( err_code );

    if ((bksz != 4096)  && (bksz != 8192) && (bksz != 16384) && 
	(bksz != 32768) && (bksz != 65536))
    {
	ret_val = JF_BAD_CREATE;
    }
    else
    {
	j->jf_blk_size = bksz;
    }

    if (!ret_val)
    {
	if (ret_val = DIcreate(&(j->jf_di_io), device, l_device, 
				    file, l_file, JF_MIN_BLKSIZE, err_code))
	{
	    if (ret_val == DI_EXISTS)
	    {
		/* Try deleting now and recreating */
		DIdelete(&(j->jf_di_io), device, l_device, file, l_file, 
		    &local_errcode);
		ret_val = DIcreate(&(j->jf_di_io), device, l_device, 
				    file, l_file, JF_MIN_BLKSIZE, err_code);
		if (ret_val)
		{
		    ret_val = JF_BAD_CREATE;
		}
	    } else
	    {
		ret_val = JF_BAD_CREATE;
	    }
	}
    }

    /* now write initial file header */

    if (!ret_val)
    {
	/* open the JF file so we can write the header. */

	if (ret_val = DIopen(&(j->jf_di_io), device, l_device, file, 
			 l_file, JF_MIN_BLKSIZE, (i4) DI_IO_WRITE, 
			 (u_i4) (DI_SYNC_MASK), err_code))
	{
	    ret_val = JF_BAD_CREATE;
	}
    }

    /* 
    ** Allocate the preallocated amount plus one additional page for the
    ** file header.
    */
    if (!ret_val)
    {
	eof = blkcnt * bksz + 1;
	tot_pages = (eof / JF_MIN_BLKSIZE) + 1;

	ret_val = DIgalloc(&(j->jf_di_io), tot_pages, &di_blockno, err_code);
	if (ret_val || di_blockno != 0)
	{
	    ret_val = JF_BAD_CREATE;
	}
    }

    if (!ret_val)
    {

	/* JF pages are mapped to JF_MIN_BLKSIZE DI pages by the formula
	** below.  JF pages are numbered from 1 to n.  DI pages are numbered
	** 0 to n.  The 0 DI page in a JF file is the header and is used
	** internally by JF to maintain an internal end-of-file persistent
	** across system crashes.
	*/
	jf_hdr.jff_eof_blk  = 0;
	jf_hdr.jff_blk_size = bksz;
	jf_hdr.jff_ascii_id = JF_ASCII_ID;

	di_num_pages = 1;
	di_blockno   = 0;

	if (ret_val = DIwrite(&(j->jf_di_io), &di_num_pages, di_blockno, 
						    (char *) &jf_hdr, err_code))
	{
	    ret_val = JF_BAD_CREATE;
	}
    }

    /*	Close the file. */

    if (!ret_val)
    {
	if (ret_val = DIclose(&(j->jf_di_io), err_code))
	{
	    ret_val = JF_BAD_CREATE;
	}
    }

    if (ret_val)
    {
	DIdelete(&(j->jf_di_io), device, l_device, file, l_file, 
	    &local_errcode);
    }

    return(ret_val);
}

/*{
** Name: JFdelete	- Delete a journal file.
**
** Description:
**      This routine deletes a journal file.
**
** Inputs:
**	jf_io				Journal file context.
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
**	23-aug-1993 (andys)
**	    In JFdelete, if we can't find the journal file to be deleted then
**	    return JF_NOT_FOUND as the VMS CL does. This allows ckpdb to 
**	    continue operation after a journal file has been deleted 
**	    erroneously. [bug 49251] [was 18-feb-1993 (andys)]
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
    JFIO	  *j =  jf_io;		/* local copy */
    char	  esa[128];
    STATUS	  ret_val;

    /* default returns */

    ret_val = OK;
    
    /* Check input parameters. */

    if (l_path + l_filename > sizeof(esa)) 
    {
	CL_CLEAR_ERR( sys_err );
	ret_val = JF_BAD_PARAM;		
    }
    else
    {
	if (ret_val = DIdelete(&(j->jf_di_io), path, l_path, filename, 
							l_filename, sys_err))
	{
	    if (ret_val == DI_FILENOTFOUND)
		ret_val = OK;
	    else
	    	ret_val = JF_BAD_DELETE;
	}
    }

    return(ret_val);
}

/*{
** Name: JFdircreate	- Create a journal directory.
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
** Name: JFdirdelete  - Deletes a journal directory.
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
**      21-dec-1989 (mikem)
**	    Make JFdirdelete call DIdirdelete() rather than DIdircreate().
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
** Name: JFlistfile - List all files in a journal directory.
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
**          i4         filelength;      Length of directory name. 
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
JFIO		*f,
char           *path,
u_i4          pathlength,
STATUS         (*func)(),
PTR            arg_list,
CL_ERR_DESC     *err_code)
{
    DI_IO	d;
    STATUS	status;

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
** Name: JFopen	- Open a journal file.
**
** Description:
**      This routine will open the journal file described by the device
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
**      jfio                            Journal file I/O context.
**	device				Pointer to device/path name.
**	l_device			Length of device/path name.
**	file				Pointer to file name.
**	l_file				Length of file name.
**      bksz                            Block size.  Must be one of:
**					4096, 8192, 16384, 32768.
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
**      24-spe-1986 (Derek)
**          Created.
**	21-jan-1989 (mmm)
**	    always specify DI_SYNC_MASK on journal files.
**	26-apr-1993 (jnash)
**	    Call DIsense to find the previously allocated amount.  
**	    Also make 65536 bytes a legal block size.
**	18-apr-1994 (jnash)
**	    fsync project: Open journal files DI_USER_SYNC_MASK. 
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
    JFIO	*j =  jfio;		/* local copy */
    STATUS	ret_val = OK;
    i4	di_num_pages;
    i4	di_blockno;
    JF_FILE_HDR	jf_hdr;

    CL_CLEAR_ERR( err_code );

    if ((bksz != 4096)  && (bksz != 8192) && (bksz != 16384) && 
	(bksz != 32768) && (bksz != 65536))
    {
	ret_val = JF_BAD_PARAM;
	return(ret_val);
    }

    j->jf_blk_size = bksz;

    /* always set the blksize to JF_MIN_BLKSIZE */
    if (ret_val = DIopen(&(jfio->jf_di_io), device, l_device, file, 
			 l_file, JF_MIN_BLKSIZE, (i4) DI_IO_WRITE,
			 (u_i4) (DI_SYNC_MASK), err_code))
    {
	if ((ret_val == DI_BADDIR)	|| 
	    (ret_val == DI_DIRNOTFOUND)	||
	    (ret_val == DI_FILENOTFOUND))
	{
	    ret_val = JF_NOT_FOUND;
	}
	else
	{
	    ret_val = JF_BAD_OPEN;
	}
    }

    /* now read the file header to find true JF end of file */

    if (!ret_val)
    {
	/* JF pages are mapped to JF_MIN_BLKSIZE DI pages by the formula
	** below.  JF pages are numbered from 1 to n.  DI pages are numbered
	** 0 to n.  The 0 DI page in a JF file is the header and is used
	** internally by JF to maintain an internal end-of-file persistent
	** across system crashes.
	*/
	di_num_pages = 1;
	di_blockno   = 0;

	if (ret_val = DIread(&(j->jf_di_io), &di_num_pages, di_blockno, 
						    (char *) &jf_hdr, err_code))
	{
	    ret_val = JF_BAD_OPEN;
	}
	else if ((jf_hdr.jff_ascii_id != JF_ASCII_ID) || 
		 (jf_hdr.jff_blk_size != bksz))
	{
	    ret_val = JF_BAD_OPEN;
	}
	else
	{
	    /* successful open */

	    j->jf_blk_cnt = jf_hdr.jff_eof_blk;
	    j->jf_ascii_id = JF_ASCII_ID;
	    j->jf_blk_size = bksz;

	    *eof_block = jf_hdr.jff_eof_blk;

	    /*
	    ** Find the previously allocated amount.
	    */
	    if (DIsense(&(j->jf_di_io), &j->jf_allocate, err_code))
	    {
		ret_val = JF_BAD_OPEN;
	    }
	    else
	    {
		/*
		** JF pages are numbered from 1 to n.  DI pages are numbered
		** from zero to n-1.  
		*/
		j->jf_allocate++;
	    }
	}

    }

    return(ret_val);
}

/*{
** Name: JFread	- Read block from journal file.
**
** Description:
**      Read next block from the journal file.
**
** Inputs:
**      jfio                            Pointer to journal context.
**	buffer				Pointer to journal block buffer.
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
**	18-jun-1990 (mikem)
**	    bug #31815 - reading past journal EOF
**	    It is possible for the disk copy of the journal file to have more
**	    blocks on disk than the internally maintained JF eof stored in the
**	    hdr of the file.  JFread() is supposed to return JF_END_FILE if
**	    there is an attempt to read past this EOF marker, even if the disk
**	    copy has more data past this EOF.  Before this bug fix JFread()
**	    ignored the JF EOF marker and just read to end of disk file as 
**	    given by DI.  This fix changes JFread() read up to the current
**	    JFeof (reading the hdr block if necessary), and then return 
**	    JF_END_FILE.
**	30-dec-1992 (mikem)
**	    Changed for loop to do/while to eliminate "unreached end-of-loop"
**	    compiler warning.
*/
STATUS
JFread(
JFIO                *jfio,
PTR		    buffer,
i4		    block,
CL_ERR_DESC	    *err_code)
{
    JFIO	*j = jfio;		/* local copy */
    STATUS	ret_val = OK;
    i4		di_num_pages;
    i4		di_blockno;

    CL_CLEAR_ERR( err_code );

    do
    {
	/* break on errors or end of control */

	if (j->jf_ascii_id != JF_ASCII_ID) 
	{
	    ret_val = JF_BAD_FILE;
	    break;
	}

	if (block > j->jf_blk_cnt)
	{
	    if ((ret_val = JF_sense(jfio, err_code)))
	    {
		break;
	    }
		
	    if (block > j->jf_blk_cnt)
	    {
		/* attempt to read past "jf" end of file */

		ret_val = JF_END_FILE;
		break;
	    }
	}
		
	/* JF pages are mapped to JF_MIN_BLKSIZE DI pages by the formula
	** below.  JF pages are numbered from 1 to n.  DI pages are numbered
	** 0 to n.  The 0 DI page in a JF file is the header and is used
	** internally by JF to maintain an internal end-of-file persistent
	** across system crashes.
	*/
	di_num_pages = (j->jf_blk_size / JF_MIN_BLKSIZE);
	di_blockno  = (((block - 1) * j->jf_blk_size) / JF_MIN_BLKSIZE) + 1;

	if (ret_val = DIread(&(j->jf_di_io), &di_num_pages, di_blockno, 
							buffer, err_code))
	{
	    if ((ret_val == DI_BADFILE) || (ret_val == DI_BADPARAM))
		ret_val = JF_BAD_FILE;
	    else if (ret_val == DI_ENDFILE)
		ret_val = JF_END_FILE;
	    else 
		ret_val = JF_BAD_READ;
	}

    } while (FALSE);

    return(ret_val);
}

/*
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
    return(OK);
}

/*{
** Name: JFwrite	- Write block to journal file.
**
** Description:
**      This routine allocates space for the next block(if needed)
**      and writes the block.  The writes can be buffered for delayed
**      writes, i.e. they can be asynchronous.
**
** Inputs:
**      jfio                            Pointer to journal context.
**      buffer                          Pointer to journal block to write.
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
**	30-jul-1991 (bryanp)
**	    Pass DIalloc the number of pages to allocate, not the number of 
**	    bytes.  Evidently we had been passing the wrong number for a 
**	    long time; the only bad effect was that DIalloc was computing a 
**	    totally bogus value for the 'io_alloc_eof' value in the DI_IO 
**	    structure. This never bit us until we added code to DIclose to 
**	    attempt to do something with this value as part of the file 
**	    extend project, at which point DIclose decided that the file 
**	    was 512 times too small and tried to allocate a mountain of 
**	    extra space.
**	26-apr-1993 (jnash)
**	    Add support for journal file preallocation reflected by the
**	    value of j->jf_allocate.
**      08-jul-1999 (i4jo01)
**        Fixed up typo where preallocated space was being used as
**        a basis for computing how many new pages should be
**        allocated. This would cause a crash after about a 190MB
**        journal file. (b97845)
*/
STATUS
JFwrite(
JFIO                *jfio,
PTR		    buffer,
i4		    block,
CL_ERR_DESC	    *err_code)
{
    JFIO	*j =  jfio;		/* local copy */
    STATUS	ret_val;
    i4		di_num_pages;
    i4		di_blockno;
    i4	dummy;
    i4	current_eof;
    i4	alloc_eof;
    i4	new_eof;

    /* default return */

    ret_val = OK;
    CL_CLEAR_ERR( err_code );

    if (j->jf_ascii_id != JF_ASCII_ID) 
    {
	ret_val = JF_BAD_FILE;
    }
    else if (block <= 0)
    {
	ret_val = JF_BAD_WRITE;
    }

    if (!ret_val)
    {
	new_eof = ((block * j->jf_blk_size) + 1);

	/* the following may need to be optimized - ie. in-line code for
	** what DIalloc and DIwrite do.
	*/
	if ((block - j->jf_blk_cnt) <= 1)
	{
	    /* normal case - don't call DIsense */
	    current_eof = ((j->jf_blk_cnt * j->jf_blk_size) + 1);
	}
	else 
	{
	    /*
	    ** Don't think the current users of JF can get here.
	    */ 
#ifdef xDEBUG
	    TRdisplay("JFwrite: block to write not within 1 of jf_blk_cnt!\n");
#endif
	    ret_val = JF_BAD_WRITE;
	}

	if (!ret_val)
	{
	    /*
	    ** Calculate the amount of space to allocate based on the 
	    ** current request and how much we preallocated when the
	    ** file was created.
	    */
	    alloc_eof = j->jf_allocate * JF_MIN_BLKSIZE;
	    if (new_eof > alloc_eof)
	    {
		di_num_pages = ((new_eof - current_eof) / JF_MIN_BLKSIZE) + 1;
		if (DIalloc(&(j->jf_di_io), di_num_pages, &dummy, err_code))
		{
		    ret_val = JF_BAD_WRITE;
		}
	    }
	}
    }

    if (!ret_val)
    {
	/* write one page of journal */

	/* JF pages are mapped to JF_MIN_BLKSIZE DI pages by the formula
	** below.  JF pages are numbered from 1 to n.  DI pages are numbered
	** 0 to n.  The 0 DI page in a JF file is the header and is used
	** internally by JF to maintain an internal end-of-file persistent
	** across system crashes.
	*/
	di_num_pages = (j->jf_blk_size / JF_MIN_BLKSIZE);
	di_blockno   = (((block - 1) * j->jf_blk_size) / JF_MIN_BLKSIZE) + 1;

	if (ret_val = DIwrite(&(j->jf_di_io), &di_num_pages, 
						di_blockno, buffer, err_code))
	{
	    if (ret_val == DI_BADFILE)
	    {
		ret_val = JF_BAD_FILE;
	    }
	    else if (ret_val == DI_BADPARAM)
	    {
		ret_val = JF_BAD_FILE;
	    }
	    else if (ret_val == DI_ENDFILE)
	    {
		ret_val = JF_BAD_FILE;
	    }
	    else
		ret_val = JF_BAD_WRITE;
	}
	else if (j->jf_blk_cnt < block)
	{
	    j->jf_blk_cnt = block;
	}
    }

    return(ret_val);
}

/*{
** Name: JFupdate	- Update journal file position to disk.
**
** Description:
**      Update the position in the journal file. This keeps
**      track of the current block at end of file.  This value
**      is returned to JFopen.  This is a seek position that
**      is valid across system crashes.
**
** Inputs:
**      jfio                             Pointer to journal file context.
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
**	04-may-89 (arana)
**	    Remove unused formal parameter "sequence".
**	18-apr-1994 (jnash)
**	    fsync project.  Journal files are now opened DI_USER_SYNC_MASK, 
**	    so we DIforce them here.
**      28-May-2003 (hanal04) Bug 109090 INGSRV 1994
**          JFcreate has been modified to use DI_SYNC_MASK. DI_USER_SYNC_MASK
**          is no longer being used so we do not need to DIforce().
*/
STATUS
JFupdate(
JFIO                *jfio,
CL_ERR_DESC	    *err_code)
{
    JFIO	*j =  jfio;		/* local copy */
    STATUS	ret_val = OK;
    i4	di_num_pages;
    i4	di_blockno;
    JF_FILE_HDR	jf_hdr;


    /* now read the file header to find true JF end of file */

    if (!ret_val)
    {
	/* JF pages are mapped to JF_MIN_BLKSIZE DI pages by the formula
	** below.  JF pages are numbered from 1 to n.  DI pages are numbered
	** 0 to n.  The 0 DI page in a JF file is the header and is used
	** internally by JF to maintain an internal end-of-file persistent
	** across system crashes.
	*/
	jf_hdr.jff_eof_blk = j->jf_blk_cnt;
	jf_hdr.jff_blk_size = j->jf_blk_size;
	jf_hdr.jff_ascii_id = JF_ASCII_ID;
	di_num_pages = 1;
	di_blockno   = 0;

	if (ret_val = DIwrite(&(j->jf_di_io), &di_num_pages, di_blockno, 
						    (char *) &jf_hdr, err_code))
	{
	    TRdisplay("JFupdate: DIwrite() failure, status: %d, error: %d.\n", 
		ret_val, err_code);
	    ret_val = JF_BAD_UPDATE;
	}
    }

    return(ret_val);
}

/*{
** Name: JF_sense()	- Read disk header to determine "real jf eof"
**
** Description:
**	Internal routine which reads the JF header block to determine the
**	"real JF eof".  The JF EOF is only updated by JFupdate.  The
**	actual file on disk may be larger than this EOF may indicate, but
**	the extra should be ignored.  One case that can make this happen
**	is if the ACP fails while archiving a CP.  On restart it will rearchive
**	the journal records, and it expects to restart at the last committed
**	JFupdate place.
**
** Inputs:
**	jfio				An already open JF file pointer.
**
** Outputs:
**	err_code			proprogate up err_code from DI calls.
**
**	Returns:
**	    OK - success
**	    non-OK - failure.
**	
** History:
**      01-mar-90 (mikem)
**          Created.
*/
static STATUS
JF_sense(
JFIO                *jfio,
CL_ERR_DESC	    *err_code)
{
    JFIO	*j =  jfio;		/* local copy */
    STATUS	ret_val = OK;
    i4	di_num_pages;
    i4	di_blockno;
    JF_FILE_HDR	jf_hdr;

    /* now read the file header to find true JF end of file */

    /* JF pages are mapped to JF_MIN_BLKSIZE DI pages by the formula
    ** below.  JF pages are numbered from 1 to n.  DI pages are numbered
    ** 0 to n.  The 0 DI page in a JF file is the header and is used
    ** internally by JF to maintain an internal end-of-file persistent
    ** across system crashes.
    */
    di_num_pages = 1;
    di_blockno   = 0;

    if (ret_val = DIread(&(j->jf_di_io), &di_num_pages, di_blockno, 
						(char *) &jf_hdr, err_code))
    {
	ret_val = JF_BAD_OPEN;
    }
    else if (jf_hdr.jff_ascii_id != JF_ASCII_ID)
    {
	ret_val = JF_BAD_OPEN;
    }
    else
    {
	/* update eof open */

	j->jf_blk_cnt = jf_hdr.jff_eof_blk;
    }

    return(ret_val);
}
