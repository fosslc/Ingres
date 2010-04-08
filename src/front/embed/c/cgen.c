/*
** Copyright (c) 1984, 2005 Ingres Corporation
**
**
** 20-jul-90 (sandyd) 
**	Workaround VAXC optimizer bug, first encountered in VAXC V2.3-024.
NO_OPTIM = vax_vms
*/

# include 	<compat.h>
# include	<cv.h>
# include	<er.h>
# include	<si.h>
# include	<lo.h>
# include 	<cm.h>
# include 	<st.h>
# include 	<me.h>
# include	<eqrun.h>
# include	<equel.h>
# include	<eqsym.h>
# include	<eqgen.h>
# include	<eqc.h>
# include	<ereq.h>

/*
+* Filename:	CGEN 
** Purpose:	Maintains all the routines to generate II calls within specific
**	  	language dependent control flow statements.
**
** Language:	Written for Unix C, but portable for VMS too.
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
**		gen_sym( mask, arg )		- Describe I/O variable.
**		gen_val( arg )			- Describe I/O value data.
**		gen_data( mask, arg )		- Dump different arg data.
**		gen_IIdebug()			- Runtime debug information.
**		gen_null()			- Dump indicator arg.
**		gen__sconst( str )		- Controls splitting strings.
**		gen__int( i )			- Integer.
**		gen__float( i )			- Float.
**		gen__obj( ind, obj )		- Write anything but strings.
**		gen__functab( func )		- Get function table pointer.
**		gen_hdl_args(flags,numargs)	- SET_SQL handler args.
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
** 1. This module will be duplicated across the different Equel languages that
**    are supported.  Some routines will be common across different languages
**    but most will have to be rewritten for each language.  Hopefully, the C
**    template for this file will guide anyone creating such a file for 
**    different languages.  Some languages need to check for an entry into
**    the "alternate" gen table much higher up than does C.  Many routines
**    check against II0 as a "null" function -- never pass in (II0 | GEN_DML)
**    as the check will then fail.
** 2. Some special cases known:
** 2.1. Languages that do not allow Null strings can have their length checked
**      here.
** 2.2. Basic can check to see if the '%' is on the end of integer strings.
**      Internal integer constants will obviously need the '%' tacked on.
** 2.3. Some VMS Cobol statements may use Perform loops, based on the function.
** 2.4. Some Unix Cobol statements may use If blocks based on the function 
**      label.
** 2.5. Some calls require special Pascal strongly typed functions - based on
**      the function and the type, it can be done here.
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
**		29-jun-87 (bab)
**			Code cleanup.  Removed g_host static since unused.
**		20-apr-88 (marge)
**			Added IIFRgpcontrol, IIFRgpsetio, 
**		 	IIFRatimeout, IIFRafield to gen__ftab table
**		  	for Venus support for popups. 
**		13-may-88 (whc)
**			added support for stored procedures to gen__ftab
**			changed nulls to be passed by value.
**		16-may-88 (marge)
**			Removed IITBacol, IIFRafield, IIFRatimeout from 
**			gen__ftab until full implementation time.
**		23-mar-89 (teresa)
**			Added cast to STlength.
**              11-may-1989 (teresa)     
**			Added function calls IITBcaClmAct, IIFRafActFld,
**			IIFRreResEntry to gen__ftab for entry activation.
**		09-aug-1989 (teresa)
**			Add function call IITBceColEnd.
**		16-nov-1989 (teresa)
**			Added function call IIFRInternal which indicates that
**			the '-c' preprocessor flag was used (for internal
**			use only). Note that IIFRInternal has only been added
**			to the code generator for the C preprocessors.
**		13-dec-1989 (barbara)
**			Added the calls IILQesEvStat and IILQegEvGetio for
**			Phoenix/Alerters.
**		04-apr-1990 (barbara)
**			Fixed gen_include regarding suffix comparisons.
**		20-jul-1990 (teresal)
**			Add decimal support.
**		08-aug-1990 (barbara)
**			Added IIFRsaSetAttrio to function table for per value
**			display attributes.
**		27-nov-1990 (barbara)
**			gen_val, which used to generate code to send numeric
**			literals and constants by value to I/O routines, now
**			sends them by reference using two new functions,
**			IILQdbl and IILQint.  See comments in gen_val.
**		15-mar-1991 (kathryn)
**			Added IILQssSetSqlio and IILQisInqSqlio.
**			      IILQshSetHandler and IILQihInqHandler
**			Added: gen_hdl_args().
**		21-mar-1991 (teresal)
**			Put in desktop porting changes.  Cast integer
**			literals to an "int" in generated output code - in 
**			the desktop environment this forces the literals 
**			to be passed as 32 bits because "int" is 
**			defined as "long".
**		04-apr-1991 (teresal)
**			Bug fix 33475 - generate IImessage rather than
**			IISmessage to eliminate problem with '%'
**			getting stripped from FRS MESSAGE statement.
**			"printf" style arguments for C is no longer
**			supported in the MESSAGE statement.
**		17-apr-1991 (teresal)
**			Add support for ACTIVATE EVENT.  Remove 
**			IILQegEvGetio from table of II calls as this routine
**			is now called internally from LIBQ rather than
**			part of the runtime interface.
**		03-dec-91 (leighb) DeskTop Porting Change:
**			Generate "(long)" in front of integers for PMFE
**			and "(int)" in front of integers for all other 
**			platforms.
**		10-jun-92 (leighb) DeskTop Porting Change:
**			Generate "sizeof(x)" when passing the address of "x"
**			instead of an absolute "4".  Do this only when -c
**			flag is NOT used.
**		14-oct-92 (lan)
**			Added IILQled_LoEndData.
**	    Added IILQlgd_LoGetData and IILQlpd_LoPutData and new function
**	    gen_datio().
**	09-nov-92 (lan)
**	    Added IILQldh_LoDataHandler.
**	16-nov-92 (lan)
**	    Removed IILQldh_LoDataHandler entry 159. 
**	20-nov-92 (kathryn)
**	    Corrected typo and add history item:
**	        20-nov-1992: (Mike S)
**	        Added EXEC 4GL support (IIG4...) routines.  
**	2-dec-1992 (Mike S)
**	    Add entry for SEND USEREVENT
**	08-dec-1992 (lan)
**	    Use gen_hdl_args instead gen_all_args to generate the arguments
**	    for IILQldh_LoDataHandler call.  Also removed redundant code in
**	    gen_all_args.
**	10-dec-1992 (kathryn)
**	    Modified gen_datio() to allow for null values.
**	13-feb-93 (teresal)
**	    Put in fix so C line directives will always start on a 
**	    newline. This fix is needed as part of the ESQL/C
**	    scanner changes to allow flexible placement of SQL
**	    statements within C host code.
**	07/28/93 (dkh) - Added support for the PURGETABLE and RESUME
**			 NEXTFIELD/PREVIOUSFIELD statements.
**   	10-sep-1993 (kchin)
**	    For axp_osf, when passing variables by value, shouldn't
**	    do any cast to int as this might truncate PTR or long 
**	    of sizes which are 8 bytes.  Changed in routine gen_data().
**	15-sep-1993 (sandyd)
**	    Added VARBYTE support in gen_sym().
**	17-sep-1993 (kathryn)
**	    Remove IILQihInqHandler fron the function table. This has never
**	    been generated by any of the preprocessors.
**	01-oct-1993 (kathryn)
**	    Changed flag from G_SET to  G_PUT for IILQlpdPutData stmt.
**	    Always generate entire length for fixed length character variables
**          for GET/PUT DATA statements. First argument generated for GET/PUT
**	    DATA statements will now be set/stored by eqgen_tl() rather than
**	    determined by gen_datio, so add one to the number of args in the
**	    gen__ftab for IILQlpd_LoPutData and IILQlgd_LoGetData.
**	20-apr-1994 (teresal)
**	    ANSI C prototypes make casting "(int)" [or "(long)" for PCs]
**	    in front of integers obsolete. Take out unnecessary casting.
**	    Note: the new ESQL/C++ will benefit from this clean up as well.
**	12-mar-1996 (thoda04)
**	    Added function parameter prototypes.
**	2-may-97 (inkdo01)
**	    Added GEN_IIFUNC entry for IILQprvProcGTTparm.
**      21-apr-1999 (hanch04)
**        Replace STrindex with STrchr
**	07-mar-2001 (abbjo03)
**	    Add support for T_WCHAR for Unicode.
**	20-mar-2001 (abbjo03)
**	    In gen_sym, correct the length output for T_WCHAR/T_NVCH variables.
**	2-may-01 (inkdo01)
**	    Added parm to ProcStatus for row procs.
**      10-jul-2001 (stial01)
**          Fixes for T_WCHAR
**	28-mar-2003 (somsa01)
**	    Changed return type of IILQdbl() and IILQint() to "void *".
**	08-dec-2003 (xu$we01)
**	    Made changes to eliminate ESQL/C++ compilation Warnings
**	    (bug 111424).
**      20-jan-2005 (horda03)
**          My attempt to fix the VMS build problem broke all build.
**          Re-worked my original change.
**	01-oct-2005 (toumi01)
**	    In gen__sconst copy string to temporary buffer before forcing
**	    a string delimiter when the line wraps; the parser may have
**	    inserted arguments whose strings live in the literal pool.
**	    This eliminates the need for gcc's "-fwritable-strings".
**	25-oct-2005 (abbjo03)
**	    Change on 1-oct-2005 should have included me.h for MEcopy().
**	17-Jul-2006 (kiria01) b116363
**	    Avoid size evaluation of ints & floats and instead use sizeof()
**	    thereby allowing the compiler to handle sizes based on variable
**	    at the time of compilation instead of pre-compilation. This will
**	    allow code to be pre-compiled on 32bit and the generated code
**	    would be consistant for 64bit too.
**	29-Aug-2006 (gupsh01) 
**	    Fixed previous change resulted in esqlc compiler generating an 
**	    instruction with size as sizeof(double) for a host variable 
**	    of type float, resulting in incorrect results. Bug 116569.
**	12-Sep-2006 (kiria01) b116363
**	    Added back the rest of b116363 fix without the 'double' error.
**	18-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/


/* Max line length for C code - with available space for closing calls, etc */

