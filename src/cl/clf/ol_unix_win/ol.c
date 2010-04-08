# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<ol.h>
# include	<st.h>
# include	<er.h>
# include	<me.h>

/*
**Copyright (c) 2004 Ingres Corporation
**
** ol.c -- The "C" source of 'OLpcall()'.
**
**	Parameters:
**		func -- (input only) The address of the procedure to be called
**			with the passed parameters.
**
**		pv -- (input only) Address of an array of parameter structures
**			describing the parameters to be passed.
**
**		pc -- (input only) The number of parameters to be passed.
**
**		lang -- (input only) The language in which the procedure was
**			written.
**
**		rettype -- (input only) The return type of the procedure.
**
**		retvalue -- (input/output) The place to put the return
**			  value;
**
**	WARNING:
**		VMS portions of this file are not supported!
**		Languages other than C and FORTRAN are untested.
**
** History:
**	2/25/88 (daveb) --	all negative pv->OL_type arguments
**				should be passed by reference.
**	2/27/88 (daveb) --	As discussed with Joe Cortopassi, strings
**				by ref get handled the same as strings
**				by value; add some error checking, pass
**				official string length instead of figuring
**				it out.  (This is better for fortran).
**	25-may-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Try to structure this for correct passing of fortran strings,
**		and make work for sequent - others are untested;
**		Define LPI_FLEN_BUNCHED_AT_END for LPI FORTRAN on sco_us5 - it
**		is the same as FLEN_BUNCHED_AT_END, except that there are
**		dummy lengths for non-string arguments;
**		Added special case for hp8_us5;
**		Change gld_u42 to BYTE_ALIGN.
**	05-05-90 (bchin)
**		apu_us5, the apollo DN10000 compiler requires any float
**		arguments to be passed in floating point registers.  So
**		I added a dummy function call to do just that.
**	17-dec-90 (jab)
**		Created the w/4gl code line for this, unfortunately, to keep
**		from touching the cl/clf/ol code line in the 6204p project.
**		Added change for HP 9000/300 to not even TRY to invoke f77
**		functions that return 'character' data type.
**	18-dec-90 (jab)
**		(OOps, slight typo on previous hack. Made it f77-specific!)
**	04-jan-91 (jab)
**		Folded into the 6.3 code line. There are no real changes
**		added to the 6.3 line that weren't part of the 6.2 line,
**		and only the apu_us5 stuff didn't appear in the 6.3 line.
**		[Until now, that is. Added hp3_us5 f77 hacks for FORTRAN
**		returning strings.]
**	23-jul-91 (vijay)
**		integrated from 6.3 port: Added dummy function call to help
**		set up floating registers for ris_us5.
**	07-jan-1992 (bonobo)
**		Added the following changes: 13-Aug-91 (dchan), 
**		03-oct-91 (emerson), 04-oct-91 (emerson) to the Unix 6.5 cl.
**      13-Aug-91 (dchan)
**              Added the ds3_ulx f77 hacks for FORTRAN returning strings
**              cf. jab's change of 04-jan-91.
**      03-oct-91 (emerson)
**		Added logic to pass a double by value (case FVAL) as a pair
**		of integers if xCL_085_PASS_DOUBLES_UNALIGNED is defined.
**		(Fix for bug 40107).
**      04-oct-91 (emerson)
**		Added #include <clconfig.h>, as I should have done in previous
**		fix (for bug 40107).
**      11-nov-91 (dchan)
**         	Added ds3_ulx to list of machines that gets FLEN_BUNCHED_AFTER
**          	defined.
**  	26-jan-92 (johnr)
**  		Piggyback ds3_ulx for ds3_osf
**  	16-mar-92 (jab)
**  		The Sun doesn't really do FORTRAN functions that return strings,
**  		either(!). On the W4GL05 version of this file, there was support
**  		for it, but it didn't ever seem to get into the 63p CL.
**  	29-apr-92 (jab)
**  		For the FORTRAN arguments, the length is given as extra args at
**  		the end of the list, not extra args immediately follow the char
**  		buffer. Changed to reflect that for the HP8.
**  	11-aug-92 (jab)
**  		Changing FORTRAN strings for the RS/6000 also.
**      21-aug-92 (puru)
**              Added amd_us5 to the list, so that FORTRAN functions return
**              strings properly
**      31-aug-92 (thall)
**              Added SEQUENT to the LPI_FLEN_BUNCHED_AT_END group and
**              sqs_ptx to the FSTRFAKE group
**  	19-oct-92 (gmcquary)
**   		Add pym_us5 to FSTR_RET flag train.
**      16-oct-92 (rkumar)
**              remove FLEN_FIRST define for sequent , and use sqs_ptx
**              define  instead of SEQUENT.
**  	30-Dec-1992 (fredv)
**  		Added NO_OPTIM for RS6000 to fix bug b48639.
**  		Optimization cause the floating registers set up wrongly.
**  	07-Jan-1993 (fredv)
**		We didn't support passing floating number by value from
**		4GL procedure to 3GL procedure for ds3_ulx in the previous
**  		releases. We used to document this fact in the release notes.
**	   	However, the info was missing the 6.4 release notes, and thus,
**  		a bug was logged on ds3_uls that passing floating number
**  		by value from 4GL to 3GL failed.
**  		Since the DEC mips compiler convention is very similar
**  		to the ris_us5 compiler, we can piggy-back most of the ris_us5
**  		code to fix the bug and add this feature back to ds3_ulx!
**      02-Feb-93 (Farzin Barazandeh)
**              - Define FLEN_BUNCHED_AT_END for Green Hill Fortran for  dg8_us5.
**              - Added dg8_us5 to list of machines that can't return strings
**                for fortran.
**              - Picked up fix from jefff to fix it so that fortran string
**                functions won't core dump.
**	16-feb-93 (davel)
**		several 6.5 changes:
**		1. Cast references to OL_PV's OL_value, which is not type PTR.
**		2. Modifications now that string arguments are passed
**		   as OL_VSTR structures
**		3. Add support for decimal arguments (OL_DEC).
**	21-jun-93 (davel)
**		Fix bug 50599 - for string returns, copy the string into the
**		buffer supplied by the caller.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	7/93 (Mike S) 
**		Add OL_PTR cases for passing and returning structure pointers.
**		For the moment, treat passing strucutre pointers like OL_I4,
**		out of ignorance concerning
**		what to do where sizeof(i4) != sizeof(OL_STRUCT_PTR)
**		(analogous changes to ingres63p change 268539)
**	17-sep-93 (dwilson)
**		Added su4-u42 to the list of platforms that 
**		 define        FSTR_RET                FSTRFAKE
**		so fortran 3GL string passing will work.
**      07-sep-93 (smc)
**              Made ap more portable SCALARP type.
**	27-sep-93 (dwilson)
**		Integrating functionality in from the ingres63p library,
**		which would include the following history from the 
**		ingres63p version of this file:
**	27-sep-93 (Mike S)
**		Don't crash if called routine returns a NULL string pointer;
**		substitute an empty string.
**	4-jan-94 (donc) Bug 55867
**		Blank pad character strings in fortran up to OL_size as opposed
**		to NULL fill. Fortran internal writes to a string do not see
**		nulls embedded in a string and as such return to 4GL callers
**		a string that appears to be truncated.
**	22-Jan-1994 (fredv)
**		Added su4_us5 for fortran string fix.
**	19-nov-93 (swm)
**		Bug #58647
**		On axp_osf the first 6 parameters are passed via registers.
**		But floats and doubles are passed via different registers
**		to other parameters. So called a fake float function before
**		the user function so that the C compiler sets up the float
**  	19-oct-92 (gmcquary)
**		registers as required. Unlike other platforms, argument
**		register usage is not optimised.
**	10-feb-1994 (ajc)
**		Added hp8_bls specific entries based on hp8
**      10-feb-1995 (chech02)
**              Added rs4_us5 for AIX 4.1.
**	22-jun-95 (allmi01)
**		Added dgi_us5 support for DG-UX on Intel based platforms following dg8_us5.
**	14-jul-95 (morayf)
**		Added SCO sos_us5 to the LPI_FLEN_BUNCHED_AT_END group.
**      10-nov-1995 (murf)
**              Added sui_us5 to all areas specifically defined with su4_us5.
**              Reason, to port Solaris for Intel.
**	13-dec-95 (morayf)
**		Added SNI RMx00 (rmx_us5) to those using 'fake' arguments
**		for strings ie. to FSTRFAKE group.
**      25-jul-96 (prida01) Re-cast types for i1 and i2 otherwise we can
**              have problems passing parameters.
**	27-may-97 (mcgem01)
**		Clean up compiler warnings.
**	29-jul-1997 (walro03)
**		Reapply changes from main made by mosjo01:
**		Based on the wisdom of Simon Connor in UK L2, I am applying
**		ingres63p change 271656 for rmx_us5, pym_us5, and ner_us5 (the
**		latter was 271656).  Apparently, some MIPS boxes pass the first
**		two floating-point parameters in floating-point registers,
**		rather than on the stack.  This change defines a dummy routine,
**		load_float_regs(), to force f-p parameters into registers.
**	29-jul-1997 (walro03)
**		Add Tandem NonStop (ts2_us5) to the above changes.
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	10-may-1999 (walro03)
**		Remove obsolete version string amd_us5, apu_us5, bu2_us5,
**		ds3_osf, odt_us5.
**      03-jul-99 (podni01)
**              Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**      27-Jul-1999 (hweho01)
**              1) Added supports for AIX 64-bit (ris_u64).
**              2) Turned off optimization to resolve run-time error. 
**                 Symptom: front end sep test failed (fe/vis24.sep). 
**                 The compiler is AIX C compiler version 3.6.0.
**	23-ju7l-2001 (stephenb)
**	    Add support for i64_aix
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**	4-Jun-2003 (bonro01)
**	    Change required for floating point parameters for i64_hpu.
**	    Itanium is similiar to MIPS platforms.  It needs to call a
**	    dummy routine to force the load of floats into registers.
**	    On other platforms, only the first 2 or 6 parms were passed
**	    as registers, however on Itanium my test program indicated
**	    that at least the first 130 parms must be passed to the
**	    dummy routine in order to pass floats correctly.
**	    this is probably related to the fact that Itanium has
**	    128 float and 128 general registers that can be used for
**	    parameters.  This routine only supports a maximum of 40
**	    parms on a function call.
**	10-Jul-2003 (bonro01)
**	    Change required because of compiler upgrade.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**	18-Jan-2005 (kodse01)
**	    Port for i64_lnx.
**      15-Mar-2006 (hweho01)
**          For AIX, modified the alignment handling of f8 (double )    
**          number that is passed by value.  
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**	12-Aug-2009 (hweho01) Bug 122460
**	    For a64_lnx platform, to avoid the failure of passing float   
**	    data by value, force to load the parameters into registers.   
**	    According to x86-64 calling convention on Linux, the first   
**	    6 parameters in the procedure are passed via registers, also   
**	    up to 8 floating parms are placed in the float registers   
**	    (xmm0-7); so total 14 registers possibly are used in 
**	    parameter passing.   
*/
/*
NO_OPTIM=ris_us5 rs4_us5 ris_u64
*/

