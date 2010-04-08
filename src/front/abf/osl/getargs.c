/*
** Copyright (c) 1984, 2004 Ingres Corporation
*/

#include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
#include	<lo.h>
#include	<si.h>
#include	<st.h>
#include	<er.h>
#include	<oserr.h>
#include	<osglobs.h>
#include	<osfiles.h>

/**
** Name:	getargs.c - OSL Parser Command Line Argument Parsing Module.
**
** Description:
**	Contains the routine used to parse the command line arguments for the
**	OSL parser.  Defines:
**
**	osgetargs()	parse command line arguments.
**
** History:
**	Revision 6.4
**	08/04/91 (emerson)
**		In osgetargs: Changes in support of allowing the user to enter
**		the osl or oslsql command manually.
**
**	Revision 6.0  87/06  wong
**	Modified arguments for 6.0 support.
**
**	Revision 5.1  86/10/17	16:13:37  wong
**	Added translator arguments, 'osFid' and 'osDebugIL'.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/
GLOBALREF char	*osAppname;	/* application name */

/*{
** Name:	osgetargs() -	Parse Command Line Arguments.
**
** Description:
**	Parse the command line arguments setting global OSL parser variables
**	as appropriate.
**
** Input:
**	argc	{nat}  Number of arguments
**	argv	{char **}  Argument vector
**
** Side Effects:
**	Assigns values to the global variables that correspond to the arguments.
**
** History:
**	Written (jrc)
**	09/86 (jhw) -- Added translator arguments, 'osFid' and 'osDebugIL'.
**	07/87 (jhw) -- Added compatiblity, application argument, and error
**			output argument.  Removed definition and dependency
**			arguments.
**	08/04/91 (emerson)
**		Changes in support of allowing the user to enter
**		the osl or oslsql command manually:
**		(1) Application name is now accepted in lieu of
**		    application ID on the -a flag; see also osobject.c.
**		(2) Any errors on the command line are now reported as
**		    user errors, not internal errors.
**		Also made some cosmetic changes (e.g. adding enclosing braces).
**	24-Apr-93 (fredb) hp9_mpe
**		Porting change: MPE/iX requires the extra parameters of SIfopen
**		to control the unusual behavior of the file system.
**      22-Nov-99 (hweho01)
**              Use CVal8 for 8 byte conversion on ris_u64 platform.
**	12-oct-2001 (somsa01)
**		Made previous change for NT_IA64 as well.
**	21-May-2003 (bonro01)
**		Add support for HP Itanium (i64_hpu)
**		Changes are required for all LP64 platforms.
*/

