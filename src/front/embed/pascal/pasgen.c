/*
** 20-jul-90 (sandyd) 
**	Workaround VAXC optimizer bug, first encountered in VAXC V2.3-024.
NO_OPTIM = vax_vms
*/

# include 	<compat.h>
# include	<er.h>
# include	<si.h>
# include 	<cm.h>
# include 	<st.h>
# include	<lo.h>
# include	<nm.h>
# include	<eqrun.h>
# include	<equel.h>
# include	<eqsym.h>
# include	<eqgen.h>
# include	<eqpas.h>
# include	<ereq.h>
# include	<ere2.h>

/*
** Copyright (c) 1984, 2001 Ingres Corporation
*/

/*
+* Filename:	pasgen.c 
** Purpose:	Maintains all the routines to generate II calls within specific
**	  	language dependent control flow statements.
**
** Language:	Written for VMS PASCAL.
**
** Defines:	gen_call( f )			- Standard Call template.
**		gen_if( oc, f, c, val )		- If Then template.
**		gen_else( oec, f, c, v, labs )	- Else template.
**		gen_if_goto(f, c, val, l, num)	- If Goto template.
**		gen_loop(oc, lb, le, n, f, c, v)- Loop template.
**		gen_goto( flag, lab, num )	- Goto template.
**		gen_label( inloop, lab, num )	- Label template.
**		gen_comment( flg, comm )	- Comment template.
**		gen_include( pre, suff, ext )	- Include template.
**		gen_eqstmt( oc )		- Open/close Equel statement.
**		gen_line( s )			- Line comment or directive.
**		gen_term()			- Terminate statement.
**		gen_declare()			- Process a declare statement.
**		gen_makesconst( s )		- Process a string constant.
**		gen_sconst( str )		- String constant.
**		gen_do_indent( i )		- Update indentation.
**		gen_host( flag, s )		- Host code (declaration) calls.
** Exports:	
**		gen_code( s )			- Macro defined as out_emit().
** Locals:
**		gen__goto( lab, num )		- Core of goto statement.
**		gen__label( ind, lab, num )	- Statement independent Label.
**		gen__II( f )			- Core of an II call.
**		gen_all_args( numargs )		- Dump all args in call.
**		gen_var_args()			- Varying number of argunents.
**		gen_io( mask, arg )		- Describe I/O arguments.
**		gen_data( arg )			- Dump different arg data.
**		gen_IIdebug()			- Runtime debug information.
**		gen_IIwrite( str )		- Controls IIwrite string lits.
**		gen_null()			- Dump indicator arg.
**		gen_sdesc( str )		- Generate string descriptor.
**		gen_asn( str, str )		- Generate assignment stmt.	
**		gen__name( i )			- Variable name.
**		gen__int( how, i )		- Integer.
**		gen__float( i )			- Float.
**		gen__obj( ind, obj )		- Write anything but strings.
**		gen__functab( func )		- Get function table pointer.
**              gen_hdl_args()                  - SET_SQL handler args.
**		gen_datio()			- PUT/GET DATA statements.
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
** 2. Some calls require special Pascal strongly typed functions - based on
**    the function and the type, it can be done here.
** 3. Routines that write data:
** 3.1. Some routines that actually write stuff make use of the external counter
**      out_cur->o_charcnt.  This is the character counter of the current line
**      being written to.  Whenever there is a call to emit code, this counter
**      gets updated even though the local code does not explicitly update it.
** 3.2. A call to gen__obj() which makes sure the object can fit on the line, 
**      needs to be made only when the data added may have gone over the end of 
**      the line.  In some cases, either when nothing is really being written,
**      or the data comes directly from the host language host code, the main
**      emit routine can be used.
**
** History:
**		30-oct-1984 	- Written for C (ncg)
**		01-dec-1985 	- Converted for PASCAL (mrw)
**		23-mar-1989	- Added cast to STlength. (teresa)
**              11-may-1989     - Added function calls IITBcaClmAct,
**                                IIFRafActFld, IIFRreResEntry to gen__ftab
**                                for entry activation. (teresa)
**		10-aug-1989	- Add function call IITBceColEnd for FRS 8.0.
**				  (teresa)
**		19-dec-1989	- Updated for Phoenix/Alerters. (bjb)
**		15-aug-1990 (barbara)
**		     Added IIFRsaSetAttrio to function table for per value
**		     setting of display attributes.
**		24-jul-1990	- Added decimal support. (teresal)
**		15-feb-1991	- Don't pass float literals as single
**				  precision, move to a temp float8 then
**				  pass as a double. (teresal)
**              22-feb-1991 (kathryn) Added:
**                      IILQssSetSqlio and IILQisInqSqlio.
**			IILQshSetHandler.
**		21-apr-1991 (teresal)
**			Add activate event call - remove IILQegEvGetio.
**		14-oct-1992 (lan)
**			Added IILQled_LoEndData.
**		14-jan-1993 (lan)
**			Added IILQldh_LoDataHandler.
**              15-jan-1993 (kathryn)
**                  Added gen_datio() function, and added entries for
**                  IILQlgd_LoGetData, and IILQlpd_LoPutData.
**		26-jul-1993 (lan)
**		    Added EXEC 4GL support routines.
**	07/28/93 (dkh) - Added support for the PURGETABLE and RESUME
**			 NEXTFIELD/PREVIOUSFIELD statements.
**	01-oct-1993 (kathryn)
**          First argument generated for GET/PUT DATA stmts ("attrs" bit mask),
**          will now be set/stored by eqgen_tl() rather than determined by
**          gen_datio, so add one to the number of args in the gen__ftab for
**          IILQlpd_LoPutData and IILQlgd_LoGetData.
**	15-feb-2001	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	20-jul-2007 (toumi01)
**	    add IIcsRetScroll and IIcsOpenScroll for scrollable cursors
*/


/* Max line length for C code - with available space for closing calls, etc */
# define	G_LINELEN	75

/* 
** Max size of converted string - allow some extra space over scanner for
** extra escaped characters required.
*/

# define 	G_MAXSCONST	300	

/* State for last statement generated to avoid illegal empty blocks */
static  bool	g_block_dangle = FALSE;


/* "how" for gen__int and gen__float */

# define	PbyNULL		0	/* unused */
# define	PbyNAME		1	/* unadorned */
# define	PbyVAL		2	/* %immed -unused as cast by proc def */
# define	PbyREF		3	/* %ref name */

/* 
** Indentation information - 
**     Each indentation is 2 spaces.  For PASCAL the indentation is made up of 
** spaces. For languages where margins are significant, and continuation of 
** lines must be explicitly flagged (Fortran) the indentation mechanism will 
** be more complex - Always start out at the correct margin, and indent from
** there. If it is a continuation, then flag it in the correct margin, and 
** then indent. In PASCAL g_indent[0-G_MININDENT-1] are always blanks.
*/

# define 	G_MAXINDENT	20
# define 	G_MININDENT	1

static		char	g_indent[ G_MAXINDENT + G_MAXINDENT ] =
		{
		  ' ', ' ', '\0',' ', ' ', ' ', ' ', ' ', ' ', ' ',
		  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '
		};

static	i4	g_indlevel = G_MININDENT;	/* Start out indented */


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

/* 
** Table of label names - correspond to L_xxx constants.
** Must be unique in the first few characters, to allow label uniqueness
** in 8 characters.
*/

