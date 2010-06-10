/*
** Copyright (c) 1984, 2008 Ingres Corporation 
*/

/**
** Name:    compat.h -	Compatibility Library Definitions File.
**
** Description:
**	General include file for compatibility library types,
**	common defines for UNIX.
**
** History:
**	12/27/84 (lichtman) -- added type "nat", which should be used
**			for "natural" sized integers
**	3/20/85 (cfr)    -- added MAX_CHAR for largest char in set
**	3/20/85 (cfr) 	 -- added STANDALONE to allow standalone backend
**	26 mar 85 (ejl)	 -- added MIN_CHAR
**	03/21/85 (lichtman) -- added I1MASK and NEG_I1.	 They were in 
**		here before, don't know who clobbered them.
**	26 mar 85 (ejl) -- added MIN_CHAR
**	8/10/85 (peb)	Updated MAX_CHAR to 0377 for 8-bit character
**			support
**	19-mar-1987 (peter)
**		Added E_ codes to UNIX compat.h.
**	03-dec-1987 (mikem)
**		remove definition of STANDALONE (only used in 5.0 and 
**		previous backend.
**	04-dec-1987 (mikem)
**		Added ME_ALIGN_MACRO()
**	08-jul-1988 (roger)
**		Defined UNIX CL_SYS_ERR.
**	20-jul-1988 (roger)
**		Respecified CL_SYS_ERR as CL_ERR_DESC.
**	23-jan-1989 (roger)
**	    Added CLERROR(x) macro (TRUE if x is a CL return error).
**	23-Feb-1989 (fredv)
**	    Removed inclusion of sys/types.h
**	    Defined NULL and NULLFUNC to 0.
**	 7-Jun-1989 (anton)
**	    Definition of BELL moved to cm.h
**	28-Nov-1989 (anton)
**	    BITS_IN macro needs parens. Fixs qrymod bug 8805.
**	22-may-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Commented out strings following #endif;
**		ps2_us5 must declare 'signed char' to have chars act as
**		signed entities;
**		Redefine VOID for compilers which won't assign pointers to
**		VOID functions.
**	23-mat-90 (rog)
**	    Added #endif for one of the additions in the previous change.
**	24-may-90 (blaise)
**	    Removed duplicate #ifdef WSC.
**	Sept-90 (bobm)
**	    CL_OFFSETOF, E_CM_MASK defined.
**	29-Oct-90 (pholman)
**        Introduce security labels for B1 (secure) INGRES.
**      20-Mar-91 (dchan)
**          "VOID" was declared twice causing too many warnings.  Deleted
**          one of the #define's.
**	2/92 (Mike S) Add DL mask
**	20-aug-92 (kwatts)
**	    Added E_SD_MASK (set to 6.4 value).
**	02-dec-92 (kwatts)
**	    E_SD_MASK was not set to 6.4 value in previous change. Had
**	    to move E_CM_MASK to make sure E_SD_MASK did not change.
**	15-Jan-1993 (daveb)
**	    Implement specified CL_CLEAR_ERR from 6.2!
**      13-jan-93 (pearl)
**          Add #define for CL_BUILD_WITH_PROTOS if xDEBUG is defined.
**      16-jul-1993 (rog)
**          Added E_FP_MASK.
**      31-aug-1993 (smc)
**	    Altered ME_ALIGN_MACRO to use SCALARP (which is now defined in
**	    bzarch.h.
**          Undefined abs() macro for axp_osf, moved include of bzarch.h
**          to before the conditional definition of abs().
**          Dummied out CL_OFFSET cast when linting to avoid a plethora of
**	    lint truncation warnings which can safely be ignored.
**	13-dec-1993 (nrb)
**	    Bug #58891
**	    Undefined abs() macro for dr6_us5.
**	3-feb-94 (stephenb)
**	    Added definition for E_SA_MASK for new sub-library SA.
**	25-apr-95 (hanch04)
**	    Undefined abs for su4_us5
**	25-apr-95 (smiba01)
**	    Added support for dr6_ues.
**     18-jul-95 (canor01)
**	  include <errno.h> for definition of errno
**	14-sep-1995 (sweeney)
**	    remove obsolete CLERRV. Remove history so old that nothing it
**	    refers to remains in this file.
**	14-sep-1995 (sweeney)
**          WECOV2 is obsolete. What was WECO anyway?
**	16-nov_1995 (murf)
**		Added the undefine for abs() macro for sui_us5 as described
**		in front!frame!valid!grammar.y by Tony Sweeney.
**	18-apr-1997 (muhpa01)
**          Undefined abs for hp8_us5.  Also, added include for pthread.h when
**	    POSIX_DRAFT_4 && hp8_us5 as this is needed to include "cma_"
**	    wrapper functions for threads implementation on HP-UX.
**      20-Jun-1997 (radve01)
**          CONCAT: concatination macro added
**      21-may-1997 (canor01)
**          Add default define for CL_PROTOTYPED.  Add definition of
**          FUNC_EXTERN for use with C++.
**	21-feb-1997 (walro03)
**		Reapply bonro01's OpIng 1.2/01 change of 09-dec-1996:
**		undefine abs() macro for ris_us5 because it's already defined
**		in /usr/include/stdlib.h.
**	22-sep-1997 (matbe01)
**	    Added nc4_us5 (NCR) to the undefine for abs() macro.
**	02-dec-1997 (walro03)
**		AIX (ris_us5) "char" is unsigned.  Force "i1" to signed char.
**	07-Nov-1997 (hanch04)
**	    Added MAXFS for MAXimum Files Size
**          Added LARGEFILE64 for files > 2gig
**          Added OFFSET_TYPE, longlong
**	06-Mar-1998 (kosma01)
**	    upgradedb results in E_DM93A7 for table iirelation.
**	    fix is to keep i1 char signed on AIX 4.x (rs4_us5).
**      18-feb-1998 (yeema01)
**          Modified for OpenRoad so that this header is consistent
**          with the NT version. Modification includes adding 
**          typdef for iP for OR40 Unix Port and adding the 
**          define of memory allocator for OpenRoad.
**	10-mar-1998 (fucch01)
**	    Added ifdef's for sgi to use different STRUCT_ASSIGN_MACRO that
**	    uses MEcopy.  Straight structure assignment was failing w/ SIGBUS
**	    error for certain structures.
**	29-sep-1998 (matbe01)
**	    Added defines for NCR (nc4_us5) since the stdio.h include uses
**	    double underscores for _cnt, _ptr, and _base.
**	03-Dec-1998 (muhpa01)
**	    Removed code for POSIX_DRAFT_4 for HP - obsolete
**	03-mar-1999 (walro03)
**	    Force 'longlong' to 'long long' for Sequent (sqs_ptx).
**	15-mar-1999 (popri01)
**	    For Unixware 7 (usl_us5), add LARGE FILE support.
**	22-Mar-1999 (bonro01)
**	    Most platforms seem to support the long long datatype, so
**	    longlong will default to 'long long' instead of 'long'.
**	    This will be the default instead of the exception.
**	    Code in iduuid.c relies on the fact that a longlong datatype
**	    is actually 8 bytes.
**	14-apr-1999 (somsa01)
**	    In the case of MAINWIN, don't define 'bool'.
**	10-may-1999 (walro03)
**	    Remove obsolete version string arx_us5, dr6_ues, ps2_us5.
**      03-jul-1999 (podni01)
**          Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**	11-Nov-1999 (jenjo02)
**	    Changed CL_CLEAR_ERR to manually clear the structure instead of
**	    using SETCLERR, which causes a kernel call to extract errno.
**	13-jan-2000 (cucjo01)
**	    When MAINWIN is used, we need to define bool (lower case)
**	    ONLY when compiling *.c files.  If we are compiling *.cpp
**	    files, then we can NOT define bool since it already defined.
**	21-jan-2000 (hweho01)
**          Added changes for ris_u64 :
**          1) undefine abs() macro.
**          2) force i1 to signed char.
**	 1-mar-2001 (mosjo01)
**          Conflict with limits.h MAX_CHAR on (sqs_ptx) and overriding 
**          compat.h definition. Best to give it a new name (IIMAX_CHAR_VALUE)
**          as it is only referenced by half a dozen pgms or so. 
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	06-feb-2001 (devjo01)
**	    Add CX ('C'luster e'X'tensions) sub-facility status mask.
**	    Add CL_GET_OS_ERR macro to hide VMS/UNIX diffs in CL_ERR_DESC.
**      24-apr-2000 (wansh01)
**          Undefined abs for sgi_us5.
**      27-Nov-2000 (linke01)
**          Undefined abs() macro for usl_us5
**      26-Jun-2001 (linke01)
**          when submitted change to piccolo, it was propagated incorrectly,
**          re-submit the change. (this is to modify change#451142).
**      01-jun-2001 (peeje01)
**          Whenever C++ files being compiled do not define bool.
**      22-Jun-2000 (linke01)
**          Undefined abs() macro for usl_us5     
**	28-jun-2001 (gupsh01)
**	    Moved the define for UCS2 from iicommon to here.
**      20-jul-2001 (fanra01)
**          Sir 130694
**	    Add type definitions for simple unicode types.
**	19-jul-2001 (stephenb)
**	    Don't define abs for i64_aix
**	16-aug-2001 (toumi01)
**	    for LP64 platforms make SIZE_TYPE default to unsigned long
**	30-oct-2001 (devjo01)
**	    Add MAXI8 and MINI8 for 64 bit platforms only.
**	08-Nov-2001 (inifa01) INGSRV 1590 bug 106318
**	    IMA tables return no rows because of datatypes size mismatch
**	    on 64 bit platform.
**	    Added new datatype long_int of type long.
**      04-Feb-2002 (fanra01)
**          Bug 107016
**          Add macros for obtaining the item count from an octet length and
**          obtaining octet length from an item count.
**	05-mar-2002 (devjo01)
**	    MAXFS is 2^63-1 for axp_osf, even though LARGEFILE64 is
**	    undefined for this platform.
**	20-Sep_2002 (inifa01)
**	    Removed long_int definition. Used longlong instead.
**	07-Oct-2002 (hanch04)
**	    Added MAX_SIZE to be the largest integer in a long.
**	17-Oct-2002 (bonro01)
**	    Changed STRUCT_ASSIGN_MACRO to use memcpy instead of MEcopy
**	    because the macro is now being used outside of the CL code
**	    and MEcopy cannot be resolved.
**	01-Nov-2002 (hweho01)
**	    Renamed MAX_SIZE to MAX_SIZE_TYPE, avoid conflicts with 
**          the existing definitions of MAX_SIZE in the source files 
**          of fe/rep/repmgr.
**	06-Jan-2003 (hanje04)
**	    Added AMD x86-64 Linux (a64_lnx) to platforms not defining abs
**	10-Mar-2003 (hanje04)
**	    Include signal.h here for Linux so that it is not included thru
**	    pthread.h which stop the signal being correctly defined.
**	04-Apr-2003 (hanje04)
**	    Only include signal.h here if where building on GLIBC 2.2 or
**	    higher
**	27-Jan-2003 (hanje04)
**	    BUG 109541
**	    Use i1 instead of bool in C++ on S/390 Linux. The native
**	    C++ bool is 4 bytes and is causing problems in programs which use
**	    C and C++ because the Ingres C bool datatype is 1 byte.
**	    NOTE: THIS IS A TEMPORARY WORK AROUND TO AVOID COMPLETELY REBULDING
**	    SHOULD BE REMOVED FOR SP2
**	24-jun-2003 (somsa01)
**	    For LP64, added i8 and u_i8.
**	7-Aug-2003 (bonro01)
**	    Bug 110556
**	    Updated MAXFS for i64_hpu to support >2Gb logs and database.
**	23-Sep-2003 (hanje04)
**	    Don't define abs() for any Linux port.
**	03-Oct-2003 (hanje04)
**	    Removed workaround got BUG 109541 and corectly defined bool to
**	    be a 4byte integer.
**	2-Jan-2004 (schka24)
**	    Define i8 for all platforms.
**	13-may-04 (hayke02)
**	    Add test for (_FILE_OFFSET_BITS == 64) for dgi.us5 when we are
**	    #defining OFFSET_TYPE and MAXFS. off_t is typedef'ed to int64_t
**	    but will not be #define'd, so OFFSET_TYPE was being #define'd to
**	    long when it should be longlong/int64_t. This change fixes bug
**	    112294, problem INGSRV 2822.
**	14-May-2004 (schka24)
**	    C typing quirk makes MINI4 useless for i8's, define MINI4LL
**	04-Oct-2004 (bonro01)
**	    Remove header file stucture.
**	21-Feb-2005 (thaju02)
**	    Added _size_type to CL_ERR_DESC moreinfo.
**      24-Feb-2005 (gorvi01)
**          Added SSIZE_TYPE to allow signed parameters to be handled
**          correctly from the crs files.
**      02-Mar-2005 (gorvi01)
**          Removed SSIZE_TYPE definition. 
**	21-Feb-2005 (thaju02)
**	    Added _size_type to CL_ERR_DESC moreinfo.
**	10-Dec-2004 (bonro01)
**	    Add a64_sol to platforms that don't define abs()
**	23-Jun-2005 (schka24)
**	    Add SSIZE_TYPE back in.  Bletch.  We ought to go back to
**	    including sys/types.h, which should be rather better standardized
**	    by now (it's 16 years since fred removed it, above.)
**	22-Apr-2005 (hanje04)
**	    Add support for Mac OS X
**	    Replace mutli-platform #ifdef for abs with xCL_ABS_EXISTS which
**	    will be defined in bzarch.h if it does.
**	31-jul-2006 (zhayu01) Unicode Support II
**	    Added define for IIOMAllocStringW.
**	21-Nov-2006 (kschendel)
**	    Zowie!  NULL has been improperly defined for the last 17 years.
**	    (ok, out of context a bit.)  Most of the time, stddef.h gets
**	    included (AFTER compat.h, by another system header), and NULL
**	    gets defined back to void* 0.  Make it be that all the time.
**	14-feb-2007 (dougi)
**	    Add CL_MAX_DECPREC to parameterize decimal precision limits.
**	14-Aug-2007 (drivi01)
**	    Add E_EP_MASK for a new facility.
**	25-Oct-200 (hanje04)
**	    BUG 114907
**	    Define bool to be enum type for Moc OS X to match the builtin defn
**	    for C++. Otherwise we get problems using extern C bool types in
**	    C++. Also make sure SIZE_T_MAX is defined.
**	26-Dec-2007 (kiria01) SIR119658
**	    Define CL_LIMIT_DECVALF8 - the lowest number bigger than all
**	    representable decimals.
**	14-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with generic OSX
**	28-apr-2008 (joea)
**	    Remove #define for NULL and depend instead on stddef.h.
**	07-Oct-2008 (smeke01) b120907
**	    Added typenames for platform-specific OS thread id types. 
**	    Currently defined to be the existing types for LNX, 
**	    su4.us5, su9.us5, rs4.us5 and r64.us5. On platforms where such 
**	    types don't exist, use the safe dummy type int. Also added 
**	    define for max u_i8 digit width.
**	27-Nov-2008 (smeke01) b121287
**	    Moved typenames for platform-specific OS thread id types to 
**	    their proper home in csnormal.h, amending them to capitals and 
**	    typedefs at the same time.
**      12-dec-2008 (joea)
**          Replace BYTE_SWAP by BIG/LITTLE_ENDIAN_INT.
**	11-May-2009 (kschendel) b122041
**	    Revise CL_OFFSETOF to use standard offsetof() if available.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**	9-Nov-2009 (kschendel) SIR 122757
**	    Add more casting to ME_ALIGN_MACRO to prevent pointer truncation.
**      26-Nov-2009 (hanal04) Bug 122938
**          Added CL_MAX_DEC_PREC_39 and CL_MAX_DEC_PREC_31 defines.
**          Different GCA protocol levels can handle different levels
**          of decimal precision.
**       01-Dec-2009 (coomi01) b122980
**          Flag use of a C99 compiler/library.
**	09-Feb-2010 (smeke01) b123226, b113797 
**	    Added define for max i8 digit width.
*/

