/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <ade.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <adfhist.h>

/**
**
**  Name: ADCHGDTLN.C -	    Get histogram datatype, precision, and length
**			    mapping information for a given datatype, precision,
**			    and length.
**
**  Description:
**      This file contains all of the routines necessary to
**      perform the external ADF call "adc_hg_dtln()" which
**      returns a structure containing datatypes, precisions, and lengths
**	for the histogram mapping function on a given datatype, precision,
**	and length.
**
**      This file defines:
**
**          adc_hg_dtln() -  Get datatype, precision, and length histogram
**			     mapping information for a given datatype,
**			     precision, and length.
**	    adc_1hg_dtln_rti() - Get datatype, precision, and length histogram
**			     mapping information for a given "RTI known"
**			     datatype, precision, and length.
**
**
**  History:
**      19-jun-86 (ericj)
**          Initial creation.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	28-oct-86 (thurston)
**	    Added char and varchar support.
**	02-jan-87 (thurston)
**	    Made all of the string types produce "char" histogram elements
**	    instead of "c".
**	20-feb-87 (thurston)
**	    Added support for nullable datatypes.
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	20-mar-89 (jrb)
**	    Made changes for adc_lenchk interface change.
**	19-apr-89 (mikem)
**	    Logical key development.  Added support for DB_LOGKEY_TYPE and
**	    DB_TABKEY_TYPE to adc_1hg_dtln_rti().
**	28-apr-89 (fred)
**	    Altered references to Adi_dtptrs to use ADI_DT_MAP_MACRO()
**	05-jul-89 (jrb)
**	    Added decimal support.
**      22-dec-1992 (stevet)
**          Added function prototype.
**	18-jan-93 (rganski)
**	    Changed semantics for character types: for character types,
**	    adc_hgdv.db_length is now also an input parameter: if <= 0 on
**	    input, output will be the same as adc_fromdv.db_length, or 1950,
**	    whichever is smaller. If > 0 on input, output will be same as input
**	    value, or adc_fromdv.db_length, or 1950, whichever is smaller. Part
**	    of Character Histogram Enhancements project. See project spec for
**	    more details.
**       6-Apr-1993 (fred)
**          Added byte string datatypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      14-aug-2009 (joea)
**          Add case for DB_BOO_TYPE in adc_1hg_dtln_rti.
**      09-mar-2010 (thich01)
**          Add DB_NBR_TYPE like DB_BYTE_TYPE for rtree indexing.
**	19-Nov-2010 (kiria01) SIR 124690
**	    Ensure whole DBV is copied.
**/


