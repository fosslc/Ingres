/*
**	Copyright (c) 2004 Ingres Corporation
*/

/*
** Name:	gtgkserr.c -	Get current GKS error
**
** Description:
**	GTgkserr()	Return current GKS error.
**	
**	This file should NOT include compat.h, to guaranteee that "extern" is
**	really "extern".
**
** History:
**		
*/

/*
**	On systems using VEC GKS, Gerror = Gint = int.  
**	On DG systems,		  Gerror = Gint = short.
*/
# ifdef DATAGENERAL
typedef short Gerror;
# else
typedef int Gerror;
# endif

/*{
** Name:	GTgkserr() -	Return current GKS error.
**
**
** History:
**	3/89 (Mike S)	 -- written
*/

Gerror
GTgkserr()
{
	extern Gerror gkserror;

	return gkserror;
}
