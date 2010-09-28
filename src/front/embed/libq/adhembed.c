/*
** Copyright (c) 2004 Ingres Corporation
*/

#include	<compat.h>
#include        <gl.h>
#include	<sl.h>
#include	<cv.h>		 
#include	<ex.h>
#include	<er.h>
#include	<st.h>
#include	<me.h>
#include	<cm.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<adf.h>
# include       <gca.h>
#include	<iisqlca.h>
#include	<iilibq.h>
#include	<adh.h>
#include	<fe.h>
#include	<afe.h>
#include	<aduucol.h>

/**
+*  Name: adhembed.c - Conversions between embedded data and ADT's.
**
**  Description:
**	This file contains routines used to convert internal data to and 
**	from embedded values.  An embedded value is one whose type and
**	format is understood by an embedded program.  To convert the data, 
**	these routines build a local DB_DATA_VALUE from the embedded data 
**	(DB_EMBEDDED_DATA) and call the external AFE routine "afe_cvinto()".
**	In the case of conversions into embedded character types, the local 
**	routine "adh_chrcvt()" is called instead of afe_cvinto().
**
**  Defines:
**	adh_evtodb()	- Map embedded type/length to internal type/length
**	adh_evcvtdb()	- Convert embedded data to internal format
**	adh_dbtoev()	- Map internal type/length to embedded type/length
**	adh_dbcvtev()	- Convert internal data to embedded format.
**	adh_chrcvt()	- Convert internal char data to embedded char data.
**	adh_charend()	- Data is in host var - blank-pad or EOS the variable.
**	adh_chkeos()	- Check character variable for EOS.
**
**  Notes:
**	The routines in this file replace the ADF routines contained in the 
**	file adhembedded.c and adcembedded.c written by Gene Thurston.
-*
**  History:
**	16-mar-1987	- written (bjb)
**	31-jul-87 (bab)
**		Removed _VOID_ from in front of MEfill--which is already
**		of type VOID.
**	09/19/88 (dkh) - Changed adh_exptabs to use the correct length
**			 for nullable 'vchar', 'text' and 'longtext' types.
**	10/24/88 (wong) - Added registers and condensed some code.
**	10/28/88 (ncg) - Added afe_cvinto and afe_tycoerce interface for
**			 better performance.  Removed the adh exception
**			 handler as this is no longer needed and replaced
**			 its reference with ADH_OVF_MACRO defined later.
**			 Also removed adh_exptabs as it is no longer used
**			 for the form system.  Strings now go through
**			 afe_cvinto.
**	03/02/89 (bjb)	 Changed adh_evcvtdb to fix dbms repeat qry bug #4825.
**	13-jun-1990 (barbara)
**	    Updated for 6.5/Orion decimal support.
**	31-oct-1990 (teresal)
**	    Updated CVpka interface for decimal support.	
**	3/91 (Mike S) Added DB_OBJ_TYPE conversions for Windows4GL
**	17-aug-91 (leighb) DeskTop Porting Change: added cv.h
**	29-nov-91 (seg)
**	    Can't dereference or do arithmetic on PTR
**	18-oct-92 (teresal)
**	    Turn on decimal stuff by checking the protocol level in
**	    the ADF control block. Take out temporary code that converted
**	    decimal to ascii string to float (and back); support should 
**	    now be in ADF to convert from decimal <-> float directly.
**	2/93 (Mike S) Make distinguishable embedded type for DB_OBJ_TYPE
**	1-apr-93 (robf)
**	  Handle security_label->string conversions, previously fell
**	  through to an empty size string
**	10-jun-1993 (kathryn)
**	    FIPS - Add Support for ANSI SQL retrieval assignment rules.
**	    Add adh_charend() to blank-pad or EOS terminate or both, for
**	    Fixed length "C" character strings. The routine assumes that
**	    data has already been copied to applications host var just ends
**	    the host var character string correctly. Change adh_chrcvt and
**	    adh_losegcvt to call adh_charend.
**	20-jul-1993 (kathryn) Bug 53472
**	    Correct the handling of DB_NUL_TYPE.
**	26-jul-1993 (mgw)
**	    Added support for DB_BYTE_TYPE and DB_VBYTE_TYPE.
**	23-jul-93 (robf)
**	   Use ADH_EXTLEN_SEC for default size of security label instead
**	   of SL value. 
**	15-sep-1993 (sandyd)
**	    Added a few more pieces needed for the handling of DB_VBYTE_TYPE 
**	    as an embedded type.
**	01-oct-1993 (sandyd)
**	    More "byte" cases, for adh_dbtoev(), adh_dbcvtev(), adh_chrcvt().
**	31-dec-1993 (teresal)
**	    Add runtime check for non-null terminated C strings. This
**	    is a requirement for ANSI SQL2. This check is only done if
**	    the '-check_eos' ESQL/C preprocessor flag was used at preprocess 
**	    time. Bug fix 55915. Also, clean up decimal error messages
**	    which is needed so we can use E_AD9999_INTERNAL_ERROR temporarily.
**      11-nov-93 (smc)
**	    Bug #58882
**          Fixed up truncating cast if size of ptr > int.
**      14-feb-94 (johnst)
**          Bug #59977
**          In the conversion routine adh_dbcvtev(), 
**          when copying object pointers, use type-correct (PTR *) casts when
**	    de-referencing db_data and ed_data instead of original (i4 *) cast.
**	    This makes a difference for DEC alpha (axp_osf) where pointers are 
**	    64-bit and ints are 32-bit, so casting a pointer as an int caused
**	    truncation of pointer values.
**	17-may-96 (kch)
**	    In the function adh_evtodb(), we now check for uninitialized
**	    nullable varchar values (db_t_count > DB_MAXSTRING) and set 
**	    adh_dv->db_length to dv->db_length. This change fixes bug 75957.
**	28-jun-96 (thoda04)
**	    Added missing void as return type of adh_losegcvt().
**	    Added fe.h and afe.h for the function prototypes.
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**      01-feb-00 (wanya01)
**          Bug 90970. In adh_dbcvtev(). Added one byte to dv->db_length to 
**          avoid creating varchar column with 0 length.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	07-mar-2001 (abbjo03)
**	    Add support for DB_NCHR_TYPE and DB_NVCHR_TYPE for Unicode.
**	22-mar-2001 (abbjo03)
**	    Additional support for DB_NVCHR_TYPEs.
**	26-May-2009 (kschendel) b122041
**	    Compiler warning fixes;  ed_length is unsigned.
**	aug-2009 (stephenb)
**		Prototyping front-ends
**      18-nov-2009 (joea)
**          Add case for DB_BOO_TYPE in adh_dbtoev.
**/

/*
**  Definition of static variables and forward static functions.
*/

static	DB_STATUS adh_chrcvt();		/* Convert to char embedded values */
static  DB_STATUS adh_error( ADF_CB *, ER_MSGID );

/*
** ADH_OVF_MACRO - ADF errors that signal numeric warnings.  These are
**		   checked for to prevent the system overhead for setting
**		   an exception handler for afe_cvinto calls.
*/
# define	ADH_OVF_MACRO(err)					\
	(   err == E_AD0121_INTOVF_WARN || err == E_AD1121_INTOVF_ERROR \
	 || err == E_AD0123_FLTOVF_WARN || err == E_AD1123_FLTOVF_ERROR \
         || err == E_AD0127_DECOVF_WARN || err == E_AD1127_DECOVF_ERROR )


/*{
+*  Name: adh_evtodb - Provide mapping of embedded value to data value
**
**  Description:
**	This routine is used to choose the datatype and length that a
**	given embedded type and length will be converted into by the
**	adh_evcvtdb() routine.  It also decides whether data copying 
**	will be required in that conversion.  Data copying is
**	necessary:
**
**	(1) if the embedded value is CHR/CHA with a length of zero or
**	    greater than DB_CHAR_MAX and the incoming parameter
**	    'adh_anychar' is FALSE.  
**	(2) if .ed_null is a non-null pointer.
**
**	With regard to (2), note that this routine sets up for data
**	copying even if .ed_null points at 0 (i.e. data is non-null).
**	This is because the DBMS relies on getting the same data
**	description for any one variable in a repeat query each time
**	the query is repeated.
**
**	If data copying will be required in adh_evcvtdb(), this routine
**	sets adh_dv.db_data to point to (what should be) an aligned
**	buffer passed in via adh_dbuf.  If data copying is not going
**	to be necessary, adh_dv.db_data is set to adh_ev.ed_data.
**
**  Inputs:
**	adf_scb			A pointer to the current ADF_CB
**				control block.
**	adh_ev			Ptr to a DB_EMBEDDED_DATA struct holding
**				embedded data and its description.
**		.ed_type	Its canonical type.  If ed_type is
**				DB_NUL_TYPE, then the DB_EMBEDDED_DATA
**				represents the null constant.  The DB_DATA_VALUE
**				chosen for conversion is DB_LTXT_TYPE with
**				just the null byte.
**		.ed_length	Its length
**		.ed_null	Ptr to the null indicator.  If this ptr
**				is NULL, then no null indicator exists.
**				If this ptr is not NULL then if the i2
**				pointed to is DB_EMB_NULL, the data
**				represented is null valued; otherwise the
**				data is valid.
**		.ed_data	Ptr to the embedded data.  (Can be null).
**	adh_dv			Must point to an unassigned DB_DATA_VALUE.
**	adh_dbuf		Must point to a DB_DATA_VALUE
**		.db_length	Length of the allocated area
**		.db_data	An aligned buffer to be used for data
**				conversion.
**	adh_anychar		If this input is TRUE, then assume the caller is
**				building a special purpose dbv which may
**				describe character data with an illegal length.
**				e.g. incoming CHR/CHA data w/len > ADH_MAX_CHAR
**				or w/len = 0 can safely be mapped to CHA type 
**				with "illegal" length.
**				If this input is FALSE, map above CHR/CHA types
**				to VCH.
**
**  Outputs:
**	adh_dv			Pointer to a DB_DATA_VALUE holding the
**				mapped information.
**		.db_datatype	The ADF type corresponding to the embedded
**				type.
**		.db_length	The length of the type.
**		.db_data	Ptr to the location of the converted
**				data (but no conversion has yet taken place).
**				If this routine decides that the conversion
**				will require data copying, .db_data will 
**				point at adh_dbuf->db_data; otherwise it
**				will point at adh_ev->ed_data.
**
**	Returns:
**	    	E_DB_OK		If succeeded.
**	    	E_DB_ERROR	If some error occurred in the mapping
**				process.
**	Errors:
**
**    	If a DB_STATUS code other than E_DB_OK is returned, the field 
**	adf_scb.adf_errcb.ad_errcode will contain one of the following 
**	codes:
**		
**		E_AD1010_BAD_EMBEDDED_TYPE Illegal canonical type
**		E_AD1011_BAD_EMBEDDED_LEN  Illegal length for the supplied type.
**		E_AD1080_STR_TO_DECIMAL	   Embedded value cannot be mapped
**					   to a  decimal.
-*
**  Side Effects:
**	None
**	
**  History:
**	15-mar-1987 - written (bjb)
**	07-jul-1988 - modified the result of the null constant (ncg)
**	13-jul-1990 (barbara)
**	    Added decimal support.  Allow DB_EMBED_DATAs of two new types.
**	    DB_DEC_TYPE (decimal format) and DB_DEC_CHR_TYPE (decimal encoded
**	    as string).  The latter type allows embedded decimal literals to
**	    escape compiler interpretation.
**	12-feb-1991 (teresal)
**	    Minor change to avoid calling CVapk if previous call to
**	    CVfa failed.
**	29-nov-91 (seg)
**	    Can't dereference or do arithmetic on PTR
**	15-jan-1993 (kathryn)
**	    Allow for long CHA/CHR/VCH types.  
**	06-apr-1993 (kathryn)
**	    For DB_DBV_TYPE ensure that precision is copied from DB_EMBED_DATA
**	    to DB_DATA_VALUE. The DB_EMBED_DATA may be decimal datatype.
**	20-jul-1993 (kathryn)
**	    Correct handling of DB_NUL_TYPE. This type has its own case and
**	    sets the datatype to -DB_LTXT_TYPE so do not attempt to negate it
**	    when the value is null.
**	18-aug-1993 (kathryn)
**	    Check for negative lengths of varying datatypes.
**	    "islong" is now true for strings longer than DB_MAXSTRING when
**	    protocol is release 6.5 or greater.
**	14-sep-1993 (mgw)
**	    Took out ridiculous 'x = x' type assignment in the DB_BYTE_TYPE
**	    case and put in a comment about why that was even attempted in
**	    the first place.
**	15-sep-1993 (sandyd)
**	    Added a few more pieces needed for the handling of DB_VBYTE_TYPE 
**	    as an embedded type.  This type should receive the same handling as 
**	    DB_VCH_TYPE.
**	21-oct-93 (donc)
**	    Correct the method by which we check for datatype support.  Rather
**	    than checking cgc_version for protocol level, check adf_proto_level
**	    bitmap for AD_LARGE_PROTO, for decimal support check for
**	    AD_DECIMAL_PROTO
**	18-nov-93 (robf)
**          Add DB_SEC_TYPE
**	31-dec-93 (teresal)
**	    Add support for '-check_eos'. Check for a negative length for CHR
**	    types.
**	07-jul-95 (forky01)
**	    Fix bug 60214.  Truncate length of dbv for varchar types to be the
**	    actual varchar data length + cnt bytes + [null byte] so that the
**	    entire varchar field isn't sent in GCA and misinterpreted on the
**	    receiving end as actual data for the width of the entire field.
**	    This is only when the DB_VCH_TYPE is passed in as a DB_DBV_TYPE.
**	17-may-96 (kch)
**	    Check for uninitialized nullable varchar values (db_t_count >
**	    DB_MAXSTRING) and set adh_dv->db_length to dv->db_length. 
**	    This change fixes bug 75957.
**      28-Jan-97 (rommi04)
**          Fix bug 80300.  Truncate length of dbv for text/vchar types to be the
**          actual text/vchar data length + cnt bytes + [null byte] so that the
**          entire text/vchar field isn't sent in GCA and misinterpreted on the
**          receiving end as actual data for the width of the entire field.
**          This is only when the DB_TXT_TYPE is passed in as a DB_DBV_TYPE.
**      01-feb-00 (wanya01)
**          Bug 90970. In adh_dbcvtev(). Added one byte to dv->db_length to 
**          avoid creating varchar column with 0 length.
**	07-mar-2001 (abbjo03)
**	    Add support for DB_NCHR_TYPE and DB_NVCHR_TYPE.
**	13-mar-2001 (abbjo03)
**	    Additional support for DB_NVCHR_TYPEs.
**	14-Jul-2004 (schka24)
**	    Allow bigints (i8).
**	03-aug-2006 (gupsh01)
**	    Added ANSI date/time types.
*/

