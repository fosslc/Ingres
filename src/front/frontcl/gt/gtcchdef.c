/*
**    Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<iicommon.h>
# include	"gkscomp.h"
# include	<er.h>
# include	<gtdef.h>
# include	"ergt.h"
# include	<fe.h>
# include	<ug.h>
# ifdef DGC_AOS
#    include 	<cm.h>
#    include	<termdr.h>
# endif
/**
** Name:    gtcchdef.c -	cchart objects
**
** Description:
**	symbols which must be defined for cchart - implies that
**	this file should be linked as an object rather than a library.
**
**	gerhnd	GKS error handler
**
** History:
**	6-nov-86 (bab)	Fix for bug 9255
**		Give more informative error message when user's data
**		for pie chart contains all zeros.
**	02-mar-89 (Mike S) Fix for bug 4426
**		Add function GTclrerr.
**	10-mar-89 (Mike S)
**		Add DG-specific escape functions.
**	10/20/89 (dkh) - Added changes to eliminate duplicate FT files
**			 in GT directory.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**    25-Oct-2005 (hanje04)
**        Add prototype for draw_err() to prevent compiler
**        errors with GCC 4.0 due to conflict with implicit declaration.
**	24-Aug-2009 (kschendel) 121804
**	    Need ug.h to satisfy gcc 4.3.
**/

extern Gws *G_ws;

extern i4  (*G_mfunc) ();
extern i4  (*G_efunc) ();

static draw_err();

static char Defr_func[24];
static Gerror Defr_err = 0;

/*{
** Name:	gerhnd	- GKS error handler
**
** Description:
**	Mechanism given by VE for application handling of GKS
**	errors - this routine replaces the default one otherwise
**	linked in from their library.  Thus, name and inputs are
**      dictated by VE.	 The default one simply dumps an error
**	message out to stderr.
**
** Inputs:
**	err	GKS error number
**	func	name of GKS procedure in which error occurred
**
** Outputs:
**	Returns:
**		error number
**
** Side Effects:
**	none
**
** History:
**	1/89 (MSS) 	version of gerhnd for DG calling sequence added
**	8/85 (rlm)	written
*/

Gerror
# ifdef DGC_AOS
  gerrorhand( err, funcnum, perrfile )
  Gerror err;		/* GKS error number */
  Gint funcnum;		/* number of GKS procedure in which error occured */
  int *perrfile;	/* Error file (ignored) */
# else
  gerhnd(err, funcname)
  Gerror err;		/* GKS error number */
  Gchar *funcname;	/* name of GKS procedure in which error occured */
# endif
{
#ifdef DGC_AOS
	Gchar funcname[24];
	extern Gint G_gkserror;

	STprintf( funcname, ERx("GKS function %d"), funcnum );
	if (G_gkserror == NO_ERROR)
		G_gkserror = err;
#endif

	if (Defr_err > 0)
		return (err);

	STcopy (funcname, Defr_func);
	Defr_err = err;

	return (err);
}

GTderr()
{
	if (Defr_err > 0)
	{
		(*G_mfunc)(ALPHAMODE);
		draw_err(ERx("GKS"), Defr_err, Defr_func);
		Defr_err = 0;
	}
}

GTclrerr()
{
	Defr_err = 0;
}

/*{
** Name:	cprinterr	- Cchart error handler
**
** Description:
**	Mechanism given by VE for application handling of Cchart
**	errors - this routine replaces the default one otherwise
**	linked in from their library.  Thus, name and inputs are
**	dictated by VE.	 The default one simply dumps an error
**	message out to stderr.	For us, a Cchart error aborts current drawing
**	operation.
**
** Inputs:
**	err	Cchart error number
**	func	name of Cchart procedure in which error occurred
**
** Outputs:
**	Returns:
**		none, generates exception
**	Exceptions:
**		EXVALJMP
**
** Side Effects:
**	none
**
** History:
**	4/86 (rlm)	written
*/

cprinterr(err, func)
i4  err;	/* Cchart error number */
char *func;	/* Cchart procedure in which error was encountered */
{
	(*G_mfunc) (ALPHAMODE);
	draw_err (ERx("Cchart"), err, func);
	EXsignal (EXVALJMP, 0);

	/* shouldn't be reached */
	return (0);
}

