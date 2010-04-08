/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <adp.h>
#include    <ulf.h>
#include    <cm.h>	/* For CMcopy(), for KANJI peripheral support */
#include    <me.h>

#include    <adfint.h>

/**
**
**  Name: ADCXFORM.C - Transform a Peripheral Datatype
**
**  Description:
**      This file contains the main datatype entry point and the support
**	routines for INGRES peripheral objects to convert a large object
**      into another style of a peripheral datatype.  This is used
**	throughout the INGRES system in converting peripheral datatypes to
**	coupons (via the adu_couponify() routine) and/or GCA style
**      (adu_redeem()).
**
**          adc_xform() - Generic transformation routine
**	    adc_lvch_xform() -- implementation for long varchar
**
**
**  History:
**      16-may-1990 (fred)
**          Created.
**	23-Nov-1992 (fred)
**	    Redone for better compatibility with OME Large Objects.
**      14-jan-1993 (stevet)
**          Added function prototypes.
**       6-Apr-1993 (fred)
**          Added long byte (DB_LBYTE_TYPE) support.  It is exactly
**          the same as varchar...
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**      12-jun-2001 (stial01)
**          Changes for DB_LNVCHR_TYPE
**      21-jun-2001 (stial01)
**          More changes for DB_LNVCHR_TYPE
**      25-jun-2001 (stial01)
**          More changes for DB_LNVCHR_TYPE
**      02-oct-2002 (stial01)
**          Added ADW_FLUSH_INCOMPLETE_SEGMENT for DB_LNVCHR_TYPE's
**          We may need to flush output buffer before it is full.
**	16-Oct-2003 (stial01, gupsh01)
**	    Added ADW_C_HALF_DBL_FLOC to indicate if half
**	    a double byte /unicode value is set in adc_chars. 
**
[@history_template@]...
**/

