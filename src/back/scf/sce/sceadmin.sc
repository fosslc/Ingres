/*
**Copyright (c) 2004 Ingres Corporation
** All Rights Reserved
*/

#include    <compat.h>
#include    <gl.h>
#include    <cv.h>
#include    <er.h>
#include    <ex.h>
#include    <si.h>
#include    <cm.h>
#include    <cs.h>
#include    <st.h>
#include    <me.h>
#include    <tm.h>
#include    <pc.h>
#include    <lk.h>
#include    <sl.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <ulf.h>
#include    <gca.h>
#include    <scf.h>
#include    <qsf.h>
#include    <adf.h>
#include    <ulm.h>

/* added for scs.h prototypes, ugh! */
#include <dudbms.h>
#include <dmrcb.h>
#include <copy.h>
#include <qefrcb.h>
#include <qefqeu.h>
#include <qefcopy.h>

#include    <sc.h>
#include    <sc0m.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>
#include    <sceshare.h>
#include    <sce.h>

/**
**
**  Name: sceadmin.sc - Administer the server and global EV subsystems
**
**  Description:
**	This module provides the interface to internal trace points to
**	maintain (cleanup, trace, dump) the event subsystem.  Refer to the
**	menus below for the available operations.
**
**	This module is very dependent on the trace point sc923 and its
**	interface.
**
**  Usage: sceadmin any_db_name
**
**  History:
**      30-jan-91 (neil)
**	    First written to support alerters.
**      21-feb-91 (neil)
**	    Allowed a system call via CALL SYSTEM (new menu).
**      16-aug-91 (ralph)
**          Bug b39085.  Detect servers that terminate without disconnecting.
**	    sce_dispatch() prints server status.
**	    sce_traptrace() extracts server status.
**	    sce_dispatch() prints pid in decimal if UNIX
**	14-sep-91 (ralph)
**	    Bug 39793. Package for customer use:
**		Add ming hint PROGRAM=IIEVUTIL
**		Specify $ingres as effective user to require DB_ADMIN.
**		Disallow parameter.  Always connect to iidbdb, to
**		effectively require TRACE subject privilege and
**		either SUPER subject privilege or DB_ADMIN explicitly
**		granted on the iidbdb database.
**	16-nov-92 (bonobo)
**		Added CUFLIB MING hint.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	18-Dec-97 (gordy)
**	    Added SQLCALIB to NEEDLIBS, now required by LIBQ.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2004 (drivi01)
**	    Replaced static libraries with SHEMBEDLIB and SHQLIB.
**	    On unix SHEMBEDLIB=SHQLIB, SHEMBEDLIB is a windows library.
**	13-May-2005 (kodse01)
**	    replace %ld with %d for old nat and long nat variables.
**	05-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Fix prototypes
**/

/*
** MING hints:

PROGRAM =	iievutil

NEEDLIBS =	SHQLIB COMPATLIB SHEMBEDLIB
**
*/

/* Forward declarations */
static i4 sce_menu(void);
static void sce_dispatch( i4 op );
static i4 sce_waserr(void);
static i4 sce_traptrace( i4 last, DB_DATA_VALUE *dbt );

/* Menu list for operations */
struct	{
    i4		mu_op;
    char	*menu;
} menus[] =
    {
	{ 1,  "Remove event thread for current server" },
	{ 2,  "Start event thread for current server" },
	{ 3,  "Trace event thread to terminal" },
	{ 4,  "Untrace event thread" },
	{ 5,  "Dump server SCE event structures" },
	{ 6,  "Dump cross server EV event structures" },
	{ 7,  "List servers connected to EV" },
	{ 8,  "Probe & remove servers connected to EV" },
	{ 9,  "Execute a trace point statement" },
	{10,  "Execute an operating system command" },
#define	MAXMU	    10
	/* To add another menu:
	** #   Text here
	** Bump MAXMU & add the case in sce_dispatch
	*/
    };

