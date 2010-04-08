/*
**    Copyright (c) 1990, 2004 Ingres Corporation
*/

/*
** cvgcccl.h
**
** Description:
**  This headers contains macros that deal with data representation.
**  Correct versions of these macros must exist for each target machine.
**  They are:
**
**    Integer type byte offset macros.  Each offset is the difference of
**    the address of the integer type and the address of the specified
**    sub-byte within the type.
**
**    I2BYTELO -- offset of the less significant byte in an i2
**    I2BYTEHI -- offset of the more significant byte in an i2
**    I4BYTE0  -- offset of the least significant byte in an i4
**    I4BYTE1  -- offset of the 2nd to least significant byte in an i4
**    I4BYTE2  -- offset of the 2nd to most significant byte in an i4
**    I4BYTE3  -- offset of the most significant byte in an i4
**    I8BYTE0  -- offset of the least significant byte in an i8
**    I8BYTE1  -- offset of the 2nd to least significant byte in an i8
**    I8BYTE2  -- offset of the 3rd to least significant byte in an i8
**    I8BYTE3  -- offset of the 4th to least  significant byte in an i8
**    I8BYTE4  -- offset of the 4th to most significant byte in an i8
**    I8BYTE5  -- offset of the 3rd to most significant byte in an i8
**    I8BYTE6  -- offset of the 2nd to most significant byte in an i8
**    I8BYTE7  -- offset of the most significant byte in an i8
**
**    Floating point type subcomponent extraction macros.  Extract the
**    following subcomponents from a floating point type:
**
**	SIGN: 0 if the number is positive, 1 if negative
**	EXP: the exponent, represented in excess-16382 form, for
**		a mantissa normalized to the range 0.5..1.0.
**	MANT (f4 only): a 32-bit unsigned integer such that
**		0.5 + (MANT / 2**33) is the mantissa, normalized to 0.5..1.0.
**	MANTLO, MANTHI (f8 only): two 32-bit unsigned integers such that
**		0.5 + ( (MANTHI * 2**32 + MANTLO) / 2**65 ) is the
**		mantissa, normalized to 0.5..1.0.
**
**    F4SIGN(f4ptr)		-- extract SIGN from an F4
**    F4EXP(f4ptr)		-- extract EXP from an F4
**    F4MANT(f4ptr)		-- extract MANT from an F4
**    F8SIGN(f8ptr)		-- extract SIGN from an F8
**    F8EXP(f8ptr)		-- extract EXP from an F8
**    F8MANTLO(f8ptr)		-- extract MANTLO from an F8
**    F8MANTHI(f8ptr)		-- extract MANTHI from an F8
**    
**    Floating point composition macros.  Construct an f4 or an f8,
**    given the sign, excesss-16382 exponent, and mantissa normalized
**    to 0.5..1.0.
**
**    MKF4(f4ptr, sign, exp, mant)
**    MKF8(f8ptr, sign, exp, mant)
**
**    If an exponent is out of range on the low end, a minimum floating 
**    point value with sign maintained is produced.  If an exponent is 
**    out of range on the high end, a max floating point value with sign 
**    maintained is produced.
**
**  This is pretty ugly stuff, but I can't think of a cleaner way to
**  do it.  At least the ugly stuff is confined as much as possible.
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
**	06-oct-92 (walt)
**	    Add alpha definition.  Changed "vax" and "alpha" to uppercase.
**      19-oct-92 (gmcquary)
**          pym_us5 specific defines.
**	20-oct-92 (abowler)
**	    Added defines for V1+ DRS6000 (dr6_uv1) to those for dr6_us5.
**	06-nov-92 (mikem)
**	    su4_us5 6.5 port.  Added su4_us5 specific definitions - just used 
**	    the same ones as su4_u42.
**	02-dec-92 (seiwald)
**	    Made alpha distinct from VAX to support G floats.
**	11-jan-93 (gmcquary)
**	    Change defines for CV_INT_TYPE & CV_FLT_TYPE to correct type.
**	22-apr-93 (kevinm)
**	    Changed the local syntax id for CV_VAXG_FLT to 7 from 0.  
**	20-jun-93 (kevinm)
**		Made ALPHA specific changes.  ALPHA is like VMS and doesn't
**		have values.h.  ALPHA is using IEEE floating point format.
**	03-sep-93 (walt)
**	    Further qualified "if defined(VAX)" to be "if defined(VAX) &&
**	    !defined(ALPHA)" when picking local syntax id's.  VAX and ALPHA
**	    are temporarily both defined on Alpha/VMS because of include file
**	    dependencies that need to be straighened out.
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	20-Apr-04 (gordy)
**	    Moved platform indepedent info to GL cvgcc.h.  Moved
**	    platform dependent info to CL cvgcccl.h.
**      19-dec-2008 (stegr01)
**          Itanium port. Removed obsolete VAX definitions.
*/

# ifndef CVGCCCL_HDR_INCLUDED
# define CVGCCCL_HDR INCLUDED


/*
** Name: CV_xxx_TYPE - local syntax selection for each processor
**
** Description:
**	This section selects the local syntax ids for each processor
**	from the options defined above.
**	
*/

