/*
**Copyright (c) 2004 Ingres Corporation
**
*/

/*
** Name: SXF.H - All external definitions needed to use the services of
**		 the Security Extensions Facility.
**
** Description:
**      This file contains all of the typedefs and defines necessary
**      to use any of the Security Extensions Facility routines.
**
**      This file defines the following typedefs:
**	    SXF_RCB		- SXF request block.
**	    SXF_AUDIT_REC	- SXF audit record structure
**
** History: 
**	6-july-1992 (markg)
**	    Initial creation.
**	30-jul-92 (pholman)
**	    Added prototype pieces, error codes etc.
**	14-aug-92 (pholman)
**	    Due to qef!qeferror and associated code, RDBMS server facilities
**	    must have consecutive error message classes (cf dbms.h).  Change
**	    the SXF error message class from the assigned 214 to 16.
**	28-sep-92 (pholman)
**	    Bring SXF_RCB uptodate with design spec.
**	22-sep-92 (markg/robf)
**	    Bring sxf versions into sync
**	25-oct-92 (markg)
**	13-nov-1992 (pholman)
**	    Add 'object owner' to RCB and audit data structures.
**	18-nov-1992 (robf)
**	    SXF_E_ALL should be last event.
**	27-nov-1992 (robf/markg)
**	    Added SXF_A_SETUSER operation, this is used when auditing
**	    a change of the current user (aka SET USER AUTHORIZATION)
**          In addition to being a regular audit action it also 
**	    causes SXF to refresh its user info.
**	28-nov-1992 (markg)
**	    Add new error messages and opcodes for the SXF audit thread.
**	22-dec-1992 (markg/robf)
**	    Merge new error messages.
**	11-jan-1993 (robf/markg)
**	    Add fields in audit rec for optional info (labels, detail)
**	3-feb-1992 (markg)
**	    Added new error messages E_SX106A and E_SX106B.
**	30-mar-1993 (robf)
**	    Add definitions for B1 Secure 2.0
**	13-apr-1993 (robf)
**	    Add SXM/SXL interface calls/structures
**	16-jul-93 (robf)
**	    Added sxf_trace function definition.
**	13-aug-93 (stephenb)
**	    Added error messages E_SX10A7 and E_SX10A6.
**	20-aug-93 (stephenb)
**	    Added error messages I_SX2037 and I_SX2038.
**	23-aug-93 (stephenb)
**	    Added error messages I_SX272A and I_SX272B.
**      14-sep-93 (smc)
**          Made sxf_sess_id a CS_SID.
**	21-sep-93 (stephenb)
**	    Changed I_SX2032_EVENT_ACCESS to I_SX2032_EVENT_CREATE and added
**	    I_SX203C_EVENT_DROP.
**	23-sep-93 (stephenb)
**	    Added error message I_SX203D_INTEGRITY_CREATE.
**	1-oct-93 (stephenb)
**	    Add messages I_SX203E_SCHEMA_CREATE and I_SX203F_SCHEMA_DROP.
**	6-oct-93 (stephenb)
**	    Add messages I_SX2040_TMP_TBL_CREATE and I_SX2041_TMP_TBL_DROP.
**	8-oct-93 (robf)
**          Add E_SX10B3
**	19-oct-93 (stephenb)
**	    Add message I_SX2042_REMOVE_TABLE, and change SX_C2_MODE to
**	    conform with CBF parameters.
**	9-nov-93 (robf)
**	    Add I_SX2740_SET_QUERY_LIMIT, I_SX2741_SET_LOCKMODE
**	19-nov-93 (stephenb)
**	    Add messages I_SX2043_GW_REGISTER, I_SX2044_GW_REMOVE.
**	30-dec-93 (robf)
**          Add I_SX2744_SESSION_IDLE_LIMIT, I_SX2745_SESSION_CONNECT_LIMIT
**      7-jan-94 (stephenb)
**          Add following messages for SXAPO functionality:
**              E_SX002D_OSAUDIT_NO_POSITION
**              E_SX002E_OSAUDIT_NO_READ
**              E_SX10B4_OSAUDIT_WRITE_FAILED
**              E_SX10B5_OSAUDIT_FLUSH_FAILED
**              E_SX10B6_OSAUDIT_OPEN_FAILED
**	18-feb-94 (stephenb)
**	    Add message I_SX2747_DUMPDB.
**	4-feb-94 (stephenb)
**          Added E_SX10B7_BAD_AUDIT_METHOD and defined SX_C2_AUDIT_METHOD
**          and SX_C2_OSAUDIT and SX_C2_INGAUDIT.
**	9-feb-94 (robf)
**          Add E_SX10B9_BAD_MO_ATTACH
**	14-feb-94 (robf)
**	    Add SXF_A_CONNECT, SXF_A_DISCONNECT for alarms
**	18-feb-94 (stephenb)
**	    Add message I_SX2747_DUMPDB.
**	22-feb-94 (robf)
**          Add I_SX2746_SESSION_ROLE.
**	24-feb-94 (stephenb)
**	    Add E_SX10BA_OSAUDIT_NOT_ENABLED.
**	29-feb-94 (stephenb)
**	    Alter SX_C2_OSAUDIT, SX_C2_INGAUDIT and SX_C2_AUDIT_METHOD to 
**	    current LRC submission.
**	7-mar-94 (robf)
**          Add I_SX274F_ROLE_REVOKE and I_SX274E_ROLE_GRANT
**	10-mar-94 (robf)
**          Add E_SX10BB_SXAP_CHK_PROTO_INFO
**	7-apr-94 (stephenb)
**	    Add message E_SX10BB_SXAP_CHK_PROTO_INFO
**	13-apr-94 (robf)
**          Add I_SX2750_SET_TRACE_POINT
**              I_SX2751_SET_NOTRACE_POINT
**      04-jan-95 (alison)
**          Add I_SX2752_RELOCATE
**	 1-nov-95 (nick)
**	    Move some lock definitions into here from within SXF.
**	10-oct-1996 (canor01)
**	    Changed INGRES_HIGH/LOW to CA_HIGH/LOW.
**	27-nov-1996 (canor01)
**	    Add SXF_C2_OLDDEFAUDIT for old default audit mechanism
**	    for use during upgrade.
**      26-feb-97 (dilma04)
**          Add I_SX2753_ROW_DEADLOCK.
**	31-Aug-1998 (hanch04)
**	    Added SXF_BYTES_PER_SINGLE_SERVER.  Based on SXF_BYTES_PER_SERVER
**	    and SXF_BYTES_PER_SES.
**	10-nov-1998 (sarjo01)
**	    Added I_SX2754_SET_TIMEOUTABORT.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Apr-2003 (jenjo02)
**	    Added I_SX2755_KILL_QUERY, SIR 110141.
**      12-Apr-2004 (stial01)
**          Define sxf_length as SIZE_TYPE.
**	29-Apr-2004 (gorvi01)
**		Added SXF_A_UNEXTEND
**      7-oct-2004 (thaju02)
**          Replace i4 with SIZE_TYPE for memory pool > 2Gig.
**	23-Jan-2006 (kschendel)
**	    Swap audit-mechanism strings so that INGRES is the default
**	    and CA is the old deprecated keyword.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
*/


/*
**  Defines of other constants.
*/

/*
** Memory for single user server.  Needed for dmfjsp.
*/
#define SXF_BYTES_PER_SINGLE_SERVER 65532L

/*}
** Name: SXF_OPERATION - SXF call operation codes
**
** Description:
**	This describes all the valid operation codes for external
**	calls to SXF.
**
** History:
**	9-july-1992 (markg)
**	    Initial creation.
**      11-nov-1992 (markg)
**          Added SXC_AUDIT_THREAD definition, increased value of
**          SXF_MAX_OPCODE.
**	14-apr-94 (robf)
**          Added SXC_AUDIT_WRITER_THREAD definition, increased value of
**          SXF_MAX_OPCODE.
*/
typedef  i4  SXF_OPERATION;
# define	SXF_MIN_OPCODE		1L
# define	SXC_STARTUP		1L
# define	SXC_SHUTDOWN		2L
# define	SXC_BGN_SESSION		3L
# define	SXC_END_SESSION		4L
# define	SXA_OPEN		5L
# define	SXA_CLOSE		6L
# define	SXR_POSITION		7L
# define	SXR_READ		8L
# define	SXR_WRITE		9L
# define	SXR_FLUSH		10L
# define	SXS_SHOW		11L
# define	SXS_ALTER		12L
# define        SXC_AUDIT_THREAD        13L

# define	SXC_ALTER_SESSION	21L

# define	SXC_AUDIT_WRITER_THREAD 22L

# define	SXF_MAX_OPCODE		22L


