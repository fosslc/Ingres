/*
**Copyright (c) 2010 Ingres Corporation
**
*/

/*
** Name: SCS.H - This file describes the data structures for the sequencer
**
** Description:
**      This file describes the data structures used and manipulated 
**      primarily by the sequencer.  These data structures are used
**      on all machines regardless of whether the machine is running a
**      server based ingres or not.
**
**	WARNING: If you alter length of the SCS_SSCB, or any structure
**		 embedded in the SCS_SSCB (such as the SCS_ICS), you
**		 *must* relink your iidbms.xxx (I learned the hard way!)
**
**		 TO AVOID HAVING ANYONE LEARN THE HARD WAY, USE THE SHARED IMAGE
**		 IDENTIFIERS TO MARK THESE CHANGES (VMS ONLY) -- FRED
**
** History:
**      17-Jan-86 (fred)
**          Created on jupiter.
**      23-Jul-86 (fred)
**          Added cursor information structure (csi)
**      26-Aug-1986 (fred)
**          Added FE/ULC style tuple descriptors to the CSI and CURRENT
**          structures to support heterogenous network data translation.
**      15-Nov-1986 (fred)
**          Added SCS_RDESC structure which encapsulates the information
**          necessary to efficiently and readably perform the network and
**          small data packet transmission.
**      23-Feb-1987 (fred)
**          Moved AIC functions to scfcontrol.h, since AIC's are really
**          declared for the entire server.
**     15-jul-88 (rogerk)
**	    Added Fast Commit thread type.
**     15-sep-88 (rogerk)
**	    Added Write Behind thread type
**     10-Mar-89 (ac)
**	    Added 2PC support.
**	14-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    Added ics_grpid to SCS_ICS for session's group id
**	    Added ics_aplid to SCS_ICS for session's application id
**	    Added ics_apass to SCS_ICS for session's applid password
**	31-mar-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Added fields for database privileges:
**	    ics_fl_dbpriv, ics_qrow_limit, ics_qdio_limit, ics_qcpu_limit,
**	    ics_qpage_limit, ics_qcost_limit.
**	24-apr-1989 (fred)
**	    Added SCS_SERVER_INIT thread type for server initiation thread.
**	25-apr-89 (ralph)
**	    GRANT Enhancements, Phase 2a:
**	    Added ics_estat -- effective user status bits
**	10-may-89 (ralph)
**	    GRANT Enhancements, Phase 2b:
**	    Added ics_ctl_dbpriv, ics_gpass to SCS_ICS
**	02-may-89 (anton)
**	    Local collation changes
**	05-sep-89 (jrb)
**	    Added ics_num_literals field to scs_ics struct to indicate whether
**	    the user had chosen decimal or floating point numeric literals.
**	28-sep-1989 (ralph)
**	    Add cur_subopt to SCS_CURRENT to bypass flattening
**	07-sep-89 (neil)
**	    Alerters: Added new trace points for events.
**	14-sep-89 (paul)
**	    Alerters control information added to SCS_SSCB
**	28-sep-1989 (ralph)
**	    Add cur_subopt to SCS_CURRENT to bypass flattening
**	19-oct-89 (paul)
**	    Add definition for the event thread for alerters.
**	16-jan-90 (ralph)
**	    Added ics_upass and ics_rupass to support user passwords.
**      10-may-1990 (fred)
**	    Saved more information from the tuple descriptor in the RDESC
**	    struct.  This information concerns the presence of the large object
**	    and compressed indicators.
**	09-oct-90 (ralph)
**	    6.3->6.5 merge:
**	    05-apr-1990 (fred)
**		Added ics_qbuf and ics_l_qbuf to allow monitoring of
**		which queries are running.
**	    30-apr-1990 (neil)
**		Cursor performance I: Modified SCS_CURSOR & SCS_CSI to track
**		current cursor pre-fetch activity.
**	    06-sep-90 (jrb)
**		Added ics_timezone to allow front-end to pass time zone to ADF.
**	06-feb-91 (ralph)
**          Add SCS_ICS.ics_sessid to contain the session id for
**          dbmsinfo('session_id')
**	04-feb-91 (neil)
**	    Changed sscb_ainterrupt to sscb_astate and defined values and
**	    flags.
**	06-nov-91 (ralph)
**	    6.4->6.5 merge:
**		12-feb-1991 (mikem)
**		    Added two fields cur_val_logkey and cur_logkey to the
**		    SCS_CURRENT structure to support returning logical key
**		    values to the client which caused the insert of the
**		    tuple and thus caused them to be assigned.
**      23-jun-1992 (fpang)
**	    Sybil merge.
**          Start of merged STAR comments:
**              13-Aug-91 (fpang)
**                   Added ics_timezone and ics_gw_parms to SCS_ICS for 
**		     support of GCA_TIMEZOBE and GCA_GW_PARMS modify assoc 
**		     messages.
**          End of STAR comments.
**	15-sep-92 (markg)
**	    Added sscb_sxccb, ssxb_sxscb and sscb_sxarea to SCS_SSCB, this
**	    is to enable support of the new SXF facility.
**	22-sep-1992 (bryanp)
**	    Added identifiers for new thread types: SCS_SRECOVERY,
**	    SCS_SLOGWRITER, SCS_SCHECK_DEAD,
**	    SCS_SGROUP_COMMIT, SCS_SFORCE_ABORT. Part of logging & locking
**	    system rewrite.
**	23-sep-1992 (stevet)
**	    Replace ics_timezone with ics_tz_name[DB_MAXNAME] to support
**	    new timezone adjustment method. 
**	7-oct-92 (daveb)
**	    Make GWF a first class facility with it's own sscb_gwscb, gwccb,
**	    and gwarea.
**	01-nov-92
**	    Added sscb_gcaparm_mask to SCS_SSCB for supporting byref parameters
**	01-nov-92
**	    Added cur_dbp_names to SCS_CURRENT for supporting byref parameters
**	20-nov-92 (markg)
**	    Added identifier for the audit thread - SCS_SAUDIT.
**	18-jan-1993 (bryanp)
**	    Add support for CPTIMER thread for timer-initiated consistency pts.
**	17-jan-1993 (ralph)
**	    DELIM_IDENT: Added SCS_ICS.ics_dbserv, which contains info regarding
**	    case translation semantics for the database.
**      24-feb-1993 (fred)
**          Added sscb_copy struct for use with large objects and copy.
**      10-mar-93 (mikem)
**          bug #47624
**          Added sscb_terminating field to indicate that session is running
**          code from scs_terminate.  During this time communication events
**          are disabled in scc_gcomplete().  This prevents unwanted
**          CSresume()'s from scc_gcomplete() incorrectly waking up other
**          CSsuspend()'s (like DI).
**	15-mar-93 (ralph)
**	    DELIM_IDENT: Added untranslated identifier fields to SCS_ICS;
**	    Added flags to indicate "special" user status
**	17-mar-93 (ralph)
**	    Change SCS_USR_E$INGRES to SCS_USR_EINGRES, 
**	    and SCS_USR_R$DBA to SCS_USR_RDBA.
**	15-apr-93 (jhahn)
**	    Added scscb_req_flags and SCS_NO_XACT_STMT for catching transaction
**	    control statements when they are disallowed.
**	02-apr-93 (ralph)
**	    DELIM_IDENT:  Add ics_cat_owner to contain the catalog owner name;
**	    will be "$ingres" if regular ids are to be translated to lower case,
**	    or "$INGRES" if regular ids are to be translated to upper case.
**	2-Jul-1993 (daveb)
**	    prototyped, moved all func externs here.
**	27-jul-93 (ralph)
**	    DELIM_IDENT: Add SCS_XEGRP_SPECIFIED to indicate that an effecive
**	    user identifier was specified on CONNECT.
**      12-aug-1993 (stevet)
**          Added ics_strtrunc for string truncate detection.
**      16-Aug-1993 (fred)
**          Added SCS_EAT_INPUT_MASK to sscb_req_flags.
**      10-sep-1993 (smc)
**          Made ics_sessid reflect the size of CV_HEX_PTR_SIZE. Files that
**	    include this header file must now include <cv.h>.
**	20-Sep-1993 (daveb)
**	    Add flags to marked "drop" connection state.
**       3-Nov-1993 (fred)
**          Add rqf tuple descriptor storage to current query and
**          cursor areas.  This to support the removal of use of stack
**          variable during long-term query processing.
**      10-Nov-1993 (fred)
**          Added paramater to prototype for scs_lowksp_alloc().   The
**          from_copy parameter indicates whether the wksp alloc'd is
**          for copy processing or not.  This is used to determine the
**          lifetime of the created temporary object.
**       2-Mar-1994 (daveb) 58423
**  	    Make scs_chk_priv() public.
**	 2-Apr-1995 (jenjo02)
**	    Added FUNC_EXTERN for scs_api_call
**	06-Apr-1995 (cohmi01)
**	    Add SCS_SWRITE_ALONG, SCS_SREAD_AHEAD for  write-along
**	    and read-ahead threads.
**	10-May-1995 (lewda02/shust01)
**	    Added a pointer to RAAT API message area in SCS_SSCB.
**	8-jun-1995 (lewda02/thaju02)
**	    Added structure SCS_RAAT_MSG and modified the SCS_SSCB.
**	11-sep-95 (wonst02)
**	    Add scs_facility() for info (to cssampler) re: the session's facility. 
**	14-Sep-1995 (shust01/thaju02)
**	    Added a pointer to raat work area (SCS_WORK_AREA) for blobs in
**	    SCS_SSCB, and added SCS_WORK_AREA.
**	    Added a pointer to raat big buffer (SCS_BIG_BUF) for blobs in
**	    SCS_SSCB, and added SCS_BIG_BUF.
**	14-nov-1995 (nick)
**	    Add II_DATE_CENTURY_BOUNDARY.
**	11-feb-1997 (cohmi01)
**	    Add IS_RAAT_REQUEST_MACRO().
**	03-apr-1997 (thaju02)
**	    Changed errno to local_errno in scs_cpsave_err prototype, due to
**	    errors in compilation of scscopy.c.
**	22-apr-1996 (loera01)
**	    Added new function, scs_compress(), to support variable-
**	    length datatype compression.
**	21-may-96 (stephenb)
**	    Add SCS_REP_QMAN define and proto for scs_dbadduser and 
**	    scs_dbdeluser.
**	03-jun-1996 (canor01)
**	    change "errno" to "errnum" since errno is defined as a
**	    function on multi-threading platforms.
**      01-aug-1996 (nanpr01 for ICL keving) 
**          Added LK Callback thread, SCS_SLKCBACKTHREAD.
**	15-Nov-1996 (jenjo02)
**	    Added Deadlock Detection thread, SCS_SDEADLOCK.
**	12-dec-1996 (canor01)
**	    Added Sampler thread, SCS_SSAMPLER, and prototype for scs_sampler().
**	23-sep-1997 (canor01)
**	    Add SCS_GCA_DESC for Frontier.
**      18-Dec-1997 (stial01)
**          Changed prototype for scs_fetch_data()
**	17-feb-1998 (somsa01)
**	    In SCS_CSI, added CSI_GCA_COMP_MASK as another csi_state;
**	    this is set if GCA_COMP_MASK (varchar compression) is turned on.
**	    (Bug #89000)
**      25-Feb-98 (fanra01)
**          Renamed scs_detach_scb to scs_scb_detach for consistency with the
**          attach function.
**	09-Mar-1998 (jenjo02)
**	    Moved SCS_S... session type defines to scf.h to expose them to
**	    other facilities.
**	    Added prototype for scs_atfactot();
**	    Added SCS_FTC structure.
**	    Removed function prototypes for scs_alloc_cbs(), scs_dealloc_cbs(),
**	    which are static, not external, functions.
**	    Added SCS_INTERNAL_THREAD flag to sscb_flags.
**	    Relocated SCD_MCB type here from scd.h, renamed it to 
**	    SCS_MCB, and embedded in SCS_SSCB.
**	8-june-1998(angusm)
**	    Cross-int mckba01's change for bug 71293 from ingres63p
**      15-Mar-1999 (hanal04)
**          Added SCF_DB_ADD_RETRIES to specify the number of times we
**          will try to add a database if we are timing out due to
**          dblist mutex / config file lock deadlocks. b55788.
**      27-May-1999 (hanal04)
**          Removed and reworked the above change for bug 55788.
**          Timeouts and retries replaced by early lock on CNF file. b97083.
**      28-Dec-2000 (horda03)
**          For Alerts (DBEvents) store the last alert in the list and the
**          number of alerts. This will improve performance adding alerts
**          to session queues (which may become very long!!). Bug 103596
**	04-Jan-2001 (jenjo02)
**	    Added (SCD_SCB*)scb parameter to prototypes for
**	    scs_dbadduser(), scs_dbdeluser(), scs_declare(),
**	    scs_clear(), scs_disable(), scs_enable(), scs_atfactot().
**	15-feb-2001 (somsa01)
**	    Sync'ed up structure size variables with the other structures.
**      13-aug-2002 (stial01)
**          Added sscb_xa_abort
**      12-Apr-2004 (stial01)
**          Define length as SIZE_TYPE.
**	11-May-2004 (schka24)
**	    Update scs_lowksp_allow prototype.
**	06-dec-2004 (thaju02)
**	    Added csi_thandle, csi_tlk_id - used to temporarily store 
**	    query text object handle. Set at cursor open and nulled out 
**	    during fetch. (B113588)
**	3-Feb-2006 (kschendel)
**	    user-only param for dbdb-info notused, remove.
**	22-Feb-2008 (kiria01) b119976
**	    Extra parameter to allow scs_compress to propagate error to
**	    calling code.
**       8-Jun-2009 (hanal04) Code Sprint SIR 122168 Ticket 387
**          Added _SCS_DBLIST as an anchor for the Must Log DB List
**	15-Jun-2009 (kiria01) SIR 121665
**	    Update GCA API to LEVEL 5
**	25-Mar-2010 (kschendel) SIR 123485
**	    Add sequencer action enum, COPY sequencer prototypes.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
*/


