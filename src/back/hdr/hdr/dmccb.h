/*
** Copyright (c) 1985, 2004 Ingres Corporation
**
**
*/

/**
** Name:  DMCCB.H - Typedefs for DMF control support.
**
** Description:
**      This file contains the control block used for all control
**      operations.  Note, not all the fields of a control block
**      need to be set for every operation.  Refer to the Interface
**      Design Specification for inputs required for a specific
**      operation.
**
** History:
**      01-sep-85 (jennifer)
**          Created.
**      12-jul-86 (jennifer)
**          Changed dmc_id type to pointer to i4 to be
**          same as SCF_SESSION and added new filed to return
**          size of session control block kept by SCF for DMF.
**      14-jul-86 (jennifer)
**          Update to include standard header.
**      05-aug-86 (jennifer)
**          Fixed lint problem with dmc_id.
**	16-jun-88 (teg)
**	    added two new flag values for accessing/patching an inconsistent
**	    config file -- DMC_FORCE (force it consistent) and DMC_PRETEND
**	    (pretend it's consistent and let me into database).
**	12-sep-88 (rogerk)
**	    Changed two constants in DMC_CB to be unique to 8 characters.
**	    DMC_FASTCOMMIT => DMC_FSTCOMMIT
**	    DMC_FORCE	   => DMC_MAKE_CONSISTENT
**	 6-feb-89 (rogerk)
**	    Added dmc_cache_name field to DMC_CB for specifying shared
**	    buffer manager.  Added dmc options SHARED_BUFMGR, DBCACHE_SIZE
**	    TBLCACHE_SIZE.
**	 8-Mar-89 (ac)
**	    Added 2PC support.
**	20-Apr-89 (fred)
**	    Added DMC_C_MAJOR_UDADT and DMC_C_MINOR_UDADT constants to
**	    DMC_CHAR_ENTRY data structure.
**	 3-may-89 (anton)
**	    Local collation support
**      14-JUL-89 (JENNIFER)
**          Added support for starting server as C2 secure.
**	20-aug-89 (ralph)
**	    Add DMC_CVCFG for config file conversion
**	12-oct-89 (sandyh)
**	    Changed DMC_LOC_ENTRY->loc_extent[64] to DB_AREA_MAX
**	08-aug-90 (ralph)
**	    Added dmc_cb.dmc_show_state so SCF can get server's security state
**	10-dec-90 (rogerk)
**	    Added DMC_C_SCANFACTOR startup parameter so the user can direct
**	    the system as to the efficiency of multi-block IO's.  Added
**	    float parameter value to dmc_cb characteristic options.
**	 4-feb-91 (rogerk)
**	    Added DMC_C_LOGGING alter option for Set Nologging statement.
**      26-MAR-1991 (JENNIFER)
**          Added DMC_S_DOWNGRADE definition so that user downgrade privilege
**          can be passed to DMF for use.
**	07-apr-1992 (jnash)
**	    Add DMC_C_LOCK_CACHE for locking shared dmf cache.
**	13-may-1992 (bryanp)
**	    Added DMC_C_INT_SORT_SIZE for specifying the DMF sorter's internal
**	    sort size threshold.
**	16-jun-1992 (bryanp)
**	    Added DMC_RECOVERY server startup option.
**	8-oct-1992 (bryanp)
**	    Added DMC_C_CHECK_DEAD_INTERVAL for specifying the check-dead
**	    interval. Added Group Commit characteristics.
**	02-nov-92 (jrb)
**	    Minor changes for multi-location sorts.
**	14-dec-92 (jrb)
**	    Removed unneeded word "typedef"; it caused Unix compiler warnings.
**	18-jan-1993 (bryanp)
**	    Added DMC_C_CP_TIMER for specifying Consistency Point timer.
**	31-mar-1993 (ralph)
**	    DELIM_IDENT:
**	    Added dmc_dbxlate to DMC_CB for passing case translation flags.
**	    Added dmc_cat_owner to DMC_CB for passing catalog owner name.
**	20-may-93 (robf)
**	    Secure 2.0: Add DMC_XACT_ID, used in DMC_SHOW to get transaction id
**	23-aug-1993 (bryanp)
**	    Added DMC_STAR_SVR startup option flag for starting minimal DMF.
**	29-sep-93 (jrb)
**	    Added DMC_ADMIN_DB flag so that SCF can tell DMF when the iidbdb
**	    is being opened only for administrative purposes (e.g. to validate
**	    a username) as opposed to opening it as a target db.  DMF wants to
**	    know this so that it can log a warning whenever the iidbdb is a 
**	    target db but is not being journaled (not recommended).
**	18-oct-93 (swm)
**	    Added dmc_session_id. This is currently unused. In my next
**	    integration dmc_id will be changed back to i4 and
**	    the overloading of the old dmc_id with two different
**	    uses will be eliminated.
**	10-oct-93 (swm)
**	    Added PTR char_pvalue for option values which are pointers.
**	    This is needed for DMC_C_ABORT option value which is the address
**	    of a function to call when a force-abort occurs.
**	    This field is currently unused, it has been added as a hook for
**	    my next integration.
**      04-apr-1995 (dilma04)
**          Add support for SET TRANSACTION ISOLATION LEVEL:
**            - DMC_SETTRANSACTION - a new value for the dmc_flags_mask field
**          of DMC_CB;
**            - DMC_C_ISOLATION_LEVEL - a new type of characteristic with four
**          possible values.  
**      24-Apr-1995 (cohmi01)
**          Add DMC_IOMASTER to indicate disk I/O master process, which
**          handles write-along and read-ahead threads.
**	05-may-1995 (cohmi01)
**	    Add DMC_BATCHMODE flag
**	02-jun-1995 (cohmi01)
**	    Add DMC_C_NUMRAHEAD for dmc_start to know how much mem is needed.
**	06-mar-1996 (nanpr01)
**	    Add DMC_BMPOOL_OP for enquiring about buffer pool availability.
**	    Add DMC_C_BMPOOL for enquiring about buffer pool availability
**	    in char_id field.
**	    Add DMC_MAXTUP in dmc_flags_mask for enquiring about 
**	    max tuple size in installation.
**	    Add DMC_C_MAXTUP in char_id for returning the value. 
**	    Add DMC_TUPSIZE in dmc_flags_mask for enquiring about 
**	    tuple size given the page size.
**	    Add DMC_C_TUPSIZE in char_id for returning the value. 
**      06-mar-1996 (stial01)
**          Added defines for buffer cache parameters for 4k,8k,16k,32k,64k
**          Added DMC_C_MAXTUPLEN
**	 24-may-96 (stephenb)
**	    Add DMC_C_REP_QSIZE define for DBMS replication.
**      01-aug-1996 (nanpr01 for ICL phil.p)
**          Introduced new value DMC_C_DMCM to DMC_CHAR_ENTRY.char_id to
**          support the Distributed Multi-Cache Management (DMCM) protocol.
**      22-nov-1996 (dilma04)
**          Row-level locking project:
**          Add DMC_C_ROW value definition.
**      25-nov-1996 (nanpr01)
**          Sir 79075 : allow setting the default page size through CBF.
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          Change char_value defines for isolation levels in DMC_CHAR_ENTRY.
**	27-Feb-1997 (jenjo02)
**	    Added support for SET TRANSACTION <access mode>:
**	      - DMC_C_TRAN_ACCESS_MODE a new characteristic with the value
**	                          DMC_C_READ_ONLY or DMC_C_READ_WRITE.
**	    Added support for SET SESSION <access mode>
**	21-aug-1997 (nanpr01)
**	    FIPS upgrade need version numbers etc.
**      14-oct-1997 (stial01)
**          Added support for SET SESSION ISOLATION LEVEL (DMC_SET_SESS_ISO)
**          and dmc_isolation.
**          Added defines for READLOCK values.
**	29-aug-1997 (bobmart)
**	    Added char_ids DMC_C_LOG_ESC_LPR_SC, DMC_C_LOG_ESC_LPR_UT,
**	    DMC_C_LOG_ESC_LPT_SC, DMC_C_LOG_ESC_LPT_UT and char_values
**	    DMC_C_DISABLE, DMC_C_ENABLE for controlling the logging of
**	    lock escalation messages when exceeding max locks per 
**	    resource or transaction for system catalogs and user tables.
**	25-nov-97 (stephenb)
**	    add DMC_C defines for replicator locking parameters.
**	13-May-1998 (jenjo02)
**	    Added DMC_C_nK_WRITEBEHIND parameters.
**      31-aug-1999 (stial01)
**          Added DMC_C_ETAB_PAGE_SIZE
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Jan-2001 (jenjo02)
**	    Added dmc_dmscb_offset, dmc_adf_cb, dmc_gwf_cb.
**      09-mar-2001 (stial01)
**          Changes for Variable Page Type SIR 102955
**	11-May-2001 (jenjo02)
**	    Deleted unneeded loc_raw_blocks from DMC_LOC_ENTRY.
**      08-Oct-2002 (hanch04)
**          Replace the i4 for length with a SIZE_TYPE (size_t) to allow
**          64-bit memory allocation.
**	20-Aug-2002 (jenjo02)
**	    Added lock timeout value of DMC_C_NOWAIT in support
**	    of SET LOCKMODE ... TIMEOUT=NOWAIT
**      22-oct-2002 (stial01)
**          Added dmc_scb_lkid, DMC_SESS_LLID to DMC_CB
**      16-sep-2003 (stial01)
**          Added DMC_C_ETAB_TYPE (SIR 110923)
**      02-jan-2004 (stial01)
**          Changes for SET [NO]BLOBLOGGING
**      14-apr-2004 (stial01)
**	    Added DMC_C_MAXBTREEKEY (SIR 108315)
**      27-aug-2004 (stial01)
**          Removed SET NOBLOBLOGGING statement
**      20-sep-2004 (devjo01)
**          Add DMC_CLASS_NODE_AFFINITY.
**      06-oct-2004 (stial01)
**          Added DMC_C_PAGETYPE_V5
**      08-nov-2004 (stial01)
**	    Added DMC_C_DOP
**      30-mar-2005 (stial01)
**          Added DMC_C_PAD_BYTES
**      05-Aug-2005 (hanje04)
**          Back out change 478041 as it should never have been X-integrated
**          into main in the first place.
**       8-Jun-2009 (hanal04) Code Sprint SIR 122168 Ticket 387
**          Added DMC2_MUST_LOG.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**      09-aug-2010 (maspa05) b123189, b123960
**          Added DMC2_READONLYDB to indicate a readonly database
**	17-Nov-2010 (jonj) SIR 124738
**	    Add DMC2_NODBMO to prevent making MO objects for
**	    database, typically when fetching iidbdb information.
**/


