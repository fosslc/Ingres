/*
** Copyright (c) 1986, Relational Technology, Inc.
*/

#include    <compat.h>
#include    <gl.h>
#include    <tr.h>
#include    <bt.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>

/**
**
**  Name: ADIFICOERCE.C - Get function instance for a coercion.
**
**  Description:
**      This file contains all of the routines necessary to
**      perform the external ADF calls "adi_ficoerce()" and "adi_cpficoerce()",
**	which return the function instance id for the automatic conversion
**	(coercion) from one given datatype into another given datatype, either
**	for the regular system, or for the copy command.
**
**      This file defines:
**
**          adi_ficoerce() - Get function instance for a coercion.
**          adi_cpficoerce() - Get function instance for a coercion for copy.
**
**
**  History:
**      25-feb-1986 (thurston)
**          Initial creation.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	07-jan-87 (thurston)
**	    In adi_ficoerce(), I allowed holes in fitab to be skipped over.
**	22-feb-87 (thurston)
**	    Added support for nullable datatypes.
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	18-mar-87 (thurston)
**	    Added some WSCREADONLY to pacify the Whitesmith compiler on MVS.
**	24-jun-87 (thurston)
**	    Added routine adi_cpficoerce().
**	05-oct-88 (thurston)
**	    Updated adi_cpficoerce() to use adi_ficoerce() to find a NORMAL
**	    COERCION if no COPY COERCION is not found.  This does not change
**	    the functionality of this routine, but may enhance performance
**	    since the adi_ficoerce() routine has been beefed up.
**	06-oct-88 (thurston)
**	    Implemented fast coercion search in adi_ficoerce() using new field
**	    in the datatypes table.
**	27-Jan-89 (fred)
**	    Altered reference to Adi_fis to be thru Adf_globs -- necessary for
**	    user-define datatype support
**	28-apr-89 (fred)
**	    Altered to use ADI_DT_MAP_MACRO().
**	05-jun-89 (fred
**	    Complete that last fix.
**      22-mar-91 (jrb)
**          Added #include of bt.h.
**      12-dec-1992 (stevet)
**          Added function prototype.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*{
** Name: adi_ficoerce() - Get function instance for a coercion.
**
** Description:
**      This function is the external ADF call "adi_ficoerce()" which
**      returns the function instance id for the automatic conversion
**	(coercion) from one given datatype into another given datatype.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adi_from_did                    The "from" datatype id.
**      adi_into_did                    The "into" datatype id.
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
**      adi_fid                         The function instance id of the
**                                      function instance to do the
**                                      automatic conversion (coercion).
**                                      If there is no coercion for the
**                                      given datatypes, this will be
**                                      returned as ADI_NOCOERCION.
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
**          E_AD2004_BAD_DTID           One of the datatype ids is
**					unknown to ADF.
**          E_AD2009_NOCOERCION         There is no coercion for the
**                                      given datatypes.
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
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	07-jan-87 (thurston)
**	    Allowed holes in fitab to be skipped over.
**	22-feb-87 (thurston)
**	    Added support for nullable datatypes.
**	06-oct-88 (thurston)
**	    Implemented fast coercion search using new field in the datatypes
**	    table.
**	22-Oct-07 (kiria01) b119354
**	    Split adi_dtcoerce into a quel and SQL version
*/

# ifdef ADF_BUILD_WITH_PROTOS
DB_STATUS
adi_ficoerce(
ADF_CB              *adf_scb,
DB_DT_ID            adi_from_did,
DB_DT_ID            adi_into_did,
ADI_FI_ID           *adi_fid)
# else
DB_STATUS
adi_ficoerce( adf_scb, adi_from_did, adi_into_did, adi_fid)
ADF_CB              *adf_scb;
DB_DT_ID            adi_from_did;
DB_DT_ID            adi_into_did;
ADI_FI_ID           *adi_fid;
# endif
{
    DB_DT_ID		from_bdt = abs(adi_from_did);
    DB_DT_ID		into_bdt = abs(adi_into_did);
    DB_DT_ID		minto_bdt;
    DB_DT_ID		mfrom_bdt;
    ADI_DATATYPE	*dt;
    ADI_COERC_ENT	*cent;
    i4		        found = FALSE;

    mfrom_bdt = ADI_DT_MAP_MACRO(from_bdt);
    minto_bdt = ADI_DT_MAP_MACRO(into_bdt);

    for (;;)
    {
	/* Check the validity of the datatype ids */

        if (   (mfrom_bdt <= 0 || mfrom_bdt > ADI_MXDTS)
	    || (minto_bdt <= 0 || minto_bdt > ADI_MXDTS)
	    || Adf_globs->Adi_dtptrs[minto_bdt] == NULL
	    || (dt = Adf_globs->Adi_dtptrs[mfrom_bdt]) == NULL
	   )
	{
	    return (adu_error(adf_scb, E_AD2004_BAD_DTID, 0));
	}

	/* First check the datatypes table to see if a coercion exists */
	if (!BTtest((i4) minto_bdt, adf_scb->adf_qlang == DB_SQL
				? (char *) &dt->adi_dtcoerce_sql
				: (char *) &dt->adi_dtcoerce_quel))
	    return (adu_error(adf_scb, E_AD2009_NOCOERCION, 0));


	/* Search for the appropriate function instance id */
	for (cent = dt->adi_coerc_ents; cent->adi_from_dt == from_bdt; cent++)
	{
	    if (cent->adi_into_dt == into_bdt)
	    {
		*adi_fid = cent->adi_fid_coerc;
		found = TRUE;
		break;
	    }
	}
	break;

    }	/* end of for(;;) stmt */

    if (!found)
	return (adu_error(adf_scb, E_AD2009_NOCOERCION, 0));
    else
        return (E_DB_OK);    
}


