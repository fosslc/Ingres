/*
** Copyright (c) 1990, 1992 INGRES Corporation
** All Rights Reserved
*/

#include    <compat.h>
#include    <gl.h>
#include    <st.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <adp.h>
#include    <ulf.h>

#include    <adfint.h>
#include    <aduint.h>
#include    <adftrace.h>
#include    <me.h>
#include    <tr.h>

/**
**
**  Name: ADUREDEEM.C - Redeem an ADF_COUPON for the real thing
**
**  Description:
{@comment_line@}...
**
**          adu_redeem - Redeem an ADF_COUPON data structure
**          adu_rdm1_setup() -- setup for redemption
**          adu_rdm2_finish() -- cleanup after redemption
**	    adu_cpn_dump - Dump the value of an ADP_PERIPHERAL COUPON
**          adu_finish_format() -- cleanup after coupon dump
**          adu_lolk() -- Return log key part of coupon
**
**  Function prototypes defined in ADUINT.H file.
**
**  History:
**      07-dec-1989 (fred)
**          Created.
**      06-Oct-1992 (fred)
**          Changed to use new adf_cb field.
**	30-Oct-1992 (fred)
**	    Fixed to correctly handle being called with a length too short
**	    for the original header.
**	20-Nov-1992 (fred)
**	    adu_redeem() rewritten for better clarity & better delineation of
**	    function in support of OME large objects.
**      05-jan-1993 (stevet)
**          Added function prototypes.
**       8-Apr-1993 (fred)
**          Fixed bug where zero length non-nullable blob was
**          returning a null byte.
**      16-Apr-1993 (fred)
**          Added adu_lolk().
**      20-apr-1993 (stevet)
**          Added missing include <st.h> and fixed up mismatched types
**          when calling ST and ME routines.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**      12-Oct-1993 (fred)
**          Removed call to ADP_INFORMATION -- subsumed by
**          adi_per_under(). 
**      14-Sep-1995 (shust01/thaju02)
**          fixed problem of endless loop when we are finished, but we
**	    come in just to get the NULL byte.  work->adw_shared.shd_o_used 
**	    had old size (which was max value), so we never end.  Set
**	    work->adw_shared.shd_o_used = 0.
**      26-feb-1996 (thoda04)
**          Corrected call to adu_error.  First parm is always ADF_CB.
**	19-apr-1996 (toumi01)
**	    For axp_osf init the 4-bytes after the ADP header in the GCA
**	    byte stream as well as the first integer of the coupon area.
**	    On this platform these are not the same field because of a
**	    4-byte slack before the 8-byte aligned coupon.  This corrects
**	    the failure to init the "more segments" indicator for null
**	    nullable blob data, which failure manifests itself in protocol
**	    read errors and iigcc looping when selecting such data.
**      10-sep-1998 (schte01)
**          Modified I4ASSIGN for axp_osf to pass the value, not the address
**          of the value, to be assigned. This worked under the -oldc
**          compiler option and got compile errors under -newc.
**      04-may-1999 (hanch04)
**          Change TRformat's print function to pass a PTR not an i4.
**      22-Nov-1999 (hweho01)
**          Extended the change for axp_osf to ris_u64 (AIX 64-bit).
**      24-may-2000 (stial01)
**          adu_rdm1_setup() clear null indicator if not null (B101656)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      16-apr-1999 (hanch04)
**          For LP64 add cpn_id[5] to debugging prints for expanded
**          64-bit coupon.
**      08-Sep-2000 (hanje04)
**          Extended the change for axp_osf to axp_lnx (Alpha Linux).
**      02-oct-2002 (stial01)
**          Added ADW_FLUSH_INCOMPLETE_SEGMENT for DB_LNVCHR_TYPE's
**          We may need to flush output buffer before it is full.
**      25-mar-2004 (stial01)
**          adu_rdm1_setup() validate result dv len <= MAXI2
**      21-apr-2004 (stial01)
**          adu_redeem() reinit work->adw_fip.fip_pop_cb.pop_coupon = coupon_dv
**          since coupon_dv is on the ade_execute_cx stack, and its location
**          (on the ade_execute_cx stack) can change on subsequent calls
**          to adu_redeem from scf.
**/

/*
** Forward Function References
*/

static i4
adu_finish_format( PTR		 result,
		   i4            length,
		   PTR           buffer);  /* Ignored */
static DB_STATUS
adu_rdm1_setup(ADF_CB             *adf_scb,
	       DB_DATA_VALUE      *result_dv,
	       DB_DATA_VALUE      *coupon_dv,
	       ADP_LO_WKSP        *work,
	       i4                 for_gca);
static DB_STATUS
adu_rdm2_finish(ADF_CB             *adf_scb,
		DB_DATA_VALUE      *result_dv,
		DB_DATA_VALUE      *coupon_dv,
		ADP_LO_WKSP        *work,
		i4                for_gca);

