/*
** Copyright (c) 1986, 2004 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <pc.h>
#include    <tr.h>
#include    <nm.h>
#include    <me.h>
#include    <lo.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <ulf.h>
#include    <cs.h>
#include    <lg.h>
#include    <lk.h>
#include    <cx.h>
#include    <er.h>
#include    <st.h>
#include    <cv.h>
#include    <scf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dm0p.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0pbmcb.h>

/*
**  Name: CACHEUTIL.C - Utility for showing/deleting shared memory buffer caches
**
**  Description:
**	Contains CACHEUTIL program - allows user to show what shared buffer
**	caches are allocated in the Ingres installation.  Also allows user
**	to destroy buffer cache that is left around following a server crash.
**
**	This program is necessary in the UNIX environment where the shared
**	memory segments used for shared buffer managers are not released
**	automatically by the OS when all DBMS servers connected to it are
**	terminated.  Thus if a UNIX DBMS server using a shared buffer cache
**	crashes, it will leave a shared memory segment installed on the system.
**	This is usually not a problem, as the server will clean up the segment
**	the next time it is started up.  But if the user does not wish to reuse
**	the segment, then it can only be destroyed by using this program.
**
**	The CACHEUTIL program accepts the following commands.
**
**	    LIST	- list existing shared caches.
**	    DESTROY	- destroy shared cache.
**	    SHOW	- show statistics for specified shared cache.
**	    EXIT	- exit cacheutil program.
**
**	The following are undocumented commands.
**
**	    DESTROY!	- destroy shared cache even if system thinks it
**			  is in use.
**	    DEBUG	- print out lots of useful and unuseful information
**			  about shared memory segments and buffer caches.
**	    DEBUGCACHE	- like DEBUG, but for single cache.
**
**  History:
**      15-may-89 (rogerk)
**	    Created for Terminator.
**	25-sep-1989 (rogerk)
**	    Give help on how to Exit program.
**	    Changed "Cache_util" references to "Cacheutil".
**	26-dec-1989 (greg)
**	    Porting changes --	cover for main
**	13-may-1991 (bonobo)
**	    Added the PARTIAL_LD mode ming hint.
**	07-jul-1992 (ananth)
**	    Prototyping DMF.
**	29-sep-1992 (bryanp)
**	    Prototyping LG and LK.
**	29-sep-1992 (nandak)
**	    Use one common transaction ID type.
**	5-nov-1992 (bryanp)
**	    Check return code from CSinitiate.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	15-mar-1993 (jnash)
**	    Call scf_call instead of CSinitiate to initialize CS 
**	    processing.  Without this, any new CSsemaphore calls added 
**	    before the first scf_call will fail.  Also fix a few compiler
**	    warnings.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**		bm_log_addr => bm_last_lsn.
**		Correct arguments to LKcreate_list().
**		Respond to some lint suggestions.
**		Use ule_format to report unexpected errors in LG/LK calls.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Changed node-name handling in ule_initiate call: formerly we were
**		passing a fixed-length node name, which might be null-padded
**		or blank-padded at the end. This could lead to garbage in the
**		node name portion of the error log lines, or, when we use node
**		names to generate trace log filenames, to garbage file names.
**		Now we use a null-terminated node name, from LGCn_name().
**	23-aug-1993 (jnash)
**	    Support -help as command line option.
**	08-sep-93 (swm)
**	    Move cs.h include above lg.h so that CS_SID type definition is
**	    picked up.
**	12-jun-1995 (canor01)
**	    add semaphore protection for memory allocation routines
**      06-mar-1996 (stial01)
**          Variable page size changes
**	03-jun-1996 (canor01)
**	    External semaphore calls not needed for memory allocation
**	    routines with operating-system threads.
**	23-sep-1996 (canor01)
**	    Moved global data definitions to dmfdata.c.
**	01-Apr-1997 (jenjo02)
**	    Added BMCB_DMCM flag to bmcb_status.
**      02-apr-1997 (hanch04)
**          move main to front of line for mkmingnt
**	15-may-97 (mcgem01)
**	    Clean up compiler error on NT; ERinit takes 4 parameters.
**	12-Dec-97 (bonro01)
**		Added cleanup code for cross-process semaphores during
**		MEsmdestroy().
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**      21-apr-1999 (hanch04)
**	    Replace STindex with STchr
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	24-Aug-1999 (jenjo02)
**	    Free/Modified group counts moved to sth_count from bm_gfcount,
**	    bm_gmcount.
**	30-Nov-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID parm to LKcreate_list() prototype.
**	15-Dec-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID parm to LGbegin() prototype.
**	    Removed DB_DIS_TRAN_ID parm from LKcreate_list() prototype.
**	10-Jan-2000 (jenjo02)
**	    Group counts moved back to bm_gfcount, bm_gmcount.
**	20-Apr-2000 (jenjo02)
**	    Cache keys beyond ".DAT" changed to ".DAn" where "n" is
**	    the shared memory segment number. Replaced hardcoded
**	    suffixes with CACHENAME_SUFFIX define to match dm0p.c.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	17-oct-2002 (somsa01)
**	    Corrected banner in cache_intro().
**	17-oct-2002 (somsa01)
**	    On Windows, printing out a set of characters which in number is
**	    equal to the width of the window causes an extra character to
**	    be written, giving the wraparound effect. Modified the printout
**	    such that we print out one less character.
**      27-oct-2003 (devjo01)
**          Replace last surviving LGCn_name with CXnode_name.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	10-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**      16-nov-2008 (stial01)
**          Redefined name constants without trailing blanks.
*/

/*
** Allows NS&E to build one executable for IIDBMS, DMFACP, DMFRCP, ...
*/
# ifdef	II_DMF_MERGE
# define    MAIN(argc, argv)	cacheutil_libmain(argc, argv)
# else
# define    MAIN(argc, argv)	main(argc, argv)
# endif

/*
**	mkming hints
MODE =		SETUID PARTIAL_LD

NEEDLIBS =	COMPATLIB MALLOCLIB

OWNER =		INGUSER

PROGRAM =	cacheutil
*/

/*
**  Forward_type_references
*/

/*
** CACHE_CONNECT struct - used for cache_connect(), cache_disconnect() calls.
*/
typedef struct
{
    u_i4		cache_flags;
#define		    CC_LGOPEN	    0x01
#define		    CC_DBADD	    0x02
#define		    CC_TRANSACT	    0x04
#define		    CC_LOCKLIST	    0x08
#define		    CC_BMLOCK	    0x10
#define		    CC_CONNECT	    0x20
    LG_LGID  		cache_logid;
    LG_DBID		cache_dbid;
    DB_TRAN_ID		cache_tranid;
    LG_LXID		cache_lxid;
    LK_LLID		cache_listid;
    LK_LKID		cache_lockid;
    LK_VALUE		cache_value;
    PTR			cache_memory[DM_MAX_BUF_SEGMENT];
    DM0P_BM_COMMON  	*cache_bm_common;
    DM0P_BMCB   	*cache_bmcb[DM_MAX_CACHE];
    SIZE_TYPE		cache_pages[DM_MAX_CACHE];
    char		cache_name[DB_MAXNAME];
} CACHE_CONNECT;

char	    Message_buffer[ER_MAX_LEN];	    /* Used for OS messages */
i4	    Language;			    /* ER language for ER calls */

/*
** Return Values
*/
#define		    CAC_WARN	    2
#define		    CAC_FAIL	    5
#define		    CAC_NOCACHE	    9

/*
** Command constants
*/
#define		    COM_EXIT	    1
#define		    COM_LIST	    2
#define		    COM_HELP	    3
#define		    COM_DESTROY	    4
#define		    COM_NUKE	    5
#define		    COM_SHOW	    6
#define		    COM_DEBUG	    7