/*{
** Name: adc_hg_dtln()-	    Get datatype, precision, and length histogram
**			    mapping information for a given datatype, precision,
**			    and length.
**
** Description:
**	This function is the external ADF call "adc_hg_dtln()" which returns a
**	structure containing datatypes, precisions, and lengths for the
**	histogram mapping function on a given datatype, precision, and length.
**	It is guaranteed that the datatype, precision, and length that a given
**	datatype, precision, and length gets mapped into will not depend on the
**	date format or money format in use or any other "session specific"
**	parameter.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adc_fromdv                      Ptr to DB_DATA_VALUE describing the
**					data value from which the histogram
**					value will be made.
**	    .db_datatype		The datatype id to be mapped.
**	    .db_prec			The precision/scale.
**	    .db_length			The internal length.
**	adg_hgdv			Ptr to DB_DATA_VALUE that will be
**					updated to describe the result
**					histogram value.
**	    .db_length			The length, in bytes, of the
**					data value resulting from the
**					"hg" mapping.
**					For character types, this is also an
**					input parameter: if <= 0 on input,
**					output will be the same as
**					adc_fromdv.db_length. If > 0 on
**					input, output will be same as input
**					value or adc_fromdv.db_length,
**					whichever is smaller. Limit either way
**					is 1950.
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
**	adg_hgdv
**	    .db_datatype		The datatype this datatype will
**					be mapped into to create a
**					histogram element (only used by
**					OPF.)  This "hg" mapping is done
**					via the adc_helem() routine.
**	    .db_prec			The precision/scale of the data value
**					resulting from the "hg" mapping.
**	    .db_length			The length, in bytes, of the
**					data value resulting from the
**					"hg" mapping.
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
**          E_AD0000_OK                 Operation succeeded.
**          E_AD2004_BAD_DTID           Datatype id unknown to ADF.
**
**	    NOTE:  The following error codes are internal consistency
**	    checking and will be returned only when the system is
**	    compiled with the xDEBUG flag.
**
**          E_AD2005_BAD_DTLEN          Internal length is illegal for
**					the given datatype.
**	    E_AD200B_BAD_PREC		Invalid precision specified for DECIMAL;
**					must be: 1<= precision <=DB_MAX_DECPREC
**	    E_AD200C_BAD_SCALE		Invalid scale specified for DECIMAL;
**					must be: 0<= scale <=precision
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	15-jun-86 (ericj)
**	    Initial creation and coding.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	20-feb-87 (thurston)
**	    Added support for nullable datatypes.
**      22-mar-91 (jrb)
**          Put comment markers around "xDEBUG" after #endif's.  This was
**          causing problems on some compilers.
**	18-jan-93 (rganski)
**	    Character Histogram Enhancements project:
**	    Changed semantics for character types: for character types,
**	    adc_hgdv.db_length is now also an input parameter: if <= 0 on
**	    input, output will be the same as adc_fromdv.db_length, or 1950,
**	    whichever is smaller. If > 0 on input, output will be same as input
**	    value, or adc_fromdv.db_length, or 1950, whichever is smaller.
**	    To get old behavior for character types, set adc_hgdv.db_length to
**	    8.
**	    See project spec for more details.
**	21-jun-93 (rganski)
**	    Return error status if the datatype can be stored in relations but
**	    no internal dtln function is defined for it in adgdttab.roc.
**	    Otherwise a segmentation violation occurs.
**	23-jul-93 (rganski)
**	    Use data type status bits for the data type to determine if
**	    histograms can be built. Replaces switch statement.
*/

# ifdef ADF_BUILD_WITH_PROTOS
DB_STATUS
adc_hg_dtln(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *adc_fromdv,
DB_DATA_VALUE	    *adc_hgdv)
# else
DB_STATUS
adc_hg_dtln( adf_scb, adc_fromdv, adc_hgdv)
ADF_CB              *adf_scb;
DB_DATA_VALUE	    *adc_fromdv;
DB_DATA_VALUE	    *adc_hgdv;
# endif
{
    DB_STATUS		db_stat;
    i4			bdt     = abs((i4) adc_fromdv->db_datatype);
    i4			bdtv;
    i4			dtinfo;	/* Data type status bits for this type */

    for (;;)
    {
        /* Check the consistency of the input datatype id */
	bdtv = ADI_DT_MAP_MACRO(bdt);
        if (bdtv <= 0  ||  bdtv > ADI_MXDTS
		||  Adf_globs->Adi_dtptrs[bdtv] == NULL)
	{
	    db_stat = adu_error(adf_scb, E_AD2004_BAD_DTID, 0);
	    break;
	}

	/* Return error for datatypes that cannot be used in a relation or
	** do not support histograms */
	/* Get datatype status bits into dtinfo */
	db_stat = adi_dtinfo(adf_scb, adc_fromdv->db_datatype, &dtinfo);
	if (!(dtinfo & AD_INDB) || (dtinfo & AD_NOHISTOGRAM))
	{
	    /* This datatype can't be used in a relation or doesn't support
	    ** histograms */
	    db_stat = adu_error(adf_scb, E_AD3011_BAD_HG_DTID, 0);
	    break;
	}

#ifdef xDEBUG
	    /* Check the consistency of adc_fromdv's length */
	    if (db_stat = adc_lenchk(adf_scb, FALSE, adc_fromdv, NULL))
	        break;
#endif  /* xDEBUG */

	/*
	** Now call the low-level routine to do the work.  Note that if the
	** datatype is nullable, we use a temp DV_DATA_VALUE which gets set
	** to the corresponding base datatype and length.
	*/

	if (adc_fromdv->db_datatype > 0)	/* non-nullable */
	{
            db_stat = (*Adf_globs->Adi_dtptrs[ADI_DT_MAP_MACRO(bdt)]->
			    adi_dt_com_vect.adp_hg_dtln_addr)
				(adf_scb, adc_fromdv, adc_hgdv);
	}
	else					/* nullable */
	{
	    DB_DATA_VALUE tmp_dv = *adc_fromdv;

	    tmp_dv.db_datatype = bdt;
	    tmp_dv.db_length--;

            db_stat = (*Adf_globs->Adi_dtptrs[ADI_DT_MAP_MACRO(bdt)]->
			adi_dt_com_vect.adp_hg_dtln_addr)
				(adf_scb, &tmp_dv, adc_hgdv);
	}
	break;
    }	    /* end of for stmt */

    return(db_stat);
}


