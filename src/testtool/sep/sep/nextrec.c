/*
**      next_record
**
**      Description:
**      subroutine to read a record from a file,
**      if 'fromLastRec' flag is set, it reads again record held
**      in holdBuf buffer.
**
**      Parameters,
**      fdesc   -   pointer to file to be read
*/

/*
** History:
**
**    29-dec-1993 (donj)
**	Split this function out of septool.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

#include <compat.h>
#include <cm.h>
#include <er.h>
#include <lo.h>
#include <si.h>
#include <st.h>

#define nextrec_c

#include <sepdefs.h>
#include <fndefs.h>

GLOBALREF    char          holdBuf [] ;
GLOBALREF    char          buffer_1 [] ;
GLOBALREF    i4            fromLastRec ;
GLOBALREF    char          cont_char [] ;
GLOBALREF    bool          in_canon ;

/* 
** Name: next_record	- read input line from file descriptor
**
** History:
**	17-Oct-2008 (wanfr01)
**	    Added comment block
**	    Added print parameter to allow printing lines as input is read, 
**	    rather than waiting until the line is merged.
*/
STATUS
next_record(FILE *fdesc, char print)
{
    STATUS                 ioerr ;
    STATUS                 got_cmmd ;
    i4                     slen ;
    i4                     j ;
    char                  *cp = NULL ;
    char                  *last_char = NULL ;
    char                  *next_last_char = NULL ;

    cp  = buffer_1;
    *cp = EOS;

    do
    {
	got_cmmd = OK;

	/*
	** ********************************************************* **
	**		Use an old record if flag set.
	** ********************************************************* **
	*/

	if (fromLastRec)
	{
	    ioerr = OK;
	    STcopy(holdBuf, buffer_1);
	    fromLastRec = 0;
	    if (print)
	    {
		append_line(cp,0);
	    }
	}
	else
	{
	    ioerr = SIgetrec(cp,TERM_LINE,fdesc);
	    if (print)
	    {
		append_line(cp,0);
	    }
	}

	slen = SEP_CMstlen(cp,0);

	/*
	** *********************************************************
	*/

	if (slen < 2) continue; /* quit on a blank line */

	if (!in_canon || (slen > TEST_LINE))
	{
	    for (last_char=cp, j=3; j<slen; j++)
		CMnext(last_char);

	    next_last_char = last_char;
	    CMnext(last_char);

	    if (CMcmpcase(last_char, cont_char) == 0)
	    {
		*last_char = EOS;
		cp = last_char;
		got_cmmd = FAIL;
	    }
	    else if ((slen >= 3) &&
		     (CMcmpcase(next_last_char, ERx("~")) == 0) &&
		     (CMcmpcase(last_char, ERx("%")) == 0))
		{
		    *next_last_char = EOS;
		    cp = next_last_char;
		    got_cmmd = FAIL;
		}
	}
    }
    while (ioerr == OK && got_cmmd == FAIL);

    in_canon = FALSE;
    return(ioerr);
}
