/*
**Copyright (c) 2006, 2009 Ingres Corporation
** NO_OPTIM=rmx_us5 rux_us5
*/

#include    <bzarch.h>
#if !defined(dg8_us5) && !defined(dgi_us5)
#include    <systypes.h>
#include    <pwd.h>
#endif

#include    <compat.h>
#include    <clconfig.h>
#include    <gl.h>
#include    <qu.h>
#include    <gc.h>
#include    <clpoll.h>
#include    <cs.h>
#include    <ex.h>
#include    <me.h>
#include    <lo.h>
#include    <er.h>
#include    <pm.h>
#include    <nm.h>
#include    <tr.h>
#include    <st.h>
#include    <errno.h>
#include    <diracc.h>
#include    <handy.h>

#if defined(dg8_us5) || defined(dgi_us5)
#include    <pwd.h>
#endif

# ifdef xCL_SUNOS_CMW
# define SunOS_CMW
# undef ulong
# include <cmw/tnet_attrs.h>
# include <cmw/sctnattrs.h>
# include <sys/label.h>
# endif

# if defined(hp8_bls)
# include <sys/sctnmasks.h>
# include <m6attrs.h>
# include <sys/sctnerrno.h>
# include <mandatory.h>
# endif /* hp8_bls */

#include    <bsi.h>

#include    "gcarw.h"
#include    "gcacli.h"

/*
** Forward and external references
*/

VOID GC_set_blocking( bool blocking_on );

