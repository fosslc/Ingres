/*
** Copyright (c) 2004, 2009 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>

#include    <cv.h>
#include    <er.h>
#include    <gc.h>
#include    <me.h>
#include    <mu.h>
#include    <qu.h>
#include    <si.h>
#include    <st.h>
#include    <tr.h>
#include    <tm.h>

#include    <iicommon.h>
#include    <gca.h>
#include    <gcaint.h>
#include    <gcn.h>
#include    <gcnint.h>
#include    <gcm.h>
#include    <gcmint.h>
#include    <gcu.h>
#include    <gcs.h>
#include    <gcaimsg.h>
#include    <gcadm.h>

/*
** Name: gcnslsn.c - top loop for name server
**
** Contains:
**	gcn_register()		- register for IIGCN listen address
**	gcn_server()		- IIGCN main state machine
**	gcn_add_mbuf()		- add mbuf to chain
**
** History:
**	26-Nov-89 (seiwald)
**	    Written.
**	05-Dec-89 (seiwald)
**	    Copy username and password from listen parms into the NCB,
**	    zapping the blanks of the username.  These blanks are coming
**	    from the VMS GClisten, I believe.  This is a temporary fix.
**	05-Dec-89 (ham)
**	    Don't copy password if the pointer is null.
**	11-Dec-89 (seiwald)
**	    Reverted to having CL, rather than mainline, loop to drive 
**	    async I/O events.  VMS doesn't loop.  A call to GCshut stops
**	    GCexec.
**	04-Jan-90 (seiwald)
**	    Negotiate gca_partner_protocol between GCA_LISTEN and GCA_RQRESP
**	    calls, as a good server should.
**	08-Feb-90 (seiwald)
**	    Fail to start if another Name Server is already running.  Use
**	    new GCA_CS_BEDCHECK fast-select to do this check.
**	13-Mar-90 (seiwald)
**	    GCnsid now takes a sys_err, not a svc_parms.
**	27-Apr-90 (jorge)
**	   Added explicit test for GCnsid() != OK.
**	12-Jun-90 (seiwald)
**	   Another Name Server is present if GCA_REQUEST returns either
**	   OK or NO_PEER.
**	22-Aug-90 (seiwald)
**	   New NSA_LISTEN1 state to pick up assoc_id; needed for 
**	   NSA_DISASSOC after a failed listen.
**	24-Aug-90 (seiwald)
**	   Maximum number of Name Server sessions now controlled by
**	   II_GCNxx_MAX symbol.  Reordered listen-reposting logic so
**	   that a MAX of 1 works.
**	30-Aug-90 (gautam)
**	   Added a GCA_TERMINATE call at shutdown
**	06-Dec-90 (seiwald)
**	   Fancier tracing.  Reset state on return.
**	11-Mar-91 (seiwald)
**	    Included all necessary CL headers as per PC group.
**	17-Apr-91 (seiwald)
**	    Replaced enums with #defines, 'cause they're bad.
**	24-Apr-91 (brucek)
**	    Added include for tm.h
**	17-Jul-91 (seiwald)
**	    New define GCN_GCA_PROTOCOL_LEVEL, the protocol level at
**	    which the Name Server is running.  It may be distinct from
**	    the current GCA_PROTOCOL_LEVEL.
**	31-Jan-92 (seiwald)
**	    Support for installation passwords: many new states to step
**	    through password validation, the resolve operation, and making
**	    a connection to a remote Name Server.
**	23-Mar-92 (seiwald)
**	    Shortened up gcn_server() by avoiding gotos.
**	26-Mar-92 (seiwald)
**	    Installation password now just a GCN_LOGIN entry with user "*".
**	27-Mar-92 (seiwald)
**	    Send GCA_RELEASE to remote Name Server after getting tickets.
**	1-Apr-92 (seiwald)
**	    Consolidated some states.
**	1-Apr-92 (seiwald)
**	    Documented.
**	3-Apr-92 (seiwald)
**	    If remote name server connection yields E_GC000B_RMT_LOGIN_FAIL,
**	    assume it doesn't speak the protocol.
**	15-Apr-92 (seiwald)
**	    Call NSA_CLEAR on the remote connection before returning to
**	    the local one.
**	28-May-92 (brucek)
**	    Don't log error message when listen failure is due to bedcheck
**	    (returns E_GC0032_NO_PEER error).
**	1-Sep-92 (seiwald)
**	    gca_erlog() renamed to gcu_erlog() and now takes ER_ARGUMENT's.
**	25-Sep-92 (brucek)
**	    Convert to GCA_API_LEVEL_4 for GCM support; now we repost
**	    listen immediately after it first returns E_GCFFFE_INCOMPLETE.
**	15-Oct-92 (brucek)
**	    Set MIB support flags on GCA_REGISTER call.
**	20-Nov-92 (gautam)
**	    Added in password prompt support.
**	08-Jan-93 (brucek)
**	    Added support for fastselect SHUTDOWN command.
**	12-Jan-93 (brucek)
**	    Don't actually validate password received on GCA_LISTEN
**	    completion -- just set gca_auth_user parm on GCA_INITIATE 
**	    and let GCA_LISTEN do the validation;
**	    Pass GCA_LS_TRUSTED flag down to gcn_oper_ns.
**	25-Jan-93 (brucek)
**	    Corrected state machine (broken by change # 400223).
**	29-Jan-93 (brucek)
**	    Fastselect SHUTDOWN now returns E_GC0040_CS_OK and also
**	    now clears GCN port address.
**	18-Feb-93 (edg)
**	    Added support for asynchronous bedchecks done via GCA_REQUEST.
**	    Bedchecking runs in a reserved ncb with an aid = maxsessions.
**	11-Mar-93 (edg)
**	    Bedcheck mechanism now uses GCA_FASTSELECT rather than
**	    GCA_REQUEST/DISASSOC.  This additionally gets the number
**	    of active associations from each server which is used as a
**	    simple load balancing basis for local server resolve.
**	23-May-93 (edg)
**	    Added gcn_start_bchk() to kick off a bedcheck thread called
**	    when the server starts up.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	10-Aug-93 (brucek)
**	    Eliminated a few unnecessary states in fsm;
**	    Fixed various bedchecking problems:
**		force bedchecking before DO_OPER regardless of timestamp;
**		don't fail bedcheck when unable to acquire storage;
**		correct error checking on GCM message;
**		queue up OPER and RESOLVE threads while bedcheck running.
**	19-Aug-93 (brucek)
**	    Set timeout on bedcheck FASTSELECT.
**	30-Aug-93 (brucek)
**	    Move stat variable out of stack and into ncb;
**	    don't check branch in RSLV_BCHK before firing up bedcheck.
**	08-sep-93 (swm)
**	    Cast completion id. parameter to IIGCa_call() to PTR as it can
**	    be a i4 or a PTR, to accomodate pointer completion ids.
**	    In some cases a i4 value is now cast to PTR; this is
**	    correct because the i4 is an index into an array.
**	16-sep-93 (johnst)
**	    Corrected -1 timeout parameter in various IIGCa_call() calls. 
**	    IIGCa_call() declares time_out as a i4; several calls
**	    here were passing time_out as a long!
**	21-Sep-93 (brucek)
**	    Removed include of dbdbms.h.
**	10-Feb-94 (brucek)
**	    Converted to use gcn_bedcheck to look for running Name Server.
**	01-apr-94 (eml)
**	    BUG #60458.
**	    Moved the call to gca_terminate out of the shutdown logic and
**	    main() in IIGCN.C. The gca terminate logic is no longer executed
**	    in callback mode, thus fixing bug #60458 where, under some
**	    conditions, the gca terminate logic called GCsync and waited for
**	    an I/O to complete that was blocked by being in Callback.
**	 4-Dec-95 (gordy)
**	    Moved global references to gcnint.h.  Extracted non-message 
**	    specific parts of gcn_rslv_init() into gcn_rslv_parse().
**	    Added prototypes.
**	 3-Sep-96 (gordy)
**	    Removed gcn_bedcheck() call, it is now done in main().
**      14-Nov-96 (fanra01)
**          Added a call to GCpasswd_prompt in the NSA_RSLV_MOST case.  The
**          GCpasswd_prompt will prompt for user id and password if the login
**          information has been set for prompt on Windows 95 only.
**	11-Mar-97 (gordy)
**	    Moved gcn utility functions to gcu.
**	19-Mar-97 (gordy)
**	    Fix GCN protocol by adding new GCA protocol level and using
**	    end-of-group rather than end-of-data to signal turn-around.
**	27-May-97 (thoda04)
**	    Include headers and function prototypes.  WIN16 fixes.
**	29-May-97 (gordy)
**	    New GCF security handling.  Passwords no longer returned by
**	    GCA_LISTEN.  GCS server keys may be passed in the aux data
**	    area by registering servers.  No more GCA AUTH function in
**	    GCA_INITIALIZE.
**	 9-Jan-98 (gordy)
**	    Reworked the Name Server state action mechanism to support
**	    labeling and subroutines.
**	 9-Jan-98 (gordy)
**	    Check GCN_NET_FLAG to see if NS_OPER request require bedchecking.
**	12-Jan-98 (gordy)
**	    Added support for direct GCA network connections.
**	 5-May-98 (gordy)
**	    Removed gca_account_name from GCA_REQUEST & GCS_FASTSELECT, now 
**	    obsolete.  Client user ID, from GCA_LISTEN gca_account_name, 
**	    added to server auth.
**	 4-Jun-98 (gordy)
**	    GCA_LISTEN now has timeout to perform background bedchecks.
**	    Bedchecking now done after servicing requests (except OPER).
**	 5-Jun-98 (gordy)
**	    Instead of bedchecking on first init, do bedchecking if thread
**	    started when there is an active listen posted.
**	10-Jun-98 (gordy)
**	    Added ticket expiration and file compression to background work.
**	10-Jul-98 (gordy)
**	    Free server key when done with association.
**	28-Jul-98 (gordy)
**	    Added additional MO objects to server bedchecks to catch
**	    registration problems.
**	26-Aug-98 (gordy)
**	    Request and save delegated authentication from GCA_LISTEN.
**	 4-Sep-98 (gordy)
**	    Release delegated authentication token.
**	15-Sep-98 (gordy)
**	    Added support for installation registry.
**	28-Sep-98 (gordy)
**	    Added actions for registry master roll-over.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	22-Mar-02 (gordy)
**	    Use NS classes rather than deprecated GCA classes.
**	15-Oct-03 (gordy)
**	    Bump GCA API level.
**	28-Jan-04 (gordy)
**	    Return error for unknown requests.
**	24-Mar-04 (wansh01)
**	    Support gcadm interface.
**	 5-Aug-04 (gordy)
**	    Include association ID in session info.
**	28-Sep-04 (gordy)
**	    The shutdown and quiesce operations must be flagged in
**	    the NCB and later transferred to global.  If placed
**	    directly in global, then all sessions will respond to
**	    the flags rather than just the specific session.
**      18-Oct-04 (wansh01)
**          Moved Check user privilege for admin session to gca(gcaasync.c).
**	 1-Mar-06 (gordy)
**	    Server registration connections are not kept open while server
**	    is active.  Server key is stored in server's NCB.  Server may
**	    register itself (NS_OPER) and update server load statistics
**	    (GCA_ACK).  Server key is accessible via gcn_get_server() which
**	    scans the NCB list looking for a matching server.
**      21-Apr-2009 (Gordy) Bug 121967
**          Add GCN_SVR_TERM so that a clean shutdown of the GCN with the
**          fix for bug 121931 does not lead to DBMS deregistrations.
**	 3-Aug-09 (gordy)
**	    Remove string length restrictions.
**	17-Dec-2009 (Bruce Lunsford) Sir 122536
**	    In support of long names, handle memory deallocation after
**	    call to GCpasswd_prompt (for Windows).
*/

/*
** GCN_NCB: Name Server association control block.
**
** History:
**	 1-Mar-06 (gordy)
**	    Server key now saved in sub-structure along with the
**	    server class and address of registered servers.
**	13-Aug-09 (gordy)
**	    Moved delegated authentication into this structure
**	    rather than in the resolve control block so that
**	    it can be managed separately.  Username and account
**	    are dynamically allocated.
*/

typedef struct 
{

    QUEUE		q;
    i4			session_id;
    u_i2		state;		/* Current state */

#define	NCB_DEPTH	3		/* Stack depth */

    u_i1		sp;		/* State stack counter */
    u_i1		labels[NCB_DEPTH + 1];	/* State label stack */
    u_i2		ss[NCB_DEPTH];	/* State stack */

    u_i4		flags;

#define	NCB_USER	0x0001
#define	NCB_SERVER	0x0002
#define	NCB_TRUSTED	0x0010
#define	NCB_HETNET	0x0020
#define	NCB_EMBEDGCC	0x0080
#define	NCB_ADMIN	0x0100
#define	NCB_QUIESCE	0x0200
#define	NCB_SHUTDOWN	0x0400

    struct 
    {

	i4		assoc_id;
	i4		protocol;	/* GCA protocol of connection */
	QUEUE		read;		/* of GCN_MBUF's */
	QUEUE		send;		/* of GCN_MBUF's */

    } client, server, *io;

    STATUS		stat;
    STATUS		*statusp;
    i4			req_proto;
    char		*username;
    char		*account;

    GCN_RESOLVE_CB	grcb;		/* resolve control block */
    i4			deleg_len;	/* Length of delegated auth */
    PTR			deleg;		/* Delegated authentication */

    struct
    {
	char		*key;
	char		*class;
	char		*addr;
    } srv;

    GCA_PARMLIST	parms;		/* GCA parameter list */
    QUEUE		bchk_q;		/* Bedcheck queue */

} GCN_NCB;


/*
** Local data
*/

static QUEUE		gcn_mbuf_que ZERO_FILL;
static QUEUE		gcn_ncb_que ZERO_FILL;
static QUEUE		gcn_ncb_free ZERO_FILL;
static bool		gcn_listening = FALSE;
static PTR 		gca_cb = NULL;
static bool		expire_lticket = FALSE;


/*
** Forward references.
*/

static VOID     gcn_release_deleg( GCN_NCB *ncb, bool free );



/*
** Name: NS_ACTION - list of actions for GCN state machine
**
** History:
**	25-Nov-89 (seiwald)
**	    Created.
**	31-Jan-92 (seiwald)
**	    Extended extensively for installation password support.
**	18-Feb-93 (edg)
**	    Extended for bedcheck support.
**	 9-Jan-98 (gordy)
**	    Reworked the Name Server state action mechanism to support
**	    labeling and subroutines.
**	12-Jan-98 (gordy)
**	    Added support for direct GCA network connections.
**	 4-Jun-98 (gordy)
**	    Reworked bedchecking.
**	10-Jun-98 (gordy)
**	    Added ticket expiration and file compression to background work.
**	15-Sep-98 (gordy)
**	    Added support for installation registry.
**	28-Sep-98 (gordy)
**	    Added actions for registry master roll-over.
**	28-Jan-04 (gordy)
**	    Added NSA_NOOP to report unknown operation error.
**	 1-Mar-06 (gordy)
**	    Added actions for registered servers: NSA_IF_SERVER, NSA_IF_ACK,
**	    NSA_SRV_START, NSA_SRV_EXIT, NSA_DO_UPD.  On NS restart, bedcheck
**	    causes re-registration: NSA_FS_INIT, NSA_FS_BEDCHK.
*/

