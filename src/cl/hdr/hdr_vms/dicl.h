/*
** Copyright (c) 1985, 2005 Ingres Corporation
**
*/

#ifndef         DICL_INCLUDED
#define         DICL_INCLUDED     1

/**CL_SPEC
** Name: DI.H - This file contains the external structs for DI.
**
** Description:
**      The DI.H file is used to contain all structures and 
**      definitions required to call the compatability library
**      DI routines.  These routines perform all direct file
**      I/O for all database files, such as database tables,
**      configuration files, etc.  It is not meant to be used for
**      such operations as copying tables to user files, the 
**      stream I/O(SI) routines are used for this.  The main
**      structure used to keep track of operating system information
**      needed to perform the I/O is called DI_IO and is defined
**      below.  This structure varies significantly depending
**      on the operating system.
**
** Specification:
**  
**  Description:
**(	Direct file I/O for shared files. These files have the following
**	extensions:
**
**               .cnf  - For configuration file.
**               .tab  - For all database files.
**               .d##  - For all files waiting to be deleted.
**                       If any of these are left around 
**                       while a database is not in use they
**                       can be deleted.
**               .m##  - The new  database file as a result of
**                       a modify.  If modify is successfull, then
**                       at the end it is changed to the .tab extention
**                       identifier.  If any of these are left 
**                       around while a database is not in use, they
**                       can be deleted.
**               .ali  - Detects aliasing of locations during 
**                       checkpointing.  This is a temporary table
**                       and can be deleted if left around while 
**                       a database is not in use.
**               .ovl  - The overflow pages during the modify to
**		         ISAM.  If modify is successfull, then at
**                       the end it is deleted.  If any of these are
**                       left around while a database is not in use,
**                       they can be deleted.
**               .sm1  - During a sysmod, a table is temporarily 
**                       rename to a .sm1 file name.
**               .sm2  - During a sysmod, an index table is temporarily 
**                       rename to a .sm2 file name.
**)                          
** 
**  Intended Uses:
**      DI is used to create, delete,  and maintain the files used 
**      by the DBMS to store the data associated with INGRES Databases.  
**      This includes all files associated with system and user tables 
**      and the database configuration file.  All these files are 
**      shared resources amoung users and processes.  No serialization
**      of calls can be expected.  For example you maybe trying
**      to open a file for one caller and be interrupted to open
**      a file for another caller.  The DI code must be able to 
**      handle this.  The DI routines
**      provide a direct access mechanism for accessing data in blocks.
**      It is a requirement of DI files that they can be created and 
**      openeed with the same block size.  Host operating systems 
**      which can check this should.  If they don't match it should
**      return DI_BADPARAM.
**      These routines are designed to provide enough functionality 
**      to insure that the data in the files can be recovered in case 
**      of system crashes or database recovery.  
**
**      Recovery also depends on a file system
**      capable of insuring the integrity of disk space structures used
**      to implement the file system.  For example, VAX/VMS must insure
**      all directory updates are atomic. DI is intended to be used only by DMF
**      and the database support routines such as CreateDB, DestroyDB,
**      VerifyDB, RecoverDB, etc.  No other part of the DBMS 
**      should be a client of the DI module.
**      
**    Assumptions:
**      It is assumed that the concept of a random access shared file
**      is understood.  The System is assumed to have some kind of
**      file system or disk I/O mechanism that DI is built on top of.  
**
**      In addition, it is assumed that the DI routines contain enough
**      functionality to insure that the main client DMF does not have
**      to have any machine dependent code to insure the same results 
**      on all machines.  For example, with some operating systems
**      all pages are physically on disk after a write operation
**      completes.  Other operating systems may complete the request
**      without actually insuring the data is on disk.  For these systems
**      the DMF will use the DIforce call to insure all data is on disk
**      at critical points like end transaction.
**      
**    Definitions:
**
**    Concepts:
**
**      The DI routines are assumed to be built on top of System service
**      routines which allow  random access to data within a file
**      by addressing a specific byte of that data.  It also assumes that
**      data can be segmented into  blocks (usually the sector size of
**      the disk ) and that contiguous blocks can be accessed as pages.
**      The basic unit of access for the DI routines is a page.  The page
**      size is set at creation time and must be openned with same size.
**      The first addressable page in a file
**      is always page 0.  For certain types of access (for example
**      read ahead) multiple pages will be accessed with one call of
**      the appropriate DI routines.  In these cases the pages must
**      be logically contiguous.
**
**      DI routines participate in guaranteeing the consistency of the
**      data.  For example the DIflush and DIforce calls exist to
**      guarantee the file header information and the data are on
**      disk prior to transaction commit, etc.   
**
**      All DI files are sharable.  That is more than one open 
**      of the file may exist at any one time for both reading
**      and writing.
**
**      The DI routines must be reentrant, namely all information
**      that must be maintained between DI calls must be passed as
**      an argument.  To accomplish this all DI calls contain a 
**      pointer to a file context (of type DI_IO) which can be used to
**	contain any information needed for future calls.  For example the
**	System file identifier, the file allocation information such as 
**      physical end of file and logical end of file, etc.  Implementation
**      of the DI routines on different systems will have different file
**      context structures and often the file context will be ignored
**      on a specific call for a specific System.
**
**      All DI routines are responsible for providing in a structure
**      (of type CL_SYS_ERR) any operating system error that occurred
**      during execution.  It should be initialized to OK by all
**      DI routines.  This operating system error will be logged
**      by the ULF log utility routines.  The operating system errors
**      must also be converted to one of the following DI errors:
**(	    
**		DI_OK           
**		DI_BADPARAM     
**		DI_BADFILE	
**		DI_ENDFILE      
**		DI_BADOPEN	
**		DI_BADCLOSE	
**		DI_BADWRITE	
**		DI_BADREAD	
**		DI_BADEXTEND	
**		DI_BADDIR	
**		DI_BADSEEK	
**		DI_BADSENSE	
**		DI_BADCREATE	
**		DI_BADDELETE    
**		DI_BADRENAME    
**              DI_NODISKSPACE
**              DI_EXCEED_LIMIT
**              DI_DIRNOTFOUND
**              DI_FILENOTFOUND
**              DI_EXISTS
**)      
**	Before I/O operations can be performed on a file the file
**      must exist.  If it does not exist it must be created.  The
**      DIcreate routine only creates the file, it does not allocate
**      any physical storage or open it.  Effectively it creates an
**      entry in a directory.  After the initial creation, the file
**      can be opened by the DIopen call.  A file is created or opened
**      by name.  The name has two parts, the pathname (directory tree)
**      and the filename.  A pathname to DI is a pathname recognized by
**      the System.  A pathname can be up to DI_PATH_MAX characters.
**	Any DI call being passed a pathname will also be passed its length.
**      The filename to DI is a filename recognized by the System.  A
**      filename can be up to DI_FILE_MAX characters including any file
**	type extension.  Any DI call being passed a filename will also 
**      be passed its length.  
**
**      The DI pathname is one created by calls to LOingpath and
**      usually has at least two parts, the physical location
**      string provided when a location is defined to accessdb.
**      For VAX/VMS this is usually a disk device name such 
**      as disk$data.  However it can be a rooted directory
**      name such as disk$data:[aaa.bbb.ccc.]. This string is
**      operating system dependent.  LOingpath adds to this
**      the operating system specific ingres name to create the entire
**      path name.  The VAX/VMS ingres name is [ingres.data.dbname]
**      which would create the path disk$data:[ingres.data.dbname]
**      or disk$data:[aaa.bbb.ccc.][ingres.data.dbname].  The 
**      maximum values for the pathname and filename are defined
**      with the constants DI_PATH_MAX and DI_FILENAME_MAX.
**
**      Once a file is opened it can be extended, read, written or
**      closed.  The DI routines to perform these operations determine
**      the specific file to access through a System file identifier
**      stored in the file context.  Each of these routines relies
**      on a valid file context to function properly.  The routines
**      used to maintain the file context are: 
**(
**                  DIalloc - Extend the logical & physical file.
**                  DIflush - Flush file header to disk.
**                  DIforce - Force all pages held by System to disk.
**                  DIread  - Reads pages from the file.
**                  DIwrite - Writes  pages to the file.
**                  DIsense - Determine logical end of file.
**)                 
**      Since you can DIread or DIwrite more than one page
**      these routines must be able to tell you if less than 
**      the number requested was written or read.  This is
**      done by returning DI_ENDFILE for these calls instead
**      of OK when the request cannot be satisfied.
**      
**(     The logical end of file is maintained through the use
**      of the DIsense, DIwrite, and DIalloc calls as follows:
**         1.  DIsense is guaranteed to return correct
**             logical end-of-file.  DIsense should 
**             return the logical
**             end-of-file on disk.  For most systems
**             this will be the value written to the file
**	       header at DIflush time.  For those systems
**	       that track end-of-file by actual writes to
**             the file, then the logical end-of-file is
**             the highest numbered page ever written by 
**             DIwrite.
**	   2.  It is guaranteed that between the time 
**             DIalloc is called on a file and the time
**             DIflush is called on a file no other DIalloc
**             calls on the file will be made by either the original
**             caller or any other caller (in the current 
**             server or any other).  This is guaranteed 
**             through the use of a file extend lock.
**         3.  A file is extended by calling DIalloc to 
**             logically extend a file.  This includes 
**             a physical extention if enough space 
**             is not currently available.  For those operating
**             systems that do not have a way of extending
**	       the file without writing blocks to
**             the file, this call may do nothing but 
**             keep track of the allocation request
**             in the DI_IO control area in memory.  
**         4.  Any DIwrite call that is for a page greater than
**             the DI_IO logical end-of-file and less than or equal to
**             the DI_IO logical end-of-file + allocation request
**             can be legally written.
**             These writes can be thought of updating the physical 
**             end-of-file on disk for 
**             those file systems that do not have a file header
**             on disk.  This call 
**             would update the DI_IO logical end-of-file value.
**	   5.  Between the time DIalloc is called and the time
**             DIflush is called DIsense will return 
**             the logical end-of-file from disk.
**	   6.  The DIflush call causes the DI_IO logical 
**	       end-of-file value to be updated in the DI_IO
**	       context and flushed to disk for those systems
**)	       that have file header information on disk.
**	   
**
**	Note that the DI_IO logical end-of-file may
**      not be the same for different servers, if one server has
**      had a file open and been actively extending it, then 
**      its DI_IO logical end-of-file reflects what has been
**      allocated even if the pages have not been written yet.
**      if another server open this file while the first server
**      is in the middle of an allocation request, the second 
**      server will initialize its DI_IO logical end-of-file 
**      from the disk information.  For file systems that
**      have a file header, then the two servers will have 
**      the same value for DI_IO logical end-of-file since
**      the header on disk is not updated until the DIflush
**      call.  However, for those systems that keep track of
**      the end-of-file on disk by what has been written, the
**      second server can have a DI_IO logical end-of-file 
**      that is different from the first one.  However DMF
**      guarantees in a multi-server environment that the
**      disk is always consistent, therefore the DI_IO logical
**      end-of-file in the second server will always be 
**      greater than the one in the first server 
**             
**
**      When you are finished with a file it must be closed with a
**      call to DIclose.  If the file is no longer needed it should
**      be deleted with a call to DIdelete.  You cannot delete a 
**      file that is open.  A delete call can be made while the file 
**      is opened, but the file will not be deleted until all 
**      opened contexts on this file are closed.  It is not possible
**      to open a file that has been marked for deletion.  Any other
**      user who has the file open already may perform all other
**      operations (such as sense, alloc, read, write, close).  
**      A file that is marked for deletion cannot be renamed.
**      A delete of a file is not associated with any open or open
**      context.  The close call uses the System identifier in the
**      file context to determine which file to close, the delete
**      is done by name.  A System path and file name must be passed
**      to the delete routine.  In order to support recovery of certain
**      file operations it is sometimes necessary to delay deletion
**      of a file to after transaction commit.  These files are 
**      often renamed until they can be deleted.  Therefore there is
**      a DIrename routine.  The file must be closed to rename it.
**
**      The other DI routines are needed to create paths (directories)
**      and destroy paths (directories) for the CreateDB and DestroyDB
**      support routines.  These DI routines are:
**(
**	    DIdircreate - Creates a directory.
**	    DIdirdelete - Deletes a directory.
**	    DIlistdir   - List all directories in a directory.
**          DIlistfiles - List all files in a directory.
**)
**	For the DIdircreate and DIcreate routines a check is
**      made to see if the object already exists.  If it does
**      either the DI_DIRNOTFOUND or DI_FILENOTFOUND must be
**      returned as errors.  DIlistdir and DIlistfiles must
**      return DI_ENDFILE when it finds the last object in 
**      a directory.
**
**      For all DI files there is a concept of quota. Therefore
**      if you try to exceed the operating system quota for 
**      a specific user or the entire process when creating,
**      opening or writing a DI file or directory, then the
**      error DI_EXCEED_LIMIT must be returned.  In addition
**      it is understood that at a minimum either the operating
**      system or the DI filesystem must be able to handle at
**      least 250 logical file opens before it can declare
**	DI_EXCEED_LIMIT;  THis is to insure that the single maximum
**	query can  be run. (32 table join with at most 6 indexes on each
**      table = 192).
**    
**	The concept of physical file ownership for such purposes
**      as operating system accounting and quota must be handled
**      by DIcreate and DIdircreate.  If a particular operating 
**      system requires that the files be owned by the user running 
**      at the time of the create the DI routines must use
**      the appropriate calls to CS to determine who is running,
**      set appropriate user id to this user and then make the
**      operating system call so the file is associated with
**      the user instead of the process.  This insures that in 
**      a server process the ingres user does not own
**      all the tables it creates. 
** 
**      To allow for multiple locations for one table (i.e. 
**      table spread accross multiple disk drives) two new
**      constants have been added to DI.  DI_EXTENT_SIZE is
**      the number of pages to be written to each disk before
**      writing to the next disk.  For example if DI_EXTENT_SIZE 
**      is four and there are three locations, the logical pages
**      0-24 would be allocated as follows:
**(              Location 0  Pages 0,1,2,3,  12,13,24,15
**               Location 1  Pages 4,5,6,7,  16,17,18,19
**               Location 2  Pages 7,8,9,10, 11,12,13,14
**)      
**      The DI_EXTENT_SIZE must be a power of two to make the 
**      calculations for determining where to write a logical 
**      page simple.  There is an additional constant used
**      in these calculations which is log2 of the extent 
**      size and is used in shifting.  The constant is 
**      DI_LOG2_EXTENT_SIZE.
**	** Note ** This constant is grossly mis-named and mis-placed
**	because DI does not do multi-locations at all.  It's up to
**	higher levels to deal with multiple locations.

**  History:
**      06-sep-85 (jennifer)
**          Created new for 6.0.
**	15-nov-87 (rogerk)
**	    Changed DI_FILENAME_MAX from 12 to 28.  This is so DI
**	    routines can handle directory names which are can be
**	    up to 24 (supported database name length) characters.
**	    28 is 24 + 4 bytes for ".dir".
**      10-DEC-87 (JENNIFER)
**          Added multiple locations constants.
**	18-oct-88 (mmm)
**	    Added DI_OSYNC_MASK and DI_FSYNC_MASK.
**	23-jan-89
**	    DI only will externally support DI_SYNC_MASK.
**	30-October-1992 (rmuth)
**	    - Add new function calls DIgalloc and DIguarantee_space.
**	    - Add some new DI error messages
**	    - Remove DI_ZERO_ALLOC and DI_O_ZERO_ALLOC.
**	1-dec-92 (ed)
**	    - increased DI_MAXNAME to 36, to complement DB_MAXNAME=32
**	    36-32 is the length of ".dir" required for directories
**	26-apr-1993 (bryanp)
**	    Finished prototyping DI for 6.5.
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**	18-apr-1994 (jnash)
**	    fsync() project.  Added DI_USER_SYNC_MASK.
**	22-dec-1995 (dougb)
**	    Cross-integrate part of change 418995 from Unix CL:
**		20-jul-1995 (canor01)
**		Add DI_LOG_FILE_MASK for use with DIopen() for those versions
**	    of DI that may need it.
**	    ??? Should this macro be used in the VMS DIopen() code?
**	04-sep-1997 (kinte01)
**          Added DI_ACCESS for file open access error.
**	15-sep-1997 (kinte01)
**	    Define DI_ACCESS to the correct value (mistakenly assigned the incorrect
**	    value in the last integration).
**	02-oct-1998 (somsa01)
**	    Added DI_NODISKSPACE to distinguish between an out-of-disk-space
**	    condition and an out-of-quota condition.
**	01-nov-1999 (kinte01)
**	    Add definitions of DI_RAWIO_ALIGN that were moved from lgdef.h
**	    on 22-Dec-1998 (jenjo02)
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**     18-sep-01 (kinte01)
**          Renamed elements of _DI_IO struct to be same as Unix structure for
**	    raw log support
**	07-dec-2004 (abbjo03)
**	    Partial cross-integration of 6-feb-89 mikem change from Unix:
**	    Added DI_IO_*_INVALID constants to keep track of who marks a
**	    DI_IO invalid.
**	14-Oct-2005 (jenjo02)
**	    Added DIopen(DI_DIRECTIO_MASK) to request directio 
**	    per file, rather than "all" files.
**	7-Aug-2009 (kschendel) SIR 122512
**	    Add a maximum page size.
**	25-Nov-2009 (kschendel) SIR 122757
**	    Remove some common definitions to di.h to fix VMS compile problems.
**/

