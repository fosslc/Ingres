/*
** Copyright (c) 2008 Ingres Corporation
*/

/**
**
**  Name: adupat.c - Routines related to pattern matching
**
**  Description:
**	This file contains routines that define the ADF interfaces for the
**	pattern matching functions.
**
** This file defines the following externally visible routine:
**
**	adu_like_all() - Perform the match
**
**  Function prototypes defined in ADUINT.H file.
**
**  History:
**      04-Apr-2008 (kiria01) SIR120473
**          Initial creation.
**      04-Sep-2008 (kiria01) SIR120473
**          Reduced MEreqmem overhead by running in stack memory. Particularly
**	    the pattern buffer which for future work will need to have its length
**	    bounded and memory pre-allocated.
**	27-Oct-2008 (kiria01) SIR120473
**	    Introduce AD_PATDATA having dropped from u_i4 vector
**	    to an u_i2 form that fits in with VLUP arrangements.
**	12-Dec-2008 (kiria01) SIR120473
**	    Split patdata.flags for alignment issues
**	15-Dec-2008 (kiria01) SIR120473
**	    Address Unicode side alignments, drop flags copy from
**	    sea_ctx and correct form handling.
**	06-Jan-2009 (kiria01) SIR120473
**	    Handle the escape char as a proper DBV so that Unicode endian
**	    issues are avoided.
**	19-Aug-2009 (drivi01)
**	    Repalce extern ADU_pat_legacy with GLOBALDEF to fix
**	    a bug with uninitialized extern in parser in
**	    Visual Studio 2008 port.
**	08-Jan-2010 (kiria01) b123118
**	    Quietened compiler warnings due to sizeof being unsigned.
**       8-Mar-2010 (hanal04) Bug 122974
**          Removed nfc_flag from adu_nvchr_fromutf8() and adu_unorm().
**          A standard interface is expected by fcn lookup / execute
**          operations. Force NFC normalization is now achieved by temporarily
**          updating the adf_uninorm_flag in the ADF_CB.
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

/*
-2 pure new only wo case
-1 pure new only with case
0  pure new only
1 try for compare results
2 try for compare results & fixup if different
3 use legacy in preference if possible
*/
GLOBALDEF i4 ADU_pat_legacy=0;	/* MO controlled: exp.adf.adu.pat_legacy */


/*
** Name: adu_like_all() - Compare two string data values using the LIKE operator
**		      pattern matching characters.
**
**      This routine compares two string data values for equality using the
**      LIKE operator pattern matching characters '%' (match zero or more
**      arbitrary characters) and '_' (match exactly one arbitrary character).
**      These pattern matching characters only have special meaning in the
**      second data value, patdv.  If the first string is of datatype "c",
**	then blanks and null chars will be ignored.  Shorter strings will be
**	considered less than longer strings.  (Since LIKE only knows about
**	whether the two are "equal" or not, this should not matter.)
**
**	Examples:
**
**	             'Smith, Homer' < 'Smith,Homer' (if neither is "c")
**
**	             'Smith, Homer' = 'Smith,Homer' (if either is "c")
**
**	                     'abcd ' > 'abcd' (if neither is "c")
**
**	                      'abcd' = 'a%d'
**
**			      'abcd' = 'a_cd%'
**
**			      'abcd' = 'abcd%'
**
**			      'abcd ' > 'abc_' (if neither is "c")
**
**			      'abcd ' = 'abc_' (if either is "c")
**
**		      'a b      c d ' = 'abc_' (if either is "c")
**
**			      'abc% ' < 'abcd' (Note % treated literally in 1st)
**
**			      'abcd' < 'abcd_'
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      str_dv                          Ptr to DB_DATA_VALUE containing first
**					string.
**	    .db_datatype		Its datatype.
**	    .db_length			Its length.
**	    .db_data			Ptr to 1st string (for "c" and "char")
**					or DB_TEXT_STRING holding first string
**					(for "text" and "varchar").
**      pat_dv                          Ptr to DB_DATA_VALUE containing second
**					string.  This is the pattern string.
**	    .db_datatype		Its datatype (assumed to be either
**					"char" or "varchar").
**	    .db_length			Its length.
**	    .db_data			Ptr to pattern string (for "c" and
**					"char") or DB_TEXT_STRING holding pat
**					string (for "text" and "varchar").
**	esc_dv				Pointer to dbv structure defining the
**					single character escape. (ay be NULL
**					implying no escape character.
**					Ignored if passed a compile pattern in
**					pat_dv.
**	pat_flags			Set to control the flavour of LIKE -
**					see adf.h for values: AD_PAT_*
**					Ignored if passed a compile pattern in
**					pat_dv.
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
**	rcmp				Result of comparison, as follows:
**					    <0  if  str_dv1 < pat_dv2
**					    =0  if  str_dv1 = pat_dv2
**					    >0  if  str_dv1 > pat_dv2
**
** Returns:
**	The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	If a DB_STATUS code other than E_DB_OK is returned, the caller
**	can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	the ADF error code.  The following is a list of possible ADF error
**	codes that can be returned by this routine:
**
**	    E_AD0000_OK			Completed sucessfully.
**	    E_AD5001_BAD_STRING_TYPE	Either str_dv's or pat_dv's datatype was
**					incompatible with this operation.
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      04-Apr-2008 (kiria01) SIR120473
**          Initial creation.
**	11-Dec-2008 (kiria01) SIR120473
**	    Protect against abends in the patdata structure
**	    
*/

