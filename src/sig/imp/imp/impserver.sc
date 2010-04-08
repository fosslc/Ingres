/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name:	impserver.sc
**
** This file contains all of the routines needed to support the "SERVER" menu
** tree of the IMA based IMP lookalike tools.
**
**
** The original intention with the IMP tool was to build in-memory lists of
** everything - in a similar way to IPM. This is the only file that actually 
** does this - all the other routines simply use the forms as the method of
** storage.
** 
** Even if the lists were to be used - it would be an ideal to rewrite using
** QU.
**
## History:
##
##	10-feb-94	(johna)
##			created from routines in individual files
##			impdspsvr.sc
##			impdspsess.sc
##			impsesdet.sc
##			impsvrdet.sc
##
##	18-mar-94	(johna)
##			reworked to use better use the table keys to
##			avoid scans and to remove single vnode dependencies.
##	23-Mar-94	(johna)
##			removed references to "active sessions" and the query
##			flattening params
##	04-may-94	(johna)
##			some comments
##	30-Nov-94 	(johna)
##			added client_info popup.
##      02-Jul-2002 (fanra01)
##          Sir 108159
##          Added to distribution in the sig directory.
##      11-Nov-2002 (fanra01)
##          Bug 109104
##          Extend local variable for server_name so it doesn't run into
##          session_id.
##	03-Sep-03 	(flomi02) 
##mdf040903
##			added lastquery information
##	26-Mar-04 	(srisu02) 
##mdf260304
##			added call to newSessEntry()
##      06-Apr-04    (srisu02) 
##              Sir 108159
##              Integrating changes made by flomi02  
##      05-Oct-04    (srisu02)
##              Make function displayGccInfo() return different values
##              when there is information to display about the Net server
##              and when there is not. Based on this value, the Net server
##              'More_info' screen may or may not be displayed
##      05-Oct-04       (srisu02)
##              Removed unnecessary comments, corrected spelling mistakes 
##		in the code comments
## 17-Aug-2005  (hanje04)
##		BUG:115067
##		Make thread (in displayServerSessions()) 32bytes to handle
##		64bit session ID's
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "imp.h"

exec sql include sqlca;
exec sql include 'impcommon.sc';

exec sql begin declare section;

extern int inFormsMode;
extern int imp_server_list;
extern int imp_query_info;   /* 
## mdf040903 
*/
extern int imp_server_popup;
extern int imp_server_parms;
extern int imp_session_list;
extern int imp_session_detail;
extern int imp_client_info;
extern int timeVal;
extern GcnEntry		*GcnListHead;
extern SessEntry	*SessListHead;
extern char	*currentVnode;

exec sql end declare section;

exec sql begin declare section;
SessEntry       l_sess;
exec sql end declare section;

/* prototypes */
void getGcnList();
void displayGcnList();
void clearGcnEntryTable();
void display_details();
void getSessList();
void displaySessList();
void clearSessnEntryTable();
void displaySession();
void toggle_server_cols();
int displayGccInfo();


/*
** Name:	processServerList
**
** Display the list of known servers for the new IMA based IPM lookalike tool.
**
## History:
##
## 26-jan-94	(johna)
##		started from pervious ad-hoc programs
## 03-Feb-94	(johna)
##		added Control menuitem 
## 10-feb-94	(johna)
##		Moved into the impserver.sc file
*/

int
processServerList()
{
	exec sql begin declare section;
		GcnEntry	*current;
		char		tserverAddress[SERVER_NAME_LENGTH]; 
		char		addr[SERVER_NAME_LENGTH]; 
		char		vnode[SERVER_NAME_LENGTH]; 
	exec sql end declare section;

	exec frs display imp_server_list read;
	exec frs initialize;
	exec frs begin;
		exec frs putform imp_server_list (vnode = :currentVnode);
		getGcnList();
		exec frs inittable imp_server_list gcn_entry_tbl read;
		displayGcnList();
	exec frs end;

	exec frs activate menuitem 'More_info';
	exec frs begin;
		exec frs getrow imp_server_list gcn_entry_tbl 
				(:addr = serveraddress);
		current = GcnListHead;
		while (current != (GcnEntry *) NULL) {
			strcpy(tserverAddress,current->serverAddress);
			if (strncmp(current->serverAddress,addr,strlen(addr)) == 0) {
				break;
			} 
			current = current->n;
		}
		if (current) {
                        if (strcmp(current->serverClass,"IINMSVR") == 0) 
                        {
 exec frs message 'Name Servers do not have more info' with style = popup; 

			} 
			else
			if ((strcmp(current->serverClass,"COMSVR") == 0) ) {
				impGccInfo(current);
			} 
                        else {
				displayServerPopup(current);
			}
		}
	exec frs end;

	exec frs activate menuitem 'Parameters';
	exec frs begin;
		exec frs getrow imp_server_list gcn_entry_tbl 
				(:addr = serveraddress);
		current = GcnListHead;
		while (current != (GcnEntry *) NULL) {
			if (strncmp(current->serverAddress,addr,strlen(addr)) == 0) {
				break;
			} 
			current = current->n;
		}
		if (current) {
			if ((strcmp(current->serverClass,"IINMSVR") == 0) ||
				(strcmp(current->serverClass,"COMSVR") == 0)) {
				exec frs message 
		'Name Servers and Net Servers do not have parameters' 
					with style = popup;
			} else {
				displayServerParms(current);
			}
		}
	exec frs end;

	exec frs activate menuitem 'Internals';
	exec frs begin;
		exec frs getrow imp_server_list gcn_entry_tbl 
				(:addr = serveraddress);
		current = GcnListHead;
		while (current != (GcnEntry *) NULL) {
			if (strncmp(current->serverAddress,addr,strlen(addr)) == 0) {
				break;
			} 
			current = current->n;
		}
		if (current) {
			if ((strcmp(current->serverClass,"IINMSVR") == 0) 
                         || (strcmp(current->serverClass,"COMSVR") == 0) 
                        ) {
				exec frs message 
		'Sorry - cannot do internals for Name Servers or Net Servers' 
					with style = popup;
			} 
                        else {
				impInternalMenu(current);
			}
		}
	exec frs end;

	exec frs activate menuitem 'Sessions';
	exec frs begin;
		exec frs getrow imp_server_list gcn_entry_tbl 
				(:addr = serveraddress);
		current = GcnListHead;
		while (current != (GcnEntry *) NULL) {
			if (strncmp(current->serverAddress,addr,strlen(addr)) == 0) {
				break;
			} 
			current = current->n;
		}
		if (current) {
			if ((strcmp(current->serverClass,"IINMSVR") == 0) ||
				(strcmp(current->serverClass,"COMSVR") == 0)) {
				exec frs message 
		'Name Servers and Net Servers do not have sessions' 
					with style = popup;
			} else {
				displayServerSessions(current);
			}
		}
	exec frs end;

	exec frs activate menuitem 'Control';
	exec frs begin;
		exec frs display submenu;
		exec frs activate menuitem 'Soft_Shut';
		exec frs begin;
			shutdownProcess(SHUTDOWN_SOFT);
		exec frs end;

		exec frs activate menuitem 'Hard_Shut';
		exec frs begin;
			shutdownProcess(SHUTDOWN_HARD);
		exec frs end;

		exec frs activate menuitem 'Close_Listen';
		exec frs begin;
			shutdownProcess(SHUTDOWN_CLOSE);
		exec frs end;

		exec frs activate menuitem 'Open_Listen';
		exec frs begin;
			shutdownProcess(SHUTDOWN_OPEN);
		exec frs end;

		exec frs activate menuitem 'End', frskey3;
		exec frs begin;
			exec frs enddisplay;
		exec frs end;
	exec frs end;


	exec frs activate menuitem 'Refresh';
	exec frs begin;
		clearGcnEntryTable();
		getGcnList();
		displayGcnList();
	exec frs end;

	exec frs activate menuitem 'End', frskey3;
	exec frs begin;
	exec frs breakdisplay;
	exec frs end;

	return(TRUE);
}

