# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"
# include	<lo.h>
# include	<nm.h>
# include	<ug.h>
# include	<er.h>
# include	"erfc.h"
# include	<frame.h>

/*
**  Copyright (c) 2004 Ingres Corporation
**
** compfrm.c -- compile a frame to a C include module
**
**  HISTORY:
**	86/12/16  (peter) Add changes to support the COMPFORM command.
**	86/11/18  (dave) Initial60 edit of files
**	85/11/04  (peter) Change extern to FUNC_EXTERN.
**	85/10/31  (peter) Change output to full pathname for frame2.h.
**	86/03/24  (bruceb)
**		if ATTIS (for now), produce compiled
**		form files that include <frame2.h> rather than
**		"[~ingres]/files/frame2.h".
**	85/12/12  (garyb) IBM porting changes.
**	85/10/09  (roger) ibs integration (CMS change, bug fix).
**	85/08/05   (wong) Changed to output full pathname for "frame2.h".
**	2/85	(ac)	- Added fcdbname varibale.
**	05/29/87 (dkh) - Changed to use frame60.h instead of frame2.h.
**	08/14/87 (dkh) - ER changes.
**	09/30/87 (dkh) - Changed header includes for VMS.
**	11/12/87 (dkh) - Code cleanup.
**	20-may-88 (bruceb)
**		Reference frame61.h for venus.
**	07/09/88 (dkh) - Fixed jup bug 2867.
**	10/21/88 (dkh) - Fixed venus bug 3708.
**	01/12/89 (wolf)- For CMS, just include '<frame61.h>'
**	09/21/89 (dkh) - Porting changes integration.
**	10/25/90 (dkh) - Fixed bug 34092.
**	02/20/93 (dkh) - Changed to run without a db connection.
**	06/05/93 (dkh) - Updated to generate include of gl.h, sl.h
**			 and iicommon.h instead of dbms.h when started
**			 with the -c internal flag.
**	08/03/93 (dkh) - Changed the code generated for internal use to no
**			 longer depend on frame61.h.  This makes building
**			 a release simpler.
**      24-sep-96 (hanch04)
**              Global data moved to crfmdata.c
*/

/* Error status returns */
# define	FC_FileError	-1

GLOBALREF FILE	*fcoutfp ;
GLOBALREF char	*fcoutfile ;
GLOBALREF char	*fcnames[NUMFORMS] ;	/* changed {0} to {0} -nml */
GLOBALREF char	**fcntable ;
GLOBALREF char	*fcname ;
GLOBALREF char	*fcdbname ;
GLOBALREF i4	fclang ;
GLOBALREF i4	fceqlcall ;
GLOBALREF FRAME *fcfrm ;
GLOBALREF bool	fcrti ;
GLOBALREF bool	fcstdalone ;
GLOBALREF char	*valstr ;
GLOBALREF i4	starttr ,
		startfd ,
		startns ;

GLOBALREF bool	fcfileopened ;

FUNC_EXTERN	FRAME	*FDfrcreate();
static			fcdoit();
static STATUS		fccrefrm();