# define	NSA_LABEL	0	/* label */
# define	NSA_GOTO	1	/* goto unconditionally */
# define	NSA_GOSUB	2	/* call subroutine */
# define	NSA_RETURN	3	/* return from subroutine */
# define	NSA_INIT	4	/* initialize */
# define	NSA_NOOP	5	/* No (permitted) operation request */
# define	NSA_LOG		6	/* log an error */
# define	NSA_SET_CLIENT	7	/* set I/O to client */
# define	NSA_SET_SERVER	8	/* set I/O to server */
# define	NSA_SET_USER	9	/* mark user thread */
# define	NSA_CLEAR_USER	10	/* clear user thread */
# define	NSA_CLEAR_ERR	11	/* set status to OK */
# define	NSA_CLEAR_IO	12	/* free io message queues */
# define	NSA_FREE	13	/* free resources */
# define	NSA_EXIT	14	/* terminate thread */
# define	NSA_IF_RESUME	15	/* if INCOMPLETE */
# define	NSA_IF_TIMEOUT	16	/* if status TIMEOUT */
# define	NSA_IF_FAIL	17	/* is status FAIL */
# define	NSA_IF_ERROR	18	/* if status not OK */
# define	NSA_IF_CLOSED	19	/* admin server closed */
# define	NSA_IF_TERM	20	/* server terminating */
# define	NSA_IF_ADMIN	21	/* monitor command */
# define	NSA_IF_SERVER	22	/* If server registration */
# define	NSA_IF_SHUT	23	/* if SHUTDOWN requested */
# define	NSA_IF_ACK	24	/* if message is GCA_ACK */
# define	NSA_IF_OPER	25	/* if message is GCN_NS_OPER */
# define	NSA_IF_AUTH	26	/* if message is GCN_NS_AUTHORIZE */
# define	NSA_IF_AUTHID	27	/* if GCN_NS_AUTHORIZE user ID */
# define	NSA_IF_RESOLVE	28	/* if message is GCN_NS[2]_RESOLVE */
# define	NSA_IF_RELEASE  29	/* if message is GCA_RELEASE */
# define	NSA_IF_IP	30	/* get installation password */
# define	NSA_IF_DIRECT	31	/* if direct connection */
# define	NSA_LISTEN	32	/* post a listen */
# define	NSA_LS_RESUME	33	/* resume a listen */
# define	NSA_LS_DONE	34	/* complete a listen */
# define	NSA_REPOST	35	/* repost listen */
# define	NSA_RQRESP	36	/* respond to listen */
# define	NSA_REQUEST	37	/* issue GCA_REQUEST */
# define	NSA_RQ_RESUME	38	/* resume GCA_REQUEST */
# define	NSA_RQ_DONE	39	/* complete GCA_REQUEST */
# define	NSA_ATTRIB	40	/* issue GCA_ATTRIBS */
# define	NSA_FS_INIT	41	/* initialize FASTSELECT parms */
# define	NSA_FS_BEDCHK	42	/* set BEDCHECK flag in FS parms */
# define	NSA_FASTSELECT	43	/* issue GCA_FASTSELECT */
# define	NSA_FS_RESUME	44	/* resume GCA_FASTSELECT */
# define	NSA_FS_DONE	45	/* complete GCA_FASTSELECT */
# define	NSA_RECEIVE	46	/* post a receive */
# define	NSA_RV_DONE	47	/* receive completed */
# define	NSA_SEND	48	/* post a send */
# define	NSA_SD_DONE	49	/* send completed */
# define	NSA_DISASSOC	50	/* disassociate */
# define	NSA_DA_RESUME	51	/* finish a disassoc */
# define	NSA_SET_SHUT	52	/* prepare for GCN SHUTDOWN */
# define	NSA_SRV_START	53	/* Server start-up */
# define	NSA_SRV_EXIT	54	/* Server exit */
# define	NSA_DO_UPD	55	/* Update server stats */
# define	NSA_ADM_SESS	56	/* execute monitor command*/
# define	NSA_ADM_RLS	57	/* format release msg */
# define	NSA_FMT_RLS	58	/* format GCA_RELEASE */
# define	NSA_DO_OPER	59	/* process GCN_NS_OPER */
# define	NSA_DO_AUTH	60	/* do GCN_NS_AUTHORIZE */
# define	NSA_RESOLVE	61	/* do GCN_NS[2]_RESOLVE */
# define	NSA_RSLV_DONE	62	/* format GCN_RESOLVED */
# define	NSA_RSLV_ERR	63	/* error during RSLV */
# define	NSA_USE_AUTH	64	/* use a ticket for password */
# define	NSA_GET_AUTH	65	/* format GCN_NS_AUTHORIZE message */
# define	NSA_GOT_AUTH	66	/* parse GCN_AUTHORIZED */
# define	NSA_IP_PROTO	67	/* check AUTH protocol level */
# define	NSA_DIRECT	68	/* format GCN_RESOLVE message */
# define	NSA_RESOLVED	69	/* process GCN2_RESOLVED message */
# define	NSA_DC_PROTO	70	/* check direct connect proto level */
# define	NSA_DC_ERROR	71	/* direct connect error */
# define	NSA_OPER_BCHK	72	/* do bedcheck before OPER */
# define	NSA_BEDCHECK	73	/* ininitalize bedchecking */
# define	NSA_BCHK_NEXT	74	/* bedcheck next server */
# define	NSA_BCHK_CHECK	75	/* check results of bedcheck */
# define	NSA_COMPRESS	76	/* compress name queue files */
# define	NSA_EXPIRE	77	/* expire tickets */
# define	NSA_DO_REG	78	/* Check registry status */
# define	NSA_GET_COMSVR	79	/* Load Comm Server addresses */
# define	NSA_CHK_MASTER	80	/* Build info to check master NS */
# define	NSA_REG_ERROR	81	/* Check for error response */
# define	NSA_GET_MASTER	82	/* Extract info on current master NS */
# define	NSA_REGISTER	83	/* Build info to register with master */
# define	NSA_IF_SLAVE	84	/* Check if slave installation */
# define	NSA_BE_MASTER	85	/* Build info to be master NS */
# define	NSA_REG_RETRY	86	/* Check if need to retry registry */
# define	NSA_CLEAR_REG	87	/* Clear registry retry */

static	char	*gcn_ns_names[] = 
{
  "NSA_LABEL",      "NSA_GOTO",	        "NSA_GOSUB",        "NSA_RETURN",

  "NSA_INIT",       "NSA_NOOP",	        "NSA_LOG",
  "NSA_SET_CLIENT", "NSA_SET_SERVER",   "NSA_SET_USER",     "NSA_CLEAR_USER",
  "NSA_CLEAR_ERR",  "NSA_CLEAR_IO",     "NSA_FREE",         "NSA_EXIT",

  "NSA_IF_RESUME",  "NSA_IF_TIMEOUT",   "NSA_IF_FAIL",      "NSA_IF_ERROR",
  "NSA_IF_CLOSED",  "NSA_IF_TERM",
  "NSA_IF_ADMIN",   "NSA_IF_SERVER",    "NSA_IF_SHUT",    
  "NSA_IF_ACK",     "NSA_IF_OPER",      "NSA_IF_AUTH",      "NSA_IF_AUTHID",  
  "NSA_IF_RESOLVE", "NSA_IF_RELEASE",   "NSA_IF_IP",        "NSA_IF_DIRECT",  

  "NSA_LISTEN",     "NSA_LS_RESUME",    "NSA_LS_DONE",      "NSA_REPOST",
  "NSA_RQRESP",     "NSA_REQUEST",      "NSA_RQ_RESUME",    "NSA_RQ_DONE",
  "NSA_ATTRIB",     "NSA_FS_INIT",      "NSA_FS_BEDCHK",    "NSA_FASTSELECT", 
  "NSA_FS_RESUME",  "NSA_FS_DONE",      "NSA_RECEIVE",      "NSA_RV_DONE",
  "NSA_SEND",       "NSA_SD_DONE",      "NSA_DISASSOC",     "NSA_DA_RESUME",  

  "NSA_SET_SHUT",   "NSA_SRV_START",    "NSA_SRV_EXIT",     "NSA_DO_UPD",
  "NSA_ADM_SESS",   "NSA_ADM_RLS",

  "NSA_FMT_RLS",    "NSA_DO_OPER",      "NSA_DO_AUTH", 
  "NSA_RESOLVE",    "NSA_RSLV_DONE",    "NSA_RSLV_ERR",
  "NSA_USE_AUTH",   "NSA_GET_AUTH",     "NSA_GOT_AUTH",     "NSA_IP_PROTO",
  "NSA_DIRECT",     "NSA_RESOLVED",     "NSA_DC_PROTO",     "NSA_DC_ERROR",

  "NSA_OPER_BCHK",  "NSA_BEDCHECK",     "NSA_BCHK_NEXT",    "NSA_BCHK_CHECK",	
  "NSA_COMPRESS",   "NSA_EXPIRE",

  "NSA_DO_REG",     "NSA_GET_COMSVR",   "NSA_CHK_MASTER",   "NSA_REG_ERROR",
  "NSA_GET_MASTER", "NSA_REGISTER",     "NSA_IF_SLAVE",     "NSA_BE_MASTER",
  "NSA_REG_RETRY",  "NSA_CLEAR_REG",  
  
};


/*
** GCN State labels
*/

# define	LBL_INIT	0
# define	LBL_LS_CHK	1
# define	LBL_LS_RESUME	2
# define	LBL_REJECT	3
# define	LBL_ERROR	4
# define	LBL_TIMEOUT	5
# define	LBL_SHUTDOWN	6
# define	LBL_ADMIN	7
# define	LBL_ADM_ERR	8
# define	LBL_SRV_REG	9
# define	LBL_SRV_READ	10
# define	LBL_SRV_UPD	11
# define	LBL_SRV_OP	12
# define	LBL_SRV_FAIL	13
# define	LBL_SRV_EXIT	14
# define	LBL_READ	15
# define	LBL_NOOP	16
# define	LBL_OPER	17
# define	LBL_DO_OPER	18
# define	LBL_AUTH	19
# define	LBL_RESOLVE	20
# define	LBL_RSLV_DONE	21
# define	LBL_RSLV_ERR	22
# define	LBL_USE_AUTH	23
# define	LBL_AUTH_ERR	24
# define	LBL_GET_AUTH	25
# define	LBL_REM_ERR	26
# define	LBL_CONN_ERR	27
# define	LBL_DIRECT	28
# define	LBL_DIRECT_ERR	29
# define	LBL_REM_ERR1	30
# define	LBL_CONN_ERR1	31
# define	LBL_DONE	32
# define	LBL_EXIT	33
# define	LBL_EXIT1	34
# define	LBL_RESPONSE	35
# define	LBL_DC_ERROR	36
# define	LBL_BEDCHECK	37
# define	LBL_BCHK_NEXT	38
# define	LBL_BCHK_DONE	39
# define	LBL_REGISTRY	40
# define	LBL_REG_DONE	41
# define	LBL_BE_MASTER	42
# define	LBL_CONNECT	43
# define	LBL_REQUEST	44
# define	LBL_RQ_CHK	45
# define	LBL_RQ_RESUME	46
# define	LBL_FASTSELECT	47
# define	LBL_FS_CHK	48
# define	LBL_FS_RESUME	49
# define	LBL_RECEIVE	50
# define	LBL_SEND	51
# define	LBL_DISCONN	52
# define	LBL_DA_CHK	53
# define	LBL_DA_RESUME	54
# define	LBL_RETURN	55

# define	LBL_MAX		56
# define	LBL_NONE	LBL_MAX

/*
** Each label has a position in the state table,
** which is determined during initialization, and
** a description of the general processing state
** associated with the label.
*/

static struct
{
    i2		state;
    char	*desc;
} gcn_labels[] =
{
    { -1,	"LISTEN" },		/* LBL_INIT */
    { -1,	"PROCESS" },		/* LBL_LS_CHK */
    { -1,	"PROCESS" },		/* LBL_LS_RESUME */
    { -1,	"REJECT" },		/* LBL_REJECT */
    { -1,	"ERROR" },		/* LBL_ERROR */
    { -1,	"TIMEOUT" },		/* LBL_TIMEOUT */
    { -1,	"SHUTDOWN" },		/* LBL_SHUTDOWN */
    { -1,	"ADMIN" },		/* LBL_ADMIN */
    { -1,	"ADMIN" },		/* LBL_ADM_ERR */
    { -1,	"SERVER" },		/* LBL_SRV_REG */
    { -1,	"SERVER" },		/* LBL_SRV_READ */
    { -1,	"SERVER" },		/* LBL_SRV_UPD */
    { -1,	"SERVER" },		/* LBL_SRV_OP */
    { -1,	"SERVER" },		/* LBL_SRV_FAIL */
    { -1,	"SERVER" },		/* LBL_SRV_EXIT */
    { -1,	"WAIT" },		/* LBL_READ */
    { -1,	"RESPOND" },		/* LBL_NOOP */
    { -1,	"REQUEST" },		/* LBL_OPER */
    { -1,	"REQUEST" },		/* LBL_DO_OPER */
    { -1,	"AUTHORIZE" },		/* LBL_AUTH */
    { -1,	"RESOLVE" },		/* LBL_RESOLVE */
    { -1,	"RESOLVE" },		/* LBL_RSLV_DONE */
    { -1,	"RESOLVE" },		/* LBL_RSLV_ERR */
    { -1,	"RESOLVE" },		/* LBL_USE_AUTH */
    { -1,	"RESOLVE" },		/* LBL_AUTH_ERR */
    { -1,	"NEGOTIATE" },		/* LBL_GET_AUTH */
    { -1,	"RESOLVE" },		/* LBL_REM_ERR */
    { -1,	"RESOLVE" },		/* LBL_CONN_ERR */
    { -1,	"DIRECT" },		/* LBL_DIRECT */
    { -1,	"RESOLVE" },		/* LBL_DIRECT_ERR */
    { -1,	"RESOLVE" },		/* LBL_REM_ERR1 */
    { -1,	"RESOLVE" },		/* LBL_CONN_ERR1 */
    { -1,	"DONE" },		/* LBL_DONE */
    { -1,	"EXIT" },		/* LBL_EXIT */
    { -1,	"EXIT" },		/* LBL_EXIT1 */
    { -1,	"RESPOND" },		/* LBL_RESPONSE */
    { -1,	"ERROR" },		/* LBL_DC_ERROR */
    { -1,	"BEDCHECK" },		/* LBL_BEDCHECK */
    { -1,	"BEDCHECK" },		/* LBL_BCHK_NEXT */
    { -1,	"BEDCHECK" },		/* LBL_BCHK_DONE */
    { -1,	"REGISTRY" },		/* LBL_REGISTRY */
    { -1,	"REGISTRY" },		/* LBL_REG_DONE */
    { -1,	"REGISTRY" },		/* LBL_BE_MASTER */
    { -1,	"REMOTE" },		/* LBL_CONNECT */
    { -1,	"CONNECT" },		/* LBL_REQUEST */
    { -1,	"CONNECT" },		/* LBL_RQ_CHK */
    { -1,	"CONNECT" },		/* LBL_RQ_RESUME */
    { -1,	"FASTSELECT" },		/* LBL_FASTSELECT */
    { -1,	"FASTSELECT" },		/* LBL_FS_CHK */
    { -1,	"FASTSELECT" },		/* LBL_FS_RESUME */
    { -1,	"RECEIVE" },		/* LBL_RECEIVE */
    { -1,	"SEND" },		/* LBL_SEND */
    { -1,	"DISCONNECT" },		/* LBL_DISCONN */
    { -1,	"DISCONNECT" },		/* LBL_DA_CHK */
    { -1,	"DISCONNECT" },		/* LBL_DA_RESUME */
    { -1,	"RETURN" },		/* LBL_RETURN */
};