/*
** {
** Name: adu_redeem	- Redeem an ADF_COUPON
**
** Description:
**      This routine redeems an ADF_COUPON, i.e. it materializes the object
**	represented by the coupon.  Since objects represented by coupons
**	may not fit in memory, this routine must deal with returning partial
**	results.  In such cases, it will store its partial status in its fourth
**	argument, which is to be returned for each call. 
**
** Inputs:
**      adf_scb                  Standard ADF session control block
**      result_dv                Ptr to DB_DATA_VALUE into which to
**                               place the result.
**      coupon_dv                Ptr to DB_DATA_VALUE which contains
**                               the input to redeem.
**      workspace_dv             Ptr to DB_DATA_VALUE which points to
**                               the workspace.  This workspace is
**                               expected to be maintained across
**                               calls to this routine for any given
**                               coupon.  (I.e. it can be "new" only
**                               when "continuation" (next param) is
**                               zero.
**      continuation             Is this a continuation of a previous
**                               call.
**
** Outputs:
**      adf_scb->adf_errcb       Filled as appropriate.
**      result_dv->db_length     Filled with the length of the result
**                               area actually used.
**      *workspace->db_data      Filled with info to be used by
**                               subsequent invocations of this routine.
**	Returns:
**          E_DB_ERROR           In case of error
**          E_DB_INFO/E_AD0002_INCOMPLETE
**                               When more calls are necessary
**          E_DB_OK              When complete.
**
**	Exceptions:
**	    None.
**
** Side Effects:
**	    None.
**
** History:
**      07-dec-1989 (fred)
**          Prototyped.
**      06-oct-1992 (fred)
**          Altered to work with timezone integration.  As a temporary measure,
**	    this routine was using the ADF_CB.adf_4rsvd field to store
**	    some context across calls.  In that this field disappeared, a new,
**	    more appropriately named field was added (adf_lo_context).  This
**	    new ADF_CB field is now used in this file.
**	30-Oct-1992 (fred)
**	    Fixed to correctly handle being called with a length too short
**	    for the original header.
**	20-Nov-1992 (fred)
**	    Rewritten for better clarity & better delineation of
**	    function in support of OME large objects.
**      18-Oct-1993 (fred)
**          Moved check for FEXI function so that STAR, which has
**          none, can send null blobs sometimes.  It helps get a tiny
**          bit of support in to STAR.
**      13-Apr-1994 (fred)
**          Altered to hand status from FEXI call straight back.  By
**          not losing the actual status, interrupts may avoid logging
**          too much stuff.
**      14-Sep-1995 (shust01/thaju02)
**          fixed problem of endless loop when we are finished, but we
**	    come in just to get the NULL byte.  work->adw_shared.shd_o_used 
**	    had old size (which was max value), so we never end.  Set
**	    work->adw_shared.shd_o_used = 0.
**	24-Oct-2001 (thaju02)
**	    If adc_lvch_xform() was unable to fit the 2-byte segment
**	    length in the result buffer, decrement result length
**	    with unused byte and flush segment. (B104122)
[@history_template@]...
*/
DB_STATUS
adu_redeem(
ADF_CB            *adf_scb,
DB_DATA_VALUE	   *result_dv,
DB_DATA_VALUE      *coupon_dv,
DB_DATA_VALUE      *workspace_dv,
i4            continuation)
{
    ADP_LO_WKSP 	*work = (ADP_LO_WKSP *) workspace_dv->db_data;
    DB_DT_ID		dtid;
    ADP_PERIPHERAL	*p = (ADP_PERIPHERAL *) result_dv->db_data;
    DB_STATUS		status;
    i4			done = FALSE;
    i4                  flush;
    i4			for_gca = 1;
    i4                  loop_around;

    if (result_dv->db_datatype != coupon_dv->db_datatype)
	return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	
    dtid = ADI_DT_MAP_MACRO(abs(result_dv->db_datatype));
    if (    dtid <= 0
	 || dtid  > ADI_MXDTS
	 || Adf_globs->Adi_dtptrs[dtid] == NULL
	 || !Adf_globs->Adi_dtptrs[dtid]->adi_under_dt
       )
	return(adu_error(adf_scb, E_AD2004_BAD_DTID, 0));

    if ((!continuation) || (work->adw_fip.fip_state == ADW_F_STARTING))
    {
	if (!continuation)
	    work->adw_fip.fip_state = ADW_F_INITIAL;
	status = adu_rdm1_setup(adf_scb, result_dv, coupon_dv, work, for_gca);
	if (status || work->adw_fip.fip_done)
	    return(status);
    }
    else if (work->adw_fip.fip_state == ADW_F_DONE_NEED_NULL)
    {
	/*
	**  This is an indication that we finished, but didn't
	**  send the NULL byte, and must do so now...
	*/
	
	work->adw_shared.shd_o_used = 0;
	status = adu_rdm2_finish(adf_scb, result_dv, coupon_dv, work, for_gca);
	return(status);
    }
    else
    {
	if (Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi == NULL)
	    return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	work->adw_shared.shd_o_used = 0;
	work->adw_shared.shd_o_length = result_dv->db_length;
	work->adw_shared.shd_o_area = (char *) result_dv->db_data;
	work->adw_fip.fip_pop_cb.pop_continuation = 0;

	work->adw_fip.fip_pop_cb.pop_coupon = coupon_dv;
    }

    for (flush = 0;
	    !done
		&& (!flush)
		&& (work->adw_shared.shd_l1_check <
		    	    	    	work->adw_fip.fip_l1_value);
	/* No update action */)
    
    {
	switch (work->adw_shared.shd_exp_action)
	{
	case ADW_FLUSH_SEGMENT:
	case ADW_START:
	    if (for_gca)
	    {
		/*
		** Since we got a segment back, insert the `here
	        ** comes another segment indicator'.
		*/

		i4		one = 1;

		if ( (work->adw_shared.shd_o_length -
		                         work->adw_shared.shd_o_used)
		       < sizeof(one))
		{
		    /* Then the next segment marker won't fit.  Dump */
		    /* the current segment and move on... */

		    /* fix_me -- better error... */
		
		    return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
		}

		I4ASSIGN_MACRO(one,
		     work->adw_shared.shd_o_area[work->adw_shared.shd_o_used]);
		work->adw_shared.shd_o_used += sizeof(one);
	    }
	    /* Fall thru -- if out of input data, get some */

	case ADW_FLSH_SEG_NOFIT_LIND:
	case ADW_NEXT_SEGMENT:
	    
	    if (   (work->adw_shared.shd_exp_action == ADW_NEXT_SEGMENT)
		&& (work->adw_shared.shd_inp_tag == ADP_P_COUPON))
	    {
		/*
		**  For [DBMS style] coupons, each segment is in a row
		**  unto itself, regardless of the length of the row.
		**  Since this routine is datatype independent, it
		**  cannot tell when a segment is up, and thus must
		**  rely on the called routine.  Thus, if the callee
		**  asks for a new segment when processing a DBMS
		**  coupon, this routine will assume that the current
		**  set of data has been all used up...
		*/

		work->adw_shared.shd_i_used =
		    work->adw_shared.shd_i_length;
	    }

	    /* Fall thru... */

	case ADW_GET_DATA:
	    if (work->adw_shared.shd_i_used == work->adw_shared.shd_i_length)
	    {
		/*
		** If no unmoved segment data, then get some more.
		*/
		
		if (work->adw_fip.fip_done)
		{
		    done = TRUE;
		    status = E_DB_OK;
		}
		else
		{
		    status =
			(*Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi)
			    (ADP_GET, &work->adw_fip.fip_pop_cb);
		    if (status)
		    {
			if (work->adw_fip.fip_pop_cb.pop_error.err_code
			    == E_AD7001_ADP_NONEXT)
			{
			    work->adw_fip.fip_done = TRUE;
			    
			    if (status == E_DB_WARN)
				status = E_DB_OK;
			    else
				break;
			}
			else
			{
			    return(adu_error(adf_scb,
				   work->adw_fip.fip_pop_cb.pop_error.err_code,
					     0));
			}
		    }
		    
		    work->adw_shared.shd_i_used = 0;
		    work->adw_shared.shd_i_length =
			work->adw_fip.fip_seg_dv.db_length;
		    work->adw_shared.shd_i_area =
			work->adw_fip.fip_seg_dv.db_data;

		}
	    }
	    break;

	case ADW_CONTINUE:  /* This shouldn't happen */
	default:            /* Nor should anything else */
	    return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	    break;
	}
	if (done)
	    break;

	status = adc_xform(adf_scb, (PTR)work);
	if (DB_FAILURE_MACRO(status))
	{
	    return(adu_error(adf_scb, E_AD7003_ADP_BAD_RECON, 0));
	}
	if (work->adw_shared.shd_exp_action == ADW_FLUSH_SEGMENT)
	{
	    flush = 1;
	}

	/* B104122 */
	if (work->adw_shared.shd_exp_action == ADW_FLSH_SEG_NOFIT_LIND)
	{
	    result_dv->db_length = work->adw_shared.shd_o_used;
	    flush = 1;
	}

	/* B?????? */
	if (work->adw_shared.shd_exp_action == ADW_FLUSH_INCOMPLETE_SEGMENT)
	{
	    result_dv->db_length = work->adw_shared.shd_o_used;
	    work->adw_shared.shd_exp_action = ADW_FLUSH_SEGMENT;
	    flush = 1;
	}

	work->adw_fip.fip_pop_cb.pop_continuation = 0;
    }

    if (work->adw_shared.shd_l1_check == work->adw_fip.fip_l1_value)
    {
	done = TRUE;
    }

    if (work->adw_fip.fip_test_mode)
    {
	work->adw_fip.fip_test_sent += work->adw_shared.shd_o_used;
	
	if ((work->adw_fip.fip_test_length
	         - work->adw_fip.fip_test_sent
	         - work->adw_fip.fip_null_bytes
	         - (for_gca * sizeof(i4)))
	     <= 0)
	{
	    /*
	    **	Then, for test purposes, we've sent all that we can.
	    **	Emulate completion.
	    */

	    work->adw_fip.fip_done = done = TRUE;
	    work->adw_shared.shd_i_used = work->adw_shared.shd_i_length;
	    work->adw_shared.shd_l1_check = work->adw_fip.fip_l1_value;
	    work->adw_shared.shd_o_used = work->adw_fip.fip_test_length
		                             - work->adw_fip.fip_test_sent
		                             - sizeof(i4)    /* end marker */
			                     - work->adw_fip.fip_null_bytes;
	                                        /* null indicator */
	    work->adw_fip.fip_test_sent =
		work->adw_fip.fip_test_length - sizeof(i4)
		                              - work->adw_fip.fip_null_bytes;
	}

    }
    
    if (done && ((work->adw_shared.shd_exp_action == ADW_NEXT_SEGMENT)
		 || (work->adw_shared.shd_exp_action == ADW_FLUSH_SEGMENT)))
    {
	/*
	** If we think we are done and have used all the input,...
	*/

	status = adu_rdm2_finish(adf_scb, result_dv, coupon_dv, work, for_gca);
    }
    else
    {
	/*
	** Otherwise, if we are surpassing the amount of data which
	** be available...
	*/

	if (work->adw_shared.shd_l1_check > work->adw_fip.fip_l1_value)
	{
	    /*
	    ** FIX_ME -- better error message...
	    ** Then we would be sending an inconsistent blob.
	    ** That would be bad.
	    */

	    return(adu_error(adf_scb, E_AD7004_ADP_BAD_BLOB,0));
	}
	adf_scb->adf_errcb.ad_errcode  = E_AD0002_INCOMPLETE;
	status = E_DB_INFO;
    }
    return(status);
}

