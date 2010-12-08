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
**  Name: ADITYNAME.C - Get datatype name from id.
**
**  Description:
**      This file contains all of the routines necessary to
**      perform the external ADF call "adi_tyname()" which
**      returns the datatype name corresponding to the supplied
**      datatype id.
**
**      This file defines:
**
**          adi_tyname() - Get datatype name from id.
**
**
**  History:    $Log-for RCS$
**      24-feb-1986 (thurston)
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
** Name: adi_tyname() - Get datatype name from id.
**
** Description:
**      This function is the external ADF call "adi_tyname()" which
**      returns the datatype name corresponding to the supplied
**      datatype id.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adi_did                         The datatype id to find the
**                                      datatype name for.
**      adi_dname                       Pointer to the ADI_DT_NAME to
**					put the datatype name in.
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
**      adi_dname                       The datatype name corresponding
**                                      to the supplied datatype id.
**                                      If the supplied datatype id
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
**          E_AD2004_BAD_DTID		Datatype id unknown to ADF.
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
**	02-apr-86 (thurston)
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
**      09-nov-2010 (gupsh01) SIR 124685
**          Protype cleanup.
*/

DB_STATUS
adi_tyname(
ADF_CB              *adf_scb,
DB_DT_ID            adi_did,
ADI_DT_NAME         *adi_dname)
{
    DB_DT_ID            bdt = abs(adi_did);
    i4                  n;

    for(n=1; n <= ADI_MXDTS; n++)
    {
	if (Adf_globs->Adi_datatypes[n].adi_dtid == DB_NODT) break;

	if (Adf_globs->Adi_datatypes[n].adi_dtid == bdt)
	{
	    STcopy(Adf_globs->Adi_datatypes[n].adi_dtname.adi_dtname,
						    adi_dname->adi_dtname);
	    return(E_DB_OK);
	}
    }

    STcopy("", adi_dname->adi_dtname);	/* return empty string */
    return(adu_error(adf_scb, E_AD2004_BAD_DTID, 0));
}
