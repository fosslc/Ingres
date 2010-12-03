/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <adfhist.h>

/**
**
**  Name: ADCHMAX.C - Create the maximum histogram element for a datatype.
**
**  Description:
**      This file contains all of the routines necessary to
**      perform the  external ADF calls "adc_hmax()" and "adc_dhmax()" which
**      create the maximum histogram element for the supplied datatype
**	and the "default" maximum histogram element for a supplied
**	datatype respectively.
**	
**
**      This file defines:
**
**	    adc_dhmax() -   Create the "default" maximum histogram element
**			    for a given datatype.
**	    adc_1dhmax_rti() -  Create the "default" maximum histogram element
**			    for a given RTI known datatype.
**          adc_hmax() -    Create the maximum histogram element for a datatype.
**	    adc_1hmax_rti() -   Create the maximum histogram element for a
**			    given RTI known datatype.
**
**
**  History:
**      26-feb-86 (thurston)
**          Initial creation.
**	18-jun-86 (ericj)
**	    Added the adc_dhmax() function as requested by OPF designer.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	28-oct-86 (thurston)
**	    Added char and varchar support.
**	20-feb-87 (thurston)
**	    Added support for nullable datatypes.
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	19-apr-89 (mikem)
**	    Logical key development.  Added support for logical_key datatype
**	    to adc_1dhmax_rti() and adc_hmax_rti().
**	28-apr-89 (fred)
**	    Altered references to Adi_dtptrs to use ADI_DT_MAP_MACRO()
**	05-jun-89 (fred)
**	    Completed above alteration :-)
**	05-jul-89 (jrb)
**	    Added decimal datatype support.
**	06-feb-92 (stevet)
**	    Changed xDEBUG code to check for precision only for DB_DEC_TYPE.
**      22-dec-1992 (stevet)
**          Added function prototype.
**       6-Apr-1993 (fred)
**          Added byte string types.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      04-Feb-2002 (fanra01)
**          Bug 107016
**          Use macros to determine the number of items used in the creation
**          of maximum histogram elements for DB_NCHR_TYPE and DB_NVCHR_TYPE.
**	08-aug-2007 (toumi01)
**	    Fix AD_UTF8_ENABLED flag test && vs. & typo.
**      14-aug-2009 (joea)
**          	Add cases for DB_BOO_TYPE in adc_1hmax_rti and adc_2dhmax_rti.
**      09-mar-2010 (thich01)
**          Add DB_NBR_TYPE like DB_BYTE_TYPE for rtree indexing.
**	19-Nov-2010 (kiria01) SIR 124690
**	    Add support for UCS_BASIC collation. Don't allow UTF8 strings with it
**	    to use UCS2 CEs for comparison related actions.
**/