GLOBALREF   DM_SVCB *dmf_svcb;

/*
**  External variables 
*/

/*
**  Forward and/or External function references.
*/
static VOID cache_help(
    i4	full);

static STATUS cache_list(VOID);

static STATUS cache_destroy(
    char    *cache_list);

static STATUS cache_nuke(
    char    *cache_list);

static STATUS cache_stats(
    char    *cache_name);

static STATUS cache_debug(VOID);

static VOID cache_get_input(
    i4	    *command,
    char    *arg_list);

static STATUS cache_init(VOID);

static STATUS cache_connect(
    char	    *cache_name,
    CACHE_CONNECT   *cache_struct,
    i4		    *dbms_count);

static STATUS cache_disconnect(
    CACHE_CONNECT   *cache_struct);

static VOID cache_intro(VOID);

static VOID cache_os_error(
    CL_ERR_DESC	*clerror);

static STATUS cache_mdebug_func(
    i4		*arg_list,
    char	*cache_key,
    CL_ERR_DESC	*err_code);

static STATUS cache_meshow_func(
    i4		*arg_list,
    char	*cache_key,
    CL_ERR_DESC	*err_code);

/*{
**  Name: main	- CACHEUTIL entry point.
**
**  Description:
**	Drive CACHEUTIL program.
**	Prompts user - gets command and executes it.
**
**  Inputs:
**	None
**
**  Outputs:
**	Prints information to terminal.
**	Returns:
**	    None
**	Exceptions:
**	    None
**
** History:
**	23-aug-1993 (jnash)
**	    Support -help as command line option.
*/
# ifdef	II_DMF_MERGE
VOID MAIN(argc, argv)
# else
VOID 
main(argc, argv)
# endif
i4	argc;
char	*argv[];
{
    STATUS	status = OK;
    i4		command = 0;
    char	arg[256];

    status = cache_init();
    if (status != OK)
	PCexit(status);

    /*
    ** Support command line -help.
    */
    if (argv[1] && STcasecmp(&argv[1][0], "-help" ) == 0)
    {
	cache_help(0);
	PCexit(E_DB_OK);
    }

    cache_intro();

    /*
    ** Get user's command and execute appropriate command routine.
    */
    while ((command != COM_EXIT) && (status == OK))
    {
	cache_get_input(&command, arg);

	switch(command)
	{
	  case COM_HELP:
	    cache_help(1);
	    break;

	  case COM_LIST:
	    status = cache_list();
	    break;

	  case COM_DESTROY:
	    status = cache_destroy(arg);
	    break;

	  case COM_NUKE:
	    status = cache_nuke(arg);
	    break;

	  case COM_SHOW:
	    status = cache_stats(arg);
	    break;

	  case COM_DEBUG:
	    status = cache_debug();
	    break;

	  case COM_EXIT:
	  default:
	    command = COM_EXIT;
	}
    }

    PCexit(status);
}

/*
** CACHE_HELP - print help on CACHEUTIL program.
**
** Description:
**	Executes HELP command in CACHEUTIL program.
**
** Inputs:
**	none
**
** Outputs:
**	Returns:
**	    none
**
** Side Effects:
**	Writes TRdisplay's to terminal.
**
** History:
**      12-jun-1989 (rogerk)
**	    Written.
**	25-sep-1989 (rogerk)
**	    Give help on how to Exit program.
[@history_template@]...
*/
static VOID
cache_help(
i4	full)
{
    CL_ERR_DESC	sys_err;
    char	buffer[25];
    char	*prompt;
    i4		read;

    if (full)
    {
	TRdisplay(" \n");
	TRdisplay("    Cacheutil is a utility program used to return information about\n");
	TRdisplay("    shared memory buffer caches installed in the Ingres Installation.\n");
	TRdisplay(" \n");
	TRdisplay("    It can be used to show brief or detailed information about existing\n");
	TRdisplay("    shared buffer caches while they are in use.\n");
	TRdisplay(" \n");
	TRdisplay("    It can also be used to destroy shared memory segments used for buffer\n");
	TRdisplay("    caches that are no longer being used.  Cacheutil will not destroy\n");
	TRdisplay("    a buffer cache that is currently being used.\n");
	TRdisplay(" \n");
	TRdisplay("    LIST -    Lists existing shared buffer managers.  Gives the size of the\n");
	TRdisplay("              cache and the number of connected DBMS servers.\n");
	TRdisplay(" \n");
	TRdisplay("    SHOW -    Lists detailed information about specified buffer cache.\n");
/*	TRdisplay("              Includes I/O counts and performance statistics.\n"); */
	TRdisplay(" \n");
	TRdisplay("    DESTROY - Destroys the shared memory segment associated with the specified\n");
	TRdisplay("              cache name.  This is needed on UNIX systems where shared segments\n");
	TRdisplay("              are allocated in a manner such that they are not automatically\n");
	TRdisplay("              released when all DBMS servers connected to them are brought down.\n");
	TRdisplay(" \n");

	/* Ask user to hit return */
	prompt = "Press RETURN to continue ...";
	TRdisplay(" \n");
	(VOID) TRrequest(buffer, sizeof(buffer), &read, &sys_err, prompt);
	TRdisplay(" \n");

	TRdisplay("              On these systems, if a DBMS with a shared memory cache fails or\n");
	TRdisplay("              is brought down in an unsupported manner, the shared memory\n");
	TRdisplay("              segment cannot be automatically cleaned up by the recovery system.\n");
	TRdisplay(" \n");
	TRdisplay("              When the failed server is restarted it will automatically clean\n");
	TRdisplay("              up the old shared segment.  In this case, Cacheutil is not\n");
	TRdisplay("              neccessary to release the shared memory.\n");
	TRdisplay(" \n");
	TRdisplay("              If no server will be restarted that specifies the same cache\n");
	TRdisplay("              name as the orphaned cache, then the shared segment must be\n");
	TRdisplay("              cleaned up through the DESTROY option of Cacheutil.\n");
    }

    TRdisplay(" \n");
    TRdisplay("    CACHEUTIL OPTIONS:\n");
    TRdisplay(" \n");
    TRdisplay("        LIST               - List installation's shared caches.\n");
    TRdisplay("        SHOW    cache_name - Give detailed statistics on shared cache.\n");
    TRdisplay("        DESTROY cache_name - Destroy shared cache.\n");
    TRdisplay("        HELP               - Print help on cacheutil program.\n");
    TRdisplay("        EXIT               - Exit cacheutil program.\n");
    TRdisplay(" \n");
}

/*
** CACHE_LIST - List shared cache's in this installation.
**
** Description:
**	Formats list of shared caches allocated to this system.
**
**	Calls MEshow_pages to get names of shared memory segments in system.
**	This generates a call to cache_meshow_func for each shared segment
**	in the installation.  The cache_meshow_func routine will search the
**	shared segments for DBMS buffer caches and formats information on
**	each - including:
**
**	    - the cache name.
**	    - the number of connections to the cache.
**
**	Shared segments are recognized as being buffer caches by actually
**	connecting to them and looking to see if they define a DMP_BMCB
**	control block.
**
** Inputs:
**	none
**
** Outputs:
**	Returns:
**	    none
**
** Side Effects:
**	Generates multiple calls to cache_meshow_func().
**	Writes TRdisplay's to terminal.
**
** History:
**      12-jun-1989 (rogerk)
**	    Written.
[@history_template@]...
*/
static STATUS
cache_list(VOID)
{
    CL_ERR_DESC	    sys_err;
    STATUS	    status;
    char	    *inst_code;

    /* Get II_INSTALLATION code */
    NMgtAt("II_INSTALLATION", &inst_code);

    TRdisplay(" \n");

    if ((inst_code == NULL) || (*inst_code == EOS))
    {
        TRdisplay("Shared Caches allocated to production installation\n");
    }
    else
    {
        TRdisplay("Shared Caches allocated to installation '%s'\n",
		    inst_code);
    }

    TRdisplay(
"-------------------------------------------------------------------------------\n");
    TRdisplay(
"|     Cache    |  Size  |    4k  |    8k  |  16k  |  32k  |  64k  | Connected |\n");
    TRdisplay(
"|      Name    | (Pages)|        |        |       |       |       |  Servers  |\n");
    TRdisplay(
"-------------------------------------------------------------------------------\n");

    status = MEshow_pages(cache_meshow_func, (PTR *)0, &sys_err);

    TRdisplay(
"-------------------------------------------------------------------------------\n");
    TRdisplay(" \n");

    return(status);
}

