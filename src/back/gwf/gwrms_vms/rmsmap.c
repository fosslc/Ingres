/*
** Copyright (c) 1989, 2005 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <cs.h>
#include    <er.h>
#include    <ex.h>
#include    <descrip.h>
#include    <scf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <gwf.h>
#include    <gwfint.h>
#include    <rms.h>
#include    <add.h>
#include    "gwrms.h"
#include    "gwrmsdt.h"
#include    "gwrmserr.h"

/**
**
**  Name: RMSMAP.C - Convert data to/from rms format
**
**  Description:
**	This module contains the functions used to convert data from rms format
**	to ingres format and vice versa. The approach used is to build an ADF
**	compiled expression (CX) for each table that converts the data in the
**	RMS record to the equivalent INGRES record. A second CX is used to
**	convert from INGRES format to RMS format for update operations and a
**	third for converting INGRES index keys to RMS index keys. These
**	expressions are compiled at the time the table is opened by the rms
**	gateway.
**
**	The conversion is performed at record access time by having the
**	position, get, update, insert functions execute the appropriate CX.
**
**	A future extension will include in the CX a qualifying expression to
**	determine if a record in the RMS file is part of this "table". This
**	will provide a simple mechanism for supporting variant record
**	structures. THIS IS NOT IMPLEMENTED AT THIS TIME!
**
**	NOTE: AT THE PRESENT TIME ALL CONVERSIONS ARE DONE INTERPRETIVELY.
**	A PERFORMANCE ENHANCEMENT WOULD BE TO ADD THE COMPILED EXPRESSIONS
**	DESCRIBED ABOVE.
**	
**	    rmscx_to_ingres	- CX for converting INGRES row to RMS record.
**	    rmscx_from_ingres	- CX for converting RMS record to INGRES row.
**	    rmscx_index_format	- CX for converting INGRES key to RMS key.
**	    rmscx_tcb_estimate	- Estimated size of all CX's.
**	    rms_from_ingres	- Convert from INGRES format to RMS format
**          rms_to_ingres	- Convert from RMS format to INGRES format
**	    rms_index_format	- Convert INGRES key vlue to RMS key value
**	    rms_cvt_handler	- handle exceptions raised during conversion.
**	    rms_check_adferr	- handle an error returned during conversion.
**
**  History:
**      26-dec-89 (paul)
**	    First written for RMS support.
**	17-apr-90 (bryanp)
**	    Added error-logging support and returns of error codes to
**	    callers.
**	20-apr-90 (bryanp)
**	    Added new parameters and new code to rms_to_ingres in an attempt to
**	    deal with record length in a rational way. The previous code had
**	    assumed that (1) the RMS record length, (2) the sum of all the RMS
**	    column sizes in the REGISTER TABLE command, and (3) the sum of all
**	    the Ingres column sizes in the REGISTER TABLE command were the
**	    same. Instead, we now allow rms_to_ingres to calculate the Ingres
**	    record length during conversion and to handle the possibility that
**	    the RMS record length does not equal the sum of the RMS column
**	    sizes.
**	7-jun-1990 (bryanp)
**	    Handle exceptions raised by adf during conversion of data to/from
**	    RMS formats. At this time, this is just a primitive handling of
**	    the exceptions; we expect that in the future we will add a much
**	    more sophisticated control over the exceptions. Added the new
**	    routine rms_cvt_handler().
**	8-jun-1990 (bryanp)
**	    Handle non-OK return codes from ADF during conversion of data.
**	    Check ADF's internal control blocks (ugh!) to see what error
**	    recovery is indicated. Isolated this code into rms_check_adferr().
**	11-jun-1990 (bryanp)
**	    The on-going saga of RMS conversion errors: attempt to distinguish
**	    conversion errors from other errors; report conversion error to
**	    front-end via scc_error(); return special conversion error code
**	    to caller as known "user" error (do not log traceback of calls, just
**	    pass conversion error code up to DMF).
**      18-may-92 (schang)
**          GW merge.
**	    4-feb-91 (linda)
**	        Code cleanup; change parameters to rms_to_ingres() routine; add
**	        nullpad/blankpad and (trailing) varying string handling; add
**	        base_to_index() routine to construct a secondary index tuple
**              out of translated base table tuple.
**	    22-jul-91 (rickh)
**	        Retired base_to_index.  Rolled its useful code into
**	        rms_to_ingres.
**      06-nov-92 (schang)
**          add dmrcb before dmtcb  
**      17-dec-1992 (schang)
**          prototype
**      03-jun-1993 (schang)
**          fix b52255, conversion errors, other than overflow and underflow,
**          returns E_DB_ERROR
**      07-jun-1993 (schang)
**          add read-only repeating group support to RMS GW
**      24-aug-93 (ed)
**          added missing includes
**      13-jun-96 (schang)
**          integrate 64 changes
**      03-mar-1999 (chash01)
**          integrate all changes since ingres 6.4
**	09-nov-1999 (kinte01)
**	   Rename EXCONTINUE to EXCONTINUES per AIX 4.3 porting change
**      24-mar-2000 (chash01)
**          separate integer overflow count and float overflow count so
**          that front_end will generate correct message (B101026.)
**	28-feb-2001	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from RMS GW code as the use is no longer allowed
**	24-feb-2005 (abbjo03)
**	    Remove include of obsolete gwxit.h.
**/

/*
** Definition of static variables and forward static functions.
*/

static	STATUS	rms_cvt_handler();	/* RMS conversion exception handler. */
static	STATUS	rms_check_adferr();	/* Error handling for ADF calls      */

/*{
** Name: rmscx_to_ingres	- Build CX for Converting RMS record to INGRES
**
** Description:
**	rmscx_to_ingres builds an ADF expression that will convert a given RMS
**	record to an INGRES record. This procedures takes a description of the
**	rms record format and the target INGRES record format and produces an
**	ADF compiled expression (CX) that will build the specified INGRES
**	record from the RMS record. Given a matching RMS record, this function
**	is then invoked via the rms_to_ingres() function.
**
** Inputs:
**	tcb		    Gateway TCB
**
** Outputs:
**	tcb		    Will contain the CX computed.
**	error->err_code	    Detailed error code, if an error occurred:
**			    E_GW5480_RMSCX_TO_INGRES_ERROR
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	    
**	Exceptions:
**
** Side Effects:
**
** History:
**	26-dec-89 (paul)
**	    Written for RMS gateway.
**	17-apr-90 (bryanp)
**	    Added error parameter, and added error handling & logging.
*/
DB_STATUS
rmscx_to_ingres
(
    GW_TCB    *tcb,
    DB_ERROR  *error
)
{
    DB_STATUS	    status;
    ADF_CB	    adf_cb;
    i4		    count = 0;
    i4		    ade_instr_cnt;
    i4		    ade_operand_count;
    i4		    ade_const_count;
    i4		    ade_const_size;
    i4		    ade_cx_size;
    i4	    	    ulecode;
    i4	    	    msglen;
        
    /*
    ** Compile the ADF CX needed to convert rms_xatt values into the
    ** corresponding ing_att values. There is a one to one correspondence
    ** between entries in rms_xatt and ing_att.
    **
    ** First initialize the CX compiler.  The main effort is to compute the
    ** approximate size of the CX.  We will then allocate a buffer of that size
    ** to contain the CX.
    */

    ade_instr_cnt = count;
    ade_operand_count = 2 * ade_instr_cnt;
    ade_const_count = 0;
    ade_const_size = 0;
    if (status = ade_cx_space(&adf_cb, ade_instr_cnt, ade_operand_count,
	ade_const_count, ade_const_size, &ade_cx_size))
    {
	/*
	** Failed to compute ade expression size. Report the error.
	*/
	_VOID_ ule_format( adf_cb.adf_errcb.ad_errcode, (CL_ERR_DESC *)0, 
			    ULE_LOG, (DB_SQLSTATE *)NULL, (char *)0, (i4)0,
			    &msglen, &ulecode, 0 );
	_VOID_ ule_format( E_GW0340_ADE_CX_SPACE_ERROR, (CL_ERR_DESC *)0, 
			    ULE_LOG, (DB_SQLSTATE *)NULL, (char *)0, (i4)0,
			    &msglen, &ulecode, 0 );
	error->err_code = E_GW5480_RMSCX_TO_INGRES_ERROR;
	status = E_DB_ERROR;
    }

    return ( status );
}

