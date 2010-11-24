/*
** Copyright (c) 1989, 2010 Ingres Corporation
*/

#ifndef GCNINT_H_INCLUDED
#define GCNINT_H_INCLUDED

#include <qu.h>
#include <si.h>

/**
** Name: gcnint.h - GCF Name Server internal structures
**
** Description:
**      This file contains the definitions of the data structures
**	used internally by GCN.
**
** History: 
**	25-Nov-89 (seiwald)
**	    Extracted from gcn.h.
**	28-Nov-89 (seiwald)
**	    New gca_incache flag in GCN_QUE_NAME, to signal that
**	    cache has been filled.
**	11-Dec-89 (seiwald)
**	    Removed cont flag from GCN_STATIC.   A call to GCshut
**	    stops GCexec.
**	24-Aug-90 (seiwald)
**	    New maxsessions set by II_GCNxx_MAX.
**	24-Apr-91 (brucek)
**	    Added gcn_last_mod timestamp to GCN_QUE_NAME
**	18-Jun-91 (alan)
**	    Added gcn_rec_modified and flag values to GCN_TUP_QUE;
**	    changed gcn_temp field in GCN_DB_RECORD to gcn_added; added
**	    gcn_tot_records and gcn_mod_records to GCN_QUE_NAME 
**	28-Jun-91 (seiwald)
**	    Internal name queue access restructured.  See gcnsfile.c
**	    for new description.
**	17-Jul-91 (seiwald)
**	    New define GCN_GCA_PROTOCOL_LEVEL, the protocol level at
**	    which the Name Server is running.  It may be distinct from
**	    the current GCA_PROTOCOL_LEVEL.
**	7-Jan-92 (seiwald)
**	    Support for installation level passwords: new GCN_RESOLVE_CB,
**	    a control block for async GCN_RESOLVE operation.
**	23-Mar-92 (seiwald)
**	    Defined GCN_GLOBAL_USER as "*" to help distinguish the owner
**	    of global records from other uses of "*".
**	28-Sep-92 (brucek)
**	    Added new fields for IMA support:
**	    GCN_STATIC.cache_modified;
**	11-Nov-92 (gautam)
**	    Defined a gca_message_type in GCN_RESOLVE_CB structure
**	    to differentiate between the GCN_RESOLVE and GCN_2_RESOLVE
**	    messages. Bumped up the GCN_GCA_PROTOCOL_LEVEL to 60
**	12-Jan-93 (brucek)
**	    Added FUNC_EXTERN for gcn_pwd_auth.
**	15-Mar-93 (edg)
**	    Added no_assocs to GCN_TUP so that tuples can be sorted on
**	    resolve.  This field is only applicable to tuples in the cache
**	    as it is not stored on disk.  Not a great place for it, but will
**	    suffice for the time being.
**	12-Aug-93 (brucek)
**	    Added GCN_STATIC.next_cleanup (for bedchecks only).
**	16-Nov-95 (gordy)
**	    Bumped protocol level.
**	29-Nov-95 (gordy)
**	    Added name server client interface for want of a better place.
**	 4-Dec-95 (gordy)
**	    Added prototypes.  Added mib_initialized and clustered to
**	    GCN_STATIC.  Reorganized initialization functions.
**	26-Feb-96 (gordy)
**	    Extracted disk filename formation to function to handle
**	    platform dependencies.
**	 5-Aug-96 (gordy)
**	    Added user field for local user ID in RESOLVE control block
**	    to be used node/login access.
**	 3-Sep-96 (gordy)
**	    Reorganization of internal function names to further separate
**	    client and server functions and localize GCA access to as few
**	    files as possible.
**	11-Mar-97 (gordy)
**	    Moved some Name Server utility functions to gcu for use
**	    outside the Name Server.
**	19-Mar-97 (gordy)
**	    Name Server communications protocol GCA violations fixed 
**	    at PROTOCOL_LEVEL_63.  Use end-of-group to indicate turn-
**	    around rather than end-of-data.
**	21-May-97 (gordy)
**	    New GCF security handling.  Updated function prototypes.
**	17-Oct-97 (rajus01)
**	    Added GCN_ATTR_LEN, gcn_rslv_attr() function prototype. 
**	    Added auth, emode, emech in GCN_RESOLVE_CB.
**	 5-Dec-97 (rajus01)
**	    Installation passwords handling as remote auth. Updated
** 	    function prototypes.
**	12-Jan-98 (gordy)
**	    Added support for direct GCA network connections.
**	17-Feb-98 (gordy)
**	    Extended gcn global info to support GCM instrumentation.
**	19-Feb-98 (gordy)
**	    Added name queue routines for better dynamic access.
**	31-Mar-98 (gordy)
**	    Added definitions for known vnode attributes.
**	 5-May-98 (gordy)
**	    Client user ID added to server auth.
**	15-May-98 (gordy)
**	    Added support for remote authentication.
**	21-May-98 (gordy)
**	    Added default remote authentication mechanism to GCN globals.
**	27-May-98 (gordy)
**	    Changed remote auth mech from pointer to char array.
**	 1-Jun-98 (gordy)
**	    GCN message buffer must be larege enough for FASTSELECT.
**	 4-Jun-98 (gordy)
**	    GCA_LISTEN in Name Server now has timeout for background checks.
**	    Removed bedcheck thread.  Bedchecking now done by regular threads
**	    during timeout or after client request has been processed.
**	 5-Jun-98 (gordy)
**	    Added GCN_QUE_FNAME and GCN_SVR_FNAME.
**	10-Jun-98 (gordy)
**	    Reworked disk file processing for reuse of deleted records.
**	    Moved file compression and ticket expiration to background.
**	16-Jun-98 (gordy)
**	    Added hashed access to Name Queues.
**	28-Jul-98 (gordy)
**	    Added bedcheck flags to globals.
**	26-Aug-98 (gordy)
**	    Added support for delegation of user authentication.
**	10-Sep-98 (gordy)
**	    Added global storage and functions for installation registry.
**	30-Sep-98 (gordy)
**	    Added structure to save registry protocol information.
**	 2-Oct-98 (gordy)
**	    Cleaned up quote handling in target specifications.
**	 2-Jul-99 (rajus01)
**	    Added GCN_RSLV_EMBEDGCC for Embedded Comsvr support.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	22-Mar-02 (gordy)
**	    Moved internal defines from gcn.h to here.
**	24-Mar-04 (wansh01) 
**	    Added gcn_gcadm rtns to support name server admin interface.
**	07-Jul-04 (wansh01) 
**	    Added gcn_sess_info().
**	15-Jul-04 (gordy)
**	    Enhanced encryption of passwords.
**	16-Jul-04 (gordy)
**	    Extended ticket versions and bumped GCN protocol level to 65.
**	 6-Aug-04 (gordy)
**	    Adjust session info to include association ID.
**	01-Oct-2004 (rajus01) Startrak Prob 148; Bug# 113165
**         Added GCN_FSEL_TIMEOUT, bchk_msg_timeout  to set the timeout 
**	   value for bedcheck messages.
**	25-Sep-2005 (loera01) Startrak Prob 148; Bug# 113165
**         Changed GCN_FSEL_TIMEOUT to 20 from 10.
**	25-Oct-2005 (gordy)
**	    Define size of authentication buffer.
**	 1-Mar-06 (gordy)
**	    SERVERS queue is no longer maintained, dropped associated
**	    maintenance functions.  Server key now saved in connection
**	    control block, parameters to several functions updated.
**	    Session control blocks now dynamically allocated - added
**	    session count to globals.
**	22-Aug-06 (rajus01)
**	    Added new variables, routines to support "register server",
**	    "show/format sessions" commands to name server.
**      21-Apr-2009 (Gordy) Bug 121967
**          Add GCN_SVR_TERM so that a clean shutdown of the GCN with the
**          fix for bug 121931 does not lead to DBMS deregistrations.
**	 3-Aug-09 (gordy)
**	    Remove fixed length field restrictions in DB records.
**	 24-Nov-2009 (frima01) Bug 122490
**	    Moved IIgcn_check and gcn_seterr_func prototypes to gcn.h
**	    in order to include them in front end modules.
**	27-Aug-10 (gordy)
**	    Added password encoding version for VNODE login passwords.
**	    Added symbols for encoding versions.
**       15-Nov-2010 (stial01) SIR 124685 Prototype Cleanup
**          Changes to eliminate compiler prototype warnings.
*/

