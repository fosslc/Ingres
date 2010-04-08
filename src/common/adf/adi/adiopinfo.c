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

/**
**
**  Name: ADIOPINFO.C - Get info about an INGRES operation ID.
**
**  Description:
**      This file contains all of the routines necessary to
**      perform the external ADF call "adi_op_info()" which
**      returns an ADI_OPINFO structure containing several
**	pieces of information about the supplied op ID. 
**
**      This file defines:
**
**          adi_op_info() - Get info about an INGRES operation ID.
**
**
**  History:    $Log-for RCS$
**      16-may-88 (thurston)
**          Initial creation.
**      22-mar-91 (jrb)
**          Added #include of bt.h.
**      22-dec-1992 (stevet)
**          Added function prototype.
**/


/*{
** Name: adi_op_info() - Get info about an INGRES operation ID.
**
** Description:
**      This function is the external ADF call "adi_op_info()" which
**      returns an ADI_OPINFO structure containing several pieces of
**      information about the supplied op ID.
**
**      This information will include:
**	    o  Is this a "post-fix", "pre-fix", or "in-fix" operation.
**	    o  Is this an operator, or a function.
**	    o  What language(s) this op ID is valid for.
**	    o  Is this specific to INGRES, ANSI SQL standard, DB2 standard, etc.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adi_opid                        The op ID to get info about.
**	adi_info			Ptr to an ADI_OPINFO structure.
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
**      adi_info                        Operation information is returned in
**					this structure.
**	    .adi_opid			The operation ID is placed here for fun.
**	    .adi_opname			The operation name (preferred name, if
**					there are synonyms) associated with the
**					OP id.
**	    .adi_use                    One of the following:
**					    ADI_PREFIX
**					    ADI_POSTFIX
**					    ADI_INFIX
**	    .adi_optype			One of the following:
**                                          ADI_COMPARISON
**                                          ADI_OPERATOR
**                                          ADI_AGG_FUNC
**                                          ADI_NORM_FUNC
**	    .adi_langs			Languages this is valid for.  This is
**					a bit mask with the following bits
**					currently defined:
**					    DB_SQL
**					    DB_QUEL
**	    .adi_systems		What systems is this valid for.  This is
**					a bit mask with the following bits
**					currently defined:
**					    ADI_INGRES_6
**					    ADI_ANSI
**					    ADI_DB2
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
**      16-may-88 (thurston)
**          Initial creation.
**	07-jul-88 (thurston)
**	    Initial coding, changed name of routine from adi_opinfo() to
**	    adi_op_info() due to 7 character collision with adi_opid(), changed
**	    interface somewhat (the ADI_OPINFO structure).
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
*/

# ifdef ADF_BUILD_WITH_PROTOS
DB_STATUS
adi_op_info(
ADF_CB              *adf_scb,
ADI_OP_ID	    adi_opid,
ADI_OPINFO	    *adi_info)
# else
DB_STATUS
adi_op_info( adf_scb, adi_opid, adi_info)
ADF_CB              *adf_scb;
ADI_OP_ID	    adi_opid;
ADI_OPINFO	    *adi_info;
# endif
{
    ADI_OPRATION        *op;

    for(op = &Adf_globs->Adi_operations[0]; op->adi_opid != ADI_NOOP; op++)
    {
	if (op->adi_opid == adi_opid)
	{
	    adi_info->adi_opid = adi_opid;
	    STcopy(op->adi_opname.adi_opname, adi_info->adi_opname.adi_opname);
	    adi_info->adi_use     = op->adi_opuse;
	    adi_info->adi_optype  = op->adi_optype;
	    adi_info->adi_langs   = op->adi_opqlangs;
	    adi_info->adi_systems = op->adi_opsystems;
	    return(E_DB_OK);
	}
    }

    adi_info->adi_opid = ADI_NOOP;
    return(adu_error(adf_scb, E_AD2002_BAD_OPID, 0));
}
