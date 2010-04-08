/********************************************************************
**
**  Copyright (C) 1995, 2005 Ingres Corporation
**
**
**    Project : Ingres Visual DBA
**
**    Source : rmcmdout.sc
**    Manages remote commands output
**
**
** History:
**
**	23-Sep-95 (johna)
**		Added MING hints and moved the 'main' to the start of the 
**		start of the line so that mkming would recognise it.
**  18-Oct-95 (Emmanuel Blattes)
**    For windows NT version :
**    flush the \r character read before the \n
**
**      04-Jan-96 (spepa01)
**          	Add CL headers so that STxcompare()/STlength() can be used.
**	25-Jan-96 (boama01)
**		Added VMS-specific and CL includes; first cut at CL-izing code;
**		see "*CL*" line markings.  Also improved error reporting.
**  25-Mar-1998 (noifr01)
**      -Added usage of dbevents to speed up rmcmd  
**      -a #else (after #ifdef VMS) management was missing in an
**       error management section
**  05-oct-1998 (kinte01)
**	  For VMS only, if a user has the SERVER CONTROL he can setup the
**	  RMCMD objects & start/stop the RMCMD server. 
**	  The above changes meant that all SQL queries referencing 
**	  ingres.<whatever> had to be rewritten in dynamic sql to be
**	  the user setting up RMCMD. For UNIX & NT this will always be
**	  ingres. For VMS this may or may not be ingres but will be the
**	  Ingres System Admin User.
**  15-Jan-1999 (matbe01)
**      Moved include for compat.h before include for stdio.h to resolve
**      compile errors for NCR (nc4_us5).
**  02-Feb-99 (noifr01)
**     specify the ingres server class when connecting, in order to support 
**     the case where the default server class is not ingres
**	16-mar-1999 (somsa01)
**	    Added include of systypes.h to allow the file to build on HP.
**  07-Sep-1999 (marro11)
**	Include system stdio.h before our compat.h; resolves type definition
**	conflicts on hpb_us5.
**  13-Sep-1999 (marro11)
**	Above fix conflicted with matbe01's nc4_us5 one.  Relocated system 
**	includes to after ingres includes (as is typically done).  Resolves
**	compilation problems for both nc4_us5 and hpb_us5.
**  02-Nov-1999 (kinte01)
**	systypes.h added from 16 mar 99 change doesn't exist on VMS so
**	adding an ifndef VMS
**  29-nov-1999 (somsa01)
**	For the TNG group, connect to iidbdb as the 'ingres' user. This
**	change is only built into Black Box Ingres. 
**  16-dec-1999 (somsa01)
**	Include systypes.h before stdio.h to prevent compilation problems
**	on platforms such as HP.
**  04-Apr-2000 (noifr01)
**  (bug 101430) manage timeout of the wait for a character , in order 
**  to be able to send back to the VDBA client a partial line (with
**  no CR), which is needed in the case where the executed command
**  asks a question and waits for the reply without a carriage
**  return being sent inbetween. SPecific code for NT and UNIX.
**  VMS to be double checked if TEget() makes problem in this context
**    
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-sep-2000 (somsa01)
**	    TNG_EDITION has been superseded by the use of embed_user.
**	11-oct-2000 (mcgem01)
**	    The Ingres II SDK can be installed and run as any user, so it needs
**	    to mimic the Embedded Ingres version.
**  03-Jan-2001 (noifr01)
**      (SIR #103625) now have rmcmd work against imadb rather than iidbdb
**  15-Mar-2002 (noifr01)
**      (bug 107810) character with value 255 was considered as an EOF under
**       NT
**  16-may-2003 (devjo01)
**	TEget DOES cause major problems for VMS.  See 04-Apr-2000 comments.
**      A number of attempts to simulate UNIX implementation have left me
**	defeated.  "New" implementation effectively reverts VMS to 2.0
**	behavior.
**  22-Oct-2003 (schph01)
**      (SIR #111153) now have rmcmd run under any user id
**  23-feb-2005 (stial01)
**     Explicitly set isolation level after connect
**	20-sep-2005 (abbjo03)
**	    Initialize ires for a successful return.
*/

