/*
**Copyright (c) 2004, 2010 Ingres Corporation
** All Rights Reserved
*/

/**
** Name: DMPEPCB.H - Peripheral Control Block Data Structure
**
** Description:
**      This file contains the definition for the peripheral control block and
**	related items.
**
** History:
**      02-Jan-1990 (fred)
**          Created.
**	06-mar-1996 (stial01 for bryanp)
**	    Made component size dependent on DB_MAXSTRING rather than
**		DB_MAXTUP since the maximum row size may significantly differ
**		from the maximum column size.
**      27-mar-2000 (stial01)
**          Added pcb_att_id
**	08-mar-2001 (somsa01)
**	    DB_MAXSTRING has increased to 32K, but the segment length is
**	    still 2K. Thus, changed component size such that is is dependent
**	    on a hard value of 2K.
**      08-Oct-2002 (hanch04)
**          Replace the i4 for length with a SIZE_TYPE (size_t) to allow
**          64-bit memory allocation.
**      05-Aug-2005 (hanje04)
**          Back out change 478041 as it should never have been X-integrated
**          into main in the first place.
[@history_template@]...
**/

/*
**  Forward and/or External typedef/struct references.
*/

typedef struct _DMPE_PCB DMPE_PCB;

/*
**  Defines of other constants.
*/

/*
**	This define declares the length of the varchar segment within the
**	blob.
*/
#define DMPE_SEGMENT_LENGTH     2000

/*
**      Definitions of overall constants used to work with peripheral objects.
*/

/*}
** Name: DMPE_RECORD - The actual record used in the table.
**
** Description:
**      This structure describes the structure used in the peripheral table to
**	hold each record.  This must correspond to the table creation used in
**	dmpe_findspace().
**
** History:
**      03-Jan-1990 (fred)
**          Created.
**	06-mar-1996 (stial01 for bryanp)
**	    Made component size dependent on DB_MAXSTRING rather than
**		DB_MAXTUP since the maximum row size may significantly differ
**		from the maximum column size.
**	08-mar-2001 (somsa01)
**	    DB_MAXSTRING has increased to 32K, but the segment length is
**	    still 2K. Thus, changed component size such that is is dependent
**	    on a hard value of 2K.
[@history_template@]...
*/
typedef struct _DMPE_RECORD
{
    DB_TAB_LOGKEY_INTERNAL
		    prd_log_key;	    /* Ptr to record's key section */
    u_i4	    prd_segment0;	    /* Ptr into record to segment */
    u_i4	    prd_segment1;
    u_i4	    prd_r_next;
#define			DMPE_TCPS_TST_COMP_PART_SIZE  16
#define                 DMPE_CPS_COMPONENT_PART_SIZE	\
	    (DMPE_SEGMENT_LENGTH -	\
		(sizeof(DB_TAB_LOGKEY_INTERNAL) \
			+ (2 * sizeof(i4)) + sizeof(u_i4)))
    char	    prd_user_space[DMPE_CPS_COMPONENT_PART_SIZE];
					    /* datatype area */
}   DMPE_RECORD;