# define	G_LINELEN	75

/* 
** Max size of converted string - allow some extra space over scanner for
** extra escaped characters required.
*/

# define 	G_MAXSCONST	300	

/* States for host language data being dumped from the grammar */

# define	G_OP		0
# define	G_KEY		1

/* 
** Indentation information - 
**     Each indentation is 2 spaces.  For C the indentation is made up of 
** spaces. For languages where margins are significant, and continuation of 
** lines must be explicitly flagged (Fortran) the indentation mechanism will 
** be more complex - Always start out at the correct margin, and indent from
** there. If it is a continuation, then flag it in the correct margin, and 
** then indent. In C g_indent[0-1] are always blanks.
*/

# define 	G_MAXINDENT	20

static		char	g_indent[ G_MAXINDENT + G_MAXINDENT ] =
		{
		  ' ', ' ','\0', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '  
		};

static	i4	g_indlevel = 1;		/* Start out 2 spaces from margin */


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
			ERx("!="),   			/* C_NOTEQ */
			(char *)0,			/* C_0 */
			ERx("=="),			/* C_EQ */
			ERx(">"),			/* C_GTR */
			ERx("<")			/* C_LESS */
		};

/* 
** Table of label names - correspond to L_xxx constants.
** Must be unique in the first few characters, to allow label uniqueness
** in 8 characters.
*/

static	 char	*gen_ltable[] = {
			ERx("IIl_"),		/* Should never be referenced */
			ERx("IIfdI"),
			ERx("IIfdB"),
			ERx("IIfdF"),
			ERx("IIfdE"),
			ERx("IImuI"),
			(char *)0,
			(char *)0,
			ERx("IIrepBeg"),
			ERx("IIrepEnd"),
			ERx("IIrtB"),
			ERx("IIrtF"),
			ERx("IIrtE"),
			ERx("IItbB"),
			ERx("IItbE"),
			(char *)0
		};

/* 
** Local goto, label, call and argument dumping calls -
** Should not be called from the outside, even though they are not static.
*/
void	gen__goto(i4 labprefix, i4  labnum);
void	gen__label(i4 ind, i4  labprefix, i4  labnum);
i4  	gen__II(i4 func);
i4  	gen_all_args(i4 flags, i4  numargs);
i4  	gen_var_args(i4 flags, i4  numargs);
i4  	gen_io(i4 ret_set, i4  args);
i4  	gen_sym(i4 varmask, i4  arg);
i4  	gen_val(i4 arg);
void	gen_data(i4 varmask, i4  arg);
i4  	gen__sconst(char **str);
i4  	gen_IIdebug(void);
void	gen_null(i4 argnum);
void	gen__int(i4 num);
void	gen__float(f8 num);
void	gen__obj(i4 ind, char *obj);
i4  	gen_hdl_args(i4 flags, i4  numargs);
i4  	gen_datio(i4 ret_set, i4  args);
/*
** Table of II calls - 
**     Correspond to II constants, the function to call to generate arguments, 
** the flags needed and the number of arguments.  For C there is very little
** argument information needed, as will probably be the case for regular 
** callable languages. For others (Cobol) this table will need to be more 
** complex and the code may be split up into partially table driven, and 
** partially special cased.
**
** ** VERY IMPORTANT: When you add a new II call, you need to add this
**		      function to the following files:
**
**	ESQL C/C++ include files (these files are included in customer's code)
**	========================
**	frontcl!specials!eqdef.pp	! Old-style C function declarations
**	frontcl!specials!eqdefc.pp	! Prototype function declaractions
**	frontcl!files!eqpname.h		! Rename II call to prototyped name
**
**	ESQL C runtime layer
**	====================
**	frontcl!libqsys!iiaclibq.c	! LIBQ C runtime layer for prototypes. 
**	frontcl!libqsys!iiaplibq.h	! Header file for iiaclibq.c.
**	or
**	frontcl!runsys!iiacfrs.c	! FRS C runtime layer for prototypes.
**	frontcl!runsys!iiapfrs.h	! Header file for iiacfrs.c
**
**	VMS shared libraries
**	=====================
**	frontcl!sharesys!libq.mar	! LIBQ calls for VMS transfer vector
**	or
**	frontcl!sharesys!frame.mar	! FRS calls for VMS transfer vector
*/

GEN_IIFUNC	*gen__functab(i4 func);