/*
** Name: gcn_states - GCN state machine control
**
** Description:
**	Each state in GCN has one action and one branch state identified
**	by a label.  If a state action does not branch, execution falls
**	through to the subsequent state
**
** History:
**	25-Nov-89 (seiwald)
**	    Created.
**	27-Jun-96 (mckba02)
**	    Don't expire the bedcheck interval timer each time
**	    we receive a GCN_NS_OPER message.  This avoids the
**	    bedchecking being performed for each GCN_NS_OPER
**	    message.  This fix is for VMS since bedchecking 
**	    on VMS takes a while. 
**	 9-Jan-98 (gordy)
**	    Reworked the Name Server state action mechanism to support
**	    labeling and subroutines.
**	12-Jan-98 (gordy)
**	    Added support for direct GCA network connections.
**	 4-Jun-98 (gordy)
**	    Reworked bedchecking.
**	 5-Jun-98 (gordy)
**	    Instead of bedchecking on first init, do bedchecking if thread
**	    started when there is an active listen posted.
**	10-Jun-98 (gordy)
**	    Added ticket expiration and file compression to background work.
**	15-Sep-98 (gordy)
**	    Added support for installation registry.
**	28-Sep-98 (gordy)
**	    Added actions for registry master roll-over.
**	28-Jan-04 (gordy)
**	    Added NSA_NOOP to report unknown operation error.
**	 1-Mar-06 (gordy)
**	    Server registration connections are not kept open while server
**	    is active.  Server may register itself (NS_OPER) and update 
**	    server load statistics (GCA_ACK).
*/

static struct 
{

  u_i1		action;
  u_i1		label;

} gcn_states[] = 
{
	/*
	** Initialize
	*/

	NSA_LABEL,	LBL_INIT,

		NSA_INIT,	LBL_EXIT,	/* If listening */

	/* 
	** Post and process GCA_LISTEN
	*/

		NSA_LISTEN,	LBL_NONE,
		NSA_REPOST,	LBL_NONE,

	NSA_LABEL,	LBL_LS_CHK,

		NSA_IF_RESUME,	LBL_LS_RESUME,	/* if INCOMPLETE */
		NSA_LS_DONE,	LBL_NONE,	
		NSA_IF_TIMEOUT,	LBL_TIMEOUT,	/* if LISTEN timed out */
		NSA_IF_ERROR,	LBL_ERROR,
		NSA_CLEAR_ERR,	LBL_NONE,
		NSA_IF_SERVER,	LBL_SRV_REG,	/* if server registration */
		NSA_IF_ADMIN,	LBL_ADMIN,	/* if monitor command  */
		NSA_IF_SHUT,	LBL_SHUTDOWN,	/* if SHUTDOWN requested */
		NSA_IF_CLOSED,	LBL_REJECT,	/* if server closed */

		NSA_SET_USER,	LBL_REJECT,	/* if max users */
		NSA_RQRESP,	LBL_NONE,
		NSA_IF_ERROR,	LBL_DONE, 
		NSA_GOTO,	LBL_READ,

	/*
	** Resume a GCA_LISTEN request
	*/

	NSA_LABEL,	LBL_LS_RESUME,

		NSA_LS_RESUME,	LBL_NONE,
		NSA_GOTO,	LBL_LS_CHK,

	/* 
	** Reject connection.
	*/

	NSA_LABEL,	LBL_REJECT,

		NSA_LOG,	LBL_NONE,
		NSA_CLEAR_ERR,	LBL_NONE,
		NSA_RQRESP, 	LBL_NONE,
		NSA_CLEAR_ERR,	LBL_NONE,
		NSA_GOSUB,	LBL_DISCONN,
		NSA_GOTO,	LBL_EXIT,

	/* 
	** Log error and exit.
	*/

	NSA_LABEL,	LBL_ERROR,

		NSA_LOG,	LBL_NONE,
		NSA_CLEAR_ERR,	LBL_NONE,
		NSA_GOSUB,	LBL_DISCONN,
		NSA_GOTO,	LBL_EXIT,

	/*
	** Background processing.
	*/

	NSA_LABEL,	LBL_TIMEOUT,

		NSA_CLEAR_ERR,	LBL_NONE,
		NSA_GOSUB,	LBL_DISCONN,
		NSA_EXPIRE,	LBL_EXIT,
		NSA_COMPRESS,	LBL_EXIT,
		NSA_GOTO,	LBL_EXIT,

	/* 
	** Shutdown requested.
	*/

	NSA_LABEL,	LBL_SHUTDOWN,

		NSA_SET_SHUT,	LBL_NONE,
		NSA_RQRESP,	LBL_NONE,
		NSA_CLEAR_ERR,	LBL_NONE,
		NSA_GOSUB,	LBL_DISCONN,
		NSA_FREE,	LBL_NONE,
		NSA_EXIT,	LBL_NONE,

	/* 
	** Monitor request.
	*/

	NSA_LABEL,	LBL_ADMIN,

		NSA_RQRESP,	LBL_NONE,
		NSA_IF_ERROR,   LBL_ERROR,
		NSA_CLEAR_ERR,	LBL_NONE,
		NSA_ADM_SESS,	LBL_ADM_ERR,	/* if error */
		NSA_GOTO,	LBL_EXIT, 

	NSA_LABEL,	LBL_ADM_ERR,

		NSA_ADM_RLS,	LBL_NONE, 	/* format gca_release msg */
		NSA_GOSUB,	LBL_SEND,
		NSA_CLEAR_ERR,	LBL_NONE,
		NSA_CLEAR_IO,  	LBL_NONE,
		NSA_GOSUB,	LBL_DISCONN,
		NSA_GOTO,	LBL_EXIT,

	/*
	** Server registration
	*/

	NSA_LABEL,	LBL_SRV_REG,

		NSA_RQRESP,	LBL_NONE,
		NSA_IF_ERROR,	LBL_ERROR, 

	NSA_LABEL,	LBL_SRV_READ,

		NSA_GOSUB,	LBL_RECEIVE,
		NSA_IF_ERROR,	LBL_SRV_FAIL,
		NSA_CLEAR_ERR,	LBL_NONE,
		NSA_IF_RELEASE, LBL_SRV_FAIL,	/* if message is RELEASE */
		NSA_IF_ACK,	LBL_SRV_UPD,	/* if message is ACK */
		NSA_IF_OPER,	LBL_SRV_OP,	/* if message is NS_OPER */
		NSA_NOOP,	LBL_NONE,
		NSA_GOSUB,	LBL_RESPONSE,
		NSA_IF_ERROR,	LBL_SRV_FAIL,
		NSA_GOTO,	LBL_SRV_READ,

	NSA_LABEL,	LBL_SRV_UPD,

		NSA_DO_UPD,	LBL_NONE,
		NSA_CLEAR_IO,	LBL_NONE,
		NSA_GOTO,	LBL_SRV_READ,

	NSA_LABEL,	LBL_SRV_OP,

		NSA_SRV_START,	LBL_NONE,
		NSA_DO_OPER,	LBL_NONE,
		NSA_GOSUB,	LBL_RESPONSE,
		NSA_IF_ERROR,	LBL_SRV_FAIL,
		NSA_GOTO,	LBL_SRV_READ,

	NSA_LABEL,	LBL_SRV_FAIL,

		NSA_IF_TERM,	LBL_SRV_EXIT,
		NSA_SRV_EXIT,	LBL_NONE,

	NSA_LABEL,	LBL_SRV_EXIT,

		NSA_CLEAR_ERR,	LBL_NONE,
		NSA_CLEAR_IO,  	LBL_NONE,
		NSA_GOSUB,	LBL_DISCONN,
		NSA_GOTO,	LBL_EXIT,

	/*
	** Read client request
	*/

	NSA_LABEL,	LBL_READ,

		NSA_GOSUB,	LBL_RECEIVE,
		NSA_IF_ERROR,	LBL_DONE,
		NSA_CLEAR_ERR,	LBL_NONE,
		NSA_IF_RELEASE, LBL_DONE,	/* if message is RELEASE */
		NSA_IF_AUTH,	LBL_AUTH,	/* if message is NS_AUTHORIZE */
		NSA_IF_AUTHID,	LBL_NOOP,	/* if NS_AUTHORIZE user ID */
		NSA_IF_OPER,	LBL_OPER,	/* if message is NS_OPER */
		NSA_IF_RESOLVE,	LBL_RESOLVE,	/* if message is NS_RESOLVE */

	/*
	** No (permitted) operation requested.
	*/

	NSA_LABEL,	LBL_NOOP,

		NSA_NOOP,	LBL_NONE,
		NSA_GOSUB,	LBL_RESPONSE,
		NSA_IF_ERROR,	LBL_DONE, 
		NSA_GOTO,	LBL_READ,

	/* 
	** Process GCN_NS_OPER request 
	*/

	NSA_LABEL,	LBL_OPER,

		NSA_OPER_BCHK,  LBL_DO_OPER, 	/* if bedcheck not needed */
		NSA_GOSUB,	LBL_BEDCHECK,
	
	NSA_LABEL,	LBL_DO_OPER,

		NSA_DO_OPER,	LBL_NONE,
		NSA_GOSUB,	LBL_RESPONSE,
		NSA_IF_ERROR,	LBL_DONE, 
		NSA_GOTO,	LBL_READ,

	/* 
	** Process GCN_NS_AUTHORIZE request
	*/

	NSA_LABEL,	LBL_AUTH,

		NSA_DO_AUTH,	LBL_NONE,
		NSA_GOSUB,	LBL_RESPONSE,
		NSA_IF_ERROR,	LBL_DONE, 
		NSA_GOTO,	LBL_READ,

	/* 
	** Process GCN_NS_RESOLVE request
	*/

	NSA_LABEL,	LBL_RESOLVE,

		NSA_RESOLVE,	LBL_RSLV_ERR,	/* if error */
		NSA_IF_IP,	LBL_USE_AUTH, 	/* if installation pwd */
		NSA_IF_DIRECT,	LBL_DIRECT,	/* if direct connect */

	NSA_LABEL,	LBL_RSLV_DONE,

		NSA_RSLV_DONE,	LBL_DONE, 	/* if error */
		NSA_GOSUB,	LBL_RESPONSE,
		NSA_IF_ERROR,	LBL_DONE, 
		NSA_GOTO,	LBL_READ,

	NSA_LABEL,	LBL_RSLV_ERR,

		NSA_RSLV_ERR,	LBL_NONE,
		NSA_SET_CLIENT,	LBL_NONE,
		NSA_GOTO,	LBL_RSLV_DONE,

	NSA_LABEL,	LBL_USE_AUTH,

		NSA_USE_AUTH,	LBL_AUTH_ERR,	/* if error */
		NSA_IF_DIRECT,	LBL_DIRECT,	/* if direct connect */
		NSA_GOTO,	LBL_RSLV_DONE,

	NSA_LABEL,	LBL_AUTH_ERR,

		NSA_IF_FAIL,	LBL_GET_AUTH,	/* if no cached ticket */
		NSA_GOTO,	LBL_RSLV_ERR,

	/* 
	** Connect to remote Name Server to get more tickets.
	*/

	NSA_LABEL,	LBL_GET_AUTH,

		NSA_SET_SERVER,	LBL_NONE,
		NSA_GET_AUTH,	LBL_RSLV_ERR, 	/* if error */
		NSA_GOSUB,	LBL_CONNECT,
		NSA_IP_PROTO,	LBL_NONE,
		NSA_IF_ERROR,	LBL_CONN_ERR,
		NSA_CLEAR_ERR,	LBL_NONE,
		NSA_GOT_AUTH,	LBL_REM_ERR,	/* if error */
		NSA_CLEAR_IO,	LBL_NONE,
		NSA_SET_CLIENT,	LBL_NONE,
		NSA_GOTO,	LBL_USE_AUTH,

	/* 
	** Remote Name Server returned an error. 
	*/

	NSA_LABEL,	LBL_REM_ERR,

		NSA_RSLV_ERR,	LBL_NONE,
		NSA_CLEAR_IO,	LBL_NONE,
		NSA_SET_CLIENT,	LBL_NONE,
		NSA_GOTO,	LBL_RSLV_DONE, 

	/* 
	** Error during remote connection.
	*/

	NSA_LABEL,	LBL_CONN_ERR,

		NSA_LOG,	LBL_NONE,
		NSA_RSLV_ERR,	LBL_NONE,
		NSA_CLEAR_ERR,	LBL_NONE,
		NSA_CLEAR_IO,	LBL_NONE,
		NSA_GOSUB,	LBL_DISCONN,
		NSA_SET_CLIENT,	LBL_NONE,
		NSA_GOTO,	LBL_RSLV_DONE, 

	/* 
	** Connect to remote Name Server for remote resolve.
	*/

	NSA_LABEL,	LBL_DIRECT,

		NSA_SET_SERVER,	LBL_NONE,
		NSA_DIRECT,	LBL_DIRECT_ERR, /* if error */
		NSA_GOSUB,	LBL_CONNECT,
		NSA_DC_PROTO,	LBL_NONE,
		NSA_IF_ERROR,	LBL_CONN_ERR1,
		NSA_CLEAR_ERR,	LBL_NONE,
		NSA_RESOLVED,	LBL_REM_ERR1,	/* if error */
		NSA_CLEAR_IO,	LBL_NONE,
		NSA_SET_CLIENT,	LBL_NONE,
		NSA_GOTO,	LBL_RSLV_DONE,

	NSA_LABEL,	LBL_DIRECT_ERR,

		NSA_GOSUB,	LBL_DC_ERROR,
		NSA_SET_CLIENT,	LBL_NONE,
		NSA_IF_IP,	LBL_USE_AUTH,
		NSA_GOTO,	LBL_RSLV_DONE,

	/* 
	** Remote Name Server returned an error. 
	*/

	NSA_LABEL,	LBL_REM_ERR1,

		NSA_GOSUB,	LBL_DC_ERROR,
		NSA_CLEAR_IO,	LBL_NONE,
		NSA_SET_CLIENT,	LBL_NONE,
		NSA_IF_IP,	LBL_USE_AUTH,
		NSA_GOTO,	LBL_RSLV_DONE, 

	/* 
	** Error during remote connection.
	*/

	NSA_LABEL,	LBL_CONN_ERR1,

		NSA_GOSUB,	LBL_DC_ERROR,
		NSA_CLEAR_IO,	LBL_NONE,
		NSA_GOSUB,	LBL_DISCONN,
		NSA_SET_CLIENT,	LBL_NONE,
		NSA_IF_IP,	LBL_USE_AUTH,
		NSA_GOTO,	LBL_RSLV_DONE, 

	/* 
	** Log user error (if any) and exit.
	*/

	NSA_LABEL,	LBL_DONE,

		NSA_LOG,	LBL_NONE,
		NSA_CLEAR_ERR,	LBL_NONE,
		NSA_CLEAR_IO,  	LBL_NONE,
		NSA_GOSUB,	LBL_DISCONN,
		NSA_CLEAR_USER,	LBL_NONE,
		NSA_GOTO,	LBL_EXIT,

	/*
	** Terminate thread.
	*/

	NSA_LABEL,	LBL_EXIT,

		NSA_BEDCHECK,	LBL_EXIT1,	/* if bedcheck not needed */
		NSA_GOSUB,	LBL_BEDCHECK,

	NSA_LABEL,	LBL_EXIT1,

		NSA_GOSUB,	LBL_REGISTRY,
		NSA_FREE,	LBL_NONE,
		NSA_EXIT,	LBL_NONE,

	/*
	** Subroutine: send response to client.
	*/

	NSA_LABEL,	LBL_RESPONSE,

		NSA_GOSUB,	LBL_SEND,
		NSA_IF_ERROR,	LBL_RETURN, 
		NSA_CLEAR_IO,	LBL_NONE,
		NSA_RETURN,	LBL_NONE,

	/*
	** Subroutine: handle direct connect errors
	*/

	NSA_LABEL,	LBL_DC_ERROR,

		NSA_LOG,	LBL_NONE,
		NSA_CLEAR_ERR,	LBL_NONE,
		NSA_DC_ERROR,	LBL_NONE,
		NSA_LOG,	LBL_NONE,
		NSA_CLEAR_ERR,	LBL_NONE,
		NSA_RETURN,	LBL_NONE,

	/* 
	** Subroutine: Do bedcheck 
	*/

	NSA_LABEL,	LBL_BEDCHECK,

		NSA_SET_SERVER,	LBL_NONE,

	NSA_LABEL,	LBL_BCHK_NEXT,

		NSA_BCHK_NEXT,	LBL_BCHK_DONE,	/* if no more servers */
		NSA_FS_INIT,	LBL_NONE,
		NSA_FS_BEDCHK,	LBL_NONE,
		NSA_GOSUB,	LBL_FASTSELECT,
		NSA_BCHK_CHECK,	LBL_NONE,
		NSA_CLEAR_ERR,	LBL_NONE,
		NSA_CLEAR_IO,	LBL_NONE,
		NSA_GOTO,	LBL_BCHK_NEXT,
	
	NSA_LABEL,	LBL_BCHK_DONE,

		NSA_CLEAR_ERR,	LBL_NONE,
		NSA_CLEAR_IO,  	LBL_NONE,
		NSA_SET_CLIENT,	LBL_NONE,
		NSA_RETURN,	LBL_NONE,

	/*
	** Subroutine: Installation Registry
	*/

	NSA_LABEL,	LBL_REGISTRY,

		NSA_DO_REG,	LBL_RETURN,	/* If nothing to do */
		NSA_GET_COMSVR,	LBL_RETURN,	/* If no Comm Server */
		NSA_SET_SERVER,	LBL_NONE,
		NSA_CHK_MASTER,	LBL_REG_DONE,	/* If error */
		NSA_FS_INIT,	LBL_NONE,
		NSA_GOSUB,	LBL_FASTSELECT,
		NSA_IF_ERROR,	LBL_BE_MASTER,
		NSA_CLEAR_ERR,	LBL_NONE,
		NSA_REG_ERROR,	LBL_BE_MASTER,	/* If error response */
		NSA_GET_MASTER, LBL_REG_DONE,	/* If same master NS */

		NSA_CLEAR_IO,	LBL_NONE,
		NSA_REGISTER,	LBL_REG_DONE,	/* If error */
		NSA_GOSUB,	LBL_CONNECT,

	NSA_LABEL,	LBL_REG_DONE,

		NSA_CLEAR_REG,	LBL_NONE,
		NSA_CLEAR_ERR,	LBL_NONE,
		NSA_CLEAR_IO,	LBL_NONE,
		NSA_SET_CLIENT,	LBL_NONE,
		NSA_RETURN,	LBL_NONE,

	NSA_LABEL,	LBL_BE_MASTER,

		NSA_IF_SLAVE,	LBL_REG_DONE,	/* If not registry peer */
		NSA_CLEAR_ERR,	LBL_NONE,
		NSA_CLEAR_IO,	LBL_NONE,
		NSA_GET_COMSVR,	LBL_REG_DONE,	/* If no Comm Server */
		NSA_BE_MASTER,	LBL_REG_DONE,	/* If error */
		NSA_FS_INIT,	LBL_NONE,
		NSA_GOSUB,	LBL_FASTSELECT,
		NSA_CLEAR_ERR,	LBL_NONE,
		NSA_CLEAR_IO,	LBL_NONE,
		NSA_REG_RETRY,	LBL_REGISTRY,	/* If need to retry */
		NSA_GOTO,	LBL_REG_DONE,

	/*
	** Subroutine: Connect
	** 
	** Connect to server, send request, receive 
	** response, send release and disconnect.
	**
	** The send MBUF queue must have two entries: a 
	** GCA_RESOLVED message to be used as the target
	** of the connection request, and the message 
	** for the request.  The response is placed in 
	** the receive MBUF queue.
	*/

	NSA_LABEL,	LBL_CONNECT,

		NSA_GOSUB,	LBL_REQUEST,
		NSA_IF_ERROR,	LBL_RETURN,
		NSA_GOSUB,	LBL_SEND,
		NSA_IF_ERROR,	LBL_RETURN, 
		NSA_GOSUB,	LBL_RECEIVE,
		NSA_IF_ERROR,	LBL_RETURN,
		NSA_CLEAR_ERR,	LBL_NONE,
		NSA_FMT_RLS,	LBL_RETURN, 	/* if error */
		NSA_GOSUB,	LBL_SEND,
		NSA_IF_ERROR,	LBL_RETURN, 
		NSA_GOSUB,	LBL_DISCONN,
		NSA_RETURN,	LBL_NONE,

	/*
	** Subroutine: Issue GCA_REQUEST
	*/

	NSA_LABEL,	LBL_REQUEST,

		NSA_REQUEST,	LBL_NONE,

	NSA_LABEL,	LBL_RQ_CHK,

		NSA_IF_RESUME, 	LBL_RQ_RESUME, 	/* if INCOMPLETE */
		NSA_RQ_DONE,	LBL_NONE,
		NSA_IF_ERROR,	LBL_RETURN,
		NSA_ATTRIB,	LBL_NONE,
		NSA_RETURN,	LBL_NONE,

	NSA_LABEL,	LBL_RQ_RESUME,

		NSA_RQ_RESUME,	LBL_NONE,
		NSA_GOTO,	LBL_RQ_CHK,

	/*
	** Subroutine: Issue GCA_FASTSELECT.
	*/

	NSA_LABEL,	LBL_FASTSELECT,

		NSA_FASTSELECT,	LBL_NONE,

	NSA_LABEL,	LBL_FS_CHK,

		NSA_IF_RESUME,	LBL_FS_RESUME,	/* if INCOMPLETE */
		NSA_IF_ERROR,	LBL_RETURN,
		NSA_FS_DONE,	LBL_NONE,
		NSA_RETURN,	LBL_NONE,

	NSA_LABEL,	LBL_FS_RESUME,

		NSA_FS_RESUME,	LBL_NONE,
		NSA_GOTO,	LBL_FS_CHK,

	/*
	** Subroutine: Issue GCA_RECEIVE
	*/

	NSA_LABEL,	LBL_RECEIVE,

		NSA_RECEIVE,	LBL_NONE,
		NSA_IF_ERROR,	LBL_RETURN, 
		NSA_RV_DONE,	LBL_RECEIVE,	/* if more messages */
		NSA_RETURN,	LBL_NONE,

	/*
	** Subroutine: Issue GCA_SEND
	*/

	NSA_LABEL,	LBL_SEND,

		NSA_SEND,	LBL_NONE,
		NSA_IF_ERROR,	LBL_RETURN,
		NSA_SD_DONE,	LBL_SEND,	/* if more messages */
		NSA_RETURN,	LBL_NONE,

	/*
	** Subroutine: Issue GCA_DISASSOC
	*/

	NSA_LABEL,	LBL_DISCONN,

		NSA_DISASSOC,	LBL_NONE,

	NSA_LABEL,	LBL_DA_CHK,

		NSA_IF_RESUME,  LBL_DA_RESUME, 	/* if INCOMPLETE */
		NSA_LOG,	LBL_NONE,
		NSA_CLEAR_ERR,	LBL_NONE,
		NSA_RETURN,	LBL_NONE,

	NSA_LABEL,	LBL_DA_RESUME,

		NSA_DA_RESUME,	LBL_NONE,
		NSA_GOTO,	LBL_DA_CHK, 

	/*
	** Generic subroutine return.
	*/

	NSA_LABEL,	LBL_RETURN,

		NSA_RETURN,	LBL_NONE,
};


