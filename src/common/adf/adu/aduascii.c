/*
** Copyright (c) 1985, 2009 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cv.h>
#include    <me.h>
#include    <st.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduint.h>
#include    <adp.h>
#include    <clfloat.h>
/**
**
**  Name: ADUASCII.C - Converts various datatypes to a character string.
**
**  Description:
**        This file contains routines that convert various Ingres datatypes
**	to character strings.
**
**	This file defines:
**          adu_ascii() - Convert an Ingres datatype to an Ingres character
**			  string.
**          adu_ascii_2arg() - Support the two argument version of string
**                             functions.  Simply call adu_ascii() to do
**                             the real work.
**	    adu_copascii() - Convert an Ingres numeric datatype to an Ingres
**			     character string and right justifies.
**
**  History:
**	13-jul-85 (roger)
**	    Use I1_CHECK_MACRO before passing an i1 to CVla.
**      02-jun-86 (ericj)    
**          Converted for Jupiter.  Merged adu_copascii() from its own file into
**	    this file.  Replaced definition of static adu_size() and referenced
**	    adu_size() in strutil.c instead.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	26-sep-86 (thurston)
**	    Made all routines work for char, varchar, and longtext.
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	01-jul-87 (thurston)
**	    Added code in adu_copascii() to check output width of CVfa() calls
**	    against the available width of the destination string.  Also added
**	    the E_AD5002_BAD_NUMBER_TYPE return to that routine.
**      31-dec-1992 (stevet)
**          Added function prototypes and two argument version of explicit
**          string coercion functions.
**      13-Apr-1993 (fred)
**          Added byte string datatype support.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      10-jun-1993 (stevet)
**          Modified adu_ascii_2arg() to call adu_6strleft() so that
**          we can avoid calling adu_movestring, which I changed to
**          look for string overflow conditions.
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	03-apr-2001 (gupsh01)
**	    Added support for new datatype long nvarchar.
**      25-jan-2007 (stial01)
**          Added support for varchar(locators)
**	19-feb-2009 (joea)
**	    Remove #ifdefs from 5-mar-2008 fix to adu_copascii so that it's
**	    visible on all platforms.
**      31-aug-2009 (joea)
**          Add case for DB_BOO_TYPE in adu_ascii.
**/


/*
** Name: adu_ascii() - Convert a data value to character string data value.
**
** Description:
**	  Converts the data value dv1 into a character string data value, rdv.
**	dv1 can be any of the following datatypes: c, char, varchar, text, f,
**	i, decimal, or longtext.  rdv may be either c, char, varchar, text, or
**	longtext.  Formats to be used are determined by adf_outarg definition
**	found in the ADF control block.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**          .adf_decimal
**		.db_decspec		TRUE if decimal character is specified
**					in .db_decimal.  If FALSE, '.' will be
**					used as the decimal character.
**              .db_decimal             If .db_decspec is TRUE, then this is
**					the decimal character to use in the
**					formatted output string.
**					    ***                            ***
**					    *** NOTE THAT THIS IS A "nat", ***
**					    *** NOT A "char".              ***
**					    ***                            ***
**	    .adf_outarg			ADF_OUTARG struct describing the format
**					and precision of string to be produced.
**		.ad_f4width		The output width for a f4 represented as
**					a character string.
**		.ad_f8width		The output width for a f8 represented as
**					a character string.
**		.ad_f4prec		The number of digits to display after
**					the decimal point.
**		.ad_f8prec		The number of digits to display after
**					the decimal point.
**		.ad_f4style		f4 output style.
**		.ad_f8style		f8 output style.
**      dv1                             DB_DATA_VALUE describing Ingres datatype
**					to be converted.
**	    .db_datatype		Its type.
**	    .db_prec			Its precision/scale.
**	    .db_length			Its length.
**	    .db_data			Ptr to the data to be converted into
**					a character string.
**	rdv				DB_DATA_VALUE describing character
**					string result.
**	    .db_length			The maximum length of the result.
**	    .db_data			Ptr to buffer to hold converted string.
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
**	    .db_data			The converted string.
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
**	    E_AD5010_BAD_STRING_TYPE	The destination DB_DATA_VALUE was not
**					the correct type.
**	    E_AD0100_NOPRINT_CHAR	One or more non-printing characters
**					were transformed to blanks.
**
**  History:
**      02/22/83 (lichtman) -- made to work for both c type and
**                  new text type
**      03/03/83 (lichtman) -- changed to pass length of string to
**                  movestring instead of expected length
**                  of the number, since the number is not
**                  necessarily as long as its Out_arg
**                  width.
**      8/2/85 (peb)        Added parameter to CVfa calls for
**                  multinational support.
**	02-jun-86 (ericj)
**	    Converter for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	26-sep-86 (thurston)
**	    Made work for char, varchar, and longtext.
**	14-jul-89 (jrb)
**	    Added decimal support.
**      11-jan-1993 (stevet)
**          Added support for abstract types to support DB_ALL_TYPE.  Also
**          change return error code to be more descriptive.
**      13-Apr-1993 (fred)
**          Added byte string datatype support.
**      03-apr-2001 (gupsh01)
**          Added support for new datatype long nvarchar.
**      13-may-2004 (gupsh01)
**	    Added support for new datatype int8.
**      19-jun-2006 (gupsh01)
**	    Added support for new date/time datatypes.
**	14-oct-2007 (gupsh01)
**	    Pass in the type of source string to the adu_movestring 
**	    routine.
**	17-Mar-2009 (kiria01) SIR121788
**	    Add missing LNLOC support and let adu_lvch_move
**	    couponify the locators.
*/

