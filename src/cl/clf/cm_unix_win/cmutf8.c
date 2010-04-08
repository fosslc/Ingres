/*
** Copyright (c) 2007, 2009 Ingres Corporation
*/

#include    <compat.h>
#include    <clconfig.h>
#include    <gl.h>
#include    <cs.h>
#include    <cm.h>
#include    <nm.h>
#include    <st.h>
#include    <lo.h>
#include    <me.h>
#include    <meprivate.h>
#include    <mu.h>
#include    <errno.h>

#ifdef	UNIX
#include    <ernames.h>
#endif

#ifdef	VMS
#include    <si.h>
#include    <cs.h>
#include    <lnmdef.h>
#include    <iodef.h>
#include    <fab.h>
#include    <rab.h>
#include    <ssdef.h>
#endif

#ifdef DESKTOP
#include    <er.h>
#endif /* DESKTOP */

/*
**  External variables
*/
GLOBALREF CM_UTF8ATTR *CM_UTF8AttrTab; 
GLOBALREF CM_UTF8CASE *CM_UTF8CaseTab;

/**
**
**  Name: CMUTF8.C - Routines to manipulate unicode strings encoded in UTF8
**	             format. These routines are equivalent to other CM 
**		     routines.
**
**  Description:
**      This file contains Routines to manipulate unicode strings encoded 
**	in UTF8 format. These routines are equivalent to other similar CM 
**	routines for multibyte characters and will be called from the macro for
**	other CM routines. However they will they take into account 
**	that the UTF8 strings can be precomposed to form unicode characters.
**
**  History:
**      05-Apr-2007 (gupsh01)
**	    Created.
**      19-Aug-2008 (whiro01)
**	    Made the CMmbstowcs and CMwcstombs functions real (taken from cmcl.h)
** 	30-Oct-2008 (gupsh01)
**	    Added remaining conversion functions for converting between 
**	    various UTF encodings, adopted from Unicode.org.
**	21-Nov-2008 (gupsh01)
**	    General cleanup.
**      05-feb-2009 (joea)
**          Rename CM_ischarsetUTF8 to CMischarset_utf8.
**	13-feb-2009 (joea)
**	    Remove left over fflush in CM_UTF8toUTF32.
**	20-feb-2009 (joea)
**	    Remove additional left over fflush calls. Replace fprintf by
**	    SIfprintf.
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

/* Name: cmupct - calculates the position count of the
**		  previous start character in a UTF8 
**		  stream.
** History:
**
**	05-may-2007 (gupsh01)
**	    Added History section.
*/
i4
cmupct (u_char *str, u_char *startpos)
{
  unsigned char	*p = str;
  int		rescnt = 1;
  if (str && startpos)
  {
    rescnt = 0;
    while (p >= startpos)
    {
        p--;
        rescnt++;
        if (((*p & 0xFF) & 0x80) == 0)
          break;
        else if (((*p & 0xFF) & 0xC0) != 0)
          break;
    }
  }
  return rescnt;
}

/* Name: cmu_getutf8property - Gets the property
**			   for UTF8 charactersets
** Description:
**
** Input
**	key	UTF8 character bytes.
**	keycnt	count of input characters.
**
** Output
**	property if ok else returns 0.
**
** History:
**	20-may-2007 (gupsh01)
**	    Created.
*/
u_i2
cmu_getutf8property(u_char *key, i4 keycnt)
{
    u_i2 index;
    if (CM_UTF8toUTF32(key, keycnt, &index) != OK)
      return (0);
    else
      return (CM_UTF8AttrTab[index].property);
}

/* Name: cmu_getutf8casecnt - Gets the casecount 
**			   for UTF8 charactersets
** Description:
** Input
** Output
**
** History:
**	20-may-2007 (gupsh01)
**	    Created.
*/
u_i2
cmu_getutf8casecnt(u_char *key, i4 keycnt)
{
    u_i2 index;
    u_i2 caseindex;
    u_i2 casecnt = 0;

    if (CM_UTF8toUTF32(key, keycnt, &index) != OK)
      return (0); 

    caseindex = CM_UTF8AttrTab[index].caseindex;
    if (caseindex != 0)
    {
	casecnt = CM_UTF8CaseTab[caseindex].casecnt;
    }
    return (casecnt);
}

/* Name: cmu_utf8_tolower - Converts a UTF8 string to lowercase
**	 This routine expects a dstlen and srclen.
**	 it will check the dstlen if it is nonzero
**
** Input 
**
** Output
**
**
** History:
**	20-may-2007 (gupsh01)
**	    Created.
**      25-Nov-2009 (hanal04) Bug 122311
**          Ingres Janitor's project. Fix compiler warnings by making
**          local casechars declaration consistent with the data type
**          of CM_UTF8CaseTab[caseindex].casechars
*/
u_i2
cmu_getutf8_tolower(u_char *src, i4 srclen, u_char *dst, i4 dstlen)
{
  u_i2		index = 0;
  u_i2		caseindex = 0;
  u_char	temp[8] = {0};
  bool		tempused = FALSE;
  u_char	*pdst;
  u_i2		plen = 0;

  /* Check for inplace replace */
  if (src == dst)
  {
    plen = srclen;
    pdst = temp;
    tempused = TRUE;
  }
  else 
    pdst = dst;

  if (srclen != 1)
  {
    /* We don't fail right now and just copy the source
    ** to destination.
    */
    if (CM_UTF8toUTF32(src, srclen, &index) != OK)
    {
      if (!(tempused))
      {
        MEcopy (src, srclen, dst);	/* copy the source to dest */
      }
      return (OK); 
    }
  }
  else 
    index = *src;

  caseindex = CM_UTF8AttrTab[index].caseindex;
  if (caseindex != 0)
  {
    u_i2	property;
    u_char	*casechars;
    i4		casecnt;

    property = CM_UTF8AttrTab[index].property;
    if (property & CM_A_UPPER)
    {
      casecnt = CM_UTF8CaseTab[caseindex].casecnt;
      if (dstlen && (dstlen < casecnt))
        return (FAIL);
      casechars = CM_UTF8CaseTab[caseindex].casechars;
      MEcopy (casechars, casecnt, pdst);
      plen = casecnt;
    }
    else 
      MEcopy (src, srclen, pdst);
  }
  else 
    MEcopy (src, srclen, pdst);

  if (tempused)
    MEcopy (pdst, plen, dst);

  return (OK);
}