# ifndef	COMPAT_INCLUDE

# define	COMPAT_INCLUDE

/*
** This picks up machine dependant stuff.  It is created  by
** mkbzarch.sh, which looks around and figures things out.
*/

#ifdef MAINWIN
#include <windows.h>
#endif

#include <bzarch.h>

#ifdef xde
#include <windows.h>
#include <io.h>
#endif

/* Pick up standard C definitions: size_t, NULL, offsetof, etc */
#include <stddef.h>

/* errno is used below, but can be defined as many things */
# include	<errno.h>
# include 	<stdlib.h>

#if defined(__GLIBC__) && __GLIBC__ >= 2 && __GLIBC_MINOR__ > 1
/* Flag use of C99 compatible library */ 
#define C99_COMPATIBLE_LIBRARY TRUE
#include <signal.h>
#endif

/*
** Per the coding standard, function prototypes must be
** included unless specifically turned off.
*/
# ifndef _NO_PROTOS
# define CL_PROTOTYPED
# define CL_HAS_PROTOS /* replaced by CL_PROTOTYPED, but lingers */
# endif /* _NO_PROTOS */

/* we should not include sys/types.h in compat.h (fredv) */

/*	SIMPLE TYPES			*/

# if defined(any_aix)
typedef  signed char    i1;
# else
typedef		char	i1;	/* this should be signed, if possible */
# endif
typedef		short	i2;
typedef		int	i4;	/* might be long on some machines */
/* Note - on Windows this might have to be declared as __int64 */
# ifdef LP64
typedef		long	i8;
#else
typedef		long long i8;
# endif  /* LP64 */
typedef		float	f4;
typedef		double	f8;