/**
**
** Name: GCACONN.C - GCregister(), GClisten(), GCrequest(), GCrelease() 
**
**	See GCACL.C for list of source files.
**
** Description:
**	GCregister()...
**	
**	GCregister() is currently a simple wrapper for ii_BSregister(),
**	which takes a string representing the listen address and builds a
**	listen structure which can be handed to ii_BSlisten() to accept a
**	connection at that address.  Because ii_BSregister() and
**	ii_BSlisten() expect the listen structure as a parameter, and upper
**	level GCA doesn't provide for this structure, the structure,
**	"listenbcb," is stuck as static data in the GCregister() source.
**	
**	GCregister() is synchronous, while ii_BSregister() is asynchronous,
**	so GCregister() forces completion (the calling of GC_register_sm())
**	by looping on iiCLpoll().
**	
**	ii_BSregister() is responsible for parsing the listen address.  It
**	currently provides for UNIX domain sockets and INTERNET domain
**	sockets.
**	
**	--------------- 
**	
**	GClisten(), GCrequest()
**	
**	GClisten() and GCrequest() initiate the server and client 
**	connection, respectively.  They allocate a control structure and 
**	call ii_BSlisten() or ii_BSconnect() to perform the actual 
**	connection.
**	
**	The control structure is the "GCA_GCB," aka the GCA_ACB's local
**	control block.  This structure contains the connection's state
**	information from connection to closing.  This struture also
**	includes the BS_BCB, the BS layer's structure.  The GCA_GCB
**	has a pointer to a GCA_CCB, the connection control block.  The 
**	GCA_CCB is used only during connection, but because client code
**	gets pointers back into parts of the GCA_CCB for user_name,
**	account_name, and access_point_identifier, the GCA_CCB must
**	remain for the duration of the connection.
**	
**	Both GClisten() and GCrequest() list GC_connect_sm() as the
**	completion routine to ii_BSlisten() and ii_BSconnect().
**	GC_connect_sm() checks the status of the BS call and returns.
**	
**	--------------- 
**	
**	Peer info exchanges
**
**	Originally, peer info was exchanged through the GCsendpeer()
**	and GCrecvpeer() functions.  This permitted CL peer info to
**	be combined with GCA peer info.  This exchange occured both
**	client to server and server to client.
**
**	Now, the CL sends client peer info during GCrequest() to be
**	received by the server during GClisten().  There is no server
**	to client peer info exchange.  GCA exchanges peer info with
**	the first GCsend() and GCreceive() requests (send/receive for
**	client, receive/send for server).
**
**	For backward compatibility, GClisten() looks for a peer info
**	packet from the original exchange scenario.  When received,
**	CL peer info is extracted and the GCA peer info is saved and
**	passed to GCA on the first GCreceive() request.  Since the
**	client is expecting an old style peer info packet in return,
**	GCsend() combines the contents of the first send request with
**	the CL peer info before sending the message.
**
**	Backward compatibility for old servers is neither required
**	nor supported.
**
**  History:    
**	10-Sep-87 (berrow)
**	    First cut at UNIX port
**	13-Jan-89 (seiwald)
**	    Revamped to use CLpoll.
**	07-Jun-89 (seiwald)
**	    Added then commented out code to check for valid user name
**	    and passwords.  A bug in the IIGCC causes it to crash on failed
**	    connections.
**	13-jun-89 (seiwald)
**	    Reinstated password checking code.  Companion change to
**	    (get this) iiCLpoll(), to allow for reentrancy, makes this
**	    work now.
**	27-jun-89 (seiwald)
**	    Being a little more rigorous about checking the return from
**	    iiCLpoll().
**	04-Aug-89 (seiwald)
**	    New GCsendpeer and GCrecvpeer to allow mainline to exchange
**	    peer info.
**	04-Oct-89 (seiwald)
**	    GCsend, sendpeer, and recvpeer are all void.
**	11-Oct-89 (seiwald)
**	    Reinstate setting size_advise.
**	10-Nov-89 (seiwald)
**	    GCA_CONNECT now part of GCA_GCB, and structure definitions
**	    are in gcarw.h.
**
**	    Call to ii_BSbuffer moved up into GClisten and GCrequest, so
**	    as not to call it at completion time.  ii_BSbuffer allocates
**	    memory, and that is prohibited at completion time in the DBMS.
**
**	    GCdisconn moved out into sender state machine.
**	14-Nov-89 (seiwald)
**	    Don't call GCrelease on failure to connect.  GCA will call
**	    it directly.  The exception is still the bedcheck code, in 
**	    which a failed listen must not bother GCA.
**	30-Dec-89 (seiwald)
**	    Added GCsync - wait for sync request to complete.
**	31-Dec-89 (seiwald)
**	    Removed internal reposting of GClisten on failure to read
**	    peer info.  This was to avoid bothering mainline with Name
**	    Server GCdetect()'s.  Because GClisten allocates memory,
**	    it cannot occur at completion time.  Mainline must repost
**	    the failed GCA_LISTEN.
**	01-Jan-90 (seiwald)
**	    Removed setaside buffer.
**	19-Jan-90 (seiwald)
**	    Made call to GC_whoami cache user name/terminal, since this
**	    is not likely to change during the life of a process.
**	12-Feb-90 (seiwald)
**	    Remove synchronous support.  GCA as of 6.4 handles syncing
**	    async requests.
**	12-Feb-90 (seiwald)
**	    Rearranged and compacted GCA_GCB.  There is now only one buffer
**	    for mainline peer info, and GCrecvpeer borrows the svc_parms
**	    handed it, rather than having one of its own.
**	26-Feb-90 (seiwald)
**	    SVC_PARMS sys_err now a pointer (to CL_ERR_DESC structure in
**	    GCA's parmlist).
**	20-Mar-90 (seiwald)
**	    Use semaphores in 6.4 to modify global data.
**      24-May-90 (seiwald)
**	    Built upon new BS interface defined by bsi.h.
**	25-May-90 (seiwald)
**	    Posix wants #include <pwd.h> at top of file, so it can 
**	    wrongfully includes sys/types.h).
**	26-May-90 (seiwald)
**	    Revert GCdetect to being a bool.
**	28-May-90 (seiwald)
**	    GCdetect returns STATUS.
**	06-Jun-90 (seiwald)
**	    Made GCalloc() return the pointer to allocated memory.
**	13-Jun-90 (seiwald)
**	    Hand syserr to (*accept)() and (*connect)() - they were
**	    overlooked.
**	30-Jun-90 (seiwald)
**	    Struct mtyp now typedef GC_MTYP.
**	    Use GC_SIZE_ADVISE from gcarw.h instead of local GC_BUF_SIZE.
**	30-Jun-90 (seiwald)
**	    Private and Lprivate renamed to bcb and lbcb, as private
**	    is a reserved word somewhere.
**	4-Jul-90 (seiwald)
**	    Hard ports for listen is now II_GC_PORT, not II_DBMS_SERVER_PORT.
**	5-Jul-90 (seiwald)
**	    GClisten drives its completion routine, even on gcb alloc fail.
**	30-Aug-90 (gautam)
**	    Added bool unixlisten to indicate if this is a listen server.
**	    listenbcb is extern since it is used by GCterminate.
**	17-Sep-90 (anton)
**	    Use MT_CS symbol to identify code compiled for MCT
**	15-Oct-90 (seiwald)
**	    Made GC_whoami strip /dev/ from tty name and change name 
**	    of tty-less programs to "batch" (was limbo).
**	19-Nov-90 (seiwald)
**	    Include systypes.h for Bull.
**	2-Jan-91 (seiwald)
**	    GCsync() now calls EXinterrupt( EX_DELIVER ).
**	6-Apr-91 (seiwald)
**	    Be a little more assiduous in clearing svc_parms->status on
**	    function entry.
**	26-Dec-91 (seiwald)
**	    Support for installation passwords: set new flags.trusted
**	    in SVC_PARMS if GClisten determines that it's peer process
**	    has the same uid.
**	27-Mar-92 (GordonW)
**	    Added include to me.h
**	    Change a couple of MEcopy to STRUCT_ASSIGN_MACRO
**	9-Sep-92 (gautam)
**	    GCdetect needs to have a gcb allocated and the lbcb, bcb 
**	    fields filled in the bsp
**	04-Mar-93 (edg)
**	    GCdetect removed.
**      16-mar-93 (smc)
**          Fixed forward declarations.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	26-jul-93 (mikem)
**	    Changed declaration of listenbcb to make sure the buffer is
**	    alligned, so that when it is treated as a structure access
**	    is properly alligned.  Before this change TCP code on su4_us5 
**	    would sometimes bus-error due to unalligned access.
**	30-mar-93 (robf)
**	    Secure 2.0: Remove xORANGE, add handling for security labels
**      11-feb-1994 (ajc)
**          Add handling for security labels of hp8_bls.
**      28-mar-1994 (ajc)
**          Hp-bls - due to OS upgrade, m6attrs.h has moved up from sys.
**      28-Dec-1994 (canor01)
**          make call to GC_check_uid in GCregister. The internal 
**          structures need to be initialized before first GCrequest
**          if it is a nonames server
**      25-apr-1995 (canor01)
**          verify the protocol on a GCrequest by calling GC_get_driver()
**      05-jun-1995 (canor01)
**          semaphore protect memory calls
**	03-jun-1996 (canor01)
**	    iiCLgetpwuid() needs to be passed a pointer to a passwd struct.
**	    Memory calls with operating-system threads no longer need
**	    semaphore protection.
**	16-jul-1996 (canor01)
**	    Add completing_mutex to GCA_GCB to protect against simultaneous
**	    access to the completing counter with operating-system threads.
**	10-oct-1996 (canor01)
**	    Add GC_set_blocking() function to determine whether to use
**	    blocking I/O or not, instead of depending on the value of the
**	    server class.
**	14-Nov-96 (gordy)
**	    Added timeout value to GCsync().
**      01-Apr-97 (bonro01)
**          System includes before <compat.h> and
**          <bzarch.h> caused problems because defines for
**          POSIX thread support did not get defined before
**          system header files were included.
**      01-Apr-1997 (mosjo01) 
**          Extended the fix dated 1-apr-1997 by bonro01 to dgi_us5.
**	 9-Apr-97 (gordy)
**	    GCsendpeer() and GCrecvpeer() are no more, RIP.  Cl peer info
**	    now exchanged during GCrequest() and GClisten().  Backward
**	    compatibility is provided through detection of old style peer
**	    info packets and special handling in GCsend() and GCreceive().
**	10-Apr-97 (gordy)
**	    ACB removed from service parms, replaced by GC control block.
**	    GCA callback now takes closure from service parms.
**	15-Apr-97 (gordy)
**	    Implemented new request association data structure.
**	21-apr-1997 (canor01)
**	    In GCrequest(), move initialization of bsp->regop field 
**	    before call to GCdriver->request.
**	22-Apr-97 (gordy)
**	    Reduced the restrictions on validating peer info.  To
**	    allow for future extensions we only need to ensure we 
**	    get the basic data set and no need to check version.
**	07-jul-1997 (canor01)
**	    Remove completing_mutex, which was needed only for BIO
**	    threads handling completions for other threads as proxy.
**	09-24-97 (rajus01)
**	    Added new defines for remote access. Added check for
**	    client connections from a remote host bypassing Ingres/Net.
**	12-Jan-98 (gordy)
**	    Host name added to GCrequest() service parms for direct 
**	    network connections.
**	30-Jan-98 (gordy)
**	    Added GC_RQ_ASSOC1 for direct remote connections.
**	 4-Jun-98 (gordy)
**	    Check for timeout of GClisten() request.
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**      03-jul-99 (podni01)
**          Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**      07-jul-1999 (kosma01)
**          Set NO_OPT for rmx_us5. Symptom is gaa17.sep of the
**          loopback tests stalling on an sql copy statement.
**          Caused iigcc to stop responding or transmitting data.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	 5-May-03 (gordy)
**	    Effective UID can change at run-time.  Reload user ID
**	    when euid changes.
**	16-Jan-04 (gordy)
**	    Replaced GC_NO_PEER (which is reserved for GCA) with 
**	    GC_ASSOC_FAIL.
**	 7-Oct-06 (lunbr01)  Sir 116548
**	    Change separator between host and port from colon (:)
**	    to semicolon (;) since colon is valid in IPv6 addresses.
**	21-Aug-08 (lunbr01)  Bug 120802
**	    Add connect retry logic for IPv6 to handle multiple addresses
**	    to avoid local connection failures on some OS's (such as
**	    Fedora Linux).
**	15-Jul-09 (gordy)
**	    New peer association structure to allow for longer variable 
**	    length names.
**	15-nov-2010 (stephenb)
**	    Proto GC_set_blocking().
**/

