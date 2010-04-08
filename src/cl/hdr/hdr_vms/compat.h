/*
** Copyright (c) 1985, 2008 Ingres Corporation
**
*/

/**CL_SPEC
** Name: COMPAT.H - Compatibility library definitions
**
** Specification:
**
** Description:
**      This file contains the standard environment definitions for the 
**      compatibility library.  These definitions include standard types,
**      special preprocessor constants for conditional compilation, and
**      standard constants.  The special preprocessor constants are used
**      to describe characteristics of the processor, operating system
**      and compiler.
**
** History:
**      17-sep-1985 (derek)
**          Changed to conform to coding standard for Jupiter.
**      19-nov-1985 (fred)
**          Changed STRUCT_ASSIGN to STRUCT_ASSIGN_MACRO
**	    to conform to coding standard for Jupiter.
**	08-jan-86 (jeff)
**	    Added ENDFILE, which is returned by SIread() and SIgetrec()
**	14-mar-86 (fred)
**	    Added def'n for PTR, as specified by the committee.  It is now
**	    a char *, but should be changed to whatever's appropriate
**      17-mar-86 (jennifer)
**          Added a typedef (CL_SYS_ERR) for system errors returned from CL.
**          The error was originally a longnat, but the CL committee 
**          wanted a structure.
**	15-apr-86 (ericj)
**	    Added I1_CHECK defn.  This is needed for machines that do not
**	    support signed characters (e.g. 3b5, UTS, etc.).
**	28-apr-86 (ericj)
**	    Added NULLCHAR and EOS defn for use with string datatypes.
**	7-may-86 (ericj)
**	    Update MINI2 definition so that if (x >= MINI2) works properly,
**	    where x is a nat.  Updated MINI1 as well.
**	13-may-86 (ericj)
**	    Renamed I1_CHECK macro to I1_CHECK_MACRO according to coding
**	    specs.
**	11-sep-1986 (Joe)
**	    Added the CLERRV structure as part of Jupiter integration.
**	16-sep-1986 (neil)
**	    Added the definition of _VOID_ to allow (void) casts on all systems.
**	10-nov-1986 (thurston)
**	    Added [I2,I4,F4,F8]ASSIGN_MACRO's.
**      20-feb-1987 (fred)
**          Changed STRUCT_ASSIGN_MACRO(to, from) to
**	    STRUCT_ASSIGN_MACRO(from, to)
**      18-mar-1987 (thurston)
**          Added WSCREADONLY.
**      19-mar-1987 (thurston)
**          Removed NEG_I1 since it has been replaced by I1_CHECK_MACRO.  (Not
**          to mention that it had been defined improperly here, anyway!)
**	20-apr-1987 (mmm)
**	    Added DB_C_CONST_MACRO() to be used to define constants previously
**	    defined as 'abcd'.
**	30-apr-1987 (mmm)
**	    Changed DB_C_CONST_MACRO to CV_C_CONST_MACRO and fixed ordering of
**	    characters.
**      20-may-87 (daved)
**          made MAX_CHAR 0377
**	30-nov-87 (stec)
**	    Added ME_ALIGN_MACRO.
**      15-Jan-88 (jbowers)
**          Added error mask for GC facility.
**	20-jul-1988 (roger)
**		Respecified CL_SYS_ERR as CL_ERR_DESC.
**	21-oct-88 (thurston)
**	    Upgraded the [I2,I4,F4,F8]ASSIGN_MACRO's to do proper casting.
**	23-jan-1989 (roger)
**	    Added CLERROR(x) macro (TRUE if x is a CL return error).
**      15-jun-89 (jrb)
**          Added GLOBALCONSTREF and GLOBALCONSTDEF.
**      95-sep-89 (jennifer)
**          Added definition of a security label that DBMS.H can 
**          reference.  This is done here since the label is operating 
**          system specific and I did not want to change every file
**          in the system to include sl.h in front of dbms.h.
**	29-aug-90 (Mike S)
**	    Add CL_OFFSETOF macro
**	15-oct-90 (greg)
**	    Must define COMPAT_INCLUDE or cm.h won't build (also prevents
**		multiple inclusion
**	1-feb-92 (Mike S)
**	    Add E_DL_MASK
**	20-aug-92 (kwatts)
**	    Added E_SD_MASK (set to 6.4 value).
**	29-oct-92 (walt)
**		Adapt for Alpha.  Define BYTE_ALIGN for Alpha.
**	11/06/92 (dkh) - Changed definition of MINI4 to be (-MAXI4 - 1)
**			 to get around an apparent alpha C compiler bug.
**	16-nov-92 (walt)
**		Define and i8 type using the system definition for 64 bit integers
**		from <ints.h>.  This is initially used in <cs.h> to define an array
**		that can save the (64 bit) alpha registers during thread switches.
**	11/19/92 (dkh) - Changed definitions of MINI2 and MINI1 to be
**			 (-MAXI2 - 1) and (-MAXI1 - 1) to be around
**			 an apparent ALPHA C compiler bug.  Similar to the
**			 11/06/92 change.
**	02-dec-92 (kwatts)
**	    E_SD_MASK was not set to 6.4 value in previous change. Had
**	    to move E_CM_MASK to make sure E_SD_MASK did not change.
**	18-jan-1993 (bryanp)
**	    Add CL_CLEAR_ERR macro.
**	13-jan-93 (pearl)
**	    Add #define for CL_BUILD_WITH_PROTOS if xDEBUG is defined.
**	21-jan-93 (walt)
**	    Change ALIGN_RESTRICT to be 'double' for Alpha.
**	08-jul-93 (walt)
**	    CL_BUILD_WITH_PROTOS nolonger used.  Don't define it anymore.
**	16-jul-1993 (rog)
**	    Added E_FP_MASK.
**	11-aug-93 (ed)
**	    changed to use CL_PROTOTYPED
**	01-feb-1995 (wolf)
**	    Add the following changes from the VAX/VMS version:
**		03-feb-94 (stephenb)
**		Added definition for E_SA_MASK for new sub-library SA.
**		05-may-94 (robf)
**		Updated for secure 2.0 integration
**		24-may-94 (vijay)
**		Added FABS macro as copy of ABS. Since some abs calls in the
**		generic code were changed to fabs, we are having problems with
**		linking executables with adf. To avoid using the system library
**		vaxcrtl.olb in the .mms files we are adding the fabs macro here.
**	02-feb-1995 (wolf)
**	    Yanked the #define for fabs, due to compile errors in cvfa and mh.
**	14-feb-1995 (wolf)
**	    Re-instate fabs(), the ingres!oping11 version used it and undefined
**	    the macro in cvfa and mh.
**	28-JUN-1995 (whitfield)
**	    Work around Alpha VMS bug that REQUIRES iosb on system services.
**	26-sep-1995 (dougb)
**	    Update the *ASSIGN_MACRO definitions to avoid alignment faults.
**	    Move iisys_* hack inside COMPAT_INCLUDE block and remove duplicate
**	    definition of CL_SYS_ERR.
**	11-feb-97 (chech02)
**	    fixed compiler error in CL_CLEAR_ERR marco define to 
**		# define CL_CLEAR_ERR( s )	(s)->error = 0
**	02-sep-1997 (teresak)
**	    Add in null pointer definition
**	23-mar-1998 (kinte01)
**	    Add in definition for unsigned short (u_short)
**	31-aug-1998 (kinte01)
**	    Add SCALARP
**	11-jun-1998 (kinte01)
**	    Cross integrate change 435890 from oping12
**          18-may-1998 (horda03)
**            X-Integrate Change 427896 to enable Evidence Sets.
**	30-jun-1998 (kinte01)
**	    Remove definition of abs. This is defined in stdlib.h 
**      10-jun-1999     (kinte01)
**         Change VAX to VMS as VAX should not be defined on AlphaVMS
**         systems. The definition of VAX on AlphaVMS systems causes
**         problems with the RMS GW (98236).
**	29-oct-1999 (kinte01)
**          Added MAXFS for MAXimum Files Size
**          Added LARGEFILE64 for files > 2gig
**          Added OFFSET_TYPE, longlong
**	01-nov-1999 (kinte01)
**	   Add moreinfo to CL_ERR_DESC (sir 99683)
**	29-dec-1999 (kinte01)
**	    Move toward getting rid of /standard=VAXC
**	24-may-2000 (kinte01)
**	   Changes for Compaq C 6.2 compiler to not generate a warning
**	   that Compaq C recognizes the keyword "signed" whereas VAX C
**	   does not and would treat as unsigned.
**	   Added this here instead of in cpe_buildenv_locals.com
**	   as we have exceeded the length of the command line we can
**	   define.
**	   Ensure that the compiler will not emit diagnostics about
**	   "signed" keyword usage when in /STAND=VAXC mode (the reason
**	   for the diagnostics is that VAX C does not support the signed
**	   keyword).  This can be removed when we no longer compile
**	   in VAXC mode.
**      19-jul-2000 (kinte01)
**         Turns on CL prototyped declarations
**	23-aug-2000 (kinte01)
**	   Add u_long definition to VMS CL for change 446586
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	22-jan-2001 (kinte01)
**	   29-Nov-1999 (hanch04)
**	      First stage of updating ME calls to use OS memory management.
**	      Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	23-jan-2001 (kinte01)
**	   Add include of stddef.h to pick up definition of size_t
**	13-mar-2001 (kinte01)
**	   Add SETCLERR macro
**	08-aug-2001 (kinte01)
**	   Add UCS2 definition 
**	   Add type definitions for simple unicode types.
**	   Add in uchar, ushort, & ulong definitions since we seem to use
**	   both these definitions and u_char, u_short, and u_long
**	   types.h must be included with sys$library specified to ensure
**	   that the correct types.h in case there is a local types.h file
**	   (e.g. vifred)
**	27-aug-2001 (kinte01)
**	   rename max_char to iimax_char_value since it conflicts with
**	   limits.h max_char on sequent and the compat.h value is overwritten
**	   Required as changes affect generic code per change 452604
**      04-Feb-2002 (fanra01)
**          Bug 107016
**          Add macros for obtaining the item count from an octet length and
**          obtainin octet length from an item count.
**	16-oct-2002 (abbjo03)
**	    Remove fabs macro.
**	07-Oct-2002 (hanch04)
**	    Added MAX_SIZE to be the largest integer in a long.
**	01-Nov-2002 (hweho01)
**	    Renamed MAX_SIZE to MAX_SIZE_TYPE, avoid conflicts with
**	    the existing definitions of MAX_SIZE in the source files
**	    of fe/rep/repmgr.
**	09-sep-2003 (abbjo03)
**	    Use HP-provided OS headers. Allow all warnings
**	23-oct-2003 (devjo01)
**	    Add E_CX_MASK, CL_GET_OS_ERR, as part of a hand cross of 
**	    applicable 103715 changes from the UNIX CL.
**	29-oct-2003 (schka70/abbjo03)
**	    Define globalref/def differently for C++, needs to be extern "C".
**	    FUNC_EXTERN too.
**	6-Jan-2004 (schka24)
**	    Add i8 macros
**	14-May-2004 (schka24)
**	    C typing quirk makes MINI4 useless for i8's, define MINI4LL
**	01-sep-2004 (abbjo03)
**	    Include bzarch.h and remove things defined there.
**	04-Oct-2004 (bonro01)
**	    Remove header file stucture.
**	23-Jun-2005 (schka24)
**	    Add SSIZE_TYPE back in.  Bletch.  We ought to go back to
**	    including sys/types.h, which should be rather better standardized
**	    by now (it's 16 years since fred removed it, above.)
**	14-feb-2007 (dougi)
**	    Add CL_MAX_DECPREC to parameterize decimal precision limits.
**	26-Dec-2007 (kiria01) SIR119658
**	    Define CL_LIMIT_DECVALF8 - the lowest number bigger than all
**	    representable decimals.
**	28-apr-2008 (joea)
**	    Remove #define for NULL and depend instead on stddef.h.
**	07-Oct-2008 (smeke01) b120907
**	    Added define for max u_i8 digit width.
**	28-oct-2008 (joea)
**	    Remove synchronous system services hack.
**	01-dec-2008 (joea)
**	    Add E_HANDY_MASK.
**      12-dec-2008 (stegr01)
**          Add conditional define for ALPHA and IA64
**	9-Nov-2009 (kschendel) SIR 122757
**	    Add more casting to ME_ALIGN_MACRO to prevent pointer truncation.
**      26-Nov-2009 (hanal04) Bug 122938
**          Added CL_MAX_DEC_PREC_39 and CL_MAX_DEC_PREC_31 defines.
**          Different GCA protocol levels can handle different levels
**          of decimal precision.
**	09-Feb-2010 (smeke01) b123226, b113797 
**	    Added define for max i8 digit width.
**/