/*
** Routines to handle the GCN linked list
**
*/

void freeGcnList() 
{
	GcnEntry	*local;

	while (GcnListHead != (GcnEntry *) NULL) {
		local = GcnListHead;
		GcnListHead = GcnListHead->n;
		(void) free((char *) local);
	}
}

GcnEntry        *
newGcnEntry()
{	
	GcnEntry        *local;
	GcnEntry        *here;

	local = (GcnEntry *) malloc((unsigned) sizeof(GcnEntry));
	if (local == (GcnEntry *) NULL) {
		return((GcnEntry *) NULL);
	}

	(void) memset((char *) local, 0, sizeof(GcnEntry));

	if (GcnListHead == (GcnEntry *) NULL) {
		GcnListHead = local;
		local->p = (GcnEntry *) NULL;
		local->n = (GcnEntry *) NULL;
		return(local);
	}

	here = GcnListHead;
	while (here->n != ((GcnEntry *) NULL)) {
		here = here->n;
	}

	here->n = local;
	local->p = here;
	local->n = (GcnEntry *) NULL;

	return(local);
}

/*
** getGcnList()
**
** Given the global pointer to the start of the list of known servers, 
** call IMA and populate the list
*/
void getGcnList()
{
	exec sql begin declare section;
	GcnEntry	l;
	GcnEntry	*n;
	exec sql end declare section;

	if (GcnListHead != (GcnEntry *) NULL) {
		freeGcnList();
	}
	exec sql repeated select 
		gcn,
		address,
		class,
		object,
		max_sessions,
		num_sessions,
		server_pid
	into
		:l.nameServer,
		:l.serverAddress,
		:l.serverClass,
		:l.serverObjects,
		:l.maxSessions,
		:l.numSessions,
		:l.serverPid
	from
		imp_gcn_view;
	exec sql begin;
		sqlerr_check();
		if (n = newGcnEntry()) {
			strcpy(n->nameServer,l.nameServer);
			strcpy(n->serverAddress,l.serverAddress);
			strcpy(n->serverClass,l.serverClass);
			strcpy(n->serverObjects,l.serverObjects);
			n->maxSessions = l.maxSessions;
			n->numSessions = l.numSessions;
			n->serverPid = l.serverPid;
		}
	exec sql end;
	sqlerr_check();
	exec sql commit;
		
}

void displayGcnList()
{
	/*
	** loop through the list of know server objects 
	*/

	exec sql begin declare section;
	GcnEntry	*local;
	exec sql end declare section;


	local = GcnListHead;
	while (local != (GcnEntry *) NULL) {
		exec frs loadtable imp_server_list gcn_entry_tbl (
			serveraddress = :local->serverAddress,
			serverclass = :local->serverClass,
			numsessions = :local->numSessions,
			serverdblist = :local->serverObjects);
		local = local->n;
	}
	exec frs redisplay;
}

void clearGcnEntryTable()
{
	exec frs inittable imp_server_list gcn_entry_tbl;
}


/*
** ShutdownProcess()
**
**	Shut down the specified server by calling the control menuitem
*/
int
shutdownProcess(how)
exec sql begin declare section;
int	how;
exec sql end declare section;
{
	exec sql begin declare section;
	char	buf[100];
	char	answer[100];
	char	addr[100];
	GcnEntry        *who;
	exec sql end declare section;

	exec frs getrow imp_server_list gcn_entry_tbl 
			(:addr = serveraddress);
	who = GcnListHead;
	while (who != (GcnEntry *) NULL) {
		if (strcmp(who->serverAddress,addr) == 0) {
			break;
		} 
		who = who->n;
	}

	if (	(strcmp(who->serverClass,"IINMSVR") == 0) ||
		(strcmp(who->serverClass,"IUSVR") == 0) ||
		(strcmp(who->serverClass,"COMSVR") == 0) )
		
		{
		sprintf(buf,
			"Don't know how to control %s (%s) with IMA yet",
			who->serverClass,
			who->serverAddress);
		exec frs message :buf with style = popup;
		return(TRUE);
	}
	/*
	** for now - cheat by building the name string directly
	*/
	strcpy(addr,currentVnode);
	strcat(addr,"::/@");
	strcat(addr,who->serverAddress);

	/*
	** we must have an INGRES, STAR, or user specified server
	*/


	/*
	** let the user bottle out..
	*/

	switch(how) {
	case SHUTDOWN_HARD: 
		sprintf(buf,"Confirm %s of server %s (%s) y/n: ",
			"Hard Shutdown",who->serverClass,addr);
		break;
	case SHUTDOWN_SOFT:
		sprintf(buf,"Confirm %s of server %s (%s) y/n: ",
			"Soft Shutdown",who->serverClass,addr);
		break;
	case SHUTDOWN_CLOSE:
		sprintf(buf,"Confirm %s of server %s (%s) y/n: ",
			"Close Listen ",who->serverClass,addr);
		break;
	case SHUTDOWN_OPEN:
		sprintf(buf,"Confirm %s of server %s (%s) y/n: ",
			"Open Listen",who->serverClass,addr);
		break;
	}
	exec frs prompt (:buf,:answer) with style = popup;
	if ((answer[0] != 'Y') && (answer[0] != 'y')) {
		exec frs message 'Cancelled . . .';
		PCsleep(2*1000);
		return(TRUE);
	}

	sprintf(buf,"%s (%s) ",who->serverClass,addr);
	switch(how) {
	case SHUTDOWN_HARD: 
		exec sql execute procedure imp_stop_server
			(server_id=:addr);
		strcat(buf,"Stopped");
		break;
	case SHUTDOWN_SOFT:
		exec sql execute procedure imp_shut_server
			(server_id=:addr);
		strcat(buf,"will shutdown");
		break;
	case SHUTDOWN_CLOSE:
		exec sql execute procedure imp_close_server
			(server_id=:addr);
		strcat(buf,"is closed");
		break;
	case SHUTDOWN_OPEN:
		exec sql execute procedure imp_open_server
			(server_id=:addr);
		strcat(buf,"is open");
		break;
	}
			
	exec frs message :buf with style = popup;
	return(TRUE);

}