/*
**  Forward and/or External typedef/struct references.
*/

/* Misc sequencer related definitions */

/* Define an action code that says what to do when returning from a
** sequencer mainline helper (such as the scs_copy_xxx routines).
** The sequencer can:
** a) continue the main loop (the "massive for loop"), which is uncommon and
**    is generally reserved for retries;
** b) break out of the main statement-type switch (the "massive switch stmt"),
**    which generally implies the end of the query, with/without error,
**    and the sequencer mainline will process error/interrupt and clean up;
**    If the helper return value ret_val is anything other than E_DB_OK,
**    the sequencer will assume the query failed (GCA_FAIL_MASK).
** d) return(ret_val), which is used if the query isn't finished and
**    there are messages to send/receive.  It might also imply some sort of
**    serious situation (session vanished, etc) in which normal cleanup
**    is inappropriate or impossible.
*/

typedef enum {
	SEQUENCER_CONTINUE,		/* "continue" the main retryer loop */
	SEQUENCER_BREAK,		/* Break out of the stmt-type switch */
	SEQUENCER_RETURN		/* Return from sequencer immediately */
} sequencer_action_enum;


/*}
** Name: SCS_DBPRIV - Sequencer dbpriv information
**
** Description:
**	This structure is used to hold dbpriv information. 
**	It provides a convenient way to carry around all the information
**	needed for dbprivs.
**	
** History:
**	18-mar-94  (robf)
**        Created
*/
typedef struct _SCS_DBPRIV SCS_DBPRIV;

struct _SCS_DBPRIV
{
	SCS_DBPRIV	*scdbp_next;
	SCS_DBPRIV	*scdbp_prev;
	SIZE_TYPE	scdbp_length;
	i2		scdbp_type;
#define			SCR_DBPRIV_TYPE	0x424	/* Internal ID for structure*/
      	i2		scdbp_s_reserved;
	PTR		scdbp_l_reserved;
	PTR		scdbp_owner;
	i4		scdbp_tag;
#define                 SCDBP_TAG         CV_C_CONST_MACRO('s','c','d','p')
        u_i4       ctl_dbpriv;	
        u_i4       fl_dbpriv;	
        i4	        qrow_limit;		/* Query row  limit	*/
        i4	        qdio_limit; 		/* Query I/O  limit	*/
        i4	        qcpu_limit;	 	/* Query CPU  limit	*/
        i4	        qpage_limit; 		/* Query page limit	*/
        i4	        qcost_limit;		/* Query cost limit	*/
        i4	        idle_limit;		/* Idle time limit      */
        i4	        connect_limit;		/* Connect time limit   */
        i4	        priority_limit;		/* Priority limit       */

} ;

