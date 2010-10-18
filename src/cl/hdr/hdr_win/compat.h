/*
** Copyright (c) 1985, 2008 Ingres Corporation
**
*/

/*
** Name:    compat.h -	Compatibility Library Definitions File.
**
** Description:
**	General include file for compatibility library types,
**	common defines for UNIX.
**
** History:
**	18-jul-95 (reijo01)
**	    Changed SETWIN32ERR so that it will populate the CL_ERR_DESC with
**		the proper values.
**	24-sep-95 (tutto01)
**	    Added defines for new TE and DL32 facilities.
**	10-dec-1996 (donc)
**	    Added defines for iP, LOADDS, DYNALINK and HUGEPTR.  These
**	    are OpenROAD specific defines.
**	11-dec-1996 (donc)
**	    Added OpenROAD Object management definitions for ME.
**	26-feb-1997 (donc)
**	    Have OpenROAD string allocation use its own MEORallocString.
**      21-may-1997 (canor01)
**          Add definition for CL_PROTOTYPED.  Add definition for
**          FUNC_EXTERN for use by C++.
**	04-jun-1997 (tchdi01)
**	    ifdefed out definition of bool for C++ compilers
**	    bool is a builtin type in ANSI C++
**      02-jun-97 (mcgem01)
**          The max command line length has been bumped to 14500 for
**          Jasmine.
**	04-aug-1997 (canor01) - xinteg by lunbr01 from main for ODBC MTS
**	    Restore definition of FUNC_EXTERN for C++.
**	19-jan-1998 (canor01) - xinteg by lunbr01 from main for ODBC MTS
**	    Redefine PTR to IIPTR to prevent conflicts with MFC.
**      07-Nov-1997 (hanch04)
**          Added MAXFS for MAXimum Files Size
**          Added OFFSET_TYPE, longlong
**	04-aug-1997 (canor01)
**	    Restore definition of FUNC_EXTERN for C++.
**	19-jan-1998 (canor01)
**	    Redefine PTR to IIPTR to prevent conflicts with MFC.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	19-nov-1999 (mcgem01)
**	    Support for 64 bit file addressing should be keyed off
**	    LARGEFILE64 rather than INTEGRAL_MAX_BITS.
**      1-mar-2001 (mosjo01)
**          Confilct with limits.h MAX_CHAR on (sqs_ptx) and overriding
**          compat.h definition. Best to give it a new name (IIMAX_CHAR_VALUE)
**          as it is only referenced by half a dozen pgms or so.
**	06-dec-1999 (somsa01)
**	    Added define for SIZE_TYPE.
**	08-feb-2001 (somsa01)
**	    Modified the checking of size_t for IA64.
**	28-jun-2001 (gupsh01)
**	    Moved the define for UCS2 from iicommon to here. 
**      28-Jun-2001 (fanra01)
**          Sir 103694
**          Add unicode types to distinguish them from regular types.
**	27-aug-2001 (somsa01 for mosjo01)
**	    Conflict with limits.h MAX_CHAR on (sqs_ptx) and overriding
**	    compat.h definition. Best to give it a new name
**	    (IIMAX_CHAR_VALUE) as it is only referenced by half a dozen
**	    pgms or so.
**	02-nov-2001 (somsa01 for devjo01)
**	    Add MAXI8 and MINI8 for 64 bit platforms only.
**      04-Feb-2002 (fanra01)
**          Bug 107016
**          Add macros for obtaining the item count from an octet length and
**          obtainin octet length from an item count.
**	29-may-2002 (somsa01 for inifa01) INGSRV 1590 bug 106318
**	    IMA tables return no rows because of datatypes size mismatch
**	    on 64 bit platform.
**	    Added new datatype long_int of type long.
**	24-Sep-2002 (inifa01) 
**	    Removed long_int definition. Used longlong instead.
**	22-oct-2002 (somsa01)
**	    Added MAX_SIZE to be the largest integer in a long.
**	04-nov-2002 (somsa01)
**	    Changed MAX_SIZE to MAX_SIZE_TYPE.
**	24-apr-2003 (somsa01)
**	    For LP64, added i8 and u_i8.
**	09-Dec-2003 (fanra01)
**	    Add the E_CX_MASK for clustering facility.
**	02-Jan-2004 (schka24)
**	    Hand integration from unix.
**	    Define i8 for all platforms.
**	26-jan-2004 (somsa01)
**	    Changed CL_CLEAR_ERR to run CLEAR_ERR.
**	27-jan-2004 (penga03)
**	    Added CL_GET_OS_ERR.
**	14-may-2004 (penga03)
**	    Added MINI4LL.
**	04-Oct-2004 (bonro01)
**	    Remove header file stucture.
**	06-Jan-2005 (gorvi01)
**		Modified MINI4LL for 64 bit win port. (Windows server 2003)
**	21-Feb-2005 (thaju02)
**	    Added _size_type to CL_ERR_DESC moreinfo.
**	01-Mar-2005 (drivi01) on behalf of gorvi01 change #475271
**	    Added SSIZE_TYPE to allow signed parameter to be 
**	    handled correctly from the crs files.
**      16-jun-2005 (huazh01)
**          change the definiton of OFFSET_TYPE from 'off_t' to
**          'unsigned long'. 'off_t' is definied as a 'long', which
**          will cause overflow error if the file offset is greater
**          than 2GB. 
**          INGSRV3334, b114687. 
**	31-jul-2006 (zhayu01) Unicode Support II
**	    Added define for IIOMAllocStringW.
**	14-feb-2007 (dougi)
**	    Add CL_MAX_DECPREC to parameterize decimal precision limits.
**	14-Aug-2007 (drivi01)
**	    Add E_EP_MASK for a new facility.
**	26-Dec-2007 (kiria01) SIR119658
**	    Define CL_LIMIT_DECVALF8 - the lowest number bigger than all
**	    representable decimals.
**	28-apr-2008 (joea)
**	    Remove #define for NULL and depend instead on stddef.h.
**      12-dec-2008 (joea)
**          Replace BYTE_SWAP by BIG/LITTLE_ENDIAN_INT.
**	06-Aug-2009 (drivi01)
**	    Add pragma to disable POSIX warning.
**	9-Nov-2009 (kschendel) SIR 122757
**	    Add more casting to ME_ALIGN_MACRO to prevent pointer truncation.
**      26-Nov-2009 (hanal04) Bug 122938
**          Added CL_MAX_DEC_PREC_39 and CL_MAX_DEC_PREC_31 defines.
**          Different GCA protocol levels can handle different levels
**          of decimal precision.
**	09-Feb-2010 (smeke01) b123226, b113797 
**	    Added defines for max u_i8 and i8 digit width.
**	22-Aug-2010 (drivi01)
**	    Update the definition of SSIZE_TYPE. It wasn't
**	    defined correctly.  typedef of SSIZE_TYPE 
**	    to long is the same as int b/c long on 
**	    64-bit OS is 4 bytes, we need it to be 8 bytes.
*/

