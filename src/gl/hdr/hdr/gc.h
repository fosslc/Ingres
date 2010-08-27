/*
** Copyright (c) 2004, 2009 Ingres Corporation
*/

# ifndef GC_HDR_INCLUDED
# define GC_HDR_INCLUDED

#include    <gccl.h>

/*
** Name: GC.H - Definitions used by GCA CL.
**
** Description:
**      Definitions in this file are for GCA and the GCA CL use only.
**
** History: 
**      01-Oct-87 (jbowers)
**          Created from GCAINTERNAL.H to contain structures and constants
**	    shared between mainline and CL code.
**      01-Jul-88 (jbowers)
**          Added changes to support Communication Server connections: added
**	    GCA_AUX_DATA, modified GCA_PEER_INFO.
**      21-Oct-88 (jbowers)
**          Added new aux data types.
**      01-Nov-88 (jbowers)
**          Modified GCA_RMT_ADDR aux data to contain the remote user id and
**	    password.  Added internal protocol id and server return status to
**	    GCA_PEER_INFO. 
**	05-Nov-88 (jorge)
**	    Added definition for GC_RESV_NSID to handle GCnsid startup
**	    race conditions. Changed GC_xxx_LISTEN to GC_xxx_NSID for
**	    clarity.
**      06-Mar-89 (jbowers)
**          Added "chop_mks_owed" flag to the ACB to deal with yet another
**	    purging race condition discovered in the comm server.
**	29-Sep-89 (seiwald)
**	    New svc_resume pointer in ACB for stashing SVC_PARMS until
**	    GCA_RESUME.
**      05-apr-1989 (jennifer)
**          Added auxiliary data type for security label.
**      27-sep-1989 (jennifer)
**          Change auxiliary data type to depend on compat.h definition
**          of security label, delete GCA_SEC_LABEL definition.
**      29-sep-1989 (jennifer)
**          Change security to use SVC_PARMS not auxiliary data.
**	02-OCT-89 (pasker)
**	    added error message to correspond with E_GC0031_USRPWD_FAIL
**	10-Nov-89 (seiwald)
**	    Added state flag to SVC_PARMS, to assist in multistate GCA
**	    operations (such as GCA_REQUEST).  Changed comments for
**	    time_out and gca_completion SVC_PARMS.  Moved sys_err struct
**	    down to the bottom.
**	10-Nov-89 (seiwald)
**	    Put storage for five SVC_PARMS into GCA_ACB, so that IIGCa_call
**	    doesn't have to allocate them from a "protected queue."
**	22-Dec-89 (seiwald)
**	    GCA now buffers send/receives.  Declare send/recv state machines.
**	30-Dec-89 (seiwald)
**	    Added temporary SVC_PARMS.flags.nsync_indicator, so GCA can
**	    run sync while the CL runs async.
**	31-Dec-89 (seiwald)
**	    Use new error GC_NO_PEER to indicate listen side peer read
**          failure.  This is for bedcheck connections from the name server.
**	31-Jan-90 (seiwald)
**	    Removed now unused pieces of SVC_PARMS.
**	07-Feb-90 (seiwald)
**	    New GCA_ID_BEDCHECK, for name server to probe registered servers.
**	16-Feb-90 (seiwald)
**	    Zombie checking removed from mainline to VMS CL.  Removed
**	    references to ACB flags controlling zombie checking.
**	16-Feb-90 (seiwald)
**	    Renamed nsync_indicator to sync_indicator, and removed old
**	    references to sync_indicator.  The CL must now be async.
**	06-Mar-90 (seiwald)
**	    Sorted out running sync vs. driving completion exits.
**	    The SVC_PARMS flag "run_sync" indicates that the operation
**	    must be driven to completion, and "drive_exit" indicates
**	    that gca_complete should drive the completion routine.
**	04-Apr-90 (ham)
**	    Put GCA_EXP_size back to 64.
**	27-May-90 (ham)
**	    Add GC_CLEAR_NSID to deassign the listen logical
**	    on name server termination.
**	15-Jun-90 (seiwald)
**	    Deconfused comments about reqd_amount and rcv_data_length.
**	18-Jun-90 (seiwald)
**	    Changed buffer lengths from i4s to nats.
**	18-Jun-90 (seiwald)
**	   Added GCA_CL_HDR_SZ.
**	18-Sep-90 (jorge)
**	   Added automatic setup of GCF62 through GCF64 #defines based
**	   on the lowest symbolic value GCFxx that is defined. This
**	   is easier than setting the entire range. GC.H is also modified.
**	16-Jan-91 (seiwald)
**	   Removed bitfields for flags; reordered pieces.
**	7-Jan-91 (seiwald)
**	   Got GCA_CL_MAX_SAVE_SIZE from <gcaint.h>.
**	   Removed GC_IPC_PROTOCOL, GCA_EXP_SIZE, GCA_CL_HDR_SZ into <gcaint.h>.
**	27-Mar-91 (seiwald)
**	    Prototyped function pointer in SVC_PARMS.
**	26-Dec-91 (seiwald)
**	   Support for installation passwords: added flags.trusted to 
**	   SVC_PARMS, so GClisten can indicate that the peer process is
**	   trustable.
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedef to quiesce the ANSI C 
**	    compiler warnings.
**	5-apr-1993 (robf)
**	    Secure 2.0. Added handling for security labels. 
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**	10-aug-93 (robf)(
**	    Reworked security label handling to be an extra element passed
**	    along with the request. This allows older non-secure frontends
**	    to connect to secure DBMS.
**	08-sep-93 (johnst)
**	    changed struct _SVC_PARMS element async_id type from i4 to PTR
**	    in sync with type change to IIGCa_call() async_id argument.
**	22-nov-93 (seiwald)
**	    Remerged with UNIX gccl.h into glhdr gc.h - gc.h is system 
**	    independent, and needs no system dependent componentry.
**	10-jan-94 (edg)
**	   Fixed previous submission's bungle -- the merge completely
**	   obliterated the GC function prototypes.  Merged to protos
**	   back in.
**	14-Mar-94 (seiwald)
**	    Support in SVC_PARMS for changes to GCA: more variable to hold
**	    more state for the new gca_service() async program interpreter.
**	4-Apr-94 (seiwald)
**	    Added DOC structure pointer to GCA_SR_SM, to keep track of message
**	    encoding.
**	7-apr-94 (seiwald)
**	   New parameters usrbuf, usrlen in GCA_SR_SM to keep track
**	   of callers buffer, so that it can be filled by GCA_RECEIVE.
**	18-Apr-94 (seiwald)
**	   New tuple descriptor support in the GCA_ACB.
**	20-Nov-95 (gordy)
**	    Added remote for embedded Comm Server configuration.
**	19-Aug-96 (gordy)
**	    Added usrptr in GCA_SR_SM for message segment concatenation.
**	 3-Sep-96 (gordy)
**	    Added gca_cb to SVC_PARMS for GCA Control Block.
**	14-Nov-96 (gordy)
**	    Added timeout argument to GCsync().
**	18-Feb-97 (gordy)
**	    Added ACB concatenate flag to patch Name Server protocol.
**	    Moved other ACB booleans into flags.
**	 9-Apr-97 (gordy)
**	    GCsendpeer() and GCrecvpeer() are no more, RIP.  GCA and CL
**	    peer info is now separate.  GC_OLD_PEER added for CLs which
**	    require detection of original peer info for backward compat.
**	10-Apr-97 (gordy)
**	    Finally removed the GCA info from the CL.  The primay impact
**	    on GC is that the GC control block formally obtained through
**	    the acb (svc_parms->acb->lcb) is now contained in the service
**	    parameters (svc_parms->gc_cb).  Also, the service parameters
**	    are no longer passed to the GC completion routine.  The value
**	    to be passed to the completion routine is now included in the
**	    service parameters (svc_parms->closure).
**	17-jun-1997 (canor01)
**	    Remove reference to "username" to avoid conflicts with
**	    defines in Jasmine.
**	24-Sep-97 (rajus01)
**	    Added GC_RMTACCESS_FAIL status code.
**	18-Dec-97 (gordy)
**	    Added counter for synchronous requests (moved from global)
**	    and flag for ACB deletion for multi-threaded sync requests.
**	15-Jan-98 (gordy)
**	    Added partner_host to service parms to support direct GCA 
**	    network connections.
**	17-Mar-98 (gordy)
**	    Added GCusername().
**	26-oct-1999 (thaju02)
**	    Added GC_NOTIMEOUT_DLOCK. (b76625)
**	19-nov-1999 (thaju02)
**	    Changed definition of GC_NOTIMEOUT_DLOCK from 0x0057 to 0x0058.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	16-Jan-04 (gordy)
**	    Removed GC_NO_PEER.  This error status has specific meaning
**	    in GCA and is unrelated to any/all CL activity.
**      03-Aug-2007 (Ralph Loen) SIR 118898
**          Add GCdns_hostname().
**	27-Aug-09 (gordy)
**	    Include gccl.h to get platform dependent definitions for
**	    GC_HOSTNAME_MAX and GC_USERNAME_MAX.
**      02-Jun-2010 (horda03) b123840
**          On VMS the svc_parms->state variable in gcaasync need to be
**          updated atomically. So define GCA_ATOMIC_INCREMENT_STATE
**      04-Jun-2010 (horda03) b123840
**          Define GCA_INCREMENT_STATE according to GCA_ATOMIC_INCREMENT_STATE
**          in this header file, rather than have the #ifdef in gcaasync.
**      14-Jun-2010 (horda03) B123919
**          Update svc_parms->state atomically on linux too.
**/

