/******************************************************************************
**
**	Copyright (c) 1983, 1991 Ingres Corporation
**
******************************************************************************/

# include	<compat.h>
# include 	<pc.h>

/****************************************************************************** **
**	Name:
**		pccrash.c
**
**	Function:
**		PCcrashself
**
**	Arguments:
**		None
**	
**	Result:
**		KILL_PROCESS signal is sent to current process
**
**	Side Effects:
**		server will shut down (one way or another)
**
******************************************************************************/
VOID
PCcrashself()
{
    ExitProcess(-1);
}
