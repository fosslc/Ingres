/*
** Copyright (c) 1986, 2009 Ingres Corporation
*/

/**
** Name:    cm.h    - Character Manipulation library.
**
** Description:
**      The CM library routines allow programmers to deal
**	with characters independently as to whether the 
**	character is one or two bytes.  This file defines:
**
**(	CM Routines for Checking Character Attributes
**	---------------------------------------------
**	CMalpha		test for alphabetic character
**	CMdbl1st	test for 1st byte of double byte
**	CMnmstart	test for leading character in name
**	CMnmchar	test for trailing character in name
**	CMprint		test for printing character
**	CMdigit		test for digit
**	CMcntrl		test for cntrol character
**	CMlower		test for lowercase character
**	CMupper		test for uppercase character
**	CMwhite		test for white space
**	CMspace		test for space character
**	CMhex		test for hexadecimal digit
**	CMoper		test for operator character
**	CM_ISUTF8_BOM	test for byte order mark in UTF8 install. 
**
**      CMhost          test for hostname character (NT ONLY)
**      CMuser          test for username character (NT ONLY)
**      CMpath          test for path character     (NT ONLY)
**		NOTE: These test macros are notionally typed
**		to return bool but will do so using the relaxed
**		boolean form of 0 being FALSE and TRUE otherwise.
**		Thus, checking whether a character is a digit
**		should be done using either:
**			if (CMdigit(ptr)) ...
**		or:
**			if (CMdigit(ptr) != FALSE) ...
**		but never as:
**			if (CMdigit(ptr) == TRUE) ...
**
**
**	CM Routines for String Movement
**	-------------------------------
**	CMnext		increment character pointer
**	CMprev		decrement character pointer
**	CMbyteinc	increment byte counter
**	CMbytedec	decrement byte counter
**	CMbytecnt	count the bytes in the next character
**	
**	CM Routines for Copying Characters
**	----------------------------------
**	CMcpychar	copy character to string
**	CMcpyinc	copy character to string and increment
**	CMtolower	copy lowercase character to string
**	CMtoupper	copy uppercase character to string
**	CMcopy		copy a character string of specified length
**
**	CM Routines for Comparing Characters
**	------------------------------------
**	CMcmpnocase	compare two characters, ignoring case
**	CMcmpcase	compare two characters for exact match
**)
** History:
**	18-sep-86 (yamamoto)
**		first written
**	23-feb-87 (yamamoto)
**		Add CMhex and CMoper
**	02-mar-87 (yamamoto)
**		Added CMbytecnt
**	09-mar-87 (peter)
**		Moved from VMS --> UNIX.
**	13-apr-87 (ncg)
**		Added CMcopy (with _cmcopy).
**	11-may-88 (ricks)
**		Added character set defines - CSxxx
**	29-nov-88 (nancy) -- fixed cmdblspace macro to use correct Kanji
**		space value (A1) in both bytes.
**	 1-Jun-89 (anton)
**		Local collation support
**	 7-Jun-89 (anton)
**		Moving definition of BELL to cm.h from compat.h
**	20-Jun-89 (anton)
**		Add COL_BLOCK - block size of local collation file
**	22-may-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Added IBM's Code Page 0 8 bit character set;
**		Added macro cmtolower and cmtoupper for CSIBM;
**		Moved the #define of character set to bzarch.h, and add error
**		if compat.h (and therefore bzarch.h) hasn't been included;
**		Must declare extern CMshift[] if CSIBM is defined;
**		Redeclare ccm_Character_Table as a GLOBALCONSTREF to match
**		cl/clf/cmtypes.roc;
**		Re-map CMcopy into MECOPY_VAR_MACRO call.
**	23-Aug-90 (bobm)
**		Changes to allow runtime loading of attribute table.
**	24-Aug-90 (jkb)
**		Backed out change " Re-map CMcopy into MECOPY_VAR_MACRO
**		call" can't have switch statement in a comma separated list
**	 1-Mar-91 (stevet)
**              Changed CMcopy definition for DOUBLEBYTE to cmicopy, which was
**              lost with no appearent reason.  Fixed DOUBLEBYTE definitions
**              of CMcmpnocase and CMcmpcase for scrollable field problems.
**	09-apr-91 (seng)
**		Add 1 to CSIBM versions of cmtoupper() and cmtolower()
**		macros.  IBM has an additional char in array.
**	24-apr-91 (seng)
**		Back out change for CSIBM versions of cmtoupper() and
**		cmtolower().  Adding 1 to the array may not be compatible
**		future implementations of user loadable character sets
**		in INGRES.
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**  	10-apr-1995(angusm)
**          Add extra constants to identify the default 'compiled-in'
**          charsets in cmafile.roc
**      12-Dec-95 (fanra01)
**          Added definitions for extracting data for DLLs on windows NT and
**          using CM character tables from code built for static libraries.
**      21-Jan-2000 (fanra01)
**          Bug 100233
**          Add NT_GENERIC prototypes for CMgetAttrTab and CMgetCaseTab.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      25-jun-2002 (fanra01)
**          Sir 108122
**          Add NT ONLY attributes and macros for hosts, users and paths.
**	28-feb-2003 (penga03)
**	    Added a prototype for cmicpychar and modified CMcpychar.
**	21-apr-2003 (penga03)
**	    Removed the change added by change number 462295 to fix bug 109750.
**	14-jan-2004 (gupsh01)
**	    Added location type for specifying output directory in CM routines.
**	23-jan-2004 (stephenb)
**	    fix Double byte CMcpychar macro with void cast so that it builds
**	    under the latest Solaris compiler.
**	18-feb-2004 (gupsh01)
**	    Added CM_DEFAULTFILE_LOC which allows looking for default unicode 
**	    coercion mapping file in $II_SYSTEM/ingres/files directory which
**	    is not yet copied to $II_SYSTEM/ingres/files/ucharmaps directory,
**	    at install time.
**	14-Jun-2004 (schka24)
**	    Added max locale name length for overrun safety.
**	17-Jun-2004 (drivi01)
**	    Added CMwcslen, CMwcschr, CMwcsncpy functions for processing 
**          wchar_t strings.
**	23-Sep-2005 (hanje04)
**	    Cast args to cmkcheck() in CMprev to queit compiler warnings.
**	20-Oct-2006 (kiria01)
**	    Increased the checking for doublebyte characters such as dblspace
**	    and factored out the conditional code to aid readability and
**	    reduce repetition.
**	06-Nov-2006 (kiria01) b117042
**	    Document the relaxed bool implementation of the CM macros following
**	    the more efficient standard of 0 being false and non-zero being truth.
**	11-Apr-2007 (gupsh01)
**	    Added support for UTF8 character sets.
**	14-Apr-2007 (gupsh01)
**	    Removed cmucpychar and cmucpyinc macros.
**	20-May-2007 (gupsh01)
**	    Added case and property translation support for UTF8 character sets.
**	19-Nov-2007 (gupsh01)
**	    Modify the definition of CM_UTF8LEAD and CM_UTF8FOLLOW.
**      05-feb-2009 (joea)
**          Change CM_isUTF8 to a bool.  Change Windows CMGETUTF8 macro to
**          use CMischarset_utf8.
**       5-Nov-2009 (hanal04) Bug 122807
**          CM_CaseTab needs to be u_char or we get values wrapping and
**          failing integer equality tests.
**	 2-Dec-2009 (wanfr01) Bug 122994
**	    Add CMGETDBL variable and use it to bypass Doublebyte checks if
**	    you are running a platform that has no doublebyte characters
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Create single byte versions of the CM macros
*/

/*
** Bell character
*/
# define	BELL '\07'

/*
**	Local collation block size
*/
# define	COL_BLOCK	1024

