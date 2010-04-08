/*
** Copyright (c) 1986, 2001 Ingres Corporation
*/
/*
** 20-jul-90 (sandyd) 
**	Workaround VAXC optimizer bug, first encountered in VAXC V2.3-024.
NO_OPTIM = vax_vms
*/

# include 	<compat.h>
# include	<er.h>
# include	<si.h>
# include	<st.h>
# include 	<cm.h>
# include	<lo.h>
# include	<nm.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<eqrun.h>
# include	<equel.h>
# include	<eqsym.h>
# include	<eqgen.h>
# include	<eqscan.h>
# include	<ereq.h>
# include	<ere3.h>
# include	<eqbas.h>


/*
+* Filename:	basgen.c 
** Purpose:	Maintains all the routines to generate II calls within specific
**	  	language dependent control flow statements.
**
** Language:	Written for VMS BASIC.
**
** Defines:	gen_call()			- Standard Call template.
**		gen_if()			- If Then template.
**		gen_else()			- Else/Case stmt template.
**		gen_if_goto()			- If Goto template.
**		gen_loop()			- Loop template.
**		gen_goto()			- Goto template.
**		gen_label()			- Label template.
**		gen_comment()			- Comment template.
**		gen_include()			- Include template.
**		gen_eqstmt()			- Open/close Equel statement.
**		gen_line()			- Line comment or directive.
**		gen_term()			- Terminate statement.
**		gen_declare()			- Process a declare statement.
**		gen_makesconst()		- Process a string constant.
**		gen_sconst()			- String constant.
**		gen_do_indent()			- Update indentation.
**		gen_host()			- Host code (declaration) calls.
**		gen_Blinenum()			- BASIC line number emitter
** Exports:	
**		gen_code()			- Macro defined as out_emit().
** Locals:
**		gen__goto()			- Core of goto statement.
**		gen__label()			- Statement independent Label.
**		gen__II()			- Core of an II call.
**		gen_all_args()			- Dump all args in call.
**		gen_var_args()			- Varying number of arguments.
**		gen_data()			- Dump different arg data.
**		gen_io()			- Describe I/O arguments.
**		gen_iodat()
**		gen_asn				- Generate assignment
**		gen_IIdebug()			- Runtime debug information.
**		gen_null()			- Dump indicator argument.
**		gen_sdesc()			- Generate string description.
**		gen__sconst()			- Controls splitting strings.
**		gen__name()			- Dumps undescribed arguments
**		gen__int()			- Integer.
**		gen__float()			- Float.
**		gen__obj()			- Write anything but strings.
**		gen__functab()			- Get function table pointer.
**		gen__continue			- Continue a BASIC line
**              gen_hdl_args(flags,numargs)     - SET_SQL handler args.
**		gen_datio()			- GET/PUT DATA statements.
**
** Externals Used:
**		out_cur				- Current output state.
**		eq				- Global Equel state.
**		
** The general rule to generating a template is that we need to pass all the
** information possibly needed in the worst case.  For example, in a language
** that does not support 'while' loops we need to generate two labels, and
** control the branches taken at the beginning and end of the loop.  Thus, to
** start a loop we pass as much information as will be needed even for 'good'
** languages such as C.  The same applies to If blocks - who knows what 
** language will not allow If statements and will require a labelled
-* implementation.
**
** Note:
** 1. This module is duplicated across the different Equel languages that
**    are supported.  Some routines will be common across different languages
**    but most will have to be rewritten for each language.  Hopefully, the C
**    template for this file will guide anyone creating such a file for 
**    different languages.  
** 2. Routines that write data:
** 2.1. Some routines that actually write stuff make use of the external counter
**      out_cur->o_charcnt.  This is the character counter of the current line
**      being written to.  Whenever there is a call to emit code, this counter
**      gets updated even though the local code does not explicitly update it.
** 2.2. A call to gen__obj() which makes sure the object can fit on the line, 
**      needs to be made only when the data added may have gone over the end of 
**      the line.  In some cases, either when nothing is really being written,
**      or the data comes directly from the host language host code, the main
**      emit routine can be used.
**
** History:
**		30-oct-1984 	- Written for C (ncg)
**		01-dec-1985 	- Converted for PASCAL (mrw)
**		26-feb-1986	- Converted to ADA (ncg)
**		05-mar-1986	- Converted to BASIC (bjb)
**		07-jul-1987	- Updated for 6.0 (bjb)
**		23-mar-1989	- Added cast for STlength (teresa)
**              11-may-1989     - Added function calls IITBcaClmAct,
**                                IIFRafActFld, IIFRreResEntry to gen__ftab
**                                for entry activation. (teresa)
**		10-aug-1989	- Add function call IITBceColEnd for FRS 8.0.
**				  (teresa)
**		19-dec-1989	- Updated for Phoenix/Alerters. (bjb)
**		05-mar-1990	- Fixed bug #9191 where functab table had
**				  incorrect entry for IIvarstrio call.
**			 	  Bad code generation caused some variable
**				  substitutions to fail at runtime.(barbara)
**		25-jul-1990	- Added decimal support. (teresal)
**              08-aug-1990 (barbara)
**                      Added IIFRsaSetAttrio to function table for per value
**                      display attributes.
**              22-feb-1991 (kathryn)
**                      Added IILQssSetSqlio and IILQisInqSqlio.
**			      IILQshSetHandler.
**		21-apr-1991 (teresal)
**			Add activate event call - remove IILQegEvGetio.
**		14-oct-1992 (lan)
**			Added IILQled_LoEndData.
**		14-jan-1993 (lan)
**			Added IILQldh_LoDataHandler.
**		15-jan-1993 (kathryn)
**		    Added gen_datio() function, and added entries for
**                  IILQlgd_LoGetData, and IILQlpd_LoPutData.
**	26-jul-1993 (lan)
**		Added EXEC 4GL support routines.
**	07/28/93 (dkh) - Added support for the PURGETABLE and RESUME
**			 NEXTFIELD/PREVIOUSFIELD statements.
**      08-30-93 (huffman)
**              add <st.h>
**      01-oct-1993 (kathryn)
**          First argument generated for GET/PUT DATA stmts ("attrs" bit mask),
**          will now be set/stored by eqgen_tl() rather than determined by
**          gen_datio, so add one to the number of args in the gen__ftab for
**          IILQlpd_LoPutData and IILQlgd_LoGetData.
**      03-oct-2000 (bolke01)
**          Added a parameter to the IIbreak gen_ftab and included the debug 
**          element gen_IIdebug.  This will hopefully cure the MST problem
**          seen on Very fast m/c after a QUEL RETRIEVE is ENDLOOP'ed
**	15-feb-2001	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	20-jul-2007 (toumi01)
**	    add IIcsRetScroll and IIcsOpenScroll for scrollable cursors
*/


/* Max line length for BASIC code - with a bit more space for closing stuff */

# define	G_LINELEN	72

/* 
** Max size of converted string - allow some extra space over scanner for
** extra escaped characters required.
*/

# define 	G_MAXSCONST	300	



/* Flags used when generating integer constants or just names  */

# define	BobjPLAIN	0		/* Just generate plain */
# define	BobjVAL		1		/* Pass obj by value */

/*
** gb_retnull - Pointer to null name or indicator variable name.
**
** gb_retnull is set to an indicator variable name in gen_null if used for
** output.  Then in gen_retasn it is checked for and generated around the
** resulting assignment statement. It is cleared at the end of gen_io.
** For example, after using a temp f8 for packed-decimal data we may generate:
**
**		call IIgetdomio(null_ind, desc, IIf8)
**		if (null_ind <> -1) then
**			dec_var = IIf8
**		end if
*/
static	char	*gb_retnull	ZERO_FILL;
/* 
** Indentation information - 
**     Each indentation is 2 spaces.  For BASIC the indentation is made up of 
** spaces. For languages where margins are significant, and continuation of 
** lines must be explicitly flagged (Fortran) the indentation mechanism will 
** be more complex - Always start out at the correct margin, and indent from
** there. If it is a continuation, then flag it in the correct margin, and 
** then indent. In PASCAL g_indent[0-G_MININDENT-1] are always blanks.
** In BASIC we indent from a margin, except for the printing of line 
** numbers which is done at the extreme left margin.
*/

# define 	G_MAXINDENT	20
# define 	G_MINMARG	6
# define	G_MININDENT	1
# define	G_MARGIN	ERx("      ")	/* 6-space margin */

static		char	g_indent[ G_MAXINDENT + G_MAXINDENT ] =
		{
		  ' ', ' ','\0', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '
		};

static	i4	g_indlevel = G_MININDENT;		/* Start out indented */



/* 
** Table of conditions - 
** The constants for logical opposites are negatives of each other. For example
** to get the negative of a condition use:
**
**	gen_condtab[ C_RANGE - cond ]
**
** For example the opposite of C_LESS ("<") is -C_LESS which gives C_GTREQ
** which when used with the extra C_RANGE gives ">=".
** See where these constants are defined.
** Table usage:
**	condition	gen_condtab[ C_RANGE + cond ]
**	complement	gen_condtab[ C_RANGE - cond ]
*/

static	char	*gen_condtab[] = {
    ERx(">="), 			/* C_GTREQ */
    ERx("<="),			/* C_LESSEQ */
    ERx("<>"),   		/* C_NOTEQ */
    (char *)0,			/* C_0 */
    ERx("="),			/* C_EQ */
    ERx(">"),			/* C_GTR */
    ERx("<")			/* C_LESS */
};

/* Table of label names - correspond to L_xxx constants */

static char	*gen_ltable[] = {
    ERx("IInolab"),			/* Should never be referenced */
    ERx("IIfd_Init"),
    ERx("IIfd_Begin"),
    ERx("IIfd_Final"),
    ERx("IIfd_End"),
    ERx("IImu_Init"),
    (char *)0,				/* We do have Activate Else clauses */
    (char *)0,
    ERx("IIrep_Begin"),
    ERx("IIrep_End"),
    ERx("IIret_Begin"),
    ERx("IIret_Flush"),
    ERx("IIret_End"),
    ERx("IItab_Begin"),
    ERx("IItab_End"),
    (char *)0
};

/* 
** Local goto, label, call and argument dumping calls -
** Should not be called from the outside, even though they are not static.
*/