#if defined(ibm_lnx) && !defined(__cplusplus)
typedef	int	bool; /* need 4 byte bool on S390 Linux */
#elif defined(OSX)
# include <stdbool.h>
#elif (!defined(MAINWIN) && !defined(__cplusplus)) || \
     (defined(MAINWIN) && !defined(__cplusplus)) 
typedef         char    bool;
#endif /* !MAINWIN (MAINWIN !__cplusplus) */

typedef		unsigned char	u_i1;	/* careful; some systems need "char" */
typedef		unsigned short	u_i2;
typedef		unsigned int	u_i4;
# ifdef LP64
typedef		unsigned long	u_i8;
#else
typedef		unsigned long long u_i8;
# endif  /* LP64 */

/*
** Unicode character types
*/
typedef unsigned short  ucs2;
typedef unsigned long   ucs4;
typedef unsigned char   utf8;

/*
** Internal representation of wide characters
*/
typedef ucs2            g_char;

typedef 	u_i2    	UCS2;	/* This is the Unicode character 
					** data type, ie. unicode code point 
					** upto 2 bytes.
				        */
 
# define        nat       DONT_USE_NAT
# define        u_nat     DONT_USE_UNAT
# define        longnat   DONT_USE_LONGNAT
# define        u_longnat DONT_USE_ULONGNAT

