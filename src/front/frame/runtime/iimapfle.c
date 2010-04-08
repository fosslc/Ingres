/*
**	iimapfile.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<menu.h>
# include	<runtime.h>
# include	<er.h>
# include	<st.h>

/**
** Name:	iimapfile.c
**
** Description:
**
**	Public (extern) routines defined:
**		IImapfile()
**	Private (static) routines defined:
**
** History:
**	Created - 07/11/85 (dkh)
**	12/11/87 (dkh) - Fixed jup bug 1540.
**	05/20/88 (dkh) - More ER changes.
**	25-Aug-1993 (fredv) - inlcuded <st.h>.
**      24-sep-96 (hanch04)
**          Global data moved to data.c
**/

FUNC_EXTERN	char	*STalloc();

GLOBALREF	char	*IIsimapfile ;

/*{
** Name:	IImapfile	-	Parse key mapping file strings
**
** Description:
**	Routine that is invoked for a ## mapfile statement.
**	Calls FTmapfile to parse strings in files to determine
**	the control/function key to command/menu position mapping.
**
** Inputs:
**	filename	Name of the mapping file
**
** Outputs:
**
** Returns:
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	
*/

IImapfile(filename)
char	*filename;
{
	IIsimapfile = STalloc(filename);
	FTmapfile(filename);
}
