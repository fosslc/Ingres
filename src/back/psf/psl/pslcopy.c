/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <cv.h>
#include    <me.h>
#include    <qu.h>
#include    <st.h> 
#include    <cm.h>
#include    <iicommon.h>
#include    <cv.h>
#include    <dbms.h>
#include    <cui.h>
#include    <gca.h>
#include    <copy.h>
#include    <adf.h>
#include    <ade.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmscb.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <dmucb.h>
#include    <qefmain.h>
#include    <qefqeu.h>
#include    <qefcopy.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psftrmwh.h>
#include    <pslcopyd.h>
#include    <usererror.h>

/*
** Name: PSLCOPY.C:	this file contains functions used in semantic actions
**			for the SQL and QUEL COPY statement
**
** Function Name Format:  (this applies to externally visible functions only)
** 
**	PSL_CP<number>_<production name>
**	
**	where <number> is a unique (within this file) number between 1 and 99
**	
** Description:
**	this file contains functions used by both SQL and QUEL grammar in
**	parsing of the COPY [TABLE] statement
**
**	Routines:
**	    psl_cp1_copy		- actions for copy production
**	    psl_cp2_copstmnt 		- actions for the copstmnt production
**	    psl_cp3_copytable		- actions for the copytable production
**	    psl_cp4_coparam		- actions for the empty coparam rule
** 	    psl_cp5_cospecs		- actions for the cospecs production
**	    psl_cp6_entname		- actions for the entname production
**	    psl_cp7_fmtspec		- actions for the fmtspec production
**	    psl_cp8_coent	    	- actions for coent production
**	    psl_cp9_nm_eq_nm		- actions for copyoption production
**	    psl_cp10_nm_eq_str		- actions for copyoption production
**	    psl_cp11_nm_eq_no		- actions for copyoption production
**	    psl_cp12_nm_eq_qdata	- actions for copyoption production
**	    psl_cp13_nm_single		- actions for copyoption production
**
** History:
**	7-may-1992 (barbara)
**	    Created as part of the Sybil project of merging Star code
**	    into the 6.5 rplus line.
**	31-aug-1992 (barbara)
**	    Minor changes to fix bugs in the distributed code.
**      22-Feb-1993 (fred)
**  	    Added support for large objects.
**	04-mar-93 (rblumer)
**	    change parameter to psf_adf_error to be psq_error, not psq_cb.
**	30-mar-1993 (rmuth)
**	    Add support for the "WITH ROW_ESTIMATE = n" clause.
**	29-jun-93 (andre)
**	    #included CV.H
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-july-1993 (rmuth)
**	    File extend changes.
**	       - Add <cv.h> and <dmucb.h>
**	       - Remove the WITH [NO]EMPTYTABLE option.
**	       - Add support for the following WITH OPTIONS, (FILLFACTOR, 
**	         LEAFFILL, NONLEAFFILL, ALLOCATION, EXTEND, MINPAGES and 
**	         MAXPAGES). This involved
**		   - Adding a psl_validate_options check to psl_cp1_copy
**	           - Enhancing psl_cp11_nm_eq_no to process new options.
**      17-aug-1993 (stevet)
**          My earlier change to call adc_dbtoev() to figure return
**          datatype/length/precision does not work for UDT COPY.
**          UDT COPY can only be exported in internal binary format/length
**          using CHAR type.  
**      03-Aug-1993 (fpang)
**          In psl_cp5_cospecs(), initialize 'unorm_len' before calling 
**          cui_idunorm().
**          Fixes B53943, 'Spurious E_SC206's during copy.'
**	15-sep-93 (swm)
**	    Added cs.h include above other header files which need its
**	    definition of CS_SID.
**	26-nov-93 (swm)
**	    Bug #58635
**	    Added PTR cast for assignment to psy_owner whose type has
**	    changed.
**	16-feb-1995 (shero03)
**	    Bug #65565
**	    Cast long UDTs to long varchar.
**	05-apr-1996 (loera01)
**	    In psl_cp1_copy(), passed along GCA_COMP_MASK in tuple descriptor, 
**          if the front end has requested variable-length datatype
**          compression. 
**	14-may-1997 (nanpr01)
**	    This code is not doing anything so get rid of it. We just allocated
**	    a piece of memory and then checking the GCA_COMP_MASK of it
**	    which doesnot make sense.
**	27-aug-1998 (somsa01)
**	    In psl_cp1_copy(), the last change introduced the testing of
**	    dt_bits in an area where it is a stale variable. Re-initialize
**	    it per column by relocating the call to adi_dtinfo().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SESBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp().
**	17-Jan-2001 (jenjo02)
**	    Added *PSS_SESBLK parm to psq_prt_tabname().
**	21-Dec-2001 (gordy)
**	    GCA_COL_ATT no longer defined with DB_DATA_VALUE.
**	21-oct-2002 (horda03) Bug 77895
**          Create a list of default values for any columns not specified in
**          the COPY FROM command.
**	31-Dec-2002 (hanch04)
**	    Fix bad cross, Added *PSS_SESBLK psf_malloc.
**      02-jan-2004 (stial01)
**          Changes to expand number of WITH CLAUSE options
**      18-jun-2004 (stial01)
**          Init mc_dv, mc_dtbits (b112521)
**	26-Jul-2005 (hanje04)
**	    Fixup bad X-integration of change 473756 (477935).
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**      24-Jun-2010 (horda03) B123987
**          For an Ingresdate tuple copied into a string for a
**          COPY INTO change the string's length to be
**          AD_11_MAX_STRING_LEN to cater for maximum interval
**          length. 
**	    
*/

/* external declarations */

