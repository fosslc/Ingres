/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <ci.h>
# include <st.h>
# include <lo.h>
# include <ex.h>
# include <pc.h>
# include <me.h>
# include <er.h>
# include <iiapi.h>
# include <sapiw.h>
# include "rsconst.h"
# include "conn.h"
# include "recover.h"
# include "repevent.h"
# include "cdds.h"
# include "rsstats.h"
# include "repserv.h"

/**
** Name:	replicat.c - Replicator Server main
**
** Description:
**	Defines
**		main	- main routine for Replicator Server
**
** History:
**	16-dec-96 (joea)
**		Created based on replicat.c in replicator library.
**	29-jan-97 (joea)
**		Add ming hints to create generic Replicator Server. Add
**		standard Ingres front-end initializations. Push *read_q() down
**		to RStransmit(). Check idle connections after RStransmit.
**		Initialize the replicated tables cache.
**      25-Jun-97 (fanra01)
**              Removed the NT_GENERIC section for redefining main.
**	05-sep-97 (joea)
**		Remove unused pass_no variable.
**	09-dec-97 (joea)
**		Add RUNTIMELIB to NEEDLIBS. Remove call to init_lkuptbl.
**	21-apr-98 (joea)
**		Convert to use OpenAPI instead of ESQL. Rename various
**		functions with standard names.
**	20-may-98 (joea)
**		Move RSkeys_check_unique and unquiet_all after the table cache
**		is initialized.
**	22-jul-98 (abbjo03)
**		Move GLOBALDEF of RSgo_again here so that it won't interfere
**		with the DistQ API.
**	04-nov-98 (abbjo03)
**		Initialize statistics reporting.
**	02-dec-98 (abbjo03)
**		Replace SIprintf with messageit. Add messageit for error
**		initializing statistics.
**	30-dec-98 (abbjo03)
**		Move RSgo_again to event.c.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	25-nov-2003 (gupsh01)
**	    Added support for IIAPI_VERSION_3.
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**	06-Apr-2005 (gupsh01)
**	    Added Bigint support. Upgraded to IIAPI_VERSION_4.
**      28-Apr-2009 (coomi01) b121984
**          Upgrade to IIAPI_VERSION_5 to allow ANSI Dates.
**      05-nov-2009 (joea)
**          Upgrade to IIAPI_VERSION_7 to support BOOLEAN.
**      30-aug-2010 (thich01) 
**          Upgrade to IIAPI_VERSION_8 to support spatial types.
**/

/*
PROGRAM =	repserv

NEEDLIBS =	REPSERVLIB REPCOMNLIB SAWLIB RUNTIMELIB APILIB UGLIB \
		AFELIB CUFLIB GCFLIB ADFLIB COMPATLIB MALLOCLIB

UNDEFS =	II_copyright
*/

GLOBALDEF	II_PTR	RSenv;

/*{
** Name:	main - main routine for Replicator Server
**
** Description:
**	Main routine for Replicator Server. Opens files, connections,
**	etc, then loops until shutdown getting events, reading queues,
**	and transmitting data.
**
** Inputs:
**	argc and argv from the command line
**
** Outputs:
**	none
**
** Returns:
**	none
*/
i4
main(
i4	argc,
char	*argv[])
{
	i4	go = 1;

	/* Tell EX this is an INGRES tool */
	(void)EXsetclient(EX_INGRES_TOOL);

	/* Use the INGRES memory allocator */
	(void)MEadvise(ME_INGRES_ALLOC);

	if (IIsw_initialize(&RSenv, IIAPI_VERSION_8) != IIAPI_ST_SUCCESS)
		PCexit(FAIL);

	/* Initialize character set attribute table */
	if (IIUGinit() != OK)
		PCexit(FAIL);

	RSerrlog_open();

	file_flags();
	com_flags(argc, argv);

	/* check if we got all required flags to run */
	if (!check_flags())
	{
		messageit(1, 93);
		RSshutdown(FAIL);
	}

	/* Replication Server Start */
	messageit(2, 43);

	check_pid();
	if (RSlocal_db_open() != OK)
		RSshutdown(FAIL);

	if (RSrdf_startup() != OK)
	{
		messageit(1, 107);
		RSshutdown(FAIL);
	}

	if (RSconns_init() != OK)
		RSshutdown(FAIL);

	if (RSstats_init() != OK)
	{
		messageit(1, 108);
		RSshutdown(FAIL);
	}

	if (RScdds_cache_init() != OK)
	{
		messageit(1, 1742);
		RSshutdown(FAIL);
	}

	if (!RSskip_check_rules && RSkeys_check_unique(&RSlocal_conn) != OK)
		RSshutdown(FAIL);

	unquiet_all(SERVER_QUIET);

	RStpc_recover();
	RSping(TRUE);

	if (RSdbevents_register() != OK)
		RSshutdown(FAIL);

	while (go)
	{
		if (RSopen_retry != CONN_NORETRY)
			RSconns_check_failed();

		go = RStransmit();
		if (RSdo_recover)
		{
			RStpc_recover();
			RSdo_recover = FALSE;
		}
		if (RSsingle_pass && !RSgo_again)
			break;

		/* check idle connections and close them */
		RSconns_check_idle();
		if ((go == ERR_WAIT_EVENT || go == 3) && !RSgo_again)
			go = RSdbevent_get();
	}

	RSshutdown(OK);
}
