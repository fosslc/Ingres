/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cv.h>
#include    <cm.h>
#include    <ex.h>
#include    <me.h>
#include    <mh.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduint.h>
#include    <clfloat.h>

/**
**
**  Name: ADUDEC.C - Functions for implementing decimal() function in INGRES
**
**  Description:
**        This file contains functions which are used to convert various
**	datatypes to decimal.  It also contains some float analogs of
**	a few decimal functions.
**
**	This file includes:
**
**	    adu_2dec_convert() - Coerces a DB_DATA_VALUE with an intrinsic type
**				 (or "char", "varchar", or "longtext") to a
**				 DB_DATA_VALUE of type "decimal".
**	    adu_decround()	- computes the round() function.
**
**  Function prototypes defined in ADUINT.H file.
**
**  History:
**	18-jul-89 (jrb)
**	    Created.
**      04-jan-1993 (stevet)
**          Added function prototypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      15-Dec-2009 (coomi01) b122767
**          Add clfloat header
**	10-May-2010 (kschendel) b123712
**	    Add floating trunc, ceil, floor.
**/


/*{
** Name: adu_2dec_convert() - Convert various types to the "decimal" datatype
**
** Description:
**
**	Implement "decimal(expr,p,s)" which actually compiles as
**	"decimal(expr,precscale)" where the p,s args are combined
**	into one precision-scale input parameter.
**
**	Actually, the precision-scale parameter is ignored anyway.
**	We take the desired precision, scale from the result data
**	value descriptor.  The two should be identical, and if they
**	aren't, it's only because the type resolver managed to lose
**	its way.  Consider:
**	select case when a=1 then decimal(exp,5,0)
**	            when a=2 then decimal(exp,10,2)
**	            else null end
**	from ...
**
**	The CASE expression is type-resolved to decimal(10,2) because
**	that's the maximum precision/scale among the WHEN branches.
**	However by the time we figure that out, the decimal conversion
**	parameters (5,0) and (10,2) have already been compiled.
**	The query compiler manages to catch itself by inserting extra
**	coercions, but the bottom line is that the result p,s is the
**	one that must be correct.  So, we'll ignore the input p,s.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	    .adf_decimal		DB_DECIMAL struct.
**		.db_decspec		TRUE if decimal character is specified
**					in .db_decimal.  If FALSE, '.' will be
**					used as the decimal character.
**              .db_decimal             If .db_decspec is TRUE, then this is
**					the decimal character to use.
**					    ***                            ***
**					    *** NOTE THAT THIS IS A "nat", ***
**					    *** NOT A "char".              ***
**					    ***                            ***
**      dv1                             Ptr to DB_DATA_VALUE to be coerced.
**	    .db_datatype		Type of data to be coerced.
**	    .db_prec			Precision/Scale of data to be coerced.
**	    .db_length			Length of data to be coerced.
**	    .db_data			Ptr to actual data to be coerced.
**      dv2
**	    Ignored integer (p,s) parameter
**
**	rdv				Ptr to DB_DATA_VALUE to hold resulting
**					decimal value.
**	    .db_prec			The precision/scale of the result type
**	    .db_length			The length of the result type.
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
**	    .db_data			Ptr to resulting decimal value.
**
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD0000_OK			Successful completion.
**	    E_AD200B_BAD_PREC		Invalid precision specified for DECIMAL;
**					must be: 1<= precision <=DB_MAX_DECPREC
**	    E_AD200C_BAD_SCALE		Invalid scale specified for DECIMAL;
**					must be: 0<= scale <=precision
**	    E_AD5003_BAD_CVTONUM	The coercion from whatever datatype to
**					the decimal datatype failed.
**	    E_AD5004_OVER_MAXTUP	dv's length was too long.
**	    E_AD9999_INTERNAL_ERROR	Coding error.
**
**  History:
**	18-jul-89 (jrb)
**	    Created.
**	15-Nov-2009 (kschendel) SIR 122890
**	    Ignore second input, use precision/scale from result DBDV.
*/

DB_STATUS
adu_2dec_convert(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE	    *dv2,
DB_DATA_VALUE	    *rdv)
{

    /* do the conversion by using the coercion routine */
    return(adu_1dec_coerce(adf_scb, dv1, rdv));
}


