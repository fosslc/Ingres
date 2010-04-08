/*
** Copyright (c) 1998, 2000 Ingres Corporation
*/

/*
**  Include Files
*/

# include <windows.h>
# include <winperf.h>
# include <compat.h>
# include <dl.h>
# include <er.h>
# include <gl.h>
# include <me.h>
# include <mh.h>
# include <nm.h>
# include <st.h>
# include <iicommon.h>
# include "oipfctrs.h"
# include "pfdata.h"
# include "pfctrmsg.h"	/* error messages */
# include "pfmsg.h"	/* error message macros and definitions */
# include "pfapi.h"
# include "pfutil.h"


/****************************************************************************
**
** Name: PFctrs - Performance DLL for Windows NT extensible performance counters
**
** Description:
** 	Implements extensible performance counters for Windows NT. This is
** 	the main program of the performance DLL that defines the counter
** 	and object data structures used to pass performance data to the 
** 	performance monitor application. 
** 
** 	The performance DLL provides the following exported functions that
** 	are called by the system in response to requests for performance data:
** 
** 	PFctrsOpen -	Initializes performance monitoring for the Ingres server
** 	PFctrsCollect -	Collects and reports performance data when requested
** 	PFctrsClose -	Closes performance monitoring
**
**  History:
**	10-Oct-1998 (wonst02)
**	    Original. (Cloned from the Windows NT sample program "perfgen.c")
** 	14-dec-1998 (wonst02)
** 	    Add definitions for cssampler objects.
** 	10-jan-1999 (wonst02)
** 	    Add definitions for cssampler event objects.
** 	16-feb-1999 (wonst02)
** 	    Add the loading of the pfServer DLL and executing the dialog.
** 	21-apr-1999 (wonst02)
** 	    Add connection parameter. Re-instate pfApiSelectSampler( ) routine.
**	10-aug-1999 (mcgem01)
**	    Change nat to i4.
**	11-aug-1999 (somsa01)
**	    Removed multiple instances from object structures and redefined
**	    as separate structures. This way, users can more dynamically
**	    customize which counters to watch.
**	23-aug-1999 (somsa01)
**	    Added code to watch particular servers.
**	16-sep-1999 (somsa01)
**	    In GetPersonalCounters(), forgot to add setting of help index
**	    for eventLOCKEVENT. Also, modified names of cache counter
**	    defines.
**	22-oct-1999 (somsa01)
**	    Added the ability to send back data for personal counters, which
**	    are defined by user queries and are stored in personal counter
**	    groups.
**	01-nov-1999 (somsa01)
**	    Fixed logic in the Collect() routine whereby Lock and Sampler
**	    counter data requests would incorrectly not get serviced.
**	26-jan-2000 (somsa01)
**	    In PFctrsOpen(), corrected the logic for retrieving the personal
**	    counters.
**	31-jan-2000 (somsa01)
**	    Changed ServerID to be dynamic.
**  25-Apr-2003 (fanra01)
**      Bug 110152
**      Remove extraneous error messages from this library when ingres is
**      not started.
****************************************************************************/

/*
**  References to constants which initialize the Object type definitions
*/

extern PFDATA_CACHE_DEFN	PfDataCacheDefn_init;
extern PFDATA_LOCK_DEFN		PfDataLockDefn;
extern PFDATA_THREAD_DEFN	PfDataThreadDefn_init;
extern PFDATA_SAMPLER_DEFN	PfDataSamplerDefn;
extern PFDATA_PERSONAL_DEFN	PfDataPersonalDefn_init;

struct clist
{
    PFDATA_CACHE_DEFN		PfDataCacheDefn;
    PFDATA_CACHE_COUNTER	*pCC;
    int				page_size;
    bool			Enabled;
    struct clist		*next;
};

struct tlist
{
    PFDATA_THREAD_DEFN		PfDataThreadDefn;
    PFDATA_THREAD_COUNTER	*pTC;
    struct tlist		*next;
};

struct PersCtrNode
{
    PERF_COUNTER_DEFINITION	PersCtr;
    PFAPI_CONN			Conn;
    bool			bConnected;
    char			*dbname;
    char			CtrName[16];
    char			*qry;
    struct PersCtrNode		*next;
};

struct plist
{
    PFDATA_PERSONAL_DEFN	PfDataPersonalDefn;
    struct PersCtrNode		*PersCtrList;
    struct plist		*next;
};

typedef struct _svrinfo IISVRINFO;

typedef struct _svrinfo
{
    int  size;
    int  servers;
    /*
    **  Bit masks for servers
    */
# define II_NMSVR   0x0001
# define II_IUSVR   0x0002
# define II_INGRES  0x0004
# define II_COMSVR  0x0008
# define II_ICESVR  0x0010
};

struct clist	*CacheList;		/* Master list of cache counters */
struct tlist	*ThreadList;		/* Master list of thread counters */
struct plist	*PersonalList;		/* Master list of personal counter
					** groups.
					*/
DWORD		dwOpenCount = 0;	/* count of "Open" threads */
BOOL		bInitOK = FALSE;	/* true = DLL initialized OK */
BOOL		bConnected = FALSE;	/* true = API connected */
DWORD		NumObjects, NumPersCtrs, LastCtrDef, LastHelpDef;
extern char	*ServerID;		/* The server to watch */

/*
**  Function Prototypes
**
**      these are used to insure that the data collection functions
**      accessed by Perflib will have the correct calling format.
*/

PM_OPEN_PROC    PFctrsOpen;
PM_COLLECT_PROC PFctrsCollect;
PM_CLOSE_PROC   PFctrsClose;

/*
** Defines, Global Variables and Constants
*/
# define UTIL_LIB   ERx("oiutil.dll")
# define PING_GCN   ERx("II_PingGCN")
# define PING_SVR   ERx("II_PingServers")

/*
** Handle to loaded util library
*/
static PTR hlibutil = NULL;
static BOOL (__cdecl *ping_func[2])() = { NULL, NULL };
static BOOL (__cdecl *ping_gcn)( void ) = NULL;
static BOOL (__cdecl *ping_svr)( IISVRINFO* ) = NULL;
static char* funcnames[] = { PING_GCN, PING_SVR, NULL };

static const char Performance_Key[] = "SYSTEM\\CurrentControlSet\\Services\\oiPfCtrs\\Performance";
static const char *Performance_Key_Ptr = Performance_Key;

static PFAPI_CONN *Conn ZERO_FILL;	/* Pointer to connection block */

typedef struct _CACHE_NAME
{
    LPCWSTR 	szInstanceName;
    const i4	PageSize;
} CACHE_NAME;

static const CACHE_NAME Cache_Name[]  =
{
    {L"2K pages ",  2*1024},
    {L"4K pages ",  4*1024},
    {L"8K pages ",  8*1024},
    {L"16K pages", 16*1024},
    {L"32K pages", 32*1024},
    {L"64K pages", 64*1024}
};

typedef struct _LOCK_NAME
{
    LPCWSTR	szInstanceName;
    const i4	LockType;
} LOCK_NAME;

static const LOCK_NAME Lock_Name[] =
{    
/*  {L"Granted",	0}, ** If we want to report granted locks */
    {L"Waiting",	1}
};

static const DWORD    Num_Lock_Instances = 
    (sizeof(Lock_Name)/sizeof(Lock_Name[0]));

typedef struct _THREAD_NAME
{
    LPCWSTR	szInstanceName;
    const i4	ThreadType;
} THREAD_NAME;

static const THREAD_NAME Thread_Name[] =
{    
    {L"Cs_Intrnl_Thread",	-1},
    {L"Normal          ",	 0},
    {L"Monitor         ",	 1},
    {L"Fast_Commit     ",	 2},
    {L"Write_Behind    ",	 3},
    {L"Server_Init     ",	 4},
    {L"Event           ",	 5},
    {L"2_Phase_Commit  ",	 6},
    {L"Recovery        ",	 7},
    {L"Logwriter       ",	 8},
    {L"Check_Dead      ",	 9},
    {L"Group_Commit    ",	10},
    {L"Force_Abort     ",	11},
    {L"Audit           ",	12},
    {L"Cp_Timer        ",	13},
    {L"Check_Term      ",	14},
    {L"Secaud_Writer   ",	15},
    {L"Write_Along     ",	16},
    {L"Read_Ahead      ",	17},
    {L"Rep_Qman        ",	18},
    {L"Lkcallback      ",	19},
    {L"Lkdeadlock      ",	20},
    {L"Sampler         ",	21},
    {L"Sort            ",	22},
    {L"Factotum        ",	23},
    {L"License         ",	24},
    {L"<unknown>       ",	25}
};
    	 
typedef struct _SAMPLER_NAME
{
    LPCWSTR	szInstanceName;
    const i4	SamplerType;
} SAMPLER_NAME;

static const SAMPLER_NAME Sampler_Name[] =
{    
    {L"Sampler",	0}
};

typedef struct _PERSONAL_NAME {
    LPCWSTR	szInstanceName;
    const i4	PersonalType;
} PERSONAL_NAME;

static const PERSONAL_NAME Personal_Name[] =
{    
    {L"Personal",	0}
};

static const DWORD    Num_Sampler_Instances = 
    (sizeof(Sampler_Name)/sizeof(Sampler_Name[0]));

/* length of UNICODE string, including trailing null */
#define MAX_INSTANCE_NAME(Name) \
    ( (lstrlenW(Name[0].szInstanceName) + 1) * sizeof(WCHAR) )

static char *DLL_functions[] = {
    "pfServer",
    NULL};

static char DLL_name[] = "pfserver";

static VOID GetCacheCounters(
	PFDATA_CACHE_DEFN	*PfDataDef,
	DWORD 			*dwCounter,
	DWORD			*dwHelp,
	DWORD			CacheType
);

static VOID GetLockCounters(
	PFDATA_LOCK_DEFN	*PfDataDef,
	DWORD 			*dwCounter,
	DWORD			*dwHelp
);

static VOID GetThreadCounters(
	PFDATA_THREAD_DEFN	*PfDataDef,
	DWORD 			*dwCounter,
	DWORD			*dwHelp
);

static VOID GetSamplerCounters(
	PFDATA_SAMPLER_DEFN	*PfDataDef,
	DWORD 			*dwCounter,
	DWORD			*dwHelp
);

static int GetPersonalCounters(
	PFDATA_PERSONAL_DEFN	*PfDataDef,
	DWORD 			*dwCounter,
	DWORD 			dwLastCounter,
	DWORD 			*dwHelp
);

static int IncrementVar(
	int	*num,
	int	amount
);


static
DWORD
GetTimeInMilliSeconds ()
{
    SYSTEMTIME  st;
    DWORD       dwReturn;

    GetSystemTime (&st);
    dwReturn = (DWORD)st.wMilliseconds;
    dwReturn += (DWORD)st.wSecond * 1000L;
    dwReturn += (DWORD)st.wMinute * 60000L;
    dwReturn += (DWORD)st.wHour * 3600000L;
    dwReturn += (DWORD)st.wDay * 86500000L;

    /* that's good enough for what it's for */

    return dwReturn;
}

/*{
** Name: load_dll	Load and run the PfServer Dialog DLL 
**
** Description:
**      Load the PfServer Dialog DLL and find its function entry point. Then run
** 	the dialog and use the server name returned in the environment variable.
**
** Inputs:
**
** Outputs:
**
**	Returns:
** 	    OK				DLL loaded OK
** 	    FAIL			Error loading DLL
** 					(More information logged in the
** 					 Windows NT Application Log)
**
**	Exceptions:
**
** Side Effects:
**	    none
**
** History:
** 	16-feb-1999 (wonst02)
** 	    Original
*/
STATUS
load_dll(void)
{
	PTR		handle;
	PTR		addr;
	CL_ERR_DESC	err ZERO_FILL;
	STATUS		status = OK;
	STATUS		ret_stat = OK;

	int (*dll_fcn)();
	int		dll_ret;

	/* load the PfServer Dialog DLL */
	status = DLprepare((char *)NULL, DLL_name, DLL_functions, &handle, &err);
	if (status != OK)
	{
	    /*
	    ** Use lang 1 which is guaranteed to be ok. The system
	    ** will return OS errors in a system dependent way
	    ** regardless of our language.
	    */
	    i4 		lang = 1;
	    i4 		msg_len = ER_MAX_LEN;
	    char	msg_buf[ER_MAX_LEN];
	    char	*msg_ptr = msg_buf;
	    CL_ERR_DESC	sys_err;

	    msg_buf[0] = EOS;
	    ERlookup(0, &err, 0, NULL, msg_buf, msg_len, lang,
	    	     &msg_len, &sys_err, 0, NULL);
	    if (msg_len)
	    {
		REPORT_ERROR_DATA_A (PFCTRS_ERROR_LOADING_DLL, LOG_USER,
	       		&status, sizeof(status), &msg_ptr);
	    }
	    else
	    {
	    	msg_ptr = DLL_name;
		REPORT_ERROR_DATA_A (PFCTRS_UNABLE_TO_LOAD_DLL, LOG_USER,
			&status, sizeof(status), &msg_ptr);
	    }
	    return FAIL;
	}

    	status = DLbind(handle, DLL_functions[0], &addr, &err);
    	if (status != OK)
    	{
	    /* function isn't there */
	    i4 		lang = 1;
	    i4 		msg_len = ER_MAX_LEN;
	    char	msg_buf[ER_MAX_LEN];
	    char	*msg_ptr = msg_buf;
	    CL_ERR_DESC	sys_err;

	    msg_buf[0] = EOS;
	    ERlookup(0, &err, 0, NULL, msg_buf, msg_len, lang,
	    	     &msg_len, &sys_err, 0, NULL);
	    if (msg_len)
	    {
		REPORT_ERROR_DATA_A (PFCTRS_ERROR_BINDING_DLL, LOG_USER,
	       		&status, sizeof(status), &msg_ptr);
	    }
	    else
	    {
	    	msg_ptr = DLL_functions[0];
		REPORT_ERROR_DATA_A (PFCTRS_UNABLE_TO_BIND_DLL, LOG_USER,
			&status, sizeof(status), &msg_ptr);
	    }
    	}

    	if (status == OK) 
	{
	    dll_fcn = (int (*)()) addr;
    	    dll_ret = (*dll_fcn)();
	    ret_stat = (dll_ret == 0) ? OK : FAIL;
	}

	status = DLunload(handle, &err);
	if (status != OK)
	{
	    i4 		lang = 1;
	    i4 		msg_len = ER_MAX_LEN;
	    char	msg_buf[ER_MAX_LEN];
	    char	*msg_ptr = msg_buf;
	    CL_ERR_DESC	sys_err;

	    msg_buf[0] = EOS;
	    ERlookup(0, &err, 0, NULL, msg_buf, msg_len, lang,
	    	     &msg_len, &sys_err, 0, NULL);
	    REPORT_ERROR_DATA_A (PFCTRS_UNABLE_TO_UNLOAD_DLL, LOG_USER,
	       	&status, sizeof(status), &msg_ptr);
	    ret_stat = FAIL;
	}

	return ret_stat;
}

