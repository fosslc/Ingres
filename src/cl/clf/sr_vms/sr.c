/*
**    Copyright (c) 1985, 2008 Ingres Corporation
*/

# include   <compat.h>
# include   <bt.h>
# include   <gl.h>
# include   <sr.h>
# include   <me.h>
# include   <cs.h>
# include   <efndef.h>
# include   <iledef.h>
# include   <iodef.h>
# include   <iosbdef.h>
# include   <fibdef.h>
# include   <ssdef.h>
# include   <eventflag.h>
#ifdef xDEBUG
# include   <dvidef.h>
# include   <dcdef.h>
# include   <vmstypes.h>
#endif
# include   <starlet.h>
# include   <astjacket.h>

/**
**
**  Name: SR.C - This file contains all the SR routines.
**
**  Description:
**      The SR.C file contains all the routines needed to perform
**      direct I/O to files.  These are used only for sort files.
**      The I/O is done in blocks(sectors).   
**
**        SRclose - Closes aan may delete a file.
**        SRopen - Opens and may create and allocate space for a file.
**        SRreadN - Reads N pages from a file.
**        SRwriteN - Writes N pages to a file and extends it if necessary.
**
**  History:
**      30-sep-85 (jennifer)    
**          Created new for 5.0.
**      02-apr-86 (jennifer)
**          Modified the system error returned from a i4 to
**          the type CL_ERR_DESC per CL request.
**	09-mar-1987 (Derek)
**	    Changed allocate to minimize number of I/O's to extend file.
**	27-jul-1987 (rogerk)
**	    Check for disk full or quota exceeded and return SR_EXCEED_LIMIT.
**	26-mar-91 (jrb)
**	    Got rid of compiler warnings.  (Added FIB pointer.)
**	26-apr-1993 (bryanp)
**	    Prototyping code for 6.5.
**      16-jul-93 (ed)
**	    added gl.h
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	02-sep-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**	1-Mar-2007 (kschendel) SIR 122512
**	    Allow arbitrary page sizes as long as they are multiples of
**	    512.  See SRopen for the rules on changing page sizes.
**	    (Note that this moots horda04's 4-sep-2008 change for bug 120775.)
**	7-Mar-2007 (kschendel) SIR 122512
**	    Add SRreadN, SRwriteN.
**	04-Apr-2008 (jonj)
**	    Get event flag using lib$get_ef() instead of non-thread-safe
**	    CACHE_IO_EF.
**	    Define SRresumeFromAST() to ease debugging of CSresume.
**      04-sep-2008 (horda03) Bug 120775
**          SR pages are not limited to 2,4,8,16,32 or 64K bytes. The
**          HASH JOIN files can be 32, 64, 96 or 128K bytes in size, so
**          the "io_log2_blk" method used here (and in DI) is not applicable.
**          Will now use this field if the pagesize is a power of 2, but now
**          will store (as a negative value) the number of (512) blocks when
**          not.
**	09-oct-2008 (stegr01/joea)
**	    Replace II_VMS_ITEM_LIST_3 by ILE3.
**	11-nov-2008 (joea)
**	    Use CL_CLEAR_ERR to initialize CL_ERR_DESC.
**      20-mar-2009 (stegr01)
**          Add support for Posix threads on Itanium (astjacket)
**      03-Jun-2009 (horda03) Bug 111687
**          On VMS 7.3-2 E_DM9713 error may occur when sorting. This has been
**          traced back to VMS not returning the number of blocks allocated in
**          the IOSB's second word (the documentation says it can return the
**          number of contiguous blocks available). The fib's fib$l_exsz will
**          always have the number of blocks allocated.
**/

/*
**  Forward and/or External typedef/struct references.
*/

typedef struct _SR_STR_DESC SR_STR_DESC;

/*
**  Local Definitions.
*/

#define                 SR_MIN_INCR     128
                                        /* maximum number of blocks
                                        ** to expand file. */
#define                 SR_PATH_MAX     128
                                        /* maximum path length in
                                        ** bytes. */
#define                 SR_FILENAME_MAX 32
                                        /* maximum file name part
                                        ** of a full file name. */