GLOBALDEF GEN_IIFUNC	gen__ftab[] = {
/*
** Index	Name		Function	Flags		Args
** -----------  --------------	--------------	-------------	-----
*/

/* 000 */    {ERx("IIf_"),		0,		0,		0},

		    /* Quel # 1 */
/* 001 */    {ERx("IIflush"),		gen_IIdebug,	0,		0},
/* 002 */    {ERx("IInexec"),		0,		0,		0},
/* 003 */    {ERx("IInextget"),		0,		0,		0},
/* 004 */    {ERx("IIputdomio"),	gen_io,		G_SET,		2},
/* 005 */    {ERx("IIparret"),		gen_all_args,	0, /* raw */	2},
/* 006 */    {ERx("IIparset"),		gen_all_args,	0, /* raw */	2},
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
/* 018 */    {ERx("IILQshSetHandler"),  gen_hdl_args,   G_SET, 		2},
/* 019 */    {(char *)0,                0,              0,              0},
/* 020 */    {(char *)0,		0,		0,		0},

		    /* Libq # 21 */
/* 021 */    {ERx("IIbreak"),		0,		0,		0},
/* 022 */    {ERx("IIeqiqio"),		gen_io,		G_RET,		3},
/* 023 */    {ERx("IIeqstio"),		gen_io,		G_SET,		3},
/* 024 */    {ERx("IIerrtest"),		0,		0,		0},
/* 025 */    {ERx("IIexit"),		0,		0,		0},
/* 026 */    {ERx("IIingopen"),		gen_var_args,	0,		0},
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
/* 045 */    {ERx("IIfmdatio"),		gen_io,		G_RET,		3},
/* 046 */    {ERx("IIfldclear"),	gen_all_args,	0,		1},
/* 047 */    {ERx("IIfnames"),		0,		0,		0},
/* 048 */    {ERx("IIforminit"),	gen_all_args,	0,		1},
/* 049 */    {ERx("IIforms"),		gen_all_args,	0,		1},
/* 050 */    {ERx("IIfsinqio"),		gen_io,		G_RET,		3},
/* 051 */    {ERx("IIfssetio"),		gen_io,		G_SET,		3},
/* 052 */    {ERx("IIfsetio"),		gen_all_args,	0,		1},
/* 053 */    {ERx("IIgetoper"),		gen_all_args,	0,		1},
/* 054 */    {ERx("IIgtqryio"),		gen_io,		G_RET,		3},
/* 055 */    {ERx("IIhelpfile"),	gen_all_args,	0,		2},
/* 056 */    {ERx("IIinitmu"),		0,		0,		0},
/* 057 */    {ERx("IIinqset"),		gen_var_args,	0,		0},
/* 058 */    {ERx("IImessage"),		gen_all_args,	0,		1},
/* 059 */    {ERx("IImuonly"),		0,		0,		0},
/* 060 */    {ERx("IIputoper"),		gen_all_args,	0,		1},
/* 061 */    {ERx("IIredisp"),		0,		0,		0},
/* 062 */    {ERx("IIrescol"),		gen_all_args,	0,		2},
/* 063 */    {ERx("IIresfld"),		gen_all_args,	0,		1},
/* 064 */    {ERx("IIresnext"),		0,		0,		0},
/* 065 */    {ERx("IIgetfldio"),	gen_io,		G_RET,		3},
/* 066 */    {ERx("IIretval"),		0,		0,		0},
/* 067 */    {ERx("IIrf_param"),	gen_all_args,	0, /* raw */	2},
/* 068 */    {ERx("IIrunform"),		0,		0,		0},
/* 069 */    {ERx("IIrunmu"),		gen_all_args,	0,		1},
/* 070 */    {ERx("IIputfldio"),	gen_io,		G_SET,		3},
/* 071 */    {ERx("IIsf_param"),	gen_all_args,	0, /* raw */	2},
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
/* 101 */    {ERx("IItrc_param"),	gen_all_args,	0, /* raw */	2},
/* 102 */    {ERx("IItsc_param"),	gen_all_args,	0, /* raw */	2},
/* 103 */    {ERx("IItscroll"),		gen_all_args,	0,		2},
/* 104 */    {ERx("IItunend"),		0,		0,		0},
/* 105 */    {ERx("IItunload"),		0,		0,		0},
/* 106 */    {ERx("IItvalrow"),		0,		0,		0},
/* 107 */    {ERx("IItvalval"),		gen_all_args,	0,		1},
/* 108 */    {ERx("IITBceColEnd"),	0,		0,		0},
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
/* 128 */    {ERx("IIcsParGet"),	gen_all_args,	0, /* raw */	2},
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
/* 138 */    {ERx("IIFRInternal"), 	gen_all_args,	0,		1},
/* 139 */    {ERx("IIFRsaSetAttrio"),	gen_io,		G_SET,		4},
/* 140 */    {ERx("IIFRaeAlerterEvent"),gen_all_args,	0,		1},
/* 141 */    {ERx("IIFRgotofld"),	gen_all_args,	0,		1},
/* 142 */    {(char *)0,		0,		0,		0},
/* 143 */    {(char *)0,		0,		0,		0},
/* 144 */    {(char *)0,		0,		0,		0},

		/* PROCEDURE support #145 */
/* 145 */    {ERx("IIputctrl"),		gen_all_args,	0,		1},
/* 146 */    {ERx("IILQpriProcInit"),	gen_all_args,	0,		2},
/* 147 */    {ERx("IILQprvProcValio"),	gen_io,		G_SET,		4},
/* 148 */    {ERx("IILQprsProcStatus"),	gen_all_args,	0,		1},
/* 149 */    {ERx("IILQprvProcGTTParm"),gen_all_args,	0,		2},
/* 150 */    {(char *)0,		0,		0,		0},

		/* EVENT support #151 */
/* 151 */    {(char *)0,		0,		0,		0},
/* 152 */    {ERx("IILQesEvStat"),      gen_all_args,   0,              2},
/* 153 */    {(char *)0,		0,		0,		0},
/* 154 */    {(char *)0,		0,		0,		0},

		/* LARGE OBJECTS support #155 */
/* 155 */    {ERx("IILQldh_LoDataHandler"),gen_hdl_args,G_DAT,		4},
/* 156 */    {ERx("IILQlgd_LoGetData"), gen_datio,      G_RETVAR,       5},
/* 157 */    {ERx("IILQlpd_LoPutData"), gen_datio,      G_PUT,          4},
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
    register	i4	ret_val = 1;

    /*
    ** The while loop is relevant only for 'multi-calls' - none used anymore,
    ** but used to be for old gen_IIwrite.
    */
    while (ret_val)
    {
	/* gen__obj( TRUE, (char *)0 ); */
	ret_val = gen__II( func );
	out_emit( ERx(";\n") );
    }
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
** Format: 	(open) 	if (func(args) condition val) then begin
** 	   	(close) end if
**
** The function constant is passed when opening and closing the If statement, 
** just in case some language will not allow If blocks.  This problem already 
** exists on Unix Cobol, where some If blocks are not allowed.  Don't forget
-* to look for the GEN_DML bit in "func" first!
**
** Labelled language (that do not want to use If blocks - Unix Cobol) note:
** 1. Based on the function constant passed, one may want to use a real If
**    or not.
** 2. If we do Equel/Assembly then we can locally keep track of If branch
**    labels.  On every If block increment it, then generate a branch to it on
**    the Not condition. One must have a local counter to keep track of the
**    'high-water' label number for every new If block, and a stack for the
**    label numbers to generate on exit from the If block.
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
	gen__int( val );					/* Value */
	gen__obj( TRUE, ERx(") ") );				/* Then */
	gen__obj( TRUE, ERx("{\n") );				/* Begin */
	gen_do_indent( 1 );

	/* Next call should Close or Else */
    } else if (o_c == G_CLOSE)
    {
	gen_do_indent( -1 );
	gen__obj( TRUE, ERx("} ") );				/* End */
	gen_comment( TRUE, gen__functab(func)->g_IIname );
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
** Format: 	(open) 	if (func(args) condition val) then begin
**	   	(else)	end else
-* 	   	(close) end if
**
** Labelled Format: 
** Format: (open) 	if (func(args) not condition val) goto lab_else;
**
**	   (else)	   goto lab_endif;
**	        lab_else:
** 	   (close)
**	        lab_endif:
**
** The function constant and labels are passed for labelled languages that
** cannot use If-Then-Else blocks in some cases.  This is currently used
** only by the Activate lists, that generate an If-Then-Else format for
** each interrupt value.  The Else label prefix is also a special case, as
** one must make the label unique, because there can be nested If-Then-Else 
** formats: for example a nested menu inside an Activate block.  This problem 
** already exists in Cobol, where the If-Then-Else of a Display loop is 
** managed by labels and gotos.
**
** Labelled language (without If-Then-Else blocks) note:
** 1. To Open an If call the function. Test the result using the Not condition 
**    (negative) against the passed value. If True (Not condition) then goto 
**    the Else label.
** 2. To Else an If generate a goto to the Endif label.  Then using the Else
**    label prefix, and both the Endif label number and Else label number,
**    generate an Else label. The label head for this should know that it must
**    include two labels - if STprintf format then it may have "Lab%d_Else%d".
** 3. To Close an If, dump the closing label.
** 4. Based on the function constant passed, one may want to use a real Else
**    or not.
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
    if (flag == G_OPEN && func == II0)		/* Last Else block */
    {
	gen__obj( TRUE, ERx("{\n") );
	gen_do_indent( 1 );
    } else if (flag == G_ELSE)
    {
	gen_do_indent( -1 );
	gen__obj( TRUE, ERx("} else ") );	/* Followed by an If or II0 */
    } else
	gen_if( flag, func, cond, val );	/* Either Open or Close */
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
-* Format:	if (func(args) condition val) goto label;
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
    gen__int( val );					/* Value */
    gen__obj( TRUE, ERx(") ") );			/* Then */
    gen__goto( labprefix, labnum );			/* Goto Label */
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
** Format: 	(open)	while (func(args) condition val) do begin
-*	   	(close)	end do
**
** Labelled Format: 
**	   (open)   lab_begin:
**			if (func(args) not condition val) goto lab_end;
**	   (close)  	goto lab_begin;
**		    lab_end:
**
** Labelled language note:
** 1. To open a loop, dump the openning label.  Call the function. Test the
**    result using the Not (negative) condition against the passed value. If
**    True then goto the closing label.
** 2. To close a loop, dump a goto to the openning label.  Now dump the
**    closing label.
** 3. Based on the function used one may want to use a real while.
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
	gen__int( val );					/* Value */
	gen__obj( TRUE, ERx(") {\n") );				/* Do Begin */
	g_loop++;
	gen_do_indent( 1 );
    } else
    {
	if (g_loop > 0)
	{
	    gen_do_indent( -1 );
	    gen__obj( TRUE, ERx("} ") );
	    gen_comment( TRUE, gen__functab(func)->g_IIname );
	    g_loop--;
	}
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
** Format:	[ if (1) ] goto label;
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
	gen__obj( TRUE, ERx("if (1) ") );
    gen__goto( labprefix, labnum );
}