# ifndef    COMPAT_INCLUDE
# define    COMPAT_INCLUDE  0

# include	<bzarch.h>

#define __NEW_STARLET

/* pick up definition of size_t and NULL */
# include <stddef.h>

/* errno is used below, but can be defined as many things */
# include <errno.h>


/*
** Turns on CL prototyped declarations 
*/
#define CL_PROTOTYPED
#ifdef xDEBUG
#define CL_DONT_SHIP_PROTOS
#endif

/* 
** Defined Constants
*/

/*
**  Define preprocessor constants used describe the machine that the code
**  is being compiled on.  In the worst case the code can contain an #ifdef
**  to handle differences between processors, operating systems and compilers
**  by referencing one of these constants. The indented values denote
**  specific characteristics of the processor, operating system and compiler.
*/

#if (defined(__alpha))
#define		ALPHA			/* The processor is an Alpha. */
#elif (defined(__ia64))
#define         IA64                    /* The processor is an Itanium */
#else
# error "ERROR - Only ALPHA or IA64 builds are expected"
#endif
#define		BITS_IN(a)	(sizeof(a) * BITSPERBYTE)
/*
**  Don't need to define VMS because it's automatically defined in the
**  cross compiler and in the Alpha C compiler.
*/
#define		VMS_V4			/* Generic Version 4 of VMS. */
#define		UPPER_FILENAME
#define		MATH_EX_HANDLE
#define		DECC			/* The compiler is VAXC V2. */


