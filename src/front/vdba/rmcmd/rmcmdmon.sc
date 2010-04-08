/********************************************************************
**
**  Copyright (C) 1995,2000 Ingres Corporation
**
**    Project : CA/OpenIngres Visual DBA
**
**    Source : rmcmdmon.sc
**    Remote daemon interactive monitor
**
**    Originally written by Mark Boas (boama01)
**
**
** History:
**
**	02-Jan-96 (boama01)
**		Written.
**	20-feb-96 (wadag01)
**		SCO (sos_us5) needs to include <types.h>.
**		Also the header file <unixio.h> does not exist on SCO.
**	26-feb-96 (morayf)
**		SNI (rmx_us5) needs changes like SCO. The unixio.h
**		inclusion should be ifdef-ed for the platform which
**		needs it, presumably the dev. platform. Otherwise
**		this file will need to be altered for EVERY port !
**	02-mar-96
**		Added Pyramid exclusion of unixio.h.
**		Also needs types.h or equivalent.
**	26-mar-96 (chimi03)
**		HP (hp8_us5) needed the same change as the above platforms.
**		After receiving counsel from people with large brains,
**		implemented this more generic solution:
**		Uncommented the include for si.h and remove all the
**		non-ingres includes.  Also cast the helptext field
**		where the compiler gave a warning.
**	08-apr-96 (chimi03)
**		Fix a typo made while integrating the above change.
**	29-may-1997 (canor01)
**		Add length parameter to 2 TEwrite() calls where it had
**		been omitted.
**    05-oct-1998 (kinte01)
**        For VMS only, if a user has the SERVER CONTROL he can setup the
**        RMCMD objects & start/stop the RMCMD server.
**   09-Mar-1999 (schte01)
**       Changed p[nlines] assignment within for loop so there is no
**       dependency on order of evaluation (in pager routine).
**	29-nov-1999 (somsa01)
**	    For the TNG group, connect to iidbdb as the 'ingres' user.
**	    This change is only built into Black Box Ingres.
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
**   09-Mar-1999 (schte01)
**       Changed p[nlines] assignment within for loop so there is no
**       dependency on order of evaluation (in pager routine).
*/

/*
PROGRAM =       rmcmdmon

NEEDLIBS =      RUNSYSLIB RUNTIMELIB FDLIB FTLIB FEDSLIB \
                UILIB LIBQSYSLIB LIBQLIB UGLIB FMTLIB AFELIB \
                LIBQGCALIB CUFLIB GCFLIB ADFLIB SQLCALIB \
                COMPATLIB 
*/
/*
	Note that MALLOCLIB should be added when this code has been CLized
*/

/** Make sure SQL areas are included before compat.h to prevent GLOBALDEF
 ** conversion of their 'extern' definitions:
 **/
exec sql include sqlca;
exec sql include sqlda;

/* *CL* -- New includes for CL routines -- */
#include <compat.h>
#include <cm.h>
#include <er.h>
#include <gl.h>
#include <lo.h>
#include <pc.h>
#include <pm.h>
#include <si.h>
#include <st.h>
#include <te.h>
#include <tm.h>
#include <gca.h>

#ifdef VMS
#include <gc.h>
#include <iicommon.h>
#include <mu.h>
#endif

#include "rmcmd.h"		/* (includes dba.h) */

/** Following are control character sequences for screen erase, cursor position,
 ** up arrow and down arrow, by terminal type:
 ** [0] = VT-class TTY (52,100,220,300)
 ** [1] = other terminal type
 ** In the future, this should be replaced by calls to a CL routine that can
 ** return characteristics from the Ingres TERMCAP file matching the current
 ** TERM_INGRES setting (assuming this hasn't been ABF-ized).
 **/
static char *screrase[] = {
	"\033[2J\033[H",	/* (VT100-class devices) */
	""			/* (other types) */
	};
static char *cursline[] = {
	"\033[%02dH",		/* (VT-100-class devices) */
	""			/* (other) */
	};
static int termtype, termrows, termcols;

static i4 embed_installation = -1;

/** General variables used in multiple functions **/
static char termbuf[256];
static int  menuline, nrows, scrnlines;
static char endline[] =
 ERx("-------------Press any key to return to main session display----------");
static char contline[] =
 ERx("---------------------- Press any key to continue ---------------------");
/** Constants that may be adjusted before product is finalized **/
#define SLEEP_SECONDS		2
#define SLEEP_MILLISECONDS	SLEEP_SECONDS*1000