static STATUS
cache_meshow_func(
i4	    *arg_list,
char	    *cache_key,
CL_ERR_DESC  *err_code)
{
    CACHE_CONNECT   cache_struct;
    STATUS	    status;
    STATUS	    ret_status = OK;
    char	    *str_ptr;
    i4		    dbms_count;
    char	    cache_name[ DB_MAXNAME + 1 ];
    i4		    cache_ix;
    i4		    pages[DM_MAX_CACHE];
    DM0P_BMCB       *bm;

    /*
    ** Check cache_key to see if it looks like a shared buffer cache.
    ** The first segment of a multi-segment cache (or only segment)
    ** is suffixed with ".dat".
    */
    STncpy(cache_name, cache_key, DB_MAXNAME );
    cache_name[ DB_MAXNAME ] = '\0';
    CVlower(cache_name);
    str_ptr = STchr(cache_name, '.');
    if ((str_ptr == NULL) || (STcompare(str_ptr, CACHENAME_SUFFIX) != 0))
	return (OK);

    /*
    ** Copy the cache name - take off the suffix from the key.
    */
    str_ptr = STchr(cache_key, '.');
    STncpy( cache_name, cache_key, str_ptr - cache_key);
    cache_name[ str_ptr - cache_key ] = '\0';
    CVlower(cache_name);

    status = cache_connect(cache_name, &cache_struct, &dbms_count); 

    if (status == OK)
    {
	/*
	** Format information on buffer manager.
	*/
	for (cache_ix = 0; cache_ix < DM_MAX_CACHE; cache_ix++)
	{
	    if (bm = cache_struct.cache_bmcb[cache_ix])
		pages[cache_ix] = bm->bm_bufcnt;
	    else
		pages[cache_ix] = 0;
	}

	TRdisplay("| %13s|%7d |%7d |%7d |%6d |%6d |%6d | %9d |\n", 
	    cache_name, pages[0], pages[1], pages[2],
	    pages[3], pages[4], pages[5], dbms_count);
    }

    status = cache_disconnect(&cache_struct);
    if (status)
	ret_status = ENDFILE;

    return (ret_status);
}

/*
** CACHE_DESTROY - Destroy specified shared caches.
**
** Description:
**	Takes list of shared cache names.  Calls MEsmdestroy to destroy
**	the shared memory segments.
**
**	First checks to make sure the specified cache name exists and that
**	it is not in use.  This routine will not destroy a cache that is in use.
**
** Inputs:
**	cache_list  - list of blank separated cache names.
**
** Outputs:
**	Returns:
**	    none
**
** Side Effects:
**	Writes TRdisplay's to terminal.
**
** History:
**      12-jun-1989 (rogerk)
**	    Written.
[@history_template@]...
*/
static STATUS
cache_destroy(
char	*cache_list)
{
    CACHE_CONNECT   cache_struct;
    CL_ERR_DESC	    sys_err;
    STATUS	    status = OK;
    char	    *word_array[25];
    char	    *cache_name;
    char	    cache_key[DB_MAXNAME];
    i4		    dbms_count;
    i4		    count;
    i4		    i;

    CVlower(cache_list);

    /*
    ** Get list of cache names specified.
    */
    count = sizeof(word_array) / sizeof(*word_array);
    STgetwords(cache_list, &count, word_array);

    if (count == 0)
    {
	TRdisplay(" \n    Must specify cache_name to destroy.\n");
	return (OK);
    }

    for (i = 0; i < count && status == OK; i++)
    {
	/*
	** Get cache key from specified name.
	** Copy cache suffix to end of key name.
	*/
	cache_name = word_array[i];
	STpolycat(2, cache_name, CACHENAME_SUFFIX, cache_key);

	/*
	** Connect to buffer cache.
	*/
	status = cache_connect(cache_name, &cache_struct, &dbms_count);
	if (status == OK)
	{
	    /*
	    ** Check connection count on buffer cache.
	    */
	    if (dbms_count == 0)
	    {
		CS_cp_sem_cleanup(cache_key, &sys_err);
		status = MEsmdestroy(cache_key, &sys_err);
		if (status != OK)
		{
		    if (status == ME_NO_PERM)
		    {
			TRdisplay(" \n    No permission to destroy cache '%s'\n",
			    cache_name);
			TRdisplay("    SHARED CACHE NOT DESTROYED\n \n");
		    }
		    else
		    {
			TRdisplay(" \n    Could not destroy shared cache '%s' - status %d\n",
			    cache_name, status);
			cache_os_error(&sys_err);
			TRdisplay("    SHARED CACHE NOT DESTROYED\n \n");
		    }
		}
	    }
	    else
	    {
		TRdisplay(" \n    Shared cache '%s' is currently in use by %d DBMS servers\n",
		    cache_name, dbms_count);
		TRdisplay("    The cache cannot be destroyed while in use.\n \n");
	    }
	}
	else if (status == CAC_NOCACHE)
	{
	    TRdisplay(" \n    Shared cache '%s' does not exist\n \n", cache_name);
	}

	status = cache_disconnect(&cache_struct);
    }

    return (status);
}

/*
** CACHE_NUKE - Destroy specified shared cache regardless of state.
**
** Description:
**	Calls MEsmdestroy to destroy the specified shared memory segment.
**
**	Destroys memory segment directly without any checking.  Cache name
**	should specify full shared memory key name as it appears to ME - 
**	that is with the ".dat" suffix appended.  The name is case sensitive.
**
** Inputs:
**	cache_name  - name of cache to destroy.
**
** Outputs:
**	Returns:
**	    none
**
** Side Effects:
**	none.
**
** History:
**      12-jun-1989 (rogerk)
**	    Written.
[@history_template@]...
*/
static STATUS
cache_nuke(
char	*cache_list)
{
    CL_ERR_DESC	sys_err;
    STATUS	status = OK;
    char	*cache_key;
    char	*str_ptr;

    cache_key = STskipblank(cache_list, STlength(cache_list));
    if (cache_key == NULL)
    {
	TRdisplay(" \n    Must specify shared segment to destroy.\n");
	return (OK);
    }

    /*
    ** Only use first argument in argument list.
    */
    str_ptr = STchr(cache_key, ' ');
    if (str_ptr)
	*str_ptr = '\0';

    /*
    ** Try to destroy specified memory segment.
    */
    CVlower(cache_key);
    CS_cp_sem_cleanup(cache_key, &sys_err);
    status = MEsmdestroy(cache_key, &sys_err);
    if (status == OK)
    {
	TRdisplay(" \n    Memory segment '%s' destroyed.\n \n", cache_key);
	return (OK);
    }

    /*
    ** MEsmdestroy failed.
    */
    if (status == ME_NO_SUCH_SEGMENT)
    {
	TRdisplay(" \n    Memory segment '%s' does not exist - SEGMENT NOT DESTROYED\n \n",
	    cache_key);

	/*
	** Check to see if name has proper suffix.
	*/
	str_ptr = STchr(cache_key, '.');
	if (str_ptr == NULL)
	{
	    TRdisplay("    When using the DESTROY! command, you should specify the full cache\n");
	    TRdisplay("    name as it is passed to the underlying shared memory routines.  This\n");
	    TRdisplay("    cache name is normally of the format: 'cache_name.dat'.  For example,\n");
	    TRdisplay("    if the server's cache name is 'CACHE1' then the shared segment name\n");
	    TRdisplay("    would be 'cache1.dat'.\n \n");
	}
	else if (STcompare(str_ptr, CACHENAME_SUFFIX))
	{
	    TRdisplay("    Shared cache memory segment names generally end with a '.dat' suffix\n \n");
	}
    }
    else if (status == ME_NO_PERM)
    {
	TRdisplay(" \n    No permission to destroy memory segment '%s'\n",
	    cache_key);
	TRdisplay("    SEGMENT NOT DESTROYED\n \n");
    }
    else
    {
	TRdisplay(" \n    Destroy memory call failed with status: %d\n", status);
	cache_os_error(&sys_err);
	TRdisplay("    SEGMENT '%s' NOT DESTROYED\n \n", cache_key);
    }

    return (OK);
}