void	gen__goto();
void	gen__label();
void	gen__II();
i4	gen_all_args();
i4	gen_var_args();
void	gen_data();
i4	gen_io();
void	gen_iodat();
void	gen_asn();
i4	gen_IIdebug();
void	gen_null();
i4	gen__sconst();
void	gen_sdesc();
void	gen__name();
void	gen__int();
void	gen__float();
void	gen__obj();
i4	gen_unsupp();
void	gen__continue();
i4	gen_hdl_args();
i4	gen_datio();

/*
** Table of II calls - 
**     Correspond to II constants, the function to call to generate arguments, 
** the flags needed and the number of arguments.  For BASIC there is very little
** argument information needed.  I/O calls are handled as a special case.
*/

GEN_IIFUNC	*gen__functab();

GLOBALDEF GEN_IIFUNC	gen__ftab[] = {
  /* Name */		/* Function */	/* Flags */		/* Args */
    {ERx("IIf_"),	0,		0,				0},

  /* Quel # 1 */
    {ERx("IIflush"),	gen_IIdebug,	0,				0},
    {ERx("IInexec"),	0,		0,				0},
    {ERx("IInextget"),	0,		0,				0},
    {ERx("IInotrimio"),	gen_io,		G_SET,	 			2},
    {ERx("IIparret"),	0,		0,		/* param */	0},
    {ERx("IIparset"),	0,		0,		/* param */	0},
    {ERx("IIgetdomio"),	gen_io,		G_RET,				2},
    {ERx("IIretinit"),	gen_IIdebug,	0,				0},
    {ERx("IIputdomio"),	gen_io,		G_SET,   /* Real IIsetdom */	2},
    {ERx("IIsexec"),	0,		0,				0},
    {ERx("IIsyncup"),	gen_IIdebug,	0,				0},
    {ERx("IItupget"),	0,		0,				0},
    {ERx("IIwritio"),	gen_io,		G_SET,	 			3},
    {ERx("IIxact"),	gen_all_args,	0,				1},
    {ERx("IIvarstrio"),	gen_io,		G_SET,				2},
    {ERx("IILQssSetSqlio"),gen_io,      G_SET,		                3},
    {ERx("IILQisInqSqlio"),gen_io,      G_RET,                          3},
    {ERx("IILQshSetHandler"),gen_hdl_args,0,                            2},
    {(char *)0,         0,              0,                              0},
    {(char *)0,		0,		0,				0},

  /* Libq # 21 */
    {ERx("IIbreak"),	gen_IIdebug,	0,				1},
    {ERx("IIeqiqio"),	gen_io,		G_RET,				3},
    {ERx("IIeqstio"),	gen_io,		G_SET,				3},
    {ERx("IIerrtest"),	0,		0,				0},
    {ERx("IIexit"),	0,		0,				0},
    {ERx("IIingopen"),	gen_var_args,	0,				15},
    {ERx("IIutsys"),	gen_all_args,	0,				3},
    {ERx("IIexExec"),	gen_all_args,	0,				4},
    {ERx("IIexDefine"),	gen_all_args,	0,				4},
    {(char *)0,		0,		0,				0},

  /* Forms # 31 */
    {ERx("IITBcaClmAct"),gen_all_args,	0,				4},
    {ERx("IIactcomm"),	gen_all_args,	0,				2},
    {ERx("IIFRafActFld"),gen_all_args,	0,				3},
    {ERx("IInmuact"),	gen_all_args,	0,				5},
    {ERx("IIactscrl"),	gen_all_args,	0,				3},
    {ERx("IIaddform"),	gen_all_args,	0,				1},
    {ERx("IIchkfrm"),	0,		0,				0},
    {ERx("IIclrflds"),	0,		0,				0},
    {ERx("IIclrscr"),	0,		0,				0},
    {ERx("IIdispfrm"),	gen_all_args,	0,				2},
    {ERx("IIprmptio"),	gen_io,		G_RETSET, /* RET, w/SET fmt */	4},
    {ERx("IIendforms"),	0,		0,				0},
    {ERx("IIendfrm"),	0,		0,				0},
    {ERx("IIendmu"),	0,		0,				0},
    {ERx("IIfmdatio"),	gen_unsupp,	IIFDATA, /* Unsupported */	2},
    {ERx("IIfldclear"),	gen_all_args,	0,				1},
    {ERx("IIfnames"),	0,		0,				0},
    {ERx("IIforminit"),	gen_all_args,	0,				1},
    {ERx("IIforms"),	gen_all_args,	0,				1},
    {ERx("IIfsinqio"),	gen_unsupp,	IIFRSINQ, /* Unsupported */	2},
    {ERx("IIfssetio"),	gen_unsupp,	IIFRSSET, /* Unsupported */	2},
    {ERx("IIfsetio"),	gen_all_args,	0,				1},
    {ERx("IIgetoper"),	gen_all_args,	0,				1},
    {ERx("IIgtqryio"),	gen_io,		G_RET, /* Unsupported */	3},
    {ERx("IIhelpfile"),	gen_all_args,	0,				2},
    {ERx("IIinitmu"),	0,		0,				0},
    {ERx("IIinqset"),	gen_unsupp,	IIINQSET, /* Unsupported */	6},
    {ERx("IImessage"),	gen_all_args,	0,				1},
    {ERx("IImuonly"),	0,		0,				0},
    {ERx("IIputoper"),	gen_all_args,	0,				1},
    {ERx("IIredisp"),	0,		0,				0},
    {ERx("IIrescol"),	gen_all_args,	0,				2},
    {ERx("IIresfld"),	gen_all_args,	0,				1},
    {ERx("IIresnext"),	0,		0,				0},
    {ERx("IIgetfldio"),	gen_io,		G_RET,				3},
    {ERx("IIretval"),	0,		0,				0},
    {ERx("IIrf_param"),	0,		0,		/* param */	0},
    {ERx("IIrunform"),	0,		0,				0},
    {ERx("IIrunmu"),	gen_all_args,	0,				1},
    {ERx("IIputfldio"),	gen_io,		G_SET,				3},
    {ERx("IIsf_param"),	0,		0,		/* param */	0},
    {ERx("IIsleep"),	gen_all_args,	0,				1},
    {ERx("IIvalfld"),	gen_all_args,	0,				1},
    {ERx("IInfrskact"),	gen_all_args,	0,				5},
    {ERx("IIprnscr"),	gen_all_args,	0,				1},
    {ERx("IIiqset"),	gen_var_args,	0,				5},
    {ERx("IIiqfsio"),	gen_io,		G_RET,				5},
    {ERx("IIstfsio"),	gen_io,		G_SET,				5},
    {ERx("IIresmu"),	0,		0,				0},
    {ERx("IIfrshelp"),	gen_all_args,	0,				3},
    {ERx("IInestmu"),	0,		0,				0},
    {ERx("IIrunnest"),	0,		0,				0},
    {ERx("IIendnest"),	0,		0,				0},
    {ERx("IIFRtoact"),	gen_all_args,	0,				2},
    {ERx("IIFRitIsTimeout"),0,		0,				0},

  /* Table fields # 86 */
    {ERx("IItbact"),	gen_all_args,	0,				3},
    {ERx("IItbinit"),	gen_all_args,	0,				3},
    {ERx("IItbsetio"),	gen_all_args,	0,				4},
    {ERx("IItbsmode"),	gen_all_args,	0,				1},
    {ERx("IItclrcol"),	gen_all_args,	0,				1},
    {ERx("IItclrrow"),	0,		0,				0},
    {ERx("IItcogetio"),	gen_io,		G_RET,				3},
    {ERx("IItcoputio"),	gen_io,		G_SET,				3},
    {ERx("IItcolval"),	gen_all_args,	0,				1},
    {ERx("IItdata"),	0,		0,				0},
    {ERx("IItdatend"),	0,		0,				0},
    {ERx("IItdelrow"),	gen_all_args,	0,				1},
    {ERx("IItfill"),	0,		0,				0},
    {ERx("IIthidecol"),	gen_all_args,	0,				2},
    {ERx("IItinsert"),	0,		0,				0},
    {ERx("IItrc_param"),0,		0,		/* param */	0},
    {ERx("IItsc_param"),0,		0,		/* param */	0},
    {ERx("IItscroll"),	gen_all_args,	0,				2},
    {ERx("IItunend"),	0,		0,				0},
    {ERx("IItunload"),	0,		0,				0},
    {ERx("IItvalrow"),	0,		0,				0},
    {ERx("IItvalval"),	gen_all_args,	0,				1},
    {ERx("IITBceColEnd"), 0,            0,              		0},
    {(char *)0,		0,		0,				0},
    {(char *)0,		0,		0,				0},

  /* QA test calls # 111 */
    {ERx("QAmessage"),	gen_var_args,	0,	/* String var trim */	13},
    {ERx("QAprintf"),	gen_var_args,	0,				13},
    {ERx("QAprompt"),	gen_var_args,	0,				13},
    {(char *)0,		0,		0,				0},
    {(char *)0,		0,		0,				0},
    {(char *)0,		0,		0,				0},
    {(char *)0,		0,		0,				0},
    {(char *)0,		0,		0,				0},
    {(char *)0,		0,		0,				0},

    /* EQUEL cursor routines # 120 */
/* 120 */    {ERx("IIcsRetrieve"),gen_all_args,	0,			3},
/* 121 */    {ERx("IIcsOpen"),	gen_all_args,	0,			3},
/* 122 */    {ERx("IIcsQuery"),	gen_all_args,	0,			3},
/* 123 */    {ERx("IIcsGetio"),	gen_io,		G_RET,			2},
/* 124 */    {ERx("IIcsClose"),	gen_all_args,	0,			3},
/* 125 */    {ERx("IIcsDelete"),	gen_all_args,	0,		4},
/* 126 */    {ERx("IIcsReplace"),gen_all_args,	0,			3},
/* 127 */    {ERx("IIcsERetrieve"),0,		0,			0},
/* 128 */    {ERx("IIcsParGet"),	0,		0,		2},
/* 129 */    {ERx("IIcsERplace"),gen_all_args,	0,			3},
/* 130 */    {ERx("IIcsRdO"),	gen_all_args,	0,			2},
/* 131 */    {ERx("IIcsRetScroll"),	gen_all_args,	0,		5},
/* 132 */    {ERx("IIcsOpenScroll"),	gen_all_args,	0,		4},
/* 133 */    {(char *)0,		0,		0,		0},
/* 134 */    {(char *)0,		0,		0,		0},

		/* Forms With Clause Routines # 135 */
/* 135 */    {ERx("IIFRgpcontrol"),	gen_all_args,	0,		2},
/* 136 */    {ERx("IIFRgpsetio"),	gen_io,		G_SET,		3},
/* 137 */    {ERx("IIFRreResEntry"),	0,		0,		0},
/* 138 */    {(char *)0,		0,		0,		0},
/* 139 */    {ERx("IIFRsaSetAttrio"),	gen_io,		G_SET,		4},
/* 140 */    {ERx("IIFRaeAlerterEvent"),gen_all_args, 	0,             	1},
/* 141 */    {ERx("IIFRgotofld"),	gen_all_args,	0,		1},
/* 142 */    {(char *)0,		0,		0,		0},
/* 143 */    {(char *)0,		0,		0,		0},
/* 144 */    {(char *)0,		0,		0,		0},

		/* PROCEDURE support #145 */
/* 145 */    {ERx("IIputctrl"),		gen_all_args,	0,		1},
/* 146 */    {ERx("IILQpriProcInit"),	gen_all_args,	0,		2},
/* 147 */    {ERx("IILQprvProcValio"),	gen_io,		G_SET,		4},
/* 148 */    {ERx("IILQprsProcStatus"),	0,		0,		0},
/* 149 */    {(char *)0,		0,		0,		0},
/* 150 */    {(char *)0,		0,		0,		0},

		/* EVENT support #151 */
/* 151 */    {(char *)0,		0,		0,		0},
/* 152 */    {ERx("IILQesEvStat"),	gen_all_args,	0,		2},
/* 153 */    {(char *)0,		0,		0,		0},
/* 154 */    {(char *)0,		0,		0,		0},

		/* LARGE OBJECTS support #155 */
/* 155 */    {ERx("IILQldh_LoDataHandler"),gen_hdl_args,G_DAT,          4},
/* 156 */    {ERx("IILQlgd_LoGetData"), gen_datio,      G_RET,          5},
/* 157 */    {ERx("IILQlpd_LoPutData"), gen_datio,      G_SET,          4},
/* 158 */    {ERx("IILQled_LoEndData"), 0,              0,              0},
/* 159 */    {(char *)0,                0,              0,              0},
/* 160 */    {(char *)0,                0,              0,              0},
/* 161 */    {(char *)0,                0,              0,              0},
/* 162 */    {(char *)0,                0,              0,              0},
/* 163 */    {(char *)0,                0,              0,              0},
/* 164 */    {(char *)0,                0,              0,              0},

		/* EXEC 4GL support  # 165 */
/* 165 */    {ERx("IIG4acArrayClear"),  gen_all_args,   0,		1},
/* 166 */    {ERx("IIG4rrRemRow"),  	gen_all_args,   0,		2},
/* 167 */    {ERx("IIG4irInsRow"),  	gen_all_args,   0,		5},
/* 168 */    {ERx("IIG4drDelRow"),  	gen_all_args,   0,		2},
/* 169 */    {ERx("IIG4srSetRow"),  	gen_all_args,   0,		3},
/* 170 */    {ERx("IIG4grGetRow"),  	gen_io,   	G_RET,		4},
/* 171 */    {ERx("IIG4ggGetGlobal"),  	gen_io,   	G_RET,		4},
/* 172 */    {ERx("IIG4sgSetGlobal"),  	gen_io,   	G_SET,		3},
/* 173 */    {ERx("IIG4gaGetAttr"),  	gen_io,   	G_RET,		3},
/* 174 */    {ERx("IIG4saSetAttr"),  	gen_io,   	G_SET,		3},
/* 175 */    {ERx("IIG4chkobj"),  	gen_all_args,  	0,		4},
/* 176 */    {ERx("IIG4udUseDscr"),	gen_all_args,   0,		3},
/* 177 */    {ERx("IIG4fdFillDscr"),	gen_all_args,   0,		3},
/* 178 */    {ERx("IIG4icInitCall"),	gen_all_args,   0,		2},
/* 179 */    {ERx("IIG4vpValParam"),	gen_io,   	G_SET,		3},
/* 180 */    {ERx("IIG4bpByrefParam"),	gen_io,   	G_RET,		3},
/* 181 */    {ERx("IIG4rvRetVal"),	gen_io,   	G_RET,		2},
/* 182 */    {ERx("IIG4ccCallComp"),	gen_all_args,   0,		0},
/* 183 */    {ERx("IIG4i4Inq4GL"),	gen_io,   	G_RET,		4},
/* 184 */    {ERx("IIG4s4Set4GL"),	gen_io,		G_SET,		4},
/* 185 */    {ERx("IIG4seSendEvent"),	gen_all_args,   0, 		1},
/* 186 */    {(char *)0,                0,              0,              0},
/* 187 */    {(char *)0,                0,              0,              0},
/* 188 */    {(char *)0,                0,              0,              0},
/* 189 */    {(char *)0,                0,              0,              0}
};