/* If DOUBLEBYTE is defined,
** update define CSDECMULTI to CSDECKANJI in CM.H!	
*/

#define		    STRUCT_ASSIGN_MACRO(b,a)	((a) = (b))

/*
** Macros for returning item or octet counts.
*/
# define II_COUNT_ITEMS(x, type)        (x / sizeof(type))
# define II_COUNT_OCTETS(x, type)       (x * sizeof(type))

/*
** Following macro aligns pointer `ptr' on memory boundary that is
** determined by N. It is user's responsibility to check if, in case of 
** dynamic memory, the alignment is possible, ie., if the pointer,
** after alignment, will be still pointing to allocated memory, and
** not to something beyond it.
**
** Current implementation assumes that N is a power of 2.
** N is the length of the data type for which `ptr' is aligned.
*/
#define		ME_ALIGN_MACRO(ptr, N)	\
		    ((PTR) ((SCALARP)((char *) ptr + (N - 1)) & ~((SCALARP)(N) - 1)))

#if defined(__cplusplus)
#define		FUNC_EXTERN		extern "C"		
#else
#define		FUNC_EXTERN				
#endif

#define		main		compatmain

/* Used to declare four character constants for debugging purposes. 
** Multi-character constants of the form 'abcd' do not compile on some
** machines, so the following macro has been added.
*/