/* Name: cmu_utf8_toupper - Converts a UTF8 string to lowercase
**	 This routine expects a dstlen and srclen.
**	 it will check the dstlen if it is nonzero
**
**	 This routine does not fail right nor for illegal input
**	 but returns.
**
** Input 
**
** Output
**
** History:
**	20-may-2007 (gupsh01)
**	    Created.
**      25-Nov-2009 (hanal04) Bug 122311
**          Ingres Janitor's project. Fix compiler warnings by making
**          local casechars declaration consistent with the data type
**          of CM_UTF8CaseTab[caseindex].casechars
*/
u_i2
cmu_getutf8_toupper(u_char *src, i4 srclen, u_char *dst, i4 dstlen)
{
  u_i2		index = 0;
  u_i2		caseindex = 0;
  u_char	temp[8] = {0};
  bool		tempused = FALSE;
  u_char	*pdst;
  u_i2		plen = 0;

  /* Check for inplace replace */
  if (src == dst)
  {
    plen = srclen;
    pdst = temp;
    tempused = TRUE;
  }
  else 
    pdst = dst;

  if (srclen != 1)
  {
    if (CM_UTF8toUTF32(src, srclen, &index) != OK)
    {
    /* We don't fail right now and just copy the source
    ** to destination.
    */
      if (!(tempused))
      {
        MEcopy (src, srclen, dst);	/* copy the source to dest */
      }
      return (OK); 
    }
  } 
  else 
    index = *src;

  caseindex = CM_UTF8AttrTab[index].caseindex;
  if (caseindex != 0)
  {
    u_i2	property;
    u_char	*casechars;
    i4		casecnt;

    property = CM_UTF8AttrTab[index].property;
    if (property & CM_A_LOWER)
    {
      casecnt = CM_UTF8CaseTab[caseindex].casecnt;
      if (dstlen && (dstlen < casecnt))
        return (FAIL);
      casechars = CM_UTF8CaseTab[caseindex].casechars;
      MEcopy (casechars, casecnt, pdst);
      plen = casecnt;
    }
    else 
      MEcopy (src, srclen, pdst);
  }
  else 
    MEcopy (src, srclen, pdst);

  if (tempused)
    MEcopy (pdst, plen, dst);

  return (OK);
}

/*
** Name: CMwcstombs
**
** Description:
**      Converts wide character string to multibyte.
**
** Input:
**      wcstr     pointer to wide character string
**      mbstr     pointer to multibyte character string
**      mbsize    maximum size of multibyte character string
**
** Output:
**      size_t     number of bytes written if successful
**
** History:
**      06-feb-2004 (drivi01)
**          Created.
**      19-aug-2008 (whiro01)
**          Made into real function for NT_GENERIC.
*/
#if defined(NT_GENERIC)
size_t CMwcstombs(const wchar_t *wcstr, char *mbstr, size_t mbsize)
{
    if (CMischarset_utf8())
	return WideCharToMultiByte(CP_UTF8, 0, wcstr, -1, mbstr, mbsize, NULL, NULL);
    else
	return WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, wcstr, -1, mbstr, mbsize, NULL, NULL);
}
#endif 

/*
** Name: CMmbstowcs
**
** Description:
**      Converts multibyte character string to wide character string
**
** Input:
**      mbstr     pointer to multibyte character string
**      wcstr     pointer to wide character string
**      wcsize    maximum size of wide character string (chars)
**
** Output:
**      size_t     number of bytes written if successful
**
** History:
**      06-feb-2004 (drivi01)
**          Created.
**      19-aug-2008 (whiro01)
**          Made into real function for NT_GENERIC.
*/
#if defined(NT_GENERIC) 
size_t CMmbstowcs(const char *mbstr, wchar_t *wcstr, size_t wcsize)
{
    if (CMischarset_utf8())
	return MultiByteToWideChar(CP_UTF8, 0, mbstr, -1, wcstr, wcsize);
    else
	return MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, mbstr, -1, wcstr, wcsize);
}
#endif 

/* The routines below are used for conversion between UTF8 and UTF32 
** adopted from Unicode.org, and modified for our requirements. 
*
** Copyright 2001-2004 Unicode, Inc.
**
** Disclaimer
**
** This source code is provided as is by Unicode, Inc. No claims are
** made as to fitness for any particular purpose. No warranties of any
** kind are expressed or implied. The recipient agrees to determine
** applicability of information provided. If this file has been
** purchased on magnetic or optical media from Unicode, Inc., the
** sole remedy for any claim will be exchange of defective media
** within 90 days of receipt.
**
** Limitations on Rights to Redistribute This Code
**
** Unicode, Inc. hereby grants the right to freely use the information
** supplied in this file in the creation of products supporting the
** Unicode Standard, and to make copies of this file in any form
** for internal or external distribution as long as this notice
** remains attached.
*/

typedef unsigned long   UTF32;  /* at least 32 bits */
typedef unsigned short  UTF16;  /* at least 16 bits */
typedef unsigned char   UTF8;   /* typically 8 bits */

#define UNI_REPLACEMENT_CHAR (UTF32)0x0000FFFD
#define UNI_MAX_LEGAL_UTF32 (UTF32)0x0010FFFF
#define UNI_SUR_HIGH_START  (UTF32)0xD800
#define UNI_SUR_HIGH_END    (UTF32)0xDBFF
#define UNI_SUR_LOW_START   (UTF32)0xDC00
#define UNI_SUR_LOW_END     (UTF32)0xDFFF
/* Some fundamental constants */
#define UNI_MAX_BMP (UTF32)0x0000FFFF
#define UNI_MAX_UTF16 (UTF32)0x0010FFFF
#define UNI_MAX_UTF32 (UTF32)0x7FFFFFFF

static const i4 halfShift  = 10; /* used for shifting by 10 bits */

static const UTF32 halfBase = 0x0010000UL;
static const UTF32 halfMask = 0x3FFUL;


