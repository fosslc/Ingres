/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
NO_OPTIM = sgi_us5
*/

#include    <compat.h>
#include    <gl.h>
#include    <cv.h>
#include    <ex.h>
#include    <me.h>
#include    <clfloat.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduint.h>
#include    <adumoney.h>

/**
**
**  Name: ADUCOERCE.C - Functions for coercing to numeric datatypes.
**
**  Description:
**        This file contains functions which are used to coerce various
**	datatypes to numeric datatypes.
**
**	This file includes:
**
**	    adu_1flt_coerce() -	Coerces a DB_DATA_VALUE with an intrinsic type
**				(or "char", "varchar", or "longtext") to a
**				DB_DATA_VALUE of type "f".
**	    adu_1dec_coerce() -	Coerces a DB_DATA_VALUE with an intrinsic type
**				(or "char", "varchar", or "longtext") to a
**				DB_DATA_VALUE of type "decimal".
**	    adu_1int_coerce() -	Coerces a DB_DATA_VALUE with an intrinsic type
**				(or "char", "varchar", or "longtext") to a
**				DB_DATA_VALUE of type "i".
**
**  Function prototypes defined in ADUINT.H
**
**  History:
**      28-apr-86 (ericj)    
**          Converted for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	14-aug-86 (thurston)
**	    Changed all calls to MEcopy() to use PTR's instead of char*'s.
**	30-sep-86 (thurston)
**	    Made adu_1flt_coerce() and adu_1int_coerce() work for char,
**	    varchar, and longtext.
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	19-mar-87 (thurston)
**	    Specifically checked for integer overflow in adu_1int_coerce() and
**	    signal exception if detected.
**	21-apr-87 (thurston)
**          Made a fix to adu_1int_coerce(), where I had a "bug" reported that
**          `int4(40000.00 * .5)' was coming out as `19999', while
**          `int4(40000.00 / 2)' was coming out as `20000'.  Sure enough, in
**          5.0, both resulted in `20000'.  This may sound strange, but I
**          `fixed' the problem by casting my temp f8 into an f4 before casting
**          it to the final i4 value.  Hey, whatever works. 
**	12-jun-87 (thurston)
**	    Since the previous band-aide applied to adu_1int_coerce() (casting
**	    to f4 before assigning to i4) only made the system break elsewhere,
**	    I have removed that fix.  The real fix has `presumably' been made
**	    to the CVexpten() routine in CVDECIMAL.MAR.
**      15-may-90 (jkb)
**          add EXmath to adu_1flt_coerce to force floating point
**          exceptions to be returned from the 387 co-processor when
**          they occur, rather than with the next floating point operation
**      31-dec-1992 (stevet)
**          Added function prototypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**      17-jan-1993 (stevet)
**          Added floating point overflow detections (B58853).
**	28-apr-94 (vijay)
**	    Change abs to fabs.
**      01-sep-1994 (mikem)
**          bug #64261
**          Changed calls to fabs() to calls to CV_ABS_MACRO()
**          On some if not all UNIX's fabs() is an actual
**          routine call which takes a double as an argument and returns a
**          double.  Calls to this code did not have proper includes to
**          define the return type so the return code was assumed to be int
**          (basically returning random data), in adumoney.c this caused bug
**          #64261, in other files garbage return values from fabs would cause
**          other problems.
**	15-jun-95 (popri01)
**	    Add NO_OPTIM for nc4_us5 (NCR C Development Toolkit - 2.03.01)
**	    to eliminate SIGSEGV which appeared in sep tests cca00 and caa04, and
**	    was traced to routine adu_1flt_coerce. The following QUEL statements 
**	    should test for the problem:
**		create cca00_t7 (func=c15, f8=f8, i4=i4, c25=c25) \p\g
**		append cca00_t7 (i4=mod(11.,5.)) \p\g
**	12-Jan-1999 (kosma01)
**	    Add NO OPTIM for sgi_us5 to persuade vis24.sep to correctly produce
**	    the correct floating point value for the c2503 subfunction of the
**	    app2503 application created by vis24.sep. ingres/sgi was producing
**	    a value of 0.0 e 0. adu_1flt_coerce() was the culprit.
**	22-jun-1999 (musro02)
**	    Remove NO_OPTIM for nc4_us5.  Strangely, it caused a segv at the
**          beginning of aud_1int_coerce when running various front-end tests 
**          under the newer version of the c compiler:
**              NCR High Performance C Compiler R3.0 (SCDE 3.03.00)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	12-apr-04 (inkdo01)
**	    Add support of BIGINT (i8).
**	14-May-2004 (schka24)
**	    Use MINI4LL in i8 contexts to avoid C typing trap.
**	12-Dec-2008 (kiria01) b121366
**	    Alignment access to db_data protected.
**/