/*
** GC status codes.
*/

# define    GC_ASSOC_FAIL	    (STATUS) (E_CL_MASK + E_GC_MASK + 0x0001)
# define    GC_INT_PROT_LVL	    (STATUS) (E_CL_MASK + E_GC_MASK + 0x000A)
# define    GC_LOGIN_FAIL	    (STATUS) (E_CL_MASK + E_GC_MASK + 0x000B)
# define    GC_LSN_FAIL		    (STATUS) (E_CL_MASK + E_GC_MASK + 0x0012)
# define    GC_SAVE_FAIL	    (STATUS) (E_CL_MASK + E_GC_MASK + 0x0014)
# define    GC_BAD_SAVE_NAME	    (STATUS) (E_CL_MASK + E_GC_MASK + 0x0015)
# define    GC_RESTORE_FAIL	    (STATUS) (E_CL_MASK + E_GC_MASK + 0x0016)
# define    GC_TIME_OUT		    (STATUS) (E_CL_MASK + E_GC_MASK + 0x0020)
# define    GC_NO_PARTNER	    (STATUS) (E_CL_MASK + E_GC_MASK + 0x0021)
# define    GC_ASSN_RLSED	    (STATUS) (E_CL_MASK + E_GC_MASK + 0x0023)
# define    GC_NM_SRVR_ID_ERR	    (STATUS) (E_CL_MASK + E_GC_MASK + 0x0025)
# define    GC_RQ_PURGED	    (STATUS) (E_CL_MASK + E_GC_MASK + 0x0027)
# define    GC_LSN_RESOURCE	    (STATUS) (E_CL_MASK + E_GC_MASK + 0x0028)
# define    GC_RQ_FAIL		    (STATUS) (E_CL_MASK + E_GC_MASK + 0x0029)
# define    GC_RQ_RESOURCE	    (STATUS) (E_CL_MASK + E_GC_MASK + 0x002A)
# define    GC_SND1_FAIL	    (STATUS) (E_CL_MASK + E_GC_MASK + 0x002B)
# define    GC_SND2_FAIL	    (STATUS) (E_CL_MASK + E_GC_MASK + 0x002C)
# define    GC_SND3_FAIL	    (STATUS) (E_CL_MASK + E_GC_MASK + 0x002D)
# define    GC_RCV1_FAIL	    (STATUS) (E_CL_MASK + E_GC_MASK + 0x002E)
# define    GC_RCV2_FAIL	    (STATUS) (E_CL_MASK + E_GC_MASK + 0x002F)
# define    GC_RCV3_FAIL	    (STATUS) (E_CL_MASK + E_GC_MASK + 0x0030)
# define    GC_USRPWD_FAIL	    (STATUS) (E_CL_MASK + E_GC_MASK + 0x0031)
# define    GC_RMTACCESS_FAIL  	    (STATUS) (E_CL_MASK + E_GC_MASK + 0x0057)
# define    GC_NOTIMEOUT_DLOCK	    (STATUS) (E_CL_MASK + E_GC_MASK + 0x0058)
# define    GC_HOST_TRUNCATED       (STATUS) (E_CL_MASK + E_GC_MASK + 0x0070)
# define    GC_NO_HOST              (STATUS) (E_CL_MASK + E_GC_MASK + 0x0071)
# define    GC_INVALID_ARGS         (STATUS) (E_CL_MASK + E_GC_MASK + 0x0072)