/*}
** Name: DMPE_PCB - Peripheral Control Block
**
** Description:
**	The PCB holds the DMF information that's needed during a single
**	Peripheral Operation (POP).  (e.g. one complete get, put, etc.)
**	POP's may take multiple stages, thus multiple trips through dmpe.
**	The PCB is what remembers things like the etab DMR_CB and
**	DMT_CB for the duration of the operation.
**
**	A PCB is usually allocated (dmpe_allocate) at the start of a POP
**	when dmpe sees the ADP_C_BEGIN_MASK flag, and is deallocated at
**	the end of the POP, or by an explicit ADP_CLEANUP call if
**	a filter function ends the POP prematurely (before processing
**	all segments).  The exception happens when a PUT POP is bulk-loading
**	one or more etabs;  in order to keep the etabs being loaded open
**	for the duration of the load (which might cover many POP's),
**	it turns out to be convenient to find and re-use the PCB
**	and its attached DMR/DMT CB's for as long as the bulk-load lasts.
**	The DMP_ET_LIST entry for any etab being bulk-loaded will point to
**	the PCB driving the bulk-load.  (Multiple DMP_ET_LIST entries
**	might point to the same PCB if we had to fill multiple etabs.)
**
**	Note that all phases of one POP take place within a single
**	thread, and hence uses that thread's transaction context.
**	This is different from the situation with a coupon.  A coupon
**	can be passed from thread to thread in a parallel query, and
**	so the coupon may only contain thread- and open-instance-independent
**	stuff (e.g. base table and attribute ID numbers; see dmpepcn.h).
**
** History:
**      02-Jan-1990 (fred)
**          Created.
**	05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with
**	    DM_SVCB.
**	24-mar-99 (stephenb)
**	    Add pcb_fblk to contain the function index for the function that
**	    can be used to coerce the passed data into the target etab field
**	    Also add pcb_base_dmtcb to hold the table CB for the base table
**	7-May-2004 (schka24)
**	    Because of parallel query, various stages in the life of a
**	    coupon can't expect the same RCB.  Change base-rcb ptr to
**	    base-TCB pointer.  Add comments based on my current, perhaps
**	    defective, understanding...
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Define PCB_CB as DM_PCB_CB.
**	16-Apr-2010 (kschendel) SIR 123485
**	    Delete pcb-base-dmt stuff, add "did we fix tcb" bool.
**	    Add row counts and "held" record to fix bug with bulk loading
**	    etabs (wasn't checking for an etab being full).
*/
struct _DMPE_PCB
{
    DMPE_PCB        *pcb_q_next;
    DMPE_PCB        *pcb_q_prev;
    SIZE_TYPE	    pcb_length;		    /* Length of control block. */
    i2              pcb_type;               /* Type of control block for 
                                            ** memory manager. */
#define                 PCB_CB              DM_PCB_CB
    i2              pcb_s_reserved;         /* Reserved for future use. */
    PTR             pcb_l_reserved;         /* Reserved for future use. */
    PTR             pcb_owner;              /* Owner of control block for
                                            ** memory manager.  pcb will be
                                            ** owned by the server. */
    i4         pcb_ascii_id;           /*
					    ** Ascii identifier for aiding
					    ** in debugging.
					    */
#define                 PCB_ASCII_ID        CV_C_CONST_MACRO('#', 'P', 'C', 'B')
	/* EO Standard Stuff */
    DMPE_PCB	    *pcb_self;		    /* Ptr to self for parm check */
    DMR_CB	    *pcb_dmrcb;		    /* DMR_CB for etab row opers */
    DMT_CB	    *pcb_dmtcb;		    /* DMT_CB for etab table opers */
    DMPE_RECORD	    *pcb_record;	    /* Ptr to record itself */
    DMPE_RECORD	    *pcb_held_record;	    /* Previous segment not yet written
					    ** (only when bulk-loading). See
					    ** dmpe-put for more info
					    */
    void	    *pcb_held_access_id;    /* Access-ID to use to write the
					    ** held record (could be prior
					    ** rather than current etab).
					    */
    DMP_ET_LIST	    *pcb_et;		    /* Pointer to entry on TCB's
					    ** etab list for the etab currently
					    ** being put into (put only).
					    */
    CS_SID	    pcb_sid;		    /* Session ID */
    i4		    pcb_cat_scan;	    /* Have we scanned the catalog */
    u_i4	    pcb_seg0_next;	    /* Next segment we need */
    u_i4	    pcb_seg1_next;

    /* If loading an etab, limit the number of rows loaded to any given one
    ** so that we don't exceed the max size of a single etab.
    */
    u_i4	    pcb_loaded;		    /* Rows loaded into this etab */
    u_i4	    pcb_per_etab;	    /* Guesstimate at max rows per etab
					    ** before making it too full
					    */

    /* etab opens need a transaction context and database context.
    ** We'll get those when the PCB is created.
    */
    DML_XCB	    *pcb_xcb;		    /* Current thread XCB (tran ID) */
    PTR		    pcb_db_id;		    /* Database ID (ODCB) */
    struct _DMPE_BQCB *pcb_bqcb;	    /* BLOB query context pointer */

    /* The base TCB is filled in only if it's a DML operation, get or put.
    ** See also pcb_fixed_tcb below.
    */
    DMP_TCB	    *pcb_tcb;		    /* TCB of base table of manipulated
					    ** object.
					    */
    DB_TAB_ID	    pcb_table_previous;	    /*
					    ** Table id of `previous' for this
					    ** peripheral object.
					    */
    ADF_FN_BLK	    pcb_fblk;		    /* coercion function for
					    ** new inserts */
    i4              pcb_att_id;             /* base table att_id */
    /* Singly-linked list of PCB's belonging to a thread, headed by
    ** xcb's xcb_pcb_list.
    */
    DMPE_PCB	    *pcb_xq_next;	    /* PCB list for cleanup */

    /* TRUE if dmpe-begin-dml had to fix the base table TCB itself.
    ** An unfix must be done when the PCB is deallocated.
    ** If FALSE, something else (eg caller RCB) is holding the TCB
    ** open for the duration of the operation, and no fix/unfix was needed.
    */
    bool	    pcb_fixed_tcb;	    /* TRUE if unfix needed */
};

void dmpe_deallocate(DMPE_PCB *pcb);