/*
** Name: SXF_ACCESS - type of access to be audited
**
** Description:
**	This typedef describes the access type of an auditable 
**	event.
**
** History:
**	9-july-1992 (markg))
**	    Initial creation.
**	21-sep-1992 (robf)
**	     Added SXF_A_REGISTER, SXF_A_REMOVE, SXF_A_RAISE
**	11-may-1993 (robf)
**	     Secure 2.0: Add MAC security events (REGRADE etc)
*/
typedef  i4  SXF_ACCESS;
# define	SXF_A_SUCCESS		0x00000001L
# define	SXF_A_FAIL		0x00000002L
# define	SXF_A_SELECT		0x00000004L
# define	SXF_A_INSERT		0x00000008L
# define	SXF_A_DELETE		0x00000010L
# define	SXF_A_UPDATE		0x00000020L
# define	SXF_A_COPYINTO		0x00000040L
# define	SXF_A_COPYFROM		0x00000080L
# define	SXF_A_SCAN		0x00000100L
# define	SXF_A_ESCALATE		0x00000200L
# define	SXF_A_EXECUTE		0x00000400L
# define	SXF_A_EXTEND		0x00000800L
# define	SXF_A_CREATE		0x00001000L
# define	SXF_A_MODIFY		0x00002000L
# define	SXF_A_DROP		0x00004000L
# define	SXF_A_INDEX		0x00008000L
# define	SXF_A_ALTER		0x00010000L
# define	SXF_A_RELOCATE		0x00020000L
# define	SXF_A_CONTROL		0x00040000L
# define	SXF_A_AUTHENT		0x00080000L
# define	SXF_A_REGRADE		0x00100000L
# define	SXF_A_WRITEFIXED	0x00200000L
# define	SXF_A_QRYTEXT		0x00600000L
# define	SXF_A_SESSION		0x00700000L
# define	SXF_A_TERMINATE		0x00800000L
# define	SXF_A_QUERY		0x00900000L
# define	SXF_A_LIMIT		0x00A00000L
# define	SXF_A_USERMESG		0x00B00000L
# define	SXF_A_SETLABEL		0x00C00000L
# define        SXF_A_CONNECT           0x00D00000L
# define        SXF_A_DISCONNECT        0x00E00000L
# define	SXF_A_UNEXTEND		0x01000000L
/*
# define	SXF_A_READ		0x00100000L
# define	SXF_A_WRITE		0x00200000L
# define	SXF_A_READWRITE		0x00400000L
*/
/* # define	SXF_A_DOWNGRADE		0x01000000L*/
# define	SXF_A_BYTID		0X02000000L
# define	SXF_A_BYKEY		0x04000000L
# define	SXF_A_MOD_DAC		0x08000000L
# define        SXF_A_REGISTER          0x10000000L
# define        SXF_A_REMOVE            0x20000000L
# define        SXF_A_RAISE             0x40000000L
# define        SXF_A_SETUSER           0x80000000L

/*
** Name: SXF_EVENT - class of events to be audited.
**
** Description:
**	This typedef describes the type of security audit
**	event that may exist in the installation security
**	audit profile of an installation.
**
** History:
**	9-July-1992 (markg)
**	    Initial creation.
**	11-may-1993 (robf)
**	    Secure 2.0: Add RESOURCE and QRYTEXT event types
**	24-feb-94 (robf)
**          SXF_E_APPLICATION and SXF_E_ROLE are synonyms.
*/
typedef  i4  SXF_EVENT;
# define	SXF_E_DATABASE		1L
# define	SXF_E_APPLICATION	2L
# define	SXF_E_ROLE		SXF_E_APPLICATION
# define	SXF_E_PROCEDURE		3L
# define	SXF_E_TABLE		4L
# define	SXF_E_LOCATION		5L
# define	SXF_E_VIEW		6L
# define	SXF_E_RECORD		7L
# define	SXF_E_SECURITY		8L
# define	SXF_E_USER		9L
# define	SXF_E_LEVEL		10L
# define	SXF_E_ALARM		11L
# define	SXF_E_RULE		12L
# define	SXF_E_EVENT		13L
# define	SXF_E_INSTALLATION	14L
# define	SXF_E_ALL		15L	
# define	SXF_E_RESOURCE		16L	/* Resource limit event */
# define	SXF_E_QRYTEXT		17L	/* Query text event */
# define	SXF_E_MAX		17L

/*
** Name: SXF_AUDIT_REC - a SXF audit record
**
** Description:
**      This typedef describes the internal structure of a SXF audit.
**      Audit records will be stored in this format in INGRES security
**      audit logs (SAL). And records will be returned in this format
**      to callers using the SXR_READ request.
**
** History:
**      07-may-1992 (pholman)
**          Initial creation.
**	11-jan-1993 (robf/markg)
**	    Added new audit fields (label, detail). These are not used
**	    currently, but should prevent requiring a conversion of
**	    the audit file when they are used later
**	7-jul-93 (robf)
**	    Added session id field
*/
typedef  struct  _SXF_AUDIT_REC
{
    SXF_EVENT       sxf_eventtype;      /* The type of audit event */
    SYSTIME         sxf_tm;             /* Time stamp for audit record */
    DB_OWN_NAME     sxf_ruserid;        /* Real identity of the user */
    DB_OWN_NAME     sxf_euserid;        /* Effective  Identity of user */
    DB_DB_NAME	    sxf_dbname;		/* Database action applies to */
    i4         sxf_messid;         /* Textual message id (ER_MSGID) */
    STATUS          sxf_auditstatus;    /* Did operation suceed or fail? */
    i4         sxf_userpriv;	/* Privilege mask for this user */
    i4         sxf_objpriv;	/* If the audit object is a user this
                                        ** will contain the privilege of the
                                        ** user.
                                        */
    SXF_ACCESS      sxf_accesstype;     /* Access type of audit operation */
    DB_OWN_NAME     sxf_objectowner;    /* Owner of object being accessed */
    char            sxf_object[DB_OBJ_MAXNAME];
                                        /* Name of object being accessed */
# define SXF_DETAIL_TXT_LEN  256        /* Size of sxf_detail_txt */
    char	    *sxf_detail_txt;	/* Text of detail, may be NULL */
    i2		    sxf_detail_len;	/* Length of text detail */
    i4	      	    sxf_detail_int;     /* Integer detail */
# define SXF_SESSID_LEN 16
    char	    sxf_sessid[SXF_SESSID_LEN];	/* Session Id */
} SXF_AUDIT_REC;

