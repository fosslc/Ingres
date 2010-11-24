/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

/**
** Name: DML.H - Typedefs and constants for the logical layer.
**
** Description:
**      This file contains all the structs, typedefs, and symbolic constants
**      used by the logical layer of DMF.  The logical layer is responsible for
**      the translation of DMF logical request to DMF physical request.
**
** History:
**      28-nov-1985 (derek)
**          Created new for Jupiter.
**	30-sep-1988 (rogerk)
**	    Added XCB support for DELAYBT.
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.
**	    Added gateway connection id to session control block.
**	    Added scb_sess_mask to keep interesting session attributes.
**	 6-dec-1989 (rogerk)
**	    Added XCB support for online backup - set status that indicates
**	29-dec-1989 (rogerk)
**	    Took out XCB_BACKUP flag - used flag in DCB instead.
**	    that the database is being backed up.
**	17-jan-1990 (rogerk)
**	    Added the XCB_SVPT_DELAY flag.  This allows us to avoid savepoint
**	    writes on read-only transactions.
**	24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.
**      26-mar-91 (jennifer)
**          Added SCB_S_DOWNGRADE privilege to session, added new performance
**	    counters.
**	09-oct-91 (jrb) integrated following change: 19-nov-90 (rogerk)
**	    Added to the session control block a timezone field to record
**	    the timezone in which the user session is running.
**	    This was done as part of the DBMS Timezone changes.
**	09-oct-91 (jrb) integrated following change: 04-feb-91 (rogerk)
**	    Added SCB_NOLOGGING flag to scb_sess_mask to indicate that the
**	    session is using "set nologging" to bypass the logging system.
**	    Added XCB_NOLOGGING to xcb_flags.
**	23-apr-1990 (fred)
**	    Added odcb_cq_next & odcb_cq_prev fields to the open database
**	    control block.  This queue contains a list of operations which
**	    need to be one when the database is closed.  This queue is added to
**	    support the use of session level (really open database lived)
**	    temporaries -- which are forcibly destroyed at the close of the
**	    database.
**      22-apr-1992 (schang)
**          GW merge
**	    18-apr-1991 (rickh)
**		Noted the curious coincidence of RCB_USER_INTR and SCB_USER_INTR.
**	7-jul-92 (walt)
**	    Prototyping DMF.
**      24-sep-92 (stevet)
**          Changed scb_timezone of DML_SCB to scb_tzcb, which is a pointer 
**          the timezone structure TM_TZ_CB for the user session.
**	26-oct-92 (rogerk)
**	    Reduced Logging Project: Added support for checksums:
**	      - Added prototype for dmc_setup_checksum routine.
**	04-nov-92 (jrb)
**	    Added DML_LOC_MASKS typedef and added pointer for this in SCB.
**	14-dec-92 (jrb)
**	    Removed unneeded word "typedef"; it caused Unix compiler warnings.
**	18-jan-1993 (bryanp)
**	    Added prototypes for LG/LK special thread routines.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed savepoints to be tracked via LSN's rather than Log addrs.
**	    Removed the xcb_abt_sp_addr stuff which was used when BI's were
**		used for undo processing.
**	    Removed unused xcb_lg_addr field.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	31-mar-1993 (ralph)
**	    DELIM_IDENT:
**	    Added scb_dbxlate to DML_SCB to contain case translation flags.
**	    Added scb_cat_owner to DML_SCB to address catalog owner name.
**       5-Nov-1993 (fred)
**          Added scb_lo_{next,prev} to keep track of large object
**          temporaries.  This list is used to destroy these
**          temporaries at opportune times.
**	05-nov-1993 (smc)
**          Removed non-portable byte offset comments.
**      14-Apr-1994 (fred)
**          Added xcb_seq_no field to the DML_XCB.  See comments there...
**	19-jan-1995 (cohmi01)
**	    For RAW I/O, add xccb_r_extent, xccb_r_loc_name to XCCB. This info
**	    must be stored to identify portions of raw file to rename/delete.
**      04-apr-1995 (dilma04)
**          Add support for SET TRANSACTION ISOLATION LEVEL.
**	15-may-96 (stephenb)
**	    add replication related fields to DML_XCB
**      22-nov-96 (dilma04)
**          Row Locking Project:
**          Add SLCB_K_ROW value for slcb_k_type parameter. 
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          - alter DML_SCB and DML_SLCB to fit changes in the way
**          of storing and setting of locking characteristics;
**          - add prototypes for dmt_set_lock_values() and
**          dmt_check_table_lock().
**	27-Feb-1997 (jenjo02)
**	    Added support for SET TRANSACTION <access mode> and
**	                      SET SESSION <access mode>.
**      14-oct-97 (stial01)
**          Changes to distinguish readlock value/isolation level
**          Readlock may be set on a table basis, isolation level cannot.
**      18-nov-97 (stial01)
**          Changed prototype for dmt_set_lock_values.
**      02-feb-98 (stial01)
**          Back out 18-nov-97 change.
**	27-mar-98 (stephenb)
**	    add flags to DML_SPCB.
**	14-May-1998 (jenjo02)
**	    Removed function prototype for dmc_write_behind() - it's been
**	    renamed to dmc_write_behind_primary(), is a factotum thread
**	    execution function, and has been moved to scf.h
**	12-aug-1998 (somsa01)
**	    Added dml_get_tabid() definition.
**	03-Dec-1998 (jenjo02)
**	    Added xccb_r_pgsize to XCCB for multi-sized
**	    raw files.
**      16-mar-1999 (nanpr01)
**	    Removed xccb_r_pgsize, xccb_r_loc_name and xccb_r_extent.
**      17-may-99 (stial01)
**          Changed prototype for dmt_set_lock_values
**      18-jan-2000 (gupsh01)
**          Added flag XCB_NONJNLTAB to indicate the records which are a 
**          part of transactions involving non-journaled tables for a 
**          journaled database. 
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Nov-2000 (jenjo02)
**	    Deleted xcb_tq_next, xcb_tq_prev, xccb_tq_next,
**	    xccb_tq_prev.
**	08-Jan-2001 (jenjo02)
**	    Added GET_DML_SCB macro to retrieve session's DML_SCB*.
**	    Deleted DML_SCF structure. Session's DML_SCB is now 
**	    embedded in SCF's SCB, not separately allocated.
**	    Correctly cast scb_gw_session_id to PTR instead of CS_SID,
**	    added scb_sid, scb_dis_tranid, scb_adf_cb to DML_SCB.
**      22-mar-2001 (stial01)
**          Added scb_global_lo (b104317)
**      08-Oct-2002 (hanch04)
**          Replace the i4 for length with a SIZE_TYPE (size_t) to allow
**          64-bit memory allocation.
**	05-Mar-2002 (jenjo02)
**	    Added structures DML_SEQ, DML_CSEQ, and function
**	    prototypes for Sequence Generators.
**      22-oct-2002 (stial01)
**          Added prototype for dmx_xa_abort
**      02-jan-2004 (stial01)
**          Added SCB_NOBLOBLOGGING, XCB_NOBLOBLOGGING for SET NOBLOBLOGGING
**	6-Feb-2004 (schka24)
**	    Kill the DMU statement count.
**	09-Apr-2004 (jenjo02)
**	    Added funcdtion prototypes for dmr_aggregate(),
**	    which was missing, and dmr_unfix(), which is new.
**      12-Apr-2004 (stial01)
**          Define length as SIZE_TYPE.
**	10-May-2004 (schka24)
**	    xcb, scb changes to get blobs working with parallel query.
**	11-Jun-2004 (schka24)
**	    Add is-session flag to XCCB, mutex the odcb xccb list.
**	21-Jun-2004 (schka24)
**	    Add is-blob-temp flag to XCCB, remove scb_lo list stuff.  Thus
**	    there is exactly one list (the parent-SCB's XCCB list) that
**	    needs to be examined and/or pruned when freeing blob temps.
**      27-aug-2004 (stial01)
**          Removed SET NOBLOBLOGGING statement
**      05-Aug-2005 (hanje04)
**          Back out change 478041 as it should never have been X-integrated
**          into main in the first place.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Fix GET_DML_SCB to return NULL if sid is CS_ADDER_ID.
**	11-Nov-2005 (jenjo02)
**	    Replaced dmx_show() with the more robust 
**	    dmxCheckForInterrupt() to standardize external
**	    interrupt handling.
**      13-feb-2006 (horda03) Bug 112560/INGSRV2880
**          Add prototype for dmt_table_locked and dmt_release_table_locks
**      29-nov-2005 (horda03) Bug 48659/INGSRV 2716
**          Added XCB_NONJNL_BT. This identifies a TX which has a
**          non-journalled BT record logged in the TX log, so that
**          should the TX update a journalled table, a (2nd) journalled
**          BT record is required.
**	19-Sep-2006 (jonj)
**	    Add dm2tInvalidateTCB prototype.
**	04-Oct-2006 (jonj)
**	    Add dmt_control_lock prototype.
**      16-oct-2006 (stial01)
**          Added xcb_locator_temp to DML_XCB
**      09-feb-2007 (stial01)
**          Moved locator context from DML_XCB to DML_SCB
**          Added new flags for locator blob holding temps.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Pass DMC_CB * instead of i4 * err_code to   
**	    dmc_set_logging(). Replace i4 *error with DB_ERROR *error
**	    in function prototypes.
**	26-Nov-2008 (jonj)
**	    SIR 120874: dml_get_tabid() converted to DB_ERROR *
**	04-Dec-2008 (jonj)
**	    SIR 120874: dmf_adf_error() converted to DB_ERROR *
**	9-sep-2009 (stephenb)
**	    Move SEQ_VALUE declaration up so that it can be used in DML_SCB.
**	    Add scb_last_id field to DML_SCB to hold last identity number
**	    generated for the current session.
**	03-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Prototyped FEXI functions dmf_tbl_info(), dmf_last_id(),
**	    several missing dmc_, dmx_xa_ functions.
**/