/*}
** Name: SCS_ICS - Ingres Control Structure
**
** Description:
**      This structure contains all the interesting information
**      used by ingres to control a user's interactions with a database.
**      Most of this information is extracted from the dbdb (if applicable)
**      at session start time.  The remainder is provided at startup
**      time by the application itself (e.g. query lang, decimal indicator)
**      It should be noted that this information is guaranteed to be valid
**      only at startup time.  Users are free to change some of this
**      information (such as decimal separator, date format) during their
**      interaction with the dbms through the use of (ick) set commands.
**
** History:
**     18-Jan-86 (fred)
**          Created on jupiter.
**	05-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    Added ics_grpid to SCS_ICS for session's group id
**	    Added ics_aplid to SCS_ICS for session's application id
**	    Added ics_apass to SCS_ICS for session's applid password
**	31-mar-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Added fields for database privileges:
**	    ics_fl_dbpriv, ics_qrow_limit, ics_qdio_limit, ics_qcpu_limit,
**	    ics_qpage_limit, ics_qcost_limit.
**	25-apr-89 (ralph)
**	    GRANT Enhancements, Phase 2a:
**	    Added ics_estat -- effective user status bits
**	01-may-89 (ralph)
**	    GRANT Enhancements, Phase 2b:
**	    Added ics_ctl_dbpriv, ics_gpass to SCS_ICS
**	02-may-89 (anton)
**	    Local collation changes
**	05-sep-89 (jrb)
**	    Added ics_num_literals field to indicate whether the user had
**	    chosen decimal or floating point numeric literals.
**	16-jan-90 (ralph)
**	    Added ics_upass and ics_rupass to support user passwords.
**	09-oct-90 (ralph)
**	    6.3->6.5 merge:
**	    05-apr-1990 (fred)
**		Added ics_qbuf and ics_l_qbuf to allow monitoring of
**		which queries are running.
**	    06-sep-90 (jrb)
**		Added ics_timezone to allow front-end to pass time zone to ADF.
**	06-feb-91 (ralph)
**          Add SCS_ICS.ics_sessid to contain the session id for
**          dbmsinfo('session_id')
**	20-may-91 (andre)
**	    Renamed fields describing user/group/role info so that their name
**	    holds a clue to whether the info belongs to the real user, session
**	    user, or the effective user.  Added new fields to contain user,
**	    group, and role identifiers for this session (ics_susername,
**	    ics_sgrpid, and ics_saplid).
**      23-jun-1992 (fpang)
**	    Sybil merge.
**          Start of merged STAR comments:
**              13-Aug-91 (fpang)
**                   Added ics_timezone and ics_gw_parms for support of 
**		     GCA_TIMEZOBE and GCA_GW_PARMS modify assoc messages.
**          End of STAR comments.
**      13-jul-92 (rog)
**          Increased the size of ics_qbuf from 128 to ER_MAX_LEN so that
**          we can trace more of the rogue query when we get an AV.
**      23-nov-92 (ed)
**          Changed DB_OWN_NAME to DB_PASSWORD for "maxname" project
**	03-dec-92 (andre)
**	    CURRENT_USER and USER refer to the "current <auth id>" which is the
**	    same as "session <auth id>" unless we are inside a module with
**	    <module auth id> in which case the "current <auth id>" would need to
**	    be reset to the <module auth id>; to accomplish this, we define
**		- ics_susername to contain SQL-session <auth id> and
**		- ics_musername to contain <module auth id>, if any.
**	    ics_eusername will point at ics_susername or ics_musername, as
**	    appropriate (at session startup it will point at ics_susername)
**
**	    since we are desupporting SET GROUP/ROLE, we delete ics_sgrpid and
**	    ics_saplid and rename
**		- ics_sgpass	    --> ics_egpass
**		- ics_sapass	    --> ics_eapass
**
**	    As a result of change in terminology, the following fields will also
**	    be renamed:
**		- ics_sustat	    --> ics_iustat
**		- ics_susername	    -->	ics_iusername
**		- ics_supass	    -->	ics_iupass
**		- ics_sucode	    --> ics_iucode
**	17-jan-1993 (ralph)
**	    DELIM_IDENT: Added SCS_ICS.ics_dbserv, which contains info regarding
**	    case translation semantics for the database.
**	15-mar-93 (ralph)
**	    DELIM_IDENT: Added untranslated identifier fields to SCS_ICS;
**	    Added flags to indicate "special" user status
**	17-mar-93 (ralph)
**	    Change SCS_USR_E$INGRES to SCS_USR_EINGRES, 
**	    and SCS_USR_R$DBA to SCS_USR_RDBA.
**	24-mar-93 (barbara)
**	    Removed Star fields ics_effect_user and ics_duser_desc as
**	    clean-up for Star delimited identifier support.
**	02-apr-93 (ralph)
**	    DELIM_IDENT:  Add ics_cat_owner to contain the catalog owner name;
**	    will be "$ingres" if regular ids are to be translated to lower case,
**	    or "$INGRES" if regular ids are to be translated to upper case.
**	27-jul-93 (ralph)
**	    DELIM_IDENT: Add SCS_XEGRP_SPECIFIED to indicate that an effecive
**	    user identifier was specified on CONNECT.
**	15-jun-93 (robf)
**	    Add ics_dbseclabel for database security label
**	    Add ics_maxustat, ics_defustat for default/maximum user privs
**	    Add ics_sec_label, ics_isec_label for current/initial session
**	    security label.
**	    Add ics_lim_sec_label, ics_expdate for limiting securty label
**	    and  expiration date
**	    Add ics_sl_type to store frontend security label type (from
**	        GCA_SL_TYPE)
**	    Add ics_idle_limit, ics_connect_limit, ics_priority_limit,
**	        ics_cur_priority.
**	5-nov-93 (robf)
**	    Add ics_srupass, system's notion of the real user password.
**	    Add ics_l_desc/ics_description for storing session description
**	14-nov-95 (nick)
**	    Add ics_year_cutoff.
**	01-oct-1999 (somsa01)
**	    Added SCS_USR_SVR_CONTROL, SCS_USR_NET_ADMIN, SCS_USR_MONITOR,
**	    and SCS_USR_TRUSTED to ics_uflags (Ingres system privileges).
**	26-mar-2001 (stephenb)
**	    Add ics_ucoldesc and ics_ucollation
**	24-oct-2001 (stephenb)
**	    Add ics_l_lqbuf and ics_lqbuf for last query buffer
**	3-Feb-2005 (schka24)
**	    Rename num_literals to parser_compat, add "tids are i4" flag
**	    to be passed along (in tedious stages) to the parser.
**	11-Sep-2006 (gupsh01)
**	    Added ics_dt_isingresdate to register whether date data type
**	    is synonymous to ingresdate or ansidate types.
**	02-Oct-2006 (gupsh01)
**	    Move ics_date_type_alias to SC_MAIN_CB.
**	03-Aug-2007 (gupsh01)
**	    Reinstate ics_date_alias for session level setting.
**      09-aug-2010 (maspa05) b123189, b123960
**          Added ics_dmf_flags2 to mirror dmc_flags_mask2 in DMC_CB
**          This is so we can pass DMC2_READONLYDB
*/
typedef struct _SCS_ICS
{

    /* now, some info about the user */

    i4		    ics_rustat;		/* real (current) user status bits */
    i4		    ics_iustat;		/* initial session user status bits */
    i4		    ics_defustat;	/* Default user privilege bits */
    i4		    ics_maxustat;	/* Maximum user privilege bits */
    i4		    ics_requstat;	/* required user status bits */
    i4              ics_ruid;		/* real user id (vms UIC) */
    u_i4	    ics_uflags;		/* "Special" user flags: */
#define		SCS_USR_EINGRES		0x0001L	/* Effective user is $ingres */
#define		SCS_USR_RDBA		0x0002L	/* Real user is dba */
#define		SCS_USR_RPASSWD		0x0004L /* Real user has password */
#define		SCS_USR_NOSUCHUSER	0x0008L /* User is invalid */
#define   	SCS_USR_PROMPT		0x0010L /* User can be prompted */
#define		SCS_USR_REXTPASSWD	0x0020L /* User has external password*/
#define		SCS_USR_DBPR_NOLIMIT	0x0040L /* User has no dbpriv limits */
#define		SCS_USR_SVR_CONTROL	0x0080L /* User has SERVER_CONTROL */
#define		SCS_USR_NET_ADMIN	0x0100L /* User has NET_ADMIN */
#define		SCS_USR_MONITOR		0x0200L /* User has MONITOR */
#define		SCS_USR_TRUSTED		0x0400L /* User has TRUSTED */
    DB_OWN_NAME	    ics_iusername;	/* he who invoked us (initial session
					** user) */
    DB_PASSWORD	    ics_iupass;  	/* initial session user password,
					*/
    DB_OWN_NAME	    ics_rusername;	/* he who invoked us in real life
					** (system user) */
    DB_PASSWORD	    ics_rupass;  	/* Real user password*/
    DB_PASSWORD	    ics_srupass;  	/* System Real user password, (iiuser)*/
    DB_TERM_NAME    ics_terminal;	/* useful for qrymod */
    DB_OWN_NAME	    ics_iucode;		/* copy of ics_iusername (why?) */
    DB_OWN_NAME	    *ics_eusername;	/* effective user name	*/
    DB_OWN_NAME	    ics_susername;	/* SQL-session <auth id> */
    DB_OWN_NAME	    ics_musername;	/* <module auth id> */
    char    	    ics_collation[DB_COLLATION_MAXNAME];/* collation name */
    char	    ics_ucollation[DB_COLLATION_MAXNAME]; /* Unicode collation */

    /* now, some info about the database */

    i4		    ics_dbstat;		/* the status bits from rel. rel. */
    DB_DB_NAME	    ics_dbname;		/* database we're working on */
    DB_OWN_NAME	    ics_dbowner;	/* who owns it */
    DB_OWN_NAME	    ics_cat_owner;	/* catalog owner */
    i4	    ics_dmf_flags;	/* flags db is opened with */
    i4	    ics_dmf_flags2;	/* flags db is opened with */
    i4	    ics_db_access_mode;	/* read or write */
    i4	    ics_lock_mode;
    i4		    ics_udbid;
    PTR		    ics_opendb_id;
    PTR		    ics_coldesc;	/* collation descriptor */
    PTR		    ics_ucoldesc;	/* unicode collation descriptor */
    u_i4	    ics_dbserv;		/* DU_DATABASE.du_dbservice flags */
    u_i4	    ics_dbxlate;	/* Case translation semantics flags */

    SCV_LOC	    *ics_loc;		/* locs which are known in this db */

    /* now, some info about the session */

    i4		    ics_appl_code;	/* Application code */
    i4		    ics_idxstruct;	/* -i flag */
    i4		    ics_mathex;		/* math exceptions */
    ADF_OUTARG	    ics_outarg;		/* output formats for adf */
    DB_LANG         ics_qlang;		/* quel or sql */
    DB_DECIMAL      ics_decimal;	/* '.' or ',' as in 5[,.]0 */
    DB_DATE_FMT	    ics_date_format;	/* date format of choice */
    i4		    ics_year_cutoff;	/* date_century cutoff */
    DB_MONY_FMT     ics_money_format;	/* money format/currency symbol */
    char            ics_tz_name[DB_TYPE_MAXLEN]; /* time zone of front end */
    i4		    ics_language;
    i4		    ics_parser_compat;	/* Parser compatibility settings.
					** See psq_parser_compat in psfparse.h
					*/
    char            ics_sessid[CV_HEX_PTR_SIZE];   /* Session identifier */

    /* some stuff about application/group identifiers */

    DB_OWN_NAME	    ics_egrpid;		/* effective group id */
    DB_PASSWORD	    ics_egpass;  	/* effective group id password */
    DB_OWN_NAME	    ics_eaplid;		/* effective application id */
    DB_PASSWORD	    ics_eapass;  	/* effective application id password */

    /* some security related stuff */
    DB_DATE	    ics_expdate;	/* Expiration date */

    /* some stuff about database privileges */

    u_i4	    ics_ctl_dbpriv;	/* Database privileges in effect:
					** see DB_PRIVILEGES for definitions
					** of possible values
					*/
    u_i4	    ics_fl_dbpriv;	/* Binary database privilege flags  */
    i4	    ics_qrow_limit;	/* Query row  limit		    */
    i4	    ics_qdio_limit;	/* Query I/O  limit		    */
    i4	    ics_qcpu_limit;	/* Query CPU  limit		    */
    i4	    ics_qpage_limit;	/* Query page limit		    */
    i4	    ics_qcost_limit;	/* Query cost limit		    */
    i4	    ics_idle_limit;	/* Idle time limit                  */
    i4	    ics_connect_limit;	/* Connect time limit               */
    i4	    ics_priority_limit;	/* Priority limit                   */
    i4	    ics_cur_priority;	/* Current Priority limit           */
    i4	    ics_cur_idle_limit;	/* Current Idle time limit          */
    i4	    ics_cur_connect_limit; /* Current Connect time limit    */
    i4         ics_l_qbuf;         /* Length of . . . */
    char            ics_qbuf[ER_MAX_LEN];      /* First few char's of query */
    i4         ics_l_lqbuf;         /* Length of . . . */
    char            ics_lqbuf[ER_MAX_LEN];      /* First few char's of last query */
    i4         ics_l_act1;         /* Length of . . . */
#define                 SCS_ACT_SIZE    80
    char            ics_act1[SCS_ACT_SIZE];/* Current major activity */
    i4         ics_l_act2;         /* Ditto for detailed activity */
    char            ics_act2[SCS_ACT_SIZE];

    /* Star extensions */
    DD_USR_DESC	    ics_real_user;	/* Star real user desc.  */
    PTR		    ics_gw_parms;	/* +c flag fom FE   */
    PTR		    ics_alloc_gw_parms; /* gw_parms with sc0m hdr */

    /* untranslated identifiers associated with the session */

    u_i4	    ics_xmap;		/* map of which id's were delimited */
#define		SCS_XDLM_EUSER	    0x01L    /* Eff user  was delimited	    */
#define		SCS_XDLM_EGRPID	    0x02L    /* Eff group was delimited	    */
#define		SCS_XDLM_EAPLID	    0x04L    /* Eff role  was delimited	    */
#define		SCS_XEGRP_SPECIFIED 0x08L    /* Effective user was specified*/
    DB_OWN_NAME	    ics_xrusername;	/* untranslated real username	    */
    DB_OWN_NAME	    ics_xeusername;	/* untranslated effective username  */
    DB_OWN_NAME	    ics_xegrpid;	/* untranslated effective group id  */
    DB_OWN_NAME	    ics_xeaplid;	/* untranslated effective role id   */
    DB_OWN_NAME	    ics_xdbowner;	/* untranslated DBA username	    */
    i4              ics_strtrunc;       /* string truncate option */
#define SCS_DESC_SIZE 256
    i4		    ics_l_desc;		/* Length of description */
    char	    ics_description[SCS_DESC_SIZE]; /* Session description */
    SCS_DBPRIV 	    ics_idbpriv;	/* Initial (no role) dbprivs */
#define SCS_DATE_ALIAS_ANSI           1
#define SCS_DATE_ALIAS_INGRES         0
    i2		    ics_date_alias;     /* date type alias valid
                                        ** values 'ingresdate' or 'ansidate'
                                        */
#define SCS_ENV_SIZE 65536
    char            *ics_env;           /* Environment description */
}   SCS_ICS;

