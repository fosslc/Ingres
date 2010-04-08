/*
** Copyright (c) 1992, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <ep.h>
#include    <er.h>
#include    <lo.h>
#include    <me.h>
#include    <nm.h>
#include    <pc.h>
#include    <si.h>
#include    <st.h>
#include    <tr.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <cx.h>
#include    <pm.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <scf.h>
#include    <dm.h>
#include    <lgdef.h>
#include    <lkdef.h>
#include    <lgkdef.h>
#include    <lgdstat.h>
#include    <lgclustr.h>
#include    <dmpp.h>
#include    <dmp.h>
#include    <dm0c.h>
#include    <dm1b.h>
#include    <dm0llctx.h>
#include    <dm0l.h>
#include    <dm0m.h>
#include    <dmve.h>
#include    <dm2d.h>
#include    <dmtcb.h>
#include    <dm2t.h>
#include    <dmfrcp.h>

/**
**
**  Name: RCPSTAT.C	- Utility tool for checking LG/LK/DMF status.
**
**  Description:
**
**	This tool is designed for use with shell scripts and other utilities
**	which wish to examine the state of the backend. It can be used to ask
**	a YES/NO question -- the answer is returned by either exiting FAIL or
**	OK. It supports the following enquiries:
**
**  Invocation		PCexit(FAIL)		PCexit(OK)
**   -------------	----------------	-------------
**   -online 		No, not online		Yes, online
**   -transactions      No, logfile is empty	Yes, logfile contains some
**						transactions.
**   -exist		Logfile doesn't exist	Log File exists
**   -format		No, logfile unreadable	Yes, logfile is
**   -enable		Logfile is not enabled	Log is enabled
**   -sizeok		Logs are diff. sizes	Logs are the
**   -csp_online	DMFCSP is not online.	DMFCSP process is online.
**   -any_csp_online	No DMFCSP is running.   At least one DMFCSP is running.
**   -connected         No connected process.	At leaset one connected process.
**
**   Optional flags
**   --------------
**   -silent	    - Do not display the information messages.
**   -dual	    - Perform the operation on the DUAL log, allowed on
**		      "-exist|format|enable".
**   -node=nodename - Only valid in Ingres Cluster installations, causes
**		      the operation requested to be performed against the
**		      specified node. If not specfied the default is the
**		      host node.
**   -rad=radid	    - Only valid on NUMA clusters.  Causes the operation
**		      to be performed against the local node which
**		      matches the specified RAD.
**
**  Undocumented, internal use only:
**  --------------------------------
**	-kill_all	Vaporize an entire installation using PCforce_exit
**	-gcmt_free	Loosen a stuck group commit buffer.
**		
**  History:
**	Fall, 1992 (bryanp)
**	    Created for the new Logging system.
**	10-nov-1992 (rogerk)
**	    Changed a few DI calls to pass address of syserr correctly.
**      14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	18-jan-1993 (bryanp)
**	    Tune up MING hints for better Unix builds.
**	    Removed -rconfdual flag since the rcpconfig.dat file is no more.
**	22-jan-1993 (bryanp)
**	    rcpstat is now an II_DMF_MERGE executable.
**	17-mar-1993 (rogerk)
**	    Moved lgd status values to lgdstat.h so that they are visible
**	    by callers of LGshow(LG_S_LGSTS).
**	26-apr-1993 (bryanp)
**	    Make an scf_call before anything else to properly initialize CS/SCF.
**	    Added a "-kill_all" argument which forcibly terminates all processes
**		on this node which have called LGopen. This is basically useful
**		only for testing, where it is useful in simulating node crashes.
**	15-may-1993 (rmuth)
**	    Various changes to support the VAX/VMS cluster.
**	    a. Add the new deluxe command line parser.
**	    b. Add the following parameters "-silent", "-node nodename",
**	       "-csp_online" and "-any_csp_online"
**	    c. Large code restructuring to support the above.
**	21-June-1993 (rmuth)
**	    Correct "-transactions" message.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Added back an accidentally-dropped "if" statement.
**	    Changed node-name handling in ule_initiate call: formerly we were
**		passing a fixed-length node name, which might be null-padded
**		or blank-padded at the end. This could lead to garbage in the
**		node name portion of the error log lines, or, when we use node
**		names to generate trace log filenames, to garbage file names.
**		Now we use a null-terminated node name, from LGCn_name().
**	23-aug-1993 (bryanp)
**	    Added internal-use-only, undocumented, "-gcmt_free" operation
**		for releasing stuck installations due to group commit bugs.
**	20-sep-1993 (jnash)
**	    Add -help.
**	18-oct-1993 (rmuth)
**	    - CL prototype, add <lgclustr.h>.
**	    - Silence some messages.
**	31-jan-1994 (bryanp) B56473
**	    Return FAIL if online and dual log not enabled.
**	31-jan-1994 (jnash)
**	    CSinitiate() now returns errors, so dump them.
**	25-apr-1994 (rmuth)
**	    b56703 The "-node nodename" option should check that the nodename
**	           is a valid cluster member.
**      20-may-95 (hanch04)
**          Added include dmtcb.h for ndmfapi
**	05-Jan-1996 (jenjo02)
**	    Mutex granularity project. Semaphores must now be be explicitly
**	    named in calls to LKMUTEX functions.
**	26-Jan-1998 (jenjo02)
**	    Partitioned Log File Project:
**	    Added support for multiple log files.
**	24-Aug-1999 (jenjo02)
**	    Dropped lgd_timer_lbb, use lfb_current_lbb instead.
**	    lfb_current_lbb_mutex replaced by individual LBB's lbb_mutex.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      28-Jun-2001 (hweho01)
**          Added the missing argument num_parms to ule_format() call
**          in _rcpstat() routine.
**	17-jul-2003 (penga03)
**	    Added include adf.h.
**	02-jul-2001 (devjo01)
**	    Supress report of E_DMA47C errors when checking another
**	    node.  These are expected, and should not be displayed.
**	18-sep-2002 (devjo01)
**	    Allow for "-rad" parameter in support of NUMA clusters.
**	17-jul-2003 (penga03)
**	    Added include adf.h.
**      11-Jun-2004 (horda03) Bug 112382/INGSRV2838
**          Ensure that the current LBB is stil "CURRENT" after
**          the lbb_mutex is acquired. If not, re-acquire.
**	22-Jun-2007 (drivi01)
**	    On Vista, this binary should only run in elevated mode.
**	    If user attempts to launch this binary without elevated
**	    privileges, this change will ensure they get an error
**	    explaining the elevation requriement.
**	07-jan-2008 (joea)
**	    Discontinue use of CXnode_name_from_nickname.
**      28-jan-2008 (stial01)
**          Redo/undo error handling during offline recovery via rcpconfig.
**      31-jan-2008 (stial01)
**          Added -rcp_stop_lsn, to set II_DMFRCP_STOP_ON_INCONS_DB
**	09-Dec-2008 (jonj)
**	    SIR 120874: use new form uleFormat, CL_CLEAR_ERR.
**      05-may-2009
**          Fixes for II_DMFRCP_PROMPT_ON_INCONS_DB / Db_incons_prompt
**      23-nov-2009 (horda03) B122555
**          Added -connected. Indicate processes connected to
**          LG/LK shared memory.
**/

/*
** Allows NS&E to build one executable for IIDBMS, DMFACP, DMFRCP, ...
*/
# ifdef II_DMF_MERGE
# define    MAIN(argc, argv)    rcpstat_libmain(argc, argv)
# else
# define    MAIN(argc, argv)    main(argc, argv)
# endif

/*
**      mkming hints
MODE =          SETUID PARTIAL_LD

NEEDLIBS =      SCFLIB DMFLIB ULFLIB COMPATLIB

OWNER =         INGUSER

PROGRAM =       ntrcpst
*/

static STATUS	_rcpstat(i4 argc,char *argv[]);
static STATUS	is_online(void);
static STATUS	has_transactions(void);
static STATUS	primary_exists(void);
static STATUS	dual_exists(void);
static STATUS	primary_readable(void);
static STATUS	dual_readable(void);
static STATUS	primary_enabled(void);
static STATUS	dual_enabled(void);
static STATUS	logsizes_match(void);
static STATUS	rconf_dual_logging(void);
static STATUS	kill_all_processes(void);
static STATUS	free_stuck_gcmt_buffer(void);
static STATUS   connected(void);

static VOID 	rcpstat_parse_command_line(
			    i4		argc,
			    char		*argv[] );

static VOID 	rcpstat_display_message(
			    STATUS		status );

static STATUS	csp_online( char *nodename );

static STATUS	any_csp_online(void);
static STATUS   is_recover(void);


#define		RCPSTAT_NOTHING_SELECTED  	0x00000000L
#define		RCPSTAT_ONLINE       	  	0x00000001L
#define		RCPSTAT_TRANSACT     	  	0x00000002L
#define		RCPSTAT_EXIST	     		0x00000004L
#define		RCPSTAT_FORMAT	     		0x00000008L 
#define		RCPSTAT_ENABLE	     		0x00000010L
#define		RCPSTAT_SIZEOK	     		0x00000020L
#define		RCPSTAT_KILLALL	     		0x00000040L
#define		RCPSTAT_CSP_ONLINE		0x00000080L
#define 	RCPSTAT_ANY_CSP_ONLINE		0x00000100L
#define		RCPSTAT_GCMT_FREE		0x00000200L
#define		RCPSTAT_HELP			0x00000400L
#define         RCPSTAT_RECOVER			0x00000800L
#define         RCPSTAT_CONNECTED_PROCESS	0x00001000L