/*
** Macro to get pointer to this session's DML_SCB
** directly from SCF's SCB:
*/
#define GET_DML_SCB(sid) \
        ((CS_SID)sid == CS_ADDER_ID ? (DML_SCB *)NULL : \
	*(DML_SCB**)((char *)sid + dmf_svcb->svcb_dmscb_offset))

/*
** forward references
*/
typedef struct _SEQ_VALUE {
	i8	intval;
	char	decval[DB_IISEQUENCE_DECLEN];
} SEQ_VALUE;

/*
**  ODCB - SECTION
**
**       This section defines all structures needed to build
**       and access the information in a Opoen Database Control Block (ODCB).
**       The following structs are defined:
**           
**           DML_ODCB                 Open Database Control Block.
*/

/*}
** Name:  DML_ODCB - DMF internal control block for open database information.
**
** Description:
**      This typedef defines the control block required to keep information
**      about an open database.  Most of the information in the ODCB comes
**      from information provided on the opendb call to DMF.  
**      This is a fixed length control block which is associated with a 
**      session control block.  The ODCB includes such information 
**      as a pointer to a database control block (DCB) which contains the
**      static information about the database, a pointer to the session control
**      block (SCB)  this ODCB is associated with, the database access mode
**      for this open, and  the database lock mode.
**       
**	In a parallel query environment, all the factotum threads share
**	the parent's ODCB, even if they are running with shared transactions
**	(and hence separate XCB's).
**	Therefore, odcb_scb_ptr points to the PARENT scb, and can be used
**	for accessing query-wide stuff.
**
**      A pictorial representation of ODCBs in an active server enviroment is:
**   
**       
**  +---->+--------+                                       DCB      
**  |     | SCB Q  |                          +-------->+--------+      
**  |     |        |                          |         | DCB Q  |   
**  |     | ODCB Q |--+                       |         |        |
**  |     |        |  |                       |         +--------+ 
**  |     +--------+  |           ODCB        |
**  |                 |         +---------+   |
**  |                 +-------->| ODCB Q  |   |
**  |                           |         |   |
**  |                           | DCB PTR |---+
**  |                           |         |   
**  +---------------------------|ODCB PTR |
**                              +---------+               
**

**     The ODCB layout is as follows:
**                                                        
**                     +----------------------------------+
**     Byte            | odcb_q_next                      |  Initial bytes are
**                     | odcb_q_prev                      |  standard DMF CB
**                     | odcb_length                      |  header. 
**                     | odcb_type  | odcb_reserved       |
**                     | odcb_reserved                    |
**                     | odcb_owner                       |
**                     | odcb_ascii_id                    |
**                     |------ End of Header -------------|
**                     | odcb_id                          |
**                     | odcb_scb_ptr                     |
**                     | odcb_dcb_ptr                     |
**                     | odcb_access_mode                 |
**                     | odcb_lk_mode                     |
**		       | odcb_cq_next			  |
**		       | odcb_cq_prev			  |
**                     +----------------------------------+
**                     
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter .
**      30-jan-1992 (bonobo)
**	    Removed the redundant typedef to quiesce the ANSI C 
**	    compiler warnings.
**	23-apr-1990 (fred)
**	    Added odcb_cq_next & odcb_cq_prev fields to the open database
**	    control block.  This queue contains a list of operations which
**	    need to be one when the database is closed.  This queue is added to
**	    support the use of session level (really open database lived)
**	    temporaries -- which are forcibly destroyed at the close of the
**	    database.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	02-oct-1994(liibi01)
**          Fix bug #60170, add two flags XCB_RELOCATE_NLOC and 
**          XCB_RELOCATE_OLOC.
**	06-dec-1996 (nanpr01)
**	    Take out the flag XCB_RELOCATE_NLOC and XCB_RELOCATE_OLOC.
**	16-feb-1999 (nanpr01)
**	    Support Update Mode locking.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Define ODCB_CB as DM_ODCB_CB.
*/

struct _DML_ODCB
{
    DML_ODCB        *odcb_q_next;           /* ODCBs queued to session control
                                            ** block. */
    DML_ODCB        *odcb_q_prev;
    SIZE_TYPE	    odcb_length;	    /* Length of control block. */
    i2              odcb_type;              /* Type of control block for
                                            ** memory manager. */
#define                 ODCB_CB             DM_ODCB_CB
    i2              odcb_s_reserved;        /* Reserved for future use. */
    PTR             odcb_l_reserved;        /* Reserved for future use. */
    PTR             odcb_owner;             /* Owner of control block for
                                            ** memory manager.  ODCB will be
                                            ** owned by the session . */
    i4         odcb_ascii_id;          /* Ascii identifier for aiding
                                            ** in debugging. */
#define                 ODCB_ASCII_ID       CV_C_CONST_MACRO('O', 'D', 'C', 'B')
    PTR             odcb_db_id;             /* Identifier for database . */
    DML_SCB         *odcb_scb_ptr;          /* Pointer to session control 
                                            ** block associated with this 
                                            ** ODCB. */
    DMP_DCB         *odcb_dcb_ptr;          /* Pointer to database control
                                            ** block associated with this 
                                            ** ODCB. */
    i4         odcb_access_mode;       /* Access mode of opener. */
#define                 ODCB_A_READ         1L
#define                 ODCB_A_WRITE        2L
    i4         odcb_lk_mode;           /* Lock mode of opener. */
#define                 ODCB_IX             LK_IX
#define                 ODCB_X              LK_X
    /* Parallel query can find multiple threads fooling with the ODCB
    ** xccb list, so it needs to be mutexed.  ODCB's aren't thread specific,
    ** unlike XCB's which are.  Technically, we only NEED to mutex additions
    ** to the list, which might happen in parallel.  List removal probably
    ** doesn't need the mutex, since it happens at transaction or session
    ** end and parallel threads are probably gone.  Careful users will take
    ** the mutex anyway.
    */
    DM_MUTEX	    odcb_cq_mutex;	    /* Mutex for cq list */
    DML_XCCB        *odcb_cq_next;          /*
					    ** Queue of commit operation control
                                            ** blocks associated with this
                                            ** database.
					    */
    DML_XCCB        *odcb_cq_prev;
};

/*
**  SCB - SECTION
**
**       This section defines all structures needed to build
**       and access the information in a Session Control Block (SCB).
**       The following structs are defined:
**
**	     DML_LOC_MASKS	     Location usage mask block.
**           DML_SCB                 Session Control Block.
*/


/*}
** Name: DML_LOC_MASKS - Location usability block
**
** Description:
**	This type defines three bit masks to indicate which locations of a
**	given type are usable by that session.  One of these blocks will exist
**	for each session.
**
** History:
**	04-nov-92 (jrb)
**	    Created for multi-location sorts.
**	14-dec-92 (jrb)
**	    Removed unneeded word "typedef"; it caused Unix compiler warnings.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with
**	    DM_SVCB.
**	13-May-2004 (schka24)
**	    LOCM type clashed with SEQ CB type.  Since I'm here, fix.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Define LOCM_CB as DM_LOCM_CB.
*/
struct _DML_LOC_MASKS
{
    i4	    *locm_next;		    /* Not used. */
    i4	    *locm_prev;		    /* Not used. */
    SIZE_TYPE	    locm_length;	    /* Length of control block. */
    i2              locm_type;		    /* Type of control block for
					    ** memory manager. */
#define                 LOCM_CB		DM_LOCM_CB
    i2              locm_s_reserved;	    /* Reserved for future use. */
    PTR             locm_l_reserved;	    /* Reserved for future use. */
    PTR             locm_owner;		    /* Owner of control block for
					    ** memory manager.  SVCB will be
					    ** owned by the server. */
    i4         locm_ascii_id;	    /* Ascii identifier for aiding
					    ** in debugging. */
#define                 LOCM_ASCII_ID       CV_C_CONST_MACRO('L', 'O', 'C', 'M')

/* 3 of the following fields point to bit maps into the extent array in the dcb.
** The maps should be accessed using the BT calls in the CL; bit 0 corresponds
** to extent array element 0, and so forth.
**
** The space for the bit maps will be allocated dynamically at the end of this
** structure as soon as the number of config file extents is known.
*/
    i4	locm_bits;	    /* number of bits in each following field */
    i4	*locm_w_allow;	    /* locs allowed to use for work */
    i4	*locm_w_use;	    /* currently wanted locs for work */
    i4	*locm_d_allow;	    /* allowed to use for data */
};