/*
** Name:	displayServerPopup
**
** Display the detail of a server for the new IMA based IPM lookalike tool.
**
## History:
##
## 26-jan-94	(johna)
##		started on RIF day from pervious ad-hoc programs
## 04-Feb-94	(johna)
##		added the server startup param screen
## 10-mar-94	(johna)
##		moved into the impserver.sc file
*/
int
displayServerPopup(entry)
GcnEntry	*entry;
{
	exec frs display imp_server_popup read;
	exec frs initialize;
	exec frs begin;
		display_details(entry);
	exec frs end;

	exec frs activate menuitem 'End', frskey3;
	exec frs begin;
	exec frs breakdisplay;
	exec frs end;

	return(TRUE);
}


void display_details(entry)
exec sql begin declare section;
GcnEntry	*entry;
exec sql end declare section;
{
	exec sql begin declare section;
	char buf[31];
	exec sql end declare section;

	exec frs putform imp_server_popup (current_sessions = 
			:entry->numSessions);
	exec frs putform imp_server_popup (max_sessions = 
			:entry->maxSessions);
	exec frs putform imp_server_popup (server_name = 
			:entry->serverAddress);
	exec frs putform imp_server_popup (server_type = 
			:entry->serverClass);
	if (strcmp(entry->serverObjects,"*") == 0) {
		sprintf(buf,"All databases");
	} else {
		strcpy(buf,entry->serverObjects);
	}
	exec frs putform imp_server_popup (server_dblist = :buf);
	exec frs redisplay;
}

int
displayServerParms(entry)
GcnEntry	*entry;
{
	exec frs display imp_server_parms read;
	exec frs initialize;
	exec frs begin;
		if (display_parameters(entry) == FALSE) {
			exec frs breakdisplay;
		}
	exec frs end;

	exec frs activate menuitem 'Control';
	exec frs begin;
		exec frs display submenu;
		exec frs activate menuitem 'Soft_Shut';
		exec frs begin;
			shutdownProcess(SHUTDOWN_SOFT);
			if (display_parameters(entry) == FALSE) {
				exec frs breakdisplay;
			}
		exec frs end;

		exec frs activate menuitem 'Hard_Shut';
		exec frs begin;
			shutdownProcess(SHUTDOWN_HARD);
			if (display_parameters(entry) == FALSE) {
				exec frs breakdisplay;
			}
		exec frs end;

		exec frs activate menuitem 'Close_Listen';
		exec frs begin;
			shutdownProcess(SHUTDOWN_CLOSE);
			if (display_parameters(entry) == FALSE) {
				exec frs breakdisplay;
			}
		exec frs end;

		exec frs activate menuitem 'Open_Listen';
		exec frs begin;
			shutdownProcess(SHUTDOWN_OPEN);
			if (display_parameters(entry) == FALSE) {
				exec frs breakdisplay;
			}
		exec frs end;

		exec frs activate menuitem 'End', frskey3;
		exec frs begin;
			exec frs enddisplay;
		exec frs end;
	exec frs end;

	exec frs activate menuitem 'Refresh';
	exec frs begin;
		if (display_parameters(entry) == FALSE) {
			exec frs breakdisplay;
		}
	exec frs end;

	exec frs activate menuitem 'End', frskey3;
	exec frs begin;
	    exec frs breakdisplay;
	exec frs end;

	return(TRUE);
}


