/*
** Copyright (c) 2004,2008, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <mh.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ade.h>	    /* Only needed for ADE_LEN_UNKNOWN */
#include    <adudate.h>
#include    <adumoney.h>
#include    <ulf.h>
#include    <adfint.h>

/**
**
**  Name: ADCMINMAXDV.C - Build min and max value or length for given datatype.
**
**  Description:
**      This file contains all of the routines necessary to
**      perform the  external ADF call "adc_minmaxdv()" which
**      will build the "minimum" and "maximum" data value for the
**      given datatype, or generate the minimum and maximum length
**	for the given datatype.
**
**      This file defines:
**
**          adc_minmaxdv() - Build min and max value or length for given
**			     datatype.
**	    adc_1minmaxdv_rti() - Actually builds the min and max value or
**				  length for the given RTI datatype as opposed
**				  to a user defined datatype.
**
**
**  History:
**      19-jun-87 (thurston)
**          Initial creation.
**	01-sep-88 (thurston)
**	    Changed the use of DB_CHAR_MAX and DB_MAXSTRING to
**	    adf_scb->adf_maxstring, since this is now a session settable
**	    parameter.
**	20-mar-89 (jrb)
**	    Made changes for adc_lenchk interface change.
**	19-apr-89 (mikem)
**	    Logical key development.  Added support for DB_LOGKEY_TYPE and 
**	    DB_TABKEY_TYPE to adc_1minmaxdv_rti().
**	28-apr-89 (fred)
**	    Altered to use ADI_DT_MAP_MACRO().
**	05-jun-89 (fred)
**	    Completed last change.
**	06-jul-89 (jrb)
**	    Added decimal support.
**	23-aug-89 (jrb)
**	    Changed FMIN to -FMAX to fix critical bug where query was returning
**	    incorrect answer for keys built on a float column.
**      22-dec-1992 (stevet)
**          Added function prototype.
**       6-Apr-1993 (fred)
**          Added byte string types.
**	24-mar-1993 (robf)
**	    Fixed maximum value for security label, was processed as string
**	    rather than fixed size variable.
**       1-Jul-1993 (fred)
**          Removed (bool) cast from adc_lenchk() call, since the type
**          of the argument is now a nat.  See adf.h for reasoning...
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**      26-feb-96 (thoda04)
**          Cast added in adc_1minmaxdv_rti() to avoid arithmetic overflow 
**          in 16 bit machines (i.e. Windows 3.1).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**       7-mar-2001 (mosjo01)
**          Had to make MAX_CHAR more unique, IIMAX_CHAR_VALUE, due to conflict
**          between compat.h and system limits.h on (sqs_ptx).
**      23-feb-2001 (gupsh01)
**          Added support for new data type, nchar and nvarchar.
**	05-feb-2001 (gupsh01)
**	    Modified the length calculation to use II_COUNT_ITEMS for 
**	    max data for ADF_NVCHR_TYPE. 
**	08-Feb-2008 (kiria01) b119885
**	    Change dn_time to dn_seconds to avoid the inevitable confusion with
**	    the dn_time in AD_DATENTRNL.
**      14-aug-2009 (joea)
**          Change the DB_BOO_TYPE cases in adc_1minmaxdv_rti.
**      09-mar-2010 (thich01)
**          Add DB_NBR_TYPE like DB_BYTE_TYPE for rtree indexing.
*/

