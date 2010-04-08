/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <tm.h>
#include    <cs.h>
#include    <me.h>
#include    <pc.h>
#include    <sr.h>
#include    <sl.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <adf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmucb.h>
#include    <dmxcb.h>
#include    <dm.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dmxe.h>
#include    <dm0m.h>
#include    <dm1b.h>
#include    <dmpp.h>
#include    <dm1p.h>
#include    <dm2t.h>
#include    <di.h>
#include    <sxf.h>
#include    <dma.h>
#include    <dmse.h>
#include    <dm2umct.h>
#include    <dm2umxcb.h>
#include    <dm2u.h>
#include    <dm2up.h>
#include    <dmftrace.h>
#include    <dmu.h>
#include    <st.h>
#include    <dm0pbmcb.h>

/**
**
** Name: DMUPINDEX.C
**
** Description:
**      This file contains the functions necessary to create parallel indices
**      on a table.
**    
**      dmu_pindex()      -  Routine to perfrom the parallel create index
**                          operation.
**
** History:
**      01-may-98 (nanpr01) 
**          Created from dmu_index.
**      18-mar-1999 (stial01)
**          Copy err_code, err_data into corresponding dmu cb, not 1st. 
**	21-mar-1999 (nanpr01)
**	    Get rid of extent names since only one table is supported per
**	    raw location.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-Apr-2001 (horda03) Bug 104402
**          Ensure that Foreign Key constraint indices are created with
**          PERSISTENCE. This is different to Primary Key constraints as
**          the index need not be UNIQUE nor have a UNIQUE_SCOPE.
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to dma_write_audit() if not C2SECURE.
**      01-feb-2001 (stial01)
**          Updated test for valid page types (Variable Page Type SIR 102955)
**      09-mar-2001 (stial01)
**          Changes for Variable Page Type SIR 102955
**      28-mar-2001 (stial01)
**          Fixed initialization of indxcb_page_type
**      04-dec-2002 (stial01)
**          Initialized local vars for each index instead of once. (b109229)
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      14-apr-2004 (stial01)
**	    E_DM007D_BTREE_BAD_KEY_LENGTH should return klen and max klen
**	11-Mar-2005 (thaju02)
**	    Use $online idxs relation info. (B114069)
**      29-nov-2005 (horda03) Bug 48659/INGSRV 2716
**          A TX can now be started with a non-journaled BT record. This
**          leaves the XCB_DELAYBT flag set but sets the XCB_NONJNL_BT
**          flag. Need to write a journaled BT record if the update
**          will be journaled.
**	23-Oct-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	08-Dec-2008 (jonj)
**	    SIR 120874: dma_? auditing functions converted to DB_ERROR *
**/