/*
+* Procedure:	gen_call 
** Purpose:	Generates a standard call to an II function.
**
** Parameters: 	func - i4 - Function index.
** Returns: 	None
**
-* Format: 	func( args );
**
** History:
**      15-jan-1993 (kathryn)
**          New gen_datio also requires func as parameter.
*/

void
gen_call( func )
i4	func;
{
    if (gen__functab(func)->g_func == gen_io)
	gen_io( func );
    else if (gen__functab(func)->g_func == gen_datio)
	gen_datio(func);
    else
    {
	gen__obj( TRUE, ERx("call ") );
	gen__II( func );
    }
    out_emit( ERx("\n") );
}


/*
+* Procedure:	gen_if 
** Purpose:	Generates a simple If statement (Open or Closed).
**
** Parameters: 	o_c  - i4 - Open / Close the If block,
**		func - i4 - Function index,
**		cond - i4 - Condition index,
**		val  - i4 - Right side value of condition.
** Returns: 	None
**
** Format: 	(open) 	if (func(args) condition val) then
** 	   	(close) end if;
**
** The function constant is passed when opening and closing the If statement, 
** just in case some language will not allow If blocks.  This problem already 
-* exists on Unix Cobol, where some If blocks are not allowed.
*/

void
gen_if( o_c, func, cond, val )
i4	o_c;						/* Open / Close */
i4	func;
i4	cond;
i4	val;
{
    if (o_c == G_OPEN)
    {
	gen__obj( TRUE, ERx("if (") );				/* If */
	gen__II( func );					/* Function */
	gen__obj( TRUE, ERx(" ") );
	gen__obj( TRUE, gen_condtab[C_RANGE + cond] );		/* Condition */
	gen__obj( TRUE, ERx(" ") );
	gen__int( BobjPLAIN, val );				/* Value */
	gen__obj( TRUE, ERx(") then") );			/* Then Begin */
	out_emit( ERx("\n") );
	gen_do_indent( 1 );

	/* Next call should Close or Else */
    }
    else if (o_c == G_CLOSE)
    {
	gen_do_indent( -1 );
	gen__obj( TRUE, ERx("end if ") );			/* End */
	gen_comment( TRUE, gen__functab(func)->g_IIname );
    }
}

/* 
+* Procedure:	gen_else 
** Purpose:	Generate an If/Else block or a case statement
**
** Parameters: 	flag	- i4 - Open / Else / Close the If block, 
**		func	- i4 - Function index,
**		cond	- i4 - Condition index,
**		val	- i4 - Right side value of condition,
**		l_else	- i4 - Else label prefix,
**		l_elsen	- i4 - Else label number,
**		l_end	- i4 - Endif label prefix,
**		l_endn	- i4 - Endif label number.
** Returns: 	None
**
** Format: 	(open) 		select func
**				    case cond val	
**		(else/open)	    case cond val
**		(else/open)         case else
-* 	   	(close) 	end select
**
** 1. The function constant and labels are passed for labelled languages that
** cannot use If-Then-Else blocks or case statements in some cases.  
** 2. Assumptions:  This routine is only called to generate
** code for activate lists.  
** 3. Implementation for BASIC:
**   For BASIC we generate case statements instead of it-then-else blocks.
**   A "case" clause is generated for each interrupt value.  The condition
**   passed in will always  be "=", but note that in BASIC case clauses
**   can contain other relational expressions.
**
**   Interpretation of incoming flag:
**
** 	G_OPEN:  This means that we should generate code to either 
**	    1) test the value of "func"; OR 
**	    2) to open up that last block of the case statement 
**	       (in BASIC the "else" word of the "case else" clause).  
**	If we are generating (1) we must either open the select statement, 
** 	or just generate the test part of the current "case" clause.  We 
** 	decide by keeping around a static flag.
**
** 	G_ELSE:  Just generate the word "case" and expect that we'll be 
**	called again with G_OPEN.
**
** 	G_CLOSE: Close the whole select statement.
*/

void
gen_else( flag, func, cond, val, l_else, l_elsenum, l_end, l_endnum )
i4	flag;					/* Open, Else or Close */
i4	func;
i4	cond;
i4	val;
i4	l_else;
i4	l_elsenum;
i4	l_end;
i4	l_endnum;
{
    static i4	casedumped = 0;		/* Last thing emitted was "case " */

    switch (flag)
    {
      case G_OPEN:
	if (func == II0)		/* Open last block of case statement */
	{
	    out_emit( ERx(" else\n") );
	    gen_do_indent( 1 );
	}
	else if (casedumped)		/* Cond/val following "case" word  */
	{
	    gen__obj( TRUE, gen_condtab[C_RANGE + cond] );
	    out_emit( ERx(" ") );
	    gen__int( BobjPLAIN, val );
	    out_emit( ERx("\n") );
	    gen_do_indent( 1 );
	}
	else				/* First time into select statement */
	{
	    gen__obj( TRUE, ERx("select ") );
	    gen__II( func );
	    out_emit( ERx("\n") );
	    gen_do_indent( 1 );
	    gen__obj( TRUE, ERx("case ") );
	    gen__obj( TRUE, gen_condtab[C_RANGE + cond] );
	    out_emit( ERx(" ") );
	    gen__int( BobjPLAIN, val );
	    out_emit( ERx("\n") );
	    gen_do_indent( 1 );
	}
	casedumped = 0;
	break;
      
      case G_ELSE:			/* Generate "case"; G_OPEN to follow */
	gen_do_indent( -1 );
	gen__obj( TRUE, ERx("case ") );
	casedumped = 1;
	break;

      case G_CLOSE:			/* Close select statement */
	gen_do_indent( -2 );
	gen__obj( TRUE, ERx("end select\n") );
	casedumped = 0;
    }
}

