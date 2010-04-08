/*
** Copyright (c) 1986, 2008 Ingres Corporation
*/

# include	  <compat.h>
# include	  <gl.h>
# include	  <cm.h>  

/*
** CM_DefAttrTab
**	An array used by the CM macros to determine character types.
**
**
LIBRARY = IMPCOMPATLIBDATA
**
** History:
**		17-jan-86 (jeff) -- Made a space a printing character, as it
**				is on UNIX.  Fixes bug #5095.
**		05-sep-86 (yamamoto)
**			changed from a 'char' array to a 'u_i2' array to 
**			accomodate some additional bits.
**			The new bits are _K (katakana), _DBL1 (1st byte of
**			two byte character) and _DBL2 (2nd byte of two byte
**			character), _NMSTARTART (valid leading character of
**			a name), and _NMCHAR (valid character in a name).
**		23-feb-1987 (yamamoto)
**			added _HEX and _OPER for checking hexadecimal digits
**			and operator characters.
**		06-mar-1987 (fred) -- Added psect definition for use in
**			building shareable image of CL on VMS.
**		10-mar-1987 (peter) -- Try to combine all the sets  of
**			character tables into one routine, for ease of
**			maintenance.
**		11-may-1988 (ricks) -- Made ifdefs simpler and added 
**			support for DEC kanji.  Added character set 
**			identifiers to correspond with cm.h.
**		30-May-1989 (fredv)
**			Integrated 5.0 support for IBM's Code Page 0 8 bit 
**			character set to 6.1.
**		21-jun-89 (russ)
**			Modifying the upper 128 chars of the default CSASCII
**			character set, such that it conforms to the default
**			character set we used in 5.0.
**		29-Jun-1989 (anton)
**			moved russ' CSASCII change to development path.  New
**			CSASCII matches ISO 8859-1 (Latin-1).
**		09-may-90 (blaise)
**			Integrated 61 changes into 63p library.
**		23-Aug-90 (bobm)
**			Change name - this is now the default array
**			we will get if CMset_attr() is never called.
**			The name change will prevent us from linking if
**			we forget to recompile something.  We leave it
**			ifdef'ed so that the compiled-in default may be
**			changed.
**			The old CMshift table used on some character sets
**			works the same as the new stuff, so we just use it,
**			and provide one for the standard ascii case.  Also
**			had the name changed.
**              28-Aug-90 (stevet)
**                      Changed '\xd7' and '\xf7' to '_P' and changed 
**                      '\xdf' and '\xff' to '_I' on the CSASCII table
**                      to match 8859-1 standard.
**                      Also changed '\xdf' to '_I' on the CSDECMULTI
**                      table.  Added the CM_DefCaseTab for CSDECMULTI.
**               1-mar-91 (stevet)
**                      Added CSSUNKANJI back, which was lost with unknown
**                      reason.  Added CM_DefCaseTab for CSDECKANJI and
**                      CSSUNKANJI.
**		07-jan-92 (bonobo)
**			Added the following change from ingres63p cl:
**			19-aug-91 (seng).
**		19-aug-91 (seng)
**			Removed first element of CSIBM's char set.  The initial
**			purpose to have this element is to prevent EOF 
**			(aka -1) from accessing something before the start
**			of the array.  This is shown not to be the case after 
**			testing without the first element.
**		24-mar-1992 (mgw) Bug #43004
**			Fixed CM_DefCaseTab[] mappings for the 8 bit characters
**			('\300' - '\375') to correctly translate case in the
**			CSSUNKANJI and CSDECMULTI cases. Also changed
**			CM_DefCaseTab[] to be just char, not unsigned char,
**			to conform with CL spec setting for the CMATTR xcase
**			structure member. CM_CaseTab can be set to point to
**			this or CM_DefCaseTab[] so they should all match.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	13-mar-1995(angusm)
**		Add extra variable to track identity of 'compiled-in'
**		default character set. This is so that defaults are correctly
**		set up if II_CHARSETxx is not set (bug 56258).
**
**		Despite comments to the contrary, 'mkbzarch' appears
**		to support only: 
**
**		CSASCII -   ISO 8895/1 
**		CSIBM	-	IBM PC code page 850 (rs6000, also os/2)
**
**		while VMS appears to support only CSDECMULTI 
**		(for both single and double-byte). 
**		This change therefore ignores the character sets 
**		for compiler macros CSDECKANJI , CSSUNKANJI, CSBURROUGHS, JIS  
**		which do not appear to map on to existing character sets
**		in common!gcf!files/gcccset.nam
**	03-feb-1998(yosts50)(Yoshizawa from Fujitsu)
**		Change the shiftjis table to support user-defined characters
**	20-may-1998 (canor01)
**	    Add support for CSKOREAN.
**	28-may-1998 (yosts50)
**	    Modify the korean table to support extend characters
**	02-jun-1998 (canor01)
**	    Restore "else" that was inadvertently dropped while emailing 
**	    changes around.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	13-apr-2007 (gupsh01)
**	    Added support for UTF8 character sets.
**	14-may-2007 (gupsh01)
**	    Modified the CM_UTF8Bytes table.
**	22-may-2007 (gupsh01)
**	    Added CM_utf8casetab and CM_utf8attrtab.
**	04-jun-2008 (gupsh01)
**	    Modify CM_utf8casetab and CM_utf8attrtab
**	    to CM_casetab_UTF8 and CM_attrtab_UTF8.
**	22-sep-2008 (gupsh01)
**	    Remove the Unicode case and properties tables
**	    from here to cmutype.roc.
**      18-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
*/

/*
** rather than cause diff's on all the lines, just redefine obsolete names
** locally
*/
# define _IU		CM_A_UPPER
# define _IL		CM_A_LOWER
# define _K		CM_A_NCALPHA
# define _I		CM_A_NCALPHA
# define _U		CM_A_UPPER
# define _L		CM_A_LOWER
# define _N		CM_A_DIGIT
# define _S		CM_A_SPACE
# define _P		CM_A_PRINT
# define _C		CM_A_CONTROL
# define _DBL1		CM_A_DBL1
# define _DBL2		CM_A_DBL2
# define _NMSTART	CM_A_NMSTART
# define _NMCHAR	CM_A_NMCHAR
# ifdef _HEX
#   undef _HEX
# endif 
# define _HEX		CM_A_HEX
# define _OPER		CM_A_OPER

# ifdef VMS
globaldef {"iicl_gvar$readonly"} const u_i2 CM_DefAttrTab[] =
# else		/* VMS */
GLOBALDEF const u_i2 CM_DefAttrTab[] =
# endif		/* VMS */

