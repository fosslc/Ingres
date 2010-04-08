/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<pc.h>
# include	<ci.h>
# include	<er.h>					 
# include	<ex.h>
# include	<me.h>
# include	<st.h>
# include	<tm.h>					 
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<dmchecks.h>				 
# include	<abclass.h>				 
# include	<oocat.h>				 
# include	<oocatlog.h>				 
# include	"abcatrow.h"				 
# include	"abfglobs.h"

/**
** Name:    abmain.c -	ABF Main Entry Point.
**
** Description:
**	Contains the entry point for the ABF, VISION, and IMAGEAPP programs.
**	Defines:
**
**	main()	ABF main entry point.
**
** History:
**
**      27-feb-90 (russ)
**              Add serialization support for ODT.
**
**      Revision 6.3/03/00  90/10  stevet
**      Add IIUGinit() call to initialize character set attribute table.
**
**	Revision 6.3/03/00  90/03  wong
**	Modified to check first argument for IMAGEAPP entry.
**
**	Revision 6.2  89/08  wong
**	Use the INGRES allocator now.
**
**	Revision 6.1  88/05  wong
**	Call 'IIabf()'.
**	12-may-1988 (marian) Removed ABRTSTABLIB from NEEDLIBS because
**	it's no longer needed.  (Also, GBFLIB.  88/11  wong)
**
**	Revision 6.0  87/02  daveb
**	2/18/87 (daveb)	 -- Call to MEadvise
**
**	Revision 5.0  85/10  peter
**	22-oct-1985 (peter)	Added license check.
**
**	Revision 2.0  82/04  joe
**	Written 4/16/82 (jrc)
**
**	10/14/90 (dkh) - Changed handling of IIar_Dbname and IIar_Status so
**			 these two last data values are not exported across
**			 the shared library interface for both VMS and UNIX.
**
**	03-apr-91 (rudyw)
**		Remove odt_us5 ifdef section to call odt_serialize.
**
**	30-aug-91 (leighb) Revived ImageBld:
**		Added IIOIimageBld flag; checked for 'imagebld' arg.
**
**	02-dec-91 (leighb) DeskTop Porting Change:
**		Added calls to routines to pass data across ABF/IAOM boundary:
**		 IIAMgaiGetIIAMabfiaom(), IIAMgfrGetIIamFrames(),
**		 IIAMgprGetIIamProcs(),   IIAMgglGetIIamGlobals(),
**		 IIARgadGetArDbname()  &  IIARgasGetArStatus();
**		Added variables loaded by these routines:
**		 IIam_Class, IIam_ProcCl, IIam_GlobCl, IIam_ConstCl,
**		 IIam_RecCl, IIam_RAttCl, IIAM_zdcDelCheck,
**		 IIAM_zccCompChange, IIAM_zctCompTree, IIAM_zdcDateChange,
**		 IIAM_zraRecAttCheck, IIam_Frames, IIam_Procs & IIam_Globals.
**		(See iaom/abfiaom.c for more information).
** 	6-Feb-92 (blaise)
**		Added include abfglobs.h.
**	4-May-92 (blaise)
**		Moved the check for whether roles are supported to IIabf();
**		we now look at iidbcapabilities rather than at the
**		authorization string, and the new check is made after
**		connecting to the dbms. (bug #43745)
**      16-nov-1992 (larrym)
**          Added CUFLIB to NEEDLIBS.  CUF is the new common utility facility.
**	03/25/94 (dkh) - Added call to EXsetclient().
**      27-Jun-95 (tutto01)
**              Redirected program entry to initialise MS windows first by
**              defining main as ii_user_main.
**	24-sep-95 (tutto01)
**		Restore main and run as a console app on NT.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2004 (drivi01)
**	    Updated NEEDLIBS to build iiabf executable dynamically on unix 
**	    and windows, added NEEDLIBSW hint which is used for compiling
**	    on windows and compliments NEEDLIBS.
*/

# ifdef DGC_AOS
# define main IIABrsmRingSevenMain
# endif

/*
**	MKMFIN Hints
**
PROGRAM = iiabf

NEEDLIBS = 	ABFLIB ABFIMAGELIB VQLIB FGLIB OSLLIB SQLLIB \
		GNLIB INGMENULIB VIFREDLIB PRINTFORMLIB COMPFRMLIB \
		COPYFORMLIB QBFLIB TBLUTILLIB INTERP_ABF \
		SHFRAMELIB SHQLIB SHCOMPATLIB ;

NEEDLIBSW =	SHEMBEDLIB SHADFLIB ;

UNDEFS =	II_copyright _And 
*/

FUNC_EXTERN IIAMABFIAOM *IIAMgaiGetIIAMabfiaom(); /* Get misc IAOM data */	 
FUNC_EXTERN APPL_COMP	*IIAMgfrGetIIamFrames(); /* system frames */		 
FUNC_EXTERN APPL_COMP	*IIAMgprGetIIamProcs();	 /* system procedures */	 
FUNC_EXTERN APPL_COMP	*IIAMgglGetIIamGlobals();/* system global variables */	 
FUNC_EXTERN	char	**IIARgadGetArDbname();	/* Get handle on database */
FUNC_EXTERN	STATUS	*IIARgasGetArStatus();	/* Get handle on status */

