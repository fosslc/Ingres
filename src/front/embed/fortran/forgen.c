/* 
** Copyright (c) 2005, 2008 Ingres Corporation  
*/ 
/*
** 20-jul-90 (sandyd) 
**	Workaround VAXC optimizer bug, first encountered in VAXC V2.3-024.
NO_OPTIM = vax_vms dg8_us5
*/

# include 	<compat.h>
# include	<er.h>
# include	<si.h>
# include	<st.h>
# include	<cm.h>
# include	<lo.h>
# include	<nm.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<eqrun.h>
# include	<equel.h>
# include	<eqsym.h>
# include	<eqscan.h>
# include	<eqgen.h>
# include	<ereq.h>
# include	<ere1.h>
# include	<eqf.h>
# include	<eqfgen.h>

# ifdef DGC_AOS
#    define DG_OR_VMS
#    define DG_OR_UNIX
# endif /* DGC_AOS */

/* 
** Allow testing of VMS preprocessor to be done on Unix.
*/
# ifdef VMS_TEST_ON_UNIX
#     undef UNIX
#     define VMS
# endif /* VMS_TEST_ON_UNIX */


# ifdef UNIX
#    define DG_OR_UNIX
# endif /* UNIX */

# ifdef VMS
#    define DG_OR_VMS
# endif /* VMS */


/*
** FORGEN - Maintains all the routines to generate II calls within specific
**	    language dependent control flow statements.
**
** Language:	Written for VMS FORTRAN/UNIX F77
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
**		gen_io()			- Set up I/O arguments.
**		gen_iodat()			- Generate I/O calls
**		gen_funcname()			- Emit name in call statements.
**		gen_sdesc()			- Put out IIsd_ calls.
**		gen_pstruct()			- Put out IIps_ call.
**		gen_data()			- Dump different arg data.
**		gen_IIdebug()			- Runtime debug information.
**		gen_null()			- Dump indicator arg.
**		gen__sconst( str )		- Controls splitting strings.
**		gen__int( how, i )		- Integer.
**		gen__float( how, i )		- Float.
**		gen__slen( var, data )		- Generate string length
**		gen__sadr( var, data )		- Generate call to address func
**		gen__obj( ind, obj )		- Write anything but strings.
**		gen__functab( func )		- Get function table pointer.
**              gen_hdl_args(flags,numargs)     - SET_SQL handler args.
**              gen_datio(func)                 - PUT/GET DATA stmts.
**
** Externals Used:
**		out_cur				- Current output state.
**		eq				- Global Equel state.
**		
** Note:
** 1. Routines that write data:
** 1.1. Some routines that actually write stuff make use of the external counter
**      out_cur->o_charcnt.  This is the character counter of the current line
**      being written to.  Whenever there is a call to emit code, this counter
**      gets updated even though the local code does not explicitly update it.
** 1.2. A call to gen__obj() which makes sure the object can fit on the line, 
**      needs to be made only when the data added may have gone over the end of 
**      the line.  In some cases, either when nothing is really being written,
**      or the data comes directly from the host language host code, the main
**      emit routine can be used.
**
** History:
**		30-oct-1984 	- Written for C (ncg)
**		17-apr-1985	- Modified to work for PL/1 (ncg).
**		04-jun-1985	- Modified to work for VMS FORTRAN (mrw).
**		01-jun-1986	- Modified to work for F77 FORTRAN (bjb).
**		11-jun-1987	- Modified for 6.0 (bjb)
**		19-may-1988	- Added IIFRgp entries to gen__ftab (marge).
**		23-mar-1989	- Added cast to STlength. (teresa)
**		31-may-1989	- Modified for DG. (sylviap)
**              11-may-1989     - Added function calls IITBcaClmAct,
**                                IIFRafActFld, IIFRreResEntry to gen__ftab
**                                for entry activation. (teresa)
**		10-aug-1989	- Add function call IITBceColEnd.
**				  (teresal)
**		18-dec-1989	- Added calls for Phoenix/Alerters (bjb)
**		24-jul-1990	- Added decimal support. (teresal)
**              08-aug-1990 (barbara)
**                      Added IIFRsaSetAttrio to function table for per value
**                      display attributes.
**		05-sep-1990 (kathryn)
**		    	Integration of Porting changes from 6202p code line:
**                  	27-feb-90 (russ)
**			    On MIXEDCASE_FORTRAN compilers generate IIExit and
**                          IIXact instead of IIexit and IIxact, respectively.
**                          Otherwise there will be a name clash between the
**                          runtime version of the routine and the C version.
**                      06-jul-90 (fredb)
**                          Added cases for HP3000 MPE/XL (hp9_mpe).
**              22-feb-1991 (kathryn)
**                      Added IILQssSetSqlio and IILQisInqSqlio.
**			      IILQshSetHandler.
**		21-apr-1991 (teresal)
**			Add activate event call - remove IILQegEvGetio.
**		22-apr-1992 (Kevinm)
**			Updated NO_OPTIM for dg8_us5.
**      	12-oct-1992 (essi)
**			Fixed bug 43760.	
**		14-oct-1992 (lan)
**			Added IILQled_LoEndData.
**		08-dec-1992 (lan)
**			Added IILQldh_LoDataHandler.
**  		01-dec-1992 (kathryn)
**			Added gen_datio() function, and added entries for
**			IILQlgd_LoGetData, and IILQlpd_LoPutData.
**		16-feb-1992 (essi)
**			Fixed bug 49607.
**	26-jul-1993 (lan)
**		Added EXEC 4GL support routines.
**	07/28/93 (dkh) - Added support for the PURGETABLE and RESUME
**			 NEXTFIELD/PREVIOUSFIELD statements.
**	26-Aug-1993 (fredv)
**		Included <st.h>.
**	01-oct-1993 (kathryn)
**          First argument generated for GET/PUT DATA stmts ("attrs" mask)
**          will now be set/stored by eqgen_tl() rather than determined by
**          gen_datio, so add one to the number of args in the gen__ftab for
**          IILQlpd_LoPutData and IILQlgd_LoGetData.
**	17-dec-93 (donc) Bug 56875
**	    Added functions to support dereferencing a structure's pointer
**	    prior to libq or libqsys calls. Added gen_pstruct in order to
**	    call libqsys function IIps. Modified gen_data to call gen_pstruct
**	    when data is of type ARG_PSTRUCT.
**	27-jan-94 (donc) Bug 58859
**	    With ARG_PSTRUCT's VMS should pass the sqlda not
**	    the %val of it. 
**      03-feb-94 (huffman) 
**          ifdef unix do gen_obj, no 'continue;' statement to be encountered
**          by vms.
**	6-may-97 (inkdo01)
**	    Added table entry for IILQprvProcGTTParm.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	17-apr-2001 (mcgem01)
**	    Implement Embedded Fortran Support on Windows.
**	28-apr-2001 (mcgem01)
**	    Use IIaddfrm and not IIxaddfrm on Windows.
**	2-may-01 (inkdo01)
**	    Added parm to ProcStatus for row procs.
**	10-Sep-2003 (bonro01)
**	    Modified Fortran syntax for i64_hpu.  HP Itanium is 64-bit,
**	    so pointer parameters need to be specified as 8 bytes.
**	    Instead of passing 0 for null pointers, we need to pass 0_8
**	    to force an 8 byte null pointer.
**   11-Sep-2003 (fanra01)
**      Bug 110907
**      Bug 110959
**      Bug 110960
**      Windows changes to handle string lengths and conversions to/from
**      user strings and C strings.
**	01-Nov-2005 (hanje04)
**	    BUGS 113212, 114839 & 115285.
**	    Modify i64_hpu change to force passing of 8 byte integer 0
**	    as NULL pointer so that it works on Linux too. g77 doesn't
**	    like 0_8 so use int8(0) instead.
**	20-jul-2007 (toumi01)
**	    add IIcsRetScroll and IIcsOpenScroll for scrollable cursors
**	23-Jun-2008 (hweho01)
**	    Avoid SEGV on 64-bit applications, replace 0 with 0_8 to get
**	    8-byte pointer in function argument generations. The problem
**	    was seen in QA embed/fortran sep tests.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
*/


/* 
** Max size of converted string - allow some extra space over scanner for
** extra escaped characters required.
*/

# define 	G_MAXSCONST	300	

/* 
** Indentation information - 
**     VMS/F77 FORTRAN format can be (sort of) free form, so we use the C
**	'pretty-print' algorithm.
**
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
		  '\0', ' ',' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '  
		};

static	i4	g_indlevel = 0;		/* Start out 0 spaces from tab */

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
    ERx(".ge."),	/* C_GTREQ */
    ERx(".le."),	/* C_LESSEQ */
    ERx(".ne."),   	/* C_NOTEQ */
    (char *)0,		/* C_0 */
    ERx(".eq."),	/* C_EQ */
    ERx(".gt."),	/* C_GTR */
    ERx(".lt.")		/* C_LESS */
};

/* 
** Table of label names - correspond to L_xxx constants.
** Must be unique in the first few characters, to allow label uniqueness
** in 8 characters.
*/

# define	L_INTERVAL	200

