/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cv.h>
#include    <me.h>
#include    <st.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduint.h>
/*
[@#include@]...
*/

#if defined(i64_win)
#pragma optimize("", off)
#endif

/*
** Define the collation name array otherwise
** defined in iicommon.h
** Note -1 origin
*/
static char *coll_array[] = {
#define _DEFINE(n,v,t) t " ",
#define _DEFINEEND
	DB_COLL_MACRO
#undef _DEFINEEND
#undef _DEFINE
};

/**
**
**  Name: ADUTYPENAME.C - Holds routines to process INGRES functions for
**			  datatype names and such.
**
**  Description:
**      This file contains all routines needed to process INGRES functions that
**      have to do with datatype names and such.
**
**          adu_typename() - INGRES's iitypename() function.
**          adu_dvdsc() - INGRES's ii_dv_desc() function.
**          adu_exttype() - INGRES's ii_ext_type() function
**	    adu_extlength() - INGRES's ii_ext_length() function.
[@func_list@]...
**
**  Function prototypes defined in ADUINT.H file.
**
**  History:
**      05-apr-87 (thurston)
**          Initial creation.
**      22-may-87 (thurston)
**          Changed result of adu_typename() to be a CHAR.  Used to be VARCHAR.
**      26-feb-88 (thurston)
**          Added adu_dvdsc() to process the INGRES ii_dv_desc() function.
**	21-apr-89 (mikem)
**	    Logical key support.  Added support for the DB_LOGKEY_TYPE and
**	    the DB_TABKEY_TYPE.
**	28-apr-89 (fred)
**	    Altered to use ADI_DT_MAP_MACRO().
**      22-may-89 (fred)
**	    Added adu_ext{type,length}() for udadt support.
**	05-jun-89 (fred)
**	    Complete changes for ADI_DT_MAP_MACRO();
**      17-jul-89 (fred)
**	    Added comments to adu_extlength() routine.  Corrected various other
**	    comments.  Fixed bug in handling nullable types in ii_extlength().
**	08-aug-91 (seg)
**	    Can't do arithmetic or dereferencing on PTR types.
**      05-jan-1993 (stevet)
**          Added function prototypes.
**      28-jan-1993 (stevet)
**          Modified adu_dvdsc to use datatype information to determine
**          the output format instead of hard code based on datatypes.  Also 
**          added support for all datetypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	11-apr-96 (pchang)
**	    Do not call adc_dbtoev() in adu_exttype() and adu_extlength() if
**	    the candidate datatype is decimal; adc_dbtoev() checks for the 
**	    AD_DECIMAL_PROTO flag to see if decimal data can be exported to the
**	    FE but we are not exporting data here but merely translating the
**	    datatype (B75430).
**	26-dec-2001 (somsa01)
**	    Added NO_OPTIM to i64_win to prevent FE catalog generation during
**	    STAR database creation from causing "E_US1068 Integer overflow
**	    detected in query." when running "register table ii_abfdependencies
**	    as link;\g".
**	21-May-2009 (kiria01) b122051
**	    Reduce uninit'ed collID and generalise the output strings driven
**	    by table macro in iicommon.h
*/


/*
[@forward_type_references@]
[@forward_function_references@]
[@#defines_of_other_constants@]
[@type_definitions@]
[@global_variable_definitions@]
[@static_variable_or_function_definitions@]
*/


/*{
** Name: adu_typename() - INGRES's iitypename() function.
**
** Description:
**      INGRES's iitypename() function.  Given a datatype id, this function
**	returns the preferred name for the datatype.  (e.g. iitypename(10)
**	returns "decimal")
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1				Ptr to DB_DATA_VALUE for first param.
**	    .db_datatype		Datatype; assumed to be DB_INT_TYPE.
**	    .db_length			Length.
**	    .db_data			Ptr to the data of first param.
**      rdv				Pointer DB_DATA_VALUE for the result.
**	    .db_datatype		Datatype.
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
**      rdv				Pointer DB_DATA_VALUE for the result.
**	    .db_data			Result will be placed here.
**
**	Returns:
**	    {@return_description@}
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      05-apr-87 (thurston)
**          Initial creation.
**      22-may-87 (thurston)
**          Changed result to be a CHAR.  Used to be VARCHAR.
**      26-feb-88 (thurston)
**          Rewrote to use the datatype pointers table; more efficient.
**      23-may-2007 (smeke01) b118342/b118344
**	    Work with i8.
**	8-Jul-2008 (kibro01) b120584
**	    Translate ingresdate to date for pre-2006r2 clients
[@history_template@]...
*/

