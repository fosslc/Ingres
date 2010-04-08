/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <gl.h>
#include    <sl.h>
#include    <st.h>
#include    <cm.h>
#include    <sl.h>
#include    <mh.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <adulcol.h>
#include    <aduucol.h>
#include    <aduint.h>

/**
**
**  Name: ADTVVCMP.C - Compare a row (tuple) with a partition value.
**
**  Description:
**	This file contains all of the routines necessary to perform the
**	external ADF call "adt_vtupcmp()" which compares the contents of
**	a row with a partition value.
**
**      This file defines:
**
**          adt_tupvcmp() - Compare a row with a partition value.
**
**
**  History:
**	5-Jan-2004 (schka24)
**	    Create by cloning adtvvcmp.c
**	2-Feb-2004 (schka24)
**	    Try comparing different columns instead of the same one
**	    over and over.
**/


/*{
** Name: adt_tupvcmp() - Compare a row with a partition value.
**
** Description:
**	This routine is used to compare the contents of a row with a
**	partition value.
**
**	A "partition value" is a constant in a particular form as
**	described by a partition-list entry and a value base address.
**	The value may be a composite, multi-column value.
**	The partition list says what data type(s) exist in the value,
**	and where each component resides within the value area.
**	It also says where the corresponding values are within the
**	row image.  (The initial partition-list built by RDF has the
**	appropriate column offsets within a base row;  nothing
**	precludes a caller from fudging up a partition-list with an
**	altered set of row-offsets, though.)
**	The caller supplies one partition list, its length (ie the number
**	of value components), an ALIGNED partition value address,
**	and a (not necessarily aligned) row base address.
**
**	To avoid the overhead of a function call for each attribute in the
**	tuples to be compared, this routine has built into it the semantics for
**	comparing several of the most common datatypes supported by INGRES.
**	If the routine does run across a datatype that is not in this list, it
**	will call the general routine "adc_compare()", thereby guaranteeing
**	that it will function (perhaps slower), for ALL datatypes, even future
**	user defined ADTs.
**
**	One significant difference between this routine and the similar
**	one adt_keytupcmp is that we have the opportunity to lay out the
**	partition value offsets ahead of time.  Therefore given that the
**	input value pointer is aligned to an ALIGN_RESTRICT boundary,
**	all data items will also be aligned to an appropriate boundary.
**	(This only applies to the partition value, NOT to the row area.)
**
**	When NULLs are involved, this routine will use the same semantics
**	that "adc_compare()" uses:  a NULL will be considered greater
**	than any other value, and equal to another NULL.
**
**	PLEASE NOTE:  As with "adc_compare()", no pattern matching for
**	-----------   the string datatypes is done within this routine.
**
**
** Inputs:
**	adfcb			Pointer to an ADF session control block.
**	    .adf_errcb		ADF_ERROR struct.
**		.ad_ebuflen	The length, in bytes, of the buffer
**				pointed to by ad_errmsgp.
**		.ad_errmsgp	Pointer to a buffer to put formatted
**				error message in, if necessary.
**      ncols			Number of columns (components) in a value.
**	part_list		Pointer to a DB_PART_LIST array, with
**				one entry per value component (ie ncols
**				entries).  Each part-list entry describes
**				the type and offset of a component.
**	row_ptr			Pointer to start of row
**	pv2			*Aligned* pointer to composite value
**
** Outputs:
**	adfcb			Pointer to an ADF session control block.
**	    .adf_errcb		ADF_ERROR struct.  If an
**				error occurs the following fields will
**				be set.  NOTE: if .ad_ebuflen = 0 or
**				.ad_errmsgp = NULL, no error message
**				will be formatted.
**		.ad_errcode	ADF error code for the error.
**		.ad_errclass	Signifies the ADF error class.
**		.ad_usererr	If .ad_errclass is ADF_USER_ERROR,
**				this field is set to the corresponding
**				user error which will either map to
**				an ADF error code or a user-error code.
**		.ad_emsglen	The length, in bytes, of the resulting
**				formatted error message.
**		.adf_errmsgp	Pointer to the formatted error message.
**      adt_cmp_result		Result of comparison.  This is
**				guaranteed to be:
**				    < 0   if row < val
**				    = 0   if row = val
**				    > 0   if row > val
**
**      Returns:
**	    The usual DB_STATUS status code
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	5-Jan-2004 (schka24)
**	    Copy from adt_vvcmp.
**	16-dec-04 (inkdo01)
**	    Add collation ID parm to aduucmp() calls.
**	02-Feb-2005 (jenjo02)
**	    Changed to call (new) adt_compare function for
**	    each attribute, return real status.
*/

DB_STATUS
adt_tupvcmp(
	ADF_CB		*adfcb,
	i4		ncols,		/* Number attrs in key. */
	DB_PART_LIST	*part_list,	/* Column descriptor array */
	PTR		row_ptr,	/* Input row base area */
	PTR		pv2,		/* Value base area */
	i4		*adt_cmp_result)	/* Place to put cmp result. */
{
    DB_ATTS		atr;
    DB_STATUS		status;

    status = E_DB_OK;
    *adt_cmp_result = 0;

    while ( --ncols >= 0 && status == E_DB_OK && *adt_cmp_result == 0 )
    {
	/* Construct a DB_ATTS for adt_compare */
	atr.type = part_list->type;
	atr.length  = part_list->length;
	atr.precision = part_list->precision;
	atr.collID = part_list->collationID;

	*adt_cmp_result = adt_compare(adfcb,
	   			      &atr,
				      (char*)row_ptr + part_list->row_offset,
				      (char*)pv2 + part_list->val_offset,
				      &status);
	part_list++;
    }

    return(status);
}
