/*
** Copyright (c) 1987, Ingres Corporation
*/

/**
** Name:	miscvalue.c	-	Return values of global values.
**
** Description:
**	This file contains several routines which simply return the value
**	of a global value.  The reason routines are needed is that
**	the global values are used by frontmain.  Frontmain is linked
**	as part of an ABF application, therefore any symbols
**	it uses must be relocatable through the tranfer vector of the
**	shared libraries.  Globalvalues can not be in the tranfer vector,
**	so these routines are used to return their values to frontmain.
**	This routine will go in the transfer vector.
**
**	IIMISCinredirect
**	IIMISCoutredirect
**	IIMISCsysin
**	IIMISCsysout
**	IIMISCsyserr
**	IIMISCcommandline
**
** 
** History
**	23-jun-1987 (Joe)
**		First Written
*/

globalvalue	iicl__inredirect;
globalvalue	iicl__outredirect;
globalvalue	iicl__sysin;
globalvalue	iicl__sysout;
globalvalue	iicl__syserr;
globalvalue	iicl__commandline;

IIMISCinredirect()
{
	return iicl__inredirect;
}

IIMISCoutredirect()
{
	return iicl__outredirect;
}

IIMISCsysin()
{
	return iicl__sysin;
}

IIMISCsysout()
{
	return iicl__sysout;
}

IIMISCsyserr()
{
	return iicl__syserr;
}

IIMISCcommandline()
{
	return iicl__commandline;
}