{
/*
**		STANDARD ASCII CHARACTER SET
*/

# ifdef CSASCII
/* 00 */	_C,			_C,			_C,			_C,
/* 04 */	_C,			_C,			_C,			_C,
/* 08 */	_C,			_S|_C,			_S|_C,			_S|_C,
/* 0C */	_S|_C,			_S|_C,			_C,			_C,
/* 10 */	_C,			_C,			_C,			_C,
/* 14 */	_C,			_C,			_C,			_C,
/* 18 */	_C,			_C,			_C,			_C,
/* 1C */	_C,			_C,			_C,			_C,
/* 20 */	_S|_P,			_P|_OPER,		_P|_OPER,		_P|_NMCHAR|_OPER,
/* 24 */	_P|_NMCHAR|_OPER,	_P|_OPER,		_P|_OPER,		_P|_OPER,
/* 28 */	_P|_OPER,		_P|_OPER,		_P|_OPER,		_P|_OPER,
/* 2C */	_P|_OPER,		_P|_OPER,		_P|_OPER,		_P|_OPER,
/* 30 */	_N|_NMCHAR|_HEX,	_N|_NMCHAR|_HEX,	_N|_NMCHAR|_HEX,	_N|_NMCHAR|_HEX,
/* 34 */	_N|_NMCHAR|_HEX,	_N|_NMCHAR|_HEX,	_N|_NMCHAR|_HEX,	_N|_NMCHAR|_HEX,
/* 38 */	_N|_NMCHAR|_HEX,	_N|_NMCHAR|_HEX,	_P|_OPER,		_P|_OPER,
/* 3C */	_P|_OPER,		_P|_OPER,		_P|_OPER,		_P|_OPER,

/* 40 */	_P|_NMCHAR|_OPER,	_U|_NMSTART|_HEX,	_U|_NMSTART|_HEX,	_U|_NMSTART|_HEX,
/* 44 */	_U|_NMSTART|_HEX,	_U|_NMSTART|_HEX,	_U|_NMSTART|_HEX,	_U|_NMSTART,
/* 48 */	_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,
/* 4C */	_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,
/* 50 */	_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,
/* 54 */	_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,
/* 58 */	_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,		_P|_OPER,
/* 5C */	_P|_OPER,		_P|_OPER,		_P|_OPER,		_P|_NMSTART,
/* 60 */	_P|_OPER,		_L|_NMSTART|_HEX,	_L|_NMSTART|_HEX,	_L|_NMSTART|_HEX,
/* 64 */	_L|_NMSTART|_HEX,	_L|_NMSTART|_HEX,	_L|_NMSTART|_HEX,	_L|_NMSTART,
/* 68 */	_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,
/* 6C */	_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,
/* 70 */	_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,
/* 74 */	_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,
/* 78 */	_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,		_P|_OPER,
/* 7C */	_P|_OPER,		_P|_OPER,		_P|_OPER,		_C,
/* 80 */	_C,			_C,			_C,			_C,	
/* 84 */	_C,			_C,			_C,			_C,
/* 88 */	_C,			_C,			_C,			_C,
/* 8C */	_C,			_C,			_C,			_C,
/* 90 */	_C,			_C,			_C,			_C,	
/* 94 */	_C,			_C,			_C,			_C,	
/* 98 */	_C,			_C,			_C,			_C,	
/* 9C */	_C,			_C,			_C,			_C,
/* A0 */	_P,			_P,			_P,			_P,
/* A4 */	_P,			_P,			_P,			_P,	
/* A8 */	_P,			_P,			_P,			_P,	
/* AC */	_P,			_P,			_P,			_P,	
/* B0 */	_P,			_P,			_P,			_P,	
/* B4 */	_P,			_P,			_P,			_P,	
/* B8 */	_P,			_P,			_P,			_P,	
/* BC */	_P,			_P,			_P,			_P,	
/* C0 */	_IU|_NMSTART,		_IU|_NMSTART,		_IU|_NMSTART,		_IU|_NMSTART,	
/* C4 */	_IU|_NMSTART,		_IU|_NMSTART,		_IU|_NMSTART,		_IU|_NMSTART,	
/* C8 */	_IU|_NMSTART,		_IU|_NMSTART,		_IU|_NMSTART,		_IU|_NMSTART,	
/* CC */	_IU|_NMSTART,		_IU|_NMSTART,		_IU|_NMSTART,		_IU|_NMSTART,	
/* D0 */	_IU|_NMSTART,		_IU|_NMSTART,		_IU|_NMSTART,		_IU|_NMSTART,	
/* D4 */	_IU|_NMSTART,		_IU|_NMSTART,		_IU|_NMSTART,		_P,	
/* D8 */	_IU|_NMSTART,		_IU|_NMSTART,		_IU|_NMSTART,		_IU|_NMSTART,	
/* DC */	_IU|_NMSTART,		_IU|_NMSTART,		_IU|_NMSTART,		_I|_NMSTART,
/* E0 */	_IL|_NMSTART,		_IL|_NMSTART,		_IL|_NMSTART,		_IL|_NMSTART,	
/* E4 */	_IL|_NMSTART,		_IL|_NMSTART,		_IL|_NMSTART,		_IL|_NMSTART,	
/* E8 */	_IL|_NMSTART,		_IL|_NMSTART,		_IL|_NMSTART,		_IL|_NMSTART,	
/* EC */	_IL|_NMSTART,		_IL|_NMSTART,		_IL|_NMSTART,		_IL|_NMSTART,	
/* F0 */	_IL|_NMSTART,		_IL|_NMSTART,		_IL|_NMSTART,		_IL|_NMSTART,	
/* F4 */	_IL|_NMSTART,		_IL|_NMSTART,		_IL|_NMSTART,		_P,	
/* F8 */	_IL|_NMSTART,		_IL|_NMSTART,		_IL|_NMSTART,		_IL|_NMSTART,
/* FC */	_IL|_NMSTART,		_IL|_NMSTART,		_IL|_NMSTART,		_I|_NMSTART

# else

/*
**		DEC MULTINATIONAL CHARACTER SET
*/

# ifdef CSDECMULTI
/* 00 */	_C,			_C,			_C,			_C,
/* 04 */	_C,			_C,			_C,			_C,
/* 08 */	_C,			_S|_C,			_S|_C,			_S|_C,
/* 0C */	_S|_C,			_S|_C,			_C,			_C,
/* 10 */	_C,			_C,			_C,			_C,
/* 14 */	_C,			_C,			_C,			_C,
/* 18 */	_C,			_C,			_C,			_C,
/* 1C */	_C,			_C,			_C,			_C,
/* 20 */	_S|_P,			_P|_OPER,		_P|_OPER,		_P|_NMCHAR|_OPER,
/* 24 */	_P|_NMCHAR|_OPER,	_P|_OPER,		_P|_OPER,		_P|_OPER,
/* 28 */	_P|_OPER,		_P|_OPER,		_P|_OPER,		_P|_OPER,
/* 2C */	_P|_OPER,		_P|_OPER,		_P|_OPER,		_P|_OPER,
/* 30 */	_N|_NMCHAR|_HEX,	_N|_NMCHAR|_HEX,	_N|_NMCHAR|_HEX,	_N|_NMCHAR|_HEX,
/* 34 */	_N|_NMCHAR|_HEX,	_N|_NMCHAR|_HEX,	_N|_NMCHAR|_HEX,	_N|_NMCHAR|_HEX,
/* 38 */	_N|_NMCHAR|_HEX,	_N|_NMCHAR|_HEX,	_P|_OPER,		_P|_OPER,
/* 3C */	_P|_OPER,		_P|_OPER,		_P|_OPER,		_P|_OPER,

/* 40 */	_P|_NMCHAR|_OPER,	_U|_NMSTART|_HEX,	_U|_NMSTART|_HEX,	_U|_NMSTART|_HEX,
/* 44 */	_U|_NMSTART|_HEX,	_U|_NMSTART|_HEX,	_U|_NMSTART|_HEX,	_U|_NMSTART,
/* 48 */	_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,
/* 4C */	_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,
/* 50 */	_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,
/* 54 */	_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,
/* 58 */	_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,		_P|_OPER,
/* 5C */	_P|_OPER,		_P|_OPER,		_P|_OPER,		_P|_NMSTART,
/* 60 */	_P|_OPER,		_L|_NMSTART|_HEX,	_L|_NMSTART|_HEX,	_L|_NMSTART|_HEX,
/* 64 */	_L|_NMSTART|_HEX,	_L|_NMSTART|_HEX,	_L|_NMSTART|_HEX,	_L|_NMSTART,
/* 68 */	_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,
/* 6C */	_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,
/* 70 */	_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,
/* 74 */	_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,
/* 78 */	_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,		_P|_OPER,
/* 7C */	_P|_OPER,		_P|_OPER,		_P|_OPER,		_C,

/* 80 */	_C,			_C,			_C,			_C,
/* 84 */	_C,			_C,			_C,			_C,
/* 88 */	_C,			_C,			_C,			_C,
/* 8C */	_C,			_C,			_C,			_C,
/* 90 */	_C,			_C,			_C,			_C,	
/* 94 */	_C,			_C,			_C,			_C,	
/* 98 */	_C,			_C,			_C,			_C,	
/* 9C */	_C,			_C,			_C,			_C,	
/* A0 */	_C,			_P,			_P,			_P,
/* A4 */	_C,			_P,			_C,			_P,	
/* A8 */	_P,			_P,			_P,			_P,	
/* AC */	_C,			_C,			_C,			_C,	
/* B0 */	_P,			_P,			_P,			_P,	
/* B4 */	_C,			_P,			_P,			_P,	
/* B8 */	_C,			_P,			_P,			_P,	
/* BC */	_P,			_P,			_C,			_P,	
/* C0 */	_IU|_NMSTART,		_IU|_NMSTART,		_IU|_NMSTART,		_IU|_NMSTART,	
/* C4 */	_IU|_NMSTART,		_IU|_NMSTART,		_IU|_NMSTART,		_IU|_NMSTART,	
/* C8 */	_IU|_NMSTART,		_IU|_NMSTART,		_IU|_NMSTART,		_IU|_NMSTART,	
/* CC */	_IU|_NMSTART,		_IU|_NMSTART,		_IU|_NMSTART,		_IU|_NMSTART,	
/* D0 */	_C,			_IU|_NMSTART,		_IU|_NMSTART,		_IU|_NMSTART,	
/* D4 */	_IU|_NMSTART,		_IU|_NMSTART,		_IU|_NMSTART,		_IU|_NMSTART,	
/* D8 */	_IU|_NMSTART,		_IU|_NMSTART,		_IU|_NMSTART,		_IU|_NMSTART,	
/* DC */	_IU|_NMSTART,		_IU|_NMSTART,		_C,			_I|_NMSTART,
/* E0 */	_IL|_NMSTART,		_IL|_NMSTART,		_IL|_NMSTART,		_IL|_NMSTART,	
/* E4 */	_IL|_NMSTART,		_IL|_NMSTART,		_IL|_NMSTART,		_IL|_NMSTART,	
/* E8 */	_IL|_NMSTART,		_IL|_NMSTART,		_IL|_NMSTART,		_IL|_NMSTART,	
/* EC */	_IL|_NMSTART,		_IL|_NMSTART,		_IL|_NMSTART,		_IL|_NMSTART,	
/* F0 */	_C,			_IL|_NMSTART,		_IL|_NMSTART,		_IL|_NMSTART,	
/* F4 */	_IL|_NMSTART,		_IL|_NMSTART,		_IL|_NMSTART,		_IL|_NMSTART,	
/* F8 */	_IL|_NMSTART,		_IL|_NMSTART,		_IL|_NMSTART,		_IL|_NMSTART,
/* FC */	_IL|_NMSTART,		_IL|_NMSTART,		_C,			_C			

# else

/*
**		DEC KANJI CHARACTER SET (Standard ASCII plus kanji)
*/
# ifdef CSDECKANJI
/* 00 */	_C,			_C,			_C,			_C,
/* 04 */	_C,			_C,			_C,			_C,
/* 08 */	_C,			_S|_C,			_S|_C,			_S|_C,
/* 0C */	_S|_C,			_S|_C,			_C,			_C,
/* 10 */	_C,			_C,			_C,			_C,
/* 14 */	_C,			_C,			_C,			_C,
/* 18 */	_C,			_C,			_C,			_C,
/* 1C */	_C,			_C,			_C,			_C,
/* 20 */	_S|_P,			_P|_OPER|_DBL2,		_P|_OPER|_DBL2,		_P|_NMCHAR|_OPER|_DBL2,
/* 24 */	_P|_NMCHAR|_OPER|_DBL2,	_P|_OPER|_DBL2,		_P|_OPER|_DBL2,		_P|_OPER|_DBL2,
/* 28 */	_P|_OPER|_DBL2,		_P|_OPER|_DBL2,		_P|_OPER|_DBL2,		_P|_OPER|_DBL2,
/* 2C */	_P|_OPER|_DBL2,		_P|_OPER|_DBL2,		_P|_OPER|_DBL2,		_P|_OPER|_DBL2,
/* 30 */	_N|_NMCHAR|_HEX|_DBL2,	_N|_NMCHAR|_HEX|_DBL2,	_N|_NMCHAR|_HEX|_DBL2,	_N|_NMCHAR|_HEX|_DBL2,
/* 34 */	_N|_NMCHAR|_HEX|_DBL2,	_N|_NMCHAR|_HEX|_DBL2,	_N|_NMCHAR|_HEX|_DBL2,	_N|_NMCHAR|_HEX|_DBL2,
/* 38 */	_N|_NMCHAR|_HEX|_DBL2,	_N|_NMCHAR|_HEX|_DBL2,	_P|_OPER|_DBL2,		_P|_OPER|_DBL2,
/* 3C */	_P|_OPER|_DBL2,		_P|_OPER|_DBL2,		_P|_OPER|_DBL2,		_P|_OPER|_DBL2,

/* 40 */	_P|_NMCHAR|_OPER|_DBL2,	_U|_NMSTART|_HEX|_DBL2,	_U|_NMSTART|_HEX|_DBL2,	_U|_NMSTART|_HEX|_DBL2,
/* 44 */	_U|_NMSTART|_HEX|_DBL2,	_U|_NMSTART|_HEX|_DBL2,	_U|_NMSTART|_HEX|_DBL2,	_U|_NMSTART|_DBL2,
/* 48 */	_U|_NMSTART|_DBL2,	_U|_NMSTART|_DBL2,	_U|_NMSTART|_DBL2,	_U|_NMSTART|_DBL2,
/* 4C */	_U|_NMSTART|_DBL2,	_U|_NMSTART|_DBL2,	_U|_NMSTART|_DBL2,	_U|_NMSTART|_DBL2,
/* 50 */	_U|_NMSTART|_DBL2,	_U|_NMSTART|_DBL2,	_U|_NMSTART|_DBL2,	_U|_NMSTART|_DBL2,
/* 54 */	_U|_NMSTART|_DBL2,	_U|_NMSTART|_DBL2,	_U|_NMSTART|_DBL2,	_U|_NMSTART|_DBL2,
/* 58 */	_U|_NMSTART|_DBL2,	_U|_NMSTART|_DBL2,	_U|_NMSTART|_DBL2,	_P|_OPER|_DBL2,
/* 5C */	_P|_OPER|_DBL2,		_P|_OPER|_DBL2,		_P|_OPER|_DBL2,		_P|_NMSTART|_DBL2,
/* 60 */	_P|_OPER|_DBL2,		_L|_NMSTART|_HEX|_DBL2,	_L|_NMSTART|_HEX|_DBL2,	_L|_NMSTART|_HEX|_DBL2,
/* 64 */	_L|_NMSTART|_HEX|_DBL2,	_L|_NMSTART|_HEX|_DBL2,	_L|_NMSTART|_HEX|_DBL2,	_L|_NMSTART|_DBL2,
/* 68 */	_L|_NMSTART|_DBL2,	_L|_NMSTART|_DBL2,	_L|_NMSTART|_DBL2,	_L|_NMSTART|_DBL2,
/* 6C */	_L|_NMSTART|_DBL2,	_L|_NMSTART|_DBL2,	_L|_NMSTART|_DBL2,	_L|_NMSTART|_DBL2,
/* 70 */	_L|_NMSTART|_DBL2,	_L|_NMSTART|_DBL2,	_L|_NMSTART|_DBL2,	_L|_NMSTART|_DBL2,
/* 74 */	_L|_NMSTART|_DBL2,	_L|_NMSTART|_DBL2,	_L|_NMSTART|_DBL2,	_L|_NMSTART|_DBL2,
/* 78 */	_L|_NMSTART|_DBL2,	_L|_NMSTART|_DBL2,	_L|_NMSTART|_DBL2,	_P|_OPER|_DBL2,
/* 7C */	_P|_OPER|_DBL2,		_P|_OPER|_DBL2,		_P|_OPER|_DBL2,		_C,

/* 80 */	_C,			_C,			_C,			_C,
/* 84 */	_C,			_C,			_C,			_C,
/* 88 */	_C,			_C,			_C,			_C,
/* 8C */	_C,			_C,			_C,			_C,
/* 90 */	_C,			_C,			_C,			_C,
/* 94 */	_C,			_C,			_C,			_C,
/* 98 */	_C,			_C,			_C,			_C,
/* 9C */	_C,			_C,			_C,			_C,
/* A0 */	_C,			_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
/* A4 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
/* A8 */	_DBL2,			_DBL2,			_DBL2,			_DBL2,
/* AC */	_DBL2,			_DBL2,			_DBL2,			_DBL2,
/* B0 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
/* B4 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
/* B8 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
/* BC */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
/* C0 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
/* C4 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
/* C8 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
/* CC */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
/* D0 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
/* D4 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
/* D8 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
/* DC */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
/* E0 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
/* E4 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
/* E8 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
/* EC */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
/* F0 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
/* F4 */	_DBL2,			_DBL2,			_DBL2,			_DBL2,
/* F8 */	_DBL2,			_DBL2,			_DBL2,			_DBL2,
/* FC */	_DBL2,			_DBL2,			_DBL2,			_C

# else

/*
 **		SUN KANJI CHARACTER SET (Standard ASCII plus kanji)
 **			(JIS-X0208)	
 */
# ifdef CSSUNKANJI
 /* 00 */	_C,			_C,			_C,			_C,
 /* 04 */	_C,			_C,			_C,			_C,
 /* 08 */	_C,			_S|_C,			_S|_C,			_S|_C,
 /* 0C */	_S|_C,			_S|_C,			_C,			_C,
 /* 10 */	_C,			_C,			_C,			_C,
 /* 14 */	_C,			_C,			_C,			_C,
 /* 18 */	_C,			_C,			_C,			_C,
 /* 1C */	_C,			_C,			_C,			_C,
 /* 20 */	_S|_P,			_P|_OPER|_DBL2,		_P|_OPER|_DBL2,		_P|_NMCHAR|_OPER|_DBL2,
 /* 24 */	_P|_NMCHAR|_OPER|_DBL2,	_P|_OPER|_DBL2,		_P|_OPER|_DBL2,		_P|_OPER|_DBL2,
 /* 28 */	_P|_OPER|_DBL2,		_P|_OPER|_DBL2,		_P|_OPER|_DBL2,		_P|_OPER|_DBL2,
 /* 2C */	_P|_OPER|_DBL2,		_P|_OPER|_DBL2,		_P|_OPER|_DBL2,		_P|_OPER|_DBL2,
 /* 30 */	_N|_NMCHAR|_HEX|_DBL2,	_N|_NMCHAR|_HEX|_DBL2,	_N|_NMCHAR|_HEX|_DBL2,	_N|_NMCHAR|_HEX|_DBL2,
 /* 34 */	_N|_NMCHAR|_HEX|_DBL2,	_N|_NMCHAR|_HEX|_DBL2,	_N|_NMCHAR|_HEX|_DBL2,	_N|_NMCHAR|_HEX|_DBL2,
 /* 38 */	_N|_NMCHAR|_HEX|_DBL2,	_N|_NMCHAR|_HEX|_DBL2,	_P|_OPER|_DBL2,		_P|_OPER|_DBL2,
 /* 3C */	_P|_OPER|_DBL2,		_P|_OPER|_DBL2,		_P|_OPER|_DBL2,		_P|_OPER|_DBL2,

 /* 40 */	_P|_NMCHAR|_OPER|_DBL2,	_U|_NMSTART|_HEX|_DBL2,	_U|_NMSTART|_HEX|_DBL2,	_U|_NMSTART|_HEX|_DBL2,
 /* 44 */	_U|_NMSTART|_HEX|_DBL2,	_U|_NMSTART|_HEX|_DBL2,	_U|_NMSTART|_HEX|_DBL2,	_U|_NMSTART|_DBL2,
 /* 48 */	_U|_NMSTART|_DBL2,	_U|_NMSTART|_DBL2,	_U|_NMSTART|_DBL2,	_U|_NMSTART|_DBL2,
 /* 4C */	_U|_NMSTART|_DBL2,	_U|_NMSTART|_DBL2,	_U|_NMSTART|_DBL2,	_U|_NMSTART|_DBL2,
 /* 50 */	_U|_NMSTART|_DBL2,	_U|_NMSTART|_DBL2,	_U|_NMSTART|_DBL2,	_U|_NMSTART|_DBL2,
 /* 54 */	_U|_NMSTART|_DBL2,	_U|_NMSTART|_DBL2,	_U|_NMSTART|_DBL2,	_U|_NMSTART|_DBL2,
 /* 58 */	_U|_NMSTART|_DBL2,	_U|_NMSTART|_DBL2,	_U|_NMSTART|_DBL2,	_P|_OPER|_DBL2,
 /* 5C */	_P|_OPER|_DBL2,		_P|_OPER|_DBL2,		_P|_OPER|_DBL2,		_P|_NMSTART|_DBL2,
 /* 60 */	_P|_OPER|_DBL2,		_L|_NMSTART|_HEX|_DBL2,	_L|_NMSTART|_HEX|_DBL2,	_L|_NMSTART|_HEX|_DBL2,
 /* 64 */	_L|_NMSTART|_HEX|_DBL2,	_L|_NMSTART|_HEX|_DBL2,	_L|_NMSTART|_HEX|_DBL2,	_L|_NMSTART|_DBL2,
 /* 68 */	_L|_NMSTART|_DBL2,	_L|_NMSTART|_DBL2,	_L|_NMSTART|_DBL2,	_L|_NMSTART|_DBL2,
 /* 6C */	_L|_NMSTART|_DBL2,	_L|_NMSTART|_DBL2,	_L|_NMSTART|_DBL2,	_L|_NMSTART|_DBL2,
 /* 70 */	_L|_NMSTART|_DBL2,	_L|_NMSTART|_DBL2,	_L|_NMSTART|_DBL2,	_L|_NMSTART|_DBL2,
 /* 74 */	_L|_NMSTART|_DBL2,	_L|_NMSTART|_DBL2,	_L|_NMSTART|_DBL2,	_L|_NMSTART|_DBL2,
 /* 78 */	_L|_NMSTART|_DBL2,	_L|_NMSTART|_DBL2,	_L|_NMSTART|_DBL2,	_P|_OPER|_DBL2,
 /* 7C */	_P|_OPER|_DBL2,		_P|_OPER|_DBL2,		_P|_OPER|_DBL2,		_C,
 
 /* 80 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
 /* 84 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
 /* 88 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
 /* 8C */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
 /* 90 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
 /* 94 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
 /* 98 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
 /* 9C */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
 /* A0 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
 /* A4 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
 /* A8 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
 /* AC */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
 /* B0 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
 /* B4 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
 /* B8 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
 /* BC */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
 /* C0 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
 /* C4 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
 /* C8 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
 /* CC */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
 /* D0 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
 /* D4 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
 /* D8 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
 /* DC */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
 /* E0 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
 /* E4 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
 /* E8 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
 /* EC */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
 /* F0 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
 /* F4 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
 /* F8 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
 /* FC */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART
 

# else


/*
**		BURROUGHS PT TERMINAL CHARACTER SET
*/

# ifdef CSBURROUGHS
/* 00 */	_C,			_C,			_C,			_C,
/* 04 */	_C,			_C,			_C,			_C,
/* 08 */	_C,			_S|_C,			_S|_C,			_S|_C,
/* 0C */	_S|_C,			_S|_C,			_C,			_C,
/* 10 */	_C,			_C,			_C,			_C,
/* 14 */	_C,			_C,			_C,			_C,
/* 18 */	_C,			_C,			_C,			_C,
/* 1C */	_C,			_C,			_C,			_C,
/* 20 */	_S|_P,			_P|_OPER,		_P|_OPER,		_P|_NMCHAR|_OPER,
/* 24 */	_P|_NMCHAR|_OPER,	_P|_OPER,		_P|_OPER,		_P|_OPER,
/* 28 */	_P|_OPER,		_P|_OPER,		_P|_OPER,		_P|_OPER,
/* 2C */	_P|_OPER,		_P|_OPER,		_P|_OPER,		_P|_OPER,
/* 30 */	_N|_NMCHAR|_HEX,	_N|_NMCHAR|_HEX,	_N|_NMCHAR|_HEX,	_N|_NMCHAR|_HEX,
/* 34 */	_N|_NMCHAR|_HEX,	_N|_NMCHAR|_HEX,	_N|_NMCHAR|_HEX,	_N|_NMCHAR|_HEX,
/* 38 */	_N|_NMCHAR|_HEX,	_N|_NMCHAR|_HEX,	_P|_OPER,		_P|_OPER,
/* 3C */	_P|_OPER,		_P|_OPER,		_P|_OPER,		_P|_OPER,

/* 40 */	_P|_NMCHAR|_OPER,	_U|_NMSTART|_HEX,	_U|_NMSTART|_HEX,	_U|_NMSTART|_HEX,
/* 44 */	_U|_NMSTART|_HEX,	_U|_NMSTART|_HEX,	_U|_NMSTART|_HEX,	_U|_NMSTART,
/* 48 */	_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,
/* 4C */	_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,
/* 50 */	_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,
/* 54 */	_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,
/* 58 */	_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,		_P|_OPER,
/* 5C */	_P|_OPER,		_P|_OPER,		_P|_OPER,		_P|_NMSTART,
/* 60 */	_P|_OPER,		_L|_NMSTART|_HEX,	_L|_NMSTART|_HEX,	_L|_NMSTART|_HEX,
/* 64 */	_L|_NMSTART|_HEX,	_L|_NMSTART|_HEX,	_L|_NMSTART|_HEX,	_L|_NMSTART,
/* 68 */	_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,
/* 6C */	_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,
/* 70 */	_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,
/* 74 */	_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,
/* 78 */	_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,		_P|_OPER,
/* 7C */	_P|_OPER,		_P|_OPER,		_P|_OPER,		_C,

/* 80 */	_C,			_C,			_C,			_C,	
/* 84 */	_C,			_C,			_C,			_C,
/* 88 */	_C,			_C,			_C,			_C,	
/* 8C */	_C,			_C,			_C,			_C,
/* 90 */	_C,			_C,			_C,			_C,	
/* 94 */	_C,			_C,			_C,			_C,
/* 98 */	_C,			_C,			_C,			_C,	
/* 9C */	_C,			_C,			_C,			_C,

		/* Begin Burroughs PT International Characters (Set 0) */

/* A0 */	_IL|_NMSTART,		_IL|_NMSTART,		_IL|_NMSTART,		_IL|_NMSTART,	
/* A4 */	_IL|_NMSTART,		_IL|_NMSTART,		_IL|_NMSTART,		_IL|_NMSTART,
/* A8 */	_IL|_NMSTART,		_IL|_NMSTART,		_IL|_NMSTART,		_IL|_NMSTART,	
/* AC */	_IL|_NMSTART,		_I,			_I,			_I,
/* B0 */	_I,			_I,			_I,			_I,	
/* B4 */	_IL|_NMSTART,		_IL|_NMSTART,		_IL|_NMSTART,		_IL|_NMSTART,

		/* End Burroughs PT International Characters */

/* B8 */	_C,			_C,			_C,			_C,	
/* BC */	_C,			_C,			_C,			_C,
/* C0 */	_C,			_C,			_C,			_C,	
/* C4 */	_C,			_C,			_C,			_C,
/* C8 */	_C,			_C,			_C,			_C,	
/* CC */	_C,			_C,			_C,			_C,
/* D0 */	_C,			_C,			_C,			_C,	
/* D4 */	_C,			_C,			_C,			_C,
/* D8 */	_C,			_C,			_C,			_C,	
/* DC */	_C,			_C,			_C,			_C,
/* E0 */	_C,			_C,			_C,			_C,	
/* E4 */	_C,			_C,			_C,			_C,
/* E8 */	_C,			_C,			_C,			_C,	
/* EC */	_C,			_C,			_C,			_C,
/* F0 */	_C,			_C,			_C,			_C,	
/* F4 */	_C,			_C,			_C,			_C,
/* F8 */	_C,			_C,			_C,			_C,	
/* FC */	_C,			_C,			_C,			_C

# else

/*
**		JAPANESE INDUSTRIAL STANDARD (JIS) KANJI CHARACTER SET
*/

# ifdef JIS
/* 00 */	_C,			_C,			_C,			_C,
/* 04 */	_C,			_C,			_C,			_C,
/* 08 */	_C,			_S|_C,			_S|_C,			_S|_C,
/* 0C */	_S|_C,			_S|_C,			_C,			_C,
/* 10 */	_C,			_C,			_C,			_C,
/* 14 */	_C,			_C,			_C,			_C,
/* 18 */	_C,			_C,			_C,			_C,
/* 1C */	_C,			_C,			_C,			_C,
/* 20 */	_S|_P,			_P|_OPER,		_P|_OPER,		_P|_NMCHAR|_OPER,
/* 24 */	_P|_NMCHAR|_OPER,	_P|_OPER,		_P|_OPER,		_P|_OPER,
/* 28 */	_P|_OPER,		_P|_OPER,		_P|_OPER,		_P|_OPER,
/* 2C */	_P|_OPER,		_P|_OPER,		_P|_OPER,		_P|_OPER,
/* 30 */	_N|_NMCHAR|_HEX,	_N|_NMCHAR|_HEX,	_N|_NMCHAR|_HEX,	_N|_NMCHAR|_HEX,
/* 34 */	_N|_NMCHAR|_HEX,	_N|_NMCHAR|_HEX,	_N|_NMCHAR|_HEX,	_N|_NMCHAR|_HEX,
/* 38 */	_N|_NMCHAR|_HEX,	_N|_NMCHAR|_HEX,	_P|_OPER,		_P|_OPER,
/* 3C */	_P|_OPER,		_P|_OPER,		_P|_OPER,		_P|_OPER,

/*
**	For characters 40 - 76, the JIS character sets differs slightly
**	from the USASCII character set in that the patterns are valid
**	second byte kanji characters (which obviously are not supported
**	in USASCII).
*/

/* 40 */	_P|_DBL2|_NMCHAR|_OPER,	_U|_DBL2|_NMSTART|_HEX,	_U|_DBL2|_NMSTART|_HEX,	_U|_DBL2|_NMSTART|_HEX,
/* 44 */	_U|_DBL2|_NMSTART|_HEX,	_U|_DBL2|_NMSTART|_HEX,	_U|_DBL2|_NMSTART|_HEX,	_U|_DBL2|_NMSTART,
/* 48 */	_U|_DBL2|_NMSTART,	_U|_DBL2|_NMSTART,	_U|_DBL2|_NMSTART,	_U|_DBL2|_NMSTART,
/* 4C */	_U|_DBL2|_NMSTART,	_U|_DBL2|_NMSTART,	_U|_DBL2|_NMSTART,	_U|_DBL2|_NMSTART,
/* 50 */	_U|_DBL2|_NMSTART,	_U|_DBL2|_NMSTART,	_U|_DBL2|_NMSTART,	_U|_DBL2|_NMSTART,
/* 54 */	_U|_DBL2|_NMSTART,	_U|_DBL2|_NMSTART,	_U|_DBL2|_NMSTART,	_U|_DBL2|_NMSTART,
/* 58 */	_U|_DBL2|_NMSTART,	_U|_DBL2|_NMSTART,	_U|_DBL2|_NMSTART,	_P|_DBL2|_OPER,
/* 5C */	_P|_DBL2|_OPER,		_P|_DBL2|_OPER,		_P|_DBL2|_OPER,		_P|_DBL2|_NMSTART,
/* 60 */	_P|_DBL2|_OPER,		_L|_DBL2|_NMSTART|_HEX,	_L|_DBL2|_NMSTART|_HEX,	_L|_DBL2|_NMSTART|_HEX,
/* 64 */	_L|_DBL2|_NMSTART|_HEX,	_L|_DBL2|_NMSTART|_HEX,	_L|_DBL2|_NMSTART|_HEX,	_L|_DBL2|_NMSTART,
/* 68 */	_L|_DBL2|_NMSTART,	_L|_DBL2|_NMSTART,	_L|_DBL2|_NMSTART,	_L|_DBL2|_NMSTART,
/* 6C */	_L|_DBL2|_NMSTART,	_L|_DBL2|_NMSTART,	_L|_DBL2|_NMSTART,	_L|_DBL2|_NMSTART,
/* 70 */	_L|_DBL2|_NMSTART,	_L|_DBL2|_NMSTART,	_L|_DBL2|_NMSTART,	_L|_DBL2|_NMSTART,
/* 74 */	_L|_DBL2|_NMSTART,	_L|_DBL2|_NMSTART,	_L|_DBL2|_NMSTART,	_L|_DBL2|_NMSTART,
/* 78 */	_L|_DBL2|_NMSTART,	_L|_DBL2|_NMSTART,	_L|_DBL2|_NMSTART,	_P|_DBL2|_OPER,
/* 7C */	_P|_DBL2|_OPER,		_P|_DBL2|_OPER,		_P|_DBL2|_OPER,		_C,

/* 80 */	_DBL2,			_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
/* 84 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
/* 88 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
/* 8C */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
/* 90 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
/* 94 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
/* 98 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
/* 9C */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
/* A0 */	_DBL2,			_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,
/* A4 */	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,
/* A8 */	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,
/* AC */	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,
/* B0 */	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,
/* B4 */	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,
/* B8 */	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,
/* BC */	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,
/* C0 */	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,
/* C4 */	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,
/* C8 */	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,
/* CC */	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,
/* D0 */	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,
/* D4 */	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,
/* D8 */	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,
/* DC */	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,	_K|_DBL2|_NMSTART,
/* E0 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
/* E4 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
/* E8 */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,
/* EC */	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,	_DBL1|_DBL2|_NMSTART,

/* From 0xf0 to 0xfc can also be used as a first byte in shiftjis. */
/* Especially, codes between 0xf0 and 0xf9 can be used as a first  */
/* byte of a user-defined character.  1998/2/3 Yoshizawa(Fujitsu)  */

/* F0 */	_DBL1|_DBL2,	_DBL1|_DBL2,	_DBL1|_DBL2,	_DBL1|_DBL2,
/* F4 */	_DBL1|_DBL2,	_DBL1|_DBL2,	_DBL1|_DBL2,	_DBL1|_DBL2,
/* F8 */	_DBL1|_C|_DBL2,	_DBL1|_C|_DBL2,	_DBL1|_K|_DBL2,	_DBL1|_K|_DBL2,
/* FC */	_DBL1|_K|_DBL2,	_C,		_C,		_C

#else
/*
**  IBM's Code Page 0 8 bit character set.
**
*/

#ifdef CSIBM
/* 00 */	_C,			_C,			_C,			_C,
/* 04 */	_C,			_C,			_C,			_C,
/* 08 */	_C,			_S|_C,			_S|_C,			_S|_C,
/* 0C */	_S|_C,			_S|_C,			_C,			_C,
/* 10 */	_C,			_C,			_C,			_C,
/* 14 */	_C,			_C,			_C,			_C,
/* 18 */	_C,			_C,			_C,			_C,
/* 1C */	_C,			_C,			_C,			_C,
/* 20 */	_S|_P,			_P|_OPER,		_P|_OPER,		_P|_NMCHAR|_OPER,
/* 24 */	_P|_NMCHAR|_OPER,	_P|_OPER,		_P|_OPER,		_P|_OPER,
/* 28 */	_P|_OPER,		_P|_OPER,		_P|_OPER,		_P|_OPER,
/* 2C */	_P|_OPER,		_P|_OPER,		_P|_OPER,		_P|_OPER,
/* 30 */	_N|_NMCHAR|_HEX,	_N|_NMCHAR|_HEX,	_N|_NMCHAR|_HEX,	_N|_NMCHAR|_HEX,
/* 34 */	_N|_NMCHAR|_HEX,	_N|_NMCHAR|_HEX,	_N|_NMCHAR|_HEX,	_N|_NMCHAR|_HEX,
/* 38 */	_N|_NMCHAR|_HEX,	_N|_NMCHAR|_HEX,	_P|_OPER,		_P|_OPER,
/* 3C */	_P|_OPER,		_P|_OPER,		_P|_OPER,		_P|_OPER,

/* 40 */	_P|_NMCHAR|_OPER,	_U|_NMSTART|_HEX,	_U|_NMSTART|_HEX,	_U|_NMSTART|_HEX,
/* 44 */	_U|_NMSTART|_HEX,	_U|_NMSTART|_HEX,	_U|_NMSTART|_HEX,	_U|_NMSTART,
/* 48 */	_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,
/* 4C */	_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,
/* 50 */	_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,
/* 54 */	_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,
/* 58 */	_U|_NMSTART,		_U|_NMSTART,		_U|_NMSTART,		_P|_OPER,
/* 5C */	_P|_OPER,		_P|_OPER,		_P|_OPER,		_P|_NMSTART,
/* 60 */	_P|_OPER,		_L|_NMSTART|_HEX,	_L|_NMSTART|_HEX,	_L|_NMSTART|_HEX,
/* 64 */	_L|_NMSTART|_HEX,	_L|_NMSTART|_HEX,	_L|_NMSTART|_HEX,	_L|_NMSTART,
/* 68 */	_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,
/* 6C */	_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,
/* 70 */	_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,
/* 74 */	_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,
/* 78 */	_L|_NMSTART,		_L|_NMSTART,		_L|_NMSTART,		_P|_OPER,
/* 7C */	_P|_OPER,		_P|_OPER,		_P|_OPER,		_C,
/* 0x80 */	_IU,	_IL,	_IL,	_IL,	_IL,	_IL,	_IL,	_IL,
/* 0x88 */	_IL,	_IL,	_IL,	_IL,	_IL,	_IL,	_IU,	_IU,
/* 0x90 */	_IU,	_IL,	_IU,	_IL,	_IL,	_IL,	_IL,	_IL,
/* 0x98 */	_P,	_IU,	_IU,	_P,	_P,	_P,	_P,	_P,
/* 0xA0 */	_IL,	_IL,	_IL,	_IL,	_IL,	_IU,	_P,	_P,
/* 0xA8 */	_P,	_P,	_P,	_P,	_P,	_P,	_P,	_P,
/* 0xB0 */	_P,	_P,	_P,	_P,	_P,	_IU,	_IU,	_IU,
/* 0xB8 */	_P,	_P,	_P,	_P,	_P,	_P,	_P,	_P,
/* 0xC0 */	_P,	_P,	_P,	_P,	_P,	_P,	_IL,	_IU,
/* 0xC8 */	_P,	_P,	_P,	_P,	_P,	_P,	_P,	_P,
/* 0xD0 */	_P,	_P,	_IU,	_IU,	_IU,	_P,	_IU,	_IU,
/* 0xD8 */	_IU,	_P,	_P,	_P,	_P,	_P,	_IU,	_P,
/* 0xE0 */	_IU,	_P,	_IU,	_IU,	_IL,	_IU,	_P,	_P,
/* 0xE8 */	_P,	_IU,	_IU,	_IU,	_IL,	_IU,	_P,	_P,
/* 0xF0 */	_P,	_P,	_P,	_P,	_P,	_P,	_P,	_P,
/* 0xF8 */	_P,	_P,	_P,	_P,	_P,	_P,	_P,	_P


/*
 **		KOREAN
 */
# ifdef CSKOREAN
 /* 00 */	_C,			_C,			_C,			_C,
 /* 04 */	_C,			_C,			_C,			_C,
 /* 08 */	_C,			_S|_C,			_S|_C,			_S|_C,
 /* 0C */	_S|_C,			_S|_C,			_C,			_C,
 /* 10 */	_C,			_C,			_C,			_C,
 /* 14 */	_C,			_C,			_C,			_C,
 /* 18 */	_C,			_C,			_C,			_C,
 /* 1C */	_C,			_C,			_C,			_C,
 /* 20 */	_S|_P,			_P|_OPER,		_P|_OPER,		_P|_NMCHAR|_OPER,
 /* 24 */	_P|_NMCHAR|_OPER,	_P|_OPER,		_P|_OPER,		_P|_OPER,
 /* 28 */	_P|_OPER,		_P|_OPER,		_P|_OPER,		_P|_OPER,
 /* 2C */	_P|_OPER,		_P|_OPER,		_P|_OPER,		_P|_OPER,
 /* 30 */	_P|_N|_NMCHAR|_HEX,	_P|_N|_NMCHAR|_HEX,	_P|_N|_NMCHAR|_HEX,	_P|_N|_NMCHAR|_HEX,
 /* 34 */	_P|_N|_NMCHAR|_HEX,	_P|_N|_NMCHAR|_HEX,	_P|_N|_NMCHAR|_HEX,	_P|_N|_NMCHAR|_HEX,
 /* 38 */	_P|_N|_NMCHAR|_HEX,	_P|_N|_NMCHAR|_HEX,	_P|_OPER,		_P|_OPER,
 /* 3C */	_P|_OPER,		_P|_OPER,		_P|_OPER,		_P|_OPER,

 /* 40 */	_P|_NMCHAR|_OPER,	_DBL2|_P|_U|_NMSTART|_HEX,	_DBL2|_P|_U|_NMSTART|_HEX,	_DBL2|_P|_U|_NMSTART|_HEX,
 /* 44 */	_DBL2|_P|_U|_NMSTART|_HEX,	_DBL2|_P|_U|_NMSTART|_HEX,	_DBL2|_P|_U|_NMSTART|_HEX,	_DBL2|_P|_U|_NMSTART,
 /* 48 */	_DBL2|_P|_U|_NMSTART,		_DBL2|_P|_U|_NMSTART,		_DBL2|_P|_U|_NMSTART,		_DBL2|_P|_U|_NMSTART,
 /* 4C */	_DBL2|_P|_U|_NMSTART,		_DBL2|_P|_U|_NMSTART,		_DBL2|_P|_U|_NMSTART,		_DBL2|_P|_U|_NMSTART,
 /* 50 */	_DBL2|_P|_U|_NMSTART,		_DBL2|_P|_U|_NMSTART,		_DBL2|_P|_U|_NMSTART,		_DBL2|_P|_U|_NMSTART,
 /* 54 */	_DBL2|_P|_U|_NMSTART,		_DBL2|_P|_U|_NMSTART,		_DBL2|_P|_U|_NMSTART,		_DBL2|_P|_U|_NMSTART,
 /* 58 */	_DBL2|_P|_U|_NMSTART,		_DBL2|_P|_U|_NMSTART,		_DBL2|_P|_U|_NMSTART,		_P|_OPER,
 /* 5C */	_P|_OPER,		_P|_OPER,		_P|_OPER,		_P|_NMSTART,
 /* 60 */	_P|_OPER,		_DBL2|_P|_L|_NMSTART|_HEX,	_DBL2|_P|_L|_NMSTART|_HEX,	_DBL2|_P|_L|_NMSTART|_HEX,
 /* 64 */	_DBL2|_P|_L|_NMSTART|_HEX,	_DBL2|_P|_L|_NMSTART|_HEX,	_DBL2|_P|_L|_NMSTART|_HEX,	_DBL2|_P|_L|_NMSTART,
 /* 68 */	_DBL2|_P|_L|_NMSTART,		_DBL2|_P|_L|_NMSTART,		_DBL2|_P|_L|_NMSTART,		_DBL2|_P|_L|_NMSTART,
 /* 6C */	_DBL2|_P|_L|_NMSTART,		_DBL2|_P|_L|_NMSTART,		_DBL2|_P|_L|_NMSTART,		_DBL2|_P|_L|_NMSTART,
 /* 70 */	_DBL2|_P|_L|_NMSTART,		_DBL2|_P|_L|_NMSTART,		_DBL2|_P|_L|_NMSTART,		_DBL2|_P|_L|_NMSTART,
 /* 74 */	_DBL2|_P|_L|_NMSTART,		_DBL2|_P|_L|_NMSTART,		_DBL2|_P|_L|_NMSTART,		_DBL2|_P|_L|_NMSTART,
 /* 78 */	_DBL2|_P|_L|_NMSTART,		_DBL2|_P|_L|_NMSTART,		_DBL2|_P|_L|_NMSTART,		_P|_OPER,
 /* 7C */	_P|_OPER,		_P|_OPER,		_P|_OPER,		_C,
 
 /* 80 */	_C,			_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,			_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,			_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,
 /* 84 */	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,			_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,			_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,			_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,
 /* 8C */	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,			_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,			_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,			_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,
 /* 90 */	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,			_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,			_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,			_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,
 /* 94 */	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,			_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,			_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,			_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,
 /* 9C */	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,			_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,			_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,			_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,
 /* A0 */	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,			_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,
 /* A4 */	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,
 /* A8 */	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,
 /* AC */	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,
 /* B0 */	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,
 /* B4 */	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,
 /* B8 */	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,
 /* BC */	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,
 /* C0 */	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,
 /* C4 */	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,
 /* C8 */	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,
 /* CC */	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,
 /* D0 */	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,
 /* D4 */	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,
 /* D8 */	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,
 /* DC */	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,
 /* E0 */	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,
 /* E4 */	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,
 /* E8 */	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,
 /* EC */	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,
 /* F0 */	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,
 /* F4 */	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,
 /* F8 */	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,
 /* FC */	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_DBL1|_DBL2|_NMSTART|_NMCHAR|_K,	_C
 


# endif					/* Korean */
# endif 				/* CSIBM */
# endif					/* JIS Kanji             */
# endif					/* Burroughs PT terminal */
# endif					/* SUN Kanji             */
# endif                                 /* DEC Kanji             */
# endif					/* DEC Multinational     */
# endif					/* Standard ASCII        */

};


