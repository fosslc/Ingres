/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <adp.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <adudate.h>
#include    <adumoney.h>

/**
**
**  Name: ADCLENCHK.C - Check validity of user specified or internal
**                      length for a datatype and return the
**                      equivalent other.
**
**  Description:
**      This file contains all of the routines necessary to
**      perform the  external ADF call "adc_lenchk()" which
**      checks the validity of the proposed length, either user
**      specified or internal, for the datatype of the given
**      data value.  For DECIMAL we also check precision.
**
**      This file defines:
**
**          adc_lenchk() - Check validity of user specified or internal
**                         length for a datatype and return the
**                         equivalent other.
**          adc_seglen() - Determine underlying datatype segment length for
**                          large objects.
**	    adc_1lenchk_rti() - Check validity and resolve length of INGRES
**				datatypes.
**	    adc_2lenchk_bool() - Check validity of ADF internal datatype
**				 "boolean".
**
**  History:
**      25-feb-86 (thurston)
**          Initial creation.
**      12-mar-86 (thurston)
**          Added the adc_rlen argument so that the .db_length field
**	    of the supplied DB_DATA_VALUE is not changed on returned,
**	    and also have it return either the user specified length
**	    or the internal length, whichever is NOT supplied.  This
**	    removes the need for the adi_ln_usr() and adi_ln_intrn()
**	    routines.
**	30-jun-86 (thurston)
**	    Altered a comment so that the adc__lenchk() routine was indeed
**	    called that, and not _adc_lenchk().  I also corrected a small bug
**	    in the adc_lenchk() routine:  If a data value's datatype was not in
**	    the legal range of datatype IDs (0 < dt_ID <= ADI_MXDTS) this
**	    routine would be looking at memory it had no business looking at,
**	    and would probably access violate.
**	25-jul-86 (thurston)
**	    Changed name of adc__lenchk() to adc_1lenchk_rti().
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	28-oct-86 (thurston)
**	    Added char and varchar support.
**	20-nov-86 (thurston)
**	    Added longtext support.
**	05-dec-86 (thurston)
**	    Fixed bug in adc_1lenchk_rti() that said an internal length of
**	    DB_CNTSIZE for text was not valid.  It is valid, and represents
**	    the empty string.
**	10-feb-87 (thurston)
**	    Changed ADF_CHAR_MAX to DB_CHAR_MAX in adc_1lenchk_rti().
**	19-feb-87 (thurston)
**	    Added support for nullable datatypes.
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	05-jun-87 (thurston)
**	    Made all low level lenchk functions return a `default' width
**	    even if the length check fails.
**	01-sep-88 (thurston)
**	    Changed the use of DB_CHAR_MAX and DB_MAXSTRING to
**	    adf_scb->adf_maxstring, since this is now a session settable
**	    parameter.
**	19-mar-89 (jrb)
**	    Interface change to return a pointer to a DB_DATA_VALUE instead
**	    of a pointer to an i4.  This allows us to return precision and
**	    scale for the decimal datatype.
**	29-mar-89 (jrb)
**	    Input precision and scale values are now checked, but this code is
**	    ifdef'd out for now.  It will be activated when all DBMS and FE
**	    code has been cleaned up to properly set the db_prec field.  Also
**	    now fills in result data only if output variable (adc_rdv) is not
**	    NULL.
**	19-apr-89 (mikem)
**	    Logical key development.  Added support for logical_key datatype
**	    to adc_1lenchk_rti().
**	28-apr-89 (fred)
**	    Altered to use ADI_DT_MAP_MACRO().
**	01-jun-89 (mikem)
**	    Bug fix for logical keys.  Was returning random datatype 
**	    id in result data value returned from adc_lenchk().
**	05-jun-89 (fred)
**	    Completed above fix for use of ADI_DT_MAP_MACRO().
**	06-jul-89 (jrb)
**	    Activated decimal checking code; it is now an error to call this
**	    routine with a decimal type and an invalid precision/scale.
**	19-dec-89 (fred)
**	    Added support for long varchar datatype.
**	22-sep-1992 (fred)
**	    Added support for the bit datatypes.
**      25-sep-92 (stevet)
**          Added check for float which translate float(1-7) to float4
**          and float(8-53) to float8
**      22-dec-1992 (stevet)
**          Added function prototype.
**       6-Apr-1993 (fred)
**          Added byte string types.
**	21-mar-1993 (robf)
**	    Include sl.h
**       1-Jul-1993 (fred)
**          Changed ADP_LENCHK_FUNC prototype.  The second argument (adc_is_usr
**          was a bool.  The problem with this is that bool's are
**          system dependent (defined in compat.h) but the prototype
**          needs to be known/shared with user code for OME.
**          Therefore, we make it a i4  (== int for users).
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**      12-Oct-1993 (fred)
**          Added adc_seglen() routines.
**      09-sep-1993 (stevet)
**          Changed adc_seglen() to pass non-null datatype ID to underlying
**          routine.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      03-apr-2001 (gupsh01)
**          Added support for unicode long nvarchar datatype.
**      16-oct-2006 (stial01)
**          Added length check for locator datatypes
**	16-Jun-2009 (thich01)
**	    Treat GEOM type the same as LBYTE.
**	20-Aug-2009 (thich01)
**	    Treat all spatial types the same as LBYTE.
**      29-sep-2009 (joea)
**          Add case for DB_BOO_TYPE in adc_1lenchk_rti.  Change
**          adc_2lenchk_bool to use i1 as the underlying type.
**      09-mar-2010 (thich01)
**          Add DB_NBR_TYPE like DB_BYTE_TYPE for rtree indexing.
**	19-Nov-2010 (kiria01) SIR 124690
**	    Add support for UCS_BASIC collation. No need for the reduced length
**	    as it doesn't use UCS2 for CEs.
**/