static char	*gen_ltable[] = {
    ERx("IInolab"),			/* Should never be referenced */
    ERx("IIfd_Init"),
    ERx("IIfd_Begin"),
    ERx("IIfd_Final"),
    ERx("IIfd_End"),
    ERx("IImu_Init"),
    (char *)0,				/* We do have Else clauses */
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
char	*gen__label();
i4	gen__II();
i4	gen_all_args();
i4	gen_var_args();
i4	gen_io();
void	gen_data();
i4	gen_IIdebug();
void	gen_IIwrite();
void	gen_null();
void	gen_sdesc();
void	gen_asn();
void	gen__int();
void	gen__float();
void	gen__name();
void	gen__obj();
i4	gen_hdl_args();
i4	gen_datio();

/*
** Table of II calls - 
**     Correspond to II constants, the function to call to generate arguments, 
** the flags needed and the number of arguments.  For C there is very little
** argument information needed, as will probably be the case for regular 
** callable languages. For others (Cobol) this table will need to be more 
** complex and the code may be split up into partially table driven, and 
** partially special cased.
*/

GEN_IIFUNC	*gen__functab();

GLOBALDEF GEN_IIFUNC	gen__ftab[] = {
/*
** Index	Name			Function	Flags		Args
** -----------  --------------		--------------	-------------	-----
*/

/* 000 */    { ERx("IIf_"),		0,		0,		0 },

			/* Quel # 1 */
/* 001 */    { ERx("IIflush"),		gen_IIdebug,	0,		0 },
/* 002 */    { ERx("IInexec"),		0,		0,		0 },
/* 003 */    { ERx("IInextget"),	0,		0,		0 },
/* 004 */    { ERx("IInotrimio"),	gen_io,		G_SET,		2 },
/* 005 */    { ERx("IIparret"),		0,		0,/* unsupp */	0 },
/* 006 */    { ERx("IIparset"),		0,		0,/* unsupp */	0 },
/* 007 */    { ERx("IIgetdomio"),	gen_io,		G_RET,		2 },
/* 008 */    { ERx("IIretinit"),	gen_IIdebug,	0,		0 },
/* 009 */    { ERx("IIputdomio"),	gen_io,		G_SET, 		2 },
/* 010 */    { ERx("IIsexec"),		0,		0,		0 },
/* 011 */    { ERx("IIsyncup"),		gen_IIdebug,	0,		0 },
/* 012 */    { ERx("IItupget"),		0,		0,		0 },
/* 013 */    { ERx("IIwritio"),		gen_io,		G_SET,		3 },
/* 014 */    { ERx("IIxact"),		gen_all_args,	0,		1 },
/* 015 */    { ERx("IIvarstrio"),	gen_io,		G_SET,		2 },
/* 016 */    { ERx("IILQssSetSqlio"),   gen_io,		G_SET,		3 },
/* 017 */    { ERx("IILQisInqSqlio"),   gen_io,		G_RET,		3 },
/* 018 */    { ERx("IILQshSetHandler"), gen_hdl_args,  0,              2 },
/* 019 */    { (char *)0,               0,              0,              0 },
/* 020 */    { (char *)0,		0,		0,		0 },

		      /* Libq # 21 */
/* 021 */    { ERx("IIbreak"),		0,		0,		0 },
/* 022 */    { ERx("IIeqiqio"),		gen_io,		G_RET,		3 },
/* 023 */    { ERx("IIeqstio"),		gen_io,		G_SET,		3 },
/* 024 */    { ERx("IIerrtest"),	0,		0,		0 },
/* 025 */    { ERx("IIexit"),		0,		0,		0 },
/* 026 */    { ERx("IIingopen"),	gen_var_args,	0,		15 },
/* 027 */    { ERx("IIutsys"),		gen_all_args,	0,		3 },
/* 028 */    { ERx("IIexExec"),		gen_all_args,	0,		4 },
/* 029 */    { ERx("IIexDefine"),	gen_all_args,	0,		4 },
/* 030 */    { (char *)0,		0,		0,		0 },

		      /* Forms # 31 */
/* 031 */    { ERx("IITBcaClmAct"),	gen_all_args,	0,		4 },
/* 032 */    { ERx("IIactcomm"),	gen_all_args,	0,		2 },
/* 033 */    { ERx("IIFRafActFld"),	gen_all_args,	0,		3 },
/* 034 */    { ERx("IInmuact"),		gen_all_args,	0,		5 },
/* 035 */    { ERx("IIactscrl"),	gen_all_args,	0,		3 },
/* 036 */    { ERx("IIaddform"),	gen_all_args,	0,/*Indir ign*/	1 },
/* 037 */    { ERx("IIchkfrm"),		0,		0,		0 },
/* 038 */    { ERx("IIclrflds"),	0,		0,		0 },
/* 039 */    { ERx("IIclrscr"),		0,		0,		0 },
/* 040 */    { ERx("IIdispfrm"),	gen_all_args,	0,		2 },
/* 041 */    { ERx("IIprmptio"),	gen_io,		G_RETSET,	4 },
/* 042 */    { ERx("IIendforms"),	0,		0,		0 },
/* 043 */    { ERx("IIendfrm"),		0,		0,		0 },
/* 044 */    { ERx("IIendmu"),		0,		0,		0 },
/* 045 */    { ERx("IIfmdatio"),	gen_io,		G_RET,		3 },
/* 046 */    { ERx("IIfldclear"),	gen_all_args,	0,		1 },
/* 047 */    { ERx("IIfnames"),		0,		0,		0 },
/* 048 */    { ERx("IIforminit"),	gen_all_args,	0,		1 },
/* 049 */    { ERx("IIforms"),		gen_all_args,	0,		1 },
/* 050 */    { ERx("IIfsinqio"),	gen_io,		G_RET,		3 },
/* 051 */    { ERx("IIfssetio"),	gen_io,		G_SET,		3 },
/* 052 */    { ERx("IIfsetio"),		gen_all_args,	0,		1 },
/* 053 */    { ERx("IIgetoper"),	gen_all_args,	0,		1 },
/* 054 */    { ERx("IIgtqryio"),	gen_io,		G_RET,		3 },
/* 055 */    { ERx("IIhelpfile"),	gen_all_args,	0,		2 },
/* 056 */    { ERx("IIinitmu"),		0,		0,		0 },
/* 057 */    { ERx("IIinqset"),		gen_var_args,	0,		6 },
/* 058 */    { ERx("IImessage"),	gen_all_args,	0,		1 },
/* 059 */    { ERx("IImuonly"),		0,		0,		0 },
/* 060 */    { ERx("IIputoper"),	gen_all_args,	0,		1 },
/* 061 */    { ERx("IIredisp"),		0,		0,		0 },
/* 062 */    { ERx("IIrescol"),		gen_all_args,	0,		2 },
/* 063 */    { ERx("IIresfld"),		gen_all_args,	0,		1 },
/* 064 */    { ERx("IIresnext"),	0,		0,		0 },
/* 065 */    { ERx("IIgetfldio"),	gen_io,		G_RET,		3 },
/* 066 */    { ERx("IIretval"),		0,		0,		0 },
/* 067 */    { ERx("IIrf_param"),	0,		0,/* unsupp */	0 },
/* 068 */    { ERx("IIrunform"),	0,		0,		0 },
/* 069 */    { ERx("IIrunmu"),		gen_all_args,	0,		1 },
/* 070 */    { ERx("IIputfldio"),	gen_io,		G_SET,		3 },
/* 071 */    { ERx("IIsf_param"),	0,		0,/* unsupp */	0 },
/* 072 */    { ERx("IIsleep"),		gen_all_args,	0,		1 },
/* 073 */    { ERx("IIvalfld"),		gen_all_args,	0,		1 },
/* 074 */    { ERx("IInfrskact"),	gen_all_args,	0,		5 },
/* 075 */    { ERx("IIprnscr"),		gen_all_args,	0,		1 },
/* 076 */    { ERx("IIiqset"),		gen_var_args,	0,		5 },
/* 077 */    { ERx("IIiqfsio"),		gen_io,		G_RET,		5 },
/* 078 */    { ERx("IIstfsio"),		gen_io,		G_SET,		5 },
/* 079 */    { ERx("IIresmu"),		0,		0,		0 },
/* 080 */    { ERx("IIfrshelp"),	gen_all_args,	0,		3 },
/* 081 */    { ERx("IInestmu"),		0,		0,		0 },
/* 082 */    { ERx("IIrunnest"),	0,		0,		0 },
/* 083 */    { ERx("IIendnest"),	0,		0,		0 },
/* 084 */    { ERx("IIFRtoact"),	gen_all_args,	0,		2 },
/* 085 */    { ERx("IIFRitIsTimeout"),	0,		0,		0 },

		      /* Table fields # 86 */
/* 086 */    { ERx("IItbact"),		gen_all_args,	0,		3 },
/* 087 */    { ERx("IItbinit"),		gen_all_args,	0,		3 },
/* 088 */    { ERx("IItbsetio"),	gen_all_args,	0,		4 },
/* 089 */    { ERx("IItbsmode"),	gen_all_args,	0,		1 },
/* 090 */    { ERx("IItclrcol"),	gen_all_args,	0,		1 },
/* 091 */    { ERx("IItclrrow"),	0,		0,		0 },
/* 092 */    { ERx("IItcogetio"),	gen_io,		G_RET,		3 },
/* 093 */    { ERx("IItcoputio"),	gen_io,		G_SET,		3 },
/* 094 */    { ERx("IItcolval"),	gen_all_args,	0,		1 },
/* 095 */    { ERx("IItdata"),		0,		0,		0 },
/* 096 */    { ERx("IItdatend"),	0,		0,		0 },
/* 097 */    { ERx("IItdelrow"),	gen_all_args,	0,		1 },
/* 098 */    { ERx("IItfill"),		0,		0,		0 },
/* 099 */    { ERx("IIthidecol"),	gen_all_args,	0,		2 },
/* 100 */    { ERx("IItinsert"),	0,		0,		0 },
/* 101 */    { ERx("IItrc_param"),	0,		0,/* unsupp */	0 },
/* 102 */    { ERx("IItsc_param"),	0,		0,/* unsupp */	0 },
/* 103 */    { ERx("IItscroll"),	gen_all_args,	0,		2 },
/* 104 */    { ERx("IItunend"),		0,		0,		0 },
/* 105 */    { ERx("IItunload"),	0,		0,		0 },
/* 106 */    { ERx("IItvalrow"),	0,		0,		0 },
/* 107 */    { ERx("IItvalval"),	gen_all_args,	0,		1 },
/* 108 */    { ERx("IITBceColEnd"),     0,              0,              0 },
/* 109 */    { (char *)0,		0,		0,		0 },
/* 110 */    { (char *)0,		0,		0,		0 },

		      /* QA test calls # 111 */
/* 111 */    { ERx("QAmessage"),	gen_var_args,	G_TRIM,		13 },
/* 112 */    { ERx("QAprintf"),		gen_var_args,	G_TRIM,		13 },
/* 113 */    { ERx("QAprompt"),		gen_var_args,	G_TRIM,		13 },
/* 114 */    { (char *)0,		0,		0,		0 },
/* 115 */    { (char *)0,		0,		0,		0 },
/* 116 */    { (char *)0,		0,		0,		0 },
/* 117 */    { (char *)0,		0,		0,		0 },
/* 118 */    { (char *)0,		0,		0,		0 },
/* 119 */    { (char *)0,		0,		0,		0 },

		    /* EQUEL cursor routines # 120 */
/* 120 */    { ERx("IIcsRetrieve"),	gen_all_args,	0,		3 },
/* 121 */    { ERx("IIcsOpen"),		gen_all_args,	0,		3 },
/* 122 */    { ERx("IIcsQuery"),	gen_all_args,	0,		3 },
/* 123 */    { ERx("IIcsGetio"),	gen_io,		G_RET,		2 },
/* 124 */    { ERx("IIcsClose"),	gen_all_args,	0,		3 },
/* 125 */    { ERx("IIcsDelete"),	gen_all_args,	0,		4 },
/* 126 */    { ERx("IIcsReplace"),	gen_all_args,	0,		3 },
/* 127 */    { ERx("IIcsERetrieve"),	0,		0,		0 },
/* 128 */    { ERx("IIcsParGet"),	0,		0,/* unsupp */	2 },
/* 129 */    { ERx("IIcsERplace"),	gen_all_args,	0,		3 },
/* 130 */    { ERx("IIcsRdO"),		gen_all_args,	0,		2 },
/* 131 */    {ERx("IIcsRetScroll"),	gen_all_args,	0,		5 },
/* 132 */    {ERx("IIcsOpenScroll"),	gen_all_args,	0,		4 },

/* 133 */    {(char *)0,		0,		0,		0},
/* 134 */    {(char *)0,		0,		0,		0},

		/* Forms With Clause Routines # 135 */
/* 135 */    {ERx("IIFRgpcontrol"),	gen_all_args,	0,		2},
/* 136 */    {ERx("IIFRgpsetio"),	gen_io,		G_SET,		3},
/* 137 */    {ERx("IIFRreResEntry"),	0,		0,		0},
/* 138 */    {(char *)0,		0,		0,		0},
/* 139 */    {ERx("IIFRsaSetAttrio"),	gen_io,		G_SET,		4},
/* 140 */    {ERx("IIFRaeAlerterEvent"),gen_all_args, 	0,              1},
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
    register	i4	ret_val = 1;

    if (gen__functab(func)->g_func == gen_io)
    {
        gen_io( func );
        out_emit( ERx(";\n") );
    }
    else if (gen__functab(func)->g_func == gen_datio)
    {
        gen_datio( func );
        out_emit( ERx(";\n") );
    }
    else
    {
      /* The while loop is relevant only for 'multi-calls' - unused */
	while (ret_val)
	{
	    ret_val = gen__II( func );
	    out_emit( ERx(";\n") );
	}
    } 
    g_block_dangle = FALSE;
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
** Format: 	(open) 	if (func(args) condition val) then begin
** 	   	(close) end;
**
** The function constant is passed when opening and closing the If statement, 
** just in case some language will not allow If blocks.  This problem already 
** exists on Unix Cobol, where some If blocks are not allowed.  Don't forget
-* to look for the GEN_DML bit in "func" first!
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
	_VOID_ gen__II( func );					/* Function */
	gen__obj( TRUE, ERx(" ") );
	gen__obj( TRUE, gen_condtab[C_RANGE + cond] );		/* Condition */
	gen__obj( TRUE, ERx(" ") );
	gen__int( PbyVAL, val );				/* Value */
	gen__obj( TRUE, ERx(") then\n") );			/* Then */
	gen__obj( TRUE, ERx("begin\n") );			/* Begin */
	gen_do_indent( 1 );
	g_block_dangle = TRUE;

	/* Next call should Close or Else */
    } else if (o_c == G_CLOSE)
    {
	/*
	** if (g_block_dangle)
	**    gen__obj( TRUE, ERx(";\n") );
	*/
	gen_do_indent( -1 );
	gen__obj( TRUE, ERx("end; ") );				/* End */
	gen_comment( TRUE, gen__functab(func)->g_IIname );
	g_block_dangle = FALSE;
    }
}