/*
+* Procedure:	gen_if_goto 
** Purpose:	Generate an If Goto template.
**
** Parameters: 	func	- i4 - Function index,
**		cond	- i4 - Condition index,
**		val	- i4 - Right side value of condition,
**		labpref	- i4 - Label prefix,
**		labnum	- i4 - Label number.
** Returns: 	None
**
-* Format:	if (func(args) condition val) then 
**		    goto label 
**		end if
*/

void
gen_if_goto( func, cond, val, labprefix, labnum )
i4	func;
i4	cond;
i4	val;
i4	labprefix;
i4	labnum;
{
    gen_if( G_OPEN, func, cond, val );			/* If (...) then */
    gen__goto( labprefix, labnum );			/* Goto Label */
    out_emit( ERx("\n") );
    gen_do_indent( -1 );
    gen__obj( TRUE, ERx("end if") );			/* End-If */
    out_emit( ERx("\n") );
}

/*
+* Procedure:	gen_loop 
** Purpose:	Dump a while loop (open the loop or close it).
**
** Parameters: 	o_c	- i4 - Open / Close the loop,
**		labbeg	- i4 - Start label prefix,
**		labend	- i4 - End label prefix,
**		labnum	- i4 - Loop label number.
**		func	- i4 - Function index,
**		cond	- i4 - Condition index,
**		val	- i4 - Right side value of condition.
** Returns: 	None
**
** Format: 	(open)	while (func(args) condition val) 
-*	   	(close)	next
*/

void
gen_loop( o_c, labbeg, labend, labnum, func, cond, val )
i4	o_c;						/* Open or Close */
i4	labbeg;
i4	labend;
i4	labnum;
i4	func;
i4	cond;
i4	val;
{
    if (o_c == G_OPEN)
    {
	gen__obj( TRUE, ERx("while (") );			/* While */
	gen__II( func );					/* Function */
	gen__obj( TRUE, ERx(" ") );
	gen__obj( TRUE, gen_condtab[C_RANGE + cond] );		/* Condition */
	gen__obj( TRUE, ERx(" ") );
	gen__int( BobjPLAIN, val );				/* Value */
	gen__obj( TRUE, ERx(")") );
	out_emit( ERx("\n") );
	gen_do_indent( 1 );
    }
    else
    {
	gen_do_indent( -1 );
	gen__obj( TRUE, ERx("next ") );
	gen_comment( TRUE, gen__functab(func)->g_IIname );
    }
}


/*
+* Procedure:	gen_goto 
** Purpose:	Put out a Goto statement.
**
** Parameters: 	flag	  - i4 - Needs If, or needs termii4or (unused).
**		labprefix - i4 - Label prefix,
**		labnum	  - i4 - Label number.
** Returns: 	None
**
** Format:	[ if (1 = 1) then ] goto label [end if]
**
** Note that this routine is called from the grammar, and may need the If
** clause to prevent compiler warnings.  The 'flag' tells the routine
** that this is a 'loop break out' statement (ie: ## endloop) and requires
-* the If clause.  
*/

void
gen_goto( flag, labprefix, labnum )
i4	flag;
i4	labprefix;
i4	labnum;
{
    if (flag == G_IF)
	gen__obj( TRUE, ERx("if (1% = 1%) then ") );
    gen__goto( labprefix, labnum );
    if (flag == G_IF)
	out_emit( ERx(" end if\n") );
    else
	out_emit( ERx("\n") );
}

/*
+* Procedure:	gen__goto 
** Purpose:	Local statement independent Goto generator.  
** Parameters:	labprefix - i4 - Label prefix index.
**		labnum    - i4 - Label number to concatei4e.
-* Returns:	None
*/

void
gen__goto( labprefix, labnum )
i4	labprefix;
i4	labnum;
{
    gen__obj( TRUE, ERx("goto ") );				/* Goto */
    gen__label( TRUE, labprefix, labnum );			/* Label */
}

/*
+* Procedure:	gen_label 
** Purpose:	Put out a constant label.
**
** Parameters: 	loop_extra - i4 - Is it extra label for a loop? 
**				  (see usage in grammar)
**	 	labprefix  - i4 - Label prefix,
**		labnum 	   - i4 - Label number.
** Returns: 	None
**
-* Format:	label:
**
** Note: Some labels are never followed by a statement or are only sometimes
**       followed by a statement.  The BASIC compiler doesn't complain about 
** 	 an "end" right after a label.
*/

void
gen_label( loop_extra, labprefix, labnum )
i4	loop_extra;
i4	labprefix;
i4	labnum;
{
    /* Labels will always begin after the margin */
    gen__label( FALSE, labprefix, labnum );
    out_emit( ERx(":\n") );		
}

/*
+* Procedure:	gen__label 
** Purpose:	Local statement independent label generator.  
**
** Parameters:	ind	  - i4 - Indentation flag,
**		labprefix - i4 - Label prefix,
**		labnum    - i4 - Label number.
-* Returns:	None
*/

void
gen__label( ind, labprefix, labnum )
i4	ind;
i4	labprefix;
i4	labnum;
{
    char	buf[30];

    /* Label prefixes and their numbers must be atomic */
    gen__obj( ind, gen_ltable[labprefix] );			/* Label */
    CVna( labnum, buf );
    out_emit( buf );
}

/*
+* Procedure:	gen_comment 
** Purpose:	Make a comment with the passed string. Should be used
**		by higher level calls, but may be used locally.
**
** Parameters: 	term - i4	- Comment terminated with newline,
**		str  - char * 	- Comment string.
** Returns: 	None
**
-* Format:	! comment string
*/

void
gen_comment( term, str )
i4	term;
char	*str;
{
    gen__obj( TRUE, ERx("! ") );
    gen__obj( TRUE, str );
    if (term)
	out_emit( ERx("\n") );
}

/*
+* Procedure:	gen_include 
** Purpose:	Generate an include file statement.
**
** Parameters: 	fullnm	- char	* - Full name without suffix,
**		suff	- char  * - and suffix separated by a '.',
**		extra	- char  * - Any extra qualifiers etc.
** Returns: 	None
**
-* Format:	%include filename
**
** Notes:	Prefix and suffix separated by a '.'.  Any extra
**		qualifiers, etc.
** Returns	None.
*/

void
gen_include( fullnm, suff, extra )
char	*fullnm;
char	*suff;
char	*extra;
{

    Bput_lnum();
    gen__obj( TRUE, ERx("%include \"") );
    out_emit( fullnm );
    if (suff != (char *)0)
    {
	out_emit( ERx(".") );
	out_emit( suff );
    }
    if (extra != (char *)0)
	out_emit( extra );
    out_emit( ERx("\"\n") );
}

/*
+* Procedure:	gen_eqstmt 
** Purpose:	Open / Close an Equel statement.
**
** Parameters:	o_c  - i4     - Open / Close,
**		cmnt - char * - Comment string.
-* Returns:	None
**
** Based on the language we may want to force statements into pseudo blocks
** (C or Pascal), and add a comment about the line number and file.
*/

void
gen_eqstmt( o_c, cmnt )
i4	o_c;
char	*cmnt;
{
    if (o_c == G_OPEN)
	gen_line( cmnt );
    else if (cmnt)
	gen_comment( TRUE, cmnt );
}

/*
+* Procedure:	gen_line 
** Purpose:	Generate a line comment or directive.
**
** Parameters:	s - char * - Optional comment string.
** Returns:	None
**
-* Format:	! File "filename", Line linenum   ( comment )
*/

void
gen_line( s )
char	*s;
{
    char	fbuf[G_MAXSCONST +20]; 

    out_emit( ERx("\n") );
    STprintf( fbuf, ERx("! File \"%s%s%s\", Line %d"), eq->eq_filename,
	eq->eq_infile == stdin ? ERx("") : ERx("."), eq->eq_ext, eq->eq_line );
    if (s)
    {
	STcat( fbuf, ERx("\t( ") );
	STcat( fbuf, s );
	STcat( fbuf, ERx(" ) ") );
    }
    gen__obj( FALSE, fbuf );
    out_emit( ERx("\n") );
}

/*
+* Procedure:	gen_term 
** Purpose:	Terminate a statement.
**
** Parameters:	None
-* Returns:	None
**
** Note:
**  1. Currently only used for Cobol special periods.
*/

void
gen_term()
{
}

/*
+* Procedure:	gen_declare 
** Purpose:	Process a ## declare ingres statement.
**
** Parameters:	Optional
** Returns:	None
**
** Note:
**  1. Used for non-C languages that need to generate either include
-*     statements or other declarations for runtime.
*/

void
gen_declare()
{
    LOCATION	loc;
    char	buf[MAX_LOC +1];
    char	*lname;
    i4	len;
    static bool	first = TRUE;

    /* Spit out real Include line */
    STcopy( ERx("eqdef.bas"), buf );
    NMloc( FILES, FILENAME, buf, &loc );
    LOtos( &loc, &lname );
    CVlower( lname );	
    gen_include( lname, (char *)0, (char *)0 );

    if (first)
    {
      /* Now to show off */
	NMgtAt( ERx("II_BAS_WHO"), &lname );
	if (lname != (char *)0 && *lname != '\0')
	{
	    SIprintf( ERx("## EQUEL BASIC\n") );
	    SIprintf(
		ERx("## Written by Barbara J. Banks (RTI), March 1986.\n") );
	    SIflush( stdout );
	}
	first = FALSE;
    }
}


/*
+* Procedure:	gen_makesconst 
** Purpose:	Make a host language string constant.
**
** Parameters:	flag  - i4    - G_INSTRING  - Nested in another string (DB 
**					       string),
**		        	 G_REGSTRING - String sent by itself in a call.
**		instr - char * - Incoming string.
**		diff  - i4 *  - Mark if there were changes made to string.
-* Returns:	char *         - Pointer to static converted string.
**
** Notes:
** 1. Special Characters.
** 1.1.	Include the '\' character before we send a double quote to the 
**	database, if it was not already preceded by a '\'.
** 1.2.	Process the host language quote embedded in a string.  In BASIC
**	we delimit strings with the single quote.  Therefore if we
**	come across a single quote within the string, we complain and 
**	replace it with a double quote (preceded by a '\' if it's to
**	be sent to the backend).
*/

