SQLSTATE_MAPPING_REQUIRED

/*
** Copyright (c) 1992, 2009 Ingres Corporation	
**
**
**    Message file for Facility code: _CL
**
** History:
**
**      4-sep-1992 (mgw) Bug #46478
**              Added UT error messages that were called from Unix, but not
**              VMS. Also made a couple of other UT messages more informative.
**      4-oct-1992 (stevet)
**              Added E_CL1F05 to E_CL1F0A timezone related error messages.
**	11-Nov-92 (mrelac)
**		Added MPE/iX-specific ZZ error messages.  They range from
**		E_CL7801 to E_CL7974.
**      9-dec-1992 (stevet)
**		Added PM related timezone error E_CL1F0B - E_CL1F0E.	
**	02-dec-1992 (kwatts)
**	        Added Smart Disk error messages. Also moved the SL messages
**		to their correct number (29).
**      3-feb-1993 (bryanp)
**		Corrected text of CL1038 (LKevent => LKrelease)
**      16-feb-1993 (dianeh)
**              Added NO_SQLSTATE_MAPPING to end of file.
**	16-apr-1993 (bryanp)
**		Corrected text of CL1024 and CL1032.
**	24-Apr-1993 (wolf, for fredb) 
**		Added another MPE specific ZZ tracing message, #7962, same
**		as #7960 except it is for SIopen instead of SIfopen.
**	30-apr-1993 (cal)
**		Add E_CL1057_LK_EXCEED_LIIMIT for LK startup parameter check.
**	02-jul-1993 (mikem)
**		Added E_CL1F0F_TM_NOSUPPORT for TMprfstat() to return when 
**		routine is not supported on a particular platform.
**	26-jul-1993 (bryanp)
**		Added DI_SLAVE_INIT_ERROR for reporting failure to start slaves.
**	24-sep-1993 (pearl)
**	    	Added E_CL2545_CS_FREE_EVENT and E_CL2544_CS_CAUSE_EVENT for
**		CScause_event() and CSfree_event() failure (6.4).
**	28-sep-93 (stephenb)
**		Changed SQLSTATE status code for E_CL2545_CS_FREE_EVENT and 
**		E_CL2544_CS_CAUSE_EVENT from E_GE98BC_OTHER_ERROR to 
**		SS50000_MISC_ERRORS.
**	04-oct-93 (dianeh)
**		Clean up text destined for messages.txt.
**	13-oct-93 (seiwald)
**	    	CS option revamp: defined new error messages for errors 
**		that were previously SIprintf'ed: E_CL252D_CS_DEFINE, 
**		E_CL252E_CS_SHORT_FDS.
**	3-dec-1993 (andyw)
**	        Modified E_CL0F28_LG_WRONG_VERSION to reflect the new
**	        configuration tool for initialisation of the log file.
**	31-jan-94 (jnash)
**	    	Add new CSinitiate() errors: E_CL2535_CS_GETPWNAM_FAIL,
**	    	E_CS2536_SETEUID_FAIL, E_CS2537_MAP_SSEG_FAIL,
**	    	E_CS2538_CS_EVINIT_FAIL.
**	25-mar-94 (dkh)
**		Added EXsetclient error: E_CL0AA0_EXSETCLIENT_LATE.
**      11-April-1994 (jpk)
**	    Change II_AUTHORIZATION references to II_AUTH_STRING.
**	20-apr-94 (mikem)
**	        Added new error from DIlistdir(): E_CL0618_DI_OPENDIR_FAILED.
**	23-may-1994 (bryanp)
**	    Add E_CL030E_CK_TEMPLATE_MISSING for missing cktmpl.def file.
**	06-apr-1995 (medji01)
**	    Removed 'instance' from E_CL1026, E_CL102B, E_CL1034, E_CL1037,
**	    E_CL1039, E_CL1029, E_CL103E, and E_CL1041.
**	14-nov-1995 (nick)
**	    Add E_CL1F10_TM_BADDATECUT.
**	05-mar-1996 (emmag)
**	    Change E_CL0910_ER_UNIXERROR:SS5000B_INTERNAL_ERROR to report
**	    an operating system error rather than a UNIX error.
**	03-jun-1996 (canor01)
**	    add async io messages:
**	    - 01-may-1995 (amo ICL)
**	       Added ANB's asyncio amendments
**	19-jun-1996 (sweeney)
**	    Fix typos.
**	12-sep-96 (nick)
**	    Add back in E_CL1F10_TM_BADDATECUT which was abducted into the 
**	    ether several months after its creaetion.
**      09-oct-96 (mcgem01)
**          Remove hard-coded Ingres references so that these
**          messages can be shared by both Ingres and Jasmine.
**	01-Aug-1996 (nanpr01)
**	    Added new lock callback error codes and description.
**	02-sep-1997 (canor01)
**	    Added E_CL061B_DI_ACCESS.
**	27-oct-1997 (canor01)
**	    Add E_CL1628_PC_WT_EXEC. Clean up references to Ingres. Make
**	    PC out of memory error clearer.
**	07-nov-1997 (canor01)
**	    Add E_CL2547_CS_SHUT_STATE
**	06-may-1998 (canor01)
**	    Add messages corresponding to return values from ca_license_check.
**	27-may-1998 (canor01)
**	    Add E_CL2660_CI_LICENSE_ERR.
**	02-sep-98 (mcgem01)
**	    All messages returned from the CA Licensing will have a
**	    "Computer Associates Licensing" prefix, so there's no need
**	    for us to have one also.
**	02-oct-1998 (somsa01)
**	    Added E_CL061C_DI_NODISKSPACE.
**	06-oct-1998 (somsa01)
**	    Added E_CL030F_CK_COMMAND_ERROR.  (SIR #93752)
**	13-nov-1998 (canor01)
**	    Synchronize Ingres/Rosetta.
**	29-dec-1998 (canor01)
**	    Add messages related to CA Licensing check of site ids.
**	11-jan-1999 (mcgem01)
**	    Add text for a number of CL messages which were previously
**	    undefined.
**	11-jan-1999 (canor01)
**	    Add more previously undefined messages.
**	12-jan-1999 (canor01)
**	    Synchronize messages with ercl.h.
**	17-mar-1999 (mcgem01)
**	    Replace references to the I&O Guide with the Getting Started
**	    Guide and the Command Reference Guide.
**	27-nov-2000 (stephenb)
**	    Fix message for E_CL0617_DI_SLAVE_INIT_ERROR, it was displaying
**	    string variables for the path to iislave, which were not passed
**	    and are not available anyway when the message is logged.
**	27-mar-2001 (devjo01)
**	    Add CX facility messages E_CL2Cxx.
**	28-Feb-2002 (jenjo02)
**	    Added E_CL1058_LK_REQUEST_RETRIED, added llb_status to
**	    E_CL103E, E_CL1037.
**      04-Oct-2002 (hanch04)
**          Added E_CL2557_CS_INTRN_THRDS_NOT_SUP
**	07-Feb-2003 (jenjo02)
**	    Added E_CL1015_LK_UBUSY (LK_UBUSY).
**	17-Dec-2003 (jenjo02)
**	    Added E_CL106D, E_CL106E, E_CL106F, E_CL1070
**	    for LKconnect().
**	13-jan-2004 (somsa01)
**	    Added E_CL140B_NM_STCORRUPT (corrupt symbol.tbl file).
**	03-feb-2004 (somsa01)
**	    backed out last change for now.
**	27-mar-2004 (devjo01)
**	    Provided clearer text & documentation for errors
**	    E_CL2C33_CX_E_OS_NOINIT and E_CL2C35_CX_E_OS_MISC_ERR.
**	06-apr-2004 (somsa01)
**	    Added E_CL140B_NM_STCORRUPT (corrupt symbol.tbl file).
**	02-nov-2004 (devjo01)
**	    Added E_CL2C2A_CX_E_DLM_RD_DIRTY.  Corrected spelling
**	    error in E_CL2C2C_CX_E_MSG_NOSUCH (retreive)
**	21-Feb-2005 (thaju02)
**	    Modified E_CL1226 to display unsigned size.
**      20-Jun-2005 (hanal04) Bug 114700 INGSRV3337
**          Ingres message file cleanup.
**  05-Jul-2005 (fanra01)
**      Bug 114804
**      Add E_CL110B, E_CL110C, E_CL110D and E_CL110E.
**      27-Jul-2004 (inifa01) changed in connection to bug 110479 (INGREP144).
**          Changed mapping of E_CLFE07 from SS50000_MISC_ERROR to
**          SS08006_CONNECTION_FAILURE.
**	26-Jun-2006 (jonj)
**	    Add CL0F2E_LG_DUPLICATE_TXN, CL0F2F_LG_NO_TRANSACTION.
**      27-dec-06 (kibro01)
**          Change incorrect '%s' to '%<n>c' in error messages
**	05-Jan-2007 (bonro01)
**	    Change Getting Started Guide to Installation Guide.
**	25-apr-2007 (abbjo03)
**	    Update E_CL25FD_CS_THREAD_STK_OVFL to reflect stack_size is a
**	    config.dat parameter.
**      31-May-2007 (wonca01) BUG# 118407
**          Modify E_CL060D_DI_BADDELETE to contain the directory that failed
**          to be deleted.
**      18-Jul-2007 (hweho01)
**          Correct the spelling error in CL121D message.
**	08-Aug-2007 (drivi01)
**	    Added E_CL2D01_NO_PRIVILEGE for Vista port.
**	07-Nov-2007 (jonj)
**	    Added missing E_CL1016_LK_NOTHELD
**	14-jan-2009 (joea)
**	    Add E_CL1F11_TM_GETTIME_ERR.
**      16-Jun-2009 (hanal04) Bug 122117
**          Added E_CL1071_LK_FORCEABORT_ALTER and E_CL1072_LK_FORCEABORT_ALTER
**	29-Sep-2009 (thaju02)
**	    Specify object type in E_CL102D.
**      09-mar-2010 (joea)
**          Add E_CL25F5_KPB_ALLOC_ERROR and E_CL25F6_MEM_STK_ALLOC_ERROR.
*/


#define _CL_CLASS   1


E_CL0301_CK_BAD_PARAM:SS50000_MISC_ERRORS
"Bad parameter passed to a CK routine"
E_CL0302_CK_SPAWN:SS08004_CONNECTION_REJECTED
"Could not spawn a subprocess to perform backup"
E_CL0303_CK_BACKUP:SS50000_MISC_ERRORS
"Unexpected backup error received from operating system"
E_CL0304_CK_CMKRNL:SS08004_CONNECTION_REJECTED
"Could not enter change mode to kernel to perform backup"
E_CL0305_CK_NOTFOUND:SS50000_MISC_ERRORS
"Checkpoint object could not be found."
E_CL0306_CK_BADDELETE:SS08004_CONNECTION_REJECTED
"Error deleting file or directory"
E_CL0307_CK_BADPATH:SS50000_MISC_ERRORS
"The path specified was in error."
E_CL0308_CK_ENDFILE:SS50000_MISC_ERRORS
"End of file"
E_CL0309_CK_BADLIST:SS50000_MISC_ERRORS
"Error listing objects in a directory"
E_CL030A_CK_FILENOTFOUND:SS50000_MISC_ERRORS
"The file specified could not be found"
E_CL030B_CK_DIRNOTFOUND:SS50000_MISC_ERRORS
"The checkpoint directory could not be found"
E_CL030C_CK_BADDIR:SS50000_MISC_ERRORS
"Error verifying directory"
E_CL030D_CK_EXISTS:SS50000_MISC_ERRORS
"The object specified already exists"
E_CL030E_CK_TEMPLATE_MISSING:SS50000_MISC_ERRORS
"The checkpoint template file (cktmpl.def) could not be opened. Check the
existence and permissions on the cktmpl.def file in the FILES subdirectory
of the II_SYSTEM subtree. If an alternate template file has been specified
via the II_CKTMPL_FILE parameter, check the existence and permissions of
the alternate template file."
E_CL030F_CK_COMMAND_ERROR:SS50000_MISC_ERRORS
"The operating system command used to perform the database checkpoint
returned an error of %0d. The command was '%1c'."
E_CL0310_CK_OWNNOTING:SS50000_MISC_ERRORS
"The checkpoint template file must be owned by the user ingres."
E_CL2A01_CM_NOCHARSET:SS50000_MISC_ERRORS
"Specified character set does not exist"
E_CL0501_CV_SYNTAX:SS50000_MISC_ERRORS
"Bad syntax in ascii form of number"
E_CL0502_CV_UNDERFLOW:SS22003_NUM_VAL_OUT_OF_RANGE
"Number underflow in the conversion"
E_CL0503_CV_OVERFLOW:SS22003_NUM_VAL_OUT_OF_RANGE
"Number overflow in the conversion"
E_CL0504_CV_TRUNCATE:SS22001_STRING_RIGHT_TRUNC
"Output too wide for space passed in"
E_CL0601_DI_BADPARAM:SS50000_MISC_ERRORS
"Bad parameter(s) passed to routine"
E_CL0602_DI_BADFILE:SS50000_MISC_ERRORS
"Illegal file control block pointer"
E_CL0603_DI_ENDFILE:SS50000_MISC_ERRORS
"End of file"
E_CL0604_DI_BADOPEN:SS08004_CONNECTION_REJECTED
"Error trying to open file"
E_CL0605_DI_BADCLOSE:SS08004_CONNECTION_REJECTED
"Error trying to close file"
E_CL0606_DI_BADWRITE:SS08004_CONNECTION_REJECTED
"Error writing page to disk"
E_CL0607_DI_BADREAD:SS08004_CONNECTION_REJECTED
"Error reading page from disk"
E_CL0608_DI_BADEXTEND:SS08004_CONNECTION_REJECTED
"Error allocating disk space"
E_CL0609_DI_BADDIR:SS08004_CONNECTION_REJECTED
"Error verifying directory"
E_CL060A_DI_BADINFO:SS08004_CONNECTION_REJECTED
"Error seeking to page in file"
E_CL060B_DI_BADSENSE:SS08004_CONNECTION_REJECTED
"Error reading file attributes"
E_CL060C_DI_BADCREATE:SS08004_CONNECTION_REJECTED
"Error creating file or directory"
E_CL060D_DI_BADDELETE:SS08004_CONNECTION_REJECTED
"Error deleting file or directory '%0c'"
E_CL060E_DI_BADRNAME:SS08004_CONNECTION_REJECTED
"Error renaming file"
E_CL060F_DI_EXCEED_LIMIT:SS08004_CONNECTION_REJECTED
"File resource quota exceeded"
E_CL0610_DI_DIRNOTFOUND:SS50000_MISC_ERRORS
"The directory pointed to path could not be found"
E_CL0611_DI_FILENOTFOUND:SS50000_MISC_ERRORS
"The file specified could not be found"
E_CL0612_DI_EXISTS:SS50000_MISC_ERRORS
"The object specified already exists"
E_CL0613_DI_BADLIST:SS50000_MISC_ERRORS
"Error listing objects in a directory"
E_CL0614_DI_INPROGRESS:SS50000_MISC_ERRORS
"A session waiting for a disk I/O operation was resumed before the
disk operation actually completed."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A session waiting for a disk I/O operation was resumed before the
**	disk operation actually completed.  This error can result from
**	an incorrect CSresume() being generated waking up a CSsuspend()
**	in the DI module.
**
** System Status:
**	All queries encountering this error will fail.
**
** Recommendation:
**	This error message indicates a software problem, and it should
**	be reported to Ingres Corporation Technical Support as a bug.
*/
E_CL0615_DI_BADRELEASE:SS50000_MISC_ERRORS
"An attempt to release a disk asynchronous completion control block failed."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A session after completing a disk I/O operation failed when attempting
**	to release a slave control block.  This error can occur if a session
**	is incorrectly resumed before a slave has completed its work and the
**	free code finds the slave control block "in use".
**
** System Status:
**	All queries encountering this error will fail.
**
** Recommendation:
**	This error message indicates a software problem, and it should
**	be reported to Ingres Corporation Technical Support as a bug.
*/
E_CL0616_DI_GENERAL_ERR:SS50000_MISC_ERRORS
"Error occurred trying to send an event to a slave process, see internal 
error for more information."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A session encountered an error while trying to send an event to
**	a slave process.
**
** System Status:
**	All queries encountering this error will fail.
**
** Recommendation:
**	This error message indicates a software problem, and it should
**	be reported to Ingres Corporation Technical Support as a bug.
*/
E_CL0617_DI_SLAVE_INIT_ERROR:SS50000_MISC_ERRORS
"Unable to start slave processes. An error occurred while the server was
attempting to connect to the system segment, allocate a server segment, and
fork some number of slave processes. The system may be low on resources, or
there may be a permissions or configuration problem.  Check shared memory 
resources, swapfile resources, process resources, semaphore resources, and 
permissions on $II_SYSTEM/ingres/bin/iislave."
/*%
** Parameters:
**    0    first 64 characters of path name passed to opendir().
**
** Explanation:
**	
**	A session encountered an error while calling the system
**	dependent interface to open a directory.   This call is
**	used to check the existence of a directory and to list
**	contents of the directory.  Usual errors are that the given
**	directory does not exist, or that for some reason permissions
**	on the directory or the path to the directory deny access for
**	the program attempting to access the directory.
**
** System Status:
**	All queries encountering this error will fail.
**
** Recommendation:
**	Check that directory exists and that permissions on the directory
**	and components of the directory seem reasonable.
*/
E_CL0618_DI_OPENDIR_FAILED:SS50000_MISC_ERRORS
"Error encountered while calling opendir() with the following path argument: 
(%0c).  Check that directory exists and that permissions on all parts of path 
seem reasonable (ie. most programs run by the dbms require that the %1c 
user id be able to access the given file locations)."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The server was attempting to fork slave processes to perform I/O
**	processing, but failed. The failure may have been due to system resource
**	shortage, or to a configuration problem. Verify that the system
**	has sufficient resources (swap space, process slots, semaphores, etc.)
**	to fork the slave processes. Check the shared memory, using
**	csreport. Check II_NUM_SLAVES to see how many slaves were being forked.
**
** System Status:
**	The server will fail to start.
**
** Recommendation:
**	Correct the configuration problem or resource shortage and restart the
**	server.
*/
E_CL0619_DI_MEREQMEM_ERR:SS50003_EXHAUSTED_RESOURCE
"DI: Error requesting memory"
E_CL061A_DI_NO_AIO_RESOURCE:SS50003_EXHAUSTED_RESOURCE
"Out of Asynchronous resources"
E_CL061B_DI_ACCESS:SS50000_MISC_ERRORS
"User does not have adequate file permissions to open a file.
The file may be marked read-only while a read-write open request was made."
E_CL061C_DI_NODISKSPACE:SS08004_CONNECTION_REJECTED
"There is no more disk space on the physical disk or partition."
E_CL0620_DILRU_RELEASE_ERR:SS50000_MISC_ERRORS
"Error releasing DI event control block"
/*%
** Parameters:
**	None.
**
** Explanation:
**	A session encountered an error while using the DIlru file cache.
**	See the internal error for more information on why this call 
**	failed. If the internal error was a severe error, for example
**	failing to release a semaphore, the server will but shut down
**	to prevent possible corruption.
**
** System Status:
**	All queries encountering this error will fail.
**
** Recommendation:
**	This error message indicates a software problem, and it should
**	be reported to Ingres Corporation Technical Support as a bug.
*/
E_CL0621_DILRU_CLOSE_ERR:SS50000_MISC_ERRORS
"Error closing a DI file in the DIlru file cache"
/*%
** Parameters:
**	None.
**
** Explanation:
**	A session encountered an error while using the DIlru file cache.
**	See the internal error for more information on why this call 
**	failed. If the internal error was a severe error, for example
**	failing to release a semaphore, the server will but shut down
**	to prevent possible corruption.
**
** System Status:
**	All queries encountering this error will fail.
**
** Recommendation:
**	This error message indicates a software problem, and it should
**	be reported to Ingres Corporation Technical Support as a bug.
*/
E_CL0622_DILRU_OPEN_ERR:SS50000_MISC_ERRORS
"Error opening a file in the DIlru file cache" 
/*%
** Parameters:
**	None.
**
** Explanation:
**	A session encountered an error while using the DIlru file cache.
**	See the internal error for more information on why this call 
**	failed. If the internal error was a severe error, for example
**	failing to release a semaphore, the server will but shut down
**	to prevent possible corruption.
**
** System Status:
**	All queries encountering this error will fail.
**
** Recommendation:
**	This error message indicates a software problem, and it should
**	be reported to Ingres Corporation Technical Support as a bug.
*/
E_CL0623_DILRU_INIT_ERR:SS50000_MISC_ERRORS
"Error initializing the DIlru file cache"
/*%
** Parameters:
**	None.
**
** Explanation:
**	A session encountered an error while using the DIlru file cache.
**	See the internal error for more information on why this call 
**	failed. If the internal error was a severe error, for example
**	failing to release a semaphore, the server will but shut down
**	to prevent possible corruption.
**
** System Status:
**	All queries encountering this error will fail.
**
** Recommendation:
**	This error message indicates a software problem, and it should
**	be reported to Ingres Corporation Technical Support as a bug.
*/
E_CL0624_DILRU_FREE_ERR:SS50000_MISC_ERRORS
"Error trying to free space in the DIlru file cache"
/*%
** Parameters:
**	None.
**
** Explanation:
**	A session encountered an error while using the DIlru file cache.
**	See the internal error for more information on why this call 
**	failed. If the internal error was a severe error, for example
**	failing to release a semaphore, the server will but shut down
**	to prevent possible corruption.
**
** System Status:
**	All queries encountering this error will fail.
**
** Recommendation:
**	This error message indicates a software problem, and it should
**	be reported to Ingres Corporation Technical Support as a bug.
*/
E_CL0625_DILRU_INPROC_ERR:SS50000_MISC_ERRORS
"Error while ensuring that a file exits in the DIlru file cache"
/*%
** Parameters:
**	None.
**
** Explanation:
**	A session encountered an error while using the DIlru file cache.
**	See the internal error for more information on why this call 
**	failed. If the internal error was a severe error, for example
**	failing to release a semaphore, the server will but shut down
**	to prevent possible corruption.
**
** System Status:
**	All queries encountering this error will fail.
**
** Recommendation:
**	This error message indicates a software problem, and it should
**	be reported to Ingres Corporation Technical Support as a bug.
*/
E_CL0626_DILRU_UNIQ_ERR:SS50000_MISC_ERRORS
"Error assigning a unique id to a file in the DIlru file cache"
/*%
** Parameters:
**	None.
**
** Explanation:
**	A session encountered an error while using the DIlru file cache.
**	See the internal error for more information on why this call 
**	failed. If the internal error was a severe error, for example
**	failing to release a semaphore, the server will but shut down
**	to prevent possible corruption.
**
** System Status:
**	All queries encountering this error will fail.
**
** Recommendation:
**	This error message indicates a software problem, and it should
**	be reported to Ingres Corporation Technical Support as a bug.
*/
E_CL0627_DILRU_FLUSH_ERR:SS50000_MISC_ERRORS
"Error flushing files from the DIlru file cache to free up space"
/*%
** Parameters:
**	None.
**
** Explanation:
**	A session encountered an error while using the DIlru file cache.
**	See the internal error for more information on why this call 
**	failed. If the internal error was a severe error, for example
**	failing to release a semaphore, the server will but shut down
**	to prevent possible corruption.
**
** System Status:
**	All queries encountering this error will fail.
**
** Recommendation:
**	This error message indicates a software problem, and it should
**	be reported to Ingres Corporation Technical Support as a bug.
*/
E_CL0640_DI_LRU_PSEM_ERR:SS50000_MISC_ERRORS
"Error requesting a semaphore"
E_CL0641_DI_LRU_VSEM_ERR:SS50000_MISC_ERRORS
"Error releasing a semaphore"
E_CL0642_DI_LRU_CSRESERVE_ERR:SS50000_MISC_ERRORS
"Error trying to reserve an event control block"
E_CL0643_DI_LRU_GENPSEM_ERR:SS50000_MISC_ERRORS
"Error requesting a semaphore using a specified function"
E_CL0644_DI_LRU_GENVSEM_ERR:SS50000_MISC_ERRORS
"Error releasing a semaphore using a specified function"
E_CL0645_DI_LRU_CSCAUSE_EV_ERR:SS50000_MISC_ERRORS
"Error causing an event to begin"
E_CL0646_DI_LRU_CSSUPEND_ERR:SS50000_MISC_ERRORS
"Error suspending a thread"
E_CL0647_DI_LRU_CSFREE_EV_ERR:SS50000_MISC_ERRORS
"Error freeing a reserved event control block"
E_CL0650_DILRU_CSCND_WT_ERR:SS50000_MISC_ERRORS
"DIlru: Error waiting for a condition"
E_CL0651_DILRU_MEREQMEM_ERR:SS50000_MISC_ERRORS
"DIlru: Error requesting memory"
E_CL0652_DILRU_SLAVE_OP_ERR:SS50000_MISC_ERRORS
"DIlru: Error trying to select a DI slave"
E_CL0653_DILRU_CSCND_BRD_ERR:SS50000_MISC_ERRORS
"DIlru: Error waking up waiters on a condition"
E_CL0654_DILRU_ALL_PINNED_ERR:SS50000_MISC_ERRORS
"DIlru: All files in the LRU are pinned"
E_CL0655_DILRU_VIC_NOT_OP:SS50000_MISC_ERRORS
"DIlru: LRU victim not open"
E_CL0656_DILRU_CSCND_INIT_ERR:SS50000_MISC_ERRORS
"Error initializing a condition object"
E_CL0657_DILRU_CACHE_LIMIT:SS50000_MISC_ERRORS
"DIlru: Cache limit reached."
E_CL0701_DSN_RECUR:SS50000_MISC_ERRORS
"Recursive call attempted"
E_CL0702_DSN_LANG:SS50000_MISC_ERRORS
"Unsupported language specifier"
E_CL0703_DSN_ALIGN:SS50000_MISC_ERRORS
"Illegal alignment constraint"
E_CL0704_DSN_VIS:SS50000_MISC_ERRORS
"Illegal visibility level"
E_CL0705_DSN_UNKNOWN:SS50000_MISC_ERRORS
"Unknown operation requested"
E_CL0801_DY_NOSTAB:SS50000_MISC_ERRORS
"No symbol table generated"
E_CL0802_DY_NOEXE:SS50000_MISC_ERRORS
"No executable built"
E_CL0803_DY_NOMEM:SS50003_EXHAUSTED_RESOURCE
"Error allocating memory to load image"
E_CL0804_DY_NOREAD:SS50000_MISC_ERRORS
"Error reading in image"
E_CL0805_DY_NOFILE:SS50000_MISC_ERRORS
"Error locating image file"
E_CL0812_DL_OSLOAD_FAIL:SS50000_MISC_ERRORS
"Error loading dynamic library"
E_CL0813_DL_VERSION_WRONG:SS50000_MISC_ERRORS
"Dynamic library has incorrect version"
E_CL0815_DL_OSUNLOAD_FAIL:SS50000_MISC_ERRORS
"Error unloading dynamic library"
E_CL0816_DL_NOT_IMPLEMENTED:SS50000_MISC_ERRORS
"Dynamic loading is not implemented for this platform"
E_CL081B_DL_NOMEM:SS50003_EXHAUSTED_RESOURCE
"Insufficent free memory for dynamic loading"
E_CL081E_DL_BAD_LOOKUPFCN:SS50000_MISC_ERRORS
"Cannot find lookup function"
E_CL0820_DL_NOT_DIR:SS50000_MISC_ERRORS
"An invalid directory was for a dynamic library"
E_CL0821_DL_DIRNOT_FOUND:SS50000_MISC_ERRORS
"A nonexistent directory was specified for a dynamic library"
E_CL0822_DL_MODULE_PRESENT:SS50000_MISC_ERRORS
"The specified dynamic library already exists"
E_CL0823_DL_MOD_COUNT_EXCEEDED:SS50000_MISC_ERRORS
"Only one dynamic library may be loaded"
E_CL0824_DL_MOD_NOT_FOUND:SS50000_MISC_ERRORS
"The specified dynamic library does not exist"
E_CL0825_DL_LOOKUP_BAD_HANDLE:SS50000_MISC_ERRORS
"The internal identifier for a dynamic library is invalid"
E_CL0826_DL_FUNC_NOT_FOUND:SS50000_MISC_ERRORS
"The specified function was not found in the dynamic library"
E_CL0827_DL_MALFORMED_LINE:SS50000_MISC_ERRORS
"A line in the dynamic library descriptor file is malformed"
E_CL0828_DL_UNRECOGNIZED_LINE:SS50000_MISC_ERRORS
"A line in the dynamic library descriptor file is invalid"
E_CL0829_DL_MOD_NOT_READABLE:SS50000_MISC_ERRORS
"The specified dynamic library is not readable"
E_CL082A_DL_NO_NAME:SS50000_MISC_ERRORS
"No dynamic module name was specified"
E_CL082B_DL_MEM_ACCESS:SS50000_MISC_ERRORS
"Memory access error while loading dynamic library."
E_CL0901_ER_NO_FILE:SS50000_MISC_ERRORS
"A %0c:[%1c.FILES]*.MNX file could not be opened"
E_CL0902_ER_NOT_FOUND:SS50000_MISC_ERRORS
"No text found for message identifier"
E_CL0903_ER_BADPARAM:SS50000_MISC_ERRORS
"Bad parameter"
E_CL0904_ER_BADREAD:SS50000_MISC_ERRORS
"Error reading the %0c message file."
E_CL0905_ER_TOOSMALL:SS50000_MISC_ERRORS
"Buffer smaller then size of the message name"
E_CL0906_ER_TRUNCATED:SS01004_STRING_RIGHT_TRUNC
"Message text truncated because buffer was too small"
E_CL0907_ER_BADOPEN:SS50000_MISC_ERRORS
"Can't open/create receive channel for error logger"
E_CL0908_ER_BADRECEIVE:SS50000_MISC_ERRORS
"Error receiving error logger message"
E_CL0909_ER_BADSEND:SS50000_MISC_ERRORS
"Error sending message to error logger"
E_CL090A_ER_DIRERR:SS50000_MISC_ERRORS
"Error accessing error logger directory"
E_CL090B_ER_NOALLOC:SS50000_MISC_ERRORS
"Error allocating space for error logger"
E_CL090C_ER_MSGOVER:SS50000_MISC_ERRORS
"Error message overflow occurred sending message to error logger"
E_CL090D_ER_NOFREE:SS50000_MISC_ERRORS
"Error freeing memory for error logger"
E_CL090E_ER_BADCLASS:SS50000_MISC_ERRORS
"Invalid class associated with error message"
E_CL090F_ER_BADLANGUAGE:SS50000_MISC_ERRORS
"Invalid language associated with error message"

