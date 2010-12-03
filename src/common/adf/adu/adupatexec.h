#ifndef _ADUPATEXEC_H_INC
#define _ADUPATEXEC_H_INC

/*
** Copyright (c) 2008, 2010 Ingres Corporation
*/

/**
** Name: adupatexec.h - This include file defines the structures for the
**			pattern matching state engine and its interface.
**
** Description:
**	This is the core include file for the pattern matching code.
**	It pulls together the three portion of code: Pattern Compiler,
**	Execution Engine and Data Access and defines their respective
**	interfaces.
**
** History:
**	06-Jun-2008 (kiria01) SIR120473
**	    Created primarilty for LONG object LIKE support
**	06-Sep-2008 (kiria01) SIR120473
**	    Adjust the structures to reduce MEreqmem needs
**	27-Oct-2008 (kiria01) SIR120473
**	    Introduce AD_PATDATA having dropped from u_i4 vector
**	    to an u_i2 form that fits in with VLUP arrangements.
**	11-Dec-2008 (kiria01) SIR120473
**	    Split solitary i4 flags in patdata to reduce abends
**	    on Solaris
**	15-Dec-2008 (kiria01) SIR120473
**	    Dropped redundant flags from AD_PAT_SEA_CTX
**      12-dec-2008 (joea)
**          Replace BYTE_SWAP by BIG/LITTLE_ENDIAN_INT.
**	06-Jan-2009 (kiria01) SIR120473
**	    Handle the escape char as a proper DBV so that Unicode endian
**	    issues are avoided.
**	09-Feb-2009 (kiria01) SIR120473
**	    Added AD_PAT2_PURE_LITERAL, AD_PAT2_LITERAL and
**	    AD_PAT2_MATCH pattern summary hints.
**	30-May-2009 (kiria01) b122128
**	    Added fast to ease streamlined execution of simple strings.
**	28-Jul-2009 (kiria01) b122377
**	    Export adu_pat_decode.
**	24-Aug-2009 (kiria01) b122525
**	    Set casts on PUTNUM/NUMSZ to quieten compiler.
**	25-Aug-2009 (drivi01)
**	    Drop ADU_pat_legacy from this header and define it
**	    as GLOBALDEF in adupat.c.
**	08-Jan-2010 (kiria01) b123118
**	    PUTNUM on bigendian was writing the wrong bytes for numbers
**	    outside the 0..252 range.
**	19-Nov-2010 (kiria01) SIR 124690
**	    Add support for UCS_BASIC collation.
**/

#include <adp.h>

/*
** Defining the following will enable the diagnostic
** code to be built in. Presently this is enabled
** and the actual invocation of the trace code will
** controlled using MO.
*/
#ifndef PAT_DBG_TRACE
#define PAT_DBG_TRACE 1 
#endif

/*
** Ensure some standard limits are defined
*/
#ifndef I1_MAX
#define I1_MAX (127)
#endif
#ifndef I1_MIN
#define I1_MIN (-I1_MAX-1)
#endif
#ifndef I2_MAX
#define I2_MAX (((i2)I1_MIN*I1_MIN-1)*2+1)
#endif
#ifndef I2_MIN
#define I2_MIN (-I2_MAX-1)
#endif
#ifndef I4_MAX
#define I4_MAX (((i4)I2_MIN*I2_MIN-1)*2+1)
#endif
#ifndef I4_MIN
#define I4_MIN (-I4_MAX-1)
#endif
#ifndef I8_MAX
#define I8_MAX (((i8)I4_MIN*I4_MIN-1)*2+1)
#endif
#ifndef I8_MIN
#define I8_MIN (-I8_MAX-1)
#endif

/*
** Define the search flags
**
** Core defined in adf.h and the reserved ones in here
**  AD_PAT_ISESCAPE_MASK
**	AD_PAT_NO_ESCAPE
**	AD_PAT_HAS_ESCAPE
**	AD_PAT_DOESNT_APPLY
**	AD_PAT_HAS_UESCAPE
**  AD_PAT_WO_CASE
**  AD_PAT_WITH_CASE
**  AD_PAT_DIACRIT_OFF
**  AD_PAT_BIGNORE
**
** Leaving the PAT2 flags for flags2
*/
#define AD_PAT2_COLLATE		0x0001U /* Collation present */
#define AD_PAT2_UNICODE_CE	0x0002U /* Unicode CE pattern */
#define AD_PAT2_UCS_BASIC	0x0004U /* Unicode pattern UCS_BASIC */
#define AD_PAT2_UTF8_UCS_BASIC	0x0008U /* UTF8 pattern UCS_BASIC */
#define AD_PAT2_ENDLIT		0x0800U /* Ending hint */
#define AD_PAT2_MATCH		0x1000U /* Matches all */
#define AD_PAT2_LITERAL		0x2000U /* Single literal */
#define AD_PAT2_PURE_LITERAL	0x4000U /* Could be EQUALS */
#define AD_PAT2_FORCE_FAIL	0x8000U /* Problem in pattern */
#define AD_PAT2_RSVD_UNUSED	0x07FCU