#define		MAXLINE			1024

# define	GCN_MAX_SESSIONS	16		/* Active sessions */
# define	GCN_TIME_CLEANUP	(5*60)		/* between bedchecks */
# define	GCN_TICKET_EXPIRE	(30*60)		/* ticket lifespan */
# define	GCN_TIMEOUT		60L		/* Listen timeout */
# define	GCN_RESTART_TIME	10L		/* if old NS is hung */
# define	GCN_FSEL_TIMEOUT	20L		/* bcheck msg timeout*/
# define	GCN_COMPRESS_POINT	50		/* File usage % */
# define	GCN_GCA_PROTOCOL_LEVEL	GCA_PROTOCOL_LEVEL_65
# define	GCN_GLOBAL_USER		"*"		/* global entry owner */
# define	GCN_NOUSER		"*"		/* needs no password */
# define	GCN_NOPASS		"*"	/* password it doesn't need */
# define	GCN_QUE_FNAME		"iiname.all"	/* Name queue file */
# define	GCN_SVR_FNAME		"svrclass.nam"	/* Server class file */
# define	GCN_SVR_MAX		32	
# define	GCN_AUTH_MAX_LEN	512

# define GCN_AUTH_TICKET	0x01

# define GCN_AUTH_TICKET_MAG	"@1@"	/* GCN_AUTH_TICKET prefix */
# define GCN_L_AUTH_TICKET_MAG	3

