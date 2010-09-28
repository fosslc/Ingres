/*
**  Copyright (c) 2009 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduint.h>
#include    <me.h>

/**
**  Name:   adugreatest.c - Implement general GREATEST/LEAST functions (like MAX)
**
**  Description:
**
**	This file defines:
**
**          adu_greatest - compare two items & return the greater
**          adu_least - compare two items & return the smaller
**
**  Function prototypes defined in ADUINT.H file.
**
**  History:	
**	06-Sep-2009 (kiria01) SIR 122894
**	    Created adding GREATEST and LEAST generic polyadic functions.
**	28-Jul-2010 (kiria01) b124142
**	    Tightened ADF_NVL_BIT handling.
**/


/*
**  Name: adu_greatest() - compare two items and the return the greatest
**
**	This function need to check for NULLs and if found return the other
**	operand. However, whether the function does propagate NULLs or not
**	will be determined by the FI_DEFN flags setting:
**		ADE_3CXI_CMP_SET_SKIP	for usual NULL propagation.
**		ADE_0CXI_IGNORE		to skip NULLs.
**
**  Inputs:
**	adf_scb				Pointer to ADF session control block.
**	dv1  				Pointer to first argument.
**      dv2  				Pointer to second argument.
**
**  Outputs:
**	rdv				Larger of the two
**
**	Returns:
**	    E_DB_OK
**
**  History:
**	06-Sep-2009 (kiria01) SIR 122894
**	    Created
*/

DB_STATUS
adu_greatest(
ADF_CB		*adf_scb,
DB_DATA_VALUE	*dv1,
DB_DATA_VALUE	*dv2,
DB_DATA_VALUE	*rdv)
{
    DB_DATA_VALUE d1;
    DB_DATA_VALUE d2;
    DB_DATA_VALUE dr;
    DB_DATA_VALUE *ret = NULL;

    if (dv1->db_datatype < 0)
    {
	d1 = *dv1;
	d1.db_datatype = -d1.db_datatype;
	if (~((char*)d1.db_data)[--d1.db_length] & ADF_NVL_BIT)
	    dv1 = &d1;
	else
	    ret = dv2;
    }
    if (dv2->db_datatype < 0)
    {
	d2 = *dv2;
	d2.db_datatype = -d2.db_datatype;
	if (~((char*)d2.db_data)[--d2.db_length] & ADF_NVL_BIT)
	{
	    if (ret)
		ret = &d2;
	    else
		dv2 = &d2;
	}
	else if (!ret)
	    ret = dv1;
	else if (rdv->db_datatype < 0)
	{
	    /* Set NULL bit & implicitly clear SING bit */
	    ((char*)rdv->db_data)[rdv->db_length-1] = ADF_NVL_BIT;
	    return E_DB_OK;
	}
	else
	    adu_error(adf_scb, E_AD9998_INTERNAL_ERROR,
			2, 0, "ret dt not nullable");
    }
    if (!ret)
    {
	i4 result;
	DB_STATUS sts = adc_compare(adf_scb, dv1, dv2, &result);
	if (sts)
	    return sts;
	ret = dv1;
	if (result < 0)
	    ret = dv2;
    }
    if (rdv->db_datatype < 0)
    {
	dr = *rdv;
	/* Clear NULL bit */
	((char*)dr.db_data)[--dr.db_length] = 0;
	dr.db_datatype = -dr.db_datatype;
	rdv = &dr;
    }
    if (ret->db_length == rdv->db_length &&
	abs(rdv->db_datatype != DB_DEC_TYPE))
    {
	MEcopy(ret->db_data, ret->db_length, rdv->db_data);
	return E_DB_OK;
    }
    else switch(abs(rdv->db_datatype))
    {
    case DB_INT_TYPE:
	return adu_1int_coerce(adf_scb, ret, rdv);
    case DB_DEC_TYPE:
	return adu_1dec_coerce(adf_scb, ret, rdv);
    case DB_FLT_TYPE:
	return adu_1flt_coerce(adf_scb, ret, rdv);
    case DB_CHR_TYPE:
    case DB_CHA_TYPE:
    case DB_VCH_TYPE:
    case DB_TXT_TYPE:
    case DB_LTXT_TYPE:
    case DB_BYTE_TYPE:
    case DB_VBYTE_TYPE:
	return adu_1strtostr(adf_scb, ret, rdv);
    case DB_NCHR_TYPE:
    case DB_NVCHR_TYPE:
	return adu_nvchr_coerce(adf_scb, ret, rdv);
    }
    return adu_error(adf_scb, E_AD9998_INTERNAL_ERROR,
				2, 0, "dt unsupported");
}


