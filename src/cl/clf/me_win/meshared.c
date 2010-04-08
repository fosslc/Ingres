/*
**  Copyright (c) 1988, 2004 Ingres Corporation
*/

#include   <compat.h>
#include   <clconfig.h>
#include   <meprivate.h>
#include   <gl.h>
#include   <cs.h>
#include   <st.h>
#include   <me.h>
#include   <errno.h>
#include   <er.h>
#include   <lo.h>
#include   <si.h>
#include   <nm.h>
#include   <winnt.h>
#include   <winbase.h>

/*
**  Name: MESHARED.C - System shared memory interface calls
**
**  Description:
**	This file contains procedures to abstract whatever shared memory
**	facilities a paticular dialect of UNIX might provide.
**
**	This is the WIN32/NT implementation, based on the Sun version.
**
**  History:
**	22-aug-1995 (canor01)
**	    add ME_segpool_sem to protect qlobal queue.
**	21-mar-1996 (canor01)
**	    clean up some casts to remove compiler warnings
**	03-may-1996 (somsa01)
**		Modified MEshow_pages() so that fname is "filled" with only the
**		filename and its extension. This is what needs to be passed into
**		(*func) ().
**      06-Jun-96 (fanra01)
**          Changed the reporting of errors in the ME_alloc_shared function.
**          When an error occurs the Win32 last error is reported.
**      13-Jan-97 (fanra01)
**          Add initialise of name when opening a file mapping.  Previously
**          taking a random stack value potentially closing an invalid handle.
**      04-Apr-97 (fanra01)
**          Bug 81339
**          Updated function to return the number of pages in the shared memory
**          file if attaching to shared memory.
**	07-apr-1997 (canor01)
**	    When the shared memory is attached to existing memory
**	    the file handle will be null, so don't try to close it when
**	    freeing the memory.
**	27-may-97 (mcgem01)
**	    Clean up compiler warnings.
**      01-oct-97 (chudu01)
**          Modified ME_alloc_shared() to avoid conflicts between Jasmine
**          and Ingres installations.  Hopefully this fix will also allow
**          multiple Jasmine installations to run on the same machine.
**      19-Aug-98 (fanra01)
**	    Bug 86981 - ingstop -force fails with fatal error.
**          Rearranged the order of registering a memory segment until after
**          we have determined the number of pages allocated.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	28-jun-2001 (somsa01)
**	    Since we may be dealing with Terminal Services, prepend
**	    "Global\" to the shared memory name, to specify that this
**	    shared memory segment is to be opened from the global name
**	    space.
**	06-aug-2001 (somsa01)
**	    Use meprivate.h instead of melocal.h .
**	29-mar-2002 (somsa01)
**	    In ME_alloc_shared(), close the memory file's handle as
**	    appropriate.
**	22-oct-2002 (somsa01)
**	    Changes to allow for 64-bit memory allocation.
**	07-oct-2003 (somsa01)
**	    Updated call to CreateFileMapping() to properly account for
**	    sizes greater than 2GB on 64-bit systems.
**	30-Nov-2004 (drivi01)
**	  Updated i4 to SIZE_TYPE to port change #473049 to windows.
**  	27-Sep-2005 (fanra01)
**    	  Bug 115298
**    	  A service started as LocalSystem user will inherit special security
**        and environment settings from the service control manager as default.
**        If the service processes do not specify security attributes the
**        defaults are used preventing access between a service and a user
**        application.
**        Add security attributes for the creation of shared memory.
**	15-Oct-2005 (drivi01)
**	  Added FlushFileBuffer function before we close handle
**	  to the file and map to speed up deletion of shared
**	  memory to avoid left over references to non-existing object.
*/

/******************************************************************************
** Definition of all external functions
******************************************************************************/

ME_SEG_INFO *ME_find_seg();
QUEUE       *ME_rem_seg();

FUNC_EXTERN BOOL GVosvers(char *OSVersionString);

# ifdef MCT
GLOBALREF       CS_SEMAPHORE    NM_loc_sem;
GLOBALREF       CS_SEMAPHORE    ME_segpool_sem;
# endif /* MCT */

/******************************************************************************
** Definition of all global variables owned by this file.
******************************************************************************/

GLOBALREF   QUEUE ME_segpool;

/******************************************************************************
**  Definition of static variables and forward static functions.
******************************************************************************/

