/*
** Copyright (c) 1987, 2010 Ingres Corporation All Rights Reserved.
*/

/**
** Name: GCN.H - The interface to GCF Name Service
**
** Description:
**      This file contains the definitions necessary for Name Server users 
**      to access the GCN service interface.
**
** History: $Log-for RCS$
**      02-Sep-87 (lin)
**          Created.
**      08-Sep-87 (berrow)
**          Moved message type definitions fron GCA.H
**	    Added some section comments in style consistent with GCA.H
**      25-Sept-87 (jbowers)
**          Added names to "typedef struct" statements, to allow mapping by
**          the debugger.
**	09-Nov-88  (jorge)
**	    Added a formal definition of the GCN_SHARED_TABLE keyword that
**	    defines a table as shared in a file system cluster environment.
**	01-Mar-89 (seiwald)
**	    Name Server (IIGCN) revamped.  Message formats between
**	    IIGCN and its clients remain the same, but the messages are
**	    constructed and extracted via properly aligning code.
**	    Variable length structures eliminated.  Strings are now
**	    null-terminated (they had to before, but no one admitted so).
**	02-Apr-89 (jorge)
**	    Put back 'C' structure definitions since they served as pseudo
**	    code definition of the GCN byte packed (un-alligned) messages)
**	25-Nov-89 (seiwald)
**	    Moved out name server internal structures to gcnint.h.
**	4-Jan-92 (seiwald)
**	    Support for installation passwords: new messages GCN_NS_AUTHORIZE
**	    and GCN_AUTHORIZED to request and receive authorization tickets
**	    from remote Name Servers.  New GCN error messages in the E_GC0140 
**	    range.  
**	23-Mar-92 (seiwald)
**	    Installation password error message shuffled.
**	23-Mar-92 (seiwald)
**	    Moved GCN_RTICKET, GCN_LTICKET from gcnstick.c here.
**	27-Mar-92 (brucek)
**	    Added E_GC0145_GCN_NO_GCN.
**	2-June-92 (seiwald)
**	    Added gcn_host to GCN_CALL_PARMS to support connecting to a remote
**	    Name Server.
**	2-July-92 (jonb)
**	    Add gcn_response_func field to GCN_CALL_PARMS to support netutil.
**	    If non-NULL, the field specifies a function which is called with 
**	    each line of output that would otherwise be displayed on the screen.
**	29-Sep-92 (brucek)
**	    Added GCN_MIB_MASK for GCN_D_OPER.gcn_flag.
**	01-Oct-92 (brucek)
**	    Moved GCN_MIB_MASK; added defines for MIB classid strings.
**	15-Oct-92 (brucek)
**	    Changed gcn_flag to unsigned i4.
**	20-Nov-92 (gautam)
**	    Added new GCN_NS_2_RESOLVE message to resolve remote user
**	    and password message.
**	03-Dec-92 (brucek)
**	    Changed GCN MIB variable names to remove "common".
**	13-May-93 (brucek)
**	    Renumbered E_GC0145_GCN_NO_GCN to E_GC0126_GCN_NO_GCN.
**	11-Jun-93 (brucek)
**	    Changed GCN_XCL_FLAG to GCN_UID_FLAG.
**	29-Nov-95 (gordy)
**	    Added GCN_TERMINATE function and prototype for IIGCn_call().
**	06-Feb-96 (rajus01)
**	    Added GCN_BRIDGESVR for Protocol Bridge.
**	24-Apr-96 (chech02)
**	    Added function prototypes for windows 3.1 port.
**	21-May-97 (gordy)
**	    New GCF security handling.  Added GCN_SERVERS to hold 
**	    info on all registered servers.  Added new message type 
**	    GCN2_RESOLVED and data object GCN2_D_RESOLVED.  Cleaned 
**	    up documentation for other Name Server resolve requests.
**	20-Aug-97 (rajus01)
**	    Added GCN_ATTR for extending vnode database to specify
**	    distributed security mechanisms.
**      17-Oct-97 (rajus01)
**          Added gcn_rmt_auth, gcn_rmt_emech, gcn_rmt_emode.
**	 9-Jan-98 (gordy)
**	    Added GCN_NET_FLAG for request to Name Server network database.
**	12-Jan-98 (gordy)
**	    Support added for direct GCA network connections.  Added
**	    error codes E_GC016x and host to GCN2_D_RESOLVED (LCL_ADDR).
**	17-Feb-98 (gordy)
**	    Expanded MIB entries for better GCM instrumentation.
**	31-Mar-98 (gordy)
**	    Make GCN2_D_RESOLVED extensible with variable array of values.
**	14-May-98 (gordy)
**	    Added support for remote auth mechanism in GCN2_D_RESOLVED.
**	27-May-98 (gordy)
**	    Added MIB entry for remote auth mechanism.
**	 4-Jun-98 (gordy)
**	    Added MIB entry for timeout.
**	10-Jun-98 (gordy)
**	    Added MIB entry for file compression point.
**	 2-Oct-98 (gordy)
**	    Moved MIB class definitions to gcm.h.
**	12-apr-1999 (mcgem01)
**	    The Name Server's class name is product specific.
**	27-Aug-99 (i4jo01)
**	    Increased number of connections granted to 32. (b98558).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	15-Oct-01 (gordy)
**	    General cleanup as followup to removal of nat/longnat.
**	    Removed internal messages and structures and unused
**	    GCN operations.
**	28-Jan-04 (gordy)
**	    Added E_GC0145_GCN_INPW_NOT_ALLOWED.
**	24-Mar-04 (wansh01)
**	    added error code E_GC0164 E_GC0165 E_GC0166 to support gcadm. 
**	11-Jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	08-Jul-04 (wansh01)
**	    added error code E_GC0167 E_GC0168 E_GC0169.
**	17-OCT-04 (wansh01)
**	    Delete error message E_GC0164_GCN_ADM_AUTH.
**	 6-Dec-05 (gordy)
**	    Added E_GC0146_GCN_INPW_SRV_NOSUPP.
**	 1-Mar-06 (gordy)
**	    Removed GCN_SERVERS class.  Added E_GC0153_GCN_SRV_STARTUP,
**	    E_GC0154_GCN_SRV_SHUTDOWN, E_GC0154_GCN_SRV_FAILURE for
**	    logging registered server status and E_GC0156_GCN_MAX_SESSIONS.
**      26-Oct-06 (Ralph Loen) Bug 116972
**          Add E_GC0157_GCN_BCHK_INFO, E_GC0158_GCN_BCHK_ACCESS, and
**          E_GC0159_GCN_BCHK_FAIL to report Name Server deregistration.
**	8-Jun-07 (kibro01) b118226
**	    Added E_GC0164_GCN_CHAINED_VNODE error
**      04-Apr-08 (rajus01) SD issue 126704, Bug 120136
**	    Added E_GC0170_GCN_USERNAME_LEN, E_GC0171_GCN_PASSWORD_LEN.
**	14-Apr-08 (rajus01) SD issue 126704, Bug 120137
**	    Added E_GC0172_GCN_PARTNER_LEN.
**	 3-Aug-09 (gordy)
**	    Added E_GC011F_GCN_BAD_RECFMT.  Increase GCN string lengths
**	    to 256.  The strings don't actually have a limit, but the
**	    symbols are used in many places, so a goood default is
**	    provided for general usage.
**	24-Nov-2009 (frima01) Bug 122490
**	    Added IIgcn_check and gcn_seterr_func prototypes from
**	    gcninit.h in order to include them in front end modules.
**      08-Apr-10 (ashco01) Bug: 123551
**          Added E_GC0173_GCN_HOSTNAME_MISMATCH informational message to
**          report difference between local hostname and config.dat
**          '.local_vnode' value. 
**	27-Aug-10 (gordy)
**	    Added E_GC0106_GCN_CONFIG_VALUE for invalid config values.
**/