DB_STATUS
adh_evtodb( adf_scb, adh_ev, adh_dv, adh_dvbuf, adh_anychar )
ADF_CB			*adf_scb;
DB_EMBEDDED_DATA	*adh_ev;
DB_DATA_VALUE		*adh_dv;
DB_DATA_VALUE		*adh_dvbuf;
bool			adh_anychar;
{
    DB_STATUS		db_stat = E_DB_OK;
    i4			len, prec, scal;
    bool		islong = FALSE;
    bool		isnull = FALSE;

    if (adh_ev->ed_null != (i2 *)0 && *adh_ev->ed_null == DB_EMB_NULL)
	isnull = TRUE;
    switch(adh_ev->ed_type)
    {
      case DB_INT_TYPE:
	if (   adh_ev->ed_length != sizeof(i1)
	    && adh_ev->ed_length != sizeof(i2)
	    && adh_ev->ed_length != sizeof(i4)
	    && adh_ev->ed_length != sizeof(i8)
	   )
	{
	    db_stat = adh_error(adf_scb, E_AD1011_BAD_EMBEDDED_LEN );
	}
	else
	{
	    adh_dv->db_datatype = DB_INT_TYPE;
	    adh_dv->db_length = adh_ev->ed_length;
	    adh_dv->db_data = adh_ev->ed_data;
	}
	break;

      case DB_FLT_TYPE:
	if (   adh_ev->ed_length != sizeof(f4)
	    && adh_ev->ed_length != sizeof(f8)
	   )
	{
	    db_stat = adh_error(adf_scb, E_AD1011_BAD_EMBEDDED_LEN );
	}
	else
	{
	    adh_dv->db_datatype = DB_FLT_TYPE;
	    adh_dv->db_length = adh_ev->ed_length;
	    adh_dv->db_data = adh_ev->ed_data;
	}
	break;

      case DB_DEC_CHR_TYPE:
      /*
      ** This type is a preprocessor generated type describing a character
      ** string representation of a decimal literal.  This allows
      ** applications to uniformly pass decimal literals (avoids compiler
      ** rules.)
      */
	if (   adh_ev->ed_null == (i2*)0
	    || *adh_ev->ed_null != DB_EMB_NULL)
	{
	    if (STindex(adh_ev->ed_data, ERx("."), 0) == (char *)0)
	    /* If no decimal point treat as float in keeping with dbms */
	    {
		adh_dv->db_prec = 0;
		adh_dv->db_datatype = DB_FLT_TYPE;
		adh_dv->db_length = 8;
		adh_dv->db_data = adh_dvbuf->db_data;
	    }
	    else if (afe_dec_info(
			adh_ev->ed_data, '.', &len, &prec, &scal) != OK)
	    /*
	    ** Obtain length, scale and precision.  Assume that decimal point
	    ** is dot because currently only dot is allowed by preprocessors.
	    */
	    {
		db_stat = adh_error(adf_scb, E_AD1080_STR_TO_DECIMAL );
	    }
	    else
	    {
		/* 
		** Interoperability check - if we are talking with a 
		** server that doesn't support decimal, map it
		** to a float.
		*/
	        if (adf_scb->adf_proto_level & AD_DECIMAL_PROTO) 
		{
		    adh_dv->db_prec = DB_PS_ENCODE_MACRO( prec, scal );
		    adh_dv->db_datatype = DB_DEC_TYPE;
		    adh_dv->db_length = len;
		    adh_dv->db_data = adh_dvbuf->db_data;
		}
		else
		{
		    adh_dv->db_prec = 0;
		    adh_dv->db_datatype = DB_FLT_TYPE;
		    adh_dv->db_length = sizeof(f8);
		    adh_dv->db_data = adh_dvbuf->db_data;
		}
	    }
	}
	/* else null decimal literal -- shouldn't ever happen */
	break;

      case DB_DEC_TYPE:
	/* 
	** Interoperability check - if we are talking with a 
	** server that doesn't support decimal, map it
	** to a float.
	*/

	if (adf_scb->adf_proto_level & AD_DECIMAL_PROTO) 
	{
	   adh_dv->db_prec = adh_ev->ed_length;
	   prec = DB_P_DECODE_MACRO( adh_ev->ed_length );
	   adh_dv->db_length = DB_PREC_TO_LEN_MACRO( prec );
	   adh_dv->db_datatype = DB_DEC_TYPE;
	   adh_dv->db_data = adh_ev->ed_data;
	}
	else
	{
	   adh_dv->db_prec = 0;
	   adh_dv->db_length = sizeof(f8);
	   adh_dv->db_datatype = DB_FLT_TYPE;
	   adh_dv->db_data = adh_dvbuf->db_data;
	}
	break;

      case DB_CHA_TYPE:
      case DB_CHR_TYPE:
	if (adh_ev->ed_type == DB_CHA_TYPE)
	{
        /* 
        ** LIBQ or preprocessor-generated type meaning "non-C" string.
        ** Map to CHA type, but ignore trailing blanks in choosing length of
        ** dbv (trailing blanks are compiler generated, not part of the data).
        */
	    char	*cp = adh_ev->ed_data; 
	    char	*endp; 
	    char	*lastp = cp;

	    if (adh_ev->ed_length == 0 && adh_dv->db_length > MAXI2) /* BLOB */
		endp = (char *)adh_ev->ed_data + adh_dv->db_length;
	    else
		endp = (char *)adh_ev->ed_data + adh_ev->ed_length;

	    while (cp < endp)
	    {
		if (*cp != ' ')
		{
		    CMnext(cp);
		    lastp = cp;
		}
		else
		{
		    CMnext(cp);
		}
	    }
	    adh_dv->db_length = lastp - (char *)adh_ev->ed_data;  /* Might be zero! */
	}
	else	/* CHR */
	{
	    II_THR_CB *thr_cb = IILQthThread();
	    II_LBQ_CB *IIlbqcb = thr_cb->ii_th_session;

        /*
	** LIBQ or preprocessor-generated type meaning "C-string".
        ** Since the preprocessor (i.e. EQUEL/C and ESQL/C) does not always 
        ** generate a length for character types, check zero-length, non-null
        ** character strings for their actual length.
        */
	    /*
	    ** Check for EOS if ESQLC flag "-check_eos" was used (FIPS)
	    */
	    if (IIlbqcb->ii_lq_flags & II_L_CHREOS)
	    {
		if (adh_chkeos((char *)adh_ev->ed_data, 
				(i4)adh_ev->ed_length) != OK)
		{
		   /* Change following when the new ADH error has been added */
		   /* db_stat = adh_error(adf_scb, E_ADXXXX_UNTERM_C_STR ) */
	    	   db_stat = adh_error(adf_scb, E_AD9999_INTERNAL_ERROR );
		   break;
		}
	    }
	    if (adh_ev->ed_length == 0)
	    {
		if (    adh_ev->ed_null == (i2*)0 
		    || *adh_ev->ed_null != DB_EMB_NULL
		   )
		{
		    adh_dv->db_length = STlength( adh_ev->ed_data );
		}
	        /* length of null-valued zero-length CHRs taken care of below */
	    }
	    else
	    {
		adh_dv->db_length = adh_ev->ed_length;  
	    }
	}
	/*
	** For embedded CHR/CHA type, if the length is 0 or greater than
	** DB_CHAR_MAX, it must "legally" be mapped to a VCH data type.  
	** Otherwise, map it to the CHA data type. 
	** However, if 'adh_anychar' equals 1, the caller wishes CHR/CHA
	** data to be left as CHA.  Note that both these mappings will allow 
	** conversion of data containing escape characters.
	*/
	if ((!isnull) && (adh_dv->db_length > DB_MAXSTRING) &&
	    (adf_scb->adf_proto_level & AD_LARGE_PROTO))
	{
		islong = TRUE;
		adh_dv->db_datatype = DB_CHA_TYPE;
	}
	else if (   (adh_dv->db_length > DB_CHAR_MAX || adh_dv->db_length == 0)
	         && !adh_anychar  
	        )
	{
	    adh_dv->db_length = 
		min(adh_dv->db_length +DB_CNTSIZE, adh_dvbuf->db_length);
	    adh_dv->db_datatype = DB_VCH_TYPE;
	    adh_dv->db_data = adh_dvbuf->db_data;
	}
	else
	{
	    adh_dv->db_datatype = DB_CHA_TYPE;
	    adh_dv->db_data = adh_ev->ed_data;
	}
	break;

      case DB_NCHR_TYPE:
	if (adh_ev->ed_length)
	{
	    adh_dv->db_length = adh_ev->ed_length;  
	}
	else
	{
	    if (adh_ev->ed_null == NULL || *adh_ev->ed_null != DB_EMB_NULL)
		adh_dv->db_length = wcslen((wchar_t *)adh_ev->ed_data) * 2;
	}
	adh_dv->db_datatype = DB_NCHR_TYPE;
	if (sizeof(wchar_t) == 2)
	    adh_dv->db_data = adh_ev->ed_data;
	else
	    adh_dv->db_data = adh_dvbuf->db_data;
	break;

      case DB_NVCHR_TYPE:
        /* Check for invalid negative length */
	if (!isnull &&
		(i2)(((AFE_NTEXT_STRING *)adh_ev->ed_data)->afe_t_count) < 0)
	{
	    db_stat = adh_error(adf_scb, E_AD1011_BAD_EMBEDDED_LEN);
	}
	else
	{
	    adh_dv->db_datatype = adh_ev->ed_type;
	    adh_dv->db_length =
		((AFE_NTEXT_STRING *)adh_ev->ed_data)->afe_t_count * 2 +
		DB_CNTSIZE;
	    if (sizeof(wchar_t) == 2)
		adh_dv->db_data = adh_ev->ed_data;
	    else
		adh_dv->db_data = adh_dvbuf->db_data;
	}
	break;

      case DB_BYTE_TYPE:
	if (adh_ev->ed_length == 0 && adh_dv->db_length > MAXI2) /* BLOB */
	{
	    /*
	    ** Note, we're trying to emulate the DB_CHA_TYPE case here.
	    ** What is done there is effectively:
	    **
	    ** adh_dv->db_length = adh_dv->db_length;
	    **
	    ** but this result is decremented by the number of trailing
	    ** blanks in the data. For DB_BYTE_TYPE, trailing blanks are
	    ** significant so we have no such need to trim them.
	    **
	    ** Note that this is all set up correctly in IIputdomio(),
	    ** but may not be in IIAG4rvRetVal() or add_param() in
	    ** abf!abfrt!g4call.c
	    */
	    ;
	}
	else
	{
	    adh_dv->db_length = adh_ev->ed_length;
	}
	adh_dv->db_datatype = DB_BYTE_TYPE;
	adh_dv->db_data = adh_ev->ed_data;
	if ((!isnull) && (adh_dv->db_length > DB_MAXSTRING))
	    islong = TRUE;
	break;

      case DB_VCH_TYPE:
      case DB_VBYTE_TYPE:
        /* Check for invalid negative length */
	if ((!isnull) &&
	    (i2)(((DB_TEXT_STRING *)adh_ev->ed_data)->db_t_count) < 0)
	    db_stat = adh_error(adf_scb, E_AD1011_BAD_EMBEDDED_LEN );
	else
	{
	    adh_dv->db_datatype = adh_ev->ed_type;
	    adh_dv->db_length = ((DB_TEXT_STRING *)adh_ev->ed_data)->db_t_count
								+ DB_CNTSIZE;
	    adh_dv->db_data = adh_ev->ed_data;
	    if ((adh_dv->db_length > (DB_MAXSTRING + DB_CNTSIZE)) && (!isnull) 
	       && (adf_scb->adf_proto_level & AD_LARGE_PROTO))
	    	    islong = TRUE;
	}
	break;

      case DB_BOO_TYPE:
        adh_dv->db_datatype = DB_BOO_TYPE;
        adh_dv->db_data = adh_ev->ed_data;
        adh_dv->db_length = DB_BOO_LEN;
        break;
    
      case DB_DTE_TYPE:
      /* This is a special FE type.  We ignore the preprocessor-generated
      ** length and used the internal length of a date format
      */
	adh_dv->db_datatype = DB_DTE_TYPE;
	adh_dv->db_data = adh_ev->ed_data;
	adh_dv->db_length = ADH_INTLEN_DTE;
	break;

      case DB_ADTE_TYPE:
	adh_dv->db_datatype = DB_ADTE_TYPE;
	adh_dv->db_length = ADH_INTLEN_ADTE;
	adh_dv->db_data = adh_ev->ed_data;
	break;

      case DB_TMWO_TYPE:
	adh_dv->db_datatype = DB_TMWO_TYPE;
	adh_dv->db_length = ADH_INTLEN_TIME;
	adh_dv->db_data = adh_ev->ed_data;
	break;

      case DB_TMW_TYPE:
	adh_dv->db_datatype = DB_TMW_TYPE;
	adh_dv->db_length = ADH_INTLEN_TIME;
	adh_dv->db_data = adh_ev->ed_data;
	break;

      case DB_TME_TYPE:
	adh_dv->db_datatype = DB_TME_TYPE;
	adh_dv->db_length = ADH_INTLEN_TIME;
	adh_dv->db_data = adh_ev->ed_data;
	break;

      case DB_TSWO_TYPE:
	adh_dv->db_datatype = DB_TSWO_TYPE;
	adh_dv->db_length = ADH_INTLEN_TSTMP;
	adh_dv->db_data = adh_ev->ed_data;
	break;

      case DB_TSW_TYPE:
	adh_dv->db_datatype = DB_TSW_TYPE;
	adh_dv->db_length = ADH_INTLEN_TSTMP;
	adh_dv->db_data = adh_ev->ed_data;
	break;

      case DB_TSTMP_TYPE:
	adh_dv->db_datatype = DB_TSTMP_TYPE;
	adh_dv->db_length = ADH_INTLEN_TSTMP;
	adh_dv->db_data = adh_ev->ed_data;
	break;

      case DB_INDS_TYPE:
	adh_dv->db_datatype = DB_INDS_TYPE;
	adh_dv->db_length = ADH_INTLEN_INTDS;
	adh_dv->db_data = adh_ev->ed_data;
	break;

      case DB_INYM_TYPE:
	adh_dv->db_datatype = DB_INYM_TYPE;
	adh_dv->db_length = ADH_INTLEN_INTYM;
	adh_dv->db_data = adh_ev->ed_data;
	break;

      case DB_DBV_TYPE:
      /*
      ** Just set up the outgoing DBV to point at the incoming one.
      */
        {
	    DB_DATA_VALUE	*dv = (DB_DATA_VALUE *)adh_ev->ed_data;

	    adh_dv->db_datatype = dv->db_datatype;
	    adh_dv->db_data = dv->db_data;
	    adh_dv->db_prec = dv->db_prec;

            switch ( dv->db_datatype )
	    {
		case  DB_VCH_TYPE :
		case -DB_VCH_TYPE :
	            /*
	            **  Readjust the dbv length to equate to varchar data length,
	            **  instead of the varchar field width.
	            */
		    /* Bug 75957 - Check for uninitialized nullable varchar */
		    if (((DB_TEXT_STRING *)dv->db_data)->db_t_count > DB_MAXSTRING
		       && dv->db_datatype == -DB_VCH_TYPE)
		        adh_dv->db_length = dv->db_length;

		    else
		    {
		        adh_dv->db_length = 
		          ((DB_TEXT_STRING *)dv->db_data)->db_t_count + DB_CNTSIZE;

                        if ( adh_dv->db_length == DB_CNTSIZE )
                          adh_dv->db_length += 1;

		        /*
		        ** Add 1 to length for NULLable byte type
		        */
		        if (dv->db_datatype == -DB_VCH_TYPE )
		          adh_dv->db_length += 1;
		    }
		    break;

		case  DB_TXT_TYPE :
		case -DB_TXT_TYPE :
                    /*  Bug 80300.
                    **  Readjust the dbv length to equate to text data length,
                    **  instead of the text field width.
                    */
                    if (((DB_TEXT_STRING *)dv->db_data)->db_t_count > DB_MAXSTRING
                       && dv->db_datatype == -DB_TXT_TYPE)
                        adh_dv->db_length = dv->db_length;

                    else
                    {
                        adh_dv->db_length =
                          ((DB_TEXT_STRING *)dv->db_data)->db_t_count + DB_CNTSIZE;

                        /*
                        ** Add 1 to length for NULLable byte type
                        */
                        if (dv->db_datatype == -DB_TXT_TYPE )
                          adh_dv->db_length += 1;
                    }
                    break;

		default :
		    adh_dv->db_length = dv->db_length;
		    break;
	    }
	}
	break;

      case DB_NUL_TYPE:
      /*
      ** The NULL constant gets converted to a null and zero-length
      ** DB_LTXT_TYPE value.
      */
	adh_dv->db_datatype = -DB_LTXT_TYPE;
	adh_dv->db_length = DB_CNTSIZE + 1;
	adh_dv->db_data = adh_dvbuf->db_data;
	break;

      default:
	db_stat = adh_error(adf_scb, E_AD1010_BAD_EMBEDDED_TYPE );
    }

    if (db_stat)
    {
	return db_stat;
    }

    /*
    ** If ed_null is set, regardless of whether it points to the value 
    ** DB_EMB_NULL, map it to a nullable data value.  See note in routine
    ** header.
    */
    if (adh_ev->ed_null != (i2 *)0 && adh_ev->ed_type != DB_DBV_TYPE
       && adh_ev->ed_type != DB_NUL_TYPE)
    {
       
	adh_dv->db_datatype = -adh_dv->db_datatype;
	adh_dv->db_data = adh_dvbuf->db_data;
	/*
	** When mapping null character data to a data value, the preprocessor-
	** generated length may be meaningless.  Choose a length that is as 
	** short as possible.  For CHR/CHA types, map to c1 rather than c0 for 
	** consistency with the DBMS; for VCH type, map to VCHAR(0).
	*/
	if (  (adh_ev->ed_type == DB_CHR_TYPE || adh_ev->ed_type == DB_CHA_TYPE)
	    && *adh_ev->ed_null == DB_EMB_NULL)		/* Really null */
	{
	    adh_dv->db_length = 1;
	}
	else if (   (   adh_ev->ed_type == DB_VCH_TYPE 
		     || adh_ev->ed_type == DB_VBYTE_TYPE)
		 && *adh_ev->ed_null == DB_EMB_NULL)
	{
	    adh_dv->db_length = DB_CNTSIZE;
	}
	if (!islong)
	{
	    adh_dv->db_length += 1;
	    if (adh_dv->db_length > adh_dvbuf->db_length)
	        adh_dv->db_length = adh_dvbuf->db_length;
        }
    }
    return db_stat;
}