typedef enum {
        conversionOK = 0,       /* conversion successful */
        sourceExhausted,        /* partial character in source, but hit end */
        targetExhausted,        /* insuff. room in target for conversion */
        sourceIllegal           /* source sequence is illegal/malformed */
} ConversionResult;

/*
 * Once the bits are split out into bytes of UTF-8, this is a mask OR-ed
 * into the first byte, depending on how many bytes follow.  There are
 * as many entries in this table as there are UTF-8 sequence types.
 * (I.e., one byte sequence, two byte... etc.). Remember that sequencs
 * for *legal* UTF-8 will be 4 or fewer bytes total.
 */
static const UTF8 firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

static const char trailingBytesForUTF8[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

/*
 * Magic values subtracted from a buffer value during UTF8 conversion.
 * This table contains as many values as there might be trailing bytes
 * in a UTF-8 sequence.
 */
static const UTF32 offsetsFromUTF8[6] = { 0x00000000UL, 0x00003080UL, 0x000E2080UL,
                     0x03C82080UL, 0xFA082080UL, 0x82082080UL };

/*
 * Utility routine to tell whether a sequence of bytes is legal UTF-8.
 * This must be called with the length pre-determined by the first byte.
 * If not calling this from ConvertUTF8to*, then the length can be set by:
 *  length = trailingBytesForUTF8[*source]+1;
 * and the sequence is illegal right away if there aren't that many bytes
 * available.
 * If presented with a length > 4, this returns FALSE.  The Unicode
 * definition of UTF-8 goes up to 4-byte sequences.
 */

static bool isLegalUTF8(const UTF8 *source, int length) {
    UTF8 a;
    const UTF8 *srcptr = source+length;
    switch (length) {
    default: return FALSE;
        /* Everything else falls through when "true"... */
    case 4: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return FALSE;
    case 3: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return FALSE;
    case 2: if ((a = (*--srcptr)) > 0xBF) return FALSE;

        switch (*source) {
            /* no fall-through in this inner switch */
            case 0xE0: if (a < 0xA0) return FALSE; break;
            case 0xED: if (a > 0x9F) return FALSE; break;
            case 0xF0: if (a < 0x90) return FALSE; break;
            case 0xF4: if (a > 0x8F) return FALSE; break;
            default:   if (a < 0x80) return FALSE;
        }

    case 1: if (*source >= 0x80 && *source < 0xC2) return FALSE;
    }
    if (*source > 0xF4) return FALSE;
    return TRUE;
}

ConversionResult
CM_ConvertUTF8toUTF32(
	const UTF8** sourceStart, const UTF8* sourceEnd, 
	UTF32** targetStart, UTF32* targetEnd, i4 flags) 
{
    ConversionResult result = conversionOK;
    const UTF8* source = *sourceStart;
    UTF32* target = *targetStart;
    while (source < sourceEnd) {
	UTF32 ch = 0;
	unsigned short extraBytesToRead = trailingBytesForUTF8[*source]; 
	if (source + extraBytesToRead >= sourceEnd) {
	    result = sourceExhausted; break;
	}
	/* Do this check whether lenient or strict */
	if (! isLegalUTF8(source, extraBytesToRead+1)) {
	    result = sourceIllegal;
	    break;
	}
	/*
	 * The cases all fall through. See "Note A" below.
	 */
	switch (extraBytesToRead) {
	    case 5: ch += *source++; ch <<= 6;
	    case 4: ch += *source++; ch <<= 6;
	    case 3: ch += *source++; ch <<= 6;
	    case 2: ch += *source++; ch <<= 6;
	    case 1: ch += *source++; ch <<= 6;
	    case 0: ch += *source++;
	}
	ch -= offsetsFromUTF8[extraBytesToRead];

	if (target >= targetEnd) {
	    source -= (extraBytesToRead+1); /* Back up the source pointer! */
	    result = targetExhausted; break;
	}
	if (ch <= UNI_MAX_LEGAL_UTF32) {
	    /*
	     * UTF-16 surrogate values are illegal in UTF-32, and anything
	     * over Plane 17 (> 0x10FFFF) is illegal.
	     */
	    if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_LOW_END) {
		if (flags == 1) {
		    source -= (extraBytesToRead+1); /* return to the illegal value itself */
		    result = sourceIllegal;
		    break;
		} else {
		    *target++ = UNI_REPLACEMENT_CHAR;
		}
	    } else {
		*target++ = ch;
	    }
	} else { /* i.e., ch > UNI_MAX_LEGAL_UTF32 */
	    result = sourceIllegal;
	    *target++ = UNI_REPLACEMENT_CHAR;
	}
    }
    return result;
}

/* Name: CM_UTF8toUTF32 - Converts a UTF8 string to UTF32 for 
** lookup in the properties table.
** Input 
** 	instr - input UTF8 string.
**	incnt - byte length of the input string. 
** 	outval - output UTF32 value
** Output
**	Return OK   - If success.
**	       FAIL - if conversion failed.
** History
**	24-may-2007 (gupsh01)
**	    Added
**	14-july-2008 (gupsh01)
**	    Fix compiler warning due to mismatched variable types.
**      25-Nov-2009 (hanal04) Bug 122311
**          Ingres Janitor's project. Fix compiler warnings by making
**          instr parameter type consistent with the type passed by all of
**          its callers.
*/
i4
CM_UTF8toUTF32 (u_char *instr, i4 incnt, u_i2 *outval)
{
        i4 	i, n;
        ConversionResult result;
        UTF32 	utf32_buf[2];
        UTF8 	utf8_buf[5];
	UTF8	*utf8SourceStart;
	UTF32	*utf32TargetStart;

        utf32_buf[0] = 0; utf32_buf[1] = 0;

        for (n = 0; n < incnt; n++) 
	  utf8_buf[n] = instr[n];

        utf8SourceStart = utf8_buf;
        utf32TargetStart = utf32_buf;

        result = CM_ConvertUTF8toUTF32((const UTF8 **) &utf8SourceStart,
                    &(utf8_buf[incnt]), &utf32TargetStart,
                    &(utf32_buf[1]), 0);
        if (result != conversionOK) 
	{
	  *outval = 0;
	  return (FAIL);
        }
        *outval = utf32_buf[0]; 
	return (OK);
}