/*{
** Name: adu_decround()	- Perform round() function on decimal values.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to DB_DATA_VALUE to be rounded.
**	    .db_datatype		Type of data to be rounded (must be 
**					decimal).
**	    .db_prec			Precision/Scale of data to be rounded.
**	    .db_length			Length of data to be rounded.
**	    .db_data			Ptr to actual data to be rounded.
**      dv2				Ptr to DB_DATA_VALUE of integer constant
**					rounding factor.
**	    .db_datatype		Must be DB_INT_TYPE
**	    .db_length			Length of integer parameter.
**	    .db_data			Ptr to integer parameter.
**	rdv				Ptr to DB_DATA_VALUE to hold resulting
**					decimal value.
**	    .db_prec			The precision/scale of the result type
**	    .db_length			The length of the result type.
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
**	    .db_data			Ptr to resulting decimal value.
**
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD0000_OK			Successful completion.
**	    E_AD200B_BAD_PREC		Invalid precision specified for DECIMAL;
**					must be: 1<= precision <=DB_MAX_DECPREC
**	    E_AD200C_BAD_SCALE		Invalid scale specified for DECIMAL;
**					must be: 0<= scale <=precision
**	    E_AD5003_BAD_CVTONUM	The coercion from whatever datatype to
**					the decimal datatype failed.
**	    E_AD5004_OVER_MAXTUP	dv's length was too long.
**	    E_AD9999_INTERNAL_ERROR	Coding error.
**
**  History:
**	9-mar-2007 (dougi)
**	    Written to support round() function for TPC DS.
*/
DB_STATUS
adu_decround(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE	    *dv2,
DB_DATA_VALUE	    *rdv)
{
    char	deccon[(DB_MAX_DECPREC+1)/2];
    char	rescon[(DB_MAX_DECPREC+1)/2];
    i4		i;
    char	signbyte;
    i4		rfact;
    i4		p1, pc, pw, pr, s1, sc, sw, sr, wlen, zcount;
    PTR		rptr, sptr, fptr;
    bool	neground, doadd;


    /* 2nd param must be integer;  if it wasn't, it should have been caught
    ** before this point
    */
    if (dv2->db_datatype != DB_INT_TYPE)
	return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	
    /* Load rounding factor. */
    switch(dv2->db_length) {
      case 1:
	rfact = *((i1 *)dv2->db_data);
	break;

      case 2:
	rfact = *((i2 *)dv2->db_data);
	break;

      case 4:
	rfact = *((i4 *)dv2->db_data);
	break;

      case 8:
	rfact = *((i8 *)dv2->db_data);
	break;
    }

    p1 = DB_P_DECODE_MACRO(dv1->db_prec);
    s1 = DB_S_DECODE_MACRO(dv1->db_prec);
    pr = DB_P_DECODE_MACRO(rdv->db_prec);
    sr = DB_S_DECODE_MACRO(rdv->db_prec);
    rptr = &rescon[0];
    sptr = dv1->db_data;

    /* If rfact is > 0, it indicates places to right of decimal after
    ** which to round. If it is 0, rounding occurs to 1st position left
    ** of the decimal. If < 0, it indicates the number of places to
    ** left of the decimal at which rounding takes place. */
    if (s1 <= rfact)
    {
	/* No rounding - just copy source to result. */
	if (CVpkpk((PTR)dv1->db_data, p1, s1, pr, sr, (PTR)rdv->db_data)
			== CV_OVERFLOW)
	    EXsignal(EXDECOVF, 0);
	MEcopy(sptr, dv1->db_length, rptr);
	return(E_DB_OK);
    }

    /* Init some stuff. */
    for (i = 0; i < (DB_MAX_DECPREC-1)/2; i++)
	    deccon[i] = 0;
    signbyte = sptr[dv1->db_length-1] & 0x0f;
    doadd = TRUE;

    /* Round right or round left? */
    if (rfact < 0)
    {
	neground = TRUE;
	rfact = -rfact;
	zcount = rfact+sr;		/* digits to 0 after the round */
    }
    else 
    {
	neground = FALSE;
	zcount = sr-rfact;		/* digits to 0 after the round */
	rfact++;
    }

    fptr = &deccon[(DB_MAX_DECPREC+1)/2 - (rfact+2)/2];
					/* addr the factor to add */

    if (neground)
    {
	/* Round left of the decimal. */
	if (rfact > p1)
	{
	    /* If we're rounding beyond the capacity of the value,
	    ** the result is simply 0. */
	    rescon[0] = 0x0c;
	    pw = 1;
	    sw = 0;
	    doadd = FALSE;
	}
	else
	{
	    /* Construct rounding factor by inserting sign byte, then
	    ** placing "5" in correct nibble. */
	    deccon[i] = signbyte;
	    if ((rfact % 2) != 0)
		*fptr |= 0x50;
	    else *fptr = 0x05;

	    pc = rfact;
	    sc = 0;
	}

    }
    else
    {
	/* Round right of the decimal. Add 5e-(rfact+1), then clear 
	** digits following rounding point. */
	deccon[i] = 0x50;		/* set last byte's digit */
	deccon[i] |= signbyte;		/* set last byte's sign */
	pc = sc = rfact;
    }

    /* If needed (usually is), perform addition to round, then clear
    ** resulting digits. */
    if (doadd)
    {
	/* Add/subtract the rounding factor to produce result. */
	MHpkadd(sptr, p1, s1, fptr, pc, sc, rptr, &pw, &sw);

	/* Now locate the right hand end needing to be zero'ed & do it. */
	wlen = (pw + 2) / 2;			/* size of work result */
	rptr = rptr + (wlen - 1);		/* point to last byte */
	for (i = 0; i < zcount; i++)
	{
	    /* 1 digit per loop - alternating nibbles. */
	    if ((i/2)*2 == i)
	    {
		*rptr &= 0x0f;		/* 0 the left nibble */
		rptr--;			/* move to next byte */
	    }
	    else *rptr &= 0xf0;		/* 0 the right nibble */
	}
    }

    /* Copy to result field. */
    if (pw == pr && sw == sr)
	MEcopy((PTR)&rescon[0], rdv->db_length, (PTR)rdv->db_data);
    else
    {
	/* Need to adjust precision/scale. Call CVpkpk(). */
	if (CVpkpk((PTR)&rescon[0], pw, sw, pr, sr, (PTR)rdv->db_data)
			== CV_OVERFLOW)
	    EXsignal(EXDECOVF, 0);
    }

    return(E_DB_OK);

}