/*
** Name: rmscx_from_ingres  - stub for future use.
**
** History:
**	17-apr-90 (bryanp)
**	    Added error parameter, in case we need it later.
*/
DB_STATUS
rmscx_from_ingres
(
    GW_TCB    *tcb,
    DB_ERROR  *error
)
{
    return (E_DB_OK);
}

/*
** Name: rmscx_index_format - stub for future use
**
** History:
**	17-apr-90 (bryanp)
**	    Added error parameter, in case we need it later.
*/
DB_STATUS
rmscx_index_format
(
    GW_TCB    *tcb,
    DB_ERROR  *error
)
{
    return (E_DB_OK);
}

/*{
** Name: rmscx_tcb_estimate	- Estimate memory for GWRMS_TCB
**
** Description:
**	Estimate the amount of memory that should be allocated for the RMS
**	specific TCB and any associated control structures.  Since this memory
**	is allocated once by the caller and SCF memory is allocated in large
**	granules, it makes sense to do one allocation.  In order to support a
**	single allocation, we must estimate the amount of memory that we will
**	need for the CX's for converting RMS to INGRES data and vice versa.
**	Since we only get one chance at allocation, we will have to be
**	extremely pessimistic.
**
** Inputs:
**	tcb	    - Generic TCB for the table for which we are estimating
**
** Outputs:
**	error->err_code	    - Detailed error code if an error occurred:
**			      E_GW5481_RMSCX_TCB_EST_ERROR
**
**	Returns:
**	    number of bytes required for CX's
**	    if an error occurs in ADF, returns 0
**	Exceptions:
**
** Side Effects:
**
** History:
**	23-dec-89 (paul)
**	    First written
**	17-apr-90 (bryanp)
**	    Added error parameter, and invented return (0) to mean "error
**	    occurred". Added error-logging and returning.
*/
i4
rmscx_tcb_estimate
(
    GW_TCB    *tcb,
    DB_ERROR  *error
)
{
    DB_STATUS	    status;
    ADF_CB	    adf_cb;
    i4		    count;
    i4		    ade_instr_cnt;
    i4		    ade_operand_count;
    i4		    ade_const_count;
    i4		    ade_const_size;
    i4	    	    ade_cx_size;
    i4	    	    ulecode;
    i4	    	    msglen;
            
    /*
    ** Compile the ADF CX needed to convert rms_xatt values into the
    ** corresponding ing_att values. There is a one to one correspondence
    ** between entries in rms_xatt and ing_att.
    **
    ** First initialize the CX compiler.  The main effort is to compute the
    ** approximate size of the CX.  We will then allocate a buffer of that size
    ** to contain the CX.  In order to make sure we don't underestimate the
    ** size of this CX, we 4X the number of operations expected in case we have
    ** to align every operand.
    */

    count = tcb->gwt_table.tbl_attr_count;
    ade_instr_cnt = count * 5;
    ade_operand_count = 2 * ade_instr_cnt;
    ade_const_count = 0;
    ade_const_size = 0;
    if (status = ade_cx_space(&adf_cb, ade_instr_cnt, ade_operand_count,
	ade_const_count, ade_const_size, &ade_cx_size))
    {
	/*
	** Failed to compute ade expression size. Report the error.
	*/
	_VOID_ ule_format( adf_cb.adf_errcb.ad_errcode, (CL_ERR_DESC *)0, 
			    ULE_LOG, (DB_SQLSTATE *)NULL, (char *)0, (i4)0,
			    &msglen, &ulecode, 0 );
	_VOID_ ule_format( E_GW0340_ADE_CX_SPACE_ERROR, (CL_ERR_DESC *)0, 
			    ULE_LOG, (DB_SQLSTATE *)NULL, (char *)0, (i4)0,
			    &msglen, &ulecode, 0 );
	error->err_code = E_GW5481_RMSCX_TCB_EST_ERROR;

	/*
	** Tell caller that there was an error:
	*/
	ade_cx_size = 0;
    }
    else
    {
	/* We are going to need two of these CX's (input and output) */
	ade_cx_size = ade_cx_size * 2;
    }

    return (ade_cx_size);
}

