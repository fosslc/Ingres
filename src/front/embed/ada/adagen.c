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
# include 	<lo.h>
# include	<nm.h>
# include	<eqrun.h>
# include	<equel.h>
# include	<eqsym.h>
# include	<eqgen.h>
# include	<eqscan.h>
# include	<ereq.h>
# include	<ere6.h>
# include	<eqada.h>

/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
+* Filename:	adagen.c 
** Purpose:	Maintains all the routines to generate II calls within specific
**	  	language dependent control flow statements.
**
** Language:	Written for VMS ADA.
**
** Defines:	gen_call()			- Standard Call template.
**		gen_if()			- If Then template.
**		gen_else()			- Else template.
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
**		gen_null()			- Var/null by address
**		gen_sconst()			- String constant.
**		gen_do_indent()			- Update indentation.
**		gen_host()			- Host code (declaration) calls.
**		char *gen_ertab[]		- Host-language errors.
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
**		get_ada_io()			- Find ADA I/O descriptor.
**		gen_IIdebug()			- Runtime debug information.
**		gen_sdesc()			- Generate string description.
**		gen__sconst()			- Controls splitting strings.
**		gen__int()			- Integer.
**		gen__float()			- Float.
**		gen__obj()			- Write anything but strings.
**		gen__functab()			- Get function table pointer.
**		gen_hdl_args()			- SET_SQL handler args.
**              gen_datio()			- Generate GET/PUT DATA stmts.
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
** 2. Some calls require special ADA casts or attributes - based on the
*     function and the type, it is done here.
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
**		26-feb-1986	- Converted to ADA (ncg)
**		03-mar-1988	- Modified for VERDIX ADA on UNIX (russ)
**		19-may-1988	- Added IIFRgp entries to gen_ftab (marge)
**		23-mar-1989	- Added cast for STlength (teresa)
**		03-oct-1989	- Modified to allow Vads & Vax together.
**				  Modified Vads float & string processing, and
**				  handling of Vax enumerated types.  (neil)
**		11-may-1989	- Added function calls IITBcaClmAct, 
**				  IIFRafActFld, IIFRreResEntry to gen__ftab 
**				  for entry activation. (teresa)
**		10-aug-1989	- Add function call IITBceColEnd for Emerald.
**				  (teresa)
**		19-dec-1989	- Updated for Phoenix/Alerters (bjb)
**		24-jul-1990	- Added decimal support. (teresal)
**              08-aug-1990 (barbara)
**                      Added IIFRsaSetAttrio to function table for per value
**                      display attributes.
**		17-jul-1990 (gurdeep rahi)
**			Put in Alsys specific changes:
**			gen_data() - Cast integer argument to runtime
**			  routines to "long_integer".
**			gen_ada_io() - Generate arguments of type long
**			  for io routines.
**			gen_sdesc() - Generate the address of a string
**			  prior to being passed to runtime routine.
**		22-feb-1991 (kathryn)
**			Added IILQssSetSqlio and IILQisInqSqlio.
**			      IILQshSetHandler.
**		21-apr-1991 (teresal)
**			Add activate event call - remove IILQeqEvGetio call.
**		14-oct-1992 (lan)
**			Added IILQled_LoEndData.
**		08-dec-1992 (lan)
**			Added IILQldh_LoDataHandler.
**		10-dec-1992 (kathryn)
**                      Added gen_datio() function, and added entries for
**                      IILQlgd_LoGetData, and IILQlpd_LoPutData.
**	26-jul-1993 (lan)
**		Added EXEC 4GL support routines.
**	07/28/93 (dkh) - Added support for the PURGETABLE and RESUME
**			 NEXTFIELD/PREVIOUSFIELD statements.
**	26-Aug-1993 (fredv)
**		Included <st.h>.
**	01-oct-1993 (kathryn)
**	    First argument generated for GET/PUT DATA stmts ("attrs" bit mask),
**          will now be set/stored by eqgen_tl() rather than determined by 
**	    gen_datio, so add one to the number of args in the gen__ftab for 
**	    IILQlpd_LoPutData and IILQlgd_LoGetData.
**      11-jan-1996 (toumi01; from 1.1 axp_osf port)
**          Added kchin's change for axp_osf
**          03-feb-1994 (kchin)
**          Try not do any casting to integer in gen_data(),
**          under 'case T_INT', since this can result in
**          64-bit address being truncated in axp_osf.
**          Also, for enumerated type on DEC Ada, 'Machine_Size'
**          rather than 'Size' is used.  This is changed in
**          gen_iodat().
**	6-may-97 (inkdo01)
**	    Added table entry for IILQprvProcGTTParm.
**      21-apr-1999 (hanch04)
**        Replace STrindex with STrchr
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	2-may-01 (inkdo01)
**	    Added parm to ProcStatus for row procs.
**	26-feb-2002 (toumi01)
**	    add explicit support for GNAT Ada compiler (initially identical
**	    to Verdix Ada)
**	20-jul-2007 (toumi01)
**	    add IIcsRetScroll and IIcsOpenScroll for scrollable cursors
*/


/* Max line length for ADA code - with a bit more space for closing stuff */

# define	G_LINELEN	78

/* 
** Max size of converted string - allow some extra space over scanner for
** extra escaped characters required.
*/

# define 	G_MAXSCONST	300	

/* State for last statement generated to avoid illegal empty blocks */
static  bool	g_block_dangle = FALSE;


/* Flags to be used with gen_sdesc for coding ADA strings */

# define 	AstrNOTRIM	0		/* String constant + C null */
# define 	AstrTRIM	1		/* Via IIsd[_reg] */
# define	AstrDESC	2		/* I/O String, via descriptor */

/* 
** Indentation information - 
**     Each indentation is 2 spaces.  For ADA the indentation is made up of 
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
** A_IO_DESC - ADA I/O descriptor.
**		Defines a structure that is used when looking up how to send
**		a particular I/O argument.  Some information comes , like the
** type and RET/SET flag, and the structure will fill in the extra information,
** like the I/O function suffix, and the way to pass the argument.  For use
** see gen_io, gen_iodat and get_ada_io.
*/

typedef struct {
		/* Info coming in */
    i4		aid_flags;		/* Flags match function flags */
    bool	aid_is_sym;		/* There is a SYM pointer */
    i4		aid_type;		/* Data type */
		/* New info we provide */
    bool	aid_is_var;		/* EQUEL will pass by ISVAR */
    char	*aid_suffix;		/* Suffix before func call */
    char	*aid_arg_desc;		/* Printf string for argument */
    i4		aid_len;		/* I/O length to be used */
} A_IO_DESC;


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
    ERx("/="),   		/* C_NOTEQ */
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
    (char *)0,			/* We do have Activate Else clauses */
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
i4	gen_IIdebug();
i4	gen__sconst();
void	gen_sdesc();
void	gen__int();
void	gen_null();
void	gen__float();
void	gen__obj();
i4	gen_unsupp();
i4	gen_hdl_args();
i4      gen_datio();

/*
** Table of II calls - 
**     Correspond to II constants, the function to call to generate arguments, 
** the flags needed and the number of arguments.  For ADA there is very little
** argument information needed.  I/O calls are handled as a special case.
*/

GEN_IIFUNC	*gen__functab();