#define	CV_C_CONST_MACRO(a, b, c, d)	\
				(i4)((((i4)(d) & 0xff) << 24) |\
					  (((i4)(c) & 0xff) << 16) |\
					  (((i4)(b) & 0xff) <<  8) |\
					  (((i4)(a) & 0xff) <<  0))
/*
** Used to find offsets of structure members
*/
# define CL_OFFSETOF(s_type, m_name) ((u_i4)&(((s_type *)NULL)->m_name))



/*
**  Define standard macros supplied by the CL.  The coding standard prohibits
**  the use of macros for functions, these are kept for historical reasons.
*/
/*
# define	abs(x)			(((x) < 0) ? -(x) : (x))
*/
# define	min(x,y)		((x) < (y) ? (x) : (y))
# define	max(x,y)		((x) > (y) ? (x) : (y))
# define	maskset(bit,word)	(word |= bit)
# define	maskclr(bit,word)	(word &= ~bit)
# define	masktest(bit,word)	((bit) & (word))


/*
**  This section defines the what the CL standard types map to on VMS 
*/

# define	nat       	DONT_USE_NAT
# define	u_nat     	DONT_USE_UNAT
# define	longnat   	DONT_USE_LONGNAT
# define	u_longnat 	DONT_USE_ULONGNAT

