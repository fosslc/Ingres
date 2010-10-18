/*
** Copyright (c) 1985, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <pc.h>
#include    <tr.h>
#include    <sl.h>
#include    <cvcl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <lkdef.h>
#include    <di.h>
#include    <sr.h>
#include    <ex.h>
#include    <adf.h>
#include    <scf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dm0p.h>
#include    <dm0m.h>
#include    <dm1b.h>
#include    <dm0pbmcb.h>
#include    <dm1p.h>
#include    <dmucb.h>
#include    <dmse.h>
#include    <dm2t.h>
#include    <dm2umct.h>
#include    <dm2umxcb.h>
#include    <dm2u.h>
#include    <dm0llctx.h>
#include    <dm0l.h>
#include    <dmd.h>
#include    <dmrthreads.h>
#include    <dmpecpn.h>

/**
**
**  Name: DMDCB.C - DMF debugging routine for control blocks.
**
**  Description:
**      The routine in this file deal with printing out individual DMF
**	control blocks or the whole DMF control block space.
**
**          dmd_fmt_cb - Format a control block.
**          dmd_dmp_pool - Dump control block pool.
**
**
**  History:
**      05-nov-1985 (derek)    
**          Created new for Jupiter.
**	20-feb-1989 (rogerk)
**	    Changed for shared buffer manager.  Added new fields for BMCB,
**	    Buffer Manager structures, and TCB.  Added LBMCB control block.
**	    Formatted BMCB when find LBMCB instead of BMCB since the latter
**	    may not be found if allocated in shared memory.
**	    Increased size of count array as a new control block was added.
**	 3-apr-1989 (rogerk)
**	    Fix semaphore bug when dumping buffer manager control block.
**	10-apr-1989 (rogerk)
**	    Format buffer manager name when dumping buffer manager.
**	15-aug-1989 (rogerk)
**	    Added support for TCB_GATEWAY tables.
**	    Added MULTI_LOC, GATEWAY, RULE , and SYSTEM_MAINTAINED to format
**	    string for printing out table attributes.  Added formatting of
**	    new tcb attribute bits.
**	14-feb-1991 (bryanp)
**	    Updates for Btree index compression project. Also, since I was here,
**	    I brought a number of out-of-date TRdisplays up to date.
**	19-aug-1991 (rogerk)
**	    Report new svcb_status values.  This was done as part of the
**	    bugfix for B39017.
**	26-sept-1991 (rmuth)
**	    Next 2 history comments are from merge65, also updated CB info.
**	  27-sep-89 (jrb)
**	    Added DECIMAL, OBJECT_KEY, and TABLE_KEY to datatype names displayed
**	    for debugging purposes.
**	  24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.
**	15-oct-1991 (mikem)
**	    Removed TRdisplay of DI_FCB fields.  These fields can not be 
**	    referenced outside of the CL as they may differ from platform to
**	    platform.  Also eliminated compile warnings on UNIX by removing 
**	    unneeded "&" from in front of references to dcb_colname and
**	    db_own_name structure members which are both arrays.
**	3-feb-1992 (bryanp)
**	    Add TCB_PEND_DESTROY to tcb_status.
**	7-july-1992 (jnash)
**	    DMF Function prototypes.
**      24-sep-1992 (stevet)
**          Replaced display for dcb->dcb_timezone and scb->scb_timezone
**          with dcb->scb_tzcb and scb->scb_tzcb.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	05-oct-1992 (rogerk)
**	    Reduced Logging Project.  In preparation for support of Partial
**	    TCB's for the new recovery schemes, changed references to
**	    some tcb fields to use the new tcb_table_io structure fields.
**	26-oct-1992 (rogerk)
**	    Added display of new tcb status fields.
**	26-oct-92 (rickh)
**	    Replaced references to DMP_ATTS with DB_ATTS.  Also replaced
**	    DM_CMP_LIST with DB_CMP_LIST.
**	16-feb-1993 (rogerk)
**	    Added tcb_open_count field.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed spcb_lg_addr to spcb_lsn.  Removed xcb_abt_sp_addr
**	    and rcb_abt_sp_addr fields which were used with BI recovery.
**	    Removed obsolete rcb_bt_bi_logging and rcb_merge_logging fields.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**		bm_log_addr => bm_last_lsn.
**		Remove lctx_lps.
**		Trace the dcb_backup_lsn
**	24-apr-1993 (bryanp)
**	    Include <pc.h> before <lk.h>
**      21-jun-1993 (mikem)
**          su4_us5 port.  Added include of me.h to pick up new macro
**          definitions and proper prototypes for MEcopy, MEfill, ...
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Replace uses of DM_LKID with LK_LKID.
**	26-jul-1993 (rog)
**	    General exception handler cleanup.
**	07-oct-93 (johnst)
**	    Bug #56442
**	    Changed calls to dm0m_search() to reflect type change of argument 
**	    "arg" from (i4) to (i4 *), to accomodate 64-bit pointer
**	    arguments. On 64-bit DEC alpha, pointers and ints are not the same
**	    size and cannot be overloaded. Also changed the data type of 
**	    dmd_fmt_cb() arg "flag" from (i4) to (i4 *) to match
**	    the new dm0m_search() arg type required. As a result, changed the
**	    previous dmd_fmt_cb(1, ..) call-by-value to dmd_fmt_cb(&one, ..) 
**	    call-by-reference to match the new arg data type.
**	    Also changed a multitude of TRdisplay format specifications from
**	    %x to %p where they refer to pointer data types, to fix problems
**	    on 64-bit pointer platforms.
**	22-feb-1994 (andys)
**	    Integrate changes from ingres63p:
**		17-nov-1993 (rogerk)
**                  Added buf_tcb_stamp and buf_state information to
**		    dmd_check output.  Changed buffer manager
**		    statistics output to allow for bigger caches and
**		    not truncate output to 3 digits.
**             01-nov-1994 (Bin Li)
**         Fix bug 61055, trace point dm420 output is wrong, 'FAST COMMIT
**         FLUSHES' should be 'CONSISTENCY POINT FLUSHES'.
**	20-jun-1994 (jnash)
**	    Eliminate obsolete tcb_logical_logging field.
**	07-jun-1995 (cohmi01)
**	    Display prefetch stats, add timestamp to header.
**	26-jun-1995 (cohmi01)
**	    Update display of prefetch stats for new prefetch types.
**	27-dec-1995 (dougb)
**	    Extend change 418298 (by canor01) to include the VMS platforms.
**	    In particular, non-Unix platforms don't use the same semaphore
**	    structure.
**      06-mar-1996 (stial01)
**          Variable Page Size Project: Changes for new BM control blocks.
**          Also when printing TCB_CB, make sure !(tcb_status & TCB_BUSY)
**	25-mar-96 (nick)
**	    Added FCWAIT support and missing buf_sts from buffer debug dump.
**	23-apr-96 (canor01)
**	    Add validity checks in dmd_fmt_cb() on some control blocks
**	    before trying to dump their contents.
**	20-nov-96 (mcgem01)
**	    Back out change 424238 which has serious performance 
**	    implications.
**	24-nov-96 (mcgem01)
**	    Restore change 424238.
**	28-feb-97 (cohmi01)
**	    Display new HCB fields - hcb_tcbreclaim, hcb_tcblimit.
**	06-Jun-1996 (jenjo02)
**	    Initialize bucket_array and group_array based on cache_ix.
**	29-jul-1996 (wonst02 for shero03)
**	    Add RTREE to table types.
**	01-aug-96 (nanpr01)
**	    Changed BM_TRAN_MAX to BMCB_TRAN_MAX. Also added DMCM 
**	    buffer manager statuses.
**	13-Sep-1996 (jenjo02)
**	    BM mutex granularity project. Altered code here to fit
**	    changes in dm0pbmcb.h structures and mutexes.
**	24-Oct-1996 (jenjo02)
**	    Replaced hcb_mutex with hash_mutex, one for each hash entry.
**	17-Jan-1997 (jenjo02)
**	    Replaced svcb_mutex with svcb_dq_mutex (DCB queue) and
**	    svcb_sq_mutex (SCB queue).
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          Altered code in dmd_fmt_cb() to fit changes in SCB and SLCB.
**	14-Mar-1997 (jenjo02)
**	    Added G_RBUSY, G_WAIT bits to g_sts.
**	24-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Replaced BMCB_MAX_PRI with DB_MAX_TABLEPRI.
**	    Removed bmcb_tblpsize, bmcb_tblpindex, lbm_tblpriority.
**	    Removed BUF_MUSTLOCK status bit.
**	01-May-1997 (jenjo02)
**	    Added new stat for group waits.
**      14-oct-1997 (stial01)
**          dmd_fmt_cb() Replace slcb_iso_level with slcb_readlock.
**          Readlock may be set on a table basis, isolation level cannot.
**	21-Nov-1997 (jenjo02)
**	    Removed single (bm_fq) and group (bm_gfq) buffer fixed queues.
**      08-Dec-1997 (hanch04)
**          Use new la_block field when block number is needed
**          for support of logs > 2 gig
**	19-Jan-1998 (jenjo02)
**	    Transaction queue generalized to be used in other venues,
**	    some fields renamed for this.
**	26-Mar-1998 (jenjo02)
**	    Updated display of buffer status to reflect current mask bits.
**	    Added new stats for WriteBehind threads.
**	2-jul-98 (thaju02)
**	    dmd_fmt_cb() switch-case TCB_CB, changed in conditional if stmt
**	    logical && operator to bitwise & operator. Resulted in dm101
**	    dump falsely reporting tcb's as being TCB_BUSY.
**	11-Aug-1998 (jenjo02)
**	    To RCB, added rcb_reltups, rcb_relpages, rcb_tup_adds, rcb_page_adds,
**	    brought rcb_state up-to-date.
**	01-Sep-1998 (jenjo02)
**	  o Moved BUF_MUTEX waits out of bmcb_mwait, lbm_mwait, into DM0P_BSTAT where
**	    mutex waits by cache can be accounted for.
**      07-Dec-1998 (stial01)
**          Added kperleaf to distinguish from kperpage (keys per index page)
**	16-feb-1999 (nanpr01)
**	    Support for update mode lock.
**      22-Feb-1999 (bonro01)
**          Replace references to svcb_log_id with
**          dmf_svcb->svcb_lctx_ptr->lctx_lgid
**	20-Apr-1999 (jenjo02)
**	    buf_index now embedded in struct buf_id.bid_index;
**	16-aug-1999 (nanpr01)
**	    Implementation of SIR 92299. Enabaling journalling on a
**	    online checkpoint.
**	24-Aug-1999 (jenjo02)
**	    Added BMCB_FHDR, BMCB_FMAP page types, stat collection by
**	    page type.
**	23-Sep-99 (thaju02)
**	    Modified dmd_fmt_cb(). Added rcb isolation level diagnostics.
**      12-nov-1999 (stial01)
**          Changes to how btree key info is stored in tcb
**	10-Jan-2000 (jenjo02)
**	    Added stats by page type for groups, added group priority
**	    queues.
**      15-may-2000 (stial01)
**          Remove Smart Disk code
**      06-jul-2000 (stial01)
**          Changes to btree leaf/index klen in tcb (b102026)
**      01-may-2001 (stial01)
**          Init adf_ucollation
**	06-Jun-2003 (devjo01)
**	    Replace scs_avformat with scs_iformat, so trace output is
**	    not flagged in the log as an error.
**	15-Jan-2004 (schka24)
**	   Reflect HCB, TCB, RCB changes here.  Add XCCB display.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      30-jun-2005 (horda03) Bug 114498/INGSRV3299
**          New flag for TCB status - also now use the macro
**          TCB_FLAG_MEANING rather than putting the values here.
**	05-Aug-2005 (hanje04)
**	    Backout change 478041 as it should never have been X-integrated
**	    into main in the first place.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	13-Jul-2006 (kiria01) b112605
**	    Display seq_cache instead of redundant seq_version.
**      09-feb-2007 (stial01)
**          Display xccb_blob_temp_flags (new)
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**      13-apr-2010 (stial01)
**          Fix DB_ATTS TRdisplay's
**/


/*
**  Forward Reference to Functions.
*/
static VOID count_cb(
		i4		count[DM0M_MAX_TYPE+1][3],
		DM_OBJECT	*obj );

static STATUS ex_handler(
		EX_ARGS *ex_args );


/*{
** Name: dmd_dmp_pool	- Dump DMF control block pool.
**
** Description:
**      The routine dumps the DMF control block space to the trace output
**	in ascii or hex.
**
** Inputs:
**      flag                            DMD_HEX or DMD_ASCII.
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	06-nov-1985 (derek)
**          Created new for Jupiter.
**	28-feb-1989 (rogerk)
**	    Increased size of count array as a new control block was added.
**	07-jul-1993 (rog)
**	    Added call to scs_avformat() to dump session info to errlog.
**      06-mar-1996 (stial01)
**          Variable Page Size Project: Changes for new BM control blocks.
**	05-Mar-2002 (jenjo02)
**	    Added displays, counts of DML_SEQ for Sequence Generators.
**	06-Jun-2003 (devjo01)
**	    Replace scs_avformat with scs_iformat, so trace output is
**	    not flagged in the log as an error.
**	15-Aug-2005 (jenjo02)
**	    Bind DMF memory "types" to real values. Count and display
**	    them all. Because DML_SCBs are preallocated by SCF, they won't
**	    appear as objects in any pool, so manually display them 
**	    here.
*/
VOID
dmd_dmp_pool(
i4            flag)
{
    i4		    count[DM0M_MAX_TYPE+1][3];
    i4		    err_code;
    DB_STATUS		    status;
    i4		    one = 1;
    i4		    i;
    PTR		    cs_scb;
    DML_SCB	    *scb;
    DB_ERROR	    local_dberr;

    /* Dump session info. */
    CSget_scb((CS_SCB **)&cs_scb);
    (void)scs_iformat(cs_scb, 1, 0, 1);

    TRdisplay("\n\n%10* SUMMARY OF DMF CONTROL BLOCKS at %@.\n\n");
    TRdisplay("%10* Type  Tag     Count     Total Size\n\n");

    /*  Calculate a control block summary. */

    MEfill(sizeof(count), 0, (char *)count);
    status = dm0m_search((DML_SCB*)NULL, 0, count_cb, (i4 *)count, &local_dberr);
    for ( i = 1; i <= DM0M_MAX_TYPE; i++ )
    {
	if ( count[i][0] )
	    TRdisplay("%10* %4d %.4s %9d     %10d\n",
		    i, &count[i][2], count[i][0], count[i][1]);
    }

    TRdisplay("%10*    TOTALS %9d     %10d\n\n",
			count[0][0], count[0][1]);

    dmd_fmt_cb(&one, (DM_OBJECT *)dmf_svcb);

    /* Display all DML_SCBs */
    dm0s_mlock(&dmf_svcb->svcb_sq_mutex);
    for ( scb = dmf_svcb->svcb_s_next;
	  scb != (DML_SCB*)&dmf_svcb->svcb_s_next;
	  scb = scb->scb_q_next )
    {
	dmd_fmt_cb(&one, (DM_OBJECT*)scb);
    }
    dm0s_munlock(&dmf_svcb->svcb_sq_mutex);

    /*	Scan the pool dumping control blocks. */

    status = dm0m_search((DML_SCB*)NULL, 0, dmd_fmt_cb, &flag, &local_dberr);
    if (status != E_DB_OK)
	TRdisplay("Error searching the memory pool.");
}

/*{
** Name: count_cb	- Count control blocks and sizes.
**
** Description:
**      This routine is called with the array of counts and a 
**	control block which is increments the length and the number
**	of bytes allocated to control blocks of that type.
**
** Inputs:
**      count                           Array [20][2].
**      obj                             Object to count.
**
** Outputs:
**      count                           Updated for object.
**	Returns:
**	    E_DB_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	06-nov-1985 (derek)
**          Created new for Jupiter.
**	15-Aug-2005 (jenjo02)
**	    Stow object's tag for use as identifier.
*/
static VOID
count_cb(
i4            count[DM0M_MAX_TYPE+1][3],
DM_OBJECT	   *obj)
{
    i4		type = obj->obj_type & ~(DM0M_LONGTERM | DM0M_SHORTTERM);

    if ( type > 0 && type <= DM0M_MAX_TYPE )
    {
	/*  Increment count of objects and size used. */

	count[0][0]++;
	count[0][1] += obj->obj_size;

	/* Increment count and size by type, stuff "tag". */

	count[type][0]++;
	count[type][1] += obj->obj_size;
	count[type][2] = obj->obj_tag;
    }
}

