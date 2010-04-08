/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"
# include	<frsctblk.h>


/**
** Name:	fttmout.c -	Routines to handle timeout feature
**
** Description:
**	This file defines:
**	IIFTtoTimeOut		- Used to determine if program has timed out
**	IIFTstsSetTmoutSecs	- Set the timeout seconds if need setting
**
** History:
**	09-nov-88 (bruceb)	Written
**	21-nov-88 (bruceb)
**		Added IIFTstsSetTmoutSecs().
**      23-sep-96 (mcgem01)
**              Global data moved to ftdata.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* Has timeout occurred? */
GLOBALREF	bool	IIFTtmout;

/*
** Current number of seconds of inactivity until timeout occurs;
** 0 means that timeout isn't enforced.
*/
GLOBALREF	i4	IIFTntsNumTmoutSecs;

/*{
** Name:	IIFTtoTimeOut	- Used to determine if program has timed out
**
** Description:
**	Used to determine if the user has allowed the program to 'time out'
**	at a request for input.  If timeout has occurred then set the event
**	value in the passed in event control block.  Also reset variable so
**	that a subsequent call on this routine would return 'no timeout'.
**	As a consequence of resetting the variable, this routine can only
**	be called once after a timeout.
**
** Inputs:
**
** Outputs:
**	evcb	Set the event member to reflect the occurrence of a timeout.
**
**	Returns:
**		TRUE	timeout has occurred.
**		FALSE	timeout has not occurred.
**
**	Exceptions:
**		None
**
** Side Effects:
**	IIFTtmout is set to FALSE;
**
** History:
**	09-nov-88 (bab)	Written.
*/

bool
IIFTtoTimeOut(evcb)
FRS_EVCB	*evcb;
{
    bool	timeout = IIFTtmout;

    IIFTtmout = FALSE;

    if (timeout)
	evcb->event = fdopTMOUT;

    return (timeout);
}

/*{
** Name:	IIFTstsSetTmoutSecs	- Set timeout seconds if need setting
**
** Description:
**	Set an FT global to value of timeout in the passed control block.
**	This value will be used in subsequent TEget calls.  This routine
**	may also be used to turn off timeout; just set value to 0.
**
** Inputs:
**	evcb	'timeout' field contains number of seconds until timeout.
**
** Outputs:
**
**	Returns:
**		VOID
**
**	Exceptions:
**		None
**
** Side Effects:
**
** History:
**	21-nov-88 (bab)	Written.
*/

VOID
IIFTstsSetTmoutSecs(evcb)
FRS_EVCB	*evcb;
{
    IIFTntsNumTmoutSecs = evcb->timeout;
}