/*{
** Name: adc_xform	- Transform Periph Dt from one style to another.
**
** Description:
**      This routine is the generic entry point to ADF for transforming a
**	peripheral datatype into another style.  Peripheral
**	datatypes are stored as a list of segments of a datatype specific
**	non-peripheral (normal) datatype.  These styles exist to allow
**      the representation of peripheral datatypes suitable for a
**      variety of different purposes.
**
**  	The styles used are as follows:
**          GCA - (with/out length known up front).  These styles are
**             used to transmit and receive data from front ends.
**             These both have a standard large object header,
**             followed by a sequence of segments each of which is
**             preceded by an int (i4) which is non-zero if the
**             following segment is present.  Thus, these look like
**               <3 i4 header>
**               {<int: value 1> <segment>}...
**               <int: 0>
**             The {} enclosed line may be repeated, or, for a null or
**             zero length object, will be absent.
**
**          COUPON -- These are a sequence of segments of the
**             underlying type.  No segment indicator is present.
**
**          DATA -- This is just a raw byte stream.
**
**	If this routine returns a status of E_DB_INFO, with an error of
**	E_AD0002_INCOMPLETE, that indicates that the input to this routine has
**	been exhausted, and more is required.  The caller should materialize
**	this data using whatever means is appropriate, and call this routine
**	again.
**
**	E_DB_WARN/E_AD7001_ADP_NONEXT is returned when the last segment has
**	been processed.
**
** Inputs:
**      scb                             ADF Session control block.
**      workspace                       Ptr to ADP_LO_WKSP to be used
**                                      for this context.  This is
**                                      to be preserved intact
**                                      (subject to its usage) across
**                                      calls to this routine.
**          adw_adc                     Private area -- untouched but
**                                      preserved by caller
**          adw_shared                  Area used to communicate
**                                      between this routine and its
**                                      callers
**              shd_type                Datatype being xformed.
**              shd_exp_action          Instructions for caller and
**                                      callee.
**                                          ADW_START -- first call.
**                                          ADW_NEXT_SEGMENT &
**                                          ADW_GET_DATA -- last call
**                                              got more data from
**                                              segment.
**                                          ADW_FLUSH_SEGMENT -- Output
**                                              area full, flush it.
**                                      This value is set on output to
**                                      tell the caller what to do
**                                      (why returned).
**              shd_i_{area,length,used} Pointer to, length of, and
**                                      amount used of the input area
**                                      to be consumed by the routine.
**              shd_o_{area,length,used} Pointer to, length of, and
**                                      amount used of the output area
**                                      to be built ed by the routine.
**              shd_{inp,out}_segmented  Whether the input or output
**                                      objects are in a style that
**                                      is constructed of segments of
**                                      the underlying type.  A value
**                                      of 0 means a byte stream;
**                                      non-zero indicates segmented
**                                      types.  In general, this is
**                                      most of the information this
**                                      routine needs to glean from...
**              shd_{in,out}_tag        The actual type of large
**                                      object.  Irrelevant (outside
**                                      of above) in most cases, but
**                                      provided nonetheless.
** Outputs:
**      scb->adf_error....              Usual error stuff.
**      workspace                       Filled as appropriate.
**          adw_adc                     Private area -- caller must
**                                      preserve. 
**          adw_shared
**             shd_exp_action           Filled as above...
**             shd_i_used               Modified as used by routine
**             shd_o_used               Modified as used by routine
**             shd_l{0,1}_chk           Filled with the length of the
**                                      object being xform'd.  This is
**                                      checked (where possible) with
**                                      the input type for validation
**                                      of the object.
**
**	Returns:
**	  status/error:
**          E_DB_INFO/E_AD0002_INCOMPLETE
**                                      Caller should shd_exp_action
**                                      to determine its next action.
**          E_DB_ERROR/(anything)       Error found in converting
**                                      object.
**          Note that this routine cannot determine when it is at the
**                                      end of its work.  Thus, it
**                                      does not return an OK/Done
**                                      status;  it always views its
**                                      work as incomplete.  Its
**                                      caller should make sure that
**                                      the operation always ends when
**                                      this routine things it needs a
**                                      new segment.
**	Exceptions:
**	    None
**
** Side Effects:
**	    None
**
** History:
**      16-May-1990 (fred)
**          Created.
**      30-Nov-1992 (fred)
**          Redone to provide for simpler interface for OME large
**          objects. 
[@history_template@]...
*/
DB_STATUS
adc_xform(
ADF_CB             *scb,
PTR                ws)
{
    DB_STATUS           status;
    DB_DT_ID		dtid;
    ADP_LO_WKSP         *workspace=(ADP_LO_WKSP *)ws;

    dtid = ADI_DT_MAP_MACRO(abs(workspace->adw_shared.shd_type));
    
    if (    dtid <= 0
	 || dtid  > ADI_MXDTS
	 || Adf_globs->Adi_dtptrs[dtid] == NULL
	 || Adf_globs->Adi_dtptrs[dtid]->adi_dt_com_vect.adp_xform_addr
			== NULL
       )
	return(adu_error(scb, E_AD2004_BAD_DTID, 0));
    if (workspace->adw_shared.shd_type > 0)
    {
	status =
	    (*Adf_globs->Adi_dtptrs[dtid]->adi_dt_com_vect.adp_xform_addr)
				(scb, ws);
    }
    else
    {
	workspace->adw_shared.shd_type = -(workspace->adw_shared.shd_type);
	status =
	    (*Adf_globs->Adi_dtptrs[dtid]->adi_dt_com_vect.adp_xform_addr)
				(scb, ws);
	workspace->adw_shared.shd_type = -(workspace->adw_shared.shd_type);
    }
    return(status);
}