/* OS and Internal threads require different handling for asynchronous calls */
#ifdef OS_THREADS_USED
#  ifdef ALPHA
#     define SR_CHECK_RESUME
      i4 SR_check_resume = 1;

      /* Only need to suspend if running internal threads or there's
      ** a possibility of running internal threads
      */

#     define SR_SUSPEND_THREAD if (sr_resume_fcn) CSsuspend(CS_DIO_MASK, 0, 0)
#  endif
#
#  define SR_RESUME_FCN NULL
#  define SR_GETDVI     sys$getdviw
#  define SR_QIO        sys$qiow

#else /* OS_THREADS_USED */

static void SRresumeFromAST( CS_SID sid );

#  define SR_RESUME_FCN     SRresumeFromAST
#  define SR_GETDVI     sys$getdvi
#  define SR_QIO        sys$qio
#  define SR_SUSPEND_THREAD CSsuspend(CS_DIO_MASK, 0, 0)

#endif

void (*sr_resume_fcn)() = SR_RESUME_FCN;
i4   (*sr_getdvi)()     = SR_GETDVI;
i4   (*sr_qio)()        = SR_QIO;

static void     SR_check_init();

/*}
** Name: SR_STR_DESC - Area to contain directory or file names.
**
** Description:
**      This structure is used to contain an operating system 
**      descriptor for file or path names. 
**
** History:
**     30-sep-85 (jennifer)
**          Created new for 5.0.
**      02-apr-86 (jennifer)
**          Modified the system error returned from a i4 to
**          the type CL_ERR_DESC per CL request.
*/
typedef struct _SR_STR_DESC
{
    i4              str_len;            /* lenght of string */
    char            *str_str;           /* the string */
};

/*
**  Definition of static variables and forward static functions.
*/

static	STATUS	iiSRallocate(
		    SR_IO		*f,
		    i4		n,
		    i4			sid,
		    CL_ERR_DESC		*err_code);

/*{
** Name: SRclose - Closes a file.
**
** Description:
**      The SRclose routine is used to close a direct access file.  
**      It will check to insure the file is open before trying to close.
**      It will also delete it if the delete flag is set to SR_IO_DELETE.
**   
** Inputs:
**      f                    Pointer to the SR file
**                           context needed to do I/O.
**      delete_flag          Variable used to indicate if 
**                           file is to be deleted.  This must 
**                           always be set to SR_IO_DELETE.
** Outputs:
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**    Returns:
**          OK
**          SR_BADCLOSE		Error closing file or bad parameters.
**          SR_BADFILE		Bad file context.
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    11-sep-85 (jennifer)
**          Created new for 5.0.
**      02-apr-86 (jennifer)
**          Modified the system error returned from a i4 to
**          the type CL_ERR_DESC per CL request.
**     8-dec-88 (rogerk)
**	    Check that we still own the file we are about to deaccess.  Look
**	    for problem where we close somebody else's file channel.
*/
static void
SRresumeFromAST( CS_SID sid )
{
    CSresume( sid );
}

