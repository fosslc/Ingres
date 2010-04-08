//
//  DLLMAI32.C
//
//  DLL entry and exit routines for the 32 bit Ingres ODBC driver.
//
//
//  Change History
//  --------------
//
//  date        programmer          description
//
//  11/25/1992  Dave Ross           Initial coding
//   3/14/1995  Dave Ross           ODBC 2.0 upgrade...
//   5/24/1996  Dave Ross           Win32 port
//   1/22/1996  Lionel Storck       Initialized cdUsb Critical Section to 
//                                  avoid abends when entrering the section.
//   7/07/1997  Dave Thole          LoadLibrary the OpenApi and CL libraries
//   9/17/1997  Dave Thole          Bullet-proof check for correct iiapi.dll
//  12/04/1997  Jean Teng           DBCS Support 
//   2/27/1998  Dave Thole,Dave Ross DBC multi-threading
//   4/20/1998  Dave Thole          Don't null the library handle before trying to free it
//   6/02/1998  Dave Thole          LockEnv doesn't need to lock every DBC
//  10/26/1998  Dave Thole          Enabled IIapi_registerXID
//  03/25/1999  Dave Thole          Updates for other product packaging
//   7/02/1999  Bobby Ward          Fixed Bug# 97975
//   11/8/1999  Bruce Lunsford      Memory leak of approx 176K avoided by not
//                                  freeing CLF & API DLLs at PROCESS_DETACH.
//                                  Memleak occurs when ODBC application 
//                                  repeatedly connects and disconnects.
//                                  Bug 98826. Temp Fix until these DLLs are
//                                  fixed. API only leaks about 1-2K per 
//                                  connect/disconnect; CLF the remainder.
//  01/11/2001  Dave Thole          Integrate MTS/DTC support into 2.8
//  09/25/2002  Ralph Loen          Bug 108800.  Added reference to 
//                                  IIapi_releaseEnv.
//  11/04/2002  Ralph Loen          Bug 108536. Added IIapi_setEnvParam() and
//                                  IIapi_formatData().
//  02/14/2003  Ralph Loen          SIR 109643.  DllMain no longer exists as a
//                                  custom DLL stub.  This file now simply
//                                  contains global data. No Ingres libraries
//                                  are loaded.
//  09/22/2003 Ralph Loen           Added ODBC internal trace log for other
//				    products and 
//                                  renamed Ingres ODBC internal trace log. 
//  06/08/2004 Ralph Loen           Added DLL_ATTACH stub info to initialize
//                                  hModule global used in dialog boxes.
//                                  Re-added dummy LoadByOrdinal().
//  06/11/2004 Sam Somashekar
//				    Cleaned up code doe Open Source.
//       3-dec-2004 (thoda04) Bug 113565
//          Initialize the csLibqxa critical section on DLL_PROCESS_ATTACH.
//
#include <compat.h>
#include <sql.h>                    /* ODBC Core definitions */
#include <sqlext.h>                 /* ODBC extensions */
#include <sqlstate.h>               /* ODBC driver sqlstate codes */
#include <sqlcmd.h>

#include <idmseini.h>
#include <idmsureg.h>

GLOBALDEF char INI_OPTIONS[] = KEY_OPTIONS;
GLOBALDEF char INI_TRACE[]   = KEY_TRACE_ODBC;
GLOBALDEF HINSTANCE   hModule;                // Saved module instance handle.
GLOBALDEF UWORD       fTrace;                 // Global trace options for environment
GLOBALDEF OSVERSIONINFO os;                   // OS version information
GLOBALDEF char    szIdmsHelp[MAX_PATH];   // Help file name.
GLOBALDEF HANDLE  hLogMutex=NULL;             // Log file mutex handle
GLOBALDEF DWORD   fLogStatus = 0;             // log status
GLOBALDEF char    szLogFile[MAX_PATH]="caiiod35.log";
GLOBALDEF FILE *  pfileLog   = NULL;
GLOBALDEF char REG_PATH_OPTIONS[] = REGSTR_PATH_OPTIONS;
GLOBALDEF BOOL    fDebug     = FALSE;         // UTIL debugging flag
GLOBALDEF char    szModuleFileName[MAX_PATH] = "caiiod35.dll";
GLOBALDEF HINSTANCE   hXolehlpModule=NULL;
GLOBALDEF HINSTANCE   hLibqModule   =NULL;   /* Saved handle of oiembdnt.dll */
GLOBALDEF HINSTANCE   hOixantModule =NULL;    /* Saved handle of oixant.dll  */

GLOBALREF
   CRITICAL_SECTION csLibqxa; /* Used to single thread calls to Ingres XA
                              routines since they are not thread-safe yet */

BOOL APIENTRY DllMain (
    HMODULE hmodule,
    DWORD   fdwReason,
    LPVOID  lpReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:

        hModule = hmodule;
        InitializeCriticalSection (&csLibqxa);
        break;
    default:
        break;
    }
    return TRUE;
}
/*
**  LoadByOrdinal
**
**  Dummy function to make ODBC driver manager use ordinals
**  when calling ODBC driver functions, for speed.
*/
void LoadByOrdinal (void)
{
    return;
}