/*{
** Name: rms_to_ingres	- Convert an RMS record to ingres format
**
** Description:
**	This procedure will convert an rms record to an ingres record by
**	interpreting the descriptions of the two records from the system
**	catalogs. For each column, an ADF function is called to convert from
**	the rms declared type to the ingres declared type.
**
**	We make the assumption that the two types are coercible. This should
**	have been checked during registration.
**
**	We attempt to allow for the possibility that all the different buffers
**	and columns being manipulated have different sizes. We conduct sanity
**	checks to see that we don't reference storage outside of the buffer, we
**	truncate columns, if necessary, to fit into the buffer, and we
**	calculate the resulting record length based on the results of the
**	conversion. NOTE: the output Ingres columns should NEVER overflow the
**	Ingres buffer. It is the RMS columns and the RMS buffer which are
**	suspect.
**
**	The error handling code has been through several iterations and is
**	still not flexible enough to suit us.
**
** Inputs:
**	tcb
**	rsb
**	rms_buffer
**	rms_buf_len	    - actual length of the rms buffer, from sys$get
**	ingres_buffer
**	ing_buf_len	    - actual length of the Ingres buffer
**
** Outputs:
**	ingres_buffer	    contains newly converted record.
**	ing_record_length   actual length of ingres record, after conversion.
**	error->err_code	    Detailed error code if an error occurred:
**			    E_GW5482_RMS_TO_INGRES_ERROR
**			    E_GW5484_RMS_DATA_CVT_ERROR
**			    <really probably should have a 'truncated' error>
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**
** Side Effects:
**
** History:
**	26-dec-89 (paul)
**	    Written.
**	17-apr-90 (bryanp)
**	    Added error parameter and error handling.  Added casts on
**	    'rms_buffer' and 'ingres_buffer' to conform to coding conventions
**	    for use of PTR type.
**	20-apr-90 (bryanp)
**	    Added new parameters and new code to rms_to_ingres in an attempt to
**	    deal with record length in a rational way. The previous code had
**	    assumed that (1) the RMS record length, (2) the sum of all the RMS
**	    column sizes in the REGISTER TABLE command, and (3) the sum of all
**	    the Ingres column sizes in the REGISTER TABLE command were the
**	    same. Instead, we now allow rms_to_ingres to calculate the Ingres
**	    record length during conversion and to handle the possibility that
**	    the RMS record length does not equal the sum of the RMS column
**	    sizes.
**	7-jun-1990 (bryanp)
**	    Arm rms_cvt_handler() as our exception handler during datatype
**	    conversion processing, as a first attempt at recovering from
**	    exceptions during conversions.
**	8-jun-1990 (bryanp)
**	    Call rms_check_adferr() when adf returns a non-OK return code.
**	    Check if ADF has already formatted error message upon return.
**	11-jun-1990 (bryanp)
**	    Distinguish conversion errors from fatal internal errors.
**	18-jun-90 (linda, bryanp)
**	    Add new error code, for the case where user registration has
**	    indicated a field that is beyond the actual RMS record length.
**	    Code is E_GWxxxx_RMS_RECORD_TOO_SHORT.
**	22-jul-1991 (rickh)
**	    Ripped out base_to_index.  Its work is now done here.
**	    Secondary index tuples are no longer hung by the chimney with
**	    care in hopes that St. Nicolaus soon would be there.
**      05-may-1993 (schang)
**          For repeating group support, this routine should
**            1.  determined if retrieved record has repeating group,
**            2.  stuff sequence no into ingres data, if has repeating group
**            3.  set up 'current' sequence no. in scb
**            4.  calculate the size of repeating member and keep it in scb
**            5.  if current repeating member has sequence no of >1, move it
**                to the position of first repeating member for conversion.
**            6.  if sequence no is greater than count, delete everything 
**                related to repeating group.
**      13-jun-96 (schang)
**          integrate changes from 64
*/
rms_to_ingres
(
    GWX_RCB	    *gwx_rcb,
    i4	    *ing_record_length,
    DB_ERROR        *error,
    PTR             *tid_loc
)
{
    GW_RSB	    *rsb	    = (GW_RSB *)gwx_rcb->xrcb_rsb;
    GW_TCB	    *tcb	    = rsb->gwrsb_tcb;
    GWRMS_RSB	    *rms_rsb	    = (GWRMS_RSB *)rsb->gwrsb_internal_rsb;
    PTR		    rms_buffer	    = rms_rsb->rms_buffer;
    i4	    rms_buf_len	    = rms_rsb->rab.rab$w_rsz;
    PTR		    ingres_buffer   = gwx_rcb->xrcb_var_data1.data_address;
    i4	    ing_buf_len	    = gwx_rcb->xrcb_var_data1.data_in_size;
    DB_TAB_ID	    *db_tab_id	    = gwx_rcb->xrcb_tab_id;
    DB_STATUS	    status	    = E_DB_OK;
    ADF_CB	    *adfcb;
    GW_ATT_ENTRY    *gw_attr;
    GW_ATT_ENTRY    *first_gw_attr;
    GW_IDX_ENTRY    *idx	    = tcb->gwt_idx;
    DMT_ATT_ENTRY   *ing_attr;
    GWRMS_XATT	    *rms_attr;
    DB_DATA_VALUE   ing_value;
    DB_DATA_VALUE   rms_value;
    i4		    column_count;
    i4		    i;
    i4		    j;
    i4		    ulecode;
    i4		    msglen;
    EX_CONTEXT	    context;
    i4		    bas_rec_length;
    i4		    isSecondaryIndex;
    char	    *idx_offset;
    bool            get_imagine = FALSE,
                    rptg_count =  FALSE;
    i4              rptg_size = 0;
    i4	            tmp_offset = 0;
        
    if (db_tab_id->db_tab_index)
	isSecondaryIndex = 1;
    else
	isSecondaryIndex = 0;

    adfcb = rsb->gwrsb_session->gws_adf_cb;
    
    /*
    ** Plant an exception handler so that if any lower-level ADF routine
    ** aborts, we can attempt to trap the error and "do the right thing".
    */
    if (EXdeclare( rms_cvt_handler, &context) != OK)
    {
	/*
	** We get here if an exception occurs and the handler longjumps
	** back to us (returns EXDECLARE). In this case, ADF has asked us
	** to abort the query, so we return with an 'error converting record'
	** error. We can distinguish between "conversion error" and "fatal
	** ADF internal error" by checking the ADF control block. This is
	** important because our caller must be able to distinguish these
	** errors.
	*/
	EXdelete();
	adfcb = rsb->gwrsb_session->gws_adf_cb;
	if (adfcb->adf_errcb.ad_errclass == ADF_USER_ERROR)
	    error->err_code = E_GW5484_RMS_DATA_CVT_ERROR;
	else
	    error->err_code = E_GW5482_RMS_TO_INGRES_ERROR;
	return (E_DB_ERROR);
    }
    /*
    ** 03-mar-1994 (schang)
    **     In order to support secondary indices on repeating group table,
    **     it is necessary to determine if the underlying base table of
    **     an index table is a repeating table after a record is obtained
    **     from RMS file.  We do so if the repeating group indicator says 
    **     it is unknown at this time.
    */
    if ( isSecondaryIndex && (rms_rsb->rptg_is_rptg == GWRMS_REC_RPTG_NULL))
    {
	column_count = tcb->gwt_table.tbl_attr_count;
        first_gw_attr = tcb->gwt_attr;
        for (i = 0; i < column_count; i++ )
        {
	    gw_attr = first_gw_attr + i;
	    ing_attr = &gw_attr->gwat_attr;
            rms_attr = gw_attr->gwat_xattr.data_address;	
	    rms_value.db_datatype = rms_attr->gtype;
            if (rms_value.db_datatype == RMSGW_IMAGINE)
                rms_rsb->rptg_is_rptg = GWRMS_REC_IS_RPTG;
            else
            {
                if (rms_rsb->rptg_is_rptg == GWRMS_REC_IS_RPTG)
                {
                    /*
                    ** we must get the count field out for this
                    ** repeating group. We use a temp variable 
                    ** for ing_value.db_data so as not to disturb
                    ** the normal ingres data buffer.
                    */
                    i4 temp;

                    rms_value.db_prec = rms_attr->gprec_scale;
                    rms_value.db_length = rms_attr->glength;
	            rms_value.db_data = rms_attr->goffset + 
                                          (char *)rms_buffer;
                    ing_value.db_datatype = ing_attr->att_type;
                    ing_value.db_prec = ing_attr->att_prec;
                    ing_value.db_length = ing_attr->att_width;
                    ing_value.db_data = &temp;
                    if ((status = adc_cvinto(adfcb, &rms_value, &ing_value))
                         != E_DB_OK)
                    {
                        if (adfcb->adf_errcb.ad_emsglen > 0)
                            _VOID_ ule_format( (i4)0, (CL_ERR_DESC *)0,
			        ULE_MESSAGE, (DB_SQLSTATE *)NULL,
			        adfcb->adf_errcb.ad_errmsgp,
			        adfcb->adf_errcb.ad_emsglen,
			        &msglen, &ulecode, 0 );
	                else
		            _VOID_ ule_format( adfcb->adf_errcb.ad_errcode,
			        (CL_ERR_DESC *)0, ULE_LOG, 
                                (DB_SQLSTATE *)NULL, (char *)0,
                                (i4)0,  &msglen, &ulecode, 0 );

		/*
		** "An error occurred converting an RMS record to Ingres
		** format. The column which could not be converted was column
		** %0d, with Ingres datatype %1d, Ingres precision %2d, Ingres
		** length %3d, Ingres record offset %4d. The RMS specification
		** was RMS datatype %5d, RMS precision %6d, RMS length %7d, and
		** RMS record offset %8d."
		*/
	                _VOID_ ule_format(E_GW500D_CVT_TO_ING_ERROR,
                                 (CL_ERR_DESC *)0, ULE_LOG, (DB_SQLSTATE *)NULL,
                                 (char *)0, (i4)0,&msglen, &ulecode, 9,
			         sizeof(i), &i, sizeof(ing_attr->att_type),
                                 &ing_attr->att_type, 
                                 sizeof(ing_attr->att_prec),
                                 &ing_attr->att_prec,
                                 sizeof(ing_attr->att_width),
                                 &ing_attr->att_width,
                                 sizeof(ing_attr->att_offset),
                                 &ing_attr->att_offset,
                                 sizeof(rms_attr->gtype), 
                                 &rms_attr->gtype,
                                 sizeof(rms_attr->gprec_scale),
                                 &rms_attr->gprec_scale,
                                 sizeof(rms_attr->glength),
                                 &rms_attr->glength,
                                 sizeof(rms_attr->goffset),
                                 &rms_attr->goffset );
			
	                if ((status = rms_check_adferr( adfcb )) != E_DB_OK)
	                {
		           if (adfcb->adf_errcb.ad_errclass == ADF_USER_ERROR)
		               error->err_code = E_GW5484_RMS_DATA_CVT_ERROR;
		           else
		               error->err_code = E_GW5482_RMS_TO_INGRES_ERROR;
		           break;
	                }
                        else 
		           error->err_code = E_GW5484_RMS_DATA_CVT_ERROR;
                        EXdelete();
                        return(E_DB_ERROR);
	            }
                    else
                    {
                        switch (ing_value.db_length)
                        {
                          case 1 :
                            rms_rsb->rptg_count = *(i1 *)ing_value.db_data;
                            break;
                          case 2 :
                            rms_rsb->rptg_count = *(i2 *)ing_value.db_data;
                            break;
                          case 4 :
                            rms_rsb->rptg_count = *(i4 *)ing_value.db_data;
                            break;
                          default : /* some error here and return? */
                            break;
                        }
                        /*
                        ** zero rptg_count means no more data, we zero out
                        ** major rms_rsb field for later use
                        */
                        if (rms_rsb->rptg_count == 0)
                        {
                            EXdelete();
                            rms_rsb->rptg_is_rptg = 0;
                            rms_rsb->rptg_seq_no = 0;
	                    error->err_code = E_GW0342_RPTG_OUT_OF_DATA;
                            return(E_DB_ERROR);
                        }
                        break;   
                    }
                }
            }
        }
        if (rms_rsb->rptg_is_rptg == GWRMS_REC_RPTG_NULL)
            rms_rsb->rptg_is_rptg = GWRMS_REC_NOT_RPTG;
    }

    if ( isSecondaryIndex )
	column_count = idx->gwix_idx.idx_array_count;
    else
	column_count = tcb->gwt_table.tbl_attr_count;

    first_gw_attr = tcb->gwt_attr;
    *ing_record_length = 0;

    for (i = 0; i < column_count; i++ )
    {
	if ( isSecondaryIndex )
	{
	    j = idx->gwix_idx.idx_attr_id[i];
	    gw_attr = first_gw_attr + j - 1;
	}
	else
	{
	    gw_attr = first_gw_attr + i;
	}

	ing_attr = &gw_attr->gwat_attr;
	rms_attr = gw_attr->gwat_xattr.data_address;	

	/* Create the data values needed for conversion */
	ing_value.db_datatype = ing_attr->att_type;
	ing_value.db_prec = ing_attr->att_prec;
	ing_value.db_length = ing_attr->att_width;

	if ( isSecondaryIndex )
	{
	    ing_value.db_data = *ing_record_length + (char *)ingres_buffer;
	}
	else
	{
	    ing_value.db_data = ing_attr->att_offset + (char *)ingres_buffer;
	}

	rms_value.db_datatype = rms_attr->gtype;
	rms_value.db_prec = rms_attr->gprec_scale;
	rms_value.db_length = rms_attr->glength;
        if (rms_value.db_datatype != RMSGW_IMAGINE)
	    rms_value.db_data = rms_attr->goffset + (char *)rms_buffer;
        else
	    rms_value.db_data = tmp_offset + (char *)rms_buffer;

	/*
	** Accumulate the Ingres record length and see if a buffer overflow
	** would occur (such an overflow would be a SERIOUS coding bug)
	*/

	*ing_record_length += ing_value.db_length;
	if (*ing_record_length > ing_buf_len)
	{
	    status = E_DB_ERROR;
	    error->err_code = E_GW5482_RMS_TO_INGRES_ERROR;
	    break;
	}

	/*
	** If the rms column's offset is off the end of the rms buffer, then
	** the user's REGISTER TABLE command is SERIOUSLY out of touch with the
	** actual table data. It's not really clear what we should do about
	** that, though: set a NULL Ingres column? try the next RMS column?
	** reject the record? For now, I have selected "reject the record", but
	** I suspect that is controversial...
	*/
	if (rms_attr->goffset > rms_buf_len)
	{
	    status = E_DB_ERROR;
	    error->err_code = E_GW5485_RMS_RECORD_TOO_SHORT;
	    break;
	}
	/*
	** If the offset is safely within the record, but the length causes
	** this column to run off the end, then trim the length back so that it
	** doesn't overrun the end of the RMS buffer. This causes us to quietly
	** truncate, in a sense, but it is far superior to attempting to
	** convert buffer garbage into Ingres data.
	*/
	if ((rms_attr->goffset + rms_attr->glength) > rms_buf_len)
	{
	    rms_value.db_length = rms_buf_len - rms_attr->goffset;
	}

	if ((status = adc_cvinto(adfcb, &rms_value, &ing_value)) != E_DB_OK)
	{
	    /*
	    ** We failed to convert an RMS record into Ingres format. This
	    ** means that some of the RMS data was un-parseable. For now, we
	    ** will log the error in exhaustive detail. We may later wish to
	    ** change the error handling so that the user can request that
	    ** conversion errors be 'ignored' without generating error
	    ** messages. In that case, we would wish to continue rather than to
	    ** break.
	    **
	    ** Note that many, even most, datatype conversions are handled
	    ** by ADF with exceptions rather than with normal error returns...
	    **
	    ** Also, the ad_errcode in the adfcb often requires parameters to
	    ** be formatted correctly, and we don't have those parameters
	    ** available here, so if possible we use the already-formatted
	    ** message provided to us in ad_errmsgp instead.
	    */
	    if (adfcb->adf_errcb.ad_emsglen > 0)
		_VOID_ ule_format( (i4)0, (CL_ERR_DESC *)0,
			    ULE_MESSAGE, (DB_SQLSTATE *)NULL,
			    adfcb->adf_errcb.ad_errmsgp,
			    adfcb->adf_errcb.ad_emsglen,
			    &msglen, &ulecode, 0 );
	    else
		_VOID_ ule_format( adfcb->adf_errcb.ad_errcode,
			    (CL_ERR_DESC *)0, 
			    ULE_LOG, (DB_SQLSTATE *)NULL, (char *)0, (i4)0,
			    &msglen, &ulecode, 0 );

		/*
		** "An error occurred converting an RMS record to Ingres
		** format. The column which could not be converted was column
		** %0d, with Ingres datatype %1d, Ingres precision %2d, Ingres
		** length %3d, Ingres record offset %4d. The RMS specification
		** was RMS datatype %5d, RMS precision %6d, RMS length %7d, and
		** RMS record offset %8d."
		*/
	    _VOID_ ule_format(E_GW500D_CVT_TO_ING_ERROR, (CL_ERR_DESC *)0,
			ULE_LOG, (DB_SQLSTATE *)NULL, (char *)0, (i4)0,
			&msglen, &ulecode, 9,
			sizeof(i),			&i,
			sizeof(ing_attr->att_type),	&ing_attr->att_type,
			sizeof(ing_attr->att_prec),	&ing_attr->att_prec,
			sizeof(ing_attr->att_width),	&ing_attr->att_width,
			sizeof(ing_attr->att_offset),	&ing_attr->att_offset,
			sizeof(rms_attr->gtype),	&rms_attr->gtype,
			sizeof(rms_attr->gprec_scale),	&rms_attr->gprec_scale,
			sizeof(rms_attr->glength),	&rms_attr->glength,
			sizeof(rms_attr->goffset),	&rms_attr->goffset
		    );
			
	    if ((status = rms_check_adferr( adfcb )) != E_DB_OK)
	    {
		if (adfcb->adf_errcb.ad_errclass == ADF_USER_ERROR)
		    error->err_code = E_GW5484_RMS_DATA_CVT_ERROR;
		else
		    error->err_code = E_GW5482_RMS_TO_INGRES_ERROR;
		break;
	    }
	}

        /*
        ** schang : if convert ok, check if rms data type is 'imaginary',
        ** if yes, add sequence number to its corresponding ingres
        ** data field.  After that, we do some "dirty" manipulation so that
        ** the repeating member can next be converted.
        */
        else
        {   /*
            **  if this column has rms datatype of imaginary
            **  but hasn't set repeating group flag
            **  then set up certain flags
            */
            if (rms_value.db_datatype == RMSGW_IMAGINE)
            {
	        if (rms_rsb->rptg_is_rptg == 0)
                {
                    rms_rsb->rptg_is_rptg = GWRMS_REC_IS_RPTG;
                    rms_rsb->rptg_seq_no = 1;
                }
                else
                {
                    /*
                    ** this is not first time get imaginary datatype
                    ** it's time to increment sequence no and put it into
                    ** imaginary data column
                    */
                    rms_rsb->rptg_seq_no++;
                }
                    switch (ing_value.db_length)
                    {
                        case 1 :
                            *(i1 *)ing_value.db_data = rms_rsb->rptg_seq_no;
                            break;
                        case 2 :
                            *(i2 *)ing_value.db_data = rms_rsb->rptg_seq_no;
                            break;
                        case 4 :
                            *(i4 *)ing_value.db_data = rms_rsb->rptg_seq_no;
                            break;
                        default :
                            break;
                    }
                get_imagine = TRUE;
                rptg_count = TRUE;
            }
            else if (get_imagine)
            {   /*
                ** logic comes here when datatype is not imaginary
                ** but if get_imagine flag is set it means we are
                ** working on data within repeating group and
                ** already meet data with imagine type, treat 
                ** every column from now as part of repeating group
                ** but first we have to take care of the count field
                ** which is not a REAL member of repeating group
                */
                if (rptg_count)
                {
                    /* 
                    ** this means current column is a count field
                    ** copy its value into rms_rsb->rptg_count field
                    ** note this value may be copied multiple times
                    ** until this rms record is discarded.  Set rptg_count
                    ** to false so that we can forget about this field
                    ** until next time we get into this routine
                    */
                    switch (ing_value.db_length)
                    {
                        case 1 :
                            rms_rsb->rptg_count = *(i1 *)ing_value.db_data;
                            break;
                        case 2 :
                            rms_rsb->rptg_count = *(i2 *)ing_value.db_data;
                            break;
                        case 4 :
                            rms_rsb->rptg_count = *(i4 *)ing_value.db_data;
                            break;
                        default :
                            break;
                    }
                    /*
                    ** zero rptg_count means no more data, we zero out
                    ** major rms_rsb field for later use
                    */
                    if (rms_rsb->rptg_count == 0)
                    {
                        EXdelete();
                        rms_rsb->rptg_is_rptg = 0;
                        rms_rsb->rptg_seq_no = 0;
	                error->err_code = E_GW0342_RPTG_OUT_OF_DATA;
                        return(E_DB_ERROR);
                    }   
                    rptg_count = FALSE;
                    if (rms_rsb->rptg_size != 0)
                    {
                        /* We got rptg_size,
                        ** we have to do some move around of repeating
                        ** members.  If we support read-only repeating
                        ** group, we can move members inside rms record.
                        ** For read-write, we must buffer rms record in
                        ** other buffer area, and leave rms record intact
                        ** for latter update.
                        */
                        MEcopy(rms_rsb->rptg_buffer +
                                  rms_rsb->rptg_size*(rms_rsb->rptg_seq_no-1),
                               rms_rsb->rptg_size, rms_rsb->rptg_buffer);
                    }
                }
                else
                {
                    /*
                    ** we get a REAL repeating member 
                    */
                    if (rms_rsb->rptg_size == 0)
                    {
                        /*
                        ** we have not worked on a rptg member, time to
                        ** accumulate rptg size, also set the position of 
                        ** the start of first repeating member
                        */
                        rptg_size += rms_value.db_length;
                        if (rms_rsb->rptg_buffer == NULL)
                            rms_rsb->rptg_buffer = rms_value.db_data;
                    }
                }
            }
            else 
            {
                /*
                ** if not imaginary type and get_imagine flag is not set,
                ** then we worked on a "normal" data type, continue.
                */
            }
        } /* end of convert_no_error */
        tmp_offset += rms_attr->glength;
    }	/* end for column_count */

    /*
    ** put the calculated repeating group size in rms_rsb so that
    ** we can use this value to move repeating data into proper
    ** place in subsequent access of the same rms record.
    */        
    if (rms_rsb->rptg_size == 0)
        rms_rsb->rptg_size = rptg_size;

    /*
    ** If this is for a secondary index, copy the tidp field.
    */

    if ( isSecondaryIndex )
    {
	idx_offset = (char *)ingres_buffer + *ing_record_length;

	*ing_record_length += DB_TID_LENGTH;
	if (*ing_record_length > ing_buf_len)
	{
	    status = E_DB_ERROR;
	    error->err_code = E_GW5482_RMS_TO_INGRES_ERROR;
	}
	else
	{
            *tid_loc = (PTR)idx_offset;
            if (rms_rsb->rptg_is_rptg == GWRMS_REC_IS_RPTG)
                rms_rsb->rptg_seq_no++;
	}
    }

    EXdelete();

    return (status);
}