/*
**	File output location types for CM*_col routines.
*/
# define	CM_COLLATION_LOC	0
# define	CM_UCHARMAPS_LOC	1
# define	CM_DEFAULTFILE_LOC	2

/* 
**	The character set being used is defined in bzarch.h, so error
**	if compat.h, and therefore bzarch.h, has not been included.
*/

# ifndef        COMPAT_INCLUDE
       # error "<compat.h> not included before <cm.h>"
# endif

/*
**	Definitions of character classification.
*/

# define	CM_A_UPPER	1	/* Upper case ALPHA */
# define	CM_A_LOWER	2	/* Lower case ALPHA */
# define	CM_A_NCALPHA	4	/* non-cased alpha */
# define	CM_A_DIGIT	8	/* DIGIT */
# define	CM_A_SPACE	0x10	/* SPACE */
# define	CM_A_PRINT	0x20	/* PRINTABLE */
# define	CM_A_CONTROL	0x40	/* CONTROL */
# define	CM_A_DBL1	0x80	/* 1st byte of double byte character */
# define	CM_A_DBL2	0x100	/* 2nd byte of double byte character */
# define	CM_A_NMSTART	0x200	/* Leading character of a name */
# define	CM_A_NMCHAR	0x400	/* Trailing character of a name */
# define	CM_A_HEX	0x800	/* Hexadecimal Digit */
# define	CM_A_OPER	0x1000	/* Operator Character */

# if defined(NT_GENERIC)
# define        CM_A_HOST       0x2000  /* hostname character */
# define        CM_A_PATH       0x4000  /* path character */
# define        CM_A_PATHCTRL   0x0004  /* path control character */
# define        CM_A_USER       0x8000  /* username character */
# endif /* NT_GENERIC */

# define	CM_A_ALPHA	(CM_A_UPPER|CM_A_LOWER|CM_A_NCALPHA)

# define 	CM_A_ILLEGALUNI	0x0000  /* Defined for UTF8 charset */

/*
** Bug 56258: identify 'compiled-in' default charset
** for some cases where II_CHARSETxx is not set
**
** These constants mirror the values in common!gcf!files!gcccset.nam
*/
 
#define CM_88591		0x1		/* ISO 8859-1 */
#define CM_DECMULTI		0x30		/* DEC MULTI */
#define CM_IBM			0x14		/* IBM code page 850 */

/*
** maximum length for name identifying character set.
*/
# define	CM_MAXATTRNAME	8

# define	CM_NOCHARSET	(E_CL_MASK + E_CM_MASK + 0x01)


/* Maximum length we'll accept for an OS supplied locale / codepage name.
** The aduconverter stuff calls this a platform character-set name.
*/

# define	CM_MAXLOCALE	50

/*
** CM_DEREF - dereference pointer and apply 8 bit mask
*/
# define	CM_DEREF(s)	(*(s)&0xFF)

/*
** CM_UTF8MULTI - Checks if this is a multibyte UTF8 character.
*/
# define        CM_UTF8MULTI(s)   ((*(s)&0xFF) & 0x80)

/*
** CM_UTF8LEAD - Checks whether a lead byte of a UTF8 character
** Follows a pattern eg 0x11...
*/
# define        CM_UTF8LEAD(s)    (((*(s)&0xFF) & 0x80) && ((*(s)&0xFF) & 0x40))

/*
** CM_UTF8FOLLOW - Checks whether a following byte of a UTF8 character.
** Follows a pattern eg 0x10...
*/
# define        CM_UTF8FOLLOW(s)  (((*(s)&0xFF) & 0x80) && !(((*(s)&0xFF) & 0x40)))

/* 
** Name: CM_UTF8ATTR - UTF8 property definition structures 
**
** Description:
**
**	Structure which defines the attribute and case translation tables
**	for UTF8 character sets. This character set is larger then usual
**	0-FF range and has special structures to store the values.
*/
typedef struct
{
	u_i2	property;
	u_i2	caseindex;
} CM_UTF8ATTR;

#define 	MAXCASECHARS	    8	/* Max case translation characters */	

/* 
** Name: CM_UTF8CASE - UTF8 case definition structures
**
** Description:
**
**	Structure which defines the attribute and case translation tables
*/
typedef struct
{
	u_i2	codepoint;
	u_i2	casecnt;
	u_char	casechars[MAXCASECHARS];

} CM_UTF8CASE;

/*
** attribute table and case translation table
*/
# if defined(NT_GENERIC) && defined(IMPORT_DLL_DATA)
/* Attr & Case tables will be accessed via a function returning the table base */
# define CMGETATTRTAB	((u_i2 *)CMgetAttrTab())
# define CMGETCASETAB	((char *)CMgetCaseTab())
# define CMGETUTF8	(CMischarset_utf8())
# define CMGETDBL	(CMischarset_doublebyte())
# define CMGETUTF8BYTES ((i4 *)CMgetUTF8Bytes())
# define CMGETUTF8ATTRTAB	((CM_UTF8ATTR *)CMgetUTF8AttrTab())
# define CMGETUTF8CASETAB	((CM_UTF8CASE *)CMgetUTF8CaseTab())
GLOBALDLLREF	u_i2	*CM_AttrTab;
GLOBALDLLREF	u_char	*CM_CaseTab;
GLOBALDLLREF	bool	CM_isUTF8;
GLOBALDLLREF	bool	CM_isDBL;
GLOBALDLLREF	i4 	*CM_UTF8Bytes;
GLOBALDLLREF	CM_UTF8ATTR	*CM_UTF8AttrTab;
GLOBALDLLREF	CM_UTF8CASE	*CM_UTF8CaseTab;

# else
/* Attr & Case tables will be accessed via direct pointers */
# define CMGETATTRTAB	CM_AttrTab
# define CMGETCASETAB	CM_CaseTab
# define CMGETUTF8	CM_isUTF8
# define CMGETDBL	CM_isDBL
# define CMGETUTF8BYTES	CM_UTF8Bytes
# define CMGETUTF8ATTRTAB	CM_UTF8AttrTab	
# define CMGETUTF8CASETAB	CM_UTF8CaseTab	
GLOBALREF	u_i2	*CM_AttrTab;
GLOBALREF	u_char	*CM_CaseTab;
GLOBALREF	bool	CM_isUTF8;
GLOBALREF	bool	CM_isDBL;
GLOBALREF	i4	*CM_UTF8Bytes;
GLOBALREF	CM_UTF8ATTR	*CM_UTF8AttrTab;
GLOBALREF	CM_UTF8CASE	*CM_UTF8CaseTab;

# endif

FUNC_EXTERN u_i2        cmkcheck(
#ifdef  CL_PROTOTYPED
        u_char          *point,
        u_char          *strstart
#endif
);
FUNC_EXTERN u_i4       cmicopy(
#ifdef  CL_PROTOTYPED
        u_char          *source,
        u_i4            len,
        u_char          *dest
#endif
);

FUNC_EXTERN i4		cmupct(
#ifdef CL_PROTOTYPED
	u_char		*str, 
	u_char		*startpos
#endif
);

FUNC_EXTERN u_i2 	cmu_getutf8property(
#ifdef CL_PROTOTYPED
	u_char 	*key, 
	i4 	keycnt
#endif
);

FUNC_EXTERN u_i2 	cmu_getutf8_toupper(
#ifdef CL_PROTOTYPED
	u_char	*src, 
	i4	srclen, 
	u_char	*dst, 
	i4	dstlen
#endif
);

FUNC_EXTERN u_i2 	cmu_getutf8_tolower(
#ifdef CL_PROTOTYPED
	u_char	*src, 
	i4	srclen, 
	u_char	*dst, 
	i4	dstlen
#endif
);

