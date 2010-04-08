/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
**      Equel Data
**
**  Description
**      File added to hold all GLOBALDEFs in front equel facility.
**      This is for use in DLLs only.
**
**  History:
**      14-Nov-95 (fanra01)
**          Created.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-oct-2001 (somsa01)
**	    Moved define of yyreswd from esqyylex.c to here.
**	23-oct-2001 (somsa01)
**	    Once again moved yyreswd to eqglobs.c.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPEMBEDLIBDATA
**	    in the Jamfile. Added LIBOBJECT hint which creates a rule
**	    that copies specified object file to II_SYSTEM/ingres/lib
*/

/*
**
LIBRARY = IMPEMBEDLIBDATA
**
LIBOBJECT = eqdata.c
**
*/

# include       <compat.h>
# include       <cv.h>
# include       <me.h>
# include       <st.h>
# include       <cm.h>
# include       <er.h>
# include       <si.h>
# include       <gl.h>
# include       <sl.h>
# include       <er.h>
# include       <iicommon.h>
# include       <eqrun.h>
# include       <eqscan.h>
# include       <eqscsql.h>
# include       <eqsym.h>
# include       <equel.h>
# include       <eqstmt.h>
# include       <fsicnsts.h>
# include       <eqfrs.h>
# include       <equtils.h>
# include       <eqgen.h>
# include       <eqesql.h>
# include       <eqlang.h>
# include       <eqgr.h>
# include       <ereq.h>

/*
**  Data from eqfrs.c
*/

GLOBALDEF FRS__	frs ZERO_FILL;

/*
**  Data from eqtarget.c
*/

GLOBALDEF i4	max_attribs = 0;	/* maximum attributes for this statement */

/*
**  Data from esqlca.c
*/

GLOBALDEF SQLCA_ELM esqlca_store[] = {
		    { sqcaCONTINUE, (char *)0 },	/* Not found */
		    { sqcaCONTINUE, (char *)0 },	/* Error */
		    { sqcaCONTINUE, (char *)0 },	/* Warning */
		    { sqcaCONTINUE, (char *)0 },	/* Message */
		    { sqcaCONTINUE, (char *)0 }		/* Dbevent */
		};

/*
**  Data from esqsel.c
*/

GLOBALDEF STR_TABLE	*esl_strtable ZERO_FILL;
                                        /* the text of the associated query */

/*
**  Data from esqyylex.c
*/

GLOBALDEF char	*sc_saveptr = (char *)0;  /* save SC_PTR before scanning word */

/*
**  Data from label.c
*/

# define	LBL_MODNUM	4
# define	LBL_STKSIZ	10

typedef struct {
    struct lbl_stack {
	struct stk_sav {
	    i2	sav_cur;
	    i1	sav_flags;
	    i1	sav_use;
	} stk_sav[LBL_MODNUM];	/* one for each of the four kinds */
	i2	stk_mode;	/* which mode is "current" */
	i2	stk_adj;	/* mode "adjective" */
    } lbl_stack[LBL_STKSIZ];	/* the stacked info */
    struct lbl_cur {
	i2	cur_cur;	/* current value */
	i2	cur_high;	/* hi-water mark */
    } lbl_cur[LBL_MODNUM];	/* one for each of the four kinds */
} LBL_STACK;

/* here is the private stuff */
GLOBALDEF LBL_STACK	lbl_labels ZERO_FILL;
GLOBALDEF i2		lbl_index = 0;	/* index of the next free location */

/*
**  Data from out.c
*/

GLOBALDEF	OUT_STATE	out_default = { out_put_def, 0, '\0' };
GLOBALDEF	OUT_STATE	*out_cur = &out_default;

/*
**  Data from sccmnt.c
*/
# if defined(THIS_IS_A_TEST)
/* dbg_vis bits */
# define	DU_Q	DML_EQUEL
# define	DU_S	DML_ESQL
# define	DU_B	(DML_ESQL|DML_EQUEL)

FUNC_EXTERN void act_dump();
FUNC_EXTERN void arg_dump();
FUNC_EXTERN i4	dbg_echo();
FUNC_EXTERN i4	dbg_who();
FUNC_EXTERN i4	dbg_osl();
FUNC_EXTERN i4	ecs_dump();
FUNC_EXTERN i4	eq_dump();
FUNC_EXTERN i4	erec_dump();
FUNC_EXTERN i4	esqlcaDump();
FUNC_EXTERN i4	EQFWdump();
FUNC_EXTERN i4	frs_dump();
FUNC_EXTERN void gr_mechanism();
FUNC_EXTERN void id_dump();
FUNC_EXTERN i4	inc_dump();
FUNC_EXTERN i4	lbl_dump();
FUNC_EXTERN void rep_dump();
FUNC_EXTERN void ret_dump();
FUNC_EXTERN i4	sc_dump();
FUNC_EXTERN i4	sc_help();
FUNC_EXTERN i4	sc_symdbg();
FUNC_EXTERN void str_dump();
FUNC_EXTERN i4	yy_dodebug();

