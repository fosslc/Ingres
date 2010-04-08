/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** eqgen.h
**	- gen, arg, and out utilities
**	- Users must include <eqsym.h> before this.
**
** History:
**      ?-?-?
**          Created
**      09-aug-1990 (barbara)
**          Added a constant IIFRSTATTR for code generation of IIFRsaSetAttrio.
**	15-mar-1991 (kathryn) Added:
**	    IISETSQL	for code generation of	IILQssSetSqlio
**	    IIINQSQL    for code generation of	IILQisInqSqlio
**	    IISETHDL	for code generation of	IILQshSetHandler
**	    IIINQHDL	for code generation of	IILQihInqHandler
**	17-apr-1991 (teresal) 
**	    Add constant IIFRSEVENT for FRS "ACTIVATE EVENT".  Also 
**	    took away constant IIEVTGET because IILQegEvGetio will
**	    not be part of the generated runtime interface but will only be
**	    called internally by LIBQ.
**	14-oct-1992 (lan)
**	    Add IIENDDATA for EXEC SQL ENDDATA statement.
**	14-oct-1992 (larrym)
**	    Added new functions IISQLCDINIT, to support FIPS 
**	    SQLCODE.  Also added a copyright notice.
**	01-nov-1992 (kathryn)
**	    Add constants -IIGETDATA - IIPUTDATA for 
**	    EXEC SQL GET DATA and EXEC SQL PUT DATA statements.
**	04-nov-1992 (larrym)
**	    added IISQGDINIT to support SQLSTATE and GET DIAGNOSTIC.
**	09-nov-1992 (lan)
**	    Added IIPUTDATAHDLR and IIGETDATAHDLR for DATAHANDLER.
**	16-nov-1992 (lan)
**	    Removed IIPUTDATAHDLR and IIGETDATAHDLR.  Replaced them with
**	    IIDATAHDLR.
**	20-nov-1992 (Mike S)
**	    Add codes for EXEC 4GL functions
**	2-dec-1992 (Mike S)
**	    Add code for send userevent
**	14-dec-1992 (lan)
**	    Added G_DELSTRING for delimited strings.
**	10-feb-1993 (larrym)
**	    removed IISQGDINIT, don't need it anymore
**	09-mar-1993 (larrym) 
**	    added IISQCNNM for connection name support
**	22-jun-1993 (kathryn)
**	    added prototype for esq_sqmods.
**	07/28/93 (dkh) - Added support for the PURGETABLE and RESUME
**			 NEXTFIELD/PREVIOUSFIELD statements.
**	17-sep-1993 (kathryn)
**	    Remove IIINQHDL definition.  This was for code generation of
**	    INQUIRE_SQL of error,copy,message and dbevent handlers.
**	    This was never completed, and will not be supported.
**	17-Dec-93 (donc) Bug 56875
**	    Added ARG_PSTRUCT and arg_pstruct_add
**	22-dec-1993 (teresal)
**	    Added protoype for esq_eoscall() routine for support of 
**	    the "-check_eos" flag. Bug fix 55915.
**      12-Dec-95 (fanra01)
**          Changed extern to GLOBALREF and added definitions for referencing
**          data in DLLs on windows NT from code built for static libraries.
**      06-Feb-96 (fanra01)
**          Changed flag from __USE_LIB__ to IMPORT_DLL_DATA.
**	06-mar-1996 (thoda04)
**	    Added function and argument prototypes for functions. 
**	2-may-97 (inkdo01)
**	    Added IIPROCGTTP to generate global temp table proc parms.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	6-Jun-2006 (kschendel)
**	    Added IISQDESCINPUT for DESCRIBE INPUT.
**	12-feb-2007 (dougi)
**	    Added IICSRETSCROLL/OPENSCROLL for scrollable cursors.
**	26-Aug-2009 (kschendel) b121804
**	    Remove peculiar dbg-comment strangeness.
**	    Drop the old WIN16 section, misleading and out of date.  Better
**	    to just reconstruct as needed.
*/

