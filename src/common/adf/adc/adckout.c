/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <cm.h>
#include    <cv.h>
#include    <me.h>
#include    <st.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduint.h>
#include    <adudate.h>
#include    <adumoney.h>

/**
**
**  Name: ADCKOUT.C - Contains routines for converting data values to a
**		      character format suitable for input as a constant.
**
**  Description:
**        This file contains routines used to convert data values
**	to be displayed by the terminal monitor.
**
**	    adc_klen() - Provide width for char string constant format of
**			 passed in data value.
**	    adc_kcvt() - Convert data value into char string constant format.
**          adc_1klen_rti() - ... for the RTI datatypes.
**          adc_2kcvt_rti() - ... for the RTI datatypes.
**
**	The following static routines are also defined in this file:
**
**          ad0_hexform() - Convert string to char string constant form as a
**			    hex constant.
**
**  History:
**      13-may-88 (thurston)
**	    Initial creation.
**	16-mar-89 (jrb)
**	    Changed floats from 7 to 6 digits of precision for f4's and from
**	    16 to 15 digits of precision for f8's.
**	20-mar-89 (jrb)
**	    Made changes for adc_lenchk interface change.
**	19-apr-89 (mikem)
**	    Logical key development.  Added support for DB_LOGKEY_TYPE or
**	    DB_TABKEY_TYPE to adc_1klen_rti() and adc_2kcvt_rti().
**	    The character string representation of  a logical key is 
**	    '\ddd' for each byte in the logical_key (so the 
**	    length is 4 * length).
**      28-apr-89 (fred)
**          Altered to use ADI_DT_MAP_MACRO().
**      05-jun-89 (fred)
**          Completed last fix.
**	05-jul-89 (jrb)
**	    Added decimal datatype support.
**      22-mar-91 (jrb)
**          Added #include of st.h.
**      03-oct-1992 (fred)
**	    Added bit strings (DB_{BIT,VBIT}_SUPPORT.
**      22-dec-1992 (stevet)
**          Added function prototype.
**	21-mar-1993 (robf)
**	    Include sl.h
**       6-Apr-1993 (fred)
**          Added byte string datatype support.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-aug-1993 (stevet)
**          Fixed type mismatch found by CL_PROTOTYPED.
**	25-aug-93 (ed)
**	    remove dbdbms.h
**      13-aug-2009 (joea)
**          Add cases for DB_BOO_TYPE in adc_1klen_rti and adc_2kcvt_rti.
**      09-mar-2010 (thich01)
**          Add DB_NBR_TYPE like DB_BYTE_TYPE for rtree indexing.
**/


static DB_STATUS ad0_hexform(ADF_CB   *adf_scb,
			     i4       adc_flags,
			     u_char   *startp,
			     i4       nbytes,
			     char     *adc_buf,
			     i4  *adc_buflen);

static char	    ad0_hexchars[16] = { '0','1','2','3','4','5','6','7',
					 '8','9','A','B','C','D','E','F' };


/*{
** Name: adc_klen() - Provide width for char string constant format of
**		      passed in data value.
**
** Description:
**      This routine is used to retrieve the width for the result of converting
**	a data value into its character string constant format.
**
** Inputs:
**      adf_scb                         Ptr to an ADF_CB struct.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	    .adf_qlang			The query language in use.
**	adc_flags			Reserved for certain flags, like format
**					for INGRES/QUEL, or such things.  At the
**					moment, only the following flags are
**					defined:
**						ADC_0001_FMT_SQL
**						ADC_0002_FMT_QUEL
**	adc_dv				Ptr to a DB_DATA_VALUE struct holding
**					data value to be converted.
**	    .db_datatype		Its datatype.
**	    .db_prec			Its precision and scale.
**	    .db_length			Its length.
**	    .db_data			Ptr to data to be converted.
**
** Outputs:
**	adf_scb
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
**	adc_width			The length, in bytes, of the field width
**					for the given data value.  That is, how
**					many bytes of memory will be needed for
**					the character string constant format of
**					the supplied data value.  This length
**					will include space for quotes, if
**					string type, and always include a byte
**					for a null-terminator.  Note, that if
**					the data value in question is the NULL
**					value, this will be set to 5:  The
**					length of "null" + the null terminator.
**
**	Returns:
**	    E_DB_OK			If it completed successfully.
**	    E_DB_ERROR			Some error occurred.
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
**	    checks which will be returned only when the system is
**	    compiled with the xDEBUG flag.
**
**          E_AD2005_BAD_DTLEN          Internal length is illegal for
**					the given datatype.  Note, it is
**					assumed that the length supplied
**					for the DB_DATA_VALUE arguments are
**					internal rather than user defined.
**	    E_AD200B_BAD_PREC		Invalid precision specified for DECIMAL;
**					must be: 1<= precision <=DB_MAX_DECPREC
**	    E_AD200C_BAD_SCALE		Invalid scale specified for DECIMAL;
**					must be: 0<= scale <=precision
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      16-may-88 (thurston)
**	    Initial creation.
**      12-jul-88 (thurston)
**          Reworded the interface description so that the caller actually
**	    supplied a complete DB_DATA_VALUE to this routine, not just a
**	    description.  (i.e. The .db_data field really is expected to point
**	    to the actual data.)
**      22-mar-91 (jrb)
**          Put comment markers around "xDEBUG" after #endif's.  This was
**          causing problems on some compilers.
*/

