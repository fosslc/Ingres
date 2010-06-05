/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <me.h>
#include    <pc.h>
#include    <sr.h>
#include    <st.h>
#include    <tm.h>
#include    <tr.h>
#include    <bt.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <adf.h>
#include    <ulf.h>
#include    <sxf.h>
#include    <dmf.h>
#include    <dmacb.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmucb.h>
#include    <dmxcb.h>
#include    <dm.h>
#include    <dml.h>
#include    <dm0m.h>
#include    <dm0s.h>
#include    <dmp.h>
#include    <dm2f.h>
#include    <dm2r.h>
#include    <dm2t.h>
#include    <dm0c.h>
#include    <dm1b.h>
#include    <dmpp.h>
#include    <dm1p.h>
#include    <dm0l.h>
#include    <dm0p.h>
#include    <dmse.h>
#include    <dm1u.h>
#include    <dm1c.h>
#include    <dm1cx.h>
#include    <dm2umct.h>
#include    <dm2umxcb.h>
#include    <dm2u.h>
#include    <dmftrace.h>
#include    <dma.h>
#include    <cm.h>
#include    <cui.h>

/**
**
**  Name: DM2UATBL.C - Alter table utility operation.
**
**  Description:
**      This file contains routines that perform the alter table
**	functionality.
**	    dm2u_atable	- Modify a permanent table.
**
**  History:
**	01-sep-1995 (athda01)
**	    Created.
**	14-nov-1996 (nanpr01)
**	    Bug 79062- causes iiattribute corruption.
**	26-dec-1996 (nanpr01)
**	    Bug 79875 79877 : relidxcount change getting lost. We read
**	    iirelation record here. We also read it in dm2u_destroy and 
**	    modify there. Then after we come back to this routine we 
**	    write back the old iirelation record on top of the modified
**	    iirelation record.
**	30-dec-1996 (nanpr01)
**	    yet another bug with alter table : table level recovery 
**	    must be disallowed after the table is altered invalidating
**	    all the table checkpoints.
**      15-aug-97 (stephenb)
**          add DML_SCB parameter to dm2t_open() call.
**	19-apr-1999 (somsa01)
**	    In si_cascade(), refined the way we delete the indices. Store
**	    them in a linked list, close purge the base table, then walk the
**	    linked list destroying the indices one by one. Also, removed
**	    unneeded variables.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	24-may-1999 (somsa01)
**	    Since we are close-purging the base table in si_cascade(),
**	    update the relation record with the proper row and page counts
**	    before dm2u_atable() updates iirelation with old information.
**      03-aug-1999 (stial01)
**          dm2u_atable() Create etab tables for any peripheral atts
**      10-sep-1999 (stial01)
**          dm2u_atable() etab page size will be determined in dmpe
**      12-jan-2000 (stial01)
**          Set TCB2_BSWAP in relstat2 for tables having etabs (Sir 99987)
**	09-jun-2000 (gupsh01)
**	    Corrected the pointer assignment to the link list. The new element
**	    was not being correctly added to the link list. Also added checking
**	    for duplicate elements as we are no longer deleting the index before
**	    calling dm2r_get.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to dma_table_access() if neither
**	    C2 nor B1SECURE.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      25-jun-2004 (stial01)
**          Allow alter table if DM_PG_V5
**      04-oct-2004 (stial01)
**          Enable alter table if DM_PG_V5
**	07-Apr-2005 (thaju02)
**	    Disallow alter column from nullable to non-nullable.(B114021)
**	    Disallow alter column if column is base table key column
**	    or part of a secondary index (B114269)
**	09-May-2005 (thaju02)
**	    Change sizediff to signed.
**	02-Jun-2005 (thaju02)
**	    For alter column, update iiindex idom entries. (B114611)
**      08-jul-2005 (stial01)
**          Disallow alter column if new row width exceeds maximum (b114817)
**      05-aug-2005 (stial01)
**          Init sizediff of altered column before updating any atts (b115002)
**	28-Feb-2008 (kschendel) SIR 122739
**	    Minor changes to operate with the new rowaccessor scheme.
**	11-Apr-2008 (kschendel)
**	    dm2r position call updated, fix here.
**	23-Oct-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2t_?, dm2r_? functions converted to DB_ERROR *
**      10-sep-2009 (stial01)
**          Don't use string compare for relcmptlvl comparisons
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/

/*
** Forward declarations of static functions:
*/
static DB_STATUS    si_drop_column_check(
			DMP_TCB		    *t,
			DMF_ATTR_ENTRY	    **attribute,
			i4		    cascade,
			DB_ERROR	    *dberr);

static	DB_STATUS   si_cascade(
			DML_XCB		    *xcb,
			DMP_RCB		    **base_rcb,
			DMP_TCB		    *t,
			DMF_ATTR_ENTRY	    **attribute,
			DMP_RELATION	    *relrecord,
			i4		    db_lockmode,
			i4		    journal_flag,
			i4		    *deleted_index,
			DB_ERROR	    *dberr);
static  DB_STATUS   si_adjust_colmaps(
			DMP_DCB		    *dcb,
			DML_XCB		    *xcb,
			DMP_RELATION	    *relrecord,
			DB_TAB_ID	    *table_id,
			i4		    attid,
			i4		    db_lockmode,
			i4		    journal_flag,
			DB_ERROR	    *dberr);
static DB_STATUS    pt_adddrop_adjust(
			DML_XCB		*xcb,
			DMP_DCB		*dcb,
			DMP_RCB		*rel_rcbp,
			DM2R_KEY_DESC	*key_desc,
			DMP_RELATION	*relrecp,
			i4		db_lockmode,
			i2		dropped_col,
			DB_ERROR	    *dberr);


/*
** Name: dm2u_atable - Alter a table's column's structure.
**
** Description:
**	This routine is called from dmu_atable() to alter a table's structure.
**	Currently supported is the ability to either add or drop a column.
**	
**	As part of adding or deleting a column this routine performs
**	the following work:
**	
**	- stall operation until online checkpoint is finished.
**	  This may not be necessary, but there doesn't seem to be any reason
**	  to allow alters during online checkpoint.  If this becomes an issue
**	  then it may be possible to relax this restriction.
**	
**	- Obtain an exclusive table lock and open the table.
**	
**	- insure same security restrictions as are currently in effect
**	  for the "modify command" (ie. Make sure that the person altering 
**	  the table has the exact same label as the person who created it).
**	
**	- disallow operation on system catalogs.  It may be possible in the
**	  future to allow alter's on "non-core" catalogs.  The first 
**	  implementation will disallow alter on all system catalogs.
**	  The next implementation may relax this and allow alter's of 
**	  extended system catalogs; but mainline code may need to be changed
**	  to handle dynamic changing of these catalogs.
**	
**	- insure that dropped column is not part of any index.   If the 
**	  cascade option has been specified then the index will be dropped,
**	  otherwise an error will be returned.
**	
**	- if ADD column
**	     - add the tuple to iiattribute if there is room (ie. less than
**	       DB_MAX_COLS columns).
**	     - Update the iirelation tuple to reflect the new attribute
**	       count, and bump the timestamp (relstamp12).
**	- if DROP column
**	     - update iiattribute tuple to reflect dropped status.
**	     - update other iiattribute tuples to reflect new offsets.
**	     - Update the iirelation tuple to reflect the new attribute
**	       count, and bump the timestamp (relstamp12).
**	
**	- Purge TCB and let it get automatically updated by next open.
**	- Other cached information throughout the server will be invalidated
**	  by the updated timestamp, in the same manner as when a modify
**	  is performed.
**	             
** Inputs:
**	dcb                             The database id. 
**	xcb                             The transaction id.
**	tbl_id                          The internal table id of the base table 
**					to be altered.
**	operation                       DMU_C_ADD_ALTER, DMU_C_DROP_ALTER.
**	cascade                         True if a drop operation and the
**					"cascade" operation specified.
**	attribute                       description of attribute to be added
**					or dropped.
**	db_lockmode                     Lockmode of the database for the caller.
**	tab_name                        table name.
**	tab_owner                       table owner.
**	
** Outputs:
**	err_code                        The reason for an error return status.
**	
** Returns:
**	         E_DB_OK
**	         E_DB_ERROR
**	     Exceptions:
**	         none
**	
**	Side Effects:
**	         none
**
** History:
**	18-oct-1993 (bryanp)
**	    Created.
**	16-oct-1996 (ramra01)
**	    Support for data types that have extensions such as
**	    long varchar, long byte. Examine relstat if the has
**	    extensions bit is on. 
**	14-nov-1996 (nanpr01)
**	    Bug 79062- causes iiattribute corruption.
**	02-dec-1996 (nanpr01)
**	    Fix up the error code mapping. We cannot include erusf.h
**	    in dmf. Let it get mapped in QEF.
**	30-dec-1996 (nanpr01)
**	    yet another bug with alter table : table level recovery 
**	    must be disallowed after the table is altered invalidating
**	    all the table checkpoints.
**	03-jan-1997 (nanpr01)
**	    Use the define's rather than hardcoded values.
**	01-oct-1998 (nanpr01)
**	    Added the new parameter in dm2r_replace for update performance
**	    improvement.
**	19-apr-1999 (somsa01)
**	    Pass the base table's rcb to si_cascade().
**	24-may-1999 (somsa01)
**	    Pass the relrecord to si_cascade().
**	28-may-1999 (nanpr01)
**	    Added table name and owner name in parameter list to display
**	    sensible error messages in errlog.log.
**      03-aug-1999 (stial01)
**          dm2u_atable() Create etab tables for any peripheral atts
**	13-mar-2002 (toumi01)
**	    Correct comment for change of DB_MAX_COLS from 300 to 1024.
**	14-Jan-2004 (jenjo02)
**	    Check TCB relstat & TCB_INDEX rather than tbl_id->db_tab_index.
**	03-Mar-2004 (gupsh01)
**	    Added support for alter table alter column.
**	6-Aug-2004 (schka24)
**	    Security access check is now a no-op, remove.
**	28-sep-2004 (gupsh01)
**	    Fixed invalid coercion type check done for alter table alter
**	    column.
**      01-Oct-2004 (stial01)
**	    Temporarily disable alter table when rows span pages
**	02-nov-2004 (thaju02)
**	    Alter table alter column causes exception, pass &adf_scb 
**	    to adi_tyname(). (B113359)
**	16-dec-04 (inkdo01)
**	    Add attcollID's.
**	07-Apr-2005 (thaju02)
**	    Disallow alter column from nullable to non-nullable.(B114021)
**	    Disallow alter column if column is base table key column
**	    or part of a secondary index (B114269)
**	09-May-2005 (thaju02)
**	    Change sizediff to signed.
**	02-Jun-2005 (thaju02)
**	    For alter column, update iiindex idom entries. (B114611)
**	30-May-2006 (jenjo02)
**	    iiindex idom count upped to DB_MAXIXATTS for indexes
**	    on clustered tables.
**	16-nov-2006 (dougi)
**	    Propagate add/drop changes to partitioned table iirelation rows 
**	    and fix iidistcol to account for dropped cols.
**	10-Oct-2007 (kschendel) SIR 122739
**	    Pass a (partial) relstat2 instead of some bogus flag thing.
**      23-Oct-2007 (kibro01) b118918
**          Some amendments of column types are allowed on the base version
**	    of the table, so the checks have been moved from parsing to 
**	    dmf, and have a more explicit error message.
**	17-Apr-2008 (kibro01) b120276
**	    Initialise ADF_CB structure
**	29-Sept-2009 (troal01)
**		Add support for geospatial
*/

