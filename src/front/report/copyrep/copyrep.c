/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<pc.h>		/* 6-x_PC_80x86 */
# include	<me.h>
# include	<ci.h>
# include	<si.h>
# include	<ex.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include	<ooclass.h>
# include	<stype.h>
# include	<sglob.h>
# include	"errc.h"

/**
** Name:	copyrep.c -	Copy Report Main Line & Entry Point.
**
** Description:
**	Defines 'main()' for main program entry for the Copy Report program
**	and also implemnts its main line.
**
** History:
**		Revision 50.0  86/05/06	 15:12:21  wong
**		Changes for PC.
**
**	16-jan-1987 (peter)	Update NEEDLIBS for 6.0.
**	06/05/88 (dkh) - Added UNDEF of s_readin since it is defined
**			 in sreportlib but referenced by reportlib.
**	01-dec-89 (sandyd)
**		Removed unnecessary UNDEFS and NEEDLIBS.
**	18-jul-90 (sylviap)
**		Moved IIOOinit to be called after the database is opened. #31722
**      05-oct-90 (stevet)
**              Added IIUGinit call to initialize character set attribute 
**              table.
**      16-nov-1992 (larrym)
**          Added CUFLIB to NEEDLIBS.  CUF is the new common utility facility.
**      15-apr-1993 (rdrane)
**          Added MTLIB to NEEDLIBS.  The "improved" r_exit() processing
**	    requires it to effect a reset of the terminal.
**	03/25/94 (dkh) - Added call to EXsetclient().
**      11-jan-1996 (toumi01; from 1.1 axp_osf port)
**	    Cast (long) 3rd... EXsignal args to avoid truncation.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**	26-Aug-2009 (kschendel) b121804
**	    sglob.h is the right one, dunno what sglobi.h is for.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
**/

# ifdef MSDOS
int	_stack = 8192;
# endif

/*{
** Name:	main() -	Copy Report Main Line & Entry Point.
**
**   COPYREP - copy out a named report from the database
**	into a text file.  The call sequence is:
**
**	COPYREP [-s] [-f] [-uuser] dbname filename {repname}
**		where	dbname		is database.



**			repname		is report name.
**			filename	is the name of the file to write to.
**			-s		stay silent.
**			-f		set if copy like Filereport.
**			-uuser		alternate user name.
**
**	History:
**		9/12/82 (ps)	written.
**		11/3/83 (nml)	Modified so argument prompting and exiting when
**				exceptions are raised works correctly.
**		1/8/85 (rlm)	global tags for dynamic report writer memory
**              23-aug-89 (sylviap)
**                      Added FSTMLIB, QRLIB and MTLIB.
**		01-dec-89 (sandyd)
**			Removed FSTMLIB, QRLIB, MTLIB, SQLLIB, and FEtesting.
**			The necesary parts of FSTM are now in UG.
**		18-jul-90 (sylviap)
**			Moved IIOOinit to be called after the database is 
**			opened. #31722
**              05-oct-90 (stevet)
**                      Added IIUGinit call to initialize character set 
**                      attribute table.
**		12-aug-1992 (rdrane)
**			Use new constants in (s/r)_reset invocations.
**			Add license check to be consistent with the rest of
**			the RW family.  Make copyright display unconditional.
**			St_called was being forced FALSE anyway!
*/

/*
**	External for this program.
*/

	GLOBALDEF bool	St_fflag = FALSE;	/* TRUE if -f flag set */

# ifdef DGC_AOS
# define main IIRCrsmRingSevenMain
# endif

/*
**	MKMFIN Hints
**
PROGRAM =	copyrep

NEEDLIBS =	COPYREPLIB SREPORTLIB REPORTLIB OOLIB \
		UFLIB RUNSYSLIB RUNTIMELIB FDLIB \
		MTLIB FTLIB FEDSLIB \
		UILIB LIBQSYSLIB LIBQLIB UGLIB FMTLIB AFELIB \
		LIBQGCALIB CUFLIB GCFLIB ADFLIB COMPATLIB MALLOCLIB

	The "s_readin" undef is needed because of circular references between
	REPORTLIB and SREPORTLIB.

UNDEFS =	s_readin
*/

main(argc, argv)
i4	argc;
char	*argv[];	/* addresses of parameter strings */
{
	/* internal declarations */

	EX_CONTEXT	context;

	/* start of MAIN routine */

	/* Tell EX this is an ingres tool. */
	(void) EXsetclient(EX_INGRES_TOOL);

	MEadvise(ME_INGRES_ALLOC);

	/* Call IIUGinit to initialize character set attribute table */
	if ( IIUGinit() != OK)
	{
	    PCexit(FAIL);
	}

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

	if  (!St_called)
	{
	    r_reset(RP_RESET_DB,RP_RESET_LIST_END);
	}

	St_called = FALSE;
	En_program = PRG_COPYREP;

	if ( EXdeclare(r_exit, &context) != OK )
	{
		EXdelete();
		PCexit(Err_type);	/* ALL EXITS COME HERE */
	}

	IIseterr(r_ing_err);

	FEcopyright(ERx("COPYREP"), ERx("1984"));

	crcrack_cmd(argc,argv);		/* will abort on errors */

	r_open_db();			/* open the database */

	/* start object-oriented facility for retrieving reports */
	IIOOinit((OO_OBJECT **)NULL);

	cr_dmp_rep(Usercode,Ptr_ren_top,En_fspec,!St_fflag);

	EXsignal(EXEXIT, 2, (long)0, (long)NOABORT);
	/* NOTREACHED */
	EXdelete();
}
