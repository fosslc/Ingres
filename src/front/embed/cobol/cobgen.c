/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** 20-jul-90 (sandyd) 
**	Workaround VAXC optimizer bug, first encountered in VAXC V2.3-024.
NO_OPTIM = vax_vms rs4_us5
*/

# include	<compat.h>
# include	<gl.h>
# include	<er.h>
# include	<cv.h>
# include	<si.h>
# include	<st.h>
# include	<lo.h>
# include	<nm.h>
# include	<cm.h>
# include	<eqrun.h>
# include	<equel.h>
# include	<eqsym.h>
# include	<eqgen.h>
# include	<eqcob.h>
# include	<ereq.h>
# include	<ere4.h>

/*
+* Filename:	COBGEN.C
**
** Purpose: 	Maintains all the routines to generate II calls within
**	    	specific langauge dependent control flow statements.
**
** Language:	Written for IBM VS/COBOL on CMS.
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
**		gen_do_indent()			- Update indentation.
**		gen_host()			- Host code (declaration) calls.
**		gen_cob01()			- Special call for 01 data items
**		gen_null()			- Gen a null-indicator.
**		gen_Cseqnum			- Emits COBOL sequence number.
** Exports:
**		gen_code()			- Macro defined as out_emit().
** Locals:
**		gen__goto()			- Core of goto statement.
**		gen__label()			- Statement independent Label.
**		gen__II()			- Core of an II call.
**		gen_all_args()			- Dump all args in call.
**		gen_var_args()			- Varying number of argunents.
**		gen_io()			- Describe I/O arguments.
**		gen_iodat()			- Internals of I/O data.
**		gen_data()			- Dump different arg data.
**		gen_IIdebug()			- Runtime debug information.
**		gen_sconst()			- String constant.
**		gen__sconst()			- Controls splitting strings.
**		gen__int()			- Integer.
**		gen__float()			- Float.
**		gen__obj()			- Write anything but strings.
**		gen__functab( func )		- Get function table pointer.
** COBOL Specific:
**		gc_nxti4()			- Get next COBOL III4 variable.
**		gc_nxtch()			- Get next COBOL IIS variable.
**		gc_nxtlch()			- Get next COBOL IIL variable.
**		gc_cvla()			- Return static CVna.
**		gc_cvfa()			- Return static CVfa.
**		gc_sdcall()			- S descriptor run-time call.
**		gc_mfsdcall()			- MF S descriptor run-time call.
**		gc_mfstrmove()			- MF STRING/MOVE statement.
**		gc_iofmt()			- Format I/O description.
**		gc_move()			- Gen a MOVE statement.
**		gc_args()			- Add/Free argument COBOL args.
**		gc_retmove()			- Move return value to user var.
**		gc_comp()			- Cast a COMP[-3] to/from an f8.
**		gc_strf8()			- Cast string to an f8.
**		gc_newline()			- Control newline generation.
**
** Externals Used:
**		out_cur				- Current output state.
**		eq				- Global Equel state.
**
** The general rule to generating a template is that we need to pass all the
** information possibly needed in the worst case.  Since COBOL
** does not support 'while' loops we need to generate two labels, and
** control the branches taken at the beginning and end of the loop.  Thus, to
** start a loop we pass as much information as will be needed even for 'good'
** languages such as C.	 The same applies to If blocks - who knows what
** language will not allow If statements and will require a labelled
-* implementation.
**
** Note for routines that write data:
**  Some routines that actually write stuff make use of the external counter
**  out_cur->o_charcnt.  This is the character counter of the current line
**  being written to.  Whenever there is a call to emit code, this counter
**  gets updated even though the local code does not explicitly update it.
**  A call to gen__obj() which makes sure the object can fit on the line,
**  needs to be made only when the data added may have gone over the end of
**  the line.  In some cases, either when nothing is really being written,
**  or the data comes directly from the host language host code, the main
**  emit routine can be used.
**
** History:
**		30-oct-1984	- Written for C (ncg)
**		30-apr-1985	- Adapted for IBM COBOL (saw)
**		10-may-1985	- Adapted back for VMS COBOL (ncg)
**		17-nov-1985	- Few modifications for VMS ESQL/COBOL (bjb)
**		12-dec-1986	- Fix bug 11013, where PARAM= option to CALL
**				  REPORT statement was truncating value > 32
**				  characters (bcw)
**		03-aug-1987	- Converted to 6.0 (mrw)
**		23-mar-1989	- Added cast to STlength. (teresa)
**		18-apr-1989	- Put in DG changes (sylviap)
**		28-apr-1989	- Fixed bug when checking for escaped characters
**				  Added DG fix to accommodate 1,2,3 & 4 byte
**				  integers. (sylviap)
**		01-may-1989	- Changed gen_null so if DG, then pass II0 to
**				  indicate a null ptr vs a ptr to zero.  
**				  (sylviap)
**		24-may-1989	- Fixed bug in gen_iodat, figuring out length
**				  of a string due to escaped characters (double 
**				  quotes). (sylviap)
**              15-jun-1989     - If the user variable is an i1, i2 or i3,
**                                it is moved to an i4. (sylviap)
**              22-jun-1989     - For DG only -- in gen_iodat, initialize head
**				  if arg_type == ARG_CHAR (sylviap)
**		22-aug-1989	- Added ANSI COBOL sequence number for ESQL
**				  only. (teresal)
**              11-may-1989     - Added function calls IITBcaClmAct,
**                                IIFRafActFld, IIFRreResEntry to gen__ftab
**                                for entry activation. (teresa)
**              09-aug-1989 	- Add function call IITBceColEnd for Emerald.
**				  (teresa)
**		16-nov-1989	- Modified to support MF COBOL: (ncg, neil)
**				  1. Collapsed generated label names.
**				  2. Cased the external calls (MF requires C
**				     case) and upper-cased for non-MF.
**				  3. See gen_io description notes for detailed
**				     MF data formats.
**				  4. MF COBOL string literals <= 160.
**				  5. MF CALL names <= 14.
**				  6. Fixed various sequence #/padding bugs.
**				  7. Allow EXECUTE PROCEDURE in loop.
**		19-dec-1989	- Updated for Phoenix/Alerters. (bjb)
**              07-feb-1990     - Modified to support MF COBOL but without use
**                                of 'structured COBOL' facilities END-IF,
**                                END-PERFORM, or CONTINUE, uses a dynamic
**                                switch in eq->eq_config to choose not to
**                                use structured COBOL. (kwatts, ken)
**                                1. Changed gen_loop to always choose 
**                                   simulated loop with labels and gotos if
**                                   the option is set.
**                                2. Changed all CONTINUEs to generate the
**                                   DG approved MOVE statement if the option
**                                   is set.
**                                3. Changed gen_if to rely on run-time ifs
**                                   for IInextget and IIcsretrieve.
**				  4. Added G_RTIF prefixes to introduce
**				     extra glue when -u option set.
**				  5. Updated gen__II to take account of
**				     G_RTIF when deciding on IIX needs.
**                                6. gen_if closure on IIERRTEST is a period
**				     and on IINEXEC it is a noop (nothing).
**				  7. gen_goto sometimes generates a dummy if
**				     for 'select loop'it is only there to
**				     avoid a possible compiler warning on 
**				     some machines, which does not include
**				     MF or VME COBOL. Don't generate the IF
**				     when compiling with -u.
**				  8. gc_retmove for null-indicated data moves
**				     changed to use period instaed of END-IF.
**		9-jul-1990	- Bug fix 31455.  Modified gen_loop to turn
**				  dot mode on so periods are added to
**				  generated statements. (teresal)
**		18-jul-1990	- Add decimal support. (teresal)
**		14-aug-1990 (barbara)
**		    Added call to IIFRsaSetAttrio to gen__ftab for per
**		    value cell highlighting support.
**		05-aug-1990 (gurdeep rahi)
**		    Handle variable arguments for MF_COBOL.  Generate prefix
**		    "IIX" and an extra argument to denote argument count for
**		    routines that expect variable number of arguments.
**		    Routine affected is gen_var_args().
**              22-feb-1991 (kathryn)
**                      Added IILQssSetSqlio and IILQisInqSqlio.
**			      IILQshSetHandler.
**		01-apr-1991 (kathryn)
**		    Add emply lines 149 and 150 so event statments are correct
**		    at lines 151 and 152 in the gen__ftab table.
**		21-apr-1991 (teresal)
**		    Add activate event call - remove IILQegEvGetio.
**		15-nov-1991 (teresal)
**		    Update comments for packed decimal code generation.
**		14-oct-1992 (lan)
**		    Added IILQled_LoEndData.
**		08-dec-1992 (lan)
**		    Added IILQldh_LoDataHandler.
**              10-dec-1992 (kathryn)
**                  Added gen_datio() function, and added entries for
**                  IILQlgd_LoGetData, and IILQlpd_LoPutData.
**		23-jul-93 (rcs/russ spence) 
**		    updated gen_include to use gen_sconst to cope with 
**		    long path names
**		26-jul-1993 (lan)
**		    Added EXEC 4GL support routines.
**	07/28/93 (dkh) - Added support for the PURGETABLE and RESUME
**			 NEXTFIELD/PREVIOUSFIELD statements.
**	26-Aug-1993 (fredv)
**		Included <st.h>.
**      01-oct-1993 (kathryn)
**          First argument generated for GET/PUT DATA stmts ("attrs" bit mask),
**          will now be set/stored by eqgen_tl() rather than determined by
**          gen_datio, so add one to the number of args in the gen__ftab for
**          IILQlpd_LoPutData and IILQlgd_LoGetData.
**	7-oct-1993 (essi)
**	    When using -u flag we are not handling the switches
**	    correctly. The gc_dot and g_was_if require a much better
**	    handling specially if we generate nested loops or
**	    if-blocks. To fix this particular bug I introduced a
**	    new flag, g_in_if. This flag is set when we enter an
**	    if-block (gen_if only) and exit when we end an if-block.
**	    It required a hack to str_chget in strman.c in order to
**	    remove the dots if we are inside of 'gen_if' (54571).
**	13-oct-93 (essi)
**	    Placed the definition of g_in_if in strman.c since it
**	    is used by other preprocessors. Placed the reference in 
**	    this file.
**	24-nov-1993 (rudyw)
**	    Withdraw 7-oct and 13-oct code changes due to problems
**	    resulting in new bug. "Unstructured" needs more research.
**	13-dec-1993 (mgw) Bug #56878
**	    Make the desc_buf[] in gen_datio() static, not automatic,
**	    since it's address get's passed out and used elsewhere.
**	    Also correct a couple of typos.
**	04-jan-1994 (lan)
**	    Fixed bug #57281 - For unstructured COBOL (-u option), the
**	    dot terminators were incorrectly placed within an IF block
**	    and consequently caused runtime error E_LQ002B.
**	04-jan-1994 (lan)
**	    Added some more comments regarding fix for bug #57281.
**	24-apr-1996 (muhpa01)
**	    Added support for Digital Cobol (DEC_COBOL).
**	6-may-97 (inkdo01)
**	    Add table entry for IILQprvProcGTTParm.
**      24-jul-1998 (dacri01)
**          gc_dot was not being initialized, so when the indicator
**          column had '*', 'D' or '\\' then it simply retained the
**          old value.  Set gc_dot = FALSE in that instance.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      02-apr-2001 (rigka01) bug #104292
**	    The maximum length of the CALL identifier in Microfocus
**	    COBOL 4.0.38 is 30; in the follow-on product, Netexpress 3.1
**	    it is 29 characters so use 29 as maximum length.  Prior to
**	    this change, max len was 14 but this caused unresolved 
**	    externals of IILQlgd_LoGetData, IILQled_LoEndData, and
**	    IILQlpd_LoPutData when linking an embedded MF cobol program
**	    which uses a blob data handler.
**	2-may-01 (inkdo01)
**	    Added parm to ProcStatus for row procs.
**	18-Sep-2003 (bonro01)
**	    For i64_hpu modified length specified when an INT parameter
**	    is specified from 4 to 8 bytes.  We only support 64-bit
**	    COBOL programs and the 64-bit Cobol code passes integer
**	    constants as 8-byte values.
**	24-dec-2003 (abbjo03)
**	    gen__functab() returns a pointer to gen__ftab, which includes a
**	    pointer to a static string. On VMS, this causes an access violation
**	    in gen__II() when CVupper() attempts to overwrite the string in
**	    place. Corrected by copying g_IIname to a local array.
**	    constants as 8-byte values.
**	31-Jan-2005 (drivi01)
**	    Modified cobol preprocessor to generate IIaddform instead of 
**	    IIaddfrm.
**	16-May-2005 (drivi01)
**	    Put IIaddfrm back to back out my previous change, IIaddfrm wasn't
**	    being properly exported hence causing problems.
**	25-May-2005 (karbh01)(b114571) (INGEMB141)
**	    For r64_us5 modified length specified when an INT parameter
**	    is specified from 4 to 8 bytes. 64-bit Cobol code passes integer
**      16-Jun-2005 (hweho01)
**          COBOL code was not generated correctly by eqcbl on AIX platform.
**          Problem occurred on embed/cob00.sep test. 
**          Compiler version: C for AIX 6.006 (inclued VisualAge C++ package). 
**	20-jul-2007 (toumi01)
**	    add IIcsRetScroll and IIcsOpenScroll for scrollable cursors
**	08-mar-2008 (toumi01) BUG 119457
**	    set G_FUNC for IIcsRetScroll for MF COBOL IIX function.
**	16-mar-2010 (frima01) BUG 123109
**	    Increase char buffer in gc_cvla to hold larger query IDs.
**	    
*/

/*
>>  For DG, NO arguments can be passed by VALUE.  To keep the code readable and
>>  to eliminate multiple ifdef's, GCmVAL is redefined to GCmREF.  Literals
>>  will still have to be moved to temporary variables before they are passed
>>  in a calling statement.
*/

# ifdef DGC_AOS
# 	undef GCmVAL
# 	define GCmVAL	GCmREF
# endif /* DGC_AOS */

/* 
** Max line length for VMS COBOL code is quite flexible in terminal format 
** but we will begin with a TAB. Thus we use MAX = 80-sizeof(TAB).
** On other system we can replace the TAB with a Area_B string.
*/
# define	G_LINELEN	72

#if defined(MF_COBOL) 
/* Max length for MF COBOL CALL identifier */
# define	GC_MFCALL_LEN   29	
#else
# define	GC_MFCALL_LEN   14	
#endif

/*
** Max size of converted string - allow some extra space over scanner for
** extra escaped characters required.
*/
# define	G_MAXSCONST	300

/*
** Length of an ANSI sequence number.
*/
# define	G_SEQLEN	6

/* State for last statement generated to avoid empty IF blocks - illegal */
static  i4	g_was_if	= 0;

/* State to avoid empty PARAGRAPH labels  - warnings for MF */
static  i4	g_was_label	= 0;

/*
** Length of COBOL variable buffers defined by us in include file.
** The short buffer size was increased from 31 to 32 because:
**	1. To accomodate the maximum decimal literal (31 digits plus 1
**	   decimal point).  Decimal literals are passed as char strings.
**	2. To anticipate the increase in the maximum length of a 
**	   INGRES name from 24 to 32 characters. 
*/

# define	GC_IISLEN	32 	/* Short buffers */
# define	GC_IILLEN	263 	/* Long buffer */

/*
** MF COBOL max string literal length = 160.  Fudge factor to allow us to
** insert quotes and other characters (say 6).
*/
# define	GC_MFSLEN	154

/* Maximum objects active in a CALL */
#   define	GC_MAXARG	15
#   define	III4max		4		/* Max III4's active in CALL */
#   define	IICHmax		14		/* Max IIS's active in CALL */

static char	*gc_fprefix ZERO_FILL;		/* "II" or "IIx", for gen__II */

/* 
** Newline control and period flag:
**	This flag, gc_dot, indicates whether we are in period mode or not.
** When we enter period mode (ie, we generated a label, or we were passed
** host code that terminated with a period) we set the flag to true.  This
** flag is then used on subsequent new-line printing (if we generate a new-line
** after a CALL we want a period).  In some cases we may go out of period mode
** (ie, we enter a generated IF block, or we were passed host code that did not
** end with a period) and then when encountering a newline we do not generate
** a period.  If the user has a terminating period on an embedded statement
** we always go into period mode.  See the routines gen_term and gc_newline
** for more details.  It's also used by gen_sqlca.
*/
GLOBALDEF bool	gc_dot = TRUE;

/*
** gc_retnull - Pointer to null name or indicator variable name.
**
** gc_retnull is set to an indicator variable name in gen_null if used for
** output.  Then in gc_retmove or gc_comp (in the dumping phase) it is checked
** for and generated around the resulting move statement. It is then cleared
** at the end of gen__II.  For example, after using a temp i4 for numeric
** edited data we may generate:
**
**		CALL "IIGETDOMIO" USING NULL-IND, III4.
**		IF NULL-IND NOT= -1 
**			MOVE III4 TO NUM-EDIT-VAR.
*/
static	char	*gc_retnull	ZERO_FILL;


