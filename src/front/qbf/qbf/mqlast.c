/*
**	MQLAST.c  -  Split from catalog.qc to aid overlaying.
**
**	Routines:
**	     mqLast()
**
**	Written: 11/10/83 (nml)
**
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>

GLOBALDEF	char	Lastname[FE_MAXNAME+1] = {EOS};

mqLast(name)
char	*name;
{
	STcopy(name, Lastname);
}
