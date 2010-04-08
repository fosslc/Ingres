/*
**  FTaddcomm
**
**  Copyright (c) 2004 Ingres Corporation
**
**  9/21/84 -
**    Stolen from addcomm for FT. (dkh)
*/

/*
**  ADDCOMM.c  -  Add a command to the frame driver
**	
**  This routine add or changes a command that is legal to the frame
**  driver.  If the command has an operation that is not contained
**  in the frame, the frame driver will return the value of the
**  operation.
**	
**  History: JEN - 1 Nov 1981 (written)
**	04/03/90 (dkh) - Integrated MACWS changes.
**	08/29/90 (dkh) - Integrated porting change ingres6202p/131520.
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
# include	"ftframe.h"

FTaddcomm(command, operation)
u_char	command;
i4	operation;
{
    froperation[command] = (i2) operation;
    fropflg[command] = (i1) (fdcmEDIT | fdcmINSRT | fdcmBRWS);

# ifdef DATAVIEW
	_VOID_ IIMWsmeSetMapEntry('o', (i4) command);
# endif	/* DATAVIEW */
}