#ifndef	COMPAT_INCLUDE
#define	COMPAT_INCLUDE

/*
** Machine dependent stuff defined in bzarch.h
*/
#include <bzarch.h>
#define CSASCII
#define LOADDS
#define HUGEPTR

/* pick up definition of size_t and NULL */
#include <stddef.h>

/*
** Per the CL spec, prototypes must be included unless
** specifically excluded.
*/
# ifndef _NO_PROTOS
# define CL_PROTOTYPED
# define CL_HAS_PROTOS /* replaced by CL_PROTOTYPED, but lingers */
# endif /* _NO_PROTOS */

/* 
** Disable compiler warning 4996 about deprecated POSIX functions
** as it is a bug. This is a temporary fix until Microsoft fixes
** the bug.
*/
#pragma warning(disable: 4996)


/******************************************************************************
**
** SIMPLE TYPES
**
******************************************************************************/

typedef	char		i1;
typedef	short		i2;
typedef	long		i4;
# ifdef LP64
# ifdef WIN64
typedef LONG64		i8;
# else
typedef long long	i8;
# endif /* WIN64 */
# else  /* LP64 */
typedef __int64     i8;
# endif /* LP64 */
typedef	float		f4;
typedef	double		f8;
# ifndef __cplusplus
typedef	int 		bool;
# endif /* __cplusplus */

typedef	unsigned char	u_i1;
typedef	unsigned short	u_i2;
typedef	unsigned long	u_i4;
# ifdef LP64
# ifdef WIN64
typedef ULONG64		u_i8;
# else
typedef unsigned long long	u_i8;
# endif /* WIN64 */
# else  /* LP64 */
typedef unsigned __int64    u_i8;
# endif /* LP64 */