/*{
** Name: adu_fltround	- Perform round() function on floating point values.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to DB_DATA_VALUE to be rounded.
**	    .db_datatype		Type of data to be rounded (must be 
**					float).
**	    .db_length			Length of data to be rounded.
**	    .db_data			Ptr to actual data to be rounded.
**      dv2				Ptr to DB_DATA_VALUE of integer constant
**					rounding factor.
**	    .db_datatype		Must be DB_INT_TYPE
**	    .db_length			Length of integer parameter.
**	    .db_data			Ptr to integer parameter.
**	rdv				Ptr to DB_DATA_VALUE to hold resulting
**					float value.
**	    .db_length			The length of the result type.
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
**	    .db_data			Ptr to resulting float value.
**
** History:
**      04-Nov-2009 (coomi01) b122767
**            Created.
*/
DB_STATUS
adu_fltround(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE	    *dv2,
DB_DATA_VALUE	    *rdv)
{
    f8 result = 0.0;
    f8 target;
    i4 rfact;

    /* 1st param should be a float and 2nd param an integer;  
    ** if they weren't, it should have been caught before this point
    */
    if ( (dv2->db_datatype != DB_INT_TYPE) || 
	 (dv1->db_datatype != DB_FLT_TYPE) ||
	 (rdv->db_datatype != DB_FLT_TYPE)
	)
	return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    
    /* Load float value into a double (f8) */
    switch (dv1->db_length) 
    {
	case 8:
	    target = *(f8 *)dv1->db_data;
	    break;

	case 4:
	    target = (f8)(*(f4 *)dv1->db_data);
	    break;

	default:
	    return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	    break;
    }

    /* Load rounding factor. */
    switch (dv2->db_length) 
    {
	case 1:
	    rfact = *((i1 *)dv2->db_data);
	    break;

	case 2:
	    rfact = *((i2 *)dv2->db_data);
	    break;

	case 4:
	    rfact = *((i4 *)dv2->db_data);
	    break;

	case 8:
	    rfact = *((i8 *)dv2->db_data);
	    break;
	
	default:
	    return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	    break;
    }

    /*
    ** Do the lowlevel work 
    */
    result = MHround(target,rfact);

    /* 
    ** Return a result
    */
    switch (rdv->db_length) 
    {
	case 8:
	    *(f8 *)rdv->db_data = result;
	    break;

	case 4:
	{
	    /* Check when converting back from F8 to F4 */
	    if ((result >= FLT_MAX) || (result <= FLT_MIN))
	    {	    
		*(f4 *)rdv->db_data = 0.0;
		EXsignal(EXFLTOVF, 0);
	    }
	    else	    
		*(f4 *)rdv->db_data = (f4)result;
	}
	break;

	default:
	    return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	    break;
    }

    return(E_DB_OK);
}


