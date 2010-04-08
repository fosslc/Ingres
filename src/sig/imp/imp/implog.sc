/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name:	implog.sc
**
** This file contains all of the routines needed to support the "LOG" menu
** tree of the IMA based IMP lookalike tools.
**
** The structures in this file could easily be moved to the impcommon.sc file.
**
** In common with the implocks.sc file - these routines generalyy are of the
** form
**	processBlah() - called from menu
**		getBlah() - get the values from the IMA query
**		displayBlah() - do the putform stuff.
**
** Also in common with the implocks stuff - the situation may have changed by
** the time we take a closer look - this needs to be examined.
**
** The 'hard' parts of the log header screen/ dual log stuff all needs to be
** written.
**
** decodeStatusVal is present as the object in IMA that provides this
** information is wrong (exp.dmf.lg.lpb_status) - see bug 62546 
**
## History:
##
##	14-apr-94	(johna)
##			Started.
##	04-may-94	(johna)
##			some comments.
##      02-Jul-2002 (fanra01)
##          Sir 108159
##          Added to distribution in the sig directory.
##
##      04-Sep-03        Added extra functionality (mdf040903)
##                         Session_Info 
##                         Tx_Info
##                         Lock_Info
##      06-Apr-04    (srisu02) 
##              Sir 108159
##              Integrating changes made by flomi02  
##    08-Oct-2007 (hanje04)
##        SIR 114907
##        Remove declaration of timebuf as it's not referenced and causes
##        errors on Mac OSX
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "imp.h"

exec sql include sqlca;
exec sql include 'impcommon.sc';

SessEntry *newSessEntry();
void resetLogSummary();
void getLogSummary();
void displayLogSummary();
void displayHeaderInfo();
void displayLogProcesses();
void TransactionDetail();
void displayLogDatabases();
void displayLogTransactions();
void getServerName();

exec sql begin declare section;

extern int imp_transaction_detail;   /* 
##mdf040903 
*/ 

struct lgd_stat {
	long	db_adds;
	long	db_removes;
	long	tx_begins;
	float	tx_ends;
	long	log_writes;
	long	log_write_ios;
	long	log_read_ios;
	long	log_waits;
	long	log_split_waits;
	long	log_free_waits;
	long	log_stall_waits;
	long	log_forces;
	long	log_group_commit;
	long	log_group_count;
	long	check_commit_timer;
	long	timer_write;
	float	kbytes_written;
	long	block_size;       /* 
##mdf200302
*/
	float	buffer_utilisation;       /* 
##mdf200302
*/
	double	avg_txn_size;       /* 
##mdf200302
*/
	long	inconsistent_db;
};

struct lg_header {
	char	status[50];
	long	block_count;
	long	block_size;
	long	buf_count;
	long	logfull_interval;
	long	abort_interval;
	long	cp_interval;
	char	begin_addr[40];
	long	begin_block;
	char	end_addr[40];
	long	end_block;
	char	cp_addr[40];
	long	cp_block;
	long	next_cp_block;
	long	reserved_blocks;
	long	inuse_blocks;
	long	avail_blocks;
};


extern 	int 	imp_lg_menu;
extern 	int 	inFormsMode;
extern 	int 	imaSession;
extern 	int 	iidbdbSession;
extern 	int 	userSession;
extern 	char	*currentVnode;

MenuEntry LgMenuList[] = {
	1,	"summary",
		"Display a Logging System Summary",
	2,	"header", 
		"Display the Log File Header",
	3,	"processes",
		"Display active processes in the logging system",
	4,	"databases",
		"Display logging information on each open database",
	5,	"transactions",
		"Display logging information on each transaction",
	0,	"no_entry",
		"No Entry"
};

exec sql end declare section;

/*
** Name:	implgmain.sc
**
** Logging System Menu for the IMA based IMP-like tool
**
## History:
##
##      04-Sep-03       (mdf040903)                     
##              Commit after calling imp_complete_vnode 
##              Commit after calling imp_reset_domain
*/
int
loggingMenu()
{
	exec sql begin declare section;
	int	i;
	char	buf[60];
	exec sql end declare section;

	exec frs display imp_lg_menu read;
	exec frs initialize;
	exec frs begin;
                makeIidbdbConnection(); 
		exec frs putform imp_lg_menu (vnode = :currentVnode);

		exec sql repeated select dbmsinfo('ima_vnode') into :buf;
		sqlerr_check();
		if (strcmp(buf,currentVnode) != 0) {
			exec sql execute procedure imp_reset_domain;
			exec sql execute procedure 
				imp_set_domain(entry=:currentVnode);
		} else {
			exec sql execute procedure imp_reset_domain;
		}
		exec sql commit;    /*
##mdf040903
*/

		exec frs inittable imp_lg_menu imp_menu read;
		i = 0;
		while (LgMenuList[i].item_number != 0) {
			exec frs loadtable imp_lg_menu imp_menu (
				item_number = :LgMenuList[i].item_number,
				short_name = :LgMenuList[i].short_name,
				long_name = :LgMenuList[i].long_name );
			i++;
		}
	exec frs end;

	exec frs activate menuitem 'Select', frskey4;
	exec frs begin;
		exec frs getrow imp_lg_menu imp_menu (:i = item_number);
		switch (i) {
		case 1:	processLogSummary();
			break;
		case 2:	processLogHeader();
			break;
		case 3:	processLogProcesses();
			break;
		case 4:	processLogDatabases();
			break;
		case 5:	processLogTransactions();
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
		exec sql commit;  /*
##mdf040903
*/
	}
	return(TRUE);
}