/*
**  Name: adu_least() - compare two items and the return the least
**
**	This function need to check for NULLs and if found return the other
**	operand. However, whether the function does propagate NULLs or not
**	will be determined by the FI_DEFN flags setting:
**		ADE_3CXI_CMP_SET_SKIP	for usual NULL propagation.
**		ADE_0CXI_IGNORE		to skip NULLs.
**
**  Inputs:
**	adf_scb				Pointer to ADF session control block.
**	dv1  				Pointer to first argument.
**      dv2  				Pointer to second argument.
**
**  Outputs:
**	rdv				Larger of the two
**
**	Returns:
**	    E_DB_OK
**
**  History:
**	06-Sep-2009 (kiria01)
**	    Created
*/

DB_STATUS
adu_least(
ADF_CB		*adf_scb,
DB_DATA_VALUE	*dv1,
DB_DATA_VALUE	*dv2,
DB_DATA_VALUE	*rdv)
{
    DB_DATA_VALUE d1;
    DB_DATA_VALUE d2;
    DB_DATA_VALUE dr;
    DB_DATA_VALUE *ret = NULL;

    if (dv1->db_datatype < 0)
    {
	d1 = *dv1;
	d1.db_datatype = -d1.db_datatype;
	if (~((char*)d1.db_data)[--d1.db_length] & ADF_NVL_BIT)
	    dv1 = &d1;
	else
	    ret = dv2;
    }
    if (dv2->db_datatype < 0)
    {
	d2 = *dv2;
	d2.db_datatype = -d2.db_datatype;
	if (~((char*)d2.db_data)[--d2.db_length] & ADF_NVL_BIT)
	{
	    if (ret)
		ret = &d2;
	    else
		dv2 = &d2;
	}
	else if (!ret)
	    ret = dv1;
	else if (rdv->db_datatype < 0)
	{
	    /* Set NULL bit & implicitly clear SING bit */
	    ((char*)rdv->db_data)[rdv->db_length-1] = ADF_NVL_BIT;
	    return E_DB_OK;
	}
	else
	    adu_error(adf_scb, E_AD9998_INTERNAL_ERROR,
			2, 0, "ret dt not nullable");
    }
    if (!ret)
    {
	i4 result;
	DB_STATUS sts = adc_compare(adf_scb, dv1, dv2, &result);
	if (sts)
	    return sts;
	ret = dv1;
	if (result > 0)
	    ret = dv2;
    }
    if (rdv->db_datatype < 0)
    {
	dr = *rdv;
	/* Clear NULL bit */
	((char*)dr.db_data)[--dr.db_length] = 0;
	dr.db_datatype = -dr.db_datatype;
	rdv = &dr;
    }
    if (ret->db_length == rdv->db_length &&
	abs(rdv->db_datatype != DB_DEC_TYPE))
    {
	MEcopy(ret->db_data, ret->db_length, rdv->db_data);
	return E_DB_OK;
    }
    else switch(abs(rdv->db_datatype))
    {
    case DB_INT_TYPE:
	return adu_1int_coerce(adf_scb, ret, rdv);
    case DB_DEC_TYPE:
	return adu_1dec_coerce(adf_scb, ret, rdv);
    case DB_FLT_TYPE:
	return adu_1flt_coerce(adf_scb, ret, rdv);
    case DB_CHR_TYPE:
    case DB_CHA_TYPE:
    case DB_VCH_TYPE:
    case DB_TXT_TYPE:
    case DB_LTXT_TYPE:
    case DB_BYTE_TYPE:
    case DB_VBYTE_TYPE:
	return adu_1strtostr(adf_scb, ret, rdv);
    case DB_NCHR_TYPE:
    case DB_NVCHR_TYPE:
	return adu_nvchr_coerce(adf_scb, ret, rdv);
    }
    return adu_error(adf_scb, E_AD9998_INTERNAL_ERROR,
				2, 0, "dt unsupported");
}
