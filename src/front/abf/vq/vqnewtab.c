/* 
**	Copyright (c) 2004 Ingres Corporation  
*/
#include	<compat.h> 
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h> 
#include 	<st.h>
#include 	<er.h>
#include 	<uf.h>
#include 	<ug.h>
#include	<ui.h>
#include        <ooclass.h>
#include        <abclass.h>
#include	<metafrm.h>
#include	"ervq.h"	


/**
** Name:	vqnewtab -	Add a new table to the visual query
**
** Description:
**	Several routines to add tables to the visual query.
**
**	This file defines:
**		IIVQantAddNewTable - Add a new table to the visual query
**			also handles the metaframe struct.
**		IIVQrsRowSize	- calc the row size for a table.
**
** History:
**	12/27/89  (tom) - extracted from vqdloop.qsc 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Aug-2009 (kschendel) 121804
**	    Need ui.h to satisfy gcc 4.3.
**/

FUNC_EXTERN PTR IIVQ_VAlloc();
FUNC_EXTERN bool IIVQmmtMakeMfTable();
FUNC_EXTERN VOID IIVQcanColAliasNames();
i4 IIVQrsRowSize();


/*{
** Name:	IIVQantAddNewTable	- add a new table to metaframe 
**
** Description:
**	Given the name for the table to add..  look it up and add it to the 
**	metaframe. This means reading the attribute definitions from
**	the catalogs.. setting up a MFTAB and MFCOL structures.. and
**	inserting them into the metaframe's table at the correct index.
**	Errors are reported by the caller.
**
** Inputs:
**	METAFRAME *mf;	- ptr to metaframe
**	char name;	- ptr to the name to be added 
**	i4 idx;	- index into tabs array to insert at 
**	i4 usage;	- table's usage
**	i4 tabsect;	- table's section
**
** Outputs:
**	Returns:
**		bool	- TRUE means the table was added,
**			  FALSE means that the table doesn't 
**			  exist or some other error.. it is up
**			  to the caller to output the error message.
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	06/16/89 (tom) - extracted and made global 
**	3/6/91 (pete) - Added new warning message for wide unkeyed tables.
*/
bool
IIVQantAddNewTable(mf, name, idx, usage, tabsect)
METAFRAME *mf;
char *name;
i4  idx;
i4  usage;
i4  tabsect; 
{
	MFTAB *tab;
	register i4  i;
	register MFCOL *col;
	register MFTAB **tabs;
	register MFJOIN *j;
	bool unique_key;


	/* create and fill in the MFTAB struct */
	if (IIVQmmtMakeMfTable(name, usage, tabsect, &tab) == FALSE)
	{
		return (FALSE);

	}

	/* if the table has no unique keys.. then we give a warning message. */
	if (tab->usage == TAB_UPDATE && (tab->flags & TAB_NOKEY))
	{
		i4 rowsize;

		IIVQer1(E_VQ00CE_No_Tab_Key, tab->name);

		/* on Ingres, if unkeyed table with rowsize > (2000/2) bytes,
		** give another warning.
		*/
		if (IIUIdcn_ingres() &&
		   ((rowsize = IIVQrsRowSize(tab)) > (i4)DB_MAXTUP_DOC/2))
		{
			i4 maxtup      = DB_MAXTUP_DOC;

			IIVQerr(E_VQ00E3_WideRowWarning, 3, (PTR) tab->name,
				(PTR) &rowsize, (PTR) &maxtup);
		}
	}

	/* only after everything is allocated and fetched without
	   error do we insert the new table into the structure */

	tabs = mf->tabs;
	for (i = mf->numtabs - 1; i >= idx; i--)
		tabs[i + 1] = tabs[i]; 
	
	tabs[idx] = tab;
	mf->numtabs++;

	/* go through the join array and adjust for the insertion */
	for (i = 0; i < mf->numjoins; i++)
	{
		j = mf->joins[i];
		if (j->tab_1 >= idx)
			j->tab_1++;
		if (j->tab_2 >= idx)
			j->tab_2++;
	}

	/* now we must create alias names for the columns 
	   so that all simple fields of the form are unique,
	   and all table field columns are unique,
	   only lookup tables could be a problem since the 
	   major table in each structure must be unique by definition */
	if (usage == TAB_LOOKUP)
	{
		IIVQcanColAliasNames(mf, idx);
	}

	return (TRUE);
}

/*{
** Name:	IIVQrsRowSize	- determine row size of a table.
**
** Description:
**	Loop thru the MFCOL array to find the tuple width for the table.
**
** Inputs:
**	MFTAB *tab;	- ptr to table to calc row size for.
**
** Outputs:
**	Returns:
**		i4	- rowsize for table.
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	6-mar-1991	(pete)
**		Initial version.
*/
i4
IIVQrsRowSize(tab)
MFTAB *tab;
{
	i4 i;
	i4 rowsize = 0;

	for (i = 0; i < tab->numcols; i++)
	{
		/* db_length is one larger if type is nullable
		** (db_datatype < 0), thus no need to do separate
		** increment of rowsize for a nullable type.
		*/
		rowsize = rowsize + tab->cols[i]->type.db_length;
		/*
		if (tab->cols[i]->type.db_datatype < 0)
			rowsize++;	** increment 1 byte for NULL **
		*/
	}

	return rowsize;
}