/*
** Miscellaneous constants for GCA
*/

/*
** The following buffer space must be available
** preceding the buffer passed to GCsend() and
** GCreceive().
*/
# define	GCA_CL_HDR_SZ		16

/*
** Buffer space to provide when calling GCsave().
*/
# define	GCA_MAX_CL_SAVE_SIZE	1024

/* Define GCA_ATOMIC_INCREMENT_STATE on platforms where the
** svc_parms->state in gca_service() needs to be updated
** atomically.
**
** Currently atomic update of "state" required on VMS and Linux.
** When defining GCA_ATOMIC_INCREMENT_STATE on other platforms
** you need to ensure the AT_ATOMIC_INCREMENT_I4 macro is defined.
*/
#if defined(VMS) || defined(int_lnx) || defined(a64_lnx)

#define GCA_ATOMIC_INCREMENT_STATE

#endif


#ifdef GCA_ATOMIC_INCREMENT_STATE

#define GCA_INCREMENT_STATE(s) AT_ATOMIC_INCREMENT_I4(s)

#else

#define GCA_INCREMENT_STATE(s) (s)++

#endif

/*
** Name Server's Listen ID functions for use with GCnsid()
*/

# define	GC_CLEAR_NSID	3
# define	GC_RESV_NSID    2
# define	GC_SET_NSID	1
# define	GC_FIND_NSID    0


