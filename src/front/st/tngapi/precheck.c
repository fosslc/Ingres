/*
** Copyright (c) 2003, 2005 Ingres Corporation
**
*/
#define _WIN32_WINNT 0x0500
#if defined(UNIX)
#include <unistd.h>
#endif
#include <compat.h>
#include <cm.h>
#include <er.h>
#include <gv.h>
#include <me.h>
#include <nm.h>
#include <st.h>
#include <lo.h>
#include <gvos.h>
#include "tngapi.h"

#if defined(UNIX)
#include <gl.h>
#include <clconfig.h>
#include <systypes.h>
#include <pwd.h>
#include <diracc.h>
#include <handy.h>
#include <erst.h>
#include <sys/utsname.h>
#else
#include <lmcons.h>
#include "tngmsg.h"
#endif  /* UNIX */

/**
** Name: precheck.c - Pre-check installation environment
**
** Description:
**	This module contains functions to validate target environment settings
**  before the actual installation process is started.
**
**  An extended function is provided where selective environment settings can
**  be validated.
**
**  Additional functions are included that convert returned status codes into
**  text messages.
**(E
**  II_GetErrorMessage          - Return message text for function status code
**	II_PrecheckInstallation	    - Pre-check installation environment
**  II_PrecheckInstallationEx   - Extended pre-check installation environment
**  II_GetIngresMessage         - Return message text for Ingres status code
**)E
**
**
** History:
**	16-apr-2002 (abbjo03)
**	   Created.
**      26-Jun-2002 (fanra01)
**          Sir 108122
**          Add hostname and username character checks.
**      07-Sep-2004 (fanra01)
**          SIR 112988
**          Add II_PrecheckInstallationEx function that allows specific
**          tests to be performed.
**      12-Feb-2004 (hweho01)
**          Added supports for Unix platforms.
**      25-Mar-2004 (hweho01)
**          Miscellaneous changes for Unix platforms.
**      07-Dec-2004 (wansh01)
**          Bug 113521
**          Changed nsize to MAX_COMPUTERNAME_LENGTH +1 for GetComputerName()
**          for NT platform to avoid rc=10 error.
**      14-Dec-2004 (fanra01)
**          Sir 113632
**          Add verification of Ingres hardware identifier for target platform.
**      14-Dec-2004 (wansh01) 
**          Bug 113521
**          Changed nsize to _MAX_PATH(256) for gethostname() 
**          for NT platform to avoid rc=11 error.
**      15-Dec-2004 (wansh01)
**          Bug 113521
**          Use GetComputerNameEx() instead of GetComputerName() to get
**          compname in ComputerNameDnsHostname format to test for 
**          matching compname and hostname. 
**      15-Jan-2005 (fanra01)
**          Sir 113776
**          Add an architecture parameter to the II_PrecheckInstallationEx
**          to enable the calling application to specify the target operating
**          platform.
**          Sir 113777
**          Add wrapper function to enable retrieval of Ingres message text
**          for a specified status code.
**      07-Feb-2005 (fanra01)
**          Sir 113881
**          Merge Windows and UNIX sources.
**      14-Feb-2005 (fanra01)
**          Sir 113890
**          Modified testing of path characters to use CM facility.
**          Sir 113891
**          Add II_CHK_USERNAME flag to enable characters in username to be
**          verified.
**      24-Feb-2005 (fanra01)
**          Add comment changes to examples for II_CHK_USERNAME.
**          Bug 113974
**          Modified Windows platform check_os_arch function to get native
**          system info if code is running in a 32-bit execution layer.
**      01-Mar-2005 (hweho01)
**          Set the default II_CHARSET to ISO88591 for Unix platforms.
**      30-Jun-2005 (fanra01)
**          Sir 114806
**          Extend path validation to walk up the directory hierarchy.
**      11-Jul-2005 (fanra01)
**          Sir 114806
**          Add more detailed return code from check_path_chars.
**      18-Aug-2005 (fanra01)
**          Sir 114806
**          Add path length check for character map file.
**      17-Oct-2005 (fanra01)
**          Sir 115396
**          Add processing for instance code validation.
**      12-Dec-2006 (hweho01)
**          Update the tngapi functions on Unix/Linux platforms
**      24-Nov-2009 (frima01) Bug 122490
**          Added include of unistd.h to eliminate gcc 4.3 warnings.
**      02-Dec-2009 (whiro01) Bug 122490
**          Header "unistd.h" doesn't exist on Windows.
*/

#if defined(NT_GENERIC)
# define II_INSTCODE        "installationcode"
# define II_CHARMAP         "charmap"
# define II_CHARSET         "INST1252"
# define PATH_SEPARATOR     "\\"
# define SLASH  '\\'
# define MAX_COMPNAME       MAX_LOC
# define MAX_HOSTNAME       MAX_LOC
#else  /* UNIX  */
# define II_INSTCODE        "II_INSTALLATION"
# define II_CHARMAP         "II_CHARSET"
# define PATH_SEPARATOR     "/"
# define SLASH  '/'
# define MAX_COMPNAME       MAX_COMPUTERNAME_LENGTH
# define MAX_HOSTNAME       MAX_LOC
#endif 

# define CURR_DIR "."     /* benign path character */

/*
** Update error_msgs to use the message table values.
*/
#if defined(UNIX)
static II_UINT4 error_msgs[] =
{
    0, 
    S_ST06A0_invalid_parameter, 
    S_ST06A1_iisystem_not_in_res,  
    S_ST06A2_os_not_min_version,  
    S_ST06A3_bad_path,    
    S_ST06A4_path_not_dir,  
    S_ST06A5_path_connot_write,   
    S_ST06A6_inval_chars_in_path,  
    S_ST06A7_no_char_map, 
    S_ST06A8_charmap_error,  
    S_ST06A9_get_computername_fail,  
    S_ST06AA_get_host_fail,  
    S_ST06AB_unmatched_name, 
    S_ST06AC_invalid_host, 
    S_ST06AD_get_user_fail, 
    S_ST06AE_invalid_user,
    S_ST06B3_path_length_exceeded,
    S_ST06B4_insufficient_buffer,
    S_ST06B5_get_message_error,
    S_ST06BB_invalid_instance,
    S_ST06BC_instance_read
} ; 

#else   /* UNIX */

