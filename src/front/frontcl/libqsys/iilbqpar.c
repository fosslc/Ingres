# include	<compat.h>
# include	<me.h>
# include	<eqrun.h>
# include	"libqsys.h"

# ifdef	VMS
/*
** IILBQPARM.C - VMS system dependent utilities to layer the arguments to
**		 Equel Param statements. These routines should not be ported 
** to non-VMS environments.  
**
** Defines:	IIxparret( tlist, argvec )
**		IIxouttrans( type, len, data, addr )
** 		IIxparset( tlist, argvec )
**		IIxintrans( type, len, arg )
** History:
**		27-feb-1985	- Written (ncg)
**		02-aug-1989	- shut ranlib up (GordonW)
**
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** IIxparret - Interface to C IIparret.
*/
void
IIxparret( tl, argv )
char	*tl;				/* Really string descriptor */
char	*argv[];
{
    void IIxouttrans();

    /* Call C param utility, but pass an argument translator */
    IIparret( IIsd(tl), argv, IIxouttrans );
}

/*
** IIxparset - Interface to C IIparset.
*/
void
IIxparset( tl, argv )
char	*tl;				/* Really string descriptor */
char	*argv[];
{
    char *IIxintrans();

    /* Call C param utility, but pass an argument translator */
    IIparset( IIsd(tl), argv, IIxintrans );
}

/*
** IIxouttrans - Reformat data on the way out of a Param 'get data'.
*/
void
IIxouttrans( type, len, data, addr )
i4	type, len;		/* Type and len sent from the param */
char	*data;
char	*addr;			/* Host address */
{
    if ( type != T_CHAR )
	MEcopy( data, len, addr );
    else
	IIsd_undo( data, addr );
}


/*
** IIxintrans - Format data before sending to a Param 'put data' call.
*/
char	*
IIxintrans( type, len, arg )
i4	type, len;		/* Type and len sent from the param format */
char	*arg;
{
    if ( type != T_CHAR )
	return arg;
    if ( len < 0 )		/* Special flag to prevent trimming blanks */
	return IIsd_no( arg );
    else
	return IIsd( arg );
}
# else
static	i4	ranlib_dummy;
# endif /* VMS */