/*{
+*  Name: adh_evcvtdb - Convert from an embedded value to an ADT.
**
**  Description:
**	This routine converts the data from an embedded program
**	into an internal data format.  
**
**	Note that to avoid extra copying of host language input data
**	adh_evtodb() will have already set the .db_data pointer to the
**	.ed_data pointer in those cases where no conversion is needed.
**	This routine can then just return.  If the embedded data is
**	null valued (.ed_type is DB_NUL_TYPE or .ed_null points to
**	DB_EMB_NULL), this routine just sets the null bit in the 
**	data value.  In all cases, afe_cvinto() is called to
**	do the data conversion.
**
**	DB_OBJ_TYPE is not allowed here.
**
**  Inputs:
**	adf_scb			A pointer to the current ADF_CB control block.
**	adh_ev			Pointer to a DB_EMBEDDED_DATA struct holding
**				embedded value to be converted.
**		.ed_type	The embedded data type
**		.ed_length	Its length
**		.ed_data	Points to the actual data.
**				If it is a null pointer, then the data is
**				null valued if (1) the i2 pointed to by
**				ed_null is DB_EMB_NULL, or (2) ed_type
**				is DB_NUL_TYPE.  The latter is the case of the
**				null constant.
**		.ed_null	Will either be a null pointer, or it will
**				point to an i2 containing the null indicator
**				for this value.  If the i2 is DB_EMB_NULL,
**				then the embedded data is null valued, and the
**				ADF value will be set to indicate it is null
**				valued.  If the i2 is 0 or ed_null is null,
**				then the embedded value is not null valued.
**	adh_dv			Pointer to the destination data value.
**		.db_datatype	The ADF type id for the type.
**		.db_length	The length of the type.
**		.db_data	Must point to an area to contain the
**				converted value.
**
**  Outputs:
**	adh_dv
**		.db_data	The area it points at will now contain an
**				ADF value corresponding to the embedded
**				value.
**
**  	Returns:
**		ED_DB_OK	If succeeded.
**		E_DB_ERROR	If some error occurred in the conversion
**				process.
**	    
**	Errors:
**
**	If a DB_STATUS code other than E_DB_OK is returned, the field
**	adf_scb.adf_errcb.ad_errcode will contain one of the following
**	codes:
**
**		E_AD1010_BAD_EMBEDDED_TYPE   The embedded type supplied is
**					     illegal
**		E_AD1012_NULL_TO_NONNULL     The embedded value is null-valued,
**					     but the ADF type is not nullable.
**		E_AD1013_EMBEDDED_NUMOVF     The requested numeric conversion
**					     would cause overflow.
**		Others from afe_tycoerce(), afe_cvinto(), e.g.
**		E_AD2009_NOCOERCION	     The requested conversion from
**					     the embedded type to the ADF
**					     type is illegal
**		E_AD2004_BAD_DTID	     The datatype id is unknown to ADF.
**		E_AD1080_STR_TO_DECIMAL	     Embedded value cannot be converted
**					     to a  decimal number.
**		E_AD1081_STR_TO_FLOAT	     Embedded value cannot be converted
**					     to a float number.
-*
**  Side Effects:
**	None
**	
**  History:
**	16-mar-1987 - written (bjb)
**	28-oct-1988 - modified to call afe_tycoerce and afe_cvinto for
**		      conversions.  No longer needs math exceptions around
**		      conversions. Removed adh_tabs argument as it is no
**		      longer used. (ncg)
**	03-mar-1989 - Fixed bug #4825: a repeat query bug whereby character
**		      data sent in nullable varchar format is incorrectly
**		      interpreted by the dbms.  The dbms relies on db_t_count
**		      alone instead of db_length AND db_t_count.  For
**		      null valued data, db_t_count is not defined; however
**		      our fix is to allow the dbms to continue to rely on
**		      db_t_count for null and non-null data.
**	13-jul-1990 (barbara)
**	    Added decimal support for 6.5/Orion.  Allow two new type,
**	    DB_DEC_TYPE and DB_DEC_CHR_TYPE.
**	31-oct-1990 (teresal)
**	    Update CVpka interface - CVpka parameters have changed.
**	29-nov-91 (seg)
**	    Can't dereference or do arithmetic on PTR
**	2/93 (Mike S) 
**	    Make being passed a DB_OBJ_TYPE an error.
**	20-jul-1993 (kathryn)
**	    Ensure that count field and nullable byte get set correctly for
**	    -DB_LTXT_TYPE. (-DB_LTXT_TYPE is sent for DB_NUL_TYPE).
**	07-mar-2001 (abbjo03)
**	    Add support for DB_NCHR_TYPE and DB_NVCHR_TYPE.
**	13-mar-2001 (abbjo03)
**	    Additional support for DB_NVCHR_TYPEs.
**	03-aug-2006 (gupsh01)
**	    Added support for new ANSI Date/time types.
*/