int
display_parameters(entry)
exec sql begin declare section;
GcnEntry	*entry;
exec sql end declare section;
{
	exec sql begin declare section;
	char buf[31];
	int	cnt;
	char msgbuf[31];
	char	local_vnode[SERVER_NAME_LENGTH];
	char	local_server[SERVER_NAME_LENGTH];
	int	local_cursors;
	char	local_capabilities[46];
	int	local_cursor_flags;
	int	local_fastcommit;
	int	local_flatten;
	int	local_listen_fails;
	int	local_name_service;
	int	local_no_star_cluster;
	int	local_noflatten;
	int	local_no_star_recov;
	int	local_nousers;
	int	local_incons_risk;
	int	local_rule_depth;
	int	local_connects;
	char	local_listen_state[10];
	int	local_max_connects;
	int	local_res_connects;
	char	local_shut_state[8];
	char	local_server_name[20];
	int	local_soleserver;
	char	local_svr_state[20];
	int	local_wbehind;
	exec sql end declare section;

	/*
	** cheat for now - assume the current vnode
	*/
	strcpy(buf,currentVnode);
	strcat(buf,"::/@");
	strcat(buf,entry->serverAddress);

	exec sql repeated select 
		server,
		cursors,
		capabilities,
		cursor_flags,
		fastcommit,
		listen_fails,
		name_service,
		no_star_cluster,
		nostar_recov,
		nousers,
		incons_risk,
		rule_depth,
		connects,
		listen_state,
		max_connects,
		res_connects,
		shut_state,
		server_name,
		soleserver,
		svr_state,
		wbehind
	into
		:local_server,
		:local_cursors,
		:local_capabilities,
		:local_cursor_flags,
		:local_fastcommit,
		:local_listen_fails,
		:local_name_service,
		:local_no_star_cluster,
		:local_no_star_recov,
		:local_nousers,
		:local_incons_risk,
		:local_rule_depth,
		:local_connects,
		:local_listen_state,
		:local_max_connects,
		:local_res_connects,
		:local_shut_state,
		:local_server_name,
		:local_soleserver,
		:local_svr_state,
		:local_wbehind
	from imp_server_parms
	where
		server = :buf; 
	sqlerr_check();

	exec sql inquire_sql (:cnt = rowcount);
	if (cnt == 0) {
		sprintf(msgbuf,"No information for server %s",buf);
		exec frs message :msgbuf with style = popup;
		return(FALSE);
	}

	exec frs putform imp_server_parms (server = 
			:local_server);
	exec frs putform imp_server_parms (svr_state = 
			:local_svr_state);
	exec frs putform imp_server_parms (shut_state = 
			:local_shut_state);
	sprintf(buf,"%s",local_name_service ? "ON" : "OFF");
	exec frs putform imp_server_parms (name_service = 
			:buf);
	exec frs putform imp_server_parms (listen_state = 
			:local_listen_state);
	exec frs putform imp_server_parms (server_name = 
			:local_server_name);
	exec frs putform imp_server_parms (listen_fails = 
			:local_listen_fails);
	exec frs putform imp_server_parms (connects = 
			:local_connects);
	exec frs putform imp_server_parms (max_connects = 
			:local_max_connects);
	exec frs putform imp_server_parms (nousers = 
			:local_nousers);
	exec frs putform imp_server_parms (res_connects = 
			:local_res_connects);
	exec frs putform imp_server_parms (capabilities = 
			:local_capabilities);
	sprintf(buf,"%s",local_fastcommit ? "ON" : "OFF");
	exec frs putform imp_server_parms (fastcommit = 
			:buf);
	sprintf(buf,"%s",local_soleserver ? "ON" : "OFF");
	exec frs putform imp_server_parms (soleserver = 
			:buf);
	sprintf(buf,"%s",local_incons_risk ? "ON" : "OFF");
	exec frs putform imp_server_parms (incons_risk = 
			:buf);
	exec frs putform imp_server_parms (wbehind = 
			:local_wbehind);
	sprintf(buf,"%s",local_no_star_recov ? "ON" : "OFF");
	exec frs putform imp_server_parms (nostar_recov = 
			:buf);
	exec frs putform imp_server_parms (rule_depth = 
			:local_rule_depth);
	exec frs putform imp_server_parms (cursors = 
			:local_cursors);
	switch(local_cursor_flags) {
	case 1:	sprintf(buf,"DIRECT_UPDATE(1)"); break;
	case 2:	sprintf(buf,"DEFERRED_UPDATE(2)"); break;
	default:sprintf(buf,"Unknown flag %d",local_cursor_flags);
	}
	exec frs putform imp_server_parms (cursor_flags = 
			:buf);
	exec frs redisplay;
	return(TRUE);
}

/*
** Name:	displayServerSessions
**
** Display the list of known sessions for the new IMA based IPM lookalike tool.
**
## History:
##
## 26-jan-94	(johna)
##		started on RIF day from pervious ad-hoc programs
## 10-mar-94	(johna)
##		moved to the impserver.sc file
## 17-Aug-2005  (hanje04)
##		BUG:115067
##		Make thread 32bytes to handle 64bit session ID's
##
*/

int
displayServerSessions(server)
GcnEntry	*server;
{
	exec sql begin declare section;
		GcnEntry	*t;
		SessEntry	*current_sess;
		char		thread[32];
		char	server_name[SERVER_NAME_LENGTH];
		char	buf[100];
		char	answer[100];
        int     resource_toggle;    /*
##mdf040903
*/
	exec sql end declare section;

	exec frs display imp_session_list read;
	exec frs initialize;
	exec frs begin;
        resource_toggle = FALSE;    /*
##mdf040903
*/
            toggle_server_cols(&resource_toggle); 
		t = GcnListHead;
		while (t != (GcnEntry *) NULL) {
			if (t->serverPid == server->serverPid) {
				sprintf(server_name,	
					"%s::/@%s",
					currentVnode,t->serverAddress);
				break;
			}
			t = t->n;
		}
		/*
		** reset the domain at this point to be just the server 
		** we are interested in - that way the IMA gateway does not
		** have to collect things it is not interested in
		*/
		exec sql execute procedure imp_reset_domain;

		exec sql execute procedure imp_set_domain(entry = :server_name);

		/*
		** FIXME - set the correct domain at this point
		*/
		exec sql commit;

		getSessList(server_name);

		exec frs inittable imp_session_list svr_sess_tbl read;
		exec frs putform imp_session_list (server_name =
					:server_name);
		displaySessList();
	exec frs end;

	exec frs activate menuitem 'More_info';
	exec frs begin;
		exec frs getrow imp_session_list svr_sess_tbl 
				(:thread = session_id);
		current_sess = SessListHead;
		while (current_sess != (SessEntry *) NULL) {
			if (strcmp(current_sess->sessionId,thread) == 0) {
				break;
			} 
			current_sess = current_sess->n;
		}
		if (current_sess) {
			displaySessionDetail(current_sess,server_name);
		}
	exec frs end;

	exec frs activate menuitem 'DBA_Operations';
	exec frs begin;
		exec frs getrow imp_session_list svr_sess_tbl
			(:thread = session_id);
		current_sess = SessListHead;
		while (current_sess != (SessEntry *) NULL) {
			if (strcmp(current_sess->sessionId,thread) == 0) {
				break;
			} 
			current_sess = current_sess->n;
		}
		exec frs display submenu;
			exec frs activate menuitem 'Remove';
			exec frs begin;
				sprintf(buf,
				"Confirm Removal of session %s (%s) y/n: ",
				current_sess->sessionId,
				current_sess->username);
			
				exec frs prompt (:buf,:answer) 
					with style = popup;
				if ((answer[0] != 'Y') && (answer[0] != 'y')) {
					exec frs message 'Cancelled . . .';
					PCsleep(2*1000);
				} else {
					exec sql execute procedure 
						imp_remove_session (
							session_id = 
						:current_sess->sessionId,
							 server_id = 
						:current_sess->serverAddress);
					exec sql commit;
				}
			exec frs end;

		exec frs activate menuitem 'End', frskey3;
		exec frs begin;
			exec frs enddisplay;
		exec frs end;
	exec frs end;

	exec frs activate menuitem 'Client_Info';
	exec frs begin;
		exec frs getrow imp_session_list svr_sess_tbl 
				(:thread = session_id);
		current_sess = SessListHead;
		while (current_sess != (SessEntry *) NULL) {
			if (strcmp(current_sess->sessionId,thread) == 0) {
				break;
			} 
			current_sess = current_sess->n;
		}
		if (current_sess) {
			displayClientInfo(server_name,thread);
		}
	exec frs end;

	exec frs activate menuitem 'Resource_Toggle';
	exec frs begin;
            toggle_server_cols(&resource_toggle); 
	exec frs end;

	exec frs activate menuitem 'Refresh';
	exec frs begin;
	    clearSessnEntryTable();
	    getSessList(server_name);
	    displaySessList();
	exec frs end;

	exec frs activate menuitem 'End', frskey3;
	exec frs begin;
	    exec frs breakdisplay;
	exec frs end;

	/*
	** reset the domain to include the whole installation
	*/
	exec sql repeated select dbmsinfo('ima_vnode') into :buf;
	sqlerr_check();
	if (strcmp(buf,currentVnode) == 0) {
		exec sql execute procedure imp_complete_vnode;
		exec sql commit;
	}
	return(TRUE);
}

