/********************************************************************
**
**  Copyright (C) 1995 Ingres Corporation
**
**    Project : CA/OpenIngres Visual DBA
**
**    Source : rmcmd.h
**    defines for remote commands
**
**
** History:
**
**	25-Jan-1996  (boama01)
**		Exclude "extern" entry points (client-only) from VMS, so
**		that "globalref"-handling pragma is unnecessary.
**  22-Oct-2003 (schph01)
**      (SIR #111153) now have rmcmd run under any user id
**  25-Aug-2010 (drivi01) Bug #124306
**      Remove hard coded length for RMCMDLINELEN.
**      Rmcmd buffer should be able to handle long ids.
********************************************************************/

#ifndef __RMCMD_INCLUDED__
#define __RMCMD_INCLUDED__

#include "dba.h"
#include <iicommon.h>
#include <compat.h>
#define RES_NOTINGRES 10000

#define RES_HDL_ERROR (-1)
#define RES_HDL_NOSVR (-2)

#define RMCMDSTATUS_ERROR           (-1)
#define RMCMDSTATUS_SENT              0
#define RMCMDSTATUS_ACCEPTED          1
#define RMCMDSTATUS_FINISHED          2
#define RMCMDSTATUS_ACK               3
#define RMCMDSTATUS_SERVERPRESENT     4
#define RMCMDSTATUS_REQUEST4TERMINATE 5

#define MAX_USER_NAME_LEN (256 + 1)

#define RMCMDLINELEN (DB_MAXNAME*4 + 3)
    
typedef struct rmcmdparams {
   int isession;
   int lastlineread;
   int rmcmdhdl;
   BOOL bSessionReleased;
   UCHAR Title[RMCMDLINELEN];
} RMCMDPARAMS, * LPRMCMDPARAMS;

#ifndef VMS
extern int CreateRemoteCmdObjects(char * lpVirtNode);

/* client side functions */

extern int execrmcmd(char * lpVirtNode, char *lpCmd, char * lpTitle);
extern int GetRmcmdOutput(LPRMCMDPARAMS lpcmdparams,int maxlines,LPUCHAR *resultlines);
extern int PutRmcmdInput(LPRMCMDPARAMS lpcmdparams,LPUCHAR lpInputString, BOOL bNoReturn);
extern int GetRmCmdStatus(LPRMCMDPARAMS lpcmdparams);
extern int LaunchRemoteProcedure(char * lpVirtNode, char *lpcmd, int * pihdl);
#endif /* VMS */

#endif /*__RMCMD_INCLUDED__ */
