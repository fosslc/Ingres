
# include	<compat.h>
# include	<gl.h>
# include	<lo.h>
# include	<cm.h>
# include	<ds.h>
# include	<st.h>
# include	"DSmapout.h"


/*
**Copyright (c) 2004 Ingres Corporation
**
** dsdecode.c
**
**	Contains routines to decode command line flags to
**	a DS SH_DESC.
**
**	History:
**	
 * Revision 1.1  90/03/09  09:14:36  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:40:43  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:47:15  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:07:02  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:11:37  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.2  88/08/31  17:17:53  roger
 * Changes needed to build r
 * 
 * Revision 1.1  88/08/05  13:31:35  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
 * Revision 60.5  87/10/27  14:50:45  bobm
 * NO CHANGE
 * 
 * Revision 60.4  87/10/01  15:41:22  bobm
 * change CH calls to CM
 * 
 * Revision 60.3  87/04/14  12:37:35  bobm
 * change STindex for Kanji
 * 
 * Revision 60.2  86/12/02  09:25:41  joe
 * Initial60 edit of files
 * 
 * Revision 60.1  86/12/02  09:25:03  joe
 * *** empty log message ***
 * 
**		Revision 1.2  86/04/24  11:09:31  daveb
**		Clean comments,
**		simplify cgd's change of having sh_keep always set.
**		
**		Revision 30.2  85/09/28  12:50:27  cgd
**		Set sh_desc->sh_keep to KEEP whether or not comline
**		has a 'k' on it (ibs integration)
**
**		Revision 30.1  85/08/14  19:09:33  source
**		llama code freeze 08/14/85
**
**		Revision 3.2  85/07/05  18:10:01  bruceb
**		change to Kira's code--don't always keep the .ds file 
**		(why keep files around to clutter up the directory?).
**		
**		Revision 3.1  85/07/04  13:35:01  bruceb
**		Kira's changes for the multi-process version.
**		
**		Revision 3.0  85/05/08  20:30:50  wong
**		Updated copyright notice.
**		
**		Revision 1.2  85/03/25  15:43:57  ron
**		Integration from VMS - now uses a file pointer and CL routines
**		instead of file descriptors and Unix system calls.
**
**		Revision 1.1  85/02/13  09:39:19  ron
**		Initial version of new DS routines from the CFE project.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      27-apr-1999 (hanch04)
**          replace STindex with STchr
*/

/*
** DSdecode
**	Given a command line flag, build a SH_DESC.
**		DSdecode now takes one more arg - ch which is a flag to 
**		specify whether it is -S or -R file. 
**		put in changes to correctly process %R flag (kira)
**
** PARAMETERS
**	
**	char	*comline;	Command line flag.
**
**	SH_DESC	*sh_desc;	Shared memory descriptor.
**
**	char	ch;		flag to name type of file.
**
** RETURNS
**	STATUS
*/


STATUS
DSdecode(comline, sh_desc, ch)
char	*comline;
SH_DESC	*sh_desc;
char	ch;
{

	if (*comline++ == '-' && *comline++ == ch)
	{
		if (*comline == 'k')
			comline++;

		/* cgd said this should always be set.
		** I don't know why (daveb).
		*/
		sh_desc->sh_keep = KEEP;

		if (*comline == 'f')
		{	/* shared file */
			sh_desc->sh_type = IS_FILE;
			++comline;
			if (CMdigit(comline))
			{
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

			/* file exists, must be open for writing */
			if (ch == 'R') 
			{
				char		locbuf[100];
				LOCATION	loc;
				FILE		*fp;

				STcopy(sh_desc->sh_reg.file.name, locbuf);
				LOfroms((LOCTYPE)FILENAME, locbuf, &loc);
				if (SIopen(&loc, "w", &fp) != OK)
				{
					SIprintf("DSdecode : error openinig .dr file\n\r");
					return(FAIL);
				}

				sh_desc->sh_reg.file.fp = fp;

			}
		}
		return(OK);
	}

	return(FAIL);
}
