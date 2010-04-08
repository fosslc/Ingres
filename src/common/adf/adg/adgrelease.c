/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>

/**
**
**  Name: ADGRELEASE.C - Release ADF for a session.
**
**  Description:
**	This file contains all of the routines necessary to perform the 
**	external ADF call "adg_release()" which releases the ADF for a
**	session within a server (or for a non-server process, such as a
**	front end).
**
**	This file defines:
**
**	    adg_release() - Release ADF for a session.
**
**
**  History:	$Log-for RCS$
**	24-feb-86 (thurston)
**	    Initial creation.
**	07-jun-86 (thurston)
**	    Stubbed adg_release() to always return E_AD0000_OK, as it is not
**	    defined as doing anything, yet.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**      21-dec-1992 (stevet)
**          Added function prototype.
**/


/*{
** Name: adg_release()	- Release ADF for a session.
**
** Description:
**	This function is the external ADF call "adg_release()" which
**	releases the ADF for a session within a server (or for a
**	non-server process, such as a front end).  It does whatever
**	cleanup and freeing of resources ADF had allocated for the
**	session.
**
**	Currently, this is a no-op since ADF does not yet allocate
**	any resources for a session.  However, it should be called
**	when terminating a session, since it might someday do this.
**	
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
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
**	    E_AD0000_OK			Operation succeeded.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	24-feb-86 (thurston)
**	    Initial creation.
**	07-jun-86 (thurston)
**	    Stubbed the routine to always return E_AD0000_OK, as it is not
**	    defined as doing anything, yet.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
*/

# ifdef ADF_BUILD_WITH_PROTOS
DB_STATUS
adg_release(
ADF_CB  *adf_scb)
# else
DB_STATUS adg_release(adf_scb)
ADF_CB  *adf_scb;
# endif
{
    return(E_DB_OK);
}
