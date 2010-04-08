/*
**  FTlotus
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History
**	86/11/11  john - Only turn on lotus by default for MSDOS.
**	86/10/13  john - Make lotus the default menu style.
**	86/10/11  john - Initial revision
**	03/19/87 (dkh) - Brought up from PC.
**	10/02/87 (dkh) - Changed FTlotus_on to FTuselotus for check7.
**      23-sep-96 (mcgem01)
**              Global data moved to ftdata.c
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

GLOBALREF i4	FTuselotus;

FTislotus()
{
	return (FTuselotus);
}