# ifdef CSBURROUGHS

/*
**	This shift table is used for lower casing the characters for
**	the BURROUGHS PT terminal.
*/

char CM_DefCaseTab[] =
{
	/*	0	1	2	3	4	5	6	7 */

/* 0x00 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 0x08 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 0x10 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 0x18 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 0x20 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 0x28 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 0x30 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 0x38 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 0x40 */	0,	'a',	'b',	'c',	'd',	'e',	'f',	'g',
/* 0x48 */	'h',	'i',	'j',	'k',	'l',	'm',	'n',	'o',
/* 0x50 */	'p',	'q',	'r',	's',	't',	'u',	'v',	'w',
/* 0x58 */	'x',	'y',	'z',	0,	0,	0,	0,	0,
/* 0x60 */	0,	'A',	'B',	'C',	'D',	'E',	'F',	'G',
/* 0x68 */	'H',	'I',	'J',	'K',	'L',	'M',	'N',	'O',
/* 0x70 */	'P',	'Q',	'R',	'S',	'T',	'U',	'V',	'W',
/* 0x78 */	'X',	'Y',	'Z',	0,	0,	0,	0,	0,
/* 0x80 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 0x88 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 0x90 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 0x98 */	0,	0,	0,	0,	0,	0,	0,	0,
		/* Effectively a NOP translation for Set 0 */
/* 0xA0 */	0xa0,	0xa1,	0xa2,	0xa3,	0xa4,	0xa5,	0xa6,	0xa7,
/* 0xA8 */	0xa8,	0xa9,	0xaa,	0xab,	0xac,	0,	0,	0,
/* 0xB0 */	0,	0,	0,	0,	0xb4,	0xb5,	0xb6,	0xb7,
		/* End Burroughs PT International Characters */
/* 0xB8 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 0xC0 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 0xC8 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 0xD0 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 0xD8 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 0xE0 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 0xE8 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 0xF0 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 0xF8 */	0,	0,	0,	0,	0,	0,	0,	0,
};