typedef	unsigned char 	u_char;
typedef	unsigned short	ushort;
typedef	unsigned short	u_short;
typedef	unsigned long	u_long;
typedef	unsigned char	uchar;
typedef	unsigned int	ulong;

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

typedef long		i4;
typedef unsigned long	u_i4;

/*
**
**    (schka24) Much of the above commentary is misleading.  The RTI
**    gang screwed up, and failed to distinguish between a generic
**    POINTER, which should be a void *;  and a byte-address pointer
**    e.g. BYTEPTR, which should be a char *.  One can do address
**    arithmetic on the latter, but not the former.  Unfortunately,
**    the PTR type was defined as a char * and people used it to do
**    byte-address arithmetic.
**    So, define a true generic pointer which can't be interpreted
**    or dereferenced;  it's just a pointer typed-and-sized placeholder.
*/
# define PTR		IIPTR

typedef	char		*PTR;       /* byte-address pointer */
typedef void *      POINTER;    /* pointer-to-unknown */

typedef int		iP;

typedef u_i2 		UCS2;	/* This is the Unicode character data type
				** ie unicode code point upto 2 bytes
				*/
/******************************************************************************
**
** MISCELLANEOUS
**
******************************************************************************/

#define	BITFLD	unsigned long
#define	_VOID_	(void)


/******************************************************************************
**
** STATUS CODES
**
******************************************************************************/

#define	STATUS		long

#define	OK		0
#define	SUCCEED		0
#define	FAIL		1


/******************************************************************************
**
** system status codes
**
******************************************************************************/

#define	ENDFILE		9900		/* read/write past EOF */


/* the following five #defines are OBSOLETE or not in the CL Spec */
#define	NULLVAL		((i4)0)	/* null value */
#define	NULLPTR		((PTR)0)	/* null pointer */
#define	NULLCHAR	('\0')		/* string terminator */
#define	NULLFILE	((FILE *)0)	/* null file ptr */
#define	NULLI4		0L		/* set i4 to 0 */

#define	EOS		('\0')		/* string terminator */

#ifndef	FALSE
#define	FALSE		0
#endif

#ifndef	TRUE
#define	TRUE		1
#endif

#define	IIMAX_CHAR_VALUE	0377
#define	MIN_CHAR	' '
#define	MAXI1		0x7f
#define	MINI1		(-MAXI1 - 1)
#define	MAXI2		0x7fff
#define	MINI2		(-MAXI2 - 1)
#define	MAXI4		0x7fffffff
#define	MINI4		(-MAXI4 - 1)
#ifdef LP64
#ifdef WIN64
# define        MINI4LL         (-2147483648LL)
#else /* WIN64 */
# define        MINI4LL         (-2147483648L)
#endif /* WIN64 */
#else
# define        MINI4LL         (-2147483648LL)
#endif
#define MAXI64		_I64_MAX
#define MINI64		_I64_MIN
#define MAXUI64		_UI64_MAX
#define MAXI8		MAXI64
#define MINI8		MINI64
#define	FMIN		1E-37
#define	FMAX		1E37
#define	I1MASK		0377
#define MAX_CMDLINE     14500
#define MIN_UCS2        0
#define MAX_UCS2        0xffff
#define MAX_UCS4        0x7fffffffUL
/*
** UTF width for representing UCS2
*/
# ifndef MAX_UTF8_WIDTH
# define MAX_UTF8_WIDTH 6
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
 
# define MAXFS          _I64_MAX
# define longlong	LONGLONG

/* if 64bit memory access is supported */
# if defined(size_t) || defined(_SIZE_T_DEFINED)
# define SIZE_TYPE	size_t
#ifdef LP64
# define MAX_SIZE_TYPE	0x7fffffffffffffffL
#else
# define MAX_SIZE_TYPE	0x7fffffff
#endif
# else
# define SIZE_TYPE	unsigned int
# define MAX_SIZE_TYPE	0x7fffffff
# endif

/* Support for 64-bit memory access for signed type */
# ifdef ssize_t
# define SSIZE_TYPE	size_t
# else
# ifdef LP64
# define SSIZE_TYPE 	LONG_PTR
# else
# define SSIZE_TYPE	long
# endif /* LP64 */
# endif /* ssize_t */