# define CV_INT_TYPE	CV_LH3264_INT
# define CV_FLT_TYPE	CV_LH_FLT
# define CV_NCSJ_TYPE	CV_ASCII_NCSJ
# define CV_NCSN_TYPE	CV_ASCII_NCSN	


/*
** Integer/Float platform depedent definitions
*/

/* VAX integer ordering (little endian) */

# define I2BYTELO	0
# define I2BYTEHI	1

# define I4BYTE0	0
# define I4BYTE1	1
# define I4BYTE2	2
# define I4BYTE3	3

# define I8BYTE0	0
# define I8BYTE1	1
# define I8BYTE2	2
# define I8BYTE3	3
# define I8BYTE4	4
# define I8BYTE5	5
# define I8BYTE6	6
# define I8BYTE7	7

# define HIBITS(x,n,ws)	(((x) >> ((ws) - (n))) & ((1 << (n)) - 1))
# define LOBITS(x,n)	((x) & ((1 << (n)) - 1))

/* 
** IEEE float
**
** ALPHA has a bias of 1023 for f8 and 127 for f4
** and a normalized mantissa of the form 1.m(0)m(1)...
** The 1 bit mantissa shift to match NTS normalized
** form of 0.1m(0)m(1) is made by offsetting the
** exponent.
*/

# define F4BIAS		126
# define F8BIAS		1022

# define F4BDIFF	(F4BIAS - CV_EXPBIAS)
# define F8BDIFF	(F8BIAS - CV_EXPBIAS)

typedef struct  {
    unsigned repmant : 23;
    unsigned repexp : 8;
    unsigned repsign : 1;
} FLOATREP;

typedef struct  {
    unsigned repmantlo : 32;
    unsigned repmanthi : 20;
    unsigned repexp : 11;
    unsigned repsign: 1;
} DOUBLEREP;

# define F4SIGN(f4ptr)	( ((FLOATREP *)(f4ptr))->repsign )
# define F4EXP(f4ptr)	( (((FLOATREP *)(f4ptr))->repexp == 0) ? 0 :	\
			  (((FLOATREP *)(f4ptr))->repexp >= 255) ? 0x7FFF :\
			  ((FLOATREP *)(f4ptr))->repexp - F4BDIFF )
# define F4MANT(f4ptr)	( ((FLOATREP *)(f4ptr))->repmant << 9 )

# define F8SIGN(f8ptr)	( ((DOUBLEREP *)(f8ptr))->repsign )
# define F8EXP(f8ptr)	( (((DOUBLEREP *)(f8ptr))->repexp == 0) ? 0 :	\
			  (((DOUBLEREP *)(f8ptr))->repexp >= 2047) ? 0x7FFF :\
			  ((DOUBLEREP *)(f8ptr))->repexp - F8BDIFF )
# define F8MANTHI(f8ptr) ( ((DOUBLEREP *)(f8ptr))->repmanthi << 12 |	\
			HIBITS( ((DOUBLEREP *)(f8ptr))->repmantlo, 12, 32 ) )
# define F8MANTLO(f8ptr) ( ((DOUBLEREP *)(f8ptr))->repmantlo << 12)

# define MKF4(sign, exp, mant, f4ptr)					\
	( ((exp) == 0) ? 						\
		( *((int *)f4ptr) = 0 ) :				\
	  ((exp) + F4BDIFF <= 0) ?					\
	        ( ((FLOATREP *)(f4ptr))->repsign = (sign),		\
		  ((FLOATREP *)(f4ptr))->repexp = 1,			\
		  ((FLOATREP *)(f4ptr))->repmant = 0 ) :		\
	  ((exp) + F4BDIFF >= 255) 					\
	      ? ( ((FLOATREP *)(f4ptr))->repsign = (sign),		\
		  ((FLOATREP *)(f4ptr))->repexp = 254,			\
		  ((FLOATREP *)(f4ptr))->repmant = 0x007FFFFF )		\
	      : ( ((FLOATREP *)(f4ptr))->repsign = (sign),		\
		  ((FLOATREP *)(f4ptr))->repexp = (exp) + F4BDIFF,	\
		  ((FLOATREP *)(f4ptr))->repmant = HIBITS(mant,23,32) ) )

