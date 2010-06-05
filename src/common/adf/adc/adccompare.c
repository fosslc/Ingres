/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <adudate.h>
#include    <adumoney.h>
#include    <mh.h>
/*  [@#include@]... */

/**
**
**  Name: ADCCOMPARE.C - Compare 2 data values of same datatype.
**
**  Description:
**      This file contains all of the routines necessary to perform the
**      external ADF call "adc_compare()" which compares two data values of the
**      same datatype.  (In most cases the two lengths will also be the same,
**      however this is not mandatory.)  This will tell the caller if the
**      supplied values are equal, or if not, which one is "greater than" the
**      other. 
**
**      This file defines:
**
**          adc_compare() - Compare 2 data values of same datatype.
**
**
**  History:
**      25-feb-86 (thurston)
**          Initial creation.
**	9-apr-86 (ericj)
**	    written.
**	13-may-86 (ericj)
**	    Replaced i1_to_i4 routine with I1_CHECK_MACRO.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	03-feb-87 (thurston)
**	    Removed the routines adc_date_compare() and adc_money_compare()
**	    since they are being replaced with adu_4date_cmp() and
**	    adu_6mny_cmp().
**	17-feb-87 (thurston)
**	    Upgraded to handle nullable datatypes.
**	19-feb-87 (thurston)
**	    Reversed all STRUCT_ASSIGN_MACRO's to conform to CL spec.
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	28-jul-87 (thurston)
**	    Documented to state that the two DB_DATA_VALUE inputs to this
**	    routine may have different lengths, as long as the datatypes are
**	    exactly the same.
**	28-apr-89 (fred)
**	    Altered to use ADI_DT_MAP_MACRO().
**	05-jul-89 (jrb)
**	    Added decimal datatype.
**      21-sep-1992 (fred)
**          Added Bit types.
**	22-Oct-1992 (fred)
**	    Modified bit types to ANSI data representation.
**      22-dec-1992 (stevet)
**          Added function prototype.
**      05-Apr-1993 (fred)
**          Added byte datatypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      11-aug-93 (ed)
**          added missing includes
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2004 (schka24)
**	    add i8 support
**      05-oct-2009 (joea)
**          Add adc_bool_compare.
**/


/*{
** Name: adc_compare()  - Compare 2 data values of same datatype.
**
** Description:
**      This function is the external ADF call "adc_compare()" which compares
**      two data values of the same datatype.  (In most cases the two lengths
**      will also be the same, however this is not mandatory.)  This will tell
**      the caller if the supplied values are equal, or if not, which one is
**      "greater than" the other. 
**
**      			******* NOTE *******
**
**      This routine is used to do comparisons for the purpose of sorting, or
**      group by.  That means that a NULL value will compare equal to another
**      NULL value, and greater than any other value.  It is also used in keyed
**	joins, but in this case the NULL value issue would be handled by the CX
**      special instruction ADE_COMPARE (in which NULL != NULL) before this
**	routine would get called.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adc_dv1                         Pointer to the 1st data value
**                                      to compare.
**		.db_datatype		Datatype id of 1st data value.
**		.db_prec		Precision/Scale of 1st data value.
**		.db_length  		Length of 1st data value.
**		.db_data    		Pointer to the actual data for
**		         		1st data value.
**      adc_dv2                         Pointer to the 2nd data value
**                                      to compare.
**		.db_datatype		Datatype id of 2nd data value.
**					(Must be same as
**					adc_dv1->db_datatype.)
**		.db_prec		Precision/Scale of 2nd data value.
**		.db_length  		Length of 2nd data value.
**		.db_data    		Pointer to the actual data for
**		         		2nd data value.
**      adc_cmp_result                  Pointer to the i4  to put the
**				 	comparison result.
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
**     *adc_cmp_result                  Result of comparison.  This is
**                                      guaranteed to be:
**                                          < 0   if 1st < 2nd
**                                          = 0   if 1st = 2nd
**                                          > 0   if 1st > 2nd
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
**          E_AD0000_OK                 Operation succeeded.
**          E_AD2004_BAD_DTID           Datatype id unknown to ADF.
**          E_AD2005_BAD_DTLEN          Internal length is illegal for
**					the given datatype.  Note, this error
**					number will be returned only when this
**					routine is conditionally compiled with
**					xDEBUG.
**	    E_AD200B_BAD_PREC		Invalid precision specified for DECIMAL;
**					must be: 1<= precision <=DB_MAX_DECPREC
**	    E_AD200C_BAD_SCALE		Invalid scale specified for DECIMAL;
**					must be: 0<= scale <=precision
**          E_AD3001_DTS_NOT_SAME       The datatypes of the 2 data
**                                      values are different.
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
**	9-apr-86 (ericj)
**	    written.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	17-feb-87 (thurston)
**	    Upgraded to handle nullable datatypes.
**	28-jul-87 (thurston)
**	    Documented to state that the two DB_DATA_VALUE inputs to this
**	    routine may have different lengths, as long as the datatypes are
**	    exactly the same.
**	20-mar-89 (jrb)
**	    Made changes for adc_lenchk interface change.
**	22-mar-91 (jrb)
**	    Put comment markers around "xDEBUG" after #endif's.  This was
**	    causing problems on some compilers.
**	14-apr-10 (smeke01) b123572
**	    Allow comparison between nullable and non-nullable values.
**	16-apr-10 (smeke01) b123577
**	    Allow comparison between char and varchar type values.
*/