/*
** CACHE_STATS - Print statistics about specified cache.
**
** Description:
**	Connects to specified shared cache and prints statistics about it.
**
** Inputs:
**	cache_name  - name of cache to show information on.
**
** Outputs:
**	Returns:
**	    none
**
** Side Effects:
**	Writes TRdisplay's to terminal.
**
** History:
**      12-jun-1989 (rogerk)
**	    Written.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**		bm_log_addr => bm_last_lsn.
**		Respond to some lint suggestions.
**      06-mar-1996 (stial01)
**          Variable page size changes
**	01-Apr-1997 (jenjo02)
**	    Added BMCB_DMCM flag to bmcb_status.
**	24-Aug-1999 (jenjo02)
**	    Free/Modified group counts moved to sth_count from bm_gfcount,
**	    bm_gmcount.
[@history_template@]...
*/
static STATUS
cache_stats(
char	*cache_name)
{
    CACHE_CONNECT   cache_struct;
    STATUS	    status = OK;
    char	    *str_ptr;
    i4		    dbms_count;
    DM0P_BMCB       *bm;
    i4         cache_ix;
    DM0P_BM_COMMON  *bm_common;
    i4		    fcount,mcount,lcount,gfcount,gmcount,glcount;

    cache_name = STskipblank(cache_name, STlength(cache_name));
    if (cache_name == NULL)
    {
	TRdisplay(" \n    Must specify cache name to show.\n");
	return (OK);
    }

    CVlower(cache_name);

    /*
    ** Only use first argument in argument list.
    */
    str_ptr = STchr(cache_name, ' ');
    if (str_ptr)
	*str_ptr = '\0';

    /*
    ** Connect to buffer cache.
    */
    status = cache_connect(cache_name, &cache_struct, &dbms_count); 
    if (status == OK)
    {
	bm_common = cache_struct.cache_bm_common;
	TRdisplay("%80*-\n \n");
	TRdisplay("  Shared Buffer Cache %t\n", sizeof(bm_common->bmcb_name),
	    &bm_common->bmcb_name);
	TRdisplay(" \n");
	TRdisplay("    Buffer manager status: %v\n",
	    "FCFLUSH,SHARED,PASS_ABORT,PREPASS_ABORT,IOMASTER,DMCM,MT", 
		bm_common->bmcb_status);
	TRdisplay("    Buffer manager id: %8d\n", bm_common->bmcb_id);
	TRdisplay("    Closed DB cache:   %8d\n", bm_common->bmcb_dbcsize);
	TRdisplay("    Closed table cache:%8d\n", bm_common->bmcb_tblcsize);
	TRdisplay("    Connect count:     %8d\n", dbms_count);
	TRdisplay("    Consistency Points:%8d\n", bm_common->bmcb_cpcount);
	TRdisplay("    CP index:          %8d\n", bm_common->bmcb_cpindex);
	TRdisplay("    CP check value:    %8d\n", bm_common->bmcb_cpcheck);
	TRdisplay("    Server count       %8d\n", bm_common->bmcb_srv_count);
	TRdisplay("    BM Lock List Key:  (0x%x, 0x%x)\n",
	    bm_common->bmcb_lock_key.lk_uhigh, bm_common->bmcb_lock_key.lk_ulow);
	TRdisplay("    Log Flush Addr:    (0x%x, 0x%x)\n",
	    bm_common->bmcb_last_lsn.lsn_high, bm_common->bmcb_last_lsn.lsn_low);
	TRdisplay(" \n");

	for (cache_ix = 0; cache_ix < DM_MAX_CACHE; cache_ix++)
	{
	    if (!(bm = cache_struct.cache_bmcb[cache_ix]))
		continue;

	    DM0P_COUNT_ALL_QUEUES(fcount, mcount, lcount);
	    DM0P_COUNT_ALL_GQUEUES(gfcount, gmcount, glcount);

	    TRdisplay(" \n");
	    TRdisplay("  Buffer Pool for pagesize %d\n", bm->bm_pgsize);
	    TRdisplay("    Total # Buffers:   %8d\n", bm->bm_bufcnt);
	    TRdisplay("    Single  Buffers:   %8d\n", bm->bm_sbufcnt);
	    TRdisplay("    Group   Buffers:   %8d\n", bm->bm_gcnt);
	    TRdisplay("    Pages per Group:   %8d\n", bm->bm_gpages);
	    TRdisplay("    Hash buckets:      %8d\n", bm->bm_hshcnt);
	    TRdisplay("    Write Behind start:%8d\n", bm->bm_wbstart);
	    TRdisplay("    Write Behind end:  %8d\n", bm->bm_wbend);
	    TRdisplay(" \n");
	    TRdisplay("    Modify Page limit: %8d\n", bm->bm_mlimit);
	    TRdisplay("    Modify Page count: %8d\n", mcount);
	    TRdisplay("    Fixed Page count:  %8d\n", lcount);
	    TRdisplay("    Free Page limit:   %8d\n", bm->bm_flimit);
	    TRdisplay("    Free Page count:   %8d\n", fcount);
	    TRdisplay("    Free Group count:  %8d\n", gfcount);
	    TRdisplay("    Fixed Group count: %8d\n", glcount);
	    TRdisplay("    Modify Group count:%8d\n", gmcount);
	    TRdisplay(" \n");
	    TRdisplay("    Buffer clock:      %8d\n", bm->bm_clock);
	}
	TRdisplay("%80*-\n \n");
    }
    else if (status == CAC_NOCACHE)
    {
	TRdisplay(" \n    Shared cache '%s' does not exist\n \n", cache_name);
    }

    status = cache_disconnect(&cache_struct);
    return (status);
}

