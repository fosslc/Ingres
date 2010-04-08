/*
** sepnm.c
**
**
** History:
**    23-oct-92 (DonJ)  Created.
**	 4-may-1993 (donj)
**	    NULLed out some char ptrs, Added prototyping of functions.
**	 4-may-1993 (donj)
**	    Add more prototyping.
**	19-may-1993 (donj)
**	    Added check for *cptr == EOS and pass back NULL ptr if found
**	    to make callers job simpler.
**      15-oct-1993 (donj)
**          Make function prototyping unconditional.
**	13-mar-2001	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from testtool code as the use is no longer allowed
*/

#include <compat.h>
#include <nm.h>
#include <st.h>

VOID
SEP_NMgtAt(char *name,char **ret_val,i4 mtag)
{
    char                  *cptr = NULL ;
/* 
**    Check for value of environment value "name"
*/
    NMgtAt(name, &cptr);
    if (cptr == NULL || *cptr == EOS)
	*ret_val = NULL;
    else
	*ret_val = STtalloc(mtag, cptr);

}