# ifdef ADF_BUILD_WITH_PROTOS
DB_STATUS
adc_compare(
ADF_CB              *adf_scb,
DB_DATA_VALUE       *adc_dv1,
DB_DATA_VALUE       *adc_dv2,
i4                  *adc_cmp_result)
# else
DB_STATUS 
adc_compare( adf_scb, adc_dv1, adc_dv2, adc_cmp_result)
ADF_CB              *adf_scb;
DB_DATA_VALUE       *adc_dv1;
DB_DATA_VALUE       *adc_dv2;
i4                  *adc_cmp_result;
# endif
{
    i4			bdt1 = abs((i4) adc_dv1->db_datatype);
    i4			bdt2 = abs((i4) adc_dv2->db_datatype);
    i4			bdtv;

    if (bdt1 != bdt2)
    {
	/*
	** Mostly the functions adc_compare() calls don't want differing datatypes,
	** but there are some that can handle them: adu_varcharcmp() can handle
	** comparisons between varchar & char.
	*/
	if ( !( (bdt1 == DB_CHA_TYPE || bdt1 == DB_VCH_TYPE ) &&
		(bdt2 == DB_CHA_TYPE || bdt2 == DB_VCH_TYPE ) ) )
	    return(adu_error(adf_scb, E_AD3001_DTS_NOT_SAME, 0));
    }

    bdtv = ADI_DT_MAP_MACRO(bdt1);
    
    if (    bdtv <= 0 || bdtv > ADI_MXDTS
	 || Adf_globs->Adi_dtptrs[bdtv] == NULL
       )
	return(adu_error(adf_scb, E_AD2004_BAD_DTID, 0));

#ifdef	xDEBUG
    /*
    **	 The block below checks for error conditions that should not
    ** occur in the production release.  This error would primarily be
    ** caused by memory smashing.
    */
    {
	i4		status;
	DB_STATUS       db_stat = E_DB_OK;

        if (db_stat = adc_lenchk(adf_scb, FALSE, adc_dv1, NULL))
	    return(db_stat);
	else if (db_stat = adc_lenchk(adf_scb, FALSE, adc_dv2, NULL))
	    return(db_stat);
    }
#endif	/* xDEBUG */

    if (adc_dv1->db_datatype > 0 && adc_dv2->db_datatype > 0)		/* both non-nullable */
    {
	return((*Adf_globs->Adi_dtptrs[ADI_DT_MAP_MACRO(bdt1)]->
		    adi_dt_com_vect.adp_compare_addr)
			(adf_scb, adc_dv1, adc_dv2, adc_cmp_result));
    }
    else /* one or both nullable */
    {
	bool		d1_isnull = FALSE;
	bool		d2_isnull = FALSE;

	if (adc_dv1->db_datatype < 0)
	    d1_isnull = ADI_ISNULL_MACRO(adc_dv1);

	if (adc_dv2->db_datatype < 0)
	    d2_isnull = ADI_ISNULL_MACRO(adc_dv2);

	if (!d1_isnull  && !d2_isnull)
	{
	    DB_DATA_VALUE   tmp_dv1;
	    DB_DATA_VALUE   tmp_dv2;

	    STRUCT_ASSIGN_MACRO(*adc_dv1, tmp_dv1);
	    STRUCT_ASSIGN_MACRO(*adc_dv2, tmp_dv2);
	    tmp_dv1.db_datatype = bdt1;
	    tmp_dv2.db_datatype = bdt2;

	    if (adc_dv1->db_datatype < 0)
		tmp_dv1.db_length--;

	    if (adc_dv2->db_datatype < 0)
		tmp_dv2.db_length--;

	    return((*Adf_globs->Adi_dtptrs[ADI_DT_MAP_MACRO(bdt1)]->
			adi_dt_com_vect.adp_compare_addr)
			    (adf_scb, &tmp_dv1, &tmp_dv2, adc_cmp_result));
	}

	if      ( d1_isnull  && !d2_isnull)
	    *adc_cmp_result = 1;
	else if (!d1_isnull  &&  d2_isnull)
	    *adc_cmp_result = -1;
	else if ( d1_isnull  &&  d2_isnull)
	    *adc_cmp_result = 0;

	return(E_DB_OK);
    }
}