/*
** Name: psl_cp1_copy	- Semantic actions for copy: production
**
** Description:
**	This routine performs the semantic action for copy: production.
**	The SQL and QUEL semantic actions are the same.
**
**	copy:	    copstmnt copytable LPAREN coparam RPAREN keywd
**		    copyfile copywith
**
**	A whole bunch of misc stuff is done here, things that have to
**	wait until the entire copy statement is parsed:
**
**	- various validation tests
**	- construction of the tuple description (ie sqlda)
**	- generation of missing-value descriptors for defaulted columns
**	- generation of a DMS_CB if there are any sequence defaults
**	- generation of CPATTINFO structures for columns that need
**	  special processing in QEF at runtime, eg import/export conversion
**	- fetching of the partition definition for partitioned tables
**	- lots of coercion setup work especially for blobs
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	    pss_adfcb	    ADF session CB
**	    pss_resrng	    range variable info
**	psq_cb		    Parser query control block
**	xlated_qry
**	    pss_q_list	    list of packets of query text for LDB (distrib only)
**	scanbuf_ptr	    points to beginning of format string.
**
** Outputs:
**	err_blk		    will be filled in if an error occurs
**	sess_cb		    ptr to a PSF session CB
**	    pss_object	    copy information for QEF
**	psq_cb
**	    .qmode	    if distributed, qmode will be changed
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	07-may-91 (barbara)
**	    Largely copied from the 6.5 SQL grammar, adding distributed-only
**	    actions.
**      13-jan-1993 (stevet)
**          Instead of always export CHAR type for datatypes that is
**          AD_NOEXPORT and AD_CONDEXPORT, call adc_dbtoev to figure out
**          the export type/length/prec.
**      22-Feb-1993 (fred)
**  	    Added support for large objects.  If peripheral objects
**          are found in the copy statement, then mark the tuple
**          descriptor as containing large objects (GCA_LO_MASK).
**  	    Similarly, mark such object's length as ADE_LEN_UNKNOWN
**          (-1 at present).  Also, do not include the length of these
**          objects in the tuple size calculation for the tuple
**          descriptor.  It will be marked as unknown and count as
**          zero (0) in the tuple descriptor's version of the length.
**	23-jun-93 (robf)
**	    For NON/CONDEXPORT types increase the gca_tsize by the external
**	    size, not the internal, 
**	    Set external size of tuple in qeu_ext_length.
**       9-Jul-1993 (fred)
**          Fixed bug in large object.  If a copy statement includes a
**          column with a `with null' clause, this routine will
**          convert that value from its character type to the
**          appropriate peripheral type.  Since peripheral types may
**          have to be materialized on disk using DMF, an open
**          writable transaction is required.  Normally, this is the
**          case.  However, if said copy statement is the first
**          statement in a transaction, the transaction (non-readonly)
**          has not been started yet.  So will will start it here if
**          and only if necessary.
**	26-jul-1993 (rmuth)
**	    Add support to deal with extra WITH options. Process the
**	    with list to make sure options are valid, call 
**	    psl_validate options to perform this task.
**	10-aug-93 (andre)
**	    fixed causes of compiler warnings
**      17-aug-1993 (stevet)
**          For AD_NOEXPORT and AD_CONDEXPORT datatypes, the call to 
**          adc_dbtoev() should only check for whether we can COPY
**          this datatype out.  The return type/lenght/precision 
**          from adc_dbtoev() should not be used.  Also added back
**          original code which force UDT to COPY in internal format/length
**          using CHAR type.
**	3-oct-93 (robf)
**          Security labels/ids don't always export as CHAR when copying,
**	    so make sure we use the appropriate values are used for these.
**      13-oct-1993 (markm)
**          Prevent user from COPY()ing into a secondary index, (bug 51426),
**          also added psf_trmwhite() to error message 5818 (user specified 
**          a view) for consistency and neatness.
**	16-feb-1995 (shero03)
**	    Bug #65565
**	    Cast long UDTs to long varchar.
**	05-apr-1996 (loera01)
**	    Passed along GCA_COMP_MASK in tuple descriptor, if the front end
**	    has requested variable-length datatype compression. 
**	03-apr-1997 (nanpr01)
**	    Fix intermittent hangs becuase commit was not performed.
**	14-may-1997 (nanpr01)
**	    This code is not doing anything so get rid of it. We just allocated
**	    a piece of memory and then checking the GCA_COMP_MASK of it
**	    which doesnot make sense. Just initialize the allocated area.
**	24-aug-98 (stephenb)
**	    zero out allocated memory for peripheral also, the data area
**	    contains the coupon which should be initialized.
**	27-aug-1998 (somsa01)
**	    The last change introduced the testing of dt_bits in an area
**	    where it is a stale variable. Re-initialize it per column
**	    by relocating the call to adi_dtinfo().
**	22-Feb-1999 (shero03)
**	    Support 4 operands in adi_0calclen
**	21-Dec-2001 (gordy)
**	    GCA_COL_ATT no longer defined with DB_DATA_VALUE: make temp
**	    copy of attribute using DB_COPY macros, update and copy back.
**      21-oct-2002 (horda03) Bug 77895
**          For any column not specified in a COPY FROM command, obtain the
**          default value, and place in a link list of QEU_MISISNG_COL
**          elements.
**	10-mar-2003 (toumi01)
**		fill missing_dbv values with "=" rather than MEcopy to
**		avoid bad data assignments on a64_lnx
**      16-Nov-2004 (inifa01) Bug 113401/INGSRV3042
**	    Ascii copy into a table with a long varchar column, when that 
**	    column is not one of those being loaded causes the server to 
**	    crash with E_QE0002 and E_DM9049.
**	    modified fix to b77895 in psl_cp1_copy() to non-BLOB case as
**	    explicit default values are not allowed for BLOBS.
**	25-Apr-2004 (schka24)
**	    Remove qeu-cb from qeu-copy data block.
**	    Make copy of partition def for partitioned COPY FROM.
**    07-Jul-2005 (inifa01) INGSRV3202 Bug 114072
**        Copy in where a column is unspecified and the unspecified
**        column is a constant, in this case 'current_user', causes
**        the copy to fail with a SEGV.
**        call psl_proc_func() to evaluate the the constant operator
**        so that a PST_CONST node is created with the datavalue field
**        filled in.
**	26-Jul-2005 (hanje04)
**	    Fixup bad X-integration of change 473756 (477935).
**	23-Nov-2005 (kschendel)
**	    Add new param to validate options (not needed here).
**	14-Jun-2006 (kschendel)
**	    Implement sequence defaults;  build DMS_CB and friends.
**	6-Jul-2006 (kschendel)
**	    Fill in unique dbid for RDF, just in case.
**	11-Feb-2009 (kibro01) b121642
**	    Space-pad the parsed attribute name - otherwise we end up using
**	    it as a prefix comparison in STbcompare, and that lets us match
**	    abc with abcdef, which means a missing abcdef field doesn't get
**	    picked up.
**      26-Nov-2009 (hanal04) Bug 122964
**          Add missing comment termination. qeu_tmptuple was not being
**          allocated leading to SEGVs in QEU copy routines.
**      26-Nov-2009 (hanal04) Bug 122938
**          Downgrade/convert attributes that can not be handled by older 
**          protocol levels. i8s must become i4s and dec(39,33) must
**          become dec(31,25). 
**       7-Dec-2009 (hanal04) Bug 123019, 122938
**          Adjust previous change for Bug 122938. Scale only needs to be
**          adjusted if it is greater than the new precision. This prevents
**          negative scale values seen in replicator testing.
**	December 2009 (stephenb)
**	    Batch procesing, new parameter for psl_proc_func
**	19-Mar-2010 (kschendel) SIR 123448
**	    Tmptuple should be allocated by qef, not psf.
**	2-Apr-2010 (toumi01) SIR 122403
**	    For the encryption project teach an old dog new tricks:
**	    a row can have different values for physical length e.g.
**		qe_copy->qeu_tup_physical
**		sess_cb->pss_resrng->pss_tabdesc->tbl_width
**	    vs. logical length e.g.
**		qe_copy->qeu_tup_length
**		sess_cb->pss_resrng->pss_tabdesc->tbl_data_width
**	    This makes things clearer in displaying table info in HELP
**	    TABLE etc. but is also essential for things like MODIFY
**	    TABLE where the CUT records for encrypted table rows may be
**	    much shorter than the rows as stored in DMF.
*/
DB_STATUS
psl_cp1_copy(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb,
	PSS_Q_XLATE	*xlated_qry,
	PTR		scanbuf_ptr,
	PSS_WITH_CLAUSE *with_clauses,
	DB_ERROR	*err_blk)
{

    QEF_RCB		*qef_rcb;
    ADF_CB		*adf_scb = (ADF_CB*) sess_cb->pss_adfcb;
    DMT_ATT_ENTRY	*att_entry;
    GCA_TD_DATA		*sqlda;
    QEU_COPY		*qe_copy;
    QEU_CPDOMD		*cpdom_desc;
    ADI_FI_DESC		*adi_fdesc;
    DB_DATA_VALUE	*src_dbv, *dest_dbv;
    DB_DATA_VALUE	cp_dbv, tmp_dbv;
    ADI_DT_NAME		src_dtname, dest_dtname;
    ADI_FI_ID		cvid;
    DB_STATUS		status = E_DB_OK;
    i4		err_code;
    i4		tab_mask;
    i4		num_atts;
    i4			i;
    i4	    		dt_bits;
    i4		perms_required;
    DMR_CB		*dmr_cb;
    i4			min_pages, max_pages;
    i4		storage_structure;
    i4		char_count;
    DMU_CHAR_ENTRY	*chr;
    i4                  peripheral_seen = 0;
    QEU_CPATTINFO	*cpattcur=NULL;
    i4		        ext_offset=0;
    PST_QNODE           *def_node;		/* default value tree node */
    QEU_MISSING_COL     *missing_col = NULL;
    i4                  check_missing;
    i4			seq_defaults;
    i4			seq_datalen;
    bool		proto_exp_imp;
    i2			null_adjust;
    i4			proto_shrinkage = 0;
    GCA_DBMS_VALUE	*gca_attdbv;

    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
    {
	/*
	** Add the rest of the copy statement.
	*/
	status = psq_x_add(sess_cb, scanbuf_ptr, &sess_cb->pss_ostream,
	    xlated_qry->pss_buf_size, &xlated_qry->pss_q_list,
	    (i4) ((char *) sess_cb->pss_endbuf - (char *) scanbuf_ptr),
	    " ", " ", (char *) NULL, err_blk);
	/* 
	** For the distributed case, we will pass to QEF the
	** translated query text, so we make the QSF object root point
	** to the first packet.  In addition, we reset query mode to 
	** PSQ_DDCOPY to make the distributed case distinguishable for SCF.
	*/
	status = psf_mroot(sess_cb, &sess_cb->pss_ostream,
			(PTR) xlated_qry->pss_q_list.pss_head, err_blk);
	if (status != E_DB_OK)
	    return (status);

	psq_cb->psq_mode = PSQ_DDCOPY;
        return (status);
    }

    qef_rcb  = (QEF_RCB *) sess_cb->pss_object;
    qe_copy  = qef_rcb->qeu_copy;
    dmr_cb   = qe_copy->qeu_dmrcb;
    chr = (DMU_CHAR_ENTRY *)dmr_cb->dmr_char_array.data_address;

    tab_mask = sess_cb->pss_resrng->pss_tabdesc->tbl_status_mask;

    /*
    ** Check if the user has specified a view.
    */
    if (tab_mask & DMT_VIEW)
    {
	(VOID) psf_error(5818L, 0L, PSF_USERERR, &err_code, err_blk, 1,
	                 psf_trmwhite(sizeof(DB_TAB_NAME), 
	        (char *) &sess_cb->pss_resrng->pss_tabdesc->tbl_name),
	                 &sess_cb->pss_resrng->pss_tabdesc->tbl_name);
    	return (E_DB_ERROR);
    }

    /*
    ** Check if user is trying to copy into a secondary index
    */
    if ((tab_mask & DMT_IDX) && (qe_copy->qeu_direction == CPY_FROM))
    {
        (VOID) psf_error(2106L, 0L, PSF_USERERR, &err_code, err_blk, 1,
	                 psf_trmwhite(sizeof(DB_TAB_NAME), 
	        (char *) &sess_cb->pss_resrng->pss_tabdesc->tbl_name),
	                 &sess_cb->pss_resrng->pss_tabdesc->tbl_name);
        return (E_DB_ERROR);
    }

    /*
    ** Check to see if user has permission to issue COPY
    */
       if(qe_copy->qeu_direction == CPY_INTO) 
	       perms_required = DB_COPY_INTO;
	else
	       perms_required = DB_COPY_FROM;

       status = psy_cpyperm(sess_cb, perms_required, err_blk);
       if (DB_FAILURE_MACRO(status))
           return(status);

    /*
    ** This is probably some sort of security violation, but we allow
    ** people to circumvent integrity checks with COPY ... so don't
    ** check for integrity constraints.  In the future, perhaps we
    ** should build some sort of compiled expression here to check
    ** integrities at runtime.
    */
    /* if ((tab_mask & DMT_INTEGRITIES) && (qe_copy->qeu_direction== CPY_FROM))
    ** {
    **     (VOID) psf_error(58xxL, 0L, PSF_USERERR, &err_code,
    **	        err_blk, sizeof(DB_TAB_NAME),
    **		&sess_cb->pss_resrng->pss_tabdesc->tbl_name);
    **	    return (E_DB_ERROR);
    ** }
    */

    /*
    ** Validate that the options specified are OK
    */
    if ( with_clauses )
    {
	min_pages = 0;
	max_pages = 0;

	storage_structure = sess_cb->pss_resrng->pss_tabdesc->tbl_storage_type;

	char_count = dmr_cb->dmr_char_array.data_in_size / 
				sizeof( DMU_CHAR_ENTRY );

	for ( i = 0; i < char_count ; i++)
	{
		switch (chr[i].char_id)
		{
		case DMU_MINPAGES:
			min_pages = chr[i].char_value;
			continue;
		case DMU_MAXPAGES:
			max_pages = chr[i].char_value;
			continue;
		default:
			continue;
		}
	}

	/* Pass 0 for allocation because we still check that one at
	** parse time for COPY, we don't deal with partitioning.
	*/
	status = psl_validate_options( sess_cb, PSQ_COPY, with_clauses,
				       storage_structure, min_pages, 
				       max_pages, (i4) FALSE, (i4) FALSE,
				       0,
				       err_blk);
	if (DB_FAILURE_MACRO(status))
	    return( status );
    }

    /*
    ** Fill in some QEU_COPY fields that will be needed to process the COPY.
    **
    ** Note a change here for encryption support: the answer to the question
    ** "How wide is that relation?" is not so easy anymore, because DMF has
    ** to know about the physical width with encryption expansion and the
    ** front ends (mostly) care about the logical data width. The COPY code
    ** that pulls from tuples and stores to CUT and thence to files must
    ** say one thing to DMF and other to CUT.
    */
    qe_copy->qeu_tup_length = sess_cb->pss_resrng->pss_tabdesc->tbl_data_width;
    qe_copy->qeu_tup_physical = sess_cb->pss_resrng->pss_tabdesc->tbl_width;
if (qe_copy->qeu_tup_length == 0)
{
TRdisplay("CRYPT_FIXME in psl_cp1_copy qeu_tup_length=%d so reset to qeu_tup_physical=%d\n",qe_copy->qeu_tup_length,qe_copy->qeu_tup_physical);
qe_copy->qeu_tup_length = qe_copy->qeu_tup_physical; 
}
    qe_copy->qeu_ext_length = qe_copy->qeu_tup_length; 
    qe_copy->qeu_dmscb = NULL;		/* Filled in below if needed */

    /*
    ** Build GCA_TD_DATA to describe the copy tuples being sent between
    ** FE and DBMS.
    */
    num_atts = (i4) sess_cb->pss_resrng->pss_tabdesc->tbl_attr_count;
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(GCA_TD_DATA) +
	    ((num_atts - 1) * sizeof(GCA_COL_ATT)),
	    (PTR *) &qe_copy->qeu_tdesc, err_blk);
    if (status != E_DB_OK)
	return (status);

    sqlda = (GCA_TD_DATA *) qe_copy->qeu_tdesc;
    sqlda->gca_result_modifier = 0;
    sqlda->gca_tsize = 0;
    sqlda->gca_l_col_desc = num_atts;
    qe_copy->qeu_att_info = NULL;
    qe_copy->qeu_missing = NULL;

    /* Do we need to check for missing attributes ? */
    check_missing = ( ( (qe_copy->qeu_stat & CPY_NOTARG) == 0) && 
                      (qe_copy->qeu_direction == CPY_FROM) );
    seq_defaults = 0;
    seq_datalen = 0;

    for (i = 0; i < num_atts; i++)
    {
	att_entry = sess_cb->pss_resrng->pss_attdesc[i+1];
	sqlda->gca_col_desc[i].gca_attdbv.db_data = 0;
	sqlda->gca_col_desc[i].gca_attdbv.db_length = att_entry->att_width;
	sqlda->gca_col_desc[i].gca_attdbv.db_datatype = att_entry->att_type;
	sqlda->gca_col_desc[i].gca_attdbv.db_prec = att_entry->att_prec;

	/*
	** For each row, check to see if the datatype can go back to the
	** frontend program as is.  If not, for copy, pretend that the
	** datatype is really CHAR (which is as close as we come to BYTE for
	** the moment).
	*/
	status = adi_dtinfo(adf_scb, att_entry->att_type, &dt_bits);
	if (status)
	{
	    (VOID) psf_error(E_PS0C05_BAD_ADF_STATUS,
		adf_scb->adf_errcb.ad_errcode, PSF_INTERR, &err_code,
		err_blk, 0);
	    return (E_DB_ERROR);
	}

        if ( check_missing )
        {
            /* For a non-binary COPY FROM commands, if this attribute isn't
            ** specified in the command, then add its default value to the
            ** list of missing columns.
            **
            ** Is this attribute specified in the copy statement ?
            */
            for (cpdom_desc = qe_copy->qeu_fdesc->cp_fdom_desc;
                 cpdom_desc != NULL;
                 cpdom_desc = cpdom_desc->cp_next)
            {
		char tmp_name[DB_ATT_MAXNAME+1];
                if (cpdom_desc->cp_type == CPY_DUMMY_TYPE) continue;

		/* Space-pad the parsed name, since otherwise it will only
	 	** check it as a prefix in STbcompare. (kibro01) b121642
		*/
		STmove(cpdom_desc->cp_domname, ' ', sizeof(tmp_name), tmp_name);
                if (STbcompare(tmp_name, DB_ATT_MAXNAME,
			att_entry->att_name.db_att_name, DB_ATT_MAXNAME,
			FALSE) == 0)
                   break;
            }

            if (!cpdom_desc)
            {
                /* User hasn't specified this attribute, so get the
                ** default value for it.
                */
                QEU_MISSING_COL *mcatt;

                /* Is this a Mandatory column ? If so, then we must
                ** abort the copy from command.
                */

                if ((att_entry->att_flags & DMU_F_NDEFAULT)
                    || ((att_entry->att_type > 0)
                        && (EQUAL_CANON_DEF_ID(att_entry->att_defaultID,
                                           DB_DEF_NOT_SPECIFIED))))
                {
                   /* Mandatory column value not present */
                   (void) psf_error(2779L, 0L, PSF_USERERR, &err_code,
                                    err_blk, 1,
                                    psf_trmwhite(sizeof(DB_ATT_NAME),
                                                 (char *) &att_entry->att_name),
                                    &att_entry->att_name);

                   return E_DB_ERROR;
                }
		
		if (! (dt_bits & AD_PERIPHERAL))
		{
                    /* Request enough memory to store the QEU_MISSING_COL
                    ** structure and the (default) attribute value.
		    ** Make sure that the value will be aligned.
                    */
                    status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
				DB_ALIGN_MACRO(sizeof(QEU_MISSING_COL)) +
				sqlda->gca_col_desc[i].gca_attdbv.db_length,
				(PTR *) &mcatt, err_blk);
                    if (status != E_DB_OK)
                        return(status);

                    /* This attribute isn't specified in the COPY statement.
                    ** Get the default value convert it to the required type
                    ** and tag on the end of the list of missing cols, ready
                    ** for replacing the value supplied by FE.
                    */
                    status = psl_make_default_node(sess_cb, &sess_cb->pss_ostream,
                                                   sess_cb->pss_resrng, i+1,
                                                   &def_node, err_blk);
                    if (status != E_DB_OK)
                        return(status);

                  if ( def_node->pst_sym.pst_type == PST_COP)
                  {
                      ADI_OP_ID opid;
                      opid = def_node->pst_sym.pst_value.pst_s_op.pst_opno;

                      status = psl_proc_func(sess_cb, &sess_cb->pss_ostream, opid,(PST_QNODE *) NULL,
                               &def_node, err_blk);

                      if (status != E_DB_OK)
                          return(status);
                  }
                    mcatt->mc_dv.db_length =
                            sqlda->gca_col_desc[i].gca_attdbv.db_length;
                    mcatt->mc_dv.db_datatype =
                            sqlda->gca_col_desc[i].gca_attdbv.db_datatype;
                    mcatt->mc_dv.db_prec =
                            sqlda->gca_col_desc[i].gca_attdbv.db_prec;
                    mcatt->mc_dv.db_data =
                            (PTR) mcatt + DB_ALIGN_MACRO(sizeof(QEU_MISSING_COL));

		    if (def_node->pst_sym.pst_type == PST_CONST)
		    {
			/* For real constants, coerce the value to the
			** column datatype now, leaving the coerced value
			** where we can get it later (mc_dv.db_data).
			*/
			if ((status = adc_cvinto(adf_scb,
                                             &def_node->pst_sym.pst_dataval,
                                             &mcatt->mc_dv)))
			{
			   return status;
			}
			/* Make sure we don't think it's a sequence default */
			mcatt->mc_seqdv.db_data = NULL;
		    }
		    else if (def_node->pst_sym.pst_type == PST_SEQOP)
		    {
			/* Sequence default.  Remember that we need sequence
			** processing, and remember the SEQOP node address
			** so that we can come back to it later.
			** Hack!! just stick it in mc_seqdv temporarily.
			*/
			++ seq_defaults;
			seq_datalen += DB_ALIGN_MACRO(def_node->pst_sym.pst_dataval.db_length);
			mcatt->mc_seqdv.db_data = (PTR) def_node;
		    }
		    /* No other types possible at present */

                    /* Need to store the attribute identity and where the
                    ** attribute data is located in the provided tuple
                    */
                    mcatt->mc_attseq = i;
                    mcatt->mc_tup_offset = att_entry->att_offset;
                    mcatt->mc_next = 0;
		    mcatt->mc_dtbits = dt_bits;

                    if(!missing_col)
                        qe_copy->qeu_missing=mcatt;
                    else
                        missing_col->mc_next=mcatt;
                    missing_col=mcatt;
		}
            }
        }

	if (dt_bits & (AD_NOEXPORT | AD_CONDEXPORT))
	{
	    DB_DATA_VALUE		db_value;
	    DB_DATA_VALUE		ev_value;

	    db_value.db_datatype = att_entry->att_type;
	    db_value.db_length = att_entry->att_width;
	    db_value.db_prec = att_entry->att_prec;
	    /* 
	    ** Call adc_dbtoev() to make sure we can export
	    ** this datatype.
	    */
	    status = adc_dbtoev(adf_scb, &db_value, &ev_value);
	    if (status)
	    {
		(VOID) psf_error(E_PS0C05_BAD_ADF_STATUS,
		  adf_scb->adf_errcb.ad_errcode, PSF_INTERR, &err_code,
		  err_blk, 0);
		return (E_DB_ERROR);
	    }
	    /* 
	    ** UDT is always COPY out as CHAR type for the time being.
	    */
	    if (dt_bits & AD_USER_DEFINED)
	    {
		if (dt_bits & AD_PERIPHERAL)
		{
		/*  Bug 65565
		    Leave the base type as is
		    The xform routine is responsible for changing the form
		*/
		}
		else
		{
		    if(sqlda->gca_col_desc[i].gca_attdbv.db_datatype > 0)
		       sqlda->gca_col_desc[i].gca_attdbv.db_datatype= DB_CHA_TYPE;
		    else
		       sqlda->gca_col_desc[i].gca_attdbv.db_datatype= -DB_CHA_TYPE;
	     	}
		sqlda->gca_col_desc[i].gca_attdbv.db_prec = 0;
	    }
	}
	
	/* Also check for peripheral status... */

	if (dt_bits & AD_PERIPHERAL)
	{
	    if (!peripheral_seen)
	    {
		peripheral_seen = 1;
		sqlda->gca_result_modifier |= GCA_LO_MASK;
	    }
	    sqlda->gca_col_desc[i].gca_attdbv.db_length =
		ADE_LEN_UNKNOWN;
	    sqlda->gca_col_desc[i].gca_attdbv.db_prec = 0;
	}
	else
	{
            gca_attdbv = &sqlda->gca_col_desc[i].gca_attdbv;

            proto_exp_imp = FALSE;
            if ( !(adf_scb->adf_proto_level & AD_I8_PROTO) && 
                 (abs(gca_attdbv->db_datatype) == DB_INT_TYPE) )
            {
                if(gca_attdbv->db_datatype < 0)
                {
                    null_adjust = 1;
                }
                else
                {
                    null_adjust = 0;
                }
                if((gca_attdbv->db_length - null_adjust) == sizeof(i8))
                {
                    gca_attdbv->db_length -= sizeof(i4);
                    proto_exp_imp = TRUE;
                    proto_shrinkage += sizeof(i4);
                }
            }

            if ( (adf_scb->adf_max_decprec < CL_MAX_DECPREC) &&
                 (abs(gca_attdbv->db_datatype) == DB_DEC_TYPE) )
            {
                /* Downgrade max_decprec to value supported by the client */
                i4      prec;
                i4      scale;
                i4      old_length;

                prec = (i4)DB_P_DECODE_MACRO(gca_attdbv->db_prec);
                if (prec > adf_scb->adf_max_decprec)
                {
                    old_length = gca_attdbv->db_length;
                    prec = adf_scb->adf_max_decprec;
                    scale = min((i4)DB_S_DECODE_MACRO(gca_attdbv->db_prec), prec);
                    gca_attdbv->db_prec = DB_PS_ENCODE_MACRO(prec, scale);
                    gca_attdbv->db_length = DB_PREC_TO_LEN_MACRO(prec);
                    if(gca_attdbv->db_datatype < 0)
                    {
                        gca_attdbv->db_length++;
                    }
                    proto_exp_imp = TRUE;
                    proto_shrinkage += old_length - gca_attdbv->db_length; 
                }
            }

	    /*
	    ** Expand external length and/or deal with protocol conversion
            ** if necessary.
	    */
	    if((gca_attdbv->db_length > att_entry->att_width) || proto_exp_imp)
	    {
		QEU_CPATTINFO *cpatt;

	        qe_copy->qeu_ext_length += 
		    (gca_attdbv->db_length - att_entry->att_width);
		/*
		** Save original attribute information so we can convert back.
		*/

		status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(QEU_CPATTINFO),
		    (PTR *) &cpatt, err_blk);
		if (status != E_DB_OK)
		    return(status);

		cpatt->cp_type = att_entry->att_type;
	    	cpatt->cp_length = att_entry->att_width;
		cpatt->cp_prec = att_entry->att_prec;
		cpatt->cp_attseq = i;
		cpatt->cp_tup_offset = att_entry->att_offset;
		cpatt->cp_ext_offset = att_entry->att_offset+ext_offset;
		cpatt->cp_flags = QEU_CPATT_EXP_INP;
		cpatt->cp_next= 0;
		if(!cpattcur)
			qe_copy->qeu_att_info=cpatt;
		else
			cpattcur->cp_next=cpatt;
		cpattcur=cpatt;
				
	        ext_offset+= gca_attdbv->db_length - att_entry->att_width;
	    }
	    sqlda->gca_tsize += gca_attdbv->db_length;
	}

	sqlda->gca_col_desc[i].gca_l_attname = 0;
    }

    /* If we encountered any sequence defaults, we need to set things up
    ** for them.  Allocate a DMS_CB, plus enough DMS_SEQ_ENTRY's for each
    ** sequence default we saw.  It may be that some of 'em reference the
    ** same sequence, in which case an entry or two may go to waste, but
    ** who cares, it's easier this way.  (Also allocate space for the
    ** sequence values as returned in native form from DMF.)
    */

    if (seq_defaults > 0)
    {
	char *value_base;
	DMS_CB *dmscb;
	DMS_SEQ_ENTRY *seq_base, *seq_entry;
	i4 entry_count;

	status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
		sizeof(DMS_CB) + seq_defaults * sizeof(DMS_SEQ_ENTRY) + seq_datalen,
		(PTR *) &dmscb, err_blk);
	if (status != E_DB_OK)
	    return (status);
	qe_copy->qeu_dmscb = dmscb;
	seq_base = (DMS_SEQ_ENTRY *) ((char *) dmscb + sizeof(DMS_CB));
	value_base = (char *) seq_base + seq_defaults * sizeof(DMS_SEQ_ENTRY);

	/* Fill in the DMS CB boilerplate stuff */

	MEfill(sizeof(DMS_CB), 0, dmscb);
	dmscb->length = sizeof(DMS_CB);
	dmscb->type = DMS_SEQ_CB;
	dmscb->ascii_id = DMS_ASCII_ID;
	dmscb->dms_flags_mask = DMS_NEXT_VALUE;
	/* Let runtime fill in db-id, tran-id */
	dmscb->dms_seq_array.data_address = (char *) seq_base;
	dmscb->dms_seq_array.data_out_size = 0;

	/* We'll count up the data_in_size after we fill in seq entries.
	** The data-out size is not used.
	** Next, chase the QEU_MISSING_COL list, looking for entries with
	** a non-null mc_seqdv.db_data.  These pointers have been hijacked
	** to point to the SEQOP default nodes with the info we need.
	** For each SEQOP, either find a matching DMS_SEQ_ENTRY or fill
	** in a new one, and then hook up both the seq-entry and the
	** mc_seqdv.db_data with the next available value slot.
	*/

	entry_count = 0;
	missing_col = qe_copy->qeu_missing;
	while (missing_col != NULL)
	{
	    def_node = (PST_QNODE *) missing_col->mc_seqdv.db_data;
	    if (def_node == NULL)
	    {
		/* Not a sequence default, just move along */
		missing_col = missing_col->mc_next;
		continue;
	    }
	    /* Set up sequence native type for coercion */
	    STRUCT_ASSIGN_MACRO(def_node->pst_sym.pst_dataval, missing_col->mc_seqdv);
	    /* Search for a sequence match, if none then fill in the next
	    ** open DMS_SEQ_ENTRY.
	    */
	    seq_entry = seq_base;
	    for (i = entry_count; i > 0; --i, ++seq_entry)
	    {
		if (MEcmp((PTR) &seq_entry->seq_name,
			  (PTR) &def_node->pst_sym.pst_value.pst_s_seqop.pst_seqname,
			  sizeof(DB_NAME)) == 0
		  && MEcmp((PTR) &seq_entry->seq_owner,
			   (PTR) &def_node->pst_sym.pst_value.pst_s_seqop.pst_owner,
			   sizeof(DB_OWN_NAME)) == 0)
		{
		    /* Found a match, just set up mc_seqdv value address */
		    missing_col->mc_seqdv.db_data = seq_entry->seq_value.data_address;
		    break;
		}
	    }
	    if (i == 0)
	    {
		/* Set up a new dms_seq_entry for this sequence default.
		** seq_entry already points at the proper (next) slot.
		*/
		++ entry_count;
		MEcopy((PTR) &def_node->pst_sym.pst_value.pst_s_seqop.pst_seqname,
			sizeof(DB_NAME), (PTR) &seq_entry->seq_name);
		MEcopy((PTR) &def_node->pst_sym.pst_value.pst_s_seqop.pst_owner,
			sizeof(DB_OWN_NAME), (PTR) &seq_entry->seq_owner);
		STRUCT_ASSIGN_MACRO(def_node->pst_sym.pst_value.pst_s_seqop.pst_seqid, seq_entry->seq_id);
		seq_entry->seq_seq = seq_entry->seq_cseq = NULL;
		seq_entry->seq_value.data_address = value_base;
		value_base += DB_ALIGN_MACRO(def_node->pst_sym.pst_dataval.db_length);
		seq_entry->seq_value.data_in_size = 0;
		seq_entry->seq_value.data_out_size = def_node->pst_sym.pst_dataval.db_length;
		missing_col->mc_seqdv.db_data = seq_entry->seq_value.data_address;
	    }
	    /* Having set everything up, check for an optimization:  if
	    ** the column exactly matches the sequence, we can point the
	    ** missing-data value at the returned sequence value, and
	    ** avoid doing any coercion at runtime.
	    ** This is left till now so that we can have all the sequence
	    ** generator stuff set up properly!
	    */
	    if (missing_col->mc_seqdv.db_datatype == missing_col->mc_dv.db_datatype
	      && missing_col->mc_seqdv.db_prec == missing_col->mc_dv.db_prec
	      && missing_col->mc_seqdv.db_length == missing_col->mc_dv.db_length)
	    {
		missing_col->mc_dv.db_data = missing_col->mc_seqdv.db_data;
		missing_col->mc_seqdv.db_data = NULL;
	    }
	    missing_col = missing_col->mc_next;
	}
	dmscb->dms_seq_array.data_in_size = entry_count * sizeof(DMS_SEQ_ENTRY);

	/* There.  Now all the runtime has to do is call DMF once per row
	** to generate (all of the) next sequence values, and then run thru
	** the missing column list coercing (or copying) the sequence values
	** into the native column value holder.
        */
    }

    if ((sqlda->gca_result_modifier & GCA_LO_MASK) == 0)
    {
	/*
	** Attributes can only expand on export, not shrink.
	*/
	if ((sqlda->gca_tsize + proto_shrinkage) < qe_copy->qeu_tup_length)
	{
	    (VOID) psf_error(E_PS0C05_BAD_ADF_STATUS,
		adf_scb->adf_errcb.ad_errcode, PSF_INTERR, &err_code,
		err_blk, 0);
	    return (E_DB_ERROR);
	}
    }


    if ((qe_copy->qeu_stat & CPY_NOTARG) == 0)
    {
	bool		leave_loop = TRUE;

	for (cpdom_desc = qe_copy->qeu_fdesc->cp_fdom_desc;
	     cpdom_desc != NULL; cpdom_desc = cpdom_desc->cp_next)
	{
	    if (cpdom_desc->cp_length == 0)
		cpdom_desc->cp_length = ADE_LEN_UNKNOWN;

	    if (cpdom_desc->cp_type == CPY_DUMMY_TYPE)
	    {
		if (STlength(cpdom_desc->cp_domname) > GCA_MAXNAME)
		{
		    i4 max = GCA_MAXNAME;

		    /*
		    ** In scscopy.c we build the gca_row_desc and
		    ** copy up to GCA_MAXNAME bytes of the cp_domname.
		    ** 
		    ** If DB_ATT_MXNAME > GCA_MAXNAME, cp_domname is truncated.
		    ** This is ok because the truncated cp_domname is mostly
		    ** used in iicopy.c for error notifications.
		    **
		    ** The only exception is cp_domname for dummy columns.
		    ** In this case the dummy column name cannot be truncated,
		    ** so below we limit the size of dummy column names to
		    ** GCA_MAXNAME.
		    */
		    (VOID) psf_error(2733, 0L, PSF_USERERR, &err_code,
			&psq_cb->psq_error, 3, 12, "DUMMY COLUMN",
			psf_trmwhite(DB_ATT_MAXNAME, cpdom_desc->cp_domname),
				cpdom_desc->cp_domname,
			sizeof(max), &max);
		    return (E_DB_ERROR);
		}

		/* BUG 2684:
		** set cp_cvlen to 0 so IICOPY correctly computes
		** the total length of the file row.
		*/

		cpdom_desc->cp_cvprec = 0;
		cpdom_desc->cp_cvlen = 0;

		/* BUG 4537: no more processing needed for dummy column */

		continue;   
	    }

	    cpdom_desc->cp_cvprec = cpdom_desc->cp_prec;
	    cpdom_desc->cp_cvlen  = cpdom_desc->cp_length;
	    cp_dbv.db_datatype    = cpdom_desc->cp_type;
	    cp_dbv.db_prec        = cpdom_desc->cp_prec;
	    cp_dbv.db_length      = cpdom_desc->cp_length;

	    /* Make temp work copy of attribute descriptor */
	    DB_COPY_ATT_TO_DV( &sqlda->gca_col_desc[cpdom_desc->cp_tupmap],
			       &tmp_dbv );

	    /* Determine source/destination of conversion */
	    if (qe_copy->qeu_direction == CPY_INTO)
	    {
		src_dbv = &tmp_dbv;
		dest_dbv = &cp_dbv;
	    }
	    else
	    {
		src_dbv = &cp_dbv;
		dest_dbv = &tmp_dbv;
	    }

	    status = E_DB_OK;
	    do    /* to break off in case of error */
	    {
		if ((status = adi_cpficoerce(adf_scb, 
				src_dbv->db_datatype,
				dest_dbv->db_datatype, &cvid)) 
			    != E_DB_OK)
		    break;

		cpdom_desc->cp_cvid = cvid;

		if ((status = adi_fidesc(adf_scb, cvid, &adi_fdesc)) 
			    != E_DB_OK)
		    break;

		if (qe_copy->qeu_direction == CPY_INTO &&
			    dest_dbv->db_length == ADE_LEN_UNKNOWN)
		{
		    if (dest_dbv->db_datatype == src_dbv->db_datatype)
		    {
			dest_dbv->db_prec   = src_dbv->db_prec;
			dest_dbv->db_length = src_dbv->db_length;
		    }
		    else if (abs(dest_dbv->db_datatype) == 
					    abs(src_dbv->db_datatype))
		    {
			if (src_dbv->db_length != ADE_LEN_UNKNOWN)
			{
			    if (dest_dbv->db_datatype > 0)
			    {
				dest_dbv->db_length = src_dbv->db_length - 1;
			    }
			    else
			    {
				dest_dbv->db_length = src_dbv->db_length + 1;
			    }
			}
		    }
		    else
		    {
                        u_i4 old_flags = adf_scb->adf_misc_flags;

                        adf_scb->adf_misc_flags |= ADF_LONG_DATE_STRINGS;
			status = adi_0calclen(adf_scb, 
				    &adi_fdesc->adi_lenspec, 1,
				    &src_dbv, dest_dbv);
                        adf_scb->adf_misc_flags = old_flags;
		    }
		    cpdom_desc->cp_cvprec = dest_dbv->db_prec;
		    cpdom_desc->cp_cvlen  = (i4) dest_dbv->db_length;
		}
		
		/* leave_loop has already been set to TRUE */
	    } while (!leave_loop);

	    /* Save changes to attribute descriptor */
	    DB_COPY_DV_TO_ATT( &tmp_dbv,
			       &sqlda->gca_col_desc[cpdom_desc->cp_tupmap] );

	    if (status != E_DB_OK)
	    {
		(VOID) adi_tyname(adf_scb, abs(src_dbv->db_datatype),
		    &src_dtname);
		(VOID) adi_tyname(adf_scb, abs(dest_dbv->db_datatype),
		    &dest_dtname);
		(VOID) psf_error(5830L, 0L, PSF_USERERR, &err_code,
		    err_blk, 3,
		    psf_trmwhite(DB_ATT_MAXNAME, cpdom_desc->cp_domname),
		    cpdom_desc->cp_domname,
		    psf_trmwhite(sizeof(src_dtname), (char *) &src_dtname),
		    &src_dtname,
		    psf_trmwhite(sizeof(dest_dtname), (char *) &dest_dtname),
		    &dest_dtname);
		return (E_DB_ERROR);    
	    }

	    /* convert value for null, if specified, to result format */
	    if (cpdom_desc->cp_withnull == TRUE &&
		    cpdom_desc->cp_nulldbv.db_length != (i4) 0)
	    {
		src_dbv = &cpdom_desc->cp_nulldbv;
		dest_dbv = &cp_dbv;

		if (cpdom_desc->cp_length == ADE_LEN_UNKNOWN)
		{
		    /* compute user-length (external length) of the 
		    ** null substitution value. */


		    if (dest_dbv->db_datatype == src_dbv->db_datatype)
		    {
			dest_dbv->db_prec = src_dbv->db_prec;
			dest_dbv->db_length = src_dbv->db_length;
		    }
		    else
		    {
			status = adi_cpficoerce(adf_scb, 
				src_dbv->db_datatype,
				dest_dbv->db_datatype, &cvid);
			if (status != E_DB_OK)
			{
			    /* value for null not compatible with row format */
			    (VOID) psf_error(5865L, 0L, PSF_USERERR, 
				    &err_code, err_blk, 1,
				    STtrmwhite(cpdom_desc->cp_domname), 
				    cpdom_desc->cp_domname);
			    return (E_DB_ERROR);
			}

			status = adi_fidesc(adf_scb, cvid, &adi_fdesc);
			if (status != E_DB_OK)
			{
			    /* value for null not compatible with row format */
			    (VOID) psf_error(5865L, 0L, PSF_USERERR, 
				    &err_code, err_blk, 1,
				    STtrmwhite(cpdom_desc->cp_domname), 
				    cpdom_desc->cp_domname);
			    return (E_DB_ERROR);
			}

			status = adi_0calclen(adf_scb, 
			    &adi_fdesc->adi_lenspec, 1,
			    &src_dbv, dest_dbv);
			if (status != E_DB_OK)
			{
			    /* value for null not compatible with row format */
			    (VOID) psf_error(5865L, 0L, PSF_USERERR, 
				    &err_code, err_blk, 1,
				    STtrmwhite(cpdom_desc->cp_domname), 
				    cpdom_desc->cp_domname);
			    return (E_DB_ERROR);
			}
		    }
		}

		if ((status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
			(i4) dest_dbv->db_length, &dest_dbv->db_data,
			err_blk)) != E_DB_OK)
		{
		    return (status);
		}

		cpdom_desc->cp_nuldata = dest_dbv->db_data;
		cpdom_desc->cp_nulprec = dest_dbv->db_prec;
		cpdom_desc->cp_nullen = dest_dbv->db_length;

		status = adi_dtinfo(adf_scb,
				    dest_dbv->db_datatype,
				    &dt_bits);
		if (status)
		{
		    (VOID) psf_error(E_PS0C05_BAD_ADF_STATUS,
				     adf_scb->adf_errcb.ad_errcode,
				     PSF_INTERR,
				     &err_code,
				     err_blk,
				     0);
		    return (E_DB_ERROR);
		}

		if (abs(dest_dbv->db_datatype) == DB_VCH_TYPE ||
		    abs(dest_dbv->db_datatype) == DB_TXT_TYPE ||
		    (dt_bits & AD_PERIPHERAL))
		{
		    MEfill(dest_dbv->db_length - DB_CNTSIZE, 
			(u_char)0, 
			(PTR)((char *)(dest_dbv->db_data) + DB_CNTSIZE)); 
		}

		if (peripheral_seen)
		{
		    /*
		    ** If the statment involves peripheral objects,
		    ** then the appropriate null indicator will need a
		    ** transaction.  Check to see if the value is
		    ** being converted into a peripheral type.  If so,
		    ** then a transaction may be required, so start
		    ** one.  Also, once so started, turn off the
		    ** peripheral_seen indicator since it will be
		    ** unnecessary to check further.
		    */
		    
		    if (dt_bits & AD_PERIPHERAL)
		    {
			QEU_CB		qeu;
			
			qeu.qeu_length	= sizeof(qeu);
			qeu.qeu_type	= QEUCB_CB;
			qeu.qeu_owner	= (PTR)DB_PSF_ID;
			qeu.qeu_ascii_id = QEUCB_ASCII_ID;
			qeu.qeu_eflag = QEF_EXTERNAL;
			qeu.qeu_db_id = sess_cb->pss_dbid;
			qeu.qeu_d_id = (CS_SID)sess_cb->pss_sessid;
			qeu.qeu_flag = 0;
			qeu.qeu_mask = 0;
			qeu.qeu_qmode = psq_cb->psq_mode;
			/* First end RO xact, then start user xact */

			status = qef_call(QEU_ETRAN, (PTR) &qeu);
			if (!status)
			{
			    status = qef_call(QEU_BTRAN, (PTR) &qeu);
			}
			if (status)
			{
			    (VOID) psf_error(E_PS0D22_QEF_ERROR,
					     qeu.error.err_code,
					     PSF_INTERR,
					     &err_code,
					     err_blk,
					     0);
			    break;
			}
			peripheral_seen = 0;
		    }
		}
		if ((status = adc_cvinto(adf_scb, src_dbv, dest_dbv))
			!= E_DB_OK)
		{
		    /* value for null not compatible with row format */
		    (VOID) psf_error(5865L, 0L, PSF_USERERR, &err_code,
			    err_blk, 1, STtrmwhite(cpdom_desc->cp_domname), 
			    cpdom_desc->cp_domname);
		    return (E_DB_ERROR);
		}
	    }
	}
    }

    /* If it's COPY FROM, and the table is partitioned, QEF will have to
    ** do "which partition" operations on the incoming rows.  For that,
    ** it will need the partition definition...
    */
    if (qe_copy->qeu_direction == CPY_FROM
      && sess_cb->pss_resrng->pss_tabdesc->tbl_status_mask & DMT_IS_PARTITIONED)
    {
	RDF_CB rdfcb;		/* RDF call control block */

	MEfill(sizeof(RDF_CB), 0, &rdfcb);
	rdfcb.rdf_info_blk = sess_cb->pss_resrng->pss_rdrinfo;
	rdfcb.rdf_rb.rdr_db_id = sess_cb->pss_dbid;
	rdfcb.rdf_rb.rdr_unique_dbid = sess_cb->pss_udbid;
	rdfcb.rdf_rb.rdr_session_id = sess_cb->pss_sessid;
	rdfcb.rdf_rb.rdr_partcopy_mem = ppd_qsfmalloc;
	rdfcb.rdf_rb.rdr_partcopy_cb = psq_cb;
	rdfcb.rdf_rb.rdr_instr = 0;	/* Don't copy names or text */
	status = rdf_call(RDF_PART_COPY, &rdfcb);
	if (DB_SUCCESS_MACRO(status))
	    qe_copy->qeu_part_def = rdfcb.rdf_rb.rdr_newpart;
	/* Return ok/notok just below... */
    }

    return (status);
}