DB_STATUS
dm2u_atable(
DMP_DCB		*dcb,
DML_XCB		*xcb,
DB_TAB_ID	*tbl_id,
i4		operation,
i4		cascade,
DMF_ATTR_ENTRY **attr_entry,
i4		relstat2,
i4		db_lockmode,
DB_TAB_NAME	*tab_name,
DB_OWN_NAME	*tab_owner,
DB_ERROR	*dberr)
{
    DMP_RCB             *r = (DMP_RCB *)0;
    DMP_RCB             *rcb = (DMP_RCB *) 0;
    DMP_RCB             *rel_rcb = (DMP_RCB *) 0;
    DMP_RCB             *attr_rcb = (DMP_RCB *) 0;
    DMP_TCB             *t;
    DMP_RELATION        relrecord;
    DMP_ATTRIBUTE       attrrecord;
    DMP_ATTRIBUTE	attrrecord_tmp;
    i4             i;
    DB_TAB_TIMESTAMP    timestamp;
    i4             error;
    i4             local_error;
    DB_ERROR            log_err;
    DB_STATUS           status;
    DB_STATUS           local_status;
    DB_TAB_ID           table_id;
    DM2R_KEY_DESC       key_desc[2];
    DM_TID              reltid;
    DM_TID              attrtid;
    DML_SCB             *scb;
    bool                syscat;
    i4             lk_mode;
    i4		purge_mode = DM2T_NOPURGE;
    i4             journal;
    i4                  gateway = 0;
    i4                  view    = 0;
    i4             has_extensions = 0;
    i4                  logging;
    LG_LSN              lsn;
    i4             table_access_mode;
    i4             timeout = 0;
    i4             journal_flag;
    i4		deleted_index = 0;
    u_i2		hold_attid = 0;
    u_i2		hold_attintl_id = 0;
    u_i2		dropped_col_intlid;
    u_i2		dropped_col_size;
    i2		dropped_col_attid = -1;
    bool		has_vers;
    bool		parttab;
    ADF_CB              adf_scb;
    i4                  dt_bits;
    DMP_ETAB_CATALOG    etab_record;
    u_i2		altcol_col_intlid;
    u_i2		altcol_col_size;
    i2			sizediff;
    bool		column_altered = FALSE;
    DMP_RCB		*idx_rcb = (DMP_RCB *) 0;
    DMP_INDEX		idxrec;
    DM_TID		idxtid;
    DB_TAB_ID		idx_tabid;
    i4			max;
    DB_ERROR		local_dberr;
    DB_ATT_NAME		tmpattnm;

    CLRDBERR(dberr);
    CLRDBERR(&log_err);

    MEfill(sizeof(ADF_CB),0,(PTR)&adf_scb);

    status = dm2u_ckp_lock(dcb, (DB_TAB_NAME *)NULL, (DB_OWN_NAME *)NULL, 
			   xcb, dberr);
    if (status != E_DB_OK)
        return (E_DB_ERROR);

    dmf_svcb->svcb_stat.utl_alter_tbl++; 

    /* 
    ** Although the user table will not be updated with any data, request to
    ** open to modify.  This will prevent READONLY table from having columns
    ** added or dropped.
    */

    table_access_mode = DM2T_A_MODIFY;  

    lk_mode = DM2T_X;
    scb = xcb->xcb_scb_ptr;

    timeout = dm2t_get_timeout(scb, tbl_id); /* from set lockmode */

    if (relstat2 & TCB2_HAS_EXTENSIONS)
        has_extensions = 1;

    for(;;)
    {
        status = dm2t_control(dcb, tbl_id->db_tab_base, xcb->xcb_lk_id,
            LK_X, (i4)0, timeout, dberr);

        if (status != E_DB_OK)
            return(E_DB_ERROR);

        /*  Open up the table. */

        status = dm2t_open(dcb, tbl_id, lk_mode,
            DM2T_UDIRECT, table_access_mode,
            timeout, (i4)20, (i4)0, xcb->xcb_log_id,
            xcb->xcb_lk_id, (i4)0, (i4)0,
            db_lockmode,
            &xcb->xcb_tran_id, &timestamp, &rcb, (DML_SCB *)0, dberr);

        if (status != E_DB_OK)
            break;

        r = rcb;
        t = r->rcb_tcb_ptr;
        r->rcb_xcb_ptr = xcb;

        journal = ((t->tcb_rel.relstat & TCB_JOURNAL) != 0);
        syscat = ((t->tcb_rel.relstat & (TCB_CATALOG | TCB_EXTCATALOG)) != 0);
        gateway = ((t->tcb_rel.relstat & TCB_GATEWAY) != 0);
        view    = ((t->tcb_rel.relstat & TCB_VIEW) != 0);
        journal_flag = (journal ? DM0L_JOURNAL : 0);
        has_vers = DMPP_VPT_PAGE_HAS_VERSIONS(t->tcb_rel.relpgtype);
	parttab = (t->tcb_rel.relnparts > 0);

        /*
        ** Check for NOLOGGING - no updates should be written to the log
        ** file if this session is running without logging.
        */
        if (xcb->xcb_flags & XCB_NOLOGGING)
            r->rcb_logging = 0;

        if ((xcb->xcb_flags & XCB_NOLOGGING) != 0 ||
            (t->tcb_temporary == TCB_TEMPORARY && t->tcb_logging == 0))
            logging = 0;
        else
            logging = 1;

        /* Can't update a system catalog */

        if (syscat)
        {
	    SETDBERR(dberr, 0, E_DM005E_CANT_UPDATE_SYSCAT);
	    status = E_DB_ERROR;
            break;
        }

	if (!has_vers)
	{
	    SETDBERR(dberr, 0, E_DM0169_ALTER_TABLE_SUPP);
	    status = E_DB_ERROR;
	    break;
	}

	if ( t->tcb_rel.relstat & TCB_INDEX )
        {
	   SETDBERR(dberr, 0, E_DM010A_ERROR_ALTERING_TABLE);
	   status = E_DB_ERROR;
           break;
        }

        if (t->tcb_open_count > 1)
        {
	   SETDBERR(dberr, 0, E_DM005D_TABLE_ACCESS_CONFLICT);
	   status = E_DB_ERROR;
           break;
        }

	/*
	** If this is DROP COLUMN, is the column to be dropped part of any
	** index? If so, and the user failed to specify CASCADE, then an
	** error will be returned.
	** Disallow ALTER COLUMN if column is a base table key column
	** or in a secondary index. It should be left to the user to 
	** drop and recreate the index.
	*/
	if ((operation == DMU_C_DROP_ALTER) || 
	    (operation == DMU_C_ALTCOL_ALTER))
	{
	    status = si_drop_column_check(t, attr_entry, cascade, dberr);
	    if (status)
		break;
	}
	
	/* Calculate max row size for this page size, page type */
	if (t->tcb_rel.relpgtype == DM_PG_V5)
	   max = DM_TUPLEN_MAX_V5;
	else
	   max = dm2u_maxreclen(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize);

	/*
	** If this is ADD COLUMN, will the tuple now be too long, or have too
	** many actual columns?
	*/
	if (operation == DMU_C_ADD_ALTER)
	{
	   if (t->tcb_rel.relatts == DB_MAX_COLS)
	   {
		SETDBERR(dberr, 0, E_DM0185_MAX_COL_EXCEEDED);
		status = E_DB_ERROR;
		break;
	   }

           if ( (t->tcb_rel.reltotwid + attr_entry[0]->attr_size) > max) 
           {
	      SETDBERR(dberr, 0, E_DM0186_MAX_TUPLEN_EXCEEDED);
	      status = E_DB_ERROR;
              break;
           }
	}
        /* Open the relation and attribute tables. */

        table_id.db_tab_base = DM_B_RELATION_TAB_ID;
        table_id.db_tab_index = DM_I_RELATION_TAB_ID;

        status = dm2t_open(dcb, &table_id, DM2T_IX, DM2T_UDIRECT,
                DM2T_A_WRITE, (i4)0, (i4)20, xcb->xcb_sp_id,
                xcb->xcb_log_id, xcb->xcb_lk_id, (i4)0,(i4)0,
                db_lockmode,
                &xcb->xcb_tran_id, &timestamp,
                &rel_rcb, (DML_SCB *)0, dberr);

        if (status != E_DB_OK)
            break;

        /*
        ** Check for NOLOGGING - no updates should be written to the log
        ** file if this session is running without logging.
        */
        if (xcb->xcb_flags & XCB_NOLOGGING)
            rel_rcb->rcb_logging = 0;

        /*
        ** Set user table journal state.
        ** Note that gateway tables are always listed as journaled.
        */
        rel_rcb->rcb_usertab_jnl = (journal_flag) ? 1 : 0;

        rel_rcb->rcb_xcb_ptr = xcb;

        table_id.db_tab_base = DM_B_ATTRIBUTE_TAB_ID;
        table_id.db_tab_index = DM_I_ATTRIBUTE_TAB_ID;

        status = dm2t_open(dcb, &table_id, DM2T_IX,
                DM2T_UDIRECT, DM2T_A_WRITE, (i4)0, (i4)20, 
                xcb->xcb_sp_id, xcb->xcb_log_id, xcb->xcb_lk_id, (i4)0,
                (i4) 0, db_lockmode,
                &xcb->xcb_tran_id, &timestamp,
                &attr_rcb, (DML_SCB *)0, dberr);

        if (status != E_DB_OK)
            break;

        /*
        ** Check for NOLOGGING - no updates should be written to the log
        ** file if this session is running without logging.
        */
        if (xcb->xcb_flags & XCB_NOLOGGING)
            attr_rcb->rcb_logging = 0;

        attr_rcb->rcb_usertab_jnl = (journal_flag) ? 1 : 0;
        attr_rcb->rcb_xcb_ptr = xcb;


        for (;;)
        {

            /* position iirelation on page for table being altered */

            table_id.db_tab_base = tbl_id->db_tab_base;
            table_id.db_tab_index = tbl_id->db_tab_index;
            key_desc[0].attr_operator = DM2R_EQ;
            key_desc[0].attr_number = DM_1_RELATION_KEY;
            key_desc[0].attr_value = (char *) &table_id.db_tab_base;
            key_desc[1].attr_operator = DM2R_EQ;
            key_desc[1].attr_number = DM_2_ATTRIBUTE_KEY;
            key_desc[1].attr_value = (char *) &table_id.db_tab_index;


            status = dm2r_position(rel_rcb, DM2R_QUAL, key_desc, (i4)1,
                                   (DM_TID *)0, dberr);
            if (status != E_DB_OK)
                break;

            for (;;)
            {
                status = dm2r_get(rel_rcb, &reltid, DM2R_GETNEXT,
                                  (char *)&relrecord, dberr);

                if (status != E_DB_OK)
		    break;

                if ((relrecord.reltid.db_tab_base == table_id.db_tab_base) &&
                    (relrecord.reltid.db_tab_index == table_id.db_tab_index))

                    break;

            }

            if (status != E_DB_OK)
               break;

	    if (t->tcb_rel.relcmptlvl == DMF_T0_VERSION ||
		t->tcb_rel.relcmptlvl == DMF_T1_VERSION ||
		t->tcb_rel.relcmptlvl == DMF_T2_VERSION)
            {
		SETDBERR(dberr, 0, E_DM00A5_ATBL_UNSUPPORTED);
		status = E_DB_ERROR;
	        break;
            }

            if (operation == DMU_C_ADD_ALTER)
            {
               status = dm2r_position(attr_rcb, DM2R_QUAL, key_desc, (i4)2,
                                      (DM_TID *)0,
				       dberr);

               if (status != E_DB_OK)
                  break;

               for (;;)
               {
                   status = dm2r_get(attr_rcb, &attrtid, DM2R_GETNEXT,
                                     (char *)&attrrecord, dberr);

                   if (status == E_DB_OK)
                   {

                      if ((attrrecord.attrelid.db_tab_base ==
		     				 table_id.db_tab_base) &&
                    	  (attrrecord.attrelid.db_tab_index ==
			 			 table_id.db_tab_index))
		      {

		          if (attrrecord.attintl_id > hold_attintl_id)
			     hold_attintl_id = attrrecord.attintl_id;

		          if ((attrrecord.attid > hold_attid) &&
                              (!attrrecord.attver_dropped))
                             hold_attid = attrrecord.attid;
		      }
                      continue;
                   }

                   if (status == E_DB_ERROR && dberr->err_code == E_DM0055_NONEXT)
                   {
                      attrrecord.attid = ++hold_attid;
                      attrrecord.attxtra = 0;
                      attrrecord.attoff = relrecord.relwid;
                      attrrecord.attfmt = attr_entry[0]->attr_type;
                      attrrecord.attfml = attr_entry[0]->attr_size;
                      attrrecord.attfmp = attr_entry[0]->attr_precision;
                      attrrecord.attkey = 0;
                      attrrecord.attflag = attr_entry[0]->attr_flags_mask;
                      COPY_DEFAULT_ID( attr_entry[0]->attr_defaultID,
                                      attrrecord.attDefaultID );
                      STRUCT_ASSIGN_MACRO(attr_entry[0]->attr_name,
                                             attrrecord.attname);
                      attrrecord.attintl_id = ++hold_attintl_id;
                      attrrecord.attver_added = (relrecord.relversion + 1);
                      attrrecord.attver_dropped = 0;
                      attrrecord.attver_altcol = 0;
                      attrrecord.attval_from = 0;
                      attrrecord.attcollID = attr_entry[0]->attr_collID;
                      attrrecord.attgeomtype = attr_entry[0]->attr_geomtype;
                      attrrecord.attsrid = attr_entry[0]->attr_srid;
                      MEfill(sizeof(attrrecord.attfree), 0,
                             (PTR)&attrrecord.attfree);

                      status = dm2r_put(attr_rcb, DM2R_DUPLICATES,
                                        (char *)&attrrecord, dberr);

                      if (status != E_DB_OK)
                      {
                            uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
                                       (char *)NULL, (i4)0, (i4 *)NULL,
                                       &local_error, 0);
			    SETDBERR(&log_err, 0, E_DM9028_ATTR_UPDATE_ERR);
                            break;
                      }

                      break;
                   }

                   else

                   {
		      SETDBERR(dberr, 0, E_DM008A_ERROR_GETTING_RECORD);
                      break;
                   }

                }   /* end of for (;;) */

                relrecord.relatts++;
                relrecord.relwid += attrrecord.attfml;
		relrecord.reltotwid += attrrecord.attfml;

		/* If we added a peripheral column, create the etab */
		status = adi_dtinfo(&adf_scb, attr_entry[0]->attr_type, 
			&dt_bits);
		if (status != E_DB_OK)
		    break;

		if (dt_bits & AD_PERIPHERAL)
		{
		    /*
		    ** VPS extension tables
		    ** Page size will be determined in dmpe
		    */
		    status = dmpe_create_extension(xcb, &relrecord, 
			    0, /* page_size determined in dmpe */
			    t->tcb_table_io.tbio_location_array,
			    t->tcb_table_io.tbio_loc_count,
			    attrrecord.attid, attrrecord.attfmt, 
			    &etab_record, dberr);
		    
		    if (status != E_DB_OK)
			break;
		}
            }
            else if (operation == DMU_C_DROP_ALTER)   /* process DROP COLUMN */
            {
	       for (i = 1; i <= t->tcb_rel.relatts; i++)
	       {
		   MEmove(t->tcb_atts_ptr[i].attnmlen,
		       t->tcb_atts_ptr[i].attnmstr,
		       ' ', DB_ATT_MAXNAME, tmpattnm.db_att_name);
		   if (t->tcb_atts_ptr[i].ver_dropped == 0 &&
		       MEcmp(tmpattnm.db_att_name,
				(PTR)&attr_entry[0]->attr_name,
				sizeof(DB_ATT_NAME) ) == 0)
		   {
		      dropped_col_intlid = t->tcb_atts_ptr[i].intl_id;
		      dropped_col_size = t->tcb_atts_ptr[i].length;
		      break;
		   }
	       }

	       if (i > t->tcb_rel.relatts)
	       {
		  SETDBERR(dberr, 0, E_DM002A_BAD_PARAMETER);
		  status = E_DB_ERROR;
		  break;
	       }

	       if (cascade)
	       {
		  status = si_cascade(xcb, &r, t, attr_entry, &relrecord,
				      db_lockmode, journal_flag,
				      &deleted_index, dberr);

		  if (status != E_DB_OK)
		     break;
	       }

               status = dm2r_position(attr_rcb, DM2R_QUAL, key_desc, (i4)2,
                                      (DM_TID *)0,
				       dberr);

               if (status != E_DB_OK)
                  break;

               for (;;)
               {
                   status = dm2r_get(attr_rcb, &attrtid, DM2R_GETNEXT,
                                     (char *)&attrrecord, dberr);

		   if (status == E_DB_OK) 
		   {
                      if (attrrecord.attrelid.db_tab_base ==
		     				 table_id.db_tab_base &&
                          attrrecord.attrelid.db_tab_index ==
			 			 table_id.db_tab_index &&  
			  attrrecord.attver_dropped == 0)
		      {
			  if (attrrecord.attintl_id == dropped_col_intlid)
			  {
		             attrrecord.attver_dropped =
			    			 (relrecord.relversion + 1);
		      	     relrecord.relwid -= attrrecord.attfml;

            	      	     status = dm2r_replace(attr_rcb, &attrtid,
			    		  DM2R_BYPOSITION, (char *)&attrrecord,
					  (char *)0, dberr);

            	      	     if (status != E_DB_OK)
            	      	     {
                	 	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
                    		    	   NULL, (char *)NULL, (i4)0, 
				     	   (i4 *)NULL, &local_error, 0);
				SETDBERR(&log_err, 0, E_DM9026_REL_UPDATE_ERR);
                	        break;
            	      	     }
			     dropped_col_attid = attrrecord.attid;
			  }
			  else if (attrrecord.attintl_id > dropped_col_intlid)
			  {
		      	     attrrecord.attoff -= dropped_col_size;
			     attrrecord.attid--;
            	             status = dm2r_replace(attr_rcb, &attrtid,
				          DM2R_BYPOSITION, (char *)&attrrecord,
					  (char *)0, dberr);

            	             if (status != E_DB_OK)
            	             {
                	 	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
                    		    	   NULL, (char *)NULL, (i4)0, 
				     	   (i4 *)NULL, &local_error, 0);
				SETDBERR(&log_err, 0, E_DM9026_REL_UPDATE_ERR);
                	        break;
            	             }
			  }
		      }
		  }
		  else
                  {
		      if (dberr->err_code == E_DM0055_NONEXT)
		      {
			 status = si_adjust_colmaps(dcb, xcb, &relrecord,
						    &table_id,
						    dropped_col_attid,
						    db_lockmode, journal_flag,
						    dberr);

			 if (status != E_DB_OK)
			    break;
		      }
                      break;
		  }
	       }
            }
	    else if (operation == DMU_C_ALTCOL_ALTER)   /* ALTER TAB ALT COLUMN */
            {
	      /* search the attribute to alter */
              for (i = 1; i <= t->tcb_rel.relatts; i++)
              {
		MEmove(t->tcb_atts_ptr[i].attnmlen,
		    t->tcb_atts_ptr[i].attnmstr,
		    ' ', DB_ATT_MAXNAME, tmpattnm.db_att_name);

                  if (t->tcb_atts_ptr[i].ver_altcol == 0 &&
                      t->tcb_atts_ptr[i].ver_dropped == 0 &&
                       MEcmp(tmpattnm.db_att_name,
                                (PTR)&attr_entry[0]->attr_name,
                                sizeof(DB_ATT_NAME) ) == 0)
                  {
		    DB_DT_ID	    coltype = 0;
            	    DB_DT_ID	    restype = 0;
                    ADI_DT_NAME     coltype_name;
                    ADI_DT_NAME     restype_name;
		    ADI_DT_BITMASK  typeset;

		    /* Check if the column is a key in the table */
                    if (t->tcb_atts_ptr[i].key)
                    {
                        uleFormat(dberr, E_DM019C_ACOL_KEY_NOT_ALLOWED, 
					(CL_ERR_DESC *)NULL, ULE_LOG,
                                           NULL, (char *)NULL, (i4)0,
                                           (i4 *)NULL, &local_error, 4,
                          sizeof(DB_OWN_NAME), t->tcb_rel.relowner.db_own_name,
                          sizeof(DB_TAB_NAME), t->tcb_rel.relid.db_tab_name,
                          sizeof(DB_DB_NAME), t->tcb_dcb_ptr->dcb_name.db_db_name,
			  t->tcb_atts_ptr[i].attnmlen,
			  t->tcb_atts_ptr[i].attnmstr);
			SETDBERR(&log_err, 0, E_DM9028_ATTR_UPDATE_ERR);
                        status = E_DB_ERROR;
                        break;
                    }

                    /* Check for incompatible type coercion */
		    coltype = abs(t->tcb_atts_ptr[i].type);
            	    restype = abs(attr_entry[0]->attr_type);
            	    if (coltype != restype)
            	    {
                      if ((status = adi_tycoerce(&adf_scb, coltype, &typeset))
                   		|| !BTtest((i4) ADI_DT_MAP_MACRO(restype), 
				(char*) &typeset))
                      {

                        STmove("<none>", ' ', 
			  sizeof (ADI_DT_NAME), (char *) &coltype_name);
                        STmove("<none>", ' ', 
		 	  sizeof (ADI_DT_NAME), (char *) &restype_name);

                        status = adi_tyname(&adf_scb, coltype, &coltype_name);
                        status = adi_tyname(&adf_scb, restype, &restype_name);

                        uleFormat(dberr, E_DM019B_INVALID_ALTCOL_PARAM, 
					(CL_ERR_DESC *)NULL, ULE_LOG,
                                           NULL, (char *)NULL, (i4)0,
                                           (i4 *)NULL, &local_error, 3,
					   STtrmwhite((char *)&coltype_name),
					   &coltype_name, 
					   STtrmwhite((char *)&restype_name), 
					   &restype_name,
					   t->tcb_atts_ptr[i].attnmlen,
					   t->tcb_atts_ptr[i].attnmstr);
			SETDBERR(&log_err, 0, E_DM9028_ATTR_UPDATE_ERR);
                        status = E_DB_ERROR;
                        break;
                      }
            	    }
		    else if ( (t->tcb_atts_ptr[i].type < 0) &&
			      (attr_entry[0]->attr_type > 0) )
		    {
			/* nullable to non-nullable */
                        uleFormat(dberr, E_DM019B_INVALID_ALTCOL_PARAM, 
					(CL_ERR_DESC *)NULL, ULE_LOG,
                                           NULL, (char *)NULL, (i4)0,
                                           (i4 *)NULL, &local_error, 3,
                                           sizeof("nullable"),
                                           "nullable",
                                           sizeof("non-nullable"),
                                           "non-nullable",
					   t->tcb_atts_ptr[i].attnmlen,
					   t->tcb_atts_ptr[i].attnmstr);
			SETDBERR(&log_err, 0, E_DM9028_ATTR_UPDATE_ERR);
                        status = E_DB_ERROR;
                        break;
		    }
		    /* If this is not the base version of a table (also
		    ** reset if the table has usermod run against it)
		    ** then make additional default value checks.
		    */
		    if (relrecord.relversion > 0)
		    {
			DB_TAB_ID *old,*new;
			old = &t->tcb_atts_ptr[i].defaultID;
			new = &attr_entry[0]->attr_defaultID;
			if (MEcmp(old,new,sizeof(DB_TAB_ID))!=0)
			{
                            uleFormat(dberr, E_DM01A0_INVALID_ALTCOL_DEFAULT, 
					(CL_ERR_DESC *)NULL, ULE_LOG,
                                           NULL, (char *)NULL, (i4)0,
                                           (i4 *)NULL, &local_error, 0);
                            log_err = *dberr;
                            status = E_DB_ERROR;
                            break;
			}
		    }
		     
                    attrrecord.attver_added = (relrecord.relversion + 1);
                    altcol_col_intlid = t->tcb_atts_ptr[i].intl_id;
                    altcol_col_size = t->tcb_atts_ptr[i].length;
                    break;
                  }
              }
	      if (status != E_DB_OK)
	        break;

              if (i > t->tcb_rel.relatts)
              {
	          /* Requesting alter of a non existing column */
		  SETDBERR(dberr, 0, E_DM002A_BAD_PARAMETER);
                  status = E_DB_ERROR;
                  break;
              }

		/*
		** If this is ALTER COLUMN, will the tuple now be too long?
		*/
		if (((t->tcb_rel.reltotwid - altcol_col_size) +
			attr_entry[0]->attr_size) > max)
		{
		    SETDBERR(dberr, 0, E_DM0186_MAX_TUPLEN_EXCEEDED);
		    status = E_DB_ERROR;
		    break;
		}
	       

	      /* Get the iiattribute record */
	      status = dm2r_position(attr_rcb, DM2R_QUAL, key_desc, (i4)2,
                                      (DM_TID *)0,
                                       dberr);
              if (status != E_DB_OK)
                  break;

	      /*
	      ** Must calculate sizediff before processing atts
	      ** We won't necessarily see altered att before the others
	      ** we need to update
	      */
	      sizediff = attr_entry[0]->attr_size - altcol_col_size;

	      for (;;)
              {
	          /* Get the iiattribute record */
                  status = dm2r_get(attr_rcb, &attrtid, DM2R_GETNEXT,
                                     (char *)&attrrecord, dberr);

                  if (status == E_DB_OK)
                  {
                    if ((attrrecord.attrelid.db_tab_base == table_id.db_tab_base) &&
                        (attrrecord.attrelid.db_tab_index == table_id.db_tab_index))
		    { 
                        if (attrrecord.attintl_id == altcol_col_intlid)
                        {
                            attrrecord.attver_dropped = relrecord.relversion + 1;
                            attrrecord.attver_altcol = relrecord.relversion + 1;


                            status = dm2r_replace(attr_rcb, &attrtid,
                                          DM2R_BYPOSITION, (char *)&attrrecord,
                                          (char *)0, dberr);
                            if (status != E_DB_OK)
                            {
                                uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
                                           NULL, (char *)NULL, (i4)0,
                                           (i4 *)NULL, &local_error, 0);
				SETDBERR(&log_err, 0, E_DM9026_REL_UPDATE_ERR);
                                break;
                            }

                            /* here we will append the record */
			    STRUCT_ASSIGN_MACRO(attrrecord.attrelid, 
						attrrecord_tmp.attrelid);
                            attrrecord_tmp.attid = attrrecord.attid;
                            attrrecord_tmp.attxtra = attrrecord.attxtra;
			    STRUCT_ASSIGN_MACRO(attr_entry[0]->attr_name,
                                        attrrecord_tmp.attname);
                            attrrecord_tmp.attoff = attrrecord.attoff;
                            attrrecord_tmp.attfmt = attr_entry[0]->attr_type;
                            attrrecord_tmp.attfml = attr_entry[0]->attr_size;
                            attrrecord_tmp.attfmp = attr_entry[0]->attr_precision;
                            attrrecord_tmp.attkey = attrrecord.attkey; 
                            attrrecord_tmp.attflag = attr_entry[0]->attr_flags_mask;
                            COPY_DEFAULT_ID( attr_entry[0]->attr_defaultID,
                                        attrrecord_tmp.attDefaultID );
                            attrrecord_tmp.attintl_id = attrrecord.attintl_id + 1;
                            attrrecord_tmp.attver_added = relrecord.relversion + 1;
				/* attrrecord.attver_added; */
                            attrrecord_tmp.attver_dropped = 0; 
                            attrrecord_tmp.attval_from = attrrecord.attval_from; 
                            attrrecord_tmp.attver_altcol = 0;
                            attrrecord_tmp.attcollID = 
					attr_entry[0]->attr_collID;
                            attrrecord_tmp.attgeomtype = attr_entry[0]->attr_geomtype;
                            attrrecord_tmp.attsrid = attr_entry[0]->attr_srid;
                            MEfill(sizeof(attrrecord_tmp.attfree), 0, 
					(PTR)&attrrecord_tmp.attfree);

                            status = dm2r_put(attr_rcb, DM2R_DUPLICATES,
                                          (char *)&attrrecord_tmp, dberr);
                            if (status != E_DB_OK)
                            {
                               uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
                                           NULL, (char *)NULL, (i4)0,
                                           (i4 *)NULL, &local_error, 0);
			       SETDBERR(&log_err, 0, E_DM9028_ATTR_UPDATE_ERR);
                               break;
                            }
                            relrecord.relwid += sizediff;
                	    relrecord.relatts++;
                            relrecord.reltotwid += sizediff;
			    column_altered = TRUE;
		        }
		        else if (attrrecord.attintl_id > altcol_col_intlid)
                        {
			  if ((column_altered == TRUE ) && 
			      (attrrecord.attintl_id == altcol_col_intlid + 1) &&
                              (MEcmp((PTR)&attrrecord.attname,
                                (PTR)&attr_entry[0]->attr_name,
                                sizeof(DB_ATT_NAME) ) == 0))
			    continue;
			    
                          attrrecord.attoff += sizediff;
                          attrrecord.attintl_id++;
                          status = dm2r_replace(attr_rcb, &attrtid,
                                          DM2R_BYPOSITION, (char *)&attrrecord,
                                          (char *)0, dberr);

                           if (status != E_DB_OK)
                           {
                                uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
                                           NULL, (char *)NULL, (i4)0,
                                           (i4 *)NULL, &local_error, 0);
				SETDBERR(&log_err, 0, E_DM9026_REL_UPDATE_ERR);
                                break;
                           }
                       }
		    }
		  }
		  else
                  {
                      if (dberr->err_code == E_DM0055_NONEXT)
                      {
			status = E_DB_OK;
			CLRDBERR(dberr);
                      }
                      break;
                  }
	      } /* end for ;; */

	      if (column_altered && t->tcb_index_count)
	      {
	          /* open iiindex */
		  idx_tabid.db_tab_base = DM_B_INDEX_TAB_ID;
		  idx_tabid.db_tab_index = DM_I_INDEX_TAB_ID;
		  status = dm2t_open(dcb, &idx_tabid, DM2T_IX,
			DM2T_UDIRECT, DM2T_A_WRITE, (i4)0, (i4)20,
			xcb->xcb_sp_id, xcb->xcb_log_id, xcb->xcb_lk_id, 
			(i4)0, (i4) 0, db_lockmode,
			&xcb->xcb_tran_id, &timestamp,
			&idx_rcb, (DML_SCB *)0, dberr);

		  if (status != E_DB_OK)
		      break;

		  status = dm2r_position(idx_rcb, DM2R_QUAL, key_desc, (i4)1, 
			      (DM_TID *)0, dberr);

		  if (status != E_DB_OK)
		      break;

		  for (;;)
		  {
		      bool	index_update = FALSE;

		      status = dm2r_get(idx_rcb, &idxtid, DM2R_GETNEXT,
                                 (char *)&idxrec, dberr);

		      if (status == E_DB_OK)
		      {
			  if (idxrec.baseid == table_id.db_tab_base)
			  {
			      for (i = 0; i < DB_MAXIXATTS; i++)
			      {
			          if (idxrec.idom[i] > altcol_col_intlid)
			          {
				      idxrec.idom[i]++;
				      index_update = TRUE;
			          }
			      }
	
		   	      if (index_update)
			      {
				  status = dm2r_replace(idx_rcb, &idxtid,
                                          DM2R_BYPOSITION, (char *)&idxrec,
                                          (char *)0, dberr);
			          if (status != E_DB_OK)
				  {
                                      uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, 
				      	   ULE_LOG, NULL, (char *)NULL, (i4)0,
                                           (i4 *)NULL, &local_error, 0);
				      SETDBERR(&log_err, 0, E_DM9027_INDEX_UPDATE_ERR);
                                      break;
				  }
			      }
		  	  }
		      }
		      else 
		      {
			  if (dberr->err_code == E_DM0055_NONEXT)
			  {
			      status = E_DB_OK;
			      CLRDBERR(dberr);
			  }
			  break;
		      }
		  }
	      }
	    }
	    else
	    {
	       SETDBERR(dberr, 0, E_DM002A_BAD_PARAMETER);
	       status = E_DB_ERROR;
	    }

	    if (status != E_DB_OK)
	       break;

            TMget_stamp((TM_STAMP *)&relrecord.relstamp12);
	    relrecord.relmoddate = TMsecs();
            relrecord.relversion++;
	    relrecord.relstat2  |= (TCB2_ALTERED |TCB2_TBL_RECOVERY_DISALLOWED);
            if (has_extensions)
	    {
		/*
		** If alter add peripheral, it is safe to specify
		** byte swapping of key only if this is the first
		** peripheral column for the table
		** (or of course if byte swapping is already on)
		*/
		if ((relrecord.relstat2 & TCB2_HAS_EXTENSIONS) == 0)
		    relrecord.relstat2 |= TCB2_BSWAP;
                relrecord.relstat2 |= TCB2_HAS_EXTENSIONS;
	    }
	    if (deleted_index)
	    {
		relrecord.relidxcount -= deleted_index;
		if (relrecord.relidxcount == 0)
		    relrecord.relstat &= ~(TCB_IDXD);
	    }


            /*
            ** Replace altered relation record in iirelation table.
            */
            status = dm2r_replace(rel_rcb, &reltid, DM2R_BYPOSITION,
                                  (char *)&relrecord, (char *)0, dberr);

            if (status != E_DB_OK)
            {
                uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
                    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
		SETDBERR(&log_err, 0, E_DM9026_REL_UPDATE_ERR);
                break;
            }

            /* Check for errors inserting records. */

            if (status != E_DB_OK)
                break;

            /*
            ** Unfix any fixed pages from iiattribute.
            */

            status = dm2r_unfix_pages(attr_rcb, dberr);
            if (status != E_DB_OK)
            {
                uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
                    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
		SETDBERR(&log_err, 0, E_DM9028_ATTR_UPDATE_ERR);
                break;
            }

	    /* If table is partitioned and operation is ADD or DROP,
	    ** update partition iirelation rows and (for DROP) fix
	    ** iidistcol att_number values. */
	    if (parttab && operation != DMU_C_ALTCOL_ALTER)
	    {
		status = pt_adddrop_adjust(xcb, dcb, rel_rcb, &key_desc[0], 
			&relrecord, db_lockmode, dropped_col_attid, dberr);
		if (status != E_DB_OK)
		    break;
	    }

            /*
            ** Log the alter operation - unless logging is disabled.
            */
            if ((xcb->xcb_flags & XCB_NOLOGGING) == 0)
            {
                /*
                ** Note assumptions made here about the location of pages
                ** created during dm1s_empty_table.
                */
                status = dm0l_alter(xcb->xcb_log_id, journal_flag,
                    tbl_id, tab_name, tab_owner, cascade,
                    operation, (LG_LSN *)0, &lsn, dberr);

                if (status != E_DB_OK)
                    break;
            }

            /*
            ** Log the DMU operation.  This marks a spot in the log file to
            ** which we can only execute rollback recovery once.  If we now
            ** issue update statements against the newly-created table, we
            ** cannot do abort processing for those statements once we have
            ** begun backing out the create.
            */
            if ((xcb->xcb_flags & XCB_NOLOGGING) == 0)
            {
                status = dm0l_dmu(xcb->xcb_log_id, journal_flag, tbl_id,
                    		  tab_name, tab_owner, (i4)DM0LALTER,
				  (LG_LSN *)0, dberr);
                if (status != E_DB_OK)
                    break;
            }

            break;
        } /* end for */

	break;

    }  /* end for */

    if (status && dberr->err_code)
    {
        /* If we have an error saved up to log - then do it */
        if (log_err.err_code && dberr->err_code > E_DM_INTERNAL)
        {
            uleFormat(&log_err, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
                    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 1,
                    sizeof(DB_DB_NAME), &dcb->dcb_name);
        }
    }

    /*
    ** Close down any of the core system catalogs that haven't been closed
    ** yet. If we get any errors here, then log the error and return it.
    */

    purge_mode = ( (status == E_DB_OK) ? DM2T_PURGE : DM2T_NOPURGE);

    if (rel_rcb)
    {
        local_status = dm2t_close(rel_rcb, purge_mode, &local_dberr);
        if (local_status)
        {
            uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, 
			NULL, 0, NULL, &local_error, 0);
            if ( status == E_DB_OK ) 
	    {
		*dberr = local_dberr;
		status = local_status;
	    }
        }
    }

    if (attr_rcb)
    {
        local_status = dm2t_close(attr_rcb, purge_mode, &local_dberr);
        if (local_status)
        {
            uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, 
			NULL, 0, NULL, &local_error, 0);
            if ( status == E_DB_OK ) 
	    {
		*dberr = local_dberr;
		status = local_status;
	    }
        }
    }

    if (idx_rcb)
    {
        local_status = dm2t_close(idx_rcb, purge_mode, &local_dberr);
        if (local_status)
        {
            uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, 
			NULL, 0, NULL, &local_error, 0);
            if ( status == E_DB_OK ) 
	    {
		*dberr = local_dberr;
		status = local_status;
	    }
        }
    }

    /* Close the user table. */

    if (r)
    {
        local_status = dm2t_close(r, purge_mode, &local_dberr);
        if (local_status)
        {
            uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, 
			NULL, 0, NULL, &local_error, 0);
            if ( status == E_DB_OK ) 
	    {
		*dberr = local_dberr;
		status = local_status;
	    }
        }
    }

    return(status);
}

