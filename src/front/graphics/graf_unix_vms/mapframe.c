
/*
**    Copyright (c) 2004 Ingres Corporation
*/

/*# includes */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ergr.h>
# include	<er.h>
# include	<adf.h>
# include	<graf.h>
# include	<gropt.h>

/**
** Name:    mapframe.c	-	Map data to a graphics frame
**
** Description:
**	This file defines:
**
**	map_frame	Routine to 'map' table/view data to a graph frame
**
** History:
**	<automatically entered by code control>
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
**  Get FE control block once to avoid all routines underneath this having
**  to call FEadfcb.
*/
GLOBALDEF ADF_CB	*GR_cb;

static	i4	nowarn = 0;


/*{
** Name:	map_frame	-	Map data to a graphics frame
**
** Description:
**
** Inputs:
**	frame		Graphics frame to have data mapped
**	map		Map structure, incidating how elements are mapped
**
** Outputs:
**	warn		Returned with pointer to a 0-terminated array of
**			warning error numbers (non-fatal).
**	Returns:
**		STATUS - OK  ( Data is 'plottable'.  Warn may still
**			       be set to reflect non-fatal errors ).
**			 GDNOTAB - unable to access table/view
**			 GDNOX	 - independent series column not found
**			 GDNOY	 - dependent series column not found
**			 GDNOZ	 - selector series column not found
**	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	12-dec-85 (drh) First written (for default data).
**      11-jan-1996 (toumi01; from 1.1 axp_osf port)
**              Added cast for arg in FEmsg() for axp_osf
*/

STATUS
map_frame( frame, map, warn )
GR_FRAME	*frame;
GR_MAP		*map;
i4		**warn;
{
	VOID	map_def();
	STATUS	retstat = OK;
	ADF_CB	*FEadfcb();

	GR_cb = FEadfcb();

	/* flag to GT that we have new data */
	GTnewdat();

	if ( map->gr_view == NULL || *(map->gr_view) == '\0' )
	{
		map_def( frame, map );
		*warn = &nowarn;
	}
	else
	{
		retstat = map_view( frame, map, warn );

# ifdef DEBUG
		{
		    i4  warnno = 0;
		    i4  i = 0;
		    for ( i = 0; i < 4; i++ )
		    {
			    warnno = ((*warn)[i] );
			    FEmsg( ERx("warnno is %d"), (bool) TRUE, (PTR)warnno );
		    }
		}
# endif
	}

	/*  Dump data to .scr file */

	GTdmpvlist();

	return( retstat );
}