DB_STATUS
adu_typename(
ADF_CB             *adf_scb,
DB_DATA_VALUE      *dv1,
DB_DATA_VALUE      *rdv)
{
    DB_DT_ID		dt;
    ADI_DATATYPE	*dtp;
    i8		        i8temp;

    switch (dv1->db_length)
    {
	case 1:
	    dt = (DB_DT_ID) I1_CHECK_MACRO( *(i1 *) dv1->db_data);
	    break;
	case 2: 
	    dt = (DB_DT_ID) *(i2 *) dv1->db_data;
	    break;
	case 4:
	    dt = (DB_DT_ID) *(i4 *) dv1->db_data;
	    break;
	case 8:
	    i8temp  = *(i8 *)dv1->db_data;

	    /* limit to i4 values */
	    if ( i8temp > MAXI4 || i8temp < MINI4LL )
		return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "typename dt overflow"));

	    dt = (DB_DT_ID) i8temp;
	    break;
	default:
	    return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "typename dt length"));
    }

    dt = ADI_DT_MAP_MACRO(abs(dt));

    if (dt > 0 && dt <= ADI_MXDTS
	&& (dtp = Adf_globs->Adi_dtptrs[dt]) != NULL)
    {
	if (dtp->adi_dtstat_bits & AD_INDB)
	{
#define ingres_prefix	"ingres"
	    if (abs(dt) == DB_DTE_TYPE &&
		((adf_scb->adf_proto_level & AD_ANSIDATE_PROTO) == 0) &&
		(STbcompare(dtp->adi_dtname.adi_dtname, STlength(ingres_prefix),
				ingres_prefix, STlength(ingres_prefix),
				FALSE) == 0))
	    {
		/* skip over "ingres" part of "ingresdate", leaving any
		** "with null" (kibro01) b120584
		*/
	        STmove( (char *) dtp->adi_dtname.adi_dtname + 
				STlength(ingres_prefix),
		    (char)   ' ',
		    rdv->db_length,
		    (char *) rdv->db_data
		  );
	    } else
	    {
	        STmove( (char *) dtp->adi_dtname.adi_dtname,
		    (char)   ' ',
		    rdv->db_length,
		    (char *) rdv->db_data
		  );
	    }

	    return (E_DB_OK);
	}
    }

    MEfill( rdv->db_length,
	    (u_char) ' ',
	    (PTR)    rdv->db_data
	  );

    return (E_DB_OK);
}