/*
** This structure is  used to store user name/label mappings from PM.
** This assumes a small number of users/labels so uses a simple  linked
** list currently. This is necessary since currently PMget can't be used
** directly within the GCA listen completion routine.
*/
typedef struct _user_label  {
	char *user_name;
	char *security_label;
	struct _user_label  *next;

} USER_LABEL;

static USER_LABEL	*ul_root = NULL;
static uid_t		saved_euid = -1;

/* Defines for Remote Access */
 
#define		GC_RA_DISABLED          (-1)
#define		GC_RA_UNDEFINED         0
#define		GC_RA_ENABLED           1
#define		GC_RA_ALL               2

static VOID	GC_listen_sm( SVC_PARMS *, STATUS );
static VOID	GC_recvpeer_sm( PTR );
static VOID	GC_request_bs( SVC_PARMS * );
static VOID	GC_request_sm( SVC_PARMS * );
static GCA_GCB	*GC_initlcb( VOID );
static VOID	GC_whoami( VOID );
static STATUS 	GC_check_uid( SVC_PARMS	*svc_parms );

GLOBALREF	BS_DRIVER	*GCdriver;
FUNC_EXTERN	BS_DRIVER	*GC_get_driver();


/*
** Name: listenbcb
**
** Description:
**	A botch.  svc_parms has room for a gcb (gc control block)
**	but no listen control block.  So we have makeshift one here.
**	This thing must be alligned so that when the buffer is treated as
**	as structure, access to members is alligned properly.
*/

ALIGN_RESTRICT listenbcb[ BS_MAX_LBCBSIZE / sizeof( ALIGN_RESTRICT ) ];

GLOBALDEF bool		iiunixlisten = FALSE;	/* is this a listen server ? */
GLOBALDEF bool		iisynclisten = FALSE;

static GC_RQ_ASSOC	gc_assoc_info ZERO_FILL;
static char		*gc_host_name = "";
static char		*gc_user_name = "";
static i4		remote_access = GC_RA_UNDEFINED;


/*{                                                                             
** Name: GCregister - Server start-up notification                            
**                                                                              
** Description:
**	See comments at head of file.
**
** Called by:
**	GCA mainline.
**
** History:
**	21-Apr-97 (gordy)
**	    Check to see if remote access is permited.
**	24-Sep-97 (rajus01)
**	    Modified the above check added by Gordy. 
**	30-Jan-98 (gordy)
**	    Make sure disabled is default remote access mode.
*/

VOID
GCregister( SVC_PARMS *svc_parms )
{
    char	*env= NULL;
    BS_PARMS	bsp;

    svc_parms->status = OK;
    svc_parms->listen_id = 0;

    if ( remote_access == GC_RA_UNDEFINED )
    {
    	NMgtAt( "II_GC_REMOTE", &env );
    	if ( ! env  ||  ! *env )
	    remote_access = GC_RA_DISABLED;
	else if ( ! STcasecmp( env, "ON" )  || 
		  ! STcasecmp( env, "ENABLED" ) )
	    remote_access = GC_RA_ENABLED;
	else if ( ! STcasecmp( env, "FULL" )  || 
		  ! STcasecmp( env, "ALL" ) )
	    remote_access = GC_RA_ALL;
	else
	    remote_access = GC_RA_DISABLED;
    }
	    
    /* get address */

    NMgtAt( "II_GC_PORT", &env );

    /* let BS listen do the work */

    bsp.lbcb = (PTR) listenbcb;
    bsp.syserr = svc_parms->sys_err;
    bsp.buf = env ? env : "";

    GCTRACE(1)( "GCregister: registering at %s\n", bsp.buf );

    (*GCdriver->listen)( &bsp );

    if ( bsp.status != OK )
    {
	GCTRACE(1)( "GCregister: failed status %x\n", bsp.status );
	svc_parms->status = bsp.status;
	return;
    }

    GCTRACE(1)( "GC_register: registered at %s\n", bsp.buf );

    iiunixlisten = TRUE;
    svc_parms->listen_id = bsp.buf;

    /* 
    ** make the call to GC_check_uid() to initialize structure
    ** even if it isn't used right now
    */
    GC_check_uid( svc_parms );

    return;
}

/*{                                                                             
** Name: GClisten - Establish a server connection with a client               
**                                                                              
** Description:                                                                 
**	See description at head of file.
**
** Called by:
**	GCA mainline.
**
** History:
**      24-Oct-90 (pholman)
**        Setup svc_parms security label for B1 INGRES systems.
**	30-mar-93 (robf)
**	  Remove xORANGE, always initialize security label
**	10-Apr-97 (gordy)
**	    ACB removed from service parms, replaced by GC control block.
**	    GCA callback now takes closure from service parms.
*/

