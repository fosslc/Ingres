/*
** Copyright (c) 2004 Ingres Corporation
NO_OPTIM = usl_us5 
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <cm.h>
#include    <cv.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduint.h>
#include    <aduucol.h>
/**
**
**  Name: ADCHELEM.C - Build a histogram element for a data value.
**
**  Description:
**      This file contains all of the routines necessary to
**      perform the  external ADF call "adc_helem()" which
**      will convert a data value into its histogram element form.
**
**      This file defines:
**
**          adc_helem() -	Build a histogram element for a data value.
**	    adc_1helem_rti() -	Build a histogram element for a
**				data value with an "RTI known" datatype.
**
**
**  History:
**      26-feb-86 (thurston)
**          Initial creation.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	28-oct-86 (thurston)
**	    Added char and varchar support.
**	02-jan-87 (thurston)
**	    In the adc_1helem_rti() routine, I now null pad instead of blank
**	    pad for "c" input.  Also, all string types create a "char" histogram
**	    element now, not a "c".
**	20-feb-87 (thurston)
**	    Added support for nullable datatypes.
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	09-jun-88 (jrb)
**	    Added CM calls for Kanji support.
**	20-mar-89 (jrb)
**	    Changed to support new adc_lenchk interface.
**	19-apr-89 (mikem)
**	    Logical key development.  Added support for DB_LOGKEY_TYPE and
**	    DB_TABKEY_TYPE to adc_1helem_rti().
**	28-apr-89 (fred)
**	    Changed ref's to Adi_dtptrs to use the ADI_DT_MAP_MACRO()
**	02-june-89 (mikem)
**	    Fixed logical key bug, added missing break to case statement.
**	    Also fixed up comments, hash element for logical keys are now
**	    always 8 bytes whether a table_key or object_key.
**	05-jul-89 (jrb)
**	    Added decimal support.
**	18-nov-91 (stevet)
**	    Fixed precision check error in adc_helem() when processing UDT.
**	22-Oct-1992 (fred)
**	    Fixed up bit histogram handling to match ANSI data representation.
**      22-dec-1992 (stevet)
**          Added function prototype.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	12-Dec-2000 (bolke01)
**	   added NO_OPTIM for usl_us5 ( UW 7.0.1) as the optimisation of the 
**	   code caused a failure in the generation of histograms for varchar
**	   columns.  (#103471)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-feb-2001 (gupsh01)
**	    Added support for unicode nchar and nvarchar type.
**      13-aug-2009 (joea)
**          Add case for DB_BOO_TYPE in adc_1helem_rti.
**/


