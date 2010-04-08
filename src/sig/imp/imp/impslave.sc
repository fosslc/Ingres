/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name:	impslave.sc
**
** This file contains all of the routines needed to support the "SLAVE" 
** Subcomponent of the IMA based IMP lookalike tools.
**
## History:
##
##	08-Jul-95 (johna)
##		written
##      02-Jul-2002 (fanra01)
##          Sir 108159
##          Added to distribution in the sig directory.
##    08-Oct-2007 (hanje04)
##	SIR 114907
##	Remove declaration of timebuf as it's not referenced and causes
##	errors on Mac OSX
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "imp.h"

exec sql include sqlca;
exec sql include 'impcommon.sc';

exec sql begin declare section;

  EXEC SQL DECLARE ima_di_slave_info TABLE
	(server	varchar(64) not null,
	 di_slaveno	integer not null,
	 di_dio	integer not null,
	 cpu_tm_msecs	integer not null,
	 cpu_tm_secs	integer not null,
	 di_idrss	integer not null,
	 di_maxrss	integer not null,
	 di_majflt	integer not null,
	 di_minflt	integer not null,
	 di_msgrcv	integer not null,
	 di_msgsnd	integer not null,
	 di_msgtotal	integer not null,
	 di_nivcsw	integer not null,
	 di_nsignals	integer not null,
	 di_nvcsw	integer not null,
	 di_nswap	integer not null,
	 di_reads	integer not null,
	 di_writes	integer not null,
	 di_stime_tm_msecs	integer not null,
	 di_stime_tm_secs	integer not null,
	 di_utime_tm_msecs	integer not null,
	 di_utime_tm_secs	integer not null);

  struct slave_ {
	char	server[65];
	long	di_slaveno;
	long	di_dio;
	long	cpu_tm_msecs;
	long	cpu_tm_secs;
	long	di_idrss;
	long	di_maxrss;
	long	di_majflt;
	long	di_minflt;
	long	di_msgrcv;
	long	di_msgsnd;
	long	di_msgtotal;
	long	di_nivcsw;
	long	di_nsignals;
	long	di_nvcsw;
	long	di_nswap;
	long	di_reads;
	long	di_writes;
	long	di_stime_tm_msecs;
	long	di_stime_tm_secs;
	long	di_utime_tm_msecs;
	long	di_utime_tm_secs;
  } slave;


extern 	int 	ima_di_slave_info;
extern 	int 	inFormsMode;
extern 	int 	imaSession;
extern 	int 	iidbdbSession;
extern 	int 	userSession;
extern 	char	*currentVnode;

exec sql end declare section;

exec sql begin declare section;
extern int 	timeVal;
exec sql end declare section;

int
processSlaveInfo(server)
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

	exec frs display ima_di_slave_info read;
	exec frs initialize;
	exec frs begin;
		exec frs putform ima_di_slave_info (server = :server_name);
		refreshSlaves(server_name);
		displaySlaveSummary(server_name);
		exec frs set_frs frs (timeout = 0);
	exec frs end;

	exec frs activate menuitem 'Refresh';
	exec frs begin;
		refreshSlaves(server_name);
		displaySlaveSummary(server_name);
	exec frs end;

	exec frs activate menuitem 'End', frskey3;
	exec frs begin;
	exec frs set_frs frs (timeout = 0);
	exec sql set_sql (printtrace = 0);
	exec frs breakdisplay;
	exec frs end;

	return(TRUE);
}

displaySlaveSummary(server_name)
exec sql begin declare section;
char	*server_name;
exec sql end declare section;
{
	int i;
	int blks;
	exec sql begin declare section;
	char	 msgBuf[100];
	exec sql end declare section;

	exec frs inittable ima_di_slave_info ima_di_slave_info;
	exec sql repeated select 
		*
	into
		:slave
	from 
		ima_di_slave_info
	where
		server = :server_name;

	exec sql begin;
		exec frs insertrow ima_di_slave_info ima_di_slave_info (
			di_slaveno = :slave.di_slaveno,
			di_dio = :slave.di_dio,
			di_reads = :slave.di_reads,
			di_writes = :slave.di_writes,
			di_stime_tm_secs = :slave.di_stime_tm_secs,
			di_stime_tm_msecs = :slave.di_stime_tm_msecs,
			di_utime_tm_secs = :slave.di_utime_tm_secs,
			di_utime_tm_msecs = :slave.di_utime_tm_msecs,
			cpu_tm_secs = :slave.cpu_tm_secs,
			cpu_tm_msecs = :slave.cpu_tm_msecs);
	exec sql end;
	sqlerr_check();
}


refreshSlaves(server_name)
exec sql begin declare section;
char	*server_name;
exec sql end declare section;
{
	exec sql execute procedure 
		ima_collect_slave_stats(server = :server_name);
}