/*
** Name: gcn_server() - IIGCN main state machine
**
** Description:
**	Main loop for accepting listens, reading clients request,
**	dispatching request, sending response, and closing connection.
**	See description at top of IIGCN.C file for more info.
**
** Inputs:
**	ptr	Session control block (NCB).
**
** Side effects:
**	IIGCN operation.
**
** Called by:
**	GCA as completion routine for all I/O.
**
** History:
**	25-Nov-89 (seiwald)
**	    Written.
**	31-Jan-92 (seiwald)
**	    Extended extensively for installation password support.
**	 4-Dec-94 (gordy)
**	    Extracted non-message specific parts of gcn_rslv_init()
**	    into gcn_rslv_parse().
**	19-Mar-97 (gordy)
**	    Fix GCN protocol by adding new GCA protocol level and using
**	    end-of-group rather than end-of-data to signal turn-around.
**	29-May-97 (gordy)
**	    New GCF security handling.  Passwords no longer returned by
**	    GCA_LISTEN.  GCS server keys may be passed in the aux data
**	    area by registering servers.
**	17-Oct-97 (rajus01)
**	    Added gcn_rslv_attr() to resolve attribute info for the vnode.
**	 4-Dec-97 (rajus01)
**	    Added protocol parameter to gcn_make_auth(), gcn_use_auth(). 
**	 9-Jan-98 (gordy)
**	    Reworked the Name Server state action mechanism to support
**	    labeling and subroutines.
**	 9-Jan-98 (gordy)
**	    Check GCN_NET_FLAG to see if NS_OPER request require bedchecking.
**	12-Jan-98 (gordy)
**	    Added support for direct GCA network connections.
**	 5-May-98 (gordy)
**	    Removed gca_account_name from GCA_REQUEST & GCS_FASTSELECT, now 
**	    obsolete.  Client user ID, from GCA_LISTEN gca_account_name, 
**	    added to server auth.
**	 4-Jun-98 (gordy)
**	    GCA_LISTEN now has timeout to perform background bedchecks.
**	    Bedchecking now done after servicing requests (except OPER).
**	10-Jun-98 (gordy)
**	    Added ticket expiration and file compression to background work.
**	10-Jul-98 (gordy)
**	    Free server key when done with association.
**	28-Jul-98 (gordy)
**	    Added additional MO objects to server bedchecks to catch
**	    registration problems.
**	26-Aug-98 (gordy)
**	    Save delegated authentication from GCA_LISTEN.
**	 4-Sep-98 (gordy)
**	    Release delegated authentication token.
**	15-Sep-98 (gordy)
**	    Added support for installation registry.
**	28-Sep-98 (gordy)
**	    Added actions for registry master roll-over.
**	 2-Jul-99 (rajus01)
**	    Added NCB_EMBEDGCC.
**	28-Jan-04 (gordy)
**	    Added NSA_NOOP to report unknown operation error.
**	28-Sep-04 (gordy)
**	    The shutdown and quiesce operations must be flagged in
**	    the NCB and later transferred to global.  If placed
**	    directly in global, then all sessions will respond to
**	    the flags rather than just the specific session.
**      01-Oct-2004 (rajus01) Startrak Prob 148; Bug# 113165
**         Use the timeout value set by bchk_msg_timeout for bedcheck messages.
**	 1-Mar-06 (gordy)
**	    Server registration connections are not kept open while server
**	    is active.  Server may register itself (NS_OPER) and update 
**	    server load statistics (GCA_ACK).
**	7-Jul-06 (rajus01)
**	    Added support for "show server" command in GCN admin interface
**	    to display registry info.
**	08-Oct-06 (rajus01)
**	   Added highwater to account for maximum number of active sessions ever**	   reached.
**	 3-Aug-09 (gordy)
**	    Use gcn_ir_save() to save IR Master information.  Username
**	    and account are dynamically allocated.  Delegated auth is
**	    now stored directly in NCB rather than resolve CB.
**	17-Dec-2009 (Bruce Lunsford) Sir 122536
**	    In support of long names, handle memory deallocation after call
**	    to GCpasswd_prompt, which may have had to increase the length of
**	    the user and/or password fields.
*/