static u_i4	rcpstat_action = RCPSTAT_NOTHING_SELECTED;

#define		NODE_BUF_SIZE		(CX_MAX_NODE_NAME_LEN+1)
static char	g_host_nodename[ NODE_BUF_SIZE ];
static char	g_req_nodename[ NODE_BUF_SIZE ];
static char    *g_log_nodename;

/*
** This is either Primary or Dual
*/
#define		MAX_LOG_FILE_TYPE 10
static char	g_log_file_type[ MAX_LOG_FILE_TYPE ];

static bool	g_action_on_dual_log 	   = FALSE;
static bool	g_silent		   = FALSE;
static bool	g_cluster		   = FALSE;
static bool	g_another_node		   = FALSE;

static i4	g_conn_procs		= 0;
static PID	g_conn_pids [LGK_MAX_PIDS];

/*
** Usage message
*/
static char *rcpstat_usage_msg[] = {
"rcpstat: invalid arguments. Acceptable arguments are:\n",
"     rcpstat%s -online               [-silent]%s\n",
"     rcpstat%s -transactions         [-silent]%s\n",
"     rcpstat%s -exist        [-dual] [-silent]%s\n",
"     rcpstat%s -format       [-dual] [-silent]%s\n",
"     rcpstat%s -enable       [-dual] [-silent]%s\n",
"     rcpstat%s -sizeok               [-silent]%s\n",
"     rcpstat%s -csp_online           [-silent]%s\n",
"     rcpstat%s -any_csp_online       [-silent]\n",
"     rcpstat%s -connected            [-silent]\n",
"     rcpstat%s -recover              [-silent]%s\n",
0	/* trailing 0 marks end of messages */
};
/*
** Warning message
*/
static char *rcpstat_warning_msg[] = {
"Warning this operation requires that Ingres is not running on\n",
"the requested node. The log file will be read but may fail due to\n",
"concurrent updates on the requested node\n",
0
};

/*
**	26-apr-1993 (bryanp)
**	    Make an scf_call before anything else to properly initialize CS/SCF.
**	26-jul-1993 (bryanp)
**	    Changed node-name handling in ule_initiate call: formerly we were
**		passing a fixed-length node name, which might be null-padded
**		or blank-padded at the end. This could lead to garbage in the
**		node name portion of the error log lines, or, when we use node
**		names to generate trace log filenames, to garbage file names.
**		Now we use a null-terminated node name, from LGCn_name().
**	20-sep-1993 (jnash)
**	    Add -help.
**	31-jan-1994 (jnash)
**	    CSinitiate() now returns errors (called within 
**	    scf_call(SCF_MALLOC)) so dump them.
**	15-May-2007 (toumi01)
**	    For supportability add process info to shared memory.
*/
VOID
# ifdef	II_DMF_MERGE
MAIN(argc, argv)
# else
main(argc, argv)
# endif
i4                  argc;
char		    *argv[];
{
# ifndef	II_DMF_MERGE
    MEadvise(ME_INGRES_ALLOC);
# endif
    PCexit(_rcpstat(argc, argv));
}

static VOID
param_error( i4 msg_index )
{
    char *radarg, *nodearg;

    if ( CXnuma_cluster_configured() )
	radarg = " -rad radid";
    else
	radarg = "";
    if ( CXcluster_enabled() )
	nodearg = " [-node nodename]";
    else
	nodearg = "";

    while ( rcpstat_usage_msg[msg_index] != NULL ) 
    {
	TRdisplay(rcpstat_usage_msg[msg_index], radarg, nodearg);
	msg_index++;
    }
}

static STATUS
_rcpstat(
i4                  argc,
char		    *argv[])
{
    CL_ERR_DESC	    sys_err;
    i4	    status;
    i4	    err_code;
    i4		    msg_index;
    SCF_CB	    scf_cb;
    char	    error_buffer[ER_MAX_LEN];
    i4 	    error_length;
    i4 	    count;
    char	*local_host;

    /*
    ** Set interactive input/output file.
    */
    status = TRset_file(TR_T_OPEN, TR_OUTPUT, TR_L_OUTPUT,
			     &sys_err);

    /* 
    ** Check if this is Vista, and if user has sufficient privileges 
    ** to execute.
    */
    ELEVATION_REQUIRED();

    if ( CXcluster_enabled() )
    {
	g_cluster = TRUE;

	CXnode_name(g_host_nodename);
	STcopy(g_host_nodename,g_req_nodename);
	g_log_nodename = g_req_nodename;
	ule_initiate( g_req_nodename, STlength(g_req_nodename), "RCPSTAT", 
			sizeof("RCPSTAT")-1);
    }    
    else
    {
	/*
	** Setup default nodename of "Current"
	*/
	STcopy( "Current", g_host_nodename);
	STcopy( "Current", g_req_nodename);
	g_log_nodename = NULL;

	ule_initiate(0, 0, "RCPSTAT", sizeof("RCPSTAT")-1);
    }

    if (status != OK)
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, 
			&err_code, 0);
	return (FAIL);
    }

    if ( argc < 2 )
    {
	/* quick exit if no params set */
	param_error(0);
	return(FAIL);
    }

    /*
    ** In addition to a simple CSinitiate() call, however, there are a number
    ** of other various CS setup tasks which must be performed. The simplest
    ** way to perform these tasks is to make an scf_call(), which performs all
    ** the various initialization (CSinitiate, CSset_sid, etc.). We don't
    ** actually care about the memory we're allocating, we just want the
    ** side-effects of calling scf_call. If we skip the scf_call, then
    ** CSp_semaphore doesn't work correctly because it can't locate the current
    ** "session".
    */

    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_length = sizeof(SCF_CB);
    scf_cb.scf_session = DB_NOSESSION;
    scf_cb.scf_facility = DB_DMF_ID;
    scf_cb.scf_scm.scm_functions = 0;
    scf_cb.scf_scm.scm_in_pages = ((42 + SCU_MPAGESIZE - 1)
	& ~(SCU_MPAGESIZE - 1)) / SCU_MPAGESIZE;

    status = scf_call(SCU_MALLOC, &scf_cb);

    if (status != OK)
    {
	uleFormat(NULL, status, (CL_ERR_DESC *) NULL, ULE_LOOKUP, 
	    (DB_SQLSTATE *) NULL, error_buffer, ER_MAX_LEN, &error_length, 
	    &err_code, 0);
	error_buffer[error_length] = EOS;
        SIwrite(error_length, error_buffer, &count, stdout);
        SIwrite(1, "\n", &count, stdout);
        SIflush(stdout);
	return (FAIL);
    }

    if ((status = LGinitialize(&sys_err, ERx("rcpstat"))) != OK)
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, 
			&err_code, 0);
	return (FAIL);
    }

    /*
    ** Parse the command line
    */
    rcpstat_parse_command_line( argc, argv );

    if (rcpstat_action == RCPSTAT_HELP)
    {
	TRdisplay("usage:\n");
	param_error(1);
	return (FAIL);
    }

    /*
    ** Setup string to indicat eof log file is Primary or dual
    */
    if ( g_action_on_dual_log )
	STcopy( "Dual", g_log_file_type );
    else
	STcopy( "Primary",  g_log_file_type );


    if (rcpstat_action == RCPSTAT_NOTHING_SELECTED)
    {
	param_error(0);
	return (FAIL);
    }

    do
    {
	if ( rcpstat_action & RCPSTAT_ONLINE )
	{
	    status = is_online();
	    break;
	}

	if ( rcpstat_action & RCPSTAT_TRANSACT )
	{
	    status = has_transactions();
	    break;
	}

	if ( rcpstat_action & RCPSTAT_EXIST )
	{
	    if ( g_action_on_dual_log )
	    {
	        status = dual_exists();
	    }
	    else
	    {
		status = primary_exists();
	    }
	    break;
	}

	if ( rcpstat_action & RCPSTAT_FORMAT )
	{
	    if ( g_action_on_dual_log )
	    {
	        status = dual_readable();
	    }
	    else
	    {
		status = primary_readable();
	    }
	    break;
	}

	if ( rcpstat_action & RCPSTAT_ENABLE )
	{
	    if ( g_action_on_dual_log )
	    {
	        status = dual_enabled();
	    }
	    else
	    {
		status = primary_enabled();
	    }

	    break;
	}

	if ( rcpstat_action & RCPSTAT_SIZEOK )
	{
	    status = logsizes_match();
	    break;
	}

	if ( rcpstat_action & RCPSTAT_KILLALL )
	{
	    status = kill_all_processes();
	    break;
	}

	if ( rcpstat_action & RCPSTAT_CSP_ONLINE )
	{
	    status = csp_online( g_req_nodename );
	    break;
	}

	if ( rcpstat_action & RCPSTAT_ANY_CSP_ONLINE )
	{
	    status = any_csp_online();
	    break;
	}

	if ( rcpstat_action & RCPSTAT_GCMT_FREE )
	{
	    status = free_stuck_gcmt_buffer();
	    break;
	}

	if ( rcpstat_action & RCPSTAT_RECOVER )
	{
	    status = is_recover();
	    break;
	}

	if ( rcpstat_action & RCPSTAT_CONNECTED_PROCESS )
	{
	    status = connected();
	    break;
	}

    } while (FALSE);

    return (status);
}