VOID
GClisten( SVC_PARMS *svc_parms )
{
	GCA_GCB		*gcb;
	static int	counter = 0;	/* const */ /* just for id'ing */
	BS_PARMS	bsp;
	static          bool   init=FALSE;

	/* Sanity & Protocol check */

	svc_parms->status = OK;
	svc_parms->size_advise = GC_SIZE_ADVISE - sizeof( GC_MTYP );
		
	/* Get user/process name */

	GC_whoami();

	/* new connection, alloc & init an lcb */

	if ( ! (gcb = GC_initlcb()) )
	{
	    svc_parms->status = GC_LSN_RESOURCE;
	    (*svc_parms->gca_cl_completion)( svc_parms->closure );
	    return;
	}

	svc_parms->gc_cb = (PTR)gcb;
	gcb->id = ( counter += 2 );
	gcb->islisten = TRUE;

        svc_parms->sec_label = (char *)&gcb->sec_label;

	/* register for accept callback with BS regfd */

	bsp.bcb = gcb->bcb;
	bsp.lbcb = (PTR) listenbcb;
	bsp.func = GC_listen_sm;
	bsp.closure = (PTR)svc_parms;
	bsp.syserr = svc_parms->sys_err;
	bsp.timeout = svc_parms->time_out;
	svc_parms->time_out = -1;	/* Don't timeout subsequent ops */
	bsp.regop = BS_POLL_ACCEPT;

	GCTRACE(1)( "GClisten %d: listening\n", gcb->id );

# ifdef OS_THREADS_USED
	if ( iisynclisten )
	    bsp.regop = BS_POLL_INVALID;   /* force blocking */
# endif /* OS_THREADS_USED */
	if( (*GCdriver->regfd)( &bsp ) )
		return;

	/* if registration unncessary, continue directly */

	(*bsp.func)( bsp.closure );
}

/*
** Name: GC_listen_sm
**
** History:
**      12-apr-90 (pholman)
**        Added the xORANGE code.
**	30-mar-93 (robf)
**	  Removed xORANGE, make security label handling generic
**	12-nov-93 (robf)
**        Initialize bsp.buf to 0 so we don't pick up the remote node
**	  name for local connections.
**	 9-Apr-97 (gordy)
**	    Peer info now received during GClisten().  Only flows client
**	    to server.
**	10-Apr-97 (gordy)
**	    ACB removed from service parms, replaced by GC control block.
**	    GCA callback now takes closure from service parms.
**	24-Sep-97 (rajus01)
**	    Set the "is_remote" flag in BS_PARMS.
**	 4-Jun-98 (gordy)
**	    Check for timeout.
**	09-Sep-2002 (hanje04)
**	    Bug 108675
**	    In GC_listen_sm, when initializing bsp, we must explicity
**	    set bsp.regopt = BS_POLL_ACCEPT if iisyclisten = FALSE 
**	    to avoid a potential blocking senario on Linux when
**	    accepting the connection in iisock_accept(). See Linux
**	    man page for accept(2) for details.
*/

static VOID
GC_listen_sm( SVC_PARMS *svc_parms, STATUS timeout_status )
{
    GCA_GCB	*gcb = (GCA_GCB *)svc_parms->gc_cb;
    BS_PARMS	bsp;

    if ( ! gcb )
    {
	(*svc_parms->gca_cl_completion)( svc_parms->closure );
	return;
    }

    if ( timeout_status != OK )
    {
	svc_parms->status = GC_TIME_OUT;
	(*svc_parms->gca_cl_completion)( svc_parms->closure );
	return;
    }

    /* 
    ** Pick up connection with BS accept 
    */
    bsp.bcb = gcb->bcb;
    bsp.lbcb = (PTR) listenbcb;
    bsp.syserr = svc_parms->sys_err;
    bsp.timeout = -1;
    bsp.buf = NULL;
    bsp.len = 0;
    bsp.is_remote = FALSE;

# ifdef OS_THREADS_USED
    if ( iisynclisten )  
    	bsp.regop = BS_POLL_INVALID;	/* force blocking */
    else
    	bsp.regop = BS_POLL_ACCEPT;
# endif /* OS_THREADS_USED */
    (*GCdriver->accept)( &bsp );

    GCTRACE(1)( "GC_listen_sm %d : status %x\n", gcb->id, bsp.status );
    if( bsp.status == OK )  
    {
    	GCTRACE(1)( "GC_listen_sm %d: Connection is %s\n", gcb->id, 
			bsp.is_remote ? "remote" : "local" );
    }
    else
    {
	svc_parms->status = bsp.status;
	(*svc_parms->gca_cl_completion)( svc_parms->closure );
	return;
    }

    /*
    ** Receive the GC peer info from partner.  Hijack svc_parms.
    */
    svc_parms->flags.flow_indicator = 0;
    if ( bsp.is_remote )  gcb->ccb.flags |= GC_IS_REMOTE;
    svc_parms->reqd_amount = sizeof( gcb->ccb.assoc_info );
    svc_parms->svc_buffer = (char *)&gcb->ccb.assoc_info;

    gcb->ccb.save_completion = svc_parms->gca_cl_completion;
    gcb->ccb.save_closure = svc_parms->closure;
    svc_parms->gca_cl_completion = GC_recvpeer_sm;
    svc_parms->closure = (PTR)svc_parms;

    GCreceive( svc_parms );

    return;
}

/*
** Name: GC_recvpeer_sm
**
** History:
**	 9-Apr-97 (gordy)
**	    GCrecvpeer() is no more, now part of GClisten.  Check for
**	    old style peer info packet; process and set flags for GCsend()
**	    and GCreceive() to provide backward compatibility.
**	10-Apr-97 (gordy)
**	    ACB removed from service parms, replaced by GC control block.
**	    GCA callback now takes closure from service parms.
**	15-Apr-97 (gordy)
**	    Implemented new request association data structure.
**	22-Apr-97 (gordy)
**	    Reduced the restrictions on validating peer info.  To
**	    allow for future extensions we only need to ensure we 
**	    get the basic data set and no need to check version.
**	24-Sep-97 (rajus01)
**	    Added check for client connections from a remote host
**	    bypassing Ingres/Net.
**	30-Jan-98 (gordy)
**	    Added GC_RQ_ASSOC1 for direct remote connections.
**	16-Jan-04 (gordy)
**	    Replaced GC_NO_PEER (which is reserved for GCA) with 
**	    GC_ASSOC_FAIL.
**	15-Jul-09 (gordy)
**	    New peer info structure for longer variable length names.
*/