static VOID
gcn_server( PTR ptr )
{
    GCN_NCB 	*ncb = (GCN_NCB *)ptr;
    GCN_MBUF	*mbuf;
    bool 	branch = FALSE;

top:

    if ( 
	 IIGCn_static.trace >= 2  &&
	 gcn_states[ ncb->state ].action != NSA_LABEL  &&
	 gcn_states[ ncb->state ].action != NSA_GOTO
       )
	TRdisplay( "%4d     GCN (%03d) %s status 0x%x\n", 
		   ncb->session_id, ncb->state, 
		   gcn_ns_names[ gcn_states[ ncb->state ].action ],
		   ncb->statusp ? *ncb->statusp : OK );

    branch = FALSE;

    switch( gcn_states[ ncb->state++ ].action )
    {
	case NSA_LABEL :
	    /*
	    ** Save label to identify processing state.
	    */
	    ncb->labels[ ncb->sp ] = gcn_states[ ncb->state - 1 ].label;
	    break;

	case NSA_GOTO : 		/* Branch unconditionally */
	    branch = TRUE;
	    break;

	case NSA_GOSUB :		/* Call subroutine */
	    ncb->ss[ ncb->sp++ ] = ncb->state;
	    branch = TRUE;
	    break;
	
	case NSA_RETURN :		/* Return from subroutine */
	    ncb->state = ncb->ss[ --ncb->sp ];
	    break;

    	case NSA_INIT :
	    /*
	    ** Initialize NCB, branch if listening.
	    */
	    ncb->statusp = &ncb->stat;
	    ncb->io = &ncb->client;
	    ncb->client.assoc_id = -1;
	    ncb->server.assoc_id = -1;
	    QUinit( &ncb->client.read );
	    QUinit( &ncb->client.send );
	    QUinit( &ncb->server.read );
	    QUinit( &ncb->server.send );
	    QUinit( &ncb->bchk_q );

	    branch = gcn_listening;
	    break;

	case NSA_NOOP :
	    /*
	    ** No (permitted) operation requested.
	    */
	    gcn_error( E_GC000D_ASSOCN_REFUSED, (CL_ERR_DESC *)0, (char *)0, 
		       (GCN_MBUF *)&ncb->io->send );
	    break;

	case NSA_LOG :			/* Log an error */
	    if ( *ncb->statusp != OK  &&  *ncb->statusp != FAIL  &&
		 *ncb->statusp != E_GC0032_NO_PEER )
	        gcu_erlog( ncb->session_id, IIGCn_static.language, 
			   *ncb->statusp, NULL, 0, NULL );
	    break;

	case NSA_SET_CLIENT :		/* Set io to client association */
	    ncb->io = &ncb->client;
	    break;

	case NSA_SET_SERVER :		/* Set io to server association */
	    ncb->io = &ncb->server;
	    break;

	case NSA_SET_USER :
	    IIGCn_static.highwater++;
	    if ( IIGCn_static.usr_sess_count < IIGCn_static.max_user_sessions )
	    {
		ncb->flags |= NCB_USER;
	        IIGCn_static.usr_sess_count++;
	    }
	    else
	    {
		*ncb->statusp = E_GC0156_GCN_MAX_SESSIONS;
		branch = TRUE;
	    }
	    break;

	case NSA_CLEAR_USER :		/* Clear user thread */
	    ncb->flags &= ~NCB_USER;
	    IIGCn_static.usr_sess_count--;
	    break;

	case NSA_CLEAR_ERR :		/* Set status to OK */
	    ncb->statusp = &ncb->stat;
	    ncb->stat = OK;
	    break;

	case NSA_CLEAR_IO :		/* Empty send/receive queues */
	    while( (mbuf = (GCN_MBUF *)QUremove( ncb->io->send.q_next )) )
		QUinsert( &mbuf->q, &gcn_mbuf_que );
	    while( (mbuf = (GCN_MBUF *)QUremove( ncb->io->read.q_next )) )
		QUinsert( &mbuf->q, &gcn_mbuf_que );
	    break;

	case NSA_FREE :
	    /*
	    ** Free allocated resourced in NCB.
	    */
	    gcn_rslv_cleanup( &ncb->grcb );
	    gcn_release_deleg( ncb, TRUE );

	    while( (mbuf = (GCN_MBUF *)QUremove( ncb->client.send.q_next )) )
		QUinsert( &mbuf->q, &gcn_mbuf_que );

	    while( (mbuf = (GCN_MBUF *)QUremove( ncb->client.read.q_next )) )
		QUinsert( &mbuf->q, &gcn_mbuf_que );

	    while( (mbuf = (GCN_MBUF *)QUremove( ncb->server.send.q_next )) )
		QUinsert( &mbuf->q, &gcn_mbuf_que );

	    while( (mbuf = (GCN_MBUF *)QUremove( ncb->server.read.q_next )) )
		QUinsert( &mbuf->q, &gcn_mbuf_que );

	    if ( ncb->username )
	    {
	    	MEfree( (PTR)ncb->username );
		ncb->username = NULL;
	    }

	    if ( ncb->account )
	    {
	        MEfree( (PTR)ncb->account );
		ncb->account = NULL;
	    }

	    if ( ncb->srv.class )
	    {
	        MEfree( (PTR)ncb->srv.class );
		ncb->srv.class = NULL;
	    }

	    if ( ncb->srv.addr )
	    {
	        MEfree( (PTR)ncb->srv.addr );
		ncb->srv.addr = NULL;
	    }

	    if ( ncb->srv.key )
	    {
		MEfree( (PTR)ncb->srv.key );
		ncb->srv.key = NULL;
	    }
	    break;

	case NSA_EXIT :
	    /*
	    ** Free the session control block (NCB).
	    **
	    ** Check server shutdown conditions.
	    **
	    ** If a listen is not currently active,
	    ** start a new session to repost the listen.
	    */
	    QUinsert( QUremove( &ncb->q ), gcn_ncb_free.q_prev );

	    if ( IIGCn_static.flags & GCN_SVR_QUIESCE ) 
	    {
		/*
		** There may be any number of system sessions
		** There should be no user or admin sessions.
		*/
		if ( IIGCn_static.usr_sess_count < 1  &&
		     IIGCn_static.adm_sess_count < 1 )
		    IIGCn_static.flags |= GCN_SVR_SHUT;
	    }

	    if ( IIGCn_static.flags & GCN_SVR_SHUT ) 
	    {
		/*
		** Shutdown the Name Server.
		*/
		CL_ERR_DESC	sys_err;
		(VOID)GCnsid( GC_CLEAR_NSID, NULL, 0, &sys_err );
		GCshut();
	    }
	    else  if ( ! gcn_listening )  
	        gcn_start_session();
	    return;

	case NSA_IF_RESUME :		/* Branch if INCOMPLETE */
	    branch = (*ncb->statusp == E_GCFFFE_INCOMPLETE);
	    break;

	case NSA_IF_TIMEOUT:		/* Branch if statis is TIMEOUT */
	    branch = (*ncb->statusp == E_GC0020_TIME_OUT);
	    break;

	case NSA_IF_FAIL:		/* Branch if status is FAIL */
	    branch = (*ncb->statusp == FAIL);
	    break;

	case NSA_IF_ERROR :		/* Branch if status not OK */
	    branch = (*ncb->statusp != OK);
	    break;

	case NSA_IF_CLOSED :		/* Branch if server closed */
	    if ( IIGCn_static.flags & GCN_SVR_CLOSED ) 	
	    {
		*ncb->statusp = E_GC0169_GCN_ADM_SVR_CLOSED;
		branch = TRUE; 
	    }
	    break;

	case NSA_IF_TERM :		/* Branch if server terminating */
	    branch = ((IIGCn_static.flags & GCN_SVR_TERM) != 0);
	    break;

	case NSA_IF_ADMIN:		/* Branch if admin session */
	    branch = ((ncb->flags & NCB_ADMIN) != 0);
	    break;
       	
	case NSA_IF_SERVER:		/* Branch if server registration */
	    branch = ((ncb->flags & NCB_SERVER) != 0);
	    break;

	case NSA_IF_SHUT :		/* Branch if SHUTDOWN requested */
	    branch = ((ncb->flags & (NCB_QUIESCE | NCB_SHUTDOWN)) != 0);
	    break;

	case NSA_IF_ACK :		/* Branch if GCA_ACK message */
	    mbuf = (GCN_MBUF *)ncb->io->read.q_next;
	    branch = (mbuf->type == GCA_ACK);
	    break;

	case NSA_IF_OPER :		/* Branch if GCN_NS_OPER message */
	    mbuf = (GCN_MBUF *)ncb->io->read.q_next;
	    branch = (mbuf->type == GCN_NS_OPER);
	    break;

	case NSA_IF_AUTH :		/* Branch if GCN_NS_AUTHORIZE msg */
	    mbuf = (GCN_MBUF *)ncb->io->read.q_next;
	    branch = (mbuf->type == GCN_NS_AUTHORIZE);
	    break;

	case NSA_IF_AUTHID :		/* Branch if GCN_NS_AUTHORIZE user ID */
	    branch = (! STcompare( ncb->username, GCN_NOUSER ));
	    break;

	case NSA_IF_RESOLVE :		/* Branch if GCN_NS[2]_RESOLVE msg */
	    mbuf = (GCN_MBUF *)(ncb->io->read.q_next);
	    ncb->grcb.gca_message_type = mbuf->type;
	    branch = (mbuf->type == GCN_NS_RESOLVE  || 
		      mbuf->type == GCN_NS_2_RESOLVE);
	    break;

	case NSA_IF_RELEASE :		/* Branch if GCA_RELEASE message */
	    mbuf = (GCN_MBUF *)(ncb->io->read.q_next);
	    branch = (mbuf->type == GCA_RELEASE);
	    break;

	case NSA_IF_IP :		/* Branch if installation password */
	    branch = gcn_can_auth( &ncb->grcb );
	    break;

	case NSA_IF_DIRECT :		/* Branch if direct connection */
	    /*
	    ** Client must be at a sufficient protocol level
	    ** to perform a direct GCA connection.
	    */
	    branch = ( (ncb->grcb.flags & GCN_RSLV_DIRECT) != 0  &&
		       ncb->io->protocol >= GCA_PROTOCOL_LEVEL_63 );
	    break;

	case NSA_LISTEN:		/* Post a listen */
            MEfill( sizeof(ncb->parms), 0, (PTR) &ncb->parms );
	    ncb->statusp = &ncb->parms.gca_ls_parms.gca_status;
	    gcn_listening = TRUE;
 
	    gca_call( &gca_cb, GCA_LISTEN, &ncb->parms, 
			GCA_ASYNC_FLAG, (PTR)ncb, 
			IIGCn_static.timeout, &ncb->stat );
	    if ( ncb->stat != OK )  break;
	    return;

	case NSA_LS_RESUME :		/* Resume a listen */
	    gca_call( &gca_cb, GCA_LISTEN, &ncb->parms, 
			GCA_RESUME, (PTR)ncb, -1, &ncb->stat );
	    if ( ncb->stat != OK )  break;
	    return;

	case NSA_LS_DONE :
	    /*
	    ** Save listen results.
	    */
	    TMnow( &IIGCn_static.now );
	    ncb->io->assoc_id = ncb->parms.gca_ls_parms.gca_assoc_id;

	    if ( *ncb->statusp == OK )
	    {
		if ( IIGCn_static.trace >= 4 )
		    TRdisplay( "%4d   GCN client protocol = %d\n", 
		    	       ncb->session_id, 
			       ncb->parms.gca_ls_parms.gca_partner_protocol );

		ncb->io->protocol = 
			ncb->parms.gca_ls_parms.gca_partner_protocol;

		if ( ncb->io->protocol > GCN_GCA_PROTOCOL_LEVEL )
		    ncb->io->protocol = GCN_GCA_PROTOCOL_LEVEL;

		ncb->username = 
			STalloc( ncb->parms.gca_ls_parms.gca_user_name );

		if ( ncb->username )
		    STzapblank( ncb->username, ncb->username );
		else 
		{
		    *ncb->statusp = E_GC0121_GCN_NOMEM;
		    break;
		}

		ncb->account = 
			STalloc( ncb->parms.gca_ls_parms.gca_account_name );

		if ( ncb->account )
		    STzapblank( ncb->account, ncb->account );
		else
		{
		    *ncb->statusp = E_GC0121_GCN_NOMEM;
		    break;
		}

		if ( ncb->parms.gca_ls_parms.gca_flags & GCA_LS_TRUSTED )
		    ncb->flags |= NCB_TRUSTED;

		if ( ncb->parms.gca_ls_parms.gca_flags & GCA_LS_EMBEDGCC )
		    ncb->flags |= NCB_EMBEDGCC;

		/* 
		** Process the aux data
		*/
		while( ncb->parms.gca_ls_parms.gca_l_aux_data > 0 )
		{
		    GCA_AUX_DATA	aux_hdr;
		    char		*aux_data;
		    i4		aux_len;

		    MEcopy( ncb->parms.gca_ls_parms.gca_aux_data,
			    sizeof( aux_hdr ), (PTR)&aux_hdr );
		    aux_data = (char *)ncb->parms.gca_ls_parms.gca_aux_data +
			       sizeof( aux_hdr );
		    aux_len = aux_hdr.len_aux_data - sizeof( aux_hdr );

		    switch( aux_hdr.type_aux_data )
		    {
		    case GCA_ID_QUIESCE  : ncb->flags |= NCB_QUIESCE;	break;
		    case GCA_ID_SHUTDOWN : ncb->flags |= NCB_SHUTDOWN;	break;
		    case GCA_ID_CMDSESS  : 
			ncb->flags |= NCB_ADMIN;
	    		IIGCn_static.highwater++;
			break;

		    case GCA_ID_SRV_KEY  :
			/*
			** We only save server keys of trusted servers.
			*/
			if ( ncb->flags & NCB_TRUSTED )
			{
			    if ( (ncb->srv.key = (char *)
			          MEreqmem( 0, aux_len + 1, FALSE, NULL )) )
			    {
				MEcopy( (PTR)aux_data, 
					aux_len, (PTR)ncb->srv.key );
				ncb->srv.key[ aux_len ] = EOS;
			        ncb->flags |= NCB_SERVER;
	    			IIGCn_static.highwater++;
			    }
			}
			break;

		    case GCA_ID_DELEGATED :
			ncb->deleg_len = aux_len;

			if ( (ncb->deleg = MEreqmem(0, aux_len, FALSE, NULL)) )
			    MEcopy( (PTR)aux_data, aux_len, ncb->deleg );
			else
			{
			    ncb->deleg = aux_data;
			    gcn_release_deleg( ncb, FALSE );
			}
			break;
		    }

		    ncb->parms.gca_ls_parms.gca_aux_data = 
						(PTR)(aux_data + aux_len);
		    ncb->parms.gca_ls_parms.gca_l_aux_data -= 
						aux_hdr.len_aux_data;
		}
	    }
	    break;

	case NSA_REPOST :
	    /*
	    ** Initial listen request has completed so clear the
	    ** active listen flag and start new session to post
	    ** new listen request.
	    */
	    gcn_listening = FALSE;
	    gcn_start_session();
	    break;

	 case NSA_RQRESP :
	    /*
	    ** Send connection accept or reject, 
	    ** depending on ncb->statusp.
	    */
            MEfill( sizeof(ncb->parms), 0, (PTR) &ncb->parms);
	    ncb->parms.gca_rr_parms.gca_assoc_id = ncb->io->assoc_id;
	    ncb->parms.gca_rr_parms.gca_request_status = *ncb->statusp;
	    ncb->parms.gca_rr_parms.gca_local_protocol = ncb->io->protocol;
	    ncb->statusp = &ncb->parms.gca_rr_parms.gca_status;

	    gca_call( &gca_cb, GCA_RQRESP, &ncb->parms, 
			GCA_ASYNC_FLAG, (PTR)ncb, -1, &ncb->stat );
	    if ( ncb->stat != OK )  break;
	    return;

	case NSA_REQUEST :
	    /*
	    ** Make a connection request (to remote name server).  
	    ** First buffer on send queue is GCN_RESOLVED message.
	    */
	    ncb->statusp = &ncb->parms.gca_rq_parms.gca_status;
	    mbuf = (GCN_MBUF *)(ncb->io->send.q_next);
            MEfill( sizeof(ncb->parms), 0, (PTR) &ncb->parms);
	    ncb->parms.gca_rq_parms.gca_partner_name = mbuf->data;
	    ncb->parms.gca_rq_parms.gca_peer_protocol = GCN_GCA_PROTOCOL_LEVEL;
	    ncb->parms.gca_rq_parms.gca_modifiers = GCA_RQ_RSLVD;

	    gca_call( &gca_cb, GCA_REQUEST, &ncb->parms, 
			GCA_ASYNC_FLAG, (PTR)ncb, -1, &ncb->stat );
	    if ( ncb->stat != OK )  break;
	    return;

	case NSA_RQ_RESUME :		/* Resume a request */
	    gca_call( &gca_cb, GCA_REQUEST, &ncb->parms, 
			GCA_RESUME, (PTR)ncb, -1, &ncb->stat );
	    if ( ncb->stat != OK )  break;
	    return;

	case NSA_RQ_DONE :		/* Connection request completed */
	    ncb->io->assoc_id = ncb->parms.gca_rq_parms.gca_assoc_id;
	    ncb->io->protocol = ncb->parms.gca_rq_parms.gca_peer_protocol;

	    /* Free GCN_RESOLVED message. */
	    QUinsert( QUremove( ncb->io->send.q_next ), &gcn_mbuf_que );

	    /* Check protocol level */
	    if ( *ncb->statusp == OK  &&  ncb->io->protocol < ncb->req_proto )
	    {
		ncb->statusp = &ncb->stat;
		ncb->stat = E_GC000A_INT_PROT_LVL; /* hijack this error code */
	    }
	    break;

	case NSA_ATTRIB :		/* connection attributes */
	    MEfill( sizeof(ncb->parms), 0, (PTR)&ncb->parms );
	    ncb->parms.gca_at_parms.gca_assoc_id = ncb->io->assoc_id;
	    ncb->statusp = &ncb->parms.gca_rq_parms.gca_status;

	    gca_call( &gca_cb, GCA_ATTRIBS, &ncb->parms, 
		      GCA_SYNC_FLAG, NULL, -1, &ncb->stat );
	    if ( ncb->stat == OK  &&
		 ncb->parms.gca_at_parms.gca_flags & GCA_HET )
		ncb->flags |= NCB_HETNET;
	    break;

	case NSA_FS_INIT :
	    /*
	    ** Initialize FASTSELECT parms.
	    **
	    ** First buffer on send queue is GCN_RESOLVED message.
	    ** First buffer on read queue is GCM message to send,
	    ** also receives GCM response.
	    */
            MEfill( sizeof(ncb->parms), 0, (PTR) &ncb->parms);
	    ncb->parms.gca_fs_parms.gca_modifiers = GCA_RQ_RSLVD | GCA_RQ_GCM;
	    ncb->parms.gca_fs_parms.gca_peer_protocol = GCN_GCA_PROTOCOL_LEVEL;

	    mbuf = (GCN_MBUF *)(ncb->io->send.q_next);
	    ncb->parms.gca_rq_parms.gca_partner_name = mbuf->data;

	    mbuf = (GCN_MBUF *)(ncb->io->read.q_next);
	    ncb->parms.gca_fs_parms.gca_buffer = mbuf->buf;
	    ncb->parms.gca_fs_parms.gca_b_length = mbuf->size;
	    ncb->parms.gca_fs_parms.gca_message_type = mbuf->type;
	    ncb->parms.gca_fs_parms.gca_msg_length = mbuf->used;
	    break;

	case NSA_FS_BEDCHK :
	    /*
	    ** Set BEDCHECK flag in FASTSELECT parms.
	    */
	    ncb->parms.gca_fs_parms.gca_modifiers |= GCA_CS_BEDCHECK;
	    break;

	case NSA_FASTSELECT :		/* Make GCA_FASTSELECT request */
	    ncb->statusp = &ncb->parms.gca_fs_parms.gca_status;
	    gca_call( &gca_cb, GCA_FASTSELECT, &ncb->parms, GCA_ASYNC_FLAG, 
	    		(PTR)ncb, IIGCn_static.bchk_msg_timeout, &ncb->stat );
	    if ( ncb->stat != OK )  break;
	    return;

	case NSA_FS_RESUME :		/* Resume a fastselect */
	    gca_call( &gca_cb, GCA_FASTSELECT, &ncb->parms, 
			GCA_RESUME, (PTR)ncb, -1, &ncb->stat );
	    if ( ncb->stat != OK )  break;
	    return;

	case NSA_FS_DONE :		/* FASTSELECT Completed */
	    mbuf = (GCN_MBUF *)(ncb->io->read.q_next);
	    mbuf->type = ncb->parms.gca_fs_parms.gca_message_type;
	    mbuf->len = ncb->parms.gca_fs_parms.gca_msg_length;
	    break;

	case NSA_RECEIVE :		/* Post a receive request */
	    ncb->statusp = &ncb->stat;
	    mbuf = gcn_add_mbuf((GCN_MBUF *)&ncb->io->read, FALSE, &ncb->stat);
	    if ( ncb->stat != OK )  break;

            MEfill( sizeof(ncb->parms), 0, (PTR) &ncb->parms );
	    ncb->parms.gca_rv_parms.gca_association_id = ncb->io->assoc_id;
	    ncb->parms.gca_rv_parms.gca_modifiers = 0;
	    ncb->parms.gca_rv_parms.gca_flow_type_indicator = GCA_NORMAL;
	    ncb->parms.gca_rv_parms.gca_buffer = mbuf->buf;
	    ncb->parms.gca_rv_parms.gca_b_length = mbuf->size;
	    ncb->statusp = &ncb->parms.gca_rv_parms.gca_status;

	    gca_call( &gca_cb, GCA_RECEIVE, &ncb->parms, 
			GCA_ASYNC_FLAG, (PTR)ncb, -1, &ncb->stat );
	    if ( ncb->stat != OK )  break;
	    return;

	case NSA_RV_DONE :
	    /*
	    ** Process message received.
	    ** Branch if more messages.
	    */
	    mbuf = (GCN_MBUF *)(ncb->io->read.q_prev);
	    mbuf->type = ncb->parms.gca_rv_parms.gca_message_type;
	    mbuf->data = ncb->parms.gca_rv_parms.gca_data_area;
	    mbuf->len = ncb->parms.gca_rv_parms.gca_d_length;

	    if ( ncb->io->protocol >= GCA_PROTOCOL_LEVEL_63 )
		branch = (! ncb->parms.gca_rv_parms.gca_end_of_group);
	    else
		branch = (! ncb->parms.gca_rv_parms.gca_end_of_data);
	    break;

	case NSA_SEND :			/* Send a message */
	    mbuf = (GCN_MBUF *)(ncb->io->send.q_next);
            MEfill( sizeof(ncb->parms), 0, (PTR) &ncb->parms );
	    ncb->parms.gca_sd_parms.gca_association_id = ncb->io->assoc_id;
	    ncb->parms.gca_sd_parms.gca_buffer = mbuf->buf;
	    ncb->parms.gca_sd_parms.gca_message_type = mbuf->type;
	    ncb->parms.gca_sd_parms.gca_msg_length = mbuf->used;
	    ncb->statusp = &ncb->parms.gca_sd_parms.gca_status;

	    if ( ncb->io->protocol >= GCA_PROTOCOL_LEVEL_63 )
	    {
		ncb->parms.gca_sd_parms.gca_end_of_data = TRUE;
		ncb->parms.gca_sd_parms.gca_modifiers = 
		    (ncb->io->send.q_next == ncb->io->send.q_prev) 
		    ? GCA_EOG : GCA_NOT_EOG;
	    }
	    else
	    {
		ncb->parms.gca_sd_parms.gca_modifiers = 0;
		ncb->parms.gca_sd_parms.gca_end_of_data = 
				(ncb->io->send.q_next == ncb->io->send.q_prev);
	    }

	    gca_call( &gca_cb, GCA_SEND, &ncb->parms, 
			GCA_ASYNC_FLAG, (PTR)ncb, -1, &ncb->stat );
	    if ( ncb->stat != OK )  break;
	    return;

	case NSA_SD_DONE :
	    /*
	    ** Send request completed.
	    ** Branch if more messages.
	    */
	    QUinsert( QUremove( ncb->io->send.q_next ), &gcn_mbuf_que );

	    if ( ncb->io->protocol >= GCA_PROTOCOL_LEVEL_63 )
		branch = (ncb->parms.gca_sd_parms.gca_modifiers & GCA_EOG) 
			 ? FALSE : TRUE;
	    else
		branch = (! ncb->parms.gca_sd_parms.gca_end_of_data);
	    break;

	case NSA_DISASSOC :
	    /*
	    ** Disconnect association.
	    */
            MEfill( sizeof(ncb->parms), 0, (PTR) &ncb->parms );
	    ncb->parms.gca_da_parms.gca_association_id = ncb->io->assoc_id;
	    ncb->io->assoc_id = -1;
	    ncb->statusp = &ncb->parms.gca_da_parms.gca_status;

	    gca_call( &gca_cb, GCA_DISASSOC, &ncb->parms, 
			GCA_ASYNC_FLAG, (PTR)ncb, -1, &ncb->stat );
	    if ( ncb->stat != OK ) break;
	    return;

	case NSA_DA_RESUME :
	    /*
	    ** Resume disassociate
	    */
	    gca_call( &gca_cb, GCA_DISASSOC, &ncb->parms, 
			GCA_RESUME, (PTR)ncb, -1, &ncb->stat );
	    if ( ncb->stat != OK )  break;
	    return;

	case NSA_SET_SHUT:		/* Prepare to shutdown Name Server */
	    if ( ncb->flags & NCB_QUIESCE )
		IIGCn_static.flags |= GCN_SVR_QUIESCE | GCN_SVR_CLOSED; 

	    if ( ncb->flags & NCB_SHUTDOWN )
		IIGCn_static.flags |= GCN_SVR_SHUT | GCN_SVR_CLOSED; 

	    *ncb->statusp = E_GC0040_CS_OK;
	    break;

	case NSA_SRV_START:
	{
	    GCN_TUP	tup;
	    char	*msg_in = ((GCN_MBUF *)ncb->io->read.q_next)->data;
	    char	*install, *class;
	    i4		flags, opcode, tupcnt;

	    /*
	    ** Server registration currently supports only
	    ** a single server class and listen address per
	    ** server.  Multiple server entries may exist
	    ** due to support for multiple objects (merged
	    ** entries).  We only need the server class and
	    ** listen address for the connection info, so
	    ** parse the GCN_NS_OPER message and retrieve
	    ** the class and address from the first entry.
	    */
	    msg_in += gcu_get_int( msg_in, &flags );	/* Flags */
	    msg_in += gcu_get_int( msg_in, &opcode );	/* OP code */
	    msg_in += gcu_get_str( msg_in, &install );	/* Installation */
	    msg_in += gcu_get_int( msg_in, &tupcnt );	/* Tuple count */

	    if ( opcode == GCN_ADD  &&  tupcnt > 0 )
	    {
		msg_in += gcu_get_str( msg_in, &class );/* Server class */
		msg_in += gcn_get_tup( msg_in, &tup );	/* Tup mask */

		/*
		** Server registration only logged/saved if
		** new or different than prior registration.
		*/
		if ( ! ncb->srv.class  ||  STcompare( class, ncb->srv.class) ||
		     ! ncb->srv.addr  ||  STcompare( tup.val, ncb->srv.addr) )
		{
		    /* Log server start-up */
		    ER_ARGUMENT	args[ 2 ];

		    args[ 0 ].er_value = class;
		    args[ 0 ].er_size = STlength( class );
		    args[ 1 ].er_value = tup.val;
		    args[ 1 ].er_size = STlength( tup.val );

		    gcu_erlog( ncb->session_id, IIGCn_static.language, 
			       E_GC0153_GCN_SRV_STARTUP, NULL, 2, (PTR)args );

		    /* Save server registration info */
		    if ( ncb->srv.class )  MEfree( (PTR)ncb->srv.class );
		    if ( ncb->srv.addr )  MEfree( (PTR)ncb->srv.addr );
		    ncb->srv.class = STalloc( class );
		    ncb->srv.addr = STalloc( tup.val );
		}
	    }
	    break;
	}

	case NSA_SRV_EXIT:
	    if ( ncb->srv.class  &&  ncb->srv.addr )
	    {
		/* Log server shutdown/failure */
		ER_ARGUMENT	args[ 2 ];
		GCN_QUE_NAME	*nq;

		args[ 0 ].er_value = (PTR)ncb->srv.class;
		args[ 0 ].er_size = STlength( ncb->srv.class );
		args[ 1 ].er_value = (PTR)ncb->srv.addr;
		args[ 1 ].er_size = STlength( ncb->srv.addr );

		if ( *ncb->statusp == OK )
		    gcu_erlog( ncb->session_id, IIGCn_static.language, 
			       E_GC0154_GCN_SRV_SHUTDOWN, NULL, 2, (PTR)args );
		else
		    gcu_erlog( ncb->session_id, IIGCn_static.language, 
			       E_GC0155_GCN_SRV_FAILURE, NULL, 2, (PTR)args );

		/*
		** Remove server registration.  An error here
		** is not critical (nor expected).  If the 
		** registration entry is not removed, it should 
		** be cleaned up by the next bedcheck.
		*/
	    	if ( ! (nq = gcn_nq_find( ncb->srv.class )) )
		{
		    if ( IIGCn_static.trace >= 1 )
			TRdisplay( "%4d   Can't find server class: %s\n",
				   ncb->session_id, ncb->srv.class );
		}
		else  if ( gcn_nq_lock( nq, TRUE ) != OK )
		{
		    if ( IIGCn_static.trace >= 1 )
			TRdisplay( "%4d   Can't lock class queue: %s\n",
				   ncb->session_id, ncb->srv.class );
		}
		else
		{
		    GCN_TUP	tup;
		    i4		row_count;

		    tup.uid = "";
		    tup.obj = "";
		    tup.val = ncb->srv.addr;

		    row_count = gcn_nq_del( nq, &tup );

		    if ( IIGCn_static.trace >= 3 )
			TRdisplay( "%4d   Dropped %d entries for %s %s\n", 
				   ncb->session_id, row_count, 
				   ncb->srv.class, ncb->srv.addr );

		    gcn_nq_unlock( nq );
		}
	    }

	    ncb->flags &= ~NCB_SERVER;
	    break;

	case NSA_DO_UPD:
	{
	    /*
	    ** Update Server statistics.
	    */
	    GCN_QUE_NAME *nq;
	    char	*msg_in = ((GCN_MBUF *)ncb->io->read.q_next)->data;
	    i4		load = -1;

	    if ( ((GCN_MBUF *)ncb->io->read.q_next)->len >= sizeof( i4 ) )
		msg_in += gcu_get_int( msg_in, &load );

	    if ( load >= 0  &&  ncb->srv.class  &&  ncb->srv.addr  &&
	         (nq = gcn_nq_find( ncb->srv.class )) )  
	    {
		if ( IIGCn_static.trace >= 3 )
		    TRdisplay( "%4d   Server upd: class %s, addr %s, load %d\n",
		    		ncb->session_id, ncb->srv.class, ncb->srv.addr,
				load );
	    	gcn_nq_assocs_upd( nq, ncb->srv.addr, load );
	    }
	    break;
	}

	case NSA_ADM_SESS: 
	    /*
	    ** Start an admin session.
	    */
	    *ncb->statusp = gcn_adm_session( ncb->io->assoc_id, 
					     gca_cb, ncb->username );
	    branch = (*ncb->statusp != OK);
	    break;

	case NSA_ADM_RLS: 
	    /*
	    ** formate gca_er_data for gca_release msg 
	    */
	    gcn_error( ncb->stat, (CL_ERR_DESC *)0, (char *)0,
		       (GCN_MBUF *)&ncb->io->send );
	    (( GCN_MBUF *)(&ncb->io->send))->type = GCA_RELEASE;
	    break;

	case NSA_FMT_RLS :		/* Format a GCA_RELEASE message */
	    mbuf = gcn_add_mbuf((GCN_MBUF *)&ncb->io->send, TRUE, ncb->statusp);

	    if ( *ncb->statusp != OK )
		branch = TRUE;
	    else
	    {
		mbuf->used = 0;
		mbuf->type = GCA_RELEASE;
	    }
	    break;

	case NSA_DO_OPER :		/* Process GCN_NS_OPER message */
	    *ncb->statusp = gcn_oper_ns( (GCN_MBUF *)ncb->io->read.q_next, 
				(GCN_MBUF *)&ncb->io->send, ncb->username, 
				(ncb->flags & NCB_TRUSTED) ? TRUE : FALSE );
	    if ( *ncb->statusp == E_GC0152_GCN_SHUTDOWN )
		IIGCn_static.flags |=  GCN_SVR_SHUT | GCN_SVR_CLOSED;
	    break;

	case NSA_DO_AUTH :		/* Process GCN_NS_AUTHORIZE message */
	    *ncb->statusp = gcn_make_auth( (GCN_MBUF *)ncb->io->read.q_next,
					   (GCN_MBUF *)&ncb->io->send,
					   ncb->io->protocol );
	    break;

	case NSA_RESOLVE :
	    /*
	    ** Process GCN_RESOLVE message.
	    ** Branch if error occurs.
	    */
	    branch = TRUE;	/* if error occurs, reset below if no error */

	    *ncb->statusp = gcn_rslv_init( &ncb->grcb, 
					   (GCN_MBUF *)ncb->io->read.q_next,
					   ncb->account, ncb->username );
	    if ( *ncb->statusp != OK )  break;

	    if( ncb->flags & NCB_EMBEDGCC )
		ncb->grcb.flags |= GCN_RSLV_EMBEDGCC;

	    if ( (*ncb->statusp = gcn_rslv_parse( &ncb->grcb )) != OK )
		break;

	    if ( (*ncb->statusp = gcn_rslv_server( &ncb->grcb )) != OK )
		break;

	    if ( (*ncb->statusp = gcn_rslv_node( &ncb->grcb )) != OK )
		break;

	    if ( (*ncb->statusp = gcn_rslv_login( &ncb->grcb )) != OK )
		break;

	    if ( (*ncb->statusp = gcn_rslv_attr( &ncb->grcb )) != OK )
		break;

# ifdef NT_GENERIC
	    {
	    /*
	    ** A user/password entry dialog is displayed (Windows only) if
	    ** user is set to one of the IIPROMPT* variations.  The userid
	    ** and/or the password entered by the user may be longer than
	    ** the input "usr" and/or "pwd" parms, in which case a longer
	    ** field(s) will be allocated (by GCpasswd_prompt). If so, the
	    ** address(es) after return from GCpasswd_prompt will be different
	    ** and the original user (and/or password) field must be freed
	    ** if it was dynamically allocated (indicated by usrptr != usrbuf
	    ** and/or pwdptr != pwdbuf).
	    ** NOTE: Entering this code, usr can = usrptr or username.
	    **       usrptr may point to predefined area usrbuf[] or
	    **       may have been allocated (if usrbuf[] too small);
	    **       it is this last case we need to catch here (ie,
	    **       reallocating usrptr to a larger area).
	    **       When usr = username (server-side only), IIPROMPT*
	    **       "shouldn't" be the value at that time.  Nevertheless,
	    **       for generality, we treat it the same way and set
	    **       usrptr to the newly allocated area.
	    **       Other parts of the code will clean up the dynamically
	    **       allocated areas whenever usrptr != usrbuf and/or
	    **       pwdptr != pwdbuf.
	    */
	    char *usr_save = ncb->grcb.usr;
	    char *pwd_save = ncb->grcb.pwd;

            GCpasswd_prompt( &ncb->grcb.usr, &ncb->grcb.pwd, ncb->grcb.vnode );

	    if ( usr_save != ncb->grcb.usr )    /* if new user area alloc'd */
	    {
		if (ncb->grcb.usrptr != ncb->grcb.usrbuf) /* if old alloc*/
		{
		    if ( IIGCn_static.trace >= 3 )
			TRdisplay( "%4d   GCN Free old user area after GCpasswd_prompt() realloc'd for vnode=%s\n",
		    		ncb->session_id, ncb->grcb.vnode);
		    MEfree( (PTR)ncb->grcb.usrptr );
		}
		ncb->grcb.usrptr = ncb->grcb.usr;  /* Set usrptr=>new area*/
	    }  /* end if new user area alloc'd. */

	    if ( pwd_save != ncb->grcb.pwd )    /* if new passwd area alloc'd */
	    {
		if (ncb->grcb.pwdptr != ncb->grcb.pwdbuf) /* if old alloc*/
		{
		    if ( IIGCn_static.trace >= 3 )
			TRdisplay( "%4d   GCN Free old pwd area after GCpasswd_prompt() realloc'd for vnode=%s\n",
		    		ncb->session_id, ncb->grcb.vnode);
		    MEfree( (PTR)ncb->grcb.pwdptr );
		}
		ncb->grcb.pwdptr = ncb->grcb.pwd;  /* Set pwdptr=>new area*/
	    }  /* end if new password area alloc'd. */

	    } /* end context block for NT specific code */
# endif

	    branch = FALSE;	/* no error occured */
	    break;

	case NSA_RSLV_DONE :		/* Prepare GCN_RESOLVED message */
	    *ncb->statusp = gcn_rslv_done( &ncb->grcb, 
					   (GCN_MBUF *)&ncb->io->send, 
					   ncb->io->protocol,
					   ncb->deleg_len, ncb->deleg );
	    branch = (*ncb->statusp != OK);
	    break;

	case NSA_RSLV_ERR :		/* Process resolve request error */
	    gcn_rslv_err( &ncb->grcb, *ncb->statusp );
	    break;

	case NSA_USE_AUTH :
	    /*
	    ** Retrieve cached authentication ticket.
	    ** Branch if ticket available.
	    */
	    *ncb->statusp = gcn_use_auth( &ncb->grcb, ncb->io->protocol );
	    branch = (*ncb->statusp != OK);
	    break;

	case NSA_GET_AUTH :
	    /* 
	    ** Prepare GCN_AUTHORIZE message 
	    ** Branch if error.
	    */
	    ncb->req_proto = GCA_PROTOCOL_LEVEL_55;	/* required protocol */
	    *ncb->statusp = gcn_get_auth( &ncb->grcb, 
	    				  (GCN_MBUF *)&ncb->io->send );
	    branch = (*ncb->statusp != OK);
	    break;

	case NSA_GOT_AUTH :
	    /*
	    ** Process GCN_NS_AUTHORIZED message.
	    ** Branch if error.
	    */
	    *ncb->statusp = gcn_got_auth( &ncb->grcb, 
					  (GCN_MBUF *)ncb->io->read.q_prev );
	    branch = (*ncb->statusp != OK);
	    break;

	case NSA_IP_PROTO :
	    /*
	    ** Check for installation password protocol error.
	    */
	    if ( *ncb->statusp == E_GC000B_RMT_LOGIN_FAIL  ||
		 *ncb->statusp == E_GC000A_INT_PROT_LVL )
	    {
		ncb->statusp = &ncb->stat;
		ncb->stat = E_GC0140_GCN_INPW_NOPROTO;
	    }
	    break;

	case NSA_DIRECT :
	    /* 
	    ** Prepare GCN_RESOLVE message 
	    ** Branch if error.
	    */
	    ncb->req_proto = GCA_PROTOCOL_LEVEL_63;	/* required protocol */
	    *ncb->statusp = gcn_direct( &ncb->grcb, (GCN_MBUF *)&ncb->io->send,
	    				ncb->deleg_len, ncb->deleg );
	    branch = (*ncb->statusp != OK);
	    break;

	case NSA_RESOLVED :
	    /*
	    ** Process GCN2_RESOLVED message.
	    ** Branch if error.
	    */
	    *ncb->statusp = gcn_resolved( &ncb->grcb,
					  (GCN_MBUF *)ncb->io->read.q_prev );
	    branch = (*ncb->statusp != OK);
	    break;

	case NSA_DC_PROTO :
	    /*
	    ** Check for direct connect protocol error.
	    */
	    if ( *ncb->statusp == E_GC000A_INT_PROT_LVL )
	    {
		ncb->statusp = &ncb->stat;
		ncb->stat = E_GC0162_GCN_DC_VERSION;
	    }
	    else  if ( *ncb->statusp == OK  &&  ncb->flags & NCB_HETNET )
	    {
		ncb->statusp = &ncb->stat;
		ncb->stat = E_GC0161_GCN_DC_HET;
	    }
	    break;

	case NSA_DC_ERROR :		/* Generate direct connect error */
	    ncb->grcb.flags &= ~GCN_RSLV_DIRECT;
	    *ncb->statusp = E_GC0160_GCN_NO_DIRECT;
	    break;

	case NSA_OPER_BCHK :		
	    /* 
	    ** Branch if operation does not require bedchecking.
	    ** Requests to the network database do not require 
	    ** bedchecking.  These requests may be flagged with 
	    ** GCN_NET_FLAG. If they aren't, it still shouldn't 
	    ** hurt to bedcheck.
	    */
	    {
		i4	flag;

		mbuf = (GCN_MBUF *)ncb->io->read.q_next;
		gcu_get_int( mbuf->data, &flag );

		if ( ! (IIGCn_static.flags & GCN_BCHK_CONN)  ||  
		     flag & GCN_NET_FLAG )
		    branch = TRUE;
		else
		    branch = (! gcn_oper_check((GCN_MBUF *)ncb->io->read.q_next,
						&ncb->bchk_q ));
	    }
	    break;

	case NSA_BEDCHECK :
	    /*
	    ** Branch if bedcheck is not required.
	    */
	    branch = ( ! (IIGCn_static.flags & GCN_BCHK_CONN)  ||
		       ! gcn_bchk_all( &ncb->bchk_q ) );
	    break;

	case NSA_BCHK_NEXT :
	    /*
	    ** Build connection info and GCM_GET request for bedcheck.
	    ** Branch if no more servers or error.
	    */
	    *ncb->statusp = gcn_bchk_next( &ncb->bchk_q, &ncb->grcb,
					   (GCN_MBUF *)&ncb->io->send,
					   (GCN_MBUF *)&ncb->io->read );
	    branch = (*ncb->statusp != OK);
	    break;

	case NSA_BCHK_CHECK :
	    /*
	    ** Extract and validate bedcheck results.
	    */
	    {
		char	bchk_install[4], bchk_class[GCA_SERVER_ID_LNG];
		i4	bchk_load = -1;

		bchk_install[0] = bchk_class[0] = EOS;

		if ( *ncb->statusp == E_GC0000_OK )
		{
		    i4  	stat, elem;
		    char 	*p;

		    /* 
		    ** Fastselect receives response into same
		    ** buffer used to send fastselect request.
		    */
		    mbuf = (GCN_MBUF *)(ncb->io->read.q_next);
		    p = mbuf->data;	/* point to start of GCM message */
		    p += gcm_get_int( p, &stat );	/* get status */
		    p += sizeof( i4 );	/* bump past error_index */
		    p += sizeof( i4 );	/* bump past future[0] */
		    p += sizeof( i4 );	/* bump past future[1] */
		    p += sizeof( i4 );	/* bump past client_perms */
		    p += sizeof( i4 );	/* bump past row_count */
		    p += gcm_get_int( p, &elem );	/* get element count */

		    /* if GCM GET succeeded, get server load */
		    if ( stat )  elem = 0;

		    while( elem-- > 0 )
		    {
			char	str[ 255 ];
			char	*class, *value;
			i4	len_c, len_v;

			p += gcm_get_str( p, &class, &len_c );	/* classid */
			p += gcm_get_str( p, &value, &len_v );	/* ignored */
			p += gcm_get_str( p, &value, &len_v );	/* value */
			p += sizeof( i4 );		/* bump past perms */

			MEcopy( class, len_c, str );
			str[ len_c ] = EOS;

			if ( ! STcompare( str, GCA_MIB_NO_ASSOCS ) )
			{
			    len_v = min( len_v, sizeof( str ) - 1 );
			    MEcopy( value, len_v, str );
			    str[ len_v ] = EOS;
			    CVan( str,  &bchk_load );
			}
			else  if ( ! STcompare( str, GCA_MIB_INSTALL_ID ) )
			{
			    len_v = min( len_v, sizeof( bchk_install ) - 1 );
			    MEcopy( value, len_v, bchk_install );
			    bchk_install[ len_v ] = EOS;
			}
			else  if ( ! STcompare(str, GCA_MIB_CLIENT_SRV_CLASS) )
			{
			    len_v = min( len_v, sizeof( bchk_class ) - 1 );
			    MEcopy( value, len_v, bchk_class );
			    bchk_class[ len_v ] = EOS;
			}
		    }
		}

		/*
		** Check results of bedcheck.
		*/
		gcn_bchk_check( &ncb->bchk_q, *ncb->statusp, 
				bchk_load, bchk_install, bchk_class );
	    }
	    break;

	case NSA_COMPRESS :
	    /*
	    ** Compress (at most) one name queue file.
	    ** Branch if file compressed.
	    */
	    {
		QUEUE *q;

		for(
		     q = IIGCn_static.name_queues.q_next;
		     q != &IIGCn_static.name_queues;
		     q = q->q_next
		   )
		    if ( gcn_nq_compress( (GCN_QUE_NAME *)q ) )
		    {
			branch = TRUE;
			break;
		    }
	    }
	    break;

	case NSA_EXPIRE :
	    /*
	    ** Check for installation password ticket expirations.
	    ** At most one ticket file is processed, so we attempt
	    ** to alternate the processing each time through.
	    ** Branch if ticket expiration occurs.
	    */
	    {
		GCN_QUE_NAME *nq;

		/*
		** First try to process the target file.
		*/
		nq = gcn_nq_find( expire_lticket ? GCN_LTICKET : GCN_RTICKET );
		
		if ( nq  &&  gcn_expire( nq ) )
		{
		    expire_lticket = ! expire_lticket;	/* alternate file */
		    branch = TRUE;
		}
		else
		{
		    /*
		    ** Try alternate file if target didn't need processing.
		    */
		    nq = gcn_nq_find( expire_lticket ? GCN_RTICKET 
						     : GCN_LTICKET );
		    if ( nq  &&  gcn_expire( nq ) )  branch = TRUE;
		}
	    }
	    break;

	case NSA_DO_REG :
	    /*
	    ** Check intallation registry mode and resources 
	    ** required for registry participation.
	    ** Branch if registry inactive or nothing to do.
	    */
	    if ( IIGCn_static.flags & GCN_REG_ACTIVE )
	    {
		if ( TMcmp( &IIGCn_static.now, 
			    &IIGCn_static.registry_check ) < 0 )
		    branch = TRUE;		/* Nothing to do */
	    }
	    else
	    {
		GCN_QUE_NAME	*nq;

		/*
		** If configured to participate in installation
		** registry, we don't become active until there
		** is a Comm Server available to handle registry
		** requests.
		*/
		if ( IIGCn_static.registry_type == GCN_REG_NONE  ||
		     ! (nq = gcn_nq_find( GCN_COMSVR ))  ||
		     gcn_nq_lock( nq, FALSE ) != OK )
		{
		    branch = TRUE;
		}
		else
		{
		    GCN_TUP	tup, tupmask;

		    /* See if there is a Comm Server registered */
		    tupmask.uid = tupmask.obj = tupmask.val = "";

		    if ( gcn_nq_ret( nq, &tupmask, 0, 1, &tup ) > 0 )
			IIGCn_static.flags |= GCN_REG_ACTIVE;
		    else
			branch = TRUE;

		    gcn_nq_unlock( nq );
		}
	    }

	    /*
	    ** If we are going to do a registry check,
	    ** set the next registry check time.  The
	    ** check requires a number of operations;
	    ** setting the time now keeps everyone else
	    ** out while the check is in progress.
	    */
	    if ( ! branch )
	    {
		STRUCT_ASSIGN_MACRO( IIGCn_static.now, 
				     IIGCn_static.registry_check );
		IIGCn_static.registry_check.TM_secs += 
						IIGCn_static.bedcheck_interval;
	    }
	    break;

	case NSA_GET_COMSVR :
	    /*
	    ** Load local connection catalogs for Comm Servers.
	    ** Branch if no Comm Servers available.
	    */
	    if ( ! gcn_ir_comsvr( &ncb->grcb ) )
	    {
		/*
		** Deactivate registry until there 
		** is a Comm Server available.
		*/
		IIGCn_static.flags &= ~GCN_REG_ACTIVE;
		gcn_ir_save( IIGCn_static.install_id, 
			     IIGCn_static.listen_addr );
		branch = TRUE;
	    }
	    break;

	case NSA_CHK_MASTER :
	    /*
	    ** Build connection info and GCM request
	    ** to check the master Name Server.
	    ** Branch if error.
	    */
	    *ncb->statusp = gcn_ir_bedcheck( &ncb->grcb, 
	    				     (GCN_MBUF *)&ncb->io->send,
					     (GCN_MBUF *)&ncb->io->read );
	    branch = (*ncb->statusp != OK);
	    break;

	case NSA_REG_ERROR :
	    /*
	    ** Check for GCM error response.
	    ** Branch if error.
	    */
	    *ncb->statusp = gcn_ir_error( (GCN_MBUF *)ncb->io->read.q_next );
	    branch = (*ncb->statusp != OK);
	    break;

	case NSA_GET_MASTER :
	    /*
	    ** Extract master Name Server address 
	    ** and compare with our cached value.
	    ** Branch if no change in master.
	    */
	    branch = ( ! gcn_ir_update( (GCN_MBUF *)ncb->io->read.q_prev ) );
	    break;

	case NSA_REGISTER :
	    /*
	    ** Build connection info and GCN_OPER request
	    ** to register with master Name Server.
	    ** Branch if error.
	    */
	    *ncb->statusp = gcn_ir_register( &ncb->grcb, 
					     (GCN_MBUF *)&ncb->io->send );
	    branch = (*ncb->statusp != OK);
	    break;

	case NSA_IF_SLAVE :
	    /*
	    ** Branch if installation is not peer or master.
	    */
	    gcn_ir_save( NULL, IIGCn_static.listen_addr );
	    branch = (IIGCn_static.registry_type != GCN_REG_PEER  &&
		      IIGCn_static.registry_type != GCN_REG_MASTER);
	    break;
	
	case NSA_BE_MASTER :
	    /*
	    ** Build connection info and GCM request to
	    ** set Comm Server in master registry mode.
	    ** Branch if error.
	    */
	    *ncb->statusp = gcn_ir_master( &ncb->grcb, 
	    				   (GCN_MBUF *)&ncb->io->send,
					   (GCN_MBUF *)&ncb->io->read );
	    branch = (*ncb->statusp != OK);
	    break;

	case NSA_REG_RETRY :
	    /*
	    ** On first retry reset check time and branch,
	    ** otherwise continue (to avoid infinite loop).
	    */
	    if ( ! (IIGCn_static.flags & GCN_REG_RETRY) )
	    {
		IIGCn_static.flags |= GCN_REG_RETRY;
		STRUCT_ASSIGN_MACRO( IIGCn_static.now, 
				     IIGCn_static.registry_check );
		branch = TRUE;
	    }
	    break;

	case NSA_CLEAR_REG :
	    /*
	    ** Reset registry retry flag.
	    */
	    IIGCn_static.flags &= ~GCN_REG_RETRY;
	    break;
    }

    if ( branch )  
	ncb->state = gcn_labels[ gcn_states[ ncb->state - 1 ].label ].state;

    goto top;
}


