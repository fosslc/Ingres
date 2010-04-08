/*
    Include file
*/

#include <compat.h>
#include <cm.h>
#include <st.h>
#include <si.h>
#include <er.h>
#include <lo.h>
#include <me.h>
#include <termdr.h>

#define sepparm_c

#include <sepdefs.h>
#include <fndefs.h>

/*
** History:
**	01-nov-1991 (donj)
**		Created to eliminate duplicate code in procmmd.c and sepostcl.c.
**      29-jan-1992 (donj)
**          Fix bug #42106 where sep was trying to tranlate as a sepparam anything
**	    like "s" or "sep".
**      02-apr-1992 (donj)
**          Removed unneeded references. Changed all refs to '\0' to EOS. Chang
**          increments (c++) to CM calls (CMnext(c)). Changed a "while (*c)"
**          to the standard "while (*c != EOS)". Implement other CM routines
**          as per CM documentation. Added use of FIND_SEPstring() and other
**	    constructions to fix "'sepparamdb'" where the symbol is not
**	    delimited by whitespace.
**	05-jul-1992 (donj)
**	    Added cl ME memory tags to use MEtfree() to make memory handling
**	    simpler.
**      16-jul-1992 (donj)
**          Replaced calls to MEreqmem with either STtalloc or SEP_MEalloc.
**	30-nov-1992 (donj)
**	    Handle allocated memory better. Took Find_SEPstring() out and put
**	    it into sepst.c.
**	08-Dec-1992 (donj)
**	    Changed ME tag for tmp_buffers trying to find error on UNIX. Also
**	    Changed Trans_SEPparam() to not try to free memory tagged
**	    "SEP_ME_TAG_NODEL" (nodelete).
**	08-Dec-1992 (donj)
**	    Fix a syntax error, missing ")".
**	04-feb-1993 (donj)
**	    Included lo.h because sepdefs.h now references LOCATION type.
**	15-feb-1993 (donj)
**	    Change casting of ME_tag to match casting of the args in the ME
**	    functions.
**	 5-may-1993 (donj)
**	    Add function prototypes, init some pointers.
**	 7-may-1993 (donj)
**	    Add more prototyping.
**       7-may-1993 (donj)
**          Fix SEP_MEalloc().
**       2-jun-1993 (donj)
**          Isolated Function definitions in a new header file, fndefs.h. Added
**          definition for module, and include for fndefs.h.
**      15-oct-1993 (donj)
**          Make function prototyping unconditional.
**      30-dec-1993 (donj)
**          Added GLOBALREF for mainW, screenLine, paging and batchMode.
**	    They're used by the disp_line() macros.
**	10-may-1994 (donj)
**	    Added include of sepparam.h. Deleted some unneccessary stuff.
**	11-may-1994 (donj)
**	    Removed trace_nr GLOBALREF. Not Used.
**	24-may-1994 (donj)
**	    Cleaned up a bit. To improve readability.
**	31-may-1994 (donj)
**	    Took out #include for sepparam.h. QA_C was complaining about all
**	    the unecessary GLOBALREF's.
**	27-mar-1995 (holla02)
**	    Added append_line after 'not defined' error.  If test aborts
**	    because SEPPARAMDB is undefined, the .log/.sep file will show why.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      26-Sep-2006 (hanal04)
**          Implement Bob Bonchik's for environment variables that have no
**          value. Now they get a space.
*/

/*
**  Definitons
*/

/*
**  Reference global variables
*/

GLOBALREF    i4            tracing ;

GLOBALREF    FILE         *traceptr ;

GLOBALREF    bool          batchMode ;

/*
**  trans_sepparam
**
**  Description:
**  subroutine to translate SEPPARAMxxxxx environmental variables
**  Parameters,
**  Token  ->  SEPPARAM to translate.
**
*/