/*
**  Forward and/or External typedef/struct references.
*/
typedef struct _DI_IO DI_IO;
/*
The following structures are defined below:
*/

/*
**  Forward and/or External function references.
*/

/* Maximum page size supported by this DI implementation: */

#define			DI_MAX_PAGESIZE		65536

/* DI return status codes. */
/* Status codes common to all platforms are in di.h */

# define                DI_BADLRU_RELEASE       (E_CL_MASK + E_DI_MASK + 0x14)
# define                DI_BADCAUSE_EVENT       (E_CL_MASK + E_DI_MASK + 0x15)
# define                DI_BADSUSPEND           (E_CL_MASK + E_DI_MASK + 0x16)


/* VMS doesn't have raw I/O, so no need for alignment. */

# define DI_RAWIO_ALIGN 0

/*}
** Name: DI_IO - This is the I/O structure needed to do direct I/O.
**
** Description:
**      The DI_IO structure is used to contain all the operating
**      system dependent I/O information required for direct I/O.
**      It is very operating system specific.  The memory for this
**      structure is allocated by the caller of the DI routines.
**	This is the structured used on VAX/VMS.
**
** History:
**     06-sep-85 (jennifer)
**          Created new for 5.0.
**     18-sep-01 (kinte01)
**          Renamed elements of _DI_IO struct to be same as Unix structure
*/
typedef struct _DI_IO
{
    i4	    io_system_eof;		/* End of file information. */
    i4	    io_alloc_eof;	/* Last allocated disk block. */
    i4	    io_type;            /* Control block identifier. */
#define                 DI_IO_ASCII_ID     'DICB'
    i4	    io_channel;         /* VMS channel used for this file */
    i4 	    io_block_size;	/* File block size. */
    i4	    io_log2_bksz;	/* Log 2 of io_block_size. */
    i4	    io_blks_log2;	/* Log 2 of io_block_size / 512. */
#define			DI_IO_BLK_SIZE		512
    i4	    io_open_flags;	/* Open flags. */
#define			DI_O_SYNC_FLAG	0x001
    struct
    {
     i4	    str_len;            /* Length of described item. */
     char	    *str_str;	        /* Address of described item. */
    }               io_fib_desc;        /* FIB descriptor. */
    char	    io_fib[32];		/* VMS File Information Block. */
} DI_IO;

#define			DI_IO_CLOSE_INVALID	-5


FUNC_EXTERN STATUS DImo_init( VOID );

#endif          /* DICL_INCLUDED */