static struct gen_ltable {
    i4		gl_num;
    char	*gl_name;
} gen_ltable[] = {
    { 0,	ERx("L_0") },	/* L_0 - Should never be referenced */
    { 7000,	ERx("L_FD_INIT") },	/* L_FDINIT */
    { 7200,	ERx("L_FDBEG") },	/* L_FDBEG */
    { 7400,	ERx("L_FDFINAL") },	/* L_FDFINAL */
    { 7600,	ERx("L_FDEND") },	/* L_FDEND */
    { 7800,	ERx("L_MUINIT") },	/* L_MUINIT */
    { 8000,	ERx("L_ACTELSE") },	/* L_ACTELSE */
    { 8200,	ERx("L_ACTEND") },	/* L_ACTEND */
    { 8400,	ERx("L_BEGREP") },	/* L_BEGREP */
    { 8600,	ERx("L_ENDREP") },	/* L_ENDREP */
    { 8800,	ERx("L_RETBEG") },	/* L_RETBEG */
    { 9000,	ERx("L_RETFLUSH") },	/* L_RETFLUSH */
    { 9200,	ERx("L_RETEND") },	/* L_RETEND */
    { 9400,	ERx("L_TBBEG") },	/* L_TBBEG */
    { 9600,	ERx("L_TBEND") }	/* L_TBEND */
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
i4	gen_io();
i4	gen_unsupp();
void	gen_iodat();
void	gen_funcname();
void	gen_sdesc();
void	gen_pstruct();
void	gen_data();
i4	gen__sconst();
i4	gen_IIdebug();
void	gen_null();
void	gen__int();
void	gen__float();
void	gen__slen();
void	gen__sadr();
void	gen__name();
void	gen__obj();
i4	gen_hdl_args();
i4      gen_datio();

/*
** Table of II calls - 
**   - We use similar logic in FORTRAN as in C.
**   - Correspond to II constants, the function to call to generate arguments, 
**       the flags needed and the number of arguments.
**   - Special table entries for F77:  Many F77 compilers complain about
**       names > 6 characters.  We therefore truncate function names
**	 when generating F77 code (see gen_funcname).  However, some names in 
**	 this table are not unique to six characters, and in these cases we 
**	 have "if defed" function names into this table for F77.  Mapping to 
**	 the real runtime function always takes place in a runtime layer for 
**	 F77 so it's quite safe to change these names.  However, if you need
**	 to add a runtime function, make sure its name is unique to six
**	 characters.
*/

# ifdef VMS
#     define PARAM_ARGS	3	/* Has extra translation function */
# else
#     define PARAM_ARGS	2
# endif /* VMS */

GEN_IIFUNC	*gen__functab();

GLOBALDEF GEN_IIFUNC	gen__ftab[] = {
    /* Name */		/* Function */	/* Flags */	/* Args */
    ERx("IInofunc"),	0,		0,		0, 
    /* Quel # 1 */
    ERx("IIflush"),	gen_IIdebug,	0,		0,
    ERx("IInexec"),	0,		0,		0,
    ERx("IInextget"),	0,		0,		0,
    ERx("IInotrimio"),	gen_io,		G_SET,		2,
# ifdef NT_GENERIC
    /*
    ** Bug 110960
    ** Windows specific layer to handle string conversion
    */
    ERx("IIxparret"),	gen_all_args,	G_NAME,		PARAM_ARGS,
    ERx("IIxparset"),	gen_all_args,	G_NAME,		PARAM_ARGS,
# else  /* NT_GENERIC */
    ERx("IIparret"),	gen_all_args,	G_NAME,		PARAM_ARGS,
    ERx("IIparset"),	gen_all_args,	G_NAME,		PARAM_ARGS,
# endif /* NT_GENERIC */
    ERx("IIgetdomio"),	gen_io,		G_RET,		2,
    ERx("IIretinit"),	gen_IIdebug,	0,		0,
    ERx("IIputdomio"),	gen_io,		G_SET, 		2,
    ERx("IIsexec"),	0,		0,		0,
    ERx("IIsyncup"),	gen_IIdebug,	0,		0,
    ERx("IItupget"),	0,		0,		0,
    ERx("IIwritio"),	gen_io,		G_SET,		3,
# ifdef MIXEDCASE_FORTRAN
    ERx("IIXact"),      gen_all_args,   0,              1,
# else
    ERx("IIxact"),      gen_all_args,   0,              1,
# endif
    ERx("IIvarstrio"),	gen_io,		G_SET,		2,
    ERx("IILQssSetSqlio"),gen_io,       G_SET,          3,
    ERx("IILQisInqSqlio"),gen_io,       G_RET,          3,
    ERx("IILQshSetHandler"),gen_hdl_args,0,             2,
    (char *)0,          0,              0,              0,
    (char *)0,		0,		0,		0,

    /* Libq # 21 */
    ERx("IIbreak"),	0,		0,		0,
    ERx("IIeqiqio"),	gen_io,		G_RET,		3,
    ERx("IIeqstio"),	gen_io,		G_SET,		3,
    ERx("IIerrtest"),	0,		0,		0,
# ifdef MIXEDCASE_FORTRAN
    ERx("IIExit"),      0,              0,              0,
# else
    ERx("IIexit"),      0,              0,              0,
# endif
    ERx("IIingopen"),	gen_var_args,	0,		15,
    ERx("IIutsys"),	gen_all_args,	0,		3,
    ERx("IIexExec"),	gen_all_args,	0,		4,
    ERx("IIexDefine"),	gen_all_args,	0,		4,
    (char *)0,		0,		0,		0,

    /* Forms # 31 */
    ERx("IITBcaClmAct"),gen_all_args,	0,		4,
    ERx("IIactcomm"),	gen_all_args,	0,		2,
    ERx("IIFRafActFld"),gen_all_args,	0,		3,
    ERx("IInmuact"),	gen_all_args,	0,		5,
    ERx("IIactscrl"),	gen_all_args,	0,		3,
# if defined(DG_OR_UNIX) || defined(hp9_mpe)
    ERx("IIaddfrm"),	gen_all_args,	G_NAME,		1,
# else
    ERx("IIxaddfrm"),	gen_all_args,	G_NAME,		1,
# endif /* DG_OR_UNIX */
    ERx("IIchkfrm"),	0,		0,		0,
    ERx("IIclrflds"),	0,		0,		0,
    ERx("IIclrscr"),	0,		0,		0,
    ERx("IIdispfrm"),	gen_all_args,	0,		2,
    ERx("IIprmptio"),	gen_io,		G_RETSET,	4, /* RET,but SET fmt */
# if defined(UNIX) || defined(hp9_mpe)
    ERx("IIenforms"),	0,		0,		0,
# else
    ERx("IIendforms"),	0,		0,		0,
# endif /* UNIX */
    ERx("IIendfrm"),	0,		0,		0,
    ERx("IIendmu"),	0,		0,		0,
    ERx("IIfmdatio"),	gen_io,		G_RET,		3,
    ERx("IIfldclear"),	gen_all_args,	0,		1,
    ERx("IIfnames"),	0,		0,		0,
# if defined(UNIX) || defined(hp9_mpe)
    ERx("IIfominit"),	gen_all_args,	0,		1,
# else
    ERx("IIforminit"),	gen_all_args,	0,		1,
# endif /* UNIX */
    ERx("IIforms"),	gen_all_args,	0,		1,
    ERx("IIfsinqio"),	gen_io,		G_RET,		3,
    ERx("IIfssetio"),	gen_io,		G_SET,		3,
    ERx("IIfsetio"),	gen_all_args,	0,		1,
    ERx("IIgetoper"),	gen_all_args,	0,		1,
    ERx("IIgtqryio"),	gen_io,		G_RET, 		3,
    ERx("IIhelpfile"),	gen_all_args,	0,		2,
    ERx("IIinitmu"),	0,		0,		0,
    ERx("IIinqset"),	gen_var_args,	0,		5,
    ERx("IImessage"),	gen_all_args,	0,		1,
    ERx("IImuonly"),	0,		0,		0,
    ERx("IIputoper"),	gen_all_args,	0,		1,
    ERx("IIredisp"),	0,		0,		0,
    ERx("IIrescol"),	gen_all_args,	0,		2,
    ERx("IIresfld"),	gen_all_args,	0,		1,
    ERx("IIresnext"),	0,		0,		0,
    ERx("IIgetfldio"),	gen_io,		G_RET,		3,
    ERx("IIretval"),	0,		0,		0,
# if defined(UNIX) || defined(hp9_mpe)
    ERx("IIrfparam"),	gen_all_args,	G_NAME,		PARAM_ARGS,
# else
# ifdef NT_GENERIC
    /*
    ** Windows specific layer to handle string conversion
    */
    ERx("IIxrf_param"),	gen_all_args,	G_NAME,		PARAM_ARGS,
# else  /* NT_GENERIC */
    ERx("IIrf_param"),	gen_all_args,	G_NAME,		PARAM_ARGS,
# endif /* NT_GENERIC */
# endif /* UNIX */
    ERx("IIrunform"),	0,		0,		0,
    ERx("IIrunmu"),	gen_all_args,	0,		1,
    ERx("IIputfldio"),	gen_io,		G_SET,		3,
# if defined(UNIX) || defined(hp9_mpe)
    ERx("IIsfparam"),	gen_all_args,	G_NAME,		PARAM_ARGS,
# else
# ifdef NT_GENERIC
    /*
    ** Windows specific layer to handle string conversion
    */
    ERx("IIxsf_param"),	gen_all_args,	G_NAME,		PARAM_ARGS,
# else  /* NT_GENERIC */
    ERx("IIsf_param"),	gen_all_args,	G_NAME,		PARAM_ARGS,
# endif /* NT_GENERIC */
# endif /* UNIX */
    ERx("IIsleep"),	gen_all_args,	0,		1,
    ERx("IIvalfld"),	gen_all_args,	0,		1,
    ERx("IInfrskact"),	gen_all_args,	0,		5,
    ERx("IIprnscr"),	gen_all_args,	0,		1,
    ERx("IIiqset"),	gen_var_args,	0,		5,
    ERx("IIiqfsio"),	gen_io,		G_RET,		5,
    ERx("IIstfsio"),	gen_io,		G_SET,		5,
    ERx("IIresmu"),	0,		0,		0,
    ERx("IIfrshelp"),	gen_all_args,	0,		3,
    ERx("IInestmu"),	0,		0,		0,
    ERx("IIrunnest"),	0,		0,		0,
    ERx("IIendnest"),	0,		0,		0,
    ERx("IIFRtoact"),	gen_all_args,	0,		2,
    ERx("IIFRitIsTimeout"),0,		0,		0,

    /* Table fields # 86 */
    ERx("IItbact"),	gen_all_args,	0,		3,
    ERx("IItbinit"),	gen_all_args,	0,		3,
    ERx("IItbsetio"),	gen_all_args,	0,		4,
    ERx("IItbsmode"),	gen_all_args,	0,		1,
# if defined(UNIX) || defined(hp9_mpe)
    ERx("IItclcol"),	gen_all_args,	0,		1,
# else
    ERx("IItclrcol"),	gen_all_args,	0,		1,
# endif /* UNIX */
    ERx("IItclrrow"),	0,		0,		0,
    ERx("IItcogetio"),	gen_io,		G_RET,		3,
    ERx("IItcoputio"),	gen_io,		G_SET,		3,
    ERx("IItcolval"),	gen_all_args,	0,		1,
    ERx("IItdata"),	0,		0,		0,
# if defined(UNIX) || defined(hp9_mpe)
    ERx("IItdtend"),	0,		0,		0,
# else
    ERx("IItdatend"),	0,		0,		0,
# endif /* UNIX */
    ERx("IItdelrow"),	gen_all_args,	0,		1,
    ERx("IItfill"),	0,		0,		0,
    ERx("IIthidecol"),	gen_all_args,	0,		2,
    ERx("IItinsert"),	0,		0,		0,
# if defined(UNIX) || defined(hp9_mpe)
    ERx("IItrcparam"),	gen_all_args,	G_NAME,		PARAM_ARGS,
    ERx("IItscparam"),	gen_all_args,	G_NAME,		PARAM_ARGS,
# else
    ERx("IItrc_param"),	gen_all_args,	G_NAME,		PARAM_ARGS,
    ERx("IItsc_param"),	gen_all_args,	G_NAME,		PARAM_ARGS,
# endif /* UNIX */
    ERx("IItscroll"),	gen_all_args,	0,		2,
    ERx("IItunend"),	0,		0,		0,
    ERx("IItunload"),	0,		0,		0,
# if defined(UNIX) || defined(hp9_mpe)
    ERx("IItvlrow"),	0,		0,		0,
# else
    ERx("IItvalrow"),	0,		0,		0,
# endif /* UNIX */
    ERx("IItvalval"),	gen_all_args,	0,		1,
    ERx("IITBceColEnd"),0,		0,		0,
    (char *)0,		0,		0,		0,
    (char *)0,		0,		0,		0,

    /* QA test calls # 111 */
    ERx("QAmessage"),	gen_var_args,	0,		13,
    ERx("QAprintf"),	gen_var_args,	0,		13,
    ERx("QAprompt"),	gen_var_args,	0,		13,
    (char *)0,		0,		0,		0,
    (char *)0,		0,		0,		0,
    (char *)0,		0,		0,		0,
    (char *)0,		0,		0,		0,
    (char *)0,		0,		0,		0,
    (char *)0,		0,		0,		0,

    /* EQUEL cursor routines # 120 */
    ERx("IIcsRetrieve"),gen_all_args,	0,		3,
    ERx("IIcsOpen"),	gen_all_args,	0,		3,
    ERx("IIcsQuery"),	gen_all_args,	0,		3,
    ERx("IIcsGetio"),	gen_io,		G_RET,		2,
    ERx("IIcsClose"),	gen_all_args,	0,		3,
    ERx("IIcsDelete"),	gen_all_args,	0,		4,
# if defined(UNIX) || defined(hp9_mpe)
    ERx("IIcsRplace"),	gen_all_args,	0,		3,
# else
    ERx("IIcsReplace"),	gen_all_args,	0,		3,
# endif /* UNIX */
# if defined(UNIX) || defined(hp9_mpe)
    ERx("IIcsRtrieveE"),0,		0,		0,
# else
    ERx("IIcsERetrieve"),0,		0,		0,
# endif /* UNIX */
    ERx("IIcsParGet"),	gen_all_args,	G_NAME,		PARAM_ARGS,
    ERx("IIcsERplace"),	gen_all_args,	0,		3,
    ERx("IIcsRdO"),	gen_all_args,	0,		2,
    ERx("IIcsRetScroll"), gen_all_args,	0,		5,
    ERx("IIcsOpenScroll"), gen_all_args, 0,		4,

    /* #133 */
/* 133 */    (char *)0,		0,		0,		0,
/* 134 */    (char *)0,		0,		0,		0,

    /* #135 Forms With Clause Support Routines  */
/* 135 */    ERx("IIFRgpcontrol"),	gen_all_args,	0,		2,
# if defined(UNIX) || defined(hp9_mpe)
/* 136 */    ERx("IIFgpsetio"),		gen_io,		G_SET,		3,
# else
/* 136 */    ERx("IIFRgpsetio"),	gen_io,		G_SET,		3,
# endif /* UNIX */   
/* 137 */    ERx("IIFRreResEntry"), 	0,		0,		0,
/* 138 */    (char *)0,		0,		0,		0,
/* 139 */    ERx("IIFRsaSetAttrio"),	gen_io,		G_SET,		4,
/* 140 */    ERx("IIFRaeAlerterEvent"), gen_all_args, 	0,              1,
/* 141 */    ERx("IIFRgotofld"),	gen_all_args,	0,		1,
/* 142 */    (char *)0,		0,		0,		0,
/* 143 */    (char *)0,		0,		0,		0,
/* 144 */    (char *)0,		0,		0,		0,

	    /* PROCEDURE support #145 */
/* 145 */    ERx("IIputctrl"),		gen_all_args,	0,		1,
/* 146 */    ERx("IILQpriProcInit"),	gen_all_args,	0,		2,
# if defined(UNIX) || defined(hp9_mpe)
/* 147 */    ERx("IILprvProcValio"),	gen_io,		G_SET,		4,
# else
/* 147 */    ERx("IILQprvProcValio"),	gen_io,		G_SET,		4,
# endif /* UNIX */
# if defined(UNIX) || defined(hp9_mpe)
/* 148 */    ERx("IILprsProcStatus"),	gen_all_args,	0,		1,
# else
/* 148 */    ERx("IILQprsProcStatus"),	gen_all_args,	0,		1,
# endif /* UNIX */
# if defined(UNIX) || defined(hp9_mpe)
/* 149 */    {ERx("IILprProcGTTParm"),	gen_all_args,   0,              2},
# else
/* 149 */    {ERx("IILQprvProcGTTParm"),gen_all_args,   0,              2},
# endif /* UNIX */
/* 150 */    (char *)0,			0,		0,		0,

		/* EVENT support #151 */
/* 151 */    (char *)0,			0,		0,		0,
/* 152 */    ERx("IILQesEvStat"),	gen_all_args,	0,		2,
/* 153 */    (char *)0,			0,		0,		0,
/* 154 */    (char *)0,			0,		0,		0,

		/* LARGE OBJECTS support #155 */
/* 155 */    ERx("IILQldh_LoDataHandler"),gen_hdl_args,	G_DAT,		4,
/* 156 */    ERx("IILQlgd_LoGetData"),  gen_datio,      G_RET,          5,
/* 157 */    ERx("IILQlpd_LoPutData"),  gen_datio,      G_SET,          4,
/* 158 */    ERx("IILQled_LoEndData"),	0,		0,		0,
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
+* Procedure:	gen_init
** Purpose:	Initialize the indent table.
** Parameters:
**	None.
**	p2	- type	- use
** Return Values:
-*	None.
** Notes:
**	Called from GR_SYMINIT at the beginning of every file
**	to make sure we start cleanly.
*/

void
gen_init()
{
    register i4	i;

    g_indlevel = 0;
    for (i=0; i<G_MAXINDENT; i++)
	g_indent[i] = ' ';
    g_indent[0] = '\0';

}

/*
** gen_call - Generates a standard call to an II function.
**
** Parameters: 	Function index.
** Returns: 	None
**
** Format: 	call func( args )
**
** History:
**      01-dec-1992 (kathryn)
**          New gen_datio also requires func as parameter.
*/

void
gen_call( func )
i4	func;
{
    register GEN_IIFUNC         *g;

    g = gen__functab( func );
    if (g->g_func == gen_io)
  	gen_io( func );
    else if (g->g_func == gen_datio)
	gen_datio(func);
    else
    {
	gen__obj( TRUE, ERx("call ") );
	gen__II( func );
	out_emit( ERx("\n") );
    } 
}


/*
** gen_if - Generates a simple If statement (Open or Closed).
**
** Parameters: 	Open / Close the If block,
**		Function index,
**		Condition index,
**		Right side value of condition.
** Returns: 	None
**
** Format: (open) 	if ( func(args) condition val ) then
** 	   (close)      end if
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
	gen__obj( TRUE, ERx("if(") );				/* If */
	gen__II( func );					/* Function */
	gen__obj( TRUE, gen_condtab[C_RANGE + cond] );		/* Condition */
	gen__int( FbyNAME, val );				/* Value */
	gen__obj( TRUE, ERx(")") );				/* Then */
	gen__obj( TRUE, ERx("then \n") );			/* Begin */
	gen_do_indent( 1 );

	/* Next call should Close or Else */
    } else if (o_c == G_CLOSE)
    {
	gen_do_indent( -1 );
	gen__obj( TRUE, ERx("end if ") );			/* End */
# if defined(UNIX) || defined(hp9_mpe) || defined (NT_GENERIC)
	out_emit( ERx("\n") );
# else 
	gen_comment( TRUE, gen__functab(func)->g_IIname );
# endif /* UNIX */
    }
}


