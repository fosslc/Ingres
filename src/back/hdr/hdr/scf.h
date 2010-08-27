/*
** Copyright (c) 1985, 2005 Ingres Corporation
**
*/

/*
** Name: SCF.H - Control blocks used in SCF Utility Operations
**
** Description:
**      This file contains the type definitions and defined constants
**      necessary to call the system control facility.
**      Included in this category are such calls as requesting information
**      about the user, requesting information about the server,
**      and requesting resources from the (SCF).
**
**      A single control block is used to request these functions of SCF.
**
** History:
**      02-Dec-1985 (fred)
**          Created.
**      03-Mar-1986 (fred)
**          Upgraded to detailed design specs.  Added semaphore block
**	    and parameters for session suspension and resumption
**	 03-Jul-1986 (fred)
**	    Added standard control block header.  Unfortunately, this makes the
**	    SCF_CB too big (72 bytes).  Oh well.  Maybe I'll think of more stuff
**	    to put in it to fill it out to 96 bytes.
**       19-Jan-1987 (rogerk)
**	    Added four scf error numbers for copy sequencing errors.
**	 11-May-1988 (fred)
**	    Added new SCC_MESSAGE & SCC_WARNING to the known operations.
**	    SCC_MESSAGE is used to generate messages in database procedures,
**	    and SCC_WARNING can be used to send warning messages.
**	28-feb-1989 (rogerk)
**	    Changed SCF_SEMAPHORE definition to match new CS_SEMAPHORE struct.
**	    Added type and process id fields.
**	20-mar-1989 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    Added definition for SCU_XENCODE opcode
**	    Added scf_xpassword, scf_xpwdlen to SCF_CB.
**	    Added error message associated with group/applid's
**	24-mar-89 (paul)
**	    Added new stype in support of user defined error messages.
**	31-mar-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Added SCI_GROUP, SCI_APLID, SCI_DBPRIV, SCI_QROW_LIMIT,
**	    SCI_QDIO_LIMIT, SCI_QCPU_LIMIT, SCI_QPAGE_LIMIT, SCI_QCCOST_LIMIT.
**	04-may-89 (neil)
**	    Added new SCC_RSERROR to the known operations.  This is used when
**	    the RAISE ERROR user statement raises an error.  It's treated much
**	    like regular errors.  Removed "stype" change from 24-mar above as
**	    it zapped the error number in the union.
**	20-may-89 (ralph)
**	    Added scf_xpasskey field to SCF_CB.
**      25-may-89 (jennifer)
**          Added SCI_SEC_LABEL and SCI_RUSERNAME.
**	30-may-89 (ralph)
**	    GRANT Enhancements, Phase 2c:
**	    Added SCI_TAB_CREATE, SCI_PROC_CREATE, SCI_SET_LOCKMODE requests.
**	19-jun-89 (rogerk)
**	    Changed SCF_SEMAPHORE definition to be same as CS_SEMAPHORE.
**	    Added SCF_SEM_INTRNL definition for scf to do single-user
**	    semaphores.  Added scf_sngl_sem to scf_ptr_union for SCF to use
**	    for single user semaphores.
**	22-jun-89 (jrb)
**	    Added E_SC0313_NOT_LICENSED for Terminator I capability checking.
**	24-jun-89 (ralph)
**	    GRANT Enhancements, Phase 2e:
**	    Added SCI_MXIO, SCI_MXROW, SCI_MXPAGE, SCI_MXCOST, SCI_MXCPU,
**	    SCI_MXPRIV and SCI_IGNORE.
**	7-sep-89 (paul)
**	    Added definitions for Alerter operations and errors.
**	23-sep-89 (ralph)
**	    Added SCI_FLATTEN for /NOFLATTEN support.
**	10-oct-89 (ralph)
**	    Added SCI_OPF_MEM for /OPF=(MEMORY=n) support.
**	22-oct-89 (paul)
**	    Added new SCE error messages and date to SCF_ALERT.
**	05-jan-90 (andre)
**	    Defined SCI_FIPS to support /FIPS server startup option.
**	08-aug-90 (ralph)
**	    Added SCI_SECSTATE for dbmsinfo('secstate').
**	    Added SCI_UPSYSCAT for dbmsinfo('update_syscat')
**	    Added SCI_DB_ADMIM for dbmsinfo('db_admin')
**	    Removed SCI_OPF_MEM
**	    Added E_SC0302_AUDIT_INIT for audit initialization error.
**	09-sep-90 (ralph)
**	    6.3->6.5 merge:
**	    Added SCS_SET_ACTIVITY and SCS_CLR_ACTIVITY.
**	06-feb-91 (ralph)
**          Added SCI_SESSID for dbmsinfo('session_id')
**	04-feb-91 (neil)
**	    Added new SCE error messages.
**	01-jul-91 (andre)
**	    Added SCI_SESS_USER, SCI_SESS_GROUP, and SCI_SESS_ROLE
**	05-nov-91 (ralph)
**	    6.4->6.5 merge
**	    25-feb-91 (rogerk)
**		Added error defines for Set Nologging / Set Session attempts
**		while in an MST - E_SC031B_SETLG_XCT_INPROG,
**		E_SC031C_ONERR_XCT_INPROG.
**	    02-jul-91 (ralph)
**		Removed SCU_SFREE -- this is not referenced anywhere other than
**		in scf_call().
**		Moved definitions of the following constants from SCUSSVC.C to
**		this file:
**			    FREE_PHASE_1
**			    SET_SHARE_PHASE_1
**			    SET_EXCLUSIVE_1
**		I don't think these were used anywhere than in SCUSSVC.C, but
**		I've added them here just to be safe.  They are placed under
**		scse_checksum in the SCF_SEM_INTRNL structure, which I don't
**		think was used anywhere either.
**	    16-aug-91 (ralph)
**		Bug b39085.  Add message constants to support EV subsystem
**		detection of phantom servers, including:
**		    E_SC029B_XSEV_REQLOCK, E_SC029C_XSEV_RLSLOCK,
**		    E_SC029D_XSEV_PREVERR, E_SC029E_XSEV_TSTLOCK,
**		    E_SC029F_XSEV_EVCONNECT.
**		Make all function codes i4.
**	    01-jan-92 (fraser)
**		Define SCU_MPAGESIZE for PMFE
**	30-jan-1992 (bonobo)
**	    	Removed the redundant typedefs and changed questionable 
**	    	integration dates to quiesce the ANSI C compiler warnings.
**	11-dec-1991 (fpang)
**	    SYBIL merge.
**	    03-apr-1989 (georgeg) 
**		Added message E_SC0302, logged to the log file when server fails
**		to regidter with nameserver. 
**	    27-mar-1990 (georgeg)
**		Added error messages for 2pc thread.
**	    31-mar-1990 (carl)
**		Added sdc_b_remained, sdc_b_ptr and sdc_err to SCC_SDC_CB for 
**		byte-alignment fix.
**	    30-may-1990 (georgeg)
**		Added SCC_RSERROR to fix bug 20697.
**	    End of merged comments.
**	28-jul-92 (pholman)
**	    Added SCI_AUDIT_LOG to support new dbmsinfo request (C2).
**	18-aug-92 (pholman)
**	    Add E_SC0030_CHANGESECAUDIT_IN_XACT error message, to allow
**	    changes to the SECURITY_AUDIT profile to be modified within
**	    an MST (C2).  Also E_SC023C_SXF_ERROR for failures in the SXF.
**	06-oct-92 (robf)
**	    Added security info request, SCI_SECURITY.
**	02-nov-92 (jrb)
**	    Added E_SC0316 and E_SC0317 for multi-location sorts.
**	23-Nov-1992 (daveb)
**	    Add SCI_IMA_VNODE, SCI_IMA_SERVER, SCI_IMA_SESSION.
**	10-dec-1992 (pholman)
**	    Add new message IDs for use with loading PM information, and
**	    using C2 security auditing.
**	29-dec-1992 (robf)
**	    Add new message IDs for C2 security auditing.
**	17-jan-1993 (ralph)
**	    Added constants for new FIPS dbmsinfo requests
**	2-Jul-1993 (daveb)
**	    prototype scf_call();
**	27-jul-1993 (ralph)
**	    DELIM_IDENT:
**	    Add SCI_DBSERVICE to return the dbservice value
**      13-Aug-1993 (fred)
**          Added error messages about large objects and dbp's.
**      13-sep-1993 (stevet)
**          Added error message about loading timezone file.
**	17-dec-93 (rblumer)
**	    removed SCI_FIPS startup/dbmsinfo option.
**	31-jan-1994 (bryanp)
**	    Add E_SC0346_IIDBMS_PARAMETER_ERROR.
**	2-feb-1995 (stephenb)
**	    Add E_SC0031
**	4-Aug-1995 (jenjo02)
**	    Added SCD_ATTACH to list of scf_call() functions
**	    Added scf_apriority, scf_aparms, scf_aprocess,
**	    scf_asid to SCF_CB.
**	14-nov-1995 (nick)
**	    Add E_SC037B_BAD_DATE_CUTOFF
**	12-dec-1995 (nick)
**	    Add SCI_REAL_USER_CASE to SCF_SI. #71800
**	06-mar-1996 (nanpr01)
**	    Added SCI_MXRECLEN to return maximum tuple length for the
**	    installation. This is for variable page size project to support
**	    increased tuple size for star.
**	 23-may-96 (stephenb)
**	    Add SCU_DBDELUSER and SCU_DBADDUSER to support DBMS replication.
**	    Also add scf_dbname field to SCF_CB
**	10-oct-1996 (canor01)
**	    Make messages more generic.
**	30-jan-97 (stephenb)
**	    Add SCD_INITSNGL for single user init of SCF.
**      27-feb-1997 (stial01)
**          Added E_SC0380_SET_LOCKMODE_ROW
**	17-jun-1997 (wonst02)
** 	    Added E_SC0381_SETLG_READONLY_DB for readonly databases.
**	05-aug-1997 (canor01)
**	    Add SCU_SDESTROY to remove the semaphore.
**	23-sep-1997 (canor01)
**	    Preliminary changes for FRONTIER.
**      18-dec-1997 (stial01)
**          Added E_SC0132_LO_REPEAT_MISMATCH.
**	09-Mar-1998 (jenjo02)
**	    Moved SCS_S... session type defines from scs.h to here.
**	    Added SCS_SFACTOTUM thread type, SCF_FTC, SCF_FTX.
**	    Added SCS_MAXSESSTYPE define.
**	13-apr-98 (inkdo01)
**	    Dropped SCS_SSORT_THREAD in switch to factotum threads.
**	06-may-1998 (canor01)
**	    Add SCS_SLICENSE type.
**	14-May-1998 (jenjo02)
**	    Removed SCS_SWRITE_BEHIND thread type. They're now started
**	    as factotum threads.
**	    Added new factotum function prototypes for WriteBehind threads.
**	19-may-1998 (nanpr01)
**	    Added new parallel index factotum function prototype.
**	23-jul-1998 (rigka01)
**	    add SCE_NOORIG for use with RDF synchronization to avoid
**	    sending RDF synch events to the originator as the will
**	    and should be ignored; sending these events may require
**	    a greater increase in event_limit and affects performance.
**	    Also when all but 1 DBMS server is killed, the resources
**	    for the servers are not released until shutdown.  The
**  	    remaining server sends events to itself and in the case
**	    where event_limit has been reached, causes the logging
**	    of message E_RD018A each time the events are sent.
**	10-nov-1998 (sarjo01)
**	    Added SCI_TIMEOUT_ABORT for dbmsinfo('timeout_abort')   
**      15-Feb-1999 (fanra01)
**          Add preiodic cleanup thread for ice.
**      15-Feb-1999 (fanra01)
**          Add Tracing and logging thread for ice.
**	09-jun-1999 (somsa01)
**	    Added E_SC051D_LOAD_CONFIG.
**      10-jun-1999 (kinte01)
**         Change VAX to VMS as VAX should not be defined on AlphaVMS
**         systems. The definition of VAX on AlphaVMS systems causes
**         problems with the RMS GW (98236).
**	01-oct-1999 (somsa01)
**	    Added SCI_SPECIAL_UFLAGS, and the SCS_ICS_UFLAGS structure to
**	    enable the retrieval of ics_uflags from the SCS_ICS control
**	    block.
**	30-Nov-1999 (jenjo02)
**	    Added SCI_DISTRANID.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	14-feb-2001 (somsa01)
**	    For NT_IA64, the max page size is 8192.
**      06-apr-2001 (stial01)
**          Added SCI_PAGETYPE_V* for dbmsinfo('page_type_v*')
**      20-apr-2001 (stial01)
**          Added SCI_UNICODE for dbmsinfo('unicode')
**	10-may-2001 (devjo01)
**	    Add symbolic constants needed by s103715. (generic cluster)
**      08-may-2002 (stial01)
**          Added SCI_LP64 for dbmsinfo('lp64')
**	02-jun-2002 (devjo01)
**	    Added E_SC0276_CX_RAISE_FAIL.
**	30-Sep-2002 (hanch04)
**	    Added RAAT_NOT_SUPPORTED error message.
**      23-Oct-2002 (hanch04)
**          Make pagesize variables of type long (L)
**      10-jun-2003 (stial01)
**          Added SCI_QBUF, SCI_QLEN, SCI_TRACE_ERRNO
**      17-jun-2003 (stial01)
**          Added SCI_LAST_QBUF, SCI_LAST_QLEN
**	17-Dec-2003 (jenjo02)
**	    Added dmrAgent() Factotum function prototype.
**	28-Feb-2004 (jenjo02)
**	    Added Factotum prototypes for
**	    DMUSource, DMUTarget.
**      12-Apr-2004 (stial01)
**          Define scf_length as SIZE_TYPE.
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**	18-Oct-2004 (shaha03)
**	    SIR # 112918 Added SCI_CSRDEFMODE for configurable default cursor 
**	    open mode.
**      03-nov-2004 (thaju02)
**          Change scm_in/out_pages to SIZE_TYPE.
**      11-apr-2005 (stial01)
**          Added SCI_CSRLIMIT
**      05-May-2005 (horda03) Bug 114453/INGSRV 3290
**          Add message to indicate server control command being executed.
**	09-may-2005 (devjo01)
**	    Add SCS_SDISTDEADLOCK for distributed deadlock thread.
**	21-jun-2005 (devjo01)
**	    Add SCS_SGLCSUPPORT for GLC simulation.
**      21-feb-2007 (stial01)
**          Added SCI_PSQ_QBUF, SCI_PSQ_QLEN, renamed SCI_LAST_QBUF, SCI_LAST_QLEN
**	12-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with generic OSX.
**      14-jan-2009 (stial01)
**          Added support for dbmsinfo('pagetype_v5')
**	10-Mar-2009 (kiria01) SIR 121665
**	    Update GCA API to LEVEL 5
**	October 2009 (stephenb)
**	    Add defines for batch processing
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add support for dbmsinfo('pagetype_v6')
**	    dbmsinfo('pagetype_v7')
**	28-may-2010 (stephenb)
**	    Add message E_SC02A1_INCORRECT_QUERY_PARMS
**      12-Aug-2010 (horda03) b124109
**          Added SCI_PARENT_SCB to obtain a factotum thread's (ultimate) parent
**          SCI_SCB information.
*/

