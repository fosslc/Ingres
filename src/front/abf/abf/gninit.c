/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<st.h>		 
#include	<pc.h>		 
#include	<lo.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	<gn.h>
#include	"gncode.h"
#include	"erab.h"

/**
** Name:	gninit.c -	ABF Name Generation Initialization.
**
** Description:
**	Contains the routine used to initialize the name generation interface
**	for an application when edited from ABF.  Defines:
**
**	iiabNamesInit()		initialize name generation for ABF.
**
** History:
**	Revision 6.0  87/05  bobm
**	Initial revision.

**	Revision 6.2  89/09  wong
**	Added separate section for file names in addition to the object-code
**	section.  JupBug #8106.
**
**	8-march-91 (blaise)
**	    Integrated PC porting changes.
**	16-aug-91 (leighb)
**	    Remove above PC porting changes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*{
** Name:	iiabNamesInit() -	Initialize Name Generation for ABF.
**
** Description:
**	Initializes GN name spaces used by ABF.	 On repeat calls
**	it removes the old name spaces from memory.
**
** History:
**	5/87 (bobm)	written
*/

/*
** length used for file names:
** Don't want it to be longer than 48, because of the size of the
** file name fields on the various forms.  If the allowable length
** on our system is SHORTER than this, use that instead.  In computing
** the allowable length, subtract 5 for extensions on systems with the
** extension counted as part of the file name ('.' plus 4 letter extension ).
*/

#ifdef CMS
#  define GF_LO_ALLOW	(LO_NM_LEN - 1)
#else
#  define GF_LO_ALLOW	( LO_EXT_LEN == 0 ? LO_NM_LEN - 5 : LO_NM_LEN )
#endif /* CMS */

#define GF_LEN		( GF_LO_ALLOW > 48 ? 48 : GF_LO_ALLOW )

#define FILE_LEN	LO_EXT_LEN + LO_NM_LEN

/*
** filename case rule.	If filenames are case sensitive on this system,
** use GN_SIGNIFICANT, otherwise use GN_LOWER
*/

#define GF_CRULE	( LO_NM_CASE ? GN_SIGNIFICANT : GN_MIXED )

struct SPACETABLE
{
	/* read only items */
	i4 gcode;	/* code for name space */
	i4 ulen;	/* uniqueness length */
	i4 tlen;	/* truncation length */
	i4 crule;	/* case rule */
	i4 arule;	/* accept rule */
	bool dopfx;	/* prefix with "II<code>" */

	/* modified at run time */
	bool created;	/* space exists */
	char pfx[6];	/* prefix */
};

static struct SPACETABLE	Sp_tab[] =
{
	{ AGN_OPROC, AGN_SULEN, AGN_STLEN, GN_MIXED, GNA_XUNDER, TRUE, FALSE },
	{ AGN_OFRAME, AGN_SULEN, AGN_STLEN, GN_MIXED, GNA_XUNDER, TRUE, FALSE },
	{ AGN_FORM, AGN_SULEN, AGN_STLEN, GN_MIXED, GNA_XUNDER, TRUE, FALSE },
	{ AGN_REC, AGN_SULEN, AGN_STLEN, GN_MIXED, GNA_XUNDER, TRUE, FALSE },
	{ AGN_SFILE, FILE_LEN, FILE_LEN, GF_CRULE, GNA_XPUNDER, FALSE, FALSE },
	{ AGN_OFILE, GF_LEN, GF_LEN, GF_CRULE, GNA_XPUNDER, FALSE, FALSE }
};

#define TABSIZE (sizeof(Sp_tab)/sizeof(Sp_tab[0]))

VOID
iiabNamesInit()
{
	register i4  i;
	register struct SPACETABLE *p;

	p = Sp_tab;
	for ( i = TABSIZE ; --i >= 0 ; ++p )
	{
		if (p->created)
			IIGNesEndSpace(p->gcode);
		if (!p->dopfx)
			(p->pfx)[0] = EOS;
		else
		{
			STprintf(p->pfx, ( p->gcode > 9 )
						? ERx("II%d") : ERx("II0%d"),
					p->gcode
			);
		}
		if (IIGNssStartSpace(p->gcode, p->pfx, p->ulen, p->tlen,
				p->crule, p->arule, (VOID (*)())NULL) != OK)
			IIUGerr(S_AB0026_Init_Spaces, UG_ERR_FATAL, 0);
		p->created = TRUE;
	}
}