/*
** Name: si_drop_column_check		- is dropped column in any indexes?
**
** Description:
**	If this is DROP COLUMN, is the column to be dropped part of any
**	index? If so, and the user failed to specify CASCADE, then an
**	error will be returned.
**
** Inputs:
**	t				- TCB for table being altered.
**	attribute			- the column being dropped.
**	cascade				- did user specify cascade?
**
** Outputs:
**	dberr				Set if column is in any indexes.
**
** Returns:
**	E_DB_OK				Column is in no indexes.
**	E_DB_ERROR			Column is in at least one index.
**
** History:
**	22-nov-1993 (bryanp)
**	    Created.
*/
static DB_STATUS
si_drop_column_check(
DMP_TCB		    *t,
DMF_ATTR_ENTRY	    **attribute,
i4		    cascade,
DB_ERROR	    *dberr)
{
    i4	i;
    DMP_TCB	*it;
    DB_ATT_NAME tmpattnm;

    CLRDBERR(dberr);

    for (i = 0; i < t->tcb_rel.relatts; i++)
    {
	MEmove(t->tcb_data_rac.att_ptrs[i]->attnmlen,
	    t->tcb_data_rac.att_ptrs[i]->attnmstr,
	    ' ', DB_ATT_MAXNAME, tmpattnm.db_att_name);
	if (MEcmp(tmpattnm.db_att_name,
		    &attribute[0]->attr_name,
		    sizeof(attribute[0]->attr_name)) == 0)
	{
	    if (t->tcb_data_rac.att_ptrs[i]->key)
	    {
		SETDBERR(dberr, 0, E_DM00A6_ATBL_COL_INDEX_KEY);
		return (E_DB_ERROR);
	    }
	}
    }
    /*
    ** If user specified cascade, no more error checking is needed.
    */
    if (cascade)
	return (E_DB_OK);

    /*
    ** See if this attribute is in any secondary index. If so, refuse the
    ** alter operation, since user failed to specify CASCADE.
    */

    for (it = t->tcb_iq_next;
	it != (DMP_TCB*) &t->tcb_iq_next;
	it = it->tcb_q_next)
    {
	for (i = 0; i < it->tcb_rel.relatts; i++)
	{
	    MEmove(it->tcb_data_rac.att_ptrs[i]->attnmlen,
		it->tcb_data_rac.att_ptrs[i]->attnmstr,
		' ', DB_ATT_MAXNAME, tmpattnm.db_att_name);
	    if (MEcmp(tmpattnm.db_att_name,
			&attribute[0]->attr_name,
			sizeof(attribute[0]->attr_name)) == 0)
	    {
		SETDBERR(dberr, 0, E_DM00A7_ATBL_COL_INDEX_SEC);
		return (E_DB_ERROR);
	    }
	}
    }

    return (E_DB_OK);
}

