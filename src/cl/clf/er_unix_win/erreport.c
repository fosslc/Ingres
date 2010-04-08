/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<er.h>
# include       <cs.h>      /* Needed for "erloc.h" */
# include	"erloc.h"

/*{
** Name: ERreport	- Lookup text for CL error message.
**
** Description:
**      Read the error message file looking for the matching error message
**	test.
**
** Inputs:
**      err_num                         The error number to lookup.
**
** Outputs:
**      err_buf                         The location to store error text.
**	Returns:
**	    OK
**	    ER_BADREAD
**	    ER_NOTFOUND
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	04-oct-1985 (derek)
**          Created new for 5.0.
**	9-jan-87 (daved)
**	    call ERlookup with the appropriate param list (added sys_err param).
**	13-mar-87 (peter)
**	    Add language parameter for kanji integration.
**	    Note this is hardcoded to language of English.
**	2-mar-89 (greg)
**		add num_parms arg to ERlookup()
**	12-jun-89 (jrb)
**	    Changed ERlookup call to pass NULL as fourth parm since ERlookup's
**	    interface changed.
**	26-oct-92 (andre)
**	    replaced call to ERlookup() with calls to ERslookup()
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	29-may-1997 (canor01)
**	    Clean up compiler warnings.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
STATUS
ERreport(i4 err_msg, char *err_buf)
{
    i4                  msg_size;
    i4		status;
    CL_ERR_DESC		err_code;

    status = ERlookup(err_msg, 0, 0, NULL, err_buf, ER_MAX_LEN, 
		    (i4)1, &msg_size, &err_code, 0, NULL);
    if (status)
	return (status);
    err_buf[msg_size] = 0;
    return (OK);    
}