/*
** Name:	impdsplgd.sc
**
** Display the Loging system summary
**
## History:
##
*/

exec sql begin declare section;

extern int 	imp_lg_summary;
extern int 	timeVal;

struct lgd_stat	lgd;
struct lgd_stat	new_lgd;

int		newLgStats = 0;

exec sql end declare section;

int
processLogSummary()
{
	exec sql begin declare section;
	char	vnode_name[SERVER_NAME_LENGTH];
	exec sql end declare section;

	exec frs display imp_lg_summary read;
	exec frs initialize;
	exec frs begin;
		exec frs putform imp_lg_summary (vnode = :currentVnode);
		getLogSummary();
		displayLogSummary();
		exec frs set_frs frs (timeout = :timeVal);
	exec frs end;

	exec frs activate menuitem 'Refresh';
	exec frs begin;
		getLogSummary();
		displayLogSummary();
	exec frs end;

	exec frs activate menuitem 'Interval';
	exec frs begin;
	exec frs set_frs frs (timeout = 0);
	exec frs display submenu;

		exec frs activate menuitem 'Since_Startup';
		exec frs begin;
			newLgStats = 0;
			resetLogSummary();
			getLogSummary();
			displayLogSummary();
			exec frs set_frs frs (timeout = :timeVal);
			exec frs enddisplay;
		exec frs end;
			
		exec frs activate menuitem 'Begin_Now';
		exec frs begin;
			newLgStats = 1;
			resetLogSummary();
			getLogSummary();
			copyLogSummary();
			getLogSummary();
			displayLogSummary();
			exec frs set_frs frs (timeout = :timeVal);
			exec frs enddisplay;
		exec frs end;
		
	exec frs end;

	exec frs activate timeout;
	exec frs begin;
		getLogSummary();
		displayLogSummary();
	exec frs end;

	exec frs activate menuitem 'End', frskey3;
	exec frs begin;
	exec frs set_frs frs (timeout = 0);
	exec frs breakdisplay;
	exec frs end;

	return(TRUE);
}

void resetLogSummary()
{
	lgd.db_adds = 0;
	lgd.db_removes = 0;
	lgd.tx_begins = 0;
	lgd.tx_ends = 0;
	lgd.log_writes = 0;
	lgd.log_write_ios = 0;
	lgd.log_read_ios = 0;
	lgd.log_waits = 0;
	lgd.log_split_waits = 0;
	lgd.log_free_waits = 0;
	lgd.log_stall_waits = 0;
	lgd.log_forces = 0;
	lgd.log_group_commit = 0;
	lgd.log_group_count = 0;
	lgd.check_commit_timer = 0;
	lgd.timer_write = 0;
	lgd.kbytes_written = 0;
	lgd.buffer_utilisation = 0;
	lgd.avg_txn_size = 0;
	lgd.block_size = 0;
	lgd.inconsistent_db = 0;
}

copyLogSummary()
{
	lgd.db_adds = new_lgd.db_adds;
	lgd.db_removes = new_lgd.db_removes;
	lgd.tx_begins = new_lgd.tx_begins;
	lgd.tx_ends = new_lgd.tx_ends;
	lgd.log_writes = new_lgd.log_writes;
	lgd.log_write_ios = new_lgd.log_write_ios;
	lgd.log_read_ios = new_lgd.log_read_ios;
	lgd.log_waits = new_lgd.log_waits;
	lgd.log_split_waits = new_lgd.log_split_waits;
	lgd.log_free_waits = new_lgd.log_free_waits;
	lgd.log_stall_waits = new_lgd.log_stall_waits;
	lgd.log_forces = new_lgd.log_forces;
	lgd.log_group_commit = new_lgd.log_group_commit;
	lgd.log_group_count = new_lgd.log_group_count;
	lgd.check_commit_timer = new_lgd.check_commit_timer;
	lgd.timer_write = new_lgd.timer_write;
	lgd.kbytes_written = new_lgd.kbytes_written;
	lgd.buffer_utilisation = new_lgd.buffer_utilisation;
	lgd.avg_txn_size = new_lgd.avg_txn_size;
	lgd.block_size = new_lgd.block_size;
	lgd.inconsistent_db = new_lgd.inconsistent_db;

}