/*{
** Name: adc_minmaxdv() - Build min and max value or length for given datatype.
**
** Description:
**      This function is the external ADF call "adc_minmaxdv()" which
**      will build the "minimum" and "maximum" data value for the
**      given datatype, or generate the minimum and maximum length
**	for the given datatype.
**
**	Here are the rules:
**	-------------------
**	Let MIN_DV and MAX_DV represent the input parameters; both are pointers
**	to DB_DATA_VALUEs.  The lengths for MIN_DV and MAX_DV may be different,
**	but their datatypes MUST be the same.
**
**	Rule 1:
**          If MIN_DV (or MAX_DV) is NULL, then the processing for MIN_DV (or
**          MAX_DV) will be completely skipped.  This allows the caller who is
**          only interested in the `maximum' (or `minimum') to more effeciently
**          use this routine.
**	Rule 2:
**          If the .db_length field of *MIN_DV (or *MAX_DV) is supplied as
**          ADE_LEN_UNKNOWN, then no minimum (or maximum) value will be built
**          and placed at MIN_DV->db_data (or MAX_DV->db_data).  Instead, the
**          minimum (or maximum) valid internal length will be returned in
**          MIN_DV->db_length (or MAX_DV->db_length).
**	Rule 3:
**          If MIN_DV->db_data (or MAX_DV->db_data) is NULL, then then no
**          minimum (or maximum) value will be built and placed at
**          MIN_DV->db_data (or MAX_DV->db_data).
**	Rule 4:
**          If none of rules 1-3 applies to MIN_DV (or MAX_DV), then the minimum
**          (or maximum) non-null value for the datatype/length will be built
**          and placed at MIN_DV->db_data (or MAX_DV->db_data). 
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	    .adf_maxstring		Max user size of any INGRES string.
**      adc_mindv 			Pointer to DB_DATA_VALUE for the `min'.
**					If this is NULL, `min' processing will
**					be skipped.
**		.db_datatype		Its datatype.  Must be the same as
**					datatype for `max'.
**		.db_prec		Its precision/scale.
**		.db_length		The length to build the `min' value for,
**					or ADE_LEN_UNKNOWN, if the `min' length
**					is requested.
**		.db_data		Pointer to location to place the `min'
**					non-null value, if requested.  If this
**					is NULL no `min' value will be created.
**      adc_maxdv 			Pointer to DB_DATA_VALUE for the `max'.
**					If this is NULL, `max' processing will
**					be skipped.
**		.db_datatype		Its datatype.  Must be the same as
**					datatype for `min'.
**		.db_prec		Its precision/scale.
**		.db_length		The length to build the `max' value for,
**					or ADE_LEN_UNKNOWN, if the `max' length
**					is requested.
**		.db_data		Pointer to location to place the `max'
**					non-null value, if requested.  If this
**					is NULL no `max' value will be created.
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
**      adc_mindv 			If this was supplied as NULL, `min'
**					processing will be skipped.
**		.db_length		If this was supplied as ADE_LEN_UNKNOWN,
**					then the `min' valid internal length for
**					this datatype will be returned here.
**		.db_data		If this was supplied as NULL, or if the
**					.db_length field was supplied as
**					ADE_LEN_UNKNOWN, nothing will be
**					returned here.  Otherwise, the `min'
**					non-null value for this datatype/length
**					will be built and placed at the location
**					pointed to by .db_data.
**      adc_maxdv 			If this was supplied as NULL, `max'
**					processing will be skipped.
**		.db_length		If this was supplied as ADE_LEN_UNKNOWN,
**					then the `max' valid internal length for
**					this datatype will be returned here.
**		.db_data		If this was supplied as NULL, or if the
**					.db_length field was supplied as
**					ADE_LEN_UNKNOWN, nothing will be
**					returned here.  Otherwise, the `max'
**					non-null value for this datatype/length
**					will be built and placed at the location
**					pointed to by .db_data.
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
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      19-jun-87 (thurston)
**          Initial creation.
**      09-nov-2010 (gupsh01) SIR 124685
**          Protype cleanup.
*/

DB_STATUS
adc_minmaxdv(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *adc_mindv,
DB_DATA_VALUE	    *adc_maxdv)
{
    i4			dt;
    i4			bdt;
    i4			mbdt;
    DB_STATUS		db_stat;
    DB_DATA_VALUE	tmp_mindv;
    DB_DATA_VALUE	tmp_maxdv;
    DB_DATA_VALUE	*ptr_mindv;
    DB_DATA_VALUE	*ptr_maxdv;


    /* First, check to see if both min and max ptrs are NULL */
    /* ----------------------------------------------------- */
    if (adc_mindv == NULL  &&  adc_maxdv == NULL)
	return(E_DB_OK);    /* no work to be done */

    /* If both are non-NULL, then do their datatypes match? */
    /* ---------------------------------------------------- */
    if (    adc_mindv != NULL
	&&  adc_maxdv != NULL
	&&  adc_mindv->db_datatype != adc_maxdv->db_datatype
       )
	return(adu_error(adf_scb, E_AD3001_DTS_NOT_SAME, 0));

    /* Does the datatype exist? */
    /* ------------------------ */
    dt = (adc_mindv != NULL ? adc_mindv->db_datatype : adc_maxdv->db_datatype);
    bdt = abs(dt);
    mbdt = ADI_DT_MAP_MACRO(bdt);
    if (mbdt <= 0  ||  mbdt > ADI_MXDTS
		||  Adf_globs->Adi_dtptrs[mbdt] == NULL)
	return(adu_error(adf_scb, E_AD2004_BAD_DTID, 0));


    /* Now find the correct function to call to do the processing */
    /* ---------------------------------------------------------- */
    ptr_mindv = adc_mindv;
    ptr_maxdv = adc_maxdv;
    if (dt < 0)		/* nullable */
    {
	if (adc_mindv != NULL)
	{
	    STRUCT_ASSIGN_MACRO(*adc_mindv, tmp_mindv);
	    tmp_mindv.db_datatype = bdt;
	    tmp_mindv.db_length--;
	    ptr_mindv = &tmp_mindv;

	    if (    adc_mindv->db_length != ADE_LEN_UNKNOWN
		&&  adc_mindv->db_data != NULL
	       )
		ADF_CLRNULL_MACRO(adc_mindv);
	}

	if (adc_maxdv != NULL)
	{
	    STRUCT_ASSIGN_MACRO(*adc_maxdv, tmp_maxdv);
	    tmp_maxdv.db_datatype = bdt;
	    tmp_maxdv.db_length--;
	    ptr_maxdv = &tmp_maxdv;

	    if (    adc_maxdv->db_length != ADE_LEN_UNKNOWN
		&&  adc_maxdv->db_data != NULL
	       )
		ADF_CLRNULL_MACRO(adc_maxdv);
	}
    }

    db_stat = (*Adf_globs->Adi_dtptrs[mbdt]->
		adi_dt_com_vect.adp_minmaxdv_addr)
		    (adf_scb, ptr_mindv, ptr_maxdv);

    return(db_stat);
}


