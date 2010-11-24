/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <st.h>
#include    <systypes.h>
#include    <clconfig.h>
#include    <cs.h>
#include    <qu.h>
#include    <gc.h>
#include    <tr.h>
#include    <lo.h>

#include    <bsi.h>

#include    "gcacli.h"
#include    "gcarw.h"
#include    <signal.h>


/**
**
**  Name: GCACL.C - GCA private CL routines
**
**  Description:
**
**  Source is broken into the following files:
**
**	GCACL.C - housekeeping + junk
**	    GCinitiate - Initialize - must be first call
**          GCterminate - Perform necessary local termination functions
**		(remainder unimplemented)
**
**	GCAGCN.C - name server support
**	    GCnsid - ID local name server
**	    GCdetect - Like GCconnchk 
**
**	GCACONN.C - connection manager
**          GCrequest - Build connection to peer entity
**          GCregister - Build "listen" internet socket
**          GClisten - Accept connection to peer entity
**          GCrelease - Delete connection to peer entity
**          GCdisconn - Deassign channel connections for an association
**
**	GCARW.C - send/receive routines
**          GCsend - Send Protocol Data Unit
**          GCreceive - Receive Protocol Data Unit
**          GCpurge   - Cancel outstanding requests and send purge mark 
**
**	GCASAVE.C - Propagate connection to child process (gag)
**	    GCsave - save context
**	    GCrestore - restore context
**
**	GCACLI.H - Internal header file for GCA CL
**
**
**
**  History:    $Log-for RCS$
**	10-Sep-87 (berrow)
**	    First cut at UNIX port
**	29-Feb-88 (berrow)
**	    Addition of asynchronous I/O capability via MASTER/SLAVE technique.
**	13-Jan-89 (seiwald)
**	    Revamped to use CLpoll.
**	31-Oct-89 (seiwald)
**	    Don't convert II_GC_TRACE value of 0 to 1.  This allows you
**	    to set II_GC_TRACE to 0 to inhibit tracing.
**	16-Feb-90 (seiwald)
**	    Zombie checking removed from mainline to VMS CL.  Removed
**	    stubbed zombie checking calls from UNIX CL.
**	24-May-90 (seiwald)
**	    Built upon new BS interface defined by bsi.h.
**	    Includes (nascent) support for switchable BS drivers.
**      26-May-90 (seiwald)
**	    Added TLI for TCP/IP.
**	06-Jun-90 (seiwald)
**	    Made GCalloc() return the pointer to allocated memory.
**	14-Jun-90 (seiwald)
**	    GCterminate is now VOID.
**	4-Jul-90 (seiwald)
**	    Support for selectable BS driver underneath GCA CL: the 
**	    symbol II_GC_PROT can be set to anything available in the
**	    protocol table in gcaptbl.c.
**	2-Aug-90 (gautam)
**	    Made a separate function to set GCdriver for BSpipe support
**	15-Aug-90 (seiwald)
**	    GCinititate returns STATUS.
**	29-Aug-90 (gautam)
**	    GCterminate now unlisten's if needed.
**	27-Feb-91 (seiwald)
**	    Catch SIGPIPE if it's not otherwise handled.  All BS drivers can
**	    potentially generate innocent SIGPIPE's.
**	6-Apr-91 (seiwald)
**	    Be a little more assiduous in clearing svc_parms->status on
**	    function entry.
**	02-jul-91 (johnr)
**          Add <systypes.h> to clear <types.h> on hp3_us5.
**	08-jul-91 (seng)
**	    Add ris_us5 (RS/6000) to list of machines needing <systypes.h>
**	7-Aug-91 (seiwald)
**	    In GCinitiate, ignore SIGPIPE even if EX is catching it.  
**	18-aug-91 (ketan)
**	    Add nc5_us5 (NCR sys 3000) to list of machines needing <systypes.h>
**      19-Aug-91 (Sanjive)
**          Add dra_us5 to list of machines needing <systypes.h> and change
**          bitwise OR to logical OR in the #if defined statement.
**      11-Sep-91 (Wojtek)
**          Added dr6_us5 to above. Also fixed the submissions for ris_us5
**	    (08-jul-91) and nc5_us5 (18-aug-91) that were using bitwise OR
**	    not Boolean.
**	08-jan-1992 (bonobo)
**	    #including <systypes.h> does not need to be port-specific.
**      12-Feb-92 (jab)
**          Added hp8_us5 to above list that needs <systypes.h>
**      25-Feb-92 (sbull)
**          Added entry for the dr6_uv1
**	17-aug-92 (peterk)
**	    Added su4_us5 to above list that needs <systypes.h>
**      19-oct-92 (gmcquary)
**          Added pym_us5 to a flag train.
**	11-Jan-93 (edg)
**	    Use gcu_get_tracesym() to get trace symbol.
**	18-Jan-93 (edg)
**	    gcu_get_tracesym() removed & replaced with inline code.
**      27-jan-93 (pghosh)
**          Modified nc5_us5 to nc4_us5. Till date nc5_us5 & nc4_us5 were
**          being used for the same port. This is to clean up the mess so
**          that later on no confusion is created.
**	24-mar-93 (kirke)
**	    Virtually everybody needs to include <systypes.h> and it doesn't
**	    hurt to include it even if it isn't required, so remove all the
**	    ifdefs and just include it.
**	05-apr-95 (canor01)
**	    Add support for shared memory batchmode driver
**	25-apr-95 (canor01)
**	    pass partner name to GC_get_driver() to check for batchmode
**	    connection request
**	04-aug-1995 (canor01)
**	    shared memory driver keyed on "batchmode" and "gca_single_mode"
**	04-oct-1999 (somsa01)
**	    Moved calls to PMget() of "batchmode" and "gca_single_mode" to
**	    GCinitiate() from GC_get_driver(). This prevents a memory leak
**	    caused by continuous PMget() calls.
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	15-Jun-2004 (schka24)
**	    It doesn't make sense to set II_GC_PROT at the shell level
**	    since everyone has to agree on it.  Use NMgtIngAt.
**	15-nov-2010 (stephenb)
**	    Add proto for GC_get_driver.
[@history_template@]...
*/


