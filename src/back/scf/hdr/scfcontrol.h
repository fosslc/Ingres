/*
** Copyright (c) 1987, 2004 Ingres Corporation
**
*/

/**
** Name: SCFCONTROL.H - This file contains the major data struct def'n
**
** Description:
**      This file contains the definitions of the major controlling
**      data structure for SCF, the SCF_CONTROL_CB type.
[@comment_line@]...
**
** History:
**      14-mar-86 (fred)
**          Created on Jupiter.
**	15-jul-88 (rogerk)
**	    Added sc_fastcommit field into scf control block for fast commit.
**	15-sep-88 (rogerk)
**	    Added sc_writebehind field into scf control block for write behind.
**	26-sep-1988 (rogerk)
**	    Added thead priority fields.
**	 6-feb-1989 (rogerk)
**	    Added support for shared memory buffer manager.  Added sc_solecache
**	    field to SC_MAIN_CB.
**	23-mar-1989 (fred)
**	    Added sc_urdy_semaphore to mark whether the server is ready for
**	    user sessions.
**	18-may-1889 (neil)
**	    Added sc_rule_depth for startup rule nesting depth.
**	23-may-1989 (jrb)
**	    Added #def for new tracepoint.
**      14-JUL-89 (JENNIFER)
**          Added indicator that server is running C2.
**	23-sep-89 (ralph)
**	    Added sc_flatten for /NOFLATTEN startup option.
**	23-sep-89 (ralph)
**	    Added sc_opf_mem for /OPF=(MEMORY=n) startup option.
**	17-oct-89 (ralph)
**	    Changed sc_flatten to sc_noflatten
**	25-oct-89 (paul)
**	    Added trace flags for dumping event subsystem data structures.
**	    Modified typedef until include file organization.
**	05-jan-90 (andre)
**	    Added sc_fips for /FIPS startup option
**	08-aug-90 (ralph)
**	    Added sc_show_state to sc_main_cb for dbmsinfo('secstate')
**	    Added sc_initmask and SC_I_AUDITSTATE for audit initialization.
**          Add sc_gclisten_fail and SC_GCA_LISTEN_MAX to SC_MAIN_CB (bug 9849)
**	    Rename scf_opf_mem to scf_reserved (scf_opf_mem no longer used)
**	09-oct-90 (ralph)
**	    6.3->6.5 merge:
**	    29-mar-90 (alan)
**		Added SC_INGRES_SERVER and SC_RMS_SERVER flags to
**		sc_capabilities for /SERVER_CLASS startup option.
**	    12-sep-90 (sandyh)
**		Added sc_psf_mem to handle PSF MEMORY startup option.
**	06-feb-91 (neil)
**	    Added sc_events for alerter configuration.
**      23-jun-1992 (fpang)
**	    Sybil merge.
**          Start of merged STAR comments:
**             06-jun-91 (fpang)
**                 Added /psf=mem startup support.
**             21-jun-91 (fpang)
**                 Added /opf=(...) startup support.
**          End of STAR comments.
**      21-aug-1992 (stevet)
**          Added sc_tz_semaphore for demand loading of timezone table.
**	25-sep-92 (markg)
**	    Add elements sc_sxbytes and sc_sxvcb to SC_MAIN_CB to support 
**	    new SXF facility.
**	7-oct-92 (daveb)
**	    Added sc_gwbytes and sc_gwvcb to support first-class GWF.
**	7-oct-1992 (bryanp)
**	    Add SC_RECOVERY_SERVER flag to SC_MAIN_CB to record recovery parm.
**	10-dec-1992 (markg)
**	    Change the name of the sc_show_state function pointer to
**	    sc_sxf_cptr.
**	17-jan-1993 (ralph)
**	    Added sc_flatflags and sc_csrflags for FIPS
**	12-apr-1993 (ralph)
**	    DELIM_IDENT
**	    Added sc_dbdb_dbtuple to SC_MAIN_CB
**	26-Jul-1993 (daveb)
**	    Add sc_listen_mask, sc_max_conns, sc_current_conns fields
**	    to the Sc_main_cb, add GLOBALREF of Sc_main_cb.
**	25-oct-93 (robf)
**	    Add sc_min_usr_priority, sc_max_usr_priority fields
**	6-jan-94 (robf)
**          Add sc_session_chk_interval
**	21-jan-94 (iyer)
**	    Added SC_MAIN_CB.sc_cname in support of dbmsinfo('db_tran_id').
**	14-feb-94 (rblumer)
**	    in SC_MAIN_CB, removed SC_FLATOPT flag (B59120);
**	    removed sc_noflatten field; it is redundant with SC_FLATNONE flag.
**	10-Mar-1994 (daveb)
**	    Add sc_highwater_conns.    
**	23-may-1995 (reijo01)
**	    Added  sc_startup_time.    
**	24-Apr-1995 (cohmi01)
**	    Add SC_IOMASTER_SERVER to indicate disk I/O master process, which
**	    handles write-along and read-ahead threads.
**      06-mar-1996 (nanpr01)
**          Added field sc_maxtup in SC_MAIN_CB which hold maximum tuple size 
**	    available in the installation. This change is required for 
**	    increasing the tuple size. This field is used in scdmain for 
**	    allocation of control blocks rather than using DB_MAXTUP.
**	    Also setup the maximum page size, available page sizes and their
**	    corresponding tuple sizes.
**	04-apr-1996 (loera01)
** 	   Added SC_VCH_COMPRESS capabilities mask to support variable
** 	   datatype compression. 
**	22-may-96 (stephenb)
**	    Add sc_rep_qman field to SC_MAIN_CB
**	27-Jun-1997 (jenjo02)
**	    Removed sc_urdy_semaphore, which served no useful purpose.
**	23-sep-1997 (canor01)
**	    Define SC_FRONTIER_SERVER.
**	03-dec-1997 (canor01)
**	    Define SC_ICE_SERVER.
**	26-Mar-1998 (jenjo02)
**	    Added sc_session_count array to SC_MAIN_CB to keep track of
**	    number of sessions by session type.
**	08-Apr-1998 (jenjo02)
**	    Added SC_IS_MT flag to sc_capabilites, set if OS threads in use,
**	    which all of SCF can check instead of constantly calling 
**	    CS_is_mt();
**	14-May-1998 (jenjo02)
**	    Removed sc_writebehind. WriteBehind threads are now started
**	    in each allocated cache.
**      08-sep-1999 (devjo01)
**          Added SC_LIC_VIOL_REJECT to indicate that the server should
**          reject new connections because of a licensing failure.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	06-oct-2000 (somsa01)
**	    In SC_MAIN_CB, the size of sc_name has been increased to 18.
**      01-Dec-2000 (hanal04) Bug 100680 INGSRV 1123
**          Updating rules fired by updates may loop indeterinately if
**          the update cause the row to move. The solution is to prefetch
**          the rows. The prefetch introduces an overhead and may not be
**          required. The config parameter rule_upd_prefetch can be used
**          to disable the prefetch solution. It is then the user's
**          responsibility to ensure consistent behaviour.
**      28-Dec-2000 (horda03) Bug 103596  INGSRV 1349
**          Added sc_event_priority to store the priority to be
**          assigned to the Event thread.
**      06-apr-2001 (stial01)
**          Added sc_pagetype to SC_MAIN_CB
**	10-may-2001 (devjo01)
**	    Added SC_CSP_SERVER. s103715.
**      10-jun-2003 (stial01)
**          Added sc_trace_errno
**	15-jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	20-sep-2004 (devjo01)
**	    Added sc_server_class, sc_class_node_affinity to SC_MAIN_CB,
**	    bumped useless version to 3.0.
**      07-oct-2004 (thaju02)
**          Define sc_psf_mem as SIZE_TYPE.
**      30-aug-06 (thaju02)
**          Add sc_rule_del_prefetch_off to SC_MAIN_CB. If true, 
**	    designates that rule_del_prefetch is set to off in 
**	    config.dat. (B116355)
**	02-oct-2006 (gupsh01)
**	    Added sc_date_type_alias.
**	10-Mar-2009 (kiria01) SIR 121665
**	    Update GCA API to LEVEL 5
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/

/*
**  Defines of other constants.
*/

