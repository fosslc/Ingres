/*
** Copyright (c) 1985, 2009 Ingres Corporation
*/

# include   <compat.h>
# include   <gl.h>
# include   <eventflag.h>
# include   <cs.h>
# include   <di.h>
# include   <me.h>
# include   <rms.h>
# include   <tr.h>
# include   <er.h>
# include   <st.h>
# include   <descrip.h>
# include   <efndef.h>
# include   <iledef.h>
# include   <iodef.h>
# include   <iosbdef.h>
# include   <fibdef.h>
# include   <atrdef.h>
# include   <fatdef.h>
# include   <jpidef.h>
# include   <ssdef.h>
#ifdef xDEBUG
# include   <dvidef.h>
# include   <dcdef.h>
#endif
# include   <lib$routines.h>
# include   <starlet.h>
# include   <vmstypes.h>
# include   <astjacket.h>

/**
**
**  Name: DI.C - This file contains all the VMS DI routines.
**
**  Description:
**      The DI.C file contains all the routines needed to perform
**      direct I/O to shared files.  All the database table files and the
**      database configuration file are accessed using these 
**      routines.  
**
**        DIalloc - Allocates pages to a file.
**        DIclose - Closes a file.
**        DIcreate - Creates a new file.
**        DIdelete -  Deletes a file.
**        DIdircreate - Creates a directory.
**        DIdirdelete - Deletes a directory.
**        DIflush - Flushes header to a file.
**        DIforce - Forces pages to disk.
**        DIlistdir - List all directories in a directory.
**        DIlistfiles - List all files in a directory.
**        DIopen - Opens a file.
**        DIread - Reads a page from a file.
**        DIrename - Changes name of a file.
**        DIsense - Verifies a page exists in file.
**        DIwrite - Writes a page to a file.
**	  DIgalloc - Allocate pages to a file guaranting that the disk space
**	             will be available.
**	  DIguarantee_space - Guarantee disk space for a range of pages that
**	                      have been previously DIalloc or Diaglloced.
** 
**	Internal routines.
**
**        getpath -	Lookup path in path cache.
**        getdirid -	Get identifier for path cache.
**	  list_files -	Return file names one by one.
**
**      For DMF pages in the files created are numbered 0,1,2,...n, however
**      VMS uses blocks which are numbered 1,2,3,...n.  There are
**      4 blocks (512 byte sectors) per page.  Therefore  page number
**      is converted to corresponding first block number as follows:
**
**           block = (page * blocks-per-page) + 1
**
**      Therefore page 0 starts with block 1, page 1 starts with block
**      5, etc.
**
**      VMS keeps track of number of pages allocated.  It also expects 
**      the maintainer of direct access files to maintain a file header
**      which contains the number of blocks allocated which is referred to
**      as io_rec.rec_eof and the number of blocks used which is
**      referred to as io_rec.rec_used.  For VMS the eof always points to 
**      the next block that would be allocated and used always points
**      to one past the last block used. Internally we assume the last page
**      with data on it is the last one used, therefore a conversion must
**      take place everytime we return  the logical end of file (used).
**      The conversion is as follows:
**
**      eof = io_rec.rec_used -1           This in all in blocks. 
**      page = (eof/blocks-per-page) -1    Subtract 1 for zero relative. 
**   
**
**  History:    $Log-for RCS$
**      06-sep-85 (jennifer)    
**          Created new for 5.0.
**      06-nov-86 (jennifer)
**          Added error returns for DIcreate and DIdircreate.
**          Moved DI_PATH_MAX and DI_FILENAME_MAX to global 
**          header.
**	28-Nov-1986 (fred)
**	    Added dispatcher suspend/resume support.
**	09-mar-1987 (Derek)
**	    Changed DIalloc to reduce number of I/O's needed to
**	    extend a file.
**	10-mar-1987 (Derek)
**	    Fixed resource problems associated with dangling sys$parse()
**	    calls.
**	27-mar-1987 (Derek)
**	    Fix some botched calls from previous fix.
**	27-jul-1987 (rogerk)
**	    Check for SS$_NOIOCHAN and SS$_EXQUOTA and return DI_EXCEED_LIMIT.
**	05-nov-1987 (rogerk)
**	    In DIlistdir and DIlistfile, return DI_ENDFILE when the user
**	    function returns something other than OK.  Also, return DI_BADLIST
**	    instead of DI_BADDIR when listing of files in directory fails.
**	    Also, allocate buffer larger than DI_FILENAME_MAX to put listed
**	    filenames into.  Just becuase DI limits the filename length of
**	    files doesn't mean there isn't something bigger than that in the
**	    directory.
**	23-dec-1989 (mikem)
**	    Update spec for DIopen().
**	22-feb-1989 (greg)
**	    CL_ERR_DESC member error must be initialized to OK
**      05-sep-89 (jennifer)
**          For B1, server must own all files not the user.
**	14-jun-1991 (Derek)
**	    Added support for DI_ZERO_ALLOC flag.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    10-jan-91 (rachael)
**		Return DI_BADFILE if file control block's io_type does not
**		contain DI_IO_ASCII_ID.
**	    10-apr-91 (teresa)
**		Fixed assorted bugs where it was possible to overflow
**		local buffer "filename" in DIdirdelete if the directory name
**		approximated DB_MAXNAME.
**	10-Feb-1992 (rmuth)
**	    DIalloc() would only allow allocation of space to a file in
**	    DI_MIN_INCR, 64 VMS blocks, sizes. 
**	    As the user can now specify the allocation and extend size
**	    for a table remove the above constraint. Also there was a 
**	    coding problem related to the above which was causing us to
**	    loose 12 Ingres pages if the default allocation of 4 Ingres 
**	    pages was used.
**	    
**	15-apr-1992 (bryanp)
**	    Document non-use of DI_IO argument to DIrename().
**	22-jul-1992 (bryanp)
**	    If sys$parse returns RMS$_CHN, treat this as SS$_NOIOCHAN; that is,
**	    return DI_EXCEED_LIMIT to our caller.
**	14-oct-1992 (bryanp)
**	    Add support for page sizes greater than 4K.
**	30-October-1992 (rmuth)
**	    - Add DIgalloc and DIguarantee_space.
**	    - REmove references to ZERO_ALLOC.
**	    - Remove check from DIalloc where we compare the logical eof
**	      with the physical eof to see if there is any free space.
**	      As we now track logical end-of-file in the FHDR/FMAP(s) this
**	      check has been moved to a higher level.
**	30-feb-1993 (rmuth)
**	    VMS does not seem to allow you to allocate less than eight 
**          disc blocks at a time, so if we make a request for one Ingres
**          page of four VMS blocks we actually get eight VMS blocks. As
**          the upper layers do not know about this extra space the space
**          is wasted. So replace the above logical/physical eof check to
**          compensate. Note this is only occurs if you request a extension
**          to a table of ONE ingres page.page.
**	26-apr-1993 (bryanp)
**	    Finished prototyping DI for 6.5.
**	    Note that arg_list is a PTR, not a (PTR *).
**	02-dec-1992 (walt)
**	    Created a writable copy of the input file name parameter in DIcreate 
**	    DIdelete and DIopen on Alpha because sys$qio was returning SS$_ACCVIO 
**	    as status when the filenames were C strings (ex: "aaaaaaaa.cnf") in the
**	    call.
**	    The Alpha C compiler puts strings into non-writable memory and somehow 
**	    sys$qio wants to access the actual string location for write.
**	30-feb-1993 (rmuth)
**	    VMS does not seem to allow you to allocate less than eight 
**          disc blocks at a time, so if we make a request for one Ingres
**          page of four VMS blocks we actually get eight VMS blocks. As
**          the upper layers do not know about this extra space the space
**          is wasted. So replace the above logical/physical eof check to
**          compensate. Note this is only occurs if you request a extension
**          to a table of ONE ingres page.page.
**      16-jul-93 (ed)
**	    added gl.h
**	17-aug-93 (walt)
**	    In DIclose, expand itmlst to be an array in order to terminate the
**	    list with a set of zeros.  This silences an SS$_BADPARAM error
**	    return from sys$getdvi.
**      18-apr-94 (jnash)
**	    Add DI_USER_SYNC_MASK DIopen() mode, which translates into 
**	    DI_SYNC_MASK.  We have chosen not to implement async i/o in 
**	    vms at this time.
**      15-jul-97 (carsu07)
**          Added to the case statement in DIopen() to include a pagesize
**          of 512 and 1024. The pagesize is specified in dmfreloc.c. 
**      19-aug-1997 (teresak)
**          Add new dummy routine DIwrite_list 
**      17-oct-1997 (rosga02)
**      Integrated changes from ingres63p change 276123
**        Item list used in the SYS$GETJPI system service call was not
**        null terminated.  This caused spurious corruption problems.
**	05-feb-1998 (kinte01)
**	    Add missing entry for 64K pagesize which is supported now.
**	02-oct-1998 (somsa01)
**	    In DIwrite(), in case of SS$_DEVICEFULL (physical device full)
**	    or SS$_EXDISKQUOTA (user allocated disk full), return
**	    DI_NODISKSPACE.
**      28-Jan-2000 (horda03) X-Int of fixes for bug 48904
**         30-mar-94 (rachael)
**           Added call to CSnoresnow to provide a temporary fix/workaround
**           for sites experiencing bug 48904. CSnoresnow checks the current
**           thread for being CSresumed too early.  If it has been, that is if 
**           scb->cs_mask has the EDONE bit set, CScancelled is called.
**	19-jul-2000 (kinte01)
**	     Correct prototype definitions by adding missing includes and
**	     external function refernces
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	07-feb-01 (kinte01)
**	    Add casts to quiet compiler warnings
**     18-sep-01 (kinte01)
**          Renamed elements of _DI_IO struct to be same as Unix structure for 
**	    raw log support
**	02-sep-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**	18-oct-2007 (abbjo03)
**	    If returning DI_ENDFILE, TRdisplay io_alloc_eof/io_system_eof.
**	04-Apr-2008 (jonj)
**	    Get a event flag using sys$get_ef() rather than using CACHE_IO_EF,
**	    which is not thread-safe.
**	09-oct-2008 (stegr01/joea)
**	    Replace II_VMS_ITEM_LIST_3 by ILE3.
**	24-oct-2008 (joea)
**	    Use EFN$C_ENF and IOSB when calling a synchronous system service.
**	11-nov-2008 (joea)
**	    Use CL_CLEAR_ERR to initialize CL_ERR_DESC.
**      04-jun-2009 (horda03) Bug 111687
**          Use fib->fib$l_exsz to determine how many blocks a file has been
**          extended by.
**	17-jul-2009 (joea)
**	    Simplify the checking for completion in DI_qio.  Correct several
**	    compiler warnings.
**	10-Nov-2010 (kschendel) SIR 124685
**	    Prototype fixes.
**	    Delete obsolete DIwrite_list, not used anywhere.
**/                                                  


/*
**  Local Definitions.
*/

#define                 DI_PCACHE_MAX	32
                                        /* Max # of paths to cache. */

/*
** Filename extension used for specifying directories:
** On VMS, a ".dir" is appended for directories.  The ".1" refers to the
** file version number.
*/
#define			DI_DIREXTENSION	    ".dir.1"


/*
** A union for use with ACP record attributes, which maintain
** the VBN in inverted format (high- and low-order words
** transposed) for PDP-11 compatibility.
*/
typedef union
{
    struct
    {
	u_i2	normal_low;
	u_i2	normal_high;
    } normal;
    i4	long_word;
} F11_JUNK;


/*}
** Name: PATH_ENTRY - Area to contain directory path information.
**
** Description:
**      This a VMS specific internal structure.
**      This structure is used to contain an operating system 
**      identifier for a directory path.  This information is used 
**      to speed up access to files within a directory.  Specifically
**      it provides enough information to the operating system to
**      allow it to find the file with one access instead of searching
**      the directory tree.
**
** History:
**     06-sep-85 (jennifer)
**          Created new for 5.0.
*/
typedef struct
{
    i4	    c_refcount;		/* LRU reference count. */
    short	    c_devnam_cnt;	/* Number of characters in device. */
    short	    c_path_cnt;		/* Number of characters in path. */
    short	    c_io_direct[3];     /* Directory id    */
    char	    c_device[30];	/* Base device name. */
    char            c_path[DI_PATH_MAX];/* Full path name */
}   PATH_ENTRY;

/*
**  Definition of static variables and forward static functions.
*/

static  PATH_ENTRY	    path_cache[DI_PCACHE_MAX];


static STATUS	getpath(
		    char            *path,
		    i4             pathlength,
		    PATH_ENTRY	    *path_entry,
		    i4		    retry,
		    i4		    sid,
		    CL_ERR_DESC      *err_code);
static STATUS	list_files(
		    struct FAB	    *fab,
		    STATUS	    (*func)(),
		    PTR		    arg_list,
		    int		    flag,
		    CL_ERR_DESC	    *err_code);
static STATUS	rms_suspend(
		    STATUS	    s,
		    struct FAB	    *fab);
static void	rms_resume(
		    struct FAB	    *fab);
static i4	DI_qio(
		    char *descrip,
		    II_VMS_CHANNEL   channel,
		    i4   func,
		    IOSB *iosb,
                    i4   p1,
                    i4   p2,
                    i4   p3,
                    i4   p4,
                    i4   p5,
                    i4   p6);
static void	DI_resume(CS_SID *sid);

static void     DI_check_init(void);

/* OS and Internal threads require different handling for asynchronous calls */
#ifdef OS_THREADS_USED
#  ifdef ALPHA
#     define DI_CHECK_RESUME
      i4 DI_check_resume = 1;

      /* Only need to suspend if running internal threads or there's
      ** a possibility of running internal threads
      */