/*
** Template for formatting UNIX system errors.
**
** Parameters:
**    0    errno
**    1    library or system call name, e.g. "open", "read"
**    2    sys_errlist[errno]
*/
E_CL0910_ER_UNIXERROR:SS5000B_INTERNAL_ERROR
"%1c() failed with operating system error %0d (%2c)"

/* Test message- internal CL error */
E_CL0911_ER_INTERR_TESTMSG:SS50000_MISC_ERRORS
"Internal compatibility routine failed, parameter was \"%0c\"."
E_CL0912_ER_NO_AUDIT:SS5000M_IPC_ERROR
"No connection exists to security audit process"
E_CL0913_ER_FILE_FORMAT:SS50000_MISC_ERRORS
"Invalid message file format"
E_CL0914_ER_FILE_VERSION:SS50000_MISC_ERRORS
"Incorrect message file version"
E_CL0915_ER_FILE_CORRUPTION:SS50000_MISC_ERRORS
"Corruption detected in message file"

E_CL0A01_EXEXIT:SS50006_FATAL_ERROR
"Exiting program"
E_CL0A17_EXBADRETURN:SS50000_MISC_ERRORS
"Invalid return from a signal. Valid returns are EXDECLARE, EXRESIGNAL, and
EXCONTINUE"
E_CL0AFF_EXDECLARE:SS50000_MISC_ERRORS
"Unwind to declaring frame"
E_CL0AA0_EXSETCLIENT_LATE:SS50000_MISC_ERRORS
"EXsetclient: Call to EXsetclient will have no effect since EX subsystem
has already been initialized"
E_CL0AA1_EXSETCLIENT_BADCLIENT:SS50000_MISC_ERRORS
"EXsetclient: Unknown client type passed in"
E_CL0E01_IN_BACKEND_NAME:SS50004_SYS_CONFIG_ERROR
"IN%0c: Unable to start %1c -- unable to find name for backend"
E_CL0E02_IN_INTR_NAME:SS50004_SYS_CONFIG_ERROR
"IN%0c: Unable to start %1c -- unable to find name for interrupt process"
E_CL0E03_IN_NO_DB_NAME:SS42505_OBJ_NOT_FOUND
"IN%0c: no database name specified"
E_CL0E04_IN_RD_UNDEF:SS50000_MISC_ERRORS
"INread: receive pipe was not defined IN%0c did not complete normally"
E_CL0E05_IN_RD_SYSERR:SS50000_MISC_ERRORS
"INread: unable to get complete PIPEBLK -- system error during read"
E_CL0E06_IN_RD_INTR:SS50000_MISC_ERRORS
"INread: read call was interrupted"
E_CL0E07_IN_RD_PIPE_GONE:SS08004_CONNECTION_REJECTED
"INread: read pipe is no longer accessible"
E_CL0E08_IN_RD_NOTWHOLE:SS08004_CONNECTION_REJECTED
"INread: system read function did not write an entire pipe block"
E_CL0E09_IN_RD_CTLC:SS50000_MISC_ERRORS
"INread: got control-C"
E_CL0E0A_IN_WR_UNDEF:SS08004_CONNECTION_REJECTED
"INwrite: send pipe was not defined IN%0c did not complete normally"
E_CL0E0B_IN_WR_SYSERR:SS08004_CONNECTION_REJECTED
"INwrite: unable to send complete PIPEBLK -- system error during write"
E_CL0E0C_IN_WR_INTR:SS50000_MISC_ERRORS
"INwrite: write call was interrupted"
E_CL0E0D_IN_WR_PIPE_GONE:SS08004_CONNECTION_REJECTED
"INwrite: write pipe is no longer accessible"
E_CL0E0E_IN_WR_NOTWHOLE:SS08004_CONNECTION_REJECTED
"INwrite: system write function did not write an entire pipe block"
E_CL0E0F_IN_WR_CTLC:SS50000_MISC_ERRORS
"INwrite: got control-C"
E_CL0E10_IN_NO_BACKEND:SS50000_MISC_ERRORS
"INintrpt: there is no %0c backend process to interrupt"
E_CL0E11_IN_INACTIVE:SS50000_MISC_ERRORS
"INtimeout: timeout period exceeded"
E_CL0F01_LG_CANTINIT:SS08004_CONNECTION_REJECTED
"Error trying to initialize log file"
E_CL0F02_LG_CANTOPEN:SS08004_CONNECTION_REJECTED
"Error trying to open log file"
E_CL0F03_LG_CANTCLOSE:SS08004_CONNECTION_REJECTED
"Error trying to close log file"
E_CL0F04_LG_BADPARAM:SS50000_MISC_ERRORS
"Bad input parameter passed to LG routine"
E_CL0F05_LG_READERROR:SS08004_CONNECTION_REJECTED
"Error reading from log file"
E_CL0F06_LG_BADHEADER:SS50000_MISC_ERRORS
"Log file has bad header context"
E_CL0F07_LG_INITHEADER:SS50000_MISC_ERRORS
"Error trying to initialize log file header context"
E_CL0F08_LG_ENDOFFILE:SS50000_MISC_ERRORS
"Log logical end of file has been reached"
E_CL0F09_LG_OFFLINE:SS50004_SYS_CONFIG_ERROR
"Logging system not running"
E_CL0F0A_LG_BADFORMAT:SS50000_MISC_ERRORS
"Log file record has a bad format"
E_CL0F0B_LG_NOTLOADED:SS50004_SYS_CONFIG_ERROR
"Logging system device has not been loaded"
E_CL0F0C_LG_UNEXPECTED:SS08004_CONNECTION_REJECTED
"Logger received unexpected error from operating system"
E_CL0F0D_LG_CHK_SUM_ERROR:SS50000_MISC_ERRORS
"Bad checksum for log page"
E_CL0F0F_LG_DB_INCONSISTENT:SS50000_MISC_ERRORS
"One of the %0c databases has become inconsistent. No new transactions
may be begun against the inconsistent database, and no new update operations
against the inconsistent database may be performed by transactions already
underway."
E_CL0F10_LG_WRITEERROR:SS50000_MISC_ERRORS
"The %0c transaction logfile has encountered an I/O error. An attempt to
write to the transaction logfile was rejected with an error."
E_CL0F11_LG_NOCLUSTERINFO:SS50000_MISC_ERRORS
"Cluster configuration is missing. The %0c logging system attempted to
consult the cluster configuration files for information about the cluster
installation, but the files were missing or could not be read."
E_CL0F12_LG_CEXCEPTION:SS50000_MISC_ERRORS
"LG_CEXCEPTION message. This message indicates that the LG_CEXCEPTION
condition was raised. Please contact Ingres Corporation Technical Support 
to report this error."
E_CL0F13_LG_INTERRUPT:SS50000_MISC_ERRORS
"Logging system operation was interrupted. This message indicates that
a logging system operation was interrupted. Typically, this message is
informational only and does not indicate a real %0c error."
E_CL0F14_LG_EXCEED_LIMIT:SS50000_MISC_ERRORS
"A logging system limit was exceeded. Usage of the %0c logging system
has exceeded either the configured system limits or the underlying system
resources. Increase the logging system configuration limits and/or increase
the authorized system resources and retry the operation."
E_CL0F15_LG_DUAL_OPEN_FAIL:SS50000_MISC_ERRORS
"The DUAL transaction log file could not be opened."
E_CL0F20_LG_SYNCHRONOUS:SS50000_MISC_ERRORS
"A logging system operation was completed synchronously, with no waiting
required. Typically, this message is informational only and does not indicate
a real %0c error."
E_CL0F21_LG_BADMUTEX:SS50000_MISC_ERRORS
"An attempt to serialize access to the logging system memory structures failed.
The underlying mutual exclusion primitives were unable to guarantee sole access
to the logging system data structures, and so this logging system operation
failed."
E_CL0F22_LG_DUALLOG_MISMATCH:SS50000_MISC_ERRORS
"The logfile header information in the dual log file does not match the
logfile header information in the primary log file. The timestamp information
in the logfile headers will be used to determine the more recent logfile,
and that logfile will be used."
E_CL0F23_LG_LOGSIZE_MISMATCH:SS50000_MISC_ERRORS
"The dual log file is a different size than the primary log file."
E_CL0F24_LG_DISABLE_PRIM:SS50000_MISC_ERRORS
"The primary log file must be disabled."
E_CL0F25_LG_DISABLE_DUAL:SS50000_MISC_ERRORS
"The dual log file must be disabled."
E_CL0F26_LG_RECONSTRUCT:SS50000_MISC_ERRORS
"The logfile header could not be read and must be re-constructed."
E_CL0F27_LG_ERROR_INITIATING_IO:SS50000_MISC_ERRORS
"An internal error was encountered initiating a logfile I/O."
E_CL0F28_LG_WRONG_VERSION:SS50000_MISC_ERRORS
"The version number encoded in the %0c log file does not match the version
number in the currently-installed DBMS software. This version of the DBMS
software cannot use the %0c log file. If you have just recently installed
a new version of the DBMS software, you may need to re-initialize the
%0c log file by running the configuration tool CBF. Consult the Release 
Notes and the Installation Guide for more details."
/*%
** Parameters:
**	None.
**
** Explanation:
**	When the Recovery Process (dmfrcp) is first started, it opens
**	the log file and performs any recovery actions needed.
**
**	The first step in this process is to verify that the log file
**	has been initialized by the current version of the DBMS software.
**
**	A version number is recorded in the log file header; this version
**	number must match the version number compiled into the dmfrcp process.
**	If the version numbers do not match, the dmfrcp process refuses to
**	start.
**
** System Status:
**	The installation cannot be started until this error is resolved.
**
** Recommendation:
**	This error is most likely caused by an incomplete upgrade of the DBMS
**	software to a new version. Occasionally, upgrading the DBMS software
**	requires that the log file be re-initialized. This process is
**	described in detail in the Release Notes, but basically involves
**	the following steps:
**
**	    1) Shutdown the installation normally, after first allowing
**	        all recovery to complete (use "rcpconfig -shutdown",
**	        not "rcpconfig -imm_shutdown").
**	    2) Install the new version of the DBMS software.
**	    3) Re-initialize the log file with the new DBMS software
**	        by running "rcpconfig -init".
**	    4) Restart the installation.
**
**	If this error was caused by omitting step (3) above, you should
**	attempt to run "rcpconfig -init", then restart the installation.
**
**	If "rcpconfig -init" indicates that there are still active transactions
**	in the log file, you must restore the previous version of the DBMS
**	software and restart the installation using the previous version
**	of the software; then restart the upgrade process from step (1) above.
*/
E_CL0F2E_LG_DUPLICATE_TXN:SS50000_MISC_ERRORS
"The distributed transaction id is already known to the logging system."
E_CL0F2F_LG_NO_TRANSACTION:SS50000_MISC_ERRORS
"The distributed transaction id is unknown to the logging system."
E_CL0F30_LG_NO_MASTER:SS50004_SYS_CONFIG_ERROR
"Could not initialize the logging system for this process because the RCP
process is not running.
This indicates that either the RCP abnormally exited,
or that the server has attempted to connect to the logging system before the
RCP was brought up.
Shutdown all processes associated with this installation, and bring up the
installation from scratch using the supported startup tools, documented in
the Command Reference Guide."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Each installation contains a single process designated to be the
**	"master" of the logging and locking operations of that installation.
**	The dmfrcp process is the usual master process.  If this process
**	exits abnormally without running certain cleanup routines, it may
**	be possible for other processes to connect to the data structures
**	left about in shared memory, and get into a state where they wait
**	forever from a response from the non-existent "master" process.
**
**	A check for the existence of the "master" is performed whenever a
**	process connects to the logging system.  If the "master" process
**	does not exist then this error is returned.
**
**	This error indicates that the dmfrcp process has exited abnormally
**	(causes include "kill -9" and system coding errors).  A server cannot
**	be allowed to start when this condition exists, so this error is
**	returned.
**
**  System Status:
**	The process being started (the dmfacp or DBMS server) fails to
**	come up, printing error messages to the error log and possibly,
**	in the case of the dmfacp process, to the ACP log file.
**
**  Recommendation:
**	1. Check to see if a dmfrcp process is running in the installation.
**	   If one exists, this error may have been caused by a race condition
**	   between starting the process and starting the dmfrcp process.
**	   Try starting the server once more.
**	2. If 1 is not true, then:
**	   NOTE: The following should only be done by a System Administrator.
**	     a. Shut down the installation using "rcpconfig imm_shutdown".
**	     b. If any other processes remain active in the installation
**	        following this shutdown, then shut down these processes
**	        manually with the appropriate operating system command.
**	     c. Restart the installation from scratch using the appropriate
**	        startup command documented in the Installation
**	        Guide.
*/
E_CL0F31_LG_CANT_OPEN:SS08004_CONNECTION_REJECTED
"Could not initialize the logging system for this process because the
database recovery log file (%0c) could not be opened.  We attempted to
open the file with the a flag mask of (%1c).  Check the ownership and
permissions on the %2c log file."
/*%
** Parameters:
**	%0 - name of the recovery log file which the system attempted to open
**	%1 - Unix flag mask passed to the open() system call
**
** Explanation:
**	All transaction recovery information is written to a single log
**	file. The system's attempt to open this file has failed.
**
**  System Status:
**	The process being started halts with a fatal error.
**
**  Recommendation:
**	1) Check the file permissions on the recovery log file.
**	   This file should be owned by the same user who owns the rest
**	   of the installation.  The path to the file should be accessible 
**	   by the DBMS System Administrator.  The file must be readable 
**	   and writable by the DBMS System Administrator.
**
**	   Make sure that the environment variable II_LOG_FILE is not
**	   set in your local environment.
**
**	2) Check permissions on executables being run.  All executables
**	   which must access the recovery log file must have the "setuid"
**	   bit on and be owned by the DBMS System Administrator.  These 
**	   executables include: dmfacp, dmfrcp, dmfjsp, iidbms, csinstall, 
**	   and others.
**
*/
E_CL0F32_LG_SEM_OWNER_DEAD:SS50000_MISC_ERRORS
"A process exited while holding a semaphore critical to a logging operation.
The semaphore will never be granted so an error has been returned.  In the
current system this error is unrecoverable, and is fatal to any process
encountering it.  If this error is encountered the installation should be
shut down (killing processes if necessary).  Use the normal startup procedures,
which will perform automatic recovery of any outstanding transactions."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An owner of a logging critical (LK) semaphore has exited.
**	The requesting process would "wait" forever for access to
**	this resource if this error were not returned (possibly
**	spinning, using all available CPU on the machine).
**
** System Status:
**	All processes encountering this error will shut down.
**
** Recommendation:
**	If any process associated with an installation does not come down,
**	the installation should be shut down by hand, and then restarted
**	(refer to the I&O guide for OS-specific information on how to do this).
**	Information in the error log may point to the cause of the semaphore
**	failure.
*/
E_CL1001_LK_BUSY:SS50003_EXHAUSTED_RESOURCE
"Resource is busy"
E_CL1002_LK_TIMEOUT:SS50000_MISC_ERRORS
"Lock timed out"
E_CL1003_LK_BADPARAM:SS50000_MISC_ERRORS
"Bad parameter(s) passed to routine"
E_CL1004_LK_DEADLOCK:SS40001_SERIALIZATION_FAIL
"Deadlock detected"
E_CL1005_LK_NOLOCKS:SS50003_EXHAUSTED_RESOURCE
"Out of lock resources"
E_CL1006_LK_UNEXPECTED:SS50000_MISC_ERRORS
"Lock manager internal error"
E_CL1007_LK_NEW_LOCK:SS50000_MISC_ERRORS
"New lock was obtained"
E_CL1008_LK_VAL_NOTVALID:SS50000_MISC_ERRORS
"Lock resource does not have a current value."
E_CL1009_LK_RETRY:SS50000_MISC_ERRORS
"Lock is already owned by another lock list."
E_CL100A_LK_INTERRUPT:SS50000_MISC_ERRORS
"Locking system request was interrupted."
E_CL100B_LK_GRANTED:SS50000_MISC_ERRORS
"Lock request was granted."
E_CL100C_LK_NORMAL:SS50000_MISC_ERRORS
"Normal completion of lock request."
E_CL100D_LK_CVTNOTGRANT:SS50000_MISC_ERRORS
"Lock cannot be converted, since it was never granted."
E_CL100E_LK_NOTGRANTED:SS50000_MISC_ERRORS
"Lock request was not granted."
E_CL100F_LK_CANCEL:SS50000_MISC_ERRORS
"Lock cancellation ignored; lock has already been granted."
E_CL1010_LK_NOINTRWAIT:SS50000_MISC_ERRORS
"Lock request will wait and is not interruptable."
E_CL1011_LK_BADMUTEX:SS50000_MISC_ERRORS
"Error acquiring mutual exclusion for the locking system."
E_CL1012_LK_INTR_GRANT:SS50000_MISC_ERRORS
"Lock request was interrupted, but lock was already granted."
E_CL1015_LK_UBUSY:SS5000H_UNAVAILABLE_RESOURCE
"Lock request would wait, but timeout = NOWAIT."
E_CL1016_LK_NOTHELD:SS5000H_UNAVAILABLE_RESOURCE
"Lock owning process is dead, lock released."
E_CL1020_LK_BUSY_ALTER:SS50000_MISC_ERRORS
"LKalter call failed because lkd->lkd_llb_inuse was set to
%0d."
E_CL1021_LK_LLB_ALLOC_FAILED:SS50000_MISC_ERRORS
"LKalter's call to allocate %0d locks lists ran out of memory"
E_CL1022_LK_LOCK_ALLOC_FAILED:SS50000_MISC_ERRORS
"LKalter's call to allocate %0d locks ran out of memory"
E_CL1023_LK_LKH_ALLOC_FAILED:SS50000_MISC_ERRORS
"LKalter's call to allocate %0d hash buckets for the lock
hash table ran out of memory"
E_CL1024_LK_RSH_ALLOC_FAILED:SS50000_MISC_ERRORS
"LKalter's call to allocate %0d hash buckets for the resource
hash table ran out of memory"
E_CL1025_LK_NOINTERRUPT_ALTER:SS50000_MISC_ERRORS
"LKalter's call to change the interrupt status failed due to
a bad parameter; the lock id = %0d; the number of locks in the
system = %d."
E_CL1026_LK_NOINTERRUPT_ALTER:SS50000_MISC_ERRORS
"LKalter's call to change the interrupt status failed due to
a bad parameter; input lock id = %0d; the system
lock type = %1d; the system id = %2d."
E_CL1027_LK_BAD_ALTER_REQUEST:SS50000_MISC_ERRORS
"LKalter received request to alter parameter %0d, which is
not a supported LKalter parameter."
E_CL1028_LK_CANCEL_BADPARAM:SS50000_MISC_ERRORS
"LKcancel() failed due to a lock id bad parameter; the input
lock id = %0d; the number of locks in the system = %d."
E_CL1029_LK_CANCEL_BADPARAM:SS50000_MISC_ERRORS
"LKcancel() failed due to a lock id bad parameter; input lock id = %0d; the system lock type = %1d;
the system id instance = %2d."
E_CL102A_LK_CREATE_BADPARAM:SS50000_MISC_ERRORS
"LKcreate_list() failed due to a lock id bad parameter; the
input related lock id = %0d; the number of locks in the
system = %d."
E_CL102B_LK_CREATE_BADPARAM:SS50000_MISC_ERRORS
"LKcreate_list() failed due to a lock id bad parameter; input
related lock id = %0d; the system related lock
type = %1d; the system related id = %2d."
E_CL102C_LK_CREATE_BADPARAM:SS50000_MISC_ERRORS
"LKcreate_list() failed due to a NULL lock list pointer."
E_CL102D_LK_EXPAND_LIST_FAILED:SS50000_MISC_ERRORS
"LK failed while trying to allocate more objects of type %1c
(%0d), The reason for failure was that the locking and logging
shared memory pool was exhausted.  %2d lock lists, %3d locks, 
%4d resources have already been allocated."
E_CL102E_LK_EXPAND_LIST_FAILED:SS50000_MISC_ERRORS
"LK failed while trying to allocate more locks.  There
are %0d locks, allocated at system startup time, of which %1d
are used.  The request was made with either
(LLB_RECOVER | LLB_MASTER) status."
E_CL102F_LK_EXPAND_LIST_FAILED:SS50000_MISC_ERRORS
"LK failed while trying to allocate more locks.  There
are %0d locks, allocated at system startup time, of which %1d
are used.  %2d locks have been reserved for use by the
recovery system."
E_CL1030_LK_EXPAND_LIST_FAILED:SS50000_MISC_ERRORS
"LK failed while trying to allocate more locks lock lists.
There are %0d locks lists, allocated at system startup time,
of which %1d are used."
E_CL1031_LK_BAD_UNIQUE_ID:SS50000_MISC_ERRORS
"LK failed while allocating a lock list, due to a bad unique
identifier passed in by the caller.  The unique id's value is
%0d, and the lk_uhigh part of this structure is %1d"
E_CL1032_LK_DUPLICATE_LOCK_ID:SS50000_MISC_ERRORS
"LK failed while allocating a lock list, due to a bad unique
identifier passed in by the caller.  The unique ID already
exists, therefore this new lock list creation must be rejected
else it would lead to a duplicate lock id. The lock list which
already exists has the value %0x.%1x (in decimal, %0d.%1d)"
E_CL1033_LK_EVENT_BAD_PARAM:SS50000_MISC_ERRORS
"LKevent() failed due to a lock id bad parameter; the
input related lock id = %0d; the number of locks in the
system = %1d."
E_CL1034_LK_EVENT_BAD_PARAM:SS50000_MISC_ERRORS
"LKevent() failed due to a lock id bad parameter; input
lock id = %0d; the system related lock
type = %1d; the system related id = %2d."
E_CL1035_LK_EVENT_BAD_PARAM:SS50000_MISC_ERRORS
"LKevent() failed due to a bad parameter. The lock
identified was already waiting for an event; status of the
lock is %0d, and flag is %1d."
E_CL1036_LK_RELEASE_BAD_PARAM:SS50000_MISC_ERRORS
"LKrelease() failed due to a lock list id bad parameter; the
input lock list id = %0d; the number of lock lists in the
system = %1d."
E_CL1037_LK_RELEASE_BAD_PARAM:SS50000_MISC_ERRORS
"LKrelease() failed due to a lock id bad parameter; input
lock id = %0d; the system lock type = %1d;
the system id = %2d; llb_status = %3x."
E_CL1038_LK_RELEASE_BAD_PARAM:SS50000_MISC_ERRORS
"LKrelease() failed due to a lock id bad parameter; the
input related lock id = %0d; the number of locks in the
system = %1d."
E_CL1039_LK_RELEASE_BAD_PARAM:SS50000_MISC_ERRORS
"LKrelease() failed due to a lock id bad parameter; input
lock id = %0d; the system lock type = %1d;
the system id = %2d."
E_CL103A_LK_RELEASE_BAD_PARAM:SS50000_MISC_ERRORS
"LKrelease() failed due to a bad parameter, neither lock_key
nor lockid were set."
E_CL103B_LK_RELEASE_BAD_PARAM:SS50000_MISC_ERRORS
"LKrelease() failed due to a bad parameter, either the
system could not find the lock (lkb = %0d) or the lock found
had an incorrect attribute (lkb_attribute = %1x)."
E_CL103C_LK_RELEASE_BAD_PARAM:SS50000_MISC_ERRORS
"LKrelease() failed due to a bad parameter, no lock_key
was specified for a partial release."
E_CL103D_LK_REQUEST_BAD_PARAM:SS50000_MISC_ERRORS
"LKrequest() failed due to a lock list id bad parameter, or a
bad mode (lock mode = %2d); the input lock list id = %0d;
the number of locks in the system = %1d."
E_CL103E_LK_REQUEST_BAD_PARAM:SS50000_MISC_ERRORS
"LKrequest() failed due to a lock list id bad parameter; input
lock list id = %0d; the system lock type = %1d;
the system lock list id = %2d; llb_status = %3x."
E_CL103F_LK_REQUEST_BAD_PARAM:SS50000_MISC_ERRORS
"LKrequest() failed due to a bad parameter, no lockid was
specified for a convert."
E_CL1040_LK_REQUEST_BAD_PARAM:SS50000_MISC_ERRORS
"LKrequest() failed due to a lock list id bad parameter; the
input lock list id = %0d; the number of lock lists in the
system = %1d."
E_CL1041_LK_REQUEST_BAD_PARAM:SS50000_MISC_ERRORS
"LKrequest() failed due to a lock id bad parameter; input
lock id = %0d; the system lock type = %1d;
the system id = %2d."
E_CL1042_LK_REQUEST_BAD_PARAM:SS50000_MISC_ERRORS
"LKrequest() failed due to no lock key being passed in."
E_CL1043_LK_SHOW_BAD_PARAM:SS50000_MISC_ERRORS
"LKshow() failed to show LK_S_ORPHANED'd locks.  Size
requested is %0d, size of a LK_ID is %1d, and lock_key is
%2x."
E_CL1044_LK_SHOW_BAD_PARAM:SS50000_MISC_ERRORS
"LKshow() failed to show LK_S_RESOURCE.  Count
requested is %0d."
E_CL1045_LK_SHOW_BAD_PARAM:SS50000_MISC_ERRORS
"LKshow() failed to show LK_S_LOCKS.  Count
requested is %0d."
E_CL1046_LK_SHOW_BAD_PARAM:SS50000_MISC_ERRORS
"LKshow() failed to show LK_S_OWNER.  Bad lockid pointer
(%0x) or bad lock_key (%1x)."
E_CL1047_LK_SHOW_BAD_PARAM:SS50000_MISC_ERRORS
"LKshow() failed due to an unrecognized show flag (%0d)."
E_CL1048_LK_CANCEL_BADPARAM:SS50000_MISC_ERRORS
"Error in LKcancel with Shared Lock List - Lock list type or status is
incorrect.  Input list id: %0d, shared list type: %1d, shared list status: %2d."
E_CL1049_LK_CANCEL_BADPARAM:SS50000_MISC_ERRORS
"Error in LKcancel with Shared Lock List - Client lock list references
no shared list.  Input list id: %0d."
E_CL104A_LK_REQUEST_BAD_PARAM:SS50000_MISC_ERRORS
"Incompatible scope for lock request. Lock key = %1c"
E_CL1050_LK_LOCKID_NOTFOUND:SS50000_MISC_ERRORS
"Error in LKrequest call with specified lock id - Lock with specified id
not found.  Unique id (%0d, %1d), Request flags %2d."
E_CL1051_LK_DUPLICATE_LOCK_ID:SS50000_MISC_ERRORS
"Error in LKcreate_list allocating shared lock list - duplicated lock id.
Lock unique id (%0d, %1d)."
E_CL1052_LK_RELEASE_BADPARAM:SS50000_MISC_ERRORS
"Error in LKrelease with Shared Lock List - Lock list type or status is
incorrect.  Input list id: %0d, shared list type: %1d, shared list status: %2d."
E_CL1053_LK_RELEASE_BADPARAM:SS50000_MISC_ERRORS
"Error in LKrelease with Shared Lock List - Client lock list references
no shared list.  Input list id: %0d."
E_CL1054_LK_REQUEST_BADPARAM:SS50000_MISC_ERRORS
"Error in LKrequest with Shared Lock List - Lock list type or status is
incorrect.  Input list id: %0d, shared list type: %1d, shared list status: %2d."
E_CL1055_LK_REQUEST_BADPARAM:SS50000_MISC_ERRORS
"Error in LKrequest with Shared Lock List - Client lock list references
no shared list.  Input list id: %0d."
E_CL1056_LK_SEM_OWNER_DEAD:SS50000_MISC_ERRORS
"A process exited while holding a semaphore critical to a locking operation.
The semaphore will never be granted so an error has been returned.  In the
current system this error is unrecoverable, and is fatal to any process
encountering it.  If this error is encountered the installation should be
shut down (killing processes if necessary).  Use the normal startup procedures,
which will perform automatic recovery of any outstanding transactions."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An owner of a locking critical (LK) semaphore has exited.
**	The requesting process would "wait" forever for access to
**	this resource if this error were not returned (possibly
**	spinning, using all available CPU on the machine).
**
** System Status:
**	All processes encountering this error will shut down.
**
** Recommendation:
**	If any process associated with an installation does not come down,
**	the installation should be shut down by hand, and then restarted
**	(refer to the I&O guide for OS-specific information on how to do this).
**	Information in the error log may point to the cause of the semaphore
**	failure.
*/
E_CL1057_LK_EXCEED_LIMIT:SS50000_MISC_ERRORS
"One of the current DBMS locking parameters is set at %0d which exceeds
a resource limit of %1d.  Please reconfigure this parameter of the locking
system.  The internal LKalter code number was %2d."
E_CL1058_LK_REQUEST_RETRIED:SS50000_MISC_ERRORS
"Lock list %0d request %1d for resource %2d with mode %3d for session (%4d,%5x) 
is being retried; lkb_state: %6x, lkb_attribute %7x."
E_CL1060_LK_CBACK_REG_FAILED:SS50000_MISC_ERRORS
"Lock callback registration has failed."
E_CL1061_LK_CBACK_THREAD_DEAD:SS50000_MISC_ERRORS
"Lock callback thread is dead because of a fatal error."
E_CL1062_LK_CBACK_GET_FAILED:SS50000_MISC_ERRORS
"Lock callback get failed."
E_CL1063_LK_CBACK_GET_BADPARAM:SS50000_MISC_ERRORS
"lk_cback_get bad parameter passed to this routine - cbt->cbt_type=%d 
number of callbacks=%d."
E_CL1064_LK_CBACK_GET_BADCBACK:SS50000_MISC_ERRORS
"lk_cback_get bad callback get or callback structure corrupted -
cback->cback_type=%d ."
E_CL1065_LK_CBACK_PUT_BADCBT:SS50000_MISC_ERRORS
"lk_cback_put bad callback put or callback thread structure corrupted -
cbt->cbt_type=%d ."
E_CL1066_LK_CBACK_PUT_FAILED:SS50000_MISC_ERRORS
"lk_cback_put function failed because either a bad parameter was passed 
to this routine or could not acquire the cbt_mutex. "
E_CL1067_LK_CBACK_FIRE_BADRSB:SS50000_MISC_ERRORS
"lk_cback_fire routine has failed because a bad rsb was passed to this 
routine - rsb->rsb_type=%d ."
E_CL1068_LK_CBACK_FIRE_FAILED:SS50000_MISC_ERRORS
"lk_cback_function function failed because either a bad parameter was passed 
to this routine or lk_cback_put returned a bad status. "
E_CL1069_LK_CBACK_INIT_FAILED:SS50000_MISC_ERRORS
"lk_cback_init failed because either we could not allocate cbt_descriptor
or we could not acquire lkd_cbt_q_mutex exclusively."
E_CL106A_LK_CBACK_REG_BADCBT:SS50000_MISC_ERRORS
"lk_cback_register failed because cbt descriptor is corrupted -
cbt_descriptor->cbt_type=%d ."
/*%
** Parameters:
**	%0d - value of parameter exceeding limit
**	%1d - resource limit the parameter should not exceed
**	%2d - table the locking system was attempting to alter
**
** Explanation:
**	Possible table names passed as %2d:
**	  1 = LK_I_LLB : The number of lock lists in the locking system
**	  2 = LK_I_BLK : Maximum number of locks in the locking system
**	  3 - LK_I_LKH : Size of the locks hash table
**	  4 - LK_I_RSH : Size of the resources hash table
**
** System Status:
**	Recovery process will not start until the parameter in question is
**	lowered below or equal to the resource limit.
**
** Recommendation:
**	Reconfigure the appropriate parameter and reinitialize the log file.
*/
E_CL106D_LK_CONNECT_BADPARAM:SS50000_MISC_ERRORS
"LKconnect() failed due to a lock list id bad parameter; the
input lock list id = %0d; the number of lock lists in the
system = %1d."
E_CL106E_LK_CONNECT_BADPARAM:SS50000_MISC_ERRORS
"LKconnect() failed due to a lock list id bad parameter; input
lock list id = %0d; the system lock type = %1d;
the system lock list id = %2d."
E_CL106F_LK_CONNECT_BADPARAM:SS50000_MISC_ERRORS
"Error in LKconnect with Shared Lock List - Client lock list references
no shared list.  Input list id: %0d."
E_CL1070_LK_CONNECT_BADPARAM:SS50000_MISC_ERRORS
"Error in LKconnect with Shared Lock List - Lock list type or status is
incorrect.  Input list id: %0d, shared list type: %1d, shared list status: %2d."
E_CL1071_LK_FORCEABORT_ALTER:SS50000_MISC_ERRORS
"LKalter's call to change the force abort status failed due to
a bad parameter; the lock id = %0d; the number of locks in the
system = %d."
E_CL1072_LK_FORCEABORT_ALTER:SS50000_MISC_ERRORS
"LKalter's call to change the force abort status failed due to
a bad parameter; input lock id = %0d; the system
lock type = %1d; the system id = %2d."
E_CL1101_LO_ADD_BAD_SYN:SS50000_MISC_ERRORS
"LOaddpath: Bad syntax in arguments to LOaddpath -- tail shouldn't begin at
root"
E_CL1102_LO_FR_BAD_SYN:SS50000_MISC_ERRORS
"LOfroms: String argument to LOfroms has bad syntax"
E_CL1103_LO_NO_SUCH:SS50000_MISC_ERRORS
"No such location exists"
E_CL1104_LO_NO_PERM:SS42503_INSUF_PRIV
"You do not have permission to access the location with this call"
E_CL1105_LO_NO_SPACE:SS50003_EXHAUSTED_RESOURCE
"Not enough space on file system"
E_CL1106_LO_NOT_FILE:SS50000_MISC_ERRORS
"Location argument was not a FILENAME"
E_CL1107_LO_NULL_ARG:SS50000_MISC_ERRORS
"Illegal null pointer was passed to location routine"
E_CL1108_LO_CANT_TELL:SS50000_MISC_ERRORS
"Undetermined error in LO routines"
E_CL1109_LONOSAVE:SS50000_MISC_ERRORS
"LOreset called without matching call to LOsave"
E_CL110A_LO_TOO_LONG:SS50000_MISC_ERRORS
"LOgt:Location obtained overflowed buffer"
E_CL110B_LO_NOT_PATH:SS50000_MISC_ERRORS
"PATH component of the location argument is invalid"
E_CL110C_LO_BAD_DEVICE:SS50000_MISC_ERRORS
"Device component of the location argument is invalid"
E_CL110D_LO_NEST_DIR:SS50000_MISC_ERRORS
"LOdelete:Directory not empty"
E_CL110E_LO_IS_RAW:SS50000_MISC_ERRORS
"LOingpath:Location is a writable RAW device"
E_CL1201_ME_GOOD:SS00000_SUCCESS
"ME routine: The Status returned was good"
E_CL1202_ME_BD_CHAIN:SS50000_MISC_ERRORS
"MEdump: correct parameter value must be one of ME_D_ALLOC, ME_D_FREE,
ME_D_BOTH"
E_CL1203_ME_BD_CMP:SS50000_MISC_ERRORS
"MEcmp: number of bytes to compare must be > 0"
E_CL1204_ME_BD_COPY:SS50000_MISC_ERRORS
"MEcopy: number of bytes to copy must be > 0"
E_CL1205_ME_BD_FILL:SS50000_MISC_ERRORS
"MEfill: number of bytes to fill must be > 0"
E_CL1206_ME_BD_TAG:SS50000_MISC_ERRORS
"MEt[alloc, free]: tags must be > 0"
E_CL1207_ME_ERR_PROGRAMMER:SS5000B_INTERNAL_ERROR
"MEfree: Internal Error."
E_CL1208_ME_FREE_FIRST:SS50000_MISC_ERRORS
"ME[t]free: can't free a block before any blocks have been allocated"
E_CL1209_ME_GONE:SS50003_EXHAUSTED_RESOURCE
"ME[t]alloc: system can't allocate any more memory for this process"
E_CL120A_ME_NO_ALLOC:SS50000_MISC_ERRORS
"ME[t]alloc: request to allocate a block with zero (or less) bytes was ignored"
E_CL120B_ME_NO_FREE:SS50000_MISC_ERRORS
"MEfree: can't find a block with this address in the free list"
E_CL120C_ME_NO_TFREE:SS50000_MISC_ERRORS
"MEtfree: process hasn't allocated any memory with this tag"
E_CL120D_ME_00_PTR:SS50000_MISC_ERRORS
"ME[t]alloc: passed a null pointer"
E_CL120E_ME_00_CMP:SS50000_MISC_ERRORS
"MEcmp: passed a null pointer"
E_CL120F_ME_00_COPY:SS50000_MISC_ERRORS
"MEcopy: passed a null pointer"
E_CL1210_ME_00_DUMP:SS50000_MISC_ERRORS
"MEdump: passed a null pointer"
E_CL1211_ME_00_FILL:SS50000_MISC_ERRORS
"MEfill: passed a null pointer"
E_CL1212_ME_00_FREE:SS50000_MISC_ERRORS
"MEfree: passed a null pointer"
E_CL1213_ME_TR_FREE:SS50000_MISC_ERRORS
"MEfree: the memory has been corrupted"
E_CL1214_ME_TR_SIZE:SS50000_MISC_ERRORS
"MEsize: the memory has been corrupted"
E_CL1215_ME_TR_TFREE:SS50000_MISC_ERRORS
"MEtfree: the memory has been corrupted"
E_CL1216_ME_OUT_OF_RANGE:SS50000_MISC_ERRORS
"ME routine: address is out of process's data space, referencing will cause bus
error"
E_CL1217_ME_BF_OUT:SS50003_EXHAUSTED_RESOURCE
"MEneed: 'buf' doesn't have 'nbytes' left to allocate"
E_CL1218_ME_BF_ALIGN:SS50000_MISC_ERRORS
"MEinitbuf: 'buf' not aligned"
E_CL1219_ME_BF_FALIGN:SS50000_MISC_ERRORS
"MEfbuf: 'buf' not aligned"
E_CL121A_ME_BF_PARAM:SS50000_MISC_ERRORS
"MEfbuf: 'bytes' argument must come from call to MEfbuf()"
E_CL121B_ME_TOO_BIG:SS50000_MISC_ERRORS
"MEreqmem/MEreqlng/ME[t][c]alloc: Object too big"
E_CL121C_ME_OUT_OF_MEM:SS50003_EXHAUSTED_RESOURCE
"MEget_pages: Can not expand memory size"
E_CL121D_ME_BAD_PARAM:SS50000_MISC_ERRORS
"Bad Parameter to ME routines"
E_CL121E_ME_ILLEGAL_USAGE:SS50000_MISC_ERRORS
"Combination of arguments to an ME routine is invalid"
E_CL121F_ME_BAD_ADVICE:SS50000_MISC_ERRORS
"An ME routine will not function with the given MEadvice)"
E_CL1220_ME_ALREADY_EXISTS:SS50000_MISC_ERRORS
"A shared memory segment to be created already exists"
/*
** Parameters:
**      None.
**
** Explanation:
**      A process has tried to attached to a shared memory segment
**      that does not exist.
**
** System Status:
**      The process will exit prematurely.
**
** Recommendation:
**      Shutdown the Ingres installation and manually confirm that all 
**      Ingres processes for the current installation have been stopped.
**      Remove any files from $II_SYSTEM/ingres/files/memory and confirm
**      that you have enough memory for your Ingres installation.
*/
E_CL1221_ME_NO_SUCH_SEGMENT:SS50000_MISC_ERRORS
"A shared memory segment to be manipulated does not exist"
E_CL1222_ME_NO_SHARED:SS50000_MISC_ERRORS
"This CL does not support shared memory"
E_CL1223_ME_NOT_ALLOCATED:SS50000_MISC_ERRORS
"The memory specified for some ME operation was not all allocated"
E_CL1224_ME_NOT_SUPPORTED:SS0A000_FEATUR_NOT_SUPPORTED
"The ME operation requested is not supported in this CL"
E_CL1225_ME_NO_PERM:SS42503_INSUF_PRIV
"Insufficient permission for some ME operation"
E_CL1226_ME_SHARED_ALLOC_FAILURE:SS50003_EXHAUSTED_RESOURCE
"ME_get_shared: The request to the operating system to allocate %0u bytes of
shared memory failed.  This request most probably failed because the size of
the allocation exceeds the maximum size shared memory segment configured for
the OS.  In most system V interface compliant shared memory OS implementations
this maximum size is user tunable by altering the SHMSIZE parameter in the
kernel configuration procedure."
/*%
** Parameters:
**	%0 - amount of shared memory requested (in bytes)
**
** Explanation:
**	Shared memory has been requested using the SystemV shmget() interface.
**	This routine has returned EINVAL, which can mean one of 2 things
**	(use "man shmget" for information specific to your OS):
**
**	    -  size is less than the system-imposed minimum or greater
**	       than the system imposed maximum.
**
**	    -  a shared memory identifier exists for key, but the size
**	       of the segment associated with it is less than size and
**	       size is not equal to zero.
**
**	The maximum size shared-memory segment a system can provide is site-
**	specific and is given by the system administrator-defined value assigned
**	to SHMSIZE (in 1k units).  This value usually is defaulted to 1024,
**	which sets an upper limit of 1 megabyte for a single shared memory
**	buffer.  This means that under a system configured with the defaults,
**	one may not start up a DBMS server using a shared buffer cache bigger
**	than 512 pages:
**
**	        512 pages * 2048 bytes/page = 1meg.
**
**	The value of SHMSIZE should be set to at least twice the number of
**	pages that you want to start up a DBMS server with. For instance,
**	if you want to start up a DBMS server with a 1024-page cache,
**	you should rebuild the kernel with SHMSIZE set to 2048 (actually, you
**	should set it slightly higher, since there is some amount of overhead
**	used by the DMF cache.)
**
**	This example assumes a 2048 byte page.
**
** System Status:
**	Server will not start up.
**
** Recommendation:
**	Either restart the server with the dmf.cache_size set to less than
**	1/2 the current value of SHMSIZE, or rebuild the kernel with SHMSIZE
**	increased enough to support the required size buffer cache.
*/
E_CL1227_ME_NO_SHMEM_LOCK:SS50000_MISC_ERRORS
"ME_get_pages: The server cannot lock memory in this platform
configuration (mmap() not supported or configured)."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A request has been made to lock memory with a server configuration
**	in which mmap() is either not supported or not configured.
**
** System Status:
**	The server will not start up.
**
** Recommendation:
**	Reconfigure the server to not request memory locking.  (If the
**	platform does indeed support mmap(), mlock() and seteuid()
**	and this error is obtained, Ingres Corporation may add this 
**	functionality to this platform in a future release.)
*/
E_CL1228_ME_NO_MLOCK:SS50000_MISC_ERRORS
"MEget_pages: As the operating system does not support either or
both mlock() or seteuid(), the request to lock memory cannot be satisfied."
/*%
** Parameters:
**	None.
**
** Explanation
**	The caller has requested locking memory on a system that does
**	not support either the mlock() or seteuid() function;  mlock()
**	is required to lock memory, seteuid() to set and reset executable
**	user IDs before and after memory locking.
**
** System Status:
**	Server will not start up.
**
** Recommendation:
**	Reconfigure the server to not request memory locking.
**	(If the platform does indeed support mmap(), mlock() and seteuid()
**	and this message is obtained, Ingres Corporation may add this 
**	functionality to this platform in a future release.)
*/
E_CL1229_ME_LOCK_FAIL:SS50000_MISC_ERRORS
"MEget_pages: The attempt to lock memory has failed.
Possible causes include: the server not started by super-user;
server executable not configured or secured properly; or not
enough physical memory."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The UNIX server has failed in the attempt to lock memory.
**	Other messages in the error log should identify the specific
**	function error code.
**
**	Possible problems include:
**	  - server not started by root
**	  - server executable not setuid root
**	  - not enough memory to satisfy the lock request
**
** System Status:
**	Server will not start up.
**
** Recommendation:
**	Either fix the specific problem or reconfigure the server and
**	remove the memory lock request.
*/
E_CL122A_ME_LCKPAG_FAIL:SS50000_MISC_ERRORS
"MEget_pages: Failure in VMS sys$lckpag call to lock memory."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The VMS server cannot lock a shared data segment because of
**	a failure of the SYS$LCKPAG system service call.  Possible
**	problems include the server not running with PSWAPM privilege,
**	or not enough memory either on the machine or in the server
**	working set (/MAXIMUM_WORKING_SET server startup parameter).
**	The specific VMS error should be available in the error log
**	file.
**
** System Status:
**	The server will not start up.
**
** Recommendation:
**	Either fix the problem noted by the error message and retry the
**	request, or remove the memory lock request from the server
**	configuration.
*/
E_CL1301_MH_BADARG:SS50000_MISC_ERRORS
"Bad argument"
E_CL1302_MH_PRECISION:SS50000_MISC_ERRORS
"Lost precision during calculation"
E_CL1303_MH_INTERNERR:SS50000_MISC_ERRORS
"Internal error"
E_CL1304_MH_MATHERR:SS50000_MISC_ERRORS
"Math error"
E_CL1305_MH_LNZERO:SS50000_MISC_ERRORS
"MHln: Value must be greater than 0."
E_CL1306_MH_SQRNEG:SS50000_MISC_ERRORS
"MHsqrt: Negative number passed to the square root function."
E_CL1401_NM_PWDOPN:SS50000_MISC_ERRORS
"NMpathIng: Unable to open passwd file"
E_CL1402_NM_PWDFMT:SS50000_MISC_ERRORS
"NMpathIng: Bad passwd file format"
E_CL1403_NM_INGUSR:SS50000_MISC_ERRORS
"NMpathIng: There is no %0c user in the passwd file"
E_CL1404_NM_INGPTH:SS50000_MISC_ERRORS
"NM[sg]tIngAt: No path returned for %0c user"
E_CL1405_NM_STOPN:SS50000_MISC_ERRORS
"NM[sg]tIngAt: Unable to open symbol table"
E_CL1406_NM_STPLC:SS50000_MISC_ERRORS
"NMstIngAt: Unable to position to record in symbol table"
E_CL1407_NM_STREP:SS50000_MISC_ERRORS
"NMstIngAt: Unable to replace record in symbol table"
E_CL1408_NM_STAPP:SS50000_MISC_ERRORS
"NMstIngAt: Unable to append symbol to symbol table"
E_CL1409_NM_LOC:SS50000_MISC_ERRORS
"NM_loc: first argument must be 'f' or 't'"
E_CL140A_NM_BAD_PWD:SS50004_SYS_CONFIG_ERROR
"NMpathIng: Bad home directory for %0c superuser"
E_CL140B_NM_STCORRUPT:SS50000_MISC_ERRORS
"NM[st,zap]IngAt: The symbol table file (symbol.tbl) seems to be corrupted.
Please reset the symbol.tbl file to the symbol.bak file, which will be located
in the same directory as the symbol.tbl file (either the II_CONFIG or the
II_ADMIN location)."
E_CL140C_NM_NO_II_SYSTEM:SS50004_SYS_CONFIG_ERROR
"II_SYSTEM has not been defined."
E_CL140D_NM_PRTLNG:SS50000_MISC_ERRORS
"NMsetpart: Part name too long."
E_CL140E_NM_PRTFMT:SS50000_MISC_ERRORS
"NMsetpart: Illegal part name format."
E_CL140F_NM_PRTNUL:SS50000_MISC_ERRORS
"NMsetpart: Part name is a null string."
E_CL1410_NM_PRTALRDY:SS50000_MISC_ERRORS
"NMsetpart: Part name already set."
E_CL1601_PC_CM_CALL:SS50000_MISC_ERRORS
"PCcmdline: arguments incorrect"
E_CL1602_PC_CM_EXEC:SS50000_MISC_ERRORS
"PCcmdline: cannot execute command specified, bad magic number"
E_CL1603_PC_CM_MEM:SS50003_EXHAUSTED_RESOURCE
"PCcmdline: system temporarily out of core memory or process requires too many
system resources"
E_CL1604_PC_CM_OWNER:SS42503_INSUF_PRIV
"PCcmdline: must be owner or super-user to execute this command"
E_CL1605_PC_CM_PATH:SS50000_MISC_ERRORS
"PCcmdline: part of specified path didn't exist"
E_CL1606_PC_CM_PERM:SS42503_INSUF_PRIV
"PCcmdline: didn't have permission to execute"
E_CL1607_PC_CM_PROC:SS50002_LIMIT_EXCEEDED
"PCcmdline: system process table is temporarily full"
E_CL1608_PC_CM_REOPEN:SS50000_MISC_ERRORS
"PCcmdline: couldn't associate argument [in, out]_name with std[in, out]"
E_CL1609_PC_CM_SUCH:SS50000_MISC_ERRORS
"PCcmdline: command doesn't exist"
E_CL160A_PC_CM_BAD:SS50000_MISC_ERRORS
"PCcmdline: child didn't execute, possibly due to bad command parameters"
E_CL160B_PC_CM_TERM:SS50000_MISC_ERRORS
"PCcmdline: child we were waiting for returned bad termination status"
E_CL160C_PC_END_PIPE:SS5000M_IPC_ERROR
"PCendpipe: had trouble closing PIPE argument passed in"
E_CL160D_PC_SP_CALL:SS50000_MISC_ERRORS
"PC[f][s]spawn: arguments incorrect"
E_CL160E_PC_SP_EXEC:SS50000_MISC_ERRORS
"PC[f][s]spawn: cannot execute command specified, bad magic number"
E_CL160F_PC_SP_MEM:SS50003_EXHAUSTED_RESOURCE
"PC[f][s]spawn: system temporarily out of core memory or process requires 
too many system resources"
E_CL1610_PC_SP_OWNER:SS42503_INSUF_PRIV
"PC[f][s]spawn: must be owner or super-user to execute this command"
E_CL1611_PC_SP_PATH:SS50000_MISC_ERRORS
"PC[f][s]spawn: part of specified path didn't exist"
E_CL1612_PC_SP_PERM:SS42503_INSUF_PRIV
"PC[f][s]spawn: didn't have permission to execute"
E_CL1613_PC_SP_PROC:SS50003_EXHAUSTED_RESOURCE
"PC[f][s]spawn: system process table is temporarily full"
E_CL1614_PC_SP_SUCH:SS50000_MISC_ERRORS
"PC[f][s]spawn: command doesn't exist"
E_CL1615_PC_SP_CLOSE:SS5000M_IPC_ERROR
"PC[f]sspawn: had trouble closing unused sides of pipes"
E_CL1616_PC_SP_DUP:SS5000M_IPC_ERROR
"PC[f]sspawn: had trouble associating, dup()'ing, the pipes with child's
std[in,out]"
E_CL1617_PC_SP_PIPE:SS5000M_IPC_ERROR
"PC[f]sspawn: couldn't establish communication pipes"
E_CL1618_PC_SP_OPEN:SS5000M_IPC_ERROR
"PCfsspawn: had trouble associating child's std[in,out] with passed in FILE
pointers"
E_CL1619_PC_SP_REOPEN:SS5000M_IPC_ERROR
"PCspawn: couldn't associate argument [in, out]_name with std[in, out]"
E_CL161A_PC_RD_CALL:SS50000_MISC_ERRORS
"PCread: one of the passed in arguments is NULL"
E_CL161B_PC_RD_CLOSE:SS5000M_IPC_ERROR
"PCread: stream corrupted, should never happen"
E_CL161C_PC_RD_OPEN:SS5000M_IPC_ERROR
"PCread: trouble establishing communication pipes"
E_CL161D_PC_SD_CALL:SS5000M_IPC_ERROR
"PCsend: invalid exception value"
E_CL161E_PC_SD_NONE:SS5000M_IPC_ERROR
"PCsend: No process corresponding to the pid (process argument) exists"
E_CL161F_PC_SD_PERM:SS42503_INSUF_PRIV
"PCsend: Can't signal specified process.  Effective uid of caller isn't 'root'
and doesn't match real uid of receiving process"
E_CL1620_PC_WT_BAD:SS5000M_IPC_ERROR
"PCwait: child didn't execute, possibly due to bad command parameters"
E_CL1621_PC_WT_NONE:SS5000M_IPC_ERROR
"PCwait: no children of this process are currently alive"
E_CL1622_PC_WT_TERM:SS5000M_IPC_ERROR
"PCwait: child we were waiting for returned bad termination status"
E_CL1623_PC_WT_INTR:SS5000M_IPC_ERROR
"PCwait: child was interrupted by a signal"
E_CL1624_PC_NOT_MBX:SS5000M_IPC_ERROR
"PEendpipe: channel not assigned to mailbox"
E_CL1625_PC_INTERLOCK:SS5000M_IPC_ERROR
"PEendpipe: mailbox locked by another process"
E_CL1626_PC_BADCHAN:SS5000M_IPC_ERROR
"PEendpipe: not a legal channel number"
E_CL1627_PC_NOPRIV:SS42503_INSUF_PRIV
"PEendpipe: mailbox not assigned or insufficient privilege"
E_CL1628_PC_WT_EXEC:SS5000M_IPC_ERROR
"PCwait: child is executing"
E_CL1701_PE_BAD_PATTERN:SS50000_MISC_ERRORS
"Permission pattern passed to a permission routine had syntax errors"
E_CL1702_PE_NULL_LOCATION:SS50000_MISC_ERRORS
"Location passed to a permission routine was null"
E_CL1800_QU:SS50000_MISC_ERRORS
"No errors from these routines"
E_CL1901_SI_BAD_SYNTAX:SS50000_MISC_ERRORS
"SIopen: Bad syntax in mode argument to SIopen"
E_CL1902_SI_EMFILE:SS50002_LIMIT_EXCEEDED
"Can't open another file -- maximum number of files already opened"
E_CL1903_SI_BAD_MODE:SS50000_MISC_ERRORS
"SIfopen: Bad mode parameter"
E_CL1904_SI_CANT_OPEN:SS50000_MISC_ERRORS
"SIfopen: Can't open file"
E_CL1B01_SR_BADFILE:SS50000_MISC_ERRORS
"Bad sort file identifier"
E_CL1B02_SR_ENDFILE:SS50000_MISC_ERRORS
"End of File"
E_CL1B03_SR_BADOPEN:SS08004_CONNECTION_REJECTED
"Error opening sort file"
E_CL1B04_SR_BADCLOSE:SS08004_CONNECTION_REJECTED
"Error closing sort file"
E_CL1B05_SR_BADWRITE:SS08004_CONNECTION_REJECTED
"Error writing to sort file"
E_CL1B06_SR_BADREAD:SS08004_CONNECTION_REJECTED
"Error reading from sort file"
E_CL1B07_SR_BADEXTEND:SS08004_CONNECTION_REJECTED
"Error allocating space to sort file"
E_CL1B08_SR_BADCREATE:SS08004_CONNECTION_REJECTED
"Error creating sort file"
E_CL1B09_SR_EXCEED_LIMIT:SS08004_CONNECTION_REJECTED
"File resource quota exceeded"
E_CL1B0A_SR_BADPARAM:SS50000_MISC_ERRORS
"Bad parameter(s) passed to SR routine"
E_CL1B0B_SR_BADDIR:SS08004_CONNECTION_REJECTED
"Error verifying directory"
E_CL1F01_TM_BAD_MSECS:SS50000_MISC_ERRORS
"Number of milliseconds for an alarm was less than zero"
E_CL1F02_TM_ZERO_TIME:SS50000_MISC_ERRORS
"Zero seconds resulted when rounding milliseconds for an alarm"
E_CL1F03_TM_NOTIMEZONE:SS50004_SYS_CONFIG_ERROR
"The environment variable, II_TIMEZONE, is undefined"
E_CL1F04_TM_SYNTAX:SS50000_MISC_ERRORS
"syntax error in time format"
E_CL1F05_TM_NO_TZNAME:SS50000_MISC_ERRORS
"The environment variable, II_TIMEZONE_NAME, is undefined"
E_CL1F06_TM_NO_TZFILE:SS50000_MISC_ERRORS
"Cannot find a matching timezone information file for the given timezone name"
E_CL1F07_TM_TZFILE_OPNERR:SS50000_MISC_ERRORS
"Cannot open the timezone information file.  This file does not exist or you
do not have sufficient privilege to access this file"
E_CL1F08_TM_TZFILE_BAD:SS50000_MISC_ERRORS
"Timezone information file does not have the correct internal structure"
E_CL1F09_TM_TZFILE_NOMEM:SS50000_MISC_ERRORS
"Cannot load timezone information file because of insufficient memory"
E_CL1F0A_TM_TZLKUP_FAIL:SS50000_MISC_ERRORS
"The timezone name cannot be found"
E_CL1F0B_TM_PMFILE_OPNERR:SS50000_MISC_ERRORS
"Error opening PM configuration file during the timezone initiation process"
E_CL1F0C_TM_PMFILE_BAD:SS50000_MISC_ERRORS
"PM configuration file is found to be in bad internal format during
the timezone initiation process"
E_CL1F0D_TM_PMNAME_BAD:SS50000_MISC_ERRORS
"Fail to find the timezone information file for the given timezone name"
E_CL1F0E_TM_PMVALUE_BAD:SS50000_MISC_ERRORS
"Invalid path name or invalid file name for the given timezone name"
E_CL1F0F_TM_NOSUPPORT:SS50000_MISC_ERRORS
"The TMprfstat() routine on this platform provides no OS statistics support, 
and  thus will return a structure with all members set to -1."
E_CL1F10_TM_BADDATECUT:SS50000_MISC_ERRORS
"The value of II_DATE_CENTURY_BOUNDARY provided is illegal"
/*%
** Parameters:
**	none.
**
** Explanation:
**	The value of II_DATE_CENTURY_BOUNDARY provided is outside
**	the permissible range.
**
** System Status:
**	The connection will fail.
**
** Recommendation:
**	Alter or remove II_DATE_CENTURY_BOUNDARY.
*/
E_CL1F11_TM_GETTIME_ERR:SS50000_MISC_ERRORS
"Error getting the current time from the system"
E_CL2101_TR_BADCREATE:SS08004_CONNECTION_REJECTED
"Error creating trace output file"
E_CL2102_TR_BADOPEN:SS08004_CONNECTION_REJECTED
"Error opening trace output file"
E_CL2103_TR_ENDINPUT:SS50000_MISC_ERRORS
"TRrequest: End of File encountered"
/*%
** Parameters:
**	None.
**
** Explanation:
**	End of input file has been encountered or user has input EOF
**	in response to prompt.
**
** System Status:
**	Client code requesting input should terminate reading.
**
** Recommendation:
**	None.
*/
E_CL2104_TR_BADPARAM:SS50000_MISC_ERRORS
"Bad parameter(s) passed to this routine"
E_CL2105_TR_BADREAD:SS08004_CONNECTION_REJECTED
"Error reading trace file"
E_CL2106_TR_BADWRITE:SS08004_CONNECTION_REJECTED
"Error writing trace file"
E_CL2107_TR_NOT_OPENED:SS50000_MISC_ERRORS
"Trace file not opened."
E_CL2201_UT_CO_FILE:SS50000_MISC_ERRORS
"Can't open utcom.def"
E_CL2202_UT_CO_IN_NOT:SS50000_MISC_ERRORS
"Can't open input file"
E_CL2203_UT_CO_LINES:SS50000_MISC_ERRORS
"Too many command lines for this suffix in utcom.def"
E_CL2204_UT_CO_CHARS:SS50000_MISC_ERRORS
"Commands for this suffix in utcom.def are too long"
E_CL2205_UT_CO_SUF_NOT:SS50000_MISC_ERRORS
"Couldn't find suffix in utcom.def"
E_CL2206_UT_ED_CALL:SS50000_MISC_ERRORS
"UTedit: NULL filename argument"
E_CL2207_UTENOEQUAL:SS50000_MISC_ERRORS
"No equal sign ('=') in argument list"
E_CL2208_UTENOPERCENT:SS50000_MISC_ERRORS
"No percent sign ('\%') in argument list"
E_CL2209_UTENOSPEC:SS50000_MISC_ERRORS
"No type specifier in argument list"
E_CL220A_UTEBADARG:SS50000_MISC_ERRORS
"The argument list does not have a comma between two arguments or ends too
early"
E_CL220B_UTENOARG:SS50000_MISC_ERRORS
"Given argument not in utexe.def entry for program"
E_CL220C_UTEBIGCOM:SS50000_MISC_ERRORS
"Command length exceeds utexe's limit"
E_CL220D_UTEBADSPEC:SS50000_MISC_ERRORS
"Utexe.def has a bad type specifier in one entry for the program"
E_CL220E_UTENOBIN:SS50000_MISC_ERRORS
"No executable found for program"
E_CL220F_UTENOPROG:SS50000_MISC_ERRORS
"The given program name is not in utexe.def"
E_CL2210_UT_PR_CALL:SS50000_MISC_ERRORS
"UTprint: NULL filename argument"
E_CL2211_UTENoInit:SS50000_MISC_ERRORS
"No text"
E_CL2212_UTENoWriteFunc:SS50000_MISC_ERRORS
"No text"
E_CL2213_UTENoReadFunc:SS50000_MISC_ERRORS
"No text"
E_CL2214_UTENoModuleType:SS50000_MISC_ERRORS
"No text"
E_CL2215_UTENOSYM:SS50000_MISC_ERRORS
"The actual argument for a \%C parameter was not a valid user command"
E_CL2216_UTELATEEXE:SS50000_MISC_ERRORS
"A module being run as a procedure had a \%C or \%E actual argument"
E_CL2217_UTEBADTYPE:SS50000_MISC_ERRORS
"The type specifier for one argument was not of an expected type"
E_CL2218_UT_EXE_DEF:SS50000_MISC_ERRORS
"Can't open utexe.def"
E_CL2219_UT_LD_DEF:SS50000_MISC_ERRORS
"Can't open utld.def"
E_CL221A_UT_LD_OBJS:SS50003_EXHAUSTED_RESOURCE
"Can't allocate memory for list of objects to be linked"
E_CL221B_UT_CMP_WARNINGS:SS50000_MISC_ERRORS
"Error UT_CMP_WARNINGS encountered"
E_CL221C_UT_CMP_FAIL:SS50000_MISC_ERRORS
"Compile failed"
E_CL221D_UT_LNK_WARNINGS:SS50000_MISC_ERRORS
"Error UT_LNK_WARNINGS encountered"
E_CL221E_UT_LNK_FAIL:SS50000_MISC_ERRORS
"Error UT_LNK_FAIL encountered"
E_CL2300_NT:SS5000B_INTERNAL_ERROR
"No errors for these routines"
E_CL2401_JF_BAD_FILE:SS50000_MISC_ERRORS
"Bad journal file"
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal error was detected by the subroutines which manage journal
**	files. A journal file handle (the JFIO control block) was invalid. This
**	can be caused by a program error or a memory corruption error.
**
** System Status:
**	The journal file operation is rejected.
**
** Recommendation:
**	This error message indicates a software problem. If the problem
**	can be reproduced, it should be reported to Ingres Corporation 
**	Technical Support as a bug.
*/
E_CL2402_JF_END_FILE:SS50000_MISC_ERRORS
"End of journal file reached"
/*%
** Parameters:
**	None.
**
** Explanation:
**	This message is used internally to indicate that the end of a journal
**	file has been reached -- i.e., the journal file has been completely
**	read. Under normal circumstances, this message should never be reported
**	to a user.
**
** Recommendation:
**	Appearance of this message in the system error log or in the ACP
**	log probably indicates a software problem. If the problem
**	can be reproduced, it should be reported to Ingres Corporation 
**	Technical Support as a bug.
*/
E_CL2403_JF_BAD_OPEN:SS08004_CONNECTION_REJECTED
"Error trying to open a journal file"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The operating system rejected an attempt to open a journal file. The
**	specific operating system error message is appended to this error
**	message, if it is available. The problem may be due to file security
**	violations, to a disk error, or the journal file may have been
**	accidentally erased.
**
** System Status:
**	The journal file operation is rejected.
**
** Recommendation:
**	Analyze the specific operating system error message for information
**	about why the journal file could not be opened. Take the appropriate
**	actions to allow the DBMS to open the journal file (alter the file
**	security appropriately, repair the broken disk, etc.).
*/
E_CL2404_JF_BAD_CLOSE:SS08004_CONNECTION_REJECTED
"Error trying to close a journal file"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The operating system rejected an attempt to close a journal file. The
**	specific operating system error message is appended to this error
**	message, if it is available. The problem may be due to a disk error.
**
** System Status:
**	The journal file operation is rejected.
**
** Recommendation:
**	Analyze the specific operating system error message for information
**	about why the journal file could not be closed. Take the appropriate
**	actions to allow the DBMS to access the journal file (alter the file
**	security appropriately, repair the broken disk, etc.).
*/
E_CL2405_JF_BAD_WRITE:SS08004_CONNECTION_REJECTED
"Error trying to write a journal file"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The operating system rejected an attempt to write to a journal file.
**	The specific operating system error message is appended to this error
**	message, if it is available. The problem is often due to insufficient
**	disk space, or it may be due to a disk error.
**
** System Status:
**	The journal file operation is rejected.
**
** Recommendation:
**	Analyze the specific operating system error message for information
**	about why the journal file could not be written. Take the appropriate
**	actions to allow the DBMS to access the journal file (add more disk
**	space, alter the file security appropriately, repair the broken disk,
**	etc.).
*/
E_CL2406_JF_BAD_READ:SS08004_CONNECTION_REJECTED
"Error trying to read a journal file"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The operating system rejected an attempt to read a journal file. The
**	specific operating system error message is appended to this error
**	message, if it is available. The problem may be due to a disk error.
**
** System Status:
**	The journal file operation is rejected.
**
** Recommendation:
**	Analyze the specific operating system error message for information
**	about why the journal file could not be read. Take the appropriate
**	actions to allow the DBMS to access the journal file (alter the file
**	security appropriately, repair the broken disk, etc.).
*/
E_CL2407_JF_BAD_ALLOC:SS08004_CONNECTION_REJECTED
"Error trying to extend a journal file"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The operating system rejected an attempt to allocate space for a
**	journal file. The specific operating system error message is appended
**	to this error message, if it is available. The problem is often due to
**	insufficient disk space, or it may be due to a disk error.
**
** System Status:
**	The journal file operation is rejected.
**
** Recommendation:
**	Analyze the specific operating system error message for information
**	about why the journal file space could not be allocated. Take the
**	appropriate actions to allow the DBMS to access the journal file (add
**	more disk space, alter the file security appropriately, repair the
**	broken disk, etc.).
*/
E_CL2408_JF_BAD_UPDATE:SS08004_CONNECTION_REJECTED
"Error trying to update the header of a journal file"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The operating system rejected an attempt to write the header block of a
**	journal file. The specific operating system error message is appended
**	to this error message, if it is available. The problem may be due to a
**	disk error, or it may be a file security error, or it may be due to
**	some other cause.
**
** System Status:
**	The journal file operation is rejected.
**
** Recommendation:
**	Analyze the specific operating system error message for information
**	about why the journal file could not be updated. Take the appropriate
**	actions to allow the DBMS to access the journal file (add more disk
**	space, alter the file security appropriately, repair the broken disk,
**	etc.).
*/
E_CL2409_JF_BAD_CREATE:SS08004_CONNECTION_REJECTED
"Error trying to create a journal file"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The operating system rejected an attempt to create a journal file.
**	The specific operating system error message is appended to this
**	error message, if it is available. The problem is often due to
**	insufficient disk space, or it may be due to a disk error or file
**	security problem.
**
** System Status:
**	The journal file operation is rejected.
**
** Recommendation:
**	Analyze the specific operating system error message for information
**	about why the journal file could not be created. Take the
**	appropriate actions to allow the DBMS to access the journal file (add
**	more disk space, alter the file security appropriately, repair the
**	broken disk, etc.).
*/
E_CL240A_JF_BAD_PARAM:SS50000_MISC_ERRORS
"A bad parameter was passed to a JF subroutine."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal error was detected by the subroutines which manage journal
**	files. An invalid parameter was passed to a journal file subroutine.
**	This can be caused by a program error or a memory corruption error.
**
** System Status:
**	The journal file operation is rejected.
**
** Recommendation:
**	This error message indicates a software problem. If the problem
**	can be reproduced, it should be reported to Ingres Corporation
**	Technical Support as a bug.
*/
E_CL240B_JF_NOT_FOUND:SS50000_MISC_ERRORS
"The specified journal file was not found."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A journal file was missing from the journal directory where the
**	the DBMS expected to find it. This message is followed in 
**	the error log by additional messages which specify which 
**	journal file was missing, and where it should have been located.
**
**	The most common reason for this error is that a journal file was
**	accidentally deleted.
**
** System Status:
**	Depending on the reason that the DBMS was trying to locate the journal
**	file, the system status will vary. If the DBMS was trying to locate the
**	journal file in order to add more records to it or to read records
**	from it, then processing stops until the journal file can be restored
**	from an operating system backup.
**
**	If the DBMS was simply attempting to delete the journal file, then
**	processing will continue (it is not considered a fatal error that the
**	journal file was missing, since the DBMS was planning to delete it
**	anyway).
**
** Recommendation:
**	If the journal file must be read or written, restore the journal file
**	from the appropriate operating system backup and retry the operation.
**
**	If the journal file was about to be deleted anyway, this message
**	is simply informational and no additional action is required.
*/
E_CL240C_JF_BAD_DELETE:SS08004_CONNECTION_REJECTED
"Error trying to delete a journal file"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The operating system rejected a request from the DBMS to delete this
**	journal file. The specific operating system error message is appended
**	to this error message, if it is available. The problem is often due to
**	a disk error or file security problem.
**
** System Status:
**	If possible, the DBMS processing will continue. The journal file which
**	could not be deleted is left in the journal directory. No subsequent
**	access to this journal file will be needed.
**
** Recommendation:
**	If the journal file must be read or written, restore the journal file
**	from the appropriate operating system backup and retry the operation.
**
**	If the journal file was about to be deleted anyway, this message
**	is simply informational and no additional action is required.
*/
E_CL240D_JF_DIR_NOT_FOUND:SS50000_MISC_ERRORS
"The specified journal file directory was not found."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The journal file directory could not be found where the DBMS expected to
**	find it. This message is followed in the error log by additional
**	messages which specify which journal directory was missing, and where
**	it should have been located.
**
**	The most common reason for this error is that the journal file
**	directory was accidentally deleted.
**
** System Status:
**	Depending on the reason that the DBMS was trying to locate the journal
**	file, the system status will vary. If the DBMS was trying to locate the
**	journal file in order to add more records to it or to read records
**	from it, then processing stops until the journal file can be restored
**	from an operating system backup.
**
**	If the DBMS was simply attempting to delete the journal file, then
**	processing will continue (it is not considered a fatal error that the
**	journal directory was missing, since the DBMS was planning to delete the
**	file anyway).
**
** Recommendation:
**	If the journal file must be read or written, restore the journal file
**	from the appropriate operating system backup and retry the operation.
**
**	If the journal file was about to be deleted anyway, this message
**	is simply informational and no additional action is required.
*/
E_CL240E_JF_BAD_LIST:SS08004_CONNECTION_REJECTED
"An error was encountered listing the contents of a journal file directory."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The operating system rejected an attempt to list the contents of a
**	journal file directory. The specific operating system error message is
**	appended to this error message, if it is available. The problem may be
**	due to file security violations, or to a disk or system error.
**
** System Status:
**	The journal file operation is rejected.
**
** Recommendation:
**	Analyze the specific operating system error message for information
**	about why the journal directory could not be examined. Take the
**	appropriate actions to allow the DBMS to list the journal directory
**	(alter the file security appropriately, repair the broken disk, etc.).
*/
E_CL240F_JF_EXISTS:SS50000_MISC_ERRORS
"A journal file by that name already exists."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The DBMS attempted to create a new journal file directory for a 
**	database, but the journal file directory already exists.
**
** System Status:
**	A message is printed indicating that a new journal file directory
**	could not be created in the indicated location.
**
** Recommendation:
**	Determine why the directory already exists. Examine the II_JOURNAL
**	definition for correctness. Retry the operation.
*/
E_CL2410_JF_BAD_DIR:SS50000_MISC_ERRORS
"Error in journal file directory specification."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The operating system rejected a journal file operation because the
**	journal file directory specification was in error. The specific
**	operating system error message is appended to this error message, if it
**	is available. The problem may be due to file security violations, or to
**	a disk or system error, or to an erroneous definition of an environment
**	variable.
**
** System Status:
**	The journal file operation is rejected.
**
** Recommendation:
**	Analyze the specific operating system error message for information
**	about why the journal directory could not be accessed. Take the
**	appropriate actions to allow the DBMS to access the journal directory
**	(alter the file security appropriately, repair the broken disk, etc.).
*/
E_CL2411_JF_BAD_TRUNCATE:SS08004_CONNECTION_REJECTED
"Error truncating a journal file."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The operating system rejected an attempt to truncate a journal file.
**	The specific operating system error message is appended to this error
**	message, if it is available. The problem may be due to a disk error.
**
** System Status:
**	The journal file operation is rejected.
**
** Recommendation:
**	Analyze the specific operating system error message for information
**	about why the journal file could not be truncated. Take the appropriate
**	actions to allow the DBMS to access the journal file (alter the file
**	security appropriately, repair the broken disk, etc.).
*/
E_CL2500_CS_OK:SS00000_SUCCESS
"All is fine."
E_CL2501_CS_INVALID_STATE:SS50000_MISC_ERRORS
"The Server control block indicates an invalid state."
E_CL2502_CS_SCB_ALLOC_FAIL:SS08004_CONNECTION_REJECTED
"Unable to allocate a session control block."
E_CL2503_CS_NOT_QUIESCENT:SS50000_MISC_ERRORS
"Server has active sessions:   Unable to perform shutdown."
E_CL2504_CS_BAD_PARAMETER:SS50000_MISC_ERRORS
"Invalid parameter on CS call."
E_CL2506_CS_TIMER_ERROR:SS50000_MISC_ERRORS
"Unable to queue timer request."
E_CL2507_CS_RDY_QUE_CORRUPT:SS50000_MISC_ERRORS
"The ready queue for the server is in an inconsistent state."
E_CL2508_CS_INTERRUPTED:SS50000_MISC_ERRORS
"The request was interrupted at user request."
E_CL2509_CS_TIMEOUT:SS50000_MISC_ERRORS
"The request has returned due to timeout."
E_CL250A_CS_NO_SEMAPHORE:SS50000_MISC_ERRORS
"The task requesting a semaphore operation does not own a semaphore."
E_CL250B_CS_COND_SIGNAL:SS50000_MISC_ERRORS
"Signaling a condition failed with %0d."
E_CL250C_CS_INVALID_MODE:SS50000_MISC_ERRORS
"The session control block indicates an invalid mode."
E_CL250D_CS_INVALID_RTN_MODE:SS50000_MISC_ERRORS
"The routine called by CS returned an invalid mode."
E_CL250E_CS_SESSION_LIMIT:SS50002_LIMIT_EXCEEDED
"Too many sessions have been requested."
E_CL250F_CS_REQUEST_ABORTED:SS50000_MISC_ERRORS
"The request has been aborted."
E_CL2510_CS_QUANTUM_INVALID:SS50000_MISC_ERRORS
"The user specified timer quantum is invalid."
E_CL2511_CS_BAD_CNX_STATUS:SS08006_CONNECTION_FAILURE
"Unable to make connection with the working partner."
E_CL2512_CS_BAD_ADMIN_SCB:SS50000_MISC_ERRORS
"The admin session has been called with a non-admin scb."
E_CL2513_CS_CORRUPT_ADMIN_SCB:SS50000_MISC_ERRORS
"The session control block for the admin SCB has been corrupted."
E_CL2514_CS_ESCAPED_EXCEPTION:SS50006_FATAL_ERROR
"An exception has escaped from user session.  This session will be terminated."
E_CL2515_CS_FORCED_EXIT:SS50006_FATAL_ERROR
"The server has been forced to exit involuntarily."
E_CL2516_CS_SYSTEM_ERROR:SS08004_CONNECTION_REJECTED
"The server has encountered an unexpected system error."
E_CL2517_CS_SMPR_DEADLOCK:SS40001_SERIALIZATION_FAIL
"A session has requested a semaphore which it already owns."
E_CL2518_CS_NORMAL_SHUTDOWN:SS00000_SUCCESS
"The Server has terminated normally."
E_CL2519_CS_INVALID_READY:SS50000_MISC_ERRORS
"A session has been RESUMED before it became runnable."
E_CL251A_CS_STACK_DEAL_FAIL:SS08004_CONNECTION_REJECTED
"OS Error deallocating a STACK region."
E_CL251B_CS_IDLE_RESUME:SS50000_MISC_ERRORS
"Attempt to resume the idle job"
E_CL251C_CS_TERM_W_SEM:SS50000_MISC_ERRORS
"Session terminated while holding semaphore(s)."
E_CL251D_NO_MORE_WRITES:SS50000_MISC_ERRORS
"No more writes needed."
E_CL251E_SYNC_COMPLETION:SS50000_MISC_ERRORS
"Read completed, but needs to sync I/O completion."
E_CL251F_CPUSTATS_WASSET:SS50000_MISC_ERRORS
"CPU statistics gathering is already enabled."
E_CL2520_CS_AMBIGUIOUS:SS50000_MISC_ERRORS
"%0c ambiguous"
E_CL2521_CS_ARG_UNKNOWN:SS50000_MISC_ERRORS
"argument %0c unknown"
E_CL2522_CS_NO_VALUE:SS50000_MISC_ERRORS
"value not supplied to %0c"
E_CL2523_CS_TOO_MANY_OPTIONS:SS50004_SYS_CONFIG_ERROR
"Too many options have been used, please re-issue with fewer options"
E_CL2524_CS_NON_NUMERIC_ARG:SS50000_MISC_ERRORS
"%0c requires non-numeric arguments"
E_CL2525_CS_NUMERIC_ARG:SS50000_MISC_ERRORS
"invalid numeric argument to %0c"
E_CL2526_CS_POSITIVE_VALUE:SS50000_MISC_ERRORS
"%0c value must be greater than zero"
E_CL2527_CS_PARSE_ERROR:SS50000_MISC_ERRORS
"internal error parsing options"
E_CL2528_CS_ARG_USAGE:SS50000_MISC_ERRORS
"Option names must begin with a '-'"
E_CL2529_CS_SEM_OWNER_DEAD:SS50000_MISC_ERRORS
"A process exited while holding the requested semaphore.   The semaphore
will never be granted so an error has been returned, if the calling routine
cannot cleanup whatever was left behind by the exited process it must exit
as the semaphore for the critical section cannot be granted."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An owner of a semaphore controlling access to a resource shared across
**	processes has exited.  The requesting process would "wait" forever for
**	access to this resource if this error were not returned (possibly
**	spinning, using all available CPU on the machine).
**
** System Status:
**	Either the server or the installation will be shut down (depending on
**	whether the resource is server- or installation-critical). In 6.3, there
**	is a single semaphore controlling access to the buffer manager which
**	is server-critical.  There are a few semaphores which control access to
**	the logging and locking components which are installation-critical.
**
** Recommendation:
**	Either restart the server or the installation, depending on whether the
**	server or the installation was shut down.  Information in the error log
**	may point to the cause of the semaphore failure.
*/
E_CL252A_CS_FLOAT_ARG:SS50000_MISC_ERRORS
"Invalid floating-point parameter for %0c.\n
Please check to ensure valid floating-point syntax is provided,\n
including the decimal point."
/*%
** Parameters:
**	%0c - parameter name
**
** Explanation:
**	An invalid value was provided for a floating point server startup
**	parameter.
**
** System Status:
**	The attempt to start the server fails.
**
** Recommendation:
**	Respecify the parameter with a valid floating point value.
*/
E_CL252B_CS_NONNEGATIVE_VALUE:SS50000_MISC_ERRORS
"%0c value must be greater than, or equal to, zero.\n
Please check to ensure you specified a non-negative value\n
for this parameter."
/*%
** Parameters:
**	%0c - parameter name
**
** Explanation:
**	A negative value was specified for a server startup parameter
**	that requires a non-negative value.
**
** System Status:
**	The attempt to start the server fails.
**
** Recommendation:
**	Respecify the parameter with a non-negative value.
*/
E_CL252C_CS_FRACTION_VALUE:SS50000_MISC_ERRORS
"%0c value must be between 0.0 and 1.0 \n
Please check to ensure you specified a non-negative value\n
for this parameter."
/*%
** Parameters:
**	%0c - parameter name
**
** Explanation:
**	A non-fraction value was specified for a server startup parameter
**	that requires 0 < value < 1.0.
**
** System Status:
**	The attempt to start the server fails.
**
** Recommendation:
**	Respecify the parameter with a fraction.
*/
E_CL252D_CS_DEFINE:SS50000_MISC_ERRORS
"Unable to set II_DBMS_SERVER."
E_CL252E_CS_SHORT_FDS:SS50000_MISC_ERRORS
"Unable to allocate enough file descriptors to support the requested\n
connection limit.  The connection limit has been lowered to %0d."
E_CL252F_NO_FREE_STACKS:SS50000_MISC_ERRORS
"Stack sharing: no session is currently in a state where it \n
can give up its stack for another session."
E_CL2530_CS_PARAM:SS5000F_TEXTUAL_INFO
""
/*%
** Parameters:
**	None.
**
** Explanation:
**	This message is issued by iirundbms for each parameter specified
**	and passed to the server.  The actual parameter names and values
**	are instantiated by iirundbms.
**
** System Status:
**	This is an informational message; startup continues.
**
** Recommendation:
**	None.
*/
E_CL2531_CS_ECHO_MISSING:SS50000_MISC_ERRORS
"Unable to locate template message to echo startup parameters.\n
Server startup parameters will not be echoed to errlog."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This message is issued by iirundbms if the echo template message
**	(E_CL2530_CS_PARAM) could not be located in the error message
**	file.  Since this message is in the same file, it is unlikely
**	that the text of this message will be located as well, but at
**	least we'll have a message ID to commence diagnosis.
**
** System Status:
**	The server startup parameters will not be echoed.
**
** Recommendation:
**	Ensure the installation process is complete.
**	Contact Ingres Corporation Technical Support.
*/
E_CL2532_CS_ECHO_SHORT:SS50000_MISC_ERRORS
"Template message for echoing startup parameters\n
is too short to contain parameter names and values.\n
Server startup parameters will not be echoed to errlog."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This message is issued by iirundbms if the echo template message
**	(E_CL2530_CS_PARAM) is too short to contain both the parameter
**	name and parameter value.
**
** System Status:
**	The server startup parameters will not be echoed.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_CL2533_CS_ECHO_ERROR:SS50000_MISC_ERRORS
"Unable to write message for echoing startup parameters\g
to the error log.\n
Server startup parameters will not be echoed to errlog."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This message is issued by iirundbms if an attempt to write
**	the echo message (E_CL2530_CS_PARAM) to the error log fails.
**
** System Status:
**	The server startup parameters will not be echoed.
**
** Recommendation:
**	Ensure there is enough disk space for the error log file.
**	Contact Ingres Corporation Technical Support.
*/
E_CL2534_CS_INV_CONTROL:SS50000_MISC_ERRORS
"An invalid control mask <%0x> was passed to iirundbms.\n
This control request will be ignored."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is an internal error with iirundbms; an unrecognized request
**	for iirundbms control was encountered.  This should never occur.
**
** System Status:
**	Processing of iirundbms continues.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_CL2535_CS_GETPWNAM_FAIL:SS50000_MISC_ERRORS
"The UNIX getpwnam() function call has failed."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is an internal error within the call used to initialize the
**	CS subsystem.  This should never occur.
**
** System Status:
**	Server or utility program startup will fail.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_CL2536_CS_SETEUID_FAIL:SS50000_MISC_ERRORS
"The UNIX seteuid() function call has failed."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is an internal error within the call used to initialize the
**	CS subsystem.  This should never occur.
**
** System Status:
**	Server or utility program startup will fail.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_CL2537_CS_MAP_SSEG_FAIL:SS50000_MISC_ERRORS
"Failure in attempt to map the UNIX system segment (shared memory).  The
most common causes of this problem are: 1) user does not have the required
privileges; 2) the shared memory has not been installed.  If not
installed, the CSINSTALL utility can be used to install the shared memory
segment."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is an indication that the calling program could not attach to
**	the shared memory known as the system segment.   The most common 
**	causes of this problem are that either the data segment has not been 
**	installed (via CSINSTALL), or that the program or user encountering
**	the problem is not running with adequate privileges.
**
** System Status:
**	Server or utility program startup will fail.
**
** Recommendation:
**	If not running with adequate privileges, determine and obtain 
**	required privs and try again.  If running with adequate 
**	privileges, and if the installation is not up, use CSINSTALL 
**	and to install the system segment and try again.  
*/
E_CL2538_CS_EVINIT_FAIL:SS50000_MISC_ERRORS
"Failure in attempt to initialize the CS event subsystem."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is an indication that a utility program failed in its
**	attempt to initialize the CS event subsystem.  This may be the result 
**	of insufficient privileges, or could indicate corruption of
**	shared memory.
**
** System Status:
**	Utility program startup will fail.
**
** Recommendation:
**	If the installation is down, reinitialize the system segment with 
**	CSINSTALL and try again.  If the installation is up, save the 
**	errlog.log, iircp.log and all other ".log" files, and notify
**   	Ingres Corporation technical support.
**	
*/
E_CL2540_CS_ACCT_LOCATE:SS50000_MISC_ERRORS
"Unable to locate the session accounting file, iiacct.\n
Session accounting is disabled."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An attempt to locate the session accounting file, iiacct,
**	failed.
**
** System Status:
**	Session accounting is disabled.
**
** Recommendation:
**	The iiacct file resides in the same directory as the error log
**	file. Ensure that the directory structure is intact.
*/
E_CL2541_CS_ACCT_OPEN:SS50000_MISC_ERRORS
"Unable to open the session accounting file, iiacct.\n
Session accounting is disabled."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An attempt to open the session accounting file, iiacct,
**	failed.
**
** System Status:
**	Session accounting is disabled.
**
** Recommendation:
**	Ensure there is enough space on the device containing
**	the iiacct file.
*/
E_CL2542_CS_ACCT_WRITE:SS50000_MISC_ERRORS
"Unable to write to the session accounting file, iiacct.\n
Session accounting is suspended."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An attempt to write to the session accounting file, iiacct,
**	failed.
**
** System Status:
**	Session accounting is suspended, and the session's
**	accounting record is lost.  Session accounting is
**	reactivated when another session successfully attempts
**	to write to the iiacct file.
**
** Recommendation:
**	Ensure there is enough space on the device containing
**	the iiacct file.
*/
E_CL2543_CS_ACCT_RESUMED:SS50000_MISC_ERRORS
"Session accounting has been resumed."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An attempt to write to the session accounting file, iiacct,
**	succeeded after previous unsuccessful attempt(s).
**
** System Status:
**	Session accounting is resumed.  The session accounting
**	records that could not be written to the iiacct file
**	were lost.
**
** Recommendation:
**	None.
*/
E_CL2544_CS_CAUSE_EVENT:SS50000_MISC_ERRORS
"Error causing an event to begin; evcb flags %0d"
/*%
** Parameters:
**	%0d - event control block flags
**
** Explanation:
**	A call to CScause_event() returned an error.
**
** System Status:
**	All queries encountering this error will fail.
**
** Recommendation:
**	None.
*/
E_CL2545_CS_FREE_EVENT:SS50000_MISC_ERRORS
"Error freeing an event control block"
/*%
** Parameters:
**	None.
**
** Explanation:
**	A call to CSfree_event() returned an error.
**
** System Status:
**	The system will eventually run out of event control blocks.
**
** Recommendation:
**	None.
*/
E_CL2546_CS_NO_START_PROF:SS50000_MISC_ERRORS
"An attempt was made to stop profiling collection for a facility before it
was started, this is probably due to a coding inconsistency. Profiling
data will be invalid for all facilities."
/*%
** Parameters:
**      None.
**
** Explanation:
**      The server has been closed to new connections.
**
** Recommendation:
**      None.
*/
E_CL2547_CS_SHUT_STATE:SS50000_MISC_ERRORS
"The server is shutting down. No new sessions can be created."