/*
**      These constants define the number of server wide trace bits
*/
#define                 SCF_NBITS       128
#define                 SCF_VPAIRS      1
#define                 SCF_VONCE       1
/*
**	These are the trace flags themselves
*/
#define                 SCT_EXDECL      0x0
		/* set ex handler on scfcall() */ 
#define                 SCT_MEMCHECK    0x1
		/* check scf memory for corruption */
#define			SCT_LOG_ERROR	0x2
		/* write copy of user error and trace output to local log */
#define			SCT_SCRLOG	0x3
#define			SCT_MEMLOG	0x4
#define                 SCT_INVOKE_DEBUG 0x5
#define			SCT_SESSION_LOG	0x6
#define			SCT_GUARD_PAGES 0x7
#define			SCT_LOG_TRACE   0xa
		/* disable trace output to user but send it to local log */
#define			SCT_EVENT	0xb
		/* Trace SCF alert operations for this session */
#define			SCT_NOBYREF 0xc /* temporary trace point
					** to disable byrefs */

/*}
** Name: SCD_AFCN - Description of a facility's AIC or PAINE
**
** Description:
**      This structure describes to SCF an AIC or PAINE for a facility.
**      These are functions which are delivered asynchronously to facilities
**      to inform them that some event has taken place.  In the INGRES DBMS,
**      the most common such event is a user generated interrupt.
**
**      Each facility's AIC has a mode associated with it.  This mode specifies
**      (at the moment) whether the facility wishes to have the AIC delivered
**      to it independent of whether the facility is active.  At the moment, the
**      only user of this mode is DMF, which wishes to be informed of interrupts
**      but is rarely directly activated by the sequencer.
**
**	AIC's are expected to be functions of the form
**
**	    STATUS
**	    ruf_aic(cb, reason)	    random user fac's aic
**	    RUF_CB  *cb;	    session control block
**	    i4 reason;	    why am i getting this
**
** History:
**     17-Jan-86 (fred)
**          Created on Jupiter.
**	20-Apr-2004 (jenjo02)
**	    Changed to return STATUS rather than VOID.
[@history_line@]...
*/
typedef struct _SCD_AFCN
{
    i4		    afcn_mode;		/* taken from call parms in scf.h */
    STATUS          (*afcn_addr)();     /* The address of the procedure */
}   SCD_AFCN;

