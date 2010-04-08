/*
**  Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"
# include	<er.h>

/*
**  wrfrm->frm.c
**  write out the frm->frame's def
**
**  History:
**	87/04/07  (joe) Added compat, dbms and fe.h
**	86/12/16  (peter) Add changes to support the COMPFORM command.
**			  If the -c flag is set (fcrti),
**			then add the GLOBALDEF header.
**	86/11/18  (dave) Initial60 edit of files
**	86/01/29  (seiwald)
**		Now calls DSwrite with sizeof() the argument instead of
**		the (wrong) magic cookie 13.  (Bug #6630)
**	03/03/87 (dkh) - Added support for ADTs.
**	08/14/87 (dkh) - ER changes.
**	09/03/88 (dkh) - Enlarged buffers for storing the form name.
**	89-jun-05 (sylviap)
**		For DG, will print out framename in both upper and lowercase.
**		DG COBOL will look for lowercase and DG FORTRAN will look for
**		uppercase.
**	12/31/89 (dkh) - Integrated IBM porting changes for re-entrancy.
**	08/17/90 (dkh) - Fixed bug 21500.
**	09/27/90 (dkh) - Fixed bug 33537.
**	03/01/93 (dkh) - Fixed bug 49951.  Updated calls to DSwrite to
**			 handle interface change for type DSD_STR.
**	03/01/93 (dkh) - Fixed problem where a garbage value for frmode
**			 was being written out if the form was retrieved
**			 from the ii_encoded_forms catalog.
**	07/07/93 (dkh) - Fixed bug 41090.  Changed label generation scheme
**			 for table field columns to remove possibility of
**			 creating duplicate labels.
**			 Also removed useless ifdefs.
**	08/03/93 (dkh) - Changed the code generated for internal use to no
**			 longer depend on frame61.h.  This makes building
**			 a release simpler.
**	08/12/93 (dkh) - Fixed bug 53856.  DSwrite handles strings differently
**			 depending on whether the language is C or macro.
**			 So we pass different lengths, depending on the
**			 language, to accomodate the difference.
**	22-jan-1998 (fucch01)
**	    Casted 2nd arg's of DSwrite calls as type PTR (equivalent
**	    to char *), to match the parameter type and get rid of 
**	    errors preventing compilation on sgi_us5.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-sep-2002 (abbjo03)
**	    Correct change of 3-aug-93 which did not take into consideration
**	    that on VMS compform defaults to Macro assembler output.
*/

/*
**  Maximum identifier size for assembler on VMS.
*/
# define	CF_MAX_VMS_IDENT		31

static	i4	frmcnt = 0;