/*
PROGRAM =       rmcmdout

NEEDLIBS =      RUNSYSLIB RUNTIMELIB FDLIB FTLIB FEDSLIB \
                UILIB LIBQSYSLIB LIBQLIB UGLIB FMTLIB AFELIB \
                LIBQGCALIB CUFLIB GCFLIB ADFLIB SQLCALIB \
                COMPATLIB 
*/
/*
	Note that MALLOCLIB should be added when this code has been CLized
*/


/** Make sure SQL areas are included before compat.h to prevent GLOBALDEF
 ** conversion of their 'extern' definitions on VMS:
 **/
exec sql include sqlca;
exec sql include sqlda;

#include <compat.h>

/* *CL* -- New includes for CL routines -- */
#include <cv.h>
#include <er.h>
#include <gl.h>
#include <lo.h>
#include <pm.h>
/* *CL* in future, use: #include <si.h> */
#include <st.h>

#ifdef VMS
#include <gc.h>
#include <iicommon.h>
#include <mu.h>
#include <gca.h>
#endif
#include    <te.h>
#ifndef VMS
#include <systypes.h>
#endif /* VMS */
#include <stdio.h>
#include <ctype.h>
#ifdef WIN32
#include "windows.h"
#endif

#include "dba.h"
#include "rmcmd.h"

#define RMCMDOUT_TIMEOUT (8)

static i4 embed_installation = -1;

/* TO BE FINISHED check ingres user and other issues */

BOOL getoneline(char * buf, int buflen, BOOL *pbNOCR)
{
   int i,result;
   BOOL bresult = TRUE;
#ifndef WIN32
   i4 nresult;
#endif
#ifdef VMS
   char testeof;	/* Due to probs with EOF from getchar in VMS */
#endif
   i=0;
   *pbNOCR = FALSE;
   while (1) {

#ifdef WIN32
	{
		char ch;
        DWORD cchRead = 0;

        while (cchRead == 0)
        {
    		BOOL bSuccess = ReadFile(GetStdHandle(STD_INPUT_HANDLE),
		                             &ch,
		                             sizeof(ch),
		                             &cchRead,
		                             NULL);

            if (!bSuccess && (GetLastError() == ERROR_BROKEN_PIPE))
            {
				bresult = FALSE;
                break;
            }
			if (ch == '\0' && i==0) /* no effect of timeout if line is empty */
			  cchRead=0;
        }
		if (!bresult)
			break;
			 

        result = (int)ch;
		if (result == '\0') {
		   *pbNOCR = TRUE;
		   break;
		}
	}
#else
#ifdef VMS
	result=getchar();
#else
	while (TRUE) {
		TEinflush();
		nresult = TEget(RMCMDOUT_TIMEOUT);
		if (nresult == TE_TIMEDOUT ) {
			if (i==0)  /* no effect of timeout if line is empty */
				continue;
			else {
			   *pbNOCR = TRUE;
 			   break;
			}
		}
		else
			break;
	}
	if (*pbNOCR == TRUE)
		break;
	result = (int)nresult;
    if (result==EOF) {
	  bresult = FALSE;
      break;
	}
#endif
#endif
#ifdef VMS
     testeof = result;		/* VMS 'getchar' has problems casting the */
     if (testeof==EOF)		/* EOF char to int; so retest as a single */
       break;			/* char.                                  */
#endif
     if (result=='\n') 
        break;
#ifdef WIN32
     /* ignore the \r in windows NT, otherwise funny character in dlg box */
     if (result!='\r')
       buf[i++]=result;
#else
     buf[i++]=result;
#endif
     if (i>=buflen-1) 
       break;
   }
   buf[i]='\0';
#ifdef VMS
   if (testeof==EOF) return FALSE; else
#endif
   return bresult;
}

#ifdef WIN32
typedef struct tagTHREADPARAM
{
    int idProcess;
    HANDLE hWrite;
}THREADPARAM, FAR * LPTHREADPARAM;