/* 
** gen_else - Generate an If block with Else clauses.
**
** Parameters: 	Open / Else / Close the If block, 
**		Function index,
**		Condition index,
**		Right side value of condition,
**		Else label prefix,
**		Else label number,
**		Endif label prefix,
**		Endif label number.
** Returns: 	None
**
** Format: (open) 	if ( func(args) condition val ) then
**	   (else)	else
** 	   (close)      end if
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
    if (flag == G_OPEN && func == II0)	/* Last Else block */
    {
	gen__obj( TRUE, ERx("\n") );
	gen_do_indent( 1 );
    } else if (flag == G_ELSE)
    {
	gen_do_indent( -1 );
	gen__obj( TRUE, ERx("else ") );		/* Followed by an If or II0 */
    } else
	gen_if( flag, func, cond, val );	/* Either Open or Close */
}


/*
** gen_if_goto - Generate an If Goto template.
**
** Parameters: 	Function index,
**		Condition index,
**		Right side value of condition,
**		Label prefix,
**		Label number.
** Returns: 	None
**
** Format: 	if ( func(args) condition val ) goto label;
*/

void
gen_if_goto( func, cond, val, labprefix, labnum )
i4	func;
i4	cond;
i4	val;
i4	labprefix;
i4	labnum;
{
    gen__obj( TRUE, ERx("if(") );				/* If */
    gen__II( func );					/* Function */
    gen__obj( TRUE, gen_condtab[C_RANGE + cond] );	/* Condition */
    gen__int( FbyNAME, val );				/* Value */
    gen__obj( TRUE, ERx(") ") );				/* Then */
    gen__goto( labprefix, labnum );			/* Goto Label */
}


