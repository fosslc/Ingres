/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DM2T.H - Definitions for TCB manipulation routines.
**
** Description:
**      This file decribes the routines that manipulate TCB's.
**
** History: $Log-for RCS$
**      07-dec-1985 (derek)
**          Created new for Jupiter.
**	23-oct-1988 (teg)
**	    Added DM2T_SHOW_READ to indicate read request comes from dmt_show
**	    and DM2T_DEL_WRITE for dropping tables.
**	 6-Feb-1989 (rogerk)
**	    Added DM2T_TOSS flag for release_tcb to toss cached pages.
**	14-jun-90 (linda, bryanp)
**	    Added DM2T_A_OPEN_NOACCESS to indicate open where no other access
**	    than table close can occur.  Used by gateways only (for now) to
**	    support remove table where underlying data file does not exist.
**	18-jun-90 (linda, bryanp)
**	    Add DM2T_A_MODIFY access code, indicating that we are opening the
**	    table only to get statistical information.  For Gateway tables,
**	    this means that it is okay if the file does not exist; default
**	    statistics will be used in that case.
**	14-aug-90 (rogerk)
**	    Add new flags for dm2t_get_tcb_ptr calls.
**	15-jan-1992 (bryanp)
**	    Added DM2T_TMP_MODIFIED to indicate rebuilding a temp table TCB
**	    following a modify of the table. Added DM2T_DESTROY flag to
**	    dm2t_close to indicate that this table should be automatically
**	    destroyed when its reference count goes to 0.
**	 7-jul-1992 (rogerk)
**	    Prototyping DMF.
**	 16-oct-1992 (rickh)
**	    Improvements for FIPS CONSTRAINTS.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project:  Added prototypes and flag constants for
**	    new table management routines. Removed old routines dm2t_locate_tcb,
**	    dm2t_hunt_tcb, dm2t_find_tcb, and dm2t_get_tcb_ptr.
**	    Added new fix and unfix tcb flags.
**	23-nov-1992 (jnash)
**	    Reduced Logging Project:  
**	      - Eliminate dm2t_rename_file FUNC_EXTERN.
**	      - Add dm2t_bumplock_tcb FUNC_EXTERN.
**	18-jan-1993 (bryanp)
**	    Add prototype for dm2t_reserve_tcb(), add DM2T_TMPMOD_WAIT flag.
**	26-jul-1993 (bryanp)
**	    Replace all uses of DM_LKID with LK_LKID.
**	9-nov-93 (robf)
**          Add prototype for dm2t_lookup_tabname
**	20-jun-1994 (jnash)
** 	    fsync project.  Add DM2T_SYNC_AT_CLOSE dm2t_open() access mode, 
**	    causing async I/O on underlying files (on file systems supporting 
**	    fsync).  Files are sync'd in dm2t_close().  Add corresponding 
**	    DM2T_CLSYNC dm2t_fix_tcb flag.  
**      10-nov-1994 (cohmi01)
**          Add dm2t_get_timeout FUNC_EXTERN.
**      22-nov-1994 (andyw)
**          Partial Backup/Recovery Project:
**              Add DM2T_RECOVERY for table level rollforwarddb
**      04-apr-1995 (dilma04)
**          Add support for CURSOR STABILITY and REPEATABLE READ isolation.
**	19-may-1995 (lewda02)
**	    RAAT API Project:
**	    Add prototype for dm2t_check_setlock to make it external.
**	21-aug-1995 (tchdi01)
**	    Production Mode Project:
**	    Add a new table access mode DM2T_A_SET_PRODUCTION
**	06-mar-1996 (stial01 for bryanp)
**	    Add page_size argument to dm2t_init_tabio_cb and dm2t_add_tabio_file
**		for managing a table's page size during recovery.
**      01-aug-1996 (nanpr01 for ICL phil.p)
**          Introduced definition of TCB callback routine dm2t_tcb_callback,
**          release flag DM2T_FLUSH, dm2t_xlock_tcb and dm2t_convert_tcb_lock
**          for suppport of DMCM protocol.
**      01-aug-1996 (nanpr01)
**          Added lock_key as parameter to the call back function since
**	    lockid may be reused. Also added dm2t_convert_bumplock_tcb
**	    to convert and bump the lock value.
**	22-Jan-1997 (jenjo02)
**	    Added prototype for dmt2_alloc_tcb() to consolidate
**	    TCB allocation and initialization.
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          - add DMT_RIS and DMT_RIX values for lock mode;
**          - changed lock_duration parameter in dm2t_open() to iso_level; 
**          - removed lock duration value definitions;
**          - removed dm2t_check_setlock() prototype.
**	25-aug-97 (stephenb)
**	    add dml_scb parameter to dm2t_open call.
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to dm2t_unfix_tcb(), dm2t_release_tcb(),
**	    dm2t_reclaim_tcb(), dm2t_destroy_temp_tcb(), dm2t_ufx_tabio_cb(),
**	    dm2t_add_tabio_file(), dm2t_init_tabio_cb() prototypes 
**	    to be passed eventually to dm0p_close_page().
**      28-May-1998 (stial01)
**          Added prototype for dm2t_lock_row
**      16-feb-1999 (nanpr01)
**          Support update mode lock.
**      12-apr-1999 (stial01)
**          Changed prototype for dm2t_alloc_tcb
**	29-Jun-1999 (jenjo02)
**	    Added 2 new external function prototypes for GatherWrite,
**	    dm2t_pin_tabio_cb() and dm2t_unpin_tabio_cb().
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	14-aug-2003 (jenjo02)
**	    Made DMP_TCB *tcb parm in dm2t_release_tcb()
**	    and dm2t_unfix_tcb() a pointer-to-pointer
**	    so that caller's TCB pointer can be
**	    obliterated.
**	15-Jan-2004 (schka24)
**	    Don't need hash bucket for dm2t_temp_build_tcb.
**	11-Jun-2004 (schka24)
**	    Eliminate unused destroy-temp flag, could cause rdf sync problems.
**	15-Mar-2005 (schka24)
**	    Add show-only flag.
**	15-Aug-2006 (jonj)
**	    Add IdomMap param to dm2t_temp_build_tcb prototype for 
**	    TempTable indexes.
**	04-Apr-2008 (jonj)
**	    Eviscerate dm2t_xlock_tcb(), dm2t_convert_bumplock_tcb(), 
**	    use more robust redefined dm2t_bumplock_tcb(). 
**	    dm2t_convert_tcb_lock() is a static function.
**	13-May-2008 (jonj)
**	    Add DM2T_RECLAIM flag bit for dm2t_release_tcb().
**	7-Aug-2008 (kibro01) b120592
**	    Add flag to dm2t_temp_build_tcb to note we're coming from
**	    an online modify.
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2t_? functions converted to DB_ERROR *
**	16-Nov-2009 (kschendel) SIR 122890
**	    Update dm2t-destroy-temp-tcb call: remove log-d since it's
**	    meaningless for temp tables, and remove opflag since it's unused.
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add DM2T_MIS, DM2T_MIX Lock modes
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support:
**	    Add lockLevel to dm2t_row_lock() prototype.
**/