ConversionResult
CM_ConvertUTF32toUTF8 (
        const UTF32** sourceStart, const UTF32* sourceEnd,
        UTF8** targetStart, UTF8* targetEnd, i2 flags, i2 *rescnt) 
{
    i4 result = conversionOK;
    const UTF32* source = *sourceStart;
    UTF8* target = *targetStart;
    while (source < sourceEnd) {
        UTF32 ch;
        unsigned short bytesToWrite = 0;
        const UTF32 byteMask = 0xBF;
        const UTF32 byteMark = 0x80;
        ch = *source++;
        if (flags == 1) {
            /* UTF-16 surrogate values are illegal in UTF-32 */
            if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_LOW_END) {
                --source; /* return to the illegal value itself */
                result = sourceIllegal;
                break;
            }
        }
        /*
         * Figure out how many bytes the result will require. Turn any
         * illegally large UTF32 things (> Plane 17) into replacement chars.
         */
        if (ch < (UTF32)0x80) {      bytesToWrite = 1;
        } else if (ch < (UTF32)0x800) {     bytesToWrite = 2;
        } else if (ch < (UTF32)0x10000) {   bytesToWrite = 3;
        } else if (ch <= UNI_MAX_LEGAL_UTF32) {  bytesToWrite = 4;
        } else {                            bytesToWrite = 3;
                                            ch = UNI_REPLACEMENT_CHAR;
                                            result = sourceIllegal;
        }

        target += bytesToWrite;
        if (target > targetEnd) {
            --source; /* Back up source pointer! */
            target -= bytesToWrite; result = targetExhausted; break;
        }
        switch (bytesToWrite) { /* note: everything falls through. */
            case 4: *--target = (UTF8)((ch | byteMark) & byteMask); ch >>= 6;
            case 3: *--target = (UTF8)((ch | byteMark) & byteMask); ch >>= 6;
            case 2: *--target = (UTF8)((ch | byteMark) & byteMask); ch >>= 6;
            case 1: *--target = (UTF8) (ch | firstByteMark[bytesToWrite]);
        }
        target += bytesToWrite;
	*rescnt = bytesToWrite;
    }
    return result;
}

i4
CM_UTF32toUTF8 (int inval, char *resstr)
{
        i4 i, n;
        ConversionResult result;
        UTF32 utf32_buf[2];
        UTF8 utf8_buf[15];
        UTF32 *utf32SourceStart;
        UTF8 *utf8TargetStart;
        i2 rescnt = 0;

        utf32_buf[0] = inval; utf32_buf[1] = 0;
        for (n = 0; n < 8; n++) utf8_buf[n] = 0;

        utf32SourceStart = utf32_buf;
        utf8TargetStart = utf8_buf;

        result = CM_ConvertUTF32toUTF8((const UTF32 **) &utf32SourceStart,
                                     &(utf32_buf[1]), &utf8TargetStart,
                                     &(utf8_buf[14]),
                                     0, &rescnt);
        if (result != conversionOK) {
        SIfprintf(stderr,
            "CM_ConvertUTF32toUTF8 : fatal error: result %d for input %08x\n", result, utf32_buf[0]); exit(1);
        }
        for (i=0; i < rescnt; i++)
        {
          resstr[i] = utf8_buf[i];
        }
        return rescnt;
}

typedef enum {
        strictConversion = 0,
        lenientConversion
} ConversionFlags;

ConversionResult 
CM_ConvertUTF32toUTF16 (
	const UTF32** sourceStart, 
	const UTF32* sourceEnd, 
	UTF16** targetStart, 
	UTF16* targetEnd, 
	ConversionFlags flags, 
	i4 *rescnt) 
{
    ConversionResult result = conversionOK;
    const UTF32* source = *sourceStart;
    UTF16* target = *targetStart;
    i4 resultcnt = 0;
    *rescnt = resultcnt;
    while (source < sourceEnd) 
    {
	UTF32 ch;
	if (target >= targetEnd) 
	{
	    result = targetExhausted; 
	    break;
	}
	ch = *source++;
	if (ch <= UNI_MAX_BMP) 
	{ 
	  /* Target is a character <= 0xFFFF
	  ** UTF-16 surrogate values are illegal in UTF-32; 
	  ** 0xffff or 0xfffe are both reserved values 
	  */
	  if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_LOW_END) 
	  {
		if (flags == strictConversion) 
		{
		    --source; /* return to the illegal value itself */
		    result = sourceIllegal;
		    break;
		} 
		else 
		{
		    *target++ = UNI_REPLACEMENT_CHAR;
		    resultcnt++;
		}
	    } 
	    else 
	    {
		*target++ = (UTF16)ch; /* normal case */
		resultcnt++;
	    }
	} 
	else if (ch > UNI_MAX_LEGAL_UTF32) 
	{
	    if (flags == strictConversion) 
	    {
		result = sourceIllegal;
	    } 
	    else 
	    {
		*target++ = UNI_REPLACEMENT_CHAR;
		resultcnt++;
	    }
	} 
	else 
	{
	    /* target is a character in range 0xFFFF - 0x10FFFF. */
	    if (target + 1 >= targetEnd) 
	    {
		--source; /* Back up source pointer! */
		result = targetExhausted; break;
	    }
	    ch -= halfBase;
	    *target++ = (UTF16)((ch >> halfShift) + UNI_SUR_HIGH_START);
	    *target++ = (UTF16)((ch & halfMask) + UNI_SUR_LOW_START);
	    resultcnt += 2;
	}
    }
    *sourceStart = source;
    *targetStart = target;
    *rescnt = resultcnt;
    return result;
}