/* 
+* Procedure:	gen_else 
** Purpose:	Generate an If block with Else clauses.
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
** Format (1): 	(open) 		case (func(args)) of
**	   	(else if)	    val:
**		(else)		    otherwise
** 	   	(close) 	end;
**
** Format (2): 	(open) 		if (func(args)) then
**	   	(else if)	elsif (func(args) condition val) then
**		(else)		else
-* 	   	(close) 	end if;
**
** Notes:
** 1.  The function constant and labels are passed for labelled languages that
**     cannot use If-Then-Else blocks in some cases.
** 2.  Generates Format 1 above, and relies two facts:
**	(1)  This is only called via the grammar using the condition "=", 
**	     allowing us to translate into a "case" statement.
**	(2)  That the G_ELSE followed by a G_OPEN are atomic because the
**	     "dangling when" problem is solved using this information.
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
# ifndef PAS_IF_THEN_ELSE	/* Use Case */

    /* Controls dangling When clause */
    static	bool	g_when_dangle = FALSE;
    static	i4	case_val = 0;

    if (flag == G_OPEN)	  /* Either "main open, or clause for [last] When */
    {
	if (!g_when_dangle)		/* Very first one in current block */
	{
	    gen__obj( TRUE, ERx("case (") );			/* Case */
	    gen__II( func );					/* Function */
	    gen__obj( TRUE, ERx(") of") );			/* Of */
	    out_emit( ERx("\n") );
	    gen_do_indent( 1 );
	    g_when_dangle = TRUE;
	}
	/* Must be a dangling When (either from first G_OPEN or G_ELSE) */
	if (func != II0)		/* Real value put out as "when val" */
	{
	    gen__int( PbyVAL, val );
	    out_emit( ERx(":") );
	    case_val = val;
	} else				/* Last When block */
	{
	    gen__obj( TRUE, ERx("otherwise") );
	    case_val = 0;
	}
	out_emit( ERx(" begin\n") );
	gen_do_indent( 1 );
	g_when_dangle = FALSE;			/* No dangling When, but ... */
	g_block_dangle = TRUE;			/* A block was just opened   */
    } else if (flag == G_ELSE)
    {
	/*
	** if (g_block_dangle)
	**    gen__obj( TRUE, ERx(";\n") );
	*/
	gen_do_indent( -1 );
	gen__obj( TRUE, ERx("end; {case ") );
	if (case_val)
	    gen__int( PbyVAL, case_val );
	else
	    gen__obj( TRUE, ERx("otherwise") );
	gen__obj( TRUE, ERx("}\n\n") );
	g_when_dangle = TRUE;			/* Is a dangling When */
	g_block_dangle = FALSE;			/* Is not an open block */
    } else if (flag == G_CLOSE)
    {
	/*
	** if (g_block_dangle)
	**    gen__obj( TRUE, ERx(";\n") );
	*/
	gen_do_indent( -1 );
	gen__obj( TRUE, ERx("end; {case ") );
	if (case_val)
	    gen__int( PbyVAL, case_val );
	else
	    gen__obj( TRUE, ERx("otherwise") );
	gen__obj( TRUE, ERx("}\n") );
	gen_do_indent( -1 );
	gen__obj( TRUE, ERx("end; {case} ") );
	gen_comment( TRUE, gen__functab(func)->g_IIname );
	g_when_dangle = FALSE;			/* Is not a dangling When */
	g_block_dangle = FALSE;			/* Is not an open block */
	case_val = 0;
    }