/*
**  Forward and/or External function references.
*/

typedef struct  _DM2T_KEY_ENTRY DM2T_KEY_ENTRY;

/*
**  The following constants are dependent on values in the logical interface
**  and the TCB.  If any of these values change the code will have to be
**  inserted to map from one set of constants to another.  As of June 1990,
**  these codes are remapped to RCB_A_* access modes in dm2t_open().  (bryanp)
*/

/*
**  Table access mode constants.
*/

#define			DM2T_A_READ		((i4)1)
#define                 DM2T_A_WRITE		((i4)2)
#define			DM2T_SHOW_READ		((i4)3)
#define			DM2T_DEL_WRITE		((i4)4)
#define			DM2T_A_OPEN_NOACCESS	((i4)5)
#define			DM2T_A_MODIFY		((i4)6)
#define			DM2T_A_SYNC_CLOSE	((i4)7)
#define			DM2T_A_SET_PRODUCTION	((i4)8)

/* 
**  Table lock mode constants.
*/

#define                 DM2T_X              LK_X
                                                /* Exclusive Table lock. */
#define                 DM2T_U              LK_U
                                                /* Update lock. - not used */
#define                 DM2T_SIX            LK_SIX 
                                                /* Shared Table lock with
                                                ** intent to update. */
#define                 DM2T_S              LK_S 
                                                /* Shared Table lock. */