/*
** Name: si_cascade		    - Secondary index CASCADE processing.
**
** Description:
**	If this column is part of any secondary indexes, they will be
**	automatically dropped.
**
** Inputs:
**	t			    - table being altered.
**	attribute		    - attribute being dropped
**
** Outputs:
**	err_code		    - reason for error return.
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**	22-nov-1993 (bryanp)
**	    Created
**	13-jan-1997 (nanpr01)
**	    Fix bug 79650 : Multiple secondary index dependency is not 
**	    getting dropped correctly. When we are looping through the 
**	    secondary index tcb's we are setting the same indexid and trying
**	    to destroy. The fix can be put in a different way.
**	19-apr-1999 (somsa01)
**	    Refined the way we delete the indices. Store them in a linked
**	    list, close purge the base table, then walk the linked list
**	    destroying the indices one by one. This way, we will not be
**	    renaming table files which are currently open.
**	24-may-1999 (somsa01)
**	    Since we are close-purging the base table, update the relation
**	    record with the proper row and page counts before dm2u_atable()
**	    updates iirelation with old information.
**    09-jun-2000 (gupsh01)
**        Corrected the pointer assignment to the link list. The new element
**        was not being correctly added to the link list. Also added checking 
**        for duplicate elements as we are no longer deleting the index before
**        calling dm2r_get.
**	11-Nov-2008 (jonj)
**	  Relocated CLRDBERR to after static index_array.
*/
static	DB_STATUS
si_cascade(
DML_XCB		    *xcb,
DMP_RCB		    **base_rcb,
DMP_TCB		    *t,
DMF_ATTR_ENTRY	    **attribute,
DMP_RELATION	    *relrecord,
i4		    db_lockmode,
i4		    journal_flag,
i4		    *deleted_index,
DB_ERROR	    *dberr)
{
    DMP_RCB		*r = (DMP_RCB *)0;
    DMP_RCB		*rcb;
    DMP_DCB		*dcb = t->tcb_dcb_ptr;
    DMP_TCB		*it;
    DB_TAB_ID		table_id;
    DM_TID		tid;
    DB_STATUS		status;
    DMP_INDEX		indexrecord;
    i4		i;
    i4             error;
    DB_TAB_TIMESTAMP	timestamp;
    DM2R_KEY_DESC	qual_list[2];
    i4			nofiles = 0;
    i4			found = 0;
    DB_ERROR		local_dberr;
    DB_ATT_NAME		tmpattnm;

    static struct index_array
    {
	DB_TAB_ID		idx_id;
	struct index_array	*next;
    } *idx_array = NULL, *tmp_array, *temp;

    CLRDBERR(dberr);

    *deleted_index = 0;
    for (;;)
    {
	/*  Build a qualification list used for searching the system tables. */

	qual_list[0].attr_number = DM_1_RELATION_KEY;
	qual_list[0].attr_operator = DM2R_EQ;
	qual_list[0].attr_value = (char *)&t->tcb_rel.reltid.db_tab_base;
	qual_list[1].attr_number = DM_2_ATTRIBUTE_KEY;
	qual_list[1].attr_operator = DM2R_EQ;
	qual_list[1].attr_value = (char *)&t->tcb_rel.reltid.db_tab_index;

	/*  Update indexes catalog and destroy indices. */

	table_id.db_tab_base = DM_B_INDEX_TAB_ID;
	table_id.db_tab_index = DM_I_INDEX_TAB_ID;
	status = dm2t_open(dcb, &table_id, DM2T_IX, DM2T_UDIRECT, 
		DM2T_A_WRITE,
		(i4)0, (i4)20, (i4)0, 
		xcb->xcb_log_id, xcb->xcb_lk_id,
		(i4)0, (i4)0,  DM2T_S, &xcb->xcb_tran_id, 
		&timestamp, &rcb, (DML_SCB *)0, dberr);
	if (status != E_DB_OK)
	    break;
	r = rcb;
	r->rcb_xcb_ptr = xcb;

	/*
	** Check for NOLOGGING - no updates should be written to the log
	** file if this session is running without logging.
	*/
	if (xcb->xcb_flags & XCB_NOLOGGING)
	    r->rcb_logging = 0;
	r->rcb_usertab_jnl = (journal_flag) ? 1 : 0;

	status = dm2r_position(r, DM2R_QUAL, qual_list, 
			     (i4)1,
			     (DM_TID *)0, dberr);
	if (status != E_DB_OK)
	    break;

	for (;;)
	{
	    status = dm2r_get(r, &tid, DM2R_GETNEXT, 
				 (char *)&indexrecord, dberr);

	    if (status == E_DB_OK)
	    {
		if (indexrecord.baseid == t->tcb_rel.reltid.db_tab_base)
		{
		    /*
		    ** if attribute[0]->attr_name is one of the attributes
		    ** in the indexrecord.indexid secondary, then store this
		    ** secondary index id in the index array so that it can
		    ** be dropped later.
		    */
		    for (it = t->tcb_iq_next;
			it != (DMP_TCB*) &t->tcb_iq_next;
			it = it->tcb_q_next)
		    {
			for (i = 0; i < it->tcb_rel.relatts; i++)
			{
			    MEmove(it->tcb_data_rac.att_ptrs[i]->attnmlen,
				it->tcb_data_rac.att_ptrs[i]->attnmstr,
				' ', DB_ATT_MAXNAME, tmpattnm.db_att_name);
			    if (MEcmp(tmpattnm.db_att_name,
					&attribute[0]->attr_name,
					sizeof(attribute[0]->attr_name)) == 0)
			    {
				   
                                    tmp_array = (struct index_array *)
                                        MEreqmem(0,
                                            sizeof(struct index_array),
                                            TRUE, NULL);
                                    tmp_array->idx_id.db_tab_base =
                                        it->tcb_rel.reltid.db_tab_base;
                                    tmp_array->idx_id.db_tab_index =
                                        it->tcb_rel.reltid.db_tab_index;
                                        found = 0;
                                        if(idx_array)
                                        {
                                                for ( temp = idx_array; temp;
                                                                temp = temp->next )
                                                {
                                                if ((temp->idx_id.db_tab_base ==
                                                        it->tcb_rel.reltid.db_tab_base)
                                                        && (temp->idx_id.db_tab_index ==
                                                        it->tcb_rel.reltid.db_tab_index))
                                                {
                                                        /* We make sure that we are not adding
                                                        ** any duplicates in the link list if
                                                        ** the intended index is already
                                                        ** added in the link list then do not add it.
                                                        */
                                                        found = 1;
                                                }
                                                }
                                        }
                                        if(!(found) || !(idx_array))
                                        {       /* Add the new element to the link list*/
                                                tmp_array->next = idx_array;
                                                idx_array = tmp_array;
                                        }
                                        else
                                                continue;
 
				status = dm2r_unfix_pages(r, dberr);

				break;
			    }
			}
			if (status != E_DB_OK)
			    break;
		    }
		    if (status != E_DB_OK)
			break;
		}
		continue;
	    }
	    if (status == E_DB_ERROR && dberr->err_code == E_DM0055_NONEXT)
	    {
		CLRDBERR(dberr);
		status = E_DB_OK;
	    }
	    break;
	}
	if (status != E_DB_OK)
	    break;

	status = dm2t_close(r, 0, dberr);
	if (status != E_DB_OK)
	    break;
	r = 0;

	/*
	** Close purge the base table before deleting the indices.
	*/
	if (*base_rcb)
	{
	    /*
	    ** Now, update relrecord with the proper page and row
	    ** counts.
	    */
	    relrecord->relpages = (*base_rcb)->rcb_relpages;
	    relrecord->reltups = (*base_rcb)->rcb_reltups;

	    status = dm2t_close(*base_rcb, DM2T_PURGE, dberr);
	    if (status != E_DB_OK)
		break;
	    *base_rcb = 0;
	}

	/*
	** Now, delete the indices found.
	*/
	while (idx_array)
	{
	    status = dm2u_destroy(dcb, xcb, &idx_array->idx_id, DM2T_X,
				  DM2U_MODIFY_REQUEST, 0, 0, 0, 0, dberr);

	    if (status == E_DB_OK)
		*deleted_index += 1;
	    else
		break;

	    tmp_array = idx_array;
	    idx_array = idx_array->next;
	    MEfree((PTR)tmp_array);
	}
	if (status != E_DB_OK)
	    break;

	return (E_DB_OK);
    }

    /*	Handle cleanup for error recovery. */

    while (idx_array)
    {
	tmp_array = idx_array;
	idx_array = idx_array->next;
	MEfree((PTR)tmp_array);
    }

    if (r)
	status = dm2t_close(r, DM2T_NOPURGE, &local_dberr);

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
	    (i4)0, (i4 *)NULL, &error, 0);
	SETDBERR(dberr, 0, E_DM9101_UPDATE_CATALOGS_ERROR); /* need new msg */
    }
    return (E_DB_ERROR);
}