# define GCN_TICK_MAG		0x30 	/* Ticket magic ID */
# define GCN_V0_TICK_MAG	0x30	/* Version 0 ticket */
# define GCN_V1_TICK_MAG	0x31	/* Version 1 ticket */
# define GCN_V2_TICK_MAG	0x32	/* Version 2 ticket */
# define GCN_V3_TICK_MAG	0x33	/* Version 3 ticket */

# define GCN_L_TICKET		64	/* Length of the ticket */
# define GCN_DFLT_L_TICKVEC	10	/* how many tickets to ask for */

# define GCN_ATTR_AUTH_MECH	"authentication_mechanism"
# define GCN_ATTR_ENC_MECH	"encryption_mechanism"
# define GCN_ATTR_ENC_MODE	"encryption_mode"
# define GCN_ATTR_CONN_TYPE	"connection_type"
# define GCN_ATTR_VAL_DIRECT	"direct"

# define GCN_ATTR_LEN		32

# define GCN_VLP_CLIENT		0
# define GCN_VLP_LOGIN		1
# define GCN_VLP_COMSVR		2

# define GCN_VLP_V0		0
# define GCN_VLP_V1		1

# define GCN_SYS_COUNT 		1
# define GCN_USER_COUNT		2
# define GCN_ALL_COUNT		3
# define GCN_SYS_INFO 		4
# define GCN_USER_INFO 		5
# define GCN_ALL_INFO 		6
/*
** Name: GCN_MBUF - buffers for name server I/O
**
** Description:
**	GCN is multithreaded, communicating with multiple clients at
**	once.  To prevent collision while accessing the name queues,
**	GCN ensures that all operations affecting the name queue are
**	atomic with respect to client I/O.  Responses to retrieve
**	operations can potentially be longer than one GCA_SEND, so GCN
**	queues up the pieces of the response usings these MBUF's.
*/

/* 
** Warning - length of buf must match the buffer length used by gcngca.c! 
*/
#define	GCN_MSG_BUFF_LEN	GCA_FS_MAX_DATA

typedef struct {
	QUEUE	q;
	i4	type;			/* GCA message type */
	char	*data;			/* user data area */
	i4	len;			/* available user data area */
	i4	used;			/* used part of user data area */
	i4	size;			/* length of message buffer */
	char	buf[GCN_MSG_BUFF_LEN];	/* message buffer */
} GCN_MBUF;

/*
** Name: GCN_RESOLVE_CB - control block for GCN_RESOLVE operation
**
** Description:
**	Holds state information for duration of a GCN_RESOLVE operation,
**	which may suspended while fetching tickets for installation
**	passwords.  Note that the pointers in this structure must point
**	to data which persists for the life of the GCN_RESOLVE operation.
**	Pointers to name queue elements is not allowed, because they may
**	change once the queue is unlocked.
**
** History:
**	 5-Aug-96 (gordy)
**	    Added user field for local user ID to be used node/login access.
**	12-Jan-98 (gordy)
**	    Added support for direct GCA network connections.
**	 5-May-98 (gordy)
**	    Added user client ID.
**	15-May-98 (gordy)
**	    Added support for remote authentication.
**	26-Aug-98 (gordy)
**	    Added support for delegation of user authentication.
**      13-Jul-01 (wansh01) 
**          Added GCN_RSLV_RMECH, GCN_RSLV_EMECH, GCN_RSLV_EMODE and
**          GCN_RSLV_CTYPE flags. 
**          changed _GCN_RESOLVE_CB field 'target' from a fixed length buffer 
**          to a ptr. 
**	15-Jul-04 (gordy)
**	    Resolved password may now be store encrypted or decrypted:
**	    added GCN_RSLV_PWD_ENC flag to indicate password state.
**	 3-Aug-09 (gordy)
**	    Support long usernames and passwords.  A small buffer is
**	    provided to handle the majority of cases.  Dynamic memory
**	    is used for exceptional cases.  Removed delegated auth.
*/

