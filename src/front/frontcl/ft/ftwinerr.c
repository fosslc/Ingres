/*
**  FTwinerr
**
**  Copyright (c) 2004 Ingres Corporation
**
**  static	char	*Sccsid = "%W%	%G%";
**
**  History:
**
**	08/19/85 - DKH - Added extra argument in call to TDwin_str().
**	05/02/87 (dkh) - Changed to call FD routines via passed pointers.
**	08/14/87 (dkh) - ER changes.
**	09/22/89 (dkh) - Porting changes integration.
**	04/27/90 (dkh) - Fixed bug 21401 by increasing size of "bufr"
**			 in FTwerr() to handle wide terminal emulator
**			 windows on a workstation.
**	08/15/90 (dkh) - Fixed bug 21670.
**	19-dec-91 (leighb) DeskTop Porting Change:
**		Changes to allow menu line to be at either top or bottom.
**	11-jan-1996 (toumi01; from 1.1 axp_osf port)
**		Changed args in FTswinerr(), FTwinerr(),
**		and FTwerr() from i4  to PTR for 64-bit portability.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-Mar-2005 (lakvi01)
**	    Corrected function prototypes.
**	26-Aug-2009 (kschendel) 121804
**	    Need termdr.h to satisfy gcc 4.3.
*/

# include	<compat.h>
# include	<pc.h>		 
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"
# include	<kst.h>
# include	<er.h>
# include	<termdr.h>

FUNC_EXTERN	KEYSTRUCT   *FTgetch();

GLOBALREF 	i4	IITDAllocSize;			 

/*VARARGS2*/
VOID
FTswinerr(str, wait, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)
char	*str;
bool	wait;
PTR	a1, a2, a3, a4, a5, a6, a7, a8, a9, a10;
{
	FTwerr(str, wait, TRUE, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
}


/*VARARGS2*/
VOID
FTwinerr(str, wait, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)
char	*str;
bool	wait;
PTR	a1, a2, a3, a4, a5, a6, a7, a8, a9, a10;
{
	FTwerr(str, wait, FALSE, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
}


/*VARARGS3*/
VOID
FTwerr(str, wait, silent, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)
char	*str;
bool	wait;
bool	silent;
PTR	a1, a2, a3, a4, a5, a6, a7, a8, a9, a10;
{
	u_char	stdmsgattr;							 
	char	bufr[MAX_TERM_SIZE + 1];
	bool	bell = TRUE;

        /*
	** brett, Disable menus windex command string
	*/
        IITDdsmenus();

	if (silent)
	{
		bell = FALSE;
	}
	stdmsgattr = TDsaveLast(bufr);
	FTmessage(str, bell, wait, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
	if (!wait)
	{
		PCsleep((u_i4)2000);		/* pause two seconds */
	}
	TDerase(stdmsg);
	TDhilite(stdmsg, 0, IITDAllocSize, stdmsgattr);				 
	TDaddstr(stdmsg, bufr);
	TDtouchwin(stdmsg);
	TDrefresh(stdmsg);
	(*FTdmpmsg)(ERx("FT(s)winerr: WAIT = '%d'; SILENT = '%d'.\nMessage was \"%s\"\n\n"), (i4) bell, (i4) silent, str);

        /*
	** brett, Enable menus windex command string
	*/
        IITDenmenus();
}
