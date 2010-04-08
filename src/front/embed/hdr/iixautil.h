

/*--------------------------- iixautil.h -----------------------------------*/

/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
**  Name: iixautil.h - Utility routines called from other LIBQXA routines.
**
*/

#ifndef  IIXAUTIL_H
#define  IIXAUTIL_H


/*
**  Reading process logicals for use by LIBQ and LIBQXA.
**  
**  The LIBQ logicals have corresponding flags/handles in 'IIglbcb'.
**  The LIBQXA logicals have flags/handles in 'IIcx_fe_thread_main_cb'.
**
*/
FUNC_EXTERN  DB_STATUS  IIxa_fe_setup_logicals();

/*
**      Set's the IIglbcb->ii_gl_lbq_hdlr to point to the libq handler.
**      In essence, this tells libq that this is an XA application.  If
**	active == FALSE, then it resets the pointer to NULL.
*/
FUNC_EXTERN  DB_STATUS	IIxa_fe_set_libq_handler(i2 active);

/*
**  XA Tracing:  We pass the address of this function to libq so it can
**		 pass query tracing info to us.
*/
FUNC_EXTERN  VOID  IIxa_fe_qry_trace_handler(PTR cb, char *outbuf, int flag);


/* 
**  LIBQXA/IIXA sub-component error handling routines. 
**
*/

FUNC_EXTERN  VOID  IIxa_error(                           /* VARARGS */
/*                         i4   generic_errno,
                           i4   iicx_errno,
                           i4        err_param_count,
                           PTR       err_param1,
                           PTR       err_param2,
                           PTR       err_param3,
                           PTR       err_param4,
                           PTR       err_param5    */
                          );

#endif  /* ifndef iixautil_h */

/*--------------------------- end of iixautil.h ----------------------------*/