typedef struct _GCN_RESOLVE_CB
{
    STATUS	status;
    i4		flags;

#define	GCN_RSLV_IP		0x0001		/* Installation password */
#define	GCN_RSLV_DIRECT		0x0002		/* Direct connection */
#define GCN_RSLV_QUOTE		0x0004		/* DB name should be quoted */
#define GCN_RSLV_EMBEDGCC	0x0008		/* Client uses embedded */
					        /* Comm Server for remote */
						/* connections */
#define GCN_RSLV_RMECH 		0x0010		/* ATTR rmech is processed */
#define GCN_RSLV_EMECH 		0x0020		/* ATTR emech is processed */
#define GCN_RSLV_EMODE 		0x0040		/* ATTR emode is processed */
#define GCN_RSLV_CTYPE         	0x0080		/* ATTR conn type is */
                                                /* processed */
#define	GCN_RSLV_PWD_ENC	0x0100		/* passwd_out is encrypted */

    i4		gca_message_type;

    /*
    ** Target connection string is dynamically allocated.
    ** Vnode, dbname and class may point to target components,
    ** other strings and at minimum should point to empty
    ** strings ("").
    */
    char	*target;
    char	*vnode;
    char	*dbname;
    char	*class;

    char	*account;				/* GCA_LISTEN user */
    char	*username;				/* GCA_LISTEN alias */

    /*
    ** Username and password specified as part of target string
    ** or provided in GCN_NS_2_RESOLVE message.  Standard length
    ** values use declared buffers, otherwise dynamically allocate
    ** space.  
    **
    ** Installation passwords are saved in an alternate buffer
    ** which is dynamically allocated.  Password buffer must be 
    ** large enough to hold an IP ticket formatted as a password.
    */
    char	*usrptr;				/* specified user ID */
    char	*pwdptr;				/* specified password */
    char	usrbuf[ 35 ];				/* usr default buffer */
    char	pwdbuf[ 35 ];				/* pwd default buffer */
    char	*ip;					/* installation pwd */
    char	*usr;
    char	*pwd;

    i4		auth_len;			/* length of remote auth */
    PTR		auth;				/* remote authentication */

    char	rmech[ GCN_ATTR_LEN ];		/* remote auth mechanism */
    char	emech[ GCN_ATTR_LEN ]; 		/* encryption mechanism */
    char	emode[ GCN_ATTR_LEN ];		/* encryption mode */

    struct 
    {
	i4	tupc;		/* Number of entries */

	/*
	** Local server information.  Host and address
	** may reference existing storage.  If needed,
	** a buffer may be dynamically allocated.  One
	** standard sized buffer is pre-declared which
	** may be reference by lclptr[0].
	*/
	char	*host[ GCN_SVR_MAX ];
	char	*addr[ GCN_SVR_MAX ];
	char	*lclptr[ GCN_SVR_MAX ];
	char	lclbuf[ 70 ];

	/*
	** Authentication is dynamically allocated.
	*/
	PTR	auth[ GCN_SVR_MAX ];
	i4	auth_len[ GCN_SVR_MAX ];

    } catl;

    struct 
    {
	i4	tupc;		/* Number of entries */
	
	/*
	** Remote server information.  Protocol, node
	** and port may reference existing storage.  If 
	** needed, a buffer may be dynamically allocated.
	** One standard sized buffer is pre-declared which 
	** may be referenced by rmtptr[0].
	*/
	char	*protocol[ GCN_SVR_MAX ];
	char	*node[ GCN_SVR_MAX ];
	char	*port[ GCN_SVR_MAX ];
	char	*rmtptr[ GCN_SVR_MAX ];
    	char	rmtbuf[ 70 ];

    } catr;

} GCN_RESOLVE_CB;

/*
** GCN_TUP - GCN_VAL pointers to the data in a name server tuple.
**    Use gcn_get_tup() and gcn_put_tup() for manipulation.
*/

typedef struct _GCN_TUP
{
      char    *uid;
      char    *obj;
      char    *val;
      i4      no_assocs;
} GCN_TUP;

/*
** GCN_TUP_QUE - This is the structure of each tuple in the memory queue.
*/    

typedef struct _GCN_TUP_QUE
{

    QUEUE	gcn_q;
    char	*gcn_type;	  /* For MO, same as name queue */
    i4		gcn_rec_no;	  /* physical record number in name file */
    i4		gcn_tup_id;	  /* unique id for MIB object instance */
    char	gcn_instance[16]; /* MIB object instance identifier */
    GCN_TUP	gcn_tup;
    char	gcn_data[1];	  /* Variable length storage for gcn_tup */

} GCN_TUP_QUE;

/*
** GCN_DB_RECORD - This is the record stored in the Name Database
** Used by gcn_nq_lock, gcn_nq_unlock. 
**
** GCN_DB_RECO - Original db record format with fixed length fields.
** The field gcn_tup_id is either 0 or a value greater than 0xFFFF
** and can be used to identify this record version.
**
** GCN_DB_REC1 - Variable length parameterized field record format.  
** Record length may also vary, but records within a particular file 
** will all be the same length.  Default/minimum length is same as 
** V0 format.
**
** GCN_DB_DEL - Deleted records (other than V0 deleted records).
*/

typedef struct _GCN_DB_REC0
{
    i4		gcn_tup_id;
    i4		gcn_l_uid;
    char	gcn_uid[ 32 ];
    i4		gcn_l_obj;
    char	gcn_obj[ 256 ];
    i4		gcn_l_val;
    char	gcn_val[ 64 ];
    bool	gcn_obsolete;
    bool	gcn_invalid;

}   GCN_DB_REC0;

