/*
**  ftmapfile.c
**
**  Handle a mapping file that has been specified from
**  an EQUEL program.  Called by IImapfile().  Mappings
**  that are specified in the designated mapping file
**  have the highest precedence.
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	Created - 07/11/85 (dkh)
**	87/05/02  dave - Changed to call FD routines via passed pointers. (dkh)
**	87/04/08  joe - Added compat, dbms and fe.h
**	86/11/18  dave - Initial60 edit of files
**	85/11/20  dave - Changed to get properly obtain a LOCATION
**			 from a filename. (dkh)
**	85/11/05  dave - extern to FUNC_EXTERN for routines. (dkh)
**	85/09/18  john - Changed nat=>nat, CVna=>CVna, CVan=>CVan,
**			 u_char=>u_char
**	85/08/23  dave - Initial version in 4.0 rplus path. (dkh)
**	85/07/19  dave - Initial revision
**	05/02/87 (dkh) - Changed to call FD routines via passed pointers.
**	08/14/87 (dkh) - ER changes.
**	08/28/87 (dkh) - Changed error numbers from 8000 series to local ones.
**	19-jan-97 (bab)	Fix for bug 11178
**		Close the mapfile after use to avoid crashing the file limit.
**	01/24/90 (dkh) - Added support for sending wivew various frs command
**			 and keystroke mappings.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"
# include	<mapping.h>
# include	<lo.h>
# include	<nm.h>
# include	<si.h>
# include	<er.h>
# include	<erft.h>

FUNC_EXTERN	FILE	*FKopenkfile();
FUNC_EXTERN     VOID    IIFTwksWviewKeystrokeSetup();

GLOBALREF	i4	keyerrcnt;
GLOBALREF	char	FKerfnm[];


VOID
FTmapfile(filename)
char	*filename;
{
	FILE		*fptr;
	LOCATION	termmaploc;
	char		erfile[40];
	char		buf[MAX_LOC + 1];

	STcopy(filename, buf);
	LOfroms(PATH & FILENAME, buf, &termmaploc);
	if (SIopen(&termmaploc, ERx("r"), &fptr) != OK)
	{
		IIUGerr(E_FT002F_NOMFILE, 0, 1, filename);
	}
	else
	{
		STcopy(FKerfnm, erfile);
		STcopy(ERx("app_ingkey.err"), FKerfnm);
		keyerrcnt = 0;
		FK40parse(fptr, (i4) DEF_BY_APPL);
		if (keyerrcnt)
		{
			IIUGerr(E_FT001F_ERRSFND, 0, 2, FKerfnm, ERx(""));
		}
		STcopy(erfile, FKerfnm);

		SIclose(fptr);
	}

	/*
	**  Resend key mappings to wview.
	*/
	IIFTwksWviewKeystrokeSetup();
}
