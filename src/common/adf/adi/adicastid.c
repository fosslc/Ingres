/*
** Copyright (c) 2002, Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <adfops.h>
#include    <ulf.h>
#include    <cm.h>
#include    <adfint.h>
/* [@#include@]... */

/**
**
**  Name: ADICASTID.C - Get cast function ID from data type.
**
**  Description:
**      This file contains the function that transforms a 
**	data type into the scalar function that coerces to that
**	type. It is used in the compilation of ANSI cast syntax.
**
**      This file defines:
**
**          adi_castid() - Get operator id from data type.
**
**
**  History:    $Log-for RCS$
**      10-jan-2002 (inkdo01)
**          Initial creation.
**  16-Jun-2009 (thich01)
**      Treat GEOM type the same as LBYTE.
**  20-Aug-2009 (thich01)
**      Treat all spatial types the same as LBYTE.
**      23-oct-2009 (joea)
**          Add case for DB_BOO_TYPE in adi_castid.
**/


/*{
** Name: adi_castid() - Get operator id from data type.
**
** Description:
**      This function is the external ADF call "adi_castid()" which
**      returns the operator id corresponding to the coercion implied
**	by the supplied data type. This function isn't as generalized
**	as other "adi" functions because it doesn't use the Adi tables.
**	Rather, as an interim approach (kludge), it simply uses a 
**	switch on the data type to select the operator id.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	    .adf_qlang			Query language in use.
**      adi_castdv                      Pointer to DB_DATA_VALUE containing
**					data type, length information used to
**					select the coercion function ID.
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
**      adi_oid                         The operator id corresponding
**                                      to the supplied data type.
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
**          E_AD2004_BAD_DTID		Data type code unknown.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      10-jan-2002 (inkdo01)
**          Initial creation.
**	1-feb-06 (dougi)
**	    Add Unicode support.
**	19-jun-2006 (gupsh01)
**	    Added support for new ANSI date/time support.
**	05-sep-2006 (gupsh01)
**	    Assigned explicit operators for ANSI date/time
**	    types instead of generic ADI_DATE_OP.
**	17-Mar-2009 (kiria01) SIR121788
**	    Add missing LNVCHR and the remaining operators
**	    that can cast.
**      25-Jan-2010 (hanal04) Bug 123168
**          Correct DB_TXT_TYPE case. We were returning ADI_VARCH_OP
**          instead of ADI_TEXT_OP.
*/

DB_STATUS
adi_castid(
ADF_CB             *adf_scb,
ADI_OP_ID          *adi_oid,
DB_DATA_VALUE	   *adi_castdv)
{

    /* This function is simply a switch on the data type, the case's
    ** of which are used to assign the operation Id. */

    switch (adi_castdv->db_datatype) {
      case DB_BOO_TYPE:
            *adi_oid = ADI_BOO_OP;
	break;

      case DB_INT_TYPE:
	if (adi_castdv->db_length == 4)
	    *adi_oid = ADI_I4_OP;
	else if (adi_castdv->db_length == 2)
	    *adi_oid = ADI_I2_OP;
	else if (adi_castdv->db_length == 8)
	    *adi_oid = ADI_I8_OP;
	else if (adi_castdv->db_length == 1)
	    *adi_oid = ADI_I1_OP;
	break;

      case DB_FLT_TYPE:
	if (adi_castdv->db_length == 8)
	    *adi_oid = ADI_F8_OP;
	else if (adi_castdv->db_length == 4)
	    *adi_oid = ADI_F4_OP;
	break;

      case DB_CHR_TYPE:
	*adi_oid = ADI_ASCII_OP;
	break;

      case DB_CHA_TYPE:
	*adi_oid = ADI_CHAR_OP;
	break;

      case DB_DTE_TYPE:
	*adi_oid = ADI_DATE_OP;
	break;

      case DB_ADTE_TYPE:
	*adi_oid = ADI_ADTE_OP;
	break;

      case DB_TMWO_TYPE:
	*adi_oid = ADI_TMWO_OP;
	break;

      case DB_TMW_TYPE:
	*adi_oid = ADI_TMW_OP;
	break;

      case DB_TME_TYPE:
	*adi_oid = ADI_TME_OP;
	break;

      case DB_TSWO_TYPE:
	*adi_oid = ADI_TSWO_OP;
	break;

      case DB_TSW_TYPE:
	*adi_oid = ADI_TSW_OP;
	break;

      case DB_TSTMP_TYPE:
	*adi_oid = ADI_TSTMP_OP;
	break;

      case DB_INYM_TYPE:
	*adi_oid = ADI_INYM_OP;
	break;

      case DB_INDS_TYPE:
	*adi_oid = ADI_INDS_OP;
	break;

      case DB_TXT_TYPE:
	*adi_oid = ADI_TEXT_OP;
	break;

      case DB_VCH_TYPE:
	*adi_oid = ADI_VARCH_OP;
	break;

      case DB_LVCH_TYPE:
	*adi_oid = ADI_LONG_VARCHAR;
	break;

      case DB_NCHR_TYPE:
	*adi_oid = ADI_NCHAR_OP;
	break;

      case DB_NVCHR_TYPE:
	*adi_oid = ADI_NVCHAR_OP;
	break;

      case DB_MNY_TYPE:
	*adi_oid = ADI_MONEY_OP;
	break;

      case DB_BYTE_TYPE:
	*adi_oid = ADI_BYTE_OP;
	break;

      case DB_VBYTE_TYPE:
	*adi_oid = ADI_VBYTE_OP;
	break;

      case DB_LBYTE_TYPE:
      case DB_GEOM_TYPE:
      case DB_POINT_TYPE:
      case DB_MPOINT_TYPE:
      case DB_LINE_TYPE:
      case DB_MLINE_TYPE:
      case DB_POLY_TYPE:
      case DB_MPOLY_TYPE:
	*adi_oid = ADI_LONG_BYTE;
	break;

      case DB_DEC_TYPE:
	*adi_oid = ADI_DEC_OP;
	break;

      case DB_TABKEY_TYPE:
	*adi_oid = ADI_TABKEY_OP;
	break;

      case DB_LOGKEY_TYPE:
	*adi_oid = ADI_LOGKEY_OP;
	break;

      case DB_BIT_TYPE:
	*adi_oid = ADI_BIT_OP;
	break;

      case DB_VBIT_TYPE:
	*adi_oid = ADI_VBIT_OP;
	break;

      case DB_LNVCHR_TYPE:
	*adi_oid = ADI_LNVCHR_OP;
	break;

      default:
	/* Bad data type - AD2004 error. */
	return (adu_error(adf_scb, E_AD2004_BAD_DTID, 0));
    }

    return (E_DB_OK);
}