typedef struct _GCN_DB_REC1
{
    i4		gcn_rec_type;
    u_i4	gcn_rec_len;

/*  GCN_DB_PARAM gcn_rec_params[*]; 	Variable length */

} GCN_DB_REC1;

typedef struct _GCN_DB_PARAM
{
    i4		gcn_p_type;

#define	GCN_P_UID		1
#define	GCN_P_OBJ		2
#define	GCN_P_VAL		3
#define	GCN_P_TID		4

    u_i4	gcn_p_len;
/*  u_i1	gcn_p_data[*]; 		Variable length */

} GCN_DB_PARAM;

typedef struct _GCN_DB_DEL
{
    i4		gcn_rec_type;
    u_i4	gcn_rec_len;

} GCN_DB_DEL;

typedef union _GCN_DB_RECORD
{
    GCN_DB_REC0	rec0;
    GCN_DB_REC1	rec1;
    GCN_DB_DEL	del;

} GCN_DB_RECORD;


/*
** Record Types:
**
** GCN_DBREC_V0		Record format GCN_DB_REC0.
** GCN_DBREC_V1		Record format GCN_DB_REC1.
** GCN_DBREC_DEL	Record format GCN_DB_DEL.
** GCN_DBREC_EOF	Logical EOF, no record format.
** 
** GCN_DBREC_V0_MASK	Reserved bits used my some V0 records.
*/

#define	GCN_DBREC_V0		0
#define	GCN_DBREC_V1		1
#define	GCN_DBREC_DEL		0xFFFF
#define	GCN_DBREC_EOF		-1

#define	GCN_DBREC_V0_MASK	0xFFFF0000

#define	GCN_RECSIZE_BASE	512
#define	GCN_RECSIZE_INC		128


/*
** GCN_REC_LST - Structure used to maintain a list of record numbers.
*/

typedef struct _GCN_REC_LST GCN_REC_LST;

struct _GCN_REC_LST
{

    GCN_REC_LST	*next;			/* Next sub-list */
    i2		size;			/* Max number of entries in rec[] */
    i2		count;			/* Current number of entries in rec[] */
    i4		rec[1];			/* Record numbers (variable length) */

#define	GCN_LIST_SMALL		14	/* Number of entries for small files */
#define	GCN_LIST_LARGE		1022	/* Number of entries for large files */

};

/*
** GCN_QUE_NAME - This is the structure containing the service classes
**		   and their QUEUE pointers.
**
** History:
**	 3-Aug-09 (gordy)
**	    Dynamically allocate gcn_type.  Record length is no longer
**	    fixed, but all records within a file will be the same length.
**	    Added record length and flag indicating file needs to be
**	    re-written to change record size.
*/

typedef struct _GCN_QUE_NAME
{
    QUEUE	q;
    char	*gcn_type;
    char	*gcn_filename;		/* Disk file name */
    FILE	*gcn_file;		/* Open file pointer */
    SYSTIME	gcn_last_mod;		/* Last mode time for shared files */
    SYSTIME	gcn_next_cleanup;	/* Time of next queue cleaning */
    bool	gcn_transient;		/* Name queue for transient servers */
    bool	gcn_shared;		/* Other's may r/w file */
    bool	gcn_incache;		/* Queue is in memory */
    bool	gcn_rewrite;		/* Force rewrite of disk file */
    i4		gcn_rec_len;		/* Record length */
    i4		gcn_logical_eof;	/* Record # of logical end-of-file */
    i4		gcn_active;		/* Number of active records */
    i4		gcn_records_added;	/* Number of new records */
    i4		gcn_records_deleted;	/* Number of deleted records */
    i4		gcn_subfields;		/* Number of value subfields */
    PTR		gcn_hash;		/* Hashed access control block */
    GCN_REC_LST *gcn_rec_list;		/* Deleted record list */
    QUEUE	gcn_deleted;		/* Deleted tuples */
    QUEUE	gcn_tuples;		/* List of tuples */

} GCN_QUE_NAME;


/*
** Name Queue hashing datatypes.
*/

#define	GCN_HASH_SMALL	8		/* Hash size for smaller queues */
#define	GCN_HASH_LARGE	32		/* Hash size for larger queues */

typedef	u_i1		GCN_HASH;
typedef	GCN_HASH	(*HASH_FUNC)( GCN_TUP * );


/*
** Name: GCN_REG - Registry protocols.
*/

typedef struct
{
    QUEUE	reg_q;
    char	*reg_proto;
    char	*reg_port;
    char	*reg_host;
} GCN_REG;


/*
** Session information for admin SHOW SESSIONS.
*/

typedef struct
{
    PTR 	sess_id;
    i4		assoc_id;
    char 	*user_id;
    char	state[ 80 ];
    bool        is_system;
    char        *svr_class;
    char        *svr_addr;
} GCN_SESS_INFO;


