/*
**      Copyright (c) 1983, 1999 Ingres Corporation
*/
# include	  <compat.h>  
# include	  <gl.h>
# include	  <si.h>  
# include	  <stinternal.h>  
# include	  <st.h>  
# include	  <stdarg.h>
/*
**	STprintf - C sprintf
**
**	Like the function 'sprintf' of the C language,
**	but return buf.
**
**	History:
**		2-15-83   - Written (jen)
**		3-8-83	  made p1 not an register var so it would
**			  compile under unix and typed the routine (cfr)
**		15-sep-1986 (Joe)
**			SIdofmt called differently.
**	30-nov-92 (pearl)
**		Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped function
**		headers.  Add include of stinternal.h, and put declaration
**		of STiflush there instead of here.  Can't prototype STprintf;
**		3rd arg is used as va_list.
**      08-feb-93 (pearl)
**              Add #include for st.h.
**      11-jun-93 (ed)
**          changed to use CL_PROTOTYPED
**
**	11/03/92 (dkh) - Changed to use the ansi c stadrag mechanism for dealing
**			 with variable number of arguments to a routine.
**
**	01-apr-93 (walt)
**		Removed FUNC_EXTERN STATUS prototype of STiflush just below
**		because it's prototyped in <stinternal.h>.
**		
**      16-jul-93 (ed)
**	    added gl.h
**	11-aug-93 (ed)
**	    unconditional prototyping
**      30-jun-98 (kinte01)
**	    added prototype for SIdofrmt
**      19-may-99 (kinte01)
**          Added casts to quite compiler warnings & change nat to i4
**          where appropriate
**	26-oct-2001 (kinte01)
**	   Clean up compiler warnings
*/

/* static char	*Sccsid = "@(#)STprintf.c	2.1  7/7/83"; */

VOID    SIdofrmt();

char *
STprintf(char *buffer, const char *fmt, ...)
{
	FILE	iobuf;
	va_list	ap2;

	iobuf._ptr = (char*)buffer;
	iobuf._cnt = MAXI2;
	va_start(ap2, fmt);
	SIdofrmt(&iobuf, fmt, STiflush, ap2);
	va_end(ap2);
	*iobuf._ptr = '\0';
	return (buffer);
}

/*
** Name:	STnprintf - VMS version of snprintf()
**
** Description:
**	VMS doesn't seem to have the handy routine snprintf(), which
**      sets a maximum length for output to a buffer.  This routine
**      emulates what is available on Unix, Linux and Windows.
**
** History:
**	08-jun-2004 (loera01)
*           Created.
*/
char *
STnprintf(char *buffer, i4 len, const char *fmt, ...)
{
	FILE	iobuf;
	va_list	ap2;

	iobuf._ptr = (char*)buffer;
        iobuf._flag = _IOSTRG;
	iobuf._cnt = len;
	va_start(ap2, fmt);
	SIdofrmt(&iobuf, fmt, STiflush, ap2);
	va_end(ap2);
        if( iobuf._cnt > 0 )
        {
  	    *iobuf._ptr = '\0';
	    return (buffer);
        }
        else
            return -1;
}

/*{
** Name:	STiflush	- Fake flush routine for ST*print
**
** Description:
**	This is a flush routine needed by SIdofrmt.
**	When called, it only has to reset the file
**	pointer count and move the character into
**	the buffer.
**
** Inputs:
**	c	The charcter to put in the buffer.
**
**	f	The file pointer for the buffer.
**
** History:
**	15-sep-1986 (Joe)
**		Written.
**      16-Jun-98 (kinte01)
**          changed return type of STiflush to VOID instead
**          of int as the routine does not return anything. Fixes
**          DEC C 5.7 compiler warning
*/
VOID
STiflush(
#ifdef CL_BUILD_WITH_PROTOS
	char	c,
	FILE	*f)
#else
        c, f)
        char c;
	FILE *f;
#endif
{
	*(f->_ptr)++ = c;
	f->_cnt = MAXI2;
}
