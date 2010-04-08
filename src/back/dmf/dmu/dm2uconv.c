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
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <tr.h>
#include    <lg.h>
#include    <lk.h>
#include    <adf.h>
#include    <ulf.h>
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
#include    <cm.h>

/**
**
**  Name: DM2UCONV.C - Convert table utility operation.
**
**  Description:
**      This file contains routines that perform the convert table
**	functionality.
**	    dm2u_convert - convert a table to a new format.
**
**  History:
**	30-October-1992 (rmuth)
**          Created. 
**	19-jan-1993 (wolf) 
**	    Removed dm0a.h, it's been deleted
**	16-mar-1993 (rmuth)
**	    Include di.h
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to pick up locking system type definitions.
**	26-jul-1993 (rogerk)
**	    Include respective CL headers for all cl calls.
**	28-may-1999 (nanpr01)
**	    Added table name and owner name in parameter list to display
**	    sensible error messages in errlog.log.
**      03-aug-1999 (stial01)
**          Added dm2u_conv_etab() Create etab tables for any peripheral atts
**      19-jan-2000 (stial01)
**          Validate kperpage calculations for BTREE/RTREE tables/indexes
**      31-jan-2000 (stial01)
**          DO NOT do kperpage validationw for gateway tables or views
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      20-jul-2004 (vansa02) Bug 112515 INGDBA #283
**          Do not open the table with modify permission, if the table is
**          a read only table.  
**	11-Apr-2008 (kschendel)
**	    dm2r position call updated, fix here.
**	23-Oct-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2t_?, dm2r_? functions converted to DB_ERROR *
**	04-Dec-2008 (jonj)
**	    SIR 120874: dm1p_?, dm1u_? functions converted to DB_ERROR *
**/

/*
** Forward declarations of static functions:
*/

static VOID dm2u_conv_log_error(
			    i4		terr_code,
			    DB_ERROR	*dberr);

static DB_STATUS
dm2u_conv_etab(
		DMP_RCB             *rcb,
		DMP_DCB		    *dcb,
		DML_XCB		    *xcb,
		DB_ERROR            *dberr);