/*

/*
** Name: GCN_STATIC - static data for GCN
**
** History:
**	15-Jul-04 (gordy)
**	    Added masks for encryption of stored passwords and passwords
**	    passed to GCC.
**	 1-Mar-06 (gordy)
**	    Added session count.
**	08-Oct-06 (rajus01)
**	    Added highwater to account for maximum number of active sessions
**	    ever reached. 
**	 3-Aug-09 (gordy)
**	    Allocate strings to remove length restrictions.
**	27-Aug-10 (gordy)
**	    Added password encoding version.
*/

typedef struct 
{

	char		*hostname;
	char		*install_id;
	char		*listen_addr;
	char		*username;
	char		*registry_addr;
	char		*registry_id;
	char		*svr_type;
	char		*lcl_vnode;
	char		*rmt_vnode;
	char		*rmt_mech;
	u_i1		login_mask[ 8 ];	/* Must be 8 bytes */
	u_i1		comsvr_mask[ 8 ];	/* Must be 8 bytes */
	QUEUE		name_queues;
	QUEUE		registry;
	i4		max_user_sessions;
	i4		highwater;	  /* Max act user sess ever reached */
	i4		usr_sess_count;
	i4		adm_sess_count;	
	i4		session_count;
	i4		language;
	i4		pwd_enc_vers;
	i4		compress_point;
	i4		ticket_cache_size;
	i4		ticket_exp_time;
	i4		ticket_exp_interval;
	i4		bedcheck_interval;
	i4		timeout;
	i4		bchk_msg_timeout;
	SYSTIME		now;
	SYSTIME		registry_check;
	SYSTIME		cache_modified;
	i4		trace;
	i4		rtckt_created;
	i4		rtckt_used;
	i4		rtckt_expired;
	i4		rtckt_cache_miss;
	i4		ltckt_created;
	i4		ltckt_used;
	i4		ltckt_expired;
	i4		ltckt_cache_miss;
	i4		flags;

#define	GCN_MIB_INIT	0x0001		/* MIB initialized */
#define	GCN_CLUSTERED	0x0002		/* Clustered */
#define	GCN_REG_ACTIVE	0x0004		/* Installation registry active */
#define	GCN_REG_RETRY	0x0008		/* Registry retry */
#define	GCN_BCHK_CONN	0x0010		/* Bedcheck enabled */
#define	GCN_BCHK_PERM	0x0020		/* Bedcheck: check permissions */
#define	GCN_BCHK_INST	0x0040		/* Bedcheck: check installation ID */
#define	GCN_BCHK_CLASS	0x0080		/* Bedcheck: check server class */
#define	GCN_SVR_SHUT	0x0100		/* monitor: server shutdown  */
#define	GCN_SVR_QUIESCE	0x0200		/* monitor: server quiesce  */
#define	GCN_SVR_CLOSED	0x0400		/* monitor: server closed  */
#define	GCN_SVR_TERM	0x0800		/* Server termination */

	i4		registry_type;	/* Installation registry type */

#define	GCN_REG_NONE	0
#define	GCN_REG_SLAVE	1
#define	GCN_REG_PEER	2
#define	GCN_REG_MASTER	3

} GCN_STATIC;

GLOBALREF	GCN_STATIC	IIGCn_static;

/*
** Name Server internal functions.
*/

FUNC_EXTERN	STATUS		gcn_srv_init( VOID );
FUNC_EXTERN	VOID		gcn_srv_term( VOID );
FUNC_EXTERN	STATUS		gcn_srv_load( char *, char *);
FUNC_EXTERN	VOID		gcn_init_mask( char *, i4, u_i1 * );

FUNC_EXTERN	STATUS		gcn_fopen( char *, char *, FILE **, i4  );
FUNC_EXTERN	VOID		gcn_error( i4, 
					   CL_ERR_DESC *, char *, GCN_MBUF * );
FUNC_EXTERN	GCN_MBUF	*gcn_add_mbuf( GCN_MBUF *, bool, STATUS * );

FUNC_EXTERN	STATUS		gcn_register( char * );
FUNC_EXTERN	STATUS		gcn_deregister(VOID);
FUNC_EXTERN	void 		gcn_start_session(void);
FUNC_EXTERN	char *		gcn_get_server( char * );

FUNC_EXTERN	i4		gcn_oper_check( GCN_MBUF *, QUEUE * );
FUNC_EXTERN	STATUS		gcn_oper_ns( GCN_MBUF *, 
					     GCN_MBUF *, char *, bool );

FUNC_EXTERN	i4		gcn_bchk_all( QUEUE * );
FUNC_EXTERN	i4		gcn_bchk_class( char *, QUEUE * );
FUNC_EXTERN	STATUS		gcn_bchk_next( QUEUE *, GCN_RESOLVE_CB *, 
					       GCN_MBUF *, GCN_MBUF * );
FUNC_EXTERN	VOID		gcn_bchk_check( QUEUE *, STATUS, 
						i4, char *, char * );