/*{
** Name: adu_1flt_coerce() - Coerce various types to the "f" datatype (floats).
**
** Description:
**	  This routine coerces the values of various datatypes in a
**	DB_DATA_VALUE into a "f" data value in a DB_DATA_VALUE.
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
**	dv				Ptr to DB_DATA_VALUE to be coerced.
**	    .db_length			The length of value to be coerced.
**	    .db_prec			The precision/scale of value to be
**					coerced.
**	    .db_data			Ptr to data to be coerced.
**	rdv				Ptr to result DB_DATA_VALUE.
**	    .db_length			The length of the result DB_DATA_VALUE.
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
**					an ADF error code or a user-error cod
e.
**		.ad_emsglen		The length, in bytes, of the resulting
**					formatted error message.
**		.adf_errmsgp		Pointer to the formatted error message.
**      rdv
**	    .db_data			Ptr to result data value.
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
**	    E_AD5003_BAD_CVTONUM	The coercion from some type to a float
**					datatype failed.
**	    E_AD5004_OVER_MAXTUP	dv's length was too long.
**	    E_AD9999_INTERNAL_ERROR	Coding error.
**
** History:
**	28-apr-86 (ericj)
**            Converted for Jupiter.  Fixed use of fval in I1_CHECK_MACRO().
**	    I1_CHECK_MACRO should operate only on integers.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	30-sep-86 (thurston)
**	    Made to work for char, varchar, and longtext.
**	14-jul-89 (jrb)
**	    Added support for decimal datatype.
**      15-may-90 (jkb)
**          add EXmath to adu_1flt_coerce to force floating point
**          exceptions to be returned from the 387 co-processor when
**          they occur, rather than with the next floating point operation
**      17-jan-1993 (stevet)
**          Added floating point overflow detections.
**	28-apr-94 (vijay)
**	    Change abs to fabs.
**      01-sep-1994 (mikem)
**          bug #64261
**          Changed calls to fabs() to calls to CV_ABS_MACRO()
**          On some if not all UNIX's fabs() is an actual
**          routine call which takes a double as an argument and returns a
**          double.  Calls to this code did not have proper includes to
**          define the return type so the return code was assumed to be int
**          (basically returning random data), in adumoney.c this caused bug
**          #64261, in other files garbage return values from fabs would cause
**          other problems.
**      21-apr-2004 (huazh01)
**           instead of using dv->db_length to test the E_AD5004 error,
**           we should use the total number of chars in the string that
**           needs to be converted to float to test such error. 
**           This fixes bug 112175, INGSRV2800.
**	12-may-2004 (gupsh01)
**	    Added support for nchar and nvarchar datatypes.
**	18-may-2004 (gupsh01)
**	    Fixed length calculations.
*/
DB_STATUS
adu_1flt_coerce(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *dv,
DB_DATA_VALUE	    *rdv)
{
    register char       *cp;
    f8			fval;
    u_i2		count;
    char                temp[DB_MAXTUP + 1];
    DB_DATA_VALUE	cdata;
    char		utemp[64] = {'0'};
    DB_STATUS		db_stat = E_DB_OK;
    char		decimal = (adf_scb->adf_decimal.db_decspec ?
				(char) adf_scb->adf_decimal.db_decimal : '.');

    if ((dv->db_datatype == DB_NCHR_TYPE) ||
        (dv->db_datatype == DB_NVCHR_TYPE))
    {
      if (dv->db_datatype == DB_NVCHR_TYPE)
      {
        cdata.db_datatype = DB_VCH_TYPE;
        cdata.db_length = (dv->db_length - DB_CNTSIZE)/sizeof(UCS2) + DB_CNTSIZE;
      }
      else if (dv->db_datatype == DB_NCHR_TYPE)
      {
        cdata.db_datatype = DB_CHA_TYPE;
        cdata.db_length = dv->db_length/sizeof(UCS2);
      }

      cdata.db_data = (PTR) utemp;

      db_stat = adu_nvchr_coerce(adf_scb, dv, &cdata);
      if (db_stat)
        return (db_stat);

      dv = &cdata;			/* Use coerced input now */
    }

    switch (dv->db_datatype)
    {
      case DB_CHA_TYPE:
      case DB_CHR_TYPE:
        if (dv->db_length > DB_MAXTUP)
            return(adu_error(adf_scb, E_AD5004_OVER_MAXTUP, 0));

        cp = temp;
        MEcopy(dv->db_data, dv->db_length, (PTR) cp);
        cp[dv->db_length] = EOS;

        if (CVaf(cp, decimal, &fval) != OK)
	    return(adu_error(adf_scb, E_AD5003_BAD_CVTONUM, 0));

        break;

      case DB_FLT_TYPE:
        if (dv->db_length == 4)
	{
	    f4 tmp;
	    F4ASSIGN_MACRO(*(f4 *) dv->db_data, tmp);
            fval = tmp;
	}
        else
            F8ASSIGN_MACRO(*(f8 *) dv->db_data, fval);
        break;

      case DB_DEC_TYPE:
	CVpkf(	(PTR)dv->db_data,
		    (i4)DB_P_DECODE_MACRO(dv->db_prec),
		    (i4)DB_S_DECODE_MACRO(dv->db_prec),
		&fval);
	break;
      
      case DB_INT_TYPE:
        switch (dv->db_length)
        {
          case 1:
            fval = I1_CHECK_MACRO(*(i1 *) dv->db_data);
            break;

          case 2:
	    {
		i2 tmp;
		I2ASSIGN_MACRO(*(i2 *) dv->db_data, tmp);
		fval = tmp;
	    }
            break;

          case 4:
	    {
		i4 tmp;
		I4ASSIGN_MACRO(*(i4 *) dv->db_data, tmp);
		fval = tmp;
	    }
            break;

          case 8:
	    {
		i8 tmp;
		I8ASSIGN_MACRO(*(i8 *) dv->db_data, tmp);
		fval = tmp;
	    }
            break;

	  default:
	    return(adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0));
        }
        break;

      case DB_VCH_TYPE:
      case DB_TXT_TYPE:
      case DB_LTXT_TYPE:
        I2ASSIGN_MACRO(((DB_TEXT_STRING *) dv->db_data)->db_t_count, count);
        if (count > DB_MAXTUP)
	    return(adu_error(adf_scb, E_AD5004_OVER_MAXTUP, 0));

        cp = temp;
        MEcopy((PTR)((DB_TEXT_STRING *)dv->db_data)->db_t_text, count, (PTR)cp);

        /*! BEGIN BUG - in TEXT the length given by db_length is not the same
	** as the string length.
	*/
        cp[count] = EOS;
        /*! END BUG */

        if (CVaf(cp, decimal, &fval) != OK) 
	    return(adu_error(adf_scb, E_AD5003_BAD_CVTONUM, 0));

        break;

	default:
	    return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    }

    EXmath(EX_ON);
    if (rdv->db_length == 4)
    {
	if (CV_ABS_MACRO(fval) > FLT_MAX)
	    EXsignal (EXFLTOVF, 0);	    
	{
	    f4 tmp = fval;
	    F4ASSIGN_MACRO(tmp, *(f4 *)rdv->db_data);
	}
    }
    else
        F8ASSIGN_MACRO(fval, *(f8 *)rdv->db_data);
    EXmath(EX_OFF);

    return(E_DB_OK);
}


