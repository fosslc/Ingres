/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<pc.h>		/* 6-x_PC_80x86 */
# include	<ci.h>
# include	<me.h>
# include	<st.h>
# include	<ex.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include	"erpf.h"

/**
** Name:    printform.c -	PrintForm Main Line and Entry Point Routine.
**
** Description:
**	Prints out the attributes of a form defined using vifred.
**	Prompts the user for the name of the database, the name of
**	the form, and the name of the file (in the current directory)
**	to which output should be directed.
**
** History:
**	Written 10/25/83 (agh)
**	Revised to handle command line arguments, 3/7/84 (agh)
**	10/21/85 (peter)	Pulled out dumpform.qc to another file.
**	5-jan-1986 (peter)	Change calls to FEuta/MKMFIN.
**	01/04/87 (dkh) - Fixed VMS compilation problem.
**	02/18/87 (daveb) -- Call to MEadvise, added MALLOCLIB hint.
**	05/08/87 (dkh) - Added include of ADFLIB in NEEDLIBS
**	08/14/87 (dkh) - ER changes.
**	10/09/87 (dkh) - Changed so that printform is not a forms program.
**	06-apr-88 (bruceb)
**		Prompt for "form" not "formname", "file" not "outfile"
**		--match expectations of utexe.def.
**	14-apr-88 (bruceb)
**		Added, if DG, a send of the terminal's 'is' string at
**		the very beginning of the program.  Needed for CEO.
**	12-aug-88 (bruceb)	Fix for bug 2900.
**		Added NULL as third parameter in call on dumpform().
**	28-dec-89 (kenl)
**		Added handling of +c flag.
**      28-sep-90 (stevet)
**              Add call to IIUGinit.
**      16-nov-1992 (larrym)
**          Added CUFLIB to NEEDLIBS.  CUF is the new common utility facility.
**	03/25/94 (dkh) - Added call to EXsetclient().
**	18-jun-97 (hayke02)
**	    Add dummy calls to force inclusion of libruntime.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
*/

# ifdef DGC_AOS
# define main IIPFrsmRingSevenMain
# endif

/*
**	MKMFIN Hints
**
PROGRAM =	printform

NEEDLIBS =	PRINTFORMLIB \
			UFLIB RUNSYSLIB RUNTIMELIB FDLIB FTLIB FEDSLIB \
			UILIB LIBQSYSLIB LIBQLIB UGLIB \
			FMTLIB AFELIB LIBQGCALIB CUFLIB GCFLIB ADFLIB \
			COMPATLIB MALLOCLIB

UNDEFS =	II_copyright
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
		    NULL
};


i4
main(argc,argv)
int	argc;
char	*argv[];
{
    char	*dbname;	/* database name */
    char	*formname;	/* form name to print */
    char	*outfile;	/* output file name */
    char	*xpipe;		/* X pipes used with -x flag */
    char	*uname;		/* -u flag entry */
    char	*connect = ERx("");
    EX_CONTEXT	context;
    EX		FEhandler();

    xpipe = uname = ERx("");		/* Start out with blanks */
	
    /* Tell EX this is an ingres tool. */
    (void) EXsetclient(EX_INGRES_TOOL);

    MEadvise( ME_INGRES_ALLOC );	/* use ME allocation */

    /* Add call to IIUGinit to initialize character set table and others */

    if ( IIUGinit() != OK )
    {
        PCexit(FAIL);
    }

    /* Get command line arguments (See 'args[]' above.) */
    args[0]._ref = (PTR)&dbname;
    args[1]._ref = (PTR)&formname;
    args[2]._ref = (PTR)&outfile;
    args[3]._ref = (PTR)&xpipe;
    args[4]._ref = (PTR)&uname;
    args[5]._ref = (PTR)&connect;
    if (IIUGuapParse(argc, argv, ERx("printform"), args) != OK)
	PCexit(FAIL);

    if (EXdeclare(FEhandler, &context) != OK)
    {
	EXdelete();
	PCexit(FAIL);
    }

    FEcopyright(ERx("PRINTFORM"), ERx("1984"));

    if (*connect != EOS)
    {
	IIUIswc_SetWithClause(connect);
    }
# ifdef DGC_AOS
    else
    {
        connect = STalloc(ERx("dgc_mode='reading'"));
    }
# endif


    if (FEingres(dbname, xpipe, uname, (char *)NULL) != OK)
    {
	IIUGmsg(ERget(E_PF0005_Could_not_open_databa), 0, 1, dbname);
	EXdelete();
	PCexit(FAIL);
    }

    CVlower(formname);
    dumpform(formname, outfile, NULL);

    FEing_exit();

    EXdelete();

    PCexit(OK);
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