#else

# ifdef CSIBM

/*
**	This is based on the IBM PC style "Code Page 0" 8 bit
**	character set.
*/

char CM_DefCaseTab[] =
{
	/*	0	1	2	3	4	5	6	7 */
/* 0x00 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 0x08 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 0x10 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 0x18 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 0x20 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 0x28 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 0x30 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 0x38 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 0x40 */	0,	'a',	'b',	'c',	'd',	'e',	'f',	'g',
/* 0x48 */	'h',	'i',	'j',	'k',	'l',	'm',	'n',	'o',
/* 0x50 */	'p',	'q',	'r',	's',	't',	'u',	'v',	'w',
/* 0x58 */	'x',	'y',	'z',	0,	0,	0,	0,	0,
/* 0x60 */	0,	'A',	'B',	'C',	'D',	'E',	'F',	'G',
/* 0x68 */	'H',	'I',	'J',	'K',	'L',	'M',	'N',	'O',
/* 0x70 */	'P',	'Q',	'R',	'S',	'T',	'U',	'V',	'W',
/* 0x78 */	'X',	'Y',	'Z',	0,	0,	0,	0,	0,

/*		0	1	2	3	4	5	6	7
		8	9	A	B	C	D	E	F  */

/* 0x80 */	0x87,	0x9A,	0x90,	0xB6,	0x8E,	0xB7,	0x8F,	0x80,
/* 0x88 */	0xD2,	0xD3,	0xD4,	0xD8,	0xD7,	0xDE,	0x84,	0x86,
/* 0x90 */	0x82,	0x92,	0x91,	0xE2,	0x99,	0xE3,	0xEA,	0xEB,
/* 0x98 */	0,	0x94,	0x81,	0,	0,	0,	0,	0,
/* 0xA0 */	0xB5,	0xD6,	0xE0,	0xE9,	0xA5,	0xA4,	0,	0,
/* 0xA8 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 0xB0 */	0,	0,	0,	0,	0,	0xA0,	0x83,	0x85,
/* 0xB8 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 0xC0 */	0,	0,	0,	0,	0,	0,	0xC7,	0xC6,
/* 0xC8 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 0xD0 */	0,	0,	0x88,	0x89,	0x8A,	0,	0xA1,	0x8C,
/* 0xD8 */	0x8B,	0,	0,	0,	0,	0,	0x8D,	0,
/* 0xE0 */	0xA2,	0,	0x93,	0x95,	0xE5,	0xE4,	0,	0,
/* 0xE8 */	0,	0xA3,	0x96,	0x97,	0xED,	0xEC,	0,	0,
/* 0xF0 */	0,	0,	0,	0,	0,	0,	0,	0,
/* 0xF8 */	0,	0,	0,	0,	0,	0,	0,	0
};