/*
** Name: psl_cp2_copstmnt - Semantic actions for the copstmnt production
**
** Description:
**	This routine performs the semantic action for copy table:
**	-- the copstmnt production.  It is the same in SQL and QUEL.
**
**	    copstmnt:	COPY copy_tbl_kwd	(SQL)
**	    copstmnt:	COPY			(QUEL)
**
**	The copy_tbl_kwd is a noise word.
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	    pss_ostream	    memory stream to allocate QEF_RCB
**	    pss_distrib	    indicates whether thread is distributed
**
** Outputs:
**	err_blk		    will be filled in if an error occurs
**	qmode		    set to query mode (PSQ_COPY)
**	sess_cb		    ptr to a PSF session CB
**	    pss_object	    Points to QEF_RCB
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	07-may-92 (barbara)
**	    copied from 6.5 SQL grammar
**	26-July-1993 (rmuth)
**	   Enhanced the COPY to accept a range of WITH options. These
**	   are passed onto to DMF through the  characteristics array in
**	   the DMR_CB. 
**	     - Allocate a DMR_CB to hold this information and set the 
**	       qeu_stat flag, CPY_DMR_CB, to indicate that this structure 
**	       exists.  
**	     - Allocate a structure of DMU_CHAR_ENTRY for these clauses.
**	     - Initialise the "with_clauses" mask.
**	26-Feb-2001 (jenjo02)
**	    Set session_id in QEF_RCB;
**	25-Apr-2004 (schka24)
**	    Remove qeu-cb from qeu-copy data block.
**      06-Oct-2006 (wonca01) BUG 115546
**          Initialize qeu_stat to CPY_DMR_CB in order for the  
**          with_clause values to be taken into consideration when 
**          specified for the COPY statement. 
**	05-Feb-2009 (kiria01) b121607
**	    Changed qmode type to reflect the change from i4 to PSQ_MODE.
**	11-Jun-2010 (kiria01) b123908
**	    Initialise pointers after psf_mopen would have invalidated any
**	    prior content.
*/
DB_STATUS
psl_cp2_copstmnt(
	PSS_SESBLK	*sess_cb,
	PSQ_MODE	*qmode,
	PSS_WITH_CLAUSE *with_clauses,
	DB_ERROR	*err_blk)
{

    DB_STATUS		status = E_DB_OK;
    QEF_RCB		*qef_rcb;
    QEU_COPY		*qe_copy;
    DMR_CB		*dmr_cb;

    *qmode = PSQ_COPY;

    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
    {
	/* Open QSF memory stream for the COPY text */
	status = psf_mopen(sess_cb, QSO_QTEXT_OBJ, &sess_cb->pss_ostream, err_blk);
	return (status);
    }

    /* Allocate a QEF control block for the copy */
    status = psf_mopen(sess_cb, QSO_QP_OBJ, &sess_cb->pss_ostream, err_blk);
    if (status != E_DB_OK)
	return (status);
    sess_cb->pss_stk_freelist = NULL;

    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(QEF_RCB),
		&sess_cb->pss_object, err_blk);
    if (status != E_DB_OK)
	return (status);

    status = psf_mroot(sess_cb, &sess_cb->pss_ostream, sess_cb->pss_object, err_blk);
    if (status != E_DB_OK)
	return (status);

    /* Fill in the QEF control block */
    qef_rcb = (QEF_RCB *) sess_cb->pss_object;
    qef_rcb->qef_length = sizeof(QEF_RCB);
    qef_rcb->qef_type = QEFRCB_CB;
    qef_rcb->qef_owner = (PTR)DB_PSF_ID;
    qef_rcb->qef_ascii_id = QEFRCB_ASCII_ID;
    qef_rcb->qef_sess_id = sess_cb->pss_sessid;

    /* 
    ** Allocate the copy control block 
    */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(QEU_COPY),
	    (PTR *) &qef_rcb->qeu_copy, err_blk);
    if (status != E_DB_OK)
	return (status);

    /*
    ** Allocate a DMR_CB control block
    */
    status = psf_malloc( sess_cb, &sess_cb->pss_ostream, sizeof( DMR_CB ), 
		  	 (PTR *)&dmr_cb, err_blk);
    if ( status != E_DB_OK )
	return( status );

    /*
    ** First item in que_stat
    */
    qef_rcb->qeu_copy->qeu_stat = CPY_DMR_CB;
    qef_rcb->qeu_copy->qeu_dmrcb = dmr_cb;
    dmr_cb->type = DMR_RECORD_CB;
    dmr_cb->length = sizeof(DMR_CB);

    /*
    ** Allocate the array of DMU_CHAR_ARRAY entries
    */
    status = psf_malloc( sess_cb, &sess_cb->pss_ostream,
			 PSS_MAX_MODIFY_CHARS * sizeof(DMU_CHAR_ENTRY),
			 (PTR *) &dmr_cb->dmr_char_array.data_address,
			 err_blk);
    if ( status != E_DB_OK )
	return( status );

    dmr_cb->dmr_flags_mask = DMR_CHAR_ENTRIES;
    dmr_cb->dmr_char_array.data_in_size = 0;

    /*
    ** Initialise mask which tracks with_clauses
    */
    MEfill(sizeof(PSS_WITH_CLAUSE), 0, with_clauses);
		

    /* Allocate the copy file descriptor */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(QEU_CPFILED),
	    (PTR *) &qef_rcb->qeu_copy->qeu_fdesc, err_blk);
    if (status != E_DB_OK)
	    return (status);

    /* Zero out QEU_COPY fields */
    qe_copy = qef_rcb->qeu_copy;
    qe_copy->qeu_count = 0;
    qe_copy->qeu_duptup = 0;
    qe_copy->qeu_maxerrs = 0;
    qe_copy->qeu_rowspace = 0;
    qe_copy->qeu_input = NULL;
    qe_copy->qeu_output = NULL;
    qe_copy->qeu_logname = NULL;
    qe_copy->qeu_part_def = NULL;
    qe_copy->qeu_copy_ctl = NULL;
    qe_copy->qeu_fdesc->cp_filedoms = 0;
    qe_copy->qeu_fdesc->cp_filename = NULL;
    qe_copy->qeu_fdesc->cp_fdom_desc = NULL;
    qe_copy->qeu_fdesc->cp_cur_fdd = NULL;

    return (status);
}