/* These are not typedefs because they are in some sys/types.h files
   but not in others.  Defines work either way. (daveb) 
   Some defines cause problems with sys/stat.h on Mac OS X */

# define	uchar	unsigned char
# ifndef OSX
# define	u_char	unsigned char
# define	ushort	unsigned short
# define	u_short	unsigned short
# define	ulong	unsigned long
# define	u_long	unsigned long
# endif

/* On NCR 3.01, the stdio.h include uses double underscores for these */

# if defined(nc4_us5) && defined(conf_INGRESII)
# define        _cnt    __cnt
# define        _ptr    __ptr
# define        _base   __base
# endif /* nc4_us5 && conf_INGRESII */
   
/* The following typedef(s) enable OpenROAD 40 Unix Port  */

typedef         int     iP;

# define	longlong   long long

/* 	COMPLEX TYPES			*/

/*}
** Name:    PTR -	Generic Pointer Type.
**
** Description:
**	A pointer type large enough to hold any other pointer type.  Usually
**	returned by a memory allocator routine or used as a formal parameter
**	for routines that accept a variable number of arguments of various
**	pointer types.
**
**	Pointer variables of any type can be freely assigned to and from PTR
**	variables.  However, it is the responsibility of the engineer to
**	ensure that all assigments from PTR variables have taken into account
**	any possible alignment problems for the base type of the pointer 
**	variable to which the assignment is being made.
**
**	Note:  PTR variables cannot under any circumstances be used directly
**	in an indirection (without appropriate casting.)  Neither can PTR
**	variables have any arithmetic operations performed on them directly,
**	but must always be cast to a specific pointer type.  In particular,
**	this means that the only legal assignment operator for PTR variables
**	is `=' (simple assignment,) `+=', etc. being disallowed.
**
**	For example, these are legal PTR assignments:
**
**	    PTR alloc;
**
**	    alloc = (PTR)((char *)alloc + sizeof(type));
**	    alloc = (PTR)((type *)alloc + 1);			(1)
**
**	Two warnings are produced by Lint when PTR variables are used:
**
**	    warning: possible pointer alignment problem
**	    warning: illegal pointer combination
**
**	The latter is produced whenever an assignment is made from or to
**	a PTR variable and can be eliminated by appropriately casting the
**	assignment (except when the `-c' flag was passed to Lint.)  For
**	example:
**
**	    double *dp;
**
**	    dp = (double *)alloc;
**	    alloc = (PTR)dp;					(2)
**
**	The former is produced whenever a PTR variable is cast to another
**	pointer type and cannot be eliminated.  (For example, (1) and (2)
**	above.)  The engineer should verify that alignment considerations
**	have been accounted for in these cases.
**
**	(schka24) Much of the above commentary is misleading.  The RTI
**	gang screwed up, and failed to distinguish between a generic
**	POINTER, which should be a void *;  and a byte-address pointer
**	e.g. BYTEPTR, which should be a char *.  One can do address
**	arithmetic on the latter, but not the former.  Unfortunately,
**	the PTR type was defined as a char * and people used it to do
**	byte-address arithmetic.
**	So, define a true generic pointer which can't be interpreted
**	or dereferenced;  it's just a pointer typed-and-sized placeholder.
*/
typedef		char *		PTR;		/* Byte-address pointer */
/* No typedef for the time being, joea suggests VOIDPTR
typedef		void *		POINTER;	** Pointer-to-unknown **
*/