/*{
** Name: gcn_register() - register for IIGCN listen address
**
** Description:
**	Does GCA housekeeping: GCA_INITIATE, GCA_REGISTER.
**	Sets IIGCN listen address with GCnsid().
**
** Outputs:
**	myaddr	- listen address of IIGCN
**
** Side effects:
**	GCnsid() saves II_GCNxx_PORT somewhere.
**
** Called by:
**	main()
**
** History:
**	21-Mar-89 (seiwald)
**	    Written.
**	02-Jul-91 (johnr)
**	    Changed the initialization of zeroparms struct in a generalized way.
**	    Replaced STRUCT_ASSIGN_MACRO with MEfill macro.
**	31-Jan-92 (seiwald)
**	    Call GCA_INITIATE at GCA_API_LEVEL_3 and GCN_GCA_PROTOCOL_LEVEL.
**	    Initialize must be handed the proper protocol level or else 
**	    GCA_LISTEN will use GCA_PROTOCOL_LEVEL, which is now several 
**	    levels old.
**	25-Sep-92 (brucek)
**	    Call GCA_INITIATE at GCA_API_LEVEL_4 for GCM support.
**	 3-Sep-96 (gordy)
**	    Removed gcn_bedcheck() call, it is now done in main().
**	19-Mar-97 (gordy)
**	    Move to API LEVEL 5 as required to fix the GCN protocol.
**	29-May-97 (gordy)
**	    No longer need to register GCA AUTH function.  This is
**	    now done through GCS by main().
**	26-Aug-98 (gordy)
**	    Request auth delegation from GCA_LISTEN.
**	15-Sep-98 (gordy)
**	    Permit anyone to do GCM operations for installation registry.
**	    Use GCA_NMSVR_CLASS as generic Name Server.
**	22-Mar-02 (gordy)
**	    Use NS classes rather than deprecated GCA classes.
**	15-Oct-03 (gordy)
**	    Bump API level to 6 to RESUME FASTSELECT requests.
**	17-Dec-2009 (Bruce Lunsford) Sir 122536
**	    Remove unref'd local variables assoc_no + tmp_stat.
*/