/*
** gen_loop - Dump a while loop (open the loop or close it).
**
** Parameters: 	Open / Close the loop,
**		Start label prefix,
**		End label prefix,
**		Loop label number.
**		Function index,
**		Condition index,
**		Right side value of condition.
** Returns: 	None
**
** Format: (open)	do while ( func(args) condition val )
**	   (close)	end do
**
** F77: no while loop, so implement with labels and gotos
** 	   (open)	labbeg     continue
**				   if ( func(args condition val ) goto labend
**	   (close)	           goto labbeg
**		        labend     continue
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
# if defined(UNIX) || defined(hp9_mpe)

    if (o_c == G_OPEN)
    {
	gen_label( G_NOLOOP, labbeg, labnum );
	gen_if_goto( func, -cond, val, labend, labnum );
	gen_do_indent( 1 );
    } 
    else
    {
	/* Go to begin label, and dump endloop label */
	gen_do_indent( -1 );
	gen__goto( labbeg, labnum, TRUE );
	gen_label( G_NOLOOP, labend, labnum );
    }

# else

    static i4	g_loop = 0;		 /* Keep track of loops */

    if (o_c == G_OPEN)
    {
	gen__obj( TRUE, ERx("do while(") );			/* While */
	gen__II( func );					/* Function */
	gen__obj( TRUE, gen_condtab[C_RANGE + cond] );		/* Condition */
	gen__int( FbyNAME, val );				/* Value */
	gen__obj( TRUE, ERx(")\n") );				/* Do Begin */
	g_loop++;
	gen_do_indent( 1 );
    } else
    {
	if (g_loop > 0)
	{
	    gen_do_indent( -1 );
	    gen__obj( TRUE, ERx("end do ") );
	    gen_comment( TRUE, gen__functab(func)->g_IIname );
	    g_loop--;
	}
    }

# endif /* UNIX */
}


/*
** gen_goto - Put out a Goto statement.
**
** Parameters: 	Goto flag - use If condition ?
**		Label prefix,
**		Label number.
** Returns: 	None
**
** Format:     [ if (.TRUE.) ] goto label;
**
** Note that this routine is called from the grammar, and may need the If
** clause to prevent compiler warnings.  The 'flag' tells the routine
** that this is a 'loop break out' statement (ie: ## endloop) and requires
** the If clause.  
*/

void
gen_goto( flag, labprefix, labnum )
i4	flag;
i4	labprefix;
i4	labnum;
{
    if (flag == G_IF)
	gen__obj( TRUE, ERx("if(.TRUE.) ") );
    gen__goto( labprefix, labnum );
}

/*
** gen__goto - Local statement independent goto generator.  
*/

void
gen__goto( labprefix, labnum )
i4	labprefix;
i4	labnum;
{
    gen__obj( TRUE, ERx("goto ") );				/* Goto */
    gen__label( TRUE, labprefix, labnum );			/* Label */
    out_emit( ERx("\n") );
}

/*
** gen_label - Put out a constant label.
**
** Parameters: 	Loop flag - is it extra label for a loop ? (see usage in G)
**	 	Label prefix,
**		Label number.
** Returns: 	None
**
** Format:      label:
**
** Note: We don't generate extra labels for loops in F77 because we will
**	 generate a labeled loops, and an "extra" label would turn out to 
**	 be a duplicate label.
**	 In VMS we always generate a CONTINUE statement, so we don't need
**	 to look at loop_extra.
*/

void
gen_label( loop_extra, labprefix, labnum )
i4	loop_extra;
i4	labprefix;
i4	labnum;
{
# if defined(UNIX) || defined(hp9_mpe)
    if (loop_extra == G_LOOP)		/* Do not print extra loop labels */
	return;
# endif /* UNIX */

    /* Labels must always begin in the left margin */
    gen__label( FALSE, labprefix, labnum );
    out_emit( ERx("\n") );
}

/*
** gen__label - Local statement independent label generator.  
**
** Parameters:	Indentation flag - Indicates whether or not label definition
**		label prefix     - index into table of label prefixes
**		number           - suffix for number
*/

void
gen__label( ind, labprefix, labnum )
i4	ind;
i4	labprefix;
i4	labnum;
{
    char	buf[80];
    register struct gen_ltable
		*p = &gen_ltable[labprefix];

    if (!ind)			/* label definition */
    {

# if defined(UNIX) || defined(hp9_mpe)
#   ifdef STRICT_F77_STD
	STprintf( buf, ERx("%-6dcontinue"),p->gl_num+(labnum % L_INTERVAL) );
#   else
	STprintf( buf, ERx("%d\tcontinue"),p->gl_num+(labnum % L_INTERVAL) );
#   endif /* STRICT_F77_STD */
# else
	STprintf( buf, ERx("%d\tcontinue\t\t! %s+%d"),
	    p->gl_num+(labnum % L_INTERVAL), p->gl_name, labnum );
# endif /* UNIX */

	gen__obj( ind, buf );
    } else			/* label use */
    {
	STprintf( buf, ERx("%d "), p->gl_num+(labnum % L_INTERVAL) );
	gen__obj( ind, buf );

# if !defined(UNIX) && !defined(hp9_mpe)
	STprintf(buf, ERx("%s+%d"), p->gl_name, labnum );
	gen_comment( FALSE, buf );
# endif /* UNIX */

    }
}

/*
** gen_comment - Make a comment with the passed string. Should be used
**		 by higher level calls, but may be used locally.
**		 Comment lines can't be continued; if it won't fit
**		 then start it on the next line.
**
** Parameters: 	Comment terminated,
**		Comment string.
** Returns: 	None
*/

void
gen_comment( term, str )
i4	term;
char	*str;
{
    /*
     * F77 and VMS FORTRAN ignore anything past column 72,
     * so it doesn't hurt to have comments there.
     */
    out_emit( BEGIN_COMMENT );
    out_emit( str );
    if (term)
	out_emit( ERx("\n") );
}

/*
** gen_include - Generate an include file statement.
**
** Parameters: 	Prefix, and suffix separated by a '.',
**		Any extra qualifiers etc.
** Returns: 	None
*/

void
gen_include( pre, suff, extra )
char	*pre;
char	*suff;
char	*extra;
{
    /* The whole include line should be atomic */
    out_emit( INCLUDE_STRING );
    out_emit( pre );
    if (suff  != (char *)0)
    {
	out_emit( ERx(".") );
	out_emit( suff );
    }
    if (extra  != (char *)0)
	out_emit( extra );
    out_emit( ERx("'\n") );
}

/*
** gen_eqstmt - Open / Close an Equel statement.
**
** Parameters:	Open / Close,
**		Comment.
** Returns:	None
**
** Based on the language we may want to force statements into pseudo blocks
** (C or PL1), add a comment about the line number and file, or generate
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
    } 
# if !defined(UNIX) && !defined(hp9_mpe)
    else
    {
	if (cmnt)
	    gen_comment( FALSE, cmnt );
    }
# endif /* UNIX */
}

/*
** gen_line - Generate a line comment or directive.
**
** Parameters:	Optional comment string.
** Returns:	None
*/

void
gen_line( s )
char	*s;
{
    char	fbuf[G_MAXSCONST +20]; 

    STprintf( fbuf, ERx("File \"%s%s%s\", Line %d"), eq->eq_filename,
	      eq->eq_infile == stdin ? ERx("") : ERx("."), eq->eq_ext,
	      eq->eq_line );
    if (s)
    {
	STcat( fbuf, ERx("\t-- ") );
	STcat( fbuf, s );
    }
# if defined(UNIX) || defined(hp9_mpe)
    out_emit( ERx("\n") );
# endif /* UNIX */
    gen_comment( TRUE, fbuf );
}


/*
** gen_term - Terminate a statement.
** Unused by FORTRAN.
*/

void
gen_term() 
{ 
}