/*}
** Name: SCS_RDESC - Description of qefcall() result
**
** Description:
**      This structure describes the results of a query such that they
**      can be sent to the front end program by the scs_send() routine.
**      The number of tuples contained in the output area, the description
**      of the tuples in the output area, and a pointer to the output
**      area itself are included.
**
** History:
**      15-Nov-1986 (fred)
**          Created
**	10-may-1990 (fred)
**	    Changed the [unused] rd_pad field to rd_modifier.  This field
**	    now holds the value of the gca_result_modifiers field from the tuple
**	    descriptor.
**       3-Nov-1993 (fred)
**          Add rqf tuple descriptor storage to current query and
**          cursor areas.  This to support the removal of use of stack
**          variable during long-term query processing.
**	05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with
**	    DM0M_OBJECT.
**	14-may-1997 (nanpr01)
**	    We need a field to save the pointer for the newly allocated
**	    piece of memory where we saved the current tuple descriptor.
**	21-Apr-2010 (kschendel) SIR 123485
**	    Delete lo_next/prev.  It was a list of coupons created by
**	    couponify (from the input stream), presumably for release at
**	    end-of-query;  but QEF/DMF has taken care of that for a long
**	    time.  The coupon list has been nothing but CPU overhead for
**	    years, apparently.  (!)
*/
typedef struct _SCS_RDESC
{
    PTR		    rd_tdesc;		/* tuple desc for networ ULC */
    i4		    rd_modifier;	/* gca_result_modifier values */
	/* See <gca.h>\GCA_TD_DATA\gca_result_modifier for #define's */
#define			SCS_LO_TYPE	0x0920
#define			SCS_LO_ASCII_ID	CV_C_CONST_MACRO('S','C','L','O')
    SCF_TD_CB       rd_rqf_desc;        /* Tuple desc's for STAR/rqf */
    i4	    rd_comp_size;	/* size of the saved tdesc buffer */
    PTR		    rd_comp_tdesc;      /* for varchar compression project
					** we need to allocate and save the
					** tdesc in scf
					*/
}   SCS_RDESC;

/*}
** Name: SCS_CSI - Cursor information block
**
** Description:
**      This structure contains information used by the sequencer to control
**      cursor activity in the system.
**
** History:
**      23-Jul-86 (fred)
**          Created on Jupiter
**	09-oct-90 (ralph)
**	    6.3->6.5 merge:
**	    30-apr-1990 (neil)
**		Cursor performance I: CSI_READONLY_MASK & CSI_SINGLETON_MASK
**	17-mar-94 (andre)
**	    defined CSI_REPEAT_MASK
**	17-mar-94 (andre)
**	    for repeat cursors involving parameters, we need to save the 
**	    parameters, pointers to them, and the number of them until the 
**	    cursor is closed.  Parameters themselves will be stored in QSF 
**	    memory, so we will save the QSF object handle and the lock id to 
**	    enable us to delete it when the cursor gets closed.  
** 
**	    QEQ_FETCH expects to get parameters packaged in QEF_PARAM structure,
**	    so we will save a template QEF_PARAM structure in SCS_CSI.
**	17-feb-1998 (somsa01)
**	    Added CSI_GCA_COMP_MASK as another csi_state; this is set if
**	    GCA_COMP_MASK (varchar compression) is turned on.  (Bug #89000)
**	31-may-06 (dougi)
**	    Added optionally used csi_textp/l fields to debug cursors.
[@history_template@]...
*/
typedef struct _SCS_CSI
{
    i4              csi_state;          /* The state of the cursor */
#define                 CSI_USED_MASK	    0x1
#define                 CSI_READONLY_MASK   0x2
                                /* The cursor is opened for read-only.  This
                                ** indication is returned to the client,
                                ** GCA_READONLY_MASK, when the cursor is opened,
                                ** and is kept around to prevent the
                                ** pre-fetching of more than one row when the
                                ** cursor is not read-only.
                                */
#define                 CSI_CLOSED_MASK     0x4
#define                 CSI_SINGLETON_MASK  0x8
                                /* The cursor query plan is a singleton plan.
                                ** This is useful for read-only cursors to
                                ** avoid trying to pre-fetch many rows when
                                ** we know that there is at most 1 row -
                                ** we can save memory and cpu usage.
                                */
#define			CSI_REPEAT_MASK	    0x10
				/* this is a repeat cursor; this is useful to
				** know when processing DEFINE CURSOR
				*/
#define			CSI_GCA_COMP_MASK   0x20
				/* Varchar compression is enabled on this
				** cursor's modifier.
				*/
    i4	    csi_rowcount;	/* row count of rows here */
    i4		    csi_tsize;		/* tuple size */
    PTR		    csi_qp;		/* handle for the qp */
    PTR		    csi_rep_qp;		/* handle for the replace qp */
    DB_CURSOR_ID    csi_id;             /* The cursor id */
    SCS_RDESC	    csi_rdesc;		/* description of result */
    PTR		    csi_amem;		/* memory allocated for this cursor */
    PTR		    csi_qef_data;	/* qef_data structs for the query */
    SCC_GCMSG	    *csi_tdesc_msg;	/* allocated msg holding tuple desc */
    PTR		    csi_rep_parm_handle;/* 
					** handle of the QSF object containing
					** the repeat cursor parameters - 
					** undefined if CSI_REPEAT_MASK is not 
					** set
					*/
    i4		    csi_rep_parm_lk_id;	/* 
					** lock id of the QEF object used to 
					** store the repeat cursor parameters
					*/
    struct _QEF_PARAM   *csi_rep_parms;	/* 
					** address of a structure describing 
					** repeat cursor parameters - undefined
					** if CSI_REPEAT_MASK is not set
					*/
    PTR		    csi_thandle;	/*
					** handle of QSF object containing 
					** query text
					*/
    i4		    csi_tlk_id;		/* 
					** lock id of the object storing
					** query text
					*/
    PTR		    csi_textp;		/* 
					** optional ptr to query text (only
					** filled in after tp sc924)
					*/
    i4		    csi_textl;		/*
					** length of text string (up to 
					** ER_MAX_LEN)
					*/
}   SCS_CSI;

/*}
** Name: SCS_CSITBL - Table of above
**
** Description:
**      This is simply a table of the above.  It is necessary since
**      the number of allowed cursors is installation defined.
**
**
** History:
**      23-Jul-86 (fred)
**          Created on Jupiter
[@history_template@]...
*/
typedef struct _SCSCSITBL
{
    SCS_CSI         csitbl[1];          /* 1 means array */
}   SCS_CSITBL;

/*}
** Name: SCS_CURSOR - Central point for cursor items
**
** Description:
**      This struct is a placeholder for all information about
**      Cursors.
**
** History:
**      23-Jul-86 (fred)
**          Created on Jupiter
**	09-oct-90 (ralph)
**	    6.3->6.5 merge:
**	    30-apr-1990 (neil)
**		Cursor performance I: Added curs_curno & curs_frows.
**	14-feb-2007 (dougi)
**	    Added curs_anchor, curs_offset for scrollable cursors.
*/
typedef struct _SCSCURSOR
{
    i4              curs_limit;         /* Number of cursors allowd for and alloc'd */
    i4              curs_curno;         /* Current cursor # when leaving SCS to
                                        ** continue pre-fetch operation.  Set at
                                        ** start of FETCH operation and may be
                                        ** reused as an index into the cursor
                                        ** table (csitbl)** upon re-entry into
                                        ** SCS after flushing a set of rows to
                                        ** the client.
                                        */
    i4              curs_frows;         /* Used to determine how many rows the
                                        ** client wants to pre-fetch.  Filled
                                        ** upon initial input of GCA_FETCH and
                                        ** modified based on cursor status, and
                                        ** whether this number of rows is
                                        ** really within the server/user
                                        ** bounds.
                                        */
    i4		    curs_anchor;	/* Copy of gca_anchor from scrolling
					** FETCH.
					*/
    i4		    curs_offset;	/* Copy of gca_offset from scrolling
					** FETCH.
					*/
    SCS_CSITBL	    *curs_csi;
}   SCS_CURSOR;