/*
**  Forward and/or External typedef/struct references.
*/

typedef struct _SCF_CB SCF_CB;

/*
**  Defines of other constants.
*/

/*
**	Session types:
*/
#define			SCS_SNORMAL	    0	    /* Normal user session */
#define			SCS_SMONITOR	    1	    /* Monitor session */
#define			SCS_SFAST_COMMIT    2	    /* Fast Commit Thread */
#define			SCS_SERVER_INIT     4	    /* Server initiator	    */
#define			SCS_SEVENT	    5	    /* EVENT service thread */
#define			SCS_S2PC	    6	    /* 2PC recovery thread */
#define			SCS_SRECOVERY	    7	    /* Recovery Thread */
#define			SCS_SLOGWRITER	    8	    /* logfile writer thread */
#define			SCS_SCHECK_DEAD	    9	    /* Check-dead thread */
#define			SCS_SGROUP_COMMIT   10	    /* group commit thread */
#define			SCS_SFORCE_ABORT    11	    /* force abort thread */
#define			SCS_SAUDIT	    12	    /* audit thread */
#define			SCS_SCP_TIMER	    13	    /* consistency pt timer */
#define			SCS_SCHECK_TERM	    14	    /* terminator thread */
#define			SCS_SSECAUD_WRITER  15	    /* security audit writer */
#define			SCS_SWRITE_ALONG    16      /* Write Along thread */
#define			SCS_SREAD_AHEAD     17      /* Read Ahead thread */
#define			SCS_REP_QMAN	    18	    /* replicator queue man */
#define                 SCS_SLK_CALLBACK    19      /* LK callback thread */
#define			SCS_SDEADLOCK	    20	    /* Deadlock Detector thread */
#define			SCS_SSAMPLER	    21	    /* Sampler thread */
#define			SCS_SFACTOTUM       23	    /* Factotum thread */
#define			SCS_PERIODIC_CLEAN  25	    /* ice cleanup thread */
#define			SCS_TRACE_LOG       26	    /* ice tracing thread */
#define			SCS_CSP_MAIN	    27	    /* main CSP thread */
#define			SCS_CSP_MSGMON      28	    /* CSP msg monitor thread */
#define			SCS_CSP_MSGTHR      29	    /* CSP message thread */
#define			SCS_SDISTDEADLOCK   30	    /* Dist. Deadlock thread */
#define			SCS_SGLCSUPPORT     31	    /* GLC support thread */

#define			SCS_MAXSESSTYPE	    31	    /* Highest session type */

/*
**      The following values are the function codes passed to 
**      SCF to request action from it.  They are passed as
**      the first argument to scf_call().
*/
#define			SCF_MIN_OPERATION   0x1L
	    /*
	    ** Informational operations
	    */
#define                 SCU_INFORMATION 0x1L
#define			SCU_IDEFINE	0x2L
	    /*
	    ** Memory operations
	    */
#define                 SCU_MALLOC      0x10L
#define                 SCU_MFREE       0x11L
	    /*
	    ** Semaphore operations
	    */
#define                 SCU_SINIT       0x20L
#define                 SCU_SWAIT       0x21L
#define                 SCU_SRELEASE    0x22L
#define 		SCU_SDESTROY    0x23L
	    /*
	    ** Declaration of asynchronous functions
	    */
#define                 SCS_DECLARE     0x30L
#define                 SCS_CLEAR       0x31L
	    /* the following are added in the scf_function field */
#define			SCS_AIC_MASK	0x10000L    	/* dcl or clr -ing an AIC */
#define			SCS_PAINE_MASK	0x20000L    	/* dcl or clr -ing a PAINE */
#define			SCS_ALWAYS_MASK	0x40000L	/* deliver aic regardless of
							   facility activation */
#define			SCS_DISABLE	0x32L		/* disable aic's temporarily */
#define			SCS_ENABLE	0x33L		/* reenable */
						/* NOTE: only aic's can be disabled */
	    /*
	    ** Communications operations
	    */
#define                 SCC_ERROR       0x40L	/* send an error to the user */
#define			SCC_TRACE	0x41L	/* send trace message */

	    /* these operations are for DDB support and will not be in phase I */

#define                 SCC_RCONNECT    0x42L	/* open connection w/ rmt dbms */
#define                 SCC_RACCEPT     0x43L	/* accept same (done by rmt be ) */
#define                 SCC_RDISCONNECT 0x44L	/* terminate the disconnection */
#define                 SCC_RWRITE      0x45L	/* send info to remote be */
#define                 SCC_RREAD       0x46L	/* read info from rmt be */
#define                 SCC_MESSAGE     0x47L	/* Send a user message to FE */
#define                 SCC_WARNING     0x48L	/* Send a warning to the user */
#define                 SCC_RSERROR	0x49L	/* RAISE ERROR - user error */

            /*
            ** Allow setting and clearing of activity.  This is
            ** used by facilities if the wish to declare that
            ** certain specially monitorable activities are taking
            ** place.  This is used in cases where it is desirable
            ** to report the progress of aborts, optimization, etc.
            */
#define                 SCS_SET_ACTIVITY 0x4AL
#define                 SCS_CLR_ACTIVITY 0x4BL

	    /*
	    ** miscellaneous operations 
	    */
#define			SCD_INITSNGL	0x4DL	/* initialize SCF for single
						** user servers */
#define			SCU_DBDELUSER	0x4EL	/* opposite of below */
#define			SCU_DBADDUSER	0x4FL	/* add to no of db users in
						** dbcb */
#define			SCU_XENCODE	0x50L	/* encode for passwords */
	    /*
	    ** operations on alerters
	    */
#define			SCE_REGISTER	0x51L	/* Register to receive alert */
#define			SCE_REMOVE	0x52L	/* Remove alert registration */
#define			SCE_RAISE	0x53L	/* Raise an Alert */
#define			SCE_RESIGNAL	0x55L   /* Propagate an alert. */

	    /*
	    ** operations for distributed.
	    */
#define			SCC_RELAY	0x54L	/* relay LDB error */

	    /*
	    ** operations for Factotum threads.
	    */
#define			SCS_ATFACTOT	0x60L	/* Create a Factotum thread */

#define		    SCF_MAX_OPERATION	0x60L

/*
**      These are parameters used to communicate with the dispatcher module
**      at the CL level.  They are passed as a mask in the "option_flags"
**	parameter to scd_suspend().
*/
#define                 SCD_INTERRUPTABLE_MASK	0x01
#define                 SCD_TIMEOUT_MASK	0x02