/*
** static utility to generate error message before killing drawing
** send to "pointerized" error message function to avoid problems
** with linking in FRS for rungraph, and assure correct FRS interactions
** for vigraph.
*/
static
draw_err (level, err, func)
char *level;			/* GKS / Cchart */
i4  err;
char *func;			/* function */
{
	char msg[80];
	i4 lerr;

	/*
	** If sum of pie chart slice values is 0, give
	** better error message; this is one error that can
	** occur without being a Vigraph/Cchart problem.
	*/
	if (STcompare(func, ERx("pie: zero sum")) == 0)
	{
		STcopy (ERget(E_GT0016_pie_zero_sum), msg);
	}
	else
	{
		lerr = err;
		IIUGfmt(msg, 80, ERget(E_GT0017_Draw_err_format),
						3, level, &lerr, func);
	}
	(*G_efunc) (msg);
}

# ifdef DGC_AOS
i4  	GTchout();
char 	*TDtgoto();
void 	Vstring();

extern char *G_tc_GL, *G_tc_GE, *G_tc_cm;
extern bool G_interactive;

/*{
** Name:	Valphamode -- put terminal in alpha (non-graphics) mode
**
** Description:
**	Function defined by VEC.  DG doesn't have the notion of terminal mode.
**	We use GKS's escape mechanism to output the TERMCAP string GL
**	(Leave Graphics mode).
**
**	The somewhat confusing steps involved in putting the terminal
**	into alpha mode on the DG are:
**		call *G_mfunc (mode_func in gtinit.c)
**		which calls calphamode (in gtinit.c) 
**		which calls dg_gescape (value of macro gesc, in dg_gks.c)
**		which calls Valphamode (below)
**		which calls Vstring (below)
**		which calls dg_callescape (in dg_gks.h)
**		which calls gescape (the GKS escape function)
**		which outputs the TERMCAP GL string.
**
**	This is actually the worst case (a non-interactive plot from VIGRAPH).
**	Interactive VIGRAPH outputs the string in calphamode.  RUNGRAPH
**	calls dg_gescape directly from mode_func.
** Inputs:
**	wsptr	workstation pointer	
**
** Outputs:
**
** Side Effects:
**	Output string to terminal.
**
** History:
**	3/89    (Mike S) written
*/
void Valphamode( wsptr )
Gws *wsptr;
{
    Gescstring  escstring;
    
    if ( G_tc_GL != NULL && *G_tc_GL != EOS )
    {
	escstring.ws = wsptr;
	escstring.s = G_tc_GL;
	Vstring( &escstring );
    }
}

/*{
** Name:	Vgraphmode -- put terminal in graphics mode
**
** Description:
**	Function defined by VEC.  DG doesn't have the notion of terminal mode.
**	We use GKS's escape mechanism to output the TERMCAP string GE
**	(Enter Graphics mode).
**
** Inputs:
**	wsptr	workstation pointer	
**
** Outputs:
**
** Side Effects:
**	Output string to terminal
**
** History:
**	3/89    (Mike S) written
*/
void Vgraphmode( wsptr )
Gws *wsptr;
{
    Gescstring escstring;
    extern i4  G_homerow;

    escstring.ws = wsptr;
    if ( G_interactive )
    {
	escstring.s = TDtgoto(G_tc_cm, G_homerow, 0);
	/*
	**	Skip padding on "cm" entry.
	*/
	while (CMdigit(escstring.s))
		CMnext(escstring.s);
	Vstring( &escstring );
    }
    if ( G_tc_GE != NULL && *G_tc_GE != EOS )
    {
	escstring.s = G_tc_GE;
	Vstring( &escstring );
    }
}

/*{
** Name:	Vstring -- output string to terminal
**
** Description:
**	Function defined by VEC.  We use GKS's escape mecahinsm to output a
**	string.
**
** Inputs:
**	escstring	workstation pointer and string to output
**
** Outputs:
**
** Side Effects:
**	Output string to terminal
**
** History:
**	3/89    (Mike S) written
**	3/23/89 (Mike S) add "guwk" to force output
*/
void Vstring( escstring )
Gescstring *escstring;
{
    extern void gspecified ();
    Gescin escparam;

    escparam.gspecified.ws = *(escstring->ws);
    escparam.gspecified.bin_data = escstring->s;
    escparam.gspecified.num_bytes = strlen( escstring->s );
    dg_callgescape( gspecified, &escparam );
    guwk( escstring->ws, POSTPONE );
}
# endif