/*{
** Name: adc_dhmax()	-   Create the "default" maximum histogram element
**			    for a datatype.
**
** Description:
**      This function is the external ADF call "adc_dhmax()" which
**      creates the "default" maximum histogram element for the
**	supplied datatype.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	adc_fromdv			DB_DATA_VALUE describing the data value
**					for which a maximum "default" histogram
**					value is to be made.
**	    .db_datatype		The datatype to get the maximum
**					default histogram value for.
**	    .db_prec			The precision/scale to get the maximum
**					default histogram value for.
**	    .db_length			Length of the data value for which
**					a maximum "default" histogram is being
**					made.
**      adc_max_dvdhg                   Pointer to the DB_DATA_VALUE
**					which describes the maximum default
**					histogram element to make.
**	    .db_datatype		Datatype id for the data value
**					being created.
**	    .db_prec			Precision/Scale for the data value
**					being created.
**	    .db_length  		Length for the data value
**					being created.
**	    .db_data    		Pointer to location to place the
**					actual data for the data value
**					being created.
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
**      adc_max_dvhg                    The maximum histogram element.
**	        .db_data    		The actual data for the data
**					value created will be placed
**					here.
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
**          E_AD2004_BAD_DTID           Datatype id unknown to ADF.
**
**	    NOTE:  The following error codes are for internal consistency
**	    checking and will be returned only when the system is compiled
**	    with the xDEBUG flag.
**
**	    E_AD3011_BAD_HG_DTID	Datatype for the resulting
**					data value is not correct for
**					histogram elements created from
**					the given datatype
**          E_AD3012_BAD_HG_DTLEN       Length for the resulting
**					data value is not correct for
**					histogram elements created from
**					the given datatype.
**          E_AD3013_BAD_HG_DTPREC      Precision/Scale for the resulting
**					data value is not correct for
**					histogram elements created from
**					the given datatype.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      18-jun-86 (ericj)
**          Initial creation.
**	25-jun-86 (ericj)
**	    Added DB_DATA_VALUE parameter, adc_fromdv,
**	    so a call can be made to adc_hg_dtln() for internal consistency
**	    checking.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	20-feb-87 (thurston)
**	    Added support for nullable datatypes.
**      22-mar-91 (jrb)
**          Put comment markers around "xDEBUG" after #endif's.  This was
**          causing problems on some compilers.
**	06-feb-92 (stevet)
**	    Changed xDEBUG code to check for precision only for DB_DEC_TYPE.
**	18-jan-93 (rganski)
**	    Initialized loc_hgdv.db_length before xDEBUG call to
**	    adc_hg_dtln(), which now uses this length as an input parameter.
**	19-Nov-2010 (kiria01) SIR 124690
**	    Ensure whole DBV copied.
*/

# ifdef ADF_BUILD_WITH_PROTOS
DB_STATUS
adc_dhmax(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *adc_fromdv,
DB_DATA_VALUE	    *adc_max_dvdhg)
# else
DB_STATUS
adc_dhmax( adf_scb, adc_fromdv, adc_max_dvdhg)
ADF_CB              *adf_scb;
DB_DATA_VALUE	    *adc_fromdv;
DB_DATA_VALUE	    *adc_max_dvdhg;
# endif
{
    DB_STATUS		db_stat = E_DB_OK;
    i4			bdt     = abs((i4) adc_fromdv->db_datatype);
    i4			mx_bdt  = abs((i4) adc_max_dvdhg->db_datatype);
    i4			mbdt;
    i4			mmx_bdt;

    mbdt = ADI_DT_MAP_MACRO(bdt);
    mmx_bdt = ADI_DT_MAP_MACRO(mx_bdt);
    for (;;)
    {
        if (	mbdt <= 0
	    ||  mbdt > ADI_MXDTS
	    ||  Adf_globs->Adi_dtptrs[mbdt] == NULL
	   )
	{
	    db_stat = adu_error(adf_scb, E_AD2004_BAD_DTID, 0);
	    break;
	}

        if (	mmx_bdt <= 0
	    ||  mmx_bdt > ADI_MXDTS
	    ||  Adf_globs->Adi_dtptrs[mmx_bdt] == NULL
	   )
	{
	    db_stat = adu_error(adf_scb, E_AD2004_BAD_DTID, 0);
	    break;
	}

#ifdef xDEBUG
	/* Do a consistency check of adc_max_dvdhg's datatype, prec, and length
	** specification.
	*/
	{
	    DB_DATA_VALUE	loc_hgdv;
	
	    loc_hgdv.db_length = adc_max_dvdhg->db_length;

	    if (db_stat = adc_hg_dtln(adf_scb, adc_fromdv, &loc_hgdv))
		break;

	    if (loc_hgdv.db_datatype != adc_max_dvdhg->db_datatype)
	    {
		db_stat = adu_error(adf_scb, E_AD3011_BAD_HG_DTID, 0);
		break;
	    }

	    if (loc_hgdv.db_length != adc_max_dvdhg->db_length)
	    {
		db_stat = adu_error(adf_scb, E_AD3012_BAD_HG_DTLEN, 0);
		break;
	    }

	    if (loc_hgdv.db_prec != adc_max_dvdhg->db_prec &&
		abs(loc_hgdv.db_datatype) == DB_DEC_TYPE)
	    {
		db_stat = adu_error(adf_scb, E_AD3013_BAD_HG_DTPREC, 0);
		break;
	    }
	}
#endif  /* xDEBUG */

	/*
	** Now call the low-level routine to do the work.  Note that if the
	** datatype is nullable, we use a temp DV_DATA_VALUE which gets set
	** to the corresponding base datatype and length.
	*/

	if (adc_fromdv->db_datatype > 0)	/* non-nullable */
	{
	    db_stat = (*Adf_globs->Adi_dtptrs[mbdt]->
			    adi_dt_com_vect.adp_dhmax_addr)
				(adf_scb, adc_fromdv, adc_max_dvdhg);
	}
	else					/* nullable */
	{
	    DB_DATA_VALUE tmp_dv = *adc_fromdv;

	    tmp_dv.db_datatype = bdt;
	    tmp_dv.db_length--;

	    db_stat = (*Adf_globs->Adi_dtptrs[mbdt]->
			    adi_dt_com_vect.adp_dhmax_addr)
				(adf_scb, &tmp_dv, adc_max_dvdhg);
	}
	break;
    }	    /* end of for stmt */

    return(db_stat);
}


