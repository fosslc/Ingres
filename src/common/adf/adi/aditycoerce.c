/*
** Copyright (c) 1986, Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
/* [@#include@]... */

/**
**
**  Name: ADITYCOERCE.C - Get the set of datatypes a given datatype can
**                        be coerced into.
**
**  Description:
**      This file contains all of the routines necessary to
**      perform the external ADF call "adi_tycoerce()" which
**      returns the set of datatypes the supplied datatype can
**	be automatically converted (coerced) into.
**
**      This file defines:
**
**          adi_tycoerce() - Get the set of datatypes a given datatype
**                           can be coerced into.
**
**
**  History:    $Log-for RCS$
**      24-feb-86 (thurston)
**          Initial creation.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	22-feb-87 (thurston)
**	    Added support for nullable datatypes.
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**      22-dec-1992 (stevet)
**          Added function prototype.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*{
** Name: adi_tycoerce()	- Get the set of datatypes a given datatype can
**                        be coerced into.
**
** Description:
**      This function is the external ADF call "adi_tycoerce()" which
**      returns the set of datatypes the supplied datatype can
**      be automatically converted (coerced) into.  This is done via
**      a datatype bit-mask where each bit set in the mask corresponds
**      to one base datatype that the given datatype can be coerced into.
**
**	By definition, if a given datatype can be coerced into a particular
**	base datatype, then it can also be coerced into that datatype's
**	nullable counterpart.  Likewise, if it can be coerced into a nullable
**	datatype, it can also be coerced into the non-nullable counterpart.
**	Furthermore, if a nullable (or non-nullable) datatype can be coerced
**	into a some datatype, then the non-nullable (or nullable) counterpart
**	can also be coerced into that datatype.
**
**      For instance, if the mask comes back with only bit number 53 and
**      bit number 91 set, that means that the given datatype can only
**      be coerced into the datatypes whose ids are 53, 91, and their nullable
**	counterparts.
**
**      Note:  These datatype bit masks should always be manipulated
**      =====  using the BT routines in the CL.  This will assure that
**             the bits get used in a compatible way inside and outside
**             of ADF.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adi_did                         The datatype id to find the
**                                      internal length for.
**      adi_dmsk                        Pointer to the ADI_DT_BITMASK to
**					put the datatype bit mask in.
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
**      *adi_dmsk                       The datatype bit mask of the set
**                                      of all datatypes the supplied
**                                      datatype can be coerced into.
**					If the datatype id is unknown to
**					to ADF, this will be zero'ed out.
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
**          E_AD2004_BAD_DTID  		Datatype id unknown to ADF.
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
**	    Initial coding.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	22-feb-87 (thurston)
**	    Added support for nullable datatypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	22-Oct-07 (kiria01) b119354
**	    Split adi_dtcoerce into a quel and SQL version
**      09-nov-2010 (gupsh01) SIR 124685
**          Protype cleanup.
*/

DB_STATUS
adi_tycoerce(
ADF_CB              *adf_scb,
DB_DT_ID            adi_did,
ADI_DT_BITMASK      *adi_dmsk)
{
    DB_DT_ID            bdt = abs(adi_did);
    u_i2                bm_size = sizeof(ADI_DT_BITMASK);
    i4                  n;


    for(n=1; n <= ADI_MXDTS; n++)
    {
	if (Adf_globs->Adi_datatypes[n].adi_dtid == DB_NODT) break;

	if (Adf_globs->Adi_datatypes[n].adi_dtid == bdt)
	{
	    MEcopy(adf_scb->adf_qlang == DB_SQL
			? (PTR)&Adf_globs->Adi_datatypes[n].adi_dtcoerce_sql
			: (PTR)&Adf_globs->Adi_datatypes[n].adi_dtcoerce_quel,
			bm_size, (PTR)adi_dmsk);
	    return(E_DB_OK);
	}
    }

    MEfill(bm_size, (u_char)0, (PTR)adi_dmsk);
    return(adu_error(adf_scb, E_AD2004_BAD_DTID, 0));
}
