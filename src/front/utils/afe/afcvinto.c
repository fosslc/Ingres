/*
**	Copyright (c) 2004, 2009 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<er.h>		 
#include	<st.h>		 
#include	<me.h>
#include	<cm.h>
#include	<gl.h>
#include	<sl.h>
#include	<iicommon.h>
#include	<fe.h>
#include	<adf.h>
#include	<afe.h>
#include	<ex.h>
#include	<adudate.h>


/**
** Name:	afcvinto.c -	Fast Conversion for ADF data types Module.
**
** Description:
**	Contains routines used for fast conversions between ADF types common
**	to the front-ends.  
**
** Defines:
**	afe_cvinto()	- fast one-to-one mapping of DB data values.
**	iiafCvtNum()	- internal routine for simple numeric conversions.
**
** Notes:
**	The routines in this file should be incorporated into ADF at some point.
**
** History:
**	Revision 6.0  87/12/01  dave
**	Initial revision.
**
**	Revision 6.2  88/10/31  neil
**	31-oct-1988	- Rewritten for performance (ncg)
**	11/15/89 (dkh) - Fixed afe_cvinto() so that variable length
**			 string conversions don't blank trim.
**
**	Revision 6.3
**	19-jul-1990 (barbara)
**	    Allow DECIMAL type on input data value in afe_cvinto().
**	13-feb-1991 (Joe)
**	     Added checks for internal ADF errors during conversion.
**	     If they are found a FE error is generated.
**
**	Revision 6.4
**	17-aug-91 (leighb) DeskTop Porting Change
**		added er.h & st.h
**  91/12/09  wong
**		Removed `money' handling in 'afe_cvinto()' to default to 'adc_cvinto()'
**		semantics.  Bug #41162.
**	14-jan-92 (seg)
**	    Can't do arithmetic on PTR variables.
**	06/24/92 (dkh) - Added change to make integer to decimal conversion
**			 work.  Previous behavior just dropped out of the
**			 bottom of the switch statement.
**	07/29/92 (dkh) - Add exception handler to gracefully deal with
**			 decimal conversion exceptions (overflow).
**      08/31/92 (dkh) - Roll back part of fix for bug 41162.  We
**                       still want to handle money to int here
**                       since user code is depending on it.
**	10/18/93 (teresal) - Expand exception handler to handle integer
**			 and float overflows. Also fix handler so it
**			 returns the status and error number correctly.
**			 This fix is needed to pass a FIPS 127-2 test. 
**			 Bug #55940.
**      20-nov-1994 (andyw)
**          Added NO_OPTIM due to Solaris 2.4/Compiler 3.00 errors
**      1-june-1995 (hanch04)
**          Removed NO_OPTIM su4_us5, installed sun patch
**	14-dec-1995 (morayf)
**	    Deleted num.i4type from duplicate assignment to I1_CHECK_MACRO.
**	    This was pointless and, in the case of SNI port, was rejected
**	    by compiler.
**	17-feb-1997 (pchang)
**	    In iiafCvtNum(), allowed numeric conversion of 'integer' to 'long'
**	    for axp_osf.  This can be extended in future to cover other machines
**	    with 64-bit architecture where 'long' is 8 bytes in length. (B76023)
**	10-may-1999 (walro03)
**	    Remove obsolete version string apu_us5.
**      22-Nov-1999 (hweho01)
**          Extended the change for axp_osf to ris_u64 (AIX 64-bit port).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	09-mar-2001 (abbjo03)
**	    Temporary fix for converting 4 byte wchar_t's to 2 bytes for
**	    platforms like Solaris.
**	15-mar-2001 (abbjo03)
**	    Additional changes for NVCHR.
**	24-apr-2003 (somsa01)
**	    Use i8 instead of long for LP64 case.
**	1-july-2004 (gupsh01)
**	    Fixed inserting wchar_t values to nchar/nvarchar columns. If
**	    the lengths were specified to afe_cvinto(), on a platform where
**	    sizeof(wchar_t) is 4, the input data was not being converted due
**	    to being of same length. (bug 112592).
**	24-Aug-2009 (kschendel/stephenb) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**      29-oct-2009 (joea)
**          Add case for DB_BOO_TYPE in afe_cvinto.
**      18-nov-2009 (joea)
**          Correct DB_BOO_TYPE case in afe_cvinto to handle conversion to
**          DB_BOO_TYPE itself and reject non-DB_{INT|BOO}_TYPE's.
**/


DB_STATUS	iiafCvtNum();

/*
** Static info needed for the exception handler IIAFadfxhdlr().
**
** The handler sets a status, which must be passed back through 
** afe_cvinto(), to indicate if an error or warning has occurred. 
**
** The handler eventually causes the adf error number to be set in
** the adf control block so the current adf cb must be available 
** for the handler routine to pass down to ADF.
*/
static DB_STATUS xhdlr_stat = E_DB_OK; /* Status set by handler */
static ADF_CB	*xhdlr_adfcb;	       /* adf cb used by handler */