/******************************************************************************
** II_FILES subdirectory that is used by ME to create files to associate
** with shared memory segments.  These files are used by MEshow_pages to
** track existing shared memory segments.
******************************************************************************/

#define	ME_SHMEM_DIR	"memory"

/*
**  Name: ME_alloc_shared	- allocate shared memory
**
**  Description:
**	Maps a shared memory segment into the process, keyed by LOCATION.
**
**  Inputs:
**	flag			ME_CREATE_MASK to create the segment.
**				   ME_ADDR_SPEC if to a fixed location.
**	memory		place we want it to go, if ME_ADDR_SPEC.
**	id			   file key identifying the segment
**	pages			number of pages to create.
**
**  Outputs:
**	pages			number of pages attached.
**	memory		place it ended up.
**	err_code		error description
**
**	Returns:
**	    OK or status from called routines.
**
**	Exceptions:
**	    Size of the requested segment is limited to 4 gbytes (32 bits).
**     This could be changed, but why?
**
**  Side Effects:
**	    File described by the location may be created or opened.
**
**  History:
**      20-jul-95 (reijo01)
**          Fix SETCLOS32ERR macro
**      06-Jun-96 (fanra01)
**          b76724 Changed to use SETWIN32ERR instead of SETCLERR when mapped
**          file fails.
**      13-Jan-97 (fanra01)
**          Initialise name so that on opening a file mapping the value stored
**          in ME_reg_seg is not random causing a CloseHandle call on a random
**          value during MEshared_free.
**      04-Apr-97 (fanra01)
**          Bug 81339
**          Modified the ME_alloc_shared function to return the number of pages
**          in the shared file if the ME_SSHARED_MASK or ME_MSHARED_MASK flags
**          are passed by getting shared file info and calculating size.
**      01-oct-97 (chudu01)
**          To avoid conflicts between Jasmine and Ingres installations, we
**          need to use a unique key.  The key parameter is used as the key 
**          for CreateFileMapping() and OpenFileMapping().  Previously both
**          Ingres and Jasmine would use the same key.  Since these mappings
**          can be shared between processes, the second server would end up
**          with a file handle that belongs to the first server.
**          By appending the SystemVarPrefix and installation code to the key
**          we can be assured of a unique key.
**      19-Aug-98 (fanra01)
**          Rearranged the order of registering a memory segment until after
**          we have determined the number of pages allocated.  An attach to
**          shared memory is allowed to give 0 as the number of requested
**          pages.
**          A page number of 0 would causes the ME_find_seg not to find the
**          segment as start and end addresses are the same.
**	29-mar-2002 (somsa01)
**	    Close the memory file's handle as appropriate.
**	07-oct-2003 (somsa01)
**	    Updated call to CreateFileMapping() to properly account for
**	    sizes greater than 2GB on 64-bit systems.
**      27-Sep-2005 (fanra01)
**          Add security attribute to the creation of the file mapping.
**	06-Dec-2006 (drivi01)
**	    Adding support for Vista, Vista requires "Global\" prefix for
**	    shared objects as well.  Replacing calls to GVosvers with 
**	    GVshobj which returns the prefix to shared objects.
*/