GLOBALDEF SC_DBGCOMMENT dbg_tab[] = {
 DU_B, ERx("act"),	act_dump,    1,	ERx("\t\tDump all activate queues"),
 DU_B, ERx("arg"),	arg_dump,    0,	ERx("\t\tDump all info about current args"),
 DU_B, ERx("csr"),   ecs_dump,	     0,	ERx("\t\tDump the cursor structures"),
 DU_B, ERx("echo"),	dbg_echo,    0,	ERx(" \"string\"\tEcho following string"),
 DU_B, ERx("eq"),	eq_dump,     0,	ERx("\t\tDump the eq structure"),
 DU_B, ERx("frs"),	frs_dump,    0,	ERx("\t\tDump frs statement info"),
 DU_B, ERx("gr"),	gr_mechanism,GR_DUMP, ERx("\t\tDump the gr structure"),
 DU_B, ERx("help"),	sc_help,     0,	ERx("\t\tGive a list of comment commands"),
 DU_B, ERx("id"),	id_dump,     0,	ERx("\t\tDump the id manager"),
 DU_B, ERx("inc"),	inc_dump,    0,	ERx("\t\tDump info on current include file"),
 DU_B, ERx("label"),	lbl_dump,    0,	ERx("\t\tDump the label manager"),
    0, ERx("osl"),	dbg_osl,     0,	ERx("\t\tMap for OSL pass 2"),
 DU_S, ERx("rec"),	erec_dump,   0,	ERx("\t\tDump info on current record"),
 DU_B, ERx("rep"),	rep_dump,    0,	ERx("\t\tDump info on current repeat query"),
 DU_B, ERx("ret"),	ret_dump,    0,	ERx("\t\tDump contents of retrieve list"),
 DU_B, ERx("scan"),	sc_dump,     0,	ERx("\t\tDump contents of scanner buffer"),
 DU_S, ERx("sqlca"),	esqlcaDump,  0, 	ERx("\t\tDump the SQLCA structure"),
 DU_B, ERx("str"),	str_dump,    0,	ERx("\t\tDump the string table"),
 DU_B, ERx("sym"), 	sc_symdbg,   0,
    ERx("[+|=(hx)|=hx]\tTurn on symbol table debug or dump symbol table"),
 DU_B, ERx("with"),	EQFWdump,  0,	ERx("\t\tDump forms with clause info"),
    0, ERx("who"),	dbg_who,     0,	ERx("\t\tPrint compiler copyright notice"),
 DU_B, ERx("yy"),	yy_dodebug,  0, ERx("\t\tToggle yydebug"),
    0, (char *)0,	0,	     0,	ERx("")
};

GLOBALDEF SC_DBGCOMMENT	*sc_dbglang ZERO_FILL;	/* Default is unused */
# endif
/*
**  Data from scinc.c
*/

GLOBALDEF	i4	(*inc_pass2)() ZERO_FILL;

/*
**  Data from scop.c
*/

GLOBALDEF	i4	(*sc_linecont)() ZERO_FILL;

/*
**  Data from scrdline.c
*/

GLOBALDEF char	sc_linebuf[ SC_LINEMAX +3 ] ZERO_FILL;
GLOBALDEF char	*SC_PTR = sc_linebuf;

GLOBALDEF i4	lex_need_input = 0; /* Do we need to read in a new line? */
GLOBALDEF i4	lex_is_newline = 0; /* Do we need to process the line? */

/*
**  Data from sydebug.c
*/

GLOBALDEF i4	_db_debug = 0;

/*
**  Data from sydec.c
*/

GLOBALDEF SYM	*syHashtab[syHASH_SIZ] ZERO_FILL;

GLOBALDEF STR_TABLE
		*syNamtab = NULL;
GLOBALDEF char	*sym_separator = ERx(".");
GLOBALDEF bool	sym_forward = TRUE;
GLOBALDEF struct symHint
		symHint ZERO_FILL;

/*
**  Data from symutils.c
*/

GLOBALDEF i4	(*sym_adjval)() = NULL;		/* for adjusting sy->st_value */

/*
**  Data from syq.c
*/

GLOBALDEF Q_ELM	*sy_qdfreelist = NULL;
GLOBALDEF i4	(*sym_delval)() = NULL;

/*
**  Data from sytree.c
*/

GLOBALDEF i4	(*sym_prtval)() = NULL;
GLOBALDEF i4	(*sym_prbtyp)() = NULL;


/*
**  Data from eqmain.c
*/

GLOBALDEF i4	eq_private = 0;		/* private state flags; see below */