/*
** gen_declare - Process a ## declare/## declare forms statement.
**
** Parameters:  None
** Returns:	None
**
** Generates:	include 'ingres_path_eqdef.for'      (VMS)
**		include '/ingres_path/eqdef.f'       (Unix, ## declare, 
**							    ## declare forms)
**		include '/ingres_path/eqfdef.f'      (Unix, ## declare forms)
*/

void
gen_declare(isforms)
i4	isforms;		/* ## declare forms vs. ## declare */
{
    LOCATION	loc; 
    char	buf[MAX_LOC +1];
    char	*lname;
    i4		len;
    static bool	first = TRUE;

    /* Spit out real Include line */
#if defined(UNIX) && defined(conf_BUILD_ARCH_32_64)
    if (eq->eq_flags & EQ_FOR_DEF64)
       STcopy( INC_FILE64, buf);
    else
# endif /* defined(UNIX) && hybrid */
    STcopy( INC_FILE, buf );

    NMloc( FILES, FILENAME, buf, &loc );
    LOtos( &loc, &lname );
    gen_include( lname, (char *)0, (char *)0 );

# ifdef UNIX   /* MPE/XL only uses 1 include file */
    if (isforms)
    {
	STcopy( ERx("eqfdef.f"), buf );
	NMloc( FILES, FILENAME, buf, &loc );
	LOtos( &loc, &lname );
	gen_include( lname, (char *)0, (char *)0 );
    }
# endif /* UNIX */

    if (first)
    {
      /* Now for some trickery ! */
	NMgtAt( ERx("II_FOR_WHO"), &lname );
	if (lname != (char *)0 && *lname != '\0')
	{
	    SIprintf( ERx("## EQUEL FORTRAN\n") );
	    SIprintf( ERx("## Written by Mark Wittenberg and Barbara Banks (RTI), June 1986.\n") );
	    SIflush( stdout );
	}
	first = FALSE;
    }
}

/*
** gen_makesconst - Make a host language string constant.
**
** Parameters:	Flag -  G_INSTRING  - Nested in another string (DB string),
**		        G_REGSTRING - String sent by itself in a call.
**		Instr - Incoming string.
**		Diff  - Mark if there were changes made to string.
** Returns:	Pointer to converted string.
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
    static	char	obuf[ G_MAXSCONST + 4 ];	/* Converted string */
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
	if (*icp == '\\')		/* Leave '\' - may be DB or \0 forms */
	{	
/*
** Some F77 compilers treat backslash as an escape character
** -- the same as C compilers.
*/
# ifdef F77ESCAPE
	    if (flag == G_INSTRING)	/* Double for compiler */
	    {
		*ocp++ = '\\';
		*diff = 1;
	    }
# endif /* F77ESCAPE */

	    icp++;
	    *ocp++ = '\\';
	    esc = 1;
	} else if (*icp == '"')
	{
	    icp++;
	    if (flag == G_INSTRING && !esc)	/* DB needs escape */
	    {
# ifdef F77ESCAPE
		*ocp++ = '\\';		/* Double for compiler */
# endif /* F77ESCAPE */
		*ocp++ = '\\';
		*diff = 1;
	    }
	    *ocp++ = '"';
	    esc = 0;
	} else if (*icp == '\'')	/* Need to escape (double) it */
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
	} else
	{
	    CMcpyinc(icp, ocp);
	    esc = 0;
	}
    }
    if (flag == G_INSTRING)		/* Add closing quote */
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
** gen__II - Generate the real II call with the necessary arguments, already
**	    stored by the argument manager.
**
** Parameters: 	Function index.
** Returns: 	None
**
** This routine assumes language dependency, and knowledge of the actual 
** arguments required by the call.
*/

void
gen__II( func )
i4	func;
{
    register GEN_IIFUNC		*g;

    g = gen__functab(func);
    gen_funcname( g->g_IIname, FALSE );
    gen__obj(TRUE, ERx("(") );
    if (g->g_func)
	_VOID_ (*g->g_func)( g->g_numargs, g->g_flags );
    gen__obj(TRUE, ERx(")") );
    arg_free();
}

    
/*
** gen_all_args - Format all the arguments stored by the argument manager.
**
** Parameters: 	Number of arguments required, and flag bits.
** Returns: 	0
**
** This routine should be used when no validation of the number of arguments 
** needs to be done. Hopefully, if there was an error before (or while) 
** storing the arguments this routine will not be called.
** The flags argument helps establish whether VMS arguments are to be
** passed by name or by value.  Note that F77 args are always passed by 
** reference, but because the lower routines know this and generate code
** accordingly, F77 doesn't care about 'flags' at this level.
*/

i4
gen_all_args( numargs, flags )
i4	numargs;
i4	flags;
{
    register	i4 i;

	if (flags & G_NAME)
	    flags = FbyNAME;
	else
	    flags = FbyVAL;

    for (i = 1; i <= numargs; i++)	/* Fixed number of arguments */
    {
	gen_data( i, flags );
	if (i < numargs) 
	    gen__obj( TRUE, ERx(",") );
    }
    return 0;
}

/*
** gen_var_args - Format varying number of arguments stored by the argument 
**		   manager.
**
** Parameters: 	numargs - Max number of arguments, and flag bits required.
** Returns: 	0
**
** Notes:	
** Generates:	(arg1, arg2, arg3, )
*/

i4
gen_var_args( numargs, flags )
i4	numargs;
i4	flags;
{
    register	i4 i;

# if defined(UNIX) || defined(hp9_mpe)

    gen__int( FbyREF, arg_count() );		/* Number of args to come */
    gen__obj( TRUE, ERx(",") );
    if (i=arg_next())
	gen_data( i, FbyREF );
    while ((i=arg_next()) && i <= numargs)	/* Varying num of arguments */
    {
	gen__obj( TRUE, ERx(",") );
	gen_data( i, FbyREF );
    }

# else 
#   ifdef DGC_AOS
       gen__int( FbyREF, arg_count() );		/* Number of args to come */
       gen__obj( TRUE, ERx(",") );
#   endif /* DGC_AOS */

    flags = (flags & G_NAME) ? FbyNAME : FbyVAL; /* Select arg passing mech */

    while ((i=arg_next()) && i < numargs)	/* Varying num of arguments */
    {
	gen_data( i, flags );
	gen__obj( TRUE, ERx(",") );
    }
    gen__int( FbyVAL, 0 );		

# endif /* UNIX */

    return 0;
}

/*
** gen_io - Generate an I/O variable to the output file.
**
** Parameters: 	func - Function number.
** Returns: 	0
**
** Note:
**  1.  There will always  be a minimum of two arguments: the null indicator
**	and the actual argument.  If there are just 2 arguments then we rely
**	on the following template format:
**		IIretfunc( indaddr, var_desc );
**		IIsetfunc( indaddr, var_or_val_desc );
**      If there are > 2 arguments then we rely on the following template 
**      format:
**		IIretfunc( indaddr, var_desc, "object" );
**		IIsetfunc( "object", indaddr, var_or_val_desc );
**	where var_desc or var_or_val_desc is:
**		ISVAR, TYPE, LEN, var_or_val
**	ISVAR really means isref.  Note that F77 arguments always get sent 
**	by reference so, isvar arg isn't used at runtime; however we keep the 
**	template consistent with all other languages.
**      We figure out which one is the null indicator, which is the data i
**	argument which must be described and the others we just dump.
*/