void getLogSummary()
{
	exec sql repeated select 
		lgd_stat_add,
		lgd_stat_remove,
		lgd_stat_begin,
		lgd_stat_end,
		lgd_stat_write,
		lgd_stat_writeio,
		lgd_stat_readio,
		lgd_stat_wait,
		lgd_stat_split,
		lgd_stat_free_wait,
		lgd_stat_stall_wait,
		lgd_stat_force,
		lgd_stat_group_force,
		lgd_stat_group_count,
		lgd_stat_pgyback_check,
		lgd_stat_pgyback_write,
		lgd_stat_kbytes,
                lfb_hdr_lgh_size,  /* 
##mdf200302 
*/
		lgd_stat_inconsist_db
	into
		:new_lgd.db_adds,
		:new_lgd.db_removes,
		:new_lgd.tx_begins,
		:new_lgd.tx_ends,
		:new_lgd.log_writes,
		:new_lgd.log_write_ios,
		:new_lgd.log_read_ios,
		:new_lgd.log_waits,
		:new_lgd.log_split_waits,
		:new_lgd.log_free_waits,
		:new_lgd.log_stall_waits,
		:new_lgd.log_forces,
		:new_lgd.log_group_commit,
		:new_lgd.log_group_count,
		:new_lgd.check_commit_timer,
		:new_lgd.timer_write,
		:new_lgd.kbytes_written,
		:new_lgd.block_size, /* 
##mdf200302 
*/
		:new_lgd.inconsistent_db
	from 
		imp_lgmo_lgd
            , imp_lgmo_lfb                            
        where                                           
                imp_lgmo_lgd.vnode = imp_lgmo_lfb.vnode 
        and     imp_lgmo_lfb.vnode = :currentVnode;     
	sqlerr_check();

	/* 
	** kbytes written is actually the number of VMS blocks
	** so....
	*/
	new_lgd.kbytes_written = new_lgd.kbytes_written / 2;

/*
new_lgd.buffer_utilisation = 12.5;
*/
new_lgd.buffer_utilisation =  (new_lgd.kbytes_written*100.00)
/*
 /
                        (new_lgd.log_write_ios*(new_lgd.block_size/1024.00))  
*/
;
new_lgd.avg_txn_size =  (new_lgd.kbytes_written * 1024.00/ new_lgd.tx_ends) ;
                                                                    
	new_lgd.db_adds -= lgd.db_adds;
	new_lgd.db_removes -= lgd.db_removes;
	new_lgd.tx_begins -= lgd.tx_begins;
	new_lgd.tx_ends -= lgd.tx_ends;
	new_lgd.log_writes -= lgd.log_writes;
	new_lgd.log_write_ios -= lgd.log_write_ios;
	new_lgd.log_read_ios -= lgd.log_read_ios;
	new_lgd.log_waits -= lgd.log_waits;
	new_lgd.log_split_waits -= lgd.log_split_waits;
	new_lgd.log_free_waits -= lgd.log_free_waits;
	new_lgd.log_stall_waits -= lgd.log_stall_waits;
	new_lgd.log_forces -= lgd.log_forces;
	new_lgd.log_group_commit -= lgd.log_group_commit;
	new_lgd.log_group_count -= lgd.log_group_count;
	new_lgd.check_commit_timer -= lgd.check_commit_timer;
	new_lgd.timer_write -= lgd.timer_write;
	new_lgd.kbytes_written -= lgd.kbytes_written;
        new_lgd.buffer_utilisation -=  lgd.buffer_utilisation;
        new_lgd.avg_txn_size -=  lgd.avg_txn_size;
	new_lgd.inconsistent_db -= lgd.inconsistent_db;

		
}

void displayLogSummary()
{
	exec frs putform imp_lg_summary 
		(vnode = :currentVnode);
	exec frs putform imp_lg_summary 
		(db_adds = :new_lgd.db_adds);
	exec frs putform imp_lg_summary 
		(db_removes = :new_lgd.db_removes);
	exec frs putform imp_lg_summary 
		(tx_begins = :new_lgd.tx_begins);
	exec frs putform imp_lg_summary 
		(tx_ends = :new_lgd.tx_ends);
	exec frs putform imp_lg_summary 
		(log_writes = :new_lgd.log_writes);
	exec frs putform imp_lg_summary 
		(log_write_ios = :new_lgd.log_write_ios);
	exec frs putform imp_lg_summary 
		(log_read_ios = :new_lgd.log_read_ios);
	exec frs putform imp_lg_summary 
		(log_waits = :new_lgd.log_waits);
	exec frs putform imp_lg_summary 
		(log_split_waits = :new_lgd.log_split_waits);
	exec frs putform imp_lg_summary 
		(log_free_waits = :new_lgd.log_free_waits);
	exec frs putform imp_lg_summary 
		(log_stall_waits = :new_lgd.log_stall_waits);
	exec frs putform imp_lg_summary 
		(log_forces = :new_lgd.log_forces);
	exec frs putform imp_lg_summary 
		(log_group_commit = :new_lgd.log_group_commit);
	exec frs putform imp_lg_summary 
		(log_group_count = :new_lgd.log_group_count);
	exec frs putform imp_lg_summary 
		(check_commit_timer = :new_lgd.check_commit_timer);
	exec frs putform imp_lg_summary 
		(timer_write = :new_lgd.timer_write);
	exec frs putform imp_lg_summary 
		(kbytes_written = :new_lgd.kbytes_written);
	exec frs putform imp_lg_summary 
                (block_size =  :new_lgd.block_size);
/*
	exec frs putform imp_lg_summary 
                (avg_txn_size =  :new_lgd.avg_txn_size);
	exec frs putform imp_lg_summary 
                (buffer_utilisation =  :new_lgd.buffer_utilisation);
*/
	exec frs putform imp_lg_summary 
		(inconsistent_db = :new_lgd.inconsistent_db);

	exec frs redisplay;
}

exec sql begin declare section;
struct 	lg_header 	hdr;
char	barchart[BARCHART_LENGTH + 2];
exec sql end declare section;

int
processLogHeader()
{

	exec frs display imp_lg_header read;
	exec frs initialize;
	exec frs begin;
		exec frs putform imp_lg_header (vnode = :currentVnode);
		getHeaderInfo();
		displayHeaderInfo();
		exec frs set_frs frs (timeout = :timeVal);
	exec frs end;

	exec frs activate timeout;
	exec frs begin;
		getHeaderInfo();
		displayHeaderInfo();
	exec frs end;

	exec frs activate menuitem 'Refresh';
	exec frs begin;
		getHeaderInfo();
		displayHeaderInfo();
	exec frs end;

	exec frs activate menuitem 'End', frskey3;
	exec frs begin;
		exec frs set_frs frs (timeout = 0);
		exec frs breakdisplay;
	exec frs end;

	return(TRUE);
}

int
deriveBlockNo(str)
char	*str;
{
	long	discard;
	int	block;
	if ((sscanf(str,"<%ld,%d,%d>",&discard,&block,&discard)) != 3) {
		return(-1);
	} 
	return(block);
}

