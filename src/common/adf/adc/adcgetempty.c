/*
** Copyright (c) 1986, 2005, 2008 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <mh.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <adp.h>
#include    <adudate.h>
#include    <adumoney.h>
#include    <ulf.h>
#include    <adfint.h>

/**
**
**  Name: ADCGETEMPTY.C - Build "empty" or "null" data value for a datatype.
**
**  Description:
**      This file contains all of the routines necessary to
**      perform the  external ADF call "adc_getempty()" which
**      will build the "empty" (or "default-default") value for the
**      given non-nullable datatype, or the "null" value for the
**      given nullable datatype.
**
**      This file defines:
**
**          adc_getempty() - Build "empty" or "null" data value for a datatype.
**	    adc_1getempty_rti() - Actually builds the "empty" data value for a
**			    RTI datatype as oppossed to a user defined datatype.
**
**
**  History:    
**      26-feb-86 (thurston)
**          Initial creation.
**	1-apr-86 (ericj)
**	    Written and added adc__getempty()
**	25-jul-86 (thurston)
**	    Changed name of adc__getempty() to adc_1getempty_rti().
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	28-oct-86 (thurston)
**	    Added char and varchar support.
**	19-feb-87 (thurston)
**	    Added support for nullable datatypes.
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	04-aug-87 (thurston)
**	    Changed adc_1getempty_rti():  For the `date' below, I now zero the
**	    whole structure instead of just setting the .dn_status field to
**	    AD_DN_NULL. 
**	20-mar-89 (jrb)
**	    Made changes for adc_lenchk interface change.
**	19-apr-89 (mikem)
**	    Logical key development.  Added support for DB_LOGKEY_TYPE and
**	    DB_TABKEY_TYPE to adc_1getempty_rti().  An "empty" logical_key is
**	    one with all 0's.
**	28-apr-89 (fred)
**	    Altered to use ADI_DT_MAP_MACRO().
**	05-jul-89 (jrb)
**	    Added support for decimal datatype.
**	4-sep-1992 (fred)
**	    Added support for peripheral datatypes.
**	22-sep-1992 (fred)
**	    Added support for the DB_VBIT_TYPE (bit varying).
**      22-dec-1992 (stevet)
**          Added function prototype.
**      05-Apr-1993 (fred)
**          Added byte datatypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	05-feb-2001 (gupsh01)
**	    Added support for unicode character type, nchar, and vnchar.
**      09-mar-2001(gupsh01)
**          Fixed for nchar data type. 
**      03-apr-2001 (gupsh01)
**          Added support for unicode long nvarchar datatype.
**	25-aug-2005 (abbjo03)
**	    In adc_1getempty_rti(), for varchar types, set the db_t_text value
**	    to a NUL character.
**      16-oct-2006 (stial01)
**          Init empty/null value for locator datatypes
**	08-Feb-2008 (kiria01) b119885
**	    Change dn_time to dn_seconds to avoid the inevitable confusion with
**	    the dn_time in AD_DATENTRNL.
**	16-Jun-2009 (thich01)
**	    Treat GEOM type the same as LBYTE.
**      10-aug-2009 (joea)
**          Add case for DB_BOO_TYPE in adc_1getempty_rti.
**	20-Aug-2009 (thich01)
**	    Treat all spatial types the same as LBYTE.
**      09-mar-2010 (thich01)
**          Add DB_NBR_TYPE like DB_BYTE_TYPE for rtree indexing.
**/