/*
** CACHE_DEBUG - List shared memory segments in system.
**
** Description:
**	Formats list of shared caches allocated to this system.
**	Lists information about all shared segments - tries to connect
**	to them even if they do not look like shared buffer cache.
**
**	Calls MEshow_pages to get names of shared memory segments in system.
**	This generates a call to cache_meshow_func for each shared segment
**	in the installation.
**
** Inputs:
**	none
**
** Outputs:
**	Returns:
**	    none
**
** Side Effects:
**	Generates multiple calls to cache_mdebug_func().
**	Writes TRdisplay's to terminal.
**
** History:
**      12-jun-1989 (rogerk)
**	    Written.
**      06-mar-1996 (stial01)
**          Variable page size changes
[@history_template@]...
*/
static STATUS
cache_debug(VOID)
{
    CL_ERR_DESC	    sys_err;
    STATUS	    status;
    char	    *inst_code;

    TRdisplay(" \n");

    /* Get II_INSTALLATION code */
    NMgtAt("II_INSTALLATION", &inst_code);

    if ((inst_code == NULL) || (*inst_code == EOS))
    {
        TRdisplay("Shared Caches allocated to production installation\n");
    }
    else
    {
        TRdisplay("Shared Caches allocated to installation '%s'\n",
		    inst_code);
    }

    TRdisplay(" \n");
    TRdisplay(
"-------------------------------------------------------------------------------\n");
    TRdisplay(
"|     Cache    |  Size  |    4k  |    8k  |  16k  |  32k  |  64k  | Connected |\n");
    TRdisplay(
"|      Name    | (Pages)|        |        |       |       |       |  Servers  |\n");
    TRdisplay(
"-------------------------------------------------------------------------------\n");

    status = MEshow_pages(cache_mdebug_func, (PTR *)NULL, &sys_err);

    TRdisplay(	
"-------------------------------------------------------------------------------\n");
    TRdisplay(" \n");

    return(status);
}

static STATUS
cache_mdebug_func(
i4	    *arg_list,
char	    *cache_key,
CL_ERR_DESC  *err_code)
{
    CL_ERR_DESC	    sys_err;
    STATUS	    status;
    STATUS	    ret_status = OK;
    DMP_BMSCB	    *bmsegp;
    PTR		    memory;
    SIZE_TYPE	    pages;
    char	    cache_name[DB_MAXNAME + 1];
    i4              cache_ix;
    i4              pool_pages[DM_MAX_CACHE];
    i4              dbms_count;
    DM0P_BMCB       *bm;
    DM0P_BM_COMMON  *bm_common;

    STncpy( cache_name, cache_key, DB_MAXNAME );
    cache_name[ DB_MAXNAME ] = '\0';
    CVlower(cache_name);

    status = MEget_pages(ME_MSHARED_MASK, 0, cache_name,
	&memory, &pages, &sys_err);
    if (status == OK)
    {
	bmsegp = (DMP_BMSCB *)memory;
	if (bmsegp->bms_type == BMSCB_CB)
	{
	    /*
	    ** Format information on buffer manager.
	    */
	    for (cache_ix = 0; cache_ix < DM_MAX_CACHE; cache_ix++)
	    {
		if (bmsegp->bms_bmcb[cache_ix] != 0)
		{
		    bm = (DM0P_BMCB *)((char *)bmsegp + bmsegp->bms_bmcb[cache_ix]);
		    pool_pages[cache_ix] = bm->bm_bufcnt;
		}
		else
		{
		    pool_pages[cache_ix] = 0;
		}
	    }

	    /* Count database connections, only stored in '.dat' memory */
	    if (bmsegp->bms_bm_common)
	    {
		bm_common = (DM0P_BM_COMMON *)((char *)bmsegp + bmsegp->bms_bm_common);
		dbms_count = bm_common->bmcb_srv_count;
	    }
	    else
		dbms_count = 0;

	    TRdisplay("| %14s|%7d |%7d |%7d |%6d |%6d |%6d | %9d |\n", 
		cache_key, pool_pages[0], pool_pages[1], pool_pages[2],
		pool_pages[3], pool_pages[4], pool_pages[5], dbms_count);

	    /*
	    ** Only MEfree cache memory. Other segments may yet be 
	    ** needed by other parts of iimerge, particularly lglkdata.mem
	    ** which will be referenced by LGK_rundown when called as
	    ** a PCatexit!
	    */
	    status = MEfree_pages(memory, pages, &sys_err);
	    if (status != OK)
	    {
		TRdisplay("    Error releasing shared memory - status %d\n",status);
		cache_os_error(&sys_err);
		ret_status = ENDFILE;
	    }
	}
	else
	{
	    /*
	    ** Format information on memory segment - not a buffer manager.
	    */
	    TRdisplay(
        "| %13s|%7d |        |        |       |       |       |Not a Cache|\n",
		cache_key, pages);
	}
    }
    else if (status == ME_NO_SUCH_SEGMENT)
    {
	TRdisplay(
	"| %13s|%7d |        |        |       |       |       |Not a Cache|\n",
		cache_key, 0);
    }
    else
    {
	TRdisplay("Could not connect to shared segment '%s' - status %d\n",
	    cache_key, status);
	cache_os_error(&sys_err);
    }

    return (ret_status);
}

/*
** CACHE_GET_INPUT - Get next command from user.
**
** Description:
**	Prompts user for next command.
**	Parses command and returns one of the following command constants:
**
**	    COM_HELP		- help
**	    COM_LIST		- list
**	    COM_SHOW		- show
**	    COM_DESTROY		- destroy
**	    COM_NUKE		- destroy!
**	    COM_CACHE_DEBUG	- debugcache
**	    COM_DEBUG		- debug
**	    COM_EXIT		- exit, quit
**          COM_LIST            - list all pieces of buffer manager shared mem
**
** Inputs:
**	none
**
** Outputs:
**	command	    - set to one of the command constants.
**	arg_list    - list of user supplied arguments to command - delimited
**		      by blank characters.
**	Returns:
**	    none
**
** Side Effects:
**	none.
**
** History:
**      12-jun-1989 (rogerk)
**	    Written.
[@history_template@]...
*/
static VOID
cache_get_input(
i4	*command,
char	*arg_list)
{
    STATUS	status;
    CL_ERR_DESC	sys_err;
    char	input_buffer[256];
    char	*prompt = "CACHEUTIL> ";
    char	*command_ptr;
    char	*arg_ptr, *ptr;
    char	*word_array[25];
    i4		count, i;
    i4		amount_read;

    *command = COM_EXIT;
    TRdisplay(" \n");
    for (;;)
    {
	MEfill(80, '\0', input_buffer);
	status = TRrequest(input_buffer, sizeof(input_buffer),
					    &amount_read, &sys_err, prompt);
	if (status != OK)
	    break;

	command_ptr = STskipblank(input_buffer, STlength(input_buffer));

	if ((command_ptr == NULL) || (amount_read == 0))
	    continue;

	if (STncasecmp(command_ptr, "list", 4 ) == 0)
	    *command = COM_LIST;
	else if (STncasecmp(command_ptr, "help", 4 ) == 0)
	    *command = COM_HELP;
	else if (STncasecmp(command_ptr, "debug", 5 ) == 0)
	    *command = COM_DEBUG;
	else if (STncasecmp(command_ptr, "destroy!", 8 ) == 0)
	{
	    *command = COM_NUKE;
	    arg_ptr = command_ptr + 8;
	}
	else if (STncasecmp(command_ptr, "destroy", 7 ) == 0)
	{
	    *command = COM_DESTROY;
	    arg_ptr = command_ptr + 7;
	}
	else if (STncasecmp(command_ptr, "show", 4 ) == 0)
	{
	    *command = COM_SHOW;
	    arg_ptr = command_ptr + 4;
	}
	else if ((STncasecmp(command_ptr, "quit", 4 ) == 0) ||
		 (STncasecmp(command_ptr, "exit", 4 ) == 0))
	{
	    *command = COM_EXIT;
	    break;
	}
	else
	{
	    TRdisplay(" \nUNRECOGNIZED COMMAND\n");
	    cache_help(0);
	    continue;
	}

	/*
	** Get argument list for commands that take arguments.
	*/
	if ((*command == COM_NUKE) || (*command == COM_DESTROY) || 
	    (*command == COM_SHOW))
	{
	    /*
	    ** Get rid of any comma delimiters.
	    */
	    while ((ptr = STchr(arg_ptr, ',')) != NULL)
		*ptr = ' ';

	    count = sizeof(word_array) / sizeof(*word_array);
	    STgetwords(arg_ptr, &count, word_array);

	    /*
	    ** Copy list of arguments to caller's buffer, separate with blanks.
	    */
	    arg_ptr = arg_list;
	    for (i = 0; i < count; i++)
	    {
		STcopy(word_array[i], arg_ptr);
		arg_ptr += STlength(word_array[i]);
		*arg_ptr++ = ' ';
	    }
	    *arg_ptr = '\0';
	}

	break;
    }
}