DB_STATUS
adh_evcvtdb( adf_scb, adh_ev, adh_dv )
ADF_CB			*adf_scb;
DB_EMBEDDED_DATA	*adh_ev;
DB_DATA_VALUE		*adh_dv;
{
    DB_STATUS		db_stat = E_DB_OK;
    DB_DATA_VALUE	*dvp;
    DB_DATA_VALUE	dv;
    DB_TEXT_STRING	*db_txt;
    i4			isnull = FALSE;
    i4			len, prec, scal;
    char		dec_buf[DB_MAX_DECLEN];
    f8			tmp_flt;

    /* Check that the embedded type is correct */
    if (    adh_ev->ed_type != DB_INT_TYPE
	&&  adh_ev->ed_type != DB_FLT_TYPE  	    
	&&  adh_ev->ed_type != DB_DEC_TYPE  	    
	&&  adh_ev->ed_type != DB_CHR_TYPE  	    
	&&  adh_ev->ed_type != DB_CHA_TYPE  	    
	&&  adh_ev->ed_type != DB_NCHR_TYPE  	    
	&&  adh_ev->ed_type != DB_VCH_TYPE  	    
	&&  adh_ev->ed_type != DB_NVCHR_TYPE  	    
	&&  adh_ev->ed_type != DB_DBV_TYPE  	    
	&&  adh_ev->ed_type != DB_DTE_TYPE
	&&  adh_ev->ed_type != DB_ADTE_TYPE
	&&  adh_ev->ed_type != DB_TMWO_TYPE
	&&  adh_ev->ed_type != DB_TMW_TYPE
	&&  adh_ev->ed_type != DB_TME_TYPE
	&&  adh_ev->ed_type != DB_TSWO_TYPE
	&&  adh_ev->ed_type != DB_TSW_TYPE
	&&  adh_ev->ed_type != DB_TSTMP_TYPE
	&&  adh_ev->ed_type != DB_INYM_TYPE
	&&  adh_ev->ed_type != DB_INDS_TYPE
	&&  adh_ev->ed_type != DB_NUL_TYPE  	    
	&&  adh_ev->ed_type != DB_DEC_CHR_TYPE
	&&  adh_ev->ed_type != DB_BYTE_TYPE  	    
	&&  adh_ev->ed_type != DB_VBYTE_TYPE  	    
       )
    {
	return (adh_error(adf_scb, E_AD1010_BAD_EMBEDDED_TYPE ));
    }

    /*
    ** We should never see DB_OBJ_TYPE here.
    */
    if (adh_dv->db_datatype == DB_OBJ_TYPE)
    {
	return (adh_error(adf_scb, E_AD2004_BAD_DTID ));
    }

    /* 
    ** If the data pointers are the same, no conversion is necessary.
    ** This includes the case where the embedded type is DBV (as long
    ** as the destination data's type is not nullable VARCHAR or TEXT).
    */
    if (   adh_ev->ed_data == adh_dv->db_data
	|| (   adh_ev->ed_type == DB_DBV_TYPE
	    && adh_dv->db_datatype != -DB_VCH_TYPE
	    && adh_dv->db_datatype != -DB_TXT_TYPE
	    && adh_dv->db_datatype != -DB_VBYTE_TYPE
	    && ((DB_DATA_VALUE*)adh_ev->ed_data)->db_data == adh_dv->db_data
           )
       )
    {
	    return E_DB_OK;
    }

    /*
    ** If the embedded data is null-valued, return an error if the data value 
    ** is not nullable.  If the data value is nullable and the embedded type 
    ** is coercible to the data type, set the null bit and return.
    */
    if (    adh_ev->ed_type == DB_NUL_TYPE
	|| (adh_ev->ed_null != (i2 *)0 && *adh_ev->ed_null == DB_EMB_NULL)
	|| (   adh_ev->ed_type == DB_DBV_TYPE
	    && ADH_ISNULL_MACRO((DB_DATA_VALUE *)adh_ev->ed_data)
	   )
       )
    {
	DB_DT_ID in_type;

	if (adh_ev->ed_type == DB_DBV_TYPE)
	    in_type = ((DB_DATA_VALUE *)adh_ev->ed_data)->db_datatype;
	else
	    in_type = adh_ev->ed_type;

	if (adh_dv->db_datatype > 0)
	    return (adh_error(adf_scb, E_AD1012_NULL_TO_NONNULL ));

	if (in_type != DB_NUL_TYPE)
	{
	    if (!afe_tycoerce(adf_scb, in_type, adh_dv->db_datatype))
		return E_DB_ERROR;
	}

	if (   adh_dv->db_datatype == -DB_VCH_TYPE
	    || adh_dv->db_datatype == -DB_LTXT_TYPE
	    || adh_dv->db_datatype == -DB_VBYTE_TYPE
	    || adh_dv->db_datatype == -DB_TXT_TYPE)
	{
	/* 
	** Bug #4825 -- Destination data of type nullable TXT or VCH
	** must set count field and nullable byte for DBMS repeat qry bug.
	*/
	    db_txt = (DB_TEXT_STRING *)adh_dv->db_data;
	    db_txt->db_t_count = 0;
	    db_txt->db_t_text[0] = ADF_NVL_BIT;
	}

	ADH_SETNULL_MACRO(adh_dv);
	return E_DB_OK;
    }

    /*
    ** Embedded DBV's that went through adh_evtodb() (usually where
    ** the caller is LIBQ) and embedded null data have been
    ** dealt with above.  
    */
    
    if (adh_ev->ed_type == DB_DBV_TYPE)
    {
	dvp = (DB_DATA_VALUE *)adh_ev->ed_data;
    }
    else
    {
	dv.db_data = adh_ev->ed_data;
	dv.db_datatype = adh_ev->ed_type;
	dv.db_prec = 0;

	switch (adh_ev->ed_type)
	{
	  case DB_DEC_CHR_TYPE:
	  /* 
	  ** Preprocessor generated type meaning decimal encoded as string
	  */
	    if (STindex(adh_ev->ed_data, ERx("."), 0) == (char *)0)
	    /* If no decimal point treat as float in keeping with dbms */
	    {
		dv.db_datatype = DB_FLT_TYPE;
		dv.db_length = sizeof(f8);
		if (CVaf(adh_ev->ed_data, '.', &tmp_flt) != OK)
		    return adh_error(adf_scb, E_AD1081_STR_TO_FLOAT );
		dv.db_data = (PTR)&tmp_flt;
	    }		
	    else if (afe_dec_info(
			adh_ev->ed_data, '.', &len, &prec, &scal ) != OK)
	    {
		return adh_error(adf_scb, E_AD1080_STR_TO_DECIMAL );
	    }
	    else
	    {
		dv.db_prec = DB_PS_ENCODE_MACRO( prec, scal );
		dv.db_length = len;
		dv.db_datatype = DB_DEC_TYPE;
		if (CVapk( adh_ev->ed_data, '.', prec, scal, dec_buf ) != OK)
		    return adh_error(adf_scb, E_AD1080_STR_TO_DECIMAL );
		dv.db_data = dec_buf;
	    }
	    break;

	  case DB_DEC_TYPE:
	    dv.db_prec = adh_ev->ed_length;
	    prec = DB_P_DECODE_MACRO( dv.db_prec );
	    dv.db_length = DB_PREC_TO_LEN_MACRO( prec );
	    break;

	  case DB_CHR_TYPE:
	  {
	    II_THR_CB *thr_cb = IILQthThread();
	    II_LBQ_CB *IIlbqcb = thr_cb->ii_th_session;

	    /*
	    ** Check for EOS if preprocessor flag "-check_eos" was used
	    */
	    if (IIlbqcb->ii_lq_flags & II_L_CHREOS)
	    {
		if (adh_chkeos((char *)adh_ev->ed_data, 
				(i4)adh_ev->ed_length) != OK)
		{
		   /* Change following when the new ADH error has been added */
		   /* return adh_error(adf_scb, E_ADXXXX_UNTERM_C_STR ) */
	    	   return adh_error(adf_scb, E_AD9999_INTERNAL_ERROR );
		   break;
		}
	    }
	    if (adh_ev->ed_length == 0)
		dv.db_length = STlength( adh_ev->ed_data );
	    else 
		dv.db_length = adh_ev->ed_length;
	   }
	   break;

	  case DB_NCHR_TYPE:
	    if (adh_ev->ed_length)
		dv.db_length = adh_ev->ed_length;
	    else 
		dv.db_length = wcslen((wchar_t *)adh_ev->ed_data) *
		    sizeof(wchar_t);
	    break;

	  case DB_NVCHR_TYPE:
	    if (adh_ev->ed_length)
		dv.db_length = adh_ev->ed_length;
	    else 
		dv.db_length =
		    ((AFE_NTEXT_STRING *)adh_ev->ed_data)->afe_t_count *
		    sizeof(wchar_t) + DB_CNTSIZE;
	    break;

	  case DB_VCH_TYPE:
	  case DB_VBYTE_TYPE:
	    if (adh_ev->ed_length == 0)
		dv.db_length = ((DB_TEXT_STRING *)adh_ev->ed_data)->db_t_count
					+DB_CNTSIZE;
	    else
		dv.db_length = adh_ev->ed_length;
	    break;
	  
	  case DB_DTE_TYPE:
	    dv.db_length = ADH_INTLEN_DTE;
	    break;

          case DB_ADTE_TYPE:
	    adh_dv->db_length = ADH_INTLEN_ADTE;
	    break;

          case DB_TMWO_TYPE:
          case DB_TMW_TYPE:
          case DB_TME_TYPE:
	    adh_dv->db_length = ADH_INTLEN_TIME;
	    break;

          case DB_TSWO_TYPE:
          case DB_TSW_TYPE:
          case DB_TSTMP_TYPE:
	    adh_dv->db_length = ADH_INTLEN_TSTMP;
	    break;

          case DB_INDS_TYPE:
  	    adh_dv->db_length = ADH_INTLEN_INTDS;
	    break;

          case DB_INYM_TYPE:
	    adh_dv->db_length = ADH_INTLEN_INTYM;
	    break;

	  default:
	    dv.db_length = adh_ev->ed_length;
	}
	dvp = &dv;
    }

    /* 
    ** After we call afe_cvinto(), check for overflow errors and warnings that
    ** are converted to embedded overflow errors.
    */

    db_stat = afe_cvinto(adf_scb, dvp, adh_dv);

    if (   adh_dv->db_datatype == -DB_VCH_TYPE
	|| adh_dv->db_datatype == -DB_TXT_TYPE
	|| adh_dv->db_datatype == -DB_VBYTE_TYPE
       )
    {
	db_txt = (DB_TEXT_STRING *)adh_dv->db_data;
	db_txt->db_t_text[db_txt->db_t_count] = 0;
    }
    if (db_stat != E_DB_OK && ADH_OVF_MACRO(adf_scb->adf_errcb.ad_errcode))
	adf_scb->adf_errcb.ad_errcode = E_AD1013_EMBEDDED_NUMOVF;
    return db_stat;
}



