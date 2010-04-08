/*
** Copyright (c) 1986, Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <st.h>
#include    <me.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
/* [@#include@]... */

/**
**
**  Name: ADITYID.C - Get datatype id from name.
**
**  Description:
**      This file contains all of the routines necessary to
**      perform the external ADF call "adi_tyid()" which
**      returns the base datatype id corresponding to the supplied
**      datatype name.
**
**      This file defines:
**
**          adi_tyid() - Get base datatype id from name.
**
**
**  History:    $Log-for RCS$
**      24-feb-1986 (thurston)
**          Initial creation.
**	03-apr-86 (thurston)
**	    In adi_tyid(), I changed to using STbcompare() instead of
**	    STcompare() to compare datatype names.
**	04-jul-86 (thurston)
**	    In adi_tyid(), I got rid of the call to STbcompare() by coding the
**	    necessary algorithm in line.  STbcompare() didn't do exactly what I
**	    needed.  Neither would STcompare(), or STscompare().  This file is
**	    no longer dependent on the ST module of the CL.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
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
** Name: adi_tyid() - Get base datatype id from name.
**
** Description:
**      This function is the external ADF call "adi_tyid()" which
**      returns the base datatype id corresponding to the supplied
**      datatype name.  Every datatype know to the ADF has a datatype
**	name and base datatype id assigned to it.  Normally, the name
**	is used as a handle through this routine to retrieve the base
**	datatype id.  The type name is required to be lowercase with no
**	trailing blanks.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adi_dname                       The datatype name to find the
**                                      base datatype id for.
**      adi_did                         Pointer to the DB_DT_ID to
**					put the base datatype id in.
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
**      *adi_did                        The base datatype id corresponding
**                                      to the supplied datatype name.
**                                      If the supplied datatype name
**                                      is not recognized by ADF, this
**                                      will be returned as DB_NODT.
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
**          E_AD2003_BAD_DTNAME		Datatype name unknown to ADF.
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
**	    Changed to using STbcompare() instead of STcompare() to
**	    compare datatype names.
**	04-jul-86 (thurston)
**	    I got rid of the call to STbcompare() by coding the
**	    necessary algorithm in line.  STbcompare() didn't do exactly what I
**	    needed.  Neither would STcompare(), or STscompare().  This routine
**	    is no longer dependent on the ST module of the CL.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	28-sep-2006 (gupsh01)
**	    Added support for searching alias date.
**	13-feb-2007
**	    Fixed searching for the date alias.
*/

# ifdef ADF_BUILD_WITH_PROTOS
DB_STATUS
adi_tyid(
ADF_CB             *adf_scb,
ADI_DT_NAME        *adi_dname,
DB_DT_ID           *adi_did)
# else
DB_STATUS
adi_tyid( adf_scb, adi_dname, adi_did)
ADF_CB             *adf_scb;
ADI_DT_NAME        *adi_dname;
DB_DT_ID           *adi_did;
# endif
{
    i4                  s;
    i4                  n;
    i4                  i;
    i4                  cmp;
    char		*c1;
    char		*c2;
    ADI_DT_NAME         dt_name;
    bool		dategiven = FALSE;

    s = sizeof(adi_dname->adi_dtname);
    c2 = &adi_dname->adi_dtname[0];

    if (c2 && ((*c2 == 'd') || (*c2 == 'D')))
    {
          if (STbcompare (c2, 0, "date", 0, TRUE) == 0)
          {
	    dategiven = TRUE;
            if (adf_scb->adf_date_type_alias & AD_DATE_TYPE_ALIAS_INGRES)
            {
                /* replace the input string with ingresdate */
                MEmove(10, (PTR)"ingresdate", '\0',
                                sizeof(ADI_DT_NAME), (PTR)&dt_name.adi_dtname);
		dt_name.adi_dtname[10]='\0';
            }
            else if (adf_scb->adf_date_type_alias & AD_DATE_TYPE_ALIAS_ANSI)
            {
                /* replace the input string with ansidate */
                MEmove(8, (PTR)"ansidate", '\0',
                                sizeof(ADI_DT_NAME), (PTR)&dt_name.adi_dtname);
		dt_name.adi_dtname[8]='\0';
            }
            else
            {
                return (adu_error(adf_scb, E_AD5065_DATE_ALIAS_NOTSET, 0));
            }
          }
    }

    for(n=1; n <= ADI_MXDTS; n++)
    {
	if (Adf_globs->Adi_datatypes[n].adi_dtid == DB_NODT) break;

	cmp = 1;
	c1 = &Adf_globs->Adi_datatypes[n].adi_dtname.adi_dtname[0];
	if (dategiven == TRUE)
	  c2 = (PTR)&dt_name.adi_dtname;
	else
    	  c2 = &adi_dname->adi_dtname[0];
	i = 0;

	while ((i++ < s)  &&  (*c1 != 0  ||  *c2 != 0))
	{
	    if (*c1++ != *c2++)
	    {
		cmp = 0;
		break;
	    }
	}

	if (cmp)
	{
	    *adi_did = Adf_globs->Adi_datatypes[n].adi_dtid;
	    return(E_DB_OK);
	}
    }

    *adi_did = DB_NODT;
    return(adu_error(adf_scb, E_AD2003_BAD_DTNAME, 0));
}