STATUS
gcn_register( char *myaddr )
{
    GCA_PARMLIST	parms;
    STATUS		status;
    CL_ERR_DESC		sys_err;
    i4			i;

    /*
    ** Local initialization.
    */
    QUinit( &gcn_mbuf_que );
    QUinit( &gcn_ncb_que );
    QUinit( &gcn_ncb_free );

    /*
    ** Initialize the label jump table.  Point directly to the
    ** label location so that the label state will be picked up.
    */
    for( i = 0; i < sizeof( gcn_states ) / sizeof( gcn_states[0] ); i++ )
	if ( gcn_states[ i ].action == NSA_LABEL )
	    gcn_labels[ gcn_states[ i ].label ].state = i;

    /*
    ** Initialize GCA.
    */
    MEfill( sizeof(parms), 0, (PTR) &parms);
    parms.gca_in_parms.gca_modifiers = GCA_AUTH_DELEGATE | GCA_GCM_READ_ANY;
    parms.gca_in_parms.gca_local_protocol = GCN_GCA_PROTOCOL_LEVEL;
    parms.gca_in_parms.gca_normal_completion_exit = gcn_server;
    parms.gca_in_parms.gca_expedited_completion_exit = gcn_server;
    parms.gca_in_parms.gca_modifiers |= GCA_API_VERSION_SPECD;
    parms.gca_in_parms.gca_api_version = GCA_API_LEVEL_6;

    gca_call(&gca_cb, GCA_INITIATE, &parms, GCA_SYNC_FLAG, NULL, -1, &status );
			
    if ( status != OK || (status = parms.gca_in_parms.gca_status) != OK )
    {
	if ( status == OK )  status = parms.gca_in_parms.gca_status;
	gcu_erlog( 0, IIGCn_static.language, status, 
		   &parms.gca_in_parms.gca_os_status, 0, NULL );
	return( status );
    }

    /* 
    ** Reserve the Name Server ID.
    */
    if ( GCnsid( GC_RESV_NSID, (char *)NULL, 0, &sys_err ) != OK )
    {
	gcu_erlog( 0, IIGCn_static.language, 
		   E_GC0122_GCN_2MANY, &sys_err, 0, NULL );
	return( E_GC0122_GCN_2MANY );
    }

    /*
    ** Create a listen address.
    */
    MEfill( sizeof(parms), 0, (PTR) &parms);
    parms.gca_rg_parms.gca_srvr_class = GCN_NS_REG;
    parms.gca_rg_parms.gca_l_so_vector = 0;
    parms.gca_rg_parms.gca_served_object_vector = NULL;
    parms.gca_rg_parms.gca_installn_id = "";
    parms.gca_rg_parms.gca_modifiers = GCA_RG_NO_NS | GCA_RG_IINMSVR;
	    
    gca_call( &gca_cb, GCA_REGISTER, &parms, GCA_SYNC_FLAG, NULL, -1, &status );

    if ( status != OK || (status = parms.gca_rg_parms.gca_status) != OK )
    {
	if ( status == OK )  status = parms.gca_rg_parms.gca_status;
	gcu_erlog( 0, IIGCn_static.language, 
		   status, &parms.gca_rg_parms.gca_os_status, 0, NULL );
	return( status );
    }

    /* 
    ** Make our listen address globally accessible.
    */
    STcopy( parms.gca_rg_parms.gca_listen_id, myaddr );

    if ( GCnsid(GC_SET_NSID, myaddr, STlength( myaddr ), &sys_err) != OK )
    {
	gcu_erlog( 0, IIGCn_static.language, 
		   E_GC0123_GCN_NSID_FAIL, &sys_err, 0, NULL );
	return( E_GC0123_GCN_NSID_FAIL );
    }

    return( OK );
}