/*
**	mecchar struct defines how each language passes ints, floats,
**	and strings, and how each language returns strings.  They
**	methods are defined as follows:
**		NONE		not supported
**		REF		pass the address of the object
**		IVAL		pass a 4 byte integer
**		FVAL		pass an 8 byte float
**		SVAL		pass pointer to string
**		SDESC		pass descriptor (pointer to string, length)
**		VDESC		pass ???
**		VSTR		return varying string with no descriptor
**		FSTRFAKE	to return a string, pass "fake arguments" as the
**				first arguments, which are really the string
**				address (&buf[0]) and max length of string.
*/

# define	NONE		0
# define	REF		1
# define	IVAL		2
# define	FVAL		3
# define	SVAL		4
# define	SDESC		5
# define	VDESC		6
# define	VSTR		7
# define	FSTRFAKE	8

#if defined(hp3_us5) || defined(ds3_ulx) || \
    defined(bu3_us5) || defined(su4_u42) || \
    defined(any_hpux) || defined(any_aix) || \
    defined(sqs_ptx) || defined(pym_us5) || defined(sparc_sol) || \
    defined(hp8_bls) || defined(sui_us5) || \
    defined(rmx_us5) || defined(ner_us5) || defined(ts2_us5)
# define	FSTR_RET		FSTRFAKE
#endif

