/*
**    Copyright (c) 1993, 2000 Ingres Corporation
*/
# include	<compat.h>
# include	<excl.h>
# include	<tr.h>
# include	<lib$routines.h>

/*
**      History:
**
**           27-may-93 (walt)
**               Add a case for zero arguments.
**           25-jun-1993 (huffman)
**               Change include file references from xx.h to xxcl.h.
**	     16-jul-1993 (walt)
**		Remove check for bad arg_count before the switch statement.
**		Let the default case print a message and exit.
**	     07-nov-1994 (markg)
**		Changed call to lib$signal to not pass arg_count. This is not
**		needed as it clutters up the signal array and makes
**		signal argument handling difficult. lib$signal is documented
**		incorrectly by DEC.
**	     31-mar-1995 (dougb)
**		Integrate Mark's change from the 6.4 codeline.  Note: the
**		real problem here is in what's passed to a handler routine
**		such as EXcatch() -- we don't want the arg count twice.
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**      22-dec-2008 (stegr01)
**          Itanium VMS port
*/

VOID
EXsignal(EX ex, i4 arg_count, i4 arg1, i4 arg2, i4 arg3, 
		 i4 arg4, i4 arg5, i4 arg6)
{

	switch (arg_count)
	{

	case 0:
		lib$signal(ex);
		break;

	case 1:
		lib$signal(ex, arg1);
		break;

	case 2:
		lib$signal(ex, arg1, arg2);
		break;

	case 3:
		lib$signal(ex, arg1, arg2, arg3);
		break;

	case 4:
		lib$signal(ex, arg1, arg2, arg3, arg4);
		break;

	case 5:
		lib$signal(ex, arg1, arg2, arg3, arg4, arg5);
		break;

	case 6:
		lib$signal(ex, arg1, arg2, arg3, arg4, arg5, arg6);
		break;

 	default:
		TRdisplay("EXsignal:  arg_count is %d (should be 0-6). \
Nothing will be signaled.\n", arg_count);
		break;
	}

}

#define TYPESIG i4
void 
i_EXsetothersig (i4 signum, TYPESIG (*func)())
{
    /* do nothing */
}