GLOBALDEF GEN_IIFUNC	gen__ftab[] = {
/*
** Index     Name			Function	Flags		Args
** -----     -----------		-----------	-----------	------
*/

/* 000 */    {ERx("IIf_"),		0,		0,		0},

			/* Quel # 1 */
/* 001 */    {ERx("IIflush"),		gen_IIdebug,	0,		0},
/* 002 */    {ERx("IInexec"),		0,		0,		0},
/* 003 */    {ERx("IInextget"),		0,		0,		0},
/* 004 */    {ERx("IInotrimio"),	gen_io,		G_SET,		2},
/* 005 */    {ERx("IIparret"),		0,		0,/* param */	0},
/* 006 */    {ERx("IIparset"),		0,		0,/* param */	0},
/* 007 */    {ERx("IIgetdomio"),	gen_io,		G_RET,		2},
/* 008 */    {ERx("IIretinit"),		gen_IIdebug,	0,		0},
/* 009 */    {ERx("IIputdomio"),	gen_io,		G_SET,		2},
/* 010 */    {ERx("IIsexec"),		0,		0,		0},
/* 011 */    {ERx("IIsyncup"),		gen_IIdebug,	0,		0},
/* 012 */    {ERx("IItupget"),		0,		0,		0},
/* 013 */    {ERx("IIwritio"),		gen_io,		G_SET,		3},
/* 014 */    {ERx("IIxact"),		gen_all_args,	0,		1},
/* 015 */    {ERx("IIvarstrio"),	gen_io,		G_SET,		2},
/* 016 */    {ERx("IILQssSetSqlio"),    gen_io,         G_SET,          3},
/* 017 */    {ERx("IILQisInqSqlio"),    gen_io,         G_RET,          3},
/* 018 */    {ERx("IILQshSetHandler"),  gen_hdl_args,   0,              2},
/* 019 */    {(char *)0,                0,              0,              0},
/* 020 */    {(char *)0,		0,		0,		0},

			/* Libq # 21 */
/* 021 */    {ERx("IIbreak"),		0,		0,		0},
/* 022 */    {ERx("IIeqiqio"),		gen_io,		G_RET,		3},
/* 023 */    {ERx("IIeqstio"),		gen_io,		G_SET,		3},
/* 024 */    {ERx("IIerrtest"),		0,		0,		0},
/* 025 */    {ERx("IIexit"),		0,		0,		0},
/* 026 */    {ERx("IIingopen"),		gen_var_args,	0,		15},
/* 027 */    {ERx("IIutsys"),		gen_all_args,	0,		3},
/* 028 */    {ERx("IIexExec"),		gen_all_args,	0,		4},
/* 029 */    {ERx("IIexDefine"),	gen_all_args,	0,		4},
/* 030 */    {(char *)0,		0,		0,		0},

			/* Forms # 31 */
/* 031 */    {ERx("IITBcaClmAct"),	gen_all_args,	0,		4},
/* 032 */    {ERx("IIactcomm"),		gen_all_args,	0,		2},
/* 033 */    {ERx("IIFRafActFld"),	gen_all_args,	0,		3},
/* 034 */    {ERx("IInmuact"),		gen_all_args,	0,		5},
/* 035 */    {ERx("IIactscrl"),		gen_all_args,	0,		3},
/* 036 */    {ERx("IIaddform"),		gen_all_args,	0,		1},
/* 037 */    {ERx("IIchkfrm"),		0,		0,		0},
/* 038 */    {ERx("IIclrflds"),		0,		0,		0},
/* 039 */    {ERx("IIclrscr"),		0,		0,		0},
/* 040 */    {ERx("IIdispfrm"),		gen_all_args,	0,		2},
/* 041 */    {ERx("IIprmptio"),		gen_io,		G_RETSET,	4},
/* 042 */    {ERx("IIendforms"),	0,		0,		0},
/* 043 */    {ERx("IIendfrm"),		0,		0,		0},
/* 044 */    {ERx("IIendmu"),		0,		0,		0},
/* 045 */    {ERx("IIfmdatio"),		gen_unsupp,	IIFDATA,	0},
/* 046 */    {ERx("IIfldclear"),	gen_all_args,	0,		1},
/* 047 */    {ERx("IIfnames"),		0,		0,		0},
/* 048 */    {ERx("IIforminit"),	gen_all_args,	0,		1},
/* 049 */    {ERx("IIforms"),		gen_all_args,	0,		1},
/* 050 */    {ERx("IIfsinqio"),		gen_unsupp,	IIFRSINQ,	3},
/* 051 */    {ERx("IIfssetio"),		gen_unsupp,	IIFRSSET,	3},
/* 052 */    {ERx("IIfsetio"),		gen_all_args,	0,		1},
/* 053 */    {ERx("IIgetoper"),		gen_all_args,	0,		1},
/* 054 */    {ERx("IIgtqryio"),		gen_unsupp,	IIGETQRY,	3},
/* 055 */    {ERx("IIhelpfile"),	gen_all_args,	0,		2},
/* 056 */    {ERx("IIinitmu"),		0,		0,		0},
/* 057 */    {ERx("IIinqset"),		gen_unsupp,	IIINQSET,	5},
/* 058 */    {ERx("IImessage"),		gen_all_args,	0,		1},
/* 059 */    {ERx("IImuonly"),		0,		0,		0},
/* 060 */    {ERx("IIputoper"),		gen_all_args,	0,		1},
/* 061 */    {ERx("IIredisp"),		0,		0,		0},
/* 062 */    {ERx("IIrescol"),		gen_all_args,	0,		2},
/* 063 */    {ERx("IIresfld"),		gen_all_args,	0,		1},
/* 064 */    {ERx("IIresnext"),		0,		0,		0},
/* 065 */    {ERx("IIgetfldio"),	gen_io,		G_RET,		3},
/* 066 */    {ERx("IIretval"),		0,		0,		0},
/* 067 */    {ERx("IIrf_param"),	0,		0,/* param */	0},
/* 068 */    {ERx("IIrunform"),		0,		0,		0},
/* 069 */    {ERx("IIrunmu"),		gen_all_args,	0,		1},
/* 070 */    {ERx("IIputfldio"),	gen_io,		G_SET,		3},
/* 071 */    {ERx("IIsf_param"),	0,		0,/* param */	0},
/* 072 */    {ERx("IIsleep"),		gen_all_args,	0,		1},
/* 073 */    {ERx("IIvalfld"),		gen_all_args,	0,		1},
/* 074 */    {ERx("IInfrskact"),	gen_all_args,	0,		5},
/* 075 */    {ERx("IIprnscr"),		gen_all_args,	0,		1},
/* 076 */    {ERx("IIiqset"),		gen_var_args,	0,		5},
/* 077 */    {ERx("IIiqfsio"),		gen_io,		G_RET,		5},
/* 078 */    {ERx("IIstfsio"),		gen_io,		G_SET,		5},
/* 079 */    {ERx("IIresmu"),		0,		0,		0},
/* 080 */    {ERx("IIfrshelp"),		gen_all_args,	0,		3},
/* 081 */    {ERx("IInestmu"),		0,		0,		0},
/* 082 */    {ERx("IIrunnest"),		0,		0,		0},
/* 083 */    {ERx("IIendnest"),		0,		0,		0},
/* 084 */    {ERx("IIFRtoact"),		gen_all_args,	0,		2},
/* 085 */    {ERx("IIFRitIsTimeout"),	0,		0,		0},

			/* Table fields # 86 */
/* 086 */    {ERx("IItbact"),		gen_all_args,	0,		3},
/* 087 */    {ERx("IItbinit"),		gen_all_args,	0,		3},
/* 088 */    {ERx("IItbsetio"),		gen_all_args,	0,		4},
/* 089 */    {ERx("IItbsmode"),		gen_all_args,	0,		1},
/* 090 */    {ERx("IItclrcol"),		gen_all_args,	0,		1},
/* 091 */    {ERx("IItclrrow"),		0,		0,		0},
/* 092 */    {ERx("IItcogetio"),	gen_io,		G_RET,		3},
/* 093 */    {ERx("IItcoputio"),	gen_io,		G_SET,		3},
/* 094 */    {ERx("IItcolval"),		gen_all_args,	0,		1},
/* 095 */    {ERx("IItdata"),		0,		0,		0},
/* 096 */    {ERx("IItdatend"),		0,		0,		0},
/* 097 */    {ERx("IItdelrow"),		gen_all_args,	0,		1},
/* 098 */    {ERx("IItfill"),		0,		0,		0},
/* 099 */    {ERx("IIthidecol"),	gen_all_args,	0,		2},
/* 100 */    {ERx("IItinsert"),		0,		0,		0},
/* 101 */    {ERx("IItrc_param"),	0,		0,		0},
/* 102 */    {ERx("IItsc_param"),	0,		0,		0},
/* 103 */    {ERx("IItscroll"),		gen_all_args,	0,		2},
/* 104 */    {ERx("IItunend"),		0,		0,		0},
/* 105 */    {ERx("IItunload"),		0,		0,		0},
/* 106 */    {ERx("IItvalrow"),		0,		0,		0},
/* 107 */    {ERx("IItvalval"),		gen_all_args,	0,		1},
/* 108 */    {ERx("IITBceColEnd"),      0,              0,              0},
/* 109 */    {(char *)0,		0,		0,		0},
/* 110 */    {(char *)0,		0,		0,		0},

		/* QA test calls # 111 */
/* 111 */    {ERx("QAmessage"),		gen_var_args,	0,		13},
/* 112 */    {ERx("QAprintf"),		gen_var_args,	0,		13},
/* 113 */    {ERx("QAprompt"),		gen_var_args,	0,		13},
/* 114 */    {(char *)0,		0,		0,		0},
/* 115 */    {(char *)0,		0,		0,		0},
/* 116 */    {(char *)0,		0,		0,		0},
/* 117 */    {(char *)0,		0,		0,		0},
/* 118 */    {(char *)0,		0,		0,		0},
/* 119 */    {(char *)0,		0,		0,		0},

		/* EQUEL cursor routines # 120 */
/* 120 */    {ERx("IIcsRetrieve"),	gen_all_args,	0,		3},
/* 121 */    {ERx("IIcsOpen"),		gen_all_args,	0,		3},
/* 122 */    {ERx("IIcsQuery"),		gen_all_args,	0,		3},
/* 123 */    {ERx("IIcsGetio"),		gen_io,		G_RET,		2},
/* 124 */    {ERx("IIcsClose"),		gen_all_args,	0,		3},
/* 125 */    {ERx("IIcsDelete"),	gen_all_args,	0,		4},
/* 126 */    {ERx("IIcsReplace"),	gen_all_args,	0,		3},
/* 127 */    {ERx("IIcsERetrieve"),	0,		0,		0},
/* 128 */    {ERx("IIcsParGet"),	gen_all_args,	0,		2},
/* 129 */    {ERx("IIcsERplace"),	gen_all_args,	0,		3},
/* 130 */    {ERx("IIcsRdO"),		gen_all_args,	0,		2},
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
/* 140 */    {ERx("IIFRaeAlerterEvent"),gen_all_args,   0,              1},
/* 141 */    {ERx("IIFRgotofld"),	gen_all_args,	0,		1},
/* 142 */    {(char *)0,		0,		0,		0},
/* 143 */    {(char *)0,		0,		0,		0},
/* 144 */    {(char *)0,		0,		0,		0},

		/* PROCEDURE support #145 */
/* 145 */    {ERx("IIputctrl"),		gen_all_args,	0,		1},
/* 146 */    {ERx("IILQpriProcInit"),	gen_all_args,	0,		2},
/* 147 */    {ERx("IILQprvProcValio"),	gen_io,		G_SET,		4},
/* 148 */    {ERx("IILQprsProcStatus"),	gen_all_args,	0,		1},
/* 149 */    {ERx("IILQprvProcGTTParm"),gen_all_args,   0,              2},
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
** Parameters: 	func - i4  - Function index.
** Returns: 	None
**
-* Format: 	func( args );
*/

void
gen_call( func )
i4	func;
{
    gen__II( func );
    out_emit( ERx(";\n") );
    g_block_dangle = FALSE;
}


/*
+* Procedure:	gen_if 
** Purpose:	Generates a simple If statement (Open or Closed).
**
** Parameters: 	o_c  - i4  - Open / Close the If block,
**		func - i4  - Function index,
**		cond - i4  - Condition index,
**		val  - i4  - Right side value of condition.
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
	gen__int( val );					/* Value */
	gen__obj( TRUE, ERx(") then") );			/* Then Begin */
	out_emit( ERx("\n") );
	gen_do_indent( 1 );
	g_block_dangle = TRUE;

	/* Next call should Close or Else */
    }
    else if (o_c == G_CLOSE)
    {
	if (g_block_dangle)			/* Null If blocks are illegal */
	    gen__obj( TRUE, ERx("null;\n") );
	gen_do_indent( -1 );
	gen__obj( TRUE, ERx("end if; ") );			/* End */
	gen_comment( TRUE, gen__functab(func)->g_IIname );
	g_block_dangle = FALSE;
    }
}