getHeaderInfo()
{

	int	last_cp;
	int	next_cp;
	int	start;
	int	end;
	int	reserve;
	int	i;
	int	j;

	exec sql repeated select 
		lgd_status,
		lfb_hdr_lgh_count,
		lfb_hdr_lgh_size,
		lfb_buf_cnt,
		lfb_hdr_lgh_l_logfull,
		lfb_hdr_lgh_l_abort,
		lfb_hdr_lgh_l_cp,
		lfb_hdr_lgh_begin,
		lfb_hdr_lgh_end,
		lfb_hdr_lgh_cp,
		lfb_reserved_space
	into
		:hdr.status,
		:hdr.block_count,
		:hdr.block_size,
		:hdr.buf_count,
		:hdr.logfull_interval,
		:hdr.abort_interval,
		:hdr.cp_interval,
		:hdr.begin_addr,
		:hdr.end_addr,
		:hdr.cp_addr,
		:hdr.reserved_blocks
	from
		imp_lgmo_lgd,
		imp_lgmo_lfb
	where
		imp_lgmo_lgd.vnode = imp_lgmo_lfb.vnode
	and	imp_lgmo_lfb.vnode = :currentVnode;
	sqlerr_check();

	/*
	** derive the rest of the required information
	*/
	hdr.begin_block = deriveBlockNo(hdr.begin_addr);
	hdr.end_block = deriveBlockNo(hdr.end_addr);
	hdr.cp_block = deriveBlockNo(hdr.cp_addr);
	
	hdr.reserved_blocks = hdr.reserved_blocks / hdr.block_size;
	
	hdr.inuse_blocks = (hdr.end_block > hdr.begin_block) ?
				hdr.end_block - hdr.begin_block :
				(hdr.block_count - hdr.begin_block) +
				hdr.end_block;

	hdr.avail_blocks = hdr.block_count - 
				(hdr.reserved_blocks + hdr.inuse_blocks);
	if ((hdr.cp_block + hdr.cp_interval) > hdr.block_count) {
		hdr.next_cp_block = (hdr.cp_block + hdr.cp_interval) 
					- hdr.block_count;
	} else {
		hdr.next_cp_block = (hdr.cp_block + hdr.cp_interval);
	}
	/*
	** calculate the barchart
	*/
	last_cp = (hdr.cp_block * BARCHART_LENGTH) / hdr.block_count;
	next_cp = (hdr.next_cp_block * BARCHART_LENGTH) / hdr.block_count;
	start = (hdr.begin_block * BARCHART_LENGTH) / hdr.block_count;
	end = (hdr.end_block * BARCHART_LENGTH) / hdr.block_count;
	/*
	** reserved blocks may be shown at the end of the normal blocks?
	*/
	reserve = (hdr.reserved_blocks * BARCHART_LENGTH) /
			hdr.block_count;

	for (i = 0; i < BARCHART_LENGTH; i++) {
		barchart[i] = ' ';
	}
	barchart[BARCHART_LENGTH + 1] = '\0';
	barchart[last_cp] = 'C';
	barchart[next_cp] = 'C';
	if (start == end) {
		barchart[start] = '*';
	} else if (start < end) {
		for (i = start; i <= end; i++) {
			barchart[i] = '-';
		}
		barchart[start] = '>';
		barchart[end] = '>';
	} else {
		for (i = start; i <= BARCHART_LENGTH; i++) {
			barchart[i] = '-';
		}
		for (i = 0; i < end; i++) {
			barchart[i] = '-';
		}
		barchart[start] = '>';
		barchart[end] = '>';
	}

	j = reserve;
	for (i = (start - 1); (i >= 0) && (j > 0); i--) {
		barchart[i] = 'R';
		j--;
	}
	if (j != 0) {
		for (i = BARCHART_LENGTH; j > 0; i--) {
			barchart[i] = 'R';
			j--;
		}
	}
	barchart[last_cp] = 'C';
	barchart[next_cp] = 'C';
}

void displayHeaderInfo()
{
	exec sql begin declare section;
	int 	pctInUse;
	char	buf[30];
	exec sql end declare section;

	exec frs putform imp_lg_header
		(status = :hdr.status);
	exec frs putform imp_lg_header
		(block_count = :hdr.block_count);
	exec frs putform imp_lg_header
		(block_size = :hdr.block_size);
	exec frs putform imp_lg_header
		(buffer_count = :hdr.buf_count);
	exec frs putform imp_lg_header
		(logfull_interval = :hdr.logfull_interval);
	exec frs putform imp_lg_header
		(abort_interval = :hdr.abort_interval);
	exec frs putform imp_lg_header
		(cp_interval = :hdr.cp_interval);
	exec frs putform imp_lg_header
		(bof = :hdr.begin_block);
	exec frs putform imp_lg_header
		(eof = :hdr.end_block);
	exec frs putform imp_lg_header
		(previous_cp = :hdr.cp_block);
	exec frs putform imp_lg_header
		(next_cp = :hdr.next_cp_block);
	exec frs putform imp_lg_header
		(reserved = :hdr.reserved_blocks);
	exec frs putform imp_lg_header
		(in_use = :hdr.inuse_blocks);
	exec frs putform imp_lg_header
		(available = :hdr.avail_blocks);
	pctInUse = (100 * (hdr.inuse_blocks + hdr.reserved_blocks)) /
			hdr.block_count;
	exec frs putform imp_lg_header
		(log_pct = :pctInUse);
	exec frs putform imp_lg_header
		(barchart = :barchart);

	/*
	** instead of calculating foce abort and logfull, simply 
	** look for the status
	*/
	if (strstr(hdr.status,"LOGFULL") != (char *) NULL) {
		strcpy(buf,"LOGFULL");
		exec frs putform imp_lg_header
			(emergency_status = :buf);
		exec frs set_frs field imp_lg_header
			(invisible(emergency_status) = 0, 
			blink(emergency_status) = 1);
	} else if (strstr(hdr.status,"FORCE") != (char *) NULL) {
		strcpy(buf,"FORCE ABORT");
		exec frs putform imp_lg_header
			(emergency_status = :buf);
		exec frs set_frs field imp_lg_header
			(invisible(emergency_status) = 0, 
			blink(emergency_status) = 0);
	} else {
		exec frs set_frs field imp_lg_header
			(invisible(emergency_status) = 1, 
			blink(emergency_status) = 0);
	}
	exec frs redisplay;
}