/*
** }
** Name:  DML_SCB - DMF internal control block for session information.
**
** Description:
**      This typedef defines the control block required to keep information
**      about sessions within this server.  A session or thread is a single
**      connection of a user to DMF.  Each session is completely seperate
**      from all others, i.e. it appears as if they are the only user 
**      of DMF.  
**
**      Most of the information in the SCB comes from runtime information
**      provided on the call to DMF to begin a session.  This is a fixed
**      size control block and includes such information as a pointer
**      to an open database control block (ODCB) for each database being
**      accessed by this session, a pointer to a transaction control block
**      (XCB) for each active transaction associated with this session,
**      a pointer to a setlock control block (SLCB) for each setlock 
**      associated with this session, the user associated with the session,
**      and some usage counts.
**
**	In a parallel thread environment, each factotum thread has its
**	own SCB, which is filled in from the parent thread.  If the child
**	is running a shared transaction, the transaction BEGIN will arrange
**	to share the parent's session lock/log context.  (The lock/log
**	ID's will be different, but the contexts will be shared.)
**
**	To summarize the situation with parallel query threads, when they
**	are all running shared transaction context (the usual situation):
**	Each thread will have its own SCB;
**	Each thread will have its own XCB and lock/log ID's;
**	Each thread will see its own logical tran ID (XCB);
**	All threads will share a physical DB_TRAN_ID;
**	All threads will share one ODCB;
**	The XCB scb ptr points to the thread SCB;
**	The ODCB scb ptr points to the parent SCB.
**
**      SCBs are queued to the server control block(SVCB) and point to 
**      open database control blocks(ODCBs) and transaction control blocks 
**      (XCBs) associated with this session.  A pictorial representation of 
**      SCBs in an active server enviroment is:
**   
**           SVCB
**       +----------+
**       |          |
**  +----| SCB Q    |
**  |    +----------+
**  |         SCB      
**  |    +-----------+ 
**  +--->|  SCB Q    |                ODCB                ODCB
**       |           |             +--------+          +--------+
**       |  ODCB  Q  |------------>| ODCB Q |--------->| ODCB Q |
**       |  SLCB  Q  |-----+       |        |          |        |
**   +---|  XCB  Q   |     |       +--------+          +--------+  
**   |   +-----------+     |                  
**   |      XCB            |                               SLCB    
**   |   +--------+        |                            +--------+
**   +-->| XCB Q  |--+     +--------------------------->| SLCB Q |
**       |        |  |                                  |        |
**       +--------+  |             XCB                  +--------+
**                   |         +---------+   
**                   +-------->| XCB Q   |   
**                             |         |   
**                             +---------+               
**

**     The SCB layout is as follows:
**                                                        
**                     +----------------------------------+
**     Byte            | scb_q_next                       |  Initial bytes are
**                     | scb_q_prev                       |  standard DMF CB
**                     | scb_length                       |  header. 
**                     | scb_type   | scb_reserved        |
**                     | scb_reserved                     |
**                     | scb_owner                        |
**                     | scb_ascii_id                     |
**                     |------ End of Header -------------|
**                     | scb_user                         |
**		       | scb_s_type                       |
**                     | scb_oq_next                      |
**                     | scb_oq_prev                      |
**                     | scb_x_next                       |
**                     | scb_x_prev                       |
**                     | scb_kq_next                      |
**                     | scb_kq_prev                      |
**                     | scb_lock_list                    |
**                     | scb_x_ref_count                  |
**                     | scb_o_ref_count                  |
**                     | scb_x_max_tran                   |
**                     | scb_o_max_opendb                 |
**		       | scb_trace                        |
**		       | scb_ui_state                     |
**		       | scb_state                        |
**		       | scb_sess_mask			  |
**		       | scb_gw_session_id                |
**		       | scb_real_user                    |
**		       | scb_audit_state                  |
**		       | scb_tzcb                         |
**                     | scb_loc_masks                    |
**                     | scb_dbxlate                      |
**                     | scb_cat_owner                    |
**                     | scb_lo_next                      |
**                     | scb_lo_prev                      |
**                     +----------------------------------+
**                     
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter .
**	15-aug-89 (rogerk)
**	    Added scb_gw_session_id for Non-SQL Gateway.  This is the gateway
**	    connection id returned by the gws_init call.
**	    Added scb_sess_mask to keep interesting session attributes.
**      26-mar-91 (jennifer)
**          Added SCB_S_DOWNGRADE privilege to session.
**	09-oct-91 (jrb) integrated following change: 19-nov-90 (rogerk)
**	    Added to the session control block a timezone field to record
**	    the timezone in which the user session is running.
**	    This was done as part of the DBMS Timezone changes.
**	09-oct-91 (jrb) integrated following change: 04-feb-91 (rogerk)
**	    Added SCB_NOLOGGING flag to scb_sess_mask to indicate that the
**	    session is using "set nologging" to bypass the logging system.
**	26-nov-89 (jrb)
**	    Added pointer to location usage mask block; for multi-location
**	    sorting project.
**	31-mar-1993 (ralph)
**	    DELIM_IDENT:
**	    Added scb_dbxlate to DML_SCB to contain case translation flags.
**	    Added scb_cat_owner to DML_SCB to address catalog owner name.
**	10-sep-93 (swm)
**	    Changed scb_gw_session_id type from i4 to CS_SID to match
**	    recent CL interface revision.
**       5-Nov-1993 (fred)
**          Added scb_lo_{next,prev} to keep track of large object
**          temporaries.  This list is used to destroy these
**          temporaries at opportune times.
**	05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with
**	    DM0M_OBJECT.
**      04-apr-1995 (dilma04)
**          Add support for SET TRANSACTION ISOLATION LEVEL.
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          - add fields to store session level locking characteristics;
**          - change scb_isolation_level to scb_tran_iso.
**	27-Feb-1997 (jenjo02)
**	    Added scb_tran_am, scb_sess_am in support of 
**	    SET SESSION | SET TRANSACTION <access mode>.
**	26-jun-98 (stephenb)
**	    Add SCB_CTHREAD_ERROR to scb_ui_state.
**	14-oct-99 (stephenb)
**	    Add SCB_BLOB_OPTIM define to session masks.
**	08-Jan-2001 (jenjo02)
**	    Correctly cast scb_gw_session_id to PTR instead of CS_SID.
**	    Added scb_sid, scb_dis_tranid, scb_adf_cb.
**	12-Jan-2004 (jenjo02)
**	    Added scb_dop, SCB_DOP_DEFAULT.
**	20-Apr-2004 (jenjo02)
**	    Added SCB_RCB_FABORT ui_state, SCB_LOGFULL_COMMIT,
**	    SCB_LOGFULL_NOTIFY,
**	    dmx_logfull_commit() function prototype for
**	    logfull commit.
**	4-May-2004 (schak24)
**	    Make CTHREAD notification a flag, not a code number, and
**	    rename it.
**	7-May-2004 (schka24)
**	    Remove blob-optim flag, inappropriately placed now with parallel
**	    query.  Note that blob temp cleanup stuff is parent only,
**	    and mutex protect it.
**	21-Jun-2004 (schka24)
**	    Remove blob-temp cleanup list, we now use the XCCB list.
**	22-Jul-2004 (schak24)
**	    Add factotum flag to session type, might be useful.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Added scb_mem_alloc, scb_mem_hwm,
**	    scb_pool_list, scb_free_list.
**	    Define SCB_CB as DM_SCB_CB.
**	12-Apr-2008 (kschendel)
**	    Added qualification-function info for DMF exception handler
**	    to see, so that it knows that it was executing a user CX.
**	    Added ADF error message area for session.
**	13-Apr-2010 (kschendel) SIR 123485
**	    Add BQCB list and mutex for new blobbery code.
*/

struct _DML_SCB
{
    DML_SCB         *scb_q_next;        /* Queue of SCBs off SVCB. */
    DML_SCB         *scb_q_prev;
    SIZE_TYPE	    scb_length;		/* Length of control block. */
    i2              scb_type;           /* Type of control block for
                                        ** memory manager. */
#define                 SCB_CB              DM_SCB_CB
    i2              scb_s_reserved;     /* Reserved for future use. */
    PTR             scb_l_reserved;     /* Reserved for future use. */
    PTR             scb_owner;          /* Owner of control block for
                                        ** memory manager.  SCB will be
                                        ** owned by the server. */
    i4         	    scb_ascii_id;       /* Ascii identifier for aiding
                                        ** in debugging. */
#define                 SCB_ASCII_ID        CV_C_CONST_MACRO('#', 'S', 'C', 'B')
    DB_OWN_NAME     scb_user;           /* User of session . */
    i4         	    scb_s_type;         /* Type of session. */
#define                 SCB_S_NORMAL        0x00L
#define                 SCB_S_SYSUPDATE     0x01L
#define                 SCB_S_SECURITY      0x02L
#define                 SCB_S_DOWNGRADE     0x04L
#define			SCB_S_FACTOTUM	    0x08L

    DML_ODCB        *scb_oq_next;       /* Queue of open database control
                                        ** blocks off SCB. */
    DML_ODCB        *scb_oq_prev;
    DML_XCB         *scb_x_next;        /* Queue of transaction control
                                        ** blocks off SCB. */
    DML_XCB         *scb_x_prev;
    DML_SLCB        *scb_kq_next;       /* Queue of set lock control 
                                        ** blocks off SCB. */
    DML_SLCB        *scb_kq_prev;
    i4		    scb_lock_list;	/* Session locks. */
    i4		    scb_x_ref_count;	/* Transaction reference count. */
    i4		    scb_o_ref_count;	/* Opendb reference count. */
    i4		    scb_x_max_tran;	/* Maximum transactions per 
                                        ** session. */
    i4		    scb_o_max_opendb;   /* Maximum opendbs per 
                                        ** session. */
    u_i4	scb_trace[100 / BITS_IN(i4)];
					/* Session level trace flags. */
					/* Must be defined as i4, see
					** dmftrace, dmcend */
    i4		scb_ui_state;		/* User interrupt state.
                                        ** 0 = no interrupt, 1 means
                                        ** user interrupts received,
					** 2 means force abort event
					** received.
					*/
#define                 SCB_USER_INTR	    0x01L /* must = RCB_USER_INTR */
#define                 SCB_FORCE_ABORT	    0x02L
#define			SCB_CTHREAD_INTR    0x04L /* CUT cooperating thread
						  ** signalled an attn */
#define                 SCB_RCB_FABORT	    0x08L /* must = RCB_FORCE_ABORT */
    i4         	    scb_state;		    /* State of the session. */
#define                 SCB_WILLING_COMMIT  0x01L
    i4	    	    scb_sess_mask;	    /* Session status mask. */
#define			SCB_GATEWAY_CONNECT 0x0010L /* Connected to Gateway */
#define			SCB_NOLOGGING	    0x0020L /* NoLogging Session */
/* unused		unused		    0x0040L  unused */
/* unused		unused		    0x0080L  unused */
#define                 SCB_LOGFULL_COMMIT  0x0100L /* Commit on LOGFULL */
#define                 SCB_LOGFULL_NOTIFY  0x0200L /* Commit and notify
						    ** on LOGFULL */
    PTR	    	    scb_gw_session_id;	    /* Gateway connection id */
    DB_OWN_NAME     scb_real_user;          /* User of session . */
    i4              scb_audit_state;        /* State of user auditing. */
#define             SCB_A_TRUE              1
#define             SCB_A_FALSE             0
    PTR	            scb_tzcb;	            /* Timezone of user session */
    DML_LOC_MASKS   *scb_loc_masks;	    /* Location usage bit masks blk */
    u_i4	    *scb_dbxlate;	    /* Case translation semantics flags;
					    ** See cui.h for values
					    */
    DB_OWN_NAME	    *scb_cat_owner;	    /*
					    ** Catalog owner name.  Will be
					    ** "$ingres" if regular ids lower;
					    ** "$INGRES" if regular ids upper.
					    */
    /* The next thing for blobs is relevant in the parent SCB ONLY.  */
    DM_MUTEX	    scb_bqcb_mutex;	    /* Protect concurrent BQCB list */
    struct _DMPE_BQCB *scb_bqcb_next;	    /* Head of BQCB list (singly linked) */
    bool	    scb_global_lo;          /* TRUE if blob etab created
					    ** for session temp parent.  This
					    ** is a hint, if false we can
					    ** skip some work looking for etabs
					    ** at session-temp drop time.
					    */
    /* End of parent only blob things */
    struct _DMPE_LLOC_CXT  *scb_lloc_cxt;   /* locator context */