/*
** Name: SXF_RCB - request block for all calls to SXF via sxf_call().
**
** Description:
**      This structure defines the request block which is used for passing
**      and retrieving all information to and from all calls to SXF. This
**      is done via the sxf_call() routine, where a SXF_RCB is the second
**      argument (the opcode is the first).
**
** History:
**	6-july-1992 (markg)
**	    Initial creation.
**	28-sep-1992 (pholman)
**	    Changes in design have caused the modification of some types,
**	    and made others obsolescent.
**	20-oct-93 (robf)
**          Updated auditstate values to be bitmap so changefile can
**	    be specified along with restart/resume
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
*/
typedef struct _SXF_RCB
{
    struct _SXF_RCB     *sxf_next;	/* Forward pointer in cb list */
    struct _SXF_RCB     *sxf_prev;      /* Backward pointer in cb list */
    SIZE_TYPE           sxf_length;     /* Length of this structure */
    i2                  sxf_cb_type;    /* Type of this structure */
# define        SXFRCB_CB               105
    i2                  sxf_s_reserved;
    PTR                 sxf_l_reserved;
    PTR                 sxf_owner;      /* Structure owner */
    i4                  sxf_ascii_id;   /* So as to see in a dump. */
# define        SXFRCB_ASCII_ID         CV_C_CONST_MACRO('S', 'X', 'R', 'B')
    i4                  sxf_mode;       /* Operation mode of the facility, 
                                        ** supplied to SXC_STARTUP.
					** If the facility is being started 
					** as a  part of the DBMS this should 
					** be setto SXF_MULTI_USER, if the 
					** facility is being started as a part 
					** of a standalone utility this should 
					** be set to SXF_SINGLE_USER. 
                                        */
# define        SXF_MULTI_USER          1
# define        SXF_SINGLE_USER         2
    i4		sxf_status;	/* The facility startup status. This
					** is supplied for SXC_STARTUP
					** requests.
					*/
# define	SXF_C2_SERVER		0x0001L
# define	SXF_STAR_SERVER		0x0002L
    PTR                 sxf_server;     /* Pointer to memory for SXF's server
                                        ** control block. This is returned
                                        ** by SXC_STARTUP, and must be supplied
                                        ** for SXC_BGN_SESSION.
                                        */
    PTR                 sxf_scb;        /* Pointer to SXF session control block.
                                        ** This is supplied for SXC_BGN_SESSION
                                        ** and SXC_END_SESSION when the
					** facility is running in
					** SXF_MULTI_USER mode.
                                        */
    CS_SID              sxf_sess_id;    /* Pointer to the session identifier
                                        ** for this thread. This must be 
                                        ** supplied for SXR_WRITE operations
                                        ** when the facility is started in 
                                        ** SXF_MULTI_USER mode.
                                        */
    i4                  sxf_force;      /* Used for SXC_SHUTDOWN and SXR_WRITE
                                        ** operations. For SXC_SHUTDOWN if
                                        ** this is supplied as non-zero, then
                                        ** SXF will attempt a forced shutdown
                                        ** by ignoring the fact that there might
                                        ** still be existing active sessions.
                                        ** For SXR_WRITE if this is supplied
                                        ** as non-zero, then the operation will
                                        ** not complete until the audit record
                                        ** has been flushed to disk.
                                        */
    i4                  sxf_scb_sz;     /* This is only used by SXC_STARTUP
                                        ** who returns it. It represents the
                                        ** number of bytes required for a SXF
                                        ** session control block.
                                        */
    i4                  sxf_max_active_ses; 
                                        /* Max number of active sessions allowed
                                        ** for the facility. This is only used 
                                        ** by SXC_STARTUP, where it is supplied.
					*/
    SIZE_TYPE           sxf_pool_sz;    /* Number of bytes to allocate for SXF's
                                        ** memory pool. This is only used by
                                        ** SXC_STARTUP, where it is supplied.
                                        ** If this number is zero or negative,
                                        ** it will be ignored, and
                                        ** .sxf_max_active_ses will be used to
                                        ** calculate the size of the memory 
                                        ** pool.
                                        */
    PTR                 sxf_filename;   /* Specifies the name for a file.
                                        ** Used by SXA_OPEN, and 
                                        ** SXS_SHOW.
                                        */
    PTR                 sxf_access_id;  /* Access identifier that is returned
                                        ** on SXA_OPEN calls, and must be 
                                        ** provided for subsequent SXR_READ,
                                        ** SXR_POSITION and SXC_CLOSE 
                                        ** operations.
                                        */
    SXF_AUDIT_REC      	*sxf_record;    /* Buffer containing audit record,
                                        ** used for SXR_READ operations.
                                        */
    DB_OWN_NAME         *sxf_objowner;  /* The owner of the object being
					** audited.
					*/
    char                *sxf_objname;   /* Name of object being audited. Used
                                        ** for all SXR_WRITE operations.
                                        */
    i4             sxf_objlength;  /* Length of .object_name. Used by
                                        ** the same operations.
                                        */
    i4             sxf_msg_id;     /* Id of the text associated with an
                                        ** audit record. Used for all SXR_WRITE
                                        ** operations.
                                        */
    i4		sxf_ustat;	/* The user status mask.
					** Must be specified for
					** SXC_BGN_SESSION operations when the
					** facility is started in single user
					** mode. And also for SXR_WRITE
					** operations when user records
					** have been modified.
					*/
    DB_OWN_NAME         *sxf_user;      /* The user performing the event being
					** audited. Must be specified for 
					** SXC_BGN_SESSION operations when the 
					** facility is started in single user
					** mode.
					*/
    DB_OWN_NAME         *sxf_ruser;     /* Name of the real user connected.
					** Must be specified for 
					** SXF_BGN_SESSION operations when 
					** the facility is started in single 
					** user mode.
					*/
    DB_DB_NAME		*sxf_db_name;   /* Name of the database being accessed.
					** Must be specified for 
					** SXF_BGN_SESSION operations when 
					** the facility is started in single 
					** user mode.
					*/
    SXF_ACCESS          sxf_accessmask; /* Type of access to be audited. Used
                                        ** for all SXR_WRITE operations.
                                        */
    SXF_EVENT           sxf_auditevent; /* Shows the type of security auditing
                                        ** event. Specified for all SXR_WRITE,
                                        ** SXS_SHOW, and SXS_ALTER operations.
                                        */
    i4                  sxf_auditstate; /* Specifies the audit state for the 
                                        ** event types above. Used for 
                                        ** SXS_ALTER and SXS_SHOW operations.
                                        */
# define        SXF_S_ENABLE            0x01
# define        SXF_S_DISABLE           0x02
# define        SXF_S_UNCHANGED         0x04
# define	SXF_S_STOP_AUDIT        0x08	/* Stop auditing */
# define	SXF_S_RESTART_AUDIT     0x010	/* Restart auditing */
# define	SXF_S_SUSPEND_AUDIT	0x020	/* Suspend auditing */
# define	SXF_S_RESUME_AUDIT	0x040	/* Resume auditing */
# define	SXF_S_CHANGE_FILE       0x080	/* New audit file */

    i4             sxf_file_size;  /* The maximum size of the security
                                        ** audit file in kilobytes. Used for 
                                        ** SXS_ALTER/SXS_SHOW/SXA_OPEN 
					** operations.
                                        */
    DB_ERROR            sxf_error;      /* Error code returned by all SXF
                                        ** operations.
                                        */
    DB_STATUS		(*sxf_dmf_cptr)();
					/* A pointer to the dmf_call function.
					** This is supplied a facility startup
					** when the facility is running in
					** multi-user mode.
					*/
    char	    *sxf_detail_txt;	/* Text of detail, may be NULL */
    i2		    sxf_detail_len;	/* Length of text detail */
    i4	      	    sxf_detail_int;     /* Integer detail */
    i4		    sxf_alter;		/* Alter operations */
# define	SXF_X_USERNAME	0x001	/* Effective user name */
# define	SXF_X_DBNAME    0x004	/* Database name */
# define	SXF_X_USERPRIV	0x008	/* User privilege mask */
# define	SXF_X_RUSERNAME 0x020	/* Real user name */

}   SXF_RCB;
/*
**  SXF use of Paramater Management
**
**  These describe the names of PM resources that are used within SXF
**  to provide basic administration defaults.
*/

/*
**  Should C2 Security Auditing be enabled ?
*/
# define	SX_C2_MODE			ERx("II.*.C2.SECURITY_AUDITING")
# define 	SX_C2_MODE_ON			ERx("ON")
# define 	SX_C2_MODE_OFF			ERx("OFF")

/*
** Which auditing method to use
*/
# define        SX_C2_AUDIT_METHOD              ERx("II.*.C2.AUDIT_MECHANISM")
# define        SX_C2_INGAUDIT                  ERx("INGRES")
# define        SX_C2_OSAUDIT                   ERx("SYSTEM")
/* old default audit method */
# define        SX_C2_OLDDEFAUDIT               ERx("CA")

/*
**  SXF error values.
**
**  These values are returned by SXF to clients.  Those errors which exist
**  with the most significant byte = 0 are 'normal;' other errors are primarily
**  meant as internal to SXF errors which will be logged and translated before
**  escaping to the client level, or as informational message values.
**
**  Note: _SX_CLASS (SXF Message Facility Code) = 16 = 0x10
*/
# define        E_SXF_MASK                      0x00100000
# define	E_SX0000_OK			0L
# define        E_SXF_EXTERNAL			(E_SXF_MASK + 0x0001L)
# define	E_SX0001_BAD_OP_CODE		(E_SXF_MASK + 0x0001L)
# define	E_SX0002_BAD_CB_TYPE		(E_SXF_MASK + 0x0002L)
# define	E_SX0003_BAD_CB_LENGTH		(E_SXF_MASK + 0x0003L)
# define	E_SX0004_BAD_PARAMETER		(E_SXF_MASK + 0x0004L)
# define	E_SX0005_INTERNAL_ERROR		(E_SXF_MASK + 0x0005L)
# define	E_SX0006_FACILITY_ACTIVE        (E_SXF_MASK + 0x0006L)
# define	E_SX0007_NO_ROWS	        (E_SXF_MASK + 0x0007L)
# define	E_SX0008_BAD_STARTUP	        (E_SXF_MASK + 0x0008L)
# define	E_SX0009_BEGIN_SESSION		(E_SXF_MASK + 0x0009L)
# define	E_SX000A_QUOTA_EXCEEDED		(E_SXF_MASK + 0x000AL)
# define	E_SX000B_SESSIONS_ACTIVE	(E_SXF_MASK + 0x000BL)
# define	E_SX000C_BAD_SHUTDOWN		(E_SXF_MASK + 0x000CL)
# define	E_SX000D_INVALID_SESSION	(E_SXF_MASK + 0x000DL)
# define	E_SX000E_BAD_SESSION_END	(E_SXF_MASK + 0x000EL)
# define	E_SX000F_SXF_NOT_ACTIVE		(E_SXF_MASK + 0x000FL)
# define	E_SX0010_SXA_NOT_ENABLED	(E_SXF_MASK + 0x0010L)
# define	E_SX0011_INVALID_ACCESS_ID	(E_SXF_MASK + 0x0011L)
# define	E_SX0012_FILE_NOT_OPEN		(E_SXF_MASK + 0x0012L)
# define	E_SX0013_FILE_NOT_POSITIONED	(E_SXF_MASK + 0x0013L)
# define	E_SX0014_BAD_POSITION_TYPE	(E_SXF_MASK + 0x0014L)
# define	E_SX0015_FILE_NOT_FOUND		(E_SXF_MASK + 0x0015L)
# define	E_SX0016_FILE_EXISTS		(E_SXF_MASK + 0x0016L)
# define	E_SX0017_SXAS_ALTER		(E_SXF_MASK + 0x0017L)
# define	E_SX0018_TABLE_NOT_FOUND	(E_SXF_MASK + 0x0018L)
# define	E_SX0019_MAC_NOT_AVAILABLE	(E_SXF_MASK + 0x0019L)
# define	E_SX001A_SOP_ERROR		(E_SXF_MASK + 0x001AL)
# define	E_SX001B_LABELMAP_NOT_FOUND 	(E_SXF_MASK + 0x001BL)
# define	E_SX001C_BAD_DB_LABEL		(E_SXF_MASK + 0x001CL)
# define	E_SX001D_BAD_USER_LABEL		(E_SXF_MASK + 0x001DL)
# define	E_SX001E_TRACE_FLAG_ERR		(E_SXF_MASK + 0x001EL)
# define	E_SX001F_DEADLOCK		(E_SXF_MASK + 0x001FL)
# define	E_SX0020_LOCK_ERROR		(E_SXF_MASK + 0x0020L)
# define	E_SX0021_ALREADY_SUSPENDED	(E_SXF_MASK + 0x0021L)
# define	E_SX0022_NOT_SUSPENDED		(E_SXF_MASK + 0x0022L)
# define	E_SX0023_SUSPEND_ERROR		(E_SXF_MASK + 0x0023L)
# define	E_SX0024_RESUME_ERROR		(E_SXF_MASK + 0x0024L)
# define	E_SX0025_SET_LOG_ERR            (E_SXF_MASK + 0x0025L)
# define	E_SX0026_USER_LOG_EXISTS	(E_SXF_MASK + 0x0026L)
# define	E_SX0027_USER_LOG_UNKNOWN	(E_SXF_MASK + 0x0027L)
# define	E_SX0028_SET_LOG_NEEDS_AUDIT	(E_SXF_MASK + 0x0028L)
# define	E_SX0029_STOP_ERROR		(E_SXF_MASK + 0x0029L)
# define	E_SX002A_RESTART_ERROR		(E_SXF_MASK + 0x002AL)
# define	E_SX002B_AUDIT_NOT_ACTIVE	(E_SXF_MASK + 0x002BL)
# define	E_SX002C_AUDIT_IS_ACTIVE	(E_SXF_MASK + 0x002CL)
# define        E_SX002D_OSAUDIT_NO_POSITION    (E_SXF_MASK + 0x002DL)
# define        E_SX002E_OSAUDIT_NO_READ        (E_SXF_MASK + 0x002EL)