#     define DI_SUSPEND_THREAD if (resume_fcn) CSsuspend(CS_DIO_MASK, 0, 0)
#  endif
#
#  define DI_RESUME_FCN NULL

#else /* OS_THREADS_USED */

#  define DI_RESUME_FCN rms_resume
#  define DI_SUSPEND_THREAD CSsuspend(CS_DIO_MASK, 0, 0)

#endif

void (*resume_fcn)() = DI_RESUME_FCN;


/*
** Name: DIget_direct_align - Get platform direct I/O alignment requirement
**
** Description:
**	Some platforms require that buffers used for direct I/O be aligned
**	at a particular address boundary.  This routine returns that
**	boundary, which is guaranteed to be a power of two.  (If some
**	wacky platform has a non-power-of-2 alignment requirement, we'll
**	return the nearest power of 2, but such a thing seems unlikely.)
**
**	I suppose this could be a manifest constant, like DI_RAWIO_ALIGN,
**	but doing it as a routine offers a bit more flexibility.
**
**	VMS may or may not support some kind of unbuffered I/O, but
**	I will wait for a platform expert to deal with it.  For now,
**	return zero.
**
** Inputs:
**	None.
**
** Outputs:
**	Returns required power-of-2 alignment for direct I/O;
**	may be zero if no alignment needed, or direct I/O not supported.
**
** History:
**	6-Nov-2009 (kschendel) SIR 122757
**	    Stub for VMS.
*/

u_i4
DIget_direct_align(void)
{
    /* Direct (unbuffered) IO not yet supported for VMS */
    return (0);
} /* DIget_direct_align */

/*{
** Name: DIguarantee_space - Guarantee space is on disc.
**
** Description:
**	This routine will guarantee that once the call has completed
**	the space on disc is guaranteed to exist. The range of pages
**	must have already been reserved by DIalloc or DIgalloc the
**	result of calling this on pages not in this range is undefined.
**	
**   Notes:
**
**   a. On file systems which cannot pre-allocate space to a file this 
**      means that DIguarantee_space will be required to reserve the space 
**      by writing to the file in the specified range. On File systems which 
**      can preallocate space this routine will probably be a nop.
**
 **  b. Currently the main use of this routine is in reduction of I/O when 
**      building tables on file systems which cannot preallocate space. See 
**      examples.
**
**   c. DI does not keep track of space in the file which has not yet
**      been guaranteed on disc this must be maintained by the client.
**
**   d. DI does not keep track of what pages have been either guaranteed on
**      disc or DIwritten and as this routine will overwrite whatever data may
**      or maynot already be in the requested palce the client has the ability 
**      to overwrite valid data.
**
**   e. The contents of the pages guaranteed by DIguarantee_space
**      are undefined until a DIwrite() to the page has happened. A DIread
**      of these pages will not fail but the contents of the page returned is
**      undefined.
**
** Inputs:
**      f               Pointer to the DI file context needed to do I/O.
**	start_pageno	Page number to start at.
**	number_pages	Number of pages to guarantee.
**
** Outputs:
**      err_code         Pointer to a variable used to return operating system 
**                       errors.
**    Returns:
**        OK
**				
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    30-October-1992 (rmuth)
**	Created.
**
** Design Details:
**
**	On VMS this is a nop as we both dialloc and digalloc prreallocate
**	space to the file.
**
*/
STATUS
DIguarantee_space(
    DI_IO		*f,
    i4		start_pageno,
    i4		number_pages,
    CL_ERR_DESC		*err_code)
{
    CL_CLEAR_ERR(err_code);
    return( OK );
}

/*{
** Name: DIgalloc - Allocates N pages to a direct access file.
**
** Description:
**	The DIgalloc routine is used to add pages to a direct access
**	file, the disc space for these pages is guaranteed to exist
**	once the routine returns. The contents of the pages allocated
**	are undefined until a DIwrite to the page has happened.
**
**      This routine can add more than one page at a time by accepting 
**	a count of the number of pages to add.
**   
**	During the period between a DIgalloc() call and a DIflush() call
**	the logical end-of-file, as determined by DIsense(), is undefined.
**
** Inputs:
**      f                Pointer to the DI file
**                       context needed to do I/O.
**      n                The number of pages to allocate.
**
** Outputs:
**      page             Pointer to variable used to 
**                       return the page number of the
**                       first page allocated.
**      err_code         Pointer to a variable used
**                       to return operating system 
**                       errors.
**    Returns:
**        OK
**        DI_BADEXTEND      	Can't allocate disk space
**        DI_BADFILE        	Bad file context.
**        DI_EXCEED_LIMIT   	Too many open files.
**
**	  
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** Design Details:	    
**
**      The number of pages is converted to the number of blocks
**      (sectors) to add.  If the amount requested cannot 
**      be obtained in large contiguous areas, this routine tries to find
**      as many smaller fragments needed to meet the request.  It will
**      never try for a fragment less than the minimum allocation size.
**
** History:
**    30-October-1992 (rmuth)
**	Created.
**    30-feb-1993 (rmuth)
**	VMS does not seem to allow you to allocate less than eight 
**      disc blocks at a time, so if we make a request for one Ingres
**      page of four VMS blocks we actually get eight VMS blocks. As
**      the upper layers do not know about this extra space the space
**      is wasted. So replace the above logical/physical eof check to
**      compensate. Note this is only occurs if you request a extension
**      to a table of ONE ingres page.
**    04-jun-2009 (horda03) Bug 111687
**      Use fib->fib$l_exsz to determine how many blocks a file has been
**      extended by.
**
** Design Details:
**
*/
STATUS
DIgalloc(
    DI_IO         	*f,
    i4	        n,
    i4	        *page,
    CL_ERR_DESC		*err_code )
{
     int            blocks;
    FAT		    io_rec;
    FIBDEF	    *fib;
                                        /* RMS record information */
    ATRDEF	    io_att[] =
                           {   
                                { ATR$S_RECATTR, ATR$C_RECATTR, (char *)&io_rec},
                                { 0, 0, 0}
                           };           /*    attr fetch information    */

    i4		    s;                    /* Request return status. */
    IOSB	    local_iosb;           /* Operation return status. */
    F11_JUNK	    current_eof;
    F11_JUNK	    new_eof;
    F11_JUNK	    current_allocated;    
    F11_JUNK	    new_allocated;    
    i4		    amount;
    i4		    extend_count;
    CL_ERR_DESC	    lerr_code;

    CL_CLEAR_ERR(err_code);
   
    /* Check file control block pointer, return if bad.*/

    if (f->io_type != DI_IO_ASCII_ID) 
        return (DI_BADFILE);
    fib = (FIBDEF *)f->io_fib;

    /* Get end of allocation and end of file information from file. */

    s = DI_qio( "DI!DI.C::DIgalloc_1",
                f->io_channel, IO$_ACCESS, &local_iosb,
                 (i4)&f->io_fib_desc, 0, 0, 0, (i4)io_att, 0);

    if (s & 1)
    {
	if (local_iosb.iosb$w_status & 1)
	{
	    /* 
	    ** Work out what the current eof and allocated are
	    */
	    current_eof.normal.normal_high = io_rec.fat$w_efblkh;
	    current_eof.normal.normal_low = io_rec.fat$w_efblkl;    
	    if (current_eof.long_word)
		current_eof.long_word--;

	    current_allocated.normal.normal_high = io_rec.fat$w_hiblkh;
	    current_allocated.normal.normal_low = io_rec.fat$w_hiblkl;

	    /* 
	    ** Convert allocation size from pages to blocks.
	    */
	    blocks = n << f->io_blks_log2;

	    /*
	    ** If enough space between the physical and logical eof
	    ** to satisfy the request then use this space
	    */
	    if ( current_eof.long_word + blocks <= current_allocated.long_word)
	    {
	        f->io_system_eof = current_eof.long_word + blocks + 1;
	        *page = current_eof.long_word >>  f->io_blks_log2;

		(void) DIflush( f, &lerr_code);

		return( OK );
	    }

	    /*
	    ** Work out what the new eof will be if all goes ok and setup
	    ** structure so that we can write it to disc.
	    */
	    new_allocated.long_word = current_allocated.long_word;
	    new_eof.long_word = current_eof.long_word + blocks + 1;
	    MEfill(sizeof(FAT), 0, (char *)&io_rec);
	    io_rec.fat$v_rtype = FAT$C_FIXED;
	    io_rec.fat$w_rsize  = f->io_block_size;
	    io_rec.fat$w_efblkh = new_eof.normal.normal_high;
	    io_rec.fat$w_efblkl = new_eof.normal.normal_low;
			

	    fib->fib$l_acctl = FIB$M_WRITETHRU;
	    fib->fib$w_exctl = FIB$M_EXTEND | FIB$M_ALCONB | FIB$M_ALDEF |
		FIB$M_ALCON;

	    /*
	    ** Extend the file, initaially try to allocate a contiguous
	    ** junk.
	    */
	    extend_count = 0;
	    while (blocks > 0)
	    {
		fib->fib$l_exsz = blocks;
		fib->fib$l_exvbn = 0;

		s = DI_qio("DI!DI.C::DIgalloc_2",
                           f->io_channel, IO$_MODIFY, &local_iosb,
                         (i4) &f->io_fib_desc, 0, 0, 0, (i4) io_att, 0);
		if (s & 1)
		{
		    if (local_iosb.iosb$w_status & 1)
		    {
			++extend_count;
			amount = fib->fib$l_exsz;
			new_allocated.long_word += amount;

			blocks -= amount;

			if (blocks < (1 << extend_count))
			    fib->fib$w_exctl &= ~FIB$M_ALCON;
			continue;
		    }

		    /*  Request failed. */
		    s = local_iosb.iosb$w_status;
		}

		err_code->error = s;

		/*
		** 
	 	** We may have an incorrect eof on disc so reset it
		** to a correct value. This can occur if we could not
		** allocate a contiguous piece of disc but we have managed
		** to allocate some chunks. 
		*/
	        f->io_alloc_eof = new_allocated.long_word;
		(void) DIflush( f, &lerr_code);

		if ((s == SS$_DEVICEFULL) || (s == SS$_EXDISKQUOTA) ||
		    (s == SS$_EXQUOTA))
		    return(DI_EXCEED_LIMIT);
		return (DI_BADEXTEND);
	    }

	    *page = current_eof.long_word >>  f->io_blks_log2;

	    f->io_system_eof = new_eof.long_word;
	    f->io_alloc_eof = new_allocated.long_word;
	    return (OK);
	}
    }

    err_code->error = s;
    return (DI_BADEXTEND);
}



