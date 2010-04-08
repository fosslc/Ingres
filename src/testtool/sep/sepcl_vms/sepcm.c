/*
** sepcm.c
**
**
** History:
**	23-oct-92 (DonJ)  Created.
**      04-feb-1993 (donj)
**          Included lo.h because sepdefs.h now references LOCATION type.
**      15-feb-1993 (donj)
**          Change casting of ME_tag to match casting of the args in the ME
**          functions.
**	 4-may-1993 (donj)
**	    NULLed out some char ptrs, Added prototyping of functions.
**	 7-may-1993 (donj)
**	    Add more prototyping.
**       7-may-1993 (donj)
**          Fix SEP_MEalloc().
**       2-jun-1993 (donj)
**          Isolated Function definitions in a new header file, fndefs.h. Added
**          definition for module, and include for fndefs.h.
**	20-aug-1993 (donj)
**	    Added a SEP_CMwhatcase() string function that returns a "1", if the
**	    entire string is uppercase, a "-1" if the entire string is lowercase,
**	    and 0 if it's mixed case.
**      15-oct-1993 (donj)
**          Make function prototyping unconditional.
**	13-mar-2001	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from testtool code as the use is no longer allowed
**
*/

#include <compat.h>
#include <cm.h>
#include <er.h>
#include <lo.h>
#include <me.h>
#include <si.h>
#include <st.h>

#define sepcm_c

#include <sepdefs.h>
#include <fndefs.h>

GLOBALREF    i4            tracing ;
GLOBALREF    FILE         *traceptr ;

i4
SEP_CMstlen(char *str,i4 maxlen)
{
    register i4     i ;
    char           *cptr = NULL ;

    cptr = str;
    for (i=0; (*cptr != EOS) && ((maxlen == 0)||(i<maxlen)); i++)
	CMnext(cptr);

    return (i);
}


char
*SEP_CMlastchar(char *str,i4 maxlen)
{
    register i4    i ;
    char          *cptr = NULL ;
    char          *lptr = NULL ;

    lptr = str;
    cptr = str;
    for (i=0; (*cptr != EOS) && ((maxlen == 0)||(i<maxlen)); i++)
    {
	lptr = cptr;
	CMnext(cptr);
    }
    return (lptr);
}


char
*SEP_CMpackwords(i2 ME_tag,i4 max_aray,char **aray)
{
    i4              i ;
    i4              j ;
    i4              str_len ;
    char           *cptr1 = NULL ;
    char           *cptr2 = NULL ;
    char           *cptr3 = NULL ;

    for (str_len=i=0;
	 (((max_aray>0)&&(i<max_aray))||
	  ((max_aray=0)&&((aray[i]!=NULL)||(aray[i][0] == EOS)))); i++)
    {
	cptr1 = aray[i];
	if (*cptr1 == EOS)
	    continue;
	str_len += (STlength(cptr1)+1);
    }

    cptr3 = cptr2 = SEP_MEalloc(ME_tag, str_len, TRUE, (STATUS *)NULL);
    for (j=0; j<i; j++)
    {
	cptr1 = aray[j];
	if (*cptr1 == EOS)
	    continue;

	while ( *cptr1 != EOS )
	    CMcpyinc(cptr1,cptr3);

    }
    *cptr3 = EOS;

    if (tracing&TRACE_SEPCM)
    {
	SIfprintf(traceptr,ERx("Leaving SEP_CMpackwords()>\n"));
	SIfprintf(traceptr,ERx("    res = %s\n"),cptr2);
    }

    return (cptr2);
}


i4
SEP_CMwhatcase(char *str)
{
    register char  *cptr ;
    register i4     i ;

    for( cptr = str, i = -2; (*cptr != EOS)&&(i != 0); CMnext(cptr))
    {
	switch (i)
	{
	   case  0:	break;

	   case -2:	if (CMlower(cptr))
			    i = -1;
			else if (CMupper(cptr))
			    i =  1;
			break;

	   case  1:	if (CMlower(cptr))
			    i =  0;
			break;

	   case -1:	if (CMupper(cptr))
			    i =  0;
			break;

	   default:	break;
	}
    }

    if (i == -2)
	i = 0;

    return (i);
}


i4
SEP_CMgetwords(i2 ME_tag,char *src,i4 max_aray,char **aray)
{
    i4              i ;
    i4              words ;

    char           *cptr1 = NULL ;
    char           *cptr2 = NULL ;
    char           *tmp_buff = NULL ;

    bool            inside_d_quote ;

    if (tracing&TRACE_SEPCM)
    {
	SIfprintf(traceptr,ERx("\nInside SEP_CMgetwords>\n"));
	SIfprintf(traceptr,ERx("    src      = %s\n"),src);
	SIfprintf(traceptr,ERx("    max_aray = %d\n"),max_aray);
    }

    inside_d_quote = FALSE;
    words = 0;
    cptr2 = tmp_buff = SEP_MEalloc(ME_tag, STlength(src)+1, TRUE,
				   (STATUS *)NULL);

    for (cptr1 = src; ((*cptr1 != EOS)&&(words < max_aray)); CMnext(cptr1))
    {
	if (CMcmpcase(cptr1,DBL_QUO) == 0)
	{
	    if (inside_d_quote)
	    {
		inside_d_quote = FALSE;
		CMcpychar(cptr1, cptr2);
		CMnext(cptr2);
		*cptr2  = EOS;

		aray[words++] = STtalloc(ME_tag, tmp_buff);
		MEfree(tmp_buff);
		cptr2 = tmp_buff = SEP_MEalloc(ME_tag, STlength(cptr1)+1,
					       TRUE, (STATUS *)NULL);
	    }
	    else
	    {
		inside_d_quote = TRUE;
		if (*tmp_buff != EOS)	    /* Finish off previous word */
		{
		    *cptr2  = EOS;

		    aray[words++] = STtalloc(ME_tag, tmp_buff);
		    MEfree(tmp_buff);
		    cptr2 = tmp_buff = SEP_MEalloc(ME_tag, STlength(cptr1)+1,
						   TRUE, (STATUS *)NULL);
		}
		CMcpychar(cptr1, cptr2);    /* Copy the '"' into new word. */
		CMnext(cptr2);
	    }
	}
	else
	if (inside_d_quote)
        {
	    CMcpychar(cptr1, cptr2);
	    CMnext(cptr2);
	}
	else
	if (CMwhite(cptr1))
	{
	    if (*tmp_buff != EOS)
	    {
		*cptr2  = EOS;

		aray[words++] = STtalloc(ME_tag, tmp_buff);
		MEfree(tmp_buff);
		cptr2 = tmp_buff = SEP_MEalloc(ME_tag, STlength(cptr1)+1,
					       TRUE, (STATUS *)NULL);
	    }
	}
	else /* not whitespace or dbl_quo or inside_qbl_quo */
        {
	    CMcpychar(cptr1, cptr2);
	    CMnext(cptr2);
	}
    }

    if ((*tmp_buff != EOS)&&(words < max_aray))
    {
	*cptr2  = EOS;

	aray[words++] = STtalloc(ME_tag, tmp_buff);
    }

    if (tracing&TRACE_SEPCM)
    {
	SIfprintf(traceptr,ERx("Leaving SEP_CMgetwords>\n"));
	SIfprintf(traceptr,ERx("    words    = %d\n"),words);
	for (i=0; i<words; i++)
	    SIfprintf(traceptr,ERx("    aray[%d] = %s\n"),i,aray[i]);
	SIfprintf(traceptr,ERx("\n"));
    }
    MEfree(tmp_buff);
    return (words);
}