# if defined(NT_GENERIC)
FUNC_EXTERN u_i2* CMgetAttrTab(
#ifdef  CL_PROTOTYPED
    void
#endif
);

FUNC_EXTERN u_char* CMgetCaseTab(
#ifdef  CL_PROTOTYPED
    void
#endif
);

FUNC_EXTERN CM_UTF8ATTR* CMgetUTF8AttrTab(
#ifdef  CL_PROTOTYPED
    void
#endif
);

FUNC_EXTERN CM_UTF8CASE* CMgetUTF8CaseTab(
#ifdef  CL_PROTOTYPED
    void
#endif
);

FUNC_EXTERN char CMgetUTF8(
#ifdef  CL_PROTOTYPED
    void
#endif
);

FUNC_EXTERN i4 *CMgetUTF8Bytes(
#ifdef  CL_PROTOTYPED
    void
#endif
);
# endif

# define CMGETATTR(s)	((CMGETUTF8 && CM_UTF8MULTI(s)) ? (cmu_getutf8property((u_char *)(s), (i4)CMUTF8cnt(s))) : (CMGETATTRTAB[CM_DEREF(s)])) 
# define CMGETATTR_SB(s)	(CMGETATTRTAB[CM_DEREF(s)]) 

# define CMGETCASE(s)	CMGETCASETAB[CM_DEREF(s)]

# define CMUTF8cnt(str)	(CM_UTF8Bytes[CM_DEREF((u_char *)(str))])

/*	define local macros	*/
/* For UTF8 character set the trailing bytes have their 2 high bits set as 
** 10 hence the bytes are in the form of 10xxxxxx 
*/

# define cmdbl2nd(s)	((CMGETUTF8 && CM_UTF8MULTI(s)) ? (CM_UTF8FOLLOW(s)) : (CMGETATTR(s) & CM_A_DBL2))

/*
** NOTE: The checks made for kanjii doublebyte space below do not regard
** buffer length! Most of the CM* macros below access only the character
** pointed at meaning that the calling code must assure that the character
** is within the buffer. However, where spaces have to be checked for, the
** space character could be a 2 byte long space and the check would have
** to look beyond the assured character. The macro cmdblspace(s) provides
** the means of checking for it but is unaware of buffer end!. To tighten
** up this deficiency, short of adding length parameters to most CM* macros
** we check the character attributes to see if the 0xA1 character is marked
** as being both an initial and a following character in the two byte set.
** This is reasonable check to make as it would otherwise not be stepped
** over correctly by CMnext etc. This does mean that an ill-formed character
** in a double byte cset could be reported as spanning the buffer end if
** a 0xA1 character happend to be present beyond buffer end.
*/
#ifdef	DOUBLEBYTE
# define KANJI_SPC 0xA1
# define cmdblspace(s)	(((CM_DEREF(s)) == KANJI_SPC) && \
	((CMGETATTR(s) & (CM_A_SPACE|CM_A_DBL1|CM_A_DBL2))==(CM_A_SPACE|CM_A_DBL1|CM_A_DBL2)) &&\
	((CM_DEREF((s)+1)) == KANJI_SPC))
#else
/* Allow optimizer to cut out redundant DOUBLEBYTE code */
# define cmdblspace(s)	(FALSE)
#endif

/*{
** Name:	CMalpha	- test for alphabetic character
**
** Description:
**	Test next character in string for alphabetic.
**
**	This routine, which is system-independent MACRO, requires 
**	the user to include the global header CM.h.  
**
**(Usage:
**	bool	CMalpha(str)
**	char	*str;
**)
** Inputs:
**	str	pointer to character to be checked
**
** Outputs:
**	Returns:
**		0	Char not alphabetic
**		!=0	Char is alphabetic
**
** Side Effects:
**	None
**
** History:
**	18-sep-86 (yamamoto)
**		first written
*/

# define CMalpha(str)	(CMGETATTR(str) & CM_A_ALPHA)


/*{
** Name:	CMdbl1st	- test for 1st byte of double byte character
**
** Description:
**	Test next character in string for leading byte of a two byte character
**
**	This routine, which is system-independent MACRO, requires 
**	the user to include the global header CM.h.  
**
**	Note this routine doesn't need the (CMGETDBL?) check, because it has
**	already been checked by the caller macro
**
**(Usage:
**	bool	CMdbl1st(str)
**	char	*str;
**)
** Inputs:
**	str	pointer to character to be checked
**
** Outputs:
**	Returns:
**		0	Char not 2 byte leader
**		!=0	Char is 2 byte leader
**
** Side Effects:
**	None
**
** History:
**	18-sep-86 (yamamoto)
**		first written
**	22-may-2007 (gupsh01)
**		support UTF8 character set where lead bytes have 
**		their 2 high bits set as 11 hence the bytes 
**		are in the form of 11xxxxxx. 
*/

# ifdef	DOUBLEBYTE
# define CMdbl1st(str)	((CMGETUTF8 && CM_UTF8MULTI(str)) ? (CM_UTF8LEAD(str)) : (CMGETATTR(str) & CM_A_DBL1))
# else
/* Allow optimizer to cut out redundant DOUBLEBYTE code */
# define CMdbl1st(str)	(FALSE)
# endif
# define CMdbl1st_SB(str)	(FALSE)



/*{
** Name:	CMnmstart	- check leading character of a name
**
** Description:
**	Within a string, check the next character to see if
**	it is a valid character as the first position of an
**	INGRES name. 
**
**	This routine, which is system-independent MACRO, requires 
**	the user to include the global header CM.h.  
**
**(Usage:
**	bool	CMnmstart(nm)
**	char	*nm;
**)
** Inputs:
**	nm	pointer to character within a name
**
** Outputs:
**	Returns:
**		0	Char not name leader
**		!=0	Char is name leader
**
** Side Effects:
**	None
**
** History:
**	18-sep-86 (yamamoto)
**		first written
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Optimize code for single byte character sets
*/

# define CMnmstart(nm)	((CMGETDBL)? ((CMGETATTR(nm) & CM_A_NMSTART) ? \
	(CMdbl1st(nm) ? (cmdbl2nd((nm)+1) && !cmdblspace(nm)) : TRUE) : FALSE) : \
	(CMGETATTR_SB(nm) & CM_A_NMSTART))
# define CMnmstart_SB(nm)	(CMGETATTR_SB(nm) & CM_A_NMSTART)


/*{
** Name:	CMnmchar	- check trailing character of a name
**
** Description:
**	Within a string, check the next character to see if
**	it is a valid character within an INGRES name. 
**
**	This routine, which is system-independent MACRO, requires 
**	the user to include the global header CM.h.  
**
**(Usage:
**	bool	CMnmchar(nm)
**	char	*nm;
**)
** Inputs:
**	nm	pointer to character within a name
**
** Outputs:
**	Returns:
**		0	Char not name character
**		!=0	Char is name character
**
** Side Effects:
**	None
**
** History:
**	18-sep-86 (yamamoto)
**		first written
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Optimize code for single byte character sets
*/

# define CMnmchar(nm)	((CMGETDBL)? ((CMGETATTR(nm) & (CM_A_NMSTART|CM_A_NMCHAR)) ? \
	(CMdbl1st(nm) ? (cmdbl2nd((nm)+1) && !cmdblspace(nm)) : TRUE) : FALSE) : \
	(CMGETATTR_SB(nm) & (CM_A_NMSTART|CM_A_NMCHAR)))
# define CMnmchar_SB(nm)	(CMGETATTR_SB(nm) & (CM_A_NMSTART|CM_A_NMCHAR))