/*{
** Name: adc_hmax()	-   Create the maximum histogram element
**			    for a datatype.
**
** Description:
**      This function is the external ADF call "adc_hmax()" which
**      creates the  maximum histogram element for the
**	supplied datatype.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	adc_fromdv			DB_DATA_VALUE describing the data value
**					for which a maximum histogram
**					value is to be made.
**	    .db_datatype		The datatype to get the maximum
**					histogram value for.
**	    .db_prec			The precision/scale to get the maximum
**					histogram value for.
**	    .db_length			Length of the data value for which
**					a maximum histogram is being
**					made.
**      adc_max_dvhg                    Pointer to the DB_DATA_VALUE
**					which describes the maximum
**					histogram element to make.
**	    .db_datatype		Datatype id for the data value
**					being created.
**	    .db_prec			Precision/Scale for the data value
**					being created.
**	    .db_length  		Length for the data value
**					being created.
**	    .db_data    		Pointer to location to place the
**					actual data for the data value
**					being created.
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
**      adc_max_dvhg                    The maximum histogram element.
**	     .db_data    		The actual data for the data
**					value created will be placed
**					here.
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
**          E_AD2004_BAD_DTID           Datatype id unknown to ADF.
**
**	    NOTE:  The following error codes are for internal consistency
**	    checking and will be returned only when the system is compiled
**	    with the xDEBUG flag.
**
**	    E_AD3011_BAD_HG_DTID	Datatype for the resulting
**					data value is not correct for
**					histogram elements created from
**					the given datatype
**          E_AD3012_BAD_HG_DTLEN       Length for the resulting
**					data value is not correct for
**					histogram elements created from
**					the given datatype.
**          E_AD3013_BAD_HG_DTPREC      Precision/Scale for the resulting
**					data value is not correct for
**					histogram elements created from
**					the given datatype.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	26-feb-86 (thurston)
**	    Initial creation.
**      18-jun-86 (ericj)
**          Removed adc_did parameter and corresponding error number,
**	    E_AD3011_BAD_HG_DTID.  Coded.
**	25-jun-86 (ericj)
**	    Decided to add adc_did parameter back.  Pass it as a DB_DATA_VALUE
**	    so a call can be made to adc_hg_dtln() for internal consistency
**	    checking.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	20-feb-87 (thurston)
**	    Added support for nullable datatypes.
**      22-mar-91 (jrb)
**          Put comment markers around "xDEBUG" after #endif's.  This was
**          causing problems on some compilers.
**	06-feb-92 (stevet)
**	    Changed xDEBUG code to check for precision only for DB_DEC_TYPE.
**	18-jan-93 (rganski)
**	    Initialized loc_hgdv.db_length before xDEBUG call to
**	    adc_hg_dtln(), which now uses this length as an input parameter.
*/