/*
**  SCF error values.
**
**  These values are returned by SCF to clients.  Those errors which exist
**  with the most significant byte = 0 are 'normal;' other errors are primarily
**  meant as internal to SCF errors which will be logged and translated before
**  escaping to the client level.
*/
#define			E_SC_OK				E_DB_OK
		/* all went as planned, else ... */
#define			E_SC_MASK			(0x090000L)
#define                 E_SC0001_OP_CODE		(E_SC_MASK + 0x0001L)
		/* op code to scf_call() was bad */
#define                 E_SC0002_BAD_SEMAPHORE		(E_SC_MASK + 0x0002L)
		/* sem not initialized or has been corrupted */
#define                 E_SC0003_INVALID_SEMAPHORE	(E_SC_MASK + 0x0003L)
		/* indicated semaphore cannot be freed or waited on */
#define			E_SC0004_NO_MORE_MEMORY		(E_SC_MASK + 0x0004L)
		/* no more available to the server -- probably fatal */
#define                 E_SC0005_LESS_THAN_REQUESTED	(E_SC_MASK + 0x0005L)
		/* less memory returned than requested */
#define                 E_SC0006_NOT_ALLOCATED		(E_SC_MASK + 0x0006L)
		/* memory not allocated -- cannot free */
#define                 E_SC0007_INFORMATION		(E_SC_MASK + 0x0007L)
		/* invalid item - which one in data */
#define                 E_SC0008_INFO_TRUNCATED		(E_SC_MASK + 0x0008L)
		/* information did not fit in requested area - item in data */
#define			E_SC0009_BUFFER			(E_SC_MASK + 0x0009L)
		/*
		** buffer provided for information is not addressable -
		**	 item in data
		*/
#define			E_SC000A_NO_BUFFER		(E_SC_MASK + 0x000AL)
		/* request for information but no place to put it */
#define			E_SC000B_NO_REQUEST		(E_SC_MASK + 0x000BL)
		/* ptr to/portion of SCI blk not addressable - item in data */
#define			E_SC000C_BUFFER_LENGTH  	(E_SC_MASK + 0x000CL)
		/* ( 0 < scf_length <= DB_ERR_SIZE ) != TRUE */
#define			E_SC000D_BUFFER_ADDR		(E_SC_MASK + 0x000DL)
		/* specified scf_buffer address not accessable */
#define			E_SC000E_AFCN_ADDR		(E_SC_MASK + 0x000EL)
		/* aic/paine address provided is not accessable */
#define			E_SC000F_IN_USE  		(E_SC_MASK + 0x000FL)
		/* memory/semaphore not freed by allocator */
#define			E_SC0010_NO_PERM		(E_SC_MASK + 0x0010L)
		/* facility not authorized to use SCS_ALWAYS_MASK */
#define			E_SC0011_NESTED_DISABLE		(E_SC_MASK + 0x0011L)
		/* AIC's are already disabled */
#define                 E_SC0012_NOT_SET                (E_SC_MASK + 0x0012L)
		/* cannot release a semaphore which is not set */
#define			E_SC0013_TIMEOUT		(E_SC_MASK + 0x0013L)
		/* out of time before operation completed.  Operation cancellation
			is up to the user. */
#define			E_SC0014_INTERRUPTED		(E_SC_MASK + 0x0014L)
		/* user interrupt while awaiting completion.  Operation
			cancellation is up to the user */

#define			E_SC0015_BAD_PAGE_COUNT		(E_SC_MASK + 0x0015L)
		/* invalid (<= 0) number of pages requested of sc0m or scu_m */
#define			E_SC0016_BAD_PAGE_ADDRESS	(E_SC_MASK + 0x0016L)
		/* page address is not page aligned */
#define			E_SC0017_BAD_SESSION		(E_SC_MASK + 0x0017L)
		/* session id is invalid */
#define			E_SC0018_BAD_FACILITY		(E_SC_MASK + 0x0018L)
		/* facility id is invalid */
#define			E_SC0019_BAD_FUNCTION_CODE	(E_SC_MASK + 0x0019L)
		/* function code was invalid */
#define			E_SC0020_WRONG_PAGE_COUNT	(E_SC_MASK + 0x0020L)
		/* page count to be freed != allocated quantity */
#define			E_SC0021_BAD_ADDRESS		(E_SC_MASK + 0x0021L)
		/* address to be freed != address allocated */
#define			E_SC0022_BAD_REQUEST_BLOCK	(E_SC_MASK + 0x0022L)
		/* scf_length != SCF_CB_SIZE or scf_type != SCF_CB_TYPE) */
#define			E_SC0023_INTERNAL_MEMORY_ERROR	(E_SC_MASK + 0x0023L)
		/* scf memory arena has been corrupted -- FATAL */
#define			E_SC0024_INTERNAL_ERROR		(E_SC_MASK + 0x0024L)
		/* internal problem of undetermined sort -- FATAL */
#define			E_SC0025_NOT_YET_AVAILABLE	(E_SC_MASK + 0x0025L)
		/* function requested has not yet been implemented.
		** Contact the development team for scheduling information */
#define			E_SC0026_SCB_ALLOCATE		(E_SC_MASK + 0x0026L)
		/* unable to allocate SCB */
#define			E_SC0027_BUFFER_LENGTH		(E_SC_MASK + 0x0027L)
		/* invalid buffer length on SCU_INFORMATION call */
#define			E_SC0028_SEMAPHORE_DUPLICATE	(E_SC_MASK + 0x0028L)
		/* you are requesting a semaphore you already have */
#define			E_SC0029_HOLDING_SEMAPHORE	(E_SC_MASK + 0x0029L)
		/* illegal to suspend while holding a semaphore */
#define			E_SC002A_NOT_DISABLED		(E_SC_MASK + 0x002AL)
		/* warning: aic's not disabled */
#define			E_SC002B_EVENT_REGISTERED	(E_SC_MASK + 0x002BL)
		/* Attempt to register an event that is already registered */
#define			E_SC002C_EVENT_NOT_REGISTERED	(E_SC_MASK + 0x002CL)
		/* Attempt to remove a registration that is not registered */
#define			E_SC002D_SET_SESS_AUTH_IN_XACT	(E_SC_MASK + 0x002DL)
		/* SET SESSION AUTHORIZATION was issued inside an MST */
#define                 E_SC002E_SETROLE_IN_XACT	(E_SC_MASK + 0x002EL)
		/* SET ROLE was issued inside an MST */
#define                 E_SC002F_SETGROUP_IN_XACT	(E_SC_MASK + 0x002FL)
		/* SET GROUP was issued inside an MST */
#define                 E_SC0030_CHANGESECAUDIT_IN_XACT	(E_SC_MASK + 0x0030L)
		/* ENABLE/DISABLE SECURITY_AUDIT  was issued inside an MST */
#define			E_SC0031_SET_SESS_AUTH_ID_INVALID (E_SC_MASK + 0x0031L)

/*
**		Internal Errors
**      These errors are E_DB_FATAL if returned to the outside
*/
#define			E_SC0100_MULTIPLE_MEM_INIT	    (E_SC_MASK + 0x100L)
		/* attempt to initialize memory more than once */
#define			E_SC0101_NO_MEM_INIT		    (E_SC_MASK + 0x101L)
		/* attempt to allocate memory before initializing it */
#define			E_SC0102_MEM_CORRUPT		    (E_SC_MASK + 0x102L)
		/* SCF memory arena has been corrupted */
#define			E_SC0103_MEM_PROTECTION		    (E_SC_MASK + 0x103L)
		/* error changing memory protections */
#define			E_SC0104_CORRUPT_POOL		    (E_SC_MASK + 0x104L)
		/* found corrupted pool header block */
#define			E_SC0105_CORRUPT_FREE		    (E_SC_MASK + 0x105L)
		/* found corrupted free block */
#define			E_SC0106_BAD_SIZE_REDUCE	    (E_SC_MASK + 0x106L)
		/* error deleting virtual pages */
#define			E_SC0107_BAD_SIZE_EXPAND	    (E_SC_MASK + 0x107L)
		/* error expanding process size */
#define			E_SC0108_UNEXPECTED_EXCEPTION	    (E_SC_MASK + 0x108L)
		/* unexpected exception occured during scf_call() op */
#define			E_SC0109_MULTI_DB_ADD		    (E_SC_MASK + 0x109L)
		/* attempt to add an added database -- operation continues */
#define			E_SC010A_RETRY			    (E_SC_MASK + 0x10AL)
		/* try operation again */
#define			E_SC010B_SCB_DBCB_LINK		    (E_SC_MASK + 0x10BL)
		/* dbcb linked to scb is incorrect -- FATAL */
#define			E_SC010C_DB_ADD			    (E_SC_MASK + 0x10CL)
		/* unable to add database */
#define			E_SC010D_DB_LOCATION		    (E_SC_MASK + 0x10DL)
		/* location listing */
#define			E_SC010E_DB_DELETE		    (E_SC_MASK + 0x10EL)
		/* unable to delete database */
#define			E_SC010F_INCOMPLETE_DB_DELETE	    (E_SC_MASK + 0x10FL)
		/* database delete incomplete */
#define			E_SC0120_DBLIST_ERROR		    (E_SC_MASK + 0x120L)
#define			E_SC0121_DB_OPEN		    (E_SC_MASK + 0x121L)
#define			E_SC0122_DB_CLOSE		    (E_SC_MASK + 0x122L)
#define			E_SC0123_SESSION_INITIATE	    (E_SC_MASK + 0x123L)
#define			E_SC0124_SERVER_INITIATE	    (E_SC_MASK + 0x124L)
#define			E_SC0125_POOL_ADDITION		    (E_SC_MASK + 0x125L)
#define			E_SC0126_SC0M_CHECK		    (E_SC_MASK + 0x126L)
#define			E_SC0127_SERVER_TERMINATE	    (E_SC_MASK + 0x127L)
#define			E_SC0128_SERVER_DOWN		    (E_SC_MASK + 0x128L)
#define			E_SC0129_SERVER_UP		    (E_SC_MASK + 0x129L)
#define			E_SC012A_BAD_MESSAGE_FOUND	    (E_SC_MASK + 0x12AL)
#define			E_SC012B_SESSION_TERMINATE	    (E_SC_MASK + 0x12BL)
#define			E_SC012C_ADF_ERROR		    (E_SC_MASK + 0x12CL)
#define			E_SC012D_DB_NOT_FOUND		    (E_SC_MASK + 0x12DL)
#define			E_SC012F_RETRY			    (E_SC_MASK + 0x12FL)
#define                 E_SC0130_LO_DBP_MISMATCH            (E_SC_MASK + 0x130L)
                 /* Large objects cannot be DBP parms */
#define                 E_SC0131_LO_STAR_MISMATCH           (E_SC_MASK + 0x131L)
                 /* Large objects cannot be parms to STAR*/
