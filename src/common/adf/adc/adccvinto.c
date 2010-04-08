/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <adp.h>
#include    <ulf.h>
#include    <adfint.h>

/**
**
**  Name: ADCCVINTO.C - Convert data value into the given datatype.
**
**  Description:
**      This file contains all of the routines necessary to
**      perform the  external ADF call "adc_cvinto()" which
**      will convert a data value into a given datatype/precision/length
**      provided ADF knows of such a coercion.
**
**      This file defines:
**
**          adc_cvinto() - Convert data value into the given datatype.
**				
**
**
**  History:
**      26-feb-86 (thurston)
**          Initial creation.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	20-feb-87 (thurston)
**	    Removed the adc_1cvinto_rti() routine since it is no longer needed.
**	    In fact, this routine does not need to be a "common datatype"
**	    routine at all, since the conversion process is handled in a
**	    datatype independent fashion.  This is done by using the ADF routine
**	    adi_ficoerce() to find the function instance of the requested
**	    coercion, and then using ADF's function instance lookup table to
**	    call the appropriate low level coercion routine.
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	27-Jan-89 (fred)
**	    Altered reference to Adi_fi_lkup to go thru Adf_globs
**	20-mar-89 (jrb)
**	    Made changes for adc_lenchk interface change.
**	28-apr-89 (fred)
**	    Altered to use ADI_DT_MAP_MACRO().
**      22-dec-1992 (stevet)
**          Added function prototype.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**      07-apr-1994 (stevet)
**          Added workspace handling for FI that require temporary
**          workspace to handle BLOB.  The adc_cvinto() will be called
**          to handle coercion between DBP parameters which can 
**          be BLOB DT.  Local space is used since adccvinto() is 
**          a very low level call and the processing is expected to
**          to be short.  Only up to 4K of workspace is supported
**          since that's the current maximum.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**/


/*{
** Name: adc_cvinto() - Convert data value into the given datatype.
**
** Description:
**      This function is the external ADF call "adc_cvinto()" which
**      will convert a data value into a given datatype and length
**      provided ADF knows of such a conversion.
**
**	It is guaranteed that for all datatypes, a conversion into the same
**	datatype will exist.  This guarantee allows this routine to move a data
**	value with one length, into a data value of the same datatype but with
**	a different length.  Or for decimal, a value with one precision/scale
**	can be coerced into a value with a different precision/scale.  This
**	will be needed when preparing a data value for insertion into a tuple
**	where the destination datatype is correct, but the length of the data
**	value is different from the length of the attribute in the relation.
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
**		.db_prec		Precision/Scale of data value
**					to be created.
**		.db_length  		Length of data value to be
**					converted.
**		.db_data    		Pointer to the actual data for
**		         		data value to be converted.
**      adc_dvinto                      Pointer to the data value to be
**					created.
**		.db_datatype		Datatype id of data value being
**					created.
**		.db_prec		Precision/Scale of data value
**					being created.
**		.db_length  		Length of data value being
**					created.
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
**      adc_dvinto                      The resulting data value.
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
**          E_AD2004_BAD_DTID           Datatype id unknown to ADF.
**          E_AD2009_NOCOERCION		No such conversion is known
**                                      to ADF.
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
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      26-feb-86 (thurston)
**          Initial creation.
**	11-jun-86 (ericj)
**	    Initial implementation and updated the spec.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	20-feb-87 (thurston)
**	    Removed the adc_1cvinto_rti() routine since it is no longer needed.
**	    In fact, this routine does not need to be a "common datatype"
**	    routine at all, since the conversion process is handled in a
**	    datatype independent fashion.  This is done by using the ADF routine
**	    adi_ficoerce() to find the function instance of the requested
**	    coercion, and then using ADF's function instance lookup table to
**	    call the appropriate low level coercion routine.
**	11-jul-89 (jrb)
**	    Decimal support (only changed comments; no code changes).
**      22-mar-91 (jrb)
**          Put comment markers around "xDEBUG" after #endif's.  This was
**          causing problems on some compilers.
**      07-apr-1994 (stevet)
**          Added workspace handling for FI that require temporary
**          workspace to handle BLOB.  The adc_cvinto() will be called
**          to handle coercion between DBP parameters which can 
**          be BLOB DT.  Local space is used since adccvinto() is 
**          a very low level call and the processing is expected to
**          to be short.  Only up to 4K of workspace is supported
**          since that's the current maximum.
*/

