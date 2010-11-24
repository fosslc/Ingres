/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <me.h>
#include    <st.h>
#include    <tm.h>
#include    <tmtz.h>
#include    <tr.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <adp.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduint.h>
#include    <adftrace.h>

static DB_STATUS
adu_opz_skip(ADF_CB		*adf_scb,
		DB_DATA_VALUE	*dv_in,
		ADP_LO_WKSP	*work,
		i4		redeem_offset,
		i4		*seek_offset,
		i4		*seek_segno);

static DB_STATUS
adu_getseg(ADF_CB        *scb,
		ADP_POP_CB	*pop_cb,
		char		**segstart,
		char		**segend,
		i4		*done,
		i4		segno);

/**
** Name:	adulvch.c - Long varchar manipulation routines
**
** Description:
**	This file contains routines which manipulate long varchar type
**      data.
**
**      This file defines:
**
**      adu_0lo_setup_workspace -- (legacy) set's up workspace for ...
**      adu_lo_setup_workspace -- set's up workspace for ...
**	adu_lo_filter -- Moves large object data thru a filter
**                          function
*       adu_lo_delete -- Delete large object [temporary]
**      adu_2lvch_upper_slave -- Uppercase(long varchar) slave function
**      adu_3lvch_upper -- uppercase(long varchar) master function
**      adu_4lvch_lower_slave -- Lowercase(long varchar) slave
**      adu_5lvch_lower -- Lowercase(long varchar)
**      adu_6lvch_left_slave  -- left(long varchar, <number>) slave fcn
**      adu_7lvch_left  -- left(long varchar, <number>)
**      adu_8lvch_right_slave -- right(long varchar, <number>) slave fcn
**      adu_9lvch_right -- right(long varchar, <number>)
**      adu_11lvch_concat -- concat(long varchar, long varchar)
**	adu_long_coerce_slave -- slave to adu_long_coerce
**	adu_long_coerce -- converts one long type to another
**
** History:
**	14-Dec-1992 (fred)
**          Created.
**      13-Apr-1993 (fred)
**          Added long byte datatype support (it is identical -- just
**          less stringent in testing datatypes).
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      26-Jul-1993 (fred)
**          Added new filter function operation.
**      12-Aug-1993 (fred)
**          Added adu_lo_delete() routine.
**	25-aug-93 (ed)
**	    remove dbdbms.h
**      22-Apr-1994 (fred)
**          Added support to adu_free_object() to free objects for a
**          specific query.
**	31-jul-1996 (kch)
**	    In adu_free_object(), we now call dmpe_call() with the new flag
**	    ADP_FREE_NON_SESS_OBJS, to indicate that we do not want to
**	    free the sess temp objects. This change fixes bug 78030.
**      05-Aug-1996 (i4jo01)
**          In adu_7lvch_left, modified to call dmpe_call to check if
**          extension table is closed. Also changed adu_lo_filter to
**          pick up new pcb struct param if available. Add new param to 
**          callers of adu_lo_filer.This change fixes bug #77189.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      27-feb-2002 (stial01)
**          adu_8lvch_right_slave() fixed arg to adu_7strlength for DOUBLEBYTE
**      04-aug-2006 (stial01)
**          Fixed bug in adu_8lvch_right_slave()
**          Added support for substring and position for blobs
**      16-oct-2006 (stial01)
**          Added locator routines.
**      08-dec-2006 (stial01)
**          adu_12lvch_substr_slave() call existing fcns for right/left ops.
**          Added routines for position FROM
**      11-dec-2006 (stial01)
**          Some more fixes for position with start position > 1
**      14-dec-2006 (stial01)
**          Enable optimization to seek to random segment in blob
**      09-feb-2007 (stial01)
**          Added adu_free_locator()
**      17-may-2007 (stial01)
**          adu_free_locator() changed locator parameter to locator_dv 
**      20-jul-2007 (stial01)
**          adu_cpn_to_locator() fixed error handling.
**	21-Mar-2009 (kiria01) SIR121788
**	    Extended adu_lo_filter to cope properly with mismatches
**	    between in-type and out-type and differential data rates due
**	    to data inflation of coercions. Previously, there were
**	    assumptions that the datatype would be the same both in and
**	    out and the ADW_FLUSH_SEGMENT would not work with last segment.
**	    adu_0lo_setup_workspace has been retained for legacy support
**	    but now just calls the extended form adu_lo_setup_workspace.
**      27-Mar-2009 (hanal04) Bug 121857
**          Correct xDEBUG code to use dv_pos. rdv is undeclared.
**	04-Jun-2010 (kiria01) b123879 i144946
**	    Don't return unnecessary errors for i8 values that are outside of
**	    i4 range when later logic would have adjusted the parameter anyway.
**/

/*{
** Name:	adu_lo_setup_workspace - Perform common wksp setup for filters
**
** Description:
**      This routine performs common setup functions for the workspace
**      which will be used by functions which use the adu_lo_filter()
**      method to operate on large objects.
**
**      This routine assumes that each such filter will recieve a
**      workspace followed by space for two segment buffers.  Therefore,
**      it will setup the input and output areas by dividing the space
**      remaining after the workspace between the input and
**      output areas.  This routine does no semantic checking (except to
**      make sure that there is some space remaining) on these values --
**      it assumes that the function instances are correctly annotated
**      to include the relevant information, and that the appropriate
**      memory allocations have already taken place.
**
** Re-entrancy:
**      This function is reentrant.
**
** Inputs:
**      scb                 	    	    Standard ADF SCB
**      dv_lo               	    	    Ptr to DB_DATA_VALUE for
**                                              representing large
**                                              object being filtered.
**      dv_work             	    	    Ptr to DB_DATA_VALUE
**                                              pointing to a workspace
**                                              area
**
** Outputs:
**      scb->adf_errcb      	    	    Filled as appropriate.
**      *dv_work->db_data (the work space area...)
**         ->adw_shared.shd_{i,o}_area
**                          	    	    These  point to buffers
**                                              which this routine can
**                                              use to process the data.
**         ->adw_shared.shd_{i,o}_length
**                          	      	    These give the length the
**                                              their respective area.
**         ->adw_fip.fip_pop_cb             Initialized...
**         ->adw_fip.fip_under_dv           Set up for type specified by
**                                              dv_lo.
**         ->adw_fip.fip_seg_dv,
**         ->adw_fip.fip_work_dv            Set up to be the segments
**                                              for input and output
**                                              respectivly, based on
**                                              the dv_lo provided.
**                                              Their db_data's point to
**                                              the shd_{i,o}_area's
**                                              respectively. 
**
** Returns:
**	DB_STATUS
**
** Exceptions:
**      None.
**
** Side Effects:
**      Not really a side effect, but a warning.  Note that this routine
**	(adu_0lo_setup_workspace) is provided to users (i.e. the address
**	of this routine) as part of the OME package.  Therefore, interface
**	changes to this routine must be carefully considered, as they may
**	have the effect of having users recoding their OME packages.
**
** History:
**	14-Dec-1992 (fred)
**         Created.
**	11-May-2004 (schka24)
**	    Null out pop-info field.
**	19-Mar-2009 (kiria01) SIR121788
**	    Added extended interface for expanding data transfers.
*/
DB_STATUS
adu_0lo_setup_workspace(ADF_CB     	*scb,
			DB_DATA_VALUE   *dv_lo,
			DB_DATA_VALUE 	*dv_work)
{
    return adu_lo_setup_workspace(scb, dv_lo, NULL, dv_work, 2);
}

DB_STATUS
adu_lo_setup_workspace(ADF_CB     	*scb,
			DB_DATA_VALUE   *dv_in,
			DB_DATA_VALUE   *dv_out,
			DB_DATA_VALUE 	*dv_work,
			i4		ws_buf_ratio)
{
    DB_STATUS	status = E_DB_OK;
    i4          length = dv_work->db_length;
    ADP_LO_WKSP	*work = (ADP_LO_WKSP *) dv_work->db_data;
    char	*buffer;
    DB_DT_ID	dtin = ADI_DT_MAP_MACRO(abs(dv_in->db_datatype));
    DB_DT_ID	dtout = dv_out
			? ADI_DT_MAP_MACRO(abs(dv_out->db_datatype))
			: dtin;

    length -= sizeof(ADP_LO_WKSP);
    if (length <= 0 || !work)
	return(adu_error(scb, E_AD9999_INTERNAL_ERROR, 0));
    
    buffer = (char *) work;
    buffer += sizeof(ADP_LO_WKSP);
    
    work->adw_fip.fip_pop_cb.pop_type = ADP_POP_TYPE;
    work->adw_fip.fip_pop_cb.pop_length = sizeof(ADP_POP_CB);
    work->adw_fip.fip_pop_cb.pop_ascii_id = ADP_POP_ASCII_ID;
    work->adw_fip.fip_pop_cb.pop_continuation = ADP_C_BEGIN_MASK;
    work->adw_fip.fip_pop_cb.pop_info = NULL;
    work->adw_fip.fip_pop_cb.pop_underdv =
	&work->adw_fip.fip_under_dv;
    
    work->adw_fip.fip_under_dv.db_datatype =
	Adf_globs->Adi_dtptrs[dtin]->adi_under_dt;
    work->adw_fip.fip_under_dv.db_prec = 0;
    work->adw_fip.fip_under_dv.db_data = 0;
    work->adw_filter_space = (PTR) 0;
    work->adw_dmf_ctx = (PTR)0;
    
    /* Determine the size we can use */
    
    if (status = (*Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi)
		(ADP_INFORMATION, &work->adw_fip.fip_pop_cb))
	return adu_error(scb,
			   work->adw_fip.fip_pop_cb.pop_error.err_code,
			   0);

    length = length / ws_buf_ratio;

    if (length > work->adw_fip.fip_under_dv.db_length)
	length = work->adw_fip.fip_under_dv.db_length;
    else if (length < work->adw_fip.fip_under_dv.db_length)
	return adu_error(scb, E_AD9999_INTERNAL_ERROR, 0);

    work->adw_shared.shd_exp_action = ADW_CONTINUE;
    work->adw_shared.shd_i_area = buffer;
    work->adw_shared.shd_o_area = buffer + length;
    work->adw_shared.shd_i_length = length;
    work->adw_shared.shd_o_length = length;
        	    
        
    work->adw_fip.fip_seg_dv = work->adw_fip.fip_under_dv;
    work->adw_fip.fip_seg_dv.db_data =
			(PTR) work->adw_shared.shd_i_area;

    work->adw_fip.fip_work_dv = work->adw_fip.fip_under_dv;
    work->adw_fip.fip_work_dv.db_datatype =
			Adf_globs->Adi_dtptrs[dtout]->adi_under_dt;
    work->adw_fip.fip_work_dv.db_data = (PTR)(buffer + length);
    work->adw_fip.fip_work_dv.db_length = dv_work->db_length
			- sizeof(ADP_LO_WKSP) - length;

    return(status);
}    

/*{
** Name:	adu_lo_filter - perform activity on a large object
**
** Description:
**	This routine performs an activity on a large object data
**	element.  There are a number of function instances which must
**	run such an object, performing some activity on [a
**	fraction of] each segment found.  Since the code to read, muck
**	with, and write large objects consists primarily of the read,
**	write stuff, we make that common, and then perform the
**	appropriate mucking about on an activity by activity basis.
**
**      This routine is given an input and output datavalue which are
**	assumed to have been type checked by the calling routine.  It
**	then calls the function given with the given arguments, using
**	the calling paradigm specified.  If the paradigm specification
**	is not correct, the results are undefined.  The calling
**	convention is defined as follows:
**
**        status =  (*fcn)(ADF_CB        *scb,
**                         DB_DATA_VALUE *input_segment_dv,
**                         DB_DATA_VALUE *output_segment_dv,
**                         ADP_LO_WKSP   *workspace);
**
**      The activity of this routine is determined from the
**	shd_exp_action value present in the workspace on the return of
**	the called routine.  The interpretation of this value is:
**
**           
**           ADW_CONTINUE:
**                  Dispose of returned segment, get next, call again;
**           ADW_FLUSH_SEGMENT:
**                  Dispose of returned segment, call again (no get next); 
**           ADW_FLUSH_STOP:
**                  Dispose of this segment, then stop;
**           ADW_GET_DATA:
**                  Ignore this segment, get next segment, call again;
**           ADW_STOP:
**                  Ignore this segment, stop;
**
** Re-entrancy:
**      This function is reentrant.
**
** Inputs:
**      scb             Standard ADF SCB
**      dv_in           Ptr to DB_DATA_VALUE pointing to a coupon for
**	                the input large object;
**      dv_out          Ptr to DB_DATA_VALUE pointing to coupon for
** 	                output large object;
**      fcn             Function to be called;
**      workspace       Workspace to be passed to the called function.
**                      Note that unlike the adc_lo_xform() cases, not all
**	                of this space is used.  The
**	                adw_shared.shd_exp_action is used as described
**	                above.  Also
**         ->adw_shared.shd_{i,o}_area
**                      These are expect to point to buffers which this
**	                routine can use to process the data.  Each must
**	                be large enough to hold a segment of data of the
**	                given type.
**         ->adw_shared.shd_{i,o}_length
**                      These give the length the their respective
**                      area.
**      continuation    A mask corresponding to the standard
**                      peripheral continuation indicators.  This mask
**                      indicates whether the this call includes the
**                      begining and/or end of the output coupon.
**                      A value of ADP_C_BEGIN_MASK only means that
**                      this is the first call for this output value,
**                      but that more will be coming.  Similarly, a
**                      value of ADP_C_END_MASK indicates that this is
**                      not the beginning, but that it is the last.
**                      The "normal" value for this mask is
**                      (ADP_C_BEGIN_MASK | ADP_C_END_MASK).
**
** Outputs:
**      scb->adf_errcb  Filled as appropriate.
**      *dv_out->db_data Filled with a valid coupon;
**
** Returns:
**	DB_STATUS
**
** Exceptions:
**      None.
**
** Side Effects:
**      Not really a side effect, but a warning.  Note that this routine
**	is provided to users (i.e. the address of this routine) as part
**	of the OME package.  Therefore, interface changes to this
**	routine must be carefully considered, as they may have the
**	effect of having users recoding their OME packages.
**
** History:
**	14-Dec-1992 (fred)
**         Created.
**      26-Jul-1993 (fred)
**          Added new filter function operation.  Adjusted operations
**          in support of distinction between ADW_FLUSH_SEGMENT and
**          ADW_FLUSH_STOP.
**	10-may-99 (stephenb)
**	    Add call to cleanup DMPE memory when we terminate get sequence.
**	    This is needed if we didn't get all segments.
*/
DB_STATUS
adu_lo_filter(ADF_CB         *scb,
	      DB_DATA_VALUE  *dv_in,
	      DB_DATA_VALUE  *dv_out,
	      ADP_FILTER_FCN *fcn,
	      ADP_LO_WKSP    *work,
	      i4             continuation,
	      ADP_POP_CB     *pop_cb)
{
    DB_STATUS	status = E_DB_OK;
    bool		done = 0;
    ADP_POP_CB	in_pop_cb;
    ADP_POP_CB	out_pop_cb;
    ADP_POP_CB	*pop_cb_ptr = ( pop_cb ? pop_cb : &in_pop_cb );
    DB_DATA_VALUE under_dv;
    DB_DT_ID	dtid = ADI_DT_MAP_MACRO(abs(dv_out->db_datatype));
    DB_STATUS	(*fcn_fexi)() = NULL;

    /* Make sure parameters are OK */
    if (!(Adf_globs->Adi_dtptrs[dtid]->adi_dtstat_bits & AD_PERIPHERAL))
	return adu_error(scb, E_AD9999_INTERNAL_ERROR, 0);

    if (status = adi_per_under(scb, dv_out->db_datatype, &under_dv))
	return status;

    /*
    ** Now that things are basically doable, initialize the pop_cb's
    */

    *pop_cb_ptr = work->adw_fip.fip_pop_cb;
    out_pop_cb = work->adw_fip.fip_pop_cb;

    pop_cb_ptr->pop_segment = &work->adw_fip.fip_seg_dv;
    out_pop_cb.pop_segment = &work->adw_fip.fip_work_dv;

    pop_cb_ptr->pop_coupon = dv_in;
    out_pop_cb.pop_coupon = dv_out;
    out_pop_cb.pop_temporary =  ADP_POP_TEMPORARY;
    out_pop_cb.pop_continuation = ADP_C_BEGIN_MASK & continuation;

    out_pop_cb.pop_user_arg = work->adw_dmf_ctx;
    out_pop_cb.pop_underdv = &under_dv;
	under_dv.db_data = 0;

    ((ADP_PERIPHERAL *) dv_out->db_data)->per_tag = ADP_P_COUPON;

    /* Get dispatch function address */
    fcn_fexi = Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi;
    
    while (status == E_DB_OK &&
	!(done && work->adw_shared.shd_exp_action != ADW_FLUSH_SEGMENT))
    {
	if (work->adw_shared.shd_exp_action != ADW_FLUSH_SEGMENT)
	{
	    if (status = (*fcn_fexi)(ADP_GET, pop_cb_ptr))
	    {
		if (DB_FAILURE_MACRO(status) ||
			(pop_cb_ptr->pop_error.err_code != E_AD7001_ADP_NONEXT))
		    return adu_error(scb, pop_cb_ptr->pop_error.err_code, 0);

		if (pop_cb_ptr->pop_error.err_code == E_AD7001_ADP_NONEXT)
		{
		    status = E_DB_OK;
		    done = TRUE;
		}
	    }
	    pop_cb_ptr->pop_continuation &= ~ADP_C_BEGIN_MASK;

	    /*
	    ** Setup for adu_lo_filter may have set ADP_C_RANDOM_MASK
	    ** (and pop_segno1). After we get first/random segment
	    ** unset ADP_C_RANDOM_MASK
	    */
	    pop_cb_ptr->pop_continuation &= ~ADP_C_RANDOM_MASK;
	}

	/* Reset to full available length */
	out_pop_cb.pop_segment->db_length =
			out_pop_cb.pop_underdv->db_length;
	if (status = (*fcn)(scb, pop_cb_ptr->pop_segment,
				out_pop_cb.pop_segment, work))
	    return status;

	if (work->adw_shared.shd_exp_action == ADW_FLUSH_STOP ||
		work->adw_shared.shd_exp_action == ADW_STOP)
	    done = TRUE;

	if (done && work->adw_shared.shd_exp_action != ADW_FLUSH_SEGMENT)
	    out_pop_cb.pop_continuation |= ADP_C_END_MASK & continuation;
        
	if (work->adw_shared.shd_exp_action == ADW_CONTINUE ||
	    work->adw_shared.shd_exp_action == ADW_FLUSH_STOP ||
	    work->adw_shared.shd_exp_action == ADW_FLUSH_SEGMENT)
	{
	    status = (*fcn_fexi)(ADP_PUT, &out_pop_cb);
	    out_pop_cb.pop_continuation &= ~ADP_C_BEGIN_MASK;
	    if (status)
		return adu_error(scb, out_pop_cb.pop_error.err_code, 0);
	}
    }
    /* clean up DMF memory */
    status = (*fcn_fexi)(ADP_CLEANUP, pop_cb_ptr);

    work->adw_dmf_ctx = out_pop_cb.pop_user_arg; /* save in case */
						      /* needed */

    return(status);
}

