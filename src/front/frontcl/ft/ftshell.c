/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"
# include	<ft.h>
# include	<lo.h>
# include	<nm.h>
# include	<pc.h>
# include	<te.h>
# include	<si.h>
# include	<er.h>


/**
** Name:	ftshell.c -	Implement the shell key.
**
** Description:
**	This file defines:
**
**	FTshell		Implement the shell key (fdopSHELL).
**
** History:
**	23-jul-87 (bruceb)	Written.
**	05/20/88 (dkh) - ER changes.
**	07-jun-89 (bruceb)
**		Turn off use of ING_SHELL.  At this time only PCcmdline's
**		default interpretor will be allowed.
**	08-jan-90 (sylviap)
**		Added PCcmdline parameters.
**      08-mar-90 (sylviap)
**              Added a parameter to PCcmdline call to pass an CL_ERR_DESC.
**	04/04/90 (dkh) - Integrated MACWS changes.
**	18-Apr-90 (brett)
**	    Move IITDfalsestop() into FTshell from FTusrinp.
**	08/29/90 (dkh) - Integrated porting change ingres6202p/131520.
**/


/*{
** Name:	FTshell	- Implement the shell key.
**
** Description:
**	Implement the shell key.  This means to clear the screen
**	and start up a command shell, redrawing the screen on return.
**	Shell chosen is that specified by ING_SHELL, if set, or else
**	the default command interpreter chosen by PCcmdline.
**
**	Note:  The usual problem exists that under BSD the shell is
**	in the same process group as the FE, so that hitting ^Z when
**	in that sub-shell, then fore-grounding, will put the user
**	back into cbreak mode.
**
**	Note 2:  No code is included for use with Vigraph.
**	Special case code has been included in gtedit.c and
**	ftuserinp.c (when GRAPHICS on) to handle the shell key
**	in that product, but some restrictions currently exist
**	on the use of the shell key.  Specifically, the shell
**	key is disallowed when editing text on the menu line,
**	and when specifying a region on the screen for a graphical
**	item (GTpoint).  Also, on multi-plane graphics terminals,
**	graphics plane will persist even when in shell; this
**	should probably be changed.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
**	Returns:
**		None.
**
**	Exceptions:
**		None.
**
** Side Effects:
**	User is placed into a command shell until exit specified.
**
** History:
**	23-jul-87 (bruceb)	Written.
*/
VOID
FTshell()
{
    char	whichshell[MAX_LOC + 1];
    char	*shellptr = whichshell;
    LOCATION	*shlocptr = NULL;
    LOCATION	shell_loc;
    CL_ERR_DESC err_code;


    /*
    ** brett, False stop windex command string.  Since the shell call
    ** will put out the VE termcap before the shell call and VS after
    ** the shell returns, we must stop windex from closing down when
    ** it receives the VE string and restarting when it receives the
    ** VS string.  This routine will cause a disable, enable instead.
    */
    IITDfalsestop();

# ifdef DATAVIEW
    _VOID_ IIMWsmwSuspendMW();
# endif	/* DATAVIEW */

    TDrstcur(0, COLS - 1, LINES - 1, 0); /* move to bottom line of screen */
    TDrestore(TE_NORMAL, FALSE);	/* Turn off forms mode */
    SIprintf(ERx("\n"));		/* move to line below the menu line */
    SIflush(stdout);		/* flush the \n before starting the shell */

# ifdef	ING_SHELL_USED
    NMgtAt(ERx("ING_SHELL"), &shellptr);
    if (shellptr == NULL || *shellptr == EOS)
    {
	shlocptr = NULL;
    }
    else
    {
	STcopy(shellptr, whichshell);
	shlocptr = &shell_loc;
	LOfroms(PATH & FILENAME, whichshell, shlocptr);
    }
    PCcmdline(shlocptr, (char *)NULL, PC_WAIT, NULL, &err_code);
# else
    PCcmdline((LOCATION *)NULL, (char *)NULL, PC_WAIT, NULL, &err_code);
# endif

    TDrestore(TE_FORMS, TRUE);	/* Place terminal back into forms mode */

# ifdef DATAVIEW
    _VOID_ IIMWrmwResumeMW();
# endif	/* DATAVIEW */

    TDrefresh(curscr);
}