/*{
** Name: DIalloc - Allocates a page to a direct access file.
**
** Description:
**      The DIalloc routine is used to add pages to a direct
**      access file.  This routine can add more than one page
**      at a time by accepting a count of the number of pages to add.
**   
**      The end of file and allocated are not updated on disk until a DIflush
**      call is issued.  This insures that pages are not considered valid 
**      until after they are formatted.  The allocation can be ignored if
**      the file is closed or the system crashes before the DIflush.
** Inputs:
**      f                Pointer to the DI file
**                       context needed to do I/O.
**      n                The number of pages to allocate.
**
** Outputs:
**      page             Pointer to variable used to 
**                       return the page number of the
**                       first page allocated.
**      err_code         Pointer to a variable used
**                       to return operating system 
**                       errors.
**    Returns:
**        OK
**        DI_BADEXTEND      Can't allocate disk space
**        DI_BADFILE        Bad file context.
**        DI_EXCEED_LIMIT   Too many open files, disk quota exceeded, or
**			    exceeded available disk space.
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    11-sep-85 (jennifer)
**          Created new for 5.0.
**    20-mar-86 (jennifer)
**          Changed longnat to nat and err_code to CL_ERR_DESC to meet 
**          CL coding standard.
**    09-mar-1987 (Derek)
**	    Changed extend request to use ALCON | ALCONB.  This causes
**	    the largest extend <= requested size to be allocated.
**    10-Feb-1992 (rmuth)
**	    DIalloc() would only allow allocation of space to a file in
**	    DI_MIN_INCR, 64 VMS blocks, sizes. 
**	    As the user can now specify the allocation and extend size
**	    for a table remove the above constraint. Also there was a 
**	    coding problem related to the above which was causing us to
**	    loose 12 Ingres pages if the default allocation of 4 Ingres 
**	    pages was used.
**    30-September-1992 (rmuth)
**	    - Remove check where we compare the logical eof with the 
**	      physical eof to see if there is any free space.
**	      As we now track logical end-of-file in the FHDR/FMAP(s) this
**	      check has been moved to a higher level.
**	    - During the build process or for temporary tables we may
**	      call DIalloc before we have called DIflush hence the
**	      we may not have updated the file header on disc to
**	      reflect the correct logical eof. But the DI_IO structure
**	      will hold this value so check to see which is greater and
**	      use this value.
**    30-feb-1993 (rmuth)
**	    VMS does not seem to allow you to allocate less than eight 
**          disc blocks at a time, so if we make a request for one Ingres
**          page of four VMS blocks we actually get eight VMS blocks. As
**          the upper layers do not know about this extra space the space
**          is wasted. So replace the above logical/physical eof check to
**          compensate. Note this is only occurs if you request a extension
**          to a table of ONE ingres page.page.
**    30-mar-94 (rachael)
**          Added call to CSnoresnow to provide a temporary fix/workaround
**          for sites experiencing bug 48904. CSnoresnow checks the current
**          thread for being CSresumed too early.  If it has been, that is if 
**          scb->cs_mask has the EDONE bit set, CScancelled is called.
**    04-jun-2009 (horda03) Bug 111687
**      Use fib->fib$l_exsz to determine how many blocks a file has been
**      extended by.
**
** Design Details:	    
**
**      The number of pages is converted to the number of blocks
**      (sectors) to add.  If the amount requested cannot 
**      be obtained in large contiguous areas, this routine tries to find
**      as many smaller fragments needed to meet the request.  It will
**      never try for a fragment less than the minimum allocation size.
*/
STATUS
DIalloc(
DI_IO         *f,
i4	      n,
i4	      *page,
CL_ERR_DESC    *err_code)
{
     int            blocks;
    FAT		    io_rec;
    FIBDEF	    *fib;
                                        /* RMS record information */
    ATRDEF	    io_att[] =
                           {   
                                { ATR$S_RECATTR, ATR$C_RECATTR, (char *)&io_rec},
                                { 0, 0, 0}
                           };           /*    attr fetch information    */

    i4		    s;                    /* Request return status. */
    IOSB	    local_iosb;           /* Operation return status. */
    F11_JUNK	    eof;
    F11_JUNK	    allocated;    
    i4		    amount;
    i4		    extend_count;

    CL_CLEAR_ERR(err_code);
   
    /* Check file control block pointer, return if bad.*/

    if (f->io_type != DI_IO_ASCII_ID) 
        return (DI_BADFILE);
    fib = (FIBDEF *)f->io_fib;

    /* Get end of allocation and end of file information from file. */

    s = DI_qio( "DI!DI.C::DIalloc_1",
                f->io_channel, IO$_ACCESS, &local_iosb,
                 (i4) &f->io_fib_desc, 0, 0, 0, (i4) io_att, 0);

    if (s & 1)
    {
	if (local_iosb.iosb$w_status & 1)
	{
	    /* Update file I/O control block with new EOF. */

	    eof.normal.normal_high  = io_rec.fat$w_efblkh;
	    eof.normal.normal_low = io_rec.fat$w_efblkl;    

	    if ( eof.long_word < f->io_system_eof )
		eof.long_word = f->io_system_eof;

	    if (eof.long_word)
		eof.long_word--;

	    allocated.normal.normal_high = io_rec.fat$w_hiblkh;
	    allocated.normal.normal_low = io_rec.fat$w_hiblkl;

	    /* 
	    ** Convert allocation size from pages to blocks. 
	    */
	    blocks = n << f->io_blks_log2;

	    /*
	    ** See if there is enough space between the logical and 
	    ** physical eof.
	    */
	    if ( eof.long_word + blocks <= allocated.long_word )
	    {
		f->io_system_eof = eof.long_word + blocks + 1;
		*page = eof.long_word >> f->io_blks_log2;
		return( OK );
	    }

	    fib->fib$l_acctl = 0;
	    fib->fib$w_exctl = FIB$M_EXTEND | FIB$M_ALCONB | FIB$M_ALDEF |
		FIB$M_ALCON;
	    extend_count = 0;
	    while (blocks > 0)
	    {
		fib->fib$l_exsz = blocks;
		fib->fib$l_exvbn = 0;
		s = DI_qio( "DI!DI.C::DIalloc_2",
                            f->io_channel, IO$_MODIFY, &local_iosb,
                         (i4) &f->io_fib_desc, 0, 0, 0, 0, 0);

		if (s & 1)
		{
		    if (local_iosb.iosb$w_status & 1)
		    {
			++extend_count;
			amount = fib->fib$l_exsz;
			allocated.long_word += amount;
			blocks -= amount;
			if (blocks < (1 << extend_count))
			    fib->fib$w_exctl &= ~FIB$M_ALCON;
			continue;
		    }

		    /*  Request failed. */
		    s = local_iosb.iosb$w_status;
		}

		err_code->error = s;
		if ((s == SS$_DEVICEFULL) || (s == SS$_EXDISKQUOTA) ||
		    (s == SS$_EXQUOTA))
		    return(DI_EXCEED_LIMIT);
		return (DI_BADEXTEND);
	    }

	    *page = eof.long_word >>  f->io_blks_log2;
	    f->io_system_eof = eof.long_word + (n << f->io_blks_log2) + 1;
	    f->io_alloc_eof = allocated.long_word;
	    return (OK);
	}
    }

    err_code->error = s;
    return (DI_BADEXTEND);
}

/*{
** Name: DIclose - Closes a file.
**
** Description:
**      The DIclose routine is used to close a direct access file.  
**      It will check to insure the file is open before trying to close.
**      A DIclose call implies a DIforce for this file.  That is
**      all data must be on disk before the close.
**   
** Inputs:
**      f                    Pointer to the DI file
**                           context needed to do I/O.
** Outputs:
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**    Returns:
**          OK
**          DI_CLOSE         Close operation failed.
**          DI_BADFILE       Bad file context.
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    11-sep-85 (jennifer)
**          Created new for 5.0.
**    20-mar-86 (jennifer)
**          Changed longnat to nat and err_code to CL_ERR_DESC to meet 
**          CL coding standard.
**     8-dec-88 (rogerk)
**	    Check that we still own the file we are about to deaccess.  Look
**	    for problem where we close somebody else's file channel.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**	10-jan-91 (rachael)
**	    Return DI_BADFILE if file control block's io_type does not
**	    contain DI_IO_ASCII_ID.
**	17-aug-93 (walt)
**	    Expand itmlst to be an array in order to terminate the list with
**	    a set of zeros.  This silences an SS$_BADPARAM error return from
**	    sys$getdvi.
*/
STATUS
DIclose(
DI_IO          *f,
CL_ERR_DESC     *err_code)
{
    int           s;                    /* Request return status. */
    IOSB	  local_iosb;           /* Operation return status. */
#ifdef xDEBUG
    long	  device_class;
    long	  dvi_length;
    ILE3  	  itmlst[2] = { {4, DVI$_DEVCLASS, &device_class, &dvi_length},
				{0, 0, 0, 0} };
#endif

    CL_CLEAR_ERR(err_code);
   
    /* Check file control block pointer, return if bad.*/

    if (f->io_type == DI_IO_ASCII_ID) 
    {
#ifdef xDEBUG
	/*
	** Check that the channel we are deallocating is still connected
	** to a disk device.  This will trap the errors where we are
	** conflicting with GCF on IO channels.
	*/
	s = sys$getdviw(EFN$C_ENF, f->io_channel, 0, &itmlst, &local_iosb, 0, 0, 0);
	if (s & 1)
	{
	    s = local_iosb.iosb$w_status;
	}

	if (((s & 1) == 0) || (device_class != DC$_DISK))
	{
	    /*
	    ** This io channel is no longer any good, or is not what we
	    ** think it is.  Return BADFILE and either the system error
	    ** (if one was returned), or SS$_NOTFILEDEV.
	    */
	    err_code->error = s;
	    if (s & 1)
		err_code->error = SS$_NOTFILEDEV;
	    return (DI_BADFILE);
	}
#endif

	s = DI_qio("DI!DI.C::DIclose_1",
                   f->io_channel, IO$_DEACCESS, &local_iosb,
		0, 0, 0, 0, 0, 0);

	if (s & 1)
	{
	    if (local_iosb.iosb$w_status & 1)
	    {
		s = sys$dassgn(f->io_channel);
		if (s & 1)
		{
		    f->io_channel = 0;
		    f->io_type = 0;
		    err_code->error = OK;
		    return (OK);
		}
	    }
	    else
	    {
 		s = local_iosb.iosb$w_status;
	    }
	}
    }
    else
    {
	err_code->error = 0;
	return (DI_BADFILE);
    }

    err_code->error = s;
    return (DI_BADCLOSE);
}

/*{
** Name: DIcreate - Creates a new file.
**
** Description:
**      The DIcreate routine is used to create a direct access file.  
**      Space does not need to be allocated at time of creation. 
**      If it is more efficient for a host operating system to
**      allocate space at creation time, then space can be allocated.
**      The size of the page for the file is fixed at create time
**      and cannot be changed at open.
**      
**   
** Inputs:
**      f                    Pointer to the DI file
**                           context needed to do I/O.
**      path                 Pointer to an area containing path name.
**      pathlength           Length of pathname.
**      filename             Pointer to an area containing file name.
**      filelength           Length of file name.
**      pagesize             Value indicating the size of the page
**                           in bytes.
**      
** Outputs:
**      f                    Updates the file control block.
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**    Returns:
**          OK
**          DI_BADDIR        Error in path specification.
**          DI_BADCREATE     Error creating file.
**          DI_BADFILE       Bad file context.
**          DI_BADPARAM      Parameter(s) in error.
**          DI_EXCEED_LIMIT  Too many open files.
**          DI_EXISTS        File already exists.
**          DI_DIRNOTFOUND   Path not found.
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** Design Details:
**      On VMS it will not allocate any space at the time of create
**      and it does not leave the file open.   
** History:
**    11-sep-85 (jennifer)
**          Created new for 5.0.
**    20-mar-86 (jennifer)
**          Changed longnat to nat and err_code to CL_ERR_DESC to meet 
**          CL coding standard.
**      06-nov-86 (jennifer)
**          Added error returns for DIcreate and DIdircreate.
**    23-Feb-87 (ac)
**	    Retry the getpath if getpath returns a match but 
**	    still fail to create the file. 
**    05-sep-89 (jennifer)
**          For B1 server must own all files, not the user.
**	  02-dec-92 (walt)
**		Created writable copy of the filename input parameter for Alpha because
**		sys$qio was returning SS$_ACCVIO for status when the string location
**		containing the file name wasn't writable.  Copy the file name into
**		writable_fname[] and put that address into the file descriptor rather
**		than the address in the filename input parameter.
*/
STATUS
DIcreate(
DI_IO          *f,
char           *path,
u_i4           pathlength,
char           *filename,
u_i4           filelength,
i4             pagesize,
CL_ERR_DESC	*err_code)
{
#ifndef xDEBUG
    i4              io_pro = 0xff00;   /* S:RWED, O:RWED. */
#else
    i4		    io_pro = 0x0fff;   /* S:REWD, O:RWED, G:RWED. */
#endif
    i4		     uic;                  /* User code for ownership. */
    FAT		     io_rec;
    FIBDEF	     *fib;
                                        /* RMS record information */
    ATRDEF	    io_att[] =
                           {   
                                { ATR$S_RECATTR, ATR$C_RECATTR, (char *) &io_rec},
                                { ATR$S_FPRO, ATR$C_FPRO, (char *)&io_pro },
                                { ATR$S_UIC, ATR$C_UIC, (char *)&uic },
                                { 0, 0, 0}
                           };           /*    attr fetch information    */
    int		    s;                    /* Request return status. */
    IOSB	    local_iosb;           /* Operation return status. */ 
    CS_SID		    sid;		  /* session identifier */
    struct
    {
	i4	    length;
	char	    *data;
    }		file_desc;            /* File descriptor. */
    struct
    {
	i4	    length;
	char	    *data;
    }		io_device;            /* Device descriptor. */
    PATH_ENTRY	    path_entry;
    ILE3	    itmlst[2] = {{4, JPI$_UIC, &uic, 0},{0, 0, 0, 0}};
    IOSB	    iosb;
    i4	    retry;
	char	writable_fname[DI_FILENAME_MAX+1];

    /*	Check for some bad parameters. */

    CL_CLEAR_ERR(err_code);
   
    if (pathlength == 0 || pathlength > DI_PATH_MAX || filelength > DI_FILENAME_MAX)
	return (DI_BADPARAM);		
    fib = (FIBDEF *)f->io_fib;

	/*	Create a writable copy of the file name  */

	STlcopy(filename, writable_fname, filelength);

    CSget_sid(&sid);

    /* Initialize file control block. */

    MEfill (sizeof(DI_IO), 0, (char *)f);

#ifdef xORANGE

	sys$getjpiw(EFN$C_ENF, 0, 0, &itmlst, &iosb, 0, 0);
#else

    /* Get user id information. */

    uic = CSvms_uic();

    /* If CS uic available, use it; if not, use current uic */

    if (uic == 0)
	sys$getjpiw(EFN$C_ENF, 0, 0, &itmlst, &iosb, 0, 0);

#endif
    for (retry = 0; retry < 2; retry++)
    {
	/* Get the internal path identifier. */

	s = getpath(path, pathlength, &path_entry, retry, sid, err_code);
	if ((s & 1) == 0)
	    break;

	/* Assign a channel for I/O. */

	io_device.length = path_entry.c_devnam_cnt;
	io_device.data = path_entry.c_device;
	s = sys$assign(&io_device, &f->io_channel, 0, 0);
	if ((s & 1) == 0)
	    continue;

	/* Try to open file, should get error, should not exist. */

	fib->fib$l_acctl =  FIB$M_NOLOCK; 
	fib->fib$l_acctl |=  FIB$M_WRITE; 
	f->io_fib_desc.str_len = sizeof(f->io_fib);
	f->io_fib_desc.str_str = (char *)fib;

	/*  Use writable copy of file name.  */
	file_desc.data = writable_fname;

	file_desc.length = filelength;
	fib->fib$w_did[0] = path_entry.c_io_direct[0];
	fib->fib$w_did[1] = path_entry.c_io_direct[1];
	fib->fib$w_did[2] = path_entry.c_io_direct[2];
	s = DI_qio("DI!DI.C::DIcreate_1",
                   f->io_channel, IO$_ACCESS, &local_iosb,
                (i4) &f->io_fib_desc, (i4) &file_desc, 0, 0, (i4)io_att, 0);

	if (s & 1)
	{
	    if (local_iosb.iosb$w_status & 1)
	    {
		sys$dassgn(f->io_channel);
		err_code->error = s;
		return (DI_EXISTS);
	    }
	}

	/* Create the file with no locking.*/

	MEfill(sizeof(FAT), 0, (char *)&io_rec);
	io_rec.fat$v_rtype = FAT$C_FIXED;
	io_rec.fat$w_rsize = pagesize;
	fib->fib$l_acctl = (FIB$M_WRITETHRU | FIB$M_NOLOCK | FIB$M_WRITE);
	s = DI_qio("DI!DI.C::DIcreate_2",
                   f->io_channel, IO$_CREATE | IO$M_CREATE, &local_iosb,
                (i4) &f->io_fib_desc, (i4) &file_desc, 0, 0, (i4) io_att, 0);
	if (s & 1)
	{
	    if (local_iosb.iosb$w_status & 1)
	    {
		sys$dassgn(f->io_channel);
		err_code->error = OK;
		return (OK);
	    }
	    s = local_iosb.iosb$w_status;
	}

	sys$dassgn(f->io_channel);
	continue;
    }

    /* Always close the file and mark file control block as not valid. */

    err_code->error = s;
    if (s == SS$_DUPFILENAME)
	return (DI_EXISTS);
    if ((s == SS$_EXQUOTA) || (s == SS$_EXDISKQUOTA) || (s == SS$_NOIOCHAN))
	return (DI_EXCEED_LIMIT);
    if (s == RMS$_DNF)
	return (DI_DIRNOTFOUND);
    if (s == RMS$_DEV)
	return (DI_BADDIR);
    return (DI_BADCREATE);
}

