/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name:	implocks.sc
**
** This file contains all of the routines needed to support the "LOCKS" menu
** tree of the IMA based IMP lookalike tools.
**
** In an attempt to avoid globals and to shut up the preprocessor - this
** file uses many similar names for the same object types. The plan was to
** have a declared strucure type in impcommon.sc and declare local instances
** of the structure in each routine. I never got around to this.
**
** I also started to put some error checking in here - but this needs
** reworking - most stuff here breaks if a remote vnode does not respond
** because of bug 62631.
**
** The general format of things in this file is:
**
**	processBlah()	- called from the Menu
**		getBlah() - get the values from the IMA table
**		displayBlah() - do the putform stuff
**
**
** As we pull a value from the form to examine, there is a good chance that
** when we do the select to look up the details - the situation will have
** changed - this is a difference from IPM - which takes a snapshot of the 
** world and then examines the snapshot
**
** Looking up the resource names is a bit of a nightmare - the way in which we
** get connected to the correct databases needs to be rethought - the way it 
** is currently done 'just happened'.
**		
## History:
##
##	10-feb-94	(johna)
##			created from routines in individual files
##			impdsplkd.sc
##			impdsplklst.sc
##			implkmain.sc
##
##	18-mar-94	(johna)
##			reworked queries to use the keys!
##
## 	04-may-94	(johna)
##			some comments.
*/

/* 
##      10-apr-02       (mdf100402)
##			Altered the registration of the imp_lkmo_llb table .
##			Removed fields from this routine accordingly.
##
##			Altered the registration of the imp_lkmo_rsb table .
##			Removed fields from this routine accordingly.
##
##			Altered the registration of the imp_lkmo_lkb table .
##			Removed fields from this routine accordingly.
##
##			When examining lock list, always get the database
##                      name because there is no guarantee that the last
##                      locklist which was 'Examine'd was for
##                      the same dbname as the current LockList.
##
##      17-apr-02       (mdf170402)
##			Wrapped a varchar() function around column name.
##			Otherwise, 2.5/6 gives fatal exception error.
##
##      24-apr-02       (mdf240402)
##			Removed a condition from where clause.
##			Otherwise, 2.6 gives no locklist information
##
##      02-Jul-2002 (fanra01)
##          Sir 108159
##          Added to distribution in the sig directory.
##      04-Sep-03       (mdf040903)                     
##              Commit after calling imp_complete_vnode 
##              Commit more often especially after selecting from iidatabase !
##              Add Row Number info for Row Level locking
##      06-Apr-04    (srisu02) 
##              Sir 108159
##              Integrating changes made by flomi02  
##	08-Oct-2007 (hanje04)
##	    SIR 114907
##	    Remove declaration of timebuf as it's not referenced and causes
##	    errors on Max OSX
**	1-Dec-2010 (kschendel)
**	    Fix warnings (string.h)
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "imp.h"
#include <string.h>

exec sql include sqlca;
exec sql include 'impcommon.sc';

exec sql begin declare section;

extern int imp_lk_menu;
extern int inFormsMode;
extern int imaSession;
extern int iidbdbSession;
extern int userSession;
extern char	*currentVnode;

char	reason[100];

MenuEntry LkMenuList[] = {
	1,	"summary","Display a Locking System Summary",
	2,	"locks", "Display locklists and related information",
	3,	"resources","Display resource lists and related information",
	0,	"no_entry","No Entry"
};

exec sql end declare section;

SessEntry *newSessEntry();
void resetLockSummary();
void getLockSummary();
void displayLockSummary();
void displayLockLists();
void clearLockLists();
void examineLockList();
void getTableName();

/*
** Name:	implkmain.sc
**
** Locking System Menu for the IMA based IMP-like tool
**
## History:
##
## 16-feb-94	(johna)
##		Written.
## 10-feb-94	(johna)
##		moved into the implocks file
*/
int
lockingMenu()
{
	exec sql begin declare section;
	int	i;
	char	buf[60];
	exec sql end declare section;

	exec frs display imp_lk_menu read;
	exec frs initialize;
	exec frs begin;
		makeIidbdbConnection();
		exec frs putform imp_lk_menu (vnode = :currentVnode);

		/* 
##
##mdf100402. 
                ** We need to look at all sessions in this installation. 
                ** Therefore, reset and then look at the 
                ** complete vnode details .
                */
		exec sql execute procedure imp_reset_domain;
		exec sql commit;  
/* 
##
##mdf240402 */
                exec sql execute procedure                         
                        imp_set_domain(entry=:currentVnode); 
/*
##
##mdf040903
*/
		exec sql commit;     
/* 
##
##mdf040903 
*/
		exec sql execute procedure imp_complete_vnode; 
		exec sql commit;     
/*
##
##mdf240402 
*/

		exec frs inittable imp_lk_menu imp_menu read;
		i = 0;
		while (LkMenuList[i].item_number != 0) {
			exec frs loadtable imp_lk_menu imp_menu (
				item_number = :LkMenuList[i].item_number,
				short_name = :LkMenuList[i].short_name,
				long_name = :LkMenuList[i].long_name );
			i++;
		}
	exec frs end;

	exec frs activate menuitem 'Select', frskey4;
	exec frs begin;
		exec frs getrow imp_lk_menu imp_menu (:i = item_number);
		switch (i) {
		case 1:	processLockSummary();
			break;
		case 2: processLockLists();
			break;
		case 3: displayResourceList();
			break;
		default:
			sprintf(buf,"Unknown option %d",i);
			exec frs message :buf with style = popup;
		}
	exec frs end;

	exec frs activate menuitem 'End', frskey3;
	exec frs begin;
		exec frs breakdisplay;
	exec frs end;

	if (strcmp(buf,currentVnode) == 0) {
		exec sql execute procedure imp_complete_vnode;
		if (sqlerr_check() == NO_ROWS) {
			fatal("lockingMenu: cannot execute imp_complete_vnode");
		}
		exec sql commit;     
/* 
##
##mdf040903 
*/
	}
	return(TRUE);
}

/*
** Name:	impdsplkd.sc
**
** Display the locking system summary
**
## History:
##
## 	16-feb-94	(johna)
##			written
##	10-mar-94	(johna)
##			Moved into the implocks.sc file
##
*/

exec sql begin declare section;

extern int imp_lock_summary;
extern int inFormsMode;
extern int timeVal;