/*{
** Name: adc_helem() - Build a histogram element for a data value.
**
** Description:
**      This function is the external ADF call "adc_helem()" which
**      will convert a data value into its histogram element form.
**
**      For each datatype there is a histogram mapping function.  This function
**      will map the values of the datatype into ordered numbers or character
**      strings.  The numbers/strings returned will be ordered such that,
**      if A1 and A2 are two values of the same datatype and f() is the mapping
**	function then, A1 < A2 implies f(A1) <= f(A2).
**
**      Each datatype has a histogram element form which enables a
**      histogram elements to be built from data values of that datatype
**      to be used by OPF for keeping statistics.  The proper datatype,
**	precision, and length to use for this conversion can be retrieved
**	via the adc_hg_dtln() routine.
**
**	NOTE:  If the input data value to this routine is the NULL value,
**	then the E_AD1050_NULL_HISTOGRAM error will be generated.  This tells
**	the optimizer to use its special NULL bucket.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adc_dvfrom                      Pointer to the data value to be
**                                      converted.
**		.db_datatype		Datatype id of data value to be
**					converted.
**		.db_prec		Precision/Scale of data value to be
**					converted.
**		.db_length  		Length of data value to be
**					converted.
**		.db_data    		Pointer to the actual data for
**		         		data value to be converted.
**      adc_dvhg                        Pointer to the data value to be
**					created.
**		.db_datatype		Datatype id of data value being
**					created.  (Must be consistent
**					with what is expected for the
**					from type/precision/length  (i.e.
**					what adc_hg_dtln() returns as
**					the datatype that a data value
**					of the from type/precision/length
**					gets mapped into to create a
**					histogram element).
**		.db_prec		Precision/Scale of data value being
**					created.  (Must be consistent
**					with what is expected for the
**					from precision/scale (i.e.
**					what adc_hg_dtln() returns as
**					the precision/scale that a data value
**					of the from datatype, prec, and length
**					gets mapped into to create a
**					histogram element).
**		.db_length  		Length of data value being
**					created.  (Must be consistent
**					with what is expected for the
**					from type/precision/length  (i.e.
**					what adc_hg_dtln() returns as
**					the length that a data value
**					of the from type/precision/length
**					gets mapped into to create a
**					histogram element).
**		.db_data    		Pointer to location to put the
**					actual data for the data value
**					being created.
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
**      adc_dvhg                        The resulting data value.
**	        .db_data    		The actual data for the data
**					value created will be placed
**					here.
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
**          E_AD1050_NULL_HISTOGRAM     Input data value was the NULL value.
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
**          E_AD3011_BAD_HG_DTID        Datatype for the resulting data
**					value is not correct for the "hg"
**					mapping on the from datatype, prec and
**					length.
**          E_AD3012_BAD_HG_DTLEN       Length for the resulting data value is
**					not correct for the "hg" mapping on the
**					from datatype, prec and length.
**          E_AD3013_BAD_HG_DTPREC      Precision/Scale for the resulting data
**					value is not correct for the "hg"
**					mapping on the from datatype, prec, and
**					length.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      26-feb-86 (thurston)
**          Initial creation.
**	27-jun-86 (ericj)
**	    Coded.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	20-feb-87 (thurston)
**	    Added support for nullable datatypes.
**      22-mar-91 (jrb)
**          Put comment markers around "xDEBUG" after #endif's.  This was
**          causing problems on some compilers.  Also removed unused stack
**          variable.
**	18-nov-91 (stevet)
**	    Fixed problem with UDT.  Only check for precision when datatype
**	    is DB_DEC_TYPE.
**	18-jan-93 (rganski)
**	    Initialized chk_dtln_dtv.db_length before xDEBUG call to
**	    adc_hg_dtln(), which now uses this length as an input parameter.
**       6-Apr-1993 (fred)
**          Added byte string datatypes.
*/