/*
** Result codes 0500-0999
*/
# define	I_SX0500_EQUAL			(E_SXF_MASK + 0x0500L)
# define	I_SX0501_DISJOINT		(E_SXF_MASK + 0x0501L)
# define	I_SX0502_DOMINATES		(E_SXF_MASK + 0x0502L)
# define	I_SX0504_NOT_DOMINATES		(E_SXF_MASK + 0x0504L)
# define	I_SX0510_ALLOW_POLY		(E_SXF_MASK + 0x0510L)
/*
** Internal SXF errors. These errors will be logged internally, but
** they will not get returned to the user.
*/
# define	E_SXF_INTERNAL			(E_SXF_MASK + 0x1000L)
# define	E_SX1001_UNKNOWN_EXCEPTION	(E_SXF_MASK + 0x1001L)
# define	E_SX1002_NO_MEMORY	        (E_SXF_MASK + 0x1002L)
# define	E_SX1003_SEM_INIT	        (E_SXF_MASK + 0x1003L)
# define	E_SX1004_LOCK_CREATE		(E_SXF_MASK + 0x1004L)
# define	E_SX1005_SESSION_INFO		(E_SXF_MASK + 0x1005L)
# define	E_SX1006_SCB_BUILD		(E_SXF_MASK + 0x1006L)
# define	E_SX1007_BAD_SCB_DESTROY	(E_SXF_MASK + 0x1007L)
# define	E_SX1008_BAD_LOCK_RELEASE	(E_SXF_MASK + 0x1008L)
# define	E_SX1009_SXA_STARTUP		(E_SXF_MASK + 0x1009L)
# define	E_SX100A_SXA_SHUTDOWN		(E_SXF_MASK + 0x100AL)
# define	E_SX100B_SXA_BGN_SESSION	(E_SXF_MASK + 0x100BL)
# define	E_SX100C_SXA_END_SESSION	(E_SXF_MASK + 0x100CL)
# define	E_SX100D_SXA_OPEN		(E_SXF_MASK + 0x100DL)
# define	E_SX100E_SXA_CLOSE		(E_SXF_MASK + 0x100EL)
# define	E_SX100F_SXA_POSITION		(E_SXF_MASK + 0x100FL)
# define	E_SX1010_SXA_READ		(E_SXF_MASK + 0x1010L)
# define	E_SX1011_SXA_WRITE		(E_SXF_MASK + 0x1011L)
# define	E_SX1012_SXA_FLUSH		(E_SXF_MASK + 0x1012L)
# define	E_SX1013_SXA_SHOW_STATE		(E_SXF_MASK + 0x1013L)
# define	E_SX1014_SXA_GET_SCB		(E_SXF_MASK + 0x1014L)
# define	E_SX1015_SXA_RSCB_BUILD		(E_SXF_MASK + 0x1015L)
# define	E_SX1016_SXAP_MEM_FREE		(E_SXF_MASK + 0x1016L)
# define	E_SX1017_SXAP_SHM_INVALID	(E_SXF_MASK + 0x1017L)
# define	E_SX1018_FILE_ACCESS_ERROR	(E_SXF_MASK + 0x1018L)
# define	E_SX1019_BAD_LOCK_CONVERT	(E_SXF_MASK + 0x1019L)
# define	E_SX101A_SXAP_STARTUP		(E_SXF_MASK + 0x101AL)
# define	E_SX101B_SXAP_SHM_DESTROY	(E_SXF_MASK + 0x101BL)
# define	E_SX101C_SXAP_SHUTDOWN		(E_SXF_MASK + 0x101CL)
# define	E_SX101D_SXAP_BGN_SESSION	(E_SXF_MASK + 0x101DL)
# define	E_SX101E_INVALID_FILENAME	(E_SXF_MASK + 0x101EL)
# define	E_SX101F_SXAP_SHOW		(E_SXF_MASK + 0x101FL)
# define	E_SX1020_SXAP_INIT_CNF		(E_SXF_MASK + 0x1020L)
# define	E_SX1021_BAD_PAGE_TYPE		(E_SXF_MASK + 0x1021L)
# define	E_SX1022_BAD_FILE_VERSION	(E_SXF_MASK + 0x1022L)
# define	E_SX1023_SXAP_OPEN		(E_SXF_MASK + 0x1023L)
# define	E_SX1024_BAD_FILE_ALLOC		(E_SXF_MASK + 0x1024L)
# define	E_SX1025_SXAP_CREATE		(E_SXF_MASK + 0x1025L)
# define	E_SX1026_SXAP_CLOSE		(E_SXF_MASK + 0x1026L)
# define	E_SX1027_BAD_FILE_MODE		(E_SXF_MASK + 0x1027L)
# define	E_SX1028_SXAP_POSITION		(E_SXF_MASK + 0x1028L)
# define	E_SX1029_INVALID_PAGENO		(E_SXF_MASK + 0x1029L)
# define	E_SX102A_END_OF_FILE		(E_SXF_MASK + 0x102AL)
# define	E_SX102B_SXAP_READ		(E_SXF_MASK + 0x102BL)
# define	E_SX102C_BAD_FILE_CONTEXT	(E_SXF_MASK + 0x102CL)
# define	E_SX102D_SXAP_STOP_AUDIT	(E_SXF_MASK + 0x102DL)
# define	E_SX102E_SXAP_WRITE		(E_SXF_MASK + 0x102EL)
# define	E_SX102F_SXAP_FLUSH		(E_SXF_MASK + 0x102FL)
# define	E_SX1030_SXAP_WRITE_PAGE	(E_SXF_MASK + 0x1030L)
# define	E_SX1031_BAD_CHECKSUM		(E_SXF_MASK + 0x1031L)
# define	E_SX1032_SXAP_READ_PAGE		(E_SXF_MASK + 0x1032L)
# define	E_SX1033_SXAP_ALLOC_PAGE	(E_SXF_MASK + 0x1033L)
# define	E_SX1034_SXAP_SHM_CREATE	(E_SXF_MASK + 0x1034L)
# define	E_SX1035_BAD_LOCK_REQUEST	(E_SXF_MASK + 0x1035L)
# define	E_SX1036_AUDIT_FILE_ERROR	(E_SXF_MASK + 0x1036L)
# define	E_SX1037_ONERR_SERVER_SHUTDOWN	(E_SXF_MASK + 0x1037L)
# define	E_SX1038_BEGIN_XACT_ERROR	(E_SXF_MASK + 0x1038L)
# define	E_SX1039_BAD_SECCAT_OPEN	(E_SXF_MASK + 0x1039L)
# define	E_SX103A_BAD_SECCAT_READ	(E_SXF_MASK + 0x103AL)
# define	E_SX103B_BAD_SECCAT_REPLACE	(E_SXF_MASK + 0x103BL)
# define	E_SX103C_XACT_COMMIT_ERROR	(E_SXF_MASK + 0x103CL)
# define	E_SX103D_XACT_ABORT_ERROR	(E_SXF_MASK + 0x103DL)
# define	E_SX103E_BAD_SECCAT_CLOSE	(E_SXF_MASK + 0x103EL)
# define	E_SX103F_AUDIT_THREAD_ERROR	(E_SXF_MASK + 0x103FL)
# define	E_SX1040_SXAS_CHECK_STATE	(E_SXF_MASK + 0x1040L)
# define	E_SX1041_INVALID_EVENT_TYPE	(E_SXF_MASK + 0x1041L)
# define	E_SX1042_BAD_ROW_COUNT		(E_SXF_MASK + 0x1042L)
# define	E_SX1043_SXAS_CACHE_INIT	(E_SXF_MASK + 0x1043L)
# define	E_SX1044_SXAS_STARTUP		(E_SXF_MASK + 0x1044L)
# define	E_SX1045_BAD_EVENT_SET		(E_SXF_MASK + 0x1045L)
# define	E_SX1046_BAD_EVENT_WAIT		(E_SXF_MASK + 0x1046L)
# define	E_SX1047_BAD_EVENT_CLEAR	(E_SXF_MASK + 0x1047L)
# define	E_SX1048_SERVER_TERMINATE	(E_SXF_MASK + 0x1048L)
# define	E_SX1049_AUDIT_DISABLED		(E_SXF_MASK + 0x1049L)
# define        E_SX104A_SXAP_BAD_MAXFILE       (E_SXF_MASK + 0x104AL)
# define        E_SX104B_SXAP_NO_LOGNAMES       (E_SXF_MASK + 0x104BL)
# define        E_SX104C_SXAP_NEED_AUDITLOGS    (E_SXF_MASK + 0x104CL)
# define        E_SX104D_SXAP_BAD_ONLOGFULL     (E_SXF_MASK + 0x104DL)
# define        E_SX104E_SXAP_BAD_ONSWITCHLOG   (E_SXF_MASK + 0x104EL)
# define        E_SX104F_SXAP_FINDCUR_OPER      (E_SXF_MASK + 0x104FL)
# define        E_SX1050_SXAP_FINDCUR_LOGCHK    (E_SXF_MASK + 0x1050L)
# define        E_SX1051_SXAP_FINDCUR_BADGET    (E_SXF_MASK + 0x1051L)
# define        E_SX1052_SHUTDOWN_LOGFULL       (E_SXF_MASK + 0x1052L)
# define        E_SX1053_SHUTDOWN_NOW           (E_SXF_MASK + 0x1053L)
# define        E_SX1054_SXAP_CHANGE_CLOSE      (E_SXF_MASK + 0x1054L)
# define        E_SX1055_SXAP_CHANGE_OPEN       (E_SXF_MASK + 0x1055L)
# define        I_SX1056_CHANGE_AUDIT_LOG       (E_SXF_MASK + 0x1056L)
# define        E_SX1057_SXAP_CHANGE_NEXT       (E_SXF_MASK + 0x1057L)
# define        E_SX1058_SXAP_CHANGE_LOG_ERR    (E_SXF_MASK + 0x1058L)
# define        E_SX1059_SXAP_CHANGE_CREATE     (E_SXF_MASK + 0x1059L)
# define 	E_SX105A_SXAP_CHANGE_PARAM	(E_SXF_MASK + 0x105AL)
# define 	E_SX105B_SXAP_CHK_CONF_ERR      (E_SXF_MASK + 0x105BL)
# define 	E_SX105C_SXAP_CHK_PROTOCOL	(E_SXF_MASK + 0x105CL)
# define	I_SX105D_RUN_ONSWITCHPROG	(E_SXF_MASK + 0x105DL)
# define	E_SX105E_RUN_SWITCH_TOOMANY     (E_SXF_MASK + 0x105EL)
# define	E_SX105F_RUN_SWITCH_ERROR       (E_SXF_MASK + 0x105FL)
# define	E_SX1060_SXAP_BAD_ONERROR	(E_SXF_MASK + 0x1060L)
# define        E_SX1061_SXAP_BAD_MODE          (E_SXF_MASK + 0x1061L)
# define	E_SX1062_SCF_SET_ACTIVITY	(E_SXF_MASK + 0x1062L)
# define	E_SX1063_SCF_CLR_ACTIVITY	(E_SXF_MASK + 0x1063L)
# define        E_SX1064_CHK_CNF_NULL_LKVAL     (E_SXF_MASK + 0x1064L)
# define	E_SX1065_BAD_REC_LENGTH		(E_SXF_MASK + 0x1065L)
# define	E_SX1066_BUF_TOO_SHORT		(E_SXF_MASK + 0x1066L)
# define        E_SX1067_AUD_THREAD_NO_SCB	(E_SXF_MASK + 0x1067L)
# define        E_SX1068_AUD_THREAD_IIDBDB_ERR  (E_SXF_MASK + 0x1068L)
# define        W_SX1069_AUD_THREAD_NO_IIDBDB   (E_SXF_MASK + 0x1069L)
# define        E_SX106A_BAD_POOL_SIZE          (E_SXF_MASK + 0x106AL)
# define        E_SX106B_MEMORY_ERROR           (E_SXF_MASK + 0x106BL)
# define        E_SX106C_SXM_ACCESS_FAILURE     (E_SXF_MASK + 0x106CL)
# define        E_SX106D_SXM_WRITE_FAILURE      (E_SXF_MASK + 0x106DL)
# define        E_SX106E_SXM_WRITE_ACCESS       (E_SXF_MASK + 0x106EL)
# define        E_SX106F_SXM_ARBITRATE_ERR      (E_SXF_MASK + 0x106FL)
# define        E_SX1070_SXM_ARB_SUBJECT_LABEL  (E_SXF_MASK + 0x1070L)
# define        E_SX1071_SXM_ARB_OBJECT_LABEL   (E_SXF_MASK + 0x1071L)
# define        E_SX1072_SXM_DEF_SUBJECT_LABEL  (E_SXF_MASK + 0x1072L)
# define        E_SX1073_SXM_DEF_LABEL          (E_SXF_MASK + 0x1073L)
# define        E_SX1074_SXM_DEF_LABEL_PARM     (E_SXF_MASK + 0x1074L)
# define	E_SX1075_SXM_DELDOWN_UNDEF	(E_SXF_MASK + 0x1075L)
# define	E_SX1076_PARM_ERROR		(E_SXF_MASK + 0x1076L)
# define	E_SX1077_SXC_GETDBCB	        (E_SXF_MASK + 0x1077L)
# define	E_SX1078_SXL_SLCMP_ERROR	(E_SXF_MASK + 0x1078L)
# define	E_SX1079_SXL_FIND_ERROR		(E_SXF_MASK + 0x1079L)
# define	E_SX107A_SXC_ALTER_SESSION	(E_SXF_MASK + 0x107AL)
# define	E_SX107B_SXL_LI_EXPAND_TYPE	(E_SXF_MASK + 0x107BL)
# define	E_SX107C_SXL_COMPARE_ERROR	(E_SXF_MASK + 0x107CL)
# define	E_SX107D_B1_NEEDS_DELETE_DOWN   (E_SXF_MASK + 0x107DL)
# define	E_SX107E_INVALID_DELETE_DOWN    (E_SXF_MASK + 0x107EL)
# define 	E_SX107F_SXM_ENCAPSULATE	(E_SXF_MASK + 0x107FL)
# define	E_SX1080_SXM_DISJ_ING_HIGH	(E_SXF_MASK + 0x1080L)
# define	E_SX1081_SXM_DISJ_ING_LOW	(E_SXF_MASK + 0x1081L)
# define	E_SX1082_SXM_DISJ_DB_LABEL	(E_SXF_MASK + 0x1082L)
# define	E_SX1083_B1_NEEDS_CA_HIGH	(E_SXF_MASK + 0x1083L)
# define	E_SX1084_B1_NEEDS_CA_LOW	(E_SXF_MASK + 0x1084L)
# define	E_SX1085_BAD_CA_HIGH		(E_SXF_MASK + 0x1085L)
# define	E_SX1086_BAD_CA_LOW		(E_SXF_MASK + 0x1086L)
# define	E_SX1087_BAD_PROC_SECLABEL	(E_SXF_MASK + 0x1087L)
# define	E_SX1088_PROC_SECLABEL_CAHIGH	(E_SXF_MASK + 0x1088L)
# define	E_SX1089_BAD_SECID_OPEN		(E_SXF_MASK + 0x1089L)
# define	E_SX108A_BAD_SECID_CLOSE	(E_SXF_MASK + 0x108AL)
# define	E_SX108B_SXL_GETID		(E_SXF_MASK + 0x108BL)
# define	E_SX108C_BAD_SECID_POSITION	(E_SXF_MASK + 0x108CL)
# define	E_SX108D_BAD_SECID_FETCH	(E_SXF_MASK + 0x108DL)
# define	E_SX108E_BAD_SECID_INSERT	(E_SXF_MASK + 0x108EL)
# define	E_SX108F_BAD_SECID_DELETE	(E_SXF_MASK + 0x108FL)
# define	E_SX1090_SXL_GETLABEL		(E_SXF_MASK + 0x1090L)
# define	E_SX1091_SXT_UBEGIN  		(E_SXF_MASK + 0x1091L)
# define	E_SX1092_SXT_UEND  		(E_SXF_MASK + 0x1092L)
# define	E_SX1093_SXL_NEW_ID		(E_SXF_MASK + 0x1093L)
# define	E_SX1094_SXL_DELETE		(E_SXF_MASK + 0x1094L)
# define	E_SX1096_SXC_SOP_ADD		(E_SXF_MASK + 0x1096L)
# define	E_SX1097_SXL_COLLATE_ERROR	(E_SXF_MASK + 0x1097L)
# define	E_SX1098_SXL_SLCOLL_ERROR	(E_SXF_MASK + 0x1098L)
# define	E_SX1099_SXLC_FIND_LABINFO      (E_SXF_MASK + 0x1099L)
# define	E_SX109A_SXLC_ADD_LABINFO       (E_SXF_MASK + 0x109AL)
# define	E_SX109B_HASH_ERROR       	(E_SXF_MASK + 0x109BL)
# define	E_SX109C_SXL_ID_NOT_FOUND	(E_SXF_MASK + 0x109CL)
# define	E_SX109D_SXL_INVALID_LABEL	(E_SXF_MASK + 0x109DL)
# define	E_SX109E_SXL_EXPAND_LABINFO	(E_SXF_MASK + 0x109EL)
# define	E_SX109F_SXL_LAB_TO_ID	        (E_SXF_MASK + 0x109FL)
# define	E_SX10A0_SXL_ID_TO_LAB	        (E_SXF_MASK + 0x10A0L)
# define	E_SX10A1_SXF_EXCEPTION	        (E_SXF_MASK + 0x10A1L)
# define	E_SX10A2_SXL_LUB_ERR		(E_SXF_MASK + 0x10A2L)
# define	E_SX10A3_SXAP_BAD_SECLABEL	(E_SXF_MASK + 0x10A3L)
# define	E_SX10A4_SXS_AUD_LABEL_LIMIT    (E_SXF_MASK + 0x10A4L)
# define        E_SX10A5_SXLC_INIT_DBCB		(E_SXF_MASK + 0x10A5L)
# define	E_SX10A6_STOPAUDIT_LOGFULL	(E_SXF_MASK + 0x10A6L)
# define	E_SX10A7_NO_CURRENT_FILE	(E_SXF_MASK + 0x10A7L)
# define	E_SX10A8_SXLC_FIND_CMP		(E_SXF_MASK + 0x10A8L)
# define	E_SX10A9_SXLC_ADD_CMP		(E_SXF_MASK + 0x10A9L)
# define	I_SX10AA_AUDIT_SUSPEND		(E_SXF_MASK + 0x10AAL)
# define	I_SX10AB_AUDIT_USER_STOP	(E_SXF_MASK + 0x10ABL)
# define	I_SX10AC_AUDIT_RESUME		(E_SXF_MASK + 0x10ACL)
# define	I_SX10AD_AUDIT_RESTART		(E_SXF_MASK + 0x10ADL)
# define	E_SX10AE_SXAP_AUDIT_SUSPEND	(E_SXF_MASK + 0x10AEL)
# define	E_SX10AF_SXAP_STOP_CLOSE_ERR	(E_SXF_MASK + 0x10AFL)
# define	E_SX10B0_SXAP_RESTART_INIT_ERR	(E_SXF_MASK + 0x10B0L)
# define	E_SX10B1_SXAP_RESTART_CREATE	(E_SXF_MASK + 0x10B1L)
# define	E_SX10B2_SXAP_RESTART_OPEN	(E_SXF_MASK + 0x10B2L)
# define	E_SX10B3_INSERT_SECID_DUP_KEY	(E_SXF_MASK + 0x10B3L)
# define        E_SX10B4_OSAUDIT_WRITE_FAILED   (E_SXF_MASK + 0x10B4L)
# define        E_SX10B5_OSAUDIT_FLUSH_FAILED   (E_SXF_MASK + 0x10B5L)
# define        E_SX10B6_OSAUDIT_OPEN_FAILED    (E_SXF_MASK + 0x10B6L)
# define        E_SX10B7_BAD_AUDIT_METHOD       (E_SXF_MASK + 0x10B7L)
# define	E_SX10B8_OSAUDIT_NO_CHANGEFILE	(E_SXF_MASK + 0x10B8L)
# define	E_SX10B9_BAD_MO_ATTACH	 	(E_SXF_MASK + 0x10B9L)
# define	E_SX10BA_OSAUDIT_NOT_ENABLED	(E_SXF_MASK + 0x10BAL)
# define	E_SX10BB_SXAP_CHK_PROTO_INFO    (E_SXF_MASK + 0x10BBL)

