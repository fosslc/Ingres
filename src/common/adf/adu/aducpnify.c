/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved
*/

#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <adp.h>
#include    <ulf.h>
#include    <me.h>

#include    <adfint.h>
#include    <adftrace.h>


/*
**  Forward Function References
*/
DB_STATUS static
adu_cpn1_init(ADF_CB	    *scb,
	      i4            continuation,
	      DB_DATA_VALUE *lodv,
	      ADP_LO_WKSP   *workspace,
	      DB_DATA_VALUE *cpn_dv);

DB_STATUS static
adu_cpn2_get_seg(ADF_CB      *scb,
		 ADP_LO_WKSP *workspace);

/*
** {
** Name:       adu_couponify -- Turn a data value into a coupon
**
** Description:
**      This routine takes a flat large object and turns it into a
**      coupon.  It understands both GCA and non-GCA styles of
**      objects, and deals with them all appropriately.
**      
**      A coupon header is constructed in the output coupon locations.
**      Then the adc_xform() routine is called to convert the datatype
**      itself.  If possible (i.e. style is NOT ADP_P_GCA_L_UNK), then
**      length of the the resultant large object is checked against
**      the header provided as input;  if not, the newly discovered
**      length is stored with the coupon.  Any transmission remnants
**      are handled by this routine (e.g. segment indicators necessary
**      for GCA transmission).
**
** Re-entrancy:
**      This routine is reentrant.  Any and all long-term context is
**      contained in the workspace provided and maintained by the caller.
**
** Inputs:
**      scb             The standard ADF session control block
**      continuation    Is this the first call in this context to this
**                      routine
**      lodv            Ptr to DB_DATA_VALUE containing the inline
**                      sections of the large object
**      workspace       Ptr to ADP_LO_WKSP for this object's context
**      cpn_dv          Ptr to DB_DATA_VALUE in which the coupon is to
**                      be constructed.
** Outputs:
**      scb->adf_errcb  Filled as appropriate
**      workspace       Modified for use by this routine.  Must be
**                      preserved over calls to this routine as long
**                      as continuation is set.
**      workspace->adw_shared.shd_i_used
**                      Set on last call describing how much of the
**                      input was used.  This is necessary since the
**                      caller cannot necessarily determine where the
**                      large object ends -- since the large object's
**                      length is not specified in the DB_DATA_VALUE
**                      which accompanies it.
**
** Returns:
**      E_DB_ERROR      When something went wrong
**      E_DB_INFO/E_AD0002_INCOMPLETE
**                      When more data is needed
**      E_DB_OK         When operation is complete.
**	<function return values>
**
** Exceptions: (if any)
**	None.
**
** Side Effects: 
**      None.
**
** History:
**      11-Dec-1992 (fred)
**          Created.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      19-Jul-1993 (fred)
**          Fixed bugs in creation and manipulations for null or zero 
**          length objects.
**       6-Aug-1993 (fred)
**          Fix another bug in management of long objects and null.
**       6-aug-93 (shailaja)
**	    Unnested comments.
**	25-aug-93 (ed)
**	    remove dbdbms.h
**      13-Apr-1994 (fred)
**          Separate some error processing.  Also, add support (i.e.
**          ignore) for interrupt handling.
**      19-nov-1994(andyw)
**          Added NO_OPTIM due to Solaris 2.4/Compiler 3.00 errors
**      1-june-1995 (hanch04)
**          Removed NO_OPTIM su4_us5, installed sun patch
**      26-feb-1996 (thoda04)
**          Corrected call to adu_error.  First parm is always ADF_CB.
**	06-Sep-2000 (thaju02)
**	    The amount of data buffer used (workspace->adw_shared.shd_i_used) 
**	    can not be greater than the amount of data from the fe 
**	    (workspace->adw_shared.shd_i_length). (b102547)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	27-Sep-2001 (thaju02)
**	    Ensure we account for null indicator in parsing data from 
**	    front end. (B103391)
**	23-apr-2003 (devjo01)
**	    ADP_PERIPHERAL does not match the data in the input stream
**	    on 64 bit platforms, due to alignment padding.  This causes
**	    the tail of this structure to be unitialized, and if the
**	    BLOB column is NULLable, and the uninitialized memory zero,
**	    processing the BLOB will end prematurely leaving the rest
**	    of the BLOB to be processed as the next tuple. (b110124).
*/
DB_STATUS
adu_couponify(
ADF_CB             *scb,
i4                 continuation,
DB_DATA_VALUE      *lodv,
ADP_LO_WKSP        *workspace,
DB_DATA_VALUE      *cpn_dv)
{
    DB_STATUS           status = E_DB_OK;
    ADP_PERIPHERAL	*cpn = (ADP_PERIPHERAL *) cpn_dv->db_data;
    i4			done = 0;
    i4                  no_flush = 0;
    i4                  first_time = 0;

    if (!continuation || workspace->adw_fip.fip_state == ADW_F_STARTING)
    {
	status = adu_cpn1_init(scb, continuation, lodv, workspace, cpn_dv);
	if (status || workspace->adw_fip.fip_done)
	    return(status);
	    
	continuation = 0; /* adc_lo_xform() has not been called yet */
	first_time = 1;
    }
    else
    {
	workspace->adw_shared.shd_i_area = (char *) lodv->db_data;
	workspace->adw_shared.shd_i_length = lodv->db_length;
	workspace->adw_shared.shd_i_used = 0;
    }

    if (workspace->adw_fip.fip_state == ADW_F_DONE_NEED_NULL)
    {
	/* 1 byte null indicator */ 
	workspace->adw_shared.shd_i_used++;
	return(status);
    }

    do
    {
	switch (workspace->adw_shared.shd_exp_action)
	{
	case ADW_START:
	case ADW_NEXT_SEGMENT:

	    status = adu_cpn2_get_seg(scb, workspace);
	    if (status)
		break;

	    /* Fall thru to input processing */

	case ADW_GET_DATA:
	case ADW_FLUSH_SEGMENT:
	    /* For this, we just continue processing. */

	    if (!workspace->adw_fip.fip_done)
	    {
		if (first_time)
		{
		    workspace->adw_shared.shd_exp_action = ADW_START;
		    first_time = 0;
		}
		
		status = adc_xform(scb, (PTR)workspace);
		if (DB_FAILURE_MACRO(status))
		    break;
	    }
	    
	    if (    (workspace->adw_shared.shd_exp_action == ADW_FLUSH_SEGMENT)
		||  (scb->adf_errcb.ad_errcode == E_AD7001_ADP_NONEXT)
		||  ((workspace->adw_shared.shd_inp_tag == ADP_P_DATA)
		     && (workspace->adw_fip.fip_l0_value == 0)
		     && (workspace->adw_fip.fip_l1_value == 0)))
	    {
		if (scb->adf_errcb.ad_errcode == E_AD7001_ADP_NONEXT)
		{
		    if (workspace->adw_shared.shd_inp_tag == ADP_P_GCA_L_UNK)
		    {
			I4ASSIGN_MACRO(workspace->adw_shared.shd_l0_check,
				       cpn->per_length0);
			I4ASSIGN_MACRO(workspace->adw_shared.shd_l1_check,
				       cpn->per_length1);
		    }
		    else if (/*(workspace->adw_shared.shd_l0_check !=
			       workspace->adw_fip.fip_l0_value)
			       || */(workspace->adw_shared.shd_l1_check
				     !=  workspace->adw_fip.fip_l1_value))
		    {
			scb->adf_errcb.ad_errcode = E_AD7004_ADP_BAD_BLOB;
			status = E_DB_ERROR;
			break;
		    }
		    workspace->adw_fip.fip_pop_cb.pop_continuation |=
			ADP_C_END_MASK;
		    scb->adf_errcb.ad_errcode = E_AD0000_OK;
		    done = 1;
		    if (/*(workspace->adw_shared.shd_l0_check == 0)
			 && */ (workspace->adw_shared.shd_l1_check == 0))
			no_flush = 1;
		}

		if (!no_flush)
		{
		    status =
			(*Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi)
			    (ADP_PUT, &workspace->adw_fip.fip_pop_cb);
		    if (status)
		    {
			scb->adf_errcb.ad_errcode =
			    workspace->adw_fip.fip_pop_cb.pop_error.err_code;
		    }
		}
		else
		{
		    status = E_DB_OK;
		}
		workspace->adw_fip.fip_pop_cb.pop_continuation = 0;
		workspace->adw_shared.shd_o_used = 0;
	    }
	    else if ( (workspace->adw_shared.shd_inp_tag !=
		       ADP_P_GCA_L_UNK)
		     && (workspace->adw_shared.shd_l1_check >
			 workspace->adw_fip.fip_l1_value))
	    {
		scb->adf_errcb.ad_errcode = E_AD7011_ADP_BAD_CPNIFY_OVER;
		status = E_DB_ERROR;
	    }
	    
	    
	    if (   (status == E_DB_WARN)
		&& (scb->adf_errcb.ad_errcode == E_AD7001_ADP_NONEXT)
		&& (workspace->adw_shared.shd_inp_tag != ADP_P_GCA_L_UNK)
		&& (    (workspace->adw_shared.shd_l0_check != 
			 workspace->adw_fip.fip_l0_value)
		    || (workspace->adw_shared.shd_l1_check != 
			workspace->adw_fip.fip_l1_value)))
	    {
		/* The completion agreement doesn't exist.  Return an error */
		
		scb->adf_errcb.ad_errcode = E_AD700F_ADP_BAD_CPNIFY;
		status = E_DB_ERROR;
	    }
	    break;

	case ADW_CONTINUE:
	default:
	    return(adu_error(scb, E_AD9999_INTERNAL_ERROR, 0));
	    break;
	}
    }    while (   (!done)
	   && (DB_SUCCESS_MACRO(status))
	   && (workspace->adw_shared.shd_i_used <
	    	    	workspace->adw_shared.shd_i_length));

    if (DB_SUCCESS_MACRO(status) && (!done))
    {
	scb->adf_errcb.ad_errcode = E_AD0002_INCOMPLETE;
	status = E_DB_INFO;
    }
    else if (workspace->adw_shared.shd_type < 0)
    {
	/*
	** If nullable and not null (by virtue of getting here),
	** eat the null byte.
	*/

	if (workspace->adw_shared.shd_i_used == 
				workspace->adw_shared.shd_i_length)
	{
	    /* 
	    ** exhausted the input buffer, yet we still have to
	    ** account for the null indicator. get next input buffer
	    */
	    workspace->adw_fip.fip_state = ADW_F_DONE_NEED_NULL;
	    scb->adf_errcb.ad_errcode = E_AD0002_INCOMPLETE;
	    status = E_DB_INFO;
	}
	else
	    workspace->adw_shared.shd_i_used += 1;
    }

    return(status);
}