struct lkd_ {
	char	vnode[65];
	long	lkd_rsh_size;
	long	lkd_lkh_size;
	long	lkd_max_lkb;
	long	lkd_stat_create_list;
	long	lkd_stat_release_all;
	long	lkd_llb_inuse;
	long	lock_lists_remaining;
	long	lock_lists_available;
	long	lkd_lkb_inuse;
	long	locks_remaining;
	long	locks_available;
	long	lkd_rsb_inuse;
	long	lkd_stat_request_new;
	long	lkd_stat_request_cvt;
	long	lkd_stat_convert;
	long	lkd_stat_release;
	long	lkd_stat_cancel;
	long	lkd_stat_rlse_partial;
	long	lkd_stat_dlock_srch;
	long	lkd_stat_cvt_deadlock;
	long	lkd_stat_deadlock;
	long	lkd_stat_convert_wait;
	long	lkd_stat_wait;
};
struct  lkd_ lkd;
struct  lkd_ new_lkd;

int	newStats = 0;

exec sql end declare section;


int
processLockSummary()
{
	exec sql begin declare section;
	char	vnode_name[SERVER_NAME_LENGTH];
	exec sql end declare section;

	exec frs display imp_lock_summary read;
	exec frs initialize;
	exec frs begin;
		exec frs putform imp_lock_summary (vnode_name = :currentVnode);
		getLockSummary();
		displayLockSummary();
		exec frs set_frs frs (timeout = :timeVal);
	exec frs end;

	exec frs activate menuitem 'Interval';
	exec frs begin;
	exec frs set_frs frs (timeout = 0);
	exec frs display submenu;

		exec frs activate menuitem 'Since_Startup';
		exec frs begin;
			newStats = 0;
			resetLockSummary();
			getLockSummary();
			displayLockSummary();
			exec frs set_frs frs (timeout = :timeVal);
			exec frs enddisplay;
		exec frs end;
			
		exec frs activate menuitem 'Begin_Now';
		exec frs begin;
			newStats = 1;
			resetLockSummary();
			getLockSummary();
			copyLockSummary();
			getLockSummary();
			displayLockSummary();
			exec frs set_frs frs (timeout = :timeVal);
			exec frs enddisplay;
		exec frs end;
		
	exec frs end;

	exec frs activate timeout;
	exec frs begin;
		getLockSummary();
		displayLockSummary();
	exec frs end;

	exec frs activate menuitem 'Refresh';
	exec frs begin;
		getLockSummary();
		displayLockSummary();
	exec frs end;

	exec frs activate menuitem 'End', frskey3;
	exec frs begin;
	exec frs set_frs frs (timeout = 0);
	exec frs breakdisplay;
	exec frs end;

	return(TRUE);
}

void resetLockSummary()
{
	lkd.lkd_stat_create_list = 0;
	lkd.lkd_stat_release_all = 0;
	lkd.lkd_rsb_inuse = 0;
	lkd.lkd_stat_request_new = 0;
	lkd.lkd_stat_request_cvt = 0;
	lkd.lkd_stat_convert = 0;
	lkd.lkd_stat_release = 0;
	lkd.lkd_stat_cancel = 0;
	lkd.lkd_stat_rlse_partial = 0;
	lkd.lkd_stat_dlock_srch = 0;
	lkd.lkd_stat_cvt_deadlock = 0;
	lkd.lkd_stat_deadlock = 0;
	lkd.lkd_stat_convert_wait = 0;
	lkd.lkd_stat_wait = 0;
}

copyLockSummary()
{
	lkd.lkd_stat_create_list = new_lkd.lkd_stat_create_list;
	lkd.lkd_stat_release_all = new_lkd.lkd_stat_release_all;
	lkd.lkd_stat_request_new = new_lkd.lkd_stat_request_new;
	lkd.lkd_stat_request_cvt = new_lkd.lkd_stat_request_cvt;
	lkd.lkd_stat_convert = new_lkd.lkd_stat_convert;
	lkd.lkd_stat_release = new_lkd.lkd_stat_release;
	lkd.lkd_stat_cancel = new_lkd.lkd_stat_cancel;
	lkd.lkd_stat_rlse_partial = new_lkd.lkd_stat_rlse_partial;
	lkd.lkd_stat_dlock_srch = new_lkd.lkd_stat_dlock_srch;
	lkd.lkd_stat_cvt_deadlock = new_lkd.lkd_stat_cvt_deadlock;
	lkd.lkd_stat_deadlock = new_lkd.lkd_stat_deadlock;
	lkd.lkd_stat_convert_wait = new_lkd.lkd_stat_convert_wait;
	lkd.lkd_stat_wait = new_lkd.lkd_stat_wait;
}

void getLockSummary()
{
	exec sql repeated select 
		vnode,
		lkd_rsh_size,
		lkd_lkh_size,
		lkd_max_lkb,
		lkd_stat_create_list,
		lkd_stat_release_all,
		lkd_llb_inuse,
		lock_lists_remaining,
		lock_lists_available,
		lkd_lkb_inuse,
		locks_remaining,
		locks_available,
		lkd_rsb_inuse,
		lkd_stat_request_new,
		lkd_stat_request_cvt,
		lkd_stat_convert,
		lkd_stat_release,
		lkd_stat_cancel,
		lkd_stat_rlse_partial,
		lkd_stat_dlock_srch,
		lkd_stat_cvt_deadlock,
		lkd_stat_deadlock,
		lkd_stat_convert_wait,
		lkd_stat_wait
	into
		:new_lkd.vnode,
		:new_lkd.lkd_rsh_size,
		:new_lkd.lkd_lkh_size,
		:new_lkd.lkd_max_lkb,
		:new_lkd.lkd_stat_create_list,
		:new_lkd.lkd_stat_release_all,
		:new_lkd.lkd_llb_inuse,
		:new_lkd.lock_lists_remaining,
		:new_lkd.lock_lists_available,
		:new_lkd.lkd_lkb_inuse,
		:new_lkd.locks_remaining,
		:new_lkd.locks_available,
		:new_lkd.lkd_rsb_inuse,
		:new_lkd.lkd_stat_request_new,
		:new_lkd.lkd_stat_request_cvt,
		:new_lkd.lkd_stat_convert,
		:new_lkd.lkd_stat_release,
		:new_lkd.lkd_stat_cancel,
		:new_lkd.lkd_stat_rlse_partial,
		:new_lkd.lkd_stat_dlock_srch,
		:new_lkd.lkd_stat_cvt_deadlock,
		:new_lkd.lkd_stat_deadlock,
		:new_lkd.lkd_stat_convert_wait,
		:new_lkd.lkd_stat_wait
	from imp_lk_summary_view
	where
		vnode = :currentVnode;
	if (sqlerr_check() == NO_ROWS) {
		fatal("getLockSummary: no rows from lk_summary_view");
	}

	new_lkd.lkd_stat_create_list -= lkd.lkd_stat_create_list;
	new_lkd.lkd_stat_release_all -= lkd.lkd_stat_release_all;
	new_lkd.lkd_stat_request_new -= lkd.lkd_stat_request_new;
	new_lkd.lkd_stat_request_cvt -= lkd.lkd_stat_request_cvt;
	new_lkd.lkd_stat_convert -= lkd.lkd_stat_convert;
	new_lkd.lkd_stat_release -= lkd.lkd_stat_release;
	new_lkd.lkd_stat_cancel -= lkd.lkd_stat_cancel;
	new_lkd.lkd_stat_rlse_partial -= lkd.lkd_stat_rlse_partial;
	new_lkd.lkd_stat_dlock_srch -= lkd.lkd_stat_dlock_srch;
	new_lkd.lkd_stat_cvt_deadlock -= lkd.lkd_stat_cvt_deadlock;
	new_lkd.lkd_stat_deadlock -= lkd.lkd_stat_deadlock;
	new_lkd.lkd_stat_convert_wait -= lkd.lkd_stat_convert_wait;
	new_lkd.lkd_stat_wait -= lkd.lkd_stat_wait;
	exec sql commit;   
/* 
##
##mdf040903 
*/
}

