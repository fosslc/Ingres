/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/
# include <compat.h>
# include <ci.h>
# include <cv.h>
# include <st.h>
# include <nm.h>
# include <lo.h>
# include <si.h>
# include <tr.h>
# include <ex.h>
# include <pc.h>
# include <me.h>
# include <er.h>
# include <gl.h>
# include <pm.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>
# include "distq.h"
# include "rsstats.h"

/**
** Name:	rsstatd.c - display Replicator Server statistics
**
** Description:
**	Defines
**		main	- main routine for displaying RepServer statistics
**		stats_display	- display statistics
**		stats_exit	- exit
**
** History:
**	17-feb-99 (abbjo03)
**		Created.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**	24-Aug-2009 (kschendel) 121804
**	    Need pm.h for PMhost.
**     16-Feb-2010 (hanje04)
**         SIR 123296
**         Add LSB option, writable files are stored under ADMIN, logs under
**         LOG and read-only under FILES location.
**/

/*
PROGRAM =	rsstatd

NEEDLIBS =	REPSERVLIB REPCOMNLIB RUNTIMELIB SAWLIB APILIB UGLIB \
		AFELIB CUFLIB GCFLIB ADFLIB COMPATLIB MALLOCLIB

UNDEFS =	II_copyright
*/

# define MAX_SERVER_NO		999
# define SEPARATOR \
    ERx("================================================================\n")


FUNC_EXTERN STATUS FEhandler(EX_ARGS *ex);
static STATUS stats_display(LOCATION *loc);
static void stats_exit(STATUS status);


/*{
** Name:	main - main routine for displaying Replicator Server statistics
**
** Description:
**	Displaying shared Replicator Server statistics.
**
** Inputs:
**	argc and argv from the command line
**
** Outputs:
**	none
**
** Returns:
**	none
**
** History:
**
**	10-Jul-2008 (kibro01) b120613
**	    Give the user a chance to know why they're not getting statistics
**	    by warning that shared_statistics hasn't been turned on.
*/
i4
main(
i4	argc,
char	*argv[])
{
	EX_CONTEXT	context;
	LOCATION	loc;
	LO_DIR_CONTEXT	lc;
	CL_ERR_DESC	err_desc;
	STATUS		status;
        char            *val;
        char            server_num[4];
	bool		shared_stats = FALSE;

	/* Tell EX this is an INGRES tool */
	(void)EXsetclient(EX_INGRES_TOOL);

	/* Use the INGRES memory allocator */
	(void)MEadvise(ME_INGRES_ALLOC);

	/* Initialize character set attribute table */
	if (IIUGinit() != OK)
		stats_exit(FAIL);

	/* Set up the generic exception handler */
	if (EXdeclare(FEhandler, &context) != OK)
		stats_exit(FAIL);

	if (TRset_file(TR_T_OPEN, TR_OUTPUT, TR_L_OUTPUT, &err_desc) != OK)
		stats_exit(FAIL);
	TRdisplay(SEPARATOR);
	TRdisplay("    Replicator Server Statistics at %@\n");
	TRdisplay(SEPARATOR);

	if (NMloc(ADMIN, PATH, NULL, &loc) != OK)
	{
		SIprintf("Error accessing II_CONFIG\n");
		stats_exit(FAIL);
	}

        PMsetDefault(1, PMhost());
        status = PMget(ERx("II.$.REPSERV.$.SHARED_STATISTICS"), &val);
        if (status == OK && STbcompare(val, 0, ERx("ON"), 0, TRUE) == 0)
                shared_stats = TRUE;

        if (!shared_stats)
        {
		TRdisplay("\n");
                TRdisplay("WARNING: ii.$.repserv.$.shared_statistics is not set to ON, so\n");
                TRdisplay("         statistics will not be available for this server.\n");
                TRdisplay("         To turn on statistics for all replication servers do:\n");
		TRdisplay("             iisetres 'ii.$.repserv.$.shared_statistics' ON\n");
                TRdisplay("         and then restart the replication server(s).\n");
        }

	LOfaddpath(&loc, ERx("memory"), &loc);
	if (LOwcard(&loc, RS_STATS_FILE, NULL, NULL, &lc) != OK)
	{
		TRdisplay("\nNo Replicator Servers found\n");
		stats_exit(FAIL);
	}
	stats_display(&loc);

	while ((status = LOwnext(&lc, &loc)) == OK)
	{
		TRdisplay(SEPARATOR);
		stats_display(&loc);
	}
	if (status != ENDFILE)
	{
		SIprintf("Error %d while searching memory files\n", status);
		stats_exit(FAIL);
	}
	LOwend(&lc);

	stats_exit(OK);
}