/*{
** Name: dm2u_convert - Convert a table to a new format..
**
** Description:
**	This routine is called from dmu_convert() and is used
**	to convert Ingres Tables to a new format. Primarily used
**	as part of database conversion.
**
** Inputs:
**	dcb				The database id. 
**	xcb				The transaction id.
**	tbl_id				The internal table id of the base table 
**
** Outputs:
**      err_code                        The reason for an error return status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	30-October-1992 (rmuth)
**	    Created.
**      16-May-2005 (zhahu02)
**      Updated for not dereferencing a NULL pointer to avoid sigsegv 
**      for upgrading (INGSRV3306/b114526).
*/
DB_STATUS
dm2u_convert(
    DMP_DCB	*dcb,
    DML_XCB	*xcb,
    DB_TAB_ID   *tbl_id,
    DB_ERROR	*dberr )
{
    DB_STATUS		status;
    DB_TAB_TIMESTAMP    timestamp;
    DMP_RCB		*rcb;
    DMP_TCB		*t;
    bool		table_open = FALSE;
    bool		table_lock = FALSE;
    i4			error;
    DB_ERROR		local_dberr;

    CLRDBERR(dberr);

    do
    {
	/*
	** Get the ckp lock
	*/
	status = dm2u_ckp_lock(dcb, (DB_TAB_NAME *)NULL, (DB_OWN_NAME *)NULL, 
				xcb, dberr);
	if ( status != E_DB_OK )
	    break;

	/*
	** Take an X table control lock on table
	*/
	status = dm2t_control( dcb, tbl_id->db_tab_base,
			       xcb->xcb_lk_id, LK_X, 
			       (i4)0, /* flags */
			       (i4)0, /*timeout */
			       dberr);
	if ( status != E_DB_OK )
	    break;

	table_lock = TRUE;

	/*
	** Open the table
	*/
	status = dm2t_open( dcb, tbl_id, DM2T_X, DM2T_UDIRECT, DM2T_A_MODIFY,
			    (i4)0,  /* timeout */
			    (i4)20, /* max locks */
			    (i4)0,  /* savepoint id */
			    xcb->xcb_log_id, xcb->xcb_lk_id,
			    (i4)0,  /* sequence */
			    (i4)0,  /* lock duration */
			    DM2T_X, 	 /* db lockmode */
			    &xcb->xcb_tran_id, &timestamp,
			    &rcb, (DML_SCB *)0, dberr );
	if ( status != E_DB_OK )
	{
	    if (dberr->err_code == E_DM0067_READONLY_TABLE_ERR)
	    {
		CLRDBERR(dberr);
		status = E_DB_OK;
	    }
	}
	else
	{
	    table_open = TRUE;
	}
        if (rcb == NULL)
        break;
	t = rcb->rcb_tcb_ptr;


	/*
	** Can't convert a system catalog without specical privilege
	*/
	if  ( (t->tcb_rel.relstat & (TCB_CATALOG | TCB_EXTCATALOG)) &&
	      (( xcb->xcb_scb_ptr->scb_s_type & SCB_S_SYSUPDATE) == 0)  )
	{
	    status     = E_DB_ERROR;
	    SETDBERR(dberr, 0, E_DM005E_CANT_UPDATE_SYSCAT);
	    break;
	}

	/*
	** If table has already been converted or no conversion
	** is needed ie its a view or a gateway table, then close
	** the table and return ok.
	*/
	if ( ( t->tcb_rel.relfhdr != DM_TBL_INVALID_FHDR_PAGENO ) ||
	     ( t->tcb_rel.relstat & TCB_GATEWAY ) ||
	     ( t->tcb_rel.relstat & TCB_VIEW ) )
	{
	    break;
	}

	/*
	** DBMS core catalogs are converted at database open time,
	** so if we reach here something has gone wrong so
	** issue error message
	*/
	if ( t->tcb_rel.relstat & TCB_CONCUR )
	{
	    status     = E_DB_ERROR;
	    SETDBERR(dberr, 0, E_DM005E_CANT_UPDATE_SYSCAT);
	    break;
	}

	/*
	** Turn of logging for the table
	*/
	rcb->rcb_logging = 0;

	/*
	** Add the FHDR and FMAP(s) to the table
	*/
	status = dm1p_convert_table_to_65( t, dberr );
	if ( status != E_DB_OK )
	    break;

	/*
	** Update the tables iirelation record
	*/
	status = dm2u_conv_rel_update(  t, xcb->xcb_lk_id, 
					&xcb->xcb_tran_id, dberr);
	if ( status != E_DB_OK )
	    break;

    } while (FALSE) ;

    if (( t->tcb_rel.relstat & TCB_GATEWAY ) == 0 &&
	( t->tcb_rel.relstat & TCB_VIEW ) == 0)
    {
	/*
	** We may have been called to convert tables having extensions
	** As of 2.5 we expect that extension tables are created when
	** the table is created/altered
	*/
	if (status == E_DB_OK && (t->tcb_rel.relstat2 & TCB2_HAS_EXTENSIONS) &&
	    (t->tcb_rel.relcmptlvl == DMF_T3_VERSION ||  /* Ingres 1.2 */
	       t->tcb_rel.relcmptlvl == DMF_T4_VERSION))    /* Ingres 2.0 */
	{
	    status = dm2u_conv_etab(rcb, dcb, xcb, dberr);
	}

	/*
	** Validate kperpage calculations for BTREE/RTREE tables/indexes 
	*/
	if (status == E_DB_OK && 
	    (t->tcb_rel.relspec == TCB_BTREE || t->tcb_rel.relspec == TCB_RTREE))
	{
	    status = dm1u_verify(rcb, xcb, DM1U_VERIFY | DM1U_KPERPAGE, dberr);
	}
    }

    /*
    ** Close the table
    */
    if (table_open)
    {
	if (status == E_DB_OK)
	{
	    status = dm2t_close( rcb, DM2T_NOPURGE, dberr );
	}
	else
	{
	    DB_STATUS		lstatus;

	    lstatus = dm2t_close( rcb, DM2T_NOPURGE, &local_dberr );
	    if ( lstatus != E_DB_OK )
	        dm2u_conv_log_error( E_DM92CD_DM2U_CONVERT_ERR, &local_dberr);
	}
    }

    /*
    ** Unlock the table
    */
    if (table_lock)
    {
	if (status == E_DB_OK)
	{
	    status = dm2t_control( dcb, tbl_id->db_tab_base,
			           xcb->xcb_lk_id, LK_N, 
			           (i4)0, /* flags */
			           (i4)0, /*timeout */
			           dberr);
	}
	else
	{
	    DB_STATUS		lstatus;

	    lstatus = dm2t_control( dcb, tbl_id->db_tab_base,
			            xcb->xcb_lk_id, LK_N, 
			            (i4)0, /* flags */
			            (i4)0, /*timeout */
			            &local_dberr);
	    if ( lstatus != E_DB_OK )
	        dm2u_conv_log_error( E_DM92CD_DM2U_CONVERT_ERR, &local_dberr);

	}
    }

    /*
    ** If all ok the return else issue error message
    */
    if ( status == E_DB_OK )
	return( E_DB_OK );

    /*
    ** Issue a traceback message
    */
    dm2u_conv_log_error( E_DM92CD_DM2U_CONVERT_ERR, dberr );

    return( status );
}

