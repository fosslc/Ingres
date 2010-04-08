/******************************************************************************
**
**	Copyright (c) 1983, 1991 Ingres Corporation
**
******************************************************************************/

# include	<compat.h>
# include	<pc.h>

/******************************************************************************
**
**	Name:
**		PCsleep.c
**
**	Function:
**		PCsleep
**
**	Arguments:
**		i4	msec;
**
**	Result:
**		Cause the calling process to sleep for msec milliseconds.
**		Resolution is rounded up to the nearest second.
**
**	Side Effects:
**		None
**
******************************************************************************/
VOID
PCsleep(msec)
u_i4	msec;
{
    Sleep(msec);
}
