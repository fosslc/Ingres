/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<pc.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include	<ci.h>
# include	<me.h>
# include	<ex.h>
# include       <errw.h>
# include	"rbftype.h" 
# include	"rbfiglob.h" 
# include	<rglob.h>

/*{
** Name:	main() -	RBF Main Line and Entry Point Routine.
**
**	Report By Forms - main routine.
**	-------------------------------
**
**	MAIN - main entry point to program.  This routine 
**		simply calls subordinate routines to do the 
**		processing.
**
**	Calling Sequence:
**		RBF dbname repname -flags -Rtraceflags
**		where	repname = name of report.
**			dbname = name of database.
**			flags = optional runtime parameters.
**			traceflags = used for debugging.
**
** Trace Flags:
**	none.
**
** History:
**		2/10/82 (ps)	written.
**		22-oct-1985 (peter)	Added license check.
**		23-dec-1986 (peter)	Added new MKMFIN hints.
**		1/16/87 (rld)	Added new MKMFIN hints.
**		2/18/87 (daveb)	 -- Call to MEadvise, added MALLOCLIB hint.
**		16-dec-1987 (rdesmond)
**			added assignment of SQL as dml if ADF_CB so that
**			non-SQL aggs (any and uniques) can be handled
**			properly.  ADF should not be sensitive to this,
**			but this is a work-around for now.
**	06/05/88 (dkh) - Added UNDEF of s_readin since it is defined
**			 in sreportlib but referenced by reportlib.
**	07-aug-89 (sylviap)
**		Added FSTMLIB to NEEDLIBS for RW scrollable output.
**              9/22/89 (elein) UG changes
**                      ingresug change #90045
**			changed <rbftype.h> and <rbfglob.h> to
**			"rbftype.h" and "rbfglob.h"
**	9/25/89 (martym)
**		>> GARNET <<
**		Added QRLIB, and MTLIB since they are now required by the 
**		ReportWriter. Added QBFLIB for JoinDef support.
**	01-sep-89 (cmr )
**			added undef for rFeditlayout.
**	01-dec-89 (sandyd)
**		Cleaned out unnecessary cruft from NEEDLIBS and UNDEFS.
**	
**	01-sep-89 (cmr )
**			added undef for rFcols and rFagg.
**	11/27/89 (martym)
**			Added two libraries GENERATELIB, TBLUTILLIB since they 
**			are neeed by QBFLIB and this was causing problems on VMS.
**	27-dec-89 (sylviap)
**		Took out extra libraries in NEEDLIBS.
**	1/16/90 (martym)
**		Added GNLIB.
**	09/05/90 (dkh) - Integrated round 3 of MACWS changes.
**      10/05/90 (stevet) - Added IIUGinit call to initialize character set
**                          attribute table.
**	7-oct-1992 (rdrane)
**		Use new constants in (s/r)_reset invocations.
**	30-oct-1992 (rdrane)
**		Move extern declarations into the module itself.
**	16-nov-1992 (larrym)
**		Added CUFLIB to NEEDLIBS.  CUF is the new common utility
**		facility.
**	15-apr-1993 (rdrane)
**		Added MTLIB to NEEDLIBS.  The "improved" r_exit() processing
**		requires it to effect a reset of the terminal.
**	03/25/94 (dkh) - Added call to EXsetclient().
**	16-jun-95 (emmag)
**	    Define main as ii_user_main for NT.
**	24-sep-95 (tutto01)
**	    Restored main, run as console app on NT.
**	17-dec-1996 (angusm)
**          Extend II_4GL_DECIMAL for use with RBF (bug 75576).	
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
**	28-Mar-2005 (lakvi01)
**	    Added include of rglob.h and ug.h.
*/

/*
**	MKMFIN Hints
PROGRAM =	rbf

NEEDLIBS =	RBFLIB RFVIFREDLIB COPYREPLIB SREPORTLIB \
		REPORTLIB QBFLIB TBLUTILLIB GNLIB SHFRAMELIB SHQLIB \
		SHCOMPATLIB

NEEDLIBSW = 	SHEMBEDLIB SHADFLIB

	The "rF..." UNDEFS are needed because of calls from RFVIFREDLIB
	back up to RBFLIB.  "VFfeature" is in RFVIFREDLIB but called by VTLIB.
	"s_readin" is needed because of circular references between
	REPORTLIB and SREPORTLIB.

UNDEFS =	rFcoptions rFeditlayout rFwrite rFroptions VFfeature \
		rFcols rFagg FDrngchk II_copyright s_readin
*/

# ifdef	DGC_AOS
	FUNC_EXTERN	STATUS	FTtermsize();
# endif

	/* external declarations */

	FUNC_EXTERN	ADF_CB	*FEadfcb();
	GLOBALREF	bool	RBF;		/* used in VIFRED */

main(argc,argv)
i4	argc;		/* number of parameters */
char	*argv[];	/* addresses of parameter strings */
{
	char		*initstr;
	char		cp[2];
	i4		widthjunk;	/* only required as function param */
	i4		linesjunk;	/* only required as function param */
	DB_STATUS	status;

	/* Tell EX this is an ingres tool. */
	(void) EXsetclient(EX_INGRES_TOOL);

	/* Use the ingres allocator instead of malloc/free default (daveb) */
	MEadvise( ME_INGRES_ALLOC );

	/* Call IIUGinit to initialize character set attribute table */
	if ( IIUGinit() != OK)
	{
	    PCexit(FAIL);
	}

# ifdef	DGC_AOS
	if (FTtermsize(&widthjunk, &linesjunk, &initstr) == OK)
	{
		SIprintf(ERx("%s"), initstr);
	}
 	/* else, do nothing--error will be found on regular terminal startup */
# endif

	/*	Setup Reset level 4 and 5 tags */

	Rst4_tag = FEgettag();
	Rst5_tag = FEgettag();

	/* start up abstract datatype facility (ADF) */
	Adf_scb = FEadfcb();
        /* Overwrite II_DECIMAL setting with II_4GL_DECIMAL (bug 75576)*/
	cp[0]=cp[1]='\0';
	status = IIUGlocal_decimal(Adf_scb, cp);
	if (status)
	{
		IIUGerr(E_RW1417_BadDecimal, 0, 1, cp);
		return(status);
	}

	/* set query language of control block to QUEL in order to allow
      	** the aggregates any, sumu, countu, and avgu, which are not allowed
	** in SQL.
	*/
	Adf_scb->adf_qlang = DB_QUEL;

	/* start of MAIN routine */

	r_reset(RP_RESET_PGM,RP_RESET_DB,RP_RESET_CALL,
		RP_RESET_RELMEM4,RP_RESET_RELMEM5,
		RP_RESET_OUTPUT_FILE,RP_RESET_LIST_END);
	s_reset(RP_RESET_PGM,RP_RESET_SRC_FILE,RP_RESET_REPORT,
		RP_RESET_LIST_END);
	rFreset(RP_RESET_PGM,RP_RESET_LIST_END);

	St_called = FALSE;
	En_program = PRG_RBF;
	RBF = TRUE;

# ifdef DATAVIEW
	IIMWomwOverrideMW();
# endif /* DATAVIEW */

	PCexit(rFrbf(argc,argv));		/* runs RBF and exits */
}