#define                 E_SC0132_LO_REPEAT_MISMATCH         (E_SC_MASK + 0x132L)
		 /* Large objects cannot be REPEAT query host vars */
#define			E_SC0200_SEM_INIT		    (E_SC_MASK + 0x200L)
#define			E_SC0201_SEM_WAIT		    (E_SC_MASK + 0x201L)
#define			E_SC0202_SEM_RELEASE		    (E_SC_MASK + 0x202L)
#define			E_SC0203_SEM_FREE		    (E_SC_MASK + 0x203L)
#define			E_SC0204_MEMORY_ALLOC		    (E_SC_MASK + 0x204L)
#define			E_SC0205_BAD_SCS_OPERATION	    (E_SC_MASK + 0x205L)
#define			E_SC0206_CANNOT_PROCESS		    (E_SC_MASK + 0x206L)
#define			E_SC0207_UNEXPECTED_ERROR	    (E_SC_MASK + 0x207L)
#define			E_SC0208_BAD_PSF_HANDLE		    (E_SC_MASK + 0x208L)
#define			E_SC0209_PSF_REQ_DISTRIBUTED	    (E_SC_MASK + 0x209L)
#define			E_SC020A_UNRECOGNIZED_QMODE	    (E_SC_MASK + 0x20AL)
#define			E_SC020B_UNRECOGNIZED_TEXT	    (E_SC_MASK + 0x20BL)
#define			E_SC020C_INPUT_OUT_OF_SEQUENCE	    (E_SC_MASK + 0x20CL)
#define			E_SC020D_NO_DISTRIBUTED		    (E_SC_MASK + 0x20DL)
#define			E_SC020E_MULTI_CURSOR_OPEN	    (E_SC_MASK + 0x20EL)
#define			E_SC020F_LOST_LOCK		    (E_SC_MASK + 0x20FL)
#define			E_SC0210_SCS_SQNCR_ERROR	    (E_SC_MASK + 0x210L)
#define			E_SC0211_CLF_ERROR		    (E_SC_MASK + 0x211L)
#define			E_SC0212_ADF_ERROR		    (E_SC_MASK + 0x212L)
#define			E_SC0213_DMF_ERROR		    (E_SC_MASK + 0x213L)
#define			E_SC0214_OPF_ERROR		    (E_SC_MASK + 0x214L)
#define			E_SC0215_PSF_ERROR		    (E_SC_MASK + 0x215L)
#define			E_SC0216_QEF_ERROR		    (E_SC_MASK + 0x216L)
#define			E_SC0217_QSF_ERROR		    (E_SC_MASK + 0x217L)
#define			E_SC0218_RDF_ERROR		    (E_SC_MASK + 0x218L)
#define			E_SC0219_SCF_ERROR		    (E_SC_MASK + 0x219L)
#define			E_SC021A_ULF_ERROR		    (E_SC_MASK + 0x21AL)
#define			E_SC021B_BAD_COPY_STATE		    (E_SC_MASK + 0x21BL)
#define			E_SC021C_SCS_INPUT_ERROR	    (E_SC_MASK + 0x21CL)
#define			E_SC021D_TOO_MANY_CURSORS	    (E_SC_MASK + 0x21DL)
#define			E_SC021E_CURSOR_NOT_FOUND	    (E_SC_MASK + 0x21EL)
#define			E_SC021F_BAD_SQNCR_CALL		    (E_SC_MASK + 0x21FL)
#define			E_SC0220_SESSION_ERROR_MAX	    (E_SC_MASK + 0x220L)
#define			E_SC0221_SERVER_ERROR_MAX	    (E_SC_MASK + 0x221L)
#define			E_SC0222_BAD_SCDNOTE_STATUS	    (E_SC_MASK + 0x222L)
#define			E_SC0223_BAD_SCDNOTE_FACILITY	    (E_SC_MASK + 0x223L)
#define			E_SC0224_MEMORY_FREE		    (E_SC_MASK + 0x224L)
#define			E_SC0225_BAD_GCA_INFO		    (E_SC_MASK + 0x225L)
#define			E_SC0226_BAD_GCA_TYPE		    (E_SC_MASK + 0x226L)
#define			E_SC0227_MEM_NOT_FREE		    (E_SC_MASK + 0x227L)
#define			E_SC0228_INVALID_TRACE		    (E_SC_MASK + 0x228L)
#define			E_SC0229_INVALID_ATTN_CALL	    (E_SC_MASK + 0x229L)
#define			E_SC022A_SERVER_LOAD		    (E_SC_MASK + 0x22AL)
#define			E_SC022B_PROTOCOL_VIOLATION	    (E_SC_MASK + 0x22BL)
#define			E_SC023C_SXF_ERROR		    (E_SC_MASK + 0x23CL)
#define                 E_SC023D_SXF_BAD_FLUSH              (E_SC_MASK + 0x23DL)
#define                 E_SC023E_SXF_BAD_WRITE              (E_SC_MASK + 0x23EL)
#define			E_SC023F_QRYTEXT_WRITE		    (E_SC_MASK + 0x23FL)

#define			E_SC0260_XENCODE_ERROR		    (E_SC_MASK + 0x260L)
#define			E_SC0261_XENCODE_BAD_PARM    	    (E_SC_MASK + 0x261L)
#define			E_SC0262_XENCODE_BAD_RESULT	    (E_SC_MASK + 0x262L)

/*
** GCA_REGISTER failed.
*/
#define			E_SC0245_CANNOT_REGISTER	    (E_SC_MASK + 0x245L)

/*
** Copy Errors
*/
#define			E_SC0250_COPY_OUT_OF_SEQUENCE	    (E_SC_MASK + 0x250L)
#define			E_SC0251_COPY_CLOSE_ERROR	    (E_SC_MASK + 0x251L)
#define			E_SC0252_COPY_SYNC_ERROR	    (E_SC_MASK + 0x252L)
#define			E_SC0253_BAD_COPY_ERRMSG	    (E_SC_MASK + 0x252L)

/*
**  Alert handling internal errors (270-29F)
*/
#define			E_SC0270_NO_EVENT_MESSAGE	    (E_SC_MASK + 0x270L)
#define			E_SC0271_EVENT_THREAD		    (E_SC_MASK + 0x271L)
#define			E_SC0272_EVENT_THREAD_ADD	    (E_SC_MASK + 0x272L)
#define			E_SC0273_EVENT_RETRY		    (E_SC_MASK + 0x273L)
#define			E_SC0274_NOTIFY_EVENT		    (E_SC_MASK + 0x274L)
#define			E_SC0275_IGNORE_ASTATE		    (E_SC_MASK + 0x275L)
#define			E_SC0276_CX_RAISE_FAIL		    (E_SC_MASK + 0x276L)
#define			E_SC0280_NO_ALERT_INIT		    (E_SC_MASK + 0x280L)
#define			E_SC0281_ALERT_ALLOCATE_FAIL	    (E_SC_MASK + 0x281L)
#define			E_SC0282_RSES_ALLOCATE_FAIL	    (E_SC_MASK + 0x282L)
#define			E_SC0283_AINST_ALLOCATE_FAIL	    (E_SC_MASK + 0x283L)
#define			E_SC0284_BUCKET_INIT_ERROR	    (E_SC_MASK + 0x284L)
#define			E_SC0285_EVENT_MEMORY_ALLOC	    (E_SC_MASK + 0x285L)
#define			E_SC0286_EVENT_SEM_INIT		    (E_SC_MASK + 0x286L)
#define			E_SC0287_AINST_SESSION_ID	    (E_SC_MASK + 0x287L)
#define			E_SC0288_XSEV_SM_DESTROY	    (E_SC_MASK + 0x288L)
#define			E_SC0289_XSEV_ALLOC		    (E_SC_MASK + 0x289L)
#define			E_SC028A_XSEV_CRLOCKLIST	    (E_SC_MASK + 0x28AL)
#define			E_SC028B_XSEV_ATTACH		    (E_SC_MASK + 0x28BL)
#define			E_SC028C_XSEV_EALLOCATE		    (E_SC_MASK + 0x28CL)
#define			E_SC028D_XSEV_REGISTERED	    (E_SC_MASK + 0x28DL)
#define			E_SC028E_XSEV_NOT_REGISTERED	    (E_SC_MASK + 0x28EL)
#define			E_SC028F_XSEV_NO_SERVER		    (E_SC_MASK + 0x28FL)
#define			E_SC0290_XSEV_SIGNAL		    (E_SC_MASK + 0x290L)
#define			E_SC0291_XSEV_NOT_CONNECT	    (E_SC_MASK + 0x291L)
#define			E_SC0292_XSEV_SEM_INIT		    (E_SC_MASK + 0x292L)
#define			E_SC0293_XSEV_VERSION		    (E_SC_MASK + 0x293L)

#define                 E_SC029B_XSEV_REQLOCK               (E_SC_MASK + 0x29BL)
#define                 E_SC029C_XSEV_RLSLOCK               (E_SC_MASK + 0x29CL)
#define                 E_SC029D_XSEV_PREVERR               (E_SC_MASK + 0x29DL)
#define                 E_SC029E_XSEV_TSTLOCK               (E_SC_MASK + 0x29EL)
#define                 E_SC029F_XSEV_EVCONNECT             (E_SC_MASK + 0x29FL)


#define			E_SC02A0_NO_QUERY_TEXT		    (E_SC_MASK + 0x2A0L)
#define			E_SC02A1_INCORRECT_QUERY_PARMS	    (E_SC_MASK + 0x2A1L)

/*
**  SCF errors related to start or stop of cluster threads.
*/
#define                 E_SC02C0_CSP_MAIN_ADD		    (E_SC_MASK + 0x2C0L)
#define                 E_SC02C1_CSP_MAIN_EXIT		    (E_SC_MASK + 0x2C1L)
#define                 E_SC02C2_CSP_MSGMON_ADD		    (E_SC_MASK + 0x2C2L)
#define                 E_SC02C3_CSP_MSGMON_EXIT	    (E_SC_MASK + 0x2C3L)
#define                 E_SC02C4_CSP_MSGTHR_ADD		    (E_SC_MASK + 0x2C4L)
#define                 E_SC02C5_CSP_MSGTHR_EXIT	    (E_SC_MASK + 0x2C5L)

/*
**  SCF errors related to NUMA clusters.
*/
#define                 E_SC02C6_BAD_RAD_PARAM              (E_SC_MASK + 0x2C6L)

/*
**  General SCF produced User errors
*/
#define                 E_SC0300_EXDB_LOCK_REQ		    (E_SC_MASK + 0x300L)
#define			E_SC0301_LOC_EXISTS		    (E_SC_MASK + 0x301L)
#define			E_SC0302_AUDIT_INIT		    (E_SC_MASK + 0x302L)
#define			E_SC0313_NOT_LICENSED		    (E_SC_MASK + 0x313L)
#define			E_SC0316_LOC_NOT_FOUND		    (E_SC_MASK + 0x316L)
#define			E_SC0317_LOC_NOT_AUTHORIZED	    (E_SC_MASK + 0x317L)
#define                 E_SC031B_SETLG_XCT_INPROG           (E_SC_MASK + 0x31BL)
#define                 E_SC031C_ONERR_XCT_INPROG           (E_SC_MASK + 0x31CL)