# define	E_SX10BC_SXAP_SHM_BAD_VER       (E_SXF_MASK + 0x10BCL)
# define	E_SX10BD_SXAP_SHM_BUF_COUNT     (E_SXF_MASK + 0x10BDL)
# define	E_SX10BE_SXAP_SHM_LOCK_ERR      (E_SXF_MASK + 0x10BEL)
# define	E_SX10BF_SXAP_SHM_UNLOCK_ERR    (E_SXF_MASK + 0x10BFL)
# define	E_SX10C0_SXAP_QWRITE_ERR        (E_SXF_MASK + 0x10C0L)
# define	E_SX10C1_SXAP_QFLUSH_ERR        (E_SXF_MASK + 0x10C1L)
# define	E_SX10C2_SXAP_QFLUSH_BUF_FREE   (E_SXF_MASK + 0x10C2L)
# define	E_SX10C3_SXAP_QALLFLUSH_ERR     (E_SXF_MASK + 0x10C3L)
# define	E_SX10C4_SXAP_QALLFLUSH_BUF     (E_SXF_MASK + 0x10C4L)
# define	E_SX10C5_AUDIT_WRITER_NO_SCB	(E_SXF_MASK + 0x10C5L)
# define	E_SX10C6_AUDIT_WRITER_ERROR	(E_SXF_MASK + 0x10C6L)
# define	E_SX10C7_AUDIT_WRITE_ERROR	(E_SXF_MASK + 0x10C7L)
# define	E_SX10C8_AUDIT_WRITE_BUF_FREE	(E_SXF_MASK + 0x10C8L)
# define	E_SX10C9_INVALID_WRITE_FIXED	(E_SXF_MASK + 0x10C9L)
# define	E_SX10CA_INVALID_UPDATE_FLOAT	(E_SXF_MASK + 0x10CAL)
# define	E_SX10CB_SXAP_SHM_PG_SIZE	(E_SXF_MASK + 0x10CBL)
# define	E_SX10CC_SXL_LABEL_ID_INFO      (E_SXF_MASK + 0x10CCL)