/*
** Define the range marker used in unicode patterns
*/
#define U_RNG (UCS2)(0xFFFFU)

/* Get the generated OPCODES */
#include "adupatexec_ops.h"

/*
** Build the opcode values as enums
*/
#define _DEFINEOP(_n,_v,_a,_b,_c,_d,_e,_f) _n=_v,
#define _DEFINEEX(_n,_v) _n=_v,
#define _ENDDEFINE
enum PAT_OP_enums{
    PAT_OPCODES
    PAT_OP_MAX
};
#undef _DEFINEOP
#undef _DEFINEEX
#undef _ENDDEFINE

/* Forward define the search struct */
struct _AD_PAT_SEA_CTX;

/*
**   AD_PAT_CTX pattern thread context
**
** This represents a scan thread. It's meant to contain
** all the state needed to stall the scan for a new buffer.
** When a FORK happens, this structure is copied for the child
** and each will have a different state.
*/
typedef struct _AD_PAT_CTX {
    struct _AD_PAT_CTX
			*next;		/* List management */
    struct _AD_PAT_SEA_CTX
			*parent;	/* Back pointer */
    u_i1		*pc;		/* Initial/Next pat op */
    u_i1		*patbufend;	/* End of pattern ops */
    u_i1		*ch;		/* 'read' pointer */
    u_i1		*litset;	/* Literal and set operators use these */
    u_i1		*litsetend;
    u_i4		L;		/* Holds bitset length */
    u_i4		O;		/* Holds bitset offset */
    enum PAT_OP_enums	state;		/* Execution state */
#if PAT_DBG_TRACE>0
    u_i4		id;
    u_i4		idp;
#endif
    u_i4		sp;		/* Stack frame pointer */
    /*
    ** frame[0] by convention models the instruction level N & M
    ** counters. The other frames will only be used by the BEG-END
    ** instructions. If none of them is active SP will be zero.
    ** It will be possible at compile time to know the max stack
    ** depth needed but for simplicity we fix a depth of 7 nests
    ** for now (8 - 1).
    */
    struct frame {
	u_i4		N;		/* Models the () N counter */
	u_i4		M;		/* Models the () M counter */
#define		DEF_STK		10
    } frame[DEF_STK+1];
} AD_PAT_CTX;

/*
** AD_PATDATA - This points to the data representing the list
** of patterns to be used. The data is structured as an u_i2
** aligned array as follows structured to match NVARCHAR varying
** length:
**	+---------------+ 
**	|  LENGTH (=TOT)|:0         Total # short words
**	+---------------+ 
**	|     NPATS     |:1         Usually this will be 1
**	+---------------+ 
**	|               |:2
**	|-    FLAGS    -|           set to match SEA_FLAGS
**	|               |:3
**	+---------------+ 
**	| PAT 1 LENGTH  |:4         # short words in pat1 incl len
**	+---------------+ 
**	|  PAT 1 DATA   |:5	    Most of the pattern operators need
**	/   Padded to   /           no alignment internally except for
**	|    align      |           Unicode literal or set operands which
**	|   with next   |           will be aligned to 2 byte boundaries
**	+---------------+ 
**	| PAT 2 LENGTH  |:4+[4]
**	+---------------+ 
**	|  PAT 2 DATA   |
**	/   Padded to   /
**	|  align next   |
**	+---------------+ 
**			 :TOT
*/
typedef union _patdata {
    struct {
	u_i2 length;
	u_i2 npats;
	u_i2 flags;
	u_i2 flags2;
	u_i2 pat1_len;	/* 1st free */
	u_i1 first_op;	/* 1st op of first pat */
#define PATDATA_1STFREE 4
    } patdata;
    u_i4 align_dummy;
    u_i2 vec[2000];		/* Sized to remove MEreqmem from adu_like_all */
} AD_PATDATA;