# else

# ifdef CSDECKANJI

/*
** DEC Kanji
*/

GLOBALDEF char CM_DefCaseTab[] =
{
/*   0 */	'\0',	'\01',	'\02',	'\03',	'\04',	'\05',	'\06',	'\07',
/*  10 */	'\010',	'\011',	'\012',	'\013',	'\014',	'\015',	'\016',	'\017',
/*  20 */	'\020',	'\021',	'\022',	'\023',	'\024',	'\025',	'\026',	'\027',
/*  30 */	'\030',	'\031',	'\032',	'\033',	'\034',	'\035',	'\036',	'\037',
/*  40 */	' ',	'!',	'"',	'#',	'$',	'%',	'&',	'\'',
/*  50 */	'(',	')',	'*',	'+',	',',	'-',	'.',	'/',
/*  60 */	'0',	'1',	'2',	'3',	'4',	'5',	'6',	'7',
/*  70 */	'8',	'9',	':',	';',	'<',	'=',	'>',	'?',
/* 100 */	'@',	'a',	'b',	'c',	'd',	'e',	'f',	'g',
/* 110 */	'h',	'i',	'j',	'k',	'l',	'm',	'n',	'o',
/* 120 */	'p',	'q',	'r',	's',	't',	'u',	'v',	'w',
/* 130 */	'x',	'y',	'z',	'[',	'\\',	']',	'^',	'_',
/* 140 */	'`',	'A',	'B',	'C',	'D',	'E',	'F',	'G',
/* 150 */	'H',	'I',	'J',	'K',	'L',	'M',	'N',	'O',
/* 160 */	'P',	'Q',	'R',	'S',	'T',	'U',	'V',	'W',
/* 170 */	'X',	'Y',	'Z',	'{',	'|',	'}',	'~',	'\177',
/* 200 */	'\200',	'\201',	'\202',	'\203',	'\204',	'\205',	'\206',	'\207',
/* 210 */	'\210',	'\211',	'\212',	'\213',	'\214',	'\215',	'\216',	'\217',
/* 220 */	'\220',	'\221',	'\222',	'\223',	'\224',	'\225',	'\226',	'\227',
/* 230 */	'\230',	'\231',	'\232',	'\233',	'\234',	'\235',	'\236',	'\237',
/* 240 */	'\240',	'\241',	'\242',	'\243',	'\244',	'\245',	'\246',	'\247',
/* 250 */	'\250',	'\251',	'\252',	'\253',	'\254',	'\255',	'\256',	'\257',
/* 260 */	'\260',	'\261',	'\262',	'\263',	'\264',	'\265',	'\266',	'\267',
/* 270 */	'\270',	'\271',	'\272',	'\273',	'\274',	'\275',	'\276',	'\277',
/* 300 */       '\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
/* 310 */       '\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
/* 320 */       '\320', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
/* 330 */       '\370', '\371', '\372', '\373', '\374', '\375', '\336', '\337',
/* 340 */       '\300', '\301', '\302', '\303', '\304', '\305', '\306', '\307',
/* 350 */       '\310', '\311', '\312', '\313', '\314', '\315', '\316', '\327',
/* 360 */       '\360', '\321', '\322', '\323', '\324', '\325', '\326', '\367',
/* 370 */       '\330', '\331', '\332', '\333', '\334', '\335', '\376', '\377'
};

