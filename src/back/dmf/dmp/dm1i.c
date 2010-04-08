/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>

#include    <cs.h>
#include    <di.h>
#include    <me.h>
#include    <pc.h>
#include    <tr.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>

#include    <adf.h>
#include    <ulf.h>

#include    <dmf.h>
#include    <dm.h>

#include    <dmp.h>
#include    <dmpl.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0p.h>
#include    <dm1c.h>
#include    <dm1i.h>
#include    <dm1p.h>
#include    <dm2f.h>
#include    <dm0m.h>
#include    <dm1r.h>
#include    <dmccb.h>
#include    <dmxcb.h>
#include    <dml.h>
#include    <dmxe.h>

/**
**
**  Name: DM1I.C - Routines to allocate and search ISAM tables.
**
**  Description:
**      This file contains the access method specific routines needed to
**      acess ISAM tables.
**
**      The routines defined in this file are:
**      dm1i_allocate       - Allocates space for a new record.
**      dm1i_search         - Sets the search limits for a scan.
**
**  History:
**      06-jan-86 (jennifer)
**          Converted for Jupiter.
**      19-mar-87 (jennifer)
**          Added code to check if pages already fixed in rcb, releases
**          them if they are.  Can't optimize this case since 
**          there is no way to determine what keys belong on this
**          page without searching through the index.
**      02-apr-87 (ac)
**          Added readlock equal nolock support.
**      24-Apr-89 (anton)
**          Added local collation support
**      29-may-89 (rogerk)
**          Check return status from dm1c_get calls.
**	19-nov-90 (rogerk)
**	    Changed to use the rcb's adf control block rather than allocating
**	    local adf cb's off of the stack.  This was done as part of the DBMS
**	    timezone changes.
**	18-dec-90 (rogerk)
**	    Fixed handling of duplicate checking/space allocation in unique
**	    secondary indexes:
**		- Use full key when searching for spot to put record - don't
**		  ignore TIDP to allocate space.
**		- Don't ignore the TIDP field when doing duplicate checking
**		  if the tidp is not part of the key (pre 6.0 indexes).
**		- Check to see if duplicate checking is required on previous
**		  or next isam page due to TIDP ignoring.
**	    Also consolidated duplicate checking code in ipage_dupcheck.
**	14-jun-1991 (Derek)
**	    Changes to support the new allocation routines.  Changed 
**	    functionallity of allocate so that is doesn't change the
**	    page.
**      15-oct-1991 (mikem)
**          Eliminated unix compile warnings from dm1i_allocate().
**      18-oct-1991 (mikem)
**          Fix 6.4->6.5 merge problem in dm1i_search() caused when rogerk's
**	    nov and dec 90 changes were merged into 6.5 code.
**      23-oct-1991 (jnash)
**          In dm1i_allocate, eliminate unused parameter in call to 
**	    dm0p_bicheck.
**	 3-nov-191 (rogerk)
**	    Added dm0l_ovfl log record during dm1i_allocate when a new overflow
**	    page is added.  This log record is used to REDO the operation of
**	    linking the new page into the overflow chain.
**	13-Nov-1991 (rmuth)
**	    In dm1i_allocate() when checking for duplicates we were 
**	    overwritting variables used latter on. This caused pages to be
**	    lost from overflow chains, fixed by adding two local variables. 
**	    Removed dependency that requires the overflow chain to always
**	    be added to the end.
**	20-Nov-1991 (rmuth)
**          Added DM1P_MINI_TRAN, when inside a mini-transaction pass this
**          flag to dm1p_getfree() so it does not allocate a page deallocate
**          earlier by this transaction. see dm1p.c for explaination wy.
**	    Corrected the code to pass the correct flags to dm1p_getfree().
**	 7-jul-1992 (rogerk)
**	    Prototyping DMF.
**      08-jul-1992 (mikem)
**          Fixed code problems uncovered by prototypes.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	26-oct-92 (rickh)
**	    Replaced references to DMP_ATTS with DB_ATTS.  Also replaced
**	    DM_CMP_LIST with DB_CMP_LIST.
**	14-dec-1992 (jnash & rogerk)
**	    6.5 Recovery Project
**	      - Changed to use new recovery protocols.  Before Images are
**		no longer written and the pages updated by the allocate must
**		be mutexed while the dm0l_ovfl log record is written and
**		the pages are updated.
**	      - Changed to pass new arguments to dm0l_ovfl.
**	      - Move compression out of dm1c layer, call tlv's here.
**	      - Add new param's to dmpp_allocate call.
**	      - Remove unused param's on dmpp_get calls.
**	      - Removed requirement that file-extend operations on system
**		catalogs must be forced to the database.  Restriction not
**		needed anymore since system catalogs now do Redo Recovery.
**	      - Changed arguments to dm0p_mutex and dm0p_unmutex calls.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	03-jan-1993 (jnash)
**	    More reduced logging work.
**	      - Add LGreserve calls.
**	08-feb-1993 (rmuth)
**	     When adding a new page set the page_stat to DMPP_OVFL as all
**	     new pages added are overflow pages.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed Begin and End Mini log records to use LSN's rather than LAs.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (rogerk)
**	    Change generation of Before Image log records during Online Backup.
**	    All page modifiers must now call dm0p_before_image_check before
**	    changing the page to implement online backup protocols.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() call.
**      10-apr-1995 (chech02) Bug# 64830 (merged the fixes from 6.4 by nick)
**          Unfix page pointed to by data in dm1i_search() to avoid
**          overwriting when fixing the result page. #64830
**	01-sep-1995 (cohmi01)
**	    Add support for DM1C_LAST request on an ISAM table, for use
**	    when executing MAX aggregate. Unlike btree, here DM1C_LAST means
**	    'last mainpg plus its overflows', all of which must be scanned.
**	23-may-96 (nick)
**	    Fix 74380 - don't fix ISAM index pages for write when not locking.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass tcb_rel.relpgsize as page_size argument to dmpp_allocate.
**	06-mar-1996 (stial01 for bryanp)
**	    Don't allocate tuple buffers on the stack.
**      06-mar-1996 (stial01)
**          ipage_dupcheck() Only allocate tuple buffer if necessary.
**	06-may-1996 (thaju02)
**	    New Page Format Support:
**		Change page header references to use macros.
**      03-june-1996 (stial01)
**          Use DMPP_INIT_TUPLE_INFO_MACRO to init DMPP_TUPLE_INFO
**	24-jul-96 (nick)
**	    Use DM1P_PHYSICAL in calls to dm1p_getfree() for system catalogs.
**	13-sep-1996 (canor01)
**	    Add rcb_tupbuf to dmpp_uncompress call to pass temporary buffer
**	    for uncompression.  Use the temporary buffer for uncompression
**	    when available.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          dm1i_allocate(): Set DM0L_ROW_LOCK flag if row locking is enabled.
**          dm1i_allocate(): Call dm1r_allocate() instead of dmpp_allocate
**          dm1i_allocate(): If row locking, extend file in a mini transaction.
**      12-dec-96 (dilma04)
**          If phantom protection is needed, set DM0P_PHANPRO fix action in
**          dm1i_search().
**      06-jan-97 (dilma04)
**          - bug 79816: in dm1i_search() DM0P_PHANPRO fix action must be set 
**          only if search mode is different from DM1C_POSITION;
**          - in dm1i_allocate() DM1P_PHYSICAL flag is needed only when calling 
**          dm1p_getfree() to allocate system catalog page;
**          - moved comment for previous change to the right place.
**      27-jan-97 (dilma04)
**          dm1i_search(): do not need phantom protection if searching to 
**          update secondary index.     
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          Use macro to check for serializable transaction.
**      10-mar-97 (stial01)
**          dm1i_allocate: Pass record to dm1r_allocate
**          ipage_dupcheck: Pass relwid to dm0m_tballoc()
**      21-may-97 (stial01)
**          dm1i_allocate() Added flags arg to dm0p_unmutex call(s).
**          No more LK_PH_PAGE locks, buffer locked with dm0p_lock_buf_macro()
**      19-jun-97 (stial01)
**          dm1i_allocate() When ovflpageno == 0, DONT unlock the buffer.
**      21-apr-98 (stial01)
**          Set DM0L_PHYS_LOCK in log rec if extension table (B90677)
**      13-Nov-98 (stial01)
**          Renamed DM1C_SI_UPDATE -> DM1C_FIND_SI_UPD
**	01-dec-1998 (nanpr01)
**	    Get rid of DMPP_TUPLE_INFO structure. Use DMPP_TUPLE_HDR instead.
**      09-feb-99 (stial01)
**          dm1i_search() Pass relcmptlvl to dmpp_ixsearch
**      05-may-1999 (nanpr01,stial01)
**          Key value locks acquired for duplicate checking are no longer held 
**          for duration of transaction. While duplicate checking, request/wait
**          for row lock if uncommitted duplicate key is found.
**	16-aug-1999 (nanpr01)
**	    Implementation of SIR 92299. Enabaling journalling on a
**	    online checkpoint.
**	21-oct-1999 (nanpr01)
**	    More CPU optimization. Do not copy tuple header when no required.
**	04-May-2000 (jenjo02)
**	    Pass blank-suppressed lengths of table/owner names to 
**	    dm0l_ovfl.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**      30-oct-2000 (stial01)
**          Pass crecord to dm1i_allocate, Pass rcb to dm1r_allocate
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-dec-2000 (wansh01)
***	     code in line 722  if ((comp_gt == 0) && (research_bucket = FALSE) 
**           should be == not =  
**      01-feb-2001 (stial01)
**          Arg changes for page accessors (Variable Page Type SIR 102955)
**      15-Jun-2001 (hanal04) Bug 104980 INGSRV 1473
**          Resolved bad logic in dm1i_allocate() when performing duplicate
**          checking against the partial key (i.e. key minus the tidp).
**      10-jul-2003 (stial01)
**          Fixed row lock error handling during allocate (b110521)
**      15-sep-2003 (stial01)
**          Skip allocate mini trans for etabs if table locking (b110923)
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      25-jun-2004 (stial01)
**          Modularize get processing for compressed, altered, or segmented rows
**      31-jan-05 (stial01)
**          Added flags arg to dm0p_mutex call(s).
**	22-Feb-2008 (kschendel) SIR 122739
**	    Minor changes for new row-accessor scheme.
**	11-Apr-2008 (kschendel)
**	    RCB's adfcb now typed as such, minor fixes here.
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	02-Dec-2008 (jonj)
**	    SIR 120874: dm1i_?, dm1r_? functions converted to DB_ERROR *,
**	    use new form uleFormat.
**	04-Dec-2008 (jonj)
**	    SIR 120874: dm1p_? functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dm1c_?, dm0p_? functions converted to DB_ERROR *
**	25-Aug-2009 (kschendel) 121804
**	    Need dmxe.h to satisfy gcc 4.3.
**      24-nov-2009 (stial01)
**          Check if table has segmented rows (instead of if pagetype supports)
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Replace DMPP_PAGE* with DMP_PINFO* where needed.
**	    New macros to lock/unlock pages, LG_LRI replaces LG_LSN,
**	    sensitized to crow_locking().
*/