/*{
+*  Name: adh_dbtoev - Get embedded type from ADF type
**
**  Description:
**	This routine is used to choose the embedded type and length that 
**	a given datatype and length should be converted into by the
**	adh_dbcvtev() routine.
**
**	DB_OBJ_TYPE remains DB_INT_TYPE.  This should avoid portability
**	problems on machines where sizeof(i4) != sizeof(PTR).
**
**	With the introduction of decimal, this function needs to know whether
**	to map DB_DEC_TYPE to float or to decimal.  For now, it always maps
**	to float because its callers (OPTIMIZEDB and FRS) are dealing with
**	internal C data.  However, it might also be called by the ABF runtime
**	system, and this is an example of situation where a dbv of DB_DEC_TYPE
**	is expected to map to decimal for some languages.  To do this, we
**	need a frontcl routine which might have compiler dependent information
**	e.g. is decimal supported?  DCLGEN could take advantage of this type
**	of info, too.
**
**  Inputs:
**	adf_scb			A pointer to the current ADF_CB
**				control block.
**	adh_dv			A pointer to a DB_DATA_VALUE structure
**				holding the datatype and length to be
**				mapped.
**		.db_datatype	The ADF type id.
**		.db_length	The length of the type.
**	adh_ev			Must point to a DB_EMBEDDED_DATA.
**
**  Outputs:
**	adh_ev			The DB_EMBEDDED_DATA containing the mapping.
**		.ed_type	The embedded data type that corresponds
**				to the ADF type.
**		.ed_length	The maximum length any value can take.
**				This length does not include the null byte
**				on nullable types.  Note that when the 
**				chosen type (.ed_type) is CHR, the length 
**				does not include the EOS byte.
**
**	Returns:
**		E_DB_OK		If succeeded.
**		E_DB_ERROR	If some error occurred in the mapping process.
**
**	Errors:
**
**	If a DB_STATUS code other than E_DB_OK is returned, the field
**	adf_scb.adf_errcb.ad_errcode will contain one of the following
**	codes:
**		E_AD2004_BAD_DTID	Datatype id unknown to ADF.
-*
**  Side Effects:
**	None.
**	
**  History:
**	16-mar-1987 - written (bjb)
**	13-jul-1990 (barbara)
**	    Added decimal support for 6.5/Orion.  See notes added to
**	    function description above.
**	2/93 (Mike S) Make the DB_OBJ_TYPE embedded values distinguishable.
**	01-oct-1993 (sandyd)
**	    Added BYTE as a legal type.
**	18-nov-93 (robf)
**          Added security_label as a legal type, passed as internal 
**	    binary label.
**	03-aug-2006 (gupsh01)
**	    Added support for new ANSI date/time types.
*/

DB_STATUS
adh_dbtoev(adf_scb, adh_dv, adh_ev)
ADF_CB			*adf_scb;
DB_DATA_VALUE		*adh_dv;
DB_EMBEDDED_DATA	*adh_ev;
{
    DB_STATUS		db_stat = E_DB_OK;

    switch (ADH_BASETYPE_MACRO(adh_dv))
    {
      case DB_INT_TYPE:
      case DB_FLT_TYPE:
      case DB_CHR_TYPE:
      case DB_BYTE_TYPE:
	adh_ev->ed_type = ADH_BASETYPE_MACRO(adh_dv);
	adh_ev->ed_length = adh_dv->db_length;
	if (ADH_NULLABLE_MACRO(adh_dv))
	    adh_ev->ed_length--;
	break;

      case DB_OBJ_TYPE:
	adh_ev->ed_type = DB_OBJ_TYPE;
	adh_ev->ed_length = sizeof(PTR);
	break;

      case DB_CHA_TYPE:
	adh_ev->ed_type = DB_CHR_TYPE;
	adh_ev->ed_length = adh_dv->db_length;
	if (ADH_NULLABLE_MACRO(adh_dv))
	    adh_ev->ed_length--;
	break;

      case DB_TXT_TYPE:
      case DB_VCH_TYPE:
      case DB_LTXT_TYPE:
      case DB_VBYTE_TYPE:
	adh_ev->ed_type = DB_CHR_TYPE;
	adh_ev->ed_length = adh_dv->db_length -DB_CNTSIZE;
	if (ADH_NULLABLE_MACRO(adh_dv))
	    adh_ev->ed_length--;
	break;

      case DB_DTE_TYPE:
	adh_ev->ed_type = DB_CHR_TYPE;
	adh_ev->ed_length = ADH_EXTLEN_DTE;
	break;

      case DB_ADTE_TYPE:
	adh_ev->ed_type = DB_CHR_TYPE;
	adh_ev->ed_length = ADH_EXTLEN_ADTE;
	break;

      case DB_TMWO_TYPE:
	adh_ev->ed_type = DB_CHR_TYPE;
	adh_ev->ed_length = ADH_EXTLEN_TMWO;
	break;

      case DB_TMW_TYPE:
	adh_ev->ed_type = DB_CHR_TYPE;
	adh_ev->ed_length = ADH_EXTLEN_TMW;
	break;

      case DB_TME_TYPE:
	adh_ev->ed_type = DB_CHR_TYPE;
	adh_ev->ed_length = ADH_EXTLEN_TME;
	break;

      case DB_TSWO_TYPE:
	adh_ev->ed_type = DB_CHR_TYPE;
	adh_ev->ed_length = ADH_EXTLEN_TSWO;
	break;

      case DB_TSW_TYPE:
	adh_ev->ed_type = DB_CHR_TYPE;
	adh_ev->ed_length = ADH_EXTLEN_TSW;
	break;

      case DB_TSTMP_TYPE:
	adh_ev->ed_type = DB_CHR_TYPE;
	adh_ev->ed_length = ADH_EXTLEN_TSTMP;
	break;

      case DB_INDS_TYPE:
	adh_ev->ed_type = DB_CHR_TYPE;
	adh_ev->ed_length = ADH_EXTLEN_INTDS;
	break;

      case DB_INYM_TYPE:
	adh_ev->ed_type = DB_CHR_TYPE;
	adh_ev->ed_length = ADH_EXTLEN_INTYM;
	break;
 
      case DB_MNY_TYPE:
      case DB_DEC_TYPE:
	adh_ev->ed_type = DB_FLT_TYPE;
	adh_ev->ed_length = sizeof(f8);
	break;
    
      default:
	db_stat = adh_error(adf_scb, E_AD2004_BAD_DTID );
	break;
    }
    return db_stat;
}



/*{
+*  Name: adh_dbcvtev - Convert an ADT to an embedded value.
**
**  Description:
**	This routine is used to convert a data value into an embedded
**	value.
**
**	Some data values may not be converted into some embedded types.
**	For instance, a numeric type is not coercible into a user
**	character type.
**
**	The routine afe_cvinto() is called to do the actual data
**	conversion.  However, if the data to be converted is null
**	valued, no conversion is necessary and this routine just sets
**	the .ed_null field of the DB_EMBEDDED_DATA.  Note also that for 
**	character conversions a special conversion routine, adh_chrcvt() 
**	is called.  A special conversion routine is needed because 
**	(1) .ed_length can legally be greater than DB_CHAR_MAX;
**	(2) conversion of escape characters should be allowed;
**	(3) embedded CHR variables need null terminating; and
**	(4) trailing blanks from CHR data need to show up in user variables.
**	afe_cvinto() finds these cases to be in error.
**
**	Also, if the dbv type is DB_OBJ_TYPE, that is special cased here, since
**	ADF does not know about that type, and this is a prototype
**	implementation.
**
**  Inputs:
**	adf_scb			A pointer to the current ADF_CB
**				control block.
**	adh_dv			Pointer to a DB_DATA_VALUE structure
**				holding the data value to be converted.
**		.db_datatype	The ADF type id.
**		.db_length	The length of the type.
**		.db_data	The internal value to convert.
**	adh_ev			Must point to a DB_EMBEDDED_DATA.
**		.ed_type	The embedded data type.
**		.ed_length	Its length
**		.ed_data	Pointer to the embedded variable
**		.ed_null	Pointer to the embedded null indicator.
**				If this pointer is null, then there is no
**				null indicator.  In this case, an attempt
**				to convert a null value will generate an error.
**
**  Outputs:
**	adh_ev			The DB_EMBEDDED_DATA into which the data is 
**				converted.
**		.ed_data	Will contain the converted value.
**		.ed_null	If this is a non-null pointer, the i2 it
**				points to will contain 0 if data being
**				converted is non-null, or DB_EMB_NULL if
**				the value being converted is null.
**				If this is a string convertion and truncation
**				is required, this will be set to original
**				length of the string.
**
**	Returns:
**	    	E_DB_OK		If succeeded.
**	 	E_DB_ERROR	If some error occurred in the conversion.
**	Errors:
**
**	If a DB_STATUS code other than E_DB_OK is returned, the field
**	adf_scb.adf_errcb.ad_errcode will contain one of the following
**	codes:
**		E_AD1012_NULL_TO_NONNULL The ADF value to be converted is
**					null valued, but the embedded
**					variable does not have a null
**					indicator.
**		E_AD1013_EMBEDDED_NUMOVF The requested numeric conversion
**					would cause overflow.
**		E_AD0101_EMBEDDED_CHAR_TRUNC The ADF charater type was truncated
**					on conversion into the embedded type.
**		Other error codes from afe_cvinto(), afe_tycoerce(), 
**					adh_chrcvt(), e.g.
**	    	E_AD2004_BAD_DTID	Datatype id unknown to ADF
**		E_AD2009_NOCOERCION	The requested conversion from
**					the datatype into the embedded type
**					is not allowed.
-*
**  Side Effects:
**	None.
**	
**  History:
**	16-mar-1987 - written (bjb)
**	13-may-1988 - modified to allow dates to and from CHR types to
**		      work correctly, and to truncate if required. Bug
**		      2629. (ncg)
**	28-oct-1988 - modified to call afe_tycoerce and afe_cvinto for
**		      conversions.  No longer needs math exceptions around
**		      conversions. (ncg)
**	13-jul-1990 (barbara)
*	    Updated for decimal in 6.5/Orion.
**	29-nov-91 (seg)
**	    Can't dereference or do arithmetic on PTR
**	1-apr-93 (robf)
**	  Handle security_label->string conversions, previously fell
**	  through to an empty size string.
**	15-sep-93 (sandyd)
**	    Embedded types can now include DB_VBYTE_TYPE, which should get
**	    the same treatment as embedded DB_VCH_TYPE.
**	01-oct-93 (sandyd)
**	    Embedded types can also include DB_BYTE_TYPE (when using SQLDA).
**	18-nov-93 (robf)
**          Allow  for embedded char truncation for DB_SEC_TYPE
**      14-feb-94 (johnst)
**          Bug #59977
**          When copying object pointers, use type-correct (PTR *) casts when
**	    de-referencing db_data and ed_data instead of original (i4 *) cast.
**	    This makes a difference for DEC alpha (axp_osf) where pointers are 
**	    64-bit and ints are 32-bit, so casting a pointer as an int caused
**	    truncation of pointer values.
**	26-may-94 (robf)
**          Conversion from security_label to varbyte was failing, due to 
**	    cut & paste errors. Reworked code to copy correctly.
**	26-oct-2004 (gupsh01)
**	    Added code to handle unicode coercion in the front end.
**	16-jun-2005 (gupsh01)
**	    Do not call adf_ucolinit(). The unicode collation table
**	    will not be initialized based on the database type we are 
**	    connecting to.
**      28-jul-2005 (stial01)
**          DO call adh_ucolinit(). Previous change moved adh_ucolinit to 
**          feingres.qsc which works for tm, but not for esqlc. (b114944)
**	02-aug-2006 (gupsh01)
**	    Added support for ANSI date/time data types.
**      15-Jul-2010 (hanal04) Bug 124087
**          Comments for adh_chrcvt() explain why embedded string variables 
**          should not use afe_cvinto() but we still were for the unicoerce
**          case. Call the new adu_nvchr_embchartouni() or
**          adu_nvchr_embunitochar() functions as required when dealing
**          with unicode coercions.
*/

