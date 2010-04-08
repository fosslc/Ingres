/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: cvgcc.h
**
** Description:
**	This file provides a macro interface to network datatype
**	conversion.  These macros convert various atomic datatypes
**	(int & float) from the local syntax to the network transfer
**	syntax (NTS) and back.  
**
**	CV_INT_TYPE	Platform integer format.
**	CV_FLT_TYPE	Platform floating point format.
**	CV_NCSJ_TYPE	Default character-set.
**	CV_NCSN_TYPE
**
**	CV_N_CHAR_SZ	Network size of atomic values.
**	CV_N_I1_SZ
**	CV_N_I2_SZ
**	CV_N_I4_SZ
**	CV_N_I8_SZ
**	CV_N_F4_SZ
**	CV_N_F8_SZ
**
**	CV_L_CHAR_SZ	Platform size of atomic values.
**	CV_L_I1_SZ
**	CV_L_I2_SZ
**	CV_L_I4_SZ
**	CV_L_I8_SZ
**	CV_L_F4_SZ
**	CV_L_F8_SZ
**
**	CV2L_I1_MACRO	Convert to local syntax.
**	CV2L_I2_MACRO
**	CV2L_I4_MACRO
**	CV2L_I8_MACRO
**	CV2L_F4_MACRO
**	CV2L_F8_MACRO
**
**	CV2N_I1_MACRO	Convert to network syntax.
**	CV2N_I2_MACRO
**	CV2N_I4_MACRO
**	CV2N_I8_MACRO
**	CV2N_F4_MACRO
**	CV2N_F8_MACRO
**
**	Integer values may be signed or unsigned.  Most modern 
**	systems represent signed integers using 2's complement 
**	arithmetic with the most-significant-bit being used for
**	the sign and a value is negated by performing a binary
**	inversion of the value and adding 1.  The byte order
**	for integers is platform depedent, with little (LSB to
**	MSB) and big (MSB to LSB) endian being the most common 
**	orderings.
**
**	NTS for 1, 2, 4 and 8 byte integer values are as follows:
**
**	------- i1 -------	------- i8 -------
**	| S | int(6:0)   |	| int(7:0)       |
**	------------------	| int(15:8)	 |
**				| int(23:16)     |
**	------- i2 -------	| int(31:24)     |
**	| int(7:0)       |	| int(39:32)     |
**	| S | int(15:8)  |	| int(47:40)     |
**	------------------	| int(55:48)     |
**				| S | int(62:56) |
**	------- i4 -------	------------------
**	| int(7:0)       |
**	| int(15:8)	 |
**	| int(23:16)     |
**	| S | int(30:24) |
**	------------------
**
**	where S is the sign bit and int(i,j) are the value bits.  
**	The representation is little-endian with sizes of 1, 2, 
**	4 and 8 bytes.  Signed values use 2's complement format 
**	where the sign bit is set for negative values.  For 
**	unsigned values, the sign bit is used as an additional 
**	value bit (MSB).
**
**	Floating point values contain three components: sign, 
**	exponent and mantissa.  The sign is represented by a 
**	single bit, set for negative values.  Unlike 2's 
**	complement integers, the sign bit does not effect 
**	the floating point value range which is the same for 
**	positive and negative values.
**
**	The exponent provides the magnitude of the floating point
**	value.  Exponents have a defined range which is biased 
**	(offset) so that exponents are always positive.
**
**	The mantissa represents the floating point value.  A
**	'normalized' mantissa is achieved by shifting, with
**	suitable adjustments to the exponent, the mantissa
**	until a fraction is achieved with the high-order bit 
**	set.  Since the MSB is always set, it may be assumed 
**	to be present and an additional bit of precision can
**	be obtained.
**
**	NTS for 4 and 8 byte floating-point values are as follows:
**
**	------- f4 -------	------- f8 -------
**	| exp(7:0)       |	| exp(7:0)       |
**	| S | exp(14:8)	 |	| S | exp(14:8)	 |
**	| mant(23:16)    |	| mant(55:48)    |
**	| mant(31:24)    |	| mant(63:56)    |
**	| mant(7:0)      |	| mant(39:32)    |
**	| mant(15:8)     |	| mant(47:40)    |
**	------------------	| mant(23:16)    |
**				| mant(31:24)    |
**				| mant(7:0)      |
**				| mant(15:8)     |
**				------------------
**
**      where S is the sign bit, exp(i,j) are the exponent bits
**	and mant(i,j) are the mantissa bits.  The F4 representation 
**	has 15 bits of exponent and 32 bits of mantissa, and is a 
**	total of 6 bytes long.  The F8 representation has 15 bits
**	of exponent and 64 bits of mantissa, and is a total of 10 
**	bytes long.  
**
**	The sign bit is the MSB of the exponent and is set for
**	negative values (Note: the sign and exponent DO NOT
**	combine as a 2's complement integer; other than sharing
**	the same two-byte word, they are separate and distinct).
**
**	The exponent is biased by the value 16382: values 1 to
**	32767 represent binary exponents of -16381 to 16385.
**	The biased exponent value 0 is reserved for floating
**	point value 0.0 (sign and mantissa are ignored and
**	should be 0 as well).
**
**	The mantissa is normallized to the fraction 0.1m(0)m(1)...
**	(a value in the range 0.5 <= m < 1.0).  The redundant 
**	most-significant bit is assumed and is not included
**	in the mantissa.
**
**      Note the odd byte ordering of the mantissa:  each 16 bit 
**	word is in little-endian order, but the overall order is 
**	big-endian.  A similar eccentricity is found on the VAX; 
**	we emulate it.
**      
**	When converting from NTS to local format, exponent values
**	smaller than supported locally should produce a 0.0 float
**	value, while exponent values larger than supported locally
**	should produce the maximum float value (maintaining sign).
**
**	This file also contains the constants which identify the
**	datatype format id for each processor and other internal
**	symbols for use by platform implementations (cvgcccl.h).
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
**	    Separated platform dependent and platform independent info.
*/