static VOID
GC_recvpeer_sm( PTR closure )
{
    SVC_PARMS	*svc_parms = (SVC_PARMS *)closure;
    GCA_GCB	*gcb = (GCA_GCB *)svc_parms->gc_cb;
    STATUS	local_status;

    /* 
    ** Reset svc_parms 
    */
    svc_parms->gca_cl_completion = gcb->ccb.save_completion;
    svc_parms->closure = gcb->ccb.save_closure;

    GCTRACE(2)( "GC_recvpeer_sm %d: status %x\n", gcb->id, svc_parms->status );

    for(;;)
    {
	if ( svc_parms->status != OK )
	{
	    svc_parms->status = GC_ASSOC_FAIL;
	    break;
	}

	GCTRACE(2)( "GC_recvpeer_sm %d: received length %d\n",
		    gcb->id, svc_parms->rcv_data_length );

	/*
	** Original peer info is identified by the data length.
	** Subsequent peer info versions are guaranteed not to
	** have this same length (see GC_request_sm()).
	*/
	if (svc_parms->rcv_data_length != sizeof(gcb->ccb.assoc_info.orig_info))
	{
	    /*
	    ** The first three peer info fields are identical
	    ** in all peer versions other than the original.
	    ** Identify and validate peer info version.
	    */
	    GC_RQ_ASSOC2	*peer2 = &gcb->ccb.assoc_info.rq_assoc.v2;
	    char	 	*host, *user, *tty;
	    u_i4		flags = 0;

	    if ( peer2->length > svc_parms->rcv_data_length )
	    {
		GCTRACE(1)( "GC_recvpeer_sm %d: invalid peer length: %d\n", 
			    gcb->id, peer2->length );
		svc_parms->status = GC_USRPWD_FAIL;
		break;
	    }

	    if ( peer2->id != GC_RQ_ASSOC_ID )
	    {
		GCTRACE(1)( "GC_recvpeer_sm %d: invalid peer info ID: 0x%x\n", 
			    gcb->id, peer2->id );
		svc_parms->status = GC_USRPWD_FAIL;
		break;
	    }

	    GCTRACE(2)( "GC_recvpeer_sm %d: peer info v%d: length %d\n",
	    		gcb->id, peer2->version, peer2->length );

	    switch( peer2->version )
	    {
	    case GC_ASSOC_V0 :
	    {
		GC_RQ_ASSOC0	*peer0 = &gcb->ccb.assoc_info.rq_assoc.v0;

		if ( peer0->length < sizeof( GC_RQ_ASSOC0 ) )
		{
		    GCTRACE(1)( "GC_recvpeer_sm %d: invalid peer v0 len: %d\n",
				gcb->id, peer0->length );
		    svc_parms->status = GC_USRPWD_FAIL;
		    break;
		}

		host = peer0->host_name;
		user = peer0->user_name;
		tty = peer0->term_name;
	    	break;
	    }
	    case GC_ASSOC_V1 :
	    {
		GC_RQ_ASSOC1	*peer1 = &gcb->ccb.assoc_info.rq_assoc.v1;

		if ( peer1->rq_assoc0.length < sizeof( GC_RQ_ASSOC1 ) )
		{
		    GCTRACE(1)( "GC_recvpeer_sm %d: invalid peer v1 len: %d\n",
				gcb->id, peer1->rq_assoc0.length );
		    svc_parms->status = GC_USRPWD_FAIL;
		    break;
		}

		host = peer1->rq_assoc0.host_name;
		user = peer1->rq_assoc0.user_name;
		tty = peer1->rq_assoc0.term_name;
		flags = peer1->flags;
		break;
	    }
	    case GC_ASSOC_V2 :
	    {
		/*
		** Host, user, and terminal names follow, in that
		** order, the fixed portion of peer info structure.
		*/
		i4 length = sizeof( GC_RQ_ASSOC2 ) + peer2->host_name_len +
			    peer2->user_name_len + peer2->term_name_len;

		if ( peer2->length < length )
		{
		    GCTRACE(1)( 
		    "GC_recvpeer_sm %d: invalid peer v2 len: %d (%d,%d,%d)\n",
				gcb->id, peer2->length, peer2->host_name_len,
				peer2->user_name_len, peer2->term_name_len );
		    svc_parms->status = GC_USRPWD_FAIL;
		    break;
		}

		host = (char *)peer2 + sizeof( GC_RQ_ASSOC2 );
		user = host + peer2->host_name_len;
		tty = user + peer2->user_name_len;
		flags = peer2->flags;
		break;
	    }
	    default :
		GCTRACE(1)( "GC_recvpeer_sm %d: unsupported peer version: %d\n",
			    gcb->id, peer2->version );
		svc_parms->status = GC_USRPWD_FAIL;
		break;
	    }

	    if ( svc_parms->status != OK )  break;

	    GCTRACE(2)
		( "GC_recvpeer_sm %d: flags 0x%x, host %s, user %s, tty %s\n",
		  gcb->id, flags, host, user, tty );

	    svc_parms->user_name = user;
	    svc_parms->account_name = user;
	    svc_parms->access_point_identifier = tty;

	    /*
	    ** Clients are only trusted if they are on the
	    ** same host and have the same user ID.
	    **
	    ** If remote, check to see if remote access is
	    ** disabled or restricted.
	    */
	    if ( ! (gcb->ccb.flags & GC_IS_REMOTE) )
	    {
		if ( STcompare( host, gc_host_name ) )
		{
		    GCTRACE(1)( "GC_recvpeer_sm %d: invalid host: %s!\n", 
				gcb->id, host );
		    svc_parms->status = GC_RMTACCESS_FAIL;
		    break;
		}

		svc_parms->flags.trusted = ! STcompare( user, gc_user_name );
	    }
	    else  if ( remote_access != GC_RA_ALL  && 
		       remote_access != GC_RA_ENABLED )
	    {
		/*
		** Remote access through GC is not allowed.
		*/
		GCTRACE(1)( "GC_recvpeer_sm %d: remote access not enabled!\n", 
			    gcb->id );
		svc_parms->status = GC_RMTACCESS_FAIL;
		break;
	    }
	    else  if ( remote_access == GC_RA_ENABLED  &&
	    	       ! (flags & GC_RQ_REMOTE) )
	    {
		/*
		** Uncontrolled remote access is not allowed.
		*/
		GCTRACE(1)( "GC_recvpeer_sm %d: invalid remote GC access\n", 
			    gcb->id );
		svc_parms->status = GC_RMTACCESS_FAIL;
		break;
	    }
	}
	else
	{
	    /*
	    ** This is an old style peer info packet from the
	    ** GC{send,recv}peer() days which combines CL and
	    ** GCA information.  Extract our info.  Set the
	    ** flags indicating that GCA info is available for
	    ** the next receive and that the next send must
	    ** provide backward compatibility by combining CL
	    ** and GCA info.
	    */
	    GC_OLD_ASSOC *peer = &gcb->ccb.assoc_info.orig_info.gc_info;

	    gcb->ccb.flags |= GC_PEER_RECV | GC_PEER_SEND;
	    svc_parms->user_name = peer->user_name;
	    svc_parms->account_name = peer->account_name;
	    svc_parms->access_point_identifier = peer->access_point_id;

	    GCTRACE(2)( "GC_recvpeer_sm %d: user %s, tty %s\n",
			gcb->id, svc_parms->user_name,
			svc_parms->access_point_identifier );

	    if ( ( gcb->ccb.flags & GC_IS_REMOTE ) && 
				remote_access != GC_RA_ALL )
	    {
		/*
		** Remote access through GC is not allowed.
		*/
		GCTRACE(1)
		    ( "GC_recvpeer_sm %d: remote access not allowed!\n", 
		      gcb->id );
		svc_parms->status = GC_RMTACCESS_FAIL;
		break;
	    }
	}

	/*
	** Check claimed user name against IPC channel user name.
	*/
	if ( GC_check_uid( svc_parms ) != OK )
	{
	    svc_parms->status = GC_USRPWD_FAIL;
	    break;
	}

	break;
    }

    /* 
    ** Drive completion 
    */
    GCTRACE(1)( "GClisten completes %d: status %x\n",
		gcb->id, svc_parms->status );
	
    (*svc_parms->gca_cl_completion)( svc_parms->closure );

    return;
}