/*
** Name:    PFctrsLibLoad
**
** Description:
**      Function loads and binds the PING_GCN function from the UTIL shared
**      library.
**
** Inputs:
**      handle      existing reference to loaded module
**
** Outputs:
**      handle      reference to loaded module
**      libfunc     function pointer
**
** Returns:
**      OK
**      FAIL
**      DL_OSLOAD_FAIL
**      DL_MOD_NOT_FOUND
**      DL_MEM_ACCESS
**
** History:
**      25-Apr-2003 (fanra01)
**          Created.
*/
STATUS
PFctrsLibLoad( PTR* handle, char** funcnames, BOOL (__cdecl **libfunc)() )
{
    STATUS      status = FAIL;
	PTR		    addr;
    i4          lang = 1;
    i4          i;
    i4          msg_len = ER_MAX_LEN;
    char        msg_buf[ER_MAX_LEN];
    char        *msg_ptr = msg_buf;
    CL_ERR_DESC	sys_err ZERO_FILL;
	CL_ERR_DESC	err ZERO_FILL;

    if((handle != NULL) && (*handle == NULL))
    {
        status = DLprepare_loc( NULL, UTIL_LIB, funcnames, NULL, DL_RELOC_DEFAULT,
            handle, &err );
        if (status != OK)
        {
            msg_buf[0] = EOS;
            ERlookup( 0, &err, 0, NULL, msg_buf, msg_len, lang,
                &msg_len, &sys_err, 0, NULL );
            if (msg_len)
            {
                REPORT_ERROR_DATA_A(PFCTRS_ERROR_LOADING_DLL, LOG_USER,
                    &status, sizeof(status), &msg_ptr);
            }
            else
            {
                msg_ptr = UTIL_LIB;
                REPORT_ERROR_DATA_A(PFCTRS_UNABLE_TO_LOAD_DLL, LOG_USER,
                &status, sizeof(status), &msg_ptr);
            }
        }
        else
        {
            for(i=0; (funcnames != NULL) && (*funcnames != NULL); funcnames +=1, libfunc+=1)
            {
                if ((status = DLbind( *handle, *funcnames, &addr, &err )) != OK)
                {
                    msg_buf[0] = EOS;
                    ERlookup( 0, &err, 0, NULL, msg_buf, msg_len, lang,
                        &msg_len, &sys_err, 0, NULL );
                    if (msg_len)
                    {
                        REPORT_ERROR_DATA_A(PFCTRS_ERROR_BINDING_DLL, LOG_USER,
                            &status, sizeof(status), &msg_ptr);
                    }
                }
                else
                {
                    *libfunc = (BOOL (__cdecl *)())addr;
                    status = OK;
                }
            }
        }
    }
    return(status);
}

/*
** Name: PFctrsLibUnload
**
** Description:
**      Function unloads the UTIL_LIB shared library
**      
** Inputs:
**      handle          reference returned from a previous call to PFctrsLibLoad
**      
** Outputs:
**      None.
**      
** Returns:
**      OK              successful
**      
** History:
**      25-Apr-2003 (fanra01)
**          Created.
*/
STATUS
PFctrsLibUnload( PTR handle )
{
    STATUS      status = OK;
    i4          lang = 1;
    i4          msg_len = ER_MAX_LEN;
    char        msg_buf[ER_MAX_LEN];
    char        *msg_ptr = msg_buf;
    CL_ERR_DESC	sys_err ZERO_FILL;
   	CL_ERR_DESC	err ZERO_FILL;

    status = DLunload(handle, &err);
    if (status != OK)
    {
        msg_buf[0] = EOS;
        ERlookup( 0, &err, 0, NULL, msg_buf, msg_len, lang,
                 &msg_len, &sys_err, 0, NULL );
        REPORT_ERROR_DATA_A(PFCTRS_UNABLE_TO_UNLOAD_DLL, LOG_USER,
               &status, sizeof(status), &msg_ptr);
        status = FAIL;
    }
    return(status);
}

/****************************************************************************
**
** Name: PFctrsOpen -	Initializes performance monitor for the Ingres server
**
** Description:
** 	This routine will initialize the data structures used to pass
**      data back to the registry. 
**
** Inputs:
** 	Pointer to object ID of each device to be opened (PfCtrs)
**
** Outputs:
** 	None
**	
** Returns:
** 	
** 
** Exceptions:
** 
** Side Effects:
**
** History:
**	10-Oct-1998 (wonst02)
**	    Original.
** 	14-dec-1998 (wonst02)
** 	    Add definitions for cssampler objects.
**	17-aug-1999 (somsa01)
**	    Converted cache and thread groups from instances to separate
**	    groups. Also, added personal groups.
**	22-oct-1999 (somsa01)
**	    Obtain any personal counter info from the registry, and adjust
**	    the appropriate members of the PERF_OBJECT_TYPE structure for
**	    the particular personal object.
**	26-jan-2000 (somsa01)
**	    Corrected the logic for retrieving the personal counters.
**  25-Apr-2003 (fanra01)
**      Add call to load library for PING_GCN function
****************************************************************************/

DWORD APIENTRY
PFctrsOpen(
    LPWSTR lpDeviceNames)

{
    LONG 		lstat;
    HKEY 		hKeyDriverPerf;
    DWORD 		size, type;
    DWORD 		dwFirstCounter, dwLastCounter, dwFirstHelp;
    STATUS 		status;
    int			i, RepQryID=0;
    struct clist	*cnode;
    struct tlist	*tnode;
    struct plist	*pnode;
    char		*serverid, *PersCtrs;

    CacheList = NULL;
    ThreadList = NULL;
    PersonalList = NULL;

    /*
    **  Since WINLOGON is multi-threaded and will call this routine in
    **  order to service remote performance queries, this library
    **  must keep track of how many times it has been opened (i.e.
    **  how many threads have accessed it). the registry routines will
    **  limit access to the initialization routine to only one thread 
    **  at a time so synchronization (i.e. reentrancy) should not be 
    **  a problem
    */
    if (!dwOpenCount)
    {
	/* open Eventlog interface */
	hEventLog = pfOpenEventLog();

    /*
    ** Open util library to load function
    ** Don't fail if the module can't be loaded.
    ** The reason should already be logged.
    */
    status = PFctrsLibLoad( &hlibutil, funcnames, ping_func );
    
	/*
	** Get counter and help index base values from registry
	** Open key to registry entry
	** Read First Counter and First Help values
	** update static data strucutures by adding base to 
	** offset value in structure.
	*/
	lstat = RegOpenKeyEx (
	    HKEY_LOCAL_MACHINE,
    	    Performance_Key,
	    0L,
	    KEY_READ,
	    &hKeyDriverPerf);

	if (lstat != ERROR_SUCCESS)
	{
	    REPORT_ERROR_DATA_A(PFCTRS_UNABLE_OPEN_DRIVER_KEY, LOG_USER,
				&lstat, sizeof(lstat), &Performance_Key_Ptr);

	    /*
	    ** This is fatal, if we can't get the base values of the 
	    ** counter or help names, then the names won't be available
	    ** to the requesting application  so there's not much
	    ** point in continuing.
	    */
	    goto OpenExitPoint;
	}

	size = sizeof (DWORD);
	lstat = RegQueryValueEx(
		    hKeyDriverPerf, 
		    "First Counter",
		    0L,
		    &type,
		    (LPBYTE)&dwFirstCounter,
		    &size);

	if (lstat != ERROR_SUCCESS)
	{
	    REPORT_ERROR_DATA(PFCTRS_UNABLE_READ_FIRST_COUNTER, LOG_USER,
			      &lstat, sizeof(lstat));

	    /*
	    ** This is fatal, if we can't get the base values of the 
	    ** counter or help names, then the names won't be available
	    ** to the requesting application  so there's not much
	    ** point in continuing.
	    */
	    goto OpenExitPoint;
	}

	size = sizeof (DWORD);
	lstat = RegQueryValueEx(
		    hKeyDriverPerf, 
		    "Last Counter",
		    0L,
		    &type,
		    (LPBYTE)&dwLastCounter,
		    &size);

	if (lstat != ERROR_SUCCESS)
	{
	    REPORT_ERROR_DATA(PFCTRS_UNABLE_READ_LAST_COUNTER, LOG_USER,
			      &lstat, sizeof(lstat));

	    /*
	    ** This is fatal, if we can't get the base values of the 
	    ** counter or help names, then the names won't be available
	    ** to the requesting application  so there's not much
	    ** point in continuing.
	    */
	    goto OpenExitPoint;
	}

	size = sizeof (DWORD);
	lstat = RegQueryValueEx(
		    hKeyDriverPerf, 
		    "First Help",
		    0L,
		    &type,
		    (LPBYTE)&dwFirstHelp,
		    &size);

	if (lstat != ERROR_SUCCESS)
	{
	    REPORT_ERROR_DATA(PFCTRS_UNABLE_READ_FIRST_HELP, LOG_USER,
			      &lstat, sizeof(lstat));

	    /*
	    ** This is fatal, if we can't get the base values of the 
	    ** counter or help names, then the names won't be available
	    ** to the requesting application  so there's not much
	    ** point in continuing.
	    */
	    goto OpenExitPoint;
	}
 
	/*
	** Get the server ID to watch.
	*/
	NMgtAt("II_PF_SERVER", &serverid);
	ServerID = (char *)MEreqmem(0, STlength(serverid)+1, TRUE, NULL);
	STcopy(serverid, ServerID);

	LastCtrDef = dwFirstCounter - 2;
	LastHelpDef = dwFirstHelp - 2;
	
	/* Issue initialization call to ME */
	MEadvise(ME_USER_ALLOC);

	/*
	** Cache Manager Objects
	*/
	for (i = 1; i <= NUM_CACHE_SIZES; i++)
	{
	    cnode = (struct clist *)MEreqmem(0, sizeof(struct clist), TRUE,
					     NULL);
	    STRUCT_ASSIGN_MACRO(PfDataCacheDefn_init, cnode->PfDataCacheDefn);
	    cnode->page_size = (int)MHpow(2, i)*1024;
	    GetCacheCounters(&cnode->PfDataCacheDefn, &LastCtrDef,
			     &LastHelpDef, cnode->page_size);

	    if (!CacheList)
		CacheList = cnode;
	    else
	    {
		struct clist	*pptr = CacheList;

		while (pptr->next)
		    pptr = pptr->next;
		pptr->next = cnode;
	    }
	}
    
	/*
	** Resource Locks Object
	*/
	GetLockCounters(&PfDataLockDefn, &LastCtrDef, &LastHelpDef);

	/*
	** CSsampler Thread Objects
	*/
	for (i = 1; i <= NUM_THREADS; i++)
	{
	    tnode = (struct tlist *)MEreqmem(0, sizeof(struct tlist), TRUE,
					     NULL);
	    STRUCT_ASSIGN_MACRO(PfDataThreadDefn_init, tnode->PfDataThreadDefn);
	    GetThreadCounters(&tnode->PfDataThreadDefn, &LastCtrDef,
			      &LastHelpDef);

	    if (!ThreadList)
		ThreadList = tnode;
	    else
	    {
		struct tlist	*pptr = ThreadList;

		while (pptr->next)
		    pptr = pptr->next;
		pptr->next = tnode;
	    }
	}

	/*
	** CSsampler Block Object
	*/
	GetSamplerCounters(&PfDataSamplerDefn, &LastCtrDef, &LastHelpDef);

	/*
	** Now, retrieve the personal groups that could be set up.
	*/
	NumObjects = NUM_PERF_OBJECTS;

	type = REG_MULTI_SZ;
	size = sizeof (DWORD);
	lstat = RegQueryValueEx(
		    hKeyDriverPerf, 
		    "Personal Counters",
		    0L,
		    &type,
		    NULL,
		    &size);

	if (lstat == ERROR_SUCCESS)
	{
	    PersCtrs = (char *)MEreqmem(0, size, TRUE, NULL);

	    RegQueryValueEx(
		    hKeyDriverPerf, 
		    "Personal Counters",
		    0L,
		    &type,
		    (LPBYTE)PersCtrs,
		    &size);
	}
	else
	    PersCtrs = NULL;

	NumPersCtrs = 0;

	for(;;)
	{
	    pnode = (struct plist *)MEreqmem(0, sizeof(struct plist), TRUE,
					     NULL);
	    STRUCT_ASSIGN_MACRO(PfDataPersonalDefn_init,
				pnode->PfDataPersonalDefn);
	    pnode->PersCtrList = NULL;
	    if (GetPersonalCounters(&pnode->PfDataPersonalDefn, &LastCtrDef,
				    dwLastCounter, &LastHelpDef))
	    {
		MEfree((PTR)pnode);
		break;
	    }

	    /*
	    ** If there are any personal counters associated with this
	    ** personal group, set up to use them.
	    */
	    if (PersCtrs)
	    {
		char	*cptr, *cptr2, idx[33];
		i4	len;
		DWORD	CtrOffset;
		PFDATA_PERSONAL_DEFN	*DefPtr;

		/*
		** The format of the Registry entry is:
		** "GroupName;GroupID;CounterName;DefaultScale;Database;Query;"
		*/
		cptr = PersCtrs;

		/*
		** This is the offset of the last counter member of the normal
		** personal data definition block.
		*/
		CtrOffset =
			PfDataPersonalDefn_init.numtlswrites_def.CounterOffset;

		DefPtr = &(pnode->PfDataPersonalDefn);

		/*
		** Find the personal counters for this group.
		*/
		while (*cptr != '\0')
		{
		    /*
		    ** Ignore the GroupName.
		    */
		    cptr = STstrindex(cptr, ";", 0, FALSE);
		    cptr++;

		    len = STstrindex(cptr, ";", 0, FALSE) - cptr;
		    if (!STbcompare(cptr, len,
			ultoa(DefPtr->PfDataPersonalObject.ObjectNameTitleIndex,
			idx, 10), len, FALSE))
		    {
			struct PersCtrNode	*node;

			NumPersCtrs++;
			DefPtr->PfDataPersonalObject.TotalByteLength +=
				sizeof(PERF_COUNTER_DEFINITION) +
				sizeof(DWORD);
			DefPtr->PfDataPersonalObject.DefinitionLength +=
				sizeof(PERF_COUNTER_DEFINITION);
			DefPtr->PfDataPersonalObject.NumCounters++;

			node = (struct PersCtrNode *)MEreqmem(0,
					sizeof(struct PersCtrNode), TRUE, NULL);
			node->next = NULL;
			MEfill(sizeof(node->Conn), 0, (PTR)&node->Conn);
			node->bConnected = FALSE;
			node->PersCtr.ByteLength =
				sizeof(PERF_COUNTER_DEFINITION);
			node->PersCtr.CounterNameTitleIndex =
				IncrementVar(&LastCtrDef, 2);
			node->PersCtr.CounterHelpTitleIndex =
				IncrementVar(&LastHelpDef, 2);
			node->PersCtr.CounterNameTitle =
				node->PersCtr.CounterHelpTitle = 0;

			/*
			** Ignore the counter name in the registry. We
			** don't need this information here.
			*/
			cptr2 = STstrindex(cptr, ";", 0, FALSE);
			cptr2++;
			cptr2 = STstrindex(cptr2, ";", 0, FALSE);

			/* Get the default scale. */
			cptr2++;
			node->PersCtr.DefaultScale = atoi(cptr2);
			cptr2 = STstrindex(cptr2, ";", 0, FALSE);

			/* Get the database name. */
			cptr2++;
			len = STstrindex(cptr2, ";", 0, FALSE) - cptr2;
			node->dbname = (char *)MEreqmem(0, len + 1, TRUE, NULL);
			STlcopy(cptr2, node->dbname, len);
			cptr2 = STstrindex(cptr2, ";", 0, FALSE);

			/*
			** Create a unique name for the counter. This will
			** be used as the repeat query id. The format will
			** be: "PersCtr<number>"
			*/
			sprintf(node->CtrName, "PersCtr%d", ++RepQryID);

			/* Get the query. */
			cptr2++;
			len = STstrindex(cptr2, ";", 0, FALSE) - cptr2;
			node->qry = (char *)MEreqmem(0, len + 1, TRUE, NULL);
			STlcopy(cptr2, node->qry, len);
			cptr2 = STstrindex(cptr2, ";", 0, FALSE);

			node->PersCtr.DetailLevel = PERF_DETAIL_ADVANCED;
			node->PersCtr.CounterType = PERF_COUNTER_RAWCOUNT;
			node->PersCtr.CounterSize = sizeof(DWORD);

			CtrOffset += sizeof(DWORD);
			node->PersCtr.CounterOffset = CtrOffset;

			if (!pnode->PersCtrList)
			    pnode->PersCtrList = node;
			else
			{
			    struct PersCtrNode	*PersPtr = pnode->PersCtrList;

			    while (PersPtr->next)
				PersPtr = PersPtr->next;
			    PersPtr->next = node;
			}
		    }

		    /*
		    ** Get to the next personal counter.
		    */
		    cptr = cptr2;
		    while (*(cptr++) != '\0');
		}
	    }

	    if (!PersonalList)
		PersonalList = pnode;
	    else
	    {
		struct plist	*pptr = PersonalList;

		while (pptr->next)
		    pptr = pptr->next;
		pptr->next = pnode;
	    }

	    NumObjects++;
	}

	if (PersCtrs)
	    MEfree(PersCtrs);
	RegCloseKey (hKeyDriverPerf); /* close key to registry */

    	Conn = (PFAPI_CONN *)MEreqmem(0, sizeof(*Conn), TRUE, &status);
	if (status != OK)
	{
	    REPORT_ERROR_DATA(PFCTRS_UNABLE_TO_GET_MEMORY, LOG_USER,
			      &status, sizeof(status));
	    return ERROR_SUCCESS;
	}

	status = pfApiInit(Conn);	/* Initialize the pfApi service */
	if (status != OK)
	{
	    REPORT_ERROR_DATA(PFCTRS_UNABLE_TO_INIT_API, LOG_USER,
			      &status, sizeof(status));
	    return ERROR_SUCCESS;
	}

	bInitOK = TRUE; /* ok to use this function */
    }

    dwOpenCount++;  /* increment OPEN counter */

    lstat = ERROR_SUCCESS; /* for successful exit */

OpenExitPoint:

    return lstat;
}