    i4         	    scb_timeout;            /* Timeout value for locks. */
    i4         	    scb_maxlocks;           /* Number of locks to obtain 
                                            ** before escalation. 
                                            */
    i4         	    scb_lock_level;         /* Lock granularity level. May 
                                            ** be DMC_C_TABLE, DMC_C_PAGE,  
                                            ** DMC_C_ROW, or DMC_C_SYSTEM. 
                                            */
    i4         	    scb_readlock;           /* Readlock value. May be one of:
					    **     DMC_C_NOLOCK
					    **     DMC_C_SHARE
					    **     DMC_C_EXCLUSIVE
					    **     DMC_C_SYSTEM
					    */
    i4         	    scb_sess_iso;           /* Session isolation level set by:
                                            ** <SET LOCKMODE SESSION WITH 
                                            ** ISOLATION = ...>  May be one of:
                                            **     DMC_C_READ_UNCOMMITTED,
                                            **     DMC_C_READ_COMMITTED,
                                            **     DMC_C_REPEATABLE_READ,
                                            **     DMC_C_SERIALIZABLE,
                                            **     DMC_C_SYSTEM. 
                                            */
    i4         	    scb_tran_iso;           /* Next transaction isolation
                                            ** level, settable through the
                                            ** SET TRANSACTION ISOLATION LEVEL
                                            ** statement. Takes the same  
                                            ** values as scb_sess_iso
                                            */
    i4         	    scb_sess_am;            /* Session access mode,
					    ** settable through the 
					    ** SET SESSION statement
					    ** to either DMC_C_READ_ONLY or
					    **           DMC_C_READ_WRITE
					    */
    i4         	    scb_tran_am;            /* Transaction access mode,
					    ** settable through the 
					    ** SET TRANSACTION statement
					    ** to either DMC_C_READ_ONLY or
					    **           DMC_C_READ_WRITE
					    */
    CS_SID	    scb_sid;		    /* This session's SID */
    DB_DIS_TRAN_ID  *scb_dis_tranid;	    /* Session's distributed 
					    ** transaction id
					    */
    ADF_CB	    *scb_adf_cb;	    /* Pointer to session's 
					    ** ADF_CB
					    */
    /* qfun-adfcb is normally NULL;  it's set while running a row-qualification
    ** function so that the DMF exception handler knows that arithmetic
    ** exceptions are plausible and can be caught.
    ** qfun-errptr is set by the logical layer to the request CB error code,
    ** so that a non-continuing (error) arithmetic exception can indicate
    ** to the calling facility that the ADF message has been issued.
    ** (remember that signal return skips all of the normal logic and
    ** returns directly out of dmf-call.)
    */
    ADF_CB	    *scb_qfun_adfcb;	    /* RCB's ADF CB during row qual */
    DB_ERROR	    *scb_qfun_errptr;	    /* &cb->error for storing
					    ** exception handler info, so
					    ** that dmfcall exit knows.
					    ** ADF "user error" goes into
					    ** error detail for QEF.
					    */

    i4		    scb_dop;		    /* Degree of parallelism */
#define		SCB_DOP_DEFAULT		8
    SIZE_TYPE	    scb_mem_alloc;	    /* Current memory alloc */
    SIZE_TYPE	    scb_mem_hwm;	    /* Session mem hwm */
    i4		    scb_mem_pools;	    /* How many pools alloc'd */
    DM_OBJECT	    scb_pool_list;	    /* Session's DM0M memory */
    DM_OBJECT	    scb_free_list;	    /* List of free blocks   */
    SEQ_VALUE	    scb_last_id;	    /* Last value for identity column */

    /* Using a single ADF error message area for the user session is
    ** unsafe in the face of parallel query.  However, allocating the ADF
    ** message area for each RCB's ADF-CB is wasteful, as a thread may
    ** have many RCB's floating around, but it can only issue one message
    ** at a time.  So, it makes some sense to stick the ADF error message
    ** area into the thread/session SCB, here.  When we're about to
    ** execute a user CX in DMF context, the ADF-CB's message area is
    ** pointed to here.
    ** (This could be allocated out of short-term memory, I suppose,
    ** but this way seems simpler.)
    */
    char	    scb_adf_ermsg[DB_ERR_SIZE];  /* Space for ADF msgs */
};


/*
**  SLCB - SECTION
**
**       This section defines the structure of the DMF set lock structure
**       which is used to keep track of default table locking parameters 
**       to be used during a session.   This section defines the following
**       control blocks:
**           
**           DML_SLCB                 Set Lock Control Block.
*/

/*}
** Name:  DML_SLCB - DMF internal control block for set lock information.
**
** Description:
**      This typedef defines the control block required to keep information
**      about table lock parameters to be used during this session. 
**      A user can issue set lock requests for any number of tables during
**      a session.  Then when a table is openned the previously specified 
**      set lock information will be used to determine such things a lock 
**      lock timeout, lock escalation, etc.  Most of the information in the 
**      SLCB comes from information provided on a call to DMF which alters
**      the session enironment.  This is a fixed length control block which 
**      is associated with a session control block.  The SLCB includes such 
**      information as a pointer to the session control block (SCB)  this 
**      SLCB is associated with, the name of the table associated with this
**      lock information,  and the lock information.
**       
**      A pictorial representation of SLCBs in an active server enviroment is:
**   
**           SCB         
**  +---->+--------+    
**  |     | SCB Q  |    
**  |     |        |    
**  |     | SLCB Q |--+ 
**  |     |        |  | 
**  |     +--------+  |           SLCB    
**  |                 |         +---------+  
**  |                 +-------->| SLCB Q  |  
**  |                           |         |  
**  |                           |         |   
**  +---------------------------|SCB  PTR |
**                              +---------+               
**

**     The SLCB layout is as follows:
**                                                        
**                     +----------------------------------+
**     Byte            | slcb_q_next                      |  Initial bytes are
**                     | slcb_q_prev                      |  standard DMF CB
**                     | slcb_length                      |  header. 
**                     | slcb_type  | slcb_reserved       |
**                     | slcb_reserved                    |
**                     | slcb_owner                       |
**                     | slcb_ascii_id                    |
**                     |------ End of Header -------------|
**                     | slcb_scb_ptr                     |
**                     | slcb_flag                        |
**                     | slcb_db_tab_base                 |
**                     | slcb_timeout                     |
**                     | slcb_limit                       |
**                     | slcb_mode                        |
**                     | slcb_k_type                      |
**                     | slcb_k_duration                  |
**                     +----------------------------------+
**                     
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter .
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**      22-nov-96 (dilma04)
**          Row Locking Project:
**          Add SLCB_K_ROW value for slcb_k_type parameter.
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          - remove slcb_flag, slcb_mode and slcb_k_duration;
**          - replace slcb_limit with slcb_maxlocks;
**          - replace slcb_k_type with slcb_lock_level;
**          - add slcb_iso_level.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Define SLCB_CB as DM_SLCB_CB.
**	17-Aug-2005 (jenjo02)
**	    Remove "db_tab_index" from consideration.
**	    Overriding lock semantics cached here apply to 
**	    all of the base table's indexes and partitions
**	    not just the base table itself.
*/

struct _DML_SLCB
{
    DML_SLCB        *slcb_q_next;           /* Not used, no queue of SLCBs. */
    DML_SLCB        *slcb_q_prev;
    SIZE_TYPE	    slcb_length;	    /* Length of control block. */
    i2              slcb_type;              /* Type of control block for
                                            ** memory manager. */
#define                 SLCB_CB             DM_SLCB_CB
    i2              slcb_s_reserved;        /* Reserved for future use. */
    PTR             slcb_l_reserved;        /* Reserved for future use. */
    PTR             slcb_owner;             /* Owner of control block for
                                            ** memory manager.  SLCB will be
                                            ** owned by the server. */
    i4         	    slcb_ascii_id;          /* Ascii identifier for aiding
                                            ** in debugging. */
#define                 SLCB_ASCII_ID       CV_C_CONST_MACRO('S', 'L', 'C', 'B')
    DML_SCB         *slcb_scb_ptr;          /* Pointer to session control 
                                            ** block associated with this 
                                            ** SLCB. */
    i4	       	    slcb_db_tab_base;       /* Base table_id sans db_tab_index */
    i4         	    slcb_timeout;           /* Timeout value for locks. */
    i4         	    slcb_maxlocks;          /* Number of locks to obtain before
                                            ** escalation. */
    i4         	    slcb_lock_level;        /* Lock granularity level. May 
                                            ** be DMC_C_TABLE, DMC_C_PAGE,  
                                            ** DMC_C_ROW, DMC_C_SESSION,
                                            ** or DMC_C_SYSTEM.
                                            */
    i4         	    slcb_readlock;          /* Readlock value. May be 
                                            ** statement. May be one of:
					    ** DMC_C_READ_NOLOCK
					    ** DMC_C_READ_SHARE
					    ** DMC_C_READ_EXCLUSIVE
					    ** DMC_C_SESSION
					    ** DMC_C_SYSTEM
                                            */ 
};


