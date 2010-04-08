/*
**	cfgetargs.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"

GLOBALREF char	*fcnames[];
GLOBALREF bool	fcfileopened;

/*{
** Name:	fcgetargs() -	get arguments for compfrm called from vifred.
**
** Description:
**	Parse arguments passed directly from VIFRED.
**
**	Valid flags are:
**		form		"%S"
**		file		"-o%S"
**		name		"-N%S"
**		compiled	"-F%S"
**		macro		"-m"
**		symbol		"-S%S"
**
** History:
**	7/86 (jhw) -- Modified to check both "macro" and "flags" for the "-m"
**			flag.
**	06/05/87 (dkh) - Changed to not use utexe.
**	07/09/88 (dkh) - Fixed jup bug 2867.
**	08/31/90 (dkh) - Integrated porting change ingres6202p/131116.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

fcgetargs(argc, argv)
i4	argc;
char	**argv;
{
	/* Initialize */

	fcoutfp = NULL;
	fcntable = fcnames;
	fclang = DSL_C;
	fcfrm = NULL;
	fcrti = FALSE;
	fcstdalone = FALSE;
	fcfileopened = FALSE;

	/* Set argument variables */

	for(; argc > 0; argc--, argv++)
	{
		switch (**argv)
		{
		  case '-':
			(*argv)++;
			switch (**argv)
			{
			  case 'o':
			  case 'O':
				fcoutfile = ++(*argv);
				break;

			  case 'm':
			  case 'M':
				fclang = DSL_MACRO;
				break;
			  
			  case 'N':
				(*argv)++;
				fcname = *argv;
				break;

			  case 'S':
				(*argv)++;
				fcname = *argv;
				break;
			}
			break;

		  default:
			*fcntable++ =  *argv;
			break;
		}
	}
}