/*{
** Name: adu_rdm1_setup	- Setup for Redeem an ADF_COUPON
**
** Description:
**      This routine performs the initialization for the redeeming of
**      a coupon.  This amount primarily to initializing the
**      ADP_LO_WKSP function instance private and shared (adw_fip &
**      adw_shared, respectively).  This routine also moves the
**      appropriate peripheral header into the output area.
**
**      Since this routine moves the peripheral header to the output
**      area, in the case where the original call to this routine has
**      insufficient space for the output header, this routine may be
**      called more than once.  We note this since it is "unusual" for
**      the initialization routine to be called more than one time for
**      an object.  In the case where the original space is of
**      insufficent size, then this routine will return an indication
**      that the output area is full, and will move nothing into it.
**      Thus, this routine considers it a call-protocol error to be
**      called a second time with insufficient space.
**
** Inputs:
**      adf_scb                     The session's control block (ptr).
**      result_dv                   Pointer to DB_DATA_VALUE for
**                                      result.
**      coupon_dv                   Pointer to DB_DATA_VALUE
**                                      describing the input coupon to
**                                      be redeemed.
**      workspace_dv                Pointer to DB_DATA_VALUE
**                                      describing redeem's workspace.
**      continuation                Continuation indicator.  0
**                                      indicates the first call, else
**                                      not the first call.
**
** Outputs:
**      adf_scb->adf_error          Filled as appropriate.
**      *result_dv->db_data         Setup with beginning of peripheral
**                                      data type.
**      *workspace_dv->db_data      Initialized for ongoing redeem work.
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None.
**
** Side Effects:
**	    None.
**
** History:
**      02-dec-1992 (fred)
**          Coded.  Created from adu_redeem prototype in support of
**          OME large objects.
**       8-Apr-1993 (fred)
**          Fixed bug in zero length large object handling.  For large
**          objects of zero length which are not nullable, the length
**          should not include the null-value-byte.
**      12-Oct-1993 (fred)
**          Removed call to ADP_INFORMATION -- subsumed by
**          adi_per_under(). 
**      10-sep-1998 (schte01)
**          Modified I4ASSIGN for axp_osf to pass the value, not the address
**          of the value, to be assigned. This worked under the -oldc
**          compiler option and got compile errors under -newc.
**	28-jul-1999 (thaju02)
**	    If the coupon stipulates that the data is not null, then
**	    the minimum result buffer length must be large enough to
**	    hold one byte of data along with all necessary info
**	    (ADP_HDR_SIZE + nextsegmarker(i4) + 2byteseglenindicator
**	    + 1byteofdata). Given a buffer with only 18 bytes of space
**	    available, 12 bytes are filled with peripheral header info
**	    in adu_rdm1_setup and 4 bytes are filled with the next
**	    segment marker in adu_redeem. In adc_lvch_xform, the 2 
**	    byte segment length indicator is initially set to zero 
**	    in the result buffer. If there is no space left in the 
**	    result buffer for segment data, we flush the buffer. In 
**	    the front end, upon receiving this result buffer, the 
**	    next segment marker indicates that there is data following, 
**	    yet the length is zero; terminal monitor infinitely loops. 
**	    (b98104)
**      12-aug-99 (stial01)
**          adu_rdm1_setup() 'blob_work' arg to adi_per_under should be  
**          NULL before redeem.
**      24-may-2000 (stial01)
**          adu_rdm1_setup() clear null indicator if not null (B101656)
**
[@history_template@]...
*/
DB_STATUS static
adu_rdm1_setup(ADF_CB             *adf_scb,
	       DB_DATA_VALUE      *result_dv,
	       DB_DATA_VALUE      *coupon_dv,
	       ADP_LO_WKSP        *work,
	       i4                 for_gca)
{
    DB_STATUS	    	status;  
    ADP_PERIPHERAL      *p = (ADP_PERIPHERAL *) result_dv->db_data;
    DB_DT_ID            dtid =
	ADI_DT_MAP_MACRO(abs(result_dv->db_datatype));
    i4			min_size;
    bool		isnull = 0;

    if (coupon_dv->db_datatype < 0)
	work->adw_fip.fip_null_bytes = 1;
    else
	work->adw_fip.fip_null_bytes = 0;

    if (ult_check_macro(&Adf_globs->Adf_trvect, ADF_011_RDM_TO_V_STYLE,
			&dummy1, &dummy2))
    {
	work->adw_fip.fip_test_mode = 1;
	work->adw_fip.fip_test_length = ADP_TST_VLENGTH +
	                                   work->adw_fip.fip_null_bytes;
	work->adw_fip.fip_test_sent = 0;
    }
    else
    {
	work->adw_fip.fip_test_mode = 0;
    }

    work->adw_shared.shd_exp_action = ADW_START;
    work->adw_fip.fip_done = FALSE;

    /*
    ** Fool the main routine into getting us an input segment.  Make
    ** look like the input segement is all used up.
    */
    
    work->adw_shared.shd_type = coupon_dv->db_datatype;
    work->adw_shared.shd_i_area = (char *) 0;
    work->adw_shared.shd_i_used = work->adw_shared.shd_i_length = 0;

    work->adw_shared.shd_o_used = ADP_HDR_SIZE;
    work->adw_shared.shd_o_length = result_dv->db_length;
    work->adw_shared.shd_o_area = (char *) result_dv->db_data;

    if (work->adw_shared.shd_o_length > MAXI2)
    {
	TRdisplay("adu_rdm1_setup shd_o_length %d MAX %d\n",
		work->adw_shared.shd_o_length, MAXI2);
	return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    }
    work->adw_shared.shd_l0_check = work->adw_shared.shd_l1_check = 0;

    work->adw_fip.fip_pop_cb.pop_continuation = ADP_C_BEGIN_MASK;
    work->adw_fip.fip_pop_cb.pop_user_arg = (PTR) 0;
    
    work->adw_fip.fip_pop_cb.pop_type = (ADP_POP_TYPE);
    work->adw_fip.fip_pop_cb.pop_length = sizeof(ADP_POP_CB);
    work->adw_fip.fip_pop_cb.pop_ascii_id = ADP_POP_ASCII_ID;
    work->adw_fip.fip_pop_cb.pop_temporary = ADP_POP_PERMANENT;
    work->adw_fip.fip_pop_cb.pop_coupon = coupon_dv;
    work->adw_fip.fip_pop_cb.pop_segment = &work->adw_fip.fip_seg_dv;
    work->adw_fip.fip_pop_cb.pop_underdv = &work->adw_fip.fip_under_dv;
    status = adi_per_under(adf_scb,
			   result_dv->db_datatype,
			   &work->adw_fip.fip_under_dv);

    if (status)
    {
	return(adu_error(adf_scb, E_AD7000_ADP_BAD_INFO, 0));
    }

    STRUCT_ASSIGN_MACRO(work->adw_fip.fip_under_dv,
			work->adw_fip.fip_seg_dv);
    work->adw_fip.fip_seg_dv.db_data = (char *) ((char *) work +
						 sizeof(ADP_LO_WKSP));

    if (ADI_ISNULL_MACRO(coupon_dv) ||
       ((((ADP_PERIPHERAL *)coupon_dv->db_data)->per_length0 == 0)
       && (((ADP_PERIPHERAL *) coupon_dv->db_data)->per_length1 == 0))) 
	isnull = 1;

    if (isnull)
	/* hdr + segment indicator + sizeof(null byte) */
        min_size = ADP_HDR_SIZE + sizeof(i4) + work->adw_fip.fip_null_bytes; 
    else	
	/* hdr + segment indicator + segment length + segment single byte */
        min_size = ADP_HDR_SIZE + sizeof(i4) + sizeof(i2) + sizeof(char); 

    if (result_dv->db_length >= min_size)
    {
	if (for_gca)
	    work->adw_shared.shd_out_tag = ADP_P_GCA;
	else
	    work->adw_shared.shd_out_tag = ADP_P_DATA;
	
	I4ASSIGN_MACRO(work->adw_shared.shd_out_tag, p->per_tag);
	
	work->adw_shared.shd_out_segmented =
	    (work->adw_shared.shd_out_tag != ADP_P_DATA);

	I4ASSIGN_MACRO( ((ADP_PERIPHERAL *)
			 coupon_dv->db_data)->per_tag,
		       work->adw_shared.shd_inp_tag);
	work->adw_shared.shd_inp_segmented =
	    (work->adw_shared.shd_inp_tag != ADP_P_DATA);

	I4ASSIGN_MACRO( ((ADP_PERIPHERAL *)
			 coupon_dv->db_data)->per_length0,
		       p->per_length0);
	I4ASSIGN_MACRO(p->per_length0, work->adw_fip.fip_l0_value);
	I4ASSIGN_MACRO(((ADP_PERIPHERAL *)
			coupon_dv->db_data)->per_length1,
		       p->per_length1);
	I4ASSIGN_MACRO(p->per_length1, work->adw_fip.fip_l1_value);
	work->adw_fip.fip_state = ADW_F_RUNNING;
    }
    else if (work->adw_fip.fip_state != ADW_F_STARTING)
    {
	work->adw_fip.fip_state = ADW_F_STARTING;
	result_dv->db_length = 0;
	adf_scb->adf_errcb.ad_errcode = E_AD0002_INCOMPLETE;
	return(E_DB_INFO);
    }
    else
    {
	return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    }

    if (isnull)
    {
	if (work->adw_fip.fip_test_mode)
	{
	    ((DB_TEXT_STRING *) p)->db_t_count = (i2) 0;

	    if (result_dv->db_length >=
		    (work->adw_fip.fip_test_length -
		              work->adw_fip.fip_test_sent))

	    {
		
		result_dv->db_length = work->adw_fip.fip_test_length -
		     work->adw_fip.fip_test_sent;
		work->adw_fip.fip_test_sent = work->adw_fip.fip_test_length;
	    }
	    else
	    {
		work->adw_fip.fip_state = ADW_F_DONE_NEED_NULL;
		work->adw_fip.fip_test_sent += result_dv->db_length;
		adf_scb->adf_errcb.ad_errcode = E_AD0002_INCOMPLETE;
		return(E_DB_INFO);
	    }
	}
	else
	{
	    i4	    zero = 0;

	    /*
	    **  Insert GCA mark that says there is no segment.
	    */
	
#if defined(axp_osf) || defined(ris_u64) || defined(LP64) || \
    defined(axp_lnx)
	    /*
	    **	We want to init the GCA field after the header, but
	    **	on axp_osf this is not the same as the start of the
	    **	coupon, because 8-byte alignment causes a slack 4-byte
	    **	integer to be between the header and the coupon.  See
	    **	adu_redeem above for a different way this field is set
	    **	to 1 when we have real data (not null).  The addressing
	    **	of this field should really be standardized in such a
	    **	way that contiguous layout of 4-byte aligned fields is
	    **	not assumed.
	    */
            I4ASSIGN_MACRO(zero,*((char *)p + ADP_HDR_SIZE)); 
#endif
	    I4ASSIGN_MACRO(zero, p->per_value);
	    
	    result_dv->db_length = ADP_NULL_PERIPH_SIZE;
	    if (work->adw_shared.shd_type > 0)
		result_dv->db_length -= 1;
        }
	if (ADI_ISNULL_MACRO(coupon_dv))
	    ADF_SETNULL_MACRO(result_dv);
	else
	    ADF_CLRNULL_MACRO(result_dv);
	adf_scb->adf_errcb.ad_errcode = E_DB_OK;
	work->adw_fip.fip_done = TRUE;
	return(E_DB_OK);
    }

    if (work->adw_fip.fip_test_mode)
    {
	i4	    	segment_count;
	i4	    	length1;
	i2		count;
	
	I4ASSIGN_MACRO(p->per_length1, length1);
	
	I4ASSIGN_MACRO(p->per_length0, segment_count);
	
	if (!segment_count)
	    segment_count = ADP_TST_SEG_CNT;
	
	count = min(	ADP_TST_VLENGTH,
		    (i2) (length1
			  + (segment_count *
			     (sizeof(i4) + sizeof(i2)))
			  + sizeof(i4) /* no more segments */
			  + work->adw_shared.shd_o_used)
		    ) - sizeof(i2);
	I2ASSIGN_MACRO(count, ((DB_TEXT_STRING *) p)->db_t_count);
    }
    return(E_DB_OK);
}