/*
+* Procedure:	gen__goto 
** Purpose:	Local statement independent goto generator.  
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
    out_emit( ERx(";\n") );
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
-* Format:	label:
**
** Note: Some labels are never followed by a statement or are only sometimes
**       followed by a statement.   Most C compilers complain about a 
** closing '}' without a preceding ';'.  To avoid warnings we force a ';' after
** these labels.
*/

void
gen_label( loop_extra, labprefix, labnum )
i4	loop_extra;
i4	labprefix;
i4	labnum;
{
    /* Labels must always begin in the left margin */
    gen__label( FALSE, labprefix, labnum );
    if (labprefix==L_RETEND || labprefix==L_FDEND || labprefix==L_FDINIT)
	out_emit( ERx(":;\n") );		/* No following statement */
    else
	out_emit( ERx(":\n") );
}

/*
+* Procedure:	gen__label 
** Purpose:	Local statement independent label generator.  
**
** Parameters:	ind	  - i4  - Indentation flag,
**		labprefix - i4  - Label prefix,
**		labnum    - i4  - Label number.
-* Returns:	None
**
** Some languages may require more complex evaluations of labels.
*/

void
gen__label( ind, labprefix, labnum )
i4	ind;
i4	labprefix;
i4	labnum;
{
    char	buf[20];

    gen__obj( ind, gen_ltable[labprefix] );			/* Label */
    /* Label prefixes and their numbers must be atomic */
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
-* Returns: 	None
*/

void
gen_comment( term, str )
i4	term;
char	*str;
{
    gen__obj( TRUE, ERx("/* ") );
    gen__obj( TRUE, str );
    gen__obj( TRUE, ERx(" */") );
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
** Notes:	1.  If the "-c" flag was given on the command line and the
**		filename begins with a '<', then we assume that this is
**		a global include that already has the angle brackets
**		on it, so we don't add the quotes.
**	  	2.  If the "-c" flag was given on the command line and
**		the filename is suffixed with ".sh" (or "hs"), assume that it's
**		an include file that will also be preprocessed as an ESQL
**		header file (ie a double preprocessed header), and emit:
**		    EXEC SQL include 'filename.sh'; -- or (".hs")
**		We also assume that since the "-c" flag is used, the whole
**		filename will be in "pre" and there won't be any extra
**		extensions (it's an internal header file).
**
** History:
**	04-apr-1990	(barbara)
**	    Yet another fix to make comparison of suffixes work correctly.
**	    We need to compare stuff in suffix (of unknown length) with
**	    a known suffix.  Have to call STbcompare with both lengths of 0
**	    otherwise it does prefix comparison (and will find string "h"
**	    equal to string "hs".
*/

void
gen_include( pre, suff, extra )
char	*pre;
char	*suff;
char	*extra;
{
    i4		is_global_include;
    char	*suffix;
    char	savech;

    /* The whole include line should be atomic */

    if (eq->eq_flags & EQ_COMPATLIB) 
    {
	/*
	** Test if this is to be double preprocessed - "hs" is also allowed
	** to avoid conflicts with "sh" shell files.
	*/
	suffix = STrchr( pre, '.');
	if (suffix)
	{
	    CMnext(suffix);
	    savech = suffix[2];
	    suffix[2] = '\0';	/* Suffix may be '.sh>' */
	    if (   STbcompare(suffix, 0, ERx("sh"), 0, TRUE) == 0
		|| STbcompare(suffix, 0, ERx("hs"), 0, TRUE) == 0
	       )
	    {
		suffix[2] = savech;
		out_emit( ERx("EXEC SQL INCLUDE ") );
		if (*pre != '<')
		    out_emit( ERx("'") );
		out_emit( pre );
		if (*pre != '<')
		    out_emit( ERx("'") );
		out_emit( ERx(";\n") );
		return;
	    }
	    suffix[2] = savech;
	}
	/* else not a double preprocessed file; fall through */
    } 
# ifdef IBM370
    if (				  *pre == '<')
# else
    if ((eq->eq_flags & EQ_COMPATLIB) && (*pre == '<')) 
# endif /* IBM370 */
    {
	is_global_include = TRUE;
	out_emit( ERx("# include ") );
    } else
    {
	is_global_include = FALSE;
	out_emit( ERx("# include \"") );
    }

    out_emit( pre );
    if (suff != (char *)0)
    {
	out_emit( ERx(".") );
	out_emit( suff );
    }
    if (extra != (char *)0)
	out_emit( extra );
    if (is_global_include)
	out_emit( ERx("\n") );
    else
	out_emit( ERx("\"\n") );
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
	gen__obj( TRUE, ERx("{\n") );			/* Begin */
	gen_do_indent( 1 );
    } else
    {
	gen_do_indent( -1 );
	gen__obj( TRUE, ERx("}") );				/* End */
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
    bool	is_line = (bool)(eq->eq_flags & EQ_SETLINE);

    /* Make sure line directive or comment always starts on a newline */
    if (out_cur->o_lastch != '\n' && out_cur->o_lastch != '\0')
	out_emit( ERx("\n") );
    /* # line directive, others may use a comment instead */
    if (!is_line)     		/* comment out #line directive */
	gen__obj( FALSE, ERx("/* ") );
    /* If # line used then don't put tab after fname; may goto unix 'error' */
    STprintf( fbuf, ERx("# line %d \""), eq->eq_line ? eq->eq_line : 1 );
    STcat( fbuf, eq->eq_filename );
    STcat( fbuf, eq->eq_infile == stdin ? ERx("") : ERx(".") );
    STcat( fbuf, eq->eq_ext );
    STcat( fbuf, is_line ? ERx("\"\t") : ERx("\" */\t") );
    gen__obj( FALSE, fbuf );
    if (s)
	gen_comment( FALSE, s );
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
**     VMS - continue.			Unix - move IIcharX to IIcharX.
*/

void
gen_term()
{
}

/*
+* Procedure:	gen_declare 
** Purpose:	Process a ## declare statement.
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
}

/*
+* Procedure:	gen_makesconst 
** Purpose:	Make a host language string constant.
**
** Parameters:	flag  - i4      - G_INSTRING  - Nested in another string (DB 
**					       string),
**		        	 G_REGSTRING - String sent by itself in a call.
**		instr - char * - Incoming string.
**		diff  - i4  *   - Mark if there were changes made to string.
-* Returns:	char *         - Pointer to static converted string.
**
** Notes:
** 1. Special Characters.
** 1.1. Process the '\' character, which is normally used to go to the 
**      database, but in C may be used to escape certain sequences.
** 1.2. Process the host language quote embedded in a string, whether it got
**      there by escaping it with a '\' or by doubling it.
** 2. Routine was written to show the parallel checking done for two very 
**    different languages such as C or Cobol.
** History:
**	17-apr-1987 -   Generate single ticks as the outside delimiters for
**			nested ESQL strings.
*/

char	*
gen_makesconst( flag, instr, diff )
i4	flag;
char	*instr;
i4	*diff;
{
    static	char	obuf[ G_MAXSCONST + 4 ];	/* Converted string */
    register	char	*icp;				/* To input */
    register	char	*ocp = obuf;			/* To output */
    register	i4	esc = 0;			/* \ was used */

    *diff = 0;

    if (flag == G_INSTRING)		/* Escape the first quote */
    {
	*ocp++ = '\\';
	*ocp++ = '"';
	*diff = 1;
    }
    for (icp = instr; *icp && (ocp < &obuf[G_MAXSCONST]);)
    {
	/* Escape in the string */
	if (*icp == '\\')
	{
	    if (flag == G_INSTRING)	/* Double for compiler then for Db */
	    {
		*ocp++ = '\\';
		*diff = 1;
	    }
	    *ocp++ = '\\';
	    esc = 1;
	    icp++;
	} else if (*icp == '"')
	{
	    if (!esc)			/* Somehow got in, act like escaped */
	    {
		if (flag == G_INSTRING)
		    *ocp++ = '\\';
		*ocp++ = '\\';
		*diff = 1;
	    }
	    if (flag == G_INSTRING)	/* Escape it if nested */
	    {
		*ocp++ = '\\';
		*diff = 1;
	    }
	    *ocp++ = '"';
	    esc = 0;
	    icp++;
	}else
	{
	    CMcpyinc(icp, ocp);
	    esc = 0;
	}
    }
    if (flag == G_INSTRING)		/* Escape the last quote */
    {
	*ocp++ = '\\';
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
** Returns: 	0 / 1 - As passed back from lower level calls.
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
    out_emit( ERx("(") );
    ret_val = 0;
    if (g->g_func)
	ret_val = (*g->g_func)( g->g_flags, g->g_numargs );
    out_emit( ERx(")") );
    arg_free();
    return ret_val;
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
i4	flags;
i4	numargs;
{
    register	i4 i;

    for (i = 1; i <= numargs; i++)	/* Fixed number of arguments */
    {
	gen_data( flags, i );
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
** Parameters: 	flags   - i4  - Flags and Number of arguments required.
**		numargs - i4  
** Returns: 	0
**
** As gen_all_args() this routine should be used when no validation of 
-* the number of arguments needs to be done.
*/

i4
gen_var_args( flags, numargs )
i4	flags;
i4	numargs;			/* Unused */
{
    register	i4 i;
    register    i4  arg_count = 0;

    while (i = arg_next())		/* Varying num of arguments */
    {
	gen_data( flags, i );
	gen__obj( TRUE, ERx(",") );
	++arg_count;
    }
    /*
    ** Desktop Porting change:
    ** calls to functions with a variable number
    ** of arguments must be padded out to the full
    ** complement of arguments.  This insures that
    ** no reference is made to objects outside the
    ** stack segment. (steveh Feb 1990)
    */
    while (arg_count++ < numargs-1)
    {
	gen__obj( TRUE, ERx("(char *)0, ") );
    }

    /* terminate the argument list */

    gen__obj( TRUE, ERx("(char *)0") );
    return 0;
}

/*
+* Procedure:	gen_io 
** Purpose:	Generate an I/O variable to the output file.
**
** Parameters: 	ret_set - i4  - Return (G_RET[G_RETSET]) or Set (G_SET) mask,
**		numargs - i4  - Number of arguments,
-* Returns: 	0
**
** Note:
**  1.  There will always be a minimum of two arguments: the null
**	indicator and the user variable.  If there are only 2 arguments then 
**	the argument dumping is the same for G_SET and G_RET:
**		IIset/retfunc( &ind_var, var_desc )
**  2.  If there are are 3 or more arguments they we rely on the following 
**	template format:
**		IIretfunc( &ind_var, var_desc, "objects" );
**		IIsetfunc( "objects", ind_var, var_or_val_desc );
**	where var_desc is:
**		ISVAR, TYPE, LEN, var_or_val
**  3.  Based on the mask we call gen_null for the indicator argument,
**	gen_sym for the I/O variable/descriptor, and all other
**	arguments we just dump.
*/

i4
gen_io( ret_set, args )
register i4	ret_set;
register i4	args;
{
    register i4	ar;

    if (ret_set == G_RET) 
    {
	gen_null( 1 );
	_VOID_ gen_sym( G_RET, 2 );
	for (ar = 3; ar <= args; ar++)
	{
	    gen__obj( TRUE, ERx(",") );
	    gen_data( 0, ar );
	}
    } else	/* G_SET or G_RETSET */
    {
	for (ar = 1; ar <= args -2; ar++)
	{
	    gen_data( 0, ar );
	    gen__obj( TRUE, ERx(",") );
	}
	gen_null( args -1 );
	_VOID_ gen_sym( ret_set, args );
    }
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
**	15-mar-1991 - (kathryn) Written.
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
			gen__obj( TRUE, ERx("(int (*)())0") );
		else
		{
		    gen__obj( TRUE, ERx("0"));
    		    er_write(E_EQ0090_HDL_TYPE_INVALID, EQ_ERROR, 1, buf);
		}
    		break;
        case  ARG_RAW:
    		gen__obj( TRUE, buf);
    	        break;
        default:
		gen__obj( TRUE, ERx("0"));
    		er_write(E_EQ0090_HDL_TYPE_INVALID, EQ_ERROR, 1, buf);
		break;
    }
    arg++;
    if (flags == G_DAT)
    {
    	gen__obj( TRUE, ERx(",") );
    	STprintf(argbuf,ERx("%s"),arg_name(arg));
    	if (STcompare(argbuf,ERx("0")) == 0)
		gen__obj( TRUE, ERx("(char *)0") );
	else
    		gen__obj( TRUE, argbuf);
    }
    return 0;
}

/*{
+*  Name:  gen_datio - Generate arguments for GET DATA and PUT DATA statements.
**
**  Description:
**	Generates one I/O variable followed by remaining integer arguments. 
**	Calls gen_sym to generate one I/O variable  and gen_data to dump
**	the remaining argments.  It is assumed that all arguments have
**	been previously validated for datatype.  The I/O variable is
**	always passed by reference.  There are two possible ret_set values
**	G_RETVAR (GET DATA) and G_PUT (PUT DATA), which determine how the
**	the remaining arguments will be passed. Note that for the GET/PUT
**	DATA statements all arguments are optional. The flags argument is
**	a bit mask set/stored by eqgen_tl() and indicates which attributes were
**	specified: II_DATSEG if segment attribute was given and II_DATLEN if
**	maxlength was specified in the GET DATA stmt or segmentlength was
**	specified for the PUT DATA stmt. The flags mask is the last argument 
**	stored and the first parameter generated.
**
**    Generates:
**        G_RETVAR:
**	       args: (flags, type, len, io_var,  maxlen, &seglength, &dataend)
**          allnull: ( 0,     0,    0, (char *)0,  0,    (long *)0,  (long *)0)
**
**	  G_PUT: 
**	       args: (flags, type, len, io_var,  seglength, dataend)
**	    allnull: (  0,     0,   0, (char *)0,    0,        0   )
**
**        Argument  type 	
**	  ==============
**	  flags  :  i4		mask indicating attributes specified.
**	  type   :  int   	data type of I/O variable.
**	  io_var :  PTR		address of I/O var to get/put the data value.
** 	G_RETVAR
**        maxlen :  int         G_RETVAR only - passed by value.
**	  seglen :  (long *)	address of variable to retrieve integer.
**	  dataend:  (long *)	address of variable to retrieve integer.
**	G_PUT    
**	  seglen :  int		integer value passed to Ingres runtime system.
**	  dataend:  int		integer value passed to Ingres runtime system.
**
**  Inputs:
**      ret_set   - G_RETVAR - GET DATA - pass non-I/0 vars by reference
**		    G_SET    - PUT DATA - pass non-I/O vars by value.	
**      args      - i4  - Number of arguments required.
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
**      01-nov-1992 - (kathryn) Written.
**	10-dec-1992   (kathryn)
**	    Modified to allow for null arguments.
**	14-dec-1992 (kathryn)
**	    When string literal has been specified, the length must be
**	    generated.
**	01-oct-1993 (kathryn)
**	    First arg generated in GET/PUT data stmts is the last arg stored.
**	    It is set/stored in eqgen_tl(), rather than determined here.  
*/

i4
gen_datio( ret_set, args )
register i4     ret_set;
register i4     args;
{
    register i4         ar;
    i4			num;
    i4			len = 0;

    /* last arg generated first - flags mask indicating attributes specified */
    arg_num( args, ARG_INT, &num );
    gen__int( num );
    gen__obj( TRUE, ERx(",") );

    ar = 1;
    if (!arg_name(ar))
    {
        gen__obj( TRUE, ERx("0,0,") );
        gen__obj( TRUE, ERx("(char *)0"));
    }
    else if (((arg_sym(ar)) == (SYM *)0) && (arg_type(ar) == ARG_CHAR))
    {
	gen__int( T_CHAR );
	gen__obj( TRUE, ERx(",") );

	len = STlength(arg_name(ar));
	gen__int(len);
	gen__obj( TRUE, ERx(",") );

	gen_data( 0, ar );
    }
    else
	_VOID_ gen_sym( ret_set, ar);
    for (ar = 2; ar < args; ar++)
    {
        gen__obj( TRUE, ERx(",") );
        if (!arg_name(ar))
        {
           if (ret_set ==  G_RETVAR && (ar != 2))
               gen__obj( TRUE, ERx("(long *)0") );
           else
               gen__int(0);
        }
        else if (ret_set == G_RETVAR && (ar != 2))
        {
            gen_data(ret_set,ar);
        }
        else
            gen_data( 0, ar );

    }
    return 0;
}

/*
+* Procedure:	gen_sym 
** Purpose:	Generate a variable to the output file.
**
** Parameters: 	varmask - i4  - Variable mask for argument - passing mechanism 
**			       and more,
**		arg 	- i4  - Argument (with symbol table entry).
** Returns: 	0
**
** Whatever is flagged in 'varmask' describe to output. The code tries to
** get around the problem of sending data by value or by reference.  The 
** standard I/O rule is that if there is a variable then always send it
** by reference, otherwise send it by value.  This is even the case for
** routines that want input.  Mostly used for Equel I/O functions.
**
-* Dumping order is: varname, vartype and varlength.
**
** History:
**	01-nov-1992 (kathryn)
**	    Added RETVAR check to generate (char *) for NULL program variables
**	    rather than generating an int(0).
**	01-sep-1993 (sandyd)
**	    Added T_VBYTE case to allow code generation for VARBYTEs in
**	    basically the same way as VARCHARs.
**	23-dec-1993 (teresal)
**	    Add support for the "-check_eos" flag. When generating i/o
**	    calls for character ESQL/C host variables, pass the length
**	    of the buffer. This is required so at runtime we know how
**	    long the string is suppose to be when checking for a null
**	    terminator.
**	24-jul-1996 (kch)
**	    Add PUT and RETVAR check when adding buffer dimension string - we
**	    do not want to substract 1 from or add 2 to the length of the
**	    buffer.
**	20-mar-2001 (abbjo03)
**	    Correct the length output for T_WCHAR and T_NVCH variables.
*/

i4
gen_sym( varmask, arg )
register i4	varmask;
register i4	arg;
{
    register SYM	*var;
    i4			dattyp, datlen;
    C_VALS		*cv;
    char		*dimstr = (char *)0;	/* Dimension string */
    i4			genamp;			/* Generate an '&' before var */
    char		*aname;

    /* Check if no user variable */
    if ((var = arg_sym(arg)) == (SYM *)0)
    {
	/* Try to send it by value */
	if (varmask == G_SET  || varmask == G_RETVAR || varmask == G_PUT) 
	    gen_val( arg );
	else
	    gen__int( 0 );
	return 0;
    }

    /* 
    ** There is a symbol table entry.  Check if it is a constant and
    ** if the mask allows such constants.
    */
    if (sym_g_useof(var) & syFisCONST) 
    {
	/* Try to send it by value */
	if (varmask == G_SET || varmask == G_PUT)
	    gen_val( arg );
	else
	    gen__int( 0 );
	return 0;
    } else if ((sym_g_useof(var) & syFisVAR) == 0)
	return 0;

    /* 
    ** At some point we might want to send integers by value.
    ** By using a switch statement here, we're ready for that, and
    ** we can move the "isvar" stuff down into the case statement.
    */
    /* Dump 'Is a var' flag for runtime, unless NULL constant */
    if (varmask != G_RETVAR && varmask != G_PUT)
    {
        gen__int( (sym_g_btype(var) == T_NUL) ? FALSE : TRUE );
        gen__obj( TRUE, ERx(",") );
    }
    switch (sym_g_btype(var)) {
      case T_NUL:
	dattyp = -T_NUL;
	datlen = 1;
	aname = ERx("0");
	genamp = FALSE;
	break;
      case T_BYTE:
	dattyp = T_INT;
	datlen = sizeof(i1);
	aname = arg_name(arg);
	genamp = TRUE;
	break;
      case T_CHAR:
      case T_VCH:
      case T_VBYTE:
	dattyp = sym_g_btype(var);
	datlen = sym_g_dsize(var);
	/*
	** If we need to check for EOS (i.e., the ANSI SQL2 '-check_eos'
	** preprocessor flag was given), then get the length of the
	** character buffer so we know how big a string to check at runtime.
	**
	** Notes: 
	**	o Later on we make the length negative so we can distinguish 
	**	  at runtime a character variable length vs. a #define 
	**	  string length. We don't want/need to check a #define 
	**	  string length. (Yuck, the things FIPS makes one do!!) 
	**	o We only check character type variables when sending
	**	  data to INGRES.
	*/
	if (   varmask == G_RET || varmask == G_RETSET 
	    || varmask == G_PUT || varmask == G_RETVAR
	    || ((esq_geneos() == OK)	   && 
		(datlen == 0)   	   &&
		(varmask == G_SET)	   && 
		(sym_g_btype(var) == T_CHAR)))
	{
	    /* Get buffer dimension string */
	    cv = (C_VALS *)sym_g_vlue(var);
	    if (cv && cv->cv_flags == CvalDIMS && cv->cv_dims[0] != '\0')
		dimstr = cv->cv_dims;
	}
	aname = arg_name(arg);
	/* & needed before varchars and varbytes but not chars */
	genamp = dattyp == T_CHAR ? FALSE : TRUE;
	break;
      case T_WCHAR:
      case T_NVCH:
	dattyp = sym_g_btype(var);
	datlen = sym_g_dsize(var);
	if (varmask == G_RET || varmask == G_RETSET || varmask == G_PUT ||
	    varmask == G_RETVAR)
	{
	    cv = (C_VALS *)sym_g_vlue(var);
	    if (cv && cv->cv_flags == CvalDIMS && cv->cv_dims[0] != '\0')
		dimstr = cv->cv_dims;
	}
	aname = arg_name(arg);
	/* & needed before nvarchar but not wchar_t */
	genamp = dattyp == T_WCHAR ? FALSE : TRUE;
	break;
      default:				/* DBV's covered as a general case */
	dattyp = sym_g_btype(var);
	datlen = sym_g_dsize(var);
	aname = arg_name(arg);
	genamp = TRUE;
	break;
    };

    /* Dump the type of the variable */
    gen__int( dattyp );
    gen__obj( TRUE, ERx(",") );

    /* Dump the length of the variable */
    if (dimstr)
    {
	if (CVan(dimstr, &datlen) == OK)
	{
	    if (dattyp == T_CHAR || dattyp == T_WCHAR)
	    {
		if (dattyp == T_WCHAR)
		    datlen *= sizeof(wchar_t);
		if (varmask == G_PUT || varmask == G_RETVAR)
		    gen__int(datlen);
		/* Generate negative length for checking EOS at runtime */
		else if ((esq_geneos() == OK) && (varmask == G_SET))
		    gen__int(-datlen);
		else
		    gen__int(datlen - (dattyp == T_CHAR ? 1 : sizeof(wchar_t)));
	    }
	    else /* T_VCH  or T_VBYTE */
	    {
		if (dattyp == T_NVCH)
		    datlen *= sizeof(wchar_t);
		gen__int(datlen + 2);
	    }
	}
	else
	{
#ifdef	PMFE
	    gen__obj( TRUE, "(long)");
#endif
	    if (dattyp == T_WCHAR)
	    {
		gen__obj(TRUE, ERx("("));
		gen__obj(TRUE, dimstr);
		gen__obj(TRUE, ERx("*"));  
		gen__int(sizeof(wchar_t));
		gen__obj(TRUE, ERx(")"));
	    }
	    else
		gen__obj(TRUE, dimstr);
	    if (varmask != G_PUT && varmask != G_RETVAR)
	    {
		if (dattyp == T_CHAR)
		    gen__obj(TRUE, ERx("-1"));	/* Allow null at run-time */
		else if (dattyp == T_WCHAR)
		    gen__int(-(i4)sizeof(wchar_t)); /* Allow null at run-time */
		else /* T_VCH  or T_VBYTE */
		    gen__obj(TRUE, ERx("+2"));	/* Make way for count */
	    }
	}
    }
    else
    {
 	if ((!datlen) || (eq->eq_flags & EQ_COMPATLIB))
 	{
 	    gen__int( datlen );
 	}
 	else
 	{
#ifdef PMFE
 	    gen__obj( TRUE, ERx("(long)") );
#endif 
 	    gen__obj( TRUE, ERx("sizeof(") );
 	    gen__obj( TRUE, aname );
	    gen__obj( TRUE, ERx(")") );
	}
    }
    gen__obj( TRUE, ERx(",") );

    /* Dump the name of the variable - possibly preceded by '&'. */
    if (genamp)
    {
	if (*aname == '*')
	    gen__obj( TRUE, aname+1 );		/* skip the "&*" */
	else
	{
	    gen__obj( TRUE, ERx("&") );
	    gen__obj( TRUE, aname );
	}
    } else
	gen__obj( TRUE, aname );
    return 0;
}

/*
+* Procedure:	gen_val 
** Purpose:	Describe a value to the output file.
**
** Parameters: 	
**		arg 	- i4  - Argument number.
** Returns: 	0
**
** Describe whatever is in the mask to the output file. Used for Equel I/O
** functions that can send arguments by value.
**
-* Dumping order is: value, type, and length.
**
** History:
**	27-nov-1990 (barbara)
**	    Everything passed to run-time as "I/O" now gets passed by ref.
**	    This fixes the problem on some architectures where overloading
**	    a PTR parameter with a literal double causes strange results.
**	    On these architectures, doubles are passed in special registers
**	    but the LIBQ routine declares all I/O args as PTRS and so
**	    will look for the args on the stack.  This change also
**	    brings us closer to supporting ANSI-C prototypes -- which
**	    don't allow pointer arguments to be overloaded with value args.
**	    Example
**		## define f 1.2				-- Double
**		## append to tbl (a = f);
**	    code generation:
**		IIputdomio (0, 0, 31, 8, f);		-- Old way
**		IIputdomio (0, 1, 31, 8, IILQdbl(f));  -- New way
**
**	    where IILQdbl returns the address of 'f'.
*/

i4
gen_val( arg )
register i4	arg;
{
    i4		argtyp;
    i4		dattyp;
    i4		datlen;
    SYM		*var;

    /* Dump the type of the value */
    if (var = arg_sym(arg))		/* Must be a constant */
    {
	dattyp = sym_g_btype( var );
	datlen = sym_g_dsize( var );
    } else 					/* No symbol entry */
    {
	if ((argtyp = arg_rep(arg)) == ARG_CHAR)
	    argtyp = arg_type(arg);

	switch (argtyp)
	{
	  case ARG_INT:  
	    dattyp = T_INT;
	    var = sym_resolve( (SYM *)0, ERx("int"), C_CLOSURE, syFisTYPE );
	    datlen = sym_g_dsize( var );
	    break;

	  case ARG_FLOAT:  
	    dattyp = T_FLOAT;
	    datlen = sizeof( f8 );
	    break;

	  case ARG_PACK:		/* Pass decimal as a character string */
	    dattyp = T_PACKASCHAR;
	    datlen = 0;
	    break;

	  case ARG_CHAR:  
	  case ARG_RAW:  			/* Should never occur */
	  default:
	    dattyp = T_CHAR;
	    datlen = 0;
	    break;
	}
    }
    /* All values passed to I/O routines now go by ref */
    gen__int( 1 );
    gen__obj( TRUE, ERx(",") );
    gen__int( dattyp );
    gen__obj( TRUE, ERx(",") );
	    
    /* Dump the length of the value */
    if (dattyp == T_INT || dattyp == T_FLOAT)
    {
#ifdef PMFE
	gen__obj( TRUE, ERx("(long)") );
#endif
	gen__obj( TRUE, ERx("sizeof(") );
	gen_data( 0, arg );
	gen__obj( TRUE, ERx(")") );
    }
    else
    	gen__int( datlen );
    gen__obj( TRUE, ERx(",") );

    /* Dump the real data */
    if (dattyp == T_FLOAT)
    {
	gen__obj( TRUE, ERx("(void *)IILQdbl("));
	gen_data( 0, arg );
	gen__obj( TRUE, ERx(")"));
    }
    else if (dattyp == T_INT)
    {
	gen__obj( TRUE, ERx("(void *)IILQint("));
	gen_data( 0, arg );
	gen__obj( TRUE, ERx(")"));
    }
    else	/* CHAR */
    {
	gen_data( 0, arg );
    }
    return 0;
}


/*
+* Procedure:	gen_data 
** Purpose:	Whatever is stored by the argument manager for a particular 
**	      	argumment, dump to the output file.  
**
** Parameters: 	varmask - i4  - Variable mask - passing mechanism and more,
**		arg	- i4  - Argument number.
-* Returns: 	None
**
** This routine should be used when no validation of the data of the argument 
** needs to be done. Hopefully, if there was an error before (or while) 
** storing the argument this routine will not be called.
**
** History:
**	11-nov-1992 (kathryn)
**	    Cast integer variables to appropriate host type (long/int) when
**	    passing variables by value.  
**	01-nov-1992 (kathryn)
**	    Add check for RETVAR flag to pass variables by reference.
*/

void
gen_data( varmask, arg )
i4		varmask;		/* By value or reference */
register i4	arg;
{
    i4		num;
    f8		flt;
    char	*s;
    SYM		*asym;

    if (asym = arg_sym(arg))		  /* User variable */
    {
	if (varmask == G_RETVAR) 
	{
   	    s = arg_name(arg);		

	    if (*s == '*')
		gen__obj( TRUE, s+1 );          /* skip the "&*" */
	    else
	    {
		gen__obj( TRUE, ERx("&") );
		gen__obj( TRUE, s );
	     }
	}
	else
	{
	    gen__obj( TRUE, arg_name(arg) );
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
    	  case ARG_CHAR: 	/* String constant */
	  case ARG_PACK:	/* Decimal constant pass as a string */
	    if (s = arg_name(arg))
	    {
		gen__obj( TRUE, ERx("(char *)") );

                if (STlength(s) > G_LINELEN - out_cur->o_charcnt - 2)
                   out_emit( ERx( "\n" ) );     /* Break the line */

		gen_sconst( s );
	    }
	    else
		gen__obj( TRUE, ERx("(char *)0") );
	    break;

	  case ARG_INT:		/* Integer string */	
	  case ARG_RAW:		/* Raw data stored */
	  default:		/* String of user numeric */
	    gen__obj( TRUE, arg_name(arg) );
	    break;
	}
	break;
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
	gen_sconst( fbuf );
	gen__obj( TRUE, ERx(",") );
	gen__int( eq->eq_line );
    } else
	gen__obj( TRUE, ERx("(char *)0,0") );
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
	gen__obj( TRUE, ERx("(short *)0,") );	/* Pass null pointer */
    }
    else
    {
	gen__obj( TRUE, ERx("&") );		/* Pass indicator by ref */
	gen__obj( TRUE, arg_name(argnum) );
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
** Call gen__sconst() that will dump the string in pieces that fit on a line
** and continue each line with the \ sign.
*/

void
gen_sconst( str )
char	*str;
{
    if (str != (char *)0)
    {
	out_emit( ERx("\"") );
	while (gen__sconst(&str))
	    out_emit( ERx("\\\n") );		/* Continue the string */
	out_emit( ERx("\"") );
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
** 2. Care should be take not to split lines in the middle of compiler esacpe
**    sequences - the most complex of which occur in C.
*/

i4
gen__sconst( str )
register char	**str;
{
    register char	*cp;
    register i4	len;
    i4			dig_len;		/* For \ddd C sequneces */

    /* Allow 5 extra spaces for possible \ or "); as the call may require */
    if ((i4)STlength(*str) <= (len = G_LINELEN - out_cur->o_charcnt - 5))
    {
	out_emit( *str );
	return 0;
    }
    for (cp = *str; len > 0 && *cp;)
    {
	/* Try not to break in the middle of compiler escape sequences */
	if (*cp == '\\')
	{
	    CMbytedec(len,cp);
	    CMnext(cp);			/* Skip the \ */
	    if (CMdigit(cp) )		/* Eat up octals <= 3 digits */
	    {
		cp++; 				/* Eat first digit */
		len--;
		dig_len = 1;
		/* Eat rest of them */
		for (; CMdigit(cp) && dig_len <= 3; dig_len++, cp++, len--)
		    ;
		continue;
	    }
	}
	CMbytedec(len,cp);
	CMnext(cp);			/* Skip current character */
    }
    if (*cp == '\0')				/* All the way till the end */
    {
	out_emit( *str );
	return 0;
    }
    { /* fall thru - just a context for a seldom used large stack entry */
	char	tempstr[G_MAXSCONST];
	SCALARP	templen;
	templen = (SCALARP)cp - (SCALARP)*str;
	if (templen > 0)
	    MEcopy(*str,(u_i2)templen,tempstr);
	tempstr[templen] = '\0';
	out_emit( tempstr );
	*str = cp;				/* More to go next time */
	return 1;
    }
}

/*
+* Procedure:	gen__int 
** Purpose:	Does local integer conversion.
**
** Parameters: 	num - i4  - Integer to put out.
-* Returns: 	None
*/

void
gen__int( num )
i4	num;
{
    char	buf[20];

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
    /* The +3 is for unused space once the object is reached */
    if ((i4)STlength(obj) > G_LINELEN - out_cur->o_charcnt +3)
	out_emit( ERx("\n") );
    /* May have been just reset or may have been a previous newline */
    if (out_cur->o_charcnt == 0 && ind)
	out_emit( g_indent );
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
    g_indlevel += i;
    n_ind = g_indlevel;
    if (o_ind < 1)
	o_ind = 1;
    else if (o_ind >= G_MAXINDENT)
	o_ind = G_MAXINDENT - 1;
    if (n_ind < 1)
	n_ind = 1;
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

    f_loc = flag & ~(G_H_INDENT | G_H_OUTDENT | G_H_NEWLINE);

    if (flag & G_H_OUTDENT)			/* Outdent before data */
	gen_do_indent( -1);
    switch (f_loc)
    {
      case G_H_KEY:
	if (was_key && *s && *s != ',')
	    out_emit( ERx(" ") );
	gen__obj( TRUE, s );
	was_key = 1;
	break;

      case G_H_OP:
	if (*s == '#' && out_cur->o_charcnt == 0)
	{
	    /* Quick way to get nice output */
	    g_indent[0] = '#';
	    out_emit( g_indent );
	    g_indent[0] = ' ';
	    break;
	}
	if (was_key && *s && *s != ',' && *s != ';' && *s != '[')
	    out_emit( ERx(" ") );
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
	gen_do_indent( 1);
}
