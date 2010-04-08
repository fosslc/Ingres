/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : remotecm.h, header file
**    Project  : IJA 
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : SQL file, use for remote command (RMCMD)

** History :
** 19-Jun-2000 (uk$so01)
**    created
**    (copy all codes of rmcmd client that previously, noifr01 added
**    into the file ijadmlll.scc)
** 05-Sep-2003 (uk$so01)
**    SIR #106648, Integrate libraries libwctrl.lib, libingll.lib in Ija
** 02-Jun-2005 (lakvi01)
**    BUG 114599, added RES_HDL_NOTGRANTED as an error when the user does
**    not have priviliges for rmcmd.
**/


#if !defined (REMOTE_RMCMDCLIENT_HEADER)
#define REMOTE_RMCMDCLIENT_HEADER


/* rmcmd definitions */

#define RES_NOTINGRES 10000

#define RES_HDL_ERROR       (-1)
#define RES_HDL_NOSVR       (-2)
#define RES_HDL_NOTGRANTED  (-3)

#define RMCMDSTATUS_ERROR           (-1)
#define RMCMDSTATUS_SENT              0
#define RMCMDSTATUS_ACCEPTED          1
#define RMCMDSTATUS_FINISHED          2
#define RMCMDSTATUS_ACK               3
#define RMCMDSTATUS_SERVERPRESENT     4
#define RMCMDSTATUS_REQUEST4TERMINATE 5

#define MAXOBJECTNAME (32+1+32+1)

#define RMSUPERUSER "INGRES" /*TO BE FINISHED to avoid multiple occurences*/

#define RMCMDLINELEN (256 + 1)
    
typedef struct tagRMCMDPARAMS {
   LPARAM lSession;
   int lastlineread;
   int lastlinesent;
   int lastlinesentread;
   TCHAR RmcmdObjects_Owner[33];
   int rmcmdhdl;
   UCHAR Title[RMCMDLINELEN];
   BOOL bHasRmcmdEvents;
   BOOL bOutputEvents;

} RMCMDPARAMS, FAR * LPRMCMDPARAMS;

extern int CreateRemoteCmdObjects(char * lpVirtNode);

/* client side functions */

extern int  execrmcmd (char * lpVirtNode, char *lpCmd, char * lpTitle);
extern int  execrmcmd1(char * lpVirtNode, char *lpCmd, char * lpTitle, BOOL bClose);
extern int  GetRmcmdOutput(LPRMCMDPARAMS lpcmdparams,int maxlines,char **resultlines);
extern int  PutRmcmdInput(LPRMCMDPARAMS lpcmdparams,char * lpInputString, BOOL bNoReturn);
extern int  GetRmCmdStatus(LPRMCMDPARAMS lpcmdparams);
extern int  LaunchRemoteProcedure1(char * lpVirtNode, char *lpcmd, LPRMCMDPARAMS lprmcmdparams, BOOL bOutputEvents);

extern int  CleanupRemoteCommandTuples(LPRMCMDPARAMS lpcmdparams);
extern int  ReleaseRemoteProcedureSession(LPRMCMDPARAMS lprmcmdparams);
extern BOOL HasRmcmdEventOccured(LPRMCMDPARAMS lprmcmdparams);
extern BOOL Wait4RmcmdEvent(LPRMCMDPARAMS lprmcmdparams, int nsec);

extern CString & GetTraceOutputBuf();
extern BOOL ExecRmcmdInBackground(char * lpVirtNode, char *lpCmd, CString InputLines);
extern LPTSTR GetFirstTraceLine();
extern LPTSTR GetNextTraceLine();

#define CMD_OK_IF_NOOUTPUT  0
#define CMD_FASTLOAD        1
#define CMD_DBLEVEL_AUDITDB 2
extern BOOL ExecRmcmdTestOutPut (LPTSTR  lpVirtNode, LPTSTR lpCmd, int iCmdType, CString & csCmdOutput);



#endif // REMOTE_RMCMDCLIENT_HEADER