# define MKF8(sign, exp, manthi, mantlo, f8ptr)				\
	( ((exp) == 0) ? 						\
		( ((int *)(f8ptr))[0] = ((int *)(f8ptr))[1] = 0 ) :	\
	  ((exp) + F8BDIFF <= 0) ?					\
		( ((DOUBLEREP *)(f8ptr))->repsign = (sign),		\
		  ((DOUBLEREP *)(f8ptr))->repexp = 1,			\
		  ((DOUBLEREP *)(f8ptr))->repmanthi = 0,		\
		  ((DOUBLEREP *)(f8ptr))->repmantlo = 0 ) :		\
	  ((exp) + F8BDIFF >= 2047) 					\
	      ? ( ((DOUBLEREP *)(f8ptr))->repsign = (sign),		\
		  ((DOUBLEREP *)(f8ptr))->repexp = 2046,		\
		  ((DOUBLEREP *)(f8ptr))->repmanthi = 0x000FFFFF,	\
		  ((DOUBLEREP *)(f8ptr))->repmantlo = 0xFFFFFFFF )	\
	      : ( ((DOUBLEREP *)(f8ptr))->repsign = (sign),		\
		  ((DOUBLEREP *)(f8ptr))->repexp = (exp) + F8BDIFF,	\
		  ((DOUBLEREP *)(f8ptr))->repmanthi = HIBITS(manthi,20,32),\
		  ((DOUBLEREP *)(f8ptr))->repmantlo = 			\
			(LOBITS(manthi,12) << 20 ) | HIBITS(mantlo,20,32) ) )


/*
** Name: CV2x_yy_MACRO - convert datatype from local to NTS and back
**
** Description:
**	CV2N_I1_MACRO	- convert i1 to NTS
**	CV2N_I2_MACRO	- convert i2 to NTS
**	CV2N_I4_MACRO	- convert i4 to NTS
**	CV2N_I8_MACRO	- convert i8 to NTS
**	CV2N_F4_MACRO	- convert f4 to NTS
**	CV2N_F8_MACRO	- convert f8 to NTS
**
**	CV2L_I1_MACRO	- convert i1 to local
**	CV2L_I2_MACRO	- convert i2 to local
**	CV2L_I4_MACRO	- convert i4 to local
**	CV2L_I8_MACRO	- convert i8 to local
**	CV2L_F4_MACRO	- convert f4 to local
**	CV2L_F8_MACRO	- convert f8 to local
**
**	NTS for I1, I2, I4 and i8 is a VAX int of the same 
**	size (lo-hi byte order).
**
**	NTS for F4 and F8 is described in cvgcc.c.
**
** Inputs:
**	src - pointer to source data; element may be unaligned
**	dst - pointer to target area; element may be unaligned
**	status - pointer to status; set only upon error
**
** History:
**	16-Feb-90 (seiwald)
**	    Written.
**	20-Apr-04 (gordy)
**	    Added support for 8 byte integers.
*/

/* Move a byte */
# define BAB(s,os,d,od) (((u_char *)d)[od] = ((u_char *)s)[os])

/*
** I1, I2, I4, I8
*/

# define CV2N_I1_MACRO( src, dst, status ) BAB( src, 0, dst, 0 )
# define CV2L_I1_MACRO( src, dst, status ) BAB( src, 0, dst, 0 )

# define CV2N_I2_MACRO( src, dst, status ) \
		BAB( src, I2BYTELO, dst, 0 ), BAB( src, I2BYTEHI, dst, 1 )
# define CV2L_I2_MACRO( src, dst, status ) \
		BAB( src, 0, dst, I2BYTELO ), BAB( src, 1, dst, I2BYTEHI )

# define CV2N_I4_MACRO( src, dst, status ) \
		BAB( src, I4BYTE0, dst, 0 ), BAB( src, I4BYTE1, dst, 1 ), \
		BAB( src, I4BYTE2, dst, 2 ), BAB( src, I4BYTE3, dst, 3 )
# define CV2L_I4_MACRO( src, dst, status ) \
		BAB( src, 0, dst, I4BYTE0 ), BAB( src, 1, dst, I4BYTE1 ), \
		BAB( src, 2, dst, I4BYTE2 ), BAB( src, 3, dst, I4BYTE3 )

# define CV2N_I8_MACRO( src, dst, status ) \
		BAB( src, I8BYTE0, dst, 0 ), BAB( src, I8BYTE1, dst, 1 ), \
		BAB( src, I8BYTE2, dst, 2 ), BAB( src, I8BYTE3, dst, 3 ), \
		BAB( src, I8BYTE4, dst, 4 ), BAB( src, I8BYTE5, dst, 5 ), \
		BAB( src, I8BYTE6, dst, 6 ), BAB( src, I8BYTE7, dst, 7 )
# define CV2L_I8_MACRO( src, dst, status ) \
		BAB( src, 0, dst, I8BYTE0 ), BAB( src, 1, dst, I8BYTE1 ), \
		BAB( src, 2, dst, I8BYTE2 ), BAB( src, 3, dst, I8BYTE3 ), \
		BAB( src, 4, dst, I8BYTE4 ), BAB( src, 5, dst, I8BYTE5 ), \
		BAB( src, 6, dst, I8BYTE6 ), BAB( src, 7, dst, I8BYTE7 )

/*
** F4, F8
** Use the support routines in cvgcc.c.
*/

# define CV2N_F4_MACRO( src, dst, status ) CV2n_f4( src, dst, status )
# define CV2N_F8_MACRO( src, dst, status ) CV2n_f8( src, dst, status )

# define CV2L_F4_MACRO( src, dst, status ) CV2l_f4( src, dst, status )
# define CV2L_F8_MACRO( src, dst, status ) CV2l_f8( src, dst, status )

#endif /* CVGCCCL_HDR INCLUDED */

