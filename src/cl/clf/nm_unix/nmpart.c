/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<cm.h>
# include	<st.h>
# include	<nm.h>

/**
** Name:	nmpart.c
**
** Description:
**	This file defines:
**
**	NMsetpart	Set part name for current program
**	NMgetpart	Get part name for current program
**
** History:
**	4/91 (Mike S) Initial version
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	14-oct-94 (nanpr01)
**	    Removed #include nmerr.h. Bug # 63157. 
**	03-jun-1996 (canor01)
**	    Removed semaphore protection. Functions not used in threaded
**	    server.
**/

/* # define's */
# define MAX_PARTNAME	8	/* Largest part name allowed */

/* GLOBALDEF's */
/* extern's */

/* static's */
static char s_partname[MAX_PARTNAME + 1] = "";


/*{
** Name:	NMsetpart       Set part name for current program
**
** Description:
**
**   NMsetpart provides the ability to provide the CL, and any routines
**   that use this interface, a way to know what major subsystem is
**   calling it.
** 
**   
**   NMsetpart should be invoked as early during the life of the program
**   as possible, certainly before any routines which call its retrieval
**   counterpart (NMgetpart).
** 
**   NMsetpart is intended to be invoked once during the life of a program,
**   and will take no action (except to return an error) upon a subsequent
**   invocation.
**   
**       . The partname should be eight alphanumeric characters or less in 
**	   length.
**       . Since the part name may be used to construct filenames, it must
**         begin with an alphabetic character.
** 
** Inputs:
**	name	Part name
**
** Outputs:
**	None
**
**	Returns:
**		OK
**		NM_PRTLNG	Part name too long
**		NM_PRTFMT	Illegal part name format
**		NM_PRTNUL	Part name is a null string
**		NM_PRTALRDY	Part name already set
**
**	Exceptions:
**		none
**
** Side Effects:
**	none
**
** History:
**	4/1 (Mike S) Initial version
*/
STATUS
NMsetpart(name)
char *name;
{
	char *p;

	/* Validate part name */
	if (name == NULL || *name == EOS)
		return NM_PRTNUL;
	else if (STlength(name) > MAX_PARTNAME)
		return NM_PRTLNG;

	if (!CMalpha(name))
		return NM_PRTFMT;

	for (p = name, CMnext(p); *p != EOS; CMnext(p))
	{
		if (!CMalpha(name) && !CMdigit(name))
			return NM_PRTFMT;
	}

	/* Is part name already set? */
	if (*s_partname != EOS)
	{
		return NM_PRTALRDY;
	}

	/* The part name is OK */
	STcopy(name, s_partname);
	return OK;
	
}


/*{
** Name:	NMgetpart       Get part name for current program
**
** Description:
**   NMgetpart provides the ability to obtain the name of the invoking
**   component/part, if it's been set earlier in the life of the application
**   using NMsetpart.
**   
**   NMgetpart will fill in the passed character buffer with the name
**   of the invoking component/part, or an empty string (EOS) if it
**   has not been set earlier by NMsetpart.
**
**   If the passed buf is NULL, the function will return a pointer to
**   a static internal buffer, which may be overwritten, or NULL if
**   it has not been set earlier by NMsetpart.
**
** Inputs:
**	none
**
** Outputs:
**	buf			the name of the invoking component/part
**
**	Returns:
**		NULL		No part has been successfully set
**		Part name	Part has been set
**
**	Exceptions:
**		none
**
** Side Effects:
**	none
**
** History:
**	4/91 (Mike S)	Initial version
**	9-jun-1995 (canor01)
**	    modified to accept a passed buffer.
*/
char *
NMgetpart(buf)
char *buf;
{
	char *part;

	if (*s_partname == EOS)
		part = NULL;
	else
		part = s_partname;

	if ( buf )
	{
	    /* return NULL if not set, else return pointer to buf */
	    if ( part )
	    {
		STcopy( part, buf );
	        part = buf;
	    }
	    else
		buf[0] = EOS;
	}
	
	return part;
}