# ifdef ADF_BUILD_WITH_PROTOS
DB_STATUS
adc_helem(
ADF_CB              *adf_scb,
DB_DATA_VALUE       *adc_dvfrom,
DB_DATA_VALUE       *adc_dvhg)
# else
DB_STATUS
adc_helem( adf_scb, adc_dvfrom, adc_dvhg)
ADF_CB              *adf_scb;
DB_DATA_VALUE       *adc_dvfrom;
DB_DATA_VALUE       *adc_dvhg;
# endif
{
    DB_STATUS           db_stat = E_DB_OK;
    i4			bdt     = abs((i4) adc_dvfrom->db_datatype);
    i4			hg_bdt  = abs((i4) adc_dvhg->db_datatype);
    i4			bdtv;
    i4			hg_bdtv;


    for (;;)
    {
        /* Check the consistency of input arguments */
	bdtv = ADI_DT_MAP_MACRO(bdt);
	hg_bdtv = ADI_DT_MAP_MACRO(hg_bdt);
	if (    bdtv <= 0  ||  bdtv > ADI_MXDTS
	    ||  Adf_globs->Adi_dtptrs[bdtv] == NULL
	    ||  hg_bdtv <= 0  ||  hg_bdtv > ADI_MXDTS
	    ||  Adf_globs->Adi_dtptrs[hg_bdtv] == NULL
	   )
	{
	    db_stat = adu_error(adf_scb, E_AD2004_BAD_DTID, 0);
	    break;
	}

#ifdef	xDEBUG
	{
	    DB_DATA_VALUE   chk_dtln_dv;

	    /* Check the validity of the data values' lengths */
	    if (   (db_stat = adc_lenchk(adf_scb, FALSE, adc_dvfrom, NULL))
		|| (db_stat = adc_lenchk(adf_scb, FALSE, adc_dvhg, NULL))
	       )
	    {
		break;
	    }

	    /* Check that the histogram value's datatype, precision, and length
	    ** are correct for the input value's datatype, precision, and
	    ** length.
	    */

	    chk_dtln_dv.db_length = adc_dvhg->db_length;

	    if (db_stat = adc_hg_dtln(adf_scb, adc_dvfrom, &chk_dtln_dv))
		break;

	    if (chk_dtln_dv.db_datatype != adc_dvhg->db_datatype)
	    {
		db_stat = adu_error(adf_scb, E_AD3011_BAD_HG_DTID, 0);
		break;
	    }
	    else if (chk_dtln_dv.db_length != adc_dvhg->db_length)
	    {	
		db_stat = adu_error(adf_scb, E_AD3012_BAD_HG_DTLEN, 0);
		break;
	    }
	    else if (abs(chk_dtln_dv.db_datatype) == DB_DEC_TYPE &&
		     chk_dtln_dv.db_prec != adc_dvhg->db_prec)
	    {	
		db_stat = adu_error(adf_scb, E_AD3013_BAD_HG_DTPREC, 0);
		break;
	    }
        }
#endif	/* xDEBUG */

	/*
	** Now call the low-level routine to do the work.  Note that if the
	** datatype is nullable, we use a temp DV_DATA_VALUE which gets set
	** to the corresponding base datatype, precision, and length.
	*/

	if (adc_dvfrom->db_datatype > 0)	/* non-nullable */
	{
	    db_stat =
		(*Adf_globs->Adi_dtptrs[ADI_DT_MAP_MACRO(bdt)]->
					adi_dt_com_vect.adp_helem_addr)
			    (adf_scb, adc_dvfrom, adc_dvhg);
	}
	else if (ADI_ISNULL_MACRO(adc_dvfrom))	/* nullable, with NULL value */
	{
	    db_stat = adu_error(adf_scb, E_AD1050_NULL_HISTOGRAM, 0);
	}
	else					/* nullable, but not NULL */
	{
	    DB_DATA_VALUE	tmp_dv;

	    STRUCT_ASSIGN_MACRO(*adc_dvfrom, tmp_dv);
	    tmp_dv.db_datatype = bdt;
	    tmp_dv.db_length--;

	    db_stat = (*Adf_globs->Adi_dtptrs[ADI_DT_MAP_MACRO(bdt)]->
					    adi_dt_com_vect.adp_helem_addr)
			    (adf_scb, &tmp_dv, adc_dvhg);
	}
	break;
    }	    /* end of for stmt */

    return(db_stat);
}