VOID
osgetargs (argc, argv)
register i4	argc;
register char	**argv;
{
    char	*iiIG_string();

    osErrfile = stderr; 	/* initialize for IBM */

    while (--argc > 0)
    {
	if ( **++argv != '-' && **argv != '+' )
	{
	    if (osIfilname == NULL)
	    {
		osIfilname = *argv;
	    }
	    else
	    {
		osuerr(E_OS000B_TooManyArgs, 0);
		/* no return */
	    }
	}
	else switch (*(*argv+1))
	{
	  /* Database Arguments (-d or -X required) */
	  case 'd':
	  case 'D':
	    osDatabase = *argv+2;
	    break;

	  case 'X':
	    osXflag = *argv;
	    break;

	  case 'u':	/* debugging, only */
	    osUflag = *argv;
	    break;

	  /* Standard Arguments */
	  case 'a':
	  case 'A':
	    /*
	    ** If the -a flag is followed by a valid decimal number,
	    ** store the number in osAppid and set osAppname to NULL.
	    ** Otherwise, assume the stuff following the -a is a name:
	    ** set osAppid to 0 and save name in osAppname;
	    ** osobjinit will ask iaom to convert it (osAppname)
	    ** to application ID (osAppid).  Note that we can't do that now
	    ** because we're not connected to the database yet.
	    */
#if defined(LP64)
            if (CVal8(*argv+2, &osAppid) == OK)
#else
	    if (CVal(*argv+2, &osAppid) == OK)
#endif
	    {
	        osAppname = (char *)NULL;
	    }
	    else
	    {
		osAppid = 0;
	        osAppname = *argv+2;
	    }
	    break;

	  case 'f':
	  case 'F':
	    osFrm = *argv+2;
	    break;

	  case 'w':
	  case 'W':
	    if ( STbcompare(*argv+2, 0, ERx("open"), 0, TRUE) == 0 )
	    {
		osChkSQL = (bool)( **argv != '-' );
	    }
	    else if ( *(*argv+2) == EOS )
	    {
		osWarning = (bool)( **argv != '-' );
	    }
	    else
	    {
		osuerr(E_OS000C_BadArg, 1, *argv);
		/* no return */
	    }
	    break;

	  case 'o':		/* name of output file */
	  case 'O':
	    osOfilname = *argv+2;
	    break;

	  /* symbol name, passed to CG. */
	  case 'n':
	  case 'N':
	    osSymbol = *argv+2;
	    break;

	  case '5':
	  case '6':
	  case '7':
	    if (STequal(*argv+2, ERx(".0")))
	    {
		osCompat = *(*argv+1) - '0';
	    }
	    break;

	  case 'g':
	  case 'G':
		osUser = FALSE;
		/*fall through*/

	  case 'l':
	  case 'L':
	    osList = TRUE;
	    if (*(*argv+2) == EOS)
	    {
		osLisfile = stdout;
	    }
	    else
	    {
		LOCATION    loc;
		char	    buf[MAX_LOC+1];

		STlcopy(osLfilname = *argv+2, buf, sizeof(buf));
		if ( LOfroms(PATH&FILENAME, buf, &loc) != OK  ||
#ifdef hp9_mpe
				SIfopen(&loc, ERx("w"), SI_TXT, 252, 
					&osLisfile) != OK)
#else
				SIopen(&loc, ERx("w"), &osLisfile) != OK)
#endif
		{
			oswarn(OSELIST, 1, buf);
			osLfilname = NULL;
			osLisfile = stdout;
		}
	    }
	    break;

	  /* Debugging Arguments */
	  case '#':
#if defined(LP64)
	    if (CVal8(*argv+2, &osFid) != OK)
#else
	    if (CVal(*argv+2, &osFid) != OK)
#endif
	    {
		osuerr(E_OS000C_BadArg, 1, *argv);
		/* no return */
	    }
	    break;

	  case 'e':
	  case 'E':
	    if (*(*argv+2) == EOS)
	    {
		osErrfile = stdout;
	    }
	    else
	    {
		LOCATION    loc;
		char	    buf[MAX_LOC+1];

		STlcopy(*argv+2, buf, sizeof(buf));
		if ( LOfroms(PATH&FILENAME, buf, &loc) != OK  ||
#ifdef hp9_mpe
				SIfopen(&loc, ERx("w"), SI_TXT, 252,
					&osErrfile) != OK)
#else
				SIopen(&loc, ERx("w"), &osErrfile) != OK)
#endif
		{
			osuerr(OSNOOUT, 1, buf);
			osErrfile = stdout;
		}
	    }
	    break;

	  case 's':
	  case 'S':
	    /*
	    ** 'osForm' must be passed through 'iiIG_string()'
	    ** since it is a name used in the program.
	    */
	    CVlower(*argv+2);
	    osForm = iiIG_string(*argv+2);
	    break;

	  case 't':
	    osDebugIL = TRUE;
	    if (*(*argv+2) == EOS)
	    {
		osOfile = stdout;
	    }
	    else
	    {
		LOCATION    loc;
		char	    buf[MAX_LOC+1];

		STlcopy(*argv+2, buf, sizeof(buf));
		if ( LOfroms(PATH&FILENAME, buf, &loc) != OK  ||
#ifdef hp9_mpe
				SIfopen(&loc, ERx("w"), SI_TXT, 252,
					&osOfile) != OK )
#else
				SIopen(&loc, ERx("w"), &osOfile) != OK )
#endif
		{
			osuerr(OSNOOUT, 1, buf);
			osOfile = stdout;
		}
	    }
	    break;

	  case 'Y':
	    yydebug = TRUE;
	    break;

	  default:
	    osuerr(E_OS000C_BadArg, 1, *argv);
	    /* no return */
	    break;
	} /* end else switch */
    } /* end command line while */
}
