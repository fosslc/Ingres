/*
** Copyright (c) 1986, 2009 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <st.h>
#include    <cv.h>
#include    <cm.h>
#include    <sl.h>
#include    <me.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <adumoney.h>
#include    <adp.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduint.h>
#include    <adulcol.h>
#include    <aduucol.h>

/*{
** Name: adu_longcmp() -    Compare two long data values for equality using
**                          the appropriate semantics.
**
** Description:
**
**     Step through records from two long objects calling base routines
**     for comaparisons.
**  
** Inputs:
**      adf_scb                         Pointer to an ADF session control block.
**          .adf_errcb                  ADF_ERROR struct.
**              .ad_ebuflen             The length, in bytes, of the buffer
**                                      pointed to by ad_errmsgp.
**              .ad_errmsgp             Pointer to a buffer to put formatted
**                                      error message in, if necessary.
**      vcdv1                           Ptr DB_DATA_VALUE describing first
**                                      record to be compared.
**          .db_data                    Ptr to DB_TEXT_STRING containing
**                                      the data.
**              .db_t_count             Number of bytes in this string.
**              .db_t_text              Ptr to the actual data
**      vcdv2                           Ptr DB_DATA_VALUE describing second
**                                      record to be compared.
**          .db_data                    Ptr to DB_TEXT_STRING containing
**                                      the data.
**              .db_t_count             Number of bytes in this string
**              .db_t_text              Ptr to the actual characters.
**
** Outputs:
**      adf_scb                         Pointer to an ADF session control block.
**          .adf_errcb                  ADF_ERROR struct.  If an
**                                      error occurs the following fields will
**                                      be set.  NOTE: if .ad_ebuflen = 0 or
**                                      .ad_errmsgp = NULL, no error message
**                                      will be formatted.
**              .ad_errcode             ADF error code for the error.
**              .ad_errclass            Signifies the ADF error class.
**              .ad_usererr             If .ad_errclass is ADF_USER_ERROR,
**                                      this field is set to the corresponding
**                                      user error which will either map to
**                                      an ADF error code or a user-error code.
**              .ad_emsglen             The length, in bytes, of the resulting
**                                      formatted error message.
**              .adf_errmsgp            Pointer to the formatted error message.
**      rcmp                            Result of comparison, as follows:
**                                          <0  if  vcdv1 < vcdv2
**                                          >0  if  vcdv1 > vcdv2
**                                          =0  if  vcdv1 = vcdv2
**      Returns:
**          The following DB_STATUS codes may be returned:
**          E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**          If a DB_STATUS code other than E_DB_OK is returned, the caller
**          can look in the field adf_scb.adf_errcb.ad_errcode to determine
**          the ADF error code.  The following is a list of possible ADF error
**          codes that can be returned by this routine:
**
**          E_AD0000_OK                 Completed successfully.
**
** History:
**
**       12-Nov-2009 (coomi01) Bug 122840
**          Created to factor out block by block extraction for the varchar
**          and byte versions of long data types from original adu_lvarcharcmp()
**          rotuine.
*/