GLOBALDEF OO_CLASS *	IIam_Class = NULL;					 
GLOBALDEF OO_CLASS *	IIam_ProcCl = NULL;					 
GLOBALDEF OO_CLASS *	IIam_GlobCl = NULL;					 
GLOBALDEF OO_CLASS *	IIam_ConstCl = NULL;					 
GLOBALDEF OO_CLASS *	IIam_RecCl = NULL;					 
GLOBALDEF OO_CLASS *	IIam_RAttCl = NULL;					 
GLOBALDEF PTR		IIAM_zdcDelCheck = NULL;				 
GLOBALDEF PTR		IIAM_zccCompChange = NULL;				 
GLOBALDEF PTR		IIAM_zctCompTree = NULL;				 
GLOBALDEF PTR		IIAM_zdcDateChange = NULL;				 
GLOBALDEF PTR		IIAM_zraRecAttCheck = NULL;				 
GLOBALDEF APPL_COMP	*IIam_Frames = NULL;	/* system frames */		 
GLOBALDEF APPL_COMP	*IIam_Procs = NULL;	/* system procedures */		 
GLOBALDEF APPL_COMP	*IIam_Globals = NULL;	/* system global variables */	 
GLOBALDEF char		**IIar_Dbname = NULL;	/* ABF runtime database */
GLOBALDEF STATUS	*IIar_Status = NULL;	/* ABF runtime status */
GLOBALDEF bool		IIabKME = FALSE;    
			/* Are the Knowledge Management Extensions present */

GLOBALREF	i4	IIOIimageBld;		/* Build Interpreted Image? */		 
     /* 0 -> no; 1 -> yes, use std iiinterp; >1 -> yes, use custom iiinterp */		 

GLOBALREF AB_CATALOG	iiabAcatalog;
GLOBALREF AB_CATALOG	iiabCcatalog;
GLOBALREF AB_CATALOG	iiabGcatalog;
GLOBALREF AB_CATALOG	iiabMcatalog;
GLOBALREF AB_CATALOG	iiabRcatalog;

main(argc, argv)
i4	argc;
char	**argv;
{
	IIAMABFIAOM *iaom;

	/* Tell EX this is an ingres tool. */
	(void) EXsetclient(EX_INGRES_TOOL);

	/* Use the INGRES allocator as of 6.2.  All user code will now be
	** executed in the context of the interpreter.  Use the default user
	** allocator instead of the INGRES allocator only when dynamically
	** linking user code into ABF.
	*/
	MEadvise( ME_INGRES_ALLOC );

        /* Add call to IIUGinit to initialize character set table and others */
	if ( IIUGinit() != OK)
	{
	        PCexit(FAIL);
	}

	iaom = IIAMgaiGetIIAMabfiaom();						 
	IIam_ProcCl = (OO_CLASS *)(iaom->IIamProcCl);				 
	iiabAcatalog.c_class = IIam_Class = (OO_CLASS *)(iaom->IIamClass);	 
	iiabGcatalog.c_class = IIam_GlobCl = (OO_CLASS *)(iaom->IIamGlobCl);	 
	iiabCcatalog.c_class = IIam_ConstCl = (OO_CLASS *)(iaom->IIamConstCl);	 
	iiabRcatalog.c_class = IIam_RecCl = (OO_CLASS *)(iaom->IIamRecCl);	 
	iiabMcatalog.c_class = IIam_RAttCl = (OO_CLASS *)(iaom->IIamRAttCl);	 
	IIAM_zdcDelCheck = *(iaom->IIAMzdcDelCheck);				 
	IIAM_zccCompChange = *(iaom->IIAMzccCompChange);			 
	IIAM_zctCompTree = *(iaom->IIAMzctCompTree);				 
	IIAM_zdcDateChange = *(iaom->IIAMzdcDateChange);			 
	IIAM_zraRecAttCheck = *(iaom->IIAMzraRecAttCheck);			 

	IIam_Frames = IIAMgfrGetIIamFrames();					 
	IIam_Procs = IIAMgprGetIIamProcs();					 
	IIam_Globals = IIAMgglGetIIamGlobals();					 
	IIar_Dbname = IIARgadGetArDbname();
	IIar_Status = IIARgasGetArStatus();

	if ( argc > 1 && STbcompare(*(argv+1), 0, "imagebld", 0, TRUE) 	== 0 )	 
	{									 
		IIOIimageBld = 1;						 
		STcopy("imageapp", *(argv+1));					 
	}									 
	if ( argc > 1 && STbcompare(*(argv+1), 0, "imageapp", 0, TRUE) == 0 )
		PCexit(IIabfImage(argc-1, argv+1));
	else
	{
		PCexit(IIabf(argc, argv));
	}
}
