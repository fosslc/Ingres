/*
**  break_line
*/

/*
** History:
**
**    29-dec-1993 (donj)
**      Split this function out of septool.c
**     4-jan-1994 (donj)
**      Added some parens, re-formatted some things qa_c complained
**      about.
**     4-jan-1994 (donj)
**      Added include of si.h for VMS vaxc.
**     9-feb-1994 (donj)
**	Rewrote to work with double and single quotes better. Delimited
**	objects on command lines weren't being passed correctly.
**    25-feb-1994 (donj)
**	Only do tracing if TRACE_SEPCM is set.
**    11-may-1994 (donj)
**	Removed trace_nr GLOBALREF. Not Used.
**    18-may-1994 (donj)
**	Removed some trace statements. Moved tokptr to its own block and
**	made it a const. Made some SIfprintf()'s into SIputrec()'s. Added
**	include of me.h.
**    24-may-1994 (donj)
**	Fixed a pointer incompatibility that HP-UX's c89 was complaining about.
**      05-Dec-94 (nive)
**              For dg8_us5, used the keyword __const__ in place of const,
**              as expected by gcc with -traditional flag ( note that cc
**              is gcc -traditional )
**	22-jun-95 (allmi01)
**		Added dgi_us5 support for DG-UX on Intel based platforms following dg8_us5.
**	05-feb-96 (musro02)
**		For non-dg., changed const to READONLY 
**		(this could probably be done for dg. also).
**	28-aug-96 (kinpa04) if VMS, do not use READONLY, it causing
**		compiling errors - invalid modifier for local storage class.
**	02-feb-97 (kamal03) VAX C precompiler doesn't honor #elif. Changed
** 		to nested #if - #else - #if
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      05-Sep-2000 (hanch04)
**          replace nat and longnat with i4
**	29-jul-2002 (somsa01)
**	    Corrected line breakup on Windows in the case of delimited
**	    identifiers.
**	10-dec-2008 (wanfr01)
**	    Bug 121351
**	    Save start of token list so you can print the list in SEP_TRACE
*/

#include <compat.h>
#include <cm.h>
#include <er.h>
#include <lo.h>
#include <me.h>
#include <si.h>
#include <st.h>

#define breakln_c

#include <sepdefs.h>
#include <fndefs.h>

#define D_MAX     10

GLOBALREF    i4            tracing ;
GLOBALREF    FILE         *traceptr ;

VOID
break_line(char *string,i4 *tokens,char **toklist)
{
    register char         *cp = string ;
    register i4            cnt ;
    register i4            d_level = 0 ;
    char                  *delimiters[D_MAX] ;
    bool                   new_token = TRUE ;

    char		  **tokliststart=toklist;

    for ( cnt = 0; cnt < D_MAX; cnt++ )
    {
        delimiters[cnt] = NULL;
    }
    cnt = 0;

    if (tracing&TRACE_SEPCM)
    {
	SIfprintf(traceptr,ERx("break_line> Start. string = %s\n"),string);
	SIflush(traceptr);
    }

    while (*cp != EOS)
    {
        if (d_level == 0)
        {   /*
            **  Get rid of Leading and trailing whitespace
            */
            for ( ; ((*cp != EOS)&&CMwhite(cp)); CMnext(cp))
                ;

            if (*cp == EOS)
            {
                continue;
            }

	    if (new_token)
	    {	/*
		**  We now have the start of a new token.
		*/
		*toklist = cp;
		new_token = FALSE;
	    }
        }

#ifdef NT_GENERIC
	if (*cp == '"' && *(cp+1) == '\\' && *(cp+2) == '"')
	{
	    /*
	    ** We have the beginning of the quoted string.
	    */
	    delimiters[++d_level]
		    = (char *)SEP_MEalloc(SEP_ME_TAG_MISC, 5, TRUE,
					  (STATUS *) NULL);
	    STcopy("\\\"\"", delimiters[d_level]);
	}

	if ((d_level > 0) &&
	    (STbcompare(cp, 3, delimiters[d_level], 3, TRUE) == 0))
	{
	    /*
	    ** We have the end of the quoted string.
	    */
	    MEfree(delimiters[d_level--]);
	}
#else
        if ((CMcmpcase(cp,DBL_QUO) == 0)||(CMcmpcase(cp,SNG_QUO) == 0))
        {   /*
            **  We have either the beginning or ending of a quoted string.
            */
            if ((d_level > 0)&&(CMcmpcase(delimiters[d_level],cp) == 0))
            {   /*
                ** We have the end of the quoted string.
                */
                MEfree(delimiters[d_level--]);
            }
            else
            {   /*
                ** We have the beginning of the quoted string.
                */
                delimiters[++d_level]
		    = (char *)SEP_MEalloc(SEP_ME_TAG_MISC, 3, TRUE,
					  (STATUS *) NULL);
                CMcpychar(cp,delimiters[d_level]);
            }
        }
#endif  /* NT_GENERIC */

        CMnext(cp);

        if (d_level == 0)
        {                                   /* If we're at the top level and */
            if (CMwhite(cp)||(*cp == EOS))  /* we're either at whitespace or */
            {                               /* at the end of the line, then  */
                if (CMwhite(cp))            /* we're at the end of a token.  */
                {
                    *cp = EOS;
                    CMnext(cp);
                }
                cnt++;
                toklist++;
		new_token = TRUE;
            }
        }
    }

    *tokens = cnt;
    *toklist = NULL;
    
    if (tracing&TRACE_SEPCM)
    {
	i4                    i;
#if defined(dg8_us5) || defined(dgi_us5)
        __const__ char           **tokptr = (__const__ char **)tokliststart ;
#else
#ifdef VMS
		  char           **tokptr = ( char **)tokliststart ;

#else
	READONLY  char           **tokptr = ( READONLY char **)tokliststart ;
#endif
#endif

	SIputrec(ERx("break_line>\n"),traceptr);

	for (i=0; i<cnt; i++)
	{
            SIfprintf(traceptr,ERx("            token[%d] = %s\n"),i,tokptr[i]);
	}
	SIputrec(ERx("\nbreak_line> Ending\n"),traceptr);
	SIflush(traceptr);
    }

}