/* IMA-III related */

E_CL2550_CS_SEMOP_GET:SS50000_MISC_ERRORS
"Error requesting a semaphore"
E_CL2551_CS_MSET:SS50000_MISC_ERRORS
"Error requesting an mset semaphore"
E_CL2552_CS_SLEEP_BOTCH:SS50000_MISC_ERRORS
"No way to sleep in this system!"
E_CL2553_CS_SEMOP_RELEASE:SS50000_MISC_ERRORS
"Error releasing a semaphore"
E_CL2554_CS_MCLEAR:SS50000_MISC_ERRORS
"Error releasing semaphore with mclear"
E_CL2555_CS_WAKEUP_BOTCH:SS50000_MISC_ERRORS
"No way to wakeup on this system!"

E_CL2556_CS_CANNOT_SUSPEND:SS50000_MISC_ERRORS
"Cannot suspend a thread that is not the current thread."

E_CL2557_CS_INTRN_THRDS_NOT_SUP:SS0A000_FEATUR_NOT_SUPPORTED
"Using II_THREAD_TYPE set to INTERNAL is not supported in this CL"


E_CL25F5_KPB_ALLOC_ERROR:SS50000_MISC_ERRORS
"Unexpected error allocating a session KP Services block."
E_CL25F6_MEM_STK_ALLOC_ERROR:SS50000_MISC_ERRORS
"Unexpected error allocating a session memory stack of length %0d."
E_CL25F7_CSMBXCRE_ERROR:SS50000_MISC_ERRORS
"Unexpected error creating system-wide %0c mailbox. Check resources, quotas,
permissions, and privileges."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This message is issued when a process is unable to create the system-
**	wide Database System Administrator mailbox which is used by the 
**	DBMS server processes for coordination and synchronization.
**
**	The process encountered an unexpected error trying to create the
**	mailbox.
**
** System Status:
**	The process does not start.
**
** Recommendation:
**	System error logs may provide more detail. Check for system-wide
**	resource exhaustion problems. Also check for sufficient privileges and
**	resources quotas by the process starting up.
*/
E_CL25F8_CSMBXCRE_NOPRIV:SS50000_MISC_ERRORS
"Insufficient privileges to create system-wide %0c mailbox."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This message is issued when a process is unable to create the system-
**	wide Database System Administrator mailbox which is used by the DBMS 
**	server processes for coordination and synchronization.
**
**	The process has insufficient privileges to create the mailbox.
**
** System Status:
**	The process does not start.
**
** Recommendation:
**	The process must have PRMMBX privilege in order to start up correctly.
*/
E_CL25F9_CS_2STK_OVFL:SS50006_FATAL_ERROR
"Stack overflow while correcting previous stack overflow.  The server will be terminated."
E_CL25FA_CS_UNWIND_FAILURE:SS08004_CONNECTION_REJECTED
"OS error while attempting to unwind an exception stack."
E_CL25FB_CS_UNDELIV_AST:SS08004_CONNECTION_REJECTED
"VMS was unable to deliver an AST to a thread."
E_CL25FC_CS_CRTCL_RESRC_HELD:SS50000_MISC_ERRORS
"Thread encountered a thread fatal error while holding a critical resource."
E_CL25FD_CS_THREAD_STK_OVFL:SS50006_FATAL_ERROR
"Thread operating for <%0c> has exhausted stack space (%1d. bytes).  The thread
will be terminated.  \nPlease consult your DBMS system administrator.  \nNOTE:
The stack space can be expanded with the use of the stack_size config.dat
parameter."
E_CL25FE_CS_25FE_RET_FROM_IDLE:SS50006_FATAL_ERROR
"The idle thread has erroneously returned.  This is server fatal."
E_CL25FF_CS_FATAL_ERROR:SS50006_FATAL_ERROR
"The server has encountered a FATAL error.  The server will be terminated."
E_CL2601_CI_NOSTR:SS50004_SYS_CONFIG_ERROR
"The authorization string (II_AUTH_STRING) is not available.
Please consult the DBMS system manager."
E_CL2602_CI_TOOLITTLE:SS50004_SYS_CONFIG_ERROR
"The authorization string (II_AUTH_STRING) contains too few characters.
Please consult the DBMS system manager."
E_CL2603_CI_TOOBIG:SS50004_SYS_CONFIG_ERROR
"The authorization string (II_AUTH_STRING) contains too many characters.
Please consult the DBMS system manager."
E_CL2604_CI_BADCHKSUM:SS50004_SYS_CONFIG_ERROR
"The authorization string (II_AUTH_STRING) checksums incorrectly.  This
is probably a typographical input error.
Please consult the DBMS system manager."
E_CL2605_CI_BADEXPDATE:SS50004_SYS_CONFIG_ERROR
"The authorization string (II_AUTH_STRING) indicates that your license has
expired.  Please consult the DBMS system manager."
E_CL2606_CI_BADSERNUM:SS50004_SYS_CONFIG_ERROR
"The authorization string (II_AUTH_STRING) does not match the serial number of
the machine on which you are running.
Please consult the DBMS system manager."
E_CL2607_CI_BADCPUMODEL:SS50004_SYS_CONFIG_ERROR
"The authorization string (II_AUTH_STRING) does not match the CPU model of
the machine on which you are running.
Please consult the DBMS system manager."
E_CL2608_CI_BADERROR:SS50004_SYS_CONFIG_ERROR
"An unexpected error occurred during authorization string processing.
Please consult the DBMS system manager."
E_CL2609_CI_BADKEY:SS50004_SYS_CONFIG_ERROR
"The authorization string (II_AUTH_STRING) was improperly created for this
version of the software.
Please consult the DBMS system manager."
E_CL260A_CI_NOCLUSTER:SS50004_SYS_CONFIG_ERROR
"The current installation indicates that VAXcluster processing is desired, but
the authorization string (II_AUTH_STRING) does not provide VAXcluster
processing.
Please consult the DBMS system manager."
E_CL2614_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (DBMS) capability is not provided by the authorization string
(II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL2615_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (VAR_INGRES) capability is not provided by the authorization
string (II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL2616_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (NET) capability is not provided by the authorization string
(II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL2617_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (QUEL TM) capability is not provided by the authorization string
(II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL2618_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (SQL TM) capability is not provided by the authorization string
(II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL2619_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (ABF) capability is not provided by the authorization string
(II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL261A_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (GBF) capability is not provided by the authorization string
(II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL261B_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (VIGRAPH) capability is not provided by the authorization string
(II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL261C_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (MENU) capability is not provided by the authorization string
(II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL261D_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (OSLQUEL) capability is not provided by the authorization string
(II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL261E_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (OSLSQL) capability is not provided by the authorization string
(II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL261F_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (QBF) capability is not provided by the authorization string
(II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL2620_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (RBF) capability is not provided by the authorization string
(II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL2621_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (REPORT) capability is not provided by the authorization string
(II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL2622_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (VIFRED) capability is not provided by the authorization string
(II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL2623_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (Equel/ADA) capability is not provided by the authorization string
(II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL2624_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (Equel/BASIC) capability is not provided by the authorization
string (II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL2625_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (Equel/C) capability is not provided by the authorization string
(II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL2626_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (Equel/COBOL) capability is not provided by the authorization
string (II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL2627_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (Equel/FORTRAN) capability is not provided by the authorization
string (II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL2628_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (Equel/PASCAL) capability is not provided by the authorization
string (II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL2629_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (Equel/PL-1) capability is not provided by the authorization
string (II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL262A_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (ESQL/ADA) capability is not provided by the authorization string
(II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL262B_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (ESQL/BASIC) capability is not provided by the authorization
string (II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL262C_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (ESQL/C) capability is not provided by the authorization string
(II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL262D_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (ESQL/COBOL) capability is not provided by the authorization
string (II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL262E_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (ESQL/FORTRAN) capability is not provided by the authorization
string (II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL262F_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (ESQL/PASCAL) capability is not provided by the authorization
string (II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL2630_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (ESQL/PL-1) capability is not provided by the authorization string
(II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL2631_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (PC-LINK) capability is not provided by the authorization string
(II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL2632_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (VAXcluster) capability is not provided by the authorization
string (II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL2633_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (BLOCK/3270 Mode) capability is not provided by the authorization string
(II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL2634_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (Distributed DBMS) capability is not provided by the authorization string
(II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL263B_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (Vision) capability is not provided by the authorization
string (II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL2643_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (Windows 4GL) capability is not provided by the authorization
string (II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL2644_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (Windows 4GL/Runtime) capability is not provided by the
authorization string (II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL2645_CI_BADCAP:SS50004_SYS_CONFIG_ERROR
"The required (Conversion Tools) capability is not provided by the authorization
string (II_AUTH_STRING).
Please consult the DBMS system manager."
E_CL2650_CI_NO_LICENSE:SS50004_SYS_CONFIG_ERROR
"%0c"
E_CL2651_CI_WG_COUNT:SS50004_SYS_CONFIG_ERROR
"%0c"
E_CL2652_CI_EXPIRED:SS50004_SYS_CONFIG_ERROR
"%0c"
E_CL2653_CI_WILL_EXPIRE:SS50004_SYS_CONFIG_ERROR
"%0c"
E_CL2654_CI_CANT_OPEN:SS50004_SYS_CONFIG_ERROR
"%0c"
E_CL2655_CI_CORR_FILE:SS50004_SYS_CONFIG_ERROR
"%0c"
E_CL2656_CI_CANT_READ:SS50004_SYS_CONFIG_ERROR
"%0c"
E_CL2657_CI_MAC_SERIAL:SS50004_SYS_CONFIG_ERROR
"%0c"
E_CL2658_CI_MACHINE_TYPE:SS50004_SYS_CONFIG_ERROR
"%0c"
E_CL2659_CI_TERMINATE:SS50004_SYS_CONFIG_ERROR
"%0c"
E_CL2660_CI_LICENSE_ERR:SS50004_SYS_CONFIG_ERROR
"Ingres Corporation Licensing Error"
I_CL2661_CI_SITEID:SS50004_SYS_CONFIG_ERROR
"This product/program is licensed to: %0c Site ID: %1c"
E_CL2662_CI_BADSITEID:SS50004_SYS_CONFIG_ERROR
"The Company Name or Site ID in the license file is not valid."
E_CL2781_GC_CONNECT_FAIL:SS50000_MISC_ERRORS
"An attempted network connection failed"
E_CL2782_GC_DISCONNECT_FAIL:SS50000_MISC_ERRORS
"An attempted network disconnect failed"
E_CL2783_GC_SEND_FAIL:SS50000_MISC_ERRORS
"An attempted network send failed"
E_CL2784_GC_RECEIVE_FAIL:SS50000_MISC_ERRORS
"An attempted network receive failed"
E_CL2785_GC_LISTEN_FAIL:SS50000_MISC_ERRORS
"An attempted network listen failed"
E_CL2786_GC_PINIT_FAIL:SS50000_MISC_ERRORS
"Protocol initialization failed"
E_CL2787_GC_OPEN_FAIL:SS50000_MISC_ERRORS
"An attempted network open failed"
E_CL2788_GC_NTWK_STATUS:SS50000_MISC_ERRORS
"Error status returned by network"
E_CL2801_SD_EOT:SS50000_MISC_ERRORS
"SDunpack: No more tuples on the current page"
E_CL2802_SD_LASTPAGE:SS50000_MISC_ERRORS
"SDpageread: The (possibly empty) last page has been returned"
E_CL2803_SD_BADCOND:SS50000_MISC_ERRORS
"SDpageread error setting up condition; log file contains error dump"
E_CL2804_SD_BADPARAM:SS50000_MISC_ERRORS
"Invalid parameter(s) passed to routine"
E_CL2805_SD_NOCOND:SS50000_MISC_ERRORS
"Failure to set up condition with SDcompile prior to use in SDpageread"
E_CL2806_SD_SHORT_TSB:SS50000_MISC_ERRORS
"The TSB buffer supplied was too short (system limit)"
E_CL2807_SD_ERROR:SS50000_MISC_ERRORS
"Errors in input to SD compilation, see/obtain trace information"
E_CL2808_SD_NOSD:SS50000_MISC_ERRORS
"System will detect this response and not use Smart Disk"
E_CL2809_SD_SHORT_BUF:SS50000_MISC_ERRORS
"The buffer supplied for literal values was too short (system limit)"
E_CL280A_SD_VME_ERROR:SS50000_MISC_ERRORS
"An error has been reported from a VME interface"
E_CL280B_SD_BAD_ID:SS50000_MISC_ERRORS
"The Smart Disk identifier supplied is not supported"
E_CL2901_SL_CMP_ERROR:SS40003_STMT_COMPL_UNKNOWN
"Error comparing security labels"
E_CL2902_SL_CNVT_ERROR:SS40003_STMT_COMPL_UNKNOWN
"Error converting security label"
E_CL2903_SL_LABEL_ERROR:SS08007_XACT_RES_UNKNOWN
"Error obtaining security label associated with process"

/*
** 27-Mar-2001 (devjo01)
**
**	CX facility provides an OS independent API for Ingres
**	cluster services, including the DLM 'D'istributed "L'ock
**	'M'anager,  a broadcast message facility, and a CMO
**	'C'luster 'M'emory 'O'bject sub-facility.
**
**	Errors should not typically get passed all the way up
**	to SQL, but will be converted within LK to LK codes.
**
**	'I' messages are informational
**	'W' messages indicate successful completion of operation,
**	    but indicate additional work may need to be done.
**	'E' messages indicate operation failed.  Most 'E'
**	    codes indicate a severe problem, but DEADLOCK
**	    code can be generated in normal operation.
*/
E_CL2C01_CX_I_OKSYNC:SS50000_MISC_ERRORS
"Distributed lock was granted or converted synchronously"
/*%
** Parameters:
**	None.
**
** Explanation:
**	No need to suspend session, lock was granted synchronously.
**
** System Status:
**	All is well. 
**
** Recommendation:
**	Stay the course.
*/
E_CL2C02_CX_I_OKASYNC:SS50000_MISC_ERRORS
"Distributed lock was granted or converted asynchronously"
/*%
** Parameters:
**	None.
**
** Explanation:
**	Internal code set in cx_status on asynchronous grant.
**
** System Status:
**	All is well. 
**
** Recommendation:
**	Stay the course.
*/
E_CL2C04_CX_I_OKINTR:SS50000_MISC_ERRORS
"Distributed lock or conversion asynchronously granted, but user interrupt received."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Asynchronous grant was successful, but session received
**	an interrupt.
**
** System Status:
**	All is well. 
**
** Recommendation:
**	None, Higher level code will decide on action.
*/
E_CL2C05_CX_I_DONE:SS50000_MISC_ERRORS
"Iterative process has completed."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An operation which may span several calls, is entirely
**	and successfully finished.  Mainly an internal code.
**
** System Status:
**	All is well. 
**
** Recommendation:
**	Stay the course.
*/
E_CL2C08_CX_W_SUCC_IVB:SS50000_MISC_ERRORS
"Distributed lock converted or granted, but value block read is marked invalid."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Lock was granted or converted, but lock value block
**	was marked invalid.  This is most likely because a 
**	process holding lock in an update mode has died.
**
** System Status:
**	Tolerable internal failure.
**
** Recommendation:
**	Internal Ingres status code.  Transactions opened by
**	abended process will be rolled back, and lock value
**	revalidated.
*/
E_CL2C09_CX_W_CANT_CAN:SS50000_MISC_ERRORS
"Distributed lock cancel unsuccessful, target request was granted."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Before distributed lock request could be canceled, it was
**	granted.  This typically would happen if lock was requested
**	with a time-out specified.
**
** System Status:
**	All is well. 
**
** Recommendation:
**	Completion routine for lock will run, setting final code.
**	No intervention required.
*/
E_CL2C0A_CX_W_TIMEOUT:SS50000_MISC_ERRORS
"Distributed lock wait timed out."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Specified lock timeout expired before lock could be granted.
**
** System Status:
**	All is well. 
**
** Recommendation:
**	Stay the course.
*/
E_CL2C0B_CX_W_INTR:SS50000_MISC_ERRORS
"Distributed lock wait interrupted. Request cancelled."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Session waiting for a DLM lock was interrupted, and pending
**	request was successfully canceled.
**
** System Status:
**	All is well. 
**
** Recommendation:
**	Higher level code is responsible for handling the session interrupt.
*/
E_CL2C0C_CX_W_INTR_IVB:SS50000_MISC_ERRORS
"Distributed lock wait interrupted, but request was granted, but value found invalid."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Effectively a combination of E_CL2C04_CX_I_OKINTR &
**	E_CL2C08_CX_W_SUCC_IVB.
**
** System Status:
**	All is well. 
**
** Recommendation:
**	Higher level code is responsible for handling the session interrupt.
*/
E_CL2C10_CX_E_NOSUPPORT:SS50000_MISC_ERRORS
"Clustering is not supported or is not configured by the OS."
/*%
** Parameters:
**	None.
**
** Explanation:
**	OS does not, or is not configured to support the facilities
**	needed by Ingres clusters.
**
** System Status:
**	CX facility is not enabled.  Cannot use Ingres clustering.
**	If Installation is configured for Ingres clusters, startup will fail.
**
** Recommendation:
**	If cluster support is expected, make sure OS clustering is
**	properly set up.
*/
E_CL2C11_CX_E_BADPARAM:SS50000_MISC_ERRORS
"Invalid parameter passed to CX routine."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Internal logic error.  Passed parameter is out of range.
**
** System Status:
**	Clustered Ingres may not be usable, depending on frequency,
**	location of error. 
**
** Recommendation:
**	Contact technical support.
*/
E_CL2C12_CX_E_INSMEM:SS50000_MISC_ERRORS
"CX facility could not allocate required memory resources."
/*%
** Parameters:
**	None.
**
** Explanation:
**	CX facility could not allocate required memory resources.
**	Not all implementations allocate memory.
**
** System Status:
**	Clustered Ingres may not be usable, depending on frequency,
**	location of error. 
**
** Recommendation:
**	Contact technical support.
*/
E_CL2C13_CX_E_NOTCONFIG:SS50000_MISC_ERRORS
"Ingres is not configured for cluster support."
/*%
** Parameters:
**	None.
**
** Explanation:
**	CXconnect was called, but Ingres is not configured for
**	clustering.  This is an internal logic error, since
**	we should not call this unless clustering is configured.
**
** System Status:
**	CX facility is not enabled.  Cannot use Ingres clustering.
**	Startup will fail.
**
** Recommendation:
**	Contact technical support.
*/
E_CL2C14_CX_E_NOCSP:SS50000_MISC_ERRORS
"A CSP must be active on node before a non-CSP CX user can start."
/*%
** Parameters:
**	None.
**
** Explanation:
**	CSP runs as part of RCP.  Installation startup should assure
**	RCP is up prior to starting regular servers.
**
** System Status:
**	CX facility is not enabled. Startup will fail.
**
** Recommendation:
**	Contact technical support.
*/
E_CL2C15_CX_E_SECONDCSP:SS50000_MISC_ERRORS
"Only one CSP per node."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Second CSP (RCP) is attempting to start, but a CSP is, or
**	appears to be, already up.
**
** System Status:
**	Startup will fail.
**
** Recommendation:
**	Contact technical support.
*/
E_CL2C16_CX_E_BADCONFIG:SS50000_MISC_ERRORS
"Configured node number is out of range or was added after startup."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Installation has this node configured with an invalid node
**	number ( valid is 1 .. 32 ), or node number was added to
**	cluster configuration after a CSP was started on at least
**	one node.
**
** System Status:
**	Startup will fail.
**
** Recommendation:
**	Check configuration.  If valid, restart Installation on all nodes.
*/
E_CL2C17_CX_E_BADCONFIG:SS50000_MISC_ERRORS
"Invalid value for ii.$.config.cmotype."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Configured CMOTYPE is not DLM, IMC, or DEFAULT.
**	IMC is only supported on Tru64 UNIX.
**
** System Status:
**	CX facility is not enabled.  Cannot use Ingres clustering.
**	Startup will fail.
**
** Recommendation:
**	Check configuration.
*/
E_CL2C18_CX_E_BADCONFIG:SS50000_MISC_ERRORS
"OS specific configuration error."
/*%
** Parameters:
**	None.
**
** Explanation:
**	OS specific configuration error. 
**	For Tru64 & Linux, this is a bad dlmsignal
**
** System Status:
**	CX facility is not enabled.  Cannot use Ingres clustering.
**	Startup will fail.
**
** Recommendation:
**	Check configuration.
*/
E_CL2C19_CX_E_BADCONFIG:SS50000_MISC_ERRORS
"OS specific configuration error."
/*%
** Parameters:
**	None.
**
** Explanation:
**	OS specific configuration error. 
**	For Tru64 this is a bad IMC rail (s/b 0,1,2, or 3)
**
** System Status:
**	CX facility is not enabled.  Cannot use Ingres clustering.
**	Startup will fail.
**
** Recommendation:
**	Check configuration.
*/
E_CL2C1A_CX_E_CORRUPT:SS50000_MISC_ERRORS
"Internal inconsistency within CX facility."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A miscellaneous consistency check within the CX
**	facility found a problem.  Surrounding errors
**	may help establish the context, but this error
**	should be reported to technical support.
**
** System Status:
**	Current operation will fail.  May indicate a 
**	severe problem.
**
** Recommendation:
**	Contact technical support.
*/
E_CL2C1B_CX_E_BADCONTEXT:SS50000_MISC_ERRORS
"CX facility call made in a bad context."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A coding logic error led to a CX function being
**	called before needed prerequisites had been
**	successfully completed.  This should not happen,
**	and indicates an oversight, or an unhandled
**	exception case within CX.
**
** System Status:
**	Current operation will fail.  May indicate a 
**	severe problem.
**
** Recommendation:
**	Contact technical support.
*/
E_CL2C1C_CX_E_LSN_LVB_FAILURE:SS50000_MISC_ERRORS
"Error propagating LSN in LVB.  Internal status=%0d, modes %1d -> %2d, lock key = %3c"
/*%
** Parameters:
**	%0d	- Internal status code (in decimal)
**	%1d	- Old mode.      
**	%2d	- New mode.      
**	%3c	- Lock Key.
**
** Explanation:
**	A failure occured during synchronizing the internal CX LSN from
**	the value contained in a lock value block.  This is an unexpected
**	condition which might compromise the integrity of the transaction
**	system.
**
** System Status:
**	Current operation will fail.  Transaction should rollback.
**	May indicate a severe problem.
**
** Recommendation:
**	Contact technical support.
*/
E_CL2C20_CX_E_NODLM:SS50000_MISC_ERRORS
"Vendor provided DLM is unavailable."
/*%
** Parameters:
**	None.
**
** Explanation:
**	We are using a vendor provided DLM on this platform, and
**	DLM is either not configured, or not enabled.
**
** System Status:
**	CX facility is not enabled.  Cannot use Ingres clustering.
**	Startup will fail.
**
** Recommendation:
**	Contact OS vendor, and or Ingres technical support.
*/
E_CL2C21_CX_E_DLM_DEADLOCK:SS50000_MISC_ERRORS
"Distributed lock request/conversion failed due to deadlock."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Lock request formed part of a deadlock cycle, and was
**	selected as the deadlock victim.
**
** System Status:
**	Normal operation.
**
** Recommendation:
**	Higher level code will process this.
*/
E_CL2C22_CX_E_DLM_CANCEL:SS50000_MISC_ERRORS
"Distributed lock request/conversion canceled."
/*%
** Parameters:
**	None.
**
** Explanation:
**	When a queued lock request is cancelled this code
**	is set by completion routine.
**
** System Status:
**	Normal operation.
**
** Recommendation:
**	Higher level code will process this.
*/
E_CL2C23_CX_E_DLM_CONNECTION:SS50000_MISC_ERRORS
"Problems connecting to, or disconnecting from DLM."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Problem starting up, or shutting down CX facility.
**
** System Status:
**	Startup may fail, or process abend.
**
** Recommendation:
**	Contact technical support.
*/
E_CL2C24_CX_E_DLM_SPINOUT:SS50000_MISC_ERRORS
"Failure generating unique id."
/*%
** Parameters:
**	None.
**
** Explanation:
**	CXunique_id failed because after 2**24 attempts, it was 
**	unable to get exclusive access to unique id counter.
**	Most likely explanation is that a process died in the
**	tiny interval when it owned this counter.
**
** System Status:
**	New transactions will be unable to start, plus other
**	severe compromises to installation functionality.
**
** Recommendation:
**	Shutdown and restart installation.  If problem persists,
**	or if no explicable process failure had occurred, contact
**	technical support.
*/
E_CL2C25_CX_E_DLM_NOTHELD:SS50000_MISC_ERRORS
"Converting or releasing a distributed lock not granted."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Attempting to unlock/convert a lock not held.   Probable internal
**	logic error.
**
** System Status:
**	Lock request will fail.
**
** Recommendation:
**	Contact technical support.
*/
E_CL2C26_CX_E_DLM_ABORTED:SS50000_MISC_ERRORS
"Distributed lock administration action aborted/interrupted."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Attempt to query OS DLM was interrupted.
**
** System Status:
**	Current operation will fail.
**
** Recommendation:
**	Contact technical support.
*/
E_CL2C27_CX_E_DLM_NOGRANT:SS50000_MISC_ERRORS
"Request or convert could not be synchronously granted & NOWAIT set."
/*%
** Parameters:
**	None.
**
** Explanation:
**	CX_F_NOWAIT was specified to a call to CXdlm_request, or
**	CXdlm_convert, and lock was held by another session
**	in an incompatible mode.
**
** System Status:
**	Normal operation.
**
** Recommendation:
**	All is well.
*/
E_CL2C28_CX_E_DLM_CMPLERR:SS50000_MISC_ERRORS
"Failure with asynchronous completion."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Completion routine for an enqueued lock request was triggered,
**	but internal consistency checks failed.  This is an unexpected
**	error, and should be reported to technical support.
**
** System Status:
**	Lock request fails. 
**
** Recommendation:
**	Contact technical support.
*/
E_CL2C29_CX_E_DLM_BADVERSION:SS50000_MISC_ERRORS
"Incompatible version of DLM."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An incompatible version of a 3rd Party DLM is installed.
**
** System Status:
**	CX facility is not enabled.  Cannot use Ingres clustering.
**	Startup will fail.
**
** Recommendation:
**	Contact technical support.  If you know version is 
**	actually compatible with Ingres, you can set config
**	parameter ii.$.config.dlmversion to the DLM version.
**	Setting ii.$.config.dlmverfile to "none" will also
**	disable file based version checking.
*/
E_CL2C2A_CX_E_DLM_RD_DIRTY:SS50000_MISC_ERRORS
"Process detached from recovery domain containing invalidated locks."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Process detached from recovery domain which still contained
**	invalidated locks.
**
** System Status:
**	Can only occur when using a DLM.  Only of concern if process
**	reporting this was the RCP (Recovery Process).  Error is
**	informational in all other contexts and is usually suppressed.
**
** Recommendation:
**	Contact technical support, recycle DLM if reported from
**	Recovery Process.
*/
E_CL2C2C_CX_E_MSG_NOSUCH:SS50000_MISC_ERRORS
"CX message facility went to retrieve a message, but message was not there."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Benign internal error code.
**
** System Status:
**	All is well.
**
** Recommendation:
**	Stay the course.
*/
E_CL2C2D_CX_E_NOBIND:SS50000_MISC_ERRORS
"Process failed to bind to RAD %d."
/*%
** Parameters:
**	Ingres RAD ID for which bind failed.
**
** Explanation:
**	Attempt to have OS affine calling process to a certain RAD failed.
**
** System Status:
**	Normal operation may continue, but at a potential performance
**	penalty.
**
** Recommendation:
**	Check configuration.
*/
E_CL2C2E_CX_E_NONUMA:SS50000_MISC_ERRORS
"A CX NUMA operation was attempted without NUMA support being configured."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Coding logic error led to a function requiring NUMA
**	support to be called in a context where NUMA support
**	is either absent, or not configured.
**
** System Status:
**	Current operation will fail.
**
** Recommendation:
**	Contact technical support.
*/
E_CL2C2F_CX_E_BADRAD:SS50000_MISC_ERRORS
"Bad RAD value was supplied to a NUMA routine."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Tried to bind process to a RAD no longer in the
**	OSes set of RADs.  Probable config error.
**
** System Status:
**	Process not bound to expected RAD.  Possible performance
**	hit.
**
** Recommendation:
**	Check configuration.
*/
E_CL2C30_CX_E_OS_UNEX_FAIL:SS50000_MISC_ERRORS
"Unexpected failure in OS call."
/*%
** Parameters:
**	None.
**
** Explanation:
**	CX receive an unexpected error code from a call to a
**	RTL routine, or other vendor provided facility.
**
** System Status:
**	Current operation fails.
**
** Recommendation:
**	Contact technical support.
*/
E_CL2C31_CX_E_OS_EFAULT:SS50000_MISC_ERRORS
"Memory fault detected within OS call."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An illegal address was passed to an OS routine, or memory
**	is corrupted.
**
** System Status:
**	Uncertain.  Current operation will fail.
**
** Recommendation:
**	Contact technical support.
*/
E_CL2C32_CX_E_OS_BADPARAM:SS50000_MISC_ERRORS
"Bad parameter report from OS routine."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An out of range parameter was passed to a non-DLM OS
**	routine.  Indicates a program logic error.
**
** System Status:
**	Uncertain.  Current operation will fail.
**
** Recommendation:
**	Contact technical support.
*/
E_CL2C33_CX_E_OS_NOINIT:SS50000_MISC_ERRORS
"Cannot open a required 3rd party shared library."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Cannot locate a shared library providing the API for a 3rd party
**	or OS provided facility such as a DLM.  This may mean required
**	component is not installed, or that the shared library is not
**	in the LD_LIBRARY_PATH (name of this environment variable may
**	vary by platform).
**
** System Status:
**	CX facility is not enabled.  Cannot use Ingres clustering.
**	Startup will fail.
**
** Recommendation:
**	Check OS configuration, LD_LIBRARY_PATH, contact technical support.
*/
E_CL2C34_CX_E_OS_SUBSYSERR:SS50000_MISC_ERRORS
"Unexpected failure in non-DLM OS subsystem."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Some OS facility aside from the DLM on which CX depends
**	failed in an unexpected way.  For Tru64 using IMC, this
**	will indicate an IMC failure.
**
** System Status:
**	Installation will likely crash.
**
** Recommendation:
**	Contact technical support.
*/
E_CL2C35_CX_E_OS_MISC_ERR:SS50000_MISC_ERRORS
"Cannot resolve all symbols in a required 3rd party shared library."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Could not resolve the address of at least one required entry
**	points in a shared library providing the API for a 3rd party
**	or OS provided facility such as a DLM.   This likely means an
**	incompatible version of the 3rd Party/OS software is installed.
**
** System Status:
**	CX facility is not enabled.  Cannot use Ingres clustering.
**      Startup will fail.
**
** Recommendation:
**	Check OS configuration, LD_LIBRARY_PATH, contact technical support.
*/
E_CL2C36_CX_E_OS_MISC_ERR:SS50000_MISC_ERRORS
"Implementation dependent OS error."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Specific meaning for this will depend on platform.
**	For Tru64 this indicates we detected an update
**	consistency error using IMC. (unexpected).
**
** System Status:
**	Uncertain.  Current operation will fail.
**
** Recommendation:
**	Contact technical support.
*/
E_CL2C37_CX_E_OS_MISC_ERR:SS50000_MISC_ERRORS
"Implementation dependent OS error."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Specific meaning for this will depend on platform.
**
** System Status:
**	Uncertain.  Current operation will fail.
**
** Recommendation:
**	Contact technical support.
*/
E_CL2C38_CX_E_OS_MISC_ERR:SS50000_MISC_ERRORS
"Implementation dependent OS error."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Specific meaning for this will depend on platform.
**
** System Status:
**	Uncertain.  Current operation will fail.
**
** Recommendation:
**	Contact technical support.
*/
E_CL2C39_CX_E_OS_CONFIG_ERR:SS50000_MISC_ERRORS
"Implementation dependent OS error due to resource depletion."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Specific meaning for this will depend on platform.
**	On Linux it means OpenDLM is out of locks or resources.
**
** System Status:
**	Uncertain.  Current operation will fail.
**
** Recommendation:
**	On Linux, reconfigure OpenDLM, otherwise
**	contact technical support.
*/
E_CL2C3A_CX_E_OS_CONFIG_ERR:SS50000_MISC_ERRORS
"Implementation dependent OS error due to resource depletion."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Specific meaning for this will depend on platform.
**	On Linux it means OpenDLM is out of resource handles.
**
** System Status:
**	Uncertain.  Current operation will fail.
**
** Recommendation:
**	On Linux, reconfigure OpenDLM, otherwise
**	contact technical support.
*/
E_CL2C3B_CX_E_OS_CONFIG_ERR:SS50000_MISC_ERRORS
"Implementation dependent OS error due to resource depletion."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Specific meaning for this will depend on platform.
**	On Linux it means OpenDLM is out of ASTs.
**
** System Status:
**	Uncertain.  Current operation will fail.
**
** Recommendation:
**	On Linux, reconfigure OpenDLM, otherwise
**	contact technical support.
*/


E_CL2C40_CX_E_BAD_PARAMETER:SS50000_MISC_ERRORS
"Invalid parameter syntax (%0c) for '-node' or '-rad' specification."
/*%
** Parameters:
**	0c	- string which caused command line parse to fail.
**
** Explanation:
**	Command line contained multiple "-rad" or "-node" arguments
**	for a utility which only supports one, or argument to "-rad"
**	was not an integer, or argument to -node was missing.
**
** System Status:
**	Command will fail to start.
**
** Recommendation:
**	Probable user error.  Correct command line parameters and
**	try again.
*/
E_CL2C41_CX_E_MUST_SET_RAD:SS50000_MISC_ERRORS
"You must specify which virtual node is the target for %0c,\nusing either '-rad=<radid>', or '-node=<nodeid>'."
/*%
** Parameters:
**	0c	- Name of utility which failed to start.
**
** Explanation:
**	When running on a box configured with multiple virtual
**	nodes, target virtual node must be specified.  You
**	may establish an implicit context by setting UNIX
**	environment variable II_DEFAULT_RAD to a valid <radid>.
**
** System Status:
**	Command will fail to start.
**
** Recommendation:
**	Probable user error.  Correct command line parameters and
**	try again.
*/
E_CL2C42_CX_E_NOT_CONFIGURED:SS50000_MISC_ERRORS
"Node specified with %0c is not configured in this cluster."
/*%
** Parameters:
**	0c	- Command line argument which had the invalid value.
**
** Explanation:
**	Value for the command line argument was not a configured
**	RAD on the current host for '-rad', or was not a valid
**	node name or node alias (nickname) for '-node'.
**
** System Status:
**	Command will fail to start.
**
** Recommendation:
**	Probable user error.  Correct command line parameters and
**	try again.
*/
E_CL2C43_CX_E_REMOTE_NODE:SS50000_MISC_ERRORS
"%0c cannot operate on a node (%1c) not located on the current host."
/*%
** Parameters:
**	0c	- Command attempted.
**	1c	- Node which is remote.
**
** Explanation:
**	Not all Ingres utilities can operate on nodes not located
**	on their current host (a.k.a. machine, a.k.a box). 
**
** System Status:
**	Command will fail to start.
**
** Recommendation:
**	Probable user error.  Correct command line parameters and
**	try again.
*/
E_CL2C44_CX_E_NO_NUMA_CONTEXT:SS50000_MISC_ERRORS
"%0c was started as part of a NUMA cluster, and no RAD context was set."
/*%
** Parameters:
**	0c	- Command attempted.
**
** Explanation:
**	Similar to E_CL2C41, but issued by servers which expect the
**	RAD context to be encoded as part of the installation code.
**
** System Status:
**	Command will fail to start.
**
** Recommendation:
**	User or configuration error.  Check & correct configuration or
**	command line parameters and try again.
*/
E_CL2D01_NO_PRIVILEGE:SS50000_MISC_ERRORS
"Access Denied as you do not have sufficient privileges.\r\nYou must invoke this utility running in elevated mode.\r\n"
/*%
** Parameters:
**
** Explanation:
**	Vista specific error message.  It will display if application requires
**	elevated privileges to run and current user doesn't have those type
**	of privileges.
**
** System Status:
**	Application will fail to start.
**
** Recommendation:
**	Start Administrator command prompt and attempt to launch application
**	one more time.
*/

/*
** 11-Nov-92 (mrelac)
**	Following are the error message definitions for MPE/iX for the error
**	message facility 'ZZ'.  These error messages are used ONLY by MPE/iX
**	CL routines to issue special CL-specific error messages and trace
**	messages.  These messages are to be used ONLY through the ZZ-provided
**	interfaces -- however, they still must have message numbers distinct
**	from all other messages, or ERcomp will break.  I am using 7800 through
**	7974.  More error messages may be added later.
*/
E_CL7801_ZZ_TEST_MSG:SS50000_MISC_ERRORS
"This is a test message"
/*
** E_CL7802_ZZ_FUNC_CALLED
**
** This message is used by trace routines to provide a generic '%s' called
** message.
*/
E_CL7802_ZZ_FUNC_CALLED:SS50000_MISC_ERRORS
"%0c called."
/*
** E_CL7803_ZZ_INTR_FAIL - Generalized 'intrinsic failure' message
**
** This message is to be used for any general intrinsic failure. The
** message should be passed the caller's name, the intrinsic name, the
** mpe status code and the ccode.
*/
E_CL7803_ZZ_INTR_FAIL:SS50000_MISC_ERRORS
"%0c: MPE/iX Intrinsic %1c failed with status %2x and ccode %3d"
/*
** E_CL7804_ZZ_INTR_CCODE - Report bad 'ccode' from intrinsic
**
** This message is similar to E_ZZ_INTR_FAIL, but is to be used when
** only the ccode is available.
*/
E_CL7804_ZZ_INTR_CCODE:SS50000_MISC_ERRORS
"%0c: MPE/iX Intrinsic %1c failed with ccode %2d"
/*
** E_CL7805_ZZ_OUT_OF_RANGE - standard 'out of range' message
**
*/
E_CL7805_ZZ_OUT_OF_RANGE:SS50000_MISC_ERRORS
"%0c: Value of %1c is %2d, but should be in range %3d..%4d"
/*
** E_CL7806_ZZ_NOT_IMPLEMENTED
**
** This message is used to log the fact that an unimplemented CL
** function has been called.
*/
E_CL7806_ZZ_NOT_IMPLEMENTED:SS50000_MISC_ERRORS
"Warning!  CL Function %0c is not implemented under MPE/iX"
/*
** E_CL7807_ZZ_FUNC_FAILED
**
** This message can be used to log an unexpected failure of an internal
** function.
*/
E_CL7807_ZZ_FUNC_FAILED:SS50000_MISC_ERRORS
"%0c: Unexpected failure in %1c"
E_CL7820_ZZ_DI_ALLOC:SS50000_MISC_ERRORS
"DIalloc called on file ID %d for %d pages"
E_CL7821_ZZ_DI_CLOSE:SS50000_MISC_ERRORS
"DIclose called on file ID %d"
E_CL7822_ZZ_DI_CREATE:SS50000_MISC_ERRORS
"DIcreate called for location %z and file %z, pagesize of %d bytes"
E_CL7823_ZZ_DI_DELETE:SS50000_MISC_ERRORS
"DIdelete called for location %z and file %z"
E_CL7824_ZZ_DI_DIR_CREATE:SS50000_MISC_ERRORS
"DIdircreate called for location %z and directory %z"
E_CL7825_ZZ_DI_DIR_DELETE:SS50000_MISC_ERRORS
"DIdirdelete called for location %z and directory %z"
E_CL7826_ZZ_DI_FLUSH:SS50000_MISC_ERRORS
"DIflush called for file ID %d"
E_CL7827_ZZ_DI_FORCE:SS50000_MISC_ERRORS
"DIforce called for file ID %d"
E_CL7828_ZZ_DI_DIR_LIST:SS50000_MISC_ERRORS
"DIlistdir called for location %z"
E_CL7829_ZZ_DI_LIST:SS50000_MISC_ERRORS
"DIlistfiles called for location %z"
E_CL782A_ZZ_DI_OPEN:SS50000_MISC_ERRORS
"DIopen called for location %z and file %z, pagesize %d, mode %d, flags %d"
E_CL782B_ZZ_DI_RENAME:SS50000_MISC_ERRORS
"DIrename called for location %z and file %z, to be renamed %z"
E_CL782C_ZZ_DI_SENSE:SS50000_MISC_ERRORS
"DIsense called for file ID %d"
E_CL782D_ZZ_DI_READ:SS50000_MISC_ERRORS
"DIread called for file ID %d, %d pages beginning with page %d"
E_CL782E_ZZ_DI_WRITE:SS50000_MISC_ERRORS
"DIwrite called for file ID %d, %d pages beginning with page %d"
E_CL782F_ZZ_FILE_ALLOC:SS50000_MISC_ERRORS
"     Allocated %d pages beginning with page %d"
E_CL7830_ZZ_FILE_CREATE:SS50000_MISC_ERRORS
"     Created file %0c"
E_CL7831_ZZ_FILE_DELETE:SS50000_MISC_ERRORS
"     Deleted file %0c"
E_CL7832_ZZ_FILE_FLUSH:SS50000_MISC_ERRORS
"     Set EOF at page %d"
E_CL7833_ZZ_FILE_OPEN:SS50000_MISC_ERRORS
"     Opened file %0c as file ID %1d"
E_CL7834_ZZ_FILE_RENAME:SS50000_MISC_ERRORS
"     Changed logical file name for %0c"
E_CL7835_ZZ_FILE_SENSE:SS50000_MISC_ERRORS
"     EOF is at page %d"
E_CL7836_ZZ_DIR_CREATE:SS50000_MISC_ERRORS
"     Created Directory %0c"
E_CL7837_ZZ_DIR_DELETE:SS50000_MISC_ERRORS
"     Deleted Directory %0c"
E_CL7838_ZZ_DI_MEM_FLUSH:SS50000_MISC_ERRORS
"An error occurred while attempting to flush mapped file pages to disk."
E_CL7839_ZZ_DIX_BAD_23TREE:SS50000_MISC_ERRORS
"A structural error was detected in a IIDIXNAM 2-3 Tree."
E_CL783A_ZZ_RENAME_INFO:SS50000_MISC_ERRORS
"Could not update File Label info for %0c, changed from %1c to %2c"
E_CL783B_ZZ_NO_DIR_DELETE:SS50000_MISC_ERRORS
"DIdirdelete: %0c does not exist (it isn't there, so we are happy)"
E_CL783C_ZZ_HPFS_ERROR:SS50000_MISC_ERRORS
"OS File System Error: %0c %1c: (%2d) %3c"
E_CL783D_ZZ_HPER_FAIL:SS50000_MISC_ERRORS
"OS File System Error: %0c %1c: (%2d) --- HPERRMSG Failed! (0x%3d)"
E_CL783E_ZZ_NO_DIX_INFO:SS50000_MISC_ERRORS
"Could not access the logical filename translation information for location %0c"
E_CL783F_ZZ_DI_EV_INIT:SS50000_MISC_ERRORS
"Starting %d DI slaves"
E_CL7840_ZZ_DI_EV_IDLE:SS50000_MISC_ERRORS
"Selected slave %d: %d open files, %d events"
E_CL7841_ZZ_DI_EV_SELECT:SS50000_MISC_ERRORS
"Selected slave %d: %d events"
E_CL7842_ZZ_DI_EV_BALANCE:SS50000_MISC_ERRORS
"Load Balancing needed: Slave %d, Slave Events %d, Total Events: %d"
E_CL7843_ZZ_DI_EV_OPEN:SS50000_MISC_ERRORS
"     Opened File %0c as Unique ID %1d on Slave %2d, File ID %3d"
E_CL7844_ZZ_DI_EV_CLOSE:SS50000_MISC_ERRORS
"     Closing File on Slave %d, File ID %d"
/*
** E_CL7860_ZZ_GC_IS_SERVER
**
** Trace message issued by gcmksrvr.c when server tracing is enabled
*/
E_CL7860_ZZ_GC_IS_SERVER:SS50000_MISC_ERRORS
"GC_start_server: This IS the server!"
/*
** E_CL7861_ZZ_GC_TWO_BACKENDS
**
** Error message issued by gcmksrvr.c if it believes it has detected a
** attempt to spawn two child backends from the same frontend process. The
** second attempt is rejected.
*/
E_CL7861_ZZ_GC_TWO_BACKENDS:SS50000_MISC_ERRORS
"GCMKSRVR: Attempt to start two backends"
/*
** E_CL7862_ZZ_GC_NULL_HDLR
**
** This message is issued if a GC routine encounters a null function
** pointer when it expects to find a valid completion handler.
*/
E_CL7862_ZZ_GC_NULL_HDLR:SS50000_MISC_ERRORS
"GCACL: Null completion handler in %0c"
/*
** E_CL7863_ZZ_GC_BAD_PTNR
**
** This message is issued by GCrequest if it is provided a patently invalid
** partner name.
*/
E_CL7863_ZZ_GC_BAD_PTNR:SS50000_MISC_ERRORS
"GCrequest: Bad partner name '%0c' -- Reason: %1c"
/*
** E_CL7864_ZZ_GC_REUSE_LCB
**
** This trace message is issued if GCrequest finds an existing LCB to reuse
*/
E_CL7864_ZZ_GC_REUSE_LCB:SS50000_MISC_ERRORS
"GCrequest: Using already-allocated LCB"
/*
** E_CL7865_ZZ_GC_NEW_LCB
**
** This trace message is issued when a new LCB is allocated
*/
E_CL7865_ZZ_GC_NEW_LCB:SS50000_MISC_ERRORS
"%0c: Allocating new LCB:"
/*
** E_CL7866_ZZ_GC_NO_MEM
**
** This message is issued if GC receives the NULL pointer back from a call
** to the mainline-provided memory allocation routines.
*/
E_CL7866_ZZ_GC_NO_MEM:SS50000_MISC_ERRORS
"%0c: No memory available for GC (NULL pointer from (*GCalloc))"
/*
** E_CL7867_ZZ_GC_SIZE
**
** This trace message describes the buffer size which GC has recommended
** to the mainline code.
*/
E_CL7867_ZZ_GC_SIZE:SS50000_MISC_ERRORS
"GCACL: Size-advise set to %d"
/*
** E_CL7868_ZZ_GC_BAD_PORT
**
** THis message is issued if GCrequest detects a failure during the initial
** handshaking protocol.
*/
E_CL7868_ZZ_GC_BAD_PORT:SS50000_MISC_ERRORS
"GCACL: %0c got port %1d instead of %2d"
/*
** E_CL7869_ZZ_GC_NO_REG
**
** This message is issued if the server mistakenly calls GClisten before
** calling GCregister.
*/
E_CL7869_ZZ_GC_NO_REG:SS50000_MISC_ERRORS
"GCACL: GClisten without previous GCregister"
/*
** E_CL786A_ZZ_GC_ASYNC_IGNORED
**
** This trace message is issued to remind the reader that GC ignores most
** asynchronous requests, treating them as synchronous.
*/
E_CL786A_ZZ_GC_ASYNC_IGNORED:SS50000_MISC_ERRORS
"GCACL: Asynchronous request treated as synchronous by %0c"
/*
** E_CL786B_ZZ_GC_NO_PASSWORD
**
** This trace message is issued to remind the reader that GC does not
** perform any password checking.
*/
E_CL786B_ZZ_GC_NO_PASSWORD:SS50000_MISC_ERRORS
"GCACL: Password passing and checking is not supported"
/*
** E_CL786C_ZZ_GC_PORT_NUM
**
** THis trace message displays a port number being used.
*/
E_CL786C_ZZ_GC_PORT_NUM:SS50000_MISC_ERRORS
"%0c: %1c port number is %2d"
/*
** E_CL786D_ZZ_GC_DISC
**
** This trace message is issued if GCsend or GCreceive is called for an
** association which has already been disconnected.
*/
E_CL786D_ZZ_GC_DISC:SS50000_MISC_ERRORS
"GCACL: %0c found disconnected association"
/*
** E_CL786E_ZZ_GC_PURGING
**
** This trace message is issued when GCsend invokes GCpurge on the caller's
** behalf.
*/
E_CL786E_ZZ_GC_PURGING:SS50000_MISC_ERRORS
"GCACL: GCsend invoking GCpurge"
/*
** E_CL786F_ZZ_GC_ONLY_ONE
**
** This message is issued if a GC client attempts to hang TWO asynchronous
** receives at the same time. The current version of GC only supports one
** asynchronous receive pending.
*/
E_CL786F_ZZ_GC_ONLY_ONE:SS50000_MISC_ERRORS
"GCACL: GCreceive supports only 1 async expedited request"
/*
** E_CL7870_ZZ_GC_PTNR_GONE
**
** This trace message describes the reason why GCconnchk has decided that
** a partner is no longer present.
*/
E_CL7870_ZZ_GC_PTNR_GONE:SS50000_MISC_ERRORS
"GCACL: GCconnchk found partner was gone because %0c"
/*
** E_CL7871_ZZ_GC_NO_NS_FILE
**
** This message is issued by GCnsid if it has unexpected trouble reading
** the name server file.
*/
E_CL7871_ZZ_GC_NO_NS_FILE:SS50000_MISC_ERRORS
"GCACL: GCnsid is unable to access the Name Server file"
/*
** E_CL7872_ZZ_GC_NO_HOSTNAME
**
** This message is logged if GChostname is unable to look up the hostname
** in the II_GC_HOSTNAME session variable.
*/
E_CL7872_ZZ_GC_NO_HOSTNAME:SS50000_MISC_ERRORS
"GCACL: GChostname: No hostname defined"
/*
** E_CL7873_ZZ_GC_BAD_ID
**
** This message is logged if GCdetect is passed a bad listen id.
*/
E_CL7873_ZZ_GC_BAD_ID:SS50000_MISC_ERRORS
"GCACL: GCdetect passed bad listen id '%0c'"
/*
** E_CL7874_ZZ_GC_EXP_SEND
**
** This trace message is issued when a send is issued on an expedited channel.
*/
E_CL7874_ZZ_GC_EXP_SEND:SS50000_MISC_ERRORS
"GCACL: Issuing an EXPEDITED send!"
/*
** E_CL7875_ZZ_GC_EOF_RCVD
**
** This trace messsage is issued when a GCpurge-type EOF message is received.
*/
E_CL7875_ZZ_GC_EOF_RCVD:SS50000_MISC_ERRORS
"GCACL: EOF message received"
/*
** E_CL7876_ZZ_GC_NOWAIT
**
** This trace message is issued when a no-wait receive is issued.
*/
E_CL7876_ZZ_GC_NOWAIT:SS50000_MISC_ERRORS
"GCACL: Issuing a no-wait receive"
/*
** E_CL7877_ZZ_GC_TIMEOUT
**
** This trace message displays the timeout used for a receive
*/
E_CL7877_ZZ_GC_TIMEOUT:SS50000_MISC_ERRORS
"GCACL: Timeout is %d seconds"
/*
** E_CL7878_ZZ_GC_BAD_IPARMS
**
** This error message is issued if the expedited flow receive handler
** is called with bad parameters.
*/
E_CL7878_ZZ_GC_BAD_IPARMS:SS50000_MISC_ERRORS
"GCACL: Interrupt handler received bad parms"
/*
** E_CL7879_ZZ_GC_WRONG_PORT
**
** This error message is issued if the expedited flow receive handler
** gets the wrong port number in the message.
*/
E_CL7879_ZZ_GC_WRONG_PORT:SS50000_MISC_ERRORS
"GCACL: Port %d is not the expedited port (%d)"
/*
** E_CL787A_ZZ_GC_NOT_ARMED
**
** This errormessage is issued if the expedited flow receive handler
** finds itself being called when interrupts were not (thought to be) armed.
*/
E_CL787A_ZZ_GC_NOT_ARMED:SS50000_MISC_ERRORS
"GCACL: Internal error -- interrupts not armed"
/*
** E_CL787B_ZZ_GC_EXP_COMP
**
** This trace message is issued when the expedited flow receive handler
** calls the mainline completion routine/
*/
E_CL787B_ZZ_GC_EXP_COMP:SS50000_MISC_ERRORS
"GCPORTS: Calling expedited flow completion routine"
/*
** E_CL787C_ZZ_GC_TRY_AGAIN
**
** This trace message is issued when the internal timeout code re-issued
** a receive.
*/
E_CL787C_ZZ_GC_TRY_AGAIN:SS50000_MISC_ERRORS
"GC_receive_it: willing to try again."
/*
** E_CL787D_ZZ_GC_ABORT_RECV
**
** This message is issued when the internal timeout code aborts a receive.
*/
E_CL787D_ZZ_GC_ABORT_RECV:SS50000_MISC_ERRORS
"GC_receive_it: aborting this receive"
/*
** E_CL787E_GC_WOULD_BLOCK
**
** This message is issued when an asynchronous I/O would have to block.
** It is not allowed to block, so this return code is returned instead.
** Actually, this message is never intended to be printed out.  It is just
** used as a flag within the GCA CL.
*/
E_CL787E_GC_WOULD_BLOCK:SS50000_MISC_ERRORS
"An asynchronous I/O request would have blocked"
/*
** E_CL787F_GC_CALLBACK_NOT_READY
**
** This message indicates that a non-I/O callback routine is not yet ready
** to be called.  When it IS ready, the ready_for_callback flag in the lcb
** will be set, the callback will be called and the ior will be freed.  Until
** then, we must not process this kind of ior.
** Actually, this message is never intended to be printed out.  It is just
** used as a flag within the GCA CL.
*/
E_CL787F_GC_CALLBACK_NOT_READY:SS50000_MISC_ERRORS
"The specified callback routine is not ready to be called yet"
/*
** E_CL7890_ZZ_BAD_LG_ID - Internal error detected in an LG_ID
**
** This message is used by the LGintern.c routines to complain about
** an LG_ID which does not have the correct internal structure. The
** LG routine will then return BAD_PARAM to the caller.
**
** This generally indicates a SEVERE internal error within the DBMS, to
** treat any occurrences of this message with respect!
*/
E_CL7890_ZZ_BAD_LG_ID:SS50000_MISC_ERRORS
"%0c: LG_ID %1d is out of range (range=[1..%2d])"
/*
** E_CL7891_ZZ_BAD_LPB - Internal error detected in an LPB block
**
** This message is used by the LGINTERN.C routines to complain about an
** LPB block passed in which does not have the correct internal
** structure. The LG routine will then return BADPARAM to the caller.
**
** This usually indicates memory corruption or other severe internal
** error.
*/
E_CL7891_ZZ_BAD_LPB:SS50000_MISC_ERRORS
"%0c: Bad LPB -- Type=%1d, Instances=(%2d, %3d)"
/*
** E_CL7892_ZZ_BAD_LPD - Internal error detected in an LPD block
**
** THis message is used by the LGINTERN.C routines to complain about
** an LPD block passed in which does not have the correct internal
** structure. The LG routine will then return BADPARAM to the caller.
**
** This usually indicates memory corruption or other sever internal
** error.
*/
E_CL7892_ZZ_BAD_LPD:SS50000_MISC_ERRORS
"%0c: Bad LPD -- Type=%1d, Instances=(%2d,%3d), Pids=(%4d,%5d)"
/*
** E_CL7893_ZZ_BAD_LXB - Internal error detected in an LXB block
**
** This message is used by the LGINTERN.C routines to complain about
** an LXB block passed in which does not have the correct internal
** structure. The LG routine will then return BADPARAM to its caller.
**
** This usually indicates memory corruption or other severe internal
** error.
*/
E_CL7893_ZZ_BAD_LXB:SS50000_MISC_ERRORS
"%0c: Bad LXB -- Type=%1d, Instances=(%2d,%3d), Status=%4x"
/*
** E_CL7894_ZZ_BAD_LDB
**
** Yet another 'data structure is corrupt' message out of LGINTERN.C
*/
E_CL7894_ZZ_BAD_LDB:SS50000_MISC_ERRORS
"%0c: Bad LDB -- Type=%1d, Instances=(%2d,%3d)"
/*
** E_CL7895_ZZ_BAD_HDR_SIZE - Log File page size is invalid
**
** The log file page size must be one of 4K, 8K, 16K, or 32K.
*/
E_CL7895_ZZ_BAD_HDR_SIZE:SS50000_MISC_ERRORS
"%0c: Header size (%1d) is wrong. It must be one of 4K, 8K, 16K, or 32K"
/*
** E_CL7896_ZZ_BCNT_ALREADY
**
** The number of buffers parameter is already set to a non-zero value.
** It cannot be changed except by bring the logging system down and then
** back up.
*/
E_CL7896_ZZ_BCNT_ALREADY:SS50000_MISC_ERRORS
"%0c: LG_A_BCNT rejected because buffer count is already set"
/*
** E_CL7897_ZZ_BAD_PARM_SIZE - General LG_alter/LG_show bad parm size
**
** This message is issued when LG_alter or LG_show detects that a 'size'
** variable passed in is not of the correct size.
**
*/
E_CL7897_ZZ_BAD_PARM_SIZE:SS50000_MISC_ERRORS
"%0c: Size of %1c was %2d, but it should have been %3d"
/*
** E_CL7898_ZZ_NEW_HDR_SIZE
**
** This trace message announces the changing of the log's page size.
*/
E_CL7898_ZZ_NEW_HDR_SIZE:SS50000_MISC_ERRORS
"%0c: Header altered -- new log page size is %1d"
/*
** E_CL7899_ZZ_DB_OPENED - trace the completion of database opening
**
** THis message is issued by LG_alter when a database has been completely
** opened. It is a trace message.
*/
E_CL7899_ZZ_DB_OPENED:SS50000_MISC_ERRORS
"Database open pending flags have been turned off"
/*
** E_CL789A_ZZ_DESTROY_FAILED
**
** THis message is issued when there is an error in LGK_destroy when the
** log is being brought offline. Typically, such an error would indicate
** a filesystem problem closing the shared memory file.
*/
E_CL789A_ZZ_DESTROY_FAILED:SS50000_MISC_ERRORS
"LGK_destroy failed, status=%x"
/*
** E_CL789B_ZZ_NO_LG_MEM
**
** This message is issued when an LG routine is unable to acquire more shared
** memory in order to expand one of the LG lists. There may be a bug causing
** routines to fail to release shared memory when they should, or the system
** may simply need more shared memory.
*/
E_CL789B_ZZ_NO_LG_MEM:SS50000_MISC_ERRORS
"%0c: Unable to expand %1c list"
/*
** E_CL789C_ZZ_ZERO_TRAN_ID
**
** The Logging system (and the mainline) code depends heavily on being able
** to detect internal consistency in transaction id's, which it does by
** requiring that the lgt_high portion of the tran id always be non-zero.
** If the lgt_high field ever goes to zero, the world is about to end, so
** LG_begin audits this error message under that condition.
*/
E_CL789C_ZZ_ZERO_TRAN_ID:SS50000_MISC_ERRORS
"LG_begin: main transaction id has become 0!"
/*
** E_CL789D_ZZ_SUSPENDING
**
** This message is issued when a process calls LG_event to suspend itself
*/
E_CL789D_ZZ_SUSPENDING:SS50000_MISC_ERRORS
"LG_event: suspending on event mask %x"
/*
** E_CL789E_ZZ_LOG_SHUTDOWN
**
** This message is issued when an LG call is rejected because the Log system
** is shutting itself down.
*/
E_CL789E_ZZ_LOG_SHUTDOWN:SS50000_MISC_ERRORS
"%0c: Logging system is shutting down - call rejected"
/*
** E_CL789F_ZZ_MULTIPLE_MASTERS
**
** This message is issued when there is already a Log Master and another
** process calls LGopen, attempting to establish itself as the Log Master.
*/
E_CL789F_ZZ_MULTIPLE_MASTERS:SS50000_MISC_ERRORS
"LG_open: Too many Log Masters"
/*
** E_CL78A0_ZZ_NO_FNAME
**
** This message is issued when a process other than the Log Master omits
** the lg file name. Only the master is allowed to omit the name.
*/
E_CL78A0_ZZ_NO_FNAME:SS50000_MISC_ERRORS
"LG_open: Filename is missing"
/*
** E_CL78A1_ZZ_MISSING_MASTER
**
** This message is issued when a non-master process attempts to open the
** Logging System before the Master has opened it.
*/
E_CL78A1_ZZ_MISSING_MASTER:SS50000_MISC_ERRORS
"LG_open: No Log Master yet"
/*
** E_CL78A2_ZZ_BAD_FCT
**
** Only a slave process can request a Fast Commit Thread. This message is
** issued if the Log Master or the Archiver attempts to make such a request.
*/
E_CL78A2_ZZ_BAD_FCT:SS50000_MISC_ERRORS
"LG_open: Only a slave may request a Fast Commit Thread"
/*
** E_CL78A3_ZZ_MULTIPLE_ARCHIVERS
**
** There can be at most one archiver process.
*/
E_CL78A3_ZZ_MULTIPLE_ARCHIVERS:SS50000_MISC_ERRORS
"LG_open: Too many Archivers"
/*
** E_CL78A4_ZZ_LOG_IS_OFFLINE
**
** This message is issued if a slave process attempts to open the Logging
** System before the Master has placed it online.
*/
E_CL78A4_ZZ_LOG_IS_OFFLINE:SS50000_MISC_ERRORS
"LG_open: Log is not online yet"
/*
** E_CL78A5_ZZ_BAD_FNAME
**
** This message is issued if the slave gives a bad Log File Name
*/
E_CL78A5_ZZ_BAD_FNAME:SS50000_MISC_ERRORS
"LG_open: Wrong filename provided by slave process"
/*
** E_CL78A6_ZZ_WRITE_LENGTH
**
** This is a trace message indicating the size of an LGwrite message
*/
E_CL78A6_ZZ_WRITE_LENGTH:SS50000_MISC_ERRORS
"Writing a record of length %d bytes"
/*
** E_CL78A7_ZZ_WRITE_TO_READONLY
**
** This message is issued when the Logging System detects an attempt to
** write to a readonly transaction.
*/
E_CL78A7_ZZ_WRITE_TO_READONLY:SS50000_MISC_ERRORS
"LG_write: Attempt to write to READONLY transaction"
/*
** E_CL78A8_ZZ_WRITE_STALL
**
** This message is issued when LGwrite determines that it must stall the
** current transaction for some reason (the reason is indicated in the
** message).
*/
E_CL78A8_ZZ_WRITE_STALL:SS50000_MISC_ERRORS
"LG_write: Stalling because %0c"
/*
** E_CL78A9_ZZ_MISSING_BT
**
** This message is issued when the Logging System detects that the first
** record written for this transaction is NOT a Begin Transaction record
** and the process doing the writing is not the Log Master.
*/
E_CL78A9_ZZ_MISSING_BT:SS50000_MISC_ERRORS
"LG_write: FirstFlag is off and you are not the master!"
/*
** E_CL78AA_ZZ_WRITE_HDR
**
** THis trace message is issued when a log record header is built into the
** current buffer
*/
E_CL78AA_ZZ_WRITE_HDR:SS50000_MISC_ERRORS
"********>>>>> Building Log Record header into buffer"
/*
** E_CL78AB_ZZ_FORCE_FLAG
**
** This trace message is issued when LG_write is about to force the current
** buffer to disk.
*/
E_CL78AB_ZZ_FORCE_FLAG:SS50000_MISC_ERRORS
"LG_write: force flag is on"
/*
** E_CL78AC_ZZ_SETTING_TIMER
**
** THis trace message is issued when LG_write initiates the timer on a buffer
** in order to cause the buffer to be forced to disk when the timer expires.
*/
E_CL78AC_ZZ_SETTING_TIMER:SS50000_MISC_ERRORS
"LG_write: setting timerLBB"
/*
** E_CL78AD_ZZ_LOGFULL
**
** This message is issued when the Logging System detects that the log has
** filled up. Everyone will need to be stalled until the archiver is able
** to free up some space.
*/
E_CL78AD_ZZ_LOGFULL:SS50000_MISC_ERRORS
"LG_write: Used=%d, limit=%d, Signalling LOGFULL"
/*
** E_CL78AE_ZZ_SIGNAL_EVENT
**
** This message is used by the LGINTERN Signal_Event routine to indicate
** that an event has been signaled and whether or not this is the 'resume'
** occurrence of this event (the 0->1 change in the event's status).
*/
E_CL78AE_ZZ_SIGNAL_EVENT:SS50000_MISC_ERRORS
"Signal_Event: Event=%0c, curStatus=%1c, Event had %2c already occurred"
/*
** E_CL78AF_ZZ_ALREADY_SUSP
**
** This trace message is issued if the suspend routine detects an internal
** suspend bug.
*/
E_CL78AF_ZZ_ALREADY_SUSP:SS50000_MISC_ERRORS
"Already suspended, nothing to do"
/*
** E_CL78B0_ZZ_WRITE_BLOCK
**
** This trace message is issued by write_block when a block is being written.
*/
E_CL78B0_ZZ_WRITE_BLOCK:SS50000_MISC_ERRORS
"write_block: LBB is at %x, buffer=%x (lga=%x.%x)"
/*
** E_CL78B1_ZZ_WAITQ_NUM
**
** This message is issued by write_block and write_complete to trace the
** wait queues and the number of waiters.
*/
E_CL78B1_ZZ_WAITQ_NUM:SS50000_MISC_ERRORS
"%0c: WaitQ is at %1x and there are %2d waiters"
/*
** E_CL78B2_ZZ_WROTE_N_BYTES
** 
** This trace message is issued by the physical I/O routine after it writes
** a new log block.
*/
E_CL78B2_ZZ_WROTE_N_BYTES:SS50000_MISC_ERRORS
"%d bytes written from %x to %x.%x"
/*
** E_CL78B3_ZZ_ADDING_TICKS
**
** This trace message is issued by the log interval handler when the timed
** buffer ages a little more.
*/
E_CL78B3_ZZ_ADDING_TICKS:SS50000_MISC_ERRORS
"LGINTERN: Adding 3 more ticks"
/*
** E_CL78B4_ZZ_CLEARING_TIMER
**
** This trace message is issued by the log interval routines under
** Conditions I'm not certain of.
*/
E_CL78B4_ZZ_CLEARING_TIMER:SS50000_MISC_ERRORS
"LGINTERN: Resetting timer LBB. Timer LBB was %0c current"
/*
** E_CL78B5_ZZ_BAD_QAST
**
** This message is issued if LG is unable to queue an AST. BAd things will
** soon happen.
*/
E_CL78B5_ZZ_BAD_QAST:SS50000_MISC_ERRORS
"LGINTERN: Bad queue of AST"
/*
** E_CL78B6_ZZ_PROC_HAS_DIED
**
** This message is issued when the RCP detects that a server has abnormally
** died. The RCP will not proceed to initiate recovery on the server's
** behalf. An occurrence of this message is not necessarily an error.
*/
E_CL78B6_ZZ_PROC_HAS_DIED:SS50000_MISC_ERRORS
"LGINTERN: Process %d has died, initiating automatic recovery"
/*
** E_CL78B7_ZZ_PID_IS_WRONG
**
** This message is issued if the RCP detects an invalid Process ID while
** checking for dead servers. This indicates a SERIOUS problem in the Logging
** System.
*/
E_CL78B7_ZZ_PID_IS_WRONG:SS50000_MISC_ERRORS
"LGINTERN: We think PID is %d, but it should be %d"
/*
** E_CL78B8_ZZ_LGD_BAD_TABLE
**
** This message is issued if the Logging System detects that one of
** its shared memory tables has become corrupt. The system does not have
** long to live at this point...
*/
E_CL78B8_ZZ_LGD_BAD_TABLE:SS50000_MISC_ERRORS
"%0c: Null block pointer found, %1c table is corrupt!"
/*
** E_CL78B9_ZZ_LGD_IS_NULL
**
** THis message is issued by the Logging System if it can't find its shared
** memory. This is generally because of incorrect client code -- for example,
** calling LGalter before calling LGinitialize.
*/
E_CL78B9_ZZ_LGD_IS_NULL:SS50000_MISC_ERRORS
"%0c: Null LGD pointer found!"
/*
** E_CL78BA_ZZ_LG_BAD_READ
**
** This message is issued by the Logging System when a read request of the
** log results in an invalid log block. The message provides details about
** where in the log the bad block occurred and what was wrong with the
** block.
*/
E_CL78BA_ZZ_LG_BAD_READ:SS50000_MISC_ERRORS
"%0c: Rec at LGA %1d.%2d, offset %3d, had lengths %4d and %5d, but expected %6d"
/*
** E_CL78BB_ZZ_LG_READ_LENGTH
**
** This message is issued by the Logging System when a read request of
** the log encounters a record length which is patently invalid. The message
** provides the length which we encountered, as well as the minimum and
** maximum lengths. The length, even if in range, may be patently invalid
** if it is not a multiple of 4
*/
E_CL78BB_ZZ_LG_READ_LENGTH:SS50000_MISC_ERRORS
"%0c: Rec at LGA %1d.%2d, offset %3d, had length %4d, but expected %5d-%6d"
/*
** E_CL78D0_ZZ_BAD_LK_ID - Internal error detected in an LK_ID
**
** This message is used by the LKINTERN.C routines to complain about
** an LK_ID which does not have the correct internal structure. The
** LK routine will then generally return BADPARAM to its caller.
**
** This generally indicates a SEVERE internal error within the code.
*/
E_CL78D0_ZZ_BAD_LK_ID:SS50000_MISC_ERRORS
"%0c: LK_ID %d is out of range (range = [1..%1d])"
/*
** E_CL78D1_ZZ_LKD_BAD_TABLE - internal error in one of the LKD tables
**
** This message is issued when a NULL block pointer is fetched for an
** otherwise valid LK_ID. This either indicates a totally bad LK_ID was
** input, or someone has corrupted the locking system's shared memory.
** Either way, you're in trouble.
*/
E_CL78D1_ZZ_LKD_BAD_TABLE:SS50000_MISC_ERRORS
"%0c: Null pointer found for valid LK_ID in %1c table"
/*
** E_CL78D2_ZZ_BAD_LLB
**
** This message is issued when an LLB pointer is fetched from the LKD's
** large block table, but the contents of the pointed-to LLB do not appear
** valid. Details about the LLB are described in the message.
*/
E_CL78D2_ZZ_BAD_LLB:SS50000_MISC_ERRORS
"%0c: Bad LLB -- Type=%1d, Instances=(%2d, %3d)"
/*
** E_CL78D3_ZZ_LKD_IS_NULL
**
** This message is issued when one of the internal LK routines is unable
** to locate the shared memory system used for LK.
*/
E_CL78D3_ZZ_LKD_IS_NULL:SS50000_MISC_ERRORS
"%0c: LKD is NULL!"
/*
** E_CL78D4_ZZ_BAD_LKB
**
** This message is issued when an LKB pointer is fetched from the LKD's
** small block table, but the contents of the pointed-to LKB do not appear
** valid. Details about the LKB are described in the message.
*/
E_CL78D4_ZZ_BAD_LKB:SS50000_MISC_ERRORS
"%0c: Bad LKB -- Type=%1d, Instances=(%2d,%3d) LockLists=(%4x,%5x)"
/*
** E_CL78D5_ZZ_BAD_RELATED_LLB
**
** This message is issued when an attempt is made to create a
** related-lock-list chain longer than 1. That is, a new lock list may be
** related to an existing lock list, but that existing lock list may NOT
** in turn be related to yet another lock list. If the Locking System
** detects this situation, it complains by issuing this error message.
*/
E_CL78D5_ZZ_BAD_RELATED_LLB:SS50000_MISC_ERRORS
"%0c: Related list at %1x is in turn related to list at %2x"
/*
** E_CL78D6_ZZ_NULL_LOCK_LIST
**
** This message is issued by LK_create_list if it is passed a NUULL
** list list pointer. LKcreate_list requires a non-NULL pointer.
*/
E_CL78D6_ZZ_NULL_LOCK_LIST:SS50000_MISC_ERRORS
"LK_create_list: lock list id is NULL"
/*
** E_CL78D7_ZZ_NO_LK_MEM
**
** This message is issued when an LK routine is unable to acquire more
** shared memory in order to expand one of the LK lists. There may be a bug
** causing routines to fail to release shared memory when they should, or
** the system may simply need more shared memory.
*/
E_CL78D7_ZZ_NO_LK_MEM:SS50000_MISC_ERRORS
"%0c: unable to expand %1c list"
/*
** E_CL78D8_ZZ_UNIQUE_HIGH_NULL
**
** This message is issued if LKcreate_list is passed a unique ID with a 0
** value in the high word. LKcreate_list requires that unique ID values
** contain a non-0 value in the high word.
*/
E_CL78D8_ZZ_UNIQUE_HIGH_NULL:SS50000_MISC_ERRORS
"LK_create_list: unique id has NULL highword"
/*
** E_CL78D9_ZZ_UNIQUE_ID_NULL
**
** This message is issued if LKcreate_list is called without the LK_ASSIGN
** flag and without a unique ID provided. One or the other of these is
** required.
*/
E_CL78D9_UNIQUE_ID_NULL:SS50000_MISC_ERRORS
"LK_create_list: Unique ID pointer is NULL"
/*
** E_CL78DA_ZZ_LK_LIST_EXISTS
**
** This message is given if a lock list by the given name already exists
** and the caller did not give the special LK_RECOVER flag to assume
** ownership of the existing lock list.
**
** Two lists by the same name are not allowed.
*/
E_CL78DA_ZZ_LK_LIST_EXISTS:SS50000_MISC_ERRORS
"LK_create_list: Lock List %x.%x already exists"
/*
** E_CL78DB_ZZ_NO_ID_NO_KEY
**
** This message is issued if a caller of LKrelease asks to release a
** specific lock but provides neither the lock id nor the lock key. Without
** at least one of these values the lock cannot be released, so the
** LKrelease call is rejected.
*/
E_CL78DB_ZZ_NO_ID_NO_KEY:SS50000_MISC_ERRORS
"LKrelease: Neither a lockid nor a lock key was provided"
/*
** E_CL78DC_ZZ_NOT_PHYSICAL
**
** This message is issued if the specific lock to be released does not
** have the LK_PHYSICAL attribute.
*/
E_CL78DC_ZZ_NOT_PHYSICAL:SS50000_MISC_ERRORS
"LK_release: Lock does not have physical attribute. Attributes=%x"
/*
** E_CL78DD_ZZ_NO_SUCH_LOCK
**
** This message is issued if LKrelease is asked to release a specific
** lock but the lock does not exist:
*/
E_CL78DD_ZZ_NO_SUCH_LOCK:SS50000_MISC_ERRORS
"LKrelease: LKB is NULL"
/*(
** E_CL78DE_ZZ_LOCK_RELEASED
**
** This trace message is issued by LKrelease when it successfully releases
** a lock.
*/
E_CL78DE_ZZ_LOCK_RELEASED:SS50000_MISC_ERRORS
"Lock released (return number %d)"
/*
** E_CL78DF_ZZ_BAD_PARTIAL
**
** This message is issued if LKrelease is called for a partial lock release
** but the lock_key provided is NULL. Partical lock releases require that the
** lock key be provided.
*/
E_CL78DF_ZZ_BAD_PARTIAL:SS50000_MISC_ERRORS
"LKrelease: Partial release requires non-NULL lock key"
/*
** E_CL78E0_ZZ_BAD_LOCK_MODE
**
** This message is issued if LKreuqest is called with a bad lock mode.
** The lock mode which was provided is echoed in the message.
*/
E_CL78E0_ZZ_BAD_LOCK_MODE:SS50000_MISC_ERRORS
"LKrequest: bad lock mode: %x"
/*
** E_CL78E1_ZZ_INVALID_CONVERT
**
** This message is issued if LKrequest is called to convert an existing
** lock, but the lock id of the existing lock is not provided.
*/
E_CL78E1_ZZ_INVALID_CONVERT:SS50000_MISC_ERRORS
"LKrequest: Lock ID is required for lock conversion request"
/*
** E_CL78E2_ZZ_LOCK_SUSPENDING
**
** This trace message is issued when LKrequest must suspend the current
** lock list until some lock becomes free
*/
E_CL78E2_ZZ_LOCK_SUSPENDING:SS50000_MISC_ERRORS
"LKrequest: suspending list because %0c"
/*
** E_CL78E3_ZZ_LOCK_REQUEST_DONE
**
** This trace message is issued when a lock request is completed.
*/
E_CL78E3_ZZ_LOCK_REQUEST_DONE:SS50000_MISC_ERRORS
"LKrequest complete (return number %d, status=%x)"
/*
** E_CL78E4_ZZ_MISSING_LOCK_KEY
**
** This message is issued when a LKrequest call cannot complete because
** the caller did not provide a lock key
*/
E_CL78E4_ZZ_MISSING_LOCK_KEY:SS50000_MISC_ERRORS
"LKrequest: Lock Key was omitted"
/*
** E_CL78E5_ZZ_BAD_KEY_TYPE
**
** This message is issued when a LKrequest call rejects a lock key because
** it has a bad lock key type.
*/
E_CL78E5_ZZ_BAD_KEY_TYPE:SS50000_MISC_ERRORS
"LKrequest: Lock key has type 0, which is not allowed"
/*
** E_CL78E6_ZZ_TOO_MANY_LOCKS
**
** THis message is issued by LKrequest when it rejects a lock request due
** to too many locks. This is not always an error, but it does indicate
** that there is a resource contention problem in the DBMS.
*/
E_CL78E6_ZZ_TOO_MANY_LOCKS:SS50000_MISC_ERRORS
"LKrequest: List owns %d locks. Max is %d. Flags were %x. List Status=%x"
/*
** E_CL78E7_ZZ_NO_LK_TABLES
**
** This message is issued by LKrequest when it rejects a LKrequest call
** because the resource and lock hash tables have not been allocated.
*/
E_CL78E7_ZZ_NO_LK_TABLES:SS50000_MISC_ERRORS
"LKrequest: Missing RSH (%x) or LKH (%x) tables"
/*
** E_CL78E8_ZZ_LOCK_UNAVAIL
**
** THis message is issued when the lock can't be granted immediately, and
** the nowait flag was specified. This may not be an error...
*/
E_CL78E8_ZZ_LOCK_UNAVAIL:SS50000_MISC_ERRORS
"LKrequest: Lock unavailable, NOWAIT specified"
/*
** E_CL78E9_ZZ_LOCK_RETRY
**
** This trace message is issued by LKrequest when it tells the RCP about
** orphaned locks.
*/
E_CL78E9_ZZ_LOCK_RETRY:SS50000_MISC_ERRORS
"LKrequest: Lock request needs retry"
/*
** E_CL78EA_ZZ_LOCK_MODE_CLASH
**
** This message is issued by LKrequest when a lock list must be suspended
** because a desired lock is held by another list in a conflicting mode.
** This is a trace message.
*/
E_CL78EA_ZZ_LOCK_MODE_CLASH:SS50000_MISC_ERRORS
"LKrequest: Lock could not be granted yet. Requested: %x, held: %x"
/*
** E_CL78EB_ZZ_LOCK_GRANTED
**
** This trace message is issued when the Locking System grands a lock.
*/
E_CL78EB_ZZ_LOCK_GRANTED:SS50000_MISC_ERRORS
"LKrequest: New Lock granted"
/*
** E_CL78EC_ZZ_LOCK_CANCEL
**
** This message is issued when the Locking System cancels a lock request.
** It is a trace message.
*/
E_CL78EC_ZZ_LOCK_CANCEL:SS50000_MISC_ERRORS
"LK(cancel): Cancelling lock list at %x"
/*
** E_CL78ED_ZZ_CANCEL_DONE
**
** THis message is issued when a lock cancel operation completes, and
** indicates that the lock list in question is no longer waiting.
*/
E_CL78ED_ZZ_CANCEL_DONE:SS50000_MISC_ERRORS
"LK(cancel): Lock list at %x is no longer waiting"
/*
** E_CL78EE_ZZ_LK_RLS_WAIT
**
** This message is issued when an attempt is made to release a waiting
** lock list. This is illegal.
*/
E_CL78EE_ZZ_LK_RLS_WAIT:SS50000_MISC_ERRORS
"LKrelease: attempt to release a waiting lock list"
/*
** E_CL78EF_ZZ_BAD_PHYS_CVT
**
** This message is issued if the convert request flags did not specify
** physical or the lock being converted has not been used for physical
** requests. The specific problem is indicated in the message.
*/
E_CL78EF_ZZ_BAD_PHYS_CVT:SS50000_MISC_ERRORS
"LK_request: Expected physical flags & lock, got flags=%x, LockAttr=%x"
/*
** E_CL78F0_ZZ_LK_MAX_LIMIT
**
** This message is issued if the Locking System refuses to allocate any
** more locks because the system determined lock limits have been reached.
*/
E_CL78F0_ZZ_LK_MAX_LIMIT:SS50000_MISC_ERRORS
"LKexpand_list: Max %0c limit of %1d has been reached. No more locks."
/*
** E_CL78F1_ZZ_LK_RESUME
**
** This trace message is issued when resuming a lock list
*/
E_CL78F1_ZZ_LK_RESUME:SS50000_MISC_ERRORS
"LockEVENT: Resuming Lock List at %x, found from Lock at %x"
/*
** E_CL78F2_ZZ_LK_SUSPEND
**
** This trace message is issued when suspending a lock list
*/
E_CL78F2_ZZ_LK_SUSPEND:SS50000_MISC_ERRORS
"LockEVENT: Suspending Lock List at %x, found from Lock at %x"
/*
** E_CL78F3_ZZ_LK_BAD_QAST
**
** This message is issued if LK cannot queue an AST
*/
E_CL78F3_ZZ_LK_BAD_QAST:SS50000_MISC_ERRORS
"LKINTERN: Unable to queue an AST!"
/*
** E_CL7900_ZZ_LGK_DEAL_EXT
**
** This trace message describes a shared memory extent being deallocated.
*/
E_CL7900_ZZ_LGK_DEAL_EXT:SS50000_MISC_ERRORS
"vms_deallocate_ext: Deallocating extent at %x, size=%d, nextPTR=%x"
/*
** E_CL7901_ZZ_LGK_DUMP_EXT
**
** This trace message is issued when the shared memory extent dump
** subroutine (dump_extents) is called.
*/
E_CL7901_ZZ_LGK_DUMP_EXT:SS50000_MISC_ERRORS
"DUMP_EXTENTS called from '%0c'"
/*
** E_CL7902_ZZ_LGK_EXTENT
**
** This trace message is issued for each extent in the extent list
** by the dump_extents subroutine.
*/
E_CL7902_ZZ_LGK_EXTENT:SS50000_MISC_ERRORS
"%0c ExtAddr=%1x ... ExtNext=%2x ... ExtSize=%3d"
/*
** E_CL7903_ZZ_LGK_SHMEMFILE
**
** This trace message is issued to indicate which shared memory file is
** being opened.
*/
E_CL7903_ZZ_LGK_SHMEMFILE:SS50000_MISC_ERRORS
"LGK_initialize: Shared memory file is '%0c'"
/*
** E_CL7904_ZZ_LGK_NO_MEM
**
** This message is issued if the LGK_initialize routine decides that there
** is insufficient shared memory for the Logging and Locking systems to
** be initialized. The amount of shared memory which was provided is shown
** in the error message.
*/
E_CL7904_ZZ_LGK_NO_MEM:SS50000_MISC_ERRORS
"LGK_initialize: Insufficient shared LGK memory (only %d bytes)"
/*
** E_CL7905_ZZ_LGK_MEM_INIT
**
** This message is issued if LGK_initialize is unable to set up the
** shared memory manager.
*/
E_CL7905_ZZ_LGK_MEM_INIT:SS50000_MISC_ERRORS
"LGK_initialize: Unable to initialize shared memory"
/*
** E_CL7906_ZZ_LGK_WAKING_UP
**
** This trace message is issued by LGK_qast when it issued an internal
** LGK_wakeup call to awaken another process.
*/
E_CL7906_ZZ_LGK_WAKING_UP:SS50000_MISC_ERRORS
"LGK_qast: Waking up destination process"
/*
** E_CL7907_ZZ_LGK_ACB_SKIPPED
**
** This trace message is issued by LGK_execute_acb_list when it examines
** an ACB and decides that the ACB is to be executed by ANOTHER process.
*/
E_CL7907_ZZ_LGK_ACB_SKIPPED:SS50000_MISC_ERRORS
"LGK_execute; I (%d), considered this ACB ... but this one is not for me"
/*
** E_CL7908_ZZ_LGK_BAD_AST
**
** This message is issued if an ACB with an invalid function code is
** encountered.
*/
E_CL7908_ZZ_LGK_BAD_AST:SS50000_MISC_ERRORS
"LGK_execute_acb_list: bad AST found"
/*
** E_CL7909_ZZ_LGK_SLEEPING
**
** This trace message is issued when a process puts itself to sleep by
** calling LGK_sleep
*/
E_CL7909_ZZ_LGK_SLEEPING:SS50000_MISC_ERRORS
"LGK_sleep: Timeout = %f"
/*
** E_CL790A_ZZ_LGK_WAKEUP
**
** THis trace message is issued when a process calls LGK_wakeup to wake
** another process.
*/
E_CL790A_ZZ_LGK_WAKEUP:SS50000_MISC_ERRORS
"LGK_wakeup: Waking up pid %d, server_num %d, session %d"
/*
** E_CL790B_ZZ_LGK_GET_SLAVE
**
** THis trace message is issued when LGK_evinit is trying to locate
** the event slave. It displays the port number being used.
*/
E_CL790B_ZZ_LGK_GET_SLAVE:SS50000_MISC_ERRORS
"LGKSHPRT: Calling port_info with Event Slave Port Number %d"
/*
** E_CL790C_ZZ_LGK_PI_FAIL
**
** This message is issued if LGK_wakeup fails to locate the pid in question
** in the process dictionary.
*/
E_CL790C_ZZ_LGK_WAKE_FAIL:SS50000_MISC_ERRORS
"LGK_wakeup: failed to wake up process %d, process ptr came back NULL"
/*
** E_CL790D_ZZ_LGK_NO_ES_INFO
**
** This message is issued if LGK_evinit is unable to get information about
** the Event Slave by calling ProcInfo.
*/
E_CL790D_ZZ_LGK_NO_ES_INFO:SS50000_MISC_ERRORS
"LGKSHPRT: Unable to get Event Slave Information; PROCINFO status=%d.%d"
/*
** E_CL790E_ZZ_LGK_BAD_ES
**
** This message is issued if LGK_evinit gets a bad program name returned
** from the Event Slave.
*/
E_CL790E_ZZ_LGK_BAD_ES:SS50000_MISC_ERRORS
"LGKSHPRT: Event Slave Program name is %0c, expected %1c"
/*
** E_CL790F_ZZ_LGK_ACK_OK
**
** This trace message is issued by the LGK_evinit if it successfully
** completes the Event Slave handshaking protocol.
*/
E_CL790F_ZZ_LGK_ACK_OK:SS50000_MISC_ERRORS
"LGKSHPRT: ACK MESSAGE was %0c successfully retrieved"
/*
** E_CL7910_ZZ_LGK_ES_DIED
**
** This message is issued if a logging or locking system user gets a bad
** system return code on a communication call with the Event Slave which
** 'must succeed'. Certain Event Slave communications are so crucial to
** the success of the system that a failure in one of them means certain
** death to the mainline. This message is issued in those cases.
*/
E_CL7910_ZZ_LGK_ES_DIED:SS50000_MISC_ERRORS
"LGKSHPRT: Panic!!! Event Slave must have died, bad status %x"
/*
** E_CL7911_ZZ_LGK_BAD_ACK
**
** This message is issued by the Event Slave handshaking protocol in 
** LGK_evinit if it receives a bad acknowledgement message.
*/
E_CL7911_ZZ_LGK_BAD_ACK:SS50000_MISC_ERRORS
"LGKSHPRT: Event Slave acknowledgement was type/subtype=%d/%d"
/*
** E_CL7912_ZZ_LGK_BAD_ES_PORT
**
** This message is issued by the Event Slave handshaking protocol in
** LGK_evinit if it receives a bad port number response from the ES.
*/
E_CL7912_ZZ_LGK_BAD_ES_PORT:SS50000_MISC_ERRORS
"LGKSHPRT: Got Port Number %d from Event Slave, Expected Port Number %d"
/*
** E_CL7913_ZZ_LGK_TIMER_ON
**
** This message is issued if a duplicate interval timer request is 
** issued. Only one interval timer is supported.
*/
E_CL7913_ZZ_LGK_TIMER_ON:SS50000_MISC_ERRORS
"LGKSHPRT: Interval Timer is already on"
/*
** E_CL7914_ZZ_LGK_BAD_SEND
**
** This message is issued if a send_msg call to send a message to the
** Event Slave fails. This indicates trouble.
*/
E_CL7914_ZZ_LGK_BAD_SEND:SS50000_MISC_ERRORS
"%0c: write failed"
/*
** E_CL7915_ZZ_LGK_MSG_SENT
**
** This trace message is issued when a message is sent to the event slave
*/
E_CL7915_ZZ_LGK_MSG_SENT:SS50000_MISC_ERRORS
"LGKSHPRT: Message sent to Event Slave"
E_CL7916_ZZ_FSERR_FCHECK:SS50000_MISC_ERRORS
"%0c (ccode %1d) -- FCHECK failed with ccode %2d"
E_CL7917_ZZ_FSERR_FERRM:SS50000_MISC_ERRORS
"%0c (ccode %1d) -- FERRMSG failed with ccode %2d"
E_CL7918_ZZ_FSERR_DISPLAY:SS50000_MISC_ERRORS
"%0c (ccode %1d) ===> FERRMSG: '%2c'"
E_CL7919_ZZ_HPERR_HDR:SS50000_MISC_ERRORS
"********************* HP Error Message Dump **************************"
E_CL791A_ZZ_HPERR_DISPLAY:SS50000_MISC_ERRORS
"MPE/iX Message %d.%d: '%z'"
E_CL791B_ZZ_HPERR_TRLR:SS50000_MISC_ERRORS
"*********************** End of Message Dump **************************"
E_CL791C_ZZ_HPERR_3:SS50000_MISC_ERRORS
"%0c: Bad value for depth parm"
E_CL791D_ZZ_HPERR_8:SS50000_MISC_ERRORS
"%0c: Message was truncated"
E_CL791E_ZZ_HPERR_15:SS50000_MISC_ERRORS
"%0c: Encountered an unexpected trap"
E_CL791F_ZZ_HPERR_16:SS50000_MISC_ERRORS
"%0c: Cannot be called on the ICS"
E_CL7920_ZZ_HPERR_17:SS50000_MISC_ERRORS
"%0c: Unable to close system catalog"
E_CL7921_ZZ_HPERR_18:SS50000_MISC_ERRORS
"%0c: Process error stack is empty"
E_CL7922_ZZ_HPERR_19:SS50000_MISC_ERRORS
"%0c: Error opening system catalog"
E_CL7923_ZZ_HPERR_26:SS50000_MISC_ERRORS
"%0c: No error message for this error number"
E_CL7924_ZZ_HPERR_200:SS50000_MISC_ERRORS
"%0c: Error reading from system catalog"
E_CL7925_ZZ_HPERR_201:SS50000_MISC_ERRORS
"%0c: Bad subsystem ID"
E_CL7926_ZZ_HPERR_202:SS50000_MISC_ERRORS
"%0c: Bad error number"
E_CL7927_ZZ_HPERR_203:SS50000_MISC_ERRORS
"%0c: Unexpected catalog error"
E_CL7928_ZZ_HPERR_UNK:SS50000_MISC_ERRORS
"%0c: Failed with info=%1d"
E_CL7929_ZZ_HPERR_BAD_SUBSYS:SS50000_MISC_ERRORS
"%0c: Unexpected subsystem -- %1d"
E_CL792A_ZZ_HPERR_ORIG:SS50000_MISC_ERRORS
"The status value which got us into this mess originally was %d.%d"
E_CL792B_ZZ_HPERR_BAD_END:SS50000_MISC_ERRORS
"***************** ABNORMAL End of Message Dump !!! **********************"
/*
** E_CL792C_ZZ_CB_INIT_FAIL
**
** This message is issued if CB_INIT fails to initialize one of the control
** blocks used for the LG/LK mutex system.
*/
E_CL792C_ZZ_CB_INIT_FAIL:SS50000_MISC_ERRORS
"LGKMTXCB: cb_init for %0c failed with %1x"
/*
** E_CL792D_ZZ_BAD_MUTEX
**
** This message is issued by the mutex/unmutex calls if their own internal
** counter does not indicate the right value. We keep track of calling code
** attempting to call the mutex twice without releasing it, or releasing
** it twice in a row, or releasing it without getting it, etc. This message
** is issued in such cases, and indicates a SEVERE, SEVERE, SEVERE problem.
*/
E_CL792D_ZZ_BAD_MUTEX:SS50000_MISC_ERRORS
"LGKMTXCB: Bad %0c %1c call, Mutex count = %2d"
/*
** E_CL792E_ZZ_CB_LOCK_FAIL
**
** This message is issued if CBlock fails.
*/
E_CL792E_ZZ_CB_LOCK_FAIL:SS50000_MISC_ERRORS
"LGKMTXCB: Bad CBlock. Status (?) returned was %x"
/*
** E_CL792F_ZZ_CB_NOT_MINE
**
** This message is issued if a caller attempts to unock a control block
** owned by some other process.
*/
E_CL792F_ZZ_CB_NOT_MINE:SS50000_MISC_ERRORS
"LGKMTXCB: Owner of %0c Control Block is %1d, not me (%2d)"
/*
** E_CL7940_ZZ_NON_NULL_FNAME
**
** This message is issued by LOaddpath if it is passed a head or
** a tail component containing a non-null filename value. LOaddparh
** cannot handle filenames in the head and tail components of the
** path.
*/
E_CL7940_ZZ_NON_NULL_FNAME:SS50000_MISC_ERRORS
"LOaddpath: Non null filenames -- add %0c to %1c"
/*
** E_CL7941_ZZ_PATH_TOO_DEEP
**
** This message is issued by LOaddpath if it is requested to create
** a path deeper than is supported by MPE/iX (that is, a path deeper
** than two levels)
*/
E_CL7941_ZZ_PATH_TOO_DEEP:SS50000_MISC_ERRORS
"LOaddpath: Attempt to add too deep -- add %0c to %1c"
/*
** E_CL7942_ZZ_CREATE_FAILED
**
** This message is issued when LOcreate fails to create a file or group.
** (file or group is indicated in the message)
*/
E_CL7942_ZZ_CREATE_FAILED:SS50000_MISC_ERRORS
"LOcreate: could not create %0c %1c"
/*
** E_CL7943_ZZ_INVALID_PATH
**
** This message is issued when LOdbname is c`lled with an invalid path.
*/
E_CL7943_ZZ_INVALID_PATH:SS50000_MISC_ERRORS
"LOdbname: not a valid PATH: %0c"
/*
** E_CL7944_ZZ_DELETE_FAILED
**
** This message is issued when LOdelete fails to delete a file
*/
E_CL7944_ZZ_DELETE_FAILED:SS50000_MISC_ERRORS
"LOdelete: error deleting %0c"
/*
** E_CL7945_ZZ_LO_CMD_FAILED
**
** This message is issued when LO gets an error status from an MPE/iX
** command issued via HPCICOMMAND. The message provides the full
** HPCICOMMAND error information.
*/
E_CL7945_ZZ_LO_CMD_FAILED:SS50000_MISC_ERRORS
"%0c: Error running %1c command -- CmdError=%2d, ErrorType=%3d, Command was %4c"
/*
** E_CL7946_ZZ_ERR_FAILED
**
** This message is issued when LO encounters problems while trying to
** lookup explanatory text for a previous failure.
*/
E_CL7946_ZZ_ERR_FAILED:SS50000_MISC_ERRORS
"LOerror: HPERRMSG failed with %0x, Filename=%1c, Message=%2c"
/*
** E_CL7947_ZZ_LO_ERR_DUMP
**
** This message is used by LOerror to dump an operating system error message
** to the CL log file for analysis.
*/
E_CL7947_ZZ_LO_ERR_DUMP:SS50000_MISC_ERRORS
"%0c %1c: (%2d) %3c"
/*
** E_CL7948_ZZ_FROMS_BAD_PATH
**
** This message is issued by LOfroms when it encounters a bad PATH component
** in the string passed to it. The PATH component does not meet MPE/iX
** requirements.
*/
E_CL7948_ZZ_FROMS_BAD_PATH:SS50000_MISC_ERRORS
"LOfroms: Bad PATH: %0c -- Reason: %1c"
/*
** E_CL7949_ZZ_FROMS_BAD_FILE
**
** This message is issued by LOfroms when it encounters a bad FILE
** component in the string passed to it. The FILENAME component does not
** meet MPE/iX requirements.
*/
E_CL7949_ZZ_FROMS_BAD_FILE:SS50000_MISC_ERRORS
"LOfroms: Bad FILENAME: %0c -- filename is too long"
/*
** E_CL794A_ZZ_FROMS_BAD_CHAR
**
** This message is issued by LOfroms when it encounters an invalid
** character. The character in question is printed (in decimal) in the
** message.
*/
E_CL794A_ZZ_FROMS_BAD_CHAR:SS50000_MISC_ERRORS
"LOfroms: Bad input: %0c -- Invalid character (%1d) in %2c"
/*
** E_CL794B_ZZ_FROMS_NODE_ONLY
**
** This message is issued by LOfroms if it is called with a string containing
** only NODE information.
*/
E_CL794B_ZZ_FROMS_NODE_ONLY:SS50000_MISC_ERRORS
"LOfroms: Attempt to set only NODE information"
/*
** E_CL794C_ZZ_LOINGPATH_NULL
**
** This message is issued by LOingpath if it is passed incorrect NULL
** arguments:
*/
E_CL794C_ZZ_LOINGPATH_NULL:SS50000_MISC_ERRORS
"LOingpath: NULL component!"
/*
** E_CL794D_ZZ_LOLAST_BAD_DIR
**
** This message is issued by LOlast if it is asked to get the last modified
** date of a directory. The MPE/iX version of LOlast does not support this
** notion.
*/
E_CL794D_ZZ_LOLAST_BAD_DIR:SS50000_MISC_ERRORS
"LOlast: request for last modified date of directory: %0c"
/*
** E_CL794E_ZZ_LOFROMS_FAILED
**
** This message is issued by LOlist when its internal LOfroms call fails.
*/
E_CL794E_ZZ_LOFROMS_FAILED:SS50000_MISC_ERRORS
"LOlist: Error in LOfroms: (%0d) %1c"
/*
** E_CL794F_ZZ_LORENAME_FAILED
**
** This message is issued by LOrename when its FRENAME call fails.
*/
E_CL794F_ZZ_LORENAME_FAILED:SS50000_MISC_ERRORS
"LOrename: unable to rename %0c to %1c"
/*
** E_CL7950_ZZ_LOSIZE_BAD_DIR
**
** This message is issued by LOsize when it is asked to get the size of
** a directory. MPE/iX does not support this notion.
*/
E_CL7950_ZZ_LOSIZE_BAD_DIR:SS50000_MISC_ERRORS
"LOsize: Request to get size of directory %0c"
E_CL7960_ZZ_SIFOPEN_PARMS:SS50000_MISC_ERRORS
"SIfopen: Attempting to open file %0c with open options %1c"
E_CL7961_ZZ_SIFOPEN_ERR:SS50000_MISC_ERRORS
"SIfopen: Unable to open %0c -- errno=%1d, errlist=%2c"
E_CL7970_ZZ_DSMREAD_ERROR:SS50000_MISC_ERRORS
"%0c: An MPE/iX error status of %1d occurred while calling DSMREAD"
E_CL7971_ZZ_DSMWRITE_ERROR:SS50000_MISC_ERRORS
"%0c: An MPE/iX error status of %1d occurred while calling DSMWRITE"
E_CL7972_ZZ_AIFFILEGGET_ERROR:SS50000_MISC_ERRORS
"%0c: An MPE/iX error status of %1d occurred while calling AIFFILEGGET"
E_CL7973_ZZ_AIFFCLOSE_ERROR:SS50000_MISC_ERRORS
"%0c: An MPE/iX error status of %1d occurred while calling AIFFILECLOSE"
E_CL7974_ZZ_AIFFILELGET_ERROR:SS50000_MISC_ERRORS
"%0c: An MPE/iX error status of %1d occurred while calling AIFFILELGET"
/*
** This is the end of the MPE/iX-specific ZZ error messages.
*/


E_CLFE01_BS_SOCK_ERR:SS50000_MISC_ERRORS
"Could not create connection endpoint."
E_CLFE02_BS_BIND_ERR:SS50000_MISC_ERRORS
"Listen port invalid or already in use."
E_CLFE03_BS_LISTEN_ERR:SS50000_MISC_ERRORS
"Unable to listen for incoming connection."
E_CLFE04_BS_ACCEPT_ERR:SS50000_MISC_ERRORS
"Unable to accept incoming connection."
E_CLFE05_BS_CONNECT_ERR:SS50000_MISC_ERRORS
"Unable to make outgoing connection."
E_CLFE06_BS_WRITE_ERR:SS50000_MISC_ERRORS
"Write to peer process failed; it may have exited."
E_CLFE07_BS_READ_ERR:SS08006_CONNECTION_FAILURE
"Read from peer process failed; it may have exited."
E_CLFE08_BS_CLOSE_ERR:SS50000_MISC_ERRORS
"Error closing connection."
E_CLFE0A_BS_BADADDR_ERR:SS50000_MISC_ERRORS
"Listen address formatted improperly."
E_CLFE0B_BS_NOHOST:SS50000_MISC_ERRORS
"Host name unknown or illegal."
E_CLFE0C_BS_NOHOST:SS50000_MISC_ERRORS
"Port name malformed or illegal."
E_CLFE0D_BS_INTERNAL:SS50000_MISC_ERRORS
"Communication failure."
E_CLFE0F_BS_SYSERR:SS50000_MISC_ERRORS
"System communication error: %0c."
E_CLFF01_INTERRUPTED:SS50000_MISC_ERRORS
"Communication operation interrupted."
E_CLFF02_TIMEOUT:SS50000_MISC_ERRORS
"Communication operation timed out."
E_CLFF03_FAILED:SS50000_MISC_ERRORS
"Select(2) call failed; presumably system error."

NO_SQLSTATE_MAPPING
