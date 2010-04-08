/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<pc.h>
# include	<ex.h>
# include	<me.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ui.h>
# include	<ooclass.h>
# include	<er.h>
# include	<erde.h>
# include	"doglobi.h"

/*
** Name:	delobj.c - Delete FE Object Main Line & Entry Point.
**
** Description:
**	Defines 'main()' for main program entry for the Delete FE Object
**	utility program	and also implements its main line.
**
**	NOTE:
**		The ifdef of DGC_AOS relates to sharing of code space between
**		REPORT,	SREPORT, and COPYREP.  Thus, even though we are the
**		mainline routine, we'll expect to share the global definitions
**		as established by the REPORT mainline.
**
** History:
**	12-jan-1993 (rdrane)
**		Created.  Modelled after REPORT/SREPORT mainlines.
**	03/25/94 (dkh) - Added call to EXsetclient().
**	
**	18-sep-95 (emmag)
**		Declare the exception handler since we're going to 
**		delete it later.   Attempting to delete an exception
**		handler that hasn't been declared on NT results in a
**		GPF.
**	10-nov-1995 (canor01)
**		Use PCexit() to exit the program instead of EXsignal()
**		to avoid differences in how the exception is handled
**		on Solaris and NT.
**	04-Dec-1995 (mckba01)
**		#68839, changed exit method to PCexit(OK);
**	18-jun-97 (hayke02)
**	    Add dummy calls to force inclusion of libruntime.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# ifdef MSDOS
int	_stack = 8192;
# endif


/*
** Name:	main() -	Delete Report Main Line & Entry Point.
**
**	DELOBJ - delete a named object from the FE catalogs.
**	(See dobjcrk.c for documentation of the calling sequence.)
**
** History:
**	12-jan-1993 (rdrane)
**		Created.
*/

/*
**	External for this program.
*/

# ifdef DGC_AOS
# define main IIRCrsmRingSevenMain
# endif


/*
**	MKMFIN Hints
**
PROGRAM =	delobj

NEEDLIBS =	DELOBJLIB \
		COPYUTILLIB COPYFORMLIB IAOMLIB \
		UFLIB RUNSYSLIB RUNTIMELIB FDLIB \
		FTLIB FEDSLIB \
		OOLIB UILIB LIBQSYSLIB LIBQLIB UGLIB FMTLIB AFELIB \
		LIBQGCALIB CUFLIB GCFLIB ADFLIB COMPATLIB MALLOCLIB
**
*/

i4
main(i4 argc, char **argv)
{
        EX_CONTEXT      context;

	EX		FEhandler();

	/* Tell EX this is an ingres tool. */
	(void) EXsetclient(EX_INGRES_TOOL);

	MEadvise(ME_INGRES_ALLOC);

	/*
	** Call IIUGinit to initialize character set attribute table.
	*/
	if  (IIUGinit() != OK)
	{
	    PCexit(FAIL);
	}

	/*
	** Reset our globals
	*/
	Ptr_fd_top = (FE_DEL *)NULL;
	Cact_fd = (FE_DEL *)NULL;

	FEcopyright(ERx("DELOBJ"),ERx("1992"));

        if ( EXdeclare(FEhandler, &context) != OK )
        {
                EXdelete();
                PCexit(FAIL);
        }

	/*
	** This routine will abort if any errors occur.  Unlike REPORT & Co.,
	** we will open the DB there so we can get the invoking user name and
	** so successfully build the list of report names
	*/
	do_crack_cmd(argc,argv);

	_VOID_ do_del_obj();

	EXdelete();
	FEexits(ERx(""));
	PCexit(OK);
	/* NOTREACHED */
}

/* this function should not be called. Its purpose is to    */
/* force some linkers that can't handle circular references */
/* between libraries to include these functions so they     */
/* won't show up as undefined                               */
dummy1_to_force_inclusion()
{
    IItscroll();
    IItbsmode();
}