/*
** CACHE_INIT - Initialize CACHEUTIL program.
**
** Description:
**	Initialized memory system, TR system.
**	Connects to Lock driver so we can do lock requests.
**
** Inputs:
**	none
**
** Outputs:
**	Returns:
**	    OK
**	    CAC_FAIL    - could not initialize correctly.
**
** Side Effects:
**	none.
**
** History:
**      12-jun-1989 (rogerk)
**	    Written.
**	5-nov-1992 (bryanp)
**	    Check return code from CSinitiate.
**	15-mar-1993 (jnash)
**	    Call scf_call instead of CSinitiate to initialize CS 
**	    processing.  Without this, any new CSsemaphore calls added 
**	    before the first scf_call will fail.
**	26-apr-1993 (bryanp)
**	    Report LK error codes using ule_format.
**	26-jul-1993 (bryanp)
**	    Changed node-name handling in ule_initiate call: formerly we were
**		passing a fixed-length node name, which might be null-padded
**		or blank-padded at the end. This could lead to garbage in the
**		node name portion of the error log lines, or, when we use node
**		names to generate trace log filenames, to garbage file names.
**		Now we use a null-terminated node name, from LGCn_name().
**	29-sep-2002 (devjo01)
**	    Call MEadvise only if not II_DMF_MERGE.
**	27-oct-2003 (devjo01)
**	    Replace last surviving LGCn_name with CXnode_name.
**	15-May-2007 (toumi01)
**	    For supportability add process info to shared memory.
[@history_template@]...
*/
static STATUS
cache_init(VOID)
{
    CL_ERR_DESC	sys_err;
    STATUS	status1, status2;
    SCF_CB	scf_cb;
    i4	err_code;
    char	node_name[CX_MAX_NODE_NAME_LEN];

# ifndef	II_DMF_MERGE
    MEadvise(ME_INGRES_ALLOC);
# endif

    status1 = TRset_file(TR_T_OPEN, TR_OUTPUT, TR_L_OUTPUT, &sys_err);
    status2 = TRset_file(TR_I_OPEN, TR_INPUT,  TR_L_INPUT, &sys_err);

    if (status1 || status2)
	return (CAC_FAIL);

    /*
    ** Initialize CACHEUTIL to be a single user server, in the style
    ** used elsewhere (by calling scf to allocate memory).  It turns out 
    ** that this memory allocation request will result in SCF initializing 
    ** SCF and CS processing (In particular: CSinitiate and CSset_sid).  
    ** Before this call, CS semaphore requests will fail.  The memory 
    ** allocated here is not used.
    */
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_length = sizeof(SCF_CB);
    scf_cb.scf_session = DB_NOSESSION;
    scf_cb.scf_facility = DB_DMF_ID;
    scf_cb.scf_scm.scm_functions = 0;
    scf_cb.scf_scm.scm_in_pages = ((42 + SCU_MPAGESIZE - 1)
	& ~(SCU_MPAGESIZE - 1)) / SCU_MPAGESIZE;

    status1 = scf_call(SCU_MALLOC, &scf_cb);
    if (status1 != OK)
    {
	TRdisplay(" \n    The call to initialize the CS system failed.\n");
	TRdisplay("    Please make sure that Ingres is installed and the\n");
	TRdisplay("    installation is active\n");
	TRdisplay(" \n");
	return (CAC_FAIL);
    }
	
    ERinit(0, (STATUS (*)()) NULL, (STATUS (*)()) NULL,
                        (STATUS (*)()) NULL, (VOID (*)()) NULL);
    ERlangcode((char *)NULL, &Language);
    STcopy("NO MESSAGE", Message_buffer);

    if (CXcluster_enabled())
    {
	CXnode_name(node_name);
	ule_initiate(node_name, STlength(node_name), "CACHEUTIL", 9);
    }    
    else
	ule_initiate(0, 0, "CACHEUTIL", 9);

    /*
    ** Connect to lock driver.
    */
    status1 = LKinitialize(&sys_err, ERx("cacheutil"));
    if (status1 != OK)
    {
	TRdisplay(" \n    The Logging and Locking system is not Active\n");
	TRdisplay("    Please make sure that Ingres is installed and the\n");
	TRdisplay("    installation is active\n");
	TRdisplay(" \n");
	TRdisplay("    (LKinitialize fails with status %d)\n", status1);
	uleFormat(NULL, status1, &sys_err, ULE_LOG, NULL, (char *)NULL, 0L, 
	    (i4 *)NULL, &err_code, 0);
	cache_os_error(&sys_err);
	TRdisplay(" \n");
	return (CAC_FAIL);
    }

    status1 = LGinitialize(&sys_err, ERx("cacheutil"));
    if (status1 != OK)
    {
	TRdisplay(" \n    The Logging and Locking system is not Active\n");
	TRdisplay("    Please make sure that Ingres is installed and the\n");
	TRdisplay("    installation is active\n");
	TRdisplay(" \n");
	TRdisplay("    (LGinitialize fails with status %d)\n", status1);
	uleFormat(NULL, status1, &sys_err, ULE_LOG, NULL, (char *)NULL, 0L, 
	    (i4 *)NULL, &err_code, 0);
	cache_os_error(&sys_err);
	TRdisplay(" \n");
	return (CAC_FAIL);
    }

    return (OK);
}

