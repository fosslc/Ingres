/*
**  ftaddfrs.c
**
**  Add an activate FRS key item to the data structures.
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	Created - 07/25/85 (dkh)
**	85/08/23  12:09:20  dave
**		Initial version in 4.0 rplus path. (dkh)
**	85/09/18  16:26:20  john
**		Changed nat=>nat, CVna=>CVna, CVan=>CVan, u_char=>u_char
**	85/09/30  10:11:51  john
**		Checked out with the rest of the "nat" changes
**	85/09/30  15:20:36  dave
**		Put in check to make sure that the a frskey is not in use
**		before trying to assign to it. (dkh)
**	85/09/30  15:54:17  dave
**		Fixed to use the appropriate value to check if a frskey
**		is in use. (dkh)
**	86/11/18  21:47:17  dave
**		Initial60 edit of files
**	86/11/21  07:41:46  joe
**		Lowercased FT*.h
**	04/03/90 (dkh) - Integrated MACWS changes.
**	08/29/90 (dkh) - Integrated porting change ingres6202p/131520.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<tdkeys.h>
# include	<mapping.h>
# include	"ftframe.h"


STATUS
FTaddfrs(index, intrval, expl, val, act)
i4	index;
i4	intrval;
char	*expl;
i4	val;
i4	act;
{
	struct frsop	*op;

	/*
	**  Go to 0 base indexing.
	*/
	index--;

	op = &(frsoper[index]);
	if (op->intrval != fdopERR)
	{
		return(FAIL);
	}
	op->intrval = intrval;
	op->exp = expl;
	op->val = (bool) val;
	op->act = (bool) act;

# ifdef DATAVIEW
	if (IIMWimIsMws())
		return(IIMWsmeSetMapEntry('s', index));
# endif	/* DATAVIEW */

	return(OK);
}


/*{
** Name:	IIFTgfeGetFrskeyExpl	- Obtain explanatory text for an frskey.
**
** Description:
**	Return the explanatory text defined for an frskey.  For a given frskey,
**	if that frskey is active for the current display loop or submenu,
**	return the explanation specified in the activate block for the frskey.
**	If the frskey is not active, return FAIL.
**
** Inputs:
**	index	Index of the frskey.  1's based, to match FTaddfrs().
**
** Outputs:
**	expl	Pointer to explanatory text; undefined if frskey not active.
**
**	Returns:
**		OK	frskey is active in the display loop or submenu.
**		FAIL	frskey is not active.
**
**	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	29-nov-89 (bruceb)	Written.
*/
STATUS
IIFTgfeGetFrskeyExpl(index, expl)
i4	index;
char	**expl;
{
    struct frsop	*op;

    /*
    **  Go to 0 base indexing.
    */
    index--;

    op = &(frsoper[index]);
    if (op->intrval <= 0)
    {
	/* FRSkey 'index' not active. */
	return(FAIL);
    }
    *expl = op->exp;
    return(OK);
}
