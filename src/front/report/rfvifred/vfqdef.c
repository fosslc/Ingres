/*
**  vfqdef.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**  	10/23/89   (martym)
**	>> GARNET <<  Changed the call to 'vfmqbf' to match the new interface.
**	26-oct-89 (bruceb)
**		Removed vfcursor() call from qdefcom().  Placed it in fdcat().
**	11/27/89   (martym)
**		Changed the decleration of 'vfmqbf' to match its new 
**		definition.
**      06/12/90 (esd) - Make the member frmaxx in FRAME during vifred
**                       include room for an attribute byte to the left
**                       of the end marker, as well as the end marker.
**                       (We won't always insert this attribute
**                       before the end marker, but we always leave
**                       room for it).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Aug-2009 (kschendel) 121804
**	    Need vt.h to satisfy gcc 4.3.
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"
# include	<vt.h>

FUNC_EXTERN	bool	vfmqbf();

i4
qdefcom(name)
char	*name;
{
	if (vfmqbf(name, FALSE, NULL, TRUE, &frame) == FALSE)
	{
		frame = NULL;
		return(FALSE);
	}

	(frame->frmaxx) += 2;
	if (!VTnewfrm(frame))
	{
		frame = NULL;
		return(FALSE);
	}

	globy = 0;
	globx = 0;
	vforigintype = OT_JOINDEF;
	STcopy(frame->frname, vforgname);
	return(TRUE);
}