/*{
** Name: adu_rdm2_finish	- Finish Redeeming an ADF_COUPON
**
** Description:
**      This routine completes the action of redeeming a coupon.
**      If running in test mode, this routine makes sure that the
**      varchar field is padded out with the appropriate amount of
**      garbage.
**
**      In general, it places the "closing remarks" in the
**      data stream.  This means that it will place the final segment
**      indicator (saying "no more") in the stream if the stream is
**      destined for GCA.  If the datatype is nullable, then the null
**      bit is set/cleared as appropriate.  If there is insufficient
**      room to make these closing remarks (or to supply the correct
**      amount of garbage in the test case), then the fip_state is set
**      to ADW_F_DONE_NEED_NULL, and an incomplete status is returned.
**      This status will set up the main redeem call to call this
**      routine again with a new buffer, repeating this action until
**      all is done.
**
** Inputs:
**      adf_scb                        Standard ADF SCB Ptr
**      result_dv                      Ptr to output area DB_DATA_VALUE
**      coupon_dv                      PTR to input area DB_DATA_VALUE
**      work                           Ptr to ADP_LO_WKSP in which work
**                                         is done.
**          ->adw_shared               Contains the result of the action
**                                         so far
**          ->adw_fip                  Contains more such results
**      for_gca                        Nat indicating whether the result
**                                         is destined for GCA
**                                         transmission.
**
** Outputs:
**      adf_scb->adf_error             Filled as appropriate.
**      result_dv->db_length           Set correctly.
**      work->adw_fip.fip_state        Set to indicate whether to
**                                         return.
**      work->adw_fip.fip_test*        Set to control the garbage
**                                         returned for the test cases.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None
**
** Side Effects:
**	    None
**
** History:
**      07-dec-1989 (fred)
**          Prototyped.
**      06-oct-1992 (fred)
**          Altered to work with timezone integration.  As a temporary measure,
**	    this routine was using the ADF_CB.adf_4rsvd field to store
**	    some context across calls.  In that this field disappeared, a new,
**	    more appropriately named field was added (adf_lo_context).  This
**	    new ADF_CB field is now used in this file.
**	30-Oct-1992 (fred)
**	    Fixed to correctly handle being called with a length too short
**	    for the original header.
**	20-Nov-1992 (fred)
**	    Rewritten for better clarity & better delineation of
**	    function in support of OME large objects.
[@history_template@]...
*/
DB_STATUS static
adu_rdm2_finish(ADF_CB             *adf_scb,
		DB_DATA_VALUE      *result_dv,
		DB_DATA_VALUE      *coupon_dv,
		ADP_LO_WKSP        *work,
		i4                for_gca)
{
    DB_STATUS         status = E_DB_OK;
    i4                zero = 0;

    if (work->adw_shared.shd_l1_check != work->adw_fip.fip_l1_value)
    {
	/* Fix_me -- get more detailed error message to log (log_key, */
	/* what's wrong, etc. */

	/* Then we are sending an inconsistent blob */

	status = adu_error(adf_scb, E_AD7004_ADP_BAD_BLOB,0);
    }
    else if (result_dv->db_length >=
	     (work->adw_shared.shd_o_used + (for_gca * sizeof(i4)) +
	                                        work->adw_fip.fip_null_bytes))

    {
	if (for_gca)
	{
	    I4ASSIGN_MACRO(zero,
			   result_dv->db_data[work->adw_shared.shd_o_used]);
	    work->adw_shared.shd_o_used += sizeof(i4);
	}
	if (work->adw_fip.fip_test_mode)
	{
	    work->adw_fip.fip_test_sent += sizeof(i4);
	    if (work->adw_fip.fip_test_sent <=
		    (work->adw_fip.fip_test_length -
		                      work->adw_fip.fip_null_bytes))

	    {
		/* Then there isn't enough to fill the varchar() */

		if (result_dv->db_length <
		    (work->adw_fip.fip_test_length
		         - work->adw_fip.fip_test_sent))
		{
		    /*
		    ** This is for test purposes only.
		    **
		    ** We need to send a true VARCHAR() datatype.
		    ** Thus, if the blob os smaller than that, then we
		    ** need to pad it out with garbage bytes.  To do
		    ** so, we will increment test_sent by the
		    ** remainder of the buffer, and happily return,
		    ** knowing that we will arrive here with an empty
		    ** buffer in the relatively near future.
		    **
		    ** To set test_sent appropriately, we must first
		    ** decrement it by the amount it was incremented
		    ** before arriving here.  Then we increment as
		    ** appropriate.
		    */
		
		    work->adw_fip.fip_test_sent -=
			work->adw_shared.shd_o_used; 
		    work->adw_fip.fip_test_sent += result_dv->db_length;
		    work->adw_fip.fip_state = ADW_F_DONE_NEED_NULL;
		    adf_scb->adf_errcb.ad_errcode  = E_AD0002_INCOMPLETE;
		    status = E_DB_INFO;
		}
	    }
	}
	
	if (status == E_DB_OK)
	{
	    result_dv->db_length = work->adw_shared.shd_o_used +
		    work->adw_fip.fip_null_bytes;
	    
	    if (work->adw_fip.fip_null_bytes)
	    {
		/*
		**	Then the datatype was nullable.  Copy the NULL byte
		**	from the coupon.
		*/
		
		if (ADI_ISNULL_MACRO(coupon_dv))
		{
		    ADF_SETNULL_MACRO(result_dv);
		}
		else
		{
		    ADF_CLRNULL_MACRO(result_dv);
		}
	    }
	    adf_scb->adf_errcb.ad_errcode = E_DB_OK;
	}
    }
    else
    {
	/*
	**  In this case, we are at the end of data, but the "closing
	**	remarks" (no more strings (if gca) and null bytes) doesn't
	**	fit in the buffer.
	**
	**  We mark this fact by setting the state to ADW_F_DONE_NEED_NULL.
	*/
	result_dv->db_length = work->adw_shared.shd_o_used;
	work->adw_fip.fip_state = ADW_F_DONE_NEED_NULL;

	adf_scb->adf_errcb.ad_errcode  = E_AD0002_INCOMPLETE;
	status = E_DB_INFO;
    }

    return(status);
}