/*
** Name: psl_cp3_copytable	- Semantic actions for the copytable production
**
** Description:
**	This routine performs some of the semantic actions for the copytable
**	(SQL and QUEL) production.
**
**	copytable:	    obj_spec		(SQL)
**	copytable	    NAME		(QUEL)
**
** Inputs:
**	sess_cb			    ptr to a PSF session CB
**	    pss_distrib		    indicates distributed thread
**	resrange		    pointer to range variable    
**	tabname			    pointer to table name
**	xlated_qry		    for query text buffering (NULL for QUEL)
**
** Outputs:
** 	err_blk		    will be filled in if an error occurs
**	sess_cb
**	    pss_object	    copy information for QEF
**	    pss_resrng	    will point to range variable    
**	ldbdesc		    will contain descriptor of LDB where table resides
**			    (Note this pointer will be null for QUEL)
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	07-may-1992 (barbara)
**	    Taken from 6.5 SQL grammar.
*/
DB_STATUS
psl_cp3_copytable(
	PSS_SESBLK	*sess_cb,
	PSS_RNGTAB	*resrange,
	DB_TAB_NAME	*tabname,
	PSS_Q_XLATE	*xlated_qry,
	DD_LDB_DESC	*ldbdesc,
	DB_ERROR	*err_blk)
{
    QEF_RCB		*qef_rcb;
    DB_TAB_TIMESTAMP	*timestamp;
    DB_STATUS		status = E_DB_OK;
    i4		err_code;
    DD_OBJ_DESC		*obj_desc; 
    DD_2LDB_TAB_INFO	*ldb_tab_info;

    /* For QUEL and SQL (non-distrib) assign values to QEF_RCB */
    if (~sess_cb->pss_distrib & DB_3_DDB_SESS)
    {
	qef_rcb = (QEF_RCB *) sess_cb->pss_object;
	STRUCT_ASSIGN_MACRO(resrange->pss_tabid, qef_rcb->qeu_copy->qeu_tab_id);
	timestamp = &resrange->pss_tabdesc->tbl_date_modified;
	STRUCT_ASSIGN_MACRO(*timestamp, qef_rcb->qeu_copy->qeu_timestamp);
	sess_cb->pss_resrng = resrange;
	return (status);
    }

    /* 
    ** For distributed, buffer text so far:
    **     "COPY TABLE tab ("
    ** and save ldb desc
    */
    obj_desc = resrange->pss_rdrinfo->rdr_obj_desc;
    ldb_tab_info = &obj_desc->dd_o9_tab_info;
    
    if (obj_desc->dd_o6_objtype == DD_3OBJ_VIEW)
    {
	(VOID) psf_error(5818L, 0L, PSF_USERERR, &err_code, err_blk, 1,
		   psf_trmwhite(sizeof(DB_TAB_NAME), (char *) tabname), 
		   tabname);
	return (E_DB_ERROR);
    }
    else if (ldb_tab_info->dd_t3_tab_type == DD_3OBJ_VIEW)
    {
	(VOID) psf_error(5838L, 0L, PSF_USERERR, &err_code, err_blk, 1,
		   psf_trmwhite(sizeof(DB_TAB_NAME), (char *) tabname), 
		   tabname);
	return (E_DB_ERROR);
    }
    else
    {
	status = psq_x_add(sess_cb, "", &sess_cb->pss_ostream, xlated_qry->pss_buf_size,
		&xlated_qry->pss_q_list, -1, (char *)NULL, (char *)NULL,
		"copy table ", err_blk);

	if (DB_FAILURE_MACRO(status))
	    return(status);

	status = psq_prt_tabname(sess_cb, xlated_qry, &sess_cb->pss_ostream, resrange,
		PSQ_COPY, err_blk);

	if (DB_FAILURE_MACRO(status))
	    return(status);

	status = psq_x_add(sess_cb, "", &sess_cb->pss_ostream, xlated_qry->pss_buf_size,
		&xlated_qry->pss_q_list, -1, (char *)NULL, (char *)NULL,
		"(", err_blk);

	/* remember table site */
	STRUCT_ASSIGN_MACRO(ldb_tab_info->dd_t9_ldb_p->dd_i1_ldb_desc,
			*ldbdesc);
    }

    sess_cb->pss_resrng = resrange;
    return (E_DB_OK);
} 