fcwrFrm(frm, starttr, startfd, startns)
register FRAME	*frm;
i4		starttr,
		startfd,
		startns;
{
	char	*name;
	char	up_name[FE_MAXNAME + 1];
	char	buf[100];
	i4	str_size;

	fcwrArr(ERx("TRIM"), 't', (i4) 0, starttr, frm->frtrimno, frm->frname,
		(i4) FALSE);
	if (fcrti == TRUE)
	{
		fcwrArr(ERx("IFIELD"), 'f', (i4) 0, startfd, frm->frfldno,
			frm->frname, (i4) FALSE);
		fcwrArr(ERx("IFIELD"), 'n', (i4) 0, startns, frm->frnsno,
			frm->frname, (i4) FALSE);
	}
	else
	{
		fcwrArr(ERx("FIELD"), 'f', (i4) 0, startfd, frm->frfldno,
			frm->frname, (i4) FALSE);
		fcwrArr(ERx("FIELD"), 'n', (i4) 0, startns, frm->frnsno,
			frm->frname, (i4) FALSE);
	}

	STprintf(buf, ERx("_form%d"), frmcnt);

	DSinit(fcoutfp, fclang, DSA_C, buf, DSV_LOCAL, ERx("FRAME"));

	if (fclang == DSL_C)
	{
		str_size = STlength(frm->frfiller) + 1;
	}
	else
	{
		str_size = sizeof(frm->frfiller);
	}
	DSwrite(DSD_STR, frm->frfiller, str_size);
	DSwrite(DSD_I4, (PTR)frm->frversion, 0);

	if (fclang == DSL_C)
	{
		str_size = STlength(frm->frname) + 1;
	}
	else
	{
		str_size = sizeof(frm->frname);
	}
	DSwrite(DSD_STR, frm->frname, str_size);

	if (fcrti == TRUE && fclang == DSL_C)
	{
		STprintf(buf, ERx("(FIELD **) _af0%d"), startfd);
	}
	else
	{
		STprintf(buf, ERx("_af0%d"), startfd);
	}
	DSwrite(DSD_ARYADR, buf, 0);				/* frfld */
	DSwrite(DSD_I4, (PTR)frm->frfldno, 0);
	if (fcrti == TRUE && fclang == DSL_C)
	{
		STprintf(buf, ERx("(FIELD **) _an0%d"), startns);
	}
	else
	{
		STprintf(buf, ERx("_an0%d"), startns);
	}
	DSwrite(DSD_ARYADR, buf, 0);				/* frnsfld */
	DSwrite(DSD_I4, (PTR)frm->frnsno, 0);
	STprintf(buf, ERx("_at0%d"), starttr);

	/*
	**  Clear out frm->frmode to avoid picking up runtime
	**  garbage if the form is from the ii_encoded_forms catalog.
	*/
	frm->frmode = 0;

	DSwrite(DSD_ARYADR, buf, 0);				/* frtrim */
	DSwrite(DSD_I4, (PTR)frm->frtrimno, 0);
	DSwrite(DSD_I4, 0, 0);					/* frscr */
	DSwrite(DSD_I4, 0, 0);					/* unscr */
	DSwrite(DSD_I4, 0, 0);					/* untwo */
	DSwrite(DSD_I4, 0, 0);					/* frrngtag */
	DSwrite(DSD_I4, 0, 0);					/* frrngptr */
	DSwrite(DSD_I4, (PTR)frm->frintrp, 0);
	DSwrite(DSD_I4, (PTR)frm->frmaxx, 0);
	DSwrite(DSD_I4, (PTR)frm->frmaxy, 0);
	DSwrite(DSD_I4, (PTR)frm->frposx, 0);
	DSwrite(DSD_I4, (PTR)frm->frposy, 0);
	DSwrite(DSD_I4, (PTR)frm->frmode, 0);
	DSwrite(DSD_I4, (PTR)frm->frchange, 0);
	DSwrite(DSD_I4, (PTR)frm->frnumber, 0);
	DSwrite(DSD_I4, (PTR)frm->frcurfld, 0);
	DSwrite(DSD_I4, frm->frcurnm, 0);
	DSwrite(DSD_I4, (PTR)frm->frmflags, 0);
	DSwrite(DSD_I4, (PTR)frm->frm2flags, 0);
	DSwrite(DSD_I4, 0, 0);					/* frres1 */
	DSwrite(DSD_I4, 0, 0);					/* frres2 */
	DSwrite(DSD_I4, 0, 0);					/* frfuture */
	DSwrite(DSD_I4, (PTR)frm->frscrtype, 0);
	DSwrite(DSD_I4, (PTR)frm->frscrxmax, 0);
	DSwrite(DSD_I4, (PTR)frm->frscrymax, 0);
	DSwrite(DSD_I4, (PTR)frm->frscrxdpi, 0);
	DSwrite(DSD_I4, (PTR)frm->frscrydpi, 0);
	DSwrite(DSD_I4, (PTR)frm->frtag, 0);
	DSfinal();
	if (fcname == NULL)
		name = frm->frname;
	else
		name = fcname;

	/*
	**  If outputting macro for VMS, then make sure we
	**  don't exceed max identifier size.
	*/
	if (fclang == DSL_MACRO)
	{
		STlcopy(name, buf, CF_MAX_VMS_IDENT);
		name = buf;
	}

	if (fcrti == TRUE)
	{	/* RT internal cl uses GLOBALDEF */
		DSinit(fcoutfp, fclang, DSA_C, name, DSV_GLOB, 
			ERx("GLOBALDEF\tFRAME *"));
	}
	else
	{	/* for all others */
		DSinit(fcoutfp, fclang, DSA_C, name, DSV_GLOB, ERx("FRAME *"));
	}

	STprintf(buf, ERx("_form%d"), frmcnt);

	DSwrite(DSD_ADDR, buf, 0);
	DSfinal();
# ifdef DGC_AOS
	/*
	** DG ESQL/FORTRAN looks for an uppercase framename, so emit both
	** upper and lowercase names.
	*/
	STcopy (name, up_name);
	CVupper (up_name);
	DSinit(fcoutfp, fclang, DSA_C, up_name, DSV_GLOB, ERx("FRAME *"));
	STprintf(buf, ERx("_form%d"), frmcnt);
	DSwrite(DSD_ADDR, buf, 0);
	DSfinal();
# endif /* DGC_AOS */

	frmcnt++;
}