# else

# ifdef CSSUNKANJI

/*
** SUN Kanji
*/

char CM_DefCaseTab[] =
{
/*   0 */	'\0',	'\01',	'\02',	'\03',	'\04',	'\05',	'\06',	'\07',
/*  10 */	'\010',	'\011',	'\012',	'\013',	'\014',	'\015',	'\016',	'\017',
/*  20 */	'\020',	'\021',	'\022',	'\023',	'\024',	'\025',	'\026',	'\027',
/*  30 */	'\030',	'\031',	'\032',	'\033',	'\034',	'\035',	'\036',	'\037',
/*  40 */	' ',	'!',	'"',	'#',	'$',	'%',	'&',	'\'',
/*  50 */	'(',	')',	'*',	'+',	',',	'-',	'.',	'/',
/*  60 */	'0',	'1',	'2',	'3',	'4',	'5',	'6',	'7',
/*  70 */	'8',	'9',	':',	';',	'<',	'=',	'>',	'?',
/* 100 */	'@',	'a',	'b',	'c',	'd',	'e',	'f',	'g',
/* 110 */	'h',	'i',	'j',	'k',	'l',	'm',	'n',	'o',
/* 120 */	'p',	'q',	'r',	's',	't',	'u',	'v',	'w',
/* 130 */	'x',	'y',	'z',	'[',	'\\',	']',	'^',	'_',
/* 140 */	'`',	'A',	'B',	'C',	'D',	'E',	'F',	'G',
/* 150 */	'H',	'I',	'J',	'K',	'L',	'M',	'N',	'O',
/* 160 */	'P',	'Q',	'R',	'S',	'T',	'U',	'V',	'W',
/* 170 */	'X',	'Y',	'Z',	'{',	'|',	'}',	'~',	'\177',
/* 200 */	'\200',	'\201',	'\202',	'\203',	'\204',	'\205',	'\206',	'\207',
/* 210 */	'\210',	'\211',	'\212',	'\213',	'\214',	'\215',	'\216',	'\217',
/* 220 */	'\220',	'\221',	'\222',	'\223',	'\224',	'\225',	'\226',	'\227',
/* 230 */	'\230',	'\231',	'\232',	'\233',	'\234',	'\235',	'\236',	'\237',
/* 240 */	'\240',	'\241',	'\242',	'\243',	'\244',	'\245',	'\246',	'\247',
/* 250 */	'\250',	'\251',	'\252',	'\253',	'\254',	'\255',	'\256',	'\257',
/* 260 */	'\260',	'\261',	'\262',	'\263',	'\264',	'\265',	'\266',	'\267',
/* 270 */	'\270',	'\271',	'\272',	'\273',	'\274',	'\275',	'\276',	'\277',
/* 300 */       '\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
/* 310 */       '\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
/* 320 */       '\320', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
/* 330 */       '\370', '\371', '\372', '\373', '\374', '\375', '\336', '\337',
/* 340 */       '\300', '\301', '\302', '\303', '\304', '\305', '\306', '\307',
/* 350 */       '\310', '\311', '\312', '\313', '\314', '\315', '\316', '\327',
/* 360 */       '\360', '\321', '\322', '\323', '\324', '\325', '\326', '\367',
/* 370 */       '\330', '\331', '\332', '\333', '\334', '\335', '\376', '\377'
};

