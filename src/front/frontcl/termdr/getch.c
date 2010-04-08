# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<kst.h>
# include	<ctrl.h> 
# include	<si.h> 
# include	<ex.h>
# ifdef	ATTCURSES
# include	<te.h>
# endif
# include	<termdr.h> 

/*
**  This routine reads in a character from the window.
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	06/19/87 (dkh) - Code cleanup.
**	11/24/89 (dkh) - Put in support for windex goto's.
**	02/08/90 (dkh) - Changed FUNC_EXTERN to GLOBALREF for "in_func".
**	08/28/90 (dkh) - Integrated porting change ingres6202p/131215.
**	09/04/90 (dkh) - Removed KFE stuff which has been superseded by SEP.
**	11/29/90 (dkh) - Removed check for EOF since only ITin() can tell
**			 the difference between real EOF and legal
**			 character 0xFF.
*/

/*
** Function pointer to routine that gets the character from the
** input buffer.
**
**  in_func now should return an KEYSTRUCT structure for international support.
*/

GLOBALREF	KEYSTRUCT	*(*in_func)();
FUNC_EXTERN	KEYSTRUCT	*IITDwiWviewInput();

GLOBALREF	bool		IITDWS;


# define	RESC		'\035'


KEYSTRUCT   *
TDgetch(awin)
WINDOW	*awin;
{
	reg	KEYSTRUCT   *inp;

	/*
	**  Note: EOF is usally -1.  If the international
	**  character set uses "0xff" as a valid character
	**  then the test below has to be changed. (dkh)
	*/
	inp = (*in_func)(stdin);
	if (IITDWS && inp->ks_ch[0] == RESC)
	{
		inp = IITDwiWviewInput();
	}
	return(inp);
}
