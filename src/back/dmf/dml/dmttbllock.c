/*
**    Copyright (c) 2005, Computer Associates International, Inc.
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <sl.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <pc.h>
#include    <me.h>
#include    <tm.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <sxf.h>
#include    <adf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmxcb.h>
#include    <dmccb.h>
#include    <lg.h>
#include    <lk.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dml.h>
#include    <dm2t.h>
#include    <dmftrace.h>
#include    <dma.h>

/**
** Name: DMTTBLLOCK.C - Implements the DMF table lock operation.
**
** Description:
**      This file contains the functions that identify whether the
**      transaction has locks on a table and to release all locks
**      held by the transaction for a given table.
**      This file defines:
**
**      dmt_table_locked        - determine if the TX has locks on a table
**      dmt_release_table_locks - release the locks held by the TX
**      dmt_control_lock        - take/release Control lock on a table
**
** History:
**      13-feb-2006 (horda03)
**          Created.
**	04-Oct-2006 (jonj)
**	    Added dmt_control_lock().
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2t_? functions converted to DB_ERROR *
**/

/*
** Forward declarations of static functions:
*/

/*{
** Name:  dmt_table_locked - Is the table locked by the transaction
**
**  INTERNAL DMF call format:  status = dmt_table_locked(&dmt_cb);
**
**  EXTERNAL call format:      status = dmf_call(DMT_TABLE_LOCKED,&dmt_cb);
**
** Description:
**      Check the locks held by the transaction to see if a lock is held on the
**      specified table.
**
** Inputs:
**      dmt_cb
**          .type                           Must be set to DMT_TABLE_CB.
**          .length                         Must be at least
**                                          sizeof(DMT_TABLE_CB) bytes.
**          .dmt_db_id                      Database to search.
**          .dmt_id                         Internal table identifier.
**          .dmt_tran_id                    Transaction ID
**
** Outputs:
**      dmt_cb
**          .dmt_record_count               0 => No lock
**                                          1 => Lock held
**          .dmt_lock_mode                  Table lock mode 
**          .error.err_code                 One of the following error numbers.
**                                               E_DM0000_OK
**                                               E_DM0010_BAD_DB_ID
**                                               E_DM001A_BAD_FLAG
**                                               E_DM003B_BAD_TRAN_ID
**                                               E_DM0061_TRAN_NOT_IN_PROGRESS
**                                               and codes from LKshow
**
**      Returns:
**         E_DB_OK                          Function completed normally.
**         E_DB_WARN                        Function completed normally with a
**                                          termination status which is in
**                                          dmt_cb.err_code.
**         E_DB_ERROR                       Function completed abnormally
**                                          with a
**                                          termination status which is in
**                                          dmt_cb.err_code.
**         E_DB_FATAL                       Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmt_cb.err_code.
** History:
**      13-feb-2006 (horda03)
**	    Created.
*/
DB_STATUS
dmt_table_locked(
DMT_CB   *dmt_cb)
{
    DMT_CB          *dmt = dmt_cb;
    STATUS          cl_status;
    u_i4            context;
    DMP_DCB         *dcb;
    i4              error = E_DM0000_OK;
    i4              i;
    LK_LKB_INFO     *lkb;
    LK_LKB_INFO     lkb_info [10];
    u_i4            length;
    i4              local_error;
    DML_ODCB        *odcb;
    DB_STATUS       status = E_DB_OK;
    CL_ERR_DESC     sys_err;
    DML_XCB         *xcb;

    CLRDBERR(&dmt->error);

    xcb = (DML_XCB *)dmt->dmt_tran_id;
#ifdef xDEBUG
    if (dm0m_check((DM_OBJECT *)xcb, (i4)XCB_CB) != E_DB_OK)
#else
    if (xcb->xcb_type != (i4)XCB_CB)
#endif
    {                  
#ifdef xDEBUG
	SETDBERR(&dmt->error, 0, E_DM003B_BAD_TRAN_ID);
#else
	SETDBERR(&dmt->error, 0, E_DM0061_TRAN_NOT_IN_PROGRESS);
#endif
       status = E_DB_ERROR;
    }
    else
    {
       odcb = (DML_ODCB *)dmt->dmt_db_id;
       dcb  = odcb->odcb_dcb_ptr;

       context = 0;

       /* Assume that no lock is held */
       for(dmt->dmt_record_count = 0; (status == E_DB_OK) && !dmt->dmt_record_count;)
       {
          cl_status = LKshow(LK_S_LIST_LOCKS, xcb->xcb_lk_id, 0, 0, 
                             sizeof(lkb_info), (PTR)lkb_info, &length, 
                             &context, &sys_err); 
          if (cl_status != OK)
          {
             uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
                    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);

	     SETDBERR(&dmt->error, 0, E_DM901D_BAD_LOCK_SHOW);
             status = E_DB_ERROR;
             break;
          }

          for (i = 0; i < (i4) (length / sizeof(LK_LKB_INFO)); i++)
          {
             lkb = &lkb_info[i];

             if ( (lkb->lkb_key[0] == LK_TABLE) &&
                  (lkb->lkb_key[1] == (i4) dcb->dcb_id) &&
                  (lkb->lkb_key[2] == (i4) dmt->dmt_id.db_tab_base) &&
                  (lkb->lkb_key[3] == (i4) dmt->dmt_id.db_tab_index) )
             {
                /* This is the LKB for the specified table */

                dmt->dmt_record_count = 1;
                dmt->dmt_lock_mode = lkb->lkb_grant_mode;

                break;
             }
          }

          if (context == 0) break; /* end of list reached */
       }
    }

    if (status != E_DB_OK)
    {
       if (dmt->error.err_code > E_DM_INTERNAL)
       {
           uleFormat(&dmt->error, 0, NULL, ULE_LOG, NULL, 
		       (char * )NULL, 0L, (i4 *)NULL, &local_error, 0);
       }
   }

   return (status);
}