/** Following are control codes for page/scroll functions in pager() **/
#define NEW_PAGE	1
#define FORCE_PAGE	2
#define END_PAGE	3
void pager(char *line, int control)
{
	int  nlines, i;
	i4  response;
	char *p[80];
	/* Each p points to the start of each line in input, up to either a
	   newline or null char */
	p[0] = p[1] = line;
	for (nlines = 1; *p[nlines] != '\0'; p[nlines]++)
		if (*p[nlines] == '\n')
		   { 
		   ++p[nlines];
		   p[nlines+1] = p[nlines];
		   nlines++;
		   }
	if (p[0] == p[1])
		nlines = 0;
	if (control == NEW_PAGE) {
		TEwrite( screrase[termtype], STlength( screrase[termtype] ) );
		scrnlines = 0;
		}
	for (i = 0; i < nlines; i++) {
		TEwrite( p[i], (int)(p[i+1]-p[i]) );
		TEput( '\r' );
		scrnlines++;
		if ((scrnlines+1) >= termrows) {
			TEwrite( contline, 70 );
			TEflush();
			response = TEget( 0 );
			TEwrite( screrase[termtype],
			  STlength( screrase[termtype] ) );
			scrnlines = 0;
			}
		}
	if (nlines)
		TEput( '\n' );
	if (control == FORCE_PAGE) {
		TEflush();
		response = TEget( 0 );
		TEwrite( screrase[termtype], STlength( screrase[termtype] ) );
		scrnlines = 0;
		}
	else if (control == END_PAGE)
		TEflush();
}

void helpscrn()
{
	int i;
	const char *helptext[] = {
"\nThe CA-Ingres Visual-DBA Remote Command Monitor (RMCMDMON) is a performance",
"monitoring tool for evaluating the execution of DBA command-line utilites",
"in an Ingres installation, in response to requests made from Visual-DBA",
"clients.  The tool monitors the status of command requests and stored inputs",
"and outputs associated with a request's execution.  It also provides the",
"system administrator with functions to manage individual requests and the",
"system catalog entries which support command operations.\n",
"The main session display shows all managed command requests.  Each request",
"is uniquely identified with a \"handle\" number.  The status of each request",
"is given as one of the following:",
"    Sent     - the command was sent from a client and is awaiting evaluation",
"               and execution by the server",
"    Serving  - the command was acknowledged by an active server and will be",
"               evaluated for correctness",
"    Accepted - the command was evaluated and accepted by the server, and is",
"               currently awaiting execution",
"    Finished - the command has completed execution by the server",
"    ERROR    - the command was not accepted or encountered an execution error",
"    Acknwlgd - ???",
"    TermRqst - a request for server shutdown has been issued by RMCMDSTP\n",
"The following actions can be requested from the main session display of the",
"Visual-DBA Remote Command Monitor by entering the appropriate letter:",
"    R - refresh the main session display",
"    I - view input lines for a command execution session",
"    O - view output lines from a command execution session",
"    K - kill a session (set its status to \"Finished\")",
"    C - clean out all command entries/inputs/outputs for an old session",
"    L - view the RMCMD command server's Log file",
"    S - stop the RMCMD command server",
"    H - display this Help on monitor usage",
"    X - exit the Remote Command Monitor",
	NULL };
	for (i = 0; helptext[i] != NULL; i++) {
		if (helptext[i] == (const char*)-1)
			pager( contline, FORCE_PAGE );
		else
			pager( (char *)helptext[i], 0 );
		}
	pager( endline, FORCE_PAGE );
}

void msgline(char *msg)
{
	char	tmpbuf[256], cursbuf[25];
	STprintf( cursbuf, cursline[termtype], menuline );
	TEwrite( cursbuf, STlength( cursbuf ) );
	STprintf( tmpbuf, ERx("%-*s"), termcols, " " );
	TEwrite( tmpbuf, STlength( tmpbuf ) );
	TEwrite( cursbuf, STlength( cursbuf ) );
	TEwrite( msg, STlength( msg ) );
	TEflush();
/* (Note that no adjustment to scrnlines is done here, since the menu line
    is simply overlaid.) */
}

void not_avail(int request)
{
	STprintf( termbuf,
	  ERx("Sorry, the '%c' command code is not yet available."),
		(char)request );
	msgline( termbuf );
	PCsleep( SLEEP_MILLISECONDS );
}