#define                 E_SC0329_AUDIT_THREAD_ADD           (E_SC_MASK + 0x329L)
#define                 E_SC032A_AUDIT_THREAD_EXIT          (E_SC_MASK + 0x32AL)
 
/*
** Problem with PMload of Parameter Management data files
*/
# define		E_SC032B_BAD_PM_FILE	    	    (E_SC_MASK + 0x32BL)
# define		E_SC032C_NO_PM_FILE                 (E_SC_MASK + 0x32CL)
/*
** C2 Informational Messages
*/
# define		E_SC032D_C2_AUDITING_ON		    (E_SC_MASK + 0x32DL)
# define		E_SC032E_C2_NEEDS_KME               (E_SC_MASK + 0x32EL)

# define	        E_SC0331_C2_INVALID_MODE            (E_SC_MASK + 0x331L)

/*
** Problem with loading timezone file
*/
# define	        E_SC033D_TZNAME_INVALID             (E_SC_MASK + 0x33DL)

/*
** Invalid II_DATE_CENTURY_BOUNDARY 
*/
# define		E_SC037B_BAD_DATE_CUTOFF            (E_SC_MASK + 0x37BL)

/*
**  Indicator flags
*/
#define			E_SC1000_PROCESSED		    (E_SC_MASK + 0x1000L)

/*
**  INGRES/STAR error messages
*/
/*
**  STAR errors prodused by the DX recovery thread.
*/
/*
** Failed to add a thread to recover DX transactions, the server will die.
*/
#define			E_SC0314_RECOVER_THREAD_ADD 	    (E_SC_MASK + 0x314L)
/*
** The STAR DX recovery thread is still active, server 
** is not open for businness yet
*/
#define			E_SC0315_RECOVERING_ORPHAN_DX	    (E_SC_MASK + 0x315L)

/*
** The first parm to iidbms.exe wasn't dbms,rms,star, or recovery:
*/
#define			E_SC0346_DBMS_PARAMETER_ERROR	    (E_SC_MASK + 0x346L)
/*
**  STAR errors generated by extracting GCA_TD_DATA/GCA_CP_MAP from the 
**  downstream LDB association.
**
**  History
**	03-apr-90 (carl)
**	    created
*/

/*
** Failed to extract a column descriptor
*/

#define			E_SC0350_COL_DESC_READ	    (E_SC_MASK + 0x350L)

/*
** Failed to extract a tuple descriptor
*/
#define			E_SC0351_TUP_DESC_READ	    (E_SC_MASK + 0x351L)

/*
** Failed to extract a copy map
*/
#define			E_SC0352_COPY_MAP_READ	    (E_SC_MASK + 0x352L)

/*
** Failed to fetch data from the LDB
*/
#define			E_SC0353_LDB_FETCH	    (E_SC_MASK + 0x353L)

/*
** errors generated by use of the RAAT interface
*/
#define			E_SC0371_NO_TX_FLAG	    (E_SC_MASK + 0x371L)
#define			E_SC0372_NO_MESSAGE_AREA    (E_SC_MASK + 0x372L)
#define			E_SC0373_PSF_RESOLVE_ERROR  (E_SC_MASK + 0x373L)
#define			W_SC0374_NO_ACCESS_TO_TABLE (E_SC_MASK + 0x374L)
#define			W_SC0375_INVALID_LOCK_MODE  (E_SC_MASK + 0x375L)
#define			E_SC0376_INVALID_GET_FLAG   (E_SC_MASK + 0x376L)
#define			E_SC0377_NULL_RECORD_ID	    (E_SC_MASK + 0x377L)
#define			E_SC0378_NO_KEY_VALUES	    (E_SC_MASK + 0x378L)
#define			E_SC0379_INVALID_KEYSIZE    (E_SC_MASK + 0x379L)
#define			E_SC037A_INVALID_OPCODE     (E_SC_MASK + 0x37AL)

/*
** More General SCF produced User errors
*/
#define                 E_SC0380_SET_LOCKMODE_ROW   (E_SC_MASK + 0x380L)
#define			E_SC0381_SETLG_READONLY_DB  (E_SC_MASK + 0x381L)

#define			E_SC0393_RAAT_NOT_SUPPORTED (E_SC_MASK + 0x393L)
#define			E_SC0394_CLUSTER_NOT_SUPPORTED (E_SC_MASK + 0x394L)

#define			E_SC0500_INVALID_LANGCODE   (E_SC_MASK + 0x500L)
#define			E_SC0501_INTERNAL_ERROR     (E_SC_MASK + 0x501L)
#define			E_SC0502_GCA_ERROR          (E_SC_MASK + 0x502L)
/* other Jasmine-specific messages here */
#define			E_SC0519_CONN_LIMIT_EXCEEDED (E_SC_MASK + 0x519L)

#define			E_SC051D_LOAD_CONFIG	    (E_SC_MASK + 0x51DL)

#define			I_SC051E_IIMONITOR_CMD      (E_SC_MASK + 0x51EL)


/*}
** Name: SCF_SESSION - Session Identifier
**
** Description:
**	This is the definition for the identifier used within the system
**	to identify a session
**
** History:
**      14-mar-1986 (fred)
**          Created on Jupiter
**	 7-sep-93 (swm)
**	    Changed definition of SCF_SESSION from i4 to CS_SID to
**	    reflect CL interface change.
*/
#define	SCF_SESSION	CS_SID