DB_STATUS
adu_like_all(
	ADF_CB		*adf_scb,
	DB_DATA_VALUE	*src_dv,
	DB_DATA_VALUE	*pat_dv,
	DB_DATA_VALUE	*esc_dv,
	u_i4		pat_flags,
	i4		*rcmp)
{
    DB_STATUS db_stat = E_DB_OK;
    AD_PAT_SEA_CTX _sea_ctx;
    AD_PAT_SEA_CTX *sea_ctx = &_sea_ctx;
    AD_PATDATA _patdata;
    AD_PATDATA *patdata;
    AD_PAT_DA_CTX da_ctx;
    DB_DATA_VALUE dv_tmp1;
    DB_DATA_VALUE dv_tmp2;
    DB_DATA_VALUE *s1 = src_dv, *p1 = pat_dv;
    i4 rcmp1 = 0;
    i4 long_seen = 0;
    DB_STATUS db_stat1 = E_DB_OK;
    char tmp[2000];
    i2 saved_uninorm_flag = adf_scb->adf_uninorm_flag;
    static struct {
	ADU_PATCOMP_FUNC *compile;
	ADU_PATEXEC_FUNC *execute;
    } rtns[] = {
	{adu_patcomp_like,     adu_pat_execute},
	{adu_patcomp_like,     adu_pat_execute_col},
	{adu_patcomp_like_uni, adu_pat_execute_uni},
    };
    enum rtns_idx {
	LIKE, LIKE_COLLATION, LIKE_UNICODE 
    } form = LIKE;
    
    if (pat_dv->db_datatype == DB_PAT_TYPE)
    {
	patdata = (AD_PATDATA*)pat_dv->db_data;
	if (ME_ALIGN_MACRO(patdata, sizeof(i2)) != (PTR)patdata)
	{
	    if ((i2)sizeof(_patdata) >= pat_dv->db_length)
		patdata = &_patdata;
	    else
	    {
		patdata = (AD_PATDATA*)MEreqmem(0, pat_dv->db_length,
			    FALSE, &db_stat);
		if (!patdata || db_stat)
		    return db_stat;
	    }
	    MEcopy(pat_dv->db_data, pat_dv->db_length, patdata);
	}
	pat_flags = patdata->patdata.flags|
		    (patdata->patdata.flags2<<16);
	/* Pre-compiled pattern */
	{
	    i4 i;
	    /* ->patdata is passed in preset with a valid [PATDATA_LENGTH] -
	    ** save it now as we will clear the lot */
	    MEfill(sizeof(*sea_ctx), 0, (PTR)sea_ctx);
	    sea_ctx->patdata = patdata;
	    /*sea_ctx->buffer = NULL;*/
	    /*sea_ctx->bufend = NULL;*/
	    /*sea_ctx->buftrueend = NULL;*/
	    /*sea_ctx->seg_offset = 0;*/
	    /*sea_ctx->buflen = 0;*/
	    /*sea_ctx->at_bof = FALSE;*/
	    /*sea_ctx->at_eof = FALSE;*/
	    /*sea_ctx->trace = FALSE;*/
	    /*sea_ctx->force_fail = FALSE;*/
	    /*sea_ctx->cmplx_lim_exc = FALSE;*/
	    /*sea_ctx->stalled = NULL;*/
	    /*sea_ctx->pending = NULL;*/
	    /*sea_ctx->free = NULL;*/
	    /*sea_ctx->setbuf = NULL;*/
#if PAT_DBG_TRACE>0
	    /*sea_ctx->nid = 0;*/
	    /*sea_ctx->infile = NULL;*/
	    /*sea_ctx->outfile = NULL;*/
#endif
	    /*sea_ctx->nctxs_extra = 0;*/
	    sea_ctx->nctxs = DEF_THD;
	    for (i = DEF_THD-1; i >= 0; i--)
	    {
		sea_ctx->ctxs[i].next = sea_ctx->free;
		sea_ctx->free = &sea_ctx->ctxs[i];
	    }
	}
	db_stat = adu_patcomp_set_pats(sea_ctx);

	/* If prior patcomp flagged force_fail - obey it */
	if (patdata->patdata.flags2 & AD_PAT2_FORCE_FAIL)
	    sea_ctx->force_fail = TRUE;
    }
    else
    {
	patdata = &_patdata;
	/* Tell compiler size we are prepared for */
	patdata->patdata.length = sizeof(_patdata)/sizeof(_patdata.vec[0]);
	sea_ctx->patdata = patdata;
	/*
	** To allow for default processing we have the user specified
	** case flags to deal with. If AD_PAT_WITH_CASE or AD_PAT_WO_CASE
	** is set then we use that setting to override any collation
	** case request. If neither are set we obey the collation.
	*/
	if (pat_flags & AD_PAT_WITH_CASE)
	{
	    pat_flags &= ~AD_PAT_WO_CASE;
	}
	else if (!(pat_flags & AD_PAT_WO_CASE) &&
		(src_dv->db_collID == DB_UNICODE_CASEINSENSITIVE_COLL ||
		pat_dv->db_collID == DB_UNICODE_CASEINSENSITIVE_COLL))
	{
	    pat_flags |= AD_PAT_WO_CASE;
	}
	/*
	** From this point on, the AD_PAT_WITH_CASE flag is ignored as
	** its state has been folded into the AD_PAT_WO_CASE flag.
	*/
    }

    if (ADU_pat_legacy == -1)
	pat_flags &= ~AD_PAT_WO_CASE;
    else if (ADU_pat_legacy == -2)
	pat_flags |= AD_PAT_WO_CASE;

    dv_tmp1.db_data = NULL;
    dv_tmp2.db_data = NULL;

    switch (abs(src_dv->db_datatype))
    {
    case DB_LNVCHR_TYPE:
    case DB_LNLOC_TYPE:
	long_seen = 1;
    case DB_NVCHR_TYPE:
    case DB_NCHR_TYPE:
    case DB_UTF8_TYPE:
    case DB_NQTXT_TYPE:
	form = LIKE_UNICODE;
	/* All handled directly by DA */
	break;
    case DB_LVCH_TYPE:
    case DB_LCLOC_TYPE:
	if (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED)
	    form = LIKE_UNICODE;
    case DB_LBYTE_TYPE:
    case DB_LBLOC_TYPE:
	long_seen = 1;
    case DB_BYTE_TYPE:
    case DB_VBYTE_TYPE:
	/* All handled directly by DA */
	break;
    case DB_CHR_TYPE:
	pat_flags |= AD_PAT_BIGNORE;	/* Ignore blanks */
	/*FALLTHROUGH*/
    case DB_CHA_TYPE:
    case DB_VCH_TYPE:
    case DB_TXT_TYPE:
    case DB_LTXT_TYPE:
	if (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED)
	{
	    form = LIKE_UNICODE;
	    dv_tmp1.db_datatype = DB_NVCHR_TYPE;
	    dv_tmp1.db_length = src_dv->db_length * 4 + DB_CNTSIZE;
	    dv_tmp1.db_prec = 0;
	    dv_tmp1.db_collID = -1;
	    if (dv_tmp1.db_length < (i2)sizeof(tmp))
		dv_tmp1.db_data = tmp;
	    else
	    {
		dv_tmp1.db_data = (char *) MEreqmem (0, dv_tmp1.db_length, TRUE, &db_stat);
		if (db_stat)
		    return db_stat;
	    }
            adf_scb->adf_uninorm_flag = AD_UNINORM_NFC;
	    db_stat = adu_nvchr_fromutf8(adf_scb, src_dv, &dv_tmp1);
            adf_scb->adf_uninorm_flag = saved_uninorm_flag;
	    src_dv = &dv_tmp1;
	}
	break;
    default:
	return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    }

    /*
    ** See how the pattern looks and coerce appropriatly
    */
    switch (abs(pat_dv->db_datatype))
    {
    case DB_LNVCHR_TYPE:
    case DB_LNLOC_TYPE:
	long_seen = 1;
    case DB_NVCHR_TYPE:
    case DB_NCHR_TYPE:
    case DB_UTF8_TYPE:
    case DB_NQTXT_TYPE:
	if (form != LIKE_UNICODE)
	{
	    DB_DATA_VALUE dv_tmp3;
	    form = LIKE_UNICODE;
	    dv_tmp1.db_datatype = DB_NVCHR_TYPE;
	    dv_tmp1.db_length = src_dv->db_length * 3 + DB_CNTSIZE;
	    dv_tmp1.db_prec = 0;
	    dv_tmp1.db_collID = -1;
	    dv_tmp3.db_datatype = DB_NVCHR_TYPE;
	    dv_tmp3.db_length = src_dv->db_length * 3 + DB_CNTSIZE;
	    dv_tmp3.db_prec = 0;
	    dv_tmp3.db_collID = -1;
	    if (dv_tmp1.db_length < (i2)sizeof(tmp))
		dv_tmp1.db_data = tmp;
	    else
	    {
		dv_tmp1.db_data = (char *) MEreqmem (0, dv_tmp1.db_length, TRUE, &db_stat);
		if (db_stat)
		    return db_stat;
	    }
	    dv_tmp3.db_data = (char *) MEreqmem (0, dv_tmp3.db_length, TRUE, &db_stat);
	    if (db_stat)
		return db_stat;
	    if (db_stat = adu_nvchr_coerce(adf_scb, src_dv, &dv_tmp3))
	    {
		if (dv_tmp1.db_data && dv_tmp1.db_data != tmp)
		    MEfree((char *)dv_tmp1.db_data);
		return db_stat;
	    }

            adf_scb->adf_uninorm_flag = AD_UNINORM_NFC;
	    db_stat = adu_unorm(adf_scb, &dv_tmp1, &dv_tmp3);
            adf_scb->adf_uninorm_flag = saved_uninorm_flag;
	    MEfree((char *)dv_tmp3.db_data);
	    if (db_stat)
	    {
		if (dv_tmp1.db_data && dv_tmp1.db_data != tmp)
		    MEfree((char *)dv_tmp1.db_data);
		return db_stat;
	    }
	    src_dv = &dv_tmp1;
	}
	break;
    case DB_CHR_TYPE:
	pat_flags |= AD_PAT_BIGNORE;	/* Ignore blanks */
	/*FALLTHROUGH*/
    case DB_LVCH_TYPE:
    case DB_LCLOC_TYPE:
    case DB_LBYTE_TYPE:
    case DB_LBLOC_TYPE:
    case DB_BYTE_TYPE:
    case DB_VBYTE_TYPE:
    case DB_CHA_TYPE:
    case DB_VCH_TYPE:
    case DB_TXT_TYPE:
    case DB_LTXT_TYPE:
	if (form == LIKE_UNICODE)
	{
	    DB_DATA_VALUE dv_tmp3;
	    dv_tmp2.db_datatype = DB_NVCHR_TYPE;
	    dv_tmp2.db_length = pat_dv->db_length * 4 + DB_CNTSIZE;
	    dv_tmp2.db_prec = 0;
	    dv_tmp2.db_collID = -1;
	    dv_tmp3.db_datatype = DB_NVCHR_TYPE;
	    dv_tmp3.db_length = pat_dv->db_length * 3 + DB_CNTSIZE;
	    dv_tmp3.db_prec = 0;
	    dv_tmp3.db_collID = -1;
	    if (dv_tmp2.db_length < (i2)sizeof(tmp) && dv_tmp1.db_data != tmp)
		dv_tmp2.db_data = tmp;
	    else
	    {
		dv_tmp2.db_data = (char *) MEreqmem (0, dv_tmp2.db_length, TRUE, &db_stat);
		if (db_stat)
		    return db_stat;
	    }
	    if (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED)
            {
                adf_scb->adf_uninorm_flag = AD_UNINORM_NFC;
		db_stat = adu_nvchr_fromutf8(adf_scb, pat_dv, &dv_tmp2);
                adf_scb->adf_uninorm_flag = saved_uninorm_flag;
            }
	    else
	    {
		dv_tmp3.db_data = (char *) MEreqmem (0, dv_tmp3.db_length, TRUE, &db_stat);
		if (db_stat)
		    return db_stat;
		if (db_stat = adu_nvchr_coerce(adf_scb, pat_dv, &dv_tmp3))
		{
		    if (dv_tmp2.db_data && dv_tmp2.db_data != tmp)
			MEfree((char *)dv_tmp2.db_data);
		    return db_stat;
		}
                adf_scb->adf_uninorm_flag = AD_UNINORM_NFC;
		db_stat = adu_unorm(adf_scb, &dv_tmp2, &dv_tmp3, 1);
                adf_scb->adf_uninorm_flag = saved_uninorm_flag;
		MEfree((char *)dv_tmp3.db_data);
		if (db_stat)
		{
		    if (dv_tmp2.db_data && dv_tmp2.db_data != tmp)
			MEfree((char *)dv_tmp2.db_data);
		    return db_stat;
		}
	    }
	    pat_dv = &dv_tmp2;
	}
	break;
    case DB_PAT_TYPE:
	if (patdata->patdata.flags2 & AD_PAT2_UNICODE)
	    form = LIKE_UNICODE;
	else if (patdata->patdata.flags2 & AD_PAT2_COLLATE)
	    form = LIKE_COLLATION;
	break;
    default:
	return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    }

    if (abs(pat_dv->db_datatype) != DB_PAT_TYPE)
    {
	if (form == LIKE && adf_scb->adf_collation)
	{
	    form = LIKE_COLLATION;
	    pat_flags |= (AD_PAT2_COLLATE<<16);
	}
	else if (form == LIKE_UNICODE)
	    pat_flags |= (AD_PAT2_UNICODE<<16);

	if (ADU_pat_legacy > 0 &&
		    !long_seen &&
		    (pat_flags & AD_PAT_FORM_MASK) == AD_PAT_FORM_LIKE)
	{
	    if (form == LIKE_UNICODE)
		db_stat1 = adu_ulike(adf_scb, s1, p1,
			(UCS2*)(esc_dv?esc_dv->db_data:0), &rcmp1);
	    else
		db_stat1 = adu_like(adf_scb, s1, p1,
			(u_char*)(esc_dv?esc_dv->db_data:0), &rcmp1);
	    if (ADU_pat_legacy == 3)
	    {
		/* Look no further */
		*rcmp = rcmp1;
		if (dv_tmp1.db_data && dv_tmp1.db_data != tmp)
		    MEfree((char *)dv_tmp1.db_data);
		if (dv_tmp2.db_data && dv_tmp2.db_data != tmp)
		    MEfree((char *)dv_tmp2.db_data);
		return db_stat1;
	    }
	}

	/* Compile the input pattern */
	db_stat = (rtns[form].compile)(adf_scb, pat_dv, esc_dv, pat_flags, sea_ctx);
    }
    if (!db_stat && sea_ctx)
    {
	if (sea_ctx->force_fail)
	    *rcmp = 1;
	else
	{
	    /* Init the data access */
	    if (!(db_stat = adu_patda_init(adf_scb, src_dv, sea_ctx, &da_ctx)))
	    {
		/* Do the search */
		db_stat = (rtns[form].execute)(sea_ctx, &da_ctx, rcmp);
	    }
	    /* Cleanup the data access */
	    (VOID)adu_patda_term(&da_ctx);
	    if (!db_stat && sea_ctx->cmplx_lim_exc)
		db_stat = adu_error(adf_scb, E_AD1026_PAT_TOO_CPLX, 0);
        }
    }
    adu_patcomp_free(sea_ctx);

    if (dv_tmp1.db_data && dv_tmp1.db_data != tmp)
	MEfree((char *)dv_tmp1.db_data);
    if (dv_tmp2.db_data && dv_tmp2.db_data != tmp)
	MEfree((char *)dv_tmp2.db_data);
    if (patdata != &_patdata && (PTR)patdata != pat_dv->db_data)
	MEfree((PTR)patdata);

    if (ADU_pat_legacy > 0 &&
		!long_seen &&
		(pat_flags & AD_PAT_FORM_MASK) == AD_PAT_FORM_LIKE)
    {
	if (db_stat1 && db_stat)
	{
	    /* Old unsupported? - just report */
	    TRdisplay("%s old fail - %d %d osts=%d nsts=%d\n",
		pat_flags & AD_PAT_WO_CASE?"ILIKE":"LIKE",
		s1->db_datatype, p1->db_datatype,
		db_stat1, db_stat);
	}
	else if (db_stat1 && !db_stat)
	{
	    /* NEW SOLUTION - */
	    TRdisplay("%s new support - %d %d osts=%d\n",
		pat_flags & AD_PAT_WO_CASE?"ILIKE":"LIKE",
		s1->db_datatype, p1->db_datatype,
		db_stat1);
	}
	else if (!db_stat1 && db_stat)
	{
	    i4 sl, pl;
	    char *s, *p;
	    /*NEW PROB - report & fixup */
	    adu_lenaddr(adf_scb, s1, &sl, &s);
	    adu_lenaddr(adf_scb, p1, &pl, &p);
	    TRdisplay("%s new problem - %d %d nsts=%d ores=%d nres=%d '%.#s' '%.#s'\n",
		pat_flags & AD_PAT_WO_CASE?"ILIKE":"LIKE",
		s1->db_datatype, p1->db_datatype,
		db_stat,
		rcmp1, *rcmp,
		sl,s,pl,p);
	    if (ADU_pat_legacy > 1)
	    {
		*rcmp = rcmp1;
		db_stat = db_stat1;
	    }
	}
	else if (*rcmp != rcmp1)
	{
	    i4 sl, pl;
	    char *s, *p;
	    /*NEW PROB - report & fixup */
	    adu_lenaddr(adf_scb, s1, &sl, &s);
	    adu_lenaddr(adf_scb, p1, &pl, &p);
	    TRdisplay("%s bad? %d %d ores=%d nres=%d '%.#s' '%.#s'\n",
		pat_flags & AD_PAT_WO_CASE?"ILIKE":"LIKE",
		s1->db_datatype, p1->db_datatype,
		rcmp1, *rcmp,
		sl,s,pl,p);
	    if (ADU_pat_legacy > 1)
	    {
		*rcmp = rcmp1;
		db_stat = db_stat1;
	    }
	}
    }
    return db_stat;
}