/*}
** Name: SCS_CURRENT - Processing current query
**
** Description:
**      This struct contains information used for the processing of
**      the current query.  It is essentially a workspace for the
**      sequencer.
**
**
** History:
**      23-Jul-1986 (fred)
**          Created on Jupiter.
**	28-sep-1989 (ralph)
**	    Add cur_subopt to SCS_CURRENT to bypass flattening
**      12-feb-1991 (mikem)
**          Added two fields cur_val_logkey and cur_logkey to the SCS_CURENT
**          structure to support returning logical key values to the client
**          which caused the insert of the tuple and thus caused them to be
**          assigned.
**	01-nov-92
**	    Added cur_dbp_names to SCS_CURRENT for supporting byref parameters
**	08-june-1998(angusm)
**	     Added field cur_p_allocated.  This field indicates whether memory
**	     was allocated for the QEF_PARAM structure pointed to by
**	     cur_qparam.  This memory will be allocated in the sequencer
**	     for REPEATED queries.
**	22-sep-03 (inkdo01)
**	    Add flag to show this is known to be a row producing procedure.
**	    This allows us to avoid re-locating the QP and fixes 110333.
**	12-Aug-2005 (schka24)
**	    Remove unused altarea flag.
**	14-Sep-2005 (schka24)
**	    Update doc for cur-state, as meanings have evolved.
**	17-july-2007 (dougi)
**	    Add cur_scrollable.
**	Jan 2010 (stephenb)
**	   Batch processing: add cur_qtxt_len and cur_qtxt.
**	28-may02010 (stephenb)
**	    Add cur_qry_parms to provide a sanity check that
**	    GCA sent us the right number of parameters for
**	    queries without text (it may not be checked in
**	    the parser)
*/
typedef struct _SCS_CURRENT
{
    i4		    cur_tup_size;	/* tuple size for current query */
    i4		    cur_state;		/* state of this query */
/* State is meaningful only for row returning queries, and is used to
** track how far along we are inside the sequencer.  (The sequencer
** whirls round and round, and one never knows when a retry might pop
** up, or when a row output buffer might fill.)
** The state transitions are currently different for row-producing procedures
** vs everything else.
** RPP's go first time -> tdesc done -> running.
**	First time means no tuple descriptor has been output.
**	Tdesc done means that we've constructed and queued the TDESCR
**	but no rows have been emitted.
**	Running means that at least one row has been emitted.
**	The distinctions are important for RPP's since we can get a
**	QEF retry either early on (if the QP is flushed after creation
**	and before we can manage to get into QEF), or later (if QEF
**	decides it needs a rule QP after execution starts, which might
**	be before or after emitting some rows!)
** All other queries go first time -> running.
**	First time means that no rows have been emitted.  TDESCR might or
**	might not have been output, and it doesn't matter.
**	Running means that at least one row has been emitted.
** Note: the order of the definitions is relevant, i.e. it must be possible
** to say something like "cur_state < CUR_RUNNING" to mean first time or
** tdesc done.
*/
#define			CUR_FIRST_TIME	0x1
#define			CUR_TDESC_DONE	0x2
#define                 CUR_RUNNING     0x3
#define			CUR_COMPLETED	0x4	/* Not currently used */
    i4		    cur_result;		/* indications for sql res/so/far */
    i4		    cur_lk_id;		/* lock we may have on qp */
    i4	    cur_row_count;		/* Total row count for this query */
    PTR		    cur_qe_cb;		/* pointer to the qef cb in use */
    QSO_OBID	    cur_qp;		/* query plan for current query */
    DB_CURSOR_ID    cur_qname;		/* query name for current cursor */
    SCS_RDESC	    cur_rdesc;		/* description of the result */
    PTR		    cur_amem;		/* memory allocated for this query */
    struct _QEF_DATA *cur_qef_data;	/* first qef_data struct for this
					** query.  qef-data's are allocated
					** sequentially as an array by scf,
					** so we can get to the N'th one by
					** just indexing instead of "next"
					** pointer chasing.
					*/
    PTR		    cur_qparam;		/* save loc for repeat q. params */
    i4		    cur_qprmcount;	/* ditto for count of these */
    i4         cur_val_logkey;     /* If either CUR_TABKEY or CUR_OBJKEY
                                        ** is enabled then a valid logical key
                                        ** is stored in cur_logkey, and should
                                        ** be returned to the client.
                                        */
#define                 CUR_TABKEY      0x01
                                        /* if asserted then table_key was
                                        ** assigned by the last insert.
                                        */
#define                 CUR_OBJKEY      0x02
                                        /* if asserted then object_key was
                                        ** assigned by the last insert.
                                        */
    DB_OBJ_LOGKEY_INTERNAL
                    cur_logkey;         /* logkey last assigned by this query */

    bool	    cur_subopt;		/* bypass flattening? */
    bool	    cur_dbp_names;	/* send names back with byref params? */
    bool	    cur_p_allocated;   /* have we allocated .cur_qparam */
    bool	    cur_rowproc;	/* 2nd (or subs.) loop on a row
					** producing procedure */
    bool	    cur_scrollable;	/* scrollable cursor */
    i4		    cur_qtxt_len;	/* size of cur_qtxt buffer */
    i4		    cur_qry_parms; 	/* number of parameters in current query */
    char	    *cur_qtxt;		/* saved query text, used if GCA 
					** does not provide query text */
}   SCS_CURRENT;

/*}
**  Name: SCS_DCCB - Direct connect session control block.
**
**  Description:
**	This structure contains information used for the processing of
**	direct connect. It has a standard sc0m type header so it can be 
**	allocated and deallocated. The main information contained are 
*	direct connect status information, and tuple descriptor status
**	information.
**
**  History:
**	04-Mar-1992 (fpang)
**	    Created for SYBIL.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
*/
typedef struct _SCS_DCCB SCS_DCCB;
struct _SCS_DCCB
{
    SCS_DCCB	    *dc_next;		/* Forward link */
    SCS_DCCB	    *dc_prev;		/* Back link */
    SIZE_TYPE	     dc_length;		/* Length, including portion not seen */
    i2		     dc_type;		/* Control block type */
    i2		     dc_s_reserved;	/* Used by mem mgmt */
    PTR		     dc_l_reserved;	/* Used by mem mgmt */
    PTR		     dc_owner;		/* Owner of block */
    i4		     dc_tag;		/* To look for in a dump */
    i4		     dc_mask;		/* Various status */
    SCC_SDC_CB	     dc_dcblk;		/* Direct connect state info */
    PTR		     dc_tdescmsg;	/* Ptr to allocated tuple desc msg */
    PTR		     dc_tdesc;		/* Ptr to tuple descriptor */
};

/*}
** Name: SCS_QTACB - Quert text audit control block
**
** Description:
**      This structure describes the area used primarily by
**      the sequencer to do query text  auditing
**
** History:
**     7-jul-93 (robf)
**	  Created
*/

typedef struct 
{
	i4	qta_seq;	/* sequence for current text */
	i4 qta_len;	/* Current number of chars in buffer */
#define SCS_QTA_BUFSIZE	256	/* Must be <= SXF detailtext maxsize */
	char	qta_buff[SCS_QTA_BUFSIZE]; /* Current text buffer */
} SCS_QTACB;


/*}
** Name: SCS_ALARM - Sequencer alarm information
**
** Description:
**	This structure is used to hold security alarms for a session
**	
** History:
**	3-dec-93  (robf)
**        Created
**	13-dec-93 (robf)
**        Added standard header for sc0m, list management,
**	  event name/owner.
*/
typedef struct _SCS_ALARM SCS_ALARM;
struct _SCS_ALARM
{
	SCS_ALARM	*scal_next;
	SCS_ALARM	*scal_prev;
	SIZE_TYPE	scal_length;
	i2		scal_type;
#define			SCA_ALARM_TYPE	0x410	/* Internal ID for structure*/
      	i2		scal_s_reserved;
	PTR		scal_l_reserved;
	PTR		scal_owner;
	i4		scal_tag;
#define                 SCA_TAG         CV_C_CONST_MACRO('s','c','a','_')
	DB_SECALARM alarm;	/* Alarm tuple  info */
	DB_EVENT_NAME event_name;	/* Event name */
	DB_OWN_NAME event_owner; /* Event owner */
} ;

/*}
** Name: SCS_ROLE - Sequencer role information
**
** Description:
**	This structure is used to hold role information for a session
**	
** History:
**	22-feb-94  (robf)
**        Created
**       8-mar-94 (robf)
**        Include role tuple.
*/
typedef struct _SCS_ROLE SCS_ROLE;
struct _SCS_ROLE
{
	SCS_ROLE	*scrl_next;
	SCS_ROLE	*scrl_prev;
	SIZE_TYPE	scrl_length;
	i2		scrl_type;
#define			SCR_ROLE_TYPE	0x420	/* Internal ID for structure*/
      	i2		scrl_s_reserved;
	PTR		scrl_l_reserved;
	PTR		scrl_owner;
	i4		scrl_tag;
#define                 SCR_TAG         CV_C_CONST_MACRO('s','c','r','_')
	DB_APPLICATION_ID roletuple;   /* role tuple */
	i4		  roleflags;
#define SCR_ROLE_DBPRIV 	0x0001	/* Role has dbprivs*/
	SCS_DBPRIV	  roledbpriv;  /* role dbprivileges */
} ;


/*
** Name: SCS_RAAT_MSG - Anchors for info related to RAAT API
**
*/
typedef struct _SCS_RAAT_MSG SCS_RAAT_MSG;
struct _SCS_RAAT_MSG
{
	SCC_GCMSG	message;
	SCS_RAAT_MSG	*next;
};


/*
** Name: SCS_BIG_BUF - Anchors for info related to RAAT API blobs, and other 
**		       big buffer items
**
*/
typedef struct _SCS_BIG_BUF SCS_BIG_BUF;
struct _SCS_BIG_BUF
{
	SCC_GCMSG	message;
	i4		length;
};

/*
** Name: SCS_WORK_AREA - Anchors for info related to blob work area
**
*/
typedef struct _SCS_WORK_AREA SCS_WORK_AREA;
struct _SCS_WORK_AREA
{
	SCC_GCMSG	message;
};


/*}
** Name: SCS_GCA_DESC - Sequencer GCA buffer description
**
** Description:
**      This structure describes the GCA buffer that has been interpreted.
**      This structure contains more information than the GCA structure
**      used to describe the interpreted received message buffer.
**
**
** History:
**      26-june-1996 (reijo01)
**          Written.
*/
typedef struct _SCS_GCA_DESC
{
    PTR         buf_ptr;                /* Pointer to buffer        */
    i4     buf_len;                /* Length of buffer         */
    PTR         data_ptr;               /* Pointer to data area     */
    i4     data_len;               /* Length of data area      */
    PTR         msg_ptr;                /* Pointer to message       */
    i4     msg_len;                /* Length of actual message */
    bool        msg_continued;          /* Message continuation     */
} SCS_GCA_DESC;


/*}
** Name: SCS_FTC - Factotum Thread Creation block
**
** Description:
**	This structure is the remnants of SCF_FTC after scdmain is
**	through with it. It contains the information needed by SCS
**	to initiate and terminate a Factotum thread.
**
**
** History:
**	09-Mar-1998 (jenjo02)
**	    Invented.
*/
typedef struct _SCS_FTC
{
    i4		ftc_state;		/* state of thread: */
#define		FTC_ENTRY_CODE_CALLED	1	/* entry code invoked */
#define		FTC_EXIT_CODE_CALLED	2	/* exit code invoked */
    PTR		ftc_data;		/* input data for thread, if any,*/
    i4		ftc_data_length;	/* and its length */
    DB_STATUS	(*ftc_thread_entry)();	/* thread execution code */
    VOID	(*ftc_thread_exit)();	/* optional thread termination code */
    CS_SID	ftc_thread_id;		/* sid of creating (parent) thread */
    DB_STATUS   ftc_status;		/* status returned by execution code */
    DB_ERROR	ftc_error;		/* error returned by execution code */
} SCS_FTC;

/*}
** Name: SCS_MCB - Memory control block
**
** Description:
**      This structure is used to keep track of the memory allocated
**      through the system control facility.  The memory is allocated
**      to a particular facility and a particular session.  This information
**      as well as the amount of memory allocated and where this memory
**      is located is maintained here.
**
** History:
**     14-Jan-86 (fred)
**          Created
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedefs to quiesce the ANSI C 
**	    compiler warnings.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	01-Apr-1998 (jenjo02)
**	    Relocated here from scd.h (SCD_MCB) and embedded in SCS_SSCB.
*/

struct _SCS_MCB
{
    SCS_MCB	    *mcb_next;		/* forward pointer */
    SCS_MCB	    *mcb_prev;		/* backward pointer */
    SIZE_TYPE	    mcb_length;		/* how long is the structure */
    i2		    mcb_type;
# define		MCB_ID  	1
    i2		    mcb_s_reserved;	/* possibly used by mem mgmt */
    PTR		    mcb_l_reserved;
    PTR		    mcb_owner;		/* owner for internal mem mgmt */
    i4		    mcb_ascii_id;	/* so as to see in a dump */

# define		MCB_ASCII_ID	CV_C_CONST_MACRO('M', 'C', 'B', ' ')

