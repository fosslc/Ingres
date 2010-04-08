/*
**  ftutil.c
**
**  Utility routines for FT.
**
**  Copyright (c) 2004 Ingres Corporation
**
**
**  History:
**
**	Created - 08/06/85 (dkh)
 * Revision 60.3  86/11/21  07:47:04  joe
 * Lowercased FT*.h
 * 
 * Revision 60.2  86/11/18  21:58:05  dave
 * Initial60 edit of files
 * 
 * Revision 60.1  86/11/18  21:57:56  dave
 * *** empty log message ***
 * 
 * Revision 40.3  85/09/18  17:14:46  john
 * Changed i4=>nat, CVla=>CVna, CVal=>CVan, unsigned char=>u_char
 * 
 * Revision 40.2  85/09/07  22:52:49  dave
 * Added support for set and inquire statements. (dkh)
 * 
 * Revision 1.2  85/09/07  20:49:09  dave
 * Added support for set and inquire statements. (dkh)
 * 
 * Revision 1.1  85/08/09  10:57:36  dave
 * Initial revision
 * 
**	04/04/90 (dkh) - Integrated MACWS changes.
**	07/10/90 (dkh) - Integrated more MACWS changes.
**	08/29/90 (dkh) - Integrated porting change ingres6202p/131520.
**      23-sep-96 (mcgem01)
**              Global data moved to ftdata.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-Mar-2005 (lakvi01)
**	    Corrected function prototypes.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"
# include	<mapping.h>


GLOBALREF	bool	ftshmumap;

VOID
FTsetmmap(val)
i4	val;
{
	ftshmumap = (bool) val;
}

i4
FTinqmmap()
{
	return((i4) ftshmumap);
}


/*
**  Returns a NULL pointer if we can not find a
**  frskey and menuitem that have the same interrupt
**  value.
*/

char *
FTflfrsmu(intrval)
i4	intrval;
{
	char	*label = NULL;
	i4	maxfrskey;
	i4	maxmapkey;
	i4	i;
	i4	keynum = 0;

	maxfrskey = MAX_CTRL_KEYS + KEYTBLSZ;
	maxmapkey = maxfrskey + 4;

	for (i = 0; i < maxfrskey; i++)
	{
		if (frsoper[i].intrval == intrval)
		{
			keynum = i + 1;
			break;
		}
	}
	if (keynum)
	{
		keynum += ftFRSOFFSET - 1;
		for (i = 0; i < maxmapkey; i++)
		{
			if (keymap[i].cmdval == keynum)
			{
				label = keymap[i].label;
				break;
			}
		}
	}

	return(label);
}

VOID
FTclrfrsk()
{
	i4		maxkey;
	i4		i;
	struct	frsop	*opptr;

	maxkey = MAX_CTRL_KEYS + KEYTBLSZ;
	opptr = &frsoper[0];

	for (i = 0; i < maxkey; i++, opptr++)
	{
		opptr->exp = NULL;
		opptr->intrval = fdopERR;
		opptr->val = FRS_UF;
		opptr->act = FRS_UF;
	}

# ifdef DATAVIEW
	_VOID_ IIMWcmtClearMapTbl('s');
# endif	/* DATAVIEW */
}