static HMODULE  hModule = NULL;
static II_UINT4 error_msgs[] =
{
    0,
    E_NULL_PARAM,
    E_NO_INSTALL_PATH,
    E_OS_NOT_MIN_VER,
    E_BAD_PATH,
    E_PATH_NOT_DIR,
    E_PATH_CANNOT_WRITE,
    E_INVAL_CHARS_IN_PATH,
    E_NO_CHAR_MAP,
    E_CHARMAP_ERROR,
    E_GET_COMPUTER_FAIL,
    E_GET_HOST_FAIL,
    E_UNMATCHED_NAME,
    E_INVALID_HOST,
    E_GET_USER_FAIL,
    E_INVALID_USER,
    E_PATH_LEN_EXCEEDED,
    E_INSUFFICIENT_BUFFER,
    E_GET_MESSAGE_ERROR,
    E_INVALID_INSTANCE,
    E_INSTANCE_ERROR
};

#endif  /* UNIX */
/*{
** Name: II_GetErrorMessage
**
** Description:
**      Returns the associated error message for the error status.
**
** Inputs:
**      status      Error status.
**      errmsg      Area to be updated with message text.  The maximum length,
**                  in octets, of a returned message is MAX_IIERR_LEN and
**                  does not include the end-of-string marker.
**                  The minimum size of this area should be
**                  (MAX_IIERR_LEN + 1).
**
** Outputs:
**      errmsg      Error text.  If the message text exceeds MAX_IIERR_LEN
**                  then the message is truncated to MAX_IIERR_LEN.
**
** Returns:
**      None.
**
** Example:
**  # include "tngapi.h"
**
**  II_CHAR errmsg[ MAX_IIERR_LEN + 1] = {'\0'};
**  II_INT  status = II_NULL_PARAM;
**
**  II_GetErrorMessage( status, errmsg );
**  printf( "Error message =%s\n", errmsg );
**
** History:
**      24-Jun-2002 (fanra01)
**          Created.
**
}*/
void
II_GetErrorMessage( II_UINT4 status, char* errmsg )
{
    char errormessage[ MAX_IIERR_LEN + 1 ] = { 0 };
#if defined(NT_GENERIC)
    char* p = errormessage;
    int  nlen = 0;

    if (errmsg)
    {
        if (hModule == NULL)
        {
            hModule =  GetModuleHandle( II_MODULENAME );
        }
        if (hModule != NULL)
        {
            FormatMessage(
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_FROM_HMODULE |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                (LPCVOID)hModule,
                status,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* Default language */
                (LPTSTR) errormessage,
                MAX_IIERR_LEN,
                NULL
            );
            STlcopy( errormessage, errmsg, MAX_IIERR_LEN );
        }
    }

#else   /* NT_GENERIC */
 
    /* Here is for Unix platforms , Send back error message to the caller */

    if( status != OK )
     {
      STprintf( errormessage, ERget(status) );
      SIfprintf( stderr,  errormessage );
      STlcopy( errormessage, errmsg, STlength(errormessage) );
     }

#endif 
    return;
}

/*
** Name: check_host
**
** Description:
**      Test the characters in thehostnames as returned from
**      the operating system.
**
** Inputs:
**      None
**
** Output:
**      error_msg   pointer to the area where an error message should be
**                  copied
**
** Returns:
**      None.
**
** History:
**      24-Jun-2002 (fanra01)
**          Created.
**      07-Dec-2004 (wansh01)
**          Bug 113521
**          Changed nsize to MAX_COMPUTERNAME_LENGTH +1 for GetComputerName()
**          for NT platform to avoid rc=10 error.
**
*/
static i4
check_host()
{
    i4			result;
    char                compname[MAX_COMPNAME + 1] = { 0 };
    char                hostname[MAX_HOSTNAME + 1] = { 0 };
    i4                  nsize = MAX_COMPNAME;
#if defined(UNIX)
    struct   passwd     *p_pswd, pwd;
    char                pwuid_buf[BUFSIZ];
    int                 size = BUFSIZ;
#else
    WSADATA             wsadata;
    WORD                wVersionRequested;
#endif

#if defined(NT_GENERIC)
    /*
    ** Get the computer name as defined for the system.
    */
    nsize = MAX_COMPNAME + 1;
    if (GetComputerNameEx( ComputerNameDnsHostname, compname, &nsize ))
    {
        /*
        ** Check the computer name for any characters that we do not support
        */
        if (!CMvalidhostname( compname ))
        {
            result = II_INVALID_HOST;
            return(result);
        }
    }
    else
    {
        result = II_GET_COMPUTER_FAIL;
        return(result);
    }

    /*
    ** Minimum version of the sockets implementation that we require.
    */
    wVersionRequested = MAKEWORD( 1, 1 );

    /*
    ** Check the version we require with the one that is available
    */
    if (WSAStartup( wVersionRequested, &wsadata ) != 0)
    {
        result = II_GET_HOST_FAIL;
        return(result);
    }
#endif  /* NT_GENERIC */
    
    /*
    ** Get the network hostname
    */
    nsize = MAX_HOSTNAME + 1;
    if (gethostname( hostname, nsize ) == 0)
    {
        /*
        ** Check if the computer is defined as different from the network name
        */
#if defined(NT_GENERIC)
        if (stricmp( compname, hostname ) != 0)
        {
            result = II_UNMATCHED_NAME;
            return(result);
        }
#endif  /* NT_GENERIC */
        /*
        ** Check if the hostname contains unsupported characters
        */
        if (!CMvalidhostname( hostname ))
        {
            result = II_INVALID_HOST;
            return(result);
        }
    }
    else
    {
        result = II_GET_HOST_FAIL;
        return(result);
    }

    result = II_SUCCESSFUL;
    return(result);
}

