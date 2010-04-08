/*
**	Copyright (c) 1983 Ingres Corporation
*/
#include	<compat.h>
#include	<gl.h>
#include	<clconfig.h>
#include	"lolocal.h"
#include	<lo.h>
#include	<cm.h>

/*{
** Name:	LOtoes() -	Convert location...
**
**	Copies a LOCATION string into the caller-allocated 
** string pszOut, prefixing any occurrence of the 
** caller-supplied character with a backslash. This is intended 
** primarily for Windows to enable filepaths to be converted 
** from single backslash to double backslash. If LOtoes() is 
** called on a LOCATION string not containing any occurrence of 
** the caller-supplied character, the end result is that the 
** LOCATION string is copied unchanged into the 
** caller-allocated string pszOut.
**
** History:
**	28-feb-08 (smeke01) b120003
**	    Written for NT. Copied with some amendments for Unix.
*/


VOID
LOtoes(loc, psChar, pszOut)
register LOCATION	*loc;	/* location to be read from */
register char	 	*psChar;/* character to be escaped, passed as a string (need not be zero-terminated)  */
register char		*pszOut;/* pszOut will point to caller-allocated character memory */
{
    char *pszIn;

    pszIn = loc->string; 
    while (*pszIn)
    {
	if (CMcmpcase(pszIn, psChar) == 0)
	{
	    /* Insert an escape character */
	    CMcpychar(BACKSLASHSTRING, pszOut);
	    CMnext(pszOut);
	}

	CMcpyinc(pszIn, pszOut);
    }

    *pszOut = '\0';
}
