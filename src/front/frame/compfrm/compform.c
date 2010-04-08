/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<cv.h>
#include	<pc.h>
#include	<ci.h>
#include	<me.h>
# include	<st.h>
#include	<ex.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	"erfc.h"
#include	"decls.h"

/**
** Name:    compform.c -	CompForm Forms Compilation Program.
**
** Description:
**	This contains the batch form compiler program.	It is
**	called with the following command syntax:
**
**	  compform dbname form file [-c] [-uusername] [-m]
**
**	  where:
**		dbname	Database.
**		form	Form name to compile.
**		file	File into which to compile form.
**		-c	Use RT internal conventions for the compiled
**			output.	 This uses a bracketed include for
**			frame2.h and puts a GLOBALDEF keyword in
**			front of the FRAME * pointer.
**		-uuser	For an alternate owner of the form.
**		-m	Write out the form in macro.  (VAX/VMS only).
**
**	This file defines:
**
**	main()	main line and entry point for form compilation program.
**
** History:
**	Revision 6.0  87/12/15  peter.
**		15-dec-1986 (peter)	Written.
**		19-jun-87 (bab) Code cleanup.
**		08/14/87 (dkh) - ER changes.
**		09/30/87 (dkh) - Disallow -m flag for all VMS environments.
**		10/28/87 (dkh) - Delete definition of FEtesting, now in ug.
**		12/18/87 (dkh) - 5004b integration.
**		28-dec-89 (kenl) - Added handling of +c flag.
**		02/23/90 (dkh) - Fixed bug 9884.
**		05/15/90 (dkh ) - Fixed merge problem that left two calls
**				  to FEingres.
**              09/28/90 (stevet) - Add call to IIUGinit.
**		10-sep-92 (leighb) DeskTop Porting Change: Tools for Windows
**		    Make sure correct correct FRAME.DLL is loaded because 
**		    much global data is accessed from that DLL.
**      16-nov-1992 (larrym)
**          Added CUFLIB to NEEDLIBS.  CUF is the new common utility facility.
**	02/20/93 (dkh) - Changed to run without a db connection.
**	03/14/93 (dkh) - Fixed up compiler complaints.
**	03/25/94 (dkh) - Added call to EXsetclient().
**	10-jun-97 (cohmi01)  Add dummy calls to force inclusion of libruntime.
**	18-Dec-97 (gordy)
**	    Added SQLCALIB to NEEDLIBS, now required by LIBQ.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**	29-Sep-2004 (drivi01)
**	    Updated NEEDLIBS to link dynamic library SHQLIB and SHFRAMELIB 
**	    to replace its static libraries. Added NEEDLIBSW hint which is used
**	    for compiling on windows and compliments NEEDLIBS.
**/

# ifdef DGC_AOS
# define main IIFCrsmRingSevenMain
# endif

/*
**	MKMFIN Hints.
**
PROGRAM = compform

NEEDLIBS = COMPFRMLIB SHFRAMELIB SHQLIB COMPATLIB SHEMBEDLIB

UNDEFS =	II_copyright
**
*/

FUNC_EXTERN	FRAME	*IICFrfReadForm();

/*{
** Name:    main() -	Main Line and Entry Point for Forms Compilation Program.
**
** Description:
**	This is the main processing routine for COMPFORM, which performs
**	standard FE startup tasks, such as parsing the command line,
**	starting INGRES, etc.
**
** Inputs:
**	argc,argv	From command line.
**
** Returns:
**	Exits through PCexit.
**
** History:
**	15-dec-1986 (peter)	Written.
*/

/* Parameter Description */

static ARG_DESC
	args[] = {
		/* Required */
		    {ERx("database"),	DB_CHR_TYPE,	FARG_PROMPT,	NULL},
		    {ERx("form"),	DB_CHR_TYPE,	FARG_PROMPT,	NULL},
		    {ERx("file"),	DB_CHR_TYPE,	FARG_PROMPT,	NULL},
		/* Internal optional */
		    {ERx("equel"),	DB_CHR_TYPE,	FARG_FAIL,	NULL},
		/* Optional */
		    {ERx("user"),	DB_CHR_TYPE,	FARG_FAIL,	NULL},
		    {ERx("connect"),	DB_CHR_TYPE,	FARG_FAIL,	NULL},
		    /* VAX/VMS only */
		    {ERx("macro"),	DB_BOO_TYPE,	FARG_FAIL,	NULL},
		/* Internal, undocumented */
		    {ERx("symbol"),	DB_CHR_TYPE,	FARG_FAIL,	NULL},
		    {ERx("rti"),	DB_BOO_TYPE,	FARG_FAIL,	NULL},
		    NULL
};