DB_STATUS
adu_ascii(
ADF_CB			*adf_scb,
register DB_DATA_VALUE	*dv1,
DB_DATA_VALUE		*rdv)
{
    DB_STATUS           db_stat = E_DB_OK;
    register char       *p;
    char		temp[ADI_OUTMXFIELD]; /* could probably be smaller */
    u_char		*str_addr;
    i4			str_len;
    i2			reswidth;
    bool		char_text = FALSE;
    i8			i8_tmp = 0;

    p = temp;

    switch(dv1->db_datatype)
    {
      case DB_INT_TYPE:
        if (dv1->db_length == 8)
	{
            CVla8(*(i8 *) dv1->db_data, p);
	}
        else if (dv1->db_length == 4)
        {
            CVla(*(i4 *) dv1->db_data, p);
        }
        else if (dv1->db_length == 2)
        {
            CVla((i4) (*(i2 *) dv1->db_data), p);
        }
        else
        {
            CVla(I1_CHECK_MACRO(*(i1 *) dv1->db_data), p);
        }

        break;

      case DB_BOO_TYPE:
        if (((DB_ANYTYPE *)dv1->db_data)->db_booltype == DB_FALSE)
            STcopy("FALSE", p);
        else
            STcopy("TRUE", p);
        break;

      case DB_VCH_TYPE:
      case DB_CHA_TYPE:
      case DB_CHR_TYPE:
      case DB_TXT_TYPE:
      case DB_LTXT_TYPE:
      case DB_BYTE_TYPE:
      case DB_VBYTE_TYPE:
	if ((db_stat = adu_3straddr(adf_scb, dv1, (char **) &str_addr)))
	    return(db_stat);
	if ((db_stat = adu_size(adf_scb, dv1, &str_len)))
	    return(db_stat);
        if ((db_stat = adu_movestring(adf_scb, str_addr, str_len, 
					dv1->db_datatype, rdv)))
	    return(db_stat);
        char_text = TRUE;

        break;

      case DB_DEC_TYPE:
	{
	    i4		pr = DB_P_DECODE_MACRO(dv1->db_prec);
	    i4		sc = DB_S_DECODE_MACRO(dv1->db_prec);
	    char	decimal = (adf_scb->adf_decimal.db_decspec
					? (char) adf_scb->adf_decimal.db_decimal
					: '.'
				  );
				  
	    /* now convert to ascii: use formula from lenspec for length, get
	    ** scale # of digits after decimal point, use left-justify option
	    */
	    if (CVpka((PTR)dv1->db_data, pr, sc, decimal,
		    AD_PS_TO_PRN_MACRO(pr, sc), sc, CV_PKLEFTJUST, p, &str_len)
		== CV_OVERFLOW)
	    {
		/* this should never happen */
		return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	    }
	}
        break;

      case DB_FLT_TYPE:
	{
	    char    decimal = (adf_scb->adf_decimal.db_decspec ?
				(char) adf_scb->adf_decimal.db_decimal : '.');
	    
	    if (dv1->db_length == 4)
	    {
		CVfa((f8) *(f4 *) dv1->db_data,
		     adf_scb->adf_outarg.ad_f4width,
		     adf_scb->adf_outarg.ad_f4prec,
		     adf_scb->adf_outarg.ad_f4style,
		     decimal, p, &reswidth);
	    }
	    else
	    {
		CVfa(*(f8 *)dv1->db_data,
		     adf_scb->adf_outarg.ad_f8width,
		     adf_scb->adf_outarg.ad_f8prec,
		     adf_scb->adf_outarg.ad_f8style,
		     decimal, p, &reswidth);
	    }
	}
        break;

      case DB_DTE_TYPE:
      case DB_ADTE_TYPE:
      case DB_TMWO_TYPE:
      case DB_TMW_TYPE:
      case DB_TME_TYPE:
      case DB_TSWO_TYPE:
      case DB_TSW_TYPE:
      case DB_TSTMP_TYPE:
      case DB_INYM_TYPE:
      case DB_INDS_TYPE:
	return( adu_6datetostr( adf_scb, dv1, rdv));
	       
      case DB_MNY_TYPE:
	return( adu_9mnytostr( adf_scb, dv1, rdv));

      case DB_LVCH_TYPE:
      case DB_LBYTE_TYPE:
      case DB_LNVCHR_TYPE:
      case DB_LCLOC_TYPE:
      case DB_LBLOC_TYPE:
      case DB_LNLOC_TYPE:
	return( adu_lvch_move( adf_scb, dv1, rdv));
	
      case DB_BIT_TYPE:
      case DB_VBIT_TYPE:
	return( adu_bit2str( adf_scb, dv1, rdv));
	
      case DB_LOGKEY_TYPE:
      case DB_TABKEY_TYPE:
	return( adu_3logkeytostr( adf_scb, dv1, rdv));

      default:
	return(adu_error(adf_scb, E_AD2090_BAD_DT_FOR_STRFUNC, 0));
    }

    if (!char_text)
    {
        if ((db_stat = adu_movestring(adf_scb, (u_char *) p,
				      (i4) STlength(p), 
				      dv1->db_datatype, rdv))
	    != E_DB_OK
	   )
	    return(db_stat);
    }

    return(E_DB_OK);
}