/* 
+* Procedure:	gen_else 
** Purpose:	Generate an If block with Else clauses.
**
** Parameters: 	flag	- i4  - Open / Else / Close the If block, 
**		func	- i4  - Function index,
**		cond	- i4  - Condition index,
**		val	- i4  - Right side value of condition,
**		l_else	- i4  - Else label prefix,
**		l_elsen	- i4  - Else label number,
**		l_end	- i4  - Endif label prefix,
**		l_endn	- i4  - Endif label number.
** Returns: 	None
**
** Format (1): 	(open) 		case (func(args)) is
**	   	(else if)	    when val =>
**		(else)		    when others =>
** 	   	(close) 	end case;
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
# ifndef ADA_IF_THEN_ELSE	/* Use Case */

    /* Controls dangling When clause */
    static	bool	g_when_dangle = FALSE;

    if (flag == G_OPEN)	  /* Either "main open, or clause for [last] When */
    {
	if (!g_when_dangle)		/* Very first one in current block */
	{
	    gen__obj( TRUE, ERx("case (") );			/* Case */
	    gen__II( func );					/* Function */
	    gen__obj( TRUE, ERx(") is") );			/* Is */
	    out_emit( ERx("\n") );
	    gen_do_indent( 1 );
	    gen__obj( TRUE, ERx("when ") );			/* When */
	    g_when_dangle = TRUE;
	}
	/* Must be a dangling When (either from first G_OPEN or G_ELSE) */
	if (func != II0)		/* Real value put out as "when val" */
	    gen__int( val );
	else				/* Last When block */
	    gen__obj( TRUE, ERx("others") );
	gen__obj( TRUE, ERx(" =>") );
	out_emit( ERx("\n") );
	gen_do_indent( 1 );
	g_when_dangle = FALSE;			/* No dangling When, but ... */
	g_block_dangle = TRUE;			/* A block was just opened   */
    }
    else if (flag == G_ELSE)
    {
	if (g_block_dangle)			/* Null When blocks are bad */
	    gen__obj( TRUE, ERx("null;\n") );
	gen_do_indent( -1 );
	out_emit( ERx("\n") );			/* Give it a blank line */
	gen__obj( TRUE, ERx("when ") );
	g_when_dangle = TRUE;			/* Is a dangling When */
	g_block_dangle = FALSE;			/* Is not an open block */
    }
    else if (flag == G_CLOSE)
    {
	if (g_block_dangle)			/* Null When blocks are bad */
	    gen__obj( TRUE, ERx("null;\n") );
	gen_do_indent( -2 );
	gen__obj( TRUE, ERx("end case; ") );
	gen_comment( TRUE, gen__functab(func)->g_IIname );
	g_when_dangle = FALSE;			/* Is not a dangling When */
	g_block_dangle = FALSE;			/* Is not an open block */
    }

# else /* ADA_IF_THEN_ELSE */			/* Use If-Then-Else */

    /* Last Else block must close the "els" word */
    if (flag == G_OPEN && func == II0)
    {
	gen__obj( TRUE, ERx("e\n") );
	gen_do_indent( 1 );
	g_block_dangle = TRUE;
    }
    /* We put out "els" to be later followed by "if" or "e" */
    else if (flag == G_ELSE)
    {
	if (g_block_dangle)			/* Null If blocks are illegal */
	    gen__obj( TRUE, ERx("null;\n") );
	gen_do_indent( -1 );
	gen__obj( TRUE, ERx("els") );		/* Followed by an If or II0 */
    }
    else
    {
	gen_if( flag, func, cond, val );	/* Either Open or Close */
	/* g_block_dangle set in gen_if */
    }
# endif /* ADA_IF_THEN_ELSE */
}

/*
+* Procedure:	gen_if_goto 
** Purpose:	Generate an If Goto template.
**
** Parameters: 	func	- i4  - Function index,
**		cond	- i4  - Condition index,
**		val	- i4  - Right side value of condition,
**		labpref	- i4  - Label prefix,
**		labnum	- i4  - Label number.
** Returns: 	None
**
** Format:	if (func(args) condition val) then
**			goto label;
-*		end if;
*/

void
gen_if_goto( func, cond, val, labprefix, labnum )
i4	func;
i4	cond;
i4	val;
i4	labprefix;
i4	labnum;
{
    gen_if( G_OPEN, func, cond, val );
    gen_goto( G_NOTERM, labprefix, labnum );
    gen_if( G_CLOSE, func, cond, val );
    g_block_dangle = FALSE;
}

/*
+* Procedure:	gen_loop 
** Purpose:	Dump a while loop (open the loop or close it).
**
** Parameters: 	o_c	- i4  - Open / Close the loop,
**		labbeg	- i4  - Start label prefix,
**		labend	- i4  - End label prefix,
**		labnum	- i4  - Loop label number.
**		func	- i4  - Function index,
**		cond	- i4  - Condition index,
**		val	- i4  - Right side value of condition.
** Returns: 	None
**
** Format: 	(open)	while (func(args) condition val) loop
-*	   	(close)	end loop;
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
	gen__int( val );					/* Value */
	gen__obj( TRUE, ERx(") loop") );			/* Do Begin */
	out_emit( ERx("\n") );
	gen_do_indent( 1 );
	g_block_dangle = TRUE;
    }
    else
    {
	if (g_block_dangle)		/* Null Loop blocks are illegal */
	    gen__obj( TRUE, ERx("null;\n") );
	gen_do_indent( -1 );
	gen__obj( TRUE, ERx("end loop; ") );
	gen_comment( TRUE, gen__functab(func)->g_IIname );
    }
}


/*
+* Procedure:	gen_goto 
** Purpose:	Put out a Goto statement.
**
** Parameters: 	flag	  - i4  - Needs If, or needs terminator (unused).
**		labprefix - i4  - Label prefix,
**		labnum	  - i4  - Label number.
** Returns: 	None
**
** Format:	[ if (TRUE) then ] goto label; [end if;]
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
    if (flag == G_IF)
	out_emit( ERx("; end if;\n") );
    else
	out_emit( ERx(";\n") );
    g_block_dangle = FALSE;
}

/*
+* Procedure:	gen__goto 
** Purpose:	Local statement independent Goto generator.  
** Parameters:	labprefix - i4  - Label prefix index.
**		labnum    - i4  - Label number to concatenate.
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
** Parameters: 	loop_extra - i4  - Is it extra label for a loop? 
**				  (see usage in grammar)
**	 	labprefix  - i4  - Label prefix,
**		labnum 	   - i4  - Label number.
** Returns: 	None
**
-* Format:	<<label>> [null;]
**
** Note: Some labels are never followed by a statement or are only sometimes
**       followed by a statement.  The ADA compiler complains about an "end" 
** without a preceding "null;" after the label.  To avoid this we put a
** "null;" after these labels.
*/

