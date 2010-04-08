/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
**
**  Project : libmon library for the Ingres Monitor ActiveX control
**
**    Source : rmcmdcli.sc
**    client part for rmcmd
**
** History
**
**	22-Nov-2001 (noifr01)
**      extracted and adapted from the winmain\rmcmdcli.sc source
**  22-Oct-2003 (schph01)
**     (SIR 111153) with DBMS ingres 2.65 or higher the rmcmd object owner is
**     always '$ingres',skip the sql statement to get the rmcmd objects owner.
**  23-Jan-2004 (schph01)
**     (sir 104378) detect version 3 of Ingres, for managing
**     new features provided in this version. replaced references
**     to 2.65 with refereces to 3  in #define definitions for
**     better readability in the future
**  02-Jun-2005 (lakvi01)
**     (BUG 114599) Made LaunchRemoteProcedure1 return RES_HDL_NOTGRANTED
**	   in case the user is not granted rmcmd privileges.
**  17-Oct-2008 (drivi01)
**	Update LIBMON_Mysystem function to launch processes on Vista
**	via ShellExecuteEx to ensure that privilege level of the
**      application being launched is preserved.
**  02-apr-2009 (drivi01)
**    	In efforts to port to Visual Studio 2008, clean up warnings.
********************************************************************/


#include <time.h>
#include <stdio.h>
#include <tchar.h>
#include "libmon.h"
#include "rmcmd.h"
#include "error.h"

// includes SQL
exec sql include sqlca;
exec sql include sqlda;