/*
**  Forward and/or External function references.
*/

PTR		(*GCalloc)();		/* Memory allocation routine */
VOID		(*GCfree)();		/* Memory deallocation routine */
i4		gc_trace = 0;		/* Tracing Flag */
static char	batch_mode[8] = "";	/* Batch server status */
static char	gca_single_mode[8] = "";/* GCA single mode status */
GLOBALREF GC_PCT GCdrivers[];		/* protocol drivers available */
GLOBALREF char  listenbcb[];            /* allocated listen control block */
GLOBALREF bool	iiunixlisten;		/* is this a listen server ? */
GLOBALREF bool	iisynclisten;		/* is this a sync listen? */
GLOBALDEF BS_DRIVER	*GCdriver;	/* protocol driver selected */

BS_DRIVER *	GC_get_driver(char *);



BS_DRIVER *
GC_get_driver(partner)
char *partner;
{
	char		*driver = NULL;
	BS_DRIVER	*bsd;
	LOCATION	driv_loc;
	char		dev[MAX_LOC];
	char		path[MAX_LOC];
	char		fprefix[MAX_LOC];
	char		fsuffix[MAX_LOC];
	char		version[4];

	/* must be semaphore protected */
	char		*mode = NULL;

	/* Set default driver */

	bsd = GCdrivers[0].driver;

	/* Scan for alternate driver */

	if ( partner != NULL )
	{
	    /* get the base name of the connect id */
	    LOfroms( PATH & FILENAME, partner, &driv_loc );
	    LOdetail( &driv_loc, dev, path, fprefix, fsuffix, version );
	    /* if it is batchmode, it will be IIB.nnnnn */
	    if ( STcmp( fprefix, "IIB" ) == 0 )
		driver = "SHM_BATCHMODE";
	}
	     
	if ( !( driver && *driver ) )
	{
	    if ( STcasecmp( batch_mode, "on" ) == 0 &&
		 STcasecmp( gca_single_mode, "on" ) == 0 )
		driver = "SHM_BATCHMODE";
	    else
		driver = NULL;
	}
	if ( !( driver && *driver ) )
	    NMgtIngAt( "II_GC_PROT", &driver );

	if( driver && *driver )
	{
	    GC_PCT	*pct = GCdrivers;

	    while( pct->prot && STcasecmp( driver, pct->prot ) )
		    pct++;
	    
	    if( pct->prot )
		    bsd = pct->driver;
	}

	return bsd;
}

