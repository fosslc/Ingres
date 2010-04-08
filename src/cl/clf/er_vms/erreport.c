/*
**    Copyright (c) 1985, 2000 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<er.h>
#ifndef	VMS
# include	<si.h>	    /* Needed for "erloc.h", if not VMS */
#endif
# include	<cs.h>	    /* Needed for "erloc.h" */
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
**	21-may-89 (jrb)
**	    Changed ERlookup call to pass NULL as fourth parm since ERlookup's
**	    interface changed.
**	26-oct-92 (andre)
**	    replaced call to ERlookup() with calls to ERslookup()
**      16-jul-93 (ed)
**	    added gl.h
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
*/
STATUS
ERreport(err_msg, err_buf)
i4                 err_msg;
char               *err_buf;
{
    i4                 msg_size;
    i4		status;
    CL_SYS_ERR		err_code;

    status = ERslookup(err_msg, 0, 0, (char *) NULL, err_buf, ER_MAX_LEN, 
		    (i4)1, &msg_size, &err_code, 0, NULL);
    if (status)
	return (status);
    err_buf[msg_size] = 0;
    return (OK);    
}
