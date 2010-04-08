/*
**	Copyright (c) 2004 Ingres Corporation
*/
#ifndef ER_H_INCLUDED
#define ER_H_INCLUDED

#include    <ercl.h>

/**CL_SPEC
** Name:	ER.h	- Define ER function externs
**
** Specification:
**
** Description:
**	Contains ER function externs
**
** History:
**	2-jun-1993 (ed)
**	    initial creation to define func_externs in glhdr
**	13-jun-1993 (ed)
**	    added ERlog 
**	1-Sep-93 (seiwald)
**	    CS option revamp: new ERoptlog() to echo startup options.
**	15-may-97 (mcgem01)
**	    Clean up compiler warnings.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	19-Aug-2009 (kschendel) 121804
**	    Allow repeated inclusion (eg by front headers).
**/

FUNC_EXTERN STATUS  ERclose(
#ifdef CL_PROTOTYPED
	    void
#endif
);

FUNC_EXTERN char *  ERget(
#ifdef CL_PROTOTYPED
	    ER_MSGID	    id
#endif
);

FUNC_EXTERN VOID    ERinit(
#ifdef CL_PROTOTYPED
	    i4		    flags,
	    STATUS	    (*p_sem_func)(),
	    STATUS	    (*v_sem_func)(),
	    STATUS	    (*i_sem_func)(),
	    VOID	    (*n_sem_func)()
#endif
);

FUNC_EXTERN STATUS  ERlangcode(
#ifdef CL_PROTOTYPED
	    char	    *language,
	    i4	    	    *code
#endif
);

FUNC_EXTERN STATUS  ERlangstr(
#ifdef CL_PROTOTYPED
	    i4		    code,
	    char	    *str
#endif
);

FUNC_EXTERN STATUS  ERslookup(
#ifdef CL_PROTOTYPED
	    i4	    msg_number,
	    CL_ERR_DESC	    *clerror,
	    i4		    flags,
	    char	    *sqlstate,
	    char	    *msg_buf,
	    i4		    msg_buf_size,
	    i4		    language,
	    i4		    *msg_length,
	    CL_ERR_DESC	    *err_code,
	    i4		    num_param,
	    ER_ARGUMENT	    *param
#endif
);

FUNC_EXTERN STATUS  ERlookup(
#ifdef CL_PROTOTYPED
	    i4	    msg_number,
	    CL_ERR_DESC	    *clerror,
	    i4		    flags,
	    i4	    *generic_error,
	    char	    *msg_buf,
	    i4		    msg_buf_size,
	    i4		    language,
	    i4		    *msg_length,
	    CL_ERR_DESC	    *err_code,
	    i4		    num_param,
	    ER_ARGUMENT	    *param
#endif
);

FUNC_EXTERN STATUS  ERrelease(
#ifdef CL_PROTOTYPED
	    ER_CLASS	    class_no
#endif
);

FUNC_EXTERN STATUS  ERreport(
#ifdef CL_PROTOTYPED
	    STATUS	    err_msg,
	    char	    *err_buf
#endif
);

FUNC_EXTERN STATUS  ERsend(
#ifdef CL_PROTOTYPED
	    i4		    flag,
	    char	    *message, 
	    i4		    msg_length, 
	    CL_SYS_ERR	    *err_code
#endif
);

FUNC_EXTERN STATUS ERlog(
#ifdef CL_PROTOTYPED
	    char            *message,
	    i4              msg_length,
	    CL_SYS_ERR      *err_code
#endif
);

FUNC_EXTERN VOID ERoptlog( 
	    char 	*option,
	    char 	*value 
) ;

#endif
