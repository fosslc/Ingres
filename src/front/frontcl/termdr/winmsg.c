/*
**      Copyright (c) 2004 Ingres Corporation
*/
/*
**	WINMSG -
**		Message to be placed on the bottom of the user's
**		screen.  Message can be in the form of the printf
**		function.  Window is static and can only be used
**		by this function.
**
**	Parameters:
**		str - message string
**
**	Side effects:
**		stdmsg is updated in this routine
**
**	Error messages:
**		none
**
**  History:
**	11/11/87 (dkh) - Code cleanup.
**	11/10/88 (dkh) - Secondary fix to venus bug 3509.
**	18-apr-90 (bruceb)
**		Lint cleanup; removed all of TDwinmsg's argN params.
**	19-dec-91 (leighb) DeskTop Porting Change:
**		Changes to allow menu line to be at either top or bottom.
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
# include	<termdr.h>

FUNC_EXTERN	i4	TDrefresh();

GLOBALREF 	i4	IITDAllocSize;			 

static		i4	stdmsgattr = 0;			 

/*
**  WINDOW MESSAGE ROUTINE
*/

STATUS							 
TDwinmsg(str)
reg	char	*str;		/* message text */
{
	TDerase(stdmsg);			/* erase the window */
	TDhilite(stdmsg, 0, IITDAllocSize, stdmsgattr);				 

	/*
	**  Just add string to window.  This should be OK
	**  since all clients should have formatted their
	**  messages already and don't need further formatting.
	*/
	TDaddstr(stdmsg, str);

	TDtouchwin(stdmsg);
	TDmove(stdmsg, (i4) 0, (i4) 0);
	TDrefresh(stdmsg);			/* refresh the window */
	return(OK);
}

VOID
IITDssaSetStdmsgAttr(newattr)				 
i4	newattr;
{
	stdmsgattr = newattr;
}							 