/*{
** Name: adc_lenchk() - Check validity of user specified or internal length
**                      for a datatype and return the equivalent other.
**
** Description:
**	This function is the external ADF call "adc_lenchk()" which checks the
**	validity of the proposed length, precision, and scale, either user
**	specified or internal, for the datatype of the given data value.
**
**	The length, precision, and scale  can be specified as either 'internal'
**	or as 'user specified'.  For example, take text(50); the user specified
**	length is 50, but the internal length is 52 (50 bytes for the
**	characters, plus 2 bytes for the count field).  If the length,
**	precision, and scale are supplied as 'user specified', the appropriate
**	internal values will be returned.  Likewise, if supplied as 'internal',
**	the appropriate user specified values will be returned.
**
**      When the datatype in question is a fixed length datatype, and
**      the caller wishes to supply the user specified length, he/she
**      MUST supply it as 0 (zero).  If not, the E_AD2007_DT_IS_FIXLEN
**      error will be generated.  Likewise, if the datatype is a
**      variable length one, and a user specified length of zero is
**      supplied the E_AD2008_DT_IS_VARLEN error will be generated.
**	Also, note that if the datatype is fixed length, and the
**	internal length is supplied, zero will be returned as the
**	user specified length.
**
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	    .adf_maxstring		Max user size of any INGRES string.
**      adc_is_usr                      TRUE if the length, precision, and scale
**					provided in adc_dv are 'user specified',
**					FALSE if 'internal'.
**      adc_dv                          The data value to check.
**              .db_datatype            Datatype id for the data value.
**		.db_prec		Checked only for DECIMAL datatype.
**					Either the user-specified precision
**					and scale information (if adc_is_usr is
**					TRUE), or the internal precision and
**					scale information (if adc_is_usr is
**					FALSE) for the data value.
**              .db_length              Either the user specified length
**                                      (if adc_is_usr is TRUE), or the
**                                      internal length (if adc_is_usr
**                                      is FALSE) of the data value.
**					If the datatype is a fixed
**					length one, the user specified
**					length MUST be supplied as zero.
**	adc_rdv				Pointer to the DB_DATA_VALUE used to
**					return info in.  If this is a NULL
**					pointer, then only the check will be
**					done, and no info returned.
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
**	adc_rdv                         If not supplied as NULL, and the check
**					succeeded, then...
**	    .db_datatype		Datatype id for the data value.  Will
**					always be the same as input.
**	    .db_prec			Either the user specified precision and
**					scale information (if adc_is_usr is
**					FALSE), or the internal precision and
**					scale information (if adc_is_usr is
**					TRUE) for the data value.  Even if the
**					check fails, this will be set to 'some
**					legal' value for the datatype.
**	    .db_length			Either the user specified length (if
**					adc_is_usr is FALSE), or the internal
**					length (if adx_is_usr is TRUE) of the
**					data value.  If the datatype was a fixed
**					length one, the user specified length
**					will be returned as zero.  Even if the
**					check fails, this will be set to 'some
**					legal' value for the datatype.
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
**          E_AD0000_OK                 Proposed length is OK for a data
**                                      value of the given datatype.
**          E_AD2004_BAD_DTID           Datatype id unknown to ADF.
**          E_AD2005_BAD_DTLEN          adc_is_usr was FALSE, and the
**                                      proposed internal length is not
**                                      OK for a data value of the given
**                                      datatype.
**          E_AD2006_BAD_DTUSRLEN       adc_is_usr was TRUE, and the
**                                      proposed user specified length
**                                      is not OK for the given datatype.
**          E_AD2007_DT_IS_FIXLEN       adc_is_usr was TRUE, the proposed
**                                      length was non-zero, but the
**                                      given datatype is a fixed length
**                                      datatype (i.e. can't have a user
**                                      specified length).
**          E_AD2008_DT_IS_VARLEN       adc_is_usr was TRUE, the proposed
**                                      length was zero, but the given
**                                      datatype is a variable length
**                                      datatype (i.e. must have a user
**                                      specified length).
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
**      25-feb-86 (thurston)
**          Initial creation.
**      12-mar-86 (thurston)
**          Added the adc_rlen argument so that the .db_length field
**	    of the supplied DB_DATA_VALUE is not changed on returned,
**	    and also have it return either the user specified length
**	    or the internal length, whichever is NOT supplied.  This
**	    removes the need for the adi_ln_usr() and adi_ln_intrn()
**	    routines.
**	30-jun-86 (thurston)
**	    Corrected a small bug in this routine: If a data value's datatype
**          was not in the legal range of datatype IDs (0 < dt_ID <= ADI_MXDTS)
**          this routine would be looking at memory it had no business looking
**          at, and would probably access violate. 
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	19-feb-87 (thurston)
**	    Added support for nullable datatypes.
**	05-jun-87 (thurston)
**	    Made all low level lenchk functions return a `default' width
**	    even if the length check fails.
**	19-mar-89 (jrb)
**	    Changed interface for decimal.  Now checks/returns precision and
**	    scale info (although this has been #ifdef'd out initially). Also
**	    now fills in result data only if output variable (adc_rdv) is not
**	    NULL.
**       1-Jul-1993 (fred)
**          Changed ADP_LENCHK_FUNC prototype.  The second argument (adc_is_usr
**          was a bool.  The problem with this is that bool's are
**          system dependent (defined in compat.h) but the prototype
**          needs to be known/shared with user code for OME.
**          Therefore, we make it a i4  (== int for users).
**	09-nov-2010 (gupsh01) SIR 124685
**          Protype cleanup.
*/

DB_STATUS
adc_lenchk(
ADF_CB             *adf_scb,
i4                 adc_is_usr,
DB_DATA_VALUE      *adc_dv,
DB_DATA_VALUE	   *adc_rdv)
{
    i4		   	bdt = abs((i4) adc_dv->db_datatype);
    DB_STATUS		db_stat;
    i4			mbdt;

    mbdt = ADI_DT_MAP_MACRO(bdt);

    /* does the datatype id exist? */
    if (mbdt <= 0  ||  mbdt > ADI_MXDTS
	    ||  Adf_globs->Adi_dtptrs[mbdt] == NULL)
	return(adu_error(adf_scb, E_AD2004_BAD_DTID, 0));

    /*
    **   Call the function associated with this datatype to determine the 
    ** validity of and corresponding length of the datatype argument passed in.
    ** Currently, this is one function; but this level of indirection is
    ** provided for implementation of user-defined abstract datatypes in the
    ** future.
    */

    if (adc_dv->db_datatype > 0)	/* non-nullable */
    {
        db_stat = (*Adf_globs->Adi_dtptrs[mbdt]->adi_dt_com_vect.adp_lenchk_addr)
			(adf_scb, adc_is_usr, adc_dv, adc_rdv);
    }
    else				/* nullable */
    {
	DB_DATA_VALUE	    tmp_dv;

	tmp_dv.db_datatype  = bdt;
	tmp_dv.db_prec	    = adc_dv->db_prec;
	tmp_dv.db_length    = (adc_is_usr ? adc_dv->db_length
				       : adc_dv->db_length - 1);

        db_stat = (*Adf_globs->Adi_dtptrs[mbdt]->adi_dt_com_vect.adp_lenchk_addr)
			(adf_scb, adc_is_usr, &tmp_dv, adc_rdv);

	if (DB_SUCCESS_MACRO(db_stat)  &&  adc_rdv != NULL)
	{

            /*
            ** Make nullable again, just in case called routine
            ** changed it.
            */
            if (adc_rdv->db_datatype > 0)
                adc_rdv->db_datatype = -adc_rdv->db_datatype;

	    if (adc_is_usr)
		adc_rdv->db_length++;
	}
    }

    return(db_stat);
}