i4
gen_io( func )
i4	func;
{
    register GEN_IIFUNC		*g;		/* Current I/O function */
    SYM				*var;		/* Symtab info */
    register i4		ioarg;		/* What is the I/O argument */
    i4				artype;		/* Type of I/O argument */
    i4				i4num;		/* Numerics stored as strings */
    f8				f8num;
    char			datbuf[ 50 ];	/* To pass to gen_iodat */
    char			*data = datbuf;
    char			*strdat;	/* String rep */

    g = gen__functab(func);
    /* 
    ** Figure out which is I/O argument based on number of args and mode:
    ** set (name = value) or ret (var = name).  G_RETSET is like G_SET in
    ** style of template.
    */
    ioarg = (g->g_flags == G_RET) ? 2 : g->g_numargs;
    if ((var=arg_sym(ioarg)) == (SYM *)0)    /* Constant used - get real data */
    {
	if (g->g_flags == G_RET || g->g_flags == G_RETSET)   /* User error */
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
	    gen_iodat( func, FALSE, T_INT, sizeof(i4), data );
	    break;
	  case ARG_FLOAT:
	    if (strdat)
		data = strdat;
	    else		/* Convert float to string */
	    {
		arg_num( ioarg, ARG_FLOAT, &f8num );
		STprintf( data, ERx("%f"), f8num, '.' );
	    }
	  /* VMS/F77 plain floating constants are 4-byte reals */
	    gen_iodat( func, FALSE, T_FLOAT, sizeof(f4), data );
	    break;
	  case ARG_PACK:
	    gen_iodat( func, FALSE, T_PACKASCHAR, 0, arg_name(ioarg) );
	    break;
	  case ARG_CHAR:
	  default:
	    gen_iodat( func, FALSE, T_CHAR, 0, arg_name(ioarg) );
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
# if !defined(UNIX) && !defined(hp9_mpe)
# 	ifndef DGC_AOS
	   /* Always use i4 by value; %val is always i4 */
	   if (g->g_flags == G_SET) 
	      gen_iodat( func, FALSE, T_INT, sizeof(i4), data );
	   else
# 	endif /* DGC_AOS */
# endif /* UNIX */
	  /* F77 never sends by value */
	    gen_iodat( func, TRUE, T_INT, sym_g_dsize(var), data );
	break;
      case T_NUL:
	gen_iodat( func, FALSE, artype, 1, data );
	break;
      case T_FLOAT:
      case T_CHAR:
      default:		/* error or T_NUL type */
	gen_iodat( func, TRUE, artype, sym_g_dsize(var), data );
	break;
    }
    arg_free();
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
**	    Also switched the arguments.
**	01-oct-1993 (kathryn) Bug 55775
**	    Call gen_null to generate null indicator variable.
*/
i4
gen_hdl_args(numargs, flags )
i4      numargs, flags;
{
    i4          num;
    i4	        arg;
    char	buf[ G_MAXSCONST +1 ];
    char	argbuf[ G_MAXSCONST +1 ];

    arg_num( 1, ARG_INT, &num );
    gen__int( FbyVAL, num );
    gen__obj( TRUE, ERx(",") );

    arg = 2;
    if (flags == G_DAT)
    {
	gen_null(arg);
	arg++;
    }
    STprintf(buf,ERx("%s"),arg_name(arg));
    switch (arg_type(arg))
    {
        case ARG_INT:
    		if (STcompare(buf,ERx("0")) == 0)
			gen__name( FbyVAL, buf);
		else
		{
		    gen__obj( TRUE, ERx("0") );
    		    er_write(E_EQ0090_HDL_TYPE_INVALID, EQ_ERROR, 1, buf);
		}
    		break;
        case  ARG_RAW:
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
    		gen__int( FbyVAL, 0 );
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
**	It is assumed that all arguments have been previously validated for 
**	correct datatype.  The I/O variable is always passed by reference.  
**	There are two possible ret_set flags
**	G_RET (GET DATA) and G_SET (PUT DATA), which determine how the
**	the remaining arguments will be passed. Note that for the GET/PUT
**	DATA statements all arguments are optional.  The "attrs" argument is
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
**	  attrs  :  i4		mask indicating attributes specified.
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
**	01-oct-1993   (kathryn) 
**          First arg generated, ("attrs" bit mask), is the last arg stored.
**          It is now set/stored in eqgen_tl(), rather than determined here.
*/
i4
gen_datio(func)
i4      func;
{

    register i4            ar;
    i4                     artype;         	/* Type of I/O argument */
    i4                     type;
    i4                     len;
    i4                     i4num;          	/* Numerics stored as strings */
    i4                     isvar;
    i4                     attrs;
    f8                     f8num;
    char                   *strdat;
    char                   datbuf[ 50 ];
    char                   *data = datbuf;
    char                   buf[ G_MAXSCONST +1 ];
    SYM                    *sy;            	/* Symtab info */
    register GEN_IIFUNC    *gio;             	/* Current I/O function */


    /* Process the I/O argument */
    ar = 1;
    strdat = (char *)0;

    if ((sy = arg_sym(ar)) != (SYM *)0)    /* User var used */
    {
        isvar = 1;
        data = arg_name(ar);
        type = sym_g_btype(sy);
        len = sym_g_dsize(sy);
    }
    else if (!arg_name(ar))
    {
        isvar = 0;
        type = T_NONE;
        len = 0;
    }
    else
    {
        isvar = 0;
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
                len = sizeof(i4);
                break;
            case ARG_FLOAT:
                if (!strdat)
                {
                    arg_num(ar, ARG_FLOAT, &f8num);
                    STprintf( data, ERx("%f"), f8num, '.' );
                }
                len = sizeof(f4);
                type = T_FLOAT;
                break;
            case ARG_PACK:
                type = T_PACKASCHAR;
                len = 0;
                break;
            case ARG_CHAR:
            default:
                type = T_CHAR;
                len = 0;
                break;
        }
    }
    gio = gen__functab(func);
    gen__obj( TRUE, ERx("call ") );
    if (type == T_CHAR)
    {
        len = 0;
        gen_funcname( gio->g_IIname, TRUE );
    }
    else
        gen_funcname( gio->g_IIname, FALSE );

    gen__obj(TRUE, ERx("(") );

  /* Dump data information - ATTRS, TYPE, LENGTH */

    arg_num( gio->g_numargs, ARG_INT, &attrs ); 
    gen__int( FbyVAL,attrs);
    gen__obj( TRUE,  ERx(",") );
    gen__int( FbyVAL, type == T_NUL ? -type : type );
    gen__obj( TRUE,  ERx(",") );

# if defined(DG_OR_UNIX) || defined(hp9_mpe) || defined(NT_GENERIC)
    /*
    ** Bug 110959
    ** Enable string check on Windows too.
    ** For Windows generate call to evaluate string length at runtime.
    */    
    if (type == T_CHAR)
        gen__slen( isvar, data );
    else
# endif /* DG_OR_UNIX */
        gen__int( FbyVAL, len );
    gen__obj( TRUE,  ERx(",") );

  /* Dump I/O argument based on type */
    switch (type)
    {
      case T_INT:
# if !defined(UNIX) && !defined(hp9_mpe)
        if (gio->g_flags == G_SET)
                gen__name(FbyVAL, data);
        else
# endif /* UNIX */
                gen__name( isvar ? FbyNAME : FbyVAL, data );
        break;
      case T_FLOAT:
        gen__name( isvar ? FbyNAME : FbyVAL, data );
        break;
      case T_NUL:
      case T_NONE:
        gen__int( FbyVAL, 0 );          /* But will go by ref for F77 */
        break;
      case T_PACKASCHAR:
        gen_sdesc( FALSE, FALSE, data); /* Convert to C string no trim needed */
        break;
      default:  /* T_CHAR */
        /*
        ** Character I/O arguments go by descriptor for VMS, and 'IIx'
        ** layer processes.  For Unix, send by ref and let layer process.
        */
# if defined(UNIX) || defined(hp9_mpe)
            gen__sadr( isvar, data );
# else
            if (isvar)
                gen__obj( TRUE, data );
            else
                gen_sconst( data );
# endif /* UNIX */
        break;
    }
    for (ar = 2; ar < gio->g_numargs; ar++)
    {
        gen__obj( TRUE, ERx(",") );
        if (!arg_name(ar))
            gen__int( FbyVAL, 0 );
        /* ints by reference for get data */
        else if (gio->g_flags == G_RET && ar != 2) 
        {
            gen_data(ar, FbyREF);
        }
        else
            gen_data(ar, FbyVAL);
    }
    out_emit( ERx(")") );
    out_emit( ERx("\n") );
    arg_free();

    return 0;
}

/*
** gen_iodat - Make real I/O call.
**
** Parameters: 	func	- Function index,
**		isvar, type len - Data information,
**		data	- Data as a string (i.e., value or var name)
** Returns: 	None
**
** Note: See gen_io comment for what we need to generate.  For VMS, there
**	is a special case when the I/O data is of type character.  In this
**	case we actually change the name of the call from 'IIabcfunc' to
**	'IIxabcfunc' and pass the VMS string descriptor.  The 'IIx' layer
**	maps the character data type to an internal data type which is
**	understood to represent 'a non-C string'.   For Unix, since all
**	calls go through a layer, there is no need for the 'IIx' convention,
**	but in that layer, character data is mapped to the internal data type.
**	When passing strings as 'objects' (i.e., form names, etc.)  we generate
**	a call to IIsd which will pass back a C-string at runtime.  In other
**	words, names must always go as C-strings, whereas data can now be
**	passed without the EOS terminator.
**
**	Example:
**	   Unix:
**	    IIsetf(IIsd("strobj"), indaddr, ISVAR, T_CHAR, IIslen(strvar), IIsadr(strvar));
**	    IIretf(indaddr, ISVAR, T_CHAR, IIslen(strvar), IIsadr(strvar), IIsd("strobj"));
**	   VMS:
**	    IIxretfunc( indaddr, ISVAR,T_CHAR,0, strvar, %val IIsd("strobj") );
**	    IIxsetfunc( %val IIsd("strobj"), indaddr,ISVAR,T_CHAR,0, strvar );
**	    
**  History:
**      22-Sep-2003 (fanra01)
**          Bug 110907
**          Enable runtime length generation for parameter for windows.
*/

void
gen_iodat( func, var, type, len, data )
i4		func;
i4		var;		/* Useful only for args of type T_CHAR */
register i4	type; 
i4		len;
register char	*data;
{
    register GEN_IIFUNC		*g;		/* Current I/O function */
    i4				ar;

    g = gen__functab(func);

    gen__obj( TRUE, ERx("call ") );

# if !defined(NT_GENERIC)
    /*
    ** Don't initialize the passed string length for Windows as it's
    ** needed to pad the string buffer since there is no mechanism to
    ** know the length of the passed buffer.
    */
    if (type == T_CHAR)
	len = 0;    /* VMS descriptor has the real size: this is "is_string?" */
# endif /* NT_GENERIC */

  /* Get func name out - special case retrieving strings, if not DG */
# ifndef DGC_AOS
    if (type == T_CHAR)
	gen_funcname( g->g_IIname, TRUE );
    else
# endif /*DGC_AOS */
	gen_funcname( g->g_IIname, FALSE );
    gen__obj(TRUE, ERx("(") );

  /* Do we need a name or other data before I/O argument */
    if (g->g_flags == G_SET || g->g_flags == G_RETSET)	
    {
	for (ar = 1; ar <= g->g_numargs-2; ar++)
	{
	    gen_data( ar, FbyNAME );
	    gen__obj( TRUE, ERx(",") );
	}
	gen_null(g->g_numargs - 1);
    }
    else
    {
	gen_null(1);
    }

  /* Dump data information - ISVAR, TYPE, LENGTH */
# ifdef DGC_AOS
    gen__int( FbyVAL, 1);			/* DG always passes by ref */
# else
    						/* will go by ref for F77 */
    gen__int( FbyVAL, var || type == T_CHAR || type == T_PACKASCHAR);	
# endif /* DGC_AOS */
    gen__obj( TRUE,  ERx(",") );
    gen__int( FbyVAL, type == T_NUL ? -type : type );
    gen__obj( TRUE,  ERx(",") );

# if defined(DG_OR_UNIX) || defined(hp9_mpe) || defined(NT_GENERIC)
    if (type == T_CHAR)
	gen__slen( var, data );
    else
# endif /* DG_OR_UNIX */
        gen__int( FbyVAL, len );

    gen__obj( TRUE,  ERx(",") );

  /* Dump I/O argument based on type */
    switch (type)
    {
      case T_INT:
      case T_FLOAT:
	gen__name( var ? FbyNAME : FbyVAL, data );
	break;
      case T_NUL:
        gen__int( FbyVAL, 0 );		/* But will go by ref for F77 */
	break;
      case T_PACKASCHAR:
	gen_sdesc( FALSE, FALSE, data); /* Convert to C string no trim needed */
	break;
      default:	/* T_CHAR */
	/* 
	** Character I/O arguments go by descriptor for VMS, and 'IIx'
	** layer processes.  For Unix, send by ref and let layer process.
	*/
# if defined(UNIX) || defined(hp9_mpe)
	gen__sadr( var, data );
# else
	if (var)
	    gen__obj( TRUE, data );
	else
#	    ifdef DGC_AOS
	    	gen_sdesc( TRUE, FALSE, data );	/* Always trim it */
#	    else
	    	gen_sconst( data );
#	    endif /* DGC_AOS */
# endif /* UNIX */
	break;
    }
  /* Do we need a name or other data after I/O argument */
    if (g->g_flags == G_RET)			/* (var = name ) */
    {
	for (ar = 3; ar <= g->g_numargs; ar++)
	{
	    gen__obj( TRUE, ERx(",") );
	    gen_data( ar, FbyNAME );
	}
    }
    gen__obj(TRUE, ERx(")") );
    out_emit( ERx("\n") );
}

/*
** gen_data - Whatever is stored by the argument manager for a particular 
**	      argument, dump to the output file.  
**
** Parameters: 	Argument number.
** Returns: 	None
**
** This routine should be used when no validation of the data of the argument 
** needs to be done. Hopefully, if there was an error before (or while) 
** storing the argument this routine will not be called.
*/

void
gen_data( arg, how )
register i4	arg;
i4		how;
{
    i4		num;
    f8		flt;
    char	*s;
    SYM		*var;

    if (var = arg_sym(arg))		/* User variable */
    {
	if (sym_g_btype(var) == T_CHAR)
	    gen_sdesc( TRUE, TRUE, arg_name(arg) );
	else		/* must be T_INT */
# ifdef DGC_AOS
        {
		/* 
	    	** If an integer*2, then will need to move it to an integer*4
	   	** in the layer, IIstol, (short-to-long)
	       	*/
		if (sym_g_dsize(var) < sizeof(i4))
		{
			gen__obj( TRUE, ERx("IIstol(") );
			gen__obj( TRUE, arg_name(arg) );
			gen__obj( TRUE, ERx(")") );
		}
		else
# endif /* DGC_AOS */
	    		gen__name( how, arg_name(arg) );
	return;
    }

    /* Based on the data representation, dump the argument */
    switch (arg_rep(arg))
    {
      case ARG_INT:  
	arg_num( arg, ARG_INT, &num );
	gen__int( FbyVAL, num );
	return;
      case ARG_FLOAT:  			/* can't happen */
	arg_num( arg, ARG_FLOAT, &flt );	/* could just use 0 */
	gen__float( FbyVAL, flt );
	return;
      case ARG_CHAR:  
	switch (arg_type(arg))
	{
	  case ARG_RAW:		/* Raw data stored */
	    gen__name( FbyNAME, arg_name(arg) );
	    break;
	  case ARG_FLOAT:	/* "can't happen" */
    	  case ARG_CHAR: 	/* String constant */
	  case ARG_PACK:	/* Decimal constant pass as a string */
	    if (s = arg_name(arg))  
		gen_sdesc( TRUE, FALSE, s );	/* Always trim it */
	    else
# if defined(i64_hpu) || ( defined(LNX) && defined(LP64))
		gen__obj(TRUE, ERx("int8(0)"));
# elif defined(UNIX) && defined(conf_BUILD_ARCH_32_64)
	{
	if (eq->eq_flags & EQ_FOR_DEF64)    
		gen__obj( TRUE, ERx("0_8"));
	else
		gen__int( FbyVAL, 0 );
	}
# else
		gen__int( FbyVAL, 0 );
# endif
	    break;
	  case ARG_INT:
	    gen__name( how, arg_name(arg) );
	    break;
	  case ARG_PSTRUCT:  /* structured data, pass as deref'ed pointer */
	    gen_pstruct( TRUE, arg_name(arg) );	
	    break;
	}
	return;
    }
}

/*
+* Procedure:	gen_funcname
** Purpose:	Generates short form of subroutine name for F77
** Parameters:
**	funcname- *char	- Full name of subroutine (from functab)
**	addx    - i4    - Insert an 'x' after initial 'I'
** Return Values:
-*	Nothing
** Notes:
**	This routine is called anwhere a subroutine call is generated
** in code generation.  For F77 it generates a six-character subroutine
** name by truncating the name.  Since F77 EQUEL programs always go through a 
** runtime layer, these abbreviated runtime names get resolved there.
**
** Imports modified:
**	None
*/

void
gen_funcname( funcname, addx )
char	*funcname;
i4	addx;				/* VMS flag for RET-type strings */
{
# if defined(UNIX) || defined(hp9_mpe)

# define	FSYMLEN	6		/* F77 needs symbols of <= 6 chars */
    char	buf[FSYMLEN +1];
    
    STlcopy( funcname, buf, FSYMLEN );
    gen__obj( TRUE, buf );

# else
    if (addx)
    {
        gen__obj( TRUE, ERx("IIx") );	/* Extra x for VMS string handling */
	out_emit( funcname+2 );
    }
    else
	gen__obj( TRUE, funcname );
# endif /* UNIX */
}


/*
** gen_sdesc - Put out function call to IIsd to convert FORTRAN strings to 
**	       C strings (trimming and null terminating).
**
** Parameters:	trim  - Trim or not,
**		isvar - Is this a variable or a string constant,
**		data  - String to dump.
** Returns:	None
** Notes:	(VMS) IIsd and IIsd_no always pass by value.
** 		(F77) IIsd and IIsdno always pass by reference.
*/

void
gen_sdesc( trim, isvar, data )
i4	trim; 		
i4	isvar;
char	*data;
{

# if !defined(UNIX) && !defined(hp9_mpe)
#    ifndef DGC_AOS
        gen__obj( TRUE, ERx("%val(") );
#    endif /* DGC_AOS */
# endif /* UNIX */

    if (trim)
	gen__obj( TRUE, ERx("IIsd(") );
    else

# if defined(DG_OR_UNIX) || defined(hp9_mpe)
	gen__obj( TRUE, ERx("IIsdno(") );
# else
	gen__obj( TRUE, ERx("IIsd_no(") );
# endif /* DG_OR_UNIX */

    if (isvar)
	gen__obj( TRUE, data );
    else
	gen_sconst( data );

# if defined(DG_OR_UNIX) || defined(hp9_mpe)
    gen__obj( TRUE, ERx(")") );
# else
    gen__obj( TRUE, ERx("))") );
# endif /* DG_OR_UNIX */
}


/*
** gen_pstruct - Put out function call to IIps to convert FORTRAN 
**	       structures to a dereferenced one.
**
** Parameters:	isvar - Is this a variable or a string constant,
**		data  - String to dump.
** Returns:	None
** Notes:	(VMS) IIpstruct always pass implicitly.
** 		(F77) IIpstruct always pass by reference.
*/

void
gen_pstruct( isvar, data )
i4	isvar;
char	*data;
{

# if defined(UNIX) || defined(hp9_mpe)
     gen__obj( TRUE, ERx("IIps(") );
# endif 

    if (isvar)
	gen__obj( TRUE, data );
    else
	gen_sconst( data );

# if defined(UNIX) || defined(hp9_mpe)
     gen__obj( TRUE, ERx(")") );
# endif 
}

/*
** gen_IIdebug - Dump file name and line number for runtime debugging.
**
** Parameters:	None
** Returns:	0
*/

i4
gen_IIdebug()
{

    if (eq->eq_flags & EQ_IIDEBUG)
    {
	char	buf[128];

# if defined(DG_OR_UNIX) || defined(hp9_mpe)

    	STprintf( buf, ERx("IIsd('%s%s%s'),%d"),
	    eq->eq_filename, (eq->eq_infile == stdin ? ERx("") : ERx(".")),
	    eq->eq_ext, eq->eq_line );
	gen__obj( TRUE, buf );
    } else
# if defined(i64_hpu) || (defined(LNX) && defined(LP64))
	gen__obj( TRUE, ERx("int8(0), 0") );
# elif defined(UNIX) && defined(conf_BUILD_ARCH_32_64)
	{
	if (eq->eq_flags & EQ_FOR_DEF64)    
		gen__obj( TRUE, ERx("0_8, 0"));
	else
		gen__obj( TRUE, ERx("0, 0") );
	}
# else
	gen__obj( TRUE, ERx("0, 0") );
# endif
    return 0;

# else

    	STprintf( buf, ERx("%%val(IIsd('%s%s%s')),%%val(%d)"),
	    eq->eq_filename, (eq->eq_infile == stdin ? ERx("") : ERx(".")),
	    eq->eq_ext, eq->eq_line );
	gen__obj( TRUE, buf );
    } else
	gen__obj( TRUE, ERx(",") );
    return 0;

# endif /* DG_OR_UNIX */

}

/*{
+*  Name:  gen_null - Generate null indicator arg to IIput/IIget routines
**
**  Description:
**	If null indicator present, pass by reference.
**	Otherwise, pass a null pointer.
**	Unix:  Since there is no way to pass a null pointer, we pass
**	the value 999 to represent a null pointer.
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
**	byte integers (or NULL).  However, since F77 passes everything by
**	reference, the layer can't distinguish between a null pointer
**	by reference and a zero-valued null indicator by reference.  
**	The solution is that for F77 only we pass an extra 'isvar' variable.  
**	For example:
**	Using null indicator:
**	VMS:	IIputdomio( ind, %val(0), %val(30), %val(4), %val(data) );
**	F77	IIputdomio( 1, ind, 30, 4, data );
**	No null indicator:
**	VMS	IIputdomio( %val(0), %val(0), %val(30), %val(4), %val(data) );
**	F77	IIputdomio( 0, 0, 30, 4, data );
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
# if defined(DG_OR_UNIX) || defined(hp9_mpe)
	gen__int( FbyVAL, 0 );			/* isindvar = FALSE */
	gen__obj( TRUE, ERx(",") );
# endif /* DG_OR_UNIX */
	gen__int( FbyVAL, 0 );			/* NULL pointer */
    }
    else
    {
# if defined(DG_OR_UNIX) || defined(hp9_mpe)
	gen__int( FbyVAL, 1 );			/* isindvar = TRUE */
	gen__obj( TRUE, ERx(",") );
# endif /* DG_OR_UNIX */
	gen__name(FbyNAME, arg_name(argnum));	/* By reference */
    }
    gen__obj( TRUE, ERx(",") );
}