/*}
** Name: SCF_SCI - Information Request Block
**
** Description:
**      This structure is used to request information from SCF.  An array of
**      these structures is passed to the scf_call() routine with an operation
**      code of SCU_INFORMATION to request information about the session or
**      server under which a facility is running.  
**
** History:
**       3-dec-1985 (fred)
**          Created.
**	24-oct-1988 (rogerk)
**	    Added SCI_DBCPUTIME, SCI_DBDIOCNT, SCI_DBBIOCNT.
**	31-mar-1989 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Added SCI_GROUP, SCI_APLID, SCI_DBPRIV, SCI_QROW_LIMIT,
**	    SCI_QDIO_LIMIT, SCI_QCPU_LIMIT, SCI_QPAGE_LIMIT, SCI_QCCOST_LIMIT.
**      25-may-89 (jennifer)
**          Added SCI_SEC_LABEL and SCI_RUSERNAME.
**	30-may-89 (ralph)
**	    GRANT Enhancements, Phase 2c:
**	    Added SCI_TAB_CREATE, SCI_PROC_CREATE, SCI_SET_LOCKMODE requests.
**	24-jun-89 (ralph)
**	    GRANT Enhancements, Phase 2e:
**	    Added SCI_MXIO, SCI_MXROW, SCI_MXPAGE, SCI_MXCOST, SCI_MXCPU,
**	    SCI_MXPRIV, and SCI_IGNORE.
**	23-sep-89 (ralph)
**	    Added SCI_FLATTEN for /NOFLATTEN support.
**	10-oct-89 (ralph)
**	    Added SCI_OPF_MEM for /OPF=(MEMORY=n) support.
**	05-dec-89 (teg)
**	    Added sci_code SCI_SV_CB.
**	05-jan-90 (andre)
**	    Defined SCI_FIPS to support /FIPS server startup option.
**	06-feb-91 (ralph)
**          Added SCI_SESSID for dbmsinfo('session_id')
**	01-jul-91 (andre)
**	    Added SCI_SESS_USER, SCI_SESS_GROUP, and SCI_SESS_ROLE for
**	    dbmsinfo('sesion_user'), dbmsinfo('session_group') and
**	    dbmsinfo('session_role')
**      06-oct-92 (robf)
**	    Added security info request.
**	02-dec-92 (andre)
**	    we are desupporting SET GROUP/ROLE, so undefine SCI_SESS_GROUP and
**	    SCI_SESS_ROLE
**
**	    SCI_SESS_USER will correspond to dbmsinfo('session_user') (session
**	    auth id) while SCI_INITIAL_USER will correspond to
**	    dbmsinfo('initial_user')
**	23-Nov-1992 (daveb)
**	    Add SCI_IMA_VNODE, SCI_IMA_SERVER, SCI_IMA_SESSION.
**	02-jun-1993 (ralph)
**	    DELIM_IDENT:
**	    Add SCI_DBSERVICE to return the dbservice value
**	9-sep-93 (robf)
**	    Add SCI_MAXUSTAT to return maximum user privs,
**	    Add SCI_AUDIT_STATE, SCI_TABLESTATS, SCI_IDLE_LIMIT,
**	        SCI_CONNECT_LIMIT, SCI_PRIORITY_LIMIT, SCI_CUR_PRIORITY
**	        SCI_MXCONNECT, SCI_MXIDLE
**	17-dec-93 (rblumer)
**	    removed the SCI_FIPS dbmsinfo option.
**	    "FIPS mode" no longer exists.  It was replaced some time ago by
**	    several feature-specific flags (e.g. flatten_nosingleton and
**	    direct_cursor_mode).
**	14-feb-94 (rblumer)
**	    removed FLATOPT and FLATNONE dbmsinfo requests (part of B59120);
**	    moved FLATTEN request to be next to FLATAGG and FLATSING requests,
**	    changed double cursor request names CSRDIRECT/CSRDEFER to
**	    single CSRUPDATE request (part of B59119; also LRC 09-feb-1994).
**	12-dec-95 (nick)
**	    Added SCI_REAL_USER_CASE
**	06-mar-1996 (nanpr01)
**	    Added SCI_MXRECLEN to return maximum tuple length for the
**	    installation. This is for variable page size project to support
**	    increased tuple size for star.
**          Also added SCI_MXPAGESZ, SCI_PAGE_2K, SCI_RECLEN_2K,
**          SCI_PAGE_4K, SCI_RECLEN_4K, SCI_PAGE_8K, SCI_RECLEN_8K,
**          SCI_PAGE_16K, SCI_RECLEN_16K, SCI_PAGE_32K, SCI_RECLEN_32K,
**          SCI_PAGE_64K, SCI_RECLEN_64K to inquire about availabity of
**          bufferpools and its tuple sizes respectively through dbmsinfo
**          function. If particular buffer pool is not available, return
**          0.
**	01-oct-1999 (somsa01)
**	    Added SCI_SPECIAL_UFLAGS for the retrieval of SCS_ICS->ics_uflags.
**	10-may-02 (inkdo01)
**	    Added SCI_SEQ_CREATE for sequence priviliege.
**	12-apr-2005 (gupsh01)
**	    Added SCI_UNICODE_NORM for type of normalization used for unicode
**	    data for the database.
**	18-May-2005 (hanje04)
**	    SCU_MPAGESIZE for MAc OS X needs to me SC_PAGESIZE
**	29-Aug-2006 (gupsh01)
**	    Added ANSI datetime system constants. CURRENT_DATE, CURRENT_TIME
**	    CURRENT_TIMESTAMP, LOCAL_TIME, LOCAL_TIMESTAMP etc.
**	29-Sep-2006 (gupsh01)
**	    Added SCI_DTTYPE_ALIAS for dbmsinfo(_date_type_alias).
**	7-Nov-2007 (kibro01) b119428
**	    Added SCI_DATE_FORMAT, SCI_MONEY_FORMAT,etc
**	26-Oct-2009 (kiria01) SIR 121883
**	    Scalar sub-query support: Added SCI_FLATCHKCARD for
**	    CARDINALITY_CHECK
*/
typedef struct _SCF_SCI
{
    i4		    sci_length;
    i4		    sci_code;

						/*
						** user name for this sess; this
						** will correspond to both
						** dbmsinfo('username') and
						** dbmsinfo('current_user')
						*/
#define                 SCI_USERNAME    1	
		/* returns DB_MAXNAME characters */
#define                 SCI_UCODE       2	/* user code (going away) */
		/* returns two characters */
#define                 SCI_DBNAME      3	/* database name */
		/* returns DB_MAXNAME characters */
#define                 SCI_PID         4	/* server process id */
		/* returns PC_PID */
#define                 SCI_SID         5	/* session id */
		/* returns SCF_SESSION */
#define			SCI_USTAT       6	/* user status bits */
		/* returns an i4 */
#define                 SCI_DSTAT       7	/* database status bits */
		/* returns an i4 */
#define                 SCI_ERRORS      8	/* number errors/session */
		/* returns a i4  */
#define                 SCI_IVERSION    9	/* version of ingres */
		/* returns 4 chars in form major.minor */
#define                 SCI_COMMTYPE    10	/* communication type */
		/* returns i4 */
#define                 SCI_SCFVERSION  11	/* version of SCF */
		/* same as iversion */
#define                 SCI_MEMORY      12	/* amt mem alloc'd to session */
		/* returns i4  (unit = pages) */
#define                 SCI_NOUSERS     13	/* num sessions in server */
		/* returns a i4  */
#define                 SCI_SNAME       14	/* server name */
		/* returns process name (DB_MAXNAME in length) */
#define                 SCI_CPUTIME     15	/* cpu time for server */
		/* returns a i4 */
#define                 SCI_PFAULTS     16	/* page faults for session */
		/* returns a i4 */
#define                 SCI_DIOCNT      17	/* direct i/o cnt for session */
		/* returns a i4 */
#define                 SCI_BIOCNT      18	/* buffered i/o for session */
		/* returns a i4 */
#define			SCI_NOW		19	/* TMsecs() value */
		/* returns a i4 */
#define			SCI_SCB		20	/* returns ptr to scb by fac */
		/* returns a PTR */
#define			SCI_DBID	21	/* returns DMF open db id */
		/* returns a PTR which is what dmf returned */
#define			SCI_UTTY	22	/* returns tty of fe process */
		/* returns sizeof(DB_TERM_NAME) characters */
#define			SCI_LANGUAGE	23	/* returns language for errors
						** (english, ...) as a i4
						*/
#define			SCI_DBA		24	/* returns owner of db */
		/* returns a DB_OWNER */
#define                 SCI_DBCPUTIME   25	/* cpu time for server */
		/* returns a i4 */
#define                 SCI_DBDIOCNT    26	/* direct i/o cnt for server */
		/* returns a i4 */
#define                 SCI_DBBIOCNT    27	/* buffered i/o for server */
		/* returns a i4 */
#define			SCI_GROUP	28	/* returns group id */
		/* returns a DB_OWNER */
#define			SCI_APLID	29	/* returns application id */
		/* returns a DB_OWNER */
#define			SCI_DBPRIV	30	/* returns dbpriv mask */
		/* returns a u_i4 */
#define			SCI_QROW_LIMIT	31	/* returns query row  limit */
		/* returns a i4 */
#define			SCI_QDIO_LIMIT	32	/* returns query I/O  limit */
		/* returns a i4 */
#define			SCI_QCPU_LIMIT	33	/* returns query CPU  limit */
		/* returns a i4 */
#define			SCI_QPAGE_LIMIT	34	/* returns query page limit */
		/* returns a i4 */
#define			SCI_QCOST_LIMIT	35	/* returns query cost limit */
		/* returns a i4 */
#define			SCI_TAB_CREATE	36	/* returns CREATE_TABLE value */
		/* returns a char(1) */
#define			SCI_PROC_CREATE	37	/* ret CREATE_PROCEDURE value */
		/* returns a char(1) */
#define			SCI_SET_LOCKMODE 38	/* returns LOCKMODE value */
		/* returns a char(1) */
#define			SCI_MXIO	39	/* returns temp I/O  limit */
		/* returns a i4 */
#define			SCI_MXROW	40	/* returns temp row  limit */
		/* returns a i4 */
#define			SCI_MXCPU	41	/* returns temp CPU  limit */
		/* returns a i4 */
#define			SCI_MXPAGE	42	/* returns temp page limit */
		/* returns a i4 */
#define			SCI_MXCOST	43	/* returns temp cost limit */
		/* returns a i4 */
#define			SCI_MXPRIV	44	/* returns temp priv mask */
		/* returns a u_i4 */
#define			SCI_IGNORE	45	/* entry is ignored */
		/* returns nothing */
#define			SCI_RUSERNAME	47	/* Real user name */
		/* returns a DB_OWN_NAME */
#define			SCI_TIMEOUT_ABORT 48	/* Timeout abort db priv */
		/* returns a char(1) */
#define			SCI_DB_ADMIN	49	/* returns DB_ADMIN value */
		/* returns a char(1) */
#define			SCI_DISTRANID	50	/* returns ptr to sscb_dis_tran_id */
		/* returns a ptr */
#define			SCI_SV_CB	51	/* returns ptr to server cb */
		/* returns a ptr */
#define			SCI_SECSTATE	52	/* returns security state */
		/* returns a char(32) */
#define			SCI_UPSYSCAT	53	/* returns UPDATE_SYSCAT val */
		/* returns a char(1) */
#define                 SCI_SESSID      54      /* returns session id */
                /* returns a char[DB_MAXNAME] */
						/*
						** returns the initial session
						** user name
						*/
#define			SCI_INITIAL_USER    55
		/* returns a DB_OWN_NAME */
						/*
						** returns the session auth id;
						** most of the time it will be
						** the same as SCI_USERNAME, but
						** if/when module language
						** support is added, they could
						** be different
						*/
#define			SCI_SESSION_USER    56
		/* returns a DB_OWN_NAME */

						/*
						** returns SECURITY value 
						*/
#define			SCI_SECURITY    57
		/* returns a char(1) */
						/*
						** returns the filename of the
						** current security audit log.
						*/
# define		SCI_AUDIT_LOG	58
		/* returns a char(32) */


						/*
						** The IMA-compatible name of
						** the current vnode
						*/
# define		SCI_IMA_VNODE	59
				/* returns char(32) */
						/*
						** The IMA-compatible name of
						** the current server.
						*/
# define		SCI_IMA_SERVER	60
 				/* returns char(32) */
						/*
						** The IMA compatible name
						** of the current session
						** (decimal)
						*/
#define			SCI_IMA_SESSION	61
 				/* returns char(32) */

#define			SCI_CSRUPDATE	62
						/*
						** Returns "DIRECT" if
						** CURSOR_UPDATE_MODE=DIRECT
						** was specified;
						** returns "DEFERRED" if
						** CURSOR_UPDATE_MODE=DEFERRED
						** was specified.
						*/
#define			SCI_SERVER_CAP	63	/* get server capabilities,
						** returns DB_1_LOCAL_SVR for
						** local dbms, DB_2_DISTRIB_SVR
						** for star DBMS, and both
						** for MCS server. */
#define			SCI_SEQ_CREATE	64	/* ret CREATE_SEQUENCE value */
		/* returns a char(1) */
#define			SCI_FLATTEN	65
						/*
						** Returns 'Y' if queries are
						** flattened; 'N' if no queries
						** are flattened..
						*/

#define			SCI_FLATAGG	66
						/*
						** Returns 'Y' if the dbms will
						** attempt to flatten queries
						** involving aggregate subsels;
						** 'N' otherwise.
						*/

#define			SCI_FLATSING	67
						/*
						** Returns 'Y' if the dbms will
						** attempt to flatten queries
						** involving singleton subsels;
						** 'N' otherwise.
						*/

#define			SCI_NAME_CASE	68	/*
						** Returns case translation
						** semantics for regular id's:
						** "UPPER" or "LOWER"
						*/

#define			SCI_DELIM_CASE	69	/*
						** Returns case translation
						** semantics for delimited id's:
						** "UPPER", "LOWER" or "MIXED"
						*/
#define			SCI_DBSERVICE	70	/*
						** Returns pointer to i4,
						** the dbservice value for 
						** the session's database.
						*/
#define			SCI_MAXUSTAT    81	/* 
						** Returns maximum user privs
						*/
#define			SCI_QRYSYSCAT   82	/* 
						** Does session have 
						** QUERY_SYSCAT database priv.
						*/
#define			SCI_AUDIT_STATE 83	/* 
						** Security audit state
						*/
#define			SCI_TABLESTATS	84	/*
						** Does session have
						** TABLE_STATISTICS database 
						** priv
						*/
#define			SCI_IDLE_LIMIT	85	/*
						** Idle time limit
						*/
#define			SCI_CONNECT_LIMIT 86	/*
						** Connect time limit
						*/
#define			SCI_PRIORITY_LIMIT 87	/*
						** Priority limit
						*/
#define			SCI_CUR_PRIORITY   88	/*
						** Current Priority 
						*/
#define			SCI_MXIDLE   	   89	/*
						** return temp idle limit
						*/
#define			SCI_MXCONNECT      90	/*
						** return temp connect limit
						*/
#define			SCI_REAL_USER_CASE 92	/*
						** Returns case translation
						** semantics for users:
						** "UPPER", "LOWER" or "MIXED"
						*/
#define			SCI_MXRECLEN	   93   /*
						** Return maximum tuple length
						** available for installation
						*/
#define			SCI_MXPAGESZ	   94   /*
						** Return maximum page size
						** available for installation
						*/
#define			SCI_PAGE_2K 	   95   /*
						** Return Y if 2K page is  
						** available in installation
						*/
#define			SCI_RECLEN_2K	   96   /*
						** Return maximum rec len for 
						** 2k page in installation
						*/
#define			SCI_PAGE_4K 	   97   /*
						** Return Y if 4K page is  
						** available in installation
						*/
#define			SCI_RECLEN_4K	   98   /*
						** Return maximum rec len for 
						** 4k page in installation
						*/
#define			SCI_PAGE_8K 	   99   /*
						** Return Y if 8K page is  
						** available in installation
						*/
#define			SCI_RECLEN_8K	  100   /*
						** Return maximum rec len for 
						** 8k page in installation
						*/
#define			SCI_PAGE_16K 	  101   /*
						** Return Y if 16K page is  
						** available in installation
						*/
#define			SCI_RECLEN_16K	  102   /*
						** Return maximum rec len for 
						** 16k page in installation
						*/
#define			SCI_PAGE_32K 	  103   /*
						** Return Y if 32K page is  
						** available in installation
						*/
#define			SCI_RECLEN_32K	  104   /*
						** Return maximum rec len for 
						** 32k page in installation
						*/
#define			SCI_PAGE_64K 	  105   /*
						** Return Y if 64K page is  
						** available in installation
						*/
#define			SCI_RECLEN_64K	  106   /*
						** Return maximum rec len for 
						** 64k page in installation
						*/
#define			SCI_SPECIAL_UFLAGS 107 
		/* returns an SCS_ICS_UFLAGS */
						/*
						** Returns the bit settings of
						** SCS_ICS->ics_uflags
						*/

#define			SCI_PAGETYPE_V1	    108 /*
						** Return Y if V1 page type is  
						** available in installation
						*/
#define			SCI_PAGETYPE_V2	    109 /*
						** Return Y if V2 page type is  
						** available in installation
						*/
#define			SCI_PAGETYPE_V3	    110 /*
						** Return Y if V3 page type is  
						** available in installation
						*/
#define			SCI_PAGETYPE_V4	    111 /*
						** Return Y if V4 page type is  
						** available in installation
						*/
#define			SCI_UNICODE	    112 /* unicode Y/N             */
#define			SCI_LP64	    113 /* lp64 Y/N                */
#define			SCI_QBUF	    114 /* Return query */
#define			SCI_QLEN	    115 /* Return query len */
#define			SCI_TRACE_ERRNO	    116 /* Return Error to be traced */
#define			SCI_PREV_QBUF	    117 /* Return prev query */
#define			SCI_PREV_QLEN	    118 /* Return prev query len */
#define			SCI_CSRDEFMODE	    119 /* Return default cursor mode 
						while opening */
						 /* 
						 ** Returns "READONLY" if
						 ** CURSOR_DEFAULT_MODE=READONLY
						 ** was specified;
						 ** returns "UPDATE" if
						 ** CURSOR_DEFAULT_MODE=UPDATE
						 ** was specified.
						  */
#define			SCI_CSRLIMIT	    120 /* Return cursor limit */
#define			SCI_UNICODE_NORM    121	/*
						** Returns type of 
						** normalization used for 
						** processing unicode data
						** valid values are:
						** "NFC" or "NFD"
						*/
#define			SCI_CURDATE	    122	/* Returns current date */
#define			SCI_CURTIME	    123	/* Returns current time */
#define			SCI_CURTSTMP	    124	/* Returns current timestamp */
#define			SCI_LCLTIME	    125	/* Returns local time */
#define			SCI_LCLTSTMP	    126	/* Returns local timestamp */
#define			SCI_DTTYPE_ALIAS    127	/* Returns INGRES or ANSI */
#define			SCI_PSQ_QBUF	    128 /* Return PSQ query */
#define			SCI_PSQ_QLEN	    129 /* Return PSQ query len */
#define			SCI_DATE_FORMAT     130	/* Returns date format */
#define			SCI_MONEY_FORMAT    131	/* Returns money format */
#define                 SCI_MONEY_PREC      132 /* Returns money precision */
#define                 SCI_DECIMAL_FORMAT  133 /* Returns decimal format */
#define			SCI_PAGETYPE_V5	    134 /*
						** Return Y if V5 page type is  
						** available in installation
						*/
#define			SCI_FLATCHKCARD	    135 /*
						** Returns 'Y' if the dbms will
						** flag cardinality errors from
						** when WHERE clause singleton
						** selects are flattened.
						** Prior to 10.0.0 flattening
						** made WHERE clause subselects
						** implicitly ANY().
						** 'N' otherwise.
						*/
#define			SCI_PAGETYPE_V6	    136 /* Y if V6 pgtype available */
#define			SCI_PAGETYPE_V7	    137 /* Y if V7 pgtype available */
#define			SCI_PARENT_SCB      138 /* returns ptr to parent scb by fac
                                                ** for factotum threads. Null otherwise
                                                */


    char	    *sci_aresult;
    i4		    *sci_rlength;
}   SCF_SCI;