/*
**  Forward and/or External typedef/struct references.
*/

typedef struct _SVC_PARMS SVC_PARMS;
typedef struct _GC_OLD_PEER GC_OLD_PEER;


/*}
** Name: SVC_PARMS - Internal GCA service parameter list
**
** Description:
**      This structure provides an extensible means of passing parameters
**	from gca_call to it's subsidiary routines.
**
** History:
**	27-Jul-89 (seiwald)
**	    Rearranged.  Connect pointer for connection control block added.
**	    Alt_exit flag added to flags word.  gca_completion (as opposed)
**	    to gca_cl_completion) added for future completion event queueing.
**	26-Feb-90 (seiwald)
**	    Consolidated error numbers: svc_parms->sys_err now points to 
**	    parmlist gca_os_status, and svc_parms->statusp points to 
**	    parmlist gca_status.
**	14-Jun-90 (seiwald)
**	    Removed unused parts of SVC_PARMS; reordered it.
**	26-Dec-91 (seiwald)
**	    Added flags.trusted so that GClisten can indicate that
**	    the peer process has the same uid and can therefore be
**	    trusted.
**	14-Jan-93 (edg)
**	    Removed PTR connection control block -- belongs in ACB.
**	11-aug-93 (ed)
**	    changed CL_HAS_PROTOS to CL_PROTOTYPED
**	 3-Sep-96 (gordy)
**	    Added gca_cb for GCA Control Block.
**	15-Jan-98 (gordy)
**	    Added partner_host to support direct GCA network connections.
**	30-Jun-99 (rajus01)
**	    Added remote for Embedded Comm Server support.
*/

struct _SVC_PARMS
{
    /* GCA CL Interface Area - generic parameters */

    PTR		gc_cb;			/* GC control block */
    STATUS	status;			/* return status from CL */
    CL_ERR_DESC	*sys_err;		/* for stuffing CL errors */
    i4	time_out;		/* Sync, async timeout in ms */
    PTR		closure;		/* Completion routine parameter */
    VOID	(*gca_cl_completion)( PTR closure );	/* Callback */

    struct
    {
	bool            new_chop;	/* Got new chop mark on recieve	*/
	bool            flow_indicator; /* flow for send/receive */

# define	GC_NORMAL	    0	/* Normal data flow indicator */
# define	GC_EXPEDITED	    1	/* Expedited data flow indicator */

	bool		trusted;	/* GClisten says peer is same uid */

	bool            run_sync;   	/* Deprecated! */

	bool		remote;		/* Is remote connection */

    } flags;

    /* GCregister parameters */