/*
**  This section includes all forward references required for defining
**  the DMC structures.
*/
typedef struct _DMC_LLIST_ITEM	    DMC_LLIST_ITEM;


/*}
** Name:  DMC_CB - DMF control call control block.
**
** Description:
**      This typedef defines the control block required for 
**      the following DMF control operations:
**
**      DMC_ALTER - Alter the DMF environment.
**      DMC_SHOW - Show the current DMF environment.
**      DMC_START_SERVER - Initialize the DMF server environment.
**      DMC_STOP_SERVER - Shutdown the DMF server environment.
**      DMC_BEGIN_SESSION - Start a thread for a user.
**      DMC_END_SESSION - End a thread for a user.
**      DMC_OPEN_DB - Open a database for this user/session.
**      DMC_CLOSE_DB - Close a database for this user/session.
**      DMC_ADD_DB - Add a database to be handled by server.
**      DMC_DELETE_DB - Delete a database from server.
**
**      Note:  Included structures are defined in DMF.H if 
**      they start with DM_, otherwise they are define as 
**      global DBMS types.
**
** History:
**      01-sep-85 (jennifer)
**          Created.
**      12-jul-86 (jennifer)
**          Changed dmc_id type to pointer to i4 to be
**          same as SCF_SESSION and added new filed to return
**          size of session control block kept by SCF for DMF.
**      14-jul-86 (jennifer)
**          Update to include standard header.
**      05-aug-86 (jennifer)
**          Fixed lint problem with dmc_id.
**	16-jun-88 (teg)
**	    added two new flag values for accessing/patching an inconsistent
**	    config file -- DMC_FORCE (force it consistent) and DMC_PRETEND
**	    (pretend it's consistent and let me into database).
**	12-sep-88 (rogerk)
**	    Changed two constants so they would be unique to 8 characters.
**	    DMC_FASTCOMMIT => DMC_FSTCOMMIT
**	    DMC_FORCE	   => DMC_MAKE_CONSISTENT
**	 6-jan-89 (rogerk)
**	    Added dmc_cache_name field for specifying shared buffer manager.
**	    Added DMC_SOLECACHE flag.
**	 2-may-89 (anton)
**	    Local collation support
**      14-JUL-89 (JENNIFER)
**          Added support for starting server as C2 secure.
**	20-aug-89 (ralph)
**	    Add DMC_CVCFG for config file conversion
**	08-aug-90 (ralph)
**	    Added dmc_cb.dmc_show_state so SCF can get server's security state
**      26-MAR-1991 (JENNIFER)
**          Added DMC_S_DOWNGRADE definition so that user downgrade privilege
**          can be passed to DMF for use.
**	16-jun-1992 (bryanp)
**	    Added DMC_RECOVERY server startup option.
**	02-nov-92 (jrb)
**	    Added DMC_LOC_MODE, DMC_NOAUTH_LOC, DMC_ADD_LOC, DMC_DROP_LOC,
**	    dmc_type_loc, and dmc_name_loc for multi-location sorting project.
**	31-mar-1993 (ralph)
**	    DELIM_IDENT:
**	    Added dmc_dbxlate to DMC_CB for passing case translation flags.
**	    Added dmc_cat_owner to DMC_CB for passing catalog owner name.
**	23-aug-1993 (bryanp)
**	    Added DMC_STAR_SVR startup option flag for starting minimal DMF.
**	29-sep-93 (jrb)
**	    Added DMC_ADMIN_DB flag so that SCF can tell DMF when the iidbdb
**	    is being opened only for administrative purposes (e.g. to validate
**	    a username) as opposed to opening it as a target db.  DMF wants to
**	    know this so that it can log a warning whenever the iidbdb is a 
**	    target db but is not being journaled (not recommended).
**	10-oct-93 (swm)
**	    Bugs #56441 #56437
**	    Added PTR char_pvalue for option values which are pointers.
**	    This is needed for DMC_C_ABORT option value which is the address
**	    of a function to call when a force-abort occurs.
**	    Changed dmc_id back to i4 and use new dmc_session_id. This is
**	    to eliminate the overloading of the old dmc_id with two different
**	    uses.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with
**	    DM0M_OBJECT.
**      24-Apr-1995 (cohmi01)
**          Add DMC_IOMASTER to indicate disk I/O master process, which
**          handles write-along and read-ahead threads.
**	06-mar-1996 (nanpr01)
**	    Add DMC_BMPOOL_OP for enquiring about buffer pool availability.
**	    Add DMC_MAXTUP in dmc_flags_mask for enquiring about 
**	    max tuple size in installation.
**	01-aug-1996 (nanpr01 for ICL phil.p)
**          Introduced dmc_flags_mask value of DMC_DMCM to indicate that
**          database is opened with the Distributed Multi-Cache Managemen
**          (DMCM) protocol.
**      27-May-1999 (hanal04)
**          Added DMC_CNF_LOCKED to indicate the calling facility has
**          already locked the CNF file.
**          Replaced l_reserved with dmc_lock_list to hold the lock list id 
**          of the lock list on which the lock is held. b97083.
**	26-mar-2001 (stephenb)
**	    Add dmc_ucoldesc
**	05-May-2004 (jenjo02)
**	    Added DMC_S_FACTOTUM session startup flag.
**      20-sep-2004 (devjo01)
**          Add DMC_CLASS_NODE_AFFINITY.
**	6-Jul-2006 (kschendel)
**	    Add place to return unique db id (the infodb dbid, ie from the
**	    config file).
**	13-May-2008 (kibro01) b120369
**	    Add dmc_flags_mask2 and DMC2_ADMIN_CMD (in DMC_CB structure).
**	12-Nov-2009 (kschendel) SIR 122882
**	    Change compatlvl to an integer.
*/
typedef struct _DMC_CB
{
    PTR             q_next;
    PTR             q_prev;
    SIZE_TYPE       length;                 /* Length of control block. */
    i2		    type;                   /* Control block type. */
#define                 DMC_CONTROL_CB      7
    i2              s_reserved;
    PTR             dmc_lock_list;
    PTR             owner;
    i4         ascii_id;             
#define                 DMC_ASCII_ID        CV_C_CONST_MACRO('#', 'D', 'M', 'C')
    DB_ERROR        error;                  /* Common DMF error block. */
    i4         dmc_op_type;            /* Type of control operation. */
#define                 DMC_SERVER_OP       1L
#define                 DMC_SESSION_OP      2L
#define                 DMC_DATABASE_OP     3L
#define                 DMC_BMPOOL_OP       4L
#define			DMC_DBMS_CONFIG_OP  5L
    PTR		    dmc_session_id;	    /* An SCF_SESSION id
                                            ** for the active session
                                            ** obtained from SCF. */
    i4   	    dmc_id;                 /* DMF SERVER ID  
                                            ** returned as a result
                                            ** of the start server  
                                            ** operation
					    */
    PTR		    dmc_server;		    /* DMF server control block info. */
    DM_DATA         dmc_name;               /* Server or session name
                                            ** depending on operation. */
    i4         dmc_flags_mask;         /* Modifier to operation. */
#define                 DMC_NOJOURNAL       0x01L
#define                 DMC_JOURNAL         0x02L
#define                 DMC_DEBUG           0x04L
#define                 DMC_NODEBUG         0X08L
#define                 DMC_SETLOCKMODE     0X10L
#define                 DMC_NOWAIT	    0X20L
#define                 DMC_SYSCAT_MODE     0x40L
#define                 DMC_FORCE_END_SESS  0x80L
#define                 DMC_ALOCATION       0x100L
#define                 DMC_FSTCOMMIT	    0x200L
#define			DMC_PRETEND	    0x400L 
						   /* pretend config file's
						   ** status = DSC_VALID */
#define			DMC_MAKE_CONSISTENT 0x800L 
						   /* force config file's
						   ** status to DSC_VALID */
#define			DMC_SOLECACHE	    0x1000L 
						    /* database can be served
						    ** by only one bufmgr  */
#define			DMC_NLK_SESS	    0x2000L 
						    /* Don't need to request
						    ** session database lock.
						    */
#define			DMC_CVCFG	    0x4000L 
						    /* OK to convert the
						    ** config file
						    */
#define			DMC_C2_SECURE	    0x8000L 
						    /* Server is running C2.
						    */
#define			DMC_RECOVERY	    0x00010000L
						    /* This server is actually
						    ** a recovery server, not
						    ** a normal server. A
						    ** recovery server is the
						    ** 6.5 equivalent of the
						    ** old "dmfrcp" process.
						    */
#define			DMC_LOC_MODE	    0x00020000L
						    /* Change set of work
						    ** or data locations for
						    ** this session.
						    */
#define			DMC_STAR_SVR	    0x00040000L
						    /* This is not a normal
						    ** DMF server; it is a STAR
						    ** server and DMF need
						    ** support ONLY the
						    ** DMT_SORT_COST operation.
						    ** Any other dmf_call is
						    ** illegal and may access-
						    ** violate.
						    */
#define			DMC_ADMIN_DB	    0x00080000L
						    /* We are opening this DB
						    ** for administrative 
						    ** purposes only.  (The
						    ** current use for this flag
						    ** is to tell DMF when the
						    ** iidbdb is being opened
						    ** for user info only as
						    ** opposed to being the 
						    ** actual target DB.)
						    */

#define                 DMC_SETTRANSACTION  0x00100000L
                                                    /* SET TRANSACTION */
#define                 DMC_BATCHMODE       0x00200000L /* batch server */
#define			DMC_IOMASTER	    0x00400000L
						    /*
						    ** This server is an
						    ** Input/Output Master
						    ** server, responsible
						    ** for performing certain
						    ** disk operations on behalf
						    ** of all other servers,
						    ** such as write-along and
						    ** read-ahead operations.
						    */
#define			DMC_MAXTUP  	    0x00800000L
						    /*
						    ** For inquiring about 
						    ** max tuple size in 
						    ** the installation
						    */

#define                 DMC_DMCM            0x01000000L
                                                    /* DB opened with the
                                                    ** DMCM protocol.
                                                    */

#define			DMC_TUPSIZE  	    0x02000000L
						    /*
						    ** For getting tuple size
						    ** given a page size
						    */

#define			DMC_XACT_ID	    0x04000000L
						    /*
						    ** Request session xact id
						    ** in DMC_SHOW
						    */
#define                 DMC_CLASS_NODE_AFFINITY 0x08000000L
						    /*
						    ** Server class can be
						    ** active only on one
						    ** node.
						    */
#define			DMC_SET_SESS_AM     0x10000000L
						    /* Set session access mode */
#define                 DMC_SET_SESS_ISO    0x20000000L
						    /* Set session isolation */
#define                 DMC_CNF_LOCKED      0x40000000L
                                                    /* CNF file locked */
#define			DMC_SESS_LLID	    0x80000000L
						    /* Request session lock
						    ** list in DMC_SHOW
						    */
    i4         dmc_flags_mask2;         /* More modifiers to operation. */
#define			DMC2_ADMIN_CMD	    0x00000001L
			/* Coming from any administration command, so 
			** don't try to replicate on open_db
			*/
#define                 DMC2_MUST_LOG       0x00000002L
                        /* Database must be logged when access from this DBMS */
#define                 DMC2_READONLYDB     0x00000004L
                        /* readonlydb */
#define                 DMC2_NODBMO	    0x00000008L
                        /* Don't make MO objects when adding DB */

    DM_DATA         dmc_char_array;	/* Pointer to an array of 
					** configuration variables. */
    DB_DB_NAME      dmc_db_name;	/* Database name. */
    char	    *dmc_db_id;		/* DMF DATABASE ID returned as a
					** result of a database open or add.
					** NOTE that what comes back from an
					** add is different from what is
					** returned from a db open!  the open
					** returns the "dmf db id" (really an
					** ODCB ptr) to be used by everyone
					** else as a db id ptr cookie.
					*/
    i4		dmc_udbid;		/* Unique (infodb) db id used mainly
					** by RDF;  session independent.
					** Returned by database open.
					*/
    i4		dmc_lock_mode;		/* The type of lock mode to set
                                        ** for the database. */
#define			DMC_L_SHARE	    1L
#define			DMC_L_EXCLUSIVE	    2L
    DB_OWN_NAME     dmc_db_owner;           /* Database owner. */    
    i4	    dmc_scfcb_size;         /* Size of session control 
                                            ** maintained by SCF for DMF.
                                            ** Used only with start server
                                            ** as an output parameter. */ 
    i4         dmc_s_type;             /* Mask to indicate the type
                                            ** of session or type of 
                                            ** database service. */
#define			DMC_S_NORMAL	    0L
					    /* For session
                                            ** has no update priviledge on
                                            ** system catalogs.
	                                    */
#define			DMC_S_SYSUPDATE	    0x01L 
					    /* For session
                                            ** has update privilege on
                                            ** system catalogs.
	                                    */
#define			DMC_S_SECURITY	    0x02L 
					    /* For session
                                            ** has update privilege on
                                            ** system catalogs.
	                                    */
#define			DMC_S_AUDIT	    0x04L 
					    /* For session
                                            ** all activity is audited.
	                                    */
#define			DMC_S_SINGLE	    0x04L 
					    /* For add database
                                            ** will have only a single
                                            ** server per database.
	                                    */
#define			DMC_S_MULTIPLE	    0x08L 
					    /* For add database
                                            ** will have multiple servers
                                            ** per database.
	                                    */
#define			DMC_S_DOWNGRADE	    0x10L 
					    /* For begin session
                                            ** user has downgrade privilege.
	                                    */
#define			DMC_S_NOAUTH_LOCS   0x20L
					    /* For session.
					    ** Remove authorization to use
					    ** given work/data locations.
					    */
#define			DMC_S_ADD_LOCS	    0x40L
					    /* For session.
					    ** Use these work locations
					    ** for this session (can be done
					    ** only if authorized).
					    */
#define			DMC_S_DROP_LOCS	    0x80L
					    /* For session.
					    ** Don't use these work locations
					    ** for this session anymore.
					    */
#define			DMC_S_TRADE_LOCS    0x100L
					    /* For session.
					    ** Use these work locations instead
					    ** of the ones currently being used.
					    */
#define			DMC_S_FACTOTUM      0x200L
					    /* For session.
					    ** This is a Factotum thread.
					    */

    i4         dmc_sl_scope;           /* Scope of set lockmode call. */
#define			DMC_SL_TABLE	    1L
#define			DMC_SL_SESSION	    2L 
                                            /* Must be table or session. */
    DB_TAB_ID       dmc_table_id;           /* The table identifier for a set
                                            ** set lockmode operation. */
    DM_DATA         dmc_db_location;        /* Pointer to area containing
                                            ** database location. */
    i4         dmc_db_access_mode;     /* Type of access allowed to a
                                            ** database. */
#define                 DMC_A_READ	    1L
#define                 DMC_A_WRITE	    2L
#define			DMC_A_CREATE	    3L
#define			DMC_A_DESTROY	    4L

    i4         dmc_isolation; 

    char	    *dmc_cache_name;	    /* Cache name for shared buffer
					    ** manager. */
    PTR		    dmc_coldesc;	    /* collation descriptor */
    PTR		    dmc_ucoldesc;	    /* Unicode collation descriptor */
    DB_STATUS	    (*dmc_show_state)();    /* Rtn to show security state */
    char	    dmc_collation[DB_COLLATION_MAXNAME]; /* collation name */
    char	    dmc_ucollation[DB_COLLATION_MAXNAME];/* Unicode collation */
    DMC_LLIST_ITEM  *dmc_names_loc;	    /* location names to alter */
    DB_LOC_NAME	    *dmc_error_loc;	    /* name of location causing error
					    ** (used only if error occurs)
					    */
    i4	    dmc_errlen_loc;	    /* length of above location name */
    u_i4	    *dmc_dbxlate;	    /* Case translation semantic flags;
					    ** See cui.h for values
					    */
    DB_OWN_NAME	    *dmc_cat_owner;	    /* Catalog owner name.  Will be
					    ** "$ingres" if regular ids lower;
					    ** "$INGRES" if regular ids upper.
					    */
    PTR		     dmc_xact_id;	    /*
					    ** Transaction id returned as
					    ** result of DMC_XACT_ID request
					    */
    u_i4	dmc_dbcmptlvl;		/* Major compat level for DB */
    i4		dmc_1dbcmptminor;	/* Minor compat no. for DB */
    i4		dmc_dbservice;		/* FIPS related case trans flags */
    i4		     dmc_dmscb_offset;	    /* Offset of *DML_SCB from
					    ** SCF's SCB */
    PTR		     dmc_adf_cb;	    /* Ptr to session's ADF_CB */
    PTR		     dmc_gwf_cb;	    /* Ptr to session's GW_SESSION */
    PTR		     dmc_scb_lkid;	    /*
					    ** Session lock list id returned as
					    ** result of DMC_SESS_LKID request
					    */
}   DMC_CB;