# ifdef LP64
# define MAX_SSIZE_TYPE	0x7fffffffffffffffL
# else
# define MAX_SSIZE_TYPE	0x7fffffff
# endif /* LP64 */

/* if 64bit file access is supported */
# ifdef LARGEFILE64
# include <io.h>
# define OFFSET_TYPE	__int64
# define lseek		_lseeki64
# define tell		_telli64
# define longlong        LONGLONG
# else /* LARGEFILE64 */
# define OFFSET_TYPE 	DWORD
# endif /* LARGEFILE64 */


/******************************************************************************
**
**  Special macro definitions
**
******************************************************************************/

/* #define abs(x)                          (((x) < 0)? -(x) : (x)) */
#define	maskset(bit,word)		(word |= (bit))
#define	maskclr(bit,word)		(word &= ~(bit))
#define	masktest(bit,word)		((bit) & (word))
#define	STRUCT_ASSIGN_MACRO(src, dest)	(dest = src)

/*
** Macros for returning item or octet counts.
*/
# define II_COUNT_ITEMS(x, type)        (x / sizeof(type))
# define II_COUNT_OCTETS(x, type)       (x * sizeof(type))


/******************************************************************************
**
** Name: CL_ERR_DESC - CL internal error descriptor.
**
** Description:
**      This type describes errors that occur inside the Compatibility
**	Library.  All implementation-dependent error information should be
**	encapsulated here.  This includes operating system errors and errors
**	that are peculiar to a specific CL implementation.
**
** 
**  intern	optional error code returned from internal CL routine
**  callid	return value of the NT system call that failed.
**  errnum	value of errno after failure
**  errfile	pointer to source file name where error set.
**  errline	source line number within that file.
**  size	byte count; `data' is accessed via PTR
**
** History:
**	11-Sep-2008 (jonj)
**	    SIR 120874: Add errfile, errline.
**
******************************************************************************/

#define CLE_INFO_ITEMS	3
#define CLE_INFO_MAX	64

typedef struct _CL_ERR_DESC {
    u_i4   intern;
    u_i4   callid;
    PTR	   errfile;
    i4	   errline;
    u_long errnum;
    struct {
	   u_i2 size;
	   union {
	      i2    	_i2;
	      i4    	_i4;
	      i4	_nat;
	      i4	_longnat;
	      SIZE_TYPE	_size_type;
	      f4   	_f4;
	      f8   	_f8;
	      char	string[CLE_INFO_MAX];
	   } data;
    } moreinfo[CLE_INFO_ITEMS];
} CL_ERR_DESC;

/******************************************************************************
**
** Set CL_ERR_DESC.  This macro is private to the CL.  Client
** code may do nothing with the CL_ERR_DESC except pass it by reference.
**
** This macro is intended to be used when reporting either clib errors
** or specific internal CL (non host OS) use.
**
** History:
**	11-Sep-2008 (jonj)
**	    SIR 120874: Add errfile, errline.
**
******************************************************************************/

#define SETCLERR(s, i, c) \
    { \
	(s)->errline = __LINE__; \
	(s)->errfile = __FILE__; \
	(s)->intern = ((u_i4) i); \
	(s)->callid = ((u_i4) c); \
	(s)->errnum =  errno; \
    }

/* Temporary, for backwards compatibility */

#define CL_SYS_ERR    CL_ERR_DESC

/******************************************************************************
**
** Set CL_ERR_DESC for WIN32 NT API errors arising from native api calls,
** which do _not_ set errno.
** It is expected that any routine that examines s->error for EWIN32ERR will
** check for s->callid. The return values are looked up in in a table
** generated from winerror.h.
** This macro is private to the CL, as is SETCLERR.
**
**
******************************************************************************/

#define EWIN32ERR     0xF1F1F1F1

#define SETCLOS2ERR  SETWIN32ERR	/* for lazy compatibility */

/******************************************************************************
**
** SETWIN32ERR is a macro to log errors arising from the
** WIN32 NT API, as opposed to (Unix) traditional clib errors.
**
** History:
**	18-jul-95 (reijo01)
**	    Changed SETWIN32ERR so that it will populate the CL_ERR_DESC with
**		the proper values.
**	11-Sep-2008 (jonj)
**	    SIR 120874: Add errfile, errline.
**
******************************************************************************/

#define SETWIN32ERR(s, i, c) \
    { \
	(s)->callid = ((u_i4) c); \
	(s)->errnum = ((u_i4) i); \
	(s)->errfile = __FILE__; \
	(s)->errline = __LINE__; \
    }

