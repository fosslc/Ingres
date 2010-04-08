/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name:	impqsf.sc
**
** This file contains all of the routines needed to support the "QSF" 
** Subcomponent of the IMA based IMP lookalike tools.
**
## History:
##
##	21-Jun-95	(johna)
##		Incorporated from earlier QSF tools
##
##      10-Apr-02       (mdf)
##              Expand domain to handle looking at a different QSF .
##
##      17-Apr-02       (mdf170402)
##              Sort the dbprocs on usage, in descending order.
##      02-Jul-2002 (fanra01)
##          Sir 108159
##          Added to distribution in the sig directory.
##	08-Oct-2007 (hanje04)
##	    SIR 114907
##	    Remove declaration of timebuf as it's not referenced and causes
##	    errors on Mac OSX
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "imp.h"

exec sql include sqlca;
exec sql include 'impcommon.sc';

exec sql begin declare section;

struct qsr_ {
	char	server[65];
	long	qsr_bkts_used;
	long	qsr_bmaxobjs;
	long	qsr_decay_factor;
	long	qsr_memleft;
	long	qsr_memtot;
	long	qsr_mx_index;
	long	qsr_mx_named;
	long	qsr_mx_rsize;
	long	qsr_mx_size;
	long	qsr_mx_unnamed;
	long	qsr_mxbkts_used;
	long	qsr_mxobjs;
	long	qsr_mxsess;
	long	qsr_named_requests;
	long	qsr_nbuckets;
	long	qsr_no_destroyed;
	long	qsr_no_index;
	long	qsr_no_named;
	long	qsr_no_unnamed;
	long	qsr_nobjs;
	long	qsr_nsess;
} qsr;

char 	bar_chart[51];
float	pct_used;

extern 	int 	ima_qsf_cache_stats;
extern 	int 	inFormsMode;
extern 	int 	imaSession;
extern 	int 	iidbdbSession;
extern 	int 	userSession;
extern 	char	*currentVnode;

exec sql end declare section;

/*
** Name:	impqsfsum.sc
**
** Display the QSF cache statistics summary
**
*/

exec sql begin declare section;
extern int 	timeVal;
exec sql end declare section;

int
processQsfSummary(server)
exec sql begin declare section;
GcnEntry	*server;
exec sql end declare section;
{
	exec sql begin declare section;
	char	server_name[SERVER_NAME_LENGTH];
	exec sql end declare section;

	exec sql set_sql (printtrace = 1);
	exec sql select dbmsinfo('IMA_VNODE') into :server_name;
	strcat(server_name,"::/@");
	strcat(server_name,server->serverAddress);
        exec sql execute procedure
                  imp_set_domain(entry = :server_name); /* 
##mdf100402 
*/

	exec frs display ima_qsf_cache_stats read;
	exec frs initialize;
	exec frs begin;
		exec frs putform ima_qsf_cache_stats (server = :server_name);
		getQsfSummary(server_name);
		displayQsfSummary();
		exec frs set_frs frs (timeout = :timeVal);

	exec frs end;

	exec frs activate timeout;
	exec frs begin;
		getQsfSummary(server_name);
		displayQsfSummary();
	exec frs end;

	exec frs activate menuitem 'Flush';
	exec frs begin;
		clear_qsf();
		getQsfSummary(server_name);
		displayQsfSummary();
	exec frs end;

	exec frs activate menuitem 'Procedures';
	exec frs begin;
		exec frs set_frs frs (timeout = 0);
		processQsfProcedures(server_name,"ALL");
		exec frs set_frs frs (timeout = :timeVal);
	exec frs end;

	exec frs activate menuitem 'RepeatQueries';
	exec frs begin;
		exec frs set_frs frs (timeout = 0);
		processQsfQueries(server_name);
		exec frs set_frs frs (timeout = :timeVal);
	exec frs end;

	exec frs activate menuitem 'Refresh';
	exec frs begin;
		getQsfSummary(server_name);
		displayQsfSummary();
	exec frs end;

	exec frs activate menuitem 'End', frskey3;
	exec frs begin;
	exec frs set_frs frs (timeout = 0);
	exec sql set_sql (printtrace = 0);
	exec frs breakdisplay;
	exec frs end;

	return(TRUE);
}