/*{                                                                             
** Name: GCrequest() - Build connection to peer entity
**                                                                              
** Description:
**	See comments at head of file.
**
** Called by:
**	GCA mainline
**
** History:
**	10-Apr-97 (gordy)
**	    ACB removed from service parms, replaced by GC control block.
**	    GCA callback now takes closure from service parms.
**	21-apr-1997 (canor01)
**	    Move initialization of bsp->regop field before call
**	    to GCdriver->request.
**	12-Jan-98 (gordy)
**	    Host name added to service parms for direct network connections.
**	 7-Oct-06 (lunbr01)  Sir 116548
**	    Change separator between host and port from colon (:)
**	    to semicolon (;) since colon is valid in IPv6 addresses.
*/

VOID
GCrequest( SVC_PARMS *svc_parms )
{
	GCA_GCB		*gcb;
	static int	counter = -1;  /* const */

	/* Sanity & Protocol check */

	svc_parms->status = OK;
	svc_parms->size_advise = GC_SIZE_ADVISE - sizeof( GC_MTYP );

	/* Get user/process name */

	GC_whoami();

	/* verify the protocol */
	GCdriver = GC_get_driver( svc_parms->partner_name );

	/* new connection, alloc & init an lcb */

	if( !( gcb = GC_initlcb() ) )
	{
		svc_parms->status = GC_RQ_RESOURCE;
		return;
	}

	svc_parms->gc_cb = (PTR)gcb;
	gcb->id = ( counter += 2 );
	gcb->islisten = FALSE;

	/* make outgoing request with BS request */

	GC_request_bs( svc_parms );
	return;
}

/*
** Name: GC_request_bs
**                                                                              
** Description:
**	Make outgoing request with BS request.
**
** Called by:
**	GCrequest(), GC_request_sm().
**
** History:
**	21-Aug-08 (lunbr01)  Bug 120802
**	    Extracted from bottom half of GCrequest() so that it could
**	    also be called from GC_request_sm() to try next IPv6 address,
**	    indicated by status BS_INCOMPLETE.
*/

static VOID
GC_request_bs( SVC_PARMS *svc_parms )
{
	GCA_GCB		*gcb = (GCA_GCB *)svc_parms->gc_cb;
	BS_PARMS        bsp;

	bsp.bcb = gcb->bcb;
	bsp.lbcb = (PTR) listenbcb;
	bsp.func = GC_request_sm;
	bsp.closure = (PTR)svc_parms;
	bsp.syserr = svc_parms->sys_err;
	bsp.timeout = svc_parms->time_out;
	bsp.buf = svc_parms->partner_name;

	/*
	** If a host name was provided which 
	** is different than our current host, 
	** combine host name and address.
	*/
	if ( svc_parms->partner_host  &&  *svc_parms->partner_host )
	{
	    GChostname( gcb->buffer, sizeof( gcb->buffer ) );

	    if ( STcompare( svc_parms->partner_host, gcb->buffer ) )
		bsp.buf = STpolycat( 3, svc_parms->partner_host, ";",
				     svc_parms->partner_name, gcb->buffer );
	}

# ifdef OS_THREADS_USED
	if ( iisynclisten )
	    bsp.regop = BS_POLL_INVALID;   /* force blocking */
	else
	    bsp.regop = BS_POLL_CONNECT;
# else /* OS_THREADS_USED */
	bsp.regop = BS_POLL_CONNECT;
# endif /* OS_THREADS_USED */

	GCTRACE(1)( "GCrequest_bs %d: connecting on %s\n", gcb->id, bsp.buf );
		
	(*GCdriver->request)( &bsp );

	/* check status */
        
	GCTRACE(1)( "GCrequest_bs %d: status %x\n", gcb->id, bsp.status );

	if( bsp.status != OK )
	{
		svc_parms->status = bsp.status;
		(*svc_parms->gca_cl_completion)( svc_parms->closure );
		return;
	}

	/* register for connect callback with BS regfd */

	if( (*GCdriver->regfd)( &bsp ) )
		return;

	/* if registration unncessary, continue directly */

	(*bsp.func)( bsp.closure );
}

/*
** Name: GC_request_sm
**
** History:
**	 9-Apr-97 (gordy)
**	    Peer info now exchanged during GCrequest().  Only flows
**	    client to server.
**	10-Apr-97 (gordy)
**	    ACB removed from service parms, replaced by GC control block.
**	    GCA callback now takes closure from service parms.
**	30-Jan-98 (gordy)
**	    Added GC_RQ_ASSOC1 for direct remote connections.
**	21-Aug-08 (lunbr01)  Bug 120802
**	    Add connect retry logic for IPv6 to handle multiple addresses
**	    to avoid local connection failures on some OS's (such as
**	    Fedora Linux).
**	15-Jul-09 (gordy)
**	    New peer info version to allow longer variable length names.
**	    Make sure new peer info data is not same size as orig peer
**	    info to ensure receiver is not confused.  
*/