/*
**   AD_PAT_SEA_CTX Search patterns context
**
** This structure holds the common state passed from the comiler
** to the executive. It holds the execution queues and the thread
** contexts and the pointers into the thread common data buffers.
*/
typedef struct _AD_PAT_SEA_CTX {
    AD_PATDATA *patdata;
    /*
    ** At execute time, buffer & bufend will be set to
    ** point to the segment of data to be scanned. All the
    ** AD_PAT_CTX block will have .ch pointing into it.
    */
    u_i1		*buffer;
    u_i1		*bufend;
    u_i1		*buftrueend;
    u_i4		buflen;
    /*
    ** flags - pat_flags
    */
    bool		at_bof		: 1;
    bool		at_eof		: 1;
    bool		trace		: 1;
    bool		force_fail	: 1;
    bool		cmplx_lim_exc	: 1;
    struct _AD_PAT_CTX	*stalled;
    struct _AD_PAT_CTX	*pending;
    struct _AD_PAT_CTX	*free;
    PTR			setbuf;		/* (vm) Temporary work buffer */

#if PAT_DBG_TRACE>0
    u_i4		nid;
    char		*infile;	/* (vm) Debug load file */
    char		*outfile;	/* (vm) Debug dump file */
#endif

    u_i4		nctxs_extra;
    u_i4		nctxs;
#define		DEF_THD		8
    struct _AD_PAT_CTX	ctxs[DEF_THD];
} AD_PAT_SEA_CTX;

/*
**   AD_PAT_DA_CTX data access context
**
** This context models the data interface that allows the data
** to be presented the same way irrespective of whether it were
** a long object or a short one.
** The rationale is that we wish to read the blob data once and
** allow multiple patterns to be scanned for without resorting
** to re-reading the data.
*/
typedef struct _AD_PAT_DA_CTX {
    ADF_CB		*adf_scb;	/* ADF context */
    AD_PAT_SEA_CTX	*sea_ctx;	/* Search context */
    DB_DATA_VALUE	*segment_dv;	/* Common DV for handling */

    /* Context for peripheral data*/
    ADP_POP_CB		pop_cb;
    DB_DATA_VALUE	under_dv;
    DB_DATA_VALUE	cpn_dv;
    DB_DATA_VALUE	norm_dv;
    ADP_PERIPHERAL	*p;
    ADF_AG_STRUCT	work;
			/*(vm) work.adf_agwork.db_data */
    PTR			mem;		/* (vm) Pointer to allocated
					** data for intermediate DBVs
					** needed for coercions */
    i1			unicode;	/* Dealing with Unicode */
    i1			unicodeCE;	/* Dealing with Unicode + CE */
    i1			utf8;		/* UTF-8 handling needed */
    i1			binary;		/* Binary handling needed */
    i1			is_long;	/* Dealing with peripheral */
    i1			fast;		/* Trivial fast track */
    i4			gets;		/* Number of gets performed */
    u_i8		seg_offset;	/* Offset of current block */
    u_i8		eof_offset;	/* Offset of block beyond eof */
    i4			trace;
} AD_PAT_DA_CTX;

