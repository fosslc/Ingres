/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project : Visual DBA
**
**    Source : rmcmd.h
**    remote commands
**
** History after 01-01-2000:
**
**  04-Apr-2000 (noifr01)
**     (bug 101430) changed prototype of GetRmcmdOutput() to take care
**     of the the carriage return management.
**     also added a boolean in the rmcmdparams structure, indicating
**     whether the rmcmd tables have the level of this fix (this will
**     allow still to connect to older versions of Ingres, regardless of
**     whether they have the fix or not)
**  23-Apr-2003 (schph01)
**    bug 109832 Add prototype of GetRmcmdObjects_Owner() function.
**  01-June-2005 (lakvi01)
**     Added RES_HDL_NOTGRANTED, for users not having privileges for
**     rmcmd.
**  25-Aug-2010 (drivi01) Bug #124306
**     Remove hard coded length for RMCMDLINELEN.
**     Rmcmd buffer should be able to handle long ids.
*/

#ifndef __RMCMD_INCLUDED__
#define __RMCMD_INCLUDED__

#include <iicommon.h>
#include <compat.h>

#define RES_NOTINGRES 10000

#define RES_HDL_ERROR		(-1)
#define RES_HDL_NOSVR		(-2)
#define RES_HDL_NOTGRANTED	(-3)

#define RMCMDSTATUS_ERROR           (-1)
#define RMCMDSTATUS_SENT              0
#define RMCMDSTATUS_ACCEPTED          1
#define RMCMDSTATUS_FINISHED          2
#define RMCMDSTATUS_ACK               3
#define RMCMDSTATUS_SERVERPRESENT     4
#define RMCMDSTATUS_REQUEST4TERMINATE 5

#define RMSUPERUSER "INGRES" /*TO BE FINISHED to avoid multiple occurences*/

#define RMCMDLINELEN (DB_MAXNAME*4 + 3)
    
typedef struct rmcmdparams {
   int isession;
   int lastlineread;
   int lastlinesent;
   int lastlinesentread;
   TCHAR RmcmdObjects_Owner[33];
   int rmcmdhdl;
   BOOL bSessionReleased;
   UCHAR Title[RMCMDLINELEN];
   BOOL bHasRmcmdEvents;
   BOOL bOutputEvents;
   BOOL bHasNoCRPatch;

} RMCMDPARAMS, FAR * LPRMCMDPARAMS;

extern int CreateRemoteCmdObjects(char * lpVirtNode);

/* client side functions */

extern int  execrmcmd (char * lpVirtNode, char *lpCmd, char * lpTitle);
extern int  execrmcmd1(char * lpVirtNode, char *lpCmd, char * lpTitle, BOOL bClose);
extern int  GetRmcmdOutput(LPRMCMDPARAMS lpcmdparams,int maxlines,LPUCHAR *resultlines, BOOL *resultNoCRs);
extern int  PutRmcmdInput(LPRMCMDPARAMS lpcmdparams,LPUCHAR lpInputString, BOOL bNoReturn);
extern int  GetRmCmdStatus(LPRMCMDPARAMS lpcmdparams);
extern int  LaunchRemoteProcedure (char * lpVirtNode, char *lpcmd, LPRMCMDPARAMS lprmcmdparams);
extern int  LaunchRemoteProcedure1(char * lpVirtNode, char *lpcmd, LPRMCMDPARAMS lprmcmdparams, BOOL bOutputEvents);

extern int  CleanupRemoteCommandTuples(LPRMCMDPARAMS lpcmdparams);
extern int  ReleaseRemoteProcedureSession(LPRMCMDPARAMS lprmcmdparams);
extern BOOL HasRmcmdEventOccured(LPRMCMDPARAMS lprmcmdparams);
extern BOOL Wait4RmcmdEvent(LPRMCMDPARAMS lprmcmdparams, int nsec);

extern int Mysystem(char* lpCmd);
extern int GetRmcmdObjects_Owner(char *lpVnodeName, char *lpOwner);

#endif /*__RMCMD_INCLUDED__ */