/*{
** Name: adu_cpn_dump	- Dump the value of a coupon
**
** Description:
**      This routine simply dumps the current value of a particular ADP_COUPON
**	structure.  The information is provided as ADF knows it -- tag & length
**	are labelled, the coupon itself is just a bunch of numbers. 
**
** Inputs:
**      adf_scb                         Session Control Block.
**      cpn_dv                          Ptr to DB_DATA_VALUE describing the
**					coupon to be printed
**      res_dv                          Ptr to DB_DATA_VALUE describing the
**					result area.
**
** Outputs:
**      *res_dv->db_data                Filled in.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      01-feb-1990 (fred)
**          Created.
[@history_template@]...
*/
DB_STATUS
adu_cpn_dump(
ADF_CB             *adf_scb,
DB_DATA_VALUE      *cpn_dv,
DB_DATA_VALUE      *res_dv)
{
    DB_DT_ID		dtid = abs(cpn_dv->db_datatype);
    ADP_PERIPHERAL	*p = (ADP_PERIPHERAL *) cpn_dv->db_data;
    DB_TEXT_STRING	*result = (DB_TEXT_STRING *) res_dv->db_data;
    ADP_PERIPHERAL	peripheral;
    
    if ((Adf_globs->Adi_dtptrs[dtid]->adi_dtstat_bits & AD_PERIPHERAL) == 0)
    {
	STcopy("<Not a peripheral>", (char *)result->db_t_text);
	result->db_t_count = STlength((char *)result->db_t_text);
    }
    else if (cpn_dv->db_length < sizeof(ADP_PERIPHERAL))
    {
	STcopy("<Too short to be a coupon>", (char *)result->db_t_text);
	result->db_t_count = STlength((char *)result->db_t_text);
    }
    else if (res_dv->db_length < 100)
    {
	STcopy("<Result too short>", (char *)result->db_t_text);
	result->db_t_count = STlength((char *)result->db_t_text);
    }
    else
    {
	MECOPY_CONST_MACRO((PTR)p, sizeof(peripheral), (PTR)&peripheral);
	p = &peripheral;
	TRformat(adu_finish_format, result, result->db_t_text,
			    (res_dv->db_length - sizeof(result->db_t_count)),
		    "Tag: %d., Length: %8x%8x, Cpn: <%#.#{%8x %}>",
		    p->per_tag, p->per_length0, p->per_length1,
			(sizeof(p->per_value.val_coupon) /
			    sizeof(p->per_value.val_coupon.cpn_id[0])),
			sizeof(p->per_value.val_coupon.cpn_id[0]),
			p->per_value.val_coupon.cpn_id, 0);
    }
    
    return(E_DB_OK);
}