static STATUS
is_online(void)
{
    STATUS	status;
    CL_ERR_DESC	sys_err;
    i4	lgd_status;
    i4	length;
    i4	err_code;

    do
    {
	if (g_another_node)
	{
	    /*
	    ** If csp is online we will assume that the log is online
	    */
	    status = LGc_csp_online( g_req_nodename );
	    if ( status == OK )
		break;
	}
	else
	{
	    status = LGshow( LG_S_LGSTS, (PTR)&lgd_status, sizeof(lgd_status), 
			     &length, &sys_err);
	    if (status != OK )
	    {
		uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
				(char *)NULL, 0L, (i4 *)NULL, 
				&err_code, 0);
		break;
	    }

	    if (lgd_status & LGD_ONLINE)
	    {
		status = OK;
		break;
	    }
	    else
	    {
		status = FAIL;
		break;
	    }
	}

    } while (FALSE);

    if (!g_silent)
	rcpstat_display_message( status );

    if ( status == OK )
	return( OK );
    else
	return( FAIL );
}

static STATUS
has_transactions(void)
{
    STATUS	status;
    CL_ERR_DESC	sys_err;
    i4	lgd_status;
    i4	lgh_status;
    i4	lgd_active_log;
    i4	lgh_active_logs;
    i4	length;
    i4	err_code;
    DI_IO	logfile;
    i4		one_page;
    char	buffer[2048];
    char	file[LG_MAX_FILE][128];
    char	path[LG_MAX_FILE][128];
    i4	l_file[LG_MAX_FILE];
    i4	l_path[LG_MAX_FILE];
    i4		count_files;

    /*
    ** if system is online, report OK
    ** if system is NOT online, open one or both log files, read header.
    ** If header
    ** is LGH_RECOVER or LGH_VALID or LGH_EOF_OK, report OK.
    ** otherwise, report FAIL.
    */

    do
    {
	/*
	** LGshow currently does not run across nodes
	*/
	if (g_another_node)
	{
	    /*
	    ** If csp is online we will assume that the log is online
	    */
	    status = LGc_csp_online( g_req_nodename );
	    if ( status == OK )
		break;
	}
	else
	{
	    status = LGshow( LG_S_LGSTS, (PTR)&lgd_status, sizeof(lgd_status), 
			     &length, &sys_err);
	    if ( status != OK )
	    {
		uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
				(char *)NULL, 0L, (i4 *)NULL, 
				&err_code, 0);
		break;
	    }

	    if (lgd_status & LGD_ONLINE)
	    {
		status = OK;
		break;
	    }
	}

	/*
	** Logging system is offline. Open primary log and read its header. 
	** If it checksums correctly and if it is an active log, and if 
	** its header is LGH_RECOVER or LGH_VALID or LGH_EOF_OK, report OK, 
	** else report FAIL.
	** If the primary log file can't be opened or is not marked active, then
	** try the dual log.
	*/

	status = LG_build_file_name( LG_PRIMARY_LOG, g_log_nodename, file,
					    l_file, path, l_path,
					    &count_files );
	if (status)
	    break;

	if (!g_silent)
	    TRdisplay( "Opening Primary Log file: %t in path: %t\n",
		        l_file[0], file[0], l_path[0], path[0] );

	status = DIopen(&logfile, path[0], l_path[0],
				file[0], l_file[0],
				2048, DI_IO_WRITE,
				(u_i4)0, &sys_err);
	if (status == OK)
	{
	    one_page = 1;
	    status = DIread(&logfile, &one_page,
				(i4)0,
				buffer, &sys_err);
	    if (status == OK)
		status = DIclose(&logfile, &sys_err);

	    if (status == OK)
	    {
		if (LGchk_sum(buffer, sizeof(LG_HEADER)) ==
				    ((LG_HEADER *)buffer)->lgh_checksum)
		{
		    lgh_active_logs = ((LG_HEADER *)buffer)->lgh_active_logs;
		    lgh_status      = ((LG_HEADER *)buffer)->lgh_status;

		    if (lgh_active_logs & LGH_PRIMARY_LOG_ACTIVE)
		    {
			if (lgh_status == LGH_RECOVER || 
			    lgh_status == LGH_VALID ||
			    lgh_status == LGH_EOF_OK)
			{
			    status = OK;
			    break;
			}
			else
			{
			    status = FAIL;
			    break;
			}
		    }
		}
	    }
	}

	/*
	** Primary log couldn't be opened or couldn't be read, or 
	** didn't checksum,  or wasn't marked active. Try the dual log.
	*/
	status = LG_build_file_name( LG_DUAL_LOG, g_log_nodename, file,
					    l_file, path, l_path,
					    &count_files );
	if (status)
	    break;

	if (!g_silent)
	    TRdisplay( "Opening Dual Log file: %t in path: %t\n",
		        l_file[0], file[0], l_path[0], path[0] );

	status = DIopen(&logfile, path[0], l_path[0],
				file[0], l_file[0],
				2048, DI_IO_WRITE,
				(u_i4)0, &sys_err);
	if (status == OK)
	{
	    one_page = 1;
	    status = DIread(&logfile, &one_page,
				(i4)0,
				buffer, &sys_err);
	    if (status == OK)
		status = DIclose(&logfile, &sys_err);
	    if (status == OK)
	    {
		if (LGchk_sum(buffer, sizeof(LG_HEADER)) ==
				    ((LG_HEADER *)buffer)->lgh_checksum)
		{
		    lgh_active_logs = ((LG_HEADER *)buffer)->lgh_active_logs;
		    lgh_status      = ((LG_HEADER *)buffer)->lgh_status;
		    if (lgh_active_logs & LGH_DUAL_LOG_ACTIVE)
		    {
			if (lgh_status == LGH_RECOVER || 
			    lgh_status == LGH_VALID ||
			    lgh_status == LGH_EOF_OK)
			{
			    status = OK;
			    break;
			}
			else
			{
			    status = FAIL;
			    break;
			}
		    }
		}
	    }
	}

	/*
	** Dual log couldn't be opened or couldn't be read, or didn't checksum,
	** or wasn't marked active, EITHER! Gack. Since we can't figure out the
	** state of the world, we won't be able to find any transactions to
	** process, so go ahead and return FAIL.
	*/
	status = FAIL;

    } while (FALSE);

    if (!g_silent)
	rcpstat_display_message(status);
	
    if ( status == OK )
	return( OK );
    else
	return( FAIL );

}

static STATUS
primary_exists(void)
{
    char	file[LG_MAX_FILE][128];
    char	path[LG_MAX_FILE][128];
    i4	l_file[LG_MAX_FILE];
    i4	l_path[LG_MAX_FILE];
    i4		count_files;
    i4		i;
    STATUS	status;
    DI_IO	di_io;
    CL_ERR_DESC	sys_err;
    i4	err_code;

    /*
    ** Try to open primary logfile and report on results.
    */
    status = LG_build_file_name( LG_PRIMARY_LOG, g_log_nodename, file,
					l_file, path, l_path,
					&count_files );

    /* Loop around all of the logfile partitions */
    for (i = 0;
	 i < count_files && status == OK;
	 i++)
    {
	if (!g_silent)
	    TRdisplay( "Checking existience of %s Log file: %t in path: %t\n",
		       g_log_file_type, l_file[i], file[i], l_path[i], path[i] );


	status = DIopen(&di_io, path[i], l_path[i], file[i], l_file[i],
			    2048, DI_IO_WRITE,
			    (u_i4)0, &sys_err);

	/*
	** If cannot open file do not send an error message if silent
	** as checking existence
	*/
	if (status)
	{
	    if (!g_silent)
		uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			    (char *)NULL, 0L, (i4 *)NULL, 
			    &err_code, 0);
	}
	else
	{
	    status = DIclose(&di_io, &sys_err);
	    if (status)
	    {
		uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
				(char *)NULL, 0L, (i4 *)NULL, 
				&err_code, 0);
	    }
	}
    }

    if (!g_silent)
	rcpstat_display_message(status);
	
    if ( status == OK )
	return( OK );
    else
	return( FAIL );
}

static STATUS
dual_exists(void)
{
    /*
    ** Try to open dual logfile and report on results.
    */
    char	file[LG_MAX_FILE][128];
    char	path[LG_MAX_FILE][128];
    i4	l_file[LG_MAX_FILE];
    i4	l_path[LG_MAX_FILE];
    i4		count_files;
    i4		i;
    STATUS	status;
    DI_IO	di_io;
    CL_ERR_DESC	sys_err;
    i4	err_code;

    /*
    ** Try to open dual logfile and report on results.
    */
    status = LG_build_file_name( LG_DUAL_LOG, g_log_nodename, file,
					l_file, path, l_path,
					&count_files );

    /* Loop around all of the logfile partitions */
    for (i = 0;
	 i < count_files && status == OK;
	 i++)
    {
	if (!g_silent)
	    TRdisplay( "Checking existience of %s Log file: %t in path: %t\n",
		       g_log_file_type, l_file[i], file[i], l_path[i], path[i] );


	status = DIopen(&di_io, path[i], l_path[i], file[i], l_file[i],
			    2048, DI_IO_WRITE,
			    (u_i4)0, &sys_err);

	/*
	** If cannot open file do not send an error message if silent
	** as checking existence
	*/
	if (status)
	{
	    if (!g_silent)
		uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			    (char *)NULL, 0L, (i4 *)NULL, 
			    &err_code, 0);
	}
	else
	{
	    status = DIclose(&di_io, &sys_err);
	    if (status)
	    {
		uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
				(char *)NULL, 0L, (i4 *)NULL, 
				&err_code, 0);
	    }
	}
    }

    if (!g_silent)
	rcpstat_display_message(status);
	
    if ( status == OK )
	return( OK );
    else
	return( FAIL );
}