STATUS
ME_alloc_shared(i4          flag,
                SIZE_TYPE   pages,
                char        *key,
                PTR         *memory,
                SIZE_TYPE   *allocated_pages,
                CL_ERR_DESC *err_code)
{
	STATUS          status;
	SIZE_TYPE       memsize;
#ifdef LP64
	LARGE_INTEGER	numbytes;
#endif
	HANDLE          name;
	HANDLE          map;
	PTR             temp;
	char            map_key[MAX_LOC+1];
	char            *install_code;
	char		*ObjectPrefix;

    SECURITY_ATTRIBUTES sa;
    
	CLEAR_ERR(err_code);
	GVshobj(&ObjectPrefix);

	if (key == NULL || *key == '\0') {
		return (ME_BAD_PARAM);
	}
	memsize = pages * ME_MPAGESIZE;
        /*
        **  Moved ME_makekey to be called each time ME_alloc_shared is called
        **  as this obtains a handle to the file which will be required later
        **  if this is an attach to shared memory.
        **  This file handle is closed during an MEshared_free.
        */
        if ((name = ME_makekey(key)) == (HANDLE) -1)
        {
            status = GetLastError();
            SETWIN32ERR(err_code, status, ER_alloc);
            return (FAIL);
        }

	/* 
	**  The file mapping key used to be the name of the file.
	**  This caused problems when Jasmine and Ingres were installed
	**  on the same machine.  Create a unique key name, and use
	**  that for File Mapping instead.
	*/

	NMgtAt("II_INSTALLATION", &install_code);
    STpolycat(4, ObjectPrefix, SystemVarPrefix, install_code,
		      key, map_key);

	if (flag & ME_CREATE_MASK) {
	        iimksecdacl( &sa );
		FlushFileBuffers(name);
#ifdef LP64
		numbytes.QuadPart = Int32x32To64(pages, ME_MPAGESIZE);
		map = CreateFileMapping(name,
		                        &sa,
		                        PAGE_READWRITE,
		                        numbytes.HighPart,
		                        numbytes.LowPart,
		                        map_key);
#else
		map = CreateFileMapping(name,
		                        &sa,
		                        PAGE_READWRITE,
		                        0,
		                        memsize,
		                        map_key);
#endif  /* LP64 */

		if (map == NULL) {
			status = GetLastError();
			SETWIN32ERR(err_code, status, ER_alloc);
			FlushFileBuffers(name);
			CloseHandle(name);
			switch (status) {
			case ERROR_ALREADY_EXISTS:
				return ME_ALREADY_EXISTS;
			case ERROR_NOT_ENOUGH_MEMORY:
				return ME_OUT_OF_MEM;
			default:
				return FAIL;
			}
		}

		if (map != NULL && GetLastError() == ERROR_ALREADY_EXISTS) {
			FlushFileBuffers(name);
			CloseHandle(map);
			CloseHandle(name);
			return (ME_ALREADY_EXISTS);
		}
	} else {
		map = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE,
		                      FALSE,
		                      map_key);

		if (map == NULL) {
			status = GetLastError();
			SETWIN32ERR(err_code, status, ER_alloc);
			FlushFileBuffers(name);
			CloseHandle(name);
			switch (status) {
			case ERROR_FILE_NOT_FOUND:
				return ME_NO_SUCH_SEGMENT;
			case ERROR_NOT_ENOUGH_MEMORY:
				return ME_OUT_OF_MEM;
			default:
				return FAIL;
			}
		}
	}

	/*
	 * Finally.  Now get a memory address for the sucker.
	 * 
	 * If ME_ADDR_SPEC is set, we'll attempt to hardwire the address; else
	 * we'll take whatever the system gives us.
	 */

	if (flag & ME_ADDR_SPEC) {
		temp = MapViewOfFileEx(map,
		                       FILE_MAP_WRITE | FILE_MAP_READ,
		                       0,
		                       0,
		                       0,
		                       *memory);
		if ((temp == NULL) || (temp != *memory)) {
			status = GetLastError();
			SETWIN32ERR(err_code, status, ER_alloc);
			return (FAIL);
		}
	} else {
		*memory = MapViewOfFile(map,
		                        FILE_MAP_WRITE | FILE_MAP_READ,
		                        0,
		                        0,
		                        0);
		if (*memory == NULL) {
			status = GetLastError();
			SETWIN32ERR(err_code, status, ER_alloc);
			return (FAIL);
		}
	}

        /*
        **  If this is not an attach to shared memory assume that pages value
        **  is valid.
        */
        if ((flag & ME_CREATE_MASK) ||
            !(flag & (ME_SSHARED_MASK | ME_MSHARED_MASK)))
        {
	    pages = (i4)(memsize / ME_MPAGESIZE);
        }
        else
        {
        BY_HANDLE_FILE_INFORMATION sFileInfo;

            /*
            **  If attaching to shared memory ignore the page argument and
            **  calculate size of shared segment in pages from file size.
            **  Assume that a single shared memory file will not exceed 4G.
            */

            if (GetFileInformationByHandle (name, &sFileInfo) == 0)
            {
                status = GetLastError();
                SETWIN32ERR(err_code, status, ER_alloc);
                return (FAIL);
            }
            else
            {
                pages = sFileInfo.nFileSizeLow / ME_MPAGESIZE;
            }
        }


	if (allocated_pages)
		*allocated_pages = pages;

        /*
        **  if this is an attach where pages is 0 ME_reg_seg will register
        **  pages calculated from the size of the file.
        */
	if (ME_reg_seg(*memory,
	               pages,
	               map,
	               name) != OK) {
		UnmapViewOfFile(memory);
		*memory = (PTR) NULL;
		return (FAIL);
	}

	return (OK);
}