/*
**  Forward and/or External function references.
*/

/*
**  Definition of static variables and forward static functions.
*/
static	DB_STATUS   ipage_dupcheck(
				DMP_RCB	    *rcb,
				i4	    dupflag,
				DMP_PINFO   *pinfo,
				char	    *record,
				i4	    keys_given,
				i4	    *comp_lt,
				i4	    *comp_gt,
				DB_ERROR    *dberr);

/*{
** Name: dm1i_allocate - Allocates space for a new record. 
**
** Description:
**    This routine finds space for a new record, allocates it, and 
**    returns the tid.
**    This routine does not change the page so that the before image of
**    the page can be logged before any changes occurs on the page.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      record                          Pointer to record to insert.
**      crecord				Compressed record (if compression)
**	tid				NOT USED.
**      need                            Value indicating amount of space
**                                      needed, assumes compressed if necessary.
**      dupflag                         A flag indicating if duplicates must be
**                                      checked.  Must be DM1C_DUPLICATES or
**                                      DM1C_NODUPLICATES.
**
** Outputs:
**      data                            Pointer to an area used to return
**                                      pointer to the data page.
**	tid				NOT RETURNED.
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
**                                      Error codes returned are:
**                                      E_DM003C_BAD_TID, E_DM0044_DELETED_TID.
**      
**      Returns:
**
**          E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**
**      Exceptions:
**          none
**
** Side Effects:
**          none.
**
** History:
**      06-jan-86 (jennifer)
**          Converted for Jupiter.
**       7-nov-88 (rogerk)
**          Send compression type to dm1c_get routine.
**      24-Apr-89 (anton)
**          Added local collation support
**      29-may-89 (rogerk)
**          Check return status from dm1c_get calls.
**	19-nov-90 (rogerk)
**	    Changed to use the rcb's adf control block rather than allocating
**	    local adf cb's off of the stack.  This was done as part of the DBMS
**	    timezone changes.
**	18-dec-90 (rogerk)
**	    Added fixes for problems with unique secondary indexes.
**	    Use full key to do search for spot to insert record - even for
**	    cases when TIDP column needs to be ignored for duplicate checking.
**	    Then when check for duplicate keys in release 6 secondary indexes,
**	    we need to check if the previous or next isam page/chain needs
**	    to also be checked.
**	    Consolidated duplicate checking code into routine ipage_dupcheck.
**      15-oct-1991 (mikem)
**          Eliminated unix compile warnings ("warning: statement not reached")
**          by changing the way "whil (xxx == E_DB_OK)" loops were coded.
**      23-oct-1991 (jnash)
**          Eliminate unused parameter in call to dm0p_bicheck.
**	 3-nov-191 (rogerk)
**	    Added dm0l_ovfl log record during dm1i_allocate when a new overflow
**	    page is added.  This log record is used to REDO the operation of
**	    linking the new page into the overflow chain.
**	13-Nov-1991 (rmuth)
**	    In dm1i_allocate() when checking for duplicates we were 
**	    overwritting variables used latter on. This caused pages to be
**	    lost from overflow chains, fixed by adding two local variables
**	    dup_data and dup_datapage. 
**	    Removed dependency that requires the overflow chain to always
**	    be added to the end.
**      20-Nov-1991 (rmuth)
**          If this is a system relation pass DM1P_OREDO and DM1P_MINI_TRAN
**          do not need to pass DM1P_PHYSICAL a this is set in the
**          rcb's 'rcb_k_duration' field.
**	08_jun_1992 (kwatts)
**	    6.5 MPF change. Replaced dm1c_allocates with accessors.
**	26-sep-1992 (jnash & rogerk)
**	    6.5 Recovery Project
**	      - Changed to use new recovery protocols.  Before Images are
**		no longer written and the pages updated by the allocate must
**		be mutexed while the dm0l_ovfl log record is written and
**		the pages are updated.
**	      - Changed to pass new arguments to dm0l_ovfl.
**	      - Add new param's to dmpp_allocate call.
**	      - Removed requirement that file-extend operations on system
**		catalogs must be forced to the database.  Restriction not
**		needed anymore since system catalogs now do Redo Recovery.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project (phase 2):
**	      - Added dm0p_pagetype call.
**	      - Removed old DM1P_NOREDO action - all allocates are now redone.
**	05-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**	08-feb-1993 (rmuth)
**	     When adding a new page set the page_stat to DMPP_OVFL as all
**	     new pages added are overflow pages.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed Begin and End Mini log records to use LSN's rather than LAs.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	26-jul-1993 (rogerk)
**	    Change generation of Before Image log records during Online Backup.
**	    All page modifiers must now call dm0p_before_image_check before
**	    changing the page to implement online backup protocols.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() call.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass tcb_rel.relpgsize as page_size argument to dmpp_allocate.
**	06-mar-1996 (stial01 for bryanp)
**	    Don't allocate tuple buffers on the stack.
**	06-may-1996 (thaju02)
**	    New Page Format Support:
**		Change page header references to use macros.
**	24-jul-96 (nick)
**	    Use DM1P_PHYSICAL for system catalogs.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Set DM0L_ROW_LOCK flag if row locking is enabled.
**          Call dm1r_allocate() instead of dmpp_allocate
**          If row locking, extend file in a mini transaction.
**      06-jan-97 (dilma04)
**          DM1P_PHYSICAL flag is needed only when calling dm1p_getfree() to
**          allocate system catalog page.
**      10-mar-97 (stial01)
**          Pass record to dm1r_allocate
**      21-may-97 (stial01)
**          dm1i_allocate() Added flags arg to dm0p_unmutex call(s).
**          No more LK_PH_PAGE locks, buffer locked with dm0p_lock_buf_macro()
**          Note we look at page_page,page_main when the buffer is not locked,
**          since they are only set when the page is formatted.
**      19-jun-97 (stial01)
**          dm1i_allocate() When ovflpageno == 0, DONT unlock the buffer.
**          If didn't find space, we want to keep the buffer locked while
**          we extend the file.
**      21-apr-98 (stial01)
**          dm1i_allocate() Set DM0L_PHYS_LOCK if extension table (B90677)
**      22-sep-98 (stial01)
**          dm1i_allocate() If row locking, downgrade/convert lock on new page  
**          after mini transaction is done (B92909)
**      18-feb-99 (stial01)
**          dm1i_allocate() pass buf_locked to dm1r_allocate
**      15-Jun-2001 (hanal04) Bug 104980 INGSRV 1473
**          If we need to ignore the TIDP when doing duplicate checking
**          we must perform dm1i_search() for the partial key even
**          if comp_lt is greater than zero because the code for the
**          comp_gt equals zero case assumed the search had been performed.
**          Additionally we must not overwrite the status from
**          ipage_dupcheck() when unfixing dup_data or we will fail
**          to notify the caller that we found a duplicate.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: must dm0pLockBufForUpd when crow_locking
**	    to ensure the Root is locked.
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support:
**	    Use NeedPhysLock(r) to determine physical page lock needs.
*/
DB_STATUS
dm1i_allocate(
DMP_RCB        *rcb,
DMP_PINFO      *pinfo,
DM_TID         *tid,
char           *record,
char	       *crecord,
i4        need,
i4        dupflag,
DB_ERROR       *dberr)
{
    DMP_RCB     *r = rcb;
    DMP_PINFO   newPinfo;
    DMP_TCB     *t = r->rcb_tcb_ptr;
    DMP_DCB     *d = t->tcb_dcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    DM_TID      localtid;
    DB_STATUS   s, unfix_status;
    DB_STATUS	local_status;
    i4		unfix_err_code;
    STATUS   	cl_status;
    CL_ERR_DESC	sys_err;
    i4     local_err_code;                                  
    DM_PAGENO   pageno;
    DM_PAGENO   spacepage;
#define             NO_SPACE_FOUND  0xffffffffL
    i4     i;
    i4     keys_given = t->tcb_keys;
    i4	dup_keys;
    DM_PAGENO	save_data;
    i4	comp_lt = 0;
    i4	comp_gt = 0;
    i4	local_comp_lt;
    i4	local_comp_gt;
    bool	ignore_tidp = FALSE;
    bool	research_bucket;
    ADF_CB      *adf_cb;
    i4	loc_id;
    i4	config_id;
    i4	ovf_loc_id;
    i4	ovf_config_id;
    i4		dm0l_flag = 0;
    DM_PAGENO   ovflpageno;
    bool        in_mini_trans = FALSE;
    LK_LKID      new_lkid;
    DB_ERROR	local_dberr;
    i4		    *err_code = &dberr->err_code;
    LG_LRI	lri;

    CLRDBERR(dberr);

    DMP_PINIT(&newPinfo);
 
    /*  Prepare for ADF key/tuple compare calls. */

    adf_cb = r->rcb_adf_cb;

    /*	Build the key from the record. */

    for (i = 0; i < t->tcb_keys; i++)
	MEcopy(&record[t->tcb_key_atts[i]->offset], 
	    t->tcb_key_atts[i]->length,
	    &r->rcb_s_key_ptr[t->tcb_key_atts[i]->key_offset]);

    /*
    ** If this is a Release 6 unique secondary index, then we need to ignore
    ** the TIDP field when doing duplicate checking.
    */
    keys_given = t->tcb_keys;
    dup_keys = keys_given;
    if ((t->tcb_rel.relstat & TCB_INDEX) &&
	(t->tcb_atts_ptr[t->tcb_rel.relatts].key != 0))
    {
	ignore_tidp = TRUE;
	dup_keys--;
    }

    /*  Drop existing buffer on the floor. */

    if ( pinfo->page )
    {
        s = dm0p_unfix_page(r, DM0P_UNFIX, pinfo, dberr);
        if (s != E_DB_OK)
        {
            /*  Handle error reporting. */

            if (dberr->err_code >= E_DM_INTERNAL)
            {
                uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
		SETDBERR(dberr, 0, E_DM9249_DM1I_ALLOCATE);
            }
            return (E_DB_ERROR);
        }
    }    

    /*
    ** Find page this record indexes to.  Need to include the entire key -
    ** including the TIDP in unique secondary indexes - as we need to find
    ** the real spot at which to insert the record.
    */
    
    s = dm1i_search(r, pinfo, r->rcb_s_key_ptr, keys_given, DM1C_POSITION,
        DM1C_EXACT, &localtid, dberr);

    if (s != E_DB_OK)
    {
	/* Don't leave page fixed in local buffer */
	if ( pinfo->page )
	    s = dm0p_unfix_page(r, DM0P_UNFIX, pinfo, &local_dberr);

	if (dberr->err_code >= E_DM_INTERNAL)
	{
	    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM9246_DM1S_DEALLOCATE);
	}
	return (E_DB_ERROR);
    }

    /*
    ** Until we need to look for space on a page,
    ** we only need to read-share lock the buffers.
    */
    dm0pLockRootForGet(r, pinfo);

    /*
    ** Remember the page found in the initial search.
    */
    save_data = DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, pinfo->page);

    /*
    ** Check for duplicates and space.
    ** Traverse down this page's overflow chain (if there is one), checking
    ** for duplicates (if required) and space (if not found yet).
    **
    ** Call the duplicate checking routine to check duplicates.
    */
    spacepage = NO_SPACE_FOUND;
    for (;;)
    {
        char          *rec_ptr;
        i4       rec_size;

        if (dupflag != DM1C_DUPLICATES)
        {
	    /*
	    ** Check for duplicates on the page.
	    ** This routine will return whether any records were found on
	    ** the page that were LESS_THAN or GREATER_THAN our insert
	    ** key.  This is used below to determine whether we need to	
	    ** search the previous or next isam page for duplicates in the
	    ** unique 2nd index case.
	    */
	    s = ipage_dupcheck(r, dupflag, pinfo, record, dup_keys, 
			    &comp_lt, &comp_gt, dberr);
	    if (DB_FAILURE_MACRO(s))
		break;
	}	

        /*
        **  Check for free space on this page if we aren't just checking for
        **  duplicates and we have yet to find a page with space.
        */

        if (dupflag != DM1C_ONLYDUPCHECK)
        {
            if (spacepage == NO_SPACE_FOUND)
            {
		/* Now we need to read-excl lock the buffer */
		dm0pLockBufForUpd(r, pinfo);

                s = dm1r_allocate(r, pinfo,
			t->tcb_rel.relcomptype == TCB_C_NONE ? record : crecord,
			need, tid, dberr);

		if (s == E_DB_ERROR)
		    break;
                if (s == E_DB_OK)
                {
                    spacepage = 
			DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
				pinfo->page);
                    if (dupflag == DM1C_DUPLICATES)
                        break;
                }
                s = E_DB_OK;
            }
        }

	/* Get ovfl page while the buffer is locked */
	ovflpageno = 
	    DMPP_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, pinfo->page);

        /* If reached last page in chain, quit. */

        if ((pageno = ovflpageno) == 0) 
            break;

	/* If not checking for duplicates and space found quit. */

	if (spacepage != NO_SPACE_FOUND && dupflag == DM1C_DUPLICATES)
	    break;

        /* Otherwise, get next overflow chain, keep checking. */

	/* NB: unfix_page also releases the lock on the buffer */
        s = dm0p_unfix_page(r, DM0P_UNFIX, pinfo, dberr);

        if (s == E_DB_OK)
	{
            s = dm0p_fix_page(r, pageno, DM0P_WRITE | DM0P_NOREADAHEAD,
                pinfo, dberr);
	}
        if (s != E_DB_OK)
            break;

	dm0pLockRootForGet(r, pinfo);

    } /* end for all pages in chain */          


    /*
    ** If we need to ignore the TIDP when doing duplicate checking, then
    ** it is possible that we will need to search the previous or next
    ** isam page for duplicate keys.
    **
    ** Since a key that that excludes the TIDP is actually a partial key,
    ** and since a partial key range can span isam pages, we may need
    ** to look forward or backward for duplicates.
    **
    ** We only need to look backward if the insert key is less than all
    ** entries on the page we just searched, since if there were any entries
    ** on the page less than our insert key, then there can be none equal
    ** to our insert key on any previous page.
    **
    ** Similarly we only need to look forward if the insert key is greater
    ** than all entries on the page we just searched.
    **
    ** The duplicate checking routine called above will keep a count of
    ** the number of entries found less than or greater than our insert key.
    **
    ** Since a duplicate key cannot span 3 pages, we never need to do
    ** duplicate checking both forward AND backward.
    **
    ** As of CA-OpenIngres 2.0, we do not include TIDP in the key of 
    ** UNIQUE indexes. This eliminates the overhead of duplicate 
    ** checking on adjacent leaves. This is necessary for row locking
    ** protocols that exist.
    */

    /* loop executed only once, to break out of ... */
    while (ignore_tidp && (dupflag != DM1C_DUPLICATES) && DB_SUCCESS_MACRO(s))
    {
	DMP_PINFO	dupPinfo;

	DMP_PINIT(&dupPinfo);

	if (t->tcb_rel.relpgtype != TCB_PG_V1)
	{
	    dm0pUnlockBuf(r, pinfo);
	    uleFormat(NULL, E_DM93F5_ROW_LOCK_PROTOCOL, NULL, ULE_LOG, NULL, NULL, 0, NULL, 
			err_code, 0);
	    SETDBERR(dberr, 0, E_DM9249_DM1I_ALLOCATE);
	    return (E_DB_ERROR);
	}

	/*
	** If there were no entries found during duplicate checking that
	** where less than our insert key, then we may need to do duplicate
	** checking on the previous isam page (and chain).
	**
	** Research the isam index using the non-tidp partial key.  If we
	** end up at a different page than we searched before then retry
	** duplicate checking with this page/chain.
	*/

	research_bucket = FALSE;
        if ((comp_lt == 0) || (comp_gt == 0))
        {
	    s = dm1i_search(r, &dupPinfo, r->rcb_s_key_ptr, dup_keys,
		DM1C_POSITION, DM1C_PREV, &localtid, dberr);
	    if (DB_FAILURE_MACRO(s))
		break;
        }

	if (comp_lt == 0)
	{
	    /*
	    ** If found a different data page, then need to retry
	    ** duplicate checking.
	    */
	    if (DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, dupPinfo.page) != 
			save_data)
		research_bucket = TRUE;

	    /*
	    ** Traverse duplicate chain looking for duplicates.
	    */
	    while (research_bucket)
	    {
		s = ipage_dupcheck(r, dupflag, &dupPinfo, record, dup_keys, 
				&local_comp_lt, &local_comp_gt, dberr);
		if (DB_FAILURE_MACRO(s))
		    break;

		pageno = DMPP_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, 
		    dupPinfo.page);
		if (pageno == 0)
		    break;

		s = dm0p_unfix_page(r, DM0P_UNFIX, &dupPinfo, dberr);
		if (DB_FAILURE_MACRO(s))
		    break;
		s = dm0p_fix_page(r, pageno, DM0P_WRITE | DM0P_NOREADAHEAD,
				    &dupPinfo, dberr);
		if (DB_FAILURE_MACRO(s))
		    break;
	    }
	}

	/*
	** If there were no entries found greater than our insert key,
	** then we need to check the next isam bucket for duplicates.
	** We don't need to do this if we have seen above that the insert
	** key spans the previous two buckets since it cannot span three.
	*/
	if ((comp_gt == 0) && (research_bucket == FALSE) && 
	    (DMPP_VPT_GET_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype, dupPinfo.page) != 0))
	{
	    pageno = DMPP_VPT_GET_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype, 
		dupPinfo.page);
	    s = dm0p_unfix_page(r, DM0P_UNFIX, &dupPinfo, dberr);
	    if (DB_FAILURE_MACRO(s))
		break;

	    s = dm0p_fix_page(r, pageno, (DM0P_WRITE | DM0P_NOREADAHEAD), 
			    &dupPinfo, dberr);
	    if (DB_FAILURE_MACRO(s))
		break;

	    /*
	    ** Traverse the overflow chain checking for duplicates.
	    */
	    for (;;)
	    {
		s = ipage_dupcheck(r, dupflag, &dupPinfo, record, dup_keys, 
				&local_comp_lt, &local_comp_gt, dberr);
		if (DB_FAILURE_MACRO(s))
		    break;

		pageno = DMPP_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, 
		    dupPinfo.page);
		if (pageno == 0)
		    break;

		s = dm0p_unfix_page(r, DM0P_UNFIX, &dupPinfo, dberr);
		if (DB_FAILURE_MACRO(s))
		    break;
		s = dm0p_fix_page(r, pageno, DM0P_WRITE | DM0P_NOREADAHEAD,
				    &dupPinfo, dberr);
		if (DB_FAILURE_MACRO(s))
		    break;
	    }
	}

	if (dupPinfo.page)
	{
		/* Note, error checking will be done below */
                /* Don't allow the duplicate checking status to be */
                /* Overwritten unless we fail to unfix the page.   */
                unfix_status = dm0p_unfix_page(r, DM0P_UNFIX, &dupPinfo,
                                &local_dberr);
                if (DB_FAILURE_MACRO(unfix_status))
                {
                    s = unfix_status;
		    *dberr = local_dberr;
                }
	} 

	break;
    }


    /*
    ** Duplicate checking is complete.  Make sure we leave with the
    ** page fixed on which we found space to insert the record.
    **
    ** If there was no space found, then allocate a new overflow chain.
    ** If we are only checking for duplicates, then return with no
    ** pages fixed.
    */


    for (;;)	/* to break out of... */
    {
    	if (s != E_DB_OK)
	    break;

        if (dupflag == DM1C_ONLYDUPCHECK)
        {
	    /* NB: unfix_page also releases the buffer lock */
            s = dm0p_unfix_page(r, DM0P_UNFIX, pinfo, dberr);
            if (s == E_DB_OK)
                return (s);
        }
        else if (spacepage != NO_SPACE_FOUND)
        {
            if (spacepage != DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, 
		pinfo->page))
            {
                s = dm0p_unfix_page(r, DM0P_UNFIX, pinfo, dberr);

                if (s == E_DB_OK)
		{
                    s = dm0p_fix_page(r, spacepage,
                        DM0P_WRITE | DM0P_NOREADAHEAD, pinfo, dberr);
		}
                if (s != E_DB_OK)
                    return (s);
            }
	    else
	        dm0pUnlockBuf(r, pinfo);
        }
        else if (spacepage == NO_SPACE_FOUND)
        {
            LG_LSN	    lsn, bm_lsn;
	    i4	    dm1p_flag = 0;

	    /*
	    ** No space was found on the current page - we must allocate
	    ** a new page and link it onto the isam overflow chain.
	    **
	    ** Allocations for system catalogs are always done within Mini
	    ** Transactions since the page locks will be released as soon as
	    ** the current update is complete.
            */

	    dm0pLockBufForUpd(r, pinfo);

	    if ( row_locking(r) || crow_locking(r) || NeedPhysLock(r) )
            {
		if ( r->rcb_logging )
		{
		    s = dm0l_bm(r, &bm_lsn, dberr);
		    if (s != E_DB_OK)
			break;
		    in_mini_trans = TRUE;
		}

		/*
		** Set flags to allocate call to indicate that the action is
		** part of a mini transaction.
		*/
                dm1p_flag |= DM1P_MINI_TRAN;

		if ( NeedPhysLock(r) )
                    dm1p_flag |= DM1P_PHYSICAL;
            }

	    /*
	    ** Call allocation code to get a free page from the free maps.
	    ** This will possibly trigger a File Extend as well.
	    **
	    ** The newpage's buffer might be pinned by dm1p_getfree.
	    ** The pin will be removed by dm0pUnlockBuf().
	    */
            s = dm1p_getfree(r, dm1p_flag, &newPinfo, dberr);
            if (s != E_DB_OK)
                break;

	    /* Save lockid for new page */
	    STRUCT_ASSIGN_MACRO(r->rcb_fix_lkid, new_lkid);

	    /* Inform buffer manager of new page's type */
	    dm0p_pagetype(tbio, newPinfo.page, r->rcb_log_id, DMPP_DATA);

	    /*
	    ** Online Backup Protocols: Check if BIs must be logged first.
	    */
	    if (d->dcb_status & DCB_S_BACKUP)
	    {
	        s = dm0p_before_image_check(r, pinfo->page, dberr);
	        if (s != E_DB_OK)
		    break;
	        s = dm0p_before_image_check(r, newPinfo.page, dberr);
	        if (s != E_DB_OK)
		    break;
	    }

	    /*
	    ** Get information on the locations of the updates for the 
	    ** log call below.
	    ** If row locking, we can look at page_page when the buffer is
	    ** not locked, since it is only set when the page is formatted
	    */
	    loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count,
	    	(i4) DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, 
		pinfo->page));
	    config_id = tbio->tbio_location_array[loc_id].loc_config_id;

	    ovf_loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count,
 		(i4) DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, 
		newPinfo.page));
            ovf_config_id = 
		tbio->tbio_location_array[ovf_loc_id].loc_config_id;

	    /*
	    ** Log operation of adding new page on the overflow chain.
	    ** The Log Address of the OVERFLOW record is recorded on both
	    ** the new and old data pages.
	    */
	    if (r->rcb_logging)
	    {
		/* Reserve space for overflow and its clr */
		cl_status = LGreserve(0, r->rcb_log_id, 2, 
		    sizeof(DM0L_OVFL) * 2, &sys_err);
		if (cl_status != OK)
		{
		    (VOID) uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		    (VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			err_code, 1, 0, r->rcb_log_id);
		    SETDBERR(dberr, 0, E_DM9249_DM1I_ALLOCATE);
		    s = E_DB_ERROR;
		    break;
		}

		/*
		** Mutex the buffers so nobody can look at them while they are 
		** changing.
		*/
		dm0pMutex(tbio, (i4)0, r->rcb_lk_id, pinfo);
		dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &newPinfo);


		dm0l_flag = (is_journal(r) ? DM0L_JOURNAL : 0);

		if ( r->rcb_state & RCB_IN_MINI_TRAN )
		    dm0l_flag |= DM0L_MINI;

		/* 
		** We use temporary physical locks for catalogs and extension
		** tables. The same protocol must be observed during recovery.
		*/
		if ( NeedPhysLock(r) )
		    dm0l_flag |= DM0L_PHYS_LOCK;
                else if (row_locking(r))
                    dm0l_flag |= DM0L_ROW_LOCK;
		else if ( crow_locking(r) )
		    dm0l_flag |= DM0L_CROW_LOCK;

		/* Get CR info from datapage, pass to dm0l_ovfl */
		DMPP_VPT_GET_PAGE_LRI_MACRO(t->tcb_rel.relpgtype, pinfo->page, &lri);

		s = dm0l_ovfl(r->rcb_log_id, dm0l_flag, FALSE,
		    &t->tcb_rel.reltid, 
		    &t->tcb_rel.relid, t->tcb_relid_len,
		    &t->tcb_rel.relowner, t->tcb_relowner_len,
		    t->tcb_rel.relpgtype,
		    t->tcb_rel.relpgsize,
		    t->tcb_rel.relloccount, config_id, ovf_config_id,
		    DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, newPinfo.page), 
		    DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, pinfo->page),
		    DMPP_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, pinfo->page), 
		    DMPP_VPT_GET_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype, pinfo->page),
		    (LG_LSN *)NULL, &lri, dberr);
		if (s != E_DB_OK)
		{
		    dm0pUnmutex(tbio, 0, r->rcb_lk_id, pinfo);
		    dm0pUnmutex(tbio, 0, r->rcb_lk_id, &newPinfo);
		    break;
		}

		/* Store log info about this change on datapage */
		DMPP_VPT_SET_PAGE_LRI_MACRO(t->tcb_rel.relpgtype, pinfo->page, &lri);
		/*
		** "newpage" is new and has no page update history, so just 
		** record the ovfl lsn, leaving the rest of the LRI information
		** on the page zero.
		*/
		DMPP_VPT_SET_PAGE_LOG_ADDR_MACRO(t->tcb_rel.relpgtype,
					newPinfo.page, lri.lri_lsn);
	    }
	    else
	    {
		dm0pMutex(tbio, (i4)0, r->rcb_lk_id, pinfo);
		dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &newPinfo);
	    }

            /*  Mark new page as on same overflow chain. */

            DMPP_VPT_SET_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype, newPinfo.page, 
		DMPP_VPT_GET_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype, pinfo->page));
	    DMPP_VPT_SET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, newPinfo.page,
		DMPP_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, pinfo->page));
            DMPP_VPT_SET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, newPinfo.page, 
		(DMPP_MODIFY | DMPP_OVFL));

            /*  Link old page to new page. */

            DMPP_VPT_SET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, pinfo->page, 
		DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, newPinfo.page));
            DMPP_VPT_SET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype,pinfo->page,DMPP_MODIFY);

	    /*
	    ** Unmutex the buffers after completing the updates.
	    */
	    dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, pinfo);
	    dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, &newPinfo);

	    /*
	    ** If this is a system catalog allocation, then complete the
	    ** Mini Transaction used for the allocate.  After this, the
	    ** new page becomes a permanent addition to the table.
	    */
	    if (in_mini_trans)
            {
		s = dm0l_em(r, &bm_lsn, &lsn, dberr);
                if (s != E_DB_OK)
		break;
            }

	    spacepage = 
		    DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, newPinfo.page);

	    /*
	    ** Now that the mini transaction is complete,
	    ** it is safe to unlock the buffer
	    */

            /*  Unfix/unlock oldpage and replace with new page. */

            s = dm0p_unfix_page(r, DM0P_UNFIX, pinfo, dberr);
            if (s != E_DB_OK)
                break;

	    *pinfo = newPinfo;
	    DMP_PINIT(&newPinfo);

	    /*
	    ** Allocate space on new page while we still hold X lock 
	    ** and pin on newpage
	    */
	    s = dm1r_allocate(r, pinfo,
			t->tcb_rel.relcomptype == TCB_C_NONE ? record : crecord,
			need, tid, dberr);

	    /*
	    ** Even if allocate failed, do the lock conversion below in
	    ** case we need to rollback. 
	    **
	    ** Note dm1r_lk_convert will also remove the pin from newpage.
	    */
	    if ( row_locking(r) || crow_locking(r) )
	    {
		local_status = dm1r_lk_convert(r, 
				DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, pinfo->page),
				pinfo,
				&new_lkid, &local_dberr);

		if (local_status != E_DB_OK)
		{
		    if (s == E_DB_OK)
		    {
			s = local_status;
			*dberr = local_dberr;
		    }
		    else
		    {
			uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL,
				    &local_err_code, 0);
		    }
		}
	    }

	    if (s != E_DB_OK)
		break;
        }

        return (s);
    }

    /* Don't leave page fixed in local buffer */

    if (dberr->err_code == E_DMF012_ADT_FAIL)
	dmd_check(E_DMF012_ADT_FAIL);

    /* NB: unfix_page also releases buffer lock */
    if (pinfo->page)
        s = dm0p_unfix_page(r, DM0P_UNFIX, pinfo, &local_dberr);
    if (newPinfo.page)
        s = dm0p_unfix_page(r, DM0P_UNFIX, &newPinfo, &local_dberr);

    /*  Handle error reporting. */

    if (dberr->err_code >= E_DM_INTERNAL)
    {
        uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9249_DM1I_ALLOCATE);
    }
    return (E_DB_ERROR);
}