/*{
** Name: adc_getempty()	- Build "empty" or "null" data value for a datatype.
**
** Description:
**      This function is the external ADF call "adc_getempty()" which
**      will build the "empty" (or "default-default") value for the
**      given non-nullable datatype.  This will return a zero value or an empty
**	string for most non-nullable datatypes.  However, for the date
**      datatype, an empty date will be returned.  This is sometimes referred
**	to as the NULL date, but do not confuse it with the real NULL date.
**	For a given nullable datatype, this routine will build the "null" value.
**
**      This function will be called to initialize any defaultable column that
**      was not specified in an append.  The following "default default" values
**      have been chosen to stay consistent with previous versions of INGRES:
**
**          datatype    value       description or comment
**          --------	-----       ----------------------
**          c        	"   "       A string of all blanks.
**          date	AD_DN_NULL  This is really the empty date, not a NULL.
**          i      	0           Zero.
**          f    	0.0         Floating point zero.
**          money	$0.00       Zero dollars and cents.
**          text	0:""        Zero characters:  The empty string.
**
**	With the Jupiter version of INGRES, two other datatypes are being
**	supported, and their "default default" values will be:
**
**          datatype    value       description or comment
**          --------    -----       ----------------------
**          char       	"   "       A string of all blanks.
**          varchar	0:""        Zero characters:  The empty string.
**
**	Two new datatype's added in the version after 6.1:
**	    table_key   0x000000    6 bytes of zero's
**	    object_key  0x0000...0  16 bytes of zero's
**
**      Please note that this routine will return an error if it is
**      attempted on any datatype that can not exist in an INGRES
**      relation.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adc_emptydv 			Pointer to DB_DATA_VALUE to
**                                      place the empty/null data value in.
**		.db_datatype		The datatype for the empty/null data
**					value.
**		.db_prec		The precision/scale empty/null data
**					value.
**		.db_length		The length for the empty/null data
**					value.
**		.db_data		Pointer to location to place
**					the .db_data field for the empty/null
**					data value.  Note that this will
**					often be a pointer into a tuple.
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
**      adc_emptydv                     The empty/null data value.
**              .db_data		The data for the empty/null data
**                                      value will be placed here.
**                                      value will be placed here.
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
**          E_AD2004_BAD_DTID           Datatype unknown to ADF.
**          E_AD2005_BAD_DTLEN          Length is illegal for this
**					datatype.  (NOTE, only if the routine
**					has been compiled with the xDEBUG
**					constant.
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
**	1-apr-86 (ericj)
**	    first written.
**	9-apr-86 (ericj)
**	    Ifdefed out length checking for performance reasons.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	28-oct-86 (thurston)
**	    Added char and varchar support.
**	19-feb-87 (thurston)
**	    Added support for nullable datatypes.
*/

# ifdef ADF_BUILD_WITH_PROTOS
DB_STATUS
adc_getempty(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *adc_emptydv)	/* Ptr to empty/null data value */
# else
DB_STATUS
adc_getempty( adf_scb, adc_emptydv)
ADF_CB              *adf_scb;
DB_DATA_VALUE	    *adc_emptydv;
# endif
{
    i4			bdt = abs((i4) adc_emptydv->db_datatype);
    i4			bdtv;
    DB_STATUS		db_stat;


    /* Does the datatype exist? */
    bdtv = ADI_DT_MAP_MACRO(bdt);
    if (bdtv <= 0  ||  bdtv > ADI_MXDTS
	||  Adf_globs->Adi_dtptrs[bdtv] == NULL)
	return(adu_error(adf_scb, E_AD2004_BAD_DTID, 0));

    /*
    **	  If datatype is non-nullable, call the function associated with this
    ** datatype to initialize a data value to be "empty".  Currently, this is
    ** only one function for the RTI known datatypes.  This level of indirection
    ** is provided for the implementation of user-defined datatypes in the
    ** future.
    **
    **    If datatype is nullable, we can set the null value right here since it
    ** will be done the same way for all nullable datatypes.
    */

    if (adc_emptydv->db_datatype > 0)		/* non-nullable */
    {
        db_stat = (*Adf_globs->Adi_dtptrs[ADI_DT_MAP_MACRO(bdt)]->
				    adi_dt_com_vect.adp_getempty_addr)
	    		(adf_scb, adc_emptydv);
    }
    else
    {
	ADF_SETNULL_MACRO(adc_emptydv);
	/*
	**  If the datatype is peripheral, then we clear its length as
	**  well.  This is because ADF guarantees that the lengths are
	**  always correct for peripheral datatypes
	*/

	if (Adf_globs->Adi_dtptrs[ADI_DT_MAP_MACRO(bdt)]->
		    adi_dtstat_bits & AD_PERIPHERAL)
	{
	    ((ADP_PERIPHERAL *) adc_emptydv->db_data)->per_length0 = 0;
	    ((ADP_PERIPHERAL *) adc_emptydv->db_data)->per_length1 = 0;
	    ((ADP_PERIPHERAL *) adc_emptydv->db_data)->per_tag = ADP_P_COUPON;
	}
	db_stat = E_DB_OK;
    }

    return(db_stat);
}