extern int GVvista();

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
    if (l1<0) 
	  i=RES_RMCMDCLI_ERR_ACCESS_RMCMDOBJ;
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
	i=RES_RMCMDCLI_ERR_OUTPUT;
  }
  exec sql commit;
  if (i!=0)
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
    int handl;
    long cnt1,cnt2,cnttemp;
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
           where relid='remotecmdview';
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

  // TO BE FINISHED: work around due to esqlc level
  // check previous max value
  sprintf(sqlstmt, "select count(*) from %s.remotecmdview", owner);
  exec sql execute immediate :sqlstmt into :cnt1;
  ManageSqlError(&sqlca); // Keep track of the SQL error if any

	if (sqlca.sqlcode < 0) 
     goto end;

	if ( GetOIVers() >= OIVERS_25) {
		lprmcmdparams->bHasNoCRPatch = TRUE;
	}
	else {
		sprintf(sqlstmt, "select count(*) from iicolumns where table_name='remotecmdout' and column_name='nocr'");
		exec sql execute immediate :sqlstmt into :cnttemp;
		ManageSqlError(&sqlca); // Keep track of the SQL error if any
		if (sqlca.sqlcode < 0)
			goto end;
		if (cnttemp>0)
			lprmcmdparams->bHasNoCRPatch = TRUE;
		else
			lprmcmdparams->bHasNoCRPatch = FALSE;
	}







  sprintf(sqlstmt,
          "execute procedure %s.launchremotecmd(rmcmd='%s')",owner,lpcmd);
  exec sql execute immediate :sqlstmt;
  ManageSqlError(&sqlca); // Keep track of the SQL error if any
	if (sqlca.sqlcode < 0) {
	  if (sqlca.sqlcode == -34000) 
        {
        handl=RES_HDL_NOTGRANTED;
        }
      else
	    handl= RES_HDL_ERROR;   
     goto end;
     }

  sprintf(sqlstmt, "select count(*) from %s.remotecmdview", owner);
  exec sql execute immediate :sqlstmt into :cnt2;
  ManageSqlError(&sqlca); // Keep track of the SQL error if any
	if (sqlca.sqlcode < 0)
     goto end;
  if (cnt2!=cnt1+1) 
     goto end;

  sprintf(sqlstmt, "select max(handle) from %s.remotecmdview", owner);
  exec sql execute immediate :sqlstmt into :handl;
	if (sqlca.sqlcode < 0) {
     handl=RES_HDL_ERROR;
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

static char TRACELINESEP = '\n';
#define RMCMD_MAXLINES 200
static char * TLCurPtr;
static char * pbufrmcmdoutput = NULL;
static BOOL bTraceLinesInit = FALSE;

#ifndef BLD_MULTI /*To make available in Non multi-thread context only */
char * LIBMON_GetTraceOutputBuf()
{
   return pbufrmcmdoutput;
}
void LIBMON_FreeTraceOutputBuf()
{
   if (pbufrmcmdoutput !=NULL)
      free(pbufrmcmdoutput);
   pbufrmcmdoutput = NULL;
}
#endif

static void ReleaseTraceLines()
{
   if (!bTraceLinesInit)
      return;
   LIBMON_FreeTraceOutputBuf();
   bTraceLinesInit=FALSE;
   return;
}

#ifndef BLD_MULTI /*To make available in Non multi-thread context only */
char * LIBMON_GetNextTraceLine()
{
   char * plastchar;
   char * presult;
   char * pc;
   if (!TLCurPtr) {
      ReleaseTraceLines();
      return (char *)NULL;
   }
   presult = TLCurPtr;
   pc = x_strchr(TLCurPtr,TRACELINESEP);
   if (pc) {
      *pc='\0';
      TLCurPtr=pc+1;
      if (TLCurPtr[0]=='\0')
         TLCurPtr=NULL;

   }
   else 
      TLCurPtr=NULL;

   while ( (plastchar = x_strgetlastcharptr(presult))!=NULL ) {
	  char c = *plastchar;
      if (c=='\n' || c=='\r')
         *plastchar ='\0';
      else
         break;
   }
   
   suppspace((LPUCHAR)presult);
   return presult;
}
char *LIBMON_GetNextSignificantTraceLine()
{
   char * presult;
   while ((presult=LIBMON_GetNextTraceLine())!=(char *) NULL) {
      if (*presult!='\0' && x_strncmp(presult,"----",4))
         return presult;
   }
   return (char *)NULL;
}
char * LIBMON_GetFirstTraceLine()
{
   TLCurPtr=pbufrmcmdoutput;
   if (TLCurPtr) {
     if (TLCurPtr[0]=='\0')
       TLCurPtr=NULL;
   }
   bTraceLinesInit=TRUE;
   return LIBMON_GetNextTraceLine();
}


int LIBMON_ExecRmcmdInBackground(char * lpVirtNode, char *lpCmd, char * InputLines)
{
	RMCMDPARAMS rmcmdparams;
	char *CurLine;
	int cpt,cpt2,ires,retval;
	int istatus;
	LPUCHAR feedbackLines[RMCMD_MAXLINES];
	BOOL NoCRs[RMCMD_MAXLINES];
	int iTimerCount = 0;
	int iCountLine = 0;
	bool bServerActive = FALSE;
   BOOL bHasTerminated = FALSE;
   time_t tim1,tim2;
   double d1;
   int iret = RES_SUCCESS;
 
   pbufrmcmdoutput = malloc(x_strlen(InputLines)+2);
   if (!pbufrmcmdoutput) 
      return RES_ERR;
   x_strcpy(pbufrmcmdoutput,InputLines);

	ires=LaunchRemoteProcedure1( lpVirtNode, lpCmd, &rmcmdparams, FALSE);
	if (ires<0) {	// Failure to launch remote command
		return RES_ERR;
	}
	rmcmdparams.rmcmdhdl=ires;
	rmcmdparams.bSessionReleased = FALSE;

   // send input lines

   for (CurLine=LIBMON_GetFirstTraceLine();CurLine;CurLine = LIBMON_GetNextTraceLine()) 
      	PutRmcmdInput(&rmcmdparams, (UCHAR *) CurLine, FALSE);

	// allocate space for future incoming lines
	memset(feedbackLines, 0, sizeof(feedbackLines));
	for (cpt=0; cpt<RMCMD_MAXLINES; cpt++) {
		feedbackLines[cpt] = (LPUCHAR)ESL_AllocMem(RMCMDLINELEN);
		if (!feedbackLines[cpt]) {
			for (cpt2=0; cpt2<cpt; cpt2++)
				ESL_FreeMem(feedbackLines[cpt2]);
			CleanupRemoteCommandTuples(&rmcmdparams);
			if (!rmcmdparams.bSessionReleased)
				ReleaseRemoteProcedureSession(&rmcmdparams);
			return RES_ERR;
		}
	}
   pbufrmcmdoutput = ESL_AllocMem(2);
   if (!pbufrmcmdoutput) 
      return FALSE;
   x_strcpy(pbufrmcmdoutput,(""));

   time( &tim1 );
	while (TRUE)	{
      if (!bHasTerminated) 
         bHasTerminated = Wait4RmcmdEvent(&rmcmdparams, 2);
      retval = GetRmcmdOutput( &rmcmdparams,
								 RMCMD_MAXLINES,
								 feedbackLines,
								 NoCRs);

		if (retval < 0) {	
			if (retval == RES_RMCMDCLI_ERR_ACCESS_RMCMDOBJ || 
			    retval == RES_RMCMDCLI_ERR_OUTPUT
			   )
				iret = retval;
			break;			  
		}
		
		if (retval > 0) {
			int i;
			for (i=0;i<retval;i++)	{
				pbufrmcmdoutput=realloc(pbufrmcmdoutput, x_strlen(pbufrmcmdoutput)+
										x_strlen(feedbackLines[i])+10);
				x_strcat(pbufrmcmdoutput,feedbackLines[i]);
				if (!NoCRs[i])
					 x_strcat(pbufrmcmdoutput,("\r\n"));
			}
			istatus = GetRmCmdStatus(&rmcmdparams);
			if ( istatus == RMCMDSTATUS_ERROR ) {
				iret=RES_ERR;
				break;
			}
		}
		
		if (retval == 0 && !bServerActive) {
         time(&tim2);
         d1= difftime( tim2,tim1);
			if (d1>30) {
				tim1=tim2;
            istatus = GetRmCmdStatus(&rmcmdparams);
				if (istatus==RMCMDSTATUS_ERROR)  {
					iret = RES_RMCMD_CMD_NOT_ACCEPTED;
					break;
				}
				if (istatus!=RMCMDSTATUS_SENT)
					bServerActive=TRUE;
				else  {
					iret=RES_RMCMD_NO_ANSWER_FRM_SVR;
					break;
				}
			}
		}
	} // End while

	if (CleanupRemoteCommandTuples(&rmcmdparams)!=RES_SUCCESS)
		iret |= MASK_RMCMD_ERR_CLEANUP;
	if (!rmcmdparams.bSessionReleased)
      ReleaseRemoteProcedureSession(&rmcmdparams);
	for (cpt=0; cpt<RMCMD_MAXLINES; cpt++)
		ESL_FreeMem(feedbackLines[cpt]);
	return iret;
}
#endif

BOOL IsNT()
{
#ifdef MAINWIN
  // SHOULD NOT BE CALLED
  return FALSE;
#else
  OSVERSIONINFO vsinfo;
  
  vsinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  if (!GetVersionEx (&vsinfo)) {
     myerror(ERR_GW);
     return FALSE;
  }

  if (vsinfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
     return TRUE;
  else
     return FALSE;
#endif
}

int LIBMON_Mysystem(char* lpCmd)
{
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

   if (GVvista())
   {
	SHELLEXECUTEINFO shex;
	char cmd[MAX_PATH];
	char *params;
	int index=-1;
	
	index = strcspn(lpCmd, " ");
	strncpy(cmd, lpCmd, index);
	cmd[index]='\0';
	params=(char *)lpCmd+index;
	
	memset(&shex, 0, sizeof(shex));
	shex.cbSize = sizeof( SHELLEXECUTEINFO );
	shex.fMask = SEE_MASK_NOCLOSEPROCESS;
	shex.hwnd	= NULL;
	shex.lpVerb	= "open";
	shex.lpFile	= cmd;
	shex.lpParameters = params;
	shex.nShow	= SW_HIDE;

	if (!(bSuccess = ShellExecuteEx( &shex )) )
		return (-1);
	while (1)
	{
		Sleep(1000);
		if (GetExitCodeProcess(shex.hProcess, &dwExit))
		{
			if (dwExit != STILL_ACTIVE)
				break;
		}
	}
   }
   else
   {
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
       return (-1);
   }
   while(1)  {
      Sleep(1000);
      if (GetExitCodeProcess(ProcInfo.hProcess, &dwExit)) {
        if ( dwExit!=STILL_ACTIVE )
            break;
      }
   }
   }


   return (1);
}