    struct
    {
	SCS_MCB		*sque_next;
	SCS_MCB		*sque_prev;
    }		    mcb_sque;

    SCF_SESSION	    mcb_session;	/* which session owns this memory */
    i4		    mcb_facility;	/* facility which owns this memory */

#define                 NO_FACILITY     0	/* indicates free */
    /* others taken from DBMS.H */    
    SIZE_TYPE	    mcb_page_count;	/* number of pages allocated */
    PTR		    mcb_start_address;	/* where these pages are */

					/* 
					** Only one address is ever needed.
					** This memory is never fragmented
					*/
};


/*}
** Name: SCS_SSCB - Sequencer session control block
**
** Description:
**      This structure describes the area used primarily by
**      the sequencer to control a session.
**
**
** History:
**     15-Jan-86 (fred)
**          Created on Jupiter.
**     15-jul-88 (rogerk)
**	    Added Fast Commit thread type.
**     15-sep-88 (rogerk)
**	    Added Write Behind thread type
**     10-Mar-89 (ac)
**	    Added 2PC support.
**	07-sep-89 (neil)
**	    Alerters: Added new trace points for events.
**	21-mar-90 (neil)
**	    Changed sscb_ainterrupt to sscb_astate and defined values and
**	    flags.
**	15-sep-92 (markg)
**	    Added sscb_sxccb, ssxb_sxscb and sscb_sxarea to support new
**	    SXF facility.
**	22-sep-1992 (bryanp)
**	    Added identifiers for new thread types: SCS_SRECOVERY,
**	    SCS_SLOGWRITER, SCS_SCHECK_DEAD,
**	    SCS_SGROUP_COMMIT, SCS_SFORCE_ABORT. Part of logging & locking
**	    system rewrite.
**	7-oct-92 (daveb)
**	    Make GWF a first class facility with it's own sscb_gwscb, gwoccb,
**	    and gwarea.
**	01-nov-92
**	    Added sscb_gcaparm_maskto support byref parameters
**	20-nov-92 (markg)
**	    Added identifier for the audit thread - SCS_SAUDIT.	
**	18-jan-1993 (bryanp)
**	    Add support for CPTIMER thread for timer-initiated consistency pts.
**  	24-Feb-1993 (fred)
**          Added sscb_copy member.
**      10-mar-93 (mikem)
**          bug #47624
**          Added sscb_terminating field to indicate that session is running
**          code from scs_terminate.  During this time communication events
**          are disabled in scc_gcomplete().  This prevents unwanted
**          CSresume()'s from scc_gcomplete() incorrectly waking up other
**          CSsuspend()'s (like DI).
**	15-apr-93 (jhahn)
**	    Added scscb_req_flags and SCS_NO_XACT_STMT for catching transaction
**	    control statements when they are disallowed.
**      16-Aug-1993 (fred)
**          Added SCS_EAT_INPUT_MASK to sscb_req_flags.  This is used 
**          when the system detects a recoverable error in the input 
**          stream.  This flag's presence tells scs_input() to just 
**          eat blocks until the EOD indicator has been reached.  Once 
**          reached, then set the sscb_state field to SCS_IERROR which 
**          will skip normal query processing...
**	20-Sep-1993 (daveb)
**	    Add flags to marked "drop" connection state.
**	25-oct-93 (robf)
**          Add terminator thread, SCS_SCHECK_TERM
**	3-dec-93 (robf)
**          Add sscs_alcb, point to alarm control block list
**	22-feb-94 (robf)
**          Add role information for the session in sscb_alcb
**	10-May-1995 (lewda02/shust01)
**	    Added a pointer to RAAT API message area in SCS_SSCB.
**	14-Sep-1995 (shust01/thaju02)
**	    Added a pointer to raat work area (SCS_WORK_AREA) for blobs in
**	    SCS_SSCB, and added SCS_WORK_AREA.
**	    Added a pointer to raat big buffer (SCS_BIG_BUF) for blobs in
**	    SCS_SSCB, and added SCS_BIG_BUF.
**      01-aug-1996 (nanpr01 for ICL keving) 
**          Added LK Callback thread, SCS_SLKCBACKTHREAD.
**	15-Nov-1996 (jenjo02)
**	    Added Deadlock Detection thread, SCS_SDEADLOCK.
**      25-Jul-2000 (hanal04) Bug 102108 INGSRV 1230
**          Added SCS_ASEM to sscb_aflags.
**	3-feb-98 (inkdo01)
**	    Added sort thread, SCS_SORT_THREAD.
**	09-Mar-1998 (jenjo02)
**	    Moved SCS_S... session type defines to scf.h to expose them to
**	    other facilities.
**	    Added SCS_FTC, sscb_factotum_parms in support of Factotum threads.
**	    Added sscb_need_facilities, removed SCS_NOGCA flag. sscb_need_facilities,
**	    sscb_facilities will indicate GCA presence via (1 << DB_GCF_ID) mask.
**	    Embed SCS_MCB memory list head in here (relocated from deprecated
**	    SCD_DSCB).
**	15-feb-98 (stephenb)
**	    add sscb_blobwork field
**      28-Dec-2000 (horda03) Bug 103596 INGSRV 1349
**          Added sscb_last_alert and sscb_num_alerts.
**      15-May-2002 (hanal04) Bug 106640 INGSRV 1629
**          Added SCS_STAR_INTERRUPT so that we can flag interrupt
**          processing and therefore identify an interrupt of
**          an interrupt.
**	13-Jul-2005 (thaju02)
**	    Add SCS_ON_USRERR_ROLLBACK.
**	30-Aug-2006 (jonj)
**	    Add SCS_XA_START
**	Jan 2010 (stephenb)
**	    Batch processing: Add SCS_INSERT_OPTIM, SCS_REPEAT_QUERY,
**	    and scs_cpy_qeccb.
**	26-Mar-2010 (kschendel) SIR 123485
**	    Delete SCS_COPY stuff.  It can live in the QEU_COPY block,
**	    rather than taking up space in the sscb all the time.
**	30-Mar-2010 (kschendel) SIR 123485
**	    Re-type some ptr's as the proper struct pointer.
**	    Delete unused xxarea pointers, nonexistent ULF cb pointers.
**	21-Apr-2010 (kschendel) SIR 123485
**	    Delete blobwork, always included in lowksp now.
**	26-jun-2010 (stephenb)
**	    Add SCS_COPY_STARTED.
*/
typedef struct _SCS_SSCB
{
	    /*
	    ** First, the controlling information which we use alot
	    */
    i4		    sscb_stype;		/* session type */
    i4		    sscb_flags;		/* session flags */
#define			SCS_VITAL	    0x01    /* If this session is
						    ** terminated then server
						    ** must shut down */
#define			SCS_PERMANENT	    0x02    /* Session is expected to
						    ** last the life of the
						    ** server */
#define			SCS_STAR	    0x04    /* Do distributed work */
#define			SCS_DROPPED_GCA	    0x08    /* GCA connection is known
						       to be dropped/bad, so don't
						       log more errors */
#define			SCS_DROP_PENDING    0x10    /* GCA connection should be
						       dropped at earliest
						       opportunity. */
#define			SCS_INTERNAL_THREAD 0x20    /* Internal thread */
#define                 SCS_STAR_INTERRUPT  0x40    /* Used to flag scs_dccont()
						       so that qef_call() errors
						       are passed up when 
						       sscb_interrupt is not
						       equal to 0 */
#define			SCS_ON_USRERR_ROLLBACK 0x80 /* used for set session with
						       on_user_error = rollback */
#define			SCS_XA_START 	    0x100   /* Session is using XA_STARTed
						       transaction semantics */
#define			SCS_INSERT_OPTIM    0x200   /* session is using insert 
						       optimization */
#define			SCS_COPY_STARTED    0x400   /* session has started the copy
						       for the above optimization */
    i4		    sscb_need_facilities; /* which fac's are needed */
    i4              sscb_facility;	  /* which fac's have started */
    i4		    sscb_cfac;
    i4		    sscb_ciu;		/* were cursors used */

    SCS_MCB	    sscb_mcb;		/* Head of session's SCF memory list */

    /*
    ** The constants found in PSFPARSE.H will be used here
    */

    i4              sscb_qmode;         /* current query mode */
    i4		    sscb_state;		/* what is going on in the world */
    SCS_CURRENT	    sscb_cquery;
    SCS_CURSOR	    sscb_cursor;
    DB_TAB_TIMESTAMP
		    sscb_comstamp;	/* commit stamp for last xact */

    DB_DIS_TRAN_ID  sscb_dis_tran_id;	/*
					** Distributed transaction id for the
					** current slave transaction
					*/

    /*
    ** The following fields are used to service input
    ** requests.
    */

    i4		    sscb_req_flags;	 /* request flags flags */
#define			SCS_NO_XACT_STMT 0x01 /* No transaction statements
					      ** allowed fpr this request */
#define                 SCS_EAT_INPUT_MASK 0x02 /* skip remaining */
						/* input */
#define			SCS_REPEAT_QUERY 0x04 /* repeat last query
					      ** (no query text sent from client)
					      */

    PTR		    sscb_lowksp;	/* Workspace for dealing w/BLOBs */
    PTR		    sscb_phandle;	/* parameters for saving over calls */
    i4  	    sscb_plk_id;	/* lock id for the stream */
    PTR		    sscb_p1handle;	/* parameters for saving over calls */
    i4  	    sscb_p1lk_id;	/* lock id for the stream */
    PTR		    sscb_proot;	
    PTR		    sscb_thandle;	/* stream id for input data */
    PTR		    sscb_troot;		/* pointer to root of above */
    i4  	    sscb_tlk_id;	/* lock id for the stream */
    char	    *sscb_tptr;		/* pointer to current place in above */
    i4		    sscb_tsize;		/* number of bytes in current piece */
    i4		    sscb_tnleft;	/* number left in contig pool */
    char	    *sscb_dptr;		/* pointer to current data area */
    i4		    sscb_dsize;		/* size of current piece */
    i4		    sscb_dnleft;	/* number of bytes left */
    i4		    sscb_dmm;		/* in data movement mode ? */
    GCA_DATA_VALUE  sscb_gcadv;		/* holds temporary comm data */
    i4		    sscb_gclname;	/* length of parameter name */
    char	    sscb_gcname[DB_MAXNAME];
    i4	    sscb_gcaparm_mask;	/* parameter mask */
    i4		    sscb_pstate;	/* current parameter state	    */
#define			SCS_GCNORMAL	0x0
#define			SCS_GCNMLEN	0x1 /* Awaiting remainder of name   */
					    /* length			    */
#define			SCS_GCNMENTIRE	0x2 /* Awaiting remainder of name   */
#define                 SCS_GCMASK      0x3 /* Awaiting remainder of mask   */
    i4		    sscb_rsptype;	/* type of response msg to send */

    /* some statistics */

    i4		    sscb_noerrors;	/* number of errors by this user */
    i4		    sscb_noretries;	/* number of retries on this query */
    i4		    sscb_nointerrupts;	/* number of interrupts */
    i4		    sscb_nostatements;	/* Number of DB statements executed */
    i4		    sscb_interrupt;
#define			SCS_PENDING_INTR    0x2
    i4		    sscb_force_abort;
#define			SCS_FAPENDING	    0x2 /* Force pending */
#define                 SCS_FORCE_ABORT	    0x1	/* forcing abort of current xact */
#define			SCS_FACOMPLETE	    0x3 /* User notification pending */
    i4		    sscb_xa_abort;
    i4		    sscb_eid;

#define			SCS_TBIT_COUNT	    8
#define                 SCS_TVALUE_COUNT    1
    ULT_VECTOR_MACRO(SCS_TBIT_COUNT, SCS_TVALUE_COUNT)
		    sscb_trace;		/* Trace bits */
#define                 SCS_TPRINT_QRY	    0
#define			SCS_TEVENT	    1
#define			SCS_TEVPRINT	    2	/* PRINT user events */
#define			SCS_TEVLOG	    3	/* LOG user events */
#define			SCS_TALERT	    4	/* Trace sce operations */
#define			SCS_TNOTIFY	    5	/* Trace event notifications */
#define			SCS_TASTATE	    6	/* Trace alert state changes */
    i4              sscb_terminating;           /* session is terminating */

    /* Alert processing control information */
    CS_SEMAPHORE    sscb_asemaphore;
    PTR		    sscb_alerts;	/* List of pending alerts */
    PTR             sscb_last_alert;    /* Location of last alert in list */
    i4              sscb_num_alerts;    /* Number of alerts in list */ 
    i4		    sscb_aflags;	/* Session-specific alert flags */
#define			SCS_AFSTATE	0x001	/* Print "astate" transitions
						** defined below.
						*/
#define			SCS_AFINPUT	0x002	/* "Real" user input was read
						** and do not try to continue
						** processing alerts.  Set by
						** scs_input.
						*/
#define			SCS_AFPURGE	0x004	/* A "purge-style" message is
						** being sent so don't add any
						** alerts as they'll get lost.
						** Set by sequencer when adding
						** GCA_IACK and reset by scc
						** when sending the message.
						*/
#define			SCS_ASEM   	0x008	/* The sscb_asemaphore has 
						** been initialised. If this
						** flag is not set we should
						** not call CSr_semaphore()
						** for the sscb_asemaphore
						*/
    i4		    sscb_astate;	/* Alert state:  
					** NOTE: For a thorough description of
					** alert states read the introductory
					** comment in the module SCENOTIFY.
					** PLEASE DO NOT add or delete states
					** without modifying that description
					** to reflect the meaning and its use.
					** If you add any more states be sure to
					** update sce_trstate.
					** (neil, 21-mar-90)
					*/
#define			SCS_ASDEFAULT	0	/* Normal mode during query */
#define			SCS_ASWAITING	1	/* Can be interrupted */
#define			SCS_ASNOTIFIED	2	/* Was interrupted */
#define			SCS_ASPENDING	3	/* Alert-send pending */
#define			SCS_ASIGNORE	4	/* Ignore GCA completion */

    /* random facilities' control areas for the session */

    
    struct _ADF_CB  *sscb_adscb;	/* address of session blk for adf */
    PTR             sscb_dmscb;		/* DMF session control block */
    struct _DMC_CB  *sscb_dmccb;	/* DMF DMC_CB call control block */
    PTR             sscb_opscb;		/* OPF session, call cb's */
    struct _OPF_CB  *sscb_opccb;
    struct _PSS_SESBLK *sscb_psscb;	/* PSF session control block */
    struct _PSQ_CB  *sscb_psccb;	/* "call cb" is query state CB */
    struct _QEF_CB  *sscb_qescb;	/* QEF session, call cb's */
    struct _QEF_RCB *sscb_qeccb;
    struct _QSF_CB  *sscb_qsscb;	/* QSF session, call cb's */
    struct _QSF_RCB *sscb_qsccb;
    PTR		    sscb_rdscb;		/* RDF session, call cb's */
    struct _RDF_CCB *sscb_rdccb;
    struct _SCF_CB  *sscb_scccb;	/* SCF call, session cb's */
    struct _SCD_SCB *sscb_scscb;
    struct _SXF_RCB *sscb_sxccb;	/* SXF call, session cb's */
    PTR		    sscb_sxscb;
    PTR		    sscb_gwscb;		/* GWF session, call cb's */
    struct _GW_RCB  *sscb_gwccb;
    PTR		    sscb_dbparam;
#define                 SCS_SQPARAMS    16
    PTR		    *sscb_param;
    PTR		    sscb_ptr_vector[SCS_SQPARAMS];
    i4	    sscb_opendb_id;	/* id of db when opened */
    SCV_DBCB	    *sscb_dbcb;		/* pointer to open db cb for session */ 

    /* finally, some startup ingres type information */

    SCS_ICS         sscb_ics;           /* ingres type information */
    SCS_DCCB        *sscb_dccb;         /* Direct Connect info block */
    SCS_QTACB	    sscb_qtacb;		/* Query text audit control block */

    bool	    sscb_is_idle;	/* Session is marked as idle */
    SYSTIME	    sscb_last_idle;	/* Time session was marked idle */
    SYSTIME	    sscb_connect_time;  /* Time session started */

    SCS_ALARM	    *sscb_alcb;		/* Alarm list for session */
    SCS_ROLE	    *sscb_rlcb;		/* Role list for session */
    SCS_RAAT_MSG    *sscb_raat_message; /* RAAT API message area */
    SCS_BIG_BUF     *sscb_big_buffer;   /* special RAAT msg area for blobs, etc */
    i4	    sscb_raat_buffer_used; /* how many message buffers used */
    SCS_WORK_AREA   *sscb_raat_workarea; /* save pointer for workarea w/blob */
    SCS_GCA_DESC    sscb_gca_desc;      /* GCA buffer description */
    PTR             sscb_segment_buffer;   /* ->buffer to hold gca data */
    SCS_FTC	    sscb_factotum_parms;   /* Parms to initiate factotum thread */
    PTR		    sscb_cpy_qeccb;	/* QEF_RCB used for insert to copy optimization 
					** this is saved between trips through the
					** sequencer and becomes invalid on commit
					** (when dynamic SQL psf memory stream and QSF 
					** objects are destroyed)
					*/
}   SCS_SSCB;