# else

# ifdef CSDECMULTI

/*
** DEC Multinational
*/

GLOBALDEF char CM_DefCaseTab[] =
{
/*   0 */	'\0',	'\01',	'\02',	'\03',	'\04',	'\05',	'\06',	'\07',
/*  10 */	'\010',	'\011',	'\012',	'\013',	'\014',	'\015',	'\016',	'\017',
/*  20 */	'\020',	'\021',	'\022',	'\023',	'\024',	'\025',	'\026',	'\027',
/*  30 */	'\030',	'\031',	'\032',	'\033',	'\034',	'\035',	'\036',	'\037',
/*  40 */	' ',	'!',	'"',	'#',	'$',	'%',	'&',	'\'',
/*  50 */	'(',	')',	'*',	'+',	',',	'-',	'.',	'/',
/*  60 */	'0',	'1',	'2',	'3',	'4',	'5',	'6',	'7',
/*  70 */	'8',	'9',	':',	';',	'<',	'=',	'>',	'?',
/* 100 */	'@',	'a',	'b',	'c',	'd',	'e',	'f',	'g',
/* 110 */	'h',	'i',	'j',	'k',	'l',	'm',	'n',	'o',
/* 120 */	'p',	'q',	'r',	's',	't',	'u',	'v',	'w',
/* 130 */	'x',	'y',	'z',	'[',	'\\',	']',	'^',	'_',
/* 140 */	'`',	'A',	'B',	'C',	'D',	'E',	'F',	'G',
/* 150 */	'H',	'I',	'J',	'K',	'L',	'M',	'N',	'O',
/* 160 */	'P',	'Q',	'R',	'S',	'T',	'U',	'V',	'W',
/* 170 */	'X',	'Y',	'Z',	'{',	'|',	'}',	'~',	'\177',
/* 200 */	'\200',	'\201',	'\202',	'\203',	'\204',	'\205',	'\206',	'\207',
/* 210 */	'\210',	'\211',	'\212',	'\213',	'\214',	'\215',	'\216',	'\217',
/* 220 */	'\220',	'\221',	'\222',	'\223',	'\224',	'\225',	'\226',	'\227',
/* 230 */	'\230',	'\231',	'\232',	'\233',	'\234',	'\235',	'\236',	'\237',
/* 240 */	'\240',	'\241',	'\242',	'\243',	'\244',	'\245',	'\246',	'\247',
/* 250 */	'\250',	'\251',	'\252',	'\253',	'\254',	'\255',	'\256',	'\257',
/* 260 */	'\260',	'\261',	'\262',	'\263',	'\264',	'\265',	'\266',	'\267',
/* 270 */	'\270',	'\271',	'\272',	'\273',	'\274',	'\275',	'\276',	'\277',
/* 300 */       '\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
/* 310 */       '\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
/* 320 */       '\320', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
/* 330 */       '\370', '\371', '\372', '\373', '\374', '\375', '\336', '\337',
/* 340 */       '\300', '\301', '\302', '\303', '\304', '\305', '\306', '\307',
/* 350 */       '\310', '\311', '\312', '\313', '\314', '\315', '\316', '\327',
/* 360 */       '\360', '\321', '\322', '\323', '\324', '\325', '\326', '\367',
/* 370 */       '\330', '\331', '\332', '\333', '\334', '\335', '\376', '\377'
};