DB_STATUS
adu_longcmp(
ADF_CB              *adf_scb,
DB_DATA_VALUE       *vcdv1,
DB_DATA_VALUE       *vcdv2,
i4                  *rcmp)
{
    i4			bits1,bits2;
    DB_DATA_VALUE       cpn_dv1,cpn_dv2;
    DB_DATA_VALUE       under_dv1,under_dv2;
    ADP_POP_CB		pop_cb1,pop_cb2;
    DB_DATA_VALUE	segment_dv1,segment_dv2;
    char		segspace1[4096];
    char		segspace2[4096];
    ADP_PERIPHERAL      *inp1_cpn = (ADP_PERIPHERAL *) vcdv1->db_data;
    ADP_PERIPHERAL      *inp2_cpn = (ADP_PERIPHERAL *) vcdv2->db_data;
    ADP_PERIPHERAL      inp1_local;
    ADP_PERIPHERAL      inp2_local;
    ADP_PERIPHERAL      tmpcpn1, tmpcpn2; 
    DB_STATUS		status;
    DB_STATUS		db_status;
    i4			length;
    bool		pop1_open = FALSE, pop2_open = FALSE;
    
    /* 
    ** Ensure like for like comparison 
    */
    if (vcdv1->db_datatype != vcdv2->db_datatype)
    {
	return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    }

    status = adi_dtinfo(adf_scb, vcdv1->db_datatype, &bits1);
    if (status)
	return(status);

    status = adi_dtinfo(adf_scb, vcdv2->db_datatype, &bits2);
    if (status)
   	return(status);

    if ((bits1 & AD_PERIPHERAL) == 0)
    {
	return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    }
    status = adi_per_under(adf_scb,
    			   vcdv1->db_datatype,
			   &under_dv1);
    cpn_dv1.db_datatype = vcdv1->db_datatype;

    if ((bits2 & AD_PERIPHERAL) == 0)
    {
	return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    }
    status = adi_per_under(adf_scb,
				vcdv2->db_datatype,
				&under_dv2);
    cpn_dv2.db_datatype = vcdv2->db_datatype;


    /* Set up the structures */	
    pop_cb1.pop_length = sizeof(pop_cb1);
    pop_cb2.pop_length = sizeof(pop_cb2);
    pop_cb1.pop_type = ADP_POP_TYPE;
    pop_cb2.pop_type = ADP_POP_TYPE;
    pop_cb1.pop_ascii_id = ADP_POP_ASCII_ID;
    pop_cb2.pop_ascii_id = ADP_POP_ASCII_ID;
    /* (temp flag setting is bogus as we're just getting, but be safe) */
    pop_cb1.pop_temporary = ADP_POP_TEMPORARY;
    pop_cb2.pop_temporary = ADP_POP_TEMPORARY;
    pop_cb1.pop_underdv = &under_dv1;
    pop_cb2.pop_underdv = &under_dv2;
    under_dv1.db_data = 0;
    under_dv2.db_data = 0;
    pop_cb1.pop_segment = &segment_dv1;
    pop_cb2.pop_segment = &segment_dv2;
    STRUCT_ASSIGN_MACRO(under_dv1, segment_dv1);
    STRUCT_ASSIGN_MACRO(under_dv2, segment_dv2);
    pop_cb1.pop_coupon = &cpn_dv1;
    pop_cb2.pop_coupon = &cpn_dv2;
    cpn_dv1.db_length = sizeof(ADP_PERIPHERAL);
    cpn_dv2.db_length = sizeof(ADP_PERIPHERAL);
    cpn_dv1.db_prec = 0;
    cpn_dv2.db_prec = 0;
    cpn_dv1.db_data = (PTR) 0;
    cpn_dv2.db_data = (PTR) 0;
    segment_dv1.db_data = segspace1;
    segment_dv2.db_data = segspace2;

    MEcopy(vcdv1->db_data, sizeof(ADP_PERIPHERAL), &tmpcpn1);
    MEcopy(vcdv2->db_data, sizeof(ADP_PERIPHERAL), &tmpcpn2);
    cpn_dv1.db_data = (char *)&tmpcpn1;
    cpn_dv2.db_data = (char *)&tmpcpn2;

    pop_cb1.pop_continuation = ADP_C_BEGIN_MASK;
    pop_cb2.pop_continuation = ADP_C_BEGIN_MASK;

    if(ME_ALIGN_MACRO(inp1_cpn, DB_ALIGN_SZ) != (PTR) inp1_cpn)
    {
        MEcopy((PTR)inp1_cpn, sizeof(ADP_PERIPHERAL), (PTR)&inp1_local);
	inp1_cpn = &inp1_local;
    }
    if(ME_ALIGN_MACRO(inp2_cpn, DB_ALIGN_SZ) != (PTR) inp2_cpn)
    {
        MEcopy((PTR)inp2_cpn, sizeof(ADP_PERIPHERAL), (PTR)&inp2_local);
	inp2_cpn = &inp2_local;
    }


    /* Get the segment from each string and compare until a difference
    **   is found or the full length of the long var chars is reached
    */
    do
    {
	segment_dv1.db_length = under_dv1.db_length;
	segment_dv2.db_length = under_dv2.db_length;

        ((DB_TEXT_STRING *) segment_dv1.db_data)->db_t_count = 0;
        ((DB_TEXT_STRING *) segment_dv2.db_data)->db_t_count = 0;

        if ((pop_cb1.pop_continuation & ADP_C_END_MASK) == 0
           && (inp1_cpn->per_length0 != 0 || inp1_cpn->per_length1 != 0))
        {
	    status = (*Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi)(
			    ADP_GET, &pop_cb1);
            if (status != E_DB_OK && pop_cb1.pop_error.err_code != E_AD7001_ADP_NONEXT)
            {
                adf_scb->adf_errcb.ad_errcode = pop_cb1.pop_error.err_code;
                break;
            }
            if (pop_cb1.pop_error.err_code == E_AD7001_ADP_NONEXT)
            {
                pop1_open = FALSE;
            }
            else
            {
                pop1_open = TRUE;
            }
        }
	pop_cb1.pop_continuation &= ~ADP_C_BEGIN_MASK;

        if ((pop_cb2.pop_continuation & ADP_C_END_MASK) == 0
           && (inp2_cpn->per_length0 != 0 || inp2_cpn->per_length1 != 0))
        {
	    status = (*Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi)(
			    ADP_GET, &pop_cb2);
            if (status != E_DB_OK && pop_cb2.pop_error.err_code != E_AD7001_ADP_NONEXT)
            {
                adf_scb->adf_errcb.ad_errcode = pop_cb2.pop_error.err_code;
                break;
            }
            if (pop_cb2.pop_error.err_code == E_AD7001_ADP_NONEXT)
            {
                pop2_open = FALSE;
            }
            else
            {
                pop2_open = TRUE;
            }
        }
	pop_cb2.pop_continuation &= ~ADP_C_BEGIN_MASK;

	length = ((DB_TEXT_STRING *) segment_dv1.db_data)->db_t_count;
	if ((length + 2) >= under_dv1.db_length)
	{
	    segment_dv1.db_length = under_dv1.db_length;
	    ((DB_TEXT_STRING *) segment_dv1.db_data)->db_t_count =
					under_dv1.db_length - 2;
	}
	else
	{
	    segment_dv1.db_length = length + 2;
	    pop_cb1.pop_continuation |= ADP_C_END_MASK;
	}
	length = ((DB_TEXT_STRING *) segment_dv2.db_data)->db_t_count;
	if ((length + 2) >= under_dv2.db_length)
	{ 
	    segment_dv2.db_length = under_dv2.db_length;
	    ((DB_TEXT_STRING *) segment_dv2.db_data)->db_t_count =
					under_dv2.db_length - 2;
	}
	else
	{
	    segment_dv2.db_length = length + 2;
	    pop_cb2.pop_continuation |= ADP_C_END_MASK;
	}

	/* Choose the appropriate comparison routine */
	switch ( abs(segment_dv1.db_datatype) )
	{
	    case (DB_NVCHR_TYPE):
	    case (DB_VCH_TYPE):
	    {
		/* Do a var char compare on each segment */
		status = adu_varcharcmp(adf_scb,&segment_dv1,&segment_dv2,rcmp);
	    }
	    break;

	    case (DB_VBYTE_TYPE):
	    {
		/* 
		** Do a straight byte compare on each segment 
		*/
		length = min(segment_dv1.db_length,segment_dv2.db_length);
		
		*rcmp = MEcmp(((DB_TEXT_STRING *) segment_dv1.db_data)->db_t_text,
			      ((DB_TEXT_STRING *) segment_dv2.db_data)->db_t_text,
			      length);

		if ((0 == *rcmp) && (segment_dv1.db_length != segment_dv2.db_length))
		{
		    /* 
		    ** They are not equal 
		    */
		    *rcmp = (segment_dv1.db_length > segment_dv2.db_length) ? 1 : -1;
		}
		
		status = (E_DB_OK);
	    }
	    break;

	    default:
		/* 
		** Need to extend the switch to do the correct base comparison
		*/
		return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
		break;

	} /* switch on segment_dv1.db_datatype */

	if (*rcmp)
        {
            /* break out, clean-up and return status */
	    break;
        }
    }
    while (( status == 0) &&
	((pop_cb1.pop_continuation & ADP_C_END_MASK) == 0 || 
	 (pop_cb2.pop_continuation & ADP_C_END_MASK) == 0));

    if (pop1_open)
    {
        pop_cb1.pop_continuation |= ADP_C_END_MASK;
        db_status = (*Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi)(
                     ADP_GET, &pop_cb1);
    }

    if (pop2_open)
    {
        pop_cb2.pop_continuation |= ADP_C_END_MASK;
        db_status = (*Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi)(
                     ADP_GET, &pop_cb2);
    }

    return(status);
}

