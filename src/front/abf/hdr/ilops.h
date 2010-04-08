/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	ilops.h -	OSL Intermediate Language Operator Definitions.
**
** Description:
**	Contains the definitions of the operators for the OSL IL.
**
** History:
**	Revision 5.1  86/10/17  16:33:49  arthur
**	Initial revision (86/10/01).
**
**	Revision 6.0  87/06  wong
**	Added SQL operators.
**	Added DB value operator and 3 element list operator.
**	Added `old-style' set/inquire FRS operators.
**	Added labelled statement header operator.
**
**	Revision 6.1  88/11  wong
**	Added IL_ACTIMOUT.
**	Added STAR support.  88/11  billc.
**	Added IL_FRSOPT, IL_COLENT, IL_FLDENT operators.  IL_SCROLL 05/88.
**
**	Revision 6.2  89/05  wong
**	Added submenu support, IL_BEGSUBMU, IL_BEGDISPMU, and IL_SUBMENU.
**
**	Revision 6.3  90/01  wong
**	Added IL_SAVPT_SQL for JupBug #4567 and IL_COPY_SQL for JupBug #9734.
**	(Also, for compatability, all 6.3/03/00 operators were added as well 
**	from 150 on up.)
**
**    	13-feb-1990 (Joe)
**          Added IL_QID.
**
**	Revision 6.3/03  90/09  wong
**	Added IL_CREATEUSER through IL_ALTERTABLE.  Replaced STAR LINK operators
**	(which are unused in rel. 6) with IL_DBVSTR and IL_EXIMMEDIATE.
**
**	Revision 6.3/03/01
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL).
**	
**	Revision 6.4
**	3/91 (Mike S)
**		Add LOADTABLE
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Add CALLPL, LOCALPROC, MAINPROC.
**	04/22/91 (emerson)
**		Modifications for alerters (added IL_ACTALERT and IL_DBSTMT).
**	05/03/91 (emerson)
**		Modifications for alerters: handle GET EVENT properly.
**		(It needs a special call to LIBQ and thus a special IL op code:
**		IL_GETEVENT).
**	05/07/91 (emerson)
**		Added IL_NEXTPROC statement (for codegen).
**
**	Revision 6.4/02
**	11/07/91 (emerson)
**		Added ARRAYREF, RELEASEOBJ, and ARRENDUNLD opcodes,
**		and new modifier flags for ARRAYREF and DOT
**		(for bugs 39581, 41013, and 41014).
**
**	Revision 6.4/03
**	09/20/92 (emerson)
**		Added QRYROWGET, QRYROWCHK, and CLRREC (for bug 39582).
**		Added ILM_BRANCH_ON_SUCCESS (for bug 46761).
**
**	Revision 6.5
**	25-aug-92 (davel)
**		Add modifier flag ILM_EXEPROC for EXECUTE PROCEDURE.
**	24-sep-92 (davel)
**		Add opcodes for IL_CONNECT & IL_DISCONNECT for multi-session
**		support.
**	10-nov-92 (davel)
**		Add opcode for IL_CHKCONNECT for more multi-session support.
**	02-feb-93 (davel)
**		Add modifier flag for ILM_HAS_CELL_ATTR for loadtable and 
**		insertrow.
**	09-feb-93 (davel)
**		Add IL_SET4GL and IL_INQ4GL for new set_4gl & inquire_4gl 
**		statements.
**	07/28/93 (dkh) - Added support for the PURGETABLE and RESUME
**			 NEXTFIELD/PREVIOUSFIELD statements.
**	3-jul-1996 (angusm)
**		Add IL_DGTT, increase IL_MAXOP (bug 75153)
**	07-Jan-1999 (kosma01)
**		Remove nested comments.
**	29-Jan-2007 (kiria01) b117277
**	    Added IL_SET_RANDOM
*/

/*}
** Name:	IL_OP -	OSL Intermediate Language Operators
*/
# define	IL_EOF		0
# define	IL_ABORT	1
# define	IL_APPEND	2
# define	IL_ASSIGN	3
# define	IL_BEGLIST	4
# define	IL_BEGMENU	5
# define	IL_BEGDISPMU	6
# define	IL_BEGTRANS	7
# define	IL_CALLF	8
# define	IL_CALLP	9
# define	IL_CALSUBSYS	10
# define	IL_CALSYS	11
# define	IL_DBVAL	12
# define	IL_CLRALL	13
# define	IL_CLRFLD	14
# define	IL_CLRROW	15
# define	IL_CLRSCR	16
# define	IL_COLACT	17
# define	IL_COPY		18
# define	IL_CREATE	19
# define	IL_DBCONST	20
# define	IL_DBVAR	21
# define	IL_DELETE	22
# define	IL_DELROW	23
# define	IL_DESINTEG	24
# define	IL_DESPERM	25
# define	IL_DESTROY	26
# define	IL_DISPLAY	27
# define	IL_DISPLOOP	28
# define	IL_DUMP		29
# define	IL_DYCALLOC	30
# define	IL_DYCCAT	31
# define	IL_DYCFREE	32
# define	IL_ENDLIST	33
# define	IL_ENDMENU	34
# define	IL_ENDTRANS	35
# define	IL_ENDUNLD	36
# define	IL_EXIT		37
# define	IL_EXPR		38
# define	IL_FLDACT	39
# define	IL_FLDENT	40
# define	IL_GETFORM	41
# define	IL_GETROW	42
# define	IL_GOTO		43
# define	IL_OINQFRS	44
# define	IL_OSETFRS	45
# define	IL_COLENT	46
# define	IL_MODE		47
# define	IL_HLPFILE	48
# define	IL_HLPFORMS	49
# define	IL_IF		50
# define	IL_INDEX	51
# define	IL_INITIALIZE	52
# define	IL_INITTAB	53
# define	IL_INQELM	54
# define	IL_INQFRS	55
# define	IL_INQING	56
# define	IL_INSROW	57
# define	IL_TL3ELM	58
# define	IL_INTEGRITY	59
# define	IL_KEYACT	60
# define	IL_LEXPR	61
# define	IL_MENUITEM	62
# define	IL_MENULOOP	63
# define	IL_MESSAGE	64
# define	IL_MODIFY	65
# define	IL_MOVCONST	66
/* aliases for type-specific move constant */
# define	IL_MVICON	IL_MOVCONST
# define	IL_MVFCON	IL_MOVCONST
# define	IL_MVSCON	IL_MOVCONST
# define	IL_NEPROMPT	67
# define	IL_PERMIT	68
# define	IL_POPSUBMU	69
# define	IL_LSTHD	70
# define	IL_PARAM	71
# define	IL_PRNTSCR	72
# define	IL_PROMPT	73
# define	IL_PUTFORM	74
# define	IL_PUTROW	75
# define	IL_PVELM	76
# define	IL_QRY		77
# define	IL_QRYBRK	78
# define	IL_QRYCHILD	79
# define	IL_QRYEND	80
# define	IL_QRYFREE	81
# define	IL_QRYGEN	82
# define	IL_QRYNEXT	83
# define	IL_QRYPRED	84
# define	IL_QRYSINGLE	85
# define	IL_QRYSZE	86
# define	IL_FRSOPT	87
# define	IL_RANGE	88
# define	IL_REDISPLAY	89
# define	IL_RELOCATE	90
# define	IL_REPLACE	91
# define	IL_RESCOL	92
# define	IL_RESFLD	93
# define	IL_RESMENU	94
# define	IL_RESNEXT	95
# define	IL_RESUME	96
# define	IL_RETFRM	97
# define	IL_RETINTO	98
# define	IL_RETPROC	99
# define	IL_SAVE		100
# define	IL_SAVEPOINT	101
# define	IL_SET		102	/* QUEL DBMS */
# define	IL_SETFRS	103	/* Forms System */
# define	IL_SETSQL	104	/* SQL DBMS */
# define	IL_SLEEP	105
# define	IL_START	106
# define	IL_STHD		107
# define	IL_STOP		108
# define	IL_TL1ELM	109
# define	IL_TL2ELM	110
# define	IL_UNLD1	111
# define	IL_UNLD2	112
# define	IL_VALFLD	113
# define	IL_VALIDATE	114
# define	IL_VALROW	115
# define	IL_VIEW		116

# define	IL_DBVSTR	117
# define	IL_EXIMMEDIATE	118	/* EXECUTE IMMEDIATE (SQL) */

/*# define	IL_??		119	 (unused) */
/*# define	IL_??		120	 (unused) */

/* Distributed Database Operator */

# define	IL_DIREXIMM	121	/* DIRECT EXECUTE IMMEDIATE */

/*# define	IL_??		122	 (unused) */

/* SQL Operators */

# define	IL_INSERT	123
# define	IL_DELETEFRM	124
# define	IL_UPDATE	125
# define	IL_COMMIT	126
# define	IL_ROLLBACK	127
# define	IL_DRPTABLE	128
# define	IL_DRPINDEX	129
# define	IL_DRPVIEW	130
# define	IL_DROP		131
# define	IL_DRPINTEG	132
# define	IL_DRPPERMIT	133
# define	IL_CRTTABLE	134
# define	IL_CRTINDEX	135
# define	IL_CRTUINDEX	136
# define	IL_CRTVIEW	137
# define	IL_CRTPERMIT	138
# define	IL_CRTINTEG	139
# define	IL_GRANT	140
# define	IL_SQLMODIFY	141

/* Additional operators (after 6.0) */

# define	IL_SCROLL	142

/* more Distributed Database Operators */
# define	IL_REGISTER	143	/* REGISTER */
# define	IL_REMOVE	144	/* REMOVE */
# define	IL_DIRCONN	145	/* DIRECT CONNECT */
# define	IL_DIRDIS	146	/* DIRECT DISCONNECT */

/* Activate ON TIMEOUT */
# define	IL_ACTIMOUT	147

# define	IL_BEGSUBMU	148
# define	IL_SUBMENU	149

/* RESUME ENTRY opcode */
# define        IL_RESENTRY	150

/* Access record members. */
#define		IL_DOT		151
/* Access array elements. */
#define		IL_ARRAY	152

/* Set INGRES */
#define		IL_SETING	153

/* TERMINATOR I additions for the "Knowledge Management Extension" package */

# define        IL_ALTERGROUP	154
# define        IL_ALTERROLE	155
# define        IL_CREATEGROUP	156
# define        IL_CREATEROLE	157
# define        IL_CREATERULE	158
# define        IL_DRPRULE	159
# define        IL_DRPPROC	160
# define        IL_DRPGROUP	161
# define        IL_DRPROLE	162
# define        IL_REVOKE	163

/* Array-row operations. */
# define        IL_ARRCLRROW	164
# define        IL_ARRDELROW	165
# define        IL_ARRINSROW	166
# define        IL_ARR1UNLD	167
# define        IL_ARR2UNLD	168

/* SQL DDL statements */
# define	IL_SAVPT_SQL	169
# define	IL_COPY_SQL	170

/* Repeat query IDs */
# define      	IL_QID		171

/* Terminator II */

# define	IL_CREATEUSER	172
# define	IL_ALTERUSER	173
# define	IL_DRPUSER	174
# define	IL_ALTERTABLE	175

# define	IL_LOADTABLE	176

# define	IL_CALLPL	177
# define	IL_LOCALPROC	178
# define	IL_MAINPROC	179

# define	IL_ACTALERT	180
# define	IL_DBSTMT	181
# define	IL_GETEVENT	182

# define	IL_NEXTPROC	183

# define	IL_ARRAYREF	184
# define	IL_RELEASEOBJ	185
# define	IL_ARRENDUNLD	186

# define	IL_QRYROWGET	187
# define	IL_QRYROWCHK	188

# define	IL_CLRREC	189

# define	IL_CONNECT	190
# define	IL_DISCONNECT	191
# define	IL_CHKCONNECT	192

# define	IL_SET4GL	193
# define	IL_INQ4GL	194

# define	IL_RESNFLD	195
# define	IL_RESPFLD	196
# define	IL_PURGETBL	197

#define		IL_DGTT		198

# define	IL_SET_RANDOM	199
/* Maximum Operator Code */

# define	IL_MAX_OP	199

/*
** Modifiers for statements.
**
** Ops are in an i2 so we use large values for the modifiers.
** The largest opcode is less than 0x100 (256) so we use larger values
** for our modifiers.
*/

/*
** This can modify all statements with SIDs:
** It says that the sid value is the id for an i4 constant that is really
** the offset to the statement.
*/
# define	ILM_LONGSID	0x1000

/*
** This can modify statements with SIDs, which, by default, branch on "failure":
** It says that the branch (to the sid value) should instead be taken on "success"
** (control should fall through on failure).  Currently (19-sep-92) this bit is set
** only for the IL_QRYNEXT opcode.
*/
# define	ILM_BRANCH_ON_SUCCESS	0x2000

/*
** The following 3 flags can modify ARRAYREF or DOT.
** abfrt!abrtclas.c includes this header to get these 3 flags;
** it depends on the fact that neither ILM_RLS_SRC nor ILM_HOLD_TARG is 1.
*/
# define	ILM_LHS		0x1000	/* ARRAYREF is for a LHS */

# define	ILM_RLS_SRC	0x2000	/* Source temp is a complex object
					** which should be "released"
					** (reference count decremented).
					*/
# define	ILM_HOLD_TARG	0x4000	/* Target temp is a complex object
					** which should be "held"
					** (reference count incremented).
					*/
/*
** This can modify an IL_CALLP:
** It says that a database procedure is being called via EXECUTE PROCEDURE.
*/
# define	ILM_EXEPROC	0x1000

/*
** This can modify an IL_INSROW or an IL_LOADTABLE:
** It says that a cell attribute list follows the column value list.
*/
# define	ILM_HAS_CELL_ATTR	0x1000

/*
** This is used to mask out the modifiers
*/
# define	ILM_MASK 	0xff
