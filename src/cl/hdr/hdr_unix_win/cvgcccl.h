/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: cvgcccl.h
**
** Description:
**	This file provides symbols and definitions for platform
**	dependencies related to network datatype conversion.
**
**	The most common architecture uses the ASCII character-
**	set, little/big endian integers and IEEE floats.  This
**	type of architecture is supported by default by cvgcc.
**	All that is required is for the symbols IEEE_FLOAT and
**	LITTLE_ENDIAN/BIG_ENDIAN to be defined in bzarch.h.
**
**	The following actions should be performed to define a new
**	platform:
**
**	1) Determine the basic hardware character-set.  If other
**	   than ASCII, choose the appropriate values from cvgcc.h 
**	   to define CV_NCSJ_TYPE and CV_NCSN_TYPE.  Otherwise,
**	   ASCII will be used by default.  These symbols are no 
**	   longer used for Het/Net conversions (configured 
**	   installation character-set is used instead), so simply 
**	   choose the character-set which best represents that 
**	   used by the compiler to interpret program source code.
**
**	2) Determine the integer format.
**	     a) If the integer format is little/big endian, make
**		sure the symbol LITTLE_ENDIAN/BIG_ENDIAN is defined
**		in bzarch.h.  Otherwise...
**	     b) Use the appropriate value from cvgcc.h to define
**		CV_INT_TYPE.  All supported platforms should now
**		support 64-bit integers, so the CV_xy3264_INT
**		symbols are prefered.  Only use CV_xy3232_INT if
**		64-bit integers are not supported, either by the
**		hardware or emulated by the compiler.
**	     c) If the bytes forming the integer are directly
**		mappable to the NTS little-endian format, define
**		the symbols CV_IxBYTEy, where x is the integer size
**		in bytes, y is the byte referenced (0 is LSB) and 
**		the value is the byte offset.  On a big-endian
**		system, CV_I8BYTE0 is defined as 7.  Otherwise...
**	     d) Define the macros CV2x_Iy_MACRO to perform integer
**		conversions between local and network syntax (where
**		x is L for net to local and N for local to net, and
**		y is the integer size in bytes).
**
**	3) Determine the floating-point format.
**	     b) If the floating-point format is IEEE 754 compatible
**		and the integer format is little/big endian, then
**		the symbols IEEE_FLOAT and LITTLE_ENDIAN/BIG_ENDIAN
**		should be defined in bzarch.h and the default cvgcc
**		implementation will suffice.  Otherwise...
**	     b) Use the appropriate value from cvgcc.h to define
**		CV_FLT_TYPE.
**	     c) If the floating-point format meets the restrictions
**		required by the default conversion implementation
**		(see comments heading the default conversion section 
**		below), then provide the bit layout, exponent bias,
**		exponent max/min and size (in bits) using the 
**		typedefs CV_FLTREP, CV_DBLREP and symbols CV_FxBIAS, 
**		CV_FxEXPBITS, CV_FxEXPMIN, CV_FxEXPMAX (where x is 
**		the float size in bytes).
**		Otherwise...
**	     d) Define the macros CV2x_Fy_MACRO to perform floating-
**		point conversions between local and network syntax
**		(where x is L for net to local and N for local to 
**		net, and y is the float size in bytes).
**
** History:
**	16-Feb-90 (seiwald)
**	    Written.
**	23-Mar-90 (seiwald)
**	    Added IBM 360/370 defines.
**	23-Mar-90 (seiwald)
**	    Bumped IBM to add Convex's floats.
**	25-May-90 (seiwald)
**	    Sequent changes.
**      25-Oct-90 (hhsiung)
**          Add CV definition for Bull DPX/2 ( bu2 and bu3 ).
**	29-Mar-91 (seiwald)
**	    Rearranged NCSJ, NCSN defines to make them a little more legible.
**	    Added entries for dr4_us5, PMFE (PC's).
**      20-Mar-91 (dchan)
**         Added decstation defines
**      22-Mar-91 (dchan)
**         Deleted extraneous lines inserted by "reserve -i"
**    22-apr-91 (szeng)
**	   Add vax_ulx defines.
**	21-mar-91 (seng)
**	    Add RS/6000 specific entries.
**	22-apr-91 (rudyw)
**	    Add odt_us5 to sqs_ptx section. Clean up some additional
**	    piccolo integration lines left from earlier submission.
**	02-jul-91 (johnr)
**	    Add hp3 specific definitions.
**	23-Jul-91 (seiwald)
**	    Add sqs_us5 definition.
**      13-Aug-91 (dchan)
**          Corrected the decstation defines.
**	16-Sep-91 (wojtek)
**	    Added dr6_us5 to the dr4_us5 defines.
**      29-jan-92 (johnst)
**            Added dra_us5 entry for ICL DRS3000 (i486 arch.).
**	30-Jan-92 (seiwald)
**	    Added GCOS definitions.
**	26-feb-92 (jab)
**	    Added info for HP8
**	29-apr-92 (johnr)
**	    Piggybacked ds3_ulx for ds3_osf.
**	29-apr-92 (pearl)
**	    Add nc5_us5 definitions.
**	26-may-92 (kevinm)
**	    Added dg8_us5 for selection of local syntax ids.
**	01-jul-92 (sweeney)
**	    Add apl_us5 to the other 680x0 machines.
**      21-Aug-92 (puru)
**          Added amd_us5 specific definitions.
**      15-sep-92 (peterk)
**          Piggy-back su4_us5 from su4_u42
**      19-oct-92 (gmcquary)
**          pym_us5 specific defines.
**	20-oct-92 (abowler)
**	    Added defines for V1+ DRS6000 (dr6_uv1) to those for dr6_us5.
**	06-nov-92 (mikem)
**	    su4_us5 6.5 port.  Added su4_us5 specific definitions - just used 
**	    the same ones as su4_u42.
**	11-jan-93 (gmcquary)
**	    Change defines for CV_INT_TYPE & CV_FLT_TYPE to correct type.
**	19-jan-93 (sweeney)
**	    usl_us5 is another Intel 80x86  SVR4.2
**      27-jan-93 (pghosh)
**          Modified nc5_us5 to nc4_us5. Till date nc5_us5 & nc4_us5 were
**          being used for the same port. This is to clean up the mess so
**          that later on no confusion is created.
**      05-aug-93 (smc)
**          Added axp_osf specific definitions.
**      22-sep-93 (johnst)
**	    Bug #56444
**          Modified axp_osf definitions to remove reference to the *_I8_MACRO's
**	    added in the previous change and use existing *_I4_MACRO's for i4  
**	    and long conversion, since the conversions to/from long's in GCF 
**	    are actually between i4s. For the DEC alpha, ingres i4s 
**	    are 32-bit and native long's are 64-bit.
**	9-aug-93 (robf)
**          Add su4_cmw
**	25-feb-94 (ajc)
**	    Add hp8_bls based on hp8*
**      10-feb-95 (chech02)
**          Add rs4_us5 for AIX 4.1.
**	09-mar-95 (smiba01)
**	    Added dr6_ues as per dr6_us5
**	24-may-95 (emmag)
**	    Add DESKTOP to define for PMFE.
**	21-jun-95 (allmi01)
**	    Added dgi_us5 support for DG-UX for Intel platforms.
**	17-jul-95 (morayf)
**	    Added sos_us5 to Sequent and old SCO ifdef.
**      10-nov-1995 (murf)
**              Added sui_us5 to all areas specifically defined with usl_us5.
**              Reason, to port Solaris for Intel.
**	13-dec-95 (morayf)
**	    Added SNI RMx00 (rmx_us5) to Pyramid (pym_us5) section.
**	28-jul-1997 (walro03)
**	    Updated for Tandem NonStop (ts2_us5).
**	28-Oct-1997 (allmi01)
**	    Updated for Silicon Graphics (sgi_us5).
**	16-feb-98 (toumi01)
**	    Added lnx_us5 for Linux port.
**	10-may-1999 (walro03)
**	    Remove obsolete version string amd_us5, apl_us5, bu2_us5, bu3_us5,
**	    dr4_us5, dr6_ues, dr6_uv1, dra_us5, ds3_osf, odt_us5, sqs_us5,
**	    vax_ulx.
**      03-jul-99 (podni01)
**          Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**	06-oct-1999 (toumi01)
**	    Change Linux config string from lnx_us5 to int_lnx.
**	08-Sep-2000 (hanje04)
**	    Added axp_lnx for Alpha Linux port.
**	22-Oct-1999 (hweho01)
**	    Added support for AIX 64-bit platform (ris_u64).
**	14-Jun-2000 (hanje04)
**	    Added ibm_lnx for OS/390 Linux
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-jan-2001 (somsa01)
**	    Properly added definitions for S/390 Linux.
**	18-jul-2001 (stephenb)
**	    From allmi01 original change, add defines for i64_aix
**	07-aug-2001 (somsa01)
**	    Added the necessary changes for NT_IA64.
**	04-Dec-2001 (hanje04)
**	    Added support for IA64 Linux (i64_lnx)
**	31-Oct-2002 (hanje04)
**	    Added support for AMD x86_64 Linux (a64_lnx)
**	02-oct-2003 (somsa01)
**	    Ported to NT_AMD64.
**	20-Apr-04 (gordy)
**	    Separated platform dependent and independent information.
**	    Consolidated and simplified common platform implementations.
**	30-Apr-2004 (hanje04)
**	    Replace BIG_ENDIAN and LITTLE_ENDIAN with BIG_ENDIAN_INT and 
**	    LITTLE_ENDIAN_INT because the former are already defined in 
**	    /usr/include/endian.h on Linux.
**	23-Nov-2010 (kschendel)
**	    Drop obsolete ports.
**	3-Dec-2010 (kschendel)
**	    Turns out DESKTOP was on for windows, fix.
*/

# ifndef CVGCCCL_HDR_INCLUDED
# define CVGCCCL_HDR INCLUDED

/*
** This section selects the local syntax ids for each processor
** from the options defined in the generic header cvnet.h.
**	
**	CV_INT_TYPE	Hardware integer format.
**	CV_FLT_TYPE	Hardware floating point format.
**	CV_NCSJ_TYPE	Compiler character-set.
**	CV_NCSN_TYPE
**
** If a particular platform is other than ASCII, little/big
** endian integer and/or IEEE float, then platform dependent 
** entries should be made below prior to the defaults.
*/

# if	defined(usl_us5)

# define CV_NCSJ_TYPE	CV_8859_NCSJ
# define CV_NCSN_TYPE	CV_8859_NCSN

# endif /* usl_us5 */

# if	defined(PMFE) || defined(NT_GENERIC)

# define CV_NCSJ_TYPE	CV_PC_NCSJ
# define CV_NCSN_TYPE	CV_PC_NCSN

# endif /* PMFE */


/*
** Default implementation for ASCII, little/big endian
** and IEEE floats.
*/

/*
** If a character set has not beed defined, default to ASCII.
*/
# ifndef CV_NCSJ_TYPE

# define CV_NCSJ_TYPE   CV_ASCII_NCSJ
# define CV_NCSN_TYPE   CV_ASCII_NCSN

# endif /* CV_NCSJ_TYPE */

/*
** Check for standard little/big endian architecture.  Note
** that the 3232 formats are reserved to just the platforms
** which do not support, either in hardware or via compiler
** emulation, a 64 bit integer.
*/
# if	defined(LITTLE_ENDIAN_INT)

# ifdef NO_64BIT_INT
# define CV_INT_TYPE	CV_LH3232_INT
# else
# define CV_INT_TYPE	CV_LH3264_INT
# endif /* NO_64BIT_INT */

# elif	defined(BIG_ENDIAN_INT)

# ifdef NO_64BIT_INT
# define CV_INT_TYPE	CV_HL3232_INT
# else
# define CV_INT_TYPE	CV_HL3264_INT
# endif /* NO_64BIT_INT */

# endif /* LITTLE_ENDIAN */

/*
** IEEE floats come in two flavors, depending on 
** platform endianness.
*/
# ifdef IEEE_FLOAT

# if	defined(LITTLE_ENDIAN_INT)
# define CV_FLT_TYPE	CV_LH_FLT
# elif	defined(BIG_ENDIAN_INT)
# define CV_FLT_TYPE	CV_HL_FLT
# endif /* LITTLE_ENDIAN */

# endif /* IEEE_FLOAT */

/*
** Default integer byte offsets for standard systems
** (little or big endian).  These symbols may be
** utilized by defining either LITTLE_ENDIAN or
** BIG_ENDIAN for a platform (bzarch.h).
*/

/*
** Byte offsets for little-endian integers
*/
# ifdef LITTLE_ENDIAN_INT

# define CV_I1BYTE0	0

# define CV_I2BYTE0     0
# define CV_I2BYTE1     1

# define CV_I4BYTE0     0
# define CV_I4BYTE1     1
# define CV_I4BYTE2     2
# define CV_I4BYTE3     3

# define CV_I8BYTE0     0
# define CV_I8BYTE1     1
# define CV_I8BYTE2     2
# define CV_I8BYTE3     3
# define CV_I8BYTE4     4
# define CV_I8BYTE5     5
# define CV_I8BYTE6     6
# define CV_I8BYTE7     7

# endif /* LITTLE_ENDIAN */

/*
** Byte offsets for big-endian integers.
*/
# ifdef BIG_ENDIAN_INT

# define CV_I1BYTE0	0

# define CV_I2BYTE0     1
# define CV_I2BYTE1     0

# define CV_I4BYTE0     3
# define CV_I4BYTE1     2
# define CV_I4BYTE2     1
# define CV_I4BYTE3     0

# define CV_I8BYTE0     7
# define CV_I8BYTE1     6
# define CV_I8BYTE2     5
# define CV_I8BYTE3     4
# define CV_I8BYTE4     3
# define CV_I8BYTE5     2
# define CV_I8BYTE6     1
# define CV_I8BYTE7     0

# endif /* BIG_ENDIAN */



/*
** Default implemenation of conversion macros between local and NTS.
**
** CV2{NL}_{IF}{1248}_MACRO( u_i1 *src, u_i1 *dst, STATUS *status )
**
** Input:
**	src	Source value.
**
** Output:
**	dst	Target destination.
**	status	Conversion status.
**
**	
** The integer conversion macros require symbol definitions in the
** form CV_IxBYTEy where x is the size of the integer in bytes, y is
** the referenced byte (LSB is 0, MSB is x-1) and the symbol value 
** is the byte offset of the referenced byte.  These symbols can be
** generated automatically for standard formats by defining the
** symbols LITTLE_ENDIAN or BIG_ENDIAN as appropriate (bzarch.h).
**
** The floating-point conversion macros are covers for the functions
** CV2n_f4(), CV2n_f8(), CV2l_f4(), CV2l_f8().  Systems which support
** IEEE 754 Standard floating-point values simply need to define the
** symbols IEEE_FLOAT and LITTLE_ENDIAN or BIG_ENDIAN.  Systems with 
** non-IEEE floats may alternatively provide the exponant bias 
** (CV_FxBIAS), number of exponent bits (CV_FxEXPBITS), unbiased 
** min exponent (CV_FxEXPMIN), unbiased max exponent (CV_FxEXPMAX) 
** and a structure defining the bit representation of floating-point 
** values (CV_FLTREP, CV_DBLREP).
**
** The following are examples of bit representation structures for
** floating-point values (specifically: IEEE big-endian):
**
**	typedef struct  
**	{
**	    unsigned repsign : 1;
**	    unsigned repexp  : 8;
**	    unsigned repmant : 23;
**	} CV_FLTREP;
**
**	typedef struct  
**	{
**	    unsigned repsign   : 1;
**	    unsigned repexp    : 11;
**	    unsigned repmanthi : 20;
**	    unsigned repmantlo : 32;
**	} CV_DBLREP;
**
** The following restrictions must be met to use these structures
** with the default implemenation macros:
**
**	1) The byte-order of the float format is the same as I4/I8.
**	2) The bits in the mantissa (repmant), or most-significant 
**	   bits of the mantissa in an F8 (repmanthi), are contiguous.
**	3) The number of (most-significant) mantissa bits (repmant,
**	   repmanthi) does not exceed 32 - <# sign bits: repsign> - 
**	   <# exp bits: repexp>.
**	4) In an F8, the least-significant mantissa bits (repmantlo)
**	   are represented by 32 contiguous bits.
*/

/* 
** Copy a byte 
*/
# define CV_COPY_BYTE(s,os,d,od) (((u_i1 *)d)[od] = ((u_i1 *)s)[os])

/*
** Generic integer conversions via macros using byte assignment.
*/

# ifndef CV2N_I1_MACRO
# define CV2N_I1_MACRO( src, dst, status ) \
	(CV_COPY_BYTE( (src), CV_I1BYTE0, (dst), 0 ), *(status) = OK)
# endif

# ifndef CV2L_I1_MACRO
# define CV2L_I1_MACRO( src, dst, status ) \
	(CV_COPY_BYTE( (src), 0, (dst), CV_I1BYTE0 ), *(status) = OK)
# endif

# ifndef CV2N_I2_MACRO
# define CV2N_I2_MACRO( src, dst, status ) \
       (CV_COPY_BYTE( (src), CV_I2BYTE0, (dst), 0 ), \
	CV_COPY_BYTE( (src), CV_I2BYTE1, (dst), 1 ), \
	*(status) = OK)
# endif

# ifndef CV2L_I2_MACRO
# define CV2L_I2_MACRO( src, dst, status ) \
       (CV_COPY_BYTE( (src), 0, (dst), CV_I2BYTE0 ), \
	CV_COPY_BYTE( (src), 1, (dst), CV_I2BYTE1 ), \
	*(status) = OK)
# endif

# ifndef CV2N_I4_MACRO
# define CV2N_I4_MACRO( src, dst, status ) \
       (CV_COPY_BYTE( (src), CV_I4BYTE0, (dst), 0 ), \
	CV_COPY_BYTE( (src), CV_I4BYTE1, (dst), 1 ), \
	CV_COPY_BYTE( (src), CV_I4BYTE2, (dst), 2 ), \
	CV_COPY_BYTE( (src), CV_I4BYTE3, (dst), 3 ), \
	*(status) = OK)
# endif

# ifndef CV2L_I4_MACRO
# define CV2L_I4_MACRO( src, dst, status ) \
       (CV_COPY_BYTE( (src), 0, (dst), CV_I4BYTE0 ), \
	CV_COPY_BYTE( (src), 1, (dst), CV_I4BYTE1 ), \
	CV_COPY_BYTE( (src), 2, (dst), CV_I4BYTE2 ), \
	CV_COPY_BYTE( (src), 3, (dst), CV_I4BYTE3 ), \
	*(status) = OK)
# endif

# ifndef CV2N_I8_MACRO
# define CV2N_I8_MACRO( src, dst, status ) \
       (CV_COPY_BYTE( (src), CV_I8BYTE0, (dst), 0 ), \
	CV_COPY_BYTE( (src), CV_I8BYTE1, (dst), 1 ), \
	CV_COPY_BYTE( (src), CV_I8BYTE2, (dst), 2 ), \
	CV_COPY_BYTE( (src), CV_I8BYTE3, (dst), 3 ), \
	CV_COPY_BYTE( (src), CV_I8BYTE4, (dst), 4 ), \
	CV_COPY_BYTE( (src), CV_I8BYTE5, (dst), 5 ), \
	CV_COPY_BYTE( (src), CV_I8BYTE6, (dst), 6 ), \
	CV_COPY_BYTE( (src), CV_I8BYTE7, (dst), 7 ), \
	*(status) = OK)
# endif

# ifndef CV2L_I8_MACRO
# define CV2L_I8_MACRO( src, dst, status ) \
       (CV_COPY_BYTE( (src), 0, (dst), CV_I8BYTE0 ), \
	CV_COPY_BYTE( (src), 1, (dst), CV_I8BYTE1 ), \
	CV_COPY_BYTE( (src), 2, (dst), CV_I8BYTE2 ), \
	CV_COPY_BYTE( (src), 3, (dst), CV_I8BYTE3 ), \
	CV_COPY_BYTE( (src), 4, (dst), CV_I8BYTE4 ), \
	CV_COPY_BYTE( (src), 5, (dst), CV_I8BYTE5 ), \
	CV_COPY_BYTE( (src), 6, (dst), CV_I8BYTE6 ), \
	CV_COPY_BYTE( (src), 7, (dst), CV_I8BYTE7 ), \
	*(status) = OK)
# endif

/*
** Generic floating-point conversions are handled via functions.
*/

# ifndef CV2N_F4_MACRO
# define CV2N_F4_MACRO( src, dst, status ) \
			CV2n_f4( (u_i1 *)(src), (u_i1 *)(dst), (status) )
# endif

# ifndef CV2L_F4_MACRO
# define CV2L_F4_MACRO( src, dst, status ) \
			CV2l_f4( (u_i1 *)(src), (u_i1 *)(dst), (status) )
# endif

# ifndef CV2N_F8_MACRO
# define CV2N_F8_MACRO( src, dst, status ) \
			CV2n_f8( (u_i1 *)(src), (u_i1 *)(dst), (status) )
# endif

# ifndef CV2L_F8_MACRO
# define CV2L_F8_MACRO( src, dst, status ) \
			CV2l_f8( (u_i1 *)(src), (u_i1 *)(dst), (status) )
# endif


/*
** IEEE floating-point representations.
**
** The IEEE 754 four-byte (single-precision) format has 1 sign
** bit, 8 exponent bits and 23 mantissa bits.  Biased exponent
** values of 1 to 254 represent binary exponents of -126 to 127
** (a bias of 127).  Biased exponent value 0 is reserved for the 
** floating point value 0.0 and denormalized mantissa values.  
** Biased exponent value 255 is reserved for infinity and NaN 
** values.  The normalized mantissa takes the form 1.m(0)m(1)... 
** where the redundant leading bit is assumed.
**
** The IEEE 754 eight-byte (double-precision) format has 1 sign
** bit, 11 exponent bits and 52 mantissa bits.  Biased exponent
** values of 1 to 2046 represent binary exponents of -1022 to
** 1023 (a bias of 1023).  Biased exponent value 0 is reserved 
** for floating point value 0.0 and denormalized mantissa values.  
** Biased exponent value 2047 is reserved for infinity and NaN 
** values.  The normalized mantiss take the form 1.m(0)m(1)... 
** where the redundant leading bit is assumed.
**
** The byte ordering of IEEE 754 floats generally matches the
** platform integer byte ordering.
**
** Float values with a biased exponent of 0 are mapped to NTS
** exponent 0 value (float value 0.0) since the denormalized
** mantissa values represent very small values close to 0.
** While IEEE 754 can represent -0.0, this is forced positive
** for NTS.  Float values with a biased exponent of 255/2047
** are given an NTS biased exponent of 32767 to force the
** largest magnitude float value (sign is maintained).
**
** Since the normalized mantissa fraction differs between NTS
** and IEEE 754, the standard exponent bias of 127 & 1023 are
** adjusted to 126 & 1022 to accomodate the shift in mantissa.
** This adjusment is represented below in the symols CV_FxBIAS,
** CV_FxEXPMIN and CV_FxEXPMAX.
*/

# ifdef IEEE_FLOAT

# define CV_F4EXPBITS	8
# define CV_F4BIAS	126
# define CV_F4EXPMIN	-125
# define CV_F4EXPMAX	128

# define CV_F8EXPBITS	11
# define CV_F8BIAS	1022
# define CV_F8EXPMIN	-1021
# define CV_F8EXPMAX	1024

# ifdef LITTLE_ENDIAN_INT

typedef struct  
{
    unsigned repmant : 23;
    unsigned repexp  : 8;
    unsigned repsign : 1;
} CV_FLTREP;

typedef struct  
{
    unsigned repmantlo : 32;
    unsigned repmanthi : 20;
    unsigned repexp    : 11;
    unsigned repsign   : 1;
} CV_DBLREP;

# endif /* LITTLE_ENDIAN */

# ifdef BIG_ENDIAN_INT

typedef struct  
{
    unsigned repsign : 1;
    unsigned repexp  : 8;
    unsigned repmant : 23;
} CV_FLTREP;

typedef struct  
{
    unsigned repsign   : 1;
    unsigned repexp    : 11;
    unsigned repmanthi : 20;
    unsigned repmantlo : 32;
} CV_DBLREP;

# endif /* BIG_ENDIAN */
# endif /* IEEE_FLOAT */


/*
** The default floating-point conversion functions utilize
** the macros defined below.  These macros require the
** definition of the symbols CV_FxBIAS, CV_FxEXPBITS,
** CV_FxEXPMIN, CV_FxEXPMAX and the typedefs for 
** CV_FLTREP and CV_DBLREP.
**
** sign = CV_F4SIGN(&f4)	Get sign bit from f4.
** exp  = CV_F4EXP(&f4)		Get biased exponent from f4.
** mant = CV_F4MANT(&f4)	Get 32 bit mantissa (LSB 0 filled) from f4.
**        CV_MKF4(s,e,m,&f4)	Make f4 from sign, exponent and mantissa.
** sign = CV_F8SIGN(&f8)	Get sign bit from f8.
** exp  = CV_F8EXP(&f8)		Get biased exponent from f8.
** hi   = CV_F8MANTHI(&f8)	Get 32 hi bits mantissa from f8.
** lo   = CV_F8MANTLO(&f8)	Get 32 lo bits mantissa (LSB 0 filled) from f8.
**        CV_MKF8(sign,exp,hi,lo,&f8)	Make f8 from sign, exp, hi/lo mant.
*/

# define CV_LOBITS(x,n)    ((x) & ((1 << (n)) - 1))
# define CV_HIBITS(x,n,ws) CV_LOBITS( (x) >> ((ws) - (n)), (n) )

# ifdef CV_F4EXPBITS

# define CV_F4SEXBITS	(CV_F4EXPBITS + 1)
# define CV_F4MANBITS	(32 - CV_F4SEXBITS)

# define CV_F4SIGN(pF4)	( ((CV_FLTREP *)(pF4))->repsign )
# define CV_F4EXP(pF4)	( ((CV_FLTREP *)(pF4))->repexp )
# define CV_F4MANT(pF4)	( ((CV_FLTREP *)(pF4))->repmant << CV_F4SEXBITS )

# define CV_MKF4(sign, exp, mant, pF4)					\
			( ((CV_FLTREP *)(pF4))->repsign = (sign),	\
			  ((CV_FLTREP *)(pF4))->repexp = (exp),		\
			  ((CV_FLTREP *)(pF4))->repmant = 		\
				CV_HIBITS( mant, CV_F4MANBITS, 32 ) )

# endif /* CV_F4EXPBITS */

# ifdef CV_F8EXPBITS

# define CV_F8SEXBITS	(CV_F8EXPBITS + 1)
# define CV_F8MANBITS	(32 - CV_F8SEXBITS)

# define CV_F8SIGN(pF8)	( ((CV_DBLREP *)(pF8))->repsign )
# define CV_F8EXP(pF8)	( ((CV_DBLREP *)(pF8))->repexp )
# define CV_F8MANTLO(pF8) ( ((CV_DBLREP *)(pF8))->repmantlo << CV_F8SEXBITS )
# define CV_F8MANTHI(pF8) \
		( (((CV_DBLREP *)(pF8))->repmanthi << CV_F8SEXBITS) | \
		  CV_HIBITS(((CV_DBLREP *)(pF8))->repmantlo,CV_F8SEXBITS,32) )

# define CV_MKF8(sign, exp, manthi, mantlo, pF8)			\
		( ((CV_DBLREP *)(pF8))->repsign = (sign),		\
		  ((CV_DBLREP *)(pF8))->repexp = (exp),			\
		  ((CV_DBLREP *)(pF8))->repmanthi = 			\
			CV_HIBITS( manthi, CV_F8MANBITS, 32 ),		\
		  ((CV_DBLREP *)(pF8))->repmantlo = 			\
			(CV_LOBITS( manthi, CV_F8SEXBITS ) << CV_F8MANBITS) | \
			CV_HIBITS( mantlo, CV_F8MANBITS, 32 ) )

# endif /* CV_F8EXPBITS */


/*
** Function prototypes for default floating-point conversion routines.
*/

FUNC_EXTERN VOID CV2n_f4(
    u_i1	*src,
    u_i1	*dst,
    STATUS	*status
);

FUNC_EXTERN VOID CV2l_f4(
    u_i1	*src,
    u_i1	*dst,
    STATUS	*status
);

FUNC_EXTERN VOID CV2n_f8(
    u_i1	*src,
    u_i1	*dst,
    STATUS	*status
);

FUNC_EXTERN VOID CV2l_f8(
    u_i1	*src,
    u_i1	*dst,
    STATUS	*status
);

# endif /* CVGCCCL_HDR_INCLUDED */