# ifdef ADF_BUILD_WITH_PROTOS
DB_STATUS
adc_klen(
ADF_CB              *adf_scb,
i4		    adc_flags,
DB_DATA_VALUE	    *adc_dv,
i4		    *adc_width)
# else
DB_STATUS
adc_klen( adf_scb, adc_flags, adc_dv, adc_width)
ADF_CB              *adf_scb;
i4		    adc_flags;
DB_DATA_VALUE	    *adc_dv;
i4		    *adc_width;
# endif
{
    DB_STATUS		db_stat = E_DB_OK;
    i4			bdt = abs((i4) adc_dv->db_datatype);
    i4			mbdt;

    mbdt = ADI_DT_MAP_MACRO(bdt);


    /* Check the consistency of the input DB_DATA_VALUE */

    /* Check the input DB_DATA_VALUEs datatype for correctness */
    if (mbdt <= 0  ||  mbdt > ADI_MXDTS
	    ||  Adf_globs->Adi_dtptrs[mbdt] == NULL)
	return (adu_error(adf_scb, E_AD2004_BAD_DTID, 0));

#ifdef xDEBUG
    /* Check the validity of the DB_DATA_VALUE lengths passed in. */
    if (db_stat = adc_lenchk(adf_scb, FALSE, adc_dv, NULL))
	return (db_stat);
#endif	/* xDEBUG */

    /*
    **	 Call the routine associated with the supplied datatype
    ** to perform this function.  Currently, this is only one
    ** routine for all the RTI known datatypes.  This level of
    ** indirection is provided for the future implementation of
    ** user-defined datatypes.
    */

    if (adc_dv->db_datatype > 0)	/* non-nullable */
    {
        db_stat = (*Adf_globs->Adi_dtptrs[mbdt]->
					adi_dt_com_vect.adp_klen_addr)
		    (adf_scb, adc_flags, adc_dv, adc_width);
    }
    else if (ADI_ISNULL_MACRO(adc_dv))	/* nullable with NULL value */
    {
	*adc_width = 5;
    }
    else				/* nullable, but not NULL */
    {
	DB_DATA_VALUE	    tmp_dv;

	STRUCT_ASSIGN_MACRO(*adc_dv, tmp_dv);
	tmp_dv.db_datatype = bdt;
	tmp_dv.db_length--;

        db_stat = (*Adf_globs->Adi_dtptrs[mbdt]->
					adi_dt_com_vect.adp_klen_addr)
		    (adf_scb, adc_flags, &tmp_dv, adc_width);
    }

    return (db_stat);
}


/*{
** Name: adc_kcvt() - Convert data value into char string constant format.
**
** Description:
**      This routine is used to convert any value into its character string
**	constant format.  For the character string datatypes, a simple quoted
**	string will be produced, with the quotes included (single quotes for
**	SQL, double for QUEL).  For QUEL, backslashes will be used to escape
**	non-printing characters; for SQL, hex constants will be used if there
**      are any non-printing characters.  For integers, a sequence of digits
**      will be generated, with a leading minus sign if negative.  Floating
**      point numbers will be produced in scientific notation.  Abstract types
**      will be surrounded by a conversion function, such as:  date("3 days"),
**      or money(1234.56).  All results will be null terminated. 
**
** Inputs:
**      adf_scb                         Ptr to an ADF_CB struct.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	    .adf_decimal
**		.db_decspec		Set to TRUE if a decimal marker is
**					specified in .db_decimal.  If FALSE,
**					ADF will assume '.' is the decimal char.
**		.db_decimal		If .db_decspec is TRUE, this is taken
**					to be the decimal character in use.
**					    ***                            ***
**					    *** NOTE THAT THIS IS A "nat", ***
**					    *** NOT A "char".              ***
**					    ***                            ***
**	    .adf_qlang			The query language in use.
**	adc_flags			Reserved for certain flags, like format
**					for INGRES/QUEL, or such things.  At the
**					moment, only the following flags are
**					defined:
**						ADC_0001_FMT_SQL
**						ADC_0002_FMT_QUEL
**	adc_dv				Ptr to a DB_DATA_VALUE struct holding
**					data value to be converted.
**	    .db_datatype		Its datatype.
**	    .db_prec			Its precision and scale.
**	    .db_length			Its length.
**	    .db_data			Ptr to data to be converted.
**	adc_buf				Ptr to a character buffer; assumed
**					to be large enough.
**
** Outputs:
**	adf_scb
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
**	adc_buf				The resulting char string constant
**					format will be placed here.
**	adc_buflen			Number of characters actually used,
**					including the null terminater.
**
**	Returns:
**	    E_DB_OK			If it completed successfully.
**	    E_DB_ERROR			If some error occurred in the
**					conversion process.
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
**	    checks which will be returned only when the system is
**	    compiled with the xDEBUG flag.
**
**          E_AD2005_BAD_DTLEN          Internal length is illegal for
**					the given datatype.  Note, it is
**					assumed that the length supplied
**					for the DB_DATA_VALUE arguments are
**					internal rather than user defined.
**	    E_AD200B_BAD_PREC		Invalid precision specified for DECIMAL;
**					must be: 1<= precision <=DB_MAX_DECPREC
**	    E_AD200C_BAD_SCALE		Invalid scale specified for DECIMAL;
**					must be: 0<= scale <=precision
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      16-may-88 (thurston)
**	    Initial creation.
**      13-jul-88 (thurston)
**	    Initial coding.
**      22-mar-91 (jrb)
**          Put comment markers around "xDEBUG" after #endif's.  This was
**          causing problems on some compilers.
*/