/******************************************************************************
**
** Clear CL_ERR_DESC for WIN32 NT.  NT does not guarantee resetting errno
** during a library call.
**
** History:
**	11-Sep-2008 (jonj)
**	    SIR 120874: Add errfile, errline.
**
******************************************************************************/

#define CLEAR_ERR(s) \
    { \
        (s)->intern = ((u_i4) 0); \
	     (s)->callid = ((u_i4) 0); \
	     (s)->errnum  = ((i4)  0); \
	     (s)->errfile  = ((PTR) NULL); \
	     (s)->errline  = ((i4) 0); \
    }

/*
** CL_CLEAR_ERR() - initialize a CL_ERR_DESC
**
**      initializes a CL_ERR_DESC structure to a ``no error''
**      present state.
**
**  Inputs:
**       err_desc
**              pointer to a CL_ERR_DESC to set.
**  Outputs:
**       err_desc
**              pointer to a CL_ERR_DESC to set.
**  Returns:
**       None
**
**  History:
**	01-Oct-1993 (chiu)
**	    Added definition for CL_CLEAR_ERR.
**	23-jan-2004 (somsa01)
**	    Changed CL_ERR_DESC to be equal to CLEAR_ERR.
*/
# define CL_CLEAR_ERR( s )      CLEAR_ERR( s )


/******************************************************************************
**
** Name: CLERRV - This structure is used to return errors from the CL.
**
** Description:
**	This structure is used to return an error and parameters for
**	the error from CL routines.
**	The argv will be sized according to the number of parameters
**	given by errcnt.
**
******************************************************************************/

typedef struct cl_err_type {
	i4 	cl_errnum;	/* The error number */
	i4 	cl_errcnt;	/* The number of parameters to the error */
	char	*cl_argv[1];	/* The parameters */
} CLERRV;

/******************************************************************************
**
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
**
******************************************************************************/

/******************************************************************************
**
** Major facility error mask for CL.
**
******************************************************************************/

#define	E_CL_MASK	0x00010000

/******************************************************************************
**
** Determine whether an error is a CLerror
**
******************************************************************************/

#define	CLERROR(x)	(x>=E_CL_MASK && x<E_CL_MASK<<1)

/******************************************************************************
**
** Error masks for CL sub-functions.
**
******************************************************************************/

#define	E_BT_MASK	0x00000100
#define	E_CH_MASK	0x00000200
#define	E_CK_MASK	0x00000300
#define	E_CP_MASK	0x00000400
#define	E_CV_MASK	0x00000500
#define	E_DI_MASK	0x00000600
#define	E_DS_MASK	0x00000700
#define	E_DY_MASK	0x00000800
#define	E_DL_MASK	E_DY_MASK
#define	E_ER_MASK	0x00000900
#define	E_EX_MASK	0x00000A00
#define	E_GV_MASK	0x00000B00
#define	E_ID_MASK	0x00000C00
#define	E_II_MASK	0x00000D00
#define	E_IN_MASK	0x00000E00
#define	E_LG_MASK	0x00000F00
#define	E_LK_MASK	0x00001000
#define	E_LO_MASK	0x00001100
#define	E_ME_MASK	0x00001200
#define	E_MH_MASK	0x00001300
#define	E_FP_MASK	E_MH_MASK
#define	E_NM_MASK	0x00001400
#define	E_OL_MASK	0x00001500
#define	E_PC_MASK	0x00001600
#define	E_PE_MASK	0x00001700
#define	E_QU_MASK	0x00001800
#define	E_SI_MASK	0x00001900
#define	E_SM_MASK	0x00001A00
#define	E_SR_MASK	0x00001B00
#define	E_ST_MASK	0x00001C00
#define	E_SX_MASK	0x00001D00
#define	E_TE_MASK	0x00001E00
#define	E_TM_MASK	0x00001F00
#define	E_TO_MASK	0x00002000
#define	E_TR_MASK	0x00002100
#define	E_UT_MASK	0x00002200


/******************************************************************************
**
** New CL sub facilities.
**
******************************************************************************/