/*}
** Name: SCS_IOMASTER - Anchors for info related to IOmaster server
**
*/
typedef struct _SCS_IOMASTER SCS_IOMASTER;
struct _SCS_IOMASTER
{
	i4		dbcount;
	char		*dblist;
	i4		num_ioslaves;	
};

/*}
** Name: SCS_DBLIST - Anchors for info requiring a DB list
**
*/
typedef struct _SCS_DBLIST SCS_DBLIST;
struct _SCS_DBLIST
{
        i4              dbcount;
        char            **dblist;
};


/**
** Name: SCS_ASTATE_MACRO	- Macro to trace session alert state transitions
**
** Description:
**	Due to the complex nature of the alert state transitions (described
**	in scenotify.c) this macro enables us to trace said transitions.
**
** History:
**	21-mar-90 (neil)
**	    Written for alert notification.
**/

#define	SCS_ASTATE_MACRO(title, sscb, new_astate)			\
	  if ((sscb).sscb_aflags & SCS_AFSTATE)				\
	    sce_trstate((title), (sscb).sscb_astate, new_astate);	\
	  (sscb).sscb_astate = new_astate

/**
** Name: IS_RAAT_REQUEST_MACRO	- return true if this is a RAAT request.
**
** Description:
**	Encapsulate the various tests that are being used by the sequencer  
**	to identify whether the front end request is a RAAT command.      
**
** History:
**	11-feb-97 (cohmi01)
**	   Added. 
**/

#define IS_RAAT_REQUEST_MACRO(scb)  \
    (((scb)->scb_cscb.cscb_gcp.gca_it_parms.gca_message_type == GCA_QUERY && \
        ((GCA_Q_DATA *)((scb)->scb_cscb.cscb_gcp.gca_it_parms.gca_data_area))\
        ->gca_language_id == DB_NDMF && (scb)->scb_sscb.sscb_thandle == 0) ||\
        ((scb)->scb_sscb.sscb_raat_buffer_used != 0))


typedef STATUS SCS_SCAN_FUNC( SCD_SCB *scb, PTR arg );

typedef void  SCS_SES_OP_FUNC( SCD_SCB *scb );

/*
** Func Externs of (some) things exported outside scs
*/
    
FUNC_EXTERN sequencer_action_enum scs_copy_bad(
				SCD_SCB *scb,
				i4 *next_op,
				DB_STATUS *ret_val);

FUNC_EXTERN sequencer_action_enum scs_copy_begin(
				SCD_SCB *scb,
				i4 *next_op,
				DB_STATUS *ret_val);

FUNC_EXTERN sequencer_action_enum scs_copy_from(
				SCD_SCB *scb,
				i4 *next_op,
				DB_STATUS *ret_val);

FUNC_EXTERN sequencer_action_enum scs_copy_into(
				SCD_SCB *scb,
				i4 op_code,
				i4 *next_op,
				DB_STATUS *ret_val);

FUNC_EXTERN VOID scs_copy_error(SCD_SCB *scb, 
				QEF_RCB *qe_ccb,
				i4 copy_done );

FUNC_EXTERN VOID scs_cpinterrupt(SCD_SCB *scb );

FUNC_EXTERN DB_STATUS scs_dbopen(DB_DB_NAME *db_name,
				 DB_DB_OWNER *db_owner,
				 SCV_LOC *loc,
				 SCD_SCB *scb,
				 DB_ERROR *error,
				 i4  flag,
				 DMC_CB *dmc,
				 DU_DATABASE *db_tuple,
				 SCV_DBCB **dbcb_loc );

FUNC_EXTERN DB_STATUS scs_dbclose(DB_DB_NAME *db_name,
				  SCD_SCB *scb,
				  DB_ERROR *error,
				  DMC_CB *dmc );

FUNC_EXTERN DB_STATUS scs_dbms_task(SCD_SCB *scb );

FUNC_EXTERN STATUS scs_atfactot(SCF_CB *scf,
				SCD_SCB *scb );

