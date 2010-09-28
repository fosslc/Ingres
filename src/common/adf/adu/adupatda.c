/*
** Copyright (c) 2009 Ingres Corporation
**
*/

/**
**
**  Name: adupatda.c - Routines related to data access for pattern matching
**
**  Description:
**	This file contains routines related to getting data. To simplify
**	things the access to simple objects is presented in the same way as
**	that of long ones, except that the second call to get will never
**	be made because the caller will test the EOF state before hand.
**
**	The one awkward bit relates to what happens when multi-byte data
**	straddles a data block boundary. To explicitly handle this in the
**	state machine would be awefully messy and very error prone. Instead
**	we choose to reject such data until complete. What this means is
**	that the very first PAT_CTX thread that notes the incomplete char
**	due to straddling, it will pull back the working .bufend pointer
**	to the character start and then stall. No other thread will see
**	the part-char until processing the next buffer full when the part
**	char should have been completed by the next data read. In this
**	context, we could be talking strict multi-byte characters or it
**	could be several characters that form part of a collation grouping.
**	Either way, the part data will be copied to prefix the next data.
**	(Unicode does not have this problem and it needs to be long data)
**
** This file defines the following externally visible routines:
**
**	adu_patda_init() - Make preparations
**	adu_patda_get() - Return next data segment
**	adu_patda_term() - Clean up
**
**  History:
**      04-Apr-2008 (kiria01)
**          Initial creation.
**  16-Jun-2009 (thich01)
**      Treat GEOM type the same as LBYTE.
**  20-Aug-2009 (thich01)
**      Treat all spatial types the same as LBYTE.
**       8-Mar-2010 (hanal04) Bug 122974
**          Removed nfc_flag from adu_nvchr_fromutf8() and adu_unorm().
**          A standard interface is expected by fcn lookup / execute
**          operations. Force NFC normalization is now achieved by temporarily
**          updating the adf_uninorm_flag in the ADF_CB.
**      21-Jun-2010 (horda03) b123926
**          Because adu_unorm() and adu_utf8_unorm() are also called via 
**          adu_lo_filter() change parameter order.
**/

#include <compat.h>
#include <gl.h>
#include <cm.h>
#include <st.h>
#include <id.h>
#include <me.h>
#include <iicommon.h>
#include <ulf.h>
#include <adf.h>
#include <adfint.h>
#include <aduint.h>
#include <nm.h>

#include "adupatexec.h"

/*		*********************
**		*** Big UTF8 NOTE ***
**		*********************
** This module is designed so that it never need process UTF8 chars itself
** as they will have been converted to NVARCHARs and handled with them.
** Therefore to remove the redundant per-character UTF8 overheads - they
** have been short-circuited below.
*/
#ifdef CMGETUTF8
#undef CMGETUTF8
#endif
#define CMGETUTF8 0

/*
** LEFTOVER_PFX_LEN - number of extra bytes in work buffer
** to allow prefixing with leftover part characters
*/
#define LEFTOVER_PFX_LEN 8

/*
** MO variable: exp.adf.adu.pat_seg_sz
** Having defined the ima_mib_objects flat object table, set variable
** using:
**
**   UPDATE ima_mib_objects
**   SET value=1978 WHERE classid='exp.adf.adu.pat_seg_sz'
**
** This would enable the segment request size to be set to 1978.
** The default of 0 is checked for and if found, is replaced with the
** record size fo the underlying blob.
*/
u_i4 ADU_pat_seg_sz = 0;