/*
**  SPCB - SECTION
**
**       This section defines the structure of the DMF savepoint 
**       control block (SPCB).  The following structs are defined:
**           
**           DML_SPCB                 Savepoint Control Block.
*/

/*}
** Name:  DML_SPCB - DMF internal control block for savepoint information.
**
** Description:
**      This typedef defines the control block required to keep information
**      about a savepoint within a transaction. 
**
**      Most of the information in the SPCB comes from information provided
**      on the DMF savepoint command.  This is a fixed length control block 
**      queued to  a transaction control block (XCB).  The SPCB includes such 
**      information as a pointer to a transaction control block (XCB),
**      the log address of the savepoint log record associated with this
**      savepoint, a savepoint identifer, and savepoint name.
**       
**      A pictorial representation of SPCBs in an active transaction 
**      enviroment is:
**   
**           XCB         
**  +---->+--------+    
**  |     | XCB  Q |    
**  |     |        |    
**  |     | SPCB Q |--+ 
**  |     |        |  | 
**  |     +--------+  |           SPCB    
**  |                 |         +---------+  
**  |                 +-------->| SPCB Q  |  
**  |                           |         |  
**  |                           |         |   
**  +---------------------------|XCB  PTR |
**                              +---------+               
**
**

**     The SPCB layout is as follows:
**                                                        
**                     +----------------------------------+
**     Byte            | spcb_q_next                      |  Initial bytes are
**                     | spcb_q_prev                      |  standard DMF CB
**                     | spcb_length                      |  header. 
**                     | spcb_type  | spcb_reserved       |
**                     | spcb_reserved                    |
**                     | spcb_owner                       |
**                     | spcb_ascii_id                    |
**                     |------ End of Header -------------|
**                     | spcb_name                        |
**                     | spcb_id                          |
**                     | spcb_xcb_ptr                     |
**                     | spcb_lsn                         |
**                     +----------------------------------+
**                     
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter .
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed savepoints to be tracked via LSN's rather than Log addrs.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	27-mar-98 (stephenb)
**	    Add spcb_flags.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Define SPCB_CB as DM_SPCB_CB.
*/

struct _DML_SPCB
{
    DML_SPCB        *spcb_q_next;           /* Not used, no queue of SPCBs. */
    DML_SPCB        *spcb_q_prev;
    SIZE_TYPE	    spcb_length;	    /* Length of control block. */
    i2              spcb_type;              /* Type of control block for
                                            ** memory manager. */
#define                 SPCB_CB             DM_SPCB_CB
    i2              spcb_s_reserved;        /* Reserved for future use. */
    PTR             spcb_l_reserved;        /* Reserved for future use. */
    PTR             spcb_owner;             /* Owner of control block for
                                            ** memory manager.  SPCB will be
                                            ** owned by a transaction. */
    i4         spcb_ascii_id;          /* Ascii identifier for aiding
                                            ** in debugging. */
#define                 SPCB_ASCII_ID       CV_C_CONST_MACRO('S', 'P', 'C', 'B')
    DB_SP_NAME      spcb_name;              /* Name of savepoint . */
    i4         spcb_id;                /* Identifier for savepoint . */
    DML_XCB         *spcb_xcb_ptr;          /* Pointer to transaction control
                                            ** block assoicated with this 
                                            ** SPCB. */
    LG_LSN	    spcb_lsn;		    /* LSN of Savepoint log record. */
    i4		    spcb_flags;		    /* useful flags */
#define		SPCB_REP_IQ_OPEN	0x1 /* replicator input queue is open */
};


/*
**  XCB - SECTION
**
**       This section defines all structures needed to build
**       and access the information in a Transaction Control Block (XCB).
**       The following structs are defined:
**           
**           DML_XCB                 Transaction Control Block.
*/

/*
** }
** Name:  DML_XCB - DMF internal control block for transaction information.
**
** Description:
**      This typedef defines the control block required to keep information
**      about a singel transaction of a session .
**
**      Most of the information in the XCB comes from runtime information
**      provided on the call to DMF to begin a transaction and from other
**      control blocks.  This control block is a fixed size and includes 
**      such information as a unique transaction identifier, a pointer to
**      the session control block this XCB is associated with, a queue of 
**      savepoint control blocks (SPCBs) associated with this transaction,
**      a queue of record access control blocks (RCBs) associated with
**      this transaction,  the database identifier which indicates which
**      database this transaction can update (a transaction can read
**      many databases but only update one), a log identifier to indicate
**      where the transaction logging should occur, the XCB state, and
**      the transaction type.  
**
**	In a parallel query environment, factotum threads that are going
**	to share lock/log resources operate with a shared transaction.
**	This means that each thread has its own XCB, each with its own
**	lock/log ID's, but the underlying lock/log context is shared.
**	Note that the XCB SCB pointer points to the thread SCB, not the
**	parent SCB.  (The ODCB SCB pointer points to the parent SCB.)
**
**      XCBs are queued to the server control block(SVCB) and point to 
**      record access control blocks(RCBs) and savepoint control blocks (SPCBs)
**      associated with this transaction.  A pictorial representation of XCBs 
**      in an active server enviroment is:
**   
**
**           SCB
**   +-->+-----------+
**   |   |           |
**   |   |           |
**   |   +-----------+
**   |
**   |       XCB      
**   |   +-----------+ 
**   |   |           |                RCB                 RCB
**   |   |           |             +--------+          +--------+
**   |   |           |             |        |          |        |
**   |   |  RCB   Q  |------------>| RCB  Q |--------->| RCB  Q |
**   |   |  SPCB  Q  |-----+       |        |          |        |
**   +---|  SCB PTR  |     |       +--------+          +--------+  
**       +-----------+     |                  
**                         |                               SPCB    
**                         |                            +--------+
**                         +--------------------------->| SPCB Q |
**                                                      |        |
**                                                      +--------+
**

**     The XCB layout is as follows:
**                                                        
**                     +----------------------------------+
**     Byte            | xcb_q_next                       |  Initial bytes are
**                     | xcb_q_prev                       |  standard DMF CB
**                     | xcb_length                       |  header. 
**                     | xcb_type   | xcb_reserved        |
**                     | xcb_reserved                     |
**                     | xcb_owner                        |
**                     | xcb_ascii_id                     |
**                     |------ End of Header -------------|
**                     | xcb_tran_id                      |
**                     | xcb_scb_ptr                      |
**                     | xcb_rq_next                      |
**                     | xcb_rq_prev                      |
**                     | xcb_sq_next                      |
**                     | xcb_sq_prev                      |
**                     | xcb_x_type                       |
**                     | xcb_state                        |
**                     | xcb_flags                        |
**                     | xcb_odcb_ptr                     |
**                     | xcb_log_id                       |
**                     | xcb_lk_id                        |
**                     | xcb_sp_id                        |
**		       | xcb_cq_next                      |
**                     | xcb_cq_prev                      |
**		       | xcb_username			  |
**                     +----------------------------------+
**                     
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter .
**	19-sep-88 (rogerk)
**	    Added xcb_flags field to store status of the transaction.
**	    Added xcb_username to save xact username for dm0l_bt call.
**	 6-dec-1989 (rogerk)
**	    Added XCB_BACKUP flag which indicates that the database is
**	    being backed up.
**	29-dec-1989 (rogerk)
**	    Took out XCB_BACKUP flag - used flag in DCB instead.
**	17-jan-1990 (rogerk)
**	    Added the XCB_SVPT_DELAY flag. This indicates that a savepoint
**	    record must be written before any other log writes can be
**	    done for this transaction.  This allows us to avoid savepoint
**	    writes on read-only transactions.
**	29-apr-1991 (Derek)
**	    Added performance counters for transaction profiling.
**	09-oct-91 (jrb) integrated the following change: 04-feb-1991 (rogerk)
**	    Defined XCB_NOLOGGING flag for Set Nologging.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV: Removed xcb_abt_sp_addr, xcb_lg_addr.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**      14-Apr-1994 (fred)
**          Added xcb_seq_no field.  This is used to store the last
**          sequence number seen.  This is used by large object
**          processing to relate temporary tables to statements, so
**          that we know when to delete them.
**	16-may-96 (stephenb)
**	    add xcb_rep_seq and xcb_rep_input_queue for replication.
**	2-aug-96 (stephenb)
**	    add xcb_rep_remote_tx for replication
**	3-jun-97 (nanpr01)
**	    Fix bug 78352 : When transactions are rolled back, temporary
**	    tables that were updated does not get dropped as documented.
**	08-Nov-2000 (jenjo02)
**	    Deleted xcb_tq_next, xcb_tq_prev.
**	05-Mar-2002 (jenjo02)
**	    Added xcb_seq, xcb_cseq for sequence generators.
**	6-Feb-2004 (schka24)
**	    Kill the DMU statement count and its limit.
**	7-May-2004 (schka24)
**	    Move blob PCB cleanup list from rcb to xcb, since each thread
**	    has to clean up its own POPs.  (The SCB would work too, but the
**	    XCB is more directly applicable.)
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Define XCB_CB as DM_XCB_CB.
**	05-Sep-2006 (jonj)
**	    Add XCB_ILOG (internal logging) bit.
**	11-Sep-2006 (jonj)
**	    Deleted XCB_SVPT_DELAY.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add xcb_crib_ptr, xcb_s_rc_retry, 
**	    xcb_retry_bos_lsn, xcb_lctx_ptr, xcb_jctx_ptr.
**	23-Mar-2010 (kschendel) SIR 123448
**	    Add XCCB list mutex so that child threads can add file-delete
**	    entries to the session thread's XCCB list.
*/