/*{
** Name: adu_dvdsc() - INGRES's ii_dv_desc() function.
**
** Description:
**      INGRES's ii_dv_desc() function.  This function produces a description
**	of the datavalue handed to it.  For example, ii_dv_desc(i4_column with
**	nulls)
**	would produce "nullable integer4".
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1				Ptr to DB_DATA_VALUE for first param.
**	    .db_datatype		Datatype.
**	    .db_prec			Precision/Scale
**	    .db_length			Length.
**	    .db_data			Ptr to the data of first param.
**      rdv				Pointer DB_DATA_VALUE for the result.
**	    .db_datatype		Datatype.
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
**      rdv				Pointer DB_DATA_VALUE for the result.
**	    .db_data			Result will be placed here.
**
**	Returns:
**	    {@return_description@}
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      26-feb-88 (thurston)
**          Initial creation.
**	21-apr-89 (mikem)
**	    Logical key support.  Added support for the DB_LOGKEY_TYPE and
**	    the DB_TABKEY_TYPE.
**	15-jul-89 (jrb)
**	    Added decimal to list of supported types.  Instead of length
**	    decimal shows precision and scale (e.g. "decimal(10, 5)")  Also,
**	    cleaned up this routine a bit.
**      28-jan-1993 (stevet)
**          Modified adu_dvdsc to use datatype information to determine
**          the output format instead of hard code based on datatypes.  Also 
**          added support for all datetypes.
**	13-june-05 (inkdo01)
**	    Add support for column level collation.
**      29-Oct-2007 (hanal04) Bug 119368
**          If the front end does not have GCA i8 support then PSF will
**          cast the RESDOMs as i4s. Update ii_dv_desc() output accordingly.
**	14-oct-2007 (gupsh01)
**	    Pass in the type of source string to the adu_movestring routine, 
**	    in order to allow for binary data to be copied untainted. Since the 
**	    character input to movestring was created in this routine itself 
**	    we pass in DB_NODT to this routine.
**      26-Nov-2009 (hanal04) Bug 122938
**          If the front end does not support the maximum precision
**          for decimals then PSF will downgrade/convert the RESDOMs to the 
**          supported maximum precision level. Update ii_dv_desc() output 
**          accordingly.
**       7-Dec-2009 (hanal04) Bug 123019, 122938
**          Adjust previous change for Bug 122938. Scale only needs to be
**          adjusted if it is greater than the new precision. This prevents
**          negative scale values seen in replicator testing.
**      25-Feb-2010 (hanal04) Bug 123291
**          Remove downgrade of decimal RESDOMs, old clients can display
**          precision in excess of 31, they just can't use them in ascii
**          copy.
[@history_template@]...
*/

DB_STATUS
adu_dvdsc(
ADF_CB             *adf_scb,
DB_DATA_VALUE      *dv1,
DB_DATA_VALUE      *rdv)
{
    char		buf[64];
    char		*cptr = &buf[0];
    ADI_DATATYPE	*dtp;
    ADI_COLUMN_INFO	*col_info;
    DB_DT_ID		dt = dv1->db_datatype;
    i4			bdt = abs(dt);
    i4			blen = dv1->db_length;
    i4			outlen = 0;
    DB_STATUS		db_stat;
    i4			mbdt;

    mbdt = ADI_DT_MAP_MACRO(bdt);
    dtp = Adf_globs->Adi_dtptrs[mbdt];
    col_info = &dtp->adi_column_info;
    
    if (dt < 0)
    {
	blen--;
	STcopy("nullable ", cptr);
	cptr += 9;
    }
    STcopy((char *) dtp->adi_dtname.adi_dtname, cptr);
    cptr += STlength((char *) dtp->adi_dtname.adi_dtname);

    if( col_info->adi_embed_allowed == TRUE)
    {
        if ( !(adf_scb->adf_proto_level & AD_I8_PROTO) && 
              (abs(dt) == DB_INT_TYPE) &&
              blen == sizeof(i8) )
        {
            CVna(4, cptr);
            cptr += STlength(cptr);
            STcopy(" cast from ", cptr);
            cptr += 11;
            STcopy((char *) dtp->adi_dtname.adi_dtname, cptr);
            cptr += STlength((char *) dtp->adi_dtname.adi_dtname);
        }

	CVna(blen, cptr);
	cptr += STlength(cptr);

    }
    /*
    ** UDT will always be displayed with parameter since there is no
    ** way to distinguish variable length and fix length UDT.  For
    ** fix length UDT, the datatype name will be displayed with 0 length
    ** (i.e. ord_pair(0)) which is a legal specificiation for fix length
    ** UDT
    */
    else if( col_info->adi_paren_allowed == TRUE || 
	    (dtp->adi_dtstat_bits & AD_USER_DEFINED))
    {
	if( bdt == DB_DEC_TYPE)
	{
            i4 prec;
            i4 scale;

            prec = (i4)DB_P_DECODE_MACRO(dv1->db_prec);
           
            scale = (i4)DB_S_DECODE_MACRO(dv1->db_prec);

	    *cptr++ = '(';
	    CVna(prec, cptr);
	    cptr += STlength(cptr);
	    *cptr++ = ',';
	    *cptr++ = ' ';
	    CVna(scale, cptr);
	    cptr += STlength(cptr);
	    *cptr++ = ')';
	}
	else
	{
	    DB_DATA_VALUE  tmp_dv;
	    
	    /* call adc_lenchk to find the declared length */
	    db_stat = adc_lenchk( adf_scb, FALSE, dv1, &tmp_dv);

	    if( db_stat != E_DB_OK)
		return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	    *cptr++ = '(';
	    CVna(tmp_dv.db_length, cptr);
	    cptr += STlength(cptr);
	    *cptr++ = ')';
	}
    }

    if (!DB_COLL_UNSET(*dv1) && dv1->db_collID > DB_NOCOLLATION)
    {
	STcopy(" collate ", cptr);
	cptr += 9;
	STcopy((char *) coll_array[dv1->db_collID+1], cptr);
	cptr += STlength((char *) coll_array[dv1->db_collID+1]);
    }

	
    if (rdv->db_datatype < 0)
    {
	DB_DATA_VALUE	    tmp_dv;

	ADF_CLRNULL_MACRO(rdv);
	STRUCT_ASSIGN_MACRO(*rdv, tmp_dv);
	tmp_dv.db_datatype = abs(tmp_dv.db_datatype);
	tmp_dv.db_length--;
	db_stat = adu_movestring(adf_scb, (u_char *)buf, cptr-buf, DB_NODT, &tmp_dv);
    }
    else
    {
	db_stat = adu_movestring(adf_scb, (u_char *)buf, cptr-buf, DB_NODT, rdv);
    }

    return (db_stat);
}

