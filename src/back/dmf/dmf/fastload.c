/*
**Copyright (c) 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <si.h>
#include    <cs.h>
#include    <di.h>
#include    <pc.h>
#include    <lo.h>
#include    <st.h>
#include    <bt.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <er.h>
#include    <pm.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <scf.h>
#include    <adf.h>
#include    <adp.h>
#include    <dm.h>
#include    <dmtcb.h>
#include    <dmp.h>
#include    <dm0c.h>
#include    <dmfjsp.h>
#include    <dm2d.h>
#include    <dm2t.h>
#include    <dm2r.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmxcb.h>
#include    <dml.h>
#include    <dm0m.h>
#include    <dm0pbmcb.h>
#include    <dmucb.h>
#include    <dmpecpn.h>

#define MAX_SEG_SIZE  MAXI2    /* Maximum (default?) segment size */
/*
** Name: FASTLOAD.C - fast load a database table
**
** Description:
**	This file contains all the functions to implement the fast load option
**	of dmfjsp, it was made part of the journal support program for
**	rapid development of a prototype, and will probably be moved at some
**	later date.
**
**	This file contains the routines:
**		dmffload - main fastloader routine.
**		dml_get_tabid - get table ID using table name/owner
**
** History:
**	15-apr-94 (stephenb)
**	    First written.
**	10-may-95 (sarjo01)
**	    First enhancement.
**	26-oct-95 (stephenb)
**	    Fix incorrect usage of lig_id in xcb, we were using the dcb log_id
**	    we should be using the one returned from dmxe_begin()
**	20-nov-95 (stephenb)
**	    Add warning regarding record size and table width.
**	30-nov-95 (emmag)
**	    Fixed length strings must be copied with MEcopy and not
**	    STcopy.  Fixed typo in previous submission.
**      06-mar-96 (stial01)
**          Variable Page Size project:
**          dml_get_tabid() Make sure buffers are allocated for the fastload
**          page size.
**	13-dec-96 (stephenb)
**	    Add code to check if table is empty, we also check for 
**	    journaling ans secondary indexes
**	6-9 jan-97 (stephenb)
**	    Start coding support for long datatypes, add new routine
**	    handle_peripherals() and re-orgainse code in dmffload.
**	31-jan-97 (stephenb)
**	    Fix check for bad table types, should use absolute
**	    value of type since it can be negative.
**	1-5 feb-97 (stephenb)
**	    Complete coding of support for long datatypes, also tidy up
**	    messages and error handling.
**	16-feb-97 (mcgem01)
**	    scb_isolation_level was renamed scb_tran_iso as part of 
**	    the set lockmode cleanup.
**	30-Apr-97 (fanra01)
**          Bug 81629 Use SI_BIN in SIfopen call since data file is binary.
**          This only affects platforms which distinguish between text and
**          binary files.
**	14-may-97 (stephenb)
**	    Remove over-restrictive check for empty tables, users can add 
**	    records and then delete them, and the table will appear to have
**	    data in. The actual table status is checked immediately after with
**	    an attemped get anyway (bug 81517)
**	4-aug-97 (stephenb)
**	    Add functionality to allow the load file to be read from
**	    standard input. Also correct code logic when the total number
**	    of rows is an exact muliple of the rows per read.
**      15-aug-97 (stephenb)
**          add DML_SCB parameter to dm2t_open() call.
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to dm2t_unfix_tcb() calls.
**	27-Oct-1997 (hanch04)
**	    Fix bad integration 
**      12-nov-97 (stial01)
**          dmffload() Init scb_tran_iso = DMC_C_SYSTEM;
**	14-jul-1998 (hanch04)
**	    Added debug output
**	22-jul-98 (stephenb)
**	    Fix up various problems in fastload of blobs, many changes in the 
**	    last year have broken it
**	13-aug-1998 (somsa01)
**	    Cleaned up handle_peripheral() so that it would work properly.
**	    Also needed to initialize some more variables in dmffload().
**	19-aug-1998 (somsa01)
**	    In dmffload(), a call to dm0m_deallocate() was mysteriously
**	    missing.
**	19-aug-1998 (somsa01)
**	    Cleaned up a messed up cross-integration by last submission.
**      02-sep-98 (musro02)
**          Copied code from dmx_commit() to process the XCCB.
**          All of this so that the temp file created by rename_load_tab()
**          is properly deleted.
**	17-dec-1998 (somsa01)
**	    In dmffload(), when dealing with UDTs which are not long
**	    datatypes, re-adjust the row size to contain their TRUE size as
**	    defined by the user.  (SIR #79167)
**	03-Dec-1998 (jenjo02)
**	    For raw files, restore page size from XCCB to FCB.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**      02-nov-1999 (stial01)
**          MEfill DML_SCB, to make sure scb_sess_mask is initialized
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	30-Nov-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID* to dmxe_begin() prototype.
**	08-Nov-2000 (jenjo02)
**	    Deleted xcb_tq_next|prev, xccb_tq_next|prev.
**	08-Jan-2001 (jenjo02)
**	    Changed SCD_INITSNGL to return DML_SCB offset.
**	05-Mar-2002 (jenjo02)
**	    Init new XCB fields xcb_seq, xcb_cseq.
**      07-Jun-2002 (stial01)
**          Support segments up to MAXI2, check for bad segment length (b105406)
**	14-aug-2003 (jenjo02)
**	    dm2t_unfix_tcb() "tcb" parm now pointer-to-pointer,
**	    obliterated by unfix to prevent multiple unfixes
**	    by the same transaction and corruption of 
**	    tcb_ref_count.
**	26-aug-2003 (jenjo02)
**	    Missed changing one instance of dm2t_unfix_tcb() to
**	    be **tab_tcb.
**      18-feb-2004 (stial01)
**          Increase maximum row size to support 256k rows.
**	16-dec-04 (inkdo01)
**	    Added various collID's.
**      24-jan-2005 (stial01)
**          Fixes for fastload long nvarchar DB_LNVCHR_TYPE (b113792)
**	25-apr-2005 (abbjo03)
**	    Fix call to dm2f_delete_file where err_code is already a pointer.
**      10-jan-2008 (stial01)
**          Support fastload [+w|-w]
**	11-Apr-2008 (kschendel)
**	    dm2r position call updated, fix here.
**	12-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	18-Nov-2008 (jonj)
**	    dm2d_? functions converted to DB_ERROR *
**	21-Nov-2008 (jonj)
**	    SIR 120874: dmxe_? functions converted to DB_ERROR *
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2f_?, dm2t_?, dm2r_? functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dm0p_? functions converted to DB_ERROR *
**      18-feb-2009 (stial01)
**          fixed format string for i8
**      01-apr-2010 (stial01)
**          Changes for Long IDs
*/
/* forward and/or static references */
static DB_STATUS
handle_peripheral(
		DMF_JSX		*jsx,
		char		*readbuffer, 
		FILE		*fdesc,
		DMP_RCB		*tab_rcb,
		DMP_TCB		*tab_tcb,
		i4		att_num);