/*}
** Name: SCI_LIST - List of items in which the client is interested
**
** Description:
**      This structure is used by the client to present SCF with a list
**      of items in which he/she/it is interested.  The list maintained
**      as a separate structure from the SCF_CB so as to make it easier
**      to allocate in a normal fashion.  It also allows for simple validation
**      of control block length.
**
** History:
**     2-Apr-86 (fred)
**          Created on Jupiter.
*/
typedef struct _SCI_LIST
{
    SCF_SCI	sci_list[1];		/* the '1' is merely to establish as an array */
}   SCI_LIST;

/*}
** Name: SCF_SCM - Memory Request Block
**
** Description:
**      This structure is used to request memory from SCF.
**      It is passed to scf_call() with an operation code
**      in the SCU_M* class (e.g. SCU_MALLOC, SCU_MFREE).
**      
**      The block contains four elements.  scm_function should be filled
**	with any modifiers requested (i.e. to zero or erase the memory before
**	returning to the caller;  scm_in_pages is expected to be
**      filled with the number of SCU_MPAGESIZE byte pages requested;
**      scm_out_pages will be filled by scf_call() with the number of pages
**      actually given to the caller;  scm_addr will be filled with the pointer
**      to the first byte of the first page returned.
**
** History:
**     03-dec-1985 (fred)
**          Created.
**	02-dec-1998 (nanpr01)
**	    Added SCU_LOCKED_MASK for locking memory. This should match
**	    ME_LOCKED_MASK in mecl.h.
**	18-May-2005 (hanje04)
**	    SCU_MPAGESIZE for Mac OS X needs to me SC_PAGESIZE
*/
typedef struct _SCF_SCM
{
    i4	    scm_functions;
#define                 SCU_MZERO_MASK		0x01L
#define                 SCU_MERASE_MASK		0x02L
#define			SCU_MNORESTRICT_MASK	0x04L
#define			SCU_LOCKED_MASK		0x10L
    SIZE_TYPE	    scm_in_pages;		/* number of pages requested */
#ifdef VMS
#define			SCU_MPAGESIZE		512L	    /* on a VAX */
#else
#ifdef OSX
#define			SCU_MPAGESIZE		SC_PAGESIZE  /* Mac OS X */
#elif defined(UNIX) || defined(NT_IA64)
#define			SCU_MPAGESIZE		8192L	    /* UNIX (specifically, sun) */
#else
#ifdef PMFE
#define                 SCU_MPAGESIZE           2048L
#else
#define			SCU_MPAGESIZE		4096L	    /* on a 370 */
#endif
#endif
#endif
    SIZE_TYPE	    scm_out_pages;		/* filled by the callee */
    char	    *scm_addr;			/*   ditto */
}   SCF_SCM;

/*}
** Name: SCF_FTC - Factotum Thread Creation Block
**
** Description:
**	This structure is used to request the creation of a Factotum thread.
**      
**
** History:
**	27-Feb-1998 (jenjo02)
**	    Created
*/
typedef struct _SCF_FTC
{
    i4	    	    ftc_facilities;		/* facilities needed by factotum thread */
#define			SCF_CURFACILITIES	-1 /* same facilities as creating thread */
    i4	    	    ftc_priority;		/* priority of factotum thread */
#define			SCF_MAXPRIORITY		CS_MAXPRIORITY
#define			SCF_MINPRIORITY		CS_MINPRIORITY
#define			SCF_DEFPRIORITY		CS_DEFPRIORITY
#define			SCF_CURPRIORITY		CS_CURPRIORITY
    PTR		    ftc_data;			/* data, or NULL, to be presented to */
    i4	    	    ftc_data_length;		/* thread and its length */
    char	    *ftc_thread_name;		/* what to call this thread */
    DB_STATUS	    (*ftc_thread_entry)();	/* thread execution code */
    VOID	    (*ftc_thread_exit)();	/* thread termination code, or NULL */
    CS_SID	    ftc_thread_id;		/* thread id of factotum thread */
						/* (returned to thread creator) */
}   SCF_FTC;

/*}
** Name: SCF_FTX - Factotum Thread eXecution Block
**
** Description:
**	This structure is passed to a Factotum thread execution and
**	termination functions.
**      
**
** History:
**	27-Feb-1998 (jenjo02)
**	    Created
*/
typedef struct _SCF_FTX
{
    PTR		    ftx_data;			/* Thread data, or NULL, */
    i4	    	    ftx_data_length;		/* and its length */
    CS_SID	    ftx_thread_id;		/* thread id of creating (parent) thread */
    DB_STATUS	    ftx_status;			/* status returned by thread execution code */
    DB_ERROR	    ftx_error;			/* error set by thread execution code */
}   SCF_FTX;

/*}
** Name: SCF_SEMAPHORE - semaphore implementation structure
**
** Description:
**      This structure is allocation in a user facilities space
**      to implement a set of semaphore operations.
**
**	This structure must mimic the CS_SEMAPHORE structure as
**	scf will call CS routines to do semaphore locking and unlocking.
**
** History:
**     3-Mar-1986 (fred)
**          Created on Jupiter
**	28-feb-1989 (rogerk)
**	    Changed SCF_SEMAPHORE definition to match new CS_SEMAPHORE struct.
**	    Added scse_type and scse_pid fields.
**	19-jun-1989 (rogerk)
**	    Changed to be duplicate of CS_SEMAPHORE.
*/
typedef	    CS_SEMAPHORE	SCF_SEMAPHORE;

/*}
** Name: SCF_SEM_INTRNL - internal semaphore implementation structure
**
** Description:
**	This structure defines a semaphore structure that can be used
**	to implement semaphores by SCF, rather than using CS.
**
**	This structure is defined to be the same as the old SCF_SEMAPHORE
**	structure.
**
** History:
**	28-feb-1989 (rogerk)
**	    Created for use by the SCF_SINGLE_USER code after making
**	    SCF_SEMAPHORE be defined to be the same as a CS_SEMAPHORE.
*/
typedef struct _SCF_SEM_INTRNL
{
    SCF_SESSION     scse_session;       /* Current owner of the semaphore */
    SCF_SESSION     scse_next;          /* next person awaiting the semaphore */
    i4              scse_semaphore;     /* the semaphore itself */
    i4	    scse_checksum;	/*
					** so SCF can tell if the semaphore
					** has been corrupted
					*/
/*
**      These are the checksums for phase 1 of SCF.
**      They are intended only for the single user model;
**      and must be replaced with real checksums when
**      doing multi-user checksums.
*/
#define                 FREE_PHASE_1        'free'
#define                 SET_SHARE_PHASE_1   'shar'
#define                 SET_EXCLUSIVE_1     'excl'
}   SCF_SEM_INTRNL;