/*
** Name: check_user
**
** Description:
**      Test the characters in the username as returned from
**      the operating system.
**
** Inputs:
**      None.
**
** Output:
**      error_msg   pointer to the area where an error message should be
**                  copied
**
** Returns:
**      None.
**
** History:
**      14-Feb-2005 (fanra01)
**          Created.
*/
static i4
check_user()
{
    i4			result;
    char                username[UNLEN + 1] = { 0 };
    i4                  nsize = MAX_COMPNAME;
#if defined(UNIX)
    struct   passwd     *p_pswd, pwd;
    char                pwuid_buf[BUFSIZ];
    int                 size = BUFSIZ;
#endif

    /*
    ** Get current user name
    */
    nsize = UNLEN;
#if defined(UNIX)
   MEfill ( sizeof(pwuid_buf), '\0', pwuid_buf );
   if( ( p_pswd = iiCLgetpwuid(getuid(), &pwd, pwuid_buf, size) ) != NULL) 
    {
       STcopy( p_pswd->pw_name, username ); 
#else   /* UNIX */
    if (GetUserName( username, &nsize ))
    {
#endif  /* UNIX */
        /*
        ** Check if the username contains unsupported characters
        */
        if (!CMvalidusername( username ))
        {
            result = II_INVALID_USER;
            return(result);
        }
    }
    else
    {
        result = II_GET_USER_FAIL;
        return(result);
    }

    result = II_SUCCESSFUL;
    return(result);
}

/*
** Name: check_os_arch - Verify OS and architecture
**
** Description:
**  Function verifies the OS version and the operating architecture and
**  determines whether the platform should support an installation of
**  Ingres.
**
**  The following checks are applied:
**      a. Verify the minimum OS version
**      b. Verfiy execution mode.
**      c. Verify architecture.
**
** Inputs:
**  None.
**
** Outputs:
**  None.
**
** Returns:
**  0           Platform verified successfully
**  non-zero    Error verifying platform
**
** History:
**      14-Dec-2004 (fanra01)
**          Created.
**
*/
# ifdef NT_GENERIC
/*
** Name: OS_ARCH
**
** Description:
**  Structure to describe Ingres architecture coupled with actual hardware
**  architecture.
**
**      inghw    Ingres hardware code will run on the processor architecture.
**      procarch Processor architecture.
**
** History:
**      15-Dec-2004 (fanra01)
**          Created.
*/

typedef struct _os_arch
{
    i4 inghw;
    i4 procarch;
}OS_ARCH;

static OS_ARCH platform[] = 
{
    { II_IA32,  PROCESSOR_ARCHITECTURE_INTEL }, /* 32-bit Ingres on IA32 */
    { II_IA32,  PROCESSOR_ARCHITECTURE_AMD64 }, /* 32-bit Ingres on AMD64 */
    { II_IA64,  PROCESSOR_ARCHITECTURE_IA64  }, /* 64-bit Ingres on IA64 */
    { II_AMD64, PROCESSOR_ARCHITECTURE_AMD64 }, /* 64-bit Ingres on AMD64 */
    { 0x000000, 0 }
};

static i4
check_os_arch( i4 architecture )
{
    typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS)(HANDLE hProcess,PBOOL Wow64Process);
    typedef VOID (WINAPI *LPFN_GETNATIVESYSTEMINFO)(LPSYSTEM_INFO lpSystemInfo);
    BOOL bIsWow64 = FALSE;
    BOOL bVersVerified = FALSE;
    LPFN_ISWOW64PROCESS fnIsWow64Process;
    LPFN_GETNATIVESYSTEMINFO fnGetNativeSystemInfo;
    DWORDLONG dwlConditionMask = 0;
    i4  result = 0;
    i4  i;
    
    SYSTEM_INFO sysinfo;
    OSVERSIONINFOEX osvi;

    MEfill( sizeof(SYSTEM_INFO), 0, &sysinfo );
    MEfill( sizeof(OSVERSIONINFOEX), 0, &osvi );
 
    /*
    ** Test the processing mode.  bIsWow64 is TRUE if this is 32-bit code
    ** running in a 32-bit execution layer on a 64-bit processor.
    */
    if ((fnIsWow64Process =
        (LPFN_ISWOW64PROCESS)GetProcAddress( GetModuleHandle("kernel32"),
            "IsWow64Process")) != NULL)
    {
        if((fnGetNativeSystemInfo =
            (LPFN_GETNATIVESYSTEMINFO)GetProcAddress(
                GetModuleHandle("kernel32"), "GetNativeSystemInfo")) == NULL)
        {
            result = GetLastError();
        }
        if (result == 0)
        {
            if (!fnIsWow64Process(GetCurrentProcess(),&bIsWow64))
            {
                result = GetLastError();
            }
        }
    }
    if (result == 0)
    {
        /*
        ** Verify the minimum version requirement.
        ** This has been set at Windows NT 4 SP3.
        */
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
        osvi.dwMajorVersion = 4;
        osvi.dwMinorVersion = 0;
        osvi.wServicePackMajor = 3;
        VER_SET_CONDITION( dwlConditionMask, VER_MAJORVERSION, 
            VER_GREATER_EQUAL );
        VER_SET_CONDITION( dwlConditionMask, VER_MINORVERSION, 
            VER_GREATER_EQUAL );
        VER_SET_CONDITION( dwlConditionMask, VER_SERVICEPACKMAJOR, 
            VER_GREATER_EQUAL );
        bVersVerified = VerifyVersionInfo( &osvi, 
            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR,
            dwlConditionMask);
        if (bVersVerified == FALSE)
        {
            result = GetLastError();
        }
        else
        {
            /*
            ** Get the system information that includes the processor
            ** architecture.
            */
            if ((bIsWow64 == TRUE) &&
                (fnGetNativeSystemInfo != NULL))
            {
                /*
                ** Running 32-bit execution layer on a 64-bit platform
                ** Use function that gets true architecture.
                */
                fnGetNativeSystemInfo( &sysinfo );
            }
            else
            {
                GetSystemInfo( &sysinfo );
            }
            if (result == 0)
            {
                result = FAIL;
                for( i=0; platform[i].inghw != 0; i+=1 )
                {
                    if ((architecture == platform[i].inghw) &&
                        (sysinfo.wProcessorArchitecture == platform[i].procarch))
                    {
                        /*
                        ** Return true if and only if the Ingres hardware
                        ** platform is specifically associated with the
                        ** processor architecture.
                        */
                        result = OK;
                        break;
                    }
                }
            }
        }
    }
    return(result);
}
# endif /* NT_GENERIC */