/*{
** Name: adu_exttype	- Return id of external type for datatype
**
** Description:
**      This routine returns the id of the external type used to represent an
**	unexportable type to the outside world. 
**
** Inputs:
**      scb                             Adf scb
**      dv_type                         A datavalue for an integer describing
**					the type
**	dv_length			A datavalue for an integer describing
**					the length
**
** Outputs:
**      *rdv                            Datavalue for integer fill with type id.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      22-may-89 (fred)
**          Created.
**	08-aug-91 (seg)
**	    Can't do arithmetic or dereferencing on PTR types.
**	11-apr-96 (pchang)
**	    Do not call adc_dbtoev() if the candidate datatype is decimal; 
**	    adc_dbtoev() checks for the AD_DECIMAL_PROTO flag to see if decimal
**	    data can be exported to the FE but we are not exporting data here 
**	    but merely returning the external id of the datatype (B75430).
**      23-may-2007 (smeke01) b118342/b118344
**	    Work with i8.
[@history_template@]...
*/
DB_STATUS
adu_exttype(
ADF_CB             *scb,
DB_DATA_VALUE      *dv_type,
DB_DATA_VALUE      *dv_length,
DB_DATA_VALUE	   *rdv)
{
    DB_DATA_VALUE		ev_value;
    DB_DATA_VALUE		dt_value;
    DB_DT_ID			dt;
    DB_DT_ID			mdt;
    i4			len;
    DB_STATUS			status = E_DB_OK;
    ADI_DATATYPE		*dtp;
    i8				i8temp;

    STRUCT_ASSIGN_MACRO(*dv_type, ev_value);
    switch (dv_type->db_length)
    {
	case 1:
	    dt = (DB_DT_ID) I1_CHECK_MACRO( *(i1 *) dv_type->db_data);
	    break;
	case 2: 
	    dt = (DB_DT_ID) *(i2 *) dv_type->db_data;
	    break;
	case 4:
	    dt = (DB_DT_ID) *(i4 *) dv_type->db_data;
	    break;
	case 8:
	    i8temp  = *(i8 *)dv_type->db_data;

	    /* limit to i4 values */
	    if ( i8temp > MAXI4 || i8temp < MINI4LL )
		return (adu_error(scb, E_AD9998_INTERNAL_ERROR, 2, 0, "exttype dt overflow"));

	    dt = (DB_DT_ID) (i4) i8temp;
	    break;
	default:
	    return (adu_error(scb, E_AD9998_INTERNAL_ERROR, 2, 0, "exttype dt length"));
    }

    switch (dv_length->db_length)
    {
	case 1:
	    len = I1_CHECK_MACRO( *(i1 *) dv_length->db_data);
	    break;
	case 2: 
	    len = *(i2 *) dv_length->db_data;
	    break;
	case 4:
	    len = *(i4 *) dv_length->db_data;
	    break;
	case 8:
	    i8temp  = *(i8 *)dv_length->db_data;

	    /* limit to i4 values */
	    if ( i8temp > MAXI4 || i8temp < MINI4LL )
		return (adu_error(scb, E_AD9998_INTERNAL_ERROR, 2, 0, "exttype len overflow"));

	    len = (i4) i8temp;
	    break;
	default:
	    return (adu_error(scb, E_AD9998_INTERNAL_ERROR, 2, 0, "exttype len length"));
    }

    mdt = ADI_DT_MAP_MACRO(abs(dt));
    ev_value.db_datatype = dt;

    if (mdt > 0 && mdt <= ADI_MXDTS
	&& (dtp = Adf_globs->Adi_dtptrs[mdt]) != NULL)
    {
	if (dtp->adi_dtstat_bits & AD_INDB)
	{
	    if (dtp->adi_dtstat_bits & (AD_NOEXPORT | AD_CONDEXPORT))
	    {
		if (mdt == DB_DEC_TYPE)
		{
		    ev_value.db_datatype = dt;
		    ev_value.db_length = len;
		    ev_value.db_prec = 0;
		}
		else
		{
		    dt_value.db_datatype = dt;
		    dt_value.db_length = len;
		    dt_value.db_prec = 0;

		    status = adc_dbtoev(scb, &dt_value, &ev_value);
		}
	    }
	}
    }
    switch (rdv->db_length)
    {
	case 1:
	    (* (i1 *) rdv->db_data) = (i1) ev_value.db_datatype;
	    break;
	case 2:
	{
	    i2	    i = ev_value.db_datatype;
	    
	    I2ASSIGN_MACRO(i, *(char *)rdv->db_data);
	    break;
	}
	case 4:
	{
	    i4	    i = ev_value.db_datatype;
	    
	    I4ASSIGN_MACRO(i, *(char *)rdv->db_data);
	    break;
	}
    }
    return(status);
}


