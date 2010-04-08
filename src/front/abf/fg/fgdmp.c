/*
**	Copyright (c) 1989, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<lo.h>
# include	<si.h>
# include	<ooclass.h>
# include	<metafrm.h>
# include	"framegen.h"


/**
** Name:	fgdmp.c - dump out a METAFRAME structure.
**
** Description:
**	This file defines:
**		IIFGdmp_DumpMF		entry point. call this.
**		fgdmp_menus
**		fgdmp_args
**		fgdmp_tabs
**		fgdmp_cols
**		fgdmp_joins
**		fgdmp_escs
**		fgdmp_vars
**
** History:
**	6/9/89 (pete)	Written.
**	9-sept-92 (blaise)
**		Added new "vartype" element to MFVAR structure.
**	11-Feb-93 (fredb)
**	   Porting change for hp9_mpe: SIfopen is required for additional
**	   control of file characteristics in IIFGdmp_DumpMF.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */

/* GLOBALDEF's */
static LOCATION dmploc;
static FILE  *dmpfile;

/* extern's */
/* static's */
static fgdmp_menus();
static fgdmp_args();
static fgdmp_tabs();
static fgdmp_cols();
static fgdmp_joins();
static fgdmp_escs();
static fgdmp_vars();


IIFGdmp_DumpMF (mf, fn)
METAFRAME *mf;	/* metaframe to dump */
char *fn;	/* file name to write dump to */
{
	char locbuf[MAX_LOC+1];

	STcopy (fn, locbuf);
	LOfroms (FILENAME&PATH, locbuf, &dmploc);

#ifdef hp9_mpe
	if (SIfopen (&dmploc, ERx("w"), SI_TXT, 252, &dmpfile) != OK)
#else
	if (SIopen (&dmploc, ERx("w"), &dmpfile) != OK)
#endif
	{
	    SIprintf(ERx("Error: unable to open METAFRAME dump file: %s"), fn);
	    return;
	}

	SIfprintf (dmpfile, ERx("\n**METAFRAME:\n"));

	SIfprintf (dmpfile, ERx("(&METAFRAME=%d) {},,popmask=%d, updmask=%d, flags=x%x,\n\tmode=%d, &apobj=%d,\n\tnummenus=%d, &menus=%d,\n"),
		mf, mf->popmask, mf->updmask, mf->flags, mf->mode, mf->apobj,
		mf->nummenus, mf->menus);

	SIfprintf (dmpfile, ERx("\tnumtabs=%d, &tabs=%d,\n\tnumjoins=%d, &joins=%d,\n\tnumescs=%d, &escs=%d,\n\tnumvars=%d, &vars=%d,\n\tstate=%d, gendate=%s, formgendate=%s.\n"),
		mf->numtabs, mf->tabs,
		mf->numjoins, mf->joins,
		mf->numescs, mf->escs,
		mf->numvars, mf->vars,
		mf->state,
		mf->gendate,
		mf->formgendate);

	fgdmp_menus (mf->nummenus, mf->menus);
	fgdmp_tabs (mf->numtabs, mf->tabs);
	fgdmp_joins (mf->numjoins, mf->joins);
	fgdmp_escs (mf->numescs, mf->escs);
	fgdmp_vars (mf->numvars, mf->vars);

	if (dmpfile != NULL)
	    SIclose(dmpfile);	
}

static
fgdmp_menus (nummenus, p_menus)
i4	nummenus;
MFMENU **p_menus;
{
	i4 i;

	SIfprintf (dmpfile, ERx("\n**MFMENU - %d entries:\n"), nummenus);

	for (i=0; i<nummenus; i++, p_menus++)
	{
	    SIfprintf (dmpfile, ERx("\n(&MFMENU=%d) text=%s, &apobj=%d, numargs=%d, &args=%d."),
		(*p_menus), (*p_menus)->text, (*p_menus)->apobj,
		(*p_menus)->numargs, (*p_menus)->args );

	    fgdmp_args ((*p_menus)->numargs, (*p_menus)->args);
	}
}