/*
** Name: psl_cp4_coparam	- Semantic actions for the coparam production
**
** Description:
**	This routine performs the semantic actions for the coparam
**	(SQL and QUEL) null production.  The null production is used when
**	the copy statement supplied no format list (implies bulk copy).
**	e.g. COPY tab t () from 'file1';
**
**	coparam:	{}
**
** Inputs:
**	sess_cb			    ptr to a PSF session CB
**	    pss_distrib		    indicates distributed thread
**	xlated_qry		    for query text buffering (NULL for QUEL)
**
** Outputs:
** 	err_blk		    will be filled in if an error occurs
**	sess_cb
**	    pss_object	    copy information for QEF
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	07-may-1992 (barbara)
**	    Taken from 6.5 SQL grammar.
**      04-Mar-1992 (fred)
**          Large object support.  Added checking and setting of
**          cp_length to unknown when dealing with a peripheral datatype.
**	14-jul-93 (robf)
**	    Non-export support, export as export type.
*/
DB_STATUS
psl_cp4_coparam(
	PSS_SESBLK	*sess_cb,
	PSS_Q_XLATE	*xlated_qry,
	DB_ERROR	*err_blk)
{
    QEF_RCB		*qef_rcb;
    QEU_COPY		*qe_copy;
    QEU_CPDOMD		*cpdom_desc;
    DB_STATUS		status = E_DB_OK;
    DMT_ATT_ENTRY	*att_entry;
    i4			attno;
    i4			num_atts;
    i4			i;
    i4		dt_bits;
    i4		err_code;
    ADF_CB		*adf_scb = (ADF_CB*) sess_cb->pss_adfcb;

    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
    {
	/* For the distributed thread buffer the empty parens and return */
	status = psq_x_add(sess_cb, "", &sess_cb->pss_ostream,
		xlated_qry->pss_buf_size, &xlated_qry->pss_q_list, (i4) -1,
		(char *) NULL, (char *) NULL, ") ", err_blk);
	return(status);
    }

    /* Non-distrib SQL and QUEL */

    qef_rcb = (QEF_RCB *) sess_cb->pss_object;
    qe_copy = qef_rcb->qeu_copy;

    /* no copy target list */
    qe_copy->qeu_stat |= CPY_NOTARG;

    /*
    ** Have to make up a copy file description based on the tuple
    ** description.
    */
    num_atts = sess_cb->pss_resrng->pss_tabdesc->tbl_attr_count;
    for (i = 0, attno = 1; attno <= num_atts; attno++)
    {
	att_entry = sess_cb->pss_resrng->pss_attdesc[attno];

	if (att_entry->att_flags & DMU_F_HIDDEN)
	    continue;

	/* Allocate file domain descriptor */
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(QEU_CPDOMD),
	    (PTR *) &cpdom_desc, err_blk);
	if (status != E_DB_OK)
	    return(status);

	STmove((char *) &att_entry->att_name, '\0', DB_ATT_MAXNAME,
	     cpdom_desc->cp_domname);

	cpdom_desc->cp_tupmap = att_entry->att_number - 1;
	status = adi_dtinfo(adf_scb, att_entry->att_type, &dt_bits);
	if (status)
	{
	    (VOID) psf_error(E_PS0C05_BAD_ADF_STATUS,
		adf_scb->adf_errcb.ad_errcode, PSF_INTERR, &err_code,
		err_blk, 0);
	    return (E_DB_ERROR);
	}

	if (dt_bits & (AD_NOEXPORT | AD_CONDEXPORT))
	{
	    DB_DATA_VALUE		db_value;
	    DB_DATA_VALUE		ev_value;

	    db_value.db_datatype = att_entry->att_type;
	    db_value.db_length = att_entry->att_width;
	    db_value.db_prec = att_entry->att_prec;
	    status = adc_dbtoev(adf_scb, &db_value, &ev_value);
	    if (status)
		return(status);
	
	     cpdom_desc->cp_type = ev_value.db_datatype;
	     if (att_entry->att_flags & DMU_F_PERIPHERAL)
	     {
		    cpdom_desc->cp_length = ADE_LEN_UNKNOWN;
	     }
	     else
	     {
		    cpdom_desc->cp_length = ev_value.db_length;
	     }
	     cpdom_desc->cp_prec = ev_value.db_prec;
	}
	else
	{
		cpdom_desc->cp_type = att_entry->att_type;
		if (att_entry->att_flags & DMU_F_PERIPHERAL)
		{
		    cpdom_desc->cp_length = ADE_LEN_UNKNOWN;
		}
		else
		{
		    cpdom_desc->cp_length = att_entry->att_width;
		}
		cpdom_desc->cp_prec = att_entry->att_prec;
	}
	cpdom_desc->cp_user_delim = FALSE;
	cpdom_desc->cp_next = NULL;
	cpdom_desc->cp_cvid = ADI_NOFI;
	cpdom_desc->cp_cvlen = 0;
	cpdom_desc->cp_cvprec = 0;
	cpdom_desc->cp_withnull = FALSE;
	cpdom_desc->cp_nullen = 0;
	cpdom_desc->cp_nulprec = 0;
	cpdom_desc->cp_nuldata = (PTR) NULL;

	if (++i == 1)
	{
	    qe_copy->qeu_fdesc->cp_fdom_desc = cpdom_desc;
	}
	else
	{
	    qe_copy->qeu_fdesc->cp_cur_fdd->cp_next = cpdom_desc;
	}
	qe_copy->qeu_fdesc->cp_cur_fdd = cpdom_desc;
    }

    qe_copy->qeu_fdesc->cp_filedoms = i;
    return (E_DB_OK);
}