# if defined(UNIX) 
/*
** Name: check_os_arch - Verify OS and architecture on UNIX/LINUX
**
** Description:
**  Function verifies the hardware architecture and OS system, 
**  determines whether the platform should support an installation of
**  Ingres.
**
**
** Inputs: Ingres architecture  ( defined as GV_HW in gv.h )   
**
**
** Returns:
**  0           Platform verified successfully
**  non-zero    Error verifying platform
**
** History:
**      15-Oct-2006 (hweho01)
**          Added for Unix/Linux platforms.
**
*/
/*
** Name: OS_ARCH
**
** Description:
**  Structure to describe Ingres architecture coupled with actual OS  
**  system name.
**
**      inghw    Ingres hardware code will run on the OS system.
**      os_name  system name.
**
*/
typedef struct _os_arch
{
    i4 inghw;
    char os_name[20];
}OS_ARCH;

static OS_ARCH platform[] = 
{
    { II_IA32,  "LINUX" }, /* 32-bit Ingres on IA32 */
    { II_IA32,  "LINUX" }, /* 32-bit Ingres on AMD64 */
    { II_IA64,  "LINUX" }, /* 64-bit Ingres on IA64 */
    { II_AMD64, "LINUX" }, /* 64-bit Ingres on AMD64 */
    { II_SPARC, "SUNOS" }, /* 32/64  Ingres on Sun Solaris */
    { II_PARISC, "HP-UX" }, /* 32/64  Ingres on HP UNIX */
    { II_POWERPC, "AIX" },   /* 32/64  Ingres on IBM POWERPC */
    { 0x000000,   NULL  } 
};

static i4
check_os_arch( i4 architecture )
{

   struct  utsname  sysinfo;  
   i4  ret,  i ; 
   i4  result = 0;
               
      /* check the running OS info */
      ret = uname( &sysinfo );
   
      result = FAIL;
      for( i=0; platform[i].inghw != 0; i+=1 )
         {
           if ((architecture == platform[i].inghw) &&
               (STcasecmp( sysinfo.sysname, platform[i].os_name) == 0))
                 {
                    result = OK;
                    break;
                 }
          }
    return(result);
}
# endif  /* UNIX */


/*
** Name: check_platform - Verify software platform
**
** Description:
**  Function determines the operating platform and verifies that the
**  minimum requirements are fulfilled.
**
** Inputs:
**  None.
**
** Outputs:
**  None.
**
** Returns:
**  0           architecture verification succeeded
**  non-zero    architecture failed verification
**
** History:
**  13-Dec-2004 (fanra01)
**      Created.
*/
static i4
check_platform( i4 architecture )
{
    i4 result = 0;
    ING_VERSION ingver;
    char* plat_override = NULL;
    
    NMgtAt(ERx("II_RELAX_PLATFORM"), &plat_override);
    if (plat_override && *plat_override)
    {
        return(result);
    }

    /*
    ** No architecture has been specified, use the internal compiled one.
    */
    if (architecture == II_DEFARCH)
    {
        MEfill( sizeof(ING_VERSION), 0, &ingver );
        /*
        ** Retrieve the hardware architecture used to build this version
        */
        if ((result = GVver( GV_M_HW, &ingver )) == OK)
        {
            /*
            ** Reassign the architecture to the retrieved one.
            */
            architecture = ingver.hardware;
        }
    }

    if (result == OK)
    {
        result = check_os_arch( architecture );
    }

    return(result);
}

/*
** Name: check_path_chars   - test path characters are supported.
**
** Description:
**  Test that the characters in the path are permitted.
**
** Inputs:
**  loc                 Pointer to location structure containing path.
**
** Outputs:
**  status              More detailed error code for failure.
**
** Returns:
**  result      OK      No invalid characters found.
**              FAIL    Invalid characters found in path.
**
** History:
**      14-Feb-2005 (fanra01)
**          Create to replace call to LOisvalid.
**	03-Jun-2005 (drivi01)
**	    Replaced Window's portion of check_path_chars
**	    with LOisvalid.
**  11-Jul-2005 (fanra01)
**      Add status output.
*/
static i4
check_path_chars( LOCATION* loc, STATUS* status )
{
    i4 result = OK;
    char	dev[MAX_LOC];
    char	path[MAX_LOC];
    char	file[MAX_LOC];
    char	ext[MAX_LOC];
    char	vers[MAX_LOC];
    char	*p;

# if defined(NT_GENERIC)

    if (!LOisvalid( loc, &result ))
    {
        if (status != NULL)
        {
            *status = result;
        }
        return FAIL;
    }

# else  /* NT_GENERIC */
    if (LOdetail( loc, dev, path, file, ext, vers ) != OK)
        return FAIL;
    for (p = path; *p != EOS; CMnext(p))
    {
	if (!(CMalpha(p) || CMdigit(p) || CMcmpnocase(p, PATH_SEPARATOR) ||
        '_' || *p == ' '))
	    return FAIL;
    }
    for (p = file; *p != EOS; CMnext(p))
    {
	if (!(CMalpha(p) || CMdigit(p) || *p == '_' || *p == ' '))
	    return FAIL;
    }
    for (p = ext; *p != EOS; CMnext(p))
    {
	if (!(CMalpha(p) || CMdigit(p) || *p == '_' || *p == ' '))
	    return FAIL;
    }
# endif /* NT_GENERIC */
    
    return(result);
}