STATUS
Trans_SEPparam(char **Token,bool Dont_free,i2 ME_tag)
{
    STATUS                 ret_val = OK ;
    i4                     parm_len ;

    char                   message [50] ;
    char                  *paramVal = NULL ;
    char                  *paramBuf = NULL ;
    char                  *parm_ptr = NULL ;
    char                  *tmp_buffer1 = NULL ;
    char                  *tmp_buffer2 = NULL ;
    char                  *tmp_ptr1 = NULL ;
    char                  *tmp_ptr2 = NULL ;
    char                  *temp_token = NULL ;

    Dont_free = (ME_tag == SEP_ME_TAG_NODEL && Dont_free != TRUE)
	      ? TRUE : Dont_free;

    if (tracing&TRACE_PARM)
    {
	SIfprintf(traceptr,ERx("Trans_SEPparam1> Token = %s\n"),*Token);
	SIputrec(ERx("                 Dont_free = "),traceptr);
	SIputrec(Dont_free ? ERx("TRUE\n") : ERx("FALSE\n"), traceptr);
	SIflush(traceptr);
    }

    if ((parm_ptr = FIND_SEPparam(*Token)) != NULL)
    {
        tmp_buffer1 = SEP_MEalloc(SEP_ME_TAG_NODEL, 128, TRUE, (STATUS *) NULL);
        tmp_ptr1    = tmp_buffer1;
        *tmp_ptr1   = EOS;
    
        if (parm_ptr != *Token)
        {
            for (temp_token = *Token; temp_token != parm_ptr; )
	    {
		CMcpyinc(temp_token,tmp_ptr1);
	    }
        }
    
        tmp_buffer2 = SEP_MEalloc(SEP_ME_TAG_NODEL, STlength(parm_ptr)+1, TRUE,
                                  (STATUS *) NULL);
        tmp_ptr2   = tmp_buffer2;
        *tmp_ptr2  = EOS;
        temp_token = parm_ptr;
    
	CMcpyinc(temp_token,tmp_ptr2);
	while (CMnmchar(temp_token))
	{
	    CMcpyinc(temp_token,tmp_ptr2);
	}
    
        CVupper(tmp_buffer2);
        NMgtAt(tmp_buffer2, &paramVal);
    
        if (tracing&TRACE_PARM)
        {
            SIputrec(ERx("Trans_SEPparam2> paramVal = "), traceptr);
            SIputrec(paramVal == NULL ? ERx("NULL") :
                     *paramVal == EOS ? ERx("EOS")  : paramVal, traceptr);
            SIputrec(ERx("\n"), traceptr);
            SIflush(traceptr);
        }
    
        if (paramVal != NULL)
        {
            STcat(tmp_ptr1,paramVal);
        }
        STcat(tmp_ptr1,temp_token);
    
        if ((ret_val = (paramVal != NULL)&&(*paramVal != EOS) ? OK : FAIL)
	    == OK)
        {
            paramBuf = STtalloc(ME_tag, tmp_buffer1);
            temp_token = *Token;
            *Token = paramBuf;
    
            if (tracing&TRACE_PARM)
            {
                SIfprintf(traceptr,ERx("Trans_SEPparam3> Token = %s\n"),*Token);
                SIflush(traceptr);
            }
    
            if (Dont_free != TRUE)
            {
                MEfree(temp_token);
            }
        }
        else
        {
            STprintf(message,ERx("ERROR: \"%s\" not defined!\n"),*Token);
            disp_line(message,0,0);
            append_line (message,1);
    
            /* Return a character space if the variable is not defined */
            paramBuf = STtalloc(ME_tag, tmp_buffer1);
            STcopy(ERx(" "), paramBuf);
            temp_token = *Token;
            *Token = paramBuf;
            ret_val = OK;

            if (tracing&TRACE_PARM)
            {
                SIputrec(ERx("Trans_SEPparam4> "),traceptr);
                SIputrec(message,traceptr);
                SIputrec(ERx("Trans_SEPparam5> Exiting ...\n"),traceptr);
                SIflush(traceptr);
            }

            if (Dont_free != TRUE)
            {
                MEfree(temp_token);
            }
        }
        MEfree(tmp_buffer1);
        MEfree(tmp_buffer2);
    }

    return (ret_val);
}

/* ******************************************************************** */

bool
IS_SEPparam(char *token)
{
    return (FIND_SEPparam(token) == NULL ? FALSE : TRUE);
}

char
*FIND_SEPparam(char *token)
{
    char                  *cp = NULL ;

    cp = FIND_SEPstring(token,ERx("SEPPARAM"));
    if (tracing&TRACE_PARM)
    {
	SIputrec(ERx("FIND_SEPparam> token <"),traceptr);
	SIputrec(token,traceptr);
	SIputrec(cp == NULL ? ERx("> didn't match\n") : ERx("> matched\n"),
		 traceptr);
	SIflush(traceptr);
    }
    return (cp);
}