i4
main(argc, argv)
i4	argc;
char	**argv;
{
    char	*userflag;
    char	*eqflag;
    char	*connect = ERx("");
    bool	macro;
    char	formname[FE_MAXNAME + 1];
    EX_CONTEXT	context;

    EX	FEhandler();

    /* Tell EX this is an ingres tool. */
    (void) EXsetclient(EX_INGRES_TOOL);

    MEadvise( ME_INGRES_ALLOC );	/* use ME allocation */

    /* Set the rest of globals to be constants */

#ifdef	PMFEWIN3
    IIload_framew();
#endif

    fcfrm = NULL;		/* address of compiled form */
    fceqlcall = FALSE;		/* Not called by EQUEL */
    fcstdalone = TRUE;		/* This is a standalone program */

    fclang = DSL_C;		/* C is default language, unless -m spec */
    fcname = NULL;		/* compiled address */
    fcrti = FALSE;		/* Use RTI conventions */

    userflag = eqflag = ERx("");	/* If not specified, must be blank */
    macro = FALSE;

    /* Add call to IIUGinit to initialize character set table and others */

    if ( IIUGinit() != OK )
    {
        PCexit(FAIL);
    }


    /* Parse out the command line */

    args[0]._ref = (PTR)&fcdbname;
    args[1]._ref = (PTR)fcnames;
    args[2]._ref = (PTR)&fcoutfile;
    args[3]._ref = (PTR)&eqflag;
    args[4]._ref = (PTR)&userflag;
    args[5]._ref = (PTR)&connect;
    args[6]._ref = (PTR)&macro;
    args[7]._ref = (PTR)&fcname;
    args[8]._ref = (PTR)&fcrti;
    if (IIUGuapParse(argc, argv, ERx("compform"), args) != OK)
    {
	PCexit(FAIL);
    }

    if (macro)
    {
#ifdef VMS
	fclang = DSL_MACRO;
#else
	FEutaerr(E_FC0004_M_NOT_SUPPORTED, 0, (char *)NULL);
#endif /* VMS */
    }

    if (EXdeclare(FEhandler, &context) != OK)
    {
	EXdelete();
	PCexit(FAIL);
    }

    FEcopyright(ERx("COMPFORM"), ERx("1986"));

    if (!fcrti)
    {
	if (*connect != EOS)
	{
	    IIUIswc_SetWithClause(connect);
	}
# ifdef DGC_AOS
	else
	{
	    connect = STalloc(ERx("dgc_mode='reading'"));
	    IIUIswc_SetWithClause(connect);
	}
# endif


	if ( FEingres(fcdbname, userflag, eqflag, (char *)NULL) != OK )
	{
	    IIUGerr( E_UG000F_NoDBconnect, UG_ERR_ERROR, 1, (PTR)fcdbname );
	    EXdelete();
	    PCexit(FAIL);
	}

	/*
	**  Lower case the form name.
	*/
	CVlower(*fcnames);

	/* call module */
	FDfrprint(1);
    }
    else
    {
	if ((fcfrm = IICFrfReadForm(*fcnames, formname)) == NULL)
	{
	    EXdelete();
	    PCexit(FAIL);
	}

	/*
	**  Set things up so that the real form name will show
	**  up in any informatinal messages later on.
	*/
	*fcnames = formname;
    }

    fccomfrm();

    if (!fcrti)
    {
	FEing_exit();
    }

    EXdelete();

    PCexit(OK);
}

/* this function should not be called. Its purpose is to   */
/* force some linkers that cant handle circular references */
/* between libraries to include these functions so they    */
/* won't show up as undefined 				   */
dummy1_to_force_inclusion()
{  
    IItscroll(); 
    IItbsmode();
}