/*
** Name: check_path
**
** Description:
**      Function to test the specified path for:
**          1. Valid characters.
**          2. An existing or valid parent directory.
**          3. Write permissions for 2.
**
** Inputs:
**      chkpath     pointer to the path string for validation.
**      eflags      flags specifying the actions to be taken.
**
** Outputs:
**      None.
**
** Returns:
**      OK          Path validation successful.
**      !OK         Path validation failed.
**
** History:
**      27-Jun-2005 (fanra01)
**          Created.
**      11-Jul-2005 (fanra01)
**          Return a more specific status code.
*/
static STATUS
check_path( char* chkpath, int eflags )
{
    STATUS          status = II_SUCCESSFUL;
    STATUS          rc;
    int             retcode = FAIL;
    LOCATION        tloc;                      /* target location */
    LOCATION        wloc;                      /* working location */
    LOCATION        cloc;                      /* working location */
    LOINFORMATION   linfo;
    i4              info;
    char*           temp = NULL;
    char*           path = NULL;
    char*           work = NULL;
    char*           curr = NULL;
    char*	        d;
    char*           p;
    char*           f;
    char*           e;
    char*           v;
    
    char*           s;

    while(TRUE)
    {
        /*
        ** Allocate working area up front.  Saves declaring arrays on the
        ** stack.
        */
        if ((temp = MEreqmem( 0, (MAX_LOC+1) * 8, TRUE, &status )) == NULL)
        {
            break;
        }
        
        /*
        ** Initialize working pointers with memory
        */
        path = temp;
        work = temp + MAX_LOC + 1;
        curr = work + MAX_LOC + 1;
        d = curr + MAX_LOC + 1;
        p = d + MAX_LOC + 1;
        f = p + MAX_LOC + 1;
        e = f + MAX_LOC + 1;
        v = e + MAX_LOC + 1;

        /*
        ** Initialize a location structure with the specified path
        ** string.
        */
        if ((eflags & (II_CHK_PATHCHAR | II_CHK_PATHDIR | II_CHK_PATHPERM)) &&
            (LOfroms( PATH, chkpath, &tloc ) != OK))
        {
            status = II_BAD_PATH;
            break;
        }

        /*
        ** Perform an illegal characters check, for all path tests.
        */
        if ((eflags & (II_CHK_PATHCHAR | II_CHK_PATHDIR | II_CHK_PATHPERM)) &&
            (check_path_chars( &tloc, &rc )))
        {
            switch(rc)
            {
                case LO_BAD_DEVICE:
                    status = II_BAD_PATH;
                    break;
                case LO_NOT_PATH:
                case LO_NOT_FILE:
                default:
                    status = II_INVAL_CHARS_IN_PATH;
                    break;
            }
            break;
        }

        /*
        ** Duplicate the specified path location into a work location.
        */
        LOcopy( &tloc, work, &wloc );
        
        /*
        ** Create an empty location for the current working device,
        ** split the target path into components and
        ** create a location of the target device.
        */
        if ((eflags & (II_CHK_PATHDIR | II_CHK_PATHPERM)) &&
            ((status = LOfroms( PATH, curr, &cloc )) == OK) &&
            ((status = LOdetail( &wloc, d, p, f, e, v )) == OK) &&
            ((status = LOcompose( d, CURR_DIR, NULL, NULL, NULL,
            &cloc )) == OK))
        {
            /*
            ** Save the current working directory
            */
            LOsave();
            
            /*
            ** Change working path to the target device
            */
            status = LOchange( &cloc );
        
            /*
            ** Starting with the whole path work backwards looking for
            ** a valid directory
            */
            for (s=work, info=0;
                (status == OK) && (retcode != OK) && (s != NULL);
                )
            {
                if ((status = LOfroms( PATH, p, &wloc )) != OK)
                {
                    status = II_BAD_PATH;
                    break;
                }
                /*
                ** Reset requested information flags for each iteration.
                */
                info = (LO_I_TYPE | LO_I_PERMS);

                switch(retcode = LOinfo( &wloc, &info, &linfo ))
                {
                    case OK:
                        /*
                        ** If the path or permission test is requested and
                        ** type info is returned test for directory flag. 
                        */
                        if ((eflags & (II_CHK_PATHDIR | II_CHK_PATHPERM)) &&
                            ((info & LO_I_TYPE) == LO_I_TYPE))
                            status = (linfo.li_type == LO_IS_DIR) ? OK : II_PATH_NOT_DIR;

                        /*
                        ** If the permission test is requested and
                        ** permissions are returned test the flags for read
                        ** and write.
                        */
                        if ((status == OK) && (eflags & II_CHK_PATHPERM))
                        {
                            if (((info & LO_I_PERMS) == LO_I_PERMS) &&
                                (linfo.li_perms & (LO_P_READ|LO_P_WRITE))
                                == (LO_P_READ|LO_P_WRITE))
                            {
                                /*
                                ** Read and write permission
                                */
                                break;
                            }
                            else
                            {
                                /*
                                ** missing a permission
                                */
                                status = II_PATH_CANNOT_WRITE;
                            }
                        }
                        else
                        {
                            break;
                        }
                    case LO_NO_SUCH:
                        /*
                        ** Look backwards for the next path separator
                        */
                        if((s = STrindex( p, PATH_SEPARATOR, 0 )) != NULL)
                        {
                            /*
                            ** If separator found truncate the path
                            ** otherwise the start of the path has
                            ** been reached, update string to test the
                            ** root directory.
                            */
                            if (s != p)
                            {
                                *s = '\0';
                            }
                            else
                            {
                                *(s+1) = '\0';
                            }
                        }
                        else
                        {
                            /*
                            ** A root path character was included in the
                            ** path that has been reached and still no
                            ** installable area found.
                            */                                
                            if (p && *p && *p == SLASH)
                            {
                                status = II_BAD_PATH;
                            }
                            else
                            {
                                /*
                                ** A relative path was specified and no
                                ** installable area has been found.
                                ** Test the current working directory of the
                                ** target device.
                                */
                                if ((status = LOgt( p, &wloc )) == OK)
                                {
                                    /*
                                    ** Reset temporary pointer to a work
                                    ** area to satisfy the loop condition.
                                    */
                                    s = work;
                                }
                            }
                        }
                        break;
                    default:
                        status = II_BAD_PATH;
                        break;
                }
            }
            break;
        }
        else
        {
            if (status != II_SUCCESSFUL)
            {
                status = II_BAD_PATH;
            }
            break;
        }
    }
    /*
    ** Free the working area
    */
    if (temp != NULL)
    {
        MEfree( temp );
    }
    return(status);
}

/*
** Name: check_instcode - test the instance code for invalid characters
**
** Description:
**      Tests the instance code for invalid characters or sequence of
**      characters.
**
** Inputs:
**      ii_install      Pointer to instance code string
**
** Outputs:
**      None.
**
** Returns:
**      II_SUCCESSFUL           valid instance code
**      II_INVALID_INSTANCECODE invalid instance code
**
** History:
**      17-Oct-2005 (fanra01)
**          Created.
*/
static STATUS
check_instcode( char* ii_install )
{
    STATUS status = II_SUCCESSFUL;
    
    if (!CMvalidinstancecode( ii_install ))
    {
        status = II_INVALID_INSTANCECODE;
    }
    return(status);
}