#define                 DM2T_IX             LK_IX 
                                                /* Exclusive Page lock. */
#define                 DM2T_IS             LK_IS 
                                                /* Shared Page lock. */
#define                 DM2T_N              LK_N
                                                /* No read lock protocol */
#define                 DM2T_RIS            ((i4)-1)
                                                /* Shared Row lock. */
#define                 DM2T_RIX            ((i4)-2) 
                                                /* Exclusive Row lock. */
#define                 DM2T_MIS            ((i4)-3) 
                                                /* Shared MVCC lock */
#define                 DM2T_MIX            ((i4)-4) 
                                                /* Exclusive MVCC lock */
/*
** Table update mode constants.
*/

#define                 DM2T_UDEFERRED	    ((i4)1)
#define                 DM2T_UDIRECT	    ((i4)2)

/*
** Flags for close table.
** Each value must define a unique bit in the flag.
*/

#define			DM2T_NOPURGE	    ((i4)0x00)
#define			DM2T_PURGE	    ((i4)0x01)
#define			DM2T_NORELUPDAT	    ((i4)0x02)
#define			DM2T_UPDATE	    ((i4)0x04)
#define			DM2T_KEEP	    ((i4)0x08)
#define			DM2T_TOSS	    ((i4)0x10)
#define			DM2T_RECLAIM	    ((i4)0x20)
#define                 DM2T_FLUSH          ((i4)0x40)

/*
** Temporary table processing constants.
*/

/* unused		unused			    0x01 */
#define                 DM2T_LOAD		    ((i4)0x02)     
#define                 DM2T_NOLOAD		    ((i4)0x03)     
#define			DM2T_TMP_MODIFIED	    ((i4)0x04)
#define			DM2T_TMP_INMEM_MOD	    ((i4)0x05)

/*
** Isolation level flag.
*/

#define                 DM2T_DEFAULT_ISOLATION      ((i4)0)
#define                 DM2T_READ_UNCOMMITTED       ((i4)1)
#define                 DM2T_CURSOR_STABILITY       ((i4)2)
#define                 DM2T_REPEATABLE_READ        ((i4)3)
#define                 DM2T_SERIALIZABLE           ((i4)4)

/*
** Flags for fix/unfix TCB calls
*/

#define                 DM2T_NOWAIT		    ((i4)0x0001)
#define                 DM2T_NOBUILD		    ((i4)0x0002)
#define                 DM2T_NOPARTIAL		    ((i4)0x0004)
#define                 DM2T_VERIFY		    ((i4)0x0008)
#define                 DM2T_EXCLUSIVE		    ((i4)0x0010)
#define                 DM2T_CLSYNC		    ((i4)0x0020)
#define                 DM2T_RECOVERY               ((i4)0x0040)
#define			DM2T_SHOWONLY		    ((i4)0x0080)

/*
** Wait_reason values for dm2t_reserve_tcb:
*/
#define			DM2T_TMPMOD_WAIT	    ((i4)1)

/*}
** Name:  DM2T_ATTR_ENTRY - Structure to identify attributes for table create.
**
** Description:
**      This typedef defines the structured used in a DMT_CREATE_TEMP 
**      operation to describe the attributes of the table to be created.
**
**      Note:  Included structures are defined in DMF.H if they start with
**      DM_, otherwise they are defined as global DBMS types.
** 
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.
**	16-oct-92 (rickh)
**	    FIPS CONSTRAINTS:  default id, default tuple pointer.
**	13-dec-04 (inkdo01)
**	    Added attr_collID for column level collation support.
**	07-Dec-09 (troal01)
**	    Consolidated with DMF_ATTR_ENTRY.
*/

/*
 * Removed.
 */

/*}
** Name:  DM2T_KEY_ENTRY - Description of key for a database table.
**
** Description:
**      This structure is used to describe the key attributes of
**      the table being created.
**	This structure is the same as the logical interface(DMT_KEY_ENTRY),
**	if either changes, need to apply to both places.
**
** History:
**      02-jun-85 (ac)
**          Created for Jupiter.
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedef to quiesce the ANSI C 
**	    compiler warnings.
*/
struct _DM2T_KEY_ENTRY
{
    DB_ATT_NAME     key_attr_name;          /* Name of table attribute. */
    i4         key_order;              /* Sort order of attribute. */
#define                 DM2T_ASCENDING       1L
#define                 DM2T_DESCENDING      2L
};