char	*
gen_makesconst( flag, instr, diff )
i4	flag;
char	*instr;
i4	*diff;
{
    static	char	obuf[G_MAXSCONST +4] ZERO_FILL; /* Converted string */
    register	char	*icp;				/* To input */
    register	char	*ocp = obuf;			/* To output */
    register	i4	esc = 0;			/* '\' was used */
    register	i4	errquote = 0;			/* "'" was used */

    *diff = 0;

    if (flag == G_INSTRING)		/* No need to escape */
    {
	*ocp++ = '"';
	*diff = 1;
    }
    for (icp = instr; *icp && (ocp < &obuf[G_MAXSCONST]);)
    {
	if (*icp == '\\')		/* Leave '\' may be DB or IIPRINTF */
	{	
	    *ocp++ = '\\';
	    esc = 1;
	    icp++;
	}
	else if (*icp == '"')
	{
	    icp++;
	    /*
	    ** Either entered via escaping it with a '\',  or alone
	    */
	    if (flag == G_INSTRING && !esc)	/* DB needs escape */
		*ocp++ = '\\';
	    *ocp++ = '"';
	    *diff = 1;
	    esc = 0;
	}
	else if (*icp == '\'')
	{
	    /* 
	    ** Single quotes may not be embedded in BASIC string constants
	    ** since we delimit them with single quotes and there is no
	    ** BASIC way to escape them.
	    */
	    icp++;
	    errquote = 1;
	    if (flag == G_INSTRING)
	    {
		*ocp++ = '\\';
	    }
	    *diff = 1;
	    *ocp++ = '"';
	}
	else
	{
	    CMcpyinc(icp, ocp);
	    esc = 0;
	}
    }
    if (flag == G_INSTRING)		/* Escape closing quote */
    {
	*ocp++ = '"';
    }
    *ocp = '\0';
    if (errquote)
	er_write( E_E30017_hbQUOTE, EQ_WARN, 0 );
    return obuf;
}



/*
** The following routines are local routines to format arguments inside of
** the corresponding runtime call.  This is synchronized with the argument
** manager.
*/

/*
+* Procedure:	gen__II 
** Purpose:	Generate the real II call with the necessary arguments, already
**	    	stored by the argument manager.
**
** Parameters: 	func  - i4 - Function index.
** Returns: 	None
**
** Notes:
**	This routine assumes language dependency, and knowledge of the actual 
**	arguments required by the call.
**
**	If the GEN_DML bit is on in "func", then vector out to the alternate
-*	gen table.  It is a fatal error if there is none.
*/

void
gen__II( func )
i4	func;
{
    register GEN_IIFUNC		*g;

    g = gen__functab( func );
    gen__obj( TRUE, g->g_IIname );
    if (g->g_func)
    {
	out_emit( ERx("(") );
	(*g->g_func)( g->g_flags, g->g_numargs );
	out_emit( ERx(")") );
    }
    arg_free();
}

/*
+* Procedure:	gen_all_args 
** Purpose:	Format all the arguments stored by the argument manager.
**
** Parameters: 	flags   - i4 - Flags and Number of arguments required.
**		numargs - i4 
** Returns: 	0
**
** This routine should be used when no validation of the number of arguments 
** needs to be done. Hopefully, if there was an error before (or while) 
-* storing the arguments this routine will not be called.
*/

i4
gen_all_args( flags, numargs )
i4	flags, numargs;
{
    register	i4 i;

    for (i = 1; i <= numargs; i++)	/* Fixed number of arguments */
    {
	gen_data( flags, i );
	if (i < numargs)
	    out_emit( ERx(",") );
    }
    return 0;
}

/*
+* Procedure:	gen_var_args 
** Purpose:	Format varying number of arguments stored by the argument 
**		manager.
**
** Parameters: 	flags   - i4 - Flags and Number of arguments required.
**		numargs - i4 - Unused
** Returns: 	0
**
** As gen_all_args() this routine should be used when no validation of 
-* the number of arguments needs to be done.
*/

i4
gen_var_args( flags, numargs )
i4	flags, numargs;
{
    register	i4  i;

    while (i = arg_next())		/* Varying num of arguments */
    {
	gen_data( flags, i );
	out_emit( ERx(",") );
    }
    gen__int( BobjVAL, 0 );		/* Emit a null argument */
    return 0;
}

/*
+* Procedure:	gen_data 
** Purpose:	Whatever is stored by the argument manager for a particular 
**	      	argument, dump to the output file.  
**
** Parameters: 	
**	arg	- i4 - Argument number.
**	flags	- i4 - To trim or not to trim
-* Returns: 	None
**
** Notes:
** 1.  This routine should be used when no validation of the data of the 
**     argument needs to be done.  Hopefully, if there was an error before 
**     (or while) storing the argument this routine will not be called.  
** 2.  Integer (constant and variable) data is always passed by value; 
**     strings get processed through gen_sdesc which will pass them by 
**     by reference; floats theoretically never come through here but we 
**     handle them just in case.
*/

void
gen_data( flags, arg )
register i4	flags, arg;
{
    i4		num;
    f8		flt;
    char	*s;
    SYM		*sy;

    if (sy = arg_sym(arg))		/* User variable */
    {
	switch (sym_g_btype(sy))
	{
	  case T_INT:
	    gen__name( BobjVAL, FALSE, arg_name(arg) );
	    break;
	  case T_CHAR:
	    gen_sdesc( TRUE, arg_name(arg) );
	    break;
	  case T_FLOAT:		/* Can't happen */
	  case T_PACKED:	/* Can't happen */
	  default:
	    gen__name( BobjPLAIN, FALSE, arg_name(arg) );
	    break;
	}
	return;
    }

    /* Based on the data representation, dump the argument */
    switch (arg_rep(arg))
    {
      case ARG_INT:  
	arg_num( arg, ARG_INT, &num );
	gen__int( BobjVAL, num );
	break;
      case ARG_FLOAT:  			/* Can't happen */
	arg_num( arg, ARG_FLOAT, &flt );
	gen__float( flt );
	break;
      case ARG_CHAR:  
	switch (arg_type(arg))
	{
    	  case ARG_CHAR: 	/* String constant - never needs trimming */
	    if (s = arg_name(arg))
		gen_sdesc( FALSE, s );
	    else		/* Fake a null pointer */
		gen__int( BobjVAL, 0 );
	    break;
	  case ARG_INT:
	    gen__name( BobjVAL, TRUE, arg_name(arg) );
	    break;
	  case ARG_FLOAT:		
	  case ARG_PACK:		
	  default:  		/* Should never occur for BASIC */
	    gen__name( BobjPLAIN, FALSE, arg_name(arg) );
	    break;
	}
	break;
    }
}

/*{
+*  Name:  gen_hdl_args - Generate the arguments stored by the argument manager.
**
**  Description:
**	Generate two arguments for the IILQsetHandler call:
**		Integer representing handler type specified by SET_SQL. 
**		Function pointer to user handler - or "0".
**
**  Inputs:
** 	flags   - i4 - Flags and Number of arguments required.
**      numargs - i4
**
**  Outputs:
**	Returns 0.
**      Errors:
**          E_EQ0090_HDL_TYPE_INVALID - Invalid SET_SQL Handler Value.
**
**  Side Effects:
**	Generates arguments into preprocessor output file for a handler call.
-*	
**  History:
**	01-mar-1991 - (kathryn) Written.
**	14-jan-1993 (lan)
**	    Modified to generate correct arguments for IILQldh_LoDataHandler.
*/
i4
gen_hdl_args(flags, numargs )
i4     flags, numargs;
{
    i4         num;
    i4	        arg;
    char	buf[ G_MAXSCONST +1 ];
    char	argbuf[ G_MAXSCONST +1 ];

    arg_num( 1, ARG_INT, &num );
    gen__int( BobjVAL, num );
    gen__obj( TRUE, ERx(",") );

    arg = 2;
    if (flags == G_DAT)
    {
	gen_null( arg );
	arg++;
    }
    STprintf(buf,ERx("%s"),arg_name(arg));
    switch (arg_type(arg))
    {
        case ARG_INT:
    		if (STcompare(buf,ERx("0")) == 0)
			gen__name( BobjVAL, TRUE, buf);
		else
		{
		    gen__obj( TRUE, ERx("0") );
    		    er_write(E_EQ0090_HDL_TYPE_INVALID, EQ_ERROR, 1, buf);
		}
    		break;
        case  ARG_RAW:
		gen__name( BobjPLAIN, FALSE, buf);
    	        break;
        default:
		gen__obj( TRUE, ERx("0") );
    		er_write(E_EQ0090_HDL_TYPE_INVALID, EQ_ERROR, 1, buf);
		break;
    }
    arg++;
    if (flags == G_DAT)
    {
    	gen__obj( TRUE, ERx(",") );
    	STprintf(argbuf,ERx("%s"),arg_name(arg));
    	if (STcompare(argbuf,ERx("0")) == 0)
		gen__name( BobjVAL, TRUE, argbuf);
	else
		gen__name( BobjPLAIN, FALSE, argbuf);
    }
    return 0;
}