/*
** Name: adu_like_comp() - Compile pattern using the LIKE operator
**		      pattern matching characters - unicode variety.
**
**      This routine compiles a string using the LIKE operator pattern
**	matching characters '%' (match zero or more arbitrary characters)
**	and '_' (match exactly one arbitrary character) etc.
**      These pattern matching characters only have special meaning in the
**      second data value, patdv.  If the first string is of datatype "c",
**	then blanks and null chars will be ignored.  Shorter strings will be
**	considered less than longer strings.  (Since LIKE only knows about
**	whether the two are "equal" or not, this should not matter.)
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      pat_dv                          Ptr to DB_DATA_VALUE containing second
**					string.  This is the pattern string.
**	    .db_datatype		Its datatype (assumed to be either
**					"char" or "varchar").
**	    .db_length			Its length.
**	    .db_data			Ptr to pattern string (for "c" and
**					"char") or DB_TEXT_STRING holding pat
**					string (for "text" and "varchar").
**	esc_dv				Pointer to dbv structure defining the
**					single character escape. May be NULL
**					implying no escape character.
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
**      ret_dv                          Ptr to DB_DATA_VALUE to receive compile string
**					string.
**	    .db_datatype		Its datatype.
**	    .db_length			Its length.
**	    .db_data			Ptr to 1st string (for "c" and "char")
**					or DB_TEXT_STRING holding first string
**					(for "text" and "varchar").
**
** Returns:
**	The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	If a DB_STATUS code other than E_DB_OK is returned, the caller
**	can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	the ADF error code.  The following is a list of possible ADF error
**	codes that can be returned by this routine:
**
**	    E_AD0000_OK			Completed sucessfully.
**	    E_AD5001_BAD_STRING_TYPE	Either pat_dv's or ret_dv's datatype was
**					incompatible with this operation.
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      22-Sep-2008 (kiria01) SIR120473
**          Initial creation.
**	05-Dec-2008 (kiria01) SIR120473
**	    Address alignment issue on Solaris
*/