/*
** Function Prototypes
*/

FUNC_EXTERN DB_STATUS	dm2t_control(
					DMP_DCB		    *dcb,
					i4		    base_id,
					i4		    lock_list,
					i4		    lock_mode,
					i4		    flags,
					i4		    timeout,
					DB_ERROR            *dberr);

FUNC_EXTERN DB_STATUS	dm2t_close(
					DMP_RCB             *rcb,
					i4		    flag,
					DB_ERROR            *dberr);

FUNC_EXTERN DB_STATUS	dm2t_open(
					DMP_DCB             *dcb,
					DB_TAB_ID           *table_id,
					i4             lock_mode,
					i4             update_mode,
					i4             req_access_mode,
					i4             timeout,
					i4             maxlocks,
					i4		    sp_id,
					i4		    log_id,
					i4		    lock_id,
					i4             sequence,
					i4             iso_level,
					i4             db_lockmode,
					DB_TRAN_ID	    *tran_id,
					DB_TAB_TIMESTAMP    *timestamp,
					DMP_RCB             **rcb,
					DML_SCB		    *scb,
					DB_ERROR            *dberr);

FUNC_EXTERN DB_STATUS	dm2t_fix_tcb(
					i4		    db_id,
					DB_TAB_ID           *table_id,
					DB_TRAN_ID          *tran_id,
					i4             lock_list,
					i4             log_id,
					i4		    flags,
					DMP_DCB             *dcb,
					DMP_TCB		    **tcb,
					DB_ERROR            *dberr);

FUNC_EXTERN DB_STATUS	dm2t_unfix_tcb(
					DMP_TCB		    **tcb,
					i4		    flags,
					i4		    lock_list,
					i4		    log_id,
					DB_ERROR            *dberr);

FUNC_EXTERN DB_STATUS	dm2t_lock_table(
					DMP_DCB		    *dcb,
					DB_TAB_ID	    *table_id,
					i4		    request_mode,
					i4		    lock_list,
					i4		    timeout,
					i4		    *grant_mode,
					LK_LKID		    *lockid,
					DB_ERROR            *dberr);

FUNC_EXTERN DB_STATUS	dm2t_reclaim_tcb(
					i4		    lock_list,
					i4		    log_id,
					DB_ERROR            *dberr);

FUNC_EXTERN DB_STATUS	dm2t_release_tcb(
					DMP_TCB            **tcb,
					i4            flag,
					i4		   lock_list,
					i4		   log_id,
					DB_ERROR            *dberr);

FUNC_EXTERN DB_STATUS	dm2t_temp_build_tcb(
					DMP_DCB             *dcb,
					DB_TAB_ID           *table_id,
					DB_TRAN_ID	    *tran_id,
					i4		    lock_id,
					i4		    log_id,
					DMP_TCB             **tcb,
					DMP_RELATION        *relation,
					DMF_ATTR_ENTRY	    **attr_array,
					i4             	    attr_count,
					DM2T_KEY_ENTRY	    **key_array,
					i4             	    key_count,
					i4             	    flag,
					i4		    recovery,
					i4		    num_alloc,
					DB_LOC_NAME	    *location,
					i4		    loc_count,
					i4		    *IdomMap,
					DB_ERROR            *dberr,
					bool		    from_online_modify);

FUNC_EXTERN DB_STATUS	dm2t_destroy_temp_tcb(
					i4		    lock_list,
					DMP_DCB             *dcb,
					DB_TAB_ID           *table_id,
					DB_ERROR            *dberr);

FUNC_EXTERN DB_STATUS	dm2t_purge_tcb(
					DMP_DCB             *dcb,
					DB_TAB_ID           *table_id,
					i4             flag,
					i4		    log_id,
					i4		    lock_id,
					DB_TRAN_ID	    *tran_id,
					i4             db_lockmode,
					DB_ERROR            *dberr);

FUNC_EXTERN void	dm2t_current_counts(
					DMP_TCB	*tcb,
					i4	*reltups,
					i4	*relpages);