/*{
** Name: II_PrecheckInstallation - Pre-check installation environment
**
** Description:
**	Verify that a machine environment will support the current release of
**	Ingres. The following checks are made:
**(E
**	a. A minimum operating system version
**	b. Other required software (currently none)
**	c. Check the install path
**	   - exists
**	   - that user can write to it
**	   - does not have any disallowed characters (Windows only).
**)E
**
** Inputs:
**	response_file	path to the Embedded Ingres response file.
**                  For more information concerning response file options
**                  please see the Ingres Embedded Edition User Guide.
**
** Outputs:
**  error_msg       Message text for status.  If the message text exceeds
**                  MAX_IIERR_LEN then the message is truncated to
**                  MAX_IIERR_LEN.
**                  Unchanged if II_SUCCESSFUL is returned.
**
** Returns:
**  II_SUCCESSFUL           The environment test completed successfully
**  II_NULL_PARAM           The parameters passed are invalid
**  II_NO_INSTALL_PATH      Error reading response file or no II_SYSTEM entry
**  II_CHARMAP_ERROR        Error processing character mapping file
**  II_GET_COMPUTER_FAIL    Attempt to retrieve computer name failed
**  II_INVALID_HOST         Computer name contains illegal characters
**  II_GET_HOST_FAILED      Attempt to retrieve the network name failed
**  II_INVALID_USER         The username contains illegal characters
**  II_UNMATCHED_NAME       Computer name does not match network name
**  II_OS_NOT_MIN_VERSION   OS version does not meet the minimum requirement
**  II_BAD_PATH             The II_SYSTEM path is incorrectly formed
**  II_INVAL_CHARS_IN_PATH  The II_SYSTEM path contains illegal characters
**  II_PATH_NOT_DIR         The II_SYSTEM path is not a directory
**  II_PATH_CANNOT_WRITE    No write permission in the II_SYSTEM directory
**  II_PATH_EXCEEDED        Length of the path exceeds permitted maximum
**
** Example:
**  # include "tngapi.h"
**
**  # if defined(UNIX)
**  II_CHAR reponsefile[] = {"./ingres.rsp"};
**  # else
**  II_CHAR reponsefile[] = {".\\ingres.rsp"};
**  # endif
**  II_INT4 status;
**  II_CHAR error_msg[MAX_IIERR_LEN + 1] = { '\0' };
**
**  if ((status = II_PrecheckInstallation( responsefile, error_msg )) !=
**      II_SUCCESSFUL)
**  {
**      printf( "II_PrecheckInstallation failed - %s\n", error_msg );
**  }
**
** Side Effects:
**	none
**
** History:
**	16-apr-2002 (abbjo03)
**	    Written.
**  11-Jul-2005 (fanra01)
**      Add more specific status code from check_path_chars.
*/
int
II_PrecheckInstallation(
char	*response_file,
char	*error_msg)
{
    LOCATION		loc;
    char		buf[MAX_LOC];
    char		charmap[MAX_LOC+1];
    LOINFORMATION	loinfo;
    i4			flags;
    int			result;
    CL_ERR_DESC clerr;

    if (!response_file || !error_msg)
    {
        result = II_NULL_PARAM;
        if (error_msg)
            II_GetErrorMessage(error_msgs[result], error_msg);
        return result;
    }

    if (!GetPrivateProfileString("Ingres Configuration", "II_SYSTEM", "", buf,
	    sizeof(buf), response_file))
    {
        result = II_NO_INSTALL_PATH;
        II_GetErrorMessage(error_msgs[result], error_msg);
        return result;
    }
    
    /*
    ** Get the charmap field from the response file.
    */
    if (!GetPrivateProfileString("Ingres Configuration", II_CHARMAP, "",
        charmap, sizeof(charmap), response_file))
    {
        result = II_NO_CHAR_MAP;
        II_GetErrorMessage(error_msgs[result], error_msg);
        return result;
    }

    /*
    ** Open and read the character map specified in the response file.
    ** This is a special installation character description file.
    */
#if defined(UNIX)
    if ((result = CMset_attr( charmap, &clerr )) != 0)
#else
    if ((result = CMread_attr( II_CHARSET, charmap, &clerr )) != 0)
#endif 
    {
        result = II_CHARMAP_ERROR;
        II_GetErrorMessage( error_msgs[result], error_msg );
        return(result);
    }

    if ((result = check_user()) != II_SUCCESSFUL)
    {
        II_GetErrorMessage( error_msgs[result], error_msg );
        return(result);
    }

    if ((result = check_host()) != II_SUCCESSFUL)
    {
        II_GetErrorMessage( error_msgs[result], error_msg );
        return(result);
    }
    
    if (check_platform( II_DEFARCH ) != OK)
    {
	result = II_OS_NOT_MIN_VERSION;
	II_GetErrorMessage(error_msgs[result], error_msg);
	return result;
    }

    if (LOfroms(PATH & FILENAME, buf, &loc) != OK)
    {
	result = II_BAD_PATH;
	II_GetErrorMessage(error_msgs[result], error_msg);
	return result;
    }

    flags = LO_I_TYPE | LO_I_PERMS;
    if (LOinfo(&loc, &flags, &loinfo) != OK)
    {
	result = II_BAD_PATH;
	II_GetErrorMessage(error_msgs[result], error_msg);
	return result;
    }

    if ((flags & LO_I_TYPE) == 0 || loinfo.li_type != LO_IS_DIR)
    {
	result = II_PATH_NOT_DIR;
	II_GetErrorMessage(error_msgs[result], error_msg);
	return result;
    }
    if ((flags & LO_I_PERMS) == 0 || (loinfo.li_perms & LO_P_WRITE) == 0)
    {
	result = II_PATH_CANNOT_WRITE;
	II_GetErrorMessage(error_msgs[result], error_msg);
	return result;
    }
    
    if (check_path_chars(&loc, &result))
    {
        switch(result)
        {
            case LO_BAD_DEVICE:
                result = II_BAD_PATH;
                break;
            case LO_NOT_PATH:
            case LO_NOT_FILE:
            default:
                result = II_INVAL_CHARS_IN_PATH;
                break;
        }
	II_GetErrorMessage(error_msgs[result], error_msg);
	return result;
    }
    
    result = II_SUCCESSFUL;
    II_GetErrorMessage(error_msgs[result], error_msg);
    return result;
}