#define	E_NT_MASK	0x00002300
#define	E_JF_MASK	0x00002400
#define	E_CS_MASK	0x00002500
#define	E_CI_MASK	0x00002600
#define	E_GC_MASK	0x00002700
#define	E_SD_MASK	0x00002800
#define	E_SL_MASK	0x00002900
#define	E_CM_MASK	0x00002A00
#define E_SA_MASK       0x00002B00
#define E_CX_MASK   0x00002C00
#define E_EP_MASK	0x00002D00

/******************************************************************************
**
** Local UNIX CL sub facilities
**
******************************************************************************/

#define	E_BS_MASK	0x0000FE00
#define	E_HANDY_MASK	0x0000FF00

/******************************************************************************
**
** This might be defined better in bzarch.h.  Then again, it might
** be obsolete (daveb).
**
******************************************************************************/

#ifndef	NEG_I1
#define	NEG_I1(a)	((a) & 0x80)
#endif

#define	BITS_IN(arg)	(BITSPERBYTE * sizeof(arg))

#ifdef BIG_ENDIAN_INT
#define	CV_C_CONST_MACRO(a, b, c, d)	\
	(i4)((((i4)(a) & 0xff) << 24) |\
		  (((i4)(b) & 0xff) << 16) |\
		  (((i4)(c) & 0xff) <<  8) |\
		  (((i4)(d) & 0xff) <<  0))
#else
#define	CV_C_CONST_MACRO(a, b, c, d)	\
	(i4)((((i4)(d) & 0xff) << 24) |\
		  (((i4)(c) & 0xff) << 16) |\
		  (((i4)(b) & 0xff) <<  8) |\
		  (((i4)(a) & 0xff) <<  0))
#endif
/******************************************************************************
**
** Following macro aligns pointer `ptr' on memory boundary that is
** determined by N. It is user's responsibility to check if, in case of
** dynamic memory, the alignment is possible, ie., if the pointer,
** after alignment, will be still pointing to allocated memory, and
** not to something beyond it.
**
** Current implementation assumes that N is a power of 2.
** N is the length of the data type for which `ptr' is aligned.
**
******************************************************************************/
#define	ME_ALIGN_MACRO(ptr, N)	\
    ((PTR) ((SCALARP)((char *) ptr + (N - 1)) & ~((SCALARP)(N) - 1)))

# ifdef __cplusplus
# define FUNC_EXTERN  extern "C"
# else /* __cplusplus */
# define FUNC_EXTERN  extern
# endif /* __cplusplus */

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
**      06-Feb-96 (fanra01)
**          Check for previously defined GLOBALREF and GLOBALDEF.
*/
#define                 CL_SLABEL_MAX   24  /* Operating system dependent	
					    ** size of label in bytes. */

# ifndef GLOBALREF
#define	GLOBALREF	extern
#endif

#define	CL_OFFSETOF	offsetof

#endif

/* chiu 9/8/93 - added the following workarounds */
/* temporary workaround for a name conflict problem in front\hdr\hdr\termdr.h */
#define _leave _ii_fe_leave

/* temporary workaround for a name conflict problem in gl\glf\pm\pm.c */
#define bsearch ii_gl_bsearch

#define STATICDEF	static

/*
** These are for IIGetOSVersion(). More can be added as required.
** The bits used are as follows (tho only the constants are currently used):
**      bit 1:  16-bit OS
**      bit 2:  32-bit OS
**      bit 4:  vers > 3.1 (ie Win 95)
*/
i4      IIgetOSVersion(void);

# define II_WIN32S      1
# define II_WINNT       2
# define II_WIN95       6

# ifndef GLOBALDEF
#define GLOBALDEF
# endif

/* 
** OpenROAD  ME definitions
*/
#define IIOMAllocString(str, session) MEORallocString(str)
#define IIOMAllocStringW(str, session) MEORallocStringW(str)
#define IIOMAllocBlock(size, clear, session) MEORreqmem(0, size, clear, NULL) 
#define IIOMAllocLongBlock(size, clear, session) MEORreqlng(0, size, clear, NULL) 
#define IIOMFreeBlock(ptr, session) MEORfree(ptr) 
#define IIOMAllocArray(num_elts, elt_size, clear, session) \
	MEORreqlng((u_i4)0, ((u_i4)num_elts)*((u_i4)elt_size), \
	    (bool)clear, (STATUS *)NULL)
#define IIOMBlockSize(block, session, size) MEORsize(block,size) 

# define CL_GET_OS_ERR(s)	(((CL_ERR_DESC*)(s))->errnum)