#ifndef	FSTR_RET
# define	FSTR_RET	SDESC
#endif

#ifndef	FSTR_PASS
# define	FSTR_PASS	SDESC
#endif

static struct {
	i4	itype;	/* how they pass ints */
	i4	ftype;	/* how they pass floats */
	i4	stype;	/* how they pass strings */
	i4	srtype;	/* how they return strings */
} mecchar[] = {
	IVAL,	FVAL,	SVAL,	SVAL,	/* OLC		0 */
	REF,	REF,	FSTR_PASS,	FSTR_RET,	/* OLFORTRAN	1 */
	REF,	REF,	VDESC,	VSTR,	/* OLPASCAL	2 */
	REF,	REF,	SDESC,	SDESC,	/* OLBASIC	3 */
	REF,	REF,	SVAL,	NONE, 	/* OLCOBOL	4 */
	REF,	REF,	SDESC,	VDESC,	/* OLPL1	5 */
	NONE,	NONE,	NONE,	NONE,	/* OLADA	6 */
	NONE,	NONE,	NONE,	NONE	/* OLOSL	7 */
};

/*
** FIXME (please!)
**
** How various machines want to pass strings to fortran.  This is very
** compiler dependant, and you need to check this with the stuff in the
** e[s]qlf preprocessor and the runsys routines. 
**
** This _really_ wants to be consolidated in one system dependant header
** file visible to mainline and to the CL.
**
** Note that we probably aren't handling cases where the length needs to be 
** passed as a short correctly.  Apollo fortran used to be this way.
**
** It's also very questionable if we are handling string returns from
** called fortran routines correctly. (daveb).
*/

# if defined (any_hpux) || defined (dg8_us5) || defined(hp8_bls) || defined(dgi_us5)
	/* XXX Not sure if this is right XXX */
# define FLEN_BUNCHED_AT_END
#	define gotit
# endif

# if defined(sco_us5) || defined(ds3_ulx) || \
     defined(sqs_ptx) || defined(sos_us5)
#	define LPI_FLEN_BUNCHED_AT_END
#	define gotit
# endif

/* default case, XXX not sure if this is XXX right for generic f77 */
# ifndef gotit
#	define FLEN_BUNCHED_AT_END
#	define gotit
# endif

/*
** If you change this, then much below needs to be altered.
*/
# define	MAX_ARGS	40

# if defined(any_aix) || defined(ds3_ulx)
void load_float_reg()
{
}
# endif /* aix */

# if defined(i64_hpu) || defined(i64_lnx)
void load_float_regs2(f8 float_arg0, f8 float_arg1)
{
}
void load_float_regs4(f8 float_arg0, f8 float_arg1, f8 float_arg2, f8 float_arg3)
{
}
void load_float_regs40(f8 float_arg0, f8 float_arg1, f8 float_arg2, f8 float_arg3, f8 float_arg4,
	f8 float_arg5, f8 float_arg6, f8 float_arg7, f8 float_arg8, f8 float_arg9,
	f8 float_arg10, f8 float_arg11, f8 float_arg12, f8 float_arg13, f8 float_arg14,
	f8 float_arg15, f8 float_arg16, f8 float_arg17, f8 float_arg18, f8 float_arg19,
	f8 float_arg20, f8 float_arg21, f8 float_arg22, f8 float_arg23, f8 float_arg24,
	f8 float_arg25, f8 float_arg26, f8 float_arg27, f8 float_arg28, f8 float_arg29,
	f8 float_arg30, f8 float_arg31, f8 float_arg32, f8 float_arg33, f8 float_arg34,
	f8 float_arg35, f8 float_arg36, f8 float_arg37, f8 float_arg38, f8 float_arg39)
{
}
# endif /* i64_hpu || i64_lnx */

# if defined(a64_lnx)
void load_float_regs2(f8 float_arg0, f8 float_arg1)
{
}
void load_float_regs4(f8 float_arg0, f8 float_arg1, f8 float_arg2, f8 float_arg3)
{
}
void load_float_regs14(f8 float_arg0, f8 float_arg1, f8 float_arg2, f8 float_arg3, 
	f8 float_arg4, f8 float_arg5, f8 float_arg6, f8 float_arg7, f8 float_arg8, 
	f8 float_arg9, f8 float_arg10, f8 float_arg11, f8 float_arg12, f8 float_arg13)
{
}
# endif /* a64_lnx */