/*{
** Name:	CMprint	- test for printing character
**
** Description:
**	Test next character in string for printing character
**
**	This routine, which is system-independent MACRO, requires 
**	the user to include the global header CM.h.  
**
**(Usage:
**	bool	CMprint(str)
**	char	*str;
**)
** Inputs:
**	str	pointer to character to be checked
**
** Outputs:
**	Returns:
**		0	Char not printable
**		!=0	Char is printable
**
** Side Effects:
**	None
**
** History:
**	18-sep-86 (yamamoto)
**		first written
**	03-Dec-2009 (wanfr01) Bug 122994
**	    Optimize code for single byte character sets
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Further Optimize code for single byte character sets
*/

# define CMprint(str)	((CMGETDBL)? (CMdbl1st(str) ? cmdbl2nd((str)+1) : \
	(CMGETATTR(str) & (CM_A_PRINT|CM_A_ALPHA|CM_A_DIGIT))) : \
	(CMGETATTR_SB(str) & (CM_A_PRINT|CM_A_ALPHA|CM_A_DIGIT))) 
# define CMprint_SB(str)	(CMGETATTR_SB(str) & (CM_A_PRINT|CM_A_ALPHA|CM_A_DIGIT)) 



/*{
** Name:	CMdigit	- test for digit
**
** Description:
** 	Test next character in string for digit
**
**	This routine, which is system-independent MACRO, requires 
**	the user to include the global header CM.h.  
**
**(Usage:
**	bool	CMdigit(str)
**	char	*str;
**)
** Inputs:
**	str	pointer to character to be checked
**
** Outputs:
**	Returns:
**		0	Char not digit
**		!=0	Char is digit
**
** Side Effects:
**	None
**
** History:
**	18-sep-86 (yamamoto)
**		first written
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Create single byte version
*/

# define CMdigit(str)	(CMGETATTR(str) & CM_A_DIGIT)
# define CMdigit_SB(str)	(CMGETATTR_SB(str) & CM_A_DIGIT)


/*{
** Name:	CMcntrl	- test for non-printing character
**
** Description:
**	Test next character in string for non-printing character
**
**	This routine, which is system-independent MACRO, requires 
**	the user to include the global header CM.h.  
**
**(Usage:
**	bool	CMcntrl(str)
**	char	*str;
**)
** Inputs:
**	str	pointer to character to be checked
**
** Outputs:
**	Returns:
**		0	Char not control character
**		!=0	Char is control character
**
** Side Effects:
**	None
**
** History:
**	18-sep-86 (yamamoto)
**		first written
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Create single byte version
*/

# define CMcntrl(str)	(CMGETATTR(str) & CM_A_CONTROL)
# define CMcntrl_SB(str)	(CMGETATTR_SB(str) & CM_A_CONTROL)


/*{
** Name:	CMlower	- test for lowercase
**
** Description:
**	Test next character in string for lowercase
**	Is *str in [a-z]
**
**	This routine, which is system-independent MACRO, requires 
**	the user to include the global header CM.h.  
**
**(Usage:
**	bool	CMlower(str)
**	char	*str;
**)
** Inputs:
**	str	pointer to character to be checked
**
** Outputs:
**	Returns:
**		0	Char not lowercase
**		!=0	Char is lowercase
**
** Side Effects:
**	None
**
** History:
**	18-sep-86 (yamamoto)
**		first written
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Create single byte version
*/

# define CMlower(str)	(CMGETATTR(str) & CM_A_LOWER)
# define CMlower_SB(str)	(CMGETATTR_SB(str) & CM_A_LOWER)

/*	define local macros	*/

# define cmtoupper(str)	(CMlower(str) ? CMGETCASE(str) : CM_DEREF(str))
# define cmtoupper_sb(str)	(CMlower_SB(str) ? CMGETCASE(str) : CM_DEREF(str))


/*{
** Name:	CMupper	- test for uppercase
**
** Description:
**	Test next character in string for uppercase
**	Is *str in [A-Z]
**
**	This routine, which is system-independent MACRO, requires 
**	the user to include the global header CM.h.  
**
**(Usage:
**	bool	CMupper(str)
**	char	*str;
**)
** Inputs:
**	str	pointer to character to be checked
**
** Outputs:
**	Returns:
**		0	Char not uppercase
**		!=0	Char is uppercase
**
** Side Effects:
**	None
**
** History:
**	18-sep-86 (yamamoto)
**		first written
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Create single byte version
*/

# define CMupper(str)	(CMGETATTR(str) & CM_A_UPPER)
# define CMupper_SB(str)	(CMGETATTR_SB(str) & CM_A_UPPER)

/*	define local macros	*/

# define cmtolower(str)	(CMupper(str) ? CMGETCASE(str) : CM_DEREF(str))
# define cmtolower_sb(str)	(CMupper_SB(str) ? CMGETCASE(str) : CM_DEREF(str))

/*{
** Name:	CMwhite	- check white space
**
** Description:
**	Within a string, check to see if the next character
**	is white space (either a space, tab, LF, NL, CR, FF 
**	or double byte space.) 
**
**	This routine, which is system-independent MACRO, requires 
**	the user to include the global header CM.h.  
**
**(Usage:
**	bool	CMwhite(str)
**	char	*str;
**)
** Inputs:
**	str	pointer positioned at the point in the string
**		to check a character for white space.	
**
** Outputs:
**	Returns:
**		0	Char not whitespace
**		!=0	Char is whitespace
**
** Side Effects:
**	None
**
** History:
**	18-sep-86 (yamamoto)
**		first written
**	23-Oct-06 (kiria01)
**		Tighten double byte space check and bias the evaluation to
**		more efficiently identify the single byte whitespace
**	03-Dec-2009 (wanfr01) Bug 122994
**	    Optimize code for single byte character sets
**	    Note this undoes the prior change so the code is easier to read.
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Create single byte version
*/

# define CMwhite(str) ((CMGETDBL)? ((CMGETUTF8 && CM_UTF8MULTI(str)) ? ((CMGETATTR(str)&CM_A_SPACE) ? TRUE : FALSE) : ((CM_AttrTab[*(str)&0377] & CM_A_SPACE) || cmdblspace(str))) : (CM_AttrTab[*(str)&0377] & CM_A_SPACE))
# define CMwhite_SB(str) (CM_AttrTab[*(str)&0377] & CM_A_SPACE)

/*{
** Name:	CMspace	- check a space or double byte space
**
** Description:
**	Within a string, check to see if the next character
**	is a space or double byte space.
**
**	This routine, which is system-independent MACRO, requires 
**	the user to include the global header CM.h.  
**
**(Usage:
**	bool 	CMspace(str) 
**	char	*str;
**)
** Inputs:
**	str	pointer positioned at the point in the string
**		to check a character for space.
**
** Outputs:
**	Returns:
**		0	Char not space character
**		!=0	Char is space character
**
** Side Effects:
**	None
**
** History:
**	18-sep-86 (yamamoto)
**		first written
**	03-Dec-2009 (wanfr01) Bug 122994
**	    Optimize code for single byte character sets
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Create single byte version
*/

# define CMspace(str)	(*(str) == ' ' || ((CMGETDBL) && cmdblspace(str)))
# define CMspace_SB(str)	(*(str) == ' ')


/*{
** Name:	CMhex() -	Test for Hexadecimal Digit.
**
** Description:
**	Checks that the current character of the input string is a hexadecimal
**	digit.  That is, one of [0-9A-Fa-f].
**
**	This routine, which is system-independent MACRO, requires 
**	the user to include the global header CM.h.  
**
**(Usage:
**	bool CMhex(str)
**	char *str;
**)
** Inputs:
**	str	{char *}  Pointer to character within a string.
**
** Outputs:
**	Returns:
**		0	Char not hex digit
**		!=0	Char is hex digit
**
** Side Effects:
**	None
**
** History:
**	02/87 (jhw) -- Written.
**	23-feb-1987 (yamamoto)
**		written using _HEX.
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Create single byte version
*/

