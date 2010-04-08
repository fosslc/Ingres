/********************************************************************
**
**  Copyright (C) 1995, 2005 Ingres Corporation
**
**
**    Project : Ingres Visual DBA
**
**    Source : rmcmdin.sc
**    manages remote commands input
**
**
** History:
**
**	23-Sep-95 (johna)
**		Added MING hints and moved the 'main' to the start of the 
**		start of the line so that mkming would recognise it.
**
**      04-Jan-96 (spepa01)
**          	Add CL headers so that STxcompare()/STlength() can be used.
**	25-Jan-96 (boama01)
**		Added VMS-specific and CL includes; first cut at CL-izing code;
**		see "*CL*" line markings.  Also improved error reporting.
**  25-Mar-1998 (noifr01)
**      -Added usage of dbevents to speed up rmcmd  
**    05-oct-1998 (kinte01)
**	  For VMS only, if a user has the SERVER CONTROL he can setup the
**	  RMCMD objects & start/stop the RMCMD server. 
**	  The above changes meant that all SQL queries referencing 
**	  ingres.<whatever> had to be rewritten in dynamic sql to be
**	  the user setting up RMCMD. For UNIX & NT this will always be
**	  ingres. For VMS this may or may not be ingres but will be the
**	  Ingres System Admin User.
**  02-Jan-1999 (noifr01)
**    register also the rmcmdcmdend dbevent, in order to be notified
**    immediatly of the completion of the remote command execution.
**    this avoids the rmcmdin process to persist up to 60 seconds 
**    after the command has terminated, and even more if a new remote command
**    has been requested within the interval.
**  02-Feb-99 (noifr01)
**     specify the ingres server class when connecting, in order to support 
**     the case where the default server class is not ingres
**  29-nov-1999 (somsa01) 
**	For the TNG group, connect to iidbdb as the 'ingres' user. This
**	change is only built into Black Box Ingres.
**  11-sep-2000 (somsa01)
**	TNG_EDITION has been superseded by the use of embed_user.
**  11-oct-2000 (mcgem01)
**      The Ingres II SDK can be installed and run as any user, so it needs
**      to mimic the Embedded Ingres version.
**  03-Jan-2001 (noifr01)
**   (SIR #103625) now have rmcmd work against imadb rather than iidbdb
**   also removed unused variables that resulted in a build warning
**  22-Oct-2003 (schph01)
**   (SIR #111153) now have rmcmd run under any user id
**  23-feb-2005 (stial01)
**     Explicitly set isolation level after connect
**	20-sep-2005 (abbjo03)
**	    Initialize ires for a successful return.
*/

/*
PROGRAM =       rmcmdin

NEEDLIBS =      RUNSYSLIB RUNTIMELIB FDLIB FTLIB FEDSLIB \
                UILIB LIBQSYSLIB LIBQLIB UGLIB FMTLIB AFELIB \
                LIBQGCALIB CUFLIB GCFLIB ADFLIB SQLCALIB \
                COMPATLIB 
*/
		
/* 
	Note that MALLOCLIB should be added when this code has been CL'ized
*/


#include <stdio.h>
#include <ctype.h>
#ifdef VMS
#include <unixio.h>
#include <signal.h>
#endif
#ifdef WIN32
#include <windows.h>
#endif

/** Make sure SQL areas are included before compat.h to prevent GLOBALDEF
 ** conversion of their 'extern' definitions on VMS:
 **/
exec sql include sqlca;
exec sql include sqlda;

/* *CL* -- New includes for CL routines -- */
#include <compat.h>
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
#include <pm.h>
#include <gca.h>
#endif

#include "dba.h"
#include "rmcmd.h"

/* TO BE FINISHED check ingres user and other issues */

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
      int ordno;
      int hdl;
      int istatus;
      int InputLine;
      int ires = 0;
      char stmt[2048];
   exec sql end declare section; 
    setvbuf(stdin, buf1,_IONBF,0);	/* *CL* do not use if converted */
    setvbuf(stdout,buf2,_IONBF,0);	/* *CL* do not use if converted */
   if (argc!=2)
#ifdef VMS
      return (200 + argc);
#else
      return;
#endif

/* *CL* converted:   hdl = atoi (argv [1]); */
   CVan( argv[1], &hdl );

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

   STprintf(stmt,ERx("register dbevent rmcmdnewinputline"));
   EXEC SQL EXECUTE IMMEDIATE :stmt;

   if (sqlca.sqlcode < 0) 
#ifdef VMS
      {
      ires = 104;
      goto end;
      }
#else
      goto end;
#endif

   STprintf(stmt,ERx("register dbevent rmcmdcmdend"));
   EXEC SQL EXECUTE IMMEDIATE :stmt;

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


   InputLine = -1;

   while (1) {
      exec sql get dbevent with wait = 60;
	  exec sql commit;
	  /*  sleep (5); */
      ordno = -1;

      exec sql  select orderno, instring
            into       :ordno,  :buf
            from remotecmdin
            where handle = :hdl and orderno>:InputLine
            order by orderno;

    EXEC SQL BEGIN;
    if (ordno>=0) {
        InputLine++;
        if (InputLine!=ordno){
            /* Old code was: printf("internal error\n");
            Should not write error message thru utility-piped stdout! instead: */
	        ires = 106;
	        goto end;
	    }
        printf("%s\n",buf);       /* output to stdout */
    }
    EXEC SQL END;
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
      /* checks if command terminated */
      istatus = -1;
      exec sql  select status
            into   :istatus
            from remotecmd
            where handle = :hdl ;
   if (sqlca.sqlcode < 0 || sqlca.sqlcode == 100) 
#ifdef VMS
      {
      ires = 107;
      goto end;
      }
#else
      goto end;
#endif
      exec sql commit;
      if (istatus==RMCMDSTATUS_FINISHED)
        break;
   }
   /* termination */
end:
   exec sql commit;
   exec sql disconnect;
#ifdef VMS
   close( 1 );
   return (ires);
#endif
}