FUNC_EXTERN	VOID		gcn_nq_file( char *, char *, char * );
FUNC_EXTERN	GCN_QUE_NAME	*gcn_nq_init(char *, char *, bool, bool, i4);
FUNC_EXTERN	GCN_QUE_NAME	*gcn_nq_create( char * );
FUNC_EXTERN	VOID		gcn_nq_drop( GCN_QUE_NAME * );
FUNC_EXTERN	GCN_QUE_NAME	*gcn_nq_find( char * );
FUNC_EXTERN	STATUS		gcn_nq_lock( GCN_QUE_NAME *, bool );
FUNC_EXTERN	VOID		gcn_nq_unlock( GCN_QUE_NAME * );
FUNC_EXTERN	bool		gcn_nq_compress( GCN_QUE_NAME * );
FUNC_EXTERN	i4		gcn_nq_add( GCN_QUE_NAME *, GCN_TUP * );
FUNC_EXTERN	i4		gcn_nq_del( GCN_QUE_NAME *, GCN_TUP * );
FUNC_EXTERN	i4		gcn_nq_qdel( GCN_QUE_NAME *, i4, PTR * );
FUNC_EXTERN	i4		gcn_nq_ret( GCN_QUE_NAME *,
					    GCN_TUP *, i4, i4, GCN_TUP * );
FUNC_EXTERN	i4		gcn_nq_scan( GCN_QUE_NAME *, GCN_TUP *, 
					     PTR *, i4, GCN_TUP **, PTR * );
FUNC_EXTERN	VOID		gcn_nq_assocs_upd(GCN_QUE_NAME *, char *, i4);
FUNC_EXTERN	bool		gcn_tup_compare( GCN_TUP *, GCN_TUP *, i4  );

FUNC_EXTERN	STATUS		gcn_nq_hash( GCN_QUE_NAME *, HASH_FUNC, i4  );
FUNC_EXTERN	VOID		gcn_nq_unhash( GCN_QUE_NAME *, bool );
FUNC_EXTERN	STATUS		gcn_nq_hadd( GCN_QUE_NAME *, GCN_TUP_QUE * );
FUNC_EXTERN	bool		gcn_nq_hdel( GCN_QUE_NAME *, GCN_TUP_QUE * );
FUNC_EXTERN	i4		gcn_nq_hscan( GCN_QUE_NAME *, GCN_TUP *, 
					      PTR *, i4, GCN_TUP **, PTR * );

FUNC_EXTERN	STATUS		gcn_rslv_init( GCN_RESOLVE_CB *, 
					       GCN_MBUF *, char *, char * );
FUNC_EXTERN	STATUS		gcn_rslv_parse( GCN_RESOLVE_CB * );
FUNC_EXTERN	STATUS		gcn_rslv_login( GCN_RESOLVE_CB * );
FUNC_EXTERN	STATUS		gcn_rslv_node( GCN_RESOLVE_CB * );
FUNC_EXTERN	STATUS		gcn_rslv_attr( GCN_RESOLVE_CB * );
FUNC_EXTERN	STATUS		gcn_rslv_server( GCN_RESOLVE_CB * );
FUNC_EXTERN	VOID		gcn_rslv_err( GCN_RESOLVE_CB *, STATUS );
FUNC_EXTERN	STATUS		gcn_rslv_done( GCN_RESOLVE_CB *,
					       GCN_MBUF *, i4, i4, PTR );
FUNC_EXTERN	VOID		gcn_rslv_cleanup( GCN_RESOLVE_CB * );
FUNC_EXTERN	VOID		gcn_unparse( char *, char *, char *, char * );
FUNC_EXTERN	bool		gcn_unquote( char ** );
FUNC_EXTERN	VOID		gcn_quote( char ** );

FUNC_EXTERN	STATUS		gcn_pwd_auth( char *, char * );
FUNC_EXTERN	STATUS		gcn_make_auth( GCN_MBUF *, GCN_MBUF *,i4 );
FUNC_EXTERN	bool		gcn_can_auth( GCN_RESOLVE_CB * );
FUNC_EXTERN	STATUS		gcn_get_auth( GCN_RESOLVE_CB *, GCN_MBUF * );
FUNC_EXTERN	STATUS		gcn_got_auth( GCN_RESOLVE_CB *, GCN_MBUF * );
FUNC_EXTERN	STATUS		gcn_use_auth( GCN_RESOLVE_CB *, i4 );
FUNC_EXTERN	i4		gcn_user_auth( i4, PTR );
FUNC_EXTERN	i4		gcn_server_auth( char *, char *, 
						 char *, i4, PTR );
FUNC_EXTERN	STATUS		gcn_rem_auth( char *, char *, 
					      i4, PTR, i4  *, PTR );
FUNC_EXTERN	STATUS		gcn_login( i4, i4, bool, 
					   char *, char *, char * );

FUNC_EXTERN	STATUS		gcn_make_tickets( char *, char *, char *, 
						   i4, i4, i4 );
