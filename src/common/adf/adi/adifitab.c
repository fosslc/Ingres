/*
** Copyright (c) 1986 - 1995, Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>

/**
**
**  Name: ADIFITAB.C - Find all function instances for a given op id.
**
**  Description:
**      This file contains all of the routines necessary to
**      perform the external ADF call "adi_fitab()" which
**      returns a pointer to a table of function instance full
**      descriptions.
**
**      This file defines:
**
**          adi_fitab() - Find all function instances for a given op id.
**
**
**  History:    $Log-for RCS$
**      25-feb-1986 (thurston)
**          Initial creation.
**	27-jun-86 (thurston)
**	    Upgraded the adi_fitab() routine to use the "now ready"
**	    operation table.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	11-sep-86 (thurston)
**	    Changed adi_operations to Adi_operations in adi_fitab().
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	06-mar-89 (jrb)
**	    Changed adi_optabfi from ptr to adi_fi_desc to index into
**	    fi table for adf startup performance improvements.
**	13-Mar-89 (fred)
**	    Changed aforementioned reference to Adi_fis to go thru Adf_globs.
**	    This change necessitated for UDADT support -- one doesn't know in
**	    advance where the function instance table is...
**      12-dec-1992 (stevet)
**          Added function prototype.
**      12-may-1995 (shero03)
**          Added adi_function for Rtree.
**/


/*{
** Name: adi_fitab()      - Find all function instances for a given op id.
**
** Description:
**      This function is the external ADF call "adi_fitab()" which
**      returns a pointer to a table of function instance full
**      descriptions.
**
**      This table will hold all function instances that use the
**      supplied operator id.  This table will be an array of
**      ADI_FI_DESC structures.  Each ADI_FI_DESC contains the
**      function instance id,  the function instance id of the complement
**      function instance (for the comparisons), the type of function instance,
**      the operator id of the function, the method for calculating the length
**      of the result, the length to make the DB_DATA_VALUE used for work space
**      by the aggregates (for the aggregates), whether the aggregate f.i. is
**	prime or not (for the aggregates), the number of arguments (or input
**	operands), and the datatype ids for the result and all arguments.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adi_oid                         The operator id.
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
**      adi_ftab                        Ptr to the function instance table of
**					all function instance full descriptions
**					that use the given operator.
**	    .adi_lntab			Number of ADI_FI_DESCs in the table
**	    .adi_tab_fi			Pointer to array of ADI_FI_DESCs
**					that use the given operator.
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
**          E_AD2002_BAD_OPID           Operator id unknown to ADF.
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
**	27-jun-86 (thurston)
**	    Upgraded this routine to use the "now ready" operation table.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	11-sep-86 (thurston)
**	    Changed adi_operations to Adi_operations.
**	06-mar-89 (jrb)
**	    Changed adi_optabfi from ptr to adi_fi_desc to index into
**	    fi table for adf startup performance improvements.
**	13-Mar-89 (fred)
**	    Changed aforementioned reference to Adi_fis to go thru Adf_globs.
**	    This change necessitated for UDADT support -- one doesn't know in
**	    advance where the function instance table is...
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**      09-nov-2010 (gupsh01) SIR 124685
**          Protype cleanup.
*/

DB_STATUS
adi_fitab(
ADF_CB             *adf_scb,
ADI_OP_ID          adi_oid,
ADI_FI_TAB         *adi_ftab)
{
    ADI_OPRATION	*op = Adf_globs->Adi_operations;


    adi_ftab->adi_tab_fi = (ADI_FI_DESC *) NULL;  /* return NULL if not found */
    adi_ftab->adi_lntab = 0;

    while (op->adi_opid != ADI_NOOP)
    {
	if (op->adi_opid == adi_oid)
	{
	    adi_ftab->adi_tab_fi = &Adf_globs->Adi_fis[op->adi_optabfidx];
	    adi_ftab->adi_lntab = op->adi_opcntfi;
	    return(E_DB_OK);
	}

	op++;
    }

    return(adu_error(adf_scb, E_AD2002_BAD_OPID, 0));
}


/*{
** Name: adi_function()   - Find the function address for a given func id.
**
** Description:
**      This function is the external ADF call "adi_function()" which
**      returns a pointer to a function that corresponds to the
**      function instance id.
**
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adi_fidesc                      Ptr to the function instance
**					descriptor
**	    .adi_tab_fi			Index in the function instance
**					lookup table that corresponds to this
**					function instance.
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
**      adi_func                        Ptr to the ADF routine that processes
**                                      the corresponding function instance.
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
**          E_AD2010_BAD_FIID           Function Instance id unknown to ADF.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      12-may-95 (shero03)
**          Initial creation.
**      09-nov-2010 (gupsh01) SIR 124685
**          Protype cleanup.
*/

DB_STATUS
adi_function(
ADF_CB             *adf_scb,
ADI_FI_DESC        *adi_fidesc,
DB_STATUS          (**adi_func)() )
{

    ADI_FI_LOOKUP       *func;


    /* Get the function instance description for this instruction */
    /* ---------------------------------------------------------- */


    if ( (adi_fidesc == NULL) ||
         (adi_fidesc->adi_finstid < 0) ||
	 (adi_fidesc->adi_finstid > Adf_globs->Adi_fi_biggest) )
        return(adu_error(adf_scb, E_AD2010_BAD_FIID, 0));

    func = &Adf_globs->Adi_fi_lkup[ADI_LK_MAP_MACRO(adi_fidesc->adi_finstid)];

    if (func->adi_fi == NULL)
        return(adu_error(adf_scb, E_AD2010_BAD_FIID, 0));
    else
    {
    	*adi_func = func->adi_func;
        return(E_DB_OK);
    }

}