/*
** CACHE_CONNECT - Connect to shared memory buffer cache.
**
** Description:
**
**	Connect process to the shared buffer manager specified by the
**	cache name.  Get Exclusive buffer manager lock to prevent servers
**	from connecting or disconnecting from the cache while we are
**	looking at it.
**
**	Store information about out connection status in the CACHE_CONNECT
**	control block.  This tells us exactly what we need to do to disconnect
**	from the cache.
**
**	The caller must call cache_disconnect with the same CACHE_CONNECT
**	control block for each call to cache_connect, whether that call
**	was successfull or not.
**
**	Do consistency checks on the buffer manager control block.  Check
**	if it is really identified as a Buffer Manager.  Call LK to check
**	the number of connections to the buffer manager.
**
** Inputs:
**	cache_name	- name of cache to connect to
**	cache_struct	- control block for connect and disconnect routines.
**
** Outputs:
**	bmcb		- pointer to buffer manager control block.
**	dbms_count	- number of server connections to buffer manager.
**	Returns:
**	    OK
**	    CAC_WARN	- connected to memory, but may not be a buffer manager,
**		          and connect count may not be correct.
**	    CAC_NOCACHE	- cache does not exist.
**	    CAC_FAIL    - could not connect properly.
**
** Side Effects:
**	Buffer manager lock is held exclusively between cache_connect and
**	cache_disconnect calls, preventing servers from connecting to or
**	disconnecting from this cache.
**
** History:
**      12-jun-1989 (rogerk)
**	    Written.
**      2-oct-1992 (ed)
**          Use DB_MAXNAME to replace hard coded numbers
**          - also created defines to replace hard coded character strings
**          dependent on DB_MAXNAME
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support.
**		Correct arguments to LKcreate_list().
**      06-mar-1996 (stial01)
**          Variable page size changes
**      29-Sep-2004 (fanch01)
**          Add LK_LOCAL flag support as cache names are confined to one node.
[@history_template@]...
*/
static STATUS
cache_connect(
char		*cache_name,
CACHE_CONNECT	*cache_struct,
i4		*dbms_count)
{
    STATUS	    status = OK;
    CL_ERR_DESC	    sys_err;
    DB_OWN_NAME	    user_name;
    DM0L_ADDDB	    add_info;
    LK_LOCK_KEY	    lockkey;
    i4              len_add_info;
    LK_RSB_INFO	    info_block;
    u_i4	    result_size;
    char	    cache_key[DB_MAXNAME];
    i4	    err_code;
    DMP_BMSCB       *bmsegp;
    char            *suffix;
    i4         segment_ix;
    i4         cache_ix;

    MEfill(sizeof(CACHE_CONNECT), '\0', (char *)cache_struct);
    STcopy(cache_name, cache_struct->cache_name);

    for(;;)    
    {
	status = LGopen(0, 0, 0, &cache_struct->cache_logid, 0, 0, &sys_err);
	if (status != OK)
	{
	    TRdisplay(" \n    Can't connect to Logging System\n");
	    TRdisplay("    LGopen status returned: %d\n", status);
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL, (char *)NULL, 0L, 
		(i4 *)NULL, &err_code, 0);
	    cache_os_error(&sys_err);
	    break;
	}
	cache_struct->cache_flags |= CC_LGOPEN;

	/*
	** Add CACHEUTIL to the logging system.
	** Use LG_NOTDB flag since we are not really going to open a database.
	*/
	STmove((PTR)DB_CACHEUTIL_INFO, ' ', sizeof(add_info.ad_dbname),
	    (PTR) &add_info.ad_dbname);
	MEcopy((PTR)DB_INGRES_NAME, sizeof(add_info.ad_dbowner),
	    (PTR) &add_info.ad_dbowner);
	MEcopy((PTR)"None", 4, (PTR) &add_info.ad_root);
	add_info.ad_dbid = 0;
	add_info.ad_l_root = 4;
	len_add_info = sizeof(add_info) - sizeof(add_info.ad_root) + 4;

	status = LGadd(cache_struct->cache_logid, LG_NOTDB, (char *)&add_info,
	    len_add_info, &cache_struct->cache_dbid, &sys_err);
	if (status != OK)
	{
	    TRdisplay(" \n    Can't Add Cache Util to Logging System\n");
	    TRdisplay("    LGadd status returned: %d\n", status);
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL, (char *)NULL, 0L, 
		(i4 *)NULL, &err_code, 0);
	    cache_os_error(&sys_err);
	    break;
	}
	cache_struct->cache_flags |= CC_DBADD;

	/*
	** Begin transaction in order to do LG and LK calls.
	** Must specify NOPROTECT transaction so that LG won't pick us
	** as a force-abort victim.
	*/
	STmove((PTR)DB_CACHEUTIL_USER, ' ', sizeof(DB_OWN_NAME),
	    (PTR) &user_name);
	status = LGbegin(LG_NOPROTECT, cache_struct->cache_dbid,
	    &cache_struct->cache_tranid, &cache_struct->cache_lxid,
	    sizeof(DB_OWN_NAME), &user_name.db_own_name[0], 
	    (DB_DIS_TRAN_ID*)NULL, &sys_err);
	if (status != OK)
	{
	    TRdisplay(" \n    Can't begin transaction for Cache Util\n");
	    TRdisplay("    LGbegin status returned: %d\n", status);
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL, (char *)NULL, 0L, 
		(i4 *)NULL, &err_code, 0);
	    cache_os_error(&sys_err);
	    break;
	}
	cache_struct->cache_flags |= CC_TRANSACT;

	/*
	** Create locklist to get buffer manager lock.
	*/
	status = LKcreate_list(LK_NONPROTECT, (i4) 0,
	    (LK_UNIQUE *)&cache_struct->cache_tranid,
	    &cache_struct->cache_listid, (i4)0, 
	    &sys_err);
	if (status != OK)
	{
	    TRdisplay(" \n    Can't create lock list for Cache Util\n");
	    TRdisplay("    LKcreate_list status returned: %d\n", status);
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL, (char *)NULL, 0L, 
		(i4 *)NULL, &err_code, 0);
	    cache_os_error(&sys_err);
	    break;
	}
	cache_struct->cache_flags |= CC_LOCKLIST;

	/*
	** Get buffer manager lock - lock that would be taken by a DBMS to
	** connect to a buffer cache of this name.
	**
	** This prevents us from connecting to a cache while a server is in
	** the middle of creating/destroying it.
	**
	** The lock value tells us if we are the only holder of the lock.
	*/
	lockkey.lk_type = LK_BM_LOCK;
	MEmove(STlength(cache_name), (PTR)cache_name, ' ',
	    (6 * sizeof(lockkey.lk_key1)), (PTR)&lockkey.lk_key1);
	status = LKrequest((LK_PHYSICAL | LK_NODEADLOCK | LK_LOCAL),
	    cache_struct->cache_listid, &lockkey, LK_X,
	    &cache_struct->cache_value,
	    &cache_struct->cache_lockid, (i4)0, &sys_err);
	if ((status != OK) && (status != LK_VAL_NOTVALID))
	{
	    TRdisplay(" \n    Can't get Buffer Cache Lock\n");
	    TRdisplay("    Lock Request status returned: %d\n", status);
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL, (char *)NULL, 0L, 
		(i4 *)NULL, &err_code, 0);
	    cache_os_error(&sys_err);
	    break;
	}
	status = OK;
	cache_struct->cache_flags |= CC_BMLOCK;
	break;
    }

    if (status)
    {
	if (status == LG_EXCEED_LIMIT)
	{
	    TRdisplay("    Out of Logging System resources - reduce number of\n");
	    TRdisplay("    users and try again.\n");
	}
	if (status == LK_NOLOCKS)
	{
	    TRdisplay("    Out of Locking System resources - reduce number of\n");
	    TRdisplay("    users and try again.\n");
	}
	return (CAC_FAIL);
    }

    for (segment_ix = 0; segment_ix < DM_MAX_BUF_SEGMENT; segment_ix++)
    {
	/*
	** Connect to this memory segment and see if it looks like a buffer
	** manager.
	**
	** Build cache key from the cache name by appending the suffix,
	** ".dat" for the first or only segment, ".dan" for subsequent
	** segments where "n" is the segment number.
	*/
	STpolycat(2, cache_name, CACHENAME_SUFFIX, cache_key);
	if ( segment_ix )
	    CVla(segment_ix, cache_key + STtrmwhite(cache_key) - 1);

	status = MEget_pages(ME_MSHARED_MASK, 0, cache_key,
	    &cache_struct->cache_memory[segment_ix],
	    &cache_struct->cache_pages[segment_ix], &sys_err);
	if (status != OK)
	{
	    if (status == ME_NO_SUCH_SEGMENT)
		continue;
	    else
	    {
		TRdisplay(
		 " \n    Could not connect to shared cache '%s' - status %d\n",
		    cache_name, status);
		cache_os_error(&sys_err);
		TRdisplay(" \n");
		return (CAC_FAIL);
	    }
	}

	bmsegp = (DMP_BMSCB *)cache_struct->cache_memory[segment_ix];
	if (bmsegp->bms_bm_common && segment_ix == 0)
	{
	    cache_struct->cache_bm_common = 
	                 (DM0P_BM_COMMON *)((char *)bmsegp + bmsegp->bms_bm_common);
	}
	cache_struct->cache_flags |= CC_CONNECT;
    }

    if (!(cache_struct->cache_flags & CC_CONNECT))
	return (CAC_NOCACHE);

    /*
    ** Check if it looks like a buffer manager.
    **
    ** Also if buffer manager allocated in more than one 
    ** piece of shared memory, make sure all pieces were mapped.
    **
    ** Set up pointer to buffer pools, regardless of which piece
    ** of memory it may be in.
    */
    for (segment_ix = 0; segment_ix < DM_MAX_BUF_SEGMENT; segment_ix++)
    {
	if (!cache_struct->cache_memory[segment_ix])
	    continue;

	bmsegp = (DMP_BMSCB *)cache_struct->cache_memory[segment_ix];

	/* Check if it looks like buffer manager memory */
	if (bmsegp->bms_type != BMSCB_CB)
	    continue;

	for (cache_ix = 0; cache_ix < DM_MAX_CACHE; cache_ix++)
	{
	    if (bmsegp->bms_bmcb[cache_ix])
		cache_struct->cache_bmcb[cache_ix] = (DM0P_BMCB *) 
			((char *)bmsegp + bmsegp->bms_bmcb[cache_ix]);
	}
    }

    /*
    ** Get connection count to this buffer cache.
    ** Do a show on the lock the count the number of connections to it.
    */
    status = LKshow(LK_S_SRESOURCE, (LK_LLID)0, &cache_struct->cache_lockid,
	(LK_LOCK_KEY *)0, sizeof(info_block), (PTR)&info_block,
	&result_size, (u_i4 *)NULL, &sys_err);
    if (status != OK)
    {
	TRdisplay(" \n    Can't get Cache connect count\n");
	TRdisplay("    Lock show status returned: %d\n", status);
	uleFormat(NULL, status, &sys_err, ULE_LOG, NULL, (char *)NULL, 0L, 
		(i4 *)NULL, &err_code, 0);
	cache_os_error(&sys_err);
	return (CAC_WARN);
    }
    *dbms_count = info_block.rsb_count - 1;

    return (OK);
}