/*
** Name: adc_float_compare()	- compare two float datatype values.
**
** Description:
**      This routine compares two float values.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adc_dv1                         Pointer to the 1st data value
**                                      to compare.
**		.db_datatype		Datatype id of 1st data value.
**		.db_length  		Length of 1st data value.
**		.db_data    		Pointer to the actual data for
**		         		1st data value.
**      adc_dv2                         Pointer to the 2nd data value
**                                      to compare.
**		.db_datatype		Datatype id of 2nd data value.
**					(Must be same as
**					adc_dv1->db_datatype.)
**		.db_length  		Length of 2nd data value.
**		.db_data    		Pointer to the actual data for
**		         		2nd data value.
**      adc_cmp_result                  Pointer to the i4  to put the
**				 	comparison result.
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
**     *adc_cmp_result                  Result of comparison.  This is
**                                      guaranteed to be:
**                                          < 0   if 1st < 2nd
**                                          = 0   if 1st = 2nd
**                                          > 0   if 1st > 2nd
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
**	10-apr-86 (ericj)
**	    taken from 4.0 execexp.c and modified for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
*/

DB_STATUS
adc_float_compare(
ADF_CB              *adf_scb,
DB_DATA_VALUE      *adc_dv1,
DB_DATA_VALUE      *adc_dv2,
i4		   *adc_cmp_result)
{
    f4	    sf1, sf2;	    /* floats to coerce the data values into */
    f8	    f1, f2;	    /* double precision floats to coerce data into */

    if (adc_dv1->db_length == 4 || adc_dv2->db_length == 4)
    {
	/* compare operands according to the operand(s) of least precision. */
	if (adc_dv1->db_length == 4)
	    sf1 = *(f4 *) adc_dv1->db_data;
	else
	    sf1 = *(f8 *) adc_dv1->db_data;
	if (adc_dv2->db_length == 4)
	    sf2 = *(f4 *) adc_dv2->db_data;
	else
	    sf2 = *(f8 *) adc_dv2->db_data;
	*adc_cmp_result = (sf1 == sf2 ? 0 : (sf1 < sf2 ? -1 : 1));
    }
    else
    {
	/* both floats are f8s, so do an f8 comparison. */
	f1 = *(f8 *) adc_dv1->db_data;
	f2 = *(f8 *) adc_dv2->db_data;
	*adc_cmp_result = (f1 == f2 ? 0 : (f1 < f2 ? -1 : 1));
    }
    return(E_DB_OK);
}


/*
** Name: adc_int_compare()	- compare two integer datatype values.
**
** Description:
**      This routine compares two integer values.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adc_dv1                         Pointer to the 1st data value
**                                      to compare.
**		.db_datatype		Datatype id of 1st data value.
**		.db_length  		Length of 1st data value.
**		.db_data    		Pointer to the actual data for
**		         		1st data value.
**      adc_dv2                         Pointer to the 2nd data value
**                                      to compare.
**		.db_datatype		Datatype id of 2nd data value.
**					(Must be same as
**					adc_dv1->db_datatype.)
**		.db_length  		Length of 2nd data value.
**		.db_data    		Pointer to the actual data for
**		         		2nd data value.
**      adc_cmp_result                  Pointer to the i4  to put the
**				 	comparison result.
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
**     *adc_cmp_result                  Result of comparison.  This is
**                                      guaranteed to be:
**                                          < 0   if 1st < 2nd
**                                          = 0   if 1st = 2nd
**                                          > 0   if 1st > 2nd
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
**	10-apr-86 (ericj)
**	    taken from 4.0 execexp.c and modified for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	10-Jan-2004 (schka24)
**	    Add i8 support.  A few compiler tests indicate that it's
**	    faster to simply coerce to i8 than it is to try to special-case
**	    i8's vs i4's.
*/