DWORD ThreadPollTerminate(LPDWORD lpdw)
{
    char ch = EOF;
	char ch0 = '\0';
    DWORD ccb;
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, ((LPTHREADPARAM)lpdw)->idProcess);
    HANDLE hWrite = ((LPTHREADPARAM)lpdw)->hWrite;
	while (TRUE) {
		DWORD dwresult=	WaitForSingleObject(hProcess, (1000 * RMCMDOUT_TIMEOUT));
		if (dwresult == WAIT_TIMEOUT) {
				WriteFile(hWrite, &ch0, sizeof(ch0), &ccb, NULL);
				continue;
				break;
		}
		break;
	}
/*    WriteFile(hWrite, &ch, sizeof(ch), &ccb, NULL); */
    CloseHandle(hProcess);
    CloseHandle(hWrite);
    return 0;
}
#endif

#ifdef VMS
int
#else
void 
#endif
main(int argc, char* argv [])
{
    /* TO BE FINISHED: handle all errors; */

   exec sql begin declare section;
      char buf[RMCMDLINELEN];
      char buf1[100];
      char buf2[100];
	  char bufNOCR[9];
      int  hdl;
      int  iOutputLine;
      int  status1;
      int ires = 0;
   exec sql end declare section;
   char bufstdio[100],bufstdo[100];
   BOOL bNoCR;

#ifdef WIN32
    HANDLE hThread;
    DWORD dwID;
    THREADPARAM param;
#endif
   setvbuf(stdin, bufstdio, _IONBF,0);
   setvbuf(stdout,bufstdo,  _IONBF,0);
#ifdef WIN32
   if (argc!=4)
      return;
#else
   if (argc!=2)
#ifdef VMS
      return (200 + argc);
#else
      return;
#endif
#endif

/* *CL* converted:   hdl = atoi (argv [1]); */
   CVan( argv[1], &hdl );
#ifdef WIN32
    param.idProcess = atoi(argv[2]);
    param.hWrite = (HANDLE)atoi(argv[3]);

    hThread = CreateThread(NULL, 
                           0, 
                           (LPTHREAD_START_ROUTINE)ThreadPollTerminate, 
                           (LPVOID)&param, 
                           0, 
                           &dwID);
    CloseHandle(hThread);
#endif

    exec sql connect 'imadb/ingres' identified by '$ingres';

   if (sqlca.sqlcode !=0)
#ifdef VMS
     return (102);
#else
     return;
#endif

   exec sql set session isolation level serializable;

   exec sql select user_id, session_id
            into   :buf1,   :buf2
            from   remotecmd
            where  handle=:hdl;

   if (sqlca.sqlcode < 0) 
#ifdef VMS
   {
      ires = 104;
      goto end;
   }
#else
      goto end;
#endif
   exec sql commit;

   iOutputLine = -1;
   while (getoneline(buf,sizeof(buf),&bNoCR)==  TRUE) {
      iOutputLine++;
      if (bNoCR)
         STcopy(ERx("Y"),bufNOCR);
      else
         STcopy(ERx(""), bufNOCR);

      exec sql insert into remotecmdout
               values(:hdl,:iOutputLine,:buf1,:buf2,:buf,:bufNOCR);

   if (sqlca.sqlcode < 0) 
#ifdef VMS
   {
      ires = 105;
      goto end;
   }
#else
      goto end;
#endif
   exec sql raise dbevent rmcmdnewoutputline;

   if (sqlca.sqlcode < 0) 
#ifdef VMS
   {
      ires = 105;
      goto end;
   }
#else
      goto end;
#endif

      exec sql commit;
 /*     printf("result sent to client : %s\n",buf); */
   }

   /* termination */
   status1=RMCMDSTATUS_FINISHED;
   exec sql update remotecmd
            set status=:status1
            where handle=:hdl;
   if (sqlca.sqlcode < 0) 
#ifdef VMS
		ires = 106;
#else
		ires = 1;
#endif
   exec sql raise dbevent rmcmdcmdend;
   if (sqlca.sqlcode < 0) 
#ifdef VMS
		ires = 106;
#else
		ires = 1;
#endif

end:
   exec sql commit;
   exec sql disconnect;
#ifdef VMS
   close( 0 );
   return (ires);
#endif
}