/*
** Name: adc_1getempty_rti() - Build the "empty" data value for a RTI datatypes.
**
** Description:
**      This function is the external ADF call "adc_1getempty_rti()" which
**      will build the "empty" (or "default-default") value for the
**      given datatype.  This will return a zero value or an empty
**	string for most datatypes.  However, for the date datatype, an
**      empty date will be returned.  This is sometimes referred to as
**	the NULL date, but do not confuse it with the real NULL date.
**
**      This function will be called to initialize any non-nullable, defaultable
**      column that was not specified in an append.  The following "default
**      default" values have been chosen to stay consistent with previous
**      versions of INGRES:
**
**          datatype	value	description or comment
**          --------	-----	----------------------
**          c        	"   "	A string of all blanks.
**          date	AD_DN_NULL This is really the empty date, not a NULL.
**          i      	0	Zero.
**          f    	0.0	Floating point zero.
**          money	$0.00	Zero dollars and cents.
**          text	0:""	Zero characters:  The empty string.
**
**	With the Jupiter version of INGRES, two other datatypes are being
**	supported, and their "default default" values will be:
**
**          datatype	value	description or comment
**          --------	-----	----------------------
**          char       	"   "	A string of all blanks.
**          varchar	0:""	Zero characters:  The empty string.
**
**      Please note that this routine will return an error if it is
**      attempted on any datatype that can not exist in an INGRES
**      relation.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adc_emptydv 			Pointer to DB_DATA_VALUE to
**                                      place the empty data value in.
**		.db_datatype		The datatype for the empty data
**					value.
**		.db_prec		The precision/scale for the empty data
**					value.
**		.db_length		The length for the empty data
**					value.
**		.db_data		Pointer to location to place
**					the .db_data field for the empty
**					data value.  Note that this will
**					often be a pointer into a tuple.
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
**      adc_emptydv                     The empty data value.
**              .db_data		The data for the empty data
**                                      value will be placed here.
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
**          E_AD2004_BAD_DTID           Datatype unknown to ADF.
**          E_AD2005_BAD_DTLEN          Length is illegal for this
**					datatype.  (NOTE, only if the routine
**					has been compiled with the xDEBUG
**					constant.
**	    E_AD200B_BAD_PREC		Invalid precision specified for DECIMAL;
**					must be: 1<= precision <=DB_MAX_DECPREC
**	    E_AD200C_BAD_SCALE		Invalid scale specified for DECIMAL;
**					must be: 0<= scale <=precision
**	    E_AD9999_INTERNAL_ERROR	Internal error
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
**
** History:
**      26-feb-86 (thurston)
**          Initial creation.
**	1-apr-86 (ericj)
**	    first written.
**	9-apr-86 (ericj)
**	    Ifdefed out length checking for performance reasons.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	28-oct-86 (thurston)
**	    Added char and varchar support.
**	04-aug-87 (thurston)
**	    In the `date' case below, I now zero the whole structure instead
**	    of just setting the .dn_status field to AD_DN_NULL.
**	19-apr-89 (mikem)
**	    Logical key development.  Added support for DB_LOGKEY_TYPE and
**	    DB_TABKEY_TYPE to adc_1getempty_rti().  An "empty" logical_key is
**	    one with all 0's.
**	05-jul-89 (jrb)
**	    Added decimal datatype support.
**	04-sep-1992 (fred)
**	    Added support for long varchar datatypes.
**	22-sep-1992 (fred)
**	    Added support for DB_VBIT_TYPE.
**      05-Apr-1993 (fred)
**          Added support for byte datatypes.
**      05-feb-2001 (gupsh01)
**          Added support for unicode character type, nchar, and vnchar.
**      03-apr-2001 (gupsh01)
**          Added support for unicode long nvarchar datatype.
**	12-may-04 (inkdo01)
**	    Added support for bigint.
**	15-jun-2006 (gupsh01)
**	    Added support for new ANSI datetime types.
**	15-may-2007 (dougi)
**	    Change NCHR default to blank from 0.
**	18-Feb-2008 (kiria01) b120004
**	    Consolidate timezone handling. This involves using the macros
**	    for dealing with the raw TZ fields.
**	24-Jun-2009 (gupsh01)
**	    Fix setting the locator values at constant offset of
**	    ADP_HDR_SIZE in the ADP_PERIPHERAL structure, irrespective
**	    of the platform. This was not happening for 64 bit platforms
**	    where per_value was aligned on ALIGN_RESTRICT boundary, and
**	    his causes problems for API / JDBC etc as it expects it to
**	    to be at the constant offset irrespective of the platform.
*/