/*
** Indentation information -
**	In VMS COBOL terminal format indentation is not strict, but we try
** to pretty print the output by indenting 2 spaces per level and starting
** with a TAB - never go lower than the first TAB unless for label.
**	For card format, statements are output beginning in col 12 (area B).
** Labels and 01 level numbers are output in col 8 (area A).  No TABS are
** used.
*/

/* For card format output */
# define	gc_mgA		ERx("       ")
# define	gc_mgB  	ERx("           ")
# define	gc_mgC  	ERx("      ")
# define	gc_mgCtoB  	ERx("     ")
# define	gc_tabtoB  	ERx("   ")

# define	G_MAXINDENT	20

static		char	g_indent[ G_MAXINDENT + G_MAXINDENT ] =
		{
		  '\0',' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '
		};

static	i4	g_indlevel = 0;	  /* Start at 0 spaces from TAB/Margin B */

/*
** Table of conditions -
** The constants for logical opposites are negatives of each other. For example
** to get the negative of a condition use:
**
**	gen_condtab[ C_RANGE - cond ]
**
** For example the opposite of C_LESS ( "<" ) is -C_LESS which gives C_GTREQ
** which when used with the extra C_RANGE gives ">=".
** See where these constants are defined.
** Table usage:
**	condition	gen_condtab[ C_RANGE + cond ]
**	complement	gen_condtab[ C_RANGE - cond ]
*/

static	char	*gen_condtab[] = {
    ERx("NOT <"),		/* C_GTREQ */
    ERx("NOT >"),		/* C_LESSEQ */
    ERx("NOT ="),		/* C_NOTEQ */
    (char *)0,			/* just a place holder */
    ERx("="),			/* C_EQ */
    ERx(">"),			/* C_GTR */
    ERx("<")			/* C_LESS */
};

/*
** Table of label names - correspond to L_xxx constants.
** Left DG names as the only names (no need for underscore if not allowed)
*/

static	 char	*gen_ltable[] = {
	    ERx("IIL-"),		/* Should never be referenced */
	    ERx("IIFD-INIT"),
	    ERx("IIFD-BEGIN"),
	    ERx("IIFD-FINAL"),
	    ERx("IIFD-END"),
	    ERx("IIMU-INIT"),
	    ERx("IIACT%d-%d"),		/* Special Else label number */
	    ERx("IIACT-END"),
	    ERx("IIREP-BEGIN"),
	    ERx("IIREP-END"),
	    ERx("IIRET-BEGIN"),
	    ERx("IIRET-FLUSH"),
	    ERx("IIRET-END"),
	    ERx("IITAB-BEGIN"),
	    ERx("IITAB-END"),
    	    (char *)0
};

/* 
** Local buffer for Labelled languages (COBOL) Else clause, and flag 
** indicating its usage 
*/
static		char		g_lab_else[50]	ZERO_FILL;
# define	G_ELSELAB	-1

/*
** Local goto, label, call and argument dumping calls -
** Should not be called from the outside, even though they are not static.
*/

void	gen__goto(), gen__label(), gen__II(); 
void	gen_data(), gen__int(), gen__float(), gen__obj(), gen_null();
i4	gen__sconst(), gen_iodat();

/* COBOL specific utilities */
char	*gc_nxti4(), *gc_nxtch(), *gc_nxtlch(), *gc_cvla(), *gc_cvfa();
void	gc_sdcall(), gc_move(), gc_args(), gc_retmove(), gc_newline();
void	gc_iofmt(), gc_comp();
void	gc_strf8(), gc_mfsdcall(), gc_mfstrmove();
bool	Cput_seqno();

/* For table below */
i4	gen_all_args(), gen_var_args(), gen_io(), gen_unsupp();
i4	gen_IIdebug();
i4	gen_hdl_args();
i4      gen_datio();

/*
** Table of II calls -
** Correspond to II constants and the number of arguments.  For COBOL, the
** II constant is used as an index to eventually reach the intended II
** function.  Groupings and special cases are handled in gen__II.
*/

GEN_IIFUNC	*gen__functab();

GLOBALDEF GEN_IIFUNC	gen__ftab[] = {
/*

** -----------  --------------		--------------	-------------	-----
*/

/* 000 */    {ERx("IIF_"),		0,		0,		0},

			/* Quel # 1 */
/* 001 */    {ERx("IIflush"),		gen_IIdebug,	G_RTIF,		2},
/* 002 */    {ERx("IInexec"),		0,		G_FUNC,		0},
/* 003 */    {ERx("IInextget"),		0,		G_FUNC,		0},
# if defined(MF_COBOL) || defined(DEC_COBOL)
/* 004 */    {ERx("IIputdomio"),	gen_io,		G_SET,		2},
#else
/* 004 */    {ERx("IINOTRIMIO"),	gen_io,		G_SET,		2},
#endif
/* 005 */    {ERx("IIparret"),		0,		0,		0},
/* 006 */    {ERx("IIparset"),		0,		0,		0},
/* 007 */    {ERx("IIgetdomio"),	gen_io,		G_RET|G_RTIF,	2},
/* 008 */    {ERx("IIretinit"),		gen_IIdebug,	0,		2},
/* 009 */    {ERx("IIputdomio"),	gen_io,		G_SET,		2},
/* 010 */    {ERx("IIsexec"),		0,		0,		0},
/* 011 */    {ERx("IIsyncup"),		gen_IIdebug,	0,		2},
/* 012 */    {ERx("IItupget"),		0,		G_FUNC,		0},
/* 013 */    {ERx("IIwritio"),		gen_io,		G_SET,		3},
/* 014 */    {ERx("IIxact"),		gen_all_args,	0,		1},
/* 015 */    {ERx("IIvarstrio"),	gen_io,		G_SET,		2},
/* 016 */    {ERx("IILQssSetSqlio"),    gen_io,         G_SET,          3},
/* 017 */    {ERx("IILQisInqSqlio"),    gen_io,         G_RET,          3},
/* 018 */    {ERx("IILQshSetHandler"),  gen_hdl_args,   0,              2},
/* 019 */    {(char *)0,                0,              0,              0},
/* 020 */    {(char *)0,		0,		0,		0},

			/* LIBQ # 21 */
/* 021 */    {ERx("IIbreak"),		0,		0,		0},
/* 022 */    {ERx("IIeqiqio"),		gen_io,		G_RET,		3},
/* 023 */    {ERx("IIeqstio"),		gen_io,		G_SET,		3},
/* 024 */    {ERx("IIerrtest"),		0,		G_FUNC,		0},
/* 025 */    {ERx("IIexit"),		0,		0,		0},
/* 026 */    {ERx("IIingopen"),		gen_var_args,	0,		10},
/* 027 */    {ERx("IIutsys"),		gen_all_args,	G_LONG,		3},
/* 028 */    {ERx("IIexExec"),		gen_all_args,	0,		4},
/* 029 */    {ERx("IIexDefine"),	gen_all_args,	0,		4},
/* 030 */    {(char *)0,		0,		0,		0},

			/* FORMS # 31 */
/* 031 */    {ERx("IITBcaClmAct"),	gen_all_args,	G_FUNC,		4},
/* 032 */    {ERx("IIactcomm"),		gen_all_args,	G_FUNC,		2},
/* 033 */    {ERx("IIFRafActFld"),	gen_all_args,	G_FUNC,		3},
/* 034 */    {ERx("IInmuact"),		gen_all_args,	G_FUNC|G_LONG,	5},
/* 035 */    {ERx("IIactscrl"),		gen_all_args,	G_FUNC,		3},
/* 036 */    {ERx("IIaddfrm"),		gen_all_args,	0,		1},
/* 037 */    {ERx("IIchkfrm"),		0,		G_FUNC,		0},
/* 038 */    {ERx("IIclrflds"),		0,		0,		0},
/* 039 */    {ERx("IIclrscr"),		0,		0,		0},
/* 040 */    {ERx("IIdispfrm"),		gen_all_args,	G_FUNC,		2},
/* 041 */    {ERx("IIprmptio"),		gen_io,		G_RETSET|G_LONG,4},
/* 042 */    {ERx("IIendforms"),	0,		0,		0},
/* 043 */    {ERx("IIendfrm"),		0,		0,		0},
/* 044 */    {ERx("IIendmu"),		0,		G_FUNC,		0},
/* 045 */    {ERx("IIfmdatio"),		gen_unsupp,	0,		0},
/* 046 */    {ERx("IIfldclear"),	gen_all_args,	0,		1},
/* 047 */    {ERx("IIfnames"),		0,		G_FUNC,		0},
/* 048 */    {ERx("IIforminit"),	gen_all_args,	0,		1},
/* 049 */    {ERx("IIforms"),		gen_all_args,	0,		1},
/* 050 */    {ERx("IIfsinqio"),		gen_io,		G_RET,		3},
/* 051 */    {ERx("IIfssetio"),		gen_io,		G_SET,		3},
/* 052 */    {ERx("IIfsetio"),		gen_all_args,	G_FUNC,		1},
/* 053 */    {ERx("IIgetoper"),		gen_all_args,	0,		1},
/* 054 */    {ERx("IIgtqryio"),		gen_io,		G_RET,		3},
/* 055 */    {ERx("IIhelpfile"),	gen_all_args,	G_LONG,		2},
/* 056 */    {ERx("IIinitmu"),		0,		G_FUNC,		0},
/* 057 */    {ERx("IIinqset"),		gen_var_args,	G_FUNC,		5},
/* 058 */    {ERx("IImessage"),		gen_all_args,	G_LONG,		1},
/* 059 */    {ERx("IImuonly"),		0,		0,		0},
/* 060 */    {ERx("IIputoper"),		gen_all_args,	0,		1},
/* 061 */    {ERx("IIredisp"),		0,		0,		0},
/* 062 */    {ERx("IIrescol"),		gen_all_args,	0,		2},
/* 063 */    {ERx("IIresfld"),		gen_all_args,	0,		1},
/* 064 */    {ERx("IIresnext"),		0,		0,		0},
/* 065 */    {ERx("IIgetfldio"),	gen_io,		G_RET,		3},
/* 066 */    {ERx("IIretval"),		0,		G_FUNC,		0},
/* 067 */    {ERx("IIrf_param"),	0,		0,		3},
/* 068 */    {ERx("IIrunform"),		0,		G_FUNC,		0},
/* 069 */    {ERx("IIrunmu"),		gen_all_args,	0,		1},
/* 070 */    {ERx("IIputfldio"),	gen_io,		G_SET,		3},
/* 071 */    {ERx("IIsf_param"),	0,		0,		3},
/* 072 */    {ERx("IIsleep"),		gen_all_args,	0,		1},
/* 073 */    {ERx("IIvalfld"),		gen_all_args,	G_FUNC,		1},
/* 074 */    {ERx("IInfrskact"),	gen_all_args,	G_FUNC|G_LONG,	5},
/* 075 */    {ERx("IIprnscr"),		gen_all_args,	G_LONG,		1},
/* 076 */    {ERx("IIiqset"),		gen_var_args,	G_FUNC,		5},
/* 077 */    {ERx("IIiqfsio"),		gen_io,		G_RET,		5},
/* 078 */    {ERx("IIstfsio"),		gen_io,		G_SET,		5},
/* 079 */    {ERx("IIresmu"),		0,		0,		0},
/* 080 */    {ERx("IIfrshelp"),		gen_all_args,	G_LONG,		3},
/* 081 */    {ERx("IInestmu"),		0,		G_FUNC,		0},
/* 082 */    {ERx("IIrunnest"),		0,		G_FUNC,		0},
/* 083 */    {ERx("IIendnest"),		0,		0,		0},
/* 084 */    {ERx("IIFRtoact"),		gen_all_args,	G_FUNC,		2},
/* 085 */    {ERx("IIFRitIsTimeout"),	0,		G_FUNC,		0},

			/* TABLE FIELDS # 86 */
/* 086 */    {ERx("IItbact"),		gen_all_args,	G_FUNC,		3},
/* 087 */    {ERx("IItbinit"),		gen_all_args,	G_FUNC,		3},
/* 088 */    {ERx("IItbsetio"),		gen_all_args,	G_FUNC,		4},
/* 089 */    {ERx("IItbsmode"),		gen_all_args,	0,		1},
/* 090 */    {ERx("IItclrcol"),		gen_all_args,	0,		1},
/* 091 */    {ERx("IItclrrow"),		0,		0,		0},
/* 092 */    {ERx("IItcogetio"),	gen_io,		G_RET,		3},
/* 093 */    {ERx("IItcoputio"),	gen_io,		G_SET,		3},
/* 094 */    {ERx("IItcolval"),		gen_all_args,	0,		1},
/* 095 */    {ERx("IItdata"),		0,		G_FUNC,		0},
/* 096 */    {ERx("IItdatend"),		0,		0,		0},
/* 097 */    {ERx("IItdelrow"),		gen_all_args,	G_FUNC,		1},
/* 098 */    {ERx("IItfill"),		0,		0,		0},
/* 099 */    {ERx("IIthidecol"),	gen_all_args,	G_LONG,		2},
/* 100 */    {ERx("IItinsert"),		0,		G_FUNC,		0},
/* 101 */    {ERx("IItrc_param"),	0,		0,		0},
/* 102 */    {ERx("IItsc_param"),	0,		0,		0},
/* 103 */    {ERx("IItscroll"),		gen_all_args,	G_FUNC,		2},
/* 104 */    {ERx("IItunend"),		0,		0,		0},
/* 105 */    {ERx("IItunload"),		0,		G_FUNC,		0},
/* 106 */    {ERx("IItvalrow"),		0,		0,		0},
/* 107 */    {ERx("IItvalval"),		gen_all_args,	G_FUNC,		1},
/* 108 */    {ERx("IITBceColEnd"),	0,		0,		0},
/* 109 */    {(char *)0,		0,		0,		0},
/* 110 */    {(char *)0,		0,		0,		0},

			/* QA test calls # 111 */
/* 111 */    {ERx("QAmessage"),		gen_var_args,	G_LONG,		13},
/* 112 */    {ERx("QAprintf"),		gen_var_args,	G_LONG,		13},
/* 113 */    {ERx("QAprompt"),		gen_var_args,	G_LONG,		13},
/* 114 */    {(char *)0,		0,		0,		0},
/* 115 */    {(char *)0,		0,		0,		0},
/* 116 */    {(char *)0,		0,		0,		0},
/* 117 */    {(char *)0,		0,		0,		0},
/* 118 */    {(char *)0,		0,		0,		0},
/* 119 */    {(char *)0,		0,		0,		0},

			/* EQUEL cursor routines # 120 */
/* 120 */    {ERx("IIcsRetrieve"),	gen_all_args,	G_FUNC,		3},
/* 121 */    {ERx("IIcsOpen"),		gen_all_args,	0,		3},
/* 122 */    {ERx("IIcsQuery"),		gen_all_args,	0,		3},
/* 123 */    {ERx("IIcsGetio"),		gen_io,		G_RET|G_RTIF,	2},
/* 124 */    {ERx("IIcsClose"),		gen_all_args,	0,		3},
/* 125 */    {ERx("IIcsDelete"),	gen_all_args,	0,		4},
/* 126 */    {ERx("IIcsReplace"),	gen_all_args,	0,		3},
/* 127 */    {ERx("IIcsERetrieve"),	0,		G_RTIF,		0},
/* 128 */    {ERx("IIcsParGet"),	gen_all_args,	0,		3},
/* 129 */    {ERx("IIcsERplace"),	gen_all_args,	0,		3},
/* 130 */    {ERx("IIcsRdO"),		gen_all_args,	0,		2},
/* 131 */    {ERx("IIcsRetScroll"),	gen_all_args,	G_FUNC,		5},
/* 132 */    {ERx("IIcsOpenScroll"),	gen_all_args,	0,		4},
/* 133 */    {(char *)0,		0,		0,		0},
/* 134 */    {(char *)0,		0,		0,		0},

		/* Forms With Clause Routines # 135 */
/* 135 */    {ERx("IIFRgpcontrol"),	gen_all_args,	0,		2},
/* 136 */    {ERx("IIFRgpsetio"),	gen_io,		G_SET,		3},
/* 137 */    {ERx("IIFRreResEntry"),	0,		0,		0},
/* 138 */    {(char *)0,		0,		0,		0},
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
/* 148 */    {ERx("IILQprsProcStatus"),	gen_all_args,	G_FUNC,		1},
/* 149 */    {ERx("IILQprvProcGTTParm"),gen_all_args,   0,              2},
/* 150 */    {(char *)0,                0,              0,              0},

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
** Purpose:	Generates a call which ultimately invokes an II function.
** Parameters:	Function index.
** Returns:	None
** Generates:	Set up args
-*		CALL "IIfunc" USING args
*/

void
gen_call( func )
i4	func;
{
    gen__II( func );
    g_was_if = 0;
}


/*
+* Procedure:	gen_if 
** Purpose:	Generates a simple If statement (Open or Closed).
** Parameters:	o_c   - i4  - Open / Close the If block,
**		func  - i4  - Function index,
**		cond  - i4  - Condition index,
**		val   - i4  - Right side value of condition.
** Returns:	None
** Generates:  	(open)   Set up args
**			 CALL "IIfunc" USING args GIVING IIRES
**		    	 IF IIRES condition val 
-*	   	(close)  END-IF
** 
** VMS COBOL has the END-IF clause, so we do not require runtime evaluation
** of IF conditions.
** MF COBOL also has END-IF and CONTINUE, use of which is controlled by the 
** -u dynamic option (eq->eq_config & EQ_COBUNS disables).
*/

void
gen_if( o_c, func, cond, val )
i4	o_c;						/* Open / Close */
i4	func;
i4	cond;
i4	val;
{
    static	bool old_dot ZERO_FILL;

    if (o_c == G_OPEN)
    {
	gen__II( func );
	old_dot = gc_dot;				/* Save current state */
	gc_dot = FALSE;					/* Cannot have dots */
	/* Don't generate a real COBOL IF for unstructured COBOL (-u)
	** in the case of IInextget or IIcsretrieve gen_ifs.
	** These cases will be handled by a "runtime if" mechanism
	*/
	if (!(             				/* NOT:		*/
	       (eq->eq_config & EQ_COBUNS) &&		/*  -u option and */
	       (func == IINEXTGET || func == IICSRETRIEVE) /* nextget or */
							   /* csretrieve */
	    ))
	{
	    gen__obj( TRUE, ERx("IF IIRES ") );	  	/* If */
	    gen__obj( TRUE, gen_condtab[C_RANGE + cond] );  /* Condition */
	    gen__obj( TRUE, ERx(" ") );
	    gen__int( val );				/* Value */
	    gc_newline();					/* Then */
	}
	gen_do_indent( 1 ); 			/* Always indent for clarity */
	g_was_if = 1;
    }
    else if (o_c == G_CLOSE)
    {
	if (g_was_if)			/* NULL If blocks are illegal */
	{
# ifndef DGC_AOS
	    if ((!eq->eq_config & EQ_COBUNS))   /* Unstructured = DG method */
		gen__obj( TRUE, ERx("CONTINUE\n") );
	    else
# endif /* DGC_AOS */
		/* DG and unstructured option do not have CONTINUE statement */
		gen__obj( TRUE, ERx("MOVE 0 TO II0\n") );
	}
	/* There was no real COBOL IF for unstructured COBOL (-u)
	** in the case of IInextget or IIcsretrieve gen_ifs.
	** These cases will be handled by a "runtime if" mechanism
	*/
	gen_do_indent( -1 );			/* Always indent for clarity */
	if (!(             				/* NOT:		*/
	       (eq->eq_config & EQ_COBUNS) &&		/*  -u option and */
	       (func == IINEXTGET || func == IICSRETRIEVE) /* nextget or */
							   /* csretrieve */
	    ))
	{
	    /* If running with the -u option we cannot use END-IF
	    ** in those cases we use period, except for IINEXEC which
	    ** has nothing generated for it.
	    */
	    if (eq->eq_config & EQ_COBUNS)   		/* -u option */
	    {
		if (func != IINEXEC)
		{
		    /* output a period, which may be on own line */
		    gen__obj( TRUE, ERx(".") );
                }
	    }
	    else
	    {
	    	gen__obj( TRUE, ERx("END-IF") );		/* End */
	    }
	    gc_dot = old_dot;
	    gc_newline();
	}
	else
	{
	    gc_dot = old_dot;				/* Omits newline */
	}
	g_was_if = 0;
    }
}


/*
+* Procedure:	gen_else 
** Purpose:	Generate an If block with Else clauses.
** Parameters:	flag 	- i4  - Open / Else / Close the If block,
**		func 	- i4  - Function index,
**		cond 	- i4  - Condition index,
**		val 	- i4  - Right side value of condition,
**		l_else 	- i4  - Else label prefix,
**		l_elsen	- i4  - Else label number,
**		l_end 	- i4  - Endif label prefix,
**		l_endn 	- i4  - Endif label number.
** Returns:	None
** Generates:
**	(open)		Set up args
**			CALL "IIfunc" USING args GIVING IIRES.
**			IF IIRES not condition val GO TO elselab.
**
**	(else)	       	GO TO endlab.
**   	  	elselab.
**	
**    	(close)	       
**    		endlab.
**
** Currently, only Activate lists generate an If-Then-Else formats (for
** each interrupt value).  The Else label prefix is a special case, as
** one must make the label unique, because there can be nested If-Then-Else
-* formats.  For example a nested menu inside an Activate block.
**
** Labelled language (without If-Then-Else blocks) note:
** 1. To Open an If call the function. Test the result using the Not condition
**    (negative) against the passed value. If True (Not condition) then goto
**    the Else label.
** 2. To Else an If generate a goto to the Endif label.	 Then using the Else
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
    if (flag != G_CLOSE && func != II0)	/* Format the label */
	STprintf( g_lab_else, gen_ltable[l_else], l_endnum, l_elsenum );

    gc_dot = TRUE;			/* We have dots now */
    if (flag == G_OPEN)
    {
	if (func != II0)		/* Otherwise following an Else */
	{
	    /* Not the condition, and do a Goto to Else label */
	    cond = -cond;
	    gen_if_goto( func, cond, val, G_ELSELAB, 0 );
	}
	gen_do_indent( 1 );
	/* Next call should Close or Else */
    }
    else if (flag == G_ELSE)
    {
	gen_term( TRUE );			  /* Dummy terminator */
	gen_do_indent( -1 );
	gen__goto( l_end, l_endnum, TRUE ); 	  /* Goto End Label */
	if (eq_ansifmt)
	    out_emit( gc_mgA );
	gen__label( FALSE, G_ELSELAB, 0 );	  /* Else label (for Activate)*/
	gc_newline();			 	  /* Attach '.' to the label */
    }
    else
    {
	gen_term( TRUE );			  /* Dummy terminator */
	gen_do_indent( -1 );
	gen_label( G_NOLOOP, l_end, l_endnum );		/* Endif Label */
    }
    g_was_if = 0;
}