void displayLockSummary()
{
	exec frs putform imp_lock_summary 
		(vnode_name = :new_lkd.vnode);
	exec frs putform imp_lock_summary 
		(res_tbl_size = :new_lkd.lkd_rsh_size);
	exec frs putform imp_lock_summary 
		(lock_tbl_size = :new_lkd.lkd_lkh_size);
	exec frs putform imp_lock_summary 
		(locks_per_tx = :new_lkd.lkd_max_lkb);
	exec frs putform imp_lock_summary 
		(ll_created = :new_lkd.lkd_stat_create_list);
	exec frs putform imp_lock_summary 
		(ll_released = :new_lkd.lkd_stat_release_all);
	exec frs putform imp_lock_summary 
		(ll_inuse = :new_lkd.lkd_llb_inuse);
	exec frs putform imp_lock_summary 
		(ll_remaining = :new_lkd.lock_lists_remaining);
	exec frs putform imp_lock_summary 
		(ll_total = :new_lkd.lock_lists_available);
	exec frs putform imp_lock_summary 
		(locks_in_use = :new_lkd.lkd_lkb_inuse);
	exec frs putform imp_lock_summary 
		(locks_remaining = :new_lkd.locks_remaining);
	exec frs putform imp_lock_summary 
		(locks_total = :new_lkd.locks_available);
	exec frs putform imp_lock_summary 
		(res_in_use = :new_lkd.lkd_rsb_inuse);
	exec frs putform imp_lock_summary 
		(locks_requested = :new_lkd.lkd_stat_request_new);
	exec frs putform imp_lock_summary 
		(locks_rerequested = :new_lkd.lkd_stat_request_cvt);
	exec frs putform imp_lock_summary 
		(locks_converted = :new_lkd.lkd_stat_convert);
	exec frs putform imp_lock_summary 
		(locks_released = :new_lkd.lkd_stat_release);
	exec frs putform imp_lock_summary 
		(locks_cancelled = :new_lkd.lkd_stat_cancel);
	exec frs putform imp_lock_summary 
		(locks_escalated = :new_lkd.lkd_stat_rlse_partial);
	exec frs putform imp_lock_summary 
		(deadlock_search = :new_lkd.lkd_stat_dlock_srch);
	exec frs putform imp_lock_summary 
		(convert_deadlock = :new_lkd.lkd_stat_cvt_deadlock);
	exec frs putform imp_lock_summary 
		(deadlock = :new_lkd.lkd_stat_deadlock);
	exec frs putform imp_lock_summary 
		(convert_wait = :new_lkd.lkd_stat_convert_wait);
	exec frs putform imp_lock_summary 
		(lock_wait = :new_lkd.lkd_stat_wait);
exec frs redisplay;
}

/*
** Name:	impdsplklst.sc
**
** Display the list of lock lists for the new IMA based IPM lookalike tool.
**
## History:
##
## 24-feb-94	(johna)
##	Written.
##
##	10-mar-94	(johna)
##			Moved into the implocks.sc file
*/

exec sql begin declare section;

extern int imp_lk_list;
extern int inFormsMode;
extern int timeVal;

exec sql end declare section;

int
processLockLists()
{
	exec sql begin declare section;
		SessEntry	*current;
		int		pid;
		int		scb_pid;   
/*
##
##mdf240402
*/
		char		thread[20];
		char		session_id[20];
		char	vnode_name[30];
		char	session_name[30];
char	e_server[SERVER_NAME_LENGTH];    
/*
##
##mdf100402
*/
SessEntry        *SessListHead;  
SessEntry       *current_sess;
		int	lkb_id_id;
		int	llb_id_id;
	        char	buf[60];
	exec sql end declare section;

	exec frs display imp_lk_list read;
	exec frs initialize;
	exec frs begin;
		exec frs inittable imp_lk_list locktbl read;
		exec frs putform imp_lk_list (vnode = :currentVnode);
		displayLockLists();
	exec frs end;

	exec frs activate menuitem 'Refresh';
	exec frs begin;
		clearLockLists();
		displayLockLists();
	exec frs end;

	exec frs activate menuitem 'More_info';
	exec frs begin;
		exec frs getrow imp_lk_list locktbl 
				(:pid = pid,
				:thread = sid,
				:session_name = username);
		displayLockDetail(pid,thread,session_name);
	exec frs end;

	exec frs activate menuitem 'Examine';
	exec frs begin;
		exec frs getrow imp_lk_list locktbl
				(:pid = pid,
				:thread = sid,
				:llb_id_id = id_id);
		examineLockList(pid,thread,llb_id_id);
	exec frs end;

	exec frs activate menuitem 'Block_Info';
	exec frs begin;
		exec frs getrow imp_lk_list locktbl
				(:pid = pid,
				:thread = sid,
				:llb_id_id = id_id);
		findBlockingLocks(pid,thread,llb_id_id);
	exec frs end;

        exec frs activate menuitem 'Session_Info';                      
        exec frs begin;                                                 
                exec frs getrow imp_lk_list locktbl                     
                                (                                       
                                :thread = sid,                          
                                :scb_pid = scb_pid, 
 /*
##
##mdf240402
*/
                                :e_server = server                      
                                );                                      
                /* current_sess = SessListHead; */
                if (current_sess = newSessEntry()) {                    
                    strcpy(current_sess->sessionId,thread );            
                    current_sess->scb_pid = scb_pid;     
/*
##
##mdf240402
*/
                    }                                                   
                                                                        
                if (current_sess) {                                     
                        displaySessionDetail(current_sess,e_server);    
                }                                                       
        exec frs end;   

        
	exec frs activate menuitem 'End', frskey3;
	exec frs begin;
	exec frs breakdisplay;
	exec frs end;

	return(TRUE);
}