/*{
** Name: dmu_pindex - Creates multiple indices on a table.
**
**  INTERNAL DMF call format:      status = dmu_pindex(&dmu_cb);
**
**  EXTERNAL call format:          status = dmf_call(DMU_PINDEX_TABLE,&dmu_cb);
**
** Description:
**    The dmu_index function handles the creation of indices.
**    This dmu function is allowed inside a user specified transaction.
**    The table name must not be the same as a system table name and must 
**    not be the same name as any other table owned by the same user. 
**    The table that this index is defined on must exist and must be
**    identified by use of the internal identifier obtained from a dmt_show
**    operation.  It is assumed that the caller has verified that the
**    base table is owned by the same user.
**
** Inputs:
**      .dmu_cb
**          .type                           Must be set to DMU_UTILITY_CB.
**          .length                         Must be at least sizeof(DMU_CB).
**          .dmu_tran_id                    Must be the current transaction 
**                                          identifier returned from the begin 
**                                          transaction operation.
**          .dmu_flags_mask                 Must be zero.
**          .dmu_tbl_id                     Internal name of table to be 
**                                          indexed.
**          .dmu_index_name                 External name of index to be 
**                                          created.
**          .dmu_location.data_address      Pointer to array of locations.
**                                          Each entry in array is of type
**                                          DB_LOC_NAME.  
**	    .dmu_location.data_in_size      The size of the location array
**                                          in bytes.
**          .dmu_olocation.data_address     This holds source of gateway
**					    table if this is a gateway register.
**          .dmu_key_array.ptr_address      Pointer to an area used to input
**                                          an array of pointer to entries
**					    of type DMU_KEY_ENTRY.  
**                                          See below for description of 
**                                          <dmu_key_list> entries.
**          .dmu_key_array.ptr_size         Size of an entry.
**          .dmu_key_array.ptr_in_count     Count of entries.
**	    .dmu_attr_array.ptr_address	    Pointer to to area used to input
**					    an array or pointers to entries
**					    of type DMU_KEY_ENTRY.
**					    If this entry is not passed in
**					    all the key's given in the key
**					    array are considered part of the
**					    key.  If this pass in, only the
**					    keys in this list are considered
**					    part of the key.  The keys listed
**					    in this list must be a prefix set
**					    of the keys listed in the key array.
**          .dmu_attr_array.ptr_size        Size of an entry.
**          .dmu_attr_array.ptr_in_count    Count of entries.
**          .dmu_char_array.data_address    Pointer to an area used to input
**                                          an array of entries of type
**                                          DMU_CHAR_ENTRY.
**                                          See below for description of 
**                                          <dmu_char_array> entries.
**          .dmu_char_array.data_in_size    Length of char_array in bytes.
**	    .dmu_gwchar_array.data_address  Pointer to an array of gateway table
**					    characteristics.  These are used
**					    if the table is a DMU_GATEWAY type
**					    table.  These characteristics are
**					    passed directly down to the Ingres
**					    Gateway system.
**	    .dmu_gwchar_array.data_in_size  Length of gwchar_array in bytes.
**	    .dmu_gwattr_array.ptr_address   Pointer to array of pointers, each
**					    of which describes gateway specific
**					    information about a table column.
**					    This is used only if the table is
**					    a DMU_GATEWAY type table.  These
**					    entries are passed directly down to
**					    the Ingres Gateway system.
**	    .dmu_gwattr_array.ptr_size	    The size of each element in array.
**	    .dmu_gwattr_array.ptr_address   The number of pointers in the array.
**
**          <dmu_key_array> entries are of type DMU_KEY_ENTRY and
**          must have following format:
**          key_attr_name                   Name of attribute.
**          key_order                       Must be DMU_ASCENDING.
**
**          <dmu_char_array> entries are of type DMU_CHAR_ENTRY and
**          must have following format:
**          char_id                         Must be one of the dmu 
**                                          characteristics like 
**                                          DMU_STRUCTURE,
**                                          DMU_IFILL,
**					    DMU_DATAFILL,
**					    DMU_LEAFILL,
**                                          DMU_MINPAGES,
**                                          DMU_MAXPAGES,
**                                          DMU_UNIQUE,
**					    DMU_COMPRESSED,
**					    DMU_GATEWAY,
**					    DMU_INDEX_COMP.
**					    DMU_CONCURRENT_ACCESS
**					    DMU_DIMENSION
**					    DMU_TABLE_PRIORITY
**          char_value                      The value to associate with above
**                                          characteristic.
**
** Output:
**      dmu_cb 
**          .dmu_idx_id                     The internal table identifier 
**                                          assigned to this index.
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM0006_BAD_ATTR_FLAG
**                                          E_DM0007_BAD_ATTR_NAME
**                                          E_DM0009_BAD_ATTR_SIZE
**                                          E_DM0008_BAD_ATTR_PRECISION
**                                          E_DM000A_BAD_ATTR_TYPE
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**					    E_DM000D_BAD_CHAR_ID
**					    E_DM000E_BAD_CHAR_VALUE
**                                          E_DM0010_BAD_DB_ID
**                                          E_DM001C_BAD_KEY_SEQUENCE.
**                                          E_DM001D_BAD_LOCATION_NAME.
**                                          E_DM001E_DUP_LOCATION_NAME.
**                                          E_DM001A_BAD_FLAG
**                                          E_DM0021_TABLES_TOO_MANY
**                                          E_DM002A_BAD_PARAMETER
**                                          E_DM0039_BAD_TABLE_NAME
**                                          E_DM003A_BAD_TABLE_OWNER
**                                          E_DM003B_BAD_TRAN_ID
**                                          E_DM0042_DEADLOCK
**                                          E_DM0045_DUPLICATE_KEY
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM004B_LOCK_QUOTA_EXCEEDED
**                                          E_DM0054_NONEXISTENT_TABLE
**                                          E_DM0059_NOT_ALL_KEYS
**                                          E_DM005D_TABLE_ACCESS_CONFLICT
**                                          E_DM005E_CANT_UPDATE_SYSCAT
**                                          E_DM005F_CANT_INDEX_CORE_SYSCAT
**                                          E_DM0064_USER_INTR
**                                          E_DM0065_USER_ABORT
**	    				    E_DM006A_TRAN_ACCESS_CONFLICT
**                                          E_DM0071_LOCATIONS_TOO_MANY
**                                          E_DM0072_NO_LOCATION
**                                          E_DM0078_TABLE_EXISTS
**					    E_DM007D_BTREE_BAD_KEY_LENGTH
**					    E_DM010F_ISAM_BAD_KEY_LENGTH
**					    E_DM0110_COMP_BAD_KEY_LENGTH
**					    E_DM0092_ERROR_INDEXING_TABLE
**                                          E_DM009F_ILLEGAL_OPERATION
**                                          E_DM0100_DB_INCONSISTENT
**                                          E_DM0103_TUPLE_TOO_WIDE
**                                          E_DM010C_TRAN_ABORTED
**                                          E_DM0125_DDL_SECURITY_ERROR
**
**          .error.err_data                 Set to attribute in error by 
**                                          returning index into attribute list.
**      Returns:
**          E_DB_OK                         Function completed normally. 
**          E_DB_WARN                       Function completed normally with 
**                                          a termination status which is in 
**                                          dmu_cb.error.err_code.
**          E_DB_ERROR                      Function completed abnormally with 
**                                          a termination status which is in
**                                          dmu_cb.error.err_code.
**          E_DB_FATAL                      Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmu_cb.error.err_code.
**
** History:
**      01-may-98 (nanpr01) 
**          Created from dmu_index.
**      18-mar-1999 (stial01)
**          Copy err_code, err_data into corresponding dmu cb, not 1st. 
**      01-may-1999 (stial01)
**          Fixed error handling.
**      17-Apr-2001 (horda03) Bug 104402
**          Added support for TCB_NOT_UNIQUE attribute.
**	15-jul-2002 (hayke02)
**	    Initialise systemGenerated et al at the beginning of the
**	    dmu/indx_cb for loop. This prevents non-system generated
**	    (constraint) persistent indices being marked as such after a
**	    system generated index has already been processed. This change
**	    fixes bug 107621.
**      24-jul-2002 (hanal04) Bug 108330 INGSRV 1847
**          Initialise the new indxcb_dmveredo field.
**	22-Dec-2003 (jenjo02)
**	    Added DMU_GLOBAL_INDEX for Partitioned Table Project.
**	6-Feb-2004 (schka24)
**	    Get rid of DMU statement count and its limit.
**	08-jul-2004 (thaju02)
**	    Online Modify - init indxcb_online_fhdr_pageno.
**	    (B112610)
**	11-Mar-2005 (thaju02)
**	    Use $online idxs relation info. (B114069)
**	11-Nov-2005 (jenjo02)
**	    Replaced dmx_show() with the more robust 
**	    dmxCheckForInterrupt() to standardize external
**	    interrupt handling.
**	15-Aug-2006 (jonj)
**	    Moved guts to common dmuIndexSetup().
**      25-oct-2006 (stial01)
**          Fixed initialization of DM2U_INDEX_CB
**	24-Oct-2008 (jonj)
**	    Cleaned up readability, fix potential memory leak.
**	22-Jul-2009 (thaju02)
**	    For E_DM007D, dmu_tup_cnt was not getting set. 
*/
DB_STATUS
dmu_pindex(DMU_CB    *dmu_cbs)
{
    DMU_CB		*dmu = dmu_cbs;
    DMU_CB		*ndmu;
    DM2U_INDEX_CB	*indx_cb, *indx_cbs, *curr_indx_cb = NULL;
    DB_OWN_NAME		table_owner;
    DB_TAB_NAME		table_name;
    DML_XCB		*xcb;
    i4			error,local_error;
    i4			NiX, k, tot_size;
    DB_STATUS		status;

    CLRDBERR(&dmu->error);

    /* Count how many control block was passed */
    ndmu = dmu;

    for ( NiX = 0; ndmu; NiX++ )
	ndmu = (DMU_CB*)ndmu->q_next;

    if (NiX == 1)
    {
	status = dmu_index(dmu);
	return(status);
    }

    tot_size = sizeof(DM2U_INDEX_CB) * NiX; 
    status = dm0m_allocate(tot_size, 0, 
			   (i4)DM2U_IND_CB, (i4)DM2U_IND_ASCII_ID, 
			   (char *)dmu, (DM_OBJECT **)&indx_cbs, &dmu->error);
    if (status != E_DB_OK)
    {
	uleFormat(&dmu->error, 0, NULL, ULE_LOG, NULL, 
		NULL, 0, NULL, &local_error, 0);
	return(E_DB_ERROR);
    }

    ndmu = dmu;
    indx_cb = indx_cbs;

    for (k = 0; k < NiX; k++)
    {
	indx_cb->indxcb_tab_name = &table_name;
	indx_cb->indxcb_tab_owner = &table_owner;

	if ( status = dmuIndexSetup(ndmu, (PTR)indx_cb) )
	{
	    /* copy error info to "first" dmu */
	    dmu->error = ndmu->error;
	    break;
	}

	/* Now link up the control blocks */
	indx_cb->q_next = NULL;
	indx_cb->q_prev = NULL;
	if (curr_indx_cb != (DM2U_INDEX_CB *) NULL)
	    curr_indx_cb->q_next = indx_cb;
	curr_indx_cb = indx_cb;

	ndmu = (DMU_CB*)ndmu->q_next;
	indx_cb++;
    }

    if (dmu->error.err_code)
    {
	if (dmu->error.err_code > E_DM_INTERNAL)
	{
	    uleFormat(&dmu->error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char * )NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	    SETDBERR(&dmu->error, 0, E_DM0092_ERROR_INDEXING_TABLE);
	}

	/* No leaking! */
	dm0m_deallocate((DM_OBJECT **) &indx_cbs);

	return(E_DB_ERROR);
    }

    /* Calls the physical layer to process the rest of the index create */
    status = dm2u_pindex(indx_cbs);

    for (k = 0, ndmu = dmu, indx_cb = indx_cbs; k < NiX; 
	 k++, indx_cb++, ndmu = (DMU_CB *)ndmu->q_next)
    {
	/* Audit successful index on TABLE. */
	if ( status == E_DB_OK && dmf_svcb->svcb_status & SVCB_C2SECURE )
	{
	  status = dma_write_audit(
	     SXF_E_TABLE,
	     SXF_A_SUCCESS | SXF_A_INDEX,
	     ndmu->dmu_index_name.db_tab_name, /* index name */
	     sizeof(ndmu->dmu_index_name.db_tab_name),
	     &ndmu->dmu_owner,		   /* Table/view owner */
	     I_SX2011_INDEX_CREATE,
	     FALSE, /* Not force */
	     &ndmu->error, NULL);
	}

	if ( status )
	{
	    /* Find the first one that got an error */
	    if ( ndmu->error.err_code == 0 )
		continue;

	    if (ndmu->error.err_code > E_DM_INTERNAL)
	    {
		uleFormat(&ndmu->error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		    (char * )NULL, (i4)0, (i4 *)NULL, &local_error, 0);
		SETDBERR(&ndmu->error, 0, E_DM0092_ERROR_INDEXING_TABLE);
	    }
	    else if ( xcb = indx_cb->indxcb_xcb )
		switch (ndmu->error.err_code)
	    {
		case E_DM004B_LOCK_QUOTA_EXCEEDED:
		case E_DM0112_RESOURCE_QUOTA_EXCEED:
		case E_DM0092_ERROR_INDEXING_TABLE:
		case E_DM0045_DUPLICATE_KEY:
	        case E_DM006A_TRAN_ACCESS_CONFLICT:
		    xcb->xcb_state |= XCB_STMTABORT;
		    break;

		case E_DM0042_DEADLOCK:
		case E_DM004A_INTERNAL_ERROR:
		case E_DM0100_DB_INCONSISTENT:
		    xcb->xcb_state |= XCB_TRANABORT;
		    break;
		case E_DM0065_USER_INTR:
		    xcb->xcb_state |= XCB_USER_INTR;
		    break;
		case E_DM010C_TRAN_ABORTED:
		    xcb->xcb_state |= XCB_FORCE_ABORT;
		    break;
		case E_DM007D_BTREE_BAD_KEY_LENGTH:
		    ndmu->dmu_tup_cnt = indx_cb->indxcb_maxklen;
		    break;
	    }
	    break;
	}
    } 

    dm0m_deallocate((DM_OBJECT **) &indx_cbs);

    return(status);
}