/*
** The Pattern operator stream is a sequence of bytes. Opcodes will
** imply parameters and these will be encoded inline in the stream, as either
** a counted byte block (literal or set info) or as a numeric value.
**
** All numbers, be they counts or offsets or lengths or even the byte block
** length specifier will be stored as follows. Small values will be stored as
** a byte, others will be prefixed with an extra byte descriptor followed by
** the value of the relevant size. It will be noted that ALL such numbers will
** be odd in byte length!
** 
** +---+
** | n |  0 <= n < 0xFC (252)
** +---+
**
** +---+
** |xFC|    Reserved for future use
** +---+
**
** +---+---+---+
** |xFD| i2 val|
** +---+---+---+
**
** +---+---+---+---+---+
** |xFE|   i4 value    |
** +---+---+---+---+---+
**
** +---+---+---+---+---+---+---+---+---+
** |xFF|           i8 value            |
** +---+---+---+---+---+---+---+---+---+
**
** In all cases the numeric byte order will be that of the machine.
**
** Literal text operands will be encoded using a *byte* length encoded as
** above, followed by that number of bytes. The next byte beyond will
** be the next instruction or operand. For example 300 bytes of literal
** text operand will be represented as:
**               1   2   3   4    299 300
** +---+---+---+---+---+---+--//-+---+---+---+
** |xFD|  300  | TEXT TEXT ... //   TEXT | Next operator
** +---+---+---+---+---+---+---+//---+---+---+
**
** Set data comes in three forms, character for character with inclusive range
** support, a compound inclusion/exclusion form or, for strict single byte
** character with or without case sensitive match, a bit set can be used. If
** collation is required then the bit set will be avoided.
** 
** The BITSET operand form is arranged as an offset adjusted bit map of bits.
** the layout will be:
**
** +---+---+---+---+---+---+---+---+---+---+---+
** |off| n |         n bytes (0-64)        | Next operator
** +---+---+---+---+---+---+---+---+---+---+---+
** Where off is the offset of bit 0 of the list (low 3 bits zero). 
** For example [0-9] would become:
** +---+---+---+---+---+
** | 6 | 2 |xFF|x03| Next operator
** +---+---+---+---+---+
** The bitset operators are BSET (1, N, 0-W, 1-W, N-W and N-M) and a negative
** form NBSET (1, N, 0-W, 1-W, N-W and N-M).
**
** The SIMPLE SET operand form will be represented as ordered characters
** (possibly multi-byte) with a special interpretation of the '-' character.
** If present, it will be *followed* by the two bound characters (poss MB)
** which represent an inclusive range pair, low then high. A literal '-' if
** present singly will be 'faked' as '---' which will be a range of 1.
** The stored set is ordered as it is built which allows the ranges to be
** optimised - overlaps are coalesced andthe final execution can make use of
** the ordering to tell if a scan can be curtailed.
**
** +---+---+---+---+---+---+---+---+---+---+---+
** | n*|         n bytes                   | Next operator
** +---+---+---+---+---+---+---+---+---+---+---+
** n* - might need count size extension as above.
** For example [A-Z0-9x.6] would be:
** +---+---+---+---+---+---+---+---+---+---+---+
** | 8 | . | - | 0 | 9 | - | A | Z | x | Next operator
** +---+---+---+---+---+---+---+---+---+---+---+
** The simple set operators are SET (1, N, 0-W, 1-W, N-W and N-M)
**
** The COMPOUND SET operand form will be represented as ordered characters
** as for the SIMPLE form except that an extra parameter marks a split in
** the data between inclusion and exclusion.
**
** +---+---+---+---+---+---+---+---+---+---+---+
** | n*| m*|  m bytes  | n-m-? bytes       | Next operator
** +---+---+---+---+---+---+---+---+---+---+---+
** n*,m* - might need count size extension as above.
** For example [a-z^aeiou] would be:
** +---+---+---+---+---+---+---+---+---+---+---+
** | 9 | 3 | - | a | z | a | e | i | o | u | Next operator
** +---+---+---+---+---+---+---+---+---+---+---+
** and would match lower case consonants.
** The compound set operators are NSET (1, N, 0-W, 1-W, N-W and N-M)
**
** UNICODE/UTF8/Collation variations
** 
** Essentially the same sequence of operators is available to each form except
** that specific pattern compilers might not generate all the forms of operators.
** Notably the bit set operators are not generated for Unicode/UTF8 when using
** CE data and when it represents string data in either a literal or set operand,
** each character will be represented by the 6 byte, level 3 CE entry for the
** compare. UCS_BASIC Unicode just uses the 2 byte codepoints raw. In SET & NSET
** these are sorted in collation order as for the other char forms so that the
** set match can be curtailed early based on the ordering.
** The Unicode pattern compiler must also ensure that the data is aligned. To do
** this, the compiler must pad with a PAT_NOP if needed. As all numbers are odd
** in size (see above) and an opcode is 1 byte long, many instructions will
** already be i2 aligned.
**
** With Unicode, there is no special case for the '-' character needed as the
** illegal collation weight/character of 0xFFFF is used. As for the '-' above,
** the 0xFFFF (range introducer) will be followed by the bound characters.
** The Unicode CE 'chars' will be the 6 byte
**
** Misc OPS:
** A number of operations require adjusting the PC in some manner. These
** include PAT_JUMP, PAT_LABEL, PAT_FOR_N_M and PAT_NEXT(_W)
** Each of these takes a self relative offset that will in effect the number
** of bytes to skip counting from the *next* instruction address.
** The PAT_NEXT instruction are problemmatic as they are usually backward
** jumps and thus the jump operand size itself affects the jump!
** PAT_LABEL is used in alternation and in effect could be a PAT_JUMP except
** that it only serves as a holder for the pattern segment length and will not
** be executed. Making it an instruction eases disassembly though.
*/