#ifndef GCN_INCLUDED
#define GCN_INCLUDED

/*
** General definitions
*/

# define    GCN_EXTNAME_MAX_LEN	256	/* Max length of connection target */
# define    GCN_RESTART_TIME	10L	/* if old NS is hung */

/*
** Named info queues
*/

# define    GCN_NODE		"NODE"		/* Network connections */
# define    GCN_LOGIN		"LOGIN"		/* Network logins */
# define    GCN_ATTR		"ATTR"		/* Other Network attributes */

# define    GCN_RTICKET		"RTICKET"	/* Tickets on client */
# define    GCN_LTICKET		"LTICKET"	/* Tickets on server */

/*
** Define Server Classes - those which must be known
** (server class registry maintained in gcaclass.h)
*/

# define    GCN_NMSVR		"IINMSVR"	/* Name server */

# define    GCN_NS_REG		"NMSVR"		/* Name server registry */
# define    GCN_COMSVR		"COMSVR"	/* Communications server */
# define    GCN_INGRES		"INGRES"	/* INGRES DBMS server */
# define    GCN_STAR		"STAR"		/* STAR server */
# define    GCN_STSYN		"D"		/* special STAR synonym */


/*
** Supplemental GCN routines
*/

FUNC_EXTERN STATUS 	gcn_testaddr( char *, i4, char * );
FUNC_EXTERN i4		gcn_words( char *, char *, char **, i4, i4 );
FUNC_EXTERN STATUS	gcn_fastselect( u_i4, char * );


