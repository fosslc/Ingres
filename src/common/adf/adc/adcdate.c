/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved
*/

#include    <compat.h>
#include    <dbms.h>
#include    <adf.h>
#include    <adudate.h>
#include    <ulf.h>

#include    <adfint.h>
#include    <aduint.h>
/*
** Name:	adcdate.c
**
** Description:
**	Externally visible date operations. This file contains date operations
**	visible outside  ADF, typically in PSF where the context of a
**	date needs to be checked
**
**	The following routines are provided:
**
**	adc_date_chk	- Checks if date is absolute, interval or empty
**
**	adc_date_add	- Adds two dates
**
** History
**    9-jul-93 (robf)
**	  Created
**    26-feb-96 (thoda04)
**	  Added aduint.h to include function prototype for adu_1date_addu
**	13-feb-1998 (walro03)
**	    aduadtdate.h and adudate.h are redundant.  aduadtdate.h deleted and
**	    references (such as here) changed.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** Name: adc_date_chk
**
** Description:
**	Checks whether a date is absolute, interval or empty.
**
** Inputs:
**	scb		SCB,
**
**	dateval		Ingres  date
**
** Outputs:
**	result		1==Absolute, 2==Interval, 0==Empty 
**
** Returns
**	DB_STATUS
**
** History:
**    9-jul-93 (robf)
**	Created
*/
DB_STATUS
adc_date_chk( 
ADF_CB	      *adf_scb,
DB_DATA_VALUE *dateval,
i4	      *result
)
{
	AD_DATENTRNL *dn;
	if(!dateval || dateval->db_datatype!=DB_DTE_TYPE)
	{
		return(adu_error(adf_scb, E_AD2004_BAD_DTID, 0));
	}
	/*
	** Check type
	*/
	dn= (AD_DATENTRNL*)dateval->db_data;

	if(dn->dn_status&AD_DN_ABSOLUTE)
		*result=1;
	else if(dn->dn_status&AD_DN_LENGTH)
		*result=2;
	else
		*result=0;
	
	return E_DB_OK;
}

/*
** Name: adc_date_add
**
** Description:
**	Adds two dates. This is essentially an externally-visible interface
**	to adu_1date_add().
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             DB_DATA_VALUE describing the first date
**					operand.
**	    .db_data			Ptr to AD_DATENTRNL struct.
**		.dn_status		A character whose bits describe the
**					internal date format.
**		.dn_highday		A character for large day intervals.
**		.dn_year		The date's year.
**		.dn_month		The date's month.
**		.dn_lowday		If the date is absolute, this is the
**					day of the month.  If the date is an
**					interval, this is the low-order part
**					of the number of days in the interval.
**		.dn_time		The time in milliseconds from 00:00.
**	dv2				DB_DATA_VALUE describing the second
**					date operand.  The fields are the same
**					as listed for dv1
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
**	rdv				DB_DATA_VALUE describing the result
**					date operand.  The fields are the same
**					as listed for dv1.
**
** Returns
**	DB_STATUS
**
** History:
**    9-jul-93 (robf)
**	Created
*/
DB_STATUS
adc_date_add( 
ADF_CB	      *adf_scb,
DB_DATA_VALUE  *dv1,
DB_DATA_VALUE  *dv2,
DB_DATA_VALUE  *rdv)
{
	return adu_1date_addu( adf_scb, dv1, dv2, rdv);
}