/*
** Security Auditing message codes
*/
# define I_SX2001_ROLE_CREATE           (E_SXF_MASK + 0x02001)
# define I_SX2002_ROLE_DROP             (E_SXF_MASK + 0x02002)
# define I_SX2003_DBACCESS_CREATE       (E_SXF_MASK + 0x02003)
# define I_SX2004_DBACCESS_DROP         (E_SXF_MASK + 0x02004)
# define I_SX2005_DBPRIV_CREATE         (E_SXF_MASK + 0x02005)
# define I_SX2006_DBPRIV_DROP           (E_SXF_MASK + 0x02006)
# define I_SX2007_GROUP_CREATE          (E_SXF_MASK + 0x02007)
# define I_SX2008_GROUP_MEM_CREATE      (E_SXF_MASK + 0x02008)
# define I_SX2009_GROUP_DROP            (E_SXF_MASK + 0x02009)
# define I_SX200A_GROUP_MEM_DROP        (E_SXF_MASK + 0x0200a)
# define I_SX200B_LOCATION_CREATE       (E_SXF_MASK + 0x0200b)
# define I_SX200C_USER_CREATE           (E_SXF_MASK + 0x0200c)
# define I_SX200D_USER_DROP             (E_SXF_MASK + 0x0200d)
# define I_SX2010_INDEX_DROP		(E_SXF_MASK + 0x02010)
# define I_SX2011_INDEX_CREATE		(E_SXF_MASK + 0x02011)
# define I_SX2012_DBPROC_CREATE         (E_SXF_MASK + 0x02012)
# define I_SX2013_DBPROC_DROP           (E_SXF_MASK + 0x02013)
# define I_SX2014_VIEW_CREATE           (E_SXF_MASK + 0x02014)
# define I_SX2015_VIEW_DROP             (E_SXF_MASK + 0x02015)
# define I_SX2016_PROT_TAB_CREATE       (E_SXF_MASK + 0x02016)
# define I_SX2017_PROT_DBP_CREATE       (E_SXF_MASK + 0x02017)
# define I_SX2018_PROT_TAB_DROP         (E_SXF_MASK + 0x02018)
# define I_SX2019_PROT_DBP_DROP         (E_SXF_MASK + 0x02019)
# define I_SX201A_DATABASE_ACCESS       (E_SXF_MASK + 0x0201a)
# define I_SX201B_ROLE_ACCESS           (E_SXF_MASK + 0x0201b)
# define I_SX201C_SEC_CAT_UPDATE        (E_SXF_MASK + 0x0201c)
# define I_SX201D_DBPROC_EXECUTE        (E_SXF_MASK + 0x0201d)
# define I_SX201E_TABLE_CREATE          (E_SXF_MASK + 0x0201e)
# define I_SX201F_DATABASE_EXTEND       (E_SXF_MASK + 0x0201f)
# define I_SX2020_PROT_REJECT           (E_SXF_MASK + 0x02020)
# define I_SX2021_ROLE_ALTER            (E_SXF_MASK + 0x02021)
# define I_SX2022_GROUP_ALTER           (E_SXF_MASK + 0x02022)
# define I_SX2023_USER_ALTER            (E_SXF_MASK + 0x02023)
# define I_SX2024_USER_ACCESS           (E_SXF_MASK + 0x02024)
# define I_SX2025_TABLE_DROP            (E_SXF_MASK + 0x02025)
# define I_SX2026_TABLE_ACCESS          (E_SXF_MASK + 0x02026)
# define I_SX2027_VIEW_ACCESS           (E_SXF_MASK + 0x02027)
# define I_SX2028_ALARM_EVENT           (E_SXF_MASK + 0x02028)
# define I_SX2029_DATABASE_CREATE       (E_SXF_MASK + 0x02029)
# define I_SX202A_DATABASE_DROP         (E_SXF_MASK + 0x0202a)
# define I_SX202B_LOCATION_ALTER        (E_SXF_MASK + 0x0202b)
# define I_SX202C_LOCATION_DROP         (E_SXF_MASK + 0x0202c)
# define I_SX202D_ALARM_CREATE          (E_SXF_MASK + 0x0202d)
# define I_SX202E_ALARM_DROP            (E_SXF_MASK + 0x0202e)
# define I_SX202F_RULE_ACCESS           (E_SXF_MASK + 0x0202f)
# define I_SX2030_PROT_EV_CREATE        (E_SXF_MASK + 0x02030)
# define I_SX2031_PROT_EV_DROP          (E_SXF_MASK + 0x02031)
# define I_SX2032_EVENT_CREATE          (E_SXF_MASK + 0x02032)
# define I_SX2033_SET_USER_AUTH         (E_SXF_MASK + 0x02033)
# define I_SX2034_SET_GROUP		(E_SXF_MASK + 0x02034)
# define I_SX2036_INTEGRITY_DROP        (E_SXF_MASK + 0x02036)
# define I_SX2037_RULE_CREATE		(E_SXF_MASK + 0x02037)
# define I_SX2038_RULE_DROP		(E_SXF_MASK + 0x02038)
# define I_SX2039_TBL_COMMENT		(E_SXF_MASK + 0x02039)
# define I_SX203A_TBL_CONSTRAINT_ADD	(E_SXF_MASK + 0x0203A)
# define I_SX203B_TBL_CONSTRAINT_DROP	(E_SXF_MASK + 0x0203B)
# define I_SX203C_EVENT_DROP		(E_SXF_MASK + 0x0203C)
# define I_SX203D_INTEGRITY_CREATE	(E_SXF_MASK + 0x0203D)
# define I_SX203E_SCHEMA_CREATE		(E_SXF_MASK + 0x0203E)
# define I_SX203F_SCHEMA_DROP		(E_SXF_MASK + 0x0203F)
# define I_SX2040_TMP_TBL_CREATE	(E_SXF_MASK + 0x02040)
# define I_SX2041_TMP_TBL_DROP		(E_SXF_MASK + 0x02041)
# define I_SX2042_REMOVE_TABLE		(E_SXF_MASK + 0x02042)
# define I_SX2043_GW_REGISTER		(E_SXF_MASK + 0x02043)
# define I_SX2044_GW_REMOVE		(E_SXF_MASK + 0x02044)
# define I_SX2045_SEQUENCE_CREATE       (E_SXF_MASK + 0x02045)
# define I_SX2046_SEQUENCE_ALTER        (E_SXF_MASK + 0x02046)
# define I_SX2047_SEQUENCE_DROP         (E_SXF_MASK + 0x02047)
# define I_SX2048_SEQUENCE_NEXT         (E_SXF_MASK + 0x02048)
# define I_SX2049_PROT_SEQ_CREATE       (E_SXF_MASK + 0x02049)
# define I_SX204A_PROT_SEQ_DROP         (E_SXF_MASK + 0x0204A)
# define I_SX201F_DATABASE_UNEXTEND     (E_SXF_MASK + 0x0204B)

