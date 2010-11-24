/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <cm.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <adfhist.h>
#include    <clfloat.h>
/*
**
**  Name: ADCHDEC.C - Decrement a histogram value.
**
**  Description:
**      This file contains a routine necessary to perform the  external ADF
**	call "adc_hdec()" which will decrement a histogram value so return
**	a new value useful for creating the lower bound of a histogram cell.
**
**	IMPORTANT NOTE:
**	===============
**	Addition of this function does not imply a change to the UDT interface.
**	The input DB_DATA_VALUE represents a histogram value and all histogram
**	datatypes are currently intrinsic, therefore there is no need to
**	reimplement this routine when a UDT is added. This will have to change
**	when we decide that a histogram datatype can be a UDT.
**
**      This file defines:
**
**          adc_hdec()	-   Generate lower bound for a histogram cell.
**
**  History:
**      05-feb-91 (stec)
**          Initial creation.
**	24-sep-1992 (fred)
**	    Added code to support BIT datatypes.  Also corrected character
**	    variable declarations in DB_CHA_TYPE section to use unsigned
**	    character variables.  Using signed characters would cause problems
**	    when working with characters whose sign bit is on (decrement
**	    would actually increment -- oops).
**      22-dec-1992 (stevet)
**          Added function prototype.
**       6-Apr-1993 (fred)
**          Added byte string datatypes.
**	29-apr-1993 (robf)
**	    Add handling for security labels.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	30-nov-94 (ramra01)
**		change cast (char) to (u_char) for function adc_hdec() 
**		Part of the code integ from 6.4.
**	18-Jun-1999 (schte01)
**       Added casts and casted assigns to avoid unaligned access in axp_osf.   
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	 7-mar-2001 (mosjo01)
**          Had to make MAX_CHAR more unique, IIMAX_CHAR_VALUE, due to conflict
**	    between compat.h and system limits.h on (sqs_ptx).
**      21-sep-2009 (joea)
**          Add case for DB_BOO_TYPE in adc_hdec.
**      09-mar-2010 (thich01)
**          Add DB_NBR_TYPE like DB_BYTE_TYPE for rtree indexing.
*/