/*{
** Name: dm2u_conv_rel_update - Updates a tables iirelation record on disc.
**
** Description:
**	This routine updates the tables iirelation record on disc. The
**	fields that may have changed are relfdhr and relpages.
**	It is called once the the FHDR and FMAP(s) have been 
**	added to the table, 
**
** Inputs:
**      tcb                             The tables TCB.
**	lock_list			Lock list to reqiest locks on.
**	tran_id				Transaction id.
**
** Outputs:
**      err_code                        The reason for an error status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	30-October-1992 (rmuth)
**	    Created.
**	01-oct-1998 (nanpr01)
**	    Added the new parameter in dm2r_replace for update performance
**	    improvement.
**/
DB_STATUS
dm2u_conv_rel_update(
DMP_TCB             *tcb,
i4		    lock_list,
DB_TRAN_ID	    *tran_id,
DB_ERROR	    *dberr)
{
    DB_STATUS		status;    
    DMP_RCB		*rcb = (DMP_RCB *)0;
    DMP_TCB		*t = tcb;
    DMP_TCB		*it;
    DM2R_KEY_DESC	qual_list[1];
    DMP_RELATION	relrecord;
    DB_TAB_ID		table_id;
    DM_TID		tid;
    bool		need_record;

    CLRDBERR(dberr);


    if (DMZ_AM_MACRO(52))
    	TRdisplay("dm2u_conv_relfhdr_update: Table:  %~t, db: %~t\n",
	    sizeof(tcb->tcb_rel.relid), &tcb->tcb_rel.relid,
	    sizeof(tcb->tcb_dcb_ptr->dcb_name), &tcb->tcb_dcb_ptr->dcb_name );


    do
    {
	status = dm2r_rcb_allocate(t->tcb_dcb_ptr->dcb_rel_tcb_ptr, 0,
	    tran_id, lock_list, 0, &rcb, dberr);
	if (status != E_DB_OK)
	    break;

	rcb->rcb_lk_id = lock_list;
	rcb->rcb_logging = 0;
	rcb->rcb_journal = 0;
	rcb->rcb_k_duration |= RCB_K_TEMPORARY;

	/*
	** Locate the iirelation record for this table
	*/
	qual_list[0].attr_number = DM_1_RELATION_KEY;
	qual_list[0].attr_operator = DM2R_EQ;
	qual_list[0].attr_value = (char*) &t->tcb_rel.reltid.db_tab_base;
	
	status = dm2r_position(
			rcb, DM2R_QUAL, qual_list, (i4)1,
			(DM_TID *)0, dberr);
	if (status != E_DB_OK)
	    break;

	need_record = TRUE;
	while ( need_record )
	{
	    status = dm2r_get( rcb, &tid, DM2R_GETNEXT, 
                               (char *)&relrecord, dberr);
	    if ( status != E_DB_OK )
		break;

	    /*
	    ** Check if we have found the record
	    */
	    if ((relrecord.reltid.db_tab_base == 
				t->tcb_rel.reltid.db_tab_base) &&
		(relrecord.reltid.db_tab_index ==
				t->tcb_rel.reltid.db_tab_index ) )
	    need_record = FALSE;
	}

	if ( need_record )
	    break;

	/*
	** Setup the fields that may have been changed during the
	** conversion.
	*/
	relrecord.relfhdr = t->tcb_rel.relfhdr;

	if ((relrecord.relpages += t->tcb_page_adds) < 0)
	    relrecord.relpages = 0;

	t->tcb_page_adds = 0;
	t->tcb_rel.relpages = relrecord.relpages;


	/*
	** Replace the record on disc
	*/
	status = dm2r_replace( rcb, &tid, DM2R_BYPOSITION, 
		    	       (char *)&relrecord, (char *)0, dberr);
	if (status != E_DB_OK)
	    break;

	/*
	** Release the rcb
	*/
	status = dm2r_release_rcb(&rcb, dberr);
	if ( status != E_DB_OK)
	    break;


    } while (FALSE );

    if ( status == E_DB_OK )
	return( E_DB_OK);

    /*
    ** Error condition
    */
    if (rcb)
    {
	i4		lerr_code;
	DB_ERROR	ldberr;
	DB_STATUS	lstatus;

	lstatus = dm2r_release_rcb(&rcb, &ldberr);
	if ( lstatus != E_DB_OK )
	    dm2u_conv_log_error(E_DM9026_REL_UPDATE_ERR, &ldberr);
    }

    dm2u_conv_log_error(E_DM9026_REL_UPDATE_ERR, dberr);

    return( status );
}