/********************** begin gen utility **********************************/

/*
** OR the function index with GEN_DML to use the "alternate" gen table
** "dml->dm_gentab"; if there is none then "gen_xx" should produce an error.
** Don't let any index be this large (I know you want to ...).  Don't forget
** to cast "dml->dm_gentab"; by default it is a "nat *", which is almost
** surely wrong.
**
** sample code:
**
**	if (index & GEN_DML)
**	    if (dml->dm_gentab)
**		tbl_ptr = &((GEN_IIFUNC *)(dml->dm_gentab))[index & ~GEN_DML];
**	    else
**		er_write( EgenNOTBL, index & ~GEN_DML );
**	else
**	    tbl_ptr = &gen__ftab[index];
*/

# define	GEN_DML		0x8000

/*
** Constants used for generating a function call, and for looking up
** into a table for printing the names of the functions.
** Note that they must be sequential.
** NEVER use (II0 | GEN_DML)!
*/

# define	II0		0

/* 
** Quel routines set up data for database statements, and correspond to
** Quel rules in the grammar
*/
# define	IIFLUSH		1
# define 	IINEXEC 	2
# define	IINEXTGET	3
# define	IINOTRIM	4
# define	IIPARRET	5
# define	IIPARSET	6
# define 	IIRETDOM 	7
# define	IIRETINIT	8
# define	IISETDOM	9
# define 	IISEXEC 	10
# define	IISYNCUP	11
# define	IITUPGET	12
# define	IIWRITEDB	13
# define	IIXACT		14
# define	IIVARSTRING	15
# define	IISETSQL	16
# define	IIINQSQL	17
# define	IISETHDL	18

/* 
** Routines that interface only with Libq and correspond to Libq rules
** in the grammar
*/
# define	IIBREAK		21
# define	IIEQINQ		22
# define	IIEQSET		23
# define 	IIERRTEST 	24
# define	IIEXIT		25
# define	IIINGOPEN	26
# define	IIUTSYS		27
# define	IIEXEXEC	28		/* For repeat queries */
# define	IIEXDEFINE	29		/* For repeat queries */

/* Regular form's field manipulation routines corresponding to Forms rules */
# define 	IIACTCLM 	31
# define 	IIACTCOMM 	32
# define 	IIACTFLD	33
# define 	IIACTMU 	34
# define 	IIACTSCRL 	35
# define	IIADDFORM	36
# define	IICHKFRM	37
# define	IICLRFLDS	38
# define	IICLRSCR	39
# define	IIDISPFRM	40
# define	IINPROMPT	41
# define	IIENDFORMS	42
# define	IIENDFRM	43
# define 	IIENDMU 	44
# define	IIFDATA		45
# define	IIFLDCLEAR	46
# define	IIFNAMES	47
# define	IIFORMINIT	48
# define	IIFORMS		49
# define	IIFRSINQ	50
# define	IIFRSSET	51
# define	IIFSETIO	52
# define	IIGETOPER	53
# define	IIGETQRY	54
# define	IIHELPFILE	55
# define 	IIINITMU 	56
# define	IIINQSET	57
# define	IIMESSAGE	58
# define	IIMUONLY	59
# define	IIPUTOPER	60
# define	IIREDISP	61
# define	IIRESCOL	62
# define	IIRESFLD	63
# define	IIRESNEXT	64
# define	IIRETFIELD	65
# define	IIRETVAL	66
# define	IIRF_PARAM	67
# define	IIRUNFRM	68
# define	IIRUNMU		69
# define	IISETFIELD	70
# define	IISF_PARAM	71
# define	IISLEEP		72
# define	IIVALFLD	73
# define	IIACTFRSK	74	/* Function key addition */
# define	IIPRNSCR	75	/* Printscreen addition */
# define	IIIQSET		76	/* New INQUIRE/SET routines (4.0) */
# define	IIIQFRS		77
# define	IISTFRS		78
# define	IIRESMU		79	/* Resume menu addition */
# define	IIFRSHELP	80	/* Internal help (file/keys) utility */
# define	IINESTMU	81	/* 6.0 DISPLAY SUBMENU */
# define	IIRUNNEST	82	/* 6.0 DISPLAY SUBMENU */
# define	IIENDNEST	83	/* 6.0 DISPLAY SUBMENU */
# define	IIACTTIME	84	/* ACTIVATE TIMEOUT */
# define	IIFRISTIME	85	/* Is TIMEOUT on? */

