/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
**
**	File :	common.c
**
**	Routines common to all UDT source code.
**
**		byte_copy - copy bytes from one place to another
**		us_error  - fills in the error block according to Ingres Corp.
**			    specification.
**
##  History:
##	19-aug-1991 (lan)
##	    Created by Tim H.
##      15-sep-1993 (stevet)
##          Declared BLOB routines to be globoldef so that it works with
##          VMS share images. 
##      13-Sep-2002 (hanal04) Bug 108637
##          Mark 2232 build problems found on DR6. Include iiadd.h
##          from local directory, not include path.
##	07-Jan-2003 (hanch04)
##	    Back out change for bug 108637
##	    Previous change breaks platforms that need ALIGNMENT_REQUIRED.
##	    iiadd.h should be taken from $II_SYSTEM/ingres/files/iiadd.h
[@history_template@]...
**/
#include    <iiadd.h>
#include    "udt.h"
#define		min(a,b)	((a) <= (b) ? (a) : (b))

#ifdef __STDC__
GLOBALDEF II_ERROR_WRITER *usc_error = 0;
GLOBALDEF II_INIT_FILTER  *usc_setup_workspace = 0;
GLOBALDEF II_LO_HANDLER   *usc_lo_handler = 0;
GLOBALDEF II_LO_FILTER    *usc_lo_filter = 0;
#else
GLOBALDEF (*usc_error)() = 0;
GLOBALDEF (*usc_setup_workspace)() = 0;
GLOBALDEF (*usc_lo_handler)() = 0;
GLOBALDEF (*usc_lo_filter)() = 0;
#endif

/*{
** Name:    byte_copy	- copy bytes from one place to another
**
** Description:
**	Simply copies one string of bytes from one place to another.
*/
#ifdef __STDC__
void byte_copy(char	*c_from ,
               int	length ,
               char	*c_to )
#else
void byte_copy(c_from, length, c_to)
char	*c_from;
int	length;
char	*c_to;
#endif
{
    int			i;

    for (i = 0;
	    i < length;
	    i++, c_from++, c_to++)
    {
	*c_to = *c_from;
    }
}

/*{
** Name:    us_error -- fill in error block
**
** Description:
**	This routine merely fills in the error block according to RTI
**	specification.  It is supplied the user error and string by the caller.
**
**  Inputs:
**	scb				The scb to fill in
**	error_code			The error code to supply
**	error_string			The error string to fill in
**
**  Outputs:
**	scb->scb_error			Filled with the aforementioned
**					information.
## History:
##	13-jun-89 (fred)
##	    Created.
##      25-nov-1992 (stevet)
##          Replaced generic_error with SQLSTATE error.
##      17-aug-1993 (stevet)
##          Set er_usererr to error_code, that value
##          is put in the SQLCA error block, not er_errocde.
*/
#ifdef __STDC__
void us_error(II_SCB	   *scb ,
              long	   error_code ,
              char	   *error_string )
#else
void us_error(scb, error_code, error_string )
II_SCB	   *scb;
long	   error_code;
char	   *error_string;
#endif
{
    char *src = II_SQLSTATE_MISC_ERROR;
    char *dest = scb->scb_error.er_sqlstate_err;
    int  i;

    scb->scb_error.er_class = II_EXTERNAL_ERROR;
    scb->scb_error.er_usererr = error_code;
    for(i=0; i < II_SQLSTATE_STRING_LEN; i++, src++, dest++)
	*dest = *src;
    if ((scb->scb_error.er_ebuflen > 0)
		&& scb->scb_error.er_errmsgp)
    {
	scb->scb_error.er_emsglen = min(scb->scb_error.er_ebuflen,
					strlen(error_string));
	byte_copy(	error_string,
			scb->scb_error.er_emsglen,
			scb->scb_error.er_errmsgp);
    }
    else
    {
	scb->scb_error.er_emsglen = 0;
    }
}