struct _DML_XCB
{
    DML_XCB         *xcb_q_next;            /* Queue of XCBs off SCB. */
    DML_XCB         *xcb_q_prev;
    SIZE_TYPE	    xcb_length;		    /* Length of control block. */
    i2              xcb_type;               /* Type of control block for
                                            ** memory manager. */
#define                 XCB_CB              DM_XCB_CB
    i2              xcb_s_reserved;         /* Reserved for future use. */
    PTR             xcb_l_reserved;         /* Reserved for future use. */
    PTR             xcb_owner;              /* Owner of control block for
                                            ** memory manager.  XCB will be
                                            ** owned by a session. */
    i4         xcb_ascii_id;           /* Ascii identifier for aiding
                                            ** in debugging. */
#define                 XCB_ASCII_ID        CV_C_CONST_MACRO('#', 'X', 'C', 'B')
    DB_TRAN_ID      xcb_tran_id;           /* Identifier of a transaction. */
    LG_CRIB	    *xcb_crib_ptr;	    /* Pointer to CRIB if transaction
    					    ** started in a MVCC-capable
					    ** database, NULL otherwise.
					    ** Memory is tacked on to DML_XCB.
					    */
    DM_OBJECT	    *xcb_lctx_ptr;	    /* Allocated DMP_JCTX, if any */
    DM_OBJECT	    *xcb_jctx_ptr;	    /* Allocated DM0J_CTX, if any */
    LG_LSN	    xcb_retry_bos_lsn;	    /* CRIB bos_lsn when retrying  
    					    ** a read-committed statement,
					    ** used to detect futility */
    DML_SCB         *xcb_scb_ptr;           /* Pointer to the session control
                                            ** block associated with this 
                                            ** transaction. */
    DMP_RCB         *xcb_rq_next;           /* Queue of record access control
                                            ** blocks associated with this
                                            ** transaction. */
    DMP_RCB         *xcb_rq_prev;
    DML_SPCB        *xcb_sq_next;           /* Queue of savepoint control
                                            ** blocks associated with this
                                            ** transaction. */
    DML_SPCB        *xcb_sq_prev;
    i4         	    xcb_x_type;             /* Type of transaction. */
#define                 XCB_RONLY           0x01L
#define                 XCB_DISTRIBUTED     0x02L
#define                 XCB_UPDATE          0x04L
/* String values of above bits */
#define XCB_X_TYPE "\
READONLY,DISTRIBUTED,UPDATE"

    i4              xcb_state;              /* State of this XCB. */
#define                 XCB_FREE            0x00L 
                                            /* Not currently in transaction. */
#define                 XCB_INUSE           0x01L
                                            /* Currently in a transaction. */
#define                 XCB_STMTABORT       0x02L
                                            /* Transaction must be aborted
				            ** an error occurred that only
                                            ** allows abort to SP or abort
					    ** transaction to occur. */
#define                 XCB_USER_INTR       0x04L
					    /* A user interrupt has occurred, 
                                            ** only a abort to SP, abort 
                                            ** transaction, commit, or begin
                                            ** transaction are allowed. */
#define                 XCB_TRANABORT       0x08L
                                            /* Transaction must be aborted
				            ** an error occurred that only
                                            ** allows abort transaction to
					    ** occur. */
#define                 XCB_FORCE_ABORT	    0x10L
                                            /* Transaction must be force
					    ** aborted in order to reclaim
					    ** log space. Only commit or 
					    ** abort the whole transaction 
					    ** is allowed. */
#define			XCB_ABORT	    (XCB_STMTABORT | XCB_TRANABORT | XCB_FORCE_ABORT)
#define                 XCB_WILLING_COMMIT  0x20L
					    /* The transaction has entered into
					    ** the WILLING COMMIT state.
					    */
#define                 XCB_DISCONNECT_WILLING_COMMIT  0x40L
					    /* The WILLING COMMIT transaction
					    ** has been disconnected.
					    */
/* String values of above bits */
#define XCB_STATE "\
INUSE,STMTABORT,USER_INTR,TRANABORT,FORCE_ABORT,WILLING_COMMIT,\
DISC_WILLING_COMMIT"

    i4	    	    xcb_flags;

#define			XCB_DELAYBT	    0x01L
					    /* No (journaled) BT record
					    ** has been written for this
					    ** transaction (even though it may
					    ** not be a read-only transaction).
					    ** One must be written before any
					    ** write operation can be logged. */
#define			XCB_JOURNAL	    0x02L
					    /* Transaction is journaled */
#define			XCB_NOLOGGING	    0x08L
					    /* No work for this transaction
					    ** will be written to the log file.
					    ** No recovery will be performed
					    ** if the transaction is aborted. */
#define                 XCB_USER_TRAN       0x10L
					    /* This is a "real" user transaction
					    ** as opposed to one which was
					    ** created just for statement
					    ** parsing and internal work. */
#define                 XCB_NONJNLTAB       0x20L
					    /* This flag indicates that the 
					    ** transacion involves a non
					    ** journaled table although the  
					    ** database is journaled.
					    */ 
#define                 XCB_NONJNL_BT       0x40L
                                            /* This TX has had a non-journalled
                                            ** BT record written.
                                            */
#define			XCB_SHARED_TXN	    0x80L
					    /* Shared transaction, don't
					    ** really end tx at commit
					    */ 
#define			XCB_ILOG	    0x100L
					    /* Internal logging in perhaps
					    ** a read-only txn.
					    */ 
/* String values of above bits */
#define XCB_FLAGS "\
DELAY_BT,JOURNAL,04,NOLOGGING,USER_TRAN,NONJNLTAB,NONJNLBT,\
SHARED_TXN,ILOG"

    DML_ODCB        *xcb_odcb_ptr;          /* Pointer to open database control
                                            ** block which indicates which
                                            ** database updates are allowed
                                            ** to in this transaction.  Note
                                            ** a transaction can read from 
                                            ** many databases but only update
                                            ** one. */
    i4         xcb_log_id;             /* Identifier to indicate which
                                            ** log file (database) the 
                                            ** update changes are logged to. */
    u_i2       xcb_slog_id_id;		    /* Shared log id_id, same as xcb_log_id
    					    ** unless shared transaction */
    i4         xcb_lk_id;              /* Identifier to locate the lock
                                            ** list for this transaction. */
    i4	    xcb_sp_id;		    /* Last save point identifier. */

    /* The XCCB list needs a mutex, since in a parallel query / shared
    ** transaction context, child threads can add pending-delete XCCB
    ** entries to the main session thread's list.
    ** In a few situations (e.g. DDL, relocate) if it's known that there are
    ** no child / worker threads doing pending-deletes, the mutex is not
    ** bothered with.
    */
    DM_MUTEX	    xcb_cq_mutex;	    /* Mutex for cq list */
    DML_XCCB        *xcb_cq_next;           /* Queue of commit operation control
                                            ** blocks associated with this
                                            ** transaction. */
    DML_XCCB        *xcb_cq_prev;
    DB_OWN_NAME	    xcb_username;	    /* Transaction owner name */
    DB_DIS_TRAN_ID  xcb_dis_tran_id;        /* Distributed transaction ID. */
    i4         xcb_seq_no;             /* Last seq no seen */
    i4	    xcb_rep_seq;	    /* replication seq in this tx */
    DMP_RCB	    *xcb_rep_input_q;	    /* RCB of dd_input_queue for this
					    ** session */
    i4	    xcb_rep_remote_tx;	    /* remote TX id for original
					    ** replicated update */
    DML_SEQ	    *xcb_seq;		    /* List of DML_SEQ opened
					    ** exclusively by txn */
    DML_CSEQ	    *xcb_cseq;		    /* List of txn's current
					    ** sequence values */
    PTR		    xcb_online_mxcb;	    /* online modify context */
    struct _DMPE_PCB *xcb_pcb_list;	    /* List of blob PCB's open.
					    ** Probably just 1 or 2 at a time,
					    ** hence singly linked list.
					    */

/*
**  Statistics counters for the current transaction.
**  Leave these at the end of control block.
*/
    i4	    xcb_s_open;
    i4	    xcb_s_fix;
    i4	    xcb_s_get;
    i4	    xcb_s_replace;
    i4	    xcb_s_delete;
    i4	    xcb_s_insert;
    i4	    xcb_s_cpu;
    i4	    xcb_s_dio;
    i4	    xcb_s_rc_retry;
};


/*
**  XCCB - SECTION
**
**       This section defines all structures needed to build
**       and access the information in a Transaction Commit 
**       Control Block (XCCB).
**       The following structs are defined:
**           
**           DML_XCCB                 Transaction Control Block.
*/

/*}
** Name:  DML_XCCB - DMF internal control block for commit information.
**
** Description:
**      This typedef defines the control block required to keep information
**      about operations which must be perfromed at commit time.
**
**      Most of the information in the XCCB comes from runtime information
**      associated with various operations.  
**      This control block is a fixed size and includes 
**      such information as an operation identifier, and information 
**      relating to that operation.  For example the files associated with 
**      the tables deleted during a DESTROY TABLE operation are acutally 
**      renamed at run time, an XCCB is created which contains the renamed file
**      name and physical location and a delete file operation code.
**      This is queued to the XCB to be performed at commit time.  This
**      allows operations like destroy to be able to be backed out 
**      during abort transaction or recovery.
**
**      XCCBs are queued to the transaction control block(XCB).
**      A pictorial representation of XCCBs in an active server enviroment is:
**   
**
**           XCB      
**       +-----------+ 
**       |           |                XCCB                XCCB
**       |           |             +--------+          +--------+
**       |           |             |        |          |        |
**       |  XCCB  Q  |------------>| XCCB Q |--------->| XCCB Q |
**       |           |             |        |          |        |
**       |           |             +--------+          +--------+  
**       +-----------+                       
**

**     The XCCB layout is as follows:
**                                                        
**                     +----------------------------------+
**     Byte            | xccb_q_next                      |  Initial bytes are
**                     | xccb_q_prev                      |  standard DMF CB
**                     | xccb_length                      |  header. 
**                     | xccb_type   | xccb_reserved      |
**                     | xccb_reserved                    |
**                     | xccb_owner                       |
**                     | xccb_ascii_id                    |
**                     |------ End of Header -------------|
**                     | xccb_sp_id                       |
**                     | xccb_xcb_ptr                     |
**                     | xccb_operation                   |
**                     | xccb_f_len                       |
**                     | xxcb_f_name                      |
**                     | xccb_p_len                       |
**                     | xccb_p_name                      |
**                     | xccb_t_table_id	          |
**                     | xccb_t_dcb                       |
**                     | xccb_t_tcb                       |
**                     +----------------------------------+
**                     
** History:
**      16-jan-86 (jennifer)
**          Created for Jupiter .
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	3-jun-97 (nanpr01)
**	    Fix bug 78352 : When transactions are rolled back, temporary
**	    tables that were updated does not get dropped as documented.
**	03-Dec-1998 (jenjo02)
**	    Added xccb_r_pgsize to XCCB for multi-sized
**	    raw files.
**      16-mar-1999 (nanpr01)
**	    Removed xccb_r_pgsize, xccb_r_loc_name and xccb_r_extent.
**	08-Nov-2000 (jenjo02)
**	    Deleted xccb_tq_next, xccb_tq_prev.
**	11-Jun-2004 (schka24)
**	    Added session vs transaction bool.
**	21-Jun-2004 (schka24)
**	    Added another bool, for am-I-a-blob-temp.
**	18-Jan-2005 (jenjo02)
**	    Add xccb_t_tcb, pointer to TempTable's TCB for 
**	    use by dmt_show().
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Define XCCB_CB as DM_XCCB_CB.
*/