static STATUS
primary_readable(void)
{
    STATUS	status;
    CL_ERR_DESC	sys_err;
    i4	lgd_status;
    i4	lgd_active_log;
    i4	length;
    i4	err_code;
    DI_IO	logfile;
    i4		one_page;
    char	buffer[2048];
    char	file[LG_MAX_FILE][128];
    char	path[LG_MAX_FILE][128];
    i4	l_file[LG_MAX_FILE];
    i4	l_path[LG_MAX_FILE];
    i4		count_files;

    /*
    ** If system is online and active logs include primary, report OK
    ** If system is online and active logs DON'T include primary, FAIL
    ** If system is offline, open primary and read log header. If header
    ** checksums correctly, report OK, else report FAIL
    */

    do
    {
	/*
	** LGshow does not currently work across nodes
	*/
	if (g_another_node)
	{
	    /*
	    ** The csp cannot tell us this information. If we find the
	    ** csp is running issue a warning message that we may not
	    ** be able to read the log file in a consistent manner and
	    ** hence may return an invalid result.
	    */
	    status = LGc_csp_online( g_req_nodename );
	    if (( status == OK ) && (!g_silent) )
	    {
		i4	msg_index;

		for (msg_index=0; rcpstat_warning_msg[msg_index] != 0; 
		     msg_index++)
		{
		    TRdisplay(rcpstat_warning_msg[msg_index]);
		}
	    }
	}
	else
	{
	    status = LGshow( LG_S_LGSTS, (PTR)&lgd_status, sizeof(lgd_status), 
			     &length, &sys_err);
	    if ( status != OK )
	    {
		uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
				(char *)NULL, 0L, (i4 *)NULL, 
				&err_code, 0);
		break;
	    }

	    if (lgd_status & LGD_ONLINE)
	    {
		status = LGshow( LG_S_DUAL_LOGGING, (PTR)&lgd_active_log, 
				 sizeof(lgd_active_log), &length, &sys_err);
		if ( status != OK )
		{
		    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
				(char *)NULL, 0L, (i4 *)NULL, 
				&err_code, 0);
		    break;
		}

		if (lgd_active_log & LG_PRIMARY_ACTIVE)
		{
		    status = OK;
		    break;
		}
		else
		{
		    status = FAIL;
		    break;
		}
	    }
	}

	/*
	** Logging system is offline. First check that all primary
	** logfile partitions exist by calling primary_exists(), then
	** open primary log and read its header. If it checksums,
	** then the log is readable.
	*/
	status = primary_exists();
	if (status == OK)
	{
	    status = LG_build_file_name( LG_PRIMARY_LOG, g_log_nodename, file,
					    l_file, path, l_path,
					    &count_files );
	    if (status == OK)
	    {
		if (!g_silent)
		    TRdisplay( "Opening %s Log file: %t in path: %t\n",
		        g_log_file_type, l_file[0], file[0], l_path[0], path[0] );

		status = DIopen(&logfile, path[0], l_path[0],
				    file[0], l_file[0],
				    2048, DI_IO_WRITE,
				    (u_i4)0, &sys_err);
		if (status == OK)
		{
		    one_page = 1;
		    status = DIread(&logfile, &one_page,
				    (i4)0,
				    buffer, &sys_err);
		    if (status == OK)
		    {
			status = DIclose(&logfile, &sys_err);
			if (status == OK)
			{
			    if (LGchk_sum(buffer, sizeof(LG_HEADER)) !=
				((LG_HEADER *)buffer)->lgh_checksum)
			    {
				status = FAIL;
			    }
			}
		    }
		}
	    }
	}
	if (status && status != FAIL)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, 
			&err_code, 0);
	}
    } while (FALSE);

    if (!g_silent)
	rcpstat_display_message( status );

    if ( status == OK )
	return( OK );
    else
	return( FAIL );

}

static STATUS
dual_readable(void)
{
    STATUS	status;
    CL_ERR_DESC	sys_err;
    i4	lgd_status;
    i4	lgd_active_log;
    i4	length;
    i4	err_code;
    DI_IO	logfile;
    i4		one_page;
    char	buffer[2048];
    char	file[LG_MAX_FILE][128];
    char	path[LG_MAX_FILE][128];
    i4	l_file[LG_MAX_FILE];
    i4	l_path[LG_MAX_FILE];
    i4		count_files;

    /*
    ** If system is online and active logs include dual, report OK
    ** If system is online and active logs DON'T include dual, FAIL
    ** If system is offline, open dual and read log header. If header
    ** checksums correctly, report OK, else report FAIL
    */

    do
    {
	/*
	** LGshow does not currently work across nodes
	*/
	if (g_another_node)
	{
	    /*
	    ** The csp cannot tell us this information. If we find the
	    ** csp is running issue a warning message that we may not
	    ** be able to read the log file in a consistent manner and
	    ** hence may return an invalid result.
	    */
	    status = LGc_csp_online( g_req_nodename );
	    if (( status == OK ) && (!g_silent) )
	    {
		i4	msg_index;

		for (msg_index=0; rcpstat_warning_msg[msg_index] != 0; 
		     msg_index++)
		{
		    TRdisplay(rcpstat_warning_msg[msg_index]);
		}
	    }
	}
	else
	{
	    status = LGshow( LG_S_LGSTS, (PTR)&lgd_status, sizeof(lgd_status), 
			     &length, &sys_err);
	    if ( status != OK)
	    {
		uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
				(char *)NULL, 0L, (i4 *)NULL, 
				&err_code, 0);
		break;
	    }

	    if (lgd_status & LGD_ONLINE)
	    {
		status = LGshow( LG_S_DUAL_LOGGING, (PTR)&lgd_active_log, 
				 sizeof(lgd_active_log), &length, &sys_err);
		if ( status != OK )
		{
		    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
				(char *)NULL, 0L, (i4 *)NULL, 
				&err_code, 0);
		    break;
		}

		if (lgd_active_log & LG_DUAL_ACTIVE)
		{ 
		    status = OK;
		    break;
		}
		else
		{
		    status = FAIL;
		    break;
		}
	    }
	}

	/*
	** Logging system is offline. Open dual log and read its header. If it
	** checksums correctly, the log is readable.
	** Logging system is offline. First check that all dual
	** logfile partitions exist by calling dual_exists(), then
	** open dual log and read its header. If it checksums,
	** then the log is readable.
	*/
	status = dual_exists();
	if (status == OK)
	{
	    status = LG_build_file_name( LG_DUAL_LOG, g_log_nodename, file,
					    l_file, path, l_path,
					    &count_files );
	    if (status == OK)
	    {
		if (!g_silent)
		    TRdisplay( "Opening %s Log file: %t in path: %t\n",
		        g_log_file_type, l_file[0], file[0], l_path[0], path[0] );

		status = DIopen(&logfile, path[0], l_path[0],
				    file[0], l_file[0],
				    2048, DI_IO_WRITE,
				    (u_i4)0, &sys_err);
		if (status == OK)
		{
		    one_page = 1;
		    status = DIread(&logfile, &one_page,
				    (i4)0,
				    buffer, &sys_err);
		    if (status == OK)
		    {
			status = DIclose(&logfile, &sys_err);
			if (status == OK)
			{
			    if (LGchk_sum(buffer, sizeof(LG_HEADER)) !=
				((LG_HEADER *)buffer)->lgh_checksum)
			    {
				status = FAIL;
			    }
			}
		    }
		}
	    }
	}
	if (status && status != FAIL)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, 
			&err_code, 0);
	}

    } while (FALSE);

    if (!g_silent)
	rcpstat_display_message( status );

    if ( status == OK )
	return( OK );
    else
	return( FAIL );

}

