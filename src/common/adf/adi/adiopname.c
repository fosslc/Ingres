/*
** Copyright (c) 1986, Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <st.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
/* [@#include@]... */

/**
**
**  Name: ADIOPNAME.C - Get operator name from id.
**
**  Description:
**      This file contains all of the routines necessary to
**      perform the external ADF call "adi_opname()" which
**      returns the operator name corresponding to the supplied
**      operator id.
**
**      This file defines:
**
**          adi_opname() - Get operator name from id.
**
**
**  History:    $Log-for RCS$
**      24-feb-86 (thurston)
**          Initial creation.
**      02-apr-86 (thurston)
**	    Added the E_AD9999_INTERNAL_ERROR return.
**	01-jul-86 (thurston)
**	    Routine adi_opname() rewritten to acount for new version of the
**          operations table. Also, because of this, the E_AD9999_INTERNAL_ERROR
**          return will never happen. 
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	11-sep-86 (thurston)
**	    Changed adi_operations to Adi_operations in adi_opname().
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**      22-dec-1992 (stevet)
**          Added function prototype.
**/


/*{
** Name: adi_opname()     - Get operator name from id.
**
** Description:
**      This function is the external ADF call "adi_opname()" which
**      returns the operator name corresponding to the supplied
**      operator id.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adi_oid                         The operator id to find the
**                                      operator name for.
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
**      adi_oname                       The operator name corresponding
**                                      to the supplied operator id.
**                                      If the supplied operator id
**                                      is not recognized by ADF, this
**                                      will be returned as a null
**                                      string.
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
**          E_AD9999_INTERNAL_ERROR     Operations table is mangled.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      24-feb-86 (thurston)
**          Initial creation.
**      02-apr-86 (thurston)
**	    Initial coding.  Added the E_AD9999_INTERNAL_ERROR return.
**	01-jul-86 (thurston)
**	    Rewritten to acount for new version of the operations table.
**	    Also, because of this, the E_AD9999_INTERNAL_ERROR return will
**	    never happen.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	11-sep-86 (thurston)
**	    Changed adi_operations to Adi_operations.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
*/

# ifdef ADF_BUILD_WITH_PROTOS
DB_STATUS
adi_opname(
ADF_CB             *adf_scb,
ADI_OP_ID          adi_oid,
ADI_OP_NAME        *adi_oname)
# else
DB_STATUS
adi_opname( adf_scb, adi_oid, adi_oname)
ADF_CB             *adf_scb;
ADI_OP_ID          adi_oid;
ADI_OP_NAME        *adi_oname;
# endif
{
    ADI_OPRATION        *op;

    for(op = &Adf_globs->Adi_operations[0]; op->adi_opid != ADI_NOOP; op++)
    {
	if (op->adi_opid == adi_oid)
	{
	    STcopy(op->adi_opname.adi_opname, adi_oname->adi_opname);
	    return(E_DB_OK);
	}
    }

    STcopy("", adi_oname->adi_opname);	/* return empty string */
    return(adu_error(adf_scb, E_AD2002_BAD_OPID, 0));
}