int
processLogProcesses()
{
	exec frs display imp_lg_processes read;
	exec frs initialize;
	exec frs begin;
		exec frs putform imp_lg_processes (vnode = :currentVnode);
		displayLogProcesses();
		exec frs set_frs frs (timeout = :timeVal);
	exec frs end;

	exec frs activate menuitem 'Refresh';
	exec frs begin;
		displayLogProcesses();
	exec frs end;
	
	exec frs activate timeout;
	exec frs begin;
		displayLogProcesses();
	exec frs end;
	
	exec frs activate menuitem 'End', frskey3;
	exec frs begin;
		exec frs set_frs frs (timeout = 0);
		exec frs breakdisplay;
	exec frs end;

	return(TRUE);
}

void displayLogProcesses()
{
	exec sql begin declare section;
	char	computed_id[20];
	long	id_id;
	long	id_instance;
	long	pid;
	int	status_val;
	char	status[30];
	long	open_db;
	long	writes;
	long	forces;
	long	waits;
	long 	begins;
	long	ends;
	char	typeBuf[100];
	exec sql end declare section;


	exec frs inittable imp_lg_processes process_tbl read;
	exec sql repeated select
		lpb_id_id,
		lpb_id_instance,
		lpb_pid,
		lpb_status_num,
		lpb_lpd_count,
		lpb_stat_write,
		lpb_stat_force,
		lpb_stat_wait,
		lpb_stat_begin,
		lpb_stat_end
	into
		:id_id,
		:id_instance,
		:pid,
		:status_val,
		:open_db,
		:writes,
		:forces,
		:waits,
		:begins,
		:ends
	from
		imp_lgmo_lpb
	where
		vnode = :currentVnode;
	exec sql begin;
		sqlerr_check();
		sprintf(computed_id,"%lx",(65536 * id_id) + id_instance);
		decodeStatusVal(status_val,typeBuf);
		exec frs loadtable imp_lg_processes process_tbl (
			computed_id = :computed_id,
			pid = :pid,
			process_type = :typeBuf,
			open_db = :open_db,
			writes = :writes,
			forces = :forces,
			waits = :waits,
			begins = :begins,
			ends = :ends);
	exec sql end;
	sqlerr_check();
	exec frs redisplay;
}

void TransactionDetail(tx_id)
char	*tx_id;
{
    exec sql begin declare section;       
        char	l_session_id[40];              /* 
##mdf040903
*/
	int	l_server_pid;                  /* 
##mdf040903
*/
        char buf[100];
        char    l_server_name[SERVER_NAME_LENGTH];      /* 
##mdf040903
*/
        SessEntry        *current_sess;   /*
##mdf040903
*/    
        SessEntry        *SessListHead;                    
    exec sql end declare section;       
    exec frs display imp_transaction_detail read;
    exec frs initialize;
	exec frs begin;
        if ( DisplayTransactionDetail(tx_id) == FALSE) {    
                exec frs breakdisplay;               
        }                                            
	    exec frs set_frs frs (timeout = :timeVal);
	exec frs end;

        exec frs activate menuitem 'Refresh';
        exec frs begin;                             
        if ( DisplayTransactionDetail(tx_id) == FALSE) {    
                exec frs breakdisplay;               
        }                                            
        exec frs end;                               

        exec frs activate menuitem 'Session_Info';                    
        exec frs begin;                                               
                                                                      
            exec frs getform imp_transaction_detail  (:l_session_id = lxb_sid);
            exec frs getform imp_transaction_detail  (:l_server_pid = lxb_pid);

            getServerName(l_server_pid, l_server_name);

            /* current_sess = SessListHead; */
            if (current_sess = newSessEntry()) {                      
                strcpy(current_sess->sessionId,l_session_id );              
                }                                                     
                                                                      
                                                                      
            if (current_sess) {                                       
                    displaySessionDetail(current_sess,l_server_name);   
            }                                                         
            /* On return, refresh current screen */
            if ( DisplayTransactionDetail(tx_id) == FALSE) {    
                exec frs breakdisplay;               
            }                                            
        exec frs end;                                                 
                                                                      
        exec frs activate menuitem 'Dist_Txn_Hex_Dump';
        exec frs begin;                             
        if ( DisplayHexDump(tx_id) == FALSE) {    
                exec frs breakdisplay;               
        }                                            
        exec frs end;                               

       exec frs activate menuitem 'End', frskey3;  
       exec frs begin;                             
               exec frs breakdisplay;              
       exec frs end;                               
}


int
DisplayHexDump(p_tx_id)
exec sql begin declare section;
    char	*p_tx_id;