/*
** {
** Name:       adu_cpn1_init -- Setup workspace and perform initialization
**
** Description:
**      This routine initializes the workspace and the output coupon.
**      
**
** Re-entrancy:
**      This routine is reentrant.  Any and all long-term context is
**      contained in the workspace provided and maintained by the
**      caller.
**
**      The workspace is set up as empty.  The peripheral header for
**      the large object is consumed, and its data is used to set up
**      the workspace for processing the large object into a coupon.
**      If the large object is null, a null coupon is created, and
**      the fip_done indicator is set so that adu_couponify()
**      processing can terminate.  If the entire large object header
**      (plus the null byte if applicable) is present, this routine
**      will return the usual info/incomplete status, indicating that
**      it needs to be recalled.  The fip_state area of the workspace
**      is set to ADW_F_STARTING in that case, indicating the need to
**      complete the setting up operation.
**
** Inputs:
**      scb             The standard ADF session control block
**      continuation    Is this the first call in this context to this
**                      routine
**      lodv            Ptr to DB_DATA_VALUE containing the inline
**                      sections of the large object
**      workspace       Ptr to ADP_LO_WKSP for this object's context
**      cpn_dv          Ptr to DB_DATA_VALUE in which the coupon is to
**                      be constructed.
** Outputs:
**      scb->adf_errcb  Filled as appropriate
**      workspace       Modified for use by this routine.  Must be
**                      preserved over calls to this routine as long
**                      as continuation is set.
**      workspace->adw_shared.shd_i_used
**                      Set on last call describing how much of the
**                      input was used.  This is necessary since the
**                      caller cannot necessarily determine where the
**                      large object ends -- since the large object's
**                      length is not specified in the DB_DATA_VALUE
**                      which accompanies it.
**
** Returns:
**      E_DB_ERROR      When something went wrong
**      E_DB_INFO/E_AD0002_INCOMPLETE
**                      When more data is needed
**      E_DB_OK         When operation is complete.
**	<function return values>
**
** Exceptions: (if any)
**	None.
**
** Side Effects: 
**      None.
**
** History:
**      11-Dec-1992 (fred)
**          Created.
**      12-Oct-1993 (fred)
**          Removed call to ADP_INFORMATION -- subsumed by
**          adi_per_under(). 
**	7-apr-1998 (thaju02) x-integ of oping12 change 435499.
**	    bug #89327, 89329 - in the GCA byte stream, the
**	    ADP header (12 bytes) is followed by 4 bytes (due to the 8-byte
**	    word alignment) containing the segment indicator followed by the 
**	    8-byte aligned coupon. Nullable blobs are null if the periph 
**	    hdr denotes zero length, the segment indicator is 0, and 
**	    the null indicator is set.
**	29-Oct-2001 (thaju02)
**	    Get seg_ind value from workspace->adw_fip.fip_periph_hdr rather
**	    than lodv->db_data, because peripheral header may span over two
**	    lodv buffers. (B105406) 
**	23-apr-2003 (devjo01)
**	    Peripheral data in input stream may not match layout of
**	    ADP_PERIPHERAL on 64 bit platforms due to alignment padding.
**	    (b110124).
*/
DB_STATUS static
adu_cpn1_init(ADF_CB	    *scb,
	      i4            continuation,
	      DB_DATA_VALUE *lodv,
	      ADP_LO_WKSP   *workspace,
	      DB_DATA_VALUE *cpn_dv)
{
    DB_STATUS           status = E_DB_OK;
    DB_DT_ID            dtid;
    i4                  adjustment;
    struct _spi_struct {
	i4		tag;
	i4		len0;
	i4		len1;
	i4		more;
	char		null;
    }	*pspi; /* Pointer to stream peripheral image */
    ADP_PERIPHERAL	*cpn = (ADP_PERIPHERAL *) cpn_dv->db_data;

    if (lodv->db_datatype != cpn_dv->db_datatype)
	return(adu_error(scb, E_AD9999_INTERNAL_ERROR, 0));
    
    dtid = ADI_DT_MAP_MACRO(abs(lodv->db_datatype));
    if (    dtid <= 0
	|| dtid  > ADI_MXDTS
	|| Adf_globs->Adi_dtptrs[dtid] == NULL
	|| !Adf_globs->Adi_dtptrs[dtid]->adi_under_dt
	)
	return(adu_error(scb, E_AD2004_BAD_DTID, 0));
    
    if (Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi == NULL)
	return(adu_error(scb, E_AD9999_INTERNAL_ERROR, 0));
    
    if (!continuation)
    {
	workspace->adw_shared.shd_exp_action = ADW_START;
	workspace->adw_fip.fip_done = FALSE;
	workspace->adw_fip.fip_seg_needed = 0;
	workspace->adw_fip.fip_state = ADW_F_STARTING;
	workspace->adw_fip.fip_l0_value = 0;
    }
    
    adjustment = ADP_HDR_SIZE - workspace->adw_fip.fip_l0_value;
    if (lodv->db_datatype < 0)
	adjustment = ADP_NULL_PERIPH_SIZE - workspace->adw_fip.fip_l0_value;
		
    if (lodv->db_length >= adjustment)
    {
	MEcopy(lodv->db_data,
			 adjustment,
			 (((char *) &workspace->adw_fip.fip_periph_hdr) +
				   workspace->adw_fip.fip_l0_value));
	workspace->adw_fip.fip_state = ADW_F_RUNNING;
    }
    else
    {
	/* Can't get entire first part */
	MEcopy(lodv->db_data,
			 lodv->db_length,
			 (((char *) &workspace->adw_fip.fip_periph_hdr) +
				   workspace->adw_fip.fip_l0_value));
	workspace->adw_fip.fip_l0_value += lodv->db_length;
	workspace->adw_fip.fip_state = ADW_F_STARTING;
    }

    if (workspace->adw_fip.fip_state == ADW_F_RUNNING)
    {
	workspace->adw_shared.shd_type = lodv->db_datatype;
	workspace->adw_shared.shd_i_area = (char *) lodv->db_data;
	workspace->adw_shared.shd_i_length = lodv->db_length;

	I4ASSIGN_MACRO(workspace->adw_fip.fip_periph_hdr.per_tag,
		                                          cpn->per_tag);
	I4ASSIGN_MACRO(workspace->adw_fip.fip_periph_hdr.per_length0,
		                                          cpn->per_length0);
	I4ASSIGN_MACRO(workspace->adw_fip.fip_periph_hdr.per_length1,
		                                          cpn->per_length1);

	workspace->adw_shared.shd_inp_tag =
	    workspace->adw_fip.fip_periph_hdr.per_tag; 
	workspace->adw_fip.fip_l0_value =
	    workspace->adw_fip.fip_periph_hdr.per_length0;
	workspace->adw_fip.fip_l1_value =
	    workspace->adw_fip.fip_periph_hdr.per_length1;
	workspace->adw_shared.shd_l0_check =
	    workspace->adw_shared.shd_l1_check = 0;

	continuation = 0;	/* adc_partition() hasn't yet been called */
        workspace->adw_shared.shd_out_tag = ADP_P_COUPON;
	I4ASSIGN_MACRO(workspace->adw_shared.shd_out_tag, cpn->per_tag);
	workspace->adw_shared.shd_out_segmented = TRUE;
	if (workspace->adw_shared.shd_inp_tag != ADP_P_DATA)
	    workspace->adw_shared.shd_inp_segmented = TRUE;
	else
	    workspace->adw_shared.shd_inp_segmented = FALSE;
	
	if (lodv->db_datatype < 0)
	{
	    pspi = (struct _spi_struct *)&(workspace->adw_fip.fip_periph_hdr);
	    if (  (pspi->len0 == 0)
	       && (pspi->len1 == 0)
	       && (pspi->more == 0)
	       && (pspi->null != 0) )
	    {
		/* Then blob is NULL.  Make coupon the same. */
		
		ADF_SETNULL_MACRO(cpn_dv);
	    }
	    else
	    {
		/* Then the blob is not null.  Clear up this right away */
		
		ADF_CLRNULL_MACRO(cpn_dv);
		workspace->adw_shared.shd_exp_action = ADW_GET_DATA;
		
		workspace->adw_fip.fip_seg_ind = pspi->more;
	    }

	    if (pspi->more == 0)
	    {
		workspace->adw_shared.shd_i_used = adjustment;
		workspace->adw_fip.fip_done = TRUE;
		scb->adf_errcb.ad_errcode = E_DB_OK;
		return(E_DB_OK);
	    }
	    
	    adjustment -= 1;
	}
	workspace->adw_shared.shd_i_used = adjustment;

	if (workspace->adw_shared.shd_inp_tag == ADP_P_GCA_L_UNK)
	{
	    workspace->adw_fip.fip_l0_value =
		workspace->adw_fip.fip_l1_value = 0;
	}
	
	if ((workspace->adw_fip.fip_l0_value != 0)
	    ||	(   (workspace->adw_shared.shd_inp_tag != ADP_P_DATA)
		 &&  (workspace->adw_shared.shd_inp_tag != ADP_P_GCA)
		 &&  (workspace->adw_shared.shd_inp_tag
		                            != ADP_P_GCA_L_UNK)))
	    
	{
	    scb->adf_errcb.ad_errcode = E_AD7004_ADP_BAD_BLOB;
	    return(E_DB_ERROR);
	}
	
	workspace->adw_fip.fip_pop_cb.pop_type = (ADP_POP_TYPE);
	workspace->adw_fip.fip_pop_cb.pop_length =
	    sizeof(workspace->adw_fip.fip_pop_cb);
	workspace->adw_fip.fip_pop_cb.pop_ascii_id = ADP_POP_ASCII_ID;
	workspace->adw_fip.fip_pop_cb.pop_coupon = cpn_dv;
	workspace->adw_fip.fip_pop_cb.pop_segment =
	    &workspace->adw_fip.fip_seg_dv;
	workspace->adw_fip.fip_pop_cb.pop_underdv =
	    &workspace->adw_fip.fip_under_dv;
	
	/*  SET BY CALLER ???
	    	workspace->adw_pop_cb.pop_temporary = ADP_POP_TEMPORARY; */
	    
	    workspace->adw_fip.fip_pop_cb.pop_continuation = ADP_C_BEGIN_MASK;
	workspace->adw_fip.fip_pop_cb.pop_user_arg = 0;
	status = adi_per_under(scb,
			       lodv->db_datatype,
			       &workspace->adw_fip.fip_under_dv);
	if (status)
	{
	    return(adu_error(scb, E_AD7000_ADP_BAD_INFO, 0));
	}
	
	STRUCT_ASSIGN_MACRO(workspace->adw_fip.fip_under_dv,
			    workspace->adw_fip.fip_seg_dv);
	workspace->adw_fip.fip_seg_dv.db_data =
	    workspace->adw_shared.shd_o_area = 
		workspace->adw_fip.fip_work_dv.db_data;
	
	workspace->adw_fip.fip_work_dv.db_datatype =
	    workspace->adw_fip.fip_under_dv.db_datatype;
	workspace->adw_fip.fip_work_dv.db_prec =
	    workspace->adw_fip.fip_under_dv.db_prec;
	if (workspace->adw_fip.fip_work_dv.db_length <
	    workspace->adw_fip.fip_seg_dv.db_length)
	{
	    workspace->adw_fip.fip_seg_dv.db_length =
		workspace->adw_fip.fip_work_dv.db_length;
	}
	workspace->adw_shared.shd_o_length =
	    workspace->adw_fip.fip_seg_dv.db_length;
	workspace->adw_shared.shd_o_used = 0;
    }
    else
    {
	/* Then we didn't get enough.  Leave early */
	
	scb->adf_errcb.ad_errcode = E_AD0002_INCOMPLETE;
	status = E_DB_INFO;
    }
    return(status);
}

