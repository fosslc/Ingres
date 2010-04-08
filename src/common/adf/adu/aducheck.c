/*
** Copyright (c) 2007 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduint.h>

/* [@#include@]... */

/**
**
**  Name: ADUCHECK.C - Routines related to checksum functions.
**
**  Description:
**      This file contains routines related to checksum functions.
**
**	This file defines the following externally visible routines:
**
**          adu_checksum() - ADF function instance to return unsigned integer
**		checksum value of a string, integer, or other data type.
**
**  Function prototypes defined in ADUINT.H file.
**
**  History:
**	17-july-2007 (dougi)
**	    Cloned from aduhash.c.
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
** Name: adu_checksum() - ADF function instance to return unsigned integer
**			checksum value of a string or other data type.
**
** Description:
**      Initially this routine is simply a stub that will later be replaced
**	by one or more functions performing various checksum algorithms.
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
**	rdv				DB_DATA_VALUE describing the integer
**					result of the checksum operation.
**	    .db_data			Ptr to the result integer.
**
**	Returns:
**	    E_AD0000_OK			Completed successfully.
**
**	Exceptions:
**	    none
**
** History:
**	17-july-2007 (dougi)
**	    Initial stub routine - needs a proper checksum function.
**	02-aug-2007 (toumi01)
**	    Flesh out stub routine with Adler-32 checksum.
*/

#define MODULO_ADLER	65521
#define OFLOW_SAFE	5550	/* loops that won't overflow sum2 */

DB_STATUS
adu_checksum(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv1,
DB_DATA_VALUE   *rdv)
{
    i4 		    in_len;
    i4 		    res_len;
    char	    *tmp;
    u_i4	    *out_p = ((u_i4 *)rdv->db_data);
    DB_STATUS	    db_stat;
    u_i1	    *data;
    u_i4	    sum1 = 1, sum2 = 0;

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

    data = (u_i1*)tmp;
    while (in_len)
    {
	size_t tmplen = in_len > OFLOW_SAFE ? OFLOW_SAFE : in_len;
	in_len -= tmplen;
	do
	{
	    sum1 += *data++;
	    sum2 += sum1;
	} while (--tmplen);
	sum1 = (sum1 & 0xffff) + (sum1 >> 16) * (65536-MODULO_ADLER);
	sum2 = (sum2 & 0xffff) + (sum2 >> 16) * (65536-MODULO_ADLER);
    }
    if (sum1 >= MODULO_ADLER)
	sum1 -= MODULO_ADLER;
    sum2 = (sum2 & 0xffff) + (sum2 >> 16) * (65536-MODULO_ADLER);
    if (sum2 >= MODULO_ADLER)
	sum2 -= MODULO_ADLER;
    *out_p = (sum2 << 16) | sum1;

    return(E_DB_OK);
}
