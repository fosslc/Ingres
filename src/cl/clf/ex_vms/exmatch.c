/*
**    Copyright (c) 1992, 2000 Ingres Corporation
*/
# include	<compat.h>
# include	<varargs.h>
# include	<excl.h>

/*
**	EXmatch(ex, count, ex_match_1, ex_match_2, ..., ex_match_count)
**	EX		ex;
**	i4		count;
**	EX		ex_match_1;
**	EX		ex_match_2;
**
**	Search the argument list for the matching condition value.  Return the
**	index if the matching value is found.  If not, return 0.
**
**	History:
**
**		19-oct-1992	(walt)
**			Took UNIX C version to replace VMS MACRO version.
**           25-jun-1993 (huffman)
**               Change include file references from xx.h to xxcl.h.
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**
*/

i4
EXmatch(va_alist)

va_dcl

{
	va_list	ap;
	EX target;
	i4	arg_count;
	i4 index;
	i4 ret_val = 0;

	va_start(ap);
	target = va_arg(ap, EX);
	arg_count = va_arg(ap, i4);
	for (index = 1; index <= arg_count; index++)
		if (target == va_arg(ap, EX))
		{
			ret_val = index;
			break;
		}
	va_end(ap);
	return(ret_val);
}