/*	MISCELLANY			*/

# define		BITFLD		u_i4

# ifdef WSC
# define		_VOID_
# else
# define		_VOID_		(void)
# endif



/*	STATUS CODES			*/

# define	STATUS		 i4

# define	OK		   0
# define	FAIL		   1		/* all purpose fail */
# define	SUCCEED		   0


/*	system status codes		*/
# define	ENDFILE		9900		/* read/write past EOF */

/* the following four #defines are OBSOLETE or not in the CL Spec */
# define	NULLVAL		((i4)0)	/* null value */
# define	NULLPTR		((PTR)0)	/* null pointer */
# define	NULLCHAR	('\0')		/* string terminator */
# define	NULLFILE	((FILE *)0)	/* null file ptr */

# define	EOS		('\0')		/* string terminator */

# ifndef	FALSE
# define	FALSE		0
# endif

# ifndef	TRUE
# define	TRUE		1
# endif

/* The following two #defines are OBSOLETE - see CL Spec */
/* Now that we support 8-bit characters, this is always 0377 peb 8/10/85 */
# define	IIMAX_CHAR_VALUE	0377
/* The smallest legal printable char, ie. it's not 0 'cause 0 isn't a char */
# define	MIN_CHAR	' '	

# define	MAXI1		0x7f
# define	MINI1		(-MAXI1 - 1)
# define	MAXI2		0x7fff
# define	MINI2		(-MAXI2 - 1)
# define	MAXI4		0x7fffffff
# define	MINI4		(-MAXI4 - 1)
/* A C typing quirk makes MINI4 truly the same as 0x80000000, meaning that
** if you use MINI4 in an i8 context you get *positive* 2147483648, not
** negative.  Define a MINI4 to be used in an i8 context.
** Example pseudocode:
**   i8 thing;  WRONG: if (thing between MINI4 AND MAXI4) fits in i4;
**		RIGHT: if (thing between MINI4LL AND MAXI4) fits in i4;
** (You can use MINI4LL in an i4 context as well, and it should work,
** but I'm not brave enough to simply redefine MINI4!  I wouldn't want
** to cause any unexpected type coercions.)
*/
#ifdef LP64
# define	MINI4LL		(-2147483648L)
#else
# define	MINI4LL		(-2147483648LL)
#endif