STATUS
SRclose(
SR_IO		*f,
i4		delete_flag,
CL_ERR_DESC	*err_code)
{
    IOSB		local_iosb;         /*  Operation return status. */
    int			s;                  /*  Request return status. */
    CS_SID		sid;
#ifdef xDEBUG
    long		device_class;
    long		dvi_length;
    ILE3		itmlst = {4, DVI$_DEVCLASS, &device_class, &dvi_length};
#endif

    CL_CLEAR_ERR(err_code);
    /*	Check for valid control block. */

    if (f->io_type != SR_IO_ASCII_ID) 
        return (SR_BADFILE);

    /*	Check that the file is open. */

    if (f->io_channel == 0)
        return (OK);
    
    CSget_sid(&sid);
    
#ifdef xDEBUG
    /*
    ** Check that the channel we are deallocating is still connected
    ** to a disk device.  This will trap the errors where we are
    ** conflicting with GCF on IO channels.
    */
    s = sr_getdvi(EFN$C_ENF, f->io_channel, 0, &itmlst, &local_iosb,
 	          sr_resume_fcn, sid, 0);
    if (s & 1)
    {
#ifdef SR_SUSPEND_THREAD
        SR_SUSPEND_THREAD;
#endif
	s = local_iosb.iosb$w_status;
    }
    if (((s & 1) == 0) || (device_class != DC$_DISK))
    {
	/*
	** This io channel is no longer any good, or is not what we
	** think it is.  Return BADCLOSE and either the system error
	** (if one was returned), or SS$_NOTFILEDEV.
	*/
	err_code->error = s;
	if (s & 1)
	    err_code->error = SS$_NOTFILEDEV;
	return (SR_BADCLOSE);
    }
#endif

    /*	Queue request to close and delete the file. */
    s = sr_qio(EFN$C_ENF, f->io_channel, 
		IO$_DEACCESS, &local_iosb, sr_resume_fcn, sid,
		0, 0, 0, 0, 0, 0);

    if (s & 1)
    {
	/*  Successfully queued, wait for completion. */

#ifdef SR_SUSPEND_THREAD
        SR_SUSPEND_THREAD;
#endif
	if (local_iosb.iosb$w_status & 1)
	{
	    /*	Deassign the channel. */

	    s = sys$dassgn(f->io_channel);
	    if (s & 1)
	    {
		/*  Return successful. */

		f->io_channel = 0;
		f->io_type = 0;
		return (OK);
	    }   
	}
	else
	    s = local_iosb.iosb$w_status;
    }

    err_code->error = s;
    return (SR_BADCLOSE);
}

