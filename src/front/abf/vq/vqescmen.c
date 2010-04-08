/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include       <er.h>
# include       <st.h>
# include       <me.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include       <abclass.h>
# include       <metafrm.h>
# include       "ervq.h"
# include       "vqescinf.h"


/**
** Name:	vqescmen.c - Get menuitem to edit escape code for
**
** Description:
**	This file defines:
**
**	IIVQgemGetEscapeMenuitem	- Get menuitem to edit escape code for
**	IIVQpmePurgeMenuitemEscapes	- Purge escsape code for nonexistent 
**					  menuitems
**
** History:
**	8/4/89 (Mike S)	Initial version
**	7/91 (Mike S) Allocate enough space for itemlist (Bug 37207)
**	10-sep-93 (dkhor)
**		Don't cast itemlist to i4 when calling MEcopy() in routine 
**		IIVQgemGetEscapeMenuitem(), since it is a pointer, cast to i4 
**		will result in truncation of addresses on 64-bit platform 
**		such as Alpha OSF/1. 
**      12-oct-93 (daver)
**              Casted parameters to MEcopy() to avoid compiler warnings
**              when this CL routine became prototyped.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
**/

/* # define's */
/* GLOBALDEF's */
/* extern's */
FUNC_EXTERN i4		FTspace();
FUNC_EXTERN i4		IIFDlpListPick();
FUNC_EXTERN VOID	IIUGbmaBadMemoryAllocation();
FUNC_EXTERN i4		IIUGmllMaxLineLen();
FUNC_EXTERN VOID	IIUGqsort();
FUNC_EXTERN VOID	IIVQeecEditEscapeCode();
FUNC_EXTERN VOID	IIVQgelGetEscLimits();
FUNC_EXTERN VOID	IIVQpmePurgeMenuitemEscapes();

/* static's */
static i4	cmp_menuitem();
static VOID	gem_lphandler();
static VOID	sort_menuitems();

static char	_semistarnl[] = 	ERx(";*\n");
static char	_semispacenl[] = 	ERx("; \n");
static char	routine[] = 	ERx("IIVQgemGetEscapeMenuitem");

static char	*itemlist;		/* menuitem listpick string */
static MFMENU	*menus[MF_MAXMENUS];	/* sorted menu list */
static char	*helptitle = NULL;	/* Help screen title */
static i4  	colsize;		/* size of menuitem tablefield column */

/*{
** Name:	IIVQgemGetEscapeMenuitem  - Get menuitem to edit escape code for
**
** Description:
**	The user has chosen to edit escape code for a menuitem.  We
**	present him a list of menuitems to pick from.  In addition, we 
**	take the opportunity to remove any escape code for non-existent
**	menuitems.
**
** Inputs:
**      escinfo         ESCINFO *       escape editing information
**              metaframe               metaframe to edit
**              flags                   form and tablefield state 
**              esctype                 type of menuitem activation 
**
** Outputs:
**      escinfo         ESCINFO *       escape editing information
**              flags                   form and tablefield state
**              item                    set by listpick handler 
**
**	Returns:
**		none
**
**	Exceptions:
**		none
**
** Side Effects:
**		none
**
** History:
**	8/4/89 (Mike S)	Initial version
**      12-oct-93 (daver)
**              Casted parameters to MEcopy() to avoid compiler warnings
**              when this CL routine became prototyped.
*/
VOID	
IIVQgemGetEscapeMenuitem(escinfo)
ESCINFO	*escinfo;
{
	i4	i;
	MFESC	**escs = escinfo->metaframe->escs;
	i4	firstesc, lastesc;
	char	*title = ERget(F_VQ00EA_Select_Esc_Menuitem);

	/*
	** If we haven't already been through here, sort 
	** menuitems, and allocate listpick string.
	*/
	if ((escinfo->flags & VQESC_MS) == 0)
	{
		i4 	totalsize;	/* size of listpick string */
		i4	indlength;	/* size of indicator string */

		/* Sort the menuitems into the "menus" array */
		sort_menuitems(escinfo->metaframe);
		escinfo->flags |= VQESC_MS;
			
		/* Build the list of menuitems */
		colsize = IIUGmllMaxLineLen(title) - 2 - 2*FTspace();

		for (i = 0; i < escinfo->metaframe->nummenus; i++)
		{
			i4 itemsize = STlength(menus[i]->text);
			/* 
			** Find the size of the menuitem column.  It must be
			** as big as the longest menuitem, and long enough so
			** that default form generation won't widen the
			** indicator column.
			*/
			colsize = max(colsize, itemsize);
		}

		indlength = max(sizeof(_semispacenl), sizeof(_semistarnl)) - 1;
		totalsize = 
		    escinfo->metaframe->nummenus * (colsize + indlength) + 1;
		itemlist = (char *)FEreqmem(escinfo->metaframe->ptag[MF_PT_ESC],
					    (u_i4)(totalsize), FALSE, NULL);
		if (itemlist == NULL)
		{
			IIUGbmaBadMemoryAllocation(routine);
		}
	}

	/* Get the index limits for this escape type */
	IIVQgelGetEscLimits(escinfo->metaframe, escinfo->esctype, 
				    &firstesc, &lastesc);

	for (i = 0; i < escinfo->metaframe->nummenus; i++)
	{
		/* Put in '*' indicator if escape code exists */
		bool matched;
		i4  j;

		matched = FALSE;
		for (j = firstesc; j <= lastesc; j++)
		{
			if (STcompare(escs[j]->item, menus[i]->text) 
				== 0)
			{
				matched = TRUE;
				break;
			}
		}

		/* If this is the first one, fill it to its full width */
		if (0 == i)
		{
			MEfill((i4)colsize, ' ', itemlist);
			itemlist[colsize] = EOS;
			MEcopy((PTR)(menus[i]->text), 
				(u_i2)STlength(menus[i]->text), (PTR)itemlist);
		}
		else
		{
			STcat(itemlist, menus[i]->text);
		}
		if (matched)
			STcat(itemlist, _semistarnl);
		else
			STcat(itemlist, _semispacenl);
	}

	/* Do the list pick. */
	if (helptitle == NULL)
		helptitle = STalloc(ERget(S_VQ0005_Select_Esc_Menuitem));
	_VOID_ IIFDlpListPick(title,
			      itemlist, 0, -1, -1, 
			      helptitle, ERx("vqescmnu.hlp"),
			      gem_lphandler, (PTR)escinfo);
	return;
}

