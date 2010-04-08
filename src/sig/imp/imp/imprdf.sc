/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name:	imprdf.sc
**
** This file contains all of the routines needed to support the "RDF" 
** Subcomponent of the IMA based IMP lookalike tools.
**
## History:
##
##	07-Jul-95	(johna)
##		Created.
##
##	10-Apr-02	(mdf)
##		Expand domain to handle looking at a different RDF.
##
##      24-Apr-02       (mdf240402)
##              Commit after calling imp_complete_vnode
##
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

  struct rdf_ {
	char	server[65];
	long	state_num;
	char	state_string[61];
	long	cache_size;
	long	max_tables;
	long	max_qtrees;
	long	max_ldb_descs;
	long	max_defaults;
  } rdf;

extern 	int 	ima_rdf_cache_info;
extern 	int 	inFormsMode;
extern 	int 	imaSession;
extern 	int 	iidbdbSession;
extern 	int 	userSession;
extern 	char	*currentVnode;

exec sql end declare section;

/*
** Name:	imprdfsum.sc
**
** Display the RDF cache statistics summary
**
*/

exec sql begin declare section;
extern int 	timeVal;
exec sql end declare section;

int
processRdfSummary(server)
exec sql begin declare section;
GcnEntry	*server;
exec sql end declare section;
{
	exec sql begin declare section;
	char	server_name[SERVER_NAME_LENGTH];
	exec sql end declare section;
        exec sql execute procedure imp_complete_vnode;  /* 
##mdf100402 
*/
        exec sql commit;  /* 
##mdf240402 
*/

	exec sql set_sql (printtrace = 1);
	exec sql select dbmsinfo('IMA_VNODE') into :server_name;
	strcat(server_name,"::/@");
	strcat(server_name,server->serverAddress);

	exec frs display ima_rdf_cache_info read;
	exec frs initialize;
	exec frs begin;
		exec frs putform ima_rdf_cache_info (server = :server_name);
		getRdfSummary(server_name);
		displayRdfSummary();
		exec frs set_frs frs (timeout = :timeVal);

	exec frs end;

	exec frs activate timeout;
	exec frs begin;
		getRdfSummary(server_name);
		displayRdfSummary();
	exec frs end;

	exec frs activate menuitem 'Flush';
	exec frs begin;
		clear_rdf();
		getRdfSummary(server_name);
		displayRdfSummary();
	exec frs end;

	exec frs activate menuitem 'Refresh';
	exec frs begin;
		getRdfSummary(server_name);
		displayRdfSummary();
	exec frs end;

	exec frs activate menuitem 'End', frskey3;
	exec frs begin;
	exec frs set_frs frs (timeout = 0);
	exec sql set_sql (printtrace = 0);
	exec frs breakdisplay;
	exec frs end;

	return(TRUE);
}

getRdfSummary(server_name)
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
		state_string,
		cache_size,
		max_tables,
		max_qtrees,
		max_ldb_descs,
		max_defaults
	into
		:rdf.server,
		:rdf.state_string,
		:rdf.cache_size,
		:rdf.max_tables,
		:rdf.max_qtrees,
		:rdf.max_ldb_descs,
		:rdf.max_defaults
	from 
		ima_rdf_cache_info
	where
		server = :server_name;

	/*sqlerr_check();*/
    if (sqlerr_check() == NO_ROWS) {      
        fatal("getRdfSummary: no rows");  
    }                                     

}

clear_rdf()
{
	exec sql set trace point rd001;
}

displayRdfSummary()
{
	exec frs putform ima_rdf_cache_info (
		server = :rdf.server,
		state_string = :rdf.state_string,
		cache_size = :rdf.cache_size,
		max_tables = :rdf.max_tables,
		max_qtrees = :rdf.max_qtrees,
		max_ldb_descs = :rdf.max_ldb_descs,
		max_defaults = :rdf.max_defaults);
}