/*{
** Name: SRopen - Opens a file.
**
** Description:
**      The SRopen routine is used to open a direct access file.  
**      It will open the file for write access and will
**      place no locks on the file.  If the create_flag is set 
**      it will create it and allocate the amount specified.
**      Additional space may be required to accomplish the sort.
**      An unlimited number of extensions must be allowed.
**      You can specify that nothing should be allocated at
**      create time.
**   
**	SR files are allocated in SR_MIN_INCR-block chunks,
**	i.e. (SR_MIN_INCR/2) Kb chunks.  Ideally, one reads and
**	writes SR files using a constant page size;  but, it's
**	allowable to read with a larger page size as long as both
**	old and new page sizes are a power of 2 no larger than
**	(SR_MIN_INCR/2) Kb.  Likewise, it's allowable to read an
**	existing SR file with a smaller page size as long as it
**	divides evenly into the original page size.  In general there
**	is no burning need to flail around with SR file page sizes...
**   
** Inputs:
**      f                    Pointer to the SR file
**                           context needed to do I/O.
**      path                 Pointer to the directory name
**                           for the file.
**      pathlength           Length of path name.
**      filename             Pointer to file name.
**      filelength           Length of file name.
**      pagesize             Value indicating size of page 
**                           in bytes.  Must be a multiple of
**				the VMS page size of 512 bytes.
**      create_flag          Value indicating if creation needed.
**                           Must be SR_IO_CREATE.
**      n                    Value indicating number of pages to
**                           pre-allocate.
**      
** Outputs:
**      f                    Updates the file control block.
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**    Returns:
**          OK
**          SR_BADDIR		Error in path specification.
**          SR_BADOPEN		Error opening file.
**          SR_BADFILE		Bad file context.
**          SR_PADPARAM		Parameter(s) in error.
**	    SR_EXCEED_LIMIT	Too many open files, exceeding disk quota
**				or exceeding available disk space.
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    30-sep-85 (jennifer)
**          Created new for 5.0.
**      02-apr-86 (jennifer)
**          Modified the system error returned from a i4 to
**          the type CL_ERR_DESC per CL request.
**	27-jul-87 (rogerk)
**	    Added return code SR_EXCEED_LIMIT.
**	17-aug-87 (rogerk)
**	    Save local return status after CSsuspend call before acting on
**	    its value.  We were losing the return status from the allocate
**	    call by seting the local return status from CSsuspend later on.
**	11-may-98 (kinte01)
**	    Added page size for 65536
*/
STATUS
SRopen(
SR_IO           *f,
char            *path,
u_i4		pathlength,
char            *filename,
u_i4	        filelength,
i4		pagesize,
u_i4		create_flag,
i4         n,
CL_ERR_DESC	*err_code)
{
    IOSB		local_iosb;         /* Operation return status. */
    i4 	        s;		    /* Request return status. */
    FIBDEF		*fib;
    struct
    {
	i4	    count;
	char	    *pointer;
    }			descriptor;
    CS_SID		sid;

    CL_CLEAR_ERR(err_code);

#ifdef SR_CHECK_RESUME
    if (SR_check_resume)
    {
       SR_check_init();
    }
#endif 

    /* Page size has to be a multiple of 512. */
    if ((pagesize & 511) != 0)
	return (SR_BADPARAM);

    /* Initialize file control block. */

    MEfill (sizeof(SR_IO), NULL, (PTR)f);
    f->io_type = SR_IO_ASCII_ID;
    CSget_sid(&sid);

    /* Assign a channel for I/O. */

    descriptor.count = pathlength;
    descriptor.pointer = path;
    s = sys$assign(&descriptor, &f->io_channel, 0, 0);
    if ((s & 1) == 0)
    {
        err_code->error = s;
	if ((s == SS$_EXQUOTA) || (s == SS$_EXDISKQUOTA) || (s == SS$_NOIOCHAN))
	    return (SR_EXCEED_LIMIT);
        return (SR_BADOPEN); 
    }

    /* Access the file with no locking.*/

    fib = (FIBDEF *)f->io_fib;
    fib->fib$l_acctl = FIB$M_NOLOCK | FIB$M_WRITE;
    fib->fib$w_nmctl = 0;
    fib->fib$w_exctl = 0;
    f->io_fib_desc.str_len = sizeof(f->io_fib);
    f->io_fib_desc.str_str = f->io_fib;

    /*	Queue the Create call. */
    s = sr_qio(EFN$C_ENF, f->io_channel, 
		IO$_CREATE | IO$M_CREATE | IO$M_ACCESS | IO$M_DELETE,
		&local_iosb, sr_resume_fcn, sid,
		&f->io_fib_desc, 0, 0, 0, 0, 0);

    if (s & 1)
    {
	/*  Successfully queued, wait for completion. */

#ifdef SR_SUSPEND_THREAD
        SR_SUSPEND_THREAD;
#endif
	s = local_iosb.iosb$w_status;
	if (local_iosb.iosb$w_status & 1)
	{
	    /*	Successful completion, setup the control block. */

	    fib->fib$l_acctl = 0;
	    f->io_block_size = pagesize;
	    f->io_blks = pagesize >> 9;

	    /* Don't bother with io_log2_blk, nothing uses it... */
	    if (n != 0)
	    {
		/*  Preallocate space for the file. */

		s = iiSRallocate(f, n, sid, err_code);
		if (s == OK)
		    return (OK);

		/*
		** If allocate returns an error, stuff the os errcode into
		** 's' for processing below.
		*/
		s = err_code->error;
	    }
	    else
	    {
		return (OK);
	    }
	}
    }

    /*	Return error. */

    err_code->error = s;
    sys$dassgn(f->io_channel);
    f->io_type = 0;
    if ((s == SS$_DEVICEFULL) || (s == SS$_EXQUOTA) || (s == SS$_EXDISKQUOTA))
	return (SR_EXCEED_LIMIT);
    return (SR_BADCREATE);
}

