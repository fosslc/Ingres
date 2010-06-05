/*
** Copyright (c) 1986 - 1999,2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <st.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <adfops.h>
#include    <ade.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduucol.h>
#include    "adudate.h"
/*
[@#include@]...
*/

/**
**
**  Name: ADICALCLEN.C - Calculate result length from LENSPEC.
**
**  Description:
**      This file contains all the routines necessary to calculate a result 
**      length based on a supplied length specification and datatypes and 
**      lengths for two operands.
**
**          adi_0calclen() - Calculate result length from LENSPEC. Replacing
**                           adi_calclen which is obseleted.
**
**  History:
**      20-jun-86 (thurston)    
**          Initial creation.
**	01-jul-86 (thurston)
**	    Added code to the adi_calclen() routine for the ADI_RESLEN length
**	    spec. 
**	02-jul-86 (thurston)
**	    In routine adi_calclen() I changed the type of argument adi_rlen to
**          be a ptr to an i4 instead of a ptr to a nat.  This makes it
**          consistent with the .db_length field of a DB_DATA_VALUE. 
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	22-sep-86 (thurston)
**	    In routine adi_calclen(), I added a fix for the case where the
**	    lenspec is { ADI_FIXED, 0 } so that this routine returns E_DB_OK
**	    instead of E_AD2022_UNKNOWN_LEN.
**	30-sep-86 (thurston)
**	    Made a fix to adi_calclen() that enables the caller to use NULL
**	    pointers for adi_dv1 and/or adi_dv2 if they are not needed.
**	15-oct-86 (thurston)
**	    Added code to the adi_calclen() routine for the ADI_SUMT length
**	    spec.
**	11-nov-86 (thurston)
**	    Added code to the adi_calclen() routine for the ADI_P2D2, ADI_2XP2,
**	    and ADI_2XM2 length specs.
**	20-nov-86 (thurston)
**	    In adi_calclen(), I added the length spec parameter to the
**	    adu_error() call for ADF error E_AD2020_BAD_LENSPEC.
**	28-dec-86 (thurston)
**	    Fixed adi_calclen() so that it now uses the ADE_LEN_UNKNOWN constant
**	    to detect unknown lengths.
**	22-feb-87 (thurston)
**	    Added support for nullable datatypes.
**	20-mar-87 (thurston)
**	    Fixed bug in adi_calclen() with datatype ID of zero (an illegal
**	    datatype) that was being treated as a nullable, thus throwing off
**	    some length calc's.
**	04-may-87 (thurston)
**	    Made many changes to lenspec code calculations so as to avoid
**	    creating intermediate values that are too long.
**	03-feb-88 (thurston)
**	    Modified the code at the bottom of adi_0calclen() to set the result
**	    length to ADE_LEN_UNKNOWN if the E_AD2022_UNKNOWN_LEN error code is
**	    returned.
**	08-feb-88 (thurston)
**	    The ADI_CSUM, ADI_CTSUM, and ADI_O1TC lenspec now assures the result
**	    will be at least 1.
**	01-sep-88 (thurston)
**	    Changed the use of DB_CHAR_MAX and DB_MAXSTRING to
**	    adf_scb->adf_maxstring, since this is now a session settable
**	    parameter.
**	27-jun-89 (jrb)
**	    Fixed problem where we were decrementing the constant value
**	    ADE_LEN_UNKNOWN whenever the datatype was nullable.
**	12-jul-89 (jrb)
**	    Modified all lenspecs to return precision/scale as well as length.
**	    Added decimal lenspecs.
**	25-jul-89 (jrb)
**	    Added a new lenspec for the decimal() conversion function.  This
**	    mandated a change to this routine's interface because we need to
**	    look at the value of the 2nd parameter to derive the result length
**	    for this function.
**      05-apr-91 (jrb)
**          Bug #36901.  ADI_2XP2 didn't limit the result length to
**          adf_maxstring as it should have.
**	05-oct-1992 (fred)
**	    Added char <-> bit string length calculations.
**      08-dec-1992 (stevet)
**          Added support for ADI_LEN_EXTERN and ADI_LEN_INDIRECT.  
**          ADI_LEN_INDIRECT allows functions using DB_ALL_TYPE to
**          map various input datatypes to result length.  ADI_LEN_EXTERN
**          allow UDT function to specify an LENSPEC function to 
**          run to calculate result length.
**      21-dec-1992 (stevet)
**          Added function prototype.
**	21-mar-1993 (robf)
**	    Allow for long return values when doing varchar(security_label,N)
**          etc, let N be up to maxium size for external string, not the
**	    default size.
**	14-may-1993 (robf)
**	    Add support for DB_SECID_TYPE
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	21-sep-93 (wolf) 
**	    Not so fast--dbdbms.h defines DB_SEC_LABEL, still needed here
**	11-oct-93 (wolf) 
**	    DB_SEC_LABEL replaced (in aduadtsc.h) so dbdbms.h can be removed
**	02-jul-97 (i4jo01)
**	    Modify DECMUL scale calculation.
**	31-jul-1998 (somsa01)
**	    In the case of long varchar or long varbyte inputs to functions,
**	    the length which is in adi_dv1->db_length is actually the length
**	    of the coupon. Therefore, the length of the result should be
**	    the maximum allowed for the session. Also added missing option
**	    for long varbyte in adi_1len_indirect().  (Bug #74162)
**	04-nov-1998 (somsa01 for natjo01)
**	    Modify DECIDIV scale calculation.  (Bug #91609)
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	22-Feb-1999 (shero03)
**	    Support 4 operands
**      13-Apr-1999 (hanal04)
**          Prevent SIGSEGV in adi_0calclen(). b96344.
**	09-Jun-1999 (shero03)
**	    Add ADI_X2P2 for unhex function.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	03-apr-2001 (gupsh01)
**	    Added support for new unicode datatype long nvarchar. 
**      15-Aug-2002 (huazh01)
**          Modify the formula for computing the result scale of a
**          decimal division. we now use:
**          max(DB_MAX_DECPREC - p1 + s1 - s2, min(s1, s2)) instead of
**          max(DB_MAX_DECPREC - p1 + s1 - s2, 0);
**          This fixes bug 108520. INGSRV 1861.
**	20-july-2004 (gupsh01)
**	    Fixed nvarchar() function.
**      19-dec-2006 (stial01)
**          adi_0calclen() ADI_NVCHAR_OP can have 2nd arg with ADI_LEN_INDIRECT
**      11-jan-2007 (stial01)
**          adi_0calclen() ADI_NVCHAR_OP fixed rlen (bytes) calculation
**	01-Feb-2008 (kiria01) b119859
**	    Add missing NCHAR(all,i) support
**	01-Feb-2008 (kiria01) b119777
**	    Adjust the length qualified coercion functions so that the
**	    length is derived from the length parameter and not limiting to
**	    length of passed string.
**      10-sep-2008 (gupsh01,stial01)
**          Fixed calclen for ADI_O1UNIDBL, Added calclen for ADI_O1UNORM
**	24-Aug-2009 (kschendel) 121804
**	    Need me.h to satisfy gcc 4.3.
**	12-Mar-2010 (toumi01) SIR 122403
**	    Added ADI_O1AES for encryption.
**/

/*
[@forward_type_references@]
[@forward_function_references@]
[@#defines_of_other_constants@]
[@type_definitions@]
[@global_variable_definitions@]
*/
/*
[@static_variable_or_function_definitions@]
*/

/* Create LENSPEC based on input datatypes */
static DB_STATUS adi_1len_indirect(ADI_OP_ID          fiopid,
				   DB_DATA_VALUE      *dv1,
				   DB_DATA_VALUE      *dv2,
				   ADI_LENSPEC        *out_lenspec);


/*
**
**	O B S O L E T E    R O U T I N E  !!!!!
**
**	No one should be calling this routine any longer... adi_0calclen
**	(below) is the proper interface now.
**
**
**
** Name: adi_calclen() - Calculate result length from LENSPEC.
**
** Description:
**	This routine calculates the length and precision of a result operand
**	given the datatypes, precisions and lengths of the input operand(s) and
**	an ADI_LENSPEC.  If any of the input operand lengths are
**	ADE_LEN_UNKNOWN, this implies that the length is not currently known.
**	This may or may not result in the E_AD2022_UNKNOWN_LEN error status
**	depending on whether the length of that particular operand is important
**	or not to the length spec.  The precision will always be known.
**	Obviously, if the length spec says that the result is a fixed length
**	and precision, then none of the input operands' lengths or precisions
**	will matter.
**
**	If the input length spec is ADI_RESLEN, meaning that the result length
**	should already be known, then this routine will not set *adi_rlen, and
**	will return either E_AD2022_UNKNOWN_LEN or E_AD0000_OK, depending on
**	whether *adi_rlen was ADE_LEN_UNKNOWN or not.  This special length
**      spec is currently only used for the coercion function instances that
**	coerce from a datatype into the same datatype.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	    .adf_maxstring		Max user size of any INGRES string.
**      adi_lenspec                     Ptr to the ADI_LENSPEC which tells how
**                                      to calculate the result length based on
**                                      the input lengths.
**      adi_dv1                         Ptr to DB_DATA_VALUE for first input
**                                      operand.  (Only used as a convenient
**                                      mechanism for supplying a datatype and
**                                      length.) 
**	    .db_datatype		Datatype for first input operand.
**	    .db_length  		Length of first input operand, or
**                                      ADE_LEN_UNKNOWN if unknown.
**      adi_dv2                         Ptr to DB_DATA_VALUE for second input
**                                      operand.  (Only used as a convenient
**                                      mechanism for supplying a datatype and
**                                      length.) 
**	    .db_datatype		Datatype for second input operand.
**	    .db_length  		Length of second input operand, or
**                                      ADE_LEN_UNKNOWN if unknown.
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
**      adi_rlen                        Result length.
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
**	    E_AD0000_OK			Operation succeeded.
**	    E_AD2005_BAD_DTLEN		Bad length for a datatype.
**	    E_AD2020_BAD_LENSPEC	Unknown length spec.
**	    E_AD2021_BAD_DT_FOR_PRINT	Bad datatype for the print stlyle
**					length specs.
**          E_AD2022_UNKNOWN_LEN	Result length depends on the length
**					of a data value you have not supplied.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	20-jun-86 (thurston)
**          Initial creation.
**	01-jul-86 (thurston)
**	    Added code for the ADI_RESLEN length spec.
**	02-jul-86 (thurston)
**	    Changed the type of argument adi_rlen to be a ptr to an i4 instead
**	    of a ptr to a nat.  This makes it consistent with the .db_length
**	    field of a DB_DATA_VALUE.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	22-sep-86 (thurston)
**	    Added a fix for the case where the lenspec is { ADI_FIXED, 0 } so
**	    that this routine returns E_DB_OK instead of E_AD2022_UNKNOWN_LEN.
**	30-sep-86 (thurston)
**	    Made a fix that enables the caller to use NULL pointers for adi_dv1
**	    and/or adi_dv2 if they are not needed.
**	15-oct-86 (thurston)
**	    Added code for the ADI_SUMT length spec.
**	11-nov-86 (thurston)
**	    Added code for the ADI_P2D2, ADI_2XP2, and ADI_2XM2 length specs.
**	20-nov-86 (thurston)
**	    Added the length spec parameter to the adu_error() call for ADF
**	    error E_AD2020_BAD_LENSPEC.
**	28-dec-86 (thurston)
**	    Fixed adi_calclen() so that it now uses the ADE_LEN_UNKNOWN constant
**	    to detect unknown lengths.
**	22-feb-87 (thurston)
**	    Added support for nullable datatypes.
**	20-mar-87 (thurston)
**	    Fixed bug with datatype ID of zero (an illegal datatype) that was
**	    being treated as a nullable, thus throwing off some length calc's.
**	04-may-87 (thurston)
**	    Made many changes to lenspec code calculations so as to avoid
**	    creating intermediate values that are too long.
**	08-feb-88 (thurston)
**	    The ADI_CSUM, ADI_CTSUM, and ADI_O1TC lenspec now assures the result
**	    will be at least 1.
**	01-sep-88 (thurston)
**	    Changed the use of DB_CHAR_MAX and DB_MAXSTRING to
**	    adf_scb->adf_maxstring, since this is now a session settable
**	    parameter.
**      05-apr-91 (jrb)
**          Bug #36901.  ADI_2XP2 didn't limit the result length to
**          adf_maxstring as it should have (this is curious since ADI_2XM2
**          did, and it's almost identical!).
**	30-sep-99 (inkdo01)
**	    Minor correction to length computation for X2P2 (for unhex func.)
**	12-apr-04 (inkdo01)
**	    Added BIGINT support.
**	18-may-2004 (gupsh01)
**	    Added length calculation of nchar, nvarchar functions for float,
**	    and integer input. 
**	15-apr-2008 (gupsh01)
**	    In a UTF8 installs make sure that the length is calculated based
**	    on max value allowed for UTF8 character lengths. Bug 120215.
**	01-Oct-2008 (gupsh01)
**	    For UTF8 installations ensure that the length value does not 
**	    exceed maxstring/2 for char/varchar types.
*/