# else /* PAS_IF_THEN_ELSE */				/* Use If-Then-Else */

    if (flag == G_OPEN && func == II0)		/* Last Else block */
    {
	gen__obj( TRUE, ERx("begin\n") );
	gen_do_indent( 1 );
	g_block_dangle = TRUE;
    } else if (flag == G_ELSE)
    {
	/*
	** if (g_block_dangle)
	**    gen__obj( TRUE, ERx(";\n") );
	*/
	gen_do_indent( -1 );
	gen__obj( TRUE, ERx("end else ") );	/* Followed by an If or II0 */
    } else
	gen_if( flag, func, cond, val );	/* Either Open or Close */
	/* g_block_dange set in gen_if */
# endif /* PAS_IF_THEN_ELSE */
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
-* Format:	if (func(args) condition val) then goto label;
*/

void
gen_if_goto( func, cond, val, labprefix, labnum )
i4	func;
i4	cond;
i4	val;
i4	labprefix;
i4	labnum;
{
    gen__obj( TRUE, ERx("if (") );			/* If */
    _VOID_ gen__II( func );				/* Function */
    gen__obj( TRUE, ERx(" ") );
    gen__obj( TRUE, gen_condtab[C_RANGE + cond] );	/* Condition */
    gen__obj( TRUE, ERx(" ") );
    gen__int( PbyVAL, val );				/* Value */
    gen__obj( TRUE, ERx(") then ") );			/* Then */
    gen__goto( labprefix, labnum );			/* Goto Label */
    g_block_dangle = FALSE;
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
** Format: 	(open)	while (func(args) condition val) do begin
-*	   	(close)	end do
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
    static i4	g_loop = 0;		 /* Keep track of loops */

    if (o_c == G_OPEN)
    {
	gen__obj( TRUE, ERx("while (") );			/* While */
	_VOID_ gen__II( func );					/* Function */
	gen__obj( TRUE, ERx(" ") );
	gen__obj( TRUE, gen_condtab[C_RANGE + cond] );		/* Condition */
	gen__obj( TRUE, ERx(" ") );
	gen__int( PbyVAL, val );				/* Value */
	gen__obj( TRUE, ERx(") do\n") );			/* Do Begin */
	gen__obj( TRUE, ERx("begin\n") );			/* Do Begin */
	g_loop++;
	gen_do_indent( 1 );
	g_block_dangle = TRUE;
    } else
    {
	if (g_loop > 0)
	{
	    /*
	    ** if (g_block_dangle)
	    **	  gen__obj( TRUE, ERx(";\n") );
	    */
	    gen_do_indent( -1 );
	    gen__obj( TRUE, ERx("end; ") );
	    gen_comment( TRUE, gen__functab(func)->g_IIname );
	    g_loop--;
	}
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
** Format:	[ if (1) then ] goto label;
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
	gen__obj( TRUE, ERx("if (TRUE) then ") );
    gen__goto( labprefix, labnum );
    g_block_dangle = FALSE;
}

/*
+* Procedure:	gen__goto 
** Purpose:	Local statement independent goto generator.  
** Parameters:	labprefix - i4 - Label prefix index.
**		labnum    - i4 - Label number to concatenate.
-* Returns:	None
*/

void
gen__goto( labprefix, labnum )
i4	labprefix;
i4	labnum;
{
    gen__obj( TRUE, ERx("goto ") );				/* Goto */
    _VOID_ gen__label( TRUE, labprefix, labnum );		/* Label */
    out_emit( ERx(";\n") );
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
**       followed by a statement.   Most PASCAL compilers complain about a 
** closing 'END' without a preceding statement.  To avoid warnings we force a
** ';' after these labels.
*/

void
gen_label( loop_extra, labprefix, labnum )
i4	loop_extra;
i4	labprefix;
i4	labnum;
{
    char	*lb;

  /* Labels must always begin in the left margin */
    lb = gen__label( FALSE, labprefix, labnum );
    if (labprefix==L_RETEND || labprefix==L_FDEND || labprefix==L_FDINIT)
	out_emit( ERx(":;\n") );		/* No following statement */
    else
	out_emit( ERx(":\n") );
    pasLbLabel( lb );
}

/*
+* Procedure:	gen__label 
** Purpose:	Local statement independent label generator.  
**
** Parameters:	ind	  - i4 - Indentation flag,
**		labprefix - i4 - Label prefix,
**		labnum    - i4 - Label number.
-* Returns:	The generated label name.
*/

char *
gen__label( ind, labprefix, labnum )
i4	ind;
i4	labprefix;
i4	labnum;
{
    static char	buf[30] ZERO_FILL;

  /* Label prefixes and their numbers must be atomic */
    STprintf( buf, gen_ltable[labprefix] );
    CVna( labnum, &buf[(i4)STlength(buf)] );
    gen__obj( ind, buf );
    return buf;
}

/*
+* Procedure:	gen_comment 
** Purpose:	Make a comment with the passed string. Should be used
**		by higher level calls, but may be used locally.
**
** Parameters: 	term - i4	- Comment terminated with newline,
**		str  - char * 	- Comment string.
-* Returns: 	None
*/

void
gen_comment( term, str )
i4	term;
char	*str;
{
    gen__obj( TRUE, ERx("{") );
    gen__obj( TRUE, str );
    gen__obj( TRUE, ERx("}") );
    if (term)
	out_emit( ERx("\n") );
}

/*
+* Procedure:	gen_include 
** Purpose:	Generate an include file statement.
**
** Parameters: 	pre	- char	* - Prefix,
**		suff	- char  * - and suffix separated by a '.',
**		extra	- char  * - Any extra qualifiers etc.
-* Returns: 	None
*/

void
gen_include( pre, suff, extra )
char	*pre;
char	*suff;
char	*extra;
{
    /* The whole include line should be atomic */
    out_emit( ERx("%include '") );
    out_emit( pre );
    if (suff != (char *)0)
    {
	out_emit( ERx(".") );
	out_emit( suff );
    }
    if (extra != (char *)0)
	out_emit( extra );
    out_emit( ERx("'\n") );
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
** (C or Pascal), add a comment about the line number and file, or generate
** a preprocessor directive about the same (C).
*/

void
gen_eqstmt( o_c, cmnt )
i4	o_c;
char	*cmnt;
{
    if (o_c == G_OPEN)
    {
	gen_line( cmnt );
	gen__obj( TRUE, ERx("begin\n") );		/* Begin */
	gen_do_indent( 1 );
    } else
    {
	gen_do_indent( -1 );
	gen__obj( TRUE, ERx("end; ") );			/* End */
	if (cmnt)
	    gen_comment( FALSE, cmnt );
	out_emit( ERx("\n") );
    }
}

/*
+* Procedure:	gen_line 
** Purpose:	Generate a line comment or directive.
**
** Parameters:	s - char * - Optional comment string.
-* Returns:	None
*/

void
gen_line( s )
char	*s;
{
    char	fbuf[G_MAXSCONST +20]; 

    gen__obj( FALSE, ERx("{") );
    STprintf( fbuf, ERx("File \"%s%s%s\", Line %d"), eq->eq_filename,
	eq->eq_infile == stdin ? ERx("") : ERx("."), eq->eq_ext, eq->eq_line );
    if (s)
    {
	STcat( fbuf, ERx("\t-- ") );
	STcat( fbuf, s );
    }
    gen__obj( FALSE, fbuf );
    out_emit( ERx("}\n") );
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
**     VMS - continue.			Unix - move IIcharX to IIcharX.
*/

void
gen_term()
{
}

/*
** gen_declare - Process a ## declare statement.
**
** Parameters:  is_env - char * - iff we should gen an INHERIT attribute.
** Returns:	None
**
** Generates:	%include 'ingres_path:eqdef.pas'
**			or
**		[inherit('ingres_path:eqenv.pen')]
*/

void
gen_declare( is_env )
char 	*is_env;
{
    LOCATION	loc; 
    char	buf[MAX_LOC +1];
    char	*lname;
    i4		len;

    if (is_env != (char *)0)
    {
	/* Spit out an inherit attribute */
	STcopy( ERx("eqenv.pen"), buf );
	NMloc( FILES, FILENAME, buf, &loc );
	LOtos( &loc, &lname );
	CVlower( lname );		/* In a global buffer */
	out_emit( ERx("[inherit('") );
	out_emit( lname );
	if (is_env && *is_env)		/* Closing string - may be other envs */
	    out_emit(is_env);
	else
	    out_emit( ERx("')]") );
    } else
    {
	/* Spit out real Include line */
	STcopy( ERx("eqdef.pas"), buf );
	NMloc( FILES, FILENAME, buf, &loc );
	LOtos( &loc, &lname );
	CVlower( lname );		/* In a global buffer */
	gen_include( lname, (char *)0, ERx("/nolist") );
    }

  /* Now for some trickery ! */
    NMgtAt( ERx("II_PAS_WHO"), &lname );
    if (lname != (char *)0 && *lname != '\0')
    {
	SIprintf( ERx("## EQUEL PASCAL\n") );
	SIprintf(
	    ERx("##\tWritten by Mark R. Wittenberg (RTI), December 1985.\n") );
	SIflush( stdout );
    }
}

/*
+* Procedure:	gen_makesconst 
** Purpose:	Make a host language string constant.
**
** Parameters:	flag  - i4     - G_INSTRING  - Nested in another string (DB 
**					       string),
**		        	 G_REGSTRING - String sent by itself in a call.
**		instr - char * - Incoming string.
**		diff  - i4 *   - Mark if there were changes made to string.
-* Returns:	char *         - Pointer to static converted string.
**
** Notes:
** 1. Special Characters.
** 1.1. Process the '\' character, which is normally used to go to the 
**      database, but in C may be used to escape certain sequences.
** 1.2. Process the host language quote embedded in a string, whether it got
**      there by escaping it with a '\' or by doubling it.
*/

char	*
gen_makesconst( flag, instr, diff )
i4	flag;
char	*instr;
i4	*diff;
{
    static	char	obuf[ G_MAXSCONST + 4 ] ZERO_FILL;/* Converted string */
    register	char	*icp;				/* To input */
    register	char	*ocp = obuf;			/* To output */
    register	i4	esc = 0;			/* '\' was used */

    *diff = 0;

    if (flag == G_INSTRING)		/* Escape the first quote */
    {
	*ocp++ = '"';
	*diff = 1;
    }
    for (icp = instr; *icp && (ocp < &obuf[G_MAXSCONST]);)
    {
	if (*icp == '\\')		/* Leave '\' may be DB or IIPRINTF */
	{	
	    icp++;
	    *ocp++ = '\\';
	    esc = 1;
	}
	else if (*icp == '"')
	{
	    icp++;
	    if (flag == G_INSTRING && !esc)	/* DB needs escape */
	    {
		*ocp++ = '\\';
		*diff = 1;
	    }
	    *ocp++ = '"';
	    esc = 0;
	}
	else if (*icp == '\'')		/* Need to escape (double) it */
	{
	    /*
	    ** Either entered via escaping it:  \'
	    **		          doubling it:  ''
	    ** or using a different delimiter:  "
	    ** In all cases add two, and if was originally doubled then knock 
	    ** out the following double.
	    */
	    icp++;
	    *ocp++ = '\'';
	    *ocp++ = '\'';
	    if (*icp == '\'')		/* Doubled but no change */
		icp++;
	    else
		*diff = 1;
	    esc = 0;
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
	*diff = 1;
    }
    *ocp = '\0';
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
** Returns: 	0 / 1 - As passed back from lower level calls.
**		1 means "call me again" -- currently unused.
**
** Notes:
**	This routine assumes language dependency, and knowledge of the actual 
**	arguments required by the call.
**
**	If the GEN_DML bit is on in "func", then vector out to the alternate
-*	gen table.  It is a fatal error if there is none.
*/

i4
gen__II( func )
i4	func;
{
    register GEN_IIFUNC		*g;
    i4				ret_val;

    g = gen__functab( func );
    gen__obj( TRUE, g->g_IIname );
    ret_val = 0;
    if (g->g_func)
    {
	out_emit( ERx("(") );
	ret_val = (*g->g_func)( g->g_numargs, g->g_flags );
	out_emit( ERx(")") );
    }
    arg_free();
    return ret_val;
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
gen_all_args( numargs, flags )
i4	numargs;
i4	flags;		/* Unused */
{
    register	i4 i;

    for (i = 1; i <= numargs; i++)	/* Fixed number of arguments */
    {
	gen_data( i, flags );
	if (i < numargs)
	    gen__obj( TRUE, ERx(",") );
    }
    return 0;
}

/*
+* Procedure:	gen_var_args 
** Purpose:	Format varying number of arguments stored by the argument 
**		manager.
**
** Parameters: 	flags   - i4 - Flags and Number of arguments required.
**		numargs - i4 
** Returns: 	0
**
** As gen_all_args() this routine should be used when no validation of 
-* the number of arguments needs to be done.
*/

i4
gen_var_args( numargs, flags )
i4	numargs, flags;			/* Unused */
{
    register	i4 i;

    while (i = arg_next())		/* Varying num of arguments */
    {
	gen_data( i, flags );
	gen__obj( TRUE, ERx(",") );
    }
    gen__int( PbyVAL, 0 );		/* Null terminate */
    return 0;
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
    gen__int( PbyVAL, num );
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
    		if (STcompare(buf,ERx("0")) != 0)
		{
		    gen__obj( TRUE, ERx("0") );
    		    er_write(E_EQ0090_HDL_TYPE_INVALID, EQ_ERROR, 1, buf);
    		    break;
		}
        case  ARG_RAW:
		gen__obj(TRUE,ERx(" %immed "));
    		gen__obj( TRUE, buf);
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
		gen__obj( TRUE, ERx("0") );
	else
	{
		gen__obj(TRUE,ERx(" %immed "));
		gen__obj( TRUE, argbuf);
	}
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
gen_datio( func, flags )
i4     func;
i4     flags;
{
    register GEN_IIFUNC         *g;             /* Current I/O function */
    SYM                         *sy;           /* Symtab info */
    register i4                 ar;          	/* What is the I/O argument */
    i4                          artype;         /* Type of I/O argument */
    i4                          i4num;          /* Numerics stored as strings */
    i4				type;		/* T_ type */
    i4				len;		/* I/O var length */
    i4				attrs;		/* mask of which attributes */
    f8                          f8num;
    char                        datbuf[ 50 ];   /* To pass to gen_iodat */
    char                        *data = datbuf;
    char                        *strdat;        /* String rep */
    char                        *exp_cp;        /* Ptr to exponent char */

    g = gen__functab(func);
    arg_num(g->g_numargs, ARG_INT, &attrs);

    ar = 1;
    strdat = (char *)0;

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
        if (strdat)
            data = strdat;

        switch (artype)
        {
            case ARG_INT:
                if (!strdat)                /* Convert numeric to string */
                {
                    arg_num( ar, ARG_INT, &i4num );
                    CVna( i4num, data );
                }
	    	type = T_INT;
	    	len = sizeof(i4);
	    	break;

	    case ARG_FLOAT:
            	if (!strdat)
            	{
               	    arg_num( ar, ARG_FLOAT, &f8num );
               	    STprintf( data, ERx("%f"), f8num, '.' );
            	}
		/*
            	** VMS PASCAL plain floating constants are REAL*4,
            	** override this and pass float constants as DOUBLE*8.
            	** Move constant to a temporary DOUBLE variable
            	** then pass variable by reference, for example.:
            	**
            	**  IIf8 := 1.23D10;
            	**  IIputfldio(%ref 'fnum'(0),0,1,31,8,%ref IIf8);
            	**
            	** Note: "E" in VMS PASCAL indicates a single precision
            	** real number while "D' indicates a double precision
            	** number, replace "E" with "D" before generating assignment
            	** statement.
            	*/
            	CVupper( data );
            	if ((exp_cp = STindex(data, ERx("E"), 0)) != NULL )
                	*exp_cp = 'D';
            	gen_asn( data, ERx("IIf8") );
		type = T_FLOAT;
		len = sizeof(f8);
		break;

	    case ARG_PACK:
		type = T_PACKASCHAR;
		len = 0;
 		break;

	    case ARG_CHAR:
		type = T_CHAR;
		len = 0;
		break;
	}
    }	
    if (type == T_CHAR) 
    {
	if (sy != (SYM *)0)
	    len = 0;
	gen__obj( TRUE, ERx("IIx") );
	out_emit( g->g_IIname+2 );
    }
    else
	gen__obj( TRUE, g->g_IIname );
    out_emit( ERx("(") );
    gen__int(PbyVAL, attrs);
    gen__obj( TRUE,  ERx(",") );
    gen__int( PbyVAL, type == T_NUL ? -type : type );
    gen__obj( TRUE,  ERx(",") );
    gen__int( PbyVAL, len );
    gen__obj( TRUE,  ERx(",") );

    /* Dump I/O argument based on type */
    switch (type)
    {
 	case T_INT:
	    if (sy != (SYM *)0)
	    {
	        if (len != sizeof(i4))
   		    gen__name( PbyREF, data );
	        else
    		    gen__name( PbyNAME, data );
	    }
            else
                gen__name( PbyVAL, data );
            break;

    	case T_FLOAT:
            /*
            ** F4 and f8 vars have a By Ref mechanism on input, but only f8 has
            ** this mechanism on output.  Float constants are by val (as f4's).
            */
            if (sy != (SYM *)0)
            {
                if (g->g_flags == G_SET || len == sizeof(f8))
                    gen__name( PbyREF, data );
                else
                    gen__name( PbyNAME, data );
            }
            else
                gen__name( PbyVAL, data );
            break;

	case T_NUL:
	case T_NONE:
            gen__name( PbyVAL, ERx("0") );
            break;

	default: /* T_CHAR or T_PACKASCHAR */
	    if (sy != (SYM *)0)
		gen_sdesc( PsdNONE, (i4)sym_g_vlue(sy), data );		
	    else
	        gen_sdesc( PsdCONST, 0, data );
	    break;

    }

    /* The remaining arguments are all integer */
    for (ar = 2; ar < g->g_numargs; ar++) 
    {
        gen__obj( TRUE, ERx(",") );
	if (!arg_name(ar))
	    gen__obj( TRUE, ERx("0") );
	else if ((flags == G_RET) && (ar != 2))
        {
	    if (((sy = arg_sym(ar)) != (SYM *)0) &&
	       (sym_g_dsize(sy) != sizeof(i4)))
	        gen__name(PbyREF, arg_name(ar));
	    else
		gen__name( PbyNAME, arg_name(ar) );
	}
	else
	    gen__name( PbyVAL, arg_name(ar) );

    }
    out_emit( ERx(")") );
    arg_free();
    return 0;
}

/*
** gen_io - Generate an I/O variable to the output file.
**
** Parameters: 	func - Function number.
** Returns: 	0
**
** Note:
**  1.  There will always be a minimum of two arguments: the null indicator
** 	and the actual argument.  If there are just 2 arguments then we rely
** 	on the following template format:
**		IIretfunc( indaddr, var_desc );
**		IIsetfunc( indaddr, var_or_val_desc );
**      If there are 2 arguments then we rely on the following template 
**      format:
**		IIretfunc( indaddr, var_desc, "object" );
**		IIsetfunc( "object", indaddr, var_or_val_desc );
**	where var_desc or var_or_val_desc is:
**		ISVAR, TYPE, LEN, var_or_val
**      We figure out which one is the null indicator, the data argument
**	which must be described and the others we just dump.
*/

i4
gen_io( func, flags )
i4 	func;
i4 	flags;
{
    register GEN_IIFUNC		*g;		/* Current I/O function */
    SYM				*var;		/* Symtab info */
    register i4 		ioarg;		/* What is the I/O argument */
    i4 				artype;		/* Type of I/O argument */
    i4 				i4num;		/* Numerics stored as strings */
    f8				f8num;
    char			datbuf[ 50 ];	/* To pass to gen_iodat */
    char			*data = datbuf;
    char			*strdat;	/* String rep */
    char			*exp_cp;	/* Ptr to exponent char */

    g = gen__functab(func);
    /* 
    ** Figure out which is I/O argument based on number of args and mode:
    ** set (name = value) or ret (var = name).  G_RETSET is like G_SET in
    ** the case of "where the ioarg is".
    */
    ioarg = (g->g_flags == G_RET) ? 2 : g->g_numargs;
    if ((var=arg_sym(ioarg)) == (SYM *)0)    /* Constant used - get real data */
    {
	if (g->g_flags == G_RET || g->g_flags == G_RETSET)	/* User error */
	{
	    arg_free();
	    return 0;
	}
	strdat = (char *)0;
	if ((artype = arg_rep(ioarg)) == ARG_CHAR)
	{
	    artype = arg_type( ioarg );		/* Real user type */
	    strdat = arg_name( ioarg );		/* String numeric or string */
	}
	switch (artype)
	{
	  case ARG_INT:
	    if (strdat)
		data = strdat;
	    else		/* Convert numeric to string */
	    {
		arg_num( ioarg, ARG_INT, &i4num );
		CVna( i4num, data );
	    }
	    /* Send i4 by value */
	    gen_iodat( func, (SYM *)0, T_INT, sizeof(i4), data );
	    break;
	  case ARG_FLOAT:
	    if (strdat)
		data = strdat;
	    else		/* Convert float to string */
	    {
		arg_num( ioarg, ARG_FLOAT, &f8num );
		STprintf( data, ERx("%f"), f8num, '.' );
	    }
	    /*
	    ** VMS PASCAL plain floating constants are REAL*4,
	    ** override this and pass float constants as DOUBLE*8.
	    ** Move constant to a temporary DOUBLE variable
	    ** then pass variable by reference, for example.:
	    **
	    **	IIf8 := 1.23D10;
	    **	IIputfldio(%ref 'fnum'(0),0,1,31,8,%ref IIf8);
	    **
	    ** Note: "E" in VMS PASCAL indicates a single precision
	    ** real number while "D' indicates a double precision
	    ** number, replace "E" with "D" before generating assignment
	    ** statement.
	    */
	    CVupper( data );
	    if ((exp_cp = STindex(data, ERx("E"), 0)) != NULL )
		*exp_cp = 'D';
	    gen_asn( data, ERx("IIf8") );
	    gen_iodat( func, TRUE, T_FLOAT, sizeof(f8), ERx("IIf8") );
	    break;
	  case ARG_PACK:
	    gen_iodat( func, (SYM *)0, T_PACKASCHAR, 0, strdat );
	    break;
	  case ARG_CHAR:
	  default:
	    gen_iodat( func, (SYM *)0, T_CHAR, 0, strdat );
	    break;
	}
	arg_free();
	return 0;
    } /* if constant */

  /* Is a variable */
    data = arg_name( ioarg );			/* Name of variable */
    artype = sym_g_btype( var );

    switch (artype)
    {
      case T_INT:
	if (g->g_flags == G_SET)
	{	
	    /* Can always pass an i4 by value - default for %immed is 4 bytes */
	    gen_iodat( func, (SYM *)0, T_INT, sizeof(i4), data );
	}
	else /* G_RET or G_RETSET */
	    gen_iodat( func, var, T_INT, sym_g_dsize(var), data );
	break;
      case T_NUL:
	gen_iodat( func, (SYM *)0, T_NUL, 1, 0 );
	break;
      case T_FLOAT:
      case T_CHAR:
      default:		/* default = error - leave as is and use its name */
	gen_iodat( func, var, artype, sym_g_dsize(var), data );
	break;
    }

    arg_free();
    return 0;
}


/*
** gen_iodat - Make real I/O call.
**
** Parameters: 	func 	- i4	- Function index.
*		varptr	- SYM*	- Is a variable.
**		type, len - i4	- Data description.
**		data	- char *- Name of data arg (value or varname).
** Returns: 	None
**
** Note:	See gen_io comment for what we need to generate.  There is
**		a special case when using strings.  If a 'setting' function
**	then we just make a function call to IIsd via gen__sdesc.  If a 
**	'returning' function we actually change the name of the call from
**	'IIretfunc' to 'IIxretfunc' and pass the real VMS string descriptor.
**	Example:
**		IIsetfunc( "strobj", indaddr, ISVAR, T_CHAR, 0, IIsd(strvar) );
**	or:
**		IIxretfunc(indadr,ISVAR, T_CHAR, 0, %[st]descr strvar,"strobj");
*/

gen_iodat( func, varptr, type, len, data )
i4		func;
SYM		*varptr;
register i4	type; 
i4		len;
register char	*data;
{
    register GEN_IIFUNC		*g;		/* Current I/O function */
    i4				ar;

    g = gen__functab(func);

    if (type == T_CHAR)
	len = 0;    /* VMS descriptor has the real size */
  /* Get func name out - special case setting and retrieving strings */
    if (type == T_CHAR && (varptr || func == IIWRITEDB))
    {
	gen__obj( TRUE, ERx("IIx") );
	out_emit( g->g_IIname+2 );
    } else
	gen__obj( TRUE, g->g_IIname );
    out_emit( ERx("(") );

  /* Do we need a name or other data before I/O argument */
    if (g->g_flags == G_SET || g->g_flags == G_RETSET)	/* (name = val) */
    {
	for (ar = 1; ar <= g->g_numargs-2; ar++)
	{
	    gen_data( ar, g->g_flags );
	    gen__obj( TRUE, ERx(",") );
	}
	gen_null(g->g_numargs - 1);
    }
    else
	gen_null(1);

  /* Dump data information */

    /* is by val or by ref? */
    gen__int( PbyVAL, varptr || type == T_CHAR || type == T_PACKASCHAR );
    gen__obj( TRUE,  ERx(",") );
    gen__int( PbyVAL, type == T_NUL ? -type : type );
    gen__obj( TRUE,  ERx(",") );
    gen__int( PbyVAL, len );
    gen__obj( TRUE,  ERx(",") );

  /* Dump I/O argument based on type */
    switch (type)
    {
      case T_INT:
	if (varptr)
	{
	    /*
	    ** The routines are declared to take "%ref val : [unsafe] integer"
	    ** which requires a 4-byte object when called as "foo(name)".
	    ** To pass a different-sized object requires "foo(%ref name)".
	    ** Funny, eh?
	    */
	    if (len != sizeof(i4))
		gen__name( PbyREF, data );
	    else
		gen__name( PbyNAME, data );
	} else
	    gen__name( PbyVAL, data );
	break;
      case T_FLOAT:
	/* 
	** F4 and f8 vars have a By Ref mechanism on input, but only f8 has
	** this mechanism on output.  Float constants are by val (as f4's).
	*/
	if (varptr)
	{
	    if (g->g_flags == G_SET || len == sizeof(f8))
		gen__name( PbyREF, data );
	    else
		gen__name( PbyNAME, data );
	}
	else
	    gen__name( PbyVAL, data );
	break;
      case T_NUL:
	gen__name( PbyVAL, ERx("0") );
	break;
      default:	/* T_CHAR or T_PACKASCHAR */
	if (varptr)
	    gen_sdesc( PsdNONE, (i4)sym_g_vlue(varptr), data );
	else if (func == IIWRITEDB)			/* Literal to IIwrite */
	    gen_IIwrite(data);
	else
	    gen_sdesc( PsdCONST, 0, data );		/* Probably G_SET */
	break;
    }
  /* Do we need a name or other data after I/O argument */
    if (g->g_flags == G_RET)			/* (var = name ) */
    {
	for (ar = 3; ar <= g->g_numargs; ar++)
	{
	    gen__obj( TRUE, ERx(",") );
	    gen_data( ar, g->g_flags );
	}
    }
    out_emit( ERx(")") );
}

/*
+* Procedure:	gen_data 
** Purpose:	Whatever is stored by the argument manager for a particular 
**	      	argumment, dump to the output file.  
**
** Parameters: 	arg	- i4 - Argument number.
**		flags	- G_TRIM, G_RET, G_SET or G_RETSET
-* Returns: 	None
**
** This routine should be used when no validation of the data of the argument 
** needs to be done. Hopefully, if there was an error before (or while) 
** storing the argument this routine will not be called.
**
** Never trim string constants -- presumably, the user wanted the blanks,
** and besides, we don't provide code for passing string constants to IIsd.
*/

void
gen_data( arg, flags )
register i4	arg;
i4		flags;
{
    i4		num;
    f8		flt;
    char	*s;
    SYM		*var;

    if (var = arg_sym(arg))		/* User variable */
    {
	if (sym_g_btype(var) == T_CHAR)
	{
	    i4		mode;

	    /* mode = (flags==G_TRIM ? PsdREG : PsdNOTRIM); */
	    mode = PsdREG;	/* always trim -- it never hurts */
	    gen_sdesc( mode, (i4)sym_g_vlue(var), arg_name(arg) );
	} else
	    gen__name( PbyNAME, arg_name(arg) );
	return;
    }
    /* Based on the data representation, dump the argument */
    switch (arg_rep(arg))
    {
      case ARG_INT:  
	arg_num( arg, ARG_INT, &num );
	gen__int( PbyVAL, num );
	break;
      case ARG_FLOAT:  
	arg_num( arg, ARG_FLOAT, &flt );
	gen__float( PbyVAL, flt );
	break;
      case ARG_CHAR:  
	switch (arg_type(arg))
	{
	  case ARG_PACK:	/* Decimal constant - ??? is this ever used*/
    	  case ARG_CHAR: 	/* String constant */
	    if (s = arg_name(arg))
		gen_sdesc( PsdCONST, 0, s );
	    else
		gen__int( PbyVAL, 0 );
	    break;
	  case ARG_RAW:		/* Raw data stored - pass by name */
	  default:		/* String of user numeric - pass by name */
	    gen__name( PbyNAME, arg_name(arg) );
	    break;
	}
	break;
    }
}

/*
+* Procedure:	gen_sdesc 
** Purpose:	Put out PASCAL string as a descriptor [inside an IIsd
**		or IIsd_no call] or a string constant. The call converts
**		PASCAL string to C string.
** Parameters:	sdfunc 	- i4	- PsdCONST: String const,	%ref "x"
**				  PsdNONE:  Var but no sd func,	%descr x
**				  PsdREG:   Var and via IIsd,	IIsd(%descr x)
**				  PsdNOTRIM:Var, via IIsd_no.	IIsd_no(%desc x)
**		vary 	- i4	- Is this a varying string.
**		data	- char* - String to dump.
-* Returns:	None
** Notes:	Never concatenate nulls onto variables (even in NOTRIM), as
**		this creates a varying string on the fly.  What gets passed
**		(if by %ref) is the address of the count field:
**			type VaryStr = record
**				length : [word] 0..ub;
**				body :   packed array[1..ub] of char;
**			end;
**			...
**			xx( %ref x+chr(0) );	{address of x.length is passed}
**		and you can't convince the compiler to do otherwise
**		(in C we could generate
**			xx( &(strcat(x,"\0")).body );
**		but not in PASCAL!).
*/

void
gen_sdesc( sdfunc, vary, data )
i4	sdfunc, vary;
char	*data;
{
    static char	*sd[] =		{ ERx(""), ERx("IIsd("), ERx("IIsd_no(") };
    static char	*varymech[] =	{ ERx("%stdescr "), ERx("%descr ") };
#ifdef	MVS
    static char	*offset[] =	{ ERx(""), ERx(")+2"), ERx(")+4") };
    i4		index;
#endif

    if (sdfunc == PsdCONST)
    {
	gen__obj( TRUE, ERx("%ref ") );
	gen_sconst( data );
	out_emit( ERx("(0)") );		/* Null terminate */
	return;
    }

#ifdef	MVS
    index = 0;
    if (vary & PvalVARYING)
	index = 1;
    if (vary & PvalSTRPTR)
	index = 2;
    if (index)
	gen__obj( TRUE, ERx("addr(") );
    gen__obj( TRUE, data );
    gen__obj( TRUE, offset[index] );
#else
    vary &= PvalVARYING;
    gen__obj( TRUE, sd[sdfunc] );
    gen__obj( TRUE, varymech[vary ? 1 : 0] );
    gen__obj( TRUE, data );
    if (sdfunc != PsdNONE)
	gen__obj( TRUE, ERx(")") );
#endif
}

/*
+* Procedure:	gen_asn 
** Purpose:	Put out PASCAL assignment statement, source to destination
**		e.g., "dst := src;".
** Parameters:	src 	- char*	- source string
**		dst	- char* - destination string
-* Returns:	None
*/

void
gen_asn( src, dst )
char    *src, *dst;
{
    gen__obj( TRUE, dst );
    gen__obj( TRUE, ERx(" := ") );
    gen__obj( TRUE, src );
    gen__obj( TRUE, ERx(";\n") );
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
	gen_sdesc( PsdCONST, 0, fbuf );
	gen__obj( TRUE, ERx(",") );
	gen__int( PbyVAL, eq->eq_line );
    } else
    {
	gen__int( PbyVAL, 0 );
	gen__obj( TRUE, ERx(",") );
	gen__int( PbyVAL, 0 );
    }
    return 0;
}

/*{
+*  Name:  gen_null - Generate null indicator arg to IIput/IIget routines
**
**  Description:
**	If null indicator present, pass by reference.
**	Otherwise, pass a null pointer.
**
**  Inputs:
**	argnum		- Argument number
**
**  Outputs:
**	None
**
**  Side Effects:
**	Generates an argument into preprocessor output file for an I/O call.
-*	
**  Notes:
**	Indicator arguments do not need to be passed with a type/length 
**	descriptor since they are always expected to be pointers to two-
**	byte integers (or NULL)
**
**  History:
**	18-feb-1987 - written (bjb)
*/

void
gen_null( argnum )
i4	argnum;
{
    if ((arg_sym(argnum)) == (SYM *)0)		/* No indicator variable? */
    {
	gen__obj( TRUE, ERx("0,") );			/* NULL pointer */
    }
    else
    {
	gen__name(PbyREF, arg_name(argnum));	/* By reference */
	gen__obj( TRUE, ERx(",") );
    }
}

/*
+* Procedure:	gen_sconst 
** Purpose:	Generate a string constant.
**
** Parameters: 	str - char * - String
-* Returns: 	None
**
** Attempt to fit it on one line (hopefully current line) and spit it all out.
** Don't try to mess with PASCAL '+' string operator, as this creates varying
** strings on the fly.
*/

void
gen_sconst( str )
char	*str;
{
    if (str == (char *)0)
	return;
    /* 
    ** The +2 is for two quotes and the +3 is for the "(0)" null terminator 
    ** that may follow.
    */
    if ((i4)STlength(str)+5 > G_LINELEN - out_cur->o_charcnt)
	out_emit( ERx("\n") );
    /* May have been just reset or may have been a previous newline */
    if (out_cur->o_charcnt == 0)
	out_emit( g_indent );
    out_emit( ERx("'") );
    out_emit( str );
    out_emit( ERx("'") );
}

/* 
**  Name: gen_IIwrite - Generate string literal for IIwrite.
**
**  Description:
**	PASCAL does not allow lines of > 255, and is the only language that
**	does not allow the continuation of string literals.  The only way to
**	do this is to use the concat operator (+), but this has the side
**	effect of modifying the internal description of the string literal
**	from "regular string" (which uses "string descriptor" - %stdescr)
**	to "varying string" (which uses "object's descriptor" - %descr).
**	Consequently, we figure out if we have to split the string first.
**	If we do not split the string then we use the %stdescr mechanism 
**	and write 1 string.  If we do split the string then we use the
**	%descr mechanism and write concatenated strings.
**		
** 	Note that we should not split lines in the middle of compiler escape
**      sequences (the single quote).
**
**  Inputs:
**	str	- String to generate
**
**  Outputs:
**	Returns:
**	    None
**	Errors:
**
**  Side Effects:
**	
**  History:
**	04-dec-1987	- Written (ncg)
*/

void
gen_IIwrite(str)
register char	*str;
{
    char	*cp;
    i4		len;
    char	savech;
# define	MECH_LEN	9	/* Length of "%[st]descr " */

    len = (i4)STlength(str);
    /* Can string fit on current line ? */
    if (len <= G_LINELEN - out_cur->o_charcnt - MECH_LEN)
    {
	out_emit(ERx("%stdescr '"));
	out_emit(str);
	out_emit(ERx("'"));
	return;
    }
    /* Can string fit on next line by itself ? */
    if (len <= G_LINELEN - (i4)STlength(g_indent))
    {
	out_emit(ERx("%stdescr\n"));
	gen__obj(TRUE, ERx("'"));
	out_emit(str);
	out_emit(ERx("'"));
	return;
    }

    /* String needs to be broken across lines */
    out_emit(ERx("%descr"));
    do
    {
	out_emit(ERx("\n"));
	gen__obj(TRUE, ERx("'"));
	len = G_LINELEN - out_cur->o_charcnt;
	for (cp = str; len > 0 && *cp;)
	{
	    /* Try not to break in the middle of compiler escape sequences */
	    if (*cp == '\'')
	    {
		cp++; 
		len--;				/* Skip the escaped ' */
	    }
	    CMbytedec(len,cp);
	    CMnext(cp);				/* Skip current character */
	}
	savech = *cp;
	*cp = '\0';
	out_emit(str);
	*cp = savech;
	str = cp;				/* More to go next time */
	out_emit(ERx("'"));
	if (*cp)
	    out_emit(ERx("+"));
    } while (*cp);
} /* gen_IIwrite */

/*
+* Procedure:	gen__name
** Purpose:	Spit out a name.
** Parameters:
**	how	- i4		- PbyNAME, PbyVAL, or PbyREF
**	s	- char *	- The name to print
** Return Values:
-*	None.
** Notes:
**	None.
*/

void
gen__name( how, s )
i4	how;
char	*s;
{
    switch (how)
    {
      case PbyVAL:
      case PbyNAME:
	gen__obj( TRUE, s );
	break;
      case PbyREF:
	gen__obj( TRUE, ERx("%ref ") );
	gen__obj( TRUE, s );
	break;
    }
}

/*
+* Procedure:	gen__int 
** Purpose:	Does local integer conversion.
**
** Parameters: 	num - i4 - Integer to put out.
**		how - i4 - Style of output.
-* Returns: 	None
*/

void
gen__int( how, num )
i4	how;
i4	num;
{
    char	buf[20];

    /* All ints are by value based on the eqdef.pas definition */
    CVna( num, buf );
    gen__obj( TRUE, buf );
}

/*
+* Procedure:	gen__float 
** Purpose:	Does local floating conversion.
**
** Parameters: 	num - f8 - Floating to put out.
**		how - i4 - Style of output.
-* Returns: 	None
*/

void
gen__float( how, num )
i4	how;
f8	num;
{
    char	buf[50];

    STprintf( buf, ERx("%f"), num, '.' );	/* Standard notation */
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
    /* May have been just reset or may have been a previous newline */
    if (out_cur->o_charcnt == 0 && ind)
	out_emit( g_indent );
		/* The 3 is for unused space once the object is reached */
    else if ((i4)STlength(obj) > G_LINELEN - out_cur->o_charcnt -3)
    {
	out_emit( ERx("\n") );
	out_emit( g_indent );
	out_emit( ERx("  ") );		/* 2 spaces */
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
	else
	    er_write( E_EQ0002_eqNOTABLE, EQ_FATAL, 0 ); /* Internal Error */
    } else
	return &gen__ftab[func];
  /* NOTREACHED */
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
i4		i;
{
    register	i4	n_ind;
    register	i4	o_ind;

    /* 
    ** Allow global indent level to exceed limits so that popping back out is 
    ** done correctly, but make sure no blanks or nulls are put in random space.
    */
    o_ind = g_indlevel;
    if (i == G_IND_RESET)		/* reset to left margin */
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
    static	char	which_op = 0;		/* Last operator */

    f_loc = flag & ~(G_H_INDENT | G_H_OUTDENT | G_H_NEWLINE);

    if (flag & G_H_OUTDENT)			/* Outdent before data */
	gen_do_indent( -1 );

    switch (f_loc)
    {
      case G_H_KEY:
	if (was_key)
	{
	    if (*s && (!CMoper(s) || (STcompare(s,ERx(":="))==0))
	      && out_cur->o_charcnt != 0)
		out_emit( ERx(" ") );
	} else
	{
	    if (out_cur->o_charcnt)
	    {
		switch (which_op)
		{
		  case ';':
		  case ']':
		  case ')':
		  case ':':
		  case '=':
		    out_emit( ERx(" ") );
		    break;
		}
	    }
	}
	gen__obj( TRUE, s );
	was_key = 1;
	which_op = 0;
	break;

      case G_H_OP:
	if (was_key)
	{
	    if (*s && ((*s == '=') || (*s == '^') || !CMoper(s)))
		out_emit( ERx(" ") );
	} else if (*s && ((which_op == ':') || (which_op == '=')))
	{
	    out_emit( ERx(" ") );
	}
	gen__obj( TRUE, s );
	was_key = 0;
	which_op = *s;
	break;

      case G_H_SCONST:
	if (was_key || which_op=='=')
	    out_emit( ERx(" ") );
	gen_sconst( s );
	was_key = 1;
	which_op = 0;
	break;

      case G_H_CODE:		/* Raw data with source program spacing */
	out_emit( s );
	was_key = 0;
	which_op = 0;
	break;

      default:
	break;
    }
    if (flag & G_H_NEWLINE)
    {
	out_emit( ERx("\n") );
	was_key = 0;			/* Newline equivalent to space */
	which_op = 0;
    }
    if (flag & G_H_INDENT)			/* Indent after data */
	gen_do_indent( 1 );
    g_block_dangle = FALSE;
}