/*{
** Name: adu_dectrunc()	- Perform trunc() function on decimal values.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to DB_DATA_VALUE to be truncated.
**	    .db_datatype		Type of data to be truncated (must be 
**					decimal).
**	    .db_prec			Precision/Scale of data to be truncated.
**	    .db_length			Length of data to be truncated.
**	    .db_data			Ptr to actual data to be truncated.
**      dv2				Ptr to DB_DATA_VALUE of integer constant
**					after which truncation is to be done.
**	    .db_datatype		Must be DB_INT_TYPE
**	    .db_length			Length of integer parameter.
**	    .db_data			Ptr to integer parameter.
**	rdv				Ptr to DB_DATA_VALUE to hold resulting
**					decimal value.
**	    .db_prec			The precision/scale of the result type
**	    .db_length			The length of the result type.
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
**	    .db_data			Ptr to resulting decimal value.
**
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD0000_OK			Successful completion.
**	    E_AD200B_BAD_PREC		Invalid precision specified for DECIMAL;
**					must be: 1<= precision <=DB_MAX_DECPREC
**	    E_AD200C_BAD_SCALE		Invalid scale specified for DECIMAL;
**					must be: 0<= scale <=precision
**	    E_AD5003_BAD_CVTONUM	The coercion from whatever datatype to
**					the decimal datatype failed.
**	    E_AD5004_OVER_MAXTUP	dv's length was too long.
**	    E_AD9999_INTERNAL_ERROR	Coding error.
**
**  History:
**	14-apr-2007 (dougi)
**	    Written to support truncate().
*/
DB_STATUS
adu_dectrunc(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE	    *dv2,
DB_DATA_VALUE	    *rdv)
{
    char	deccon[(DB_MAX_DECPREC+1)/2];
    char	rescon[(DB_MAX_DECPREC+1)/2];
    i4		i;
    i4		tfact;
    i4		p1, pr, s1, sr, rlen, zcount;
    PTR		rptr;

    /* 2nd param must be integer;  if it wasn't, it should have been caught
    ** before this point
    */
    if (dv2->db_datatype != DB_INT_TYPE)
	return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	
	
    /* Load truncation factor. */
    switch(dv2->db_length) {
      case 1:
	tfact = *((i1 *)dv2->db_data);
	break;

      case 2:
	tfact = *((i2 *)dv2->db_data);
	break;

      case 4:
	tfact = *((i4 *)dv2->db_data);
	break;

      case 8:
	tfact = *((i8 *)dv2->db_data);
	break;
    }

    p1 = DB_P_DECODE_MACRO(dv1->db_prec);
    s1 = DB_S_DECODE_MACRO(dv1->db_prec);
    pr = DB_P_DECODE_MACRO(rdv->db_prec);
    sr = DB_S_DECODE_MACRO(rdv->db_prec);

    /* First, copy the value to the result area. */
    if (CVpkpk((PTR)dv1->db_data, p1, s1, pr, sr, (PTR)rdv->db_data)
			== CV_OVERFLOW)
	EXsignal(EXDECOVF, 0);

    /* Figure out how many digits (from right) to 0. */
    if (tfact > sr)
	return(E_DB_OK);		/* no truncation, just return */

    zcount = sr - tfact;		/* no. of zeroes to place */
    if (zcount > pr)
	zcount = pr;

    /* Find end of result and stick in the 0's. */
    rlen = (pr + 2) / 2;
    rptr = rdv->db_data + (rlen - 1);

    for (i = 0; i < zcount; i++)
    {
	/* 1 digit per loop - alternating nibbles. */
	if ((i/2)*2 == i)
	{
	    *rptr &= 0x0f;		/* 0 the left nibble */
	    rptr--;			/* move to next byte */
	}
	else *rptr &= 0xf0;		/* 0 the right nibble */
    }

    return(E_DB_OK);

}


/*{
** Name: adu_flttrunc	- Perform trunc() function on floating point values.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to DB_DATA_VALUE to be trunced.
**	    .db_datatype		Type of data to be trunced (must be 
**					float).
**	    .db_length			Length of data to be trunced.
**	    .db_data			Ptr to actual data to be trunced.
**      dv2				Ptr to DB_DATA_VALUE of integer constant
**					truncing factor.
**	    .db_datatype		Must be DB_INT_TYPE
**	    .db_length			Length of integer parameter.
**	    .db_data			Ptr to integer parameter.
**	rdv				Ptr to DB_DATA_VALUE to hold resulting
**					float value.
**	    .db_length			The length of the result type.
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
**	    .db_data			Ptr to resulting float value.
**
** History:
**	10-May-2010 (kschendel) b123712
**            Created.
*/
DB_STATUS
adu_flttrunc(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE	    *dv2,
DB_DATA_VALUE	    *rdv)
{
    f8 result = 0.0;
    f8 target;
    i4 rfact;

    /* 1st param should be a float and 2nd param an integer;  
    ** if they weren't, it should have been caught before this point
    */
    if ( (dv2->db_datatype != DB_INT_TYPE) || 
	 (dv1->db_datatype != DB_FLT_TYPE) ||
	 (rdv->db_datatype != DB_FLT_TYPE)
	)
	return(adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "flttrunc types"));
    
    /* Load float value into a double (f8) */
    switch (dv1->db_length) 
    {
	case 8:
	    target = *(f8 *)dv1->db_data;
	    break;

	case 4:
	    target = (f8)(*(f4 *)dv1->db_data);
	    break;

	default:
	    return(adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "flttrunc flt len"));
	    break;
    }

    /* Load trunc position. */
    switch (dv2->db_length) 
    {
	case 1:
	    rfact = *((i1 *)dv2->db_data);
	    break;

	case 2:
	    rfact = *((i2 *)dv2->db_data);
	    break;

	case 4:
	    rfact = *((i4 *)dv2->db_data);
	    break;

	case 8:
	    rfact = *((i8 *)dv2->db_data);
	    break;
	
	default:
	    return(adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "flttrunc int len"));
	    break;
    }

    /*
    ** Do the lowlevel work 
    */
    result = MHtrunc(target,rfact);

    /* 
    ** Return a result
    */
    switch (rdv->db_length) 
    {
	case 8:
	    *(f8 *)rdv->db_data = result;
	    break;

	case 4:
	{
	    /* Check when converting back from F8 to F4 */
	    if ((result >= FLT_MAX) || (result <= FLT_MIN))
	    {	    
		*(f4 *)rdv->db_data = 0.0;
		EXsignal(EXFLTOVF, 0);
	    }
	    else	    
		*(f4 *)rdv->db_data = (f4)result;
	}
	break;

	default:
	    return(adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "flttrunc result len"));
	    break;
    }

    return(E_DB_OK);
} /* adu_flttrunc */