# ifdef ADF_BUILD_WITH_PROTOS
DB_STATUS
adc_hmax(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *adc_fromdv,
DB_DATA_VALUE	    *adc_max_dvhg)
# else
DB_STATUS
adc_hmax( adf_scb, adc_fromdv, adc_max_dvhg)
ADF_CB              *adf_scb;
DB_DATA_VALUE	    *adc_fromdv;
DB_DATA_VALUE	    *adc_max_dvhg;
# endif
{
    DB_STATUS           db_stat = E_DB_OK;
    i4			bdt     = abs((i4) adc_fromdv->db_datatype);
    i4			mx_bdt  = abs((i4) adc_max_dvhg->db_datatype);
    i4			mbdt;
    i4			mmx_bdt;

    mbdt = ADI_DT_MAP_MACRO(bdt);
    mmx_bdt = ADI_DT_MAP_MACRO(mx_bdt);

    for (;;)
    {
	if (	mbdt <= 0
	    ||  mbdt > ADI_MXDTS
	    ||  Adf_globs->Adi_dtptrs[mbdt] == NULL
	   )
	{
	    db_stat = adu_error(adf_scb, E_AD2004_BAD_DTID, 0);
	    break;
	}

	if (	mmx_bdt <= 0
	    ||  mmx_bdt > ADI_MXDTS
	    ||  Adf_globs->Adi_dtptrs[mmx_bdt] == NULL
	   )
	{
	    db_stat = adu_error(adf_scb, E_AD2004_BAD_DTID, 0);
	    break;
	}

#ifdef xDEBUG
	/* Do a consistency check of adc_max_dvdhg's datatype, prec, and length
	** specification.
	*/
	{
	    DB_DATA_VALUE	loc_hgdv;
	
	    loc_hgdv.db_length = adc_max_dvhg->db_length;

	    if (db_stat = adc_hg_dtln(adf_scb, adc_fromdv, &loc_hgdv))
		break;

	    if (loc_hgdv.db_datatype != adc_max_dvhg->db_datatype)
	    {
		db_stat = adu_error(adf_scb, E_AD3011_BAD_HG_DTID, 0);
		break;
	    }

	    if (loc_hgdv.db_length != adc_max_dvhg->db_length)
	    {
		db_stat = adu_error(adf_scb, E_AD3012_BAD_HG_DTLEN, 0);
		break;
	    }

	    if (loc_hgdv.db_prec != adc_max_dvhg->db_prec &&
		abs(loc_hgdv.db_datatype) == DB_DEC_TYPE)
	    {
		db_stat = adu_error(adf_scb, E_AD3013_BAD_HG_DTPREC, 0);
		break;
	    }
	}
#endif	/* xDEBUG */

	/*
	** Now call the low-level routine to do the work.  Note that if the
	** datatype is nullable, we use a temp DV_DATA_VALUE which gets set
	** to the corresponding base datatype and length.
	*/

	if (adc_fromdv->db_datatype > 0)	/* non-nullable */
	{
	    db_stat = (*Adf_globs->Adi_dtptrs[mbdt]->
			    adi_dt_com_vect.adp_hmax_addr)
				(adf_scb, adc_fromdv, adc_max_dvhg);
	}
	else					/* nullable */
	{
	    DB_DATA_VALUE tmp_dv = *adc_fromdv;

	    tmp_dv.db_datatype = bdt;
	    tmp_dv.db_length--;

	    db_stat = (*Adf_globs->Adi_dtptrs[mbdt]->
			    adi_dt_com_vect.adp_hmax_addr)
				(adf_scb, &tmp_dv, adc_max_dvhg);
	}
	break;
    }	/* end of for stmt */

    return(db_stat);
}