void displayLockLists()
{
	exec sql begin declare section;
	char	computed_id[SHORT_STRING];	
	char	computed_status[50];	
	char	l_vnode[SERVER_NAME_LENGTH];	
	int	l_llb_pid;
	char	l_database[30];	/*
##
##mdf040903 
*/
	int	l_scb_pid;  
/*
##
##mdf240402 
*/
	char	l_llb_sid[SESSION_ID_LENGTH];
	int	l_llb_id_id;
	/*int	l_llb_id_instance; */
	char	l_username[SESSION_ID_LENGTH];
	int	l_llb_lkb_count;
	int	l_llb_llkb_count;
	int	l_llb_max_lkb;
	char	l_llb_status[50];
	char	l_state[50];
	/*char	l_wait_reason[50]; */             
/*
##
##mdf040903
*/
char	l_server[65];    
/*
##
##mdf100402
*/
	exec sql end declare section;


	sprintf(computed_id,"%s%%",currentVnode);
	exec sql repeated select distinct 
		llb.vnode,	/* node on which the locking system resides */
		sess.server,	
/* 
##
##mdf100402 
dbms server */
		llb_pid,	/* pid of the owning server */
		sess.scb_pid,	
/* 
##
##mdf240402 
*/
		sess.database,	
/* 
##
##mdf040903 
*/
		llb_sid,	/* sid of the owning session */
		llb_id_id,	/* id_id of the lock list */
		hex(llb_id_id),	/* hex id_id of the lock list */
		/*llb_id_instance,** id_instance of the lock list */
		username,	/* username of the owning session */
		llb_lkb_count,	/* count of locks on the list */
		llb_llkb_count,	/* count of logical locks on the list */
		llb_max_lkb,	/* max locks for this list */
		llb_status,	/* status of the list */
		state		/* state of the owning session */
		/*,wait_reason*/ /* wait reason (if any) of the session */
	into 
		:l_vnode,
		:l_server,    
/* 
##
##mdf100402 
*/
		:l_llb_pid,
		:l_scb_pid,   
/* 
##
##mdf240402 
*/
		:l_database,   
/* 
##
##mdf040903 
*/
		:l_llb_sid,
		:l_llb_id_id, 
                :computed_id,
		/*:l_llb_id_instance, */
		:l_username,
		:l_llb_lkb_count,
		:l_llb_llkb_count,
		:l_llb_max_lkb,
		:l_llb_status,
		:l_state
		/* , :l_wait_reason */
	from 	
		imp_lkmo_llb llb,
		imp_session_view sess
	where
		vnode = :currentVnode 
	and 	server like :computed_id
	/* and	llb_pid = scb_pid  
##
##mdf240402 
*/
	and	varchar(llb_sid) = session_id    
/*
##
##mdf170402
*/
	and	llb_related_llb <> 0
	and	sess.database != 'imadb'             /*
##
##mdf040903
*/
        order by llb_id_id;
	exec sql begin;
		/*sprintf(computed_id,"%lx",(65536 * l_llb_id_id)  */
                              /*  + l_llb_id_instance */
                               /*); */
		if (l_llb_status[0] == ' ') {
			strcpy(computed_status,"<None>");
		} else {
			strcpy(computed_status,l_llb_status);
		}
		exec frs loadtable imp_lk_list locktbl (
			pid = :l_llb_pid,
			scb_pid = :l_scb_pid,  
/* 
##
##mdf240402 
*/
			server = :l_server,    
/* 
##
##mdf100402 
*/
			database = :l_database,    
/* 
##
##mdf040903 
*/
			sid = :l_llb_sid,
			computed_id = :computed_id,
			id_id = :l_llb_id_id,
			/*id_instance = :l_llb_id_instance, */
			username = :l_username,
			lkb_count = :l_llb_lkb_count,
			llkb_count = :l_llb_llkb_count,
			max_lkb = :l_llb_max_lkb,
			llb_status = :computed_status,
			state = :l_state
			/*,wait_reason = :l_wait_reason */
                         );
	exec sql end;
	(void) sqlerr_check();
	exec sql commit;    
/* 
##
##mdf040903 
*/
	exec frs redisplay;
}

void clearLockLists()
{
	exec frs inittable imp_lk_list locktbl;
}

exec sql begin declare section;
extern int inp_lk_tx;
exec sql end declare section;