/*
** Name: adc_1helem_rti() - Build a histogram element for a "RTI known"
**			    data value.
**
** Description:
**      This function is the external ADF call "adc_1helem_rti()" which
**      will convert a data value with a "RTI known" datatype into
**	its histogram element form.
**
**      For each datatype there is a histogram mapping function.  This
**      function will map the values of the datatype into ordered
**      numbers.  The numbers (probably integers) returned will be
**      ordered such that, if A1 and A2 are two values of the same
**      datatype and f() is the mapping function then, A1 < A2 implies
**      f(A1) <= f(A2).
**
**      Each datatype has a histogram element form which enables a
**      histogram elements to be built from data values of that datatype
**      to be used by OPF for keeping statistics.  The proper datatype
**      and length to use for this conversion can be retrieved via the
**      adc_hg_dtln() routine.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adc_dvfrom                      Pointer to the data value to be
**                                      converted.
**		.db_datatype		Datatype id of data value to be
**					converted.
**		.db_prec		Precision/Scale of data value to be
**					converted.
**		.db_length  		Length of data value to be
**					converted.
**		.db_data    		Pointer to the actual data for
**		         		data value to be converted.
**      adc_dvhg                        Pointer to the data value to be
**					created.
**		.db_datatype		Datatype id of data value being
**					created.  (Must be consistent
**					with what is expected for the
**					from type/precision/length  (i.e.
**					what adc_hg_dtln() returns as
**					the datatype that a data value
**					of the from type/precision/length
**					gets mapped into to create a
**					histogram element).
**		.db_prec		Precision/Scale of data value being
**					created.  (Must be consistent
**					with what is expected for the
**					from precision/scale (i.e.
**					what adc_hg_dtln() returns as
**					the precision/scale that a data value
**					of the from datatype, prec, and length
**					gets mapped into to create a
**					histogram element)).
**		.db_length  		Length of data value being
**					created.  (Must be consistent
**					with what is expected for the
**					from type/precision/length  (i.e.
**					what adc_hg_dtln() returns as
**					the length that a data value
**					of the from type/precision/length
**					gets mapped into to create a
**					histogram element)).
**		.db_data    		Pointer to location to put the
**					actual data for the data value
**					being created.
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
**      adc_dvhg                        The resulting data value.
**	        .db_data    		The actual data for the data
**					value created will be placed
**					here.
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
**          E_AD9999_INTERNAL_ERROR	Internal Error.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      27-jun-86 (ericj)
**          Initial creation.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	28-oct-86 (thurston)
**	    Added char and varchar support.
**	02-jan-87 (thurston)
**	    Null pad instead of blank pad for "c" input.  Also, all string types
**	    create a "char" histogram element now, not a "c".
**	09-jun-88 (jrb)
**	    Added CM calls for Kanji support.
**	19-apr-89 (mikem)
**	    Logical key development.  Added support for DB_LOGKEY_TYPE and
**	    DB_TABKEY_TYPE to adc_1helem_rti().
**	02-jun-89 (mikem)
**	    fixed logical key bug, added missing break to case statement.
**	    Also fixed up comments, hash element for logical keys are now
**	    always 8 bytes whether a table_key or object_key.
**	05-jul-89 (jrb)
**	    Added support for decimal datatype.
**	25-sep-1992 (fred)
**	    Added support for BIT types.
**	22-Oct-1992 (fred)
**	    Fixed up to match ANSI data representation.
**       6-Apr-1993 (fred)
**          Added byte string datatypes.
**      26-feb-2001 (gupsh01)
**          Added support for unicode nchar and nvarchar type.
**	24-dec-04 (inkdo01)
**	    Change NCHAR, NVARCHAR to compute collation weight as 
**	    histogrammed value.
**	9-nov-05 (inkdo01)
**	    Strip trailing blanks from NVARCHAR to fix OP03A2.
**	06-sep-06 (gupsh01)
**	    Added case for ANSI date/time types.
**	15-may-2007 (dougi)
**	    Add support for UTF8 server.
**	11-sep-07 (kibro01) b119085
**	    If the normalisation could overflow the buffer, reduce its effective
**	    length since we're limiting the histogram length anyway
**	14-Jan-08 (kiria01) b119738
**	    Corrected the buffer lengths for unicode and utf8 data and
**	    eliminated the unused pair of null bytes from collation data.
**	17-Mar-2009 (toumi01) b121809
**	    Stop treating BYTE data as CHAR, which causes problems with
**	    utf8 data.
**       8-Mar-2010 (hanal04) Bug 122974
**          Removed nfc_flag from adu_nvchr_fromutf8() and adu_unorm(). 
**          A standard interface is expected by fcn lookup / execute 
**          operations. Force NFC normalization is now achieved by temporarily 
**          updating the adf_uninorm_flag in the ADF_CB.
*/