struct _DML_XCCB
{
    DML_XCCB         *xccb_q_next;          /* Queue of XCCBs off XCB. */
    DML_XCCB         *xccb_q_prev;
    SIZE_TYPE	    xccb_length;	    /* Length of control block. */
    i2              xccb_type;              /* Type of control block for
                                            ** memory manager. */
#define                 XCCB_CB             DM_XCCB_CB
    i2              xccb_s_reserved;        /* Reserved for future use. */
    PTR             xccb_l_reserved;        /* Reserved for future use. */
    PTR             xccb_owner;             /* Owner of control block for
                                            ** memory manager.  XCCB will be
                                            ** owned by a transaction. */
    i4         xccb_ascii_id;          /* Ascii identifier for aiding
                                            ** in debugging. */
    i4	    xccb_sp_id;		    /* Savepoint at time of creation. */
#define                 XCCB_ASCII_ID       CV_C_CONST_MACRO('X', 'C', 'C', 'B')
    DML_XCB         *xccb_xcb_ptr;          /* Pointer to the transaction 
                                            ** control
                                            ** block associated with this 
                                            ** commit operation. */
    i4         xccb_operation;         /* Type of operation to perform
                                            ** at commit time. */
#define                 XCCB_F_DELETE       1L
#define                 XCCB_T_DESTROY      2L
    i4         xccb_f_len;
    DM_FILE         xccb_f_name;
    i4         xccb_p_len;
    DM_PATH         xccb_p_name;
    DB_TAB_ID       xccb_t_table_id;     
    DMP_DCB        *xccb_t_dcb;
    DMP_TCB	   *xccb_t_tcb;

    /* xccb_is_session is TRUE if the XCCB is a temp table DESTROY xccb,
    ** and the temp table is session lifetime (the XCCB is on the odcb
    ** list and uses the scb lock list).  Destroy xccb's with is-session
    ** FALSE are on the XCB list and use the XCB lock list.
    */
    bool	    xccb_is_session;

    /* xccb_is_blob_temp is TRUE if the XCCB is a blob holding table DESTROY
    ** XCCB.  (Proper usage is for it to also be is-session, although this
    ** is not checked.)  Blob holding temps are deleted en masse at
    ** blob-cleanup time.  (Said time is determined by the world outside of
    ** DMF, and can be longer than a transaction but shorter than a session.)
    ** This flag is used to identify temp tables that get cleaned up at
    ** blob-cleanup time, so that we don't have to maintain any separate
    ** lists of blob holding objects.
    */
    i4	    xccb_blob_temp_flags;
#define BLOB_HOLDING_TEMP	0x01 /* blob (non-session) temp */
				     /* Okay to destroy at end of query */
				     /* UNLESS referenced by a locator */
#define BLOB_HAS_LOCATOR	0x02 /* blob in temp has locator */
#define BLOB_LOCATOR_TBL	0x04 /* this is blob locator table */
    i4      xccb_blob_locator;       /* locator for blob in this blob temp */
};

/*}
** Name: DML_SEQ  - Sequence generator structure.
**
** Description:
**	This structure defines a Server's cache of 
**	a range of Sequence Generator values.
**
** History:
**	05-Mar-2002 (jenjo02)
**	    Created.
**	03-may-2005 (hayke02)
**	    Add pool_type to match obj_pool_type in DM_OBJECT.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Define SEQ_CB as DM_SEQ_CB.
**	13-Jul-2006 (kiria01) b112605
**	    Dropped redundant seq_version
**	24-apr-2007 (dougi)
**	    Hard-coded decimal sequence length to match iisequence.
**	4-june-2008 (dougi)
**	    Change integer values to i8 for 64-bit integer sequences.
**	15-june-2008 (dougi)
**	    Add support for unordered (random) sequences.
*/

struct _DML_SEQ
{
/* standard DM_OBJECT header */
    DML_SEQ         *seq_q_next;	    /* Next seq on some list */
    DML_SEQ         *seq_q_prev;	    /* Prev seq on some list */
    SIZE_TYPE       seq_length;             /* Length of the control block. */
    i2              seq_type;             
#define                 SEQ_CB              DM_SEQ_CB
    i2              seq_s_reserved;         /* Reserved for future use. */
    PTR             seq_l_reserved;         /* Reserved for future use. */
    PTR             seq_cb_owner;           /* Owner of control block for
                                            ** memory manager.  SEQ will be
                                            ** owned by the DCB. */
    i4              seq_ascii_id;           /* Ascii identifier for aiding
                                            ** in debugging. */
#define                SEQ_ASCII_ID         CV_C_CONST_MACRO('#', 'S', 'E', 'Q')

    i4		seq_db_id;	/* Owning database */
    DB_NAME	seq_name;	/* Sequence name */
    DB_OWN_NAME	seq_owner;	/* Sequence owner */
    u_i4	seq_id;		/* Sequence id */
    i4		seq_flags;	/* Sequence Attributes: */
#define	    SEQ_CYCLE		0x0001L
#define	    SEQ_ORDER		0x0002L
#define	    SEQ_CACHE		0x0004L
#define	    SEQ_UPDATED		0x0008L
#define	    SEQ_UNORDERED	0x0010L
#define     SEQ_SYSGEN		0x0020L
    i4		seq_lock_mode;	/* Strength of SEQUENCE lock */
    i4		seq_cache;	/* Remaining cached values */
    DB_DT_ID	seq_dtype;	/* Data type of value, int or dec */
    i2		seq_dlen;	/* Length of returned value */
    i4		seq_prec;	/* Decimal precision */
    SEQ_VALUE	seq_start;	/* Start value */
    SEQ_VALUE	seq_begin;	/* Beginning cached value */
    SEQ_VALUE	seq_next;	/* Next cached value */
    SEQ_VALUE	seq_end;	/* Ending cached value */
    SEQ_VALUE	seq_incr;	/* Increment */
    SEQ_VALUE	seq_min;	/* Minimum sequence value */
    SEQ_VALUE	seq_max;	/* Maximum sequence value */
    CS_SEMAPHORE seq_mutex;	/* In-process thread protection */
};

/*}
** Name: DML_CSEQ  - Transaction's current Sequence value.
**
** Description:
**	This structure supplies a transaction with its current
**	value of a Sequence Generator.
**
** History:
**	05-Mar-2002 (jenjo02)
**	    Created.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Define CSEQ_CB as DM_CSEQ_CB.
*/
struct _DML_CSEQ
{
/* standard DM_OBJECT header */
    DML_CSEQ        *cseq_q_next;	    /* Next on xcb->cseq */
    DML_CSEQ        *cseq_q_prev;	    /* Prev on xcb->cseq */
    SIZE_TYPE       cseq_length;            /* Length of the control block. */
    i2              cseq_type;             
#define                 CSEQ_CB             DM_CSEQ_CB
    i2              cseq_s_reserved;        /* Reserved for future use. */
    PTR             cseq_l_reserved;        /* Reserved for future use. */
    PTR             cseq_cb_owner;          /* Owner of control block for
                                            ** memory manager.  CSEQ will be
                                            ** owned by the XCB. */
    i4              cseq_ascii_id;          /* Ascii identifier for aiding
                                            ** in debugging. */
#define                CSEQ_ASCII_ID        CV_C_CONST_MACRO('C', 'S', 'E', 'Q')
    DML_SEQ	*cseq_seq;	/* Referenced DML_SEQ */
    SEQ_VALUE	cseq_curr;	/* Transaction's current value */
};


/*
**  Forward and/or External function references.
*/

    /* Translate or issue ADF error from CX execution */
FUNC_EXTERN DB_STATUS dmf_adf_error(
    ADF_ERROR	*adf_errcb,
    DB_STATUS	status,
    DML_SCB	*scb,
    DB_ERROR	*dberr);

    /* Adds a database to a server. */
FUNC_EXTERN DB_STATUS dmc_add_db(
    DMC_CB	*dmc_cb);

    /* Alters control characteristics. */
FUNC_EXTERN DB_STATUS dmc_alter(
    DMC_CB	*dmc_cb);

    /* Alters session's logging state. */
FUNC_EXTERN DB_STATUS dmc_set_logging(
    DMC_CB	*dmc_cb,
    DML_SCB	*scb,
    DMP_DCB	*dcb,
    i4	mode);

    /* Begins a session. */
FUNC_EXTERN DB_STATUS dmc_begin_session(
    DMC_CB	*dmc_cb);

    /* Closes a database for a session. */
FUNC_EXTERN DB_STATUS dmc_close_db(
    DMC_CB	*dmc_cb);

    /* Deletes a database from a server. */
FUNC_EXTERN DB_STATUS dmc_del_db(
    DMC_CB	*dmc_cb);

    /* Ends a session. */
FUNC_EXTERN DB_STATUS dmc_end_session(
    DMC_CB	*dmc_cb);

     /* Runs server protocols necessary for fast commit */