static STATUS
primary_enabled(void)
{
    STATUS	status;
    CL_ERR_DESC	sys_err;
    i4	lgd_status;
    i4	lgd_active_log;
    i4	lgh_active_logs;
    i4	length;
    i4	err_code;
    DI_IO	logfile;
    i4		one_page;
    char	buffer[2048];
    char	file[LG_MAX_FILE][128];
    char	path[LG_MAX_FILE][128];
    i4	l_file[LG_MAX_FILE];
    i4	l_path[LG_MAX_FILE];
    i4		count_files;

    /*
    ** If system is online and active logs include primary, report OK.
    ** If system is online and active logs DON'T include primary, FAIL
    ** If system is offline, open primary and read log header. If header
    ** checksums correctly and lgh_active_logs & LGH_PRIMARY_LOG_ACTIVE is
    ** true, report OK, else report FAIL.
    */

    do
    {
	/*
	** LGshow does not currently work across nodes
	*/
	if (g_another_node)
	{
	    /*
	    ** The csp cannot tell us this information. If we find the
	    ** csp is running issue a warning message that we may not
	    ** be able to read the log file in a consistent manner and
	    ** hence may return an invalid result.
	    */
	    status = LGc_csp_online( g_req_nodename );
	    if (( status == OK ) && (!g_silent) )
	    {
		i4	msg_index;

		for (msg_index=0; rcpstat_warning_msg[msg_index] != 0; 
		     msg_index++)
		{
		    TRdisplay(rcpstat_warning_msg[msg_index]);
		}
	    }

	}
	else
	{
	    status = LGshow( LG_S_LGSTS, (PTR)&lgd_status, sizeof(lgd_status), 
			     &length, &sys_err);
	    if ( status != OK )
	    {
		uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
				(char *)NULL, 0L, (i4 *)NULL, 
				&err_code, 0);
		break;
	    }

	    if (lgd_status & LGD_ONLINE)
	    {
		status = LGshow( LG_S_DUAL_LOGGING, (PTR)&lgd_active_log,
				 sizeof(lgd_active_log), &length, &sys_err);
		if ( status != OK )
		{
		    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
				(char *)NULL, 0L, (i4 *)NULL, 
				&err_code, 0);
		    break;
		}

		if (lgd_active_log & LG_PRIMARY_ACTIVE)
		{
		    status = OK;
		    break;
		}
		else
		{
		    status = FAIL;
		    break;
		}
	    }
	}

	/*
	** Logging system is offline. Open primary log and read its header. 
	** If it checksums correctly and lgh_active_logs & 
	** LGH_PRIMARY_LOG_ACTIVE is true, report OK, else report FAIL.
	*/

	status = LG_build_file_name( LG_PRIMARY_LOG, g_log_nodename, file,
					    l_file, path, l_path,
					    &count_files );
	if (status == OK)
	{
	    if (!g_silent)
		TRdisplay( "Opening %s Log file: %t in path: %t\n",
		        g_log_file_type, l_file[0], file[0], l_path[0], path[0] );

	    status = DIopen(&logfile, path[0], l_path[0],
				file[0], l_file[0],
				2048, DI_IO_WRITE,
				(u_i4)0, &sys_err);
	    if (status == OK)
	    {
		one_page = 1;
		status = DIread(&logfile, &one_page,
				(i4)0,
				buffer, &sys_err);
		if (status == OK)
		{
		    status = DIclose(&logfile, &sys_err);
		    if (status == OK)
		    {
			if (LGchk_sum(buffer, sizeof(LG_HEADER)) ==
				    ((LG_HEADER *)buffer)->lgh_checksum)
			{
			    lgh_active_logs = 
				((LG_HEADER *)buffer)->lgh_active_logs;
			    if ((lgh_active_logs & LGH_PRIMARY_LOG_ACTIVE) == 0)
				status = FAIL;
			}
			else
			    status = FAIL;
		    }
		}
	    }
	}
	if (status && status != FAIL)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, 
			&err_code, 0);
	}
    } while (FALSE);

    if (!g_silent)
	rcpstat_display_message( status );

    if ( status == OK )
	return( OK );
    else
	return( FAIL );
}

/*
** Name: dual_enabled		- is the dual log file enabled?
**
** History:
**	26-jul-1993 (bryanp)
**	    Added back an accidentally-dropped "if" statement.
**	31-jan-1994 (bryanp) B56473
**	    Return FAIL if online and dual log not enabled.
*/
static STATUS
dual_enabled(void)
{
    STATUS	status;
    CL_ERR_DESC	sys_err;
    i4	lgd_status;
    i4	lgd_active_log;
    i4	lgh_active_logs;
    i4	length;
    i4	err_code;
    DI_IO	logfile;
    i4		one_page;
    char	buffer[2048];
    char	file[LG_MAX_FILE][128];
    char	path[LG_MAX_FILE][128];
    i4	l_file[LG_MAX_FILE];
    i4	l_path[LG_MAX_FILE];
    i4		count_files;

    /*
    ** If system is online and active logs include dual , report OK.
    ** If system is online and active logs DON'T include dual, FAIL
    ** If system is offline, open dual and read log header. If header
    ** checksums correctly and lgh_active_logs & LGH_DUAL_LOG_ACTIVE is
    ** true, report OK, else report FAIL.
    */

    do
    {
	/*
	** LGshow does not currently work across nodes
	*/
	if (g_another_node)
	{
	    /*
	    ** The csp cannot tell us this information. If we find the
	    ** csp is running issue a warning message that we may not
	    ** be able to read the log file in a consistent manner and
	    ** hence may return an invalid result.
	    */
	    status = LGc_csp_online( g_req_nodename );
	    if (( status == OK ) && (!g_silent) )
	    {
		i4	msg_index;

		for (msg_index=0; rcpstat_warning_msg[msg_index] != 0; 
		     msg_index++)
		{
		    TRdisplay(rcpstat_warning_msg[msg_index]);
		}
	    }
	}
	else
	{
	    status = LGshow( LG_S_LGSTS, (PTR)&lgd_status, sizeof(lgd_status), 
			     &length, &sys_err);
	    if ( status != OK )
	    {
		uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
				(char *)NULL, 0L, (i4 *)NULL, 
				&err_code, 0);
		break;
	    }

	    if (lgd_status & LGD_ONLINE)
	    {
		status = LGshow( LG_S_DUAL_LOGGING, (PTR)&lgd_active_log,
				 sizeof(lgd_active_log), &length, &sys_err);
		if ( status != OK )
		{
		    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
				(char *)NULL, 0L, (i4 *)NULL, 
				&err_code, 0);
		    break;
		}

		if (lgd_active_log & LG_DUAL_ACTIVE)
		{
		    status = OK;
		    break;
		}
		else
		{
		    status = FAIL;
		    break;
		}
	    }
	}
	/*
	** Logging system is offline. Open dual log and read its header. 
	** If it checksums correctly and lgh_active_logs & 
	** LGH_DUAL_LOG_ACITVE is true, report OK, else report FAIL.
	*/

	status = LG_build_file_name( LG_DUAL_LOG, g_log_nodename, file,
					    l_file, path, l_path,
					    &count_files );
	if (status == OK)
	{
	    if (!g_silent)
		TRdisplay( "Opening %s Log file: %t in path: %t\n",
		        g_log_file_type, l_file[0], file[0], l_path[0], path[0] );

	    status = DIopen(&logfile, path[0], l_path[0],
				file[0], l_file[0],
				2048, DI_IO_WRITE,
				(u_i4)0, &sys_err);
	    if (status == OK)
	    {
		one_page = 1;
		status = DIread(&logfile, &one_page,
				(i4)0,
				buffer, &sys_err);
		if (status == OK)
		{
		    status = DIclose(&logfile, &sys_err);
		    if (status == OK)
		    {
			if (LGchk_sum(buffer, sizeof(LG_HEADER)) ==
				    ((LG_HEADER *)buffer)->lgh_checksum)
			{
			    lgh_active_logs = 
				((LG_HEADER *)buffer)->lgh_active_logs;
			    if ((lgh_active_logs & LGH_DUAL_LOG_ACTIVE) == 0)
				status = FAIL;
			}
			else
			    status = FAIL;
		    }
		}
	    }
	}
	else if ( g_another_node && E_DMA47C_LGOPEN_DUALLOG_PATH == status )
	{
	    /*
	    ** Dual log was not configured.  This is OK if we are
	    ** not positive that dual log exists. (other node case).
	    */
	    status = FAIL;
	}

	if (status && status != FAIL)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, 
			&err_code, 0);
	}
    } while (FALSE);

    if (!g_silent)
	rcpstat_display_message( status );

    if ( status == OK )
	return( OK );
    else
	return( FAIL );

}

static STATUS
logsizes_match(void)
{
    char	file[LG_MAX_FILE][128];
    char	path[LG_MAX_FILE][128];
    i4	l_file[LG_MAX_FILE];
    i4	l_path[LG_MAX_FILE];
    i4		count_files_prim;
    i4		count_files_dual;
    STATUS	status;
    DI_IO	di_io;
    CL_ERR_DESC	sys_err;
    i4	err_code;
    i4	lgd_status;
    i4	length;
    i4	primary_size = 0;
    i4	dual_size = 0;
    i4	blocks_first;
    i4	blocks_partition;
    i4		i;

    /*
    ** If system is online, report OK
    ** If system is offline, open both logs and get their sizes. If both logs
    ** could be opened and their sizes match, report OK, else report FAIL.
    */
    do
    {
	/*
	** LGshow does not currently work across nodes
	*/
	if (g_another_node)
	{
	    /*
	    ** If csp is online we will assume that the node is online and
	    ** all is ok.
	    */
	    status = LGc_csp_online( g_req_nodename );
	    if ( status == OK )
		break;
	}
	else
	{
	    status = LGshow( LG_S_LGSTS, (PTR)&lgd_status, sizeof(lgd_status), 
			     &length, &sys_err) ;
	    if ( status != OK )
	    {
		uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
				(char *)NULL, 0L, (i4 *)NULL, 
				&err_code, 0);
		break;
	    }

	    if (lgd_status & LGD_ONLINE)
	    {
		status = OK;
		break;
	    }
	}

	/*
	** Try to open primary logfile and get its size.
	*/
	status = LG_build_file_name( LG_PRIMARY_LOG, g_log_nodename, file,
					    l_file, path, l_path,
					    &count_files_prim );

	/* Loop through all primary partitions */
	for (i = 0, blocks_first = 0;
	     i < count_files_prim && status == OK;
	     i++)
	{
	    if (!g_silent)
		TRdisplay( "Opening Primary Log file: %t in path: %t\n",
			    l_file[i], file[i], l_path[i], path[i] );

	    status = DIopen(&di_io, path[i], l_path[i], file[i], l_file[i],
				2048, DI_IO_WRITE,
				(u_i4)0, &sys_err);

	    if (status == OK)
	    {
		status = DIsense(&di_io, &blocks_partition, &sys_err);
		if (status == OK)
		{
		    primary_size += blocks_partition;

		    status = DIclose(&di_io, &sys_err);

		    if (status == OK)
		    {
			/* All partitions must be the same size */
			if (blocks_first == 0)
			    blocks_first = blocks_partition;
			if (blocks_partition != blocks_first)
			    status = E_DMA810_LOG_PARTITION_MISMATCH;
		    }
		}
	    }
	}

	if (status == OK)
	{
	    status = LG_build_file_name( LG_DUAL_LOG, g_log_nodename, file,
					    l_file, path, l_path,
					    &count_files_dual );

	    /* Primary and dual files must have same number of partitions */
	    if (status == OK && count_files_prim != count_files_dual)
		status = E_DMA810_LOG_PARTITION_MISMATCH;
	}

	/* Loop through all dual partitions */
	for (i = 0, blocks_first = 0;
	     i < count_files_dual && status == OK;
	     i++)
	{
	    if (!g_silent)
		TRdisplay( "Opening Secondary Log file: %t in path: %t\n",
			    l_file[i], file[i], l_path[i], path[i] );

	    status = DIopen(&di_io, path[i], l_path[i], file[i], l_file[i],
				2048, DI_IO_WRITE,
				(u_i4)0, &sys_err);

	    if (status == OK)
	    {
		status = DIsense(&di_io, &blocks_partition, &sys_err);
		if (status == OK)
		{
		    dual_size += blocks_partition;

		    status = DIclose(&di_io, &sys_err);

		    if (status == OK)
		    {
			/* All partitions must be the same size */
			if (blocks_first == 0)
			    blocks_first = blocks_partition;
			if (blocks_partition != blocks_first)
			    status = E_DMA810_LOG_PARTITION_MISMATCH;
		    }
		}
	    }
	}

	if (status)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			    (char *)NULL, 0L, (i4 *)NULL, 
			    &err_code, 0);
	}
	else
	if (primary_size != dual_size)
	    status = FAIL;
    } while (FALSE);

    if (!g_silent)
	rcpstat_display_message( status );

    if ( status == OK )
	return( OK );
    else
	return( FAIL );

}