# define I_SX2501_CHANGE_AUDIT_PROFILE   (E_SXF_MASK + 0x02501)
# define I_SX2502_ENAB_AUDIT_ALL         (E_SXF_MASK + 0x02502)
# define I_SX2503_DISB_AUDIT_ALL         (E_SXF_MASK + 0x02503)
# define I_SX2504_ENAB_AUDIT_TABLE       (E_SXF_MASK + 0x02504)
# define I_SX2505_DISB_AUDIT_TABLE       (E_SXF_MASK + 0x02505)
# define I_SX2506_ENAB_AUDIT_VIEW        (E_SXF_MASK + 0x02506)
# define I_SX2507_DISB_AUDIT_VIEW        (E_SXF_MASK + 0x02507)
# define I_SX2508_ENAB_AUDIT_DATABASE    (E_SXF_MASK + 0x02508)
# define I_SX2509_DISB_AUDIT_DATABASE    (E_SXF_MASK + 0x02509)
# define I_SX250A_ENAB_AUDIT_USER        (E_SXF_MASK + 0x0250a)
# define I_SX250B_DISB_AUDIT_USER        (E_SXF_MASK + 0x0250b)
# define I_SX250C_ENAB_AUDIT_PROCEDURE   (E_SXF_MASK + 0x0250c)
# define I_SX250D_DISB_AUDIT_PROCEDURE   (E_SXF_MASK + 0x0250d)
# define I_SX250E_ENAB_AUDIT_SECURITY    (E_SXF_MASK + 0x0250e)
# define I_SX250F_DISB_AUDIT_SECURITY    (E_SXF_MASK + 0x0250f)
# define I_SX2510_ENAB_AUDIT_ALARM       (E_SXF_MASK + 0x02510)
# define I_SX2511_DISB_AUDIT_ALARM       (E_SXF_MASK + 0x02511)
# define I_SX2512_ENAB_AUDIT_DBEVENT     (E_SXF_MASK + 0x02512)
# define I_SX2513_DISB_AUDIT_DBEVENT     (E_SXF_MASK + 0x02513)
# define I_SX2514_ENAB_AUDIT_RULE	 (E_SXF_MASK + 0x02514)
# define I_SX2515_DISB_AUDIT_RULE	 (E_SXF_MASK + 0x02515)
# define I_SX2516_ENAB_AUDIT_LOCATION	 (E_SXF_MASK + 0x02516)
# define I_SX2517_DISB_AUDIT_LOCATION	 (E_SXF_MASK + 0x02517)
# define I_SX2518_ENQUIRE_AUDIT_LOG	 (E_SXF_MASK + 0x02518)
# define I_SX2519_ENAB_AUDIT_RESOURCE    (E_SXF_MASK + 0x02519)
# define I_SX251A_DISB_AUDIT_RESOURCE    (E_SXF_MASK + 0x0251A)
# define I_SX251B_ENAB_AUDIT_QRYTEXT     (E_SXF_MASK + 0x0251B)
# define I_SX251C_DISB_AUDIT_QRYTEXT     (E_SXF_MASK + 0x0251C)