/*
** Name: adc_1lenchk_rti() - Check validity and resolve length, precision, and
**			     scale of INGRES datatypes.
**
** Description:
**      This function is an internal implementation of the ADF call
**	"adc_lenchk".  It performs the same function as the external 
**	"adc_lenchk".  Its purpose is to do the "real work" for datatypes
**	known to the RTI products.  These datatypes include c, text, char,
**	varchar, money, date, decimal, i, and f.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	    .adf_maxstring		Max user size of any INGRES string.
**      adc_is_usr                      TRUE if the length, precision, and scale
**					provided in adc_dv are 'user specified',
**					FALSE if 'internal'.
**      adc_dv                          The data value to check.
**              .db_datatype            Datatype id for the data value.
**		.db_prec		Either the user-specified precision
**					and scale information (if adc_is_usr is
**					TRUE), or the internal precision and
**					scale information (if adc_is_usr is
**					FALSE) for the data value.
**              .db_length              Either the user specified length
**                                      (if adc_is_usr is TRUE), or the
**                                      internal length (if adc_is_usr
**                                      is FALSE) of the data value.
**					If the datatype is a fixed
**					length one, the user specified
**					length MUST be supplied as zero.
**	adc_rdv				Pointer to the DB_DATA_VALUE used to
**					return info in.  If this is a NULL
**					pointer, then only the check will be
**					done, and no info returned.
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
**	adc_rdv                         If not supplied as NULL, and the check
**					succeeded, then...
**	    .db_datatype		Datatype id for the data value.  Will
**					always be the same as input.
**	    .db_prec			Either the user specified precision and
**					scale information (if adc_is_usr is
**					FALSE), or the internal precision and
**					scale information (if adc_is_usr is
**					TRUE) for the data value.  Even if the
**					check fails, this will be set to 'some
**					legal' value for the datatype.
**	    .db_length			Either the user specified length (if
**					adc_is_usr is FALSE), or the internal
**					length (if adx_is_usr is TRUE) of the
**					data value.  If the datatype was a fixed
**					length one, the user specified length
**					will be returned as zero.  Even if the
**					check fails, this will be set to 'some
**					legal' value for the datatype.
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
**          E_AD0000_OK                 Proposed length is OK for a data
**                                      value of the given datatype.
**          E_AD2004_BAD_DTID           Datatype id unknown to ADF.
**          E_AD2005_BAD_DTLEN          adc_is_usr was FALSE, and the
**                                      proposed internal length is not
**                                      OK for a data value of the given
**                                      datatype.
**          E_AD2006_BAD_DTUSRLEN       adc_is_usr was TRUE, and the
**                                      proposed user specified length
**                                      is not OK for the given datatype.
**          E_AD2007_DT_IS_FIXLEN       adc_is_usr was TRUE, the proposed
**                                      length was non-zero, but the
**                                      given datatype is a fixed length
**                                      datatype (i.e. can't have a user
**                                      specified length).
**          E_AD2008_DT_IS_VARLEN       adc_is_usr was TRUE, the proposed
**                                      length was zero, but the given
**                                      datatype is a variable length
**                                      datatype (i.e. must have a user
**                                      specified length).
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
**      23-mar-86 (ericj)
**	    Initially written.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	28-oct-86 (thurston)
**	    Added char and varchar support.
**	20-nov-86 (thurston)
**	    Added longtext support.
**	05-dec-86 (thurston)
**	    Fixed bug that said an internal length of DB_CNTSIZE for text was
**	    not valid.  It is valid, and represents the empty string.
**	10-feb-87 (thurston)
**	    Changed ADF_CHAR_MAX to DB_CHAR_MAX.
**	05-jun-87 (thurston)
**	    Made function return a `default' width even if the length
**	    check fails.
**	01-sep-88 (thurston)
**	    Changed the use of DB_CHAR_MAX and DB_MAXSTRING to
**	    adf_scb->adf_maxstring, since this is now a session settable
**	    parameter.
**	19-mar-89 (jrb)
**	    Changed interface to handle precision and scale info and do checks
**	    on it.  Also now fills in result data only if output variable
**	    (adc_rdv) is not NULL.
**	19-apr-89 (mikem)
**	    Logical key development.  Added support for logical_key datatype
**	    to adc_1lenchk_rti().
**	01-jun-89 (mikem)
**	    Bug fix for logical keys.  Was returning random datatype 
**	    id in result data value returned from adc_lenchk().
**	06-jul-89 (jrb)
**	    Added decimal support.
**	19-dec-89 (fred)
**	    Added Long Varchar support.
**	22-sep-1992 (fred)
**	    Added bit type(s) support.
**      25-sep-92 (stevet)
**          Added check for float which translate float(1-7) to float4
**          and float(8-53) to float8
**       6-Apr-1993 (fred)
**          Added byte string datatypes.
**       1-Jul-1993 (fred)
**          Changed ADP_LENCHK_FUNC prototype.  The second argument (adc_is_usr
**          was a bool.  The problem with this is that bool's are
**          system dependent (defined in compat.h) but the prototype
**          needs to be known/shared with user code for OME.
**          Therefore, we make it a i4  (== int for users).
**	19-apr-94 (robf)
**	    Updated error handling for security labels to
**	    return E_AD1098_BAD_LEN_SEC_INTRNL so
**	    user can tell better what the problem is.
**	31-jan-2001 (gupsh01)
**	    Added support for new unicode data type nchar and nvarchar
**	09-march-2001 (gupsh01)
**	    Fixed case with nchar data type.
**      03-apr-2001 (gupsh01)
**          Added support for unicode long nvarchar datatype.
**	13-nov-01 (inkdo01)
**	    Interpret float precision properly.
**	9-jan-02 (inkdo01)
**	    Tidy up float length validation (precision -> length conversion
**	    is now done in adi_encode_colspec).
**	30-may-02 (inkdo01)
**	    Once more lightly, to fix bloody floats for front end callers
**	    such as ABF that still pass precision in the length field.
**	03-dec-03 (gupsh01)
**	    Added support for internal UTF8 types.
**	23-dec-03 (inkdo01)
**	    Added support for bigint (64-bit integer).
**	22-mar-05 (inkdo01)
**	    Add support for collation IDs.
**	24-apr-06 (dougi)
**	    Add new date/time types.
**	01-aug-06 (gupsh01)
**	    Fixed length of DB_TMW_TYPE and DB_TSW_TYPE.
**	11-oct-2006 (dougi)
**	    Record precision for INTERVAL DAY TO SECOND.
**	27-sep-2007 (gupsh01)
**	    To accomodate UTF8 character sets, modify the length check 
**	    for nchar/nvarchar, to allow a max of DB_MAXSTRING instead of
**	    checking against adf_maxstring. Unicode types are generated
**	    for internal processing and single byte UTF8 characters in 
**	    char/varchar  can expand on conversion to nchar/nvarchar types.
**	15-oct-2007 (gupsh01)
**	    Previous change accidently suppressed error message for longer
**	    lengths on a install with UTF8 character set.
**	01-oct-2008 (gupsh01)
**	    Break out the case for byte/varbyte etc which should not be 
**	    effected by restriction in lengths on char/varchar in UTF8 
**	    installations.
**	27-Oct-2008 (kiria01) SIR120473
**	    Allow for length check support for compiled patterns.
*/