/*
** Name: adc_1minmaxdv_rti() - Actually builds the min and max value or length
**			       length for the given RTI datatype as oppossed to
**			       a user defined datatype.
**
** Description:
**	This function is the internal routine to perform the adc_minmaxdv()
**	algorithm for the RTI datatypes.  See description of that general
**	routine above for more detail.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	    .adf_maxstring		Max user size of any INGRES string.
**      adc_mindv 			Pointer to DB_DATA_VALUE for the `min'.
**					If this is NULL, `min' processing will
**					be skipped.
**		.db_datatype		Its datatype.  Must be the same as
**					datatype for `max'.
**		.db_prec		Its precision/scale.
**		.db_length		The length to build the `min' value for,
**					or ADE_LEN_UNKNOWN, if the `min' length
**					is requested.
**		.db_data		Pointer to location to place the `min'
**					non-null value, if requested.  If this
**					is NULL no `min' value will be created.
**      adc_maxdv 			Pointer to DB_DATA_VALUE for the `max'.
**					If this is NULL, `max' processing will
**					be skipped.
**		.db_datatype		Its datatype.  Must be the same as
**					datatype for `min'.
**		.db_prec		Its precision/scale.
**		.db_length		The length to build the `max' value for,
**					or ADE_LEN_UNKNOWN, if the `max' length
**					is requested.
**		.db_data		Pointer to location to place the `max'
**					non-null value, if requested.  If this
**					is NULL no `max' value will be created.
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
**      adc_mindv 			If this was supplied as NULL, `min'
**					processing will be skipped.
**		.db_length		If this was supplied as ADE_LEN_UNKNOWN,
**					then the `min' valid internal length for
**					this datatype will be returned here.
**		.db_data		If this was supplied as NULL, or if the
**					.db_length field was supplied as
**					ADE_LEN_UNKNOWN, nothing will be
**					returned here.  Otherwise, the `min'
**					non-null value for this datatype/length
**					will be built and placed at the location
**					pointed to by .db_data.
**      adc_maxdv 			If this was supplied as NULL, `max'
**					processing will be skipped.
**		.db_length		If this was supplied as ADE_LEN_UNKNOWN,
**					then the `max' valid internal length for
**					this datatype will be returned here.
**		.db_data		If this was supplied as NULL, or if the
**					.db_length field was supplied as
**					ADE_LEN_UNKNOWN, nothing will be
**					returned here.  Otherwise, the `max'
**					non-null value for this datatype/length
**					will be built and placed at the location
**					pointed to by .db_data.
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
**      19-jun-87 (thurston)
**          Initial creation.
**	01-sep-88 (thurston)
**	    Changed the use of DB_CHAR_MAX and DB_MAXSTRING to
**	    adf_scb->adf_maxstring, since this is now a session settable
**	    parameter.
**	19-apr-89 (mikem)
**	    Logical key development.  Added support for DB_LOGKEY_TYPE and 
**	    DB_TABKEY_TYPE to adc_1minmaxdv_rti().
**	06-jul-89 (jrb)
**	    Added decimal support.
**	23-aug-89 (jrb)
**	    Changed FMIN to -FMAX to fix critical bug where query was returning
**	    incorrect answer for keys built on a float column.
**       6-Apr-1993 (fred)
**          Added byte string types.
**      15-jan-95 (inkdo01)
**          Set precision for max, as well as min - fixes b66210.
**      26-feb-96 (thoda04)
**          Cast added to DB_LTXT_TYPE case to avoid arithmetic overflow 
**          in 16 bit machines (i.e. Windows 3.1).
**      23-feb-2001 (gupsh01)
**          Added support for new data type, nchar and nvarchar.
**	05-feb-2001 (gupsh01)
**	    Corrected DB_NVCHR_TYPE to use II_COUNT_ITEMS macro when
**	    calculating length of unicode data type.
**	06-feb-2001 (gupsh01)
**	    Fixed the minimum and maximum value building for NCHAR and 
**	    NVARCHAR case.
**	12-may-04 (inkdo01)
**	    Added support for bigint.
**	15-jun-2006 (gupsh01)
**	    Added support for ANSI datetime data types.
**	8-dec-2006 (dougi)
**	    Add max values for new date/time types.
**	21-aug-2007 (gupsh01)
**	    Properly form the max key for UTF8 datatypes. 
**	7-sep-2007 (dougi)
**	    Fix assignment to VCH for UTF8 enabled.
**	14-sep-2007 (gupsh01)
**	    Reassign the max keys for the UTF8 installation.
**	    The new keys are based on the maximum weight in the
**	    collation table, rather than just the maximum code
**	    point. 
**	27-sep-2007 (gupsh01)
**	    Add support for handling text types in UTF8 installation.
**	08-Mar-2008 (kiria01) b120086
**	    Corrected 3 cut & paste errors that wopuld have resulted
**	    at best in an uninitialised adc_maxdv->db_length and at
**	    worse a segv through dereferencing adc_mindv having
**	    checked adc_maxdv.
**	1-Apr-2008 (kibro01) b120193
**	    Use i2 values for lengths of nvarchar
**	01-Oct-2008 (gupsh01)
**	    Break out the case for byte/varbyte from char/varchar
**	    which should not be checked against restricted adf_maxstring 
**	    length in UTF8 installations.
**	19-Nov-2010 (kiria01) SIR 124690
**	    Add support for UCS_BASIC collation. No need for the reduced length
**	    as it doesn't use UCS2 CEs.
*/