exec sql end declare section;
{
    exec sql begin declare section;
    int  cnt;  
    char msgbuf[100];
    char l_lxb_dis_tran_id_hexdump[350];
    exec sql end declare section;

    exec frs display imp_transaction_hexdump read;
    exec frs initialize;
    exec frs begin;
        exec frs set_frs frs (timeout = :timeVal);

        exec sql repeated select
                lxb_dis_tran_id_hexdump 
        into
                    :l_lxb_dis_tran_id_hexdump 
        from imp_lgmo_lxb
        where lxb_tran_id = :p_tx_id;
        sqlerr_check();
        exec sql inquire_sql (:cnt = rowcount);                      
        if (cnt == 0) {                                              
            sprintf(msgbuf,"No information for Transaction id %s",p_tx_id); 
            exec frs message :msgbuf with style = popup;         
            return(FALSE);                                       
        }                                                            

        exec frs putform imp_transaction_hexdump (lxb_dis_tran_id_hexdump = :l_lxb_dis_tran_id_hexdump );
    exec frs end;

   exec frs activate menuitem 'End', frskey3;  
   exec frs begin;                             
           exec frs breakdisplay;              
   exec frs end;                               
    return(TRUE);                                       
}


int
DisplayTransactionDetail(p_tx_id)
exec sql begin declare section;
    char	*p_tx_id;
exec sql end declare section;
{
    exec sql begin declare section;
    int  cnt;  
    char msgbuf[100];
    char l_lxb_db_name[32];
    char l_lxb_db_owner[32];
    char l_lxb_first_lga[32];
    int  l_lxb_id_id;
    int  l_lxb_id_instance;
    char l_lxb_is_prepared[2];
    char l_lxb_is_xa_dis_tran_id[2];
    char l_lxb_last_lga[32];
    char l_lxb_last_lsn[32];
    int  l_lxb_pid;
    int  l_lxb_reserved_space;
    char l_lxb_sid[40];
    int  l_lxb_stat_force;
    int  l_lxb_stat_split;
    int  l_lxb_stat_wait;
    int  l_lxb_stat_write;
    char l_lxb_status[101];
    char l_lxb_user_name[101];
    char l_lxb_wait_reason[21];
    char l_vnode[70];
    char l_lxb_tran_id[20];
    exec sql end declare section;

    exec sql repeated select
            lxb_db_name             
            ,lxb_db_owner            
            ,lxb_first_lga           
            ,lxb_id_id               
            ,lxb_id_instance         
            ,lxb_is_prepared         
            ,lxb_is_xa_dis_tran_id   
            ,lxb_last_lga            
            ,lxb_last_lsn            
            ,lxb_pid                 
            ,lxb_reserved_space      
            ,lxb_sid                 
            ,lxb_stat_force          
            ,lxb_stat_split          
            ,lxb_stat_wait           
            ,lxb_stat_write          
            ,lxb_status              
            ,lxb_user_name           
            ,lxb_wait_reason         
            ,vnode                   
            ,lxb_tran_id             
    into
             :l_lxb_db_name             
            , :l_lxb_db_owner            
            , :l_lxb_first_lga           
            , :l_lxb_id_id               
            , :l_lxb_id_instance         
            , :l_lxb_is_prepared         
            , :l_lxb_is_xa_dis_tran_id   
            , :l_lxb_last_lga            
            , :l_lxb_last_lsn            
            , :l_lxb_pid                 
            , :l_lxb_reserved_space      
            , :l_lxb_sid                 
            , :l_lxb_stat_force          
            , :l_lxb_stat_split          
            , :l_lxb_stat_wait           
            , :l_lxb_stat_write          
            , :l_lxb_status              
            , :l_lxb_user_name           
            , :l_lxb_wait_reason         
            , :l_vnode
            , :l_lxb_tran_id             
    from imp_lgmo_lxb
    where lxb_tran_id = :p_tx_id;
    sqlerr_check();
    exec sql inquire_sql (:cnt = rowcount);                      
    if (cnt == 0) {                                              
                sprintf(msgbuf,"No information for Transaction id %s",p_tx_id); 
                exec frs message :msgbuf with style = popup;         
                return(FALSE);                                       
    }                                                            

    exec frs putform imp_transaction_detail (lxb_db_name = :l_lxb_db_name );
    exec frs putform imp_transaction_detail (lxb_db_owner = :l_lxb_db_owner );
    exec frs putform imp_transaction_detail (lxb_first_lga = :l_lxb_first_lga );
    exec frs putform imp_transaction_detail (lxb_tran_id = :l_lxb_tran_id );
    exec frs putform imp_transaction_detail (lxb_id_id = :l_lxb_id_id );
    exec frs putform imp_transaction_detail (lxb_id_instance = :l_lxb_id_instance );
    exec frs putform imp_transaction_detail (lxb_is_prepared = :l_lxb_is_prepared );
    exec frs putform imp_transaction_detail (lxb_is_xa_dis_tran_id = :l_lxb_is_xa_dis_tran_id );
    exec frs putform imp_transaction_detail (lxb_last_lga = :l_lxb_last_lga );
    exec frs putform imp_transaction_detail (lxb_last_lsn = :l_lxb_last_lsn );
    exec frs putform imp_transaction_detail (lxb_pid = :l_lxb_pid );
    exec frs putform imp_transaction_detail (lxb_reserved_space = :l_lxb_reserved_space );
    exec frs putform imp_transaction_detail (lxb_sid = :l_lxb_sid );
    exec frs putform imp_transaction_detail (lxb_stat_force = :l_lxb_stat_force );
    exec frs putform imp_transaction_detail (lxb_stat_split = :l_lxb_stat_split );
    exec frs putform imp_transaction_detail (lxb_stat_wait = :l_lxb_stat_wait );
    exec frs putform imp_transaction_detail (lxb_stat_write = :l_lxb_stat_write );
    exec frs putform imp_transaction_detail (lxb_status = :l_lxb_status );
    exec frs putform imp_transaction_detail (lxb_user_name = :l_lxb_user_name );
    exec frs putform imp_transaction_detail (lxb_wait_reason = :l_lxb_wait_reason );
    exec frs putform imp_transaction_detail (vnode = :l_vnode );

    if   (strcmp(l_lxb_is_xa_dis_tran_id,"N") == 0) {
            exec frs set_frs menu '' (active('Dist_Txn_Hex_Dump') = off);
    }
    else
    {
            exec frs set_frs menu '' (active('Dist_Txn_Hex_Dump') = on);
    }
    exec frs redisplay;

    return(TRUE);                                       
}