DB_STATUS
adu_like_comp(
	ADF_CB		*adf_scb,
	DB_DATA_VALUE	*pat_dv,
	DB_DATA_VALUE	*ret_dv,
	DB_DATA_VALUE	*esc_dv,
	u_i4		pat_flags)
{
    DB_STATUS db_stat = E_DB_OK;
    AD_PAT_SEA_CTX _sea_ctx;
    AD_PAT_SEA_CTX *sea_ctx = &_sea_ctx;
    AD_PATDATA *patdata = (AD_PATDATA*)ret_dv->db_data;
    AD_PATDATA _patdata;

    if (ME_ALIGN_MACRO(patdata, sizeof(i2)) != (PTR)patdata)
    {
	if ((i2)sizeof(_patdata) >= ret_dv->db_length)
	    patdata = &_patdata;
	else
	{
	    patdata = (AD_PATDATA*)MEreqmem(0, ret_dv->db_length, FALSE, &db_stat);
            if (!patdata || db_stat)
		return db_stat;
	}
    }
    patdata->patdata.length = ret_dv->db_length/sizeof(patdata->vec[0]);
    sea_ctx->patdata = patdata;

    /*
    ** To allow for default processing we have the user specified
    ** case flags to deal with. If AD_PAT_WITH_CASE or AD_PAT_WO_CASE
    ** is set then we use that setting to override any collation
    ** case request. If neither are set we obey the collation.
    */
    if (pat_flags & AD_PAT_WITH_CASE)
    {
	pat_flags &= ~AD_PAT_WO_CASE;
    }
    else if (!(pat_flags & AD_PAT_WO_CASE) &&
	    pat_dv->db_collID == DB_UNICODE_CASEINSENSITIVE_COLL)
    {
	pat_flags |= AD_PAT_WO_CASE;
    }
    /*
    ** From this point on, the AD_PAT_WITH_CASE flag is ignored as
    ** its state has been folded into the AD_PAT_WO_CASE flag.
    */
    if (ADU_pat_legacy == -1)
	pat_flags &= ~AD_PAT_WO_CASE;
    else if (ADU_pat_legacy == -2)
	pat_flags |= AD_PAT_WO_CASE;

    if (adf_scb->adf_collation)
	pat_flags |= (AD_PAT2_COLLATE<<16);

    /* Compile the input pattern into output parameter */
    db_stat = adu_patcomp_like(adf_scb, pat_dv, esc_dv, pat_flags, sea_ctx);

    /* Map the forced fail so the executor sees it */
    if (sea_ctx->force_fail)
	patdata->patdata.flags2 |= AD_PAT2_FORCE_FAIL;

    adu_patcomp_free(sea_ctx);
    if (patdata != (AD_PATDATA*)ret_dv->db_data)
    {
	MEcopy((PTR)patdata, patdata->patdata.length*sizeof(i2),
		ret_dv->db_data);
	if (patdata != &_patdata)
	    MEfree((PTR)patdata);
    }
    return db_stat;
}