DB_STATUS
adc_1minmaxdv_rti(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *adc_mindv,
DB_DATA_VALUE	    *adc_maxdv)
{
    DB_STATUS		db_stat = E_DB_OK;
    DB_DT_ID		dt;
    i4			prec;
    i4		len;
    PTR			data;
    u_char		*pk;
    UCS2 		*cpuni; 
    DB_NVCHR_STRING     *chr;
    u_i2 		zero = 0;

    /* Process the `min' */
    /* ----------------- */
    if (adc_mindv != NULL)
    {
	dt   = adc_mindv->db_datatype;
	prec = DB_P_DECODE_MACRO(adc_mindv->db_prec);
	len  = adc_mindv->db_length;
	data = adc_mindv->db_data; 

	if (len == ADE_LEN_UNKNOWN)
	{
	    /* Set the `min' length */
	    /* -------------------- */
	    switch (dt)
	    {
	      case DB_BOO_TYPE:
                adc_mindv->db_length = 1;
		break;

	      case DB_FLT_TYPE:
		adc_mindv->db_length = 4;
		break;

	      case DB_INT_TYPE:
	      case DB_CHR_TYPE:
	      case DB_CHA_TYPE:
	      case DB_DEC_TYPE:
	      case DB_BIT_TYPE:
	      case DB_BYTE_TYPE:
              case DB_NBR_TYPE:
		adc_mindv->db_length = 1;
		break;

	      case DB_TXT_TYPE:
	      case DB_VCH_TYPE:
	      case DB_LTXT_TYPE:
	      case DB_VBYTE_TYPE:
		adc_mindv->db_length = DB_CNTSIZE;
		break;

	      case DB_VBIT_TYPE:
	      {
		DB_BIT_STRING	bs;

		adc_mindv->db_length = sizeof(bs.db_b_count);
		break;
	      }

	      case DB_MNY_TYPE:
		adc_mindv->db_length = ADF_MNY_LEN;
		break;

	      case DB_DTE_TYPE:
		adc_mindv->db_length = ADF_DTE_LEN;
		break;

	      case DB_LOGKEY_TYPE:
		adc_mindv->db_length = DB_LEN_OBJ_LOGKEY;
		break;

	      case DB_TABKEY_TYPE:
		adc_mindv->db_length = DB_LEN_TAB_LOGKEY;
		break;

	      case DB_NCHR_TYPE:
              case DB_NVCHR_TYPE:
		adc_mindv->db_length = sizeof(u_i2);
		break;

	      default:
		return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	    }
	}
	else if (data != NULL)
	{
	    /* Build the `min' value */
	    /* --------------------- */

	    /* First, make sure supplied length is valid for this datatype */
	    /* ----------------------------------------------------------- */
	    if (db_stat = adc_lenchk(adf_scb, FALSE, adc_mindv, NULL))
		return(db_stat);

	    /* Now, we can build the `min' value */
	    /* --------------------------------- */
	    switch (dt)
	    {
	      case DB_BOO_TYPE:
                ((DB_ANYTYPE *)data)->db_booltype = DB_FALSE;
		break;

	      case DB_FLT_TYPE:
		if (len == 4)
		    *(f4 *)data = -FMAX;
		else
		    *(f8 *)data = -FMAX;
		break;

	      case DB_DEC_TYPE:
	        pk = (u_char *)data;

		/* fill first byte in depending on parity of prec */
	        if ((prec & 1) == 0)
		    *pk = 0x09;
		else
		    *pk = 0x99;

		/* fill intermediate bytes if there are any */
		if (len >= 3)
		    MEfill(len-2, (u_char)0x99, (PTR)(pk+1));

		/* place minus sign */
		pk[len-1] = 0x90 | MH_PK_MINUS;
		break;

	      case DB_INT_TYPE:
		switch (len)
		{
		  case 1:
		    *(i1 *)data = MINI1;
		    break;
		  case 2:
		    *(i2 *)data = MINI2;
		    break;
		  case 4:
		    *(i4 *)data = MINI4;
		    break;
		  case 8:
		    *(i8 *)data = MINI8;
		    break;
		}
		break;

	      case DB_CHR_TYPE:
		MEfill(len, (u_char) MIN_CHAR, (PTR) data);
		break;

	      case DB_CHA_TYPE:
	      case DB_BIT_TYPE:
	      case DB_BYTE_TYPE:
              case DB_NBR_TYPE:
		MEfill(len, (u_char) 0, (PTR) data);
		break;

	      case DB_TXT_TYPE:
		((DB_TEXT_STRING *) data)->db_t_count = 0;
		break;

	      case DB_NCHR_TYPE:
		MEfill(len, (UCS2) AD_MIN_UNICODE , (PTR) data);
		break;

	      case DB_NVCHR_TYPE:
		if (adc_mindv->db_data)
		{
  		  i2             i = 0;
  		  UCS2	min = AD_MIN_UNICODE;
  		  i2           s = 0;

	  	  chr = (DB_NVCHR_STRING *) adc_mindv->db_data;
		  s = II_COUNT_ITEMS((adc_mindv->db_length - DB_CNTSIZE), UCS2);

  		  for (i = 0; i < s; i++)
  		  {
    		    I2ASSIGN_MACRO(min, chr->element_array[i]);
  		  }
		  I2ASSIGN_MACRO(s, chr->count);
		}
		break;

	      case DB_VCH_TYPE:
	      case DB_LTXT_TYPE:
	      case DB_VBYTE_TYPE:
		((DB_TEXT_STRING *) data)->db_t_count = len - DB_CNTSIZE;
		if (len > DB_CNTSIZE)
		{
		    MEfill(len - DB_CNTSIZE, (u_char) 0,
			   (PTR) ((DB_TEXT_STRING *) data)->db_t_text);
		}
		break;

	      case DB_VBIT_TYPE:
		((DB_BIT_STRING *) data)->db_b_count = len - DB_BCNTSIZE;
		if (len > DB_BCNTSIZE)
		{
		    MEfill(len - DB_BCNTSIZE, (u_char) 0,
			   (PTR) ((DB_BIT_STRING *) data)->db_b_bits);
		}
		break;

	      case DB_MNY_TYPE:
		((AD_MONEYNTRNL *) data)->mny_cents = AD_3MNY_MIN_NTRNL;
		break;

	      case DB_DTE_TYPE:
		{
		    AD_DATENTRNL    *d = (AD_DATENTRNL *) data;

		    d->dn_status = AD_DN_NULL;
		    d->dn_highday = 0;
		    d->dn_year = 0;
		    d->dn_month = 0;
		    d->dn_lowday = 0;
		    d->dn_time = 0;
		}
		break;

	      case DB_ADTE_TYPE:
		{
		    AD_ADATE *ad = (AD_ADATE *) data;
		    ad->dn_year = 0;
		    ad->dn_month = 0;
		    ad->dn_day = 0;
		}
		break;

	      case DB_TMWO_TYPE:
       	      case DB_TMW_TYPE:
       	      case DB_TME_TYPE:
        	{
	   	    AD_TIME *adtm = (AD_TIME *) data;

	     	    adtm->dn_seconds = 0;
	     	    adtm->dn_nsecond = 0;
        	}
        	break;

              case DB_TSWO_TYPE:
              case DB_TSW_TYPE:
              case DB_TSTMP_TYPE:
               {
	            AD_TIMESTAMP *atstmp = (AD_TIMESTAMP *) data;

	            atstmp->dn_year = 0;
	            atstmp->dn_month = 0;
	            atstmp->dn_day = 0;
	            atstmp->dn_seconds = 0;
	            atstmp->dn_nsecond = 0;
               }
               break;

              case DB_INYM_TYPE:
               {
	            AD_INTYM *aintym = (AD_INTYM *) data;

	            aintym->dn_years = 0;
	            aintym->dn_months = 0;
               }
               break;

              case DB_INDS_TYPE:
               {
	            AD_INTDS *aintds = (AD_INTDS *) data;

	            aintds->dn_days = 0;
	            aintds->dn_seconds = 0;
	            aintds->dn_nseconds = 0;
               }
               break;

	      case DB_LOGKEY_TYPE:
	      case DB_TABKEY_TYPE:

		MEfill(len, (u_char) 0, (PTR) data);
		break;

	      default:
		return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	    }
	}
    }


    /* Process the `max' */
    /* ----------------- */
    if (adc_maxdv != NULL)
    {
	dt   = adc_maxdv->db_datatype;
	prec = DB_P_DECODE_MACRO(adc_maxdv->db_prec);
	len  = adc_maxdv->db_length;
	data = adc_maxdv->db_data; 

	if (len == ADE_LEN_UNKNOWN)
	{
	    /* Set the `max' length */
	    /* -------------------- */
	    switch (dt)
	    {
	      case DB_BOO_TYPE:
                adc_maxdv->db_length = 1;
		break;

	      case DB_FLT_TYPE:
		adc_maxdv->db_length = 8;
		break;

	      case DB_DEC_TYPE:
		adc_maxdv->db_length = DB_MAX_DECLEN;
		break;
		
	      case DB_INT_TYPE:
		adc_maxdv->db_length = 8;
		break;

	      case DB_CHR_TYPE:
	      case DB_CHA_TYPE:
	  	if ((adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
			adc_maxdv->db_collID != DB_UCS_BASIC_COLL)
		  adc_maxdv->db_length = adf_scb->adf_maxstring/2;
		else
		  adc_maxdv->db_length = adf_scb->adf_maxstring;
		break;
	      case DB_BIT_TYPE:
	      case DB_BYTE_TYPE:
              case DB_NBR_TYPE:
		adc_maxdv->db_length = adf_scb->adf_maxstring;
		break;

	      case DB_TXT_TYPE:
	      case DB_VCH_TYPE:
	  	if ((adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
			adc_maxdv->db_collID != DB_UCS_BASIC_COLL)
		  adc_maxdv->db_length = adf_scb->adf_maxstring/2 + DB_CNTSIZE;
		else
		  adc_maxdv->db_length = adf_scb->adf_maxstring + DB_CNTSIZE;
		break;

	      case DB_VBYTE_TYPE:
		adc_maxdv->db_length = adf_scb->adf_maxstring + DB_CNTSIZE;
		break;

	      case DB_VBIT_TYPE:
		adc_maxdv->db_length = adf_scb->adf_maxstring + DB_BCNTSIZE;
		break;

	      case DB_LTXT_TYPE:
		adc_maxdv->db_length = (i4)(MAXI2) + DB_CNTSIZE;
		break;

	      case DB_MNY_TYPE:
		adc_maxdv->db_length = ADF_MNY_LEN;
		break;

	      case DB_DTE_TYPE:
		adc_maxdv->db_length = ADF_DTE_LEN;
		break;

	      case DB_ADTE_TYPE:
		adc_maxdv->db_length = ADF_ADATE_LEN;
		break;

	      case DB_TMWO_TYPE:
	      case DB_TMW_TYPE:
	      case DB_TME_TYPE:
		adc_maxdv->db_length = ADF_TIME_LEN;
		break;

	      case DB_TSWO_TYPE:
	      case DB_TSW_TYPE:
	      case DB_TSTMP_TYPE:
		adc_maxdv->db_length = ADF_TSTMP_LEN;
		break;

	      case DB_INYM_TYPE:
		adc_maxdv->db_length = ADF_INTYM_LEN;
		break;

	      case DB_INDS_TYPE:
		adc_maxdv->db_length = ADF_INTDS_LEN;
		break;

	      case DB_LOGKEY_TYPE:
		adc_maxdv->db_length = DB_LEN_OBJ_LOGKEY;
		break;

	      case DB_TABKEY_TYPE:
		adc_maxdv->db_length = DB_LEN_TAB_LOGKEY;
		break;

	      case DB_NCHR_TYPE:
	      case DB_NVCHR_TYPE:
		adc_maxdv->db_length = DB_MAXSTRING;
		break;

	      default:
		return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	    }
	}
	else if (data != NULL)
	{
	    /* Build the `max' value */
	    /* --------------------- */

	    /* First, make sure supplied length is valid for this datatype */
	    /* ----------------------------------------------------------- */
	    if (db_stat = adc_lenchk(adf_scb, FALSE, adc_maxdv, NULL))
		return(db_stat);

	    /* Now, we can build the `max' value */
	    /* --------------------------------- */

	  /* Check if UTF8 enabled server */
	  if ((adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) && 
		(((dt == DB_CHA_TYPE) || 
		  (dt == DB_CHR_TYPE) || 
		  (dt == DB_TXT_TYPE) || 
		  (dt == DB_VCH_TYPE)))) 
          {
    		u_char	    utf8highkey3[] = {0xEF, 0xBF, 0xBF}; /* UFFFF */
    		u_char	    utf8highkey2[] = {0xDB, 0x95};	 /* U06D5 */
    		u_char	    utf8highkey1 = 0x7A;		 /* U007A */
		i4	    count = 0;
		i4	    hilen  = 0;
		u_char	    *datap = data;

		  
		if ((dt == DB_VCH_TYPE) || (dt == DB_TXT_TYPE)) 
		{
		  ((DB_TEXT_STRING *) data)->db_t_count = len - DB_CNTSIZE;
		  datap = (u_char *)&(((DB_TEXT_STRING *) data)->db_t_text);
		  if (len > DB_CNTSIZE)
		  {
		    count = ((len - DB_CNTSIZE)/3);
		    hilen = ((len - DB_CNTSIZE) % 3);
		  }
		  else
		  {
		    count = 0;
		    hilen = 0;
		  }
		}
		else { 
		  count = (len / 3);
		  hilen = len % 3;
		}

		/* Copy max UTF8 value */
                for ( ;count; count--)
                {
                    MEcopy(utf8highkey3, 3, datap);
		    datap += 3;
                }

                if (hilen == 2)
                {
                    MEcopy(utf8highkey2, 2, datap);
		    datap += 2;
                }
                else if (hilen == 1)
                    *datap++ = utf8highkey1;
          } 
	  else 
	  {
	    switch (dt)
	    {
	      case DB_BOO_TYPE:
                ((DB_ANYTYPE *)data)->db_booltype = DB_TRUE;
		break;

	      case DB_FLT_TYPE:
		if (len == 4)
		    *(f4 *)data = FMAX;
		else
		    *(f8 *)data = FMAX;
		break;

	      case DB_DEC_TYPE:
	        pk = (u_char *)data;

		/* fill first byte in depending on parity of prec */
	        if ((prec & 1) == 0)
		    *pk = 0x09;
		else
		    *pk = 0x99;

		/* fill intermediate bytes if there are any */
		if (len >= 3)
		    MEfill(len-2, (u_char)0x99, (PTR)(pk+1));

		/* place plus sign */
		pk[len-1] = 0x90 | MH_PK_PLUS;
		break;

	      case DB_INT_TYPE:
		switch (len)
		{
		  case 1:
		    *(i1 *)data = MAXI1;
		    break;
		  case 2:
		    *(i2 *)data = MAXI2;
		    break;
		  case 4:
		    *(i4 *)data = MAXI4;
		    break;
		  case 8:
		    *(i8 *)data = MAXI8;
		    break;
		}
		break;

	      case DB_CHR_TYPE:
		MEfill(len, (u_char) IIMAX_CHAR_VALUE, (PTR) data);
		break;

	      case DB_CHA_TYPE:
	      case DB_BYTE_TYPE:
              case DB_NBR_TYPE:
		MEfill(len, (u_char) 0xFF, (PTR) data);
		break;

	      case DB_BIT_TYPE:
		MEfill(len, (u_char) ~0, (PTR) data);
		break;

	      case DB_TXT_TYPE:
		((DB_TEXT_STRING *) data)->db_t_count = len - DB_CNTSIZE;
		if (len > DB_CNTSIZE)
		{
		    MEfill(len - DB_CNTSIZE, (u_char) AD_MAX_TEXT,
			   (PTR) ((DB_TEXT_STRING *) data)->db_t_text);
		}
		break;

	      case DB_VCH_TYPE:
	      case DB_LTXT_TYPE:
	      case DB_VBYTE_TYPE:
		((DB_TEXT_STRING *) data)->db_t_count = len - DB_CNTSIZE;
		if (len > DB_CNTSIZE)
		{
		    MEfill(len - DB_CNTSIZE, (u_char) 0xFF,
			   (PTR) ((DB_TEXT_STRING *) data)->db_t_text);
		}
		break;

	      case DB_VBIT_TYPE:
		((DB_BIT_STRING *) data)->db_b_count = len - DB_BCNTSIZE;
		if (len > DB_CNTSIZE)
		{
		    MEfill(len - DB_BCNTSIZE, (u_char) ~0,
			   (PTR) ((DB_BIT_STRING *) data)->db_b_bits);
		}
		break;

	      case DB_MNY_TYPE:
		((AD_MONEYNTRNL *) data)->mny_cents = AD_1MNY_MAX_NTRNL;
		break;

	      case DB_DTE_TYPE:
		{
		    AD_DATENTRNL    *d = (AD_DATENTRNL *) data;

		    d->dn_status =    AD_DN_ABSOLUTE
				    | AD_DN_YEARSPEC
				    | AD_DN_MONTHSPEC
				    | AD_DN_DAYSPEC
				    | AD_DN_TIMESPEC;
		    d->dn_highday = 0;
		    d->dn_year = AD_24DTE_MAX_ABS_YR;
		    d->dn_month = 12;
		    d->dn_lowday = 31;
		    d->dn_time = AD_9DTE_IMSPERDAY - 1;
		}
		break;

	      case DB_ADTE_TYPE:
		{
		    AD_ADATE *ad = (AD_ADATE *) data;
		    ad->dn_year = AD_24DTE_MAX_ABS_YR;
		    ad->dn_month = 12;
		    ad->dn_day = 31;
		}
		break;

	      case DB_TMWO_TYPE:
       	      case DB_TMW_TYPE:
       	      case DB_TME_TYPE:
        	{
	   	    AD_TIME *adtm = (AD_TIME *) data;

	     	    adtm->dn_seconds = AD_40DTE_SECPERDAY-1;
	     	    adtm->dn_nsecond = AD_33DTE_MAX_NSEC;
        	}
        	break;

              case DB_TSWO_TYPE:
              case DB_TSW_TYPE:
              case DB_TSTMP_TYPE:
               {
	            AD_TIMESTAMP *atstmp = (AD_TIMESTAMP *) data;

	            atstmp->dn_year = AD_24DTE_MAX_ABS_YR;
	            atstmp->dn_month = 12;
	            atstmp->dn_day = 31;
	            atstmp->dn_seconds = AD_40DTE_SECPERDAY-1;
	            atstmp->dn_nsecond = AD_33DTE_MAX_NSEC;
               }
               break;

              case DB_INYM_TYPE:
               {
	            AD_INTYM *aintym = (AD_INTYM *) data;

	            aintym->dn_years = AD_26DTE_MAX_INT_YR;
	            aintym->dn_months = 12;
               }
               break;

              case DB_INDS_TYPE:
               {
	            AD_INTDS *aintds = (AD_INTDS *) data;

	            aintds->dn_days = AD_28DTE_MAX_INT_DAY;
	            aintds->dn_seconds = AD_40DTE_SECPERDAY-1;
	            aintds->dn_nseconds = AD_33DTE_MAX_NSEC;
               }
               break;

	      case DB_LOGKEY_TYPE:
	      case DB_TABKEY_TYPE:
		MEfill(len, (u_char) 0xff, (PTR) data);
		break;

	      case DB_NCHR_TYPE:
		if (adc_maxdv->db_data)
		{
		  int i;
		  cpuni = (UCS2 *) adc_maxdv->db_data;
		  for (i = II_COUNT_ITEMS(adc_maxdv->db_length, 
			UCS2); i; i--)
    		  *cpuni++ = AD_MAX_UNICODE;
		}
		break;

	      case DB_NVCHR_TYPE:
		if (adc_maxdv->db_data)
		{
		  i2             i = 0;
		  UCS2            max = AD_MAX_UNICODE;
		  i2	          s = 0;

		  chr = (DB_NVCHR_STRING *) adc_maxdv->db_data;
		  s = II_COUNT_ITEMS(
			(adc_maxdv->db_length - DB_CNTSIZE), UCS2);

		  for (i = 0; i < s; i++)
		  {
		    I2ASSIGN_MACRO(max, chr->element_array[i]);
		  }
		  I2ASSIGN_MACRO(s, chr->count);
		}
		break;

	      default:
		return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	    }
	  }
	}
    }

    return(db_stat);
}