/*{
+*  Name:  gen_datio - Generate arguments for GET DATA and PUT DATA statements.
**
**  Description:
**	Generates one I/O variable followed by remaining integer arguments. 
**	It is assumed that all arguments have been previously validated for 
**	correct datatype.  The I/O variable is always passed by reference.  
**	There are two possible flags
**	G_RET (GET DATA) and G_SET (PUT DATA), which determine how the
**	the remaining arguments will be passed. Note that for the GET/PUT
**	DATA statements all arguments are optional.  The "attrs" parameter is
**      a bit mask set/stored by eqgen_tl() and indicates which attributes were
**      specified: II_DATSEG if segment attribute was given and II_DATLEN if
**      maxlength was specified in the GET DATA stmt or segmentlength was
**      specified for the PUT DATA stmt. The "attrs" mask is the last argument
**      stored and the first parameter generated.
**
**    Flags:
**       G_RET - GET DATA - pass non-I/0 vars by reference
**       G_SET - PUT DATA - pass non-I/O vars by value.
**
**    Arguments Generated:
**
**        Argument  type 	
**	  ==============
**	  attrs  :  int		mask indicating attributes specified.
**	  type   :  int   	data type of I/O variable.
**	  io_var :  PTR		address of I/O var to get/put the data value.
** 	G_RET
**        maxlen :  int         G_RET only - passed by value.
**	  seglen :  (long *)	address of variable to retrieve integer.
**	  dataend:  (long *)	address of variable to retrieve integer.
**	G_SET    
**	  seglen :  int		integer value passed to Ingres runtime system.
**	  dataend:  int		integer value passed to Ingres runtime system.
**
** Note: 
**   -  VMS is a special case when the I/O data is of type character.  In 
**      case we actually change the name of the call from 'IIabcfunc' to
**      'IIxabcfunc' and pass the VMS string descriptor.  The 'IIx' layer
**      maps the character data type to an internal data type which is
**      understood to represent 'a non-C string'.   For Unix, since all
**      calls go through a layer, there is no need for the 'IIx' convention,
**      but in that layer, character data is mapped to the internal data type.
**      When passing strings as 'objects' (i.e., form names, etc.)  we generate
**      a call to IIsd which will pass back a C-string at runtime.  In other
**      words, names must always go as C-strings, whereas data can now be
**	passed without the EOS terminator.
**
**  Inputs:
**	func:	- Function number.
**
**  Outputs:
**      Returns 0.
**      Errors: None.
**	   
**
**  Side Effects:
**      Generates arguments into preprocessor output file. 
-*
**  History:
**	15-jan-1993   (kathryn) Written.
**      01-oct-1993 (kathryn)
**          First arg generated for GET/PUT DATA stmts ("attrs" mask) is the
**          last arg stored.  It is now set/stored in eqgen_tl(), rather than
**          determined here.
*/

i4
gen_datio(func)
i4     func;
{
    register GEN_IIFUNC         *gio;           /* Current I/O function */
    SYM                         *sy;            /* Symtab info */
    register i4                 ar;             /* What is the I/O argument */
    i4                          artype;         /* Type of I/O argument */
    i4 				type;
    i4 				len;
    i4                          i4num;          /* Numerics stored as strings */
    i4 				attrs;
    f8                          f8num;
    char                        datbuf[ SC_NUMMAX+1 ];  /* For some integers */
    char                        *data = datbuf;
    char                        *strdat;        /* String rep */

    /* Process the I/O argument */

    strdat = (char *)0;

    ar = 1;
    if ((sy = arg_sym(ar)) != (SYM *)0)
    {
	data = arg_name(ar);
	type = sym_g_btype(sy);
	len = sym_g_dsize(sy);
    }
    else if (!arg_name(ar))
    {
	type = T_NONE;
	len = 0;
    }
    else
    {
	if (artype = arg_rep(ar) == ARG_CHAR)
	{
	    artype = arg_type(ar);
	    strdat = arg_name(ar);
	}
 	switch (artype)
	{
	    case ARG_INT:
		if (strdat)
		    STcopy(strdat, data);
		else
		{
		    arg_num( ar, ARG_INT, &i4num );
		    CVna( i4num, data );
		}
		STcat( data, ERx("%")); /* Add '%' or VMS will treat as float */
		len = sizeof(i4);
		type = T_INT;
		break;

	    case ARG_FLOAT:
		if (strdat)    
		    data = strdat;
		else	/* Convert f8 to char - never really happens */
		{
		    arg_num( ar, ARG_FLOAT, &f8num );
		    STprintf( data, ERx("%f"), f8num, '.' );
		}
		gen_asn( FALSE, data, ERx("IIf8") );
		out_emit( ERx("\n") );
		type = T_FLOAT;
		len = sizeof(f8);
		break;

	    case ARG_PACK:
                if (strdat)
		    data = strdat;
		len = 0;
		type = T_PACKASCHAR;
		break;

            case ARG_CHAR:
	    default:
		if (strdat)
		    data = strdat;
                len = 0;
                type = T_CHAR;
                break;
	}
    }
    gio = gen__functab( func );
    gen__obj( TRUE, ERx("call ") );
    if (type == T_CHAR)
    {
        gen__obj( TRUE, ERx("IIx") );
        out_emit( gio->g_IIname+2 );
    }
    else
        gen__obj( TRUE, gio->g_IIname );
    out_emit( ERx("(") );
    arg_num( gio->g_numargs, ARG_INT, &attrs );
    gen__int( BobjVAL,attrs);
    out_emit( ERx(",") );

    gen__int( BobjVAL, type == T_NUL ? -type : type);   /* EQUEL type */
    out_emit( ERx(",") );

    gen__int( BobjVAL, len  );                  	/* EQUEL length */
    out_emit( ERx(",") );
    
    switch (type)                       /* I/O argument based on type */
    {
      case T_CHAR:
        /*  Literals and vars go by VMS desc */
        if (sy != (SYM *)0)
            gen__obj( TRUE, data );
        else
            gen_sconst( data );
        break;

      case T_NUL:
      case T_NONE:
        gen__int( BobjVAL, 0 );         /* Simulate a null pointer */
        break;

      case T_PACKASCHAR:                /* This is always a literal */
        gen_sdesc(FALSE, data);
        break;

      case T_INT:
        if (sy == (SYM *)0)
        {
            /*
            ** We send the 2nd param as FALSE telling gen__name not to
            ** add a '%' after this integer parameter.
            */
            gen__name( BobjVAL, FALSE, data );
            break;
        }
        /* Fall through with else */

      case T_FLOAT:
      case T_PACKED:
      default:
        /* No mechanism -- will go by reference */
        gen__name( BobjPLAIN, FALSE, data );
        break;
    }
    for (ar = 2; ar < gio->g_numargs; ar++)
    {
	gen__obj( TRUE, ERx(",") );
	if (!arg_name(ar))	
	    gen__int( BobjVAL, 0 );
        else if ((gio->g_flags == G_RET) && (ar != 2))
	{
	    gen__name(BobjPLAIN,FALSE, arg_name(ar));
	}
	else if ((sy = arg_sym(ar)) != (SYM *)0)
	{
	    gen__name( BobjVAL, FALSE, arg_name(ar) );
	}
	else
	    gen__name( BobjVAL, TRUE, arg_name(ar) );	
    }
    out_emit( ERx(")") );
    arg_free();
    gb_retnull = (char *)0;
    return 0;
}


/*
+* Procedure:	gen_io
** Purpose:	Generate an I/O variable to the output file.
**
** Parameters: 
**	func - i4 - Function number.
-* Return Values:
**	0
**
** Notes:
**  1.  Where is the I/O argument.
**	Routine:  gen_io
**
**	Any I/O argument has the following format:
**
**		IIretfunc( indaddr, var_desc, var [object(s)] );
**		IIsetfunc( [object(s),], indaddr, var_or_val_desc, var_or_val );
**
**	where var_desc or var_or_val_desc is:
**		ISVAR, TYPE, LEN
**	and indaddr is a pointer to the user's null indicator.
**
**      We figure out which one is the null indicator, which is the argument 
**	which must be described and the others we just dump.  For example, 
**	IIputfldio has one extra object (fieldname), while IIiqfsio has three 
**	extra objects (flag, objectname and rownumber) and IIgetdomio has no 
**	extra objects.
**
**   2. For variables that are stored inside non-standard storage (packed
**	decimals) and for float literals on IIsetfuncs we generate an
**	assignment statement into an 8-byte float either before or after
**	the call as necessary.
**
**   3. Table of passing mechanisms for described variables:
**
**		ISVAR	TYPE	LEN	BASIC mech	NOTES
**		-----  ------   ----    ----------      -----------------
**   INPUT (IIsetfunc)
**   Literals:
**	int	0      T_INT	4	by value        Concat with '%'
**	float	1      T_FLOAT	8	by ref		Move to IIF8
**	decimal 1      T_PACKASCHAR 0	by ref		Pass as Char string
**	char	1      T_CHAR	0	by ref		Null terminate
**
**   Vars/User-def constants:
**	int	0      T_INT	4	by value	No concat with '%'
**	float	1      T_FLOAT	d_size	by ref		A BASIC float
**	decimal 1      T_PACKED p,,s    by ref		A BASIC decimal
**	char	1      T_CHAR	0	by desc		Use "IIx"
**	
**   OUTPUT (IIretfunc)
**   Literals:
**	Error
**	
**   Vars
**	int	1      T_INT	d_size	by ref		VMS default
**	float	1      T_FLOAT	d_size	by ref		A BASIC float
**	decimal 1      T_PACKED p,,s    by ref		A BASIC decimal
**	char	1      T_CHAR	0	by desc		use "IIx"
** 	
*/

i4 
gen_io( func )
i4 	func;
{
    register GEN_IIFUNC		*gio;		/* Current I/O function */
    SYM				*sy;		/* Symtab info */
    register i4 		ioarg;		/* What is the I/O argument */
    i4 				artype;		/* Type of I/O argument */
    i4 				i4num;		/* Numerics stored as strings */
    f8				f8num;
    char			datbuf[ SC_NUMMAX+1 ];	/* For some integers */
    char			*data = datbuf;
    char			*strdat;	/* String rep */

    gio = gen__functab( func );

    /* 
    ** Figure out which is the I/O argument based on number of args and mode.
    ** It is either the first argument (RET) or the last (SET), as in:
    ** 		SET (name = value)	or
    **		RET (var = name)
    */
    ioarg = (gio->g_flags == G_RET) ? 2 : gio->g_numargs;

    if ((sy = arg_sym(ioarg)) == (SYM *)0)    /* User literal used */
    {
	if (gio->g_flags == G_RET || gio->g_flags == G_RETSET)	/* User error */
	{
	    arg_free();
	    return 0;
	}

	strdat = (char *)0;
	if ((artype = arg_rep(ioarg)) == ARG_CHAR)
	{
	    artype = arg_type( ioarg );		/* Real user type */
	    strdat = arg_name( ioarg );		/* String - numeric or char */
	}
	switch (artype)
	{
	  case ARG_INT:
	    if (strdat)
		STcopy(strdat, data);		/* Need local copy of arg */
	    else		/* Convert numeric to string */
	    {
		arg_num( ioarg, ARG_INT, &i4num );
		CVna( i4num, data );
	    }
	    /* 
	    ** Will send i4 by value.
	    ** We concatenate the '%' here because gen_iodat can't
	    ** tell the difference between an integer variable and an integer
	    ** literal when they both go by value.
	    */ 
	    STcat( data, ERx("%") );	/* Add '%' or VMS will treat as float */
	    gen_iodat( func, FALSE, T_INT, sizeof(i4), data );
	    break;
	  case ARG_FLOAT:
	    if (strdat)
		data = strdat;
	    else		/* Convert f8 to char - never really happens */
	    {
		arg_num( ioarg, ARG_FLOAT, &f8num );
		STprintf( data, ERx("%f"), f8num, '.' );  /* Not very precise */
	    }
	    /*
	    ** VMS BASIC plain floating constants will be moved into
	    ** a LONG_FLOAT argument.
	    */
	    gen_asn( FALSE, data, ERx("IIf8") );
	    out_emit( ERx("\n") );
	    gen_iodat( func, TRUE, T_FLOAT, sizeof(f8), ERx("IIf8") );
	    break;
	  case ARG_PACK:
	    gen_iodat( func, FALSE, T_PACKASCHAR, 0, strdat );
	    break;
	  case ARG_CHAR:
	  default:
	    /* Will be decided on how to send this - IIsd or with Null */
	    gen_iodat( func, FALSE, T_CHAR, 0, strdat );
	    break;
	}
    } /* If user literal */
    /* 
    ** Is a variable (or user-defined constant in which case flags
    ** had better not be G_RET)
    */
    else
    {
	data = arg_name( ioarg );
	artype = sym_g_btype( sy );

	switch (artype)
	{
	  case T_INT:
	    if (gio->g_flags == G_SET)	/* Will be sent by value */
	      gen_iodat( func, FALSE, T_INT, sizeof(i4), data );
	    else			/* Must received into actual var */
	      gen_iodat( func, TRUE, T_INT, sym_g_dsize(sy), data );
	    break;

	  case T_NUL:
	    gen_iodat( func, FALSE, artype, 1, data );
	    break;

	  case T_CHAR:
	  case T_FLOAT:
	  case T_PACKED:
	  default:			/* error, generic null */
	    gen_iodat( func, TRUE, artype, sym_g_dsize(sy), data );
	    break;
	}
    }

    arg_free();
    gb_retnull = (char *)0;
    return 0;
}

