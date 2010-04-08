/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<cv.h>		 
# include	<me.h>
# include	<pc.h>		 
# include	<si.h>
# include	<ex.h>
# include	<ci.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include	<stype.h>	/* includes rtype.h */
# include	<oocat.h>
# ifndef DGC_AOS
# include	<sglobi.h>	/* includes scons.h  */
# else
# include	<sglob.h>
# endif
# include	"ersr.h"

/*
** Name:	main.c -	Report Specifier Main Line & Entry Point.
**
** Description:
**	Defines 'main()' for main program entry for the Report Specifier, and
**	also implements its main line.
**
** History:
**	86/05/07  11:45:41  wong
**		Removed "FMTLIB" and "SQLLIB" from hints; not needed.  Added
**		stack definition for PC and also conditional compilation of
**		transaction code for PC and CS.
**	2/18/87 (daveb)	 -- Call to MEadvise, added MALLOCLIB hint.
**	31-jan-1989 (danielt)
**		took out transaction code
**	18-jul-90 (sylviap)
**		Moved IIOOinit to be called after the database is opened.
**	09-jul-90 (cmr) fix for bug #30813
**		Moved CatWriteOn/Off calls in here instead of scattered in
**		other routines.  RBF and sreport were not detecting DBMS
**		errors or deadlock (CatWriteOff() resets the errorno).
**	05-oct-90 (stevet) Added IIUGinit call to initialize character set
**		attribute table.
**      3/11/91 (elein)
**              PC Porting changes * 6-x_PC_80x86 *
**	17-aug-91 (leighb) DeskTop Porting Change: added cv.h & pc.h
**	26-aug-1992 (rdrane)
**		R6.5 changes.
**	16-sep-1993 (rdrane)
**		Remove SQLLIB from NEEDLIBS (why didn't it go away with
**		the 86/05/07 change, and why is FMTLIB still there?)
**	03/25/94 (dkh) - Added call to EXsetclient().
**	12-jan-1996 (toumi01; from 1.1 axp_osf port)
**		Cast (long) 3rd... EXsignal args to avoid truncation.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

# ifdef MSDOS
# ifndef PMFE                                            
int	_stack = 16384;
# endif
# endif

/*{
** Name:	main() -	Report Specifier Main Line & Entry Point.
**
**	Report Specifier - main program.
**	-------------------------------
**
**	MAIN - main entry point to program.  This routine
**		simply calls subordinate routines to do the
**		processing.
**
**	Calling Sequence:
**		SREPORT {flags} txtfile dbname -rtraceflags
**		where	txtfile = file containing report specification.
**			dbname = name of database.
**			flags - optional flags.
**			traceflags = used for debugging.
**
**	Error Messages:
**		910.
**
**	Trace Flags:
**		11.0, 11.1.
**
**	History:
**		5/30/81 (ps) - written.
**		10/28/83(nml)- modified to reflect changes made to UNIX 2.0
**			       code.
**		2/13/84 (gac)	added multi-statement transactions.
**		2/26/85 (rlk) - Fixed a bug from the CFE project (added
**				defs for Rst{4|5}_tag.
**		10/15/85 (prs) - Add license check.
**		1/16/86 (rld) - Add MKMFIN hints
**		14-aug-89 (sylviap)
**			Added FSTMLIB, QRLIB and MTLIB.
**		8/21/89 (elein) garnet
**			added file name param to sreadin
**		30-nov-89 (sandyd)
**			Removed FSTMLIB, QRLIB, MTLIB, and FEtesting UNDEF.
**		18-jul-90 (sylviap)
**			Moved IIOOinit to be called after the database is opened
**	        05-oct-90 (stevet) Added IIUGinit call to initialize 
**                      character set attribute table.
**		24-oct-90 (sandyd)
**			Fixed trailing space on NEEDLIBS line, that was
**			messing up Porting's mkming.
**		26-aug-1992 (rdrane)
**			Use new constants in (s/r)_reset() invocations.
**			Eliminate transaction semantics - FE's run with
**			AUTOCOMMIT ON.  Demote to a plain vanilla ".c" file.
**			Converted r_error() err_num value to hex to facilitate
**			lookup in errw.msg file.  Fix-up external declarations -
**			they've been moved to hdr files.  Call IIseterr()
**			directly so we can eliminate s_init().  Reduce the size
**			of s_buffer - it only needs to hold an i2 alphanumeric
**			representation.
**		16-nov-1992 (larrym)
**			Added CUFLIB to NEEDLIBS.  CUF is the new common
**			utility facility.
**		15-apr-1993 (rdrane)
**			Added MTLIB to NEEDLIBS.  The "improved" r_exit()
**			processing requires it to effect a reset of the
**			terminal.
*/