/****************************************************************************
**
** Name: PFctrsCollect - Collects and reports performance data when requested
**
** Description:
** 	This routine will return the data for the server counters.
** 
** 	The first time data collection is called, it connects a session to the
** 	Ingres DBMS server and runs a query to collect the IMA measurements.
** 	Then it retrieves rows from the IMA table and returns the results.
**
** Inputs:
**	lpValueName	pointer to a wide character string passed by registry.
**	lppData		pointer to the address of the buffer to receive the 
** 			completed PerfDataBlock and subordinate structures. 
** 			This routine will append its data to the buffer
** 			starting at the point referenced by *lppData.
**	lpcbTotalBytes	the address of the DWORD that tells the size in bytes
** 			of the buffer referenced by the lppData argument
**	NumObjectTypes	the address of the DWORD to receive the number of 
** 			objects added by this routine 
**
** Outputs:
**	lppData		points to the first byte after the data structure added 
** 			by this routine. This routine updated the value at 
** 			lppdata after appending its data.
**	lpcbTotalBytes	the number of bytes added by this routine is written to 
** 			the DWORD pointed to by this argument
**	NumObjectTypes	the number of objects added by this routine is written 
** 			to the DWORD pointed to by this argument
**	
** Returns:
**	ERROR_MORE_DATA if buffer passed is too small to hold data any error 
** 			conditions encountered are reported to the event log if
**			event logging is enabled.
**	ERROR_SUCCESS	if success or any other error. Errors, however are
**			also reported to the event log.
** 	
** Exceptions:
** 
** Side Effects:
**
** History:
**	10-Oct-1998 (wonst02)
**	    Original.
** 	14-dec-1998 (wonst02)
** 	    Add definitions for cssampler objects.
**	17-aug-1999 (somsa01)
**	    Split the cache and thread groups from instances into separate
**	    groups. Also, added personal groups.
**	22-oct-1999 (somsa01)
**	    Send back data for any personal counters as well.
**  25-Apr-2003 (fanra01)
**      Add call of loaded ping_func function and only allow connect if
**      gcn is alive.
****************************************************************************/

DWORD APIENTRY
PFctrsCollect(
    IN      LPWSTR  lpValueName,
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes)