/*{
** Name: adi_cpficoerce() - Get function instance for a coercion for copy.
**
** Description:
**	This function is the external ADF call "adi_cpficoerce()" which
**	returns the function instance id for the automatic conversion
**	(coercion) from one given datatype into another given datatype
**	for the copy command.  This differs from the "adi_ficoerce()"
**	call in that there may be a copy coercion that exists for a
**	pair of datatypes that does not have a normal coercion, or the
**	copy coercion may have slightly different semantics, thus a
**	separate function instance.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adi_from_did                    The "from" datatype id.
**      adi_into_did                    The "into" datatype id.
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
**      adi_fid                         The function instance id of the
**                                      function instance to do the automatic
**                                      conversion (coercion).  If there is no
**                                      coercion (either copy or regular
**                                      coercion) for the given given datatypes,
**                                      this will be returned as ADI_NOCOERCION.
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
**          E_AD2004_BAD_DTID           One of the datatype ids is
**					unknown to ADF.
**          E_AD2009_NOCOPYCOERCION     There is no copy coercion or regular
**					coercion for the given datatypes.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      24-jun-87 (thurston)
**          Initial creation.
**	05-oct-88 (thurston)
**	    Updated to use adi_ficoerce() to find a NORMAL COERCION if no COPY
**	    COERCION is not found.  This does not change the functionality of
**	    this routine, but may enhance performance since the adi_ficoerce()
**	    routine has been beefed up.
**	09-Mar-1999 (shero03)
**	    Support data type array in fidesc
*/

# ifdef ADF_BUILD_WITH_PROTOS
DB_STATUS
adi_cpficoerce(
ADF_CB              *adf_scb,
DB_DT_ID            adi_from_did,
DB_DT_ID            adi_into_did,
ADI_FI_ID           *adi_fid)
# else
DB_STATUS
adi_cpficoerce( adf_scb, adi_from_did, adi_into_did, adi_fid)
ADF_CB              *adf_scb;
DB_DT_ID            adi_from_did;
DB_DT_ID            adi_into_did;
ADI_FI_ID           *adi_fid;
# endif
{
    DB_DT_ID		from_bdt = abs(adi_from_did);
    DB_DT_ID		into_bdt = abs(adi_into_did);
    ADI_FI_DESC	        *fi_desc_ptr;
    i4		        found = FALSE;
    DB_STATUS		db_stat;
    DB_DT_ID		minto_bdt;
    DB_DT_ID		mfrom_bdt;
    
    mfrom_bdt = ADI_DT_MAP_MACRO(from_bdt);
    minto_bdt = ADI_DT_MAP_MACRO(into_bdt);

    for (;;)
    {
	/* Check the validity of the datatype ids */

        if (   (mfrom_bdt <= 0 || mfrom_bdt > ADI_MXDTS)
	    || (minto_bdt <= 0 || minto_bdt > ADI_MXDTS)
	    || Adf_globs->Adi_dtptrs[mfrom_bdt] == NULL
	    || Adf_globs->Adi_dtptrs[minto_bdt] == NULL
	   )
	{
	    return (adu_error(adf_scb, E_AD2004_BAD_DTID, 0));
	}

	/* Search for the appropriate COPY COERCION function instance id */
	for (fi_desc_ptr = (ADI_FI_DESC *)&Adf_globs->Adi_fis[Adf_globs->
						   Adi_copy_coercion_fis];
		(   fi_desc_ptr->adi_fitype == ADI_COPY_COERCION
		 && fi_desc_ptr->adi_finstid != ADFI_ENDTAB
		);
	     fi_desc_ptr++
	    )
	{
	    if (fi_desc_ptr->adi_finstid == ADI_NOFI)
		continue;   /* Hole in fitab ... skip this one. */

	    if (fi_desc_ptr->adi_dtresult == into_bdt
		&& fi_desc_ptr->adi_dt[0] == from_bdt
	       )
	    {
		*adi_fid = fi_desc_ptr->adi_finstid;
		found = TRUE;
		return (E_DB_OK);
	    }
	}
	break;

    }	/* end of for(;;) stmt */


    /* Attempt to find normal COERCION function instance ID */

    if (db_stat = adi_ficoerce(adf_scb, from_bdt, into_bdt, adi_fid))
    {
	if (adf_scb->adf_errcb.ad_errcode == E_AD2009_NOCOERCION)
	    db_stat = adu_error(adf_scb, E_AD200A_NOCOPYCOERCION, 0);
    }
    return (db_stat);
}