/*{
** Name: adu_extlength	- Return external length for datatype
**
** Description:
**      This routine returns the external length used to represent an
**	unexportable type to the outside world. 
**
** Inputs:
**      scb                             Adf scb
**      dv_type                         A datavalue for an integer describing
**					the type
**	dv_length			A datavalue for an integer describing
**					the length
**
** Outputs:
**      *rdv                            Datavalue for integer to fill with type
**					    the length.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      22-may-89 (fred)
**          Created.
**      17-jul-89 (fred)
**          Fixed bug in handling nullable types as input.  The type, nullable
**	    and all, should be sent on to adc_dbtoev() since that routine
**	    correctly manages the nullability.  If it is not sent on, then
**	    nullability will not be correctly sent, and our caller will assume
**	    that it has.  This results in converted types length being off when
**	    they are null.
**	26-may-93 (robf)(
**	    Fixed bug in handling nullable types on input. check for mdt>0
**	    made nullable types (<0) never get sent to adc_dbtoev.
**	08-aug-91 (seg)
**	    Can't do arithmetic or dereferencing on PTR types.
**	11-apr-96 (pchang)
**	    Do not call adc_dbtoev() if the candidate datatype is decimal; 
**	    adc_dbtoev() checks for the AD_DECIMAL_PROTO flag to see if decimal
**	    data can be exported to the FE but we are not exporting data here 
**	    but merely returning the external length of the datatype (B75430). 
**      23-may-2007 (smeke01) b118342/b118344
**	    Work with i8.
[@history_template@]...
*/
DB_STATUS
adu_extlength(
ADF_CB            *scb,
DB_DATA_VALUE      *dv_type,
DB_DATA_VALUE      *dv_length,
DB_DATA_VALUE	   *rdv)
{
    DB_DATA_VALUE		ev_value;
    DB_DATA_VALUE		dt_value;
    DB_DT_ID			dt;
    DB_DT_ID			mdt;
    i4			len;
    DB_STATUS			status;
    ADI_DATATYPE		*dtp;
    i8		        i8temp;

    switch (dv_type->db_length)
    {
	case 1:
	    dt = (DB_DT_ID) I1_CHECK_MACRO( *(i1 *) dv_type->db_data);
	    break;
	case 2: 
	    dt = (DB_DT_ID) *(i2 *) dv_type->db_data;
	    break;
	case 4:
	    dt = (DB_DT_ID) *(i4 *) dv_type->db_data;
	    break;
	case 8:
	    i8temp  = *(i8 *)dv_type->db_data;

	    /* limit to i4 values */
	    if ( i8temp > MAXI4 || i8temp < MINI4LL )
		return (adu_error(scb, E_AD9998_INTERNAL_ERROR, 2, 0, "extlength dt overflow"));

	    dt = (DB_DT_ID) (i4) i8temp;
	    break;
	default:
	    return (adu_error(scb, E_AD9998_INTERNAL_ERROR, 2, 0, "extlength dt length"));
    }

    switch (dv_length->db_length)
    {
	case 1:
	    len = I1_CHECK_MACRO( *(i1 *) dv_length->db_data);
	    break;
	case 2: 
	    len = *(i2 *) dv_length->db_data;
	    break;
	case 4:
	    len = *(i4 *) dv_length->db_data;
	    break;
	case 8:
	    i8temp  = *(i8 *)dv_length->db_data;

	    /* limit to i4 values */
	    if ( i8temp > MAXI4 || i8temp < MINI4LL )
		return (adu_error(scb, E_AD9998_INTERNAL_ERROR, 2, 0, "extlength len overflow"));

	    len = (i4) i8temp;
	    break;
	default:
	    return (adu_error(scb, E_AD9998_INTERNAL_ERROR, 2, 0, "extlength len length"));
    }

    ev_value.db_length = (i2) len;
    mdt = ADI_DT_MAP_MACRO(abs(dt));

    if (mdt > 0 && mdt <= ADI_MXDTS
	&& (dtp = Adf_globs->Adi_dtptrs[mdt]) != NULL)
    {
	if (dtp->adi_dtstat_bits & AD_INDB)
	{
	    if (dtp->adi_dtstat_bits & (AD_NOEXPORT | AD_CONDEXPORT))
	    {
		if (mdt == DB_DEC_TYPE)
		{
		    ev_value.db_datatype = dt;
		    ev_value.db_length = len;
		    ev_value.db_prec = 0;
		}
		else
		{
		    dt_value.db_datatype = dt;
		    dt_value.db_length = len;
		    dt_value.db_prec = 0;

		    status = adc_dbtoev(scb, &dt_value, &ev_value);

		    if (status)
			return(status);
		}
	    }
	}
    }

    switch (rdv->db_length)
    {
	case 1:
	    (* (i1 *) rdv->db_data) = (i1) ev_value.db_length;
	    break;
	case 2:
	{
	    i2	    i = ev_value.db_length;
	    
	    I2ASSIGN_MACRO(i, *(char *)rdv->db_data);
	    break;
	}
	case 4:
	{
	    i4	    i = ev_value.db_length;
	    
	    I4ASSIGN_MACRO(i, *(char *)rdv->db_data);
	    break;
	}
    }
	    
    return(E_DB_OK);
}
/*
[@function_definition@]...
*/