getQsfSummary(server_name)
exec sql begin declare section;
char	*server_name;
exec sql end declare section;
{
	int i;
	int blks;
	exec sql begin declare section;
	char	 msgBuf[100];
	exec sql end declare section;

	exec sql repeated select 
		server,
		qsf_bkts_used,
		qsf_bmaxobjs,
		qsf_decay_factor,
		qsf_memleft,
		qsf_memtot,
		qsf_mx_index,
		qsf_mx_named,
		qsf_mx_rsize,
		qsf_mx_size,
		qsf_mx_unnamed,
		qsf_mxbkts_used,
		qsf_mxobjs,
		qsf_mxsess,
		qsf_named_requests,
		qsf_nbuckets,
		qsf_no_destroyed,
		qsf_no_index,
		qsf_no_named,
		qsf_no_unnamed,
		qsf_nobjs,
		qsf_nsess
	into
		:qsr.server,
		:qsr.qsr_bkts_used,
		:qsr.qsr_bmaxobjs,
		:qsr.qsr_decay_factor,
		:qsr.qsr_memleft,
		:qsr.qsr_memtot,
		:qsr.qsr_mx_index,
		:qsr.qsr_mx_named,
		:qsr.qsr_mx_rsize,
		:qsr.qsr_mx_size,
		:qsr.qsr_mx_unnamed,
		:qsr.qsr_mxbkts_used,
		:qsr.qsr_mxobjs,
		:qsr.qsr_mxsess,
		:qsr.qsr_named_requests,
		:qsr.qsr_nbuckets,
		:qsr.qsr_no_destroyed,
		:qsr.qsr_no_index,
		:qsr.qsr_no_named,
		:qsr.qsr_no_unnamed,
		:qsr.qsr_nobjs,
		:qsr.qsr_nsess	
	from 
		ima_qsf_cache_stats
	where
		server = :server_name;

	sqlerr_check();

	/*
	** calculate the percentage used and the bar_chart
	*/
	pct_used = (100 * (qsr.qsr_memtot - qsr.qsr_memleft)) ;
	pct_used = pct_used / qsr.qsr_memtot;
	blks = (pct_used * 50) / 100;
	for (i = 0; i < blks; i++) {
		bar_chart[i] = '%';
	}
	bar_chart[i] = '\0';
}

clear_qsf()
{
	exec sql set trace point qs506;
}

displayQsfSummary()
{
	exec frs putform ima_qsf_cache_stats (
		server = :qsr.server,
		qsr_bkts_used = :qsr.qsr_bkts_used,
		qsr_bmaxobjs = :qsr.qsr_bmaxobjs,
		qsr_decay_factor = :qsr.qsr_decay_factor,
		qsr_memleft = :qsr.qsr_memleft,
		qsr_memtot = :qsr.qsr_memtot,
		qsr_mx_index = :qsr.qsr_mx_index,
		qsr_mx_named = :qsr.qsr_mx_named,
		qsr_mx_rsize = :qsr.qsr_mx_rsize,
		qsr_mx_size = :qsr.qsr_mx_size,
		qsr_mx_unnamed = :qsr.qsr_mx_unnamed,
		qsr_mxbkts_used = :qsr.qsr_mxbkts_used,
		qsr_mxobjs = :qsr.qsr_mxobjs,
		qsr_mxsess = :qsr.qsr_mxsess,
		qsr_named_requests = :qsr.qsr_named_requests,
		qsr_nbuckets = :qsr.qsr_nbuckets,
		qsr_no_destroyed = :qsr.qsr_no_destroyed,
		qsr_no_index = :qsr.qsr_no_index,
		qsr_no_named = :qsr.qsr_no_named,
		qsr_no_unnamed = :qsr.qsr_no_unnamed,
		qsr_nobjs = :qsr.qsr_nobjs,
		qsr_nsess = :qsr.qsr_nsess,
		pct_used = :pct_used,
		bar_chart = :bar_chart);
}