int supply_handle(char *purpose)
{
	exec sql begin declare section;
	int  user_hdl = -1;
	int  user_char;
	exec sql end declare section;
	char user_str[21], *in_str;

	if (nrows == 0) {
	  msgline( ERx("No sessions are available for selection.") );
	  PCsleep( SLEEP_MILLISECONDS );
	  return (-1);
	  }
	while (user_hdl < 0) {
		STprintf( termbuf, ERx("Handle of command %s: "),
			purpose );
		msgline( termbuf );
		in_str = user_str;
		while (1) {
			user_char = TEget( 0 );
			if (user_char == EOF)  break;
			if ((char)user_char == '\n')  break;
			if ((char)user_char == '\r')  break;
			*in_str++ = (char)user_char;
			TEput( (char)user_char );
			TEflush();
			if ((in_str - user_str) == sizeof(user_str))  break;
			}
		*in_str = '\0';
		if (STlength( user_str ) < 1)
			return (-1);
		if (CVan( user_str, &user_char ) == OK) {
			exec sql select handle into :user_hdl
				from remotecmd
				where handle = :user_char;
			exec sql commit;
			}
		if (user_hdl < 0) {
			STprintf( termbuf,
	  			ERx("'%s' is not a valid handle."), user_str );
			msgline( termbuf );
	  		PCsleep( SLEEP_MILLISECONDS );
			}
		}
	return (user_hdl);
}

