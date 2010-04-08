/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<pc.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<me.h>
# include	<ci.h>
# include	<er.h>
# include	<ex.h>
# include	<adf.h>
# include	<fmt.h>
# include	<rtype.h> 
# include	<rglob.h> 
# include	<ug.h> 

/**
** Name:	rmain.c -	Report Writer Main Line & Entry Point.
**
** Description:
**	Defines 'main()' for main program entry for the Report Writer and
**	also implements its main line.
**
** History:
**	86/05/12  10:16:15  wong
**		Added stack definition for PC.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**	29-Sep-2004 (drivi01)
**	    Updated NEEDLIBS to link dynamic library SHQLIB and SHFRAMELIB
**	    to replace its static libraries. Added NEEDLIBSW hint which is 
**	    used for compiling on windows and compliments NEEDLIBS.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
**/

# ifdef MSDOS
# ifndef PMFE						/* 6-x_PC_80x86 */
int	_stack = 10240;
# endif
# endif

/*{
** Name:	main() -	Report Writer Main Line & Entry Point.
**
**	Report Formatter - main routine.
**	-------------------------------
**
**	MAIN - main entry point to program.  This routine 
**		simply calls subordinate routines to do the 
**		processing.
**
**	Calling Sequence:
**		REPORT dbname repname [(parameters)] [-flags] [-Rtraceflags]
**		where  	repname = name of report.
**			dbname = name of database.
**			parameters = optional runtime parameters.
**			flags = run time flags
**			traceflags = used for debugging.
**
**	Trace Flags:
**		0.0, 1.0, 1.1.
**
**	History:
**		3/4/81 (ps)	written.
**		10/15/81 (ps)	modified for calling from program. 
**		22-oct-1985 (peter)	Added license check.
**		2/18/87 (daveb)	 -- Call to MEadvise, added MALLOCLIB hint.
**		07-aug-89 (sylviap)	
**			Added FSTMLIB, QRLIB and MTLIB for scrollable output.
**		17-oct-89 (sylviap)	
**			Took out FSTMLIB and QRLIB.
**		30-nov-89 (sandyd)
**			Removed unnecessary FEtesting UNDEF.
**              05-oct-90 (stevet)
**                      Added IIUGinit call to initialize character set 
**                      attribute table.
**		3/11/91 (elein)
**			PC Porting changes * 6-x_PC_80x86 *
**	      01-sep-1992 (rdrane)
**	          Use new constants in (s/r)_reset() invocations.
**      16-nov-1992 (larrym)
**          Added CUFLIB to NEEDLIBS.  CUF is the new common utility facility.
**      16-sep-1993 (rdrane)
**          Remove SQLLIB from NEEDLIBS.
**	03/25/94 (dkh) - Added call to EXsetclient().
**	16-jun-95 (emmag)
**	    Define main as ii_user_main for NT.
**	24-sep-95 (tutto01)
**	    Restored main, run as console app on NT..
*/

# ifdef DGC_AOS
# define main IIRWrsmRingSevenMain
# endif

/*
**	MKMFIN Hints
**
PROGRAM =	report

NEEDLIBS =	REPORTLIB SREPORTLIB SHFRAMELIB	SHQLIB SHCOMPATLIB

NEEDLIBSW =	SHEMBEDLIB SHADFLIB 

UNDEFS = 	II_copyright
**
*/
i4	/* 6-x_PC_80x86 */
main(argc,argv)
i4	argc;		/* number of parameters */
char	*argv[];	/* addresses of parameter strings */
{
	/* start of MAIN routine */

	/* Tell EX this is an ingres tool. */
	(void) EXsetclient(EX_INGRES_TOOL);

	/* Use the ingres allocator instead of malloc/free default (daveb) */

	MEadvise( ME_INGRES_ALLOC );


	/* Call IIUGinit to initialize character set attribute table */
	if ( IIUGinit() != OK)
	{
	        PCexit(FAIL);
	}

	r_reset(RP_RESET_PGM,RP_RESET_DB,RP_RESET_LIST_END);

	En_program = PRG_REPORT;
	St_called = FALSE;

	PCexit(r_report(argc,argv));	/* runs the report and exits */
}