/* Place to save server process ids traped from EV trace */
#define	MAXPIDS	500
struct {
    PID		pid;
    char	stat[33];
} pids[MAXPIDS];
static	i4	num_pids = 0;

/*{
** Name: main   - Main entry point
**
** Description:
**	Accept input, connect to database, and field requests.
**
** Inputs:
**	argc			# of command line arguments
**	argv[1]			Database name
** Outputs:
**	Exits
**
** History:
**      30-jan-91 (neil)
**	    First written to support alerters.
**	16-aug-91 (ralph)
**	    Use database "iidbdb" if no database was specified.
**	2-Jul-1993 (daveb)
**	    specify int return type.
*/

int
main(argc, argv)
i4	argc;
char	**argv;	
{
    EXEC SQL BEGIN DECLARE SECTION;
	char	*db;
	i4	op;
    EXEC SQL END DECLARE SECTION;

    MEadvise(ME_INGRES_ALLOC);

#ifdef xDEBUG
    if (argc > 2)
    {
	SIprintf("Usage: %s [any_db_name]\n",argv[0]);
	PCexit(FAIL);
    }

    if (argc == 2)
	db = argv[1];
    else
	db = "iidbdb";
#else
    db = "iidbdb";
#endif

    SIprintf("EVADM: Connecting ...\n");
    SIflush(stdout);
    EXEC SQL CONNECT :db identified by '$ingres';
    if (sce_waserr())
    {
	SIprintf("EVADM: Exiting because of error ...\n");
	EXEC SQL DISCONNECT;
	PCexit(FAIL);
    }
    while (1)
    {
	if ((op = sce_menu()) == 0)
	    break;
	sce_dispatch(op);
    }
    SIprintf("EVADM: Disconnecting ...\n");
    EXEC SQL DISCONNECT;
    PCexit(OK);	
    /*NOTREACHED*/	
    return(FAIL);	
} /* main */

/*{
** Name: sce_menu   - Menu for input
**
** Description:
**	Prompt for action
**
** Inputs:
**	None
** Outputs:
**	Menu action
**
** History:
**      30-jan-91 (neil)
**	    First written to support alerters.
*/
static i4
sce_menu()
{
    char	buf[100];
    i4		mu;
    i4		i;

    while (1)
    {
	for (i = 0; i < MAXMU; i++)
	    SIprintf(" %2d   %s\n", menus[i].mu_op, menus[i].menu);
	SIprintf("  Q   Quit\n");
	while (1)
	{
	    SIprintf("  Choice: ");
	    if (SIgetrec(buf, sizeof(buf)-1, stdin) != OK)
		return 0;
	    buf[STlength(buf)-1] = EOS;
	    if (buf[0] != EOS)
		break;
	}
	if (buf[0] == 'q' || buf[0] == 'Q')
	    return 0;
	if (CVan(buf, &mu) != OK)
	{
	    SIprintf("%%EVADM-ERR: Invalid menu option <%s>\n", buf);
	    continue;
	}
	if (mu < 1 || mu > MAXMU)
	{
	    SIprintf("%%EVADM-ERR: Menu option <%d> out of range 1..%d\n", 
		     mu, MAXMU);
	    continue;
	}
	return mu;
    }
} /* sce_menu */

