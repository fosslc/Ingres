/*
**    Include file
*/
#include <compat.h>
#include <lo.h>
#include <si.h>

#define trantok_c

#include <sepdefs.h>
#include <seperrs.h>
#include <fndefs.h>


/*
** History:
**	08-Dec-1992 (DonJ)
**	    Changed call to Trans_SEPparam() from Dont_free = FALSE to TRUE.
**	    Might be causing problems on UNIX.
**      15-feb-1993 (donj)
**          Change casting of ME_tag to match casting of the args in the ME
**          functions.
**	 5-may-1993 (donj)
**	    Add function prototypes, init some pointers.
**	 7-may-1993 (donj)
**	    Add more prototyping.
**       2-jun-1993 (donj)
**          Isolated Function definitions in a new header file, fndefs.h. Added
**          definition for module, and include for fndefs.h.
**	 3-sep-1993 (donj)
**	    Add checking for NULL values for the bool prarmeters, chg_token and
**	    chg_tokens, so that calling programs can NULL it if they don't care.
**      15-oct-1993 (donj)
**          Make function prototyping unconditional.
**	14-dec-1993 (donj)
**	    Add functions to do @VALUE() processing (IS_ValueNotation() and
**	    Trans_SEPvar()) to Trans_Token().
**	30-dec-1993 (donj)
**	    Added chg_mask parameter to Trans_Token() to allow for excluding
**	    SEPPARAM translation from certain uses.
**	18-jan-1994 (donj)
**	    Put @VALUE() processing ahead of SEPPARAM processing in
**	    Trans_Token. This allows us to translate SEPPARAMS using @VALUE()
**	    where SEPPARAMS wouldn't work otherwise.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

GLOBALREF    i4            testGrade ;


STATUS
Trans_Tokens(i2 ME_tag,char *Tokens[],bool *chg_tokens)
{
    STATUS                 ret_val ;
    register i4            i ;
    bool                   chg_token ;

    if (chg_tokens != NULL)
	*chg_tokens = FALSE;

    for (i = 1 ; Tokens[i] ; i++)
    {
	ret_val = Trans_Token(ME_tag, &Tokens[i], &chg_token, TRANS_TOK_ALL);

	if (ret_val != OK)
	    return(ret_val);
	if ((chg_token == TRUE)&&(chg_tokens != NULL))
	    *chg_tokens = TRUE;
    }

    return (OK);
}


STATUS
Trans_Token(i2 ME_tag, char **Token, bool *chg_token, i4  chg_mask)
{
    STATUS                 ret_val = OK ;
    char                  *T_Token = NULL ;

    T_Token    = *Token;    

    if (chg_token != NULL)
	*chg_token = FALSE;

    if ((chg_mask&TRANS_TOK_VAR)&&(ret_val == OK))
    {
        if (IS_ValueNotation(T_Token))
        {
            if ((ret_val = Trans_SEPvar(&T_Token,TRUE,ME_tag)) != OK)
                testGrade = SEPerr_Cant_Trans_Token;
            else
            if (chg_token != NULL)
                *chg_token = TRUE;
        }
    }

    if ((chg_mask&TRANS_TOK_PARAM)&&(ret_val == OK))
    {
	if (IS_SEPparam(T_Token))
	{
	    if ((ret_val = Trans_SEPparam(&T_Token,TRUE,ME_tag)) != OK)
		testGrade = SEPerr_Cant_Trans_Token;
	    else
	    if (chg_token != NULL)
		*chg_token = TRUE;
	}
    }

    if ((chg_mask&TRANS_TOK_FILE)&&(ret_val == OK))
    {
	if (IS_FileNotation(T_Token))
	{
	    if ((ret_val = Trans_SEPfile(&T_Token,NULL,TRUE,ME_tag)) != OK)
		testGrade = SEPerr_Cant_Trans_Token;
	    else
	    if (chg_token != NULL)
		*chg_token = TRUE;
	}
    }

    *Token = T_Token;
    return (ret_val);
}