/*{
** Name: dm1i_search - Sets the scan limits for a ISAM table.
**
** Description:
**      This routine sets the serach limits for a scan of a ISAM table.
**      The low and high tids for the scan are place in the RCB.
**      These are then used to determine which records to retrieve when 
**      get next is called.  This routine will leave the data page fixed.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      key                             Pointer to the key.
**      keys_given                      Number of attributes in key.
**      mode                            Must be set to indicate type of
**                                      position desired.
**      direction                       A valindicating the direction of
**                                      the search.  Must be DM1C_PREV for
**                                      first or DM1C_NEXT for last or 
**                                      DM1C_EXACT for page position.
**
** Outputs:
**      tid                             A pointer to an area to return
**                                      the tid meeting the search criteria.
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
**                                      Error codes returned are:
**                                      E_DM0055_NONEXT, E_DM003C_BAD_TID,
**                                      E_DB0044_DELETED_TID.
**                    
**      Returns:
**
**          E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      06-jan-86 (jennifer)
**          Converted for Jupiter.
**      02-apr-87 (ac)
**          Added read/nolock support.
**      24-Apr-89 (anton)
**          Added local collation support
**	19-nov-90 (rogerk)
**	    Changed to use the rcb's adf control block rather than allocating
**	    local adf cb's off of the stack.  This was done as part of the DBMS
**	    timezone changes.
**      18-oct-1991 (mikem)
**          Fix 6.4->6.5 merge problem in dm1i_search() caused when rogerk's
**	    nov and dec 90 changes were merged into 6.5 code.  The problem
**	    was that a pointer to a (* ADB_CB) was being passed to 
**	    adt_kkcmp() rather than just the pointer itself.
**	08_jun_1992 (kwatts)
**	    6.5 MPF change. Replaced in-line index search with an accessor call
**      10-apl-1995 (chech02) Bug# 64830 (merged the fixes from 6.4 by nick)
**          Unfix page pointed to by data in dm1i_search() to avoid
**          overwriting when fixing the result page. #64830
**	01-sep-1995 (cohmi01)
**	    Add support for DM1C_LAST request on an ISAM table, for use
**	    when executing MAX aggregate. Unlike btree, here DM1C_LAST means
**	    'last mainpg plus its overflows', all of which must be scanned.
**	23-may-96 (nick)
**	    Alter actions for fixes of the ISAM index pages ; this had been
**	    changed from DM0P_RPHYS to (DM0P_READ | DM0P_NOLOCK) as part of
**	    the OpenIngres development.  This caused update RCBs to fix the
**	    page for write yet with no page locking resulting in server 
**	    crashes in dm0p_force_pages() as multiple fixes for write
**	    break transaction ownership of the page.  Fixed by changing 
**	    DM0P_READ back to DM0P_RPHYS ( which ensures the RCB state 
**	    is ignored ). I've left the DM0P_NOLOCK in as this does give a 
**	    performance benefit and there is no reason to lock these read-only
**	    pages but see warnings below. #74380
**	06-may-1996 (thaju02)
**	    New Page Format Support:
**		Change page header references to use macros.
**      12-dec-96 (dilma04)
**          If phantom protection is needed, set DM0P_PHANPRO fix action.
**      06-jan-97 (dilma04)
**          DM0P_PHANPRO must be set only if mode != DM1C_POSITION. Otherwise,
**          X lock would be taken on the data page instead of IX for insert 
**          operation with row locking (bug 79816).
**      27-jan-97 (dilma04)
**          Search to update secondary index should be done without phantom
**          protection. This fixes bug 80114.    
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          Use macro to check for serializable transaction.
**      09-feb-99 (stial01)
**          dm1i_search() Pass relcmptlvl to dmpp_ixsearch
*/
DB_STATUS
dm1i_search(
DMP_RCB         *rcb,
DMP_PINFO       *pinfo,
char            *key,
i4         keys_given,
i4         mode,
i4         direction,
DM_TID          *tid,
DB_ERROR        *dberr)
{
    DMP_RCB             *r = rcb;
    DMP_TCB		*t = r->rcb_tcb_ptr;
    DMP_PINFO           indexPinfo;
    DMPP_PAGE           *index;
    i4             fix_action;
    DB_STATUS           s;
    i4                  partialkey;
    i4                  top;
    i4                  bottom;
    DB_ATTS		**keyatts = r->rcb_tcb_ptr->tcb_key_atts;
    i4             index_level;
    i4             compare;
    ADF_CB              *adf_cb;
    i4             local_err_code;
    DB_ERROR		local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    if (mode == DM1C_EXTREME || (keys_given == 0 && mode != DM1C_LAST))
    {
        tid->tid_i4 = -1;
        if (direction == DM1C_PREV)
        {   
            tid->tid_tid.tid_page = 0;
            tid->tid_tid.tid_line = DM_TIDEOF;
        }
        return (E_DB_OK);
    }

    /* 
    **  Find main page this record should index to. 
    **  Searches each level of the dirctory for the lowest (highest) page
    **  on which the key could occur if mode is < (>) 0.
    **  Note that the keys tell the minimum value on that page.
    */

    partialkey = FALSE;
    if (keys_given != r->rcb_tcb_ptr->tcb_keys)
        partialkey = TRUE;

    /* The actual ISAM directory search must be performed */

    adf_cb = r->rcb_adf_cb;
    tid->tid_tid.tid_page = r->rcb_tcb_ptr->tcb_rel.relprim;

    if (pinfo && pinfo->page)
    {
# ifdef XDEBUG
	TRdisplay("Data page fixed in call to dm1i_search(), unfixing ...\n");
# endif
	s = dm0p_unfix_page(r, DM0P_UNFIX, pinfo, dberr);
	if (s)
	    return(s);
    }
    
    do
    {
	/*
	** N.B.  The use of DM0P_NOLOCK here is dubious ( as a window
	** is left for another thread to fix this page for write from 
	** elsewhere in the server and screw this fixer ).  I can't find
	** anywhere that this actually happens ( as the ISAM index will
	** only get changed on a modify ) and leaving it in gives a
	** performance gain but it's worth noting here as something
	** which is worth removing if spurious ISAM related problems arise.
	*/
        s = dm0p_fix_page(r, (DM_PAGENO) tid->tid_tid.tid_page, 
        	(DM0P_RPHYS | DM0P_NOREADAHEAD | DM0P_NOLOCK), 
		&indexPinfo, dberr);
        if (s != E_DB_OK)
            break;

	/* if positioning at last mainpage, use highkey entry on index page */
	if (mode == DM1C_LAST && direction == DM1C_NEXT)
	{
	    tid->tid_tid.tid_page = 
		DMPP_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, indexPinfo.page) + 
		DMPP_VPT_GET_PAGE_NEXT_LINE_MACRO(t->tcb_rel.relpgtype, 
		indexPinfo.page) - 1;
	}
	else
	{
	    /* Binary search for key. */
        
	    tid->tid_tid.tid_page = 
		DMPP_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, indexPinfo.page) + 
		(*t->tcb_acc_plv->dmpp_ixsearch)(t->tcb_rel.relpgtype,
		t->tcb_rel.relpgsize, indexPinfo.page,
		t->tcb_key_atts, keys_given, key,
		partialkey, direction, t->tcb_rel.relcmptlvl, adf_cb);
	}
	index_level = DMPP_VPT_GET_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype, indexPinfo.page);

        s = dm0p_unfix_page(r, DM0P_UNFIX, &indexPinfo, dberr);
        if (s != E_DB_OK)
            break;
    } while (index_level);  

    if (s == E_DB_OK)
    {
        /* Fix the page if it is the low one, do not fix the high, unless  */
	/* positioning past the high entry for a pseudo-backwards scan */

        tid->tid_tid.tid_line = DM_TIDEOF;
        if (direction == DM1C_PREV || direction == DM1C_EXACT ||
	    (mode == DM1C_LAST && direction == DM1C_NEXT))
        {
            fix_action = DM0P_READ;
	    if (rcb->rcb_access_mode == RCB_A_WRITE)
		fix_action = DM0P_WRITE;
            if (row_locking(r) && serializable(r) &&
                   mode != DM1C_POSITION && mode != DM1C_FIND_SI_UPD &&
		   direction != DM1C_EXACT)
                fix_action |= DM0P_PHANPRO;

            s = dm0p_fix_page(r, (DM_PAGENO)tid->tid_tid.tid_page, fix_action,
                  pinfo, dberr);
            if (s == E_DB_OK)
                return (s);
        }
        else
            return (E_DB_OK);
    }

    if (indexPinfo.page)
        s = dm0p_unfix_page(r, DM0P_UNFIX, &indexPinfo, &local_dberr);

    /*  Handle error reporting. */

    if (dberr->err_code >= E_DM_INTERNAL)
    {
        uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM924A_DM1I_SEARCH);
    }
    return (E_DB_ERROR);
}