FUNC_EXTERN DB_STATUS dmc_fast_commit(
    DMC_CB	*dmc_cb);

FUNC_EXTERN DB_STATUS dmc_group_commit(DMC_CB *dmc_cb);
FUNC_EXTERN DB_STATUS dmc_recovery(DMC_CB *dmc_cb);
FUNC_EXTERN DB_STATUS dmc_logwriter(DMC_CB *dmc_cb);
FUNC_EXTERN DB_STATUS dmc_check_dead(DMC_CB *dmc_cb);
FUNC_EXTERN DB_STATUS dmc_force_abort(DMC_CB *dmc_cb);
FUNC_EXTERN DB_STATUS dmc_cp_timer(DMC_CB *dmc_cb);

FUNC_EXTERN DB_STATUS dmc_write_along(DMC_CB *dmc_cb);
FUNC_EXTERN DB_STATUS dmc_repq(DMC_CB *dmc_cb);
FUNC_EXTERN DB_STATUS dmc_repq(DMC_CB *dmc_cb);
FUNC_EXTERN DB_STATUS dmc_read_ahead(DMC_CB *dmc_cb);

    /* Opens a database for a session. */
FUNC_EXTERN DB_STATUS dmc_open_db(
    DMC_CB	*dmc_cb);

    /* Shows control characteristics. */
FUNC_EXTERN DB_STATUS dmc_show(
    DMC_CB	*dmc_cb);

    /* Starts a server. */
FUNC_EXTERN DB_STATUS dmc_start_server(
    DMC_CB	*dmc_cb);

    /* Stops a server. */
FUNC_EXTERN DB_STATUS dmc_stop_server(
    DMC_CB	*dmc_cb);

    /* reset the effective user name known to this */
FUNC_EXTERN DB_STATUS dmc_reset_eff_user_id(
    DMC_CB	*dmc_cb);

    /* Alter access characteristics to table. */
FUNC_EXTERN DB_STATUS dmr_alter(
    DMR_CB	*dmr_cb);

    /* Delete a record. */
FUNC_EXTERN DB_STATUS dmr_delete(
    DMR_CB	*dmr_cb);

    /* Reset end of file for temporary. */
FUNC_EXTERN DB_STATUS dmr_dump_data(
    DMR_CB	*dmr_cb);

    /* Get a record. */
FUNC_EXTERN DB_STATUS dmr_get(
    DMR_CB	*dmr_cb);

    /* Load a record. */
FUNC_EXTERN DB_STATUS dmr_load(
    DMR_CB	*dmr_cb);

    /* Position a record. */
FUNC_EXTERN DB_STATUS dmr_position(
    DMR_CB	*dmr_cb);

    /* Roll up arithmetic warnings from position or get qual function */
FUNC_EXTERN void dmr_adfwarn_rollup(
    ADF_CB	*to_adfcb,
    ADF_CB	*from_adfcb);

    /* Put a record. */
FUNC_EXTERN DB_STATUS dmr_put(
    DMR_CB	*dmr_cb);

    /* Replace a record. */
FUNC_EXTERN DB_STATUS dmr_replace(
    DMR_CB	*dmr_cb);

    /* Aggregate function */
FUNC_EXTERN DB_STATUS dmr_aggregate(
    DMR_CB	*dmr_cb);

    /* Unfix RCB's pages */
FUNC_EXTERN DB_STATUS dmr_unfix(
    DMR_CB	*dmr_cb);

    /* Alter information about a table. */
FUNC_EXTERN DB_STATUS dmt_alter(
    DMT_CB	*dmt_cb);

    /* Close a table. */
FUNC_EXTERN DB_STATUS dmt_close(
    DMT_CB	*dmt_cb);

    /* Create a temporary table. */
FUNC_EXTERN DB_STATUS dmt_create_temp(
    DMT_CB	*dmt_cb);

    /* Destroy a temporary table. */
FUNC_EXTERN DB_STATUS dmt_delete_temp(
    DMT_CB	*dmt_cb);

    /* Destroy helper for destroy temp table */
FUNC_EXTERN DB_STATUS dmt_destroy_temp(
    DML_SCB	*scb,
    DML_XCCB	*xccb,
    DB_ERROR	*error);

    /* Open a table. */
FUNC_EXTERN DB_STATUS dmt_open(
    DMT_CB	*dmt_cb);

    /* Estimates the cost of a sort. */
FUNC_EXTERN DB_STATUS dmt_sort_cost(
    DMT_CB	*dmt_cb);

    /* Show information about a table */
FUNC_EXTERN DB_STATUS dmt_show(
    DMT_SHW_CB	*dmt_show_cb);

    /* Check what lock a TX has on a table */
FUNC_EXTERN DB_STATUS dmt_table_locked(
    DMT_CB      *dmt_cb);

    /* Release all locks a TX has on a table */
FUNC_EXTERN DB_STATUS dmt_release_table_locks(
    DMT_CB      *dmt_cb);

    /* Take, release Control lock on a table */
FUNC_EXTERN DB_STATUS dmt_control_lock(
    DMT_CB      *dmt_cb);

    /* Invalidate (toss) a TCB  */
FUNC_EXTERN DB_STATUS	dm2tInvalidateTCB(
			DMT_CB		*dmt);

    /* Aborts a transaction. */
FUNC_EXTERN DB_STATUS dmx_abort(
    DMX_CB	*dmx_cb);

    /* Force abort xa distributed transaction */
FUNC_EXTERN DB_STATUS dmx_xa_abort(
    DMX_CB      *dmx_cb);

    /* Start xa distributed transaction */
FUNC_EXTERN DB_STATUS dmx_xa_start(
    DMX_CB      *dmx_cb);

    /* End xa distributed transaction */
FUNC_EXTERN DB_STATUS dmx_xa_end(
    DMX_CB      *dmx_cb);

    /* Prepare xa distributed transaction */
FUNC_EXTERN DB_STATUS dmx_xa_prepare(
    DMX_CB      *dmx_cb);

    /* Commit xa distributed transaction */
FUNC_EXTERN DB_STATUS dmx_xa_commit(
    DMX_CB      *dmx_cb);

    /* Rollback xa distributed transaction */
FUNC_EXTERN DB_STATUS dmx_xa_rollback(
    DMX_CB      *dmx_cb);

    /* Check transaction for external interrupts */
FUNC_EXTERN DB_STATUS dmxCheckForInterrupt(
    DML_XCB	*xcb,
    i4	*error);

    /* Begins a transaction. */
FUNC_EXTERN DB_STATUS dmx_begin(
    DMX_CB	*dmx_cb);

    /* Commits a transaction. */
FUNC_EXTERN DB_STATUS dmx_commit(
    DMX_CB	*dmx_cb);

    /* Commits a transaction, keeps log/log context and locks. */
FUNC_EXTERN DB_STATUS dmx_logfull_commit(
    DML_XCB	*xcb,
    DB_ERROR	*error);

    /* Resume a transaction. */
FUNC_EXTERN DB_STATUS dmx_resume(
    DMX_CB	*dmx_cb);

    /* Prepare to commit a transaction. */
FUNC_EXTERN DB_STATUS dmx_secure(
    DMX_CB	*dmx_cb);

    /* Declares a transaction savepoint. */
FUNC_EXTERN DB_STATUS dmx_savepoint(
    DMX_CB	*dmx_cb);

    /* Determines if data page checksums are used. */
FUNC_EXTERN VOID dmc_setup_checksum(VOID);

    /* Checks user supplied table locking values. */
FUNC_EXTERN VOID dmt_check_table_lock(
                     DML_SCB         *scb,
                     DMT_CB          *dmt,
                     i4         *timeout,
                     i4         *maxlocks,
		     i4         *readlock,
                     i4         *lock_level);

    /* Sets table locking characteristics */
STATUS dmt_set_lock_values(
                     DMT_CB          *dmt,
                     i4         *timeout_pt,
                     i4         *max_locks_pt,
		     i4         *readlock_pt,
                     i4         *lock_level_pt,
                     i4         *iso_level_pt,
                     i4         *lock_mode_pt,
		     i4          est_pages,
		     i4          total_pages);

    /* Gets the table ID given the table name */
FUNC_EXTERN DB_STATUS dml_get_tabid(
		     DMP_DCB         *dcb,
		     DB_TAB_NAME     *table_name,
		     DB_OWN_NAME     *table_owner,
		     DB_TAB_ID       *table_id,
		     DB_ERROR        *dberr);

    /* Sequence generator functions */
FUNC_EXTERN DB_STATUS	dms_open_seq(PTR    dms_cb);
FUNC_EXTERN DB_STATUS	dms_close_seq(PTR   dms_cb);
FUNC_EXTERN DB_STATUS	dms_next_seq(PTR    dms_cb);
FUNC_EXTERN VOID	dms_close_db(
			DMP_DCB		*dcb);
FUNC_EXTERN VOID	dms_destroy_db(
			i4		db_id);
FUNC_EXTERN VOID	dms_stop_server(void);
FUNC_EXTERN DB_STATUS	dms_end_tran(
			i4		txn_state,
#define		DMS_TRAN_COMMIT	1L
#define		DMS_TRAN_ABORT	2L
			DML_XCB		*xcb,
			DB_ERROR	*error);
FUNC_EXTERN DB_STATUS	dms_fetch_iiseq(
			PTR		dms,
			DML_XCB		*xcb,
			DML_SEQ   	*s);
FUNC_EXTERN DB_STATUS	dms_update_iiseq(
			PTR		dms,
			DML_XCB		*xcb,
			DML_SEQ   	*s);
FUNC_EXTERN i8		dms_nextUnorderedVal(
			i8		currval,
			i4		seqlen);

    /* FEXI functions in DMF */
FUNC_EXTERN DB_STATUS	dmf_last_id(
			i8		*id);

FUNC_EXTERN DB_STATUS	dmf_tbl_info(
			i4		*reltid,
			i4		*reltidx,
			i4		op_code,
			char		*tableinfo_request,
			i4		*return_count);