/*{
** Name: II_PrecheckInstallationEx - Extended installation environment pre-check
**
** Description:
**  Verify that a machine environment will support the current release of
**  Ingres.
**  The following checks may be made by specifying the required set of flags:
**(E
**
**  a. Test for invalid characters in the machine hostname
**  b. Test for invalid characters in the username
**  c. A minimum operating system version
**  d. Test that the path value supplied for II_SYSTEM
**      i)      does not contain disallowed characters (Windows only)
**      ii)     consists of an existing parent directory
**  e. Test that the operating system user of the running process is granted
**     permission to write in d.ii).
**  f. The value of the instancecode field in the response file is tested
**     for valid characters and character sequences.
**)E
**
**  The path check flags II_CHK_PATHDIR and II_CHK_PATHPERM may be combined
**  (bit wise); they include an implicit II_CHK_PATHCHAR. All path checks
**  implicitly include an II_CHK_PATHLEN flag.
**  Use the II_CHK_ALL flag as a convenient shorthand to perform each of the
**  tests together.
**  Specifying a flag of zero will check for the existence of the
**  II_SYSTEM entry in the response file.
**
** Inputs:
**  eflags          Specifies the pre-installation checks to be performed.
**                  Can be a combination of the following
**                  0                   Test for existence of II_SYSTEM entry
**                                      in response file
**                  II_CHK_HOSTNAME     Verify characters in machine name
**                  II_CHK_OSVERSION    Verify minimum OS version
**                  II_CHK_PATHCHAR     Check characters used in II_SYSTEM
**                  II_CHK_PATHDIR      Check II_SYSTEM path for a valid
**                                      existing directory
**                  II_CHK_PATHPERM     Check path write permissions
**                  II_CHK_PATHLEN      Check length of path not exceeded
**                  II_CHK_USERNAME     Verify characters in user name
**                  II_CHK_INSTCODE     Verify characters of the configured
**                                      instance code
**                  II_CHK_ALL          Equivalent to II_PrecheckInstallation
**                                      call.
**  architecture    The value of architecture is used only if the
**                  II_CHK_VERSION flag is set in the eflags field.
**                  If the II_CHK_VERSION flag is set
**                      Let the value of architecture be set as follows:
**                      Case:
**                          -   II_DEFVER determines if the suitability of
**                              the internal Ingres platform architecture
**                              is appropriate for the operating architecture.
**                          -   Determines if the suitability of the defined
**                              value of architecture is appropriate for the
**                              operating architecture.
**                  If the II_CHK_VERSION flag is not set
**                      Let the value of architecture be set to II_DEFVER.
**
**  response_file   Path to the Embedded Ingres response file
**                  For more information concerning response file options
**                  please see the Ingres Embedded Edition User Guide.
**                  This function requires valid values for the II_SYSTEM and
**                  charmap entries.
**
** Outputs:
**  error_msg       Message text for status.  If the message text exceeds
**                  MAX_IIERR_LEN then the message is truncated to
**                  MAX_IIERR_LEN.
**                  Unchanged if II_SUCCESSFUL is returned.
**
** Returns:
**  II_SUCCESSFUL           The environment test completed successfully
**  II_NULL_PARAM           The parameters passed are invalid
**  II_NO_INSTALL_PATH      Error reading response file or no II_SYSTEM entry
**  II_NO_CHAR_MAP          Error reading response file or no II_CHARMAP entry
**  II_CHARMAP_ERROR        Error processing character mapping file
**  II_GET_COMPUTER_FAIL    Attempt to retrieve computer name failed
**  II_INVALID_HOST         Computer name contains illegal characters
**  II_GET_HOST_FAILED      Attempt to retrieve the network name failed
**  II_INVALID_USER         The username contains illegal characters
**  II_UNMATCHED_NAME       Computer name does not match network name
**  II_OS_NOT_MIN_VERSION   OS version does not meet the minimum requirement
**  II_BAD_PATH             The II_SYSTEM path is incorrectly formed
**  II_INVAL_CHARS_IN_PATH  The II_SYSTEM path contains illegal characters
**  II_PATH_NOT_DIR         The II_SYSTEM path does not contain a valid
**                          directory
**  II_PATH_CANNOT_WRITE    No write permission in the II_SYSTEM directory or
**                          a parent directory.
**  II_PATH_EXCEEDED        Length of the path exceeds permitted maximum
**  II_INVALID_INSTANCECODE Instance code contains illegal characters
**  II_INSTANCE_ERROR       Error reading instance code or no instancecode
**                          entry.
**
** Example:
**  # include "tngapi.h"
**
**  # if defined(UNIX)
**  II_CHAR reponsefile[] = {"./ingres.rsp"};
**  # else
**  II_CHAR reponsefile[] = {".\\ingres.rsp"};
**  # endif
**  II_INT4 status;
**  II_INT4 eflags = (II_CHK_HOSTNAME | II_CHK_OSVERSION | \
**      II_CHK_PATHCHAR | II_CHK_PATHDIR | II_CHK_PATHPERM | II_CHK_PATHLEN \
**      II_CHK_INSTCODE);
**  II_CHAR error_msg[MAX_IIERR_LEN + 1] = { '\0' };
**
**  if ((status = II_PrecheckInstallationEx( eflags, II_DEFARCH, responsefile,
**      error_msg )) != II_SUCCESSFUL)
**  {
**      printf( "II_PrecheckInstallationEx failed - %s\n", error_msg );
**  }
**
**  if ((status = II_PrecheckInstallationEx( II_CHK_OSVERSION, II_IA64,
**      responsefile, error_msg )) != II_SUCCESSFUL)
**  {
**      printf( "II_PrecheckInstallationEx failed - %s\n", error_msg );
**      if (status == II_OS_NOT_MIN_VERSION)
**      {
**          printf( "Target environment is unsuitable for IA64 binaries\n");
**      }
**  }
**
** Side Effects:
**  none
**
** History:
**  07-Sep-2004 (fanra01)
**      Created based on II_PrecheckInstallation.
**  30-Jun-2005 (fanra01)
**      Moved and extended path validation into check_path function.
**  18-Aug-2005 (fanra01)
**      Include path length check on character map file.
}*/
int
II_PrecheckInstallationEx(
    int     eflags,
    int     architecture,
    char*   response_file,
    char*   error_msg
)
{
    i4                  status = II_SUCCESSFUL;
    i4			        flags;
    i4                  pathlen;
    LOCATION		    loc;
    LOINFORMATION	    loinfo;
    char		        buf[MAX_LOC + 3]; /* over size buffer to detect overrun */
    char		        charmap[MAX_LOC + 3];
    char		        ii_install[MAX_LOC + 3];
    CL_ERR_DESC         clerr;
    i4                  pathchk = 0;
    
    while(TRUE)
    {
        if (!response_file || !error_msg)
        {
            status = II_NULL_PARAM;
            break;
        }
        if ((pathlen = GetPrivateProfileString( ERx("Ingres Configuration"),
            ERx("II_SYSTEM"), "", buf, sizeof(buf), response_file )) == 0)
        {
            status = II_NO_INSTALL_PATH;
            break;
        }
        else
        {
            if ((pathchk = (eflags & (II_CHK_PATHCHAR | II_CHK_PATHDIR |
                             II_CHK_PATHPERM | II_CHK_PATHLEN))) &&
                (pathlen > MAX_LOC))
            {
                status = II_PATH_EXCEEDED;
                break;
            }
        }
        if (eflags & II_CHK_INSTCODE)
        {
            if ((pathlen = GetPrivateProfileString("Ingres Configuration",
                 II_INSTCODE, "", ii_install, sizeof(ii_install), response_file)) == 0)
            {
                status = II_INSTANCE_ERROR;
                break;
            }
            else
            {
                if ((status = check_instcode( ii_install )) != II_SUCCESSFUL)
                {
                    break;
                }
            }
        }
        if ((eflags & (II_CHK_HOSTNAME | II_CHK_USERNAME | pathchk)) &&
            ((pathlen = GetPrivateProfileString("Ingres Configuration",
             II_CHARMAP, "", charmap, sizeof(charmap), response_file)) == 0))
        {
            status = II_NO_CHAR_MAP;
            break;
        }
        else
        {
            if (eflags & (II_CHK_HOSTNAME | II_CHK_USERNAME | pathchk))
            {
                if (pathlen > MAX_LOC)
                {
                    status = II_PATH_EXCEEDED;
                    break;
                }
                else
                {
#if defined(UNIX)
                    if ((status = CMset_attr( charmap, &clerr )) != 0)
#else
                    if ((status = CMread_attr( II_CHARSET, charmap, &clerr )) != 0)
#endif 
                    {
                        status = II_CHARMAP_ERROR;
                        break;
                    }
                }
            }
        }
    
        if ((eflags & II_CHK_HOSTNAME) &&
            ((status = check_host()) != II_SUCCESSFUL))
        {
            break;
        }
        
        if ((eflags & II_CHK_USERNAME) &&
            ((status = check_user()) != II_SUCCESSFUL))
        {
            break;
        }
        
        if ((eflags & II_CHK_OSVERSION) &&
            (check_platform( architecture ) != OK))
        {
            status = II_OS_NOT_MIN_VERSION;
            break;
        }

        /*
        ** Perform path validation test.
        */
        if (eflags & (II_CHK_PATHCHAR | II_CHK_PATHDIR | II_CHK_PATHPERM))
        {
            status = check_path( buf, eflags );
            break;
        }
        break;
    }
    if ((status != II_SUCCESSFUL) && (error_msg != NULL))
    {
        II_GetErrorMessage( error_msgs[status], error_msg );
    }
    return(status);
}

