/*
**		Copyright (c) 1983,2000 Ingres Corporation
*/

#include	<compat.h>  
#include	<gl.h>
#include	<descrip.h>
#include	<lo.h>  
#include	<st.h>  
#include	<er.h>  
#include	<nm.h>  
#include	<starlet.h>

/*LOgt
**
**	Get Location.
**
**	Get Location of current process. Put value into loc.
**
**		History
**			03/09/83 -- (mmm)
**				written
**			09/13/83 -- VMS CL (dd)
**			10/89 (Mike S) Clean up
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes and
**	   external function references
*/

/* static char	*Sccsid = "@(#)LOgt.c	1.4  6/29/83"; */

FUNC_EXTERN char * NMgetenv();

LOgt(buf,loc)
char			*buf;
register LOCATION	*loc;
{
	char		dirbuf[MAX_LOC+1];	/* Buffer for directory name */
	$DESCRIPTOR	(dirdesc, dirbuf);      /* descriptor for it */
	i2		length;                 /* Its length */

	/* Get default directory -- null terminate it */
	_VOID_	sys$setddir(0, &length, &dirdesc);
	dirdesc.dsc$a_pointer[length] = NULL;

	/* Add default disk */
	STpolycat(2, NMgetenv(ERx("SYS$DISK")), dirbuf, buf);

	/* Make location */
	return (LOfroms(PATH, buf, loc));
}