/*
+* Procedure:	gen_if_goto 
** Purpose:	Generate an If Goto template.
** Parameters:	func 	- i4  - Function index,
**		cond 	- i4  - Condition index,
**		val 	- i4  - Right side value of condition,
**		labpref	- i4  - Label prefix,
**		labnum 	- i4  - Label number.
** Returns:	None
** Generates:	Set up args
**		CALL "IIfunc" USING args GIVING IIRES.
-*		IF IIRES cond val GO TO label.
*/

void
gen_if_goto( func, cond, val, labprefix, labnum )
i4	func;
i4	cond;
i4	val;
i4	labprefix;
i4	labnum;
{
    gc_dot = TRUE;
    gen__II( func );
    gen__obj( TRUE, ERx("IF IIRES ") );			/* If */
    gen__obj( TRUE, gen_condtab[C_RANGE + cond] );	/* Condition */
    gen__obj( TRUE, ERx(" ") );
    gen__int( val );					/* Value */
    gen__obj( TRUE, ERx(" ") );				/* Then */
    gen__goto( labprefix, labnum, TRUE );		/* Goto Label */
    g_was_if = 0;
}


/*
+* Procedure:	gen_loop 
** Purpose:	Dump a while loop (open the loop or close it).
** Parameters:	o_c	- i4  - Open / Close the loop,
**		labbeg	- i4  - Start label prefix,
**		labend	- i4  - End label prefix,
**		labnum	- i4  - Loop label number.
**		func	- i4  - Function index,
**		cond	- i4  - Condition index,
**		val	- i4  - Right side value of condition.
** Returns:	None
** Generates:
**	(open)	openlab.
**			CALL "IIfunc" USING args GIVING IIRES.
**			IF IIRES cond val GO TO endlab.
**		
**	(close)		GO TO openlab.
-*		endlab.
**
** Labelled language note:
** 1. To open a loop, dump the opening label.  Call the function. Test the
**    result using the Not (negative) condition against the passed value. If
**    True then goto the closing label.
** 2. To close a loop, dump a goto to the openning label.  Now dump the
**    closing label.
** 3. MF COBOL has PERFORM ... END_PERFORM loop.
** 4. Edited to allow EXECUTE PROCEDURE (IIPROCSTAT) in a loop.  Format is:
**	CALL "IIPROCSTAT" GIVING IIRES.
**	PERFORM UNTIL IIRES NOT= 0
**		code
**		CALL IIPROCSTAT GIVING IIRES
**	END-PERFORM.
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
# ifndef DGC_AOS
    if ( ((eq->eq_config & EQ_COBUNS) == 0)      &&	/* No -u option and  */
	 (func == IINEXEC || func == IIPROCSTAT)	/* Real PERFORM loop */
       )
    {
	static	bool old_dot ZERO_FILL;

	if (o_c == G_OPEN)
	{
	    gen__obj( TRUE, ERx("MOVE ") );	/* Set up loop control */
	    gen__int( val );
	    gen__obj( TRUE, ERx(" TO IIRES") );
	    gc_newline();
	    if (func == IIPROCSTAT)
		gen__II(func);
	    old_dot = gc_dot;			/* Save current state */
	    gc_dot = FALSE;			/* No dots till end of loop */
	    gen__obj( TRUE, ERx("PERFORM UNTIL IIRES ") );	/* Real loop */
	    gen__obj( TRUE, gen_condtab[C_RANGE - cond] );	/* Condition */
	    gen__obj( TRUE, ERx(" ") );
	    gen__int( val );
	    gc_newline();
	    gen_do_indent( 1 );
	}
	else
	{
	    if (func == IIPROCSTAT)
		gen__II(func);
	    gen_do_indent( (-1) );
	    gen__obj( TRUE, ERx("END-PERFORM") );
	    gc_dot = old_dot;
	    gc_newline();
	}
	return;
    }
# endif /* DGC_AOS */
    /* Loop must be simulated by labels and gotos */
    if (o_c == G_OPEN)
    {
	/* Begin label, Not the condition, and do a Goto to Endloop */
	gen_label( G_NOLOOP, labbeg, labnum );
	gen_if_goto( func, -cond, val, labend, labnum );
	gen_do_indent( 1 );
    }
    else
    {
	/* Goto Begin label, and dump Endloop label */
	gen_term( TRUE );			  /* Dummy terminator */
	gen_do_indent( -1 );
	gen__goto( labbeg, labnum, TRUE );
	gen_label( G_NOLOOP, labend, labnum );
    }
    if (eq->eq_config & EQ_COBUNS)
	gc_dot = FALSE;		/* Bug 57281 - for -u, periods remain off */
				/* This is how it was before fix for
				** bug 31455 was introduced */
    else
	gc_dot = TRUE;		/* Bug 31455 - turn periods on */
    g_was_if = 0;
}


/*
+* Procedure:	gen_goto 
** Purpose:	Put out a Goto statement.
** Parameters:	flag 	  - i4  - G_IF     - Requires an If around it.
**			    	 G_TERM   - Absolute Goto can be terminated.
**			    	 G_NOTERM - None of the above.
**		labprefix - i4  - Label prefix,
**		labnum    - i4  - Label number.
** Returns:	None
-* Generates:  	[ IF (II0 = 0) ] GO TO label [ END-IF ]
**
** Note that this routine is called from the grammar, and may need the If
** clause to prevent compiler warnings.	 The 'flag' tells the routine
** that this is a 'loop break out' statement (ie: ## endloop) and requires
** the If clause.  Internally generated gen_goto() calls will use G_NOLOOP.
** If clause not generated when compiling with -u (unstructured) in order to
** avoid the END-IF. COBOL compilers do not object the same as C ones anyway.
**
**	G_IF&&not -u	- IF (II0 = 0) GO TO label END-IF
**	G_TERM		- GO TO label.
**	G_NOTERM (else)	- GO TO label
*/

void
gen_goto( flag, labprefix, labnum )
i4	flag;
i4	labprefix;
i4	labnum;
{
    if ( (flag == G_IF) && ( !(eq->eq_config & EQ_COBUNS) ) )
	gen__obj( TRUE, ERx("IF (II0 = 0) ") );

    gen__goto( labprefix, labnum, FALSE );

    if (flag == G_TERM)
	gc_dot = TRUE;
    else if( (flag == G_IF) && ( !(eq->eq_config & EQ_COBUNS) ) )
    	gen__obj( TRUE, ERx(" END-IF") );
    gc_newline();
    g_was_if = 0;
}

/*
+* Procedure:	gen__goto 
** Purpose:	Local statement independent goto generator.
** Parameters:	labprefix - i4  - Label prefix,
**		labnum    - i4  - Label number.
**		period	  - i4  - Can we put out a period.
-* Returns:	None
*/

void
gen__goto( labprefix, labnum, period )
i4	labprefix;
i4	labnum;
i4	period;
{
    gen__obj( TRUE, ERx("GO TO ") );		  /* Goto */
    gen__label( TRUE, labprefix, labnum );	  /* Label */
    if (period)  
    {
	gc_dot = TRUE;
	gc_newline();
    }
}

/*
+* Procedure:	gen_label 
** Purpose:	Put out a constant label.
** Parameters:	loop_extra - i4  - Is it extra label for a loop ? (See usage 
**				  in main grammar: Languages that have labelled
**				  loops should ignore this kind of label.)
**		labprefix  - i4  - Label prefix,
**		labnum	   - i4  - Label number.
** Returns:	None
-* Generates:	label.
**
** COBOL labels are really considered paragraph names.  When they serve as a 
** place marker, they must begin in left margin and be followed by a period.
*/

void
gen_label( loop_extra, labprefix, labnum )
i4	loop_extra;
i4	labprefix;
i4	labnum;
{
    if (loop_extra == G_LOOP)	     	/* Do not print extra loop labels */
	return;
    gen_term( TRUE );	     		/* Terminate sentence before label */
    if (eq_ansifmt)
    {
	/* Pad out at least until Area A */
	if (out_cur->o_charcnt == G_SEQLEN)
	    out_emit( ERx(" ") );	/* Assume sequence no. already output */
	else if (out_cur->o_charcnt < G_SEQLEN)
	    out_emit( gc_mgA );		/* Adjust output line to margin A */
    }	
    gen__label( FALSE, labprefix, labnum );
    gc_dot = TRUE;
    gc_newline();
    g_was_if = 0;
    g_was_label = 1;
}

/*
+* Procedure:	gen__label 
** Purpose:	Local statement independent label generator.
** Parameters:	ind	- i4  - Indentation flag (FALSE if must be at left), 
**		labpref - i4  - Label prefix,
**		labnum	- i4  - Label number (special case G_ELSELAB).
-* Returns:	None
*/

void
gen__label( ind, labprefix, labnum )
i4	ind;		    /* Allow indentation or leftmost margin */
i4	labprefix;
i4	labnum;
{
    char	buf[16];

    if (labprefix == G_ELSELAB)			/* Special Else label */
    {
	gen__obj( ind, g_lab_else );
	return;
    }
    gen__obj( ind, gen_ltable[labprefix] );	/* Label */
    /* Label prefixes and their numbers must be atomic */
    CVna( labnum, buf );
    out_emit( buf );
}

/*
+* Procedure:	gen_comment 
** Purpose:	Make a comment with the passed string. Should be used
**		by higher level calls, but may be used locally.
** Parameters:	term - i4	- Comment terminated with newline (unused),
**		str  - char *	- Comment string.
** Returns:	None
-*
** Notes:
** VMS COBOL comments begin with an asterisk in column 1 for terminal format,
** in column 7 for card format.
*/

void
gen_comment( term, str )
i4	term;			/* Unused */
char	*str;
{
    out_emit( ERx("\n") ); 	/* Lets try for at least 1 blank line between */
    if (eq_ansifmt)
	gen__obj( FALSE, ERx("      *-- ") );
    else
	gen__obj( FALSE, ERx("*-- ") );
    out_emit( str );
    out_emit( ERx("\n") );
}

/*
+* Procedure:	gen_include 
** Purpose:	Generate an include file statement.
** Parameters:	pre, suff - char * - Prefix, and suffix separated by a '.',
**		extra     - char * - Any extra qualifiers etc.
** Returns:	None
-* Generates:	COPY "filename".	   
*/

void
gen_include( pre, suff, extra )
char	*pre;
char	*suff;
char	*extra;
{
    char  buf[MAX_LOC +1];

    /* The whole COPY line should be atomic */
    if (eq_ansifmt)
    {
	if (dml->dm_lang != DML_ESQL)	/* EQUEL doesn't support seqno yet */
	{
	    out_emit( gc_mgB );
	}
	else
	{
	    (VOID) Cput_seqno();	/* Emit ANSI COBOL sequence number */
	    if (out_cur->o_charcnt == G_SEQLEN)
	        out_emit( gc_mgCtoB );
	    else
	        out_emit( gc_mgB );	/* No sequence number in buffer */
	}
    }	
    else
    {
	out_emit( ERx("\t") );
    }

/* Following code fragment stores the copy command into a buffer */
/* and then uses gen_sconst to generate it in cobol line lengths */

    out_emit("COPY ");
    STcopy(pre,buf);
    if (suff != (char *)0)
    {
        STcat(buf,".");
        STcat(buf,suff);
    }
    if (extra != (char *)0)
        STcat(buf,extra);
    gen_sconst(buf);

    gc_dot = TRUE;
    gc_newline();			/* Must have period */
}