/*
+* Procedure:	gen_iodat
** Purpose:	Make real I/O call.
** Parameters: 
**	func 	- i4	- Function index.
**	isvar	- i4	- Is a variable (as opposed to a literal)
**	type	- i4	- Data type.
**	len	- i4	- Data length.
**	data	- char *- Data name (may be var name or data as a string).
** Returns: 	None
-* Notes:	See comments in gen_io of details.
*/

void
gen_iodat( func, isvar, type, len, data )
i4		func;
i4		isvar;
i4		type; 
i4		len;	
register char	*data;
{
    register GEN_IIFUNC	*gio;		/* Current I/O function */
    i4			ar;
    char		buf[ G_MAXSCONST +1 ];

    gen__obj( TRUE, ERx("call ") );
    gio = gen__functab(func);

    /* Emit func name -- with special prefix if retrieving strings */
    if (type == T_CHAR)
    {
	gen__obj( TRUE, ERx("IIx") );
	out_emit( gio->g_IIname+2 );
    }
    else
	gen__obj( TRUE, gio->g_IIname );
    out_emit( ERx("(") );

    /* Do we need a name or other data before I/O argument */
    if (gio->g_flags == G_SET || gio->g_flags == G_RETSET)  /* (name = val) */
    {
	for (ar = 1; ar <= gio->g_numargs-2; ar++)
	{
	    gen_data( 0, ar );
	    gen__obj( TRUE, ERx(",") );
	}
	gen_null(gio->g_numargs -1);
    }
    else
    {
	gen_null(1);
    }

    /* Dump data information */
    /* Indirect or not (but strings are always indirect */
    gen__int( BobjVAL, isvar || type == T_CHAR || type == T_PACKASCHAR );   
    out_emit( ERx(",") );

    gen__int( BobjVAL, type == T_NUL ? -type : type);		/* EQUEL type */
    out_emit( ERx(",") );

    gen__int( BobjVAL, len  );			/* EQUEL length */
    out_emit( ERx(",") );

    switch (type) 			/* I/O argument based on type */
    {
      case T_CHAR:
	/*  Literals and vars go by VMS desc */
	if (isvar)	
	    gen__obj( TRUE, data );	
	else
	    gen_sconst( data );
	break;

      case T_NUL:
	gen__int( BobjVAL, 0 );		/* Simulate a null pointer */
	break;

      case T_PACKASCHAR:		/* This is always a literal */	
	gen_sdesc(FALSE, data);
	break;

      case T_INT:
	if (!isvar)	
	{
	    /*
	    ** We send the 2nd param as FALSE telling gen__name not to 
	    ** add a '%' after this integer parameter because gen_io
	    ** concatenated it already where necessary.
	    */
	    gen__name( BobjVAL, FALSE, data );
	    break;
	}
	/* Fall through with else */

      case T_FLOAT:
      case T_PACKED:
      default:
	/* No mechanism -- will go by reference */
	gen__name( BobjPLAIN, FALSE, data );
	break;
    }

    /* Do we need a name or other data after I/O argument */
    if (gio->g_flags == G_RET)			/* (var = name ) */
    {
	for (ar = 3; ar <= gio->g_numargs; ar++)
	{
	    gen__obj( TRUE, ERx(",") );
	    gen_data( 0, ar );
	}
    }
    out_emit( ERx(")") );
}


/*
+* Procedure:	gen_asn
** Purpose:	Assign src to dst
** Parameters:
**	isret	- bool		- Is this an assignment AFTER a retrieval
**	src	- char *	- source string
**	dst1   	- char *	- destination string
** Return Values:
-*	None
** Notes:
**	If this an assignment after a retrieval, then we may need to generate
**	an IF statement if a null indicator was used with a dummy type.  See
**	the declaration of gb_retnull for an example.
*/

void
gen_asn( isret, src, dst )
bool	isret;
char	*src, *dst;
{
    if (isret && gb_retnull)
    {
	gen__obj(TRUE, ERx("if ("));
	gen__obj(TRUE, gb_retnull);
	gen__obj(TRUE, ERx(" <> -1) then\n"));	
	gen_do_indent( 1 );
    }
    gen__obj( TRUE, dst );
    gen__obj( TRUE, ERx(" = ") );
    gen__obj( TRUE, src );

    if (isret && gb_retnull)
    {
	out_emit(ERx("\n"));
	gen_do_indent( -1 );
	gen__obj(TRUE, ERx("end if"));
	gb_retnull = (char *)0;
    }
}

/*
+* Procedure:	gen_IIdebug 
** Purpose:	Dump file name and line number for runtime debugging.
**
** Parameters:	None
-* Returns:	0
*/

i4
gen_IIdebug()
{
    char	fbuf[ G_MAXSCONST +1 ];

    if (eq->eq_flags & EQ_IIDEBUG)
    {
	STprintf( fbuf, ERx("%s%s%s"), eq->eq_filename,
	          eq->eq_infile == stdin ? ERx("") : ERx("."), eq->eq_ext );
	gen_sdesc( FALSE, fbuf );
	gen__obj( TRUE, ERx(",") );
	gen__int( BobjVAL, eq->eq_line );
    }
    /* else uses default parameters */
    else
    {
	gen__int( BobjVAL, 0 );
	out_emit ( ERx(",") );
	gen__int( BobjVAL, 0 );
    }
    return 0;
}


/*
+* Procedure:	gen_sdesc 
** Purpose:	Put out BASIC string as an untrimmed C string (concatenated with
**		a Null).
** Parameters:	isvar	- i4   - A var or a string constant
**		data	- char* - String to dump.
-* Returns:	None
**
** Notes:
**
*/

void
gen_sdesc( isvar, data )
i4 	isvar;
char	*data;
{
    if (isvar)
	gen__obj( TRUE, data );
    else
	gen_sconst( data );
    gen__obj( TRUE, ERx("+chr$(0) by ref") );
}

/*
+* Procedure:	gen_sconst 
** Purpose:	Generate a string constant.
**
** Parameters: 	str - char * - String
-* Returns: 	None
**
** Attempt to fit it on one line (hopefully current line) and spit it all out.
** If not use BASIC '+' concat operator.
*/

void
gen_sconst( str )
char	*str;
{
    if (str == (char *)0)
	return;

    if (G_LINELEN - out_cur->o_charcnt < 7)  /* Not enough room to be useful */
	gen__continue();

    while (gen__sconst(&str))
    {
	out_emit( ERx(" +") );		/* Continue literal */
	gen__continue();
    }
}

/* 
+* Procedure:	gen__sconst 
** Purpose:	Local utility for writing out the contents of strings.
**
** Parameters:	str - char ** - Address of the string pointer to write.
** Returns:	0 - No more data to dump,
-*		1 - Call me again.
**
** Notes:
** 1. The Equel preprocessor assumes that strings are the only thing that can be
**    broken across lines.  Based on the language we are using the break may
**    be implemented here, or higher up (see gen_sconst).
** 2. Care should be take not to split lines in the middle of compiler escape
**    sequences - the most complex of which occur in C.  Not applicable in
**    BASIC.
** 3. In BASIC, we assume that linelen will be long enough to output at least
**    two chars before a split is made.  
*/

i4
gen__sconst( str )
register char	**str;
{
    register char	*cp;
    register i4		linelen;
    char		savech;
    char		*delim; 

    /*
    ** Delimit EQUEL literals with single quote; ESQL with double quotes
    */
    if (dml_islang(DML_ESQL))
	delim = ERx("\"");
    else
	delim = ERx("'");
    /*
    ** If an embedded quote is found that matches the delimiter, we assume  
    ** we have a host string in the declare section, and therefore, the string 
    ** is delimited with opposite quotes to avoid a BASIC compiler error. 
    */
    if (STindex(*str,delim,0))
	if (dml_islang(DML_ESQL))
	    delim = ERx("'");
	else
	    delim = ERx("\"");
    /* 
    ** We count 2 for two quotes and 2 for continuation (" +") and an
    ** extra one for safety (= 5)
    */
    linelen = G_LINELEN - out_cur->o_charcnt - 5;
    /* 
    ** Avoid continuation strings with a very short length
    */
    if (linelen - (i4)STlength(*str) >= -3)
    {
	out_emit( delim );
	out_emit( *str );
	out_emit( delim );
	return 0;
    }
    out_emit( delim );
    for (cp = *str; linelen > 0 && *cp;)
    {
	cp++; linelen--;		/* Skip current character */
    }
    if (*(cp +1) == ' ') 		/* Avoid breaking just before space */
	cp++;
    savech = *cp;
    *cp = '\0';
    out_emit( *str );
    out_emit( delim );
    *cp = savech;
    *str = cp;					/* More to go next time */
    return 1;
}