DB_STATUS
adc_1helem_rti(
ADF_CB		   *adf_scb,
DB_DATA_VALUE      *adc_dvfrom,
DB_DATA_VALUE      *adc_dvhg)
{
    DB_STATUS       db_stat = E_DB_OK;
    u_char	    *fromp;
    u_char	    *fromendp;
    u_char	    *top;
    u_char	    *toendp;

    /* Convert a DB_DATA_VALUE to its histogram DB_DATA_VALUE representation */
    switch (adc_dvfrom->db_datatype)
    {
      case DB_CHR_TYPE:
        fromp    = (u_char *) adc_dvfrom->db_data;
	top      = (u_char *) adc_dvhg->db_data;
	fromendp = fromp + adc_dvfrom->db_length;
	toendp   = top + adc_dvhg->db_length;

	/*  Compress out spaces and null pad end */
	while (fromp < fromendp)
	    if (CMspace(fromp))
    		CMnext(fromp);
    	    else
	       	if (top + CMbytecnt(fromp) <= toendp)
		    CMcpyinc(fromp, top);
		else
		    break;

	if (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED)
	    /* Cut out before we pad buffer & leave top
	    ** at end of string for next bit.
	    */
	    goto UTF8prep;

	while (top < toendp)
    	    *top++ = '\0';

	break;

      case DB_BYTE_TYPE:
        fromp    = (u_char *) adc_dvfrom->db_data;
	top      = (u_char *) adc_dvhg->db_data;
	fromendp = fromp + adc_dvfrom->db_length;
	toendp   = top + adc_dvhg->db_length;

	/*  Move characters and blank pad end */
	while (fromp < fromendp && top + 1 <= toendp)
	    *top++ = *fromp++;

	break;

      case DB_CHA_TYPE:
	fromp    = (u_char *) adc_dvfrom->db_data;
	top      = (u_char *) adc_dvhg->db_data;
	fromendp = fromp + adc_dvfrom->db_length;
	toendp   = top + adc_dvhg->db_length;

	/*  Move characters and blank pad end */
	while (fromp < fromendp && top + CMbytecnt(fromp) <= toendp)
	    CMcpyinc(fromp, top);

	if (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED)
	    /* Cut out before we pad buffer & leave top
	    ** at end of string for next bit.
	    */
	    goto UTF8prep;

	while (top < toendp)
	    *top++ = ' ';

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
	if ((db_stat = adu_3datehmap(adf_scb, adc_dvfrom, adc_dvhg)) != E_DB_OK)
	    return(db_stat);
	break;

      case DB_BOO_TYPE:
        ((DB_ANYTYPE *)adc_dvhg->db_data)->db_booltype =
	  ((DB_ANYTYPE *)adc_dvfrom->db_data)->db_booltype;
        break;

      case DB_FLT_TYPE:
      case DB_INT_TYPE:
      case DB_LOGKEY_TYPE:
      case DB_TABKEY_TYPE:
	/*   Histogram values for floats, integers, label ids are the 
	**   same as their actual value.  Histograms for logical keys
	**   are built from their first 8 bytes.
	*/
	MEcopy((PTR) adc_dvfrom->db_data, adc_dvhg->db_length,
	       (PTR) adc_dvhg->db_data);
	break;

      case DB_DEC_TYPE:
	CVpkf(	adc_dvfrom->db_data,
		(i4)DB_P_DECODE_MACRO(adc_dvfrom->db_prec),
		(i4)DB_S_DECODE_MACRO(adc_dvfrom->db_prec),
		(f8 *)adc_dvhg->db_data);
	break;
	
      case DB_MNY_TYPE:
	if ((db_stat = adu_5mnyhmap(adf_scb, adc_dvfrom, adc_dvhg)) != E_DB_OK)
	    return(db_stat);
	break;

      case DB_VBYTE_TYPE:
	fromp	 = ((DB_TEXT_STRING *) adc_dvfrom->db_data)->db_t_text;
	fromendp = fromp + ((DB_TEXT_STRING *) adc_dvfrom->db_data)->db_t_count;
	top	 = (u_char *) adc_dvhg->db_data;
	toendp	 = top + adc_dvhg->db_length;
	
	while (fromp < fromendp && top + 1 <= toendp)
	    *top++ = *fromp++;

	break;

      case DB_TXT_TYPE:
      case DB_VCH_TYPE:
	fromp	 = ((DB_TEXT_STRING *) adc_dvfrom->db_data)->db_t_text;
	fromendp = fromp + ((DB_TEXT_STRING *) adc_dvfrom->db_data)->db_t_count;
	top	 = (u_char *) adc_dvhg->db_data;
	toendp	 = top + adc_dvhg->db_length;
	
	while (fromp < fromendp && top + CMbytecnt(fromp) <= toendp)
	    CMcpyinc(fromp, top);

	if (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED)
	    /* Cut out before we pad buffer & leave top
	    ** at end of string for next bit.
	    */
	    goto UTF8prep;

	/* If necessary, pad end with '\0' for text or ' ' for varchar */
	if (adc_dvfrom->db_datatype == DB_TXT_TYPE)
	    while (top < toendp)
		*top++ = '\0';
	else
	    while (top < toendp)
		*top++ = ' ';


	break;

      case DB_BIT_TYPE:
        fromp    = (u_char *) adc_dvfrom->db_data;
	top      = (u_char *) adc_dvhg->db_data;
	fromendp = fromp + adc_dvfrom->db_length;
	toendp   = top + adc_dvhg->db_length;

	while (fromp < fromendp)
	{
	    *top++ = *fromp++;
	    if (top > toendp)
		break;
	}

	while (top < toendp)
	    *top++ = '\0';

	break;

      case DB_VBIT_TYPE:
      {
	  i4	bit_count;
	  i4	bits_remaining;
	  i4	byte_count;

	  fromp = ((DB_BIT_STRING *) adc_dvfrom->db_data)->db_b_bits;
	  bit_count = ((DB_BIT_STRING *) adc_dvfrom->db_data)->db_b_count;
	  byte_count = bit_count / BITSPERBYTE;
	  bits_remaining = bit_count % BITSPERBYTE;
	  fromendp = fromp + byte_count;
	  top = (u_char *) adc_dvhg->db_data;
	  toendp = top + adc_dvhg->db_length;

	  while (fromp < fromendp)
	  {
	      *top++ = *fromp++;
	      if (top > toendp)
		  break;
	  }
	  
	  if (top < toendp)
	  {
	      *top++ = *fromp & ~(~0 >> bits_remaining);
	      
	      while (top < toendp)
		  *top++ = '\0';
	  }
	  break;
      }

     case DB_NCHR_TYPE:
     case DB_NVCHR_TYPE:
     {
	    DB_DATA_VALUE	res_dv;
            DB_TEXT_STRING	*txt;
	    char		cw[DB_MAX_HIST_LENGTH+DB_CNTSIZE];
	    i2			save_len;

	    /* NOTE res_dv will ve set as VBYTE to get the variable
	    ** length data noted
	    */
	    res_dv.db_data = cw;
	    res_dv.db_datatype = DB_VBYTE_TYPE;
	    res_dv.db_length = sizeof(cw);
	    res_dv.db_prec = 0;
	    res_dv.db_collID = -1;

	    /* Ensure we don't ask for more than we'll use */
	    if (res_dv.db_length > adc_dvhg->db_length + DB_CNTSIZE)
		res_dv.db_length = adc_dvhg->db_length + DB_CNTSIZE;

	    /* Strip trailing blanks from NVARCHAR. */
	    if (adc_dvfrom->db_datatype == DB_NVCHR_TYPE)
	    {
		i2	i;
		UCS2	*u = ((DB_NVCHR_STRING *)adc_dvfrom->db_data)->element_array;
		save_len = ((DB_NVCHR_STRING *)adc_dvfrom->db_data)->count;

		for (i = save_len; i > 0 && u[i-1] == U_BLANK; i--)
		    /*SKIP*/;
		((DB_NVCHR_STRING *)adc_dvfrom->db_data)->count = i;
	    }

	    db_stat = adu_ucollweight(adf_scb, adc_dvfrom, &res_dv);
	    if (db_stat != E_DB_OK)
		return(db_stat);

	    /* Reset length of NVARCHAR in case caller needs it */
	    if (adc_dvfrom->db_datatype == DB_NVCHR_TYPE)
		((DB_NVCHR_STRING *)adc_dvfrom->db_data)->count = save_len;

	    /* Copy result to adc_dvhg (above we've ensured the it will
	    ** at most fill the dvhg buffer)
	    */
	    txt = (DB_TEXT_STRING *)res_dv.db_data;
	    MEcopy(txt->db_t_text, txt->db_t_count, adc_dvhg->db_data);
	    /* And zero pad output */
	    MEfill(adc_dvhg->db_length-txt->db_t_count, 0,
	             adc_dvhg->db_data+txt->db_t_count);
	    break;
     }

      default:
	return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));

    }	/* end of switch stmt */

    return(E_DB_OK);

