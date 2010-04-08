/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <mh.h>
#include    <ex.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduint.h>

/**
**
**  Name: ADUINTPOW.C - This file implements the exponentiation operation.
**
**  Description:
**        This file contains routines used to implement the exponentiation
**	operation.
**	This file defines:
**
**	    adu_2int_pow() -	    raise a number to an integral power.
**
**  Function prototype defined in ADUINT.H file.
**
**  History:    $Log-for RCS$
**      20-may-86 (ericj)    
**          Converted for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	29-apr-87 (thurston)
**	    Added specific check in adu_2int_pow() to see if arg 1 is zero and
**	    arg 2 is non-positive before calling MHipow(), and if so, return
**	    appropriate error code.
**	24-aug-87 (thurston)
**	    In adu_2int_pow(), I removed the specific check for a bad math arg
**          (see history comment by me dated 29-apr-87).  Instead, we now depend
**          on the MH routines to signal MH_BADARG, and adx_handler() to deal
**          with it. 
**      04-jan-1993 (stevet)
**          Added function prototypes.
**      19-feb-1999 (matbe01)
**          Move include for ex.h after include for mh.h. The check for "ifdef
**          EXCONTINUE" in ex.h does no good if sys/context.h (via mh.h) is
**          included after ex.h.
**/


/*{
** Name: adu_2int_pow() - Raises a number to an integral power.
**
** Description:
**	  This routine raises a number to an integral power.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      num_dv                          DB_DATA_VALUE describing the number
**					which is to be raised to some power.
**	    .db_datatype		Its type.
**	    .db_length			Its length.
**	    .db_data			Ptr to the data which is to be
**					interpreted as some number.
**	exp_dv				DB_DATA_VALUE describing the exponent
**					operand.
**	    .db_datatype		Its type.
**	    .db_length			Its length.
**	    .db_data			Ptr to the data which is to be inter-
**					preted as some number.
**      rdv                             DB_DATA_VALUE describing the result
**					data value, which is assumed to be a
**					float.
**	    .db_length			Its length, 4 or 8?
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
**	rdv
**	    .db_data			Ptr to the area which is to hold the
**					result.
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD0000_OK			Completed successfully.
**
**	Exceptions:
**					All of the hardware generated math
**					exceptions are possible.
**
** History:
**	20-may-86 (ericj)
**          Converted for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	29-apr-87 (thurston)
**	    Added specific check to see if arg 1 is zero and arg 2 is
**	    non-positive before calling MHipow(), and if so, return
**	    appropriate error code.
**	24-aug-87 (thurston)
**	    Removed the specific check for a bad math arg (see history comment
**	    by me dated 29-apr-87).  Instead, we now depend on the MH routines
**	    to signal MH_BADARG, and adx_handler() to deal with it.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	08-aug-93 (shailaja)
**	    unnested comments.
**	25-aug-93 (ed)
**	    remove dbdbms.h
*/

DB_STATUS
adu_2int_pow(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *num_dv,
DB_DATA_VALUE	    *exp_dv,
DB_DATA_VALUE	    *rdv)
{
    DB_STATUS		db_stat;
    i4			e;
    f8			d;
    DB_DATA_VALUE	exp;
    DB_DATA_VALUE	flt;
    FUNC_EXTERN f8	MHipow();


    EXmath((i4)EX_ON);

    for(;;)
    {
	exp.db_datatype = DB_INT_TYPE;
	exp.db_length = 4;
	exp.db_data = (PTR) &e;
	flt.db_datatype = DB_FLT_TYPE;
	flt.db_length = 8;
	flt.db_data = (PTR) &d;

	if ((db_stat = adu_1int_coerce(adf_scb, exp_dv, &exp)) != E_DB_OK)
	    break;
	if ((db_stat = adu_1flt_coerce(adf_scb, num_dv, &flt)) != E_DB_OK)
	    break;

/*******************************************************************************
**  Depend on MH to signal MH_BADARG, and adx_handler() to deal with it.
**
**	if (d == 0.0  &&  e <= 0)
**	{
**	    db_stat = adu_error(adf_scb, E_AD6000_BAD_MATH_ARG, 0);
**	    break;
**	}
*******************************************************************************/

	d = MHipow(d, (i4)e);
	db_stat = adu_1flt_coerce(adf_scb, &flt, rdv);
	break;
    }

    EXmath((i4)EX_OFF);
    return(db_stat);
}