DB_STATUS
adi_calclen(adf_scb, adi_lenspec, adi_dv1, adi_dv2, adi_rlen)
ADF_CB             *adf_scb;
ADI_LENSPEC        *adi_lenspec;
DB_DATA_VALUE      *adi_dv1;
DB_DATA_VALUE      *adi_dv2;
i4                 *adi_rlen;
{
    register i4		len1;
    register i4		len2;
    register i4		extra;
    DB_DT_ID		dt1;
    bool		nullable = FALSE;


    /* Set appropriate length for first operand */
    if (adi_dv1 == NULL  ||  adi_dv1->db_datatype == 0)
    {
	len1 = ADE_LEN_UNKNOWN;
    }
    else if (adi_dv1->db_datatype > 0)
    {
	len1 = adi_dv1->db_length;
	dt1 = adi_dv1->db_datatype;
    }
    else
    {
	len1 = adi_dv1->db_length - 1;
	nullable = TRUE;
	dt1 = -adi_dv1->db_datatype;
    }

    /* Set appropriate length for second operand */
    if (adi_dv2 == NULL  ||  adi_dv2->db_datatype == 0)
    {
	len2 = ADE_LEN_UNKNOWN;
    }
    else if (adi_dv2->db_datatype > 0)
    {
	len2 = adi_dv2->db_length;
    }
    else
    {
	len2 = adi_dv2->db_length - 1;
	nullable = TRUE;
    }


    /* Now switch on LENSPEC */
    switch (adi_lenspec->adi_lncompute)
    {
	case ADI_O1:
	    /* Length of arg #1 */
	    *adi_rlen = len1;
	    break;

	case ADI_O1UNIDBL:
	    /* Length of arg #1 * 2 */
	    if (len1 == ADE_LEN_UNKNOWN)
		*adi_rlen = ADE_LEN_UNKNOWN;
	    else
		*adi_rlen = len1*2;
	    break;

	case ADI_O1UNORM:
	    if (len1 == ADE_LEN_UNKNOWN)
	    {
		*adi_rlen = ADE_LEN_UNKNOWN;
	    }
	    else
	    {
	      int dtype = abs(dt1);
	      int templen = 0;

	      if ((dtype == DB_VCH_TYPE) || 
		  (dtype == DB_TXT_TYPE) || 
		  (dtype == DB_NVCHR_TYPE))
		templen = DB_CNTSIZE;
				
	      *adi_rlen = templen;
	      if (len1 < templen)
	      {
		  return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 
				2, 0, "unorm user length"));
	      }
	      else 
	      {
		switch (dtype)
		{
		  case DB_CHA_TYPE:
		  case DB_CHR_TYPE:
		    *adi_rlen = (len1 * DB_MAXUTF8_EXP_FACTOR);
		    if ((adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
		        (*adi_rlen > (adf_scb->adf_maxstring/2)))
		      *adi_rlen = (adf_scb->adf_maxstring/2);
		    else if (*adi_rlen > (adf_scb->adf_maxstring)) 
		      *adi_rlen = (adf_scb->adf_maxstring);
		    break;
		  case DB_VCH_TYPE:
		  case DB_TXT_TYPE:
		    *adi_rlen = ((len1 - DB_CNTSIZE) * 
				DB_MAXUTF8_EXP_FACTOR) + DB_CNTSIZE;
		    if ((adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
		        (*adi_rlen > (adf_scb->adf_maxstring/2 + DB_CNTSIZE))) 
		      *adi_rlen = (adf_scb->adf_maxstring/2 + DB_CNTSIZE);
		    else if (*adi_rlen > (adf_scb->adf_maxstring + DB_CNTSIZE))
		      *adi_rlen = (adf_scb->adf_maxstring + DB_CNTSIZE);
		    break;
		  case DB_NCHR_TYPE:
		    *adi_rlen = (len1 * DB_MAXUTF16_EXP_FACTOR);
		    if (*adi_rlen > DB_MAXSTRING) 
		      *adi_rlen = (DB_MAXSTRING);
		    break;
		  case DB_NVCHR_TYPE:
		    *adi_rlen = ((len1 - DB_CNTSIZE) * 
				 DB_MAXUTF16_EXP_FACTOR) + DB_CNTSIZE;
		    if (*adi_rlen > (DB_MAXSTRING + DB_CNTSIZE)) 
		      *adi_rlen = (DB_MAXSTRING + DB_CNTSIZE);
		    break;
		  default: 
	    	    return (adu_error(adf_scb, E_AD2020_BAD_LENSPEC, 2,
		      (i4) sizeof(adi_lenspec->adi_fixedsize),
		      (i4 *) &adi_lenspec->adi_fixedsize));
		    break;
		} 
	      }
	    }
	    break;

	case ADI_O2:
	    /* Length of arg #2 */
	    *adi_rlen = len2;
	    break;

	case ADI_LONGER:
	    /* Length is longer of the args */
	    if (len1 == ADE_LEN_UNKNOWN || len2 == ADE_LEN_UNKNOWN)
		*adi_rlen = ADE_LEN_UNKNOWN;
	    else
		*adi_rlen = max(len1, len2);
	    break;

	case ADI_SHORTER:
	    /* Length is shorter of the args */
	    if (len1 == ADE_LEN_UNKNOWN || len2 == ADE_LEN_UNKNOWN)
		*adi_rlen = ADE_LEN_UNKNOWN;
	    else
		*adi_rlen = min(len1, len2);
	    break;

	case ADI_DIFFERENCE:
	    /* length of arg #1 - length of arg #2 */
	    if (len1 == ADE_LEN_UNKNOWN || len2 == ADE_LEN_UNKNOWN)
		*adi_rlen = ADE_LEN_UNKNOWN;
	    else
		*adi_rlen = len1 - len2;
	    break;

	case ADI_CSUM:
	    /* length of arg #1 + length of arg #2,
	    ** but not greater than DB_CHAR_MAX, and not less than 1
	    */
	    if (len1 == ADE_LEN_UNKNOWN || len2 == ADE_LEN_UNKNOWN)
		*adi_rlen = ADE_LEN_UNKNOWN;
	    else
	      if (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED)
		*adi_rlen = min(DB_UTF8_MAXSTRING, max(1, (len1 + len2)));
	      else
		*adi_rlen = min(DB_CHAR_MAX, max(1, (len1 + len2)));
	    break;

	case ADI_SUMV:
	    /* length of arg #1 + length of arg #2,
	    ** but not greater than maxstring + DB_CNTSIZE
	    */
	    if (len1 == ADE_LEN_UNKNOWN || len2 == ADE_LEN_UNKNOWN)
		*adi_rlen = ADE_LEN_UNKNOWN;
	    else if ((adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
			abs(dt1) != DB_VBYTE_TYPE)
		*adi_rlen = min((adf_scb->adf_maxstring/2 + DB_CNTSIZE),
				(len1 + len2));
	    else
		*adi_rlen = min((adf_scb->adf_maxstring + DB_CNTSIZE),
				(len1 + len2));
	    break;

	case ADI_TSUM:
	    /* length of arg #1 + length of arg #2 - DB_CNTSIZE,
	    ** but not greater than maxstring + DB_CNTSIZE
	    */
	    if (len1 == ADE_LEN_UNKNOWN || len2 == ADE_LEN_UNKNOWN)
		*adi_rlen = ADE_LEN_UNKNOWN;
	    else if ((adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
			abs(dt1) != DB_VBYTE_TYPE)
		*adi_rlen = min((adf_scb->adf_maxstring/2 + DB_CNTSIZE),
				((len1 + len2) - DB_CNTSIZE));
	    else
		*adi_rlen = min((adf_scb->adf_maxstring + DB_CNTSIZE),
				((len1 + len2) - DB_CNTSIZE));
	    break;

	case ADI_CTSUM:
	    /* length of arg #1 + length of arg #2 - DB_CNTSIZE,
	    ** but not greater than DB_CHAR_MAX, and not less than 1
	    */
	    if (len1 == ADE_LEN_UNKNOWN || len2 == ADE_LEN_UNKNOWN)
		*adi_rlen = ADE_LEN_UNKNOWN;
	    else
	      if (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED)
		*adi_rlen = min(DB_UTF8_MAXSTRING,
				max(1, ((len1 + len2) - DB_CNTSIZE)));
	      else
		*adi_rlen = min(DB_CHAR_MAX,
				max(1, ((len1 + len2) - DB_CNTSIZE)));
	    break;

	case ADI_SUMT:
	    /* length of arg #1 + length of arg #2 + DB_CNTSIZE */
	    if (len1 == ADE_LEN_UNKNOWN || len2 == ADE_LEN_UNKNOWN)
		*adi_rlen = ADE_LEN_UNKNOWN;
	    else
		*adi_rlen = len1 + len2 + DB_CNTSIZE;
	    break;

	case ADI_FIXED:
	    /* fixed length */
	    *adi_rlen = adi_lenspec->adi_fixedsize;
	    break;

	case ADI_TPRINT:    /* add DB_CNTSIZE */
	case ADI_PRINT:	    /* add nothing for "c" format */
	case ADI_NPRINT:    /* for nchar output */
	case ADI_NTPRINT:   /* for nvarchar output */
	    /* Result length for both of these LENSPEC's is determined by
            ** the default print characteristics of arg #1's datatype.  This
            ** result type is used only by the ASCII function at the present
            ** time.  A precondition to using this LENSPEC is that arg #1's
            ** datatype must be "f" (float) or "i" (int). 
	    */

	    switch (adi_lenspec->adi_lncompute)
	    {
		case ADI_PRINT:		/* no extra space needed in CHAR type */
		case ADI_NPRINT:	
		    extra = 0;
		    break;

		case ADI_NTPRINT:	
		case ADI_TPRINT:	/* allow extra space for */
		    extra = DB_CNTSIZE;	/* count field in text   */
		    break;
	    }

	    if (dt1 == DB_FLT_TYPE)
	    {
		switch (len1)
		{
		    case 4:
			*adi_rlen = adf_scb->adf_outarg.ad_f4width + extra;
			break;

		    case 8:
			*adi_rlen = adf_scb->adf_outarg.ad_f8width + extra;
			break;

		    default:
			return (adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0));
		}

		if (adi_lenspec->adi_lncompute == ADI_NPRINT)
			*adi_rlen = *adi_rlen * 2;
		else if (adi_lenspec->adi_lncompute == ADI_NTPRINT)
			*adi_rlen = (*adi_rlen - extra) * 2 + extra;

	    }
	    else if (dt1 == DB_INT_TYPE)
	    {
		switch (len1)
		{
		    case 1:
			*adi_rlen = adf_scb->adf_outarg.ad_i1width + extra;
			break;

		    case 2:
			*adi_rlen = adf_scb->adf_outarg.ad_i2width + extra;
			break;

		    case 4:
			*adi_rlen = adf_scb->adf_outarg.ad_i4width + extra;
			break;

		    case 8:
			*adi_rlen = adf_scb->adf_outarg.ad_i8width + extra;
			break;

		    default:
			return (adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0));
		}

		if (adi_lenspec->adi_lncompute == ADI_NPRINT)
			*adi_rlen = *adi_rlen * 2;
		else if (adi_lenspec->adi_lncompute == ADI_NTPRINT)
			*adi_rlen = (*adi_rlen - extra) * 2 + extra;
	    }
	    else
	    {
		/* Who knows what this is... make it a user error */
		return (adu_error(adf_scb, E_AD2021_BAD_DT_FOR_PRINT, 0));
	    }
	    break;

	case ADI_O1CT:
	    /* length of arg #1 + DB_CNTSIZE */
	    *adi_rlen = (len1 != ADE_LEN_UNKNOWN
				? len1 + DB_CNTSIZE
				: ADE_LEN_UNKNOWN);
	    break;

	case ADI_O1TC:
	    /* length of arg #1 - DB_CNTSIZE size,
	    ** but not greater than DB_CHAR_MAX, nor less than 1
	    */
	    if (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED)
	      *adi_rlen = (len1 != ADE_LEN_UNKNOWN
				? min(DB_UTF8_MAXSTRING, max(1, (len1 - DB_CNTSIZE)))
				: ADE_LEN_UNKNOWN);
	    else
	      *adi_rlen = (len1 != ADE_LEN_UNKNOWN
				? min(DB_CHAR_MAX, max(1, (len1 - DB_CNTSIZE)))
				: ADE_LEN_UNKNOWN);
	    break;

	case ADI_ADD1:
	    /* length of arg #1 + 1 */
	    *adi_rlen = (len1 != ADE_LEN_UNKNOWN
				? len1 + 1
				: ADE_LEN_UNKNOWN);
	    break;

	case ADI_SUB1:
	    /* length of arg #1 - 1 */
	    *adi_rlen = (len1 != ADE_LEN_UNKNOWN
				? len1 - 1
				: ADE_LEN_UNKNOWN);
	    break;

	case ADI_P2D2:
	    /* (length of arg #1 + DB_CNTSIZE) / 2 */
	    *adi_rlen = (len1 != ADE_LEN_UNKNOWN
				? (len1 + DB_CNTSIZE) / 2
				: ADE_LEN_UNKNOWN);
	    break;

	case ADI_2XP2:
	    /* (length of arg #1 * 2) + DB_CNTSIZE) */
	    if (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED)
	      *adi_rlen = (len1 != ADE_LEN_UNKNOWN
				? min((adf_scb->adf_maxstring/2 + DB_CNTSIZE),
				      ((len1 * 2) + DB_CNTSIZE))
				: ADE_LEN_UNKNOWN);
	    else
	      *adi_rlen = (len1 != ADE_LEN_UNKNOWN
				? min((adf_scb->adf_maxstring + DB_CNTSIZE),
				      ((len1 * 2) + DB_CNTSIZE))
				: ADE_LEN_UNKNOWN);
	    break;

	case ADI_2XM2:
	    /* (length of arg #1 * 2) - DB_CNTSIZE,
	    ** but not greater than maxstring + DB_CNTSIZE
	    */
	    if (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED)
	      *adi_rlen = (len1 != ADE_LEN_UNKNOWN
				? min((adf_scb->adf_maxstring/2 + DB_CNTSIZE),
				      ((len1 * 2) - DB_CNTSIZE))
				: ADE_LEN_UNKNOWN);
	    else
	      *adi_rlen = (len1 != ADE_LEN_UNKNOWN
				? min((adf_scb->adf_maxstring + DB_CNTSIZE),
				      ((len1 * 2) - DB_CNTSIZE))
				: ADE_LEN_UNKNOWN);
	    break;
	
	case ADI_X2P2:
	    /* (length of arg #1 / 2) + DB_CNTSIZE */
	    /* Remove var. length before computing result length. */
	    extra = (len1 != ADE_LEN_UNKNOWN && (dt1 == DB_VCH_TYPE || 
		dt1 == DB_TXT_TYPE || dt1 == DB_VBYTE_TYPE)) ? 2 : 0;
	    *adi_rlen = (len1 != ADE_LEN_UNKNOWN
				? ((len1 - extra)/ 2) + DB_CNTSIZE
				: ADE_LEN_UNKNOWN);
	    break;

	case ADI_RSLNO1CT:
	    /* length should already be known ... if it is, just return.
	    ** If not, set result length to 1st operand's length.
	    */
	    if (*adi_rlen == ADE_LEN_UNKNOWN)
	    {
		return (adu_error(adf_scb, E_AD2022_UNKNOWN_LEN, 0));
	    }
	    else if (	*adi_rlen < DB_CNTSIZE
	    	     || ((adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
		         (*adi_rlen > adf_scb->adf_maxstring/2 + DB_CNTSIZE)
			)
		     || *adi_rlen > adf_scb->adf_maxstring + DB_CNTSIZE
		    )
	    {
		*adi_rlen = (len1 != ADE_LEN_UNKNOWN
				    ? len1 + DB_CNTSIZE
				    : ADE_LEN_UNKNOWN);
	    }
	    else
	    {
		return (E_DB_OK);
	    }
	    break;

	case ADI_RESLEN:
	    /* length should already be known ... just return */
	    if (*adi_rlen == ADE_LEN_UNKNOWN)
		return (adu_error(adf_scb, E_AD2022_UNKNOWN_LEN, 0));
	    else
		return (E_DB_OK);
	    break;

	default:
	    /* invalid result length specification */
	    return (adu_error(adf_scb, E_AD2020_BAD_LENSPEC, 2,
		    (i4) sizeof(adi_lenspec->adi_lncompute),
		    (i4 *) &adi_lenspec->adi_lncompute));
    }				    /* end of switch statement */

    if (*adi_rlen == ADE_LEN_UNKNOWN)
	return (adu_error(adf_scb, E_AD2022_UNKNOWN_LEN, 0));

    if (nullable)   /* If either input is nullable, assume output is */
	(*adi_rlen)++;

    return (E_DB_OK);
}


/*{
** Name: adi_0calclen() - Calculate result length from LENSPEC.
**
** Description:
**	This routine calculates the length and precision of a result operand
**	given the datatypes, precisions, and lengths of the input operand(s),
**	the datatype of the output operand, and an ADI_LENSPEC.
**
**	If any of the input operand lengths are ADE_LEN_UNKNOWN, this implies
**	that the length is not currently known.  This may or may not result in
**	the E_AD2022_UNKNOWN_LEN error status depending on whether the length
**	of that particular operand is important or not to the length spec.
**	Obviously, if the length spec says that the result is a fixed length
**	and precision, then none of the input operands' lengths or precisions
**	will matter.
**
**	If the input length spec is ADI_RESLEN, meaning that the result length
**	and precision should already be known, then this routine will not set
**	the result length or precision, and will return either
**	E_AD2022_UNKNOWN_LEN or E_AD0000_OK, depending on whether the result
**	length supplied was ADE_LEN_UNKNOWN or not.  This special length spec
**	is currently only used for the coercion function instances that coerce
**	from a datatype into the same datatype.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	    .adf_maxstring		Max user size of any INGRES string.
**      adi_lenspec                     Ptr to the ADI_LENSPEC which tells how
**                                      to calculate the result length based on
**                                      the input lengths.
**      adi_dv1                         Ptr to DB_DATA_VALUE for first input
**                                      operand.  (Only used as a convenient
**                                      mechanism for supplying a datatype and
**                                      length.) 
**	    .db_datatype		Datatype for first input operand.
**	    .db_prec			Precision/Scale for first input operand
**	    .db_length  		Length of first input operand, or
**                                      ADE_LEN_UNKNOWN if unknown.
**	    .db_data			A pointer to the data for first input
**					operand (currently not used).
**      adi_dv2                         Ptr to DB_DATA_VALUE for second input
**                                      operand.  (Only used as a convenient
**                                      mechanism for supplying a datatype and
**                                      length.) 
**	    .db_datatype		Datatype for second input operand.
**	    .db_prec			Precision/Scale for second input operand
**	    .db_length  		Length of second input operand, or
**                                      ADE_LEN_UNKNOWN if unknown.
**	    .db_data			A pointer to the data for second input
**					operand (currently used only by the
**					ADI_DECFUNC lenspec).
**      adi_dvr                         Ptr to DB_DATA_VALUE for result, or
**                                      output perand.
**	    .db_datatype		Datatype for output operand.
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
**      adi_dvr
**	    .db_prec			Result precision/scale
**	    .db_length  		Result length.
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
**	    E_AD0000_OK			Operation succeeded.
**	    E_AD2005_BAD_DTLEN		Bad length for a datatype.
**	    E_AD2020_BAD_LENSPEC	Unknown length spec.
**	    E_AD2021_BAD_DT_FOR_PRINT	Bad datatype for the print style
**					length specs.
**          E_AD2022_UNKNOWN_LEN	Result length depends on the length
**					of a data value you have not supplied.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	20-jun-86 (thurston)
**          Initial creation.
**	01-jul-86 (thurston)
**	    Added code for the ADI_RESLEN length spec.
**	02-jul-86 (thurston)
**	    Changed the type of argument adi_rlen to be a ptr to an i4 instead
**	    of a ptr to a nat.  This makes it consistent with the .db_length
**	    field of a DB_DATA_VALUE.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	22-sep-86 (thurston)
**	    Added a fix for the case where the lenspec is { ADI_FIXED, 0 } so
**	    that this routine returns E_DB_OK instead of E_AD2022_UNKNOWN_LEN.
**	30-sep-86 (thurston)
**	    Made a fix that enables the caller to use NULL pointers for adi_dv1
**	    and/or adi_dv2 if they are not needed.
**	15-oct-86 (thurston)
**	    Added code for the ADI_SUMT length spec.
**	11-nov-86 (thurston)
**	    Added code for the ADI_P2D2, ADI_2XP2, and ADI_2XM2 length specs.
**	20-nov-86 (thurston)
**	    Added the length spec parameter to the adu_error() call for ADF
**	    error E_AD2020_BAD_LENSPEC.
**	28-dec-86 (thurston)
**	    Fixed adi_calclen() so that it now uses the ADE_LEN_UNKNOWN constant
**	    to detect unknown lengths.
**	22-feb-87 (thurston)
**	    Added support for nullable datatypes.
**	20-mar-87 (thurston)
**	    Fixed bug with datatype ID of zero (an illegal datatype) that was
**	    being treated as a nullable, thus throwing off some length calc's.
**	04-may-87 (thurston)
**	    Made many changes to lenspec code calculations so as to avoid
**	    creating intermediate values that are too long.
**	05-jun-87 (thurston)
**	    Changed calling sequence.  Formerly, the datatype of the output was
**	    not supplied, and the result length was returned in an i4.  However,
**	    there are cases where the result type is needed (esp. for nullable
**	    datatypes), so now a ptr to the result DB_DATA_VALUE is supplied
**	    with the datatype set, and the result length is returned in it.
**	03-feb-88 (thurston)
**	    Modified the code at the bottom of this routine to set the result
**	    length to ADE_LEN_UNKNOWN if the E_AD2022_UNKNOWN_LEN error code is
**	    returned.
**	08-feb-88 (thurston)
**	    The ADI_CSUM, ADI_CTSUM, and ADI_O1TC lenspec now assures the result
**	    will be at least 1.
**	01-sep-88 (thurston)
**	    Changed the use of DB_CHAR_MAX and DB_MAXSTRING to
**	    adf_scb->adf_maxstring, since this is now a session settable
**	    parameter.
**	27-jun-89 (jrb)
**	    Fixed problem where we were decrementing the constant value
**	    ADE_LEN_UNKNOWN whenever the datatype was nullable.  Of course, this
**	    value should not be modified!
**	12-jul-89 (jrb)
**	    Modified all lenspecs to return precision/scale as well as length.
**	    Added decimal lenspecs.
**	25-jul-89 (jrb)
**	    Added a new lenspec for the decimal() conversion function.  This
**	    mandated a change to this routine's interface because we need to
**	    look at the value of the 2nd parameter to derive the result length
**	    for this function.
**      05-apr-91 (jrb)
**          ADI_2XP2 didn't limit the result length to adf_maxstring as it
**          should have (this is curious since ADI_2XM2 did, and it's almost
**          identical!).
**	05-oct-1992 (fred)
**	    Added char <-> bit string length calculations.
**      08-dec-1992 (stevet)
**          Added support for ADI_LEN_EXTERN and ADI_LEN_INDIRECT.  
**          ADI_LEN_INDIRECT allows functions using DB_ALL_TYPE to
**          map various input datatypes to result length.  ADI_LEN_EXTERN
**          allow UDT function to specify an LENSPEC function to 
**          run to calculate result length.
**	02-jul-97 (i4jo01)
**	    Modified formula for calculating decimal scale output length.
**	    Must allow for integral digits to be displayed in the case
**	    where the scale combines to be greater than DB_MAX_DECPREC
**	    but there are still integral digits which need to be displayed.
**	31-jul-1998 (somsa01)
**	    In the case of long varchar or long varbyte inputs to functions,
**	    the length which is in adi_dv1->db_length is actually the length
**	    of the coupon. Therefore, the length of the result should be
**	    the maximum allowed for the session.  (Bug #74162)
**	04-nov-1998 (somsa01 for natjo01)
**	    Modified the scale calculation for DECIDIV. Need to allow for
**	    leading integral digits. (b91609).
**	22-Feb-1999 (shero03)
**	    Support 4 operands 
**      13-Apr-1999 (hanal04)
**          Prevent SIGSEGV by checking adi_dv2->db_data is not NULL
**          before trying to copy the contents. b96344. 
**      30-sep-99 (inkdo01)
**          Minor correction to length computation for X2P2 (for unhex func.)
**      28-dec-2001 (zhahu02)
**         Updated rs with case ADI_DECIDIV (b98986). 
**      12-apr-02 (gupsh01)
**          Added new length specification cases for coercion from unicode
**          nchar and nvarchar types to character types char and varchar.
**	    Also changed the min length returned for the ADI_O1TC case to 2.
**	01-aug-02 (gupsh01)
**	    Change the minimum length returned by the ADI_O1TC from 2 to 1 as 
**	    this prevents from using like select call with just '%' character.
**      15-Aug-2002 (huazh01)
**          Use max(DB_MAX_DECPREC - p1 + s1 - s2, min(s1, s2)) to
**          determine the result scale for decimal division.
**          Bug 108520 INGSRV 1861.
**	23-oct-2002 (drivi01)
**	    Added an if loop to ADI_DECMUL, ADI_DECIMUL, ADI_IDECMUL that 
**	    calculates scale using different formula in case if result scale 
**	    is too high and percision - scale doesn't leave enough space for
**	    digits to the left of the decimal point in the product causing an
**	    overflow error.
**	18-nov-2002 (drivi01)
**	    Made additional change to my last change to prevent possible negative
**	    numbers for scale when precision for two decimals multiplied is too
**	    high.
**	25-nov-02 (inkdo01)
**	    Update CTOU to incorporate expansion space for normalization.
**	19-nov-02 (drivi01)
**	    Backed out changes #460603,460686,460173 due to disagreement between 
**	    Development and Level2 Support on decimal functionality.    
**	12-apr-04 (inkdo01)
**	    Added BIGINT support. Also added ADI_4ORLONGER to support bigint
**	    result of integer ops.
**	12-may-04 (inkdo01)
**	    Further support of bigint (decimal coercions).
**	18-may-2004 (gupsh01)
**	    Added length calculation of nchar, nvarchar functions for integer,
**	    float and decimal input. 
**	20-dec-04 (inkdo01)
**	    Added ADI_CWTOVB for collation_weight() support.
**      08-jun-2006 (huazh01)
**          add codes to decide the return value for adi_dvr->db_collID.
**          this fixes b116215.
**	10-oct-2005 (gupsh01)
**	    Fixed ADI_VARUTOC case. It did not take care of nvarchar DB_CNTSIZE
**	    length. 
**	25-Apr-2006 (kschendel)
**	    Turns out that to make multi-param UDF's useful, we need countvec
**	    style lenspec calls.  Implement them here.
**      08-jun-2006 (huazh01)
**          add codes to decide the return value for adi_dvr->db_collID.
**          this fixes b116215.
**	3-july-06 (dougi)
**	    Added ADI_CTO48 for coercions from strings to int/float to set 
**	    length to 4 or 8 depending on the size of the input.
**	31-jul-2006 (kibro01) Bug 116433
**	    Backed out change 431041 (original bug 83402) which caused a
**	    precision of (p1+p2)%31 as opposed to documented behaviour
**	    or min(p1+p2,31).
**	13-sep-2006 (dougi)
**	    Dropped ADI_CTO48 alonmg with string/numeric coercions.
**	25-oct-2006 (gupsh01)
**	    Fixed the length calculation for date to date coercion and
**	    added new result length calculation directive ADI_DATE2DATE.
**	26-oct-2006 (gupsh01)
**	    Rearrange ADI_DATE2DATE case.
**	7-feb-2007 (dougi)
**	    Change ADI_FIXED to assign result precision 6 for time/timestamp
**	    result types.
**	7-feb-2007 (gupsh01)
**	    Modify the precision value from 6 to 9, as 9 is maximum 
**	    supported value for precision. 
**	8-mar-2007 (dougi)
**	    Add support for ADI_DECROUND (for round() function).
**	13-apr-2007 (dougi)
**	    Add support for ADI_DECCEILFL (for ceil(), floor() functions).
**	13-jul-2007 (gupsh01)
**	    Added ADI_O1UNIDBL for length calculation of unicode upper
**	    lower operations.
**	29-jun-2007 (gupsh01)
**	    Fixed the result length calculation for date->date conversion.
**	23-jul-2007 (smeke01) b118798.
**	   If the client session cannot cope with i8 (8-bytes width) integer 
**	   results then reduce the result size to 4-bytes width. 
**      27-aug-2007 (huazh01)
**          Initialize 'local_lenspec'. Uninitialized random value
**          may cause server return wrong result.
**          Bug 117998.
**	14-Feb-2007 (kiria01) b119902
**	    In 2 param coercion consider the null byte in length checks but
**	    don't add in directly as it will be done later in the generic code.
**	12-Apr-2008 (kiria0) b119410
**	    Added ADI_MINADD1 (At least 'fixed bytes' and then add 1 extra.
**	15-apr-2008 (gupsh01)
**	    In a UTF8 installs make sure that the length is calculated based
**	    on max value allowed for UTF8 character lengths. Bug 120215.
**	01-Oct-2008 (gupsh01)
**	    For UTF8 installations ensure that the length value does not 
**	    exceed maxstring/2 for char/varchar types.
**	27-Oct-2008 (kiria01) SIR120473
**	    Added ADI_PATCOMP & ADI_PATCOMPU to estimate compiled pattern
**	    length.
**      12-May-2009 (hanal04) Bug 122031
**          For UTF8 installations non-unicode input into collation_weight()
**          will be coerced to unicode by adu_collweight() and passed to
**          adu_ucollweight(). Make sure the result length takes this into account.
**	19-Feb-2010 (kiria01) b123308
**	    Moved the ADI_DECBLEND functionality into ADI_LONGER so that
**	    decimals can be catered for better. It is worth noting that
**	    ADI_LONGER and ADI_SHORTER are amongst the modifers that require
**	    BOP operands to match in datatype (though not necessarily in prec
**	    or length).
**	13-May-2010 (gupsh01) 
**	    Fixed the minimum length for an nchar/nvarchar data type. This length
**	    should not be at least sizeof(UCS2) not 1.
*/

# ifdef ADF_BUILD_WITH_PROTOS
DB_STATUS
adi_0calclen(
ADF_CB             *adf_scb,
ADI_LENSPEC        *adi_lenspec,
i4		   adi_numops,
DB_DATA_VALUE      *adi_dv[ADI_MAX_OPERANDS],
DB_DATA_VALUE      *adi_dvr)
# else
DB_STATUS
adi_0calclen( adf_scb, adi_lenspec, adi_numops, adi_dv, adi_dvr)
ADF_CB             *adf_scb;
ADI_LENSPEC        *adi_lenspec;
i4		   adi_numops;
DB_DATA_VALUE      *adi_dv[ADI_MAX_OPERANDS];
DB_DATA_VALUE      *adi_dvr;
# endif
{
    i4			len[ADI_MAX_OPERANDS];
    register i4		extra;
    register i4		rlen;
    DB_DT_ID		dt1;
    DB_STATUS           status;
    DB_STATUS           (*func)();
    ADI_FI_DESC         *fi_desc;
    ADI_LENSPEC         local_lenspec;
    i4			prec[ADI_MAX_OPERANDS];
    i4			rprec;
    i4			p1;
    i4			s1;
    i4			p2;
    i4			s2;
    i4			rp;
    i4			rs;
    i4			i;
    PTR			data[ADI_MAX_OPERANDS];
    bool		nullable = FALSE;
    i4 			min_length;
    
    if (abs(adi_dvr->db_datatype == DB_NVCHR_TYPE) || (adi_dvr->db_datatype == DB_NCHR_TYPE))
        min_length = sizeof(UCS2);
    else
        min_length = sizeof(char);


    MEfill(sizeof(ADI_LENSPEC), (u_char) 0, (char *)&local_lenspec);

    /* Set appropriate length for each operand */
    for (i = 0; i < ADI_MAX_OPERANDS; i++)
    {	
        if (i >= adi_numops
	    ||  adi_dv[i] == NULL
	    ||  adi_dv[i]->db_datatype == 0
	    ||  adi_dv[i]->db_length == ADE_LEN_UNKNOWN )
        {
	    len[i] = ADE_LEN_UNKNOWN;
        }
        else
        {
	    prec[i] = adi_dv[i]->db_prec;
	    data[i] = adi_dv[i]->db_data;

	    if (adi_dv[i]->db_datatype > 0)
	        len[i] = adi_dv[i]->db_length;
	    else
	        len[i] = adi_dv[i]->db_length - 1;
        }
    }

    switch(adi_lenspec->adi_lncompute)
    {
    case ADI_LEN_EXTERN:
    case ADI_LEN_EXTERN_COUNTVEC:
    case ADI_LEN_INDIRECT:

	status = adi_fidesc( adf_scb, (ADI_FI_ID)adi_lenspec->adi_fixedsize,
			     &fi_desc);

	if( status == E_DB_OK)
	{
	    switch(adi_lenspec->adi_lncompute)
	    {
	    case ADI_LEN_EXTERN:
		/* 
		** Call external function to get lenspec
		*/
        
		func = Adf_globs->Adi_fi_lkup[
				ADI_LK_MAP_MACRO(fi_desc->adi_finstid)
					    ].adi_agbgn;
		return( (*func)( adf_scb, fi_desc->adi_fiopid, adi_dv[0], 
				 adi_dv[1], adi_dvr));

	    case ADI_LEN_EXTERN_COUNTVEC:
		{
		    DB_DATA_VALUE *dvpp[ADI_MAX_OPERANDS+1];
		    i4 i;
    
		    /* Like EXTERN, but with counted vector for multi params.
		    ** Set up 0'th entry to be the result dbdv.
		    */
    
		    dvpp[0] = adi_dvr;
		    for (i = 0; i < adi_numops; ++i)
			dvpp[i+1] = adi_dv[i];
    
		    func = Adf_globs->Adi_fi_lkup[
				    ADI_LK_MAP_MACRO(fi_desc->adi_finstid)
						].adi_agbgn;
		    return ((*func)(adf_scb, fi_desc->adi_fiopid,
				    adi_numops+1, &dvpp[0]));
		}
		break;
	    case ADI_LEN_INDIRECT:
		status = adi_1len_indirect( fi_desc->adi_fiopid, 
					   adi_dv[0], adi_dv[1], 
					   &local_lenspec);
	    }
	}
	if( status != E_DB_OK)
	    return (adu_error(adf_scb, E_AD2020_BAD_LENSPEC, 2,
		    (i4) sizeof(adi_lenspec->adi_fixedsize),
		    (i4 *) &adi_lenspec->adi_fixedsize));
        break;

    default:
	STRUCT_ASSIGN_MACRO( *adi_lenspec, local_lenspec);
    }

    /* Now switch on LENSPEC */
    switch (local_lenspec.adi_lncompute)
    {
	case ADI_O1:
	    /* Length of arg #1 */
	    rprec = prec[0];
	    rlen = len[0];
	    break;

	case ADI_O1UNIDBL:
	    /* ADI_O1UNIDBL Length of arg #1 * 2 */
	    if (len[0] == ADE_LEN_UNKNOWN)
	    {
		/* can't be DECIMAL */
		rprec = 0;
		rlen = ADE_LEN_UNKNOWN;
	    }
	    else
	    {
		rprec = prec[0];
		rlen = len[0]*2;
	    }
	    break;

	case ADI_O1UNORM:
	{
	    if (len[0] == ADE_LEN_UNKNOWN)
	    {
		rprec = 0;
		rlen = ADE_LEN_UNKNOWN;
	    }
	    else
	    {
	      int dtype = abs(adi_dv[0]->db_datatype);
	      int templen = 0;

	      rprec = prec[0];

	      if ((dtype == DB_VCH_TYPE) || 
		  (dtype == DB_TXT_TYPE) || 
		  (dtype == DB_NVCHR_TYPE))
		templen = DB_CNTSIZE;
				
	      rlen = templen;
	      if (len[0] < templen)
	      {
		  return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 
				2, 0, "unorm user length"));
	      }
	      else 
	      {
		switch (dtype)
		{
		  case DB_CHA_TYPE:
		  case DB_CHR_TYPE:
		    rlen = (len[0] * DB_MAXUTF8_EXP_FACTOR);
		    if ((adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
		        (rlen > (adf_scb->adf_maxstring/2)))
		      rlen = adf_scb->adf_maxstring/2;
		    else if (rlen > (adf_scb->adf_maxstring))
		      rlen = adf_scb->adf_maxstring;
		    break;
		  case DB_VCH_TYPE:
		  case DB_TXT_TYPE:
		    rlen = ((len[0] - DB_CNTSIZE) 
				* DB_MAXUTF8_EXP_FACTOR) + DB_CNTSIZE;
		    if ((adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
		        (rlen > (adf_scb->adf_maxstring/2 + DB_CNTSIZE)))
		      rlen = (adf_scb->adf_maxstring/2 + DB_CNTSIZE);
		    else if (rlen > (adf_scb->adf_maxstring + DB_CNTSIZE)) 
		      rlen = (adf_scb->adf_maxstring + DB_CNTSIZE);
		    break;
		  case DB_NCHR_TYPE:
		    rlen = (len[0] * DB_MAXUTF16_EXP_FACTOR);
		    if (rlen > DB_MAXSTRING) 
		      rlen = DB_MAXSTRING;
		    break;
		  case DB_NVCHR_TYPE:
		    rlen = ((len[0] - DB_CNTSIZE) 
				* DB_MAXUTF16_EXP_FACTOR) + DB_CNTSIZE;
		    if (rlen > (DB_MAXSTRING + DB_CNTSIZE)) 
		      rlen = (DB_MAXSTRING + DB_CNTSIZE);
		    break;
		  default: 
	    	    return (adu_error(adf_scb, E_AD2020_BAD_LENSPEC, 2,
		      (i4) sizeof(adi_lenspec->adi_fixedsize),
		      (i4 *) &adi_lenspec->adi_fixedsize));
		    break;
		} 
	      }
	    }
	    break;
	}

	case ADI_O2:
	    /* Length of arg #2 */
	    rprec = prec[1];
	    rlen = len[1];
	    break;

	case ADI_LONGER:
	    /* Length is longer of the args */
	    if (len[0] == ADE_LEN_UNKNOWN || len[1] == ADE_LEN_UNKNOWN)
	    {
		/* can't be DECIMAL */
		rprec = 0;
		rlen = ADE_LEN_UNKNOWN;
	    }
	    else
	    {
		if (abs(adi_dv[0]->db_datatype) == DB_DEC_TYPE)
		{
		    /*
		    ** For blending two decimal values together. Simply
		    ** maxing the length is not enough.
		    */
		    p1 = DB_P_DECODE_MACRO(prec[0]);
		    s1 = DB_S_DECODE_MACRO(prec[0]);
		    p2 = DB_P_DECODE_MACRO(prec[1]);
		    s2 = DB_S_DECODE_MACRO(prec[1]);
		    rp = min(DB_MAX_DECPREC, max(p1-s1, p2-s2) + max(s1, s2));
		    rs = max(s1, s2);
		    rprec = DB_PS_ENCODE_MACRO(rp, rs);
		    rlen = DB_PREC_TO_LEN_MACRO(rp);
		}
		else if (len[0] > len[1])
		{
		    rprec = prec[0];
		    rlen = len[0];
		}
		else
		{
		    rprec = prec[1];
		    rlen = len[1];
		}
	    }
	    break;

	case ADI_SHORTER:
	    /* Length is shorter of the args */
	    if (len[0] == ADE_LEN_UNKNOWN || len[1] == ADE_LEN_UNKNOWN)
	    {
		/* can't be DECIMAL */
		rprec = 0;
		rlen = ADE_LEN_UNKNOWN;
	    }
	    else
	    {
		if (len[0] < len[1])
		{
		    rprec = prec[0];
		    rlen = len[0];
		}
		else
		{
		    rprec = prec[1];
		    rlen = len[1];
		}
	    }
	    break;

	case ADI_4ORLONGER:
	    /* Length is 4 or the longer of the args - whichever 
	    ** is longer (for int ops with bigint args). */
	    rprec = local_lenspec.adi_psfixed;
	    rlen = 4;	/* (to start) */
	    if (len[0] > rlen)
		rlen = len[0];
	    if (adi_numops > 1 && len[1] > rlen)
		rlen = len[1];
	    /* 
	    ** If the client cannot cope with i8 (8-bytes width) 
	    ** integer results then reduce to 4-bytes width. b118798.
	    **/
	    if ( !(adf_scb->adf_proto_level & AD_I8_PROTO) && (abs(adi_dvr->db_datatype) == DB_INT_TYPE) )
	    {
		rlen = 4;
	    }

	    break;

	case ADI_IMULI:
	case ADI_IADDI:
	case ADI_ISUBI:
	case ADI_IDIVI:
	    /* 
	    ** Length is 8 or 4 depending on what the session can cope with 
	    */

	    rprec = local_lenspec.adi_psfixed;

	    /* 
	    ** If the client cannot cope with i8 (8-bytes width) 
	    ** integer results then reduce to 4-bytes width. b118798.
	    */
	    if (adf_scb->adf_proto_level & AD_I8_PROTO)
		rlen = 8;
	    else
		rlen = 4;

	    break;

	case ADI_DIFFERENCE:
	    /* length of arg #1 - length of arg #2 */
	    rprec = 0;
	    if (len[0] == ADE_LEN_UNKNOWN || len[1] == ADE_LEN_UNKNOWN)
		rlen = ADE_LEN_UNKNOWN;
	    else
		rlen = len[0] - len[1];
	    break;

	case ADI_CSUM:
	    /* length of arg #1 + length of arg #2,
	    ** but not greater than DB_CHAR_MAX, and not less than 1
	    */
	    rprec = 0;
	    if (len[0] == ADE_LEN_UNKNOWN || len[1] == ADE_LEN_UNKNOWN)
		rlen = ADE_LEN_UNKNOWN;
	    else
		if ((adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) && 
		    (abs(adi_dv[0]->db_datatype) != DB_BYTE_TYPE))
		  rlen = min(DB_UTF8_MAXSTRING, max(min_length, (len[0] + len[1])));
		else
		  rlen = min(DB_CHAR_MAX, max(min_length, (len[0] + len[1])));
	    break;

	case ADI_SUMV:
	    /* length of arg #1 + length of arg #2,
	    ** but not greater than maxstring + DB_CNTSIZE
	    */
	    rprec = 0;
	    if (len[0] == ADE_LEN_UNKNOWN || len[1] == ADE_LEN_UNKNOWN)
		rlen = ADE_LEN_UNKNOWN;
	    else
		if ((adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) && 
		    (abs(adi_dv[0]->db_datatype) != DB_VBYTE_TYPE))
		  rlen = min((adf_scb->adf_maxstring/2 + DB_CNTSIZE),
			   (len[0] + len[1]));
		else
		  rlen = min((adf_scb->adf_maxstring + DB_CNTSIZE),
			   (len[0] + len[1]));
	    break;

	case ADI_TSUM:
	    /* length of arg #1 + length of arg #2 - DB_CNTSIZE,
	    ** but not greater than maxstring + DB_CNTSIZE
	    */
	    rprec = 0;
	    if (len[0] == ADE_LEN_UNKNOWN || len[1] == ADE_LEN_UNKNOWN)
		rlen = ADE_LEN_UNKNOWN;
	    else
	        if ((adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
                    (abs(adi_dv[0]->db_datatype) != DB_VBYTE_TYPE))
		  rlen = min((adf_scb->adf_maxstring/2 + DB_CNTSIZE),
				((len[0] + len[1]) - DB_CNTSIZE));
		else
		  rlen = min((adf_scb->adf_maxstring + DB_CNTSIZE),
				((len[0] + len[1]) - DB_CNTSIZE));
	    break;

	case ADI_CTSUM:
	    /* length of arg #1 + length of arg #2 - DB_CNTSIZE,
	    ** but not greater than DB_CHAR_MAX, and not less than 1
	    */
	    rprec = 0;
	    if (len[0] == ADE_LEN_UNKNOWN || len[1] == ADE_LEN_UNKNOWN)
		rlen = ADE_LEN_UNKNOWN;
	    else
		if (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED)
		  rlen = min(DB_UTF8_MAXSTRING, max(min_length, ((len[0] + len[1]) - DB_CNTSIZE)));
		else
		  rlen = min(DB_CHAR_MAX, max(min_length, ((len[0] + len[1]) - DB_CNTSIZE)));
	    break;

	case ADI_SUMT:
	    /* length of arg #1 + length of arg #2 + DB_CNTSIZE */
	    rprec = 0;
	    if (len[0] == ADE_LEN_UNKNOWN || len[1] == ADE_LEN_UNKNOWN)
		rlen = ADE_LEN_UNKNOWN;
	    else
		rlen = len[0] + len[1] + DB_CNTSIZE;
	    break;

	case ADI_FIXED:
	    /* fixed length */
	    rprec = local_lenspec.adi_psfixed;
	    rlen  = local_lenspec.adi_fixedsize;

	    /* Set precision to default 6 for new time types. */
	    if (rprec == 0)
        	switch (abs(adi_dvr->db_datatype))
        	{
                 case DB_TMWO_TYPE:
                 case DB_TMW_TYPE:
                 case DB_TME_TYPE:

                 case DB_TSWO_TYPE:
                 case DB_TSW_TYPE:
                 case DB_TSTMP_TYPE:

                 case DB_INDS_TYPE:
                   rprec = 9;
                 break;
		}
	    break;

	case ADI_TPRINT:    /* add DB_CNTSIZE */
	case ADI_PRINT:	    /* add nothing for "c" format */
	case ADI_NPRINT:    /* add nothing for nchar */
	case ADI_NTPRINT:   /* add DB_CNTSIZE for nvarchar */
	    /* Result length for both of these LENSPEC's is determined by
            ** the default print characteristics of arg #1's datatype.  This
            ** result type is used only by the ASCII function at the present
            ** time.  A precondition to using this LENSPEC is that arg #1's
            ** datatype must be "f" (float), "dec" (decimal), or "i" (int). 
	    */

	    /* will always be a string datatype */
	    dt1 = abs(adi_dv[0]->db_datatype);
	    rprec = 0;

	    switch (local_lenspec.adi_lncompute)
	    {
		case ADI_PRINT:		/* no extra space needed in CHAR type */
		case ADI_NPRINT:		/* no extra space needed in CHAR type */
		    extra = 0;
		    break;

		case ADI_NTPRINT:	
		case ADI_TPRINT:	/* allow extra space for */
		    extra = DB_CNTSIZE;	/* count field in text   */
		    break;
	    }

	    if (dt1 == DB_FLT_TYPE)
	    {
		switch (len[0])
		{
		    case 4:
			rlen = adf_scb->adf_outarg.ad_f4width + extra;
			break;

		    case 8:
			rlen = adf_scb->adf_outarg.ad_f8width + extra;
			break;

		    default:
			return (adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0));
		}

		if (local_lenspec.adi_lncompute == ADI_NPRINT)
		    rlen = rlen * sizeof(UCS2);
		else if (local_lenspec.adi_lncompute == ADI_NTPRINT)
		    rlen = (rlen - extra) * sizeof(UCS2) + extra;
	    }
	    else if (dt1 == DB_INT_TYPE)
	    {
		switch (len[0])
		{
		    case 1:
			rlen = adf_scb->adf_outarg.ad_i1width + extra;
			break;

		    case 2:
			rlen = adf_scb->adf_outarg.ad_i2width + extra;
			break;

		    case 4:
			rlen = adf_scb->adf_outarg.ad_i4width + extra;
			break;

		    case 8:
			rlen = adf_scb->adf_outarg.ad_i8width + extra;
			break;

		    default:
			return (adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0));
		}

		if (local_lenspec.adi_lncompute == ADI_NPRINT)
		    rlen = rlen * sizeof(UCS2);
		else if (local_lenspec.adi_lncompute == ADI_NTPRINT)
		    rlen = (rlen - extra) * sizeof(UCS2) + extra;
	    }
	    else if (dt1 == DB_DEC_TYPE)
	    {
		i4		pr = DB_P_DECODE_MACRO(prec[0]);
		i4		sc = DB_S_DECODE_MACRO(prec[0]);
		
		/* This macro converts precision and scale to display length */
		rlen = AD_PS_TO_PRN_MACRO(pr, sc) + extra;

		if (local_lenspec.adi_lncompute == ADI_NPRINT)
		    rlen = rlen * sizeof(UCS2);
		else if (local_lenspec.adi_lncompute == ADI_NTPRINT)
		    rlen = (rlen - extra) * sizeof(UCS2) + extra;
	    }
	    else
	    {
		/* Who knows what this is... make it a user error */
		return (adu_error(adf_scb, E_AD2021_BAD_DT_FOR_PRINT, 0));
	    }
	    break;

	case ADI_O1CT:
	    /* length of arg #1 + DB_CNTSIZE */
	    rprec = 0;
	    rlen = (len[0] != ADE_LEN_UNKNOWN
				? len[0] + DB_CNTSIZE
				: ADE_LEN_UNKNOWN);
	    break;

	case ADI_O1TC:
	    /* length of arg #1 - DB_CNTSIZE,
	    ** but not greater than DB_CHAR_MAX, and not less than 1
	    */
	    rprec = 0;
	    if (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED)
	      rlen = (len[0] != ADE_LEN_UNKNOWN
				? min(DB_UTF8_MAXSTRING, max(min_length, (len[0] - DB_CNTSIZE)))
				: ADE_LEN_UNKNOWN);
	    else
	      rlen = (len[0] != ADE_LEN_UNKNOWN
				? min(DB_CHAR_MAX, max(min_length, (len[0] - DB_CNTSIZE)))
				: ADE_LEN_UNKNOWN);
	    break;

	case ADI_ADD1:
	    /* length of arg #1 + 1 */
	    rprec = 0;
	    rlen = (len[0] != ADE_LEN_UNKNOWN
				? len[0] + 1
				: ADE_LEN_UNKNOWN);
	    break;

	case ADI_SUB1:
	    /* length of arg #1 - 1 */
	    rprec = 0;
	    rlen = (len[0] != ADE_LEN_UNKNOWN
				? len[0] - 1
				: ADE_LEN_UNKNOWN);
	    break;

	case ADI_MINADD1:
	    /* length of arg #1 + 1 */
	    rprec = 0;
	    if (len[0] == ADE_LEN_UNKNOWN)
		rlen = len[0];
	    else if (len[0] < local_lenspec.adi_fixedsize)
		rlen = local_lenspec.adi_fixedsize + 1;
	    else
		rlen = len[0] + 1;
	    break;

	case ADI_PATCOMP:
	    /* estimate length of compiling pattern in arg #1 */
	    rprec = 0;
	    if (len[0] == ADE_LEN_UNKNOWN)
		rlen = len[0];
	    else
		rlen = min((adf_scb->adf_maxstring + DB_CNTSIZE),
				      ((len[0] * 4) + DB_CNTSIZE + 36));
	    break;

	case ADI_PATCOMPU:
	    /* estimate length of compiling unicode pattern in arg #1 */
	    rprec = 0;
	    if (len[0] == ADE_LEN_UNKNOWN)
		rlen = len[0];
	    else
		rlen = min((adf_scb->adf_maxstring + DB_CNTSIZE),
				      ((len[0] * 10) + DB_CNTSIZE + 36));	
	    break;

	case ADI_P2D2:
	    /* (length of arg #1 + DB_CNTSIZE) / 2 */
	    rprec = 0;
	    rlen = (len[0] != ADE_LEN_UNKNOWN
				? (len[0] + DB_CNTSIZE) / 2
				: ADE_LEN_UNKNOWN);
	    break;

	case ADI_2XP2:
	    /* (length of arg #1 * 2) + DB_CNTSIZE) */
	    rprec = 0;
	    if (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) 
	      rlen = (len[0] != ADE_LEN_UNKNOWN
				? min((adf_scb->adf_maxstring/2 + DB_CNTSIZE),
				      ((len[0] * 2) + DB_CNTSIZE))
				: ADE_LEN_UNKNOWN);
	    else
	      rlen = (len[0] != ADE_LEN_UNKNOWN
				? min((adf_scb->adf_maxstring + DB_CNTSIZE),
				      ((len[0] * 2) + DB_CNTSIZE))
				: ADE_LEN_UNKNOWN);
	    break;

	case ADI_2XM2:
	    /* (length of arg #1 * 2) - DB_CNTSIZE,
	    ** but not greater than maxstring + DB_CNTSIZE
	    */
	    rprec = 0;
	    if (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) 
	      rlen = (len[0] != ADE_LEN_UNKNOWN
				? min((adf_scb->adf_maxstring/2 + DB_CNTSIZE),
				      ((len[0] * 2) - DB_CNTSIZE))
				: ADE_LEN_UNKNOWN);
	    else
	      rlen = (len[0] != ADE_LEN_UNKNOWN
				? min((adf_scb->adf_maxstring + DB_CNTSIZE),
				      ((len[0] * 2) - DB_CNTSIZE))
				: ADE_LEN_UNKNOWN);
	    break;

	case ADI_X2P2:
	    dt1 = abs(adi_dv[0]->db_datatype);
	    /* Remove var. length before computing result length. */
	    extra = (len[0] != ADE_LEN_UNKNOWN && (dt1 == DB_VCH_TYPE || 
		dt1 == DB_TXT_TYPE || dt1 == DB_VBYTE_TYPE)) ? 2 : 0;
	    /* (length of arg #1 / 2) + DB_CNTSIZE */
	    rprec = 0;
	    rlen = (len[0] != ADE_LEN_UNKNOWN
				? ((len[0] - extra)/ 2) + DB_CNTSIZE
				: ADE_LEN_UNKNOWN);
	    break;

	case ADI_RSLNO1CT:
	    /* length should already be known ... if it is, just return.
	    ** If not, set result length to 1st operand's length.
	    */
	    if (adi_dvr->db_length == ADE_LEN_UNKNOWN)
	    {
		return (adu_error(adf_scb, E_AD2022_UNKNOWN_LEN, 0));
	    }
	    else if (	adi_dvr->db_length < DB_CNTSIZE
		     || 
	    	        ((adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) && 
			    (adi_dvr->db_length > adf_scb->adf_maxstring/2 + DB_CNTSIZE))
		     || 
			(adi_dvr->db_length > adf_scb->adf_maxstring + DB_CNTSIZE)
		    )
	    {
		rprec = 0;
		rlen = (len[0] != ADE_LEN_UNKNOWN
				    ? len[0] + DB_CNTSIZE
				    : ADE_LEN_UNKNOWN);
	    }
	    else
	    {
		return (E_DB_OK);
	    }
	    break;

	case ADI_RESLEN:
	    /* length and precicion should already be known ... just return */
	    if (adi_dvr->db_length == ADE_LEN_UNKNOWN)
		return (adu_error(adf_scb, E_AD2022_UNKNOWN_LEN, 0));
	    else
		return (E_DB_OK);
	    break;

	case ADI_DECADD:
	    /*
	    ** This lenspec is for adding/subtracting  one decimal number
	    ** to/from another.  The formulas were obtained from the DB2 spec
	    ** and if changed will necessitate changes to the CL code as well.
	    */
	    p1 = DB_P_DECODE_MACRO(prec[0]);
	    s1 = DB_S_DECODE_MACRO(prec[0]);
	    p2 = DB_P_DECODE_MACRO(prec[1]);
	    s2 = DB_S_DECODE_MACRO(prec[1]);
	    rp = min(DB_MAX_DECPREC, max(p1-s1, p2-s2) + max(s1, s2) + 1);
	    rs = max(s1, s2);
	    rprec = DB_PS_ENCODE_MACRO(rp, rs);
	    rlen = DB_PREC_TO_LEN_MACRO(rp);
	    break;

	case ADI_DECMUL:
	    /*
	    ** This lenspec is for multiplying one decimal number by another.
	    ** The formulas were obtained from the DB2 spec and if changed
	    ** will necessitate changes to the CL code as well.
	    */
	    p1 = DB_P_DECODE_MACRO(prec[0]);
	    s1 = DB_S_DECODE_MACRO(prec[0]);
	    p2 = DB_P_DECODE_MACRO(prec[1]);
	    s2 = DB_S_DECODE_MACRO(prec[1]);
	    rp = min(DB_MAX_DECPREC, p1+p2);
	    rs = min(DB_MAX_DECPREC, (s1+s2));
	    rprec = DB_PS_ENCODE_MACRO(rp, rs);
	    rlen = DB_PREC_TO_LEN_MACRO(rp);
	    break;

	case ADI_DECIMUL:
	    /*
	    ** This lenspec is for multiplying a decimal number by an int.
	    ** The formulas were obtained from the decimal product spec and
	    ** changes to them will necessitate changes to the CL code as well.
	    */
	    p1 = DB_P_DECODE_MACRO(prec[0]);
	    s1 = DB_S_DECODE_MACRO(prec[0]);
	    switch (len[1])
	    {
	      case 1:
		p2 = AD_I1_TO_DEC_PREC;
		s2 = AD_I1_TO_DEC_SCALE;
		break;
	      case 2:
		p2 = AD_I2_TO_DEC_PREC;
		s2 = AD_I2_TO_DEC_SCALE;
		break;
	      case 4:
		p2 = AD_I4_TO_DEC_PREC;
		s2 = AD_I4_TO_DEC_SCALE;
		break;
	      case 8:
		p2 = AD_I8_TO_DEC_PREC;
		s2 = AD_I8_TO_DEC_SCALE;
		break;
	    }
	    rp = min(DB_MAX_DECPREC, p1+p2);
	    rs = min(DB_MAX_DECPREC, s1+s2);	    
	    rprec = DB_PS_ENCODE_MACRO(rp, rs);
	    rlen = DB_PREC_TO_LEN_MACRO(rp);
	    break;

	case ADI_IDECMUL:
	    /*
	    ** This lenspec is for multiplying an int by a decimal number.
	    ** The formulas were obtained from the decimal product spec and
	    ** changes to them will necessitate changes to the CL code as well.
	    */
	    switch (len[0])
	    {
	      case 1:
		p1 = AD_I1_TO_DEC_PREC;
		s1 = AD_I1_TO_DEC_SCALE;
		break;
	      case 2:
		p1 = AD_I2_TO_DEC_PREC;
		s1 = AD_I2_TO_DEC_SCALE;
		break;
	      case 4:
		p1 = AD_I4_TO_DEC_PREC;
		s1 = AD_I4_TO_DEC_SCALE;
		break;
	      case 8:
		p1 = AD_I8_TO_DEC_PREC;
		s1 = AD_I8_TO_DEC_SCALE;
		break;
	    }
	    p2 = DB_P_DECODE_MACRO(prec[1]);
	    s2 = DB_S_DECODE_MACRO(prec[1]);
	    rp = min(DB_MAX_DECPREC, p1+p2);
	    rs = min(DB_MAX_DECPREC, s1+s2);
	    rprec = DB_PS_ENCODE_MACRO(rp, rs);
	    rlen = DB_PREC_TO_LEN_MACRO(rp);
	    break;

	case ADI_DECDIV:
	    /*
	    ** This lenspec is for dividing one decimal number by another.
	    ** The formulas were obtained from the DB2 spec and if changed
	    ** will necessitate changes to the CL code as well.
	    */
	    p1 = DB_P_DECODE_MACRO(prec[0]);
	    s1 = DB_S_DECODE_MACRO(prec[0]);
	    p2 = DB_P_DECODE_MACRO(prec[1]);
	    s2 = DB_S_DECODE_MACRO(prec[1]);
	    rp = DB_MAX_DECPREC;
	    rs = max(DB_MAX_DECPREC - p1 + s1 - s2, min(s1, s2));
	    rprec = DB_PS_ENCODE_MACRO(rp, rs);
	    rlen = DB_PREC_TO_LEN_MACRO(rp);
	    break;

	case ADI_DECIDIV:
	    /*
	    ** This lenspec is for dividing a decimal number by an int.
	    ** The formulas were obtained from the decimal product spec and
	    ** changes to them will necessitate changes to the CL code as well.
	    */
	    p1 = DB_P_DECODE_MACRO(prec[0]);
	    s1 = DB_S_DECODE_MACRO(prec[0]);
	    switch (len[1])
	    {
	      case 1:
		p2 = AD_I1_TO_DEC_PREC;
		s2 = AD_I1_TO_DEC_SCALE;
		break;
	      case 2:
		p2 = AD_I2_TO_DEC_PREC;
		s2 = AD_I2_TO_DEC_SCALE;
		break;
	      case 4:
		p2 = AD_I4_TO_DEC_PREC;
		s2 = AD_I4_TO_DEC_SCALE;
		break;
	      case 8:
		p2 = AD_I8_TO_DEC_PREC;
		s2 = AD_I8_TO_DEC_SCALE;
		break;
	    }
	    rp = DB_MAX_DECPREC;
	    rs = max(DB_MAX_DECPREC - p1 + s1 - s2, min(s1, s2));
	    rprec = DB_PS_ENCODE_MACRO(rp, rs);
	    rlen = DB_PREC_TO_LEN_MACRO(rp);
	    break;

	case ADI_IDECDIV:
	    /*
	    ** This lenspec is for dividing an int by a decimal number.
	    ** The formulas were obtained from the decimal product spec and
	    ** changes to them will necessitate changes to the CL code as well.
	    */
	    switch (len[0])
	    {
	      case 1:
		p1 = AD_I1_TO_DEC_PREC;
		s1 = AD_I1_TO_DEC_SCALE;
		break;
	      case 2:
		p1 = AD_I2_TO_DEC_PREC;
		s1 = AD_I2_TO_DEC_SCALE;
		break;
	      case 4:
		p1 = AD_I4_TO_DEC_PREC;
		s1 = AD_I4_TO_DEC_SCALE;
		break;
	      case 8:
		p1 = AD_I8_TO_DEC_PREC;
		s1 = AD_I8_TO_DEC_SCALE;
		break;
	    }
	    p2 = DB_P_DECODE_MACRO(prec[1]);
	    s2 = DB_S_DECODE_MACRO(prec[1]);
	    rp = DB_MAX_DECPREC;
	    rs = max(DB_MAX_DECPREC - p1 + s1 - s2, min(s1, s2));
	    rprec = DB_PS_ENCODE_MACRO(rp, rs);
	    rlen = DB_PREC_TO_LEN_MACRO(rp);
	    break;

	case ADI_DECAVG:
	    /*
	    ** This lenspec is for the avg() aggregate over decimal values.
	    */
	    p1 = DB_P_DECODE_MACRO(prec[0]);
	    s1 = DB_S_DECODE_MACRO(prec[0]);
	    rp = DB_MAX_DECPREC;
	    rs = min(DB_MAX_DECPREC, s1+1);
	    rprec = DB_PS_ENCODE_MACRO(rp, rs);
	    rlen = DB_PREC_TO_LEN_MACRO(rp);
	    break;

	case ADI_DECSUM:
	    /*
	    ** This lenspec is for the sum() aggregate over decimal values.
	    */
	    s1 = DB_S_DECODE_MACRO(prec[0]);
	    rp = DB_MAX_DECPREC;
	    rs = s1;
	    rprec = DB_PS_ENCODE_MACRO(rp, rs);
	    rlen = DB_PREC_TO_LEN_MACRO(rp);
	    break;

	case ADI_DECINT:
	    /*
	    ** This lenspec is for the integer to decimal coercion
	    */
	    switch (len[0])
	    {
	      case 1:
		rp = AD_I1_TO_DEC_PREC;
		rs = AD_I1_TO_DEC_SCALE;
		break;
	      case 2:
		rp = AD_I2_TO_DEC_PREC;
		rs = AD_I2_TO_DEC_SCALE;
		break;
	      case 4:
		rp = AD_I4_TO_DEC_PREC;
		rs = AD_I4_TO_DEC_SCALE;
		break;
	      case 8:
		rp = AD_I8_TO_DEC_PREC;
		rs = AD_I8_TO_DEC_SCALE;
		break;
	    }
	    rprec = DB_PS_ENCODE_MACRO(rp, rs);
	    rlen = DB_PREC_TO_LEN_MACRO(rp);
	    break;

	case ADI_DECFUNC:
	  /*
	  ** This lenspec is for the decimal() conversion function.  The
	  ** precision and scale are derived from the VALUE of the 2nd
	  ** argument.  Currently this is the only lenspec which requires
	  ** the ability to look at the value (i.e. dereference the db_data
	  ** pointer) of a parameter.
	  */
	    switch (len[1])
	    {
	      case 1:
		rp = DB_P_DECODE_MACRO(*(i1 *)data[1]);
		rs = DB_S_DECODE_MACRO(*(i1 *)data[1]); 
		break;
	      case 2:
		rp = DB_P_DECODE_MACRO(*(i2 *)data[1]);
		rs = DB_S_DECODE_MACRO(*(i2 *)data[1]); 
		break;
	      case 4:
		rp = DB_P_DECODE_MACRO(*(i4 *)data[1]);
		rs = DB_S_DECODE_MACRO(*(i4 *)data[1]); 
		break;
	    }
	    rprec = DB_PS_ENCODE_MACRO(rp, rs);
	    rlen = DB_PREC_TO_LEN_MACRO(rp);
	    break;

	case ADI_DECBLEND:
	    /*
	    ** This is used for blending two decimal values together.  It is
	    ** used for the SQL "union" and for the "ifnull" function.
	    */
	    p1 = DB_P_DECODE_MACRO(prec[0]);
	    s1 = DB_S_DECODE_MACRO(prec[0]);
	    p2 = DB_P_DECODE_MACRO(prec[1]);
	    s2 = DB_S_DECODE_MACRO(prec[1]);
	    rp = min(DB_MAX_DECPREC, max(p1-s1, p2-s2) + max(s1, s2));
	    rs = max(s1, s2);
	    rprec = DB_PS_ENCODE_MACRO(rp, rs);
	    rlen = DB_PREC_TO_LEN_MACRO(rp);
	    break;

	case ADI_DECROUND:
	    /*
	    ** round() adds one to result precision (in case we add a digit).
	    ** That's all there is tuit.
	    */
	    p1 = DB_P_DECODE_MACRO(prec[0]);
	    s1 = DB_S_DECODE_MACRO(prec[0]);
	    if (p1 < DB_MAX_DECPREC)
		rp = p1+1;
	    else rp = p1;
	    rs = s1;
	    rprec = DB_PS_ENCODE_MACRO(rp, rs);
	    rlen = DB_PREC_TO_LEN_MACRO(rp);
	    break;

	case ADI_DECCEILFL:
	    /*
	    ** ceil(), floor() have same precision, but 0 scale for result.
	    */
	    p1 = DB_P_DECODE_MACRO(prec[0]);
	    rp = p1;
	    rs = 0;
	    rprec = DB_PS_ENCODE_MACRO(rp, rs);
	    rlen = DB_PREC_TO_LEN_MACRO(rp);
	    break;

	case ADI_CHAR2BIT:
	{
	    i4	    bytes;
	    i4		    bits;
	    i4		    dt1;
	    i4		    dtr;
	    
	    dt1 = abs(adi_dv[0]->db_datatype);
	    dtr = abs(adi_dvr->db_datatype);
	    
	    if ((dt1 == DB_VCH_TYPE) || (dt1 == DB_LTXT_TYPE))
	    {
		len[0] -= DB_CNTSIZE;
	    }

	    bytes = len[0]/BITSPERBYTE;
	    bits = len[0] % BITSPERBYTE;

	    if (bits)
		bytes += 1;

	    rprec = bits;
	    rlen = bytes;
	    
	    if (dtr == DB_VBIT_TYPE)
		rlen += DB_BCNTSIZE;
	    break;
	}
 
	case ADI_BIT2CHAR:
	{
	    i4	bits;
	    i4		    dt1;
	    i4		    dtr;
	    
	    dt1 = abs(adi_dv[0]->db_datatype);
	    dtr = abs(adi_dvr->db_datatype);
	    

	    if (dt1 == DB_VBIT_TYPE)
		len[0] -= DB_BCNTSIZE;

	    bits = len[0] * BITSPERBYTE;
	    if (prec[0])
		bits = bits - BITSPERBYTE + prec[0];

	    rprec = 0;
	    rlen = bits;
	    if ((dtr == DB_VCH_TYPE) || (dtr == DB_LTXT_TYPE))
	    {
		rlen += DB_CNTSIZE;
	    }
	    break;
	}

	case ADI_COUPON:
	{
	    rprec = 0;

	    if (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED)
	      rlen = adf_scb->adf_maxstring/2;
	    else
	      rlen = adf_scb->adf_maxstring;

	    if ((fi_desc->adi_fiopid == ADI_VARCH_OP) ||
		(fi_desc->adi_fiopid == ADI_TEXT_OP) ||
		(fi_desc->adi_fiopid == ADI_VBYTE_OP))
		rlen += DB_CNTSIZE;
	    break;
	}

        case ADI_CTOU:
        {
            rprec = 0;
	    /* Unicode result includes expansion space for normalization. */
	    if (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED)
              rlen = (len[0] != ADE_LEN_UNKNOWN
                                ? min(DB_UTF8_MAXSTRING, max(2, 4*(len[0])*sizeof(UCS2)))
                                : ADE_LEN_UNKNOWN);
	    else
              rlen = (len[0] != ADE_LEN_UNKNOWN
                                ? min(DB_CHAR_MAX, max(2, 4*(len[0])*sizeof(UCS2)))
                                : ADE_LEN_UNKNOWN);

            break;
        }

        case ADI_UTOC:
        {
            rprec = 0;
	    if (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED)
              rlen = (len[0] != ADE_LEN_UNKNOWN
                                ? min(DB_UTF8_MAXSTRING,
                                      max(2, (((len[0] + 1)/sizeof(UCS2))*3)))
                                : ADE_LEN_UNKNOWN);
	    else
              rlen = (len[0] != ADE_LEN_UNKNOWN
                                ? min(DB_CHAR_MAX,
                                      max(2, (((len[0] + 1)/sizeof(UCS2))*3)))
                                : ADE_LEN_UNKNOWN);

            break;
        }

        case ADI_CTOVARU:
        {
            rprec = 0;
	    if (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED)
              rlen = (len[0] != ADE_LEN_UNKNOWN
                                ? min(DB_UTF8_MAXSTRING,
                                      max(2, (len[0]*sizeof(UCS2) + DB_CNTSIZE)))
                                : ADE_LEN_UNKNOWN);
	    else
              rlen = (len[0] != ADE_LEN_UNKNOWN
                                ? min(DB_CHAR_MAX,
                                      max(2, (len[0]*sizeof(UCS2) + DB_CNTSIZE)))
                                : ADE_LEN_UNKNOWN);

            break;
        }

        case ADI_VARCTOU:
        {
            rprec = 0;
	    if (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED)
              rlen = (len[0] != ADE_LEN_UNKNOWN
                                ? min(DB_UTF8_MAXSTRING,
                                      max(2, (len[0] - DB_CNTSIZE)*sizeof(UCS2)))
                                : ADE_LEN_UNKNOWN);
	    else
              rlen = (len[0] != ADE_LEN_UNKNOWN
                                ? min(DB_CHAR_MAX,
                                      max(2, (len[0] - DB_CNTSIZE)*sizeof(UCS2)))
                                : ADE_LEN_UNKNOWN);
            break;
        }

        case ADI_UTOVARC:
        {
            rprec = 0;
	    if (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED)
              rlen = (len[0] != ADE_LEN_UNKNOWN
                           ? min(DB_UTF8_MAXSTRING,
                              max(2, (((len[0] + 1)/sizeof(UCS2))*3 + DB_CNTSIZE)))
                                : ADE_LEN_UNKNOWN);
	    else
              rlen = (len[0] != ADE_LEN_UNKNOWN
                           ? min(DB_CHAR_MAX,
                              max(2, (((len[0] + 1)/sizeof(UCS2))*3 + DB_CNTSIZE)))
                                : ADE_LEN_UNKNOWN);

            break;
        }

        case ADI_VARUTOC:
        {
            rprec = 0;
	    if (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED)
              rlen = (len[0] != ADE_LEN_UNKNOWN
                           ? min(DB_UTF8_MAXSTRING,
                              max(2, (((len[0] - DB_CNTSIZE + 1)/sizeof(UCS2))*3+ DB_CNTSIZE)))
                                : ADE_LEN_UNKNOWN);
	    else
              rlen = (len[0] != ADE_LEN_UNKNOWN
                           ? min(DB_CHAR_MAX,
                              max(2, (((len[0] - DB_CNTSIZE + 1)/sizeof(UCS2))*3+ DB_CNTSIZE)))
                                : ADE_LEN_UNKNOWN);
            break;
        }

        case ADI_CWTOVB:
        {
	    i4		    dt1;
	    i4		    slen;
	    
	    dt1 = abs(adi_dv[0]->db_datatype);
	    slen = (adi_dv[0]->db_datatype > 0) ? adi_dv[0]->db_length :
		adi_dv[0]->db_length - 1;
	    if (dt1 == DB_VCH_TYPE || dt1 == DB_TXT_TYPE ||
			dt1 == DB_NVCHR_TYPE)
		slen -= DB_CNTSIZE;

            rprec = 0;
            /* For UTF8 installs non-unicode types will be coerced and call adu_ucollweight() */
	    if (dt1 == DB_NCHR_TYPE || dt1 == DB_NVCHR_TYPE || (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED))
	    {
		if (adi_dv[0]->db_collID == DB_UNICODE_CASEINSENSITIVE_COLL)
		    rlen = slen * (MAX_CE_LEVELS-2) * sizeof(u_i2);
		else rlen = slen * MAX_CE_LEVELS * sizeof(u_i2);
	    }
	    else if (adf_scb->adf_collation)
		rlen = slen * sizeof(u_i2);
	    else rlen = slen;

	    rlen += DB_CNTSIZE;		/* add length of vbyte */
            break;
        }

        case ADI_DATE2DATE:
        {
           rprec = 6;
           switch (abs(adi_dvr->db_datatype))
           {
                 case DB_DTE_TYPE:
                 rprec = 0;
                 rlen = ADF_DTE_LEN;
                 break;

                 case DB_ADTE_TYPE:
                 rlen = ADF_ADATE_LEN;
                 break;

                 case DB_TMWO_TYPE:
                 case DB_TMW_TYPE:
                 case DB_TME_TYPE:
                 rlen = ADF_TIME_LEN;
                 break;

                 case DB_TSWO_TYPE:
                 case DB_TSW_TYPE:
                 case DB_TSTMP_TYPE:
                   rlen = ADF_TSTMP_LEN;
                 break;

                 case DB_INYM_TYPE:
                   rlen = ADF_INTYM_LEN;
                 break;

                 case DB_INDS_TYPE:
                   rlen = ADF_INTDS_LEN;
                 break;

               default:
                 return (adu_error(adf_scb, E_AD2022_UNKNOWN_LEN, 0));
           }
           break;
       }
	case ADI_O1AES:
	    /* 16-byte multiple for block encryption + varbyte length */
	    rprec = prec[0];
	    if (len[0] % 16)
		rlen = (16*((len[0]/16)+1))+sizeof(u_i2);
	    else
		rlen = len[0]+sizeof(u_i2);
	    break;

	default:
	    /* invalid result length specification */
	    return (adu_error(adf_scb, E_AD2020_BAD_LENSPEC, 2,
		    (i4) sizeof(adi_lenspec->adi_lncompute),
		    (i4 *) &adi_lenspec->adi_lncompute));
    }				    /* end of switch statement */

    adi_dvr->db_prec = rprec;
    adi_dvr->db_length = rlen;
    
    if (rlen == ADE_LEN_UNKNOWN)
	return (adu_error(adf_scb, E_AD2022_UNKNOWN_LEN, 0));

    /* 
    ** If these are string function with 2 arguments than we want
    ** to use the second argument as the result length
    */
    /* b96344 */
    if( adi_numops == 2 && adi_dv[1]->db_data &&
	adi_lenspec->adi_lncompute == ADI_LEN_INDIRECT)
    {
	if( (fi_desc->adi_fiopid == ADI_VARCH_OP || 
	     fi_desc->adi_fiopid == ADI_TEXT_OP  || 
	     fi_desc->adi_fiopid == ADI_ASCII_OP || 
	     fi_desc->adi_fiopid == ADI_CHAR_OP  ||
	     fi_desc->adi_fiopid == ADI_BYTE_OP  ||
	     fi_desc->adi_fiopid == ADI_VBYTE_OP || 
	     fi_desc->adi_fiopid == ADI_NCHAR_OP ||
	     fi_desc->adi_fiopid == ADI_NVCHAR_OP) &&
	   abs(adi_dv[1]->db_datatype) == DB_INT_TYPE)
	{
	    i4 minlen = sizeof(char);
	    i4 nullen = 0;
	    switch( adi_dv[1]->db_length)
	    {
		case 1:
		  rlen = I1_CHECK_MACRO(*(i1 *)adi_dv[1]->db_data);
		  break;

		case 2:
		  rlen = *(i2 *)adi_dv[1]->db_data;
		  break;

		case 4:
		  rlen = *(i4 *)adi_dv[1]->db_data;
		  break;

		default:
		  return (adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0));
	      }

	    /* nvarchar(all, i) -> i is chars, rlen is bytes */
	    if (fi_desc->adi_fiopid == ADI_NCHAR_OP ||
	        fi_desc->adi_fiopid == ADI_NVCHAR_OP)
	    {
	    	rlen *= sizeof(UCS2);
	    	minlen = sizeof(UCS2);
	    }

	    if( fi_desc->adi_fiopid == ADI_VARCH_OP || 
	        fi_desc->adi_fiopid == ADI_TEXT_OP  ||
	        fi_desc->adi_fiopid == ADI_VBYTE_OP ||
		fi_desc->adi_fiopid == ADI_NVCHAR_OP)
	    {
		rlen += DB_CNTSIZE;
		minlen += DB_CNTSIZE;
	    }
	    /* Adjust minlen for nullable */
	    if (adi_dv[0]->db_datatype < 0)
		nullen++;
	    /* Use the second argument if the length is valid or
	    ** otherwise use minlen */
	    adi_dvr->db_length = rlen >= minlen+nullen ? rlen : minlen;
	}
    }
	
    /*
    ** b116215:
    ** we now inherit collation specification if:
    ** a) Both operands have the same collation specification.
    ** or
    ** b) One operand has collation specification and the other has not.
    */
    if (adi_numops == 2 &&
        (!(adi_dv[0]->db_collID > DB_NOCOLLATION &&
          adi_dv[1]->db_collID > DB_NOCOLLATION) ||
          adi_dv[1]->db_collID == adi_dv[0]->db_collID
        )
       )
    {
        adi_dvr->db_collID = max(adi_dv[0]->db_collID, adi_dv[1]->db_collID);
    }

    if (adi_dvr->db_datatype < 0)  /* If output is nullable, increment length */
	adi_dvr->db_length++;

    return (E_DB_OK);
}

