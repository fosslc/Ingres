/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

/*
** Name: gcoint.h - Definitions for compiling Object Descriptors.
**
** History:
**	15-Oct-01 (gordy)
**	    Extracted from gcdcomp.h
**	16-Dec-02 (gordy)
**	    Renamed gcd component to gco.
**	12-May-03 (gordy)
**	    Provide prototype for gco_do_16src().
**	15-Mar-04 (gordy)
**	    Support eight-byte integers.
**	 4-Jan-05 (gordy)
**	    Change gco_do_16src() to take unsigned char references
**	    since they are used as indices into the translation arrays.
**	31-May-06 (gordy)
**	    Support ANSI date/time types.
**	24-Oct-06 (gordy)
**	    Support Blob/Clob locators.
**	 9-Oct-09 (gordy)
**	    Support Boolean type.
**	22-Jan-10 (gordy)
**	    Expose charset translation routines outside gco (moved to 
**	    gcocomp.h).  DOUBLEBYTE code generalized for multi-byte 
**	    charset processing.  Added new operations: OP_END_TUP, 
**	    OP_CHAR_MSG, OP_SAVE_MSG, OP_RSTR_MSG, OP_DV_REF, 
**	    OP_SEG_BEGIN, OP_SEG_END, OP_SEG_CHECK, OP_PUSH_REFI2, 
**	    OP_UPD_REFI2, OP_PUSH_REFI4, OP_UPDI4, OP_CHAR_LIMIT, 
**	    OP_ADJUST_PAD, OP_PAD, OP_POP, OP_SKIP_INPUT, OP_SAVE_CHAR, 
**	    OP_RSTR_CHAR. 
*/

#ifndef GCOINT_H_INCLUDED
#define GCOINT_H_INCLUDED

/* 
** Operation codes for compiled message descriptors.
*/

# define OP_ERROR		0x00	/* error has occured */
# define OP_DONE		0x01	/* complete msg converted */

# define OP_DOWN		0x02	/* decend into complex type */
# define OP_UP			0x03	/* finish complex type */
# define OP_DEBUG_TYPE		0x04	/* trace the current type */
# define OP_DEBUG_BUFS		0x05	/* trace the input/output buffers */

# define OP_JZ			0x06	/* jump if top-of-stack zero */
# define OP_DJNZ		0x07	/* decrement top-of-stack, jump if !0 */
# define OP_JNINP		0x08	/* branch if input depleted */
# define OP_GOTO		0x09	/* branch unconditionally */

# define OP_CALL_DDT		0x0A	/* call DBMS datatype program */
# define OP_CALL_DV		0x0B	/* call data value program */
# define OP_CALL_TUP		0x0C	/* call tuple program */
# define OP_END_TUP		0x0D	/* tuple processing completed */
# define OP_RETURN		0x0E	/* return from program call */

# define OP_PUSH_LIT		0x0F	/* push 2 byte signed quanity */
# define OP_PUSH_I2		0x10	/* push i2 from data stream */
# define OP_PUSH_I4		0x11	/* push i4 from data stream */
# define OP_PUSH_REFI2		0x12	/* push i2 & reference from output */
# define OP_UPD_REFI2		0x13	/* update i2 reference in output */
# define OP_PUSH_REFI4		0x14	/* push i4 & reference from output */
# define OP_UPD_REFI4		0x15	/* update i4 reference in output */
# define OP_PUSH_VCH		0x16	/* compute varchar padding length */
# define OP_CHAR_LIMIT		0x17	/* character length limited (no pad) */
# define OP_ADJUST_PAD		0x18	/* adjust padding for data change */
# define OP_POP			0x19	/* discard top of stack */

# define OP_CV_I1		0x1A	/* convert i1s */
# define OP_CV_I2		0x1B	/* convert i2s */
# define OP_CV_I4		0x1C	/* convert i4s */
# define OP_CV_I8		0x1D	/* convert i8s */
# define OP_CV_F4		0x1E	/* convert f4s */
# define OP_CV_F8		0x1F	/* convert f8s */
# define OP_CV_BYTE		0x20	/* convert bytes */
# define OP_CV_CHAR		0x21	/* convert chars (no output limit) */
# define OP_CV_PAD		0x22	/* convert padding */

# define OP_PAD			0x23	/* output padding */
# define OP_SKIP_INPUT		0x24	/* Skip remaining input */
# define OP_SAVE_CHAR		0x25	/* Save character input */
# define OP_RSTR_CHAR		0x26	/* Restore character input */