{
    /*  Variables for reformating the data */

    PERF_INSTANCE_DEFINITION	*pPerfInstanceDefinition;
    DWORD			instance, row, num_rows;
    ULONG			SpaceNeeded;
    DWORD			dwQueryType, dwTime;
    char			*vnode;
    char			dbname[DB_MAXNAME*2+3];	/* space for
							** "vnode::dbname"
							*/
    STATUS 			status;
    PFDATA_CACHE_DEFN		*pPfDataCacheDefn;
    PFDATA_LOCK_DEFN 		*pPfDataLockDefn;
    PFDATA_THREAD_DEFN  	*pPfDataThreadDefn;
    PFDATA_SAMPLER_DEFN 	*pPfDataSamplerDefn;
    PFDATA_PERSONAL_DEFN	*pPfDataPersonalDefn;
    PFDATA_LOCK_COUNTER 	*pLC;
    PFDATA_SAMPLER_COUNTER 	*pSC;
    PFDATA_PERSONAL_COUNTER	*pPC;
    IMA_DMF_CACHE_STATS		*cache;   /* page sizes: 2,4,8,16,32,64K */
    IMA_LOCKS			*lock;	  /* Active locks in the server */
    IMA_CSSAMPLER_THREADS	*thread;  /* Thread types */
    IMA_CSSAMPLER		*sampler; /* The CSsampler block */
    struct clist		*cptr;
    struct tlist		*tptr;
    struct plist		*pptr;
    struct PersCtrNode		*PCPtr;
    BOOL                gcn_alive = FALSE;

    /*
    ** Before doing anything else, see if Open went OK
    */
    if (!bInitOK)
    {
	/* unable to continue because open failed. */
	*lpcbTotalBytes = (DWORD) 0;
	*lpNumObjectTypes = (DWORD) 0;
	return ERROR_SUCCESS; /* yes, this is a successful exit */
    }
    
    /* see if this is a foreign (i.e. non-NT) computer data request */
    dwQueryType = pfGetQueryType (lpValueName);
    
    if (dwQueryType == QUERY_FOREIGN)
    {
	/*
	** This routine does not service requests for data from
	** Non-NT computers
	*/
	*lpcbTotalBytes = (DWORD) 0;
	*lpNumObjectTypes = (DWORD) 0;
	return ERROR_SUCCESS;
    }
    else if (dwQueryType == QUERY_ITEMS)
    {
	bool	CounterFound = FALSE;

	/*
	** First, let's see if it is a cache counter.
	*/
	cptr = CacheList;
	while (cptr && !CounterFound)
	{
	    CounterFound = pfIsNumberInUnicodeList(
		cptr->PfDataCacheDefn.PfDataCacheObject.\
		ObjectNameTitleIndex, lpValueName);
	    cptr = cptr->next;
	}

	if ( !CounterFound &&
	     !(CounterFound = pfIsNumberInUnicodeList (PfDataLockDefn.\
					PfDataLockObject.ObjectNameTitleIndex,
					lpValueName)))
	{
	    /*
	    ** Then, let's see if this is a thread counter.
	    */
	    tptr = ThreadList;
	    while (tptr && !CounterFound)
	    {
		CounterFound = pfIsNumberInUnicodeList(
			tptr->PfDataThreadDefn.PfDataThreadObject.\
			ObjectNameTitleIndex, lpValueName);
		tptr = tptr->next;
	    }
	}

	if ( !CounterFound &&
	     !(CounterFound = pfIsNumberInUnicodeList (PfDataSamplerDefn.\
					PfDataSamplerObject.\
					ObjectNameTitleIndex, lpValueName)))
	{
	    /*
	    ** Let's see if this is a personal group counter.
	    */
	    pptr = PersonalList;
	    while (pptr && !CounterFound)
	    {
		CounterFound = pfIsNumberInUnicodeList(
			pptr->PfDataPersonalDefn.PfDataPersonalObject.\
			ObjectNameTitleIndex, lpValueName);
		pptr = pptr->next;
	    }

	    if (!CounterFound)
	    {
		/*
		** Request received for data object not provided by
		** this routine.
		*/
		*lpcbTotalBytes = (DWORD) 0;
		*lpNumObjectTypes = (DWORD) 0;
		return ERROR_SUCCESS;
	    }
	}
    }

    /*
    ** ping the gcn each time a collect is requested as personal counters
    ** can connect to different targets.
    ** Error messages are returned if only the gcn is alive in a local
    ** instance or if the vnode config is incorrect in a client instance.
    */
    if (ping_func[0] != NULL)
    {
        IISVRINFO svrinfo;

        
        if ((gcn_alive = ping_func[0]()) == TRUE)
        {
            svrinfo.size = sizeof(IISVRINFO);
            svrinfo.servers = 0;

            if ((ping_func[1] != NULL) && ((status = ping_func[1]( &svrinfo )) == OK))
            {
                gcn_alive = (svrinfo.servers & (II_INGRES | II_COMSVR)) ? TRUE : FALSE;
            }
        }
    }
    else
    {
        gcn_alive = TRUE;
    }
     
    if ((!bConnected) && (gcn_alive == TRUE))
    {
	/* Get enough memory for the rows of all tables */
    	Conn->cache_stats = (IMA_DMF_CACHE_STATS *)
	    MEreqmem(0, 
		(sizeof(*cache) * NUM_CACHE_SIZES) +
		(sizeof(*lock) * Num_Lock_Instances) +
		(sizeof(*thread) * NUM_THREADS) +
		(sizeof(*sampler) * Num_Sampler_Instances), 
		TRUE, &status);
	if (status != OK)
	{
	    Conn->cache_stats = NULL;
	    REPORT_ERROR_DATA(PFCTRS_UNABLE_TO_GET_MEMORY, LOG_USER,
			      &status, sizeof(status));
	    bInitOK = FALSE;
	    *lpcbTotalBytes = (DWORD) 0;
    	    *lpNumObjectTypes = (DWORD) 0;
	    return ERROR_SUCCESS;
	}
		
	/*
	** Load the pfServer DLL to display the dialog
	** 	for the user to select a local/remote server.
	*/
/*	load_dll();	Remove until DLL working correctly */
		
	NMgtAt( "II_PF_VNODE", &vnode );
	if (vnode[0] != '\0')
	    STprintf(dbname, "%s::IMADB", vnode);
	else
	    STcopy("IMADB", dbname);

	Conn->status = pfApiConnect (dbname, "", "", Conn, TRUE);
	if (Conn->status == OK)
	    bConnected = TRUE;
	else
	{
	    bInitOK = FALSE;
	    *lpcbTotalBytes = (DWORD) 0;
    	    *lpNumObjectTypes = (DWORD) 0;
	    return ERROR_SUCCESS;	/* error reported already */
	}
    }
	else
	{
        if (gcn_alive == FALSE)
        {
            /*
            ** gcn is not alive
            */
            *lpcbTotalBytes = (DWORD) 0;
    	    *lpNumObjectTypes = (DWORD) 0;
            return ERROR_SUCCESS;	/* error reported already */
        }
	}

    /*
    ** Connect to the databases involved in any personal counters.
    */
    pptr = PersonalList;
    while (pptr)
    {
	if (pptr->PersCtrList)
	{
	    PCPtr = pptr->PersCtrList;
	    while (PCPtr)
	    {
		if ((!PCPtr->bConnected) && (gcn_alive == TRUE))
		{
		    PCPtr->Conn.state = PFCTRS_STAT_INITED;
		    PCPtr->Conn.status = pfApiConnect(PCPtr->dbname, "", "",
						&PCPtr->Conn, FALSE);
		    if (PCPtr->Conn.status == OK)
			PCPtr->bConnected = TRUE;
		    else
		    {
			bInitOK = FALSE;
			*lpcbTotalBytes = (DWORD) 0;
			*lpNumObjectTypes = (DWORD) 0;
			return ERROR_SUCCESS;	/* error reported already */
		    }
		}

		PCPtr = PCPtr->next;
	    }
	}

	pptr = pptr->next;
    }

    cache = Conn->cache_stats;
    lock = (IMA_LOCKS *)&cache[NUM_CACHE_SIZES];
    thread = (IMA_CSSAMPLER_THREADS *)&lock[Num_Lock_Instances];
    sampler = (IMA_CSSAMPLER *)&thread[NUM_THREADS];

    /*
    ** I copied the comment block below from the Microsoft example, but
    ** I do not really understand if it applies or not, so BEWARE....
    */
    /*
    **  Always return an "instance sized" buffer after the definition blocks
    **	to prevent perfmon from reading bogus data. This is strictly a hack
    **	to accomodate how PERFMON handles the "0" instance case.
    **	By doing this, perfmon won't choke when there are no instances
    **	and the counter object & counters will be displayed in the
    **	list boxes, even though no instances will be listed.
    */
    SpaceNeeded = 
	/* Cache Group */
	NUM_CACHE_SIZES * (sizeof(PFDATA_CACHE_DEFN) +
	  (sizeof(PERF_INSTANCE_DEFINITION) +
	  MAX_INSTANCE_NAME(Cache_Name) +   /* size of unicode instance names */
	  sizeof(PFDATA_CACHE_COUNTER))) + 
	/* Lock Group */
    	sizeof(*pPfDataLockDefn) +
	  (Num_Lock_Instances * 
	  (sizeof(*pPerfInstanceDefinition) +
	  MAX_INSTANCE_NAME(Lock_Name) +    /* size of unicode instance names */
	  sizeof(*pLC))) +
	/* Thread Group */
	NUM_THREADS * (sizeof(PFDATA_THREAD_DEFN) + 
	  (sizeof(PERF_INSTANCE_DEFINITION) +
	  MAX_INSTANCE_NAME(Thread_Name) +  /* size of unicode instance names */
	  sizeof(PFDATA_THREAD_COUNTER) +
	  sizeof(WCHAR))) +	/* HACK!! Later on when we actually fill in
				** the thread block, it will be 2 bytes
				** bigger than what we figure out it to be
				** here.
				*/
	/* Sampler Group */
    	sizeof(*pPfDataSamplerDefn) +
	  (Num_Sampler_Instances * 
	  (sizeof(*pPerfInstanceDefinition) +
	  MAX_INSTANCE_NAME(Sampler_Name) + /* size of unicode instance names */
	  sizeof(*pSC)));

    /*
    ** Adjust the space needed with any personal groups.
    */
    if (NumObjects > NUM_PERF_OBJECTS)
    {
	SpaceNeeded +=  (NumPersCtrs * sizeof(PERF_COUNTER_DEFINITION)) +
			(NumPersCtrs * sizeof(DWORD)) +
			((NumObjects - NUM_PERF_OBJECTS) * 
			(sizeof(*pPfDataPersonalDefn) +
			sizeof(*pPerfInstanceDefinition) +
			MAX_INSTANCE_NAME(Personal_Name) +
			sizeof(*pPC) +
			sizeof(WCHAR)));  /* HACK!! Later on when we actually
					  ** fill in the thread block, it will
					  ** be 2 bytes bigger than what we
					  ** figure out it to be here.
					  */
    }

    if ( *lpcbTotalBytes < SpaceNeeded )
    {
	*lpcbTotalBytes = (DWORD) 0;
	*lpNumObjectTypes = (DWORD) 0;
	return ERROR_MORE_DATA;
    }

    /* Get current time for this sample */
    dwTime = GetTimeInMilliSeconds();

    *lpcbTotalBytes = (DWORD) 0;

    /*
    ** Copy the (constant, initialized) Object Type and counter definitions
    ** to the caller's data buffer for the Cache Counters.
    */
    num_rows = NUM_CACHE_SIZES;
    status = pfApiSelectCache (Conn, &cache[0], &num_rows);
    if (status != OK)
    {
	bInitOK = FALSE;
	*lpcbTotalBytes = (DWORD) 0;
    	*lpNumObjectTypes = (DWORD) 0;
	return ERROR_SUCCESS;	/* Select error... error reported already */
    }

    cptr = CacheList;
    for (instance = 0, row = 0; instance < NUM_CACHE_SIZES; instance++)
    {
	if (cptr == CacheList)
	    pPfDataCacheDefn = (PFDATA_CACHE_DEFN *)*lppData;
	else
	    pPfDataCacheDefn = (PFDATA_CACHE_DEFN *)pPerfInstanceDefinition;

	memmove(pPfDataCacheDefn, &cptr->PfDataCacheDefn,
		sizeof(PFDATA_CACHE_DEFN));
	
	/*
	** Get data to return for each instance.
	*/
	pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
					&pPfDataCacheDefn[1];

	pfBuildInstanceDefinition(
		pPerfInstanceDefinition,
		(PVOID *)&cptr->pCC,
		0,
		0,
		(DWORD)-1,
		Cache_Name[instance].szInstanceName);

	cptr->pCC->CounterBlock.ByteLength = sizeof(*cptr->pCC);
	if (cache[row].page_size == Cache_Name[instance].PageSize)
	{
	    cptr->pCC->fix_count = cache[row].fix_count;
	    cptr->pCC->unfix_count = cache[row].unfix_count;
	    cptr->pCC->read_count = cache[row].read_count;
	    cptr->pCC->write_count = cache[row].write_count;
	    cptr->pCC->group_read_count = cache[row].group_buffer_read_count;
	    cptr->pCC->group_write_count = cache[row].group_buffer_write_count;
	    cptr->pCC->dirty_unfix_count = cache[row].dirty_unfix_count;
	    cptr->pCC->force_count = cache[row].force_count;
	    cptr->pCC->io_wait_count = cache[row].io_wait_count;
	    cptr->pCC->hit_count = cache[row].hit_count;
	    cptr->pCC->free_buffer_count = cache[row].free_buffer_count;
	    cptr->pCC->free_buffer_waiters = cache[row].free_buffer_waiters;
	    cptr->pCC->fixed_buffer_count = cache[row].fixed_buffer_count;
	    cptr->pCC->modified_buffer_count = cache[row].modified_buffer_count;
	    cptr->pCC->free_group_buffer_count =
					cache[row].free_group_buffer_count;
	    cptr->pCC->fixed_group_buffer_count =
					cache[row].fixed_group_buffer_count;
	    cptr->pCC->modified_group_buffer_count =
					cache[row].modified_group_buffer_count;
	    cptr->pCC->flimit = cache[row].flimit;
	    cptr->pCC->mlimit = cache[row].mlimit;
	    cptr->pCC->wbstart = cache[row].wbstart;
	    cptr->pCC->wbend = cache[row].wbend;

	    cptr->Enabled = TRUE;
	    row++;
	}
	else
	    cptr->Enabled = FALSE;

	/* update instance pointer for next instance */
	pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)&cptr->pCC[1];

	pPfDataCacheDefn->PfDataCacheObject.TotalByteLength =
		(PBYTE)pPerfInstanceDefinition - (PBYTE)pPfDataCacheDefn;
	*lpcbTotalBytes += pPfDataCacheDefn->PfDataCacheObject.TotalByteLength;

	/* set instance count */
	pPfDataCacheDefn->PfDataCacheObject.NumInstances = 1;

	cptr = cptr->next;
    }

    /*
    ** Copy the (constant, initialized) Object Type and counter definitions
    ** to the caller's data buffer for the Lock Counters.
    */
    pPfDataLockDefn = (PFDATA_LOCK_DEFN *)pPerfInstanceDefinition;

    memmove(pPfDataLockDefn, &PfDataLockDefn, sizeof(PfDataLockDefn));

    /*
    **	Get data to return
    */
    pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
				&pPfDataLockDefn[1];

    num_rows = Num_Lock_Instances;
    status = pfApiSelectLocks (Conn, &lock[0], &num_rows);
    if (status != OK)
    {
	bInitOK = FALSE;
	*lpcbTotalBytes = (DWORD) 0;
    	*lpNumObjectTypes = (DWORD) 0;
	return ERROR_SUCCESS;	/* Select error... error reported already */
    }

    pfBuildInstanceDefinition(
	pPerfInstanceDefinition,
	(PVOID *)&pLC,
	0,
	0,
	(DWORD)-1,
	Lock_Name[0].szInstanceName);

    pLC->CounterBlock.ByteLength = sizeof(*pLC);
    /* Number of waiting threads */
    pLC->lock_thread_count = lock[0].threadcount;
    /* Num. resources waited upon */
    pLC->lock_resource_count = lock[0].resourcecount;

    /* update instance pointer for next instance */
    pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)&pLC[1];

    pPfDataLockDefn->PfDataLockObject.TotalByteLength = 
	(PBYTE)pPerfInstanceDefinition - (PBYTE)pPfDataLockDefn;
    *lpcbTotalBytes += pPfDataLockDefn->PfDataLockObject.TotalByteLength;

    /* set instance count */
    pPfDataLockDefn->PfDataLockObject.NumInstances = num_rows;
    
    /*
    ** Copy the (constant, initialized) Object Type and counter definitions
    ** to the caller's data buffer for the Thread Counters.
    */
    num_rows = NUM_THREADS;
    status = pfApiSelectThreads (Conn, &thread[0], &num_rows);
    if (status != OK)
    {
	bInitOK = FALSE;
	*lpcbTotalBytes = (DWORD) 0;
    	*lpNumObjectTypes = (DWORD) 0;
	return ERROR_SUCCESS;	/* Select error... error reported already */
    }

    tptr = ThreadList;
    for (instance = 0, row = 0; instance < NUM_THREADS; instance++)
    {
	pPfDataThreadDefn = (PFDATA_THREAD_DEFN *)pPerfInstanceDefinition;

	memmove(pPfDataThreadDefn, &tptr->PfDataThreadDefn,
		sizeof(tptr->PfDataThreadDefn));
	/*
	** Get data to return for each instance
	*/
	pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
					&pPfDataThreadDefn[1];

	pfBuildInstanceDefinition(
	    pPerfInstanceDefinition,
	    (PVOID *)&tptr->pTC,
	    0,
	    0,
	    (DWORD)-1,
	    Thread_Name[instance].szInstanceName);

	tptr->pTC->CounterBlock.ByteLength = sizeof(*tptr->pTC);
	if (row < num_rows)
	{
	    tptr->pTC->numthreadsamples = thread[row].numthreadsamples;
	    tptr->pTC->stateFREE = thread[row].stateFREE;
	    tptr->pTC->stateCOMPUTABLE = thread[row].stateCOMPUTABLE;
	    tptr->pTC->stateEVENT_WAIT = thread[row].stateEVENT_WAIT;
	    tptr->pTC->stateMUTEX = thread[row].stateMUTEX;
	    tptr->pTC->facilityCLF = thread[row].facilityCLF;
	    tptr->pTC->facilityADF = thread[row].facilityADF;
	    tptr->pTC->facilityDMF = thread[row].facilityDMF;
	    tptr->pTC->facilityOPF = thread[row].facilityOPF;
	    tptr->pTC->facilityPSF = thread[row].facilityPSF;
	    tptr->pTC->facilityQEF = thread[row].facilityQEF;
	    tptr->pTC->facilityQSF = thread[row].facilityQSF;
	    tptr->pTC->facilityRDF = thread[row].facilityRDF;
	    tptr->pTC->facilitySCF = thread[row].facilitySCF;
	    tptr->pTC->facilityULF = thread[row].facilityULF;
	    tptr->pTC->facilityDUF = thread[row].facilityDUF;
	    tptr->pTC->facilityGCF = thread[row].facilityGCF;
	    tptr->pTC->facilityRQF = thread[row].facilityRQF;
	    tptr->pTC->facilityTPF = thread[row].facilityTPF;
	    tptr->pTC->facilityGWF = thread[row].facilityGWF;
	    tptr->pTC->facilitySXF = thread[row].facilitySXF;
	    tptr->pTC->eventDISKIO = thread[row].eventDISKIO;
	    tptr->pTC->eventMESSAGEIO = thread[row].eventMESSAGEIO;
	    tptr->pTC->eventLOGIO = thread[row].eventLOGIO;
	    tptr->pTC->eventLOG = thread[row].eventLOG;
	    tptr->pTC->eventLOCK = thread[row].eventLOCK;
	    tptr->pTC->eventLOGEVENT =  thread[row].eventLOGEVENT;
	    tptr->pTC->eventLOCKEVENT = thread[row].eventLOCKEVENT;

	    row++;
	}

	/* update instance pointer for next instance */
	pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)&tptr->pTC[1];

	pPfDataThreadDefn->PfDataThreadObject.TotalByteLength = 
		(PBYTE)pPerfInstanceDefinition - (PBYTE)pPfDataThreadDefn;
	*lpcbTotalBytes +=
		pPfDataThreadDefn->PfDataThreadObject.TotalByteLength;

	/* set instance count */
	pPfDataThreadDefn->PfDataThreadObject.NumInstances = 1;

	tptr = tptr->next;
    }
    
    /*
    ** Copy the (constant, initialized) Object Type and counter definitions
    ** to the caller's data buffer for the Sampler Counters.
    */
    pPfDataSamplerDefn = (PFDATA_SAMPLER_DEFN *)pPerfInstanceDefinition;

    memmove(pPfDataSamplerDefn, &PfDataSamplerDefn, sizeof(PfDataSamplerDefn));

    /*
    ** Get data to return 
    */
    pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
				 &pPfDataSamplerDefn[1];

    num_rows = Num_Sampler_Instances;
    status = pfApiSelectSampler (Conn, &sampler[0], &num_rows);
    if (status != OK)
    {
	bInitOK = FALSE;
	*lpcbTotalBytes = (DWORD) 0;
    	*lpNumObjectTypes = (DWORD) 0;
	return ERROR_SUCCESS;	/* Select error... error reported already */
    }

    pfBuildInstanceDefinition(
	pPerfInstanceDefinition,
	(PVOID *)&pSC,
	0,
	0,
	(DWORD)-1,
	Sampler_Name[0].szInstanceName);

    pSC->CounterBlock.ByteLength = sizeof(*pSC);
    pSC->numsamples_count    = sampler[0].numsamples_count;
    pSC->numtlssamples_count = sampler[0].numtlssamples_count;
    pSC->numtlsslots_count   = sampler[0].numtlsslots_count;
    pSC->numtlsprobes_count  = sampler[0].numtlsprobes_count;
    pSC->numtlsreads_count   = sampler[0].numtlsreads_count;
    pSC->numtlsdirty_count   = sampler[0].numtlsdirty_count;
    pSC->numtlswrites_count  = sampler[0].numtlswrites_count;

    /* update instance pointer for next instance */
    pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)&pSC[1];

    pPfDataSamplerDefn->PfDataSamplerObject.TotalByteLength = 
	(PBYTE)pPerfInstanceDefinition - (PBYTE)pPfDataSamplerDefn;
    *lpcbTotalBytes += pPfDataSamplerDefn->PfDataSamplerObject.TotalByteLength;

    /* set instance count */
    pPfDataSamplerDefn->PfDataSamplerObject.NumInstances = num_rows;

    /*
    ** Now, return any personal group info.
    */
    if (PersonalList)
    {
	int			i;
	PTR			SrcPtr, DestPtr;

	pptr = PersonalList;
	for (i = NUM_PERF_OBJECTS+1; i <= (int)NumObjects; i++)
	{
	    /*
	    ** Copy the (constant, initialized) Object Type and counter
	    ** definitions to the caller's data buffer.
	    */
	    pPfDataPersonalDefn =
		(PFDATA_PERSONAL_DEFN *)pPerfInstanceDefinition;
	    memmove(pPfDataPersonalDefn, &pptr->PfDataPersonalDefn,
		    sizeof(pptr->PfDataPersonalDefn));

	    /*
	    ** If there are any personal counters for this group, copy over
	    ** their definitions as well.
	    */
	    if (pptr->PersCtrList)
	    {
		struct PersCtrNode	*PCtr = pptr->PersCtrList;
		PTR			DefPtr;

		DefPtr = (PTR)pPfDataPersonalDefn +
			 sizeof(pptr->PfDataPersonalDefn);

		while (PCtr)
		{
		    memmove(DefPtr, &PCtr->PersCtr, sizeof(PCtr->PersCtr));
		    DefPtr += sizeof(PCtr->PersCtr);
		    PCtr = PCtr->next;
		}

		/*
		** Get data to return for each instance.
		*/
		pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)DefPtr;
	    }
	    else
	    {
		/*
		** Get data to return for each instance.
		*/
		pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
						&pPfDataPersonalDefn[1];
	    }

	    pfBuildInstanceDefinition(
			pPerfInstanceDefinition,
			(PVOID *)&pPC,
			0,
			0,
			(DWORD)-1,
			Personal_Name[0].szInstanceName);

	    /*
	    ** Copy the data that we've already gathered into the personal
	    ** group, if there was any data retrieved.
	    */
	    pPC->CounterBlock.ByteLength = sizeof(*pPC);
	    DestPtr = NULL;

	    cptr = CacheList;
	    while (cptr)
	    {
		if (!DestPtr)
		    DestPtr = (PTR)pPC + sizeof(PERF_COUNTER_BLOCK);
		else
		{
		    DestPtr +=  sizeof(PFDATA_CACHE_COUNTER) -
				sizeof(PERF_COUNTER_BLOCK);
		}

		if (cptr->Enabled)
		{
		    SrcPtr = (PTR)cptr->pCC + sizeof(PERF_COUNTER_BLOCK);
		    MEcopy( SrcPtr, sizeof(PFDATA_CACHE_COUNTER) -
			    sizeof(PERF_COUNTER_BLOCK), DestPtr );
		}

		cptr = cptr->next;
	    }

	    DestPtr +=  sizeof(PFDATA_CACHE_COUNTER) -
			sizeof(PERF_COUNTER_BLOCK);
	    SrcPtr = (PTR)pLC + sizeof(PERF_COUNTER_BLOCK);
	    MEcopy( SrcPtr, sizeof(PFDATA_LOCK_COUNTER) -
		    sizeof(PERF_COUNTER_BLOCK), DestPtr );

	    tptr = ThreadList;
	    while (tptr)
	    {
		if (tptr == ThreadList)
		{
		    DestPtr +=  sizeof(PFDATA_LOCK_COUNTER) -
				sizeof(PERF_COUNTER_BLOCK);
		}
		else
		{
		    DestPtr +=  sizeof(PFDATA_THREAD_COUNTER) -
				sizeof(PERF_COUNTER_BLOCK);
		}

		SrcPtr = (PTR)tptr->pTC + sizeof(PERF_COUNTER_BLOCK);
		MEcopy( SrcPtr, sizeof(PFDATA_THREAD_COUNTER) -
			sizeof(PERF_COUNTER_BLOCK), DestPtr);

		tptr = tptr->next;
	    }

	    DestPtr +=  sizeof(PFDATA_THREAD_COUNTER) -
			sizeof(PERF_COUNTER_BLOCK);
	    SrcPtr = (PTR)pSC + sizeof(PERF_COUNTER_BLOCK);
	    MEcopy( SrcPtr, sizeof(PFDATA_SAMPLER_COUNTER) -
		    sizeof(PERF_COUNTER_BLOCK), DestPtr);

	    /*
	    ** If there are any personal counters for this group, copy over
	    ** their data as well.
	    */
	    if (pptr->PersCtrList)
	    {
		struct PersCtrNode	*PCPtr = pptr->PersCtrList;
		DWORD			value;

		DestPtr +=  sizeof(PFDATA_SAMPLER_COUNTER) -
			    sizeof(PERF_COUNTER_BLOCK);

		while (PCPtr)
		{
		    status = pfApiExecSelect(&PCPtr->Conn, PCPtr->CtrName,
					     PCPtr->qry, &value);
		    if (status != OK)
		    {
			bInitOK = FALSE;
			*lpcbTotalBytes = (DWORD) 0;
			*lpNumObjectTypes = (DWORD) 0;
			return ERROR_SUCCESS;
		    }

		    MEcopy(&value, sizeof(value), DestPtr);
		    DestPtr += sizeof(value);
		    pPC->CounterBlock.ByteLength += sizeof(value);
		    PCPtr = PCPtr->next;
		}

		/* update instance pointer for next instance */
		pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)DestPtr;
	    }
	    else
	    {
		/* update instance pointer for next instance */
		pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)&pPC[1];
	    }

	    pPfDataPersonalDefn->PfDataPersonalObject.TotalByteLength =
		(PBYTE)pPerfInstanceDefinition - (PBYTE)pPfDataPersonalDefn;
	    *lpcbTotalBytes +=
		pPfDataPersonalDefn->PfDataPersonalObject.TotalByteLength;

	    /* set instance count */
	    pPfDataPersonalDefn->PfDataPersonalObject.NumInstances = 1;

	    pptr = pptr->next;
	}
    }

    /* update arguments for return */
    *lppData = (PVOID)pPerfInstanceDefinition;
    *lpNumObjectTypes = NumObjects;

    return ERROR_SUCCESS;
}


