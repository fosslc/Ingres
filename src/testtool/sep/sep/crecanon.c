/*
**    Include file
*/

#include <compat.h>
#include <si.h>
#include <cm.h>
#include <lo.h>
#include <er.h>

#ifdef VMS
#include <pc.h>
#endif

#include <termdr.h>

#define crecanon_c

#include <sepdefs.h>
#include <sepfiles.h>
#include <fndefs.h>

/*
** History:
**	27-apr-1994 (donj)
**	    Created
**	24-may-1994 (donj)
**	    Took out tracing GLOBALREF's. Unused.
**	31-may-1994 (donj)
**	    Took out #include for sepparam.h. QA_C was complaining about all
**	    the unecessary GLOBALREF's.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
**  Reference global variables
*/

GLOBALREF    WINDOW       *statusW ;
GLOBALREF    char          canonAns ;

#ifdef VMS
GLOBALREF    i4            SEPtcsubs ;
#endif

GLOBALREF    char          kfebuffer [] ;
GLOBALREF    char         *kfe_ptr;

STATUS
create_canon(SEPFILE *res_ptr)
{
    STATUS                 ret_val ;
    char                   do_com_result ;

    if ((ret_val = disp_res(res_ptr)) == OK)
    {
	put_message(statusW, NULLSTR);

	do_com_result = do_comments();
	append_line(OPEN_CANON,1);
	switch (do_com_result)
	{
	   case 'R':
	   case 'r':
			append_sepfile(res_ptr);
			break;
	   case 'E':
	   case 'e':
			break;
	   case 'I':
	   case 'i':
			append_line(SKIP_SYMBOL,1);
			break;
	}
	append_line(CLOSE_CANON,1);

	if (canonAns == EOS)
	{
	    disp_prompt(NULLSTR,NULL,NULL);
	}
	fix_cursor();
    }
    return (ret_val);
}

STATUS
create_canon_fe(SEPFILE *res_ptr)
{
    STATUS                 ret_val = OK ;   /* if OK, refresh FE screen */
    char                   do_com_result ;

#ifdef VMS
    if (SEPtcsubs < 4)
	PCsleep(1000);
#endif
    show_cursor();

    do_com_result = do_comments();
    append_line(OPEN_CANON,1);
    switch (do_com_result)
    {
       case 'R':
       case 'r':
		append_sepfile(res_ptr);
		break;
       case 'E':
       case 'e':
		break;
       case 'I':
       case 'i':
		append_line(SKIP_SYMBOL,1);
		break;
    }
    append_line(CLOSE_CANON,1);
    kfe_ptr = kfebuffer;                /* reset counter for key buffer */

    if (canonAns == EOS)
    {
	disp_prompt(NULLSTR,NULL,NULL);	/* erase prompt */
    }
    else
    {
	CMnext(kfe_ptr);
	ret_val = FAIL;			/* don't refresh FE screen */
    }
    return (ret_val);
}