/*{
** Name:       adu_cpn2_get_segment -- Prepare for next segment
**
** Description:
**      This routine examines the large object to determine if there
**      another segment to get.  If the object is a GCA style object,
**      then this routine consumes the segment indicator
**      (appropriately updating the workspace control area) and tests
**      it for the presence of another segment.  If the object is not
**      a GCA style, then the presence of another segment is
**      determined by whether the overall routine has consumed a
**      length equivalent to the length of the large object yet.
**
**      If there is another segment, then the
**      workspace->adw_fip.fip_done indicator remains false;  if there
**      are no more, it is set to true.
**
**      If there is not enough data to determine the presense or
**      absence of another segment, this routine returns the
**      info/incomplete status and expects to be recalled with more
**      data.  Any data it is consumed so far is stored in the
**      workspace.
**
** Re-entrancy:
**      This routine is reentrant.  Any and all long-term context is
**      contained in the workspace provided and maintained by the caller.
**
** Inputs:
**      scb             The standard ADF session control block
**      workspace       Ptr to ADP_LO_WKSP for this object's context
**
** Outputs:
**      scb->adf_errcb  Filled as appropriate
**                      Set to E_AD7001_ADP_NONEXT if there is output
**                      to flush
**      workspace->adw_fip.fip_done
**                      Set if there are no more segments.
**
** Returns:
**      E_DB_ERROR      When something went wrong
**      E_DB_INFO/E_AD0002_INCOMPLETE
**                      When more data is needed
**      E_DB_OK         When operation is complete.
**	<function return values>
**
** Exceptions: (if any)
**	None.
**
** Side Effects: 
**      None.
**
** History:
**      11-Dec-1992 (fred)
**          Created.
*/
DB_STATUS static
adu_cpn2_get_seg(ADF_CB      *scb,
		 ADP_LO_WKSP *workspace)
{
    DB_STATUS     status = E_DB_OK;
    i4            to_move;
    char          plain_memory[sizeof(workspace->adw_fip.fip_seg_ind)];
    
    scb->adf_errcb.ad_errcode = E_AD0000_OK;

    if ((workspace->adw_shared.shd_inp_tag == ADP_P_GCA_L_UNK) ||
	(workspace->adw_shared.shd_inp_tag == ADP_P_GCA))
    {
	/*
	** In these cases, we must consume the segment indicator.
	** If it is non-zero, then we have a segment to work with,
	** and we should move it into the blob.  If the segment
	** indicator is zero, then we are finished.  Flush the
	** current segment if there is one, and return complete.
	*/
	
	if (workspace->adw_fip.fip_seg_needed == 0)
	{
	    workspace->adw_fip.fip_seg_needed =
		sizeof(workspace->adw_fip.fip_seg_ind);
	    workspace->adw_fip.fip_seg_ind = 0;
	}
	else
	{
	    MEcopy((PTR) &workspace->adw_fip.fip_seg_ind,
			     sizeof(workspace->adw_fip.fip_seg_ind),
			     (PTR) plain_memory);
	}
	
	
	to_move = min(workspace->adw_fip.fip_seg_needed,
		      (workspace->adw_shared.shd_i_length -
		       workspace->adw_shared.shd_i_used));
	if (to_move > 0)
	{
	    MEcopy((PTR) &workspace->adw_shared.shd_i_area[
			            workspace->adw_shared.shd_i_used],
			     to_move,
			     (PTR) &plain_memory[
				      sizeof(workspace->adw_fip.fip_seg_ind) -  
					 workspace->adw_fip.fip_seg_needed]);
	    workspace->adw_fip.fip_seg_needed -= to_move;
	    workspace->adw_shared.shd_i_used += to_move;
	    MEcopy((PTR) plain_memory,
			     sizeof(plain_memory),
			     (PTR)  &workspace->adw_fip.fip_seg_ind);
	}
	
	if (workspace->adw_fip.fip_seg_needed != 0)
	{
	    /* 
	    ** Then we need more data to determine if we are done.  Set the
	    ** status appropriately and return.
	    */
	    scb->adf_errcb.ad_errcode = E_AD0002_INCOMPLETE;
	    status = E_DB_INFO;
	}
	else if (workspace->adw_fip.fip_seg_ind == 0)
	{
	    /* Then we are finished */
	    workspace->adw_fip.fip_done = TRUE;
	}
    }
    else
    {
	/*
	** Otherwise, we are in a non-marked type.  For these, we test for
	** completion by seeing if the lengths match yet...
	*/

	if (workspace->adw_shared.shd_l1_check ==
	    workspace->adw_fip.fip_l1_value)
	{
	    workspace->adw_fip.fip_done = TRUE;
	}
    }
    
    if (status == E_DB_OK)
    {
	if (workspace->adw_fip.fip_done)
	{
	    /*
	    ** Then we are finished.  We have found the last segment.  In this
	    ** case, we need to see if any output has beed used.  If it has,
	    ** then we set the system up to flush that output without recalling
	    ** the adc_xform() routine, since all the data necessary has already
	    ** been transformed.
	    */

	    scb->adf_errcb.ad_errcode = E_AD7001_ADP_NONEXT;
	}
    }

    return(status);
}