/*
** Name: adu_ascii_2arg() - Convert a data value to character string data value.
**
** Description:
**      Support the two argument version of string functions CHAR(), VARCHAR(),
**      TEXT() and C().  Simply call adu_ascii() to do the real work.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**      dv1                             DB_DATA_VALUE describing Ingres datatype
**					to be converted.
**	    .db_datatype		Its type.
**	    .db_prec			Its precision/scale.
**	    .db_length			Its length.
**	    .db_data			Ptr to the data to be converted into
**					a character string.
**	rdv				DB_DATA_VALUE describing character
**					string result.
**	    .db_length			The maximum length of the result.
**	    .db_data			Ptr to buffer to hold converted string.
**
** Outputs:
**	adf_scb				Pointer to an ADF session control block.
**	rdv
**	    .db_data			The converted string.
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
**	    E_AD0000_OK			Routine completed successfully.
**	    E_AD5001_BAD_STRING_TYPE	Either dv1's or rdv's datatype was
**					inappropriate for this operation.
**
**  History:
**     31-dec-1992 (stevet)
**         Created.
**     10-jun-1993 (stevet)
**         Modified to disable string truncation detection since this function
**         is behaved like the left() function.
*/
DB_STATUS
adu_ascii_2arg(
ADF_CB			*adf_scb,
DB_DATA_VALUE	        *dv1,
DB_DATA_VALUE	        *dv2,
DB_DATA_VALUE		*rdv)
{
    ADF_STRTRUNC_OPT  input_strtrunc=adf_scb->adf_strtrunc_opt;
    STATUS            status;

    adf_scb->adf_strtrunc_opt = ADF_IGN_STRTRUNC;
    if (dv1->db_datatype == DB_NCHR_TYPE || dv1->db_datatype == DB_NVCHR_TYPE)
	status = adu_nvchr_coerce(adf_scb, dv1, rdv);
    else
	status = adu_ascii(adf_scb, dv1, rdv);
    adf_scb->adf_strtrunc_opt = input_strtrunc;
    return( status);
}