DB_STATUS
adc_1lenchk_rti(
ADF_CB          *adf_scb,
i4	    	adc_is_usr,
DB_DATA_VALUE	*adc_dv,
DB_DATA_VALUE	*adc_rdv)
{
    DB_STATUS	    db_stat = E_DB_OK;
    i4		    dt;

    switch (adc_dv->db_datatype)
    {
      case DB_CHR_TYPE:
      case DB_CHA_TYPE:
	/* c or char datatype; note, internal and user lengths are the same */

	if (adc_rdv != NULL)
	{
	    adc_rdv->db_datatype = adc_dv->db_datatype;
	    adc_rdv->db_length = 1;	/* just some legal length */
	    adc_rdv->db_prec   = 0;
	    adc_rdv->db_collID = adc_dv->db_collID;
	}
	
	if (adc_is_usr  &&  adc_dv->db_length == 0)
	{
	    db_stat = adu_error(adf_scb, E_AD2008_DT_IS_VARLEN, 0);
	}
	else if ( (adc_dv->db_length <= 0) || 
		  ((adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
		   adc_dv->db_collID != DB_UCS_BASIC_COLL &&
		   (adc_dv->db_length > adf_scb->adf_maxstring/2)) || 
		  (adc_dv->db_length > adf_scb->adf_maxstring) || 
		  (adc_dv->db_length > DB_MAXSTRING) )
	{
	    if (adc_is_usr)
		db_stat = adu_error(adf_scb, E_AD2006_BAD_DTUSRLEN, 0);
	    else
		db_stat = adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0);
	}
	else if (adc_rdv != NULL)
	{
	    adc_rdv->db_length = adc_dv->db_length;
	}
	break;

      case DB_BYTE_TYPE:
      case DB_NCHR_TYPE:
      case DB_NBR_TYPE:

	if (adc_rdv != NULL)
	{
	    adc_rdv->db_datatype = adc_dv->db_datatype;
	    adc_rdv->db_length = 1;	/* just some legal length */
	    adc_rdv->db_prec   = 0;
	    adc_rdv->db_collID = adc_dv->db_collID;
	}
	
	if (adc_is_usr  &&  adc_dv->db_length == 0)
	{
	    db_stat = adu_error(adf_scb, E_AD2008_DT_IS_VARLEN, 0);
	}
	else if ( (adc_dv->db_length <= 0) || 
		  ((adc_dv->db_datatype != DB_NCHR_TYPE) && 
		   (adc_dv->db_length > adf_scb->adf_maxstring)) || 
		  (adc_dv->db_length > DB_MAXSTRING) )
	{
	    if (adc_is_usr)
		db_stat = adu_error(adf_scb, E_AD2006_BAD_DTUSRLEN, 0);
	    else
		db_stat = adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0);
	}
	else if (adc_rdv != NULL)
	{
	/*
	** If this is the unicode type NVCHR data type then this
	** length will be dependent on which way the conversion
	** is taking place. The length will be calculated like the
	** nvarchar type. ( see comment below )
	*/
	if (adc_dv->db_datatype != DB_NCHR_TYPE)
	{
	    adc_rdv->db_length = adc_dv->db_length;
	}
	else
	{
	    if (adc_dv->db_datatype == DB_NCHR_TYPE )
	    {
	        if (adc_is_usr)
	        {
	            adc_rdv->db_length = adc_dv->db_length * DB_ELMSZ;
	        }
	        else
	        {
	            if ( adc_dv->db_length % DB_ELMSZ != 0 )
	            {
	               db_stat = adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0);
	            }
	            else
	                adc_rdv->db_length = adc_dv->db_length / DB_ELMSZ ;
	         }
	    }
	}
	}
	break;

      case DB_DTE_TYPE:
	/* Date datatype, a fixed length datatype. */

	/* Set result values whether check fails or not */
	if (adc_rdv != NULL)
	{
	    adc_rdv->db_datatype = DB_DTE_TYPE;
	    adc_rdv->db_length   = (adc_is_usr ? ADF_DTE_LEN : 0);
	    adc_rdv->db_prec     = 0;
	}

	if (adc_is_usr  &&  adc_dv->db_length != 0)
	    db_stat = adu_error(adf_scb, E_AD2007_DT_IS_FIXLEN, 0);
	else if (!adc_is_usr  &&  adc_dv->db_length != ADF_DTE_LEN)
	    db_stat = adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0);

	break;

      case DB_ADTE_TYPE:
	/* ANSI date datatype, a fixed length datatype. */

	/* Set result values whether check fails or not */
	if (adc_rdv != NULL)
	{
	    adc_rdv->db_datatype = DB_ADTE_TYPE;
	    adc_rdv->db_length   = (adc_is_usr ? ADF_ADATE_LEN : 0);
	    adc_rdv->db_prec     = 0;
	}

	if (adc_is_usr  &&  adc_dv->db_length != 0)
	    db_stat = adu_error(adf_scb, E_AD2007_DT_IS_FIXLEN, 0);
	else if (!adc_is_usr  &&  adc_dv->db_length != ADF_ADATE_LEN)
	    db_stat = adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0);

	break;

      case DB_TMWO_TYPE:
      case DB_TMW_TYPE:
      case DB_TME_TYPE:
	/* Time datatypes, fixed length datatypes. */

	/* Set result values whether check fails or not */
	if (adc_rdv != NULL)
	{
	    adc_rdv->db_datatype = adc_dv->db_datatype;
	    adc_rdv->db_length   = (adc_is_usr ? ADF_TIME_LEN : 0);
	    adc_rdv->db_prec     = adc_dv->db_prec;
	}

	if (adc_is_usr  &&  adc_dv->db_length != 0)
	    db_stat = adu_error(adf_scb, E_AD2007_DT_IS_FIXLEN, 0);
	else if (!adc_is_usr  &&  adc_dv->db_length != ADF_TIME_LEN &&
			adc_dv->db_length != ADF_TIME_LEN-2)
	    db_stat = adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0);

	break;

      case DB_TSWO_TYPE:
      case DB_TSW_TYPE:
      case DB_TSTMP_TYPE:
	/* Timestamp datatypes, fixed length datatypes. */

	/* Set result values whether check fails or not */
	if (adc_rdv != NULL)
	{
	    adc_rdv->db_datatype = adc_dv->db_datatype;
	    adc_rdv->db_length   = (adc_is_usr ? ADF_TSTMP_LEN : 0);
	    adc_rdv->db_prec     = adc_dv->db_prec;
	}

	if (adc_is_usr  &&  adc_dv->db_length != 0)
	    db_stat = adu_error(adf_scb, E_AD2007_DT_IS_FIXLEN, 0);
	else if (!adc_is_usr  &&  adc_dv->db_length != ADF_TSTMP_LEN &&
			adc_dv->db_length != ADF_TSTMP_LEN-2)
	    db_stat = adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0);

	break;

      case DB_INYM_TYPE:
	/* Interval year to month datatype, a fixed length datatype. */

	/* Set result values whether check fails or not */
	if (adc_rdv != NULL)
	{
	    adc_rdv->db_datatype = adc_dv->db_datatype;
	    adc_rdv->db_length   = (adc_is_usr ? ADF_INTYM_LEN : 0);
	    adc_rdv->db_prec     = 0;
	}

	if (adc_is_usr  &&  adc_dv->db_length != 0)
	    db_stat = adu_error(adf_scb, E_AD2007_DT_IS_FIXLEN, 0);
	else if (!adc_is_usr  &&  adc_dv->db_length != ADF_INTYM_LEN)
	    db_stat = adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0);

	break;

      case DB_INDS_TYPE:
	/* Interval day to second datatype, a fixed length datatype. */

	/* Set result values whether check fails or not */
	if (adc_rdv != NULL)
	{
	    adc_rdv->db_datatype = adc_dv->db_datatype;
	    adc_rdv->db_length   = (adc_is_usr ? ADF_INTDS_LEN : 0);
	    adc_rdv->db_prec     = adc_dv->db_prec;
	}

	if (adc_is_usr  &&  adc_dv->db_length != 0)
	    db_stat = adu_error(adf_scb, E_AD2007_DT_IS_FIXLEN, 0);
	else if (!adc_is_usr  &&  adc_dv->db_length != ADF_INTDS_LEN)
	    db_stat = adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0);

	break;

      case DB_FLT_TYPE:
	/* Float datatype, which can either be 4 or 8 bytes long */

	/* set default length to 4, in case check fails */
	if (adc_rdv != NULL)
	{
	    adc_rdv->db_datatype = DB_FLT_TYPE;
	    adc_rdv->db_length   = 4;
	    adc_rdv->db_prec     = 0;
	}

	if (adc_dv->db_length != 4 && adc_dv->db_length != 8)
	{
	    /* Precision check is done in adi_encode_... for its callers
	    ** (e.g. PSF), or here for others (such as ABF). */
	    if (adc_is_usr && adc_rdv != NULL)
	    {
		if (adc_dv->db_length > DB_MAX_F8PREC ||
		    adc_dv->db_length < 0)
		    db_stat = adu_error(adf_scb, E_AD2006_BAD_DTUSRLEN, 0);
		else
		{
		    /* Do the length to F4 and F8 mapping */
		    if(adc_dv->db_length <= DB_MAX_F4PREC)
			adc_rdv->db_length = 4;
		    else
			adc_rdv->db_length = 8;
		}
	    }
	    else
	    {
		db_stat = adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0);
	    }
	}
	else
	{
	    if (adc_rdv != NULL)
		adc_rdv->db_length = adc_dv->db_length;
	}
	break;

      case DB_DEC_TYPE:
	/* set default to precision 1, scale 0, in case check fails */
	if (adc_rdv != NULL)
	{
	    adc_rdv->db_datatype = DB_DEC_TYPE;
	    adc_rdv->db_length   = 1;
	    adc_rdv->db_prec	 = DB_PS_ENCODE_MACRO(1,0);
	}

	if (	adc_dv->db_length < 1
	    ||  adc_dv->db_length > DB_MAX_DECLEN
	   )
	{
	    if (adc_is_usr)
	    {
		if (adc_dv->db_length)
		    db_stat = adu_error(adf_scb, E_AD2006_BAD_DTUSRLEN, 0);
		else
		    db_stat = adu_error(adf_scb, E_AD2008_DT_IS_VARLEN, 0);
	    }
	    else
	    {
		db_stat = adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0);
	    }
	}
	else
	{
	    i4		prec  = DB_P_DECODE_MACRO(adc_dv->db_prec);	    
	    i4		scale = DB_S_DECODE_MACRO(adc_dv->db_prec);	    

	    if (    prec < 1
		||  prec > DB_MAX_DECPREC
		||  DB_PREC_TO_LEN_MACRO(prec) != adc_dv->db_length
	       )
	    {
		db_stat = adu_error(adf_scb, E_AD200B_BAD_PREC, 0);
	    }
	    else
	    {
		if (scale < 0  ||  scale > prec)
		{
		    db_stat = adu_error(adf_scb, E_AD200C_BAD_SCALE, 0);
		}
		else
		{
		    if (adc_rdv != NULL)
		    {
			adc_rdv->db_length = adc_dv->db_length;
			adc_rdv->db_prec   = adc_dv->db_prec;
		    }
		}
	    }
	}
	break;
	    
      case DB_BOO_TYPE:
        if (adc_rdv)
	{
	    adc_rdv->db_datatype = DB_BOO_TYPE;
	    adc_rdv->db_length = (adc_is_usr ? sizeof(i1) : 0);
	    adc_rdv->db_prec = 0;
	}
        if (adc_is_usr && adc_dv->db_length)
            db_stat = adu_error(adf_scb, E_AD2007_DT_IS_FIXLEN, 0);
	else if (!adc_is_usr && adc_dv->db_length != sizeof(i1))
	    db_stat = adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0);
	break;
         
      case DB_INT_TYPE:
	/* Integer datatype, which can be 1, 2, 4 or 8 bytes long */

	/* set default length to 2, in case check fails */
	if (adc_rdv != NULL)
	{
	    adc_rdv->db_datatype = DB_INT_TYPE;
	    adc_rdv->db_length   = 2;
	    adc_rdv->db_prec     = 0;
	}

	if (	adc_dv->db_length != 8
	    &&  adc_dv->db_length != 4
	    &&  adc_dv->db_length != 2
	    &&	adc_dv->db_length != 1
	   )
	{
	    if (adc_is_usr)
	    {
		if (adc_dv->db_length)
		    db_stat = adu_error(adf_scb, E_AD2006_BAD_DTUSRLEN, 0);
		else
		    db_stat = adu_error(adf_scb, E_AD2008_DT_IS_VARLEN, 0);
	    }
	    else
	    {
		db_stat = adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0);
	    }
	}
	else
	{
	    if (adc_rdv != NULL)
		adc_rdv->db_length = adc_dv->db_length;
	}
	break;

      case DB_MNY_TYPE:
	/* Money datatype, a fixed length datatype. */

	/* Set result length whether check fails or not */
	if (adc_rdv != NULL)
	{
	    adc_rdv->db_datatype = DB_MNY_TYPE;
	    adc_rdv->db_length   = (adc_is_usr ? ADF_MNY_LEN : 0);
	    adc_rdv->db_prec     = 0;
	}

	if (adc_is_usr && adc_dv->db_length != 0)
	    db_stat = adu_error(adf_scb, E_AD2007_DT_IS_FIXLEN, 0);
	else if (!adc_is_usr && adc_dv->db_length != ADF_MNY_LEN)
	    db_stat = adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0);
	break;

      case DB_TXT_TYPE:
      case DB_VCH_TYPE:
      {
        i4 maxlen = adf_scb->adf_maxstring;

        if ((adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
		   adc_dv->db_collID != DB_UCS_BASIC_COLL)
            maxlen = adf_scb->adf_maxstring/2;

	/* Set result length whether check fails or not */
	if (adc_rdv != NULL)
	{
	    adc_rdv->db_datatype = adc_dv->db_datatype;
	    adc_rdv->db_length   = (adc_is_usr ? 1 + DB_CNTSIZE : 1);
	    adc_rdv->db_prec     = 0;
	    adc_rdv->db_collID	 = adc_dv->db_collID;
	}

	if (adc_is_usr)
	{
	    if ( adc_dv->db_length <= 0 || adc_dv->db_length > maxlen)
	    {
		if (adc_dv->db_length)
		    db_stat = adu_error(adf_scb, E_AD2006_BAD_DTUSRLEN, 0);
		else
		    db_stat = adu_error(adf_scb, E_AD2008_DT_IS_VARLEN, 0);
	    }
	    else
	    {
		if (adc_rdv != NULL)
		{
		    adc_rdv->db_length = adc_dv->db_length + DB_CNTSIZE;
	        }
	    }
	}
	else
	{
	    if (    (adc_dv->db_length < DB_CNTSIZE) ||  
		    ( adc_dv->db_length > (maxlen + DB_CNTSIZE) )
	       )
	    {
		db_stat = adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0);
	    }
	    else
	    {
		if (adc_rdv != NULL)
		{
		         adc_rdv->db_length = adc_dv->db_length - DB_CNTSIZE;
		}
	    }
	}
	break;
      }

      case DB_LTXT_TYPE:
      case DB_VBYTE_TYPE:
      case DB_NVCHR_TYPE:
      case DB_UTF8_TYPE:
      case DB_PAT_TYPE:
        /*
        ** For NVCHR & PAT data type, To calculate the internal size based on
        ** user size, multiply by 2 then add 2. To calculate user size
        ** given internal length, subtract 2 then divide by 2; note
        ** that the internal length must be divisible by 2 in this case.
        */

	/* Set result length whether check fails or not */
	if (adc_rdv != NULL)
	{
	    adc_rdv->db_datatype = adc_dv->db_datatype;
	    adc_rdv->db_length   = (adc_is_usr ? 1 + DB_CNTSIZE : 1);
	    adc_rdv->db_prec     = 0;
	    adc_rdv->db_collID	 = adc_dv->db_collID;
	}

	if (adc_is_usr)
	{
	    if (    adc_dv->db_length <= 0
		||  ((adc_dv->db_datatype == DB_NVCHR_TYPE ||
		      adc_dv->db_datatype == DB_PAT_TYPE)
     			&&
		      adc_dv->db_length > DB_MAX_NVARCHARLEN
		    )
		||
		    (	adc_dv->db_length > adf_scb->adf_maxstring
		     &&
			adc_dv->db_datatype != DB_LTXT_TYPE
		    )
	       )
	    {
		if (adc_dv->db_length)
		    db_stat = adu_error(adf_scb, E_AD2006_BAD_DTUSRLEN, 0);
		else
		    db_stat = adu_error(adf_scb, E_AD2008_DT_IS_VARLEN, 0);
	    }
	    else
	    {
		if (adc_rdv != NULL)
		{
		    if (adc_dv->db_datatype == DB_NVCHR_TYPE ||
			adc_dv->db_datatype == DB_PAT_TYPE )
		    {
		   	 adc_rdv->db_length = adc_dv->db_length * DB_ELMSZ
               		                     + DB_CNTSIZE;
		    }
             	    else
		        adc_rdv->db_length = adc_dv->db_length + DB_CNTSIZE;
	        }
	    }
	}
	else
	{
	    if (    (adc_dv->db_length < DB_CNTSIZE)
		||
		    (	
		       ( adc_dv->db_datatype != DB_LTXT_TYPE ) && 
		       ( adc_dv->db_datatype != DB_NVCHR_TYPE ) &&
		       ( adc_dv->db_datatype != DB_PAT_TYPE ) &&
		       ( adc_dv->db_length > (adf_scb->adf_maxstring + DB_CNTSIZE) )
		    )
		||  
		    (
		     ( adc_dv->db_datatype == DB_NVCHR_TYPE ||
			adc_dv->db_datatype == DB_PAT_TYPE)
		     &&
		     (( adc_dv->db_length > (DB_MAXSTRING + DB_CNTSIZE)) ||
		      ( ((adc_dv->db_length - DB_CNTSIZE) % DB_ELMSZ) != 0 )
		     )
    		    )
	       )
	    {
		db_stat = adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0);
	    }
	    else
	    {
		if (adc_rdv != NULL)
		{
		    if (adc_dv->db_datatype == DB_NVCHR_TYPE ||
			adc_dv->db_datatype == DB_PAT_TYPE)
     		    {
		         adc_rdv->db_length = ( adc_dv->db_length
               			                  - DB_CNTSIZE) / DB_ELMSZ ;
     		    }
	            else
		    {
		         adc_rdv->db_length = adc_dv->db_length - DB_CNTSIZE;
		     }
		 }
	    }
	}
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
      case DB_LNVCHR_TYPE:
	/* Long Varchar datatype, a fixed length datatype, internally */

	/* Set result values whether check fails or not */
	if (adc_rdv != NULL)
	{
	    adc_rdv->db_datatype = adc_dv->db_datatype;
	    adc_rdv->db_length   = (adc_is_usr ? sizeof(ADP_PERIPHERAL) : 0);
	    adc_rdv->db_prec     = 0;
	    adc_rdv->db_collID	 = adc_dv->db_collID;
	}

	if (adc_is_usr  &&  adc_dv->db_length != 0)
	    db_stat = adu_error(adf_scb, E_AD2007_DT_IS_FIXLEN, 0);
	else if (!adc_is_usr  &&  adc_dv->db_length != sizeof(ADP_PERIPHERAL))
	    db_stat = adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0);

	break;

      case DB_LBLOC_TYPE:
      case DB_LCLOC_TYPE:
      case DB_LNLOC_TYPE:
	/* Long locator,  a fixed length datatype, internally */

	/* Set result values whether check fails or not */
	if (adc_rdv != NULL)
	{
	    adc_rdv->db_datatype = adc_dv->db_datatype;
	    adc_rdv->db_length   = (adc_is_usr ? ADP_LOC_PERIPH_SIZE : 0);
	    adc_rdv->db_prec     = 0;
	    adc_rdv->db_collID	 = adc_dv->db_collID;
	}

	if (adc_is_usr  &&  adc_dv->db_length != 0)
	    db_stat = adu_error(adf_scb, E_AD2007_DT_IS_FIXLEN, 0);
	else if (!adc_is_usr  &&  adc_dv->db_length != ADP_LOC_PERIPH_SIZE)
	    db_stat = adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0);

	break;

      case DB_LOGKEY_TYPE:
      case DB_TABKEY_TYPE:

	/* Logical key  datatypes, a fixed length datatypes. */

	/* Set result values whether check fails or not */
	dt = adc_dv->db_datatype;

	if (adc_rdv != NULL)
	{
	    adc_rdv->db_datatype = dt;
	    if (dt == DB_LOGKEY_TYPE)
	    {
		adc_rdv->db_length = (adc_is_usr ? DB_LEN_OBJ_LOGKEY : 0);
	    }
	    else 
	    {
		adc_rdv->db_length = (adc_is_usr ? DB_LEN_TAB_LOGKEY : 0);
	    }
	    adc_rdv->db_prec = 0;
	}

	if (adc_is_usr && adc_dv->db_length != 0)
	{
	    db_stat = adu_error(adf_scb, E_AD2007_DT_IS_FIXLEN, 0);
	}
	else if ((!adc_is_usr && (dt == DB_LOGKEY_TYPE) && 
		 (adc_dv->db_length != DB_LEN_OBJ_LOGKEY)) ||
		 (!adc_is_usr && (dt == DB_TABKEY_TYPE) && 
		 (adc_dv->db_length != DB_LEN_TAB_LOGKEY)))
	{
	    db_stat = adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0);
	}

	break;
      case DB_BIT_TYPE:
      case DB_VBIT_TYPE:
	{
	    i4			valid_bytes;
	    DB_BIT_STRING	bs;

	    valid_bytes = DB_MAXTUP;

	    if (adc_rdv)
	    {
		adc_rdv->db_datatype = adc_dv->db_datatype;
		adc_rdv->db_prec = 1;
		adc_rdv->db_length = 1;
	    }
	    
	    if (adc_is_usr && adc_dv->db_length == 0)
	    {
		db_stat = adu_error(adf_scb, E_AD2008_DT_IS_VARLEN, 0);
	    }
	    else if (adc_is_usr)
	    {
		
		if (adc_dv->db_datatype == DB_VBIT_TYPE)
		    valid_bytes -= sizeof(bs.db_b_count);
		
		if ((adc_dv->db_length < 0) ||
		    (adc_dv->db_length > (valid_bytes * BITSPERBYTE)))
		{
		    db_stat = adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0);
		}
		else if (adc_rdv)
		{
		    adc_rdv->db_prec = adc_dv->db_length % BITSPERBYTE;
		    if (adc_rdv->db_prec == 0)
			adc_rdv->db_prec = BITSPERBYTE;

		    adc_rdv->db_length = adc_dv->db_length / BITSPERBYTE;
		    if (adc_dv->db_length % BITSPERBYTE)
			adc_rdv->db_length += 1;
		    if (adc_dv->db_datatype == DB_VBIT_TYPE)
		    {
			adc_rdv->db_length += sizeof(bs.db_b_count);
		    }
		}
	    }
	    else /* Then it must be a system length */
	    {
		if ((adc_dv->db_length < 0)
		    || (adc_dv->db_length > DB_MAXTUP))
		{
		    db_stat = adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0);
		}
		else if (adc_rdv)
		{
		    valid_bytes = adc_dv->db_length;
		    if (adc_dv->db_datatype == DB_VBIT_TYPE)
			valid_bytes -= sizeof(bs.db_b_count);

		    adc_rdv->db_length =  valid_bytes * BITSPERBYTE;
		    if (adc_dv->db_prec != 0)
			adc_rdv->db_length = adc_rdv->db_length
			           - BITSPERBYTE + adc_dv->db_prec;
		}
	    }
	    break;
	}

      default:
	db_stat = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
    }	/* end of switch stmt */

    return(db_stat);
}