main (int argc, char* argv [])
{

   exec sql begin declare section;
	int  status_val;
	int  handle_val;
	int  order_val;
	char userid    [40];
	char sessionid [40];
	char cmds      [256];
	int  ires;
   exec sql end declare section;

/** General-purpose non-SQL variables **/
	char buf [1000], inchar;
	i4  request;
	int  nlines;
	int  action = 0;
#define ACTION_REQUEST	0	/* In loop to solicit requested action */
#define ACTION_REFRESH	1	/* Refresh command list and restart loop */
#define ACTION_EXIT	2	/* Exit loop to exit monitor */
	SYSTIME	time_now;
	TEENV_INFO	teinfo;

/** Main screen line literals **/
	char main1 [] =
"RMCMDMON  Visual DBA Remote Command Daemon Monitor   %24s",
	     main2 [] =
"HANDLE USER---------------- SESSION--- STATUS-- COMMAND---------------------",
	     sessline [] = "%6d %-20s %-10s %-8s %-30s",
	     endsess [] =
"----------------------------------------------------------------------------\n",
	     menu [] =
"Refresh, Input, Output, Kill, Clean, Log, Stop-daemon, Help, eXit: ";
/** Input/output screen literals **/
	char ioscr2 [] =
"%s lines for Handle = %d, User = %s, session %s:",
	     ioscr3 [] =
"ORDER LINE------------------------------------------------------------------",
	     ioline [] = "%5d %-73s";


	FEcopyright( ERx("Visual DBA"), ERx("1995"));

/*
   setvbuf( stdin, NULL, _IONBF, 0 );
   setvbuf( stdout, NULL, _IONBF, 0 );
*/
/*   SIeqinit(); */
/* Open terminal and determine terminal type for control sequences */
   TEopen( &teinfo );
   if (teinfo.rows > 0)
	termrows = teinfo.rows;
   else
	termrows = 24;		/* Should be from termcap file... */
   if (teinfo.cols > 0)
	termcols = teinfo.cols;
   else
	termcols = 80;		/* Should be from termcap file... */
   termtype = 0;		/* Should be from TERM_INGRES... */

   if (embed_installation == -1)
   {
	char	config_string[256];
	char	*value, *host;

	embed_installation = 0;

	/*
	** Get the host name, formally used iipmhost.
	*/
	host = PMhost();

	/*
	** Build the string we will search for in the config.dat file.
	*/
	STprintf( config_string,
		  ERx("%s.%s.setup.embed_user"),
		  SystemCfgPrefix, host );

	PMinit();
	if (PMload( NULL, (PM_ERR_FUNC *)NULL ) == OK)
	{
	    if (PMget(config_string, &value) == OK)
		if ((value[0] != '\0') &&
		    (STbcompare(value,0,"on",0,TRUE) == 0))
		    embed_installation = 1;
	}
    }
   
#ifdef EVALUATION_RELEASE
   exec sql connect iidbdb identified by ingres;
#else
   if (embed_installation)
	exec sql connect iidbdb identified by ingres;
   else
	exec sql connect iidbdb;
#endif

   if (sqlca.sqlcode !=0)
   {
       TEwrite( ERx("Cannot connect to 'iidbdb'.\n"), 28 );
       TEclose();
       return 0;
   }
  /************ check if session user is INGRES *************************/
  ires = RES_ERR;
  exec sql select dbmsinfo('session_user') into :userid;
  if (sqlca.sqlcode < 0) 
  {
	TEwrite( ERx("Cannot determine SQL user.\n"), 27 );
	goto end;
  }

    STtrmwhite(userid);

    PMsetDefault( 0, ERx( "ii" ) );
    PMsetDefault( 1, PMhost() );


   if ( chk_priv( buf, GCA_PRIV_SERVER_CONTROL ) != OK)
   {
        TEwrite( ERx("User is not Authorized.\n"), 24 );
        ires = RES_NOTINGRES;
        goto end;
   }

  /****** Main monitor screen logic ******/
/** The monitor screen shows all current client command sessions (in-progress
 ** and complete), ordered by handle.  From this screen, the following keys
 ** may be pressed (lower- or upper-case):
 **   R - refresh the command display;
 **   I - view input lines for a session;
 **   O - view output lines for a session;
 **   K - kill a session (set its status to _FINISHED);
 **   C - clean out all table rows for an old session;
 **   L - view the RMCMD Daemon Log file;
 **   S - stop the RMCMD Daemon process;
 **   H - get Help on monitor usage;
 **   X - exit the daemon monitor.
 ** When a scrollable display of sessions, input lines, or output lines is
 ** being viewed, the up-arrow and down-arrow keys may also be used to scroll
 ** the display backwards and forwards, respectively.
 **/

/* Loop until exit requested; build main screen of command entries */
   while(1) {
	TMnow( &time_now );
	TMstr( &time_now, buf );
	STprintf( termbuf, main1, buf );
	pager( termbuf, NEW_PAGE );
	pager( main2, 0 );
	nrows = 0;
	exec sql repeated select *
	  into :handle_val, :status_val, :userid, :sessionid, :cmds
	  from remotecmd
	  order by handle;
	exec sql begin;
	  switch (status_val) {
	    case RMCMDSTATUS_ERROR: STcopy( ERx("ERROR"), buf ); break;
	    case RMCMDSTATUS_SENT: STcopy( ERx("Sent"), buf ); break;
	    case RMCMDSTATUS_ACCEPTED: STcopy( ERx("Accepted"), buf ); break;
	    case RMCMDSTATUS_FINISHED: STcopy( ERx("Finished"), buf ); break;
	    case RMCMDSTATUS_ACK: STcopy( ERx("Acknwlgd"), buf ); break;
	    case RMCMDSTATUS_SERVERPRESENT: STcopy( ERx("Serving"), buf ); break;
	    case RMCMDSTATUS_REQUEST4TERMINATE: STcopy( ERx("TermRqst"), buf ); break;
	    default: STprintf( buf, ERx("%-8d"), status_val );
	    }
	  STprintf( termbuf, sessline, handle_val, userid, sessionid, buf,
			cmds );
	  pager( termbuf, 0 );
	  nrows++;
	exec sql end;
	exec sql commit;
	if (nrows == 0) {
	    pager( ERx("No sessions are currently available."), 0 );
	    }
	pager( endsess, END_PAGE );
	menuline = nrows > 0 ? 4 + nrows : 4 + 1;

/* Repeat actions on same screen until refresh or exit is required */
	while( action == ACTION_REQUEST ) {
	    msgline( menu );
	    request = TEget( 0 );
	    if (request == EOF)
		break;
	    inchar = request;
	    CMtoupper( &inchar, &inchar );
	    switch (inchar) {
	        case 'X': action = ACTION_EXIT; break;
		case 'R': action = ACTION_REFRESH; break;

		case 'I':
		    if ((handle_val = supply_handle(ERx("for input display")))
		      < 0)
			break;
		    TMnow( &time_now );
		    TMstr( &time_now, buf );
		    STprintf( termbuf, main1, buf );
		    pager( termbuf, NEW_PAGE );
		    exec sql select user_id, session_id
			into :userid, :sessionid
			from remotecmd
			where handle = :handle_val;
		    STprintf( termbuf, ioscr2, ERx("Input"), handle_val,
			userid, sessionid );
		    pager( termbuf, 0 );
		    pager( ioscr3, 0 );
		    nlines = 0;
		    exec sql select orderno, instring
			into :order_val, :cmds
			from remotecmdin
			where handle = :handle_val
			order by orderno;
		    exec sql begin;
			STprintf( termbuf, ioline, order_val, cmds );
			pager( termbuf, 0 );
			nlines++;
		    exec sql end;
		    exec sql commit;
		    if (nlines == 0) {
			pager ( ERx("No input lines are currently available."),
			  0 );
			}
		    pager( endsess, 0 );
		    pager( endline, FORCE_PAGE );
		    action = ACTION_REFRESH;
		    break;

		case 'O':
		    if ((handle_val = supply_handle(ERx("for output display")))
		      < 0)
			break;
		    TMnow( &time_now );
		    TMstr( &time_now, buf );
		    STprintf( termbuf, main1, buf );
		    pager( termbuf, NEW_PAGE );
		    exec sql select user_id, session_id
			into :userid, :sessionid
			from remotecmd
			where handle = :handle_val;
		    STprintf( termbuf, ioscr2, ERx("Output"), handle_val,
			userid, sessionid );
		    pager( termbuf, 0 );
		    pager( ioscr3, 0 );
		    nlines = 0;
		    exec sql select orderno, outstring
			into :order_val, :cmds
			from remotecmdout
			where handle = :handle_val
			order by orderno;
		    exec sql begin;
			STprintf( termbuf, ioline, order_val, cmds );
			pager( termbuf, 0 );
			nlines++;
		    exec sql end;
		    exec sql commit;
		    if (nlines == 0) {
			pager( ERx("No output lines are currently available."),
			  0 );
			}
		    pager( endsess, 0 );
		    pager( endline, FORCE_PAGE );
		    action = ACTION_REFRESH;
		    break;

		case 'K': 
		    if ((handle_val = supply_handle(ERx("to be killed"))) < 0)
			break;
		    exec sql select user_id, session_id, status
			into :userid, :sessionid, :status_val
			from remotecmd
			where handle = :handle_val;
		    if (status_val == RMCMDSTATUS_FINISHED) {
			STprintf( buf,
			  ERx("Session for handle %d is already finished."),
			  handle_val );
			msgline( buf );
			exec sql commit;
	  		PCsleep( SLEEP_MILLISECONDS );
			break;
			}
		    order_val = -1;
		    exec sql select max(orderno)
			into :order_val
			from remotecmdout
			where handle = :handle_val;
		    order_val++;
		    exec sql insert into remotecmdout
			values (:handle_val, :order_val, :userid, :sessionid,
			'***COMMAND ABORTED THRU RMCMDMON***');
		    order_val = RMCMDSTATUS_FINISHED;
		    exec sql update remotecmd
			set status = :order_val
			where handle = :handle_val;
		    exec sql commit;
		    action = ACTION_REFRESH;
		    break;

		case 'C':
		    if ((handle_val = supply_handle(ERx("to be cleaned"))) < 0)
			break;
		    exec sql select max(handle)
			into :order_val
			from remotecmd;
		    if (order_val != handle_val) {
			STprintf( buf,
		ERx("The highest-handle command (%d) must be cleaned first."),
			  order_val );
			msgline( buf );
			exec sql commit;
	  		PCsleep( SLEEP_MILLISECONDS );
			break;
			}
		    exec sql select user_id, session_id, status
			into :userid, :sessionid, :status_val
			from remotecmd
			where handle = :handle_val;
		    if ((status_val != RMCMDSTATUS_FINISHED)
		      && (status_val != RMCMDSTATUS_ERROR)) {
			STprintf( buf,
		ERx("Session for handle %d must be finished or in error."),
			  handle_val );
			msgline( buf );
			exec sql commit;
	  		PCsleep( SLEEP_MILLISECONDS );
			break;
			}
		    exec sql delete from remotecmd
			where handle = :handle_val;
		    exec sql delete from remotecmdin
			where handle = :handle_val;
		    exec sql delete from remotecmdout
			where handle = :handle_val;
		    exec sql commit;
		    action = ACTION_REFRESH;
		    break;

		case 'L': not_avail(request); break;
		case 'S': not_avail(request); break;

		case 'H':
		    TMnow( &time_now );
		    TMstr( &time_now, buf );
		    STprintf( termbuf, main1, buf );
		    pager( termbuf, NEW_PAGE );
		    helpscrn();
		    action = ACTION_REFRESH;
		    break;

		case '\n':
		case '\r': break;
		default: 
		    TEwrite( ERx("\a"), 1 );	/* (ring the bell) */
		    TEflush();
		}
	    } /* (end while(ACTION_REQUEST)) */
	if (action == ACTION_EXIT)
	    break;
	action = 0;
   } /* (end while(1)) */

end:
   exec sql disconnect;
   TEwrite( ERx("\n"), 1 );
   TEclose ();
}