/*{
+*  Name: gen_null - Generate null indicator arg to IIput/IIget routines
**
**  Description:
**	If null indicator present, pass by reference.  Otherwise
**	pass a null pointer.
**
**  Inputs:
**	argnum		- Argument number
**
**  Outputs:
**	None
-*
**  Side Effects:
**	Generates an argument into preprocessor output file for an I/O call.
**	
**  Notes:
**	Indicator arguments do not need to be passed with a type/length
**	descriptor since they are always expected to be pointers to two-
**	byte integers (or NULL)
**
**  History:
**	07-jul-1987 - written (bjb)
*/

void
gen_null( argnum )
i4	argnum;
{
    if ((arg_sym(argnum)) == (SYM *)0)		/* No indicator variable? */
    {
	gen__int( BobjVAL, 0  );		/* Fake a null pointer */
	gb_retnull = (char *)0;
    }
    else
    {
	gen__name( BobjPLAIN, FALSE, arg_name(argnum) );
	gb_retnull = arg_name(argnum);
    }
    gen__obj( TRUE, ERx(",") );
}

/*
+* Procedure:	gen__name
** Purpose:	Spit out a name and a parameter passing mechanism
** Parameters:
**	how	- 	i4	- BobjVAL or BobjPLAIN
**	addpercent - 	i4 	- Concat a '%' so BASIC won't assume float
**	name	- 	char* 	- The name to print
** Return Values:
-*	None
** Notes:
**	This routine is called from gen_data and gen_iodat when a
**	parameter is to be emitted.  
** Imports modified:
**	None
*/

void
gen__name( how, addpercent, name )
i4	how, addpercent;
char	*name;
{
    char	buf[ SC_WORDMAX+1 ];

    switch (how)
    {
      case BobjVAL:
	if (addpercent) 	/* Must be dealing with integer literal */
	  STprintf(  buf, ERx("%s%% by value"), name );
	else
	  STprintf( buf, ERx("%s by value"), name );
	gen__obj( TRUE, buf );
	break;
      case BobjPLAIN:
      default:
	gen__obj( TRUE, name );
    }
}

/*
+* Procedure:	gen__int 
** Purpose:	Does local integer conversion.
**
** Parameters: 	
**		num - i4 - Integer to put out.
**		how - i4 - Qualifying code (if any) to put out
-* Returns: 	None
*/

void
gen__int( how, num )
i4	how, num;
{
    char	buf[20];

    switch (how)
    {
      case BobjVAL:
	/* Integer being used as a parameter */
	STprintf( buf, ERx("%d%% by value"), num );
	break;
      case BobjPLAIN:
	STprintf( buf, ERx("%d%%"), num );
	break;
    }
    gen__obj( TRUE, buf );
}

/*
+* Procedure:	gen__float 
** Purpose:	Does local floating conversion.
**
** Parameters: 	num - f8 - Floating to put out.
-* Returns: 	None
*/

void
gen__float( num )
f8	num;
{
    char	buf[50];

    /* 
    ** Right now we should never arrive in this routine.
    */
    STprintf( buf, ERx("%f by value"), num, '.' );
    gen__obj( TRUE, buf );
}

/*
+* Procedure:	gen__obj 
** Purpose:	Write an object within the line length. This utility is used for
**	     	everything but string constants.
**
** Parameters:	ind - i4 	- Indentation flag,
**		obj - char *	- Object to dump.
-* Returns:	None
*/

void
gen__obj( ind, obj )
i4		ind;
register char	*obj;
{
    /* The 3 is for unused space once the object is reached */
    if ((i4)STlength(obj) > G_LINELEN - out_cur->o_charcnt -3)
	/* Line up the line continuation symbols */
	gen__continue();
    else
    {
	/* Always begin printing after the margin */
	if (out_cur->o_charcnt == 0)
	    out_emit( G_MARGIN );
    
	/* 
	** We may be at this position because of indentation just done or
	** because we emitted a line number
	*/
	if (ind && out_cur->o_charcnt == G_MINMARG)
	    out_emit( g_indent );
    }

    if (obj)
	out_emit( obj );
}

/*
+* Procedure:	gen__functab
** Purpose:	Point to the correct gen__ftab entry.
** Parameters:
**	func	- i4	- Function index.
** Return Values:
-*	GEN_IIFUNC *	- A pointer to the correct entry in the correct table.
** Notes:
**	If the GEN_DML bit is on then use the alternate table; it is a fatal
**	error if there is no alternate.
*/

GEN_IIFUNC *
gen__functab( func )
i4	func;
{
    if (func & GEN_DML)
    {
	func &= ~GEN_DML;
	if (dml->dm_gentab)
	    return &((GEN_IIFUNC *)(dml->dm_gentab))[func];
	else 		/* FATAL Internal Error */
	    er_write( E_EQ0002_eqNOTABLE, EQ_FATAL, 0 );
    } else
	return &gen__ftab[func];
}

/*
+* Procedure:	gen_do_indent 
** Purpose:	Update indentation value.
**
** Parameters:	i - i4 - Update value.
-* Returns:	None
** Notes:
**		Indent value of G_IND_PIN means outdent one level,
*		but not past the margin.
*/

void
gen_do_indent( i )
i4	i;
{
    register	i4	n_ind;
    register	i4	o_ind;

    /* 
    ** Allow global indent level to exceed limits so that popping back out is 
    ** done correctly, but make sure no blanks or nulls are put in random space.
    */
    o_ind = g_indlevel;
    if (i == G_IND_RESET)		
    {
	g_indlevel = G_MININDENT;
    } else if (i == G_IND_PIN)		/* outdent, but not past margin */
    {
	g_indlevel--;
	if (g_indlevel < G_MININDENT)
	    g_indlevel = G_MININDENT;
    } else
	g_indlevel += i;
    n_ind = g_indlevel;
    if (o_ind < G_MININDENT)
	o_ind = G_MININDENT;
    else if (o_ind >= G_MAXINDENT)
	o_ind = G_MAXINDENT - 1;
    if (n_ind < G_MININDENT)
	n_ind = G_MININDENT;
    else if (n_ind >= G_MAXINDENT)
	n_ind = G_MAXINDENT - 1;
    g_indent[ o_ind * 2 ] = ' ';
    g_indent[ n_ind * 2 ] = '\0';
}

/*
+* Procedure:	gen_unsupp 
** Purpose:	Do not implement undocumented function for new language.
**	        Used for FORMDATA, INQUIRE_FRS, SET_FRS and GETOPER
** Parameters:	
**	func - i4 - function index.
**	args - i4 - Argument count (unused).
-* Returns:	0
*/

i4
gen_unsupp( func )
i4	func;
{
    switch (func)
    {
      case IIFDATA:
      	/* First one after the "while (IIfnames) loop" */
	er_write( E_EQ0076_grNOWUNSUPP, EQ_ERROR, 1, ERx("FORMDATA") );
	break;
      case IIGETQRY:
      	/* First one after the "if (IIfsetio) then" */
	er_write( E_EQ0076_grNOWUNSUPP, EQ_ERROR, 1, ERx("GETOPER") );
	break;
      case IIINQSET:
	er_write(E_EQ0076_grNOWUNSUPP, EQ_ERROR, 1, ERx("INQUIRE_FRS/SET_FRS"));
	break;
      case IIFRSINQ:
      case IIFRSSET:
	/* Do nothing as the error is already detected by IIINQSET */
	break;
    }
    return 0;
}

/*
+* Procedure:	gen_host 
** Purpose:	Generate pure host code from the L grammar (probably declaration
**	      	stuff).
**
** Parameters:	flag - i4	- How to generate object.
**		s    - char *	- String to generate.
-* Returns:	None
*/

void
gen_host( flag, s )
register i4	flag;
register char	*s;
{
    i4			f_loc;
    static	i4	was_key = 0;		/* Last object was a keyword */

    f_loc = flag & ~(G_H_INDENT | G_H_OUTDENT | G_H_NEWLINE);

    if (flag & G_H_OUTDENT)			/* Outdent before data */
	gen_do_indent( -1 );

    switch (f_loc)
    {
      case G_H_KEY:
	if (was_key && out_cur->o_charcnt != 0 && *s != ',')
		out_emit( ERx(" ") );
	gen__obj( TRUE, s );
	was_key = 1;
	break;

      case G_H_OP:
	gen__obj( TRUE, s );
	was_key = 0;
	break;

      case G_H_SCONST:
	if (was_key)
	    out_emit( ERx(" ") );
	gen_sconst( s );
	was_key = 1;
	break;

      case G_H_CODE:		/* Raw data with source program spacing */
	out_emit( s );
	was_key = 0;
	break;

      default:
	break;
    }
    if (flag & G_H_NEWLINE)
    {
	out_emit( ERx("\n") );
	was_key = 0;		/* Newline equivalent to space */
    }
    if (flag & G_H_INDENT)			/* Indent after data */
	gen_do_indent( 1 );
}

/*
+* Procedure:	gen_Blinenum
** Purpose:	Emit BASIC line number at left margin
** Parameters:
**	linenum	- *char	- number string
** Return Values:
-*	None
** Notes:
**	Emit line number and follow it with white space to position 
** subsequent output at the "no indentation" level.
**
** Imports modified:
**	None
*/

gen_Blinenum( linenum )
char	*linenum;
{
    if (linenum && *linenum != '\0')
    {
	if (out_cur->o_charcnt != 0)
	    out_emit( ERx("\n") );
	out_emit( linenum );
	out_emit( ERx(" ") );	/* Ensure at least one space */
	while (out_cur->o_charcnt < G_MINMARG)
	    out_emit( ERx(" ") );
    }
}

/*
+* Procedure:	gen__continue
** Purpose:	Continue a line of BASIC code
** Parameters:
**	None
** Return Values:
-*	None
** Notes:
**	Generate BASIC line continuation symbol ("&") and necessary
**	margins on next line.
*/

void
gen__continue()
{
    while (out_cur->o_charcnt < G_LINELEN)
	out_emit( ERx(" ") );
    out_emit( ERx(" &\n") );
    out_emit( G_MARGIN );
    out_emit( g_indent );
    out_emit( ERx("  ") );	/* indent continuation line a little more */
}