/* Table field manipulation routines corresponding to Table field rules */
# define	IITBACT		86
# define	IITBINIT	87
# define	IITBSETIO	88
# define	IITBSMODE	89
# define	IITCLRCOL	90
# define	IITCLRROW	91
# define	IITCOLRET	92
# define	IITCOLSET	93
# define	IITCOLVAL	94
# define	IITDATA		95
# define	IITDATEND	96
# define	IITDELROW	97
# define	IITFILL		98
# define	IITHIDECOL	99
# define	IITINSERT	100
# define	IITRC_PARAM	101
# define	IITSC_PARAM	102
# define	IITSCROLL	103
# define	IITUNEND	104
# define	IITUNLOAD	105
# define	IITVALROW	106
# define	IITVALVAL	107
# define	IITBCECOLEND	108	/* No more column values to be set*/

/* Gen calls to QA routines for testing */
# define	QAMESSAGE	111
# define	QAPRINTF	112
# define	QAPROMPT	113

/* EQUEL cursor routines */
# define	IICSRETRIEVE	120
# define	IICSOPEN	121
# define	IICSQUERY	122
# define	IICSGET		123
# define	IICSCLOSE	124
# define	IICSDELETE	125
# define	IICSREPLACE	126
# define	IICSERETRIEVE	127
# define	IICSPARGET	128
# define	IICSEREPLACE	129
# define	IICSRDO		130
# define	IICSRETSCROLL	131
# define	IICSOPENSCROLL	132

/* IIFR entries */
# define	IIFRGPCONTROL	135
# define	IIFRGPSETIO	136
# define	IIFRRERESENTRY	137
# define	IIFRINTERNAL	138	/* Only defined in C preprocessors
					** indicates '-c' command flag is on */
# define	IIFRSTATTR	139
# define	IIACTEVENT	140
# define	IIFRNXTPRVFLD	141	/* resume nextfield/previousfield */


/* stored-procedure support */
# define	IIPUTCTRL	145
# define	IIPROCINIT	146
# define	IIPROCVALIO	147
# define	IIPROCSTAT	148
# define	IIPROCGTTP	149

/* event routines */
# define	IIEVTSTAT	152

/* large objects handling routines */
# define	IIDATAHDLR	155
# define	IIGETDATA       156
# define	IIPUTDATA       157
# define	IIENDDATA	158

/*
# define	IICSEXOPEN	130
# define	IICSEXDEFINE	149
*/

/* EXEC 4GL routines */
# define	IIG4CLRARR	165
# define	IIG4REMROW	166
# define	IIG4INSROW	167
# define	IIG4DELROW	168
# define	IIG4SETROW	169
# define	IIG4GETROW	170
# define	IIG4GETGLOB	171
# define	IIG4SETGLOB	172
# define	IIG4GETATTR	173
# define	IIG4SETATTR	174
# define	IIG4CHKOBJ	175
# define	IIG4USEDESCR	176
# define	IIG4DESCRIBE	177
# define	IIG4INITCALL	178
# define	IIG4VALPARAM	179
# define	IIG4BYREFPARAM	180
# define	IIG4RETVAL	181
# define	IIG4CALLCOMP	182
# define	IIINQ4GL	183
# define	IISET4GL	184
# define	IISENDUSEREVENT	185

/* Run-time dynamic ESQL cursor routines */
# define	IICSDAGET	(GEN_DML | 1)
/* Leave 4 holes for new routines (2, 3, 4, 5) */