# ifdef ADF_BUILD_WITH_PROTOS
DB_STATUS
adc_kcvt(
ADF_CB              *adf_scb,
i4		    adc_flags,
DB_DATA_VALUE	    *adc_dv,
char		    *adc_buf,
i4		    *adc_buflen)
# else
DB_STATUS
adc_kcvt( adf_scb, adc_flags, adc_dv, adc_buf, adc_buflen)
ADF_CB              *adf_scb;
i4		    adc_flags;
DB_DATA_VALUE	    *adc_dv;
char		    *adc_buf;
i4		    *adc_buflen;
# endif
{
    DB_STATUS		db_stat = E_DB_OK;
    i4			bdt = abs((i4) adc_dv->db_datatype);
    i4			mbdt;

    mbdt = ADI_DT_MAP_MACRO(bdt);


    /* Check the consistency of the input DB_DATA_VALUE */

    /* Check the input DB_DATA_VALUEs datatype for correctness */
    if (mbdt <= 0  ||  mbdt > ADI_MXDTS
		||  Adf_globs->Adi_dtptrs[mbdt] == NULL)
	return (adu_error(adf_scb, E_AD2004_BAD_DTID, 0));

#ifdef xDEBUG
    /* Check the validity of the DB_DATA_VALUE lengths passed in. */
	if (db_stat = adc_lenchk(adf_scb, FALSE, adc_dv, NULL))
	    return (db_stat);
#endif	/* xDEBUG */

    /*
    **	 Call the routine associated with the supplied datatype
    ** to perform this function.  Currently, this is only one
    ** routine for all the RTI known datatypes.  This level of
    ** indirection is provided for the future implementation of
    ** user-defined datatypes.
    */

    if (adc_dv->db_datatype > 0)	/* non-nullable */
    {
        db_stat = (*Adf_globs->Adi_dtptrs[mbdt]->
					    adi_dt_com_vect.adp_kcvt_addr)
		    (adf_scb, adc_flags, adc_dv, adc_buf, adc_buflen);
    }
    else if (ADI_ISNULL_MACRO(adc_dv))	/* nullable with NULL value */
    {
	adc_buf[0] = 'n';
	adc_buf[1] = 'u';
	adc_buf[2] = 'l';
	adc_buf[3] = 'l';
	adc_buf[4] = EOS;
	*adc_buflen = 5;
	db_stat = E_DB_OK;
    }
    else				/* nullable, but not NULL */
    {
	DB_DATA_VALUE	    tmp_dv;

	STRUCT_ASSIGN_MACRO(*adc_dv, tmp_dv);
	tmp_dv.db_datatype = bdt;
	tmp_dv.db_length--;

        db_stat = (*Adf_globs->Adi_dtptrs[mbdt]->adi_dt_com_vect.adp_kcvt_addr)
		    (adf_scb, adc_flags, &tmp_dv, adc_buf, adc_buflen);
    }

    return (db_stat);
}


