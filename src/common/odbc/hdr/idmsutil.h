/*
**  IDMSUTIL
**
**  Module Name:    idmsutil.h
**
**  description:    IDMSUTIL header file.
**
**
**   Change History
**   --------------
**
**   date        programmer          description
**
**   11/25/1992  Dave Ross           Initial coding.
**   10/06/1993  Dave Ross           Replacement for IDMSUWIN.
**   06/02/1994  Dave Ross           Port to UNIX.
**   02/06/1996  Dave Ross           Port to Win32.
**   02/17/1999  Dave Thole          Port to UNIX.
**   05/11/2002  Ralph Loen          Removed dependency on API data types. 
**      14-feb-2003 (loera01) SIR 109643
**          Minimize platform-dependency in code.  Make backward-compatible
**          with earlier Ingres versions.
**      07-mar-2003 (loera01) SIR 109789
**          More clean-up.  Add getReadOnly routine.
*/

#ifndef _INC_IDMSUTIL
#define _INC_IDMSUTIL

#ifdef _WIN32
#define DSIAPI              __stdcall
#define FASTCALL            __fastcall
#else
#define FASTCALL
#define DSIAPI
#endif

#ifdef _CAIDUTIL
#define UTIL32API DLLEXPORT
#define UTIL16API EXPORT
#else
/*#define UTIL32API DLLIMPORT*/
#define UTIL32API
#define UTIL16API
#endif

#ifndef _WIN32       /* Windows NT, 95, 3.1 specific */
#ifndef CDECL
#define CDECL
#endif
#undef  EXPORT
#define EXPORT 
#ifndef PASCAL
#define PASCAL
#endif
#ifndef NEAR
#define NEAR
#endif
#ifndef CALLBACK
#define CALLBACK
#endif
typedef const char    *LPCSTR;
typedef unsigned int   UINT;
typedef void *         HANDLE;
typedef HANDLE         HINSTANCE;
typedef HANDLE         HGLOBAL;
typedef HANDLE         HRSRC;
typedef char           CHAR;
typedef UINT           WPARAM;
typedef i4             LPARAM;
LPVOID  LockResource(HGLOBAL hResData);
BOOL    FreeResource(HGLOBAL hResData);
#endif  /* end ifndef (_WIN32) */


/*
#ifdef __hpux
#include <dl.h>
#endif
*/

#ifdef __cplusplus
extern "C" {
#endif

/*
**  Utility functions exported by CAIDUTIL and CAIDUT32:
*/
int  UtGetIniInt (LPCSTR, LPCSTR, int);
void UtGetIniField (LPCSTR, LPCSTR, LPCSTR, LPSTR, int);
int  UtGetIniString (LPCSTR, LPCSTR, LPCSTR, LPSTR, int);
BOOL UtWriteIniString (LPCSTR szSection, LPCSTR, LPCSTR);
void UtTimer (unsigned long timer);
void UtPrint (LPCSTR szFormat, ...);

#ifdef _WIN32

//
//  Utility functions exported by CAIDUT32:
//
void UtSnap (LPCSTR, const void *, UINT);
#define UtLog UtPrint

//
//  Utility functions to maintain registry info:
//
LONG UtRegCreateKey (LPCSTR szSubkey, PHKEY, LPDWORD pdwDisp);
LONG UtRegOpenKey (LPCSTR szSubkey, PHKEY);
LONG UtRegQueryValue (HKEY, LPCSTR szKeyname, LPDWORD, LPBYTE, LPDWORD);
LONG UtRegSetValue (HKEY, LPCSTR szKeyname, DWORD, const BYTE *, DWORD);
int  UtRegGetPath (LPCSTR szSection, LPCSTR szKeyname, LPSTR szSubkey);
BOOL UtWriteIniInt (LPCSTR szSection, LPCSTR szKeyname, LPCSTR szValue);

//
//  UtRegGetPath return codes:
//
#define UT_REG_SUCCESS     0        // Subkey is in IDMS section of registry
#define UT_REG_ODBC        1        // Subkey assumed to be in ODBC section of registry
#define UT_REG_TEMP        2        // Subkey is in cache, not in registry
#define UT_REG_SERVERS     3        // Internal use only, not returned

#else

void UtSnap (LPCSTR, const void *, long);
void UtLog (LPCSTR, ...);
void UtGetIniSect (LPSTR,  LPCSTR);

#endif

BOOL GetReadOnly(char *);

#ifndef NT_GENERIC

UINT    GetPrivateProfileInt      (LPCSTR, LPCSTR, int, LPCSTR);
int     GetPrivateProfileString   (LPCSTR, LPCSTR, LPCSTR, LPSTR, int, LPCSTR);
BOOL WritePrivateProfileString (LPCSTR, LPCSTR, LPCSTR, LPCSTR);

#ifdef NT_GENERIC
HINSTANCE LoadLibrary    (LPCSTR);
void      FreeLibrary    (HINSTANCE);
FARPROC   GetProcAddress (HINSTANCE, LPCSTR);
#endif

#endif  /* ifndef NT_GENERIC */

/*
**  Log File Options:
*/
#define LOG_APPEND      0x0001          /* append to existing log    */
#ifndef _WIN32
#define LOG_CLOSE       0x0002          /* default, close log        */
#define LOG_OPEN        0x0004          /* keep log file open        */
#define LOG_PROCESS     0x0008          /* separate log for each pid */
#endif

/*
**  Debug macros:
*/
#define SNAP(o,t,a,l)   if ((o) && (a)) UtSnap(t,a,l)
#ifndef __cplusplus
#endif

#ifdef NT_GENERIC
#define TIMESTART(o,t)  if (o) t = GetCurrentTime()
#define TIMEEND(o,t)    if (o) t = GetCurrentTime() - t
#else
#define TIMESTART(o,t)  if (o) t = 0
#define TIMEEND(o,t)    if (o) t = 0
#endif

#define TIMEPRINT(o,t)  if (o) UtTimer(t)

#ifdef __cplusplus
}
#endif

#endif /* _INC_IDMSUTIL */
