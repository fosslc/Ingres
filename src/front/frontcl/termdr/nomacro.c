/*
**  nomacr.c
**
**  Set flag indicating that no macro expasion is to be
**  done for the function key that is specified.
**
**  Copyright (c) 2004 Ingres Corporation
**
**
**  History:
**
**	Created - 07/12/85 (dkh)
 * Revision 4002.2  86/06/11  11:28:03  mgw
 * Re-integrated with VMS
 * 
 * Revision 40.2  85/09/18  17:56:13  john
 * Changed i4=>nat, CVla=>CVna, CVal=>CVan, unsigned char=>u_char
 * 
 * Revision 40.1  85/08/22  17:43:13  dave
 * Initial revision
 * 
 * Revision 1.1  85/07/19  11:33:36  dave
 * Initial revision
 * 
** History:
**      24-sep-96 (mcgem01)
**              Global data moved to termdrdata.c
**	28-Mar-2005 (lakvi01)
**		Corrected function prototypes.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<termdr.h>
# include	<tdkeys.h>


GLOBALREF	bool	macexp[KEYTBLSZ];

VOID
TDnomacro(index)
i4	index;
{
	if (index >= 0 && index < KEYTBLSZ)
	{
		macexp[index] = FALSE;
	}
}


/*
**  New routine to re-enable a 3.0 function key definition.
**  Fix for BUG 9449. (dkh)
*/
VOID
TDonmacro(index)
i4	index;
{
	if (index >= 0 && index < KEYTBLSZ)
	{
		macexp[index] = TRUE;
	}
}