/*
** Name: adc_2lenchk_bool() - Check validity of ADF datatype "boolean".
**
** Description:
**       This function is an internal implementation of the ADF call
**	"adc_lenchk".  It performs the same function as the external 
**	"adc_lenchk".  Its purpose is to do the "real work" for the ADF
**	datatype "boolean".
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adc_is_usr                      TRUE if the length, precision, and scale
**					provided in adc_dv are 'user
**					specified', FALSE if 'internal'.
**					Please note, that this should always be
**					FALSE, since the "boolean" datatype is
**					only an internal entity.
**      adc_dv                          The data value to check.
**              .db_datatype            Datatype id for the data value.
**              .db_length              The internal length of the data value.
**		.db_prec		The internal precision of the value.
**	adc_rdv				Pointer to the DB_DATA_VALUE used to
**					return info in.  If this is a NULL
**					pointer, then only the check will be
**					done, and no info returned.
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
**	 adc_rdv                        If not supplied as NULL, and the check
**					succeeded, then...
**	    .db_datatype		Datatype id for the data value.  Will
**					always be DB_BOO_TYPE.
**	    .db_length			Internal length of the boolean.
**	    .db_prec			Internal precision of the boolean.
**					NOTE:  Since "boolean" is an internal
**					datatype the user specified length and
**					precision will be returned as zero.
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
**          E_AD0000_OK                 Proposed length is OK for a data
**                                      value of the given datatype.
**          E_AD2005_BAD_DTLEN          adc_is_usr was FALSE, and the
**                                      proposed internal length is not
**                                      OK for a data value of the given
**                                      datatype.
**	    E_AD9999_INTERNAL_ERROR	Bad news.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      29-dec-86 (thurston)
**	    Initially written.
**	05-jun-87 (thurston)
**	    Made function return a `default' width even if the length
**	    check fails.
**	19-mar-89 (jrb)
**	    Changed interface for decimal: we accept a ptr to a DB_DATA_VALUE
**	    now instead of a ptr to an i4.
**       1-Jul-1993 (fred)
**          Changed ADP_LENCHK_FUNC prototype.  The second argument (adc_is_usr
**          was a bool.  The problem with this is that bool's are
**          system dependent (defined in compat.h) but the prototype
**          needs to be known/shared with user code for OME.
**          Therefore, we make it a i4  (== int for users).
*/