/*
** Name: adc_1hg_dtln_rti()-	Get datatype and length histogram mapping
**				information for a given "RTI known" 
**				datatype and length.
**
** Description:
**	This function is the external ADF call "adc_1hg_dtln_rti()" which
**	returns a structure containing datatypes, precisions, and lengths for
**	the histogram mapping function on a given datatype, precision, and
**	length.  It is guaranteed that the datatype, precision, and length that
**	a given datatype, precision, and length gets mapped into will not
**	depend on the date format or money format in use or any other "session
**	specific" parameter.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adc_fromdv                      Ptr to DB_DATA_VALUE describing the
**					data value from which the histogram
**					value will be made.
**	    .db_datatype		The datatype id to be mapped.
**	    .db_prec			The precision/scale.
**	    .db_length			The internal length.
**	adg_hgdv			Ptr to DB_DATA_VALUE that will be
**					updated to describe the result
**					histogram value.
**	    .db_length			The length, in bytes, of the
**					data value resulting from the
**					"hg" mapping.
**					For character types, this is also an
**					input parameter: if <= 0 on input,
**					output will be the same as
**					adc_fromdv.db_length. If > 0 on
**					input, output will be same as input
**					value or adc_fromdv.db_length,
**					whichever is smaller. Limit either way
**					is 1950.
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
**	adg_hgdv
**	    .db_datatype		The datatype this datatype will
**					be mapped into to create a
**					histogram element (only used by
**					OPF.)  This "hg" mapping is done
**					via the adc_helem() routine.
**	    .db_prec			The precision/scale of the data value
**					resulting from the "hg" mapping.
**	    .db_length			The length, in bytes, of the
**					data value resulting from the
**					"hg" mapping.
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
**          E_AD0000_OK                 Operation succeeded.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	21-jun-86 (ericj)
**	    Initial creation and coding.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	28-oct-86 (thurston)
**	    Added char and varchar support.
**	02-jan-87 (thurston)
**	    Made all of the string types produce "char" histogram elements
**	    instead of "c".
**	19-apr-89 (mikem)
**	    Logical key development.  Added support for DB_LOGKEY_TYPE and
**	    DB_TABKEY_TYPE to adc_1hg_dtln_rti().
**	05-jul-89 (jrb)
**	    Added decimal support.
**	18-jan-93 (rganski)
**	    Changed semantics for character types: for character types,
**	    adc_hgdv.db_length is now also an input parameter: if <= 0 on
**	    input, output will be the same as adc_fromdv.db_length, or 1950,
**	    whichever is smaller. If > 0 on input, output will be same as input
**	    value, or adc_fromdv.db_length, or 1950, whichever is smaller. Part
**	    of Character Histogram Enhancements project. See project spec for
**	    more details.
**       6-Apr-1993 (fred)
**          Added byte string datatypes.
**	23-apr-93 (robf)
**	    Improve support for security labels, use regular size rather 
**	     than 1 byte.
**	1-dec-98 (inkdo01)
**	    For ADE_LEN_UNKNOWN (probably a function or expression involving
**	    a RQ parameter), set string length to 8.
**	6-jun-01 (hayke02)
**	    If we get a adc_hgdv->db_length of 0, set it to 1 - this will
**	    occur for the resdom in the temp tab for 'select varchar(NULL)...
**	    union select varchar(NULL)...'. This change fixes bug 104672.
**  05-feb-2000 (gupsh01)
**          Added unicode char nchar and nvarchar support. The histogram's
**          datatypes and length are DB_INT_TYPE (integer) and 2, respectively.
**	24-dec-04 (inkdo01)
**	    Change NCHR/NVCHR to BYTE to store collation_weight.
**	24-apr-06 (dougi)
**	    Add support of new date/time data types.
**	15-may-2007 (dougi)
**	    Add support of UTF8 enabled server.
**	03-Oct-2008 (kiria01) b120966
**	    Added DB_BYTE_TYPE support to allow for Unicode CE entries
**	    to be treated as raw collation data alongside DB_CHA_TYPE.
**	    If this is not done, CE entries get compated using CHAR semantics
**	    which is so wrong.
**	19-Nov-2010 (kiria01) SIR 124690
**	    Add support for UCS_BASIC collation. No need for the increased length
**	    as it doesn't use UCS2 CEs.
*/