/*
** Routines to handle the session linked list
*/
void freeSessList() 
{
	SessEntry	*local;

	while (SessListHead != (SessEntry *) NULL) {
		local = SessListHead;
		SessListHead = SessListHead->n;
		(void) free((char *) local);
	}
}

SessEntry        * newSessEntry()
{	
	SessEntry        *local;
	SessEntry        *here;

	local = (SessEntry *) malloc((unsigned) sizeof(SessEntry));
	if (local == (SessEntry *) NULL) {
		return((SessEntry *) NULL);
	}

	(void) memset((char *) local, 0, sizeof(SessEntry));

	if (SessListHead == (SessEntry *) NULL) {
		SessListHead = local;
		local->p = (SessEntry *) NULL;
		local->n = (SessEntry *) NULL;
		return(local);
	}

	here = SessListHead;
	while (here->n != ((SessEntry *) NULL)) {
		here = here->n;
	}

	here->n = local;
	local->p = here;
	local->n = (SessEntry *) NULL;

	return(local);
}

/*
** getSessList()
**
** Given the global pointer to the start of the list of known servers, 
** call IMA and populate the list
**
** MDF200302 Sort on database (descending )
*/
void getSessList(serverName)
exec sql begin declare section;
char	*serverName;
exec sql end declare section;
{
	exec sql begin declare section;
	SessEntry	*n_sess;
	char	hack_buf[100];
	exec sql end declare section;

	if (SessListHead != (SessEntry *) NULL) {
		freeSessList();
	}

	exec sql repeated select 
		server, 
		session_id, 
		state, 
		wait_reason,
		mask, 
		priority,
		thread_type, 
		timeout, 
		smode, 
		uic, 
		cputime,
		dio,
		bio,
		locks,
		username,
		assoc_id,
		euser,
		ruser,
		database,
		dblock,
		facidx,
		facility,
		activity,
		act_detail,
		query,
		dbowner,
		terminal,
		s_name,
		groupid,
		role,
		appcode,
		scb_pid
	into
		:l_sess.serverAddress,
		:l_sess.sessionId,
		:l_sess.state,
		:l_sess.wait_reason,
		:l_sess.mask,
		:l_sess.priority,
		:l_sess.thread_type,
		:l_sess.timeout,
		:l_sess.mode,
		:l_sess.uic,
		:l_sess.cputime,
		:l_sess.dio,
		:l_sess.bio,
		:l_sess.locks,
		:l_sess.username,
		:l_sess.assoc_id,
		:l_sess.euser,
		:l_sess.ruser,
		:l_sess.database,
		:l_sess.dblock,
		:l_sess.facility_id,
		:l_sess.facility,
		:l_sess.activity,
		:l_sess.act_detail,
		:l_sess.query,
		:l_sess.dbowner,
		:l_sess.terminal,
		:l_sess.s_name,
		:l_sess.groupid,
		:l_sess.role,
		:l_sess.appcode,
		:l_sess.scb_pid
	from
		imp_session_view 
	where
		server = trim(:serverName)
        order by database desc;
	exec sql begin;
		sqlerr_check();
		if (n_sess = newSessEntry()) {
			strcpy(n_sess->serverAddress,l_sess.serverAddress);
			strcpy(n_sess->sessionId,l_sess.sessionId);
			strcpy(n_sess->state,l_sess.state);
			if (strncmp(l_sess.wait_reason,"00",2) == 0) {
				sprintf(n_sess->wait_reason,"COM");
			} else {
				strcpy(n_sess->wait_reason,l_sess.wait_reason);
			}
			strcpy(n_sess->mask,l_sess.mask);
			n_sess->priority - l_sess.priority;
			strcpy(n_sess->thread_type,l_sess.thread_type);
			n_sess->timeout - l_sess.timeout;
			n_sess->mode - l_sess.mode;
			n_sess->uic - l_sess.uic;
			n_sess->cputime - l_sess.cputime;
			n_sess->bio - l_sess.bio;
			n_sess->dio - l_sess.dio;
			n_sess->locks - l_sess.locks;
			strcpy(n_sess->username,l_sess.username);
			n_sess->assoc_id - l_sess.assoc_id;
			strcpy(n_sess->euser,l_sess.euser);
			strcpy(n_sess->ruser,l_sess.ruser);
			if (strncmp(l_sess.database,"<no_",4) == 0) {
				sprintf(n_sess->database,"NONE");
			} else {
				strcpy(n_sess->database,l_sess.database);
			}
			strcpy(n_sess->dblock,l_sess.dblock);
			n_sess->facility_id - l_sess.facility_id;
			if (l_sess.facility_id == 0) {
				sprintf(n_sess->facility,"   ");
			} else {
				strcpy(n_sess->facility,l_sess.facility);
			}
			strcpy(n_sess->activity,l_sess.activity);
			strcpy(n_sess->act_detail,l_sess.act_detail);
			strcpy(n_sess->query,l_sess.query);
			strcpy(n_sess->dbowner,l_sess.dbowner);
			strcpy(n_sess->terminal,l_sess.terminal);
			if (l_sess.s_name[0] == ' ') {
				strcpy(n_sess->s_name, (char *) (l_sess.s_name + 1));
			} else {
				strcpy(n_sess->s_name,l_sess.s_name);
			}
			strcpy(n_sess->groupid,l_sess.groupid);
			strcpy(n_sess->role,l_sess.role);
			n_sess->appcode = l_sess.appcode;
			n_sess->scb_pid = l_sess.scb_pid;
		}
	exec sql end;
	sqlerr_check();
	exec sql commit;
		
}

