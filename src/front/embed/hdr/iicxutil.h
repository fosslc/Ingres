

/*--------------------------- iicxutil.h -----------------------------------*/

/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
**  Name: iicxutil.h - Utility Context Management routines. Called from
**                     other IICX modules.
**
**  History:
**      06-Oct-1998 (thoda04)
**          Added miscellaneous function prototypes
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

#ifndef  IICXUTIL_H
#define  IICXUTIL_H

/*
**
**  CB-level routines.
**
*/

/*
**
** Get a new CB from a free-list, if present. If not, allocate the CB.
**
*/

FUNC_EXTERN   DB_STATUS  IICXget_new_cb( IICX_ID        *cx_id,
                                         IICX_U_SUB_CB  *cx_sub_cb_p, 
                                         IICX_CB        **cx_cb_p_p );


/*
** This routine physically deletes a CB. Memory used by the CB is freed.
**
*/

FUNC_EXTERN   DB_STATUS  IICXfree_cb( IICX_CB  *cx_cb_p );


FUNC_EXTERN   DB_STATUS  IICXfree_all_cbs( IICX_CB  *cx_cb_p );


FUNC_EXTERN   DB_STATUS  IICXfree_sub_cb( i4             cx_type,
                                          IICX_U_SUB_CB  *cx_sub_cb_p );


/*
**  Definitions of top-level routines to add/find/delete LIST CB elements.
**  The parent CB that contains the list CB must have been LOCK'ed while
**  making the LIST CB calls.
*/

FUNC_EXTERN DB_STATUS  IICXadd_list_cb(
                          IICX_ID       *cx_id,           /* IN */
                          IICX_LIST_CB  **cx_list_cb_head /* INOUT */
                       );


FUNC_EXTERN  DB_STATUS  IICXfind_list_cb(
                           IICX_ID        *cx_id,           /* IN */
                           IICX_LIST_CB   *cx_list_cb_head, /* IN */
                           IICX_LIST_CB   **cx_list_cb_p_p  /* OUT */
                        );


FUNC_EXTERN  DB_STATUS  IICXdelete_list_cb(
                           IICX_ID        *cx_id,           /* IN */
                           IICX_LIST_CB   **cx_list_cb_head /* INOUT */
                        );

FUNC_EXTERN  DB_STATUS  IICXdelete_all_list_cbs(
                           IICX_LIST_CB   **cx_list_cb_head /* INOUT */
                        );

FUNC_EXTERN  DB_STATUS  IICXfree_all_list_cbs(
                           IICX_LIST_CB   *cx_list_cb_head  /* IN */
                        );




/* 
**  IICX sub-component miscellaneous routines. 
**
*/

FUNC_EXTERN  DB_STATUS  IICXassign_id(IICX_ID     *src_cx_id_p,
                                 IICX_ID     *dest_cx_id_p);


FUNC_EXTERN  i4    IICX_compare_id(IICX_ID     *cx_id_1_p,
                                   IICX_ID     *cx_id_2_p);


FUNC_EXTERN  VOID  IICXformat_id(IICX_ID     *cx_id,
                                 char        *text_buffer);

FUNC_EXTERN DB_STATUS  IICXget_new_cx_cb      (IICX_CB **cx_cb_p_p);
FUNC_EXTERN DB_STATUS  IICXlock_insert_cb     (IICX_CB  *cx_cb_p);
FUNC_EXTERN DB_STATUS  IICXlink_to_parents    (IICX_CB  *cx_cb_p);
FUNC_EXTERN DB_STATUS  IICXdelink_from_parents(IICX_CB  *cx_cb_p);
FUNC_EXTERN DB_STATUS  IICXunlock_remove_cb   (IICX_CB  *cx_cb_p);



/* 
**  IICX sub-component error handling routines. 
**
*/

FUNC_EXTERN  VOID  IICXerror(                           /* VARARGS */
/*                         i4   generic_errno,
                           i4   iicx_errno,
                           i4        err_param_count,
                           PTR       err_param1,
                           PTR       err_param2,
                           PTR       err_param3,
                           PTR       err_param4,
                           PTR       err_param5    */
                          );




#endif  /* ifndef iicxutil_h */

/*--------------------------- end of iicxutil.h ----------------------------*/