/*
** Name: rcpstat_display_message
**
** Description:
**	This routine display an information message about the request.
**
** Inputs:
**      status			        Indicates if the request was 
**				        succesfull or failed.
**
** Outputs:
**	None
**
** Side Effects:
**
** History:
**	30-apr-1993 (rmuth)
**	    Created.
**      23-Nov-2009 (horda03) Bug 122555
**          Display connected processes. ("-connected" flag).
*/
static VOID
rcpstat_display_message(
STATUS		status )
{
    do
    {
	if ( status == OK )
	{
	    if ( rcpstat_action & RCPSTAT_ONLINE )
	    {
		TRdisplay("Logging system is ONLINE on node %s.\n",
			  g_req_nodename);
		break;
	    }

	    if ( rcpstat_action & RCPSTAT_EXIST )
	    {
	        TRdisplay( "%s log file exists on node %s.\n",
	   	           g_log_file_type, g_req_nodename );
		break;
	    }

	    if ( rcpstat_action & RCPSTAT_FORMAT )
	    {
	        TRdisplay( "%s log file formatted on node %s.\n",
	   	           g_log_file_type, g_req_nodename );
		break;
	    }

	    if ( rcpstat_action & RCPSTAT_ENABLE )
	    {
	        TRdisplay( "%s log file enabled on node %s.\n",
	   	           g_log_file_type, g_req_nodename );
		break;
	    }

	    if ( rcpstat_action & RCPSTAT_SIZEOK )
	    {
	        TRdisplay( 
		   "Primary and Dual log files are same size on node %s.\n",
	   	   g_req_nodename );
		break;
	    }

	    if ( rcpstat_action & RCPSTAT_TRANSACT )
	    {
		TRdisplay(
		  "Logging system has recoverable xacts on node %s.\n",
			  g_req_nodename);
		break;
	    }

	    if ( rcpstat_action & RCPSTAT_CSP_ONLINE )
	    {
		TRdisplay("The Ingres cluster is ONLINE on node %s.\n",
		          g_req_nodename);
		break;
	    }

	    if ( rcpstat_action & RCPSTAT_ANY_CSP_ONLINE )
	    {
		TRdisplay("The Ingres cluster is currently ONLINE.\n");
		break;
	    }

	    if ( rcpstat_action & RCPSTAT_RECOVER )
	    {
		TRdisplay("Logging system is RECOVERING on node %s.\n",
			  g_req_nodename);
		break;
	    }

            if ( rcpstat_action & RCPSTAT_CONNECTED_PROCESS )
            {
                i4 i;
                /* Need different display formats. PIDs on VMS are displayed in
                ** hex. Decimals on other platforms.
                */
#ifdef VMS
#define PID_DISPLAY "   %x\n"
#else
#define PID_DISPLAY "   %d\n"
#endif
                TRdisplay("%d process%s connected to Locking/Logging shared memory\n",
                           g_conn_procs, (g_conn_procs == 1) ? "" : "es");

                for( i=0; i < g_conn_procs; i++)
                {
                   TRdisplay(PID_DISPLAY, g_conn_pids [i]);
                }

                break;
            }

	}
	else
	{
	    if ( rcpstat_action & RCPSTAT_ONLINE )
	    {
		TRdisplay("Logging system is OFFLINE on node %s.\n",
			  g_req_nodename);
		break;
	    }

	    if ( rcpstat_action & RCPSTAT_EXIST )
	    {
	        TRdisplay( "%s log file does not exist on node %s.\n",
	   	           g_log_file_type, g_req_nodename );
		break;
	    }

	    if ( rcpstat_action & RCPSTAT_FORMAT )
	    {
	        TRdisplay( "%s log file unformatted on node %s.\n",
	   	           g_log_file_type, g_req_nodename );
		break;
	    }

	    if ( rcpstat_action & RCPSTAT_ENABLE )
	    {
	        TRdisplay( "%s log file not enabled on node %s.\n",
	   	           g_log_file_type, g_req_nodename );
		break;
	    }

	    if ( rcpstat_action & RCPSTAT_SIZEOK )
	    {
	        TRdisplay( 
		   "Primary and Dual log files are not same size on node %s.\n",
	   	   g_req_nodename );
		break;
	    }

	    if ( rcpstat_action & RCPSTAT_TRANSACT )
	    {
		TRdisplay(
		  "Logging system has no recoverable xacts on node %s.\n",
			  g_req_nodename);
		break;
	    }
	    if ( rcpstat_action & RCPSTAT_CSP_ONLINE )
	    {
		TRdisplay("The Ingres cluster is OFFLINE on node %s.\n",
		          g_req_nodename);
		break;
	    }

	    if ( rcpstat_action & RCPSTAT_ANY_CSP_ONLINE )
	    {
		TRdisplay("The Ingres cluster is currently OFFLINE.\n");
		break;
	    }

	    if ( rcpstat_action & RCPSTAT_RECOVER )
	    {
		TRdisplay("The RCP is not waiting for a RECOVER event on node %s.\n",
			  g_req_nodename);
		break;
	    }

            if ( rcpstat_action & RCPSTAT_CONNECTED_PROCESS )
            {
               TRdisplay("No processes connected to the Locking/Logging shared memory.\n");
               break;
            }

	}

    } while (FALSE);

    return;
}