DB_STATUS
adh_dbcvtev(adf_scb, adh_dv, adh_ev)
ADF_CB			*adf_scb;
DB_DATA_VALUE		*adh_dv;
DB_EMBEDDED_DATA	*adh_ev;
{
    DB_STATUS		db_stat = E_DB_OK;
    DB_DATA_VALUE	dv, *dvp;
    i4			prec;
    i4			dtextlen = 0;
    bool  		adh_date_class = FALSE;
    i4			unicoerce = FALSE;

    adf_scb->adf_errcb.ad_errcode = E_DB_OK;	/* Default */
    /*
    ** Check for DB_OBJ_TYPE right away.  If the dv is DB_OBJ_TYPE and
    ** the ev is DB_OBJ_TYPE of length sizeof(PTR), then do conversion
    ** here.  If not, fall through and let ADF give error because of
    ** bad type.
    */
    if (adh_dv->db_datatype == DB_OBJ_TYPE
	&& adh_ev->ed_type == DB_OBJ_TYPE
	&& adh_ev->ed_length == sizeof(PTR))
    {
	/*
	** Move object pointer
	*/

        * ((PTR *) adh_ev->ed_data) = *((PTR *) adh_dv->db_data);
	return E_DB_OK;
    }

    /*
    ** Assume that data to be converted is not null valued and set
    ** embedded null indicator accordingly.
    */
    if (adh_ev->ed_null)
	*adh_ev->ed_null = 0;

    /*
    ** Conversion of a null-valued DBV consists of setting the .ed_null field
    ** in the embedded data.  The afe_tycoerce() routine is used to verify 
    ** that the data conversion is legal.
    */
    if (ADH_ISNULL_MACRO(adh_dv) && adh_ev->ed_type != DB_DBV_TYPE)
    {
	if (!afe_tycoerce(adf_scb, adh_ev->ed_type, adh_dv->db_datatype))
	    return E_DB_ERROR;
	if (adh_ev->ed_null == (i2 *)0)
	    return (adh_error(adf_scb, E_AD1012_NULL_TO_NONNULL ));
	*adh_ev->ed_null = DB_EMB_NULL;
	return E_DB_OK;
    }

    /*
    ** Legal conversions into embedded character types from character ADTs are 
    ** treated separately for reasons stated in the routine header.  However, 
    ** conversion of DTE values into embedded character types, conversion of
    ** character types into embedded DBVS, and illegal character conversions 
    ** drop through afe_cvinto().
    */
    unicoerce = adh_dounicoerce (adh_dv, adh_ev);
    if (unicoerce != 0)
    {
	II_THR_CB *thr_cb = IILQthThread();
        II_LBQ_CB *IIlbqcb = thr_cb->ii_th_session;

	db_stat = adh_ucolinit(IIlbqcb, 0);
    }

    if (   (   adh_ev->ed_type == DB_CHR_TYPE 
	    || adh_ev->ed_type == DB_VCH_TYPE
	    || adh_ev->ed_type == DB_VBYTE_TYPE
	    || adh_ev->ed_type == DB_CHA_TYPE
	    || adh_ev->ed_type == DB_NCHR_TYPE
	    || adh_ev->ed_type == DB_NVCHR_TYPE
	    || adh_ev->ed_type == DB_BYTE_TYPE
	   )
	&& (    abs(adh_dv->db_datatype) == DB_CHR_TYPE
	    ||  abs(adh_dv->db_datatype) == DB_CHA_TYPE
	    ||  abs(adh_dv->db_datatype) == DB_TXT_TYPE
	    ||  abs(adh_dv->db_datatype) == DB_VCH_TYPE
	    ||  abs(adh_dv->db_datatype) == DB_NCHR_TYPE
	    ||  abs(adh_dv->db_datatype) == DB_NVCHR_TYPE
	    ||  abs(adh_dv->db_datatype) == DB_LTXT_TYPE
	    ||  abs(adh_dv->db_datatype) == DB_VBYTE_TYPE
	    ||  abs(adh_dv->db_datatype) == DB_BYTE_TYPE
	   )
       ) 
    {
	if (unicoerce)
        {
            dv.db_datatype = adh_ev->ed_type;
            dv.db_length = adh_ev->ed_length;
            dv.db_prec = 0;
            dv.db_data = adh_ev->ed_data;
            dvp = &dv;

            if (unicoerce > 0)
            {
                /* Unicode to non-unicode coercion required */
                return(adu_nvchr_embunitochar(adf_scb, adh_dv, dvp));
            }
            else if (unicoerce < 0)
            {
                /* Non-unicode to unicode coercion required */
                return(adu_nvchr_embchartouni(adf_scb, adh_dv, dvp));
            }
        }
        else
        {
	    return (adh_chrcvt(adf_scb, adh_dv, adh_ev));
        }
    }

   if ((abs(adh_dv->db_datatype) == DB_DTE_TYPE) ||
       (abs(adh_dv->db_datatype) == DB_ADTE_TYPE) ||
       (abs(adh_dv->db_datatype) == DB_TMWO_TYPE) ||
       (abs(adh_dv->db_datatype) == DB_TMW_TYPE) ||
       (abs(adh_dv->db_datatype) == DB_TME_TYPE) ||
       (abs(adh_dv->db_datatype) == DB_TSWO_TYPE) ||
       (abs(adh_dv->db_datatype) == DB_TSW_TYPE) ||
       (abs(adh_dv->db_datatype) == DB_TSTMP_TYPE) ||
       (abs(adh_dv->db_datatype) == DB_INDS_TYPE) ||
       (abs(adh_dv->db_datatype) == DB_INYM_TYPE)
      )
    {
      adh_date_class = TRUE;
      switch (abs(adh_dv->db_datatype))
      {
	case DB_DTE_TYPE:
	  dtextlen = ADH_EXTLEN_DTE;
	break;

	case DB_ADTE_TYPE:
	  dtextlen = ADH_EXTLEN_ADTE;
	break;

	case DB_TMWO_TYPE:
	  dtextlen = ADH_EXTLEN_TMWO;
	break;

	case DB_TMW_TYPE:
	  dtextlen = ADH_EXTLEN_TMW;
	break;

	case DB_TME_TYPE:
	  dtextlen = ADH_EXTLEN_TME;
	break;

	case DB_TSTMP_TYPE:
	  dtextlen = ADH_EXTLEN_TSTMP;
	break;

	case DB_TSWO_TYPE:
	  dtextlen = ADH_EXTLEN_TSWO;
	break;

	case DB_TSW_TYPE:
	  dtextlen = ADH_EXTLEN_TSW;
	break;

	case DB_INYM_TYPE:
	  dtextlen = ADH_EXTLEN_INTYM;
	break;

	case DB_INDS_TYPE:
	  dtextlen = ADH_EXTLEN_INTDS;
	break;
      }
    }
    
    if (adh_ev->ed_type == DB_DBV_TYPE)
    {
	dvp = (DB_DATA_VALUE *)adh_ev->ed_data;
    }
    else
    {
	dv.db_datatype = adh_ev->ed_type;
	dv.db_prec = 0;
	dv.db_data = adh_ev->ed_data;

	/* Handle special cases of CHR for dates/security labels, 
	** and VCH with zero length.
	*/
	if (adh_ev->ed_type == DB_CHR_TYPE)
	{
	    /* 
	    ** CHR characters are not blank padded, but if zero-length are
	    ** assumed exact fits.  Must be a date here, as strings are
	    ** handled earlier.
	    */
	    if (adh_date_class == TRUE)
	    {
		  if (   adh_ev->ed_length == 0
		      || adh_ev->ed_length > dtextlen)
		  {
		    /* Exact match or truncate early */
		    dv.db_length = dtextlen;
		  }
		  else
		  {
		    /* Must be smaller than date length */
		    dv.db_length = adh_ev->ed_length;
		  }
	    }
	    else
	    {
                /* Bad conversion; don't risk overwriting */
                dv.db_length = 0;
	    }
	}
	else if (adh_ev->ed_type == DB_DEC_TYPE)
	{
	    dv.db_prec = adh_ev->ed_length;
	    prec = DB_P_DECODE_MACRO( adh_ev->ed_length );
	    dv.db_length = DB_PREC_TO_LEN_MACRO( prec );
	}
    	else if (adh_ev->ed_type == DB_VCH_TYPE)
	{
	    /* 
	    ** VCH characters may be zero-length and must be adjusted
	    ** to be the correct size.  Here too they must be dates.
	    */
	    if (adh_date_class == TRUE)
	    {
		  if (adh_ev->ed_length == 0)
		  {
		    /* Exact match */
		    dv.db_length = dtextlen + DB_CNTSIZE;
		  }
		  else
		  {
		    /* Must be a defined length */
		    dv.db_length = adh_ev->ed_length;
		  }
	     }
	     else
	     {
		/* Bad conversion; don't risk overwriting */
		  dv.db_length = DB_CNTSIZE;
	     }
	}
	else if (adh_ev->ed_type == DB_DTE_TYPE)
	{
	    dv.db_length = ADH_INTLEN_DTE;
	}
	else if (adh_ev->ed_type == DB_ADTE_TYPE)
	{
	    dv.db_length = ADH_INTLEN_ADTE;
	}
	else if ((adh_ev->ed_type == DB_TME_TYPE)
	 	|| (adh_ev->ed_type == DB_TMW_TYPE)
		|| (adh_ev->ed_type == DB_TMWO_TYPE))

	{
	    dv.db_length = ADH_INTLEN_TIME;
	}
	else if ((adh_ev->ed_type == DB_TSTMP_TYPE)
	 	|| (adh_ev->ed_type == DB_TSW_TYPE)
		|| (adh_ev->ed_type == DB_TSWO_TYPE))
	{
	    dv.db_length = ADH_INTLEN_TSTMP;
	}
	else if (adh_ev->ed_type == DB_INDS_TYPE)
	{
	    dv.db_length = ADH_INTLEN_INTDS;
	}
	else if (adh_ev->ed_type == DB_INYM_TYPE)
	{
	    dv.db_length = ADH_INTLEN_INTYM;
	}
	else
	{
	    dv.db_length = adh_ev->ed_length;
	}

	dvp = &dv;
    }

    /* 
    ** After we call afe_cvinto(), check for overflow errors and warnings that
    ** are converted to embedded overflow errors.
    */

    db_stat = afe_cvinto(adf_scb, adh_dv, dvp);

    if (db_stat != E_DB_OK)
    {
	if (ADH_OVF_MACRO(adf_scb->adf_errcb.ad_errcode))
	    adf_scb->adf_errcb.ad_errcode = E_AD1013_EMBEDDED_NUMOVF;
	return db_stat;
    }

    /*
    ** For DTE/SECURITY_LABEL to character conversion, null terminate 
    ** (embedded CHR only) and check for truncation.
    */
    if (adh_date_class == TRUE)
    {
      /* Truncated character strings don't want overhead of adh_error */
      if (    (   adh_ev->ed_type == DB_CHR_TYPE
            || adh_ev->ed_type == DB_CHA_TYPE
	)
        &&  (dvp->db_length < dtextlen)
        )
      {
	adf_scb->adf_errcb.ad_errcode = E_AD0101_EMBEDDED_CHAR_TRUNC;
      }
      else if (    adh_ev->ed_type == DB_VCH_TYPE
	    && (dvp->db_length - DB_CNTSIZE < dtextlen)
	    )
      {
  	adf_scb->adf_errcb.ad_errcode = E_AD0101_EMBEDDED_CHAR_TRUNC;
      }

      if (adh_ev->ed_type == DB_CHR_TYPE)
      {
	((char *)adh_ev->ed_data)[dvp->db_length] = '\0';
      }
    }

    return db_stat;
}