/*{
** Name: adu_1dec_coerce() - Coerce various types to the "decimal" datatype
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
**      dv                              Ptr to DB_DATA_VALUE to be coerced.
**	    .db_datatype		Type of data to be coerced.
**	    .db_prec			Precision/Scale of data to be coerced.
**	    .db_length			Length of data to be coerced.
**	    .db_data			Ptr to actual data to be coerced.
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
**	    E_AD5003_BAD_CVTONUM	The coercion from whatever datatype to
**					the decimal datatype failed.
**	    E_AD5004_OVER_MAXTUP	dv's length was too long.
**	    E_AD9999_INTERNAL_ERROR	Coding error.
**
**  History:
**	15-jul-89 (jrb)
**	    Created.
**      21-apr-2004 (huazh01)
**          instead of using dv->db_length to test the E_AD5004 error,
**          we should use the total number of chars in the string that
**          needs to be converted to decimal to test such error. 
**          This fixes bug 112175, INGSRV2800.
**	11-may-2004 (gupsh01)
**	    Added support for nchar and nvarchar datatypes.
**	10-may-2007 (gupsh01)
**	    Added support for UTF8 charactersets.
**	30-may-2008 (gupsh01)
**	    Remove the previous change as decimal digit are single byte.
**      28-Aug-2007 (kiria01) b121252
**          Allow string data floats to be represented in scientific notation.
**	
*/