void displaySessList()
{
	/*
	** loop through the list of know server objects 
	*/

	exec sql begin declare section;
	SessEntry	*local_sess;
	exec sql end declare section;


	local_sess = SessListHead;
	while (local_sess != (SessEntry *) NULL) {
		exec frs loadtable imp_session_list svr_sess_tbl (
			session_id = :local_sess->sessionId,
			session_name = :local_sess->s_name,
			terminal = :local_sess->terminal,
			database = :local_sess->database,
			cputime = :local_sess->cputime,
			dio = :local_sess->dio,
			state = :local_sess->wait_reason,
			facility = :local_sess->facility,
			query = :local_sess->query);
		local_sess = local_sess->n;
	}
	exec frs redisplay;
}

void clearSessnEntryTable()
{
	exec frs inittable imp_session_list svr_sess_tbl;
}

/*
** Name:	impdspses.sc
**
** Display session detail information for the new IMA based IPM lookalike tool.
**
## History:
##
## 27-jan-94	(johna)
##		started from pervious ad-hoc programs
## 10-feb-94	(johna)
##		moved into the impserver.sc file
##
## 24-apr-02    (
##mdf240402)
##		Don't use scb_pid in the WHERE clause
*/
int displaySessionDetail(sess,server)
SessEntry	*sess;
char		*server;
{
/*
	exec sql begin declare section;
		char		buf[100];
	exec sql end declare section;
*/

	exec frs display imp_session_detail read;
	exec frs initialize;
	exec frs begin;
		exec frs set_frs frs (timeout = :timeVal);

		if (getSessionInfo(sess,server)) {
			displaySession();
		} 
                else {
			exec frs set_frs frs (timeout = 0);
			exec frs message 'That session has exited.' 
				with style = popup;
			exec frs breakdisplay;
		}
	exec frs end;

	exec frs activate menuitem 'Client_Info';
	exec frs begin;
		displayClientInfo(server,sess->sessionId);
	exec frs end;

	exec frs activate menuitem 'More Query';
	exec frs begin;
		displayQueryInfo(server,sess->sessionId);
			displaySession();
	exec frs end;

	exec frs activate menuitem 'Refresh';
	exec frs begin;
		if (getSessionInfo(sess, server)) {
			displaySession();
		} else {
			exec frs set_frs frs (timeout = 0);
			exec frs message 'That session has exited.' 
				with style = popup;
			exec frs breakdisplay;
		}
	exec frs end;

	exec frs activate timeout;
	exec frs begin;
		if (getSessionInfo(sess,server)) {
			displaySession();
		} else {
			exec frs set_frs frs (timeout = 0);
			exec frs message 'That session has exited.' 
				with style = popup;
			exec frs breakdisplay;
		}
		displaySession();
	exec frs end;

	exec frs activate menuitem 'End', frskey3;
	exec frs begin;
	    exec frs set_frs frs (timeout = 0);
	    exec frs breakdisplay;
	exec frs end;

	return(TRUE);
}

int
getSessionInfo(p_sess,p_server)
exec sql begin declare section;
SessEntry	*p_sess;
char	*p_server;
exec sql end declare section;
{
	/* reselect the information in case it has changed */

	exec sql begin declare section;
	int i;
	int j;
	char	buf[100];
	exec sql end declare section;

	i = 0;
	exec sql repeated select
		server,
		session_id,
		state,
		wait_reason,
		mask,
		priority,
		thread_type,
		timeout,
		smode,
		uic,
		cputime,
		dio,
		bio,
		locks,
		username,
		assoc_id,
		euser,
		ruser,
		database,
		dblock,
		facidx,
		facility,
		activity,
		act_detail,
		query,
		dbowner,
		terminal,
		s_name,
		groupid,
		role,
		appcode,
		scb_pid
	into
		:l_sess.serverAddress,
		:l_sess.sessionId,
		:l_sess.state,
		:l_sess.wait_reason,
		:l_sess.mask,
		:l_sess.priority,
		:l_sess.thread_type,
		:l_sess.timeout,
		:l_sess.mode,
		:l_sess.uic,
		:l_sess.cputime,
		:l_sess.dio,
		:l_sess.bio,
		:l_sess.locks,
		:l_sess.username,
		:l_sess.assoc_id,
		:l_sess.euser,
		:l_sess.ruser,
		:l_sess.database,
		:l_sess.dblock,
		:l_sess.facility_id,
		:l_sess.facility,
		:l_sess.activity,
		:l_sess.act_detail,
		:l_sess.query,
		:l_sess.dbowner,
		:l_sess.terminal,
		:l_sess.s_name,
		:l_sess.groupid,
		:l_sess.role,
		:l_sess.appcode,
		:l_sess.scb_pid
	from
		imp_session_view
	where
		session_id = :p_sess->sessionId and
		server = :p_server;
	if ((sqlca.sqlcode != 0) && 
		(sqlca.sqlcode != 100)) 
	{
		sprintf(buf,"Sqlca.sqlcode is %d",sqlca.sqlcode);
		exec frs set_frs frs (timeout = 0);
		exec frs message :buf with style = popup;
		exec frs set_frs frs (timeout = :timeVal);
	}
	exec sql inquire_sql (:i = rowcount);
	exec sql commit;
	if (i == 0) {
		return(FALSE);
	}
/* 
##mdf040902  
Get lastquery info */
	exec sql repeated select 
            session_lastquery into :l_sess.lastquery
	from
		imp_lastquery
	where
	    session_id = :p_sess->sessionId
	and server = :p_server;
	if ((sqlca.sqlcode != 0) && 
		(sqlca.sqlcode != 100)) 
	{
		sprintf(buf,"Sqlca.sqlcode is %d",sqlca.sqlcode);
		exec frs set_frs frs (timeout = 0);
		exec frs message :buf with style = popup;
		exec frs set_frs frs (timeout = :timeVal);
	}
	exec sql commit;
        /* We are not interested in a zero rowcount */

	return(TRUE);
}