/*{
** Name: adi_1len_indirect() - Calculate LENSPEC base on input datatypes.
**
** Description:
**	This routine creates a LENSPEC structure based on the input
**      datatype.  Function with DB_ALL_TYPE operand may require to
**      call this routine to figure out the LENSPEC according to input
**      datatypes.
**
** Inputs:
**      fiopid                          Operator ID for this function 
**                                      instance.
**      dv1                             Ptr to DB_DATA_VALUE for first input
**                                      operand.  (Only used as a convenient
**                                      mechanism for supplying a datatype and
**                                      length.) 
**	    .db_datatype		Datatype for first input operand.
**	    .db_prec			Precision/Scale for first input operand
**	    .db_length  		Length of first input operand, or
**                                      ADE_LEN_UNKNOWN if unknown.
**	    .db_data			A pointer to the data for first input
**					operand (currently not used).
**      dv2                             Ptr to DB_DATA_VALUE for second input
**                                      operand.  (Only used as a convenient
**                                      mechanism for supplying a datatype and
**                                      length.) 
**	    .db_datatype		Datatype for second input operand.
**	    .db_prec			Precision/Scale for second input operand
**	    .db_length  		Length of second input operand, or
**                                      ADE_LEN_UNKNOWN if unknown.
**	    .db_data			A pointer to the data for second input
**					operand (currently used only by the
**					ADI_DECFUNC lenspec).
**      out_lenspec                     Ptr output ADI_LENSPEC structure.
**
** Outputs:
**      out_lenspec
**	    .adi_lncompute              LENSPEC type
**	    .adi_fixedsize              Fixed size

**	Returns:
**	    The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      08-dec-1992 (stevet)
**        Created for DB_ALL_TYPE version of ADI_VARCH_OP, ADI_TEXT_OP,
**        ADI_CHAR_OP and ADI_ASCII_OP.
**      23-Apr-1993 (fred)
**        Added byte operations
**	31-jul-1998 (somsa01)
**	    Added special case for long varchar and long varbyte input
**	    datatypes.  (Bug #74162)
**	03-dec-2003 (gupsh01)
**	    Added support for ADI_NVCHAR_OP and ADI_NCHAR_OP. 
**	18-may-2004 (gupsh01)
**	    Added support for ADI_NPRINT and ADI_NTPRINT lenspecs.
**	17-jun-2006 (gupsh01)
**	    Added support for ANSI datetime datatypes.
**	23-oct-2006 (gupsh01)
**	    Length calculation between ANSI date/time types modified.
**	26-oct-2006 (gupsh01)
**	    Reinstate the length calculations for ADI_INDIRECT for date 
**	    types as this is not used for coercion between a date to date
**	    types.
**	08-Apr-2008 (kiria01) b121900
**	    Correct the NCHAR(NVARCHAR) and NVARCHAR(NCHAR) codes - the
**	    TC & CT were the wrong way round!
**	    Also made NVARCHAR(NVARCHAR) use ADI_O1 as it should.
*/
static DB_STATUS
adi_1len_indirect( 
ADI_OP_ID          fiopid,
DB_DATA_VALUE      *dv1,
DB_DATA_VALUE      *dv2,
ADI_LENSPEC        *out_lenspec) 
{
    switch( fiopid)
    {
      case ADI_VARCH_OP:
      case ADI_TEXT_OP:
      case ADI_VBYTE_OP:
	switch(abs(dv1->db_datatype))
	{
	  case DB_CHR_TYPE:
	  case DB_CHA_TYPE:
	  case DB_BYTE_TYPE:
	    out_lenspec->adi_lncompute = ADI_O1CT;
	    break;
	  case DB_LTXT_TYPE:
	  case DB_VCH_TYPE:
	  case DB_TXT_TYPE:
	  case DB_VBYTE_TYPE:
	    out_lenspec->adi_lncompute = ADI_O1;
	    break;
	  case DB_LVCH_TYPE:
	  case DB_LBYTE_TYPE:
	  case DB_LNVCHR_TYPE:
	    out_lenspec->adi_lncompute = ADI_COUPON;
	    break;
	  case DB_DTE_TYPE:
	    out_lenspec->adi_lncompute = ADI_FIXED;
	    out_lenspec->adi_fixedsize = AD_1DTE_OUTLENGTH + DB_CNTSIZE;
	    break;
          case DB_ADTE_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_2ADTE_OUTLENGTH + DB_CNTSIZE;
            break;
          case DB_TMWO_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_3TMWO_OUTLENGTH + DB_CNTSIZE;
            break;
          case DB_TMW_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_4TMW_OUTLENGTH + DB_CNTSIZE;
            break;
          case DB_TME_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_5TME_OUTLENGTH + DB_CNTSIZE;
            break;
          case DB_TSWO_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_6TSWO_OUTLENGTH + DB_CNTSIZE;
            break;
          case DB_TSW_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_7TSW_OUTLENGTH + DB_CNTSIZE;
            break;
          case DB_TSTMP_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_8TSTMP_OUTLENGTH + DB_CNTSIZE;
            break;
          case DB_INYM_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_9INYM_OUTLENGTH + DB_CNTSIZE;
            break;
          case DB_INDS_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_10INDS_OUTLENGTH + DB_CNTSIZE;
            break;
	  case DB_FLT_TYPE:
	  case DB_DEC_TYPE:
	  case DB_INT_TYPE:
	    out_lenspec->adi_lncompute = ADI_TPRINT;
	    break;
	  case DB_MNY_TYPE:
	    out_lenspec->adi_lncompute = ADI_FIXED;
	    out_lenspec->adi_fixedsize = 20+DB_CNTSIZE;
	    break;
	  case DB_LOGKEY_TYPE:
	    out_lenspec->adi_lncompute = ADI_FIXED;
	    out_lenspec->adi_fixedsize = DB_LEN_OBJ_LOGKEY+DB_CNTSIZE;
	    break;
	  case DB_TABKEY_TYPE:
	    out_lenspec->adi_lncompute = ADI_FIXED;
	    out_lenspec->adi_fixedsize = DB_LEN_TAB_LOGKEY+DB_CNTSIZE;
	    break;
	  case DB_BIT_TYPE:
	  case DB_VBIT_TYPE:
	    out_lenspec->adi_lncompute = ADI_BIT2CHAR;
	    break;
	  case DB_NCHR_TYPE:
	    out_lenspec->adi_lncompute = ADI_UTOVARC;
	    break;
	  case DB_NVCHR_TYPE:
	    out_lenspec->adi_lncompute = ADI_UTOC;
	    break;
	  default:
	    out_lenspec->adi_lncompute = ADI_O1CT;
	    break;
	}
	break;

      case ADI_CHAR_OP:
      case ADI_ASCII_OP:
      case ADI_BYTE_OP:
	switch(abs(dv1->db_datatype))
	{
	  case DB_CHR_TYPE:
	  case DB_CHA_TYPE:
          case DB_BYTE_TYPE:
	    out_lenspec->adi_lncompute = ADI_O1;
	    break;
	  case DB_LTXT_TYPE:
	  case DB_VCH_TYPE:
	  case DB_TXT_TYPE:
	  case DB_VBYTE_TYPE:
	    out_lenspec->adi_lncompute = ADI_O1TC;
	    break;
	  case DB_LVCH_TYPE:      
	  case DB_LBYTE_TYPE:      
	  case DB_LNVCHR_TYPE:
	    out_lenspec->adi_lncompute = ADI_COUPON;
	    break;
	  case DB_DTE_TYPE:
	    out_lenspec->adi_lncompute = ADI_FIXED;
	    out_lenspec->adi_fixedsize = AD_1DTE_OUTLENGTH;
	    break;
          case DB_ADTE_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_2ADTE_OUTLENGTH;
            break;
          case DB_TMWO_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_3TMWO_OUTLENGTH;
            break;
          case DB_TMW_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_4TMW_OUTLENGTH;
            break;
          case DB_TME_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_5TME_OUTLENGTH;
            break;
          case DB_TSWO_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_6TSWO_OUTLENGTH;
            break;
          case DB_TSW_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_7TSW_OUTLENGTH;
            break;
          case DB_TSTMP_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_8TSTMP_OUTLENGTH;
            break;
          case DB_INYM_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_9INYM_OUTLENGTH;
            break;
          case DB_INDS_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_10INDS_OUTLENGTH;
            break;
	  case DB_FLT_TYPE:
	  case DB_DEC_TYPE:
	  case DB_INT_TYPE:
	    out_lenspec->adi_lncompute = ADI_PRINT;
	    break;
	  case DB_MNY_TYPE:
	    out_lenspec->adi_lncompute = ADI_FIXED;
	    out_lenspec->adi_fixedsize = 20;
	    break;
	  case DB_LOGKEY_TYPE:
	    out_lenspec->adi_lncompute = ADI_FIXED;
	    out_lenspec->adi_fixedsize = DB_LEN_OBJ_LOGKEY;
	    break;
	  case DB_TABKEY_TYPE:
	    out_lenspec->adi_lncompute = ADI_FIXED;
	    out_lenspec->adi_fixedsize = DB_LEN_TAB_LOGKEY;
	    break;
	  case DB_BIT_TYPE:
	  case DB_VBIT_TYPE:
	    out_lenspec->adi_lncompute = ADI_BIT2CHAR;
	    break;
	  case DB_NCHR_TYPE:
	    out_lenspec->adi_lncompute = ADI_UTOC;
	    break;
	  case DB_NVCHR_TYPE:
	    out_lenspec->adi_lncompute = ADI_VARUTOC;
	    break;
	  default:
	    out_lenspec->adi_lncompute = ADI_O1;
	    break;
	}
	break;

      case ADI_NCHAR_OP:
	switch(abs(dv1->db_datatype))
	{
	  case DB_CHR_TYPE:
          case DB_CHA_TYPE:
          case DB_BYTE_TYPE:
            out_lenspec->adi_lncompute = ADI_CTOU;
            break;
          case DB_LTXT_TYPE:
          case DB_VCH_TYPE:
          case DB_TXT_TYPE:
          case DB_VBYTE_TYPE:
            out_lenspec->adi_lncompute = ADI_VARCTOU;
            break;
          case DB_LVCH_TYPE:
          case DB_LBYTE_TYPE:
          case DB_LNVCHR_TYPE:
            out_lenspec->adi_lncompute = ADI_COUPON;
            break;
          case DB_DTE_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_1DTE_OUTLENGTH * sizeof(UCS2);
            break;
          case DB_ADTE_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_2ADTE_OUTLENGTH * sizeof(UCS2);
            break;
          case DB_TMWO_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_3TMWO_OUTLENGTH * sizeof(UCS2);
            break;
          case DB_TMW_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_4TMW_OUTLENGTH * sizeof(UCS2);
            break;
          case DB_TME_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_5TME_OUTLENGTH * sizeof(UCS2);
            break;
          case DB_TSWO_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_6TSWO_OUTLENGTH * sizeof(UCS2);
            break;
          case DB_TSW_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_7TSW_OUTLENGTH * sizeof(UCS2);
            break;
          case DB_TSTMP_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_8TSTMP_OUTLENGTH * sizeof(UCS2);
            break;
          case DB_INYM_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_9INYM_OUTLENGTH * sizeof(UCS2);
            break;
          case DB_INDS_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_10INDS_OUTLENGTH * sizeof(UCS2);
            break;
          case DB_FLT_TYPE:
          case DB_DEC_TYPE:
          case DB_INT_TYPE:
            out_lenspec->adi_lncompute = ADI_NPRINT;
            break;
          case DB_MNY_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = 20 * sizeof(UCS2);
            break;
          case DB_LOGKEY_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = DB_LEN_OBJ_LOGKEY * sizeof(UCS2);
            break;
          case DB_TABKEY_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = DB_LEN_TAB_LOGKEY * sizeof(UCS2);
            break;
	  /* Fix me add support for BIT TYPE */
	  case DB_NVCHR_TYPE:
	    out_lenspec->adi_lncompute = ADI_O1TC;
	    break;
	  default:
	    out_lenspec->adi_lncompute = ADI_O1;
	    break;
	}
	break;

      case ADI_NVCHAR_OP:
	switch(abs(dv1->db_datatype))
	{
	  case DB_CHR_TYPE:
          case DB_CHA_TYPE:
          case DB_BYTE_TYPE:
            out_lenspec->adi_lncompute = ADI_CTOVARU;
            break;
          case DB_LTXT_TYPE:
          case DB_VCH_TYPE:
          case DB_TXT_TYPE:
          case DB_VBYTE_TYPE:
            out_lenspec->adi_lncompute = ADI_CTOU;
            break;
          case DB_LVCH_TYPE:
          case DB_LBYTE_TYPE:
          case DB_LNVCHR_TYPE:
            out_lenspec->adi_lncompute = ADI_COUPON;
            break;
          case DB_DTE_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_1DTE_OUTLENGTH * sizeof(UCS2);
            break;
          case DB_ADTE_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_2ADTE_OUTLENGTH * sizeof(UCS2);
            break;
          case DB_TMWO_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_3TMWO_OUTLENGTH * sizeof(UCS2);
            break;
          case DB_TMW_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_4TMW_OUTLENGTH * sizeof(UCS2);
            break;
          case DB_TME_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_5TME_OUTLENGTH * sizeof(UCS2);
            break;
          case DB_TSWO_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_6TSWO_OUTLENGTH * sizeof(UCS2);
            break;
          case DB_TSW_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_7TSW_OUTLENGTH * sizeof(UCS2);
            break;
          case DB_TSTMP_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_8TSTMP_OUTLENGTH * sizeof(UCS2);
            break;
          case DB_INYM_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_9INYM_OUTLENGTH * sizeof(UCS2);
            break;
          case DB_INDS_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = AD_10INDS_OUTLENGTH * sizeof(UCS2);
            break;
          case DB_FLT_TYPE:
          case DB_DEC_TYPE:
          case DB_INT_TYPE:
            out_lenspec->adi_lncompute = ADI_NTPRINT;
            break;
          case DB_MNY_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = 20 * sizeof(UCS2);
            break;
          case DB_LOGKEY_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = DB_LEN_OBJ_LOGKEY * sizeof(UCS2);
            break;
          case DB_TABKEY_TYPE:
            out_lenspec->adi_lncompute = ADI_FIXED;
            out_lenspec->adi_fixedsize = DB_LEN_TAB_LOGKEY * sizeof(UCS2);
            break;
	  /* Fix me: add support for BIT TYPE */
	  case DB_NCHR_TYPE:
	    out_lenspec->adi_lncompute = ADI_O1CT;
	    break;
	  case DB_NVCHR_TYPE:
	    out_lenspec->adi_lncompute = ADI_O1;
	    break;
	  default:
	    out_lenspec->adi_lncompute = ADI_O1CT;
	    break;
	}
	break;

      default:
	return( E_DB_ERROR);
    }
    return( E_DB_OK);
}
