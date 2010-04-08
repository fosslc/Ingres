/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<er.h>				 
#include	<lo.h>
#include	<me.h>
#include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>

/**
** Name:	abrtargs.c - ABF Interpreter Default Arguments Routine.
**
** Description:
**	This module contains the routine IIARload_defargs().  It loads
**	default arguments (if present) to argc and argv at startup.  It does
**	not overwrite any arguments given on the command line, it only provides
**	additional arguments.  The default arguments are found in the
**	static struct 'defarg' and are present only if 'defarg.use' is
**	TRUE.  The way this static structure gets loaded with anything
**	other than the values given below, is that when an interpreted image
**	is built, the build process scans the resulting executable file for
**	"Default ArgV Struct".  When it finds that string, it sets 'defarg.use'
**	to TRUE, puts a count of the number of default arguments in 
**	'defarg.argscnt', and puts the default arguments themselves in
**	'defarg.args' as null terminated strings.
**
**	Currently the only system making use of this is the IBM/PC.
**	On other systems, 'defarg.use' is FALSE and IIARload_defargs()
**	returns immediately.
**
** History:
**	19-sep-91 (leighb):	Initial Creation
**	02/05/92 (emerson)
**		Add missing #include <lo.h> and add various casts to
**		function calls.  Also fix the way the size of the allocated
**		argv was being computed.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-May-2009 (kschendel) b122041
**	    Compiler warning fixes (include fe.h).
*/

FUNC_EXTERN	VOID	iiARsptSetParmText();

/*
**	The following structure is used to load additional command line 
**	arguments if the 'use' switch is set and the default argument
**	id not given.  Example of command line arguments supported:
**
**		-dhuman::vt1 -fphone_or -aPHONE_OR -rPHONE_OR.RTT
*/

static	struct
{
	char	struct_marker[20];	/* Marker for exe patching */
	bool	use;			/* TRUE -> load default args */
	i4	argscnt;		/* # of defaults args in 'args' */
	char	args[256];		/* default args: null separated */
}	defarg	= { "Default ArgV Struct", FALSE, 0, "\0" };

static	char **	newargv;		/* place to build new arg ptrs   */
static	char	rttarg[MAX_LOC+3] = "-r";	/* place to build -r arg */

/*
** IIARload_defargs:
**		Handle -a command line arg;
**		Check command line args for default overrides; if no
**		override for a default arg, add it to args.
**
** Side Effect:	Adjusts argc and argv.
**
** History:
**
**	24-mar-92 (leighb) DeskTop Porting Change: 
**		Added room for NULL ptr when allocating newargv.
**	26-may-92 (leighb) DeskTop Porting Change:
**		Added -a flag processing.
**	10-sep-92 (leighb) DeskTop Porting Change:
**		Added call to OLsetup for Windows 3.x.
*/

VOID
IIARload_defargs(argcptr, argvptr)
i4   *	argcptr;
char ***argvptr;
{
	DB_TEXT_STRING *TextString;	/* Ptr to Place to save -a args */
	i4	argc = *argcptr;	/* save original argc */
	char ** argv = *argvptr;	/* save original argv */
	char *	p;			/* walk defarg.args strings */
	char *	a;			/* point to next defarg.arg */
	i4	i;			/* count argv's	*/
	i4	n;			/* count defarg.arg strings */
	i4	size;			/* size of -a args string */

#ifdef	PMFEWIN3			 
	OLsetup();			 
#endif					 

	if ( !defarg.use )
		return;

	/*
	**	Check for -a given on command line.  If so, save args
	**	following -a to be handled later.
	*/

	for (i = 0; ++i < argc; )			 
	{
		if ((*(p = argv[i]) == '-')
		 && ((*(p+1) == 'a') || (*(p+1) == 'A')))
		{
			/*
			** Save old 'argc' in 'n';
			** Set 'argc' to no longer count -a and following args
			*/

			n = argc;
			*argcptr = argc = i;

			if (i+1 == n)	/* Are there further args? */
				break;	/*	No -> break out    */

			/*
			** -a or -A is followed by further args, 
			** thus make TextString point to a
			** copy of the remaining args.
			*/

			size = -1;
			while (++i < n)
			{
				size += STlength( argv[i] ) + 1;
			}
			if ((TextString = (DB_TEXT_STRING *)FEreqmem(0,
					size + sizeof(DB_TEXT_STRING)
					     - sizeof(TextString->db_t_text),
						(bool)FALSE, (STATUS *)NULL ))
					== (DB_TEXT_STRING *)NULL)
			{
			   IIUGbmaBadMemoryAllocation(ERx("IIARload_defargs"));
			}
			TextString->db_t_count = size;
			p = (char *)TextString->db_t_text;

			for (i = argc; ++i < n; )
			{
				STcat(p, argv[i]);
				STcat(p, " ");
			}
			*(p+STlength(p)-1) = '\0';

			/*
			** Provide command-line parameters for 
			** IIARclpCmdLineParms().
			*/
			iiARsptSetParmText(TextString, 
						(DB_TEXT_STRING *(*)())NULL);
			break;
		}
	}

	/*
	**	Check command line args for default overrides; if no
	**	override for a default arg, add it to args.
	*/

	newargv = (char **)MEreqmem( (u_i4)0,
				     (u_i4)( (argc + defarg.argscnt + 2) 		 
					      * sizeof(PTR) ),
				     (bool)TRUE, (STATUS *)NULL );
	if (newargv == NULL)
		return;		/* No Memory !? */
	MEcopy((PTR)argv, (u_i2)(argc * sizeof(PTR)), (PTR)newargv);
	*argvptr = newargv;
	for (n = defarg.argscnt, p = defarg.args; n--; )
	{
		while (*p <= ' ')
			++p;
		if (((*p == '-') || (*p == '+'))
		 && ((p == defarg.args) || (*(p-1) <= ' ')))
		{
			a = p++;
			for (i = 0; ++i < argc; )
			{
				if (*p == *(argv[i]+1))
					goto nextdefarg;
			}
			newargv[(*argcptr)++] = a;
		}
nextdefarg:
		while (*p++)
			;
	}
	argv = *argvptr;
	STcopy(argv[0], rttarg+2);
	for (i = *argcptr; --i; )
	{
		if ((*argv[i] == '-') && (*(argv[i]+1) == 'r'))
			return;
	}
	/* if no -r given, use executable file */
	newargv[(*argcptr)++] = rttarg;
}