/*{
** Name: rms_from_ingres	- Convert an Ingres record to RMS format
**
** Description:
**	This procedure will convert an Ingres record to an RMS record by
**	interpreting the descriptions of the two records from the system
**	catalogs. For each column, an ADF function is called to convert from
**	the rms declared type to the ingres declared type.
**
**	We make the assumption that the two types are coercible. This should
**	have been checked during registration.
**
**	The record length issue is rather interesting. In general, the length
**	of the Ingres record is considerably different from the length of the
**	RMS record. In addition, the length of the RMS record AFTER the update
**	may not be the same as the length of the RMS record before the update.
**	And if this is a NEW RMS record (a $PUT), then we are required to
**	figure out the RMS record length and provide it to RMS. Many of these
**	things are currently in flux and you can depend on their changing...
**
**	Note that under-registered, over-registered, and mis-registered tables
**	may and do occur. We perform sanity checks to avoid overwriting
**	storage, but have still not defined all the semantics of the various
**	register table combinations.
**
** Inputs:
**	tcb
**	rsb
**	ingres_buffer	    The Ingres record to be converted.
**	ing_buf_len	    Length of the ingres_buffer, in bytes
**	rms_buffer	    Address of a (hopefully large enough) buffer.
**	rms_cur_reclen	    Length of the rms record being updated, if this
**			    is an update. 0 if this is a put.
**	rms_buf_len	    Length of the RMS buffer, in bytes. Any attempt to
**			    convert data off the end of this buffer is an error.
**
** Outputs:
**	rms_buffer	    contains newly converted record.
**	rms_length	    - how big is the resulting RMS buffer.
**			    Actually, as currently coded, this value is the sum
**			    of all the lengths of all the RMS columns which
**			    were converted, which is only the same as the
**			    RMS record length if the table was completely and
**			    correctly registered...
**	error->err_code	    Detailed error code if an error occurred:
**			    E_GW5483_RMS_FROM_INGRES_ERROR
**			    E_GW5484_RMS_DATA_CVT_ERROR
**			    E_GW5485_RMS_RECORD_TOO_SHORT
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**
** Side Effects:
**
** History:
**	26-dec-89 (paul)
**	    Written.
**	17-apr-90 (bryanp)
**	    Added error parameter and error handling.  Added casts on
**	    'rms_buffer' and 'ingres_buffer' to conform to coding conventions
**	    for use of PTR type.
**	7-jun-1990 (bryanp)
**	    Arm rms_cvt_handler() as our exception handler during datatype
**	    conversion processing, as a first attempt at recovering from
**	    exceptions during conversions.
**	8-jun-1990 (bryanp)
**	    Call rms_check_adferr() to analyzer return code from ADF.
**	    Use pre-formatted ADF error message if possible, rather than our own
**	11-jun-1990 (bryanp)
**	    Distinguish internal errors from 'expected' data conversion errors.
**	19-jun-1990 (linda, bryanp)
**	    Began beefing up the internal error-checking in this routine.
**	    Added more parameters so that this routine could make sensible
**	    decisions about the work it's doing.
*/
DB_STATUS
rms_from_ingres
(
    GW_TCB    *tcb,
    GW_RSB    *rsb,
    PTR	      ingres_buffer,
    i4   ing_buf_len,
    PTR	      rms_buffer,
    i4   rms_cur_reclen,
    i4   rms_buf_len,
    i4   *rms_length,
    DB_ERROR  *error
)
{
    DB_STATUS	    status = E_DB_OK;
    ADF_CB	    *adfcb;
    GW_ATT_ENTRY    *gw_attr;
    DMT_ATT_ENTRY   *ing_attr;
    GWRMS_XATT	    *rms_attr;
    GWRMS_XATT	    *last_rms_attr;
    DB_DATA_VALUE   ing_value;
    DB_DATA_VALUE   rms_value;
    i4		    column_count;
    i4		    i;
    i4		    ulecode;
    i4		    msglen;
    EX_CONTEXT	    context;
    i4		    lastlen;
        
    adfcb = rsb->gwrsb_session->gws_adf_cb;
    
    *rms_length = 0;
    column_count = tcb->gwt_table.tbl_attr_count;
    gw_attr = tcb->gwt_attr;

    /*
    ** Plant an exception handler so that if any lower-level ADF routine
    ** aborts, we can attempt to trap the error and "do the right thing".
    */
    if (EXdeclare(rms_cvt_handler, &context) != OK)
    {
	/*
	** We get here if an exception occurs and the handler longjumps back to
	** us (returns EXDECLARE). In this case, ADF has asked us to abort the
	** query, so we return with an 'error converting record' error.
	*/
	EXdelete();
	adfcb = rsb->gwrsb_session->gws_adf_cb;
	if (adfcb->adf_errcb.ad_errclass == ADF_USER_ERROR)
	    error->err_code = E_GW5484_RMS_DATA_CVT_ERROR;
	else
	    error->err_code = E_GW5483_RMS_FROM_INGRES_ERROR;
	return (E_DB_ERROR);
    }

    for (i = 0; i < column_count; i++, gw_attr++)
    {
	ing_attr = &gw_attr->gwat_attr;
	rms_attr = gw_attr->gwat_xattr.data_address;	

	/* Create the data values needed for conversion */
	ing_value.db_datatype = ing_attr->att_type;
	ing_value.db_prec = ing_attr->att_prec;
	ing_value.db_length = ing_attr->att_width;
	ing_value.db_data = ing_attr->att_offset + (char *)ingres_buffer;
	
	if ((ing_attr->att_offset + ing_attr->att_width) > ing_buf_len)
	{
	    /*
	    ** This Ingres column is off the end of the Ingres buffer. This is
	    ** a coding error, so report it as such:
	    */
	    status = E_DB_ERROR;
	    error->err_code = E_GW5483_RMS_FROM_INGRES_ERROR;
	    break;
	}
	
	rms_value.db_datatype = rms_attr->gtype;
	rms_value.db_prec = rms_attr->gprec_scale;
	rms_value.db_length = rms_attr->glength;
	rms_value.db_data = rms_attr->goffset + (char *)rms_buffer;

	/*
	** If the rms column's offset is off the end of the rms buffer, then
	** the user's REGISTER TABLE command is out of touch with the actual
	** table data. It's not really clear what we should do about that,
	** though: set a NULL RMS column??? try the next RMS column? reject the
	** record? For now, We have selected "reject the record", but we
	** suspect that is controversial...
	**
	** This is the "over-registered table" problem: you have registered
	** more RMS columns that actually can fit in the RMS record. An
	** alternate way of treating this would be to quietly extend the record
	** (re-allocating a larger buffer), since it appears that RMS supports
	** this. A third possibility would be to truncate the record to the max
	** RMS record length. If we truncate, we should issue an error/warning.
	** We might even issue a warning when we extend the record.
	*/
	if (rms_attr->goffset > rms_buf_len)
	{
	    status = E_DB_ERROR;
	    error->err_code = E_GW5485_RMS_RECORD_TOO_SHORT;
	    break;
	}
	/*
	** If the offset is safely within the record, but the length causes
	** this column to run off the end, then trim the length back so that it
	** doesn't overrun the end of the RMS buffer. This causes us to quietly
	** truncate, in a sense, but it is far superior to attempting to store
	** off the end of the RMS buffer.
	*/
	if ((rms_attr->goffset + rms_attr->glength) > rms_buf_len)
	{
	    rms_value.db_length = rms_buf_len - rms_attr->goffset;
	}

	if ((status = adc_cvinto(adfcb, &ing_value, &rms_value)) != E_DB_OK)
	{
	    /*
	    ** We failed to convert an Ingres record into RMS format. This
	    ** means that some of the RMS data was un-parseable. For now, we
	    ** will log the error in exhaustive detail. We may later wish to
	    ** change the error handling so that the user can request that
	    ** conversion errors be 'ignored' without generating error         
	    ** messages. In that case, we would wish to continue rather than to
	    ** break.
	    **
	    ** Also, the ad_errcode in the adfcb often requires parameters to
	    ** be formatted correctly, and we don't have those parameters
	    ** available here, so if possible we use the already-formatted
	    ** message provided to us in ad_errmsgp instead.
	    */
	    if (adfcb->adf_errcb.ad_emsglen > 0)
		_VOID_ ule_format( (i4)0, (CL_ERR_DESC *)0,
			    ULE_MESSAGE, (DB_SQLSTATE *)NULL,
			    adfcb->adf_errcb.ad_errmsgp,
			    adfcb->adf_errcb.ad_emsglen,
			    &msglen, &ulecode, 0 );
	    else
		_VOID_ ule_format( adfcb->adf_errcb.ad_errcode,
			    (CL_ERR_DESC *)0, 
			    ULE_LOG, (DB_SQLSTATE *)NULL, (char *)0, (i4)0,
			    &msglen, &ulecode, 0 );

		/*
		** "An error occurred converting an Ingres record to RMS
		** format. The column which could not be converted was column
		** %0d, with Ingres datatype %1d, Ingres precision %2d, Ingres
		** length %3d, Ingres record offset %4d. The RMS specification
		** was RMS datatype %5d, RMS precision %6d, RMS length %7d, and
		** RMS record offset %8d."
		*/
	    _VOID_ ule_format(E_GW500E_CVT_TO_RMS_ERROR, (CL_ERR_DESC *)0,
			ULE_LOG, (DB_SQLSTATE *)NULL, (char *)0, (i4)0,
			&msglen, &ulecode, 9,
			sizeof(i),			&i,
			sizeof(ing_attr->att_type),	&ing_attr->att_type,
			sizeof(ing_attr->att_prec),	&ing_attr->att_prec,
			sizeof(ing_attr->att_width),	&ing_attr->att_width,
			sizeof(ing_attr->att_offset),	&ing_attr->att_offset,
			sizeof(rms_attr->gtype),	&rms_attr->gtype,
			sizeof(rms_attr->gprec_scale),	&rms_attr->gprec_scale,
			sizeof(rms_attr->glength),	&rms_attr->glength,
			sizeof(rms_attr->goffset),	&rms_attr->goffset
		    );
			
	    if ((status = rms_check_adferr( adfcb )) != E_DB_OK)
	    {
		if (adfcb->adf_errcb.ad_errclass == ADF_USER_ERROR)
		    error->err_code = E_GW5484_RMS_DATA_CVT_ERROR;
		else
		    error->err_code = E_GW5483_RMS_FROM_INGRES_ERROR;
		break;
	    }
	}

	/*
	** Calculate rms record length based on the maximum from our various
	** fields and offsets. NOTE: rms_value.db_length may need to be taken
	** into account here as well, if the conversion code can potentially
	** produce an RMS value of different length than the extended format
	** specifies (in glength).  Also save the identity of the last RMS
	** field, in case it is a varying string type.
	*/
	if ((rms_attr->goffset + rms_attr->glength) > *rms_length)
	{
	    *rms_length = rms_attr->goffset + rms_attr->glength;
	    last_rms_attr = rms_attr;
	}
    }

    /*
    ** Now we want to cross the calculated rms record length against the
    ** current rms buffer length.  We know that *rms_length will always be <=
    ** rms_buf_len.  We need to make sanity checks on the length, which are
    ** different for different file types and different operations...  Also, we
    ** may replace *rms_length with rms_buf_len or rms_cur_rec_len.
    */

    /* If last col is a varying string type, find its length before padding. */
    switch (last_rms_attr->gtype)
    {
	case	RMSGW_BL_VARSTR:
	    lastlen = gw02__padlen((char *)rms_buffer + rms_attr->goffset,
				  ' ', rms_attr->glength, rms_attr->glength);
	    break;

	case	RMSGW_NL_VARSTR:
	    lastlen = gw02__padlen((char *)rms_buffer + rms_attr->goffset,
				  '\0', rms_attr->glength, rms_attr->glength);
	    break;

	default:
	    lastlen = rms_attr->glength;
	    break;
    }

    /* Adjust 'rms_length' return parameter if necessary. */
    switch (last_rms_attr->gtype)
    {
	case	RMSGW_BL_VARSTR:
	case	RMSGW_NL_VARSTR:
	{
	    i4	ldif;

	    ldif = rms_attr->glength - lastlen;
	    *rms_length = *rms_length - ldif;
	    break;
	}

	default:
	{
	    break;
	}
    }

    EXdelete();

    return (status);
}

