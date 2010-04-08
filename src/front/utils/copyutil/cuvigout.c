/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<si.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ooclass.h>
#include	<oosymbol.h>
#include	<oodefine.h>
#include	<cu.h>
#include	"ercu.h"


/**
** Name:	cuvigout.c -    Copy a vigraph graph out.
**
** Description:
**	Copies a vigraph graph to a copyutil file.
**
**
** History:
**	30-Jul-1987 (Joe)
**		First Written
**/

/*{
** Name:	IICUcvoCopyVigraphOut	-  Copy a vigraph graph to a file.
**
** Description:
**	Given a vigraph graph name this copies the vigraph graph's
**	definition to a copyutil file.  The layout for the fields
**	of the different vigraph graph tables is given in the CURECORD
**	for the vigraph graph objects.
**
** Inputs:
**	name		The name of the vigraph graph to copy out.
**
**	class		The class of the vigraph graph to copy out.
**			This is not used in this routine.  We copy
**			out any vigraph graph we find.
**
**	notthereok	If this is TRUE, then the vigraph graph not being
**			there is not an error.  Otherwise it is an
**			error.
**
**	fp		The file the output to.
**
** Outputs:
**	Returns:
**		OK if succeeded.
**		If notthereok is false and the vigraph graph is not present
**		returns a failure status.
**
** History:
**	30-jul-1987 (Joe)
**		Taken from 5.0 copyapp.
**      18-oct-1993 (kchin)
**          	Since the return type of OOsnd() has been changed to PTR,
**          	when being called in IICUcvoCopyVigraphOut(), its return
**          	type needs to be cast to the proper datatype.
**      06-dec-93 (smc)
**		Bug #58882
**      	Commented lint pointer truncation warning.
*/
STATUS
IICUcvoCopyVigraphOut(name, class, notthereok, fp)
char	*name;
OOID	class;
bool	notthereok;
FILE	*fp;
{
    STATUS	rval;
    OOID	gr;
    OOID	ei;
    OO_ENCODING	*en;

    /* lint truncation warning if size of ptr > OOID, but code valid */
    if ((gr = (OOID)OOsnd(OC_GRAPH, _withName, name, (char *)NULL)) == nil)
    {
	if (notthereok)
	    return OK;
	IIUGerr(E_CU0009_NO_SUCH_GRAPH, 0, 1, name);
	return E_CU0009_NO_SUCH_GRAPH;
    }
    /* lint truncation warning if size of ptr > OOID, but code valid */
    ei = (OOID)OOsnd(gr, _encode, 0, TRUE);
    en = (OO_ENCODING *)OOp(ei);
    return cu_encwrite(name, CUE_GRAPH, OC_GRAPH, en->estring, fp);
}