/*{
** Name: IIVQpmePurgeMenuitemEscapes	- Purge escsape code for nonexistent 
**					  menuitems
**
** Description:
** Here, we merely set the escape type to an illegally large value.  This 
** causes it to be deleted before being saved to the database.  
**
** It is assumed that the escape codes are already in memory.
**
** Inputs:
**	metaframe	METRAFRAME *	Metaframe to purge menuitem escapes for
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
**	none
**
** History:
**	14 aug 89 (Mike S)	Initial version
*/
VOID
IIVQpmePurgeMenuitemEscapes(metaframe)
METAFRAME	*metaframe;	/* Metaframe to edit */
{
	i4 	i, j;		/* Loop indices */
	i4 	purge = 0;	/* Number to purge */
	bool 	match;		/* The escape code matched a menuitem */
	i4	strcomp;	/* String comparison value */

	/* Sort the menuitems into the "menus" array */
	sort_menuitems(metaframe);

	/* Loop through escape code */
	for (i = 0; i < metaframe->numescs; i++)
	{
		register MFESC *escptr = metaframe->escs[i];

		/* Check menu-type escape code */
		if ((escptr->type == ESC_MENU_START) || 
		    (escptr->type == ESC_MENU_END))
		{
			/* See if the menuitem exists */
			for (j = 0, match = FALSE; j < metaframe->nummenus; j++)
			{
				strcomp = STcompare(escptr->item, 
						    menus[j]->text);
				if (strcomp == 0)
				{
					match = TRUE;
					break;
				}
				else if (strcomp < 0)
				{
					break;
				}

			}
			/* If not, mark it so we can delete it */
			if (!match)
			{
				escptr->type = NUMESCTYPES + 1;
				purge++;
			}
		}
	}

	 /* See if any were changed */
	 if (purge > 0)
		metaframe->updmask |= MF_P_ESC;
}

/*{
** Name:        gem_lphandler - Handle menuitem listpick
**
** Description:
**      If we got a menuitem, go edit escape code.
**
** Inputs:
**      choice          i4      The user's choice number.
**		
** Outputs:
**      data            PTR     escape editing information
**      resume          bool *  Whether to resume listpick
**
**      Returns:
**              none
**
**      Exceptions:
**              none
**
** Side Effects:
**              none
**
** History:
**      8/4/89 (Mike S) Initial version
*/
static VOID
gem_lphandler(data, choice, resume)

PTR     data;
i4      choice;
bool    *resume;
{
        ESCINFO *escinfo = (ESCINFO *)data;     /* cast this to oringinal type*/ 
	*resume = FALSE;		/* Always fall back to first listpick */
        if (choice < 0)
                return;
 
        /* Edit escape code */
	escinfo->item = menus[choice]->text;
        IIVQeecEditEscapeCode(escinfo);
}

/*
** 	sort_menuitems
**
**	Copy the menuitmes to the static "menus" array, and sort them
**	alphabetically.
*/
static VOID 
sort_menuitems(metaframe)
METAFRAME	*metaframe;
{
	i4 i;

	/* Copy the menuitem pointers to the "menus" array */
	for (i = 0; i < metaframe->nummenus; i++)
		menus[i] = metaframe->menus[i];

	/* Sort the menuitems (acutally sort the pointers) */
	if (metaframe->nummenus > 1)
		IIUGqsort((char *)menus, metaframe->nummenus, 
	  	  	  sizeof(MFMENU *), cmp_menuitem);

}

/*
** Compare menuitem pointers by comparing their item fields.
*/
static i4
cmp_menuitem(itm1, itm2)
char *itm1;
char *itm2;
{
	MFMENU *mnu1 = *(MFMENU **)itm1;
	MFMENU *mnu2 = *(MFMENU **)itm2;

	return (STcompare(mnu1->text, mnu2->text));
}