int
decodeStatusVal(val,buf)
int	val;
char	*buf;
{

	/* note that we do not check the validity of buf */


	*buf = '\0';

	if (val & LPB_MASTER) {
		strcat(buf,"RCP ");
	}
	if (val & LPB_ARCHIVER) {
		strcat(buf,"ACP ");
	}
	if (val & LPB_FCT) {
		strcat(buf,"FCTDBMS ");
	}
	if (val & LPB_RUNAWAY) {
		strcat(buf,"RUNAWAY ");
	}
	if (val & LPB_SLAVE) {
		strcat(buf,"DBMS ");
	}
	if (val & LPB_CKPDB) {
		strcat(buf,"CKP ");
	}
	if (val & LPB_VOID) {
		strcat(buf,"VOID ");
	}
	if (val & LPB_SHARED_BUFMGR) {
		strcat(buf,"SHARED ");
	}
	if (val & LPB_IDLE) {
		strcat(buf,"IDLE ");
	}
	if (val & LPB_DEAD) {
		strcat(buf,"DEAD ");
	}
	if (val & LPB_DYING) {
		strcat(buf,"DYING ");
	}
	if (val & LPB_FOREIGN_RUNDOWN) {
		strcat(buf,"RUNDOWN ");
	}
	if (val & LPB_CPAGENT) {
		strcat(buf,"CPAGENT ");
	}
	return(TRUE);
}

int
processLogDatabases()
{
	exec frs display imp_lg_databases read;
	exec frs initialize;
	exec frs begin;
		exec frs putform imp_lg_processes (vnode = :currentVnode);
		displayLogDatabases();
		exec frs set_frs frs (timeout = :timeVal);
	exec frs end;

	exec frs activate menuitem 'Refresh';
	exec frs begin;
		displayLogDatabases();
	exec frs end;
	
	exec frs activate timeout;
	exec frs begin;
		displayLogDatabases();
	exec frs end;
	
	exec frs activate menuitem 'End', frskey3;
	exec frs begin;
		exec frs set_frs frs (timeout = 0);
		exec frs breakdisplay;
	exec frs end;

	return(TRUE);
}

void displayLogDatabases()
{
	exec sql begin declare section;
	char	dbname[40];
	char	dbstatus[40];
	long	tx_count;
	long 	db_begins;
	long	db_ends;
	long	db_reads;
	long	db_writes;
	exec sql end declare section;

	exec frs inittable imp_lg_databases logdb_tbl read;
	exec sql repeated select
		ldb_db_name,
		ldb_status,
		ldb_lxb_count,
		ldb_stat_begin,
		ldb_stat_end,
		ldb_stat_read,
		ldb_stat_write
	into
		:dbname,
		:dbstatus,
		:tx_count,
		:db_begins,
		:db_ends,
		:db_reads,
		:db_writes
	from
		imp_lgmo_ldb
	where
		vnode = :currentVnode;
	exec sql begin;
		sqlerr_check();
		exec frs loadtable imp_lg_databases logdb_tbl (
			dbname = :dbname,
			dbstatus = :dbstatus,
			tx_count = :tx_count,
			begins = :db_begins,
			ends = :db_ends,
			reads = :db_reads,
			writes = :db_writes);
	exec sql end;
	exec frs redisplay;
}