/******************************************************************************
** Name: MEsmdestroy()	- Destroy a shared memory segment
**
** Description:
**	Remove the shared memory segment specified by shared memory identifier
**	"key" from the system and destroy any system data structure associated
**	with it.
**
**	Note: The shared memory pages are not necessarily removed from
**	processes which have the segment mapped.  It is up to the clients to
**	detach the segment via MEfree_pages prior to destroying it.
**
**	Protection Note: The caller of this routine must have protection to
**	destroy the shared memory segment.  Protections are enforced by the
**	underlying Operating System.  In general, this routine can only be
**	guaranteed to work when executed by a user running with the same
**	effective privledges as the user who created the shared memory segment.
**
** Inputs:
**	user_key			identifier which was previosly
**				      used in a successful MEget_pages() call
**				      (not necessarily a call in this process)
**
** Outputs:
**	err_code 		System specific error information.
**
**	Returns:
**	    OK
**	    ME_NO_PERM		    No permission to destroy shared memory
**				            segment.
**	    ME_NO_SUCH_SEGMENT	indicated shared memory segment does not
**				            exist.
**	    ME_NO_SHARED	    No shared memory in this CL.
**	    ME_BAD_ADVICE	    call was made during ME_USER_ALLOC
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
******************************************************************************/
STATUS
MEsmdestroy(char        *user_key,
            CL_ERR_DESC *err_code)
{
	LOCATION        location;
	LOCATION        temp_loc;
	char            loc_buf[MAX_LOC + 1];
	STATUS          ret_val = OK;

	CLEAR_ERR(err_code);

	/*
	 * Get location of ME files for shared memory segments.
	 */

# ifdef MCT
	gen_Psem(&NM_loc_sem);
# endif /* MCT */
	ret_val = NMloc(FILES,
	                PATH,
	                (char *) NULL,
	                &temp_loc);
	if (!ret_val)
	{
		LOcopy(&temp_loc,
		       loc_buf,
		       &location);
		LOfaddpath(&location,
		           ME_SHMEM_DIR,
		           &location);
		LOfstfile(user_key,
		          &location);

		if (LOexist(&location) != OK)
			ret_val = FAIL;
		else {
			ret_val = LOdelete(&location);
			switch (ret_val) {
			case OK:
			case ME_NO_PERM:
				break;;

			default:
				ret_val = ME_NO_SUCH_SEGMENT;
				break;;
			}
		}
	}
# ifdef MCT
	gen_Vsem(&NM_loc_sem);
# endif /* MCT */
	return (ret_val);
}