/*{
** Name: adu_decceil()	- Perform ceiling() function on decimal values.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to DB_DATA_VALUE to be ceilinged.
**	    .db_datatype		Type of data to be ceilinged (must be 
**					decimal).
**	    .db_prec			Precision/Scale of data to be ceilinged.
**	    .db_length			Length of data to be ceilinged.
**	    .db_data			Ptr to actual data to be ceilinged.
**	rdv				Ptr to DB_DATA_VALUE to hold resulting
**					decimal value.
**	    .db_prec			The precision/scale of the result type
**	    .db_length			The length of the result type.
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
**	    .db_data			Ptr to resulting decimal value.
**
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD0000_OK			Successful completion.
**	    E_AD200B_BAD_PREC		Invalid precision specified for DECIMAL;
**					must be: 1<= precision <=DB_MAX_DECPREC
**	    E_AD200C_BAD_SCALE		Invalid scale specified for DECIMAL;
**					must be: 0<= scale <=precision
**	    E_AD5003_BAD_CVTONUM	The coercion from whatever datatype to
**					the decimal datatype failed.
**	    E_AD5004_OVER_MAXTUP	dv's length was too long.
**	    E_AD9999_INTERNAL_ERROR	Coding error.
**
**  History:
**	14-apr-2007 (dougi)
**	    Written to support ceiling().
**	12-nov-2008 (dougi) BUG 121218
**	    Minor fix to properly locate last byte of intermediate result.
**	30-Mar-2010 (kschendel) b123493
**	    Don't smash the input, copy it.  Fix check for ceil(n.00),
**	    was looking at sign.
*/
DB_STATUS
adu_decceil(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE	    *rdv)
{
    i4		i, p1, pr, pw, s1, sr, sw, wlen, zcount;
    PTR		sptr, wptr;
    char	deccon[(DB_MAX_DECPREC+1)/2];
    char	conone = 0x1c;
    bool	negceil = FALSE, allzero = TRUE;


    /* Extract source/result precision/scale. */
    p1 = DB_P_DECODE_MACRO(dv1->db_prec);
    s1 = DB_S_DECODE_MACRO(dv1->db_prec);
    pr = DB_P_DECODE_MACRO(rdv->db_prec);
    sr = DB_S_DECODE_MACRO(rdv->db_prec);

    /* Loop over right side of decimal to see if there are non-0s.
    ** If not, there's nothing to do but assign result.
    ** Skip the sign nibble and work backwards.
    */
    for (i = 1, wptr = &dv1->db_data[dv1->db_length-1];
	i <= s1 && allzero; i++)
    {
	if ((i % 2) == 0)
	{
	    if (*wptr & 0x0f)
		allzero = FALSE;
	}
	else
	{
	    if (*wptr & 0xf0)
		allzero = FALSE;
	    wptr--;
	}
    }

    /* If source scale is 0 or everything right of the decimal is 0,
    ** there's no ceiling() to be done. Just assign source to result. */
    if (s1 == 0 || allzero)
    {
	/* Just copy source to result. */
	if (CVpkpk((PTR)dv1->db_data, p1, s1, pr, sr, (PTR)rdv->db_data)
			== CV_OVERFLOW)
	    EXsignal(EXDECOVF, 0);
	return(E_DB_OK);
    }

    /* Is source negative or positive? */
    switch (dv1->db_data[dv1->db_length-1] & 0x0f) {
      case 11:
      case 13:
	negceil = TRUE;
	break;
      case 10:
      case 12:
      case 14:
      case 15:
	negceil = FALSE;
	break;
    }

    /* If source is positive, add one & remove zeroes to perform ceilng.
    ** If negative, just remove zeroes. */
    if (!negceil)
    {
	MHpkadd(dv1->db_data, p1, s1, &conone, 1, 0, &deccon[0], &pw, &sw);
	wlen = (pw + 2) / 2;
    }
    else
    {
	MEcopy(dv1->db_data, dv1->db_length, &deccon[0]);
	pw = p1;
	sw = s1;
	wlen = dv1->db_length;
    }
    sptr = &deccon[0];

    /* Zero the digits right of the decimal. */
    for (i = 0, wptr = sptr + (wlen -1); i < sw; i++)
    {
	/* 1 digit per loop - alternating nibbles. */
	if ((i/2)*2 == i)
	{
	    *wptr &= 0x0f;		/* 0 the left nibble */
	    wptr--;			/* move to next byte */
	}
	else *wptr &= 0xf0;		/* 0 the right nibble */
    }

    /* Finally copy from source/computation to result. */
    if (CVpkpk(sptr, pw, sw, pr, sr, (PTR)rdv->db_data)
			== CV_OVERFLOW)
	EXsignal(EXDECOVF, 0);
    return(E_DB_OK);

}