DB_STATUS
adu_1dec_coerce(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *dv,
DB_DATA_VALUE	    *rdv)
{
    STATUS	    status;
    register char   *cp;
    i4		    val;
    i8		    val8;
    f8		    fval;
    u_i2	    count;
    char	    temp[DB_MAXTUP + 1];
    char	    decimal = (adf_scb->adf_decimal.db_decspec ?
				(char) adf_scb->adf_decimal.db_decimal : '.');
    bool	    v8;
    DB_DATA_VALUE   cdata;
    char            utemp[64] = {'0'};
    DB_STATUS       db_stat = E_DB_OK;

    switch (dv->db_datatype)
    {
      case DB_INT_TYPE:
	v8 = FALSE;
        switch (dv->db_length)
        {
          case 1:
            val = I1_CHECK_MACRO(*(i1 *)dv->db_data);
            break;

          case 2:
	    {
		i2 tmp;
		I2ASSIGN_MACRO(*(i2 *)dv->db_data, tmp);
		val = tmp;
	    }
            break;

          case 4:
	    {
		i4 tmp;
		I4ASSIGN_MACRO(*(i4 *)dv->db_data, tmp);
		val = tmp;
	    }
            break;

	  case 8:
	    I8ASSIGN_MACRO(*(i8 *)dv->db_data, val8);
	    val = val8;
	    v8 = TRUE;
	    break;

	  default:
	    return(adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0));
        }

	if ((!v8 && CVlpk(val,
		(i4)DB_P_DECODE_MACRO(rdv->db_prec),
		(i4)DB_S_DECODE_MACRO(rdv->db_prec),
		(PTR)rdv->db_data)
	    == CV_OVERFLOW) ||
	   (v8 && CVl8pk(val8,
		(i4)DB_P_DECODE_MACRO(rdv->db_prec),
		(i4)DB_S_DECODE_MACRO(rdv->db_prec),
		(PTR)rdv->db_data)
	    == CV_OVERFLOW))
	{
	    EXsignal(EXDECOVF, 0);
	}
        break;

      case DB_DEC_TYPE:
	if (CVpkpk((PTR)dv->db_data,
			(i4)DB_P_DECODE_MACRO(dv->db_prec),
			(i4)DB_S_DECODE_MACRO(dv->db_prec),
			(i4)DB_P_DECODE_MACRO(rdv->db_prec),
			(i4)DB_S_DECODE_MACRO(rdv->db_prec),
		    (PTR)rdv->db_data)
	    == CV_OVERFLOW)
	{
	    EXsignal(EXDECOVF, 0);
	}
	break;

      case DB_FLT_TYPE:
        if (dv->db_length == 4)
	{
	    f4 tmp;
	    F4ASSIGN_MACRO(*(f4 *)dv->db_data, tmp);
	    fval = tmp;
	}
        else
	    F8ASSIGN_MACRO(*(f8 *)dv->db_data, fval);

	if (CVfpk(fval,
		    (i4)DB_P_DECODE_MACRO(rdv->db_prec),
		    (i4)DB_S_DECODE_MACRO(rdv->db_prec),
		    (PTR)rdv->db_data)
	    == CV_OVERFLOW)
	{
	    EXsignal(EXDECOVF, 0);
	}
        break;
	
      case DB_MNY_TYPE:
	/* convert money from cents to dollars */
	{
	    AD_MONEYNTRNL tmp;
	    tmp = *(AD_MONEYNTRNL *)dv->db_data;
            fval = tmp.mny_cents / 100.0;
	}
	if (CVfpk(fval,
		    (i4)DB_P_DECODE_MACRO(rdv->db_prec),
		    (i4)DB_S_DECODE_MACRO(rdv->db_prec),
		    (PTR)rdv->db_data)
	    == CV_OVERFLOW)
	{
	    EXsignal(EXDECOVF, 0);
	}
        break;

      case DB_NCHR_TYPE:
	cdata.db_datatype = DB_CHA_TYPE;
        cdata.db_length = dv->db_length/sizeof(UCS2);
	cdata.db_data = (PTR) utemp;

	if (db_stat = adu_nvchr_coerce(adf_scb, dv, &cdata))
	    return (db_stat);
	dv = &cdata;	/* Use coerced value now as though we were CHAR */
        /* FALLTHROUGH */

      case DB_CHA_TYPE:
      case DB_CHR_TYPE:
        if (dv->db_length > DB_MAXTUP)
            return(adu_error(adf_scb, E_AD5004_OVER_MAXTUP, 0));

        cp = temp;
        MEcopy(dv->db_data, dv->db_length, (PTR) cp);
        cp[dv->db_length] = EOS;

        status = CVapk(cp,
		    decimal,
		    (i4)DB_P_DECODE_MACRO(rdv->db_prec),
		    (i4)DB_S_DECODE_MACRO(rdv->db_prec),
		    (PTR)rdv->db_data);

	if (status == CV_SYNTAX)
	{
	    /*
	    ** Before giving up with syntax error - see if it was
	    ** structured in scientific notation and if so coerce via
	    ** float8
	    */
	    if (CVaf(cp, decimal, &fval) != OK)
		return(adu_error(adf_scb, E_AD5003_BAD_CVTONUM, 0));
	    status = CVfpk(fval,
			(i4)DB_P_DECODE_MACRO(rdv->db_prec),
			(i4)DB_S_DECODE_MACRO(rdv->db_prec),
			(PTR)rdv->db_data);
	}

	if (status == CV_OVERFLOW)
	    EXsignal(EXDECOVF, 0);
	    
        break;

      case DB_NVCHR_TYPE:
	cdata.db_datatype = DB_VCH_TYPE;
	cdata.db_length = (dv->db_length - DB_CNTSIZE)/sizeof(UCS2) + DB_CNTSIZE;
	cdata.db_data = (PTR) utemp;

	if (db_stat = adu_nvchr_coerce(adf_scb, dv, &cdata))
	    return (db_stat);

	dv = &cdata;	/* Use coerced value now as though we were VARCHAR */
        /* FALLTHROUGH */

      case DB_VCH_TYPE:
      case DB_TXT_TYPE:
      case DB_LTXT_TYPE:
        I2ASSIGN_MACRO(((DB_TEXT_STRING *) dv->db_data)->db_t_count, count);
        if (count > DB_MAXTUP)
            return(adu_error(adf_scb, E_AD5004_OVER_MAXTUP, 0));

        cp = temp;
        MEcopy((PTR)((DB_TEXT_STRING *)dv->db_data)->db_t_text, count, (PTR)cp);
        cp[count] = EOS;

        status = CVapk(cp,
		    decimal,
		    (i4)DB_P_DECODE_MACRO(rdv->db_prec),
		    (i4)DB_S_DECODE_MACRO(rdv->db_prec),
		    (PTR)rdv->db_data);

	if (status == CV_SYNTAX)
	{
	    /*
	    ** Before giving up with syntax error - see if it was
	    ** structured in scientific notation and if so coerce via
	    ** float8
	    */
	    if (CVaf(cp, decimal, &fval) != OK)
		return(adu_error(adf_scb, E_AD5003_BAD_CVTONUM, 0));
	    status = CVfpk(fval,
			(i4)DB_P_DECODE_MACRO(rdv->db_prec),
			(i4)DB_S_DECODE_MACRO(rdv->db_prec),
			(PTR)rdv->db_data);
	}

	if (status == CV_OVERFLOW)
	    EXsignal(EXDECOVF, 0);
	    
        break;

	default:
	    return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    }

    return(E_DB_OK);
}