/*{
** Name:  dmt_release_table_locks - Release TX's locks on a table
**
**  INTERNAL DMF call format:  status = dmt_release_table_locks(&dmt_cb);
**
**  EXTERNAL call format:      status = dmf_call(DMT_RELEASE_TABLE_LOCKS,&dmt_cb);
**
** Description:
**    Remove all the locks held by a transaction on the specified table.
**
**    WARNING: It is the callers responsibility to ensure that the locks held
**             by the transaction on the specified table are no longer required. As
**             this command will release all lock types (TABLE, CONTROL, PAGE
**             ROW) which are held by the transaction on the specified table.
**
** Inputs:
**      dmt_cb
**          .type                           Must be set to DMT_TABLE_CB.
**          .length                         Must be at least
**                                          sizeof(DMT_TABLE_CB) bytes.
**          .dmt_db_id                      Database to search.
**          .dmt_id                         Internal table identifier.
**          .dmt_tran_id                    Transaction ID
**
** Outputs:
**      dmt_cb
**          .error.err_code                 One of the following error numbers.
**                                               E_DM0000_OK
**                                               E_DM0010_BAD_DB_ID
**                                               E_DM001A_BAD_FLAG
**                                               E_DM003B_BAD_TRAN_ID
**                                               E_DM0061_TRAN_NOT_IN_PROGRESS
**                                               and codes from LKrelease
**
**      Returns:
**         E_DB_OK                          Function completed normally.
**         E_DB_WARN                        Function completed normally with a
**                                          termination status which is in
**                                          dmt_cb.err_code.
**         E_DB_ERROR                       Function completed abnormally
**                                          with a
**                                          termination status which is in
**                                          dmt_cb.err_code.
**         E_DB_FATAL                       Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmt_cb.err_code.
** History:
**      13-feb-2006 (horda03)
**        Created.
*/
DB_STATUS
dmt_release_table_locks(
DMT_CB   *dmt_cb)
{
   DMT_CB          *dmt = dmt_cb;
   DMP_DCB         *dcb;
   i4              error = E_DM0000_OK;
   LK_LOCK_KEY     key;
   DML_ODCB        *odcb;
   DB_STATUS       ret = E_DB_OK;
   STATUS          status;
   CL_ERR_DESC     sys_err;
   i4              ule_error;
   DML_XCB         *xcb;

   CLRDBERR(&dmt->error);

   xcb = (DML_XCB *)dmt->dmt_tran_id;

#ifdef xDEBUG
   if (dm0m_check((DM_OBJECT *)xcb, (i4)XCB_CB) != E_DB_OK)
#else
   if (xcb->xcb_type != (i4)XCB_CB)
#endif
   {
#ifdef xDEBUG
	SETDBERR(&dmt->error, 0, E_DM003B_BAD_TRAN_ID);
#else
	SETDBERR(&dmt->error, 0, E_DM0061_TRAN_NOT_IN_PROGRESS);
#endif
       ret = E_DB_ERROR;
   }
   else
   { 
      odcb = (DML_ODCB *)dmt->dmt_db_id;
      dcb  = odcb->odcb_dcb_ptr; 

      key.lk_key1 = (i4) dcb->dcb_id;
      key.lk_key2 = (i4) dmt_cb->dmt_id.db_tab_base;
      key.lk_key3 = (i4) dmt_cb->dmt_id.db_tab_index;

      status = LKrelease( LK_PARTIAL | LK_RELEASE_TBL_LOCKS,
                          xcb->xcb_lk_id, (LK_LKID *) NULL,
                          &key,
                          (LK_VALUE *)NULL, &sys_err);

      if (status != OK)
      {
         uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
                    (char *)NULL, (i4)0, (i4 *)NULL, &ule_error, 0);
  

	 SETDBERR(&dmt->error, 0, E_DM901B_BAD_LOCK_RELEASE);
         ret = E_DB_ERROR;
      } 
   }

   if (ret != E_DB_OK)
   {
      if (dmt->error.err_code > E_DM_INTERNAL)
      {
         uleFormat(&dmt->error, 0 , NULL, ULE_LOG, NULL,
                    (char * )NULL, 0L, (i4 *)NULL, &error, 0);
      }
   }

   return (ret);
}

