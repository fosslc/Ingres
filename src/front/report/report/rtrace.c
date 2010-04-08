/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<adf.h>
# include	<fmt.h>
#include <rtype.h>
#include <rglob.h>

GLOBALDEF i2 R_vm_trace = 0;

/*
**	debugging tracer which mainly calls FEgetvm with some
**	other information (standard prefix, cpu time) added, and
**	with a "printf" type argument list in the string.  If
**	CFEVMSTAT not set, calls are ifdef'ed out.
**
**	12/1/84 (rlm)
**
*/
r_FEtrace (s,a,b,c,d,e,f)
char *s;
i4  a,b,c,d,e,f;
{
	char buf[480];
	char cs[480];

	if (R_vm_trace != 0)
	{
		STprintf (cs,s,a,b,c,d,e,f);
		STprintf (buf,ERx("RW%d: %s (%d ms.)"),En_program,cs,TMcpu());
		FEgetvm (buf);
	}
}