/****************************************************************************
**
** Name: PFctrsClose -	Closes performance monitoring
**
** Description:
**	This routine closes the open handles to the server performance counters
** 	and, when the last thread calls, terminates the pfApi service.
**
** Inputs:
**
** Outputs:
**	
** Returns:
**	ERROR_SUCCESS
** 	
** Exceptions:
** 
** Side Effects:
**
** History:
**	10-Oct-1998 (wonst02)
**	    Original.
**	17-aug-1999 (somsa01)
**	    Added cleanup of Cache, Thread and Personal linked lists.
**	22-oct-1999 (somsa01)
**	    Added cleanup of personal counter related structures.
**
****************************************************************************/

DWORD APIENTRY
PFctrsClose( )
{
	STATUS 	status;

	/*
	** Disconnect from the Ingres DBMS server
	*/
	if (Conn)
	{
    	    pfApiDisconnect (Conn, TRUE);
	}

	MEfree(ServerID);

	/*
	** Free up the cache group linked list.
	*/
	if (CacheList)
	{
	    struct clist	*pptr;

	    while (CacheList)
	    {
		pptr = CacheList->next;
		MEfree((PTR)CacheList);
		CacheList = pptr;
	    }
	}
    
	/*
	** Free up the thread group linked list.
	*/
	if (ThreadList)
	{
	    struct tlist	*pptr;

	    while (ThreadList)
	    {
		pptr = ThreadList->next;
		MEfree((PTR)ThreadList);
		ThreadList = pptr;
	    }
	}
    
	/*
	** Free up the personal group linked list.
	*/
	if (PersonalList)
	{
	    struct plist	*pptr;
	    struct PersCtrNode	*PCPtr;

	    while (PersonalList)
	    {
		/*
		** Disconnect any personal counter connections, then free
		** up any memory blocks.
		*/
		if (PersonalList->PersCtrList)
		{
		    while (PersonalList->PersCtrList)
		    {
			if (PersonalList->PersCtrList->bConnected)
			{
			    pfApiDisconnect(&PersonalList->PersCtrList->Conn,
					    FALSE);
			}
			MEfree(PersonalList->PersCtrList->dbname);
			MEfree(PersonalList->PersCtrList->qry);

			PCPtr = PersonalList->PersCtrList->next;
			MEfree((PTR)PersonalList->PersCtrList);
			PersonalList->PersCtrList = PCPtr;
		    }
		}

		pptr = PersonalList->next;
		MEfree((PTR)PersonalList);
		PersonalList = pptr;
	    }
	}
    
	if (!(--dwOpenCount)) {
	    /* 
	    ** When this is the last thread, terminate the pfApi service.
	    */
	    if (Conn)
	    {
	    	status = pfApiTerm(Conn);
	    	if (status != OK)
	    	{
	    	    REPORT_ERROR_DATA (PFCTRS_UNABLE_TO_TERM_API, LOG_USER,
		    		&status, sizeof(status));
	    	}
	    	if (Conn->cache_stats)
		    MEfree((PTR)Conn->cache_stats);
	    	MEfree((PTR)Conn);
	    }
        /*
        ** Unloaded UTIL_LIB library
        */
        status = PFctrsLibUnload( hlibutil );

	    pfCloseEventLog();
	}

    return ERROR_SUCCESS;
}

static VOID
GetCacheCounters(
	PFDATA_CACHE_DEFN	*PfDataDef,
	DWORD 			*dwCounter,
	DWORD			*dwHelp,
	DWORD			CacheType)
{
    PfDataDef->PfDataCacheObject.ObjectNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->PfDataCacheObject.ObjectHelpTitleIndex =
						IncrementVar(dwHelp, 2);

    /* assign index of default counter */
    switch(CacheType)
    {
	case 2048:
	    PfDataDef->PfDataCacheObject.DefaultCounter = 
		(PFCTRS_2K_CACHE_HIT_COUNT - PFCTRS_2K_CACHE_OBJ)/2 - 1;
	    break;

	case 4096:
	    PfDataDef->PfDataCacheObject.DefaultCounter = 
		(PFCTRS_4K_CACHE_HIT_COUNT - PFCTRS_4K_CACHE_OBJ)/2 - 1;
	    break;

	case 8192:
	    PfDataDef->PfDataCacheObject.DefaultCounter = 
		(PFCTRS_8K_CACHE_HIT_COUNT - PFCTRS_8K_CACHE_OBJ)/2 - 1;
	    break;

	case 16384:
	    PfDataDef->PfDataCacheObject.DefaultCounter = 
		(PFCTRS_16K_CACHE_HIT_COUNT - PFCTRS_16K_CACHE_OBJ)/2 - 1;
	    break;

	case 32768:
	    PfDataDef->PfDataCacheObject.DefaultCounter = 
		(PFCTRS_32K_CACHE_HIT_COUNT - PFCTRS_32K_CACHE_OBJ)/2 - 1;
	    break;

	case 65536:
	    PfDataDef->PfDataCacheObject.DefaultCounter = 
		(PFCTRS_64K_CACHE_HIT_COUNT - PFCTRS_64K_CACHE_OBJ)/2 - 1;
	    break;
    }

    PfDataDef->fix_count_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->fix_count_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->unfix_count_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->unfix_count_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->read_count_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->read_count_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->write_count_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->write_count_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->group_read_count_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->group_read_count_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->group_write_count_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->group_write_count_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->dirty_unfix_count_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->dirty_unfix_count_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->force_count_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->force_count_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->io_wait_count_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->io_wait_count_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->hit_count_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->hit_count_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->free_buffer_count_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->free_buffer_count_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->free_buffer_waiters_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->free_buffer_waiters_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->fixed_buffer_count_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->fixed_buffer_count_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->modified_buffer_count_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->modified_buffer_count_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->free_group_buffer_count_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->free_group_buffer_count_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->fixed_group_buffer_count_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->fixed_group_buffer_count_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->modified_group_buffer_count_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->modified_group_buffer_count_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->flimit_def.CounterNameTitleIndex = IncrementVar(dwCounter, 2);
    PfDataDef->flimit_def.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->mlimit_def.CounterNameTitleIndex = IncrementVar(dwCounter, 2);
    PfDataDef->mlimit_def.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->wbstart_def.CounterNameTitleIndex = IncrementVar(dwCounter, 2);
    PfDataDef->wbstart_def.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->wbend_def.CounterNameTitleIndex = IncrementVar(dwCounter, 2);
    PfDataDef->wbend_def.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
}

static VOID
GetLockCounters(
	PFDATA_LOCK_DEFN	*PfDataDef,
	DWORD 			*dwCounter,
	DWORD			*dwHelp)
{
    PfDataDef->PfDataLockObject.ObjectNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->PfDataLockObject.ObjectHelpTitleIndex = IncrementVar(dwHelp, 2);

    /* assign index of default counter */
    PfDataDef->PfDataLockObject.DefaultCounter = 
		(PFCTRS_LOCK_WAITS_COUNT - PFCTRS_LOCK_OBJ)/2 - 1;

    PfDataDef->lockwaits_def.CounterNameTitleIndex = IncrementVar(dwCounter, 2);
    PfDataDef->lockwaits_def.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->lockresources_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->lockresources_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
}

static VOID
GetThreadCounters(
	PFDATA_THREAD_DEFN	*PfDataDef,
	DWORD 			*dwCounter,
	DWORD			*dwHelp)
{
    PfDataDef->PfDataThreadObject.ObjectNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->PfDataThreadObject.ObjectHelpTitleIndex =
						IncrementVar(dwHelp, 2);

    /* assign index of default counter */
    PfDataDef->PfDataThreadObject.DefaultCounter = -1;
	
    PfDataDef->numthreadsamples_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateFREE_def.CounterNameTitleIndex = IncrementVar(dwCounter, 2);
    PfDataDef->stateCOMPUTABLE_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateEVENT_WAIT_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateMUTEX_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityCLF_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityADF_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDMF_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityOPF_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityPSF_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQEF_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQSF_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRDF_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySCF_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityULF_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDUF_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGCF_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRQF_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityTPF_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGWF_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySXF_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventDISKIO_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGIO_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventMESSAGEIO_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOG_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCK_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGEVENT_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCKEVENT_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);

    PfDataDef->numthreadsamples_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateFREE_def.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->stateCOMPUTABLE_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateEVENT_WAIT_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateMUTEX_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityCLF_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityADF_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDMF_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityOPF_def.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->facilityPSF_def.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->facilityQEF_def.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->facilityQSF_def.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->facilityRDF_def.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->facilitySCF_def.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->facilityULF_def.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->facilityDUF_def.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->facilityGCF_def.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->facilityRQF_def.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->facilityTPF_def.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->facilityGWF_def.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->facilitySXF_def.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->eventDISKIO_def.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGIO_def.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->eventMESSAGEIO_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOG_def.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCK_def.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGEVENT_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCKEVENT_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
}

static VOID
GetSamplerCounters(
	PFDATA_SAMPLER_DEFN	*PfDataDef,
	DWORD 			*dwCounter,
	DWORD			*dwHelp)
{
    PfDataDef->PfDataSamplerObject.ObjectNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->PfDataSamplerObject.ObjectHelpTitleIndex =
						IncrementVar(dwHelp, 2);

    /* assign index of default counter */
    PfDataDef->PfDataSamplerObject.DefaultCounter = 
		(PFCTRS_SAMP_NUMSAMPLES_COUNT - PFCTRS_SAMP_OBJ)/2 - 1;

    PfDataDef->numsamples_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numtlssamples_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numtlsslots_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numtlsprobes_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numtlsreads_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numtlsdirty_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numtlswrites_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);

    PfDataDef->numsamples_def.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->numtlssamples_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->numtlsslots_def.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->numtlsprobes_def.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->numtlsreads_def.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->numtlsdirty_def.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->numtlswrites_def.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
}