/*
** Name: adu_copascii() - Convert number to character string
**
**  Converts the numeric data value dv1 into a character string data value, rdv.
**  Formats to be used are determined by adf_scb->adf_outarg.
**  This is a modified version of adu_ascii(), and is used only for copying a
**  character domain into a file.  The difference is that it right justifies
**  instead of left justifying, and will report an error if the print format in
**  use will not fit into the field width supplied.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**          .adf_decimal
**		.db_decspec		TRUE if decimal character is specified
**					in .db_decimal.  If FALSE, '.' will be
**					used as the decimal character.
**              .db_decimal             If .db_decspec is TRUE, then this is
**					the decimal character to use in the
**					formatted output string.
**					    ***                            ***
**					    *** NOTE THAT THIS IS A "nat", ***
**					    *** NOT A "char".              ***
**					    ***                            ***
**	    .adf_outarg			ADF_OUTARG struct describing the format
**					and precision of string to be produced.
**		.ad_f4width		The output width for a f4 represented as
**					a character string.
**		.ad_f8width		The output width for a f8 represented as
**					a character string.
**		.ad_f4prec		The number of digits to display after
**					the decimal point.
**		.ad_f8prec		The number of digits to display after
**					the decimal point.
**		.ad_f4style		f4 output style.
**		.ad_f8style		f8 output style.
**      dv1                             DB_DATA_VALUE describing Ingres datatype
**					to be converted.
**	    .db_datatype		Its type.
**	    .db_prec			Its precision/scale.
**	    .db_length			Its length.
**	    .db_data			Ptr to the data to be converted into
**					a character string.
**	rdv				DB_DATA_VALUE describing character
**					string result.
**	    .db_length			The maximum length of the result.
**	    .db_data			Ptr to buffer to hold converted string.
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
**	    .db_data			The converted string.
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
**	    E_AD0000_OK			Completed successfully.
**	    E_AD1030_F_COPY_STR_TOOSHORT
**					The destination string was not big
**					enough to hold the formatted floating
**					point number.
**	    E_AD5001_BAD_STRING_TYPE	The destination DB_DATA_VALUE was not
**					a string type.
**	    E_AD5002_BAD_NUMBER_TYPE	The input DB_DATA_VALUE was not
**					a numeric type.
**	    E_AD0100_NOPRINT_CHAR	One or more non-printing characters
**					were transformed to blanks.
**
**  History:
**      8/2/85 (peb)    Added parameter to CVfa calls for multinational.
**	02-jun-86 (ericj)
**	    Converted for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	26-sep-86 (thurston)
**	    Made work for char, varchar, and longtext.
**	01-jul-87 (thurston)
**	    Added code to check output width of CVfa() calls against the
**	    available width of the destination string.  Also added the
**	    E_AD5002_BAD_NUMBER_TYPE return.
**	19-sep-89 (jrb)
**	    Added decimal support.
**      07-aub-1990 (jonb)
**          Changed &&'s in an if statement to ||'s.
**	28-Feb-2005 (schka24)
**	    Yet another missed i8 conversion.
**      05-Mar-2008 (coomi01) : b119968
**          Check f4 case for boundary condition at +/- FLT_MAX
**          here we must make a slight adjustment to ensure rounding
**          does not cause overflow.
**	14-oct-2007 (gupsh01)
**	    Pass in the type of source string to the adu_movestring 
**	    routine.
*/

