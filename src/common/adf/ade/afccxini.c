/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#define	    ADE_FRONTEND

#include	<compat.h>
#include	<gl.h>
#include	<sl.h>
#include	<iicommon.h>
#include	<adf.h>
#include	<ade.h>
#include	<adefebe.h>
#include	<adeint.h>


/**
** Name:  AFCCXINI.C - Initialize a CXHEAD for the front-ends
**
** Description:
**	This file contains a routine to initialize a CXHEAD structure
**	for use by the front-ends.
**
** Defines:
**	afc_cxhead_init()
**
**/


/*{
** Name:    afc_cxhead_init()    -    Initialize a CXHEAD for the front-ends
**
** Description:
**    This routine initializes a CXHEAD structure for use by the
**    front-ends.
**
** Inputs:
**    adf_scb			Pointer to an ADF session control block.
**        .adf_errcb		ADF_ERROR struct.
**             .ad_ebuflen	The length, in bytes, of the buffer
**				pointed to by ad_errmsgp.
**             .ad_errmsgp	Pointer to a buffer to put formatted
**				error message in, if necessary.
**    afc_cxhd			Pointer to space allocated for a CX header.
**    afc_veraln		Pointer to a VERALN structure.
**        .afc_hi_version	Major version for a CX.
**        .afc_lo_version	Minor version for a CS.
**        .afc_max_align	Alignment used to compile the CX.
**    offset_array		Array of 1st and last offsets for each
**				of the 3 CX segments.
**
**
** Outputs:
**    adf_scb			Pointer to an ADF session control block.
**        .adf_errcb		ADF_ERROR struct.  If an error occurs
**				the following fields will be set.
**				NOTE:  if .ad_ebuflen = 0 or
**				.ad_errmsgp = NULL, no error message
**				will be formatted.
**            .ad_errcode	ADF error code for the error.
**            .ad_errclass	Signifies the ADF error class.
**            .ad_usererr	If .ad_errclass is ADF_USER_ERROR,
**				this field is set to the corresponding
**				user error which will either map to
**				an ADF error code or a user-error code.
**            .ad_emsglen	The length, in bytes, of the resulting
**				formatted error message.
**            .adf_errmsgp	Pointer to the formatted error message.
**    afc_cxhead		Pointer to a CX header.
**        .cx_hi_version	Upper half of what version of the CX this is.
**        .cx_lo_version	Lower half of what version of the CX this is.
**        .cx_mi_last_offset	Offset (in i2's) from base[ADE_CXBASE]
**				of the last MAIN instruction.
**        .cx_hi_base		The highest numbered base used.
**        .cx_alignment		The alignment restriction on this machine.
**
**    Returns:
**        E_DB_OK
**
**    Exceptions:
**        none
**
** Side Effects:
**        The CXHEAD will be initialized by this routine.
**
** History:
**        25-mar-87 (agh)
**		First written.
**        29-dec-1992 (stevet)
**              Added function prototype.
**
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	17-Feb-2005 (schka24)
**	    minor changes for cx segment struct.
*/

# ifdef ADF_BUILD_WITH_PROTOS
DB_STATUS
afc_cxhead_init(
ADF_CB		*adf_scb,
PTR		afc_cxhd,
AFC_VERALN	*afc_veraln,
i4		*offset_array)
# else
DB_STATUS
afc_cxhead_init( adf_scb, afc_cxhd, afc_veraln, offset_array)
ADF_CB		*adf_scb;
PTR		afc_cxhd;
AFC_VERALN	*afc_veraln;
i4		*offset_array;
# endif
{
	register ADE_CXHEAD	*cxhead = (ADE_CXHEAD *) afc_cxhd;
	register i4		*optr = offset_array;

	cxhead->cx_hi_version = afc_veraln->afc_hi_version;
	cxhead->cx_lo_version = afc_veraln->afc_lo_version;
	cxhead->cx_allocated = 0;
	cxhead->cx_bytes_used = 0;
	cxhead->cx_seg[ADE_SINIT].seg_1st_offset = *optr++;
	cxhead->cx_seg[ADE_SINIT].seg_last_offset = *optr++;
	cxhead->cx_seg[ADE_SMAIN].seg_1st_offset = *optr++;
	cxhead->cx_seg[ADE_SMAIN].seg_last_offset = *optr++;
	cxhead->cx_seg[ADE_SFINIT].seg_1st_offset = *optr++;
	cxhead->cx_seg[ADE_SFINIT].seg_last_offset = *optr++;
	cxhead->cx_k_last_offset = 0;
	cxhead->cx_seg[ADE_SINIT].seg_n = 0;
	cxhead->cx_seg[ADE_SMAIN].seg_n = 0;
	cxhead->cx_seg[ADE_SFINIT].seg_n = 0;
	cxhead->cx_seg[ADE_SVIRGIN].seg_n = 0;
	cxhead->cx_n_k = 0;
	cxhead->cx_n_vlts = 0;
	cxhead->cx_hi_base = 0;
	cxhead->cx_alignment = afc_veraln->afc_max_align;
	return (E_DB_OK);
}