DB_STATUS
adc_cvinto(
ADF_CB             *adf_scb,
DB_DATA_VALUE      *adc_dvfrom,
DB_DATA_VALUE      *adc_dvinto)
{
    DB_STATUS       	db_stat;
    ADI_FI_ID	    	cnvt_fi_id;
    ADI_FI_DESC         *cnvt_fi_desc;
    DB_DATA_VALUE       tmp_dvfrom;
    DB_DATA_VALUE       tmp_dvinto;
    DB_DATA_VALUE       tmp_dvwork;
    i4			f_bdt = abs((i4) adc_dvfrom->db_datatype);
    i4			i_bdt = abs((i4) adc_dvinto->db_datatype);
    i4			f_bdtv;
    i4			i_bdtv;


    /* Check the consistency of the input arguments */

    for (;;)
    {
        /* Check the DB_DATA_VALUEs datatype for correctness */
	f_bdtv = ADI_DT_MAP_MACRO(f_bdt);
	i_bdtv = ADI_DT_MAP_MACRO(i_bdt);
        if (    f_bdtv <= 0  ||  f_bdtv > ADI_MXDTS
	     || i_bdtv <= 0  ||  i_bdtv > ADI_MXDTS
	     || Adf_globs->Adi_dtptrs[f_bdtv] == NULL
	     || Adf_globs->Adi_dtptrs[i_bdtv] == NULL
	    )
	{
	    db_stat = adu_error(adf_scb, E_AD2004_BAD_DTID, 0);
	    break;
	}

#ifdef xDEBUG
        /* Check the validity of the DB_DATA_VALUE lengths passed in. */
	if (   (db_stat = adc_lenchk(adf_scb, FALSE, adc_dvfrom, NULL))
	    || (db_stat = adc_lenchk(adf_scb, FALSE, adc_dvinto, NULL))
	   )
	    break;
#endif /* xDEBUG */	


        /* Get the function instance id associated with this coercion */
        if (db_stat = adi_ficoerce(adf_scb, adc_dvfrom->db_datatype,
			           adc_dvinto->db_datatype, &cnvt_fi_id)
           )
	    break;

	/* Check for NULL into non-nullable, and NULL -> NULL */
	if (ADI_ISNULL_MACRO(adc_dvfrom))	/* from value is NULL */
	{
	    if (adc_dvinto->db_datatype > 0)	/* into is non-nullable */
	    {
		db_stat = adu_error(adf_scb, E_AD1012_NULL_TO_NONNULL, 0);
	    }
	    else				/* into is nullable */
	    {
		ADF_SETNULL_MACRO(adc_dvinto);
		db_stat = E_DB_OK;
	    }
	    break;
	}


	/* Set up temp data values, assuring them to be base datatypes */
	STRUCT_ASSIGN_MACRO(*adc_dvfrom, tmp_dvfrom);
	STRUCT_ASSIGN_MACRO(*adc_dvinto, tmp_dvinto);
	if (adc_dvfrom->db_datatype < 0)	/* from is nullable */
	{
	    tmp_dvfrom.db_datatype = f_bdt;
	    tmp_dvfrom.db_length--;
	}
	if (adc_dvinto->db_datatype < 0)	/* into is nullable */
	{
	    ADF_CLRNULL_MACRO(adc_dvinto);
	    tmp_dvinto.db_datatype = i_bdt;
	    tmp_dvinto.db_length--;
	}

	if ((db_stat = adi_fidesc(adf_scb, cnvt_fi_id, &cnvt_fi_desc)) 
	    != E_DB_OK)
	    break;

	if (cnvt_fi_desc->adi_fiflags & ADI_F4_WORKSPACE)
	{
	    /* 
	    ** Need to create workspace.  Only support up to 4K 
	    ** workspace for the moment.  
	    */
	    if (cnvt_fi_desc->adi_agwsdv_len <= 2048)
	    {
		/* Declare as ALIGN_RESTRICT to ensure alignment. */
		ALIGN_RESTRICT  buff[2048/sizeof(ALIGN_RESTRICT)];

		tmp_dvwork.db_data = (PTR) &buff[0];
		db_stat =
		    (*Adf_globs->Adi_fi_lkup[ADI_LK_MAP_MACRO(cnvt_fi_id)].adi_func)
			(adf_scb, &tmp_dvfrom, &tmp_dvwork, &tmp_dvinto);

	    }
	    else if (cnvt_fi_desc->adi_agwsdv_len <= 4096)
	    {
		ALIGN_RESTRICT  buff[4096/sizeof(ALIGN_RESTRICT)];

		tmp_dvwork.db_data = (PTR) &buff[0];
		db_stat =
		    (*Adf_globs->Adi_fi_lkup[ADI_LK_MAP_MACRO(cnvt_fi_id)].adi_func)
			(adf_scb, &tmp_dvfrom, &tmp_dvwork, &tmp_dvinto);
	    }
	    else
	    {
		db_stat = 
		    adu_error(adf_scb, E_AD7012_ADP_WORKSPACE_TOOLONG, 0);
		break;		
	    }

	}
	else
	{
        /* Call the routine thru the ADF internal func instance lookup table */
	    db_stat =
	       (*Adf_globs->Adi_fi_lkup[ADI_LK_MAP_MACRO(cnvt_fi_id)].adi_func)
		    (adf_scb, &tmp_dvfrom, &tmp_dvinto);
	}
	break;
    }       /* end of for statement */

    return(db_stat);
}