/*
** Name: adc_1dhmax_rti() - Create the "default" maximum histogram element
**			    for a given "RTI known" datatype.
**
** Description:
**	  This routine creates a "default" maximum histogram element
**	for a given "RTI known" datatype.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	adc_fromdv			DB_DATA_VALUE describing the data value
**					for which a maximum "default" histogram
**					value is to be made.
**	    .db_datatype		The datatype to get the maximum
**					default histogram value for.
**      adc_max_dvdhg                   Pointer to the DB_DATA_VALUE
**					which describes the maximum default
**					histogram element to make.
**	    .db_prec			Precision/Scale for the data value
**					being created (will always be 0 for now)
**	    .db_length  		Length for the data value
**					being created.
**	    .db_data    		Pointer to location to place the
**					actual data for the data value
**					being created.
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
**      adc_max_dvhg                    The maximum histogram element.
**	        .db_data    		The actual data for the data
**					value created will be placed
**					here.
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
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      18-jun-86 (ericj)
**          Initial creation.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	28-oct-86 (thurston)
**	    Added char and varchar support.
**	19-apr-89 (mikem)
**	    Logical key development.  Added support for logical_key datatype
**	    to adc_1dhmax_rti() and adc_hmax_rti().
**	05-jul-89 (jrb)
**	    Added decimal support.
**	22-sep-1992 (fred)
**	    Added BIT type(s) support.
**       6-Apr-1993 (fred)
**          Added byte string types.
**	23-Feb-2001 (gupsh01)
**	    Added nchar and nvarchar data types for unicode support.
**      04-Feb-2002 (fanra01)
**          Correct the setting of the DB_NCHR_TYPE and DB_NVCHR_TYPE
**          elements to prevent over stepping the allocated buffer.
**	24-apr-06 (dougi)
**	    Added support for new date/time types.
**	23-mar-2007 (dougi)
**	    Timestamp histogram elements are only i4's.
**	15-may-2007 (dougi)
**	    Add support for UTF8-enabled server.
*/