#define		bool		int
#define		FALSE		0
#define		TRUE		1
#define		u_char		unsigned char
#define		uchar		unsigned char

/* the following four #defines are OBSOLETE - see CL Spec */
#define		NULLCHAR	'\0'
#define       	NULLPTR		((PTR)0)        /* null pointer */
#define		IIMAX_CHAR_VALUE 0xff		/* previously MAX_CHAR */
#define		MIN_CHAR	' '

#define		EOS		'\0'
#define		i1		char
#define		MAXI1		0x7f
#define		MINI1		(-MAXI1 - 1)
#define		I1MASK		0xff
#define		u_i1		unsigned char
#define		i2		short
#define		MAXI2		0x7fff
#define		MINI2		(-MAXI2 - 1)
#define		u_i2		unsigned short
#define		ushort		unsigned short
#define		u_short		unsigned short
#define		i4		int
#define		MAXI4		0x7fffffff
#define        MAXI8            0x7fffffffffffffffL
#define		MINI4		(-MAXI4 - 1)
#define        MINI8            ((-MAXI8) - 1)
/* A C typing quirk makes MINI4 truly the same as 0x80000000, meaning that
** if you use MINI4 in an i8 context you get *positive* 2147483648, not
** negative.  Define a MINI4 to be used in an i8 context.
** Example pseudocode:
**   i8 thing;  WRONG: if (thing between MINI4 AND MAXI4) fits in i4;
**              RIGHT: if (thing between MINI4LL AND MAXI4) fits in i4;
** (You can use MINI4LL in an i4 context as well, and it should work,
** but I'm not brave enough to simply redefine MINI4!  I wouldn't want
** to cause any unexpected type coercions.)
*/
#ifdef LP64
# define	MINI4LL		(-2147483648L)
#else
# define	MINI4LL		(-2147483648LL)
#endif

#define		u_i4		unsigned int
#define		f4		float
#define		f8		double
#define		FMIN		1E-37	
#define		FMAX		1E37

/*  Following definitions of 64 bit integers taken from Alpha <ints.h>  */
#define		i8		signed __int64
#define		u_i8		unsigned __int64

typedef		char 		*PTR;
#define		ulong		unsigned long
#define		u_long		unsigned long
#define		longlong	long long
#define		BITFLD		unsigned int
#define		VOID		void
#define		_VOID_		(void)
#define		STATUS		int
#define		OK		0
#define		FAIL		1
#define		SUCCEED		0
#define		ENDFILE		9900

