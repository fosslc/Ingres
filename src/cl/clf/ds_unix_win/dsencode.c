
# include	<compat.h>
# include	<gl.h>
# include	<st.h>
# include	<ds.h>
# include	"DSmapout.h"

/*
**Copyright (c) 2004 Ingres Corporation
**
** dsencode.c
**
**	Contains routines to build/encode command line flags from/to
**	a DS SH_DESC.
**
**	History:
**	
 * Revision 1.1  90/03/09  09:14:36  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:40:46  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:47:17  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:07:04  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:11:37  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.1  88/08/05  13:31:36  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
 * Revision 60.3  88/06/16  12:06:17  bobm
 * remove RCS header / log
 * 
**Revision 30.1  85/08/14  19:09:35  source
**llama code freeze 08/14/85
**
**		Revision 3.1  85/07/04  13:38:09  bruceb
**		Kira's changes for the multi-process version.
**		
**		Revision 3.0  85/05/08  20:30:41  wong
**		Updated copyright notice.
**		
**		Revision 1.2  85/03/25  15:44:02  ron
**		Integration from VMS - now uses a file pointer and CL routines
**		instead of file descriptors and Unix system calls.
**
**		Revision 1.1  85/02/13  09:38:04  ron
**		Initial version of new DS routines from the CFE project.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	16-aug-93 (ed)
**	    add <st.h>
**	29-may-1997 (canor01)
**	    Clean up compiler warnings.
*/

/*
** DSencode
**	Given a SH_DESC build a command line flag that encodes
**	the SH_DESC.  This must match what DSdecode expects.
**		DSencode now takes one more arg -ch, which is a flag to specify
**		whether it is -S or -R file. (kira)
**
** PARAMETERS
**	
**	SH_DESC	sh_desc;	Shared memory descriptor.
**
**	char	*str		Buffer to hold resulting command line string.
**
**	char	ch;		flag to name type of file
**
** RETURNS
**	Length of resulting string.
*/
i4
DSencode(SH_DESC *sh_desc, char *str, char ch)
{

	if (sh_desc->sh_type == IS_FILE)
	{
		STprintf(str, " -%c%sf%d:%s",
				ch,
				(sh_desc->sh_keep) ? "k" : "",
				(i4)0,  	/*  Not used on VMS */
				sh_desc->sh_reg.file.name);
	}
	else
		*str = 0;
	return (STlength(str));
}
