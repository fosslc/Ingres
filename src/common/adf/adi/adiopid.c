/*
** Copyright (c) 1986, Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <cm.h>
#include    <adfint.h>
/* [@#include@]... */

/**
**
**  Name: ADIOPID.C - Get operator id from name.
**
**  Description:
**      This file contains all of the routines necessary to
**      perform the  external ADF call "adi_opid()" which
**	returns the operator id corresponding to the supplied
**      operator name.
**
**      This file defines:
**
**          adi_opid() - Get operator id from name.
**
**
**  History:    $Log-for RCS$
**      24-feb-86 (thurston)
**          Initial creation.
**	03-apr-86 (thurston)
**	    In adi_opid(), I changed to using STbcompare() instead of
**	    STcompare() for comparing operation names.
**	01-jul-86 (thurston)
**	    Routine adi_opid() rewritten to acount for new version of the
**	    operations table. 
**	04-jul-86 (thurston)
**	    In adi_opid(), I got rid of the call to STbcompare() by coding the
**	    necessary algorithm in line.  STbcompare() didn't do exactly what I
**	    needed.  Neither would STcompare(), or STscompare().  This file is
**	    no longer dependent on the ST module of the CL.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	11-sep-86 (thurston)
**	    Changed adi_operations to Adi_operations in adi_opid().
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	09-mar-87 (thurston)
**	    Added code to adi_opid() to take use of the new .adi_opqlangs field
**	    in the operations table; verifies that operation is defined for
**	    query language in use.
**      21-dec-1992 (stevet)
**          Added function prototype.
**	24-feb-93 (ralph)
**	    DELIM_IDENT:
**	    Use CMcmpnocase to treat Adi_operations.adi_opname.adi_opname
**	    as case insensitive when looking up opid.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*{
** Name: adi_opid() - Get operator id from name.
**
** Description:
**      This function is the external ADF call "adi_opid()" which
**      returns the operator id corresponding to the supplied
**      operator name.  All operators, functions, conversion functions,
**	and aggregate functions known to the ADF have an operator name
**	and operator id assigned to them.  Normally, the name is used
**	as a handle through this routine to retrieve the operator id.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	    .adf_qlang			Query language in use.
**      adi_oname                       The operator name to find the
**                                      operator id for.
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
**      adi_oid                         The operator id corresponding
**                                      to the supplied operator name.
**                                      If the supplied operator name
**                                      is not recognized by ADF, this
**                                      will be returned as ADI_NOOP.
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
**          E_AD2001_BAD_OPNAME		Operator name unknown to ADF.
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
**	03-apr-86 (thurston)
**	    Changed to using STbcompare() instead of STcompare() for
**	    comparing operation names.
**	01-jul-86 (thurston)
**	    Rewritten to acount for new version of the operations table.
**	04-jul-86 (thurston)
**	    I got rid of the call to STbcompare() by coding the
**	    necessary algorithm in line.  STbcompare() didn't do exactly what I
**	    needed.  Neither would STcompare(), or STscompare().  This routine
**	    is no longer dependent on the ST module of the CL.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	11-sep-86 (thurston)
**	    Changed adi_operations to Adi_operations.
**	09-mar-87 (thurston)
**	    Added code to take use of the new .adi_opqlangs field in the
**	    operations table; verifies that operation is defined for query
**	    language in use.
**	24-feb-93 (ralph)
**	    DELIM_IDENT:
**	    Use CMcmpnocase to treat Adi_operations.adi_opname.adi_opname
**	    as case insensitive when looking up opid.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
*/

# ifdef ADF_BUILD_WITH_PROTOS
DB_STATUS
adi_opid(
ADF_CB             *adf_scb,
ADI_OP_NAME        *adi_oname,
ADI_OP_ID          *adi_oid)
# else
DB_STATUS
adi_opid( adf_scb, adi_oname, adi_oid)
ADF_CB             *adf_scb;
ADI_OP_NAME        *adi_oname;
ADI_OP_ID          *adi_oid;
# endif
{
    i4                  s;
    i4                  i;
    i4                  cmp;
    char		*c1;
    char		*c2;
    ADI_OPRATION       *op;

    s = sizeof(adi_oname->adi_opname);

    for(op = &Adf_globs->Adi_operations[0]; op->adi_opid != ADI_NOOP; op++)
    {
	cmp = 1;
	c1 = &op->adi_opname.adi_opname[0];
	c2 = &adi_oname->adi_opname[0];
	i = 0;
	while ((i++ < s)  &&  (*c1 != 0  ||  *c2 != 0))
	{
	    if (CMcmpnocase(c1,c2) != 0)
	    {
		cmp = 0;
		break;
	    }
	    c1++;
	    c2++;
	}

	if (cmp)
	{
	    if (op->adi_opqlangs & adf_scb->adf_qlang)
	    {
	        *adi_oid = op->adi_opid;
	        return(E_DB_OK);
	    }
	    else
	    {
		/* Operation name not defined for query language in use. */
    		*adi_oid = ADI_NOOP;
    		return(adu_error(adf_scb, E_AD2001_BAD_OPNAME, 0));
	    }
	}
    }

    /* Operation name not found in table. */
    *adi_oid = ADI_NOOP;
    return(adu_error(adf_scb, E_AD2001_BAD_OPNAME, 0));
}