/*{
** Name: dmd_fmt_cb	- Format a DMF control block.
**
** Description:
**      This routine will format a DMF control block in hex or
**	ascii depending on the flag argument passed by reference.
**
** Inputs:
**      flag_ptr                        ptr to DMD_HEX or DMD_ASCII flag.
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	06-nov-1985 (derek)
**          Created new for Jupiter.
**	28-feb-1989 (rogerk)
**	    When find lbmcb, format the entire buffer manager contents.
**	    Lock buffer manager while doing this.
**	 3-apr-1989 (rogerk)
**	    Fix semaphore bug when dumping buffer manager control block.
**	10-apr-1989 (rogerk)
**	    Format buffer manager name when dumping buffer manager.
**	12-jun-1989 (rogerk)
**	    Changed printing of mutex structures to not used fixed size
**	    structure.  Use sizeof(DM_MUTEX) to determine the number of bytes
**	    print as the size may change depending on the CL definitions.
**	12-jun-1990 (linda, bryanp)
**	    Added some options to the TRdisplay of access codes:
**	    <3>,<4>,OPEN_NOACC (items 3 & 4 are empty flags, OPEN_NOACC is
**	    a special access code for gateway tables).
**	14-feb-1991 (bryanp)
**	    Updates for Btree index compression project. Also, since I was here,
**	    I brought a number of out-of-date TRdisplays up to date.
**	19-aug-1991 (rogerk)
**	    Report new svcb_status values.  This was done as part of the
**	    bugfix for B39017.
**	15-oct-1991 (mikem)
**	    Removed TRdisplay of DI_FCB fields.  These fields can not be 
**	    referenced outside of the CL as they may differ from platform to
**	    platform.  Also eliminated compile warnings on UNIX by removing 
**	    unneeded "&" from in front of references to dcb_colname and
**	    db_own_name structure members which are both arrays.
**	3-feb-1992 (bryanp)
**	    Add TCB_PEND_DESTROY to tcb_status.
**	05-oct-1992 (rogerk)
**	    Reduced Logging Project.  Added support for partial TCB's
**	    which are not initialized with system catalog information.
**	    These TCB's are used only for IO operations.  The former
**	    tcb fields which carried information needed for IO operations
**	    have been moved into the tcb_table_io structure.
**	    Changed debug routine to reference new table_io fields and
**	    to only display system catalog information if it has been
**	    initialized.
**	26-oct-1992 (rogerk)
**	    Added new loc_config_id field to the location array.
**	    Added display of new tcb status fields.
**	04-nov-92 (jrb)
**	    Changed "SRT" to "WRK" for multi-location sorts project.
**	18-jan-93 (rogerk)
**	    Removed offline csp flag from dcb.
**	26-jul-1993 (bryanp)
**	    Replace uses of DM_LKID with LK_LKID.
**	31-jan-1994 (jnash)
**	    LSN's are displayed as (%d,%d).
**	22-feb-1994 (andys)
**	    Integrate changes from ingres63p:
**		17-nov-1993 (rogerk)
**                  Added buf_tcb_stamp and buf_state information to
**		    dmd_check output.  Changed buffer manager
**		    statistics output to allow for bigger caches and
**		    not truncate output to 3 digits.
**	    Change buf_sts TRdisplay to have WCOMMIT and BUF_MUTEX_OK.
**	20-jun-1994 (jnash)
**	    Eliminate obsolete tcb_logical_logging field.
**	07-jun-1995 (cohmi01)
**	    Format PFH (prefetch header) and prefetch event request blocks.
**	27-dec-1995 (dougb)
**	    Extend change 418298 (by canor01) to include the VMS platforms.
**	    In particular, non-Unix platforms don't use the same semaphore
**	    structure.
**      06-mar-1996 (stial01)
**          Variable Page Size Project: Changes for new BM control blocks.
**	25-mar-96 (nick)
**	    Added FCWAIT support and missing buf_sts from TRdisplay() of 
**	    the buffer header.
**	23-apr-96 (canor01)
**	    Add validity checks on some control blocks
**	    before trying to dump their contents.
**	06-Jun-1996 (jenjo02)
**	    Initialize bucket_array and group_array based on cache_ix.
**	13-Sep-1996 (jenjo02)
**	    BM mutex granularity project. Altered code here to fit
**	    changes in dm0pbmcb.h structures.
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          Altered code here to fit changes in SCB and SLCB.
**	14-Mar-1997 (jenjo02)
**	    Added G_RBUSY, G_WAIT bits to g_sts.
**	24-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Replaced BMCB_MAX_PRI with DB_MAX_TABLEPRI.
**	    Removed bmcb_tblpsize, bmcb_tblpindex, lbm_tblpriority, bmc_priority.
**	    Added new statistics for fixed priority buffers.
**	    Removed BUF_MUSTLOCK status bit.
**	01-May-1997 (jenjo02)
**	    Added new stat for group waits.
**      14-oct-1997 (stial01)
**          dmd_fmt_cb() Replace slcb_iso_level with slcb_readlock.
**          Readlock may be set on a table basis, isolation level cannot.
**	26-Mar-1998 (jenjo02)
**	    Updated display of buffer status to reflect current mask bits.
**	11-Aug-1998 (jenjo02)
**	    To RCB, added rcb_reltups, rcb_relpages, rcb_tup_adds, rcb_page_adds,
**	    brought rcb_state up-to-date.
**	01-Sep-1998 (jenjo02)
**	  o Moved BUF_MUTEX waits out of bmcb_mwait, lbm_mwait, into DM0P_BSTAT where
**	    mutex waits by cache can be accounted for.
**      12-Apr-1999 (stial01)
**          Distinguish leaf/index info for BTREE/RTREE indexes
**	24-Aug-1999 (jenjo02)
**	    Added BMCB_FHDR, BMCB_FMAP page types, stat collection by
**	    page type.
**	09-Apr-2001 (jenjo02)
**	    Added GatherWrite stats.
**	31-Jan-2003 (jenjo02)
**	    bs_stat name changes for Cache Flush Agents.
**	23-Dec-2003 (jenjo02)
**	    Added relstat2 to TCB display, partitioning bits and
**	    variables to TCB/RCB, display of RTB and RTL Agent list.
**	6-Feb-2004 (schka24)
**	    Remove dmu count from XCB.
**	01-Mar-2004 (jenjo02)
**	    Deleted mx_relation from MXCB.
**	12-Mar-2004 (jenjo02)
**	    Fixed bad TRdisplay of RTB.
**	18-Mar-2004 (jenjo02)
**	    Fixed bad length specs in TRdisplay of XCCB.
**	07-Apr-2004 (jenjo02)
**	    Added SI_VALUE_LOCKED state to rcb_si_flags.
**	12-Apr-2004 (jenjo02)
**	    Added scb_dop to DML_SCB display.
**	20-Apr-2004 (jenjo02)
**	    Updated scb_sess_mask bit interpretations.
**	12-May-2004 (schka24)
**	    Updated a few things in rcb, xccb, scb.
**	21-Jun-2004 (schka24)
**	    scb_lo_xxx gone, add xccb is-blob-temp flag.
**	10-Apr-2005 (schka24)
**	    Some displays in BUF: dump were too small for large caches.
**	15-Aug-2005 (jenjo02)
**	    Add dumping of SCB ShortTerm memory pool.
**	17-Aug-2005 (jenjo02)
**	    db_tab_index removed from DML_SLCB.
**	12-Dec-2005 (jenjo02)
**	    For "unknown" objects, display up to 2048 bytes
**	    to aid debugging.
**	20-Jan-2006 (kschendel)
**	    Update tbio flag display, group display.
**	13-Apr-2006 (jenjo02)
**	    Add CLUSTERED bit to relstat, replacing "VBASE"
**	11-Sep-2006 (jonj)
**	    Update xcb_flags.
**	12-Feb-2007 (jonj)
**	    Skipped time-consuming display of totally unused cache buffers,
**	    hex-display b_sts, correct erroneous bit interpretations.
**	27-Feb-2007 (jonj)
**	    Don't display empty hcb lists, add tcb_unique_id to TCB
**	    display, show x(tcb_status), update tcb_status bits.
**	22-Jun-2007 (jonj)
**	    Worthy FCB information being incorrectly displayed,
**	    or not at all.
**	    Display something about busy TCBs without getting too bold.
**	26-Sep-2007 (jonj)
**	    Add buf_partno, buf_pid, buf_lock_mode to BUFfer display.
**	    Updated stale dcb_status bits.
**	09-Oct-2007 (jonj)
**	    Add DCB_STATUS, DCB_LOC_FLAGS, RCB_STATE, RCB_SI_FLAGS,
**	    TBIO_CACHE_FLAGS, TBIO_STATUS, RELSTAT, RELSTAT2,
**	    LOC_STATUS defines to dmp.h
**	11-Oct-2007 (jonj)
**	    Add BUF_STS define to dm0pbmcb.h
**	25-Feb-2008 (kschendel) SIR 122739
**	    TCB changes for rowaccessor structure.
**	04-Apr-2008 (jonj)
**	    o Add BMCB_STATUS, BMCB_PAGE_TYPES, BMC_STS, BM_STATUS,
**	      RCB_UPDATE_MODE, RCB_ACCESS_MODE, RCB_LK_TYPE,
**	      RCB_K_DURATION, RCB_ISO_LEVEL, RCB_CSRR_FLAGS,
**	      SVCB_STATUS.
**	    o Display cache Table queues
**	    o Show DB id's as 0x, not decimal.
**	    o Show partitioned table's db_tab_index partition id
**	      in a more readable format than a signed negative
**	      number, "db_tab_base,-db_tab_index", also changed
**	      in LKkey_to_string.
**	    o Show each partition's TCB pointer pp_tcb when 
**	      deconstructing a partitioned TCB.
**      16-Apr-2008 (kschendel)
**          Minor addition to RCB (rowaddr *).
**	01-may-2008 (toumi01) Bug #119033
**	    Relabel GREADS summary (which is count of **IO events**) and
**	    GREADS FHDR, FMAP, ROOT, etc. which are counts of **pages**).
**	    Using the same labels makes it appear that the details should
**	    add up to the total, but they are apples and oranges.
**	    New literals are GREADIOS and GREADPGS. Ditto for GWRITES,
**	    which are now GWRITEIOS and GWRITEPGS.
**	30-May-2008 (jonj)
**	    LSNs are 2 u_i4's; display them in hex rather than
**	    signed decimal.
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	26-Nov-2008 (jonj)
**	    SIR 120874: rtb_err_code replaced with rtb_error.
**	4-Dec-2008 (kschendel) SIR 122739
**	    XCCB "delete" display broken, fix.
**	12-Nov-2009 (kschendel) SIR 122882
**	    Update relcmptlvl display.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC:
**	    Add new cache stats, display of CRIB.
**	10-Feb-2010 (jonj)
**	    SIR 121619 MVCC: Add bs_lreadio stat for physical log reads.
**	26-Feb-2010 (jonj)
**	    DMP_RCB anchors CRIB, display cursor CRIBs.
**	15-Mar-2010 (smeke01) b119529
**	    Replaced code for displaying DM0P_BSTAT structure by call
**	    to function dmd_summary_statistics(). Replaced %d with %u
**	    for DM0P_BSTAT fields.
**	16-Apr-2010 (kschendel) SIR 123485
**	    Add the BQCB.
**	29-Jun-2010 (jonj)
**	    Add buf_cr_undo, buf_cr_where to DM0P_BUFFER stuff.
**	05-Oct-2010 (jonj) B124549
**	    Add more stuff from the CRIB, show RCB's CRIB.
*/
static STATUS
ex_handler(
	EX_ARGS *ex_args )
{
    return (EXDECLARE);
}