i4
OLpcall (func, pv, pc, lang, rettype, retvalue)
void		(*func)();
register OL_PV	*pv;
register i4	pc;
i4		lang;
i4		rettype;
OL_RET		*retvalue;
{
	SCALARP			av[MAX_ARGS];
	register SCALARP	*ap = av;
	register i4		type;
	register i4		t;
	OL_RET			real_ret;
# if defined(any_aix)
# define RIS_FREGS		20   /* actually 32 registers, but we have
				      * a maximum of 40 int parms or 20 doubles.
				      */
	f8			ris_flt_arg[RIS_FREGS];
	register i4		float_cnt = 0;
# endif /* aix */
# ifdef ds3_ulx
# define RIS_FREGS		2   /* actually more than 2 floating registers,
				     * but at most two floating parameters
				     * will be passed thru registers.
				     */
	f8			ris_flt_arg[RIS_FREGS];
	register i4		float_cnt = 0;
# endif /* ds3_ulx */

# if defined(ner_us5) || defined(pym_us5) || defined(rmx_us5) \
  || defined(ts2_us5) || defined(rux_us5)
/*
** MAX_FLOAT_REG_ARGS is the maximum number of floating-point
** arguments that will be passed in floating-point registers.
**
** On the MIPS R3000, typically the first two floating-point
** parameters are loaded into $f12-$f13 and $f14-$f15.
*/
# define MAX_FLOAT_REG_ARGS	2
	f8			float_args[MAX_FLOAT_REG_ARGS];
	register i4		float_cnt = 0;
# endif /* ner_us5 pym_us5 rmx_us5 ts2_us5 */

# ifdef axp_osf
# define FREGS	6		/* floating point argument registers */
	f8			float_arg[FREGS];
	i4			float_index;
	i4			floats_passed;
# endif /* axp_osf */

# if defined(i64_hpu) || defined (i64_lnx)
# define FREGS	40		/* floating point argument registers */
	f8			float_arg[FREGS];
	i4			float_index;
	i4			floats_passed;
# endif /* i64_hpu || i64_lnx */

# ifdef FLEN_BUNCHED_AT_END
	i2			lenv[MAX_ARGS];
	register i2		*lp = lenv;
	register i4		i;
# endif

# ifdef LPI_FLEN_BUNCHED_AT_END
	i4			lenv[MAX_ARGS];
	register i4		*lp = lenv;
	register i4		i;
# endif

# ifdef FLEN_STRUCT
	FLEN_STRUCT		fstrs[MAX_ARGS];
	FLEN_STRUCT		*sp = fstrs;
# endif

# if defined(a64_lnx)
# define FREGS	14            	
	f8			float_arg[FREGS];
	i4			float_index;
	i4			floats_passed;
# endif /* a64_lnx */

# if defined(axp_osf) || defined(i64_hpu) || defined (i64_lnx) || defined(a64_lnx)
	/*
	** initialise float_args array to zeros to avoid exceptions
	*/
	for (float_index=0; float_index<FREGS; float_index++)
		float_arg[float_index] = 0.0;
	float_index = 0;
	floats_passed = 0;
# endif /* axp_osf || i64_hpu || i64_lnx */

	if( retvalue == NULL )
	    retvalue = &real_ret;

/*
** Some machines can't return strings from fortran.  This code prev
ents
** a core dump if the user attempts this in his application.
*/

#if defined(dg8_us5) || defined(dgi_us5)
        if (abs(rettype) == OL_STR && lang == OLFORTRAN)
        {
                return (FAIL);
        }
#endif

/*
** Some machines, like the HP 9000/300, expect that we [somehow]
** pass a "save area" as an "invisible" argument as a place to
** stash string return values. (Don't laugh. Some machines do this
** for returning structures, too. At least it keeps the code reentrant.)
**
** Note that the area must be provided by the caller, as is specified as an
** input parameter in retvalue->OL_string.  This must point to an area sized
** at OL_MAX_RETLEN.  (Note also that this works for any language where 
** the string return type is FSTRFAKE - not just FORTRAN!)
**
** This means that the f77 statement:
** 	character*24 retval, routinename
** 	...
** 	retval = routinename(arg1,arg2,arg3,arg4)
** is invoked as if you'd said, in C:
** 	char retval[24];
** 	void routinename();
** 	...
** 	routinename(retval, sizeof(retval),&arg1,&arg2,&arg3,&arg4);
** We tell it the string is a little smaller "just in case" it wants to
** put a '\0' on the end.
**
*/
	if (abs(rettype) == OL_STR && mecchar[ lang ].srtype == FSTRFAKE)
	{
		*ap++ = (SCALARP)retvalue->OL_string;
		*ap++ = OL_MAX_RETLEN-1;
	}

	for (; pc--; ++pv)
	{
		if( ap >= &av[ MAX_ARGS ] )
		    return( FAIL );

		/* Figure out how the argument should be put into av. */

		t = pv->OL_type;
		switch( abs( t ) )
		{
		case OL_PTR:
		case OL_I4:	type = t < 0 ? REF : mecchar[ lang ].itype;
				break;

		case OL_F8:	type = t < 0 ? REF : mecchar[ lang ].ftype;
				break;

		/* OL_STR & default lumped together for historical reasons.
		** [ An old if-elseif-else defaulted to string, and some
		** versions of calling code turned out to rely on that
		** behaviour. (daveb) ]
		*/
		case OL_STR:
		default:
				/*
				** Handling value/reference strings is up
				** to the caller.  If by value, the caller
				** should make a copy of it, giving us an
				** the copy's address. If by reference, we
				** just get the address of the string.
				**
				** So, we handle OL_STR and -OL_STR the
				** same (daveb).
				*/
				type = mecchar[ lang ].stype;
				break;
		}

		/* Stuff the argument into the array as appropriate
		** for the arg passing convention.
		*/
		switch( type )
		{
		case SVAL:
			*ap++ = (SCALARP)(&((OL_VSTR *)pv->OL_value)->string[0] );
# ifdef LPI_FLEN_BUNCHED_AT_END
			*lp++ = 0;
# endif
			break;
		case REF:
			*ap++ = (SCALARP)pv->OL_value;
# ifdef LPI_FLEN_BUNCHED_AT_END
			*lp++ = 0;
# endif
			break;
		case IVAL:
			/* check for size of the integer and cast
			** Only cast to max i4 for now
			*/
			switch (pv->OL_size)
			{
			case sizeof(i1):
				*ap++ = *(i1 *)pv->OL_value;
				break;
			case sizeof(i2):
				*ap++ = *(i2 *)pv->OL_value;
				break;
			case sizeof(i4):
				*ap++ = *(i4 *)pv->OL_value;
				break;
			default :
				*ap++ = *(i4 *)pv->OL_value;
			}
# ifdef LPI_FLEN_BUNCHED_AT_END
			*lp++ = 0;
# endif
			break;
		case FVAL:

# ifdef xCL_085_PASS_DOUBLES_UNALIGNED
			/*
			** Certain platforms (e.g. SPARC) require that
			** doubles normally be aligned on 8-byte boundaries,
			** but pass them (by value) as pairs of integers
			** (aligned on 4-byte boundaries).
			*/
			ap[0] = ((i4 *)pv->OL_value)[0];
			ap[1] = ((i4 *)pv->OL_value)[1];
# else
			/* FIXME alignment should be done for 
			   more than just the gould (daveb) */

# if defined(BYTE_ALIGN) && !defined(any_aix)
			ap = (SCALARP *) ME_ALIGN_MACRO(ap, sizeof(ALIGN_RESTRICT));
# endif

# if (defined (any_hpux) || defined(hp8_bls)) && \
     !defined(i64_hpu)
	 		/* reverse f8's for right to left */
			ap[0] = ((i4 *)pv->OL_value)[1];
			ap[1] = ((i4 *)pv->OL_value)[0];
# else
			*(f8 *)ap = *(f8 *)pv->OL_value;
# endif


# if defined(ner_us5) || defined(pym_us5) || defined(rmx_us5) \
  || defined(ts2_us5) || defined(rux_us5)
			if (float_cnt < MAX_FLOAT_REG_ARGS)
				float_args[float_cnt++] = *(f8 *)pv->OL_value;
# endif /* ner_us5 pym_us5 rmx_us5 ts2_us5 */

# if defined(any_aix)
			if (float_cnt < RIS_FREGS)
				ris_flt_arg[float_cnt++] = *(f8 *)pv->OL_value;
# endif /* aix */

# if defined(axp_osf) || defined(i64_hpu) || defined(i64_lnx) || defined(a64_lnx)
			/*
			** float_index is incremented for all argument
			** types, not just floats, because argument
			** register use is not optimised
			*/
			if (float_index < FREGS)
			{
				floats_passed = 1;
				float_arg[float_index] = *(f8 *)pv->OL_value;
			}
# endif /* axp_osf || i64_hpu || i64_lnx || a64_lnx */

# endif /* xCL_085_PASS_DOUBLES_UNALIGNED */

			ap += sizeof( f8 ) / sizeof( *ap );
# ifdef LPI_FLEN_BUNCHED_AT_END
			*lp++ = 0;
# endif
			break;

		case SDESC:
			{
				char *str_val = 
					&((OL_VSTR *)pv->OL_value)->string[0];
				i2 *slen = &((OL_VSTR *)pv->OL_value)->slen;

				if ( lang == OLFORTRAN )
				    MEfill((pv->OL_size - *slen + 1), ' ',
				    	str_val + *slen);

				if ( lang == OLFORTRAN )
				    MEfill((pv->OL_size - *slen + 1), ' ',
				    	str_val + *slen);

				/* These were formerly taking the STlength of
				** OL_value instead of the OL_size.  Joe 
				** Cortopassi informs me this is wrong if it's 
				** a by-ref string the called procedure may 
				** extend (daveb).
				*/
# if defined FLEN_FIRST
				/* size, then str addr */
				*ap++ = pv->OL_size;
				*ap++ = (i4)str_val;
# endif
# if defined FLEN_AFTER
				/* str addr, then size */
				*ap++ = (i4)str_val;
				*ap++ = pv->OL_size;
# endif
# ifdef FLEN_STRUCT
				/* fill in a struct, and pass its addr */
				sp->s = str_val;
				sp->len = pv->OL_size;
				*ap++ = sp++;
# endif
# ifdef FLEN_BUNCHED_AT_END
				/* pass str addr, and save len for later */
				*ap++ = (SCALARP)str_val;
				*lp++ = pv->OL_size;
# endif
# ifdef LPI_FLEN_BUNCHED_AT_END
				/* pass str addr, and save len for later */
				*ap++ = (i4)str_val;
				*lp++ = pv->OL_size;
# endif
			}
			break;
		}
# if defined(axp_osf) || defined(i64_hpu) || defined(i64_lnx) || defined(a64_lnx)
		/*
		** float_index is incremented for all argument
		** types, not just floats, because argument
		** register use is not optimised
		*/
		float_index++;
# endif /* axp_osf || i64_hpu || i64_lnx || a64_lnx */
	}

# ifdef FLEN_BUNCHED_AT_END
	/* Append the saved string lengths */
	if ( lang == OLFORTRAN )
		for ( i = 0; i < lp - lenv; i++)
			*ap++ = lenv[i];
# endif

# ifdef LPI_FLEN_BUNCHED_AT_END
	/* Append the saved string lengths */
	if ( lang == OLFORTRAN )
		for ( i = 0; i < lp - lenv; i++)
			*ap++ = &lenv[i];
# endif

# if defined(ner_us5) || defined(pym_us5) || defined(rmx_us5) \
  || defined(ts2_us5) || defined(rux_us5)
	if (float_cnt)
		load_float_regs(float_args[0], float_args[1]);
# endif /* ner_us5 pym_us5 rmx_us5 ts2_us5 */

	switch( rettype )
        {
	case OL_NOTYPE:
	case OL_PTR:
	case OL_I4: switch( ap - av )
		    {
	            case 0:
		    case 1:
		    case 2:
# if defined(any_aix)
			    if (float_cnt)
				    (void) load_float_reg(ris_flt_arg[0]);
# endif
# ifdef axp_osf
			    if (floats_passed)
				load_float_regs(float_arg[0], float_arg[1],
						float_arg[2], float_arg[3],
						float_arg[4], float_arg[5]);
# endif /* axp_osf */
# if defined(i64_hpu) || defined(i64_lnx) || defined(a64_lnx)
			    if (floats_passed)
				load_float_regs2(float_arg[0], float_arg[1]);
# endif /* i64_hpu || i64_lnx || a64_lnx */

			    if (rettype == OL_PTR)
			    {
				retvalue->OL_ptr =
				    (*(OL_STRUCT_PTR (*)())func)(av[0], av[1]);
			    }
			    else
			    {
			    	retvalue->OL_int =
				    (*(i4 (*)())func)(av[0], av[1]); 
			    }
			    break;
	            case 3:
	            case 4:
# if defined(any_aix)
                           /* This doesn't look right.  Should there be
			    * two more float args?  -- dwilson
			    */
                            if (float_cnt)
                                (void) load_float_reg(ris_flt_arg[0],
				  ris_flt_arg[1]);
# endif
# if defined(ds3_ulx)
                            if (float_cnt)
                                (void) load_float_reg(ris_flt_arg[0],
				  ris_flt_arg[1]);
# endif /* ds3_ulx */
# ifdef axp_osf
			    if (floats_passed)
				load_float_regs(float_arg[0], float_arg[1],
						float_arg[2], float_arg[3],
						float_arg[4], float_arg[5]);
# endif /* axp_osf */
# if defined(i64_hpu) || defined(i64_lnx) || defined(a64_lnx)
			    if (floats_passed)
				load_float_regs4(float_arg[0], float_arg[1], float_arg[2], float_arg[3]);
# endif /* i64_hpu || i64_lnx || a64_lnx */
			    if (rettype == OL_PTR)
			    {
				retvalue->OL_ptr =
				  (*(OL_STRUCT_PTR (*)())func)
				      (av[0], av[1], av[2], av[3]);
			    }
			    else
			    {
			        retvalue->OL_int =
				  (*(i4 (*)())func)(av[0], av[1], av[2], av[3]);
			    }
			    break;
	            default:
# if defined(any_aix)
                           if (float_cnt)
                                (void) load_float_reg(ris_flt_arg[0],
                                ris_flt_arg[1], ris_flt_arg[2],ris_flt_arg[3],
				ris_flt_arg[4], ris_flt_arg[5],ris_flt_arg[6],
				ris_flt_arg[7], ris_flt_arg[8], ris_flt_arg[9],
				ris_flt_arg[10],ris_flt_arg[11],ris_flt_arg[12],
				ris_flt_arg[13],ris_flt_arg[14],ris_flt_arg[15],
				ris_flt_arg[16],ris_flt_arg[17],ris_flt_arg[18],
				ris_flt_arg[19]);
# endif /* aix */
# if defined(ds3_ulx)
                            if (float_cnt)
                                (void) load_float_reg(ris_flt_arg[0],
				  ris_flt_arg[1]);
# endif /* ds3_ulx */
# ifdef axp_osf
			    if (floats_passed)
				load_float_regs(float_arg[0], float_arg[1],
						float_arg[2], float_arg[3],
						float_arg[4], float_arg[5]);
# endif /* axp_osf */
# if defined(a64_lnx)
			    if (floats_passed)
				load_float_regs14(float_arg[0], float_arg[1],
						  float_arg[2], float_arg[3],
						  float_arg[4], float_arg[5],
						  float_arg[6], float_arg[7],
						  float_arg[8], float_arg[9], 
						  float_arg[10], float_arg[11],
						  float_arg[12], float_arg[13]);
# endif /* a64_lnx */
# if defined(i64_hpu) || defined(i64_lnx)
			    if (floats_passed)
				load_float_regs40(float_arg[0], float_arg[1], float_arg[2], float_arg[3], float_arg[4],
						float_arg[5], float_arg[6], float_arg[7], float_arg[8], float_arg[9],
						float_arg[10], float_arg[11], float_arg[12], float_arg[13], float_arg[14],
						float_arg[15], float_arg[16], float_arg[17], float_arg[18], float_arg[19],
						float_arg[20], float_arg[21], float_arg[22], float_arg[23], float_arg[24],
						float_arg[25], float_arg[26], float_arg[27], float_arg[28], float_arg[29],
						float_arg[30], float_arg[31], float_arg[32], float_arg[33], float_arg[34],
						float_arg[35], float_arg[36], float_arg[37], float_arg[38], float_arg[39]);
# endif /* i64_hpu || i64_lnx */
			    if (rettype == OL_PTR)
			    {
			      retvalue->OL_ptr =
				  (*(OL_STRUCT_PTR (*)())func)
			      (av[0], av[1], av[2], av[3],
			       av[4], av[5], av[6], av[7], av[8], av[9], av[10],
			       av[11], av[12], av[13], av[14], av[15], av[16],
			       av[17], av[18], av[19], av[20], av[21], av[22],
			       av[23], av[24], av[25], av[26], av[27], av[28],
			       av[29], av[30], av[31], av[32], av[33], av[34],
			       av[35], av[36], av[37], av[38], av[39] );
			    }
			    else
			    {
			      retvalue->OL_int =
				  (*(i4 (*)())func)
			      (av[0], av[1], av[2], av[3],
			       av[4], av[5], av[6], av[7], av[8], av[9], av[10],
			       av[11], av[12], av[13], av[14], av[15], av[16],
			       av[17], av[18], av[19], av[20], av[21], av[22],
			       av[23], av[24], av[25], av[26], av[27], av[28],
			       av[29], av[30], av[31], av[32], av[33], av[34],
			       av[35], av[36], av[37], av[38], av[39] );
			    }
			    break;
	            }
                    break;

	case OL_F8: switch( ap - av )
		    {
	            case 0:
		    case 1:
		    case 2:
# if defined(any_aix)
                            if (float_cnt)
                                (void) load_float_reg(ris_flt_arg[0]);
# endif
# ifdef axp_osf
			    if (floats_passed)
				load_float_regs(float_arg[0], float_arg[1],
						float_arg[2], float_arg[3],
						float_arg[4], float_arg[5]);
# endif /* axp_osf */
# if defined(i64_hpu) || defined(i64_lnx) || defined(a64_lnx)
			    if (floats_passed)
				load_float_regs2(float_arg[0], float_arg[1]);
# endif /* i64_hpu || i64_lnx */
			    retvalue->OL_float =
				(*(f8 (*)())func)(av[0], av[1]); break;
	            case 3:
	            case 4:
# if defined(any_aix)
                            if (float_cnt)
                                (void) load_float_reg(ris_flt_arg[0],
				   ris_flt_arg[1]);
# endif
# ifdef axp_osf
			    if (floats_passed)
				load_float_regs(float_arg[0], float_arg[1],
						float_arg[2], float_arg[3],
						float_arg[4], float_arg[5]);
# endif /* axp_osf */
# if defined(i64_hpu) || defined(i64_lnx) || defined(a64_lnx)
			    if (floats_passed)
				load_float_regs4(float_arg[0], float_arg[1], float_arg[2], float_arg[3]);
# endif /* i64_hpu || i64_lnx */
			    retvalue->OL_float =
				(*(f8 (*)())func)(av[0], av[1], av[2], av[3] );
				break;
	            default:
# if defined(any_aix)
                           if (float_cnt)
                                (void) load_float_reg(ris_flt_arg[0],
                                ris_flt_arg[1], ris_flt_arg[2],ris_flt_arg[3],
				ris_flt_arg[4], ris_flt_arg[5],ris_flt_arg[6],
				ris_flt_arg[7], ris_flt_arg[8], ris_flt_arg[9],
				ris_flt_arg[10],ris_flt_arg[11],ris_flt_arg[12],
				ris_flt_arg[13],ris_flt_arg[14],ris_flt_arg[15],
				ris_flt_arg[16],ris_flt_arg[17],ris_flt_arg[18],
				ris_flt_arg[19]);
# endif /* aix */
# if defined(ds3_ulx)
                            if (float_cnt)
                                (void) load_float_reg(ris_flt_arg[0],
				  ris_flt_arg[1]);
# endif /* ds3_ulx */
# ifdef axp_osf
			    if (floats_passed)
				load_float_regs(float_arg[0], float_arg[1],
						float_arg[2], float_arg[3],
						float_arg[4], float_arg[5]);
# endif /* axp_osf */
# if defined(a64_lnx)
			    if (floats_passed)
				load_float_regs14(float_arg[0], float_arg[1],
						  float_arg[2], float_arg[3],
						  float_arg[4], float_arg[5],
						  float_arg[6], float_arg[7],
						  float_arg[8], float_arg[9], 
						  float_arg[10], float_arg[11],
						  float_arg[12], float_arg[13]);
# endif /* a64_lnx */
# if defined(i64_hpu) || defined(i64_lnx)
			    if (floats_passed)
				load_float_regs40(float_arg[0], float_arg[1], float_arg[2], float_arg[3], float_arg[4],
						float_arg[5], float_arg[6], float_arg[7], float_arg[8], float_arg[9],
						float_arg[10], float_arg[11], float_arg[12], float_arg[13], float_arg[14],
						float_arg[15], float_arg[16], float_arg[17], float_arg[18], float_arg[19],
						float_arg[20], float_arg[21], float_arg[22], float_arg[23], float_arg[24],
						float_arg[25], float_arg[26], float_arg[27], float_arg[28], float_arg[29],
						float_arg[30], float_arg[31], float_arg[32], float_arg[33], float_arg[34],
						float_arg[35], float_arg[36], float_arg[37], float_arg[38], float_arg[39]);
# endif /* i64_hpu || i64_lnx */
			    retvalue->OL_float =
				(*(f8 (*)())func)(av[0], av[1], av[2], av[3],
			    av[4], av[5], av[6], av[7], av[8], av[9], av[10],
			    av[11], av[12], av[13], av[14], av[15], av[16],
			    av[17], av[18], av[19], av[20], av[21], av[22],
			    av[23], av[24], av[25], av[26], av[27], av[28],
			    av[29], av[30], av[31], av[32], av[33], av[34],
			    av[35], av[36], av[37], av[38], av[39] ); break;
	            }

                    break;

	case OL_STR:
			{
				char *ret;
				 switch( ap - av )
			    {
		             case 0:
			     case 1:
			     case 2:
# if defined(any_aix)
			    if (float_cnt)
				    (void) load_float_reg(ris_flt_arg[0]);
# endif
# ifdef axp_osf
			    if (floats_passed)
				load_float_regs(float_arg[0], float_arg[1],
						float_arg[2], float_arg[3],
						float_arg[4], float_arg[5]);
# endif /* axp_osf */
# if defined(i64_hpu) || defined(i64_lnx) || defined(a64_lnx)
			    if (floats_passed)
				load_float_regs2(float_arg[0], float_arg[1]);
# endif /* i64_hpu || i64_lnx */
				    ret = (*(char * (*)())func)(av[0], av[1]); break;
		             case 3:
		             case 4:
# if defined(any_aix)
                            if (float_cnt)
                                (void) load_float_reg(ris_flt_arg[0],
				    ris_flt_arg[1]);
# endif
# ifdef axp_osf
			    if (floats_passed)
				load_float_regs(float_arg[0], float_arg[1],
						float_arg[2], float_arg[3],
						float_arg[4], float_arg[5]);
# endif /* axp_osf */
# if defined(i64_hpu) || defined(i64_lnx) || defined(a64_lnx)
			    if (floats_passed)
				load_float_regs4(float_arg[0], float_arg[1], float_arg[2], float_arg[3]);
# endif /* i64_hpu || i64_lnx */
				    ret = (*(char * (*)())func)(av[0], av[1], av[2],
	                                 av[3] ); break;
		             default:
# if defined(any_aix)
                           if (float_cnt)
                                (void) load_float_reg(ris_flt_arg[0],
                                ris_flt_arg[1], ris_flt_arg[2],ris_flt_arg[3],
				ris_flt_arg[4], ris_flt_arg[5],ris_flt_arg[6],
				ris_flt_arg[7], ris_flt_arg[8], ris_flt_arg[9],
				ris_flt_arg[10],ris_flt_arg[11],ris_flt_arg[12],
				ris_flt_arg[13],ris_flt_arg[14],ris_flt_arg[15],
				ris_flt_arg[16],ris_flt_arg[17],ris_flt_arg[18],
				ris_flt_arg[19]);
# endif /* aix */
# if defined(ds3_ulx)
                            if (float_cnt)
                                (void) load_float_reg(ris_flt_arg[0],
				  ris_flt_arg[1]);
# endif /* ds3_ulx */
# ifdef axp_osf
			    if (floats_passed)
				load_float_regs(float_arg[0], float_arg[1],
						float_arg[2], float_arg[3],
						float_arg[4], float_arg[5]);
# endif /* axp_osf */
# if defined(a64_lnx)
			    if (floats_passed)
				load_float_regs14(float_arg[0], float_arg[1],
						  float_arg[2], float_arg[3],
						  float_arg[4], float_arg[5],
						  float_arg[6], float_arg[7],
						  float_arg[8], float_arg[9], 
						  float_arg[10], float_arg[11],
						  float_arg[12], float_arg[13]);
# endif /* a64_lnx */
# if defined(i64_hpu) || defined(i64_lnx)
			    if (floats_passed)
				load_float_regs40(float_arg[0], float_arg[1], float_arg[2], float_arg[3], float_arg[4],
						float_arg[5], float_arg[6], float_arg[7], float_arg[8], float_arg[9],
						float_arg[10], float_arg[11], float_arg[12], float_arg[13], float_arg[14],
						float_arg[15], float_arg[16], float_arg[17], float_arg[18], float_arg[19],
						float_arg[20], float_arg[21], float_arg[22], float_arg[23], float_arg[24],
						float_arg[25], float_arg[26], float_arg[27], float_arg[28], float_arg[29],
						float_arg[30], float_arg[31], float_arg[32], float_arg[33], float_arg[34],
						float_arg[35], float_arg[36], float_arg[37], float_arg[38], float_arg[39]);
# endif /* i64_hpu || i64_lnx */
				    ret = (*(char * (*)())func)(av[0], av[1], av[2], av[3],
				    av[4], av[5], av[6], av[7], av[8], av[9], av[10],
				    av[11], av[12], av[13], av[14], av[15], av[16],
				    av[17], av[18], av[19], av[20], av[21], av[22],
				    av[23], av[24], av[25], av[26], av[27], av[28],
				    av[29], av[30], av[31], av[32], av[33], av[34],
				    av[35], av[36], av[37], av[38], av[39] ); break;
		            }
	
			    /* for the "fake" string returns, the returned
			    ** string is the first argument.  It sure looks
			    ** like there's no need to copy it below, as
			    ** av[0] and OL_string should point to the same 
			    ** place...  We do it anyway out of paranoia.
			    ** also note that it makes sense to copy this
			    ** into retvalue->OL_string, which has been
			    ** allocated by our caller.
			    */
			    if (mecchar[ lang ].srtype == FSTRFAKE)
				ret = (char *)av[0];
			    if (ret != NULL)
			    {
			        STncpy( retvalue->OL_string, ret,
						OL_MAX_RETLEN - 1 );
				retvalue->OL_string[ OL_MAX_RETLEN - 1 ] = EOS;
			    }
			    else
			    {
				/* 
				** Change a returned NULL string 
				** to an empty one 
				*/
				*retvalue->OL_string = EOS;
			    }
			}
                break;
	}
	return( OK );
}

# ifdef axp_osf
load_float_regs( a, b, c, d, e, f )
float	a, b, c, d, e, f;
{
}
# endif /* axp_osf */

# if defined(ner_us5) || defined(pym_us5) || defined(rmx_us5) \
  || defined(ts2_us5) || defined(rux_us5)
load_float_regs(fparg1, fparg2)
f8 fparg1;
f8 fparg2;
{
}
# endif /* ner_us5 pym_us5 rmx_us5 ts2_us5 */