/*
** Name: adc_1klen_rti() - Provide width for char string constant format of
**			   passed in data value for the RTI known datatypes.
**
** Description:
**	This routine is used to retrieve the width for the result of converting
**	a data value into its character string constant format for the RTI
**	known datatypes.
**
** Inputs:
**      adf_scb                         Ptr to an ADF_CB struct.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	    .adf_qlang			The query language in use.
**	adc_flags			Reserved for certain flags, like format
**					for INGRES/QUEL, or such things.  At the
**					moment, only the following flags are
**					defined:
**						ADC_0001_FMT_SQL
**						ADC_0002_FMT_QUEL
**	adc_dv				Ptr to a DB_DATA_VALUE struct holding
**					data value to be converted.
**	    .db_datatype		Its datatype.
**	    .db_prec			Its precision and scale.
**	    .db_length			Its length.
**	    .db_data			Ptr to data to be converted.
**
** Outputs:
**	adf_scb
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
**	adc_width			The length, in bytes, of the field width
**					for the given data value.  That is, how
**					many bytes of memory will be needed for
**					the character string constant format of
**					the supplied data value.  This length
**					will include space for quotes, if
**					string type, and always include a byte
**					for a null-terminator.  Note, that if
**					the data value in question is the NULL
**					value, this will be set to 5:  The
**					length of "null" + the null terminator.
**
**	Returns:
**	    E_DB_OK			If it completed successfully.
**	    E_DB_ERROR			Some error occurred.
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
**	    checks which will be returned only when the system is
**	    compiled with the xDEBUG flag.
**
**          E_AD2005_BAD_DTLEN          Internal length is illegal for
**					the given datatype.  Note, it is
**					assumed that the length supplied
**					for the DB_DATA_VALUE arguments are
**					internal rather than user defined.
**	    E_AD200B_BAD_PREC		Invalid precision specified for DECIMAL;
**					must be: 1<= precision <=DB_MAX_DECPREC
**	    E_AD200C_BAD_SCALE		Invalid scale specified for DECIMAL;
**					must be: 0<= scale <=precision
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-jul-88 (thurston)
**	    Initial creation.
**	19-apr-89 (mikem)
**	    Logical key development.  Added support for DB_LOGKEY_TYPE or
**	    DB_TABKEY_TYPE to adc_1klen_rti() and adc_2kcvt_rti().
**	    The character string representation of a logical key is 
**	    '\ddd' for each byte in the logical_key (so the 
**	    length is 4 * length).
**	05-jul-89 (jrb)
**	    Added decimal support.
**       6-Apr-1993 (fred)
**          Added byte string datatype support.
**	12-may-04 (inkdo01)
**	    Added support for bigint.
**	16-jan-2007 (dougi)
**	    Change to support longer decimal.
**	05-Sep-2008 (gupsh01)
**	    Fix usage of CMdbl1st for UTF8 and multibyte character sets
**	    can can be upto four bytes long. Use CMbytecnt to ensure
**	    we do not assume that a multbyte string can only be a maximum
**	    of two bytes long. (Bug 120865)
*/