/*{
** Name: II_GetIngresErrorMessage - Lookup error message from message file
**
** Description:
**  Return the text associated with the Ingres status.
**  The function returns the text associated with the Ingres status including
**  the decoded facility representation of the status in the text.
**
** Inputs:
**  ingres_status           Retrieve the text message associated with this
**                          value.
**  message_length          Length of the buffer area to write the message
**                          including additional space for the end of string
**                          marker.
**  message                 Pointer to the memory area where the retrieved
**                          message is to be written.
**
** Outputs:
**  message_length          If the function returns II_SUCCESSFUL the
**                          message_length field contains the number of
**                          storage octets in the message area that were used.
**  message                 The message text associated with the Ingres
**                          status.
**
** Returns:
**  II_SUCCESSFUL           The message was retrieved successfully
**  II_NULL_PARAM           The parameters passed are invalid; no output
**                          fields are updated.
**  II_INSUFFICIENT_BUFFER  A larger message area is required.
**  II_GET_MESSAGE_ERROR    An error orccured during the execution of this
**                          function.
**
** Example:
**  # include "tngapi.h"
**
**  II_INT4 status;
**  II_INT4 ingres_status = 0xC0132;
**  II_INT4 message_length = 20;
**  II_CHAR message[MAX_IIERR_LEN + 1;
**
**  status = II_GetIngresMessage( ingres_status, &message_length, message );
**  if (status != II_SUCCESSFUL)
**  {
**      if (status == II_INSUFFICIENT_BUFFER)
**      {
**          printf( "A larger buffer is required\n" );
**      }
**  }
**
**  message_length = MAX_IIERR_LEN + 1;
**  status = II_GetIngresMessage( ingres_status, &message_length, message );
**  if (status != II_SUCCESSFUL)
**  {
**      printf( "II_GetIngresMessage failed\n" );
**  }
**
** History:
**      20-Jan-2005 (fanra01)
**          Created.
}*/
int
II_GetIngresMessage(
    unsigned int ingres_status,
    int* message_length,
    char* message )
{
    i4          status;
    CL_ERR_DESC clerror;
    i4          flags;
    i4          msglen = 0;

    CL_CLEAR_ERR( &clerror );
    
    while(TRUE)
    {
        /*
        ** If any of the parameters are not valid, stop.
        */
        if ((message == NULL) || (message_length == NULL) ||
            (*message_length == 0))
        {
            status = II_NULL_PARAM;
            break;
        }
        /*
        ** Call ERslookup to get the message text from the message file.
        ** Let the value of flags be:
        **      ~ER_TEXTONLY    is not set, the error is decoded to include
        **                      the error identifier.
        **      ER_NOPARAM      there are no parameter values provided to be
        **                      entered into the message string.
        ** Let the value of language be:
        **      -1              use the default language.
        */
        flags = ER_NOPARAM;
        status = ERslookup( ingres_status, NULL, flags, NULL, message,
            *message_length, -1, &msglen, &clerror, 0, NULL );
        switch(status)
        {
            case OK:
                *message_length = STlength( message );
                status = II_SUCCESSFUL;
                break;
            case ER_TOOSMALL:
                status = II_INSUFFICIENT_BUFFER;
                break;
            default:
                status = II_GET_MESSAGE_ERROR;
                break;
        }
        break;
    }
    return(status);
}
