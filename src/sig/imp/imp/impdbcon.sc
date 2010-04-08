/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name:	impdbcon.sc
**
** DB connection routines for the new IMA based IPM lookalike tool.
**
**
** This file contains the routines used to connect and disconnect from the 
** various databases needed by the IMP. This section has undergone quite
** substantial changes as the connection to the iidbdb and user databases
** was deferred.
**
** The error check routine needs considerable rework to deal with no rows 
** situations. I was about to do this when the decision to handover was made.
** 
## History:
##
## 26-jan-94	(johna)
##		started on RIF day from pervious ad-hoc programs
## 04-may-94	(johna)
##		some comments
## 02-Jul-2002 (fanra01)
##      Sir 108159
##      Added to distribution in the sig directory.
## 04-Sep-03    (flomi02)  mdf040903
##              Commit transactions to release locks
## 06-Apr-04    (srisu02) SIR 108159
##      Integrating changes made by flomi02  
##
*/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef UNIX
#include <sys/signal.h>
#define SIGHANDLERS \
signal(SIGINT,sighandler); \
signal(SIGQUIT,sighandler); \
signal(SIGBUS,sighandler); \
signal(SIGSEGV,sighandler)

#else
#include <signal.h>
#define SIGHANDLERS \
signal(SIGINT,sighandler); \
signal(SIGSEGV,sighandler)

#endif /* NT_GENERIC */
#include "imp.h"

exec sql include sqlca;

void	sighandler( int );

exec sql begin declare section;

extern int iidbdbConnection;
extern int dbConnection;
extern int inFormsMode;
extern int imaSession;
extern int iidbdbSession;
extern char	*currentVnode;

exec sql end declare section;

int
sqlerr_check()
{
	exec sql begin declare section;
	char    msgBuf[1024];
	int     currentConnection;
	exec sql end declare section;

	if (sqlca.sqlcode < 0) {
		exec sql inquire_sql (:msgBuf = errortext);
		exec sql inquire_sql (:currentConnection = session);

		if (inFormsMode) {
			exec frs set_frs frs (timeout = 0);
			exec frs message :msgBuf with style = popup;
			exec frs clear screen;
			exec frs endforms;
		} else {
			printf("%s\n",msgBuf);
		}
		if (currentConnection) {
			exec sql disconnect;
		}
		exit(0);
	} else if (sqlca.sqlcode == 100) {
		return(NO_ROWS);
	}
	return(TRUE);
}


/*
** makeDbConnections - originally made connections to the IMA database and 
** the local IIDBDB. The IIDBDB  connect was postponed as we may want to switch
** vnodes.
*/
int
makeDbConnections(dbname)
exec sql begin declare section;
char	*dbname;
exec sql end declare section;
{
	exec sql begin declare section;
	char	buf[60];
	char	iidbdbName[60];
	exec sql end declare section;

	exec sql connect :dbname session :imaSession;
	sqlerr_check();
	dbConnection = TRUE;
	exec sql set lockmode session where readlock=nolock;
	exec sql execute procedure imp_complete_vnode;
	exec sql commit;    
/*
## mdf040903 
*/

	exec sql set_sql (session = :imaSession);

        SIGHANDLERS;

	return(TRUE);
}

/*
** Set up the IIDBDB connection for the relevant vnode.
*/
int
makeIidbdbConnection()
{

	exec sql begin declare section;
	char	buf[60];
	char	iidbdbName[60];
	exec sql end declare section;

	if (iidbdbConnection) {
		return(FALSE);
	}
	sprintf(iidbdbName,"%s::iidbdb",currentVnode);
	exec sql connect :iidbdbName session :iidbdbSession;
	sqlerr_check();
	exec sql set_sql (session = :imaSession);
	iidbdbConnection = TRUE;

        SIGHANDLERS;
	signal(SIGFPE,sighandler);

	return(TRUE);


}

int
closeIidbdbConnection()
{
	exec sql begin declare section;
	int     currentConnection;
	exec sql end declare section;

	if (iidbdbConnection) {
		iidbdbConnection = FALSE;
		exec sql inquire_sql (:currentConnection = session);
		exec sql set_sql (session = :iidbdbSession);
		exec sql disconnect;
		exec sql inquire_sql (:currentConnection = session);
		if (currentConnection) {
			exec sql set_sql (session = :currentConnection);
		}
	}
	return(TRUE);
}

int
closeDbConnections()
{
	exec sql begin declare section;
	int     currentConnection;
	exec sql end declare section;

	exec sql set_sql (session = :imaSession);
	exec sql inquire_sql (:currentConnection = session);
	if (currentConnection) {
		exec sql disconnect;
	}
	closeIidbdbConnection();
	return(TRUE);
}

void
sighandler(sig)
int	sig;
{
	exec sql begin declare section;
	char	msgBuf[100];
	exec sql end declare section;

	sprintf(msgBuf, "Received Signal %d - Exiting...",sig);
	if (inFormsMode) {
		exec frs set_frs frs (timeout = 0);
		exec frs message :msgBuf with style = popup;
		exec frs clear screen;
		exec frs endforms;
	} else {
		printf("%s\n",msgBuf);
	}
	exit(0);
}

int
fatal(str)
exec sql begin declare section;
char	*str;
exec sql end declare section;
{
	exec sql begin declare section;
	int	currentConnection;
	exec sql end declare section;

	exec sql inquire_sql (:currentConnection = session);

	if (inFormsMode) {
		exec frs set_frs frs (timeout = 0);
		exec frs message :str with style = popup;
		exec frs clear screen;
		exec frs endforms;
	} else {
		printf("%s\n",str);
	}
	if (currentConnection) {
		exec sql disconnect;
	}
	exit(0);
}
