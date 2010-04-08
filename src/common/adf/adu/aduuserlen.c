/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduint.h>
/*
[@#include@]...
*/

/**
**
**  Name: ADUUSERLEN.C - Holds routines to process INGRES iiuserlen() function.
**
**  Description:
**      This file contains all routines needed to process the iiuserlen()
**      function.
**
**          adu_userlen() - INGRES's iiuserlen() function.
[@func_list@]...
**
**  Function prototypes defined in ADUINT.H file.
**
**  History:
**      07-jun-87 (thurston)
**          Initial creation.
**	20-mar-89 (jrb)
**	    Made changes for adc_lenchk interface change.
**      05-jan-1993 (stevet)
**          Added function prototypes.
**       1-Jul-1993 (fred)
**          Altered call to adc_lenchk() to match prototype.  See
**          adf.h for details.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
[@history_template@]...
**/


/*
[@forward_type_references@]
[@forward_function_references@]
[@#defines_of_other_constants@]
[@type_definitions@]
[@global_variable_definitions@]
[@static_variable_or_function_definitions@]
*/


/*{
** Name: adu_userlen() - INGRES's iiuserlen() function.
**
** Description:
**      INGRES's iiuserlen() function.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1				Ptr to DB_DATA_VALUE for first param,
**					which will be the datatype ID.
**	    .db_datatype		Datatype; assumed to be DB_INT_TYPE.
**	    .db_length			Length.
**	    .db_data			Ptr to the data of first param.
**      dv2				Ptr to DB_DATA_VALUE for second param,
**					which will be the internal length times
**					65536 plus the prec_scale, both numbers
**					taken from the iiattribute catalog.  The
**					reason this is done in this fashion is
**					two fold:  First, we need both pieces of
**					info for the upcoming DECIMAL datatype,
**					and second, INGRES currently does not
**					support functions of three args.
**	    .db_datatype		Datatype; assumed to be DB_INT_TYPE.
**	    .db_length			Length.
**	    .db_data			Ptr to the data of first param.
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
**	    .db_data			Result will be placed here.  This will
**					be the user defined length for most
**					datatypes.  However, for DECIMAL, this
**					will be the user specified `precision'.
**
**	Returns:
**	    {@return_description@}
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      07-jun-87 (thurston)
**          Initial creation.
**	20-mar-89 (jrb)
**	    Made changes for adc_lenchk interface change and for decimal
**	    datatype support.
**       1-Jul-1993 (fred)
**          Removed (bool) cast for adc_lenchk() since second argument
**          is now a nat.  See adf.h for details.
**      23-may-2007 (smeke01) b118342/b118344
**	    Work with i8.
[@history_template@]...
*/

DB_STATUS
adu_userlen(
ADF_CB             *adf_scb,
DB_DATA_VALUE      *dv1,
DB_DATA_VALUE      *dv2,
DB_DATA_VALUE      *rdv)
{
    DB_STATUS		db_stat;
    DB_DATA_VALUE	tmp_dv;
    DB_DATA_VALUE	adu_dv;
    i4			ulen;


    /* First, set the datatype */
    /* ----------------------- */
    switch (dv1->db_length)
    {
      case 1:
	tmp_dv.db_datatype = (DB_DT_ID) I1_CHECK_MACRO(*(i1 *)dv1->db_data);
	break;

      case 2:
	tmp_dv.db_datatype = (DB_DT_ID) *(i2 *)dv1->db_data;
	break;

      case 4:
	tmp_dv.db_datatype = (DB_DT_ID) *(i4 *)dv1->db_data;
	break;

      case 8:
	tmp_dv.db_datatype = (DB_DT_ID) *(i8 *)dv1->db_data;
	break;

      default:
	return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "iiuserlen datatype length"));
    }


    /* Next, set the internal length, and precision/scale */
    /* -------------------------------------------------- */
    switch (dv2->db_length)
    {
      case 1:
	tmp_dv.db_length = I1_CHECK_MACRO(*(i1 *)dv2->db_data);
	break;

      case 2:
	tmp_dv.db_length = *(i2 *)dv2->db_data;
	break;

      case 4:
	tmp_dv.db_length = *(i4 *)dv2->db_data;
	break;

      case 8:
	tmp_dv.db_length = (i4) *(i8 *)dv2->db_data;
	break;

      default:
	return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "iiuserlen len length"));
    }
    /* Gee, this doesn't make too much sense if dv2 wasn't an i4, eh? */
    tmp_dv.db_prec = tmp_dv.db_length % 65536;
    tmp_dv.db_length /= 65536;


    /* Use adc_lenchk() to get the user length */
    if (db_stat = adc_lenchk(adf_scb, FALSE, &tmp_dv, &adu_dv))
	return(db_stat);

    /* if decimal then return precision instead of length */
    if (abs(tmp_dv.db_datatype) == DB_DEC_TYPE)
	ulen = DB_P_DECODE_MACRO(adu_dv.db_prec);
    else
	ulen = adu_dv.db_length;
    

    /* Now set result */
    /* -------------- */
    switch (rdv->db_length)
    {
      case 1:
	*(i1 *)rdv->db_data = ulen;
	break;

      case 2:
	*(i2 *)rdv->db_data = ulen;
	break;

      case 4:
	*(i4 *)rdv->db_data = ulen;
	break;

      default:
	return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "iiuserlen result length"));
    }

    return(E_DB_OK);
}

/*
[@function_definition@]...
*/