/*{
** Name:  dmt_control_lock - Acquire/release Table control lock.
**
**  INTERNAL DMF call format:  status = dmt_control_lock(&dmt_cb);
**
**  EXTERNAL call format:      status = dmf_call(DMT_CONTROL_LOCK,&dmt_cb);
**
** Description:
**
**	Take or release a Control lock on a table.
**
** Inputs:
**      dmt_cb
**          .type                           Must be set to DMT_TABLE_CB.
**          .length                         Must be at least
**                                          sizeof(DMT_TABLE_CB) bytes.
**          .dmt_db_id                      Database
**          .dmt_id                         Internal table identifier.
**          .dmt_tran_id                    Transaction ID
**          .dmt_lock_mode                  Control lock mode 
**						DMT_S - take the lock
**						DMT_N - release the lock
**
** Outputs:
**      dmt_cb
**          .error.err_code                 One of the following error numbers.
**                                               E_DM0000_OK
**                                               E_DM0010_BAD_DB_ID
**                                               E_DM003B_BAD_TRAN_ID
**
**      Returns:
**         E_DB_OK                          Function completed normally.
**         E_DB_WARN                        Function completed normally with a
**                                          termination status which is in
**                                          dmt_cb.err_code.
**         E_DB_ERROR                       Function completed abnormally
**                                          with a
**                                          termination status which is in
**                                          dmt_cb.err_code.
**         E_DB_FATAL                       Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmt_cb.err_code.
** History:
**      04-Oct-2006 (jonj)
**        Created.
*/
DB_STATUS
dmt_control_lock(
DMT_CB   *dmt)
{
    DML_ODCB        *odcb;
    DML_XCB         *xcb;
    DB_STATUS       status;
    i4              error;

    CLRDBERR(&dmt->error);

    xcb = (DML_XCB *)dmt->dmt_tran_id;
    odcb = (DML_ODCB *)dmt->dmt_db_id;

    if ( dm0m_check((DM_OBJECT *)odcb, (i4)ODCB_CB) == E_DB_OK )
    {
        if ( dm0m_check((DM_OBJECT *)xcb, (i4)XCB_CB) == E_DB_OK )
	{
	    status = dm2t_control(odcb->odcb_dcb_ptr,
				  dmt->dmt_id.db_tab_base,
				  xcb->xcb_lk_id,
				  dmt->dmt_lock_mode,
				  (i4)0, (i4)0,
				  &dmt->error);
	}
	else
	    SETDBERR(&dmt->error, 0, E_DM003B_BAD_TRAN_ID);
    }
    else
	SETDBERR(&dmt->error, 0, E_DM0010_BAD_DB_ID);


    if ( dmt->error.err_code > E_DM_INTERNAL )
    {
        uleFormat(&dmt->error, 0, NULL, ULE_LOG, NULL,
                    (char * )NULL, 0L, (i4 *)NULL, &error, 0);
    }

    return( (dmt->error.err_code) ? E_DB_ERROR : E_DB_OK );
}