UTF8prep:
    /* UTF8 enabled server and data type is c/text/char/varchar. Convert
    ** UTF8 to UCS2, then get collation weight (as for Unicode types).
    ** NOTE That the datatype specific handling has happened for each of
    ** the string types and been formatted into the adc_dvhg output buffer
    ** and the pointer 'top' left at the end of unpadded data (or end of
    ** truncateded buffer)
    */
    {
	DB_DATA_VALUE	res_dv, utf8_dv;
	DB_TEXT_STRING	*txt;
	char		uval[DB_MAX_HIST_LENGTH+DB_CNTSIZE];
	char		cw[DB_MAX_HIST_LENGTH+DB_CNTSIZE];
	i4		res_len, i;

	/* Set res_dv to reflect adjusted data so far upto 'top' */
	res_dv.db_data = adc_dvhg->db_data;
	res_dv.db_datatype = DB_CHA_TYPE;
	res_dv.db_length = top - (u_char*)adc_dvhg->db_data;
	res_dv.db_prec = 0;
	res_dv.db_collID = -1;

	/* utf8_dv will be used to get the expanded form of the original
	** and is likely to be a truncated representation of the data.
	*/
	utf8_dv.db_data = uval;
	utf8_dv.db_datatype = DB_NVCHR_TYPE;
	utf8_dv.db_length = sizeof(uval);
	utf8_dv.db_prec = 0;
	utf8_dv.db_collID = -1; /* UTF8 default collation? */

	if (db_stat = adu_nvchr_fromutf8(adf_scb, adc_dvfrom, &utf8_dv))
	    return(db_stat);

	/* setup res_dev now to reflect the collation buffer */
	res_dv.db_data = cw;
	res_dv.db_datatype = DB_VBYTE_TYPE;
	res_dv.db_length = sizeof(cw);
	res_dv.db_prec = 0;
	res_dv.db_collID = -1;

	/* Ensure we don't ask for more than we'll use */
	if (res_dv.db_length > adc_dvhg->db_length + DB_CNTSIZE)
	    res_dv.db_length = adc_dvhg->db_length + DB_CNTSIZE;

	/* We do not strip trailing blanks from resulting NVARCHAR
	** as all the previous datatype mangling will have left the
	** buffer as needed. Any remaining trailing spaces would be
	** needed and besides, if we end up truncating the data due
	** to buffersize, the truncation point could be in a run of
	** valid embedded spaces.
	*/

	db_stat = adu_ucollweight(adf_scb, &utf8_dv, &res_dv);
	if (db_stat != E_DB_OK)
	    return(db_stat);

	/* Copy result to adc_dvhg (above we've ensured the it will
	** at most fill the dvhg buffer)
	*/
	txt = (DB_TEXT_STRING *)res_dv.db_data;
	MEcopy(txt->db_t_text, txt->db_t_count, adc_dvhg->db_data);
	/* And zero pad output */
	MEfill(adc_dvhg->db_length-txt->db_t_count, 0,
		    adc_dvhg->db_data+txt->db_t_count);
    }

    return(E_DB_OK);

}