/*
** Name: IIGCn_call
**
** Description:
**	Function entry point for GCN API interface.
*/

typedef struct _GCN_CALL_PARMS	GCN_CALL_PARMS;
typedef struct _GCN_NM_DATA	GCN_NM_DATA;


FUNC_EXTERN STATUS
IIGCn_call( i4 service_code, GCN_CALL_PARMS *params );


/*
** GCN Status values
**
** There are reserved ranges of STATUS values
**
**	0x0000 ... 0x00FF	GCA STATUS values
**	0x0100 ... 0x01FF	GCN STATUS values
**	0x1000 ... 0x1FFF	GCS STATUS values
**	0x2000 ... 0x2FFF	GCC STATUS values
**
*/

# define	E_GC0100_GCN_SVRCLASS_NOTFOUND  (STATUS) (E_GCF_MASK + 0x0100)
# define	E_GC0101_GCN_FILE_OPEN		(STATUS) (E_GCF_MASK + 0x0101)
# define	E_GC0103_GCN_NOT_USER		(STATUS) (E_GCF_MASK + 0x0103)
# define	E_GC0104_GCN_USERS_FILE		(STATUS) (E_GCF_MASK + 0x0104)
# define	E_GC0105_GCN_CHAR_INIT		(STATUS) (E_GCF_MASK + 0x0105)
# define	E_GC0106_GCN_CONFIG_VALUE	(STATUS) (E_GCF_MASK + 0x0106)
# define	E_GC0110_GCN_SYNTAX_ERR		(STATUS) (E_GCF_MASK + 0x0110)
# define	E_GC011F_GCN_BAD_RECFMT		(STATUS) (E_GCF_MASK + 0x011F)
# define	E_GC0120_GCN_BAD_RECLEN		(STATUS) (E_GCF_MASK + 0x0120)
# define	E_GC0121_GCN_NOMEM		(STATUS) (E_GCF_MASK + 0x0121)
# define	E_GC0122_GCN_2MANY		(STATUS) (E_GCF_MASK + 0x0122)
# define	E_GC0123_GCN_NSID_FAIL		(STATUS) (E_GCF_MASK + 0x0123)
# define	E_GC0124_GCN_NO_IINAME		(STATUS) (E_GCF_MASK + 0x0124)
# define	E_GC0125_GCN_BAD_FILE		(STATUS) (E_GCF_MASK + 0x0125)
# define	E_GC0126_GCN_NO_GCN		(STATUS) (E_GCF_MASK + 0x0126)
# define	E_GC0130_GCN_BAD_SYNTAX		(STATUS) (E_GCF_MASK + 0x0130)
# define	E_GC0131_GCN_BAD_RMT_VNODE	(STATUS) (E_GCF_MASK + 0x0131)
# define	E_GC0132_GCN_VNODE_UNKNOWN	(STATUS) (E_GCF_MASK + 0x0132)
# define	E_GC0133_GCN_LOGIN_UNKNOWN	(STATUS) (E_GCF_MASK + 0x0133)
# define	E_GC0134_GCN_INT_NQ_ERROR	(STATUS) (E_GCF_MASK + 0x0134)
# define	E_GC0135_GCN_BAD_SVR_TYPE	(STATUS) (E_GCF_MASK + 0x0135)
# define	E_GC0136_GCN_SVRCLASS_UNKNOWN	(STATUS) (E_GCF_MASK + 0x0136)
# define	E_GC0137_GCN_NO_GCC		(STATUS) (E_GCF_MASK + 0x0137)
# define	E_GC0138_GCN_NO_SERVER		(STATUS) (E_GCF_MASK + 0x0138)
# define	E_GC0139_GCN_NO_DBMS		(STATUS) (E_GCF_MASK + 0x0139)
# define	E_GC0140_GCN_INPW_NOPROTO	(STATUS) (E_GCF_MASK + 0x0140)
# define	E_GC0141_GCN_INPW_NOSUPP	(STATUS) (E_GCF_MASK + 0x0141)
# define	E_GC0142_GCN_INPW_INVALID	(STATUS) (E_GCF_MASK + 0x0142)
# define	E_GC0143_GCN_INPW_BADTICKET	(STATUS) (E_GCF_MASK + 0x0143)
# define	E_GC0144_GCN_INPW_BROKEN	(STATUS) (E_GCF_MASK + 0x0144)
# define	E_GC0145_GCN_INPW_NOT_ALLOWED	(STATUS) (E_GCF_MASK + 0x0145)
# define	E_GC0146_GCN_INPW_SRV_NOSUPP	(STATUS) (E_GCF_MASK + 0x0146)
# define	E_GC0151_GCN_STARTUP		(STATUS) (E_GCF_MASK + 0x0151)
# define	E_GC0152_GCN_SHUTDOWN		(STATUS) (E_GCF_MASK + 0x0152)
# define	E_GC0153_GCN_SRV_STARTUP	(STATUS) (E_GCF_MASK + 0x0153)
# define	E_GC0154_GCN_SRV_SHUTDOWN	(STATUS) (E_GCF_MASK + 0x0154)
# define	E_GC0155_GCN_SRV_FAILURE	(STATUS) (E_GCF_MASK + 0x0155)
# define	E_GC0156_GCN_MAX_SESSIONS	(STATUS) (E_GCF_MASK + 0x0156)
# define        E_GC0157_GCN_BCHK_INFO          (STATUS) (E_GCF_MASK + 0x0157)
# define        E_GC0158_GCN_BCHK_ACCESS        (STATUS) (E_GCF_MASK + 0x0158)
# define        E_GC0159_GCN_BCHK_FAIL          (STATUS) (E_GCF_MASK + 0x0159)
# define	E_GC0160_GCN_NO_DIRECT		(STATUS) (E_GCF_MASK + 0x0160)
# define	E_GC0161_GCN_DC_HET		(STATUS) (E_GCF_MASK + 0x0161)
# define	E_GC0162_GCN_DC_VERSION		(STATUS) (E_GCF_MASK + 0x0162)
# define	E_GC0163_GCN_DC_NO_SERVER	(STATUS) (E_GCF_MASK + 0x0163)
# define	E_GC0164_GCN_CHAINED_VNODE	(STATUS) (E_GCF_MASK + 0x0164)
# define	E_GC0165_GCN_ADM_CMD		(STATUS) (E_GCF_MASK + 0x0165)
# define	E_GC0166_GCN_ADM_INT_ERR	(STATUS) (E_GCF_MASK + 0x0166)
# define	E_GC0167_GCN_ADM_INIT_ERR	(STATUS) (E_GCF_MASK + 0x0167)
# define	E_GC0168_GCN_ADM_SESS_ERR	(STATUS) (E_GCF_MASK + 0x0168)
# define	E_GC0169_GCN_ADM_SVR_CLOSED 	(STATUS) (E_GCF_MASK + 0x0169)
# define	E_GC0170_GCN_USERNAME_LEN	(STATUS) (E_GCF_MASK + 0x0170)
# define	E_GC0171_GCN_PASSWORD_LEN	(STATUS) (E_GCF_MASK + 0x0171)
# define	E_GC0172_GCN_PARTNER_LEN	(STATUS) (E_GCF_MASK + 0x0172)
# define	E_GC0173_GCN_HOSTNAME_MISMATCH	(STATUS) (E_GCF_MASK + 0x0173)