/*{
** Name:	adu_lo_delete -- Delete large object temporary
**
** Description:
**      This routine calls the underlying function to delete the 
**      large object passed in.  It is used only to delete temporary files.
**
**	(schka24) As of May 2004, this routine appears to be unused...
**
** Re-entrancy:
**      This function is reentrant.
**
** Inputs:
**      scb                 	    	    Standard ADF SCB
**      cpn                                 Pointer to peripheral 
**          	    	    	    	        object (coupon).
** Outputs:
**      scb->adf_errcb      	    	    Filled as appropriate.
**
** Returns:
**	DB_STATUS
**
** Exceptions:
**      None.
**
** Side Effects:
**      None.
**
** History:
**	12-Aug-1993 (fred)
**         Created.
*/
DB_STATUS
adu_lo_delete(ADF_CB     	*scb,
	      DB_DATA_VALUE     *dbdv)
{
    ADP_POP_CB        pop_cb;
    DB_STATUS         status;

    pop_cb.pop_type = ADP_POP_TYPE;
    pop_cb.pop_length = sizeof(ADP_POP_CB);
    pop_cb.pop_ascii_id = ADP_POP_ASCII_ID;
    pop_cb.pop_continuation = ADP_C_BEGIN_MASK;
    pop_cb.pop_coupon = dbdv;

    status = (*Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi)
	                                           (ADP_DELETE, &pop_cb);
    if (DB_FAILURE_MACRO(status))
    {
	status = adu_error(scb, pop_cb.pop_error.err_code, 0);
    }
    return(status);
}

/*
** {
** Name:	adu_2lvch_upper_slave - Perform segment by segment uppercase
**
** Description:
**	This routine simply provides the slave filter function for the
**      uppercasing of long varchar objects.  For each segment with
**      which it is called, it calls the normal character string
**      uppercase() function to perform that operation on that segment.
**      It then sets the next expected action (shd_exp_action) field to
**      ADW_CONTINUE, since the uppercase function will always traverse
**      the entire long varchar.
**
**      This routine exists separately from the normal uppercase()
**      function because its calling convention is slightly different.
**      If this proves to be a problem, then fixing the filter function
**      thru which it is called to have a separate calling convention
**      for these types of functions is the obvious enhancement.
**
** Re-entrancy:
**      Totally.
**
** Inputs:
**      scb                              ADF session control block.
**      dv_in                            Ptr to DB_DATA_VALUE describing
**                                       input
**      dv_out                           Ptr to DB_DATA_VALUE describing
**                                       output area.
**      work                             Ptr to ADP_LO_WKSP workspace.
**
** Outputs:
**      scb->adf_errcb                   Filled as appropriate.
**      dv_out->db_length                Set correctly.
**      work->adw_shared.shd_exp_action  Always set to ADW_CONTINUE.
**
** Returns:
**	DB_STATUS
**
** Exceptions:
**      None.
**
** Side Effects: 
**      None.
**
** History:
**	15-Dec-1992 (fred)
**         Created.
**	09-Mar-2007 (gupsh01)
**	   Added support for long nvarchar.
*/
static DB_STATUS
adu_2lvch_upper_slave(ADF_CB	    *scb,
		      DB_DATA_VALUE *dv_in,
		      DB_DATA_VALUE *dv_out,
		      ADP_LO_WKSP   *work)
{
    DB_STATUS	    	status;

    if (dv_in->db_datatype == DB_NVCHR_TYPE)
      status = adu_nvchr_toupper(scb, dv_in, dv_out);
    else
      status = adu_15strupper(scb, dv_in, dv_out);
    work->adw_shared.shd_exp_action = ADW_CONTINUE;
    return(status);
}

/*{
** Name:	adu_3lvch_upper - Perform Uppercase(Long Varchar)
**
** Description:
**      This routine sets up the general purpose large object filter to
**      have it filter a long varchar field to uppercase said object.
**      It will set up the coupons, then call the general filter
**      function which will arrange to uppercase each of the [varchar]
**      segments, thus uppercasing the entire long varchar object.
**
**      This routine makes use of the static adu_2lvch_upper_slave()
**      routine also defined in this file.  It is thru this slave
**      routine that the work actually gets done.
**
** Re-entrancy:
**      Totally.
**
** Inputs:
**      scb                              ADF session control block.
**      dv_in                            Ptr to DB_DATA_VALUE describing
**                                       input coupon
**      dv_work                          Ptr to DB_DATA_VALUE which
**                                       points to an area large enough
**                                       to hold an ADP_LO_WKSP plus 2
**                                       segments (for input & output).
**      dv_out                           Ptr to DB_DATA_VALUE describing
**                                       output area in which the output
**                                       coupon will be created.
**
** Outputs:
**      scb->adf_errcb                   Filled as appropriate.
**      *dv_out->db_data                 Defines the output coupon
**
** Returns:
**	DB_STATUS
**
** Exceptions:
**      None.
**
** Side Effects: 
**      None.
**
** History:
**	15-Dec-1992 (fred)
**         Created.
*/
DB_STATUS
adu_3lvch_upper(ADF_CB      	*scb,
		DB_DATA_VALUE  	*dv_in,
		DB_DATA_VALUE   *dv_work,
		DB_DATA_VALUE   *dv_out)
{
    DB_STATUS              status = E_DB_OK;
    ADP_PERIPHERAL         *inp_cpn = (ADP_PERIPHERAL *) dv_in->db_data;
    ADP_PERIPHERAL         *out_cpn = (ADP_PERIPHERAL *) dv_out->db_data;
    ADP_LO_WKSP            *work = (ADP_LO_WKSP *) dv_work->db_data;

    for (;;)
    {
	if ((inp_cpn->per_length0 != 0) || (inp_cpn->per_length1 != 0))
	{
	    status = adu_0lo_setup_workspace(scb, dv_in, dv_work);
	    if (status)
		break;
	    out_cpn->per_length0 = inp_cpn->per_length0;
	    out_cpn->per_length1 = inp_cpn->per_length1;
	    
	    status = adu_lo_filter(scb,
				   dv_in,
				   dv_out,
				   adu_2lvch_upper_slave,
				   work,
				   (ADP_C_BEGIN_MASK | ADP_C_END_MASK),
                                   NULL);
	}
	else
	{
	    STRUCT_ASSIGN_MACRO(*inp_cpn, *out_cpn);
	}
	break;
    }
    return(status);
}

/*{
** Name:	adu_4lvch_lower_slave - Perform segment by segment lowercase
**
** Description:
**	This routine simply provides the slave filter function for the
**      lowercasing of long varchar objects.  For each segment with
**      which it is called, it calls the normal character string
**      lowercase() function to perform that operation on that segment.
**      It then sets the next expected action (shd_exp_action) field to
**      ADW_CONTINUE, since the lowercase function will always traverse
**      the entire long varchar.
**
**      This routine exists separately from the normal lowercase()
**      function because its calling convention is slightly different.
**      If this proves to be a problem, then fixing the filter function
**      thru which it is called to have a separate calling convention
**      for these types of functions is the obvious enhancement.
**
** Re-entrancy:
**      Totally.
**
** Inputs:
**      scb                              ADF session control block.
**      dv_in                            Ptr to DB_DATA_VALUE describing
**                                       input
**      dv_out                           Ptr to DB_DATA_VALUE describing
**                                       output area.
**      work                             Ptr to ADP_LO_WKSP workspace.
**
** Outputs:
**      scb->adf_errcb                   Filled as appropriate.
**      dv_out->db_length                Set correctly.
**      work->adw_shared.shd_exp_action  Always set to ADW_CONTINUE.
**
** Returns:
**	DB_STATUS
**
** Exceptions:
**      None.
**
** Side Effects: 
**      None.
**
** History:
**	15-Dec-1992 (fred)
**         Created.
**	09-Mar-2007 (gupsh01)
**	   Added support for long nvarchar.
*/
static DB_STATUS
adu_4lvch_lower_slave(ADF_CB	    *scb,
		      DB_DATA_VALUE *dv_in,
		      DB_DATA_VALUE *dv_out,
		      ADP_LO_WKSP   *work)
{
    DB_STATUS	    	status;

    if (dv_in->db_datatype == DB_NVCHR_TYPE)
      status = adu_nvchr_tolower(scb, dv_in, dv_out);
    else
      status = adu_9strlower(scb, dv_in, dv_out);
    work->adw_shared.shd_exp_action = ADW_CONTINUE;
    return(status);
}

/*{
** Name:	adu_5lvch_lower - Perform Lowercase(Long Varchar)
**
** Description:
**      This routine sets up the general purpose large object filter to
**      have it filter a long varchar field to lowercase said object.
**      It will set up the coupons, then call the general filter
**      function which will arrange to lowercase each of the [varchar]
**      segments, thus lowercasing the entire long varchar object.
**
**      This routine makes use of the static adu_4lvch_lower_slave()
**      routine also defined in this file.  It is thru this slave
**      routine that the work actually gets done.
**
** Re-entrancy:
**      Totally.
**
** Inputs:
**      scb                              ADF session control block.
**      dv_in                            Ptr to DB_DATA_VALUE describing
**                                       input coupon
**      dv_work                          Ptr to DB_DATA_VALUE which
**                                       points to an area large enough
**                                       to hold an ADP_LO_WKSP plus 2
**                                       segments (for input & output).
**      dv_out                           Ptr to DB_DATA_VALUE describing
**                                       output area in which the output
**                                       coupon will be created.
**
** Outputs:
**      scb->adf_errcb                   Filled as appropriate.
**      *dv_out->db_data                 Defines the output coupon
**
** Returns:
**	DB_STATUS
**
** Exceptions:
**      None.
**
** Side Effects: 
**      None.
**
** History:
**	15-Dec-1992 (fred)
**         Created.
*/
DB_STATUS
adu_5lvch_lower(ADF_CB      	*scb,
		DB_DATA_VALUE  	*dv_in,
		DB_DATA_VALUE   *dv_work,
		DB_DATA_VALUE   *dv_out)
{
    DB_STATUS              status = E_DB_OK;
    ADP_PERIPHERAL         *inp_cpn = (ADP_PERIPHERAL *) dv_in->db_data;
    ADP_PERIPHERAL         *out_cpn = (ADP_PERIPHERAL *) dv_out->db_data;
    ADP_LO_WKSP            *work = (ADP_LO_WKSP *) dv_work->db_data;

    for (;;)
    {
	if ((inp_cpn->per_length0 != 0) || (inp_cpn->per_length1 != 0))
	{
	    status = adu_0lo_setup_workspace(scb, dv_in, dv_work);
	    if (status)
		break;
	    out_cpn->per_length0 = inp_cpn->per_length0;
	    out_cpn->per_length1 = inp_cpn->per_length1;
	    
	    status = adu_lo_filter(scb,
				   dv_in,
				   dv_out,
				   adu_4lvch_lower_slave,
				   work,
				   (ADP_C_BEGIN_MASK | ADP_C_END_MASK),
                                   NULL);
	}
	else
	{
	    STRUCT_ASSIGN_MACRO(*inp_cpn, *out_cpn);
	}
	break;
    }
    return(status);
}

