/*
**	Copyright (c) 1989, 2004 Ingres Corporation
**	All rights reserved.
*/

# include <compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include <cm.h>
# include <st.h>
# include <ol.h>
# include <nm.h>
# include <er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include <fe.h>
# include <abclass.h>
# include <metafrm.h>
# include <oodep.h>
# include <erqg.h>
# include <qg.h>
# include <mqtypes.h>
# include <ilerror.h>

/**
** Name:	iammeta.c -	Metaframe construction utilities
**
** Description:
**
**	Routines used by IIAMapFetch to construct structures from
**	catalogs.
**
** 	IIAMigdInitGenDates - Initialize gendate and formgen date
** :	IIAMamAttachMeta() - attach a metaframe.
**
** History:
**	4/89 (bobm)	written.
**	7/90 (Mike S)	Removed ABF build routines; now done in iampaget.c
**	03/09/91 (emerson)
**		Integrated DESKTOP porting changes.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

static PTR loc_alloc();
static char *loc_s_alloc();

/*{
** Name:	IIAMigdInitGenDates - Initialize gendate and formgen date
**
** Description:
**	Translate abf_arg4 and abf_arg5 catalog columns to gendat, formgendate,
**	and state field of metaframe structure.
**
** Inputs:
**	frm	USER_FRAME *	user_frame structure
**	formgen	char *		abf_arg4 column
**	statcol	char *		abf_arg5 column
**
** Outputs:
**	none	
**
**	Returns:
**		none
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	4/6/90 (Mike S) Initial version
*/
VOID
IIAMigdInitGenDates(frm, formgen, statcol)
USER_FRAME *frm;
char 	*formgen;
char 	*statcol;
{
	METAFRAME *m = frm->mf;
	char	*datepart;

	if ((datepart = STindex(formgen,"@", 0)) != NULL)
	{
		++datepart;
		formgen = datepart;
	}

	/*
	** strip the leading state information from the
	** column data.  The rest of it is the generation
	** date.  If there is no leading part, or it is
	** non-numeric, just set state 0.  The latter
	** really shouldn't happen, but we set 0 for the
	** sake of consistent behaviour.
	*/
	if ((datepart = STindex(statcol,"@", 0)) != NULL)
	{
		*datepart = EOS;
		++datepart;
		if (CVan(statcol,&(m->state)) != OK)
			m->state = 0;
	}
	else
	{
		datepart = statcol;
		m->state = 0;
	}
	if (*datepart == '<')
		*datepart = EOS;
	if (*formgen == '<')
		*formgen = EOS;
	m->gendate = loc_s_alloc(datepart, frm->data.tag);
	m->formgendate = loc_s_alloc(formgen, frm->data.tag);
}

/*{
** Name:	IIAMamAttachMeta() - attach a metaframe.
**
** Description:
**	Attach a metaframe structure to a user frame.
**
** Inputs:
**	app	application
**	frm	Frame object.
**	statcol	Status column from catalog.
**
** History:
**	4/89 (bobm)	written
*/
VOID
IIAMamAttachMeta(frm)
USER_FRAME *frm;
{
	METAFRAME *m;
	TAGID tag;

	tag = frm->data.tag;

	/*
	** We get zeroed memory for new metaframe.  This assures NULL
	** pointers, zero counts and zeroed masks.
	*/
	m = (METAFRAME *) loc_alloc(sizeof(METAFRAME),tag,TRUE);

	/*
	** allocate the fixed-length arrays.
	*/
	m->menus = (MFMENU **)
		loc_alloc(sizeof(MFMENU *)*MF_MAXMENUS,tag,FALSE);

	m->tabs = NULL;
	m->joins = NULL;
	m->escs = NULL;
	m->vars = NULL;

	frm->mf = (METAFRAME *) m;
	m->apobj = (OO_OBJECT *) frm;

	m->gendate = ERx("");
}

/*
** local covers to allocation to avoid checking for success
** all over the place.
*/
static PTR
loc_alloc(size,tag,zeroflag)
i4  size;
TAGID tag;
bool zeroflag;
{
	register PTR p;

	p = FEreqmem(tag,size,zeroflag,(STATUS *) NULL);
	if (p == NULL)
		IIUGbmaBadMemoryAllocation(ERx("IIAMamAttachMeta"));
	return (p);
}

static char *
loc_s_alloc(s,tag)
char *s;
TAGID tag;
{
	register char *sp;

	sp = (char *) loc_alloc(STlength(s)+1,tag,FALSE);
	STcopy(s,sp);
	return (sp);
}