/*{
** Name: si_adjust_colmaps
**
** Description:
**	This routine loops through the catalogs to fix the attid.
** Inputs:
**	dmu_cb		CREATE TABLE DMF control block.  this contains
**			the attribute descriptors.
**
** Outputs:
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None
**
** History:
**	01-aug-95 (athda01)
**	    Written for ALTER TABLE ADD/DROP COLUMN support.    
**	21-Apr-2005 (schka24)
**	    Just close the catalogs we update, don't PURGE them.  We don't
**	    have or need exclusive access to them, and multiple simultaneous
**	    alter's could lead to DM9C5D and a hang.
**
*/

static DB_STATUS
si_adjust_colmaps(
    DMP_DCB		*dcb,
    DML_XCB		*xcb,
    DMP_RELATION	*relrecord,
    DB_TAB_ID		*table_id,
    i4		attid,
    i4		db_lockmode,
    i4		journal_flag,
    DB_ERROR  		*dberr)
{
    DB_STATUS		status = E_DB_OK;
    DB_STATUS	    	local_status = E_DB_OK;
    i4		error;
    i4		local_error;
    i4			i, j;
    i4			cats;
    DB_INTEGRITY    	ituple;
    DB_PROTECTION   	ptuple;
    DB_IIPRIV		vtuple;
    DB_IIRULE		rtuple;
    DM2R_KEY_DESC 	*key_array[4];  /* iipriv, iirule, iiint, iiprot */
    DB_TAB_ID		cat_tbl_id[4];
    DB_COLUMN_BITMAP	*tuple_cols[4];
    PTR			tuple[4];
    DM2R_KEY_DESC 	ikey_array[2];
    DM2R_KEY_DESC 	pkey_array[2];
    DM2R_KEY_DESC 	vkey_array[3];
    DM2R_KEY_DESC 	rkey_array[2];
    DB_TAB_TIMESTAMP	timestamp;
    DMP_RCB		*r = (DMP_RCB *) 0;
    DM_TID		tuptid;
    DB_ERROR		local_dberr;

    CLRDBERR(dberr);

    for ( ;; ) 
    {

	cats = 0;

	vkey_array[0].attr_number = DM_1_PRIV_KEY;
	vkey_array[0].attr_operator = DMR_OP_EQ;
	vkey_array[0].attr_value = (char *) &table_id->db_tab_base;
	vkey_array[1].attr_number = DM_2_PRIV_KEY;
	vkey_array[1].attr_operator = DMR_OP_EQ;
	vkey_array[1].attr_value = (char *) &table_id->db_tab_index;
	key_array[cats] = vkey_array;
	cat_tbl_id[cats].db_tab_base = DM_B_PRIV_TAB_ID;
	cat_tbl_id[cats].db_tab_index = DM_I_PRIV_TAB_ID;
	tuple_cols[cats] = (DB_COLUMN_BITMAP*) &vtuple.db_priv_attmap;
	tuple[cats] = (char *) &vtuple;

	if (relrecord->relstat & TCB_INTEGS)
	{
	   cats++;
	   ikey_array[0].attr_number = DM_1_INTEGRITY_KEY;
	   ikey_array[0].attr_operator = DMR_OP_EQ;
	   ikey_array[0].attr_value = (char*) &table_id->db_tab_base;
	   ikey_array[1].attr_number = DM_2_INTEGRITY_KEY;
	   ikey_array[1].attr_operator = DMR_OP_EQ;
	   ikey_array[1].attr_value = (char*)  &table_id->db_tab_index;
	   key_array[cats] = ikey_array;
	   cat_tbl_id[cats].db_tab_base = DM_B_INTEGRITY_TAB_ID;
	   cat_tbl_id[cats].db_tab_index = DM_I_INTEGRITY_TAB_ID;
	   tuple_cols[cats] = &ituple.dbi_columns;
	   tuple[cats] = (char *) &ituple;

	}

	if (relrecord->relstat & TCB_PRTUPS)
	{
	   cats++;
	   pkey_array[0].attr_number = DM_1_PROTECT_KEY;
	   pkey_array[0].attr_operator = DMR_OP_EQ;
	   pkey_array[0].attr_value = (char*) &table_id->db_tab_base;
	   pkey_array[1].attr_number = DM_2_PROTECT_KEY;
	   pkey_array[1].attr_operator = DMR_OP_EQ;
	   pkey_array[1].attr_value = (char*) &table_id->db_tab_index;
	   key_array[cats] = pkey_array;
	   cat_tbl_id[cats].db_tab_base = DM_B_PROTECT_TAB_ID;
	   cat_tbl_id[cats].db_tab_index = DM_I_PROTECT_TAB_ID;
	   tuple_cols[cats] = (DB_COLUMN_BITMAP*) &ptuple.dbp_domset;
	   tuple[cats] = (char *) &ptuple;
	}

	if (relrecord->relstat & TCB_RULE)
	{
	   cats++;
	   rkey_array[0].attr_number = DM_1_RULE_KEY;
	   rkey_array[0].attr_operator = DMR_OP_EQ;
	   rkey_array[0].attr_value = (char*) &table_id->db_tab_base;
	   rkey_array[1].attr_number = DM_2_RULE_KEY;
	   rkey_array[1].attr_operator = DMR_OP_EQ;
	   rkey_array[1].attr_value = (char*) &table_id->db_tab_index;
	   key_array[cats] = rkey_array;
	   cat_tbl_id[cats].db_tab_base = DM_B_RULE_TAB_ID;
	   cat_tbl_id[cats].db_tab_index = DM_I_RULE_TAB_ID;
	   tuple_cols[cats] = &rtuple.dbr_columns;
	   tuple[cats] = (char *) &rtuple;
	}

	/* beginning of loops through catalog tables */

	for (i = 0; i <= cats; i++)
	{

	    status = dm2t_open(dcb, &cat_tbl_id[i],
	   		       DM2T_IX, DM2T_UDIRECT,
			       DM2T_A_WRITE, (i4)0, (i4)20,
			       xcb->xcb_sp_id, xcb->xcb_log_id, xcb->xcb_lk_id,
			       (i4)0, (i4)0, db_lockmode,
			       &xcb->xcb_tran_id, &timestamp, &r, (DML_SCB *)0,
			       dberr);

	    if (status != E_DB_OK)
	       break;

	    if (xcb->xcb_flags & XCB_NOLOGGING)
	       r->rcb_logging = 0;

	    r->rcb_usertab_jnl = (journal_flag) ? 1 : 0;
	    r->rcb_xcb_ptr = xcb;

	    status = dm2r_position(r, DM2R_QUAL, key_array[i], (i4)2,
				   (i4)0,
				   dberr);

	    if (status != E_DB_OK)
	       break;

	    for (;;)
	    {
		status = dm2r_get(r, &tuptid, DM2R_GETNEXT, (char *) tuple[i],
				  dberr);

		if ( (status != E_DB_OK) && (dberr->err_code == E_DM0055_NONEXT) )
		{
		   status = E_DB_OK;
		   CLRDBERR(dberr);
		   break;
		}

		if (status != E_DB_OK)
		   break;

		if ( BTcount((char *) tuple_cols[i], DB_COL_BITS) ==
		      (DB_COL_BITS) )

		   continue;

		j = attid;
		BTclear(j, (char *)tuple_cols[i]);

		for ( ;
		      (j = BTnext(j, (char *)tuple_cols[i], (DB_COL_BITS - j))) 
		       != -1;
		    )
		{
		    BTset( (j - 1), (char *)tuple_cols[i]);
		    BTclear(j, (char *)tuple_cols[i]);
		}

		status = dm2r_replace(r, &tuptid, DM2R_BYPOSITION,
				      (char *) tuple[i], (char *)0, dberr);

		if (status != E_DB_OK)
		   break;

	     }	/* end of for (;;)  */

	     if (status != E_DB_OK)
		break;

	     status = dm2t_close(r, DM2T_NOPURGE, dberr);

	     if (status != E_DB_OK)
	     {
		uleFormat(dberr, 0, NULL, ULE_LOG, NULL, 
			    NULL, 0, NULL, &error, 0);
		break;
	     }
	     r = 0;
	 }	/* end of for (i <= cats) */

	    break;
    }	/* end of for ;; loop */

    if (r)
    {
       local_status = dm2t_close(r, DM2T_NOPURGE, &local_dberr);

       if (local_status)
       {
	  if (dberr->err_code == 0)
	     *dberr = local_dberr;
	  
	  uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, 
	  		NULL, 0, NULL, &local_error, 0);
       }
    }

    return (status);
}