/*
+* Procedure:	gen_eqstmt 
** Purpose:	Open / Close an Equel statement.
** Parameters:	o_c  - i4  - Open / Close,
**		cmnt - i4  - Comment.
-* Returns:	None
*/

void
gen_eqstmt( o_c, cmnt )
i4	o_c;
char	*cmnt;
{
    if (o_c == G_OPEN)
	gen_line( cmnt );
# if defined(MF_COBOL) || defined(DEC_COBOL)
    else if (g_was_label)	/* Avoid empty labels before closure */
    {
        if ((!eq->eq_config & EQ_COBUNS))   /* Unstructured = DG method */
            gen__obj( TRUE, ERx("CONTINUE") );
        else
            gen__obj( TRUE, ERx("MOVE 0 TO II0") );
	gc_newline();
    }
#endif /* MF_COBOL */
}

/*
+* Procedure:	gen_line 
** Purpose:	Generate a comment about where we are.
** Parameters:	s - char * - Optional comment string.
** Returns:	None
-* Generates:	* File X.QCB, Line 10 -- COMMAND
*/

void
gen_line( s )
char	*s;
{
    char	fbuf[G_MAXSCONST +16];
    char	sbuf[50];

    STprintf( fbuf, ERx("File %s%s%s, Line %d"), eq->eq_filename, 
	eq->eq_flags & EQ_STDIN ? ERx("") : ERx("."), eq->eq_ext, eq->eq_line );
    if (s)
    {
	STcopy( s, sbuf );
	CVupper( sbuf );
	STcat( fbuf, ERx(" -- ") );
	STcat( fbuf, sbuf );
    }
    gen_comment( TRUE, fbuf );
}

/*
+* Procedure:	gen_term
** Purpose:	Put out a terminator to properly syncronize statements.
** Parameters: 	dot - bool - Flag to tell whether to turn off gc_dot.
-* Returns:	None
**
** Notes:
** 1. If gc_dot is ON then do not put out anything as the last
**    statement had a period.  Otherwise resort to a "CONTINUE." statement.
** 2. For buffered queries (REPEAT and CURSOR), we call gen_term(0) to turn
**    off the dot.
*/

void
gen_term( dot )
bool	dot;
{
    if (!dot)		/* Turn off the dot */
    {
	gc_dot = FALSE;
	return;
    }
    /* If we have just put out a statement with a period then do not update */
    if (!gc_dot)
    {
# ifndef DGC_AOS
        if ((!eq->eq_config & EQ_COBUNS))   /* Unstructured = DG method */
	   gen__obj( TRUE, ERx("CONTINUE") );
        else
# endif /* DGC_AOS */
	/* DG and unstructured option do not have CONTINUE statement */
	   gen__obj( TRUE, ERx("MOVE 0 TO II0") );
	gc_dot = TRUE;
	gc_newline();
    }
}

/*
+* Procedure:	gen_declare 
** Purpose:	Process a ## DECLARE statement.
** Parameters:	None
** Returns:	None
** Generates:	COPY "ingres_path_eqdef.cob".
**  If Debug:
**		01 IIDBG.
**		    02 FILLER PIC X(7) VALUE IS "neil.qc".
-*		    02 FILLER PIC X VALUE LOW-VALUE.  
*/

void
gen_declare()
{
    LOCATION	loc; 
    char	buf[MAX_LOC +1];
    char	*lname;
    i4		len;

    /* Spit out real Include line */
# if defined(MF_COBOL) || defined(DEC_COBOL)
    STcopy( ERx("eqmdef.cbl"), buf );
#else
    STcopy( ERx("EQDEF.COB"), buf );
#endif
    NMloc( FILES, FILENAME, buf, &loc );
    LOtos( &loc, &lname );
#if !defined(MF_COBOL) && !defined(DEC_COBOL)
    CVupper( lname );		/* In a global buffer */
#endif
    gen_comment( TRUE, ERx("EQUEL run-time communications area.") );
    gen_include( lname, (char *)0, (char *)0 );

    /* Put out IIDBG structure */
    if (eq->eq_flags & EQ_IIDEBUG)
    {
	gen_comment( TRUE, ERx("EQUEL -d run-time flag.") );
    	STprintf( buf, ERx("%s%s%s"), eq->eq_filename, 
		  eq->eq_infile == stdin ? ERx("") : ERx("."), eq->eq_ext );
	len = (i4)STlength( buf );
	gen__obj( TRUE, ERx("01 IIDBG.\n") );
	gen_do_indent( 1 );
	gen__obj( TRUE, ERx("02 FILLER PIC X(") );
	gen__int( len );
	gen__obj( TRUE, ERx(") VALUE IS \"") );
	out_emit( buf );
	gen__obj( TRUE, ERx("\".\n") );
	gen__obj( TRUE, ERx("02 FILLER PIC X VALUE LOW-VALUE.\n") );
	gen_do_indent( (-1) );
    }
    /* Now for some trickery ! */
    NMgtAt( ERx("II_COBOL_WHO"), &lname );
    if (lname != (char *)0 && *lname != '\0')
    {
	SIprintf( ERx("## EQUEL/COBOL\n") );
	SIprintf( ERx("## Rewritten by Neil C. Goodman (RTI), May 1985.\n") );
	SIflush( stdout );
    }
}