/*
** Name: rcpstat_parse_command_line
**
** Description:
**	This routine parses the command line and builds any context
**	required by the operation requested.
**
** Inputs:
**	argc				Number of arguments
**	argv				Array of Ptr to arguments.
**
** Outputs:
**	None
**
** Side Effects:
**
** History:
**	30-apr-1993 (rmuth)
**	    Created.
**	20-sep-1993 (jnash)
**	    Add -help.
**	25-apr-1994 (rmuth)
**	    b56703 The "-node nodename" option should check that the nodename
**	           is a valid cluster member.
**      23-Nov-2009 (horda03) Bug 122555
**          Added "-connected" flag.
*/
static VOID
rcpstat_parse_command_line(
i4		argc,
char		*argv[])
{
    i4	i;
    char	*pvalue;
    bool	invalid_args = FALSE;

    /*
    ** Check for minimum number of args
    */
    if ( argc < 2 )
    {
	return;
    }

    /*
    ** Parse the command line arguments
    */
    for ( i = 1; i < argc; i++)
    {
	if ( argv[i][0] == '-')
	{
	    switch (argv[i][1])
	    {
	    case 'a' :
		/*
		** "-any_csp_online" : Check if any CSP is running
		*/

		/*
		** Check no other option selected
		*/
		if ( rcpstat_action != RCPSTAT_NOTHING_SELECTED )
		{
		    invalid_args = TRUE;
		    break;
		}

		if ( STcompare("any_csp_online", &argv[i][1]) == 0 )
		{
		    /*
		    ** Only allowed in a cluster environment
		    */
		    if (!g_cluster)
		    {
			TRdisplay(
	"This option is only allowed in Ingres cluster environment\n");
		        invalid_args = TRUE;
			break;
		    }

		    rcpstat_action |= RCPSTAT_ANY_CSP_ONLINE;
		}
		else
		{
		    invalid_args = TRUE;
		}
		break;

	    case 'c' :
		/*
		** "-csp_online" : Check if any CSP is running
                ** "-connected"  : Check for connected processed to LGK mem.
		*/

		/*
		** Check no other option selected
		*/
		if ( rcpstat_action != RCPSTAT_NOTHING_SELECTED )
		{
		    invalid_args = TRUE;
		    break;
		}

		if ( STcompare("csp_online", &argv[i][1]) == 0 )
		{
		    /*
		    ** Only allowed in a cluster environment
		    */
		    if (!g_cluster)
		    {
			TRdisplay(
	"This option is only allowed in Ingres cluster environment\n");
		        invalid_args = TRUE;
			break;
		    }
		    rcpstat_action |= RCPSTAT_CSP_ONLINE;
		}
                else if ( STcompare("connected", &argv[i][1]) == 0 )
                {
                   rcpstat_action |= RCPSTAT_CONNECTED_PROCESS;
                }
		else
		{
		    invalid_args = TRUE;
		}
		break;

	    case 'd' :
		/*
		** "-dual" : Perform requested action on dual log
		*/
		if ( STcompare("dual", &argv[i][1]) == 0 )
		{
		    g_action_on_dual_log = TRUE;
		}
		else
		{
		    invalid_args = TRUE;
		}
		break;

	    case 'e' :
		/*
		** "-enable" : Check if log file enabled
		** "-exist"  : Check if log file exists.
		*/

		/*
		** Check no other option selected
		*/
		if ( rcpstat_action != RCPSTAT_NOTHING_SELECTED )
		{
		    invalid_args = TRUE;
		    break;
		}

		if ( STcompare("enable", &argv[i][1]) == 0 )
		   
		{
		    rcpstat_action |= RCPSTAT_ENABLE;
		}
		else
		{
		    if ( STcompare("exist", &argv[i][1]) == 0 )
		    {
			rcpstat_action |= RCPSTAT_EXIST;
		    }
		    else
		    {
			invalid_args = TRUE;
		    }
		}

		break;

	    case 'f' :
		/*
		** "-format" : Check if log is formatted
		*/

		/*
		** Check no other primary action specified
		*/
		if ( rcpstat_action != RCPSTAT_NOTHING_SELECTED )
		{
		    invalid_args = TRUE;
		    break;
		}

		if ( STcompare("format", &argv[i][1]) == 0 )
		{
		    rcpstat_action |= RCPSTAT_FORMAT;
		}
		else
		{
		    invalid_args = TRUE;
		}
		break;

	    case 'g' :
		/*
		** "-gcmt_free": loosen stuck group commit buffer
		*/

		/*
		** Check no other primary action specified
		*/
		if ( rcpstat_action != RCPSTAT_NOTHING_SELECTED )
		{
		    invalid_args = TRUE;
		    break;
		}

		if ( STcompare("gcmt_free", &argv[i][1]) == 0 )
		{
		    rcpstat_action |= RCPSTAT_GCMT_FREE;
		}
		else
		{
		    invalid_args = TRUE;
		}
		break;

	    case 'h':

		if (STcompare("help", &argv[i][1]) == 0)
		{
		    rcpstat_action |= RCPSTAT_HELP;
		    break;
		}
		else
		{
		    invalid_args = TRUE;
		}
		break;
    
	    case 'k' :
		/*
		** "-kill_all" : Kill all ingres processes
		*/

		/*
		** Check no other primary action specified
		*/
		if ( rcpstat_action != RCPSTAT_NOTHING_SELECTED )
		{
		    invalid_args = TRUE;
		    break;
		}

		if ( STcompare("kill_all", &argv[i][1]) == 0 )
		{
		    rcpstat_action |= RCPSTAT_KILLALL;
		}
		else
		{
		    invalid_args = TRUE;
		}
		break;
		
	    case 'n' :
		/*
		** "-node=nodename" or "-node nodename" : 
		*/

		if ( STxcompare("node", 4, &argv[i][1], 0, 0, 0) == 0 )
		{
		    /*
		    ** Only allowed in a cluster environment
		    */
		    if (!g_cluster)
		    {
			TRdisplay(
	"The -node option is only allowed from an Ingres cluster member\n");
		        invalid_args = TRUE;
			break;
		    }
			
		    if ( '=' == argv[i][5] )
		    {
			pvalue = &argv[i][6];
		    }
		    else if ( ( '\0' == argv[i][5] ) && ( ++i < argc ) )
		    {
			/*
			** Next argument must be the nodename
			*/
			pvalue = &argv[i][0];
		    }
		    else
		    {
			/* 
			** Node argument not supplied so break
			*/
		        invalid_args = TRUE;
			break;
		    }

		    STlcopy( pvalue, g_req_nodename, CX_MAX_NODE_NAME_LEN );

		    /*
		    ** See if requested host node is remote.
		    */
		    if (STcompare( g_host_nodename, g_req_nodename ) != 0 )
		    {
			g_another_node = TRUE;
		    }

		    /*
		    ** If another node validate that it is a cluster node
		    */

		    if (g_another_node)
		    {
			if ( CXnode_number(g_req_nodename) <= 0 )
			{
			    TRdisplay(
				"Requested node %s is not a cluster member\n",
				g_req_nodename);
			    invalid_args = TRUE;
			    break;
			}
			g_log_nodename = g_req_nodename;
		    }
		}
		else
		{
		    invalid_args = TRUE;
		}
		break;

	    case 'o' :
		/*
		** "-online" : Check if log is online
		*/

		/*
		** Check no other primary action specified
		*/
		if ( rcpstat_action != RCPSTAT_NOTHING_SELECTED )
		{
		    invalid_args = TRUE;
		    break;
		}

		if ( STcompare("online", &argv[i][1]) == 0 )
		{
		    rcpstat_action |= RCPSTAT_ONLINE;
		}
		else
		{
		    invalid_args = TRUE;
		}
		break;

	    case 'r' :
		/*
		** "-recover" : Check if RECOVER rcp waiting
		*/

		/*
		** Check no other primary action specified
		*/
		if ( rcpstat_action != RCPSTAT_NOTHING_SELECTED )
		{
		    invalid_args = TRUE;
		    break;
		}

		if ( STcompare("recover", &argv[i][1]) == 0 )
		{
		    rcpstat_action |= RCPSTAT_RECOVER;
		}
		else
		{
		    invalid_args = TRUE;
		}
		break;

	    case 's' :
		/*
		** "-sizeok" : Log files are the same size.
		** "-silent" : Just return True or False.
		*/
		if (( STcompare("sizeok", &argv[i][1]) == 0 )
				&&
		   ( rcpstat_action == RCPSTAT_NOTHING_SELECTED ))
		{
		    rcpstat_action = RCPSTAT_SIZEOK;
		}
		else
		{
		    if ( STcompare("silent", &argv[i][1]) == 0 )
		    {
			g_silent = TRUE;
		    }
		    else
		    {
		        invalid_args = TRUE;
		    }
		}

		break;

	    case 't' :
		/*
		** "-transactions" :  Check if log file contains some xacts.
		*/
		/*
		** Check no other primary action specified
		*/
		if ( rcpstat_action != RCPSTAT_NOTHING_SELECTED )
		{
		    invalid_args = TRUE;
		    break;
		}

		if ( STcompare("transactions", &argv[i][1]) == 0 )
		{
		    rcpstat_action = RCPSTAT_TRANSACT;
		}
		else
		{
		    invalid_args = TRUE;
		}
		break;

	    default:
		invalid_args = TRUE;
		break;
	    }

	}
	else
	{
	    invalid_args = TRUE;
	}

	/*
	** If invalid args have been specfied than bail
	*/
	if ( invalid_args )
	{
	    rcpstat_action = RCPSTAT_NOTHING_SELECTED;
	    break;
	}
		
    }

    return;
}

/*
** Name: csp_online
**
** Description:
**	This routine checks if the requested CSP process is online.
**
** Inputs:
**	nodename			Node on which to check.
**
** Outputs:
**	OK				If the CSP process is running.
**	FAIL				If the CSP process is not running.
**
** Side Effects:
**
** History:
**	30-apr-1993 (rmuth)
**	    Created.
*/
static STATUS
csp_online(
char	*nodename )
{
    STATUS	status;

    status = LGc_csp_online( nodename );

    if (!g_silent)
	rcpstat_display_message( status );

    if ( status == OK )
	return( OK );
    else
	return( FAIL );


}
/*
** Name: any_csp_online
**
** Description:
**	This routine checks if the any CSP process is online.
**
** Inputs:
**	None
**
** Outputs:
**	OK		- If a CSP process is running
**	FAIL		- If no CSP processes are running.
**	None
**
** Side Effects:
**
** History:
**	30-apr-1993 (rmuth)
**	    Created.
*/
static STATUS
any_csp_online( VOID )
{
    STATUS	status;

    status = LGc_any_csp_online();

    if (!g_silent)
	rcpstat_display_message( status );

    if ( status == OK )
	return( OK );
    else
	return( FAIL );
}