/*{
+*  Name: adh_chrcvt - Convert a character ADT to an embedded CHR type.
**
**  Description:
**	This is a local function which converts ADTs of types CHR, CHA,
**	TXT, LTXT, VCH, BYTE, and VBYTE to embedded CHR, CHA, VCH, BYTE,
**	and VBYTE types.
**	It is called instead of afe_cvinto() from adh_dbcvtev() because:
**	(1) Embedded character variables may be greater than DB_CHAR_MAX,
**	    but afe_cvinto() would treat this as an error.
**	(2) Conversion of data containing escape characters into embedded 
**	    character variables should be allowed, but afe_cvinto() would 
**	    treat this as an error.
**	(3) Embedded CHR variables should not be blank padded to their 
**	    given length (their length might not be known because the 
**	    preprocessor did not generate a length and is therefore assumed 
**	    to be as big as the source data length)
**	(4) Trailing blanks on CHR type data values should get copied into
**	    user variables.  This is a documented feature.
**
** 	This routine assumes 
**	(1) That it won't be called if the data to be converted is null valued.
**	(2) It won't be called if the data to be converted is DTE type.
**	    DTE conversions will have gone through afe_cvinto().
**
**  Inputs:
**	adf_scb			A pointer to the current ADF_CB
**				control block.
**	adh_dv			Pointer to a DB_DATA_VALUE structure
**				holding the data value to be converted.
**		.db_datatype	The ADF type id (a character type).
**		.db_length	The length of the type.
**		.db_data	The internal value to convert.
**	adh_ev			Must point to a DB_EMBEDDED_DATA.
**		.ed_type	The embedded data type (CHR or VCH).
**		.ed_length	Its length.
**		.ed_data	Pointer to the embedded variable.
**	
**  Outputs:
**	adh_ev			The DB_EMBEDDED_DATA into which the data is 
**				converted.
**		.ed_data	Will contain the converted value.
**		.ed_null	If truncation occurred, will be set to original
**				length of string.
**
**	Returns:
**	    	E_DB_OK		If succeeded.
**	 	E_DB_ERROR	If some error occurred in the conversion.
**	Errors:
**
**	If a DB_STATUS code other than E_DB_OK is returned, the field
**	adf_scb.adf_errcb.ad_errcode will contain one of the following
**	codes:
**	    	E_AD2009_NOCOERCION	The requested conversion from the
**					datatype into the embedded character 
**					type is not allowed.
**		E_AD0101_EMBEDDED_CHAR_TRUNC The ADF type was truncated on
**					conversion into the embedded type.
-*
**  Side Effects:
**	None
**	
**  History:
**	16-mar-1987 - written (bjb)
**	28-oct-1988 - modified final copies to reduce overhead of CMcopy (ncg)
**	21-nov-1988 - set indicator var to original length on truncation for
**		      ANSI compatibility. (ncg)
**	29-nov-91 (seg)
**	    Can't dereference or do arithmetic on PTR
**	10-jun-1993 (kathryn)
**	    After copying data to application variable - call adh_charend to
**	    blank-pad, EOS terminate or both(FIPS).
**	15-sep-1993 (sandyd)
**	    Added a few more pieces needed for the handling of DB_VBYTE_TYPE 
**	    as an embedded type.  This type should receive the same handling as 
**	    DB_VCH_TYPE (including truncation warnings).
**	16-sep-1993 (kathryn)
**	    Call adh_charend for DB_BYTE_TYPE to null pad fixed length byte
**	    strings.
**	01-oct-1993 (sandyd)
**	    Don't use CMcopy() to truncate strings of "byte" datatypes.
**	    Those should not be interpreted as characters, and should always
**	    be truncated on simple byte boundaries.  (No double-byte character
**	    boundaries to watch for.)
**      26-Oct-03 (zhahu02)
**      Updated to avoid SIGSEGV in iiinterp (b111188/INGCBT502) .
**	22-mar-2001 (abbjo03)
**	    Add support for NCHAR and NVARCHAR.
**	21-oct-2003 (gupsh01)
**	    Fix truncation of input data into result data for nchar and nvarchar 
**	    datatypes, when wchar_t is four bytes.
**	    
*/

static DB_STATUS
adh_chrcvt(adf_scb, adh_dv, adh_ev)
ADF_CB			*adf_scb;
DB_DATA_VALUE		*adh_dv;
DB_EMBEDDED_DATA	*adh_ev;
{
    DB_STATUS		db_stat = E_DB_OK;
    DB_DT_ID		dvtype;
    i4			dvlen;
    i4			evlen;
    i4			cplen;
    char		*frmptr, *toptr;

    /*
    ** Verify that coercion is allowed, and get pointer to and length of 
    ** character data to be converted.
    */
    dvtype = abs(adh_dv->db_datatype);
    switch (dvtype)
    {
      case DB_VCH_TYPE:
      case DB_TXT_TYPE:
      case DB_LTXT_TYPE:
      case DB_VBYTE_TYPE:
        if (adh_dv->db_length == 0)
        {
        dvlen = 0;
        break;
        }
	dvlen = ((DB_TEXT_STRING *)adh_dv->db_data)->db_t_count;
	frmptr = (char *)((DB_TEXT_STRING *)adh_dv->db_data)->db_t_text;
	break;
      
      case DB_NVCHR_TYPE:
	dvlen = ((DB_NVCHR_STRING *)adh_dv->db_data)->count * sizeof(wchar_t);
	frmptr = (char *)adh_dv->db_data + DB_CNTSIZE;
	break;
      
      case DB_NCHR_TYPE:
	dvlen = adh_dv->db_length / sizeof(UCS2) * sizeof(wchar_t);
	frmptr = (char *)adh_dv->db_data;
	break;

      case DB_CHR_TYPE:
      case DB_CHA_TYPE:
      case DB_BYTE_TYPE:
	dvlen = adh_dv->db_length;
	/*
	** If the source type is nullable, then make sure we do not copy
	** any null byte to the embedded destination area.
	*/
	if (ADH_NULLABLE_MACRO(adh_dv))
	    dvlen--;
	frmptr = (char *)adh_dv->db_data;
	break;

      default:
	return (adh_error(adf_scb, E_AD2009_NOCOERCION ));
    }

    if (adh_ev->ed_length <= 0)
    {
	evlen = dvlen;
    }
    else
    {
	if (adh_ev->ed_type == DB_VCH_TYPE || adh_ev->ed_type == DB_VBYTE_TYPE)
	    evlen = adh_ev->ed_length - DB_CNTSIZE;
	else if (adh_ev->ed_type == DB_NVCHR_TYPE)
	    evlen = adh_ev->ed_length - DB_CNTSIZE;
	else /* CHR or CHA or BYTE */
	    evlen = adh_ev->ed_length;
    }
    if (adh_ev->ed_type == DB_VCH_TYPE || adh_ev->ed_type == DB_VBYTE_TYPE)
	toptr = (char *)((DB_TEXT_STRING *)adh_ev->ed_data)->db_t_text;
    else if (adh_ev->ed_type == DB_NVCHR_TYPE)
	toptr = (char *)((AFE_NTEXT_STRING *)adh_ev->ed_data)->afe_t_text;
    else /* CHR or CHA or NCHR or BYTE */
  	toptr = (char *)adh_ev->ed_data; 

    if (dvlen <= evlen)			/* The whole thing fits */
    {
	if ((dvtype == DB_NCHR_TYPE || dvtype == DB_NVCHR_TYPE) &&
	    sizeof(wchar_t) != 2)
	{
	    i4		i;
	    u_i2	*pi = (u_i2 *)frmptr;
	    u_i4	*po = (u_i4 *)toptr;

	    for (i = 0; i < dvlen / 4; ++i, ++pi, ++po)
		*po = (u_i4)*pi;
	}
        else
	{
	    MEcopy(frmptr, (u_i2)dvlen, toptr);
	}
	cplen = dvlen;
    }
    else				/* Must truncate */
    {
	if (dvtype == DB_BYTE_TYPE || dvtype == DB_VBYTE_TYPE)
	{
	    /*
	    ** Truncate on simple byte boundary for byte data.
	    */
	    cplen = evlen;
	    MEcopy(frmptr, (u_i2)evlen, toptr);
	}
	if ((dvtype == DB_NCHR_TYPE || dvtype == DB_NVCHR_TYPE) && 
	    sizeof(wchar_t) == 4)
        {
	    i4          i;
	    u_i2        *pi = (u_i2 *)frmptr;
	    u_i4        *po = (u_i4 *)toptr;

	    for (i = 0; i < evlen / 4; ++i, ++pi, ++po)
	    *po = (u_i4)*pi;
	    cplen = evlen;
	} 
	else
	{
	    /*
	    ** Truncate on character boundary (possibly double-byte)
	    ** for character data.
	    */
	    cplen = CMcopy(frmptr, (u_i2)evlen, toptr);
	}
	/*
	** Truncated character strings don't want overhead of adh_error.
	** If indicator - set to original length.
	*/
	adf_scb->adf_errcb.ad_errcode = E_AD0101_EMBEDDED_CHAR_TRUNC;
	if (adh_ev->ed_null)
	    *adh_ev->ed_null = (i2)dvlen;
    }
    if (adh_ev->ed_type == DB_CHR_TYPE || adh_ev->ed_type == DB_CHA_TYPE 
	|| adh_ev->ed_type == DB_BYTE_TYPE || adh_ev->ed_type == DB_NCHR_TYPE)
	adh_charend(adh_ev->ed_type, toptr, cplen, evlen);
    else if (adh_ev->ed_type == DB_VCH_TYPE || adh_ev->ed_type == DB_VBYTE_TYPE)
	((DB_TEXT_STRING *)adh_ev->ed_data)->db_t_count = cplen;
    else if (adh_ev->ed_type == DB_NVCHR_TYPE)
	((AFE_NTEXT_STRING *)adh_ev->ed_data)->afe_t_count = cplen /
	    sizeof(wchar_t);

    return db_stat;
}
/* {
** Name: adh_losegcvt - Convert a Large Object Segment into DB_DATA_VALUE.
**
** Description:
**	This routine is called after a segment of a large object is retrieved
**	from the database. To avoid unnecessary copying of large objects the
**	segments are retrieved directly into the callers buffer by setting up
**	a DB_DATA_VALUE with the db_data field pointing directly into the
**	callers variable.  If the datatype of the variable is a varying type
**	(DB_VCH_TYPE etc..) then the data is placed in the data area (db_data
**	+ DB_CNT_SIZE).  
**	Given a DB_DATA_VALUE where db_data points to the callers destination 
**	variable and contains the data of a large object segment, this routine 
**	converts the segment into an internal DB_DATA_VALUE or host language 
**	variable as follows: 
**	-) If hosttype is DB_DBV_TYPE then unset the NULL indicator bit.
**	   For hosttype of DB_DBV_TYPE - dbv will contain the underlying 
**	   DB_DATA_VALUE.
**	-) If the callers variable is of type varying (DB_VCH_TYPE, etc..)
**	   Set the count field.
**	-) If the users variable is a non-varying char type then blank-pad or 
**	   EOS terminate according to datatype.
**
**    Assumptions:
**    1) The db_data field of the DB_DATA_VALUE points directly into the
**	 the users destination buffer and contains the segment data.
**    2) This routine will not be called for a null data value.
**
** Input:
**     hosttype: 	DB_DBV_TYPE or segment data type 
**			  if segment datatype then same as dbv->db_datatype.
**     hostlen:		Size of segment buffer. 
**     dbv:		DBV of large object segment. 
**     len:		Length of actual segment retrieved.
**     flag:		ADH_STD_SQL_MTHD for standard sql method, otherwise
**			we were called from a datahandler.
**
** Output:
**     None.
**
** History:
**      20-apr-1993 (kathryn) Written.
**	17-aug-1993 (kathryn)
**	    Only call adh_charend when data length is less than var length.
**	01-oct-1993 (kathryn)
**	    The len variable does not include DB_CNTSIZE for varying char
**	    types.
**	28-feb-1994 (mgw) Bug #56880
**	    use flag to determine whether we're calling from datahandler
**	    or not and thus whether to null terminate on boundry condition
**	    of len == outlen or not.
**	26-sep-2001 (gupsh01) 
**	    Convert the length to number of bytes before calling adh_charend 
**	    for null terminating for DB_NCHR_TYPES. Bug #105895
*/