# define OP_MSG_BEGIN		0x27	/* begin message */
# define OP_IMP_BEGIN		0x28	/* begin VARIMPAR */
# define OP_COPY_MSG		0x29	/* Copy input to output */
# define OP_CHAR_MSG		0x2A	/* Copy character input to output */
# define OP_SAVE_MSG		0x2B	/* Save message input */
# define OP_RSTR_MSG		0x2C	/* Restore message input */
# define OP_VAR_BEGIN		0x2D	/* begin VAREXPAR */
# define OP_VAR_END		0x2E	/* end VAREXPAR */
# define OP_DV_BEGIN		0x2F	/* begin GCA_DATA_VALUE value */
# define OP_DV_END		0x30	/* end GCA_DATA_VALUE value */
# define OP_DV_ADJUST		0x31	/* Adjust space for GCA_DATA_VALUE */
# define OP_DV_REF		0x32	/* save meta-data length reference */
# define OP_SEG_BEGIN		0x33	/* Begin segments */
# define OP_SEG_END		0x34	/* End of segments */
# define OP_SEG_CHECK		0x35	/* Check for end-of-segments */

# define OP_MARK		0x36	/* save current processing state */
# define OP_REWIND		0x37	/* backup to saved processing state */
# define OP_PAD_ALIGN		0x38	/* allow for alignment pad */
# define OP_DIVIDE		0x39	/* Divide stack by argument */
# define OP_BREAK		0x3A	/* indicate break in tuple stream */


/* 
** Error codes for OP_ERROR.
*/

# define EOP_DEPLETED	0	/* more message and no more descriptor */
# define EOP_NOMSGOD	1	/* no object descriptor for message */
# define EOP_NOTPLOD	2	/* no object descriptor for TPL */
# define EOP_BADDVOD	3	/* GCA_DATA_VALUE elements incorrect */
# define EOP_VARLSTAR	4	/* List elements incorrect */
# define EOP_ARR_STAT	5	/* invalid array status for an element */
# define EOP_NOT_INT	6	/* invalid TPL for explicit array length */
# define EOP_NOT_ATOM	7	/* invalid atomic datatype */


/*
** DBMS datatypes supported by GCO.  In the past, some DBMS types
** were mapped to GC atomic types, but no longer.  Also, some type
** codes represented multiple types distinguished by other factors
** such as length.  Supported DBMS types are now mapped to the
** following internal types, each of which are represented by a 
** single object descriptor (set of GC atomic elements).
** 
** Note that there is both a nullable and non-nullable version for
** each type.  The nullable type must be numerically 1 greater than
** the non-nullable type (DBMS nullable types are represented by
** negative type codes).  Compressed variable length types are also
** assigned their own distinct type codes.
*/

/* Numeric types */

# define GCO_DT_INT1		0	/* Integer (1 byte) */
# define GCO_DT_INT1_N		1
# define GCO_DT_INT2		2	/* Integer (2 byte) */
# define GCO_DT_INT2_N		3
# define GCO_DT_INT4		4	/* Integer (4 byte) */
# define GCO_DT_INT4_N		5
# define GCO_DT_INT8		6	/* Integer (8 byte) */
# define GCO_DT_INT8_N		7
# define GCO_DT_FLT4		8	/* Float (4 byte) */
# define GCO_DT_FLT4_N		9
# define GCO_DT_FLT8		10	/* Float (8 byte) */
# define GCO_DT_FLT8_N		11
# define GCO_DT_DEC		12	/* Decimal */
# define GCO_DT_DEC_N		13
# define GCO_DT_MONEY		14	/* Money */
# define GCO_DT_MONEY_N		15

/* String types */