# define	FMIN		1E-37
# define	FMAX		1E37

# define	I1MASK		0377
# define	MIN_UCS2	0
# define	MAX_UCS2	0xffff
# define	MAX_UCS4	0x7fffffffUL

# ifdef		LP64		/* 64 bit platform */
# ifdef		MAXI64		/* Get val from vendor header */
# define	MAXI8		MAXI64
# else
# define	MAXI8		0x7fffffffffffffffL
# endif		
# ifdef		MINI64
# define	MINI8		MINI64
# else
# define 	MINI8		((-MAXI8) - 1)
# endif

#else /*LP64*/

/* LPI32 platforms (probably!) need the LL suffix instead of just L */
#define		MAXI8		0x7fffffffffffffffLL
#define		MINI8		((-MAXI8) - 1)

# endif		/*LP64*/

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
# define CL_MAX_DECPREC	CL_MAX_DECPREC_39 /* Highest supported precision */
   /* Define the lowest number that is bigger than all
   ** representable decimals. Floats whose magnitude is greater
   ** or equal to this cannot be represented in decimal
   ** datatype. It should be 1.0e39 (1 bigger than 39x9s) but
   ** F8 precision is means that the practical limit is more like
   ** 9.99999999999999e38
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
#if defined(size_t)
# define SIZE_TYPE	size_t
# elif defined(OSX)
/* size_t doesn't satisfy ifdef on Mac OSX */
# include <sys/types.h>
# ifndef SIZE_T_MAX
# include <limits.h>
# endif
# define SIZE_TYPE size_t
# define MAX_SIZE_TYPE SIZE_T_MAX
#else
#ifdef LP64
# define SIZE_TYPE	unsigned long
# define MAX_SIZE_TYPE	0x7fffffffffffffffL
#else
# define SIZE_TYPE	unsigned int
# define MAX_SIZE_TYPE	0x7fffffff
#endif
#endif

/* Signed size-of-something.  This should be ssize_t from sys/types.h,
** which we don't include.  I don't have time right now to make that
** happen, so here's another stupid local typedef.
*/
#ifdef ssize_t
# define SSIZE_TYPE	ssize_t
#else
#ifdef LP64
# define SSIZE_TYPE	long
#else
# define SSIZE_TYPE	int
#endif
#endif


/* Now support 64 bit file system access */
#if (_FILE_OFFSET_BITS == 64) || defined(LARGEFILE64)
#ifdef  off64_t
# define OFFSET_TYPE     off64_t
#else
# define OFFSET_TYPE     longlong
#endif
# define	MAXFS	((longlong)(0x7fffffffffffffff))
#else /* LARGEFILE64 */
#ifdef  off_t
# define OFFSET_TYPE     off_t
#else
# define OFFSET_TYPE     long
#endif
#if defined(i64_hpu)
# define	MAXFS	LONG_MAX
#elif defined(axp_osf)
# define	MAXFS	0x7fffffffffffffffl
#else
# define        MAXFS	MAXI4
#endif
#endif /* LARGEFILE64 */

/*
**  Special macro definitions
*/