FUNC_EXTERN	STATUS		gcn_save_rtickets( char *,
						   char *, char *, i4, i4  );
FUNC_EXTERN	STATUS		gcn_use_rticket( char *, char *, char *, 
						 u_i1 *, i4 * );
FUNC_EXTERN	STATUS		gcn_use_lticket( char *, u_i1 *, i4 );
FUNC_EXTERN	i4		gcn_drop_tickets( char * );
FUNC_EXTERN	bool		gcn_expire( GCN_QUE_NAME * );

FUNC_EXTERN	STATUS		gcn_direct( GCN_RESOLVE_CB *, 
					    GCN_MBUF *, i4, PTR );
FUNC_EXTERN	STATUS		gcn_resolved( GCN_RESOLVE_CB *, GCN_MBUF * );
FUNC_EXTERN	STATUS		gcn_connect_info( GCN_RESOLVE_CB *, GCN_MBUF *,
						  char *, char *, i4, PTR );

FUNC_EXTERN	STATUS		gcn_ir_init( VOID );
FUNC_EXTERN	VOID		gcn_ir_proto( GCN_RESOLVE_CB *, char * );
FUNC_EXTERN	i4		gcn_ir_comsvr( GCN_RESOLVE_CB * );
FUNC_EXTERN	STATUS		gcn_ir_bedcheck( GCN_RESOLVE_CB *, 
						 GCN_MBUF *, GCN_MBUF * );
FUNC_EXTERN	STATUS		gcn_ir_error( GCN_MBUF * );
FUNC_EXTERN	bool		gcn_ir_update( GCN_MBUF * );
FUNC_EXTERN	STATUS		gcn_ir_register(GCN_RESOLVE_CB *, GCN_MBUF *);
FUNC_EXTERN	STATUS		gcn_ir_master( GCN_RESOLVE_CB *, 
					       GCN_MBUF *, GCN_MBUF * );
FUNC_EXTERN	STATUS		gcn_ir_save( char *, char * );

/*
** Name Server general functions.
*/

FUNC_EXTERN	i4		gcn_copy_str( char *, i4, char *, i4 );
FUNC_EXTERN	i4		gcn_put_tup( char *, GCN_TUP * );
FUNC_EXTERN	i4		gcn_get_tup( char *, GCN_TUP * );
FUNC_EXTERN	STATUS		gcn_getflag( i4, i4, PTR, i4, char *); 

FUNC_EXTERN	VOID		gcn_checkerr( char *, STATUS *, 
					      STATUS, CL_ERR_DESC * );

FUNC_EXTERN	STATUS		gcn_init( VOID );
FUNC_EXTERN	STATUS		gcn_term( VOID );
FUNC_EXTERN	STATUS		gcn_request( char *, i4  *, i4 * );
FUNC_EXTERN	STATUS		gcn_recv( i4, char *, i4, bool,
					  i4  *, i4 *, bool *, bool * );
FUNC_EXTERN	STATUS		gcn_send( i4, 
					  i4, char *, i4, bool );
FUNC_EXTERN	STATUS		gcn_release( i4 );
FUNC_EXTERN	STATUS		gcn_fastselect( u_i4, char * );
FUNC_EXTERN	STATUS		gcn_testaddr( char *, i4, char * );
FUNC_EXTERN	STATUS		gcn_bedcheck( VOID );

/*
** Name Server client interface definitions.
*/

FUNC_EXTERN	STATUS		IIgcn_initiate( VOID );
FUNC_EXTERN	STATUS		IIgcn_terminate( VOID );
FUNC_EXTERN	STATUS		IIgcn_stop( VOID );
FUNC_EXTERN	STATUS		IIgcn_oper( i4, GCN_CALL_PARMS * );

/*
** Name Server admin interface definitions.
*/

# define GCN_SESS_SYS           0x0001
# define GCN_SESS_USER          0x0002
# define GCN_SESS_ADMIN         0x0004
# define GCN_SESS_FMT           0x0010

FUNC_EXTERN	STATUS 		gcn_gcadm_session ( i4, PTR, char * );
FUNC_EXTERN	STATUS 		gcn_gcadm_init( VOID );
FUNC_EXTERN	STATUS 		gcn_gcadm_term( VOID );
FUNC_EXTERN	i4 		gcn_sess_info( PTR, i4  );
FUNC_EXTERN 	i4 		gcn_verify_sid( PTR  );
FUNC_EXTERN	i4              gcn_get_session_info( PTR, GCN_SESS_INFO ** );
FUNC_EXTERN 	STATUS 		gcn_encrypt( char *, char *, char * );

/*
** GCADM interface with GCN server and iimonitor
*/
FUNC_EXTERN	STATUS		gcn_adm_init(VOID);
FUNC_EXTERN	STATUS		gcn_adm_term(VOID);
FUNC_EXTERN	STATUS		gcn_adm_session( i4, PTR, char * );
#endif