/*
** Unicode character types
*/
typedef		unsigned short	ucs2;
typedef		unsigned long	ucs4;
typedef		unsigned char	utf8;

/*
** Internal representation of wide characters
*/
typedef		ucs2		g_char;

typedef		u_i2		UCS2;   /* This is the Unicode character
					** data type, ie. unicode code point
					** upto 2 bytes.
					*/


#define		MIN_UCS2	0
#define		MAX_UCS2	0xffff
#define		MAX_UCS4	0x7fffffffUL

/*
** UTF width for representing UCS2
*/
# ifndef MAX_UTF8_WIDTH
# define	MAX_UTF8_WIDTH	6
# endif

#define MAX_UI8_DIGITS 20 /* u_i8 has max 20-digits in decimal */
#define MAX_I8_DIGITS_AND_SIGN 21 /* i8 has max 20-digits in decimal + 1 for sign */

/* Maximum decimal precision. */
# ifndef CL_MAX_DECPREC
# define CL_MAX_DECPREC_31 31 /* GCA_PROTOCOL_LEVEL_66 and below */
# define CL_MAX_DECPREC_39 39 /* GCA_PROTOCOL_LEVEL_67 and above */
# define CL_MAX_DECPREC CL_MAX_DECPREC_39 /* Highest supported precision */
   /* Define the lowest number that is bigger than all
   ** representable decimals. Floats whose magnitude is greater
   ** or equal to this cannot be represented in decimal
   ** datatype.
   */
#  ifndef CL_LIMIT_DECVALF8
#  define CL_LIMIT_DECVALF8 1.0E39
#  endif
# endif

/*
**          OFFSET_TYPE for lint usage.
**          lseek() return and "offset" arg varies on machines,
**          lseek returns a "OFFSET_TYPE" not an int.
*/
/* Now support 64 bit memory access */
#ifdef size_t
#define 	SIZE_TYPE	size_t
#else
#ifdef LP64
#define	SIZE_TYPE	unsigned long
#define	MAX_SIZE_TYPE	0x7fffffffffffffffL
#else
#define	SIZE_TYPE	unsigned int
#define	MAX_SIZE_TYPE	0x7fffffff
#endif
#endif

/* Signed size-of-something.  This should be ssize_t from sys/types.h,
** which we don't include.  I don't have time right now to make that
** happen, so here's another stupid local typedef.
*/
#ifdef ssize_t
# define SSIZE_TYPE     ssize_t
#else
#ifdef LP64
# define SSIZE_TYPE     long
#else
# define SSIZE_TYPE     int
#endif
#endif

/* Now support 64 bit file system access */
#ifdef LARGEFILE64
#ifdef off64_t
#define		OFFSET_TYPE	off64_t
#else
#define		OFFSET_TYPE	longlong
#endif
#define		MAXFS		((longlong)(0x7fffffffffffffff))
#else /* LARGEFILE64 */
#define		MAXFS		MAXI4
#ifdef  off_t
#define		OFFSET_TYPE	off_t
#else
#define		OFFSET_TYPE	long
#endif
#endif /* LARGEFILE64 */


/*}
** Name: CL_SEC_LABEL_DESC - Security Label definition.
**
** Description
**
**      This structure defines a security label for use
**      by the non-cl code.  The operating system specific
**      values are CL_SLABEL_MAX the maximum number of bytes
**      in the operating system internal label, CL_SLEVEL_MAX
**      the maximum number of security levels supported by 
**      this operating system, and CL_ELABEL_MAX the maximum
**      number bytes allowed for the external format of 
**      a security label.  If CL_ELABEL_MAX is greater than 
**      2000 bytes, a row containing a security label that
**      is being displayed would cause the result to be 
**      greater than 2000 bytes limit, therefore this number
**      can vary from operating system to operating system, 
**      but should probably be limited to a resonable number
**      of bytes so that INGRES can display data resonably.
**      I suggest some number < 512.
**
**      The label must contain a level field and a category
**      field.  The level field must be of type i1,i2 or i4 
**      with its value used as an integer variable by the 
**      non-cl code.  The data in security label in all other
**      cases (when level is not explicity referenced) is treated
**      as binary data.
**    
**      For systems that do not have a B1 operating system (or
**      labels) the default values should be as follows:
**      
**	CL_SLABEL_MAX	24
**      CL_SLEVEL_MAX   16
**      CL_ELABEL_MAX   128
**      
**      with 'level' defined as an (i4)
**             
**      
** History:
**     17-mar-89 (sandyh)
**          written
*/
#define		CL_SLABEL_MAX	24   /* Operating system dependent
                                     ** size of label in bytes. */

