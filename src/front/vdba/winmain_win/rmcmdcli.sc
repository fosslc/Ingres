/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project : CA/OpenIngres Visual DBA
**
**    Source : rmcmdcli.sc
**    Client part of remote commands
**
**    Author : Francois Noirot-Nerin
**
** History:
**	09-dec-1998 (somsa01)
**	    Converted all queries of the form "ingres." to dynamic SQL,
**	    using the owner stored in iirelation. This is because platforms
**	    such as VMS may not have "ingres" as the owner of the
**	    installation.
**  30-dec-1998 (noifr01)
**      -extended the previous change to new queries that had been added 
**       directly in the "main" branch, and therefore were not covered by the
**       cross-integration from the oping20 branch.
**      -took this opportunity to optimize the speed / minimize resource consumption
**       by executing only once per executed remote command the request providing
**       the owner stored in iirelation [and storing it in the rmcmdparams structure
**       attached to the remote command execution request]
**  04-Apr-2000 (noifr01)
**     (bug 101430) manage the fact that now, rows returned by rmcmdout may not
**     be full lines up to a carriage return.
**     manage the new column that indicates whether the carriage return is to
**     be added or not. 
**     detect, when connected to older versions of Ingres, if the patch containing
**     the fix was applied to rmcmd: if not, consider a C.R. is to be appendend
**     to the line consistently with the logic before the fix
**  03-Jan-2001 (noifr01)
**     (SIR #103625) rmcmd now works against imadb rather than iidbdb. Connect to
**     iidbdb only if rmcmd tables not found in imadb
**  26-Mar-2001 (noifr01)
**   (sir 104270) removal of code for managing Ingres/Desktop
**  30-Mar-2001 (noifr01)
**       (sir 104378) differentiation of II 2.6.
**  23-Sep-2002 (schph01)
**     (bug 108760) In LaunchRemoteProcedure1() function, add lowercase function in "where" clause to compare
**     with the view name 'remotecmdview'.
**  23-Apr-2003 (schph01)
**     bug 109832 add function to retrieve the current owner of the Rmcmd
**     Object.
**  22-Oct-2003 (schph01)
**     (SIR 111153) with DBMS ingres 2.65 or higher the rmcmd object owner is
**     always '$ingres',skip the sql statement to get the rmcmd objects owner.
**  23-Jan-2004 (schph01)
**     (sir 104378) detect version 3 of Ingres, for managing
**     new features provided in this version. replaced references
**     to 2.65 with refereces to 3  in #define definitions for
**     better readability in the future
** 22-Sep-2004 (uk$so01)
**    BUG #113104 / ISSUE 13690527, Add the functionality
**    to allow vdba to request the IPM.OCX and SQLQUERY.OCX
**    to know if there is an SQL session through a given vnode
**    or through a given vnode and a database.
** 28-Oct-2004 (noifr01)
**    (bug 113217) (fix of deadlock situations in remotecmd tables):
**    - set the lockmode of the session to exclusive
**    - simplified the query sequence through usage of the value
**      returned by the launchremotecmd procedure
** 31-May-2005 (lakvi01)
**    When trying to create a database without rmcmd privileges
**    LaunchRemoteProcedure1() should return RES_HDL_NOTGRANTED.
** 17-Mar-2009 (drivi01)
**    Cleaned up warning in this file in efforts to port to
**    Visual Studio 2008.  
*/

#include <time.h>
#include <stdio.h>
#include <process.h>
#include "dba.h"
#include "rmcmd.h"
#include "dbaginfo.h"
#include "error.h"
#include "msghandl.h"
#include "dll.h"
#include "dgerrh.h"
#include "resource.h"
#include "extccall.h"


// includes SQL
exec sql include sqlca;
exec sql include sqlda;


int GetRmcmdOutput(LPRMCMDPARAMS lpcmdparams,int maxlines,LPUCHAR *resultlines, BOOL *resultNoCRs)
{
  int i;
  long l1;
  exec sql begin declare section;
    char buf[RMCMDLINELEN];
    int prevordno,ordno,hdl;
    char sqlstmt[400];
	char bufNoRC[10];
  exec sql end declare section;
  //ActivateSession(lpcmdparams->isession);

  if (lpcmdparams->lastlinesent>lpcmdparams->lastlinesentread) {
    i         = 0;
    hdl       = lpcmdparams->rmcmdhdl;
    prevordno = lpcmdparams->lastlinesentread;

    sprintf(sqlstmt,
	"select orderno, instring from %s.remotecmdinview where handle = %d and orderno > %d order by orderno",
	lpcmdparams->RmcmdObjects_Owner, hdl, prevordno);

    exec sql execute immediate :sqlstmt into :ordno, :buf;
    exec sql begin;
      ManageSqlError(&sqlca); // Keep track of the SQL error if any
      lpcmdparams->lastlinesentread++;
      if (lpcmdparams->lastlinesentread!=ordno)
        myerror(ERR_RMCLI);
      x_strcpy(resultlines[i],buf);
	  resultNoCRs[i] = FALSE;
      i++;
      if (i>=maxlines) {
        exec sql endselect;
      }
    exec sql end;
    ManageSqlError(&sqlca); // Keep track of the SQL error if any
    l1=sqlca.sqlcode;
    if (l1<0) {
      // "Error while accessing RMCMD objects"
      MessageWithHistoryButton(GetFocus(),ResourceString(IDS_ERR_ACCESS_RMCMD_OBJ) ) ;
    }
    exec sql commit;
    return i;
  }

  i=0;
  prevordno = lpcmdparams->lastlineread;
  hdl       = lpcmdparams->rmcmdhdl;

  /*********** get returned line(s) *******************/
	if (lpcmdparams->bHasNoCRPatch) {
		sprintf(sqlstmt,
		"select orderno, outstring, nocr from %s.remotecmdoutview where handle = %d and orderno > %d order by orderno",
		lpcmdparams->RmcmdObjects_Owner, hdl, prevordno);
	}
	else {
		sprintf(sqlstmt,
		"select orderno, outstring, 'N' from %s.remotecmdoutview where handle = %d and orderno > %d order by orderno",
		lpcmdparams->RmcmdObjects_Owner, hdl, prevordno);
	}

  exec sql execute immediate :sqlstmt into :ordno, :buf, :bufNoRC;
  exec sql begin;
    ManageSqlError(&sqlca); // Keep track of the SQL error if any
    lpcmdparams->lastlineread++;
    if (lpcmdparams->lastlineread!=ordno)
      myerror(ERR_RMCLI);
    x_strcpy(resultlines[i],buf);
	if (bufNoRC[0]=='Y')
		resultNoCRs[i]=TRUE;
	else
		resultNoCRs[i]=FALSE;


    i++;
    if (i>=maxlines) {
      exec sql endselect;
    }
  exec sql end;
  ManageSqlError(&sqlca); // Keep track of the SQL error if any
  l1=sqlca.sqlcode;
  if (l1<0) {
      //"Unable to get the Output of the Command"
      MessageWithHistoryButton(GetFocus(),ResourceString(IDS_ERR_RMCMD_OUTPUT_COMMAND) ) ;
  }
  exec sql commit;
  if (i)
    return i;

  /*********** if no line returned, check if command finished***********/
  if (GetRmCmdStatus(lpcmdparams)==RMCMDSTATUS_FINISHED) {
    /* no line returned, and command finished. Set status to 3 for cleanup */
    // TO BE FINISHED create a procedure for that
    return (-1);
  }
  return 0;
}

// delete the tuples on the server side
int CleanupRemoteCommandTuples(LPRMCMDPARAMS lpcmdparams)
{
  exec sql begin declare section;
    int hdl;
    char sqlstmt[400];
  exec sql end declare section;

  hdl = lpcmdparams->rmcmdhdl;


  sprintf(sqlstmt, "delete from %s.remotecmdinview where handle = %d",
	lpcmdparams->RmcmdObjects_Owner, hdl);
  exec sql execute immediate :sqlstmt;
  ManageSqlError(&sqlca); // Keep track of the SQL error if any
  

  if (sqlca.sqlcode < 0) {
    exec sql rollback;
    goto ercleanup;
  }

  sprintf(sqlstmt, "delete from %s.remotecmdview where handle = %d",
	lpcmdparams->RmcmdObjects_Owner, hdl);
  exec sql execute immediate :sqlstmt;
  ManageSqlError(&sqlca); // Keep track of the SQL error if any
  

  if (sqlca.sqlcode < 0) {
    exec sql rollback;
    goto ercleanup;
  }

  sprintf(sqlstmt, "delete from %s.remotecmdoutview where handle = %d",
	lpcmdparams->RmcmdObjects_Owner, hdl);
  exec sql execute immediate :sqlstmt;
  ManageSqlError(&sqlca); // Keep track of the SQL error if any
  if (sqlca.sqlcode < 0) {
    exec sql rollback;
    goto ercleanup;
  }

  exec sql commit;
  return RES_SUCCESS;

ercleanup:
  //"Unable to remove the request from the rmcmd system tables.\nPlease do it manually."
  MessageWithHistoryButton(GetFocus(),ResourceString(IDS_ERR_RMCMD_REMOVE) ) ;
  return RES_ERR;
}

int PutRmcmdInput(LPRMCMDPARAMS lpcmdparams,LPUCHAR lpInputString, BOOL bNoReturn)
{
  exec sql begin declare section;
    char sqlstmt[400];
    int prevordno,ordno,hdl,cnt1,cnt2;
  exec sql end declare section;
  int iret=RES_ERR;

  //ActivateSession(lpcmdparams->isession);

  prevordno = lpcmdparams->lastlinesent;
  hdl       = lpcmdparams->rmcmdhdl;

  ordno=-1;

  /*********** get last line sent *******************/
  sprintf(sqlstmt,
	"select orderno from %s.remotecmdinview where handle = %d order by orderno desc",
	lpcmdparams->RmcmdObjects_Owner, hdl);
  exec sql execute immediate :sqlstmt into :ordno;
  ManageSqlError(&sqlca); // Keep track of the SQL error if any

  if (lpcmdparams->lastlinesent!=ordno) {
     myerror(ERR_RMCLI);
     goto endin;
  }

  ordno++;

  sprintf(sqlstmt, "select count(*) from %s.remotecmdinview", lpcmdparams->RmcmdObjects_Owner);
  exec sql execute immediate :sqlstmt into :cnt1;
  ManageSqlError(&sqlca); // Keep track of the SQL error if any
	if (sqlca.sqlcode < 0) 
     goto endin;

  sprintf(sqlstmt,
	"execute procedure %s.sendrmcmdinput(inputhandle=%d, inputstring='%s')",
	lpcmdparams->RmcmdObjects_Owner,hdl,lpInputString);
  exec sql execute immediate :sqlstmt;
  ManageSqlError(&sqlca); // Keep track of the SQL error if any
	if (sqlca.sqlcode < 0)   
     goto endin;

  sprintf(sqlstmt, "select count(*) from %s.remotecmdinview", lpcmdparams->RmcmdObjects_Owner);
  exec sql execute immediate :sqlstmt into :cnt2;
  ManageSqlError(&sqlca); // Keep track of the SQL error if any
	if (sqlca.sqlcode < 0) 
     goto endin;
  if (cnt2!=cnt1+1) {
     myerror(ERR_RMCLI);
     goto endin;
  }
  if (lpcmdparams->bHasRmcmdEvents) {
    wsprintf(sqlstmt,"raise dbevent %s.rmcmdnewinputline",lpcmdparams->RmcmdObjects_Owner);
    exec sql execute immediate :sqlstmt;
  }
  iret = RES_SUCCESS;
  lpcmdparams->lastlinesent = ordno ;

endin: 
  if (iret==RES_SUCCESS)
     exec sql commit;
  else 
     exec sql rollback;

  return 0;
}

int GetRmCmdStatus(LPRMCMDPARAMS lpcmdparams)
{
  exec sql begin declare section;
    int hdl,istatus;
    char sqlstmt[400];
  exec sql end declare section;

  //ActivateSession(lpcmdparams->isession);

  hdl       = lpcmdparams->rmcmdhdl;
  istatus   = RMCMDSTATUS_ERROR;


  sprintf(sqlstmt, "select status from %s.remotecmdview where handle = %d",
	lpcmdparams->RmcmdObjects_Owner, hdl);
  exec sql execute immediate :sqlstmt into :istatus;
  ManageSqlError(&sqlca); // Keep track of the SQL error if any
  exec sql commit;
  return istatus;
}

int execrmcmd1(char * lpVirtNode, char *lpCmd, char * lpTitle, BOOL bClose)
{
  RMCMDPARAMS rmcmdparams;
  int ires;

  if (bClose)  {
     if (DBACloseNodeSessions(lpVirtNode,TRUE,TRUE)!=RES_SUCCESS) {
       //
       // Failure in closing sessions for this node. Probably active SQL Test window on this node
       //
       ErrorMessage ((UINT)IDS_E_CLOSING_SESSION2, RES_ERR);
       return -1;
     }
     if (ExistSqlQueryAndIpm((LPCTSTR)lpVirtNode, NULL))
     {
        ErrorMessage ((UINT)IDS_E_CLOSING_SESSION2, RES_ERR);
        return -1;
     }
  }

//  TS_MessageBox( TS_GetFocus(), lpCmd, lpTitle, MB_OK | MB_TASKMODAL);

  fstrncpy(rmcmdparams.Title,lpTitle, RMCMDLINELEN);
  ires=LaunchRemoteProcedure(lpVirtNode,lpCmd,&rmcmdparams);
  if (ires == RES_HDL_NOTGRANTED) {
	//
	// Privileges absent for the user
	//
	ErrorMessage ((UINT)IDS_E_RMCMD_PRIVILEGES, RES_ERR);
	return ires;
  }
  if (ires<0) {
    //
    // Failure to launch remote command
    //
    ErrorMessage ((UINT)IDS_E_LAUNCHING_RMCMD, RES_ERR);
    return ires;
  }
  rmcmdparams.rmcmdhdl=ires;
  rmcmdparams.bSessionReleased = FALSE;

  ESL_RemoteCmdBox((LPRMCMDPARAMS)&rmcmdparams);

  if (!rmcmdparams.bSessionReleased) 
    ReleaseRemoteProcedureSession(&rmcmdparams);

  return rmcmdparams.rmcmdhdl;
}
int execrmcmd(char * lpVirtNode, char *lpCmd, char * lpTitle)
{
  return execrmcmd1(lpVirtNode, lpCmd,lpTitle, TRUE);
}

int ReleaseRemoteProcedureSession(LPRMCMDPARAMS lprmcmdparams)
{
    exec sql begin declare section;
       char sqlstmt[400];
    exec sql end declare section;
	int ires=RES_SUCCESS;
	int ires1;
    if (lprmcmdparams->bHasRmcmdEvents) {
	    if (lprmcmdparams->bOutputEvents) {
            wsprintf(sqlstmt, "remove dbevent %s.rmcmdnewoutputline", lprmcmdparams->RmcmdObjects_Owner);
            exec sql execute immediate :sqlstmt;
		    ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (sqlca.sqlcode<0)
				ires=RES_ERR;
		}
        wsprintf(sqlstmt, "remove dbevent %s.rmcmdcmdend", lprmcmdparams->RmcmdObjects_Owner);
        exec sql execute immediate :sqlstmt;
	    ManageSqlError(&sqlca); // Keep track of the SQL error if any
		if (sqlca.sqlcode<0)
			ires=RES_ERR;
	}
    lprmcmdparams->bSessionReleased = TRUE;
	lprmcmdparams->bHasRmcmdEvents  = FALSE;
	lprmcmdparams->bOutputEvents    = FALSE;
	
    ires1=ReleaseSession(lprmcmdparams->isession,RELEASE_COMMIT);
	if (ires1!=RES_SUCCESS)
		ires=ires1;
	return ires;
}

int LaunchRemoteProcedure1(char * lpVirtNode, char *lpcmd, LPRMCMDPARAMS lprmcmdparams, BOOL bOutputEvents)
{
  exec sql begin declare section;
    char connectname[MAXOBJECTNAME];
    char sqlstmt[400];
    char procname[100];
    int handl;
    long cnt2,cnttemp;
    char owner[33];
  exec sql end declare section;
  int iret, i;
  char * pdbname;
  lprmcmdparams->lastlineread     = -1;
  lprmcmdparams->lastlinesent     = -1;
  lprmcmdparams->lastlinesentread = -1;
  lprmcmdparams->bOutputEvents    = FALSE;
  

  for (i=0,pdbname = "imadb";i<2;i++,pdbname="iidbdb") {
    if (!lpVirtNode || !(*lpVirtNode))
      x_strcpy(connectname,pdbname);
    else 
      sprintf(connectname,"%s::%s",lpVirtNode,pdbname);

    // Corrected 26/10/95 : READLOCK needed (instead of NOREADLOCK)
    iret = Getsession(connectname,SESSION_TYPE_INDEPENDENT, &(lprmcmdparams->isession));
    if (iret !=RES_SUCCESS)
      return RES_HDL_ERROR;

    exec sql set lockmode session where readlock=exclusive;
    if (sqlca.sqlcode < 0)
    {
         ReleaseSession(lprmcmdparams->isession, RELEASE_ROLLBACK);
         return RES_HDL_ERROR;
    }

    handl=RES_HDL_ERROR;
    if ( GetOIVers() >= OIVERS_30) {
      lstrcpy(owner,"$ingres");
      lstrcpy(lprmcmdparams->RmcmdObjects_Owner,owner);
      break;
    }

    /*
    ** Get the owner of the rmcmd objects from iirelation.
    */
    exec sql repeated select relowner
           into :owner
           from iirelation
           where lowercase(relid)='remotecmdview';
    ManageSqlError(&sqlca); // Keep track of the SQL error if any
    if (sqlca.sqlcode < 0 || sqlca.sqlcode==100)  {
      if (i==0) {
         ReleaseSession(lprmcmdparams->isession, RELEASE_ROLLBACK);
         continue;
      }
      goto end;
    }
    suppspace(owner);
    lstrcpy(lprmcmdparams->RmcmdObjects_Owner,owner);
    break;
  }


	if ( GetOIVers() >= OIVERS_25) {
		lprmcmdparams->bHasNoCRPatch = TRUE;
	}
	else {
		sprintf(sqlstmt, "select count(*) from iicolumns where lowercase(table_name)='remotecmdout' and lowercase(column_name)='nocr'");
		exec sql execute immediate :sqlstmt into :cnttemp;
		ManageSqlError(&sqlca); // Keep track of the SQL error if any
		if (sqlca.sqlcode < 0)
			goto end;
		if (cnttemp>0)
			lprmcmdparams->bHasNoCRPatch = TRUE;
		else
			lprmcmdparams->bHasNoCRPatch = FALSE;
	}


  STprintf(procname,"%s.launchremotecmd",owner);
  STcopy(lpcmd, sqlstmt);

  exec sql execute procedure :procname(rmcmd=:sqlstmt) into :handl;
  ManageSqlError(&sqlca); // Keep track of the SQL error if any
  if (sqlca.sqlcode < 0 || handl<0) {
    if (sqlca.sqlcode < 0) 
	  {
      if (sqlca.sqlcode == -34000) 
        {
        handl=RES_HDL_NOTGRANTED;
        }
      else
	    handl= RES_HDL_ERROR;
      }
	goto end;
  }
  

  exec sql repeated select count(*) into :cnt2 
            from iievents where event_name='rmcmdnewoutputline' and event_owner=:owner;
  if (sqlca.sqlcode < 0) {
     handl=RES_HDL_ERROR;
     goto end;
  }
  lprmcmdparams->bHasRmcmdEvents=FALSE;
  if (cnt2>0) {
    wsprintf(sqlstmt, "register dbevent %s.rmcmdcmdend", owner);
    exec sql execute immediate :sqlstmt;
    ManageSqlError(&sqlca); // Keep track of the SQL error if any
	if (sqlca.sqlcode>=0) {
		if (bOutputEvents) {
            wsprintf(sqlstmt, "register dbevent %s.rmcmdnewoutputline", owner);
            exec sql execute immediate :sqlstmt;
            if (sqlca.sqlcode<0) {
                wsprintf(sqlstmt, "remove dbevent %s.rmcmdcmdend", owner);
                exec sql execute immediate :sqlstmt;
			    ManageSqlError(&sqlca); // Keep track of the SQL error if any
			}
			else {
				lprmcmdparams->bOutputEvents  = TRUE;
		        lprmcmdparams->bHasRmcmdEvents= TRUE;
			}
		}
		else 
	        lprmcmdparams->bHasRmcmdEvents=TRUE;
	}
	if (!lprmcmdparams->bHasRmcmdEvents) {
		 exec sql commit;
	     handl=RES_HDL_ERROR;
	     goto end;
	}
  }
  exec sql commit;
  if (sqlca.sqlcode < 0) {
     handl=RES_HDL_ERROR;
     goto end;
  }

end:
  if (handl<0)
     ReleaseSession(lprmcmdparams->isession, RELEASE_ROLLBACK);
  return handl;
}

int LaunchRemoteProcedure(char * lpVirtNode, char *lpcmd, LPRMCMDPARAMS lprmcmdparams)
{
	return  LaunchRemoteProcedure1(lpVirtNode, lpcmd, lprmcmdparams,TRUE);
}

BOOL HasRmcmdEventOccured(LPRMCMDPARAMS lprmcmdparams)
{
   exec sql begin declare section;
    char v_name [MAXOBJECTNAME];
    char v_text [256];
    char v_time[60];
    char v_owner[MAXOBJECTNAME];
   exec sql end declare section;

   if (!lprmcmdparams->bHasRmcmdEvents)
		return TRUE;

   exec sql get dbevent;
   ManageSqlError(&sqlca);

   exec sql inquire_sql(:v_name=dbeventname,:v_owner=dbeventowner,
                        :v_time=dbeventtime,:v_text =dbeventtext);
   ManageSqlError(&sqlca);

   exec sql commit;
   suppspace(v_name);
   if (v_name[0]=='\0')
	  return FALSE;
   else
      return TRUE;
}

BOOL Wait4RmcmdEvent(LPRMCMDPARAMS lprmcmdparams, int nsec)
{
   exec sql begin declare section;
    char v_name [MAXOBJECTNAME];
    char v_text [256];
    char v_time[60];
    char v_owner[MAXOBJECTNAME];
	int nsec1=nsec;
   exec sql end declare section;

   if (!lprmcmdparams->bHasRmcmdEvents) {
		Sleep(nsec * 1000 );
		return FALSE;
   }

   exec sql get dbevent with wait=:nsec1;
   ManageSqlError(&sqlca);
    exec sql inquire_sql(:v_name=dbeventname,:v_owner=dbeventowner,
                        :v_time=dbeventtime,:v_text =dbeventtext);
   ManageSqlError(&sqlca);

   exec sql commit;
   suppspace(v_name);
   if (v_name[0]=='\0')
	  return FALSE;

   return TRUE;
}

int Mysystem(char* lpCmd)
{
//#ifdef WIN32
   char buf [200];

   BOOL bSuccess;
   DWORD dwExit;
   STARTUPINFO StartupInfo;
   LPSTARTUPINFO lpStartupInfo;
   PROCESS_INFORMATION ProcInfo;
   lpStartupInfo = &StartupInfo;
   memset(lpStartupInfo, 0, sizeof(STARTUPINFO));
   StartupInfo.cb = sizeof(STARTUPINFO);
   StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
   StartupInfo.wShowWindow = SW_HIDE;

   if(x_strnicmp(lpCmd, "iicopy", x_strlen("iicopy")))   {
      if (IsNT())
         wsprintf (buf, "cmd.exe /c %s", lpCmd);
      else
         wsprintf (buf, "command.com /c %s", lpCmd);
   }
   else
     x_strcpy(buf, lpCmd);

   bSuccess = CreateProcess(
       NULL,            // pointer to name of executable module
       buf,             // pointer to command line string
       NULL,            // pointer to process security attributes
       NULL,            // pointer to thread security attributes
       FALSE,           // handle inheritance flag
       CREATE_NEW_CONSOLE,// creation flags
       NULL,            // pointer to new environment block
       NULL,            // pointer to current directory name
       lpStartupInfo,   // pointer to STARTUPINFO
       &ProcInfo        // pointer to PROCESS_INFORMATION
       );
   if (!bSuccess) {//"CreateProcess Failed"
       TS_MessageBox (TS_GetFocus(), ResourceString(IDS_ERR_CREATE_PROCESS), "Error", MB_OK);
       return (-1);
   }
   while(1)  {
      Sleep(1000);
      if (GetExitCodeProcess(ProcInfo.hProcess, &dwExit)) {
        if ( dwExit!=STILL_ACTIVE )
            break;
      }
   }
   return (1);
//#endif
}

int GetRmcmdObjects_Owner(char *lpVnodeName, char *lpOwner)
{
  exec sql begin declare section;
    char connectname[MAXOBJECTNAME];
    char owner[33];
  exec sql end declare section;
  char * pdbname;
  int isession,i,iret, ires = RES_ERR;

  owner[0] = '\0';

  for (i=0,pdbname = "imadb";i<2;i++,pdbname="iidbdb") {
    if (!lpVnodeName || !(*lpVnodeName))
      x_strcpy(connectname,pdbname);
    else 
      sprintf(connectname,"%s::%s",lpVnodeName,pdbname);

    iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &isession);
    if (iret !=RES_SUCCESS)
      return RES_ERR;

    /*
    ** Get the owner of the rmcmd objects from iirelation.
    */
    exec sql repeated select relowner
           into :owner
           from iirelation
           where lowercase(relid)='remotecmdview';
    ManageSqlError(&sqlca); // Keep track of the SQL error if any
    if (sqlca.sqlcode < 0 || sqlca.sqlcode==100)  {
      if (i==0) {
         ReleaseSession(isession, RELEASE_ROLLBACK);
         continue;
      }
      goto end;
    }
    suppspace(owner);
    lstrcpy(lpOwner,owner);
    ires = RES_SUCCESS;
    break;
  }

end:
  if (ires != RES_SUCCESS)
    ReleaseSession(isession, RELEASE_ROLLBACK);
  else
    ReleaseSession(isession, RELEASE_COMMIT);

  return ires;
}