/*{
** Name: SRreadN - Read N pages of a file.
**
** Description:
**      The SRreadN routine is used to read pages of a direct access 
**      work file. 
**   
** Inputs:
**      f                    Pointer to the SR file
**                           context needed to do I/O.
**      page                 Value indicating page to read.
**      buf                  Pointer to the area to hold
**                           page being read.
**	n		     Number of pages to read.  Caller needs to ensure
**			     that the read doesn't read past EOF.
**      
** Outputs:
**      f                    Updates the file control block.
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**    Returns:
**          OK
**          SR_BADFILE		Bad file context.
**          SR_BADREAD		Error reading file.
**          SR_ENDFILE		End of file reached.
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    30-sep-85 (jennifer)
**          Created new for 5.0.
**      02-apr-86 (jennifer)
**          Modified the system error returned from a i4 to
**          the type CL_ERR_DESC per CL request.
**      04-sep-2008 (horda03) Bug 120775
**          Different ways to calculate the VMS file block
**          for the required page if io_log2_blk is negative.
**	15-Oct-2009 (kschendel) SIR 122512
**	    Swap around params to match unix/win and DI.
*/
STATUS
SRreadN(
SR_IO	        *f,
i4		n,
i4         page,
char            *buf,
CL_ERR_DESC	*err_code)
{
    i4           s;                    /* Request return status. */
    IOSB	 local_iosb;           /* Operation return status. */ 
    CS_SID	 sid;

    CL_CLEAR_ERR(err_code);
    /* Check file control block pointer, return if bad.*/

    if (f->io_type == SR_IO_ASCII_ID) 
    {
	/*  Queue the read request. */

	CSget_sid(&sid);

	s = sr_qio(EFN$C_ENF, f->io_channel, 
                 IO$_READVBLK, &local_iosb, sr_resume_fcn, sid,
                 buf, n*f->io_block_size, (page * f->io_blks) + 1, 0, 0, 0
                );

	if (s & 1)
	{
	    /*	Successfully queued, wait for completion. */

#ifdef SR_SUSPEND_THREAD
            SR_SUSPEND_THREAD;
#endif

	    /*	Check completion status. */

	    if (local_iosb.iosb$w_status & 1)
		return (OK);
	    s = local_iosb.iosb$w_status;
	}

	/*  Return error. */

	err_code->error = s;
	return (SR_BADREAD);
    }

    err_code->error = OK;    
    return (SR_BADFILE);
}

/*{
** Name: SRwriteN -  Writes N pages of a file.
**
** Description:
**      The SRwrite routine is used to write pages of a direct access 
**      file.  If file space has to be allocated first, that is done.
**   
** Inputs:
**      f                    Pointer to the SR file
**                           context needed to do I/O.
**      page                 Value indicating page to write.
**      buf                  Pointer to page to write.
**	n		     Number of pages to write.
**      
** Outputs:
**      f                    Updates the file control block.
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**    Returns:
**          OK
**          SR_BADFILE		Bad file context.
**          SR_BADWRITE		Error writing file.
**          SR_BADPARAM		Parameter(s) in error.
**	    SR_EXCEED_LIMIT	Exceeding disk quota or exceeding available
**				disk space.
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    11-sep-85 (jennifer)
**          Created new for 5.0.
**      02-apr-86 (jennifer)
**          Modified the system error returned from a i4 to
**          the type CL_ERR_DESC per CL request.
**	27-jul-87 (rogerk)
**	    Added return code SR_EXCEED_LIMIT.
**      04-sep-2008 (horda03) Bug 120775
**          Different ways to calculate the VMS file block
**          for the required page if io_log2_blk is negative.
**	15-Oct-2009 (kschendel) SIR 122512
**	    Swap around params to match unix/win and DI.
*/
STATUS
SRwriteN(
SR_IO	        *f,
i4		n,
i4         page,
char            *buf,
CL_ERR_DESC	*err_code)
{
    i4           s;                    /* Request return status. */
    IOSB	 local_iosb;           /* Operation return status. */ 
    CS_SID	 sid;

    CL_CLEAR_ERR(err_code);
    /* Check file control block pointer, return if bad.*/

    if (f->io_type == SR_IO_ASCII_ID) 
    {
	CSget_sid(&sid);

	if (f->io_allocated < ((page+n) * f->io_blks))
	{
	    s = iiSRallocate(f, n, sid, err_code);
	    if (s != OK)
		return (s);
	}

	s = sr_qio(EFN$C_ENF, f->io_channel, 
                     IO$_WRITEVBLK, &local_iosb, sr_resume_fcn, sid,
                     buf, n*f->io_block_size, (page * f->io_blks) + 1, 0, 0, 0
                    );

	if (s & 1)
	{
	    /*	Successfully queued, wait for completion. */

#ifdef SR_SUSPEND_THREAD
            SR_SUSPEND_THREAD;
#endif

	    /*	Check completion status. */

	    if (local_iosb.iosb$w_status & 1)
		return (OK);
	    s = local_iosb.iosb$w_status;
	}

	/*  Return error. */

	err_code->error = s;
	return (SR_BADWRITE);
    }

    err_code->error = OK;    
    return (SR_BADFILE);
}