void displaySession()
{
	exec sql begin declare section;
	    char	lbuf[300];    /* 
##mdf040903 
Made this string longer */
	exec sql end declare section;

	exec frs putform imp_session_detail (session_id = :l_sess.sessionId);
	exec frs putform imp_session_detail (session_id = :l_sess.sessionId);
	exec frs putform imp_session_detail (server_addr = :l_sess.serverAddress);
	exec frs putform imp_session_detail (session_name = :l_sess.username);
	exec frs putform imp_session_detail (terminal = :l_sess.terminal);
	if (strncmp(l_sess.wait_reason,"00",2) == 0) {
		sprintf(lbuf,"CS_COMPUTABLE");
	} else {
		sprintf(lbuf,"%s (%s)",l_sess.state,l_sess.wait_reason);
	}
	exec frs putform imp_session_detail (state_description = :lbuf);
	exec frs putform imp_session_detail (mask = :l_sess.mask);
	exec frs putform imp_session_detail (ruser = :l_sess.ruser);
	exec frs putform imp_session_detail (euser = :l_sess.euser);
	exec frs putform imp_session_detail (database = :l_sess.database);
	exec frs putform imp_session_detail (dba = :l_sess.dbowner);
	exec frs putform imp_session_detail (group_id = :l_sess.groupid);
	exec frs putform imp_session_detail (role = :l_sess.role);
	exec frs putform imp_session_detail (dblock = :l_sess.dblock);
	switch (l_sess.facility_id) {
	    case 0: sprintf(lbuf,"(none)"); break;
	    case 1: sprintf(lbuf,"CLF (Compatibility Lib)"); break;
	    case 2: sprintf(lbuf,"ADF (Abstract Datatype Facility)"); break;
	    case 3: sprintf(lbuf,"DMF (Data Manipulation Facility)"); break;
	    case 4: sprintf(lbuf,"OPF (Optimizer Facility)"); break;
	    case 5: sprintf(lbuf,"PSF (Parser Facility)"); break;
	    case 6: sprintf(lbuf,"QEF (Query Execution Facility)"); break;
	    case 7: sprintf(lbuf,"QSF (Query Storage Facility)"); break;
	    case 8: sprintf(lbuf,"RDF (Relation Destriptor Facility)"); break;
	    case 9: sprintf(lbuf,"SCF (System Control Facility)"); break;
	    case 10:sprintf(lbuf,"ULF (Utility Facility)"); break;
	    case 11:sprintf(lbuf,"DUF (Database Utility Facility)"); break;
	    case 12:sprintf(lbuf,"GCF (General Communication facility)"); break;
	    case 13:sprintf(lbuf,"RQF (Remote Query facility)"); break;
	    case 14:sprintf(lbuf,"TPF (Transaction Processing facility)"); break;
	    case 15:sprintf(lbuf,"GWF (Gateway facility)"); break;
	    case 16:sprintf(lbuf,"SXF (Security Extensions facility)"); break;
	    default:sprintf(lbuf,"Unknown facility code %d",l_sess.facility_id);
	}

	exec frs putform imp_session_detail (facility_desc = :lbuf);
	exec frs putform imp_session_detail (app_code = :l_sess.appcode);
	exec frs putform imp_session_detail (activity = :l_sess.activity);
	exec frs putform imp_session_detail (activity_detail = :l_sess.act_detail);
	exec frs putform imp_session_detail (query = :l_sess.query);
	exec frs putform imp_session_detail (lastquery = :l_sess.lastquery);
	exec frs redisplay;
}

displayDomain(str)
char	*str;
{
	exec sql begin declare section;
	char buf1[300];
	char buf[30];
	exec sql end declare section;

	if ((str) && (*str)) {
		strcpy(buf1, str);
		strcat(buf1, " - Domain is ");
	} else {
		strcpy(buf1,"Domain is ");
	}
	exec sql repeated select domplace into :buf from imp_domain;
	exec sql begin;
		strcat(buf1,buf);
		strcat(buf1,", ");
	exec sql end;

	exec frs message :buf1 with style = popup;
}

/*
** display the client information for this session
*/
displayClientInfo(server,session)
exec sql begin declare section;
char	*server;
char	*session;
exec sql end declare section;
{
	exec sql begin declare section;
	char	client_server[21];
	char	client_session_id[21];
	char	client_pid[21];
	char	client_host[21];
	char	client_user[21];
	char	client_tty[21];
	char	client_connection[31];
	char	client_info[100];
	int	rowsFound = 0;
	exec sql end declare section;

	exec frs display imp_client_info read;
	exec frs initialize;
	exec frs begin;
	    exec frs set_frs frs (timeout = 0);
	    exec sql repeated select 
		server,
		session_id,
		client_host,
		client_pid,
		client_tty,
		client_user,
		client_info,
		client_connect
	    into
		:client_server,
		:client_session_id,
		:client_host,
		:client_pid,
		:client_tty,
		:client_user,
		:client_info,
		:client_connection
	    from
		imp_client_info
	    where
		    server = :server
	    and	session_id = :session;
            if (sqlerr_check() == NO_ROWS) {                              
		exec frs message 'That session has exited' 
			with style = popup;
		exec frs breakdisplay;
	    } else {
                                                              
		exec frs putform(
			client_user = :client_user,
			client_host = :client_host,
			client_tty = :client_tty,
			client_pid = :client_pid,
			client_connection = :client_connection,
			client_info = :client_info);
		exec frs redisplay;
	    }
	exec frs end;

	exec frs activate menuitem 'End', frskey3;
	exec frs begin;
	    exec frs breakdisplay;
	exec frs end;

	return(TRUE);
}
			
			


/*
** 
##mdf040903
** display the full query information for this session
*/
int
displayQueryInfo(server,p_session)
exec sql begin declare section;
char	*server;
char	*p_session;
exec sql end declare section;
{
	/* reselect the information in case it has changed */

	exec sql begin declare section;
	int rowsFound;
	int j;
	char	buf[100];
        SessEntry       *local_sess = (SessEntry*)NULL;
	exec sql end declare section;

	rowsFound = 0;

	exec frs display imp_query_info read;
	exec frs initialize;
	exec frs begin;
		exec frs set_frs frs (timeout = 0);
		local_sess = newSessEntry() ;/* 
##mdf260304 
*/
                strcpy(local_sess->sessionId,p_session );
                if (getSessionInfo(local_sess, server)) 
                {
                    exec frs putform(                      
                            lastquery = :l_sess.lastquery, 
                            query = :l_sess.query          
                            );                             
                    exec frs redisplay;                    
                } else {                                                   
                        exec frs set_frs frs (timeout = 0);                
                        exec frs message 'That l_session has exited.'        
                                with style = popup;                        
                        exec frs breakdisplay;                             
                }                                                          
        exec frs end;                                                      

        exec frs activate menuitem 'Refresh';                              
        exec frs begin;                                                    
                strcpy(local_sess->sessionId,p_session );
                if (getSessionInfo(local_sess, server)) 
                {
                    exec frs putform(                      
                            lastquery = :l_sess.lastquery, 
                            query = :l_sess.query          
                            );                             
                    exec frs redisplay;                    
                } else {                                                   
                        exec frs set_frs frs (timeout = 0);                
                        exec frs message 'That p_session hhas exited.'        
                                with style = popup;                        
                        exec frs breakdisplay;                             
                }                                                          
        exec frs end;                                                      

	exec frs activate menuitem 'End', frskey3;
	exec frs begin;
	    exec frs breakdisplay;
	exec frs end;

	return(TRUE);
}