# define GCO_DT_C		16	/* C */
# define GCO_DT_C_N		17
# define GCO_DT_CHAR		18	/* Char */
# define GCO_DT_CHAR_N		19
# define GCO_DT_VCHR		20	/* Varchar */
# define GCO_DT_VCHR_N		21
# define GCO_DT_VCHR_C		22	/* Varchar (compressed) */
# define GCO_DT_VCHR_CN		23
# define GCO_DT_LCHR		24	/* Long varchar */
# define GCO_DT_LCHR_N		25
# define GCO_DT_LCLOC		26	/* Long varchar locator */
# define GCO_DT_LCLOC_N		27
# define GCO_DT_BYTE		28	/* Byte */
# define GCO_DT_BYTE_N		29
# define GCO_DT_VBYT		30	/* Varbyte */
# define GCO_DT_VBYT_N		31
# define GCO_DT_VBYT_C		32	/* Varbyte (compressed) */
# define GCO_DT_VBYT_CN		33
# define GCO_DT_LBYT		34	/* Long byte */
# define GCO_DT_LBYT_N		35
# define GCO_DT_LBLOC		36	/* Long byte locator */
# define GCO_DT_LBLOC_N		37
# define GCO_DT_TEXT		38	/* Text */
# define GCO_DT_TEXT_N		39
# define GCO_DT_TEXT_C		40	/* Text (compressed) */
# define GCO_DT_TEXT_CN		41
# define GCO_DT_LTXT		42	/* Long text */
# define GCO_DT_LTXT_N		43
# define GCO_DT_LTXT_C		44	/* Long text (compressed) */
# define GCO_DT_LTXT_CN		45
# define GCO_DT_QTXT		46	/* Query text */
# define GCO_DT_QTXT_N		47
# define GCO_DT_NCHAR		48	/* Char (UCS2) */
# define GCO_DT_NCHAR_N		49
# define GCO_DT_NVCHR		50	/* Varchar (UCS2) */
# define GCO_DT_NVCHR_N		51
# define GCO_DT_NVCHR_C		52	/* Varchar (UCS2,compressed) */
# define GCO_DT_NVCHR_CN	53
# define GCO_DT_NLCHR		54	/* Long varchar (UCS2) */
# define GCO_DT_NLCHR_N		55
# define GCO_DT_LNLOC		56	/* Long varchar (UCS2) locator */
# define GCO_DT_LNLOC_N		57
# define GCO_DT_NQTXT		58	/* Query text (UCS2) */
# define GCO_DT_NQTXT_N		59

/* Abstract types */

# define GCO_DT_IDATE		60	/* Date */
# define GCO_DT_IDATE_N		61
# define GCO_DT_ADATE		62
# define GCO_DT_ADATE_N		63
# define GCO_DT_TIME		64	/* Time */
# define GCO_DT_TIME_N		65
# define GCO_DT_TMWO		66
# define GCO_DT_TMWO_N		67
# define GCO_DT_TMTZ		68
# define GCO_DT_TMTZ_N		69
# define GCO_DT_TS		70	/* Timestamp */
# define GCO_DT_TS_N		71
# define GCO_DT_TSWO		72
# define GCO_DT_TSWO_N		73
# define GCO_DT_TSTZ		74
# define GCO_DT_TSTZ_N		75
# define GCO_DT_INTYM		76	/* Interval */
# define GCO_DT_INTYM_N		77
# define GCO_DT_INTDS		78
# define GCO_DT_INTDS_N		79
# define GCO_DT_BOOL		80	/* Boolean */
# define GCO_DT_BOOL_N		81
# define GCO_DT_LKEY		82	/* Logical key */
# define GCO_DT_LKEY_N		83
# define GCO_DT_TKEY		84	/* Table key */
# define GCO_DT_TKEY_N		85
# define GCO_DT_SECL		86	/* Security label */
# define GCO_DT_SECL_N		87


/* 
** Global data declaration 
*/

GLOBALREF       i4               gco_trace_level;
GLOBALREF	GCA_OBJECT_DESC *gco_od_data_value;

GLOBALREF	GCA_OBJECT_DESC *gco_msg_ods[];
GLOBALREF	i4		gco_l_msg_ods;
GLOBALREF	GCO_OP		**gco_msg_map;

GLOBALREF	GCA_OBJECT_DESC	*gco_ddt_ods[];
GLOBALREF	i4		gco_l_ddt_ods;
GLOBALREF	GCO_OP		**gco_ddt_map;

GLOBALREF	GCO_OP		*gco_tuple_prog;


/*
** Internal functions
*/

FUNC_EXTERN	bool	gco_comp_tdescr( i4  flags, i4 count, 
					 PTR desc, bool eod, GCO_OP **pc );

FUNC_EXTERN	VOID	gco_comp_dv( GCA_ELEMENT *ela, GCO_OP *in_pc );

FUNC_EXTERN	u_i2	gco_align_pad( GCA_ELEMENT *ela, u_i2 offset );

#endif