processQsfProcedures(server_name,user_name)
exec sql begin declare section;
char	*server_name;
char	*user_name;
exec sql end declare section;
{

	exec frs display ima_qsf_dbp read;
	exec frs initialize;
	exec frs begin;
		exec frs putform ima_qsf_dbp (server = :server_name);
		displayDbProcs(user_name);
	exec frs end;

	exec frs activate timeout;
	exec frs begin;
		displayDbProcs(user_name);
	exec frs end;

	exec frs activate menuitem 'Flush';
	exec frs begin;
		clear_qsf();
		displayDbProcs(user_name);
	exec frs end;

	exec frs activate menuitem 'Refresh';
	exec frs begin;
		displayDbProcs(user_name);
	exec frs end;

	exec frs activate menuitem 'End', frskey3;
	exec frs begin;
		exec frs set_frs frs (timeout = :timeVal);
	exec frs breakdisplay;
	exec frs end;

	return(TRUE);
}

processQsfQueries(server_name)
exec sql begin declare section;
char	*server_name;
exec sql end declare section;
{

	exec frs display ima_qsf_rqp read;
	exec frs initialize;
	exec frs begin;
		exec frs putform ima_qsf_rqp (server = :server_name);
		displayRqp();
	exec frs end;

	exec frs activate timeout;
	exec frs begin;
		displayRqp();
	exec frs end;

	exec frs activate menuitem 'Flush';
	exec frs begin;
		clear_qsf();
		displayRqp();
	exec frs end;


	exec frs activate menuitem 'Refresh';
	exec frs begin;
		displayRqp();
	exec frs end;

	exec frs activate menuitem 'End', frskey3;
	exec frs begin;
		exec frs set_frs frs (timeout = :timeVal);
	exec frs breakdisplay;
	exec frs end;

	return(TRUE);
}

displayDbProcs(user_name)
exec sql begin declare section;
char	*user_name;
exec sql end declare section;
{
	exec sql begin declare section;
	char	db_name[80];
	char	db_user_name[80];
	char	dbp_name[80];
	int	db_id;
	int	size;
	int	usage;
	int	allUsers = FALSE;
	int	anything = FALSE;
	exec sql end declare section;

	makeIidbdbConnection();
	if (strcmp(user_name,"ALL") == 0) allUsers = TRUE;

	exec frs inittable ima_qsf_dbp ima_qsf_dbp read;
	exec sql repeated select
		trim(shift(dbp_name,-20)),
		trim(dbp_owner),
		dbp_size,
		dbp_dbid,
		dbp_usage_count
	into
		:dbp_name,
		:db_user_name,
		:size,
		:db_id,
		:usage
	from
		ima_qsf_dbp
        order by dbp_usage_count desc;   /* 
##mdf170402 
*/
	exec sql begin;
		if ((allUsers) || (strcmp(db_user_name,user_name) == 0))
		{
			decode_db_id(db_name,db_id);
			exec frs loadtable ima_qsf_dbp ima_qsf_dbp(
				dbp_name = :dbp_name,
				dbp_owner = :db_user_name,
				dbp_size = :size,
				dbp_dbname = :db_name,
				dbp_usage_count = :usage);
		}
	exec sql end;
	sqlerr_check();
	exec frs redisplay;


}

displayRqp()
{
	exec sql begin declare section;
	char	db_name[80];
	char	db_user_name[80];
	char	rqp_name[80];
	int	db_id;
	int	size;
	int	usage;
	int	allUsers = FALSE;
	int	anything = FALSE;
	exec sql end declare section;

	makeIidbdbConnection();

	exec frs inittable ima_qsf_rqp ima_qsf_rqp read;
	exec sql repeated select
		trim(shift(rqp_name,-20)),
		rqp_size,
		rqp_dbid,
		rqp_usage_count
	into
		:rqp_name,
		:size,
		:db_id,
		:usage
	from
		ima_qsf_rqp
                order by rqp_usage_count desc;
	exec sql begin;
		decode_db_id(db_name,db_id);
		exec frs loadtable ima_qsf_rqp ima_qsf_rqp(
				rqp_name = :rqp_name,
				rqp_size = :size,
				rqp_dbname = :db_name,
				rqp_usage_count = :usage);
	exec sql end;
	sqlerr_check();
	exec frs redisplay;


}

decode_db_id(name,id)
exec sql begin declare section;
char	*name;
int	id;
exec sql end declare section;
{
	
	exec sql set_sql (session = :iidbdbSession);
		exec sql repeated select
			name
		into
			:name
		from 
			iidatabase
		where
			db_id = :id;
		if (sqlerr_check() == NO_ROWS) {
			sprintf(name,"(%ld)",id);
		}
		exec sql commit;
	exec sql set_sql (session = :imaSession);
}