/*{
** Name: pt_adddrop_adjust	- make partitioned table adjustments 
**
** Description:
**	This routine updates partition iirelation rows to match base
**	table master row and, for drop column, adjusts iidistcol att_number
**	values, as necessary.
**
** Inputs:
**	rel_rcb		Control block for iirelation fetchs.
**	key_desc	Ptr to key description for iirelation fetches.
**	relrecp		Ptr to iirelation row image.
**	dropped_col	att_number of dropped column (or -1).
**
** Outputs:
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None
**
** History:
**	16-nov-2006 (dougi)
**	    Written for partitioned table support.
**
*/

static DB_STATUS
pt_adddrop_adjust(
	DML_XCB		*xcb,
	DMP_DCB		*dcb,
	DMP_RCB		*rel_rcbp,
	DM2R_KEY_DESC	*key_desc,
	DMP_RELATION	*relrecp,
	i4		db_lockmode,
	i2		dropped_col,
	DB_ERROR	*dberr)

{
    DB_STATUS	status, local_status;
    DMP_RELATION relrecord;
    DM_TID	reltid, dctid;
    DB_TAB_ID	table_id;
    DB_TAB_TIMESTAMP timestamp;
    DMP_RCB	*dcrcb;
    DM2R_KEY_DESC dckey_desc[2];
    DB_IIDISTCOL dcrow;
    i4		local_error;
    i4		error;
    DB_ERROR	local_dberr;

    CLRDBERR(dberr);
    

    /* Position at start of iirelation rows for the table, then loop 
    * over iirelation rows for partitions, updating each from master row. */
    status = dm2r_position(rel_rcbp, DM2R_QUAL, key_desc, (i4)1, 
			(DM_TID *)0, dberr);
    if (status != E_DB_OK)
	return(status);

    for ( ; ; )
    {
	status = dm2r_get(rel_rcbp, &reltid, DM2R_GETNEXT, 
			(char *)&relrecord, dberr);
	if (status != E_DB_OK)
	    break;

	/* Skip unless this is a partition iirelation row. */
	if (relrecord.reltid.db_tab_index >= 0)
	    continue;

	relrecord.reltotwid = relrecp->reltotwid;
	relrecord.relwid = relrecp->relwid;
	relrecord.relmoddate = relrecp->relmoddate;
	relrecord.relversion = relrecp->relversion;
	relrecord.relidxcount = relrecp->relidxcount;
	relrecord.relstamp12 = relrecp->relstamp12;

	/* Replace modified iirelation row. */
	status = dm2r_replace(rel_rcbp, &reltid, DM2R_BYPOSITION,
			(char *)&relrecord, (char *) 0, dberr);
	if (status != E_DB_OK)
	    return(status);
    }

    /* If this is ADD COLUMN, we're done. */
    if (dropped_col < 0)
	return(E_DB_OK);

    /* Now set up access to IIDISTCOL table and look for partitioning
    ** columns whose att_number > dropped_col. */
    table_id.db_tab_base = DM_B_DISTCOL_TAB_ID;
    table_id.db_tab_index = DM_I_DISTCOL_TAB_ID;

    status = dm2t_open(dcb, &table_id, DM2T_IX, DM2T_UDIRECT,
			DM2T_A_WRITE, (i4)0, (i4)20,
			xcb->xcb_sp_id, xcb->xcb_log_id, xcb->xcb_lk_id, 
			(i4)0, (i4)0,
			db_lockmode, &xcb->xcb_tran_id, &timestamp,
			&dcrcb, (DML_SCB *)0, dberr);
    if (status != E_DB_OK)
	return(status);

    table_id.db_tab_base = relrecp->reltid.db_tab_base;
    table_id.db_tab_index = 0;

    dckey_desc[0].attr_operator = DM2R_EQ;
    dckey_desc[0].attr_number = DM_1_DISTCOL_KEY;
    dckey_desc[0].attr_value = (char *)&table_id.db_tab_base;
    dckey_desc[1].attr_operator = DM2R_EQ;
    dckey_desc[1].attr_number = DM_2_DISTCOL_KEY;
    dckey_desc[1].attr_value = (char *)&table_id.db_tab_index;

    status = dm2r_position(dcrcb, DM2R_QUAL, dckey_desc, (i4)2,
			(DM_TID *)0, dberr);
    if (status != E_DB_OK)
	return(status);

    /* Loop over the partitioning columns. */
    for ( ; ; )
    {
	status = dm2r_get(dcrcb, &dctid, DM2R_GETNEXT, 
			(char *)&dcrow, dberr);

	if (status != E_DB_OK)
	{
	    if (dberr->err_code == E_DM0055_NONEXT)
	    {
		CLRDBERR(dberr);
		status = E_DB_OK;
	    }
	    break;
	}

	if (dcrow.attid <= dropped_col)
	    continue;

	/* Partitioning column follows dropped column - decrement
	** att_number and replace row. */
	dcrow.attid--;

	status = dm2r_replace(dcrcb, &dctid, DM2R_BYPOSITION, 
			(char *)&dcrow, (char *)0, dberr);
	if (status != E_DB_OK)
	    return(status);
    }

    /* Close IIDISTVAL. */
    local_status = dm2t_close(dcrcb, (status != E_DB_OK) ? DM2T_PURGE :
					DM2T_NOPURGE, &local_dberr);
    if (local_status != E_DB_OK)
    {
	if (status == E_DB_OK)
	{
	    status = local_status;
	    *dberr = local_dberr;
	}
    }

    return(status);

}
