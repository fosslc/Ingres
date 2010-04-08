
/* 
**	Copyright (c) 2004 Ingres Corporation  
*/
#include	<compat.h> 
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h> 
#include 	<st.h>
#include 	<er.h>
#include 	<uf.h>
#include 	<ug.h>
#include 	<ui.h>
#include        <ooclass.h>
#include        <abclass.h>
#include	<metafrm.h>


/**
** Name:	vqalias -	make unique alias names for table columns
**
** Description:
**	Given a table from the visual query we will create alias
**	names for the columns if the names conflict with other
**	tables.
**
**	This file defines:
**		IIVQcanColAliasNames - as above
**
** History:
**	12/27/89  (tom) - extracted from vqdloop.qsc 
**	11-sep-92 (blaise)
**	    Support for delimited ids: when checking to see whether we need to
**	    create a new alias for a column, compare aliases, rather than
**	    column names, because column name and alias are no longer the same
**	    when the metaframe is first set up.
**	12-mar-93 (blaise)
**	    Now we have support for delimited ids in the dbms, fixed this
**	    code so it handles them correctly.
**	15-sep-93 (connie)
**	    The 3rd argument to IIUGdlm_ChkdlmBEobject should be FALSE.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

FUNC_EXTERN PTR IIVQ_VAlloc();

static VOID vq_DupName();
static VOID vq_SetAlias();


/*{
** Name:	IIVQcanColAliasNames - Create column alias names for a table 
**
** Description:
**	This function goes through a table's columns and creates alias names
**	for the column names such that simple fields of the form are unique,
**	and table field column names are unique.
**
** Inputs:
**	METAFRAME *mf;	- ptr to metaframe pointer
**	i4	idx;	- index of the table to create the alias names for
**
** Outputs:
**	Returns:
**		void
**	
**	Exceptions:
**
** Side Effects:
**
** History:
**	07/24/89 (tom) - created
*/
VOID 
IIVQcanColAliasNames(mf, idx)
METAFRAME *mf;
i4  idx;
{
	register MFTAB *tab = mf->tabs[idx];
	register i4  i;
	register i4  start;
	register i4  sect;

	start = IIVQptPrimaryTab(mf, sect = tab->tabsect);

	for (i = 0; i < tab->numcols; i++)
	{
		vq_DupName(mf, tab->cols[i], start, sect, idx);
	}
}

/*{
** Name:	vq_DupName - search the tables for duplicate names
**
** Description:
**	(Subroutine of the vq_ColAlias.)
**	This function goes through a table's columns and creates alias names
**	for the column names such that simple fields of the form are unique,
**	and table field column names are unique.
**
** Inputs:
**	METAFRAME *mf;	- ptr to metaframe pointer
**	MFCOL	col;	- column to consider an alias name for
**	i4	start;	- index of starting table to consider
**	i4	sect;	- section to be considered
**	i4	idx;	- index of the table that contains the column currently
**			  under consideration 
**
** Outputs:
**	Returns:
**		void
**	
**	Exceptions:
**
** Side Effects:
**
** History:
**	07/24/89 (tom) - created
*/
static VOID 
vq_DupName(mf, col, start, sect, idx)
METAFRAME *mf;
MFCOL *col;
i4  start;
i4  sect;
i4  idx;
{
	register i4  k;
	register MFCOL *c2;
	register i4  j;
	register MFTAB *t2;

	/* for all of the tables in the section */
	for (j = start; j < mf->numtabs; j++)
	{
		/* don't check the table against itself */
		if (j == idx)
		{
			continue;
		}

		t2 = mf->tabs[j];

		/* if we are not in the same section any more then
		   break out of the inner loop */
		if (t2->tabsect != sect)
		{
			break;
		}
		
		/* for all the columns of the table */
		for (k = 0; k < t2->numcols; k++)
		{
			c2 = t2->cols[k];

			/*
			** If the aliases are equal, then set a new alias name.
			** Those column names which do not contain strange
			** characters (i.e. not delimited identifiers) will
			** have an alias equal to the column name. However
			** column names which do contain strange characters
			** will have an alias which is different to the column
			** name. This is why we compare aliases rather than
			** column names.
			*/
			/* if the name is equal.. then set an alias name */
			if (STequal(col->alias, c2->alias))
			{
				vq_SetAlias(mf, col, start, sect);
			}
		}
	}
}

/*{
** Name:	vq_SetAlias - set a new alias name
**
** Description:
**	There was a duplicate column name found within the section, 
**	so scan the alias names to insure a unique alias.
**
** Inputs:
**	METAFRAME *mf;	- ptr to metaframe pointer
**	MFCOL	col;	- column to consider an alias name for
**	i4	start;	- index of starting table to consider
**	i4	sect;	- section to be considered
**
** Outputs:
**	Returns:
**		void
**	
**	Exceptions:
**
** Side Effects:
**
** History:
**	07/24/89 (tom) - created
*/
static VOID 
vq_SetAlias(mf, col, start, sect)
METAFRAME *mf;
MFCOL *col;
i4  start;
i4  sect;
{
	register i4  k;
	register MFCOL *c2;
	register i4  j;
	register MFTAB *t2;
	i4 len;
	i4 tmp;
	i4 collision = 1;
	char name[FE_MAXNAME+1];

	if (IIUGdlm_ChkdlmBEobject(col->name, NULL, FALSE) == UI_DELIM_ID)
	{
		/*
		** the name is a delimited identifier. Strip out any strange
		** characters, and truncate the name to 12.
		*/
		IIUGfnFeName(col->name, name);
		if (name[0] == EOS)
		{
			/*
			** The string consisted entirely of strange characters.
			** Assign the alias a one-character name. This 
			** shouldn't happen too often
			*/
			name[0] = 'x';
			name[1] = EOS;
		}
		if ((len = STlength(name)) > 12)
		{
			name[12] = EOS;
			len = 12;
		}
	}
	else
	{
		/* get the first 12 characters of the name.. or fewer
	   	if the name is shorter */
		len = STlcopy(col->name, name, 12); 
	}

	/* for all of the tables in the section */
	for (j = start; j < mf->numtabs; j++)
	{
		t2 = mf->tabs[j];

		/* if we are not in the same section any more then
		   break out of the inner loop */
		if (t2->tabsect != sect)
		{
			break;
		}
		
		/* for all the columns in the section */
		for (k = 0; k < t2->numcols; k++)
		{
			c2 = t2->cols[k];

			/* is the name the same */
			if (MEcmp(name, c2->alias, len) == 0)
			{
				/* see if the name has a number postpended..
				   if so remember the next highest number */
				if (  CVan(c2->alias + len, &tmp) == OK
				   && tmp > 0
				   && collision <= tmp
				   )
				{
					collision = tmp + 1;
				}

			}
		}
	}
	/* postpend the number to the end of the (possibly) truncated name */
	CVna(collision, name + len);

	/* allocate the new alias string and post into the column structure */
	col->alias = IIVQ_VAlloc(0, name);
}