static
fgdmp_args (numargs, p_args)
i4	numargs;
MFCFARG **p_args;
{
	i4 i;

	SIfprintf (dmpfile, ERx("\n\tMFCFARG - %d entries:\n"), numargs);

	for (i=0; i<numargs; i++, p_args++)
	{
	    SIfprintf (dmpfile, ERx("\t(&MFCFARG=%d) var=%s, expr=%s.\n"),
		(*p_args), (*p_args)->var, (*p_args)->expr );
	}
}

static
fgdmp_tabs (numtabs, p_tabs)
i4	numtabs;
MFTAB **p_tabs;
{
	i4 i;

	SIfprintf (dmpfile, ERx("\n**MFTAB - %d entries:\n"), numtabs);

	for (i=0; i<numtabs; i++, p_tabs++)
	{
	    SIfprintf (dmpfile, ERx("\n(&MFTAB=%d) name=%s, utility=%s, tabsect=%d, usage=%d, flags=x%x, numcols=%d, &cols=%d."),
		(*p_tabs), (*p_tabs)->name, (*p_tabs)->utility,
		(*p_tabs)->tabsect, (*p_tabs)->usage, (*p_tabs)->flags,
		(*p_tabs)->numcols, (*p_tabs)->cols);

	    fgdmp_cols ((*p_tabs)->numcols, (*p_tabs)->cols);
	}
}

static
fgdmp_cols (numcols, p_cols)
i4	numcols;
MFCOL **p_cols;
{
	i4 i;

	SIfprintf (dmpfile, ERx("\n\tMFCOL - %d entries:\n"), numcols);

	for (i=0; i<numcols; i++, p_cols++)
	{
	    SIfprintf (dmpfile,ERx("\t(&MFCOL=%d) name=%s, alias=%s, utility=%s,\n\t\t{,type.db_length=%d, type.db_datatype=%d,},\n\t\tflags=x%x, corder=%d, info=%s.\n"),
		(*p_cols), (*p_cols)->name, (*p_cols)->alias,
		(*p_cols)->utility,
		(*p_cols)->type.db_length, (*p_cols)->type.db_datatype,
		(*p_cols)->flags, (*p_cols)->corder, (*p_cols)->info);
	}
}

static
fgdmp_joins (numjoins, p_joins)
i4	numjoins;
MFJOIN **p_joins;
{
	i4 i;

	SIfprintf (dmpfile, ERx("\n**MFJOIN - %d entries:\n"), numjoins);

	for (i=0; i<numjoins; i++, p_joins++)
	{
	    SIfprintf (dmpfile, ERx("(&MFJOIN=%d) type=%d, tab_1=%d, tab_2=%d, col_1=%d, col_2=%d.\n"), (*p_joins),
	    	(*p_joins)->type,
		(*p_joins)->tab_1, (*p_joins)->tab_2,
		(*p_joins)->col_1, (*p_joins)->col_2 );
	}
}

static
fgdmp_escs (numescs, p_escs)
i4	numescs;
MFESC **p_escs;
{
	i4 i;

	SIfprintf (dmpfile, ERx("\n**MFESC - %d entries:\n"), numescs);

	for (i=0; i<numescs; i++, p_escs++)
	{
	    SIfprintf (dmpfile, ERx("(&MFESC=%d) type=%d, tabsect=%d, item=%s,\ntext=%s.\n"),
		(*p_escs),
	    	(*p_escs)->type, (*p_escs)->tabsect,
		(*p_escs)->item, (*p_escs)->text );
	}
}

static
fgdmp_vars (numvars, p_vars)
i4	numvars;
MFVAR **p_vars;
{
	i4 i;

	SIfprintf (dmpfile, ERx("\n**MFVAR - %d entries:\n"), numvars);

	for (i=0; i<numvars; i++, p_vars++)
	{
	    SIfprintf (dmpfile,
		ERx("(&MFVAR=%d) name=%s, vartype=%d, dtype=%s, comment=%s.\n"),
		(*p_vars), (*p_vars)->name, (*p_vars)->vartype,
		(*p_vars)->dtype, (*p_vars)->comment );
	}
}