/*{
** Name: adu_1int_coerce() - Coerce various types to the "i" datatype (integers)
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
**      dv                              Ptr to DB_DATA_VALUE to be coerced.
**	    .db_datatype		Type of data to be coerced.
**	    .db_prec			Precision/Scale of data to be coerced.
**	    .db_length			Length of data to be coerced.
**	    .db_data			Ptr to actual data to be coerced.
**	rdv				Ptr to DB_DATA_VALUE to hold resulting
**					integer.
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
**	    .db_data			Ptr to resulting integer value.
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
**	    E_AD5003_BAD_CVTONUM	The coercion from whatever datatype to
**					an integer datatype failed.
**	    E_AD5004_OVER_MAXTUP	dv's length was too long.
**	    E_AD9999_INTERNAL_ERROR	Coding error.
**
**  History:
**      4/4/83 (kooi) - made temp MAXTUP+1 cause copy may give us
**          something that long (instead of 256). Added error
**          checks for >MAXTUP
**	5/26/84 (lichtman) - added conversion from money to int.
**	    (fix bug #2303).
**	6/20/85 (wong) - used 'I1_CHECK_MACRO()' to hide unsigned char
**          conversion.
**      8/2/85 (peb)    Added parameter to CVaf calls for multinational
**              support.
**	28-apr-86 (ericj)
**	    Convert for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	30-sep-86 (thurston)
**	    Made to work for char, varchar, and longtext.
**	19-mar-87 (thurston)
**	    Specifically checked for integer overflow in adu_1int_coerce() and
**	    signal exception if detected.
**	21-apr-87 (thurston)
**	    Had a "bug" reported where `int4(40000.00 * .5)' was coming out as
**	    `19999', while `int4(40000.00 / 2)' was coming out as `20000'.  Sure
**	    enough, in 5.0, both resulted in `20000'.  This may sound strange,
**	    but I `fixed' the problem by casting my temp f8 into an f4 before
**	    casting it to the final i4 value.  Hey, whatever works.
**	12-jun-87 (thurston)
**	    Since the previous band-aide (casting to f4 before assigning to i4)
**	    only made the system break elsewhere, I have removed that fix.  The
**	    real fix has `presumably' been made to the CVexpten() routine in
**	    CVDECIMAL.MAR.
**	21-jan-88 (bruceb)
**	    Whenever raise_exception is set to TRUE, set val to 0 (for
**	    consistency) rather than to (i4)fval (which causes a floating
**	    point trap on DG.)
**	14-jul-89 (jrb)
**	    Added decimal support.
**	23-dec-03 (inkdo01)
**	    Added support of i8 result from decimal coercion and i8 to
**	    integer coercion.
**	10-Jan-2004 (schka24)
**	    Change above to use i8 typedef, should now be generally available.
**	12-feb-04 (inkdo01)
**	    Forgot to do other integer to i8 (trivial case).
**      21-apr-2004 (huazh01)
**          instead of using dv->db_length to test the E_AD5004 error,
**          we should use the total number of chars in the string that
**          needs to be converted to integer to test such error. 
**          This fixes bug 112175, INGSRV2800.
**	7-may-2004 (gupsh01)
**	    Add support for nchar and nvarchar input datatypes.
**	14-May-2004 (schka24)
**	    char->i8 missing;  simplify by converting all to i8 and
**	    checking for overflow at store time; C type quirk with MINI4
**	    caused spurious overflow indications.
**	    Change pointer, not struct, for unicode (don't touch source dv,
**	    it's not good hygiene.)
**	17-May-2004 (schka24)
**	    Oops, last edit broke int4('0.11') and similar.
**	11-sep-2006 (dougi)
**	    Lengthen utemp to allow Unicode coercions.
**	08-Aug-2007 (gupsh01)
**	    Further lengthen utemp and add the error check
**	    so that longer unicode characters upto max allowed
**	    tuple length are accepted.
**	15-Aug-2007 (gupsh01)
**	    Tweak the length of the temp buffer to avoid errors.
**	29-Aug-2007 (gupsh01)
**	    Limit the length of intermediate coercion data types used 
**	    for each of nchar/nvarchr types to avoid unexpected results.
**	27-May-2010 (kiria01) b123822
**	    Correct sence of length calculation of prior change.
*/