    i4		num_connected;		/* for GCregister */
    i4		num_active;		/* for GCregister */
    char	*svr_class;		/* for GCregister */
    char	*listen_id;		/* from GCregister */

    /* GClisten parameters */

    char	*user_name;		/* filled in by GClisten */
    char	*account_name;		/* " */
    char	*access_point_identifier;	/* " */
    i4		size_advise;		/* " */
    char	*sec_label;		/* filled in by GClisten, will */
                                        /* be zero in non-B1 systems. */

    /* GCrequest parameters */

    char	*partner_host;		/* to GCrequest, may be NULL */
    char	*partner_name;		/* to GCrequest */ 

    /* GCsend, GCreceive, GCsave, GCrestore parameters */

    PTR		svc_buffer;		/* send/receive buffer */
    i4		svc_send_length;	/* send buffer size */
    i4		reqd_amount;		/* receive buffer size */
    i4		rcv_data_length;	/* actual data received */

};


/*
** Name: GC_OLD_PEER
**
** Description:
**	This structure represents the info which was originally
**	exchanged between peers by GCA.  The GCA CL could piggy-
**	back its own info through the functions GCsendpeer() and
**	GCrecvpeer().  After the demise of these functions, the
**	CL and GCA peer info was separated forever.  This structure
**	remains for backward compatibility with applications using
**	the original functions.  The CL may require the size of
**	this structure to detect the initial data transmission
**	from an old-style program (the contents are of no interest
**	to the CL).
**
** History:
**      14-May-87 (berrow)
**          Initial structure implementation
**      01-Jul-88 (jbowers)
**          Added GCA_AUX_DATA structure to GCA_PEER_INFO.
**      01-Nov-88 (jbowers)
**	    Turned 2 "futures" into internal protocol id and status.
**	1-Aug-89 (seiwald)
**	    Exchanged gca_partner_protocol and gca_version.
**	    Gca_version was being used as if it was partner_protocol.
**	11-Aug-89 (jorge)
**	    Corrected gca_partner_protocol to be a i4, retired gca_version
**	    as gca_past1.
**	 8-Apr-97 (gordy)
**	    GCA and GC peer info separated. Removed GC{send,recv}peer(), RIP.
*/

struct _GC_OLD_PEER
{
    i4	old1[5];
    i4		old2;
    char	old3[96];
    u_i4	old4;
    i4	old5;
    char	old6[304];
};


/*
** Function definitions.
*/

FUNC_EXTERN STATUS	GCinitiate( PTR (*alloc)(), VOID (*free)() );
FUNC_EXTERN VOID	GCterminate( SVC_PARMS *svc_parms );
FUNC_EXTERN VOID	GCregister( SVC_PARMS *svc_parms );
FUNC_EXTERN VOID	GClisten( SVC_PARMS *svc_parms );
FUNC_EXTERN VOID	GCrequest( SVC_PARMS *svc_parms );
FUNC_EXTERN VOID	GCdisconn( SVC_PARMS *svc_parms );
FUNC_EXTERN VOID	GCrelease( SVC_PARMS *svc_parms );
FUNC_EXTERN VOID	GCreceive( SVC_PARMS *svc_parms );
FUNC_EXTERN VOID	GCsend( SVC_PARMS *svc_parms );
FUNC_EXTERN VOID	GCsave( SVC_PARMS *svc_parms );
FUNC_EXTERN VOID	GCrestore( SVC_PARMS *svc_parms );
FUNC_EXTERN VOID	GCpurge( SVC_PARMS *svc_parms );
FUNC_EXTERN VOID	GCsync( i4 timeout );
FUNC_EXTERN STATUS	GCnsid( i4  flag, char *gca_listen_id,
				i4 length, CL_ERR_DESC *sys_err ); 
FUNC_EXTERN VOID	GChostname( char *, i4  ); 
FUNC_EXTERN VOID	GCusername( char *, i4  );
FUNC_EXTERN STATUS	GCusrpwd( char *, char *, CL_ERR_DESC * );
FUNC_EXTERN VOID	GCexec( VOID );
FUNC_EXTERN VOID	GCshut( VOID );
FUNC_EXTERN STATUS      GCdns_hostname( char *, i4 );

# endif /* GC_HDR_INCLUDED */