/*}
** Name: CL_ERR_DESC - CL internal error descriptor.
**
** Description:
**      This type describes errors that occur inside the Compatibility
**	Library.  All implementation-dependent error information should be
**	encapsulated here.  This includes operating system errors and errors
**	that are peculiar to a specific CL implementation.
**
** History:
**     19-mar-86 (jennifer)
**          Created per CL committee request.
**	19-Jun-2008 (jonj)
**	    SIR 120874: Add errfile, errline.
*/
typedef struct
{
	i4	error;
	u_i4	intern;    /* optional error code returned from internal CL 
                           ** routine */
	i2	reserved;  /* reserved for future use as requested by mikem */
	u_i2	callid;    /* integer id of the Unix call that failed */
	i4	errnum;    /* value of errno after failure */
	PTR	errfile;   /* Source file name where error set */
	i4	errline;   /* Source line number within that file */

# define CLE_INFO_ITEMS 3
# define CLE_INFO_MAX   64

    struct
    {
	u_i2	size;  /* byte count; `data' is accessed via PTR */
	union
	{
	   i2          _i2;
	   i4          _i4;
	   i4          _nat;
	   i4          _longnat;
           f4          _f4;
           f8          _f8;
           char        string[CLE_INFO_MAX];
        } data;
    } moreinfo[CLE_INFO_ITEMS]; /* additional information for error reporting*/

}   CL_ERR_DESC;



/* Temporary, for backwards compatibility */

# define CL_SYS_ERR    CL_ERR_DESC

/*
** CL_GET_OS_ERR.   This macro allows code common to UNIX & VMS
** outside the CL, but which accesses the errnum/error field (in
** particular DLF/DMF & DMF/LK) to extract the field contents despite
** differences in the field names for CL_ERR_DESC.
*/
# define CL_GET_OS_ERR(s)       (((CL_ERR_DESC*)(s))->errnum)

/*
** CL_CLEAR_ERR() - initialize a CL_ERR_DESC
**		
**	initializes a CL_ERR_DESC structure to a ``no error''
**	present state.
**
**  Inputs:
**	 err_desc
**		pointer to a CL_ERR_DESC to set.
**  Outputs:
**	 err_desc
**		pointer to a CL_ERR_DESC to set.
**  Returns:
**	 None
**
**  History:
**	19-Jun-2008 (jonj)
**	    SIR 120874: Add errfile, errline, fully clear all members.
*/
# define CL_CLEAR_ERR( s ) \
    (s)->error = (s)->intern = (s)->callid = (s)->errnum = (s)->errline = 0, \
    (s)->errfile = NULL

/* Determine whether an error is a CL error */

# define    CLERROR(x)    (x >= E_CL_MASK && x < E_CL_MASK<<1)

/*
** Set CL_ERR_DESC.  This macro is private to the CL.  Client
** code may do nothing with the CL_ERR_DESC except pass it by reference.
**
**  History:
**	19-Jun-2008 (jonj)
**	    SIR 120874: Add errfile, errline.
*/

# define SETCLERR(s, i, c) \
    { \
        (s)->errline = __LINE__; \
        (s)->errfile = __FILE__; \
        (s)->intern = ((u_i4) i); \
        (s)->callid = ((u_i2) c); \
        (s)->errnum = errno; \
    }