DB_STATUS
adc_1klen_rti(
ADF_CB              *adf_scb,
i4		    adc_flags,
DB_DATA_VALUE	    *adc_dv,
i4		    *adc_width)
{
    DB_STATUS		db_stat = E_DB_OK;
    register i4	length = adc_dv->db_length;
    register u_char	*f = (u_char *)adc_dv->db_data;
    register u_char	*p;
    i2			res_width;
    i2			i2tmp;
    i4			i4tmp;
    i8			i8tmp;
    f4			f4tmp;
    f8			f8tmp;
    char		tmpbuf[64];
    i4			qlang = adf_scb->adf_qlang;


    if      (adc_flags & ADC_0001_FMT_SQL)
	qlang = DB_SQL;
    else if (adc_flags & ADC_0002_FMT_QUEL)
	qlang = DB_QUEL;


    switch (adc_dv->db_datatype)
    {
      case DB_BOO_TYPE:
        *adc_width = 2;
        break;

      case DB_INT_TYPE:
	switch (length)
	{
	  case 1:
	    i4tmp = I1_CHECK_MACRO(*(i1 *)f);
	    break;

	  case 2:
	    I2ASSIGN_MACRO(*(i2 *)f, i2tmp);
	    i4tmp = i2tmp;
	    break;

	  case 4:
	    I4ASSIGN_MACRO(*(i4 *)f, i4tmp);
	    break;

	  case 8:
	    I8ASSIGN_MACRO(*(i8 *)f, i8tmp);
	    break;

	  default:
	    db_stat = adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0);
	    break;
	}

	if (db_stat == E_DB_OK)
	{
	    if (i4tmp == MINI4)
	    {
		*adc_width = 12;
		break;
	    }

	    *adc_width = 0;
	    if (i4tmp < 0)
	    {
		(*adc_width)++;
	    }
	    i4tmp = abs(i4tmp);

	    if      (i4tmp >= 1000000000)
		(*adc_width) += 11;
	    else if (i4tmp >= 100000000)
		(*adc_width) += 10;
	    else if (i4tmp >= 10000000)
		(*adc_width) += 9;
	    else if (i4tmp >= 1000000)
		(*adc_width) += 8;
	    else if (i4tmp >= 100000)
		(*adc_width) += 7;
	    else if (i4tmp >= 10000)
		(*adc_width) += 6;
	    else if (i4tmp >= 1000)
		(*adc_width) += 5;
	    else if (i4tmp >= 100)
		(*adc_width) += 4;
	    else if (i4tmp >= 10)
		(*adc_width) += 3;
	    else
		(*adc_width) += 2;
	}
	break;

      case  DB_DEC_TYPE:
	*adc_width = DB_MAX_DECPREC+1+1;
	break;
	/* JRBCMT: fix this after CVpka is written */
      case  DB_FLT_TYPE:
	{
	    i4	    wid;
	    i4	    prec;


	    switch (length)
	    {
	      case 4:
		F4ASSIGN_MACRO(*(f4 *)f, f4tmp); 
		f8tmp = f4tmp;
#ifdef    IEEE
		wid  = 14;
		prec = 6;
#else
		wid  = 13;
		prec = 6;
#endif /* IEEE */
		break;

	      case 8:
		F8ASSIGN_MACRO(*(f8 *)f, f8tmp); 
#ifdef    IEEE
		wid  = 23;
		prec = 15;
#else
		wid  = 22;
		prec = 15;
#endif /* IEEE */
		break;

	      default:
		db_stat = adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0);
		break;
	    }
	    
	    if (db_stat == E_DB_OK)
	    {
		CVfa(f8tmp, wid, prec, 'e', '.', tmpbuf, &res_width);
		*adc_width = res_width + 1;	/* +1 for <eos> */
	    }
	}
	break;

      case  DB_MNY_TYPE:
	F8ASSIGN_MACRO(((AD_MONEYNTRNL*)(adc_dv->db_data))->mny_cents, 
		       f8tmp);
	f8tmp /= 100;	/* because money is stored as cents */
	CVfa(f8tmp, (i4) 18, (i4) 2, 'f', '.', tmpbuf, &res_width);
	/*
	**         money  (   <number>    )  <eos>
	** width =   5  + 1 + res_width + 1 +  1
	**	 =   8 + res_width
	*/
	*adc_width = 8 + res_width;
	break;

      case DB_CHR_TYPE:
      case DB_CHA_TYPE:
      case DB_TXT_TYPE:
      case DB_VCH_TYPE:
      case DB_LTXT_TYPE:
      case DB_BYTE_TYPE:
      case DB_VBYTE_TYPE:
      case DB_NBR_TYPE:
	{
	    u_char		*endp;
	    register i4	inbytes;
	    register i4	outbytes;


	    if (    adc_dv->db_datatype == DB_CHA_TYPE
		||  adc_dv->db_datatype == DB_CHR_TYPE
		||  adc_dv->db_datatype == DB_BYTE_TYPE
                ||  adc_dv->db_datatype == DB_NBR_TYPE
	       )
	    {
		p = f;
		inbytes = length;
	    }
	    else
	    {
		p = f + DB_CNTSIZE;
		I2ASSIGN_MACRO(*(i2 *)f, i2tmp);
		inbytes = i2tmp;
	    }

	    endp   = p + inbytes;

	    outbytes = 0;
	    while (p < endp)
	    {
		switch (*p)
		{
		  case (u_char)'\n':
		  case (u_char)'\t':
		  case (u_char)'\b':
		  case (u_char)'\r':
		  case (u_char)'\f':
		    if (qlang == DB_SQL)
		    {
			/* must do in hex constant form */
			*adc_width = 4 + (2 * inbytes);
			return (E_DB_OK);
		    }
		    outbytes += 2;
		    break;

		  case (u_char)'\\':
		    if (qlang == DB_SQL)
			outbytes++;
		    else
			outbytes += 2;
		    break;

		  default:
		    if (CMcntrl((char *)p))
		    {
			if (qlang == DB_SQL)
			{
			    /* must do in hex constant form */
			    *adc_width = 4 + (2 * inbytes);
			    return (E_DB_OK);
			}

			/* convert the character to \ddd, where
			** the d's are octal digits
			*/
			outbytes += 4;
		    }
		    else
		    {
			outbytes += CMbytecnt(p);
		    }
		    break;
		}
		CMnext(p);
	    }
	    *adc_width = 3 + outbytes;
	    db_stat = E_DB_OK;
	}
	break;

      case DB_DTE_TYPE:
	*adc_width = 9 + AD_1DTE_OUTLENGTH;
	break;

      case DB_ADTE_TYPE:
	*adc_width = 9 + AD_2ADTE_OUTLENGTH;
      break;

      case DB_TMWO_TYPE:
	*adc_width = 9 + AD_3TMWO_OUTLENGTH;
      break;

      case DB_TMW_TYPE:
	*adc_width = 9 + AD_4TMW_OUTLENGTH;
      break;

      case DB_TME_TYPE:
	*adc_width = 9 + AD_5TME_OUTLENGTH;
      break;

      case DB_TSWO_TYPE:
	*adc_width = 9 + AD_6TSWO_OUTLENGTH;
      break;

      case DB_TSW_TYPE:
	*adc_width = 9 + AD_7TSW_OUTLENGTH;
      break;

      case DB_TSTMP_TYPE:
	*adc_width = 9 + AD_8TSTMP_OUTLENGTH;
      break;

      case DB_INYM_TYPE:
	*adc_width = 9 + AD_9INYM_OUTLENGTH;
      break;

      case DB_INDS_TYPE:
	*adc_width = 9 + AD_10INDS_OUTLENGTH;
      break;

      case DB_LOGKEY_TYPE:
      case DB_TABKEY_TYPE:
      {
          /*
          ** Logical key character format will be standard SQL constant
          ** form for character representation non-printable characters,
          ** so all bytes will be printed in the form '\XXX'.
          */

          *adc_width = 4 * length;
	  break;
      }

      case DB_BIT_TYPE:
	length = adc_dv->db_length * BITSPERBYTE;
	if (adc_dv->db_prec)
	    length = length - BITSPERBYTE + adc_dv->db_prec;

	*adc_width = length + 3; /* 3 -> two quotes and a null */
	break;

      case DB_VBIT_TYPE:
	I4ASSIGN_MACRO(*adc_dv->db_data, i4tmp);
	length = i4tmp;

	*adc_width = length + 3;
	break;

      default:
	db_stat = adu_error(adf_scb, E_AD2004_BAD_DTID, 0);
	break;
    }
    return (db_stat);
}