/*****************************************************************************
** Name: MEshared_free()	- Free shared memory
**
** Description:
**	Free a region of shared memory and return new region for potential
**	futher freeing of the break region.
**
**	If there is attached shared memory in the region being
**	freed then those pages should only be marked free if the whole
**	segment can be detached.
**
**	There is a small table of attached segments which is scanned.
**
** Inputs:
** addr				address of region
**	pages				number of pages to check
**
** Outputs:
**	addr				new address of region
**	pages				new number of pages
**	err_code			CL_ERR_DESC
**
**	Returns:
**	    OK
**	    ME_NOT_ALLOCATED
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	07-apr-1997 (canor01)
**	    When the shared memory is attached to existing memory
**	    the file handle will be null, so don't try to close it.
**	10-apr-1997 (canor01)
**	    If the address of the memory to be freed does not fall
**	    within the shared memory range, return ME_NOT_ALLOCATED.
**
*****************************************************************************/
STATUS
MEshared_free(PTR         *addr,
              SIZE_TYPE          *pages,
              CL_ERR_DESC *err_code)
{
	STATUS			status = OK;
	register ME_SEG_INFO	*seginf;
	char			*lower, *upper, *last;
	SIZE_TYPE		off, len;
	QUEUE			*next_queue;

	CLEAR_ERR(err_code);

	lower = (char *) *addr;
	upper = lower + ME_MPAGESIZE * *pages;
	last = NULL;

	gen_Psem(&ME_segpool_sem);
	seginf = ME_find_seg(lower, upper, &ME_segpool);
	if ( seginf == NULL )
	{
	    /* memory address was not within shared memory range */
	    gen_Vsem(&ME_segpool_sem);
	    return( ME_NOT_ALLOCATED );
	}
	for ( ;
	     seginf;
	     seginf = ME_find_seg(lower,
	                          upper,
	                          next_queue)) {

		next_queue = &seginf->q;

		if (last && last != seginf->eaddr) {
			status = ME_NOT_ALLOCATED;
			break;
		}

		last = seginf->addr;
		off  = 0;
		len  = seginf->npages;

		if (lower > seginf->addr) {
			off = (lower - seginf->addr) / ME_MPAGESIZE;
			len -= off;
		}

		if (upper < seginf->eaddr) {
			len -= (seginf->eaddr - upper) / ME_MPAGESIZE;
		}

		if (MEalloctst(seginf->allocvec,
		               (i4)off,
		               (i4)len,
		               TRUE)) {
			status = ME_NOT_ALLOCATED;
			break;
		}

		MEclearpg(seginf->allocvec, (i4)off, (i4)len);

		if (!MEalloctst(seginf->allocvec,
		                0,
		                seginf->npages,
		                FALSE)) {
			/* detach segment */
			/*
			 * WARNING::: if the address is NOT the BASE of a
			 * shared memory segment obtained from MapViewOfFile,
			 * we're either gonna fail or puke. Good luck!
			 */

			status = UnmapViewOfFile(seginf->addr);

			if (status == FALSE) {
				status = GetLastError();
				SETCLOS2ERR(err_code, status, ER_mmap);
				gen_Vsem(&ME_segpool_sem);
				return (ME_BAD_PARAM);
			}

			status = CloseHandle(seginf->mem_handle);

			if (status == FALSE) {
				status = GetLastError();
				SETCLOS2ERR(err_code, status, ER_close);
				gen_Vsem(&ME_segpool_sem);
				return (ME_BAD_PARAM);
			}

			/*
			** if just attaching to existing memory,
			** there will just be a memory handle, but
			** the file_handle will be NULL.
			*/
			if ( seginf->file_handle )
			{
			    FlushFileBuffers(seginf->file_handle);
			    status = CloseHandle(seginf->file_handle);
			}

			if (status == FALSE) {
				status = GetLastError();
				SETCLOS2ERR(err_code, status, ER_close);
				gen_Vsem(&ME_segpool_sem);
				return (ME_BAD_PARAM);
			} else {
				status = OK;
			}

			next_queue = ME_rem_seg(seginf);
			*addr = (PTR) NULL;
		}
	}
	gen_Vsem(&ME_segpool_sem);
	return status;
}

/*****************************************************************************
** Name: ME_makekey()	- Make WIN32 file for mapping
**                        shared memory from user key.
**
** Description:
**	Create a file to identify a shared memory segment.  The file's inode
**	will be used as a shared memory key.  The file's existence will be
**	used to track shared segments on the system.
**
** Inputs:
**	key			character key identifying shared segment.
**
** Outputs:
**	return value
**
**	Returns:
**	    HANDLE  to opened file, or
**	    -1      file could not be created.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    File created in installation's memory directory.
**
*****************************************************************************/
HANDLE
ME_makekey(char *user_key)
{
	LOCATION location;
	LOCATION temp_loc;
	char     loc_buf[MAX_LOC + 1];
	char	 *shmemname;

	STATUS   ret_val;
	HANDLE   handle;

	/*
	 * Build location to file in MEMORY directory. Get location of ME
	 * files for shared memory segments.
	 */

# ifdef MCT
	gen_Psem(&NM_loc_sem);
# endif /* MCT */
	ret_val = NMloc(FILES,
	                PATH,
	                (char *) NULL,
	                &temp_loc);
	if (ret_val)
	{
# ifdef MCT
		gen_Vsem(&NM_loc_sem);
# endif /* MCT */
		return (HANDLE) -1;
	}

	LOcopy(&temp_loc,
	       loc_buf,
	       &location);
# ifdef MCT
	gen_Vsem(&NM_loc_sem);
# endif /* MCT */
	LOfaddpath(&location,
	           ME_SHMEM_DIR,
	           &location);
	LOfstfile(user_key,
	          &location);

	/*
	 * Create file to identify the shared segment. This will also allow
	 * MEshow_pages to tell what shared segments have been created.
	 */

	LOtos(&location,
	      &shmemname);
	handle = CreateFile(shmemname,
	                    GENERIC_READ | GENERIC_WRITE,
	                    FILE_SHARE_READ | FILE_SHARE_WRITE,
	                    NULL,
	                    OPEN_ALWAYS,
	                    FILE_ATTRIBUTE_NORMAL,
	                    NULL);

	if (handle == INVALID_HANDLE_VALUE) {
		handle = (HANDLE) -1;
	}

	return (handle);
}