/*
** Name: psl_cp5_cospecs	- Semantic actions for the cospecs production
**
** Description:
**	This routine performs the semantic actions for the cospecs
**	(SQL and QUEL) production.  cospecs is the top level rule for
**	parsing the copy format specifications.
**
**	cospecs:	entname EQUAL fmtspec
**		|	cospecs COMMA entname EQUAL fmtspec
**
** Inputs:
**	sess_cb			    ptr to a PSF session CB
**	    pss_distrib		    indicates distributed thread
**	    pss_stmt_flags	    PSS_CP_DUMMY_COL set if dummy column
**	xlated_qry		    for query text buffering (NULL for QUEL)
**	domname			    name of domain
**	scanbuf_ptr		    points to beginning of format string.
**
** Outputs:
** 	err_blk		    will be filled in if an error occurs
**	sess_cb
**	    pss_object	    copy information for QEF
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	07-may-1992 (barbara)
**	    Taken from 6.5 SQL grammar.
**	16-oct-1992 (barbara)
**	    When looking ahead into scanbuf for generating text, skip
**	    intevening white space.
**	01-mar-93 (barbara)
**	    Star support for delimited ids.  Delimit and unnormalize
**	    column names.
**	03-Aug-1993 (fpang)
**	    Initialize 'unorm_len' before calling cui_idunorm().
**	    Fixes B53943, 'Spurious E_SC206's during copy.'
**	02-sep-1993
**	    Star delimited ids: To decide whether or not to delimit, Star now
**	    relies on the presence of the LDB's DB_DELIMITED_CASE capability
**	    instead of its OPEN/SQL_LEVEL.
**	31-oct-1993
**	    Supply a length to psq_x_add() when adding column name to
**	    query buffer, otherwise psq_x_add() can't handle a name with an
**	    embedded blank in it (bug 91735)
*/
DB_STATUS
psl_cp5_cospecs(
	PSS_SESBLK	*sess_cb,
	PSS_Q_XLATE	*xlated_qry,
	char		*domname,
	PTR		scanbuf_ptr,
	DB_ERROR	*err_blk)
{
    QEF_RCB		*qef_rcb;
    QEU_COPY		*qe_copy;
    QEU_CPDOMD		*cpdom_desc;
    DMT_ATT_ENTRY	*coldesc;
    char		*delimiter;
    DB_STATUS		status = E_DB_OK;
    DB_ATT_NAME		colname;
    i4		err_code;

    if (~sess_cb->pss_distrib & DB_3_DDB_SESS)
    {
	qef_rcb = (QEF_RCB *) sess_cb->pss_object;
	qe_copy = qef_rcb->qeu_copy;
	cpdom_desc = qe_copy->qeu_fdesc->cp_cur_fdd;
    }

    /*
    ** If fmtspec is not a dummy field, then domain name must be a column name
    ** in the table being copied.  Look it up in the range table.
    */
    if ((sess_cb->pss_stmt_flags & PSS_CP_DUMMY_COL) == 0)
    {
	STmove(domname, ' ', sizeof(DB_ATT_NAME), (char *) &colname);
	coldesc = pst_coldesc(sess_cb->pss_resrng, &colname);
	if (coldesc == (DMT_ATT_ENTRY *) NULL)
	{
	    (VOID) psf_error(5801L, 0L, PSF_USERERR, &err_code,
		err_blk, 2, STlength(domname),
		domname, sizeof(DB_TAB_NAME),
		&sess_cb->pss_resrng->pss_tabdesc->tbl_name);
	    return (E_DB_ERROR);
	}

	if (~sess_cb->pss_distrib & DB_3_DDB_SESS)
	{
	    /* cp_tupmap maps the file domain to a table attribute */

	    cpdom_desc->cp_tupmap = coldesc->att_number - 1;
	}
	else
	{
	    /* if column names are mapped, use underlying column name */

	    DD_2LDB_TAB_INFO   *ldb_tab_info =
	     &sess_cb->pss_resrng->pss_rdrinfo->rdr_obj_desc->dd_o9_tab_info;

						    /* assume no mapping */
	    char		   *attr_name = domname;
	    u_i4		    name_len, unorm_len;
	    u_char		    unorm_buf[DB_MAXNAME *2 +3];

	    if (ldb_tab_info->dd_t6_mapped_b)
	    {
		attr_name =
	       ldb_tab_info->dd_t8_cols_pp[coldesc->att_number]->dd_c1_col_name;
	    }

	    if (ldb_tab_info->dd_t9_ldb_p->dd_i2_ldb_plus.dd_p3_ldb_caps.
				dd_c1_ldb_caps & DD_8CAP_DELIMITED_IDS)
	    {
	        /* Delimit and unnormalize column name */
	        name_len = (u_i4) psf_trmwhite(sizeof(DD_ATT_NAME), attr_name);
		unorm_len = sizeof(unorm_buf) - 1;

	        status = cui_idunorm((u_char *)attr_name, &name_len,
				 unorm_buf, &unorm_len, CUI_ID_DLM, err_blk);

	        if (DB_FAILURE_MACRO(status))
		    return (status);
	        unorm_buf[unorm_len] = '\0';
		attr_name = (char *)unorm_buf;
	    }

	    status = psq_x_add(sess_cb, attr_name, &sess_cb->pss_ostream,
			 xlated_qry->pss_buf_size,
			 &xlated_qry->pss_q_list, STlength(attr_name),
			 " ", " ", (char *) NULL, err_blk);
	
	    if (DB_FAILURE_MACRO(status))
	    {
		return(status);
	    }
	}
    }
    else	/* dummy domain */
    {
	if (~sess_cb->pss_distrib & DB_3_DDB_SESS)
	{
	    /*
	    ** Check to see if the domain name is a delimiter name.  If it
	    ** is, substitute the delimiter character for the domain name.
	    */
	    status = psl_copydelim(cpdom_desc->cp_domname, &delimiter);
	    if ((status == E_DB_OK) && (delimiter != cpdom_desc->cp_domname))
	    {
		STcopy(delimiter, cpdom_desc->cp_domname);
	    }
	    status = E_DB_OK;
	}
	else
	{
	    /* Buffer domain name */
	    status = psq_x_add(sess_cb, domname, &sess_cb->pss_ostream,
		xlated_qry->pss_buf_size, &xlated_qry->pss_q_list,
		(i4) -1, (char *) NULL, (char *) NULL, " ", err_blk);

	    if (DB_FAILURE_MACRO(status))
		return(status);
	}
    }

    /* For the distributed thread, buffer the format specification */
    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
    {
	u_char *c1 = sess_cb->pss_nxtchar;

	status = psq_x_add(sess_cb, scanbuf_ptr, &sess_cb->pss_ostream,
	    xlated_qry->pss_buf_size, &xlated_qry->pss_q_list,
	    (i4) ((char *) c1 - (char *) scanbuf_ptr),
	    " ", " ", (char *) NULL, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return(status);

    	/*
    	** yacc may not have looked ahead to the comma or the closing
    	** paren, depending on the column format specification.
	*/
    
	while (CMspace(c1))
	    CMnext(c1);

	if (!CMcmpcase(c1, ")"))
	{
	    status = psq_x_add(sess_cb, "", &sess_cb->pss_ostream,
			xlated_qry->pss_buf_size, &xlated_qry->pss_q_list,
			-1, (char *) NULL, (char *) NULL, ")", err_blk);
	}
	else if (!CMcmpcase(c1, ","))
	{
	    status = psq_x_add(sess_cb, "", &sess_cb->pss_ostream,
			xlated_qry->pss_buf_size, &xlated_qry->pss_q_list,
			-1, (char *) NULL, (char *) NULL, ",", err_blk);
	}
    }

    return (status);
}

/*
** Name: psl_cp6_entname	- Semantic actions for the entname production
**
** Description:
**	This routine performs the semantic actions for the entname
**	(SQL and QUEL) production.  entname is the domain name of an
**	element in the format list.
**
**	entname:	NAME
**
** Inputs:
**	sess_cb			    ptr to a PSF session CB
**	    pss_distrib		    indicates distributed thread
**	entname			    domain name
**
** Outputs:
** 	err_blk		    will be filled in if an error occurs
**	sess_cb
**	    pss_object	    copy information for QEF
**	scanbuf_ptr	    will point to format specification
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	07-may-1992 (barbara)
**	    Taken from 6.5 SQL grammar.
*/
DB_STATUS
psl_cp6_entname(
	PSS_SESBLK	*sess_cb,
	char		*entname,
	DB_ERROR	*err_blk,
	PTR		*scanbuf_ptr)
{

    QEF_RCB	*qef_rcb;
    QEU_COPY	*qe_copy;
    QEU_CPDOMD	*cpdom_desc;
    DB_STATUS	status = E_DB_OK;

    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
    {
	/*
	** Mark place in scanner buffer so that all of format specification
	** can be buffered into DD_ packets at one go.
	*/
	*scanbuf_ptr = (PTR) sess_cb->pss_nxtchar;
	return (status);
    }

    qef_rcb = (QEF_RCB *) sess_cb->pss_object;
    qe_copy = qef_rcb->qeu_copy;

    /*
    ** Allocate file domain descriptor.
    */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(QEU_CPDOMD),
	(PTR *) &cpdom_desc, err_blk);
    if (status != E_DB_OK)
	return (status);

    STmove(entname, '\0', DB_ATT_MAXNAME, cpdom_desc->cp_domname);
    /* zero out unused fields */
    cpdom_desc->cp_next = NULL;
    cpdom_desc->cp_type = 0;
    cpdom_desc->cp_length = 0;
    cpdom_desc->cp_prec = 0;
    cpdom_desc->cp_user_delim = FALSE;
    cpdom_desc->cp_delimiter[0] = '\0';
    cpdom_desc->cp_delimiter[1] = '\0';
    cpdom_desc->cp_tupmap = 0;
    cpdom_desc->cp_cvid = ADI_NOFI;
    cpdom_desc->cp_cvlen = 0;
    cpdom_desc->cp_cvprec = 0;
    cpdom_desc->cp_withnull = FALSE;
    cpdom_desc->cp_nullen = 0;
    cpdom_desc->cp_nulprec = 0;
    cpdom_desc->cp_nuldata = (PTR) NULL;

    /*
    ** Put this descriptor on chain of file domain descriptors.
    ** If it is first domain descriptor, put it on head of list.
    */
    if (qe_copy->qeu_fdesc->cp_filedoms == 0)
    {
	qe_copy->qeu_fdesc->cp_fdom_desc = cpdom_desc;
    }
    else
    {
	qe_copy->qeu_fdesc->cp_cur_fdd->cp_next = cpdom_desc;
    }

    qe_copy->qeu_fdesc->cp_cur_fdd = cpdom_desc;
	qe_copy->qeu_fdesc->cp_filedoms++;

    return (status);
}