DB_STATUS
adc_1hg_dtln_rti(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *adc_fromdv,
DB_DATA_VALUE	    *adc_hgdv)
{

    /* Calculate the corresponding histogram datatype and length */

    switch(adc_fromdv->db_datatype)
    {
      case DB_CHR_TYPE:
      case DB_CHA_TYPE:
      case DB_LOGKEY_TYPE:
      case DB_TABKEY_TYPE:
	adc_hgdv->db_datatype	= DB_CHA_TYPE;
	adc_hgdv->db_prec	= 0;
	if (adc_hgdv->db_length <= 0)
	 if (adc_fromdv->db_length == ADE_LEN_UNKNOWN) adc_hgdv->db_length = 8;
	 else adc_hgdv->db_length = adc_fromdv->db_length;
	else
	    adc_hgdv->db_length	= min(adc_fromdv->db_length,
				      adc_hgdv->db_length);
	if (adc_hgdv->db_length > DB_MAX_HIST_LENGTH)
	    adc_hgdv->db_length = DB_MAX_HIST_LENGTH;
	if ((adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
	    (adc_fromdv->db_datatype == DB_CHA_TYPE ||
	     adc_fromdv->db_datatype == DB_CHR_TYPE))
	    goto UTF8merge;
	break;

      case DB_TXT_TYPE:
      case DB_VCH_TYPE:
	adc_hgdv->db_datatype	= DB_CHA_TYPE;
	adc_hgdv->db_prec	= 0;
	if (adc_hgdv->db_length <= 0)
	 if (adc_fromdv->db_length == ADE_LEN_UNKNOWN) adc_hgdv->db_length = 8;
	 else adc_hgdv->db_length = adc_fromdv->db_length - DB_CNTSIZE;
	else
	    adc_hgdv->db_length	= min(adc_fromdv->db_length - DB_CNTSIZE,
				      adc_hgdv->db_length);
	if (adc_hgdv->db_length > DB_MAX_HIST_LENGTH)
	    adc_hgdv->db_length = DB_MAX_HIST_LENGTH;
	if (adc_hgdv->db_length == 0) /* Bug 104672 */
	    adc_hgdv->db_length = 1;
	if (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED)
	    goto UTF8merge;
	break;

      case DB_DTE_TYPE:
      case DB_ADTE_TYPE:
	adc_hgdv->db_datatype	= DB_INT_TYPE;
	adc_hgdv->db_prec	= 0;
	adc_hgdv->db_length	= AD_DTE1_HSIZE;
	break;
    
      case DB_TME_TYPE:
      case DB_TMW_TYPE:
      case DB_TMWO_TYPE:
	adc_hgdv->db_datatype	= DB_INT_TYPE;
	adc_hgdv->db_prec	= 0;
	adc_hgdv->db_length	= AD_TME1_HSIZE;
	break;
    
      case DB_TSTMP_TYPE:
      case DB_TSW_TYPE:
      case DB_TSWO_TYPE:
	adc_hgdv->db_datatype	= DB_INT_TYPE;
	adc_hgdv->db_prec	= 0;
	adc_hgdv->db_length	= AD_TST1_HSIZE;
	break;
    
      case DB_INYM_TYPE:
      case DB_INDS_TYPE:
	adc_hgdv->db_datatype	= DB_INT_TYPE;
	adc_hgdv->db_prec	= 0;
	adc_hgdv->db_length	= AD_INT1_HSIZE;
	break;
    
      case DB_BOO_TYPE:
      case DB_FLT_TYPE:
      case DB_INT_TYPE:
	adc_hgdv->db_datatype	= adc_fromdv->db_datatype;
	adc_hgdv->db_prec	= 0;
	adc_hgdv->db_length	= adc_fromdv->db_length;
	break;

      case DB_DEC_TYPE:
	adc_hgdv->db_datatype	= DB_FLT_TYPE;
	adc_hgdv->db_prec	= 0;
	adc_hgdv->db_length	= AD_DEC1_HSIZE;
	break;
	
      case DB_MNY_TYPE:
	adc_hgdv->db_datatype	= DB_INT_TYPE;
	adc_hgdv->db_prec	= 0;
	adc_hgdv->db_length	= AD_MNY1_HSIZE;
	break;

      case DB_BIT_TYPE:
	adc_hgdv->db_datatype 	= DB_BIT_TYPE;
	if (adc_fromdv->db_length > AD_BIT1_MAX_HSIZE)
	{
	    adc_hgdv->db_length = AD_BIT1_MAX_HSIZE;
	    adc_hgdv->db_prec = BITSPERBYTE;
	}
	else
	{
	    adc_hgdv->db_length	= adc_fromdv->db_length;
	    adc_hgdv->db_prec	= adc_fromdv->db_prec;
	}
	break;

      case DB_VBIT_TYPE:
      {
	DB_BIT_STRING	bs;
	i4		len;

	adc_hgdv->db_datatype 	= DB_BIT_TYPE;
	len = adc_fromdv->db_length - sizeof(bs.db_b_count);
	if (len > AD_BIT1_MAX_HSIZE)
	{
	    adc_hgdv->db_length	= AD_BIT1_MAX_HSIZE;
	    adc_hgdv->db_prec = BITSPERBYTE;
	}
	else
	{
	    adc_hgdv->db_length = len;
	    adc_hgdv->db_prec = adc_fromdv->db_prec;
	}
	break;
      }

      case DB_BYTE_TYPE:
      case DB_NBR_TYPE:
	adc_hgdv->db_datatype	= DB_BYTE_TYPE;
	adc_hgdv->db_prec	= 0;
	if (adc_hgdv->db_length <= 0)
	 if (adc_fromdv->db_length == ADE_LEN_UNKNOWN) adc_hgdv->db_length = 8;
	 else adc_hgdv->db_length = adc_fromdv->db_length;
	else
	    adc_hgdv->db_length	= min(adc_fromdv->db_length,
				      adc_hgdv->db_length);
	if (adc_hgdv->db_length > DB_MAX_HIST_LENGTH)
	    adc_hgdv->db_length = DB_MAX_HIST_LENGTH;
	break;

      case DB_VBYTE_TYPE:
	adc_hgdv->db_datatype	= DB_BYTE_TYPE;
	adc_hgdv->db_prec	= 0;
	if (adc_hgdv->db_length <= 0)
	 if (adc_fromdv->db_length == ADE_LEN_UNKNOWN) adc_hgdv->db_length = 8;
	 else adc_hgdv->db_length = adc_fromdv->db_length - DB_CNTSIZE;
	else
	    adc_hgdv->db_length	= min(adc_fromdv->db_length - DB_CNTSIZE,
				      adc_hgdv->db_length);
	if (adc_hgdv->db_length > DB_MAX_HIST_LENGTH)
	    adc_hgdv->db_length = DB_MAX_HIST_LENGTH;
	break;

      case DB_NCHR_TYPE:
      case DB_NVCHR_TYPE:
	adc_hgdv->db_prec       = 0;
	if (adc_hgdv->db_length <= 0)
	 if (adc_fromdv->db_length == ADE_LEN_UNKNOWN) adc_hgdv->db_length = 8;
	 else adc_hgdv->db_length = adc_fromdv->db_length;
	else
	    adc_hgdv->db_length	= min(adc_fromdv->db_length,
				      adc_hgdv->db_length);
UTF8merge:
	adc_hgdv->db_datatype = DB_BYTE_TYPE;
	if (adc_fromdv->db_collID != DB_UCS_BASIC_COLL)
	    /* To make room for collation weight, multiply by 4. */
	    adc_hgdv->db_length *= 4;
	if (adc_hgdv->db_length > DB_MAX_HIST_LENGTH)
	    adc_hgdv->db_length = DB_MAX_HIST_LENGTH;
	break;
	
      default:
	return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));

    }	/* end of histogram switch stmt */

    return(E_DB_OK);
}