/*}
** Name: SCF_ALERT	- Structure for passing an Event description to SCF
**
** Description:
**	This structure is used to contain the description of an event (alerter)
**	being passed to SCF.
**
** History:
**	07-sep-89 (paul)
**	    First written to support alerters
**	14-feb-90 (neil)
**	    Modified to use pointers.
*/
typedef struct _SCF_ALERT
{
    DB_ALERT_NAME   *scfa_name;		/* Name of the event (alerter) */
    DB_DATE	    *scfa_when;		/* Timestamp when event was raised.
					** This field is ignored when not
					** used with the RAISE operation.
					*/
    i4		    scfa_flags;		/* Request modifiers */
#define	    SCE_NOSHARE	    0x01	/* Flag indicating that the event is
					** private to the current session. This
					** event may only be raised or received
					** by the current session.
					*/
#define	    SCE_NOORIG	    0x02	/* Flag indicating that the event is
					** not to be sent to the originating
					** server */
    i4		    scfa_text_length;	/* Length of the following text string*/
    char	    *scfa_user_text;	/* User text associated with this
					** occurrence of the named event.
					*/
}   SCF_ALERT;

/*}
** Name: SCF_CB - User level request block for services from the SCF
**
** Description:
**      This control block is used as the request mechanism for calls
**      to scf_call().  The caller is expected to fill in the appropriate
**	fields before calling.  Old information stored in fields which are
**      filled in by SCF will be lost.  Fields which are not used in the
**      request are undefined at call return time.
**
** History:
**     04-Dec-1985 (fred)
**          Created.
**	 03-Jul-1986 (fred)
**	    Added standard control block header.  Unfortunately, this makes the
**	    SCF_CB too big (72 bytes).  Oh well.  Maybe I'll think of more stuff
**	    to put in it to fill it out to 96 bytes.
**	24-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    Added scf_xpassword, scf_xpwdlen to SCF_CB.
**	24-mar-89 (paul)
**	    Added new stype in support of user defined error messages.
**	04-may-89 (neil)
**	    Removed user defined type - in union it overwrites the number.
**	20-may-89 (ralph)
**	    Added scf_xpasskey field to SCF_CB.
**	23-may-89 (jrb)
**	    Added the "scf_aux_union" union and made it hold the generic error
**	    code.  Also, renamed scf_enumber to scf_local_error.
**	12-jun-89 (rogerk)
**	    Added scf_sngl_sem to scf_ptr_union for SCF to use for single
**	    user semaphores.
**	20-oct-92 (andre)
**	    replaced scf_generic_error with scf_sqlstate
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	23-may-96 (stephenb)
**	    Add scf_dbname field for replicator calls from DMF.
**	30-jan-96 (stephenb)
**	    Add scf_dml_scb to scf_ptr_union for single user SCF inits.
**	20-Apr-2004 (jenjo02)
**	    Changed scf_afcn's to return STATUS rather than VOID.
*/
struct _SCF_CB
{
    SCF_CB	    *scf_next;
    SCF_CB	    *scf_prev;
    SIZE_TYPE	    scf_length;		/* length of this control block */
    i2		    scf_type;		/* the type of the control block */
#define		SCF_CB_TYPE	    (i2) 0x2
    i2		    scf_s_reserved;
    PTR		    scf_l_reserved;
    PTR		    scf_owner;
    i4		    scf_ascii_id;
#define		SCF_ASCII_ID	    CV_C_CONST_MACRO('s', 'c', 'f', ' ')
    DB_ERROR        scf_error;          /* error information */
    i4		    scf_facility;	/* facility id of requestor */
    SCF_SESSION	    scf_session;        /* session this is for */
    union
    {
	i4		scf_blength;	/* length of the error txt to send */
	i4		scf_ilength;	/* number of information requests */
	i4		scf_xpwdlen;	/* Length of password to encrypt */
    }		    scf_len_union;
    union
    {
        i4		scf_local_error;  /* error number corresponding to ... */
        i4              scf_atype;      /* Activity type */
#define             SCS_A_MAJOR	    0
#define             SCS_A_MINOR	    1
        i4		scf_stype;	  /* is it share or exclusive */
#define		SCU_SHARE	    0x0
#define		SCU_EXCLUSIVE	    0x1
#define		SCU_LT_MASK	    0x100   /* long term, can suspend */
	i4		scf_amask;	/* what type of AIC/PAINE request is this */
	PTR	    scf_xpasskey;	/* pointer to password key */
    }		    scf_nbr_union;
    union
    {
	DB_SQLSTATE	scf_sqlstate;  /* SQLSTATE status code for this error */
    }		    scf_aux_union;
    union
    {
	PTR             scf_buffer;     /* buffer for error message */
	SCF_SEMAPHORE   *scf_semaphore;	/* semaphore to wait */
	SCF_SEM_INTRNL	*scf_sngl_sem;	/* single-user semaphore */
	SCI_LIST	*scf_sci;	/* request for information */
	STATUS          (*scf_afcn)();  /* address of async function for aic/paine */
	PTR		scf_xpassword;	/* Pointer to password to encrypt */
	SCF_ALERT	*scf_alert_parms;   /*Pointer to event (alerter) request
					    ** parameter block.
					    */
	PTR		scf_dml_scb;	/* DML_SCB for SCF single user init */
	SCF_FTC		*scf_ftc;	/* factotum thread info */
    }		    scf_ptr_union;
    SCF_SCM	    	scf_scm;
    DB_DB_NAME	    *scf_dbname;	/* name of db to operate on */
};
#define		SCF_CB_SIZE	    sizeof(SCF_CB)

/*}
** Name: SCS_ICS_UFLAGS - SCS_ICS->ics_flags information
**
** Description:
**	This structure defines those bits which are set in SCS_ICS->ics_flags.
**	Keep in mind that if more bits are added in the future, they should
**	also be added to this structure as well. It is primarily used when
**	a facility wants to gain access to those bits via SCU_INFORMATION,
**	such as whether or not a session user has the SERVER_CONTROL Ingres
**	system privilege.
**
** History:
**      01-oct-1999 (somsa01)
**	    Created.
*/
typedef struct _SCS_ICS_UFLAGS
{
    bool	scs_usr_eingres;	/* Effective user is $ingres */
    bool	scs_usr_rdba;		/* Real user is dba */
    bool	scs_usr_rpasswd;	/* Real user has password */
    bool	scs_usr_nosuchusr;	/* User is invalid */
    bool	scs_usr_prompt;		/* User can be prompted */
    bool	scs_usr_rextpasswd;	/* User has external password */
    bool	scs_usr_dbpr_nolimit;	/* User has no dbpriv limits */
    bool	scs_usr_svr_control;	/* Session user has SERVER_CONTROL */
    bool	scs_usr_net_admin;	/* Session user has NET_ADMIN */
    bool	scs_usr_monitor;	/* Session user has MONITOR */
    bool	scs_usr_trusted;	/* Session user has TRUSTED */
} SCS_ICS_UFLAGS;

/*}
** Name: SCC_SDC_CB - STAR dicrect connect control block
**
** Description:
**      This control block is used for controling STAR direct 
**	connect protocols. It is only understood by STAR.
**
** History:
**      13-sep-1988 (alexh)
**	    created
**	31-mar-1990 (carl)
**	    added sdc_b_remained, sdc_b_ptr and sdc_err for byte-alignment 
**	    fix; also renamed sdc_status to sdc_ldb_status
**	12-jan-1993 (fpang)
**	    Added sdc_peer_protocol.
*/
typedef struct _SCC_SDC_CB
{
  i4		sdc_mode;	/* current message flow direction */
#define	SDC_WRITE	0
#define	SDC_READ	1
#define SDC_CHK_INT	2
  PTR		sdc_client_msg;	/* client message, i.e., a GCA_RV_PARMS */
  i4		sdc_msg_type;	/* current GCA message type */
  PTR		sdc_ldb_msg;	/* LDB message to be returned to the client */
  PTR		sdc_rv;	/* LDB message */
  i4	sdc_ldb_status;	/* status of the LDB association */
#define	SDC_OK		0
#define	SDC_LOST_ASSOC	1
#define	SDC_INTERRUPTED	2
  u_i4	sdc_id1_cdesc;	/* tuple id of client message */
  PTR		sdc_td1_cdesc;	/* tuple descriptor pointer of client message */
  u_i4	sdc_id2_ldesc;	/* tuple id of LDB message */
  PTR		sdc_td2_ldesc;	/* tuple descriptor pointer of  client message 
				*/
  i4	sdc_b_remained;	/* number of bytes remained unread in the 
				** downstream association */
  PTR		sdc_b_ptr;	/* pointer at the first unread byte in the
				** downstream association */
  DB_ERROR	sdc_err;
  i4	sdc_peer_protocol; /* GCA protocol of ldb */
} SCC_SDC_CB;

/*}
** Name: SCF_TD_CB - STAR tuple descriptor control block
**
** Description:
**      This control block is used as a place holder for the tuple description.
**	SCF passes this structure to RQF through QEF to copy in the TUPLE descriptor
**	from the LDB. 
**
** History:
**	19-aug-1989 (georgeg)
**	    Created
**	29-sep-1989 (georgeg)
**	    added field scf_1ldb_size, scf_1star_size. 
**	29-may-1990 (georgeg)
**	    removed  fields scf_1ldb_size, scf_1star_size, to fix bug 21405. 
*/
typedef struct _SCF_TD_CB
{
  PTR		scf_ldb_td_p;	/* whete to put the ldb tuple descriptor */
  PTR		scf_star_td_p;	/* A pointer to the TD descriptor as calculated by STAR*/
} SCF_TD_CB;


/* Function externs */

FUNC_EXTERN STATUS scf_call( i4 operation, SCF_CB *cb );

/* Factotum thread entry and termination function externs: */

/* Writebehind threads: */
FUNC_EXTERN DB_STATUS dmc_write_behind_primary( SCF_FTX *ftx );
FUNC_EXTERN DB_STATUS dmc_write_behind_clone( SCF_FTX *ftx );
FUNC_EXTERN VOID dm0p_wb_clone_exit( SCF_FTX *ftx );
FUNC_EXTERN STATUS      build_pindex( SCF_FTX *ftx );
FUNC_EXTERN VOID        build_pindex_exit( SCF_FTX *ftx );
FUNC_EXTERN DB_STATUS dmrAgent( SCF_FTX *ftx );
FUNC_EXTERN DB_STATUS DMUSource( SCF_FTX *ftx );
FUNC_EXTERN DB_STATUS DMUTarget( SCF_FTX *ftx );