/*
** Name: psl_cp7_fmtspec	- Semantic actions for the fmtspec production
**
** Description:
**	This routine performs the semantic actions for the fmtspec
**	(SQL and QUEL) production.  fmtspec is the rule describing the
**	format specification in the COPY format list.
**
**	fmtspec:	coent
**			{  empty  }
**		|	coent WITH NULLWORD
**		|	coent WITH NULLWORD LPAREN covalue RPAREN
**
** Inputs:
**	sess_cb			ptr to a PSF session CB
**	    pss_distrib		indicates distributed thread
**
** Outputs:
** 	err_blk		    	will be filled in if an error occurs
**	sess_cb
**	    pss_object	    	copy information for QEF
**	w_nullval	    	TRUE if "covalue" production
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	07-may-1992 (barbara)
**	    Taken from 6.5 SQL grammar.
*/
DB_STATUS
psl_cp7_fmtspec(
	PSS_SESBLK	*sess_cb,
	DB_ERROR	*err_blk,
	bool		w_nullval)
{
    QEF_RCB		*qef_rcb;
    QEU_COPY		*qe_copy;
    QEU_CPDOMD		*cpdom_desc;
    i4		err_code;

    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
	return (E_DB_OK);

    qef_rcb = (QEF_RCB *) sess_cb->pss_object;
    qe_copy = qef_rcb->qeu_copy;
    cpdom_desc = qe_copy->qeu_fdesc->cp_cur_fdd;

    if (cpdom_desc->cp_type == CPY_DUMMY_TYPE)
    {
    /* fixme COPY: null not allowed in a dummy column */
	(VOID) psf_error(5862L, 0L, PSF_USERERR, &err_code, err_blk, 1,
	    STtrmwhite(cpdom_desc->cp_domname), cpdom_desc->cp_domname);
	return (E_DB_ERROR);
    }

    cpdom_desc->cp_withnull = TRUE;

    if (w_nullval == FALSE)
    {
	if (cpdom_desc->cp_length == 0)
	{
	    (VOID) psf_error(5863L, 0L, PSF_USERERR, &err_code, err_blk, 1,
		STtrmwhite(cpdom_desc->cp_domname), cpdom_desc->cp_domname);
	    return (E_DB_ERROR);
	}

	++cpdom_desc->cp_length;
	cpdom_desc->cp_type = -(cpdom_desc->cp_type);
	cpdom_desc->cp_nulldbv.db_prec = 0;
	cpdom_desc->cp_nulldbv.db_length = 0;
	cpdom_desc->cp_nulldbv.db_data = (PTR) NULL;
    }
    return (E_DB_OK); 
}


/*
** Name:    psl_cp8_coent()	    - semantic action for coent: production
**
** Description:	    This function implements the semantic actions for coent:
**		    production
**
**	coent:	    name_or_sconst
**	     |	    NAME LPAREN intconst_p RPAREN cp_delim
**	     |	    NAME LPAREN intconst_p COMMA intconst_p RPAREN cp_delim
**
** Input:
**	sess_cb		PSF session CB
**	psq_cb		PSF request CB
**	parse_coent	TRUE if type/length/delimiter were NOT specified
**			in type(len)[delim] format (e.g. money, smallint, d0nl)
**			FALSE otherwise
**	type_name	pointer to the type name if parse_coent==FALSE
**			pointer to the type spec otherwise
**	len_prec_num	1 if type(len)[delim] was specified
**			2 if type(len, prec)[delim] was specified
**			0 otherwise
**	lenprec		ptr to array containing length [and precision] if
**			type(len[, prec])[delim] was specified
**			NULL otherwise
**	type_delim	delimiter if type(len[, prec])delim was specified
**			NULL otherwise
**
** Output:
**	sess_cb
**	  pss_stmt_flags PSS_CP_DUMMY_COL bit set if dummy column
**	psq_cb
**	  psq_error	filled if an error is encountered
**	
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	11-dec-91 (andre)
**	    plagiarized from coent: semantic actions
**	07-may-1992 (barbara)
**	    Extracted from 6.5 SQL grammar into this module.
**	9-Dec-2009 (kschendel) SIR 122974
**	    Add a couple checks for csv/ssv support.
**	2-Mar-2010 (kschendel) SIR 122974
**	    Tighten CSV check so that it's only allowed with BYTE, C, CHAR,
**	    D, and TEXT.  Prohibit CSV with anything else.
*/
DB_STATUS
psl_cp8_coent(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb,
	bool		parse_coent,
	char		*type_name,
	i4		len_prec_num,
	i4		*type_lenprec,
	char		*type_delim)
{
    ADF_CB		*adf_scb = (ADF_CB*) sess_cb->pss_adfcb;
    DB_DATA_VALUE	typelen;
    DB_STATUS		status;
    i4		err_code;
    char		*len_ptr;
    QEF_RCB		*qef_rcb;
    QEU_CPDOMD		*cpdom_desc;
    char		save_char;

    /*
    ** The distributed thread tests for potential dummy column and returns.
    ** The column name will not be mapped to the LDB col name for dummy columns.
    */
    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
    {
        if (parse_coent && !CMcmpcase(type_name, "d"))
	{
	    char	*ch = type_name;
	    CMnext(ch);
	    if (*ch == EOS || CMdigit(ch))
		sess_cb->pss_stmt_flags |= PSS_CP_DUMMY_COL;
	}

	return (E_DB_OK);
    }

    qef_rcb = (QEF_RCB *) sess_cb->pss_object;
    cpdom_desc = qef_rcb->qeu_copy->qeu_fdesc->cp_cur_fdd;

    if (parse_coent)
    {
	char		*ch;

	/*
	** This is a column type, possibly with a delimiter, possibly quoted.
	**     - i4
	**     - c0tab
	**     - date
	*/

	/* skip past column descriptor */
	for (ch = type_name; (*ch != EOS && !CMdigit(ch)); CMnext(ch))
	;

	len_ptr = ch;	/* length spec, if any, starts here */

	if (*ch)
	{
	    /*
	    ** length spec was specified; skip over it until hit EOS or what
	    ** looks like a beginning of a delimiter
	    */
	    for (CMnext(ch); (*ch != EOS && CMdigit(ch)); CMnext(ch))
	    ;
		
	    /*
	    ** now `ch' is pointing at the first character of the delimiter
	    ** (if any); if a delimiter was, indeed, specified, we save the
	    ** first char of the delimiter and replace it with EOS.  It will be
	    ** restored later (after we are done calling adi_encode_colspec()
	    */
	    if (*ch)
	    {
		save_char = *ch;
		*ch = EOS;
		type_delim = ch;
	    }
	    else
	    {
		type_delim = (char *) NULL;
	    }
	}
	else
	{
	    /* delimiter was not specified */
	    type_delim = (char *) NULL;
	}
    }

    /*
    ** Check for `dummy' type here; if not the dummy type then ask ADF
    ** to figure it out
    */
    if (parse_coent && !CMcmpcase(type_name, "d") && (len_ptr - type_name) == 1)
    {
	i4	    cp_length;

	if (*len_ptr)	    /* we treat d as equivalent to d0 */
	{
	    if (CVal(len_ptr, &cp_length) != OK)
		cp_length = -1;		    /* guaranteed bad length */

	    if (cp_length < 0 || cp_length > CPY_MAXDOMAIN)
	    {
		(VOID) psf_error(5804L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    STlength(cpdom_desc->cp_domname), cpdom_desc->cp_domname,
		    STlength(len_ptr), len_ptr);
		return (E_DB_ERROR);
	    }

	    typelen.db_length = cp_length;
	}
	else
	{
	    typelen.db_length = 0;
	}
	
	typelen.db_datatype 	= CPY_DUMMY_TYPE;
	sess_cb->pss_stmt_flags |= PSS_CP_DUMMY_COL;
	typelen.db_prec	    	= 0;
    }
    else
    {
	status = adi_encode_colspec(adf_scb, type_name, len_prec_num,
	    type_lenprec, ADI_F2_COPY_SPEC, &typelen);
	if (status != E_DB_OK)
	{
	    (VOID) psf_error(5833L, 0L, PSF_USERERR, &err_code,
		&psq_cb->psq_error, 1,
		STlength(cpdom_desc->cp_domname), cpdom_desc->cp_domname);

	    if (adf_scb->adf_errcb.ad_errclass == ADF_USER_ERROR)
		psf_adf_error(&adf_scb->adf_errcb, 
			      &psq_cb->psq_error, sess_cb);

	    return (E_DB_ERROR);
	}
    }

    /* if there is a delimiter, then process it */
    if (type_delim)
    {
	char		*delimiter;

	if (parse_coent)
	{
	    /*
	    ** one may specify both length and delimiter only when using
	    ** type(len[,prec])delim construct
	    */
	    if (typelen.db_length)
	    {
		(VOID) psf_error(5832L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 1,
		    STlength(cpdom_desc->cp_domname), cpdom_desc->cp_domname);
		return (E_DB_ERROR);
	    }

	    /*
	    ** first character in the delimiter has been saved in save_char and
	    ** replaced with EOS; before proceeeding, we must restore it
	    */
	    *type_delim = save_char;
	}

	status = psl_copydelim(type_delim, &delimiter);
	if (status != E_DB_OK)
	{
	    (VOID) psf_error(5831L, 0L, PSF_USERERR, &err_code,
		&psq_cb->psq_error, 2,
		STlength(cpdom_desc->cp_domname), cpdom_desc->cp_domname,
		STlength(type_delim), type_delim);
	    return (E_DB_ERROR);
	}
	/* FIXME??? CSV and SSV "delimiters" require support in the front-
	** end that is new in 10.0.  We could test for >= GCA protocol level
	** 68, and prohibit CSV/SSV otherwise.  On the other hand that would
	** prevent back-porting CSV/SSV to earlier clients as a patch.
	** For now, I'm going to let CSV/SSV through without complaint;
	** they will simply do nothing useful (ie they will almost certainly
	** error out) on pre-10.0 clients.  [kschendel Dec '09]
	*/

	/* CSV only works with BYTE, C, CHAR, D, TEXT. No counted types, no
	** unicode, no binary types.
	*/
	if ((*delimiter == CPY_CSV_DELIM || *delimiter == CPY_SSV_DELIM)
	  && (typelen.db_datatype != DB_BYTE_TYPE
		&& typelen.db_datatype != DB_CHR_TYPE
		&& typelen.db_datatype != DB_CHA_TYPE
		&& typelen.db_datatype != DB_TXT_TYPE
		&& typelen.db_datatype != CPY_DUMMY_TYPE) )
	{
	    (void) psf_error(E_US1947_6465_X_NOTWITH_Y, 0, PSF_USERERR,
			&err_code, &psq_cb->psq_error, 3,
			0, "COPY",
			0, type_delim,
			0, type_name);
	    return (E_DB_ERROR);
	}
	/* Disallow CSV and fixed length formats. */
	if ((*delimiter == CPY_CSV_DELIM || *delimiter == CPY_SSV_DELIM)
	  && typelen.db_length > 0)
	{
	    (void) psf_error(E_US1947_6465_X_NOTWITH_Y, 0, PSF_USERERR,
			&err_code, &psq_cb->psq_error, 3,
			0, "COPY",
			0, type_delim,
			0, "a fixed width domain");
	    return (E_DB_ERROR);
	}

	cpdom_desc->cp_user_delim = TRUE;
	CMcpychar(delimiter, cpdom_desc->cp_delimiter);
    }

    cpdom_desc->cp_type   = typelen.db_datatype;
    cpdom_desc->cp_length = typelen.db_length;
    cpdom_desc->cp_prec   = typelen.db_prec;

    return(E_DB_OK);
}