/*
** GCN Service codes and Parameter List structures
**
**	GCN_INITIATE:	Initialize GCN interface
**	GCN_TERMINATE:	Terminate GCN interface
**	GCN_ADD:	Add entry to NS Database
**	GCN_DEL:	Delete entry from NS Database
**	GCN_RET:	Retrieve entry from NS Database
**	GCN_SHUTDOWN:	Shutdown Name Server
**
** Note: Service codes 0x0004 (GCN_MOD) and 0x0007 
**       (GCN_SECURITY) are no longer used.
*/

# define	GCN_ADD		(i4)0x0001	/* GCN_CALL_PARMS */
# define	GCN_DEL		(i4)0x0002	/* GCN_CALL_PARMS */
# define	GCN_RET		(i4)0x0003	/* GCN_CALL_PARMS */
# define	GCN_INITIATE 	(i4)0x0005	/* */
# define	GCN_SHUTDOWN	(i4)0x0006	/* */
# define	GCN_TERMINATE	(i4)0x0008	/* */


/*
** Name: GCN_CALL_PARMS
**
** Description:
**	Parameters for GCN_ADD, GCN_DEL and GCN_RET services.
*/

struct _GCN_CALL_PARMS
{
    STATUS	gcn_status;
    char	(*async_id)();
    i4		(*gcn_response_func)();
    u_i4	gcn_flag;
    char	*gcn_host;
    char	*gcn_type;
    char	*gcn_uid;
    char	*gcn_obj;
    char	*gcn_value;
    char	*gcn_install;
    GCN_NM_DATA *gcn_buffer;		
};