void
adh_losegcvt(hosttype, hostlen, dbv, len, flag)
DB_DT_ID	hosttype;
i4		hostlen;
DB_DATA_VALUE   *dbv;
i4		len;	
i4		flag;
{
	i4 outlen;

	if (dbv->db_data == (PTR)0)
		return;

	outlen = hostlen;

	if (hosttype == DB_DBV_TYPE) 
	{
	    outlen = dbv->db_length;
	    if (ADH_NULLABLE_MACRO(dbv))
	    {
		ADH_UNSETNULL_MACRO((DB_DATA_VALUE *)dbv);
		outlen--;
	    }
	}
		
	if (IIDB_VARTYPE_MACRO(dbv))
	{
	    if (len <= MAXI2)
		((DB_TEXT_STRING *)dbv->db_data)->db_t_count = len;
	    else
		((DB_TEXT_STRING *)dbv->db_data)->db_t_count = -1;
	}
	else if ( IIDB_CHARTYPE_MACRO(dbv) && (!IIDB_LONGTYPE_MACRO(dbv))
		 && ((len < outlen) ||
			((len == outlen) && (flag & ADH_STD_SQL_MTHD)))
		)
	{
	    if (hosttype == DB_DBV_TYPE) 
		adh_charend(DB_CHA_TYPE, dbv->db_data, len, outlen);
	    else
		{	
		    /* 
			** for long nvarchar type we must pass in actually the 
			** number of bytes. The length we have here is actually
			** # of wchar_t's for long nvarchars.
			*/
			if (dbv->db_datatype == DB_NCHR_TYPE)
				len = len*sizeof(wchar_t);			

			adh_charend(dbv->db_datatype, dbv->db_data, len, outlen);
		}
	}
}

/*{
** Name:  	adh_charend - Correctly end character data into callers var.
**
** Description:
**	Given a character string buffer with data already retrieved from 
**	Ingres, end the character data according to datatype, size of the
**	buffer and internal flags. Either blank-pad, EOS terminate, 
**	or both(FIPS). Note that this routine does not handle setting the
**	length for varchar types, that should be done my the calling routine.
**
**
** Inputs:
** 	type		datatype of variable.
**	db_data		pointer to callers variable with data
**	inlen		length of data
**	outlen		length of variable
**
** Outputs:
**	db_data		may be blank-padded to outlen, may be EOS terminated.
**
** Returns:
**	void.
**
** History:
**	10-jun-1993 (kathryn)
**	    Written.
**	16-sep-1993 (kathryn)
**	    NULL pad fixed length byte strings.
**	16-mar-2001 (abbjo03)
**	    Add support for NCHAR types.
**	21-jun-2001 (gupsh01)
**	    Corrected the string termination for DB_NCHR_TYPE.
**	26-sep-2001 (gupsh01)
**	    Remove the previous fix for null terminating the
**	    DB_NCHR_TYPE, this was earlier done to fix null
**	    termination of long nvarchar types. For long nvarchar
**	    the length passed was no of wchars, fixed the calling
**	    routine to send in no of bytes insted. [ bug 105895 ]
*/

void
adh_charend (DB_DT_ID type, char *db_data, i4 inlen, i4 outlen)
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    i4		len = inlen;

    if (outlen > inlen)
    {
	if (type == DB_BYTE_TYPE)
		MEfill( outlen - inlen, 0, db_data + inlen);
	else if (type == DB_CHA_TYPE ||
		(type == DB_CHR_TYPE && (IIlbqcb->ii_lq_flags & II_L_CHRPAD)))
	{
	  	MEfill( outlen - inlen, ' ', db_data + inlen);
		len = outlen;
 	}
    }
    if (type == DB_CHR_TYPE)
	db_data[len] = EOS;
    else if (type == DB_NCHR_TYPE)
    {
	wchar_t	*p = (wchar_t *)db_data;
	p[len/sizeof(wchar_t)] = (wchar_t)0;
    }
}

/*{
** Name:  	adh_chkeos - Check that this CHR variable has a valid EOS. 
**
** Description:
**	Look through a character variable for a valid end of string. If one
**	is not found, return an error. This functionality is required 
**	for FIPS; we must be able to detect a non-null terminated C
**	string. This routine will only be called for ESQL/C and only
**	when the "-check_eos" preprocessor flag was used at preprocess
**	time.
**
**
** Inputs:
**	addr		pointer to CHR variable
**	len		pointer to length of CHR variable
**
** Outputs:
**	None
**
** Returns:
**	STATUS	
**	  	OK	Found an EOS
**		FAIL	No EOS found, give an error.
**
** History:
**	10-jan-1994 (teresal)
**	    Written.
*/

STATUS
adh_chkeos (char *addr, i4 len)
{
    /*
    ** A negative length indicates the size of the CHAR buffer declared -
    ** not the actual string length. If the length is positive or zero,
    ** we don't need to check for EOS because we either have a character
    ** pointer, a #DEFINE string, or a literal string. If length is negative,
    ** the calling program should set the length back to zero so we will get 
    ** the actual string length later on.
    */
    if (len < 0)	
    {
    	len = -len;
	if (STnlength(len, addr) == len)
	    return FAIL;
    }		
    return OK;
}


/*
** Name: adh_error
**
** Description:
**
** Input:
**
** Output:
**
** Returns:
**
** History:
*/

static DB_STATUS 
adh_error( ADF_CB *adf_scb, ER_MSGID err )
{
    DB_STATUS status;

    MUp_semaphore( &IIglbcb->ii_gl_sem );
    status = afe_error( adf_scb, err, 0 );
    MUv_semaphore( &IIglbcb->ii_gl_sem );

    return( status );
}

/* 
** Name: adh_ucolinit - Initializes the collation table for adf_cb in
**			II_LBQ_CB for a session. 
**
** Description:
**
**	This function initializes the unicode collation table for the 
**	ADF control block in II_LBQ_CB. This should only be called if
**	unicode coercion is need in the front end.
**
** Input:
**
**	II_LBQ_CB	Libq control block which holds the initiated 
**			values
** Output:
**	IIlbqcb->ii_lq_ucode_ctbl	The fixed sized unicode coercion table 
**					is initiated.
**      IIlbqcb->ii_lq_ucode_cvtbl	The variable sized vtable 
**					is initiated.
** Returns:
**
**      FAIL 			if aduucolinit fails.
**      OK				if successful.
**
** History:
**	26-oct-2004 (gupsh01)
**	    Added.
**	15-feb-2005 (gupsh01)
**	    Initiate unicode mapping table here.
**	17-jun-2005 (gupsh01)
**	    expose this function to be used outside adhembed.c.
**	23-jun-2005 (gupsh01)
**	    Fix the interface definition for this function.
**	26-May-2009 (kschendel) b122041
**	    Change interface definition again.
**      12-Jul-2009 (hanal04) Bug 122103
**          If we're not nfc we must be nfd.
*/
STATUS 
adh_ucolinit(IIlbqcb, setnfc)
II_LBQ_CB *IIlbqcb;
i4	setnfc;
{
    char                col[] = "udefault";
    ADF_CB              *adf_cb;
    STATUS		status = OK;
    CL_ERR_DESC         sys_err;

    if (!(IIlbqcb->ii_lq_ucode_init) )
    { 
        if (status = aduucolinit(col,
                  MEreqmem, (ADUUCETAB **)&IIlbqcb->ii_lq_ucode_ctbl,
                  &IIlbqcb->ii_lq_ucode_cvtbl, &sys_err))
        {
          status = FAIL;
        }

	if (status = adg_init_unimap() != E_DB_OK)
          status = FAIL;

        IIlbqcb->ii_lq_ucode_init = 1;
        adf_cb =  (ADF_CB *) IIlbqcb->ii_lq_adf;
        adf_cb->adf_ucollation = IIlbqcb->ii_lq_ucode_ctbl;  

	if (setnfc && (status = adg_setnfc(adf_cb) != E_DB_OK))
      status = FAIL;

        if (!setnfc)
            adf_cb->adf_uninorm_flag = AD_UNINORM_NFD;
    }

    return status;
}

/* 
** Name: adh_dounicoerce- Finds out if the unicode coercion is needed
**
** Description:
**
**	This function identifies if the conversion operation between a 
**	unicode and non unicode datatype is required in front end. 
**	The coercion is done for cases when a unicode value is selected
**	from the database and the result is placed in a character variable.
**	Or a varchar value is placed in a wchar_t variable. 
**
**
** Input:
**	adh_dv			Pointer to a DB_DATA_VALUE structure
**				holding the data value to be converted.
**		.db_datatype	The ADF type id (a character type).
**		.db_length	The length of the type.
**		.db_data	The internal value to convert.
**	adh_ev			Must point to a DB_EMBEDDED_DATA.
**		.ed_type	The embedded data type (CHR or VCH).
**		.ed_length	Its length.
**		.ed_data	Pointer to the embedded variable.
**
** Output:
**
**		1     		Unicode to non-unicode coercion is required.
**		-1    		Non-unicode to unicode coercion is required.
**		0    	 	Unicode coercion is not required.
**
** Returns:
**
** History:
**	26-oct-2004 (gupsh01)
**	    Added.
**	29-oct-2004 (gupsh01)
**	    adh_ev is actually of type DB_EMBEDDED_DATA not 
**	    DB_DATA_VALUE.
**      15-Jul-2010 (hanal04) Bug 124087
**          We need to know whether there is no unicode coercion,
**          char to uni, or uni to char coercion required. Return
**          an i4 so we can return one of three states.
*/
i4
adh_dounicoerce (adh_dv, adh_ev)
DB_DATA_VALUE           *adh_dv;
DB_EMBEDDED_DATA	*adh_ev;
{
    bool 	is_uni_dbv = FALSE;
    bool 	is_uni_edv = FALSE;

      if ((abs(adh_dv->db_datatype) == DB_NCHR_TYPE) ||  
	  (abs(adh_dv->db_datatype) == DB_NVCHR_TYPE) || 
	  (abs(adh_dv->db_datatype) == DB_LNVCHR_TYPE)) 
        is_uni_dbv = TRUE;

      if ((abs(adh_ev->ed_type) == DB_NCHR_TYPE) ||  
	  (abs(adh_ev->ed_type) == DB_NVCHR_TYPE) ||
	  (abs(adh_ev->ed_type) == DB_LNVCHR_TYPE)) 
        is_uni_edv = TRUE;

      if (is_uni_dbv > is_uni_edv)
	return 1;
      else if (is_uni_dbv < is_uni_edv)
	return -1;
      else
        return 0;
}
