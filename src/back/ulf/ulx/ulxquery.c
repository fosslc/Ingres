/*
**Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ex.h>
#include    <st.h>
#include    <ulf.h>
#include    <ulx.h>

/*
**
**  Name: ULXQUERY.C -- Query Pretty-Print Functions
**
**  Description:
**	This file contains utility routines for formatting queries into
**	80 character or less sized lines.
**	    ulx_format_query - Format query into caller-supplied buffer
**
**  History:
**      02-jun-1993 (rog)
**          Initial creation.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/* Internal Constants */
#define	LINE_SIZE	80
#define	NO_DELIM	EOS
#define	SINGLE_QUOTE	'\''
#define	DOUBLE_QUOTE	'"'


/*{
** Name: ulx_format_query -- Format query into caller-supplied buffer
**
** Description:
**      Formats a query in the input buffer into the output buffer,
**	inserting newlines at appropriate places so that the lines
**	in the output buffer are mostly 80 characters or less.  
**	(If there is no convenient place to break the line, we place
**	characters in the output buffer until we find a convenient place.)
**
** Inputs:
**	inbuf				input buffer
**	inlen				length of input message
**	outlen				size of output buffer
**
** Outputs:
**	outbuf				buffer of outlen bytes to hold
**					formatted query
** Returns:
**	VOID
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	02-jun-1993 (rog)
**	    Initial creation.
**	22-jul-1993 (rog)
**	    Bug fixes: outbuf was not being incremented after we wrote to it.
*/
VOID
ulx_format_query( char *inbuf, i4  inlen, char *outbuf, i4  outlen )
{
    i4		count;
    char	*start, *lastpt;
    char	delim;

    count = 0;
    delim = NO_DELIM;
    lastpt = NULL;
    start = inbuf;
    while (inlen-- && *inbuf != EOS && outlen--)
    {
	/* Do not try to split lines in the middle of delimiter strings. */
	if (*inbuf == SINGLE_QUOTE || *inbuf == DOUBLE_QUOTE)
	{
	    switch(delim)
	    {
		case NO_DELIM:
		    delim = *inbuf;
		    break;
		case SINGLE_QUOTE:
		    if (*inbuf == SINGLE_QUOTE)
		    {
			delim = NO_DELIM;
		    }
		    break;
		case DOUBLE_QUOTE:
		    if (*inbuf == DOUBLE_QUOTE)
		    {
			delim = NO_DELIM;
		    }
		    break;
	    }
	}

	if (delim == NO_DELIM)
	{
	    switch (*inbuf)
	    {
		case ',':
		case ')':
		case ' ':
		    lastpt = inbuf;
		    break;
		case '(':
		case '+':
		case '-':
		case '*':
		case '/':
		    lastpt = inbuf - 1;
		    break;
	    }
	}

	/*
	** If we saw a newline, copy everything up to the newline, add
	** a newline, and reset all of the pointers accordingly.
	*/
	if (*inbuf == '\n')
	{
	    STncpy( outbuf, start, inbuf - start);
	    outbuf[ inbuf++ - start ] = '\0';
	    outbuf += (inbuf - ++start);
	    STcopy("\n", outbuf);
	    outbuf++;
	    start = inbuf;
	    lastpt = NULL;
	    count = 0;
	    outlen--;
	    continue;
	}

	/*
	** If we seen enough characters so far,
	** copy them and reset the pointers.
	*/
	if (count++ >= LINE_SIZE-1 && lastpt != NULL)
	{
	    count = inbuf - lastpt;
	    STncpy( outbuf, start, ++lastpt - start);
 	    outbuf[ lastpt - start ] = '\0';
	    outbuf += lastpt - start;
	    STcopy("\n", outbuf);
	    outbuf++;
	    start = lastpt;
	    outlen--;
	    lastpt = NULL;
	}

	inbuf++;
    }

    /* Copy the rest of the stuff. */
    STncpy( outbuf, start, inbuf - start);
    outbuf[ inbuf - start ] = '\0';
}
