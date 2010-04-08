/*
**  trecover.c - Write out characters to recovery filed.
**
**  write a character to the recovery file
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**
**	07/25/86 (png & a.dea) take out STlength call
**		in SIwrite because of WSC problem.
**	08/11/86 (a.dea) -- Renamed this module from
**		recover.c to trecover.c for uniqueness.
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
# include	<si.h>
 
GLOBALREF	FILE	*fdrcvfile;
GLOBALREF	bool	fdrecover;
GLOBALREF	bool	Rcv_disabled;
 
 
TDrcvwrite(ch)
u_char  ch;
{
	i4     count;
 
	if (!fdrecover || Rcv_disabled)
		return;
	SIwrite(1, &ch, &count, fdrcvfile);
}
 
TDrcvstr(str)
char    *str;
{
	i4     count;
	u_i4	len;
 
	if (!fdrecover || Rcv_disabled)
		return;
# ifdef WSC
	else
	{
        	len = STlength(str);
        	SIwrite(len, str, &count, fdrcvfile);
	}
# else
	SIwrite(STlength(str), str, &count, fdrcvfile);
# endif
}
