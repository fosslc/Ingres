/*
**  Copyright (c) 2004 Ingres Corporation
**
**
**  VFINSERT.c -- approximate the insert mode of qbf
**
**  History:
**	mm/dd/yy (RTI) -- created for 5.0.
**	10/20/86 (KY)  -- Changed CH.h to CM.h.
**	21-jul-88 (bruceb)	Fix for bug 2969.
**		Changes to vfEdtStr to specify maximum length.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"
# include	<cm.h>
# include	<vt.h>


/*
** The definition of VFBUFSIZ had better match that of other code in
** Vifred and VT, as well as column size in ii_trim catalog.
*/
# define	VFBUFSIZ	150


u_char *
vfEdtStr(frm, str, stry, strx, maxlen)
FRAME	*frm;
char	*str;
i4	stry;
i4	strx;
i4	maxlen;
{
	static	u_char	buf[VFBUFSIZ + 1];
	u_char		bbuf[VFBUFSIZ + 1];

	/* Clearly, maxlen had better be less than VFBUFSIZ */

	/*
	**  Fix for BUG 7498. (dkh)
	*/
	VTgetstring(frm, stry, strx, endxFrm, str, bbuf, VT_GET_EDIT, maxlen);
	STcopy(bbuf, buf);
	return(buf);
}