/*}
** Name:  DMC_CHAR_ENTRY - Structure for setting configuration characteristics.
**
** Description:
**      This typedef defines the structure needed to describe a 
**      configuration characteristics which can be altered or obtained.
**      Characteristics are defined for the server, session and database
**      configurations.
**
**	Most characteristic values are of integer type and are set in the
**	char_value field.  Characteristic values that are of floating point
**	type or some pointer type should be set in the char_fvalue or
**	char_pvalue field, respectively.
**
**      Note:  Included structures are defined in DMF.H if
**      they start with DM_, otherwise they are define as 
**      global DBMS types.
**
** History:
**      01-sep-85 (jennifer)
**          Created.
**	20-apr-89 (fred)
**	    Added DMC_C_{MAJOR,MINOR}_UDADT constants and DMC_C_UNKNOWN value
**	    for user defined ADT support.
**	10-dec-90 (rogerk)
**	    Added DMC_C_SCANFACTOR startup parameter so the user can direct
**	    the system as to the efficiency of multi-block IO's.
**	    Added char_fvalue field so floating point parameters can also
**	    be passed (new SCANFACTOR parameter is a float).
**	 4-feb-91 (rogerk)
**	    Added DMC_C_LOGGING alter option for Set Nologging statement.
**	07-apr-1992 (jnash)
**	    Add DMC_C_LOCK_CACHE for locking shared dmf cache.
**	13-may-1992 (bryanp)
**	    Added DMC_C_INT_SORT_SIZE for specifying the DMF sorter's internal
**	    sort size threshold.
**	8-oct-1992 (bryanp)
**	    Added DMC_C_CHECK_DEAD_INTERVAL for specifying the check-dead
**	    interval. Added Group Commit characteristics.
**	18-jan-1993 (bryanp)
**	    Added DMC_C_CP_TIMER for specifying Consistency Point timer.
**	02-jan-94 (andre)
**	    defined DMC_C_DB_JOURN for reporting journaling status of a database
**	5-jan-93 (robf)
**          Added DMC_C_XACT_TYPE for DMC_SHOW.
**	06-mar-1996 (nanpr01)
**	    Add DMC_C_BMPOOL  for enquiring about buffer pool availability.
**	    Add DMC_C_MAXTUP for returning the max tuple size of the 
**	    installation.
**      01-aug-1996 (nanpr01 for ICL phil.p)
**          Added DMC_C_DMCM to indicate whether or not a server is running
**          the Distributed Multi-Cache Management (DMCM) protocol.
**      22-nov-1996 (dilma04)
**          Row-level locking project:
**          Add DMC_C_ROW value definition. 
**      25-nov-1996 (nanpr01)
**          Sir 79075 : allow setting the default page size through CBF.
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          Change char_value defines for isolation levels.
**	27-Feb-1997 (jenjo02)
**	    Added support for SET TRANSACTION <access mode>:
**	      - DMC_C_TRAN_ACCESS_MODE a new characteristic with the value
**	                          DMC_C_READ_ONLY or DMC_C_READ_WRITE.
**	29-aug-1997 (bobmart)
**	    Added char_ids DMC_C_LOG_ESC_LPR_SC, DMC_C_LOG_ESC_LPR_UT,
**	    DMC_C_LOG_ESC_LPT_SC, DMC_C_LOG_ESC_LPT_UT and char_values
**	    DMC_C_DISABLE, DMC_C_ENABLE for controlling the logging of
**	    lock escalation messages when exceeding max locks per 
**	    resource or transaction for system catalogs and user tables.
**	26-marc-1998 (nanpr01)
**	    Added DMC_C_BLOB_ETAB_STRUCT for default blob structure.
**	13-apr-98 (inkdo01)
**	    Added DMC_C_PSORT_BSIZE for default parallel sort exchange
**	    buffer size (in rows).
**	26-may-1998 (nanpr01)
**	    Added DMC_C_PIND_BSIZE & DMC_C_PIND_NBUFFERS for default
**	    exchange buffer size in rows and number of buffers.
**	20-Aug-2002 (jenjo02)
**	    Added lock timeout value of DMC_C_NOWAIT in support
**	    of SET LOCKMODE ... TIMEOUT=NOWAIT
**	8-Mar-2004 (schka24)
**	    Added dmf tcb limit.
**	20-Apr-2004 (jenjo02)
**	    Added support for SET SESSION ON_LOGFULL 
**	27-july-06 (dougi)
**	    Added DMC_C_SORT_CPU/IOFACTOR to adjust sort cost estimates.
**      30-Aug-2006 (kschendel)
**          Add ID for build buffer size (build-pages).
**	05-Nov-2007 (wanfr01)
**	    SIR 119419
**	    Added DMC_C_CORE_ENABLED
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Added DMC_C_MVCC lock level, DMC_C_PAGETYPE_V6
**	    DMC_C_PAGETYPE_V7
**	13-Apr-2010 (toumi01) SIR 122403
**	    Add DMC_C_CRYPT_MAXKEYS for data at rest encryption.
**	19-Nov-2010 (kiria01) SIR 124690
**	    Add support for controlling the defaulting of the collation type
*/
typedef struct _DMC_CHAR_ENTRY
{
    i4         char_id;                /* Characteristic identifier. */
#define                 DMC_C_LSESSION      1L	    /*	Session limit. */
#define                 DMC_C_DATABASE_MAX  2L	    /*  Database limit. */
#define                 DMC_C_MEMORY_MAX    3L	    /*  Memory limit in K. */
#define                 DMC_C_TRAN_MAX      4L	    /*	Transaction limit. */ 
#define			DMC_C_BUFFERS	    5L	    /*	Page Buffers. */
#define			DMC_C_GBUFFERS	    6L	    /*  Group Buffers. */
#define			DMC_C_GCOUNT	    7L	    /*  Group Buffer count. */
#define			DMC_C_TCB_HASH	    8L	    /*	TCB hash table size. */
#define                 DMC_C_LTIMEOUT      9L
                                            /* The value for this
                                            ** is either DMC_C_SESSION,
                                            ** DMC_C_SYSTEM, DMC_C_NOWAIT,
					    ** or a positive number. */
#define                 DMC_C_LLEVEL        10L
                                            /* The value for this
                                            ** is either DMC_C_SESSION,
                                            ** DMC_C_SYSTEM, DMC_C_ROW,
                                            ** DMC_C_PAGE, DMC_C_TABLE
					    ** or DMC_C_COMMITTED. */
#define                 DMC_C_LMAXLOCKS     11L
                                            /* The value for this
                                            ** is either DMC_C_SESSION,
                                            ** DMC_C_SYSTEM, or a positive
                                            ** number. */
#define                 DMC_C_LREADLOCK     12L
                                            /* The value for this
                                            ** is either DMC_C_SESSION,
                                            ** DMC_C_SYSTEM, DMC_C_X for
                                            ** exclusive readlock,
                                            ** DMC_C_S for shared readlock,
                                            ** or DMC_C_N for nolock. */
#define                 DMC_C_ABORT	    13L
					    /* The value for this is the 
					    ** server exception handler for
					    ** the force abort event.
					    */
#define			DMC_C_MLIMIT	    14L
					    /* The upper bound of modified pages
					    ** in the buffer manager. */
#define			DMC_C_FLIMIT	    15L
					    /* The lower bound of free pages
					    ** in the buffer manager. */
#define			DMC_C_WSTART	    16L
					    /* The number of modified pages at
					    ** which to start asynchronous
					    ** write behind. */
#define			DMC_C_WEND	    17L
					    /* The number of modified pages at
					    ** which to stop asynchronous
					    ** write behind. */
#define			DMC_C_SHARED_BUFMGR 18L
					    /* Use Shared Memory Buffer Cache */
#define			DMC_C_DBCACHE_SIZE  19L
					    /* The number of databases that
					    ** can remain cached in the Buffer
					    ** Manager while closed. This is
					    ** the number of entries in the
					    ** db cache array. */
#define			DMC_C_TBLCACHE_SIZE 20L
					    /* The number of tables that
					    ** can remain cached in the Buffer
					    ** Manager while closed. This is
					    ** the number of entries in the
					    ** tbl cache array. */
#define			DMC_C_MAJOR_UDADT   21L
#define			DMC_C_MINOR_UDADT   22L
					    /* The major and minor id's
					    ** for the current set of user
					    ** defined datatypes in use.  These
					    ** are requested by SCF at server
					    ** startup time, and compared
					    ** against the set proposed for use.
					    ** If incompatible, the server will
					    ** shutdown immediately.  These
					    ** values are output to SCF, not
					    ** input to DMF.  They are obtained
					    ** from a lock-value (currently) set
					    ** by the recovery system (RCP or
					    ** CSP).
					    */
#define			DMC_C_SCANFACTOR    23L
					    /* A factor that describes the
					    ** efficiency of multi-block
					    ** IO's in this environment.  Its
					    ** value indicates the savings
					    ** factor of doing one multi-block
					    ** IOs as opposed to doing multiple
					    ** single-page IOs.  The scanfactor
					    ** is passed as a floating point
					    ** parameter.
					    */
#define			DMC_C_LOGGING	    24L
					    /* Set [No]Logging option. */
#define			DMC_C_INT_SORT_SIZE 25L
					    /* Specifies the threshold between
					    ** "internal" and "external" sorts
					    ** for the DMF sorter. If the
					    ** estimated size of a DMF sort is
					    ** less than this value, DMF will
					    ** attempt to perform the sort
					    ** internally (i.e., without I/O).
					    */
#define                 DMC_C_LOCK_CACHE    26L
					    /* Request DMF cache locking. */
#define			DMC_C_CHECK_DEAD_INTERVAL    27L
					    /* check-dead interval */
#define			DMC_C_GC_INTERVAL   28L
#define			DMC_C_GC_NUMTICKS   29L
#define			DMC_C_GC_THRESHOLD  30L
#define			DMC_C_CP_TIMER	    31L
					    /* DMC_C_CP_TIMER specifies the
					    ** consistency point timer interval
					    ** in seconds. Recovery Server only.
					    */
#define			DMC_C_DB_JOURN      32L
					    /*
					    ** if the caller requested (by 
					    ** calling DMC_SHOW) the journaling
					    ** status of a database, char_value 
					    ** will be set to DMC_C_ON if the 
					    ** database is being journaled and 
					    ** to DMC_C_OFF otherwise
					    */
#define                 DMC_C_ISOLATION_LEVEL 33L
                                            /* SET TRANSACTION ISOLATION LEVEL */
#define			DMC_C_NUMRAHEAD	34L /* number of readahead threads */
#define			DMC_C_BMPOOL    35L /*
					    ** if the caller requested (by 
					    ** calling DMC_SHOW) the bufferpool 
					    ** availability status of a server
					    ** char_id will be set to 
					    ** DMC_C_BMPOOL and char_value
					    ** will be set to DMC_C_ON if the 
					    ** bufferpool is available and 
					    ** to DMC_C_OFF otherwise
					    */
#define			DMC_C_MAXTUP    36L /*
					    ** return the max tuple size of
					    ** the installation depending
					    ** on buffer pool availability
					    */
#define			DMC_C_TUPSIZE   37L /*
					    ** returning the tuple size 
					    ** given a page
					    */
#define			DMC_C_TRAN_ACCESS_MODE 38L
					    /* SET TRANSACTION ACCESS MODE */
#define			DMC_C_XACT_TYPE 40L
					    /* DMC_C_XACT_TYPE returns
					    ** the transaction type (READ or
					    ** WRITE)
					    */
#define                 DMC_C_DMCM          41L
                                            /*
                                            ** Whether or not DMCM is 
                                            ** activated for a server.
                                            */
#define                 DMC_C_LOG_READNOLOCK 42L 
					    /*
					    ** Whether or not to log if 
					    ** readlock=nolock reduces iso level
					    */
#define			DMC_C_LOG_ESC_LPR_SC  43L
					    /*
					    ** Log lock escalation message to
					    ** errlog when hitting max locks 
					    ** per resource for system catalog
					    */
#define			DMC_C_LOG_ESC_LPR_UT  44L
					    /*
					    ** Log lock escalation message to
					    ** errlog when hitting max locks 
					    ** per resource for user table
					    */
#define			DMC_C_LOG_ESC_LPT_SC  45L
					    /*
					    ** Log lock escalation message to
					    ** errlog when hitting max locks
					    ** per transaction for sys catalog
					    */
#define			DMC_C_LOG_ESC_LPT_UT  46L
					    /*
					    ** Log lock escalation message to
					    ** errlog when hitting max locks
					    ** per transaction for user table
					    */

#define			DMC_C_2K_WRITEBEHIND 49L /* 2k WriteBehind. */
#define			DMC_C_4K_BUFFERS     50L /* 4k Page Buffers. */
#define			DMC_C_4K_GBUFFERS    51L /* 4k Group Buffers. */
#define			DMC_C_4K_GCOUNT	     52L /* 4k Group Buffer count. */
#define			DMC_C_4K_MLIMIT	     53L /* 4k Modify page limit */
#define			DMC_C_4K_FLIMIT	     54L /* 4k Free page limit */
#define			DMC_C_4K_WSTART	     55L /* 4k Write behind start */
#define			DMC_C_4K_WEND	     56L /* 4k Write behind end */
#define                 DMC_C_4K_NEWALLOC    57L /* Alloc 4k pool separately */
#define                 DMC_C_4K_SCANFACTOR  58L /* 4k Scan factor */
#define			DMC_C_4K_WRITEBEHIND 59L /* 4k WriteBehind. */
#define			DMC_C_8K_BUFFERS     60L /* 8k Page Buffers. */
#define			DMC_C_8K_GBUFFERS    61L /* 8k Group Buffers. */
#define			DMC_C_8K_GCOUNT	     62L /* 8k Group Buffer count. */
#define			DMC_C_8K_MLIMIT	     63L /* 8k Modify page limit */
#define			DMC_C_8K_FLIMIT	     64L /* 8k Free page limit */
#define			DMC_C_8K_WSTART	     65L /* 8k Write behind start */
#define			DMC_C_8K_WEND	     66L /* 8k Write behind end */
#define                 DMC_C_8K_NEWALLOC    67L /* Alloc 8k pool separately */
#define                 DMC_C_8K_SCANFACTOR  68L /* 8k Scan factor */
#define			DMC_C_8K_WRITEBEHIND 69L /* 8k WriteBehind. */
#define			DMC_C_16K_BUFFERS    70L /* 16k Page Buffers. */
#define			DMC_C_16K_GBUFFERS   71L /* 16k Group Buffers. */
#define			DMC_C_16K_GCOUNT     72L /* 16k Group Buffer count. */
#define			DMC_C_16K_MLIMIT     73L /* 16k Modify page limit */
#define			DMC_C_16K_FLIMIT     74L /* 16k Free page limit */
#define			DMC_C_16K_WSTART     75L /* 16k Write behind start */
#define			DMC_C_16K_WEND	     76L /* 16k Write behind end */
#define                 DMC_C_16K_NEWALLOC   77L /* Alloc 16k pool separately */
#define                 DMC_C_16K_SCANFACTOR 78L /* 16k Scan factor */
#define			DMC_C_16K_WRITEBEHIND 79L /* 16k WriteBehind. */
#define			DMC_C_32K_BUFFERS    80L /* 32k Page Buffers. */
#define			DMC_C_32K_GBUFFERS   81L /* 32k Group Buffers. */
#define			DMC_C_32K_GCOUNT     82L /* 32k Group Buffer count. */
#define			DMC_C_32K_MLIMIT     83L /* 32k Modify page limit */
#define			DMC_C_32K_FLIMIT     84L /* 32k Free page limit */
#define			DMC_C_32K_WSTART     85L /* 32k Write behind start */
#define			DMC_C_32K_WEND	     86L /* 32k Write behind end */
#define                 DMC_C_32K_NEWALLOC   87L /* Alloc 32k pool separately */
#define                 DMC_C_32K_SCANFACTOR 88L /* 32k Scan factor */
#define			DMC_C_32K_WRITEBEHIND 89L /* 32k WriteBehind. */
#define			DMC_C_64K_BUFFERS    90L /* 64k Page Buffers. */
#define			DMC_C_64K_GBUFFERS   91L /* 64k Group Buffers. */
#define			DMC_C_64K_GCOUNT     92L /* 64k Group Buffer count. */
#define			DMC_C_64K_MLIMIT     93L /* 64k Modify page limit */
#define			DMC_C_64K_FLIMIT     94L /* 64k Free page limit */
#define			DMC_C_64K_WSTART     95L /* 64k Write behind start */
#define			DMC_C_64K_WEND	     96L /* 64k Write behind end */
#define                 DMC_C_64K_NEWALLOC   97L /* Alloc 64k pool separately */
#define                 DMC_C_64K_SCANFACTOR 98L  /* 64k Scan factor */
#define			DMC_C_64K_WRITEBEHIND 99L /* 64k WriteBehind. */
#define                 DMC_C_MAXTUPLEN     100L /* Installation max tuplen */
#define			DMC_C_REP_QSIZE	    101L /* No of records in replicator
						 ** shared memory region */
#define 		DMC_C_DEF_PAGE_SIZE 102L /* default page size */
#define			DMC_C_REP_SALOCK    103L /* Replicator shad/arch 
						 ** lockmode */
#define			DMC_C_REP_IQLOCK    104L /* Replicator input queue
						 ** lockmode */
#define			DMC_C_REP_DQLOCK    105L /* replicator dist queue
						 ** lockmode */
#define			DMC_C_REP_DTMAXLOCK 106L /* rep dist threads maxlocks */
#define			DMC_C_PSORT_BSIZE   107L /* # rows in exchange buff
						 ** for parallel sort */
#define			DMC_C_PSORT_ROWS    108L /* # rows in sort before 
						 ** sort threads are used */
#define			DMC_C_PSORT_NTHREADS 109L/* # threads to use per
						 ** parallel sort */
#define			DMC_C_BLOB_ETAB_STRUCT 110L/* default blob etab
						 ** structure */
#define			DMC_C_PIND_BSIZE    111L /* # rows in exchange buff
						 ** for parallel index */
#define			DMC_C_PIND_NBUFFERS 112L /* # of exchange buffers for
						 ** parallel index */
#define                 DMC_C_ETAB_PAGE_SIZE 113L /* etab page size */
#define			DMC_C_PAGETYPE_V1    114L /* page type V1 */
#define			DMC_C_PAGETYPE_V2    115L /* page type V2 */
#define			DMC_C_PAGETYPE_V3    116L /* page type V3 */
#define			DMC_C_PAGETYPE_V4    117L /* page type V4 */
#define			DMC_C_ETAB_TYPE	     118L /* etab type */
#define			DMC_C_TCB_LIMIT	     119L /* TCB limit */
#define			DMC_C_MAXBTREEKEY    120L /* max btree key length */

#define			DMC_C_ON_LOGFULL     121L /* on_logfull ... */
#define			DMC_C_PAGETYPE_V5    122L /* page type V5 */
#define			DMC_C_DOP	     123L /* DegreeOfParallelism */
#define			DMC_C_PAD_BYTES      124L /* DMF pad bytes */
#define			DMC_C_SORT_CPUFACTOR 125L /* sort cpu cost adjustment 
						** factor */
#define			DMC_C_SORT_IOFACTOR  126L /* sort IO cost adjustment 
						** factor */
#define			DMC_C_CORE_ENABLED   127L /* enable core dumps on segvs */
#define                 DMC_C_BUILD_PAGES    128L /* build-buffer pages
                                                ** aka mwrite_blocks */
#define			DMC_C_PAGETYPE_V6    129L /* page type V6 */
#define			DMC_C_PAGETYPE_V7    130L /* page type V7 */
#define			DMC_C_CRYPT_MAXKEYS  131L /* max shmem encrypt keys */
#define			DMC_C_DEF_COLL	     132L /* Default collation */
#define			DMC_C_DEF_UNI_COLL   133L /* Default Unicode collation */
    i4         char_value;             /* Value of characteristic. */
#define                 DMC_C_ON            1L
#define                 DMC_C_OFF           0L
#define                 DMC_C_SESSION       -2L
#define                 DMC_C_SYSTEM        -3L
#define                 DMC_C_NOWAIT        -4L
#define                 DMC_C_TABLE         1L
#define                 DMC_C_PAGE          2L
#define                 DMC_C_ROW           3L	
#define                 DMC_C_MVCC	    4L	
#define			DMC_C_WRITE_XACT    10L
#define			DMC_C_READ_XACT     11L
#define			DMC_C_NO_XACT       12L
#define			DMC_C_UNKNOWN	    -1L	/* See below */
					    /*
					    ** DMC_C_{MAJOR,MINOR}_UDADT
					    ** value was not available.
					    ** This should never happen.
					    */

                        /* Isolation levels: */
#define                 DMC_C_READ_UNCOMMITTED  1L
#define                 DMC_C_READ_COMMITTED    2L
#define                 DMC_C_REPEATABLE_READ   3L
#define                 DMC_C_SERIALIZABLE      4L

			/* Access Modes: */
#define			DMC_C_READ_ONLY		1L
#define			DMC_C_READ_WRITE	2L

			/* Readlock values */
#define                 DMC_C_READ_NOLOCK       1L
#define                 DMC_C_READ_SHARE        2L
#define                 DMC_C_READ_EXCLUSIVE    3L

			/* Etab type */
#define			DMC_C_ETAB_DEFAULT	1L
#define			DMC_C_ETAB_NEW		2L
#define			DMC_C_ETAB_NEWHEAP	3L

			/* on_logfull option: */
#define			DMC_C_LOGFULL_ABORT	1L
#define			DMC_C_LOGFULL_COMMIT	2L
#define			DMC_C_LOGFULL_NOTIFY	3L

 

    f4		    char_fvalue;            /* Value of floating point
					    ** characteristic. */
    PTR		    char_pvalue;            /* Value of pointer
					    ** characteristic. */
}    DMC_CHAR_ENTRY;    