int
displayLockDetail(pid,session_id,username)
exec sql begin declare section;
int	pid;
char	*session_id;
char	*username;
exec sql end declare section;
{
	exec sql begin declare section;
	int i;
	char tx_id[40];	
	char	computed_id[SHORT_STRING];
	int	count;
	int	id_id;
	int	instance=0;
	int	related_llb;
	exec sql end declare section;
	exec frs display imp_lk_tx read;
	exec frs initialize;
	exec frs begin;

		exec frs message 'Working . . .';
		exec sql repeated select 
			lxb_tran_id 
		into 
			:tx_id
		from
			imp_lgmo_lxb
		where 
			vnode = :currentVnode
		and	lxb_pid = :pid
		and	lxb_sid = :session_id
		and	lxb_user_name = :username;

		exec frs putform imp_lk_tx (tx_id = :tx_id);
		exec sql commit;            
/* 
##
##mdf040903 
*/

		exec sql repeated select 
			llb_lkb_count,
			llb_id_id,
			/*llb_id_instance, */
			llb_related_llb
		into
			:count,
			:id_id,
			/*:instance, */
			:related_llb
		from
			imp_lkmo_llb
		where 
			vnode = :currentVnode and
			llb_id_id in (
				select 
					llb_related_llb_id_id	
				from
					imp_lkmo_llb
				where
					vnode = :currentVnode
				and 	llb_pid = :pid
				and	llb_sid = :session_id
			);

		if (sqlerr_check() == NO_ROWS) {
		        exec sql commit;            /* 
##mdf040903 
*/
			exec frs message 'No Information Found...';
			PCsleep(2*1000);
		} else {
		        exec sql commit;            /* 
##mdf040903 
*/
			sprintf(computed_id,"%lx",(65536 * id_id) + instance);
			exec frs putform imp_lk_tx 
				(related_lock_list_id = :computed_id);
			exec frs putform imp_lk_tx (related_count = :count);
			
			computed_id[0] = '\0';
			id_id = 0;
			instance = 0;

			/*
			** Get The Wait Resource ID
			*/
			exec sql repeated select 
				rsb_id_id
                               /*, rsb_id_instance*/
			into
				:id_id
                               /*, :instance */
			from
				imp_lkmo_rsb
			where
				vnode = :currentVnode and
				rsb_id_id in
				(	select 
						lkb_rsb_id_id
					from
						imp_lkmo_lkb lk ,
						imp_lkmo_llb lb
					where
						lk.lkb_id_id = lb.llb_wait_id_id
					and	lb.llb_pid = :pid
					and	lb.llb_sid = :session_id
					and	lk.vnode = lb.vnode
					and	lk.vnode = :currentVnode
				);

			if (sqlerr_check() != NO_ROWS) {
		                exec sql commit;            /* 
##mdf040903 
*/
				sprintf(computed_id,"%lx",
					(65536 * id_id) + instance);
				exec frs putform imp_lk_tx 
					(waiting_resource_list_id = 
						:computed_id);
			}
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
** have to make the following global to fool the preprocessor into allowing
** selects inside a select loop (in different sessions)
*/

exec sql begin declare section;
int	gotDbName = FALSE;
int	connectedToDb = FALSE;
int	i;
char	buf[60];
char	userDbName[60];

/*int 	e_llb_id_instance; */
int	e_llb_id_id;
/*int 	e_lkb_id_instance; */
int	e_lkb_id_id;
int	e_rsb_id_id;
/*int	e_rsb_id_instance;*/
int	e_llb_wait_id_id;
char	e_rsb_grant[5];
char	e_rsb_convert[5];
char	e_lkb_request_mode[5];
char	e_lkb_state[30];
int	e_lkb_rsb_id_id;
int	e_lkb_llb_id_id;
char	e_dbname[30];
char	e_tablename[30];
char	e_s_name[30];
int	e_rsb_name0;
int	e_rsb_name1;
int	e_rsb_name2;
int	e_rsb_name3;
int	e_rsb_name4;
int	e_rsb_name5;
int	e_rsb_name6;
exec sql end declare section;

void examineLockList(pid,session_id,llb_id_id)
exec sql begin declare section;
int	pid;
char	*session_id;
int	llb_id_id;
exec sql end declare section;
{
	
	exec sql begin declare section;
	char	computed_id[SHORT_STRING];	
	char	computed_type[SHORT_STRING];	
        char	buf[60];
	exec sql end declare section;

	exec frs display imp_lk_locks read with style = popup;
	exec frs initialize;
	exec frs begin;
		ShowLockList(pid,llb_id_id);
	exec frs end;

	exec frs activate menuitem 'Resource_Info';
	exec frs begin;
		exec frs getrow imp_lk_locks lock_details (
			:computed_id = computed_id,
			:e_lkb_rsb_id_id = lkb_rsb_id_id);
		displayResourceDetail(e_lkb_rsb_id_id);
	exec frs end;

	exec frs activate menuitem 'Refresh';
	exec frs begin;
	    ShowLockList(pid,llb_id_id);
	exec frs end;

	exec frs activate menuitem 'End', frskey3;
	exec frs begin;
	    exec frs breakdisplay;
	exec frs end;

	if (connectedToDb) {
		exec sql set_sql (session = :userSession);
		exec sql disconnect;
		connectedToDb = FALSE;
	}
	exec sql set_sql (session = :imaSession);
}

ShowLockList(pid,llb_id_id)
exec sql begin declare section;
    int	pid;
    int	llb_id_id;
exec sql end declare section;
{
    exec sql begin declare section;
        char    computed_id[SHORT_STRING];    
	char	computed_type[SHORT_STRING];	
    exec sql end declare section;

    exec frs inittable imp_lk_locks lock_details read;

    exec sql repeated select
            /*lkb_id_instance,    */
            lkb_id_id,
            lkb_request_mode,
            lkb_state,
            lkb_rsb_id_id,
            lkb_llb_id_id,
            rsb_name0,
            rsb_name1,
            rsb_name2,
            rsb_name3,
            rsb_name4,
            rsb_name5          /* 
##mdf040903
*/
        into
            /*:e_lkb_id_instance, */
            :e_lkb_id_id,
            :e_lkb_request_mode,
            :e_lkb_state,
            :e_lkb_rsb_id_id,
            :e_lkb_llb_id_id,
            :e_rsb_name0,
            :e_rsb_name1,
            :e_rsb_name2,
            :e_rsb_name3,
            :e_rsb_name4,
            :e_rsb_name5         /* 
##mdf040903
*/
        from
            imp_lkmo_lkb lk,
            imp_lkmo_rsb rb
        where
            lk.vnode = rb.vnode
        and    lk.vnode = :currentVnode
        and    rsb_id_id = lkb_rsb_id_id
        and    lkb_llb_id_id = :llb_id_id
        order by
            rsb_name1,rsb_name0,rsb_name2,rsb_name3
                        ,rsb_name4,rsb_name5;
        exec sql begin;
            getDbName();
            getTableName();

            sprintf(computed_id,"%lx",(65536 * e_lkb_id_id) 
                    /*+ e_lkb_id_instance */
                                        );
            decode_rsb0(e_rsb_name0,computed_type);
            if (e_rsb_name0 != LK_TABLE) {
                if (e_rsb_name0 != LK_ROW) { 
                exec frs loadtable imp_lk_locks lock_details (
                    computed_id = :computed_id,
                    lkb_rq_state = :e_lkb_request_mode,
                    lkb_gr_state = :e_lkb_state,
                    lkb_type = :computed_type,
                    lkb_db = :e_dbname,
                    lkb_table = :e_tablename,
                    lkb_rsb_id_id = :e_lkb_rsb_id_id,
                    lkb_page = :e_rsb_name4);
                } else {
                exec frs loadtable imp_lk_locks lock_details (
                    computed_id = :computed_id,
                    lkb_rq_state = :e_lkb_request_mode,
                    lkb_gr_state = :e_lkb_state,
                    lkb_type = :computed_type,
                    lkb_db = :e_dbname,
                    lkb_table = :e_tablename,
                    lkb_rsb_id_id = :e_lkb_rsb_id_id,
                    lkb_page = :e_rsb_name4
                    , lkb_row = :e_rsb_name5
                                        );
                } 
            } else {
                exec frs loadtable imp_lk_locks lock_details (
                    computed_id = :computed_id,
                    lkb_rq_state = :e_lkb_request_mode,
                    lkb_gr_state = :e_lkb_state,
                    lkb_type = :computed_type,
                    lkb_db = :e_dbname,
                    lkb_table = :e_tablename,
                    lkb_rsb_id_id = :e_lkb_rsb_id_id);
            }
            exec sql set_sql (session = :imaSession);
        exec sql end;
        if (sqlerr_check() == NO_ROWS) {
        exec sql commit;     /* 
##mdf040903 
*/
        sprintf(buf,"No Resources Found for Lock Id %d",llb_id_id);
        exec frs message :buf;
        PCsleep(2*1000);
        }
        exec sql commit;     /* 
##mdf040903 
*/
	gotDbName = FALSE;   /* 
##mdf100402 
Always get dbname next time*/
}

int
getDbName()
{

	exec sql begin declare section;
	    char	buf[100];
	exec sql end declare section;


	if (gotDbName == FALSE) {
		
		exec sql set_sql (session = :iidbdbSession);
		sqlerr_check();              /* 
##mdf040903
*/

		exec sql repeated select 
			name 
		into
			:e_dbname
		from 
			iidatabase 
		where
			db_id = :e_rsb_name1;
		if (sqlerr_check() == NO_ROWS) {

		        exec sql commit;     /*
##mdf040903
*/
			gotDbName = FALSE;
			connectedToDb = FALSE;
			strcpy(reason,"Unavailable");
			exec sql set_sql (session = :imaSession);
			return(FALSE);

		} else {
			gotDbName = TRUE; 
			/* gotDbName = FALSE;   
##mdf100402
*/
		}
			
		exec sql commit;
	}

	sprintf(userDbName,"%s::%s",currentVnode,e_dbname);
	if (connectedToDb == FALSE) {
		exec sql connect :userDbName session :userSession;
		if (sqlca.sqlcode != 0) {
			connectedToDb = FALSE;
			strcpy(reason,"Unavailable");
		} else {
			connectedToDb = TRUE; 
		}
	}
	exec sql set_sql (session = :imaSession);
	return(TRUE);
}

void getTableName()
{
		if (connectedToDb) {
			exec sql set_sql (session = :userSession);
			exec sql repeated select 
				relid
			into	
				:e_tablename
			from 
				iirelation
			where
				reltid = :e_rsb_name2 
			and	reltidx = 0;
			sqlerr_check();
			exec sql commit;
		} else {
			strcpy(e_tablename,reason);
		}
		exec sql set_sql (session = :imaSession);
}

displayResourceDetail(id_id)
exec sql begin declare section;
int	id_id;
exec sql end declare section;
{

	/*
	** we should still be connected to the user db at this point
	*/
	exec sql begin declare section;
	char	computed_type[SHORT_STRING];	
	char	computed_id[SHORT_STRING];	
        char	e_server[65];    /*
##mdf100402
*/
        int	e_scb_pid;    /*
##mdf100402
*/
        char	e_session_id[SESSION_ID_LENGTH];    /*
##mdf100402
*/
        SessEntry        *SessListHead;  
        SessEntry       *current_sess;
	exec sql end declare section;

	exec frs display imp_res_locks read with style = popup;
	exec frs initialize;
	exec frs begin;
		ShowResourceLocks(id_id);
	exec frs end;

       
        exec frs activate menuitem 'Session_Info', frskey4;                  
        exec frs begin;                                                   
                exec frs getrow imp_res_locks res_lock_tbl                
                                (
                                :e_session_id = session_id,
                                :e_scb_pid = scb_pid,
                                :e_server = server
                                );                   
                /* current_sess = SessListHead; */
                if (current_sess = newSessEntry()) {
                    strcpy(current_sess->sessionId,e_session_id );
                    current_sess->scb_pid = e_scb_pid;
                    }

                if (current_sess) {                                       
                        displaySessionDetail(current_sess,e_server);   
                }                                                         
        exec frs end;                                                     
        
                                                                          
	exec frs activate menuitem 'Refresh';
	exec frs begin;
	    ShowResourceLocks(id_id);
	exec frs end;
                                                                          
	exec frs activate menuitem 'End', frskey3;
	exec frs begin;
	    exec frs breakdisplay;
	exec frs end;

}


ShowResourceLocks(id_id)
exec sql begin declare section;
int	id_id;
exec sql end declare section;
{
    exec sql begin declare section;
        char    computed_id[SHORT_STRING];    
        char    computed_id1[SHORT_STRING];    
        char    e_server[65];  
        char    e_session_id[SESSION_ID_LENGTH];
        int    e_scb_pid;    
    exec sql end declare section;
    exec frs clear field all;
    exec frs inittable imp_res_locks res_lock_tbl read;
    exec sql repeated select 
            rsb_id_id,
            /*rsb_id_instance,*/
            rsb_grant_mode,
            rsb_convert_mode,
            rsb_name0,
            rsb_name1,
            rsb_name2,
            rsb_name3,
            rsb_name4,
            rsb_name5,
            rsb_name6
    into
            :e_rsb_id_id,
            /*:e_rsb_id_instance,*/
            :e_rsb_grant,
            :e_rsb_convert,
            :e_rsb_name0,
            :e_rsb_name1,
            :e_rsb_name2,
            :e_rsb_name3,
            :e_rsb_name4,
            :e_rsb_name5,
            :e_rsb_name6
    from
            imp_lkmo_rsb
    where
            rsb_id_id = :id_id
    and    vnode = :currentVnode;

    if (sqlerr_check() == NO_ROWS) {
        exec sql commit;               /* 
##mdf040903 
*/
        exec frs message 'No information available..';
        PCsleep(2*1000);
        } 
    else 
    {
        exec sql commit;               /* 
##mdf040903 
*/
        sprintf(computed_id,"%lx",(65536 * e_rsb_id_id) 
                                   /* + e_rsb_id_instance */
                                         );
        exec frs putform imp_res_locks (resource_id = :computed_id);
        exec frs putform imp_res_locks (convert = :e_rsb_convert);
        decode_rsb0(e_rsb_name0,computed_id);
        exec frs putform imp_res_locks (resource_type = :computed_id);
        exec frs putform imp_res_locks (granted = :e_rsb_grant);

        /* 
        ** get the database, table and  page information
        */
        getDbName();
        exec frs putform imp_res_locks (dbname = :e_dbname);

        if (    (e_rsb_name0 != LK_DATABASE) &&
            (e_rsb_name0 != LK_SV_DATABASE) &&
            (e_rsb_name0 != LK_OPEN_DB) &&
            (e_rsb_name0 != LK_BM_DATABASE) ) {

            getTableName();
            exec frs putform imp_res_locks 
                (table_name = :e_tablename);
        }

        exec frs set_frs field imp_res_locks (invisible(page_number)=1);
        if (       (e_rsb_name0 == LK_PAGE) 
            || (e_rsb_name0 == LK_BM_PAGE)
            || (e_rsb_name0 == LK_ROW)
                ) {
            exec frs putform imp_res_locks (page_number = :e_rsb_name4);
            exec frs set_frs field imp_res_locks (invisible(page_number)=0);
        }

        /* 
##mdf040903 Show row number information 
*/
        exec frs set_frs field imp_res_locks (invisible(row_number)=1);
        if    (e_rsb_name0 == LK_ROW) {
            exec frs putform imp_res_locks (row_number = :e_rsb_name5);
            exec frs set_frs field imp_res_locks (invisible(row_number)=0);
        }

        /* 
        ** traverse the list of locks including this resource
        */
        sprintf(computed_id,"%s%%",currentVnode);
        exec sql repeated select 
            server,     /* 
##mdf100402 
*/
            session_id,     /* 
##mdf100402 
*/
            scb_pid,   /*   
##mdf100402 
*/
            lkb_id_id,
            /*lkb_id_instance, */
            llb_id_id,
            /*llb_id_instance, */
            lkb_request_mode,
            lkb_state,
            s_name
        into
            :e_server,     /* 
##mdf100402 
*/
            :e_session_id,     /* 
##mdf100402 
*/
            :e_scb_pid,     /* 
##mdf100402 
*/
            :e_lkb_id_id,
            /*:e_lkb_id_instance, */
            :e_llb_id_id,
            /*:e_llb_id_instance,*/
            :e_lkb_request_mode,
            :e_lkb_state,
            :e_s_name
        from
            imp_lkmo_lkb, 
            imp_lkmo_llb, 
            imp_scs_sessions
        where
            lkb_llb_id_id = llb_id_id
        /*and    llb_pid = scb_pid 
##mdf240402 
*/
        and    varchar(llb_sid) = session_id   /*
##mdf170402
*/
        and     server like :computed_id
        and    lkb_rsb_id_id = :e_rsb_id_id
        and    imp_lkmo_lkb.vnode = imp_lkmo_llb.vnode
        and    imp_lkmo_lkb.vnode = :currentVnode;
        exec sql begin;
            sqlerr_check();
            sprintf(computed_id,"%lx",
                    (65536 *e_lkb_id_id) 
                                              /* + e_lkb_id_instance */
                                              );
            sprintf(computed_id1,"%lx",
                    (65536 *e_llb_id_id) 
                                               /*+ e_llb_id_instance */
                                                );
            exec frs loadtable imp_res_locks res_lock_tbl (
                resource_id = :computed_id,
                list_id = :computed_id1,
                session_name = :e_s_name,
                server = :e_server,  /*
##mdf100402
*/
                scb_pid = :e_scb_pid,  /*
##mdf100402
*/
                session_id = :e_session_id,  /*
##mdf100402
*/
                request_mode = :e_lkb_request_mode,
                grant_state = :e_lkb_state);
        exec sql end;
    }
}

/*
** findBlockingLocks()
** look for the lock that is blocking this list.
*/
findBlockingLocks(pid,session_id,llb_id_id)
exec sql begin declare section;
int	pid;
char	*session_id;
int	llb_id_id;
exec sql end declare section;
{

	exec sql begin declare section;
	char	buf[100];
	exec sql end declare section;

	exec frs message 'Working...';

	exec sql repeated select 
		llb_wait_id_id
	into	
		:e_llb_wait_id_id
	from 
		imp_lkmo_llb
	where
		llb_id_id = :llb_id_id
	and	vnode = :currentVnode;

	sqlerr_check();

	if (e_llb_wait_id_id == 0) {
		exec frs message 'This locklist has no currently blocked locks'
			with style = popup;
		return;
	}
	/* get the resource ID */
	exec sql repeated select 
		lkb_rsb_id_id
	into	
		:e_lkb_rsb_id_id
	from	
		imp_lkmo_lkb
	where
		lkb_id_id = :e_llb_wait_id_id
	and 	vnode = :currentVnode;

	sqlerr_check();

	displayResourceDetail(e_lkb_rsb_id_id);
}
	

/*
** display resource list
*/

displayResourceList()
{

	exec sql begin declare section;
	char	computed_type[SHORT_STRING];	
	char	computed_id[SHORT_STRING];	
	char	computed_id1[SHORT_STRING];	
	int	old_db_id = 0;
	exec sql end declare section;

	exec frs display imp_res_list read;
	exec frs initialize;
	exec frs begin;
		exec frs inittable imp_res_list lock_details read;
		refreshResourceList();
	exec frs end;

        
        exec frs activate menuitem 'Resource_Info';                  
        exec frs begin;                                                   
		exec frs getrow imp_res_list lock_details (
			:e_lkb_rsb_id_id = lkb_rsb_id_id);
		displayResourceDetail(e_lkb_rsb_id_id);
	exec frs end;
       

	exec frs activate menuitem 'Refresh';
	exec frs begin;
		refreshResourceList();
	exec frs end;

	exec frs activate menuitem 'End', frskey3;
	exec frs begin;
	    exec frs breakdisplay;
	exec frs end;

	if (connectedToDb) {
		exec sql set_sql (session = :userSession);
		exec sql disconnect;
		connectedToDb = FALSE;
	}
	exec sql set_sql (session = :imaSession);
}

decode_rsb0(id,str)
int	id;
char	*str;
{
	switch(id) {
	case LK_DATABASE:	strcpy(str, LK_DATABASE_NAME ); break;
	case LK_TABLE:		strcpy(str, LK_TABLE_NAME ); break;
	case LK_PAGE:		strcpy(str, LK_PAGE_NAME ); break;
	case LK_EXTEND_FILE:	strcpy(str, LK_EXTEND_FILE_NAME ); break;
	case LK_BM_PAGE:	strcpy(str, LK_BM_PAGE_NAME ); break;
	case LK_CREATE_TABLE:	strcpy(str, LK_CREATE_TABLE_NAME ); break;
	case LK_OWNER_ID:	strcpy(str, LK_OWNER_ID_NAME ); break;
	case LK_CONFIG:		strcpy(str, LK_CONFIG_NAME ); break;
	case LK_DB_TEMP_ID:	strcpy(str, LK_DB_TEMP_ID_NAME ); break;
	case LK_SV_DATABASE:	strcpy(str, LK_SV_DATABASE_NAME ); break;
	case LK_SV_TABLE:	strcpy(str, LK_SV_TABLE_NAME ); break;
	case LK_SS_EVENT:	strcpy(str, LK_SS_EVENT_NAME ); break;
	case LK_TBL_CONTROL:	strcpy(str, LK_TBL_CONTROL_NAME ); break;
	case LK_JOURNAL:	strcpy(str, LK_JOURNAL_NAME ); break;
	case LK_OPEN_DB:	strcpy(str, LK_OPEN_DB_NAME ); break;
	case LK_CKP_DB:		strcpy(str, LK_CKP_DB_NAME ); break;
	case LK_CKP_CLUSTER:	strcpy(str, LK_CKP_CLUSTER_NAME ); break;
	case LK_BM_LOCK:	strcpy(str, LK_BM_LOCK_NAME ); break;
	case LK_BM_DATABASE:	strcpy(str, LK_BM_DATABASE_NAME ); break;
	case LK_BM_TABLE:	strcpy(str, LK_BM_TABLE_NAME ); break;
	case LK_CONTROL:	strcpy(str, LK_CONTROL_NAME ); break;
	case LK_EVCONNECT:	strcpy(str, LK_EVCONNECT_NAME ); break;
	case LK_AUDIT:		strcpy(str, LK_AUDIT_NAME ); break;
	case LK_ROW:		strcpy(str, LK_ROW_NAME ); break;
	default:		strcpy(str, "Unknown");
	}
}

refreshResourceList()
{

	exec sql begin declare section;
	char	computed_type[SHORT_STRING];	
	char	computed_id[SHORT_STRING];	
	char	computed_id1[SHORT_STRING];	
	int	old_db_id = 0;
	int	lk_row = LK_ROW;
	int	lk_database = LK_DATABASE;
	int	lk_table = LK_TABLE;
	int	lk_page = LK_PAGE;
	exec sql end declare section;

	exec frs inittable imp_res_list lock_details read;
	exec sql repeated select 
		rsb_id_id,
		/*rsb_id_instance, */
		rsb_grant_mode,
		rsb_convert_mode,
		rsb_name0,
		rsb_name1,
		rsb_name2,
		rsb_name3,
		rsb_name4,
		rsb_name5,
		rsb_name6
	into
		:e_rsb_id_id,
		/*:e_rsb_id_instance, */
		:e_rsb_grant,
		:e_rsb_convert,
		:e_rsb_name0,
		:e_rsb_name1,
		:e_rsb_name2,
		:e_rsb_name3,
		:e_rsb_name4,
		:e_rsb_name5,
		:e_rsb_name6
	from
		imp_lkmo_rsb

	where
		rsb_name0 in (:lk_database,:lk_table,:lk_page, :lk_row)

	order by 
		rsb_name1,
		rsb_name0 asc,
		rsb_name2,
		rsb_name4,
		rsb_name5;
	exec sql begin;

		sprintf(computed_id,"%lx",(65536 * e_rsb_id_id) 
                                   /* + e_rsb_id_instance */
                                         );

		if (e_rsb_name0 != old_db_id) {
			if (connectedToDb == TRUE) {
				exec sql set_sql (session=:userSession);
				exec sql disconnect;
			}
			connectedToDb = FALSE;
			gotDbName = FALSE;
			exec sql set_sql (session = :imaSession);
			old_db_id = e_rsb_name0;
		}
                getDbName();
		if (    (e_rsb_name0 != LK_DATABASE) &&
			(e_rsb_name0 != LK_SV_DATABASE) &&
			(e_rsb_name0 != LK_OPEN_DB) &&
			(e_rsb_name0 != LK_BM_DATABASE) ) {
			getTableName();
		}

		decode_rsb0(e_rsb_name0,computed_id1);

		if (e_rsb_name0 == LK_ROW)   /* 
##mdf040903
*/
                { 
			exec frs loadtable imp_res_list lock_details (
				computed_id = :computed_id,
		                lkb_rsb_id_id = :e_rsb_id_id, 
/*
##MDF100402
*/
				rsb_cv_state = :e_rsb_convert,
				rsb_gr_state = :e_rsb_grant,
				rsb_type = :computed_id1,
				rsb_db = :e_dbname,
				rsb_table = :e_tablename,
				rsb_page = :e_rsb_name4,
				rsb_row = :e_rsb_name5
			);
		} 
                else {
		if (	(e_rsb_name0 == LK_PAGE) 
			|| (e_rsb_name0 == LK_BM_PAGE)
                    ) {
			exec frs loadtable imp_res_list lock_details (
				computed_id = :computed_id,
		                lkb_rsb_id_id = :e_rsb_id_id, /*
##MDF100402
*/
				rsb_cv_state = :e_rsb_convert,
				rsb_gr_state = :e_rsb_grant,
				rsb_type = :computed_id1,
				rsb_db = :e_dbname,
				rsb_table = :e_tablename,
				rsb_page = :e_rsb_name4
			);
		} else {
			exec frs loadtable imp_res_list lock_details (
				computed_id = :computed_id,
		                lkb_rsb_id_id = :e_rsb_id_id, /*
##MDF100402
*/
				rsb_gr_state = :e_rsb_grant,
				rsb_cv_state = :e_rsb_convert,
				rsb_type = :computed_id1,
				rsb_db = :e_dbname,
				rsb_table = :e_tablename
			);
		} }
		e_tablename[0] = '\0';
		exec sql end;
		sqlerr_check();

}