FUNC_EXTERN DB_STATUS	dm2t_update_logical_key(
					DMP_RCB	*input_rcb,
					DB_ERROR            *dberr);

FUNC_EXTERN DB_STATUS	dm2t_lookup_tabid(
					DMP_DCB		*dcb,
					DB_TAB_NAME     *table_name,
					DB_OWN_NAME     *owner_name,
					DB_TAB_ID       *table_id,
					DB_ERROR            *dberr);

FUNC_EXTERN DB_STATUS	dm2t_lookup_tabname(
					DMP_DCB		*dcb,
					DB_TAB_ID       *table_id,
					DB_TAB_NAME     *table_name,
					DB_OWN_NAME     *owner_name,
					DB_ERROR            *dberr);

FUNC_EXTERN VOID	dm2t_wait_for_tcb(
					DMP_TCB	    *tcb,
					i4	    lock_list);

FUNC_EXTERN VOID	dm2t_awaken_tcb(
					DMP_TCB	    *tcb,
					i4	    lock_list);

FUNC_EXTERN DB_STATUS   dm2t_fx_tabio_cb(
					i4         db_id,
					DB_TAB_ID       *table_id,
					DB_TRAN_ID      *tran_id,
					i4         lock_id,
					i4         log_id,
					i4         flags,
					DMP_TABLE_IO    **tabio_cb,
					DB_ERROR            *dberr);


FUNC_EXTERN DB_STATUS	dm2t_ufx_tabio_cb(
					DMP_TABLE_IO	*tabio_cb,
					i4		flags,
					i4		lock_id,
					i4		log_id,
					DB_ERROR            *dberr);

FUNC_EXTERN DB_STATUS	dm2t_init_tabio_cb(
					DMP_DCB		*dcb,
					DB_TAB_ID	*table_id,
					DB_TAB_NAME	*table_name,
					DB_OWN_NAME	*table_owner,
					i4		loc_count,
					i4		lock_id,
					i4		log_id,
					i4		page_type,
					i4		page_size,
					DB_ERROR            *dberr);

FUNC_EXTERN DB_STATUS	dm2t_add_tabio_file(
					DMP_DCB		*dcb,
					DB_TAB_ID	*table_id,
					DB_TAB_NAME	*table_name,
					DB_OWN_NAME	*table_owner,
					i4		location_id,
					i4		location_number,
					i4		location_count,
					i4		lock_id,
					i4		log_id,
					i4		page_type,
					i4		page_size,
					DB_ERROR            *dberr);

FUNC_EXTERN VOID	dm2t_wt_tabio_ptr(
					i4		db_id,
					DB_TAB_ID	*table_id,
					i4		mode,
					i4		lock_id);

FUNC_EXTERN DB_STATUS 	dm2t_bumplock_tcb(
					DMP_DCB         *dcb,
					DMP_TCB		*tcb,
					DB_TAB_ID       *table_id,
					i4		increment,
					DB_ERROR            *dberr);

FUNC_EXTERN DB_STATUS	dm2t_reserve_tcb(
					DMP_TCB		*tcb,
					DMP_DCB		*dcb,
					i4		wait_reason,
					i4		lock_list,
					DB_ERROR            *dberr);

FUNC_EXTERN i4     dm2t_get_timeout(
                                        DML_SCB		*scb,
                                        DB_TAB_ID	*table_id);

FUNC_EXTERN DB_STATUS   dm2t_tcb_callback(
                                        i4              lock_mode,
                                        PTR             ptr,
                                        LK_LKID         *lockid,
					LK_LOCK_KEY	*lock_key);

FUNC_EXTERN DB_STATUS   dm2t_alloc_tcb(
					DMP_RELATION    *rel,
					i4		loc_count,
					DMP_DCB         *dcb,
					DMP_TCB         **tcb,
					DB_ERROR            *dberr);

FUNC_EXTERN bool        dm2t_row_lock(
					DMP_TCB         *tcb,
					i4		lockLevel);

FUNC_EXTERN VOID        dm2t_pin_tabio_cb(
					DMP_TABLE_IO    *tabio_cb);

FUNC_EXTERN VOID        dm2t_unpin_tabio_cb(
					DMP_TABLE_IO    *tabio_cb,
					i4		lock_list);