DB_STATUS
adc_2lenchk_bool(
ADF_CB              *adf_scb,
i4		    adc_is_usr,
DB_DATA_VALUE	    *adc_dv,
DB_DATA_VALUE	    *adc_rdv)
{
    DB_STATUS	    db_stat = E_DB_OK;


    /* Set result length whether check fails or not */
    if (adc_rdv != NULL)
    {
	adc_rdv->db_datatype = DB_BOO_TYPE;
	adc_rdv->db_length   = (adc_is_usr ? sizeof(i1) : 0);
	adc_rdv->db_prec     = 0;
    }

    if (adc_dv->db_datatype != DB_BOO_TYPE)
	db_stat = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
    else if (!adc_is_usr && adc_dv->db_length != sizeof(i1))
	db_stat = adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0);

    return(db_stat);
}

/*{
** Name: adc_seglen() - Determine segment length for underlying datatype.
**
** Description:
**	This function is the external ADF call "adc_seglen()" which
**      determines the valid length for segments of a large object.
**
**      This routine is used only for large objects.  For large
**      objects, the object is segmented for storage.  This means that
**      it is stored as a collection of segments of some underlying
**      datatype.  For example, long varchar objects are stored as a
**      sequence of varchar segments.  Each of these segments must
**      have its own length.
**
**      The length of the segments is determined from two sources.
**      The underlying data manager has some restriction about
**      how large a segment it is will to store.  In the future,
**      this restriction may vary from instance to instance, but for
**      now, there is a single restriction for the data manager.
**
**      Once the maximum size (as determined by the data manager) is
**      known, then object type specific requirements come into play.
**      In the case of long varchar, there are no object specific
**      requirements, because any number of bytes can be used to store
**      character strings.  However, for other object types, say one
**      in which the atomic elements are 16 byte quantities, then the
**      maximum size must be divisible by 16.  In this case if the data
**      manager based restriction was, say, 1000, the maximum space
**      used by such an object would be 992 bytes.  Thus, for such an
**      object, if the data manager restriction were 1000, then this
**      routine would fill in the result's length as 992.
**
**      This routine is called with the "adc_rdv" parameter partially
**      filled in.  The db_datatype field will have already been set
**      up with the underlying datatype corresponding to the adc_l_dt
**      parameter.  The db_length field will be filled with the data
**      manager length (the maximum allowed).  This object type
**      specific routine is then given the opportunity to adjust
**      (down only) the maximum length value.
**
**      Note that the object type specific instance of this routine is
**      optional.  It is necessary only when the object cannot use the
**      maximum length provided.  In all other cases, the object type
**      specific routine address should be left NULL.
**
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	    .adf_maxstring		Max user size of any INGRES string.
**      adc_l_dt                        Large object type in question.
**	adc_rdv				Pointer to the DB_DATA_VALUE used to
**					return info in.
**          .db_datatype                    Set to the underlying
**                                          datatype for adc_l_dt.
**
**          .db_length                      Set to the data manager
**                                          maximum value.
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
**	adc_rdv
**	    .db_length			Length for underlying specification.
**                                      Note that this is the internal
**                                      length (in bytes) for the object.
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
**          E_AD0000_OK                 Proposed length is OK for a data
**                                      value of the given datatype.
**          E_AD2004_BAD_DTID           Datatype id unknown to ADF.
**          E_AD2005_BAD_DTLEN          adc_is_usr was FALSE, and the
**                                      proposed internal length is not
**                                      OK for a data value of the given
**                                      datatype.
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
**       12-Oct-1993 (fred)
**          Created
**       09-nov-1993 (stevet)
**          Pass non-null value to seglen routine, all other datatype
**          routines only handle non-null datatype value.
*/