# define CMhex(str)	(CMGETATTR(str) & CM_A_HEX)
# define CMhex_SB(str)	(CMGETATTR_SB(str) & CM_A_HEX)


/*{
**  Name: CMoper - Test for operator character
**
**  Description:
**	Test next character in a string for membership in the set of operators.
**	This set of operators constitutes the set of all INGRES operators
**	that are used throught the query languages and their various
**	extensions.  These operators also include all the operators of
**	host languages in which a query language may be embedded.
**	The set of operators is made up of all printable characters, less
**	the set of alphanumeric and space characters.  This set of operators
**	may change across different character sets (ie, the EBCDIC value for
**	^), and languages (ie, the Spanish question prefix, the upside down '?')
**
** 	For example, the set of ASCII (American) operators is:
**
**	    ! " # $ % & ' ( ) * + , - . / : ; < = > ? @ [ \ ] ^ `
**	    { | } ~
**
**(Usage:
**	bool CMoper(str)
**	char *str;
**)
**
**  Inputs:
**	str	- Pointer to character to be checked.
**
**  Outputs:
**	Returns:
**		0	Char not oper character
**		!=0	Char is oper character
**	Errors:
**	    None
**
**  Side Effects:
**	None
**	
**  History:
**	10-feb-1987 (neil)	- written.
**	23-feb-1987 (yamamoto)
**		written using _OPER.
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Create single byte version
*/

# define CMoper(str)	(CMGETATTR(str) & CM_A_OPER)
# define CMoper_SB(str)	(CMGETATTR_SB(str) & CM_A_OPER)


/*{
** Name: 	CM_ISUTF8_BOM - Checks if the given source string is a UTF-8 
**			    byte order mark.
**
** Input:
**	src 		String to test for UTF8 byte order mark. 
**
** History:
**	16-Feb-2009 (gupsh01)
**	    Written
*/
# define CM_ISUTF8_BOM(src) ((CMGETUTF8) && ((CMbytecnt(src) == 3) && ((*((u_char *)src) == 0xEF) && (*((u_char *)src + 1) == 0xBB) && (*((u_char *)src + 2) == 0xBF)) ))


/*{
** Name:	CMcpychar	- copy one character to another
**
** Description:
**	Copy one character (either one or two bytes) from
**	string 'src' to the current position in string 'dst'.
**	This mimics the (*d = *s) expression of C.
**
**	This routine, which is system-independent MACRO, requires 
**	the user to include the global header CM.h.  
**
**(Usage:
**	VOID	CMcpychar(src, dst)
**	char	*src;
**	char	*dst;
**)
** Inputs:
**	src	pointer to character, which is positioned at the 
**		point in string from which character is to be copied.
**	dst	pointer in string into which character is to be copied.
**
** Outputs:
**	Returns:
**		VOID
**
** Side Effects:
**	None
**
** History:
**	18-sep-86 (yamamoto)
**		first written
**	28-feb-2003 (penga03)
**	    Modified the CMcpychar to consider following four situations 
**	    if DOUBLEBYTE is defined:
**	    case 1: source is double byte character, destination is double 
**	            byte character
**	    case 2: source is double byte character, destination is single 
**	            byte character
**	    case 3: source is single byte character, destination is double 
**	            byte character
**	    case 4: source is single byte character, destination is single 
**	            byte character
**	21-apr-2003 (penga03)
**	    Took away the last change made in 28-feb-2003.
**	3-sep-04 (inkdo01)
**	    Rescind preceding change - testing destination is ill-considered
**	    since many (most) destination areas are uninitialized and would 
**	    produce unpredictable results.
**	11-apr-2007 (gupsh01)
**	    Add support for UTF8. 
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Create single byte version
*/
# ifdef	DOUBLEBYTE
# define CMcpychar(src, dst) ( (CMbytecnt(src) == 1) ? (*(dst) = *(src)) :( (CMbytecnt(src) == 2) ?( (*((dst)+1) = *((src)+1)),(*(dst) = *(src))) :( (CMbytecnt(src) == 3) ? ( (*((dst)+2) = *((src)+2)),(*((dst)+1) = *((src)+1)),(*(dst) = *(src))) : ( (CMbytecnt(src) == 4) ? ( (*((dst)+3) = *((src)+3)),(*((dst)+2) = *((src)+2)), (*((dst)+1) = *((src)+1)),(*(dst) = *(src)) ) : 0))))
# else
# define CMcpychar(src, dst)	(*(dst) = *(src))
# endif
# define CMcpychar_SB(src, dst) (*(dst) = *(src)) 


/*{
** Name:	CMcpyinc	- copy one character to another
**				(incrementing character pointer)
**
** Description:
**	Copy one character (either one or two bytes) from
**	string 'src' to the next position in string 'dst',
**	incrementing the pointers for both strings by one or two bytes.  
**	This mimics the (*c++ = *d++) expression of C.
**
**	This routine, which is system-independent MACRO, requires 
**	the user to include the global header CM.h.  
**
**(Usage:
**	VOID	CMcpyinc(src, dst)
**	char	*src;
**	char	*dst;
**)
** Inputs:
**	src	pointer to character, which is positioned at the 
**		point in string from which character is to be copied.
**	dst	pointer in string into which character is to be copied. 
**
** Outputs:
**	src, dst	pointer to character, incremented by 1 or 2 bytes.
**
**	Returns:
**		VOID
**
** Side Effects:
**	The passed strings, 'src' and 'dst', are both 
**	changed upon return.
**
** History:
**	18-sep-86 (yamamoto)
**		first written
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Create single byte version
*/
# ifdef	DOUBLEBYTE
# define CMcpyinc(src,dst) ((CMbytecnt(src) == 1) ? (*(dst)++ = *(src)++) : ((CMbytecnt(src) == 2) ? (*(dst)++ = *(src)++, *(dst)++ = *(src)++) : ( (CMbytecnt(src) == 3) ? (*(dst)++ = *(src)++, *(dst)++ = *(src)++, *(dst)++ = *(src)++) : ((CMbytecnt(src) == 4) ? (*(dst)++ = *(src)++, *(dst)++ = *(src)++, *(dst)++ = *(src)++, *(dst)++ = *(src)++) : 0 )))) 
# else
# define CMcpyinc(src, dst)	(*(dst)++ = *(src)++)
# endif
# define CMcpyinc_SB(src,dst) (*(dst)++ = *(src)++) 


/*{
** Name:	CMtolower	- convert to lowercase
**
** Description:
**	Copy one character (either one or two bytes) from
**	string 'src' to string 'dst'.  If a character lies in [A-Z],
**	this routine converts it to lowercase in the 'dst' string.
**
**	Note that the copy can be done in place ('src' = 'dst').
**
**	This routine, which is system-independent MACRO, requires 
**	the user to include the global header CM.h.  
**
**(Usage:
**	VOID	CMtolower(src, dst)
**	char	*src;
**	char	*dst;
**)
** Inputs:
**	src	pointer to character, which is positioned at the 
**		point in string from which character is to be copied.
**	des	pointer in string into which lower case version of
**		character is to be placed.
**
** Outputs:
**	Returns:
**		VOID
**
** Side Effects:
**	None
**
** History:
**	18-sep-86 (yamamoto)
**		first written
**	03-Dec-2009 (wanfr01) Bug 122994
**	    Optimize code for single byte character sets
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Create single byte version
*/
# ifdef	DOUBLEBYTE
# define CMtolower(src, dst) ((CMGETDBL)? ((CMGETUTF8 && CM_UTF8MULTI(src)) ? (cmu_getutf8_tolower((u_char *)src, CMUTF8cnt(src), (u_char *)dst, 0)) : (CMdbl1st(src) ? (*(dst) = *(src), *((dst)+1) = *((src)+1)) : (*(dst) = cmtolower(src)))) : (*(dst) = cmtolower_sb(src)))
# else
# define CMtolower(src, dst)	(*(dst) = cmtolower(src))
# endif
# define CMtolower_SB(src, dst)	(*(dst) = cmtolower_sb(src))


