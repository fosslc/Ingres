/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduint.h>
/**
**
**  Name: ADUIFTRU.C - The INGRES "ii_iftrue" function.
**
**  Description:
**      If the first param is non-zero, return the second param. Else,
**	return the NULL value.
**
**          adu_iftrue -  The INGRES "ii_iftrue" function.
**
**
**  History:
**	18-dec-89 (jrb)
**	    Created.
**      04-jan-1993 (stevet)
**          Added function prototypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**/


/*{
** Name: adu_iftrue() - The INGRES "ii_iftrue" function.
**
** Description:
**      Check first parameter. If non-zero, return the second param.  If it is zero,
**  return the NULL value whose type is the type of the second parameter.
**
**	The first parameter must be a non-nullable integer; the second parameter
**	can be any type at all.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1				Ptr to DB_DATA_VALUE for first param.
**	    .db_datatype		Datatype.
**	    .db_prec			Precision/Scale
**	    .db_length			Length.
**	    .db_data			Ptr to the data of first param.
**      dv2				Ptr to DB_DATA_VALUE for second param.
**	    .db_datatype		Datatype.
**	    .db_prec			Precision/Scale
**	    .db_length			Length.
**	    .db_data			Ptr to the data of second param.
**      rdv				Pointer DB_DATA_VALUE for the result.
**	    .db_datatype		Datatype.
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
**      rdv				Pointer DB_DATA_VALUE for the result.
**	    .db_data			Result will be placed here.
**
**	Returns:
**	    E_DB_OK
**
**
** Side Effects:
**	    none
**
** History:
**	18-dec-89 (jrb)
**	    Created.
**	15-Mar-2005 (schka24)
**	    Minor cleanup: use 9998, don't need to check rdv nullability.
**      23-may-2007 (smeke01) b118342/b118344
**	    Work with i8.
*/

DB_STATUS
adu_iftrue(
ADF_CB              *adf_scb,
DB_DATA_VALUE       *dv1,
DB_DATA_VALUE       *dv2,
DB_DATA_VALUE       *rdv)
{
    i4			val;
    u_char		*nullp;


    /* Check first argument to make sure it's following the rules (first
    ** parameter must have non-nullable integer as its type)
    */
    if (dv1->db_datatype != DB_INT_TYPE)
	return(adu_error(adf_scb, E_AD2080_IFTRUE_NEEDS_INT, 0));
    
    /* result type must be nullable version of 2nd arg */
    if (-abs(dv2->db_datatype) != rdv->db_datatype)
	return(adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "adu_iftrue type"));

    /* length of result must be either the same or one greater than 2nd arg's */
    if (rdv->db_length != dv2->db_length && rdv->db_length-1 != dv2->db_length)
	return(adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "adu_iftrue len"));

    switch (dv1->db_length)
    {
      case 1:
	val = I1_CHECK_MACRO(*(i1 *)dv1->db_data);
	break;
      case 2:
	val = *(i2 *)dv1->db_data;
	break;
      case 4:
        val = *(i4 *)dv1->db_data;
	break;
      case 8:
	if (*(i8 *)dv1->db_data)
	   val = 1;
	else
	   val = 0;
	break;
      default:
	return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "adu_iftrue val length"));
    }

    nullp = (u_char *) rdv->db_data + rdv->db_length - 1;
    if (val)
    {
	*nullp = 0;
	MEcopy((PTR)dv2->db_data, dv2->db_length, (PTR)rdv->db_data);
    }
    else
    {
	*nullp = ADF_NVL_BIT;
    }

    return(E_DB_OK);
}