/*
** Name: adc_2kcvt_rti() - Convert data value into char string constant format
**			   for the RTI known datatypes.
**
** Description:
**      This routine is used to convert a data value into its character string
**	constant format for the RTI known datatypes.
**
** Inputs:
**      adf_scb                         Ptr to an ADF_CB struct.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	    .adf_decimal
**		.db_decspec		Set to TRUE if a decimal marker is
**					specified in .db_decimal.  If FALSE,
**					ADF will assume '.' is the decimal char.
**		.db_decimal		If .db_decspec is TRUE, this is taken
**					to be the decimal character in use.
**					    ***                            ***
**					    *** NOTE THAT THIS IS A "nat", ***
**					    *** NOT A "char".              ***
**					    ***                            ***
**	    .adf_qlang			The query language in use.
**	adc_flags			Reserved for certain flags, like format
**					for INGRES/QUEL, or such things.  At the
**					moment, only the following flags are
**					defined:
**						ADC_0001_FMT_SQL
**						ADC_0002_FMT_QUEL
**	adc_dv				Ptr to a DB_DATA_VALUE struct holding
**					data value to be converted.
**	    .db_datatype		Its datatype.
**	    .db_prec			Its precision and scale.
**	    .db_length			Its length.
**	    .db_data			Ptr to data to be converted.
**	adc_buf				Ptr to a character buffer; assumed
**					to be large enough.
**
** Outputs:
**	adf_scb
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
**	adc_buf				The resulting char string constant
**					format will be placed here.
**	adc_buflen			Number of characters actually used,
**					including the null terminater.
**
**	Returns:
**	    E_DB_OK			If it completed successfully.
**	    E_DB_ERROR			If some error occurred in the
**					conversion process.
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
**	    checks which will be returned only when the system is
**	    compiled with the xDEBUG flag.
**
**          E_AD2005_BAD_DTLEN          Internal length is illegal for
**					the given datatype.  Note, it is
**					assumed that the length supplied
**					for the DB_DATA_VALUE arguments are
**					internal rather than user defined.
**	    E_AD200B_BAD_PREC		Invalid precision specified for DECIMAL;
**					must be: 1<= precision <=DB_MAX_DECPREC
**	    E_AD200C_BAD_SCALE		Invalid scale specified for DECIMAL;
**					must be: 0<= scale <=precision
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      13-jul-88 (thurston)
**	    Initial creation.
**	19-apr-89 (mikem)
**	    Logical key development.  Added support for DB_LOGKEY_TYPE or
**	    DB_TABKEY_TYPE to adc_1klen_rti() and adc_2kcvt_rti().
**	    The character string representation of  a logical key is 
**	    '\ddd' for each byte in the logical_key (so the 
**	    length is 4 * length).
**	05-jul-89 (jrb)
**	    Added decimal support.
**      03-oct-1992 (fred)
**	    Added bit string types.
**       6-Apr-1993 (fred)
**          Added byte string datatypes.
**	12-may-04 (inkdo01)
**	    Added support for bigint.
[@history_template@]...
*/