/*
** Name: kill_all_processes	- testing utility which kills an installation
**
** Description:
**	This routine looks up the process IDs of all processes which are
**	connected to the logging system (i.e., have issued an LGopen), and calls
**	PCforce_exit() to kill each such process. It is intended for use in
**	testing situations where we wish to simulate a node failure.
**
** Inputs:
**	None
**
** Outputs:
**	None
**
** Side Effects:
**	The installation dies.
**
** History:
**	26-apr-1993 (bryanp)
**	    Created for Cluster Support.
*/
static STATUS
kill_all_processes(VOID)
{
    STATUS	status = OK;
    LG_PROCESS	lpb;
    i4	length;
    CL_ERR_DESC	sys_err;
    i4	err_code;
    PID		lg_pids[200];	    /* arbitrary number I picked */
    i4	lgd_csp_pid;
    i4		actual_pids = 0;
    i4		i;

    status = LGshow(LG_S_CSP_PID, (PTR)&lgd_csp_pid, sizeof(lgd_csp_pid), 
                        &length, &sys_err);
    if (status)
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
	TRdisplay("Can't show logging system CSP Process ID.\n");
	return (status);
    }
    if (lgd_csp_pid)
	lg_pids[actual_pids++] = lgd_csp_pid;

    length = 0;

    for (;;)
    {
	status = LGshow(LG_N_PROCESS, (PTR)&lpb, sizeof(lpb), &length,
			&sys_err);
	if (status)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
	    TRdisplay("Can't show process information.\n");
	    return (status);
	}
	if (length != 0 && actual_pids < 200)
	{
#ifdef VMS		/* only VMS really likes to print PID's in hex */
	    TRdisplay("    %8x  %8x %v %6.4{%8d%}\n",
#else
	    TRdisplay("    %8x  %8d %v %6.4{%8d%}\n",
#endif
		lpb.pr_id, lpb.pr_pid,
	    "MASTER,ARCHIV,FCT,RUNAWAY,SLAVE,CKPDB,VOID,SBM,IDLE,DEAD,DYING",
		lpb.pr_type, &lpb.pr_stat, 0);
	    lg_pids[actual_pids++] = lpb.pr_pid;
	}
	else
	    break;
    }
    for (i = 0; i < actual_pids; i++)
    {
#ifdef VMS
	TRdisplay("Forcibly terminating process %x\n",
#else
	TRdisplay("Forcibly terminating process %d\n",
#endif
		lg_pids[i]);
	status = PCforce_exit(lg_pids[i], &sys_err);
	if (status)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
	    TRdisplay("Can't terminate process.\n");
	    return (status);
	}
    }
    TRdisplay("Successfully terminated %d processes\n", actual_pids);

    return (OK);
}

/*
** This code used to be run by an '-rconfdual' flag to rcpstat. The data file
** no longer exists (it's been replaced by PM's config.dat file). I left the
** code here for historical reasons, but it's not currently used.
*/
#if 0
static STATUS
rconf_dual_logging(void)
{
    char	    file_name[50];
    STATUS	    status;
    CL_ERR_DESC	    sys_err;
    i4	    count;
    LOCATION	    loc;
    FILE	    *fp;
    RCONF	    rconf;

    /*	Compute the name of the configuration file to use. */

    MEcopy("rcpconfig.dat_", 14, file_name);
    if (LGcluster())
    {
	status = LGcnode_info(&file_name[14], DM_CNAME_MAX, &sys_err);
	if (status == OK)
		file_name[14 + DM_CNAME_MAX] = 0;
    }
    else
	file_name[13] = 0;

    for (;;)
    {
	/* Open the rcpconfig.dat file. */

	status = NMloc(UTILITY, FILENAME, file_name, &loc);
	if (status == OK)
	{
	    if ((status = SIfopen(&loc, ERx("r"), SI_TXT, 0, &fp)) == OK)
	    {
		/* Get the configuration data. */

		status = SIread(fp, sizeof(RCONF), &count, (char *)&rconf);
		if (status == OK)
		    status = SIclose(fp);
	    }
	}
	if (status != OK)
	{
	    /* log the CL error. It may help. */
	    TRdisplay("\nCan't open %s.\n", file_name);
	    break;
	}

	if (rconf.dual_logging)
	    return (OK);
	else
	    return (FAIL);
    }
    return (status);
}
#endif
/*
** Name: free_stuck_gcmt_buffer		- workaround for group commit bugs
**
** Description:
**	We're currently plagued by an intermittent bug in group commit rundown
**	which leaves a group commit buffer on the timer queue with no group
**	commit thred watching it. If an installation gets stuck in such a
**	state, this routine frees it.
**
** History:
**	23-aug-1993 (bryanp)
**	    Created.
**	26-Jan-1996 (jenjo02)
**	    Removed acquisition and freeing of lgd_mutex. 
**	    Lock lgd_current_lbb_mutex instead.
**	24-Aug-1999 (jenjo02)
**	    Dropped lgd_timer_lbb, use lfb_current_lbb instead.
**	    lfb_current_lbb_mutex replaced by individual LBB's lbb_mutex.
*/
static STATUS
free_stuck_gcmt_buffer(void)
{
    LGD		    *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    CL_ERR_DESC	    sys_err;
    LBB		    *current_lbb;
    STATUS	    status;
    STATUS	    ret_status = OK;
    LFB		    *lfb = &lgd->lgd_local_lfb;
    LPB		    *master_lpb;

    master_lpb = (LPB *)LGK_PTR_FROM_OFFSET(lgd->lgd_master_lpb);

    while (current_lbb = (LBB *)LGK_PTR_FROM_OFFSET(lfb->lfb_current_lbb))
    {
       if (status = LG_mutex(SEM_EXCL, &current_lbb->lbb_mutex))
       {
   	   TRdisplay("RCPSTAT: Unable to acquire current LBB mutex.\n");
   	   return (status);
       }

       if ( current_lbb == (LBB *)LGK_PTR_FROM_OFFSET(lfb->lfb_current_lbb) )
       {
	  /* current_lbb is still the "CURRENT" lbb. */
          break;
       }

       (VOID)LG_unmutex(&current_lbb->lbb_mutex);

#ifdef xDEBUG
       TRdisplay( "RCPSTAT(1):: Scenario for INGSRV2838 handled. EOF=<%d, %d, %d>\n",
                  lfb->lfb_header.lgh_end.la_sequence,
                  lfb->lfb_header.lgh_end.la_block,
                  lfb->lfb_header.lgh_end.la_offset);
#endif
    }

    if ((current_lbb->lbb_state & LBB_FORCE) == 0)
    {
	TRdisplay("RCPSTAT: No buffer found which needs forcing.\n");
	ret_status = LG_NO_BUFFER;
	(VOID)LG_unmutex(&current_lbb->lbb_mutex);
    }
    else
    {
	TRdisplay("RCPSTAT: Queueing write now...\n");
	lgd->lgd_gcmt_threshold = 1000;
	/* LG_queue_write returns with current's lbb_mutex released */
	LG_queue_write(current_lbb, master_lpb, (LXB*)NULL, 0);
	TRdisplay("RCPSTAT: Disabling group commit, since it's broken!\n");
    }

    return(ret_status);
}


static STATUS
is_recover(void)
{
    STATUS	status;
    CL_ERR_DESC	sys_err;
    i4	lgd_status;
    i4	length;
    i4	err_code;
    LG_RECOVER	recover_show;
    LG_RECOVER  recover_init;

    MEfill(sizeof(recover_init), 0, (char *)&recover_init);

    do
    {
	if (g_another_node)
	{
	    /*
	    ** If csp is online we will assume that the log is online
	    */
	    status = LGc_csp_online( g_req_nodename );
	    if ( status == OK )
		break;
	}
	else
	{
	    status = LGshow(LG_S_SHOW_RECOVER, 
			    (PTR)&recover_show, sizeof(recover_show), 
			    &length, &sys_err);

	    if (status != OK )
	    {
		uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
				(char *)NULL, 0L, (i4 *)NULL, 
				&err_code, 0);
		break;
	    }

	    if (MEcmp(&recover_init, &recover_show, sizeof(LG_RECOVER)))
	    {
		if (!g_silent)
		{
		    TRdisplay(
	    "WARNING The RCP encountered an error and is waiting for input.\n");
		    TRdisplay("Valid input is:\n");
		    TRdisplay("rcpconfig -rcp_continue_ignore_db=%~t\n",
			DB_MAXNAME, recover_show.lg_dbname);
		    TRdisplay("OR\n");
		    TRdisplay("rcpconfig -rcp_continue_ignore_table=%~t\n",
			DB_MAXNAME, recover_show.lg_tabname);
		    TRdisplay("OR\n");
		    TRdisplay("rcpconfig -rcp_continue_ignore_lsn=%d,%d\n",
			recover_show.lg_lsn.lsn_high, 
			recover_show.lg_lsn.lsn_low);
		    TRdisplay("OR\n");
		    TRdisplay("rcpconfig -rcp_stop_lsn=%d,%d\n",
			recover_show.lg_lsn.lsn_high, 
			recover_show.lg_lsn.lsn_low);
		}
		status = OK;
		break;
	    }
	    else
	    {
		status = FAIL;
		break;
	    }
	}

    } while (FALSE);

    if (!g_silent)
	rcpstat_display_message( status );

    if ( status == OK )
	return( OK );
    else
	return( FAIL );
}
/*
** Name: connected
**
** Description:
**      This routine checks if any processes (except self) are connected
**      to the LGK memory.
**
** Inputs:
**      None
**
** Outputs:
**      OK              - If any other process connected.
**      FAIL            - If no other process connected
**      None
**
** Side Effects:
**
** History:
**      23-Nov-2009 (horda03) Bug 122555
**          Created.
*/
static STATUS
connected( VOID )
{
   STATUS	status;
   LGK_MEM	*lgk_mem = (LGK_MEM *)LGK_base.lgk_mem_ptr;
   i4		i;
   PID		pid;

   PCpid( &pid );

   for ( i = 0; i < LGK_MAX_PIDS; i++ )
   {
      if (lgk_mem->mem_pid[i] == pid) continue;

      if ( lgk_mem->mem_pid[i] &&
           PCis_alive(lgk_mem->mem_pid[i]) )
      {
         g_conn_pids [g_conn_procs++] = lgk_mem->mem_pid[i];
      }
   }

   status =  (g_conn_procs) ? OK : FAIL;

   if (!g_silent)
      rcpstat_display_message( status );

   return status;
}
   