DB_STATUS
adc_1dhmax_rti(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *adc_fromdv,
DB_DATA_VALUE	    *adc_max_dvdhg)
{
    register u_char	*cp;
    register i4	i;
    DB_STATUS           db_stat = E_DB_OK;
    UCS2 	*cpuni;

    switch(adc_fromdv->db_datatype)
    {
    
      case DB_CHR_TYPE:
        cp = (u_char *) adc_max_dvdhg->db_data;
	if ((adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
		adc_fromdv->db_collID != DB_UCS_BASIC_COLL)
	    goto UTF8merge;
        for (i = adc_max_dvdhg->db_length; i; i--)
	    *cp++ = AD_CHR4_DHMAX_VAL;
	break;

      case DB_CHA_TYPE:
        cp = (u_char *) adc_max_dvdhg->db_data;
	if ((adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
		adc_fromdv->db_collID != DB_UCS_BASIC_COLL)
	    goto UTF8merge;
        for (i = adc_max_dvdhg->db_length; i; i--)
	    *cp++ = AD_CHA4_DHMAX_VAL;
	break;

      case DB_DTE_TYPE:
        *(i4 *) adc_max_dvdhg->db_data = AD_DTE3_HMAX_VAL;
	break;

      case DB_ADTE_TYPE:
        *(i4 *) adc_max_dvdhg->db_data = AD_DTE3_HMAX_VAL;
	break;

      case DB_TME_TYPE:
      case DB_TMW_TYPE:
      case DB_TMWO_TYPE:
        *(i4 *) adc_max_dvdhg->db_data = AD_TME3_HMAX_VAL;
	break;

      case DB_TSTMP_TYPE:
      case DB_TSW_TYPE:
      case DB_TSWO_TYPE:
        *(i4 *) adc_max_dvdhg->db_data = AD_TST3_HMAX_VAL;
	break;

      case DB_INYM_TYPE:
      case DB_INDS_TYPE:
        *(i4 *) adc_max_dvdhg->db_data = AD_INT3_HMAX_VAL;
	break;

      case DB_FLT_TYPE:
	if (adc_max_dvdhg->db_length == 4)
	    *(f4 *) adc_max_dvdhg->db_data = AD_F4_2DHMAX_VAL;
	else if (adc_max_dvdhg->db_length == 8)
	    *(f8 *) adc_max_dvdhg->db_data = AD_F8_2DHMAX_VAL;
	break;

      case DB_DEC_TYPE:
	*(f8 *) adc_max_dvdhg->db_data = AD_DEC2_DHMAX_VAL;
	break;
	
      case DB_BOO_TYPE:
        ((DB_ANYTYPE *)adc_max_dvdhg->db_data)->db_booltype = AD_BOO_DHMAX_VAL;
        break;
	
      case DB_INT_TYPE:
	if (adc_max_dvdhg->db_length == 8)
	    *(i8 *) adc_max_dvdhg->db_data = AD_I4_2DHMAX_VAL;
	else if (adc_max_dvdhg->db_length == 4)
	    *(i4 *) adc_max_dvdhg->db_data = AD_I4_2DHMAX_VAL;
	else if (adc_max_dvdhg->db_length == 2)
	    *(i2 *) adc_max_dvdhg->db_data = AD_I2_2DHMAX_VAL;
	else if (adc_max_dvdhg->db_length == 1)
	    *(i1 *) adc_max_dvdhg->db_data = AD_I1_2DHMAX_VAL;
	break;

      case DB_MNY_TYPE:
        *(i4 *) adc_max_dvdhg->db_data = AD_MNY3_HMAX_VAL;
	break;

      case DB_TXT_TYPE:
        cp = (u_char *) adc_max_dvdhg->db_data;
	if ((adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
		adc_fromdv->db_collID != DB_UCS_BASIC_COLL)
	    goto UTF8merge;
        for (i = adc_max_dvdhg->db_length; i; i--)
	    *cp++ = AD_TXT4_DHMAX_VAL;
	break;
	
      case DB_VCH_TYPE:
        cp = (u_char *) adc_max_dvdhg->db_data;
	if ((adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
		adc_fromdv->db_collID != DB_UCS_BASIC_COLL)
	    goto UTF8merge;
        for (i = adc_max_dvdhg->db_length; i; i--)
	    *cp++ = AD_VCH4_DHMAX_VAL;
	break;

      case DB_LOGKEY_TYPE:
      case DB_TABKEY_TYPE:
        cp = (u_char *) adc_max_dvdhg->db_data;
        for (i = adc_max_dvdhg->db_length; i; i--)
           *cp++ = AD_LOG4_DHMAX_VAL;
        break;

      case DB_BIT_TYPE:
      case DB_VBIT_TYPE:
	cp = (u_char *) adc_max_dvdhg->db_data;
	for (i = adc_max_dvdhg->db_length; i; i--)
	    *cp++ = AD_BIT4_DHMAX_VAL;
	break;

      case DB_BYTE_TYPE:
      case DB_VBYTE_TYPE:
      case DB_NBR_TYPE:
	cp = (u_char *) adc_max_dvdhg->db_data;
	for (i = adc_max_dvdhg->db_length; i; i--)
	    *cp++ = AD_BYTE4_DHMAX_VAL;
	break;

      
      case DB_NCHR_TYPE:
      case DB_NVCHR_TYPE:
UTF8merge:
        cpuni = (UCS2 *) adc_max_dvdhg->db_data;
        /*
        ** Use macro to return the number of UCS2 characters from the length
        ** in octets.
        */
        for (i = II_COUNT_ITEMS(adc_max_dvdhg->db_length, UCS2); i; i--)
            *cpuni++ = AD_NCHR4_DHMAX_VAL;
       break;

      default:
	db_stat = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	break;
    
    }	/* end of switch stmt */

    return(db_stat);
}



/*
** Name: adc_1hmax_rti() -  Create the maximum histogram element
**			    for a given "RTI known" datatype.
**
** Description:
**	  This routine creates a maximum histogram element
**	for a given "RTI known" datatype.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	adc_fromdv			DB_DATA_VALUE describing the data value
**					for which a maximum histogram
**					value is to be made.
**	    .db_datatype		The datatype to get the maximum
**					histogram value for.
**      adc_max_dvhg                    Pointer to the DB_DATA_VALUE
**					which describes the maximum
**					histogram element to make.
**	    .db_prec			Precision/Scale for the data value
**					being created (will always be 0 for now)
**	    .db_length  		Length for the data value
**					being created.
**	    .db_data    		Pointer to location to place the
**					actual data for the data value
**					being created.
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
**      adc_max_dvhg                    The maximum histogram element.
**	    .db_data    		The actual data for the data
**					value created will be placed
**					here.
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
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      18-jun-86 (ericj)
**          Initial creation.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	28-oct-86 (thurston)
**	    Added char and varchar support.
**	19-apr-89 (mikem)
**	    Logical key development.  Added support for logical_key datatype
**	    to adc_1dhmax_rti() and adc_hmax_rti().
**	05-jul-89 (jrb)
**	    Added decimal support.
**	22-sep-1992 (fred)
**	    Added BIT types support.
**       6-Apr-1993 (fred)
**          Added byte string types
**	05-feb-2001 (gupsh01)
**	    Added support for new data type, nchar and nvarchar.
**      04-Feb-2002 (fanra01)
**          Correct the setting of the DB_NCHR_TYPE and DB_NVCHR_TYPE
**          elements to prevent over stepping the allocated buffer.
**	12-may-04 (inkdo01)
**	    Added support for bigint.
**	24-apr-06 (dougi)
**	    Added support for new date/time types.
**	23-mar-2007 (dougi)
**	    Timestamp histogram elements are only i4's.
**	15-may-2007 (dougi)
**	    Add support for UTF8-enabled server.
*/

DB_STATUS
adc_1hmax_rti(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *adc_fromdv,
DB_DATA_VALUE	    *adc_max_dvhg)
{
    register u_char	*cp;
    register i4	i;
    DB_STATUS           db_stat = E_DB_OK;
    UCS2 		*cpuni;

    switch(adc_fromdv->db_datatype)
    {
    
      case DB_CHR_TYPE:
        cp = (u_char *) adc_max_dvhg->db_data;
	if ((adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
		adc_fromdv->db_collID != DB_UCS_BASIC_COLL)
	    goto UTF8merge;
        for (i = adc_max_dvhg->db_length; i; i--)
	    *cp++ = AD_CHR5_HMAX_VAL;
	break;

      case DB_CHA_TYPE:
        cp = (u_char *) adc_max_dvhg->db_data;
	if ((adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
		adc_fromdv->db_collID != DB_UCS_BASIC_COLL)
	    goto UTF8merge;
        for (i = adc_max_dvhg->db_length; i; i--)
	    *cp++ = AD_CHA5_HMAX_VAL;
	break;

      case DB_DTE_TYPE:
        *(i4 *) adc_max_dvhg->db_data = AD_DTE3_HMAX_VAL;
	break;

      case DB_ADTE_TYPE:
        *(i4 *) adc_max_dvhg->db_data = AD_DTE3_HMAX_VAL;
	break;

      case DB_TME_TYPE:
      case DB_TMW_TYPE:
      case DB_TMWO_TYPE:
        *(i4 *) adc_max_dvhg->db_data = AD_TME3_HMAX_VAL;
	break;

      case DB_TSTMP_TYPE:
      case DB_TSW_TYPE:
      case DB_TSWO_TYPE:
        *(i4 *) adc_max_dvhg->db_data = AD_TST3_HMAX_VAL;
	break;

      case DB_INYM_TYPE:
      case DB_INDS_TYPE:
        *(i4 *) adc_max_dvhg->db_data = AD_INT3_HMAX_VAL;
	break;

      case DB_FLT_TYPE:
	if (adc_max_dvhg->db_length == 4)
	    *(f4 *) adc_max_dvhg->db_data = AD_F4_2HMAX_VAL;
	else if (adc_max_dvhg->db_length == 8)
	    *(f8 *) adc_max_dvhg->db_data = AD_F8_2HMAX_VAL;
	break;

      case DB_DEC_TYPE:
	*(f8 *) adc_max_dvhg->db_data = AD_DEC4_HMAX_VAL;
	break;
	
      case DB_BOO_TYPE:
        ((DB_ANYTYPE *)adc_max_dvhg->db_data)->db_booltype = AD_BOO_HMAX_VAL;
        break;
	
      case DB_INT_TYPE:	
	if (adc_max_dvhg->db_length == 8)
	    *(i8 *) adc_max_dvhg->db_data = AD_I8_2HMAX_VAL;
	else if (adc_max_dvhg->db_length == 4)
	    *(i4 *) adc_max_dvhg->db_data = AD_I4_2HMAX_VAL;
	else if (adc_max_dvhg->db_length == 2)
	    *(i2 *) adc_max_dvhg->db_data = AD_I2_2HMAX_VAL;
	else if (adc_max_dvhg->db_length == 1)
	    *(i1 *) adc_max_dvhg->db_data = AD_I1_2HMAX_VAL;
	break;

      case DB_MNY_TYPE:
        *(i4 *) adc_max_dvhg->db_data = AD_MNY3_HMAX_VAL;
	break;

      case DB_TXT_TYPE:
        cp = (u_char *) adc_max_dvhg->db_data;
	if ((adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
		adc_fromdv->db_collID != DB_UCS_BASIC_COLL)
	    goto UTF8merge;
        for (i = adc_max_dvhg->db_length; i; i--)
	    *cp++ = AD_TXT5_HMAX_VAL;
	break;
	
      case DB_VCH_TYPE:
        cp = (u_char *) adc_max_dvhg->db_data;
	if ((adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
		adc_fromdv->db_collID != DB_UCS_BASIC_COLL)
	    goto UTF8merge;
        for (i = adc_max_dvhg->db_length; i; i--)
	    *cp++ = AD_VCH5_HMAX_VAL;
	break;

      case DB_LOGKEY_TYPE:
      case DB_TABKEY_TYPE:
        cp = (u_char *) adc_max_dvhg->db_data;
        for (i = adc_max_dvhg->db_length; i; i--)
            *cp++ = AD_LOG5_HMAX_VAL;
	break;

      case DB_BIT_TYPE:
      case DB_VBIT_TYPE:
	cp = (u_char *) adc_max_dvhg->db_data;
	for (i = adc_max_dvhg->db_length; i; i--)
	    *cp++ = AD_BIT5_HMAX_VAL;
	break;

      case DB_BYTE_TYPE:
      case DB_VBYTE_TYPE:
      case DB_NBR_TYPE:
	cp = (u_char *) adc_max_dvhg->db_data;
	for (i = adc_max_dvhg->db_length; i; i--)
	    *cp++ = AD_BYTE5_HMAX_VAL;
	break;

      case DB_NCHR_TYPE:
      case DB_NVCHR_TYPE:
UTF8merge:
        cpuni = (UCS2 *) adc_max_dvhg->db_data;
        /*
        ** Use macro to return the number of UCS2 characters from the length
        ** in octets.
        */
	for (i = II_COUNT_ITEMS(adc_max_dvhg->db_length, UCS2); i; i--)
	    *cpuni++ = AD_NCHR5_HMAX_VAL;
        break;

      default:
	db_stat = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	break;
    
    }	/* end of switch stmt */

    return(db_stat);
}