/*
** Name: dmffload - main fastload entry point from dmfjsp
**
** Description:
**	Set up environment for fast loading, will currently only handle
**	a single table/file in a single local database.
**
** Inputs:
**	jsx
**	dcb
**
** Outputs:
**	err_code
**
** Returns:
**	None.
**
** History:
**	15-apr-94 (stephenb)
**	    First created as a prototype.
**	10-may-95 (sarjo01)
**	    First enhancement. 
**	13-dec-96 (stephenb)
**	    Add code to check if table is empty, we also check for 
**	    journaling ans secondary indexes
**	6-9 jan-97 (stephenb)
**	    Start coding support for long datatypes, add new routine
**	    handle_peripherals() and re-orgainse code in dmffload.
**	1-5 feb-97 (stephenb)
**	    Complete coding of support for long datatypes, also tidy up
**	    messages and error handling.
**      30-Apr-97 (fanra01)
**          Bug 81629  Modified SIfopen to use SI_BIN as binary character was
**          interpreted as EOF and prematurely terminated data file.
**	14-may-97 (stephenb)
**	    Remove over-restrictive check for empty tables, users can add 
**	    records and then delete them, and the table will appear to have
**	    data in. The actual table status is checked immediately after with
**	    an attemped get anyway (bug 81517)
**      12-nov-97 (stial01)
**          dmffload() Init scb_tran_iso = DMC_C_SYSTEM;
**	13-aug-1998 (somsa01)
**	    Needed to initialize dml_xcb.xcb_tq_next and dml_xcb.xcb_tq_prev
**	    to NULL.
**	19-aug-1998 (somsa01)
**	    A call to dm0m_deallocate() was mysteriously missing.
**	17-dec-1998 (somsa01)
**	    When dealing with UDTs which are not long datatypes, re-adjust
**	    the row size to contain their TRUE size as defined by the user.
**	    (SIR #79167)
**      20-May-2000 (hanal04, stial01 & thaju02) Bug 101418 INGSRV 1170
**          Initialise the scb lock list as part of the move from TX
**          lock list to Session lock list for global temporary table
**          lock ID locks.
**	17-Aug-2000 (thaju02)
**	    If table empty, set DM2R_EMPTY flag upon calling dm2r_load for 
**	    DM2R_L_BEGIN operation. (B102383)
**	03-Dec-1998 (jenjo02)
**	    For raw files, restore page size from XCCB to FCB.
**	21-mar-1999 (nanpr01)
**	    Supporting raw locations.
**      15-jun-1999 (hweho01)
**          Corrected argument mismatch in ule_format() call,
**          the first one should be an integer, not a pointer.
**      20-sep-2004 (huazh01)
**          changed prototype for dmxe_commit(). 
**          INGSRV2643, b111502.
**	6-Feb-2004 (schka24)
**	    Remove DMU count.
**	12-May-2004 (schka24)
**	    Put lock-id in fake scb as well as xcb, for temp blob holders.
**	    (temp) prohibit load into partitioned tables, notimp for now.
**	21-Jun-2004 (schka24)
**	    Remove SCB blob-cleanup list, xccb list used now.
**	25-apr-2005 (abbjo03)
**	    Fix call to dm2f_delete_file where err_code is already a pointer.
**	28-Jul-2005 (schka24)
**	    Back out x-integration for bug 111502, doesn't
**	    apply to r3.
**	01-Aug-2005 (sheco02)
**	    Fixed the typo in change 478179.
**	26-Jun-2006 (kschendel)
**	    Pass correct buffer-manager type to DB open in case of cache
**	    sharing (the -cache_name thing).
**	13-Nov-2006 (kschendel)
**	    Take out "norelupdate" flag, was preventing update of row/page
**	    counts after nonrebuild/nonrecreate load (ie load into existing
**	    table).  Remove redundant fix of table TCB.  Run fastload in
**	    logging mode, not nologging; logging will be minimal anyway,
**	    and we don't need broken tables if fastloading aborts.
**	    Don't pass bogus row estimate to DMF, use large number.
**	11-Jan-2008 (kschendel) b122122
**	    Remove nosync on the dcb.  This eventually leads to no syncing
**	    when nonempty heaps are loaded.
**	17-Apr-2008 (kibro01) b120276
**	    Initialise ADF_CB structure
**	25-Nov-2008 (wanfr01) 
**	    Bug 121280
**	    Flush output to fix HOQA diffs.
**	9-sep-2009 (stephenb)
**	    Add dmf_last_id FEXI.
**	16-Nov-2009 (kschendel) SIR 122890
**	    Change fastload to use logical layer txn services instead of
**	    calling dmxe.  It's more appropriate, and would allow change-
**	    tracking to work if we ever implement that.
**	    Make sure the fastcommit flag is set in the DCB if appropriate;
**	    otherwise we can fail to set WCOMMIT and end up with DM9302
**	    errors.
**	16-Mar-2010 (troal01)
**	    Added ADF FEXI dmf_get_srs.
**	19-Apr-2010 (kschendel) SIR 123485
**	    Various changes for new blob handling, which among other things
**	    requires the use of DML level table open here.
*/
DB_STATUS
dmffload( DMF_JSX	*jsx,
	  DMP_DCB	*dcb,
          char		*owner,
          i4            buffsize)
{
    DML_SCB             dml_scb;
    DML_XCB		*xcb;
    DML_ODCB		dml_odcb;
    DML_LOC_MASKS	*dml_loc_masks = 0;
    DMP_EXT		*ext;
    DMX_CB		dmx;
    SCF_CB              scf_cb;
    DMT_CB		dmt;
    DMP_RCB		*tab_rcb = NULL, *master_rcb = NULL;
    DMP_TCB		*tab_tcb = NULL, *master_tcb = NULL;
    DM_MDATA		*load_data;
    char		*table;
    char		*inputfile;
    DB_TAB_NAME		tab_name;
    DB_OWN_NAME		tab_owner;
    DB_OWN_NAME		cat_owner;
    DB_TAB_ID		tab_id;
    DB_TAB_TIMESTAMP	timestamp;
    DB_STATUS		status;
    bool		dbopen = FALSE;
    bool		open_xact = FALSE;
    bool		file_open = FALSE;
    bool 		table_open = FALSE;
    i4		size;
    STATUS		cl_stat;
    LOCATION		oloc;
    FILE		*fdesc;
    char		*ptr;
    char		*readbuffer;
    i4		bytes_read;
    i4		i;
    DB_STATUS		local_stat;
    i4		local_err;
    STATUS		local_clstat;
    i4			row_size;
    i4			nrows;
    i4			rows_read = 0;
    i4			done = 0;
    i4			rows_done = 0;
    i4             input_bsize, max_bytes_per_read;
    DM_TID		tid;
    char		*rec_buf;
    i4			start_offset = 0;
    i4			record_offset = 0;
    bool		have_peripherals = FALSE;
    ADF_CB              adf_scb;
    i4			att_num;
    i4			start_att;
    CL_ERR_DESC         sys_err;
    i4		flag;
    u_i4		xlate = 0;
    char		*etab_struct;
    i4			dt_bits;
    bool		table_empty = TRUE;
    i4			flags;
    char		*str;
    i4			partition_no;
    i4			open_flags;
    DB_ERROR		local_dberr;

    CLRDBERR(&jsx->jsx_dberr);

    MEfill(sizeof(ADF_CB),0,(PTR)&adf_scb);

    /* Stub out non-integrated Datallegro change that allows one to specify
    ** the physical partition number on the command line -- insufficiently
    ** robust for general distribution.
    */
    partition_no = -1;



    if (jsx->jsx_status & JSX_TRACE)
    {
#ifdef VMS
            flag = TR_F_OPEN;
#else
            flag = TR_F_APPEND;
#endif
        TRset_file(flag, "fastload.dbg", 12, &sys_err);
    }

    input_bsize = buffsize * 0x400; 
    if (input_bsize < 0x8000 || input_bsize > 0x100000)
        input_bsize = 0x100000;
    if (jsx->jsx_tbl_cnt < 1)
    {
	uleFormat(&jsx->jsx_dberr, E_DM1612_FLOAD_BADARGS, (CL_ERR_DESC *)NULL, 
	    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	return (E_DB_ERROR);
    }
    table = jsx->jsx_tbl_list[0].tbl_name.db_tab_name;
    MEcopy(owner, DB_OWN_MAXNAME, tab_owner.db_own_name);

    if (jsx->jsx_fname_cnt < 1)
    {
	uleFormat(&jsx->jsx_dberr, E_DM1612_FLOAD_BADARGS, (CL_ERR_DESC *)NULL, 
	    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	return (E_DB_ERROR);
    }
    inputfile = jsx->jsx_fname_list[0];
    adf_scb.adf_errcb.ad_emsglen = 0;
    adf_scb.adf_errcb.ad_errmsgp = 0;
    /*
    ** init DML session control block
    */
    MEfill(sizeof(DML_SCB), 0, &dml_scb);
    dml_scb.scb_q_next = &dml_scb;
    dml_scb.scb_q_prev = &dml_scb;
    dml_scb.scb_length = sizeof(DML_SCB);
    dml_scb.scb_type = SCB_CB;
    dml_scb.scb_ascii_id = SCB_ASCII_ID;
    dml_scb.scb_oq_next = (DML_ODCB *)&dml_scb.scb_oq_next;
    dml_scb.scb_oq_prev = (DML_ODCB *)&dml_scb.scb_oq_next;
    dml_scb.scb_x_next = (DML_XCB *)&dml_scb.scb_x_next;
    dml_scb.scb_x_prev = (DML_XCB *)&dml_scb.scb_x_next;
    dml_scb.scb_kq_next = (DML_SLCB *)&dml_scb.scb_kq_next;
    dml_scb.scb_kq_prev = (DML_SLCB *)&dml_scb.scb_kq_next;
    dml_scb.scb_s_type = SCB_S_NORMAL;
    dml_scb.scb_ui_state = 0;
    dml_scb.scb_state = 0;
    dml_scb.scb_loc_masks = NULL;
    dml_scb.scb_audit_state = SCB_A_FALSE;
    MEcopy(DB_INGRES_NAME, DB_OWN_MAXNAME, cat_owner.db_own_name);
    MEcopy((PTR) &jsx->jsx_username, sizeof(DB_OWN_NAME), (PTR) &dml_scb.scb_user);
    dml_scb.scb_cat_owner = &cat_owner;
    dml_scb.scb_tran_iso = DMC_C_SYSTEM;
    dml_scb.scb_timeout = DMC_C_SYSTEM;
    dml_scb.scb_maxlocks = DMC_C_SYSTEM;
    dml_scb.scb_lock_level = DMC_C_SYSTEM;
    dml_scb.scb_sess_iso = DMC_C_SYSTEM;
    dml_scb.scb_readlock = DMC_C_SYSTEM;
    dml_scb.scb_dbxlate = &xlate;
    dml_scb.scb_lock_list = dmf_svcb->svcb_lock_list;
    CSget_sid(&dml_scb.scb_sid);
    dml_scb.scb_adf_cb = &adf_scb;
    dm0s_minit(&dml_scb.scb_bqcb_mutex, "BQCB mutex");
    /*
    ** Init DML Open database control block, and queue it to the session
    ** control block (above)
    */
    MEfill(sizeof(DML_ODCB), 0, &dml_odcb);
    dml_odcb.odcb_q_next = &dml_odcb;
    dml_odcb.odcb_q_prev = &dml_odcb;
    dml_odcb.odcb_length = sizeof(DML_ODCB);
    dml_odcb.odcb_type = ODCB_CB;
    dml_odcb.odcb_ascii_id = ODCB_ASCII_ID;
    dml_odcb.odcb_scb_ptr = &dml_scb;
    dml_odcb.odcb_cq_next = dml_odcb.odcb_cq_prev = 
	(DML_XCCB *)&dml_odcb.odcb_cq_next;
    /* queue it */
    dml_odcb.odcb_q_next = dml_scb.scb_oq_next;
    dml_odcb.odcb_q_prev = (DML_ODCB *)&dml_scb.scb_oq_next;
    dml_scb.scb_oq_next->odcb_q_prev = &dml_odcb;
    dml_scb.scb_oq_next = &dml_odcb;

    /*
    ** get needed PM values
    */
    cl_stat = PMget("!.blob_etab_structure", &etab_struct);
    if (cl_stat == PM_NOT_FOUND || etab_struct == NULL || *etab_struct == 0)
    {
	dmf_svcb->svcb_blob_etab_struct = DB_HASH_STORE;
    }
    else if (STcasecmp(etab_struct, "hash" ) == 0)
    {
        dmf_svcb->svcb_blob_etab_struct = DB_HASH_STORE;
    }
    else if (STcasecmp(etab_struct, "btree" ) == 0)
    {
        dmf_svcb->svcb_blob_etab_struct = DB_BTRE_STORE;
    }
    else if (STcasecmp(etab_struct, "isam" ) == 0)
    {
        dmf_svcb->svcb_blob_etab_struct = DB_ISAM_STORE;
    }
    else
        dmf_svcb->svcb_blob_etab_struct = DB_HASH_STORE;

    /*
    ** Setup the FEXI functions
    */
    {
        FUNC_EXTERN DB_STATUS   dmf_tbl_info();
        FUNC_EXTERN DB_STATUS	dmf_last_id();
        FUNC_EXTERN DB_STATUS	dmf_get_srs();

 
        status = adg_add_fexi(&adf_scb, ADI_01PERIPH_FEXI, dmpe_call);
        if (status != E_DB_OK)
        {
            uleFormat(&adf_scb.adf_errcb.ad_dberror, 0,
                NULL, ULE_LOG, NULL, 0, 0, 0, &local_err, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM007F_ERROR_STARTING_SERVER);
            return(E_DB_ERROR);
        }

        status = adg_add_fexi(&adf_scb, ADI_03ALLOCATED_FEXI,
                              dmf_tbl_info );
        if (status != E_DB_OK)
        {
            uleFormat(&adf_scb.adf_errcb.ad_dberror, 0,
                NULL, ULE_LOG, NULL, 0, 0, 0, &local_err, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM007F_ERROR_STARTING_SERVER);
            return(E_DB_ERROR);
        }
 
        status = adg_add_fexi(&adf_scb, ADI_04OVERFLOW_FEXI,
                              dmf_tbl_info );
        if (status != E_DB_OK)
        {
            uleFormat(&adf_scb.adf_errcb.ad_dberror, 0,
                NULL, ULE_LOG, NULL, 0, 0, 0, &local_err, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM007F_ERROR_STARTING_SERVER);
            return(E_DB_ERROR);
        }
        
	status = adg_add_fexi(&adf_scb, ADI_07LASTID_FEXI, 
			      dmf_last_id );
        if (status != E_DB_OK)
        {
            uleFormat(&adf_scb.adf_errcb.ad_dberror, 0,
                NULL, ULE_LOG, NULL, 0, 0, 0, &local_err, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM007F_ERROR_STARTING_SERVER);
            return(E_DB_ERROR);
        }

	    status = adg_add_fexi(&adf_scb, ADI_08GETSRS_FEXI,
				  dmf_get_srs );
        if (status != E_DB_OK)
        {
            uleFormat(&adf_scb.adf_errcb.ad_dberror, 0,
                NULL, ULE_LOG, NULL, 0, 0, 0, &local_err, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM007F_ERROR_STARTING_SERVER);
            return(E_DB_ERROR);
        }
    }
    /*
    ** Open the load file
    */
    if (STcompare(inputfile, "-") == 0)
	fdesc = stdin;
    else
    {
        _VOID_ LOfroms(FILENAME & PATH, inputfile, &oloc);
        cl_stat = SIfopen(&oloc, "r", SI_BIN, 0, &fdesc);
        if (cl_stat != OK)
        {
            uleFormat(&jsx->jsx_dberr, E_DM1600_FLOAD_OPEN_FILE, (CL_ERR_DESC *)NULL, 
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
            return (E_DB_ERROR);
        }
    }
    file_open = TRUE;
    /*
    ** Open the database.
    ** The DCB is in DCB_MULTIPLE state, so -sole_server won't fly if
    ** a real dbms server has the DB open.  But, if we attached to a
    ** shared cache, we can allow -sole_cache opens to work.
    ** Fastloading iidbdb is insane, but check anyway.  (iidbdb is
    ** always multi-cache.)
    ** We probably ought to be using ADD here, but we don't.  Make
    ** sure to initialize the fastcommit flag, nothing else will.
    */
    dcb->dcb_bm_served = DCB_MULTIPLE;
    if (dcb->dcb_id != 1 && jsx->jsx_status2 & JSX2_MUST_DISCONNECT_BM)
    {
	dcb->dcb_bm_served = DCB_SINGLE;
	/* Turn on fast commit.  An interesting situation may arise if
	** the DBMS server is using shared cache but slow commit;  it's
	** not clear that fast and slow commit can coexist in a shared
	** cache.  I have better things to worry about today, though.
	** (dm0p certainly isn't checking for any discrepancy.)
	*/
	dcb->dcb_status |= DCB_S_FASTCOMMIT;
    }
    if (jsx->jsx_status & JSX_WAITDB)
	open_flags = 0;
    else
	open_flags = DM2D_NOWAIT;
    status = dm2d_open_db(dcb, DM2D_A_WRITE, DM2D_IX,
                          dmf_svcb->svcb_lock_list, open_flags, 
                          &jsx->jsx_dberr);
    if (status != E_DB_OK)
    {
	if (jsx->jsx_dberr.err_code == E_DM004C_LOCK_RESOURCE_BUSY)
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM161B_FLOAD_DB_BUSY);
	    return (E_DB_INFO);
	}
        uleFormat(&jsx->jsx_dberr, E_DM1601_FLOAD_OPEN_DB, (CL_ERR_DESC *)NULL, 
	    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
        return (E_DB_ERROR);
    }
    dbopen = TRUE;
    dml_scb.scb_o_ref_count = 1;
    dml_odcb.odcb_dcb_ptr = dcb;
    dml_odcb.odcb_access_mode = ODCB_A_WRITE;
    dml_odcb.odcb_lk_mode = ODCB_IX;

    for (;;)
    {
	/*
	** init location masks from DCB info
	*/
        ext = dcb->dcb_ext;
        size = sizeof(i4) *
	    (ext->ext_count+BITS_IN(i4)-1)/BITS_IN(i4);
        status = dm0m_allocate((i4)sizeof(DML_LOC_MASKS) + 3*size,
	    DM0M_ZERO, (i4)LOCM_CB, (i4)LOCM_ASCII_ID, 
	    (char *)&dml_scb, (DM_OBJECT **)&dml_loc_masks, &jsx->jsx_dberr);
        if (status != E_DB_OK)
        {
            uleFormat(&jsx->jsx_dberr, E_DM160D_FLOAD_NOMEM, (CL_ERR_DESC *)NULL, 
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
            break;
        }
	
        dml_scb.scb_loc_masks = dml_loc_masks;
        dml_loc_masks->locm_bits = size * BITSPERBYTE;
        dml_loc_masks->locm_w_allow = 
	    (i4 *)((char *)dml_loc_masks + sizeof(DML_LOC_MASKS));
	dml_loc_masks->locm_w_use = 
	    (i4 *)((char *)dml_loc_masks + sizeof(DML_LOC_MASKS) + size);
	dml_loc_masks->locm_d_allow =
	    (i4 *)((char *)dml_loc_masks + sizeof(DML_LOC_MASKS) + 
	    size + size);

        for (i=0; i < ext->ext_count; i++)
        {
            if (ext->ext_entry[i].flags & DCB_WORK)
            {
                BTset(i, (char *)dml_loc_masks->locm_w_allow);
                BTset(i, (char *)dml_loc_masks->locm_w_use);
            }
 
            if (ext->ext_entry[i].flags & DCB_AWORK)
                BTset(i, (char *)dml_loc_masks->locm_w_allow);
 
            if (ext->ext_entry[i].flags & DCB_DATA)
                BTset(i, (char *)dml_loc_masks->locm_d_allow);
        }
        /*
        ** Init SCF for blob functionality
        */
        scf_cb.scf_length = sizeof(SCF_CB);
        scf_cb.scf_type = SCF_CB_TYPE;
        scf_cb.scf_facility = DB_DMF_ID;
        scf_cb.scf_session = (SCF_SESSION) DB_NOSESSION;
        scf_cb.scf_ptr_union.scf_dml_scb = (PTR)&dml_scb;

        status = scf_call(SCD_INITSNGL, &scf_cb);
        if (status != E_DB_OK)
	{
            uleFormat(&jsx->jsx_dberr, E_DM1618_FLOAD_INIT_SCF, (CL_ERR_DESC *)NULL, 
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	    break;
	}
	/* SCF returns offset from SCF's SCB to DML_SCB* */
	dmf_svcb->svcb_dmscb_offset = scf_cb.scf_len_union.scf_ilength;

	/*
	** start a transaction
	*/
	MEfill(sizeof(dmx), 0, (PTR) &dmx);
	dmx.q_next = dmx.q_prev = (PTR) &dmx;
	dmx.length = sizeof(dmx);
	dmx.type = DMX_TRANSACTION_CB;
	dmx.ascii_id = DMX_ASCII_ID;
	dmx.dmx_session_id = (PTR) dml_scb.scb_sid;
	dmx.dmx_db_id = (PTR) &dml_odcb;
	dmx.dmx_option = DMX_USER_TRAN;
	/* flags-mask stays zero for write transaction */
	status = dmx_begin(&dmx);
	if (status != E_DB_OK)
	{
	    jsx->jsx_dberr = dmx.error;
	    uleFormat(&jsx->jsx_dberr, E_DM1602_FLOAD_START_XACT,(CL_ERR_DESC *)NULL, 
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	    break;
	}
	xcb = (DML_XCB *)dmx.dmx_tran_id;
	/* Lock id needs to be in SCB too, for temp etab holders */
	dml_scb.scb_lock_list = xcb->xcb_lk_id;
	open_xact = TRUE;

	/*
	** get table ID from name
	*/
    	MEcopy(table, DB_TAB_MAXNAME, tab_name.db_tab_name);

	status = dml_get_tabid(dcb, &tab_name, &tab_owner, &tab_id, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&jsx->jsx_dberr, E_DM1603_FLOAD_SHOW, (CL_ERR_DESC *)NULL, 
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	    break;
	}
	/*
	** open the table
	*/
	MEfill(sizeof(dmt), 0, (PTR) &dmt);
	dmt.length = sizeof(DMT_CB);
	dmt.type = DMT_TABLE_CB;
	dmt.ascii_id = DMT_ASCII_ID;
	dmt.dmt_record_access_id = NULL;
	dmt.dmt_tran_id = (PTR) xcb;
	dmt.dmt_db_id = (PTR) &dml_odcb;
	dmt.dmt_lock_mode = DMT_X;
	if (partition_no != -1)
	    dmt.dmt_lock_mode = DMT_IX;
	dmt.dmt_update_mode = DMT_U_DIRECT;
	dmt.dmt_access_mode = DMT_A_WRITE;
	dmt.dmt_flags_mask = DMT_DBMS_REQUEST;
	dmt.dmt_id = tab_id;
	status = dmt_open(&dmt);
	if (status != E_DB_OK)
	{
	    jsx->jsx_dberr = dmt.error;
	    uleFormat(&jsx->jsx_dberr, E_DM1604_FLOAD_OPEN_TABLE,(CL_ERR_DESC *)NULL, 
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	    break;
	}
	tab_rcb = (DMP_RCB *) dmt.dmt_record_access_id;
	tab_tcb = tab_rcb->rcb_tcb_ptr;
	table_open = TRUE;
	/*
	** at this point we check if the table can be fastladed, it must be
	** an empty indexed table, or if it containse records, a heap, and
	** should not be journalled or indexed
	*/
	if (tab_tcb->tcb_rel.relstat & (TCB_IDXD | TCB_JOURNAL | TCB_GATEWAY))
	{
	    uleFormat(&jsx->jsx_dberr, E_DM1615_FLOAD_BAD_TABLE_FORMAT, (CL_ERR_DESC *)NULL, 
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	    break;
	}

	/* ***** TEMP UNTIL WE HAVE PARTITIONED LOAD DONE */
	/* Error Condition 1: table is partitioned but user does not specify a partition_no */
	if ((tab_tcb->tcb_rel.relstat & TCB_IS_PARTITIONED)&&(-1==partition_no))
	{
	    SIprintf("Partitioned tables may not (yet) be fastloaded.\n");
	    uleFormat(&jsx->jsx_dberr, E_DM1615_FLOAD_BAD_TABLE_FORMAT, (CL_ERR_DESC *)NULL,
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	    break;
	}
	/* Error Condition 2: table is not partitioned but user specified a partition_no */
	if (!(tab_tcb->tcb_rel.relstat & TCB_IS_PARTITIONED)&&(-1!=partition_no))
	{
	    SIfprintf(stderr, "The table is not partitioned, therefore, the partition number you specified is invalid.\n");
	    uleFormat(&jsx->jsx_dberr, E_DM1615_FLOAD_BAD_TABLE_FORMAT, (CL_ERR_DESC *)NULL,
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	    break;
	}

	if(-1!=partition_no)
	{   /* *** CAN'T GET HERE */
	    /* When loading, tab_xxx will always be the object we load into, now matter
	    ** 1) unpartitioned table (where tab_xxx point to the table) or
	    ** 2) partitioned table (where tab_xxx point to the real partition)
	    */
	    master_rcb=tab_rcb;
	    master_tcb=tab_tcb;
	    tab_rcb=NULL;
	    tab_tcb=NULL;
	    dmt.dmt_id=master_tcb->tcb_pp_array[partition_no].pp_tabid;
	    dmt.dmt_flags_mask = 0;
	    status = dmt_open(&dmt);
	    if (status != E_DB_OK)
	    {
		jsx->jsx_dberr = dmt.error;
		uleFormat(&jsx->jsx_dberr, E_DM1604_FLOAD_OPEN_TABLE,(CL_ERR_DESC *)NULL, 
		    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
		break;
	    }
	    tab_rcb = (DMP_RCB *) dmt.dmt_record_access_id;
	    tab_tcb = tab_rcb->rcb_tcb_ptr;
	    /*
	    ** at this point we check if the table can be fastladed, it must be
	    ** an empty indexed table, or if it containse records, a heap, and
	    ** should not be journalled or indexed
	    */
	    if (tab_tcb->tcb_rel.relstat & (TCB_IDXD | TCB_JOURNAL | TCB_GATEWAY))
	    {
		uleFormat(&jsx->jsx_dberr, E_DM1615_FLOAD_BAD_TABLE_FORMAT, (CL_ERR_DESC *)NULL, 
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
		break;
	    }
	}

	/* final check, see if we can get a row from the table */
	status = dm2r_position(tab_rcb, DM2R_ALL, (DM2R_KEY_DESC *) 0,
		0, &tid, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&jsx->jsx_dberr, E_DM1617_FLOAD_TABLE_ERROR, (CL_ERR_DESC *)NULL, 
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	    break;
	}
	rec_buf = MEreqmem(0, tab_tcb->tcb_rel.relwid, FALSE, &status);
	if (rec_buf == NULL || status != OK)
	{
	    uleFormat(&jsx->jsx_dberr, E_DM1617_FLOAD_TABLE_ERROR, (CL_ERR_DESC *)NULL, 
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	    break;
	}
	status = dm2r_get(tab_rcb, &tid, DM2R_GETNEXT, rec_buf, &jsx->jsx_dberr);
	if (status != E_DB_OK && jsx->jsx_dberr.err_code != E_DM0055_NONEXT)
	{
	    uleFormat(&jsx->jsx_dberr, E_DM1617_FLOAD_TABLE_ERROR, (CL_ERR_DESC *)NULL, 
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	    (VOID)MEfree(rec_buf);
	    break;
	}
	if (status == E_DB_OK)
	{
	    if (tab_tcb->tcb_rel.relspec != TCB_HEAP)
	    {
	    	uleFormat(&jsx->jsx_dberr, E_DM1616_FLOAD_TABLE_NOT_EMPTY, (CL_ERR_DESC *)NULL, 
		    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	        (VOID)MEfree(rec_buf);
	        break;
	    }
	    else
		table_empty = FALSE;
	}
	(VOID)MEfree(rec_buf);

	/*
	** Get row width
	*/
	row_size = tab_tcb->tcb_rel.relwid;
	/*
	** Re-adjust the row size if we are dealing with user-defined
	** datatypes which are not long datatypes.
	*/
	for (i = 1; i <= tab_tcb->tcb_rel.relatts; i++)
	{
	    status = adi_dtinfo(&adf_scb, tab_tcb->tcb_atts_ptr[i].type,
			&dt_bits);
	    if ((!(dt_bits & AD_PERIPHERAL)) && (dt_bits & AD_USER_DEFINED))
	    {
		DB_DATA_VALUE	db_value, ev_value;

		db_value.db_datatype = tab_tcb->tcb_atts_ptr[i].type;
		db_value.db_length = tab_tcb->tcb_atts_ptr[i].length;
		db_value.db_prec = tab_tcb->tcb_atts_ptr[i].precision;
		db_value.db_collID = tab_tcb->tcb_atts_ptr[i].collID;
		status = adc_dbtoev(&adf_scb, &db_value, &ev_value);

		/* Take off the old size and append the proper size. */
		row_size -= tab_tcb->tcb_atts_ptr[i].length;
		row_size += (i2)ev_value.db_length;
	    }
	}

	/*
	** need to check for peripheral fields in the table (long varchar,
	** long byte). We will have to couponize them before loading them into
	** the table.
	*/
	for (i = 1; i <= tab_tcb->tcb_rel.relatts; i++)
	{
	    if (tab_tcb->tcb_atts_ptr[i].flag & ATT_PERIPHERAL)
	    {
		/* got a peripheral attribute */
		start_att = i;
		have_peripherals = TRUE;
		start_offset = tab_tcb->tcb_atts_ptr[i].offset;
	        input_bsize = row_size;
		if (tab_rcb->rcb_bqcb_ptr != NULL)
		    tab_rcb->rcb_bqcb_ptr->bqcb_multi_row = TRUE;
		break;
	    }
	}
        /*
        ** Allocate data input buffer 
        */
        readbuffer = MEreqmem(0, input_bsize, TRUE, NULL);
        if (readbuffer == NULL)
        {
	    uleFormat(&jsx->jsx_dberr, E_DM160D_FLOAD_NOMEM, (CL_ERR_DESC *)NULL, 
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	    return (E_DB_ERROR);
        }
		
        nrows = input_bsize / row_size;
        max_bytes_per_read = nrows * row_size;
        /*
        ** Allocate record output buffer array
        */
        load_data = (DM_MDATA *)MEreqmem(0, nrows*sizeof(DM_MDATA), 
	    TRUE, NULL);
        if (load_data == NULL)
        {
	    uleFormat(&jsx->jsx_dberr, E_DM160D_FLOAD_NOMEM, (CL_ERR_DESC *)NULL, 
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	    break;
        }
	/*
	** print out warning about record size
	*/
	if (have_peripherals)
	    SIprintf("\nWARNING: Cannot determine record size of input file:\nfastload expects this file to be in binary format and contain\nrecords of variable length.  Fastload will load data from the file into\nsuccessive table columns.  A bad file format will result in corrupted\ndata being loaded into the table.\n\n");
	else
	    SIprintf("\nWARNING: Cannot determine record size of input file:\nfastload expects this file to be in binary format and contain\nrecords of %d bytes in length.  Fastload will load a record from\neach contiguous %d bytes of the file; a bad file format\nwill result in corrupted data being loaded into the table.\n\n", row_size, row_size);

	/*
	** initiate the load.
	** Pass a large row-estimate to force rebuild/recreate loading
	** when the table is empty initially.  We have no clue what the
	** actual row estimate might be (fstat on the input file could
	** tell us, but there's no CL (SI) function to do that).
	*/
	status = dmxe_writebt(xcb, FALSE, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	      (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM160F_FLOAD_LOAD_ERR);
	    break;
	}
	flags = DM2R_NOSORT;
	if (table_empty)
	    flags |= DM2R_EMPTY;
	status = dm2r_load(tab_rcb, tab_tcb, DM2R_L_BEGIN | DM2R_L_FLOAD, 
	    flags, &rows_read, 
	    NULL, 1000000, NULL, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&jsx->jsx_dberr, E_DM160F_FLOAD_LOAD_ERR, (CL_ERR_DESC *)NULL, 
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	    break;
	}
        /*
        ** Initialize the DM_MDATA structure
	*/
	for (ptr = readbuffer, i=0; i<nrows; i++)
	{
	    load_data[i].data_address = ptr;
	    ptr += row_size; 
	    load_data[i].data_next = &load_data[i+1];
	    load_data[i].data_size = row_size;
	}
	load_data[nrows-1].data_next = NULL;
	SIprintf("Begin load...\n");
        while (!done)	
	{
	    record_offset = start_offset;
	    att_num = start_att;
	    if (have_peripherals)
	    {
		if (record_offset > 0)
		{
		    /* get initial part of record, before peripheral */
		    cl_stat = SIread(fdesc, record_offset, &bytes_read, 
			readbuffer);
		    if (bytes_read < record_offset) /* no records left */
		        break;
		    else if (cl_stat != OK)
		    {
	    		uleFormat(&jsx->jsx_dberr, E_DM1619_FLOAD_BAD_READ, 
			    (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL, (i4)0, 
			    (i4 *)NULL, &local_err, 0);
			status = E_DB_ERROR;
			break;
		    }
		}
		status = handle_peripheral(jsx, readbuffer + record_offset, 
		    fdesc, tab_rcb, tab_tcb, att_num);
		if (status == E_DB_WARN) /* no data left */
		{
		    status = E_DB_OK;
		    CLRDBERR(&jsx->jsx_dberr);
		    /* 
		    ** O.K. to break here since we only read 1 row at a time
		    ** for peripherals 
		    */
		    break;
		}
		if (status != E_DB_OK)
		    break;
		record_offset += sizeof(ADP_PERIPHERAL);
		/*
		** read null byte
		*/
		if (tab_tcb->tcb_atts_ptr[att_num].length > 
		    sizeof(ADP_PERIPHERAL))
		{
		    /* there must be a null byte to read */
		    cl_stat = SIread(fdesc, 1, &bytes_read, readbuffer +
			record_offset);
		    if (cl_stat != OK || bytes_read != 1)
		    {
	    		uleFormat(&jsx->jsx_dberr, E_DM1619_FLOAD_BAD_READ, 
			    (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL, (i4)0, 
			    (i4 *)NULL, &local_err, 0);
			status = E_DB_ERROR;
	    		break;
		    }
		    record_offset += 1;
		}
		/* 
		** check for another peripheral 
		*/
		for (i = att_num + 1; i <= tab_tcb->tcb_rel.relatts; i++)
		{
		    if (tab_tcb->tcb_atts_ptr[i].flag & ATT_PERIPHERAL)
		    {
			i4	diff;
			/*
			** found one, get interveining fields
			*/
			if (record_offset < tab_tcb->tcb_atts_ptr[i].offset)
			{
			    diff = tab_tcb->tcb_atts_ptr[i].offset - 
				record_offset;
			    cl_stat = SIread(fdesc, diff, &bytes_read,
				readbuffer + record_offset);
			    if (cl_stat != OK || bytes_read != diff)
			    {
	    		        uleFormat(&jsx->jsx_dberr, E_DM1619_FLOAD_BAD_READ, 
				    (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL, 
				    (i4)0, (i4 *)NULL, &local_err, 0);
				status = E_DB_ERROR;
				break;
			    }
			}
			att_num = i;
			record_offset = tab_tcb->tcb_atts_ptr[i].offset;
			/*
			** couponise
			*/
			status = handle_peripheral(jsx, readbuffer + record_offset,
			    fdesc, tab_rcb, tab_tcb, att_num);
			if (status != E_DB_OK)
			    break;
		        record_offset += sizeof(ADP_PERIPHERAL);
			/*
			** read null byte
			*/
			if (tab_tcb->tcb_atts_ptr[att_num].length > 
		    	    sizeof(ADP_PERIPHERAL))
			{
		    	    /* there must be a null byte to read */
		    	    cl_stat = SIread(fdesc, 1, &bytes_read, readbuffer +
				record_offset);
		    	    if (cl_stat != OK || bytes_read != 1)
		    	    {
	    		        uleFormat(&jsx->jsx_dberr, E_DM1619_FLOAD_BAD_READ, 
				    (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL, 
				    (i4)0, (i4 *)NULL, &local_err, 0);
				status = E_DB_ERROR;
	    			break;
		    	    }
		    	    record_offset += 1;
			}
		    }
		}
		if (status != E_DB_OK)
		    break;
		/*
		** get the last of the record (if any)
		*/
		if (att_num < tab_tcb->tcb_rel.relatts)
		{
		    cl_stat = SIread(fdesc, tab_tcb->tcb_rel.relwid - 
			record_offset, &bytes_read, readbuffer + record_offset);
		    if (cl_stat != OK || 
			bytes_read != tab_tcb->tcb_rel.relwid - record_offset)
		    {
			uleFormat(&jsx->jsx_dberr, E_DM1619_FLOAD_BAD_READ, 
			    (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL, 
			    (i4)0, (i4 *)NULL, &local_err, 0);
			status = E_DB_ERROR;
			break;
		    }
		}
		rows_read = 1;
		rows_done += 1;
	    }
	    else /* standard read, no peripherals */
	    {
	        cl_stat = SIread(fdesc, max_bytes_per_read, &bytes_read,
		    readbuffer);
                rows_read = bytes_read / row_size;
                rows_done += rows_read;
	    }
            if (rows_read > 0)
            {
	        if (rows_read < nrows)
                {
	            load_data[rows_read-1].data_next = NULL;
		    done = 1;
                }
	        status = dm2r_load(tab_rcb, tab_tcb, DM2R_L_ADD | DM2R_L_FLOAD, 
		    (DM2R_NOSORT | DM2R_EMPTY), &rows_read, 
		    load_data, nrows, NULL, &jsx->jsx_dberr);
	        if (status != E_DB_OK)
	        {
	    	    uleFormat(&jsx->jsx_dberr, E_DM160F_FLOAD_LOAD_ERR, 
			(CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	      	        (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	    	    break;
	        }
            }
	    else
		done = 1;
	}
	if (status != E_DB_OK)
	    break;
	/*
	** finish the load.
	** will do the query-end for blobs.
	*/
	status = dm2r_load(tab_rcb, tab_tcb, DM2R_L_END | DM2R_L_FLOAD, 0, 
	    &rows_read, NULL, 0, NULL, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&jsx->jsx_dberr, E_DM160F_FLOAD_LOAD_ERR, 
		(CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	    break;
	}
	SIprintf("Load finished\n");

	SIflush(stdout);
	/*
	** close the table
	*/ 
	/* Dequeue RCB from XCB first.  NOTE, probably ought to be using
	** DML level table open/close services too.
	*/
	tab_rcb->rcb_xq_next->rcb_q_prev = tab_rcb->rcb_xq_prev;
	tab_rcb->rcb_xq_prev->rcb_q_next = tab_rcb->rcb_xq_next;
	status = dm2t_close(tab_rcb, DM2T_NOPURGE, &jsx->jsx_dberr); 
	if (status != E_DB_OK) 
	{ 
	    uleFormat(&jsx->jsx_dberr, E_DM1605_FLOAD_CLOSE_TABLE, 
		(CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	    break;
	}
	if (NULL!=master_rcb)
	{
	    /* That was the partition being closed, now do the master */
	    tab_rcb = NULL;
	    master_rcb->rcb_xq_next->rcb_q_prev = master_rcb->rcb_xq_prev;
	    master_rcb->rcb_xq_prev->rcb_q_next = master_rcb->rcb_xq_next;
	    status = dm2t_close(master_rcb, DM2T_NOPURGE, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
	    {
		uleFormat(&jsx->jsx_dberr, E_DM1605_FLOAD_CLOSE_TABLE, 
		    (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
		break;
	    }
	}
	table_open = FALSE;

	/* If blobs, make sure all blob holding tanks are gone.
	** (normally happens at blob move time).
	*/
	if (have_peripherals)
	    (void) dmpe_query_end(FALSE, TRUE, &local_dberr);

	/*
	** end the transaction
	** most dmx cb stuff still there from begin.
	*/
	dmx.dmx_option = 0;
	dmx.dmx_flags_mask = 0;
	status = dmx_commit(&dmx);
	if (status != E_DB_OK)
	{
	    jsx->jsx_dberr = dmx.error;
	    uleFormat(&jsx->jsx_dberr, E_DM1606_FLOAD_COMMIT_XACT, 
		(CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	    break;
	}
	open_xact = FALSE;
	/* Avoid changing the message if not blobs... */
	str = "loaded:";
	if (rows_done > rows_read)
	    str = "read  :";
	if (have_peripherals)
	    SIfprintf(stderr, "Row size   : %d\nRows %s %d\nTotal bytes (excluding variable length data): %lld\n",
                 (i4)row_size, str, rows_done, (i8)row_size * rows_done);
	else
	    SIfprintf(stderr, "Row size   : %d\nRows %s %d\nTotal bytes: %lld\n",
                 (i4)row_size, str, rows_done, (i8)row_size * rows_done);
	if (rows_done > rows_read)
	{
	    SIfprintf(stderr, "Rows discarded because of duplicate keys: %d\n",
			rows_done - rows_read);
	}
	open_xact = FALSE;
         
	/*
	** close the database
	*/
	status = dm2d_close_db(dcb, dmf_svcb->svcb_lock_list,
	    (DM2D_NLK | DM2D_NLG), &jsx->jsx_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&jsx->jsx_dberr, E_DM1607_FLOAD_CLOSE_DB, 
		(CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	    break;
	}
	dbopen = FALSE;
	/*
	** close the load file
	*/
	cl_stat = SIclose(fdesc);
	if (cl_stat != OK)
	{
	    uleFormat(&jsx->jsx_dberr, E_DM1608_FLOAD_CLOSE_FILE, 
		(CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	    break;
	}
	/*
	** memory cleanup
	*/
	if (dml_loc_masks)
	    dm0m_deallocate((DM_OBJECT **)&dml_loc_masks);

	if (jsx->jsx_status & JSX_TRACE)
	    TRset_file(TR_F_CLOSE, 0, 0, &sys_err);

	return (E_DB_OK);
    }
    /*
    ** clean up
    */
    if (table_open)
    {
	if (tab_rcb != NULL)
	{
	    tab_rcb->rcb_xq_next->rcb_q_prev = tab_rcb->rcb_xq_prev;
	    tab_rcb->rcb_xq_prev->rcb_q_next = tab_rcb->rcb_xq_next;
	    local_stat = dm2t_close(tab_rcb, DM2T_NOPURGE, &local_dberr); 
	    if (local_stat != E_DB_OK) 
	    { 
		uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
		uleFormat(&jsx->jsx_dberr, E_DM1605_FLOAD_CLOSE_TABLE, 
		    (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	    }
	}
	if(NULL!=master_rcb)
	{
	    master_rcb->rcb_xq_next->rcb_q_prev = master_rcb->rcb_xq_prev;
	    master_rcb->rcb_xq_prev->rcb_q_next = master_rcb->rcb_xq_next;
	    local_stat = dm2t_close(master_rcb, DM2T_NOPURGE, &local_dberr);
	    if (local_stat != E_DB_OK)
	    {
		uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
		uleFormat(&jsx->jsx_dberr, E_DM1605_FLOAD_CLOSE_TABLE, 
		    (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	    }
	}
    }
    if (open_xact)
    {
	dmx.dmx_option = 0;
	local_stat = dmx_abort(&dmx);
	if (local_stat != E_DB_OK)
	{
	    jsx->jsx_dberr = dmx.error;
	    uleFormat(&jsx->jsx_dberr, E_DM1609_FLOAD_ABORT_XACT, 
		    (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	}
    }
    if (dbopen)
    {
	local_stat = dm2d_close_db(dcb, dmf_svcb->svcb_lock_list, 
	    (DM2D_NLK | DM2D_NLG), &local_dberr);
	if (local_stat != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	    uleFormat(&jsx->jsx_dberr, E_DM1607_FLOAD_CLOSE_DB, 
		    (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	}
    }
    if (file_open)
    {
	local_clstat = SIclose(fdesc);
	if (local_clstat != OK)
        {
	    uleFormat(&jsx->jsx_dberr, E_DM1608_FLOAD_CLOSE_FILE, 
		    (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
        }
    }
    if (dml_loc_masks)
	dm0m_deallocate((DM_OBJECT **)&dml_loc_masks);
    return (E_DB_ERROR);
}
/*
** Name: dml_get_tabid - get table ID given table name
**
** Description:
**	Use dm2 low level dmf routines to get the table ID of a table
**	given it's name (using iirelation, iirel_idx). Table ID may then be
**	used to open the table
**
** Inputs:
**	dcb		Database control block
**	table_name	Name of the table.
**	table_owner	Owner of the table.
**
** Outputs:
**	table_id	ID of the table.
**	err_code	error code.
**
** History:
**	25-apr-94 (stephenb)
**	    Initial creation.
**      06-mar-96 (stial01)
**          Variable Page Size project:
**          Make sure buffers are allocated for the fastload page size.
**
*/
DB_STATUS
dml_get_tabid (	DMP_DCB		*dcb,
		DB_TAB_NAME	*table_name,
		DB_OWN_NAME	*table_owner,
		DB_TAB_ID	*table_id,
		DB_ERROR	*dberr)
{
    DB_TAB_ID		catalog_id;
    DB_TRAN_ID		tran_id;
    DB_STATUS		status;
    DB_TAB_TIMESTAMP	timestamp;
    DMP_RCB		*r_idx = NULL;
    DMP_RCB             *rel = NULL;
    DM2R_KEY_DESC	key_desc[2];
    DM_TID              tid;
    DM_TID              rel_tid;
    DMP_RELATION        relation;
    DMP_RINDEX          rindexrecord;
    i4		local_err;
    DB_STATUS		local_stat;
    char		pad_tab_name[DB_TAB_MAXNAME];
    char		pad_own_name[DB_OWN_MAXNAME];
    DB_ERROR		local_dberr;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    for(;;)
    {
	/*
	** open iirel_idx
	*/
	catalog_id.db_tab_base = DM_B_RIDX_TAB_ID;
	catalog_id.db_tab_index = DM_I_RIDX_TAB_ID;
	tran_id.db_high_tran = 0;
	tran_id.db_low_tran = 0;

	status = dm2t_open(dcb, &catalog_id, DM2T_IS, DM2T_UDIRECT,
	    DM2T_A_READ, (i4)0, (i4)20, 0, 0, 
	    dmf_svcb->svcb_lock_list, (i4)0,(i4)0, 0, &tran_id,
	    &timestamp, &r_idx, (DML_SCB *)0, dberr);
	if (status != E_DB_OK)
	    break;
	/*
	** find iirelation entry using iirel_idx
	*/
	MEmove(STlength(table_name->db_tab_name), table_name->db_tab_name,
	    ' ', DB_TAB_MAXNAME, pad_tab_name);
	MEmove(STlength(table_owner->db_own_name), table_owner->db_own_name,
	    ' ', DB_OWN_MAXNAME, pad_own_name);
	key_desc[0].attr_operator = DM2R_EQ;
	key_desc[0].attr_number = DM_1_RELIDX_KEY;
	key_desc[0].attr_value = pad_tab_name;
	key_desc[1].attr_operator = DM2R_EQ;
	key_desc[1].attr_number = DM_2_RELIDX_KEY;
	key_desc[1].attr_value = pad_own_name;

	status = dm2r_position(r_idx, DM2R_QUAL, key_desc, (i4)2, 
	    (DM_TID *)0, dberr);
	if (status != E_DB_OK)
	    break;

	status = dm2r_get(r_idx, &tid, DM2R_GETNEXT, (char *)&rindexrecord,
	    dberr);
	if (status == E_DB_OK)
	    rel_tid.tid_i4 = rindexrecord.tidp;
	else
	    break;
	/*
	** close iirel_idx
	*/
	status = dm2t_close(r_idx, DM2T_NOPURGE, dberr);
	if (status != E_DB_OK)
	    break;

	/*
	** now use iirelation to get the table id
	*/
	catalog_id.db_tab_base = DM_B_RELATION_TAB_ID;
	catalog_id.db_tab_index = DM_I_RELATION_TAB_ID;
	tran_id.db_high_tran = 0;
	tran_id.db_low_tran = 0;

	status = dm2t_open(dcb, &catalog_id, DM2T_IS, DM2T_UDIRECT, 
	    DM2T_A_READ, (i4)0, (i4)20, 0, 0, 
	    dmf_svcb->svcb_lock_list, (i4)0, (i4)0, 0, &tran_id, 
	    &timestamp, &rel, (DML_SCB *)0, dberr);
	if (status != E_DB_OK)
	    break;

	status = dm2r_get(rel, &rel_tid, DM2R_BYTID, (char *)&relation,
	    dberr);
	if (status != E_DB_OK)
	    break;

	status = dm2t_close(rel, DM2T_NOPURGE, dberr);
	if (status != E_DB_OK)
	    break;

	/*
	** fill in table ID
	*/
	table_id->db_tab_base = relation.reltid.db_tab_base;
	table_id->db_tab_index = relation.reltid.db_tab_index;

	/*
	** Make sure a buffer cache was allocated for the fastload pagesize
	*/
	if (!(dm0p_has_buffers(relation.relpgsize)))
	{
	    status = dm0p_alloc_pgarray(relation.relpgsize, dberr);
	    if (status)
		return (E_DB_ERROR);
	}

	return (E_DB_OK);
    }
    /*
    ** handle errors
    */
    if (r_idx)
	local_stat = dm2t_close(r_idx, DM2T_NOPURGE, &local_dberr);

    if (rel)
	local_stat = dm2t_close(rel, DM2T_NOPURGE, &local_dberr);

    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
	(i4)0, (i4 *)NULL, err_code, 0);
    SETDBERR(dberr, 0, E_DM160C_FLOAD_NO_TABLE);

    return (E_DB_ERROR);
}

/*
** Name: handle_peripheral - handle peripheral datatypes
**
** Description:
**	This routine handles datatypes that contain peripheral records,
**	given the record buffer and current file descriptor, it will
**	read in the data and call ADF to produce a coupon. The coupon will
**	then be added to the load record.
**
** Inputs:
**	readbuffer	- current record buffer
**	file		- file descriptor of input file
**	att_num		- attribute number of peripheral datatype 
**
** Outputs:
**	readbuffer	- with coupon added
**	err_code	- possible error
**
** Returns:
**	E_DB_OK
**	E_DB_WARN	- No data to read in buffer
**	E_DB_ERROR
**
** History:
**	7-jan-97 (stephenb)
**	    Initial creation.
**	1-5 feb-97 (stephenb)
**	    Complete coding of support for long datatypes, also tidy up
**	    Initial creation.
**	13-aug-1998 (somsa01)
**	    Cleaned up so that it would work properly.
**	11-May-2004 (schka24)
**	    Name change for temp flag, make sure pop-info is clear.
**	17-Apr-2008 (kibro01) b120276
**	    Initialise ADF_CB structure
**	19-Apr-2010 (kschendel) SIR 123485
**	    Use blob-wksp thing to send blob directly home.
*/
static DB_STATUS
handle_peripheral(
		DMF_JSX		*jsx,
		char		*readbuffer, 
		FILE		*fdesc,
		DMP_RCB		*tab_rcb,
		DMP_TCB		*tab_tcb,
		i4		att_num)
{
    i2			seg_length, nextseg_length;
    i4			seg_bytes;
    char		*pdata;
    i2			more_data = 1;
    i4			cont = 0;
    DB_DATA_VALUE	load_dv, coupon_dv;
    DB_BLOB_WKSP	wksp;
    ADP_LO_WKSP		*workspace;
    ADP_PERIPHERAL	coupon;
    DB_DT_ID		datatype;
    DB_STATUS		status;
    STATUS		cl_stat;
    i4			bytes_read;
    ADF_CB		adf_scb;
    ADP_PERIPHERAL	*adpp;
    i4			extra;
    i4			ind4;
    i4			max_seg_bytes = MAX_SEG_SIZE;
    i4			local_err;

    CLRDBERR(&jsx->jsx_dberr);

    MEfill(sizeof(ADF_CB),0,(PTR)&adf_scb);

    for (;;) /* to break out of */
    {
	/* 
	** Allocate workspace for ADF 
	*/
	if (abs(tab_tcb->tcb_atts_ptr[att_num].type) == DB_LNVCHR_TYPE)
	    max_seg_bytes *= sizeof(UCS2);

	workspace = (ADP_LO_WKSP *)MEreqmem(0, sizeof(ADP_LO_WKSP) +
					    DB_MAXTUP, TRUE, &cl_stat);
	if (workspace == NULL || cl_stat != OK)
	{
	    uleFormat( &jsx->jsx_dberr, E_DM160D_FLOAD_NOMEM, (CL_ERR_DESC *)NULL, 
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	    break;
	}
	workspace->adw_fip.fip_pop_cb.pop_temporary = ADP_POP_TEMPORARY;
	/* Even though we've said "temporary", arrange for dmpe to send
	** the blob directly to the target etab.  dmpe will know what
	** to do.
	** Since it's a binary load, don't (shouldn't) need to coerce.
	*/
	wksp.access_id = tab_rcb;
	wksp.base_attid = att_num;
	wksp.flags = BLOBWKSP_ACCESSID | BLOBWKSP_ATTID;
	MEfill(sizeof(coupon), 0, (PTR) &coupon);

	/* 
	** Get memory for peripheral data 
	*/
	pdata = MEreqmem(0, ADP_HDR_SIZE + sizeof(i4) + sizeof(i2) +
			 max_seg_bytes, TRUE, &cl_stat);
	if (pdata == NULL || cl_stat != OK)
	{
	    uleFormat( &jsx->jsx_dberr, E_DM160D_FLOAD_NOMEM, (CL_ERR_DESC *)NULL, 
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	    break;
	}

	/*
	** Initiate ADP header
	*/
	adpp = (ADP_PERIPHERAL *)pdata;
	adpp->per_tag = ADP_P_GCA_L_UNK;
	adpp->per_length0 = 0;
	adpp->per_length1 = 0;
	adpp->per_value.val_nat = 1;
	extra = ADP_HDR_SIZE;

	/*
	** Read the first length to see if there are any segments
	** to read for this blob.
	*/
	cl_stat = SIread(fdesc, 2, &bytes_read, (char *)&seg_length);
	if (cl_stat == ENDFILE) /* no records left */
	{
	    cl_stat = OK;
	    status = E_DB_WARN;
	    break;
	}

	if (cl_stat != OK || bytes_read != 2)
	{
	    /* We've read an invalid length! */
	    uleFormat(&jsx->jsx_dberr, E_DM1619_FLOAD_BAD_READ, (CL_ERR_DESC *)NULL, 
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** Check for invalid segment length which could cause fastload
	** to overwrite memory and loop
	** max-seg-size is max i2, no need for > test.
	*/
	if (seg_length < 0)
	{
	    /* We've read an invalid length! */
	    uleFormat(&jsx->jsx_dberr, E_DM1619_FLOAD_BAD_READ, (CL_ERR_DESC *)NULL, 
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	    status = E_DB_ERROR;
	    break;
	}

	nextseg_length = seg_length;

        while (more_data)
        {
	    /*
	    ** The segment indicator flag is nonzero, and 0 after the
	    ** last segment.
	    */
	    ind4 = seg_length;
	    MEcopy((char *)&ind4, sizeof(i4), pdata + extra);

	    seg_bytes = seg_length;
	    if (abs(tab_tcb->tcb_atts_ptr[att_num].type) == DB_LNVCHR_TYPE)
		seg_bytes *= sizeof(UCS2);

	    if (nextseg_length)
	    {
		/* 
		** ADF wants length to be part of the data.
		*/
		MEcopy((char *)&seg_length, sizeof(seg_length),
			pdata + extra + sizeof(i4));

		/* 
		** Read the blob segment data.
		*/
		cl_stat = SIread(fdesc, seg_bytes, &bytes_read,
				pdata + extra + sizeof(i4) + sizeof(i2));
		if (cl_stat != OK || bytes_read != seg_bytes)
		{
	    	    uleFormat( &jsx->jsx_dberr, E_DM1619_FLOAD_BAD_READ,
				(CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
				(i4)0, (i4 *)NULL, &local_err, 0);
		    status = E_DB_ERROR;
		    break;
		}

		/* 
		** Get the next segment's length.
		*/
		cl_stat = SIread(fdesc, 2, &bytes_read,
				(char *)&nextseg_length);
		if (cl_stat == ENDFILE) /* no records left */
		{
		    cl_stat = OK;
		    status = E_DB_WARN;
		    break;
		}
		if (cl_stat != OK || bytes_read != 2)
		{
	    	    uleFormat( &jsx->jsx_dberr, E_DM1619_FLOAD_BAD_READ,
				(CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
				(i4)0, (i4 *)NULL, &local_err, 0);
		    status = E_DB_ERROR;
		    break;
		}
		/*
		** Check for invalid segment length which could cause fastload
		** to overwrite memory and loop
		** max-seg-size is max i2, no need for > test.
		*/
		if (nextseg_length < 0)
		{
		    /* We've read an invalid length! */
	    	    uleFormat( &jsx->jsx_dberr, E_DM1619_FLOAD_BAD_READ,
				(CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
				(i4)0, (i4 *)NULL, &local_err, 0);
		    status = E_DB_ERROR;
		    break;
		}
	    }

	    /* 
	    ** setup adf blocks 
	    */
	    if (abs(tab_tcb->tcb_atts_ptr[att_num].type) == DB_LNVCHR_TYPE)
		datatype = DB_LNVCHR_TYPE;
	    else if (abs(tab_tcb->tcb_atts_ptr[att_num].type) == DB_VCH_TYPE || 
		abs(tab_tcb->tcb_atts_ptr[att_num].type) == DB_LVCH_TYPE)
	        datatype = DB_LVCH_TYPE;
	    else
		datatype = DB_LBYTE_TYPE;

	    load_dv.db_data = pdata;
	    load_dv.db_datatype = datatype;
	    load_dv.db_prec = 0;
	    load_dv.db_collID = -1;

	    if (seg_length)
		load_dv.db_length = extra + sizeof(i4) + sizeof(i2) +
				    seg_bytes;
	    else
		load_dv.db_length = sizeof(i4);

	    coupon_dv.db_data = (char *)&coupon;
	    coupon_dv.db_datatype = datatype;
	    coupon_dv.db_length = sizeof(ADP_PERIPHERAL);
	    coupon_dv.db_prec = 0;
	    coupon_dv.db_collID = -1;

	    workspace->adw_fip.fip_work_dv.db_data = (PTR)(workspace + 1);
	    workspace->adw_fip.fip_work_dv.db_length = DB_MAXTUP;
	    workspace->adw_fip.fip_work_dv.db_prec = 0;
	    workspace->adw_fip.fip_work_dv.db_collID = -1;
	    workspace->adw_fip.fip_pop_cb.pop_info = (PTR) &wksp;

	    /* 
	    ** couponify the data 
	    */
	    status = adu_couponify(&adf_scb, cont, &load_dv, 
		workspace, &coupon_dv);
	    if (status == E_DB_INFO)
		more_data = 1;
	    else if (status != E_DB_OK)
	    {
	    	uleFormat(&jsx->jsx_dberr, E_DM161A_FLOAD_COUPONIFY_ERROR, 
		    (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL, (i4)0, 
		    (i4 *)NULL, &local_err, 0);
		status = E_DB_ERROR;
		break;
	    }
	    else
		more_data = 0;

	    cont = 1;
	    extra = 0;
	    seg_length = nextseg_length;
	}
	if (cl_stat != OK || status != E_DB_OK)
	    break;
	/* 
	** add coupon to the record 
	*/
	MEcopy((char *)&coupon, sizeof(coupon), readbuffer);

	break;
    }

    /*
    ** cleanup
    */
    if (workspace)
	MEfree((PTR)workspace);

    if (pdata)
	MEfree(pdata);

    if (cl_stat != OK || status == E_DB_ERROR)
	return(E_DB_ERROR);
    else if (status == E_DB_WARN)
	return(E_DB_WARN);
    else
	return(E_DB_OK);
}