DB_STATUS
adc_2kcvt_rti(
ADF_CB              *adf_scb,
i4		    adc_flags,
DB_DATA_VALUE	    *adc_dv,
char		    *adc_buf,
i4		    *adc_buflen)
{
    DB_STATUS		db_stat = E_DB_OK;
    register i4	length = adc_dv->db_length;
    register u_char	*f = (u_char *)adc_dv->db_data;
    register char	*t = (char *)adc_buf;
    register u_char	*p;
    i2			res_width;
    i2			i2tmp;
    i4			i4tmp;
    i8			i8tmp;
    f4			f4tmp;
    f8			f8tmp;
    char		decimal = (adf_scb->adf_decimal.db_decspec
					? (char) adf_scb->adf_decimal.db_decimal
					: '.'
				  );
    i4			qlang = adf_scb->adf_qlang;


    if      (adc_flags & ADC_0001_FMT_SQL)
	qlang = DB_SQL;
    else if (adc_flags & ADC_0002_FMT_QUEL)
      	qlang = DB_QUEL;


    switch (adc_dv->db_datatype)
    {
      case DB_BOO_TYPE:
        *t = (((DB_ANYTYPE *)adc_dv->db_data)->db_booltype == DB_FALSE ? 'f'
             : 't');
        t[1] = EOS;
        *adc_buflen = 2;
        break;

      case DB_INT_TYPE:
	switch (length)
	{
	  case 1:
	    i4tmp = I1_CHECK_MACRO(*(i1 *)f);
	    break;

	  case 2:
	    I2ASSIGN_MACRO(*(i2 *)f, i2tmp);
	    i4tmp = i2tmp;
	    break;

	  case 4:
	    I4ASSIGN_MACRO(*(i4 *)f, i4tmp);
	    break;

	  case 8:
	    I8ASSIGN_MACRO(*(i8 *)f, i8tmp);
	    break;

	  default:
	    db_stat = adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0);
	    break;
	}

	if (db_stat == E_DB_OK)
	{
	    CVla(i4tmp, t);
	    *adc_buflen = STlength((char *) t) + 1;
	}
	break;

      case  DB_DEC_TYPE:
	/* JRBCMT: fix this after CVpka is re-written */
	break;
	
      case  DB_FLT_TYPE:
	{
	    i4	    wid;
	    i4	    prec;


	    switch (length)
	    {
	      case 4:
		F4ASSIGN_MACRO(*(f4 *)f, f4tmp); 
		f8tmp = f4tmp;
#ifdef    IEEE
		wid  = 14;
		prec = 6;
#else
		wid  = 13;
		prec = 6;
#endif /* IEEE */
		break;

	      case 8:
		F8ASSIGN_MACRO(*(f8 *)f, f8tmp); 
#ifdef    IEEE
		wid  = 23;
		prec = 15;
#else
		wid  = 22;
		prec = 15;
#endif /* IEEE */
		break;

	      default:
		db_stat = adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0);
		break;
	    }
	    
	    if (db_stat == E_DB_OK)
	    {
		CVfa(f8tmp, wid, prec, 'e', decimal, t, &res_width);
		*adc_buflen = res_width + 1;
	    }
	}
	break;

      case  DB_MNY_TYPE:
	STcopy("money(", (char *) t);
	t += 6;

	F8ASSIGN_MACRO(((AD_MONEYNTRNL*)(adc_dv->db_data))->mny_cents,
		       f8tmp);
	f8tmp /= 100;	/* because money is stored as cents */
	CVfa(f8tmp, (i4) 18, (i4) 2, 'f', decimal, t, &res_width);
	t += res_width;
	*t++ = ')';
	*t++ = EOS;
	*adc_buflen = t - (char *)adc_buf;
	break;

      case DB_CHR_TYPE:
      case DB_CHA_TYPE:
      case DB_TXT_TYPE:
      case DB_VCH_TYPE:
      case DB_LTXT_TYPE:
      case DB_BYTE_TYPE:
      case DB_VBYTE_TYPE:
      case DB_NBR_TYPE:
	{
	    u_char		*startp;
	    u_char		*endp;
	    register i4	inbytes;

	    if (    adc_dv->db_datatype == DB_CHA_TYPE
		||  adc_dv->db_datatype == DB_CHR_TYPE
		||  adc_dv->db_datatype == DB_BYTE_TYPE
                ||  adc_dv->db_datatype == DB_NBR_TYPE
	       )
	    {
		p = f;
		inbytes = length;
	    }
	    else
	    {
		p = f + DB_CNTSIZE;
		I2ASSIGN_MACRO(*(i2 *)f, i2tmp);
		inbytes = i2tmp;
	    }

	    startp = p;
	    endp   = p + inbytes;

	    *t++ = (qlang == DB_SQL ? '\'' : '\"');
	    while (p < endp)
	    {
		switch (*p)
		{
		  case (u_char)'\n':
		    if (qlang == DB_SQL)
			return (ad0_hexform(adf_scb, adc_flags, startp, inbytes,
					    adc_buf, adc_buflen));
		    STcopy("\\n", (char *)t);
		    t += 2;
		    break;

		  case (u_char)'\t':
		    if (qlang == DB_SQL)
			return (ad0_hexform(adf_scb, adc_flags, startp, inbytes,
					    adc_buf, adc_buflen));
		    STcopy("\\t", (char *)t);
		    t += 2;
		    break;

		  case (u_char)'\b':
		    if (qlang == DB_SQL)
			return (ad0_hexform(adf_scb, adc_flags, startp, inbytes,
					    adc_buf, adc_buflen));
		    STcopy("\\b", (char *)t);
		    t += 2;
		    break;

		  case (u_char)'\r':
		    if (qlang == DB_SQL)
			return (ad0_hexform(adf_scb, adc_flags, startp, inbytes,
					    adc_buf, adc_buflen));
		    STcopy("\\r", (char *)t);
		    t += 2;
		    break;

		  case (u_char)'\f':
		    if (qlang == DB_SQL)
			return (ad0_hexform(adf_scb, adc_flags, startp, inbytes,
					    adc_buf, adc_buflen));
		    STcopy("\\f", (char *)t);
		    t += 2;
		    break;

		  case (u_char)'\\':
		    if (qlang == DB_SQL)
		    {
			*t++ = '\\';
		    }
		    else
		    {
			STcopy("\\\\", (char *)t);
			t += 2;
		    }
		    break;

		  default:
		    if (CMcntrl((char *)p))
		    {
			u_i4	curval;
			i4	curpos;

			if (qlang == DB_SQL)
			    return (ad0_hexform(adf_scb, adc_flags, startp,
						inbytes, adc_buf, adc_buflen));

			/* convert the character to \ddd, where
			** the d's are octal digits
			*/
			*t = '\\';
			curval = *(u_char *)p;
			/* while there are more octal digits */
			for (curpos = 3; curpos > 0; curpos--)
			{
			    /* put the digit in the output string */
			    t[curpos] = (u_char)'0' + (curval % 8);
			    /* remove the digit from the current value */
			    curval /= 8;
			}
			t += 4;
		    }
		    else
		    {
			CMcpychar(p, t);
			CMnext(t);
		    }
		    break;
		}

		CMnext(p);
	    }
	    *t++ = (qlang == DB_SQL ? '\'' : '\"');
	    *t++ = EOS;
	    *adc_buflen = t - (char *)adc_buf;
	    db_stat = E_DB_OK;
	}
	break;

      case DB_DTE_TYPE:
	{
	    DB_DATA_VALUE   str_dv;

	    STcopy("date(", (char *) t);
	    t += 5;
	    *t++ = (qlang == DB_SQL ? '\'' : '\"');
	    str_dv.db_data     = (PTR) t;
	    str_dv.db_datatype = DB_CHA_TYPE;
	    str_dv.db_length   = AD_1DTE_OUTLENGTH;
	    db_stat = adu_6datetostr(adf_scb, adc_dv, &str_dv);
	    if (DB_SUCCESS_MACRO(db_stat))
	    {
		t += AD_1DTE_OUTLENGTH;
		*t++ = (qlang == DB_SQL ? '\'' : '\"');
		*t++ = ')';
		*t++ = EOS;
		*adc_buflen = t - (char *)adc_buf;
	    }
	}
	break;

      case DB_LOGKEY_TYPE:
      case DB_TABKEY_TYPE:
      {
          return (ad0_hexform(adf_scb, adc_flags, (u_char *)adc_dv->db_data,
                              length, adc_buf, adc_buflen));
          break;
      }

      case DB_BIT_TYPE:
      {
	DB_DATA_VALUE	str_dv;

	str_dv.db_datatype = DB_CHA_TYPE;
	str_dv.db_prec = 0;
	*t++ = (qlang == DB_SQL ? '\'' :'\"');
	str_dv.db_data = (PTR) t;

	length = adc_dv->db_length * BITSPERBYTE;
	if (adc_dv->db_prec)
	    length = length - BITSPERBYTE + adc_dv->db_prec;
	str_dv.db_length = length;
	db_stat = adu_bit2str(adf_scb, adc_dv, &str_dv);
	if (DB_SUCCESS_MACRO(db_stat))
	{
	    t += length;
	    *t++ = (qlang == DB_SQL ? '\'' : '\"');
	    *t++ = EOS;
	    *adc_buflen = t - (char *) adc_buf;
	}
	break;
      }
      
      case DB_VBIT_TYPE:
      {
	DB_DATA_VALUE	str_dv;

	str_dv.db_datatype = DB_CHA_TYPE;
	str_dv.db_prec = 0;
	*t++ = (qlang == DB_SQL ? '\'' :'\"');
	str_dv.db_data = (PTR) t;

	I4ASSIGN_MACRO(*adc_dv->db_data, i4tmp);
	length = i4tmp;
	str_dv.db_length = length;
	db_stat = adu_bit2str(adf_scb, adc_dv, &str_dv);
	if (DB_SUCCESS_MACRO(db_stat))
	{
	    t += length;
	    *t++ = (qlang == DB_SQL ? '\'' : '\"');
	    *t++ = EOS;
	    *adc_buflen = t - (char *) adc_buf;
	}
	break;
      }
      
      default:
	db_stat = adu_error(adf_scb, E_AD2004_BAD_DTID, 0);
	break;
    }
    return (db_stat);
}