/*{
**
** Description:
**      This routine will decrement a histogram value so that any values 
**      which map into the histogram cell created for this value will
**	satisfy the following condition:
**
**		new_hist_value < mapped_value <= orig_hist_value
**	where:
**		new_hist_value	- value output by this procedure
**		orig_hist_value	- value input to this procedure
** 
**      The routine is needed to create cells in which the adjacent cell is 
**      empty, since histogram cells have the hist_lower < value <= hist_upper
**      property.  Thus, the cell beside this one could be indicated as having
**      no values if hist_lower is chosen accordingly. 
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adc_dvfhg			Pointer to the data value to be
**                                      converted.
**		.db_datatype		Datatype id of data value to be
**					converted.
**		.db_length  		Length of data value to be
**					converted.
**		.db_data    		Pointer to the actual data for
**		         		data value to be converted.
**      adc_dvthg                       Pointer to the data value to be
**					created.
**		.db_datatype		Datatype id of data value being
**					created.  (Must be consistent
**					with what is expected for the
**					from datatype and length  (i.e.
**					what adc_hg_dtln() returns as
**					the datatype that a data value
**					of the from datatype and length
**					gets mapped into to create a
**					histogram element).
**		.db_length  		Length of data value being
**					created.  (Must be consistent
**					with what is expected for the
**					from datatype and length  (i.e.
**					what adc_hg_dtln() returns as
**					the length that a data value
**					of the from datatype and length
**					gets mapped into to create a
**					histogram element).
**		.db_data    		Pointer to location to put the
**					actual data for the data value
**					being created.
**
** Outputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.  If an
**					error occurs the following fields will
**					be set.  NOTE: if .ad_ebuflen = 0 or
**					.ad_errmsgp = NULL, no error message
**					will be formatted.
**		.ad_errcode		ADF error code for the error.
**		.ad_errclass		Signifies the ADF error class.
**		.ad_usererr		If .ad_errclass is ADF_USER_ERROR,
**					this field is set to the corresponding
**					user error which will either map to
**					an ADF error code or a user-error code.
**		.ad_emsglen		The length, in bytes, of the resulting
**					formatted error message.
**		.adf_errmsgp		Pointer to the formatted error message.
**      adc_dvthg                       The resulting data value.
**	       *.db_data    		The actual data for the data
**					value created will be placed
**					here.
**
**      Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**          E_AD0000_OK                 Operation succeeded.
**          E_AD2004_BAD_DTID           Datatype id unknown to ADF.
**	    E_AD3001_DTS_NOT_SAME	Datatypes of source and destination
**					are different.
**          E_AD2005_BAD_DTLEN          Internal length is illegal for
**					the given datatype.
**          E_AD3011_BAD_HG_DTID        Datatype for the resulting data
**					value is not correct for the
**					"hg" mapping on the from datatype
**					and length.
**
**      Exceptions:
**          none
**
** Side Effects:
**          Will use values define in the ADC_HDEC struct in the
**	    ADF server control block.
**
** History:
**      05-feb-91 (stec)
**          Based on code removed from opqoptdb.sc.
**	24-sep-1992 (fred)
**	    Added code to support BIT datatypes.  Also corrected character
**	    variable declarations in DB_CHA_TYPE section to use unsigned
**	    character variables.  Using signed characters would cause problems
**	    when working with characters whose sign bit is on (decrement
**	    would actually increment -- oops).
**       6-Apr-1993 (fred)
**          Added byte string datatypes.
**	21-jun-93 (rganski)
**	    b50012: changed handling of completely null-filled character-type
**	    element: instead of copying src to dst, copy all null characters to
**	    dst; this is necessary because this function is often called with
**	    src and dst pointing to same location (in-place decrementation).
**	    This avoids problem for c type where decremented null string
**	    becomes higher than null string.
**	30-Nov-94 (ramra01)
**		b64336: Code integration of 6.4. The only change is (char) to (u_char)      
**		cast.	
**	12-may-04 (inkdo01)
**	    Add BIGINT support - note special code for axp_osf has been 
**	    commented out.
**	29-Jun-05 (hweho01)
**	    Uncommented the calling of I8CAST_MACRO for axp_osf.  
*/
DB_STATUS
adc_hdec(
ADF_CB              *adf_scb,
DB_DATA_VALUE       *adc_dvfhg,
DB_DATA_VALUE       *adc_dvthg)
{
    DB_STATUS   db_stat = E_DB_OK;
    i4		fhg_bdt	 = abs((i4) adc_dvfhg->db_datatype);
    i4		thg_bdt  = abs((i4) adc_dvthg->db_datatype);
    i4		fhg_bdtv;
    i4		thg_bdtv;
    PTR		src;
    PTR		dst;

    for (;;)
    {
        /* Check the consistency of input arguments */
	fhg_bdtv = ADI_DT_MAP_MACRO(fhg_bdt);
	thg_bdtv = ADI_DT_MAP_MACRO(thg_bdt);

	if (    fhg_bdtv <= 0  ||  fhg_bdtv > ADI_MXDTS
	    ||  Adf_globs->Adi_dtptrs[fhg_bdtv] == NULL
	    ||  thg_bdtv <= 0  ||  thg_bdtv > ADI_MXDTS
	    ||  Adf_globs->Adi_dtptrs[thg_bdtv] == NULL
	   )
	{
	    db_stat = adu_error(adf_scb, E_AD2004_BAD_DTID, 0);
	    break;
	}

	if ( fhg_bdtv != thg_bdtv )
	{
	    db_stat = adu_error(adf_scb, E_AD3001_DTS_NOT_SAME, 0);
	    break;
	}

	src = adc_dvfhg->db_data;
	dst = adc_dvthg->db_data;

	switch (adc_dvfhg->db_datatype)
	{

        case DB_BOO_TYPE:
	    if (((DB_ANYTYPE *)src)->db_booltype == DB_FALSE)
	        ((DB_ANYTYPE *)dst)->db_booltype = (i1)-1;
            else if (((DB_ANYTYPE *)src)->db_booltype == DB_TRUE)
	        ((DB_ANYTYPE *)dst)->db_booltype = DB_FALSE;
            else
	        db_stat = adu_error(adf_scb, E_AD3011_BAD_HG_DTID, 0);
            break;

	case DB_INT_TYPE:
	{
	    switch (adc_dvfhg->db_length)
	    {
	    case 1:
#ifdef axp_osf
		if (I1_CHECK_MACRO(
		    CHCAST_MACRO(&(((DB_ANYTYPE *)src)->db_i1type))) == MINI1)
		    CHCAST_MACRO(&(((DB_ANYTYPE *)dst)->db_i1type)) = MINI1;
		else
		    CHCAST_MACRO(&(((DB_ANYTYPE *)dst)->db_i1type)) =
		    I1_CHECK_MACRO(
		    CHCAST_MACRO(&(((DB_ANYTYPE *)src)->db_i1type))) - 1;
#else
		if (I1_CHECK_MACRO(((DB_ANYTYPE *)src)->db_i1type) == MINI1)
		    ((DB_ANYTYPE *)dst)->db_i1type = MINI1;
		else
		    ((DB_ANYTYPE *)dst)->db_i1type =
			I1_CHECK_MACRO(((DB_ANYTYPE *)src)->db_i1type) - 1;
#endif
		break;

	    case 2:
#ifdef axp_osf
		if (I2CAST_MACRO(&(((DB_ANYTYPE *)src)->db_i2type)) == MINI2)
		     I2CAST_MACRO(&(((DB_ANYTYPE *)dst)->db_i2type)) = MINI2;
		else
		    I2CAST_MACRO(&(((DB_ANYTYPE *)dst)->db_i2type)) =
			I2CAST_MACRO(&(((DB_ANYTYPE *)src)->db_i2type)) - 1;
#else
		if (((DB_ANYTYPE *)src)->db_i2type == MINI2)
		    ((DB_ANYTYPE *)dst)->db_i2type = MINI2;
		else
		    ((DB_ANYTYPE *)dst)->db_i2type =
			((DB_ANYTYPE *)src)->db_i2type - 1;
#endif
		break;

	    case 4:
#ifdef axp_osf
		if (I4CAST_MACRO(&(((DB_ANYTYPE *)src)->db_i4type)) == MINI4)
		     I4CAST_MACRO(&(((DB_ANYTYPE *)dst)->db_i4type)) = MINI4;
		else
		    I4CAST_MACRO(&(((DB_ANYTYPE *)dst)->db_i4type)) =
			I4CAST_MACRO(&(((DB_ANYTYPE *)src)->db_i4type)) - 1;
#else
		if (((DB_ANYTYPE *)src)->db_i4type == MINI4)
		    ((DB_ANYTYPE *)dst)->db_i4type = MINI4;
		else
		    ((DB_ANYTYPE *)dst)->db_i4type =
			((DB_ANYTYPE *)src)->db_i4type - 1;
#endif
		break;

	    case 8:
#ifdef axp_osf
		if (I8CAST_MACRO(&(((DB_ANYTYPE *)src)->db_i8type)) == MINI8)
		     I8CAST_MACRO(&(((DB_ANYTYPE *)dst)->db_i8type)) = MINI8;
		else
		    I8CAST_MACRO(&(((DB_ANYTYPE *)dst)->db_i8type)) =
			I8CAST_MACRO(&(((DB_ANYTYPE *)src)->db_i8type)) - 1;
#else 
		if (((DB_ANYTYPE *)src)->db_i8type == MINI8)
		    ((DB_ANYTYPE *)dst)->db_i8type = MINI8;
		else
		    ((DB_ANYTYPE *)dst)->db_i8type =
			((DB_ANYTYPE *)src)->db_i8type - 1;
#endif 
		break;

	    default:
		db_stat = adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0);
		break;
	    }
	    break;
	}

	case DB_FLT_TYPE:
	{
	    switch (adc_dvfhg->db_length)
	    {
	    case 4:
#ifdef axp_osf
		if (F4CAST_MACRO(&(((DB_ANYTYPE *)src)->db_f4type)) > 0.0)
		{
		    F4CAST_MACRO(&(((DB_ANYTYPE *)dst)->db_f4type)) =
			Adf_globs->Adc_hdec.fp_f4pos *
			    F4CAST_MACRO(&(((DB_ANYTYPE *)src)->db_f4type));
		}
		else if (F4CAST_MACRO(&(((DB_ANYTYPE *)src)->db_f4type)) < 0.0)
		{
		    F4CAST_MACRO(&(((DB_ANYTYPE *)dst)->db_f4type)) =
			Adf_globs->Adc_hdec.fp_f4neg *
			    F4CAST_MACRO(&(((DB_ANYTYPE *)src)->db_f4type));
		}
		else
		{
		    F4CAST_MACRO(&(((DB_ANYTYPE *)dst)->db_f4type)) = 
			    -Adf_globs->Adc_hdec.fp_f4nil;
		}
#else
		if (((DB_ANYTYPE *)src)->db_f4type > 0.0)
		{
		    ((DB_ANYTYPE *)dst)->db_f4type =
			Adf_globs->Adc_hdec.fp_f4pos *
			    ((DB_ANYTYPE *)src)->db_f4type;
		}
		else if (((DB_ANYTYPE *)src)->db_f4type < 0.0)
		{
		    ((DB_ANYTYPE *)dst)->db_f4type =
			Adf_globs->Adc_hdec.fp_f4neg *
			    ((DB_ANYTYPE *)src)->db_f4type;
		}
		else
		{
		    ((DB_ANYTYPE *)dst)->db_f4type = 
			    -Adf_globs->Adc_hdec.fp_f4nil;
		}
#endif

		break;

	    case 8:
#ifdef axp_osf
		if (F8CAST_MACRO(&(((DB_ANYTYPE *)src)->db_f8type)) > 0.0)
		{
		    F8CAST_MACRO(&(((DB_ANYTYPE *)dst)->db_f8type)) =
			Adf_globs->Adc_hdec.fp_f8pos *
			    F8CAST_MACRO(&(((DB_ANYTYPE *)src)->db_f8type));
		}
		else if (F8CAST_MACRO(&(((DB_ANYTYPE *)src)->db_f8type)) < 0.0)
		{
		    F8CAST_MACRO(&(((DB_ANYTYPE *)dst)->db_f8type)) =
			Adf_globs->Adc_hdec.fp_f8neg *
			    F8CAST_MACRO(&(((DB_ANYTYPE *)src)->db_f8type));
		}
		else
		{
		    F8CAST_MACRO(&(((DB_ANYTYPE *)dst)->db_f8type)) =
			-Adf_globs->Adc_hdec.fp_f8nil;
		}
#else
		if (((DB_ANYTYPE *)src)->db_f8type > 0.0)
		{
		    ((DB_ANYTYPE *)dst)->db_f8type =
			Adf_globs->Adc_hdec.fp_f8pos *
			    ((DB_ANYTYPE *)src)->db_f8type;
		}
		else if (((DB_ANYTYPE *)src)->db_f8type < 0.0)
		{
		    ((DB_ANYTYPE *)dst)->db_f8type =
			Adf_globs->Adc_hdec.fp_f8neg *
			    ((DB_ANYTYPE *)src)->db_f8type;
		}
		else
		{
		    ((DB_ANYTYPE *)dst)->db_f8type =
			-Adf_globs->Adc_hdec.fp_f8nil;
		}
#endif
		break;

	    default:
		db_stat = adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0);
		break;
	    }

	    break;
	}

	case DB_CHA_TYPE:
        case DB_BIT_TYPE:
    	case DB_BYTE_TYPE:
        case DB_NBR_TYPE:
	{
	    /*
	    ** All character string data types should be converted to SQL char
	    ** type since the comparison semantics are correct if the lengths
	    ** are equal
	    ** (histograms should only be compared if the lengths equal)
	    ** Bit string datatypes are all converted to fixed length BIT.
	    ** - this is done since varbit, text and varchar histogram
	    ** elements will not have an length prefix, and quel "c" type
	    ** cannot have embedded nulls. 
	    ** - use the original attribute type to know how to properly
	    ** extend the histogram string element.
	    ** 
	    ** All bit operations operate almost the same way, so we munge
	    ** the two types together.
	    ** The bit operations differ in the value to assign to the lower
	    ** order bytes/bits (for characters, use IIMAX_CHAR_VALUE,
	    ** for BITs, 0xff.
	    **
	    ** For decrementing, we simply decrement the value of the least
	    ** significant byte.  This is identical for either BIT or
	    ** CHARACTER string types.  For BIT types, the type is stored
	    ** such that a bytewise compare of the values for values of equal
	    ** length will alway produce the correct result.
	    */

	    register u_char	*tp;	    /* ptr to buffer contain 
					    ** original histogram */
	    register u_char	*fp;	    /* ptr to buffer containing 
					    ** destination of result */
	    register u_char	max_value;  /* Maximum value for DType */

	    register i4 	len;

	    if (adc_dvfhg->db_datatype == DB_CHA_TYPE)
		max_value = IIMAX_CHAR_VALUE;
	    else
		max_value = ((u_char) ~0);

	    len = adc_dvfhg->db_length;
	    fp = ((u_char *)src) + len;
	    tp = ((u_char *)dst) + len;
	    while (len-- && !(*--fp))
	    {
		*--tp = max_value;
	    }

	    if (len >= 0)
	    {
		/*
		** Decrement the value -- as described above.
		*/

		*--tp = (*fp) - 1;

		/* move remaining characters to output buffer */
		while (len--)
		    *--tp = *--fp;
	    }
	    else
	    {
		/* only happens with completely null filled element */
		len = adc_dvfhg->db_length;
		tp = ((u_char *)dst);
		while (len--)
		/* Code Integration from 6.4 b64336 */
		/*  *tp++ = (char) 0;	*/
		    *tp++ = (u_char) 0;
	    }
	    break;
	}


	default:
	{
	    db_stat = adu_error(adf_scb, E_AD3011_BAD_HG_DTID, 0);
	    break;
	}

	}	/* switch */

	break;

    }	    /* end of for stmt */

    return(db_stat);
}