/*{
** Name: adu_fltceil()	- Perform ceiling() function on float values.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to DB_DATA_VALUE to be ceilinged.
**	    .db_datatype		Type of data to be ceilinged (must be 
**					float).
**	    .db_length			Length of data to be ceilinged.
**	    .db_data			Ptr to actual data to be ceilinged.
**	rdv				Ptr to DB_DATA_VALUE to hold resulting
**					float value.
**	    .db_length			The length of the result type.
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
**	    .db_data			Ptr to resulting decimal value.
**
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.
**
**  History:
**	10-May-2010 (kschendel) b123712
**	    Add floating ceil.
*/
DB_STATUS
adu_fltceil(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE	    *rdv)
{
    f8	input, result;

    /* 1st param should be a float.
    ** if it isn't, it should have been caught before this point
    */
    if ( (dv1->db_datatype != DB_FLT_TYPE) ||
	 (rdv->db_datatype != DB_FLT_TYPE)
	)
	return(adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "fltceil type"));
    
    /* Load float value into a double (f8) */
    switch (dv1->db_length) 
    {
	case 8:
	    input = *(f8 *)dv1->db_data;
	    break;

	case 4:
	    input = (f8)(*(f4 *)dv1->db_data);
	    break;

	default:
	    return(adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "fltceil len"));
	    break;
    }


    /*
    ** Do the lowlevel work, use a C99 / posix.1-2001 function.
    */
    result = ceil(input);

    /* 
    ** Return a result
    */
    switch (rdv->db_length) 
    {
	case 8:
	    *(f8 *)rdv->db_data = result;
	    break;

	case 4:
	{
	    /* Check when converting back from F8 to F4 */
	    if ((result >= FLT_MAX) || (result <= FLT_MIN))
	    {	    
		*(f4 *)rdv->db_data = 0.0;
		EXsignal(EXFLTOVF, 0);
	    }
	    else	    
		*(f4 *)rdv->db_data = (f4)result;
	}
	break;

	default:
	    return(adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "fltceil result len"));
	    break;
    }

    return(E_DB_OK);

} /* adu_fltceil */


