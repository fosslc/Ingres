/*
** Copyright (c) 1984 Ingres Corporation
*/

/* static char * Sccsid = "%W%  %G%"; */

# include	<compat.h>
#include    <gl.h>
# include	<si.h>
# include	<st.h>
# include	<ds.h>
# include	"dsmapout.h"

/*
** DSencode
**
** Contains routines to build/decode command line flags from/to
** a DS SH_DESC.
*/

/*
** DSencode
**	Given a SH_DESC build a command line flag that encodes
**	the SH_DESC.  This must match what DSdecode expects.
**
** PARAMETERS
**	
**	SH_DESC	sh_desc;	Shared memory descriptor.
**
**	char	*str		Buffer to hold resulting command line string.
**
** RETURNS
**	Length of resulting string.
**	History:
**      11-aug-93 (ed)
**          added missing includes
*/
i4
DSencode(sh_desc, str)
SH_DESC	*sh_desc;
char	*str;
{

	if (sh_desc->sh_type == IS_FILE)
	{
		STprintf(str, " -S%sf%d:%s",
				(sh_desc->sh_keep) ? "k" : "",
				(i4)0,  	/*  Not used on VMS */
				sh_desc->sh_reg.file.name);
	}
	else
		*str = 0;
	return STlength(str);
}