FUNC_EXTERN bool scs_ddbsearch(char *db_name,
			     SCV_DBCB  *dbcb_list,
			     SCV_DBCB  *terminal,
			     SCV_DBCB **search_point );

FUNC_EXTERN DB_STATUS scs_ddbremove(SCV_DBCB *dbcb );

FUNC_EXTERN DB_STATUS scs_ddbclose(SCD_SCB *scb );

FUNC_EXTERN SCV_DBCB * scs_ddbadd(PTR db_owner,
				  PTR db_name,
				  i4 access_mode,
				  SCV_DBCB *dbcb );

FUNC_EXTERN DB_STATUS scs_ddbdb_info(SCD_SCB *scb,
				     PTR dbdb_name,
				     DB_ERROR *error );

FUNC_EXTERN DB_STATUS scs_audit_thread( SCD_SCB *scb);

FUNC_EXTERN i4 scs_initiate(SCD_SCB *scb );

FUNC_EXTERN DB_STATUS scs_terminate(SCD_SCB *scb );

FUNC_EXTERN DB_STATUS scs_icsxlate(SCD_SCB *scb, DB_ERROR *error );

FUNC_EXTERN VOID scs_merge_dbpriv(SCD_SCB *scb, 
		SCS_DBPRIV *inpriv,
		SCS_DBPRIV *outpriv);

FUNC_EXTERN VOID scs_cvt_dbp_tuple(SCD_SCB *scb, 
		DB_PRIVILEGES *inpriv,
		SCS_DBPRIV *outpriv);

FUNC_EXTERN DB_STATUS scs_put_sess_dbpriv( SCD_SCB *scb, 
	SCS_DBPRIV *dbpriv);

FUNC_EXTERN VOID scs_init_dbpriv(SCD_SCB *scb,
	 SCS_DBPRIV *dbpriv,
	 bool       all_priv);

FUNC_EXTERN DB_STATUS scs_load_role_dbpriv( 
		SCD_SCB *scb, 
		QEU_CB *qeu,
		DB_PRIVILEGES *dbpriv_tup);

FUNC_EXTERN DB_STATUS scs_get_dbpriv(SCD_SCB *scb,
				     QEU_CB *qeu,
				     DB_PRIVILEGES *inpriv,
				     SCS_DBPRIV    *outpriv);

FUNC_EXTERN DB_STATUS scs_dbdb_info(SCD_SCB *scb,
				    DMC_CB *dmc,
				    DB_ERROR *error );

FUNC_EXTERN VOID scs_dmf_to_user(DB_ERROR *error );

FUNC_EXTERN VOID scs_ctlc(SCD_SCB *sid );

FUNC_EXTERN VOID scs_log_event(SCD_SCB *sid );

FUNC_EXTERN STATUS scs_disassoc(i4 assoc_id );

FUNC_EXTERN int scs_msequencer(i4 op_code, SCD_SCB *scb, i4  *next_op );

FUNC_EXTERN int scs_api_call(i4 op_code, SCD_SCB *scb, i4  *next_op );

FUNC_EXTERN VOID scs_mo_init(void);

FUNC_EXTERN STATUS scs_scb_attach( SCD_SCB *scb );

FUNC_EXTERN STATUS scs_scb_detach( SCD_SCB *scb );

FUNC_EXTERN DB_STATUS scs_sequencer(i4 op_code, SCD_SCB *scb, i4  *next_op );

FUNC_EXTERN DB_STATUS scs_input(SCD_SCB *scb, i4  *next_state );

FUNC_EXTERN VOID scs_qef_error(DB_STATUS status,
			       i4 err_code,
			       i4 error_blame,
			       SCD_SCB *scb );

FUNC_EXTERN VOID scs_qsf_error(DB_STATUS status,
			       i4 err_code,
			       i4 error_blame );

FUNC_EXTERN VOID scs_gca_error(DB_STATUS status,
			       i4 err_code,
			       i4 error_blame );

/* scs.h users may not have qefcopy.h, so define this: */
typedef struct _QEU_COPY *QEU_COPY_PTR;
FUNC_EXTERN DB_STATUS scs_blob_fetch(SCD_SCB *scb,
				     DB_DATA_VALUE *coupon_dv,
				     i4  *qry_size,
				     char **buffer,
				     QEU_COPY_PTR qeu_copy);

FUNC_EXTERN DB_STATUS scs_fetch_data(SCD_SCB *scb,
				     i4  for_psf,
				     i4  qry_size,
				     char *buf,
				     i4  print_qry,
				     bool *ad_peripheral );

FUNC_EXTERN DB_STATUS scs_fdbp_data(SCD_SCB *scb,
				    i4  qry_size,
				    char *buf,
				    i4  print_qry );

FUNC_EXTERN DB_STATUS scs_qt_fetch(SCD_SCB *scb,
				   i4  *p_qry_size,
				   char **p_buf );

FUNC_EXTERN DB_STATUS scs_desc_send(SCD_SCB *scb, SCC_GCMSG **pmessage );

FUNC_EXTERN DB_STATUS scs_access_error(i4 qmode );

FUNC_EXTERN i4  scs_csr_find(SCD_SCB *scb );

FUNC_EXTERN VOID scs_cursor_remove(SCD_SCB *scb, SCS_CSI *cursor );

FUNC_EXTERN DB_STATUS scs_lowksp_alloc(SCD_SCB 	*scb,
				       PTR  	*lowkspp);

FUNC_EXTERN DB_STATUS ldb_fetch(char **from,
				u_i2 size,
				u_i2 *nleft,
				u_i2 *save,
				char **saveptr,
				char *dest );

FUNC_EXTERN DB_STATUS scs_dctdesc(SCD_SCB *scb );

FUNC_EXTERN DB_STATUS scs_dccpmap(SCD_SCB *scb );

FUNC_EXTERN STATUS scs_dcinput(SCD_SCB *scb, GCA_RV_PARMS **it );

FUNC_EXTERN DB_STATUS scs_dcsend(SCD_SCB *scb,
				 GCA_RV_PARMS *it,
				 SCC_GCMSG **message,
				 bool copy );

FUNC_EXTERN bool scs_dcmsg(GCA_RV_PARMS *it );

FUNC_EXTERN DB_STATUS scs_dcinterrupt(SCD_SCB *scb, GCA_RV_PARMS **it );

FUNC_EXTERN DB_STATUS scs_dccont(SCD_SCB *scb,
				 i4  sdmode,
				 i4  msgtype,
				 PTR msg );

FUNC_EXTERN DB_STATUS scs_dcendcpy( SCD_SCB *scb );

FUNC_EXTERN DB_STATUS scs_dccnct( SCD_SCB *scb,
				 i4  *qry_status,
				 i4  *next_op,
				 i4  opcode );

FUNC_EXTERN DB_STATUS scs_dccopy(SCD_SCB *scb,
				 i4  *qry_status,
				 i4  *next_op,
				 i4  opcode );

FUNC_EXTERN DB_STATUS scs_dcexec(SCD_SCB *scb,
				 i4  *qry_status,
				 i4  *next_op,
				 i4  opcode );

FUNC_EXTERN DB_STATUS scs_dcxproc(SCD_SCB *scb, 
				  i4  *qry_status,
				  i4  *next_op,
				  i4  opcode );

FUNC_EXTERN DB_STATUS scs_declare(SCF_CB *scf_cb,
				  SCD_SCB *scb );

FUNC_EXTERN DB_STATUS scs_clear(SCF_CB *scf_cb,
				SCD_SCB *scb );

FUNC_EXTERN DB_STATUS scs_disable(SCF_CB *scf_cb,
				  SCD_SCB *scb );

FUNC_EXTERN DB_STATUS scs_enable(SCF_CB *scf_cb,
				 SCD_SCB *scb );

FUNC_EXTERN STATUS scs_attn(i4 eid, SCD_SCB *scb );

FUNC_EXTERN DB_STATUS scs_i_interpret(SCD_SCB *scb );

FUNC_EXTERN STATUS scs_fhandler( EX_ARGS *ex_args );

FUNC_EXTERN STATUS scs_format(SCD_SCB *scb,
			      char *stuff,
			      i4  powerful,
			      i4  suppress_sid );

FUNC_EXTERN STATUS scs_avformat(void);

FUNC_EXTERN i4  scs_facility(SCD_SCB *scb);

FUNC_EXTERN STATUS scs_iformat(SCD_SCB *scb,
			       i4  powerful,
			       i4  suppress_sid,
			       i4  err_output );

FUNC_EXTERN bool scs_chk_priv( char *user_name, char *priv_name );

FUNC_EXTERN DB_STATUS scs_set_priority(SCD_SCB *scb, i4  req_priority);
			
FUNC_EXTERN void scs_scan_scbs( SCS_SCAN_FUNC *func, PTR arg );

FUNC_EXTERN SCS_SES_OP_FUNC scs_remove_sess;

FUNC_EXTERN DB_STATUS scs_fire_db_alarms(SCD_SCB *scb, i4);

FUNC_EXTERN DB_STATUS scs_get_db_alarms(SCD_SCB *scb);

FUNC_EXTERN i4 scs_trimwhite( i4  len, char *buf );

FUNC_EXTERN DB_STATUS scs_check_session_role(SCD_SCB *scb, 
	DB_OWN_NAME *rolename, 
	DB_PASSWORD *rolepassword,
	i4	    *roleprivs,
	SCS_DBPRIV  **roledbpriv);

FUNC_EXTERN DB_STATUS scs_get_session_roles(SCD_SCB *scb);

FUNC_EXTERN DB_STATUS scs_check_password(
	SCD_SCB *scb, 
	char *passwd, 
	i4 pwlen, 
	bool do_alarms_audit);

FUNC_EXTERN DB_STATUS check_external_password (
        SCD_SCB *scb,
        DB_OWN_NAME *rolename,
        DB_PASSWORD *rolepassword,
	bool	    auth_role
);
FUNC_EXTERN VOID scs_cvt_dbpr_tuple ( SCD_SCB *scb,
	DB_PRIVILEGES *indbpriv,
	SCS_DBPRIV    *outdbpriv
);

FUNC_EXTERN DB_STATUS scs_propagate_privs(
	SCD_SCB *scb, 
	i4 privs
);

FUNC_EXTERN DB_STATUS scs_compress(
	GCA_TD_DATA 	*tdesc,
	PTR		local_row_buf,
	i4		row_count,
	i4		*total_dif
);
FUNC_EXTERN DB_STATUS scs_dbadduser(SCF_CB    *scf_cb,
				    SCD_SCB *scb );
FUNC_EXTERN DB_STATUS scs_dbdeluser(SCF_CB    *scf_cb,
				    SCD_SCB *scb );
FUNC_EXTERN STATUS scs_sampler( SCD_SCB* scb );
