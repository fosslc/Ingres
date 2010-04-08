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
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      22-sep-2000 (mcgem01)
**              More nat and longnat to i4 changes.
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