/*{
** Name: sce_dispatch   - Dispatch admin operation
**
** Description:
**	Based on action execute admin function
**
** Inputs:
**	op		Operation code according to menu (1..MAXMU)
** Outputs:
**	Trace output
**
** History:
**      30-jan-91 (neil)
**	    First written to support alerters.
**	16-aug-91 (ralph)
**          Bug b39085.  Detect servers that terminate without disconnecting.
**	    sce_dispatch() prints server status.
**	    sce_dispatch() prints pid in decimal if UNIX
*/
static void
sce_dispatch( i4 op )
{
    EXEC SQL BEGIN DECLARE SECTION;
	i4	pidx;
	char	stmt[200];
	char	buf[100];
    EXEC SQL END DECLARE SECTION;

    SIprintf("\n                          ... %s\n", menus[op-1].menu);
    SIflush(stdout);
    switch (op)
    {
      case 1:		/* Remove event thread */
	STprintf(stmt, "SET TRACE POINT SC923 %d", SCE_AREM_THREAD);
	EXEC SQL EXECUTE IMMEDIATE :stmt;
	break;
      case 2:		/* Add event thread */
	STprintf(stmt, "SET TRACE POINT SC923 %d", SCE_AADD_THREAD);
	EXEC SQL EXECUTE IMMEDIATE :stmt;
	break;
      case 3:		/* Trace event thread */
	EXEC SQL SET TRACE TERMINAL;
	if (sce_waserr())
	    break;
	STprintf(stmt, "SET TRACE POINT SC923 %d 1", SCE_ATRC_THREAD);
	EXEC SQL EXECUTE IMMEDIATE :stmt;
	break;
      case 4:		/* Untrace event thread */
	EXEC SQL SET NOTRACE TERMINAL;
	if (sce_waserr())
	    break;
	STprintf(stmt, "SET TRACE POINT SC923 %d 0", SCE_ATRC_THREAD);
	EXEC SQL EXECUTE IMMEDIATE :stmt;
	break;
      case 5:		/* Dump SCE structures */
    	EXEC SQL SET TRACE POINT SC921;
	break;
      case 6:		/* Dump EV structures */
    	EXEC SQL SET TRACE POINT SC922;
	break;
      case 7:		/* List servers in EV */
      case 8:		/* Update servers in EV */
	IItm_trace(sce_traptrace);
	num_pids = 0;
    	EXEC SQL SET TRACE POINT SC922;
	IItm_trace(0L);
	if (sce_waserr())
	    break;
	for (pidx = 0; pidx < num_pids; pidx++)
	{
#ifdef	UNIX
	    STprintf(buf, "%d", pids[pidx].pid);
#else
	    STprintf(buf, "%x", pids[pidx].pid);
#endif
	    CVupper(buf);
	    SIprintf("   [%d] %s: %s %s", pidx+1, SCEV_xSVR_PID, buf,
		     pids[pidx].stat);
	    if (op == 7)
	    {
		SIprintf("\n");
		continue;
	    }
	    SIprintf(" - (R)emove, (N)ext, (Q)uit ? ");
	    SIgetrec(buf, sizeof(buf)-1, stdin);
	    if (buf[0] == 'q' || buf[0] == 'Q')
		break;
	    if (buf[0] == 'r' || buf[0] == 'R')
	    {
		STprintf(stmt, "SET TRACE POINT SC923 %d %d",
			 SCE_AREMEX_THREAD, pids[pidx].pid);
		EXEC SQL EXECUTE IMMEDIATE :stmt;
	    }
	}
	if (num_pids == 0)
	{
	    SIprintf("%%EVADM-INFO: Cannot access server list - ");
	    SIprintf("maybe you're not connected to EV\n");
	}
	break;
      case 9:		/* Tracepoint query */
	SIprintf("  Query: ");
	SIgetrec(stmt, sizeof(stmt)-1, stdin);
	if (stmt[0] != EOS && stmt[0] != '\n')
	{
	    if (stmt[STlength(stmt)-2] == ';')
		stmt[STlength(stmt)-2] = EOS;
	    else
		stmt[STlength(stmt)-1] = EOS;
	    EXEC SQL EXECUTE IMMEDIATE :stmt;
	}
	break;
      case 10:		/* O/S command */
	SIprintf("  Command [return for subprocess]: ");
	SIgetrec(stmt, sizeof(stmt)-1, stdin);
	if (stmt[0] != EOS)
	    stmt[STlength(stmt)-1] = EOS;
	/* An empty line implies spawning a subprocess */
	EXEC SQL CALL SYSTEM (COMMAND = :stmt);
	break;
      default:
	SIprintf("%%EVADM-ERR: Menu option <%d> out of range 1..%d\n", 
		 op, MAXMU);
	break;
    }
    SIprintf("\n");
} /* sce_dispatch */