DB_STATUS
adc_seglen(ADF_CB             *adf_scb,
	   DB_DT_ID           adc_l_dt,
	   DB_DATA_VALUE      *adc_rdv)
{
    DB_STATUS	        status;
    i4		   	bdt = abs(adc_l_dt);
    i4			mbdt;

    mbdt = ADI_DT_MAP_MACRO(bdt);

    /* does the datatype id exist? */
    if (mbdt <= 0  ||  mbdt > ADI_MXDTS
	    ||  Adf_globs->Adi_dtptrs[mbdt] == NULL)
	return(adu_error(adf_scb, E_AD2004_BAD_DTID, 0));

    if (Adf_globs->Adi_dtptrs[mbdt]->adi_dtstat_bits & AD_PERIPHERAL)
    {
	if (Adf_globs->Adi_dtptrs[mbdt]->adi_dt_com_vect.adp_seglen_addr
	    != 0)
	{
	    status =
		Adf_globs->Adi_dtptrs[mbdt]->adi_dt_com_vect.adp_seglen_addr
		    (adf_scb, (DB_DT_ID)bdt, adc_rdv);
	}
	else
	{
	    /*
	    ** If no seglen routine is provided, then the assumption
	    ** is that the datatype will work with the maximum length.
	    */
	    
	    status = E_DB_OK;
	}
    }
    else
    {
	status = adu_error(adf_scb, E_AD2004_BAD_DTID, 0);
    }
    return(status);
}