/*{
** Name: dm2_conv_log_error - Used to log trace back messages.
**
** Description:
**	This routine is used to log traceback messages. The rcb parameter
**	is set if the routine is can be seen from outside this module.
**	
** Inputs:
**	terr_code			Holds traceback message indicating
**					current routine name.
**	dberr				Holds error message generated lower
**					down in the call stack.
**	rcb				Pointer to rcb so can get more info.
**
** Outputs:
**
**	Returns:
**	    none
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none
**
** History:
**	30-October-1992 (rmuth)
**	   Created
**	
*/
static VOID
dm2u_conv_log_error(
    i4	terr_code,
    DB_ERROR	*dberr)
{
    i4	l_err_code;

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &l_err_code, 0);
	SETDBERR(dberr, 0, terr_code);
    }
    else
    {
	/* 
	** err_code holds the error number which needs to be returned to
	** the DMF client. So we do not want to overwrite this message
	** when we issue a traceback message
	*/
	uleFormat(NULL, terr_code, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &l_err_code, 0);
    }

    return;
}


/*{
** Name: dm2u_conv_etab - Make sure all peripheral atts have an extension table.
**
** Description:
**      For each peripheral attribute of the specified table, we scan
**      the iiextended_relation catalog to see if an etab has been created.
**      If not, we create one.
**
** Inputs:
**      rcb                             The rcb for the table having extensions.
**	dcb				The database id. 
**	xcb				The transaction id.
**
** Outputs:
**      dberr	                        The reason for an error status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      03-Aug-1999 (stial01)
**	    Created.
**	17-Apr-2008 (kibro01) b120276
**	    Initialise ADF_CB structure
**/
static DB_STATUS
dm2u_conv_etab(
	DMP_RCB             *rcb,
	DMP_DCB		    *dcb,
	DML_XCB		    *xcb,
	DB_ERROR            *dberr)
{
    DMP_TCB             *t = rcb->rcb_tcb_ptr;
    i4          	dt_bits;
    i4          	att_id;
    ADF_CB      	adf_scb;
    DB_TAB_ID           table_id;
    DB_STATUS           status, local_status;
    bool                etab_found;
    DMP_ETAB_CATALOG    etab_record;
    DMP_RCB             *iietab_rcb;
    DM_TID              tid;
    DM2R_KEY_DESC       qual_list[2];
    DB_TAB_TIMESTAMP    timestamp;
    i4			error;
    DB_ERROR		local_dberr;

    CLRDBERR(dberr);

    MEfill(sizeof(ADF_CB),0,(PTR)&adf_scb);

    /* Open iiextended_relation, it should exist */
    table_id.db_tab_base = DM_B_ETAB_TAB_ID;
    table_id.db_tab_index = 0;
    status = dm2t_open( dcb, &table_id, DM2T_IS, DM2T_UDIRECT, DM2T_A_READ,
			(i4)0,  /* timeout */
			(i4)20, /* max locks */
			(i4)0,  /* savepoint id */
			xcb->xcb_log_id, xcb->xcb_lk_id,
			(i4)0,  /* sequence */
			(i4)0,  /* lock duration */
			DM2T_S, 	 /* db lockmode */
			&xcb->xcb_tran_id, &timestamp,
			&iietab_rcb, (DML_SCB *)0, dberr );
    if (status != E_DB_OK)
	return (status);

    /* Check for peripheral columns */
    for (att_id = 1; att_id <= t->tcb_rel.relatts; att_id++)
    {
	etab_found = FALSE;
	status = adi_dtinfo(&adf_scb, t->tcb_atts_ptr[att_id].type,&dt_bits);
	if (status != E_DB_OK)
	{
	    *dberr = adf_scb.adf_errcb.ad_dberror;
	    break;
	}

	if ((dt_bits & AD_PERIPHERAL) == 0)
	    continue;

	/* Position and try to get a record for this table,att */
	qual_list[0].attr_number = DM_1_ETAB_KEY;
	qual_list[0].attr_operator = DM2R_EQ;
	qual_list[0].attr_value = (char *)&t->tcb_rel.reltid.db_tab_base;

	status = dm2r_position(iietab_rcb, DM2R_QUAL, qual_list, (i4)1,
		     (DM_TID *)0, dberr);

	if (status && (dberr->err_code == E_DM0074_NOT_POSITIONED ||
			dberr->err_code == E_DM008E_ERROR_POSITIONING))
	{
	    status = dm2r_position(iietab_rcb, DM2R_ALL, 0, 0, NULL,
			dberr);
	    if (status)
		break;
	}

	do
	{
	    status = dm2r_get(iietab_rcb, &tid, DM2R_GETNEXT,
			 (char *)&etab_record, dberr);

	    if (status == E_DB_OK && 
		etab_record.etab_base == t->tcb_rel.reltid.db_tab_base &&
		etab_record.etab_att_id == att_id)
		etab_found = TRUE;
	} while (status == E_DB_OK && etab_found == FALSE);

	if (status && dberr->err_code != E_DM0055_NONEXT)
	    break;

	if (etab_found)
	    continue;

	/*
	** VPS extension tables... need to pass page size
	*/
	status = dmpe_create_extension(xcb, &t->tcb_rel, DM_COMPAT_PGSIZE,
		t->tcb_table_io.tbio_location_array,
		t->tcb_table_io.tbio_loc_count,
		att_id, t->tcb_atts_ptr[att_id].type, &etab_record, dberr);

	if (status)
	    break;

    }

    local_status = dm2t_close(iietab_rcb, 0, &local_dberr);
    if (local_status && status == E_DB_OK)
    {
	*dberr = local_dberr;
	status = local_status;
    }
    if ( status == E_DB_OK )
        CLRDBERR(dberr);
    return (status);
}