# else

# ifdef CSKOREAN

/*
** Korean
*/

char CM_DefCaseTab[] =
{
/*   0 */	'\0',	'\01',	'\02',	'\03',	'\04',	'\05',	'\06',	'\07',
/*  10 */	'\010',	'\011',	'\012',	'\013',	'\014',	'\015',	'\016',	'\017',
/*  20 */	'\020',	'\021',	'\022',	'\023',	'\024',	'\025',	'\026',	'\027',
/*  30 */	'\030',	'\031',	'\032',	'\033',	'\034',	'\035',	'\036',	'\037',
/*  40 */	' ',	'!',	'"',	'#',	'$',	'%',	'&',	'\'',
/*  50 */	'(',	')',	'*',	'+',	',',	'-',	'.',	'/',
/*  60 */	'0',	'1',	'2',	'3',	'4',	'5',	'6',	'7',
/*  70 */	'8',	'9',	':',	';',	'<',	'=',	'>',	'?',
/* 100 */	'@',	'a',	'b',	'c',	'd',	'e',	'f',	'g',
/* 110 */	'h',	'i',	'j',	'k',	'l',	'm',	'n',	'o',
/* 120 */	'p',	'q',	'r',	's',	't',	'u',	'v',	'w',
/* 130 */	'x',	'y',	'z',	'[',	'\\',	']',	'^',	'_',
/* 140 */	'`',	'A',	'B',	'C',	'D',	'E',	'F',	'G',
/* 150 */	'H',	'I',	'J',	'K',	'L',	'M',	'N',	'O',
/* 160 */	'P',	'Q',	'R',	'S',	'T',	'U',	'V',	'W',
/* 170 */	'X',	'Y',	'Z',	'{',	'|',	'}',	'~',	'\177',
/* 200 */	'\200',	'\201',	'\202',	'\203',	'\204',	'\205',	'\206',	'\207',
/* 210 */	'\210',	'\211',	'\212',	'\213',	'\214',	'\215',	'\216',	'\217',
/* 220 */	'\220',	'\221',	'\222',	'\223',	'\224',	'\225',	'\226',	'\227',
/* 230 */	'\230',	'\231',	'\232',	'\233',	'\234',	'\235',	'\236',	'\237',
/* 240 */	'\240',	'\241',	'\242',	'\243',	'\244',	'\245',	'\246',	'\247',
/* 250 */	'\250',	'\251',	'\252',	'\253',	'\254',	'\255',	'\256',	'\257',
/* 260 */	'\260',	'\261',	'\262',	'\263',	'\264',	'\265',	'\266',	'\267',
/* 270 */	'\270',	'\271',	'\272',	'\273',	'\274',	'\275',	'\276',	'\277',
/* 300 */       '\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
/* 310 */       '\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
/* 320 */       '\320', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
/* 330 */       '\370', '\371', '\372', '\373', '\374', '\375', '\336', '\337',
/* 340 */       '\300', '\301', '\302', '\303', '\304', '\305', '\306', '\307',
/* 350 */       '\310', '\311', '\312', '\313', '\314', '\315', '\316', '\327',
/* 360 */       '\360', '\321', '\322', '\323', '\324', '\325', '\326', '\367',
/* 370 */       '\330', '\331', '\332', '\333', '\334', '\335', '\376', '\377'
};


# else


/*
** standard ascii (8859-1).
*/

char CM_DefCaseTab[] =
{
/*   0 */	'\0',	'\01',	'\02',	'\03',	'\04',	'\05',	'\06',	'\07',
/*  10 */	'\010',	'\011',	'\012',	'\013',	'\014',	'\015',	'\016',	'\017',
/*  20 */	'\020',	'\021',	'\022',	'\023',	'\024',	'\025',	'\026',	'\027',
/*  30 */	'\030',	'\031',	'\032',	'\033',	'\034',	'\035',	'\036',	'\037',
/*  40 */	' ',	'!',	'"',	'#',	'$',	'%',	'&',	'\'',
/*  50 */	'(',	')',	'*',	'+',	',',	'-',	'.',	'/',
/*  60 */	'0',	'1',	'2',	'3',	'4',	'5',	'6',	'7',
/*  70 */	'8',	'9',	':',	';',	'<',	'=',	'>',	'?',
/* 100 */	'@',	'a',	'b',	'c',	'd',	'e',	'f',	'g',
/* 110 */	'h',	'i',	'j',	'k',	'l',	'm',	'n',	'o',
/* 120 */	'p',	'q',	'r',	's',	't',	'u',	'v',	'w',
/* 130 */	'x',	'y',	'z',	'[',	'\\',	']',	'^',	'_',
/* 140 */	'`',	'A',	'B',	'C',	'D',	'E',	'F',	'G',
/* 150 */	'H',	'I',	'J',	'K',	'L',	'M',	'N',	'O',
/* 160 */	'P',	'Q',	'R',	'S',	'T',	'U',	'V',	'W',
/* 170 */	'X',	'Y',	'Z',	'{',	'|',	'}',	'~',	'\177',
/* 200 */	'\200',	'\201',	'\202',	'\203',	'\204',	'\205',	'\206',	'\207',
/* 210 */	'\210',	'\211',	'\212',	'\213',	'\214',	'\215',	'\216',	'\217',
/* 220 */	'\220',	'\221',	'\222',	'\223',	'\224',	'\225',	'\226',	'\227',
/* 230 */	'\230',	'\231',	'\232',	'\233',	'\234',	'\235',	'\236',	'\237',
/* 240 */	'\240',	'\241',	'\242',	'\243',	'\244',	'\245',	'\246',	'\247',
/* 250 */	'\250',	'\251',	'\252',	'\253',	'\254',	'\255',	'\256',	'\257',
/* 260 */	'\260',	'\261',	'\262',	'\263',	'\264',	'\265',	'\266',	'\267',
/* 270 */	'\270',	'\271',	'\272',	'\273',	'\274',	'\275',	'\276',	'\277',
/* 300 */       '\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
/* 310 */       '\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
/* 320 */       '\360', '\361', '\362', '\363', '\364', '\365', '\366', '\327',
/* 330 */       '\370', '\371', '\372', '\373', '\374', '\375', '\376', '\377',
/* 340 */       '\300', '\301', '\302', '\303', '\304', '\305', '\306', '\307',
/* 350 */       '\310', '\311', '\312', '\313', '\314', '\315', '\316', '\317',
/* 360 */       '\320', '\321', '\322', '\323', '\324', '\325', '\326', '\367',
/* 370 */       '\330', '\331', '\332', '\333', '\334', '\335', '\376', '\377'
};

# endif				/* Korean */
# endif				/* DEC Multinational     */
# endif                         /* SUN Kanji */
# endif                         /* DEC Kanji */
# endif				/* CSIBM */
# endif				/* CSBURROUGHS */



/*
** Bug 56258: identify 'compiled-in' default charset
** for some cases where II_CHARSETxx is not set
**
** These constants mirror the values in common!gcf!files!gcccset.nam
*/

#if defined(CSASCII)
GLOBALDEF const i4	CMdefcs=CM_88591;
#elif defined(CSDECMULTI)
GLOBALDEF const i4	CMdefcs=CM_DECMULTI;
#elif defined(CSIBM)
GLOBALDEF const i4	CMdefcs=CM_IBM;
#elif defined(CSKOREAN)
GLOBALDEF const i4	CMdefcs=CM_KOREAN;
#else
GLOBALDEF const i4	CMdefcs = 0;		/* just in case... */
#endif 

# ifdef VMS
globaldef {"iicl_gvar$readonly"} const i4 CM_BytesForUTF8[] =
# else		/* VMS */
GLOBALDEF const i4 CM_BytesForUTF8[] =
# endif		/* VMS */
{
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3, 4,4,4,4,4,4,4,4,1,1,1,1,1,1,1,1
};
