/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name:	imp.sc
**
** Driving routine for the new IMA based IPM lookalike tool.
**
** This routine really needs to parse command line args properly (in addition
** to the as-yet-unwritten options screen.
**
** General comments for the whole utility - error checking does not exist or
** does not work (what do you expect for a prototype) - also there seems to be
** some problem with the session switching logic.
**
** Each form needs to display the auto-timeout value (if set) or some other
** suiutable piece of information.
**
** The whole code should be reworked with the CL.
**
## History:
##
## 26-jan-94	(johna)
##		started on RIF day from pervious ad-hoc programs
## 04-may-94	(johna)
##		some comments
##
## 02-Jul-2002 (fanra01)
##      Sir 108159
##      Added to distribution in the sig directory.
## 06-Apr-04    (srisu02) 
##              Sir 108159
##              Integrating changes made by flomi02  
*/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "imp.h"

exec sql include sqlca;
exec sql begin declare section;
extern int VnodeNotDefined = TRUE;            /*
##mdf040903
*/
extern int timeVal;
extern char	*currentVnode;
exec sql end declare section;

# ifndef DATABASE
# define DATABASE "imadb"
# endif

void getVnode();
void getTimeVal();
void printerr();

int
main(argc,argv)
int	argc;
char	*argv[];
{

	exec sql begin declare section;
	char	dbname[30];
	char	argbuf[30];
	int	cnt = 1;
	int	spareInt = 0;
	exec sql end declare section;

	VnodeNotDefined = TRUE;             /*
##mdf040903
*/

	/*
	** process the command line args
	*/
    	strcpy(dbname, DATABASE);
	while (cnt < argc) {
		if (*argv[cnt] != '-') {
			/*
			** assume a database name
			*/
			strcpy(dbname,argv[cnt]);
		} else {
			/*
			** flag - process as appropriate
			*/
			strncpy(argbuf,argv[cnt],30);
			switch(argbuf[1]) {
				case 'r':
					getTimeVal(argbuf);
					break;
				/* 
##mdf040903 { 
*/
				case 'v':
					VnodeNotDefined = FALSE;
					VnodeNotDefined = TRUE;
					getVnode(argbuf);
					break;
				/* 
##mdf040903 } 
*/
				default:
					printerr(argbuf);
			}
		}
		cnt++;
	}
	exec sql set_sql (tracefile = 'imp.trace');
	makeDbConnections(dbname);
	exec sql select dbmsinfo('ima_vnode') into :dbname;
	sqlerr_check();
        if (VnodeNotDefined == TRUE) {
	    currentVnode = dbname;
        }
	/* 
	** don't make the iidbdb connection until we need to - in 
	** case the user switches Vnodes on us before we goo looking
	*/
	/* makeIidbdbConnection(); */
	setupFrs();
	startMenu();
	closeFrs();
	closeDbConnections();

}

void getVnode(buf)
char	*buf;
{
	char	nodeval[30];

	if (sscanf(buf,"-v%s",nodeval) != 1) {
		fprintf(stderr,
			"Ignoring \"%s\" - not a valid node value\n",
			buf);
	} else {
		currentVnode = nodeval;
/*exec frs message :currentVnode; exec frs sleep 3; */
	}
}

void getTimeVal(buf)
char	*buf;
{
	int	spareInt = 0;

	if (sscanf(buf,"-r%d",&spareInt) != 1) {
		fprintf(stderr,
			"Ignoring \"%s\" - not a valid timeout value\n",
			buf);
	} else {
		timeVal = spareInt;
	}
}

void printerr(buf)
char	*buf;
{
	fprintf(stderr,
	"Ignoring \"%s\" - not a valid flag\n",
	buf);
}