/* Run-time ESQL routines */
# define	IISQCONNECT	(GEN_DML | 6)
# define	IISQEXIT	(GEN_DML | 7)
# define	IISQFLUSH	(GEN_DML | 8)
# define	IISQINIT	(GEN_DML | 9)
# define	IISQSTOP	(GEN_DML | 10)
# define	IISQUSER	(GEN_DML | 11)
# define	IISQPRINT	(GEN_DML | 12)
# define	IISQMODS	(GEN_DML | 13)
# define	IISQPARMS	(GEN_DML | 14)
# define	IISQTPC		(GEN_DML | 15)
# define	IISQSESS	(GEN_DML | 16)
# define	IISQLCDINIT	(GEN_DML | 17)
# define	IISQCNNM	(GEN_DML | 18)
/* Leave 2 holes for new routines (19 .. 20) */

/* Dynamic SQL routines */
# define	IISQEXIMMED	(GEN_DML | 21)
# define	IISQEXSTMT	(GEN_DML | 22)
# define	IISQDAIN	(GEN_DML | 23)
# define	IISQPREPARE	(GEN_DML | 24)
# define	IISQDESCRIBE	(GEN_DML | 25)
# define	IISQDESCINPUT	(GEN_DML | 26)
/* Leave 4 holes for new routines (27 .. 30) */

/* Dynamic ESQL FRS routines */
# define	IIFRSQDESC	(GEN_DML | 31)
# define	IIFRSQEXEC	(GEN_DML | 32)


/*
** Logical Conditions used by Equel.
**
** These constants are used as lookups into a table of strings.
** Note that Logical opposites are negatives of each other.  This helps Gen 
** to generate loops for labelled languages.  The constants C_RANGE is the 
** needed to add when looking up the corresponding logical string. The entry
** for C_RANGE should have nothing.
*/

# define	C_RANGE		3		/* Add for lookup into table */
# define	C_GTREQ		(-3)
# define	C_LESSEQ	(-2)
# define	C_NOTEQ		(-1)
# define	C_0		0
# define	C_EQ		1
# define	C_GTR		2
# define	C_LESS		3

/*
** Labels used by Equel - are also used as constants either to generate integer
** labels or as lookups into strings.
*/

# define	L_0		0

# define 	L_FDINIT 	1
# define 	L_FDBEG 	2
# define 	L_FDFINAL 	3
# define 	L_FDEND 	4

# define 	L_MUINIT 	5

# define	L_ACTELSE	6
# define	L_ACTEND	7

# define 	L_BEGREP 	8
# define 	L_ENDREP 	9

# define 	L_RETBEG 	10
# define 	L_RETFLUSH 	11
# define 	L_RETEND 	12

# define 	L_TBBEG 	13
# define 	L_TBEND 	14

/* Constants to tell if openning, closing or else'ing a structured block */
# define	G_OPEN		0
# define	G_CLOSE		1
# define	G_ELSE		2

/* 
** Constants to tell if label is attached to loop - if using a labelled
** language then we do not want to print it, as the loop does it anyway.
*/
# define	G_NOLOOP	0
# define	G_LOOP		1

/* 
** Constants to tell if goto needs an 'if' if front of it, or a 'terminator'
** after it.
*/
# define	G_IF		0
# define	G_TERM		1
# define	G_NOTERM	2

/* Flags whether string generation is a nested string (DB) or regular string */
# define 	G_INSTRING 	0
# define 	G_REGSTRING 	1
# define	G_DELSTRING	2

/* Flags to allow specify different host language generation - declarations */
# define	G_H_INDENT	0001
# define	G_H_OUTDENT	0002
# define	G_H_NEWLINE	0004
# define	G_H_KEY		0010
# define	G_H_OP		0020
# define	G_H_SCONST	0040
# define	G_H_CODE	0100
# define	G_H_CONTINUE	0200
# define	G_H_NOPRINT	0400	/* Buffered queries turn on/off */
# define	G_H_PRINT	01000	/* 	    for COBOL 		*/