/*{
** Name:	CMtoupper	- convert to uppercase
**
** Description:
**	Copy one character (either one or two bytes) from
**	string 'src' to string 'dst'.  If a character lies in [a-z], 
**	this routine converts it to uppercase in the 'dst' string.
**
**	Note that the copy can be done in place ('src' = 'dst').
**
**	This routine, which is system-independent MACRO, requires 
**	the user to include the global header CM.h.  
**
**(Usage:
**	VOID	CMtoupper(src, dst)
**	char	*src;
**	char	*dst;
**)
** Inputs:
**	src	pointer to character, which is positioned at the 
**		point in string from which character is to be copied. 
**	dst	pointer in string into which upper case version of
**		character is to be placed.
**
** Outputs:
**	Returns:
**		VOID
**
** Side Effects:
**	None
**
** History:
**	18-sep-86 (yamamoto)
**		first written
**	03-Dec-2009 (wanfr01) Bug 122994
**	    Optimize code for single byte character sets
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Create single byte version
*/


# ifdef	DOUBLEBYTE
# define CMtoupper(src, dst) ((CMGETDBL)? ((CMGETUTF8 && CM_UTF8MULTI(src)) ? (cmu_getutf8_toupper((u_char *)src, CMUTF8cnt(src), (u_char *)dst, 0)) : (CMdbl1st(src) ? (*(dst) = *(src), *((dst)+1) = *((src)+1)) : (*(dst) = cmtoupper(src)))) : (*(dst) = cmtoupper_sb(src)))
# else
# define CMtoupper(src, dst)	(*(dst) = cmtoupper(src))
# endif
# define CMtoupper_SB(src, dst) (*(dst) = cmtoupper_sb(src))


/*{
** Name:	CMbyteinc	- increment a byte counter
**
** Description:
**	Increment a byte counter one or two bytes within a
**	string, depending on whether the next character in
**	the string pointed to by 'str' is one or two bytes long.
**	This mimics the	++i expression of C when used in manipulating
**	string pointers, but takes into account 2 byte characters.
**
**	This routine, which is system-independent MACRO, requires 
**	the user to include the global header CM.h.  
**
**(Usage:
**	i4	CMbyteinc(count, str)
**	i4	count;
**	char	*str;
**)
** Inputs:
**	count	byte counter depending on 'str'
**	str	pointer to character within a string.
**
** Outputs:
**	count	This will change the value of the parameter 'count'
**		either to 'count+1' or 'count+2'.
**	Returns:
**		The return value is set to the value of 'count',
**		after increment (as done by ++i).
**
** Side Effects:
**	None
**
** History:
**	18-sep-86 (yamamoto)
**		first written
**	11-apr-2007 (gupsh01)
**		Added support for UTF8.
**	03-Dec-2009 (wanfr01) Bug 122994
**	    Optimize code for single byte character sets
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Create single byte version
*/

# ifdef	DOUBLEBYTE
# define CMbyteinc(count, str)	((CMGETDBL)? ((CMGETUTF8) ? (count += CMUTF8cnt(str)) : (CMdbl1st(str) ? (count) += 2 : ++(count))) : (++count))
# else
# define CMbyteinc(count, str)	(++count)
# endif
# define CMbyteinc_SB(count, str)	(++count)


/*{
** Name:	CMbytedec	- decrement a byte counter
**
** Description:
**	Decrement a byte counter one or two bytes within a
**	string, depending on whether the next character in
**	the string pointed to by 'str' is one or two bytes long.
**	This mimics the --i expression of C when used in manipulating
**	string pointers, but takes into account 2 byte characters.
**
**	This routine, which is system-independent MACRO, requires 
**	the user to include the global header CM.h.  
**
**(Usage:
**	i4	CMbytedec(count, str)
**	i4	count;
**	char	*str;
**)
** Inputs:
**	count	byte counter depending on 'str'
**	str	pointer to character within a string.
**
** Outputs:
**	count	This will change the value of the parameter 'count'
**		either to 'count-1' or 'count-2'.
**	Returns:
**		The return value is set to the value of 'count',
**		after decrement (as done by --i).
**
** Side Effects:
**	None
**
** History:
**	18-sep-86 (yamamoto)
**		first written
**	11-apr-2007 (gupsh01)
**		Added support for UTF8.
**	03-Dec-2009 (wanfr01) Bug 122994
**	    Optimize code for single byte character sets
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Create single byte version
*/

# ifdef	DOUBLEBYTE
# define CMbytedec(count, str)	((CMGETDBL)? ((CMGETUTF8) ? (count -= CMUTF8cnt(str)) : (CMdbl1st(str) ? (count) -= 2 : --(count))) : (--(count)))
# else
# define CMbytedec(count, str)	(--(count))
# endif
# define CMbytedec_SB(count, str)	(--(count))


/*{
** Name:	CMbytecnt	- count the bytes in the next character
**
** Description:
**	Count the number of bytes in the next character, returning
**	either 1 or 2.
**
**	This routine, which is system-independent MACRO, requires 
**	the user to include the global header CM.h.  
**
**(Usage:
**	i4	CMbytecnt(str)
**	char	*str;
**)
** Inputs:
**	str	pointer to character within a string.
**
** Outputs:
**	Returns:
**		The number of bytes in the next character, either 1 or 2.
**
** Side Effects:
**	None
**
** History:
**	11-apr-2007 (gupsh01)
**		Added support for UTF8.
**	03-Dec-2009 (wanfr01) Bug 122994
**	    Optimize code for single byte character sets
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Create single byte version
*/
 
# ifdef DOUBLEBYTE
# define CMbytecnt(ptr)	((CMGETDBL)? ((CMGETUTF8) ? (CMUTF8cnt(ptr)) : (CMdbl1st(ptr) ? 2 : 1)) : 1)
# else
# define CMbytecnt(ptr)	1
# endif
# define CMbytecnt_SB(ptr)	1


/*{
** Name:	CMnext	- increment a character string pointer
**
** Description:
**	Move string pointer forward one character within a string.
**	This mimics the ++c expression in C, but takes
**	into account whether the next character is
**	one or two bytes long.
**
**	Note that if you use this routine in conjunction with
**	either CMbyteinc or CMbytedec, you should call the CMbyte* 
**	routine first to make sure that you are pointing to the same character.  
**
**	This routine, which is system-independent MACRO, requires 
**	the user to include the global header CM.h.  
**
**(Usage:
**	char	*CMnext(str)
**	char	*str;
**)
** Inputs:
**	str	pointer to character to be incremented
**
** Outputs:
**	str	This will change the value of the parameter 
**		'str' to either 'str+1' or 'str+2', 
**		depending on the size of the character.  
**	Returns:
**		The return value is set to the incremented value 
**		sent to the routine (as done by ++c).  
**
** Side Effects:
**	None
**
** History:
**	18-sep-86 (yamamoto)
**		first written
**	11-apr-2007 (gupsh01)
**		Added support for UTF8.
**	03-Dec-2009 (wanfr01) Bug 122994
**	    Optimize code for single byte character sets
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Create single byte version
*/