void toggle_server_cols(p_resource_toggle)
exec sql begin declare section;
int	*p_resource_toggle;
exec sql end declare section;
{
    if (*p_resource_toggle) {
        exec frs set_frs column imp_session_list svr_sess_tbl (
                    invisible(cputime) = 0
                    ,invisible(dio) = 0
                    ,invisible(state) = 1
                    ,invisible(facility) = 1
                    );
        *p_resource_toggle = FALSE;
    }
    else {
        exec frs set_frs column imp_session_list svr_sess_tbl (
                    invisible(cputime) = 1
                    ,invisible(dio) = 1
                    ,invisible(state) = 0
                    ,invisible(facility) = 0
                    );
        *p_resource_toggle = TRUE;
    }
}

/*
** Monitor Traffic for Specific GCC Server
**
**
## History:
##      04-Sep-03       (
##mdf040903)                     
*/
int
impGccInfo(p_server)
GcnEntry	*p_server;
{
    exec sql begin declare section;
	int	i;
	char	buf[60];
    exec sql end declare section;

	exec frs display imp_gcc_info read;
	exec frs initialize;
	exec frs begin;
		exec frs set_frs frs (timeout = :timeVal);

		exec sql repeated select dbmsinfo('ima_vnode') into :buf;
		sqlerr_check();
		exec sql execute procedure imp_reset_domain;
		exec sql execute procedure 
				imp_set_domain(entry=:currentVnode);
		exec sql commit;  /*
##mdf040903
*/

		if(displayGccInfo(p_server) == FALSE)
                {
                  exec frs breakdisplay;
                }

	exec frs end;


	exec frs activate menuitem 'Refresh';
	exec frs begin;
		displayGccInfo(p_server);
	exec frs end;

	exec frs activate timeout;
	exec frs begin;
		displayGccInfo(p_server);
	exec frs end;


	exec frs activate menuitem 'End', frskey3;
	exec frs begin;
		exec frs set_frs frs (timeout = 0);
		exec frs breakdisplay;
	exec frs end;

	exec sql execute procedure imp_complete_vnode;
	exec sql commit;   /*
##mdf040903
*/
	return(TRUE);
}

int displayGccInfo(p_server)
GcnEntry	*p_server;
{
    exec sql begin declare section;      
        int  l_conn_count;
        int  l_data_in;
        int  l_data_out;
        char l_ib_encrypt_mech[13];
        char l_ib_encrypt_mode[13];
        int  l_inbound_current;
        int  l_inbound_max;
        int  l_msgs_in;
        int  l_msgs_out;
        char l_net_server[65];
        char l_ob_encrypt_mech[13];
        char l_ob_encrypt_mode[13];
        char l_outbound_current;
        int  l_outbound_max;
        int  l_pl_protocol;
        char l_registry_mode[13];
        int  l_trace_level;
        char l_msgbuf[100];                    
    exec sql end declare section;        

    strcpy(l_net_server,currentVnode);
    strcat(l_net_server,"::/@");                                         
    strcat(l_net_server,p_server->serverAddress);                          
    exec frs putform imp_gcc_info (net_server = :l_net_server);

    exec sql repeated select 
        int4(conn_count)
        ,int4(data_in)
        ,int4(data_out)
        ,ib_encrypt_mech
        ,ib_encrypt_mode
        ,int4(inbound_current)
        ,int4(inbound_max)
        ,int4(msgs_in)
        ,int4(msgs_out)
        ,ob_encrypt_mech
        ,ob_encrypt_mode
        ,int4(outbound_current)
        ,int4(outbound_max)
        ,int4(pl_protocol)
        ,registry_mode
        ,int4(trace_level)
    into
        :l_conn_count
        , :l_data_in
        , :l_data_out
        , :l_ib_encrypt_mech
        , :l_ib_encrypt_mode
        , :l_inbound_current
        , :l_inbound_max
        , :l_msgs_in
        , :l_msgs_out
        , :l_ob_encrypt_mech
        , :l_ob_encrypt_mode
        , :l_outbound_current
        , :l_outbound_max
        , :l_pl_protocol
        , :l_registry_mode
        , :l_trace_level	

    from imp_gcc_info
    where net_server = :l_net_server;
    if (sqlerr_check() == NO_ROWS) {                              
        exec sql commit;    
	 exec frs set_frs frs (timeout = 0);
        sprintf(l_msgbuf,"No information for Net Server %s",l_net_server);
        exec frs message :l_msgbuf with style = popup;                  
        return(FALSE);
    }                                                             
    else {                                                        
        exec sql commit;                                           

        exec frs putform imp_gcc_info (conn_count= :l_conn_count);
        exec frs putform imp_gcc_info (data_in= :l_data_in);
        exec frs putform imp_gcc_info (data_out= :l_data_out);
        exec frs putform imp_gcc_info (ib_encrypt_mech= :l_ib_encrypt_mech);
        exec frs putform imp_gcc_info (ib_encrypt_mode= :l_ib_encrypt_mode);
        exec frs putform imp_gcc_info (inbound_current= :l_inbound_current);
        exec frs putform imp_gcc_info (inbound_max= :l_inbound_max);
        exec frs putform imp_gcc_info (msgs_in= :l_msgs_in);
        exec frs putform imp_gcc_info (msgs_out= :l_msgs_out);
        exec frs putform imp_gcc_info (net_server= :l_net_server);
        exec frs putform imp_gcc_info (ob_encrypt_mech= :l_ob_encrypt_mech);
        exec frs putform imp_gcc_info (ob_encrypt_mode= :l_ob_encrypt_mode);
        exec frs putform imp_gcc_info (outbound_current= :l_outbound_current);
        exec frs putform imp_gcc_info (outbound_max= :l_outbound_max);
        exec frs putform imp_gcc_info (pl_protocol= :l_pl_protocol);
        exec frs putform imp_gcc_info (registry_mode= :l_registry_mode);
        exec frs putform imp_gcc_info (trace_level= :l_trace_level);
        return(TRUE);
    }
}

