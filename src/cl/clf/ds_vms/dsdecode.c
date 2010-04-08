/*
** Copyright (c) 1984, 2001 Ingres Corporation
*/

/* static char * Sccsid = "%W%  %G%"; */

# include	<compat.h>
# include	<gl.h>
# include	<cm.h>
# include	<si.h>
# include	<st.h>
# include	<ds.h>
# include	"dsmapout.h"

/*
** DSdecode
**
** Contains routines to decode command line flags to
** a DS SH_DESC.
*/

/*
** DSdecode
**	Given a command line flag, build a SH_DESC.
**
** PARAMETERS
**	
**	char	*comline;	Command line flag.
**
**	SH_DESC	*sh_desc;	Shared memory descriptor.
**
** RETURNS
**	STATUS
**	History:
**      11-aug-93 (ed)
**          added missing includes
**	04-jun-2001 (kinte01)
**	    replace STindex with STchr
*/
STATUS
DSdecode(comline, sh_desc)
char	*comline;
SH_DESC	*sh_desc;
{
	if (*comline++ == '-' && *comline++ == 'S')
	{
		if (*comline == 'k')
		{
			sh_desc->sh_keep = KEEP;
			comline++;
		}

		if (*comline == 'f')
		{	/* shared file */
			sh_desc->sh_type = IS_FILE;
			++comline;
			if (CMdigit(comline)) {
				char	*c;

				/*
				** Unused on VMS 
				** Just show file is closed by setting
				** the file pointer to NULL.
				*/

				c = STchr(comline, ':');
				*c++ = '\0';
				sh_desc->sh_reg.file.fp = NULL;
				comline = c;
			}
			sh_desc->sh_reg.file.name = comline;
		}
		return OK;
	}

	return FAIL;
}