/*
** Name: ad0_hexform() - Convert string to char string constant form as a hex
**			 constant.
**
** Description:
**      This routine is used to convert a data value into its character string
**	constant format for the RTI known datatypes.
**
** Inputs:
**      adf_scb                         Ptr to an ADF_CB struct.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	adc_flags			Reserved for certain flags, like format
**					for INGRES/QUEL, or such things.
**	startp				Pointer to beginning of string.
**	nbytes				Number of bytes in string.
**	adc_buf				Ptr to a character buffer; assumed
**					to be large enough.
**
** Outputs:
**	adf_scb
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
**	adc_buf				The resulting char string constant
**					format will be placed here.
**	adc_buflen			Number of characters actually used,
**					including the null terminater.
**
**	Returns:
**	    E_DB_OK			If it completed successfully.
**	    E_DB_ERROR			If some error occurred in the
**					conversion process.
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**          E_AD0000_OK                 Operation succeeded.
**
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-jul-88 (thurston)
**	    Initial creation.
*/

static DB_STATUS
ad0_hexform(
ADF_CB              *adf_scb,
i4		    adc_flags,
u_char		    *startp,
i4		    nbytes,
char		    *adc_buf,
i4		    *adc_buflen)
{
    register u_char	*f = (u_char *)startp;
    register char	*t = adc_buf;
    register i4	in_len = nbytes;
    register i4	chr;
    register i4	c1;
    register i4	c2;


    *t++ = 'X';
    *t++ = '\'';

    while (in_len--)
    {
	chr = *f++;	    /* Do NOT use I1_CHECK_MACRO !!! */
	c1 = chr / 16;
	c2 = chr - (c1*16);
	*t++ = ad0_hexchars[c1];
	*t++ = ad0_hexchars[c2];
    }

    *t++ = '\'';
    *t++ = EOS;
    *adc_buflen = t - adc_buf;
    return (E_DB_OK);
}