/******************************************************************************
** Name: MEshow_pages()	- show system's allocated shared memory segments.
**
** Description:
**	This routine is used by clients of the shared memory allocation
**	routines to show what shared memory segments are currently allocated
**	by the system.
**
**	The routine takes a function parameter - 'func' - that will be
**	called once for each existing shared memory segment allocated
**	by Ingres in the current installation.
**
**      The client specified function must have the following call format:
**(
**          STATUS
**          func(arg_list, key, err_code)
**          i4         *arg_list;       Pointer to argument list.
**          char       *key;		Shared segment key.
**          CL_SYS_ERR *err_code;       Pointer to operating system
**                                      error codes (if applicable).
**)
**
**	The following return values from the routine 'func' have special
**	meaning to MEshow_pages:
**
**	    OK (zero)	- If zero is returned from 'func' then MEshow_pages
**			  will continue normally, calling 'func' with the
**			  next shared memory segment.
**	    ENDFILE	- If ENDFILE is returned from 'func' then MEshow_pages
**			  will stop processing and return OK to the caller.
**
**	If 'func' returns any other value, MEshow_pages will stop processing
**	and return that STATUS value to its caller.  Additionally, the system
**	specific 'err_code' value returned by 'func' will be returned to the
**	MEshow_pages caller.
**
** Inputs:
**	func			Function to call with each shared segment.
**	arg_list		Optional pointer to argument list to pass
**				to function.
**
** Outputs:
**	err_code 		System specific error information.
**
**	Returns:
**	    OK
**	    ME_NO_PERM		No permission to show shared memory segment.
**	    ME_BAD_PARAM	Bad function argument.
**	    ME_NO_SHARED	No shared memory in this CL.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
******************************************************************************/
STATUS
//MEshow_pages(STATUS		(*func) (PTR, const char *, CL_SYS_ERR *),
MEshow_pages(STATUS		(*func) (),
             PTR        *arg_list,
             CL_SYS_ERR *err_code)
{
	LOCATION        locptr;
	LO_DIR_CONTEXT  lo_dir;
	STATUS          status;
	char            fname[LO_FILENAME_MAX + 1];
	char		dev[LO_DEVNAME_MAX + 1];
	char		path[LO_PATH_MAX + 1];
	char		fprefix[LO_FPREFIX_MAX + 1];
	char		fsuffix[LO_FSUFFIX_MAX + 1];
	char		version[LO_FVERSION_MAX + 1];

	/*
	 * Get location ptr to directory to search for memory segment names.
	 */

# ifdef MCT
	gen_Psem(&NM_loc_sem);
# endif /* MCT */
	if ((status = NMloc(FILES,
	                    PATH,
	                    (char *) NULL,
	                    &locptr)) == OK) {
		LOfaddpath(&locptr,
		           ME_SHMEM_DIR,
		           &locptr);
	} else {
# ifdef MCT
		gen_Vsem(&NM_loc_sem);
# endif /* MCT */
		return (status);
	}
# ifdef MCT
	gen_Vsem(&NM_loc_sem);
# endif /* MCT */

	/*
	 * For each file in the memory location, call the user supplied
	 * function with the filename.  Shared memory keys are built from
	 * these filenames.
	 */

	status = LOwcard(&locptr,
	                 (char *) NULL,
	                 (char *) NULL,
	                 (char *) NULL,
	                 &lo_dir);

	if (status != OK) {
		LOwend(&lo_dir);
		return (ME_NO_SHARED);
	}

	while (status == OK) {
		LOdetail(&locptr, dev, path, fprefix, fsuffix, version);
		(VOID) STpolycat(3, fprefix, ".", fsuffix, fname);
		if (~lo_dir.find_buffer.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			status = (*func) (arg_list,
			                  fname,
			                  err_code);
			if (status != OK) {
				break;
			}
		}
		status = LOwnext(&lo_dir, &locptr);
	}

	LOwend(&lo_dir);

	if (status == ENDFILE) {
		return (OK);
	}

	return (status);
}