/*
** gen_sconst - Generate a string constant.
**
** Parameters: 	String
** Returns: 	None
**
** Call gen__sconst() that will dump the string in pieces that fit on a line
** and continue each line with the a simple newline.
*/

void
gen_sconst( str )
char	*str;
{
    char	*ss = str;

    if (ss == (char *)0)
	ss = ERx("NULL");
    else if (*ss == '\0')
	ss = ERx(" ");

    gen__obj(TRUE, ERx("'") );			/* start the string */

    /* write lines of the string */
    while (gen__sconst(&ss))
	out_emit( CONT_STR );

    /* if at the end of this line, start another continuation line */
    if ( STR_FUDGE == 0 && out_cur->o_charcnt >= G_LINELEN )
	out_emit( CONT_STATE );

    /* close the string */
    out_emit( ERx("'") );
}

/* 
** gen__sconst - Local utility for writing out the contents of strings.
**
** Parameters:	Address of the string pointer to write.
** Returns:	0 - No more data to dump,
**		1 - Call me again.
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

    len = G_LINELEN - out_cur->o_charcnt - STR_FUDGE;
    if ((i4)STlength(*str) <= len)
    {
	out_emit( *str );
	return 0;
    }
    for (cp = *str; len > 0 && *cp;)
    {
	/* Try not to break in the middle of compiler escape sequences */
	if (*cp == '\'')
	{
	    cp++; 
	    len--;				/* Skip the escaped ' */
	    /* Watch out for loop control variable 'len' */
	    if (len <= 0)			
		break;				
	}
	cp++; len--;				/* Skip current character */
    }
    if (*cp == '\0')				/* All the way till the end */
    {
	out_emit( *str );
	return 0;
    }
    savech = *cp;
    *cp = '\0';
    out_emit( *str );
    *cp = savech;
    *str = cp;					/* More to go next time */
    return 1;
}