/*{
** Name: DIdelete - Deletes a file.
**
** Description:
**      The DIdelete routine is used to delete a direct access file.
**      The file does not have to be open to delete it.  If the file is
**      open (by another server/user) it will be deleted when the 
**      last user perfroms a DIclose.  This caller should not be
**      calling delete if the caller has not closed the file.  If 
**      this can be detected, an error can be returned.
**
** Inputs:
**      notused              NULL or pointer to DI_IO structure; not used
**      path                 Pointer to the area containing 
**                           the path name for this file.
**      pathlength           Length of path name.
**      filename             Pointer to the area containing
**                           the file name.
**      filelength           Length of file name.
** Outputs:
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**    Returns:
**        OK
**        DI_BADFILE         Bad file context.
**        DI_BADOPEN         Error openning file.
**        DI_DIRNOTFOUND     Path not found.
**        DI_BADDELETE       Error deleting file.
**        DI_BADPARAM        Parameter(s) in error.
**	  DI_EXCEED_LIMIT    Open file limit exceeded.
**        DI_FILENOTFOUND    File not found.
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    11-sep-85 (jennifer)
**          Created new for 5.0.
**    20-mar-86 (jennifer)
**          Changed longnat to nat and err_code to CL_ERR_DESC to meet 
**          CL coding standard.
**	  02-dec-92 (walt)
**		Created writable copy of the filename input parameter for Alpha because
**		sys$qio was returning SS$_ACCVIO for status when the string location
**		containing the file name wasn't writable.  Copy the file name into
**		writable_fname[] and put that address into the file descriptor rather
**		than the address in the filename input parameter.
**	7-Mar-2007 (kschendel) SIR 122512
**	    Caller can pass NULL di-io as a signal for unix, so use a local
**	    di-io as a work area.
*/
STATUS
DIdelete(
DI_IO          *notused,
char           *path,
u_i4          pathlength,
char           *filename,
u_i4          filelength,
CL_ERR_DESC     *err_code)
{
    DI_IO	    local_diio;
    DI_IO	    *f;
    i4		    s;                    /* Request return status. */
    IOSB	    local_iosb;           /* Operation return status. */
    CS_SID	    sid;			/* session identifier */
    PATH_ENTRY	    path_entry;
    FIBDEF	    *fib;
    struct
    {
	int	    length;
	char	    *data;
    }		    file_desc;            /* File descriptor for delete. */
    struct
    {
	int	    length;
	char	    *data;
    }		    io_device;            /* File descriptor for directory. */
    u_i4	    length;
    i4	    retry;

	char	writable_fname[DI_FILENAME_MAX+1];

    CL_CLEAR_ERR(err_code);
   
    /* Adjust length for directory name when deleting a directory
    ** instead of a file. 
    */

    length = filelength;
    if ((length > 6) && 
           (MEcmp(&filename[filelength-6], ".dir.1", (u_i2)6) == 0))
    {
	length = filelength - 6;
    }
    if (pathlength == 0 || pathlength > DI_PATH_MAX || length > DI_FILENAME_MAX)
	return (DI_BADPARAM);
    /* Don't depend on caller for a DI_IO, use a local one.  There's nothing
    ** we need it for anyway, other than system call workspace.
    */
    f = &local_diio;
    fib = (FIBDEF *)f->io_fib;
    CSget_sid(&sid);

	/*	Create a writable copy of the file name  */

	STlcopy(filename, writable_fname, filelength);

    /*	Perform operation twice to cache invalidated path entries. */

    for (retry = 0; retry < 2; retry++)
    {
	/*  Get device and directory information. */

	s = getpath(path, pathlength, &path_entry, retry, sid, err_code);
	if ((s & 1) == 0)
	    break;

	/* Assign a channel to this file. */
	
	MEfill(sizeof(DI_IO), 0, (char *)f);
	f->io_fib_desc.str_len = sizeof(f->io_fib);
	f->io_fib_desc.str_str = (char *)fib;
	io_device.length = path_entry.c_devnam_cnt;
	io_device.data = path_entry.c_device;
	s = sys$assign(&io_device, &f->io_channel, 0, 0);
	if ((s & 1 ) == 0)
	    continue;	

	/*
	** First open the file with nolock 
	*/

	fib->fib$l_acctl = (FIB$M_WRITE | FIB$M_NOLOCK);
	file_desc.length = filelength;

	/*  Use writable copy of file name.  */
	file_desc.data = writable_fname;

	fib->fib$w_did[0] = path_entry.c_io_direct[0];
	fib->fib$w_did[1] = path_entry.c_io_direct[1];
	fib->fib$w_did[2] = path_entry.c_io_direct[2];
	s = DI_qio("DI!DI.C::DIdelete_1",
                    f->io_channel, IO$_ACCESS | IO$M_ACCESS, &local_iosb,
		     (i4) &f->io_fib_desc, (i4) &file_desc, 0, 0, 0, 0);
	if (s & 1)
	{
	    if (local_iosb.iosb$w_status & 1)
	    {
		/*
		** Now try and delete the file
		*/

		s = DI_qio("DI!DI.C::DIdelete_2",
                           f->io_channel, IO$_DELETE | IO$M_DELETE, &local_iosb,
		     (i4) &f->io_fib_desc, (i4) &file_desc, 0, 0, 0, 0);

		if (s & 1)
		{
		    if (local_iosb.iosb$w_status & 1)
		    {
			err_code->error = OK;
			sys$dassgn(f->io_channel);
			return (OK);
		    }
		}
	    }
	    s = local_iosb.iosb$w_status;
	}
	continue;
    }

    err_code->error = s;
    if (f->io_channel)
	sys$dassgn(f->io_channel);
    if ((s == SS$_EXQUOTA) || (s == SS$_EXDISKQUOTA) || (s == SS$_NOIOCHAN))
	return (DI_EXCEED_LIMIT);
    if (s == SS$_NOSUCHFILE)
	return (DI_FILENOTFOUND);
    if (s == RMS$_DNF)
	return (DI_DIRNOTFOUND);
    if (s == RMS$_DEV)
	return (DI_BADDIR);
    return (DI_BADDELETE);    
}

/*{
** Name: DIdircreate - Creates a directory.
**
** Description:
**      The DIdircreate will take a path name and directory name 
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
**    11-sep-85 (jennifer)
**          Created new for 5.0.
**    20-mar-86 (jennifer)
**          Changed longnat to nat and err_code to CL_ERR_DESC to meet 
**          CL coding standard.
**      06-nov-86 (jennifer)
**          Added error returns for DIcreate and DIdircreate.
**	22-jul-1992 (bryanp)
**	    If sys$parse returns RMS$_CHN, treat this as SS$_NOIOCHAN; that is,
**	    return DI_EXCEED_LIMIT to our caller.
*/
STATUS
DIdircreate(
DI_IO          *f,
char           *path,
u_i4           pathlength,
char           *dirname,
u_i4           dirlength,
CL_ERR_DESC     *err_code)
{
    i4	          s;                    /* Request return status. */
    i4            full_length;          /* Length of a fully expanded
                                        ** directory name. */
    i4            prot;                 /* protection value for create. */
    i4		  uic;			/* uic for directory file ownership */
    CS_SID	  sid;			/* session identification */

    struct dsc$descriptor_s
                  create_desc;          /* Creation descriptor. */
    char          full_name[NAM$C_MAXRSS];
					/* Area to hold a fully 
                                        ** expanded directory name. */
    char          esa[NAM$C_MAXRSS];    /* Device name. */
    struct FAB    fab;                  /* File access block. */
    struct NAM    nam;                  /* File name for access. */
    ILE3	  itmlst[2] = {{4, JPI$_UIC, &uic, 0},{0, 0, 0, 0}};
					/* uic of current user */
    IOSB	  iosb;
    
    CL_CLEAR_ERR(err_code);
   
    /* Check input parameters. */

    if (pathlength == 0 || pathlength > DI_PATH_MAX ||
	dirlength == 0 || dirlength > DI_FILENAME_MAX)
    {
	return (DI_BADPARAM);
    }

#ifdef DI_CHECK_RESUME

    if (DI_check_resume)
    {
       DI_check_init();
    }
#endif

    CSget_sid(&sid);

    /* Check if path exists. */

    MEfill(sizeof(fab), 0, (char *)&fab);
    MEfill(sizeof(nam), 0, (char *)&nam);
    fab.fab$l_nam = &nam;
    fab.fab$l_fna = path;
    fab.fab$b_fns = pathlength;
    fab.fab$l_ctx = sid;
    nam.nam$l_esa = esa;
    nam.nam$b_ess = sizeof(esa);
    fab.fab$b_bid = FAB$C_BID;
    fab.fab$b_bln = FAB$C_BLN;
    nam.nam$b_bid = NAM$C_BID;
    nam.nam$b_bln = NAM$C_BLN;
    s = rms_suspend(sys$parse(&fab, resume_fcn, resume_fcn), &fab);
    if ((s & 1) == 0)
    {
	/*
	**  If RMS gets SS$_NOIOCHAN error it returns a incomplete
	**  status code of 0x00020000. (i.e. RMS-W-NORMAL).  Check
	**  for this and return DI_EXCEED_LIMIT.
	*/

	if (s == RMS$_CHN || (s & 0xffff) == 0)
	{
	    err_code->error = SS$_NOIOCHAN;
	    return (DI_EXCEED_LIMIT);
	}
	err_code->error = s;
	if (s == RMS$_DNF)
	    return (DI_DIRNOTFOUND);
	return (DI_BADDIR);
    }
	
    /*	Release RMS resources allocated by the last call. */

    nam.nam$b_nop = NAM$M_SYNCHK;
    rms_suspend(sys$parse(&fab, resume_fcn, resume_fcn), &fab);

    /* 
    ** Create the full directory name from the path name and 
    ** the directory name.  The path name must have one of
    ** the following formats:
    ** 
    **    devname:[name1.name2. ... namex]
    **    devname:<name1.name2. ... namex>
    **
    ** Combined with the dirname the fullname must look like:
    **
    **    devname:[name1.name2. ... namex.dirname]
    **    devname:<name1.name2. ... namex.dirname>
    */

    MEcopy(path, pathlength, full_name);
    MEcopy(dirname, dirlength, &full_name[pathlength]);  
    full_name[pathlength+dirlength] = full_name[pathlength-1];
    full_name[pathlength-1] = '.';             
    full_length = pathlength + dirlength + 1;

    /* Check if directory exists. */

    MEfill(sizeof(fab), 0, (char *)&fab);
    MEfill(sizeof(nam), 0, (char *)&nam);
    fab.fab$l_nam = &nam;
    fab.fab$l_ctx = sid;
    nam.nam$l_esa = esa;
    nam.nam$b_ess = sizeof(esa);
    fab.fab$b_bid = FAB$C_BID;
    fab.fab$b_bln = FAB$C_BLN;
    nam.nam$b_bid = NAM$C_BID;
    nam.nam$b_bln = NAM$C_BLN;
    fab.fab$l_fna = full_name;
    fab.fab$b_fns = full_length;

    s = rms_suspend(sys$parse(&fab, resume_fcn, resume_fcn), &fab);
    if ((s & 1) == 0)
    {
	/*
	**  If RMS gets SS$_NOIOCHAN error it returns a incomplete
	**  status code of 0x00020000. (i.e. RMS-W-NORMAL).  Check
	**  for this and return DI_EXCEED_LIMIT.
	*/

	if (s == RMS$_CHN || (s & 0xffff) == 0)
	{
	    err_code->error = SS$_NOIOCHAN;
	    return (DI_EXCEED_LIMIT);
	}
	err_code->error = s;
	if (s != RMS$_DNF)
	    return (DI_BADDIR);
    }
    else
    {    
	/*  Release RMS resources allocated by last call. */

	nam.nam$b_nop = NAM$M_SYNCHK;
	rms_suspend(sys$parse(&fab, resume_fcn, resume_fcn), &fab);
        err_code->error = OK;
        return (DI_EXISTS);
    }

    /* 
    ** Set up the input descriptor for the operating system
    ** call.
    */   

    create_desc.dsc$b_class = DSC$K_CLASS_S;
    create_desc.dsc$b_dtype = DSC$K_DTYPE_T;
    create_desc.dsc$w_length = full_length;
    create_desc.dsc$a_pointer = full_name;
    prot = 0xff;

#ifdef xORANGE
	sys$getjpiw(EFN$C_ENF, 0, 0, &itmlst, &iosb, 0, 0);
#else

    /* get uic of current user for directory ownership */
    
    uic = CSvms_uic();

    /* If CS info available use it; if not, use current uic */

    if (uic == 0)
	sys$getjpiw(EFN$C_ENF, 0, 0, &itmlst, &iosb, 0, 0);
    
#endif
    s = lib$create_dir(&create_desc, &uic, &prot, 0, 0, 0);
    if (s & 1)
    {
	err_code->error = OK;
	return (OK);
    }

    err_code->error = s;
    if ((s == SS$_EXQUOTA) || (s == SS$_EXDISKQUOTA) || (s == SS$_NOIOCHAN))
	return(DI_EXCEED_LIMIT);
    return (DI_BADDIR);
}