/*
** Name: rms_index_format	- stub for later use
**
** history:
**	17-apr-90 (bryanp)
**	    Added error parameter, in case we need it eventually.
*/
DB_STATUS
rms_index_format(error)
DB_ERROR	*error;
{
    return (E_DB_OK);
}

/*
** Name: rms_cvt_handler	- exception handler for conversion errors
**
** Description:
**	ADF may raise exceptions during data conversion. This is not at all
**	unexpected and simply reflects the fact that the RMS data is basically
**	external and may not be convertible as the user indicated. When an
**	exception is raised, the EX facility calls us. We, in turn, ask ADF
**	what it thinks about this exception. ADF tells us whether the exception
**	is to be
**	    1) ignored
**	    2) warned about
**	    3) treated as a query-fatal error.
**
**	Right now, we don't handle warnings correctly. Instead, both warnings
**	and ignored conditions are quietly ignored, and result in an EXCONTINUES.
**	query fatal errors result in a EXDECLARE back to the EXdeclare'd
**	routines which do something useful like abort the query.
**
** Inputs:
**	ex_args		    - Exception arguments, as called by EX
**
** Outputs:
**	None
**
** Returns:
**	EXCONTINUES	    - Please continue with the next column.
**	EXDECLARE	    - Please abort this query
**	EXRESIGNAL	    - We don't know what to do, ask the next guy.
**
** Exceptions and Side Effects:
**	The EXDECLARE return causes an abrupt return to a higher routine.
**
** History:
**	7-jun-1990 (bryanp)
**	    Added to help with conversion errors.
**	11-jun-1990 (bryanp)
**	    If the error is a 'user' error, and exceptions are query-fatal,
**	    send error message to front-end via scc_error().
**	29-oct-92 (andre)
**	    in SCF_CB, scf_generic_error has been replaced with scf_sqlstate +
**	    in ADF_ERROR, ad_generic_error has been replaced with ad_sqlstate
**	09-nov-1999 (kinte01)
**	   Rename EXCONTINUE to EXCONTINUES per AIX 4.3 porting change
*/
static STATUS
rms_cvt_handler
(
    EX_ARGS  *ex_args
)
{
    SCF_CB	scf_cb;
    ADF_CB	*adf_cb;
    DB_STATUS	adf_status;
    DB_STATUS	status;
    i4	msglen;
    i4	ulecode;
    char	emsg_buf [ER_MAX_LEN];
    SCF_SCI	sci_list[2];
    i4		sid;

    for (;;)
    {
	/* Get the ADF_CB for this session */
	scf_cb.scf_type = SCF_CB_TYPE;
	scf_cb.scf_length = sizeof(SCF_CB);
	scf_cb.scf_facility = DB_ADF_ID;
	scf_cb.scf_session = DB_NOSESSION;
	scf_cb.scf_ptr_union.scf_sci = (SCI_LIST *)sci_list;
	scf_cb.scf_len_union.scf_ilength = 1;
	sci_list[0].sci_length = sizeof(PTR);
	sci_list[0].sci_code = SCI_SCB;
	sci_list[0].sci_aresult = (PTR)(&adf_cb);
	sci_list[0].sci_rlength = NULL;
	if ((status = scf_call(SCU_INFORMATION, &scf_cb)) != E_DB_OK)
	{
	    gwf_error(scf_cb.scf_error.err_code, GWF_INTERR, 0);
	    gwf_error(E_GW0302_SCU_INFORMATION_ERROR, GWF_INTERR, 0);
	    break;
	}

	/*
	** We'd like an error message, so if there isn't already a buffer,
	** find one.
	*/
	if (adf_cb->adf_errcb.ad_ebuflen == 0)
	{
	    adf_cb->adf_errcb.ad_ebuflen = sizeof(emsg_buf);
	    adf_cb->adf_errcb.ad_errmsgp = emsg_buf;
	}

	/*
	** Now ask ADF to decipher the error which occurred
	*/

	adf_status = adx_handler( adf_cb, ex_args );

	switch ( adf_status )
	{
	    case E_DB_OK:
		/*
		** The exception is to be ignored.
		*/
		return (EXCONTINUES);

	    case E_DB_WARN:
		if (adf_cb->adf_errcb.ad_errcode == E_AD0116_EX_UNKNOWN)
		{
		    /*
		    ** YUCK! ADF doesn't recognize this. Ask the next guy.
		    */
		    return (EXRESIGNAL);
		}
		else
		{
		    /*
		    ** The exception deserves a warning. Unfortunately, we
		    ** aren't currently set up to do this...
		    */
		    return (EXCONTINUES);
		}

	    case E_DB_ERROR:
		/*
		** If the exception is to be query-fatal, ad_errcode is set
		** to an error numbewr in the range of E_AD1120 to E_AD1170,
		** or if an internal error occurred in the ADF error analysis,
		** ad_errcode is set to some internal error code.
		*/
		if (adf_cb->adf_errcb.ad_errcode == E_AD0116_EX_UNKNOWN)
		{
		    /*
		    ** YUCK! ADF doesn't recognize this. Ask the next guy.
		    */
		    return (EXRESIGNAL);
		}
		else
		{
		    switch ( adf_cb->adf_errcb.ad_errcode )
		    {
			case E_AD1120_INTDIV_ERROR:
			case E_AD1121_INTOVF_ERROR:
			case E_AD1122_FLTDIV_ERROR:
			case E_AD1123_FLTOVF_ERROR:
			case E_AD1124_FLTUND_ERROR:
			case E_AD1125_MNYDIV_ERROR:
			    /*
			    ** This was an ADF exception and I have handled
			    ** it for you by formatting an error message for
			    ** you to issue to the user and, after you have
			    ** done this, control should resume at the
			    ** instruction immediately after the one which
			    ** declared you. The code there should cause the
			    ** routine to clean up, if necessary, and return
			    ** with an appropriate error condition.
			    */
			    /*
			    ** Issuing to the user...
			    */
			    CSget_sid( &sid );
			    scf_cb.scf_type = SCF_CB_TYPE;
			    scf_cb.scf_length = sizeof(SCF_CB);
			    scf_cb.scf_facility = DB_GWF_ID;
			    scf_cb.scf_session = sid; /* was: DB_NOSESSION */
			    scf_cb.scf_nbr_union.scf_local_error = 
					    adf_cb->adf_errcb.ad_errcode;
			    STRUCT_ASSIGN_MACRO(adf_cb->adf_errcb.ad_sqlstate,
				scf_cb.scf_aux_union.scf_sqlstate);
			    scf_cb.scf_len_union.scf_blength =
					    adf_cb->adf_errcb.ad_emsglen;
			    scf_cb.scf_ptr_union.scf_buffer =
					    adf_cb->adf_errcb.ad_errmsgp;
			    status = scf_call(SCC_ERROR, &scf_cb);
			    if (status != E_DB_OK)
			    {
				gwf_error(scf_cb.scf_error.err_code,
						GWF_INTERR, 0);
				gwf_error(E_GW0303_SCC_ERROR_ERROR,
						GWF_INTERR, 0);

				return (EXRESIGNAL); /* ??? */
			    }

			    return (EXDECLARE);

			default:
			    /*
			    ** adx_handler() puked.
			    */
			    _VOID_ ule_format( adf_cb->adf_errcb.ad_errcode,
				    (CL_ERR_DESC *)0, ULE_LOG,
				    (DB_SQLSTATE *)NULL, (char *)0, (i4)0,
				    &msglen, &ulecode, 0 );

			    return (EXRESIGNAL);
		    }
		}

	    default:
		/*
		** adx_handler() puked.
		*/
		_VOID_ ule_format( adf_cb->adf_errcb.ad_errcode,
			    (CL_ERR_DESC *)0, 
			    ULE_LOG, (DB_SQLSTATE *)NULL, (char *)0, (i4)0,
			    &msglen, &ulecode, 0 );

		return (EXRESIGNAL);
	}

	break;
    }
    /*
    ** Getting here is a bug...
    */
    return (EXRESIGNAL);
}