/*
** Name: adu_like_comp_uni() - Compile pattern using the LIKE operator
**		      pattern matching characters - unicode variety.
**
**      This routine compiles a string using the LIKE operator pattern
**	matching characters '%' (match zero or more arbitrary characters)
**	and '_' (match exactly one arbitrary character) etc.
**      These pattern matching characters only have special meaning in the
**      second data value, patdv.  If the first string is of datatype "c",
**	then blanks and null chars will be ignored.  Shorter strings will be
**	considered less than longer strings.  (Since LIKE only knows about
**	whether the two are "equal" or not, this should not matter.)
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      pat_dv                          Ptr to DB_DATA_VALUE containing second
**					string.  This is the pattern string.
**	    .db_datatype		Its datatype (assumed to be either
**					"char" or "varchar").
**	    .db_length			Its length.
**	    .db_data			Ptr to pattern string (for "c" and
**					"char") or DB_TEXT_STRING holding pat
**					string (for "text" and "varchar").
**	esc_dv				Pointer to dbv structure defining the
**					single character escape. May be NULL
**					implying no escape character.
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
**      ret_dv                          Ptr to DB_DATA_VALUE to receive compile string
**					string.
**	    .db_datatype		Its datatype.
**	    .db_length			Its length.
**	    .db_data			Ptr to 1st string (for "c" and "char")
**					or DB_TEXT_STRING holding first string
**					(for "text" and "varchar").
**
** Returns:
**	The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	If a DB_STATUS code other than E_DB_OK is returned, the caller
**	can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	the ADF error code.  The following is a list of possible ADF error
**	codes that can be returned by this routine:
**
**	    E_AD0000_OK			Completed sucessfully.
**	    E_AD5001_BAD_STRING_TYPE	Either pat_dv's or ret_dv's datatype was
**					incompatible with this operation.
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      22-Sep-2008 (kiria01) SIR120473
**          Initial creation.
*/