# ifdef	DOUBLEBYTE
# define CMnext(str)	((CMGETDBL)? ((CMGETUTF8) ? ((str) += (CMUTF8cnt(str))) : ((CMdbl1st(str)) ? ((str) += 2) : (++(str)))) : (++str))
# else
# define CMnext(str)	(++(str))
# endif
# define CMnext_SB(str)	(++(str))


/*{
** Name:	CMprev	- decremenent a character pointer
**
** Description:
**	Move backwards one character within a string.
**	This mimics the --c expression in C, but takes
**	into account whether the previous character is
**	one or two bytes long.
**
**	Note that if you use this routine in conjunction with
**	either CMbyteinc or CMbytedec, you should call
**	the CMprev reoutine first to make sure that you
**	are pointing to the same character.  
**
**	THE USE OF THIS ROUINE IS HIGHLY DISCOURAGED
**	UNLESS ABSOLUTELY NECESSARY.  PLEASE TRY TO CODE
**	SUCH THAT MOVING BACKWARDS WITHIN A STRING IS
**	NOT NEEDED.
**
**	This routine, which is system-independent MACRO, requires 
**	the user to include the global header CM.h.  
**
**(Usage:
**	char	*CMprev(str, startpos)
**	char	*str;
**	char	*startpos;
**)
** Inputs:
**	str	pointer to character to be decremented
**	startpos	start pointer of the string being processed
**		(or a location in the string which you know contains
**		the start of a character) as double byte processing
**		must reprocess the string from the start to find
**		the previous character.
**
** Outputs:
**	str	This will change the value of the parameter
**		'str' to either 'str-1' or 'str-2',
**		depending on the size of the character.
**
**	Returns:
**		The return value is set to the value
**		of 'str' after decrement (as done by --c). 
**
** Side Effects:
**	None
**
** History:
**	18-sep-86 (yamamoto)
**		first written
**	19-apr-2007 (gupsh01)
**	    Modified to include UTF8 support.
**	23-mar-2007 (gupsh01)
**	    Fixed the start point to this routine.
**	03-Dec-2009 (wanfr01) Bug 122994
**	    Optimize code for single byte character sets
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Create single byte version
*/
#define CMuprev(str, startpos) ((str) -= (cmupct((u_char *)(str), (u_char *)startpos))) 
# ifdef	DOUBLEBYTE
# define CMprev(str, startpos)	((CMGETDBL) ?  ((CMGETUTF8) ?  (CMuprev(str, startpos)) : ((cmkcheck((u_char *)((str)-1), (u_char *)startpos) == CM_A_DBL2) ? (str) -= 2 : --(str))) : (--(str)))
# else
# define CMprev(str, startpos)	(--(str))
# endif
# define CMprev_SB(str, startpos)	(--(str))


/*{
** Name:	CMcmpnocase	- compare characters, ignoring case.
**
** Description:
**	Compare the next character in each of two strings,
**	either single or double byte characters.
**	(Case is not significant)
**
**	This routine, which is system-independent MACRO, requires 
**	the user to include the global header CM.h.  
**
**(Usage:
**	i4	CMcmpnocase(str1, str2)
**	char	*str1;
**	char	*str2;
**)
** Inputs:
**	str1, str2	pointer to characters, within strings, to be compared
**
** Outputs:
**	Returns:
**		<  0	if char1 < char2
**		== 0	if char1 == char2
**		>  0	if char1 > char2
**
**			(char1 and char2 is one character, include double
**			byte character, within the string str1
**			and str2.)
** Side Effects:
**	None
**
** History:
**	18-sep-86 (yamamoto)
**		first written
**	03-Dec-2009 (wanfr01) Bug 122994
**	    Optimize code for single byte character sets
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Create single byte version
*/

# ifdef	DOUBLEBYTE
# define CMcmpnocase(str1, str2) ((CMGETDBL) ? (CMdbl1st(str1) ? CMcmpcase(str1, str2) : \
	((i4)(cmtolower(str1)) - (i4)(cmtolower(str2)))) : \
	((i4)(cmtolower(str1)) - (i4)(cmtolower(str2))))
# else
# define CMcmpnocase(str1, str2)	((i4)(cmtolower(str1)) - (i4)(cmtolower(str2)))
# endif
# define CMcmpnocase_SB(str1, str2)	((i4)(cmtolower_sb(str1)) - (i4)(cmtolower_sb(str2)))


/*{
** Name:	CMcmpcase	- compare characters
**
** Description:
**	Compare the next character in each of two strings,
**	either single or double byte.  (Case is significance)
**
**	This routine, which is system-independent MACRO, requires 
**	the user to include the global header CM.h.  
**
**(Usage:
**	i4	CMcmpcase(str1, str2)
**	char	*str1;
**	char	*str2;
**)
** Inputs:
**	str1, str2	pointer to characters, within strings, to be compared
**
** Outputs:
**	Returns:
**		<  0	if char1 < char2
**		== 0	if char1 == char2
**		>  0	if char1 > char2
**
**			(char1 and char2 is one character, include double
**			byte character, within the string str1
**			and str2.)
** Side Effects:
**	None
**
** History:
**	18-sep-86 (yamamoto)
**		first written
**	03-Dec-2009 (wanfr01) Bug 122994
**	    Optimize code for single byte character sets
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Create single byte version
*/

# ifdef	DOUBLEBYTE
# define CMcmpcase(str1, str2)	((CMGETDBL)?  (CMdbl1st(str1) ?  (((i4)(CM_DEREF(str1)) == (i4)(CM_DEREF(str2))) ? \
			((i4)(CM_DEREF((str1)+1)) - (i4)(CM_DEREF((str2)+1))) : \
			((i4)(CM_DEREF(str1)) - (i4)(CM_DEREF(str2)))) : ((i4)(CM_DEREF(str1)) - (i4)(CM_DEREF(str2)))) \
			: ((i4)(CM_DEREF(str1)) - (i4)(CM_DEREF(str2))))

# else
# define CMcmpcase(str1, str2)	((i4)(CM_DEREF(str1)) - (i4)(CM_DEREF(str2)))
# endif
# define CMcmpcase_SB(str1, str2)	((i4)(CM_DEREF(str1)) - (i4)(CM_DEREF(str2)))

/*{
**  Name: CMcopy - Copy character data from source to destination.
**
**  Description:
**	CMcopy will copy (not necessarily null-terminated) character data
**	from a source buffer into a destination buffer of a given length.
**	Like STlcopy, CMcopy returns the number of bytes it copied
**	(which may differ from the number of bytes requested to be copied
**	if the last byte was double byte and could not be completely copied).
**
**	This routine, which is a system-independent macro, requires the user
**	to include the global header <cm.h>.  Also note that because some of 
**	the macro expansions may use copy_len twice, you should be aware of
**	possible side effects (ie, ++ and -- operators).
**
**(Usage:
**	u_i4	length, copy_len;
**	char	*source, *dest;
**)
**	length = CMcopy(source, copy_len, dest);
**
**  Inputs:
**	source    - Pointer to beginning of source buffer.
**	copy_len  - Number of bytes to copy.
**	dest	  - Pointer to beginning of destination buffer.
**
**  Outputs:
**	Returns:
**	    Number of bytes actually copied (this may be less than copy_len
**	    in the case of double-byte characters, but will never be more).
**
**  Side Effects:
**	None
**	
**  History:
**	08-mar-1987	- Written (_cmcopy not yet written) (ncg)
**      02-mar-1998 (kosma01)
**          Include me.h for DOUBLEBYTE. Some files, (cl/clf/dl/dl.c,
**          cl/clf/ut/utcompile.c and front/rep/repmgr/maillist.qsc)
**          do not include me.h. On AIX 4.x utcompile.c and dl.c are
**          missing a prototype for MEreqmem, causing perfectly good
**          assignments to char * to go awry. For maillist.qsc a late
**          include of me.h and then sys/m_types.h after front/hdr/hdr/ft.h
**          defines reg as register, causes syntax errors.
**	13-Jul-2006 (kschendel)
**	    MEcopy size is no longer i2.
**	03-Dec-2009 (wanfr01) Bug 122994
**	    Optimize code for single byte character sets
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Create single byte version
*/