/*{
** Name: DIdirdelete  - Deletes a directory.
**
** Description:
**      The DIdirdelete routine is used to delete a direct access 
**      directory.  The name of the directory to delete should
**      be specified as path and dirname, where dirname does not
**      include a type qualifier or version and the path does
**	not include the directory name (which it normally does for
**	other DI calls).  The directory name may be  converted to a filename
**	of the directory and use DIdelete to delete it.
**      This call should fail with DI_BADDELETE if any files still exists
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
**	  DI_DIRNOTFOUND     Path not found.
**	  DI_FILENOTFOUND    File not found.
**        DI_BADPARAM        Parameter(s) in error.
**        DI_BADDELETE       Error trying to delete directory.
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    11-sep-85 (jennifer)
**          Created new for 5.0.
**    20-mar-86 (jennifer)
**          Changed longnat to nat and err_code to CL_ERR_DESC to meet 
**          CL coding standard.
**    17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    10-apr-91 (teresa)
**		Fixed assorted bugs where it was possible to overflow
**		local buffer "filename" if the dirname was 23 or 24 characters
**		long.  The DI_FILENAME_MAX constant is defined at 28, which is
**		inadequate for a filename of DB_MAXNAME (currently 24) and
**		the 6 characters that are appended to the filename.
*/
STATUS
DIdirdelete(
DI_IO          *f,
char           *path,
u_i4          pathlength,
char           *dirname,
u_i4          dirlength,
CL_ERR_DESC     *err_code)
{
    i4	          s;                    /* Request return status. */
    i4            l_filename;		/* Length of file name. */
    i4		  l_pathname;		/* Length of path name. */
    i4		  i;
    char          filename[DI_FILENAME_MAX + sizeof(DI_DIREXTENSION)];
    char	  pathname[DI_PATH_MAX];
                                        /* Area to contain file name
                                        ** of directory. */

    CL_CLEAR_ERR(err_code);
   
    /* Check input parameters. */

    if (pathlength == 0 || pathlength > DI_PATH_MAX ||
	dirlength > DI_FILENAME_MAX)
    {
	return (DI_BADPARAM);		
    }

    /* 
    ** Create a filename from directory name, this involves
    ** adding the ".dir" qualifier and ".1" to indicate a
    ** directory.  All directories must be version 1.
    ** If no directory name is passed in, delete the last
    ** directory in the path.
    */

    if (dirlength == 0)
    {
	/*  Find directory to be delete. */

	for (i = pathlength; --i >= 0; )
	    if (path[i] == '.' || path[i] == '[')
		break;
	if (path[i] != '.')
	    return (DI_BADOPEN);
	l_pathname = i + 1;
	MEcopy(path, l_pathname, pathname);
	pathname[i] = ']';
	l_filename = pathlength -i - 2;
	MEcopy(&path[l_pathname], l_filename, filename);
    }
    else
    {
	l_pathname = pathlength;
	l_filename = dirlength;
	MEcopy(dirname, l_filename, filename);
	MEcopy(path, l_pathname, pathname);
    }
    MEcopy (DI_DIREXTENSION, sizeof(DI_DIREXTENSION)-1, &filename[l_filename]);
    l_filename += sizeof(DI_DIREXTENSION)-1;

    /* Call DIdelete to delete the directory which is just a file. */

    s = DIdelete(f, pathname, l_pathname, filename, l_filename, err_code);
    return (s);
}

/*{
** Name: DIflush - Flushes a file.
**
** Description:
**      The DIflush routine is used to write the file header to disk.
**      The file header contains the end of file and allocated information.
**      This routine maybe a null operation on systems that can't
**      support it.
**      
**   
** Inputs:
**      f                    Pointer to the DI file
**                           context needed to do I/O.
**      
** Outputs:
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**    Returns:
**          OK
**          DI_BADFILE       Bad file context.
**          DI_BADEXTEND     Error flushing file header.
**    Exceptions: 
**        none
**
** Side Effects:
**        none
**
** History:
**    11-sep-85 (jennifer)
**          Created new for 5.0.
**    20-mar-86 (jennifer)
**          Changed longnat to nat and err_code to CL_ERR_DESC to meet 
**          CL coding standard.
*/
STATUS
DIflush(
DI_IO          *f,
CL_ERR_DESC     *err_code)
{
    int           s;                    /* Request return status. */
    IOSB	  local_iosb;           /* Operation return status. */ 
    i4            eof;                  /* Variable used to calculate
                                        ** eof. */
    FIBDEF	    *fib;
    FAT             io_rec;
                                        /* RMS record information */
    ATRDEF           io_att[] =
                           {   
                                { ATR$S_RECATTR, ATR$C_RECATTR, (char *)&io_rec},
                                { 0, 0, 0}
                           };           /*    attr fetch information    */

    CL_CLEAR_ERR(err_code);
   
    /* Check file control block pointer, return if bad.*/

    if (f->io_type != DI_IO_ASCII_ID) 
        return (DI_BADFILE);
    fib = (FIBDEF *)f->io_fib;

    /* 
    ** Now write end of file (used) and allocated to disk. 
    ** Used is all that was requested, Allocated is what was
    ** actually obtained which is always greater than or equal
    ** to the amount requested. 
    */

    MEfill(sizeof(FAT), 0, (char *)&io_rec);
    io_rec.fat$v_rtype = FAT$C_FIXED;
    io_rec.fat$w_rsize = f->io_block_size;
    io_rec.fat$w_efblkh =
	((F11_JUNK *)&f->io_system_eof)->normal.normal_high;
    io_rec.fat$w_efblkl =
	((F11_JUNK *)&f->io_system_eof)->normal.normal_low;

    fib->fib$l_acctl = FIB$M_WRITETHRU;
    fib->fib$w_exctl = 0;
    s = DI_qio("DI!DI.C::DIflush_1",
               f->io_channel, IO$_MODIFY, &local_iosb,
                 (i4) &f->io_fib_desc, 0, 0, 0, (i4) io_att, 0);

    if (s & 1)
    {
	if (local_iosb.iosb$w_status & 1)
	{
	    err_code->error = OK;    
	    return (OK);
	}
	s = local_iosb.iosb$w_status;
    }

    err_code->error = s;
    return (DI_BADEXTEND);
}

/*{
** Name: DIforce - Forces all pages to the disk.
**
** Description:
**      The DIforce routine is used to force all pages held by an operating system
**      to disk.  This is not necessary on VMS so this routine will just return.
**      This routine should wait for completion of all I/O to insure all pages are 
**      correctly on disk.  If an error occurs it should return DI_BADWRITE.
**   
** Inputs:
**      f                    Pointer to the DI file
**                           context needed to do I/O.
**      
** Outputs:
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**    Returns:
**          OK
**          DI_BADFILE       Bad file context.
**          DI_BADWRITE      Error forcing pages to disk.
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    11-sep-85 (jennifer)
**          Created new for 5.0.
**    20-mar-86 (jennifer)
**          Changed longnat to nat and err_code to CL_ERR_DESC to meet 
**          CL coding standard.
**    18-apr-94 (jnash)
**          Just add comment here that Unix fsync project turns
**	    DI_USER_SYNC_MASK DIopen requests into DI_SYNC_MASK opens, but
**	    does nothing about removing DIforce calls that are executed 
**	    by mainline.
*/
STATUS
DIforce(
DI_IO          *f,
CL_ERR_DESC     *err_code)
{
    CL_CLEAR_ERR(err_code);
    return (OK);
}

/*{
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
**(
**          STATUS
**          funct_name(arg_list, dirname, dirlength, err_code)
**          PTR        arg_list;       Pointer to argument list. 
**          char       *dirname;        Pointer to directory name.
**          i4        dirlength;       Length of directory name.
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
**    11-sep-85 (jennifer)
**          Created new for 5.0.
**    20-mar-86 (jennifer)
**          Changed longnat to nat and err_code to CL_ERR_DESC to meet 
**          CL coding standard.
**    05-nov-1987 (rogerk)
**	    When the user function returns something other than OK, return
**	    DI_ENDFILE.  Also, return DI_BADLIST instead of DI_BADDIR when
**	    listing of files in directory fails.  Also, allocate buffer
**	    larger than DI_FILENAME_MAX to put listed directory names into.
**	    Just becuase DI limits the filename length of files doesn't mean
**	    there isn't something bigger than that in the directory.
**	26-apr-1993 (bryanp)
**	    arg_list is a PTR, not a (PTR *).
*/
STATUS
DIlistdir(
DI_IO          *f,
char           *path,
u_i4           pathlength,
STATUS         (*func)(),
PTR            arg_list,
CL_ERR_DESC     *err_code)
{
    i4	          s;                    /* Request return status. */
    i4            fulllength;
    i4	          dirlength;		/* Length of directory name. */                  
    bool	  done;		
    struct FAB    fab;                  /* File access block. */
    struct NAM    nam;                  /* File name for access. */
    char          fullname[NAM$C_MAXRSS];
                                        /* Full directory name plus
                                        ** search qualification, e.g.
                                        ** dev1:[dir1.dir2]*.dir.1 */
    char          esa[NAM$C_MAXRSS];    /* Area to return  full 
                                        ** directory name. */
    char          rsa[NAM$C_MAXRSS];    /* Area to return file names
                                        ** on search. */

    CL_CLEAR_ERR(err_code);
   
    /* Check input parameters. */

    if (pathlength == 0 || pathlength > DI_PATH_MAX)
	return (DI_BADPARAM);		

    /* Build a file access block to locate directory. */

    MEfill(sizeof(fab), 0, (char *)&fab);
    MEfill(sizeof(nam), 0, (char *)&nam);
    MEcopy(path, pathlength, fullname);
    MEcopy("*.dir.1",7,&fullname[pathlength]);
    fulllength = pathlength + 7;
    fab.fab$b_fns = fulllength;
    fab.fab$l_fna = fullname;
    fab.fab$l_nam = &nam;
    CSget_sid((CS_SID *)&fab.fab$l_ctx);
    nam.nam$l_esa = esa;
    nam.nam$l_rsa = rsa;
    nam.nam$b_rss = sizeof(rsa);
    nam.nam$b_ess = sizeof(esa);
    fab.fab$b_bid = FAB$C_BID;
    fab.fab$b_bln = FAB$C_BLN;
    nam.nam$b_bid = NAM$C_BID;
    nam.nam$b_bln = NAM$C_BLN;
    return (list_files(&fab, func, arg_list, 0, err_code));
}

/*{
** Name: DIlistfile - List all files in a directory.
**
** Description:
**      The DIlistfile routine will list all files that exist
**      in the directory(path) specified.  This routine expects
**      as input a function to call for each file found.  This
**      insures that all resources tied up by this routine will 
**      eventually be freed.  The files are not listed in any
**      specific order. The function passed to this routine
**      must return OK if it wants to continue with more files,
**      or a value not equal to OK if it wants to stop listing.
**	If the function returns anything by OK, then DIlistfile
**	will return DI_ENDFILE.
**      The function must have the following call format:
**(
**          STATUS
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
** History:
**    11-sep-85 (jennifer)
**          Created new for 5.0.
**    20-mar-86 (jennifer)
**          Changed longnat to nat and err_code to CL_ERR_DESC to meet 
**          CL coding standard.
**    10-mar-1987 (Derek)
**	    Make sure that the filename is returned in lowercase.
**    05-nov-1987 (rogerk)
**	    When the user function returns something other than OK, return
**	    DI_ENDFILE.  Also, return DI_BADLIST instead of DI_BADDIR when
**	    listing of files in directory fails.  Also, allocate buffer
**	    larger than DI_FILENAME_MAX to put listed file names into.
**	    Just becuase DI limits the filename length of files doesn't mean
**	    there isn't something bigger than that in the directory.
**	26-apr-1993 (bryanp)
**	    arg_list is a PTR, not a (PTR *).
*/
STATUS
DIlistfile(
DI_IO          *f,
char           *path,
u_i4           pathlength,
STATUS         (*func)(),
PTR            arg_list,
CL_ERR_DESC     *err_code)
{
    i4	          s;                    /* Request return status. */
    i4	         filelength;		/* Length of directory name. */                  
    bool	 done;
    i4            fulllength;
    struct FAB    fab;                  /* File access block. */
    struct NAM    nam;                  /* File name for access. */
    char          fullname[NAM$C_MAXRSS];
                                        /* Full directory name plus
                                        ** search qualification, e.g.
                                        ** dev1:[dir1.dir2]*.dir.1 */
    char          esa[NAM$C_MAXRSS];    /* Area to return  full 
                                        ** directory name. */
    char          rsa[NAM$C_MAXRSS];    /* Area to return file names
                                        ** on search. */

    CL_CLEAR_ERR(err_code);
   
    /* Check input parameters. */

    if (pathlength == 0 || pathlength > DI_PATH_MAX)
	return (DI_BADPARAM);		

    /* Build a file access block to locate directory. */

    MEfill(sizeof(fab), 0, (char *)&fab);
    MEfill(sizeof(nam), 0, (char *)&nam);
    MEcopy(path, pathlength, fullname);
    MEcopy("*.*.*", 5, &fullname[pathlength]);
    fulllength = pathlength + 5;
    fab.fab$l_nam = &nam;
    fab.fab$b_fns = fulllength;
    fab.fab$l_fna = fullname;
    CSget_sid((CS_SID *)&fab.fab$l_ctx);
    nam.nam$l_rsa = rsa;
    nam.nam$l_esa = esa;
    nam.nam$b_rss = sizeof(rsa);
    nam.nam$b_ess = sizeof(esa);
    fab.fab$b_bid = FAB$C_BID;
    fab.fab$b_bln = FAB$C_BLN;
    nam.nam$b_bid = NAM$C_BID;
    nam.nam$b_bln = NAM$C_BLN;
    return (list_files(&fab, func, arg_list, 1, err_code));
}