DB_STATUS
adu_like_comp_uni(
	ADF_CB		*adf_scb,
	DB_DATA_VALUE	*pat_dv,
	DB_DATA_VALUE	*ret_dv,
	DB_DATA_VALUE	*esc_dv,
	u_i4		pat_flags)
{
    DB_STATUS db_stat = E_DB_OK;
    AD_PAT_SEA_CTX _sea_ctx;
    AD_PAT_SEA_CTX *sea_ctx = &_sea_ctx;
    AD_PATDATA *patdata = (AD_PATDATA*)ret_dv->db_data;
    AD_PATDATA _patdata;

    if (ME_ALIGN_MACRO(patdata, sizeof(i2)) != (PTR)patdata)
    {
	if ((i2)sizeof(_patdata) >= ret_dv->db_length)
	    patdata = &_patdata;
	else
	{
	    patdata = (AD_PATDATA*)MEreqmem(0, ret_dv->db_length, FALSE, &db_stat);
	    if (!patdata || db_stat)
		return db_stat;
	}
    }
    patdata->patdata.length = ret_dv->db_length/sizeof(patdata->vec[0]);
    sea_ctx->patdata = patdata;

    /*
    ** To allow for default processing we have the user specified
    ** case flags to deal with. If AD_PAT_WITH_CASE or AD_PAT_WO_CASE
    ** is set then we use that setting to override any collation
    ** case request. If neither are set we obey the collation.
    */
    if (pat_flags & AD_PAT_WITH_CASE)
    {
	pat_flags &= ~AD_PAT_WO_CASE;
    }
    else if (!(pat_flags & AD_PAT_WO_CASE) &&
	    pat_dv->db_collID == DB_UNICODE_CASEINSENSITIVE_COLL)
    {
	pat_flags |= AD_PAT_WO_CASE;
    }
    /*
    ** From this point on, the AD_PAT_WITH_CASE flag is ignored as
    ** its state has been folded into the AD_PAT_WO_CASE flag.
    */
    if (ADU_pat_legacy == -1)
	pat_flags &= ~AD_PAT_WO_CASE;
    else if (ADU_pat_legacy == -2)
	pat_flags |= AD_PAT_WO_CASE;

    pat_flags |= (AD_PAT2_UNICODE<<16);

    /* Compile the input pattern into output parameter */
    db_stat = adu_patcomp_like_uni(adf_scb, pat_dv, esc_dv, pat_flags, sea_ctx);

    /* Map the forced fail so the executor sees it */
    if (sea_ctx->force_fail)
	patdata->patdata.flags2 |= AD_PAT2_FORCE_FAIL;

    adu_patcomp_free(sea_ctx);
    if (patdata != (AD_PATDATA*)ret_dv->db_data)
    {
	MEcopy((PTR)patdata, patdata->patdata.length*sizeof(i2),
		ret_dv->db_data);
	if (patdata != &_patdata)
	    MEfree((PTR)patdata);
    }
    return db_stat;
}