#ifdef	DOUBLEBYTE
#ifndef MECOPY_VAR_MACRO
# include <me.h>
#endif
#  define   CMcopy(source, copy_len, dest)				    \
	((CMGETDBL)? (cmicopy((source), (copy_len), (dest))) : 		/* Uses for loop */ \
    (MEcopy((PTR)(source), (copy_len), (PTR)(dest)),copy_len))
#  define   CMcopy_SB(source, copy_len, dest)				    \
    (MEcopy((PTR)(source), (copy_len), (PTR)(dest)),copy_len)
#else
#ifndef MECOPY_VAR_MACRO
# include <me.h>
#endif
#  define   CMcopy(source, copy_len, dest)		 		    \
    (MEcopy((PTR)(source), (copy_len), (PTR)(dest)),copy_len)
#endif

/*
** Name: CMATTR
**
** Description:
**
**	Structure which defines the attribute array and case translation
**	table for a character set.
**
**	Both items are indexed by character value.  The attr item contains
**	the bits used for character classification routines.  The xcase
**	array contains the alternate case character if the character is
**	upper or lower case.  It is meaningless otherwise.
*/

typedef struct
{
	u_i2 attr[256];
	char xcase[256];
} CMATTR;

# if defined(NT_GENERIC)

/*
** Name: CMhost
**
** Description:
**      Test for the host attribute
**
** Input:
**      str     pointer to character to test
**
** Outputs:
**	Returns:
**		0	Char not hostname character
**		!=0	Char is hostname character
**
** History:
**      25-jun-2002 (fanra01)
**          Created.
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Create single byte version
*/
# define CMhost(str)	(CMGETATTR(str) & CM_A_HOST)
# define CMhost_SB(str)	(CMGETATTR_SB(str) & CM_A_HOST)

/*
** Name: CMuser
**
** Description:
**      Test for the user attribute
**
** Input:
**      str     pointer to character to test
**
** Outputs:
**	Returns:
**		0	Char not username character
**		!=0	Char is username character
**
** History:
**      25-jun-2002 (fanra01)
**          Created.
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Create single byte version
*/
# define CMuser(str)    (CMGETATTR(str) & CM_A_USER)
# define CMuser_SB(str)    (CMGETATTR_SB(str) & CM_A_USER)

/*
** Name: CMpath
**
** Description:
**      Test for the path attribute
**
** Input:
**      str     pointer to character to test
**
** Outputs:
**	Returns:
**		0	Char not pathname character
**		!=0	Char is pathname character
**
** History:
**      25-jun-2002 (fanra01)
**          Created.
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Create single byte version
*/
# define CMpath(str) (CMGETATTR(str) & CM_A_PATH)
# define CMpath_SB(str) (CMGETATTR_SB(str) & CM_A_PATH)

/*
** Name: CMpathctrl
**
** Description:
**      Test for the path control attribute.
**      NB. Overloaded with CM_A_NCALPHA only to be used with install
**          testing.
**
** Input:
**      str     pointer to character to test
**
** Outputs:
**	Returns:
**		0	Char not path control character
**		!=0	Char is path control character
**
** History:
**      25-jun-2002 (fanra01)
**          Created.
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Create single byte version
*/
# define CMpathctrl(str) (CMGETATTR(str) & CM_A_PATHCTRL)
# define CMpathctrl_SB(str) (CMGETATTR_SB(str) & CM_A_PATHCTRL)
# endif /* NT_GENERIC */

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
**     size_t     number of bytes written if successful
**
** History:
**      06-feb-2004 (drivi01)
**          Created.
**      19-aug-2008 (whiro01)
**          Made into real function for NT_GENERIC (moved to cmutf8.c).
*/
#if defined(NT_GENERIC)
size_t CMwcstombs(const wchar_t *wcstr, char *mbstr, size_t mbsize);
#else
#define CMwcstombs(wcstr, mbstr, mbsize) wcstombs(mbstr, (const wchar_t *)wcstr, mbsize)
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
**          Made into real function for NT_GENERIC (moved to cmutf8.c).
*/
#if defined(NT_GENERIC)
size_t CMmbstowcs(const char *mbstr, wchar_t *wcstr, size_t wcsize);
#else
#define CMmbstowcs(mbstr, wcstr, wcsize) mbstowcs(wcstr, mbstr, wcsize)
#endif 

/*
** Name: CMwcslen
**
** Description:
**      Measures length of a wchar_t
**
** Input:
**      wcstr      pointer to wide character string
**      
**
** Output:
**      int Length
**
** History:
**      12-mar-2004 (drivi01)
**          Created.
*/
#if defined(NT_GENERIC) 
#define CMwcslen(wcstr) (wcslen(wcstr))
#else /*NT_GENERIC*/
#define CMwcslen(wcstr) (wcslen(wcstr))
#endif

/*
** Name: CMwcsncmp
**
** Description:
**      Compares two wchar_t strings
**
** Input:
**      wcstr      pointer to wide character string1
**	wcstr	   pointer to wide character string2
**	int	   Size of a string
**      
**
** Output:
**      int Resolution
**	0 - Identical
**	>0 or <0 Different 
**
** History:
**      12-mar-2004 (drivi01)
**          Created.
*/
#if defined(NT_GENERIC) 
#define CMwcsncmp(wcstr1, wcstr2, int) (wcsncmp(wcstr1, wcstr2, int))
#else /*NT_GENERIC*/
#define CMwcsncmp(wcstr1, wcstr2, int) (wcsncmp(wcstr1, wcstr2, int))
#endif

/*
** Name: CMwcschr
**
** Description:
**      Finds a character in a string
**
** Input:
**      wcstr      pointer to wide character string
**	wc	   character to search for
**      
**
** Output:
**      wcstr	   pointer to the list of occurances
**
** History:
**      12-mar-2004 (drivi01)
**          Created.
*/
#if defined(NT_GENERIC) 
#define CMwcschr(wcstr1, wc) (wcschr(wcstr1, wc))
#else /*NT_GENERIC*/
#define CMwcschr(wcstr1, wc) (wcschr(wcstr1, wc))
#endif


/*
** Name: CMwcscpy
**
** Description:
**      Copy a string
**
** Input:
**      str1      pointer to a destination character string
**	str2	  pointer to a source character string
**      
**
** Output:
**      wcstr	   pointer to the destination string
**			if null indicates an error.
**
** History:
**      12-mar-2004 (drivi01)
**          Created.
*/
#if defined(NT_GENERIC) 
#define CMwcscpy(str1, str2) (wcscpy(str2, str1))
#else /*NT_GENERIC*/
#define CMwcscpy(str1, str2) (wcscpy(str2, str1))
#endif



/*
** Name: CMwcsncpy
**
** Description:
**      Copy a string
**
** Input:
**      str1      pointer to a destination character string
**	str2	  pointer to a source character string
**	size	  number of characters to be copied
**      
**
** Output:
**      wcstr	   pointer to the destination string,
**			if null is returned indicates an error.
**
** History:
**      12-mar-2004 (drivi01)
**          Created.
*/
#if defined(NT_GENERIC) 
#define CMwcsncpy(str1, str2, size) (wcsncpy(str2, str1, size))
#else /*NT_GENERIC*/
#define CMwcsncpy(str1, str2, size) (wcsncpy(str2, str1, size))
#endif