DB_STATUS
adc_int_compare(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *adc_dv1,
DB_DATA_VALUE	    *adc_dv2,
i4		    *adc_cmp_result)
{
    i8		a0,a1;	/* i8s to coerce the data values into */

    switch(adc_dv1->db_length)
    {
      case 1:
	a0 = I1_CHECK_MACRO(*(i1 *)adc_dv1->db_data);
	    break;
      case 2:
	a0 = *(i2 *)adc_dv1->db_data;
	    break;
      case 4:
	a0 = *(i4 *)adc_dv1->db_data;
	    break;
      case 8:
	a0 = *(i8 *)adc_dv1->db_data;
	    break;
    }
    switch(adc_dv2->db_length)
    {
      case 1:
	a1 = I1_CHECK_MACRO(*(i1 *)adc_dv2->db_data);
	    break;
      case 2:
	a1 = *(i2 *)adc_dv2->db_data;
	    break;
      case 4:
	a1 = *(i4 *)adc_dv2->db_data;
	    break;
      case 8:
	a1 = *(i8 *)adc_dv2->db_data;
	    break;
    }
    *adc_cmp_result = (a0 == a1 ? 0 : (a0 < a1 ? -1 : 1));

    return(E_DB_OK);
}


/*
** Name: adc_bool_compare - compare two boolean datatype values.
**
** Description:
**      This routine compares two boolean values.
**
** Inputs:
**      adf_scb           Pointer to an ADF session control block.
**      dv1               Pointer to the 1st data value
**      dv2               Pointer to the 2nd data value
**
** Outputs:
**      adf_scb           Pointer to an ADF session control block.
**      cmp_result        Result of comparison.  This is
**                        guaranteed to be:
**                            < 0   if 1st < 2nd
**                            = 0   if 1st = 2nd
**                            > 0   if 1st > 2nd
**
**      Returns:
**          E_DB_OK       Operation succeeded.
**
** History:
**      05-oct-2009 (joea)
**          Created.
*/
DB_STATUS
adc_bool_compare(
ADF_CB *adf_scb,
DB_DATA_VALUE *dv1,
DB_DATA_VALUE *dv2,
i4 *cmp_result)
{
    *cmp_result = ((DB_ANYTYPE *)dv1->db_data)->db_booltype
                  - ((DB_ANYTYPE *)dv2->db_data)->db_booltype;
    return E_DB_OK;
}


/*
** Name: adc_dec_compare()	- compare two integer datatype values.
**
** Description:
**      This routine compares two decimal values.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control blk.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adc_dv1                         Pointer to the 1st data value
**                                      to compare.
**		.db_datatype		Datatype id of 1st data value.
**		.db_prec		Precision/Scale of 1st data value.
**		.db_length  		Length of 1st data value.
**		.db_data    		Pointer to the actual data for
**		         		1st data value.
**      adc_dv2                         Pointer to the 2nd data value
**                                      to compare.
**		.db_datatype		Datatype id of 2nd data value.
**					(Must be same as
**					adc_dv1->db_datatype.)
**		.db_prec		Precision/Scale of 2nd data value.
**		.db_length  		Length of 2nd data value.
**		.db_data    		Pointer to the actual data for
**		         		2nd data value.
**      adc_cmp_result                  Pointer to the i4  to put the
**				 	comparison result.
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
**     *adc_cmp_result                  Result of comparison.  This is
**                                      guaranteed to be:
**                                          < 0   if 1st < 2nd
**                                          = 0   if 1st = 2nd
**                                          > 0   if 1st > 2nd
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
**	05-jul-89 (jrb)
**	    Created.
*/