# define I_SX2701_REGISTER_AUDIT_SECURITY (E_SXF_MASK + 0x2701)
# define I_SX2702_QUERY_AUDIT_SECURITY   (E_SXF_MASK + 0x2702)
# define I_SX2703_REGISTER_AUDIT_LOG     (E_SXF_MASK + 0x2703)
# define I_SX2704_RAISE_DBEVENT          (E_SXF_MASK + 0x2704)
# define I_SX2705_REGISTER_DBEVENT       (E_SXF_MASK + 0x2705)
# define I_SX2706_REMOVE_DBEVENT         (E_SXF_MASK + 0x2706)
# define I_SX2707_USER_TERMINATE	 (E_SXF_MASK + 0x2707)
# define I_SX2708_USER_MONITOR	         (E_SXF_MASK + 0x2708)
# define I_SX2709_AUDITDB                (E_SXF_MASK + 0x2709)
# define I_SX270A_AUDITDB_TABLE          (E_SXF_MASK + 0x270A)
# define I_SX270B_CKPDB          	 (E_SXF_MASK + 0x270B)
# define I_SX270C_ROLLDB          	 (E_SXF_MASK + 0x270C)
# define I_SX270D_ALTERDB          	 (E_SXF_MASK + 0x270D)
# define I_SX270E_LOCK_ESCALATE          (E_SXF_MASK + 0x270E)
# define I_SX270F_TABLE_MODIFY           (E_SXF_MASK + 0x270F)
# define I_SX2710_TABLE_RELOCATE         (E_SXF_MASK + 0x2710)
# define I_SX2711_INFODB                 (E_SXF_MASK + 0x2711)
# define I_SX2712_CKPDB_ENABLE_JNL       (E_SXF_MASK + 0x2712)
# define I_SX2713_CKPDB_DISABLE_JNL      (E_SXF_MASK + 0x2713)
# define I_SX2714_ALTERDB_NOJNL          (E_SXF_MASK + 0x2714)
# define I_SX2715_CKPDB_TABLE_JNLON      (E_SXF_MASK + 0x2715)
# define I_SX2716_CKPDB_TABLE_JNLOFF     (E_SXF_MASK + 0x2716)
# define I_SX2717_TABLE_JNLON            (E_SXF_MASK + 0x2717)
# define I_SX2718_TABLE_JNLOFF           (E_SXF_MASK + 0x2718)
# define I_SX2719_TABLE_SAVE             (E_SXF_MASK + 0x2719)
# define I_SX271A_TABLE_PATCH            (E_SXF_MASK + 0x271A)
# define I_SX271B_DELETE_DOWN		 (E_SXF_MASK + 0x271B)
# define I_SX271C_WRITE_DOWN		 (E_SXF_MASK + 0x271C)
# define I_SX271D_WRITE_UP		 (E_SXF_MASK + 0x271D)
# define I_SX271E_INSERT_UP		 (E_SXF_MASK + 0x271E)
# define I_SX271F_INSERT_DOWN		 (E_SXF_MASK + 0x271F)
# define I_SX2720_WRITE_FIXED		 (E_SXF_MASK + 0x2720)
# define I_SX2721_WRITE_DISJOINT	 (E_SXF_MASK + 0x2721)
# define I_SX2722_RECORD_ACCESS		 (E_SXF_MASK + 0x2722)
# define I_SX2723_RECBYTID_ACCESS	 (E_SXF_MASK + 0x2723)
# define I_SX2724_RECBYKEY_ACCESS	 (E_SXF_MASK + 0x2724)
# define I_SX2725_SET_SESS_SECLABEL	 (E_SXF_MASK + 0x2725)
# define I_SX2726_SET_SESS_PRIV		 (E_SXF_MASK + 0x2726)
# define I_SX2727_USER_EXPIRE		 (E_SXF_MASK + 0x2727)
# define I_SX2728_LIMIT_SECLABEL         (E_SXF_MASK + 0x2728)
# define I_SX2729_QUERY_TEXT             (E_SXF_MASK + 0x2729)
# define I_SX272A_TBL_SYNONYM_CREATE	 (E_SXF_MASK + 0x272A)
# define I_SX272B_TBL_SYNONYM_DROP	 (E_SXF_MASK + 0x272B)
# define I_SX272C_TBL_COMMENT	 	 (E_SXF_MASK + 0x272C)
# define I_SX272D_ALTER_SECAUDIT 	 (E_SXF_MASK + 0x272D)
# define I_SX272E_CREATE_PROFILE	 (E_SXF_MASK + 0x272E)
# define I_SX272F_ALTER_PROFILE	 	 (E_SXF_MASK + 0x272F)
# define I_SX2730_DROP_PROFILE	 	 (E_SXF_MASK + 0x2730)
# define I_SX2731_QUERY_ROW_LIMIT	 (E_SXF_MASK + 0x2731)
# define I_SX2732_QUERY_CPU_LIMIT	 (E_SXF_MASK + 0x2732)
# define I_SX2733_QUERY_DIO_LIMIT	 (E_SXF_MASK + 0x2733)
# define I_SX2734_QUERY_PAGE_LIMIT	 (E_SXF_MASK + 0x2734)
# define I_SX2735_QUERY_COST_LIMIT	 (E_SXF_MASK + 0x2735)
# define I_SX2736_TABLE_DEADLOCK	 (E_SXF_MASK + 0x2736)
# define I_SX2737_PAGE_DEADLOCK		 (E_SXF_MASK + 0x2737)
# define I_SX2738_LOCK_TIMEOUT		 (E_SXF_MASK + 0x2738)
# define I_SX2739_LOCK_LIMIT		 (E_SXF_MASK + 0x2739)
# define I_SX273A_ALTER_DATABASE	 (E_SXF_MASK + 0x273A)
# define I_SX273B_QUERY_SYSCAT	 	 (E_SXF_MASK + 0x273B)
# define I_SX273C_TABLE_STATS		 (E_SXF_MASK + 0x273C)
# define I_SX273D_ENQUIRE_AUDIT_STATE    (E_SXF_MASK + 0x273D)
# define I_SX273E_SET_SESS_PRIORITY      (E_SXF_MASK + 0x273E)
# define I_SX273F_SET_SESSION_LIMIT      (E_SXF_MASK + 0x273F)
# define I_SX2740_SET_QUERY_LIMIT	 (E_SXF_MASK + 0x2740)
# define I_SX2741_SET_LOCKMODE	 	 (E_SXF_MASK + 0x2741)
# define I_SX2742_EVENT_ALARM		 (E_SXF_MASK + 0x2742)
# define I_SX2743_DB_ALARM 		 (E_SXF_MASK + 0x2743)
# define I_SX2744_SESSION_IDLE_LIMIT 	 (E_SXF_MASK + 0x2744)
# define I_SX2745_SESSION_CONNECT_LIMIT  (E_SXF_MASK + 0x2745)
# define I_SX2746_SESSION_ROLE           (E_SXF_MASK + 0x2746)
# define I_SX2747_DUMPDB                 (E_SXF_MASK + 0x2747)
# define I_SX2748_SET_SERVER_CLOSED	 (E_SXF_MASK + 0x2748)
# define I_SX2749_SET_SERVER_OPEN	 (E_SXF_MASK + 0x2749)
# define I_SX274A_STOP_SERVER	 	 (E_SXF_MASK + 0x274A)
# define I_SX274B_REMOVE_SESSION	 (E_SXF_MASK + 0x274B)
# define I_SX274C_CRASH_SESSION	 	 (E_SXF_MASK + 0x274C)
# define I_SX274D_SET_SERVER_SHUT	 (E_SXF_MASK + 0x274D)
# define I_SX274E_ROLE_GRANT	 	 (E_SXF_MASK + 0x274E)
# define I_SX274F_ROLE_REVOKE	 	 (E_SXF_MASK + 0x274F)
# define I_SX2750_SET_TRACE_POINT	 (E_SXF_MASK + 0x2750)
# define I_SX2751_SET_NOTRACE_POINT	 (E_SXF_MASK + 0x2751)
# define I_SX2752_RELOCATE               (E_SXF_MASK + 0x2752)
# define I_SX2753_ROW_DEADLOCK           (E_SXF_MASK + 0x2753)
# define I_SX2754_SET_TIMEOUTABORT       (E_SXF_MASK + 0x2754)
# define I_SX2755_KILL_QUERY	         (E_SXF_MASK + 0x2755)
/*
**	These are lookup texts used by SXA gateway
*/
/*
**	Audit access types
*/
# define I_SX2800_BAD_ACCESS_TYPE	(E_SXF_MASK + 0x2800)
# define I_SX2801_SELECT	(E_SXF_MASK + 0x2801)
# define I_SX2802_INSERT	(E_SXF_MASK + 0x2802)
# define I_SX2803_DELETE	(E_SXF_MASK + 0x2803)
# define I_SX2804_UPDATE	(E_SXF_MASK + 0x2804)
# define I_SX2805_COPYINTO	(E_SXF_MASK + 0x2805)
# define I_SX2806_COPYFROM	(E_SXF_MASK + 0x2806)
# define I_SX2807_SCAN		(E_SXF_MASK + 0x2807)
# define I_SX2808_ESCALATE	(E_SXF_MASK + 0x2808)
# define I_SX2809_EXECUTE	(E_SXF_MASK + 0x2809)
# define I_SX2810_EXTEND	(E_SXF_MASK + 0x2810)
# define I_SX2811_CREATE	(E_SXF_MASK + 0x2811)
# define I_SX2812_MODIFY	(E_SXF_MASK + 0x2812)
# define I_SX2813_DROP		(E_SXF_MASK + 0x2813)
# define I_SX2814_INDEX		(E_SXF_MASK + 0x2814)
# define I_SX2815_ALTER		(E_SXF_MASK + 0x2815)
# define I_SX2816_RELOCATE	(E_SXF_MASK + 0x2816)
# define I_SX2817_CONTROL	(E_SXF_MASK + 0x2817)
# define I_SX2818_AUTHENT	(E_SXF_MASK + 0x2818)
# define I_SX2819_READ		(E_SXF_MASK + 0x2819)
# define I_SX2820_WRITE		(E_SXF_MASK + 0x2820)
# define I_SX2821_READWRITE	(E_SXF_MASK + 0x2821)
# define I_SX2822_TERMINATE	(E_SXF_MASK + 0x2822)
# define I_SX2823_DOWNGRADE	(E_SXF_MASK + 0x2823)
# define I_SX2824_BYTID		(E_SXF_MASK + 0x2824)
# define I_SX2825_BYKEY		(E_SXF_MASK + 0x2825)
# define I_SX2826_MOD_DAC	(E_SXF_MASK + 0x2826)
# define I_SX2827_REGISTER	(E_SXF_MASK + 0x2827)
# define I_SX2828_REMOVE	(E_SXF_MASK + 0x2828)
# define I_SX2829_RAISE		(E_SXF_MASK + 0x2829)
# define I_SX282A_SETUSER       (E_SXF_MASK + 0x282A)
# define I_SX282B_REGRADE	(E_SXF_MASK + 0x282B)
# define I_SX282C_WRITEFIXED	(E_SXF_MASK + 0x282C)
# define I_SX2830_QRYTEXT	(E_SXF_MASK + 0x2830)
# define I_SX2831_SESSION	(E_SXF_MASK + 0x2831)
# define I_SX2832_QUERY		(E_SXF_MASK + 0x2832)
# define I_SX2833_LIMIT		(E_SXF_MASK + 0x2833)
# define I_SX2834_USERMESG	(E_SXF_MASK + 0x2834)
# define I_SX2835_CONNECT       (E_SXF_MASK + 0x2835)
# define I_SX2836_DISCONNECT    (E_SXF_MASK + 0x2836)
/* 
**	Event object types
*/
# define I_SX2900_BAD_EVENT_TYPE	(E_SXF_MASK + 0x2900)
# define I_SX2901_DATABASE	(E_SXF_MASK + 0x2901)
# define I_SX2902_APPLICATION	(E_SXF_MASK + 0x2902)
# define I_SX2903_PROCEDURE	(E_SXF_MASK + 0x2903)
# define I_SX2904_TABLE		(E_SXF_MASK + 0x2904)
# define I_SX2905_LOCATION	(E_SXF_MASK + 0x2905)
# define I_SX2906_VIEW		(E_SXF_MASK + 0x2906)
# define I_SX2907_RECORD	(E_SXF_MASK + 0x2907)
# define I_SX2908_SECURITY	(E_SXF_MASK + 0x2908)
# define I_SX2909_USER		(E_SXF_MASK + 0x2909)
# define I_SX2910_LEVEL		(E_SXF_MASK + 0x2910)
# define I_SX2911_ALARM		(E_SXF_MASK + 0x2911)
# define I_SX2912_RULE		(E_SXF_MASK + 0x2912)
# define I_SX2913_EVENT		(E_SXF_MASK + 0x2913)
# define I_SX2914_ALL		(E_SXF_MASK + 0x2914)
# define I_SX2915_INSTALLATION	(E_SXF_MASK + 0x2915)
# define I_SX2916_RESOURCE	(E_SXF_MASK + 0x2916)
# define I_SX2917_QRYTEXT	(E_SXF_MASK + 0x2917)


/*
** The following lock type definitions are to be used as the first element
** of a lock key of type LK_AUDIT.
*/
# define	SXAS_LOCK		1
# define	SXAP_LOCK		2
# define	SXLC_LOCK		3	/* SXL cache lock */

/*
** These are sub-lock id types used for the SXAS state lock
*/
# define	SXAS_STATE_LOCK	        1	/* Main state lock */
# define	SXAS_SAVE_LOCK		2	/* Save lock */

/*
** These are sub-lock id types for the SXAP physical lock
*/
# define	SXAP_SHMCTL	1
# define	SXAP_ACCESS	2
# define	SXAP_QUEUE	3

/*
** Function Prototypes definitions
*/
FUNC_EXTERN DB_STATUS sxf_call(
    SXF_OPERATION	op_code,
    SXF_RCB 		*rcb);

FUNC_EXTERN DB_STATUS sxf_trace(
	DB_DEBUG_CB  *trace_cb
);