/* Routines used by the control flow generator */
void	gen_call(i4 func);
void	gen_if(i4 o_c, i4  func, i4  cond, i4  val);
void	gen_else(i4 flag, i4  func, i4  cond, i4  val,
                 i4  l_else, i4  l_elsenum, i4  l_end, i4  l_endnum);
void	gen_if_goto(i4 func, i4  cond, i4  val, i4  labprefix, i4  labnum);
void	gen_loop(i4 o_c, i4  labbeg, i4  labend, i4  labnum,
                 i4  func, i4  cond, i4  val);
void	gen_goto(i4 flag, i4  labprefix, i4  labnum);
void	gen_label(i4 loop_extra, i4  labprefix, i4  labnum);
void	gen_comment(i4 term, char *str);
void	gen_include(char *pre, char *suff, char *extra);
void	gen_eqstmt(i4 o_c, char *cmnt);
void	gen_line(char *s);
void	gen_term(/*varies by language*/);
void	gen_declare( /*varies - either bool or void */ );
char	*gen_makesconst(i4 flag, char *instr, i4  *diff);
char	*gen_sqmakesconst(i4 flag, char *instr, i4  *diff);
void	gen_sconst(char *str);
void	gen_do_indent(i4 i);
void	gen_host(i4 flag, char *s);

/* Export the final code emitter for sc_eat calls */
# define	gen_code	out_emit
/********************** end gen utility ************************************/

/********************** begin arg utility **********************************/
/* Argument data definitons for storers of arguments */
# define	ARG_NOTYPE	0
# define	ARG_INT		1
# define	ARG_FLOAT	2
# define	ARG_CHAR	3
# define	ARG_RAW		4
# define	ARG_PACK	5
# define	ARG_PSTRUCT	6

/*
** Argument managing routines
*/
void	arg_var_add(/* SYM *asym, char *aname */);
void	arg_str_add(i4 atype, char *adata);
void	arg_int_add(i4 adata);
void	arg_float_add(f8 adata);
void	arg_push(void);
void	args_toend(i4 start, i4  end);
i4  	arg_count(void);
i4  	arg_next(void);
char	*arg_name(i4 a);
void	arg_num();
SYM 	*arg_sym(i4 a);
i4  	arg_type(i4 a);
i4  	arg_rep(i4 a);
void	arg_free(void);
void	arg_pstruct_add();


    void	arg_dump(char *calname);
/********************** end arg utility ************************************/

/********************** begin out utility **********************************/
/* Output state partially known outside */

typedef struct {
    i4		(*o_func)(char);    /* Output function to use - private */
    i4		o_charcnt;		/* Char number in line */
    char	o_lastch;		/* Last character */
} OUT_STATE;

# define	OUTNULL		(OUT_STATE *)0

# if defined(NT_GENERIC) && defined(IMPORT_DLL_DATA)
GLOBALDLLREF	OUT_STATE	*out_cur;	/* Current output state */
# else              /* NT_GENERIC && IMPORT_DLL_DATA */
GLOBALREF	OUT_STATE	*out_cur;	/* Current output state */
# endif             /* NT_GENERIC && IMPORT_DLL_DATA */

/*
** Routines defined by the code emitter
*/
OUT_STATE	*out_init_state(OUT_STATE *out_s, i4  (*out_func)(char));
OUT_STATE	*out_use_state(OUT_STATE *out_s);
void		out_emit(char *str);
void		out_erline(i4 l);
i4  		out_put_def(char ch);
void		out_suppress(i4 flg);
/********************** end out utility ************************************/

/*** Prototypes ***/

/* equel/eqmisc.c */

FUNC_EXTERN void 	esq_init(void);
FUNC_EXTERN i4   	esq_repeat(i4 sqflag);
FUNC_EXTERN void 	esq_sqmods(i4 mod_flag);
FUNC_EXTERN void 	esq_eoscall(i4 mod_flag);
FUNC_EXTERN STATUS	esq_geneos(void);

VOID	eqgen_tl(i4 stmt_id);

i4  	esl_query(i4 init);
i4  	esl_clean(void);
