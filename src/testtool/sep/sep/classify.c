/*
** classify.c
*/

/*
** History:
**
**    29-dec-1993 (donj)
**	Split this function out of septool.c
**     4-jan-1994 (donj)
**      Added some parens, re-formatted some things qa_c complained
**      about. Simplified the while-loop in classify_cmmd().
**     4-jan-1994 (donj)
**	Added include of si.h for VMS vaxc.
**    18-may-1994 (donj)
**	Added include of cv.h since we're using CVlower() here.
**    24-may-1994 (donj)
**	Improve readability.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

#include <compat.h>
#include <cv.h>
#include <lo.h>
#include <me.h>
#include <si.h>
#include <st.h>

#define classify_c

#include <sepdefs.h>
#include <fndefs.h>

i4
classify_cmmd(CMMD_NODE *master,char *acmmd)
{
    CMMD_NODE	*head = master ;
    CMMD_NODE   *next = NULL ;
    i4		 cmp ;
    char        *acmd ;
    i4           ret_nat = UNKNOWN_CMMD ;
    bool         do_it = TRUE ;

    if (master && acmmd)
    {
	acmd = STtalloc(SEP_ME_TAG_CMMDS, acmmd);
	CVlower(acmd);
	while ( do_it )
	{
	    if ((cmp = STcompare(acmd, head->cmmd_nm)) == 0)
	    {
		ret_nat = head->cmmd_id;
		do_it = FALSE;
	    }
	    else if ((next = (cmp < 0) ? head->left : head->right) != NULL)
	    {
		head = next;
	    }
	    else
	    {
		do_it = FALSE;
	    }
	}
	MEfree(acmd);
    }
    return (ret_nat);
}