/*
** GCN message types
**
**	GCN_NS_OPER	Name Server Operation
**	GCN_RESULT	Operation Result
**
** Note: message types 0x40, 0x41, 0x44 - 0x47 are for
**       internal GCA/GCN communications.
*/

# define	GCN_NS_OPER		0x42	/* GCN_D_OPER */
# define	GCN_RESULT		0x43	/* GCN_OPER_DATA */

/*
** GCN data objects
**
** GCN is aware of and deals with a specific set of complex 
** data objects which pass across the GCN service interface 
** as data associated with various message types.  This is 
** the set of objects which are meaningful to the application 
** users of GCN.
**
** The structures included here are for reference only.
*/

/* GCN_VAL: General GCN value */    
typedef	struct _GCN_VAL GCN_VAL;

struct _GCN_VAL
{
    i4		gcn_l_item;
    char	gcn_value[1];
};


/* GCN_REQ_TUPLE: GCN tuple format */    
typedef struct _GCN_REQ_TUPLE GCN_REQ_TUPLE;

struct _GCN_REQ_TUPLE
{
    GCN_VAL	gcn_type;
    GCN_VAL	gcn_uid;
    GCN_VAL	gcn_obj;
    GCN_VAL	gcn_val;

};

/*
** The following lengths are not hard limits.  The
** associated values may be longer and have no fixed
** maximum.  These values are for backward support
** and provide enough room for the majority cases.
*/
# define	GCN_TYP_MAX_LEN		32
# define	GCN_UID_MAX_LEN		256
# define	GCN_OBJ_MAX_LEN		256
# define	GCN_VAL_MAX_LEN		256

/* GCN_NM_DATA: GCN data rows */
struct _GCN_NM_DATA
{
    i4			gcn_tup_cnt;	/* Length of following array */
    GCN_REQ_TUPLE	gcn_tuple[1];	/* Variable length */
};


/*
** GCN_D_OPER: Operation data
**
** Messages: GCN_NS_OPER
*/
typedef struct _GCN_D_OPER GCN_D_OPER;

struct _GCN_D_OPER
{
    u_i4		gcn_flag;	/* Default is PRIVATE|SHARE */

# define	GCN_DEF_FLAG	0x0000
# define	GCN_PUB_FLAG	0x0001
# define	GCN_UID_FLAG	0x0002		/* Operation sets userid */
# define	GCN_SOL_FLAG	0x0004		/* Sole server indication  */
# define	GCN_MRG_FLAG	0x0008		/* Merge operation */
# define	GCN_NET_FLAG	0x0010		/* Network database request */

    i4			gcn_opcode;	/* GCN_ADD, GCN_DEL, GCN_RET */
    GCN_VAL		gcn_install;
    i4			gcn_tup_cnt;	/* Length of following array */
    GCN_REQ_TUPLE 	gcn_tuple[1];	/* Variable length */
};


/*
** GCN_OPER_DATA: Operation result data
**
** Messages: GCN_RESULT
*/
typedef struct _GCN_OPER_DATA GCN_OPER_DATA;

struct _GCN_OPER_DATA
{
    i4			gcn_op;		/* GCN_ADD, GCN_DEL, GCN_RET */
    i4			gcn_tup_cnt;	/* Length of following array */
    GCN_REQ_TUPLE 	gcn_tuple[1];	/* Variable length */
};

/* prototypes */

FUNC_EXTERN     STATUS          IIgcn_check( VOID );
FUNC_EXTERN     VOID            gcn_seterr_func( i4  ((*)(char *)) );
#endif
/************************** End of gcn.h *************************************/