/*
** Name: rms_check_adferr()	- grody error handling for ADF error.
**
** Description:
**	ADF's handling of data conversion errors is 2-fold: some error result
**	in exceptions being raised. Others result in a non-zero return code
**	being returned from adc_cvinto(). This routine handles those errors:
**
**	If the error was an internal ADF error, we abort the query.
**	    THERE IS ONE EXCEPTION TO THIS. See history comments re: AD2009
**	If the error was a data conversion error, we check the ADF "-x" setting
**	    1) If it says 'ignore error', we just return OK
**	    2) If it says 'issue warning', we increment ADF's warning counter
**		and return OK.
**	    3) If it says 'abort query', we return E_DB_ERROR.
**
**	Most of this code should really be in ADF, but I'm in a rush and John
**	isn't here to talk to...
**
** Inputs:
**	adf_scb	    - the ADF control block from the adc_cvinto() call.
**
** Outputs:
**	adf_scb	    - may have its warning counts incremented.
**
** Returns:
**	E_DB_OK	    - continue with query
**	E_DB_ERROR  - abort query.
**		      If the adf control block's ad_errclass says ADF_USER_ERROR
**		      then the error message has already been sent to the
**		      front-end and the caller should not treat this as an
**		      'internal' error, but rather as a standard conversion err.
**
** History:
**	8-jun-1990 (bryanp)
**	    Created, apologetically.
**	11-jun-1990 (bryanp)
**	    This routine is still in flux. Current thinking is that this routine
**	    should report the error to the user when aborting the query.
**	    Caller will then take a special return path which does NOT result
**	    in E_SC0206 (an internal error has occurred...).
**	19-jun-90 (linda, bryanp)
**	    Error message AD2009 indicates that no datatype coercion exists.
**	    Until we get the set defined, we would like this message delivered
**	    to the user, rather than generating an internal error.
**	    The reason for this is that the REGISTER TABLE command currently
**	    allows the user to define RMS file mappings which appear valid,
**	    but which the current incarnation of the coercion software cannot
**	    handle. Since this is not really an internal error so much as
**	    a temporary lack on the part of our ADF support, we don't want
**	    to 'frighten' the user with the dread "internal error..." message.
**	    In addition, the AD2009 message may actually clue them in to the
**	    real problem...
**	29-oct-92 (andre)
**	    ERlookup() has been renamed to ERslookup() + ERslookup() returns
**	    SQLSTATE status code instead of generic error + in ADF_ERROR
**	    ad_generic_err was replaced with ad_sqlstate + in SCF_CB
**	    scf_generic_error has been replaced with scf_sqlstate
**      03-jun-93 (schang)
**          errors occurred during conversion, other than overflow and
**          underflow, should return E_DB_ERROR
**      24-mar-2000 (chash01)
**          separate integer overflow count and float overflow count so
**          that front_end will generate correct message.
*/
static STATUS
rms_check_adferr
(
    ADF_CB  *adf_scb
)
{
    SCF_CB	scf_cb;
    DB_STATUS	status;
    i4		sid;
    DB_STATUS	return_status;
    STATUS	tmp_status;
    CL_ERR_DESC	sys_err;

    if (adf_scb->adf_errcb.ad_errclass != ADF_USER_ERROR)
    {
	/*
	** Internal ADF error. Abort query, UNLESS this is the AD2009, which
	** indicates "no coercion exists". For AD2009, let's report the no
	** coercion message to the frontend instead of returning "SC0206". We
	** 'downgrade' this message from an ADF internal error to an ADF
	** user error, format it, and report it to the user. Gack.
	*/
	if (adf_scb->adf_errcb.ad_errcode == E_AD2009_NOCOERCION)
	{
	    tmp_status  = ERslookup(adf_scb->adf_errcb.ad_errcode, 0,
				ER_TIMESTAMP,
				adf_scb->adf_errcb.ad_sqlstate.db_sqlstate,
				adf_scb->adf_errcb.ad_errmsgp,
				adf_scb->adf_errcb.ad_ebuflen, -1,
				&adf_scb->adf_errcb.ad_emsglen,
				&sys_err, (i4)0,
				(ER_ARGUMENT *)0);

	    if (tmp_status != OK)
	    {
		return (E_DB_ERROR);
	    }
	    else
	    {
		/*
		** else we drop through to send the message to the frontend, and
		** return with an 'abort this query' error. We downgrade this
		** message to USER error so that the layers above us will think
		** it is a 'normal' conversion error. We do NOT, however, place
		** it under the control of "-x". This error is unconditionally
		** query-fatal.
		*/
		adf_scb->adf_errcb.ad_errclass = ADF_USER_ERROR; /* GACK! */
		return_status = E_DB_ERROR;
	    }
	}
	else
	{
	    return (E_DB_ERROR);
	}
    }
    else
    {
	/*
	** Check the ADF control block (GACK!! -- I know, I'm sorry...) to
	** decide whether the user requested ignore errors, warn errors, or
	** abort query errors. The only tricky one is warn errors, which
	** require some inspection of the particular error to determine which
	** error code to increment.
	*/

	switch (adf_scb->adf_errcb.ad_errcode)
        {
	    case E_GW7001_FLOAT_OVF:
	    case E_GW7002_FLOAT_UND:
	    case E_GW7000_INTEGER_OVF:
	      /* Set up Warning or Error in the internal message buffer */

	      if (adf_scb->adf_exmathopt == ADX_IGN_MATHEX)
	      {
		  return (E_DB_OK);
	      }
              /*
              ** 24-mar-2000 (chash01)
              **   separate integer overflow count and float overflow count so
              **   that front_end will generate correct message.
              */
	      else if (adf_scb->adf_exmathopt == ADX_WRN_MATHEX)
	      {
                  if (adf_scb->adf_errcb.ad_errcode == E_GW7000_INTEGER_OVF)
	      	      adf_scb->adf_warncb.ad_intovf_cnt++;
                  else
	      	      adf_scb->adf_warncb.ad_fltovf_cnt++;
		  return (E_DB_OK);
              }
              else
	      {
	        /*
	        ** "-xf" == abort query. Do nothing here; drop through to common
	        ** code which sends message to front-end. Then return ERROR,
	        ** which will cause our caller  to abort  the query.
	        */
	          return_status = E_DB_ERROR;
	      }
              break;

            default :
	      return_status = E_DB_ERROR;
              break;
        }
    }
    /*
    ** We get here in the case that this error will abort the query, but it
    ** is a 'user' error, which means that it is NOT an internal error and
    ** should result in a friendly user message rather than an unfriendly
    ** 'internal error' message. The friendly user message is, we hope,
    ** already formatted in the ADF control block. We package it up and send
    ** it to the user, then return E_DB_ERROR to cause our caller to break
    ** out of his processing. Our caller can distinguish this return from
    ** internal error returns because of the ADF_USER_ERROR value in the
    ** ADF control block...
    */
    CSget_sid( &sid );

    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_length = sizeof(SCF_CB);
    scf_cb.scf_facility = DB_GWF_ID;
    scf_cb.scf_session = sid;	    /* was: DB_NOSESSION */
    scf_cb.scf_nbr_union.scf_local_error = adf_scb->adf_errcb.ad_errcode;
    STRUCT_ASSIGN_MACRO(adf_scb->adf_errcb.ad_sqlstate,
	scf_cb.scf_aux_union.scf_sqlstate);
    scf_cb.scf_len_union.scf_blength = adf_scb->adf_errcb.ad_emsglen;
    scf_cb.scf_ptr_union.scf_buffer = adf_scb->adf_errcb.ad_errmsgp;
    status = scf_call(SCC_ERROR, &scf_cb);
    if (status != E_DB_OK)
    {
	gwf_error(scf_cb.scf_error.err_code, GWF_INTERR, 0);
	gwf_error(E_GW0303_SCC_ERROR_ERROR, GWF_INTERR, 0);

	/* upgrade to 'internal error' */
	adf_scb->adf_errcb.ad_errclass = ADF_INTERNAL_ERROR;
    }

    return (return_status);
}