/*}
** Name: DMC_LOC_ENTRY - Describes a location list entry.
**
** Description:
**      This structure defines the information needed to define a location
**	that will be used to locate data by DMF.  The data that can be located
**	in a location includes: system tables, user tables, log file, journal
**	file.
**
** History:
**     25-nov-1985 (derek)
**          Created new for jupiter.
**     12-oct-1989 (sandyh)
**	    Changed loc_extent[64] to DB_AREA_MAX.
**	02-nov-92 (jrb)
**	    Renamed "LOC_SORT" to "LOC_WORK".
**	11-mar-1999 (nanpr01)
**	    Productize raw location.
**	11-May-2001 (jenjo02)
**	    Deleted unneeded loc_raw_blocks from DMC_LOC_ENTRY.
*/
typedef struct _DMC_LOC_ENTRY
{
    i4         	    loc_flags;          /* Type of location entry. */
#define                 LOC_DEFAULT     0x01L
#define			LOC_DATA	0x02L
#define			LOC_JNL		0x04L
#define                 LOC_CKP         0x08L
#define                 LOC_DUMP        0x10L
#define			LOC_WORK	0x20L
#define			LOC_LOG		0x10000L
    DB_LOC_NAME	    loc_name;		/* Name of the location. */
    i4	    	    loc_l_extent;	/* Length of the extent name. */
    char	    loc_extent[DB_AREA_MAX];	/* Physical extent name. */
}   DMC_LOC_ENTRY;


/*}
** Name: DMC_LLIST_ITEM - Describes a location list entry.
**
** Description:
**	This is an item in a location list in the dmc_cb.
**	
** History:
**	03-nov-92 (jrb)
**	    Created for multi-location sorts project.
**	14-dec-92 (jrb)
**	    Removed unneeded word "typedef"; it caused Unix compiler warnings.
*/
struct _DMC_LLIST_ITEM
{
    struct _DMC_LLIST_ITEM	*dmc_llink;	    /* Next item in list */
    DB_LOC_NAME			dmc_lname;	    /* Location Name */
};
