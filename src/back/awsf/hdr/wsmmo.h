/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** wsmmo.h
**
** Description
**      Function prototypes for the wsmmo module.
**
** History:
**      16-Jul-98 (fanra01)
**          Created.
**      07-Sep-1998 (fanra01)
**          Corrected case of header files to match piccolo.
*/
#ifndef WSSMO_INCLUDED
#define WSSMO_INCLUDED

# include <actsession.h>
# include <usrsession.h>

FUNC_EXTERN STATUS act_define (void);

FUNC_EXTERN STATUS act_sess_attach (char * name, ACT_PSESSION  actSession);

FUNC_EXTERN VOID act_sess_detach (char * name);

FUNC_EXTERN STATUS usr_define (void);

FUNC_EXTERN STATUS usr_sess_attach (char * name, USR_PSESSION  usrSession);

FUNC_EXTERN VOID usr_sess_detach (char * name);

FUNC_EXTERN STATUS usr_trans_define (void);

FUNC_EXTERN STATUS usr_trans_attach (USR_PTRANS  usrtrans);

FUNC_EXTERN VOID usr_trans_detach (USR_PTRANS  usrtrans);

FUNC_EXTERN STATUS usr_curs_define (void);

FUNC_EXTERN STATUS usr_curs_attach (USR_PCURSOR  usrcursor);

FUNC_EXTERN VOID usr_curs_detach (USR_PCURSOR  usrcursor);

#endif