/*
+* Procedure:	gen__name
** Purpose:	Spit out a name.
** Parameters:
**	how	- i4		- FbyNAME, FbyVAL, or FbyREF
**	s	- char *	- The name to print
** Return Values:
-*	None.
** Notes:
**	None.
**
** Imports modified:
**	None.
*/

void
gen__name( how, s )
    i4		how;
    char	*s;
{
    char	buf[SC_WORDMAX+10];

# ifdef DGC_AOS 
    how = FbyNAME;
# endif /* DGC_AOS */
# if defined(UNIX) || defined(hp9_mpe)
    how = FbyREF;		/* F77 arguments always go by reference */
# endif /* UNIX */

    switch (how)
    {
      case FbyVAL:
	STprintf( buf, ERx("%%val(%s)"), s );
	gen__obj( TRUE, buf );
	break;
      case FbyNAME:
      case FbyREF:
	gen__obj( TRUE, s );
	break;
    }
}

/*
** gen__int - Does local integer conversion.
**
** Parameters: 	Flag (by value or by reference), Integer to put out.
** Returns: 	None
*/

void
gen__int( how, num )
i4	how;
i4	num;
{
    char	buf[20];

# ifdef DGC_AOS 
    how = FbyNAME;
# endif /* DGC_AOS */
# if defined(UNIX) || defined(hp9_mpe)
    how = FbyREF;		/* F77 arguments always go by reference */
# endif /* UNIX */
    switch (how)
    {
      case FbyVAL:
	STprintf( buf, ERx("%%val(%d)"), num );
	break;
      case FbyREF:
      case FbyNAME:
	CVna( num, buf );
	break;
    }
    gen__obj( TRUE, buf );
}


/*
** gen__float - Does local floating conversion.
**
** Parameters: 	Flag (by value or by reference), Floating to put out.
** Returns: 	None
*/

void
gen__float( how, num )
i4	how;
f8	num;
{
    char	buf[50];

# ifdef DGC_AOS 
    how = FbyNAME;
# endif /* DGC_AOS */
# if defined(UNIX) || defined(hp9_mpe)
    how = FbyREF;
# endif /* UNIX */

    switch (how)
    {
      case FbyVAL:
	STprintf( buf, ERx("%%val(%f)"), num, '.' );
	break;
      case FbyREF:
      case FbyNAME:
	STprintf( buf, ERx("%f"), num, '.' );		/* Standard notation */
	break;
    }
    gen__obj( TRUE, buf );
}

/*
** gen__slen - generate string length
**
** Parameters: 	Flag indicating whether data is constant or variable
**		Pointer to data string
** Returns: 	None
**
** History:
**      22-Sep-2003 (fanra01)
**          Bug 110959
**          Add value parameter of string length for Windows.
*/

void
gen__slen( var, data )
i4	 var;
char	*data;
{
    char	buf[20];
    i4		len;

    if ( var )
    {
# ifdef NT_GENERIC
    /*
    ** Windows specific generation of string length at runtime
    */
	gen__obj( TRUE, ERx("%val(") );
# endif /* NT_GENERIC */
	gen__obj( TRUE, ERx("IIslen(") );
	gen__obj( TRUE, data );
	gen__obj( TRUE, ERx(")") );
# ifdef NT_GENERIC
	gen__obj( TRUE, ERx(")") );
# endif /* NT_GENERIC */
    }
    else
    {
# ifdef DGC_AOS
	gen__obj( TRUE, ERx("IIslen(") );
	gen_sconst( data );
	gen__obj( TRUE, ERx(")") );
# else
# ifdef NT_GENERIC
    /*
    ** Windows specific generation of string length at runtime
    */
	gen__obj( TRUE, ERx("%val(") );
	gen__obj( TRUE, ERx("IIslen(") );
	gen_sconst( data );
	gen__obj( TRUE, ERx(")") );
	gen__obj( TRUE, ERx(")") );
# else  /* NT_GENERIC */
	gen__int( FbyREF, 0 );
# endif /* NT_GENERIC */
# endif /* DGC_AOS */
    }
}
/*
** gen__sadr - generate call to string address  function
**
** Parameters: 	Flag indicating whether data is constant or variable
**		Pointer to data string
** Returns: 	None
*/

void
gen__sadr( var, data )
i4	 var;
char	*data;
{

    if (var)
    {
	gen__obj( TRUE, ERx("IIsadr(") );
	gen__obj( TRUE, data );
    }
    else
    {
	gen__obj( TRUE, ERx("IIsdno(") );
	gen_sconst( data );
    }
    gen__obj( TRUE, ERx(")") );
}


/*
** gen__obj - Write an object within the line length. This utility is used for
**	     everything but string constants.
**
** Parameters:	Indentation flag and Object
** Returns:	None
*/

void
gen__obj( ind, obj )
i4		ind;
register char	*obj;
{
    if ((i4)STlength(obj) > G_LINELEN - out_cur->o_charcnt - 1 )
    {
	if (out_cur->o_charcnt != 0)
	    out_emit( CONT_STATE );	/* continuation line start */
	else
	    out_emit( ERx("\n") );
	if (ind)
	    out_emit( g_indent );
    }

    /* May have been a previous newline */
    if (out_cur->o_charcnt == 0 && ind)
    {
# ifndef STRICT_F77_STD
	out_emit( ERx("\t") );
	out_emit( g_indent );
# else
	out_emit( ERx("      ") );
# endif /* STRICT_F77_STD */
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
}


/*
** gen_do_indent - Update indentation value.
**
** Parameters:	Update value.
** Returns:	None
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
# ifndef STRICT_F77_STD

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

# endif /* STRICT_F77_STD */
}

/*
** gen_host - Generate pure host code from the L grammar (probably declaration
**	      stuff).
**
** Parameters:	Flag - what is to be generated.
** Returns:	None
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
	if (was_key && out_cur->o_charcnt != 0)
	    gen__obj(TRUE, ERx(" ") );
	gen__obj( TRUE, s );
	was_key = 1;
	break;
      case G_H_OP:
	gen__obj( TRUE, s );
	was_key = 0;
	break;
      case G_H_SCONST:
	if (was_key)
	    gen__obj(TRUE, ERx(" ") );
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
