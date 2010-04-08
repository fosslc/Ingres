# include	<compat.h>
# include	<gl.h>
# include	<si.h>
# include	<lo.h>
# include	"silocal.h"

/* SIcat
**
**	Copy file
**
**	Copy file with location pointed to by "in" to previously opened
**	stream "out".  "Out" is usually standard output.
**
**Copyright (c) 2004 Ingres Corporation
**	History
**		03/09/83 -- (mmm) written
**		
 * Revision 1.1  90/03/09  09:16:23  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:45:18  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:59:50  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:19:28  source
 * sc
 * 
 * Revision 1.2  89/04/11  12:14:34  source
 * sc
 * 
 * Revision 1.3  89/03/23  12:20:43  kathryn
 * bug 4533 - changed comment.
 * 
 * Revision 1.2  89/03/23  12:17:54  kathryn
 * Bug # 4533 - Changed SI_CAT_NULL to SI_CANT_OPEN same error different
 * macro name.
 * 
 * Revision 1.1  88/08/05  13:44:43  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
 * Revision 60.2  86/12/02  10:53:31  joe
 * Initial60 edit of files
 * 
 * Revision 60.1  86/12/02  10:53:08  joe
 * *** empty log message ***
 * 
**Revision 30.2  85/11/01  10:52:31  joe
**Took y line changes.
**
**		Revision 30.2  85/09/17  12:33:32  roger
**		Return SI_CAT_NONE if file is nonexistant, _NULL if
**		fp is null, _DIR if trying to cat a directory (or
**		trashed LOCATION).
**	03/23/89 (kathryn)  Bug #4533
**		Changed SI_CAT_NULL to SI_CANT_OPEN. They were the same
**		error message, just different macro names.
**	19-jun-95 (emmag)
**	    Changed include from SI.h to silocal.h due to a conflict
**	    with gl\hdr\hdr\si.h on platforms which don't support case
**	    sensitivity in file names.
**	03-jun-1999 (hanch04)
**          Added LARGEFILE64 for files > 2gig
**	08-jul-1999 (hanch04)
**	    Reverse fopen64 and fopen.  fopen64 should be for LARGEFILE64
**		
**
*/

/* static char	*Sccsid = "@(#)SIcat.c	3.1  9/30/83"; */

STATUS
SIcat(in, out)
LOCATION	*in;	/* file location */
register FILE	*out;	/* file pointer */
{
	register FILE	*input_file;
	register i2	c;
	i2		flag;
	STATUS		ret_val = OK;

	if (out == NULL)
		ret_val = SI_CANT_OPEN;
	else if (LOisdir(in, &flag) != OK || flag == ISDIR)
		ret_val = SI_CAT_DIR;
#ifdef LARGEFILE64
	else if ((input_file = fopen64(in->string, "r")) == NULL)
#else
	else if ((input_file = fopen(in->string, "r")) == NULL)
#endif /* LARGEFILE64 */
		ret_val = SI_CAT_NONE;
	else
	{
		while ((c = SIgetc(input_file)) != EOF)
		{
			SIputc(c, out);
		}

		fclose(input_file);
	}

	return(ret_val);
}