/*{
** Name:	adu_6lvch_left_slave - Perform segment by segment left
**
** Description:
**	This routine simply provides the slave filter function for the
**      left-ing of long varchar objects.  For each segment with
**      which it is called, it subtracts the length of that segment from
**      the work->adw_adc.adc_longs[ADC_L_LRCOUNT] field.
**      
**      If that field is positive and non-zero, then the filter function
**      is told to continue -- i.e. include that segment, and go get
**      another.
**      
**      When that field becomes non-positive, then the enough data to
**      satisfy the left function has been seen.  We know that this will
**      be the last segment, so we set the exp_action field to mark that
**      this segment should be included, but no more processing is
**      necessary.  If the field is zero, then that's it -- we need this
**      entire segment, but only this segment (and the ones before...).
**      
**      If the field is negative, then we need to truncate the current
**      segment.  To do this, we simply  use the normal varchar left
**      function to create left(current segment, number_of_chars_to_satisfy).
**
**      In either case, we update the current
**      adw_shared.shd_l{0,1}_check fields, adding the length of the
**      current segment.  This will be used by the overall parent
**      routine to correctly specify the length for the output coupon.
**
** Re-entrancy:
**      Totally.
**
** Inputs:
**      scb                              ADF session control block.
**      dv_in                            Ptr to DB_DATA_VALUE describing
**                                       input
**      dv_out                           Ptr to DB_DATA_VALUE describing
**                                       output area.
**      work                             Ptr to ADP_LO_WKSP workspace.
**          ->adw_shared.shd_l{0,1}_check
**                                       Length of blob seen so far
**          ->adw_adc.adc_longs[ADC_L_LRCOUNT]
**                                       Number of characters we still
**                                       need to finish the function.
**
** Outputs:
**      scb->adf_errcb                   Filled as appropriate.
**      dv_out->db_length                Set correctly.
**      work->adw_shared.shd_exp_action  ADW_CONTINUE if not done yet,
**                                       ADW_FLUSH_STOP when complete.
**          ->adw_shared.shd_l{0,1}_check
**                                       Length of blob seen so far
**          ->adw_adc.adc_longs[ADC_L_LRCOUNT]
**                                       Number of characters we still
**                                       need to finish the function.
**
** Returns:
**	DB_STATUS
**
** Exceptions:
**      None.
**
** Side Effects: 
**      None.
**
** History:
**	15-Dec-1992 (fred)
**         Created.
**	09-may-2007 (gupsh01)
**	   Added support for UTF8.
**	13-Oct-2010 (thaju02) B124469
**	    Cpn's per_length is in byte units. Adu_redeem is expecting 
**	    length in bytes. For UTF8, tally bytes and push byte count into 
**	    cpn for adu_redeem.
*/
static DB_STATUS
adu_6lvch_left_slave(ADF_CB	    *scb,
		     DB_DATA_VALUE *dv_in,
		     DB_DATA_VALUE *dv_out,
		     ADP_LO_WKSP   *work)
{
    DB_STATUS        status = E_DB_OK;
    DB_TEXT_STRING   *in_seg = (DB_TEXT_STRING *) dv_in->db_data;
    DB_TEXT_STRING   *out_seg = (DB_TEXT_STRING *) dv_out->db_data;
    DB_DATA_VALUE    count_dv;
    i4               count, byte_count = 0;

    for (;;)
    {
	count_dv.db_data = (PTR) &count;
	count_dv.db_length = sizeof(count);
	count_dv.db_datatype = DB_INT_TYPE;
	count_dv.db_prec = 0;
	
	if ((Adf_globs->Adi_status & ADI_DBLBYTE) ||
            (scb->adf_utf8_flag & AD_UTF8_ENABLED))
	{
	    status = adu_7strlength(scb, dv_in, &count_dv);
	    if (status)
	        break;
	}
	else
	    count = in_seg->db_t_count;
	work->adw_adc.adc_longs[ADW_L_LRCOUNT] -= count;

	if (work->adw_adc.adc_longs[ADW_L_LRCOUNT] <= 0)
	{
	    /*
	    ** At this point, we have all the data we need.
	    ** We now set the count for the last segment to be
	    ** whatever amount we need.  This will be the
	    ** inverse of what is left in "count" -- which is
	    ** negative, so we add it to the count sitting in
	    ** the buffer, effectively subtracting it from
	    ** that number.
	    */
	    
	    work->adw_shared.shd_exp_action = ADW_FLUSH_STOP;
	    
	    count += work->adw_adc.adc_longs[ADW_L_LRCOUNT];
	    status = adu_6strleft(scb,
				  dv_in,
				  &count_dv, 
				  dv_out);
	    if (status)
		break;

	    if ((scb->adf_utf8_flag & AD_UTF8_ENABLED) && 
		(work->adw_fip.fip_under_dv.db_datatype == DB_VCH_TYPE))
	    {
		count_dv.db_data = (PTR) &byte_count;

		if (status = adu_23octetlength(scb, dv_out, &count_dv))
		    break;
		work->adw_adc.adc_longs[ADW_L_CPNBYTES] += byte_count;
	    }
	}
	else /* count of char's still needed > 0 */
	{
	    MECOPY_VAR_MACRO(dv_in->db_data,
			     dv_in->db_length,
			     dv_out->db_data);
	    dv_out->db_length = dv_in->db_length;

	    if ((scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
		(work->adw_fip.fip_under_dv.db_datatype == DB_VCH_TYPE))
	    {
		count_dv.db_data = (PTR) &byte_count;

		if (status = adu_23octetlength(scb, dv_in, &count_dv))
		    break;
		work->adw_adc.adc_longs[ADW_L_CPNBYTES] += byte_count;
	    }

	    work->adw_shared.shd_exp_action = ADW_CONTINUE;
	}
	work->adw_shared.shd_l1_check += count;
	break;
    }
    return(status);
}

/*{
** Name:	adu_7lvch_left - Perform Left(Long Varchar)
**
** Description:
**      This routine sets up the general purpose large object filter to
**      have it filter a long varchar field to perform the "left"
**      function on said object.  It will set up the workspace with
**      initial parameter information,  then call the general filter
**      function which will arrange to get the left n characters from
**      the segments.
**
**      This routine makes use of the static adu_6lvch_left_slave()
**      routine also defined in this file.  It is thru this slave
**      routine that the work actually gets done.
**
**      This routine uses the workspace to pass information to the slave
**      routine.  This routine will initialize the
**      adw_shared.shd_l{0,1}_check areas to zero;  it expects that the
**      slave routine will provide the correct values here for the
**      resultant large object.
**
**      This routine will set the workspace's
**      adw_adc.adc_longs[ADC_L_LRCOUNT] value to be N in left(long
**      varchar, N).  The slave routine will use this value to calculate
**      the left N character of the long varchar.
**
** Re-entrancy:
**      Totally.
**
** Inputs:
**      scb                              ADF session control block.
**      dv_in                            Ptr to DB_DATA_VALUE describing
**                                       input coupon
**      dv_count                         Ptr to DB_DATA_VALUE for the
**                                       input count.  This is assumed to be
**                                       DB_INT_TYPE.
**      dv_work                          Ptr to DB_DATA_VALUE which
**                                       points to an area large enough
**                                       to hold an ADP_LO_WKSP plus 2
**                                       segments (for input & output).
**      dv_out                           Ptr to DB_DATA_VALUE describing
**                                       output area in which the output
**                                       coupon will be created.
**
** Outputs:
**      scb->adf_errcb                   Filled as appropriate.
**      *dv_out->db_data                 Defines the output coupon
**
** Returns:
**	DB_STATUS
**
** Exceptions:
**      None.
**
** Side Effects: 
**      None.
**
** History:
**	16-Dec-1992 (fred)
**         Created.
**	7-May-2004 (schka24)
**	    Remove close-table call added sometime before Steve's
**	    cleanup call was put into the filter function.  Cleanup
**	    does what we want.
**	22-May-2007 (smeke01) b118376
**         Make left cope with negative counts.
**      23-may-2007 (smeke01) b118342/b118344
**	    Work with i8.
**	13-Oct-2010 (thaju02) B124469
**	    Cpn's per_length is in byte units. Adu_redeem is expecting
**	    length in bytes. For UTF8, tally bytes and push byte count into
**	    cpn for adu_redeem.
*/
DB_STATUS
adu_7lvch_left(ADF_CB        *scb,
	       DB_DATA_VALUE *dv_in,
	       DB_DATA_VALUE *dv_count,
	       DB_DATA_VALUE *dv_work,
	       DB_DATA_VALUE *dv_out)
{
    DB_STATUS              status = E_DB_OK;
    ADP_PERIPHERAL         *inp_cpn = (ADP_PERIPHERAL *) dv_in->db_data;
    ADP_PERIPHERAL         *out_cpn = (ADP_PERIPHERAL *) dv_out->db_data;
    ADP_LO_WKSP            *work = (ADP_LO_WKSP *) dv_work->db_data;
    ADP_POP_CB             pop_cb;


    for (;;)
    {
	if ((inp_cpn->per_length0 != 0) || (inp_cpn->per_length1 != 0))
	{
	    status = adu_0lo_setup_workspace(scb, dv_in, dv_work);
	    if (status)
		break;

	    /* get the value of N, the leftmost chars wanted */

	    switch (dv_count->db_length)
	    {
		case 1:
		    work->adw_adc.adc_longs[ADW_L_LRCOUNT] =
			I1_CHECK_MACRO(*(i1 *) dv_count->db_data);
		    if ( work->adw_adc.adc_longs[ADW_L_LRCOUNT] < (i1)0 )
			work->adw_adc.adc_longs[ADW_L_LRCOUNT] = 0;
		    break;
		case 2: 
		    work->adw_adc.adc_longs[ADW_L_LRCOUNT] =
			*(i2 *) dv_count->db_data;
		    if ( work->adw_adc.adc_longs[ADW_L_LRCOUNT] < (i2)0 )
			work->adw_adc.adc_longs[ADW_L_LRCOUNT] = 0;
		    break;
		case 4:
		    work->adw_adc.adc_longs[ADW_L_LRCOUNT] =
			*(i4 *) dv_count->db_data;
		    if ( work->adw_adc.adc_longs[ADW_L_LRCOUNT] < (i4)0 )
			work->adw_adc.adc_longs[ADW_L_LRCOUNT] = 0;
		    break;
		case 8:
		{
		    i8 i8temp  = *(i8 *)dv_count->db_data;

		    /* adc_longs[] is an array of i4s so limit to i4 values as before */
		    if ( i8temp > MAXI4 )
			work->adw_adc.adc_longs[ADW_L_LRCOUNT] = MAXI4;
		    else if ( i8temp < 0 )
			work->adw_adc.adc_longs[ADW_L_LRCOUNT] = 0;
		    else
			work->adw_adc.adc_longs[ADW_L_LRCOUNT] = (i4) i8temp;

		    break;
		}
		default:
		    return (adu_error(scb, E_AD9998_INTERNAL_ERROR, 2, 0, "lvch_left count length"));
	    }

	    work->adw_shared.shd_l1_check = 0;
	    work->adw_shared.shd_l0_check = 0;

	    work->adw_adc.adc_longs[ADW_L_CPNBYTES] = 0;

	    status = adu_lo_filter(scb,
				   dv_in,
				   dv_out,
				   adu_6lvch_left_slave,
				   work,
				   (ADP_C_BEGIN_MASK | ADP_C_END_MASK),
				   NULL);
	    
	    out_cpn->per_length0 = work->adw_shared.shd_l0_check;
	    out_cpn->per_length1 = work->adw_shared.shd_l1_check;

	    if ((scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
		(work->adw_fip.fip_under_dv.db_datatype == DB_VCH_TYPE))
		out_cpn->per_length1 = work->adw_adc.adc_longs[ADW_L_CPNBYTES];
	}
	else
	{
	    STRUCT_ASSIGN_MACRO(*inp_cpn, *out_cpn);
	}
	break;
    }

    return(status);
}

/*{
** Name:	adu_8lvch_right_slave - Perform segment by segment right
**
** Description:
**	This routine simply provides the slave filter function for the
**      left-ing of long varchar objects.  For each segment with
**      which it is called, it subtracts the length of that segment from
**      the work->adw_adc.adc_longs[ADC_L_LRCOUNT] field.
**      
**      If that field is positive and non-zero, then the filter function
**      is told to continue -- i.e. include that segment, and go get
**      another.
**      
**      When that field becomes non-positive, then the enough data to
**      satisfy the left function has been seen.  We know that this will
**      be the last segment, so we set the exp_action field to mark that
**      this segment should be included, but no more processing is
**      necessary.  If the field is zero, then that's it -- we need this
**      entire segment, but only this segment (and the ones before...).
**      
**      If the field is negative, then we need to truncate the current
**      segment.  To do this, we simply  use the normal varchar left
**      function to create left(current segment, number_of_chars_to_satisfy).
**
**      In either case, we update the current
**      adw_shared.shd_l{0,1}_check fields, adding the length of the
**      current segment.  This will be used by the overall parent
**      routine to correctly specify the length for the output coupon.
**
** Re-entrancy:
**      Totally.
**
** Inputs:
**      scb                              ADF session control block.
**      dv_in                            Ptr to DB_DATA_VALUE describing
**                                       input
**      dv_out                           Ptr to DB_DATA_VALUE describing
**                                       output area.
**      work                             Ptr to ADP_LO_WKSP workspace.
**          ->adw_shared.shd_l{0,1}_check
**                                       Length of blob seen so far
**          ->adw_adc.adc_longs[ADC_L_LRCOUNT]
**                                       Number of characters we still
**                                       need to start the function.
**
** Outputs:
**      scb->adf_errcb                   Filled as appropriate.
**      dv_out->db_length                Set correctly.
**      work->adw_shared.shd_exp_action  ADW_GET_DATA if not done yet,
**                                       ADW_CONTINUE in satifying it...
**          ->adw_shared.shd_l{0,1}_check
**                                       Length of blob seen so far
**          ->adw_adc.adc_longs[ADC_L_LRCOUNT]
**                                       Number of characters we still
**                                       need to start the function.
**
** Returns:
**	DB_STATUS
**
** Exceptions:
**      None.
**
** Side Effects: 
**      None.
**
** History:
**	15-Dec-1992 (fred)
**         Created.
**	09-may-2007 (gupsh01)
**	   Added support for UTF8.
**      13-Oct-2010 (thaju02) B124469
**          Cpn's per_length is in byte units. Adu_redeem is expecting
**          length in bytes. For UTF8, tally bytes and push byte count into
**          cpn for adu_redeem.
*/
static DB_STATUS
adu_8lvch_right_slave(ADF_CB	    *scb,
		      DB_DATA_VALUE *dv_in,
		      DB_DATA_VALUE *dv_out,
		      ADP_LO_WKSP   *work)
{
    DB_STATUS        status = E_DB_OK;
    DB_TEXT_STRING   *in_seg = (DB_TEXT_STRING *) dv_in->db_data;
    DB_TEXT_STRING   *out_seg = (DB_TEXT_STRING *) dv_out->db_data;
    DB_DATA_VALUE    count_dv;
    i4          count, byte_count = 0;
    i4          amount_needed = 0;

    for (;;)
    {
	count_dv.db_data = (PTR) &count;
	count_dv.db_length = sizeof(count);
	count_dv.db_datatype = DB_INT_TYPE;
	count_dv.db_prec = 0;
	
	if ((Adf_globs->Adi_status & ADI_DBLBYTE)||
            (scb->adf_utf8_flag & AD_UTF8_ENABLED))
	{
	    status = adu_7strlength(scb, dv_in, &count_dv);
	    if (status)
	    	break;
	}
	else
	    count = in_seg->db_t_count;

	if (work->adw_adc.adc_longs[ADW_L_LRCOUNT] > 0)
	{
	    amount_needed = -work->adw_adc.adc_longs[ADW_L_LRCOUNT];
	    work->adw_adc.adc_longs[ADW_L_LRCOUNT] -= count;
	    if (work->adw_adc.adc_longs[ADW_L_LRCOUNT] == 0)
		break;
	}

	if (work->adw_adc.adc_longs[ADW_L_LRCOUNT] <= 0)
	{
	    
	    /*
	    ** At this point, we are at the point which is
	    ** interesting.
	    ** 
	    ** We now know that we need all the sements after this one.
	    ** Furthermore, we know that we need some fraction of this
	    ** one.  If the LRCOUNT field is negative, then we need to
	    ** shift the current segment over by the inverse of that
	    ** many bytes.  The shift operator shift left if value is
	    ** negative, so we simply use that number.
	    */
	    
	    work->adw_shared.shd_exp_action = ADW_CONTINUE;
	    
	    if (work->adw_adc.adc_longs[ADW_L_LRCOUNT] < 0)
	    {
		count = amount_needed;
		work->adw_adc.adc_longs[ADW_L_LRCOUNT] = 0;
		status = adu_11strshift(scb,
				      dv_in,
				      &count_dv, 
				      dv_out);
		if (status)
		    break;
		if ((Adf_globs->Adi_status & ADI_DBLBYTE)||
            	    (scb->adf_utf8_flag & AD_UTF8_ENABLED))
		{
		    status = adu_7strlength(scb, dv_out, &count_dv);
		    if (status)
		        break;

		    if ((scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
			(work->adw_fip.fip_under_dv.db_datatype == DB_VCH_TYPE))
		    {
			count_dv.db_data = (PTR) &byte_count;

			if (status = adu_23octetlength(scb, dv_out, &count_dv))
			    break;
			work->adw_adc.adc_longs[ADW_L_CPNBYTES] += byte_count;
		    }
	    	}
		else
		    count = out_seg->db_t_count;
	    }
	    else /* need only part? */
	    {
		MECOPY_VAR_MACRO(dv_in->db_data,
				 dv_in->db_length,
				 dv_out->db_data);
		dv_out->db_length = dv_in->db_length;

		if ((scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
		    (work->adw_fip.fip_under_dv.db_datatype == DB_VCH_TYPE))
		{
		    count_dv.db_data = (PTR) &byte_count;

		    if (status = adu_23octetlength(scb, dv_in, &count_dv))
			break;
		    work->adw_adc.adc_longs[ADW_L_CPNBYTES] += byte_count;
		} 
	    }
	    work->adw_shared.shd_l1_check += count;
	}
	else /* Still ignoreing stuff? */
	{
	    /* If still ignoring stuff, then go get next one */

	    work->adw_shared.shd_exp_action = ADW_GET_DATA;
	}
	break;
    }
    return(status);
}

/*{
** Name:	adu_9lvch_right - Perform Right(Long Varchar)
**
** Description:
**      This routine sets up the general purpose large object filter to
**      have it filter a long varchar field to perform the "right"
**      function on said object.  It will set up the workspace with
**      initial parameter information,  then call the general filter
**      function which will arrange to get the right n characters from
**      the segments.
**
**      This routine makes use of the static adu_8lvch_right_slave()
**      routine also defined in this file.  It is thru this slave
**      routine that the work actually gets done.
**
**      This routine uses the workspace to pass information to the slave
**      routine.  This routine will initialize the
**      adw_shared.shd_l{0,1}_check areas to zero;  it expects that the
**      slave routine will provide the correct values here for the
**      resultant large object.
**
**      This routine will set the workspace's
**      adw_adc.adc_longs[ADC_L_LRCOUNT] value to be N in right(long
**      varchar, N).  The slave routine will use this value to calculate
**      the right N character of the long varchar.
**
** Re-entrancy:
**      Totally.
**
** Inputs:
**      scb                              ADF session control block.
**      dv_in                            Ptr to DB_DATA_VALUE describing
**                                       input coupon
**      dv_count                         Ptr to DB_DATA_VALUE for the
**                                       input count.  This is assumed to be
**                                       DB_INT_TYPE.
**      dv_work                          Ptr to DB_DATA_VALUE which
**                                       points to an area large enough
**                                       to hold an ADP_LO_WKSP plus 2
**                                       segments (for input & output).
**      dv_out                           Ptr to DB_DATA_VALUE describing
**                                       output area in which the output
**                                       coupon will be created.
**
** Outputs:
**      scb->adf_errcb                   Filled as appropriate.
**      *dv_out->db_data                 Defines the output coupon
**
** Returns:
**	DB_STATUS
**
** Exceptions:
**      None.
**
** Side Effects: 
**      None.
**
** History:
**	16-Dec-1992 (fred)
**         Created.
**      23-may-2007 (smeke01) b118342/b118344
**	    Work with i8.
**	13-Oct-2010 (thaju02) B124469
**	    inp_cpn per_length1 is in byte units, whereas N is in char units.
**	    Convert inp_cpn per_length from byte to char units and  feed 
**	    adu_redeem length in bytes. 
*/
DB_STATUS
adu_9lvch_right(ADF_CB        *scb,
                DB_DATA_VALUE *dv_in,
                DB_DATA_VALUE *dv_count,
                DB_DATA_VALUE *dv_work,
                DB_DATA_VALUE *dv_out)
{
    DB_STATUS              status = E_DB_OK;
    ADP_PERIPHERAL         *inp_cpn = (ADP_PERIPHERAL *) dv_in->db_data;
    ADP_PERIPHERAL         *out_cpn = (ADP_PERIPHERAL *) dv_out->db_data;
    ADP_LO_WKSP            *work = (ADP_LO_WKSP *) dv_work->db_data;
    ADP_POP_CB		   *inpop;
    i4                count;
    i4		      incpn_length1 = 0;

    for (;;)
    {
	if ((inp_cpn->per_length0 != 0) || (inp_cpn->per_length1 != 0))
	{
	    status = adu_0lo_setup_workspace(scb, dv_in, dv_work);
	    if (status)
		break;

	    /* get the value of N, the rightmost chars wanted */

	    switch (dv_count->db_length)
	    {
		case 1:
		    count =  *(i1 *) dv_count->db_data;
		    break;
		case 2: 
		    count =  *(i2 *) dv_count->db_data;
		    break;
		case 4:
		    count =  *(i4 *) dv_count->db_data;
		    break;
		case 8:
		{
		    i8 i8temp  = *(i8 *)dv_count->db_data;

		    /* per_length1 field is u_i4 so limit to i4 values as before */
		    if (i8temp > MAXI4)
			count = MAXI4;
		    else if (i8temp < MINI4LL)
			count =  MINI4;
		    else
			count = (i4) i8temp;
		    break;
		}
		default:
		    return (adu_error(scb, E_AD9998_INTERNAL_ERROR, 2, 0, "lvch_right count length"));
	    }

	    /* Now, need to ignore all char's before that */
	    if ((scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
		(work->adw_fip.fip_under_dv.db_datatype == DB_VCH_TYPE))
	    {
		/*
		** for utf8, cpn per_length1 is in byte units, but count 
		** is in char units. Compute lob length in char units.
		*/
		DB_DATA_VALUE	count_dv;

		count_dv.db_data = (PTR)&incpn_length1;
		count_dv.db_length = sizeof(incpn_length1);
		count_dv.db_datatype = DB_INT_TYPE;
		count_dv.db_prec = 0;
		if (status = adu_7strlength(scb, dv_in, &count_dv))
		    break;
	    }
	    else
		incpn_length1 = inp_cpn->per_length1;

	    work->adw_adc.adc_longs[ADW_L_LRCOUNT] =
		                  incpn_length1 - count;

#ifdef RIGHTOPZ
	    inpop = &work->adw_fip.fip_pop_cb;
	    if (incpn_length1 > count)
	    {
		i4			redeem_offset;
		i4			seek_offset;
		i4			seek_segno;

		/* Try to skip reading segments we don't need  */
		redeem_offset = incpn_length1 - count;
		seek_offset = 0;
		seek_segno = 0;
		status = adu_opz_skip(scb, dv_in, work, redeem_offset, 
			&seek_offset, &seek_segno);
		if (seek_offset)
		{
		    work->adw_adc.adc_longs[ADW_L_LRCOUNT] -= seek_offset;
		    inpop->pop_continuation = ADP_C_RANDOM_MASK | ADP_C_BEGIN_MASK;
		    inpop->pop_segno1 = seek_segno;
		}
	    }
#endif

	    if (work->adw_adc.adc_longs[ADW_L_LRCOUNT] < 0)
		work->adw_adc.adc_longs[ADW_L_LRCOUNT] = 0;

	    work->adw_shared.shd_l1_check = 0;
	    work->adw_shared.shd_l0_check = 0;
	    work->adw_adc.adc_longs[ADW_L_CPNBYTES] = 0;

	    status = adu_lo_filter(scb,
				   dv_in,
				   dv_out,
				   adu_8lvch_right_slave,
				   work,
				   (ADP_C_BEGIN_MASK | ADP_C_END_MASK),
                                   NULL);
	    out_cpn->per_length0 = work->adw_shared.shd_l0_check;
	    out_cpn->per_length1 = work->adw_shared.shd_l1_check;
            if ((scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
                (work->adw_fip.fip_under_dv.db_datatype == DB_VCH_TYPE))
                out_cpn->per_length1 = work->adw_adc.adc_longs[ADW_L_CPNBYTES];
	}
	else
	{
	    STRUCT_ASSIGN_MACRO(*inp_cpn, *out_cpn);
	}
	break;
    }
    return(status);
}

/*{
** Name:	adu_10lvch_concat_slave - Perform segment by segment concat
**
** Description:
**	This routine simply provides the slave filter function for the
**      concatenatio of long varchar objects.  For each segment with
**      which it is called, it copies the segment into the new segment.
**      It then sets the next expected action (shd_exp_action) field to
**      ADW_CONTINUE, since the concat function will always traverse
**      the entire long varchar.
**
**      This routine exists separately from the normal concat()
**      function because its calling convention is slightly different.
**      If this proves to be a problem, then fixing the filter function
**      thru which it is called to have a separate calling convention
**      for these types of functions is the obvious enhancement.
**
** Re-entrancy:
**      Totally.
**
** Inputs:
**      scb                              ADF session control block.
**      dv_in                            Ptr to DB_DATA_VALUE describing
**                                       input
**      dv_out                           Ptr to DB_DATA_VALUE describing
**                                       output area.
**      work                             Ptr to ADP_LO_WKSP workspace.
**
** Outputs:
**      scb->adf_errcb                   Filled as appropriate.
**      dv_out->db_length                Set correctly.
**      work->adw_shared.shd_exp_action  Always set to ADW_CONTINUE.
**
** Returns:
**	DB_STATUS
**
** Exceptions:
**      None.
**
** Side Effects: 
**      None.
**
** History:
**	15-Dec-1992 (fred)
**         Created.
**	09-may-2007 (gupsh01)
**	   Added support for UTF8.
*/
static DB_STATUS
adu_10lvch_concat_slave(ADF_CB	      *scb,
			DB_DATA_VALUE *dv_in,
			DB_DATA_VALUE *dv_out,
			ADP_LO_WKSP   *work)
{
    i4                 count;

    if ((Adf_globs->Adi_status & ADI_DBLBYTE)||
	(scb->adf_utf8_flag & AD_UTF8_ENABLED))
    {
    	DB_STATUS          status;
    	DB_DATA_VALUE      count_dv;

    	count_dv.db_data = (PTR) &count;
    	count_dv.db_length = sizeof(count);
    	count_dv.db_datatype = DB_INT_TYPE;
    	count_dv.db_prec = 0;
    
    	status = adu_7strlength(scb, dv_in, &count_dv);
    	if (status)
	    return(status);
    }
    else
        count = ((DB_TEXT_STRING *) dv_in->db_data)->db_t_count;

    MECOPY_VAR_MACRO(dv_in->db_data,
		     dv_in->db_length,
		     dv_out->db_data);
    work->adw_shared.shd_exp_action = ADW_CONTINUE;
    work->adw_shared.shd_l1_check += count;
    return(E_DB_OK);
}

/*{
** Name:	adu_11vch_concat - Perform Concat(Long Varchar, long varchar)
**
** Description:
**      This routine sets up the general purpose large object filter to
**      have it filter a long varchar field to perform the "concat"
**      function on said objects.  It will set up the workspace with
**      initial parameter information,  then call the general filter
**      function which will arrange to get the the segments for the
**      first, then second objects.
**
**      This routine makes use of the static adu_10lvch_concat_slave()
**      routine also defined in this file.  It is thru this slave
**      routine that the work actually gets done.
**
**      This routine uses the workspace to pass information to the slave
**      routine.  This routine will initialize the
**      adw_shared.shd_l{0,1}_check areas to zero;  it expects that the
**      slave routine will provide the correct values here for the
**      resultant large object.
**
**      If either of the input objects has zero length, the resultant
**      object is simply copied from the other (possibly non-zero) object.
**
** Re-entrancy:
**      Totally.
**
** Inputs:
**      scb                              ADF session control block.
**      dv_in1                           Ptr to DB_DATA_VALUE describing
**                                       the first input coupon
**      dv_in2                           Ptr to DB_DATA_VALUE describing
**                                       the second input coupon
**      dv_work                          Ptr to DB_DATA_VALUE which
**                                       points to an area large enough
**                                       to hold an ADP_LO_WKSP plus 2
**                                       segments (for input & output).
**      dv_out                           Ptr to DB_DATA_VALUE describing
**                                       output area in which the output
**                                       coupon will be created.
**
** Outputs:
**      scb->adf_errcb                   Filled as appropriate.
**      *dv_out->db_data                 Defines the output coupon
**
** Returns:
**	DB_STATUS
**
** Exceptions:
**      None.
**
** Side Effects: 
**      None.
**
** History:
**	16-Dec-1992 (fred)
**         Created.
*/
DB_STATUS
adu_11lvch_concat(ADF_CB        *scb,
		  DB_DATA_VALUE *dv_in,
		  DB_DATA_VALUE *dv_in2,
		  DB_DATA_VALUE *dv_work,
		  DB_DATA_VALUE *dv_out)
{
    DB_STATUS              status = E_DB_OK;
    ADP_PERIPHERAL         *inp1_cpn = (ADP_PERIPHERAL *) dv_in->db_data;
    ADP_PERIPHERAL         *inp2_cpn = (ADP_PERIPHERAL *) dv_in2->db_data;
    ADP_PERIPHERAL         *out_cpn = (ADP_PERIPHERAL *) dv_out->db_data;
    ADP_LO_WKSP            *work = (ADP_LO_WKSP *) dv_work->db_data;
    i4                     one_worth_doing = 0;
    i4                     two_worth_doing = 0;
    
    for (;;)
    {
	if (	(inp1_cpn->per_length0 != 0)
	    ||  (inp1_cpn->per_length1 != 0))
	{
	    one_worth_doing = 1;
	}
	if (    (inp2_cpn->per_length0 != 0)
	    ||  (inp2_cpn->per_length1 != 0))
	{
	    two_worth_doing = 1;
	}
	
	if (one_worth_doing && two_worth_doing)
	{
	    status = adu_0lo_setup_workspace(scb, dv_in, dv_work);
	    if (status)
		break;
	    
	    work->adw_shared.shd_l1_check = 0;
	    work->adw_shared.shd_l0_check = 0;
	    
	    status = adu_lo_filter(scb,
				   dv_in,
				   dv_out,
				   adu_10lvch_concat_slave,
				   work,
				   ADP_C_BEGIN_MASK,
                                   NULL);
	    if (status)
		break;
	    
	    status = adu_lo_filter(scb,
				   dv_in2,
				   dv_out,
				   adu_10lvch_concat_slave,
				   work,
				   ADP_C_END_MASK,
                                   NULL);

	    out_cpn->per_length0 = work->adw_shared.shd_l0_check;
	    out_cpn->per_length1 = work->adw_shared.shd_l1_check;
	}
	else if (one_worth_doing)
	{
	    STRUCT_ASSIGN_MACRO(*inp1_cpn, *out_cpn);
	}
	else
	{
	    STRUCT_ASSIGN_MACRO(*inp2_cpn, *out_cpn);
	}
	break;
    }
    return(status);
}
VOID
adu_free_objects(PTR 	    storage_location,
		 i4    sequence_number)
{

    ADP_POP_CB        pop_cb;
    DB_STATUS         status;

    pop_cb.pop_type = ADP_POP_TYPE;
    pop_cb.pop_length = sizeof(ADP_POP_CB);
    pop_cb.pop_ascii_id = ADP_POP_ASCII_ID;
    pop_cb.pop_continuation = ADP_C_BEGIN_MASK;
    pop_cb.pop_user_arg = storage_location;
    pop_cb.pop_segno1 = sequence_number;

    status = (*Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi)
	                                           (ADP_FREE_NONSESS_OBJS, &pop_cb);
    return;
}


/*{
** Name:	adu_12lvch_substr_slave - Perform segment by segment substring
**
** Description:
**	This routine simply provides the slave filter function for the
**      substring of long varchar objects.  For each segment with
**      which it is called, it subtracts the length of that segment from
**      the work->adw_adc.adc_longs[ADC_L_SUBSTR_POS] field.
**      
**      While that field is positive and non-zero, then the filter function
**      will ignore the current segment and go get another.
**      
**      When that field becomes negative, then then we are at the first  
**      segment of interest, containing the FROM position. To get to the
**      FROM position in this segment, we may need to shift the segment
**      left before copying it to the result.
**
**      If work->adw_adc.adc_longs[ADC_L_SUBSTR_CNT] is non-zero, it 
**      specifies the length of the blob to be returned. If necessary,
**      the input will be truncated or padded.
**
** Re-entrancy:
**      Totally.
**
** Inputs:
**      scb                              ADF session control block.
**      dv_in                            Ptr to DB_DATA_VALUE describing
**                                       input
**      dv_out                           Ptr to DB_DATA_VALUE describing
**                                       output area.
**      work                             Ptr to ADP_LO_WKSP workspace.
**          ->adw_shared.shd_l{0,1}_check
**                                       Length of blob seen so far
**          ->adw_adc.adc_longs[ADC_L_SUBSTR_POS]
**                                       Number of characters we still
**                                       need to start the function.
**          ->adw_adc.adc_longs[ADC_L_SUBSTR_CNT]
**                                       Number of characters to be in result.
**
** Outputs:
**      scb->adf_errcb                   Filled as appropriate.
**      dv_out->db_length                Set correctly.
**      work->adw_shared.shd_exp_action  ADW_GET_DATA if not done yet,
**                                       ADW_CONTINUE in satifying it...
**          ->adw_shared.shd_l{0,1}_check
**                                       Length of blob seen so far
**          ->adw_adc.adc_longs[ADC_L_SUBSTR_POS]
**                                       Number of characters we still
**                                       need to start the function.
**
** Returns:
**	DB_STATUS
**
** Exceptions:
**      None.
**
** Side Effects: 
**      None.
**
** History:
**	04-Oct-2006 (stial01)
**         Created.
**	13-Oct-2010 (thaju02) B124469
**	    Cpn's per_length is in byte units. Adu_redeem is expecting
**	    length in bytes. For UTF8, tally bytes and push byte count into
**	    cpn for adu_redeem.
*/
static DB_STATUS
adu_12lvch_substr_slave(ADF_CB	    *scb,
		      DB_DATA_VALUE *dv_in,
		      DB_DATA_VALUE *dv_out,
		      ADP_LO_WKSP   *work)
{
    DB_STATUS        status = E_DB_OK;
    DB_TEXT_STRING   *in_seg = (DB_TEXT_STRING *) dv_in->db_data;
    DB_TEXT_STRING   *out_seg = (DB_TEXT_STRING *) dv_out->db_data;
    DB_DATA_VALUE    count_dv;
    i4          count = 0, byte_count = 0;
    i4          amount_needed = 0;

    if (!work->adw_adc.adc_longs[ADW_L_SUBSTR_POS] &&
	!work->adw_adc.adc_longs[ADW_L_SUBSTR_CNT])
    {
	MECOPY_VAR_MACRO(dv_in->db_data,
			 dv_in->db_length,
			 dv_out->db_data);
	dv_out->db_length = dv_in->db_length;
	return (E_DB_OK);
    }

    for ( ; ; )
    {
	/*
	** Extra segment processing for FROM and FOR substring params
	*/
	count_dv.db_data = (PTR) &count;
	count_dv.db_length = sizeof(count);
	count_dv.db_datatype = DB_INT_TYPE;
	count_dv.db_prec = 0;
	if (work->adw_fip.fip_under_dv.db_datatype == DB_NVCHR_TYPE)
	   status = adu_nvchr_length(scb, dv_in, &count_dv);
	else /* string or byte type */
	    status = adu_22charlength(scb, dv_in, &count_dv);
	if (status)
	    break;

	if (work->adw_adc.adc_longs[ADW_L_SUBSTR_POS])
	{
	    if (work->adw_adc.adc_longs[ADW_L_SUBSTR_POS] - count >= 0)
	    {
		/* Still ignoring stuff, then go get next one */
		work->adw_adc.adc_longs[ADW_L_SUBSTR_POS] -= count;
		work->adw_shared.shd_exp_action = ADW_GET_DATA;
		break;
	    }

	    /* This 'left' shift can be done 'in-place' */
	    count -= work->adw_adc.adc_longs[ADW_L_SUBSTR_POS];
	    if (work->adw_fip.fip_under_dv.db_datatype == DB_NVCHR_TYPE)
		status = adu_nvchr_right(scb, dv_in, &count_dv, dv_in);
	    else /* string or byte type */
	    {
		status = adu_10strright(scb, dv_in, &count_dv, dv_in);
	    }
	    work->adw_adc.adc_longs[ADW_L_SUBSTR_POS] = 0;
	}

	if (work->adw_adc.adc_longs[ADW_L_SUBSTR_CNT] &&
		work->adw_shared.shd_l1_check + count >=
			work->adw_adc.adc_longs[ADW_L_SUBSTR_CNT])
	{
	    /*
	    ** At this point, we have all the data we need.
	    ** We now truncate the last segment
	    */
	    count = work->adw_adc.adc_longs[ADW_L_SUBSTR_CNT] -
			work->adw_shared.shd_l1_check;
	    if (work->adw_fip.fip_under_dv.db_datatype == DB_NVCHR_TYPE)
		status = adu_nvchr_left(scb, dv_in, &count_dv, dv_out);
	    else /* string or byte type */
	    {
		status = adu_6strleft(scb, dv_in, &count_dv, dv_out);

		if ((scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
		    (work->adw_fip.fip_under_dv.db_datatype == DB_VCH_TYPE))
		{
		    count_dv.db_data = (PTR) &byte_count;

		    status = adu_23octetlength(scb, dv_out, &count_dv);
		    work->adw_adc.adc_longs[ADW_L_CPNBYTES] += byte_count;
		}
	    }
	    work->adw_shared.shd_exp_action = ADW_FLUSH_STOP;
	}
	else
	{
	    work->adw_shared.shd_exp_action = ADW_CONTINUE;
	    MECOPY_VAR_MACRO(dv_in->db_data, dv_in->db_length, dv_out->db_data);
	    dv_out->db_length = dv_in->db_length;
            if ((scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
		(work->adw_fip.fip_under_dv.db_datatype == DB_VCH_TYPE))
	    {
                count_dv.db_data = (PTR) &byte_count;

                if (status = adu_23octetlength(scb, dv_in, &count_dv))
		    break;
                work->adw_adc.adc_longs[ADW_L_CPNBYTES] += byte_count;
	    }
	}

	/* Maintain l1_check only if ADW_L_SUBSTR_CNT */
	if (work->adw_adc.adc_longs[ADW_L_SUBSTR_CNT])
	    work->adw_shared.shd_l1_check += count;

	break; /* always */
    }

    return(status);
}

/*
** Name: adu_13lvch_substr() - Return the substring from a string.
**
** Description:
**	This file contains an implementation of 
**	SUBSTRING ( char FROM int ).  It is based on the
**	description for substring in ISO/IEC FDIS 9075-2:1999
**
** Inputs:
**      scb                              ADF session control block.
**      dv_in                            Ptr to DB_DATA_VALUE describing
**                                       input coupon
**      dv_pos                           DB_DATA_VALUE containing FROM operand
**      dv_work                          Ptr to DB_DATA_VALUE which
**                                       points to an area large enough
**                                       to hold an ADP_LO_WKSP plus 2
**                                       segments (for input & output).
**      dv_out                           Ptr to DB_DATA_VALUE describing
**                                       output area in which the output
**                                       coupon will be created.
**
** Outputs:
**      scb->adf_errcb                   Filled as appropriate.
**      *dv_out->db_data                 Defines the output coupon
**
** Returns:
**	DB_STATUS
**
** Exceptions:
**      None.
**
** Side Effects: 
**      None.
**
** History:
**	04-aug-2006 (stial01)
**         Created.
**
*/
DB_STATUS
adu_13lvch_substr(ADF_CB        *scb,
		DB_DATA_VALUE *dv_in,
		DB_DATA_VALUE *dv_pos,
		DB_DATA_VALUE *dv_work,
		DB_DATA_VALUE *dv_out)
{
    DB_STATUS		status;

    status = adu_14lvch_substrlen(scb, dv_in, dv_pos, NULL, dv_work,
			dv_out);
    return (status);
}

/*
** Name: adu_14lvch_substrlen() - Return the substring from a string.
**
** Description:
**	This file contains an implementation of 
**	SUBSTRING ( char FROM int FOR int ).  It is based on the
**	description for substring in ISO/IEC FDIS 9075-2:1999
**
** Inputs:
**      scb                              ADF session control block.
**      dv_in                            Ptr to DB_DATA_VALUE describing
**                                       input coupon
**      dv_pos                           DB_DATA_VALUE containing FROM operand
**      dv_count                         DB_DATA_VALUE containing FOR operand
**      dv_work                          Ptr to DB_DATA_VALUE which
**                                       points to an area large enough
**                                       to hold an ADP_LO_WKSP plus 2
**                                       segments (for input & output).
**      dv_out                           Ptr to DB_DATA_VALUE describing
**                                       output area in which the output
**                                       coupon will be created.
**
** Outputs:
**      scb->adf_errcb                   Filled as appropriate.
**      *dv_out->db_data                 Defines the output coupon
**
** Returns:
**	DB_STATUS
**
** Exceptions:
**      None.
**
** Side Effects: 
**      None.
**
** History:
**	04-aug-2006 (stial01)
**         Created.
**      23-may-2007 (smeke01) b118342/b118344
**	    Work with i8.
**	13-Oct-2010 (thaju02) B124469
**	    Cpn's per_length is in byte units. Adu_redeem is expecting
**	    length in bytes. For UTF8, tally bytes and push byte count into
**	    cpn for adu_redeem.
**
*/
DB_STATUS
adu_14lvch_substrlen(ADF_CB        *scb,
		DB_DATA_VALUE *dv_in,
		DB_DATA_VALUE *dv_pos,
		DB_DATA_VALUE *dv_count,
		DB_DATA_VALUE *dv_work,
		DB_DATA_VALUE *dv_out)
{
    DB_STATUS		status = E_DB_OK;
    ADP_PERIPHERAL	*inp_cpn = (ADP_PERIPHERAL *) dv_in->db_data;
    ADP_PERIPHERAL	*out_cpn = (ADP_PERIPHERAL *) dv_out->db_data;
    ADP_LO_WKSP		*work = (ADP_LO_WKSP *) dv_work->db_data;
    i4			start_pos;
    i4			offset;
    i4			seek_offset;
    i4			seek_segno;
    i4			per_length1;
    i4			count;
    ADP_POP_CB		*inpop;

    for (;;)
    {
	/* get the start position (the FROM value) */
	switch (dv_pos->db_length)
	{
	    case 1:
		start_pos =  *(i1 *) dv_pos->db_data;
		break;
	    case 2:
		start_pos =  *(i2 *) dv_pos->db_data;
		break;
	    case 4:
		start_pos =  *(i4 *) dv_pos->db_data;
		break;
	    case 8:
	    {
		i8 i8temp  = *(i8 *)dv_pos->db_data;

		/* limit to i4 values */
		if (i8temp > MAXI4)
		    start_pos = MAXI4;
		else if (i8temp < MINI4LL )
		    start_pos = MINI4LL;
		else
		    start_pos = (i4)i8temp;
		break;
	    }
	    default:
		return (adu_error(scb, 
			E_AD9998_INTERNAL_ERROR, 2, 0, "lvch_substr start length"));
	}

	if (start_pos <= 0)
	    start_pos = 1;

	/* Get the count (or FOR value) */
	if (!dv_count)
	{
	    /* no count specified */
	    count = 0;
	}
	else switch (dv_count->db_length)
	{
	    case 1:
		count = *(i1 *) dv_count->db_data;
		break;
	    case 2:
		count = *(i2 *) dv_count->db_data;
		break;
	    case 4:
		count = *(i4 *) dv_count->db_data;
		break;
	    case 8:
	    {
		i8 i8temp  = *(i8 *)dv_count->db_data;

		/* limit to i4 values */
		if (i8temp > MAXI4)
		    count = MAXI4;
		else if (i8temp < MINI4LL)
		    count = MINI4;
		else
		    count = (i4) i8temp;
		break;
	    }
	    default:
		return (adu_error(scb, 
			E_AD9998_INTERNAL_ERROR, 2, 0, "lvch_substr for length"));
	}

	inpop = &work->adw_fip.fip_pop_cb;
	if ((inp_cpn->per_length0 != 0) || (inp_cpn->per_length1 != 0))
	{
	    status = adu_0lo_setup_workspace(scb, dv_in, dv_work);
	    if (status)
		break;

	    offset = start_pos - 1;

	    /* Now, need to ignore all char's before that */
	    work->adw_adc.adc_longs[ADW_L_SUBSTR_POS] = offset;

	    seek_offset = 0;
	    seek_segno = 0;
	    if (inp_cpn->per_length1 > count)
	    {
		/* Try to skip reading segments we don't need  */
		status = adu_opz_skip(scb, dv_in, work, offset, 
				&seek_offset, &seek_segno);
		if (seek_offset)
		{
		    work->adw_adc.adc_longs[ADW_L_SUBSTR_POS] -= seek_offset;
		    inpop->pop_continuation = ADP_C_RANDOM_MASK | ADP_C_BEGIN_MASK;
		    inpop->pop_segno1 = seek_segno;
		}
	    }

	    if (work->adw_adc.adc_longs[ADW_L_SUBSTR_POS] < 0)
		work->adw_adc.adc_longs[ADW_L_SUBSTR_POS] = 0;

	    work->adw_shared.shd_l1_check = 0;
	    work->adw_shared.shd_l0_check = 0;

	    work->adw_adc.adc_longs[ADW_L_SUBSTR_CNT] = count;
	    work->adw_adc.adc_longs[ADW_L_CPNBYTES] = 0;

	    status = adu_lo_filter(scb,
				   dv_in,
				   dv_out,
				   adu_12lvch_substr_slave,
				   work,
				   (ADP_C_BEGIN_MASK | ADP_C_END_MASK),
				   NULL);
	    out_cpn->per_length0 = work->adw_shared.shd_l0_check;
	    out_cpn->per_length1 = work->adw_shared.shd_l1_check;

	    if ((scb->adf_utf8_flag & AD_UTF8_ENABLED) && 
		(work->adw_fip.fip_under_dv.db_datatype == DB_VCH_TYPE))
		out_cpn->per_length1 = work->adw_adc.adc_longs[ADW_L_CPNBYTES];
	}
	else
	{
	    STRUCT_ASSIGN_MACRO(*inp_cpn, *out_cpn);
	}
	break;
    }
    return(status);
}


/*{
** Name:	adu_opz_skip - Optimize Right/substring
**
** Description:
**      This routine attempts to optimize Right and FROM processing.
**      Based on the blob length, and assuming full segments,
**      it calculates what should be the last segment number and length.
**      It seeks to that segment to verify, and if so adjusts the
**      workspace so that we will skip the unnecessary (left) segments. 
**
** Inputs:
**      scb                              ADF session control block.
**      dv_in                            Ptr to DB_DATA_VALUE describing
**                                       input coupon
**      dv_work                          Ptr to DB_DATA_VALUE which
**                                       points to an area large enough
**                                       to hold an ADP_LO_WKSP plus 2
**                                       segments (for input & output).
**      redeem_offset			 number of bytes in blob to skip 
**
** Outputs:
**      scb->adf_errcb                   Filled as appropriate.
**      seek_offset			 number of bytes skipped by setting
**                                       ADP_C_RANDOM_MASK and pop_segno1
**      seek_segno			 If seek_offset, corresponding segno1
**
** Returns:
**	DB_STATUS
**
** Exceptions:
**      None.
**
** Side Effects: 
**      None.
**
** History:
**	04-aug-2006 (stial01)
**         Created.
**	09-may-2007 (gupsh01)
**	   Added support for UTF8.
*/
static DB_STATUS
adu_opz_skip(ADF_CB		*adf_scb,
		DB_DATA_VALUE	*dv_in,
		ADP_LO_WKSP	*work,
		i4		redeem_offset,
		i4		*seek_offset,
		i4		*seek_segno)
{
    DB_STATUS	    	status, local_status;  
    ADP_PERIPHERAL	*inp_cpn = (ADP_PERIPHERAL *) dv_in->db_data;
    ADP_POP_CB		in_pop_cb;
    ADP_POP_CB		*pop_cb;
    bool		isnull = 0;
    i4			length1;
    i4			last_seg;
    i2			last_seglen;
    i4			offset_seg;
    i4			segbytes;
    i2			seglen;
    DB_DATA_VALUE	*segdv;
    i4			maxseglen;
    char		*segdata;
    bool		optimize = FALSE;
    DB_DATA_VALUE	count_dv;
    bool		didget = FALSE;
    i4			opzerr = 0;

    /*
    ** UNITS of redeem_offset depends on underlying datatype
    ** For byte datatypes, redeem_offset units = bytes
    ** For char datatypes, redeem_offset units = chars (single/nchar/dbl)
    */
    *seek_offset = 0;
    *seek_segno = 0;

    /* Note this optimization depends on ADP_C_RANDOM_MASK working correctly */
    for (status = E_DB_OK; ; )
    {
	/* No segment seek optimization for DBLBYTE */
	if ((Adf_globs->Adi_status & ADI_DBLBYTE) ||
	    (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED))
	{
	    opzerr = 1;
	    break;
	}

	/* Get blob length */
	I4ASSIGN_MACRO(inp_cpn->per_length1, length1); 

	pop_cb = &work->adw_fip.fip_pop_cb;
	pop_cb->pop_segment = &work->adw_fip.fip_seg_dv;
	pop_cb->pop_coupon = dv_in;
	pop_cb->pop_user_arg = (PTR)NULL;
	segdv = pop_cb->pop_segment;

	/* Calculate last segment # assuming full segments */
	if (work->adw_fip.fip_under_dv.db_datatype == DB_NVCHR_TYPE)
	    maxseglen = (pop_cb->pop_underdv->db_length - sizeof(i2))/ sizeof(UCS2);
	else
	    maxseglen = pop_cb->pop_underdv->db_length - sizeof(i2);

	last_seg = (length1 / maxseglen) + 1;
	last_seglen = (length1 % maxseglen);
	offset_seg = (redeem_offset / maxseglen) + 1;

	/* Don't optimize small seek */
	if (offset_seg < 4)
	{
	    opzerr = 2;
	    break;
	}
	
	pop_cb->pop_continuation = ADP_C_RANDOM_MASK | ADP_C_BEGIN_MASK;
	pop_cb->pop_segno1 = last_seg;
	pop_cb->pop_segno0 = 0;
	didget = TRUE;

	status = (*Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi)
		(ADP_GET, pop_cb);

	/*
	** Only optimize/seek if this is really the last segment
	** We might also get this error if the blob spans etabs and
	** the coupon etab is not the last segment etab
	*/
	if (status != E_DB_WARN  || 
		pop_cb->pop_error.err_code != E_AD7001_ADP_NONEXT ||
		    pop_cb->pop_segno1 != last_seg)
	{
	    opzerr = 3;
	    break;
	}

	/* adu_lenaddr returns #bytes and ptr to data */
	status = adu_lenaddr(adf_scb, segdv, &segbytes, &segdata);
	if (status)
	{
	    opzerr = 4;
	    break;
	}

	if (segbytes)
	{
	    count_dv.db_length = sizeof(seglen);
	    count_dv.db_datatype = DB_INT_TYPE;
	    count_dv.db_data = (PTR)&seglen;

	    if (work->adw_fip.fip_under_dv.db_datatype == DB_NVCHR_TYPE)
		status = adu_nvchr_length(adf_scb, segdv, &count_dv);
	    else
		status = adu_22charlength(adf_scb, segdv, &count_dv);
	    if (status)
	    {
		opzerr = 5;
		break;
	    }
	}
	else
	{
	    seglen = 0;
	}

	if (seglen != last_seglen)
	{
	    opzerr = 6;
	    break;
	}

	optimize = TRUE;
	status = E_DB_OK;
	opzerr = 0;
	break;	/* Always */
    }

    /* Always CLEANUP pop, dmpe_get needs this after RANDOM get */
    if (didget)
    {
	local_status = (*Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi)
		    (ADP_CLEANUP, pop_cb);
	pop_cb->pop_continuation = ADP_C_BEGIN_MASK;
    }

    if (status == E_DB_OK && optimize)
    {
	*seek_segno = offset_seg;
	*seek_offset = (offset_seg - 1) * maxseglen;
#ifdef xDEBUG
	TRdisplay("ADP: OPTIMIZE start position %d RANDOM seg %d seek position %d\n",
		    redeem_offset + 1, offset_seg, *seek_offset + 1);
#endif
	return (E_DB_OK);
    }

    /* we didn't seek to correct segment */

#ifdef xDEBUG
	    TRdisplay("ADP: NOOPTIMIZE start position %d (%d)\n", 
		redeem_offset + 1, opzerr);
#endif

    return(E_DB_OK);
}


/*{
** Name: adu_15lvch_position() ANSI standard to locate 'string' within a blob.
** 	Returns the position of the first occurrence of 'dv1' within 'dv_lo'
**
** Description:
**	This routine returns the position of the first occurrence of 'dv1'
**	within 'dv_lo'. 'dv1' can be any number of characters.  If not found,
**	it will return 0. This retains trailing blanks in the matching string.  
**	The empty string is found (by definition) at offset 1. 
**      Note: the difference between 'locate' and 'position' is the
**      handling of the empty search string and an unfound search string.
**
**	This routine works for all INGRES character types: c, text, char, and
**	varchar. It also works for nchar,nvarchar.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**      dv1                             Ptr to DB_DATA_VALUE containing string
**					to be found.
**	    .db_datatype		Its type.
**	    .db_length			Its length.
**	    .db_data			Ptr to search string.
**	dv_lo				Ptr to DB_DATA_VALUE describing
**					input coupon for blob being searched.
**      dv_work				Ptr to DB_DATA_VALUE which
**					points to an area large enough
**					to hold an ADP_LO_WKSP plus 1
**					segments (for input).
** Outputs:
**	adf_scb				Filled as appropriate.
**	rdv				Ptr to result DB_DATA_VALUE.
**	    .db_data			Ptr to an i2/i4 which holds the result.
**
**  Returns:
**	    The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.
**
**  History:
**      04-aug-2006 (stial01)
**         Created.
*/
DB_STATUS
adu_15lvch_position(ADF_CB        *scb,
		DB_DATA_VALUE *dv1,
		DB_DATA_VALUE *dv_lo,
		DB_DATA_VALUE *dv_work,
		DB_DATA_VALUE *dv_pos)
{
    DB_STATUS		status;

    status = adu_16lvch_position(scb, dv1, dv_lo, NULL, dv_work, dv_pos);
    return (status);
}


DB_STATUS
adu_16lvch_position(ADF_CB        *scb,
		DB_DATA_VALUE *dv1,
		DB_DATA_VALUE *dv_lo,
		DB_DATA_VALUE *dv_from,
		DB_DATA_VALUE *dv_work,
		DB_DATA_VALUE *dv_pos)
{
    DB_STATUS	    status, local_status;
    char	    *p1, *p2;
    ADP_PERIPHERAL  *inp_cpn = (ADP_PERIPHERAL *) dv_lo->db_data;
    ADP_LO_WKSP     *work = (ADP_LO_WKSP *) dv_work->db_data;
    char	    *morep2, *morep2end;
    char	    *p1save,
		    *p2startpos,
		    *p1end,
		    *p2end,
		    *p2last,
		    *ptemp;
    i4		    size1;
    i4		    pos = 1;
    i4		    p2lastpos;
    i4		    found;
    DB_DT_ID	    bdt1;
    DB_DT_ID	    bdt2;
    DB_STATUS	    db_stat;
    DB_DT_ID	    dtid = ADI_DT_MAP_MACRO(abs(dv_lo->db_datatype));
    ADP_POP_CB	    pop_cb;
    i4		    done = 0;
    i4		    start_pos;
    i4		    seek_offset;
    i4		    seek_segno;
    DB_DATA_VALUE   count_dv;
    i4		    count;
    i4		    elmsize;

    bdt1 = dv1->db_datatype;
    bdt2 = dv_lo->db_datatype;
    if ((Adf_globs->Adi_dtptrs[dtid]->adi_dtstat_bits & AD_PERIPHERAL) == 0)
    {
	return(adu_error(scb, E_AD2085_LOCATE_NEEDS_STR, 0));
    }

    /* FIX ME position nvchr with alternate collations not supported */
    /* FIX ME db_collID is not init here */
    if (bdt2 == DB_LNVCHR_TYPE &&
     (dv1->db_collID > DB_UNICODE_COLL || dv_lo->db_collID > DB_UNICODE_COLL))
    {
	return(adu_error(scb, E_AD2085_LOCATE_NEEDS_STR, 0));
    }

    if (dv_from)
    {
	/* get the start position (the FROM value) */
	switch (dv_from->db_length)
	{
	    case 1:
		start_pos =  *(i1 *) dv_from->db_data;
		break;
	    case 2:
		start_pos =  *(i2 *) dv_from->db_data;
		break;
	    case 4:
		start_pos =  *(i4 *) dv_from->db_data;
		break;
	    default:
		return (adu_error(scb, 
			E_AD9998_INTERNAL_ERROR, 2, 0, "pos start len"));
	}

	count_dv.db_data = (PTR) &count;
	count_dv.db_length = sizeof(count);
	count_dv.db_datatype = DB_INT_TYPE;
	count_dv.db_prec = 0;
    }
    else
	start_pos = 1;

    if ((db_stat = adu_lenaddr(scb, dv1, &size1, &ptemp)) != E_DB_OK)
	return (db_stat);
    p1save = ptemp;
    p1end  = ptemp + size1;
    found = FALSE;

    /* p2lastpos is one past the last position we should start a search from */
    p2lastpos = max(0, inp_cpn->per_length1 - size1 + 1);

    if (bdt2 == DB_LNVCHR_TYPE)
	elmsize = sizeof(UCS2);
    else if (bdt2 == DB_LBYTE_TYPE 
		|| (((Adf_globs->Adi_status & ADI_DBLBYTE) == 0) &&
	    		((scb->adf_utf8_flag & AD_UTF8_ENABLED) == 0)))
	elmsize = 1;
    else /* doublebyte, character size varies */
	elmsize = 0;

    for ( ; ; )
    {
	pop_cb.pop_user_arg = (PTR) 0;

	if (size1 <= 0)
	{
	    /* The empty string will be found at the start */
	    /* empty string - leave pos = 1 */
	    found = TRUE;
	    break;
	}

	if ((size1 > inp_cpn->per_length1) ||
		(inp_cpn->per_length0 == 0 && inp_cpn->per_length1 == 0) ||
		start_pos > p2lastpos)
	{
	    pos = 0;
	    break;
	}

	status = adu_0lo_setup_workspace(scb, dv_lo, dv_work);
	if (status)
	    break;

	/* Init pop */
	STRUCT_ASSIGN_MACRO(work->adw_fip.fip_pop_cb, pop_cb);
	pop_cb.pop_segment = &work->adw_fip.fip_seg_dv;
	pop_cb.pop_coupon = dv_lo;
        pop_cb.pop_segno1 = 0;

	/* See if we can determine FROM segment number  */
	seek_offset = 0;
	seek_segno = 0;
	if (start_pos > 1)
	{
	    status = adu_opz_skip(scb, dv_lo, work, start_pos - 1, 
			&seek_offset, &seek_segno);
	    pos += seek_offset;
	}

	/* Read first pop segment, do FROM processing  */
	for ( ; ; )
	{
	    status = adu_getseg(scb, &pop_cb, &p2, &p2end, &done, seek_segno);
	    if (status)
		break;

	    if (pos == start_pos)
		break;

	    /* FROM processing */
	    if (work->adw_fip.fip_under_dv.db_datatype == DB_NVCHR_TYPE)
	       status = adu_nvchr_length(scb, pop_cb.pop_segment, &count_dv);
	    else /* string or byte type */
		status = adu_22charlength(scb, pop_cb.pop_segment, &count_dv);
	    if (status)
		break;
	    if (pos + count < start_pos)
	    {
		pos += count;
		continue;
	    }
	    while (pos < start_pos && p2 < p2end)
	    {
		if (elmsize)
		    p2 += elmsize;
		else
		    CMnext(p2);
		pos++;
	    }
	    break; /* always */
	}

	p2startpos = p2;
	morep2 = morep2end = NULL;

	p1 = p1save;

	while (!found)
	{
	    if (pos > p2lastpos)
		break;

	    if (!p2)
	    {
		status = adu_getseg(scb, &pop_cb, &p2, &p2end, &done, 0);
		if (status)
		    break;

		p2startpos = p2;
		morep2 = morep2end = NULL;
	    }

	    if (elmsize)
	    {
		while (p1 < p1end && p2 < p2end && !MEcmp(p1, p2, elmsize))
		{
		    p1 += elmsize;
		    p2 += elmsize;
		}
	    }
	    else
	    {
		while (p1 < p1end && p2 < p2end && !CMcmpcase(p1, p2))
		{
		    CMnext(p1);
		    CMnext(p2);
		}
	    }

	    if (p1 >= p1end)
	    {
	    	found = TRUE;
		break;
	    }

	    if (p2 < p2end)
	    {
		/* Character mismatch, more p2 */
		if (p1 != p1save)
		{
		    /* save our position in p2 */
		    morep2 = p2;
		    morep2end = p2end;
		    /* use p1 buffer for p2, these are chars we have seen */
		    /* and they may be gone from blob segment */
		    p2startpos = p1save;
		    p2end = p1;
		}

		if (elmsize)
		    p2startpos += elmsize;
		else
		    CMnext(p2startpos);
		pos++;
		p2 = p2startpos;
		p1 = p1save;
		continue;
	    }

	    if (p2 >= p2end)
	    {
		if (morep2)
		{
		    p2startpos = morep2;
		    p2 = p2startpos;
		    p2end = morep2end;
		    morep2 = morep2end = NULL;
		    continue;
		}
		else if (p2 >= p1save && p2 <= p1end)
		{
		    p2 = p2startpos;
		    if (elmsize)
			p2 += elmsize;
		    else
			CMnext(p2);
		    pos++;
		    if (p2 < p2end)
			continue;
		}
	    }

	    /* Need another segment */
	    if (done)
		break;

	    p2 = p2startpos = p2end = NULL;
	}

	break; /* always */
    }

    /* Always CLEANUP */
    local_status = (*Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi)
	    (ADP_CLEANUP, &pop_cb);
	    
    if (!found)	    /* if not found, make pos = size plus 1 */
	pos = 0;		/* not found - pos = 0 */

    if (dv_pos->db_length == 2)
	*(i2 *)dv_pos->db_data = pos;
    else
	*(i4 *)dv_pos->db_data = pos;

#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect, ADF_001_STRFI_EXIT,&dum1,&dum2))
    {
        TRdisplay("adu_16lvch_position: returning DB_DATA_VALUE:\n");
        adu_prdv(dv_pos);
    }
#endif

    return (E_DB_OK);
}


/*{
** Name: adu_17lvch_position() ANSI standard to locate blob within a blob.
** 	Returns the position of the first occurrence of 'dv1' within 'dv2'
**
** Description:
**	This routine returns the position of the first occurrence of 'dv1'
**	within 'dv2' where dv1 AND dv2 are blobs.  If not found,
**	it will return 0. This retains trailing blanks in the matching string.  
**	The empty string is found (by definition) at offset 1. 
**      Note: the difference between 'locate' and 'position' is the
**      handling of the empty search string and an unfound search string.
**
**	This routine works for lvarchar and lnvarchar datatypes.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	dv1				Ptr to DB_DATA_VALUE describing
**					input coupon for blob to be found.
**	dv2				Ptr to DB_DATA_VALUE describing
**					input coupon for blob being searched.
**      dv_work				Ptr to DB_DATA_VALUE which
**					points to an area large enough
**					to hold an ADP_LO_WKSP plus 1
**					segments (for input).
** Outputs:
**	adf_scb				Filled as appropriate.
**	rdv				Ptr to result DB_DATA_VALUE.
**	    .db_data			Ptr to an i2/i4 which holds the result.
**
**  Returns:
**	    The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.
**
**  History:
**      04-aug-2006 (stial01)
**         Created.
*/
DB_STATUS
adu_17lvch_position(ADF_CB        *scb,
		DB_DATA_VALUE *dv1,
		DB_DATA_VALUE *dv2,
		DB_DATA_VALUE *dv_work,
		DB_DATA_VALUE *dv_pos)
{
    DB_STATUS		status;

    status = adu_18lvch_position(scb, dv1, dv2, NULL, dv_work, dv_pos);
    return (status);
}

DB_STATUS
adu_18lvch_position(ADF_CB        *scb,
		DB_DATA_VALUE *dv1,
		DB_DATA_VALUE *dv2,
		DB_DATA_VALUE *dv_from,
		DB_DATA_VALUE *dv_work,
		DB_DATA_VALUE *dv_pos)
{
    DB_STATUS	    status, local_status;
    char	    *p1, *p2;
    ADP_PERIPHERAL  *cpn1 = (ADP_PERIPHERAL *) dv1->db_data;
    ADP_PERIPHERAL  *cpn2 = (ADP_PERIPHERAL *) dv2->db_data;
    ADP_LO_WKSP     *work = (ADP_LO_WKSP *) dv_work->db_data;
    char	    *p1startpos,
		    *p2startpos,
		    *p1end,
		    *p2end,
		    *ptemp;
    i4		    pos = 1;
    i4		    p2lastpos;
    i4		    found;
    DB_DT_ID	    bdt1;
    DB_DT_ID	    bdt2;
    DB_STATUS	    db_stat;
    DB_DT_ID	    dtid1 = ADI_DT_MAP_MACRO(abs(dv1->db_datatype));
    DB_DT_ID	    dtid2 = ADI_DT_MAP_MACRO(abs(dv2->db_datatype));
    DB_DT_ID	    udtid1, udtid2;
    ADP_POP_CB	    pop1, pop2;
    i4		    p2startseg;
    i4		    donep1 = 0;
    i4		    donep2 = 0;
    i4		    start_pos;
    i4		    seek_offset;
    i4		    seek_segno = 0;
    DB_DATA_VALUE   count_dv;
    i4		    count;
    i4		    elmsize;

    bdt1 = dv1->db_datatype;
    bdt2 = dv2->db_datatype;
    if (bdt1 != bdt2 ||
       (Adf_globs->Adi_dtptrs[dtid1]->adi_dtstat_bits & AD_PERIPHERAL) == 0 ||
       ((Adf_globs->Adi_dtptrs[dtid2]->adi_dtstat_bits & AD_PERIPHERAL) == 0))
    {
    	return(adu_error(scb, E_AD2085_LOCATE_NEEDS_STR, 0));
    }

    /* FIX ME position nvchr with alternate collations not supported */
    /* FIX ME db_collID is not init here */
    if (bdt1 == DB_LNVCHR_TYPE &&
       (dv1->db_collID > DB_UNICODE_COLL || dv2->db_collID > DB_UNICODE_COLL))
    {
	return(adu_error(scb, E_AD2085_LOCATE_NEEDS_STR, 0));
    }

    if (dv_from)
    {
	/* get the start position (the FROM value) */
	switch (dv_from->db_length)
	{
	    case 1:
		start_pos =  *(i1 *) dv_from->db_data;
		break;
	    case 2:
		start_pos =  *(i2 *) dv_from->db_data;
		break;
	    case 4:
		start_pos =  *(i4 *) dv_from->db_data;
		break;
	    default:
		return (adu_error(scb, 
			E_AD9998_INTERNAL_ERROR, 2, 0, "pos start len"));
	}

	count_dv.db_data = (PTR) &count;
	count_dv.db_length = sizeof(count);
	count_dv.db_datatype = DB_INT_TYPE;
	count_dv.db_prec = 0;
    }
    else
	start_pos = 1;

    found = FALSE;

    /* p2lastpos is one past the last position we should start a search from */
    p2lastpos = max(0, cpn2->per_length1 - cpn1->per_length1 + 1);

    if (bdt2 == DB_LNVCHR_TYPE)
	elmsize = sizeof(UCS2);
    else if (bdt2 == DB_LBYTE_TYPE 
		|| (((Adf_globs->Adi_status & ADI_DBLBYTE) == 0) &&
	    		((scb->adf_utf8_flag & AD_UTF8_ENABLED) == 0)))
	elmsize = 1;
    else /* doublebyte, character size varies */
	elmsize = 0;

    for ( ; ; )
    {
	pop1.pop_user_arg = (PTR) 0;
 	pop2.pop_user_arg = (PTR) 0;

	if (cpn1->per_length1 <= 0)
	{
	    /* The empty string will be found at the start */
	    /* empty string - leave pos = 1 */
	    found = TRUE;
	    break;
	}

	status = adu_0lo_setup_workspace(scb, dv1, dv_work);
	if (status)
	    break;

	if ((cpn1->per_length1 > cpn2->per_length1)
	    || (cpn2->per_length0 == 0 && cpn2->per_length1 == 0) ||
		start_pos > p2lastpos)
	{
	    pos = 0;
	    break;
	}

	/* Init pop1 */
	STRUCT_ASSIGN_MACRO(work->adw_fip.fip_pop_cb, pop1);
	pop1.pop_segment = &work->adw_fip.fip_work_dv;
	pop1.pop_coupon = dv1;
	p1 = p1end = NULL;
	pop1.pop_continuation = ADP_C_BEGIN_MASK;

	/* Read first pop1 segment */
	status = adu_getseg(scb, &pop1, &p1, &p1end, &donep1, 0);
	if (status)
	    break;
	p1startpos = p1;

	/* Init pop2 */
	STRUCT_ASSIGN_MACRO(work->adw_fip.fip_pop_cb, pop2);
	pop2.pop_segment = &work->adw_fip.fip_seg_dv;
	pop2.pop_coupon = dv2;
	p2startpos = p2 = p2end = NULL;
	pop2.pop_continuation = ADP_C_BEGIN_MASK;
	pop2.pop_segno1 = 0;

	/* See if we can determine FROM segment number */
	seek_offset = 0;
	seek_segno = 0;
	if (start_pos > 1)
	{
	    status = adu_opz_skip(scb, dv2, work, start_pos - 1, 
			&seek_offset, &seek_segno); 
	    if (seek_offset)
		pos += seek_offset;
	}

	/* Read first pop2 segment, do FROM processing  */
	for ( ; ; )
	{
	    status = adu_getseg(scb, &pop2, &p2, &p2end, &donep2, seek_segno);
	    if (status)
		break;

	    if (pos == start_pos)
		break;

	    /* FROM processing */
	    if (work->adw_fip.fip_under_dv.db_datatype == DB_NVCHR_TYPE)
	       status = adu_nvchr_length(scb, pop2.pop_segment, &count_dv);
	    else /* string or byte type */
		status = adu_22charlength(scb, pop2.pop_segment, &count_dv);
	    if (status)
		break;
	    if (pos + count < start_pos)
	    {
		pos += count;
		continue;
	    }
	    while (pos < start_pos && p2 < p2end)
	    {
		if (elmsize)
		    p2 += elmsize;
		else
		    CMnext(p2);
		pos++;
	    }
	    break; /* always */
	}

	if (status)
	    break;

	p2startpos = p2;
	p2startseg = pop2.pop_segno1;

	/* Find position of dv1 in dv2 */
	for ( ; ; )
	{
	    if (pos > p2lastpos)
		break;

	    if ( elmsize )
	    {
		while (p1 < p1end && p2 < p2end && !MEcmp(p1, p2, elmsize))
		{
		    p1 += elmsize;
		    p2 += elmsize;
		}
	    }
	    else
	    {
		while (p1 < p1end && p2 < p2end && !CMcmpcase(p1, p2))
		{
		    CMnext(p1);
		    CMnext(p2);
		}
	    }

	    if (p1 >= p1end)
	    {
		/* Out of input p1 */
		if (donep1)
		{
		    found = TRUE;
		    break;
		}

		status = adu_getseg(scb, &pop1, &p1, &p1end, &donep1, 0);
		if (status)
		    break;
		continue;
	    }

	    if (p2 >= p2end && !donep2)
	    {
		/* Out of input p2 */
		status = adu_getseg(scb, &pop2, &p2, &p2end, &donep2, 0);
		if (status)
		    break;
		continue;
	    }

	    /* Character mismatch, Reset pop1 to beginning, segment one  */
	    if (pop1.pop_segno1 != 1)
	    {
		status = adu_getseg(scb, &pop1, &p1, &p1end, &donep1, 1);
		if (status)
		    break;
	    }
	    else
		p1 = p1startpos;

	    /* Character mismatch, Reset pop2 segment */
	    if (pop2.pop_segno1 != p2startseg)
	    {
		status = adu_getseg(scb, &pop2, &p2, &p2end,&donep2,p2startseg);
		if (status)
		    break;
	    }

	    /* Reset pop2 position */
	    p2 = p2startpos;

	    if (p2 < p2end)
	    {
		/* increment pop2 position */
		if (elmsize)
		    p2 += elmsize;
		else
		    CMnext(p2);
		p2startpos = p2;
		pos++;
		if (p2 < p2end)
		    continue;
	    }

	    /* Need another segment */
	    if (!donep2)
	    {
		status = adu_getseg(scb, &pop2, &p2, &p2end, &donep2, 0);
		p2startpos = p2;
		p2startseg = pop2.pop_segno1;
	    }
	    else
	    {
		/* Really out of input p2 */
		break;
	    }
	}

	break; /* ALWAYS */
    }

    local_status = (*Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi)
	    (ADP_CLEANUP, &pop1);

    local_status = (*Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi)
	    (ADP_CLEANUP, &pop2);
	    
    if (!found)	    /* if not found, make pos = size plus 1 */
	pos = 0;		/* not found - pos = 0 */

    if (dv_pos->db_length == 2)
	*(i2 *)dv_pos->db_data = pos;
    else
	*(i4 *)dv_pos->db_data = pos;

#ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect, ADF_001_STRFI_EXIT,&dum1,&dum2))
    {
	TRdisplay("adu_18lvch_position: returning DB_DATA_VALUE:\n");
	adu_prdv(dv_pos);
    }
#endif

    return (E_DB_OK);
}


/*{
** Name: adu_getseg() Get a blob segment, return segment start,end
**
** Description:
**      This routine gets a blob segment, return segment start,end
**	This routine works for all lvarchar and lnvarchar, lbyte
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**      pop_cb				Ptr to ADF Peripheral Operation CB
**
** Outputs:
**      segstart			Ptr to place to put start segment ptr.
**      segend				Ptr to place end segment ptr. 
**      done				Ptr to place to indicate done.
**
**  Returns:
**	    The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	    If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.
**
**  History:
**      04-aug-2006 (stial01)
**         Created.
*/
static DB_STATUS
adu_getseg(ADF_CB        *scb,
		ADP_POP_CB	*pop_cb,
		char		**segstart,
		char		**segend,
		i4		*done,
		i4		segno)
{
    i4			len;
    char		*temp;
    DB_STATUS		status;

    if (segno)
    {
	status = E_DB_OK;
	if (pop_cb->pop_segno1 == segno)
	{
	    /* DON'T reset done status */
	    status = E_DB_OK;
	}
	else
	{
	    *done = FALSE;

	    /* Always CLEANUP, RANDOM needs to open/position */
	    status = (*Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi)
		(ADP_CLEANUP, pop_cb);

	    if (status)
		return (status);

	    /* RANDOM seek to segno */
	    pop_cb->pop_segno1 = segno;
	    pop_cb->pop_continuation = ADP_C_BEGIN_MASK | ADP_C_RANDOM_MASK;
	    status =
	    (*Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi)(ADP_GET, pop_cb);
	}
    }
    else
    {
	*done = FALSE;

	status =
	(*Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi)(ADP_GET, pop_cb);
    }

    if (status)
    {
	if (DB_FAILURE_MACRO(status) ||
	    (pop_cb->pop_error.err_code != E_AD7001_ADP_NONEXT))
	{
	    status = adu_error(scb, pop_cb->pop_error.err_code, 0);
	}
	else if (pop_cb->pop_error.err_code == E_AD7001_ADP_NONEXT)
	{
	    *done = 1;
	    status = E_DB_OK;
	}
    }

    pop_cb->pop_continuation &= ~(ADP_C_BEGIN_MASK | ADP_C_RANDOM_MASK);

    if (status == E_DB_OK)
    {
	status = adu_lenaddr(scb, pop_cb->pop_segment, &len, &temp);
    }

    if (status == E_DB_OK)
    {
	*segstart = temp;
	*segend = temp + len;
    }
    else
    {
	*segstart = *segend = NULL;
    }

    if (segno && pop_cb->pop_segno1 != segno)
    {
	pop_cb->pop_error.err_code = E_AD7001_ADP_NONEXT;
	status = E_DB_ERROR;
    }

    return (status);
}


/*{
** Name:    adu_locator_to_cpn - Get COUPON indicated by blob LOCATOR 
**
** Description:
**      This routine gets the COUPON for the input blob LOCATOR
**
** Inputs:
**      adf_scb                 Standard ADF session control block
**      locator_dv              Locator for blob 
**
** Outputs:
**      scb->adf_errcb          Filled as appropriate.
**      coupon_dv               Ptr to DB_DATA_VALUE which contains
**                              the coupon for blob indicated by the locator
**
** Returns:
**	DB_STATUS
**
** Exceptions:
**      None.
**
** Side Effects: 
**      None.
**
** History:
**      16-oct-2006 (stial01)
**         Created.
*/
DB_STATUS
adu_locator_to_cpn(
ADF_CB		*adf_scb,
DB_DATA_VALUE   *locator_dv,
DB_DATA_VALUE	*coupon_dv)
{
    ADP_POP_CB		pop_cb;
    DB_STATUS		status;
    DB_DATA_VALUE	test_dv;
    ADP_PERIPHERAL	test_locator;

    /* get coupon for this locator */
    pop_cb.pop_type = (ADP_POP_TYPE);
    pop_cb.pop_length = sizeof(ADP_POP_CB);
    pop_cb.pop_ascii_id = ADP_POP_ASCII_ID;
    pop_cb.pop_segment = locator_dv;
    pop_cb.pop_coupon = coupon_dv;
    pop_cb.pop_user_arg = (PTR)0;
    pop_cb.pop_continuation = 0;

    status = 
	(*Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi)(ADP_LOCATOR_TO_CPN, 
	&pop_cb);

    if (status)
    {
	return(adu_error(adf_scb, E_AD2094_INVALID_LOCATOR, 0));
    }

    return (E_DB_OK);
}


/*
** {
** Name: adu_cpn_to_locator	- Create a locator for the input coupon
**
** Description:
**	This routine creates a LOCATOR for the input COUPON.
**
** Inputs:
**      adf_scb                 Standard ADF session control block
**      coupon_dv               Ptr to DB_DATA_VALUE which contains
**                              the coupon.
**
** Outputs:
**      adf_scb->adf_errcb      Filled as appropriate.
**      locator_dv              Ptr to DB_DATA_VALUE into which to
**                              place the locator.
**
**	Returns:
**	    DB_STATUS
**
**	Exceptions:
**	    None.
**
** Side Effects:
**	    None.
**
** History:
**      16-oct-2006 (stial01)
**	    created.
**	24-jun-2009 (gupsh01)
**	    Fix setting of locator value at constant offset of
**	    ADP_HDR_SIZE in the ADP_PERIPHERAL structure, irrespective
**	    of the platform.
*/
DB_STATUS
adu_cpn_to_locator(
ADF_CB            *adf_scb,
DB_DATA_VALUE      *coupon_dv,
DB_DATA_VALUE	   *locator_dv)
{
    ADP_POP_CB		pop_cb;
    DB_DT_ID		dtid;
    ADP_PERIPHERAL	*p = (ADP_PERIPHERAL *) locator_dv->db_data;
    DB_STATUS		status;

    dtid = ADI_DT_MAP_MACRO(abs(coupon_dv->db_datatype));
    if (   dtid <= 0
	|| dtid  > ADI_MXDTS
	|| Adf_globs->Adi_dtptrs[dtid] == NULL
	|| !Adf_globs->Adi_dtptrs[dtid]->adi_under_dt
	|| (Adf_globs->Adi_dtptrs[dtid]->adi_dtstat_bits & AD_PERIPHERAL) == 0)
    {
	return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    }

    dtid = ADI_DT_MAP_MACRO(abs(locator_dv->db_datatype));
    if ((abs(locator_dv->db_datatype) != DB_LNLOC_TYPE 
	&& abs(locator_dv->db_datatype) != DB_LBLOC_TYPE
	&& abs(locator_dv->db_datatype) != DB_LCLOC_TYPE)
	|| (locator_dv->db_length != ADP_LOC_PERIPH_SIZE &&
	   (locator_dv->db_length != ADP_LOC_PERIPH_SIZE + 1))
	|| dtid <= 0
	|| dtid  > ADI_MXDTS
	|| Adf_globs->Adi_dtptrs[dtid] == NULL)
    {
	return(adu_error(adf_scb, E_AD2004_BAD_DTID, 0));
    }

    pop_cb.pop_type = (ADP_POP_TYPE);
    pop_cb.pop_length = sizeof(ADP_POP_CB);
    pop_cb.pop_ascii_id = ADP_POP_ASCII_ID;
    pop_cb.pop_coupon = coupon_dv;
    pop_cb.pop_segment = locator_dv;

    status = 
    (*Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi)(ADP_CPN_TO_LOCATOR, 
	&pop_cb);

    if (status)
	return (adu_error(adf_scb, E_AD7013_ADP_CPN_TO_LOCATOR, 0));

#ifdef xDEBUG
    {
      ADP_LOCATOR locval = 0;
      MEcopy (((char *)p + ADP_HDR_SIZE), sizeof(ADP_LOCATOR), &locval);
      TRdisplay("Created locator %d for coupon\n", locval);
    }
#endif

    return(status);
}

VOID
adu_free_locator(PTR 	    storage_location,
DB_DATA_VALUE	   *locator_dv)
{

    ADP_POP_CB        pop_cb;
    DB_STATUS         status;

    pop_cb.pop_type = ADP_POP_TYPE;
    pop_cb.pop_length = sizeof(ADP_POP_CB);
    pop_cb.pop_ascii_id = ADP_POP_ASCII_ID;
    pop_cb.pop_continuation = ADP_C_BEGIN_MASK;
    pop_cb.pop_user_arg = storage_location; /* this is the DML_SCB */
    pop_cb.pop_segment = locator_dv; /* NULL for ALL */

    status = (*Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi)
	                                           (ADP_FREE_LOCATOR, &pop_cb);
    
    return;
}

/*{
** Name:	adu_long_coerce_slave - Perform segment by segment coercion
**
** Description:
**	This routine simply provides the slave filter function for the
**      coericon of long objects.  For each segment with
**      which it is called, it calls the normal character string
**      coercion functions to perform that operation on that segment.
**      It then sets the next expected action (shd_exp_action) field to
**      ADW_CONTINUE, if the coerced data will fit. Otherwise, we hold
**	onto the residue and resurn ADW_FLUSH_SEGMENT.
**
**      This routine is integral to adu_long_coerce.
**
** Re-entrancy:
**      Totally.
**
** Inputs:
**      scb                              ADF session control block.
**      dv_in                            Ptr to DB_DATA_VALUE describing
**                                       input
**      dv_out                           Ptr to DB_DATA_VALUE describing
**                                       output area.
**      work                             Ptr to ADP_LO_WKSP workspace.
**
** Outputs:
**      scb->adf_errcb                   Filled as appropriate.
**      dv_out->db_length                Set correctly.
**      work->adw_shared.shd_exp_action  Always set to ADW_CONTINUE.
**
** Returns:
**	DB_STATUS
**
** Exceptions:
**      None.
**
** Side Effects: 
**      State is maintained in the passed context block.
**
** History:
**	19-May-2009 (kiria01) SIR121788
**         Created.
**      18-Aug-2010 (hanal04) Bug 124271
**         Correct setting of shd_l1_check. UTF-8 NVCH to VCH shows
**         the old code was wrong.
*/

/*
** Private context block setup in adu_long_coerse
*/
struct long_coerce_ctx {
    ADU_NORM1_FUNC *func;	/* Coercion function */
    DB_DATA_VALUE temp_dv;	/* Temp data in caller scope */
    char *chp;			/* Data ptr to unused */
    u_i2 left;			/* Overflow data to process */
    u_i2 multiplier;		/* chars or UCS2 */
};

static DB_STATUS
adu_long_coerce_slave(ADF_CB	    *scb,
		      DB_DATA_VALUE *dv_in,
		      DB_DATA_VALUE *dv_out,
		      ADP_LO_WKSP   *work)
{
    DB_STATUS status;
    struct long_coerce_ctx *ctx =
		    (struct long_coerce_ctx*)work->adw_filter_space;
    u_i2 size;
    register char *p = dv_out->db_data;

    /* Have we unfinished business? */
    if (ctx->chp)
    {
	/* Prepare work_dv as though just called (ctx->func) */
	size = ctx->left;
	*(u_i2*)p = size / ctx->multiplier;
	p += sizeof (u_i2);
	MEcopy(ctx->chp, ctx->left, p);
	ctx->chp = NULL;
	status = E_DB_OK;
    }
    else
    {
	/* ctx->temp_dv points at the same buffer as dv_out except
	** that the latter is length restricted. */
	if (status = (*ctx->func)(scb, dv_in, &ctx->temp_dv))
	    return status;
	size = *(u_i2*)p;
	p += sizeof (u_i2);
	size *= ctx->multiplier;
    }
    /* p is left pointing at the first byte after the count */

    if (size + DB_CNTSIZE <= dv_out->db_length)
    {
	/* Size ok */
	if (dv_out->db_datatype == DB_VCH_TYPE)
            work->adw_shared.shd_l1_check += size;
	else
	    work->adw_shared.shd_l1_check += size / ctx->multiplier;
	work->adw_shared.shd_exp_action = ADW_CONTINUE;
    }
    else
    {
	/* Too big - need to arrange a split. */
	ctx->left = size + DB_CNTSIZE - dv_out->db_length;
	ctx->chp = dv_out->db_data + dv_out->db_length;

	/* If dealing with variable length chars, might need to
	** adjust the end so that the last char is not cut */
	if (dv_out->db_datatype == DB_VCH_TYPE)
	{
	    register char *e = p + dv_out->db_length - DB_CNTSIZE;
	    register i4 l;
	    do
	    {
		l = CMbytecnt(p);
		p += l;
		work->adw_shared.shd_l1_check++;
	    } while (p < e);
	    if (p > e)
	    {
		l -= p - e;
		ctx->left += l;
		ctx->chp -= l;
		work->adw_shared.shd_l1_check--;
	    }
	}
	else
	    work->adw_shared.shd_l1_check +=
				(size - ctx->left) / ctx->multiplier;
	*(u_i2*)ctx->temp_dv.db_data = (size - ctx->left) /
		    ctx->multiplier;
	/* We are leaving data to be picked up next time round
	** so set ADW_FLUSH_SEGMENT so that the GET is skipped
	** after the PUT and we get called back for the rest. */
	work->adw_shared.shd_exp_action = ADW_FLUSH_SEGMENT;
    }
    return(status);
}

/*{
** Name:	adu_long_coerce - Perform coercion lob->lob
**
** Description:
**      This routine sets up the general purpose large object filter to
**      have it filter a long object to another long object.
**      It will set up the coupons, then call the general filter
**      function which will arrange to coerce each of the
**      segments, thus coercing the entire long object.
**
**      This routine makes use of the static adu_long_coerce_slave()
**      routine also defined in this file.  It is thru this slave
**      routine that the work actually gets done.
**
**	Locator types get couponified to the long type they represent
**	and if then there is no coercion required, it is handed off
**	to the more efficient adu_lvch_move to handle.
**
**	    NOTE: FI_DEFN using this function should use:
**		flags ADI_F4_WORKSPACE
**		lenspec ADI_FIXED sizeof(ADP_PERIPHERAL)
**		workspace (3*DB_MAXTUP)+sizeof(ADP_LO_WKSP)
**	    The multiplier, 3, can be dropped to 2 if neither
**	    datatype is LNVCHR.
**
**
** Re-entrancy:
**      Totally.
**
** Inputs:
**      scb                              ADF session control block.
**      dv_in                            Ptr to DB_DATA_VALUE describing
**                                       input coupon
**      dv_work                          Ptr to DB_DATA_VALUE which
**                                       points to an area large enough
**                                       to hold an ADP_LO_WKSP plus 2
**                                       segments (for input & output).
**      dv_out                           Ptr to DB_DATA_VALUE describing
**                                       output area in which the output
**                                       coupon will be created.
**
** Outputs:
**      scb->adf_errcb                   Filled as appropriate.
**      *dv_out->db_data                 Defines the output coupon
**
** Returns:
**	DB_STATUS
**
** Exceptions:
**      None.
**
** Side Effects: 
**      None.
**
** History:
**	19-Mar-2009 (kiria01) SIR121788
**         Created to complement adu_lvch_move.
*/
DB_STATUS
adu_long_coerce(ADF_CB      	*adf_scb,
		DB_DATA_VALUE  	*dv_in,
		DB_DATA_VALUE   *dv_work,
		DB_DATA_VALUE   *dv_out)
{
    DB_STATUS		status = E_DB_OK;
    ADP_PERIPHERAL	*inp_cpn = (ADP_PERIPHERAL *) dv_in->db_data;
    ADP_PERIPHERAL	*out_cpn = (ADP_PERIPHERAL *) dv_out->db_data;
    DB_DATA_VALUE	loc_dv;
    ADP_PERIPHERAL	cpn;
    ADP_LO_WKSP		*work = (ADP_LO_WKSP *) dv_work->db_data;
    struct long_coerce_ctx ctx; 
    i4			inbits;

    if (Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi == NULL)
	return(adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "lvch fexi null"));

    /* Fixup any locator passed in */
    if (status = adi_dtinfo(adf_scb, dv_in->db_datatype, &inbits))
	return status;
    if (inbits & AD_LOCATOR)
    {
	loc_dv = *dv_in;
	loc_dv.db_length = sizeof(ADP_PERIPHERAL);
	loc_dv.db_data = (PTR)&cpn;
	loc_dv.db_datatype = abs(dv_in->db_datatype)==DB_LCLOC_TYPE
			    ? DB_LVCH_TYPE
			    : abs(dv_in->db_datatype)==DB_LNLOC_TYPE
			    ? DB_LNVCHR_TYPE : DB_LBYTE_TYPE;
	if (status = adu_locator_to_cpn(adf_scb, dv_in, &loc_dv))
	    return status;
	dv_in = &loc_dv;
	inp_cpn = &cpn;
    }

    /* If no coersion really needed switch to the more direct
    ** code in adu_lvch_move */
    if (abs(dv_in->db_datatype) == abs(dv_out->db_datatype))
	return adu_lvch_move(adf_scb, dv_in, dv_out);

    if (inp_cpn->per_length0 || inp_cpn->per_length1)
    {
	i4 ratio;
	if (abs(dv_in->db_datatype) == DB_LNVCHR_TYPE ||
	    abs(dv_out->db_datatype) == DB_LNVCHR_TYPE)
	{
	    /* We are setting the buffer ratio to 3 to ensure that
	    ** 1/3 is the input size and 2/3 for output. The wksp
	    ** needs to be corespondingly sized in the FI_DEFN */
	    ratio = 3;
	    ctx.func = adu_nvchr_coerce;
	    if (abs(dv_out->db_datatype) == DB_LNVCHR_TYPE)
		ctx.multiplier = sizeof(UCS2);
	    else
		ctx.multiplier = sizeof(char);
	}
	else
	{
	    ratio = 2;
    	    ctx.func = adu_1strtostr;
	    ctx.multiplier = sizeof(char);
	}
	adu_lo_setup_workspace(adf_scb, dv_in, dv_out, dv_work, ratio);

	/* .fip_work_dv will be left pointing to the whole available buffer */
	ctx.temp_dv = work->adw_fip.fip_work_dv;
	ctx.chp = NULL;

	/* Pass for record overflow control */
	work->adw_filter_space = (PTR)&ctx;

	work->adw_shared.shd_l1_check = 0;
	work->adw_shared.shd_l0_check = 0;

	status = adu_lo_filter(adf_scb,
			dv_in,
			dv_out,
			adu_long_coerce_slave,
			work,
			(ADP_C_BEGIN_MASK | ADP_C_END_MASK),
                        NULL);
	out_cpn->per_length0 = work->adw_shared.shd_l0_check;
	out_cpn->per_length1 = work->adw_shared.shd_l1_check;
    }
    else
	*out_cpn = *inp_cpn;
    return status;
}


/*{
** Name:	adu_long_unorm - Perform unorm on lob->lob
**
** Description:
**      This routine sets up the general purpose large object filter to
**      have it filter a long object to another long object.
**      It will set up the coupons, then call the general filter
**      function which will arrange to unorm each of the
**      segments, thus normalising the entire long object.
**
**      This routine makes use of the static adu_long_coerce_slave()
**      routine also defined in this file.  It is thru this slave
**      routine that the work actually gets done. The callback function
**	does the unormalisation and the normal unicode coerce will do
**	the rest.
**
**	Locator types get couponified to the long type they represent
**	and if then there is no coercion required, it is handed off
**	to the more efficient adu_lvch_move to handle.
**
**
** Re-entrancy:
**      Totally.
**
** Inputs:
**      scb                              ADF session control block.
**      dv_in                            Ptr to DB_DATA_VALUE describing
**                                       input coupon
**      dv_work                          Ptr to DB_DATA_VALUE which
**                                       points to an area large enough
**                                       to hold an ADP_LO_WKSP plus 2
**                                       segments (for input & output).
**      dv_out                           Ptr to DB_DATA_VALUE describing
**                                       output area in which the output
**                                       coupon will be created.
**
** Outputs:
**      scb->adf_errcb                   Filled as appropriate.
**      *dv_out->db_data                 Defines the output coupon
**
** Returns:
**	DB_STATUS
**
** Exceptions:
**      None.
**
** Side Effects: 
**      None.
**
** History:
**	05-May-2009 (kiria01) b122030
**         Created to implement long unorm
*/
DB_STATUS
adu_long_unorm(ADF_CB      	*adf_scb,
		DB_DATA_VALUE  	*dv_in,
		DB_DATA_VALUE   *dv_out)
{
    DB_STATUS		status = E_DB_OK;
    ADP_PERIPHERAL	*inp_cpn = (ADP_PERIPHERAL *) dv_in->db_data;
    ADP_PERIPHERAL	*out_cpn = (ADP_PERIPHERAL *) dv_out->db_data;
    DB_DATA_VALUE	loc_dv;
    ADP_PERIPHERAL	cpn;
    DB_DATA_VALUE	dv_work;
    char		_work[(3*DB_MAXTUP)+sizeof(ADP_LO_WKSP)];
    ADP_LO_WKSP		*work = (ADP_LO_WKSP *)_work; 
    struct long_coerce_ctx ctx; 
    i4			inbits;

    dv_work.db_data = (PTR)&_work;
    dv_work.db_datatype = DB_CHA_TYPE;
    dv_work.db_length = sizeof(_work);
    dv_work.db_collID = 0;
    dv_work.db_prec = 0;

    if (Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi == NULL)
	return(adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "lvch fexi null"));

    /* Fixup any locator passed in */
    if (status = adi_dtinfo(adf_scb, dv_in->db_datatype, &inbits))
	return status;
    if (inbits & AD_LOCATOR)
    {
	loc_dv = *dv_in;
	loc_dv.db_length = sizeof(ADP_PERIPHERAL);
	loc_dv.db_data = (PTR)&cpn;
	loc_dv.db_datatype = abs(dv_in->db_datatype)==DB_LCLOC_TYPE
			    ? DB_LVCH_TYPE
			    : abs(dv_in->db_datatype)==DB_LNLOC_TYPE
			    ? DB_LNVCHR_TYPE : DB_LBYTE_TYPE;
	if (status = adu_locator_to_cpn(adf_scb, dv_in, &loc_dv))
	    return status;
	dv_in = &loc_dv;
	inp_cpn = &cpn;
    }

    if (inp_cpn->per_length0 || inp_cpn->per_length1)
    {
	i4 ratio;
	if (abs(dv_in->db_datatype) == DB_LNVCHR_TYPE ||
	    abs(dv_out->db_datatype) == DB_LNVCHR_TYPE)
	{
	    /* We are setting the buffer ratio to 3 to ensure that
	    ** 1/3 is the input size and 2/3 for output. The wksp
	    ** needs to be corespondingly sized in the FI_DEFN */
	    ratio = 3;
	    ctx.func = adu_unorm;
	    if (abs(dv_out->db_datatype) == DB_LNVCHR_TYPE)
		ctx.multiplier = sizeof(UCS2);
	    else
		ctx.multiplier = sizeof(char);
	}
	else
	{
	    ratio = 2;
    	    ctx.func = adu_utf8_unorm;
	    ctx.multiplier = sizeof(char);
	}
	adu_lo_setup_workspace(adf_scb, dv_in, dv_out, &dv_work, ratio);

	/* .fip_work_dv will be left pointing to the whole available buffer */
	ctx.temp_dv = work->adw_fip.fip_work_dv;
	ctx.chp = NULL;

	/* Pass for record overflow control */
	work->adw_filter_space = (PTR)&ctx;

	work->adw_shared.shd_l1_check = 0;
	work->adw_shared.shd_l0_check = 0;

	status = adu_lo_filter(adf_scb,
			dv_in,
			dv_out,
			adu_long_coerce_slave,
			work,
			(ADP_C_BEGIN_MASK | ADP_C_END_MASK),
                        NULL);
	out_cpn->per_length0 = work->adw_shared.shd_l0_check;
	out_cpn->per_length1 = work->adw_shared.shd_l1_check;
    }
    else
	*out_cpn = *inp_cpn;
    return status;
}

/*
** Name: adu_19lvch_chrlen - return length of lob in char units.
**	 
** Description:
**	This file implements length(long varchar) for UTF8.
**	Result is in char units.
**
** Inputs:
**      scb                              ADF session control block.
**      cpn_dv                           Ptr to DB_DATA_VALUE describing
**                                       input coupon
**      rdv                              Ptr to DB_DATA_VALUE describing
**                                       output area for resulting length
**
** Outputs:
**      scb->adf_errcb                   Filled as appropriate.
**      *rdv->db_data                    resulting length
**
** Returns:
**	DB_STATUS
**
** Exceptions:
**      None.
**
** Side Effects: 
**      None.
**
** History:
**	13-Oct-2010 (thaju02) B124469
**	    Created. 
*/
DB_STATUS
adu_19lvch_chrlen(ADF_CB	*scb,
		DB_DATA_VALUE	*cpn_dv,
		DB_DATA_VALUE	*rdv)
{
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		local_stat = E_DB_OK;
    ADP_PERIPHERAL	*inp_cpn = (ADP_PERIPHERAL *) cpn_dv->db_data;
    DB_DATA_VALUE	work_dv;
    ADP_LO_WKSP		*work = 0;
    i4			per_length1;
    i4			count = 0;
    i4			totcount = 0;
    bool                done = 0;
    ADP_POP_CB 		in_pop_cb;
    ADP_POP_CB		*pop_cb_ptr =  &in_pop_cb;
    DB_DATA_VALUE	under_dv, count_dv;
    DB_STATUS		(*fcn_fexi)() = NULL;

    for (;;)
    {
	work = (ADP_LO_WKSP *)MEreqmem(0, sizeof(ADP_LO_WKSP) +
                                            (2 * DB_MAXTUP), TRUE, &status);
	if (work == NULL || status != OK)
	    break;
	work_dv.db_data = (PTR)work;
	work_dv.db_length = sizeof(ADP_LO_WKSP) + (2 * DB_MAXTUP);
	work_dv.db_datatype = work_dv.db_prec = 0;
	work_dv.db_collID = -1;
	
	if ((inp_cpn->per_length0 != 0) || (inp_cpn->per_length1 != 0))
	{
	    if (status = adu_0lo_setup_workspace(scb, cpn_dv, &work_dv))
		break;

	    work->adw_shared.shd_l1_check = 0;
	    work->adw_shared.shd_l0_check = 0;

	    if (status = adi_per_under(scb, cpn_dv->db_datatype, &under_dv))
		break;

	    *pop_cb_ptr = work->adw_fip.fip_pop_cb;
	    pop_cb_ptr->pop_segment = &work->adw_fip.fip_seg_dv;
	    pop_cb_ptr->pop_coupon = cpn_dv;
	    pop_cb_ptr->pop_continuation = ADP_C_BEGIN_MASK;

            count_dv.db_data = (PTR) &count;
            count_dv.db_length = sizeof(count);
            count_dv.db_datatype = DB_INT_TYPE;
            count_dv.db_prec = 0;

	    fcn_fexi = Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi;
    
	    while (status == E_DB_OK &&
	    	!(done && work->adw_shared.shd_exp_action != ADW_FLUSH_SEGMENT))
	    {
		if (work->adw_shared.shd_exp_action != ADW_FLUSH_SEGMENT)
		{
		    if (status = (*fcn_fexi)(ADP_GET, pop_cb_ptr))
		    {
			if (DB_FAILURE_MACRO(status) ||
				(pop_cb_ptr->pop_error.err_code != 
				E_AD7001_ADP_NONEXT))
			    return adu_error(scb, 
				pop_cb_ptr->pop_error.err_code, 0);

			if (pop_cb_ptr->pop_error.err_code == 
				E_AD7001_ADP_NONEXT)
			{
			    status = E_DB_OK;
			    done = TRUE;
			}
		    }
		    pop_cb_ptr->pop_continuation &= ~ADP_C_BEGIN_MASK;
		    pop_cb_ptr->pop_continuation &= ~ADP_C_RANDOM_MASK;
		}

		if (status = adu_22charlength(scb, pop_cb_ptr->pop_segment, 
		    	&count_dv))
		    break;

		totcount += count;

		work->adw_shared.shd_exp_action = ADW_CONTINUE;

		if (done && work->adw_shared.shd_exp_action != ADW_FLUSH_SEGMENT)
		    pop_cb_ptr->pop_continuation |= ADP_C_END_MASK;
	    }
	}

        if (rdv->db_length == 2)
            *(i2 *)rdv->db_data = totcount;
        else
            *(i4 *)rdv->db_data = totcount;
	break;
    }

    /* clean up */
    local_stat = (*fcn_fexi)(ADP_CLEANUP, pop_cb_ptr);

    if (work)
	MEfree((PTR)work);

    return(status);
}