/*{
** Name:	stats_display	- display statistics
**
** Description:
**	Display the statistics for the given RepServer.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	OK or FAIL
**
**	26-Aug-2009 (kschendel) b121804
**	    Pages-alloc arg must be SIZE_TYPE or compiler complains.
*/
static STATUS
stats_display(
LOCATION	*loc)
{
	i4		i;
	i4		j;
	char		dev[MAX_LOC+1];
	char		path[MAX_LOC+1];
	char		fname[MAX_LOC+1];
	char		ext[MAX_LOC+1];
	char		vers[MAX_LOC+1];
	i4		server_no;
	char		sm_key[13];
	SIZE_TYPE	pages_alloc;
	STATUS		status;
	CL_ERR_DESC	err_desc;
	RS_MONITOR	*mon_stats;
	RS_TARGET_STATS	*targ;
	RS_TABLE_STATS	*tbl;
	char		tm[26];
	u_i4		targ_size;
	u_i4		tbl_size;

	LOdetail(loc, dev, path, fname, ext, vers);
	CVan(ext, &server_no);
	if (server_no < 1 || server_no > MAX_SERVER_NO)
		return (FAIL);

	STprintf(sm_key, ERx("%s.%s"), fname, ext);
	status = MEget_pages(ME_SSHARED_MASK & ~ME_CREATE_MASK, 0, sm_key,
		(PTR *)&mon_stats, &pages_alloc, &err_desc);
	if (status != OK)
	{
		i4	len;
		char	msg_buf[ER_MAX_LEN];
		CL_ERR_DESC	err_code;

		ERslookup(0, &err_desc, ER_TEXTONLY, NULL, msg_buf,
			sizeof(msg_buf), -1, &len, &err_code, 0,
			(ER_ARGUMENT *)NULL);
		SIprintf(ERx("\n%s\n"), msg_buf);
		SIprintf("Error mapping memory for server %d\n", server_no);
		return (status);
	}

	TRdisplay("\nServer %d\n    Connected to database %d - %s::%s\n",
		mon_stats->server_no, mon_stats->local_db_no,
		mon_stats->vnode_name, mon_stats->db_name);
	TMcvtsecs(mon_stats->startup_time, tm);
	TRdisplay("    Process ID %d, running since %s\n\n", mon_stats->pid,
		tm);

	targ_size = sizeof(RS_TARGET_STATS) + sizeof(RS_TARGET_STATS) %
		sizeof(ALIGN_RESTRICT);
	tbl_size = sizeof(RS_TABLE_STATS) + sizeof(RS_TABLE_STATS) %
		sizeof(ALIGN_RESTRICT);
	targ = (RS_TARGET_STATS *)((PTR)mon_stats + sizeof(RS_MONITOR) +
		sizeof(RS_MONITOR) % sizeof(ALIGN_RESTRICT));
	for (i = 0; i < mon_stats->num_targets;
		++i, targ = (RS_TARGET_STATS *)((PTR)targ + targ_size +
		tbl_size * mon_stats->num_tables))
	{
		TRdisplay("  Target database %d - %s::%s\n", targ->db_no,
			targ->vnode_name, targ->db_name);
		if (targ->first_txn_time)
		{
			TMcvtsecs(targ->first_txn_time, tm);
			TRdisplay("    First transaction at %s\n", tm);
			TMcvtsecs(targ->last_txn_time, tm);
			TRdisplay("    Last transaction  at %s\n", tm);
		}
		TRdisplay("    %d transactions completed\n\n", targ->num_txns);
		tbl = (RS_TABLE_STATS *)((PTR)targ + targ_size);
		for (j = 0; j < mon_stats->num_tables; ++j, ++tbl)
		{
			if (tbl->num_rows[0] || tbl->num_rows[1] ||
				tbl->num_rows[2])
			{
				TRdisplay("        Table %d: %s.%s\n",
					tbl->table_no, tbl->table_owner,
					tbl->table_name);
				TRdisplay("            Inserts:  %9d\n",
					tbl->num_rows[RS_INSERT-1]);
				TRdisplay("            Updates:  %9d\n",
					tbl->num_rows[RS_UPDATE-1]);
				TRdisplay("            Deletes:  %9d\n",
					tbl->num_rows[RS_DELETE-1]);
			}
		}
		TRdisplay(ERx("\n"));
	}

	TRdisplay(ERx("\n"));
	return (OK);
}


/*{
** Name:	stats_exit	- exit
**
** Description:
**	Disconnect and exit.
**
** Inputs:
**	status	- exit status
**
** Outputs:
**	none
**
** Returns:
**	OK or FAIL
*/
static void
stats_exit(
STATUS	status)
{
	EXdelete();
	PCexit(status);
}