int
processLogTransactions()
{
        exec sql begin declare section;       
	char	txn_id[40];                  /* 
##mdf040903 
*/
	char	session_id[40];              /* 
##mdf040903
*/
	int	server_pid;                  /* 
##mdf040903
*/
	int	llb_id_id;                   /* 
##mdf040903
*/
        char    server_name[SERVER_NAME_LENGTH];      /* 
##mdf040903
*/
        char buf[100];
        SessEntry        *current_sess;   /*
##mdf040903
*/    
        SessEntry        *SessListHead;                    
        exec sql end declare section;         

	exec frs display imp_lg_transactions read;
	exec frs initialize;
	exec frs begin;
		exec frs putform imp_lg_transactions (vnode = :currentVnode);
		displayLogTransactions();
		exec frs set_frs frs (timeout = 0);
		exec frs set_frs frs (timeout = :timeVal); /* 
##mdf040903
*/
	exec frs end;

	
	exec frs activate timeout;/* 
##mdf040903 
*/
	exec frs begin;
		displayLogTransactions();
	exec frs end;

	exec frs activate menuitem 'Refresh';
	exec frs begin;
		displayLogTransactions();
	exec frs end;
	
	exec frs activate menuitem 'Tx_Detail';
	exec frs begin;
            exec frs getrow imp_lg_transactions log_txtbl 
                        (:txn_id = ext_tx_id);                       
	    TransactionDetail(txn_id);
	    displayLogTransactions();
	exec frs end;
	
	exec frs activate menuitem 'Lock_Info';
	exec frs begin;
            exec frs getrow imp_lg_transactions log_txtbl 
                        (:txn_id = ext_tx_id
                        ,:session_id = session_id
                        ,:server_pid = server_pid
                        );                       

            exec sql select llb_id_id 
            into :llb_id_id 
            from imp_lkmo_llb  llb1
            where vnode = :currentVnode
            and llb_pid= :server_pid
            and llb_sid= :session_id
            and llb_related_llb_id_id in(
                    select llb_id_id 
                    from imp_lkmo_llb  llb2
                    where vnode= llb1.vnode
                    and llb_pid= llb1.llb_pid
                    and llb_sid= llb1.llb_sid
                    );
             if (sqlerr_check() == NO_ROWS) {                                
                exec sql commit;
                sprintf(buf,"No Session information for Transaction id %s",txn_id); 
                exec frs message :buf with style = popup;         
             }                                                               
             else {
                exec sql commit;
                examineLockList(server_pid,session_id,llb_id_id) ;
             }                                                               
	exec frs end;
	
        exec frs activate menuitem 'Session_Info';                    
        exec frs begin;                                               
            exec frs getrow imp_lg_transactions log_txtbl 
                        (:txn_id = ext_tx_id
                        ,:session_id = session_id
                        ,:server_pid = server_pid
                        );                       
                                                                      
            getServerName(server_pid, server_name);

            /* current_sess = SessListHead; */

            if (current_sess = newSessEntry()) {                      
                strcpy(current_sess->sessionId,session_id );              
                }                                                     
                                                                      
                                                                      
            if (current_sess) {                                       
                    displaySessionDetail(current_sess,server_name);   
            }                                                         
            displayLogTransactions();
        exec frs end;                                                 
                                                                      
	exec frs activate menuitem 'End', frskey3;
	exec frs begin;
		exec frs set_frs frs (timeout = 0);
		exec frs breakdisplay;
	exec frs end;


	return(TRUE);
}

void displayLogTransactions()
{
    exec sql begin declare section;
	int	server_pid;                 /* 
##mdf040903
*/
	char	session_id[40];             /* 
##mdf040903
*/
        int     cnt;                        /* 
##mdf040903
*/
	char	tbl_tx_id[40];              /* 
##mdf040903
*/
        int     h_row;                        /* 
##mdf040903
*/
	char	tx_id[40];
	char	dbname[40];
	char	txstatus[40];
	char	txname[40];
	long	db_splits;
	long	db_writes;
    exec sql end declare section;

    /* 
##mdf040903. 
Remember the current tx_id so we can scroll back to it */
    exec frs inquire_frs table imp_lg_transactions 
        (:h_row = datarows(log_txtbl ));
        if (h_row > 0)
    {
        exec frs getrow imp_lg_transactions log_txtbl 
                    (:tbl_tx_id = ext_tx_id);                       
    }
    h_row = 0;
    exec frs inittable imp_lg_transactions log_txtbl read;
    exec sql repeated select
		lxb_pid,                    /*
##mdf040903
*/
		lxb_sid,                    /*
##mdf040903
*/
		lxb_tran_id,
		lxb_db_name,
		lxb_user_name,
		lxb_status,
		lxb_stat_write,
		lxb_stat_split
    into
		:server_pid,                    /*
##mdf040903
*/
		:session_id,                    /*
##mdf040903
*/
		:tx_id,
		:dbname,
		:txname,
		:txstatus,
		:db_writes,
		:db_splits
	from
		imp_lgmo_lxb
	where
		vnode = :currentVnode
	and	lxb_status like 'ACTIVE%'
        order by lxb_stat_write desc;
	exec sql begin;
		sqlerr_check();
		exec frs loadtable imp_lg_transactions log_txtbl (
			server_pid = :server_pid,    /*
##mdf040903
*/
			session_id = :session_id,    /*
##mdf040903
*/
			ext_tx_id = :tx_id,
			database = :dbname,
			username = :txname,
			status = :txstatus,
			writes = :db_writes,
			splits = :db_splits);
                if (strcmp(tx_id, tbl_tx_id)==0 )
                {
                    exec frs inquire_frs table imp_lg_transactions 
                        (:h_row = datarows(log_txtbl ));
                }
	exec sql end;
	sqlerr_check();
        exec sql inquire_sql (:cnt = rowcount);  /*
##mdf040903
*/
        exec sql commit;                         /*
##mdf040903
*/
        if (cnt == 0) {                    
            exec frs set_frs menu '' (active('Tx_Detail') = off);
            exec frs set_frs menu '' (active('Lock_Info') = off);
            exec frs set_frs menu '' (active('Session_Info') = off);
                }                          
        else {                             
            exec frs set_frs menu '' (active('Tx_Detail') = on);
            exec frs set_frs menu '' (active('Lock_Info') = on);
            exec frs set_frs menu '' (active('Session_Info') = on);
            if (h_row > 0) {                    
                exec frs scroll imp_lg_transactions log_txtbl to :h_row;
                } 
            }                          
	exec frs redisplay;
}


void getServerName(p_server_pid, p_server_name)
exec sql begin declare section;    
    int    p_server_pid;         
    char   *p_server_name;         
exec sql end declare section; 
{                                   
    exec sql begin declare section;
    char msgbuf[100];
    char   l_server_name[20];         
    exec sql end declare section; 
    /*
    ** Based on the tx_id, and the session_info
    ** get the server listen address
    */
    exec sql repeated select server
    into :l_server_name
    from imp_srv_block
    where server_pid= :p_server_pid  ;

    sqlerr_check();
    strcpy(p_server_name,l_server_name );              
}