/*{
** Name: sce_waserr   - Was there an error
**
** Inputs:
**	none
** Outputs:
**	Error number != 0 means error
**
** History:
**      30-jan-91 (neil)
**	    First written to support alerters.
*/

static i4
sce_waserr()
{
    EXEC SQL BEGIN DECLARE SECTION;
	i4	err;
    EXEC SQL END DECLARE SECTION;

    EXEC SQL INQUIRE_INGRES (:err = ERRORNO);
    return (err != 0);
} /* sce_waserr */

/*{
** Name: sce_traptrace   - Routine to save/inspect trace text
**
** Description:
**	Very dependent on IItm_trace working as it does for the TM.
**	See interface in libq/iitm.c.
**
**	Searches trace text for special lines:
**
**	   Marker			Action
**	   ------			------
**	   Begin with SCEV_xSVR_PID	Extract/save server id
**		
**	If new trace lines are trapped make sure to add the #define pattern
**	(SCEV_XYYY) and modify appropriate SCE dump routine and this routine.
**
** Inputs:
**	last		Last piece
**	dbt		DBV with text line
** Outputs:
**	None
**
** History:
**      30-jan-91 (neil)
**	    First written to support alerters.
**	16-aug-91 (ralph)
**          Bug b39085.  Detect servers that terminate without disconnecting.
**	    sce_traptrace() extracts server status.
*/

static i4
sce_traptrace( i4 last, DB_DATA_VALUE *dbt )
{
    char	*cp;
    char	pbuf[100];
    char	*ocp;

    if (last)
	return 0;
    
    /* Skip initial blansk and search for our pattern */
    for (cp = dbt->db_data; *cp == ' '; cp++) ;
    if (MEcmp((PTR)SCEV_xSVR_PID, cp, sizeof(SCEV_xSVR_PID)-1) != 0)
	return 0;
    /* Skip the pattern till the next word */
    for ( ; *cp != ' '; cp++) ;
    for ( ; *cp == ' '; cp++) ;
    /*
    ** Now at server process id - skip the 0x (if there's 1 or 2).  Why 2?
    ** Some STprintf implementations internally precede with 0x.  The trace
    ** routine also puts a 0x out there - so we may have 2!
    */
    if (*cp == '0' && (*(cp+1) == 'x' || *(cp+1) == 'X'))
	cp += 2;
    if (*cp == '0' && (*(cp+1) == 'x' || *(cp+1) == 'X'))
	cp += 2;
    /* Pick up hex process id */ 
    ocp = pbuf;
    while (CMhex(cp))
	CMcpyinc(cp, ocp);
    *ocp = EOS;
    if (CVahxl(pbuf, &pids[num_pids].pid) != OK)
    {
	SIprintf("%%EVADM-ERR: Cannot extract pid %s from <%.*s>\n", 
		 pbuf, dbt->db_length, dbt->db_data);
	IItm_trace(0L);
	return 0;
    }
    /* Skip the pattern till the next word */
    for ( ; *cp != ' '; cp++) ;
    for ( ; *cp == ' '; cp++) ;
    /* Now get the server status */
    ocp = pids[num_pids].stat;
    while (!CMwhite(cp))
	CMcpyinc(cp, ocp);
    *ocp = EOS;
    /* Bump pid counter */
    num_pids++;
    if (num_pids >= MAXPIDS)                /* Dats lotta servers */
    {
	SIprintf("%%EVADM-ERR: Server pid list overflow <MAXPIDS = %d>\n",
		MAXPIDS);
	IItm_trace(0L);
	return 0;
    }
    return 0;
} /* sce_traptrace */