static VOID
GC_request_sm( SVC_PARMS *svc_parms )
{
    GCA_GCB		*gcb = (GCA_GCB *)svc_parms->gc_cb;
    BS_PARMS		bsp;

    /* 
    ** pick up connection with BS connect 
    */
    bsp.bcb = gcb->bcb;
    bsp.lbcb = (PTR) listenbcb;
    bsp.syserr = svc_parms->sys_err;
    bsp.timeout = -1;

    (*GCdriver->connect)( &bsp );

    GCTRACE(1)( "GC_request_sm %d: status %x\n", gcb->id, bsp.status );
    if( bsp.status == BS_INCOMPLETE )
    {
	GCTRACE(1)( "GC_request_sm %d: connect failed but more addresses to try\n", gcb->id );
	GC_request_bs( svc_parms );
	return;
    }
    if ( bsp.status != OK )  
    {
	svc_parms->status = bsp.status;
	(*svc_parms->gca_cl_completion)( svc_parms->closure );
	return;
    }

    /*
    ** Send the GC peer info to partner.  Hijack svc_parms.  
    ** Must allow room for MTYP at head of buffer.
    */
    svc_parms->time_out = -1;	/* Don't timeout subsequent ops */
    svc_parms->flags.flow_indicator = 0;
    svc_parms->svc_buffer = gcb->buffer + sizeof( GC_MTYP );

    if ( ! svc_parms->partner_host  ||  ! *svc_parms->partner_host )
    {
	/*
	** Local connection.  Peer association information is already
	** built (see GC_whoami()).  Latest version is always sent.
	*/
	svc_parms->svc_send_length = gc_assoc_info.v2.length;
	MEcopy( (PTR)&gc_assoc_info, 
		svc_parms->svc_send_length, (PTR)svc_parms->svc_buffer );
    }
    else
    {
	/*
	** Direct remote connection.  Need to set the REMOTE flag,
	** which can't be done directly in the pre-built peer info
	** used for local connections.
	*/
	GC_RQ_ASSOC	rq_assoc;

	MEcopy( (PTR)&gc_assoc_info, gc_assoc_info.v2.length, (PTR)&rq_assoc );
	rq_assoc.v2.flags = GC_RQ_REMOTE;
	svc_parms->svc_send_length = rq_assoc.v2.length;
	MEcopy( (PTR)&rq_assoc, 
		svc_parms->svc_send_length, (PTR)svc_parms->svc_buffer );
    }

    /*
    ** Detection of the original peer info structure relies on the
    ** structure length (see GC_recvpeer_sm()).  If the current peer
    ** structure happens to be the same size, add one additional 
    ** (garbage) byte to avoid confusion.
    */
    if ( svc_parms->svc_send_length == sizeof( GC_PEER_ORIG ) )  
    	svc_parms->svc_send_length++;

    GCsend( svc_parms );

    return;
}


/*{                                                                             
** Name: GCrelease() - Release buffers used by connection.
**                                                                              
** Description:
**	Frees the buffers that GC_initlcb() creates for GClisten()
**	and GCrequest().  Assumes the connection has gone through
**	GCdisconn().
**
** Called by:
**	GCA mainline
**
** History:
**	10-Apr-97 (gordy)
**	    ACB removed from service parms, replaced by GC control block.
*/

VOID
GCrelease( SVC_PARMS *svc_parms )
{
    GCA_GCB *gcb = (GCA_GCB *)svc_parms->gc_cb;

    svc_parms->status = OK;

    if ( gcb )
    {
	GCTRACE(2)( "GCrelease: run/active recv %s, send %s\n",
		    gcb->recv.running ? "running" : "not runnning",
		    gcb->send.running ? "running" : "not runnning" );

	(*GCfree)( (PTR)gcb );
	svc_parms->gc_cb = NULL;
    }

    return;
}


/* 
** Name: GC_initlcb() - create a GCA_GCB and return it
**
** Called by:
**	GClisten(), GCrequest(), GCdetect().
*/ 

static GCA_GCB *
GC_initlcb( VOID )
{
	GCA_GCB *gcb;

	if( !( gcb = (GCA_GCB *)(*GCalloc)( sizeof( *gcb ) ) ) )
		return (GCA_GCB *)0;

	MEfill( sizeof( *gcb ), 0, (PTR)gcb );

	return gcb;
}


/*
** Name: GC_whoami() - fill in peer info struct with user ID
**
** Description:
**	Pretends to fill in struct with user id and tty.
**	A botch from the old implementation.  Some other part of
**	the CL should do this.
**
** Called by:
**	GClisten()
**	GCrequest()
**
** History:
**	10-jul-89 seiwald
**	    Use geteuid() so client identifies itself as the effective,
**	    not real, user id.  bug 6932.
**	27-Dec-91 (seiwald)
**	    Now just initializes the global structure, and lets other
**	    code copy the pieces out.
**	30-may-95 (tutto01)
**	    Changed getpwuid call to the reentrant iiCLgetpwuid.
**	03-jun-1996 (canor01)
**	    iiCLgetpwuid() needs to be passed a pointer to a passwd struct.
**	15-Apr-97 (gordy)
**	    Implemented new request association data structure.
**	 5-May-03 (gordy)
**	    Effective UID can change at run-time.  Check for change and
**	    re-initialize.
**	15-Jul-09 (gordy)
**	    New peer info version now allows for long variable length names.
*/

static VOID
GC_whoami( VOID )
{
    GC_RQ_ASSOC2	*rq_assoc = &gc_assoc_info.v2;
    char		*out, *name;
    i4			avail, len;
    struct passwd	*pw, pwd;
    char		pwuid_buf[512];
    char 		*ttyname(); 
    uid_t		euid = geteuid();

    /*
    ** Do this only once (on first listen or request).
    ** Lock out anyone else by initializing length.
    ** If effective uid changes, need to reinitialize.
    */
    if ( rq_assoc->length  &&  euid == saved_euid )  return;

    rq_assoc->length = sizeof( *rq_assoc );
    rq_assoc->id = GC_RQ_ASSOC_ID;
    rq_assoc->version = GC_ASSOC_V2;
    rq_assoc->flags = 0;
    rq_assoc->host_name_len = 0;
    rq_assoc->user_name_len = 0;
    rq_assoc->term_name_len = 0;
    saved_euid = euid;

    /*
    ** rq_assoc->length is the amount of buffer currently used.
    ** Output position is GC_RQ_ASSOC buffer offset by current used.
    ** The available length is GC_RQ_ASSOC buffer length less current used.
    */
    out = (char *)gc_assoc_info.buffer + rq_assoc->length;
    avail = sizeof( gc_assoc_info.buffer ) - rq_assoc->length;

    /*
    ** Save the host name.
    */
    gethostname( out, avail );

    out[ avail - 1 ] = EOS;		/* EOS not guaranteed if truncated */
    len = STlength( out ) + 1;		/* include EOS */
    rq_assoc->length += len;
    rq_assoc->host_name_len = len;
    gc_host_name = out;			/* save for comparing inbound info */
    out += len;
    avail -= len;

    if ( avail > 0 )
    {
	/* 
	** Who are we ? -- use effective id 
	*/
	pw = iiCLgetpwuid( euid, &pwd, pwuid_buf, sizeof( pwuid_buf ) );

	name = pw ? pw->pw_name : "nobody";
	len = STlength( name );
	len = min( len, avail - 1 );	/* leave room for EOS */
	MEcopy( name, len, out );
	out[ len++ ] = EOS;		/* include EOS */
	rq_assoc->length += len;
	rq_assoc->user_name_len = len;
	gc_user_name = out;		/* save for comparing inbound info */
	out += len;
	avail -= len;
    }

    if ( avail > 0 )
    {
	/*
	** Save our associated device name.
	*/
	name = ttyname( 0 /* stdin */ );

	if ( ! name )
	    name = "batch";
	else  if ( ! STscompare( name, 5, "/dev/", 5 ) )  
	    name += 5;		/* strip /dev/ from tty name */

	len = STlength( name );
	len = min( len, avail - 1 );	/* leave room for EOS */
	MEcopy( name, len, out );
	out[ len++ ] = EOS;		/* include EOS */
	rq_assoc->length += len;
	rq_assoc->term_name_len = len;
    }

    return;
}