/*{
** Name: adu_finish_format	- Complete the TRformat processing
**
** Description:
**      This routine is called by TRformat at the completetion of processing of
**	its call.  This routine simply fills in the count field for the varchar,
**	and returns. 
**
** Inputs:
**      result                          Pointer to DB_TEXT_STRING being filled
**      length                          The length of the finished product
**      buffer                          The buffer (ignored)
**
** Outputs:
**      result->db_t_count              Filled with the length parameter
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      01-Feb-1990 (fred)
**          Created.
[@history_template@]...
*/
static i4
adu_finish_format( PTR		result,
		   i4           length,
		   PTR          buffer)  /* Ignored */
{
    ((DB_TEXT_STRING *)(result))->db_t_count = length;
}

/*{
** Name: adu_lolk	- Return logical key part of coupon
**
** Description:
**      This routine simply returns the logical key part of a
**      particular ADP_COUPON structure.
**
** Inputs:
**      adf_scb                         Session Control Block.
**      cpn_dv                          Ptr to DB_DATA_VALUE describing the
**					coupon to be printed
**      res_dv                          Ptr to DB_DATA_VALUE describing the
**					result area.  Must be DB_TAB_KEY
**
** Outputs:
**      *res_dv->db_data                Filled in.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      16-Apr-1993 (fred)
**          Created.
[@history_template@]...
*/
DB_STATUS
adu_lolk(
ADF_CB             *adf_scb,
DB_DATA_VALUE      *cpn_dv,
DB_DATA_VALUE      *res_dv)
{
    DB_DT_ID		dtid = abs(cpn_dv->db_datatype);
    ADP_PERIPHERAL	*p = (ADP_PERIPHERAL *) cpn_dv->db_data;
    ADP_PERIPHERAL	peripheral;
    
    if ((Adf_globs->Adi_dtptrs[dtid]->adi_dtstat_bits & AD_PERIPHERAL) == 0)
    {
	MEfill(res_dv->db_length, '\377', res_dv->db_data);
    }
    else
    {
	MEfill(res_dv->db_length, '\0', res_dv->db_data);
    }
    
    return(E_DB_OK);
}