/*{
** Name: DIopen - Opens a file.
**
** Description:
**      The DIopen routine is used to open a direct access file.  
**      It will open the file for write access and will
**      place no locks on the file.  The file is always openned
**      for write even if it is only going to be read.  DIread
**      and DIwrite are not expected to check how the file 
**      was openned.
**      If the path does not exists the error DI_DIRNOTFOUND
**      must be returned before returning DI_FILENOTFOUND.
**
**      If possible the operating system should check that the pages'
** 	size at open is the same as the size at create.  If 
**	the page sizes at open and create time don't match
**	DI_BADPARAM should be returned.
**   
**	The DIopen() command also determines actions performed
**	by DIwrite() on this file.  The "flag" argument contains
**	information which is interpreted by system dependant implementations
**	of database I/O.  (On unix systems there are different options
**	for guaranteeing writes to disk - each of which come with certain
**	performance penalties - so that one might want them enabled in 
**	certain situations (ie. database files), but not in others 
**	(ie. temp files)).
**   
** Inputs:
**      f                    Pointer to the DI file
**                           context needed to do I/O.
**      path                 Pointer to the directory name
**                           for the file.
**      pathlength           Length of path name.
**      filename             Pointer to file name.
**      filelength           Length of file name.
**      pagesize             Value indicating size of page 
**                           in bytes. Must match size at create.
**      mode                 Value indicating the access mode.
**                           This must be DI_IO_READ or DI_IO_WRITE.
**	flags		     Used to alter system specific behaviour of IO
**			     If a unix vendor provides alternate control of
**			     disk I/O though new system calls or flags, it
**			     may be necessary to add to these flags.
**
**				DI_SYNC_MASK	Will use whatever syncing 
**						method is available on the
**						machine.  On unix this currently
**						means O_SYNC or fsync().  On
**						vms this is the default method
**						of writing.
** Outputs:
**      f                    Updates the file control block.
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**    Returns:
**          OK
**          DI_BADDIR        Path specification error.
**          DI_BADOPEN       Error openning file.
**          DI_BADFILE       Bad file context.
**          DI_BADPARAM      Parameter(s) in error.
**          DI_DIRNOTFOUND   Path not found.
**          DI_EXCEED_LIMIT  Open file limit exceeded.
**          DI_FILENOTFOUND  File not found.
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    11-sep-85 (jennifer)
**          Created new for 5.0.
**    20-mar-86 (jennifer)
**          Changed longnat to nat and err_code to CL_ERR_DESC to meet 
**          CL coding standard.
**    23-Feb-87 (ac)
**	    Retry the getpath if getpath returns a match but 
**	    still fail to open the file. 
**    04-aug-87 (rogerk)
**	    When an error occurs, set 's' to the local_iosb status only if
**	    the call to sys$qio was successful.  Otherwise the value is
**	    meaningless.
**    29-sep-87 (rogerk)
**	    Check the err_code parm to getpath to see if the directory
**	    did not exist, rather than the return status.
**    17-oct-88 (mmm)
**	    Added flags argument, which is currently not used on vms.
**    23-dec-1989 (mikem)
**	    Update spec for DIopen() - support for DI_SYNC_MASK which is
**	    no-op on VMS.
**	14-oct-1992 (bryanp)
**	    Support page sizes other than 2K and 4K -- 8, 16, 32K now OK.
**	  02-dec-92 (walt)
**		Created writable copy of the filename input parameter for Alpha because
**		sys$qio was returning SS$_ACCVIO for status when the string location
**		containing the file name wasn't writable.  Copy the file name into
**		writable_fname[] and put that address into the file descriptor rather
**		than the address in the filename input parameter.
**      18-apr-94 (jnash)
**	    Add DI_USER_SYNC_MASK DIopen() mode, which translates into 
**	    DI_SYNC_MASK.  We have chosen not to implement async i/o in 
**	    vms at this time.
**      15-jul-97 (carsu07)
**          Added to the case statement in DIopen() to include a pagesize
**          of 512 and 1024.
**	05-feb-98 (kinte01)
**	    Added missing case statement entry for 64K pagesize
*/
STATUS
DIopen(
DI_IO          *f,
char           *path,
u_i4           pathlength,
char           *filename,
u_i4           filelength,
i4             pagesize,
i4             mode,
u_i4      flags,
CL_ERR_DESC     *err_code)
{
    int		    s;                    /* Request return status. */
    IOSB	    local_iosb;           /* Operation return status. */ 
    struct
    {
	i4	    length;
	char	    *data;
    }		    file_desc;    
    struct
    {
	i4	    length;
	char	    *data;
    }		    io_device;    
    PATH_ENTRY	    path_entry;
    i4		    uic;                  /* User code for ownership. */
    FIBDEF	    *fib;
    FAT		    io_rec;
                                        /* RMS record information */
    ATRDEF	    io_att[] =
                           {   
                                { ATR$S_RECATTR, ATR$C_RECATTR, (char *)&io_rec},
                                { 0, 0, 0}
                           };           /*    attr fetch information    */
    CS_SID	    sid;
    i4	    retry;

	char	writable_fname[DI_FILENAME_MAX+1];

    CL_CLEAR_ERR(err_code);
   

    /* Check input parameters. */

    if (pathlength == 0 || pathlength > DI_PATH_MAX ||
	filelength > DI_FILENAME_MAX ||
       (mode != DI_IO_READ && mode != DI_IO_WRITE))
    {
	return (DI_BADPARAM);		
    }

    /*
    ** The VMS cl does not implement async i/o via DI_USER_SYNC_MASK.
    ** DI_USER_SYNC_MASK requests are turned into normal DI_SYNC_MASK 
    ** requests.  Note also that DIforce() does nothing on VMS.
    */
    if (flags & DI_USER_SYNC_MASK) 
    {
	flags &= ~DI_USER_SYNC_MASK;
	flags |= DI_SYNC_MASK;
    }

	/*	Create a writable copy of the file name  */

	STlcopy(filename, writable_fname, filelength);

    /* Initialize file control block. */

    MEfill (sizeof(DI_IO), 0, (char *)f);
    CSget_sid(&sid);
    fib = (FIBDEF *)f->io_fib;
    for (retry = 0; retry < 2; retry++)
    {
	/* Get the internal path identifier. */

	s = getpath(path, pathlength, &path_entry, retry, sid, err_code);
	if ((s & 1) == 0)
	    break;

	/* Assign a channel for I/O. */

	io_device.length = path_entry.c_devnam_cnt;
	io_device.data = path_entry.c_device;
	s = sys$assign(&io_device, &f->io_channel, 0, 0);
	if ((s & 1) == 0)
	    continue;

	/* Open file with no locks. */

	fib->fib$l_acctl =  FIB$M_NOLOCK; 
	if (mode == DI_IO_WRITE)
	    fib->fib$l_acctl |=  FIB$M_WRITE; 
	f->io_fib_desc.str_len = sizeof(f->io_fib);
	f->io_fib_desc.str_str = (char *)fib;

	/*  Use writable copy of file name.  */
	file_desc.data = writable_fname;

	file_desc.length = filelength;
	fib->fib$w_did[0] = path_entry.c_io_direct[0];
	fib->fib$w_did[1] = path_entry.c_io_direct[1];
	fib->fib$w_did[2] = path_entry.c_io_direct[2];
	s = DI_qio("DI!DI.C::DIopen_1",
                   f->io_channel, IO$_ACCESS | IO$M_ACCESS, &local_iosb,
                (i4) &f->io_fib_desc, (i4) &file_desc, 0, 0, (i4) io_att, 0);
	if (s & 1)
	{
	    if (local_iosb.iosb$w_status & 1)
	    {
		fib->fib$l_acctl = 0;
		fib->fib$w_did[0] = 0;
		fib->fib$w_did[1] = 0;
		fib->fib$w_did[2] = 0;
		f->io_block_size = pagesize;
		switch (pagesize)
		{
		    case 512:
			f->io_blks_log2 = 0;
			f->io_log2_bksz = 9;
			break;
		    case 1024:
			f->io_blks_log2 = 1;
			f->io_log2_bksz = 10;			
			break;
		    case 2048:
			f->io_blks_log2 = 2;
			f->io_log2_bksz = 11;
			break;
		    case 4096:
			f->io_blks_log2 = 3;
			f->io_log2_bksz = 12;
			break;
		    case 8192:
			f->io_blks_log2 = 4;
			f->io_log2_bksz = 13;
			break;
		    case 16384:
			f->io_blks_log2 = 5;
			f->io_log2_bksz = 14;
			break;
		    case 32768:
			f->io_blks_log2 = 6;
			f->io_log2_bksz = 15;
			break;
		    case 65536:
			f->io_blks_log2 = 7;
			f->io_log2_bksz = 16;
			break;
		    default:
			sys$dassgn(f->io_channel);
			return (DI_BADOPEN);
			break;
		}
		f->io_type = DI_IO_ASCII_ID;
		f->io_open_flags = 0;
		if (flags & DI_SYNC_MASK)
		    f->io_open_flags |= DI_O_SYNC_FLAG;
		return (OK);
	    }
	    s = local_iosb.iosb$w_status;
	}
	
	sys$dassgn(f->io_channel);
	continue;
    }

    err_code->error = s;
    if (s == SS$_NOSUCHFILE)
	return (DI_FILENOTFOUND);
    if ((s == SS$_EXQUOTA) || (s == SS$_EXDISKQUOTA) || (s == SS$_NOIOCHAN) ||
	(s == SS$_EXBYTLM))
    {
	return (DI_EXCEED_LIMIT);
    }
    if (s == RMS$_DNF)
	return (DI_DIRNOTFOUND);
    return (DI_BADOPEN);
}

/*{
** Name: DIread - Read a page of a file.
**
** Description:
**      The DIread routine is used to read pages of a direct access 
**      file.   For the large block read option, the number of pages
**      to read is an input parameter to this routine.  It will
**      return the number of pages it read, since at
**      end of file it may read less pages than requested.
**      If multiple page reads are requested, the buffer is assumed
**      to be large enough to hold n pages.  The size of a page is 
**      determined at create.
**      
** Inputs:
**      f                    Pointer to the DI file
**                           context needed to do I/O.
**      n                    Pointer to value of number of pages to read.
**      page                 Value indicating page to begin reading.
**      buf                  Pointer to the area to hold
**                           page(s) being read.
**      
** Outputs:
**      n                    Number of pages read.
**      f                    Updates the file control block.
**      buf                  Pointer to the page read.
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**    Returns:
**          OK
**          DI_BADFILE       Bad file context.
**          DI_BADREAD       Error reading file.
**          DI_BADPARAM      Parameter(s) in error.
**          DI_ENDFILE       Not all blocks read.
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    11-sep-85 (jennifer)
**          Created new for 5.0.
**    20-mar-86 (jennifer)
**          Changed longnat to nat and err_code to CL_ERR_DESC to meet 
**          CL coding standard.
**    17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    10-jan-91 (rachael)
**		Added return of DI_BADFILE if fcb io_type is bad
**	04-Oct-2007 (jonj)
**	    Add read-past-end-of file check, to hopefully preclude
**	    SYSTEM-W-ENDOFFILE errors seen in cluster testing.
*/
STATUS
DIread(
DI_IO	       *f,
i4	       *n,
i4	       page,
char           *buf,
CL_ERR_DESC     *err_code)
{
    i4            last_block_to_read;
    int           s;                    /* Request return status. */
    IOSB	  local_iosb;           /* Operation return status. */ 

    CL_CLEAR_ERR(err_code);
   
    /* Check file control block pointer, return if bad.*/

    if (f->io_type == DI_IO_ASCII_ID) 
    {
	/*
	** Sanity check to make sure we are reading within the bounds of
	** the file. Note: we may still be reading garbage pages--it is
	** up to the upper layers to guarantee that we are not doing this
	*/
	last_block_to_read = (page + *n) << f->io_blks_log2;

	if ( last_block_to_read > f->io_alloc_eof )
	{
	    i4	real_eof;

	    s = DIsense(f, &real_eof, err_code);

	    if ( s || last_block_to_read > f->io_alloc_eof )
	    {
		*n = 0;
		if ( s )
		    return(s);
		TRdisplay("%@ DIread %d: io_alloc_eof: %d io_system_eof: %d\n",
		    __LINE__, f->io_alloc_eof, f->io_system_eof);
		return(DI_ENDFILE);
	    }
	}

	s = DI_qio("DI!DI.C::DIread_1",
                   f->io_channel, IO$_READVBLK, &local_iosb,
                (i4) buf, f->io_block_size * *n, (page << f->io_blks_log2) + 1,
		0, 0, 0);

	if (s & 1)
	{
	    if (local_iosb.iosb$w_status & 1)
	    {
		err_code->error = OK;
		return (OK);
	    }
	    s = local_iosb.iosb$w_status;
	    *n = local_iosb.iosb$l_dev_depend >> f->io_log2_bksz;
	}
	else
	    *n = 0;
    }
    else
    {
	err_code->error = 0;
	return (DI_BADFILE);
    }
    
    err_code->error = s;
    if (s == SS$_ENDOFFILE)
    {
	TRdisplay("%@ DIread %d: io_alloc_eof: %d io_system_eof: %d\n",
	    __LINE__, f->io_alloc_eof, f->io_system_eof);
	return (DI_ENDFILE);
    }
    return (DI_BADREAD);
}

