/*
** Copyright (c) 2004 Ingres Corporation
*/

#define	DUT_MAIN 1
#include    <compat.h>
#include    <dbms.h>
#include    <ex.h>
#include    <me.h>
#include    <pc.h>
#include    <er.h>
#include    <duerr.h>
EXEC SQL INCLUDE 'dut.sh';

/**
**
**  Name: DUTMAIN.SC - Database utility for INGRES/STAR (StarView)
**
**  Description:
**
**  History:
**      23-oct-88 (alexc)
**          Creation for 6.0 starview.
**	08-mar-1989 (alexc)
**	    Alpha test version.
**      08-apr-90 (carl)
**          Retrofitted changes from hp9_mpe port
**	17-sep-90 (fpang)
**	    Added MEadvise(ME_INGRES_ALLOC)
**	22-jan-90 (fpang)
**	    Added UFLIB, not really needed here because it's already in the
**	    MINGH file, but just for consistency.
**      16-nov-1992 (larrym)
**          Added CUFLIB to NEEDLIBS.  CUF is the new common utility facility.
**      26-oct-1993 (dianeh)
**          Added RUNTIMELIB to NEEDLIBS; deleted TBACCLIB.
**	03/25/94 (dkh) - Added call to EXsetclient().
**	16-jun-95 (emmag)
**	    Define main as ii_user_main on NT.
**	24-sep-95 (tutto01)
**	    Restored main, run as a console app on NT.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      08-Aug-2002 (hweho01)
**          Remove "MODE = SETUID" in mkming hints, the file permission   
**          4755 caused error if the user is not ingres. (Star 11679284,     
**          and 12139322, Bug 108124) 
**	29-Sep-2004 (drivi01)
**	    Updated NEEDLIBS to link dynamic library SHQLIB and SHFRAMELIB
**	    to replace its static libraries. 
*/

/*
**      MKMFIN Hints
**

NEEDLIBS = STARVIEWLIB SHEMBEDLIB SHQLIB SHFRAMELIB \	
	   SHCOMPATLIB 

OWNER =	INGUSER

UNDEFS = IIeqiqio
PROGRAM = starview
*/

/* function_definition */
FUNC_EXTERN    i4      FEhandler();

/*
** Name: MAIN	- Main routine for DUTMAIN.SC(StarView).
**
** Description:
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
**      27-oct-1988 (alexc)
**          Creation.
**      08-apr-90 (carl)
**          Retrofitted changes from hp9_mpe port:
**          1)  clear explicitly dut_cb and dut_errcb upon startup;
**          2)  install FEhandler to handle frontend failures.
**          3)  initialize the forms system before calling dut_ii1_init,
**              so that error messages in dut_ii1_init work properly;
**	13-apr-90 (alexc)
**	    FUNCT_EXTERN declaration change to FUNC_EXTERN.
*/

i4
main(argc, argv)
i4	argc;
char	**argv;
{
    DUT_CB	dut_cb;
    DUT_ERRCB	dut_errcb;
    EX_CONTEXT  context;

    /* Tell EX this is an ingres tool. */
    (void) EXsetclient(EX_INGRES_TOOL);

    MEadvise(ME_INGRES_ALLOC);
    MEfill((u_i2) sizeof(DUT_CB), (u_char) 0, (PTR) & dut_cb);
    MEfill((u_i2) sizeof(DUT_ERRCB), (u_char) 0, (PTR) & dut_errcb);

    if (EXdeclare(FEhandler, & context) != OK)
        PCexit(FAIL);

    dut_ii3_form_init(&dut_cb, &dut_errcb);

    /* Get Node name and Database name from command line */

    if (dut_ii1_init(argc, argv, &dut_cb, &dut_errcb)
	!= E_DU_OK)
    {
	dut_ee2_exit(&dut_cb, &dut_errcb);
        EXdelete();
	PCexit(0);
    }

    dut_ff1_1intro(&dut_cb, &dut_errcb);
    if (dut_cb.dut_c8_status != E_DU_OK)
    {
	dut_ee2_exit(&dut_cb, &dut_errcb);
    }

    dut_uu4_cleanup(&dut_cb, &dut_errcb);

    EXdelete();
    PCexit(0);
}