/*}
** Name: CLERRV - This structure is used to return errors from the CL.
**
** Description:
**	This structure is used to return an error and parameters for
**	the error from CL routines.
**	The argv will be sized according to the number of parameters
**	given by errcnt.
**
** History:
**     11-sep-86 (Joe)
**          Updated from 5.0 compat.h
*/
typedef struct cl_err_type
{
	i4	cl_errnum;	/* The error number */
	i4	cl_errcnt;	/* The number of parameters to the error */
	char	*cl_argv[1];	/* The parameters */
} CLERRV;

/*
**  This section defines the what the CL standard error constant 
**  are mapped to.  The error constant masks for all other facilities
**  are defined in ER.H.  The errors for each CL sub-functions should
**  be defined in the sub-function header file, for example DI errors
**  should be defined as follows in DI.H. 
**
**  #define              DI_BADPARAM  (E_CL_MASK + E_DI_MASK + 0x01)
**  #define              DI_BADFILE   (E_CL_MASK + E_DI_MASK + 0x02)
**  #define              DI_ENDFILE   (E_CL_MASK + E_DI_MASK + 0x03)
**       etc.
**
**  In the cl source directory for ER the text file that
**  is used to convert these numbers to text is named:
**  
**     ERcl.txt
**
**  This file needs an entry for each error defined.
*/

/* Major facility error mask for CL. */

#define              E_CL_MASK          0x00010000

/* Error masks for CL sub-functions. */

#define              E_BT_MASK          0x00000100
#define              E_CH_MASK          0x00000200
#define              E_CK_MASK          0x00000300
#define              E_CP_MASK          0x00000400
#define              E_CV_MASK          0x00000500
#define              E_DI_MASK          0x00000600
#define              E_DS_MASK          0x00000700
#define              E_DY_MASK          0x00000800
#define		     E_DL_MASK		E_DY_MASK
					/* Will eventually replace DY */
#define              E_ER_MASK          0x00000900
#define              E_EX_MASK          0x00000A00
#define              E_GV_MASK          0x00000B00
#define              E_ID_MASK          0x00000C00
#define              E_II_MASK          0x00000D00
#define              E_IN_MASK          0x00000E00
#define              E_LG_MASK          0x00000F00
#define              E_LK_MASK          0x00001000
#define              E_LO_MASK          0x00001100
#define              E_ME_MASK          0x00001200
#define              E_MH_MASK          0x00001300
#define		     E_FP_MASK		E_MH_MASK
					/* Will eventually replace MH */
#define              E_NM_MASK          0x00001400
#define              E_OL_MASK          0x00001500
#define              E_PC_MASK          0x00001600
#define              E_PE_MASK          0x00001700
#define              E_QU_MASK          0x00001800
#define              E_SI_MASK          0x00001900
#define              E_SM_MASK          0x00001A00
#define              E_SR_MASK          0x00001B00
#define              E_ST_MASK          0x00001C00
#define              E_SX_MASK          0x00001D00
#define              E_TE_MASK          0x00001E00
#define              E_TM_MASK          0x00001F00
#define              E_TO_MASK          0x00002000
#define              E_TR_MASK          0x00002100
#define              E_UT_MASK          0x00002200

/* New CL sub facilities. */

#define              E_NT_MASK          0x00002300
#define		     E_JF_MASK		0x00002400
#define		     E_CS_MASK		0x00002500
#define		     E_CI_MASK		0x00002600
#define		     E_GC_MASK		0x00002700
#define              E_SD_MASK          0x00002800
#define		     E_SL_MASK		0x00002900
#define              E_CM_MASK          0x00002A00
#define              E_SA_MASK          0x00002B00
#define              E_CX_MASK          0x00002C00

/* Local UNIX CL sub facilities */
#define		     E_HANDY_MASK	0x0000FF00

# endif	    /* COMPAT_INCLUDE */