static VOID	convert_err();

/*{
** Name:	IIAFadfxhdlr - AFE conversion excpetion handler for decimal
**
** Description:
**	This excpetion handler is used to capture a decimal overflow
**	exception so that clients of afe_cvinto() don't need to change.
**
** Inputs:
**	ex	An exception that has occurred.
**
** Outputs:
**
**	Returns:
**		EXDECLARE	If routine is passed EXDECOVF (decimal overflow)
**		EXRESIGNAL	If an exception other than EXDECOVF is passed
**
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	07/29/92 (dkh) - Initial version.
*/
EX
IIAFadfxhdlr(ex)
EX_ARGS	*ex;
{
	switch (ex->exarg_num)
	{
	  case EXINTOVF:
	  case EXFLTOVF:
	  case EXFLTUND:
	  case EXDECOVF:
	    xhdlr_stat = adx_handler(xhdlr_adfcb, ex);
	    return(EXDECLARE);
	  default:
	    xhdlr_stat = E_DB_OK;
	    return(EXRESIGNAL);
	}
}


/*{
**  Name:	afe_cvinto - Fast 1-1 convertion routine for DB data values.
**
**  Description:
**	This routine is used instead of adc_cvinto in order to provide
**	faster conversions.  All the conversion rules follow those of
**	adc_cvinto with the following exceptions:
**
**	1. Null data is copied to nullable data without length validations
**	   and with knowledge of ADF_NVL_BIT placement.
**	2. Exact-type-match data is copied without data type or length checks.
**	3. String copying is done without max length checks.
**	4. String copying is done without non-printable-char validation in
**	   CHR and TXT.  This is important to maintain as tab characters
**	   entered via user variables are expanded and aligned in a form
**	   field, rather than being replaced with a single blank as via ADF.
**	5. There is no support for conversions of non-strings to/from LTXT.
**
**	Special ADF internal knowledge:
**
**	1. The Nullable data lengths are decremented by one for copying.
**	2. Internal date conversion routines are named.
**
**	The routine first checks for nullability conditions (these are the
**	fastest as no copying is required).  Next, if the input and output
**	data types, lengths and precisions are an exact match then the data
**	is copied without any checks.  If they are not an exact match then
**	based on the input data type the data is converted.
**
**  Inputs:
**	acb			Pointer to the current ADF_CB control block.
**	idbv			Input DB_DATA_VALUE with source data:
**	   .db_datatype		Data type.
**	   .db_length		Data length.
**	   .db_prec		Precision (unused right now).
**	   .db_data		Input value.
**	odbv			Output DB_DATA_VALUE with description:
**	   .db_datatype		Data type.
**	   .db_length		Data length.
**	   .db_prec		Precision (unused right now).
**
**  Outputs:
**	acb.adf_errcb		ADF error data (if errroneous conversion):
**	   .ad_errcode		Error code if failure.  See numbers below.
**	   .ad_emsglen		If an error occured this is set to zero as
**				no message is looked up.
**	odbv			Output DB_DATA_VALUE with result buffer:
**	   .db_data		To be filled with converted/copied data.
**
** Returns:
**	{DB_STATUS}    	E_DB_OK		If succeeded.
**	    		E_DB_ERROR	If error occurred in the conversion.
**	   	 	E_DB_WARN	If warning occurred in the conversion.
** Errors:
**	E_AD0121_INTOVF_WARN - Numeric overflow from iiafCvtNum.
**	E_AD1012_NULL_TO_NONNULL - NULL data to non-nullable output.
**	E_AD2004_BAD_DTID - Invalid data type id.
**	E_AD2005_BAD_DTLEN - Invalid data type length.
**	E_AD2009_NOCOERCION - No type compatibility.
**	ADF errors reported from internal date conversion routines.
**
**  History:
**	12/01/87 (dkh)	- Initial version for jupiter.
**	31-oct-1988	- Written (ncg)
**	21-Apr-89 (GordonW)  replaced 'MEcopy()' with a MECOPY macro call.
**	18-Jul-1989 (bryanp) - Added MECOPY macro for "money" conversion
**				alignment.
**	08/01/89 (jhw) -- Trim `char' and `c' types before copying into
**				any of the `text' or `varchar' types.
**	19-jul-1990 (barbara)
**	    Allow DECIMAL type on input data values. If type and prec fields
**	    match do direct copy, otherwise call adc_cvinto().
**	29-aug-90 (bruceb)
**		If odbv's original value was NULL, and the idbv data didn't
**		properly convert, than restore odbv to NULL before returning
**		from afe_cvinto().
**	04-sep-90 (bruceb)
**		Moved ADF_TRIMBLANKS_MACRO into afe.h and renamed
**		AFE_TRIMBLANKS.
**	09/90 (jhw) - Modified to use 'AFE_SETNULL()' and 'AFE_CLRNULL()'.
**	09/27/90 (esd) Fixed check of result of afe_tycoerce -
**		it returns bool, not STATUS.
**	13-jan-1991 (Joe)
**	     Added check for internal ADF errors.  If found generates
**	     an AFE error.
**	12/09/1991 (jhw) - Removed `money' special cases; must rely on ADF
**		semantics in 'adc_cvinto()'.  Bug #41162.
**	14-jan-92 (seg)
**	    Can't do arithmetic on PTR variables.
**      02-Aug-99 (islsa01)
**          Fixed for the bug# 97033.  
**          Checks for an unknown datatype, if found sets the idbv and odbv
**          to NULL.
**	1-july-2004 (gupsh01)
**	    Fixed inserting wchar_t values to nchar/nvarchar columns. If
**	    the lengths were specified to afe_cvinto(), on a platform where
**	    sizeof(wchar_t) is 4, the input data was not being converted due
**	    to being of same length. (bug 112592).
**	07-sep-2006 (gupsh01)
**	    Added support for new ANSI date/time types.
**	10-sep-2006 (gupsh01)
**	    Fixed copying precision value for ANSI date/time types.
**	23-jul-2008 (gupsh01)
**	    When coercing from a char/varchar to nchar/nvarchar adjust 
**	    the result length for solaris platform, where 
**	    wchar_t is 4 bytes.
**	22-Jan-2009 (kiria01) SIR 121297
**	    Propagate precision for dates
*/