DB_STATUS
adc_1getempty_rti(
ADF_CB              *adf_scb,
DB_DATA_VALUE       *adc_emptydv)	/* Ptr to empty data value */
{

#ifdef	xDEBUG
    DB_STATUS   db_stat;

    /* check the validity of the length of the data value */
    if (db_stat = adc_lenchk(adf_scb, FALSE, adc_emptydv, NULL))
	return(db_stat);
#endif	/* xDEBUG */

    /* fill up the data value with the "default-default" value. */
    switch (adc_emptydv->db_datatype)
    {
      case DB_CHA_TYPE:
      case DB_CHR_TYPE:
	/* fill character data with blanks. */
	MEfill(adc_emptydv->db_length, (u_char)' ', adc_emptydv->db_data);
	break;

      case  DB_NCHR_TYPE:
            *(UCS2 *) adc_emptydv->db_data = U_BLANK;
	break;

      case DB_LOGKEY_TYPE:
      case DB_TABKEY_TYPE:
	/* fill character data with zeros. */
	MEfill(adc_emptydv->db_length, (u_char)0, adc_emptydv->db_data);
	break;

      case DB_DTE_TYPE:
	{
	    AD_DATENTRNL    *dp = (AD_DATENTRNL *) adc_emptydv->db_data;

	    /* Date datatypes are defaultable by set status to AD_DN_NULL. */
	    dp->dn_status   = AD_DN_NULL;
	    dp->dn_year	    = 0;
	    dp->dn_month    = 0;
	    dp->dn_highday  = 0;
	    dp->dn_lowday   = 0;
	    dp->dn_time	    = 0;
	}
	break;

      case DB_ADTE_TYPE:
        {
	     AD_ADATE *ad = (AD_ADATE *) adc_emptydv->db_data;

	     ad->dn_year = 0;
	     ad->dn_month = 0;
	     ad->dn_day = 0;
        }
        break;

       case DB_TMWO_TYPE:
       case DB_TMW_TYPE:
       case DB_TME_TYPE:
        {
	     AD_TIME *adtm = (AD_TIME *) adc_emptydv->db_data;

	     adtm->dn_seconds = 0;
	     adtm->dn_nsecond = 0;
	     AD_TZ_SET(adtm, 0);
        }
        break;

       case DB_TSWO_TYPE:
       case DB_TSW_TYPE:
       case DB_TSTMP_TYPE:
        {
	     AD_TIMESTAMP *atstmp = (AD_TIMESTAMP *) adc_emptydv->db_data;

	     atstmp->dn_year = 0;
	     atstmp->dn_month = 0;
	     atstmp->dn_day = 0;
	     atstmp->dn_seconds = 0;
	     atstmp->dn_nsecond = 0;
	     AD_TZ_SET(atstmp, 0);
        }
        break;

       case DB_INYM_TYPE:
        {
	     AD_INTYM *aintym = (AD_INTYM *) adc_emptydv->db_data;

	     aintym->dn_years = 0;
	     aintym->dn_months = 0;
        }
        break;

       case DB_INDS_TYPE:
        {
	     AD_INTDS *aintds = (AD_INTDS *) adc_emptydv->db_data;

	     aintds->dn_days = 0;
	     aintds->dn_seconds = 0;
	     aintds->dn_nseconds = 0;
        }
        break;

      case DB_BOO_TYPE:
        ((DB_ANYTYPE *)adc_emptydv->db_data)->db_booltype = DB_FALSE;
        break;

      case DB_INT_TYPE:
	if (adc_emptydv->db_length == 1)
	    *(i1 *) adc_emptydv->db_data = 0;
	else if (adc_emptydv->db_length == 2)
	    *(i2 *) adc_emptydv->db_data = 0;
	else if (adc_emptydv->db_length == 4)
	    *(i4 *) adc_emptydv->db_data = 0;
	else if (adc_emptydv->db_length == 8)
	    *(i8 *) adc_emptydv->db_data = 0;
	break;

      case DB_DEC_TYPE:
	MEfill(adc_emptydv->db_length, (u_char)0, adc_emptydv->db_data);
	((u_char *)adc_emptydv->db_data)[adc_emptydv->db_length-1] = MH_PK_PLUS;
	break;

      case DB_FLT_TYPE:
	if (adc_emptydv->db_length == 4)
	    *(f4 *) adc_emptydv->db_data = 0.0;
	else
	    *(f8 *) adc_emptydv->db_data = 0.0;
	break;

      case DB_MNY_TYPE:
	((AD_MONEYNTRNL *) adc_emptydv->db_data)->mny_cents= 0.0;
	break;

      case DB_VCH_TYPE:
      case DB_TXT_TYPE:
      case DB_LTXT_TYPE:
      case DB_VBYTE_TYPE:
	((DB_TEXT_STRING *) adc_emptydv->db_data)->db_t_count = 0;
	*((DB_TEXT_STRING *)adc_emptydv->db_data)->db_t_text = '\0';
	break;

      case DB_LVCH_TYPE:
      case DB_LBYTE_TYPE:
      case DB_GEOM_TYPE:
      case DB_POINT_TYPE:
      case DB_MPOINT_TYPE:
      case DB_LINE_TYPE:
      case DB_MLINE_TYPE:
      case DB_POLY_TYPE:
      case DB_MPOLY_TYPE:
      case DB_GEOMC_TYPE:
      case DB_LBIT_TYPE:
      case DB_LNVCHR_TYPE:
	{
	    ADP_PERIPHERAL	*periph = (ADP_PERIPHERAL *)
						adc_emptydv->db_data;
	    periph->per_tag = ADP_P_DATA;
	    periph->per_length0 = periph->per_length1 = 0;
	}
	break;

      case DB_LBLOC_TYPE:
      case DB_LCLOC_TYPE:
      case DB_LNLOC_TYPE:
	{
	    ADP_PERIPHERAL	*periph = (ADP_PERIPHERAL *)
						adc_emptydv->db_data;
	    periph->per_tag = ADP_P_LOC_L_UNK;
	    periph->per_length0 = periph->per_length1 = 0;
	    MEfill (sizeof(ADP_LOCATOR), 0, ((char *)periph + ADP_HDR_SIZE)); 
	}
	break;
	

      case DB_VBIT_TYPE:
	((DB_BIT_STRING *) adc_emptydv->db_data)->db_b_count = 0;
	break;

      case DB_BIT_TYPE:
      case DB_BYTE_TYPE:
      case DB_NBR_TYPE:
	MEfill(adc_emptydv->db_length, (u_char)0, adc_emptydv->db_data);
	break;

      case DB_NVCHR_TYPE:
	((DB_NVCHR_STRING *) adc_emptydv->db_data)->count = 0;
	break;

      default:
	return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    }
    return(E_DB_OK);
}