# ifndef min
# ifndef xCL_ABS_EXISTS
# define	abs(x)			(((x) < 0) ? -(x) : (x))
# endif /* xCL_ABS_EXISTS */
# define	min(x,y)		((x) < (y) ? (x) : (y))
# define	max(x,y)		((x) > (y) ? (x) : (y))
# define	maskset(bit,word)	(word |= bit)
# define	maskclr(bit,word)	(word &= ~bit)
# define	masktest(bit,word)	((bit) & (word))
# endif  /* min */

# define        CAT(x, y)       x ## y
# define        CONCAT(x, y)    CAT(x,y)

/*
** Valid for all Unix compilers.  Not so Whitesmiths on IBM.
** I am not going to imitate the "#ifdef WSC" above;  this
** header is not supposed to be shared.
*/
#ifdef sgi_us5
# define        STRUCT_ASSIGN_MACRO(src, dest)  memcpy((PTR)&dest, (PTR)&src, sizeof(dest))
#else
# define        STRUCT_ASSIGN_MACRO(src, dest)  (dest = src)
#endif /* sgi_us5 */

/*
** Macros for returning item or octet counts.
*/
# define II_COUNT_ITEMS(x, type)	(x / sizeof(type))
# define II_COUNT_OCTETS(x, type)	(x * sizeof(type))


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
**	20-jul-1988 (roger)
**            Defined this structure for UNIX. CL routines that take a parameter
**          of type CL_ERR_DESC* should set the `errno' and `callid' fields
**	    if an error occurs as the result of a UNIX system or library call.
**	      `Intern' may be used to pass an error code that is not part of
**	    a CL routine's published interface (i.e., is not a documented
**	    return code) to the error reporting system.  This will usually
**	    be the return code from an internal (implementation-specific)
**	    CL routine.
**	      The SETCLERR() macro should be used to set these three mandatory
**	    fields.  It gets the value of errno from the Unix global variable.
**	    `Intern' and `callid' should be set to 0 if not otherwise being set.
**	      `Moreinfo' may be used to pass implementation-specific parameters
**	    to the error reporting system.  Up to CLE_INFO_ITEMS of data
**	    can be passed; data should be stored in the appropriately typed
**	    union member and a byte count (<= CLE_INFO_MAX) placed in `size'.
**	21-Feb-2005 (thaju02)
**	    Added _size_type to CL_ERR_DESC moreinfo.
**	19-Jun-2008 (jonj)
**	    SIR 120874: Add errfile, errline.
*/
typedef struct
{
    u_i4    intern;    /* optional error code returned from internal CL routine */
    i2	    reserved;  /* reserved for future use as requested by mikem */
    u_i2    callid;    /* integer id of the Unix call that failed */
    i4      errnum;    /* value of errno after failure */
    PTR     errfile;   /* Source file name where error set */
    i4	    errline;   /* Source line number within that file */

# define CLE_INFO_ITEMS	3
# define CLE_INFO_MAX	64

    struct
    {
	u_i2 size;	/* byte count; `data' is accessed via PTR */
	union
	{
	    i2    	_i2;
	    i4    	_i4;
	    i4		_nat;
	    i4		_longnat;
	    SIZE_TYPE	_size_type;
	    f4   	_f4;
	    f8   	_f8;
	    char	string[CLE_INFO_MAX];
	} data;
    } moreinfo[CLE_INFO_ITEMS]; /* additional information for error reporting */

}    CL_ERR_DESC;

/*
** Set CL_ERR_DESC.  This macro is private to the CL.  Client
** code may do nothing with the CL_ERR_DESC except pass it by reference.
*/
# define SETCLERR(s, i, c) \
    { \
	(s)->errline = __LINE__; \
	(s)->errfile = __FILE__; \
	(s)->intern = ((u_i4) i); \
	(s)->callid = ((u_i2) c); \
	(s)->errnum = errno; \
    }

/*
** CL_GET_OS_ERR.   This macro allows code common to UNIX & VMS
** outside the CL, but which accesses the errnum/error field (in
** particular DLF/DMF & DMF/LK) to extract the field contents despite
** differences in the field names for CL_ERR_DESC.
*/
# define CL_GET_OS_ERR(s)	(((CL_ERR_DESC*)(s))->errnum)

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
** History:
**	11-Nov-1999 (jenjo02)
**	    Changed CL_CLEAR_ERR to manually clear the structure instead of
**	    using SETCLERR, which causes a kernel call to extract errno.
**	19-Jun-2008 (jonj)
**	    SIR 120874: Also init errfile, errline members.
*/
# define CL_CLEAR_ERR( s ) \
    (s)->intern = (s)->callid = (s)->errnum = (s)->errline = 0, \
    (s)->errfile = NULL

/* Temporary, for backwards compatibility */

# define CL_SYS_ERR    CL_ERR_DESC

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