/*{
** Name: adu_decfloor()	- Perform floor() function on decimal values.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to DB_DATA_VALUE to be floor()ed
**	    .db_datatype		Type of data to be floor()ed (must be 
**					decimal).
**	    .db_prec			Precision/Scale of data to be floored.
**	    .db_length			Length of data to be floored.
**	    .db_data			Ptr to actual data to be floored.
**	rdv				Ptr to DB_DATA_VALUE to hold resulting
**					decimal value.
**	    .db_prec			The precision/scale of the result type
**	    .db_length			The length of the result type.
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
**	    .db_data			Ptr to resulting decimal value.
**
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD0000_OK			Successful completion.
**	    E_AD200B_BAD_PREC		Invalid precision specified for DECIMAL;
**					must be: 1<= precision <=DB_MAX_DECPREC
**	    E_AD200C_BAD_SCALE		Invalid scale specified for DECIMAL;
**					must be: 0<= scale <=precision
**	    E_AD5003_BAD_CVTONUM	The coercion from whatever datatype to
**					the decimal datatype failed.
**	    E_AD5004_OVER_MAXTUP	dv's length was too long.
**	    E_AD9999_INTERNAL_ERROR	Coding error.
**
**  History:
**	14-apr-2007 (dougi)
**	    Written to support floor().
**	12-nov-2008 (dougi) BUG 121218
**	    Minor fix to properly locate last byte of intermediate result.
**	30-Mar-2010 (kschendel) b123493
**	    Don't smash the input, copy it.
*/
DB_STATUS
adu_decfloor(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE	    *rdv)
{
    i4		i, p1, pr, pw, s1, sr, sw, wlen, zcount;
    PTR		sptr, wptr;
    char	deccon[(DB_MAX_DECPREC+1)/2];
    char	conmone = 0x1d;
    bool	negfloor = FALSE, allzero = TRUE;


    /* Extract source/result precision/scale. */
    p1 = DB_P_DECODE_MACRO(dv1->db_prec);
    s1 = DB_S_DECODE_MACRO(dv1->db_prec);
    pr = DB_P_DECODE_MACRO(rdv->db_prec);
    sr = DB_S_DECODE_MACRO(rdv->db_prec);

    /* Loop over right side of decimal to see if there are non-0s.
    ** If not, there's nothing to do but assign result.
    ** Skip the sign nibble and work backwards.
    */
    for (i = 1, wptr = &dv1->db_data[dv1->db_length-1];
	i <= s1 && allzero; i++)
    {
	if ((i % 2) == 0)
	{
	    if (*wptr & 0x0f)
		allzero = FALSE;
	}
	else
	{
	    if (*wptr & 0xf0)
		allzero = FALSE;
	    wptr--;
	}
    }

    /* If source scale is 0 or everything right of the decimal is 0,
    ** there's no floor() to be done. Just assign source to result. */
    if (s1 == 0 || allzero)
    {
	/* Just copy source to result. */
	if (CVpkpk((PTR)dv1->db_data, p1, s1, pr, sr, (PTR)rdv->db_data)
			== CV_OVERFLOW)
	    EXsignal(EXDECOVF, 0);
	return(E_DB_OK);
    }

    /* Is source negative or positive? */
    switch (dv1->db_data[dv1->db_length-1] & 0x0f) {
      case 11:
      case 13:
	negfloor = TRUE;
	break;
      case 10:
      case 12:
      case 14:
      case 15:
	negfloor = FALSE;
	break;
    }

    /* If source is negative, subtract one & remove zeroes to perform floor.
    ** If positive, just remove zeroes. */
    if (negfloor)
    {
	MHpkadd(dv1->db_data, p1, s1, &conmone, 1, 0, &deccon[0], &pw, &sw);
	wlen = (pw + 2) / 2;
    }
    else
    {
	MEcopy(dv1->db_data, dv1->db_length, &deccon[0]);
	pw = p1;
	sw = s1;
	wlen = dv1->db_length;
    }
    sptr = &deccon[0];

    /* Zero the digits right of the decimal. */
    for (i = 0, wptr = sptr + (wlen -1); i < sw; i++)
    {
	/* 1 digit per loop - alternating nibbles. */
	if ((i/2)*2 == i)
	{
	    *wptr &= 0x0f;		/* 0 the left nibble */
	    wptr--;			/* move to next byte */
	}
	else *wptr &= 0xf0;		/* 0 the right nibble */
    }

    /* Copy from source/computation to result. */
    if (CVpkpk(sptr, pw, sw, pr, sr, (PTR)rdv->db_data)
			== CV_OVERFLOW)
	EXsignal(EXDECOVF, 0);
    return(E_DB_OK);

}


/*{
** Name: adu_fltfloor()	- Perform floor() function on float values.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to DB_DATA_VALUE to be floor'ed.
**	    .db_datatype		Type of data to be floor'ed (must be 
**					float).
**	    .db_length			Length of data to be floor'ed.
**	    .db_data			Ptr to actual data to be floor'ed.
**	rdv				Ptr to DB_DATA_VALUE to hold resulting
**					float value.
**	    .db_length			The length of the result type.
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
**	    .db_data			Ptr to resulting decimal value.
**
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.
**
**  History:
**	10-May-2010 (kschendel) b123712
**	    Add floating floor.
*/
DB_STATUS
adu_fltfloor(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE	    *rdv)
{
    f8	input, result;

    /* 1st param should be a float.
    ** if it isn't, it should have been caught before this point
    */
    if ( (dv1->db_datatype != DB_FLT_TYPE) ||
	 (rdv->db_datatype != DB_FLT_TYPE)
	)
	return(adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "fltfloor type"));
    
    /* Load float value into a double (f8) */
    switch (dv1->db_length) 
    {
	case 8:
	    input = *(f8 *)dv1->db_data;
	    break;

	case 4:
	    input = (f8)(*(f4 *)dv1->db_data);
	    break;

	default:
	    return(adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "fltfloor len"));
	    break;
    }


    /*
    ** Do the lowlevel work, use a C99 / posix.1-2001 function.
    */
    result = floor(input);

    /* 
    ** Return a result
    */
    switch (rdv->db_length) 
    {
	case 8:
	    *(f8 *)rdv->db_data = result;
	    break;

	case 4:
	{
	    /* Check when converting back from F8 to F4 */
	    if ((result >= FLT_MAX) || (result <= FLT_MIN))
	    {	    
		*(f4 *)rdv->db_data = 0.0;
		EXsignal(EXFLTOVF, 0);
	    }
	    else	    
		*(f4 *)rdv->db_data = (f4)result;
	}
	break;

	default:
	    return(adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "fltfloor result len"));
	    break;
    }

    return(E_DB_OK);

} /* adu_fltfloor */