# ifndef CVGCC_HDR_INCLUDED
# define CVGCC_HDR_INCLUDED

/*
** Sizes of network types.  Constant sizes.  Do not change.
*/

# define CV_N_CHAR_SZ		1
# define CV_N_I1_SZ		1
# define CV_N_I2_SZ		2
# define CV_N_I4_SZ		4
# define CV_N_I8_SZ		8
# define CV_N_F4_SZ		6
# define CV_N_F8_SZ		10

/* 
** Sizes of local types.  Constant definitions.  Do not change.
*/

# define CV_L_CHAR_SZ		sizeof( char )
# define CV_L_I1_SZ		sizeof( i1 )
# define CV_L_I2_SZ		sizeof( i2 )
# define CV_L_I4_SZ		sizeof( i4 )
# define CV_L_I8_SZ		sizeof( i8 )
# define CV_L_F4_SZ		sizeof( f4 )
# define CV_L_F8_SZ		sizeof( f8 )


/*
** Name: CV_xxxx_yyy - local syntax ids for atomic datatypes
**
** Description:
**	These defines enumerate the various processor specific formats
**	for ints and floats.  These we call the local syntax ids.  New 
**	representations of ints and floats should be given names and
**	ids (0<=id<=127).
**
**	When two IIGCC's establish a connection, they compare the
**	datatype ids of the processors at each end of the connection.
**	If the ids for a datatype are different, that datatype undergoes
**	conversion when traveling through the IIGCC.  The id -1 matches
**	no other ids, and forces conversion.
**
**	Note that a single id represents the size and byte ordering 
**	of sizes of integers.  A single id represents the size and 
**	bit patterns for both a float and a double.
*/

# define CV_HET_TYPE	-1	/* Forces het conversion */

# define CV_LH3232_INT	0	/* VAX - lo to hi - 32bit int, long */
# define CV_HL3232_INT	1	/* SUN - hi to lo - 32bit int, long */
# define CV_BG3636_INT  2       /* BULL GCOS 36 bit int, long */
# define CV_LH3264_INT  3       /* Alpha - lo to hi - 32bit int, 64bit long */
# define CV_HL3264_INT  4       /* AIX - hi to lo - 32bit int, 64bit long */ 

# define CV_VAX_FLT	0	/* VAX floats */
# define CV_HL_FLT	1	/* IEEE floats - hi to lo */
# define CV_DRS_FLT	2	/* DRS 500 floating point */
# define CV_LH_FLT	3	/* IEEE floats - lo to hi */
# define CV_CONVEX_FLT	4	/* Convext floats */
# define CV_IBM_FLT	5	/* IBM 360/370 floats */
# define CV_BULLG_FLT   6       /* BULL GCOS floats */
# define CV_VAXG_FLT	7	/* VAX floats with G style doubles */
# define CV_PC_FLT	10	/* IBM PC 80x86 floats */

# define CV_ASCII_NCSJ	0x00	/* ASCII */
# define CV_ASCII_NCSN	0x00

# define CV_8859_NCSJ	0x00	/* ISO_8859_1 */
# define CV_8859_NCSN	0x01	

# define CV_PC_NCSJ	0x00	/* ASCII_IBMPC */
# define CV_PC_NCSN	0x10

# define CV_EBCDIC_NCSJ	0x01	/* EBCDIC_C */
# define CV_EBCDIC_NCSN	0x00

# define CV_EBCICL_NCSJ	0x01	/* EBCDIC_ICL */
# define CV_EBCICL_NCSN	0x01


/*
** Include the associated CL header which provides platform
** dependent symbols and implementations.  
**
** The following symbols must be defined in cvgcccl.h:
**
**	CV_INT_TYPE
**	CV_FLT_TYPE
**	CV_NCSJ_TYPE
**	CV_NCSN_TYPE
**	CV2L_I1_MACRO
**	CV2L_I2_MACRO
**	CV2L_I4_MACRO
**	CV2L_I8_MACRO
**	CV2L_F4_MACRO
**	CV2L_F8_MACRO
**	CV2N_I1_MACRO
**	CV2N_I2_MACRO
**	CV2N_I4_MACRO
**	CV2N_I8_MACRO
**	CV2N_F4_MACRO
**	CV2N_F8_MACRO
*/

/*
** The following symbols are provided for CL implementations.
*/

# define CV_EXPBIAS	16382		/* NTS float exponent bias */


# include <cvgcccl.h>

# endif /* CVGCC_HDR_INCLUDED */