/* CM_UTF32toUTF16 - This routine accepts a UTF32 value as an i4
**		     It converts the UTF32 code to the output buffer 
**		     It writes the number of UTF16 code units written 
**		     to ocount;
**	  Input
**		invalue  The UTF32 code point value between 0 to 0x10FFFF.
**		outstr	An array of outstr pointer at least 
**
**
**	   returns converstionOK if successful >0 otherwise. 
**
** History
**	3-nov-2008 (gupsh01)
**	    Added.
*/
i4
CM_UTF32toUTF16 (i4 inval, u_i2 *outbuf, u_i2 *outend, i4 *outlen)
{
	i4 i, rescnt;
        ConversionResult result = conversionOK;
        UTF32 utf32_buf[2];
	UTF16 utf16_buf[3];
        UTF32 *utf32SourceStart, *utf32TargetStart;
        UTF16 *utf16SourceStart, *utf16TargetStart;

        utf32_buf[0] = inval; utf32_buf[1] = 0;
	utf16_buf[0] = utf16_buf[1] = utf16_buf[2] = 0;

        utf32SourceStart = utf32_buf;         
	utf16TargetStart = utf16_buf;

        if (result = CM_ConvertUTF32toUTF16((const UTF32 **) &utf32SourceStart,
                                     &(utf32_buf[1]), &utf16TargetStart,
                                     &(utf16_buf[2]),
				     0, &rescnt) != conversionOK ) 
	{
#ifdef xDebug
          SIfprintf(stderr,
            "CM_ConvertUTF32toUTF16 : fatal error: result %d for input %08x\n", 
		result, utf32_buf[0]); 
#endif
	   return result;
        }
	else if ((outbuf + rescnt) > outend)
	{
#ifdef xDebug
          SIfprintf(stderr,
            "CM_ConvertUTF32toUTF16 : fatal error: result %d for input %08x\n", 
		result, utf32_buf[0]); 
#endif
	   return targetExhausted;
	}
	else
	{
	   *outlen = rescnt; 
	   *outbuf = utf16_buf[0];
	   if (rescnt == 2)
		*(outbuf + 1) = utf16_buf[1];
	   return conversionOK;
	}
}

ConversionResult 
CM_ConvertUTF16toUTF32 (
	const UTF16** sourceStart, const UTF16* sourceEnd, 
	UTF32** targetStart, UTF32* targetEnd, ConversionFlags flags) 
{
    ConversionResult result = conversionOK;
    const UTF16* source = *sourceStart;
    UTF32* target = *targetStart;
    UTF32 ch, ch2;
    while (source < sourceEnd) {
	const UTF16* oldSource = source; /*  In case we have to back up because of target overflow. */
	ch = *source++;
	/* If we have a surrogate pair, convert to UTF32 first. */
	if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_HIGH_END) {
	    /* If the 16 bits following the high surrogate are in the source buffer... */
	    if (source < sourceEnd) {
		ch2 = *source;
		/* If it's a low surrogate, convert to UTF32. */
		if (ch2 >= UNI_SUR_LOW_START && ch2 <= UNI_SUR_LOW_END) {
		    ch = ((ch - UNI_SUR_HIGH_START) << halfShift)
			+ (ch2 - UNI_SUR_LOW_START) + halfBase;
		    ++source;
		} else if (flags == strictConversion) { /* it's an unpaired high surrogate */
		    --source; /* return to the illegal value itself */
		    result = sourceIllegal;
		    break;
		}
	    } else { /* We don't have the 16 bits following the high surrogate. */
		--source; /* return to the high surrogate */
		result = sourceExhausted;
		break;
	    }
	} else if (flags == strictConversion) {
	    /* UTF-16 surrogate values are illegal in UTF-32 */
	    if (ch >= UNI_SUR_LOW_START && ch <= UNI_SUR_LOW_END) {
		--source; /* return to the illegal value itself */
		result = sourceIllegal;
		break;
	    }
	}
	if (target >= targetEnd) {
	    source = oldSource; /* Back up source pointer! */
	    result = targetExhausted; break;
	}
	*target++ = ch;
    }
    *sourceStart = source;
    *targetStart = target;
#ifdef CVTUTF_DEBUG
if (result == sourceIllegal) {
    SIfprintf(stderr, "ConvertUTF16toUTF32 illegal seq 0x%04x,%04x\n", ch, ch2);
    SIflush(stderr);
}
#endif
    return result;
}

ConversionResult 
CM_ConvertUTF16toUTF8 (
	const UTF16** sourceStart, const UTF16* sourceEnd, 
	UTF8** targetStart, UTF8* targetEnd, ConversionFlags flags) 
{
    ConversionResult result = conversionOK;
    const UTF16* source = *sourceStart;
    UTF8* target = *targetStart;
    while (source < sourceEnd) {
	UTF32 ch;
	unsigned short bytesToWrite = 0;
	const UTF32 byteMask = 0xBF;
	const UTF32 byteMark = 0x80; 
	const UTF16* oldSource = source; /* In case we have to back up because of target overflow. */
	ch = *source++;
	/* If we have a surrogate pair, convert to UTF32 first. */
	if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_HIGH_END) {
	    /* If the 16 bits following the high surrogate are in the source buffer... */
	    if (source < sourceEnd) {
		UTF32 ch2 = *source;
		/* If it's a low surrogate, convert to UTF32. */
		if (ch2 >= UNI_SUR_LOW_START && ch2 <= UNI_SUR_LOW_END) {
		    ch = ((ch - UNI_SUR_HIGH_START) << halfShift)
			+ (ch2 - UNI_SUR_LOW_START) + halfBase;
		    ++source;
		} else if (flags == strictConversion) { /* it's an unpaired high surrogate */
		    --source; /* return to the illegal value itself */
		    result = sourceIllegal;
		    break;
		}
	    } else { /* We don't have the 16 bits following the high surrogate. */
		--source; /* return to the high surrogate */
		result = sourceExhausted;
		break;
	    }
	} else if (flags == strictConversion) {
	    /* UTF-16 surrogate values are illegal in UTF-32 */
	    if (ch >= UNI_SUR_LOW_START && ch <= UNI_SUR_LOW_END) {
		--source; /* return to the illegal value itself */
		result = sourceIllegal;
		break;
	    }
	}
	/* Figure out how many bytes the result will require */
	if (ch < (UTF32)0x80) {	     bytesToWrite = 1;
	} else if (ch < (UTF32)0x800) {     bytesToWrite = 2;
	} else if (ch < (UTF32)0x10000) {   bytesToWrite = 3;
	} else if (ch < (UTF32)0x110000) {  bytesToWrite = 4;
	} else {			    bytesToWrite = 3;
					    ch = UNI_REPLACEMENT_CHAR;
	}

	target += bytesToWrite;
	if (target > targetEnd) {
	    source = oldSource; /* Back up source pointer! */
	    target -= bytesToWrite; result = targetExhausted; break;
	}
	switch (bytesToWrite) { /* note: everything falls through. */
	    case 4: *--target = (UTF8)((ch | byteMark) & byteMask); ch >>= 6;
	    case 3: *--target = (UTF8)((ch | byteMark) & byteMask); ch >>= 6;
	    case 2: *--target = (UTF8)((ch | byteMark) & byteMask); ch >>= 6;
	    case 1: *--target =  (UTF8)(ch | firstByteMark[bytesToWrite]);
	}
	target += bytesToWrite;
    }
    *sourceStart = source;
    *targetStart = target;
    return result;
}