/* Numeric operand encodings */
#define OP_i8 ((u_i1)255)
#define OP_i4 ((u_i1)254)
#define OP_i2 ((u_i1)253)

/*
** Macros to support the reading and writing of values with the
** opcode stream
*/
/* GETNUM - read a general number */
#ifdef LITTLE_ENDIAN_INT
/*LITTLE-ENDIAN*/
#define GETNUM(lv) ((lv)++,((lv)[-1])<OP_i2)?(lv)[-1]\
		    :(lv)[-1]==OP_i2?((lv)+=2,(i2)(((\
					(lv)[-1])<<8)|(lv)[-2]))\
		    :(lv)[-1]==OP_i4?((lv)+=4,(i4)(((((((\
					(lv)[-1])<<8)|(lv)[-2])<<8)|\
					(lv)[-3])<<8)|(lv)[-4]))\
		    :((lv)+=8,(i8)(((((((((((((((\
					(lv)[-1])<<8)|(lv)[-2])<<8)|\
					(lv)[-3])<<8)|(lv)[-4])<<8)|\
					(lv)[-5])<<8)|(lv)[-6])<<8)|\
					(lv)[-7])<<8)|(lv)[-8]))
#else
/*BIG-ENDIAN*/
#define GETNUM(lv) ((lv)++,((lv)[-1])<OP_i2)?(lv)[-1]\
		    :(lv)[-1]==OP_i2?((lv)+=2,(i2)(((\
					(lv)[-2])<<8)|(lv)[-1]))\
		    :(lv)[-1]==OP_i4?((lv)+=4,(i4)(((((((\
					(lv)[-4])<<8)|(lv)[-3])<<8)|\
					(lv)[-2])<<8)|(lv)[-1]))\
		    :((lv)+=8,(i8)(((((((((((((((\
					(lv)[-8])<<8)|(lv)[-7])<<8)|\
					(lv)[-6])<<8)|(lv)[-5])<<8)|\
					(lv)[-4])<<8)|(lv)[-3])<<8)|\
					(lv)[-2])<<8)|(lv)[-1]))
#endif
/* NUMSZ - return number of bytes to represent a given number */
#define NUMSZ(v) (((u_i8)(v)<OP_i2)?1:\
	    (I2_MIN<=(i4)(v)&&(i4)(v)<=I2_MAX)?3:\
	    (I4_MIN<=(i4)(v)&&(i4)(v)<=I4_MAX)?5:9)

/* PUTNUM0 - put a zero */
#define PUTNUM0(lv) *(lv)++ = (u_i1)0

/* PUTNUM - write a general number */
#ifdef LITTLE_ENDIAN_INT
/*LITTLE-ENDIAN*/
#define PUTNUM(lv, v) if ((u_i8)(v)<OP_i2){\
                            *(lv)++ = (u_i1)(v);\
			}else if (I2_MIN<=(i4)(v)&&(i4)(v)<=I2_MAX){\
				u_i1 *_n = (u_i1*)&v;\
				*(lv)++ = OP_i2;\
				*(lv)++ = *_n++;\
				*(lv)++ = *_n++;\
			}else if (I4_MIN<=(i4)(v)&&(i4)(v)<=I4_MAX){\
				u_i1 *_n = (u_i1*)&v;\
				*(lv)++ = OP_i4;\
				*(lv)++ = *_n++;\
				*(lv)++ = *_n++;\
				*(lv)++ = *_n++;\
				*(lv)++ = *_n++;\
			}else{\
				u_i1 *_n = (u_i1*)&v;\
				*(lv)++ = OP_i8;\
				*(lv)++ = *_n++;\
				*(lv)++ = *_n++;\
				*(lv)++ = *_n++;\
				*(lv)++ = *_n++;\
				*(lv)++ = *_n++;\
				*(lv)++ = *_n++;\
				*(lv)++ = *_n++;\
				*(lv)++ = *_n++;\
			}