/*}
** Name: SCD_ALIST - The list of declared async fcns for a session
**
** Description:
**      This structure contains the list of async functions which a
**      session has declared.  Each function address is associated with a
**      particular facility, and is expected to be a VOID function
**	called as function(session_id, reason_id);
**
** History:
**     16-Jan-86 (fred)
**          Created on jupiter.
**
*/
typedef struct _SCD_ALIST
{
    SCD_AFCN        sc_afcn[DB_MAX_FACILITY + 1];
			       /* 'cause C arrays are 0-based */ 
}   SCD_ALIST;

/*}
** Name: SCF_MAIN_CB - Main controlling structure within SCF
**
** Description:
**      This structure is the central controlling data element within
**      SCF.  All queues and lists are rooted here.  Furthermore, many
**      server level statistics are kept here.
**
** History:
**     14-mar-1986 (fred)
**          Initial definition on Jupiter
**	19-Jun-1986 (fred)
**	    Added definition for connection request block que (sc_crb_q).
**	15-jul-88 (rogerk)
**	    Added sc_fastcommit field into scf control block for fast commit.
**	15-sep-88 (rogerk)
**	    Added sc_writebehind field into scf control block for write behind.
**	26-sep-1988 (rogerk)
**	    Added thead priority fields.
**	 6-feb-1989 (rogerk)
**	    Added support for shared memory buffer manager.  Added sc_solecache
**	    field to specify that databases served by this server should not
**	    be shared with another buffer manager.
**	18-may-1989 (neil)
**	    Added sc_rule_depth for startup rule nesting depth.
**      14-JUL-89 (JENNIFER)
**          Added indicator that server is running C2.
**	22-jun-89 (jrb)
**	    Added SC_RESRC_CNTRL #define for Terminator phase I.  This bit will
**	    indicate whether the site is licensed for DBprivs/Grants and Rules.
**	23-sep-89 (ralph)
**	    Added sc_flatten for /NOFLATTEN startup option.
**	23-sep-89 (ralph)
**	    Added sc_opf_mem for /OPF=(MEMORY=n) startup option.
**	17-oct-89 (ralph)
**	    Changed sc_flatten to sc_noflatten
**	05-jan-90 (andre)
**	    Added sc_fips for /FIPS startup option
**	08-aug-90 (ralph)
**	    Added sc_show_state to sc_main_cb for dbmsinfo('secstate')
**	    Added sc_initmask and SC_I_AUDITSTATE for audit initialization.
**          Add sc_gclisten_fail and SC_GCA_LISTEN_MAX to SC_MAIN_CB (bug 9849)
**	    Rename scf_opf_mem to scf_reserved (scf_opf_mem no longer used)
**	09-oct-90 (ralph)
**	    6.3->6.5 merge:
**	    29-mar-90 (alan)
**		Added SC_INGRES_SERVER and SC_RMS_SERVER flags to
**		sc_capabilities for /SERVER_CLASS startup option.
**	    12-sep-90 (sandyh)
**		Added sc_psf_mem to handle PSF MEMORY startup option.
**	06-feb-91 (neil)
**	    Added sc_events for alerter configuration.
**      23-jun-1992 (fpang)
**	    Sybill merge.
**          Start of merged STAR comments:
**              24-Aug-1988 (alexh)
**                  Added SC_CAP_STAR and SC_CAP_BACKEND to distiguish STAR 
**		    server
**              26-Oct-1988 (alexh)
**                  Added sc_ii_vnode for installation node identification.
**              28-nov-1989 (georgeg)
**                  Added SC_2PC_RECOVER SC_INBUSINESS in sc_state, when this 
**		    flag is set the STAR server is recovering orphan DX's, no 
**		    FE is allowed to connect.
**              12-jul-1990 (georgeg)
**                  Added sc_main_cb.sc_clustered_nodes, keep a list of names of
**                  the nodes in the cluster.
**              02-nov-1990 (georgeg)
**                  Added sc_main_cb.sc_nostar_recovr flag to starr 
**		    distributed transaction tread, sc_main_cb.sc_no_star_cluster
**		    do not alias nodes if on clusters.
**          End of STAR comments.
**	25-sep-92 (markg)
**	    Added elements sc_sxbytes and sc_sxvcb to support new SXF facility.
**	7-oct-92 (daveb)
**	    Added sc_gwbytes and sc_gwvcb to support first-class GWF.
**	7-oct-1992 (bryanp)
**	    Add SC_RECOVERY_SERVER flag to SC_MAIN_CB to record recovery parm.
**	10-dec-1992 (markg)
**	    Change the name of the sc_show_state function pointer to
**	    sc_sxf_cptr.
**	17-jan-1993 (ralph)
**	    Added sc_flatflags and sc_csrflags for FIPS
**	12-apr-1993 (ralph)
**	    DELIM_IDENT
**	    Added sc_dbdb_dbtuple to SC_MAIN_CB
**	1-jun-1993 (robf)
**	    Added sc_secflags to hold security config flags.
**	26-Jul-1993 (daveb)
**	    Add sc_listen_mask, sc_max_conns, sc_current_conns fields
**	    to the Sc_main_cb, add GLOBALREF of Sc_main_cb.
**	17-dec-93 (rblumer)
**	    removed sc_fips field.
**	    "FIPS mode" no longer exists.  It was replaced some time ago by
**	    several feature-specific flags (e.g. flatten_nosingleton and
**	    direct_cursor_mode).
**	21-Jan-94 (iyer)
**	    Added SC_MAIN_CB.sc_cname in support of dbmsinfo('db_tran_id').
**	14-feb-94 (rblumer)
**	    removed sc_noflatten field; it is redundant with SC_FLATNONE flag.
**	    removed SC_FLATOPT flag (B59120).
**	10-Mar-1994 (daveb)
**	    Add sc_highwater_conns.    
**	23-may-1995 (reijo01)
**	    Added  sc_startup_time.    
**	06-Apr-1995 (cohmi01)
**	    Add SC_IOMASTER_SERVER to indicate disk I/O master process, which
**	    handles write-along and read-ahead threads.
**	13-jun-1995 (canor01)
**	    remove sc_loi_semaphore for a more generic approach (MCT).
**	04-jul-1995 (emmag)
**	    sc_loi_semaphore should only be removed where MCT is defined.
**      24-jul-1995 (lawst01)
**          Added define for SC_GCA_SINGLE_MODE.
**      06-mar-1996 (nanpr01)
**          Added field sc_maxtup in SC_MAIN_CB which hold maximum tuple size 
**	    available in the installation. This change is required for 
**	    increasing the tuple size. This field is used in scdmain for 
**	    allocation of control blocks rather than using DB_MAXTUP.
**	    Also setup the maximum page size, available page sizes and their
**	    corresponding tuple sizes.
**	22-may-96 (stephenb)
**	    Add sc_rep_qman field
**	03-jun-1996 (canor01)
**	    OS-threads version of LOingpath no longer needs protection by
**	    sc_loi_semaphore.
**      01-aug-1996 (nanpr01 for ICL phil.p)
**          Added sc_dmcm field into scf control block for the Distributed
**          Multi-Cache Management (DMCM) protocol.
**	27-Jun-1997 (jenjo02)
**	    Removed sc_urdy_semaphore, which served no useful purpose.
**	3-feb-98 (inkdo01)
**	    Added sc_sortthreads for parallel sort project.
**	26-Mar-1998 (jenjo02)
**	    Added sc_session_count array to SC_MAIN_CB to keep track of
**	    number of sessions by session type.
**	    SCD_MCB renamed to SCS_MCB.
**	08-Apr-1998 (jenjo02)
**	    Added SC_IS_MT flag to sc_capabilites, set if OS threads in use,
**	    which all of SCF can check instead of constantly calling 
**	    CS_is_mt();
**	13-apr-98 (inkdo01)
**	    Dropped sc_sortthreads to use factotum threads.
**	14-May-1998 (jenjo02)
**	    Removed sc_writebehind. WriteBehind threads are now started
**	    in each allocated cache.
**      08-sep-1999 (devjo01)
**          Added SC_LIC_VIOL_REJECT to indicate that the server should
**          reject new connections because of a licensing failure.
**      10-Mar-2000 (wanfr01)
**          Bug 100847, INGSRV 1127:
**          Added sc_assoc_request field
**	06-oct-2000 (somsa01)
**	    The size of sc_name has been increased to 18.
**      01-Dec-2000 (hanal04) Bug 100680 INGSRV 1123
**          Added sc_rule_upd_prefetch_off. Set to TRUE is the config
**          parameter rule_upd_prefetch is set to OFF.
**      28-Dec-2000 (horda03) Bug 103596  INGSRV 1349
**          Added sc_event_priority to store the priority to be
**          assigned to the Event thread.
**	16-jul-03 (hayke02)
**	    Added sc_psf_vch_prec for config parameter psf_vch_prec. This
**	    change fixes bug 109134.
**      31-aug-2004 (sheco02)
**          X-integrate change 466442 to main.
**	20-sep-2004 (devjo01)
**	    Added sc_server_class, sc_class_node_affinity to SC_MAIN_CB,
**	    bumped useless version to 3.0.
**	18-Oct-2004 (shaha03)
**	    SIR 112918, Added configurable default cursor open mode support.
**	    added "sc_defcsrflag" to the structure.
**	30-aug-06 (thaju02)
**	    Add sc_rule_del_prefetch_off. If true, designates that
**	    rule_del_prefetch is set to off in config.dat. (B116355)
**	02-oct-2006 (gupsh01)
**	    Added sc_date_type_alias.
**	11-jul-2007 (kibro01) b118702
**	    Added value for explicit NONE in date_type_alias
**	7-march-2008 (dougi) SIR119414
**	    Added SC_[NO_]CACHEDYN flags.
**	01-may-2008 (smeke01) b118780
**	    Corrected name of sc_avgrows to sc_totrows.
**      23-Sep-2009 (hanal04) Bug 115316
**          Added SC_CAPABILITIES_FLAGS for iimonitor output of new
**          "SHOW SERVER CAPABILITIES".
**	26-Oct-2009 (kiria01) SIR 121883
**	    Scalar sub-query support: Added SC_FLATNCARD for
**	    CARDINALITY_CHECK
**      10-feb-2010 (maspa05) SIR 123293
**          Changed sc_server_class to use SVR_CLASS_MAXNAME instead of 
**          hard-coded 24
*/
typedef struct _SC_MAIN_CB
{
    i4         sc_state;		/* overall state of the server */
#define		SC_UNINIT	0x0L
#define		SC_INIT		0x1L
#define		SC_SHUTDOWN	0x2L
#define		SC_OPERATIONAL	0x3L
#define		SC_2PC_RECOVER	0X4L	/* Star is recovering DX orphans */
    PID		    sc_pid;		/* server's process id */
    i4	    sc_scf_version;	/* version of SCF */
#define		SC_SCF_VERSION	CV_C_CONST_MACRO('v', '3', '.', '0')
    i4	    sc_facilities;	/* which facilities are known */
    i4	    sc_capabilities;	/* capabilities which this server has */
#define		SC_RESRC_CNTRL		0x0001
#define         SC_C_C2SECURE           0x0002	/* Server is running C2. */
#define         SC_INGRES_SERVER	0x0004	/* INGRES DMBS server */
#define         SC_RMS_SERVER		0x0008	/* RMS Gateway server */
#define		SC_STAR_SERVER		0x0010	/* Star, distributed server */
#define		SC_RECOVERY_SERVER	0x0020  /* Recovery Server */
#define		SC_BATCH_SERVER 	0x0080  /* Server optimized for batch */
#define		SC_IOMASTER_SERVER	0x0100  /* Master I/O driver process */
#define         SC_GCA_SINGLE_MODE      0x0200  /* gca operates in batch mode */
#define         SC_VCH_COMPRESS         0x0400  /* Supports vch compression */
#define         SC_ICE_SERVER           0x1000  /* ICE server */
#define		SC_CSP_SERVER		0x2000	/* CSP enabled RCP */
#define         SC_LIC_VIOL_REJECT      0x4000  /* Reject new connections */
#define         SC_IS_MT           	0x8000  /* OS threads in use */
#define SC_CAPABILITIES_FLAGS "\
RESRC_CNTRL,C2SECURE,INGRES_SERVER,RMS_SERVER,STAR_SERVER,RECOVERY_SERVER,,\
BATCH_SERVER,IOMASTER_SERVER,GCA_SINGLE_MODE,VCH_COMPRESS,,ICE_SERVER,\
CSP_SERVER,LIC_VIOL_REJECT,IS_MT"
    u_i4	    sc_initmask;	/* Server initialization masks */
#define		SC_I_AUDITSTATE		0x0001	/* Audit state initialized */
    i4		    sc_irowcount;	/* number of row's to expect the first
					** time throught a select/retrieve */
#define                 SC_ROWCOUNT_INITIAL 20
    u_i4	    sc_totrows;		/* total number of rows from select */
    u_i4	    sc_selcnt;		/* Number of selects */
    u_i4	    sc_flatflags;	/* QUERY_FLATTEN flags */
#define		SC_FLATNAGG		0x01L	/* NOAGGREGATE */
#define		SC_FLATNSING		0x02L	/* NOSINGLETON */
#define		SC_FLATNONE		0x04L	/* SET =>Don't flatten queries*/
#define		SC_FLATNCARD		0x08L	/* SET =>Don't card chk on
						** flattened where clause
						** singleton sub-selects */
    u_i4	    sc_csrflags;	/* CURSOR_MODE flags */
#define		SC_CSRDIRECT		0x01L   /* UPDATE_MODE = DIRECT */
#define		SC_CSRDEFER		0x02L   /* UPDATE_MODE = DEFERRED */
#define		SC_CACHEDYN		0x04L	/* cache dynamic cursor plans */
#define		SC_NO_CACHEDYN		0x08L	/* don't cache dynamic plans */
    u_i4	    sc_defcsrflag;	/* CURSOR_MODE default values */
#define		SC_CSRRDONLY	0x01L   /* OPEN_MODE = READONLY */
#define     SC_CSRUPD		0x02L   /* OPEN_MODE = UPDATE */
    u_i4	    sc_secflags;	/* Security flags, typically set at
					** startup reflecting the overall 
					** security policy
					*/
#define		SC_SEC_ROW_AUDIT	0x02L	/* Row security auditing */
#define		SC_SEC_ROW_KEY		0x04L	/* Row security key  */
#define		SC_SEC_PASSWORD_NONE	0x010L	/* No user passwords */
#define		SC_SEC_ROLE_NONE	0x020L	/* No roles */
#define		SC_SEC_ROLE_NEED_PW	0x040L	/* Roles need password */
#define		SC_SEC_OME_DISABLED	0x080L	/* OME disabled */
    SIZE_TYPE       sc_psf_mem;         /* PSF memory pool size */
    i4		    sc_acc;		/* number of active cursors/session */
    i4		    sc_rule_depth;	/* Rule nesting depth */
    i2		    sc_rule_del_prefetch_off; /* disable prefetch on rules
					      ** triggered by a delete if
					      ** true.
					      */
    i2              sc_rule_upd_prefetch_off; /* disable prefetch on rules
                                              ** triggered by an update
                                              ** if true.
                                              */
    i4		    sc_session_chk_interval; /* Session check interval */
#define                 SC_DEF_SESS_CHK_INTERVAL 30
    i4		    sc_soleserver;	/* are we the only server in the inst */
    i4		    sc_solecache;	/* DB's not served by another cache */
    i4		    sc_fastcommit;	/* are we using fast commit protocol */
    i4		    sc_names_reg;	/*Do we register with the name server */
    i4		    sc_writealong; 	/* number of write along threads */
    i4		    sc_readahead;   	/* number of read ahead threads */
    i4		    sc_rep_qman;	/* no of replicator que man threads */
    i4	sc_maxtup;			/* max tup size of the installation */
    i4	sc_maxpage;			/* max page size of the inst.       */
    i4	sc_pagesize[DB_MAX_CACHE];	/* available page sizes of the inst.*/	
    i4	sc_tupsize[DB_MAX_CACHE];	/* available tuple size of the inst.*/ 	
    i4	sc_pagetype[DB_MAX_PGTYPE +1];	/* available page types of the inst. */
    i4		    sc_listen_mask;	/* GCA listen state */
#define		SC_LSN_REGULAR_OK	0x01 /* accept regular users */
#define		SC_LSN_SPECIAL_OK	0x02 /* accept special users */
#define		SC_LSN_TERM_IDLE	0x04 /* kill server when idle */
    i4		    sc_max_conns;	/* max number of user connections */
    i4		    sc_rsrvd_conns;	/* reserved conns over max_conns
					   available for special users */
    i4		    sc_current_conns;	/* current connection count.
					   Server is idle when count == 0 */
    i4		    sc_nousers;		/* number of total threads,
					   including DBMS internal threads */
    i4		    sc_highwater_conns;	/* max connections ever current */
    i4		    sc_min_priority;	/* minimum thread priority */
    i4		    sc_max_priority;	/* maximum thread priority */
    i4		    sc_norm_priority;	/* default priority for user sessions */
    i4		    sc_max_usr_priority;/* maximum user thread priority */
    i4		    sc_min_usr_priority;/* minimum user thread priority */
    i4              sc_gclisten_fail;   /* # of consecutive GCA_LISTEN fails;
                                        ** if > SC_GCA_LISTEN_MAX, CS_CLOSE.  */
#define         SC_GCA_LISTEN_MAX   20  /* Max consecutive GCA_LISTEN failures*/
    i2		    sc_date_type_alias;	/* Type of data type to which date is 
					** alias to i.e ingresdate or ansidate*/
#define         SC_DATE_NOTSET      0x00  /* Date is not set */
#define         SC_DATE_INGRESDATE  0x01  /* Date is set to ingresdate */
#define         SC_DATE_ANSIDATE    0x02  /* Date is set to ansidate */
#define         SC_DATE_NONE        0x04  /* Date is set to disallow "date" */
    i4		    sc_errors[DB_MAX_FACILITY+1];
    DB_STATUS	    (*sc_sxf_cptr)();	/* Pointer to the sxf_call() routine */
    char	    sc_cname[DB_NODE_MAXNAME]; /* node name if cluster */
    char	    sc_sname[18];	/* name of the server */
    SCS_MCB	    *sc_mcb_list;	/* list of all known mcb's in system */
    CS_SEMAPHORE    sc_mcb_semaphore;	/* semaphore for updating this list */
    struct
    {
	SCV_DBCB	*kdb_next;
	SCV_DBCB	*kdb_prev;
	CS_SEMAPHORE	kdb_semaphore;
    }		    sc_kdbl;		/* list of databases known to server */
    SC0M_CB	    sc_sc0m;
    TIMER	    sc_timer;
    SCD_ALIST	    sc_alist;		/* aic's for the server */ 
    SCD_ALIST	    sc_plist;		/* paines' for the server */ 
    i4	    sc_adbytes;		 /* size of adf's scb */
    PTR		    sc_advcb;
    i4	    sc_dmbytes;
    PTR		    sc_dmvcb;
    i4	    sc_opbytes;		/* ditto */
    PTR		    sc_opvcb;
    i4	    sc_psbytes;
    PTR		    sc_psvcb;
    i4	    sc_qebytes;
    PTR		    sc_qevcb;
    i4	    sc_qsbytes;
    PTR		    sc_qsvcb;
    i4	    sc_rdbytes;
    PTR		    sc_rdvcb;
    i4	    sc_sxbytes;
    PTR		    sc_sxvcb;
    i4	    sc_gwbytes;
    PTR		    sc_gwvcb;
    PTR		    sc_dbdb_loc;
    char	    sc_iversion[DB_TYPE_MAXLEN];
    CS_SEMAPHORE    sc_misc_semaphore;
    PTR		    sc_gca_cb;		    /* GCA_API_LEVEL_5 cb */
    GCA_LS_PARMS    sc_gcls_parms;
    i2		    sc_assoc_timeout;
    CS_SEMAPHORE    sc_gcsem;		    /* Semaphore for use of ... */
    GCA_SD_PARMS    sc_gcdc_sdparms;	    /* for don't care sends */
    SCD_SCB	    *sc_proxy_scb;	    /* for use when there really is */
					    /* no session		    */
    i4	    sc_major_adf;	    /* User major id		    */
    i4	    sc_minor_adf;	    /* user adt minor id	    */
    i4		    sc_risk_inconsistency;  /* User requested inconsistency */
					    /* risk			    */
    ULT_VECTOR_MACRO(SCF_NBITS, SCF_VONCE)
		    sc_trace_vector;
    /*
    ** Root for the server alert registration list.
    ** This list contains all registrations for alerts for session's running
    ** in this server. The entry is a pointer to a structure containing
    ** an array of hash buckets.
    ** Each bucket is a pointer to a list of alerts (SCE_ALERT).
    */
#ifndef SE_HASH_TYPE
#  define	SCE_HASH	i4
#endif
    SCE_HASH	    *sc_alert;		    /* hash array of alerts */
    i4		    sc_events;		    /* Corresponds to user /EVENTS */
    i4              sc_event_priority;      /* Priority of Event Thread */
    /* Star additions */
    DD_NODE_NAME    sc_ii_vnode;	    /* Save install node name */
    i4		    sc_prt_gcaf;	    /* Controls dumping of GCA msgs */
    DD_CLUSTER_INFO sc_clustered_nodes;	    /* Nodes in this cluster */
    i4		    sc_no_star_cluster;	    /* Do cluster optimizations if
					    ** not set */
    i4		    sc_nostar_recovr;	    /* Do 2PC recovery if not set */
    CS_SEMAPHORE    sc_tz_semaphore;	    /* Semaphore to prevent 
					    ** concurrent update to TZ table
					    ** structure.
					    */
    PTR		    sc_dbdb_dbtuple;	    /* Pointer to IIDBDB's iidatabase
					    ** tuple
					    */
    i4         	    sc_startup_time;        /* The time the server started */
    i4              sc_dmcm;                /* DMCM protocol */
    bool	    sc_psf_vch_prec;        /* varchar precedence */
    i4		    sc_trace_errno;

    struct				    /* session/thread counts: */
    {
	i4		current;	/* current session count */
	i4		hwm;		/* highwater mark */
	u_i4	created;	/* number of sessions created */
    }	sc_session_count[SCS_MAXSESSTYPE + 1];
    i4		    sc_class_node_affinity; /* If set server class may only
    					       be run on 1 node of a cluster */
    char	    sc_server_class[SVR_CLASS_MAXNAME]; 
                                            /* ' ' padded svr class name */

}   SC_MAIN_CB;

GLOBALREF SC_MAIN_CB *Sc_main_cb;