bool
CM_isLegalUTF8Sequence(const UTF8 *source, const UTF8 *sourceEnd) 
{
    i4 length = trailingBytesForUTF8[*source]+1;
    if (source+length > sourceEnd) {
	return FALSE;
    }
    return isLegalUTF8(source, length);
}

ConversionResult 
CM_ConvertUTF8toUTF16 (
	const UTF8** sourceStart, const UTF8* sourceEnd, 
	UTF16** targetStart, UTF16* targetEnd, ConversionFlags flags) 
{
    ConversionResult result = conversionOK;
    const UTF8* source = *sourceStart;
    UTF16* target = *targetStart;
    while (source < sourceEnd) {
	UTF32 ch = 0;
	unsigned short extraBytesToRead = trailingBytesForUTF8[*source];
	if (source + extraBytesToRead >= sourceEnd) {
	    result = sourceExhausted; break;
	}
	/* Do this check whether lenient or strict */
	if (! isLegalUTF8(source, extraBytesToRead+1)) {
	    result = sourceIllegal;
	    break;
	}
	/*
	 * The cases all fall through. See "Note A" below.
	 */
	switch (extraBytesToRead) {
	    case 5: ch += *source++; ch <<= 6; /* remember, illegal UTF-8 */
	    case 4: ch += *source++; ch <<= 6; /* remember, illegal UTF-8 */
	    case 3: ch += *source++; ch <<= 6;
	    case 2: ch += *source++; ch <<= 6;
	    case 1: ch += *source++; ch <<= 6;
	    case 0: ch += *source++;
	}
	ch -= offsetsFromUTF8[extraBytesToRead];

	if (target >= targetEnd) {
	    source -= (extraBytesToRead+1); /* Back up source pointer! */
	    result = targetExhausted; break;
	}
	if (ch <= UNI_MAX_BMP) { /* Target is a character <= 0xFFFF */
	    /* UTF-16 surrogate values are illegal in UTF-32 */
	    if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_LOW_END) {
		if (flags == strictConversion) {
		    source -= (extraBytesToRead+1); /* return to the illegal value itself */
		    result = sourceIllegal;
		    break;
		} else {
		    *target++ = UNI_REPLACEMENT_CHAR;
		}
	    } else {
		*target++ = (UTF16)ch; /* normal case */
	    }
	} else if (ch > UNI_MAX_UTF16) {
	    if (flags == strictConversion) {
		result = sourceIllegal;
		source -= (extraBytesToRead+1); /* return to the start */
		break; /* Bail out; shouldn't continue */
	    } else {
		*target++ = UNI_REPLACEMENT_CHAR;
	    }
	} else {
	    /* target is a character in range 0xFFFF - 0x10FFFF. */
	    if (target + 1 >= targetEnd) {
		source -= (extraBytesToRead+1); /* Back up source pointer! */
		result = targetExhausted; break;
	    }
	    ch -= halfBase;
	    *target++ = (UTF16)((ch >> halfShift) + UNI_SUR_HIGH_START);
	    *target++ = (UTF16)((ch & halfMask) + UNI_SUR_LOW_START);
	}
    }
    *sourceStart = source;
    *targetStart = target;
    return result;
}

/*
** The following is an implementation of wcwidth, adopted from 
** http://www.cl.cam.ac.uk/~mgk25/ucs/wcwidth.c by Marcus Kuhn and 
** based on http://www.unicode.org/unicode/reports/tr11/.
** Author has graciously permitted free use of this code with the 
** following copy right notice found on his website: 
**   Permission to use, copy, modify, and distribute this software
**   for any purpose and without fee is hereby granted. The author
**   disclaims all warranties with regard to this software.
**
** wcwidth() and wcswidth() are defined in IEEE Std 1002.1-2001 for 
** Unicode. It may or may not however be implemented by the c compiler 
** eg on Windows and hence is being coded here to facilitate calcultion 
** of terminal width in the routine CM_UTF8_twidth(). 
**
** Currently this routine is useful for a on a UTF8 aware output 
** terminal where fixed width rendering of a UTF8 character is possible
**
** Description: Terminal width of a Unicode Character.
** ---------------------------------------------------
** In fixed-width output devices, Latin characters all occupy a single
** "cell" position of equal width, whereas ideographic CJK characters
** occupy two such cells.  
**
** Unicode standard defines character-cell width classes for 
** Unicode characters in technical report tr11. Characters are classified
** with having either East Asian FullWidth (F), Wide (W), Half-width (H), 
** or Narrow (Na) classes.
** For some characters it defines East Asian Ambiguous (A) class, where 
** the width choice depends purely on a preference of backward
** compatibility with either historic CJK or Western practice. Historically
** 16 bit characters (even for Greek, Cyrillic etc) were printed in double
** width where as 8-bit in single-width. 
** Another lass is Not East Asian (Neutral) class. Existing practice does 
** not dictate a width for any of these characters. It would however  make 
** sense typographically to allocate two character cells to characters 
** such as for instance EM SPACE or VOLUME INTEGRAL, which cannot be 
** represented adequately with a single-width glyph. 
**
** East Asian ambiguous class and Neutral class are both assigned a single
** cell width which may not be adequate representation of these characters.
** There is no adequate standard that defines the behavior of these 
** characters hence this routine is only approximate in calculating the
** display width for these characters. It needs to be improved further as
** these standards develop in the future. 
**/

/* Local structure only used in this file */
typedef struct _CM_WC_WIDTH_INTERVAL {
  i4 first;
  i4 last;
} CM_WC_WIDTH_INTERVAL;