DB_STATUS
adu_1int_coerce(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *dv,
DB_DATA_VALUE	    *rdv)
{
    register char   *cp;
    i4		    val;
    i8		    val8;
    f8		    fval;
    u_i2	    count;
    char	    temp[DB_MAXTUP + 1];
    char	    decimal = (adf_scb->adf_decimal.db_decspec ?
				(char) adf_scb->adf_decimal.db_decimal : '.');
    i4		    raise_exception = FALSE;
    DB_DATA_VALUE   cdata;
    char	    utemp[DB_MAXTUP + 1] = {'0'};
    DB_STATUS	    db_stat = E_DB_OK;

    if ((dv->db_datatype == DB_NCHR_TYPE) || 
        (dv->db_datatype == DB_NVCHR_TYPE))
    {
      if (dv->db_datatype == DB_NCHR_TYPE)
      {
        cdata.db_datatype = DB_CHA_TYPE;
        if (dv->db_length > DB_MAXTUP)
            return(adu_error(adf_scb, E_AD5004_OVER_MAXTUP, 0));
	else
            cdata.db_length = dv->db_length/sizeof(UCS2);
      }
      else if (dv->db_datatype == DB_NVCHR_TYPE)
      {
	u_i2 count;
	I2ASSIGN_MACRO(((DB_NVCHR_STRING *) dv->db_data)->count, count);
        cdata.db_datatype = DB_VCH_TYPE;
        if (count * sizeof(UCS2) + DB_CNTSIZE > DB_MAXTUP)
            return(adu_error(adf_scb, E_AD5004_OVER_MAXTUP, 0));
	else
            cdata.db_length = (dv->db_length - DB_CNTSIZE)/sizeof(UCS2) 
				+ DB_CNTSIZE;
      }

      cdata.db_data = (PTR) utemp;

      db_stat = adu_nvchr_coerce(adf_scb, dv, &cdata);
      if (db_stat)
	return (db_stat);

      dv = &cdata;			/* Use our coerced value now */
    }

    switch (dv->db_datatype)
    {
      case DB_CHA_TYPE:
      case DB_CHR_TYPE:
        if (dv->db_length > DB_MAXTUP)
            return(adu_error(adf_scb, E_AD5004_OVER_MAXTUP, 0));

        cp = temp;
        MEcopy(dv->db_data, dv->db_length, (PTR) cp);
        cp[dv->db_length] = EOS;

        if (CVal8(cp, &val8) != OK)
        {
            if (CVaf(cp, decimal, &fval) != OK)
		return(adu_error(adf_scb, E_AD5003_BAD_CVTONUM, 0));
	    if (fval >= MINI8 && fval <= MAXI8)
		val8 = (i8) fval;
	    else
	    {
		raise_exception = TRUE;
		val8 = (i8) 0;
	    }
        }
        break;

      case DB_INT_TYPE:
        switch (dv->db_length)
        {
          case 1:
            val8 = I1_CHECK_MACRO(*(i1 *)dv->db_data);
            break;

          case 2:
	    {
		i2 tmp;
		I2ASSIGN_MACRO(*(i2 *)dv->db_data, tmp);
		val8 = tmp;
	    }
            break;

          case 4:
	    {
		i4 tmp;
		I4ASSIGN_MACRO(*(i4 *)dv->db_data, tmp);
		val8 = tmp;
	    }
            break;

          case 8:
	    {
		i8 tmp;
		I8ASSIGN_MACRO(*(i8 *)dv->db_data, tmp);
		val8 = tmp;
	    }
            break;

	  default:
	    return(adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0));
        }
        break;

      case DB_DEC_TYPE:
	if (rdv->db_length == 8)
	{
	    if (CVpkl8((PTR)dv->db_data,
		    (i4)DB_P_DECODE_MACRO(dv->db_prec),
		    (i4)DB_S_DECODE_MACRO(dv->db_prec),
		&val8) == CV_OVERFLOW)
	    {
		raise_exception = TRUE;
	    }
	}
	else
	{
	    if (CVpkl((PTR)dv->db_data,
		    (i4)DB_P_DECODE_MACRO(dv->db_prec),
		    (i4)DB_S_DECODE_MACRO(dv->db_prec),
		&val) == CV_OVERFLOW)
	    {
		raise_exception = TRUE;
	    }
	    val8 = (i8) val;
	}
	break;
	
      case DB_FLT_TYPE:
        if (dv->db_length == 4)
	{
	    f4 tmp;
	    F4ASSIGN_MACRO(*(f4 *)dv->db_data, tmp);
	    fval = tmp;
	    val8 = tmp;
	}
        else
	{
	    F8ASSIGN_MACRO(*(f8 *)dv->db_data, fval);
	    val8 = fval;
	}
	if (fval < MINI8 || fval > MAXI8)
	    raise_exception = TRUE;
        break;

      case DB_MNY_TYPE:
	/* convert money from cents to dollars */
	{
	    AD_MONEYNTRNL tmp = *(AD_MONEYNTRNL *) dv->db_data;
            val8 = tmp.mny_cents / 100.0;
	}
        break;

      case DB_VCH_TYPE:
      case DB_TXT_TYPE:
      case DB_LTXT_TYPE:
	/* Convert string-y things to i8 if we can, f8 if that fails */
        I2ASSIGN_MACRO(((DB_TEXT_STRING *) dv->db_data)->db_t_count, count);
        if (count > DB_MAXTUP)
            return(adu_error(adf_scb, E_AD5004_OVER_MAXTUP, 0));

        cp = temp;
        MEcopy((PTR)((DB_TEXT_STRING *)dv->db_data)->db_t_text, count, (PTR)cp);
        /*! BEGIN BUG - in TEXT the length given by db_length is not the same
	**  as the string length
	*/
        cp[count] = EOS;
        /*! END BUG */

        if (CVal8(cp, &val8) != OK)
        {
	    /* Might be something like 0.11 which cval says is syntax error */
            if (CVaf(cp, decimal, &fval) != OK)
		return(adu_error(adf_scb, E_AD5003_BAD_CVTONUM, 0));
	    if (fval >= MINI8 && fval <= MAXI8)
		val8 = (i8) fval;
	    else
	    {
		raise_exception = TRUE;
		val8 = (i8) 0;
	    }
        }
        break;

	default:
	    return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    }

    switch (rdv->db_length)
    {
      case 1:
	if (raise_exception  || val8 >MAXI1 || val8 < MINI1)
	    EXsignal(EXINTOVF, (i4) 0);
        *(i1 *)rdv->db_data = val8;
        break;

      case 2:
	{
	    i2 tmp;
	    if (raise_exception  || val8 >MAXI2 || val8 < MINI2)
		EXsignal(EXINTOVF, (i4) 0);
	    tmp = val8;
	    I2ASSIGN_MACRO(tmp, *(i2 *)rdv->db_data);
        }
        break;

      case 4:
	{
	    i4 tmp;
	    if (raise_exception  || val8 >MAXI4 || val8 < MINI4LL)
		EXsignal(EXINTOVF, (i4) 0);
	    tmp = val8;
	    I4ASSIGN_MACRO(tmp, *(i4 *)rdv->db_data);
        }
        break;

      case 8:
	{
	    i8 tmp;
	    if (raise_exception)
		EXsignal(EXINTOVF, (i4) 0);
	    tmp = val8;
	    I8ASSIGN_MACRO(tmp, *(i8 *)rdv->db_data);
        }
	break;

      default:
	return(adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0));
    }


    return(E_DB_OK);
}