/*{
** Name: gcn_deregister() -
**
** Description:
**      Does GCA housekeeping: GCA_TERMINATE.
**
** Outputs:
**      status  - OK, or failure status from GCA_TERMINATE.
**
** Side effects:
**      N/A.
**
** Called by:
**      main()
**
** History:
**      15-Apr-2009 (hanal04) Bug 121931
**          Written.
*/
STATUS
gcn_deregister(VOID)
{
    GCA_PARMLIST	parms;
    STATUS		status;

    IIGCn_static.flags |= GCN_SVR_TERM;

    /*
    ** Initialize GCA_PARMLIST.
    */
    MEfill( sizeof(parms), 0, (PTR) &parms);

    gca_call(&gca_cb, GCA_TERMINATE, &parms, GCA_SYNC_FLAG, NULL, -1, &status );

    if ( status != OK || (status = parms.gca_in_parms.gca_status) != OK )
    {
        gcu_erlog( 0, IIGCn_static.language, status,
                   &parms.gca_in_parms.gca_os_status, 0, NULL );
        return( status );
    }

    return(OK);
}


/*
** Name: gcn_start_sesss
**
** Description:
**	Allocate an NCB for a new GCN session and start it running.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	15-Aug-04 (wansh01)
**	    Created.
**	 1-Mar-06 (gordy)
**	    Dynamically allocate NCBs.
*/

void  
gcn_start_session()
{
    GCN_NCB	*ncb;
    i4		sid;

    /*
    ** Get a free NCB or allocate new NCB.
    */
    if ( (ncb = (GCN_NCB *)QUremove( gcn_ncb_free.q_next )) )
    	sid = ncb->session_id;		/* Save SID for re-use */
    else if ( (ncb = (GCN_NCB *)MEreqmem(0, sizeof( GCN_NCB ), FALSE, NULL)) )
	sid = ++IIGCn_static.session_count;
    else
    {
	/*
	** Failure to start a session may result in no active
	** listen.  The next free session will be used to repost
	** the listen (see NSA_EXIT), so the error is not critical.
	*/
	ER_ARGUMENT	er_arg;

	if ( IIGCn_static.trace >= 1 )
	    TRdisplay( "%4d   GCN NCB allocation error.\n", -1 );

	er_arg.er_value = ERx("NCB");
	er_arg.er_size = STlength( er_arg.er_value );
	gcu_erlog( 0, IIGCn_static.language, 
		   E_GC0121_GCN_NOMEM, NULL, 1, (PTR)&er_arg );
	return;
    }

    /*
    ** Basic NCB initialization - full init done in NSA_INIT.
    */
    MEfill( sizeof( GCN_NCB ), 0, (PTR)ncb );
    ncb->session_id = sid;
    QUinsert( &ncb->q, &gcn_ncb_que );

    gcn_server( (PTR)ncb );		/* Start session */
    return;
}


/*
** Name: gcn_get_server
**
** Description:
**	Search sessions for registered server with the desired
**	listen address.  Server's key is returned if found.
**
** Input:
**	addr	Listen address.
**
** Output:
**	None.	
**
** Returns:
**	char *	Server key.  NULL if no server or server has no key.
**
** History:
**	 1-Mar-06 (gordy)
**	    Created.
*/

char *
gcn_get_server( char *addr )
{
    QUEUE	*q;

    /*
    ** Search NCB's for active registered server with
    ** the target listen address.
    */
    for( q = gcn_ncb_que.q_next; q != &gcn_ncb_que; q = q->q_next )
    {
    	GCN_NCB	*ncb = (GCN_NCB *)q;

        if ( ncb->flags & NCB_SERVER  &&  ncb->srv.addr  &&
	     ! STcompare( addr, ncb->srv.addr ))
	    return( ncb->srv.key );
    }

    return( NULL );
}


/*
** Name: gcn_add_mbuf
**
** Description:
**	Get an MBUF off gcn_mbuf_que and insert into caller's queue.
**	Format as GCA buffer if caller asks.
**
** Input:
**	format		TRUE if format for GCA send.
**
** Output:
**	mbuf_head	Queue to receive buffer.
**	status		Error code if NULL returned.
**
** Returns:
**	GCN_MBUF *	Buffer, NULL if error.
**
** History:
**	19-Mar-97 (gordy)
**	    Don't call GCA_FORMAT at API_LEVEL_5 and above.
*/

GCN_MBUF *
gcn_add_mbuf( GCN_MBUF *mbuf_head, bool format, STATUS *status )
{
    GCN_MBUF	*mbuf;

    *status = OK;
    mbuf = (GCN_MBUF *)QUremove( gcn_mbuf_que.q_next );

    if ( ! mbuf )
    {
	mbuf = (GCN_MBUF *)MEreqmem( 0, sizeof( GCN_MBUF ), 0, status );
	if ( ! mbuf )  return( NULL );
	mbuf->size = sizeof( mbuf->buf );
    }

    QUinsert( &mbuf->q, mbuf_head->q.q_prev );

    if ( format )
    {
	mbuf->data = mbuf->buf;
	mbuf->len = mbuf->size;
    }

    return( mbuf );
}


/*
** Name: gcn_release_deleg
**
** Description:
**	Calls GCS to release delegation token.  Free memory 
**	allocated for token if requested.
**
** Input:
**	ncb		Name Server connection control block.
**	free		Free memory if TRUE.
**
** Output:
**	None.
**
** Returns:
**	Void
**
** History:
**	 4-Sep-98 (gordy)
**	    Created.
**	13-Aug-09 (gordy)
**	    Moved delegated authentication to NCB.  Make sure deleg exists.
*/

static VOID
gcn_release_deleg( GCN_NCB *ncb, bool free )
{
    GCS_REL_PARM	rel;
    STATUS		status;

    if ( ncb->deleg_len  &&  ncb->deleg )
    {
	rel.length = ncb->deleg_len;
	rel.token = ncb->deleg;

	status = IIgcs_call( GCS_OP_RELEASE, GCS_NO_MECH, (PTR)&rel );

	if ( status != OK )
	    gcu_erlog( -1, IIGCn_static.language, status, NULL, 0, NULL );

	if ( free )  MEfree( ncb->deleg );
    }

    ncb->deleg = NULL;
    ncb->deleg_len = 0;
    return;
}


/*
** Name: gcn_sess_info
**
** Description:
**
** Input:
**	ptr	ptr to formatted info lines  
**	i4 	flag to request system/user count or information.
**
** Output:
**	None.
**
** Returns:
**	session counts
**
** History:
**	15-Jun-04 (wansh01)
**	    Created.
**	 5-Aug-04 (gordy)
**	    Include association ID in session info.
**	22-Jun-06 (rajus01)
**	    Include server class for system sessions to identify 
**	    registered server sessions in the SHOW SYSTEM SESSIONS 
**	    command for GCN. 
**	23-Sep-06 (rajus01)
**	    Added server id info. Removed gcn_verify_sid(),gcn_get_sess_info().
**	    Gordy's rework of gcnsadm.c eliminated the need for these routines. 
**	 3-Aug-09 (gordy)
**	    Reference user ID, server class and address rather than copying.
*/

i4  
gcn_sess_info( PTR info, i4 sess_flag   )
{
    GCN_SESS_INFO	*sess = (GCN_SESS_INFO *)info; 
    QUEUE		*q;
    i4			count = 0; 
    i4			j, state;
    bool	is_system = FALSE;

    for( q = gcn_ncb_que.q_next; q != &gcn_ncb_que; q = q->q_next )
    {
        GCN_NCB	*ncb = (GCN_NCB *)q;
	bool	selected = FALSE;

	if ( (state = ncb->state) != 0 )
	{
	    if ( IIGCn_static.trace >= 4 )
		 TRdisplay(" gcn_sess_info %d %d %d\n", ncb->session_id,
		  ncb->state, ncb->io->assoc_id );
	    
	    switch ( sess_flag )   
	    {
	    case GCN_SYS_COUNT:
		if ( ! (ncb->flags & NCB_USER) )
                {
                    count++;
                    is_system = TRUE;
                }
		break;

	    case GCN_SYS_INFO:
		if ( ! (ncb->flags & NCB_USER) ) 
		{
		    selected = TRUE; 
                    is_system = TRUE;
		    count++;
		}
		break;

	    case GCN_USER_COUNT:
		if ( ncb->flags & NCB_USER )  count++;
		break;

	    case GCN_USER_INFO:
		if ( ncb->flags & NCB_USER ) 
		{
		    selected = TRUE; 
		    count++;
		}
		break;

	    case GCN_ALL_COUNT:
	        if ( ! (ncb->flags & NCB_USER) ) 
                    is_system = TRUE;
		count++;
		break;

	    case GCN_ALL_INFO:
	        if ( ! (ncb->flags & NCB_USER) ) 
                    is_system = TRUE;
		selected = TRUE; 
		count++;
		break;
	    }

	    if ( selected ) 
	    {
		sess->sess_id = (PTR)ncb;     /* NS control block hex address */
		sess->assoc_id = ncb->io->assoc_id;  /* NS GCA association ID */
		sess->user_id = ncb->username;
		sess->is_system = is_system;

                if ( is_system )
                {
		   sess->svr_class = ncb->srv.class ? ncb->srv.class : "";
		   sess->svr_addr = ncb->srv.addr ? ncb->srv.addr : "";
		}

		sess->state[0] = EOS;

		for( j = 0; j <= ncb->sp; j++ )
		{
		    if ( j )  STcat( sess->state, "/" );
		    STcat( sess->state, 
		       gcn_labels[ ncb->labels[j] ].desc );
		}

		if ( IIGCn_static.trace >= 4 )
		    TRdisplay( "gcn_sess_info user=%s, state=%s\n", 
			       sess->user_id, sess->state ); 

		sess++;
	    }
	}

        is_system = FALSE;
        
    }

    if ( IIGCn_static.trace >= 4 )
	TRdisplay( "gcn_sess_info count= %d \n", count); 

    return( count );
}
