/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <hshcrc.h>
#include    <adfint.h>
#include    <aduint.h>

/* [@#include@]... */

/**
**
**  Name: ADUHASH.C - Routines related to hash function.
**
**  Description:
**      This file contains routines related to the hash function.
**
**	This file defines the following externally visible routines:
**
**          adu_hash() - ADF function instance to return unsigned integer
**			value of a string, integer, or other data type.
**
**  Function prototypes defined in ADUINT.H file.
**
**  History:
**      12-Jan-1999 (shero03)
**          Initial creation.
**      02-feb-1999 (hanch04)
**          Replace nat and longnat with i4
**	13-dec-2001 (devjo01)
**	    In adu_hash add case labels for NCHR, NVCHR.
**/

/*
[@forward_type_references@]
[@forward_function_references@]
[@#defines_of_other_constants@]
[@type_definitions@]
[@global_variable_definitions@]
*/

/*
**  Definition of static variables and forward static functions.
*/
	
/*
[@static_variable_or_function_definition@]...
*/


/*
** Name: adu_hash() - ADF function instance to return unsigned integer
**			hash value of a string or other data type.
**
** Description:
**      This function converts a text datatype into a varchar stringf where
**	each character pair represents the hex representation of the 
**	corresponding character of the input string.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             DB_DATA_VALUE describing the string
**					operand to be shown as hex.
**	    .db_datatype		
**	    .db_length			
**	    .db_data			Ptr to data.
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
**	rdv				DB_DATA_VALUE describing the varchar
**					which holds the hex representation of
**					the input string.
**	    .db_data			Ptr to the varchar.
**
**	Returns:
**	    E_AD0000_OK			Completed successfully.
**
**	Exceptions:
**	    none
**
** History:
**      12-Jan-1999 (shero03)
**          Initial creation.
**	13-dec-2001 (devjo01)
**	    Add cases for NCHR, NVCHR.
**	01-Feb-2005 (jenjo02)
**	    Peek at adi_dtstat_bits directly instead of
**	    calling adi_dtinfo to do it.
*/

DB_STATUS
adu_hash(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv1,
DB_DATA_VALUE   *rdv)
{
    i4 		    in_len;
    i4 		    res_len;
    char	    *tmp;
    u_i4	    *out_p = ((u_i4 *)rdv->db_data);
    DB_STATUS	    db_stat;

    /* Don't support long data types */
    if ( Adf_globs->Adi_dtptrs[ADI_DT_MAP_MACRO(dv1->db_datatype)]->
		adi_dtstat_bits & AD_PERIPHERAL )
        return (adu_error(adf_scb, E_AD2090_BAD_DT_FOR_STRFUNC, 0));

    switch (dv1->db_datatype)
    {
	/* call lenaddr for the stringish types - it gets varying length
	** lengths, among other things. */
        case DB_CHA_TYPE:
        case DB_CHR_TYPE:
        case DB_BYTE_TYPE:
        case DB_VCH_TYPE:
        case DB_TXT_TYPE:
        case DB_LTXT_TYPE:
	case DB_VBYTE_TYPE:
	case DB_NCHR_TYPE:
	case DB_NVCHR_TYPE:
	 if (db_stat = adu_lenaddr(adf_scb, dv1, &in_len, &tmp))
	 return(db_stat);
	 break;
	default:	/* all the others */
	 in_len = dv1->db_length;	/* fixed length */
	 tmp = dv1->db_data;
	 break;
    }

    *out_p = -1;
    HSH_CRC32(tmp, in_len, out_p);

    return(E_DB_OK);
}


/*
[@function_definition@]...
*/