DB_STATUS
adu_copascii(
ADF_CB		*adf_scb,
DB_DATA_VALUE	*dv1,
DB_DATA_VALUE	*rdv)
{
    register char       *p;
    char		temp[ADI_OUTMXFIELD];
    i4			leftfill;
    i4			plen;
    i4			str_len;
    i2			out_width;

    p = temp;

    switch(dv1->db_datatype)
    {
      case DB_INT_TYPE:
        if (dv1->db_length == 8)
        {
            CVla8(*(i8 *) dv1->db_data, p);
        }
        else if (dv1->db_length == 4)
        {
            CVla(*(i4 *) dv1->db_data, p);
        }
        else if (dv1->db_length == 2)
        {
            CVla((i4) *(i2 *) dv1->db_data, p);
        }
        else
        {
            CVla(I1_CHECK_MACRO(*(i1 *) dv1->db_data), p);
        }

        break;

      case DB_DEC_TYPE:
	{
	    i4		pr = DB_P_DECODE_MACRO(dv1->db_prec);
	    i4		sc = DB_S_DECODE_MACRO(dv1->db_prec);
	    i4		available_width = rdv->db_length;
	    char	decimal = (adf_scb->adf_decimal.db_decspec
					? (char) adf_scb->adf_decimal.db_decimal
					: '.'
				  );
				  
	    if (    rdv->db_datatype == DB_VCH_TYPE
		||  rdv->db_datatype == DB_TXT_TYPE
		||  rdv->db_datatype == DB_LTXT_TYPE
	       )
		available_width -= DB_CNTSIZE;

	    /* now convert to ascii: use formula from lenspec for length, get
	    ** scale # of digits after decimal point, use left-justify
	    */
	    if (CVpka((PTR)dv1->db_data, pr, sc, decimal,
		    AD_PS_TO_PRN_MACRO(pr, sc), sc, CV_PKLEFTJUST, p, &str_len)
		== CV_OVERFLOW)
	    {
		/* this should never happen */
		return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	    }

	    if (str_len > available_width)
		return(adu_error(adf_scb, E_AD1030_F_COPY_STR_TOOSHORT, 0));
	}
        break;

      case DB_FLT_TYPE:
	{
	    char    decimal = (adf_scb->adf_decimal.db_decspec ?
				(char) adf_scb->adf_decimal.db_decimal : '.');
	    i4	    available_width = rdv->db_length;


	    if (    rdv->db_datatype == DB_VCH_TYPE
		||  rdv->db_datatype == DB_TXT_TYPE
		||  rdv->db_datatype == DB_LTXT_TYPE
	       )
		available_width -= DB_CNTSIZE;

	    if (dv1->db_length == 4)
	    {
		f8 f8tmp = (f8) *(f4 *) dv1->db_data;

                /*  BUG : b119968
		**
		**  CVfa is a utility routine for both f8 and f4 floats.
		**  In the f4 case, the value is converted to f8 prior 
		**  to call (as here).
		**
		**  This works well except for the boundary case, where 
		**  the f4 is FLT_MAX. In this case, an effective 
		**  rounding up of the f8 that occurs as we convert to 
		**  a string (eventually through fcvt) "overflows" the 
		**  original f4 value.
		**
		**  We bump down the last sig bit of the f4, and then
		**  we have no problems.
		*/
		if ( f8tmp == FLT_MAX )
		{
		    f8tmp = FLT_MAX_ROUND;
		} 
                else if ( f8tmp == -FLT_MAX ) 
		{
		    f8tmp = -FLT_MAX_ROUND;
		} 
		CVfa(f8tmp,
		     adf_scb->adf_outarg.ad_f4width,
		     adf_scb->adf_outarg.ad_f4prec,
		     adf_scb->adf_outarg.ad_f4style,
		     decimal, p, &out_width);

		if (out_width > available_width)
		    return(adu_error(adf_scb, E_AD1030_F_COPY_STR_TOOSHORT, 0));
	    }
	    else
	    {
		CVfa(*(f8 *)dv1->db_data,
		     adf_scb->adf_outarg.ad_f8width,
		     adf_scb->adf_outarg.ad_f8prec,
		     adf_scb->adf_outarg.ad_f8style,
		     decimal, p, &out_width);

		if (out_width > available_width)
		    return(adu_error(adf_scb, E_AD1030_F_COPY_STR_TOOSHORT, 0));
	    }
	}
	break;

      default:
	return(adu_error(adf_scb, E_AD5002_BAD_NUMBER_TYPE, 0));
    }

    while (*p == ' ')
        p++;

    /*
    ** TEXT, VARCHAR, and LONGTEXT are variable length copies,
    ** so no need to pad on the left.
    */
    if (    rdv->db_datatype == DB_TXT_TYPE
	||  rdv->db_datatype == DB_VCH_TYPE
	||  rdv->db_datatype == DB_LTXT_TYPE
       )	
    {
        return(adu_movestring(adf_scb, (u_char *) p, 
				(i4) STlength(p), dv1->db_datatype, rdv));
    }

    /*
    ** At this point the result type can be C or CHAR
    */
    plen     = min(STlength(p), rdv->db_length);
    leftfill = rdv->db_length - plen;

    MEfill(leftfill, ' ', rdv->db_data);
    MEcopy((PTR)p, plen, (PTR)((char *)rdv->db_data + leftfill));

    return(E_DB_OK);
}