/*{
** Name: ipage_dupcheck - check current page for duplicates. 
**
** Description:
**	This routine checks the current datapage for any entries duplicating
**	the insert record passed in.
**
**	It checks for both duplicate key and duplicate row violations.
**
**	It returns whether any records were found which were greater than
**	or less than the insert record.  This information is used to
**	optimize duplicate checking in dm1i_allocate.
**
** Inputs:
**      rcb                             Pointer to record access context.
**	datapage			Page on which to search.
**      record                          Pointer to record to insert.
**	keys_given			Number of keys to use.
**
** Outputs:
**	comp_lt				Set to non-zero if at least one record
**					found which compares less than the
**					insert record.
**	comp_gt				Set to non-zero if at least one record
**					found which compares greater than the
**					insert record.
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
**                                      Error codes returned are:
**					    E_DM0046_DUPLICATE_RECORD
**					    E_DM0045_DUPLICATE_KEY
**      
**	Returns:
**	    E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**	18-dec-90 (rogerk)
**	    Written with code from dm1i_allocate.
**	08-Jun-1992 (kwatts)
**	    6.5 MPF project. Replaced dmpp_get_offset_macro,
**	    and dm1c_get calls with accessor calls. Note the get_offset 
**	    goes completely, instead we check the response from the get
**	    accessor.
**	14-oct-1992 (jnash)
**	    Reduced logging project.
**	      - Remove unused param's on dmpp_get calls.
**	      - Move compression out of dm1c layer, call tlv's here.
**	06-mar-1996 (stial01 for bryanp)
**	    Don't allocate tuple buffers on the stack.
**      06-mar-1996 (stial01)
**          ipage_dupcheck() Only allocate tuple buffer if necessary.
**	06-may-1996 (thaju02)
**	    New Page Format Support:
**		Change page header references to use macros.
**	20-may-1996 (ramra01)
**	    Added DMPP_TUPLE_INFO argument to the get accessor routine
**      03-june-1996 (stial01)
**          Use DMPP_INIT_TUPLE_INFO_MACRO to init DMPP_TUPLE_INFO
**      18-jul-1996 (ramra01 for bryanp)
**          Alter table support: allow for row versions, pass version number 
**          to dmpp_uncompress or call dm1r_cvt_row as necessary.
**	13-sep-1996 (canor01)
**	    Add rcb_tupbuf to dmpp_uncompress call to pass temporary buffer
**	    for uncompression.  Use the temporary buffer for uncompression
**	    when available.
**      10-mar-97 (stial01)
**          Pass relwid to dm0m_tballoc()
**	11-aug-97 (nanpr01)
**	    Add the t->tcb_comp_atts_count with relwid in dm0m_tballoc call.
**      05-may-1999 (nanpr01,stial01)
**          ipage_dupcheck() Key value locks acquired for dupcheck are no 
**          longer held for duration of transaction. While duplicate checking,
**          request/wait for row lock if uncommitted duplicate key is found.
**	13-aug-99 (stephenb)
**	    alter uncompress calls to new proto.
**      28-oct-99 (stial01)
**          If lock row fails don't break with buf_locked set, buf not locked
**	29-Dec-2003 (jenjo02)
**	    Added support for Partitioned tables, Global indexes.
**      16-mar-2004 (gupsh01)
**          Modified dm1r_cvt_row to include adf control block in parameter
**          list.
**      11-Jan-2009 (coomi01) b123128
**          When looking for dups, do not compare incoming target record
**          with original placement.
*/
static DB_STATUS
ipage_dupcheck(
DMP_RCB	    *rcb,
i4	    dupflag,
DMP_PINFO   *pinfo,
char	    *record,
i4	    keys_given,
i4	    *comp_lt,
i4	    *comp_gt,
DB_ERROR    *dberr)
{
    DMP_RCB	*r = rcb;
    DMP_TCB	*t = r->rcb_tcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    ADF_CB      *adf_cb;
    DM_TID      localtid;
    DB_STATUS   s = E_DB_OK, get_status, local_status;
    i4	i;
    i4     compare;
    i4	rec_size;
    char	*record2 = NULL;
    i4		row_version = 0;	
    u_i4	row_low_tran = 0;
    u_i2	row_lg_id;
    char	*rec_ptr;
    i4		new_lock;
    i4		flags;
    DM_TID      tid_to_lock;
    DM_TID8	tid8;
    bool	did_lock;
    bool        must_lock = FALSE;
    LK_LOCK_KEY save_lock_key;
    DB_ERROR	local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);
 
    if (t->tcb_data_rac.compression_type != TCB_C_NONE && r->rcb_tupbuf == NULL)
    {
	r->rcb_tupbuf = dm0m_tballoc((t->tcb_rel.relwid +
					t->tcb_data_rac.worstcase_expansion));
        if (r->rcb_tupbuf == NULL)
        {
	    SETDBERR(dberr, 0, E_DM923D_DM0M_NOMORE);
            return (E_DB_ERROR);
        }
    }
    record2 = r->rcb_tupbuf;

    adf_cb = r->rcb_adf_cb;
    localtid.tid_tid.tid_page = DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, 
					pinfo->page);
    for (i = 0; i < ((i4)DMPP_VPT_GET_PAGE_NEXT_LINE_MACRO(t->tcb_rel.relpgtype,
	pinfo->page)); i++)
    {
	did_lock = FALSE;
	if ((row_locking(r) || crow_locking(r)) && must_lock)
	{
	    /* rechecking an uncommitted row, wait for the lock */
	    dm0pUnlockBuf(r, pinfo);

	    if ( crow_locking(r) )
	    {
	        s = dm1r_crow_lock(r, LK_PHYSICAL, &tid_to_lock, NULL, dberr);
	    }
	    else
	    {
		/* don't clear lock coupling in rcb */
		flags = DM1R_ALLOC_TID | DM1R_LK_PHYSICAL; 
		s = dm1r_lock_row(r, flags, &tid_to_lock,
				    &new_lock, &save_lock_key, dberr);
	    }

	    dm0pLockRootForGet(r, pinfo);

	    if ( s != E_DB_OK )
	        break;

	    if ( row_locking(r) )
	    {
		/* Check status after relocking the buffer... */
		_VOID_ dm1r_unlock_row(r, &save_lock_key, &local_dberr);
	    }
	    must_lock = FALSE;
	    did_lock = TRUE;
	}

        localtid.tid_tid.tid_line = i;
	rec_size = t->tcb_rel.relwid;
	s = (*t->tcb_acc_plv->dmpp_get)(t->tcb_rel.relpgtype,
		t->tcb_rel.relpgsize, pinfo->page, &localtid, &rec_size,
		&rec_ptr, &row_version, &row_low_tran, 
		&row_lg_id, (DMPP_SEG_HDR *)NULL);
	get_status = s;

	if (s == E_DB_WARN)
	{
	    s = E_DB_OK;
	    if (!rec_ptr ||
		    DMPP_SKIP_DELETED_ROW_MACRO(r, pinfo->page, row_lg_id, row_low_tran))
		continue;
	}

	/* Additional processing if compressed, altered, or segmented */
	if (s == E_DB_OK && 
	    (t->tcb_data_rac.compression_type != TCB_C_NONE || 
	    row_version != t->tcb_rel.relversion ||
	    t->tcb_seg_rows))
	{
	    s = dm1c_get(r, pinfo->page, &localtid, record2, dberr);
	    rec_ptr = record2;
	}

	if (s == E_DB_ERROR)
	{
	    /*
	    ** DM1C_GET returned an error - this indicates that
	    ** something is wrong with the tuple at the current tid.
	    */
	    uleFormat(NULL, E_DM938A_BAD_DATA_ROW, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 4,
		    sizeof(DB_DB_NAME), &r->rcb_tcb_ptr->tcb_dcb_ptr->dcb_name,
		    sizeof(DB_TAB_NAME), &r->rcb_tcb_ptr->tcb_rel.relid,
		    sizeof(DB_OWN_NAME), &r->rcb_tcb_ptr->tcb_rel.relowner,
		    0, localtid.tid_i4);
	    SETDBERR(dberr, 0, E_DM938B_INCONSISTENT_ROW);
	    break;
	}

	/* If dup row checking (base tables only), we may need lock */
	if ((t->tcb_rel.relstat & TCB_UNIQUE) == 0
	    && DMPP_DUPCHECK_NEED_ROW_LOCK_MACRO(r, pinfo->page, row_lg_id, row_low_tran)
	    && !did_lock)
	{
	    s = adt_ktktcmp(adf_cb, (i4)t->tcb_keys,
			t->tcb_key_atts, record, rec_ptr, &compare);
	    /* Don't lock or dup row check if key doesn't match */
	    if (s == E_DB_OK && compare != 0)
		continue;
	    tid_to_lock.tid_i4 = localtid.tid_i4;
	    must_lock = TRUE;
	    i--;
	    continue; /* recheck the row */
	}

	if ((t->tcb_rel.relstat & TCB_UNIQUE) == 0)
	{
	    /*
	    ** Bug : 123128
	    ** On update, detect compare with outgoing record, and skip
	    */
	    if (
		(r->rcb_state & RCB_ROW_UPDATED)   &&
		(pinfo->page == r->rcb_data.page) && 
		(localtid.tid_tid.tid_line == r->rcb_currenttid.tid_tid.tid_line ) &&
		(localtid.tid_tid.tid_page == r->rcb_currenttid.tid_tid.tid_page ) )
	    {
		continue;
	    }

	    s = adt_tupcmp(adf_cb, (i4)t->tcb_rel.relatts, t->tcb_atts_ptr, 
				record, rec_ptr, (i4 *)&compare);
	    if (s == E_DB_OK)
	    {
		if (compare == 0)
		{
		    s = E_DB_ERROR;
		    SETDBERR(dberr, 0, E_DM0046_DUPLICATE_RECORD);
		    break;
		}
		continue;
	    }
	}
	else
	{
	    s = adt_ktktcmp(adf_cb, keys_given, t->tcb_key_atts, record, 
				rec_ptr, (i4 *)&compare);
	    if (s == E_DB_OK)
	    {
		if (compare < 0)
		{
		    /* Found record > insert record */
		    (*comp_gt)++;
		    continue;
		}
		else if (compare > 0)
		{
		    /* Found record < insert record */
		    (*comp_lt)++;
		    continue;
		}

		/* Same key */
		if (DMPP_DUPCHECK_NEED_ROW_LOCK_MACRO(r, pinfo->page, row_lg_id, row_low_tran)
		    && !did_lock)
		{
		    if (t->tcb_rel.relstat & TCB_INDEX)
		    {
			/* Extract the TID out of the secondary index */
			if ( t->tcb_rel.relstat2 & TCB2_GLOBAL_INDEX )
			{
			    MEcopy((char *)(rec_ptr + rec_size - sizeof(DM_TID8)),
				sizeof(DM_TID8), &tid8);
			    tid_to_lock.tid_i4 = tid8.tid_i4.tid;
			    /* dm1r_lock_row needs this */
			    r->rcb_partno = tid8.tid.tid_partno;
			}
			else
			    MEcopy((char *)(rec_ptr + rec_size - sizeof(DM_TID)),
				sizeof(DM_TID), &tid_to_lock);
		    }
		    else
			tid_to_lock.tid_i4 = localtid.tid_i4;

		    must_lock = TRUE;
		    i--;
		    continue; /* recheck the row */
		}
		else if (get_status == E_DB_WARN)
		    continue;

		s = E_DB_ERROR;
		SETDBERR(dberr, 0, E_DM0045_DUPLICATE_KEY);
		break;
	    }
	}		    
	SETDBERR(dberr, 0, E_DMF012_ADT_FAIL);
	break;
    }

    return (s);
}