STATUS
fcinit()
{
	char		*name;
	char		bufdev[64];
	char		bufpath[64];
	char		bufpref[64];
	char		bufsuf[64];
	char		bufver[64];
	char		locbfr[MAX_LOC + 1];
	LOCATION	loc;

	if (fcfileopened)
	{
		return(OK);
	}
	fcfileopened = TRUE;

	valstr = ERx("_v%s");	/* used for validation trees */
	if (fcoutfile != NULL)
	{
		STcopy(fcoutfile,locbfr);
		if (LOfroms(FILENAME & PATH, locbfr, &loc) != OK)
		{
			/*
			**  Fix for BUG 5458. (dkh)
			*/

			return((i4) FC_FileError);
		}
# ifdef hp9_mpe
		if (SIfopen(&loc, ERx("w"), SI_TXT, 256, &fcoutfp) != OK)
# else
		if (SIopen(&loc, ERx("w"), &fcoutfp) != OK)
# endif
		{
			return((i4) FC_FileError);
		}

		LOdetail(&loc, bufdev, bufpath, bufpref, bufsuf, bufver);
		name = bufpref;
	}
	else
	{
		name = ERx("ing_");
		fcoutfp = stdout;
	}
	DSbegin(fcoutfp, fclang, name);
	if (fclang == DSL_C)
	{ /* include FRAME definitions */
		LOCATION	path;
		char		*cp;

# ifdef ATTIS
		SIfprintf(fcoutfp, ERx("# include\t<frame61.h>\n\n"));
# else
# ifdef CMS
		SIfprintf(fcoutfp, ERx("# include\t<frame61.h>\n\n"));
# else
		if (fcrti == TRUE)
		{	/* Use bracketed path */
			SIfprintf(fcoutfp, ERx("# include\t<compat.h>\n"));
			SIfprintf(fcoutfp, ERx("# include\t<gl.h>\n"));
			SIfprintf(fcoutfp, ERx("# include\t<sl.h>\n"));
			SIfprintf(fcoutfp, ERx("# include\t<iicommon.h>\n"));
			SIfprintf(fcoutfp, ERx("# include\t<fe.h>\n"));
			SIfprintf(fcoutfp, ERx("# include\t<ft.h>\n"));
			SIfprintf(fcoutfp, ERx("# include\t<fmt.h>\n"));
			SIfprintf(fcoutfp, ERx("# include\t<adf.h>\n"));
			SIfprintf(fcoutfp, ERx("# include\t<frame.h>\n\n"));
			SIfprintf(fcoutfp, ERx("typedef struct ifldstr\n{\n"));
			SIfprintf(fcoutfp, ERx("\ti4\tfltag;\n"));
			SIfprintf(fcoutfp, ERx("\tint\t*fld;\n} IFIELD;\n\n"));
			SIfprintf(fcoutfp, ERx("# define\tCAST_PINT\t(int *)\n\n"));
		}
		else
		{
# ifdef VMS
		  SIfprintf(fcoutfp,
		   ERx("# include\t\"II_SYSTEM:[INGRES.FILES]frame61.h\"\n\n"));
# else
			NMloc(FILES, FILENAME, ERx("frame61.h"), &path);
			LOtos(&path, &cp);
			SIfprintf(fcoutfp, ERx("# include\t\"%s\"\n\n"), cp);
# endif /* VMS */
		}
# endif /* CMS */
# endif /* ATTIS */
	}
	return(OK);
}


STATUS
fccomfrm()
{
	STATUS	retval;
	char	**np;

	if (fcfrm != NULL)
	{
		if ((retval = fcinit()) != OK)
		{
			if (retval == FC_FileError)
			{
				IIUGerr(E_FC0006_bad_file, UG_ERR_ERROR,
					1, fcoutfile);
				return(OK);
			}
			return(retval);
		}
		fcdoit(fcfrm);
		if (fcrti && fcstdalone == TRUE)
		{
			IIUGmsg(ERget(S_FC0005_Written), (bool) FALSE, 2,
				*fcnames, fcoutfile);
		}
	}
	else
	{
		for (np = fcnames; *np != NULL; np++)
		{
			if ((retval = fccrefrm(*np)) == FC_FileError)
			{
				/*
				**  Put back solid lines if we are called
				**  from vifred.
				*/
				if (!fcstdalone)
				{
					FDfrprint(0);
				}
				IIUGerr(E_FC0006_bad_file, UG_ERR_ERROR, 1,
					fcoutfile);
				return(OK);
			}
			if (retval != OK)
			{	/* Could not find the form */
				if (fcstdalone != TRUE)
				{
					syserr(ERget(E_FC0001_can_t_build_fr),
						*np);
				}
				else
				{
					return(retval);
				}
			}
			else
			{	/* Form found and built */
				if (fcstdalone == TRUE)
				{
					IIUGmsg(ERget(S_FC0005_Written),
						(bool) FALSE, 2,
						*np, fcoutfile);
				}
			}
		}
	}
	DSend(fcoutfp, fclang);
	if (fcoutfile != NULL)
		SIclose(fcoutfp);
	return((i4) OK);
}

/*
** given the frame named np
** add the code for if to outfp
*/
static
STATUS
fccrefrm(name)
char	*name;
{
	STATUS	stat;
	FRAME	*frm;

	/* want only the amount of work done that vifred gets */
	FDsetparse(TRUE);
	if ((frm = FDfrcreate(name)) == NULL)
	{
		FDsetparse(FALSE);
		return(FAIL);
	}
	if ((stat = fcinit()) != OK)
	{
		FDsetparse(FALSE);
		return(stat);
	}
	fcdoit(frm);
	FDsetparse(FALSE);
	return(OK);
}

static
fcdoit(frm)
FRAME	*frm;
{
	if (fcstdalone == TRUE)
	{
		IIUGmsg(ERget(S_FC0002_Writing), (bool) FALSE,
			2, frm->frname, fcoutfile);
	}
	fcwrTrim(frm->frtrim, starttr, frm->frtrimno, frm->frname);
	fcwrField(frm->frfld, startfd, frm->frfldno, frm->frname);
	fcwrNonSeq(frm->frnsfld, startns, frm->frnsno, frm->frname);
	fcwrFrm(frm, starttr, startfd, startns);
	starttr += frm->frtrimno;
	startfd += frm->frfldno;
	startns += frm->frnsno;
}