/*
+* Procedure:	gen_makesconst 
** Purpose:	Make a host language string constant.
** Parameters:	flag  -	char * - G_INSTRING:  Nested in another string (DB 
**					      string),
**				 G_REGSTRING: String sent by itself in a call.
**		instr - char * - Incoming string.
**		diff  - i4  *   - Mark if there were changes made to string.
-* Returns:	char * 	       - Pointer to static converted string.
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
    static	char	obuf[ G_MAXSCONST + 4 ];	/* Converted string */
    register	char	*icp;				/* To input */
    register	char	*ocp = obuf;			/* To output */
    register	i4	esc = 0;			/* \ was used */

    *diff = 0;

    if (flag == G_INSTRING)		/* Escape the first quote */
    {
	*ocp++ = '"';
	*ocp++ = '"';
	*diff = 1;
    }
    for (icp = instr; *icp && (ocp < &obuf[G_MAXSCONST]); )
    {
	if (*icp == '\\')		/* Leave '\' - may be DB or \0 forms */
	{	
	    *ocp++ = '\\';
	    esc = 1;
	    icp++;
	}
	else if (*icp == '"')
	{
	    if (flag == G_INSTRING && !esc)	/* DB needs escape */
		*ocp++ = '\\';
	    *ocp++ = '"';			/* Escape the quote for COBOL */
	    *ocp++ = '"';
	    icp++;				/* Skip over the quote */
	    if (*icp == '"')			/* Skip if already doubled */
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
** Purpose:	Generate a call to a run-time interface, passing the II 
**		function number and the necessary arguments, already stored 
**		by argument manager.
** Parameters:	func    - i4  - Function index.
** Returns:	None
** Generates:	[ MOVE ? TO IIarg ] ...	Set up args
**		CALL "IIfunc" USING args [GIVING IIRES]
**
** This routine assumes language dependency, and knowledge of the actual
-* arguments required by the call.
**
** History:
**	10-dec-1992 (kathryn)
**	    Add gen_datio to special case which may require IIXLQ prefix for
**	    passing string descriptors.
**	24-dec-2003 (abbjo03)
**	    Copy g->g_IIname to a local array to prevent access violation when
**	    calling CVupper().
*/

void
gen__II( func )
i4	func;
{
    register	GEN_IIFUNC	*g;
    i4		ret_val = 1;
    char	callbuf[GC_MFCALL_LEN+20];	/* For MAX CALL names */
    char	iiname[GL_MAXNAME+1];

# ifdef DGC_AOS
    /*  ALL DG calls need to go through the layer */
    gc_fprefix = ERx("IIX");		
# else
    gc_fprefix = ERx("II");		/* Reset by g->g_func, maybe */
# endif /* DGC_AOS */
    g = gen__functab( func );
    STcopy(g->g_IIname, iiname);
    while (ret_val)
    {
	    gc_args( GCmFREE, (char *)0 );
# ifdef DGC_AOS
    	    /* DG does not support GIVING so call using IIRES as a parameter */
	    if (g->g_flags & G_FUNC)
	         gc_args( GCmREF, ERx("IIRES") );
# endif /* DGC_AOS */
# if defined(MF_COBOL) || defined(DEC_COBOL)
    	    /*
	    ** MF does not support GIVING so call using IIRES as a parameter
	    ** and modify the call name (IIX) to go through function interface.
	    */
	    if (g->g_flags & G_FUNC)
	    {
	        gc_args( GCmREF, ERx("IIRES") );
		gc_fprefix = ERx("IIX");		
	    }
	    /* 
	    ** If generating unstructured code, modify further routines
	    ** names (IIX) to go through run-time IF function interface.
	    */
	    if ( (g->g_flags & G_RTIF) && (eq->eq_config & EQ_COBUNS) )
		gc_fprefix = ERx("IIX");		

# endif /* MF_COBOL */

	    if (g->g_func)
	    {
# ifdef DGC_AOS
		/* ret_val is TRUE if need to call due to a long I/O string */
		if (func == IIWRITEDB)
		{
		   ret_val = (*g->g_func)( func, g->g_flags, g->g_numargs );
		}
		else
# endif /* DGC_AOS */
		{
		   _VOID_ (*g->g_func)( func, g->g_flags, g->g_numargs );
		   ret_val = 0;
		}
	    }
	    else
	    {
	       ret_val = 0;
	    }
	    /*
	    ** Any MOVE statements that were required are set up, with the 
	    ** real arguments buffered by gc_args. Call the required routine 
	    ** using all buffered arguments. If this is a function call then 
	    ** terminate with GIVING IIRES.
	    */
	
	    gen__obj( TRUE, ERx("CALL \"") );
#if defined(MF_COBOL) || defined(DEC_COBOL)
	    /* Truncate CALL names so <= MAX length */
	    STprintf(callbuf, ERx("%s%s"), gc_fprefix, iiname + 2);
	    callbuf[GC_MFCALL_LEN] = EOS;
	    out_emit(callbuf);
#else
	    if (g->g_func == gen_io ||
                g->g_func == gen_datio )  /* Name may have been changed */
	    {
		out_emit( gc_fprefix );
	    	CVupper(iiname + 2);
		out_emit(iiname + 2);
	    } 
	    else				/* Name may start with II, QA */
	    {
# ifdef DGC_AOS
		/* All DG call will go throught the layer so prefix = 'IIX' */
		out_emit( gc_fprefix );
		CVupper(iiname + 2);
		out_emit(iiname + 2);
# else
		CVupper(iiname);
		out_emit(iiname);
# endif /* DGC_AOS */
	    }
#endif /* MF_COBOL */
	    out_emit( ERx("\"") );
	
# ifndef VMS	/* MF and DG require IIRES */
	    /* Need to dump if a function because pushed IIRES on stack as 
	    ** parameter and numargs may be 0.  For DG and MF.
	    */
	    if ((g->g_flags & G_FUNC) || (g->g_numargs > 0))
		gc_args( GCmDUMP, (char *)0 );
# else
	    if (g->g_numargs > 0)		/* Dump arguments if any */
		gc_args( GCmDUMP, (char *)0 );

	    if (g->g_flags & G_FUNC)
	       gen__obj( TRUE, ERx(" GIVING IIRES") );
# endif /* VMS */
	    gc_newline();
	
	    /*
	    ** In case of an Output function do a reverse MOVE if there was any
	    ** dummy II argument used.
	    **
	    ** For MF we may need both (such as is done for COMP with scale):
	    **		CALL USING IIF8
	    **		CALL "IIMFF8TOPK" USING IIF8 IIPK.
	    **		MOVE IIPK TO <COMP-var>.
	    ** and therefore the order of the reverse move is important.
	    */
	    if (g->g_flags & (G_RET|G_RETSET))
	    {
#if defined(MF_COBOL) || defined(DEC_COBOL)
		/* Move indicator back to user - see gc_retmove comments */
		if (gc_retnull)
		    gc_move(FALSE, ERx("IIIND"), gc_retnull);
#endif /* MF_COBOL */
		gc_comp( 0, (char *)0, (char *)0, (COB_SYM *)0 );
		gc_retmove( 0, (char *)0, (char *)0 );
		gc_retnull = (char *)0;
    	    }
	}
        arg_free();
}
		
/*
+* Procedure:	gen_all_args 
** Purpose:	Format all the arguments stored by the argument manager.
** Parameters:	func    - i4  - Function index (unused),
**		flag    - i4  - Flags to lower level gen_data(),
**		numargs - i4  - Number of arguments required.
-* Returns:	0
**
** This routine should be used when no validation of the number of arguments
** needs to be done. Hopefully, if there was an error before (or while)
** storing the arguments this routine will not be called.
*/

i4
gen_all_args( func, flag, numargs )
i4	func, flag, numargs;
{
    register	i4 i;

    for (i = 1; i <= numargs; i++)	/* Fixed number of arguments */
	gen_data( flag, i );
    return 0;
}

/*
+* Procedure:	gen_var_args 
** Purpose:	Format varying number of arguments stored by the argument
**		manager.
** Parameters:	func    - i4  - Function index (unused),
**		flag    - i4  - Flags to lower level gen_data(),
**		numargs - i4  - Max number of arguments required.
-* Returns:	0
**
** As gen_all_args() this routine should be used when no validation of
** the number of arguments needs to be done.
*/

i4
gen_var_args( func, flag, numargs )
i4	func, flag, numargs;
{
    register	i4 i;
    char	*arg_num;

#ifdef DGC_AOS
    arg_num = gc_cvla( arg_count());	/* get argument count */
    gc_move( FALSE, arg_num, gc_nxti4(GCmREF) ); 
# endif /* DGC_AOS */

#if defined(MF_COBOL) || defined(DEC_COBOL)
	/*
	 * Generate argument count and prefix "IIX" for
	 * IIsqConnect, IIingopen etc.
	 */
	arg_num = gc_cvla( arg_count());      /* get argument count */
	gc_move( FALSE, arg_num, gc_nxti4(GCmVAL) );
	gc_fprefix = ERx("IIX");
#endif

    while (i = arg_next())		/* Varying num of arguments */
	gen_data( flag, i );
    gc_args( GCmOMIT, (char *)0 );	/* Null terminate */
    return 0;
}

/*
+* Procedure:	gen_data 
** Purpose:	Whatever is stored by the argument manager for a particular
**	      	argument, dump to the output file.
** Parameters:	flag - i4  - Flags being sent with argument.
**		arg  - i4  - Argument number.
** Returns:	None
**
** This routine should be used when no validation of the data of the argument
** needs to be done. Hopefully, if there was an error before (or while)
** storing the argument this routine will not be called.
**
** Decide if "MOVE" is necessary, based on the following rules:
**
** COBOL type		Length	Scale	Eqtype	MOVE(data)	MOVE(I/O)
**
** COMP			1-4	0	i2	N (as i4)	N (as i2)
**			5-9	0	i4	N (as i4)	N (as i4)
**			10-18	0	f8	-Y (as f8)	Y (as f8)
**			any	1	f8	-Y (as f8)	Y (as f8)
** COMP-1				f4	-N (as f4)	N (as f4)
** COMP-2				f8	-N (as f8)	N (as f8)
** COMP-3		1-10	0	i4 	Y  (as i4)	N (as packed)
**			otherwise	packed  -N (as packed)  N (as packed)
**			otherwise	f8	-Y (as f8)	Y (as f8)
** Numeric Display	1-9	0	i4	Y (as i4)	Y (as i4)
**			otherwise	f8	-Y (as f8)	Y (as f8)
** Numeric Edited	1-9	0	i4	Y (as i4)	Y (as i4)
**			otherwise	f8	-Y (as f8)	Y (as f8)
** INDEX				i4	N (as i4)	N (as i4)
**
** Notes:
**   1. MOVE(data) columns marked with a '-' are error conditions that are 
**	caught earlier but we still must process.
-*   2. Unsigned COMP's are treated in exactly the same way as signed COMP's.
**   3. For DG, GCmVAL has been redefined to GCmREF.  All arguments must
**      be passed by reference.
**   4. For MF we do some global moving -- see cobtypes.c and details here.
*/

void
gen_data( flag, arg )
i4		flag;
register i4	arg;
{
    i4		len;			/* Length of strings */
    i4		num;			/* i4 and f8 work variables */
    register	char	*sdata;		/* Name of argument or variable */
    i4		artype;			/* Argument type */
    SYM		*var;			/* If SYM then user variable */
    register	COB_SYM	*cs;		/* COBOL variable information */
    char	*ch_num;		/* char representation of a number */

    if (var = arg_sym(arg))		  /* User variable */
    {
	sdata = arg_name( arg );
	cs = (COB_SYM *)sym_g_vlue( var );
	switch ( COB_MASKTYPE(cs->cs_type) )
	{
	  case COB_COMP:
	  case COB_4:		/* Not VMS */
	    if (cs->cs_nscale || cs->cs_nlen > COB_MAXCOMP)
	    {
		/* ERROR in this context - has scale or too big - cannot use */
# ifdef DGC_AOS
		gc_args( GCmREF, ERx("II0") );
# else
		gc_args( GCmVAL, ERx("0") );
# endif /* DGC_AOS */
	    }
# ifdef DGC_AOS
    	    else if (cs->cs_nlen <= COB_MAXI3)
	    {
		/* 
		** If S9(1) - S9(6) USAGE COMP, then move to i4. 
		** Only need to move for DG because DG always passes
		** by reference, while VMS passes by value.
		*/

		gc_move( FALSE, sdata, gc_nxti4(GCmREF) );
    	    }
# endif /* DGC_AOS */

	    else if (cs->cs_nlen == COB_MAXCOMP)   
	    {
		/* Move to i4 - VMS allows MAXINT into an S9(9) USAGE COMP */
		gc_move( FALSE, sdata, gc_nxti4(GCmVAL) );
	    }
	    else		/* Pass an as i4 By Value */
	    {
# if defined(MF_COBOL) || defined(DEC_COBOL)
		/* Move all COMP's to i4 for MF */
		gc_move( FALSE, sdata, gc_nxti4(GCmVAL) );
# else
		gc_args( GCmVAL, sdata );
# endif /* MF_COBOL */
	    }
	    break;

	  case COB_INDEX:		/* Is an i4 - use By Value */
	    gc_args( GCmVAL, sdata );
	    break;

	  case COB_1:		/* Errors in this context - pass By Reference */
	  case COB_2:
	    gc_args( GCmREF, sdata );
	    break;

	  case COB_3:
	  /*
	  ** For backwards compatibility, treat a zero scale decimal 
	  ** as a integer here.
	  */
	  case COB_EDIT:
	  case COB_NUMDISP:
	    if (cs->cs_nscale || cs->cs_nlen > COB_MAXCOMP)
	    {
		/* ERROR in this context - has scale or too big - cannot use */
# ifdef DGC_AOS
		gc_args( GCmREF, ERx("II0") );
# else
		gc_args( GCmVAL, ERx("0") );
# endif /* DGC_AOS */
	    }
	    else			/* Move to an i4 */
		gc_move( FALSE, sdata, gc_nxti4(GCmVAL) );
	    break;

	  case COB_DISPLAY:	    	/* User string variable */
	    len = cs->cs_slen;
	    if (   (len > GC_IISLEN)			/* Use long buffer? */
		&& (flag & (G_LONG|G_DYNSTR)))
	    {
		if (   (flag & G_LONG)			/* Use long */
		    || (len <= GC_IILLEN))		/* Dyn str short enuf */
		{
		    gc_move(FALSE, sdata, gc_nxtlch(GCmREF));
# ifdef DGC_AOS
		    /* 
		    ** Pass a zero length to indicate to layer a null-terminated
		    ** string 
		    */
	            gc_args( GCmREF, ERx("II0") );
# endif /* DGC_AOS */
		}
		else					
		{
# ifdef DGC_AOS
	            /* 
		    ** On DG, the character variable is larger than can fit in 
		    ** the long buffer, so will pass the variable and length.
		    ** The runtime layer will dynamically allocate space to
		    ** null terminate the string before passing to LIBQ.
		    ** This is only necessary for the PREPARE and DIRECT
		    ** EXECUTE commands.
		    */
	            gc_args( GCmREF, sdata );
	            ch_num = gc_cvla( len );
	            gc_move( FALSE, ch_num, gc_nxti4(GCmREF) );
# else
#   if defined(MF_COBOL) || defined(DEC_COBOL)
		    gc_mfsdcall(len, sdata);         /* Dyn str needs IIMFsd */
#   else
		    gc_sdcall(TRUE, TRUE, sdata);    /* Dyn str needs IIsd */
#   endif /* MF_COBOL */
# endif /* DGC_AOS */
		}
	    }
	    else					/* Use short buffer */
	    {
		gc_move( FALSE, sdata, gc_nxtch(GCmREF) );
	    }
	    break;

	  case COB_NOTYPE:
	  case COB_RECORD:
	  default:
	    /* ERROR in this context has been caught earlier */
# ifdef DGC_AOS
	    gc_args( GCmREF, ERx("II0") );
# else
	    gc_args( GCmVAL, ERx("0") );
# endif /* DGC_AOS */
	    break;
	}
	return;
    }
    /* Based on the data representation, dump the argument */
    sdata = (char *)0;
    if ((artype = arg_rep(arg)) == ARG_CHAR)
    {
	artype = arg_type( arg );
	sdata = arg_name( arg );
    }
    switch (artype)
    {
      case ARG_INT:
	if (sdata == (char *)0)		/* Real integer */
	{
	    arg_num( arg, ARG_INT, &num );
	    sdata = gc_cvla( num );
	}
# ifdef DGC_AOS
	/* Need to pass by reference on DG */
	gc_move( FALSE, sdata, gc_nxti4(GCmREF) );
# else
	gc_args( GCmVAL, sdata );
# endif /* DGC_AOS */
	break;

      case ARG_FLOAT:		
	/* 
	** Floats can only be used in I/O statements so this must be an error.
	** Just pass 0 By Value.
	*/
# ifdef DGC_AOS
        gc_args( GCmREF, ERx("II0") );
# else
	gc_args( GCmVAL, ERx("0") );
# endif /* DGC_AOS */
	break;

      case ARG_CHAR:			/* String of characters */
	if (sdata == (char *)0)		/* Null string */
	    gc_args( GCmOMIT, (char *)0 );
	else
	{
	    len = (i4)STlength( sdata );
#if defined(MF_COBOL) || defined(DEC_COBOL)
	    if (len > GC_MFSLEN && (flag & (G_LONG|G_DYNSTR)))
		gc_mfstrmove(TRUE, sdata, gc_nxtlch(GCmREF));
	    else
#endif /* MF_COBOL */
	    if (len>GC_IISLEN && (flag & (G_LONG|G_DYNSTR))) /* Use long buf */
		gc_move( TRUE, sdata, gc_nxtlch(GCmREF) );
	    else					/* Use short buffer */
		gc_move( TRUE, sdata, gc_nxtch(GCmREF) );
	}
	break;

      case ARG_RAW:
	gc_args( GCmREF, sdata );
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
**	01-mar-1991 - (kathryn) 
**		Written.
**	01-apr-1991 - (kathryn)
**		Changed Handler error type to non-FATAL.
**	15-apr-1991 (kathryn)
**		Remove gen__obj of "," from args list.
**	08-dec-1992 (lan)
**	    Modified to generate correct arguments for IILQldh_LoDataHandler.
**      03-Nov-2000 (ashco01) Bug: 103099
**          Changed declaration of buf & argbuf to 'static' to prevent
**          possible corruption of the arguments they contain when their
**          memory was released on return. This prevents potential re-use
**          of this memory before the formatted COBOL statement and 
**          associated arguments are written to the output file.
*/
i4
gen_hdl_args(func, flag, numargs )
i4      func, flag, numargs;
{
    i4	        arg;
    static char	buf[ G_MAXSCONST +1 ];
    static char	argbuf[ G_MAXSCONST +1 ];

    gen_data(flag,1);

    arg = 2;
    if (flag == G_DAT)
    {
	gen_null(flag, arg);
	arg++;
    }
    STprintf(buf,ERx("%s"),arg_name(arg));
    switch (arg_type(arg))
    {
        case ARG_INT:
    		if (STcompare(buf,ERx("0")) == 0)
			gc_args( GCmOMIT, (char *)0 );
		else
    		    er_write(E_EQ0090_HDL_TYPE_INVALID, EQ_ERROR, 1, buf);
    		break;
        case  ARG_RAW:
		gc_args( GCmVAL, buf );
    	        break;
        default:
    		er_write(E_EQ0090_HDL_TYPE_INVALID, EQ_ERROR, 1, buf);
		break;
    }
    arg++;
    if (flag == G_DAT)
    {
    	STprintf(argbuf,ERx("%s"),arg_name(arg));
	if (STcompare(argbuf, ERx("0")) == 0)
		gc_args( GCmOMIT, (char *)0 );
	else
		gc_args( GCmVAL, argbuf );
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
**      01-oct-1993 (kathryn)
**          First arg generated for GET/PUT DATA stmts ("attrs" mask) is the
**          last arg stored.  It is now set/stored in eqgen_tl(), rather than
**          determined here. No longer call gen_iodat to generate the I/O
**	    parameter - move required code from that routine to this one.
**	13-dec-1993 (mgw) Bug #56878
**	    Make desc_buf[] static since it's address will be passed out.
*/
i4
gen_datio( func, flag, args )
i4              func;
register i4     flag;
register i4     args;
{
    register i4         ar;
    register SYM        *sy;
    COB_SYM		*cs;
    char                *varname;
    char                *temp_ptr;
    static    char 	desc_buf[50];
    i4			attrs;
    i4			type;
    i4			len;
    i4			gcarg;

    arg_num( args, ARG_INT, &attrs );

    ar = 1;
    varname =  arg_name( ar );

    if (!(attrs & II_DATSEG))
    {
	type = 0;
	len = 0;
	gcarg = GCmOMIT;
    }
    else if (sy = arg_sym(ar))
    {
	/* Only char types allowed - already checked in eqtarget */

	cs = (COB_SYM *)sym_g_vlue( sy );
	len = cs->cs_slen;

# if !defined(MF_COBOL) && !defined(DEC_COBOL)
        type = T_CHAR;
	gcarg = GCmDESC;
	gc_fprefix = ERx("IIX");
#else
	type = T_CHA;
        gcarg = GCmREF;    /* Maybe error earlier - let it slide here */
#endif

    }
    else  /* String literal on PUT DATA stmt */
    {
#if defined(MF_COBOL) || defined(DEC_COBOL)
        /* First figure original string length to avoid extra run-time blanks */
        for (len = 0, temp_ptr = varname; *temp_ptr; temp_ptr++, len++)
        {
          if (*temp_ptr == '"')         /* Any nested quote will be escaped */
             temp_ptr++;                /* So skip one w/o adjusting length */
        }
	type = T_CHAR;
#else
        len = (i4)STlength( varname );
	type = T_CHAR;
	gcarg = GCmSTDESC;
	gc_fprefix = ERx("IIX");

#endif   /* MF_COBOL */

	
    }
    STprintf( desc_buf, ERx("%d %d %d"), attrs, type, len );
    gc_args( GCmVAL, desc_buf);

#if defined(MF_COBOL) || defined(DEC_COBOL)
    if ((sy == (SYM *)0) && (gcarg != GCmOMIT))
    {
        if (len < GC_IISLEN)
            gc_mfstrmove(FALSE, varname, gc_nxtch(GCmREF));
        else
            gc_mfstrmove(FALSE, varname, gc_nxtlch(GCmREF));
    }
    else
#endif
    gc_args(gcarg,varname);

    for (ar = 2; ar < args; ar++)
    {
        varname = arg_name(ar);
        if (!varname)
        {
            if ((flag & G_RET) && ar != 2)
                gc_args( GCmOMIT, (char *)0 );
            else
                gc_args(GCmVAL, ERx("0"));
        }
        else if (flag == G_RET && ar != 2)
            gc_args( GCmREF, varname );
        else
            gen_data(G_SET,ar);
    }
    return 0;
}


/*
+* Procedure:	gen_io    
** Purpose:	Handle I/O variables destined for C runtime routines,
**		whether they set values or expect return values.
** Parameters:	func  - i4  - Function index.
**		flags - i4  - Flags for call.
**		args  - i4  - Number of arguments.
** Returns:	complete - i4  	0 = completed processing string
**			   	1 = string too long to fit in long buffer; 
**				    need to be called again.
**	Only for IIwritio will the return value ever be anything other than 0.
**	For all other calls, the scanner only allows input of 255 characters.
**
** Based on the flag figure out what to call in gen_iodat.  Also, if there
** is a name associated with the the value/variable dump it properly.
**
** Note:
**  1.  There will always be a minimum of two arguments: the null indicator
**	and the actual argument.  If there are just 2 arguments they we rely
**	on the following template format:
**		CALL "IIRETfunc" USING indaddr isvar type len var
**		CALL "IISETfunc" USING indaddr isvar type len var_or_val
**      If there are 3 arguments they we rely on the following template 
**      format:
**		CALL "IIRET" USING indaddr isvar type len var "object"
**		CALL "IISET" USING "object" indaddr isvar type len var_or_val
**      We figure out which one is the argument which must be described and the 
**      others we just dump.
**
**	G_RETSET is really a G_RET, but the position of its I/O argument
**	in relation to the other arguments is like the set: ie,
**	    set (name = var)  and  prompt (msg, var)
** 2.	For variables that are stored inside non-standard storage (picture,
**	numeric display, etc.) we generate an assignment before or after
**	the call (something like):
**	    MOVE num_edit_var TO IIF8
**	    CALL "IISET" USING "object" indaddr ISVAR T_FLOAT sizeof(f8) IIf8
**	or:
**	    CALL "IIRET" USING indaddr ISVAR T_FLOAT sizeof(f8) IIf8 "object"
**	    MOVE IIF8 TO num_edit_var
**	2.1: For decimal constants, we pass them as a character string like:
**	    	MOVE "1.45" TO IISDAT
**	    	CALL "IISET" USING "object" indaddr ISVAR T_PACKASCHAR 0 IIS
** 3.	For variables that are stored as a regular COMP with scale  
** 	(or too large) we generate a call to convert it to an f8 
**	(or convert an f8 to COMP). This is because a simple compiler 
**	assignment to an f8 (or back) truncates precision.
**	    CALL "IICMPTOF8" USING IIF8 CMP-SCALE-VAR BY VALUE len scale
**	    CALL "IISET" USING "object" indaddr ISVAR T_FLOAT sizeof(f8) IIF8
**	or:
**	    CALL "IIRET" USING ISVAR indaddr T_FLOAT sizeof(f8) IIF8 "object"
**	    CALL "IIF8TOCMP" USING IIF8 CMP-SCALE-VAR BY VALUE len scale.
** 4.	For MF COBOL:
**	01 IIPK PIC S9(9)V9(9) USAGE COMP-3.
**	01 IIF8 PIC X(8) USAGE COMP-X.
**	4.1: On Input of COMP/num-edit variables with scale or len > MAX:
**	    	MOVE <var> TO IIPK.
**	    	CALL "IIMFpktof8" USING IIF8 IIPK 18 9 IS-SIGN.
**	    	CALL "IISET" USING "obj" indaddr ISVAR T_FLOAT sizeof(f8) IIF8
** 	4.2: On Output of COMP/num-edit variables with scale or len > MAX:
**	    	CALL "IIRET" USING ISVAR indaddr T_FLOAT sizeof(f8) IIF8 "obj"
**	    	CALL "IIMFf8topk" USING IIF8 IIPK 18 9 IS-SIGN.
**	    	MOVE IIPK TO <var>.
**	4.3: On Input & Output of COMP with no scale and len < MAX:
**		[Input: MOVE <var> TO III4.]
**	    	CALL "II-I/O" USING ISVAR indaddr T_INT sizeof(i4) III4
**		[Output: MOVE III4 TO <var>.]
**	4.4: On Input of COMP-3 variables: (Note: s=scale and p=precision)
**	    	CALL "IISET" USING "obj" indaddr ISVAR T_PACKED (p,s) <var> 
** 	4.5: On Output of COMP-3 variables: (Note: s=scale and p=precision)
**	    	CALL "IIRET" USING ISVAR indaddr T_PACKED (p,s) <var> "obj"
**	4.6: On Input of PIC X variables:
**		Regular - CALL "IISET" USING inaddr ISVAR T_CHA len <var>
**		Notrim  - CALL "IISET" USING inaddr ISVAR T_CHAR len <var>
**	4.7: On Output of PIC X variables:
**		CALL "IIRET" USING inaddr ISVAR T_CHA len <var>
**	4.8: On input of integer literals:
**		CALL "IISET" USING ISVAL T_INT sizeof(i4) <literal>.
**	4.9: On input of decimal literals, e.g. "1.35":
**		MOVE "<decimal-literal>" to IISDAT.
**		CALL "IISET" USING ISVAR T_PACKASCHAR 0 IIS.
**	4.10:On input of float literals, e.g., "1.34E5"
**		MOVE "<float-literal>" to C-STRING.
**		CALL "IIMFstrtof8" USING C-STRING IIF8.
**		CALL "IISET" USING ISVAR T_FLT sizeof(f8) IIF8.
**	4.11:On input of string literals (see gc_mfstrmove):
**		MOVE <piece-1> TO IIVAR(subscript).
**		...
**		CALL "IISET" USING ISVAR T_CHAR <str-len> BY REFERENCE <iivar>.
**	4.12: Indicator variables are always MOVE'd from/to IIIND.
*/

i4
gen_io( func, flag, args )
i4		func;
register i4	flag;
register i4	args;
{
    register i4	ar;
    i4			datflag;
    i4			complete = 0;

    datflag = flag & ~(G_RET|G_SET|G_RETSET);

    if (flag & G_RET)
    {
	gen_null( G_RET, 1 );
	_VOID_ gen_iodat( func, G_RET, 2 );	/* I/O arg is #2 */
	for (ar = 3; ar <= args; ar++)		/* Dump rest of arguments */
	    gen_data( datflag, ar );
    }
    else	/* G_SET or G_RETSET */
    {
	for (ar = 1; ar <= args-2; ar++)	/* Dump initial arguments */
	    gen_data( datflag, ar );
	gen_null( flag, args-1 );
	complete = gen_iodat( func, G_SET, args );		/* Last arg is the I/O arg */
    }
    return (complete);
}


/*
+* Procedure:	gen_iodat 
** Purpose:	Direct the code generation for I/O variables destined
**		for C runtime routines.
** Parameters:	func  - i4  - Function index.
**		flags - i4  - Flags for call.
**		args  - i4  - Number of arguments.
** Returns:	ret_val - i4    0 = finish processing string
**			        1 = string is too long to fit in long buffer;
**				    need to call again to finish processing 
**				    string.
**	Only for IIwritio will the return value ever be anything other than 0.
**	For all other calls, the scanner only allows input of 255 characters.
**
** See gen_data comment for notes on what is MOVEd into and out of dummy
-* II variables.
**
** Note:  For DG, GCmVAL has been redefined to GCmREF.  No arguments can 
**        be passed by VALUE.
*/

i4
gen_iodat( func, flag, arg )
i4		func;
register i4	flag;
register i4	arg;
{
    register SYM	*var;
    COB_SYM		*cs;
    char		*varname;
    i4			len;
    i4			num;
    f8			flt;
    char		*sdata;
    i4                  artype;
    char		save_ch;
    i4			ret_val = 0;
    static char		*head = (char *)0;
    char		*temp_ptr, *tail;
    i4			count = 0;
    i4			i;
# if defined(MF_COBOL) || defined(DEC_COBOL)
    static COB_SYM	IIPK_sym ZERO_FILL;

    if (IIPK_sym.cs_type == 0)
    {
	/* 01 IIPK PIC S9(9)V9(9) USAGE COMP-3 */
	IIPK_sym.cs_type = (i1)COB_3;
	IIPK_sym.cs_nlen = (i1)18;
	IIPK_sym.cs_nscale = (i1)9;
    }
#endif /* MF_COBOL */

    if (var = arg_sym(arg))			/* User variable */
    {
	/* Send null constants by value */
	if (sym_g_btype(var) == T_NUL)
	{
	    gc_iofmt( FALSE, -T_NUL, 1 );
# ifdef DGC_AOS
            gc_args( GCmREF, ERx("II0") );
# else
	    gc_args(GCmVAL, ERx("0"));
# endif /* DGC_AOS */
	    return (ret_val);
	}

	varname =  arg_name( arg );
	cs = (COB_SYM *)sym_g_vlue( var );
	artype = COB_MASKTYPE(cs->cs_type);
#if !defined(MF_COBOL) && !defined(DEC_COBOL)	/* VMS or DG */
	switch (artype)
	{
	  case COB_COMP:
	  case COB_4:		/* Not VMS */
	    /* Error - Is too big so MOVE to f8 and pass By Reference */
	    if (cs->cs_nlen > COB_MAXCOMP)
	    {
		gc_iofmt( TRUE, T_FLOAT, sizeof(f8) );
		if (flag & (G_RET|G_RETSET))
		    gc_retmove( TRUE, ERx("IIF8"), varname );
		else
		    gc_move( FALSE, varname, ERx("IIF8") );
		gc_args( GCmREF, ERx("IIF8") );
	    }
	    /* Maybe scaled - so use f8 and internal run-time conversion */
	    else if (cs->cs_nscale)
	    {
		gc_iofmt( TRUE, T_FLOAT, sizeof(f8) );
# ifdef DGC_AOS
		if (flag & (G_RET|G_RETSET))
		    gc_retmove( TRUE, ERx("IIF8"), varname );
		else
		    gc_move( FALSE, varname, ERx("IIF8") );
# else
		if (flag & (G_RET|G_RETSET))
		    gc_comp( 1, ERx("IIF8TOCMP"), varname, cs );
		else
		    gc_comp( 0, ERx("IICMPTOF8"), varname, cs );
# endif /* DGC_AOS */
		gc_args( GCmREF, ERx("IIF8") );
	    }
	    else if (cs->cs_nlen == COB_MAXCOMP)
	    {
		/* Move to i4 - VMS allows MAXINT into an S9(9) USAGE COMP */
		gc_iofmt( TRUE, T_INT, sizeof(i4) );
		sdata = gc_nxti4(GCmREF);
		if (flag & (G_RET|G_RETSET))
		    gc_retmove( TRUE, sdata, varname );
		else
		    gc_move( FALSE, varname, sdata );
	    }
	    else		/* Pass as an i2/i4 By Reference */
	    {
# ifdef DGC_AOS
	        /* 
		** DG's PIC S9(1-2) are 1 byte, 
		**	      (3-4)     2
		**	      (5-6)     3
		**            (7-9)     4
		*/
		gc_iofmt(TRUE, T_INT, sym_g_dsize(var));
# else
		gc_iofmt(TRUE, T_INT, cs->cs_nlen<=4 ? sizeof(i2) : sizeof(i4));
# endif /* DGC_AOS */
		gc_args( GCmREF, varname );
	    }
	    break;

	  case COB_INDEX:		/* Is an i4 - use By Reference */
	    gc_iofmt( TRUE, T_INT, sizeof(i4) );
	    gc_args( GCmREF, varname );
	    break;

	  case COB_1:			/* Is an f4/f8 - use By Reference */
	  case COB_2:
	    gc_iofmt( TRUE, T_FLOAT, cs->cs_nlen );
	    gc_args( GCmREF, varname );
	    break;

	  case COB_3:				/* Pass a Packed Decimal var */
	    /*
	    ** Pass all COMP-3 as a packed decimal by reference (even for
	    ** zero scale decimals - this will fix the previous problem of
	    ** fitting a 10 digit decimal whose value is larger than
	    ** the maximum integer value into a i4).
	    */
	    gc_iofmt( TRUE, T_PACKED, sym_g_dsize(var));
	    gc_args( GCmREF, varname );
	    break;
	  case COB_EDIT:			/* 	a Numeric Edited */
	  case COB_NUMDISP:			/*	a Numeric Display */
	    /* Has scale or too big so MOVE to Float and pass By Reference */
	    if (cs->cs_nscale || cs->cs_nlen > COB_MAXCOMP)
	    {
		gc_iofmt( TRUE, T_FLOAT, sizeof(f8) );
		if (flag & (G_RET|G_RETSET))
		{
		    gc_retmove( TRUE, ERx("IIF8"), varname );
		}
		else
		{
		    gc_move( FALSE, varname, ERx("IIF8") );
		}
		gc_args( GCmREF, ERx("IIF8") );
	    }
	    else		/* MOVE to an i4 and pass By Reference */
	    {
		gc_iofmt( TRUE, T_INT, sizeof(i4) );
		sdata = gc_nxti4(GCmREF);
		if (flag & (G_RET|G_RETSET))
		    gc_retmove( TRUE, sdata, varname );
		else
		    gc_move( FALSE, varname, sdata );
	    }
	    break;

	  case COB_DISPLAY:
	  default:		/* Error earlier - but let it slide here */
	    gc_iofmt( TRUE, T_CHAR, cs->cs_slen );
# ifdef DGC_AOS
	    gc_args( GCmREF, varname );	  /* Always pass by reference for DG */
# else
	    gc_args( GCmDESC, varname );	/* Always pass by descriptor */
# endif /* DGC_AOS */
	    gc_fprefix = ERx("IIX");		/* IO arg by descriptor */
	    break;
	} /* end switch */
#else	/* Is MF_COBOL */
	switch (artype)
	{
	  case COB_COMP:
	  case COB_EDIT:			/* 	a Numeric Edited */
	  case COB_NUMDISP:			/*	a Numeric Display */
	    /*
	    ** Any scaled edited/COMP - Use temporary f8/pack + run-time.
	    ** COMP > MAX is really an error but still make this work for edit.
	    */
	    if (cs->cs_nscale || cs->cs_nlen > COB_MAXCOMP)
	    {
		gc_iofmt(TRUE, T_FLOAT, sizeof(f8));	/* To pass to I/O */
		if (flag & (G_RET|G_RETSET))
		{
		    gc_comp(1, ERx("IIMFf8topk"), ERx("IIPK"), &IIPK_sym);
		    gc_retmove(TRUE, ERx("IIPK"), varname);
		}
		else
		{
		    gc_move(FALSE, varname, ERx("IIPK"));
		    gc_comp(0, ERx("IIMFpktof8"), ERx("IIPK"), &IIPK_sym);
		}
		gc_args(GCmREF, ERx("IIF8"));
	    }
	    else	/* MF: All other cases are moved to an I4 */
	    {
		/* Move to i4 - VMS allows MAXINT into an S9(9) USAGE COMP */
		gc_iofmt( TRUE, T_INT, sizeof(i4) );
		sdata = gc_nxti4(GCmREF);
		if (flag & (G_RET|G_RETSET))
		    gc_retmove( TRUE, sdata, varname );
		else
		    gc_move( FALSE, varname, sdata );
	    }
	    break;

	  case COB_INDEX:		/* Is an i4 - use By Reference */
	    gc_iofmt( TRUE, T_INT, sizeof(i4) );
	    gc_args( GCmREF, varname );
	    break;

	  case COB_3:				/* Process a Packed var */
	    /*
	    ** MF Cobol code: do the same thing as VMS for packed decimals 
	    **
	    */
	    gc_iofmt( TRUE, T_PACKED, sym_g_dsize(var));
	    gc_args( GCmREF, varname );
	    break;

	  case COB_DISPLAY:
	  default:		/* Maybe error earlier - let it slide here */
	    gc_iofmt(TRUE, func == IINOTRIM ? T_CHAR : T_CHA, cs->cs_slen);
	    gc_args( GCmREF, varname );
	    break;
	}
#endif	/* If not MF_COBOL */
	return (ret_val);
    }

    /* Not a user variable, dump the argument based on its representation */

    if (flag & (G_RET|G_RETSET))		/* Error earlier - so recover */
    {
	gc_iofmt( TRUE, T_INT, sizeof(i4) );
	gc_args( GCmREF, gc_nxti4(GCmREF) );
	return (ret_val);
    }

    sdata = (char *)0;
    if ((artype = arg_rep(arg)) == ARG_CHAR)
    {
	artype = arg_type( arg );
	sdata = arg_name( arg );
    }
    switch (artype)
    {
      case ARG_INT:		/* Pass integer constant By Value */
# if defined (i64_hpu) || defined (r64_us5)
	gc_iofmt( FALSE, T_INT, sizeof(i8) );
# else
	gc_iofmt( FALSE, T_INT, sizeof(i4) );
# endif
	if (sdata == (char *)0)
	{
	    arg_num( arg, ARG_INT, &num );
	    sdata = gc_cvla( num );
	}
# ifdef DGC_AOS
	/* Pass by reference on DG */
	gc_move( FALSE, sdata, gc_nxti4(GCmREF) );
# else
	gc_args( GCmVAL, sdata );
# endif /* DGC_AOS */
	break;

      case ARG_FLOAT:		/* MOVE float to IIF8 and pass By Ref */
	gc_iofmt( TRUE, T_FLOAT, sizeof(f8) );
	if (sdata == (char *)0)
	{
	    arg_num( arg, ARG_FLOAT, &flt );
	    sdata = gc_cvfa( flt );
	}
# if defined(MF_COBOL) || defined(DEC_COBOL)
	gc_strf8(sdata);
# else
	gc_move( FALSE, sdata, ERx("IIF8") );
# endif /* MF_COBOL */
	gc_args( GCmREF, ERx("IIF8") );
	break;

      case ARG_PACK:
	gc_iofmt( TRUE, T_PACKASCHAR, 0 );
	gc_move( TRUE, sdata, gc_nxtch(GCmREF) );
	break;

      case ARG_CHAR:
      default:			/* Error earlier - but recover */
# ifdef DGC_AOS
	/* Initialize head */
	if (head == (char *)0)
	   head = sdata;

	/* 
	**  On DG, strings are not passed by descriptor.  Need to MOVE to COBOL
	**  variable.  String may be longer than the long buffer, so keep track 
	**  what have already passed by using 'head' and 'tail'.  If unable
	**  to complete processing string, then return TRUE (ret_val = 1), so 
	**  will will be called again.
	*/

	len = (i4)STlength( head );		/* get length of string */
						/* Now send string */
	if (len <= GC_IILLEN)			/* Use short buffer? */
	{
	   /* 
	   ** Only for IIwritio, will a string ever be broken up, since the
	   ** scanner doesn't allow literal string > 255.
	   */

	   /*  count pairs of quotes to adjust length */

	   temp_ptr = head;		/* initialize temp_ptr to beginning */
	   for (i = 1; i <= len; temp_ptr++, i++)
	   {
	      if (*temp_ptr == '"') 
	      {
	         count++;	/* count it to adjust length */
		 temp_ptr++;	/* point to next double quote */
		 i++;
	      }
	   }
	   len = len - count;	/* adjust length for escaped character */ 
	   
	   gc_iofmt( TRUE, T_CHAR, len );
	   if ((len + count) < GC_IISLEN)		/* Use long buffer? */
	      gc_move(TRUE, head, gc_nxtch(GCmREF));
	   else
	      gc_move(TRUE, head, gc_nxtlch(GCmREF));
	   head = (char *)0;			/* clear head for next string */
	}
	else					/* need to break up string */
	{
	   /* 
	   ** Count the number of pairs of double quotes to adjust 
	   ** the length of the string.  Also do not break up the string in 
	   ** the middle of an escaped character (double quote).
	   ** Double qoutes will ALWAYS be in pairs. 
	   */
	   temp_ptr = head;	/* initialize temp_ptr and tail to beginning */
	   tail = temp_ptr;
	   for (i = 1; i <= len; temp_ptr++, i++)
	   {
	      if (*temp_ptr == '"') 
	      {
	         count++;	/* count it to adjust length */
		 temp_ptr++;	/* point to next quote */
		 i++;
	      }
	      else
	      {
		 tail = temp_ptr;/* tail points to last non-quote char */
	      }
	   }
	   /* 
	   **  String is too long to fit in the long buffer, so will
	   **  need to break up.  Create a string that will fit by 
	   **  replacing the GC_IILLENth character w/ a null.
	   **  After sending it over, replace the null w/original
	   **  character, move pointer (head) to point to new head
	   **  of string and repeat.
	   */		
	   save_ch = *(tail);	  		  /* save character */
	   *(tail) = '\0';            		  /* replace w/ NULL */
	   len = (i4)STlength(head) - count;     /* get new length, account
						  ** for any double quotes */
	   gc_iofmt( TRUE, T_CHAR, len );         /* put out descriptors */
	   gc_move(TRUE, head, gc_nxtlch(GCmREF));/* move string */
	   *(tail) = save_ch;         		  /* restore character */
	   head = tail;               		  /* point to new string */
	   ret_val = 1;	                          /* set flag to repeat */
	}
	gc_fprefix = ERx("IIX");		/* IO arg by descriptor */
# else
#   if defined(MF_COBOL) || defined(DEC_COBOL)
	/* First figure original string length to avoid extra run-time blanks */
	for (len = 0, temp_ptr = sdata; *temp_ptr; temp_ptr++, len++)
	{
	  if (*temp_ptr == '"') 	/* Any nested quote will be escaped */
	     temp_ptr++;		/* So skip one w/o adjusting length */
	}
	gc_iofmt(TRUE, T_CHAR, len);	/* Original length ignores blanks */
	if (len < GC_IISLEN)
	    gc_mfstrmove(FALSE, sdata, gc_nxtch(GCmREF));
	else
	    gc_mfstrmove(FALSE, sdata, gc_nxtlch(GCmREF));
#   else
	len = (i4)STlength( sdata );
	gc_iofmt( TRUE, T_CHAR, len );
	gc_args( GCmSTDESC, sdata );
	gc_fprefix = ERx("IIX");		/* IO arg by descriptor */
#   endif   /* MF_COBOL */
# endif /* DGC_AOS */
	break;
    }
    return (ret_val);
}



/*
+* Procedure:	gen_IIdebug 
** Purpose:	Dump file name and line number for runtime debugging.
** Parameters:	None
** Returns:	0 - Unused.
** Generate:	CALL "IIfunc" USING IIDBG,BY VALUE line
-*		CALL "IIfunc" USING OMITTED OMITTED
*/

i4
gen_IIdebug()
{
    if (eq->eq_flags & EQ_IIDEBUG)
    {
	gc_args( GCmREF, ERx("IIDBG") );
	gc_args( GCmVAL, gc_cvla(eq->eq_line) );
    } else
    {
	gc_args( GCmOMIT, (char *)0 );
	gc_args( GCmOMIT, (char *)0 );
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
**	flag		- Flag to tell whether to save the indicator name.
**			  Save to null ind on G_RET and G_RETSET.
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
**      MF COBOL:
**	  Input:
**		MOVE <ind> TO IIIND.
**		CALL "IISET" USING IIIND .... <val>
**	  Output:
**		CALL "IIRET" USING IIIND .... <var>
**		MOVE IIIND TO <ind>.
**		
**  History:
**	18-feb-1987 - written (bjb)
**	19-jun-1987 - Modified for COBOL. (mrw)
**	21-nov-1989 - Added MF support. (ncg)
*/

void
gen_null( flag, argnum )
i4	flag;
i4	argnum;
{
    if ((arg_sym(argnum)) == (SYM *)0)		/* No indicator variable? */
    {
# ifdef DGC_AOS
	/*
	** On DG, pass II0 to distinguish between a null pointer and an 
	** indicator pointing to zero.
	*/
        gc_args( GCmREF, ERx("II0") );
# endif /* DGC_AOS */
	gc_args( GCmOMIT, (char *)0 );		/* Pass null pointer */
	gc_retnull = (char *)0;
    } else
    {
# ifdef DGC_AOS
        gc_args( GCmREF, ERx("II1") );
# endif /* DGC_AOS */
# if defined(MF_COBOL) || defined(DEC_COBOL)
	if ((flag & (G_RET|G_RETSET)) == 0)
	{
	    /* MOVE <ind> TO IIIND.  Use IIIND. */
	    gc_move(FALSE, arg_name(argnum), ERx("IIIND"));
	    gc_args(GCmREF, ERx("IIIND"));
	    gc_retnull = (char *)0;
	}
	else
	{
	    /* Use IIIND.  MOVE IIIND TO <ind>. [IF <ind> NOT = -1 ...] */
	    gc_args(GCmREF, ERx("IIIND"));
	    gc_retnull = arg_name(argnum);
	}
# else
	gc_args( GCmREF, arg_name(argnum) );
	gc_retnull = (flag & (G_RET|G_RETSET)) ? arg_name(argnum) : (char *)0;
# endif /* MF_COBOL */
    }
}
	

/*
+* Procedure:	gen_sconst 
** Purpose:	Generate a string constant.
** Parameters:	String
** Returns:	None
**
** Call gen__sconst() that will dump the string in pieces that fit on a line
** and continue each line appropriately for COBOL.  VMS allows COBOL strings
** upto 256, so we do not have to worry, because that is the maximum string
** constant accepted by the scanner. Also VMS COBOL allows upto 256 characters
** on a line so we do not have to be too careful, about where our strings
-* are going.
*/

void
gen_sconst( str )
char	*str;
{
    if (str != (char *)0)
    {
	out_emit( ERx("\"") );
	while (gen__sconst(&str))
	{
	    if (eq_ansifmt)
		out_emit( ERx("\n      -    ") );
	    else
		out_emit( ERx("\n-\t") );
	    out_emit( g_indent );
	    out_emit( ERx("  \"") );			/* Continue literal */
	}
	out_emit( ERx("\"") );
    }
}

/*
+* Procedure:	gen__sconst 
** Purpose:	Local utility for writing out the contents of strings.
** Parameters:	str - char ** - Address of the string pointer to write.
** Returns:	0   - No more data to dump,
**		1   - Call me again.
**
** The strategy for is to put out as much as we can on a line, without
** breaking the string in the middle of a compiler escape sequence (a 
-* doubled ").
** For MF COBOL - we MUST break even in the middle of a doubles escape to
** prevent the compiler treating the unused column as an embedded blank.
** This may be a bug on other implementations as well.
*/

i4
gen__sconst( str )
register char	**str;
{
    register char	*cp;
    register i4	len;
    char		savech;

    /* Allow a few extra spaces for closing quote and optional terminator */
    if ((i4)STlength(*str) <= ((len = G_LINELEN - out_cur->o_charcnt) - 2))
    {
	out_emit( *str );		    /* Fits on current line */
	return 0;
    }
    for (cp = *str; len > 0 && *cp; )
    {
#if !defined(MF_COBOL) && !defined(DEC_COBOL)
	/*
	** Try not to break in the middle of compiler escape sequences
	** For MF you MUST reach the last column in order to break regardless
	** if this is an escape sequence - otherwise it will pad with blanks.
	*/
	if (*cp == '"')
	{
	    cp++;
	    len--;			/* Skip the escaped " */
	}
#endif /* MF_COBOL */
	cp++;				/* Skip the current character */
	len--;
    }
    if (len < 0)			/* string too long to fit */
    {
	cp = cp - 2;			/* put last two items on next line */
    }
    savech = *cp;
    *cp = '\0';
    out_emit( *str );
    *cp = savech;
    *str = cp;			  	/* More to go next time round */
    return 1;			/* Do it with another call */
}


/*
+* Procedure:	gen__int 
** Purpose:	Does local integer conversion.
** Parameters:	num - i4  - Integer to put out.
-* Returns:	None
*/

void
gen__int( num )
i4	num;
{
    char	buf[16];

    CVna( num, buf );
    gen__obj( TRUE, buf );
}

/*
+* Procedure:	gen__float 
** Purpose:	Does local floating conversion.
** Parameters:	num - f8 - Floating to put out.
-* Returns:	None
*/

void
gen__float( num )
f8	num;
{
    char	buf[50];

    STprintf( buf, ERx("%f"), num, '.' ); /* Standard notation */
    gen__obj( TRUE, buf );
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
}


/*
+* Procedure:	gc_retmove 
** Purpose:	Move data back into COBOL variable if a dummy II variables
**		were used as arguments.
** Parameters:	add	- i4	- Add arguments or MOVE them,
**		iiname	- char *- The dummy II variable name,
**		cobname	- char *- COBOL variable to MOVE to.
** Returns:	None
** Example:	A variable X of type PIC S9(3)V9 will be replaced by IIF8
**		in an output call, and after the call we will move data back.
**
**		CALL "IIRETDAT" USING IIF8.
**		MOVE IIF8 TO X.
**
** In the case when a null indicator is used with output statements, AND we
** use a temporary variable (ie, for numeric edited) we must not change the
** original variable.  For example we may generate something like:
**
**		CALL "IIGETDOMIO" USING NULL-IND, III4.
**		IF NULL-IND NOT= -1 
**			MOVE III4 TO NUM-EDIT-VAR.
** For MF COBOL:
**		CALL "IIGETDOMIO" USING IIIND, III4.
**		MOVE IIIND TO NULL-IND.
**		IF NULL-IND NOT= -1 
**			MOVE III4 TO NUM-EDIT-VAR.
*/

void
gc_retmove( add, iiname, cobname )
i4	add;
char	*iiname, *cobname;
{
    static struct {
	char	*gc_r_iiname;
	char	*gc_r_cobname;
    } gc_r = {0};
    bool	old_dot_mode;

    if (add)
    {
	gc_r.gc_r_iiname = iiname;
	gc_r.gc_r_cobname = cobname;
    }
    else
    {
	if (gc_r.gc_r_iiname) 	/* II dummy variable was used  */
	{
	    if (gc_retnull)	/* If null indicator then compare against -1 */
	    {
		old_dot_mode = gc_dot;
		gc_dot = FALSE;
		gen__obj( TRUE, ERx("IF ") );
		gen__obj( TRUE, gc_retnull );
		gen__obj( TRUE, ERx(" NOT = -1") );
		gc_newline();
		gen_do_indent( 1 );
	    }
	    gc_move( FALSE, gc_r.gc_r_iiname, gc_r.gc_r_cobname );
	    if (gc_retnull)
	    {
		gc_dot = old_dot_mode;
		gen_do_indent( -1 );
		/* If running with the -u option we cannot use END-IF
		** in those cases we use period.
		*/
		if (eq->eq_config & EQ_COBUNS)   		/* -u option */
		{
	    	    /* output a period, which may be on own line */
	    	    /* unless gc_dot means we are about to output one. */
	    	    if (! gc_dot) 
			gen__obj( TRUE, ERx(".") );
		}
		else
		    gen__obj( TRUE, ERx("END-IF") );
		gc_newline();
	    }
	    gc_r.gc_r_iiname = (char *)0;
	}
    }
}

/*
+* Procedure:	gc_comp 
** Purpose:	Convert COMP-3 to/from an f8.
** Parameters:	add	- i4	    - Spit out argumnents now (or save them).
**		func	- char *    - Function to use (IIPKTOF8, etc).
**		varname	- char *    - The COMP[-3] variable (or NULL to free).
**		cs	- COB_SYM * - Information about variable.
** Returns:	None
-* Notes:	See the comment for gen_io for example.
**		See the comment for gc_retmove for indicator variable checking.
*/

void
gc_comp( add, func, varname, cs )
i4	add;
char	*func, *varname;
COB_SYM	*cs;
{
    static struct {
	char	*cmp_func;
	char	*cmp_cobname;
	COB_SYM	*cmp_cs;
    } gc_cmp = {0};
    bool	old_dot_mode;
    bool	null_ind_dump = FALSE;

    if (add)
    {
	gc_cmp.cmp_func = func;
	gc_cmp.cmp_cobname = varname;
	gc_cmp.cmp_cs = cs;
	return;
    }
    if (func == (char *)0)		/* Dump stored name from gen__II */
    {
	func = gc_cmp.cmp_func;
	varname = gc_cmp.cmp_cobname;
	cs = gc_cmp.cmp_cs;
	gc_cmp.cmp_func = (char *)0;	/* Reset for next time */
	if (func && gc_retnull)	/* If null indicator then compare against -1 */
	{
	    null_ind_dump = TRUE;
	    old_dot_mode = gc_dot;
	    gc_dot = FALSE;
	    gen__obj( TRUE, ERx("IF ") );
	    gen__obj( TRUE, gc_retnull );
	    gen__obj( TRUE, ERx(" NOT = -1") );
	    gc_newline();
	    gen_do_indent( 1 );
	}
    }
    if (func) 				/* Call now if there's a function */
    {
	gen__obj( TRUE, ERx("CALL \"") );	/* Make func name atomic */
	out_emit( func );
	out_emit( ERx("\"") );
	gen__obj( TRUE, ERx(" USING IIF8 ") );
	gen__obj( TRUE, varname );
	gen__obj( TRUE, ERx(" VALUE ") );
	gen__int( cs->cs_nlen );
	gen__obj( TRUE, ERx(" ") );
	gen__int( cs->cs_nscale );
#if defined(MF_COBOL) || defined(DEC_COBOL)
	gen__obj(TRUE, ERx(" "));		/* Add sign indicator */
	gen__int((cs->cs_type & COB_NOSIGN) ? 0 : 1);
#endif /* MF_COBOL */
	gc_newline();
	gc_cmp.cmp_func = (char *)0;
    }
    if (null_ind_dump)
    {
	gc_dot = old_dot_mode;
	gen_do_indent( -1 );
	/* If running with the -u option we cannot use END-IF
	** in those cases we use period.
	*/
	if (eq->eq_config & EQ_COBUNS)   		/* -u option */
	{
	    /* output a period, which may be on own line */
	    /* unless gc_dot means we are about to output one. */
	        if (! gc_dot) 
		    gen__obj( TRUE, ERx(".") );
	}
	else
	    gen__obj( TRUE, ERx("END-IF") );
	gc_newline();
    }
}

/*
+* Procedure:	gc_strf8 
** Purpose:	Convert numeric string to an F8 at run-time.
** Parameters:	f8str	- char *    - String to convert.
** Returns:	None
** MF Only:
**		MOVE "123.456" IIF8BUF.
**		CALL "IIMFstrtof8" USING IIF8STR IIF8.
*/

void
gc_strf8(f8str)
char	*f8str;
{
    gc_move(TRUE, f8str, ERx("IIF8BUF"));
    gen__obj(TRUE, ERx("CALL \"IIMFstrtof8\" USING IIF8STR IIF8"));
    gc_newline();
}

/*
+* Procedure:	gen__obj 
** Purpose:	Write an object within the line length using the global
**		indentation. This utility is used for everything but string 
**		constants.
** Parameters:	ind - i4  - Indentation flag and Object
-* Returns:	None
** Notes:
**		Indentation for card format is to col 12 (Area B); for
**		terminal format it col 9 (one tabspace in).
**		
**		Make sure that the object can fit with 1 extra space on
**	  	the line.  This is because we may have to add a terminating
**		period (which may be optional) and we out_emit it.  If this
**		terminating period ends up in column 73 then it will be
**		ignored by the compiler and may cause unwanted control
**		flow. (neil)
*/

void
gen__obj( ind, obj )
i4		ind;
register char	*obj;
{
    /* -1 to allow for terminating period */
    if ((i4)STlength(obj) > G_LINELEN - out_cur->o_charcnt -1)
	out_emit( ERx("\n") );
    /* May have been just reset or may have been a previous newline   */
    if (out_cur->o_charcnt == 0 && ind)	   /* Indent if newline */
    {
	if (eq_ansifmt)
	    out_emit( gc_mgB );
	else
	    out_emit( ERx("\t") );
	out_emit( g_indent );	
    }
    /* Adjust margin to B if sequence number (or portions thereof) were ouput */
    else if (eq_ansifmt && (dml->dm_lang == DML_ESQL))
    {
	if (ind)
	{
	    if (out_cur->o_charcnt == G_SEQLEN)	/* Full sequence number */
	        out_emit( gc_mgCtoB );
	    else if ((out_cur->o_charcnt > 0) && 
		     (out_cur->o_charcnt < G_SEQLEN)) /* Must of put out tab */
	        out_emit( gc_tabtoB );
	}
    }
    if (obj)
	out_emit( obj );
    g_was_label = 0;		/* Something's been dumped */
}

/*
+* Procedure:	gen_do_indent 
** Purpose:	Update indentation value.
** Parameters:	i - i4  - Update value.
-* Returns:	None
*/

void
gen_do_indent( i )
i4		i;
{
    register	i4	n_ind;		/* New indentation */
    register	i4	o_ind;		/* Old indentation */

    /*
    ** Allow global indent level to exceed limits so that popping back out is
    ** done correctly, but make sure no blanks or nulls are put in random space.
    */
    o_ind = g_indlevel;
    g_indlevel += i;
    n_ind = g_indlevel;
    if (o_ind < 0)
	o_ind = 0;
    else if (o_ind >= G_MAXINDENT)
	o_ind = G_MAXINDENT - 1;
    if (n_ind < 0)
	n_ind = 0;
    else if (n_ind >= G_MAXINDENT)
	n_ind = G_MAXINDENT - 1;
    g_indent[ o_ind * 2 ] = ' ';
    g_indent[ n_ind * 2 ] = '\0';
}

/*
+* Procedure:	gen_unsupp 
** Purpose:	Do not support undocumented functions in a new language.
**		Used for Formdata old format.
** Parameters:	None
-* Returns:	0    - Unused.
*/

i4
gen_unsupp()
{
    static i2  f = 0;

    if (!f)
    {
	f = 1;
	er_write( E_EQ0076_grNOWUNSUPP, EQ_ERROR, 1, ERx("FORMDATA") );
    }
    return 0;
}

/*
+* Procedure:	gen_host 
** Purpose:	Generate pure host code from the COBOL grammar (probably 
**		declaration stuff).
** Parameters:	flag - i4      - How to generate,
**		s    - char * - Stuff to generate.
-* Returns:	None
*/

void
gen_host( flag, s )
register i4	flag;
register char	*s;
{
    i4			f_loc;
    static	i4	was_key = 0;	/* Last object was a keyword */
    char		*t1, *t2;	/* 2 comment indicator testers */
    char		*dot;

    f_loc = flag & ~(G_H_INDENT | G_H_OUTDENT | G_H_NEWLINE);

    if (flag & G_H_OUTDENT)		/* Outdent before data */
	gen_do_indent( -1);
    switch ( f_loc )
    {
      case G_H_KEY:
	if (was_key && out_cur->o_charcnt != 0)
	    out_emit( ERx(" ") );
	gen__obj( TRUE, s );
	was_key = 1;
	break;

      case G_H_OP:
	if (s && *s == '.')	/* Attach terminating period */
	    out_emit( s );
	else
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
	t1 = t2 = s;
	if (eq_ansifmt && s && *s != '\t')
	    t2 = s + G_SEQLEN;				/* Indicator column */
	if (s && *t1 != '*' && *t1 != '\\' && *t1 != 'D'
	      && *t2 != '*' && *t2 != '\\' && *t2 != 'D')
	     
	{
	    FUNC_EXTERN char *STrindex();

	    dot = STrindex(s, ERx("."), 0);
	    /*
	    ** If the dot is just before the newline then this is a period.
	    ** In example: "abcd.\n"  Length of string is 6,
	    ** 		    ^   ^
	    **		    s   dot   dot - s  = 4,
	    ** and note that dot may be null too.
	    */
	    if (dot && (dot - s == (i4)STlength(s) - 2))
		gc_dot = TRUE;
	    else
		gc_dot = FALSE;
	}
        else
            gc_dot = FALSE;
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

/*
+* Procedure:	gen_cob01
** Purpose:	Outputs the level number of level 1 data items
** Parameters:
**	s	- char*	- string containing level number
** Return Values:
-*	None
** Notes:
**	Since the code dumper (gen__obj) by default emits code in Area
** B, we need to special case level 1 data items to start in Area A --
** particularly important for ANSI-card formatted output.
*/

void
gen_cob01( s )
char	*s;
{
    if (eq_ansifmt)
    {
	/* Pad out at least until Area A */
	if (out_cur->o_charcnt == G_SEQLEN)
	    out_emit( ERx(" ") );	/* Assume sequence no. already output */
	else if (out_cur->o_charcnt < G_SEQLEN)
	    out_emit( gc_mgA );		/* Adjust output line to margin A */
	out_emit( s );
	out_emit( ERx("  ") );
    }
    else
    {
	gen__obj( TRUE, s );
	out_emit( ERx(" ") );
    }
}

/*
+* Procedure:	gc_nxti4
** Purpose:	Construct name of next available III4 variable available to 
**		MOVE something into. Chosen via round-robin.
** Parameters: 	mech - i4  - Mechanism to add the argument with.
** Returns:	char *    - Static string containing the name (never NULL).
** Side Effects:
**		Adds the return argument via gc_args with the given mechanism.
**
-* Notes:	Relies on the fact that there are at most 4 III4's needed.
*/

char *
gc_nxti4( mech )
i4	mech;
{
    static	i4	i_cnt = 0;
    static	char	*i_arr[] = 		/* Table for fast lookup */
	    {
#if defined(MF_COBOL) || defined(DEC_COBOL)
		ERx("III4-1"), ERx("III4-2"), ERx("III4-3"), ERx("III4-4")
#else
		ERx("III4(1)"), ERx("III4(2)"), ERx("III4(3)"), ERx("III4(4)")
#endif /* MF_COBOL */
	    };
    char		*i_ret;			/* Return value */

    i_ret = i_arr[i_cnt];
    gc_args( mech, i_ret );
    i_cnt = (i_cnt + 1) % III4max;
    return i_ret;
}


/*
+* Procedure:	gc_nxtch
** Purpose:	Construct name of next available IIS variable available to 
**		MOVE something into. Chosen via round-robin.
** Parameters:	mech - i4  - Mechanism to add the argument with.
** Returns:	char *    - Static string containing the name (never NULL).
** Side Effects:
**		Adds the return argument via gc_args with the given mechanism.
**
** Notes:	Relies on the fact that there are at most 14 IIS's needed in
**		one call.  They are in the following format.
**
**		02 IIS1.
**			03 IISDAT1 PIC X(33).
-*			03 FILLER PIC X VALUE LOW-VALUE.
*/

char * 
gc_nxtch( mech ) 
i4	mech; 
{
    static i4	s_cnt = 0;
    static struct  {			/* Table for fast lookup */
	char	*s_dat;			/* Data area */
	char	*s_s;			/* String pointer */
    } s_arr[] = {
	{ERx("IISDAT1"),  ERx("IIS1")},  {ERx("IISDAT2"), ERx("IIS2")},
	{ERx("IISDAT3"),  ERx("IIS3")},  {ERx("IISDAT4"), ERx("IIS4")},
	{ERx("IISDAT5"),  ERx("IIS5")},  {ERx("IISDAT6"), ERx("IIS6")},
	{ERx("IISDAT7"),  ERx("IIS7")},  {ERx("IISDAT8"), ERx("IIS8")},
	{ERx("IISDAT9"),  ERx("IIS9")},  {ERx("IISDAT10"), ERx("IIS10")},
	{ERx("IISDAT11"), ERx("IIS11")}, {ERx("IISDAT12"), ERx("IIS12")},
	{ERx("IISDAT13"), ERx("IIS13")}, {ERx("IISDAT14"), ERx("IIS14")}
    };
    char		*s_ret;			/* Return value */

    s_ret = s_arr[s_cnt].s_dat;			/* For MOVE */
    gc_args( mech, s_arr[s_cnt].s_s );		/* For CALL */
    s_cnt = (s_cnt + 1) % IICHmax;
    return s_ret;
}


/*
+* Procedure:	gc_nxtlch
** Purpose:	Construct name of next available IIL variable (like gc_nxtch).
** Parameters:	mech - i4  - Mechanism to add the argument with.
** Returns:	char *    - Static string containing the name (never NULL).
** Side Effects:
**		Adds the return argument via gc_args with the given mechanism.
-* Notes:	Uses same logic as gc_nxtch, with same formatted COBOL buffers.
*/

char * 
gc_nxtlch( mech ) 
i4	mech; 
{
    static i4	l_cnt = 0;
    static struct  {			/* Table for fast lookup */
		char	*s_dat;			/* Data area */
		char	*s_s;			/* String pointer */
	    } l_arr[] = {
		{ERx("IILDAT1"),  ERx("IIL1")},  {ERx("IILDAT2"), ERx("IIL2")}
	    };
    char		*l_ret;			/* Return value */

    l_ret = l_arr[l_cnt].s_dat;			/* For MOVE */
    gc_args( mech, l_arr[l_cnt].s_s );		/* For CALL */
    l_cnt = 1 - l_cnt;
    return l_ret;
}


/*
+* Procedure:	gc_args
** Purpose:	Add an argument to the list to be generated following
**		the CALL statement.  Dump the stored arguments.
** Parameters:	mech - i4      - Mechanism to save with or dump,
**				GCmREF	  - By Reference,
**				GCmVAL	  - By Value,
**				GCmDESC	  - By Descriptor,
**				GCmSTDESC - By Descriptor and String constant,
**					    BY CONTENT for MF.
**				GCmOMIT	  - Omitted,
**				GCmFREE	  - Free up local args,
**				GCmDUMP	  - Dump the arguments.
**		arg  - char * - Argument name to add.
-* Returns:	None
*/

void
gc_args( mech, arg )
i4	mech;
char	*arg;
{
    static i4  		a_cnt = 0;
    static struct	cobargs
		{
		    i2	a_mech;
		    char	*a_name;
		} a_arr[GC_MAXARG];
    static char	*cob_mechs[] = {
		ERx("REFERENCE"),
		ERx("VALUE"),
		ERx("DESCRIPTOR"),
#if !defined(MF_COBOL) && !defined(DEC_COBOL)
		ERx("DESCRIPTOR"), 
#else
		ERx("CONTENT"), 		/* GCmSTDESC */
#endif
		ERx("OMITTED")
    };
    register struct 	cobargs	*ca;
    register i4	prevmech;
    register i4	space;

    if (mech == GCmFREE)	/* In case of errors outside of our control */
	a_cnt = 0;
    else if (mech != GCmDUMP)	/* Add an argument */
    {
	if (a_cnt < GC_MAXARG)
	{
# ifdef DGC_AOS
    	    if (mech == GCmOMIT)	
	    {
	     	a_arr[a_cnt].a_mech = GCmREF;
	    	a_arr[a_cnt++].a_name = ERx("II0");
  		return;
	    }
# endif /* DGC_AOS */
# if defined(MF_COBOL) || defined(DEC_COBOL)
    	    if (mech == GCmOMIT)	
	    {
	     	a_arr[a_cnt].a_mech = GCmVAL;
	    	a_arr[a_cnt++].a_name = ERx("0");
  		return;
	    }
# endif /* MF_COBOL */
	    a_arr[a_cnt].a_mech = mech;
	    a_arr[a_cnt++].a_name = arg;
	}
	return;
    }
    else			/* Dump the arguments in a CALL USING */
    {
	if (a_cnt > 0)
	{
	    gen__obj( TRUE, ERx(" USING ") );
	    space = 1;
	}
	prevmech = GCmFREE;
	for(ca = &a_arr[0]; a_cnt > 0; ca++, a_cnt--)
	{
	    if (ca->a_mech != prevmech || prevmech == GCmOMIT)
	    {
		prevmech = ca->a_mech;
		/* We do not need by REFERENCE if it is the first */
		if (ca != &a_arr[0] || ca->a_mech != GCmREF)
		{
		    if (!space)
			gen__obj( TRUE, ERx(" ") );
		    gen__obj( TRUE, cob_mechs[ca->a_mech] );
		    space = 0;
		}
	    }
	    if (ca->a_mech != GCmOMIT)
	    {
		if (!space)
		    gen__obj( TRUE, ERx(" ") );
		if (ca->a_mech == GCmSTDESC)
		    gen_sconst( ca->a_name );
		else
		    gen__obj( TRUE, ca->a_name );
		space = 0;
#ifdef DGC_AOS
	    }
	    else
	    {
	        gen__obj(TRUE, ERx("II0") );
# endif /* DGC_AOS */
	    }
	}
    }
}

/*
+* Procedure:	gc_cvla
** Purpose:	Return pointer to static (round-robin) buffer after CVna.
** Parameters:	i - i4  - Integer to convert.
** Returns:	char * - Static buffer with converted integer.
-* Notes:	Relies on the fact that at most 4 such integer constants are 
**		needed.
*/

char *
gc_cvla( i )
i4	i;
{
    static	i4	i_cnt = 0;
    static	char	i_arr[III4max][12] = {0};
    register 	char	*i_ret;			/* Return value */

    i_ret = i_arr[i_cnt];
    CVna( i, i_ret );
    i_cnt = (i_cnt + 1) % III4max;
    return i_ret;
}

/*
+* Procedure:	gc_cvfa
** Purpose:	Return pointer to static buffer after CVfa.
** Parameters:	f - f8 - Double to convert.
** Returns:	char * - Static buffer with converted double.
-* Notes:	Relies on fact that only 1 such float constant is needed.
*/

char *
gc_cvfa( f )
f8	f;
{
    static	char	f_buf[50] = {0};

    STprintf( f_buf, ERx("%f"), f, '.' );
    return f_buf;
}

/*
+* Procedure:	gc_move
** Purpose:	Generate a MOVE statement.
** Parameters:	isstr - i4      - Is this a string constanst,
**		src   - char * - Source data,
**		dest  - char * - Destination variable.
** Returns:	None
-* Generates:	MOVE src TO DEST
*/

void
gc_move( isstr, src, dest )
i4	isstr;
char	*src, *dest;
{
    gen__obj( TRUE, ERx("MOVE ") );
    if (isstr)
	gen_sconst( src );
    else
	gen__obj( TRUE, src );
    gen__obj( TRUE, ERx(" TO ") );
    gen__obj( TRUE, dest );
    gc_newline();
}

/*
+* Procedure:	gc_iofmt
** Purpose:	Construct I/O descriptor and pass it as an argument.
** Parameters: 	isvar, type, len - i4  - I/O data description.
** Returns:	None
** Side Effects:
**		Adds static buffer as an argument via gc_args with a BY
-*		VALUE mechanism.
*/

void
gc_iofmt( isvar, type, len )
i4	isvar, type, len;
{
    static char desc_buf[50];
    char	*varname;

# ifdef DGC_AOS
	/* 
	**  For DG, arguments always go by reference. However ISVAR is used
	**  to indicate to the runtime layer if the string is a variable (II1)
	**  or a literal (II0).
	*/
	if (isvar)
           gc_args( GCmREF, ERx("II1") );	
	else
           gc_args( GCmREF, ERx("II0") );	
	varname = gc_cvla( type );
	gc_move( FALSE, varname, ERx("IITYPE") );
        gc_args( GCmREF, ERx("IITYPE") );
	varname = gc_cvla( len );
	gc_move( FALSE, varname, ERx("IILEN") );
        gc_args( GCmREF, ERx("IILEN") );
# else
    STprintf( desc_buf, ERx("%d %d %d"), isvar, type, len );
    gc_args( GCmVAL, desc_buf);
# endif /* DGC_AOS */
}


/*
+* Procedure:	gc_sdcall
** Purpose:	Generate a call to VMS run-time string descriptor processors.
** Parameters: 	trim  - i4	- Should we use IIsd_reg or IIsd_notrim,
** 	 	isvar - i4	- String variable or constant (ALWAYS TRUE).
**		s     - char *  - Data to send.
** Returns:	None
** Side Effects:
**		Make the runtime call putting the resultant address in IIRES
**		and adding that as an argument.
-* Example:	CALL "IISD" USING DESCRIPTOR strvar GIVING IIRES
**	####   Note: not changed for DG, because DG never calls gc_sdcall.  ##
** 	See gc_mfsdcall for MF.
*/

void
gc_sdcall( trim, isvar, s )
i4	trim, isvar;
char	*s;
{
    static char	*sd_call[2] = { ERx("IISD_NO"), ERx("IISD") };

    gen__obj( TRUE, ERx("CALL \"") );		/* Make func name atomic */
    out_emit( sd_call[trim] );
    out_emit( ERx("\"") );
    gen__obj( TRUE, ERx(" USING DESCRIPTOR ") );
    if (isvar)
	gen__obj( TRUE, s );
    else
	gen_sconst( s );
    gen__obj( TRUE, ERx(" GIVING IIRES") );
    gc_args( GCmVAL, ERx("IIRES") );
    gc_newline();
}

/*
+* Procedure:	gc_mfsdcall
** Purpose:	Generate a call to MF run-time cob-to-c string processors.
** Parameters: 	len  - i4	- Length of variable
**		var  - char *   - Var name to send.
** Returns:	None
** Side Effects:
**		Make the runtime call putting the resultant address in IIADDR
**		and adding that as an argument.
** Example:
**		CALL "IIMFsd" USING VALUE <len> REFERENCE <var> IIADDR.
**		CALL "IIxyz" BY VALUE IIADDR.
*/

void
gc_mfsdcall(len, var)
i4	len;
char	*var;
{
    gen__obj(TRUE, ERx("CALL \"IIMFsd\""));
    gen__obj(TRUE, ERx(" USING VALUE ") );
    gen__int(len);
    gen__obj(TRUE, ERx(" REFERENCE ") );
    gen__obj(TRUE, var);
    gen__obj(TRUE, ERx(" IIADDR") );
    gc_newline();
    gc_args(GCmVAL, ERx("IIADDR"));		/* For CALL */
}

/*
+* Procedure:	gc_mfstrmove
** Purpose:	Generate a sub-string MOVE statement to avoid max MF COBOL 
**	        string length.  This length ALSO applies to concatenated
**		literals (operands) used with the STRING statement.
** Parameters: 	term - i4	- Null terminate the string.
**		str  - char *	- String to generate.
**		var  - char *   - Var name to send.
** Returns:	None
** Example:
**		MOVE <first-part> TO iivar(1:part-length).
**		MOVE <next-part> TO iivar(part+1:rest-length).
**		[MOVE LOW-VALUE TO iivar(last:1).]
*/

void
gc_mfstrmove(term, str, var)
i4	term;
char	*str;
char	*var;
{
    char	*head;
    char	*cp;
    char	savech;
    i4		len;
    i4		base;
    char	varref[100];

    /* For each piece generate a MOVE statement */
    for (head = str, base = 1; *head; base += len)
    {
	gen__obj(TRUE, ERx("MOVE "));
	for (len = 0, cp = head; len < GC_MFSLEN && *cp; cp++, len++)
	{
	    /* Skip over escaped quotes for length */
	    if (*cp == '"')
		cp++;
	}
	savech = *cp;
	*cp = EOS;
	gen_sconst(head);
	*cp = savech;
	head = cp;
	gen__obj(TRUE, ERx(" TO "));
	STprintf(varref, ERx("%s(%d:%d)"), var, base, len);
	gen__obj(TRUE, varref);
	gc_newline();
    }
    if (term)				/* Null terminate - probably gen_data */
    {
	gen__obj(TRUE, ERx("MOVE LOW-VALUE TO "));
	STprintf(varref, ERx("%s(%d:1)"), var, base);
	gen__obj(TRUE, varref);
	gc_newline();
    }
}
/*
+* Procedure:	gc_newline
** Purpose:	Print the newline character.
** Parameters: 	None
-* Returns:	None
**
** If we are in "dot" mode then print ".\n", otherwise just "\n".  Note
** that we may turn off "dot" mode (ie, entering an IF block), or the user
** may (ie, we checked some host code that didn't terminate with a 
** period - see gen_host).
*/

void
gc_newline()
{
    if (gc_dot)
	out_emit( ERx(".") );
    out_emit( ERx("\n") );
}
/*
+* Procedure:   gen_Cseqnum
** Purpose:     Emit COBOL sequence number at left margin (only for ANSI format)
** Parameters:  seqnum - *char - number string
-* Returns:	None
** Notes:
**      Emit sequence number.
**
*/
gen_Cseqnum( seqnum )
char    *seqnum;
{
    if (seqnum && *seqnum != '\0')
    {
	if (out_cur->o_charcnt != 0)
	    out_emit( ERx("\n") );
        out_emit( seqnum );
    }
    else
    {
	out_emit(gc_mgC);
    }
    *seqnum = '\0';		/* reset sequence number buffer */  
}
