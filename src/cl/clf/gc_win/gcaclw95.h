/*
** Copyright (c) 1987, 2002 Ingres Corporation
*/

/**
** Name: GCINTERN.H
**
** Description:
**      This file contains the definitions of datatypes and constants
**      used within the GC module.
**
** History: $Log-for RCS$
**	24-feb-94 (marc_b)
**		History file created and additional definitions added to
**		support changes to GCusrpwd().
**      11-nov-95 (chech02)
**        Added support for Windows 95.
**      04-Sep-97 (fanra01)
**          Changed return type for IIWriteFile.
**	08-aug-1999 (mcgem01)
**	    Change nat to i4.
**      03-Dec-1999 (fanra01)
**          Bug 99392
**          Modifed IOPARMS to include a completion function.
**          Also change the async IO routines to be able to specify a
**          completion function as a parameter.
**      20-Sep-2001 (lunbr01) Bug 105793
**          Increase MAX_ASSOCS from 100 to 1000 and eliminate unneeded
**          MAX_CONNECTIONS and MAX_GCA_THREADS.  MAX_ASSOCS will also now
**          only affect Win9x, not NT or W2K.
**      27-sep-2001 (lunbr01) Bug 105902
**          Add prototype for GCreceiveCompletion.
**/

# define	LISTEN_PFX	"\\pipe\\ingres\\"
# define 	LISTEN_PFX_LEN	(sizeof(LISTEN_PFX)-1)
# define 	GCN_NAME	LISTEN_PFX "ii_gcn"
# define 	GCC_NAME	LISTEN_PFX "ii_gcc"
# define 	DBMS_NAME	LISTEN_PFX "ii_dbms"
# define 	MAXSHARE 	80
# define 	MAXUSER 	80
# define 	SIDSIZE 	64
# define 	MAX_DOMAIN_SIZE 	64

# define		GC_CHN_S_STD 0
# define		GC_CHN_R_STD 1
# define		GC_CHN_S_EXP 2 
# define		GC_CHN_R_EXP 3
# define		IPC_TIMEOUT_SLEEP 10 /* milliseconds */

# define		GC_WAIT_DEFAULT    0
# define		GC_WAIT_FOREVER		-1
#ifdef xDEBUG
# define GCACL_DEBUG_MACRO( y ) (void)TRdisplay y
#else
# define GCACL_DEBUG_MACRO( y )
#endif

# define    SLEEP_TIME_MSECS    100
# define    MAXPNAME            80      /* max length of a pipe name (including hostname) */
# define    MAX_ASSOCS          1000    /* max # associations on W9x */
# define    ACCESS_POINT_SIZE   10
//# define    LISTEN_PFX          ""
# define    CONNECT_NORMAL      1
# define    CONNECT_DETECT      0
# define    DETECT_TIMEOUT      0   /* detects should happen "instantly" */
# define    GC_STACK_SIZE       32768

/*
** Name:    IOPARMS
**
** Description:
**      Structure to hold I/O parameters in Windows 95 for asynchronous
**      completion.
**
**      pSvcParms       pointer to service paramater structure
**      pAsipc          pointer to IPC association structure
**      hFd             handle to pipe
**      status          I/O status
**      hCmpltThread    handle to thread which will have APC queued
**      dwNumberXfer    counter for received message bytes
**      pIocb           pointer to I/O control block structure
**      completion      Pointer to completion function.
**
** History:
**      25-Jul-97 (fanra01)
**          Created.
**      03-Dec-1999 (fanra01)
**          Add completion to the structure.
*/

typedef struct tagIOPARMS
{
    SVC_PARMS   *pSvcParms;
    ASSOC_IPC   *pAsipc;
    HANDLE      hFd;
    STATUS      status;
    HANDLE      hCmpltThread;
    DWORD       dwNumberXfer;
    ASYNC_IOCB  *pIocb;
    VOID (WINAPI *completion)();
}IOPARMS, *PIOPARMS;

/* 
** Exported Function prototypes.
*/

BOOL	 CreateAsc(ASSOC_IPC * asipc, char *name );
VOID 	 CloseAsc(ASSOC_IPC * asipc);
VOID    GetAscInfo(ASSOC_IPC *, i4 *outbuf_size, i4 *inbuf_size );
STATUS 	 OpenAsc(ASSOC_IPC * asipc, char *name, DWORD timeout );
DWORD	AscWait( ASSOC_IPC *asipc, int timeout );
BOOL	DetectServer( char *name );
ULONG	IIReadFile( ASSOC_IPC *asipc, HANDLE hFile, LPVOID lpBuffer, DWORD nNumberToRead,
			LPDWORD	lpNumberRead, LPOVERLAPPED lpOverlapped, DWORD timout );
ULONG	IIWriteFile( ASSOC_IPC *asipc, HANDLE hFile, LPVOID lpBuffer, DWORD nNumberToWrite,
			LPDWORD	lpNumberWritten, LPOVERLAPPED lpOverlapped );
i4		IPCsave( ASSOC_IPC *asipc, ULONG *buff );
BOOL		IPCrestore( ASSOC_IPC *asipc, ULONG *buff );

FUNC_EXTERN VOID    GCrequest95( SVC_PARMS * svc_parms );

FUNC_EXTERN VOID    GClisten95( SVC_PARMS * svc_parms );

FUNC_EXTERN VOID    GClisten95_2( void );

FUNC_EXTERN VOID    GCreceive95( PIOPARMS pIoparms );

FUNC_EXTERN VOID    GCsend95( PIOPARMS pIoparms );

FUNC_EXTERN VOID    GCAnonPipeReceive( HANDLE fd, SVC_PARMS *svc_parms,
                                       ASSOC_IPC *asipc, ASYNC_IOCB *iocb,
                                       BOOL expedited,
                                       VOID (WINAPI *completion)(DWORD, DWORD, LPOVERLAPPED) );

FUNC_EXTERN DWORD   GCAnonPipeSend( HANDLE fd, SVC_PARMS *svc_parms,
                                    ASSOC_IPC *asipc, ASYNC_IOCB *iocb,
                                    VOID (WINAPI *completion)(DWORD, DWORD, LPOVERLAPPED) );

FUNC_EXTERN VOID    GCNamePipeReceive( HANDLE fd, SVC_PARMS *svc_parms,
                                       ASSOC_IPC *asipc, ASYNC_IOCB *iocb,
                                       BOOL expedited,
                                       VOID (WINAPI *completion)(DWORD, DWORD, LPOVERLAPPED) );

FUNC_EXTERN DWORD   GCNamePipeSend( HANDLE fd, SVC_PARMS *svc_parms,
                                    ASSOC_IPC *asipc, ASYNC_IOCB *iocb,
                                    VOID (WINAPI *completion)(DWORD, DWORD, LPOVERLAPPED) );

FUNC_EXTERN VOID    GCreceiveCompletion (SVC_PARMS *svc_parms,
                                         DWORD *lpError,
                                         DWORD dwNumberXfer);