/* Determine whether an error is a CL error */

# define    CLERROR(x)    (x >= E_CL_MASK && x < E_CL_MASK<<1)

/* Error masks for CL sub-functions. */

#define              E_BT_MASK          0x00000100
#define              E_CH_MASK          0x00000200
#define              E_CK_MASK          0x00000300
#define              E_CP_MASK          0x00000400
#define              E_CV_MASK          0x00000500
#define              E_DI_MASK          0x00000600
#define              E_DS_MASK          0x00000700
#define              E_DY_MASK          0x00000800
#define              E_DL_MASK          E_DY_MASK
					/* This will eventually replace DY */
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
#define              E_FP_MASK          E_MH_MASK
					/* This will eventually replace MH */
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
#define              E_CI_MASK          0x00002600
#define              E_GC_MASK          0x00002700
#define              E_SD_MASK          0x00002800
#define              E_SL_MASK          0x00002900
#define              E_CM_MASK          0x00002A00
#define		     E_SA_MASK		0x00002B00
#define		     E_CX_MASK		0x00002C00
#define		     E_EP_MASK		0x00002D00

/* Local UNIX CL sub facilities */
#define              E_BS_MASK          0x0000FE00
#define		     E_HANDY_MASK	0x0000FF00

/* 
** This might be defined better in bzarch.h.  Then again, it might
** be obsolete (daveb).
*/ 
# ifndef	NEG_I1
# define	NEG_I1(a)	((a) & 0x80)
# endif

# define	BITS_IN(arg)	(BITSPERBYTE * sizeof(arg))

# ifdef BIG_ENDIAN_INT
# define	CV_C_CONST_MACRO(a, b, c, d)	\
				(i4)((((i4)(a) & 0xff) << 24) |\
					  (((i4)(b) & 0xff) << 16) |\
					  (((i4)(c) & 0xff) <<  8) |\
					  (((i4)(d) & 0xff) <<  0))
# else
# define	CV_C_CONST_MACRO(a, b, c, d)	\
				(i4)((((i4)(d) & 0xff) << 24) |\
					  (((i4)(c) & 0xff) << 16) |\
					  (((i4)(b) & 0xff) <<  8) |\
					  (((i4)(a) & 0xff) <<  0))
# endif
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
#define		    ME_ALIGN_MACRO(ptr, N)	\
		    ((PTR) ((SCALARP)((char *) ptr + (N - 1)) & ~((SCALARP)(N) - 1)))

# ifdef VMS
# define	FUNC_EXTERN
# else
# ifdef __cplusplus
# define        FUNC_EXTERN     extern "C"
# else /* __cplusplus */
# define        FUNC_EXTERN     extern
# endif /* __cplusplus */
# endif

# ifdef NO_VOID_ASSIGN
# define                VOID            i4
# else
# define                VOID            void
# endif

/*
** Offset in bytes of member m_name in struct s_type, as a u_i4.
** Use standard offsetof() if available (it should be), otherwise use the
** usual trick of casting zero to the required struct * type.
*/
# if defined(offsetof)
# define    CL_OFFSETOF(s_type,m_name) ((u_i4) (offsetof(s_type,m_name)))
# else
# define    CL_OFFSETOF(s_type,m_name) ((u_i4) \
		((char *)&((s_type *) 0)->m_name - (char *)0 ))
# endif

/*}
** Name: CL_SEC_LABEL_DESC - Security Label definition.
**
** Description
**
**      This structure defines a security label for use
**      by the non-cl code.  The operating system specific
**      values are CL_SLABLE_MAX the maximum number of bytes
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
** History:
**	8-mar-90 (pholman)
**	    Re-integrated back into compat.h.  (Had originally been
**	    written by sandyh, 17-mar-89.)
*/
#define                 CL_SLABEL_MAX   24  /* Operating system dependent	
                                            ** size of label in bytes. */
 
# endif		/* COMPAT_INCLUDE */

/*  4/13/98 Added the following for OR40 Unix port */

#define IIOMAllocString(str, session) MEORallocString(str)
#define IIOMAllocStringW(str, session) MEORallocStringW(str)
#define IIOMAllocBlock(size, clear, session) MEORreqmem(0, size, clear, NULL)
#define IIOMAllocLongBlock(size, clear, session)  \
          MEORreqlng(0, size, clear, NULL)
#define IIOMFreeBlock(ptr, session) MEORfree(ptr)
#define IIOMAllocArray(num_elts, elt_size, clear, session) \
          MEORreqlng((u_i4)0, ((u_i4)num_elts)*((u_i4)elt_size), \
          (bool)clear, (STATUS *)NULL)
#define IIOMBlockSize(block, session, size) MEORsize(block,size)
 
 

