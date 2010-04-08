/*
**    Copyright (c) 1989, 2000 Ingres Corporation
*/
/*
**
** LOparse.c
**
*/

# include	<compat.h>
# include	<gl.h>
# include	<rms.h>
# include	<descrip.h>
# include	<lib$routines.h>
# include	<starlet.h>
# include	<lo.h>
# include	<me.h>
# include	<st.h>
# include	<er.h>  
# include	"lolocal.h"
# include	"loparse.h"


/*	
** Name:	LOparse	-		Interface to VMS SYS$PARSE function.  
**
** Description:
**	LOparse calls SYS$PARSE for the specified file name, initializing
** 	the RMS structures, if need be.  When the FAB is no longer needed,
**	LOclean_fab (below) should ALWAYS be called, to recover dynamic memory
**	and device channels.
**
** Input Parameters:
**		char		*filename	Name of file
** 		LOpstruct	*pstruct	RMS parse structure
**              bool		init		Whether to init the RMS structs
**
** Output Parameters:
**		none
**
** Returns:
** 		i4		Return from SYS$PARSE
**
** Assumptions:
**		none
**
**	Side Effects:
**		none
**
**	History:
**		4/17/89	-- (Mike S) written
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
*/

typedef	struct FAB	_FAB;
typedef struct NAM	_NAM;

i4
LOparse( filename, pstruct, init )
char 		*filename;
LOpstruct       *pstruct;
bool		init;               
{
	register _FAB 	*fab = &(pstruct->pfab);
	register _NAM 	*nam = &(pstruct->pnam);
	char 		*esa = pstruct->pesa;

	/* Initialize structures if told to */
	if (init)
	{                     
		MEfill(sizeof(*fab), NULL, (PTR)fab);
		MEfill(sizeof(*nam), NULL, (PTR)nam);
		fab->fab$b_bid = FAB$C_BID;
		fab->fab$b_bln = FAB$C_BLN;
		fab->fab$l_nam = nam;
		nam->nam$b_bid = NAM$C_BID;
		nam->nam$b_bln = NAM$C_BLN;
		nam->nam$l_esa = esa;
		nam->nam$b_ess = MAX_LOC;
	}

	/* Set file name */
	fab->fab$b_fns = STlength(fab->fab$l_fna = filename);

	/* Return result */
	return (sys$parse(fab));
}

/*	
** Name:	LOclean_fab -		Clean up FAB
**
** Description:
**	Force a FAB, previously used by SYS$PARSE, which might be holding
**	onto a parse channel, to give it up.  The techinque used is specified
**	in the VAX RMS reference manual.
**
** Input Parameters:
**    		_FAB		*fab	FAB to clean up
**
** Output Parameters:
**		none
**
** Returns:
** 		none
**
** Assumptions:
**		none
**
**	Side Effects:
**		We may regain a channel, and free some dynamic memory.
**
**	History:
**		4/17/89	-- (Mike S) written
*/
VOID
LOclean_fab( fab )
_FAB *fab;
{
	fab->fab$l_nam->nam$b_nop = NAM$M_SYNCHK;
	fab->fab$l_nam->nam$l_rlf = (_NAM *)NULL;
	fab->fab$b_fns = 0;
	sys$parse(fab);
}

/*	
** Name:	LOchannels_left -		Count available channels
**
** Description:
**	Open channels until we can't anymore, then close them.  This is
**	a quick and dirty way to count how many are left.  This is NOT
**	a production routine; it's for debugging, to see if channels
**	are being lost.
**
** Input Parameters:
**    		none
**
** Output Parameters:
**		none
**
** Returns:
** 		i4		Number of channels left.
**
** Assumptions:
**		none
**
**	Side Effects:
**		none
**
**	History:
**		4/17/89	-- (Mike S) written
*/                                  

# ifdef	CHANNEL_DEBUG
# define MAX_CHAN 1200	/* Maximum channels to open */

i4 LOchannels_left()
{
    $DESCRIPTOR( tt_desc, "TT");
    int sys$assign(), sys$dassgn(), status;
    short channels[MAX_CHAN];
    int chan_count = 0, i;

    while (1)
    {
	status = sys$assign( &tt_desc, channels + chan_count, 0, 0);
	if (status & 1)
	{
	    chan_count++;
	}
	else
	{
	    for ( i = 0; i < chan_count; i++ )
	    {
		status = sys$dassgn( channels[i] );
		if ( !(status & 1) ) lib$stop(status);
	    }
	    return (chan_count);
	}
    }
}
# endif		/* CHANNEL_DEBUG */