DB_STATUS
afe_cvinto ( acb, idbv, odbv )
ADF_CB		*acb;
DB_DATA_VALUE	*idbv;
DB_DATA_VALUE	*odbv;
{
    DB_DT_ID	itype, otype;		/* Input/output type */
    i4	ilen, olen;		/* Input/output length */
    i2		iprec, oprec;		/* Input/output precision (DEC only) */
    PTR		idata, odata;		/* Input/output data pointers */
    DB_STATUS	stat;			/* Return status from other routines */
    DB_DATA_VALUE idbv_tmp, odbv_tmp;	/* Temp DBV's for date conversions */
    bool	onull = FALSE;		/* Set odbv to NULL on error return */
    EX_CONTEXT	context;
    f8          mny;

    itype = idbv->db_datatype;
    ilen = idbv->db_length;
    iprec = idbv->db_prec;
    idata = idbv->db_data;
    otype = odbv->db_datatype;
    olen = odbv->db_length;
    oprec = odbv->db_prec;
    odata = odbv->db_data;


    /*
    ** If input is nullable, then check if it's null.  If so set output to
    ** null and return.  Otherwise assume input is not nullable for rest of
    ** routine.
    */
    if ( AFE_NULLABLE(itype) )
    { /* Input is Nullable */
	if ( AFE_ISNULL(idbv) )
	{ /* Input is Null */
	    if ( !AFE_NULLABLE(otype) )
	    { /* Output is not Nullable */
		acb->adf_errcb.ad_errcode = E_AD1012_NULL_TO_NONNULL;
		acb->adf_errcb.ad_emsglen = 0;
		return E_DB_ERROR;
	    }
	    /* Types compatible? */
	    else if ( ! afe_tycoerce(acb, itype, otype) )
	    {
		if (acb->adf_errcb.ad_errclass == ADF_INTERNAL_ERROR)
		    convert_err(acb, idbv, odbv);
		return E_DB_ERROR;
	    }
	    else
	    { /* Set output to Null and return */
		AFE_SETNULL(odbv);
		return E_DB_OK;
	    }
	}
	/* Input is Nullable but not Null - assume not Nullable for routine */
	itype = AFE_DATATYPE(idbv);
	--ilen;
    }
    /*
    ** If output is Nullable (input was not already Null) set it to
    ** non-Nullable for rest of routine.
    */
    if ( AFE_NULLABLE(otype) )
    {
	if ( AFE_ISNULL(odbv) )
	{
	    onull = TRUE;
	    AFE_CLRNULL(odbv);
	}
	otype = AFE_DATATYPE(odbv);
	--olen;
    }


   /* 
   ** if it is an unknown datatype, set idbv and odbv to NULL 
   */

    if (itype == DB_NODT)
    {
       /* Set output to NULL and return */
       AFE_SETNULL(idbv);
       AFE_SETNULL(odbv);
       return E_DB_OK;
    }

    /*
    ** This is a temporary kluge to deal with the difference in size between
    ** wchar_t on Solaris (4 bytes) and the DB_NCHR_TYPE (2 bytes). Eventually
    ** this should go into ADF.
    */
    if (itype == otype && (itype == DB_NCHR_TYPE || itype == DB_NVCHR_TYPE) &&
	sizeof(wchar_t) != 2)
    {
	i4	i;
	u_i2	*po;
	
	if (otype == DB_NVCHR_TYPE)
	{
	    po = (u_i2 *)(odata + DB_CNTSIZE);
	    *((u_i2 *)odata) = *(u_i2 *)idata;
	}
	else
	    po = (u_i2 *)odata;

	if (sizeof(wchar_t) == 4)
	{
	    u_i4	*pi;
	    i4		len;

	    /* need to skip over the length indicator */
	    if (itype == DB_NVCHR_TYPE)
	    {
		pi = (u_i4 *)((AFE_NTEXT_STRING *)idata)->afe_t_text;
		len = (ilen - DB_CNTSIZE) / 4;
	    }
	    else
	    {
		pi = (u_i4 *)idata;
		len = ilen / 4;
	    }
	    for (i = 0; i < len; ++i, ++pi, ++po)
		*po = (u_i2)*pi;
	    return E_DB_OK;
	}
	/* missing the inverse conversion for now */
    }

    /*
    ** Copy if the same type and length.
    ** Note that we do NOT check 'db_prec' for non-decimal types because we do
    ** not want to compare undefined values.   Can only assume at this point
    ** (barbara, 7-19-90) that 'db_prec' field is set for decimal type.
    */
    if (   itype == otype
	&& ilen == olen
	&& (itype != DB_DEC_TYPE || iprec == oprec))
    {
	MECOPY_VAR_MACRO(idata, (u_i2)ilen, odata);
	return E_DB_OK;
    }

    /* Based on input type figure out how to convert the data */
    switch (itype)
    {
      /* Numerics */
      case DB_INT_TYPE:
      case DB_FLT_TYPE:
	switch (otype)
	{
	  case DB_INT_TYPE:
	  case DB_FLT_TYPE:
	    stat = iiafCvtNum(acb, itype, ilen, idata, otype, olen, odata);
	    break;
	  case DB_DEC_TYPE:	/* Decimal is handled by ADF */
	    xhdlr_adfcb = acb; 	/* Save adf cb so handler can access it */
	    if (EXdeclare(IIAFadfxhdlr, &context) != OK)
	    {
		EXdelete();
	    	return(xhdlr_stat);
	    }
	    stat = adc_cvinto(acb, idbv, odbv);
	    EXdelete();
	    break;
	  default:
	    if ((stat = adc_cvinto(acb, idbv, odbv)) != E_DB_OK
		&& acb->adf_errcb.ad_errclass == ADF_INTERNAL_ERROR)
	    {
		convert_err(acb, idbv, odbv);
	    }
	    return stat;
	    break;
	}
	break;

          case DB_MNY_TYPE:
	    /*
	    **  Handle conversion from money to int here since users
	    **  were depending on this but ADF itself does not support it.
	    **  If output type is NOT an integer type, fall through and
	    **  let ADF handle it.
	    */
	    if (otype == DB_INT_TYPE)
	    {
	      MECOPY_CONST_MACRO(idata, sizeof(f8), (PTR) &mny);
	      mny /= 100;
	      itype = DB_FLT_TYPE;
	      ilen = sizeof(mny);
	      idata = (PTR) &mny;
	      stat = iiafCvtNum(acb, itype, ilen, idata, otype, olen, odata);
	      break;
	    }
      case DB_DEC_TYPE:
	xhdlr_adfcb = acb;	/* Save adf cb so handler can access it */
	if (EXdeclare(IIAFadfxhdlr, &context) != OK)
	{
	    EXdelete();
	    return(xhdlr_stat);
	}
	stat = adc_cvinto(acb, idbv, odbv);
	EXdelete();
	break;

      case DB_BOO_TYPE:
        switch (otype)
        {
          case DB_INT_TYPE:
 	  case DB_BOO_TYPE:
            switch (olen)
            {
              case 1:
                *(i1 *)odata = *(i1 *)idata;
                break;
              case 2:
                *(i2 *)odata = *(i1 *)idata;
                break;
              case 4:
                *(i4 *)odata = *(i1 *)idata;
                break;
              case 8:
                *(i8 *)odata = *(i1 *)idata;
              default:
                return E_DB_ERROR;
            }
            return E_DB_OK;
          default:
            return E_DB_ERROR;
        }
        break;

      /* Strings */
      case DB_TXT_TYPE:			/* Pick off the fixed part of varchar */
      case DB_VCH_TYPE:
      case DB_LTXT_TYPE:
	ilen = ((DB_TEXT_STRING *)idata)->db_t_count;
	idata = (PTR)((DB_TEXT_STRING *)idata)->db_t_text;
	/* FALL THROUGH */
      case DB_CHA_TYPE:
      case DB_CHR_TYPE:
	switch (otype)
	{
	  case DB_CHA_TYPE:
	  case DB_CHR_TYPE:
	    /* Based on exact fit/truncate/pad rules copy and fill */
	    if (ilen <= olen)
	    {
		MECOPY_VAR_MACRO(idata, (u_i2)ilen, odata);
	    }
	    else
	    {
		ilen = CMcopy(idata, olen, odata);
	    }
	    if (ilen < olen)
		MEfill((u_i2)(olen - ilen), ' ', (char *)odata + ilen);
	    return E_DB_OK;
	  case DB_TXT_TYPE:
	  case DB_VCH_TYPE:
	  case DB_LTXT_TYPE:
	    if (itype == DB_CHA_TYPE || itype == DB_CHR_TYPE)
	    	AFE_TRIMBLANKS_MACRO((char *)idata, ilen);
	    olen -= DB_CNTSIZE;
	    if (ilen <= olen)
	    {
		MECOPY_VAR_MACRO(idata, (u_i2)ilen, (char *)odata + DB_CNTSIZE);
	    }
	    else
	    {
		ilen = CMcopy(idata, olen, (char *)odata + DB_CNTSIZE);
	    }
	    ((DB_TEXT_STRING *)odata)->db_t_count = ilen;
	    return E_DB_OK;
	  case DB_DTE_TYPE:
	    /* Internal DATE routines requires non-nullable values */
	    idbv_tmp.db_datatype = DB_CHA_TYPE;
	    idbv_tmp.db_length = ilen;
	    idbv_tmp.db_data = idata;
	    odbv_tmp.db_datatype = DB_DTE_TYPE;
	    odbv_tmp.db_length =
		(odbv->db_datatype < 0) ? odbv->db_length - 1 : odbv->db_length;
	    odbv_tmp.db_data = odata;
	    odbv_tmp.db_prec = oprec;
	    stat = adu_14strtodate(acb, &idbv_tmp, &odbv_tmp);
	    break;
	  case DB_ADTE_TYPE:
	  case DB_TMWO_TYPE:
	  case DB_TMW_TYPE:
	  case DB_TME_TYPE:
	  case DB_TSWO_TYPE:
	  case DB_TSW_TYPE:
	  case DB_TSTMP_TYPE:
	  case DB_INYM_TYPE:
	  case DB_INDS_TYPE:
	    /* Internal DATE routines requires non-nullable values */
	    idbv_tmp.db_datatype = DB_CHA_TYPE;
	    idbv_tmp.db_length = ilen;
	    idbv_tmp.db_data = idata;
	    odbv_tmp.db_datatype = otype;
	    odbv_tmp.db_length =
		(odbv->db_datatype < 0) ? odbv->db_length - 1 : odbv->db_length;
	    odbv_tmp.db_data = odata;
	    odbv_tmp.db_prec = oprec;
	    stat = adu_21ansi_strtodt(acb, &idbv_tmp, &odbv_tmp);
	    break;

	  default:
	    if ((stat = adc_cvinto(acb, idbv, odbv)) != E_DB_OK
		&& acb->adf_errcb.ad_errclass == ADF_INTERNAL_ERROR)
	    {
		convert_err(acb, idbv, odbv);
	    }
	    /* If the coercion result is an nchar/nvarchar type adjust 
	    ** the data for solaris platform, where sizeof(wchar_t) = 4
	    ** (Bug 129211).
	    */
            if ((otype == DB_NCHR_TYPE || otype == DB_NVCHR_TYPE) &&
               ((itype != DB_NVCHR_TYPE) && (itype != DB_NCHR_TYPE)) &&
               sizeof(wchar_t) == 4) 
            {
	      i4      i;
	      u_i4    *p_out;
	      char    *bufp;
	      bool    gotmemory = FALSE;
	      u_i4    *resbuf;

	      /* Define unibuffer lengths to be used locally */
	      #define UNI_OUTBUF_LEN	2000
	      #define UNI_INBUF_LEN	(UNI_OUTBUF_LEN / 2)

	      char    work_buffer[UNI_OUTBUF_LEN];

	      if (olen > UNI_INBUF_LEN) 
	      {
		/* Allocate a suitably sized working buffer */
		resbuf = (u_i4 *)MEreqmem(0, olen, TRUE, &stat);
		if (resbuf != NULL)
		  gotmemory = TRUE;
		else
		{
		  convert_err(acb, idbv, odbv);
		  return E_DB_ERROR;
		}
	      }
	      else 
		resbuf = (u_i4 *)work_buffer;

              if (otype == DB_NVCHR_TYPE)
              {
                 p_out = (u_i4 *)(((AFE_NTEXT_STRING*)resbuf)->afe_t_text);
              }
              else
                p_out = (u_i4 *)resbuf;

              if (sizeof(wchar_t) == 4)
              {
                u_i2        *p_in;
                i4          len;

                /* need to skip over the length indicator */
                if (otype == DB_NVCHR_TYPE)
                {
                    p_in = ((DB_NVCHR_STRING *)odata)->element_array;
                    len = (olen - DB_CNTSIZE) / 4;

                    /* Set the count in the output buffer */
                    ((AFE_NTEXT_STRING*)resbuf)->afe_t_count =
                        ((DB_NVCHR_STRING *)odata)->count;
                }
                else
                {
                    p_in = (u_i2 *)odata;
                    len = olen / 4;
                }
                for (i = 0; i < len; ++i, ++p_in, ++p_out)
                    *p_out = (u_i2)*p_in;

		MEcopy (resbuf, olen, odata);

	        if (gotmemory) 
		  MEfree ((char *)resbuf);

                return E_DB_OK;
              }
	     }
	    return stat;
	   break;
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
      case DB_INDS_TYPE:
      case DB_INYM_TYPE:
	switch (otype)
	{
	  case DB_CHA_TYPE:
	  case DB_CHR_TYPE:
	  case DB_TXT_TYPE:
	  case DB_VCH_TYPE:
	  case DB_LTXT_TYPE:
	    /* Internal DATE routines requires non-nullable values */
	    idbv_tmp.db_datatype = itype;
	    idbv_tmp.db_length =
		idbv->db_length - ((idbv->db_datatype < 0) ? 1 : 0);
	    idbv_tmp.db_data = idata;
	    idbv_tmp.db_prec = iprec;
	    odbv_tmp.db_datatype = abs(odbv->db_datatype);
	    odbv_tmp.db_length =
		odbv->db_length - ((odbv->db_datatype < 0) ? 1 : 0);
	    odbv_tmp.db_data = odata;
	    stat = adu_6datetostr(acb, &idbv_tmp, &odbv_tmp);
	    break;

	  case DB_DTE_TYPE:
	  case DB_ADTE_TYPE:
	  case DB_TMWO_TYPE:
	  case DB_TMW_TYPE:
	  case DB_TME_TYPE:
	  case DB_TSWO_TYPE:
	  case DB_TSW_TYPE:
	  case DB_TSTMP_TYPE:
	  case DB_INDS_TYPE:
	  case DB_INYM_TYPE:
	    if (itype ==  otype)
	    {
	      MECOPY_VAR_MACRO(idata, (u_i2)ilen, odata);
	      return E_DB_OK;
	    }
	    else
	    {
	      /* Internal DATE routines requires non-nullable values */
	      idbv_tmp.db_datatype = itype;
	      idbv_tmp.db_length =
		idbv->db_length - ((idbv->db_datatype < 0) ? 1 : 0);
	      idbv_tmp.db_data = idata;
	      idbv_tmp.db_prec = iprec;
	      odbv_tmp.db_datatype = abs(odbv->db_datatype);
	      odbv_tmp.db_length =
		odbv->db_length - ((odbv->db_datatype < 0) ? 1 : 0);
	      odbv_tmp.db_data = odata;
	      odbv_tmp.db_prec = oprec;
	      stat = adu_2datetodate(acb, &idbv_tmp, &odbv_tmp);
	    }
	    break;

	  default:
	    if ((stat = adc_cvinto(acb, idbv, odbv)) != E_DB_OK
		&& acb->adf_errcb.ad_errclass == ADF_INTERNAL_ERROR)
	    {
		convert_err(acb, idbv, odbv);
	    }
	    return stat;
	    break;
	}
	break;
      default:
	if ((stat = adc_cvinto(acb, idbv, odbv)) != E_DB_OK
	    && acb->adf_errcb.ad_errclass == ADF_INTERNAL_ERROR)
	{
	    convert_err(acb, idbv, odbv);
	}
	return stat;
    }
    if ( stat != OK && onull )
	AFE_SETNULL(odbv);

    return stat;
} /* afe_cvinto */

/*{
**  Name:	iiafCvtNum() - Fast numeric conversion routine.
**
**  Description:
**	Routine to convert numeric values of one type and length, to a
**	(not necessarily) different type and length.  This routine can be
**	used instead of adc_cvinto for fast numeric conversions.
**
**	The input and output numeric can be i1, i2, i4, i8, f4, or f8.
**
**	The input number will be converted to the type and length specified
**	in the output.
**
**  Inputs:
**	acb			Pointer to the current ADF_CB control block.
**	itype			Input data type id (DB_INT_TYPE or DB_FLT_TYPE
**				not nullable).
**	ilen			Input data type length (may not include null).
**	idata			Input data value.
**	otype			Output data type id (DB_INT_TYPE or DB_FLT_TYPE
**				not nullable).
**	olen			Output data type length (may not include null).
**
**  Outputs:
**	acb.adf_errcb		ADF error data (if errroneous conversion):
**	   .ad_errcode		Error code if failure.  See numbers below.
**	   .ad_emsglen		If an error occured this is set to zero as
**				no message is looked up.
**	odata			Output data area filled.
**
**	Returns:
**	    	E_DB_OK		If succeeded.
**	    	E_DB_ERROR	If error occurred in the conversion.
**	    	E_DB_WARN	If warning occurred in the conversion.
**	Errors:
**		E_AD0121_INTOVF_WARN - Numeric overflow from iiafCvtNum.
**		E_AD2005_BAD_DTLEN - Invalid data type length.
**
**  Side Effects:
**	None
**	
**  History:
**	Was originally IIconvert().
**	24-jul-1984	- Do floating point internally in f8 to avoid loss of
**			  precision.  Change "number" to be of type "union",
**			  so we don't implicitly assume that the size of our
**			  union is 8 bytes. (Bug 3754). (mrw)
**	18-jul-1984	- Correct a problem in f4->f8 conversion caused by a
**			  Whitesmiths C deficiency. (scl)
**	10-oct-1986	- Modified to reduce copy overhead. (ncg)
**	31-oct-1988	- Rewrote as afe_cvtnum (ncg)
**	13-dec-89 (thomasg)
**		conditionally replace calls of MECOPY_VAR_MACRO with 'MEcopy()'
**		because of pre-processor/compilation errors on apu_us5.
**	08-june-90 (vijay)
**		Added I1_CHECK_MACROs.
**	31-oct-1988	- Rewrote as iiafCvtNum (ncg)
**	08-june-90 (vijay)
**		Added I1_CHECK_MACROs.
**	17-feb-1997 (pchang)
**	    	Allowed numeric conversion of 'integer' to 'long' for axp_osf.
**		This can be extended in future to cover other machines with 
**		64-bit architecture where 'long' is 8 bytes in length. (B76023)
**	11-Sep-2000 (hanje04)
**		Added (axp_lnx) to 'int' - 'long' numeric conversion.
**	06-Mar-2003 (hanje04)
**		Extend axp_osf int - long change to all 64 bit platforms (LP64)
**	15-Jul-2004 (schka24)
**	    Support i8's everywhere.
*/

DB_STATUS
iiafCvtNum ( acb, itype, ilen, idata, otype, olen, odata )
ADF_CB		*acb;
DB_DT_ID	itype, otype;
i4		ilen, olen;
PTR		idata, odata;
{
    /* Any numeric-conversion type union */
    union any_number {
	i1	i1type;
	i2	i2type;
	i4	i4type;
	i8	i8type;
	f4	f4type;
	f8	f8type;
    } num;
    f8		f8low, f8high;		/* Temps for overflow checks */
    f8		tempf8;			/* Temp for Whitesmith copy */

    /* If exact matches copy */
    if (itype == otype && ilen == olen)
    {
	MECOPY_VAR_MACRO(idata, (u_i2)ilen, odata);
	return E_DB_OK;
    }
    /* Validate lengths of input and output */
    if (ilen > sizeof(f8) || olen > sizeof(f8))
    {
	acb->adf_errcb.ad_errcode = E_AD2005_BAD_DTLEN;
	acb->adf_errcb.ad_emsglen = 0;
	return E_DB_ERROR;
    }

    MECOPY_VAR_MACRO(idata, (u_i2)ilen, (PTR)&num);/* Copy number into buffer */
    if (itype != otype)
    {
	/*
	** The source and destination types are different so the source is
	** converted to an i8 if the destination is an integer, otherwise
	** to an f8 if the destination is a float.
	*/
	if (otype == DB_FLT_TYPE)	/* itype = INT: any_integer->f8 */
	{
	    switch (ilen)
	    {
	      case sizeof(i1):	/* i1 to f8 */
		num.f8type = I1_CHECK_MACRO(num.i1type);
		break;
	      case sizeof(i2):	/* i2 to f8 */
		num.f8type = num.i2type;
		break;
	      case sizeof(i4):	/* i4 to f8 */
		num.f8type = num.i4type;
		break;
	      case sizeof(i8):	/* i8 to f8 */
		num.f8type = num.i8type;
		break;
	      default:
		acb->adf_errcb.ad_errcode = E_AD2005_BAD_DTLEN;
		acb->adf_errcb.ad_emsglen = 0;
		return E_DB_ERROR;
	    }
	    ilen = sizeof(f8);
	    /* itype = INT, otype = FLOAT, ilen = 8, num.f8type = idata  */
	}
	else		/* itype = FLOAT, otype = INT: any_float->i4 */
	{
	    /* Will the float overflow the integer? */
	    f8low = (f8)MINI8;
	    f8high = (f8)MAXI8;

	    switch (ilen)
	    {
	      case sizeof(f4):
		if (num.f4type > f8high || num.f4type < f8low)
		{
		    acb->adf_errcb.ad_errcode = E_AD0121_INTOVF_WARN;
		    acb->adf_errcb.ad_emsglen = 0;
		    return E_DB_WARN;
		}
		num.i8type = (i8) num.f4type;
		break;
	      case sizeof(f8):
		if (num.f8type > f8high || num.f8type < f8low)
		{
		    acb->adf_errcb.ad_errcode = E_AD0121_INTOVF_WARN;
		    acb->adf_errcb.ad_emsglen = 0;
		    return E_DB_WARN;
		}
		num.i8type = (i8) num.f8type;
		break;
	      default:
		acb->adf_errcb.ad_errcode = E_AD2005_BAD_DTLEN;
		acb->adf_errcb.ad_emsglen = 0;
		return E_DB_ERROR;
	    }
	    ilen = sizeof(i8);
	    /* itype = FLOAT, otype = INT, ilen = 8, num.i8type = idata  */
	}
    }

    /* "num" is now the same type as destination; convert lengths to match */
    if (ilen != olen)
    {
	if (otype == DB_FLT_TYPE) /* otype is a FLOAT and "num" is an f8 */
	{
	    switch (olen)
	    {
	      case sizeof(f4):
		num.f4type = num.f8type;	/* f8 to f4 with rounding */
		break;
	      case sizeof(f8):
		/*
		** Required step of a problem in the Whitesmiths C compiler;
		** without an intermediate step the f8 always ends up zero.
		*/
		tempf8 = num.f4type;   		/* f4 to f8 */
		num.f8type = tempf8;	    	/* f8 to f8 */
		break;
	      default:
		acb->adf_errcb.ad_errcode = E_AD2005_BAD_DTLEN;
		acb->adf_errcb.ad_emsglen = 0;
		return E_DB_ERROR;
	    }
	}
	else			/* otype is an INT and "num" is an i4 */
	{
	    /* For simplicity, convert to i8, then down to olen.
	    ** This is faster on most modern machines than all sorts of
	    ** jumping around anyway.
	    */
	    switch (ilen)
	    {
	    case 1:
		num.i8type = num.i1type;
		break;
	    case 2:
		num.i8type = num.i2type;
		break;
	    case 4:
		num.i8type = num.i4type;
		break;
	    case 8:
		break;
	    default:
		acb->adf_errcb.ad_errcode = E_AD2005_BAD_DTLEN;
		acb->adf_errcb.ad_emsglen = 0;
		return E_DB_ERROR;
	    }
	    switch (olen)
	    {
	    case sizeof(i1):
		if (num.i8type > MAXI1 || num.i8type < MINI1)
		{
		    acb->adf_errcb.ad_errcode = E_AD0121_INTOVF_WARN;
		    acb->adf_errcb.ad_emsglen = 0;
		    return E_DB_WARN;
		}
		num.i1type = num.i8type;
		break;
	    case sizeof(i2):
		if (num.i8type > MAXI2 || num.i8type < MINI2)
		{
		    acb->adf_errcb.ad_errcode = E_AD0121_INTOVF_WARN;
		    acb->adf_errcb.ad_emsglen = 0;
		    return E_DB_WARN;
		}
		num.i2type = num.i8type;
		break;
	    case sizeof(i4):
		if (num.i8type > MAXI4 || num.i8type < MINI4LL)
		{
		    acb->adf_errcb.ad_errcode = E_AD0121_INTOVF_WARN;
		    acb->adf_errcb.ad_emsglen = 0;
		    return E_DB_WARN;
		}
		num.i4type = num.i8type;
		break;
	    case 8:
		break;
	    default:
		acb->adf_errcb.ad_errcode = E_AD2005_BAD_DTLEN;
		acb->adf_errcb.ad_emsglen = 0;
		return E_DB_ERROR;
	    }
	} /* Output is integer */
    } /* Length not equal */

    /* Conversion is complete copy the result into output */
    MECOPY_VAR_MACRO((PTR)&num, (u_i2)olen, odata);
    return E_DB_OK;
}

/*{
** Name:	convert_err	- Generate no conversion error
**			
** Description:
**	This routine adds the error E_AD2001_NO_CONVERSION to the
**	ADF_CB.
**
** Inputs:
**	adfcb		The ADF_CB.
**
**	idbv		The dbv of the source.
**
**	odbv		The dbv of the destination.
**
** Outputs:
**	adfcb
**	    .adf_errcb	Set to contain the E_AF2001_NO_CONVERSION error.
**
** History:
**	13-feb-1991 (Joe)
**	     First Written
*/
static VOID
convert_err(adfcb, idbv, odbv)
ADF_CB		*adfcb;
DB_DATA_VALUE	*idbv;
DB_DATA_VALUE	*odbv;
{
    ER_MSGID		typeid;
    DB_USER_TYPE	itype;
    DB_USER_TYPE	otype;

    if (afe_tyoutput(adfcb, idbv, &itype) != OK)
    {
	typeid = ( idbv->db_datatype == DB_OBJ_TYPE )
			? S_AF0000_OBJECT : S_AF0001_MISSING;
	STlcopy(ERget(typeid), itype.dbut_name, DB_TYPE_SIZE);
    }
    if (afe_tyoutput(adfcb, odbv, &otype) != OK)
    {
	typeid = ( odbv->db_datatype == DB_OBJ_TYPE )
	    		? S_AF0000_OBJECT : S_AF0001_MISSING;
	STlcopy(ERget(typeid), otype.dbut_name, DB_TYPE_SIZE);
    }
    _VOID_ afe_error( adfcb, E_AF2001_NO_CONVERSION,
		     4, STlength(itype.dbut_name), (PTR) itype.dbut_name,
		     STlength(otype.dbut_name), (PTR) otype.dbut_name
    );
}