#else
/*BIG-ENDIAN*/
#define PUTNUM(lv, v) if ((u_i8)(v)<OP_i2){\
                            *(lv)++ = (u_i1)(v);\
			}else if (I2_MIN<=(i4)(v)&&(i4)(v)<=I2_MAX){\
				u_i1 *_n = (u_i1*)&v+sizeof(v)-sizeof(i2);\
				*(lv)++ = OP_i2;\
				*(lv)++ = *_n++;\
				*(lv)++ = *_n++;\
			}else if (I4_MIN<=(i4)(v)&&(i4)(v)<=I4_MAX){\
				u_i1 *_n = (u_i1*)&v+sizeof(v)-sizeof(i4);\
				*(lv)++ = OP_i4;\
				*(lv)++ = *_n++;\
				*(lv)++ = *_n++;\
				*(lv)++ = *_n++;\
				*(lv)++ = *_n++;\
			}else{\
				u_i1 *_n = (u_i1*)&v+sizeof(v)-sizeof(i8);\
				*(lv)++ = OP_i8;\
				*(lv)++ = *_n++;\
				*(lv)++ = *_n++;\
				*(lv)++ = *_n++;\
				*(lv)++ = *_n++;\
				*(lv)++ = *_n++;\
				*(lv)++ = *_n++;\
				*(lv)++ = *_n++;\
				*(lv)++ = *_n++;\
			}
#endif

/*
** Typedefs for functions
*/
typedef DB_STATUS ADU_PATCOMP_FUNC(
	ADF_CB		*adf_scb,
	DB_DATA_VALUE	*pat_dv,
	DB_DATA_VALUE	*esc_dv,
	u_i4		pat_flags,
	AD_PAT_SEA_CTX	*sea_ctx);

typedef DB_STATUS ADU_PATEXEC_FUNC(
	AD_PAT_SEA_CTX *sea_ctx,
	struct _AD_PAT_DA_CTX *da_ctx,
	i4 *rcmp);

/*
** Routines implemented in adupatexec.c
*/
ADU_PATEXEC_FUNC adu_pat_execute;
ADU_PATEXEC_FUNC adu_pat_execute_utf8; /* AD_PAT2_UTF8_UCS_BASIC */
ADU_PATEXEC_FUNC adu_pat_execute_col; /* AD_PAT2_COLLATE */
ADU_PATEXEC_FUNC adu_pat_execute_uni; /* AD_PAT2_UCS_BASIC */
ADU_PATEXEC_FUNC adu_pat_execute_uniCE; /* AD_PAT2_UNICODE_CE */

/*
** Routines implemented in adupatcomp.c
*/
ADU_PATCOMP_FUNC adu_patcomp_like;
ADU_PATCOMP_FUNC adu_patcomp_like_uni;

VOID adu_patcomp_free(AD_PAT_SEA_CTX *sea_ctx);
bool adu_pat_decode(
	u_i1 **pc1,
	i4 *N,
	i4 *M,
	i4 *L,
	u_i1 **S,
	u_i1 **B);

DB_STATUS adu_patcomp_set_pats (
	AD_PAT_SEA_CTX *sea_ctx);

#if PAT_DBG_TRACE>0
bool adu_pat_disass(
	char buf[256],
	u_i4 buflen,
	u_i1 **pc1,
	char **p1,
	i8 *v1,
	bool uni);
VOID adu_patcomp_dump (
	PTR	data,
	char	*fname);
#endif

/*
** Routines implemented in adupatda.c
*/

DB_STATUS adu_patda_init(
	ADF_CB		*adf_scb,
	DB_DATA_VALUE	*src_dv,
	AD_PAT_SEA_CTX *sea_ctx,
	AD_PAT_DA_CTX	*dv_ctx);

DB_STATUS adu_patda_term(AD_PAT_DA_CTX *dv_ctx);

DB_STATUS adu_patda_get(AD_PAT_DA_CTX *dv_ctx);
void adu_patda_ucs2_lower_1(ADUUCETAB*, const UCS2**, const UCS2*, UCS2**);
void adu_patda_ucs2_upper_1(ADUUCETAB*, const UCS2**, const UCS2*, UCS2**);
void adu_patda_ucs2_minmax_1(ADUUCETAB*, const UCS2**, const UCS2*, UCS2**, UCS2**);

extern i4 ADU_pat_debug;
extern u_i4 ADU_pat_cmplx_lim;
extern u_i4 ADU_pat_cmplx_max;
extern u_i4 ADU_pat_seg_sz;
#endif /*_ADUPATEXEC_H_INC*/
