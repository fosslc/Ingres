#ifndef HTTPEXT_H
#define HTTPEXT_H
/********
*
*  Copyright (c) 2004 Ingres Corporation 
*
*  Copyright (c) 1995  Process Software Corporation
*
*  Copyright (c) 1995  Microsoft Corporation
*
*
*  Module Name  : HttpExt.h
*
*  Abstract :
*
*     This module contains  the structure definitions and prototypes for the
*     version 1.0 HTTP Server Extension interface.
*
** History:
**      18-Jan-1999 (fanra01)
**          Removed ISAPI specific function prototypes for unix.
******************/
#ifndef UNIX
#include <windows.h>
#else
#include <errno.h>
#define WINAPI
#define VOID	void
#define BOOL	char
#define PVOID	void *
#define LPVOID	void *
#define LPSTR	char *
#define LPBYTE	char *
#define LPDWORD	unsigned long *
#define CHAR	char
#define DWORD	unsigned long
#define MAKELONG(lsb, msb)		((lsb) | ((msb) << 16))
#define SetLastError(x)			errno = x
#define GetLastError()			errno
#define ERROR_NOT_ENOUGH_MEMORY		8
#define ERROR_INVALID_INDEX			1413
#define ERROR_INSUFFICIENT_BUFFER	122
#define ERROR_NO_DATA				232
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define   HSE_VERSION_MAJOR           2      
#define   HSE_VERSION_MINOR           0      
#define   HSE_LOG_BUFFER_LEN         80
#define   HSE_MAX_EXT_DLL_NAME_LEN  256

typedef   LPVOID  HCONN;

/* the following are the status codes returned by the Extension DLL */

#define   HSE_STATUS_SUCCESS                       1
#define   HSE_STATUS_SUCCESS_AND_KEEP_CONN         2
#define   HSE_STATUS_PENDING                       3
#define   HSE_STATUS_ERROR                         4

/* The following are the values to request services with the ServerSupportFunction.
  Values from 0 to 1000 are reserved for future versions of the interface */

#define   HSE_REQ_BASE                             0
#define   HSE_REQ_SEND_URL_REDIRECT_RESP           ( HSE_REQ_BASE + 1 )
#define   HSE_REQ_SEND_URL                         ( HSE_REQ_BASE + 2 )
#define   HSE_REQ_SEND_RESPONSE_HEADER             ( HSE_REQ_BASE + 3 )
#define   HSE_REQ_DONE_WITH_SESSION                ( HSE_REQ_BASE + 4 )
#define   HSE_REQ_END_RESERVED                     1000

/* These are Microsoft specific extensions */

#define   HSE_REQ_MAP_URL_TO_PATH                  (HSE_REQ_END_RESERVED+1)
#define   HSE_REQ_GET_SSPI_INFO                    (HSE_REQ_END_RESERVED+2)
#define   HSE_APPEND_LOG_PARAMETER                 (HSE_REQ_END_RESERVED+3)
#define   HSE_REQ_SEND_URL_EX                      (HSE_REQ_END_RESERVED+4)

/* Bit Flags for TerminateExtension

  HSE_TERM_ADVISORY_UNLOAD - The server wants to unload the extension.
  The extension can return TRUE if OK, FALSE if the server should not
  unload the extension.

  HSE_TERM_MUST_UNLOAD - Server indicating that the extension is about to
  be forcibly unloaded.   The extension cannot refuse.
*/

#define HSE_TERM_ADVISORY_UNLOAD                   0x00000001
#define HSE_TERM_MUST_UNLOAD                       0x00000002


/* passed to GetExtensionVersion */

typedef struct   _HSE_VERSION_INFO {

    DWORD  dwExtensionVersion;
    CHAR   lpszExtensionDesc[HSE_MAX_EXT_DLL_NAME_LEN];

} HSE_VERSION_INFO, *LPHSE_VERSION_INFO;

/* passed to extension procedure on a new request */
typedef struct _EXTENSION_CONTROL_BLOCK {

    DWORD     cbSize;                 /* size of this struct. */
    DWORD     dwVersion;              /* version info of this spec */
    HCONN     ConnID;                 /* Context number not to be modified! */
    DWORD     dwHttpStatusCode;       /* HTTP Status code */
    CHAR      lpszLogData[HSE_LOG_BUFFER_LEN]; /* null terminated log info
												specific to this Extension DLL */

    LPSTR     lpszMethod;             /* REQUEST_METHOD */
    LPSTR     lpszQueryString;        /* QUERY_STRING */
    LPSTR     lpszPathInfo;           /* PATH_INFO */
    LPSTR     lpszPathTranslated;     /* PATH_TRANSLATED */

    DWORD     cbTotalBytes;           /* Total bytes indicated from client */
    DWORD     cbAvailable;            /* Available number of bytes */
    LPBYTE    lpbData;                /* pointer to cbAvailable bytes */

    LPSTR     lpszContentType;        /* Content type of client data */

    BOOL (WINAPI * GetServerVariable) ( HCONN       hConn,
                                        LPSTR       lpszVariableName, 						                      							  					
				                        LPVOID      lpvBuffer,
                                        LPDWORD     lpdwSize );

    BOOL (WINAPI * WriteClient)  ( HCONN      ConnID,
                                   LPVOID     Buffer,
                                   LPDWORD    lpdwBytes,
                                   DWORD      dwReserved );

    BOOL (WINAPI * ReadClient)  ( HCONN      ConnID,
                                  LPVOID     lpvBuffer,
                                  LPDWORD    lpdwSize );

    BOOL (WINAPI * ServerSupportFunction)( HCONN      hConn,
                                           DWORD      dwHSERRequest,
                                           LPVOID     lpvBuffer,
                                           LPDWORD    lpdwSize,
                                           LPDWORD    lpdwDataType );

} EXTENSION_CONTROL_BLOCK, *LPEXTENSION_CONTROL_BLOCK;


/* these are the prototypes that must be exported from the extension DLL */
#ifndef UNIX
#define ICEAPI  __declspec(dllexport)

ICEAPI BOOL  WINAPI   GetExtensionVersion( HSE_VERSION_INFO  *pVer );
ICEAPI DWORD WINAPI   HttpExtensionProc(  EXTENSION_CONTROL_BLOCK *pECB );
ICEAPI BOOL  WINAPI   TerminateExtension( DWORD dwFlags );
#endif
/* the following type declarations are for the server side */

typedef BOOL  (WINAPI * PFN_GETEXTENSIONVERSION)( HSE_VERSION_INFO  *pVer );
typedef DWORD (WINAPI * PFN_HTTPEXTENSIONPROC )( EXTENSION_CONTROL_BLOCK *pECB );
typedef BOOL  (WINAPI * PFN_TERMINATEEXTENSION )( DWORD dwFlags );
#ifdef __cplusplus
}
#endif

#endif /* HTTPEXT_H */