void
gen_label( loop_extra, labprefix, labnum )
i4	loop_extra;
i4	labprefix;
i4	labnum;
{
    /* Labels will always begin in the left margin */
    out_emit( ERx("<<") );
    gen__label( FALSE, labprefix, labnum );
    if (labprefix==L_RETEND || labprefix==L_FDEND || labprefix==L_FDINIT)
	out_emit( ERx(">> null;\n") );		/* No following statement */
    else
	out_emit( ERx(">>\n") );
}

/*
+* Procedure:	gen__label 
** Purpose:	Local statement independent label generator.  
**
** Parameters:	ind	  - i4  - Indentation flag,
**		labprefix - i4  - Label prefix,
**		labnum    - i4  - Label number.
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
-* Format:	-- comment string
*/

void
gen_comment( term, str )
i4	term;
char	*str;
{
    gen__obj( TRUE, ERx("-- ") );
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
-* Format:	With filename;  Use filename;
**
** Notes:	The filename is given to EQUEL to ensure that we parse
**		all the variables (code should be done INLINE), but we
** assume that all the variables are in the filename's package, and we
** generate a With clause.
*/

void
gen_include( fullnm, suff, extra )
char	*fullnm;
char	*suff;
char	*extra;
{
    char	   filebuf[MAX_LOC+1];
    register char *cp;

    STcopy( fullnm, filebuf );

    /* Walk back to strip off file name - LOdetail does not work on VMS */
    cp = STrchr( filebuf, '.' );
    if (cp)
    {
	register char	*p;

	*cp = '\0';
	p = NULL;
	for (cp=filebuf; *cp != EOS; CMnext(cp))
	{
	    if (p==NULL && CMnmstart(cp))
		p = cp;
	    else if (!CMnmchar(cp))
	    	p = NULL;
	}
	if (p)
	    cp = p;
	else
	    cp = ERx("Bad_file_name");
    } else
	cp = filebuf;			/* Must be a logical name */
	
    /*
    ** Remove the trailing underscore from package names which are
    ** used under VMS.
    */
    if (eq->eq_config & EQ_ADVAX)
    {
	if (cp[((i4)STlength(cp))-1] == '_')	/* Knock trailing _ */
	    cp[((i4)STlength(cp))-1] = '\0';
    }

    out_emit( ERx("with ") );
    out_emit( cp );
    out_emit( ERx(";  use ") );
    out_emit( cp );
    out_emit( ERx(";\n") );
}

/*
+* Procedure:	gen_eqstmt 
** Purpose:	Open / Close an Equel statement.
**
** Parameters:	o_c  - i4      - Open / Close,
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
-* Format:	-- File "filename", Line linenum   { comment }
*/

void
gen_line( s )
char	*s;
{
    char	fbuf[G_MAXSCONST +20]; 

    if (out_cur->o_charcnt)
	out_emit( ERx("\n") );
    STprintf( fbuf, ERx("--File \"%s%s%s\", Line %d"), eq->eq_filename,
	eq->eq_infile == stdin ? ERx("") : ERx("."), eq->eq_ext, eq->eq_line );
    if (s)
    {
	STcat( fbuf, ERx("\t{ ") );
	STcat( fbuf, s );
	STcat( fbuf, ERx(" } ") );
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
-* Procedure:	gen_declare 
** Purpose:	Process a the ## WITH EQUEL statement.
** Parameters:  forms - bool - Flag for EQUEL or EQUEL_FORMS.
** Returns:	None
-* Generates:	With EQUEL or With EQUEL_FORMS;
*/

void
gen_declare( forms )
bool	forms;
{
    static bool	asked = FALSE;

    /* Spit out the With line */
    gen__obj(FALSE, ERx("with EQUEL; use EQUEL;             -- INGRES\n"));
    if (forms)
	gen__obj( FALSE,
		ERx("with EQUEL_FORMS; use EQUEL_FORMS; -- INGRES/FORMS\n") );

    /* A bit of joint trickery! */
    if (!asked)
    {
	char	*lname;

	NMgtAt( ERx("II_ADA_WHO"), &lname );
	if (lname != (char *)0 && *lname != '\0')
	{
	    SIprintf( ERx("## package EQUEL_ADA is\n") );
	    SIprintf(
		ERx("##    Written by Neil C. Goodman & Mark R. Wittenberg ") );
	    SIprintf( ERx("(RTI), February 1986;\n") );
	    SIprintf( ERx("## end EQUEL_ADA;\n") );
	    SIflush( stdout );
    	}
	asked = TRUE;
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
**		diff  - i4  *  - Mark if there were changes made to string.
-* Returns:	char *         - Pointer to static converted string.
**
** Notes:
** 1. Special Characters.
** 1.1.	Include the '\' character before we send a double quote to the 
**	database, if it was not '\'.
** 1.2.	Process the host language quote embedded in a string, whether it got
**	there by escaping it with a '\' or by doubling it.
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

    *diff = 0;

    if (flag == G_INSTRING)		/* Escape the first quote */
    {
	*ocp++ = '"';
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
	    /*
	    ** Either entered via escaping it:  \"
	    **		          doubling it:  ""
	    ** or using a different delimiter:  '
	    ** In all cases add two, and if was originally doubled then knock 
	    ** out the following double.
	    */
	    if (flag == G_INSTRING && !esc)	/* DB needs escape */
		*ocp++ = '\\';
	    *ocp++ = '"';			/* Double the quote for ADA */
	    *ocp++ = '"';
	    icp++;				/* Skip over the quote */
	    if (*icp == '"')			/* User already doubled */
		icp++;
	    *diff = 1;
	    esc = 0;
	}
	else
	{
	    CMcpyinc( icp, ocp );
	    esc = 0;
	}
    }
    if (flag == G_INSTRING)		/* Escape closing quote */
    {
	*ocp++ = '"';
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
** Parameters: 	func  - i4  - Function index.
** Returns: 	None
**
** Notes:
**	This routine assumes language dependency, and knowledge of the actual 
**	arguments required by the call.
**
**	If the GEN_DML bit is on in "func", then vector out to the alternate
-*	gen table.  It is a fatal error if there is none.
** History:
**	10-dec-1992 (kathryn)
**	    New gen_datio also requires func as parameter.
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
	if (g->g_func == gen_io)			/* Special suffixes */
	    gen_io( func );
	else if (g->g_func == gen_datio)
            gen_datio( func );
	else if (g->g_func == gen_IIdebug)		/* Maybe no parens */
	    gen_IIdebug();
	else
	{
	    out_emit( ERx("(") );
	    (*g->g_func)( g->g_flags, g->g_numargs );
	    out_emit( ERx(")") );
	}
    }
    arg_free();
}

/*
+* Procedure:	gen_all_args 
** Purpose:	Format all the arguments stored by the argument manager.
**
** Parameters: 	flags   - i4  - Flags and Number of arguments required.
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
** Parameters: 	flags   - i4  - Flags and Number of arguments required.
**		numargs - i4  - Unused
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
    register	bool comma = FALSE;

    while (i = arg_next())		/* Varying num of arguments */
    {
	if (comma)
	    out_emit( ERx(",") );
	comma = TRUE;			/* For next argument */
	gen_data( flags, i );
    }
    return 0;
}

/*
+* Procedure:	gen_data 
** Purpose:	Whatever is stored by the argument manager for a particular 
**	      	argument, dump to the output file.  
**
** Parameters: 	
**	flags	- i4  - Flags type of argument.
**	arg	- i4  - Argument number.
-* Returns: 	None
**
** This routine should be used when no validation of the data of the argument 
** needs to be done. Hopefully, if there was an error before (or while) 
** storing the argument this routine will not be called.
*/

void
gen_data( flags, arg )
i4		flags;
register i4	arg;
{
    i4		num;
    f8		flt;
    char	*s;
    SYM		*sy;
    char	buf[ G_MAXSCONST +1 ];

    if (sy = arg_sym(arg))		/* User variable */
    {
	switch (sym_g_btype(sy))
	{
	  case T_INT: 	/* Cast to an integer, just in case */
	    if (((i4)sym_g_vlue(sy) & AvalENUM) && sym_g_type(sy)) /* Enum */
		STprintf( buf, ERx("%s'Pos(%s)"), sym_str_name(sym_g_type(sy)),
			  arg_name(arg) );
	    else
	    {
		if (eq->eq_config & EQ_ADALSYS)
		    STprintf( buf, ERx("Long_Integer(%s)"), arg_name(arg) );
		else	
#if defined(axp_osf)
             /* try not do such casting on axp_osf, since this can result
                in 64-bit address being truncated */
                    STprintf( buf, ERx("%s"), arg_name(arg) );
#else /* axp_osf */
		    STprintf( buf, ERx("Integer(%s)"), arg_name(arg) );
#endif /* axp_osf */
	    }
	    gen__obj( TRUE, buf );
	    break;
	  case T_FLOAT:		/* Shouldn't happen - pass by reference */
	    STprintf( buf, ERx("%s'Address"), arg_name(arg) );
	    gen__obj( TRUE, buf );
	    break;
	  case T_CHAR:
	  default:
	    /*
	    ** G_TRIM - if a string var, then trim it (QAcalls). Not IIwritedb
	    **          as that is defined to accept a string (not an integer).
	    */
	    gen_sdesc( flags==G_TRIM ? AstrTRIM:AstrNOTRIM, sy, arg_name(arg) );
	    break;
	}
	return;
    }

    /* Based on the data representation, dump the argument */
    switch (arg_rep(arg))
    {
      case ARG_INT:  
	arg_num( arg, ARG_INT, &num );
	gen__int( num );
	break;
      case ARG_FLOAT:  
	arg_num( arg, ARG_FLOAT, &flt );
	gen__float( flt );
	break;
      case ARG_CHAR:  
	switch (arg_type(arg))
	{
    	  case ARG_CHAR: 	/* String constant - may need trimming */
	    if (s = arg_name(arg))
	    {
		/*
		** G_TRIM - QAcalls trim string constants (for ADA procedure
		**	    definition.
		*/
		gen_sdesc( flags==G_TRIM ? AstrTRIM:AstrNOTRIM, (SYM *)0, s );
	    }
	    else if (flags != G_SQLDA && (eq->eq_config & EQ_ADVAX))
		gen__obj(TRUE, ERx("String'Null_parameter")); /* Null vax str */
	    else
		gen__obj( TRUE, ERx("IInull") );	      /* Null address */
	    break;
	  case ARG_RAW:		/* Raw data stored - pass by name */
	  default:		/* String of user numeric - pass by name */
	    gen__obj( TRUE, arg_name(arg) );
	    break;
	}
	break;
    }
}


/*
+* Procedure:	gen_io
** Purpose:	Generate an I/O variable to the output file.
**
** Parameters: 
**	func - i4  - Function number.
-* Return Values:
**	0
**
** Notes:
**  1.  There will always be a minimum of two arguments: the null indicator
** 	and the actual argument.  If there are just 2 arguments then we rely
** 	on the following template format:
**		IIretfunc( indaddr, var_desc )] );
**		IIsetfunc( indaddr, var_or_val_desc );
**	where var_desc or var_or_val_desc is:
**		ISVAR, TYPE, LEN
**
**      If there are 3 arguments then we rely on the following template 
**      format:
**		IIretfunc( indaddr, var_desc, "object" );
**		IIsetfunc( "object", indaddr, var_or_val_desc );
**	where var_desc or var_or_val_desc is:
**		ISVAR, TYPE, LEN, var_or_val
**      We figure out which one is the null indicator, the data argument
**	which must be described and the others we just dump.
**
**      For example, IIsetfield has one extra object 
**	(fieldname), while IIiqfrs has three extra objects (flag, objectname
**	and rownumber) and IIretdom as no extra objects.
**
**   2. I/O calls - their names and passing mechanisms:
**	Routine:  gen_iodat
**
**   	There are a few different cases which rely heavily on the package
**	definitions of EQUEL and EQUEL_FORMS.  There are basically four input
**	(SET) routines and two output (RET) routines.  Ignoring the 
**	'extra objects' that we must generate the routines are declared as
**	something like the following:
** 	
**		IIsetI( NOVAR, T_INT,   sizeof(i4); ival: Integer    by val );
**		IIsetF( ISVAR, T_FLOAT, sizeof(f8); fref: Long_Float by ref );
**		IIsetA( ISVAR, vartyp,  varlen;     addr: Address    by val );
**		IIsetS( ISVAR, T_CHAR,  0; 	    str:  C String   by desc);
**
**		IIretA( ISVAR, vartyp,  varlen;     addr: Address    by val );
**		IIretS( ISVAR, T_CHAR,  0; 	    str:  String     by desc);
**
**	These routines are overloaded, and most go to one C run-time routine.
**	When using strings (on a RET, a RETSET, or a SET) the string is
**	automatically passed by descriptor, so that is what is used:
**
**		IIputdomioS( ind, ISVAR, T_CHAR, 0, String(strvar) ); -- Regular
**		IInotrimioS( ind, ISVAR, T_CHAR, 0, String(strvar) ); -- Notrim
**		IIretdomioS( ind, ISVAR, T_CHAR, 0, String(strvar) ); -- Always
**
**	Note that the variable may have to be cast when setting because the
**	packages assume that they are the generic types and not derived types.
**	For example:
**
**		word: short_integer;
**
**		IIsetI( ISVAR, T_INT, sizeof(i4), Integer(word) );
**
**  3.  Rules for passing arguments & for filling the table in get_ada_io:
**	Routine:  get_ada_io
**
**	Some of these rules and information is implied by the definition
**	of the arguments (their types and passing mechanisms) in the EQUEL-
**	defined packages, and some is explicitly done by casting, Integer(word),
**	and adding attributes, var'Address.  For the details about how we
**	generate the function suffixes (I, F, A and S) and the arguments see
**	get_ada_io.
*/

i4
gen_io( func )
i4	func;
{
    register GEN_IIFUNC		*gio;		/* Current I/O function */
    SYM				*sy;		/* Symtab info */
    register i4		ioarg;		/* What is the I/O argument */
    i4				artype;		/* Type of I/O argument */
    i4				i4num;		/* Numerics stored as strings */
    f8				f8num;
    char			datbuf[ 50 ];	/* To pass to gen_iodat */
    char			*data = datbuf;
    char			*strdat;	/* String rep */

    gio = gen__functab( func );
    /* 
    ** Figure out which is I/O argument based on number of args and mode:
    ** set (name = value) or ret (var = name).  G_RETSET is like G_SET in
    ** the case of "where the ioarg is".
    */
    ioarg = (gio->g_flags == G_RET) ? 2 : gio->g_numargs;

    if ((sy = arg_sym(ioarg)) != (SYM *)0)    /* User var used */
	gen_iodat( func, sy, sym_g_btype(sy), arg_name(ioarg) );
    else	/* Is a literal */
    {
	if (gio->g_flags == G_RET)		/* User error */
	    return 0;

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
		data = strdat;
	    else		/* Convert numeric to string */
	    {
		arg_num( ioarg, ARG_INT, &i4num );
		CVna( i4num, data );
	    }
	    /* Will send i4 by value */
	    gen_iodat( func, (SYM *)0, T_INT, data );
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
	    ** VMS ADA plain floating constants will be coerced into
	    ** a LONG_FLOAT argument.
	    */
	    gen_iodat( func, (SYM *)0, T_FLOAT, data );
	    break;
	  case ARG_PACK:
	    gen_iodat( func, (SYM *)0, T_PACKASCHAR, strdat );
	    break;
	  case ARG_CHAR:
	  default:
	    /* Will be decided on how to send this - IIsd or with Null */
	    gen_iodat( func, (SYM *)0, T_CHAR, strdat );
	    break;
	}
    } /* If a variable */

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
** 	flags   - i4  - Flags and Number of arguments required.
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
**	08-dec-1992 (lan)
**	    Modified to generate correct arguments for IILQldh_LoDataHandler.
*/
i4
gen_hdl_args(flags, numargs )
i4      flags, numargs;
{
    i4          num;
    i4	        arg;
    char	buf[ G_MAXSCONST +1 ];
    char	argbuf[ G_MAXSCONST +1 ];

    arg_num( 1, ARG_INT, &num );
    gen__int( num );
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
			gen__obj( TRUE, ERx("IInull") );
		else
		{
		    gen__obj( TRUE, ERx("0") );
    		    er_write(E_EQ0090_HDL_TYPE_INVALID, EQ_ERROR, 1, buf);
	        }
    		break;
        case  ARG_RAW:
    		gen__obj( TRUE, buf);
		gen__obj( TRUE, ERx("'Address"));
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
		gen__obj( TRUE, ERx("IInull") );
	else
    		gen__obj( TRUE, argbuf);
    }
    return 0;
}

/*
+* Procedure:	gen_iodat
** Purpose:	Make real I/O call.
** Parameters: 
**	func 	- i4	- Function index.
**	sy	- SYM *	- Symbol table entry if a variable.
**	type	- i4	- Data type.
**	data	- char *- Data name (may be var name or data as a string).
** Returns: 	None
-* Notes:	See comments in gen_io of details.
*/

void
gen_iodat( func, sy, type, data )
i4		func;
SYM		*sy;
i4		type; 
register char	*data;
{
    register GEN_IIFUNC	*gio;		/* Current I/O function */
    register A_IO_DESC	*aid;
    i4			ar;
    i4			len;
    char		buf[ G_MAXSCONST +1 ];
    A_IO_DESC		*get_ada_io();

    gio = gen__functab(func);
    aid = get_ada_io( func, sy, &type );

    /* Gen following suffix after func name (done by gen__II) */
    out_emit( aid->aid_suffix );
    out_emit( ERx("(") );

    /* Do we need a name or other data before I/O argument */
    if (gio->g_flags == G_SET || gio->g_flags == G_RETSET)  /* (name = val) */
    {
	for (ar = 1; ar <= gio->g_numargs-2; ar++)
	{
	    gen_data( 0, ar );
	    out_emit( ERx(",") );
	}
	gen_null( gio->g_numargs - 1 );
    } else
	gen_null( 1 );

    /* Dump data information */
    gen__int( aid->aid_is_var );	/* Is by ref to EQUEL routine */
    out_emit( ERx(",") );

    gen__int( type == T_NUL ? -type : type );		/* EQUEL type */
    out_emit( ERx(",") );

    if ((len = aid->aid_len) < 0)	/* Could not determine EQUEL length */
	len = sy ? sym_g_dsize(sy) : 0;

    if (type == T_CHA)			/* Must have been Vads G_RET String */
    {
	STprintf(buf, ERx("%s'Length"), data);
	gen__obj(TRUE, buf);
    }
    else if (type == T_INT && sy && (sym_g_useof(sy) & syFisVAR) &&
	     ((i4)sym_g_vlue(sy) & AvalENUM))
    {
#if defined(axp_osf)
    /* Note: Make sure "Machine_Size" is used for enumerated type
       on DEC OSF/1, otherwise an error message will be generated
       during runtime:
        A datatype conversion error occurred while getting a value
        from tablefield persontbl column _state. (E_FI2054) */

        char  *size_str = (eq->eq_config & EQ_ADALSYS) ?
#else /* axp_osf */
	char  *size_str = (eq->eq_config & (EQ_ADVADS|EQ_ADGNAT|EQ_ADALSYS)) ?
#endif /* axp_osf */
				 ERx("Size") : ERx("Machine_Size");
	
	/* Generate enum_type_name'size/8 for length */
	STprintf(buf, ERx("%s'%s/8"), sym_str_name(sym_g_type(sy)), size_str);
	gen__obj(TRUE, buf);
    }
    else
    {
	gen__int( len  );		/* EQUEL length */
    }

    out_emit( ERx(",") );

    switch (type) 			/* I/O argument based on type */
    {
      case T_PACKASCHAR:		/* Decimal constant pass as string */
	gen_sdesc ( AstrNOTRIM, sy, data);
	break;
      case T_CHA:			/* Special case of Vads G_RET string */
      case T_CHAR:
	if (gio->g_flags == G_SET)	/* Special case - has no aid_arg_desc */
	    gen_sdesc( AstrDESC, sy, data );
	else
	{
	    STprintf( buf, aid->aid_arg_desc, data );
	    gen__obj( TRUE, buf );
	}
	break;
      case T_INT:	/* Check for enumerated literals on input */
	if (sy && ((i4)sym_g_vlue(sy) & AvalENUM) &&
	    (sym_g_useof(sy) & syFisCONST) &&
	    sym_g_type(sy) && gio->g_flags == G_SET)
	{
	    STprintf( buf, ERx("%s'Pos(%s)"), sym_str_name(sym_g_type(sy)),
									data );
	    gen__obj( TRUE, buf );
	    break;
	}
	/* else FALL THROUGH */
      case T_NUL:
      case T_FLOAT:
      default:
	STprintf( buf, aid->aid_arg_desc, data );
	gen__obj( TRUE, buf );
	break;
    }

    /* Do we need a name or other data after I/O argument */
    if (gio->g_flags == G_RET)			/* (var = name ) */
    {
	for (ar = 3; ar <= gio->g_numargs; ar++)
	{
	    out_emit( ERx(",") );
	    gen_data( 0, ar );
	}
    }
    out_emit( ERx(")") );
}


/*
+* Procedure:	get_ada_io
** Purpose:	Find an ADA I/O descriptor mapping to what we must use for
**		current I/O call.
** Parameters:
**	func	- i4	- Function index.
**	sy	- SYM * - Symbol table pointer.
**	type	- i4	- Type of the data (may change from T_CHAR to T_CHA)
** Return Values:
**	A_IO_DESC *     - Pointer to entry in table that tells how to dump
-*			  the I/O argument.
** Notes:
**	This routine relies on the comment before ge_io on how to send
**	I/O arguments.  In particular, the suffix to the function, and the
**	way to actually write the argument are of importance.  String
**	arguments, once processed by this routine, are again handled via
**	another level (gen_sdesc) - that is why they have NULL entries for
**	their "argument description".
**
**	We look for an entry whose RET/SET flag, SYM * flag, and types match
**	the input specs.  We return that entry which supplies more information
**	(the ISVAR flag, the function suffix, and the argument printf string, 
**	possibly the length too) to the caller.  If not entry was found, we
**	assume that a bad var was used (an error was already printed in
**	grammar) and we set the return value to the T_UNDEF entry (AioNOTFOUND).
**
**	Both integral types (on G_SET) and string types always get a cast,
**	even if not strictly necessary.  This saves complication in the
**	grammar checking for the need, and doesn't cost anything at runtime.
**	Short_integer is not the same type as Integer, and "array of char" is
**	not the same type as String, even though they are compatible if cast.
**	Note that G_SET strings are done by hand in gen_sdesc.
**
**	A special handling of G_RET strings is done for Vads Ada.  The 
**	descriptor returned is one that can go through the "Address"
**	interface and pass the string by address (and the length of that
**	string).  The type must be modified to T_CHA to allow this to work.
*/

A_IO_DESC	*
get_ada_io( func, sy, type )
i4	func;
SYM	*sy;
i4	*type;
{
# define AioNOTFOUND	8	/* Fall out entry marker for not-found */
# define AioNOSIZE	(-1)	/* Flag that we cannot determine the size */

    static	A_IO_DESC	aid_tab[] = {

    /*
    ** Static table:
    **	   DO NOT INSERT ROWS INTO THIS TABLE WITHOUT ADJUSTING #defines ABOVE
    ** Info we compare with    -  New info we provide
    ** Flag  Is SYM  Type    Is Var  Suff       Arg-Desc 	   Length
    ** ----  ------  ------  ------  --------   --------	   ------
    */
				/*SET*/
/*INT*/
    {G_SET,  FALSE,  T_INT,  FALSE,  ERx("I"),  ERx("%s"),	   sizeof(i4)},
    {G_SET,  TRUE,   T_INT,  FALSE,  ERx("I"),  ERx("Integer(%s)"),sizeof(i4)},
    {G_SET,  TRUE,   T_INT,  FALSE,  ERx("I"),  ERx("Long_Integer(%s)"),sizeof(i4)},
    /* This next entry is for enumerated variables */
    {G_SET,  TRUE,   T_INT,  TRUE,   ERx("A"),  ERx("%s'Address"), AioNOSIZE},
/*FLT*/
    /* Next 2 rows are for Vax and Vads Ada Input flt lits (Vads = Value) */
    {G_SET,  FALSE,  T_FLOAT,TRUE,   ERx("F"),  ERx("%s"),	   sizeof(f8)},
    {G_SET,  FALSE,  T_FLOAT,FALSE,  ERx("F"),  ERx("%s"),	   sizeof(f8)},
    {G_SET,  TRUE,   T_FLOAT,TRUE,   ERx("A"),  ERx("%s'Address"), AioNOSIZE },
/* NUL */
    {G_SET,  TRUE,   T_NUL,  FALSE,  ERx("I"),  ERx("0"), 1 },
/*UDF	- Can only happen with Is_SYM = TRUE; next entry is AioNOTFOUND       */
    {G_SET,  TRUE,   T_UNDEF,TRUE,   ERx("A"),  ERx("%s"),	   sizeof(i4)},
/*CHR	- Special cases; do not have arg-desc entries. For IIx...io SET calls */
    {G_SET,  FALSE,  T_CHAR, TRUE,   ERx("S"),  (char *)0,	   0},
    {G_SET,  TRUE,   T_CHAR, TRUE,   ERx("S"),  (char *)0,	   0},
/* DECCHAR - pass a decimal as a character string */
    /* Next 2 rows are for VAX & Vads Ada Input decimal lits (Vads = addr) */	
    {G_SET,  FALSE,  T_PACKASCHAR, TRUE,  ERx("D"), ERx("%s"),	   0},
    {G_SET,  FALSE,  T_PACKASCHAR, TRUE,  ERx("A"), ERx("%s"),	   0},

				/*RET*/
/*INT*/
    {G_RET,  TRUE,   T_INT,  TRUE,   ERx("A"),  ERx("%s'Address"), AioNOSIZE},
/*FLT*/
    {G_RET,  TRUE,   T_FLOAT,TRUE,   ERx("A"),  ERx("%s'Address"), AioNOSIZE},
/*UDF*/
    {G_SET,  TRUE,   T_UNDEF,TRUE,   ERx("A"),  ERx("%s'Address"), sizeof(i4)},
/*CHR*/
    /* Next 2 rows are for Vax, Vads & Alsys Ada Return strings */
    {G_RET,  TRUE,   T_CHAR, TRUE,   ERx("S"),  ERx("String(%s)"), 0}, /* Vax */
    {G_RET,  TRUE,   T_CHAR, TRUE,   ERx("A"),  ERx("%s'Address"), 0}, /* Vads*/

    {0,      FALSE,  0,      FALSE,  (char *)0, (char *)0,	   0}
    };
    register A_IO_DESC	*aid;
    bool	is_sym = (sy != (SYM *)0);
    register GEN_IIFUNC	*gio;			/* Current I/O function */
    i4			flags;

    gio = gen__functab( func );
    flags = gio->g_flags;
    if (flags == G_RETSET)	/* RETSET is like RET for this purpose */
	flags = G_RET;

    /* Find AID entry with matching attributes */
    for (aid = aid_tab; aid->aid_suffix; aid++)
    {
	if (flags == aid->aid_flags &&
		is_sym == aid->aid_is_sym &&
		*type == aid->aid_type)
	{
	    if (   flags==G_RET && *type == T_CHAR
		&& (eq->eq_config & EQ_ADVAX) == 0)
	    {
		*type = T_CHA;		/* Vads and Alsys use string address */
		aid++;
	    }
	    else if (flags == G_SET && *type == T_FLOAT && !is_sym && 
		     (eq->eq_config & (EQ_ADVADS|EQ_ADGNAT|EQ_ADALSYS)))
	    {
		aid++;			/* Vads by value */
	    }
	    else if (flags == G_SET && *type == T_PACKASCHAR &&
		     (eq->eq_config & (EQ_ADVADS|EQ_ADGNAT)))
	    {
		aid++;			/* Vads pass address */
	    }
	    else if (flags == G_SET && *type == T_INT && sy &&
		     (sym_g_useof(sy) & syFisVAR) &&
		     ((i4)sym_g_vlue(sy) & AvalENUM))
	    {
		aid += 2;	/* Enumerated variables by ref on input */
	    }
	    else if (flags == G_SET && *type == T_INT && sy &&
				(eq->eq_config & EQ_ADALSYS))
	    {
		aid++ ;   /* Alsys casting for ints */
	    }
	    return aid;
	}
    }

    /* Could not find it - Must be a higher level error */
    if (aid->aid_suffix == (char *)0)
	return &aid_tab[ AioNOTFOUND ];
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
**      passed without the EOS terminator.
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
**	10-dec-1992   (kathryn) Written.
**	01-oct-1993 (kathryn)
**          First arg generated for GET/PUT DATA stmts ("attrs" mask) is the
**	    last arg stored.  It is now set/stored in eqgen_tl(), rather than 
**	    determined here.
*/
i4
gen_datio( func )
i4     func;
{
    register i4                 ar;
    i4                          artype;         /* Type of I/O argument */
    i4                          type;
    i4                          len;
    i4                          i4num;          /* Numerics stored as strings */
    i4                          attrs;
    f8                          f8num;
    char                        *strdat;
    char                        datbuf[ 50 ];
    char                        *data = datbuf;
    char                        buf[ G_MAXSCONST +1 ];
    SYM                         *sy;            /* Symtab info */
    register  A_IO_DESC         *aid;
    register GEN_IIFUNC         *gio;           /* Current I/O function */


    
    /* Process the I/O argument */
    ar = 1;
    strdat = (char *)0;
    if ((sy = arg_sym(ar)) != (SYM *)0)    /* User var used */
    {
        data = arg_name(ar);
        type = sym_g_btype(sy);
    }
    else if (!arg_name(ar))
    {
        type = T_NONE;
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
                if (!strdat)
                {
                    arg_num(ar, ARG_INT, &i4num);
                    CVna( i4num, data );
                }
                type = T_INT;
                break;
            case ARG_FLOAT:
                if (!strdat)
                {
                    arg_num(ar, ARG_FLOAT, &f8num);
                    STprintf( data, ERx("%f"), f8num, '.' );
                }
                type = T_FLOAT;
                break;
            case ARG_PACK:
                type = T_PACKASCHAR;
                break;
            case ARG_CHAR:
            default:
                type = T_CHAR;
                break;
        }
    }
    gio = gen__functab( func );
    aid = get_ada_io( func, sy, &type );
    arg_num( gio->g_numargs, ARG_INT, &attrs );
    if (!(attrs & II_DATSEG))
        out_emit(ERx("A") );
    else
        out_emit( aid->aid_suffix );
    out_emit( ERx("(") );
    gen__int(attrs);
    gen__obj( TRUE, ERx(",") );
    gen__int( type == T_NUL ? -type : type );           /* EQUEL type */
    out_emit( ERx(",") );

    if (!(attrs & II_DATSEG))
    {
        gen__int(0);
        out_emit( ERx(",") );
        gen__obj( TRUE, ERx("IInull") );
    }
    else
    {
        if ((len = aid->aid_len) < 0)     /* Could not determine EQUEL length */
            len = sy ? sym_g_dsize(sy) : 0;
        if (type == T_CHA)               /* Must be Vads G_RET String */
        {
            STprintf(buf, ERx("%s'Length"), data);
            gen__obj(TRUE, buf);
        }
        else if (type == T_INT && sy && (sym_g_useof(sy) & syFisVAR) &&
                 ((i4)sym_g_vlue(sy) & AvalENUM))
        {
            char  *size_str = (eq->eq_config & (EQ_ADVADS|EQ_ADGNAT|EQ_ADALSYS)) ?
                       ERx("Size") : ERx("Machine_Size");
            /* Generate enum_type_name'size/8 for length */
            STprintf(buf, ERx("%s'%s/8"), sym_str_name(sym_g_type(sy)),
                        size_str);
            gen__obj(TRUE, buf);
        }
        else
            gen__int( len  );               /* EQUEL length */
        out_emit( ERx(",") );

        /* Segment argument based on type */
        switch (type)
        {
          /* Decimal constant pass as string */
          case T_PACKASCHAR:
            gen_sdesc ( AstrNOTRIM, sy, data);
            break;

          /* Special case of Vads G_RET string */
          case T_CHA:
          case T_CHAR:
            if (gio->g_flags == G_SET)          /* Special - no aid_arg_desc */
                gen_sdesc( AstrDESC, sy, data );
            else
            {
                STprintf( buf, aid->aid_arg_desc, data );
                gen__obj( TRUE, buf );
            }
            break;
          case T_INT:       /* Check for enumerated literals on input */
            if (sy && ((i4)sym_g_vlue(sy) & AvalENUM) &&
                (sym_g_useof(sy) & syFisCONST) &&
                sym_g_type(sy) && gio->g_flags == G_SET)
            {
                STprintf( buf, ERx("%s'Pos(%s)"),
                        sym_str_name(sym_g_type(sy)), data );
                gen__obj( TRUE, buf );
                break;
            }
            /* else FALL THROUGH */
          case T_NUL:
          case T_FLOAT:
          default:
            STprintf( buf, aid->aid_arg_desc, data );
            gen__obj( TRUE, buf );
            break;
        }
    }
    for (ar = 2; ar < gio->g_numargs; ar++)
    {
        gen__obj( TRUE, ERx(",") );
        if (!arg_name(ar))
	{
	    if (gio->g_flags == G_RET && ar != 2)
            	gen__obj( TRUE, ERx("IInull") );
	    else
		gen__int(0);
	}
        else if (gio->g_flags == G_RET && ar != 2)
        {
            STprintf( buf, ERx("%s'Address"), arg_name(ar) );
            gen__obj( TRUE, buf );
        }
        else
            gen_data( gio->g_flags, ar );
    }
    out_emit( ERx(")") );
    return 0;
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
	out_emit( ERx("(") );
	STprintf( fbuf, ERx("%s%s%s"), eq->eq_filename,
	          eq->eq_infile == stdin ? ERx("") : ERx("."), eq->eq_ext );
	gen_sdesc( AstrNOTRIM, (SYM *)0, fbuf );
	out_emit( ERx(",") );
	gen__int( eq->eq_line );
	out_emit( ERx(")") );
    }
    /* else uses default parameters */
    return 0;
}