static int
GetPersonalCounters(
	PFDATA_PERSONAL_DEFN	*PfDataDef,
	DWORD 			*dwCounter,
	DWORD 			dwLastCounter,
	DWORD 			*dwHelp)
{
    PfDataDef->PfDataPersonalObject.ObjectNameTitleIndex =
						IncrementVar(dwCounter, 2);
    if (PfDataDef->PfDataPersonalObject.ObjectNameTitleIndex > dwLastCounter)
    {
	*dwCounter -= 2;
	return(1);
    }

    PfDataDef->PfDataPersonalObject.ObjectHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->PfDataPersonalObject.DefaultCounter = -1;

    /* Fill in the cache counters */
    PfDataDef->fix_count_def_2K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->fix_count_def_2K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->unfix_count_def_2K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->unfix_count_def_2K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->read_count_def_2K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->read_count_def_2K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->write_count_def_2K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->write_count_def_2K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->group_read_count_def_2K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->group_read_count_def_2K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->group_write_count_def_2K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->group_write_count_def_2K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->dirty_unfix_count_def_2K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->dirty_unfix_count_def_2K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->force_count_def_2K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->force_count_def_2K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->io_wait_count_def_2K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->io_wait_count_def_2K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->hit_count_def_2K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->hit_count_def_2K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->free_buffer_count_def_2K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->free_buffer_count_def_2K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->free_buffer_waiters_def_2K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->free_buffer_waiters_def_2K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->fixed_buffer_count_def_2K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->fixed_buffer_count_def_2K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->modified_buffer_count_def_2K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->modified_buffer_count_def_2K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->free_group_buffer_count_def_2K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->free_group_buffer_count_def_2K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->fixed_group_buffer_count_def_2K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->fixed_group_buffer_count_def_2K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->modified_group_buffer_count_def_2K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->modified_group_buffer_count_def_2K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->flimit_def_2K.CounterNameTitleIndex = IncrementVar(dwCounter, 2);
    PfDataDef->flimit_def_2K.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->mlimit_def_2K.CounterNameTitleIndex = IncrementVar(dwCounter, 2);
    PfDataDef->mlimit_def_2K.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->wbstart_def_2K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->wbstart_def_2K.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->wbend_def_2K.CounterNameTitleIndex = IncrementVar(dwCounter, 2);
    PfDataDef->wbend_def_2K.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);

    PfDataDef->fix_count_def_4K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->fix_count_def_4K.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->unfix_count_def_4K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->unfix_count_def_4K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->read_count_def_4K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->read_count_def_4K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->write_count_def_4K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->write_count_def_4K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->group_read_count_def_4K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->group_read_count_def_4K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->group_write_count_def_4K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->group_write_count_def_4K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->dirty_unfix_count_def_4K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->dirty_unfix_count_def_4K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->force_count_def_4K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->force_count_def_4K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->io_wait_count_def_4K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->io_wait_count_def_4K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->hit_count_def_4K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->hit_count_def_4K.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->free_buffer_count_def_4K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->free_buffer_count_def_4K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->free_buffer_waiters_def_4K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->free_buffer_waiters_def_4K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->fixed_buffer_count_def_4K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->fixed_buffer_count_def_4K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->modified_buffer_count_def_4K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->modified_buffer_count_def_4K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->free_group_buffer_count_def_4K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->free_group_buffer_count_def_4K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->fixed_group_buffer_count_def_4K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->fixed_group_buffer_count_def_4K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->modified_group_buffer_count_def_4K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->modified_group_buffer_count_def_4K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->flimit_def_4K.CounterNameTitleIndex = IncrementVar(dwCounter, 2);
    PfDataDef->flimit_def_4K.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->mlimit_def_4K.CounterNameTitleIndex = IncrementVar(dwCounter, 2);
    PfDataDef->mlimit_def_4K.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->wbstart_def_4K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->wbstart_def_4K.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->wbend_def_4K.CounterNameTitleIndex = IncrementVar(dwCounter, 2);
    PfDataDef->wbend_def_4K.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);

    PfDataDef->fix_count_def_8K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->fix_count_def_8K.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->unfix_count_def_8K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->unfix_count_def_8K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->read_count_def_8K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->read_count_def_8K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->write_count_def_8K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->write_count_def_8K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->group_read_count_def_8K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->group_read_count_def_8K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->group_write_count_def_8K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->group_write_count_def_8K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->dirty_unfix_count_def_8K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->dirty_unfix_count_def_8K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->force_count_def_8K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->force_count_def_8K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->io_wait_count_def_8K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->io_wait_count_def_8K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->hit_count_def_8K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->hit_count_def_8K.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->free_buffer_count_def_8K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->free_buffer_count_def_8K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->free_buffer_waiters_def_8K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->free_buffer_waiters_def_8K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->fixed_buffer_count_def_8K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->fixed_buffer_count_def_8K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->modified_buffer_count_def_8K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->modified_buffer_count_def_8K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->free_group_buffer_count_def_8K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->free_group_buffer_count_def_8K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->fixed_group_buffer_count_def_8K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->fixed_group_buffer_count_def_8K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->modified_group_buffer_count_def_8K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->modified_group_buffer_count_def_8K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->flimit_def_8K.CounterNameTitleIndex = IncrementVar(dwCounter, 2);
    PfDataDef->flimit_def_8K.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->mlimit_def_8K.CounterNameTitleIndex = IncrementVar(dwCounter, 2);
    PfDataDef->mlimit_def_8K.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->wbstart_def_8K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->wbstart_def_8K.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->wbend_def_8K.CounterNameTitleIndex = IncrementVar(dwCounter, 2);
    PfDataDef->wbend_def_8K.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);

    PfDataDef->fix_count_def_16K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->fix_count_def_16K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->unfix_count_def_16K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->unfix_count_def_16K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->read_count_def_16K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->read_count_def_16K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->write_count_def_16K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->write_count_def_16K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->group_read_count_def_16K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->group_read_count_def_16K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->group_write_count_def_16K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->group_write_count_def_16K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->dirty_unfix_count_def_16K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->dirty_unfix_count_def_16K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->force_count_def_16K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->force_count_def_16K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->io_wait_count_def_16K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->io_wait_count_def_16K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->hit_count_def_16K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->hit_count_def_16K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->free_buffer_count_def_16K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->free_buffer_count_def_16K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->free_buffer_waiters_def_16K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->free_buffer_waiters_def_16K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->fixed_buffer_count_def_16K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->fixed_buffer_count_def_16K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->modified_buffer_count_def_16K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->modified_buffer_count_def_16K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->free_group_buffer_count_def_16K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->free_group_buffer_count_def_16K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->fixed_group_buffer_count_def_16K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->fixed_group_buffer_count_def_16K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->modified_group_buffer_count_def_16K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->modified_group_buffer_count_def_16K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->flimit_def_16K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->flimit_def_16K.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->mlimit_def_16K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->mlimit_def_16K.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->wbstart_def_16K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->wbstart_def_16K.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->wbend_def_16K.CounterNameTitleIndex = IncrementVar(dwCounter, 2);
    PfDataDef->wbend_def_16K.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);

    PfDataDef->fix_count_def_32K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->fix_count_def_32K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->unfix_count_def_32K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->unfix_count_def_32K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->read_count_def_32K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->read_count_def_32K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->write_count_def_32K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->write_count_def_32K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->group_read_count_def_32K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->group_read_count_def_32K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->group_write_count_def_32K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->group_write_count_def_32K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->dirty_unfix_count_def_32K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->dirty_unfix_count_def_32K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->force_count_def_32K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->force_count_def_32K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->io_wait_count_def_32K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->io_wait_count_def_32K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->hit_count_def_32K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->hit_count_def_32K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->free_buffer_count_def_32K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->free_buffer_count_def_32K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->free_buffer_waiters_def_32K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->free_buffer_waiters_def_32K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->fixed_buffer_count_def_32K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->fixed_buffer_count_def_32K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->modified_buffer_count_def_32K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->modified_buffer_count_def_32K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->free_group_buffer_count_def_32K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->free_group_buffer_count_def_32K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->fixed_group_buffer_count_def_32K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->fixed_group_buffer_count_def_32K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->modified_group_buffer_count_def_32K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->modified_group_buffer_count_def_32K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->flimit_def_32K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->flimit_def_32K.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->mlimit_def_32K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->mlimit_def_32K.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->wbstart_def_32K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->wbstart_def_32K.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->wbend_def_32K.CounterNameTitleIndex = IncrementVar(dwCounter, 2);
    PfDataDef->wbend_def_32K.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);

    PfDataDef->fix_count_def_64K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->fix_count_def_64K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->unfix_count_def_64K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->unfix_count_def_64K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->read_count_def_64K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->read_count_def_64K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->write_count_def_64K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->write_count_def_64K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->group_read_count_def_64K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->group_read_count_def_64K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->group_write_count_def_64K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->group_write_count_def_64K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->dirty_unfix_count_def_64K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->dirty_unfix_count_def_64K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->force_count_def_64K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->force_count_def_64K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->io_wait_count_def_64K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->io_wait_count_def_64K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->hit_count_def_64K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->hit_count_def_64K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->free_buffer_count_def_64K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->free_buffer_count_def_64K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->free_buffer_waiters_def_64K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->free_buffer_waiters_def_64K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->fixed_buffer_count_def_64K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->fixed_buffer_count_def_64K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->modified_buffer_count_def_64K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->modified_buffer_count_def_64K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->free_group_buffer_count_def_64K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->free_group_buffer_count_def_64K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->fixed_group_buffer_count_def_64K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->fixed_group_buffer_count_def_64K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->modified_group_buffer_count_def_64K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->modified_group_buffer_count_def_64K.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->flimit_def_64K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->flimit_def_64K.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->mlimit_def_64K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->mlimit_def_64K.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->wbstart_def_64K.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->wbstart_def_64K.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->wbend_def_64K.CounterNameTitleIndex = IncrementVar(dwCounter, 2);
    PfDataDef->wbend_def_64K.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);

    /* Set the lock counters */
    PfDataDef->lockwaits_def.CounterNameTitleIndex = IncrementVar(dwCounter, 2);
    PfDataDef->lockwaits_def.CounterHelpTitleIndex = IncrementVar(dwHelp, 2);
    PfDataDef->lockresources_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->lockresources_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);

    /* Set the thread counters */
    PfDataDef->numthreadsamples_def_csint.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numthreadsamples_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateFREE_def_csint.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateFREE_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateCOMPUTABLE_def_csint.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateCOMPUTABLE_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateEVENT_WAIT_def_csint.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateEVENT_WAIT_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateMUTEX_def_csint.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateMUTEX_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityCLF_def_csint.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityCLF_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityADF_def_csint.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityADF_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDMF_def_csint.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDMF_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityOPF_def_csint.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityOPF_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityPSF_def_csint.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityPSF_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQEF_def_csint.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQEF_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQSF_def_csint.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQSF_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRDF_def_csint.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRDF_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySCF_def_csint.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySCF_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityULF_def_csint.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityULF_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDUF_def_csint.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDUF_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGCF_def_csint.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGCF_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRQF_def_csint.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRQF_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityTPF_def_csint.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityTPF_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGWF_def_csint.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGWF_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySXF_def_csint.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySXF_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventDISKIO_def_csint.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventDISKIO_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGIO_def_csint.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGIO_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventMESSAGEIO_def_csint.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventMESSAGEIO_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOG_def_csint.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOG_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCK_def_csint.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCK_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGEVENT_def_csint.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGEVENT_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCKEVENT_def_csint.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCKEVENT_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->numthreadsamples_def_normal.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numthreadsamples_def_normal.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateFREE_def_normal.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateFREE_def_normal.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateCOMPUTABLE_def_normal.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateCOMPUTABLE_def_normal.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateEVENT_WAIT_def_normal.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateEVENT_WAIT_def_normal.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateMUTEX_def_normal.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateMUTEX_def_normal.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityCLF_def_normal.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityCLF_def_normal.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityADF_def_normal.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityADF_def_normal.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDMF_def_normal.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDMF_def_normal.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityOPF_def_normal.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityOPF_def_normal.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityPSF_def_normal.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityPSF_def_normal.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQEF_def_normal.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQEF_def_normal.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQSF_def_normal.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQSF_def_normal.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRDF_def_normal.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRDF_def_normal.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySCF_def_normal.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySCF_def_normal.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityULF_def_normal.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityULF_def_normal.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDUF_def_normal.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDUF_def_normal.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGCF_def_normal.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGCF_def_normal.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRQF_def_normal.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRQF_def_normal.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityTPF_def_normal.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityTPF_def_normal.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGWF_def_normal.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGWF_def_normal.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySXF_def_normal.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySXF_def_normal.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventDISKIO_def_normal.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventDISKIO_def_normal.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGIO_def_normal.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGIO_def_normal.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventMESSAGEIO_def_normal.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventMESSAGEIO_def_normal.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOG_def_normal.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOG_def_normal.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCK_def_normal.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCK_def_normal.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGEVENT_def_normal.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGEVENT_def_normal.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCKEVENT_def_normal.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCKEVENT_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->numthreadsamples_def_mon.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numthreadsamples_def_mon.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateFREE_def_mon.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateFREE_def_mon.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateCOMPUTABLE_def_mon.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateCOMPUTABLE_def_mon.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateEVENT_WAIT_def_mon.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateEVENT_WAIT_def_mon.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateMUTEX_def_mon.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateMUTEX_def_mon.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityCLF_def_mon.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityCLF_def_mon.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityADF_def_mon.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityADF_def_mon.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDMF_def_mon.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDMF_def_mon.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityOPF_def_mon.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityOPF_def_mon.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityPSF_def_mon.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityPSF_def_mon.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQEF_def_mon.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQEF_def_mon.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQSF_def_mon.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQSF_def_mon.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRDF_def_mon.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRDF_def_mon.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySCF_def_mon.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySCF_def_mon.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityULF_def_mon.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityULF_def_mon.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDUF_def_mon.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDUF_def_mon.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGCF_def_mon.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGCF_def_mon.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRQF_def_mon.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRQF_def_mon.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityTPF_def_mon.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityTPF_def_mon.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGWF_def_mon.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGWF_def_mon.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySXF_def_mon.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySXF_def_mon.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventDISKIO_def_mon.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventDISKIO_def_mon.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGIO_def_mon.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGIO_def_mon.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventMESSAGEIO_def_mon.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventMESSAGEIO_def_mon.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOG_def_mon.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOG_def_mon.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCK_def_mon.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCK_def_mon.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGEVENT_def_mon.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGEVENT_def_mon.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCKEVENT_def_mon.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCKEVENT_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->numthreadsamples_def_fc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numthreadsamples_def_fc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateFREE_def_fc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateFREE_def_fc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateCOMPUTABLE_def_fc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateCOMPUTABLE_def_fc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateEVENT_WAIT_def_fc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateEVENT_WAIT_def_fc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateMUTEX_def_fc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateMUTEX_def_fc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityCLF_def_fc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityCLF_def_fc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityADF_def_fc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityADF_def_fc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDMF_def_fc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDMF_def_fc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityOPF_def_fc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityOPF_def_fc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityPSF_def_fc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityPSF_def_fc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQEF_def_fc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQEF_def_fc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQSF_def_fc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQSF_def_fc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRDF_def_fc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRDF_def_fc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySCF_def_fc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySCF_def_fc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityULF_def_fc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityULF_def_fc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDUF_def_fc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDUF_def_fc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGCF_def_fc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGCF_def_fc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRQF_def_fc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRQF_def_fc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityTPF_def_fc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityTPF_def_fc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGWF_def_fc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGWF_def_fc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySXF_def_fc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySXF_def_fc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventDISKIO_def_fc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventDISKIO_def_fc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGIO_def_fc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGIO_def_fc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventMESSAGEIO_def_fc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventMESSAGEIO_def_fc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOG_def_fc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOG_def_fc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCK_def_fc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCK_def_fc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGEVENT_def_fc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGEVENT_def_fc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCKEVENT_def_fc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCKEVENT_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->numthreadsamples_def_wb.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numthreadsamples_def_wb.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateFREE_def_wb.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateFREE_def_wb.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateCOMPUTABLE_def_wb.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateCOMPUTABLE_def_wb.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateEVENT_WAIT_def_wb.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateEVENT_WAIT_def_wb.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateMUTEX_def_wb.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateMUTEX_def_wb.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityCLF_def_wb.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityCLF_def_wb.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityADF_def_wb.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityADF_def_wb.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDMF_def_wb.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDMF_def_wb.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityOPF_def_wb.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityOPF_def_wb.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityPSF_def_wb.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityPSF_def_wb.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQEF_def_wb.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQEF_def_wb.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQSF_def_wb.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQSF_def_wb.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRDF_def_wb.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRDF_def_wb.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySCF_def_wb.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySCF_def_wb.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityULF_def_wb.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityULF_def_wb.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDUF_def_wb.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDUF_def_wb.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGCF_def_wb.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGCF_def_wb.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRQF_def_wb.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRQF_def_wb.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityTPF_def_wb.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityTPF_def_wb.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGWF_def_wb.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGWF_def_wb.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySXF_def_wb.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySXF_def_wb.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventDISKIO_def_wb.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventDISKIO_def_wb.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGIO_def_wb.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGIO_def_wb.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventMESSAGEIO_def_wb.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventMESSAGEIO_def_wb.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOG_def_wb.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOG_def_wb.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCK_def_wb.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCK_def_wb.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGEVENT_def_wb.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGEVENT_def_wb.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCKEVENT_def_wb.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCKEVENT_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->numthreadsamples_def_si.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numthreadsamples_def_si.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateFREE_def_si.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateFREE_def_si.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateCOMPUTABLE_def_si.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateCOMPUTABLE_def_si.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateEVENT_WAIT_def_si.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateEVENT_WAIT_def_si.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateMUTEX_def_si.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateMUTEX_def_si.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityCLF_def_si.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityCLF_def_si.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityADF_def_si.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityADF_def_si.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDMF_def_si.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDMF_def_si.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityOPF_def_si.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityOPF_def_si.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityPSF_def_si.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityPSF_def_si.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQEF_def_si.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQEF_def_si.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQSF_def_si.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQSF_def_si.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRDF_def_si.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRDF_def_si.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySCF_def_si.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySCF_def_si.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityULF_def_si.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityULF_def_si.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDUF_def_si.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDUF_def_si.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGCF_def_si.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGCF_def_si.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRQF_def_si.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRQF_def_si.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityTPF_def_si.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityTPF_def_si.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGWF_def_si.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGWF_def_si.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySXF_def_si.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySXF_def_si.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventDISKIO_def_si.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventDISKIO_def_si.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGIO_def_si.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGIO_def_si.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventMESSAGEIO_def_si.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventMESSAGEIO_def_si.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOG_def_si.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOG_def_si.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCK_def_si.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCK_def_si.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGEVENT_def_si.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGEVENT_def_si.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCKEVENT_def_si.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCKEVENT_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->numthreadsamples_def_event.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numthreadsamples_def_event.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateFREE_def_event.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateFREE_def_event.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateCOMPUTABLE_def_event.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateCOMPUTABLE_def_event.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateEVENT_WAIT_def_event.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateEVENT_WAIT_def_event.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateMUTEX_def_event.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateMUTEX_def_event.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityCLF_def_event.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityCLF_def_event.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityADF_def_event.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityADF_def_event.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDMF_def_event.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDMF_def_event.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityOPF_def_event.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityOPF_def_event.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityPSF_def_event.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityPSF_def_event.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQEF_def_event.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQEF_def_event.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQSF_def_event.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQSF_def_event.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRDF_def_event.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRDF_def_event.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySCF_def_event.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySCF_def_event.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityULF_def_event.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityULF_def_event.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDUF_def_event.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDUF_def_event.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGCF_def_event.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGCF_def_event.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRQF_def_event.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRQF_def_event.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityTPF_def_event.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityTPF_def_event.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGWF_def_event.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGWF_def_event.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySXF_def_event.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySXF_def_event.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventDISKIO_def_event.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventDISKIO_def_event.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGIO_def_event.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGIO_def_event.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventMESSAGEIO_def_event.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventMESSAGEIO_def_event.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOG_def_event.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOG_def_event.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCK_def_event.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCK_def_event.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGEVENT_def_event.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGEVENT_def_event.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCKEVENT_def_event.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCKEVENT_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->numthreadsamples_def_2pc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numthreadsamples_def_2pc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateFREE_def_2pc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateFREE_def_2pc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateCOMPUTABLE_def_2pc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateCOMPUTABLE_def_2pc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateEVENT_WAIT_def_2pc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateEVENT_WAIT_def_2pc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateMUTEX_def_2pc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateMUTEX_def_2pc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityCLF_def_2pc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityCLF_def_2pc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityADF_def_2pc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityADF_def_2pc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDMF_def_2pc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDMF_def_2pc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityOPF_def_2pc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityOPF_def_2pc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityPSF_def_2pc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityPSF_def_2pc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQEF_def_2pc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQEF_def_2pc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQSF_def_2pc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQSF_def_2pc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRDF_def_2pc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRDF_def_2pc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySCF_def_2pc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySCF_def_2pc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityULF_def_2pc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityULF_def_2pc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDUF_def_2pc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDUF_def_2pc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGCF_def_2pc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGCF_def_2pc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRQF_def_2pc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRQF_def_2pc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityTPF_def_2pc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityTPF_def_2pc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGWF_def_2pc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGWF_def_2pc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySXF_def_2pc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySXF_def_2pc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventDISKIO_def_2pc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventDISKIO_def_2pc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGIO_def_2pc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGIO_def_2pc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventMESSAGEIO_def_2pc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventMESSAGEIO_def_2pc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOG_def_2pc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOG_def_2pc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCK_def_2pc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCK_def_2pc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGEVENT_def_2pc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGEVENT_def_2pc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCKEVENT_def_2pc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCKEVENT_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->numthreadsamples_def_recovery.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numthreadsamples_def_recovery.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateFREE_def_recovery.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateFREE_def_recovery.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateCOMPUTABLE_def_recovery.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateCOMPUTABLE_def_recovery.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateEVENT_WAIT_def_recovery.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateEVENT_WAIT_def_recovery.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateMUTEX_def_recovery.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateMUTEX_def_recovery.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityCLF_def_recovery.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityCLF_def_recovery.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityADF_def_recovery.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityADF_def_recovery.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDMF_def_recovery.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDMF_def_recovery.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityOPF_def_recovery.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityOPF_def_recovery.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityPSF_def_recovery.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityPSF_def_recovery.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQEF_def_recovery.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQEF_def_recovery.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQSF_def_recovery.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQSF_def_recovery.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRDF_def_recovery.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRDF_def_recovery.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySCF_def_recovery.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySCF_def_recovery.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityULF_def_recovery.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityULF_def_recovery.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDUF_def_recovery.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDUF_def_recovery.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGCF_def_recovery.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGCF_def_recovery.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRQF_def_recovery.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRQF_def_recovery.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityTPF_def_recovery.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityTPF_def_recovery.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGWF_def_recovery.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGWF_def_recovery.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySXF_def_recovery.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySXF_def_recovery.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventDISKIO_def_recovery.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventDISKIO_def_recovery.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGIO_def_recovery.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGIO_def_recovery.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventMESSAGEIO_def_recovery.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventMESSAGEIO_def_recovery.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOG_def_recovery.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOG_def_recovery.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCK_def_recovery.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCK_def_recovery.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGEVENT_def_recovery.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGEVENT_def_recovery.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCKEVENT_def_recovery.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCKEVENT_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->numthreadsamples_def_lgw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numthreadsamples_def_lgw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateFREE_def_lgw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateFREE_def_lgw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateCOMPUTABLE_def_lgw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateCOMPUTABLE_def_lgw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateEVENT_WAIT_def_lgw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateEVENT_WAIT_def_lgw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateMUTEX_def_lgw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateMUTEX_def_lgw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityCLF_def_lgw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityCLF_def_lgw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityADF_def_lgw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityADF_def_lgw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDMF_def_lgw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDMF_def_lgw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityOPF_def_lgw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityOPF_def_lgw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityPSF_def_lgw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityPSF_def_lgw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQEF_def_lgw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQEF_def_lgw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQSF_def_lgw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQSF_def_lgw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRDF_def_lgw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRDF_def_lgw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySCF_def_lgw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySCF_def_lgw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityULF_def_lgw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityULF_def_lgw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDUF_def_lgw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDUF_def_lgw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGCF_def_lgw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGCF_def_lgw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRQF_def_lgw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRQF_def_lgw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityTPF_def_lgw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityTPF_def_lgw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGWF_def_lgw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGWF_def_lgw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySXF_def_lgw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySXF_def_lgw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventDISKIO_def_lgw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventDISKIO_def_lgw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGIO_def_lgw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGIO_def_lgw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventMESSAGEIO_def_lgw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventMESSAGEIO_def_lgw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOG_def_lgw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOG_def_lgw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCK_def_lgw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCK_def_lgw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGEVENT_def_lgw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGEVENT_def_lgw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCKEVENT_def_lgw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCKEVENT_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->numthreadsamples_def_cd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numthreadsamples_def_cd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateFREE_def_cd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateFREE_def_cd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateCOMPUTABLE_def_cd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateCOMPUTABLE_def_cd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateEVENT_WAIT_def_cd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateEVENT_WAIT_def_cd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateMUTEX_def_cd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateMUTEX_def_cd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityCLF_def_cd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityCLF_def_cd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityADF_def_cd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityADF_def_cd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDMF_def_cd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDMF_def_cd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityOPF_def_cd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityOPF_def_cd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityPSF_def_cd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityPSF_def_cd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQEF_def_cd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQEF_def_cd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQSF_def_cd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQSF_def_cd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRDF_def_cd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRDF_def_cd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySCF_def_cd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySCF_def_cd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityULF_def_cd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityULF_def_cd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDUF_def_cd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDUF_def_cd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGCF_def_cd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGCF_def_cd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRQF_def_cd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRQF_def_cd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityTPF_def_cd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityTPF_def_cd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGWF_def_cd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGWF_def_cd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySXF_def_cd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySXF_def_cd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventDISKIO_def_cd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventDISKIO_def_cd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGIO_def_cd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGIO_def_cd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventMESSAGEIO_def_cd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventMESSAGEIO_def_cd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOG_def_cd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOG_def_cd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCK_def_cd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCK_def_cd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGEVENT_def_cd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGEVENT_def_cd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCKEVENT_def_cd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCKEVENT_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->numthreadsamples_def_gc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numthreadsamples_def_gc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateFREE_def_gc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateFREE_def_gc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateCOMPUTABLE_def_gc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateCOMPUTABLE_def_gc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateEVENT_WAIT_def_gc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateEVENT_WAIT_def_gc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateMUTEX_def_gc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateMUTEX_def_gc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityCLF_def_gc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityCLF_def_gc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityADF_def_gc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityADF_def_gc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDMF_def_gc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDMF_def_gc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityOPF_def_gc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityOPF_def_gc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityPSF_def_gc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityPSF_def_gc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQEF_def_gc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQEF_def_gc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQSF_def_gc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQSF_def_gc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRDF_def_gc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRDF_def_gc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySCF_def_gc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySCF_def_gc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityULF_def_gc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityULF_def_gc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDUF_def_gc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDUF_def_gc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGCF_def_gc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGCF_def_gc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRQF_def_gc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRQF_def_gc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityTPF_def_gc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityTPF_def_gc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGWF_def_gc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGWF_def_gc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySXF_def_gc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySXF_def_gc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventDISKIO_def_gc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventDISKIO_def_gc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGIO_def_gc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGIO_def_gc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventMESSAGEIO_def_gc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventMESSAGEIO_def_gc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOG_def_gc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOG_def_gc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCK_def_gc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCK_def_gc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGEVENT_def_gc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGEVENT_def_gc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCKEVENT_def_gc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCKEVENT_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->numthreadsamples_def_fa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numthreadsamples_def_fa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateFREE_def_fa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateFREE_def_fa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateCOMPUTABLE_def_fa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateCOMPUTABLE_def_fa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateEVENT_WAIT_def_fa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateEVENT_WAIT_def_fa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateMUTEX_def_fa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateMUTEX_def_fa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityCLF_def_fa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityCLF_def_fa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityADF_def_fa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityADF_def_fa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDMF_def_fa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDMF_def_fa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityOPF_def_fa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityOPF_def_fa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityPSF_def_fa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityPSF_def_fa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQEF_def_fa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQEF_def_fa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQSF_def_fa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQSF_def_fa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRDF_def_fa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRDF_def_fa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySCF_def_fa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySCF_def_fa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityULF_def_fa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityULF_def_fa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDUF_def_fa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDUF_def_fa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGCF_def_fa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGCF_def_fa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRQF_def_fa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRQF_def_fa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityTPF_def_fa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityTPF_def_fa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGWF_def_fa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGWF_def_fa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySXF_def_fa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySXF_def_fa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventDISKIO_def_fa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventDISKIO_def_fa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGIO_def_fa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGIO_def_fa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventMESSAGEIO_def_fa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventMESSAGEIO_def_fa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOG_def_fa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOG_def_fa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCK_def_fa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCK_def_fa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGEVENT_def_fa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGEVENT_def_fa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCKEVENT_def_fa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCKEVENT_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->numthreadsamples_def_audit.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numthreadsamples_def_audit.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateFREE_def_audit.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateFREE_def_audit.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateCOMPUTABLE_def_audit.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateCOMPUTABLE_def_audit.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateEVENT_WAIT_def_audit.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateEVENT_WAIT_def_audit.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateMUTEX_def_audit.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateMUTEX_def_audit.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityCLF_def_audit.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityCLF_def_audit.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityADF_def_audit.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityADF_def_audit.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDMF_def_audit.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDMF_def_audit.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityOPF_def_audit.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityOPF_def_audit.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityPSF_def_audit.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityPSF_def_audit.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQEF_def_audit.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQEF_def_audit.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQSF_def_audit.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQSF_def_audit.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRDF_def_audit.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRDF_def_audit.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySCF_def_audit.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySCF_def_audit.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityULF_def_audit.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityULF_def_audit.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDUF_def_audit.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDUF_def_audit.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGCF_def_audit.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGCF_def_audit.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRQF_def_audit.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRQF_def_audit.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityTPF_def_audit.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityTPF_def_audit.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGWF_def_audit.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGWF_def_audit.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySXF_def_audit.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySXF_def_audit.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventDISKIO_def_audit.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventDISKIO_def_audit.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGIO_def_audit.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGIO_def_audit.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventMESSAGEIO_def_audit.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventMESSAGEIO_def_audit.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOG_def_audit.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOG_def_audit.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCK_def_audit.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCK_def_audit.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGEVENT_def_audit.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGEVENT_def_audit.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCKEVENT_def_audit.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCKEVENT_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->numthreadsamples_def_cptimer.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numthreadsamples_def_cptimer.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateFREE_def_cptimer.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateFREE_def_cptimer.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateCOMPUTABLE_def_cptimer.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateCOMPUTABLE_def_cptimer.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateEVENT_WAIT_def_cptimer.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateEVENT_WAIT_def_cptimer.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateMUTEX_def_cptimer.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateMUTEX_def_cptimer.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityCLF_def_cptimer.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityCLF_def_cptimer.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityADF_def_cptimer.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityADF_def_cptimer.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDMF_def_cptimer.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDMF_def_cptimer.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityOPF_def_cptimer.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityOPF_def_cptimer.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityPSF_def_cptimer.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityPSF_def_cptimer.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQEF_def_cptimer.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQEF_def_cptimer.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQSF_def_cptimer.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQSF_def_cptimer.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRDF_def_cptimer.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRDF_def_cptimer.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySCF_def_cptimer.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySCF_def_cptimer.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityULF_def_cptimer.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityULF_def_cptimer.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDUF_def_cptimer.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDUF_def_cptimer.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGCF_def_cptimer.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGCF_def_cptimer.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRQF_def_cptimer.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRQF_def_cptimer.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityTPF_def_cptimer.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityTPF_def_cptimer.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGWF_def_cptimer.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGWF_def_cptimer.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySXF_def_cptimer.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySXF_def_cptimer.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventDISKIO_def_cptimer.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventDISKIO_def_cptimer.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGIO_def_cptimer.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGIO_def_cptimer.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventMESSAGEIO_def_cptimer.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventMESSAGEIO_def_cptimer.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOG_def_cptimer.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOG_def_cptimer.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCK_def_cptimer.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCK_def_cptimer.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGEVENT_def_cptimer.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGEVENT_def_cptimer.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCKEVENT_def_cptimer.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCKEVENT_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->numthreadsamples_def_ckterm.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numthreadsamples_def_ckterm.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateFREE_def_ckterm.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateFREE_def_ckterm.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateCOMPUTABLE_def_ckterm.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateCOMPUTABLE_def_ckterm.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateEVENT_WAIT_def_ckterm.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateEVENT_WAIT_def_ckterm.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateMUTEX_def_ckterm.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateMUTEX_def_ckterm.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityCLF_def_ckterm.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityCLF_def_ckterm.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityADF_def_ckterm.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityADF_def_ckterm.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDMF_def_ckterm.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDMF_def_ckterm.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityOPF_def_ckterm.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityOPF_def_ckterm.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityPSF_def_ckterm.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityPSF_def_ckterm.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQEF_def_ckterm.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQEF_def_ckterm.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQSF_def_ckterm.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQSF_def_ckterm.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRDF_def_ckterm.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRDF_def_ckterm.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySCF_def_ckterm.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySCF_def_ckterm.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityULF_def_ckterm.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityULF_def_ckterm.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDUF_def_ckterm.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDUF_def_ckterm.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGCF_def_ckterm.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGCF_def_ckterm.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRQF_def_ckterm.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRQF_def_ckterm.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityTPF_def_ckterm.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityTPF_def_ckterm.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGWF_def_ckterm.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGWF_def_ckterm.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySXF_def_ckterm.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySXF_def_ckterm.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventDISKIO_def_ckterm.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventDISKIO_def_ckterm.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGIO_def_ckterm.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGIO_def_ckterm.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventMESSAGEIO_def_ckterm.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventMESSAGEIO_def_ckterm.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOG_def_ckterm.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOG_def_ckterm.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCK_def_ckterm.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCK_def_ckterm.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGEVENT_def_ckterm.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGEVENT_def_ckterm.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCKEVENT_def_ckterm.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCKEVENT_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->numthreadsamples_def_saw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numthreadsamples_def_saw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateFREE_def_saw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateFREE_def_saw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateCOMPUTABLE_def_saw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateCOMPUTABLE_def_saw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateEVENT_WAIT_def_saw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateEVENT_WAIT_def_saw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateMUTEX_def_saw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateMUTEX_def_saw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityCLF_def_saw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityCLF_def_saw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityADF_def_saw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityADF_def_saw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDMF_def_saw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDMF_def_saw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityOPF_def_saw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityOPF_def_saw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityPSF_def_saw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityPSF_def_saw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQEF_def_saw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQEF_def_saw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQSF_def_saw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQSF_def_saw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRDF_def_saw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRDF_def_saw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySCF_def_saw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySCF_def_saw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityULF_def_saw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityULF_def_saw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDUF_def_saw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDUF_def_saw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGCF_def_saw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGCF_def_saw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRQF_def_saw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRQF_def_saw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityTPF_def_saw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityTPF_def_saw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGWF_def_saw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGWF_def_saw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySXF_def_saw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySXF_def_saw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventDISKIO_def_saw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventDISKIO_def_saw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGIO_def_saw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGIO_def_saw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventMESSAGEIO_def_saw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventMESSAGEIO_def_saw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOG_def_saw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOG_def_saw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCK_def_saw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCK_def_saw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGEVENT_def_saw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGEVENT_def_saw.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCKEVENT_def_saw.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCKEVENT_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->numthreadsamples_def_wa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numthreadsamples_def_wa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateFREE_def_wa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateFREE_def_wa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateCOMPUTABLE_def_wa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateCOMPUTABLE_def_wa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateEVENT_WAIT_def_wa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateEVENT_WAIT_def_wa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateMUTEX_def_wa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateMUTEX_def_wa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityCLF_def_wa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityCLF_def_wa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityADF_def_wa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityADF_def_wa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDMF_def_wa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDMF_def_wa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityOPF_def_wa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityOPF_def_wa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityPSF_def_wa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityPSF_def_wa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQEF_def_wa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQEF_def_wa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQSF_def_wa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQSF_def_wa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRDF_def_wa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRDF_def_wa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySCF_def_wa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySCF_def_wa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityULF_def_wa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityULF_def_wa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDUF_def_wa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDUF_def_wa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGCF_def_wa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGCF_def_wa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRQF_def_wa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRQF_def_wa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityTPF_def_wa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityTPF_def_wa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGWF_def_wa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGWF_def_wa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySXF_def_wa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySXF_def_wa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventDISKIO_def_wa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventDISKIO_def_wa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGIO_def_wa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGIO_def_wa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventMESSAGEIO_def_wa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventMESSAGEIO_def_wa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOG_def_wa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOG_def_wa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCK_def_wa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCK_def_wa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGEVENT_def_wa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGEVENT_def_wa.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCKEVENT_def_wa.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCKEVENT_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->numthreadsamples_def_ra.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numthreadsamples_def_ra.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateFREE_def_ra.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateFREE_def_ra.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateCOMPUTABLE_def_ra.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateCOMPUTABLE_def_ra.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateEVENT_WAIT_def_ra.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateEVENT_WAIT_def_ra.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateMUTEX_def_ra.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateMUTEX_def_ra.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityCLF_def_ra.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityCLF_def_ra.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityADF_def_ra.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityADF_def_ra.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDMF_def_ra.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDMF_def_ra.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityOPF_def_ra.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityOPF_def_ra.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityPSF_def_ra.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityPSF_def_ra.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQEF_def_ra.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQEF_def_ra.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQSF_def_ra.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQSF_def_ra.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRDF_def_ra.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRDF_def_ra.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySCF_def_ra.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySCF_def_ra.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityULF_def_ra.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityULF_def_ra.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDUF_def_ra.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDUF_def_ra.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGCF_def_ra.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGCF_def_ra.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRQF_def_ra.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRQF_def_ra.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityTPF_def_ra.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityTPF_def_ra.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGWF_def_ra.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGWF_def_ra.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySXF_def_ra.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySXF_def_ra.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventDISKIO_def_ra.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventDISKIO_def_ra.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGIO_def_ra.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGIO_def_ra.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventMESSAGEIO_def_ra.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventMESSAGEIO_def_ra.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOG_def_ra.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOG_def_ra.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCK_def_ra.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCK_def_ra.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGEVENT_def_ra.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGEVENT_def_ra.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCKEVENT_def_ra.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCKEVENT_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->numthreadsamples_def_repq.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numthreadsamples_def_repq.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateFREE_def_repq.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateFREE_def_repq.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateCOMPUTABLE_def_repq.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateCOMPUTABLE_def_repq.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateEVENT_WAIT_def_repq.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateEVENT_WAIT_def_repq.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateMUTEX_def_repq.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateMUTEX_def_repq.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityCLF_def_repq.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityCLF_def_repq.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityADF_def_repq.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityADF_def_repq.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDMF_def_repq.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDMF_def_repq.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityOPF_def_repq.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityOPF_def_repq.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityPSF_def_repq.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityPSF_def_repq.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQEF_def_repq.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQEF_def_repq.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQSF_def_repq.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQSF_def_repq.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRDF_def_repq.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRDF_def_repq.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySCF_def_repq.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySCF_def_repq.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityULF_def_repq.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityULF_def_repq.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDUF_def_repq.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDUF_def_repq.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGCF_def_repq.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGCF_def_repq.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRQF_def_repq.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRQF_def_repq.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityTPF_def_repq.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityTPF_def_repq.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGWF_def_repq.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGWF_def_repq.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySXF_def_repq.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySXF_def_repq.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventDISKIO_def_repq.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventDISKIO_def_repq.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGIO_def_repq.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGIO_def_repq.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventMESSAGEIO_def_repq.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventMESSAGEIO_def_repq.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOG_def_repq.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOG_def_repq.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCK_def_repq.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCK_def_repq.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGEVENT_def_repq.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGEVENT_def_repq.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCKEVENT_def_repq.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCKEVENT_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->numthreadsamples_def_lkc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numthreadsamples_def_lkc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateFREE_def_lkc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateFREE_def_lkc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateCOMPUTABLE_def_lkc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateCOMPUTABLE_def_lkc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateEVENT_WAIT_def_lkc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateEVENT_WAIT_def_lkc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateMUTEX_def_lkc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateMUTEX_def_lkc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityCLF_def_lkc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityCLF_def_lkc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityADF_def_lkc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityADF_def_lkc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDMF_def_lkc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDMF_def_lkc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityOPF_def_lkc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityOPF_def_lkc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityPSF_def_lkc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityPSF_def_lkc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQEF_def_lkc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQEF_def_lkc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQSF_def_lkc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQSF_def_lkc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRDF_def_lkc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRDF_def_lkc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySCF_def_lkc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySCF_def_lkc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityULF_def_lkc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityULF_def_lkc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDUF_def_lkc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDUF_def_lkc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGCF_def_lkc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGCF_def_lkc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRQF_def_lkc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRQF_def_lkc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityTPF_def_lkc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityTPF_def_lkc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGWF_def_lkc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGWF_def_lkc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySXF_def_lkc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySXF_def_lkc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventDISKIO_def_lkc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventDISKIO_def_lkc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGIO_def_lkc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGIO_def_lkc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventMESSAGEIO_def_lkc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventMESSAGEIO_def_lkc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOG_def_lkc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOG_def_lkc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCK_def_lkc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCK_def_lkc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGEVENT_def_lkc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGEVENT_def_lkc.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCKEVENT_def_lkc.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCKEVENT_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->numthreadsamples_def_lkd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numthreadsamples_def_lkd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateFREE_def_lkd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateFREE_def_lkd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateCOMPUTABLE_def_lkd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateCOMPUTABLE_def_lkd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateEVENT_WAIT_def_lkd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateEVENT_WAIT_def_lkd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateMUTEX_def_lkd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateMUTEX_def_lkd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityCLF_def_lkd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityCLF_def_lkd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityADF_def_lkd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityADF_def_lkd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDMF_def_lkd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDMF_def_lkd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityOPF_def_lkd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityOPF_def_lkd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityPSF_def_lkd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityPSF_def_lkd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQEF_def_lkd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQEF_def_lkd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQSF_def_lkd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQSF_def_lkd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRDF_def_lkd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRDF_def_lkd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySCF_def_lkd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySCF_def_lkd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityULF_def_lkd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityULF_def_lkd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDUF_def_lkd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDUF_def_lkd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGCF_def_lkd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGCF_def_lkd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRQF_def_lkd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRQF_def_lkd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityTPF_def_lkd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityTPF_def_lkd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGWF_def_lkd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGWF_def_lkd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySXF_def_lkd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySXF_def_lkd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventDISKIO_def_lkd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventDISKIO_def_lkd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGIO_def_lkd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGIO_def_lkd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventMESSAGEIO_def_lkd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventMESSAGEIO_def_lkd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOG_def_lkd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOG_def_lkd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCK_def_lkd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCK_def_lkd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGEVENT_def_lkd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGEVENT_def_lkd.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCKEVENT_def_lkd.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCKEVENT_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->numthreadsamples_def_sampler.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numthreadsamples_def_sampler.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateFREE_def_sampler.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateFREE_def_sampler.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateCOMPUTABLE_def_sampler.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateCOMPUTABLE_def_sampler.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateEVENT_WAIT_def_sampler.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateEVENT_WAIT_def_sampler.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateMUTEX_def_sampler.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateMUTEX_def_sampler.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityCLF_def_sampler.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityCLF_def_sampler.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityADF_def_sampler.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityADF_def_sampler.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDMF_def_sampler.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDMF_def_sampler.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityOPF_def_sampler.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityOPF_def_sampler.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityPSF_def_sampler.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityPSF_def_sampler.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQEF_def_sampler.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQEF_def_sampler.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQSF_def_sampler.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQSF_def_sampler.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRDF_def_sampler.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRDF_def_sampler.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySCF_def_sampler.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySCF_def_sampler.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityULF_def_sampler.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityULF_def_sampler.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDUF_def_sampler.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDUF_def_sampler.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGCF_def_sampler.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGCF_def_sampler.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRQF_def_sampler.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRQF_def_sampler.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityTPF_def_sampler.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityTPF_def_sampler.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGWF_def_sampler.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGWF_def_sampler.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySXF_def_sampler.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySXF_def_sampler.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventDISKIO_def_sampler.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventDISKIO_def_sampler.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGIO_def_sampler.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGIO_def_sampler.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventMESSAGEIO_def_sampler.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventMESSAGEIO_def_sampler.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOG_def_sampler.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOG_def_sampler.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCK_def_sampler.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCK_def_sampler.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGEVENT_def_sampler.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGEVENT_def_sampler.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCKEVENT_def_sampler.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCKEVENT_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->numthreadsamples_def_sort.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numthreadsamples_def_sort.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateFREE_def_sort.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateFREE_def_sort.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateCOMPUTABLE_def_sort.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateCOMPUTABLE_def_sort.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateEVENT_WAIT_def_sort.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateEVENT_WAIT_def_sort.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateMUTEX_def_sort.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateMUTEX_def_sort.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityCLF_def_sort.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityCLF_def_sort.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityADF_def_sort.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityADF_def_sort.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDMF_def_sort.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDMF_def_sort.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityOPF_def_sort.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityOPF_def_sort.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityPSF_def_sort.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityPSF_def_sort.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQEF_def_sort.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQEF_def_sort.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQSF_def_sort.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQSF_def_sort.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRDF_def_sort.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRDF_def_sort.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySCF_def_sort.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySCF_def_sort.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityULF_def_sort.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityULF_def_sort.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDUF_def_sort.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDUF_def_sort.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGCF_def_sort.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGCF_def_sort.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRQF_def_sort.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRQF_def_sort.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityTPF_def_sort.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityTPF_def_sort.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGWF_def_sort.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGWF_def_sort.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySXF_def_sort.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySXF_def_sort.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventDISKIO_def_sort.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventDISKIO_def_sort.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGIO_def_sort.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGIO_def_sort.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventMESSAGEIO_def_sort.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventMESSAGEIO_def_sort.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOG_def_sort.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOG_def_sort.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCK_def_sort.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCK_def_sort.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGEVENT_def_sort.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGEVENT_def_sort.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCKEVENT_def_sort.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCKEVENT_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->numthreadsamples_def_fact.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numthreadsamples_def_fact.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateFREE_def_fact.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateFREE_def_fact.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateCOMPUTABLE_def_fact.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateCOMPUTABLE_def_fact.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateEVENT_WAIT_def_fact.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateEVENT_WAIT_def_fact.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateMUTEX_def_fact.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateMUTEX_def_fact.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityCLF_def_fact.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityCLF_def_fact.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityADF_def_fact.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityADF_def_fact.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDMF_def_fact.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDMF_def_fact.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityOPF_def_fact.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityOPF_def_fact.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityPSF_def_fact.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityPSF_def_fact.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQEF_def_fact.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQEF_def_fact.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQSF_def_fact.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQSF_def_fact.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRDF_def_fact.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRDF_def_fact.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySCF_def_fact.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySCF_def_fact.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityULF_def_fact.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityULF_def_fact.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDUF_def_fact.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDUF_def_fact.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGCF_def_fact.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGCF_def_fact.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRQF_def_fact.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRQF_def_fact.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityTPF_def_fact.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityTPF_def_fact.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGWF_def_fact.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGWF_def_fact.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySXF_def_fact.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySXF_def_fact.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventDISKIO_def_fact.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventDISKIO_def_fact.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGIO_def_fact.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGIO_def_fact.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventMESSAGEIO_def_fact.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventMESSAGEIO_def_fact.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOG_def_fact.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOG_def_fact.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCK_def_fact.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCK_def_fact.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGEVENT_def_fact.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGEVENT_def_fact.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCKEVENT_def_fact.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCKEVENT_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->numthreadsamples_def_lic.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numthreadsamples_def_lic.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateFREE_def_lic.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateFREE_def_lic.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateCOMPUTABLE_def_lic.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateCOMPUTABLE_def_lic.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateEVENT_WAIT_def_lic.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateEVENT_WAIT_def_lic.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateMUTEX_def_lic.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateMUTEX_def_lic.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityCLF_def_lic.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityCLF_def_lic.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityADF_def_lic.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityADF_def_lic.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDMF_def_lic.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDMF_def_lic.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityOPF_def_lic.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityOPF_def_lic.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityPSF_def_lic.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityPSF_def_lic.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQEF_def_lic.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQEF_def_lic.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQSF_def_lic.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQSF_def_lic.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRDF_def_lic.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRDF_def_lic.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySCF_def_lic.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySCF_def_lic.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityULF_def_lic.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityULF_def_lic.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDUF_def_lic.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDUF_def_lic.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGCF_def_lic.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGCF_def_lic.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRQF_def_lic.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRQF_def_lic.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityTPF_def_lic.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityTPF_def_lic.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGWF_def_lic.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGWF_def_lic.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySXF_def_lic.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySXF_def_lic.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventDISKIO_def_lic.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventDISKIO_def_lic.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGIO_def_lic.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGIO_def_lic.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventMESSAGEIO_def_lic.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventMESSAGEIO_def_lic.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOG_def_lic.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOG_def_lic.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCK_def_lic.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCK_def_lic.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGEVENT_def_lic.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGEVENT_def_lic.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCKEVENT_def_lic.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCKEVENT_def_csint.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->numthreadsamples_def_unknown.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numthreadsamples_def_unknown.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateFREE_def_unknown.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateFREE_def_unknown.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateCOMPUTABLE_def_unknown.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateCOMPUTABLE_def_unknown.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateEVENT_WAIT_def_unknown.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateEVENT_WAIT_def_unknown.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->stateMUTEX_def_unknown.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->stateMUTEX_def_unknown.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityCLF_def_unknown.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityCLF_def_unknown.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityADF_def_unknown.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityADF_def_unknown.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDMF_def_unknown.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDMF_def_unknown.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityOPF_def_unknown.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityOPF_def_unknown.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityPSF_def_unknown.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityPSF_def_unknown.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQEF_def_unknown.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQEF_def_unknown.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityQSF_def_unknown.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityQSF_def_unknown.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRDF_def_unknown.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRDF_def_unknown.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySCF_def_unknown.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySCF_def_unknown.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityULF_def_unknown.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityULF_def_unknown.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityDUF_def_unknown.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityDUF_def_unknown.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGCF_def_unknown.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGCF_def_unknown.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityRQF_def_unknown.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityRQF_def_unknown.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityTPF_def_unknown.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityTPF_def_unknown.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilityGWF_def_unknown.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilityGWF_def_unknown.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->facilitySXF_def_unknown.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->facilitySXF_def_unknown.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventDISKIO_def_unknown.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventDISKIO_def_unknown.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGIO_def_unknown.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGIO_def_unknown.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventMESSAGEIO_def_unknown.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventMESSAGEIO_def_unknown.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOG_def_unknown.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOG_def_unknown.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCK_def_unknown.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCK_def_unknown.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOGEVENT_def_unknown.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOGEVENT_def_unknown.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->eventLOCKEVENT_def_unknown.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->eventLOCKEVENT_def_unknown.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
						 
    /* Set the sampler counters */
    PfDataDef->numsamples_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numsamples_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->numtlssamples_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numtlssamples_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->numtlsslots_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numtlsslots_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->numtlsprobes_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numtlsprobes_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->numtlsreads_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numtlsreads_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->numtlsdirty_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numtlsdirty_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);
    PfDataDef->numtlswrites_def.CounterNameTitleIndex =
						IncrementVar(dwCounter, 2);
    PfDataDef->numtlswrites_def.CounterHelpTitleIndex =
						IncrementVar(dwHelp, 2);

    return(0);
}

static int
IncrementVar (int *num, int amount)
{
    *num += amount;
    return (*num);
}

