/*
**  IDMSUT32
**
**  Module Name:    idmsut32.h
**
**  description:    CAIDUT32 internal header file.
**
**
**   Change History
**   --------------
**
**   date        programmer          description
**
**   11/08/1994  Dave Ross           Initial coding.
**    1/09/1996  Dave Ross           Win32 port.
**      14-feb-2003 (loera01) SIR 109643
**          Minimize platform-dependency in code.  Make backward-compatible
**          with earlier Ingres versions.
**
*/

#ifndef _INC_IDMSUT32
#define _INC_IDMSUT32

#include <stdio.h>
#include "idmsureg.h"                   // registry strings
/*#include "idmsuids.h"                   // string resource ids */

//
//  Log File Status:
//
#define LOG_OPEN        0x0001          // keep log file open

//
//  Global variables and constants defined in idmsut32.c:
//
GLOBALREF BOOL     fDebug;                 // UTIL debugging flag
GLOBALREF DWORD    fLogStatus;             // log status
GLOBALREF FILE *   pfileLog;               // -->log file block
GLOBALREF HANDLE   hLogMutex;              // Single thread access to log
GLOBALREF CRITICAL_SECTION csUsb;          // Single thread access to USB

GLOBALREF char  REG_PATH_OPTIONS[];  // Registry path for global options
GLOBALREF char  INI_OPTIONS[];       // Old ini file [Options] section name

GLOBALREF DWORD fLogOptions;         // Log options
GLOBALREF char  szLogFile[];         // Log file name

#ifndef TRACE_MSG
#define TRACE_MSG       "ODBC Trace: %s\r\n"
#define ERROR_MSG       "ODBC Error: %s\r\n"
#endif

//
//  Internal macro definitions:
//
//#define TRACE_INTL(f) TRACE(fDebug & UTIL_TRACE_INTL,TRACE_MSG,f)

//
//  Internal function prototypes:
//

#endif                                  // _INC_IDMSUT32