# ifdef DGC_AOS
# define main IINRrsmRingSevenMain
# endif

/*
**	MKMFIN Hints
**
PROGRAM =	sreport

NEEDLIBS =	SREPORTLIB REPORTLIB OOLIB \
		UFLIB RUNSYSLIB RUNTIMELIB FDLIB MTLIB FTLIB \
		FEDSLIB UILIB LIBQSYSLIB LIBQLIB UGLIB FMTLIB AFELIB \
		LIBQGCALIB CUFLIB GCFLIB ADFLIB \
		COMPATLIB MALLOCLIB

UNDEFS	=	II_copyright
*/

i4       
main(argc,argv)
i4	argc;		/* number of parameters */
char	*argv[];	/* addresses of parameter strings */
{
	/* internal declarations */

	char		s_buffer[8];	/* used to hold converted error count */
	i4		i;
	EX_CONTEXT	context;

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

	/* start up abstract data facility */
	Adf_scb = FEadfcb();

	/*	Define Reset level 4 and 5 tags */


	Rst4_tag = FEgettag();
	Rst5_tag = FEgettag();

	/*
	** Reset report writer globals
	*/
	r_reset(RP_RESET_PGM,RP_RESET_CALL,
		RP_RESET_RELMEM4,RP_RESET_RELMEM5,
		RP_RESET_OUTPUT_FILE,RP_RESET_LIST_END);
	s_reset(RP_RESET_PGM,RP_RESET_SRC_FILE,RP_RESET_REPORT,
		RP_RESET_LIST_END);

	if  (St_called)
	{
	    r_reset(RP_RESET_DB,RP_RESET_LIST_END);
	}

	St_called = FALSE;
	En_program = PRG_SREPORT;

	if ( EXdeclare(r_exit, &context) != OK ) /* go to r_exit on interupt */
	{
		EXdelete();
		PCexit(Err_type);	/* ALL EXITS COME HERE */
	}

	IIseterr(r_ing_err);		/* set up tracers, links, etc. */
	s_crack_cmd(argc,argv);		/* will abort on errors */

	FEcopyright(ERx("SREPORT"), ERx("1981"));

	r_open_db();			/* open the database */

	/* start object-oriented facility for saving reports */
	IIOOinit((OO_OBJECT **)NULL);

	s_readin(En_sfile);			/* read in the file */

	/* if no errors have been found, write the report to the database */

	if (Err_count > 0)
	{
		CVna((i4)Err_count,s_buffer);	/* convert number to string */
		r_error(0x38E, FATAL, s_buffer, NULL);
	}

	/* add files to database */

	IIseterr(errproc);

	for (i = 0; i < MAX_RETRIES; i++)
	{
		iiuicw1_CatWriteOn(); /* allow writing FE catalog */
		ing_deadlock = FALSE;

		s_del_old();		/* delete old reports */
		if (ing_deadlock)
		{
			continue;
		}
		if (FEinqerr() != OK )
		{
			iiuicw0_CatWriteOff(); /* disallow writing FE catalog */
			EXsignal(EXEXIT, 2, (long)0, (long)ABORT);
		}

		s_copy_new();		/* put in the new ones */
		if (ing_deadlock)
		{
			continue;
		}
		if (FEinqerr() != OK )
		{
			iiuicw0_CatWriteOff(); /* disallow writing FE catalog */
			EXsignal(EXEXIT, 2, (long)0, (long)ABORT);
		}

		iiuicw0_CatWriteOff(); /* disallow writing FE catalog */
		break;
	}

	if (i >= MAX_RETRIES)
	{
		SIprintf(ERget(E_SR0003_Too_many_retries));
	}

	EXsignal(EXEXIT, 2, (long)0, (long)NOABORT);
	/* NOTREACHED */
	EXdelete();
}