/* static function for binary search in interval table */
static i4 
CM_bisearch(UTF32 ucs, CM_WC_WIDTH_INTERVAL *table, i4 max) 
{
  i4 min = 0;
  i4 mid;

  if (ucs < table[0].first || ucs > table[max].last)
    return 0;
  while (max >= min) {
    mid = (min + max) / 2;
    if (ucs > table[mid].last)
      min = mid + 1;
    else if (ucs < table[mid].first)
      max = mid - 1;
    else
      return 1;
  }

  return 0;
}

/* Name: CM_mk_wcwidth - Compute width of a Unicode character.
**
** The following function define the column width of an ISO 10646
** character as follows:
**
**    - The null character (U+0000) has a column width of 0.
**
**    - Other C0/C1 control characters and DEL will lead to a return
**      value of -1.
**
**    - Non-spacing and enclosing combining characters (general
**      category code Mn or Me in the Unicode database) have a
**      column width of 0.
**
**    - SOFT HYPHEN (U+00AD) has a column width of 1.
**
**    - Other format characters (general category code Cf in the Unicode
**      database) and ZERO WIDTH SPACE (U+200B) have a column width of 0.
**
**    - Hangul Jamo medial vowels and final consonants (U+1160-U+11FF)
**      have a column width of 0.
**
**    - Spacing characters in the East Asian Wide (W) or East Asian
**      Full-width (F) category as defined in Unicode Technical
**      Report #11 have a column width of 2.
**
**    - All remaining characters (including all printable
**      ISO 8859-1 and WGL4 characters, Unicode control characters,
**      etc.) have a column width of 1.
**
** History:
**	
**	17-Nov-2008 (gupsh01)
**	    Added.
**/

static i4 
CM_mk_wcwidth(UTF32 ucs)
{
  /* sorted list of non-overlapping intervals of non-spacing characters */
  /* generated by "uniset +cat=Me +cat=Mn +cat=Cf -00AD +1160-11FF +200B c" */
  CM_WC_WIDTH_INTERVAL combining[] = {
    { 0x0300, 0x036F }, { 0x0483, 0x0486 }, { 0x0488, 0x0489 },
    { 0x0591, 0x05BD }, { 0x05BF, 0x05BF }, { 0x05C1, 0x05C2 },
    { 0x05C4, 0x05C5 }, { 0x05C7, 0x05C7 }, { 0x0600, 0x0603 },
    { 0x0610, 0x0615 }, { 0x064B, 0x065E }, { 0x0670, 0x0670 },
    { 0x06D6, 0x06E4 }, { 0x06E7, 0x06E8 }, { 0x06EA, 0x06ED },
    { 0x070F, 0x070F }, { 0x0711, 0x0711 }, { 0x0730, 0x074A },
    { 0x07A6, 0x07B0 }, { 0x07EB, 0x07F3 }, { 0x0901, 0x0902 },
    { 0x093C, 0x093C }, { 0x0941, 0x0948 }, { 0x094D, 0x094D },
    { 0x0951, 0x0954 }, { 0x0962, 0x0963 }, { 0x0981, 0x0981 },
    { 0x09BC, 0x09BC }, { 0x09C1, 0x09C4 }, { 0x09CD, 0x09CD },
    { 0x09E2, 0x09E3 }, { 0x0A01, 0x0A02 }, { 0x0A3C, 0x0A3C },
    { 0x0A41, 0x0A42 }, { 0x0A47, 0x0A48 }, { 0x0A4B, 0x0A4D },
    { 0x0A70, 0x0A71 }, { 0x0A81, 0x0A82 }, { 0x0ABC, 0x0ABC },
    { 0x0AC1, 0x0AC5 }, { 0x0AC7, 0x0AC8 }, { 0x0ACD, 0x0ACD },
    { 0x0AE2, 0x0AE3 }, { 0x0B01, 0x0B01 }, { 0x0B3C, 0x0B3C },
    { 0x0B3F, 0x0B3F }, { 0x0B41, 0x0B43 }, { 0x0B4D, 0x0B4D },
    { 0x0B56, 0x0B56 }, { 0x0B82, 0x0B82 }, { 0x0BC0, 0x0BC0 },
    { 0x0BCD, 0x0BCD }, { 0x0C3E, 0x0C40 }, { 0x0C46, 0x0C48 },
    { 0x0C4A, 0x0C4D }, { 0x0C55, 0x0C56 }, { 0x0CBC, 0x0CBC },
    { 0x0CBF, 0x0CBF }, { 0x0CC6, 0x0CC6 }, { 0x0CCC, 0x0CCD },
    { 0x0CE2, 0x0CE3 }, { 0x0D41, 0x0D43 }, { 0x0D4D, 0x0D4D },
    { 0x0DCA, 0x0DCA }, { 0x0DD2, 0x0DD4 }, { 0x0DD6, 0x0DD6 },
    { 0x0E31, 0x0E31 }, { 0x0E34, 0x0E3A }, { 0x0E47, 0x0E4E },
    { 0x0EB1, 0x0EB1 }, { 0x0EB4, 0x0EB9 }, { 0x0EBB, 0x0EBC },
    { 0x0EC8, 0x0ECD }, { 0x0F18, 0x0F19 }, { 0x0F35, 0x0F35 },
    { 0x0F37, 0x0F37 }, { 0x0F39, 0x0F39 }, { 0x0F71, 0x0F7E },
    { 0x0F80, 0x0F84 }, { 0x0F86, 0x0F87 }, { 0x0F90, 0x0F97 },
    { 0x0F99, 0x0FBC }, { 0x0FC6, 0x0FC6 }, { 0x102D, 0x1030 },
    { 0x1032, 0x1032 }, { 0x1036, 0x1037 }, { 0x1039, 0x1039 },
    { 0x1058, 0x1059 }, { 0x1160, 0x11FF }, { 0x135F, 0x135F },
    { 0x1712, 0x1714 }, { 0x1732, 0x1734 }, { 0x1752, 0x1753 },
    { 0x1772, 0x1773 }, { 0x17B4, 0x17B5 }, { 0x17B7, 0x17BD },
    { 0x17C6, 0x17C6 }, { 0x17C9, 0x17D3 }, { 0x17DD, 0x17DD },
    { 0x180B, 0x180D }, { 0x18A9, 0x18A9 }, { 0x1920, 0x1922 },
    { 0x1927, 0x1928 }, { 0x1932, 0x1932 }, { 0x1939, 0x193B },
    { 0x1A17, 0x1A18 }, { 0x1B00, 0x1B03 }, { 0x1B34, 0x1B34 },
    { 0x1B36, 0x1B3A }, { 0x1B3C, 0x1B3C }, { 0x1B42, 0x1B42 },
    { 0x1B6B, 0x1B73 }, { 0x1DC0, 0x1DCA }, { 0x1DFE, 0x1DFF },
    { 0x200B, 0x200F }, { 0x202A, 0x202E }, { 0x2060, 0x2063 },
    { 0x206A, 0x206F }, { 0x20D0, 0x20EF }, { 0x302A, 0x302F },
    { 0x3099, 0x309A }, { 0xA806, 0xA806 }, { 0xA80B, 0xA80B },
    { 0xA825, 0xA826 }, { 0xFB1E, 0xFB1E }, { 0xFE00, 0xFE0F },
    { 0xFE20, 0xFE23 }, { 0xFEFF, 0xFEFF }, { 0xFFF9, 0xFFFB },
    { 0x10A01, 0x10A03 }, { 0x10A05, 0x10A06 }, { 0x10A0C, 0x10A0F },
    { 0x10A38, 0x10A3A }, { 0x10A3F, 0x10A3F }, { 0x1D167, 0x1D169 },
    { 0x1D173, 0x1D182 }, { 0x1D185, 0x1D18B }, { 0x1D1AA, 0x1D1AD },
    { 0x1D242, 0x1D244 }, { 0xE0001, 0xE0001 }, { 0xE0020, 0xE007F },
    { 0xE0100, 0xE01EF }
  };

  /* test for 8-bit control characters
  ** Note: Originally the routine returns 0 for '\0' and -1 for
  ** all control characters between [0x01 - 0x1F] and [0x7f - 0x9F]
  ** However we would return 1 for all such characters as in Ingres
  ** Front end is aware of how to handle these characters.
  */
  if (ucs < 32 || (ucs >= 0x7f && ucs < 0xa0))
    return 1;

  /* binary search in table of non-spacing characters */
  if (CM_bisearch(ucs, combining,
	       sizeof(combining) / sizeof(CM_WC_WIDTH_INTERVAL) - 1))
    return 0;

  /* if we arrive here, ucs is not a combining or C0/C1 control character */

  return 1 + 
    (ucs >= 0x1100 &&
     (ucs <= 0x115f ||                    /* Hangul Jamo init. consonants */
      ucs == 0x2329 || ucs == 0x232a ||
      (ucs >= 0x2e80 && ucs <= 0xa4cf &&
       ucs != 0x303f) ||                  /* CJK ... Yi */
      (ucs >= 0xac00 && ucs <= 0xd7a3) || /* Hangul Syllables */
      (ucs >= 0xf900 && ucs <= 0xfaff) || /* CJK Compatibility Ideographs */
      (ucs >= 0xfe10 && ucs <= 0xfe19) || /* Vertical forms */
      (ucs >= 0xfe30 && ucs <= 0xfe6f) || /* CJK Compatibility Forms */
      (ucs >= 0xff00 && ucs <= 0xff60) || /* Fullwidth Forms */
      (ucs >= 0xffe0 && ucs <= 0xffe6) ||
      (ucs >= 0x20000 && ucs <= 0x2fffd) ||
      (ucs >= 0x30000 && ucs <= 0x3fffd)));
}