/*
** Name: adu_patda_init() - ADF function instance to initialise DA context
**
** Description:
**      This function 
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
**	rdv				DB_DATA_VALUE describing the varchar
**					which holds the hex representation of
**					the input string.
**	    .db_data			Ptr to the varchar.
**
**	Returns:
**	    E_AD0000_OK			Completed successfully.
**          E_AD9999_INTERNAL_ERROR	If the data types are unexpected
**
**	Exceptions:
**	    none
**
** History:
**      04-Apr-2008 (kiria01)
**          Initial creation.
**	15-Dec-2005 (kiria01)
**	    Fix buffer overrun from underestimation of buffer requirements.
**	29-Dec-2008 (kiria01)
**	    Initialize the pop_cb data better lest dmf refer to it in
**	    error.
**	06-Jan-2009 (kiria01) SIR120473
**	    Apply a couple of casts to quieten compiler warnings.
**	17-Mar-2009 (kiria01)
**	    Couponify locators.
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	30-May-2009 (kiria01) b122128
**	    Added fast to ease streamlined execution of simple strings
**	    and pulled the address and length apart inline.
**	04-Jun-2009 (kiria01) b122128
**	    Don't forget to reset utf8 flag for unicode and make sure that
**	    we don't inadvertantly trip stall processing when processing
**	    non-varying length datatypes and single character ending match.
*/
DB_STATUS
adu_patda_init(
	ADF_CB		*adf_scb,
	DB_DATA_VALUE	*src_dv,
	AD_PAT_SEA_CTX	*sea_ctx,
	AD_PAT_DA_CTX	*da_ctx)
{
    DB_STATUS db_stat = E_DB_OK;
    i4 len = src_dv->db_length;
    u_i1 *s = (u_i1*)src_dv->db_data;
    i4 inbits;
    i4 seg_multr;
    ADP_PERIPHERAL cpn;

    da_ctx->adf_scb = adf_scb;
    da_ctx->unicode = FALSE;
    da_ctx->binary = FALSE;
    da_ctx->utf8 = FALSE;
    da_ctx->fast = FALSE;
    da_ctx->sea_ctx = sea_ctx;
    da_ctx->mem = NULL;
    da_ctx->gets = 0;
    da_ctx->seg_offset = 0;

    if (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED)
	da_ctx->utf8 = TRUE;

    /* Given src_dv determine the mode in which we will work */
    switch (abs(src_dv->db_datatype))
    {
    case DB_NVCHR_TYPE:
    case DB_UTF8_TYPE:
    case DB_NQTXT_TYPE:
	len = (*(u_i2*)s) * sizeof(u_i2);
	s += sizeof(u_i2);
	/*FALLTHROUGH*/
    case DB_NCHR_TYPE:
	/* these will need */
        da_ctx->unicode = TRUE;
	da_ctx->utf8 = FALSE;
	goto commonstr;
    case DB_VBYTE_TYPE:
	len = *(u_i2*)s;
	s += sizeof(u_i2);
	/*FALLTHROUGH*/
    case DB_BYTE_TYPE:
	da_ctx->binary = TRUE;
	da_ctx->utf8 = FALSE;
	goto commonstr;
    case DB_VCH_TYPE:
    case DB_TXT_TYPE:
    case DB_LTXT_TYPE:
	len = *(u_i2*)s;
	s += sizeof(u_i2);
	/*FALLTHROUGH*/
    case DB_CHR_TYPE:
    case DB_CHA_TYPE:
commonstr:
	da_ctx->is_long = FALSE;
	da_ctx->eof_offset = len;

	/* Setup under_dv to mimic the ADP_INFORMATION
	** setting of the underlying datatype. This is
	** needed to avoid inadvertantly dropping into stall
	** processing with non-VARxxxx data in PAT_ENDLIT
	** operator. This is set as though the 'under' value
	** was a varying length datatype. */
	da_ctx->under_dv.db_length = len + sizeof(i2);
	da_ctx->under_dv.db_data = NULL;
	da_ctx->under_dv.db_datatype = 0;
	/*
	** Point segment_dv to the raw data
	** mimicing what the LOB get will do
	*/
	da_ctx->segment_dv = src_dv;
	/* Ensure we don't seem to have a LOB buffer */
	da_ctx->work.adf_agwork.db_data = NULL;
        break;

    case DB_LNVCHR_TYPE:
    case DB_LNLOC_TYPE:
        da_ctx->unicode = TRUE;
	/*FALLTHROUGH*/
    case DB_LBYTE_TYPE:
    case DB_GEOM_TYPE:
    case DB_POINT_TYPE:
    case DB_MPOINT_TYPE:
    case DB_LINE_TYPE:
    case DB_MLINE_TYPE:
    case DB_POLY_TYPE:
    case DB_MPOLY_TYPE:
    case DB_GEOMC_TYPE:
    case DB_LBLOC_TYPE:
	da_ctx->binary = !da_ctx->unicode;
	da_ctx->utf8 = FALSE;
	/*FALLTHROUGH*/
    case DB_LVCH_TYPE:
    case DB_LCLOC_TYPE:
	da_ctx->is_long = TRUE;

	if (!Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi)
	    return(adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "No fexi rtn"));

	/* Populate the POP */
	if (db_stat = adi_dtinfo(adf_scb, src_dv->db_datatype, &inbits))
	    return db_stat;
	if (!(inbits & AD_PERIPHERAL))
	{
	    return(adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "Not peripheral"));
	}
	if (db_stat = adi_per_under(adf_scb, src_dv->db_datatype,
				   &da_ctx->under_dv))
	    return db_stat;
	da_ctx->pop_cb.pop_length = sizeof(da_ctx->pop_cb);
	da_ctx->pop_cb.pop_type = ADP_POP_TYPE;
	da_ctx->pop_cb.pop_ascii_id = ADP_POP_ASCII_ID;
	da_ctx->pop_cb.pop_continuation = ADP_C_BEGIN_MASK;
	da_ctx->pop_cb.pop_info = NULL;
	/* This is irrelevant if the output side isn't long */ 
	da_ctx->pop_cb.pop_temporary = ADP_POP_TEMPORARY;
	da_ctx->pop_cb.pop_segno0 = 0;
	da_ctx->pop_cb.pop_segno1 = 0;
	da_ctx->pop_cb.pop_underdv = &da_ctx->under_dv;
	    da_ctx->under_dv.db_data = 0;
	da_ctx->work.adf_agwork = da_ctx->under_dv;
	da_ctx->pop_cb.pop_segment = &da_ctx->work.adf_agwork;
	da_ctx->segment_dv		= &da_ctx->work.adf_agwork;

	da_ctx->pop_cb.pop_coupon = &da_ctx->cpn_dv;
	da_ctx->pop_cb.pop_user_arg = NULL;
	da_ctx->pop_cb.pop_info = NULL;
	da_ctx->cpn_dv.db_length = sizeof(ADP_PERIPHERAL);
	da_ctx->cpn_dv.db_prec = 0;
	switch (abs(src_dv->db_datatype))
	{
	case DB_LNLOC_TYPE:
	    da_ctx->cpn_dv.db_datatype = DB_LNVCHR_TYPE;
	    goto loc_common;
	case DB_LBLOC_TYPE:
	    da_ctx->cpn_dv.db_datatype = DB_LBYTE_TYPE;
	    goto loc_common;
	case DB_LCLOC_TYPE:
	    da_ctx->cpn_dv.db_datatype = DB_LVCH_TYPE;
loc_common: da_ctx->cpn_dv.db_data = (PTR) &cpn;
	    if (db_stat = adu_locator_to_cpn(adf_scb, src_dv, &da_ctx->cpn_dv))
		return (db_stat);
	    break;
	default:
	    da_ctx->cpn_dv.db_datatype = src_dv->db_datatype;
	    da_ctx->cpn_dv.db_data = src_dv->db_data;
	}
	db_stat = (*Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi)
	    (ADP_INFORMATION, &da_ctx->pop_cb);
	da_ctx->work.adf_agwork.db_data = NULL;
	if (db_stat)
	{
	    return adu_error(adf_scb, da_ctx->pop_cb.pop_error.err_code, 0);
	}
	if (!(len = ADU_pat_seg_sz))
	    len = da_ctx->under_dv.db_length;
	da_ctx->work.adf_agwork.db_length = len;
	da_ctx->work.adf_agwork.db_data = (char *)MEreqmem(0,
		da_ctx->work.adf_agwork.db_length + LEFTOVER_PFX_LEN, FALSE, &db_stat);
	if (db_stat)
	{
	    return adu_error(adf_scb, da_ctx->pop_cb.pop_error.err_code, 0);
	}
	da_ctx->work.adf_agwork.db_datatype = da_ctx->under_dv.db_datatype;
	da_ctx->work.adf_agwork.db_prec = da_ctx->under_dv.db_prec;

	da_ctx->eof_offset = ((ADP_PERIPHERAL *)da_ctx->cpn_dv.db_data)->per_length1;
	
        break;
    default:
	return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    }


    if (da_ctx->unicode)
	/*
	** Need memory for:
	** norm_dv for normalised intermediate (2x)
	** sea_ctx->buffer for CE intermediate (3x)
	*/
	seg_multr = 2 + 3;
    else if (da_ctx->utf8)
	/*
	** Need memory for:
	** norm_dv for normalised intermediate (3x)
	** sea_ctx->buffer for CE intermediate (9x)
	** utf8 assumptions should be covered by Unicode
	*/
	seg_multr = 3 + 9;
    else if (sea_ctx->patdata->patdata.flags & (AD_PAT_WO_CASE|AD_PAT_BIGNORE))
	/*
	** Both need memory for:
	** sea_ctx->buffer for lowercase/packed intermediate.
	** With or without collation the needs are the same and there
	** is no difference for equality.
	*/
	seg_multr = 1;
    else
    {
	/*
	** Needs memory for:
	** nothing - sea_ctx->buffer will be setup by the GET
	** for longs but otherwise - set up now for fast track
	*/
	if (da_ctx->fast = !da_ctx->is_long)
	{
	    sea_ctx->buffer = s;
	    sea_ctx->bufend = s + len;
	}
	return E_DB_OK;
    }

    {
	PTR p;

	da_ctx->mem = p = MEreqmem(0, seg_multr*(len + DB_CNTSIZE), FALSE, &db_stat);
	if (db_stat)
	{
	    return adu_error(adf_scb, da_ctx->pop_cb.pop_error.err_code, 0);
	}
	if (da_ctx->utf8 || da_ctx->unicode)
	{
	    da_ctx->norm_dv.db_data = p;
	    da_ctx->norm_dv.db_datatype = DB_NVCHR_TYPE;
	    da_ctx->norm_dv.db_prec = 0;
	    da_ctx->norm_dv.db_length = 2 * len + DB_CNTSIZE;
	    /* Cater for utf8 extra */
	    if (da_ctx->utf8)
		da_ctx->norm_dv.db_length += len;

	    p += da_ctx->norm_dv.db_length;

	    /* Setup sea_ctx->buffer to describe the final buffer */
	    sea_ctx->buffer = (u_i1*)p;
	    sea_ctx->buflen = seg_multr*(len + DB_CNTSIZE)-da_ctx->norm_dv.db_length;
	}
	else if (sea_ctx->patdata->patdata.flags & (AD_PAT_WO_CASE|AD_PAT_BIGNORE))
	{
	    sea_ctx->buffer = (u_i1*)p;
	    sea_ctx->buflen = len + DB_CNTSIZE;
	    sea_ctx->buftrueend = sea_ctx->bufend;
	}
	else
	{
	    /*
	    ** Will use segment_dv raw and sea_ctx->buffer will
	    ** be setup by the GET
	    */
	    sea_ctx->buffer = NULL;
	    sea_ctx->buflen = 0;
	    sea_ctx->bufend = NULL;
	    sea_ctx->buftrueend = NULL;
	}
    }
    return db_stat;
}

