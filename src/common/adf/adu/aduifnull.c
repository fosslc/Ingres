/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduint.h>

/**
**
**  Name: ADUIFNULL.C - The INGRES "ifnull" function.
**
**  Description:
**      If the first param is NULL, return the second param. Else,
**	return the first param. 
**
**          adu_ifnull -  The INGRES "ifnull" function.
**
**  Function prototypes defined in ADUINT.H file.
**
**  History:    $Log-for RCS$
**      07-jan-87 (daved)
**          written
**	24-feb-87 (thurston)
**	    Changed adu_ifnull() to use ADI_ISNULL_MACRO() instead of calling
**          adc_isnull(). Also added an xDEBUG check to prevent memory stomping.
**	16-apr-87 (thurston)
**	    Added code to adu_ifnull() to handle cases where the datatypes of
**	    the inputs are the same, but the lengths are different.
**      04-jan-1993 (stevet)
**          Added function prototypes.
**	20-oct-1995 (pchang)
**	    Changed adu_ifnull() to also take into account the precisions of
**	    the input data values when determining if a conversion is in order 
**	    - decimal data values often have the same length but not the same 
**	    precision and it would be wrong to simply do an MEcopy (B71009). 
**	2-sep-1998 (rigka01)
**	    Cross integrate change 433119 (bug 82496) and change 434909
**	    (bug 89868) from 1.2 to 2.0
**	    Bug 82496: take into account the prcisions of the input data
**	    values when determining if a conversion is in order - decimal
**	    data values often have the same length but not the same
**	    precision and it would be wrong to simpley do an MEcopy(B71009)
**	    Bug 89868: change for bug 82496 allows ME copy to take place
**	    when dv->db_datatype is not nullable.  The fix is to add this
**	    condition in order to prevent bug 89868.
**	07-Dec-2009 (Drewsturgiss)
**		Addition of the adu_nvl2 function
**/


/*{
** Name: adu_ifnull() - The INGRES "ifnull" function.
**
** Description:
**      Check first parameter. If not null, assign it to result. If it is null,
**  assign second param to result. 
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1				Ptr to DB_DATA_VALUE for first param.
**	    .db_datatype		Datatype.
**	    .db_prec			Precision/Scale
**	    .db_length			Length.
**	    .db_data			Ptr to the data of first param.
**      dv2				Ptr to DB_DATA_VALUE for second param.
**	    .db_datatype		Datatype.
**	    .db_prec			Precision/Scale
**	    .db_length			Length.
**	    .db_data			Ptr to the data of second param.
**      rdv				Pointer DB_DATA_VALUE for the result.
**	    .db_datatype		Datatype.
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
**      rdv				Pointer DB_DATA_VALUE for the result.
**	    .db_data			Result will be placed here.
**
**	Returns:
**	    E_DB_OK
**
**
** Side Effects:
**	    none
**
** History:
**      07-jan-87 (daved)
**          written
**	24-feb-87 (thurston)
**	    Changed to use ADI_ISNULL_MACRO() instead of calling adc_isnull().
**	    Also added an xDEBUG check to prevent memory stomping.
**	16-apr-87 (thurston)
**	    Added code to handle cases where the datatypes of the inputs are the
**	    same, but the lengths are different.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	20-oct-1995 (pchang)
**	    Changed to also take into account the precisions of the input data 
**	    values when determining if a conversion is in order - decimal data 
**	    values often have the same length but not the same precision and 
**	    it would be wrong to simply do an MEcopy (B71009). 
**	2-sep-1998 (rigka01)
**	    Cross integrate change 433119 (bug 82496) and change 434909
**	    (bug 89868) from 1.2 to 2.0
**	    Bug 82496: take into account the precisions of the input data
**	    values when determining if a conversion is in order - decimal
**	    data values often have the same length but not the same
**	    precision and it would be wrong to simpley do an MEcopy(B71009)
**	    Bug 89868: change for bug 82496 allows ME copy to take place
**	    when dv->db_datatype is not nullable.  The fix is to add this
**	    condition in order to prevent bug 89868.
**	25-Jan-2001 (thaju02 for andbr08)
**	    Cleared null byte AFTER copying dv1 or dv2 into dv. (B96079)
*/