DB_STATUS
adc_dec_compare(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *adc_dv1,
DB_DATA_VALUE	    *adc_dv2,
i4		    *adc_cmp_result)
{
    *adc_cmp_result = MHpkcmp(	adc_dv1->db_data,
				(i4)DB_P_DECODE_MACRO(adc_dv1->db_prec),
				(i4)DB_S_DECODE_MACRO(adc_dv1->db_prec),
				adc_dv2->db_data,
				(i4)DB_P_DECODE_MACRO(adc_dv2->db_prec),
				(i4)DB_S_DECODE_MACRO(adc_dv2->db_prec));

    return(E_DB_OK);
}

/*
** Name: adc_bit_compare()	- compare two bit string datatype values.
**
** Description:
**      This routine compares two bit string values.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control blk.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adc_dv1                         Pointer to the 1st data value
**                                      to compare.
**		.db_datatype		Datatype id of 1st data value.
**		.db_prec		Precision/Scale of 1st data value.
**		.db_length  		Length of 1st data value.
**		.db_data    		Pointer to the actual data for
**		         		1st data value.
**      adc_dv2                         Pointer to the 2nd data value
**                                      to compare.
**		.db_datatype		Datatype id of 2nd data value.
**					(Must be same as
**					adc_dv1->db_datatype.)
**		.db_prec		Precision/Scale of 2nd data value.
**		.db_length  		Length of 2nd data value.
**		.db_data    		Pointer to the actual data for
**		         		2nd data value.
**      adc_cmp_result                  Pointer to the i4  to put the
**				 	comparison result.
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
**     *adc_cmp_result                  Result of comparison.  This is
**                                      guaranteed to be:
**                                          < 0   if 1st < 2nd
**                                          = 0   if 1st = 2nd
**                                          > 0   if 1st > 2nd
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
**	21-Sep-1992 (fred)
**	    Created.
**	22-Oct-1992 (fred)
**	    Modified to ANSI data representation.  This packs all bits
**	    from the high end down.  Thus, on a 8 bit byte machine, the
**	    value for a 3 bit value is held in the bits represented by the
**	    mask 0xE0.
*/
DB_STATUS
adc_bit_compare(
ADF_CB              *adf_scb,
DB_DATA_VALUE       *adc_dv1,
DB_DATA_VALUE       *adc_dv2,
i4                  *adc_cmp_result)
{
    DB_BIT_STRING	*bs1;
    DB_BIT_STRING       *bs2;
    u_char		*b1;
    u_char		*b2;
    u_i4		len1;
    u_i4		len2;
    u_i4                valid_length;
    u_i4                cmp_bytes;
    u_i4		i;
    u_i4                rem_bits;
    u_char		rem1, rem2;
    i4			comp = 0;
    i4			equal_length = 0;

    if (adc_dv1->db_datatype == DB_VBIT_TYPE)
    {
	bs1 = (DB_BIT_STRING *) adc_dv1->db_data;
	I4ASSIGN_MACRO(bs1->db_b_count, len1);
	b1 = (u_char *) bs1->db_b_bits;
    }
    else
    {
	len1 = ((adc_dv1->db_length - 1) * BITSPERBYTE) + adc_dv1->db_prec;
	b1 = (u_char *) adc_dv1->db_data;
    }

    if (adc_dv2->db_datatype == DB_VBIT_TYPE)
    {
	bs2 = (DB_BIT_STRING *) adc_dv2->db_data;
	I4ASSIGN_MACRO(bs2->db_b_count, len2);
	b2 = (u_char *) bs2->db_b_bits;
    }
    else
    {
	len2 = ((adc_dv2->db_length - 1) * BITSPERBYTE) + adc_dv2->db_prec;
	b2 = (u_char *) adc_dv2->db_data;
    }

    if (len1 < len2)
    {
	valid_length = len1;
    }
    else if (len1 > len2)
    {
	valid_length = len2;
    }
    else
    {
	valid_length = len1;
	equal_length = 1;
    }

    cmp_bytes = valid_length / BITSPERBYTE;
    rem_bits = valid_length % BITSPERBYTE;

    for (i = 0; !comp && (i < cmp_bytes); i++)
	comp = *b1++ - *b2++;


    if ((comp == 0) && (rem_bits != 0))
    {
	/*
	** Here, we need to figure out how many bits to compare.
	** So far, cmp_bytes * BITSPERBYTE bits have been compared.
	** So far, they are all the same.
	** ANSI BIT semantics state that all the comparison is done
	** only to the length of the shorter string.  This has been figured
	** out above via the rem_bits calculation.  Thus, we want to look
	** at rem_bits bits in this byte.
	** 
	** The bits are all packed in at the top.  Therefore, we need a mask
	** with the high-order rem_bits bits set.  This is the complement of
	** all bits on shifted left by rem_bits, or ~(~0 >> rem_bits).
	*/

	rem1 = *b1 & ~((u_char) ~0 >> rem_bits);
	rem2 = *b2 & ~((u_char) ~0 >> rem_bits);

	comp = rem1 - rem2;
    }

    if (comp == 0)
    {
	comp = len1 - len2;
    }
    *adc_cmp_result = comp;

    return(E_DB_OK);
}