/*{
** Name: adu_decprec() - Determine best precision and scale for a string.
**
** Description:
**      adu_decprec() Determine best precision and scale for a string by
**			inspection.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to DB_DATA_VALUE containing first
**					argument - not necessaarily a string.
**	    .db_datatype		Its datatype.
**	    .db_length			Its length.
**	    .db_data			Pointer to the LHS data
**
** Outputs:
**	ps				Pointer to an i2 to receive encoded
**					precision and scale.
**					On error return this will be set to 0
**					
**
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD0000_OK			Completed sucessfully.
**	    E_AD9999_INTERNAL_ERROR	dv1's datatype was incompatible with
**					this operation.
**	    E_AD1080_STR_TO_DECIMAL	Is not a valid decimal string
**
**  History:
**	11-Mar-2009 (kiria01) b121781
**	    Created
*/

DB_STATUS
adu_decprec(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv1,
i2		*ps
)
{
    DB_STATUS	db_stat = E_DB_OK;
    char	decimal = (adf_scb->adf_decimal.db_decspec ?
			(char) adf_scb->adf_decimal.db_decimal : '.');
    i4		length;
    char	temp[64], *p, *hold;
    i4		integral, scale, expnt, prec;

    *ps = 0;
    switch (dv1->db_datatype)
    {
    case DB_CHA_TYPE:
    case DB_CHR_TYPE:
        length = dv1->db_length;
	if (length >= sizeof(temp))
	    length = sizeof(temp)-1;
	MEcopy(dv1->db_data, length, temp);
	/* Drop to common string processing */
        break;

    case DB_VCH_TYPE:
    case DB_TXT_TYPE:
    case DB_DYC_TYPE:
    case DB_LTXT_TYPE:
    case DB_QUE_TYPE:
    case DB_UTF8_TYPE: /* Not expecting UTF8 numbers to need normalising */
    case DB_QTXT_TYPE:
    case DB_TFLD_TYPE:
        length = ((DB_TEXT_STRING *) dv1->db_data)->db_t_count;
	if (length >= sizeof(temp))
	    length = sizeof(temp)-1;
        MEcopy((PTR)((DB_TEXT_STRING *)dv1->db_data)->db_t_text, length, (PTR)temp);
	/* Drop to common string processing */
        break;

    case DB_NCHR_TYPE:
    case DB_NVCHR_TYPE:
    case DB_NQTXT_TYPE:
	{
	    DB_DATA_VALUE cdata;
	    cdata.db_datatype = DB_CHA_TYPE;
	    cdata.db_length = sizeof(temp) - 1;
	    cdata.db_data = (PTR)temp;

	    db_stat = adu_nvchr_coerce(adf_scb, dv1, &cdata);
	    if (db_stat)
		return (db_stat);
	    length = cdata.db_length;
	}
	/* Drop to common string processing */
	break;

    default:
	return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    }

    /* Common string check */
    temp[length] = EOS;
    p = temp;
    /* Lose leading spaces */
    while (CMspace(p))
	CMnext(p);
    integral = scale = expnt = 0;
    if (*p == '-' || *p == '+')
    {
	/* Skip sign and any associated spaces */
	CMnext(p);
	while (CMspace(p))
	    CMnext(p);
    }
    /* Skip any leading zeros */
    while (*p == '0')
	CMnext(p);

    hold = p;
    while (CMdigit(p))
        CMnext(p);
    integral = p - hold;
    if (*p == decimal)
    {
        CMnext(p);
	hold = p;
	while (CMdigit(p))
	    CMnext(p);
	scale = p - hold;
    }
    if (*p == 'e' || *p == 'E')
    {
	i4 neg;
	CMnext(p);
	if ((neg = *p == '-') || *p == '+')
	    CMnext(p);
	while (CMdigit(p))
	{
	    expnt = 10 * expnt + *p - '0';
	    CMnext(p);
	    if (expnt > 2 * CL_MAX_DECPREC)
		return E_AD1080_STR_TO_DECIMAL;
	}
	if (neg)
	    expnt = -expnt;
    }
    while (CMspace(p))
	CMnext(p);
    if (*p)
	return E_AD1080_STR_TO_DECIMAL;

    /* Fold in exponent */
    integral += expnt;
    if (integral < 0)
	integral = 0;
    scale -= expnt;
    if (scale < 0)
	scale = 0;
    prec = integral + scale;
    if (integral == 0)
	prec++;
    if (prec > CL_MAX_DECPREC)
	return E_AD1080_STR_TO_DECIMAL;
    *ps = DB_PS_ENCODE_MACRO(prec, scale);

    return E_AD0000_OK;
}