/*{
** Name: GCinitiate - Initialize static elements w/in GC CL.
**
** Description:
**
** This routine is called by gca_initate to pass certain variables to the CL.
** Currently, these are the addresses of the memory allocation and deallocation
** routines to be used.
**
** Inputs:
**
**      alloc                           Pointer to memory allocation routine
**
**      dealloc                         Pointer to memory deallocation routine
**
** Outputs:
**
*       none
**
** Returns:
**     none
** Exceptions:
**     none
**
** Side Effects:
**     none
** History:
**      10-Nov-1987 (jbowers)
**          Created.
**	7-Aug-91 (seiwald) bug #38968
**	    Ignore SIGPIPE even if EX is catching it.  
**	11-Jan-93 (edg)
**	    Stop getting trace symbol II_GC_LOG.  Added call to
**	    gcu_get_tracesym() to get gc trace level.
**	18-Jan-93 (edg)
**	    gcu_get_tracesym replaced with inline code.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	16-aug-93 (ed)
**	    add <st.h>
*/

STATUS
GCinitiate( alloc, free )
PTR	(*alloc)();
VOID	(*free)();
{
	char		*trace;
	char		*log;
	char		*driver = NULL;
	char		*mode = NULL;
	CL_ERR_DESC 	desc;
	TYPESIG		(*sigpipe)();
	TYPESIG		i_EXcatch();

	GCalloc = alloc;
	GCfree = free;

	NMgtAt( "II_GC_TRACE", &trace );
	if( !( trace && *trace ) && PMget( "!.gc_trace_level", &trace ) != OK )
		gc_trace = 0;
	else
		gc_trace = atoi( trace );

	if (PMget("!.batchmode", &driver) == OK)
	    STcopy(driver, batch_mode);
	else
	    STcopy("off", batch_mode);

	if (PMget("!.gca_single_mode", &mode) == OK)
	    STcopy(mode, gca_single_mode);
	else
	    STcopy("off", gca_single_mode);

	sigpipe = signal( SIGPIPE, SIG_IGN );
	
	if( sigpipe != SIG_DFL && sigpipe != i_EXcatch )
	    signal( SIGPIPE, sigpipe );

	GCdriver = GC_get_driver( NULL );

	return OK;
}
	

/*{
** Name: GCterminate    - Perform local system-dependent termination functions
**
** Description:
**
**	GCterminate is invoked by mainline code as the last operation done as
**	a GCA user is terminating its connection with GCA.  It may be used to
**	perform any final system-dependent functions which may be required,
**	such as cancelling system functions which are still in process, etc.
**
** Inputs:
**	svc_parms	    pointer to svc_parms parameter list
**
** Outputs:
**      none
**
**	Returns:
**      none
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      19-Jan-88 (jbowers)
**          Initial function implementation
*/
VOID
GCterminate(svc_parms)
SVC_PARMS	*svc_parms;
{	
	BS_PARMS	bsp;

	svc_parms->status = OK;

	if (iiunixlisten == TRUE) 
	{
	    bsp.buf = svc_parms->listen_id ;
	    bsp.lbcb = listenbcb;
	    bsp.syserr = svc_parms->sys_err;
	    if ( iisynclisten == FALSE )
		(*GCdriver->unlisten)( &bsp );
	}
}