/*
** Name: adc_byte_compare()	- compare two byte string datatype values.
**
** Description:
**      This routine compares two byte string values.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control blk.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adc_dv1                         Pointer to the 1st data value
**                                      to compare.
**		.db_datatype		Datatype id of 1st data value.
**		.db_prec		Precision/Scale of 1st data value.
**		.db_length  		Length of 1st data value.
**		.db_data    		Pointer to the actual data for
**		         		1st data value.
**      adc_dv2                         Pointer to the 2nd data value
**                                      to compare.
**		.db_datatype		Datatype id of 2nd data value.
**					(Must be same as
**					adc_dv1->db_datatype.)
**		.db_prec		Precision/Scale of 2nd data value.
**		.db_length  		Length of 2nd data value.
**		.db_data    		Pointer to the actual data for
**		         		2nd data value.
**      adc_cmp_result                  Pointer to the i4  to put the
**				 	comparison result.
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
**     *adc_cmp_result                  Result of comparison.  This is
**                                      guaranteed to be:
**                                          < 0   if 1st < 2nd
**                                          = 0   if 1st = 2nd
**                                          > 0   if 1st > 2nd
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
**	05-Apr-1993 (fred)
**	    Created.
**	14-nov-01 (inkdo01)
**	    Fix odd treatment of compares with differing lengths. Should 
**	    simply confirm longer value is 0's to its end.
*/
DB_STATUS
adc_byte_compare(
ADF_CB              *adf_scb,
DB_DATA_VALUE       *adc_dv1,
DB_DATA_VALUE       *adc_dv2,
i4                  *adc_cmp_result)
{
    u_i2            len1;
    u_i2            len2;
    u_i2	    difflen;
    u_char          *bp1;
    u_char          *bp2;
    u_char	    *bEnd;
    DB_TEXT_STRING  *text;
    u_i2            cmp_length;
    i4              cmp_result;

    if (adc_dv1->db_datatype == DB_VBYTE_TYPE)
    {
	text = (DB_TEXT_STRING *) adc_dv1->db_data;
	I2ASSIGN_MACRO(text->db_t_count, len1);
	bp1 = (u_char *) text->db_t_text;
    }
    else
    {
	len1 = adc_dv1->db_length;
	bp1 = (u_char *) adc_dv1->db_data;
    }
    
    if (adc_dv2->db_datatype == DB_VBYTE_TYPE)
    {
	text = (DB_TEXT_STRING *) adc_dv2->db_data;
	I2ASSIGN_MACRO(text->db_t_count, len2);
	bp2 = (u_char *) text->db_t_text;
    }
    else
    {
	len2 = adc_dv2->db_length;
	bp2 = (u_char *) adc_dv2->db_data;
    }
    
    cmp_length = min(len1, len2);
    cmp_result = MEcmp((PTR) bp1, (PTR) bp2, cmp_length);
    if (cmp_result || len1 == len2)
	*adc_cmp_result = cmp_result;
    else
    {
	/* Equal so far, but lengths differ. Assure the longer
	** is padded out with zeros. */
	*adc_cmp_result = 0;
	if (len1 > len2)
	{
	    difflen = len1 - len2;
	    bEnd = bp1 + len1;
	    cmp_result = len2+1;
	}
	else
	{
	    difflen = len2 - len1;
	    bEnd = bp2 + len2;
	    cmp_result = -(len1 +1);
	}
	while (difflen-- && bEnd--)
	    if (*bEnd != 0)
	    {
		*adc_cmp_result = cmp_result;
		break;
	    }
    }	/* end of "=" but differing lengths logic */

    return(E_DB_OK);
}