/*{
** Name: DIrename - Renames a file. 
**
** Description:
**      The DIrename will change the name of a file. 
**	The file need not be closed.  The file can be renamed
**      but the path cannot be changed.  A fully qualified
**      filename must be provided for old and new names.
**      This includes the type qualifier extension.
**      The file should be closed before it is renamed.  If 
**      this can be detected, an error should be returned.
**   
** Inputs:
**	di_io_unused	     UNUSED DI_IO pointer (always set to 0 by caller).
**      path                 Pointer to the path name.
**      pathlength           Length of path name.
**      oldfilename          Pointer to old file name.
**      oldlength            Length of old file name.
**      newfilename          Pointer to new file name.
**      newlength            Length of new file name.
** Outputs:
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**    Returns:
**        OK
**        DI_BADRNAME        Any i/o error during rename.
**        DI_BADPARAM        Parameter(s) in error.
**        DI_DIRNOTFOUND     Path not found.
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    11-sep-85 (jennifer)
**          Created new for 5.0.
**    20-mar-86 (jennifer)
**          Changed longnat to nat and err_code to CL_ERR_DESC to meet 
**          CL coding standard.
**    06-nov-86 (jennifer)
**          Add DI_IO construct as an input parameter.
**	15-apr-1992 (bryanp)
**	    Remove DI_IO argument from DIrename(). There is no reason to have
**	    this argument; and it adds complexity to the interface.
*/
STATUS
DIrename(
DI_IO	       *di_io_unused,
char           *path,
u_i4          pathlength,
char           *oldfilename,
u_i4          oldlength,
char           *newfilename,
u_i4          newlength,
CL_ERR_DESC     *err_code)
{
    i4	          s;                    /* Request return status. */
    char          oldesa[NAM$C_MAXRSS]; /* Area to return expanded
                                        ** file name. */
    char          newesa[NAM$C_MAXRSS]; /* Area to return expanded
                                        ** file name. */
    struct FAB    oldfab;               /* File access block. */
    struct FAB    newfab;               /* File access block. */
    struct NAM    oldnam;               /* File name for access. */
    struct NAM    newnam;               /* File name for access. */
  
    CL_CLEAR_ERR(err_code);
    
    /* Check input parameters. */

    if (  pathlength == 0 || (pathlength > DI_PATH_MAX) ||
        (oldlength > DI_FILENAME_MAX) || (newlength > DI_FILENAME_MAX))
    {
	return (DI_BADPARAM);
    }

#ifdef DI_CHECK_RESUME

    if (DI_check_resume)
    {
       DI_check_init();
    }
#endif

    /* Create full lookup name for old file name. */

    MEcopy(path, pathlength,oldesa);
    MEcopy(oldfilename,oldlength,&oldesa[pathlength]);

    /* Create full lookup name for new file name. */

    MEcopy(path, pathlength,newesa);
    MEcopy(newfilename,newlength,&newesa[pathlength]);

    /* Create a file access block with old name. */
    MEfill(sizeof(oldfab), 0, (char *)&oldfab);
    MEfill(sizeof(oldnam), 0, (char *)&oldnam);
    oldfab.fab$l_nam = &oldnam;
    oldfab.fab$b_fns = pathlength + oldlength;
    oldfab.fab$l_fna = oldesa;
    CSget_sid((CS_SID *)&oldfab.fab$l_ctx);
    oldnam.nam$l_esa = oldesa;
    oldnam.nam$b_ess = sizeof(oldesa);
    oldfab.fab$b_bid = FAB$C_BID;
    oldfab.fab$b_bln = FAB$C_BLN;
    oldnam.nam$b_bid = NAM$C_BID;
    oldnam.nam$b_bln = NAM$C_BLN;

    /* Create a file access block with new name. */

    MEfill(sizeof(newfab), 0, (char *)&newfab);
    MEfill(sizeof(newnam), 0, (char *)&newnam);
    newfab.fab$l_nam = &newnam;
    newfab.fab$b_fns = pathlength + newlength;
    newfab.fab$l_fna = newesa;
    newnam.nam$l_esa = newesa;
    newnam.nam$b_ess = sizeof(newesa);
    newfab.fab$b_bid = FAB$C_BID;
    newfab.fab$b_bln = FAB$C_BLN;
    newnam.nam$b_bid = NAM$C_BID;
    newnam.nam$b_bln = NAM$C_BLN;

    /* Now rename the file. */    

    s = rms_suspend(sys$rename(&oldfab, resume_fcn, resume_fcn, &newfab), &oldfab);
    if ((s & 1) == 0)
    {
	/*
	**  If RMS gets SS$_NOIOCHAN error it returns a incomplete
	**  status code of 0x00020000. (i.e. RMS-W-NORMAL).  Check
	**  for this and return DI_EXCEED_LIMIT.
	*/

	if ((s & 0xffff) == 0)
	{
	    err_code->error = SS$_NOIOCHAN;
	    return (DI_EXCEED_LIMIT);
	}
        err_code->error = s;
	if (s== RMS$_DNF)
	    return(DI_DIRNOTFOUND);
        return (DI_BADRNAME);
    }
    return (OK);
}

/*{
** Name: DIsense - Determines end of file.
**
** Description:
**      The DIsense routine is used to find the end of a direct
**      access file.  This routine will always determine the last page
**      used (set by DIflush) by accessing the information from the 
**      file header on disk.  The last page used may be less than
**      the actual physical allocation.
**      (See the file header for explanation of file header contents.)
**      If the file is empty the DIsense call returns -1.  The
**      file must be open to issue a DIsense call.
**
**      The end of file and allocated are not updated until a DIflush
**      call is issued.  This insures that pages are not considered valid 
**      until after they are formatted.
**
**      The DIsense is not used for temporary files.  Their
**      end of file is maintained in the DMF Table Control Block (TCB)
**      structure.  This does not create a problem since temporary
**      files are exclusively visible to only one server and one
**      thread within a server.
**   
** Inputs:
**      f                    Pointer to the DI file
**                           context needed to do I/O.
** Outputs:
**      page                 Pointer to a variable used
**                           to return the page number of the
**                           last page used.
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**    Returns:
**        OK
**        DI_BADINFO         Error occured.
**        DI_BADFILE         Bad file context.
**        DI_BADPARAM        Parameter(s) in error.
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    11-sep-85 (jennifer)
**          Created new for 5.0.
**    20-mar-86 (jennifer)
**          Changed longnat to nat and err_code to CL_ERR_DESC to meet 
**          CL coding standard.
**	05-Oct-2007 (jonj)
**	    Update f->io_alloc_eof, f->io_system_eof for all to see.
*/
STATUS
DIsense(
DI_IO          *f,
i4        *page,
CL_ERR_DESC     *err_code)
{
    i4	          s;                    /* Request return status. */
    IOSB	  local_iosb;           /* Operation return status. */
    F11_JUNK	    eof;
    F11_JUNK	    allocated;
    FAT             io_rec;
                                        /* RMS record information */
    ATRDEF	    io_att[] =
                           {   
                                { ATR$S_RECATTR, ATR$C_RECATTR, (char *)&io_rec},
                                { 0, 0, 0}
                           };           /*    attr fetch information    */

    CL_CLEAR_ERR(err_code);
   
    /* Check file control block pointer, return if bad.*/

    if (f->io_type != DI_IO_ASCII_ID) 
        return (DI_BADFILE);

    /*
    ** Synchronization with other processes is required, must
    ** read physical file to determine EOF.
    */

    s = DI_qio("DI!DI.C::DIsense_1",
               f->io_channel, IO$_ACCESS, &local_iosb,
                 (i4) &f->io_fib_desc, 0, 0, 0, (i4) io_att, 0);

    if (s & 1)
    {
	if (local_iosb.iosb$w_status & 1)
	{
	    /* Update file I/O control block with new EOF. */

	    eof.normal.normal_high  = io_rec.fat$w_efblkh;
	    eof.normal.normal_low = io_rec.fat$w_efblkl;
	    if (eof.long_word)
		eof.long_word--;

	    allocated.normal.normal_high = io_rec.fat$w_hiblkh;
	    allocated.normal.normal_low = io_rec.fat$w_hiblkl;
    
	    /* Return EOF page number to caller. */

	    *page = (eof.long_word >> f->io_blks_log2) - 1;

	    /* Update system/alloc EOF for all to see */
	    f->io_system_eof = max( f->io_system_eof, eof.long_word );
	    f->io_alloc_eof = max( f->io_alloc_eof, allocated.long_word );

	    err_code->error = OK;
	    return (OK);
	}
	s = local_iosb.iosb$w_status;
    }
    
    err_code->error = s;
    return (DI_BADINFO);
}

/*{
** Name: DIwrite -  Writes  page(s) of a file to disk.
**
** Description:
**      The DIwrite routine is used to write pages of a direct access 
**      file.  This routine should be flexible enough to write multiple
**      contiguous pages.  The number of pages to write is indicated
**      as an input parameter,  This value is updated to indicate the
**      actual number of pages written.  A synchronous write is preferred
**      but not required.
**   
** Inputs:
**      f                    Pointer to the DI file
**                           context needed to do I/O.
**      n                    Pointer to value indicating number of pages to write.
**      page                 Value indicating page(s) to write.
**      buf                  Pointer to page(s) to write.
**      
** Outputs:
**      f                    Updates the file control block.
**      n                    Pointer to value indicating number of pages 
**			     written.
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**    Returns:
**          OK
**          DI_BADFILE       Bad file context.
**          DI_BADWRITE      Error writing.
**          DI_BADPARAM      Parameter(s) in error.
**          DI_ENDFILE       Write past end of file.
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    11-sep-85 (jennifer)
**          Created new for 5.0.
**    20-mar-86 (jennifer)
**          Changed longnat to nat and err_code to CL_ERR_DESC to meet 
**          CL coding standard.
**    02-oct-1998 (somsa01)
**          In case of SS$_DEVICEFULL (physical device full) or
**          SS$_EXDISKQUOTA (user allocated disk full), return
**          DI_NODISKSPACE.
**	04-Oct-2007 (jonj)
**	    Add write-past-end-of file check, to hopefully preclude
**	    SYSTEM-W-ENDOFFILE errors seen in cluster testing.
**	06-May-2008 (jonj)
**	    Expand ENDOFFILE handling. If we get one, resense and
**	    retry the write. For whatever reason, this seems to
**	    "work"; in a cluster, there's obviously something not
**	    well understood about file header syncronization.
**	    When it happens, throw messages to errlog.log.
*/
STATUS
DIwrite(
DI_IO	       *f,
i4            *n,
i4        page,
char           *buf,
CL_ERR_DESC     *err_code)
{
    i4		  last_block_to_write;
    int           s;                    /* Request return status. */
    IOSB	  local_iosb;           /* Operation return status. */ 
    TM_STAMP	  tim;
    CL_ERR_DESC   sys_err;
    char	  err_buffer[256];
    i4		  Sensed = 0;
    i4		  MustSense = 0;
    i4		  SavedN = *n;
    CS_SCB        *MySCB;

    CL_CLEAR_ERR(err_code);
   
    /* Check file control block pointer, return if bad. */

    if (f->io_type != DI_IO_ASCII_ID) 
	return (DI_BADFILE);

    for (;;)
    {
	/*
	** Sanity check to make sure we are writing within the bounds of
	** the file.
	*/
	last_block_to_write = (page + *n) << f->io_blks_log2;

	if ( last_block_to_write > f->io_alloc_eof || MustSense )
	{
	    i4	real_eof;

	    /* Note we've sensed */
	    Sensed = 1;

	    s = DIsense(f, &real_eof, err_code);

	    if ( s || last_block_to_write > f->io_alloc_eof )
	    {
		*n = 0;
		if ( s )
		    return(s);

		if ( MustSense )
		{
		    /* Out of bounds after sense, not good */
		    TMget_stamp(&tim);
		    TMstamp_str(&tim, err_buffer);
		    STprintf(err_buffer + STlength(err_buffer),
			" [%x,%x] DIwrite %d: DIsense page %d n %d last_block %d"
			" io_alloc_eof %d io_system_eof %d real_eof %d",
			    MySCB->cs_pid,
			    MySCB,
			    __LINE__,
			    page,
			    SavedN,
			    last_block_to_write,
			    f->io_alloc_eof,
			    f->io_system_eof,
			    real_eof);
		    ERlog(err_buffer, STlength(err_buffer), &sys_err);
		}

		return(DI_ENDFILE);
	    }
	}

	s = DI_qio("DI!DI.C::DIwrite_1",
                   f->io_channel, IO$_WRITEVBLK, &local_iosb,
                 (i4) buf, f->io_block_size * *n, (page << f->io_blks_log2) + 1,
		 0, 0, 0);

	if (s & 1)
	{
	    if (local_iosb.iosb$w_status & 1)
	    {
		/*
		** Successful write. 
		**
		** If second attempt after sense, log it.
		*/
		if ( MustSense )
		{
		    TMget_stamp(&tim);
		    TMstamp_str(&tim, err_buffer);
		    STprintf(err_buffer + STlength(err_buffer),
			" [%x,%x] DIwrite %d: SS$_ENDOFFILE page %d n %d last_block %d"
			" io_alloc_eof %d io_system_eof %d OK after sense",
			    MySCB->cs_pid,
			    MySCB,
			    __LINE__,
			    page,
			    SavedN,
			    last_block_to_write,
			    f->io_alloc_eof,
			    f->io_system_eof);
		    ERlog(err_buffer, STlength(err_buffer), &sys_err);
		}
		err_code->error = OK;
		return (OK);
	    }
	    s = local_iosb.iosb$w_status;
	    *n = local_iosb.iosb$l_dev_depend >> f->io_log2_bksz;
	}
	else
	    *n = 0;
	
	err_code->error = s;
	if (s == SS$_ENDOFFILE)
	{
	    /*
	    ** Apparent write past EOF.
	    **
	    ** Log it, then if we haven't already done so,
	    ** re-sense and try the write again.
	    */
	    CSget_scb(&MySCB);

	    TMget_stamp(&tim);
	    TMstamp_str(&tim, err_buffer);
	    STprintf(err_buffer + STlength(err_buffer),
		" [%x,%x] DIwrite %d: SS$_ENDOFFILE page %d n %d last_block %d"
		" io_alloc_eof %d io_system_eof %d dev_depend %d Sensed %d",
		    __LINE__,
		    MySCB->cs_pid,
		    MySCB,
		    page,
		    SavedN,
		    last_block_to_write,
		    f->io_alloc_eof,
		    f->io_system_eof,
		    local_iosb.iosb$l_dev_depend,
		    Sensed);
	    ERlog(err_buffer, STlength(err_buffer), &sys_err);

	    if ( !Sensed && !MustSense )
	    {
		MustSense = 1;
		*n = SavedN;
		continue;
	    }

	    return(DI_ENDFILE);
	}
	if ((s == SS$_DEVICEFULL) || (s == SS$_EXDISKQUOTA))
	    return(DI_NODISKSPACE);
	return (DI_BADWRITE);
    }
}