/*
** Name: iiSRallocate - allocates a page to a direct access file.
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
**      f                Pointer to the SR file
**                       context needed to do I/O.
**      n                The number of pages to allocate.
**
** Outputs:
**      err_code         Pointer to a variable used
**                       to return operating system 
**                       errors.
**    Returns:
**        OK
**        SR_BADEXTEND
**        SR_BADFILE
**	  SR_EXCEED_LIMIT
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    30-sep-85 (jennifer)
**          Created new for 5.0.
**      02-apr-86 (jennifer)
**          Modified the system error returned from a i4 to
**          the type CL_ERR_DESC per CL request.
**	09-mar-1987 (Derek)
**	    Changed extend options to include ALCON so that we always allocate
**	    the largest extent less than or equal to the extend size.
**	27-jul-87 (rogerk)
**	    Added return code SR_EXCEED_LIMIT.
**      03-Jun-2009 (horda03) Bug 111687
**          Get the number of blocks allocated from the fib$l_exsz.
*/
static	STATUS
iiSRallocate(
SR_IO		*f,
i4		n,
i4		sid,
CL_ERR_DESC	*err_code)
{
    int            blocks;
    i4	          s;                    /* Request return status. */
    IOSB	  local_iosb;           /* Operation return status. */
    i4            allocated;    
    i4            amount;               /* Amount allocated each
                                        ** request. */
    i4		  extend_count;
    FIBDEF	  *fib;

    CL_CLEAR_ERR(err_code);
    /* Convert allocation size from pages to blocks. */

    blocks = n * f->io_blks;             
    allocated = f->io_allocated;

    /* 
    ** Set file I/O control block extention parameters 
    ** for the allocation I/O request.
    */

    fib = (FIBDEF *)f->io_fib;
    fib->fib$l_acctl = 0;
    fib->fib$w_exctl = FIB$M_EXTEND | FIB$M_ALCONB | FIB$M_ALDEF | FIB$M_ALCON;
    extend_count = 0;

    /*
    ** Repeat request for extensions until the number of blocks
    ** requested by the caller is allocated or until an error
    ** is returned indicating no more space.
    */

    while (blocks > 0)
    {
	fib->fib$l_exsz = SR_MIN_INCR;
	if (blocks > SR_MIN_INCR)
	    fib->fib$l_exsz = blocks;
	fib->fib$l_exvbn = 0;

        s = sr_qio(EFN$C_ENF, f->io_channel, IO$_MODIFY, 
                     &local_iosb, sr_resume_fcn, sid,
                     &f->io_fib_desc, 0, 0, 0, 0, 0
                    );

	if (s & 1)
	{
#ifdef SR_SUSPEND_THREAD
            SR_SUSPEND_THREAD;
#endif
	    if (local_iosb.iosb$w_status & 1)
	    {
		++extend_count;
		amount = fib->fib$l_exsz;
		allocated += amount;
		blocks -= amount;
		if (blocks < (1 << extend_count))
		    fib->fib$w_exctl &= ~FIB$M_ALCON;
		continue;
	    }

	    /*	Request failed. */

	    s = local_iosb.iosb$w_status;
	}

	err_code->error = s;
	if ((s == SS$_EXDISKQUOTA) || (s == SS$_DEVICEFULL) ||
	    (s == SS$_EXQUOTA))
	    return (SR_EXCEED_LIMIT);
	return (SR_BADEXTEND);
    }

    /* Reset file I/O control block extension request values. */

    f->io_allocated = allocated;
    return (OK);
}

#ifdef SR_CHECK_RESUME
static void
SR_check_init()
{
   if (!CS_is_mt())
   {
      sr_resume_fcn = SRresumeFromAST;
      sr_getdvi     = sys$getdvi;
      sr_qio        = sys$qio;
   }

   SR_check_resume = 0;
}
#endif