/*
** Name: GCsync - wait for sync request to complete
**
** Description:
**	GCsync is called repeatedly while GCA mainline waits for
**	the last posted GCA CL request to complete.  If the
**	implementation requires, GCsync may poll for lower level 
**	I/O operations; otherwise, GCsync should block and return
**	when the CL request completes.
**
**	GCsync may return before the last posted request completes,
**	and will be reinvoked by GCA.  For example, it may return 
**	after each lower level I/O operation.
**
**	GCsync must also allow keyboard generated interrupts to
**	occur, but not while completions are being driven.
**
** Input:
**	timeout		Timeout value in milli-seconds.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	22-Dec-89 (seiwald)
**	    Created.
**	14-Nov-96 (gordy)
**	    Added timeout argument.
*/

VOID
GCsync( i4 timeout )
{
    iiCLpoll( &timeout );
    EXinterrupt( EX_DELIVER );

    return;
}

/*
** Name: GC_check_uid - Check claimed and real user id
**
** Description:
** 	This routine is used to validate the claimed user name against the
**	user id returned from the IPC channel to detect attempts at spoofing.
**
** Returns:
**	 OK - Everything seems OK, as far as we can tell
**
**	!OK - Apparent attempt to spoof
**
** History:
**	1-jun-94 (robf)
**           Created
**	2-jun-94 (robf)
**           Changed defaults to be ON for secure, OFF otherwise.
**	994 (canor01)
**           Allowed for a NULL gcb, for calls from gca_register()
**           when there is no name server.
**	15-Jul-09 (gordy)
**	    Extend user name buffer to allow max platform length.
*/
static STATUS
GC_check_uid( SVC_PARMS	*svc_parms )
{
    STATUS	status = OK;
    static bool init = FALSE;
    static bool check_uid;
    BS_EXT_INFO info;
    char	user_name[ GC_USERNAME_MAX + 1 ];
    GCA_GCB	*gcb = (GCA_GCB *)svc_parms->gc_cb;
    BS_PARMS	bsp;

    if ( ! init )
    {
	char *c = NULL;
	init=TRUE;

	/*
	** Check if initialized.
	** Default is OFF if not secure or ON if secure 
	** For secure we check if port has labels, since we can't
	** tell what higher level code may be doing
	*/
	check_uid=FALSE;
	/* See if PM value overrides default */
	if(PMget("!.gc_check_uid", &c)==OK)
	{
		if(!STcasecmp(c,"off"))
			check_uid=FALSE;
		else
			check_uid=TRUE;
	}
	/* See if NM variable override */
	NMgtAt( "II_GC_CHECK_UID",  &c );
	if(c && *c )
	{
		if(!STcasecmp(c,"off"))
			check_uid=FALSE;
		else
			check_uid=TRUE;
	}
	/*
	** If no support in BS don't try checking
	*/
        if(!GCdriver->ext_info)
		check_uid=FALSE;
    }

    for(;;)
    {
	/*
	** Only check if configured to do so
	*/
	/* 19-Dec-1994 (canor01)
	** gcb can be null if there is no name server and
	** it is a gca_register() call.
	*/
	if(!check_uid || gcb == NULL)
	    break;

	/*
	** Call BS to get its notion of UID
	*/
	info.info_request=BS_EI_USER_ID;
	info.info_value=0;
	info.len_user_id=sizeof(user_name)-1;
	info.user_id=(PTR)user_name;

	bsp.bcb = gcb->bcb;
	bsp.lbcb = (PTR) listenbcb;
	bsp.syserr = svc_parms->sys_err;

	if((*GCdriver->ext_info)(&bsp, &info)!=OK)
	{	
		GCTRACE(1)("GC_check_uid: Unable to verify user id in BS\n");
		status= !OK;
		break;
	}
	if(info.info_value==0)
	{
		/*
		** No support in BS for user ID, so done
		*/
		GCTRACE(1)("GC_check_uid: BS doesn't support extended user id\n");
		check_uid=FALSE;
		break;
	}
	/*
	** Now check claimed user id against BS user id.
	** If they mismatch we fail on error, unless the BS user id is
	** the same as the current user id. in which case we allow as a
	** trusted connection.
	*/
	if(!STbcompare(svc_parms->user_name, 0, 
		user_name, info.len_user_id, TRUE))
	{
		/* Claimed names match OK */
		break;
	}
	/*
	** No match, so check if trusted
	*/
	if ( ! STbcompare(gc_user_name, 0, user_name, info.len_user_id, TRUE) )
	    break;	/* Trusted */

	/* Not trusted */
	GCTRACE(1)( "GC_check_uid, apparent name mismatch:\n");
	GCTRACE(1)( "\t\tMy name '%s'\n", gc_user_name);
	GCTRACE(1)( "\t\tService parms name '%s'\n", svc_parms->user_name);
	GCTRACE(1)( "\t\tBS IPC name '%s'\n", user_name);
	status= !OK;
	break;

    }

    return( status );
}

/*
** Name: GC_set_blocking - Set blocking I/O
**
** Description:
**	Enable blocking I/O for servers.  Called from CSdispatch().
**
** History:
**	09-oct-1996 (canor01)
**	    Created.
*/

VOID
GC_set_blocking( bool blocking_on )
{
    iisynclisten = blocking_on;
}