/*
** Name: psl_cp9_nm_eq_nm	- Semantic actions for copyoption production
**
** Description:
**	This routine performs the semantic actions for one of the 
**	copyoption (SQL and QUEL) productions.  These productions are
**	for parsing the WITH clause on the COPY statement.
**
**	copyoption:	NAME EQUAL NAME
**		|	NAME EQUAL CONTINUE	-- SQL only
**		|	ROLLBACK EQUAL NAME	-- QUEL only
**
** Inputs:
**	sess_cb			    ptr to a PSF session CB
**	    pss_distrib		    indicates distributed thread
**	name		NAME or keyword given on LHS
**	value		NAME or keyword given on RHS
**
** Outputs:
** 	err_blk		    will be filled in if an error occurs
**	sess_cb
**	    pss_object	    copy information for QEF
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	07-may-1992 (barbara)
**	    Taken from 6.5 SQL grammar.
*/
DB_STATUS
psl_cp9_nm_eq_nm(
	PSS_SESBLK	*sess_cb,
	char		*name,
	char		*value,
	DB_ERROR	*err_blk)
{
    QEF_RCB	*qef_rcb;
    QEU_COPY	*qe_copy;
    i4	err_code;

    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
	return (E_DB_OK);

    qef_rcb = (QEF_RCB *) sess_cb->pss_object;
    qe_copy = qef_rcb->qeu_copy;

    if (!CMcmpcase(name, "o") && !STcompare(name, "on_error"))
    {
	if (STcompare(value, "terminate") == 0)	
	{
	    if (qe_copy->qeu_stat & CPY_CONTINUE)
	    {
		(VOID) psf_error(5853L, 0L, PSF_USERERR, &err_code,
		    err_blk, 0);
		return (E_DB_ERROR);
	    }
	}
	else if (STcompare(value, "continue") == 0)
	{
	    if (qe_copy->qeu_maxerrs)
	    {
		(VOID) psf_error(5853L, 0L, PSF_USERERR, &err_code,
		    err_blk, 0);
		return (E_DB_ERROR);
	    }

	    qe_copy->qeu_stat |= CPY_CONTINUE;
	}
	else
	{
	    (VOID) psf_error(5852L, 0L, PSF_USERERR, &err_code,
		err_blk, 1, STlength(name), name);
	    return (E_DB_ERROR);
	}
    }

    else if (!CMcmpcase(name, "r") && !STcompare(name, "rollback"))
    {
	if (STcompare(value, "disabled") == 0)
	{
	    qe_copy->qeu_stat |= CPY_NOABORT;
	}
	else if (STcompare(value, "enabled") == 0)
	{
	    if (qe_copy->qeu_stat & CPY_NOABORT)
	    {
	    	(VOID) psf_error(5853L, 0L, PSF_USERERR, &err_code,
		    err_blk, 0);
		return (E_DB_ERROR);
	    }
	}
	else
	{
	    (VOID) psf_error(5852L, 0L, PSF_USERERR, &err_code,
		err_blk, 1, STlength(value), value);
	    return (E_DB_ERROR);
	}
    }

    else if (	(!CMcmpcase(name, "f") && !STcompare(name, "function"))
	     ||	(!CMcmpcase(name, "c") && !STcompare(name, "copyhandler"))
	    )
    {
	/* Ignore the FUNCTION = USER_FUNCTION clause */
	/* Ignore the COPYHANDLER = name clause */
    }
    else
    {
	(VOID) psf_error(5851L, 0L, PSF_USERERR, &err_code,
	    err_blk, 1, STlength(name), name);
    }

    return (E_DB_OK);
}


/*
** Name: psl_cp10_nm_eq_str	- Semantic actions for copyoption production
**
** Description:
**	This routine performs the semantic actions for one of the 
**	copyoption (SQL and QUEL) productions.  These productions are
**	for parsing the WITH clause on the COPY statement.
**
**	copyoption:	NAME EQUAL SCONST
**
** Inputs:
**	sess_cb			    ptr to a PSF session CB
**	    pss_distrib		    indicates distributed thread
**	name		NAME or keyword given on LHS
**	value		contents of string given on RHS
**
** Outputs:
** 	err_blk		    will be filled in if an error occurs
**	sess_cb
**	    pss_object	    copy information for QEF
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	07-may-1992 (barbara)
**	    Taken from 6.5 SQL grammar.
*/
DB_STATUS
psl_cp10_nm_eq_str(
	PSS_SESBLK	*sess_cb,
	char		*name,
	char		*value,
	DB_ERROR	*err_blk)
{
    QEF_RCB	*qef_rcb;
    QEU_COPY	*qe_copy;
    i4	err_code;
    DB_STATUS	status;

    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
	return (E_DB_OK);

    qef_rcb = (QEF_RCB *) sess_cb->pss_object;
    qe_copy = qef_rcb->qeu_copy;

    if (STcompare(name, "log") !=0)
    {
	(VOID) psf_error(5851L, 0L, PSF_USERERR, &err_code,
	    err_blk, 1, STlength(name), name);
	return (E_DB_ERROR);
    }

    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, STlength(value) + 1,
	(PTR *) &qe_copy->qeu_logname, err_blk);
    if (status != E_DB_OK)
	return (status);

    STcopy(value, qe_copy->qeu_logname);

    return (E_DB_OK);
}


/*
** Name: psl_cp11_nm_eq_no	- Semantic actions for copyoption production
**
** Description:
**	This routine performs the semantic actions for one of the 
**	copyoption (SQL and QUEL) productions.  The copyoption productions
**	are for parsing the WITH clause on the COPY statement.
**
**	copyoption:	NAME EQUAL int2_int4_p
**
** Inputs:
**	sess_cb			    ptr to a PSF session CB
**	    pss_distrib		    indicates distributed thread
**	name		NAME or keyword given on LHS
**	value		integer value on RHS
**
** Outputs:
** 	err_blk		    will be filled in if an error occurs
**	sess_cb
**	    pss_object	    copy information for QEF
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	07-may-1992 (barbara)
**	    Taken from 6.5 SQL grammar.
**	30-mar-1993 (rmuth)
**	    Add support for the "WITH ROW_ESTIMATE = n" clause.
**	26-July-1993 (rmuth)
**	   Enhanced the COPY to accept a range of WITH options. These
**	   are passed onto to DMF through the  characteristics array in
**	   the DMR_CB. The new with clauses are are already dealt with
**	   in psl_nm_eq_no, so call that routine from here.
*/
DB_STATUS
psl_cp11_nm_eq_no(
	PSS_SESBLK	*sess_cb,
	char		*name,
	i4		value,
	PSS_WITH_CLAUSE *with_clauses,
	DB_ERROR	*err_blk)
{
    QEF_RCB	*qef_rcb;
    QEU_COPY	*qe_copy;
    i4	err_code;
    bool	error_count  = FALSE;
    bool	row_estimate = FALSE;
    DB_STATUS	status;

    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
	return (E_DB_OK);

    qef_rcb = (QEF_RCB *) sess_cb->pss_object;
    qe_copy = qef_rcb->qeu_copy;

    if (STcompare(name, "error_count") == 0)
    {
	error_count = TRUE;
    }
    else
    if (STcompare(name, "row_estimate") == 0)
    {
        row_estimate = TRUE;
    }
    else
    {
	status = psl_nm_eq_no( sess_cb,
			       name,
			       value,
			       with_clauses,
			       PSQ_COPY,
			       err_blk,
			       (PSS_Q_XLATE *) NULL);
	return( status );
    }

    /*
    ** If we reached here it is either row_estimate or error_count
    */
    if (value < 0)
    {
	char		numbuf[25];
	
	(VOID) CVla((i4) value, numbuf);
	(VOID) psf_error(5852L, 0L, PSF_USERERR, &err_code,
	    err_blk, 1, STlength(numbuf), numbuf);
	return (E_DB_ERROR);
    }

    if (qe_copy->qeu_stat & CPY_CONTINUE)
    {
	(VOID) psf_error(5853L, 0L, PSF_USERERR, &err_code, err_blk, 0);
	return (E_DB_ERROR);
    }

    if (error_count)
        qe_copy->qeu_maxerrs = value;
    else
	if (row_estimate)
	    qe_copy->qeu_count = value;

    return (E_DB_OK);
}


/*
** Name: psl_cp12_nm_eq_qdata	- Semantic actions for copyoption production
**
** Description:
**	This routine performs the semantic actions for one of the 
**	copyoption (SQL and QUEL) productions.  The copyoption productions
**	are for parsing the WITH clause on the COPY statement.
**
**	copyoption:	NAME EQUAL QDATA
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	    pss_distrib	    indicates distributed thread
**	psq_cb		    PSF request CB
**	name		    NAME or keyword given on LHS
**	value		    QDATA given on RHS
**
** Outputs:
**	sess_cb
**	    pss_object	    copy information for QEF
**	psq_cb		    PSF request CB
** 	    err_blk	    will be filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	07-may-1992 (barbara)
**	    Taken from 6.5 SQL grammar.
**	26-may-93 (andre)
**	    if the "value" is expected to contain a string verify that it does
**
**	    When calling qdata_cvt(), ensure that the output datatype agrees
**	    with input datatype as regards nullability
**	13-May-2009 (kschendel) b122041
**	    Compiler caught improper assignment to qeu-maxerrs. 
*/
DB_STATUS
psl_cp12_nm_eq_qdata(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb,
	char		*name,
	DB_DATA_VALUE	*value,
	DB_ERROR	*err_blk)
{
    QEF_RCB	*qef_rcb;
    QEU_COPY	*qe_copy;
    i4	err_code;

    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
	return (E_DB_OK);

    qef_rcb = (QEF_RCB *) sess_cb->pss_object;
    qe_copy = qef_rcb->qeu_copy;

    if (!STcompare(name, "log"))
    {
	DB_STATUS	status;
	DB_TEXT_STRING	*str;
	DB_DT_ID	totype;

	/*
	** before we call qdata_cvt() to convert input value into longtext, make
	** sure that the input value is of "character type", i.e. c, char, text,
	** varchar, or longtext
	*/

	status = psl_must_be_string(sess_cb, value, &psq_cb->psq_error);
	if (DB_FAILURE_MACRO(status))
	    return(status);

	/*
	** if the input datatype was nullable, so must be the output datatype
	*/

	totype = (value->db_datatype < 0) ? -DB_LTXT_TYPE : DB_LTXT_TYPE;

	status = qdata_cvt(sess_cb, psq_cb, value, totype, (PTR *) &str);

	if (DB_FAILURE_MACRO(status))
	    return (status);

	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, str->db_t_count + 1,
		(PTR *) &qe_copy->qeu_logname, &psq_cb->psq_error);

	if (DB_FAILURE_MACRO(status))
	    return (status);

	/* copy log file name */
	MEcopy((PTR) str->db_t_text, str->db_t_count,
	    (PTR) qe_copy->qeu_logname);

	/* and NULL terminate */
	*(qe_copy->qeu_logname + str->db_t_count) = EOS;
    }

    else if (!STcompare(name, "error_count"))
    {
	i4		i4_value;
	ADF_CB		adf_cb;
	ADF_CB		*adf_scb = (ADF_CB*) sess_cb->pss_adfcb;
	DB_DATA_VALUE	tdv;
	ADI_DT_NAME	dt_fname;
	ADI_DT_NAME	dt_tname;
	char		numbuf[25];

	/* Copy the session ADF block into local one */
	STRUCT_ASSIGN_MACRO(*adf_scb, adf_cb);

	adf_cb.adf_errcb.ad_ebuflen = 0;
	adf_cb.adf_errcb.ad_errmsgp = 0;
	tdv.db_length		    = 4;
	tdv.db_prec		    = 0;
	tdv.db_datatype		    = DB_INT_TYPE;
	tdv.db_data		    = (PTR) &i4_value;

	if (adc_cvinto(&adf_cb, value, &tdv) != E_DB_OK)
	{
	    (VOID) adi_tyname(&adf_cb, value->db_datatype, &dt_fname);
	    (VOID) adi_tyname(&adf_cb, (DB_DT_ID) DB_INT_TYPE, &dt_tname);
	    (VOID) psf_error(2911L, 0L, PSF_USERERR,
		    &err_code, &psq_cb->psq_error, 3,
		    sizeof (sess_cb->pss_lineno), &sess_cb->pss_lineno,
		    psf_trmwhite(sizeof (dt_fname), (char *) &dt_fname), 
		    &dt_fname,
		    psf_trmwhite(sizeof (dt_tname), (char *) &dt_tname), 
		    &dt_tname);
	    return (E_DB_ERROR);
	}

	if (i4_value < 0)
	{
	    (VOID) CVla(i4_value, numbuf);
	    (VOID) psf_error(5852L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 1, STtrmwhite(numbuf), numbuf);
	    return (E_DB_ERROR);
	}

	if (qe_copy->qeu_stat & CPY_CONTINUE)
	{
	    (VOID) psf_error(5853L, 0L, PSF_USERERR, &err_code,
		&psq_cb->psq_error, 0);
	    return (E_DB_ERROR);
	}

	qe_copy->qeu_maxerrs = i4_value;
    }

    else
    {
	(VOID) psf_error(5851L, 0L, PSF_USERERR, &err_code,
	    &psq_cb->psq_error, 1, STtrmwhite(name), name);
	return (E_DB_ERROR);
    }

    return (E_DB_OK);
}


/*
** Name: psl_cp13_nm_single	- Semantic actions for copyoption production
**
** Description:
**	This routine performs the semantic actions for one of the 
**	copyoption (SQL and QUEL) productions.  These productions are
**	for parsing the WITH clause on the COPY statement.
**
**	copyoption:	NAME 
**
** Inputs:
**	sess_cb			    ptr to a PSF session CB
**	    pss_distrib		    indicates distributed thread
**	name		NAME or keyword given on LHS
**
** Outputs:
** 	err_blk		    will be filled in if an error occurs
**	sess_cb
**	    pss_object	    copy information for QEF
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	07-may-1992 (barbara)
**	    Taken from 6.5 SQL grammar.
**	26-July-1993 (rmuth)
**	    Remove [NO]EMPTYTABLE.
**	10-aug-93 (andre)
**	    removed extraneous return()
*/
DB_STATUS
psl_cp13_nm_single(
	PSS_SESBLK	*sess_cb,
	char		*name,
	DB_ERROR	*err_blk)
{
    i4	err_code;
    QEF_RCB	*qef_rcb;
    QEU_COPY	*qe_copy;

    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
	return (E_DB_OK);

    qef_rcb = (QEF_RCB *) sess_cb->pss_object;
    qe_copy = qef_rcb->qeu_copy;
    
    _VOID_ psf_error(5851L, 0L, PSF_USERERR, &err_code,
	    err_blk, 1, STlength(name), name);
    return (E_DB_ERROR);
}