/*
+* Procedure:	gen_sdesc 
** Purpose:	Put out ADA string as an untrimmed C string [concatenated with
**		a Null] or a trimmed C string [inside an IIsd call], or
**		as a VMS/Ada string (by descriptor), for the IIx...io calls.
** Parameters:	sflag 	- i4	- How to send the string.
**		    AstrTRIM:   Trim via IIsd, send as an integer by value.
**		    AstrNOTRIM: Just add a C null - untrimmed, send as a C
**				string by reference.
**		    AstrDESC:   Send as an Ada I/O string.
**		sy	- SYM * - If var this will be var pointer.
**		data	- char* - String to dump.
-* Returns:	None
**
** Notes: 	The following table is used:
**
**			AstrTRIM	    AstrNOTRIM		  AstrDESC
** Vax:		      +-------------------+---------------------+-------------
**	isvar	      |	IIsd(String(var)) | String(var) & II0   | String(var)
**	isconst	      | IIsd("string")	  | "string" & II0      | "string"
** Vads:
**	isvar	      |	?		  |IIsa(String(var)&II0)| String(var)
**	isconst	      | ?		  |IIsa("string" & II0) | "string"
*/

void
gen_sdesc( sflag, sy, data )
i4	sflag;
SYM	*sy;
char	*data;
{
    /*
    ** This is left over from Release 5 and not used anymore.  This will not
    ** work for Vads as there is no IIsd.
    */
    if (sflag == AstrTRIM)
	gen__obj( TRUE, ERx("IIsd(") );

    /* If not an I/O string and Vads then "String to Address" */
    if (sflag != AstrDESC && (eq->eq_config & (EQ_ADVADS|EQ_ADGNAT|EQ_ADALSYS)))
	gen__obj( TRUE, ERx("IIsa(") );

    if (sy == (SYM *)0)
    {
	gen_sconst( data );
    }
    else
    {
	/*
	** Should look at some sym stuff to make sure that we do not
	** need to cast via String( var )
	*/
	gen__obj( TRUE, ERx("String(") );
	gen__obj( TRUE, data );
	gen__obj( TRUE, ERx(")") );
    }
    if (sflag == AstrTRIM)
	out_emit( ERx(")") );
    else if (sflag == AstrNOTRIM)
	gen__obj( TRUE, ERx("&II0") );

    if (   sflag != AstrDESC
	&& (eq->eq_config & (EQ_ADVADS|EQ_ADGNAT|EQ_ADALSYS)))	/* Close IIsa */
	out_emit( ERx(")") );
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
**	17-jul-1987 - Modified for Ada. (mrw)
*/

void
gen_null( argnum )
i4	argnum;
{
    if ((arg_sym(argnum)) == (SYM *)0)		/* No indicator variable? */
    {
	gen__obj( TRUE, ERx("IInull,") );	/* Pass null pointer */
    } else
    {
	gen__obj( TRUE, arg_name(argnum) );
	gen__obj( TRUE, ERx("'Address,") );
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
** If not, try to use ADA '&' concat operator.
*/

void
gen_sconst( str )
char	*str;
{
    i4		len;

    if (str == (char *)0)
	return;
    /* 
    ** Do not break up small pieces of strings over end of line.
    ** The +2 is for two quotes and the +4 is for the "&II0" null terminator 
    ** that may follow (= 6).  The extra 2 spaces are for the new line done
    ** if the first condition is true.
    */
    len = STlength( str );
    if ((len > G_LINELEN - out_cur->o_charcnt - 6) &&
	(len < G_LINELEN - ((i4)STlength(g_indent)) - 8))
    {
	out_emit( ERx("\n") );
	out_emit( g_indent );
	out_emit( ERx("  ") );		/* 2 spaces */
    }
	
    while (gen__sconst(&str))
    {
	out_emit( ERx(" &\n") );	/* Continue literal */
	out_emit( g_indent );
	out_emit( ERx("   ") );
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
**    be implemented here, as is the case in our example, or higher up (see
**    gen_sconst).
** 2. Care should be take not to split lines in the middle of compiler escape
**    sequences - the most complex of which occur in C.
*/

i4
gen__sconst( str )
register char	**str;
{
    register char	*cp;
    register i4	len;
    char		savech;

    out_emit( ERx("\"") );
    /* 
    ** The +2 is for two quotes and the +4 is for the "&II0" null terminator 
    ** that may follow (= 6).
    */
    if (((i4)STlength(*str)) <= (len = G_LINELEN - out_cur->o_charcnt - 6))
    {
	out_emit( *str );
	out_emit( ERx("\"") );
	return 0;
    }
    if (len < 2)		/* Avoid a null string generated by us */
	len = 2;
    for (cp = *str; len > 0 && *cp;)
    {
	/* Try not to break in the middle of compiler escape sequences */
	if (*cp == '"')
	{
	    cp++; 
	    len--;				/* Skip the escaped " */
	}
	cp++; len--;				/* Skip current character */
    }
    if (*cp == '\0')				/* All the way till the end */
    {
	out_emit( *str );
	out_emit( ERx("\"") );
	return 0;
    }
    savech = *cp;
    *cp = '\0';
    out_emit( *str );
    out_emit( ERx("\"") );
    *cp = savech;
    *str = cp;					/* More to go next time */
    return 1;
}

/*
+* Procedure:	gen__int 
** Purpose:	Does local integer conversion.
**
** Parameters: 	
**		num - i4  - Integer to put out.
-* Returns: 	None
*/

void
gen__int( num )
i4	num;
{
    char	buf[20];

    /* All ints are passed by value based on the EQUEL packages */
    CVna( num, buf );
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

    /* All floats are passed by reference based on the EQUEL packages */
    STprintf( buf, ERx("%f"), num, '.' );	/* Standard notation */
    gen__obj( TRUE, buf );
}

/*
+* Procedure:	gen__obj 
** Purpose:	Write an object within the line length. This utility is used for
**	     	everything but string constants.
**
** Parameters:	ind - i4  	- Indentation flag,
**		obj - char *	- Object to dump.
-* Returns:	None
*/

void
gen__obj( ind, obj )
i4		ind;
register char	*obj;
{
    /* May have been just been a newline */
    if (out_cur->o_charcnt == 0 && ind)
    {
	out_emit( g_indent );
    }
    /* The 3 is for unused space once the object is reached */
    else if (((i4)STlength(obj)) > G_LINELEN - out_cur->o_charcnt -3)
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
	else 		/* FATAL Internal Error */
	    er_write( E_EQ0002_eqNOTABLE, EQ_FATAL, 0 );
    } else
	return &gen__ftab[func];
}

/*
+* Procedure:	gen_do_indent 
** Purpose:	Update indentation value.
**
** Parameters:	i - i4  - Update value.
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
+* Procedure:	gen_unsupp 
** Purpose:	Do not implement undocumented function for new language.
**	        Used for FORMDATA, INQUIRE_FRS, SET_FRS and GETOPER
** Parameters:	
**	func - i4  - function index.
**	args - i4  - Argument count (unused).
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
	if (g_block_dangle)
	    er_write( E_EQ0076_grNOWUNSUPP, EQ_ERROR, 1, ERx("FORMDATA") );
	break;
      case IIGETQRY:
      	/* First one after the "if (IIfsetio) then" */
	if (g_block_dangle)
	    er_write( E_EQ0076_grNOWUNSUPP, EQ_ERROR, 1, ERx("GETOPER") );
	break;
      case IIINQSET:
	er_write( E_EQ0077_grISUNSUPP, EQ_ERROR, 1,
						ERx("INQUIRE_FRS/SET_FRS") );
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
    static	char	which_op = 0;		/* Last operator */
    register	char	ch;

    f_loc = flag & ~(G_H_INDENT | G_H_OUTDENT | G_H_NEWLINE);

    if (flag & G_H_OUTDENT)			/* Outdent before data */
	gen_do_indent( -1 );

    switch (f_loc)
    {
      case G_H_KEY:
	if (was_key)
	{
	    if ((*s && !CMoper(s)) && out_cur->o_charcnt != 0)
		out_emit( ERx(" ") );
	} else
	{
	    if (out_cur->o_charcnt)
	    {
		ch = which_op;
		if (ch==';' || ch==')' || ch==':' || ch=='=' || ch==',')
		    out_emit( ERx(" ") );
	    }
	}
	gen__obj( TRUE, s );
	was_key = 1;
	which_op = 0;
	break;

      case G_H_OP:
	if (s)
	    ch = *s;
	else
	    ch = '\0';
	if (was_key)
	{
	    if (ch==':' || ch=='-' || ch=='=' || ch=='(' || ch=='\'' ||
	        !CMoper(s))
	    {
		out_emit( ERx(" ") );
	    }
	} else if (ch== ':' || ch=='=' || (ch=='\'' && which_op==','))
	{
	    out_emit( ERx(" ") );
	}
	/*
	** SIR 4167: ADA lines must be <= 120 chars - this can happen in
	** variable initializations, thus we generate a newline before
	** the := operator if the line is already long enough.
	*/
	if (s && s[0] == ':' && s[1] == '=' && out_cur->o_charcnt >= 40)
	    out_emit(ERx("\n"));
	gen__obj( TRUE, s );
	was_key = 0;
	which_op = ch;
	break;

      case G_H_SCONST:
	if (was_key)
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
	was_key = 0;		/* Newline equivalent to space */
    }
    if (flag & G_H_INDENT)			/* Indent after data */
	gen_do_indent( 1 );
    g_block_dangle = FALSE;
}