/*
** Name: adu_patda_term() - ADF function instance to cleanup DA context
**
** Description:
**      This function 
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
**	rdv				DB_DATA_VALUE describing the varchar
**					which holds the hex representation of
**					the input string.
**	    .db_data			Ptr to the varchar.
**
**	Returns:
**	    E_AD0000_OK			Completed successfully.
**          E_AD9999_INTERNAL_ERROR	If the data types are unexpected
**
**	Exceptions:
**	    none
**
** History:
**      04-Apr-2008 (kiria01)
**          Initial creation.
*/
DB_STATUS
adu_patda_term(AD_PAT_DA_CTX *da_ctx)
{
    DB_STATUS db_stat = E_DB_OK;

    if (da_ctx->is_long)
    {
	if (da_ctx->gets)
	    db_stat = (*Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi)(
			ADP_CLEANUP, &da_ctx->pop_cb);
	if (da_ctx->work.adf_agwork.db_data)
	{
	    /* Cleanup after long */
	    MEfree((PTR)da_ctx->work.adf_agwork.db_data);
	}
    }
    if (da_ctx->mem)
	MEfree((PTR)da_ctx->mem);

    return db_stat;
}

/*
** Name: adu_patda_get() - ADF function instance to return next segment
**
** Description:
**      This function returns the next (only?) segment
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
**	rdv				DB_DATA_VALUE describing the varchar
**					which holds the hex representation of
**					the input string.
**	    .db_data			Ptr to the varchar.
**
**	Returns:
**	    E_AD0000_OK			Completed successfully.
**          E_AD9999_INTERNAL_ERROR	If the data types are unexpected
**
**	Exceptions:
**	    none
**
** History:
**      04-Apr-2008 (kiria01)
**          Initial creation.
**	19-Sep-2009 (kiria01) b122610
**	    Adjust eof_offset when removing spaces as the PAT_ENDLIT
**	    relies on its accuracy.
*/
DB_STATUS
adu_patda_get(AD_PAT_DA_CTX *da_ctx)
{
    DB_STATUS db_stat = E_DB_OK;
    AD_PAT_SEA_CTX *sea_ctx = da_ctx->sea_ctx;
    i4 len;
    u_i4 pat_flags = sea_ctx->patdata->patdata.flags;
    i2 saved_uninorm_flag = da_ctx->adf_scb->adf_uninorm_flag;

    if (sea_ctx->at_eof)
    {
	/* Read past eof ??? */
	return(adu_error(da_ctx->adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    }
    else if (da_ctx->is_long)
    {
	if (sea_ctx->bufend)
	    da_ctx->seg_offset += sea_ctx->bufend-sea_ctx->buffer;

	/*
	** If we have leftover data we can usually fix this up using the
	** intermediate buffers but if the processing is simple and needs
	** none then we are stuck as the leftover data is about to be written
	** over by the GET. What we do in this case, is allocate an intermediate
	** and prepend now & continue using it.
	*/
	if (!da_ctx->mem && sea_ctx->buftrueend > sea_ctx->bufend)
	{
	    u_i1 *src;
	    /*
	    ** We come through here just the once - from then on .mem
	    ** is set.
	    */
	    da_ctx->mem = MEreqmem(0, da_ctx->segment_dv->db_length+LEFTOVER_PFX_LEN,
			    FALSE, &db_stat);
	    src = sea_ctx->bufend;
	    sea_ctx->buffer = (u_i1*)da_ctx->mem;
	    sea_ctx->bufend = sea_ctx->buffer;
	    while (src < sea_ctx->buftrueend)
		*sea_ctx->bufend++ = *src++;
	    sea_ctx->buftrueend = sea_ctx->bufend; 
	}
	if (sea_ctx->at_bof = !da_ctx->gets)
	{
	    ADP_PERIPHERAL *p = (ADP_PERIPHERAL *)(da_ctx->pop_cb.pop_coupon->db_data);
	    if (!p->per_length0 && !p->per_length1)
	    {
		/* First time in and we have found a zero length blob
		** We need to handle this in a special manner as an
		** ADP_GET call will fail */
		sea_ctx->at_eof = TRUE;
		/* Ensure buffer pointers are valid. */
		sea_ctx->buftrueend = sea_ctx->bufend = sea_ctx->buffer;
		/* Nothing else to do... */
		return E_DB_OK;
	    }
	    if (sea_ctx->patdata->patdata.flags2 & AD_PAT2_ENDLIT)
	    {
		/* We have the hint that ENDLIT is possible so try */
		u_i1 *pc = &sea_ctx->patdata->patdata.first_op+1;
		u_i8 L = GETNUM(pc);
		u_i4 maxseglen = da_ctx->under_dv.db_length - sizeof(i2);
		u_i4 last_seg = (p->per_length1 - L) / maxseglen;
		
		/* Don't optimize if will span multi-seg or if small seek */
		if (last_seg > 2)
		{
		    da_ctx->pop_cb.pop_continuation |= ADP_C_RANDOM_MASK;
		    da_ctx->pop_cb.pop_segno1 = last_seg+1;
		    da_ctx->seg_offset = last_seg * maxseglen;
		}
	    }
	}
	db_stat = (*Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi)(
			    ADP_GET, &da_ctx->pop_cb);
	if (DB_FAILURE_MACRO(db_stat))
	{
	    da_ctx->adf_scb->adf_errcb.ad_errcode =
			da_ctx->pop_cb.pop_error.err_code;
	    sea_ctx->buffer = NULL;
	    sea_ctx->bufend = NULL;
	    return db_stat;
	}
	da_ctx->pop_cb.pop_continuation &= ~(ADP_C_BEGIN_MASK|ADP_C_RANDOM_MASK);
	da_ctx->gets++;

	if (da_ctx->pop_cb.pop_error.err_code == E_AD7001_ADP_NONEXT)
	{
	    db_stat = E_DB_OK;
	    sea_ctx->at_eof = TRUE;
	}
    }
    else
    {
	/* segment_dv already setup */
	sea_ctx->at_bof = TRUE;
	sea_ctx->at_eof = TRUE;
	/* Drop out with setup done prior */
	if (da_ctx->fast)
	    return E_DB_OK;
    }

    if (da_ctx->utf8|da_ctx->unicode)
    {
	const UCS2 *s;
	/*
	** Date gotten in segment_dv - needs norm & CEizing
	** norm_dv for normalised intermediate (2x)
	** sea_ctx->buffer for CE final (3x)
	*/
	if (da_ctx->unicode)
        {
            da_ctx->adf_scb->adf_uninorm_flag = AD_UNINORM_NFC;
	    db_stat = adu_unorm(da_ctx->adf_scb, da_ctx->segment_dv, &da_ctx->norm_dv);
            da_ctx->adf_scb->adf_uninorm_flag = saved_uninorm_flag;
        }
	else
        {
            da_ctx->adf_scb->adf_uninorm_flag = AD_UNINORM_NFC;
	    db_stat = adu_nvchr_fromutf8(da_ctx->adf_scb, da_ctx->segment_dv, &da_ctx->norm_dv);
            da_ctx->adf_scb->adf_uninorm_flag = saved_uninorm_flag;
        }
	if (db_stat)
	    return db_stat;
	if (db_stat = adu_lenaddr(da_ctx->adf_scb, &da_ctx->norm_dv, &len, (char**)&s))
	    return (db_stat);
	if (db_stat = adu_pat_MakeCElen(da_ctx->adf_scb, s, s+len/sizeof(UCS2),
			&da_ctx->sea_ctx->buflen))
	    return db_stat;

	sea_ctx->bufend = sea_ctx->buffer;
	adu_pat_MakeCEchar(da_ctx->adf_scb, &s, s+len/sizeof(UCS2),
			    (UCS2**)&sea_ctx->bufend, pat_flags);
	/* Leave bufend pointing correctly */
    }
    else if (pat_flags & AD_PAT_WO_CASE)
    {
	/*
	** Date gotten in segment_dv - lowercase into data_dv
	** data_dv for lowercase intermediate
	*/
	u_i1 *src, *end;
	/*
	** Do we have a leftover part character? If so, prepend to the buffer.
	*/
	src = sea_ctx->bufend;
	sea_ctx->bufend = sea_ctx->buffer;
	while (src < sea_ctx->buftrueend)
	    *sea_ctx->bufend++ = *src++;

	/* Get the buffer & length */
	if (db_stat = adu_lenaddr(da_ctx->adf_scb, da_ctx->segment_dv, &len, (char**)&src))
	    return (db_stat);
	/* Point to end of buffer */
	end = src + len;

	if (pat_flags & AD_PAT_BIGNORE)
	{
	    while (src < end)
	    {
		if (!CMspace(src) && *src)
		{
		    CMtolower(src, sea_ctx->bufend);
		    CMnext(sea_ctx->bufend);
		}
		CMnext(src);
	    }
	}
	else
	{
	    while (src < end)
	    {
		CMtolower(src, sea_ctx->bufend);
		CMnext(sea_ctx->bufend);
		CMnext(src);
	    }
	}
	/* Leave bufend pointing correctly & set true end */
	sea_ctx->buftrueend = sea_ctx->bufend;
    }
    else if (pat_flags & AD_PAT_BIGNORE)
    {
	u_i1 *src, *end;
	/*
	** Do we have a leftover part character? If so, prepend to the buffer.
	*/
	src = sea_ctx->bufend;
	sea_ctx->bufend = sea_ctx->buffer;
	while (src < sea_ctx->buftrueend)
	    *sea_ctx->bufend++ = *src++;

	/*
	** Data gotten in segment_dv - copy non-spaces into data_dv
	*/
	if (db_stat = adu_lenaddr(da_ctx->adf_scb, da_ctx->segment_dv, &len, (char**)&src))
	    return (db_stat);
	end = src + len;
	while (src < end)
	{
	    if (!CMspace(src) && *src)
		CMcpyinc(src, sea_ctx->bufend);
	    else
	    {
		/* As we are removing a character we are affecting the
		** logical eof offset so adjust accordingly. */
		da_ctx->eof_offset--;
		CMnext(src);
	    }
	}
	/* Leave bufend pointing correctly */
	sea_ctx->buftrueend = sea_ctx->bufend;
    }
    else if (da_ctx->mem)
    {
	u_i1 *src, *end;
	/*
	** Do we have a leftover part character? If so, prepend to the buffer.
	*/
	src = sea_ctx->bufend;
	sea_ctx->bufend = sea_ctx->buffer;
	while (src < sea_ctx->buftrueend)
	    *sea_ctx->bufend++ = *src++;

	/*
	** Data gotten in segment_dv - copy into data_dv
	*/
	if (db_stat = adu_lenaddr(da_ctx->adf_scb, da_ctx->segment_dv, &len, (char**)&src))
	    return (db_stat);
	end = src + len;
	while (src < end)
	    CMcpyinc(src, sea_ctx->bufend);
	/* Leave bufend pointing correctly */
	sea_ctx->buftrueend = sea_ctx->bufend;
    }
    else
    {
	/*
	** Date gotten in segment_dv - use raw buffer & point buffer/bufend
	** directly at the data
	*/
	if (db_stat = adu_lenaddr(da_ctx->adf_scb, da_ctx->segment_dv, &len, (char**)&sea_ctx->buffer))
	    return (db_stat);
	sea_ctx->bufend = sea_ctx->buffer + len;
    }
    return db_stat;
}