/*
** Name: getpath - Returns a device name and directory identifer.
**
** Description:
**	This is a VMS specific internal routine.
**      This is used to cache internal path 
**      information making it quicker to access a file.  It
**      keeps an array of path cache entries and will return one that 
**      matches the specified path name, or will define a new
**      one if none exists.  The array has a maximum length of
**      DI_PCACHE_MAX.  If they are all used, it will just
**      throw out the first one and use it for the requested
**      path. 
**   
** Inputs:
**      path                 Pointer to the path name.
**      pathlength           Length of path name.
**      
** Outputs:
**      path_entry	     Pointer to area to returned path.
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**    Returns:
**          OK
**          DI_BADDIR
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    11-sep-85 (jennifer)
**          Created new for 5.0.
**    20-mar-86 (jennifer)
**          Changed longnat to nat and err_code to CL_ERR_DESC to meet 
**          CL coding standard.
**    23-Feb-87 (ac)
**	    Added retry flag to re-make the path for the path specified.
**	    This is to solve the problem that internal path information
**	    of a file in the cache is out of date due to the directory
**	    is destroyed and recreated outside the context of the current 
**	    running server/process.
**	22-jul-1992 (bryanp)
**	    If sys$parse returns RMS$_CHN, treat this as SS$_NOIOCHAN; that is,
**	    return DI_EXCEED_LIMIT to our caller.
*/
static STATUS
getpath(
char            *path,
i4             pathlength,
PATH_ENTRY	*path_entry,
i4		retry,
i4		sid,
CL_ERR_DESC      *err_code)
{
    int         s;			/* Request return status. */
    int         i;			/* Array counter. */
    int		reclaim;		/* Slot to reclaim. */
    int		refcount;		/* Minimum reference count. */
    int		ast_status;
    int		fnb;
    struct FAB	fab;
    struct NAM	nam;
    char        esa[NAM$C_MAXRSS];


    CL_CLEAR_ERR(err_code);
   
#ifdef DI_CHECK_RESUME

    if (DI_check_resume)
    {
       DI_check_init();
    }
#endif

    ast_status = sys$setast(0);

    /*	Look in the path cache. */

    for (i = 0; i < DI_PCACHE_MAX; i++)
    {
	if (path_cache[i].c_refcount &&
	    path_cache[i].c_path_cnt == pathlength &&
	    MEcmp(path, path_cache[i].c_path, pathlength) == 0)
	{
	    if (retry)
	    {
		/*  Purge cache entry. */

		path_cache[i].c_refcount = 0;
		continue;
	    }

	    /*	Return cache entry. */

	    path_cache[i].c_refcount++;
	    *path_entry = path_cache[i];
	    if (ast_status == SS$_WASSET)
		sys$setast(1);
	    return (SS$_NORMAL);
	}
    }
    if (ast_status == SS$_WASSET)
	sys$setast(1);
    
    /*	Perform device and directory identifier search. */

    MEfill(sizeof(fab), 0, (char *)&fab);
    MEfill(sizeof(nam), 0, (char *)&nam);
    fab.fab$l_nam = &nam;
    fab.fab$l_fna = path;
    fab.fab$b_fns = pathlength;
    fab.fab$l_ctx = sid;
    nam.nam$l_esa = esa;
    nam.nam$b_ess = sizeof(esa);
    fab.fab$b_bid = FAB$C_BID;
    fab.fab$b_bln = FAB$C_BLN;
    nam.nam$b_bid = NAM$C_BID;
    nam.nam$b_bln = NAM$C_BLN;
    s = rms_suspend(sys$parse(&fab, resume_fcn, resume_fcn), &fab);
    if ((s & 1) == 0)
    {
	/*
	**  If RMS gets SS$_NOIOCHAN error it returns a incomplete
	**  status code of 0x00020000. (i.e. RMS-W-NORMAL).  Check
	**  for this and return SS$_NOIOCHAN.
	*/

	if (s == RMS$_CHN)
	    s = SS$_NOIOCHAN;
	else if ((s & 0xffff) == 0)
	    s = SS$_NOIOCHAN;
	err_code->error = s;
	return (s);
    }
    fnb = nam.nam$l_fnb;

    /*	Initialize the path entry. */

    path_entry->c_refcount = 1;
    path_entry->c_io_direct[0] = nam.nam$w_did[0];
    path_entry->c_io_direct[1] = nam.nam$w_did[1];
    path_entry->c_io_direct[2] = nam.nam$w_did[2];
    path_entry->c_devnam_cnt = nam.nam$b_dev;
    path_entry->c_path_cnt = pathlength;
    path_entry->c_devnam_cnt = nam.nam$b_dev;
    MEcopy(path, pathlength, path_entry->c_path);
    MEcopy(nam.nam$l_dev, nam.nam$b_dev, path_entry->c_device);

    /*	Release RMS resources. */

    nam.nam$b_nop = NAM$M_SYNCHK;
    rms_suspend(sys$parse(&fab, resume_fcn, resume_fcn), &fab);

    /*	Check for garbage in path name. */

    if ((fnb & (NAM$M_EXP_DEV | NAM$M_EXP_DIR | NAM$M_EXP_NAME |
	NAM$M_EXP_TYPE | NAM$M_EXP_VER | NAM$M_NODE)) != 
	(NAM$M_EXP_DEV | NAM$M_EXP_DIR))
    {
	err_code->error = RMS$_DEV;
	return (RMS$_DEV);
    }

    if (ast_status == SS$_WASSET)
	sys$setast(0);

    /*	Find cache entry to reuse. */

    reclaim = 0;
    refcount = 0x7fffffff;
    for (i = 0; i < DI_PCACHE_MAX; i++)
    {
	if (path_cache[i].c_path_cnt == pathlength &&
	    MEcmp(path, path_cache[i].c_path, pathlength) == 0)
	{
	    reclaim = DI_PCACHE_MAX;
	    break;
	}
	if (path_cache[i].c_refcount == 0)
	    reclaim = i, refcount = 0;
	else if (path_cache[i].c_refcount < refcount)
	{
	    reclaim = i;
	    refcount = path_cache[i].c_refcount;
	}
    }
    if (reclaim < DI_PCACHE_MAX)
    {
	path_cache[reclaim] = *path_entry;
    }

    if (ast_status == SS$_WASSET)
	sys$setast(1);
    return (SS$_NORMAL);
}

/*
** Name: list_files	- Common routine to list files
**
** Description:
**      This routine is called internally by DIlistfile and DIlistdir to
**	handle the listing operation. 
**
** Inputs:
**      fab                             Pointer to fab.
**      flag                            Edit flag.
**
** Outputs:
**	Returns:
**	    DI_ENDFILE
**	    DI_BADLIST
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      17-dec-1987 (Derek)
**          Created.
*/
static STATUS
list_files(
struct FAB          *fab,
STATUS		    (*func)(),
PTR		    arg_list,
int		    flag,
CL_ERR_DESC	    *err_code)
{
    i4	    s;
    i4	    length;
    struct NAM	    *nam = fab->fab$l_nam;
    char	    filename[NAM$C_MAXRSS];

    CL_CLEAR_ERR(err_code);
   
#ifdef DI_CHECK_RESUME

    if (DI_check_resume)
    {
       DI_check_init();
    }
#endif

    s = rms_suspend(sys$parse(fab, resume_fcn, resume_fcn), fab);
    if ((s & 1) == 0)
    {
	/*
	**  If RMS gets SS$_NOIOCHAN error it returns a incomplete
	**  status code of 0x00020000. (i.e. RMS-W-NORMAL).  Check
	**  for this and return DI_EXCEED_LIMIT.
	*/

	if (s == RMS$_CHN || (s & 0xffff) == 0)
	{
	    err_code->error = SS$_NOIOCHAN;
	    return (DI_EXCEED_LIMIT);
	}
        err_code->error = s;
	if (s != RMS$_DNF)
	    return (DI_BADDIR);
	return (DI_DIRNOTFOUND);
    }

    /* Now search directory for all files. */
    
    for (;;)
    {
	i4	    i;

        s = rms_suspend(sys$search(fab, resume_fcn, resume_fcn), fab);
	if ((s & 1) == 0)
	{
            err_code->error = s;
	    if (s == RMS$_FNF || s == RMS$_NMF)
		return (DI_ENDFILE);
	    return (DI_BADLIST);
	}	
        
	/* 
	** Get file name part and type of returned name.
        ** That is, the returned name is of the form:
        **     dev1:[dir1.dir2]name.typ;v
        ** We only want to pass the name and tyfpe or
        **     name.typ
        */

	if (flag == 0)
	{
	    MEcopy (nam->nam$l_name, nam->nam$b_name, filename);
	    length = nam->nam$b_name;                
	}
	else
	{
	    MEcopy (nam->nam$l_name, nam->nam$b_name + nam->nam$b_type, filename);
	    length = nam->nam$b_name + nam->nam$b_type;                
	    for (i = 0; i < length; i++)
		if (filename[i] >= 'A' && filename[i] <= 'Z')
		    filename[i] |= ' ';
	}
	s = (*func)(arg_list, filename, length, err_code);
        if (s != OK)
	    break;
    }    

    /*	Release RMS resources. */

    nam->nam$b_nop = NAM$M_SYNCHK;
    rms_suspend(sys$parse(fab, resume_fcn, resume_fcn), fab);
    return (DI_ENDFILE);
}

/*
** Name: rms_suspend	- Wait for RMS completion.
**       rms_resume     - Signal RMS completion.
** Description:
**      This routine waits for the RMS completion AST and then
**	calls CSresume to wake the thread up again. 
**
** Inputs:
**      fab                             Pointer to fab.
**
** Outputs:
**	Returns:
**	    void
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      17-dec-1987 (Derek)
**          Created.
*/

static STATUS
rms_suspend(
STATUS		s,
struct FAB	*fab)
{
    if (s == RMS$_BLN || s == RMS$_BUSY || s == RMS$_FAB || s == RMS$_RAB ||
	s == RMS$_STR)
    {
	return (s);
    }

#ifdef DI_SUSPEND_THREAD

    DI_SUSPEND_THREAD;

#endif

    return (fab->fab$l_sts);
}
static void
rms_resume(
struct FAB	*fab)
{
    CSresume(fab->fab$l_ctx);
}

/* horda03 - Added to track problems in DI due to Rogue Resumes */

static i4
DI_qio(
char *descrip,
II_VMS_CHANNEL channel,
i4 func,
IOSB *iosb,
i4 p1,
i4 p2,
i4 p3,
i4 p4,
i4 p5,
i4 p6)
{
   i4                s;

#ifdef OS_THREADS_USED

   /* ALPHA can be internal or OS threaded */

#ifdef ALPHA
   if (! resume_fcn )
   {
#endif
      s = sys$qiow(EFN$C_ENF, channel, func, iosb, NULL, 0,
                   p1, p2, p3, p4, p5, p6 );

#ifdef ALPHA
   }
   else
   {
#endif
#endif /* OS_THREADS_USED */

#if defined(ALPHA) || !defined(OS_THREADS_USED)

      CS_SID sid;
      CSget_sid(&sid);

      CSnoresnow(descrip, 1);

      s = sys$qio( EFN$C_ENF, channel, func, iosb, DI_resume, &sid,
                   p1, p2, p3, p4, p5, p6 );

      if (s & 1)
      {
         /* QIO action, now wait for completion */

         do
         {
            CSsuspend( CS_DIO_MASK, 0, 0);

         } while (!iosb->iosb$w_status);
      }
#endif /*defined(ALPHA) || !defined(OS_THREADS_USED) */

#if defined(OS_THREADS_USED) && defined(ALPHA)
   }
#endif

   if (s & 1) s = iosb->iosb$w_status;

   return s;
}

static void
DI_resume(CS_SID *sid)
{
    CSresume(*sid);
}

#ifdef DI_CHECK_RESUME
static void
DI_check_init(void)
{
   if (!CS_is_mt()) resume_fcn = rms_resume;

   DI_check_resume = 0;
}
#endif