/*
** {
** Name: adc_lvch_xform	- Transform Long Varchar-style types
**
** Description:
**      This routine transforms a long varchar-style types into their
**      component segments.  It can transform any non-coupon type into
**      a coupon (DBMS style) type.
**	
**
** Inputs:
**      scb                             ADF Session control block.
**      workspace                       Ptr to ADP_LO_WKSP to be used
**                                      for this context.  This is
**                                      to be preserved intact
**                                      (subject to its usage) across
**                                      calls to this routine.
**          adw_adc                     Private area -- untouched but
**                                      preserved by caller
**          adw_shared                  Area used to communicate
**                                      between this routine and its
**                                      callers
**              shd_type                Datatype being xformed.
**              shd_exp_action          Instructions for caller and
**                                      callee.
**                                          ADW_START -- first call.
**                                          ADW_NEXT_SEGMENT &
**                                          ADW_GET_DATA -- last call
**                                              got more data from
**                                              segment.
**                                          ADW_FLUSH_SEGMENT -- Output
**                                              area full, flush it.
**                                      This value is set on output to
**                                      tell the caller what to do
**                                      (why returned).
**              shd_i_{area,length,used} Pointer to, length of, and
**                                      amount used of the input area
**                                      to be consumed by the routine.
**              shd_o_{area,length,used} Pointer to, length of, and
**                                      amount used of the output area
**                                      to be built ed by the routine.
**              shd_{inp,out}_segmented  Whether the input or output
**                                      objects are in a style that
**                                      is constructed of segments of
**                                      the underlying type.  A value
**                                      of 0 means a byte stream;
**                                      non-zero indicates segmented
**                                      types.  In general, this is
**                                      most of the information this
**                                      routine needs to glean from...
**              shd_{in,out}_tag        The actual type of large
**                                      object.  Irrelevant (outside
**                                      of above) in most cases, but
**                                      provided nonetheless.
** Outputs:
**      scb->adf_error....              Usual error stuff.
**      workspace                       Filled as appropriate.
**          adw_adc                     Private area -- caller must
**                                      preserve. 
**          adw_shared
**             shd_exp_action           Filled as above...
**             shd_i_used               Modified as used by routine
**             shd_o_used               Modified as used by routine
**             shd_l{0,1}_chk           Filled with the length of the
**                                      object being xform'd.  This is
**                                      checked (where possible) with
**                                      the input type for validation
**                                      of the object.
**
**	Returns:
**	  status/error:
**          E_DB_INFO/E_AD0002_INCOMPLETE
**                                      Caller should shd_exp_action
**                                      to determine its next action.
**          E_DB_ERROR/(anything)       Error found in converting
**                                      object.
**          Note that this routine cannot determine when it is at the
**                                      end of its work.  Thus, it
**                                      does not return an OK/Done
**                                      status;  it always views its
**                                      work as incomplete.  Its
**                                      caller should make sure that
**                                      the operation always ends when
**                                      this routine things it needs a
**                                      new segment.
**	Exceptions:
**	    None
**
** Side Effects:
**	    None
**
** History:
**    30-Nov-1992 (fred)
**       Created.
**    6-Apr-1993 (fred)
**       Added long byte support.  This differs from long varchar only
**       in that with long byte, we don't have to (indeed should not)
**       do the double byte machinations that character strings
**       require. 
**    20-Oct-1993 (fred)
**    	 Removed debug code which kept segment count in l0 field.  It
**       tended to mess STAR up.  (Rightfully.)
**    24-Oct-2001 (thaju02)
**	 If the output buffer does not have enough space for the 
**	 2-byte segment length indicator, then we have to flush
**	 the buffer. Note buffer already contains the 4 byte
**	 "more segment/end-of-data" indicator, thus upon re-entering
**	 adu_redeem, make sure that we do not send out another
**	 "more segment/end-of-data" indicator. (B104122)	    	
**    16-Oct-2003 (stial01, gupsh01)
**	  Added ADW_C_HALF_DBL_FLOC to indicate if half
**	  a unicode value is set in adc_chars.
**    14-Dec-2005 (wanfr01)
**        Bug 115140, INGSRV3426
**        When appending the second byte of a double byte
**	  character to the buffer, remember to bump up the
**	  byte count by one.
**	24-May-2007 (gupsh01)
**	  Added support to store partial bytes for a multibyte
**	  character.
**    28-Aug-2007 (thaju02) 
**	 For long nvarchar, there needs to be  enough space in the 
**	 output buffer for 2-byte segment length and first nchar,
**	 otherwise flush buffer. Upon re-entering adu_redeem do not 
**	 output another "more segment/end-of-data" indicator. Extend 
**	 fix for bug 104122 to long nvarchar. (B119016)
*/
DB_STATUS
adc_lvch_xform(
ADF_CB	      *scb,
PTR           ws)
{
    ADP_LO_WKSP     *workspace=(ADP_LO_WKSP *)ws;
    DB_STATUS	    status = E_DB_OK;
# define		START		0
# define		GET_SEG_LENGTH	1
# define		GET_SEGMENT	2
# define	        END_SEGMENT	3
# define		COMPLETE	4
    i4		    old_state = 1;
    i2		    count;
    i2              *countloc;
    DB_TEXT_STRING  *vch;
    i4		    done;
    i4		    data_available;
    i4		    moved;
    i4		    offset;
    i4		    really_moved;
    i4		    space_left;
    char	    *p;
    char            *chp;

    if (workspace->adw_shared.shd_exp_action == ADW_START)
    {	
	workspace->adw_adc.adc_state = START;
    }
    else if (workspace->adw_adc.adc_state < 0)
    {
	/*
	**  Recover from a negative state -- this indicates that the previous
	**  call returned with an incomplete status.  Therefore, the caller
	**  will have provided a new input buffer -- of which we have used no
	**  information
	*/

	workspace->adw_adc.adc_state = -workspace->adw_adc.adc_state;
    }
    p = ((char *) workspace->adw_shared.shd_i_area) +
	    	    	    	workspace->adw_shared.shd_i_used;

    if ( (workspace->adw_shared.shd_exp_action == ADW_FLUSH_SEGMENT) ||
	 (workspace->adw_shared.shd_exp_action == ADW_FLSH_SEG_NOFIT_LIND) )
    {
	/* If the last thing we did was flush a segment, then we need */
	/* to store the new location in which to store the segment */
	/* count.  We keep the the beginning of this area's offset in */
	/* the adc_longs[ADW_L_COUNTLOC] entry.  Its position is the */
	/* number of bytes used at the moment. */

	workspace->adw_adc.adc_longs[ADW_L_COUNTLOC] = 
					 workspace->adw_shared.shd_o_used;
    }
    countloc = (i2 *) &workspace->adw_shared.shd_o_area[
			     workspace->adw_adc.adc_longs[ADW_L_COUNTLOC]];
			     

    workspace->adw_shared.shd_exp_action = ADW_CONTINUE;

    for (done = 0; !done; )
    {
	switch (workspace->adw_adc.adc_state)
	{
	case START:
	    
	    workspace->adw_adc.adc_need = 0;
	    workspace->adw_shared.shd_l0_check = 0;
	    workspace->adw_shared.shd_l1_check = 0;
	    workspace->adw_adc.adc_longs[ADW_L_CPNBYTES] = 0;

	    count = 0;
	    workspace->adw_adc.adc_longs[ADW_L_COUNTLOC] = 
					 workspace->adw_shared.shd_o_used;
	    countloc = (i2 *) &workspace->adw_shared.shd_o_area[
				  workspace->adw_adc.adc_longs[ADW_L_COUNTLOC]];
	    workspace->adw_adc.adc_longs[ADW_L_BYTES] = 0;

	    workspace->adw_adc.adc_chars[ADW_C_HALF_DBL] = '\0';
	    workspace->adw_adc.adc_chars[ADW_C_HALF_DBL_FLOC] = 
						ADW_C_HALF_DBL_CLEAR;
	    workspace->adw_adc.adc_state = GET_SEG_LENGTH;
	    old_state = 0;
	    break;
	    
	case GET_SEG_LENGTH:
	    if (workspace->adw_shared.shd_inp_segmented)
	    {
		/* Then this data comes with a length up front. */
		if (!old_state++)
		{
		    workspace->adw_adc.adc_need = sizeof(i2);
		}
		data_available =
		    workspace->adw_shared.shd_i_length -
			workspace->adw_shared.shd_i_used;

		if (data_available > 0)
		{
		    moved = min(workspace->adw_adc.adc_need, data_available);
		    offset = sizeof(i2) - workspace->adw_adc.adc_need;
		    MEcopy(p, moved, &workspace->adw_adc.adc_memory[offset]);
		    data_available -= moved;
		    workspace->adw_shared.shd_i_used += moved;
		    p += moved;
		    workspace->adw_adc.adc_need -= moved;
		    if (workspace->adw_adc.adc_need == 0)
		    {
			I2ASSIGN_MACRO(*workspace->adw_adc.adc_memory,
				workspace->adw_adc.adc_shorts[ADW_S_COUNT]);
			if (workspace->adw_shared.shd_type == DB_LNVCHR_TYPE)
				 workspace->adw_adc.adc_shorts[ADW_S_COUNT] *= sizeof(UCS2);
			workspace->adw_adc.adc_state = GET_SEGMENT;
			old_state = 0;
		    }
		    else
		    {
			workspace->adw_shared.shd_exp_action = ADW_GET_DATA;
			done = 1;
		    }
		}
		else
		{
		    workspace->adw_shared.shd_exp_action = ADW_GET_DATA;
		    done = 1;
		}
		
	    }
	    else
	    {
		/*
		**	If this is not a coupon which has segments embedded as
		**	varchar style strings (i.e. with embedded lengths),
		**	then we treat each call as a segment unto itself.
		*/
		
		workspace->adw_adc.adc_shorts[ADW_S_COUNT] =
		    workspace->adw_shared.shd_i_length;
		workspace->adw_adc.adc_state = GET_SEGMENT;
		old_state = 0;
	    }

	    break;
	    
	case GET_SEGMENT:
	    
	    if (!old_state++)
	    {
		workspace->adw_adc.adc_need =
		    workspace->adw_adc.adc_shorts[ADW_S_COUNT];
	    }
	    
	    /*
	    ** If this is the first time thru here and we are
	    ** beginning a new segment, then we need to initialize the
	    ** segment, and account for the space used by it.
	    */

	    if (workspace->adw_shared.shd_o_used ==
		        workspace->adw_adc.adc_longs[ADW_L_COUNTLOC])
	    {
		count = 0;
		workspace->adw_adc.adc_longs[ADW_L_BYTES] = 0;
		if (workspace->adw_shared.shd_out_segmented)
		{
		    if ( (workspace->adw_shared.shd_o_length - 
				workspace->adw_shared.shd_o_used) > 
			 (sizeof(i2) + ((workspace->adw_shared.shd_type 
				== DB_LNVCHR_TYPE) ? sizeof(UCS2) : 0)) )
		    {
			/* reserve space for segment length in output buffer */
		    	workspace->adw_shared.shd_o_used += sizeof(i2);
		    	I2ASSIGN_MACRO(count, *countloc);
		    }
		    else
		    {
			/*
			** unable to fit 2-byte segment length indicator
			** in buffer, so flush it out and left adu_redeem
			** know that we have sent out the eod indicator
			** but no segment length
			*/
			workspace->adw_shared.shd_exp_action = 
						ADW_FLSH_SEG_NOFIT_LIND;
			done = 1;
			break;
		    }
		}
	    }
	    
	    data_available =
		workspace->adw_shared.shd_i_length -
		    workspace->adw_shared.shd_i_used;

	    chp = ((char *) &workspace->adw_shared.shd_o_area[
					  workspace->adw_shared.shd_o_used]);

	    space_left = workspace->adw_shared.shd_o_length
		    - workspace->adw_shared.shd_o_used;

	    if (data_available <= 0)
	    {
		workspace->adw_shared.shd_exp_action = ADW_GET_DATA;
		done = 1;
		break;
	    }
	    else if (workspace->adw_adc.adc_chars[ADW_C_HALF_DBL_FLOC] != 
                                                         ADW_C_HALF_DBL_CLEAR)
	    {
		/* Get the second 1/2 of a double byte/long nvarchar value */
		if (space_left < 2)
		{
		    /* Then dbl byte won't fit -- get out now */
		    
		    workspace->adw_shared.shd_exp_action = ADW_FLUSH_SEGMENT;
		    done = 1;
		    break;
		}
		chp[0] = workspace->adw_adc.adc_chars[ADW_C_HALF_DBL];
		workspace->adw_adc.adc_chars[ADW_C_HALF_DBL] = '\0';
	        workspace->adw_adc.adc_chars[ADW_C_HALF_DBL_FLOC] = 
							ADW_C_HALF_DBL_CLEAR;
		chp[1] = *p++;
		chp += 2;
		workspace->adw_adc.adc_longs[ADW_L_BYTES] += 2;
		data_available -= 1;
		workspace->adw_shared.shd_i_used += 1;
		workspace->adw_shared.shd_o_used  += 2;
		workspace->adw_adc.adc_need -= 1;
		space_left -= 2;
		count++;
		I2ASSIGN_MACRO(count, *countloc);
		
		/* adjust the coupon bytes and shd_l1_check */
		if (workspace->adw_shared.shd_type == DB_LNVCHR_TYPE)
		{
		    workspace->adw_adc.adc_longs[ADW_L_CPNBYTES] += 2;
		    workspace->adw_shared.shd_l1_check = 
		    workspace->adw_adc.adc_longs[ADW_L_CPNBYTES] / sizeof(UCS2);
		}
		else
		{
		    workspace->adw_shared.shd_l1_check += 2;
		}

		if (data_available <= 0)
		{
		    workspace->adw_shared.shd_exp_action = ADW_GET_DATA;
		    done = 1;
		    break;
		}
	    }
	    else if (workspace->adw_shared.shd_type == DB_LNVCHR_TYPE)
	    {
		if (space_left > 0 && space_left < sizeof(UCS2))
		{
		    /* Then nchar char won't fit -- get out now */
		    
		    workspace->adw_shared.shd_exp_action = ADW_FLUSH_INCOMPLETE_SEGMENT;
		    done = 1;
		    break;
		}

		if (space_left % sizeof(UCS2) != 0)
		    space_left = (space_left / sizeof(UCS2)) * sizeof(UCS2);
	    }

	    moved = min(workspace->adw_adc.adc_need, data_available);
	    
	    if (space_left > 0)
	    {
		moved = min(moved, space_left);
		
		if (workspace->adw_shared.shd_type == DB_LVCH_TYPE)
		{
		    really_moved = CMcopy(p, moved, chp);
		}
		else if (workspace->adw_shared.shd_type == DB_LNVCHR_TYPE)
		{
		   /* Move long nvarchar only in units of UCS2 */
		   really_moved = moved / sizeof(UCS2) * sizeof(UCS2);
		   if (really_moved > 0)
		     MEcopy(p, really_moved, chp);
		}
		else
		{
		    /*
		    ** If the datatype being munged on is not a
		    ** character type, then we don't worry about the
		    ** double-byte-isms with which  we must otherwise
		    ** deal when munging on characters.
		    */

		    MEcopy(p, moved, chp);
		    really_moved = moved;
		}
		workspace->adw_adc.adc_need -= really_moved;
		workspace->adw_shared.shd_i_used += really_moved;
		workspace->adw_shared.shd_o_used += really_moved;

		/* Since the coupon is in units,
		** we must keep track of data moved in units too
		** For DB_LVCH_TYPE   chars = bytes
		** For DB_LNVCHR_TYPE chars = bytes / sizeof(UCS2)
		*/
		workspace->adw_adc.adc_longs[ADW_L_BYTES] += really_moved;
		if (workspace->adw_shared.shd_type == DB_LNVCHR_TYPE)
		    count =  workspace->adw_adc.adc_longs[ADW_L_BYTES] / sizeof(UCS2);
		else
		    count =  workspace->adw_adc.adc_longs[ADW_L_BYTES];

		if (workspace->adw_shared.shd_out_segmented)
		{
		    I2ASSIGN_MACRO(count, *countloc);
		}
		p += really_moved;

		if (workspace->adw_shared.shd_type == DB_LNVCHR_TYPE)
		{
		    u_i4	l1_bytes;

		    l1_bytes = workspace->adw_adc.adc_longs[ADW_L_CPNBYTES];
		    l1_bytes += really_moved;
		    I4ASSIGN_MACRO(l1_bytes, 
			workspace->adw_adc.adc_longs[ADW_L_CPNBYTES]); 
		    workspace->adw_shared.shd_l1_check = l1_bytes / sizeof(UCS2);
		}
		else
		    workspace->adw_shared.shd_l1_check += really_moved;

		if (moved != really_moved)
		{   /* Store the extra bytes in adc_chars, and set the flag */ 

		    i4  tbytes, partial_bytes = moved - really_moved;
		    char *outloc;
	          
                    if ((partial_bytes < 1) && 
			(partial_bytes > ADW_C_HALF_DBL_MAXCNT)) 
		    { 
			/* something really bad happened */
	    		status = adu_error(scb, E_AD9999_INTERNAL_ERROR, 0);
	    		break;
		    }
		    outloc = &workspace->adw_adc.adc_chars[ADW_C_HALF_DBL];

		    /* Copy the bytes */
		    for (tbytes = partial_bytes; tbytes; tbytes--)
		    {
		      *outloc++ = *p++;
		    }

		    /* set the count */
	 	    workspace->adw_adc.adc_chars[ADW_C_HALF_DBL_FLOC] = partial_bytes;
	    	    workspace->adw_adc.adc_need -= partial_bytes;
	    	    workspace->adw_shared.shd_i_used += partial_bytes;
		}

		if (workspace->adw_adc.adc_need == 0)
		{
		    workspace->adw_adc.adc_state = END_SEGMENT;
		    old_state = 0;
		}

		/*
		** Else, go around loop and figure out what we're out
		** of.
		*/
	    }
	    else
	    {
		workspace->adw_shared.shd_exp_action =
                                         ADW_FLUSH_SEGMENT;
		done = 1;
	    }
	    break;
	    
	case END_SEGMENT:
	    if (!old_state++)
	    {
		/*
		** If we are at the end of a segment, then we must
		** return to our caller so that it can prepare the
		** next segment for our consumption.  This
		** preparation may involve some manipulation of the
		** input buffer.
		**
		** The first time thru here (signified by oldstate ==
		** 0), we will tell our caller that we need a new
		** segment (exp_action = ADW_NEXT_SEGMENT).  When we
		** return here, (oldstate != 0), we move on to the
		** beginning of segment processing.
		*/
	   
		workspace->adw_shared.shd_exp_action =
		    ADW_NEXT_SEGMENT;
		done = 1;
	    }
	    else
	    {
		workspace->adw_adc.adc_state = GET_SEG_LENGTH;
		old_state = 0;
	    }
	    break;

	    
	case COMPLETE:
	    
	    if (workspace->adw_adc.adc_chars[ADW_C_HALF_DBL])
	    {
		status = adu_error(scb, E_AD7004_ADP_BAD_BLOB, 0);
	    }
	    else
	    {
		if (workspace->adw_shared.shd_o_used)
		{
		    workspace->adw_shared.shd_exp_action =
			ADW_FLUSH_SEGMENT;
		    done = 1;
		}
	    }
	    break;
	    
	    
	default:
	    status = adu_error(scb, E_AD9999_INTERNAL_ERROR, 0);
	    break;
	}
	
	if ((workspace->adw_shared.shd_exp_action != ADW_CONTINUE)
	    || (status != E_DB_OK))
	    break;
    }
    if (    (status == E_DB_OK)
	&&  (workspace->adw_shared.shd_exp_action != ADW_CONTINUE))
    {
	/*
	**  Here, we have exhausted the users input buffer, and must
	**  request more.  Since the next call will be made with a new buffer,
	**  we will mark that nothing has been used here, and correct the used
	**  count on the way in.  We mark this state by making the state
	**  negative.  We do not reset the used field here, since that would
	**  not provide the appropriate information to the caller.
	*/
	
	workspace->adw_adc.adc_state = -workspace->adw_adc.adc_state;
	status = E_DB_INFO;
	scb->adf_errcb.ad_errcode = E_AD0002_INCOMPLETE;
    }
    return(status);
}
