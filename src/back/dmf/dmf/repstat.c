/*
**Copyright (c) 2004 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <pc.h>
#include    <sl.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dm.h>
#include    <dm2rep.h>

/*
** Name: repstat.c
**
** Description:
**	report current status of various replication tasks within the DBMS,
**	currently the program will simply dump the contents of the replicator
**	shared memory transaction queue list.
**
** History:
**	17-dec-97 (stephenb)
**	    Initial creation
**	08-jan-98 (mcgem01)
**	    Include pc.h to silence compiler warning regarding PCexit.
**	4-dec-98 (stephenb)
**	    trans_time is now an HRSYSTIME
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-sep-2002 (devjo01)
**	    Call MEadvise only if not II_DMF_MERGE.
**	10-Dec-2008 (jonj)
**	    SIR 120874: Remove last vestiges of CL_SYS_ERR,
**	    old form uleFormat.
*/

/*
**	Ming hints
MODE =		SETUID PARTIAL_LD

NEEDLIBS =      SCFLIB DMFLIB ADFLIB ULFLIB GCFLIB COMPATLIB MALLOCLIB

OWNER =		INGUSER

PROGRAM =	repstat
*/

/*
** Forward and/or external refferences and globals
*/
GLOBALREF	DMC_REP         *Dmc_rep; /* replicator shared memory segment */

static STATUS repstat(
    i4		argc,
    char	*argv[]);
/*
** Allows NS&E to build one executable for IIDBMS, DMFACP, DMFRCP, ...
*/
# ifdef II_DMF_MERGE
# define    MAIN(argc, argv)    repstat_libmain(argc, argv)
# else
# define    MAIN(argc, argv)    main(argc, argv)
# endif


/*
** Name: main
**
** Description:
**	main program entry for repstat, this program reports the status
**	of replicator DBMS functions, currently it just dumps the contents
**	of the replicator transaction queue shared memory
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	success or failure through PCexit
**
** Side effects:
**	None
**
** History:
**	17-dec-97 (stephenb)
**	    Initial creation
**	25-Jul-07 (drivi01)
**	    Updated error message for insufficient privileges
**	    to make sence on Vista.
*/
# ifdef II_DMF_MERGE
VOID MAIN(argc, argv)
# else
VOID
main(argc, argv)
# endif
int     argc;
char    *argv[];
{
# ifndef	II_DMF_MERGE
    MEadvise(ME_INGRES_ALLOC);
# endif
    PCexit(repstat(argc, argv));
}

static STATUS 
repstat(
    i4		argc,
    char	*argv[])
{
    STATUS	status;
    CL_ERR_DESC	sys_err;
    SIZE_TYPE	pages_got;
    DMC_REPQ	*repq;
    i4		i;
    bool	last;
    char	commit_time[26];
    i4		entries;


    status = TRset_file(TR_T_OPEN, TR_OUTPUT, TR_L_OUTPUT,
                             &sys_err);
    if (status)
    {
        return (FAIL);
    }
    /*
    ** attach to memory
    */
    status = MEget_pages(ME_SSHARED_MASK, 0, DMC_REP_MEM, (PTR *)&Dmc_rep,
	&pages_got, &sys_err);
    if (status == ME_NO_SUCH_SEGMENT)
    {
	TRdisplay("%@ REPSTAT: The replicator shared memory segment has not yet been installed,\nplease make sure the config parameter 'rep_txq_size' is set to a positive\ninteger and that Ingres has been started\n");
	return(FAIL);
    }
    else if (status != OK || pages_got < 1)
    {
	TRdisplay("%@ REPSTAT: Error connecting to replicator transaction queue shared memeory\n");
	TRdisplay("Please make sure this program has the correct privileges,\nand is being run as ingres or in elevated mode\n");
	return(FAIL);
    }

    repq = (DMC_REPQ *)(Dmc_rep + 1);
    /*
    ** print stats
    */
    TRdisplay("============================================================\n");
    TRdisplay("     Replicator statistics at %@\n");
    TRdisplay("============================================================\n\n");
    TRdisplay(" Replicator transaction queue header\n");
    TRdisplay(" -----------------------------------\n");
    TRdisplay(" Mutex Address:				%x\n", 
	&Dmc_rep->rep_sem);
    TRdisplay(" Queue Size (in entries):		%d\n", 
	Dmc_rep->seg_size);
    TRdisplay(" Start of queue at entry No:		%d\n", 
	Dmc_rep->queue_start);
    TRdisplay(" End of queue at entry No:		%d\n", 
	Dmc_rep->queue_end);
    if (Dmc_rep->queue_end == Dmc_rep->queue_start &&
	repq[Dmc_rep->queue_start].active == 0 &&
	repq[Dmc_rep->queue_start].tx_id == 0)
	entries = 0;
    else if (Dmc_rep->queue_end == Dmc_rep->queue_start)
	entries = 1;
    else if (Dmc_rep->queue_end > Dmc_rep->queue_start)
	entries = Dmc_rep->queue_end - Dmc_rep->queue_start + 1;
    else
	entries = Dmc_rep->seg_size - Dmc_rep->queue_start + Dmc_rep->queue_end;
    TRdisplay(" No of entries currently in queue:	%d\n\n", entries);
    TRdisplay("============================================================\n");
    TRdisplay(" Current Queue Entries:\n");
    TRdisplay("============================================================\n");
    if (entries == 0)
    {
	TRdisplay (" NONE....\n");
    }
    else
    {
        for(i = Dmc_rep->queue_start, last = FALSE;;i++)
        {
	    if (last)
	        break;
	    if (i >= Dmc_rep->seg_size)
	        i = 0;
	    if (i == Dmc_rep->queue_end)
	        last = TRUE;
	    /*
	    ** display entry
	    */
    	    TRdisplay("------------------------------------------------------------\n");
	    TRdisplay(" Entry %d\n", i);
    	    TRdisplay("------------------------------------------------------------\n");
    	    TRdisplay(" Transaction ID:		%x\n", repq[i].tx_id);
	    status = TMcvtsecs(repq[i].trans_time.tv_sec, commit_time);
	    TRdisplay(" Commit Time:			%s\n", commit_time);
	    TRdisplay(" Database:			%s\n", 
	        repq[i].dbname.db_db_name);
	    TRdisplay(" Entry being processed ?	%s\n", 
	        repq[i].active ? "Yes" : "No");
        } /* end display loop */
    } /* end else */
    TRdisplay("\n");
    return(OK);
}