DB_STATUS
adu_ifnull(
ADF_CB              *adf_scb,
DB_DATA_VALUE       *dv1,
DB_DATA_VALUE       *dv2,
DB_DATA_VALUE       *rdv)
{
    DB_DATA_VALUE       *dv;
    DB_STATUS		db_stat;


    dv = (ADI_ISNULL_MACRO(dv1) ? dv2 : dv1);

    /* clear null byte in result afer copying dv1 incase dv1 equals rdv */
    if (rdv->db_datatype < 0)
	ADF_CLRNULL_MACRO(rdv);

    /* b89868 - Fix for 82496 assumes dv is nullable */
    /* Check this to prevent inappropriate MEcopy    */  
    if ((dv->db_datatype == rdv->db_datatype &&
         dv->db_length == rdv->db_length &&
	 dv->db_prec == rdv->db_prec) ||
	(dv->db_datatype == -rdv->db_datatype &&
	 dv->db_datatype < 0 &&                 /* nullable */
	 dv->db_length == rdv->db_length+1 &&
         dv->db_prec == rdv->db_prec))
    {
	/* b89868 - Copy rdv->db_length as the result maybe non-nullable */
	/* and will therefore be smaller than dv->db_length by 1         */
	/* Prior conditions ensure that the lengths are either equal     */
	/* or dv is nullable and rdv is non-nullable                     */  
	MEcopy((PTR) dv->db_data, rdv->db_length, (PTR) rdv->db_data);
	db_stat = E_DB_OK;
    }
    else
    {
	db_stat = adc_cvinto(adf_scb, dv, rdv);	/* Needed to adjust lengths */
    }

    return (db_stat);
}



/*{
** Name: adu_nvl2() - The INGRES "nvl2" function.
**
** Description:
**      Check first parameter. If not null, assign the second param to result. If it is null,
**  assign third param to result. 
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1				Ptr to DB_DATA_VALUE for first param.
**	    .db_datatype		Datatype.
**	    .db_prec			Precision/Scale
**	    .db_length			Length.
**	    .db_data			Ptr to the data of first param.
**      dv2				Ptr to DB_DATA_VALUE for second param.
**	    .db_datatype		Datatype.
**	    .db_prec			Precision/Scale
**	    .db_length			Length.
**	    .db_data			Ptr to the data of second param.
**      rdv				Pointer DB_DATA_VALUE for the result.
**	    .db_datatype		Datatype.
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
**      rdv				Pointer DB_DATA_VALUE for the result.
**	    .db_data			Result will be placed here.
**
**	Returns:
**	    E_DB_OK
**
**
** Side Effects:
**	    none
** History:
**      07-Dec-2009 (drewsturgiss)
**          written
*/

DB_STATUS
adu_nvl2(
ADF_CB              *adf_scb,
DB_DATA_VALUE       *dv1,
DB_DATA_VALUE       *dv2,
DB_DATA_VALUE       *dv3,
DB_DATA_VALUE       *rdv)
{
	DB_DATA_VALUE       *dv;
	DB_STATUS		db_stat;

	dv = (ADI_ISNULL_MACRO(dv1) ? dv3 : dv2);

	/* clear null byte in result afer copying dv1 incase dv1 equals rdv */
	if (rdv->db_datatype < 0)
		ADF_CLRNULL_MACRO(rdv);

	/* b89868 - Fix for 82496 assumes dv is nullable */
	/* Check this to prevent inappropriate MEcopy    */  
	if ((dv->db_datatype == rdv->db_datatype &&
	dv->db_length == rdv->db_length &&
	dv->db_prec == rdv->db_prec) ||
	(dv->db_datatype == -rdv->db_datatype &&
	dv->db_datatype < 0 &&                 /* nullable */
	dv->db_length == rdv->db_length+1 &&
	dv->db_prec == rdv->db_prec))
	{
		/* b89868 - Copy rdv->db_length as the result maybe non-nullable */
		/* and will therefore be smaller than dv->db_length by 1         */
		/* Prior conditions ensure that the lengths are either equal     */
		/* or dv is nullable and rdv is non-nullable                     */  
		MEcopy((PTR) dv->db_data, rdv->db_length, (PTR) rdv->db_data);
		db_stat = E_DB_OK;
	}
	else
	{
		db_stat = adc_cvinto(adf_scb, dv, rdv);	/* Needed to adjust lengths */
	}

	return (db_stat);
}