/* Name: CM_UTF8_twidth - Finds the width of a UTF8 string when
**			  displayed on a UTF8 termainal.
**
** Description:
**
**     This routine takes in a UTF8 character and calculates the 
**     terminal width it occupies.
**
** Input:
**		strstart 	Pointer to the start of the UTF8 
**			        character, whose width we need to
**				compute. 
**		strend	 	Pointer to the end of the UTF8 
**			        character, whose width we need to
**				compute. This ensures that we do
**				have access to the complete sequence
**				of a multibyte UTF8 character.
**
** Output			Width occupied by the UTF8 character
**				on a fixed width UTF8 compatible terminal. 
**				Typically East asian characters will 
**				occupy two spaces and European characters
**				will occupy one space. 
**		
** Returns:			width if ok
**				-1 Other wise.
**
** Note: This is an approximate value and may not work for all kinds of 
**	 characters and all terminals. (see note above)
**
** Usage: Typically this routine would be used to calculate the difference 
** 	  in length of the UTF8 character in bytes and the width of the 
**	  character expressed in number of spaces occupied by this character. 
** 	  on a fixed width UTF8 enabled terminal. This diffence can be used to
**	  compensate for the dis-alignment that occurs when the bytes shrink
**	  on rendering this character.
**
**        if ((utf8wid = CM_UTF8_twidth(str_start, str_end)) > 0)
**        {
**	    diff = CMbytecnt(str_start) - utf8wid;
**	
** History:
**	17-Nov-2008 (gupsh01)
**	    Added.
*/
i4
CM_UTF8_twidth (char *strstart, char *strend)
{
        i4      i, n;
        ConversionResult result;
        UTF32   utf32_buf[2];
        UTF8    utf8_buf[5];
        UTF8    *utf8SourceStart;
        UTF32   *utf32TargetStart;
	i4	width = 0;
	i4	incnt = CMbytecnt(strstart);

	if (strstart + incnt > strend)
          return (-1);

        utf32_buf[0] = 0; utf32_buf[1] = 0;

        for (n = 0; n < incnt; n++)
          utf8_buf[n] = strstart[n];

        utf8SourceStart = utf8_buf;
        utf32TargetStart = utf32_buf;

        if ((result = CM_ConvertUTF8toUTF32( (const UTF8 **) &utf8SourceStart,
                    		             &(utf8_buf[incnt]), 
				             &utf32TargetStart,
                     		             &(utf32_buf[1]), 0)) !=  conversionOK)
        {
          return (-1);
        }

        if ((width = CM_mk_wcwidth(utf32_buf[0])) < 0)
          return -1;
        else
	  return (width);
}