VOID
dmd_fmt_cb(
i4             *flag_ptr,
DM_OBJECT	    *obj)
{
    DMP_BMSCB	    *bmseg;
    DM0P_BM_COMMON  *bm_common;
    DM0P_BMCB       *bm;
    DM0P_BSTAT      *bs;
    DMP_LBMCB	    *lbmcb;
    DMP_DCB	    *dcb;
    DMP_EXT	    *ext;
    DMP_FCB	    *fcb;
    DMP_HCB	    *hcb;
    DMP_MISC	    *misc;
    DM2U_MXCB	    *mxcb;
    DML_ODCB	    *odcb;
    DMP_RCB	    *rcb;
    DML_SCB	    *scb;
    DML_SLCB	    *slcb;
    DML_SPCB	    *spcb;
    DM_SVCB 	    *svcb;
    DMP_TCB	    *tcb;
    DML_XCB	    *xcb;
    DML_XCCB	    *xccb;
    DMP_LCTX	    *lctx;
    DMP_TABLE_IO    *tbio;
    DMP_LOCATION    *loc;
    DMP_PFH	    *pfh;
    DMP_PFREQ	    *pfr;
    DML_SEQ	    *seq;
    DML_CSEQ	    *cseq;
    DMR_RTB	    *rtb;
    DMR_RTL	    *rtl;
    DMPE_BQCB	    *bqcb;
    i4		    i;
    i4		    type, mode;
    i4		    already_locked;
    EX_CONTEXT	    context;
    i4	    buf_count;
    DM0P_BID        *next_trq;
    i4         next_buf;
    DM0P_BUFFER	    *b;
    DM0P_BUFFER     *buf_array;
    DM0P_GROUP	    *g;
    DM0P_GROUP      *group_array;
    DM0P_HASH_HEAD  *bucket_array;
    DM0P_BMCACHE    *c;
    i4         cache_ix;
    i4         page_size;
    i4		    fcount,mcount,lcount,gfcount,gmcount,glcount;
    i4		    skipped;
    i4		    err_code;
    DB_ERROR	    local_dberr;
    DMPP_PAGE	    *page;
    i4		    page_stat;
    LG_CRIB	    *crib;

    if (EXdeclare(ex_handler, &context))
    {
	TRdisplay("\n%4* EXCEPTION OCCURED WHILE DUMPING CONTROL BLOCK.\n");
	EXdelete();
	return;
    }

TRdisplay("\n%.4s Control Block @0x%p..0x%p owned by 0x%x for %d bytes.\n\n",
	    &obj->obj_tag, obj, (char *)obj + obj->obj_size - 1, 
            obj->obj_owner, obj->obj_size);

    if (*flag_ptr == 0)
    {
	for (i = 0; i < obj->obj_size; i+= 32)
	    TRdisplay("   0x%p:%< %8.4{%x %}%2< >%32.1{%c%}<\n",
		(char *)obj + i, 0);
	EXdelete();
	return;
    }

    switch (obj->obj_type)
    {
    case BMSCB_CB:
	bmseg = (DMP_BMSCB *)obj;
	TRdisplay("    BMSCB in Local Memory (0x%p), \n", bmseg);
	for (cache_ix = 0; cache_ix < DM_MAX_CACHE; cache_ix++)
	{
	    if (bmseg->bms_bmcb[cache_ix])
	    {
		bm = (DM0P_BMCB *)((char *)bmseg + bmseg->bms_bmcb[cache_ix]);
		DM0P_COUNT_ALL_QUEUES(fcount, mcount, lcount);
		DM0P_COUNT_ALL_GQUEUES(gfcount, gmcount, glcount);
		TRdisplay("    Page Size: %6d    Status: %v\n", 
			bm->bm_pgsize, "GROUP,FWAIT", bm->bm_status);
		TRdisplay("    Fixed buffers:    %8d, group: %8d\n",
			lcount, glcount);
		TRdisplay ("    Modified buffers: %8d, group: %8d\n",
			mcount, gmcount);
		TRdisplay ("    Free buffers:     %8d, group: %8d\n",
			fcount, gfcount);
	    }
	}
	break;

    case LBMCB_CB:
	lbmcb = (DMP_LBMCB *)obj;
	TRdisplay("    bm_common ptr: 0x%p  dbcache: 0x%p  tblcache: 0x%p\n",
	    lbmcb->lbm_bm_common, lbmcb->lbm_dbcache, lbmcb->lbm_tblcache);
	TRdisplay("    Lock list: 0x%x  Bm lock id: 0x%x\n",
		lbmcb->lbm_lock_list, lbmcb->lbm_bm_lockid);
	TRdisplay("    Statistics%64*-\n");
	TRdisplay("          Lock reclaim: %10d\n",
			lbmcb->lbm_lockreclaim);
	TRdisplay("          CP flushes: %10d\n",
			lbmcb->lbm_fcflush);

	/*
	** Buffer Manager control block.  Get pointer to BMSCBs to format buffer
	** manager.  Do this here instead of when find BMSCB since the BMSCB may
	** be in a shared segment and won't be found with the dump pool routine.
	** Verify the BMSCB control block before trying to use it.
	**
	** Lock the buffer manager mutex while dumping the information.
	** Note that this will stall other servers connected to this buffer
	** manager.  Ignore semaphore deadlock errors as this merely indicates
	** that we already had the semaphore locked.
	**
	** Dirty-read what we can get away with. Lock other structures.
	*/
	bm_common = (DM0P_BM_COMMON *)lbmcb->lbm_bm_common;
	if (MEcmp(bm_common->bmcb_name.db_name, 
		dmf_svcb->svcb_bmseg[0]->bms_name.db_name, DB_DB_MAXNAME))
	    break;

	TRdisplay("    Buffer Manager Name: '%t'\n",
	    sizeof(bm_common->bmcb_name.db_name), bm_common->bmcb_name.db_name);
	TRdisplay("    Buffer Manager id: %d  Connections: %d\n",
	    bm_common->bmcb_id, bm_common->bmcb_srv_count);
	TRdisplay("    Status: %v\n", 
	    BMCB_STATUS, bm_common->bmcb_status);
	TRdisplay("    CP count: %8d  CP index: %8d  CP check: %8d\n",
	    bm_common->bmcb_cpcount, bm_common->bmcb_cpindex, bm_common->bmcb_cpcheck);
	TRdisplay("    Database cache size: %4d  Table cache size: %4d  Waits: %d\n",
	    bm_common->bmcb_dbcsize, bm_common->bmcb_tblcsize,
	    bm_common->bmcb_bmcwait);
	TRdisplay("    Lock List Key: (0x%x, 0x%x)\n",
	    bm_common->bmcb_lock_key.lk_uhigh, bm_common->bmcb_lock_key.lk_ulow);
	TRdisplay("    Log Flush Addr (0x%x, 0x%x)\n",
	    bm_common->bmcb_last_lsn.lsn_high, bm_common->bmcb_last_lsn.lsn_low);

	TRdisplay("    Weighting constants%55*-\n");
	TRdisplay("%25* FHDR      FMAP      ROOT     INDEX      LEAF      DATA\n");
	TRdisplay("%8* REFERENCE  %6.4{%10d%}\n", bm_common->bmcb_reference, 0);
	TRdisplay("%8* REUSE      %6.4{%10d%}\n", bm_common->bmcb_rereference, 0);
	TRdisplay("%8* MAXIMUM    %6.4{%10d%}\n", bm_common->bmcb_maximum, 0);

	TRdisplay("    Transaction Queues%56*-\n");
	for (i = 0; i < BMCB_TRAN_MAX; i++)
	{
	    buf_count = bm_common->bmcb_totbufcnt;
	    already_locked = CSp_semaphore(1, 
		&bm_common->bmcb_txnq_buckets[i].blqh_mutex.bms_semaphore);
	    next_trq = &bm_common->bmcb_txnq_buckets[i].blqh_bid;
	    if (next_trq->bid_index != BUF_ENDLIST)
		TRdisplay("%8* Tran Bucket[%d]:\n", i);
	    while (next_trq->bid_index != BUF_ENDLIST)
	    { 
		buf_array = lbmcb->lbm_buffers[next_trq->bid_cache];
		b = &buf_array[next_trq->bid_index];
		TRdisplay(
		    "%12* BUF(0x%p)DB(0x%x)TBL(%d%s%d)PG(%d)PART(%d)PRI(%d)TYPE(%w)\n",
		    b, b->buf_lock_key.lk_key1, b->buf_lock_key.lk_key2,
		    (b->buf_lock_key.lk_key3 & DB_TAB_PARTITION_MASK)
			? ",-" : ",",
		    b->buf_lock_key.lk_key3 & ~DB_TAB_PARTITION_MASK,
		    b->buf_lock_key.lk_key4,
		    b->buf_partno,
		    b->buf_priority,
		    BMCB_PAGE_TYPES, b->buf_type);
		if (--buf_count < 0)
		{
		    TRdisplay("%12* BUFFER QUEUE INFINITE?.\n");
		    break;
		}
		next_trq = &b->buf_txnq.blq_next;
	    }
	    if (!already_locked) 
		CSv_semaphore(&bm_common->bmcb_txnq_buckets[i].blqh_mutex.bms_semaphore);
	}

	TRdisplay("    Table Queues%50*-\n");
	for (i = 0; i < BMCB_TABLE_MAX; i++)
	{
	    buf_count = bm_common->bmcb_totbufcnt;
	    already_locked = CSp_semaphore(1, 
		&bm_common->bmcb_tabq_buckets[i].blqh_mutex.bms_semaphore);
	    next_trq = &bm_common->bmcb_tabq_buckets[i].blqh_bid;
	    if (next_trq->bid_index != BUF_ENDLIST)
		TRdisplay("%8* Table Bucket[%d]:\n", i);
	    while (next_trq->bid_index != BUF_ENDLIST)
	    { 
		buf_array = lbmcb->lbm_buffers[next_trq->bid_cache];
		b = &buf_array[next_trq->bid_index];
		TRdisplay(
		    "%12* BUF(0x%p)DB(0x%x)TBL(%d%s%d)PG(%d)PART(%d)PRI(%d)TYPE(%w)\n",
		    b, b->buf_lock_key.lk_key1, b->buf_lock_key.lk_key2,
		    (b->buf_lock_key.lk_key3 & DB_TAB_PARTITION_MASK)
			? ",-" : ",",
		    b->buf_lock_key.lk_key3 & ~DB_TAB_PARTITION_MASK,
		    b->buf_lock_key.lk_key4,
		    b->buf_partno,
		    b->buf_priority,
		    BMCB_PAGE_TYPES, b->buf_type);
		if (--buf_count < 0)
		{
		    TRdisplay("%12* BUFFER QUEUE INFINITE?.\n");
		    break;
		}
		next_trq = &b->buf_tabq.blq_next;
	    }
	    if (!already_locked) 
		CSv_semaphore(&bm_common->bmcb_tabq_buckets[i].blqh_mutex.bms_semaphore);
	}

	TRdisplay("    Database Cache Entries%52*-\n");
	already_locked = CSp_semaphore(1, 
	    &bm_common->bmcb_dbclist.sth_mutex.bms_semaphore);
	next_buf = bm_common->bmcb_dbclist.sth_stq.stq_next;
	while (next_buf != BUF_ENDLIST)
	{
	    c = &lbmcb->lbm_dbcache[next_buf];
	    TRdisplay(
	    "%8* Status: %v  DBid: 0x%x  Tabid: %d  Refcount: %d\n",
		BMC_STS, c->bmc_sts, c->bmc_dbid, 
		c->bmc_tabid, c->bmc_refcount);
	    TRdisplay("%8* Lkid: 0x%x  Lkvalue: (0x%x, 0x%x)\n",
		c->bmc_lkid, c->bmc_lkvalue.lk_high, c->bmc_lkvalue.lk_low);
	    next_buf = c->bmc_state.stq_next;
	}
	if (!already_locked)
	    CSv_semaphore(&bm_common->bmcb_dbclist.sth_mutex.bms_semaphore);

	TRdisplay("    Table Cache Entries%55*-\n");
	already_locked = CSp_semaphore(1, 
	    &bm_common->bmcb_tblclist.sth_mutex.bms_semaphore);
	next_buf = bm_common->bmcb_tblclist.sth_stq.stq_next;
	while (next_buf != BUF_ENDLIST)
	{
	    c = &lbmcb->lbm_tblcache[next_buf];
	    TRdisplay(
	    "%8* Status: %v  DBid: 0x%x  Tabid: %d  Refcount: %d\n",
		BMC_STS, c->bmc_sts, c->bmc_dbid, 
		c->bmc_tabid, c->bmc_refcount);
	    TRdisplay("%8* Lkid: 0x%x  Lkvalue: (0x%x, 0x%x)\n",
		c->bmc_lkid, c->bmc_lkvalue.lk_high, c->bmc_lkvalue.lk_low);
	    next_buf = c->bmc_state.stq_next;
	}
	if (!already_locked)
	    CSv_semaphore(&bm_common->bmcb_tblclist.sth_mutex.bms_semaphore);

	for (cache_ix = 0; cache_ix < DM_MAX_CACHE; cache_ix++)
	{
	    bm = lbmcb->lbm_bmcb[cache_ix];

	    if (!bm || (bm->bm_status & BM_ALLOCATED) == 0)
		continue;

	    page_size = bm->bm_pgsize / 1024;
	    bs = &lbmcb->lbm_stats[cache_ix];
	    buf_array = lbmcb->lbm_buffers[cache_ix];
	    bucket_array = lbmcb->lbm_buckets[cache_ix];
	    group_array = lbmcb->lbm_groups[cache_ix];

	    DM0P_COUNT_ALL_QUEUES(fcount, mcount, lcount);
	    DM0P_COUNT_ALL_GQUEUES(gfcount, gmcount, glcount);

	    TRdisplay("    BUFFER CACHE (%2dK) %55*-\n", page_size);
	    TRdisplay("    Buckets:0x%p Pages:0x%p Buffers:0x%p Groups:0x%p\n",
		lbmcb->lbm_buckets[cache_ix], lbmcb->lbm_pages[cache_ix],  
		lbmcb->lbm_buffers[cache_ix], lbmcb->lbm_groups[cache_ix]);

	    TRdisplay("    Buffer Cache Configuration (%2dK) %41*-\n", 
		page_size);
	    TRdisplay("    Buffer count: %d (single: %d)  Bucket count: %d\n",
		bm->bm_bufcnt, bm->bm_sbufcnt, bm->bm_hshcnt);
	    TRdisplay("    Group count: %d  Pages count for a group: %d \n",
		bm->bm_gcnt, bm->bm_gpages);
	    TRdisplay(
	       "    Free count:  %d Limit: %d Modify count: %d Limit: %d\n",
		fcount, bm->bm_flimit, mcount, bm->bm_mlimit);
	    TRdisplay("    Free group count:  %d Modify group count: %d\n",
		gfcount, gmcount); 
	    TRdisplay("    Fixed count: %d  Group fixed count: %d\n",
		lcount, glcount);
	    if (bm->bm_status & BM_WB_IN_CACHE)
	    {
		TRdisplay(
		 "    WB start limit: %d, WB end limit: %d\n",
		    bm->bm_wbstart, bm->bm_wbend);
		TRdisplay(
		 "    WB flushes: %u, Agents cloned: %u\n",
		    bs->bs_cache_flushes, bs->bs_cfa_cloned);
		TRdisplay(
		 "    Agents active: %u, Agent hwm: %u\n",
		    bs->bs_cfa_active, bs->bs_cfa_hwm);
		TRdisplay(
		 "    WB pages flushed: %u, WB groups flushed: %u\n",
		    bs->bs_wb_flushed, bs->bs_wb_gflushed);
	    }

	    TRdisplay("    Clock: %d Status %v\n", 
		    bm->bm_clock,
		    BM_STATUS, bm->bm_status);

	    dmd_summary_statistics(bs, bm->bm_pgsize, 4);

	    TRdisplay("    Free queue (%2dK) %57*-\n", page_size); 
	    for (i = 0; i < DB_MAX_TABLEPRI; i++)
	    {
		buf_count = bm->bm_bufcnt;
		already_locked = CSp_semaphore(1, 
			&bm->bm_fpq[i].sth_mutex.bms_semaphore);
		next_buf = bm->bm_fpq[i].sth_stq.stq_next;
		if (next_buf != BUF_ENDLIST)
		    TRdisplay("%8* Priority[%1d]:\n", i);
		skipped = 0;
		while (next_buf != BUF_ENDLIST)
		{
		    b = &buf_array[next_buf];
		    if ( b->buf_lock_key.lk_key1 == 0 )
		      skipped++;
		    else
		      TRdisplay(
			"%12* PRI(%d)BUF(0x%p)DB(0x%x)TBL(%d%s%d)PG(%d)PART(%d)TYPE=%w\n",
			b->buf_priority, b, b->buf_lock_key.lk_key1,
			b->buf_lock_key.lk_key2, 
			(b->buf_lock_key.lk_key3 & DB_TAB_PARTITION_MASK)
			    ? ",-" : ",",
			b->buf_lock_key.lk_key3 & ~DB_TAB_PARTITION_MASK,
			b->buf_lock_key.lk_key4,
			b->buf_partno,
			BMCB_PAGE_TYPES, b->buf_type);
		    if (--buf_count < 0)
		    {
			TRdisplay("%12* BUFFER QUEUE INFINITE?.\n");
			break;
		    }
		    next_buf = b->buf_state.stq_next;
		}
		if ( skipped )
		    TRdisplay("%12* %d unused buffers not shown\n",
				skipped);
		
		if (!already_locked)
		    CSv_semaphore(&bm->bm_fpq[i].sth_mutex.bms_semaphore);
		TRdisplay("\n");
	    }

	    TRdisplay("    Modify queue (%2dK) %55*-\n", page_size); 
	    for (i = 0; i < DB_MAX_TABLEPRI; i++)
	    {
		buf_count = bm->bm_bufcnt;
		already_locked = CSp_semaphore(1, 
			&bm->bm_mpq[i].sth_mutex.bms_semaphore);
		next_buf = bm->bm_mpq[i].sth_stq.stq_next;
		if (next_buf != BUF_ENDLIST)
		    TRdisplay("%8* Priority[%1d]:\n", i);
		skipped = 0;
		while (next_buf != BUF_ENDLIST)
		{
		    b = &buf_array[next_buf];
		    if ( b->buf_lock_key.lk_key1 == 0 )
		      skipped++;
		    else
		      TRdisplay(
			"%12* PRI(%d)BUF(0x%p)DB(0x%x)TBL(%d%s%d)PG(%d)PART(%d)TYPE=%w\n",
			b->buf_priority, b, b->buf_lock_key.lk_key1,
			b->buf_lock_key.lk_key2,
			(b->buf_lock_key.lk_key3 & DB_TAB_PARTITION_MASK)
			    ? ",-" : ",",
			b->buf_lock_key.lk_key3 & ~DB_TAB_PARTITION_MASK,
			b->buf_lock_key.lk_key4,
			b->buf_partno,
			BMCB_PAGE_TYPES, b->buf_type);
		    if (--buf_count < 0) 
		    {
			TRdisplay("%12* BUFFER QUEUE INFINITE?.\n");
			break;
		    }
		    next_buf = b->buf_state.stq_next;
		}
		if ( skipped )
		    TRdisplay("%12* %d unused buffers not shown\n",
				skipped);
		if (!already_locked)
		    CSv_semaphore(&bm->bm_mpq[i].sth_mutex.bms_semaphore);
	    }

	    TRdisplay("    Hash Queues (%2dK) %56*-\n", page_size); 
	    for (i = 0; i < bm->bm_hshcnt; i++)
	    {
		buf_count = bm->bm_bufcnt;
		already_locked = CSp_semaphore(1, 
			&bucket_array[i].hash_mutex.bms_semaphore);
		next_buf = bucket_array[i].hash_next;
		if (next_buf != BUF_ENDLIST) 
		    TRdisplay("%8* Bucket[%d]:\n", i);
		while (next_buf != BUF_ENDLIST)
		{ 
		    b = &buf_array[next_buf];
		    TRdisplay(
			"%12* BUF(0x%p)DB(0x%x)TBL(%d%s%d)PG(%d)PART(%d)PRI(%d)TYPE=%w\n",
			b, b->buf_lock_key.lk_key1, b->buf_lock_key.lk_key2,
			(b->buf_lock_key.lk_key3 & DB_TAB_PARTITION_MASK)
			    ? ",-" : ",",
			b->buf_lock_key.lk_key3 & ~DB_TAB_PARTITION_MASK,
			b->buf_lock_key.lk_key4,
			b->buf_partno,
			b->buf_priority, 
			BMCB_PAGE_TYPES, b->buf_type);
		    if (--buf_count < 0)
		    {
			TRdisplay("%12* BUFFER QUEUE INFINITE?.\n");
			break;
		    }
		    next_buf = b->buf_next;
		}
		if (!already_locked)
		    CSv_semaphore(&bucket_array[i].hash_mutex.bms_semaphore);
	    }	    

	    TRdisplay("    Fixed Buffers (%2dK) %56*-\n", page_size); 
	    for (i = 0; i < bm->bm_bufcnt; i++)
	    {
		b = &buf_array[i];
		if (b->buf_sts & BUF_FIXED)
		{
		    TRdisplay(
			"%12* BUF(0x%p)DB(0x%x)TBL(%d%s%d)PG(%d)PART(%d)PRI(%d)TYPE=%w\n",
			b, b->buf_lock_key.lk_key1, b->buf_lock_key.lk_key2,
			(b->buf_lock_key.lk_key3 & DB_TAB_PARTITION_MASK)
			    ? ",-" : ",",
			b->buf_lock_key.lk_key3 & ~DB_TAB_PARTITION_MASK,
			b->buf_lock_key.lk_key4,
			b->buf_partno,
			b->buf_priority,
			BMCB_PAGE_TYPES, b->buf_type);
		}
	    }

	    if (bm->bm_status & BM_GROUP)
	    {
		TRdisplay("    FREE Group Queues (%2dK) %51*-\n",page_size); 
		for ( i = 0; i < DB_MAX_TABLEPRI; i++ )
		{
		    already_locked = CSp_semaphore(1, 
			    &bm->bm_gfq[i].sth_mutex.bms_semaphore);
		    next_buf = bm->bm_gfq[i].sth_stq.stq_next;
		    if (next_buf != BUF_ENDLIST)
			TRdisplay("%8* Priority[%1d]:\n", i);
		    skipped = 0;
		    while (next_buf != BUF_ENDLIST)
		    {
			g = &group_array[next_buf];
			if ( g->g_sts == 0 && g->g_valcount == 0 && g->g_refcount == 0 &&
			     g->g_modcount == 0 && g->g_pages == 0 )
			{
			    skipped++;
			}
			else
			    TRdisplay(
			    "%8* Gr: %d  Status: %v  Val_cnt: %d  Ref_cnt: %d  Mod_cnt: %d  Pages: %d  Buffer: %d\n",
				g->g_index,
				G_STS, g->g_sts,
				g->g_valcount, g->g_refcount, g->g_modcount, g->g_pages, g->g_buffer);
			next_buf = g->g_state.stq_next;
		    }
		    if ( skipped )
			TRdisplay("%8* %d unused groups not shown\n",
				    skipped);
		    if (!already_locked)
			CSv_semaphore(&bm->bm_gfq[i].sth_mutex.bms_semaphore);
		}

		TRdisplay("    FIXED Groups (%2dK) %50*-\n", page_size);
		for (i = 0; i < bm->bm_gcnt; i++)
		{
		    g = &group_array[i];
		    if (g->g_sts & G_FIXED)
		    {
			TRdisplay(
			"%8* Gr: %d  Status: %v  Val_cnt: %d  Ref_cnt: %d  Mod_cnt: %d  Pages: %d  Buffer: %d\n",
			    g->g_index,
			    G_STS, g->g_sts,
			    g->g_valcount, g->g_refcount, g->g_modcount, g->g_pages, g->g_buffer);
		    }
		}

		TRdisplay("    MODIFIED Group Queue (%2dK) %47*-\n", 
			    page_size);
		for ( i = 0; i < DB_MAX_TABLEPRI; i++ )
		{
		    already_locked = CSp_semaphore(1, 
			    &bm->bm_gmq[i].sth_mutex.bms_semaphore);
		    next_buf = bm->bm_gmq[i].sth_stq.stq_next;
		    if (next_buf != BUF_ENDLIST)
			TRdisplay("%8* Priority[%1d]:\n", i);
		    while (next_buf != BUF_ENDLIST)
		    {
			g = &group_array[next_buf];
			TRdisplay(
			"%8* Gr: %d  Status: %v  Val_cnt: %d  Ref_cnt: %d  Mod_cnt: %d  Pages %d  Buffer: %d\n",
			    g->g_index,
			    G_STS, g->g_sts,
			    g->g_valcount, g->g_refcount, g->g_modcount, g->g_pages, g->g_buffer);
			next_buf = g->g_state.stq_next;
		    }
		    if (!already_locked)
			CSv_semaphore(&bm->bm_gmq[i].sth_mutex.bms_semaphore);
		}
	    }

	    TRdisplay("    Buffer Contents (%2dK) %52*-\n",page_size); 
	    for (i = 0; i < bm->bm_bufcnt; i++)
	    {
		b = &buf_array[i];
		if ( b->buf_dbid == 0 )
		    continue;
		TRdisplay(
		"    BUF: 0x%p (%d) OWNER: %6d DB: 0x%x TAB: (%d%s%d) PG: %d PART: %d\n",
		    b, b->buf_id.bid_index, b->buf_owner, b->buf_dbid,
		    b->buf_tabid.db_tab_base, 
		    (b->buf_tabid.db_tab_index & DB_TAB_PARTITION_MASK)
			? ",-" : ",",
		    b->buf_tabid.db_tab_index & ~DB_TAB_PARTITION_MASK,
		    b->buf_lock_key.lk_key4,
		    b->buf_partno);
		TRdisplay(
		    "%8* REF=%2d CPCNT=%5d PRI=%2d TYPE=%w HASHB=%d TABB=%d\n",
		    b->buf_refcount, b->buf_cpcount, b->buf_priority,
		    BMCB_PAGE_TYPES, b->buf_type,
		    b->buf_hash_bucket, 
		    b->buf_tabq_bucket);
		TRdisplay("%8* NEXT=0x%x PREV=0x%x VALID_STAMP=%d\n",
		    b->buf_state.stq_next, b->buf_state.stq_prev, 
		    b->buf_tcb_stamp);
		TRdisplay("%8* CRNEXT=%d CRPREV=%d CRROOT=%d CR_ISO=%d\n",
		    b->buf_cr_next, b->buf_cr_prev, b->buf_cr_root, b->buf_cr_iso);
		TRdisplay("%8* CR_UNDO=%d CR_NOUNDO=%d CR_WHERE=%d\n",
		    b->buf_cr_undo, b->buf_cr_noundo, b->buf_cr_where );
		TRdisplay("%8* PINCOUNT=%d MPEND=%d\n",
		    b->buf_pincount, b->buf_mpend);
		TRdisplay("%8* TRANB=%5d TRAN=0x%x%x LKID=%x/%x V=%x%x\n",
		    b->buf_txnq_bucket, b->buf_tran_id.db_high_tran,
		    b->buf_tran_id.db_low_tran, 
		    b->buf_lk_id.lk_unique, b->buf_lk_id.lk_common,
		    b->buf_oldvalue.lk_high, b->buf_oldvalue.lk_low);
		TRdisplay("%8* PID=0x%x LKMODE=%w LGID=%x LKLIST=%x\n",
		    b->buf_pid, 
		    LOCK_MODE, b->buf_lock_mode,
		    b->buf_log_id,
		    b->buf_lock_list);
		TRdisplay("%8* STATUS (0x%x) %v\n",
		    b->buf_sts,
		    BUF_STS, b->buf_sts);
		page = (DMPP_PAGE*)((char *)lbmcb->lbm_pages[cache_ix] +
				b->buf_page_offset);
		page_stat = DMPP_VPT_GET_PAGE_STAT_MACRO(b->buf_relpgtype, page);
		TRdisplay("%8* PAGE: 0x%p PAGE STATUS (0x%x) %v\n",
		    page,
		    page_stat,
		    PAGE_STAT, page_stat);
		TRdisplay("%8* LSN=(%x,%x) CLSN=(%x,%x) BLSN=(%x,%x)\n",
		    b->buf_page_lsn.lsn_high, 
		    b->buf_page_lsn.lsn_low,
		    b->buf_cr_clsn.lsn_high, 
		    b->buf_cr_clsn.lsn_low,
		    b->buf_cr_blsn.lsn_high, 
		    b->buf_cr_blsn.lsn_low);
		TRdisplay("%8* TRAN=0x%x%x\n",
		    b->buf_page_tranid.db_high_tran, 
		    b->buf_page_tranid.db_low_tran); 
	    }
	}

	break;

    case DCB_CB:
	dcb = (DMP_DCB *)obj;
	TRdisplay("    Database (%~t,%~t)  Type: %w  Status: (0x%x) %v\n",
	    sizeof(dcb->dcb_name), &dcb->dcb_name,
	    sizeof(dcb->dcb_db_owner), &dcb->dcb_db_owner,
	    ",PRIVATE,PUBLIC,DISTRIBUTED", dcb->dcb_db_type,
	    dcb->dcb_status,
	    DCB_STATUS, dcb->dcb_status);
	TRdisplay("    Identifier: 0x%x  Access mode: %w  Served: %w  BM: %w\n",
	    dcb->dcb_id, ",READ,WRITE", dcb->dcb_access_mode,
	    ",MULTIPLE,SINGLE", dcb->dcb_served,
	    ",MULTIPLE,SINGLE", dcb->dcb_bm_served);
	TRdisplay("    Reference Count: %4d  Open Count: %4d Valid Stamp: 0x%x\n",
	    dcb->dcb_ref_count, dcb->dcb_opn_count, dcb->dcb_valid_stamp);
	TRdisplay("    Relation  TCB: 0x%p    Attribute TCB: 0x%p    Index     TCB: 0x%p\n",
	    dcb->dcb_rel_tcb_ptr, dcb->dcb_att_tcb_ptr, dcb->dcb_idx_tcb_ptr);
	TRdisplay("    Lock id: %x%x  Temp lock id: %x%x  Open lock id: %x%x\n",
	    dcb->dcb_lock_id[0], dcb->dcb_lock_id[1],
	    dcb->dcb_tti_lock_id[0], dcb->dcb_tti_lock_id[1],
	    dcb->dcb_odb_lock_id[0], dcb->dcb_odb_lock_id[1]);
	TRdisplay("    Log id: %x  Mutex: %#.4{%x %}\n", dcb->dcb_log_id,
	    (sizeof(DM_MUTEX) / 4), &dcb->dcb_mutex, 0);
	TRdisplay("    Location: %~t  Physical : %t\n",
	    sizeof(dcb->dcb_location.logical), &dcb->dcb_location.logical,
	    dcb->dcb_location.phys_length, &dcb->dcb_location.physical);
	TRdisplay("	Online Backup Starting Log Addr: (0x%x,0x%x,0x%x)\n",
	    dcb->dcb_backup_addr.la_sequence,dcb->dcb_backup_addr.la_block,
	    dcb->dcb_backup_addr.la_offset);
	TRdisplay("	Online Backup Starting Log Sequence Number: (%x,%x)\n",
	    dcb->dcb_backup_lsn.lsn_high, dcb->dcb_backup_lsn.lsn_low);
	TRdisplay("	Sync Flag: %v\n","NOSYNC",dcb->dcb_sync_flag);
	TRdisplay("	Name Lock Id: (%x,%x), Owner Lock Id: (%x,%x)\n",
	    dcb->dcb_nam_id.lk_unique, dcb->dcb_nam_id.lk_common,
	    dcb->dcb_own_id.lk_unique, dcb->dcb_own_id.lk_common);
	TRdisplay("	Collation ptr: 0x%p, Collation name %~t\n",
	    dcb->dcb_collation, sizeof(dcb->dcb_colname), dcb->dcb_colname);
	TRdisplay("	Timezone: 0x%p \n", dcb->dcb_tzcb);
	/* Make sure there is a ROOT location */
	if ( dcb->dcb_root != NULL )
	{
	    TRdisplay("	Root Loc: %~t, Root Phys: %~t, Flags %v\n",
		sizeof(dcb->dcb_root->logical), &dcb->dcb_root->logical,
		dcb->dcb_root->phys_length, &dcb->dcb_root->physical,
		DCB_LOC_FLAGS, dcb->dcb_root->flags);
	}
	/* Make sure there is a JNL location */
	if ( dcb->dcb_jnl != NULL )
	{
	    TRdisplay("	Jnl Loc: %~t, Jnl Phys: %~t, Flags %v\n",
		sizeof(dcb->dcb_jnl->logical), &dcb->dcb_jnl->logical,
		dcb->dcb_jnl->phys_length, &dcb->dcb_jnl->physical,
		DCB_LOC_FLAGS, dcb->dcb_jnl->flags);
	}
	/* Make sure there is a CKP location */
	if ( dcb->dcb_ckp != NULL )
	{
	    TRdisplay("	Ckp Loc: %~t, Ckp Phys: %~t, Flags %v\n",
		sizeof(dcb->dcb_ckp->logical), &dcb->dcb_ckp->logical,
		dcb->dcb_ckp->phys_length, &dcb->dcb_ckp->physical,
		DCB_LOC_FLAGS, dcb->dcb_ckp->flags);
	}
	/* Make sure there is a DMP location */
	if ( dcb->dcb_dmp != ( DMP_LOC_ENTRY *) NULL )
	{
		TRdisplay("	Dmp Loc: %~t, Dmp Phys: %~t, Flags %v\n",
	    	    sizeof(dcb->dcb_dmp->logical), &dcb->dcb_dmp->logical,
	    	    dcb->dcb_dmp->phys_length, &dcb->dcb_dmp->physical,
		    DCB_LOC_FLAGS, dcb->dcb_dmp->flags);
	}

	/* Display  extents */
	for ( i = 0; dcb->dcb_ext != NULL && i < dcb->dcb_ext->ext_count; i++)
	{
	   if ( ((dcb->dcb_ext->ext_entry[i].flags & DCB_ROOT       )!=0) &&
		((dcb->dcb_ext->ext_entry[i].flags & DCB_JOURNAL    )!=0) &&
		((dcb->dcb_ext->ext_entry[i].flags & DCB_CHECKPOINT )!=0) &&
		((dcb->dcb_ext->ext_entry[i].flags & DCB_DUMP       )!=0) 
	      )
	   {
		TRdisplay("	Loc: %~t, Phys: %~t, Flags %v\n",
	    	    sizeof(dcb->dcb_ext->ext_entry[i].logical), 
		    &dcb->dcb_ext->ext_entry[i].logical,
	    	    dcb->dcb_ext->ext_entry[i].phys_length, 
		    &dcb->dcb_ext->ext_entry[i].physical,
		    DCB_LOC_FLAGS, dcb->dcb_ext->ext_entry[i].flags);

	   }
	}

	/* Display Sequences */
	if ( seq = dcb->dcb_seq )
	{
	    do TRdisplay("     Sequence %p Name: %~t Owner: %~t Id: (%d,%d)\n",
		    seq, 
		    sizeof(seq->seq_name.db_name), seq->seq_name.db_name,
		    sizeof(seq->seq_owner.db_own_name), seq->seq_owner.db_own_name,
		    seq->seq_id, seq->seq_cache);
	    while ( seq = seq->seq_q_next );
	}
	break;

    case EXT_CB:  
	ext = (DMP_EXT *)obj;
	TRdisplay("%8* Logical %16* Flags %10* Physical\n%8* %80*-\n");
	TRdisplay("%#.#{%8* %.#s  %16v %.*s\n%}",
	    ext->ext_count, sizeof(DMP_LOC_ENTRY), ext->ext_entry,
	    sizeof(DB_LOC_NAME), &(((DMP_LOC_ENTRY *)0)->logical),
	    "ROOT,DATA,JNL,CKP,ALI,WRK,DMP", &(((DMP_LOC_ENTRY *)0)->flags),
	    &(((DMP_LOC_ENTRY *)0)->phys_length),
	    &(((DMP_LOC_ENTRY *)0)->physical));
	break;

    case FCB_CB:
	fcb = (DMP_FCB *)obj;
	TRdisplay("    TBIO: 0x%p  Location: %~t  Last Page: %d  State: (0x%x) %v\n",
	    fcb->fcb_tcb_ptr, sizeof(fcb->fcb_location->logical),
	    &fcb->fcb_location->logical, fcb->fcb_last_page,
	    fcb->fcb_state, FCB_STATES, fcb->fcb_state);
	TRdisplay("    Deferred Alloc: %d  Phys Alloc: %d  Logical alloc: %d\n",
	    fcb->fcb_deferred_allocation,
	    fcb->fcb_physical_allocation,
	    fcb->fcb_logical_allocation);
	TRdisplay("    Rawstart: %d  Rawpages %d\n",
	    fcb->fcb_rawstart,
	    fcb->fcb_rawpages);
	TRdisplay("    Filename: %#t%t\n",
	    sizeof(fcb->fcb_location->physical),
	    fcb->fcb_location->phys_length, &fcb->fcb_location->physical,
	    sizeof(fcb->fcb_filename), &fcb->fcb_filename);
	break;

    case HCB_CB:
	hcb = (DMP_HCB *)obj;
	TRdisplay("    TQ Mutex: %#.4{%x %}\n", (sizeof(DM_MUTEX) / 4),
	    &hcb->hcb_tq_mutex, 0);
	TRdisplay("	HCB owner: %d\n",hcb->hcb_owner);
	TRdisplay("	TCB limit: %d   TCBs reclaimed: %d\n", 
	    hcb->hcb_tcblimit, hcb->hcb_tcbreclaim);
	TRdisplay("	TCB's releasable - Next: 0x%p Previous: 0x%p\n",
		hcb->hcb_ftcb_list.q_next, hcb->hcb_ftcb_list.q_prev);
	TRdisplay("    Hash Buckets: %d\n", HCB_HASH_ARRAY_SIZE);
	TRdisplay("        Address     Next        Previous\n");
	/* Don't display empty lists */
	{
	    skipped = 0;

	    for ( i = 0; i < HCB_HASH_ARRAY_SIZE; i++ )
	    {
		if ( hcb->hcb_hash_array[i].hash_q_next ==
			(DMP_TCB*)&hcb->hcb_hash_array[i].hash_q_next &&
		     hcb->hcb_hash_array[i].hash_q_prev ==
			(DMP_TCB*)&hcb->hcb_hash_array[i].hash_q_next )
		{
		    skipped++;
		}
		else 
		{
		    if ( skipped )
			TRdisplay("%8* ...%d empty buckets not shown\n",
					skipped);
		    skipped = 0;
		    TRdisplay("%8* 0x%p  0x%p  0x%p\n",
				&hcb->hcb_hash_array[i],
				hcb->hcb_hash_array[i].hash_q_next,
				hcb->hcb_hash_array[i].hash_q_prev);
		}
	    }
	    if ( skipped )
		TRdisplay("%8* ...%d empty buckets not shown\n", skipped);
	}
	break;

    case LCTX_CB:
	lctx = (DMP_LCTX *)obj;
	TRdisplay("    Status: %v  Log id: %x  Lock id: %x\n",
	    "NOLOG,CLUSTER", lctx->lctx_status, 
            lctx->lctx_lgid, lctx->lctx_lxid);
	TRdisplay("    Area: 0x%p  Length: %d  Block_size: %d\n",
	    lctx->lctx_area, lctx->lctx_l_area, lctx->lctx_bksz);
	TRdisplay("    Mutex: %#.4{%x %}\n", (sizeof(DM_MUTEX) / 4),
	    &lctx->lctx_mutex, 0);
	TRdisplay("	LG log record header: 0x%p\n",lctx->lctx_header);
	break;

    case ODCB_CB:
	odcb = (DML_ODCB *)obj;
	TRdisplay("    Database Identifier: 0x%p  DCB: 0x%p  SCB: 0x%p  Owner: 0x%x\n",
	    odcb->odcb_db_id, odcb->odcb_dcb_ptr, odcb->odcb_scb_ptr,
	    odcb->odcb_owner);
	TRdisplay("    Access Mode: %w   Lock Mode: %w\n",
	    ",READ,WRITE", odcb->odcb_access_mode,
	    "N,IS,IX,S,SIX,U,X", odcb->odcb_lk_mode);
	TRdisplay("    XCCB Next: 0x%p   Previous: 0x%p\n",
	    odcb->odcb_cq_next, odcb->odcb_cq_prev);
	break;

    case MISC_CB:
	misc = (DMP_MISC *)obj;
	TRdisplay("    Misc data: 0x%p Length %d\n\n", misc->misc_data, misc->misc_length - sizeof(DMP_MISC));
	if (misc->misc_data && misc->misc_length > sizeof(DMP_MISC))
	  for (i = 0; i < misc->misc_length - sizeof(DMP_MISC); i+= 32)
	    TRdisplay("   0x%p:%< %8.4{%x %}%2< >%32.1{%c%}<\n",
		(char *)&misc->misc_data[i], 0);
	break;

    case MXCB_CB:
	mxcb = (DM2U_MXCB *)obj;
	TRdisplay("    Operation = %w DCB = 0x%p XCB = 0x%p RCB = 0x%p\n",
	    ",MODIFY,INDEX", mxcb->mx_operation, mxcb->mx_dcb, mxcb->mx_xcb, mxcb->mx_rcb);
	TRdisplay("    Compare List\n");
	TRdisplay("%#.#{%8* %4.2d  %8.2w  %4.2d %12.2w\n%}",
	    mxcb->mx_c_count, sizeof(DB_CMP_LIST), mxcb->mx_cmp_list, &((DB_CMP_LIST *)0)->cmp_offset,
	    ",,,DATE,,MONEY,,,,,DECIMAL,OBJECT_KEY,TABLE_KEY,,,,,,,,CHAR,VCHAR,,,,,,,,,INT,FLOAT,CHAR,,,,,TEXT",
	    &((DB_CMP_LIST *)0)->cmp_type,
	    &((DB_CMP_LIST *)0)->cmp_length, "ascending,descending", &((DB_CMP_LIST *)0)->cmp_direction);
        TRdisplay("    BASE Key Information\n");
TRdisplay("         ATTID  OFFSET  TYPE    LENGTH PREC KEY KOFFSET\n"); 
        TRdisplay("%#[%8* %6.4d  %4.2d  %8.2w  %4.2d %4.2d %3.2d    %4.2d\n%]",
	    mxcb->mx_ab_count, mxcb->mx_b_key_atts,
	    &((DB_ATTS *)0)->ordinal_id, 
	    &((DB_ATTS *)0)->offset, 
	    ",,,DATE,,MONEY,,,,,DECIMAL,OBJECT_KEY,TABLE_KEY,,,,,,,,CHAR,VCHAR,,,,,,,,,INT,FLOAT,CHAR,,,,,TEXT",
	    &((DB_ATTS *)0)->type,
	    &((DB_ATTS *)0)->length, &((DB_ATTS *)0)->precision, &((DB_ATTS *)0)->key,
	    &((DB_ATTS *)0)->key_offset); 
        TRdisplay("    INDEX Key Information\n");
TRdisplay("         ATTID  OFFSET  TYPE    LENGTH PREC KEY KOFFSET\n"); 
        TRdisplay("%#[%8* %6.4d  %4.2d  %8.2w  %4.2d %4.2d %3.2d    %4.2d\n%]",
	    mxcb->mx_ai_count, mxcb->mx_i_key_atts,
	    &((DB_ATTS *)0)->ordinal_id, 
	    &((DB_ATTS *)0)->offset, 
	    ",,,DATE,,MONEY,,,,,DECIMAL,OBJECT_KEY,TABLE_KEY,,,,,,,,CHAR,VCHAR,,,,,,,,,INT,FLOAT,CHAR,,,,,TEXT",
	    &((DB_ATTS *)0)->type,
	    &((DB_ATTS *)0)->length, &((DB_ATTS *)0)->precision, &((DB_ATTS *)0)->key,
	    &((DB_ATTS *)0)->key_offset); 
	TRdisplay("Index compression information: %w\n",
	    "UNCOMPRESSED,COMPRESSED", mxcb->mx_index_comp);
	TRdisplay("	Rwidth: %d  Kwidth: %d  Structure: %w  Compressed %w\n",
	    mxcb->mx_width, mxcb->mx_kwidth, 
	    "0,1,2,HEAP,4,ISAM,6,HASH,8,9,10,BTREE,,RTREE",mxcb->mx_structure,
	    "NONE,STD,OLD,3,4,5,6,HIDATA",mxcb->mx_data_rac.compression_type);
	TRdisplay("	Unique: %w  Truncate: %w  Duplicates: %w\n",
	    "NONUNIQUE,UNIQUE", mxcb->mx_unique,
	    "NOTRUNCATE,TRUNCATE", mxcb->mx_truncate,
	    "NODUPLICATE,DUPLICATE", mxcb->mx_duplicates);
	TRdisplay("	Reorg %d\n",mxcb->mx_reorg);
	TRdisplay("	Fill Factor - Data: %d  Btree Index: %d  Btree Leaf: %d\n",
	    mxcb->mx_dfill, mxcb->mx_ifill, mxcb->mx_lfill);
	TRdisplay("	Maxpages: %d  Minpages %d  Reltups: %d\n",
	    mxcb->mx_maxpages, mxcb->mx_minpages, mxcb->mx_reltups);
	TRdisplay("	Allocation: 0x%x  Extend: 0x%x  NewRelstat: 0x%x  New tuple Count %d\n",
	    mxcb->mx_allocation, mxcb->mx_extend, mxcb->mx_newrelstat,
	    mxcb->mx_newtup_cnt);
	break;

    case RCB_CB:
	rcb = (DMP_RCB *)obj;
	TRdisplay("    TCB: 0x%p  Tran_id: 0x%x%x   Log Id: 0x%x  Lock Id: 0x%x\n",
	    rcb->rcb_tcb_ptr, rcb->rcb_tran_id.db_high_tran,
	    rcb->rcb_tran_id.db_low_tran, rcb->rcb_log_id, rcb->rcb_lk_id);
	TRdisplay("    Update Mode: %w   Access Mode: %w  Sequence: %d  State: %v\n",
	    RCB_UPDATE_MODE, rcb->rcb_update_mode,
	    RCB_ACCESS_MODE, rcb->rcb_access_mode,
	    rcb->rcb_seq_number,
	    RCB_STATE, rcb->rcb_state);
	TRdisplay("    Lock Type: %w Limit: %3d  Count: %3d  Timeout: %4d  Mode: %w\n",
	    RCB_LK_TYPE, rcb->rcb_lk_type, 
	    rcb->rcb_lk_limit, 
            rcb->rcb_lk_count,
	    rcb->rcb_timeout, 
	    LOCK_MODE, rcb->rcb_lk_mode);
	TRdisplay("    Initial Lock Type: %w    Lock Mode: %w  Duration: %v\n",
	    RCB_LK_TYPE, rcb->rcb_k_type, 
	    LOCK_MODE, rcb->rcb_k_mode,
	    RCB_K_DURATION, rcb->rcb_k_duration);
	TRdisplay("    Isolation Lvl: %w  CSRR Flags: %v  DMR opcode %d\n",
	    RCB_ISO_LEVEL, rcb->rcb_iso_level, 
	    RCB_CSRR_FLAGS, rcb->rcb_csrr_flags,
	    rcb->rcb_dmr_opcode);
	/* Display CRIB contents, if any */
	if ( crib = rcb->rcb_crib_ptr )
	{
	    TRdisplay("    CRIB: 0x%p next: 0x%p prev: 0x%p\n",
	    	crib, crib->crib_next, crib->crib_prev);
	    TRdisplay("          Low LSN:(%x,%x) Last Commit:(%x,%x) Bos LSN:(%x,%x)\n",
		crib->crib_low_lsn.lsn_high,
		crib->crib_low_lsn.lsn_low,
		crib->crib_last_commit.lsn_high,
		crib->crib_last_commit.lsn_low,
		crib->crib_bos_lsn.lsn_high,
		crib->crib_bos_lsn.lsn_low);
	    TRdisplay("          Bos Tranid: 0x%x Sequence: %d Xid Array: 0x%p\n",
	        crib->crib_bos_tranid,
		crib->crib_sequence,
		crib->crib_xid_array);
	    TRdisplay("          Cursid: 0x%p Inuse: %d RCB State: %v\n",
	        crib->crib_cursid,
		crib->crib_inuse,
		RCB_STATE, crib->crib_rcb_state);
	    TRdisplay("          Active Transactions: %d...%d\n",
	        crib->crib_lgid_low,
		crib->crib_lgid_high);
	    for ( i = crib->crib_lgid_low; i <= crib->crib_lgid_high; i++ )
	    {
	        if ( crib->crib_xid_array[i] )
		    TRdisplay("          %4d: 0x%x\n",
		        i, crib->crib_xid_array[i]);
	    }
	}
	TRdisplay("    Partition %d  Rcb_reltid:",
	    rcb->rcb_partno);
	if ( rcb->rcb_reltid )
	    TRdisplay(" (%d%s%d)\n",
		rcb->rcb_reltid->db_tab_base,
		(rcb->rcb_reltid->db_tab_index & DB_TAB_PARTITION_MASK)
		    ? ",-" : ",", 
		rcb->rcb_reltid->db_tab_index & ~DB_TAB_PARTITION_MASK);
	else
	    TRdisplay(" (0,0)\n");
	TRdisplay("    Locked data: %d  Data lock id (uniq,comm): (%d,%d)\n",
	    rcb->rcb_locked_data, rcb->rcb_data_lkid.lk_unique,
	    rcb->rcb_data_lkid.lk_common);
	TRdisplay("    Locked leaf: %d  Leaf lock id (uniq,comm): (%d,%d)\n",
	    rcb->rcb_locked_leaf, rcb->rcb_leaf_lkid.lk_unique,
	    rcb->rcb_leaf_lkid.lk_common);
	TRdisplay("    Locked tid (page,line): (%d,%d) Reltid: (%d%s%d)\n",
	    rcb->rcb_locked_tid.tid_tid.tid_page, 
	    rcb->rcb_locked_tid.tid_tid.tid_line,
	    rcb->rcb_locked_tid_reltid.db_tab_base,
	    (rcb->rcb_locked_tid_reltid.db_tab_index & DB_TAB_PARTITION_MASK)
		? ",-" : ",",
	    rcb->rcb_locked_tid_reltid.db_tab_index & ~DB_TAB_PARTITION_MASK);
	TRdisplay("    Row lock id: (%d,%d)\n",
	    rcb->rcb_row_lkid.lk_unique, rcb->rcb_row_lkid.lk_common);
	TRdisplay("    Base lock id: (%d,%d)\n",
	    rcb->rcb_base_lkid.lk_unique, rcb->rcb_base_lkid.lk_common);
        TRdisplay("    Crow tid: (%d,%d) Mvcc lock id: (%d,%d)\n",
	    rcb->rcb_crow_tid.tid_tid.tid_page,
	    rcb->rcb_crow_tid.tid_tid.tid_line,
	    rcb->rcb_mvcc_lkid.lk_unique,
	    rcb->rcb_mvcc_lkid.lk_common);
	TRdisplay("    Next RCB: 0x%p  Previous: 0x%p  Related RCB: 0x%p\n",
	    rcb->rcb_xq_next, rcb->rcb_xq_prev, rcb->rcb_rl_next);
	TRdisplay("    Data page:  0x%p  CR Page: 0x%p Root page: 0x%p Pincount: %d\n",
	    rcb->rcb_data.page, rcb->rcb_data.CRpage, rcb->rcb_data.Rootpage, 
	    rcb->rcb_data.pincount);
	TRdisplay("    Other page: 0x%p  CR Page: 0x%p Root page: 0x%p Pincount: %d\n",
	    rcb->rcb_other.page, rcb->rcb_other.CRpage, rcb->rcb_other.Rootpage,
	    rcb->rcb_other.pincount);
	TRdisplay("    Savepoint Id: %d\n", rcb->rcb_sp_id);
	TRdisplay("    Interrupt Pointer: 0x%p  Default: %d\n", 
	    rcb->rcb_uiptr, rcb->rcb_uidefault);
	TRdisplay("    Logging: %w  Journal: %w\n",
	    "NO,YES", rcb->rcb_logging,
	    "NO,YES", is_journal(rcb));
	TRdisplay("    Low {%d,%d} Current {%d,%d} High {%d,%d}\n",
	    rcb->rcb_lowtid.tid_tid.tid_page, rcb->rcb_lowtid.tid_tid.tid_line,
	    rcb->rcb_currenttid.tid_tid.tid_page, 
            rcb->rcb_currenttid.tid_tid.tid_line,
	    rcb->rcb_hightid.tid_tid.tid_page, 
            rcb->rcb_hightid.tid_tid.tid_line);
	TRdisplay("    Fetch {%d,%d} Reposition {%d,%d}\n", 
	    rcb->rcb_fetchtid.tid_tid.tid_page, 
            rcb->rcb_fetchtid.tid_tid.tid_line,
	    rcb->rcb_p_lowtid.tid_tid.tid_page, 
            rcb->rcb_p_lowtid.tid_tid.tid_line);
	TRdisplay("    Base RCB: %p  RTB: %p  SI Flags: %v\n",
	    rcb->rcb_base_rcb, rcb->rcb_rtb_ptr,
	    RCB_SI_FLAGS, rcb->rcb_si_flags);
	TRdisplay("    SI Oldtid: {%d,%d}  SI Newtid: {%d,%d}\n",
	    rcb->rcb_si_oldtid.tid_tid.tid_page,
	    rcb->rcb_si_oldtid.tid_tid.tid_line,
	    rcb->rcb_si_newtid.tid_tid.tid_page,
	    rcb->rcb_si_newtid.tid_tid.tid_line);

	TRdisplay("    RECORD:\n");
	for (i = 0; i < rcb->rcb_srecord_ptr - rcb->rcb_record_ptr; i += 32)
	    TRdisplay("    0x%p:%< %8.4{%x %}%2< >%32.1{%c%}<\n",
		rcb->rcb_record_ptr + i, 0);
	TRdisplay("    SCRATCH:\n");
	for (i = 0; i < rcb->rcb_srecord_ptr - rcb->rcb_record_ptr; i += 32)
	    TRdisplay("    0x%p:%< %8.4{%x %}%2< >%32.1{%c%}<\n",
		rcb->rcb_srecord_ptr + i, 0);
	TRdisplay("    Qualification Fcn: 0x%p Arg: 0x%p Rowaddr*: 0x%p\n",
	    rcb->rcb_f_qual, rcb->rcb_f_arg, rcb->rcb_f_rowaddr);
	TRdisplay("    HIGH KEY: given = %3d operator = %w\n", 
            rcb->rcb_ll_given,
	    ",LTE,EQ,GTE", rcb->rcb_ll_op_type);
	for (i = 0; i < rcb->rcb_ll_ptr - rcb->rcb_hl_ptr; i += 32)
	    TRdisplay("    0x%p:%< %8.4{%x %}%2< >%32.1{%c%}<\n",
	        rcb->rcb_hl_ptr + i, 0);
	TRdisplay("    LOW KEY:  given = %3d operator = %w\n", 
            rcb->rcb_ll_given,
	    ",LTE,EQ,GTE", rcb->rcb_ll_op_type);
	for (i = 0; i < rcb->rcb_ll_ptr - rcb->rcb_hl_ptr; i += 32)
	    TRdisplay("    0x%p:%< %8.4{%x %}%2< >%32.1{%c%}<\n",
	        rcb->rcb_ll_ptr + i, 0);
	TRdisplay("    SCRATCH KEY:\n");
	for (i = 0; i < rcb->rcb_ll_ptr - rcb->rcb_hl_ptr; i += 32)
	    TRdisplay("    0x%p:%< %8.4{%x %}%2< >%32.1{%c%}<\n",
	        rcb->rcb_s_key_ptr + i, 0);

	TRdisplay("	RCB queue Next: 0x%p Previous 0x%p\n",
	    rcb->rcb_q_next, rcb->rcb_q_prev);
	TRdisplay("	Owner: %d    Etab parent: %d\n",
		rcb->rcb_owner, rcb->rcb_et_parent_id);
	TRdisplay("	Internal RCB: %w\n","NO,YES",rcb->rcb_internal_req);
	TRdisplay("	XCB for this RCB: 0x%p\n",rcb->rcb_xcb_ptr);
	TRdisplay("	RNL CB: 0x%p	1st Page: 0x%p	2nd Page: 0x%p\n",
	    rcb->rcb_rnl_cb, rcb->rcb_1rnl_page, rcb->rcb_2rnl_page);
	TRdisplay("	Hash nofull page: %d\n",rcb->rcb_hash_nofull);
	TRdisplay("	Collation Desc: 0x%p\n",rcb->rcb_collation);
	TRdisplay("	Unicode Collation Desc: 0x%p\n",rcb->rcb_ucollation);
	TRdisplay("	Gateway RCB: 0x%p\n",rcb->rcb_gateway_rsb);
	TRdisplay("	ADF CB: 0x%p\n",rcb->rcb_adf_cb);
	TRdisplay("	Updating list: Next 0x%p  Previous 0x%p\n",
		rcb->rcb_upd_list.q_next,rcb->rcb_upd_list.q_prev);
	TRdisplay("	Updating list (master): Next 0x%p  Previous 0x%p\n",
		rcb->rcb_mupd_list.q_next,rcb->rcb_mupd_list.q_prev);
	TRdisplay("	Val Logkey: %v\n","TABKEY,OBJKEY",rcb->rcb_val_logkey);
	TRdisplay("	LogKey - Low: 0x%x  High: 0x%x  Rel: 0x%x  DB: 0x%x\n",
	    rcb->rcb_logkey.olk_low_id, rcb->rcb_logkey.olk_high_id,
	    rcb->rcb_logkey.olk_rel_id, rcb->rcb_logkey.olk_db_id);
	TRdisplay("	Fix: %d	 IO: %d	 Replace: %d  Insert %d  Delete %d\n",
	    rcb->rcb_s_fix, rcb->rcb_s_io, rcb->rcb_s_rep, rcb->rcb_s_ins,
	    rcb->rcb_s_del);
	TRdisplay("	Get: %d  Qual: %d  Scan: %d  Load: %d\n",
	    rcb->rcb_s_get, rcb->rcb_s_qual, rcb->rcb_s_scan,
	    rcb->rcb_s_load);
	TRdisplay("     Reltups: %d Relpages: %d Tuple adds: %d Page adds: %d\n",
	    rcb->rcb_reltups, rcb->rcb_relpages, rcb->rcb_tup_adds,
	    rcb->rcb_page_adds);
	break;

    case SCB_CB:
	scb = (DML_SCB *)obj;
	TRdisplay("    User: %~t  Type: %v  Tran_count: %d   DB_count: %d\n",
	    sizeof(scb->scb_user), &scb->scb_user,
	    "SYSUPDATE,SECURITY,DOWNGRADE,FACTOTUM", 
	    scb->scb_s_type,
	    scb->scb_x_ref_count, scb->scb_o_ref_count);
	TRdisplay("    Session ID: 0x%p Interrupt State: %v (%d)  Lock List: 0x%x\n",
	    scb->scb_sid,
	    "USER_INTERRUPT,FORCE_ABORT,CTHREAD_INTR,FAPENDING", scb->scb_ui_state,
	    scb->scb_ui_state, scb->scb_lock_list);
	TRdisplay("    ODCB    Next: 0x%p   Previous: 0x%p\n",
	    scb->scb_oq_next, scb->scb_oq_prev);
	TRdisplay("    XCB     Next: 0x%p   Previous: 0x%p\n",
	    scb->scb_x_next, scb->scb_x_prev);
	TRdisplay("    SLCB    Next: 0x%p   Previous: 0x%p\n",
	    scb->scb_kq_next, scb->scb_kq_prev);
	TRdisplay("    Session State: %v (%x)\n",
	    "WILLING_COMMIT", scb->scb_state, scb->scb_state);
	TRdisplay("    Session Mask: %v (%x)\n",
	    "1,2,4,8,GW_CONNECT,NO_LOGGING,40,80,LFULL_COMMIT,LFULL_NOTIFY",
	    scb->scb_sess_mask,
	    scb->scb_sess_mask);
	TRdisplay("    GW session ID=%p, Session TimeZone=0x%p\n",
	    scb->scb_gw_session_id, scb->scb_tzcb);
	TRdisplay("    Max Trans: %d  Max Opendbs: %d\n",
	    scb->scb_x_max_tran, scb->scb_o_max_opendb);
	TRdisplay("    Real User: %~t  Audit State: %w\n",
	    sizeof(scb->scb_real_user.db_own_name), 
	    scb->scb_real_user.db_own_name,
	    "FALSE,TRUE", scb->scb_audit_state);
        TRdisplay("    Timeout: %d, Maxlocks: %d, GTT ET hint: %d\n",
            scb->scb_timeout, scb->scb_maxlocks, scb->scb_global_lo);
        TRdisplay("    Lock Level: %d, Iso Level: %d, DOP: %d\n",
            scb->scb_lock_level, scb->scb_sess_iso,
	    scb->scb_dop);
	/* If shortterm memory used, display it */
	if ( scb->scb_mem_hwm )
	{
	    TRdisplay("    ShortTerm (ST) Memory Alloc: %d, HWM: %d\n",
		scb->scb_mem_alloc, scb->scb_mem_hwm);
	    TRdisplay("    ST pool list Next: %p Prev: %p Size: %d\n",
		scb->scb_pool_list.obj_next, 
		scb->scb_pool_list.obj_prev, 
		scb->scb_pool_list.obj_size);
	    TRdisplay("    ST free list Next: %p Prev: %p Size: %d\n",
		scb->scb_free_list.obj_next, scb->scb_free_list.obj_prev,
		scb->scb_free_list.obj_size);
	    TRdisplay ("\n     Displaying contents of ST Memory...\n");
	    (VOID)dm0m_search(scb, 0, dmd_fmt_cb, flag_ptr, &local_dberr);

	    TRdisplay("\n     ...End of ST Memory\n");
	}

	
	break;

    case SLCB_CB:
	slcb = (DML_SLCB *)obj;
	TRdisplay("    SCB:0x%p Table: (%x,0)\n",
	    slcb->slcb_scb_ptr, slcb->slcb_db_tab_base);
        TRdisplay ("     Timeout: %d, Maxlocks: %d\n",
            slcb->slcb_timeout, slcb->slcb_maxlocks);
        TRdisplay ("     Lock Level: %d, Readlock: %d\n",
            slcb->slcb_lock_level, slcb->slcb_readlock);
	break;

    case SPCB_CB:
	spcb = (DML_SPCB *)obj;
	TRdisplay(
	    "    Savepoint Name: %.#s  Identifier: %d   XCB:0x%p LSN:(%x,%x)\n",
	    sizeof(spcb->spcb_name), &spcb->spcb_name, 
	    spcb->spcb_id,
	    spcb->spcb_xcb_ptr, 
	    spcb->spcb_lsn.lsn_high, spcb->spcb_lsn.lsn_low);
	break;

    case SVCB_CB:
	svcb = (DM_SVCB *)obj;
	TRdisplay("    Server Name: %~t  Id: 0x%x  Status: %v\n",
	    sizeof(svcb->svcb_name), &svcb->svcb_name, svcb->svcb_id,
	    SVCB_STATUS, svcb->svcb_status);
	TRdisplay("    Lock list: %x     Log id: %x\n",
	    svcb->svcb_lock_list, svcb->svcb_lctx_ptr->lctx_lgid);
	TRdisplay("    DCB   Next: 0x%p    Previous: 0x%p\n", svcb->svcb_d_next,
	    svcb->svcb_d_prev);
	TRdisplay("    SCB   Next: 0x%p    Previous: 0x%p\n", svcb->svcb_s_next,
	    svcb->svcb_s_prev);
	TRdisplay("    HCB: 0x%p  LBMCB: 0x%p bm_common: 0x%p  LCTX: 0x%p\n",
	    svcb->svcb_hcb_ptr, svcb->svcb_lbmcb_ptr, 
	    svcb->svcb_lbmcb_ptr->lbm_bm_common,
	    svcb->svcb_lctx_ptr);
	TRdisplay("    DCB Mutex: %#.4{%x %}\n", (sizeof(DM_MUTEX) / 4),
	    &svcb->svcb_dq_mutex, 0);
	TRdisplay("    SCB Mutex: %#.4{%x %}\n", (sizeof(DM_MUTEX) / 4),
	    &svcb->svcb_sq_mutex, 0);
	TRdisplay("    Maximums Sessions: %d  Databases: %d  Transactions: %d  Memory: %d\n",
	    svcb->svcb_s_max_session, svcb->svcb_d_max_database,
	    svcb->svcb_x_max_transactions, svcb->svcb_m_max_memory);
	TRdisplay("    Statistics Sessions: %d  Databases: %d  Transactions: %d\n",
	    svcb->svcb_cnt.cur_session, svcb->svcb_cnt.cur_database,
	    svcb->svcb_cnt.cur_transaction);
	TRdisplay("    Shortterm (ST) memory allocations: %d\n",
	    svcb->svcb_stat.ses_mem_alloc);
	for ( i = 0; i < svcb->svcb_pools; i++ )
	{
	    TRdisplay("    LT Memory pool[%d] Alloc: %d  Dealloc: %d  Expand %d\n",
		i, svcb->svcb_stat.mem_alloc[i],
		   svcb->svcb_stat.mem_dealloc[i],
		   svcb->svcb_stat.mem_expand[i]);
	    TRdisplay("    LT Memory Mutex[%d]: %#.4{%x %}\n", 
		i, (sizeof(DM_MUTEX) / 4),
		&svcb->svcb_mem_mutex[i], 0);
	    TRdisplay("    LT Memory pool list[%d] Next: %p Prev: %p Size: %d\n",
		i, svcb->svcb_pool_list[i].obj_next, svcb->svcb_pool_list[i].obj_prev,
		svcb->svcb_pool_list[i].obj_size);
	    TRdisplay("    LT Memory free list[%d] Next: %p Prev: %p Size: %d\n",
		i, svcb->svcb_free_list[i].obj_next, svcb->svcb_free_list[i].obj_prev,
		svcb->svcb_free_list[i].obj_size);
	}
	break;

    case TCB_CB:
	tcb = (DMP_TCB *)obj;
	tbio = &tcb->tcb_table_io;

	TRdisplay("    Table Name: (%~t,%~t)   Id: (%d%s%d)  Partition: %d Uid: %x\n",
	    sizeof(tcb->tcb_rel.relid), &tcb->tcb_rel.relid,
	    sizeof(tcb->tcb_rel.relowner), &tcb->tcb_rel.relowner, 
	    tcb->tcb_rel.reltid.db_tab_base, 
	    (tcb->tcb_rel.reltid.db_tab_index & DB_TAB_PARTITION_MASK)
		? ",-" : ",",
	    tcb->tcb_rel.reltid.db_tab_index & ~DB_TAB_PARTITION_MASK,
	    tcb->tcb_partno, tcb->tcb_unique_id);
	TRdisplay("    Status: (0x%x) %v\n", tcb->tcb_status,
	    TCB_FLAG_MEANING, tcb->tcb_status);
	TRdisplay("    Ref_count: %d  Valid_count: %d  Open_count: %d\n",
	    tcb->tcb_ref_count, tcb->tcb_valid_count, tcb->tcb_open_count);
	if ( tcb->tcb_status & TCB_BUSY )
	{
	    TRdisplay("    ***Busy TCB, other info assumed inconsistent\n");
	    break;
	}
	TRdisplay("    Parent TCB: 0x%p  DCB:0x%p  Master TCB: 0x%p\n",
	    tcb->tcb_parent_tcb_ptr, tcb->tcb_dcb_ptr,tcb->tcb_pmast_tcb);
	TRdisplay("    Hash Bucket:0x%p  Hash Mutex at: 0x%p iirelation tid (%d,%d)\n",
	    tcb->tcb_hash_bucket_ptr,tcb->tcb_hash_mutex_ptr,
	    tcb->tcb_iirel_tid.tid_tid.tid_page,tcb->tcb_iirel_tid.tid_tid.tid_line);
	TRdisplay("    Tuple Adds: %d   Page Adds: %d\n",
	    tcb->tcb_tup_adds, tcb->tcb_page_adds);
	TRdisplay("    Table Lockid: %x%x\n", 
	    tcb->tcb_lk_id.lk_unique, tcb->tcb_lk_id.lk_common);
	TRdisplay("    Table Lockid at 0x%p (%s)\n",
	    tcb->tcb_lkid_ptr, (tcb->tcb_lkid_ptr == &tcb->tcb_lk_id) ? "Self":"Other");
	TRdisplay("    Statement Unique SI: %d  Unique Indexes: %d\n",
	    tcb->tcb_stmt_unique, tcb->tcb_unique_count);
	TRdisplay("    RTB Next: %p\n",
	    tcb->tcb_rtb_next);

	if ((tcb->tcb_status & TCB_PARTIAL) == 0)
	{
	    TRdisplay("    Attributes:  %w %w %w %w %w %w %w\n",
		"PERMANENT,TEMPORARY", tcb->tcb_temporary,
		"USER,SYSTEM", tcb->tcb_sysrel,
		",LOAD", tcb->tcb_loadfile,
		"NOLOGGING,", tcb->tcb_logging,
		",NO_FILE", tcb->tcb_nofile,
		"NO_UPDATE_IDX,UPDATE_IDX", tcb->tcb_update_idx,
		",NO_TIDS", tcb->tcb_no_tids);
	    TRdisplay("    More Attributes: %w %w %w\n",
		"NO_BT_OVFL,BT_DUPS_ON_OVFL",tcb->tcb_dups_on_ovfl,
		"RNG_KEYS_ONLY, RNG_HAS_NONKEY",tcb->tcb_rng_has_nonkey,
		",UNIQUE_IDX",tcb->tcb_unique_index);
	    TRdisplay("    INDEX Keys: %6.2{%5.2d%}\n", tcb->tcb_ikey_map, 0);
	
	    if (tcb->tcb_rel.relspec == TCB_BTREE || 
		tcb->tcb_rel.relspec == TCB_RTREE)

		TRdisplay("    KEY Count: %d   IXklen %d Leafklen: %d   PerIndex: %d PerLeaf\n",
		    tcb->tcb_keys, 
		    tcb->tcb_ixklen, 
		    tcb->tcb_klen,
		    tcb->tcb_kperpage, 
		    tcb->tcb_kperleaf);
	    else
		TRdisplay("    KEY Count: %d   Length: %d   Perpage: %d\n",
		    tcb->tcb_keys, tcb->tcb_klen, tcb->tcb_kperpage);
	    TRdisplay("    Worstcase compression expansion: data %d, leaf/index %d\n",
		tcb->tcb_data_rac.worstcase_expansion,
		tcb->tcb_index_rac.worstcase_expansion);
	    TRdisplay("    Logical Key: %d,%d Table-type: %w\n",
		tcb->tcb_high_logical_key, tcb->tcb_low_logical_key,
		",,,HEAP,,ISAM,,HASH,,,,BTREE,,RTREE,GATEWAY", 
		tcb->tcb_table_type);
	    TRdisplay("    LCT: 0x%p\n", tcb->tcb_lct_ptr);
	    TRdisplay("    Relation Information:\n");
	    TRdisplay("        Name: (%~t,%~t)   Id: (%d%s%d)   Page size: (%d)\n",
		sizeof(tcb->tcb_rel.relid), &tcb->tcb_rel.relid,
		sizeof(tcb->tcb_rel.relowner), &tcb->tcb_rel.relowner, 
		tcb->tcb_rel.reltid.db_tab_base, 
		(tcb->tcb_rel.reltid.db_tab_index & DB_TAB_PARTITION_MASK)
		    ? ",-" : ",",
		tcb->tcb_rel.reltid.db_tab_index & ~DB_TAB_PARTITION_MASK,
		tcb->tcb_rel.relpgsize);
	    TRdisplay("        Attributes: %d  Width: %d  Structure: %w Type: %w\n",
		tcb->tcb_rel.relatts, tcb->tcb_rel.relwid,
		",,,HEAP,,ISAM,,HASH,,,,BTREE,,RTREE", tcb->tcb_rel.relspec,
		"0,1,2,HEAP,4,ISAM,6,HASH,7,8,9,10,BTREE,,RTREE,GATEWAY",
		tcb->tcb_table_type);
	    TRdisplay("        Status: %v\n",
		RELSTAT, tcb->tcb_rel.relstat);
	    TRdisplay("        Relstat2: %v\n",
		RELSTAT2, tcb->tcb_rel.relstat2);
	    TRdisplay("        Tuples: %d Pages Total: %d Primary: %d Main: %d\n",
		tcb->tcb_rel.reltups, tcb->tcb_rel.relpages, 
		tcb->tcb_rel.relprim, tcb->tcb_rel.relmain);
	    TRdisplay("        Location: %~t       Save Date: %d\n",
		sizeof(tcb->tcb_rel.relloc), &tcb->tcb_rel.relloc, 
		tcb->tcb_rel.relsave);
	    TRdisplay("        Cmpt: %d Create: %x Mod: %x QID1: %d QID2: %d\n",
		tcb->tcb_rel.relcmptlvl,
		tcb->tcb_rel.relcreate, tcb->tcb_rel.relmoddate,
		tcb->tcb_rel.relqid1, tcb->tcb_rel.relqid2);
	    TRdisplay("        Index Count: %d  NumLocs: %d\n", 
		tcb->tcb_rel.relidxcount, tcb->tcb_rel.relloccount);
	    TRdisplay("        Ifill: %d Dfill: %d Lfill: %d Min: %d Max: %d\n",
		tcb->tcb_rel.relifill, tcb->tcb_rel.reldfill, 
		tcb->tcb_rel.rellfill, tcb->tcb_rel.relmin,tcb->tcb_rel.relmax);
	    TRdisplay("        Gateway_id: %d Gateway_other: %d\n",
		tcb->tcb_rel.relgwid, tcb->tcb_rel.relgwother);
	    TRdisplay("        LogicalKey(%x,%x)\n",
		tcb->tcb_rel.relhigh_logkey, tcb->tcb_rel.rellow_logkey);
	    TRdisplay("        Allocation: %d Extend: %d Free_header: %d\n",
		tcb->tcb_rel.relallocation, tcb->tcb_rel.relextend,
		tcb->tcb_rel.relfhdr);
	    TRdisplay("        Timestamp - High: 0x%x  Low: 0x%x\n",
		tcb->tcb_rel.relstamp12.db_tab_high_time,
		tcb->tcb_rel.relstamp12.db_tab_low_time);
	    TRdisplay("        Relnparts: %d  Relnpartlevels: %d\n",
		tcb->tcb_rel.relnparts, tcb->tcb_rel.relnpartlevels);
	}

	TRdisplay("    Table IO Information @0x%p\n", tbio);
	TRdisplay("        Table IO Flags: %v %v\n",
	    TBIO_STATUS, tbio->tbio_status,
	    TBIO_CACHE_FLAGS, tbio->tbio_cache_flags);
	TRdisplay("        DBid: %d  Table Id: (%d%s%d)\n",
	    tbio->tbio_dcbid, tbio->tbio_reltid.db_tab_base, 
	    (tbio->tbio_reltid.db_tab_index & DB_TAB_PARTITION_MASK)
		? ",-" : ",",
	    tbio->tbio_reltid.db_tab_index & ~DB_TAB_PARTITION_MASK);
	TRdisplay("        Table Type: %d  %w %w %w\n",
	    tbio->tbio_table_type,
	    "PERMANENT,TEMPORARY", tbio->tbio_temporary,
	    "USER_REL,SYSTEM_REL", tbio->tbio_sysrel,
	    "NORMAL,EXTENDED", tbio->tbio_extended);
	TRdisplay("        Page size: %d   Cache Ix: %d\n",
		tbio->tbio_page_size, tbio->tbio_cache_ix);
	TRdisplay("        Last Initialized Page: %d  Last Allocated Page: %d\n",
	    tbio->tbio_lpageno, tbio->tbio_lalloc);
	TRdisplay("        Check EOF: %d   Sync-on-close: %d   TT Hiwater Pageno: %d\n",
		tbio->tbio_checkeof, tbio->tbio_sync_close, tbio->tbio_tmp_hw_pageno);
	TRdisplay("        Cache Priority: %d  Valid Stamp0x%x\n", 
	    tbio->tbio_cache_priority, tbio->tbio_cache_valid_stamp);
	TRdisplay("    Location Count: %d  Location Array PTR: 0x%p\n", 
	    tbio->tbio_loc_count, tbio->tbio_location_array);

	TRdisplay("    INDEX TCBs Next :0x%p  Previous: 0x%p Count: %d\n",
	    tcb->tcb_iq_next, tcb->tcb_iq_prev, tcb->tcb_index_count);
	TRdisplay("    FREE     Next: 0x%p   Previous: 0x%p\n",
	    tcb->tcb_fq_next, tcb->tcb_fq_prev);
	TRdisplay("    ACTIVE   Next: 0x%p   Previous: 0x%p\n",
	tcb->tcb_rq_next, tcb->tcb_rq_prev);
	TRdisplay("    FREE BASE TCBs  Next: 0x%p  Previous 0x%p\n",
	    tcb->tcb_ftcb_list.q_next, tcb->tcb_ftcb_list.q_prev);
	TRdisplay("    Possibly-updating RCBs  Next: 0x%p  Previous 0x%p\n",
	    tcb->tcb_upd_rcbs.q_next, tcb->tcb_upd_rcbs.q_prev);
	TRdisplay("    Mutex: %#.4{%x %}\n", (sizeof(DM_MUTEX) / 4),
	    &tcb->tcb_mutex, 0);

	if ( ! (tcb->tcb_status & TCB_PARTIAL) && (tcb->tcb_atts_ptr) )
	{
	    TRdisplay("    Attribute Information\n");
	    TRdisplay("         ATTID  OFFSET  TYPE    LENGTH  PREC KEY KOFFSET\n"); 
	    TRdisplay("%#.#{%8* %6.4d  %4.2d  %8.2w  %4.2d %4.2d %3.2d    %4.2d\n%}",
		tcb->tcb_rel.relatts, 
		sizeof(DB_ATTS), &tcb->tcb_atts_ptr[1], 
		&((DB_ATTS *)0)->ordinal_id, 
		&((DB_ATTS *)0)->offset, 
		",,,DATE,ADATE,MONEY,TMWO,TMW,TME,TSWO,DECIMAL,OBJECT_KEY,TABLE_KEY,,,,,,TSW,TSTMP,CHAR,VCHAR,LVCH,BYTE,VBYTE,LBYTE,NCHAR,NVCHR,LNVCHR,,INT,FLOAT,C,,,,,TEXT,BOOL",
		 &((DB_ATTS *)0)->type,
		&((DB_ATTS *)0)->length, &((DB_ATTS *)0)->precision, 
		&((DB_ATTS *)0)->key,
		&((DB_ATTS *)0)->key_offset); 
	}

	if ( ! (tcb->tcb_status & TCB_PARTIAL) && (tcb->tcb_keys))
	{
	    TRdisplay("    Key Information\n");
	    TRdisplay("         ATTID  OFFSET  TYPE    LENGTH PREC KEY KOFFSET\n"); 
	    TRdisplay("%#[%8* %6.4d  %4.2d  %8.2w  %4.2d %4.2d %3.2d    %4.2d\n%]",
		tcb->tcb_rel.relkeys, tcb->tcb_key_atts,
		&((DB_ATTS *)0)->ordinal_id, 
		&((DB_ATTS *)0)->offset, 
		",,,DATE,ADATE,MONEY,TMWO,TMW,TME,TSWO,DECIMAL,OBJECT_KEY,TABLE_KEY,,,,,,TSW,TSTMP,CHAR,VCHAR,LVCH,BYTE,VBYTE,LBYTE,NCHAR,NVCHR,LNVCHR,,INT,FLOAT,C,,,,,TEXT,BOOL",
		&((DB_ATTS *)0)->type,
		&((DB_ATTS *)0)->length, &((DB_ATTS *)0)->precision, &((DB_ATTS *)0)->key,
		&((DB_ATTS *)0)->key_offset); 
	}

	TRdisplay("    Location Information\n");
	TRdisplay("        Location Count: %d  Location Array PTR: 0x%p\n\n",
	    tcb->tcb_table_io.tbio_loc_count, 
	    tcb->tcb_table_io.tbio_location_array);
	for (i = 0; i < tcb->tcb_table_io.tbio_loc_count; i++)
	{
	    loc = &tcb->tcb_table_io.tbio_location_array[i];
	    TRdisplay("        Location [%d] %v\n", i,
		LOC_STATUS, loc->loc_status);
	    TRdisplay("            EXT Ptr: 0x%p  FCB Ptr: 0x%p\n",
		loc->loc_ext, loc->loc_fcb);
	    if (loc->loc_status & LOC_VALID)
	    {
		TRdisplay("            Id: %d  #: %d  Name: %~t\n",
		    loc->loc_id, loc->loc_config_id, 
		    sizeof(loc->loc_name), &loc->loc_name);
	    }
	}
	TRdisplay("    Physical Partition Array: 0x%p\n",tcb->tcb_pp_array);
	if ((tcb->tcb_rel.relstat & TCB_IS_PARTITIONED) && tcb->tcb_pp_array != NULL)
	{
	    DMT_PHYS_PART *pp;
	    for (i = 0; i < tcb->tcb_rel.relnparts; i++)
	    {
		pp = &tcb->tcb_pp_array[i];
		TRdisplay("        PP[%5d]: ID (%d%s%d) TCB: 0x%p\n",
		    i, pp->pp_tabid.db_tab_base, 
		    (pp->pp_tabid.db_tab_index & DB_TAB_PARTITION_MASK)
			? ",-" : ",",
		    pp->pp_tabid.db_tab_index & ~DB_TAB_PARTITION_MASK,
		    pp->pp_tcb);
		TRdisplay("                  reltups %d, relpages %d, iirel tid (%d,%d)\n",
		    pp->pp_reltups, pp->pp_relpages,
		    pp->pp_iirel_tid.tid_tid.tid_page,pp->pp_iirel_tid.tid_tid.tid_line);
	    }
	}


	break;

    case XCB_CB:
	xcb = (DML_XCB *)obj;
	TRdisplay("    Type: %v (%x) State: %v (%x) ODCB: 0x%p\n",
	    XCB_X_TYPE, xcb->xcb_x_type, xcb->xcb_x_type,
	    XCB_STATE, xcb->xcb_state, xcb->xcb_state,
	    xcb->xcb_odcb_ptr);
	TRdisplay("    Flags: %v (%x)\n",
	    XCB_FLAGS, xcb->xcb_flags, xcb->xcb_flags);
	TRdisplay("    Tran Id: 0x%x%x  Log Id: 0x%x Lock Id: 0x%x\n",
	    xcb->xcb_tran_id.db_high_tran, xcb->xcb_tran_id.db_low_tran,
	    xcb->xcb_log_id, xcb->xcb_lk_id);
	TRdisplay("    RCB    Next: 0x%p   Previous: 0x%p\n",
	    xcb->xcb_rq_next, xcb->xcb_rq_prev);
	TRdisplay("    SPCB   Next: 0x%p   Previous: 0x%p\n",
	    xcb->xcb_sq_next, xcb->xcb_sq_prev);
	TRdisplay("    Commit Next: 0x%p   Previous: 0x%p\n",
	    xcb->xcb_cq_next, xcb->xcb_cq_prev);
	TRdisplay("    DCB: 0x%p  Savepoint Id: %d\n",
	    xcb->xcb_odcb_ptr, xcb->xcb_sp_id);
	TRdisplay("	Owner: %~t\n", 
	    sizeof(xcb->xcb_username.db_own_name), xcb->xcb_username.db_own_name);
	TRdisplay("	Open: %d    Fix: %d      Get: %d   Replace: %d \n",
	    xcb->xcb_s_open, xcb->xcb_s_fix, xcb->xcb_s_get,
	    xcb->xcb_s_replace);
	TRdisplay("	Delete: %d  Insert: %d   CPU: %d   DIO: %d\n",
	    xcb->xcb_s_delete, xcb->xcb_s_insert, xcb->xcb_s_cpu,
	    xcb->xcb_s_dio);

	/* Display CRIB contents, if any */
	if ( crib = xcb->xcb_crib_ptr )
	{
	    TRdisplay("    CRIB: 0x%p next: 0x%p prev: 0x%p\n",
	    	crib, crib->crib_next, crib->crib_prev);
	    TRdisplay("          Low LSN:(%x,%x) Last Commit:(%x,%x) Bos LSN:(%x,%x)\n",
		crib->crib_low_lsn.lsn_high,
		crib->crib_low_lsn.lsn_low,
		crib->crib_last_commit.lsn_high,
		crib->crib_last_commit.lsn_low,
		crib->crib_bos_lsn.lsn_high,
		crib->crib_bos_lsn.lsn_low);
	    TRdisplay("          Bos Tranid: 0x%x Sequence: %d Xid Array: 0x%p\n",
	        crib->crib_bos_tranid,
		crib->crib_sequence,
		crib->crib_xid_array);
	    TRdisplay("          Cursid: 0x%p Inuse: %d RCB State: %v\n",
	        crib->crib_cursid,
		crib->crib_inuse,
		RCB_STATE, crib->crib_rcb_state);
	    TRdisplay("          Active Transactions: %d...%d\n",
	        crib->crib_lgid_low,
		crib->crib_lgid_high);
	    for ( i = crib->crib_lgid_low; i <= crib->crib_lgid_high; i++ )
	    {
	        if ( crib->crib_xid_array[i] )
		    TRdisplay("          %4d: 0x%x\n",
		        i, crib->crib_xid_array[i]);
	    }
	}
	        
	/* Display Sequences */
	if ( seq = xcb->xcb_seq )
	{
	    do TRdisplay("     Sequence %p Name: %~t Owner: %~t Id: (%d,%d)\n",
		    seq, 
		    sizeof(seq->seq_name.db_name), seq->seq_name.db_name,
		    sizeof(seq->seq_owner.db_own_name), seq->seq_owner.db_own_name,
		    seq->seq_id, seq->seq_cache);
	    while ( seq = seq->seq_q_next );
	}
	break;

    case CRIB_CB:
	/* Cursor CRIBs are separately allocated */
        crib = (LG_CRIB*)((char*)obj + sizeof(DM_OBJECT));
	TRdisplay("    CRIB: 0x%p next: 0x%p prev: 0x%p cursid: 0x%p inuse: %d\n",
	    crib, crib->crib_next, crib->crib_prev,
	    crib->crib_cursid, crib->crib_inuse);
	TRdisplay("          Low LSN:(%x,%x) Last Commit:(%x,%x) Bos LSN:(%x,%x)\n",
	    crib->crib_low_lsn.lsn_high,
	    crib->crib_low_lsn.lsn_low,
	    crib->crib_last_commit.lsn_high,
	    crib->crib_last_commit.lsn_low,
	    crib->crib_bos_lsn.lsn_high,
	    crib->crib_bos_lsn.lsn_low);
	TRdisplay("          Bos Tranid: 0x%x Sequence: %d Xid Array: 0x%p\n",
	    crib->crib_bos_tranid,
	    crib->crib_sequence,
	    crib->crib_xid_array);
	TRdisplay("          Cursid: 0x%p Inuse: %d RCB State: %v\n",
	    crib->crib_cursid,
	    crib->crib_inuse,
	    RCB_STATE, crib->crib_rcb_state);
	TRdisplay("          Active Transactions: %d...%d\n",
	    crib->crib_lgid_low,
	    crib->crib_lgid_high);
	for ( i = crib->crib_lgid_low; i <= crib->crib_lgid_high; i++ )
	{
	    if ( crib->crib_xid_array[i] )
		TRdisplay("          %4d: 0x%x\n",
		    i, crib->crib_xid_array[i]);
	}
	break;

    case XCCB_CB:
	xccb = (DML_XCCB *) obj;
	TRdisplay("    q_next: 0x%p   q_prev: 0x%p\n",
		xccb->xccb_q_next, xccb->xccb_q_prev);
	TRdisplay("    XCB: 0x%p   DCB: 0x%p\n",
		xccb->xccb_xcb_ptr, xccb->xccb_t_dcb);
	TRdisplay("    Savepoint ID %d   Commit-op %v (%d)  tabid (%d,%d)\n",
		xccb->xccb_sp_id, "DELETE,DESTROY", xccb->xccb_operation,
		xccb->xccb_operation, xccb->xccb_t_table_id.db_tab_base,
		xccb->xccb_t_table_id.db_tab_index);
	TRdisplay("    File: %t  Path:%t\n",
		xccb->xccb_f_len,xccb->xccb_f_name.name,
		xccb->xccb_p_len,xccb->xccb_p_name.name);
	TRdisplay("    Is-session? %d   blob_temp_flags %x  blob_locator %d\n",
		xccb->xccb_is_session, xccb->xccb_blob_temp_flags,
		xccb->xccb_blob_locator);
	break;

    case PFH_CB:
	pfh = (DMP_PFH *)obj;
	already_locked = CSp_semaphore(1,&pfh->pfh_mutex);
	TRdisplay("    Status: %v (%x) num blocks: %d  num sched: %d\n",
	    "PFWAIT", pfh->pfh_status, pfh->pfh_status,  
	    pfh->pfh_numreqs, pfh->pfh_numsched);

	/*
	** ??? I'm not sure that the ( DESKTOP or VMS ) case here (which
	** ??? just skips dumping the smx_count of the mutex) shouldn't
	** ??? be used everywhere.  There shouldn't be references to the
	** ??? internals of CL structures in generic code.
	*/
# if defined( DESKTOP ) || defined( VMS )
	/* pfh->pfh_mutex.cs_smstatistics.cs_smx_count doesn't exist. */
	TRdisplay( "    TOTALS: sched: %d  complete: %d  cancelled: %d\n",
		  pfh->pfh_scheduled, pfh->pfh_racomplete,
		  pfh->pfh_cancelled );
# else /* DESKTOP or VMS */
	TRdisplay(
	    "    TOTALS: mutex: %d  sched: %d  complete: %d  cancelled: %d\n",
	    pfh->pfh_mutex.cs_smstatistics.cs_smx_count,
	    pfh->pfh_scheduled, pfh->pfh_racomplete, pfh->pfh_cancelled);
# endif /* not DESKTOP or VMS */
	
	TRdisplay(
	    "    lag: %d  fall: %d  lkcon: %d  bufstress: %d  noblocks: %d\n",
	    pfh->pfh_lagbehind, pfh->pfh_fallbehind, pfh->pfh_lkcontend,
	    pfh->pfh_bufstress, pfh->pfh_no_blocks);
	
	TRdisplay("    Prefetch request blocks (%d)\n", pfh->pfh_numreqs);
	pfr = pfh->pfh_next;
	do 
	{
	    TRdisplay("        %x typ: %v (%x)  stat: %v (%x)  pincnt: %d\n",
	    pfr,
	    "PREFET_SEC,PREALLOC_SEC,PREFET_PAGE,PREFET_OVFL", 
	    pfr->pfr_type, pfr->pfr_type,
	    "FREE,WAITED_ON,STALE_REQ", pfr->pfr_status,pfr->pfr_status,
	    pfr->pfr_pincount);

	    switch (pfr->pfr_type)
	    {
	    	case PFR_PREFET_SECIDX:
	    	case PFR_PREALLOC_SECIDX:

	    	TRdisplay("        tot: %d  usr: %d  ra: %d  tid: %d/%d\n",
	    	pfr->t.si.totix, pfr->t.si.userix, pfr->t.si.raix,
	    	pfr->pfr_table_id.db_tab_base,pfr->pfr_table_id.db_tab_index);
		break;

		case PFR_PREFET_PAGE:

		TRdisplay("        page: %d\n",
		pfr->t.dp.pageno);
		break;

		case PFR_PREFET_OVFCHAIN:

		TRdisplay("start page: %d  usr cnt: %d RA cnt: %d\n",
		pfr->t.oc.startpage, pfr->t.oc.userpcnt, pfr->t.oc.rapcnt);
		break;
	    }

	    pfr = pfr->pfr_next;
	} while (pfr != pfh->pfh_next);
	/*
	** If we didn't already hold the pfh semaphore, release it now.
	*/
	if (!already_locked)
	    CSv_semaphore(&pfh->pfh_mutex);

	break;

    case SEQ_CB:
    case CSEQ_CB:
	if ( obj->obj_type == CSEQ_CB )
	{
	    cseq = (DML_CSEQ *)obj;
	    seq = cseq->cseq_seq;
	    TRdisplay("    Current value of Sequence %p : ", seq);

	    if ( seq->seq_dtype == DB_INT_TYPE )
		TRdisplay("%d\n", cseq->cseq_curr.intval);
	    else if ( seq->seq_dtype == DB_DEC_TYPE )
	    {
		char	curr[DB_MAX_DECPREC+2];
		i4	stlen;

		CVpka(cseq->cseq_curr.decval, DB_MAX_DECPREC, 0, 0,
		     sizeof(curr), 0, CV_PKLEFTJUST, curr, &stlen);

		TRdisplay("%s\n", curr);
	    }
	    else
		TRdisplay("UNKNOWN\n");
	}
	else
	    seq = (DML_SEQ *)obj;

	TRdisplay("    DB: %d Name: %~t Owner: %~t Id: (%d,%d) Flags: %v (%x) Lock Mode: %d\n",
	    seq->seq_db_id,
	    sizeof(seq->seq_name.db_name), seq->seq_name.db_name,
	    sizeof(seq->seq_owner.db_own_name), seq->seq_owner.db_own_name,
	    seq->seq_id, seq->seq_cache,
	    "CYCLE,ORDER,CACHE,UPDATED",
	    seq->seq_flags, seq->seq_flags,
	    seq->seq_lock_mode);
	TRdisplay("    Cache: %d Dtype: %d Dlen: %d Prec: %d\n",
	    seq->seq_cache, seq->seq_dtype,
	    seq->seq_dlen, seq->seq_prec);
	if ( seq->seq_dtype == DB_INT_TYPE )
	{
	    TRdisplay("    Next: %d Incr: %d Min: %d Max: %d\n",
		seq->seq_next.intval, seq->seq_incr.intval,
		seq->seq_min.intval, seq->seq_max.intval);
	}
	else if ( seq->seq_dtype == DB_DEC_TYPE )
	{
	    /* Need room for EOS and sign */
	    char	next[DB_MAX_DECPREC+2];
	    char	incr[DB_MAX_DECPREC+2];
	    char	min[DB_MAX_DECPREC+2];
	    char	max[DB_MAX_DECPREC+2];
	    i4		stlen;

	    CVpka(seq->seq_next.decval, DB_MAX_DECPREC, 0, 0,
	   	 sizeof(next), 0, CV_PKLEFTJUST, next, &stlen);
	    CVpka(seq->seq_incr.decval, DB_MAX_DECPREC, 0, 0,
	   	 sizeof(incr), 0, CV_PKLEFTJUST, incr, &stlen);
	    CVpka(seq->seq_min.decval, DB_MAX_DECPREC, 0, 0,
	   	 sizeof(min), 0, CV_PKLEFTJUST, min, &stlen);
	    CVpka(seq->seq_max.decval, DB_MAX_DECPREC, 0, 0,
		 sizeof(max), 0, CV_PKLEFTJUST, max, &stlen);

	    TRdisplay("    Next: %s Incr: %s Min: %s Max: %s\n",
		next, incr, min, max);
	}

	break;

    case RTB_CB:
	rtb = (DMR_RTB *)obj;

	TRdisplay("    Next: %p Prev: %p RCB: %p\n",
	    rtb->rtb_q_next, rtb->rtb_q_prev, rtb->rtb_rcb);
	TRdisplay("    DMR Operation: %d (%s)  Seq: %d  Flags: %x  DMR_CB: %p\n",
	    rtb->rtb_dmr_op,
	    (rtb->rtb_dmr_op == DMR_DELETE) 	? "DELETE" :
	    (rtb->rtb_dmr_op == DMR_GET) 	? "GET" :
	    (rtb->rtb_dmr_op == DMR_POSITION) 	? "POSITION" :
	    (rtb->rtb_dmr_op == DMR_PUT) 	? "PUT" :
	    (rtb->rtb_dmr_op == DMR_REPLACE) 	? "REPLACE" :
	    (rtb->rtb_dmr_op == DMR_LOAD) 	? "LOAD" :
	    (rtb->rtb_dmr_op == DMR_DUMP_DATA)	? "DUMP_DATA" :
	    (rtb->rtb_dmr_op == DMR_ALTER) 	? "ALTER" :
	    (rtb->rtb_dmr_op == DMR_AGGREGATE)	? "AGGREGATE" : "?",
	    rtb->rtb_dmr_seq, rtb->rtb_dm2r_flags, rtb->rtb_dmr);
	TRdisplay("    Status: %d  Errcode: %d\n",
	    rtb->rtb_status, rtb->rtb_error.err_code);
	TRdisplay("    Dupchecks: %d  SI Agents: %d  Agents: %d  Active: %d\n",
	    rtb->rtb_si_dupchecks, rtb->rtb_si_agents, 
	    rtb->rtb_agents, rtb->rtb_active);
	TRdisplay("    State: %v  Cond Waiters: %d  Agents...\n",
	    "BUSY,IDLE,CLOSE,TERMINATE", rtb->rtb_state,
	    rtb->rtb_cond_waiters);
	if ( rtl = rtb->rtb_rtl )
	{
	    TRdisplay("    Agents...\n");
	    while ( rtl )
	    {
		TRdisplay("          RTL: %p  TCB: %p  RCB: %p  SID: %x  Next: %p  Prev: %p\n",
		    rtl, rtl->rtl_tcb, rtl->rtl_rcb, rtl->rtl_sid,
		    rtl->rtl_next, rtl->rtl_prev);
		rtl = rtl->rtl_next;
	    }
	}
	break;

    case DM_BQCB_CB:
	bqcb = (DMPE_BQCB *) obj;
	TRdisplay("    Next: %p  base_id %d, nlobs %d\n",
		bqcb->hdr.obj_next, bqcb->bqcb_base_id, bqcb->bqcb_natts);
	TRdisplay("    Hints: TableLock %s, X-lock %s, CRIB %p\n",
		bqcb->bqcb_table_lock ? "Y" : "N",
		bqcb->bqcb_x_lock ? "Y" : "N",
		bqcb->bqcb_crib);
	TRdisplay("    Load-etabs %d, multi-row %s, logkey RCB %p\n",
		bqcb->bqcb_load_etabs,
		bqcb->bqcb_multi_row ? "Y" : "N",
		bqcb->bqcb_logkey_rcb);
	break;



    default:
	{
	    /*
	    ** Display more than just the header, up to 2k
	    ** of the object.
	    */
	    i4	todo;

	    if ( (todo = obj->obj_size) < sizeof(DM_OBJECT) )
		todo = sizeof(DM_OBJECT);
	    else if ( todo > 2048 )
		todo = 2048;

	    TRdisplay("    UNKNOWN CONTROL BLOCK\n");
	    for (i = 0; i < todo; i += 32)
		TRdisplay("    0x%p:%< %8.4{%x %}%2< >%32.1{%c%}<\n",
		    (char*)obj + i, 0);
	}
    }

    EXdelete();
}