/*
** CACHE_DISCONNECT - Disonnect from shared memory buffer cache.
**
** Description:
**
**	Disconnect from shared buffer manager.  Must be called after
**	all calls to cache_connect() - regardless of return status from
**	that call.  The cache_connect call will store information in
**	the CACHE_CONNECT control block to be used by cache_disconnect().
**
** Inputs:
**	cache_struct	- control block used in cache_connect() call.
**
** Outputs:
**	Returns:
**	    OK
**	    CAC_FAIL    - could not disconnect properly.
**
** Side Effects:
**	Buffer manager lock is held exclusively between cache_connect and
**	cache_disconnect calls, preventing servers from connecting to or
**	disconnecting from this cache.
**
** History:
**      12-jun-1989 (rogerk)
**	    Written.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**		Respond to some lint suggestions.
**      06-mar-1996 (stial01)
**          Variable page size changes
**      29-Sep-2004 (fanch01)
**          Add LK_LOCAL flag support as cache names are confined to one node.
[@history_template@]...
*/
static STATUS
cache_disconnect(
CACHE_CONNECT	*cache_struct)
{
    CL_ERR_DESC	    sys_err;
    STATUS	    status;
    STATUS	    ret_status = OK;
    i4	    err_code;
    i4         segment_ix;

    if (cache_struct->cache_flags & CC_CONNECT)
    {
	for (segment_ix = 0; segment_ix < DM_MAX_BUF_SEGMENT; segment_ix++)
	{
	    if (!cache_struct->cache_memory[segment_ix])
		continue;

	    status = MEfree_pages(cache_struct->cache_memory[segment_ix],
		cache_struct->cache_pages[segment_ix], &sys_err);
	    if (status != OK)
	    {
		TRdisplay("    Error freeing shared memory.\n");
		TRdisplay("    MEfree_pages status returned: %d\n", status);
		cache_os_error(&sys_err);
		ret_status = CAC_FAIL;
	    }
	    cache_struct->cache_flags &= ~CC_CONNECT;
	}
    }

    if (cache_struct->cache_flags & CC_BMLOCK)
    {
	status = LKrequest(LK_PHYSICAL | LK_CONVERT | LK_NODEADLOCK | LK_LOCAL,
	    cache_struct->cache_listid, (LK_LOCK_KEY *)NULL, LK_N,
	    &cache_struct->cache_value,
	    &cache_struct->cache_lockid, (i4)0, &sys_err);
	if (status != OK)
     	{
	    TRdisplay("    Error releasing Buffer Manager lock for Cache Util\n");
	    TRdisplay("    LKrequest status returned: %d\n", status);
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL, (char *)NULL, 0L, 
		(i4 *)NULL, &err_code, 0);
	    cache_os_error(&sys_err);
	    ret_status = CAC_FAIL;
	}
	cache_struct->cache_flags &= ~CC_BMLOCK;
    }

    if (cache_struct->cache_flags & CC_LOCKLIST)
    {
	status = LKrelease(LK_ALL, cache_struct->cache_listid, (LK_LKID *)0, 
	    (LK_LOCK_KEY *)0, (LK_VALUE *)0, &sys_err);
	if (status != OK)
     	{
	    TRdisplay("    Error releasing lock list for Cache Util\n");
	    TRdisplay("    LKrelease status returned: %d\n", status);
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL, (char *)NULL, 0L, 
		(i4 *)NULL, &err_code, 0);
	    cache_os_error(&sys_err);
	    ret_status = CAC_FAIL;
	}
	cache_struct->cache_flags &= ~(CC_LOCKLIST | CC_BMLOCK);
    }

    if (cache_struct->cache_flags & CC_TRANSACT)
    {
	status = LGend(cache_struct->cache_lxid, 0, &sys_err);
	if (status != OK)
	{ 
	    TRdisplay("    Error ending transaction for Cache Util\n");
	    TRdisplay("    LGend status returned: %d\n", status);
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL, (char *)NULL, 0L, 
		(i4 *)NULL, &err_code, 0);
	    cache_os_error(&sys_err);
	    ret_status = CAC_FAIL;
	}
	cache_struct->cache_flags &= ~CC_TRANSACT;
    }

    if (cache_struct->cache_flags & CC_DBADD)
    {
	status = LGremove(cache_struct->cache_dbid, &sys_err);
	if (status != OK)
	{
	    TRdisplay("    Error Removing Cache Util from Logging System\n");
	    TRdisplay("    LGremove status returned: %d\n", status);
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL, (char *)NULL, 0L, 
		(i4 *)NULL, &err_code, 0);
	    cache_os_error(&sys_err);
	    ret_status = CAC_FAIL;
	}
	cache_struct->cache_flags &= ~CC_DBADD;
    }

    if (cache_struct->cache_flags & CC_LGOPEN)
    {
	status = LGclose(cache_struct->cache_logid, &sys_err);
	if (status != OK)
	{
	    TRdisplay("    Error disconnecting from Logging System\n");
	    TRdisplay("    LGclose status returned: %d\n", status);
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL, (char *)NULL, 0L, 
		(i4 *)NULL, &err_code, 0);
	    cache_os_error(&sys_err);
	    ret_status = CAC_FAIL;
	}
	cache_struct->cache_flags &= ~CC_LGOPEN;
    }

    return (ret_status);
}

static VOID
cache_intro(VOID)
{
    TRdisplay("    Ingres Shared Buffer Cache Utility\n");
    cache_help(0);
}

static VOID
cache_os_error(
CL_ERR_DESC  *clerror)
{
    CL_ERR_DESC	sys_err;
    i4		msg_len;

    ERslookup((i4)0, clerror, 0, (char *) NULL, Message_buffer, ER_MAX_LEN,
	Language, &msg_len, &sys_err, 0, (ER_ARGUMENT *) NULL);
    TRdisplay("    System error : %s\n", Message_buffer);
    STcopy("NO MESSAGE", Message_buffer);
}

