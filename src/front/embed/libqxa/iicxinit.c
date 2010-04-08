/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<si.h>		/* needed for iicxfe.h */
# include       <cv.h>
# include       <er.h>
# include       <me.h>
# include       <iicx.h>
# include       <iicxfe.h>
# include       <iicxtx.h>
# include       <iicxxa.h>
# include       <iicxinit.h>
# include       <iicxutil.h>  /* has IICXerror() */
# include       <generr.h>
# include       <ercx.h>


/*
**  Name: iicxinit.c - Has the startup and shutdown routines for the IICX
**                     sub-component.
**
**  Description:
**      This module contains startup/shutdown routines for the IICX 
**      subcomponent.
**	
**  Defines:
**
**	IICXstartup()
**      IICXshutdown()
**      
**  Notes:
**	<Specific notes and examples>
**
**  History:
**	08-sep-1992	- First written (mani)
**	23-nov-1992 (larrym)
**	    Added er.h for ERx macro.
**	09-mar-1993 (larrym)
**	    fixed bug with creating list head control block.
**	04-07-1993 (larrym,mani)
**	    modified tracking of thread association to transaction branch
**	    It used to be in the fe_thread_cb, now it's in a new transaction
**	    thread cb.
**	30-Sep-1993 (mhughes)
**	    Added creation/destruction of icas_xn_main cb's in 
**          tux_startup/shutdown
**      22-Jan-1996 (stial01)
**          Deleted tux_startup/shutdown
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

#define     IICX_FE_THREAD_ID_DEFAULT      1

GLOBALDEF   IICX_MAIN_CB             *IIcx_main_cb           = NULL;
GLOBALDEF   IICX_FE_THREAD_MAIN_CB   *IIcx_fe_thread_main_cb = NULL;
GLOBALDEF   IICX_XA_RMI_MAIN_CB      *IIcx_xa_rmi_main_cb    = NULL;
GLOBALDEF   IICX_XA_XN_MAIN_CB       *IIcx_xa_xn_main_cb     = NULL;
GLOBALDEF   IICX_XA_XN_THRD_MAIN_CB  *IIcx_xa_xn_thrd_main_cb= NULL;
GLOBALDEF   IICX_ICAS_XN_MAIN_CB     *IIcx_icas_xn_main_cb   = NULL;

static      DB_STATUS       fe_startup();
static      DB_STATUS       xa_startup();

static      DB_STATUS       fe_shutdown();
static      DB_STATUS       xa_shutdown();

/*
**   Name: IICXstartup()
**
**   Description: 
**        The IICX MAIN CB is created and initialized. All process-scoped
**        IICX structures and values are initialized. 
**
**   Inputs:
**       None.
**
**   Outputs:
**       Returns: E_DB_OK/WARN if successful.
**
**   History:
**       25-Aug-1992 - First written (mani)
*/

DB_STATUS
IICXstartup()
{
    char      ebuf[20];
    STATUS    cl_status;
    DB_STATUS db_status;

    if (IIcx_main_cb != NULL)
    {
        IICXerror(GE_WARNING, E_CX0001_INTERNAL, 1, 
                     ERx("Multiple Startups of CX facility. Unexpected"));
        return( E_DB_WARN );
    }

    if ((IIcx_main_cb = (IICX_MAIN_CB *)MEreqmem(
              (u_i4)0, (u_i4)sizeof(IICX_MAIN_CB), TRUE, 
              (STATUS *)&cl_status)) == NULL)
    {
        CVna((i4)cl_status, ebuf);
        IICXerror(GE_NO_RESOURCE, E_CX0002_CX_CB_ALLOC, 2, ebuf, 
                                   ERx("IIcx_main_cb"));
        return( E_DB_FATAL );
    }

    IIcx_main_cb->num_free_cbs      = 0;
    IIcx_main_cb->cb_free_list      = NULL;
    IIcx_main_cb->num_free_list_cbs = 0;
    IIcx_main_cb->list_cb_free_list = NULL;

    db_status = fe_startup();
    if (DB_FAILURE_MACRO(db_status))
        return( db_status );

    db_status = xa_startup();
    if (DB_FAILURE_MACRO(db_status))
        return( db_status );

    return( E_DB_OK );

} /* IICXstartup */




/*
**   Name: IICXshutdown()
**
**   Description: 
**        This function deletes all process-scoped data structures owned by
**        the IICX sub-component.
**
**   Inputs:
**       None.
**
**   Outputs:
**       Returns: E_DB_OK/WARN if successful.
**
**
**   History:
**       25-Aug-1992 - First written (mani)
*/

DB_STATUS
IICXshutdown()
{
  DB_STATUS  db_status;
  STATUS     cl_status;
  char       ebuf[20];

  db_status = xa_shutdown();
  if (DB_FAILURE_MACRO(db_status)) 
      return( db_status );

  db_status = fe_shutdown();
  if (DB_FAILURE_MACRO(db_status)) 
      return( db_status );

  if ((cl_status = MEfree((PTR)IIcx_main_cb)) != OK)
  {
      CVna((i4)cl_status, ebuf);
      IICXerror(GE_LOGICAL_ERROR, E_CX0003_CX_CB_FREE, 2, ebuf, 
                                     ERx("IIcx_main_cb"));
      return( E_DB_FATAL );
  }
  IIcx_main_cb = NULL;

  return(db_status);


} /* IICXshutdown */


/*
** All static routines below.
**
*/

/*
**   Name: fe_startup
**
**   Description: 
**        The IICX FE THREAD MAIN CB is created and initialized.
**
**   Inputs:
**       None.
**
**   Outputs:
**       Returns: E_DB_OK/WARN if successful.
**
**   History:
**       25-Aug-1992 - First written (mani)
**	10-Nov-1993 (mhughes)
**	    Added initialisation of TUXEDO-specific fields in fe_thread_main
**	12-Nov-1993 (larrym)
**	    Moved tuxedo test field into tuxedo global structure.
**	13-Dec-1993 (mhughes)
**	    Initialise process id & process type fields. Only used for tuxedo,
**	    but always there.
*/

static DB_STATUS  
fe_startup()
{
    char      ebuf[20];
    DB_STATUS db_status;
    STATUS    cl_status;
    IICX_ID   fe_thread_cx_id;
    IICX_CB   *fe_dummy_thread_cx_cb_p;

    if (IIcx_fe_thread_main_cb != NULL)
    {
        IICXerror(GE_WARNING, E_CX0001_INTERNAL, 1, 
            ERx("Multiple Startups of CX facility. fe_startup()."));
        return( E_DB_WARN );
    }

    if ((IIcx_fe_thread_main_cb = (IICX_FE_THREAD_MAIN_CB *)MEreqmem(
         (u_i4)0, (u_i4)sizeof(IICX_FE_THREAD_MAIN_CB), TRUE, 
         (STATUS *)&cl_status)) == NULL)
    {
        CVna((i4)cl_status, ebuf);
        IICXerror(GE_NO_RESOURCE, E_CX0002_CX_CB_ALLOC, 2, ebuf, 
                                   ERx("IIcx_fe_thread_main_cb"));
        return( E_DB_FATAL );
    }

    IIcx_fe_thread_main_cb->fe_thread_list              = NULL;
    IIcx_fe_thread_main_cb->num_free_fe_thread_cbs      = 0;
    IIcx_fe_thread_main_cb->fe_thread_cb_free_list      = NULL;
    IIcx_fe_thread_main_cb->fe_main_tuxedo_switch       = NULL;
    *(IIcx_fe_thread_main_cb->process_id)      		= EOS;
    *(IIcx_fe_thread_main_cb->process_type)    		= EOS;


    /* CREATE the default FE THREAD CB. There will be more than one */
    /* when LIBQ/LIBQXA goes multi-threaded.                        */

    IICXsetup_fe_thread_id_MACRO( fe_thread_cx_id, 
                                  IICX_FE_THREAD_ID_DEFAULT );

    /* Make sure there are no 'bootstrapping' problems in calling the */
    /* CX routines starting here. CHECK !!!                           */

    db_status = IICXcreate_lock_cb( &fe_thread_cx_id, NULL, 
                   &(fe_dummy_thread_cx_cb_p) );

    if (DB_FAILURE_MACRO( db_status ))
        return( db_status );

    IICXunlock_cb( fe_dummy_thread_cx_cb_p );

    return( E_DB_OK );

} /* fe_startup */



/*
**   Name: fe_shutdown
**
**   Description: 
**        The IICX FE THREAD MAIN CB is deleted.
**
**   Inputs:
**       None.
**
**   Outputs:
**       Returns: E_DB_OK/WARN if successful.
**
**   History:
**       25-Aug-1992 - First written (mani)
*/

static DB_STATUS  
fe_shutdown()
{
    char      ebuf[20];
    DB_STATUS db_status;
    STATUS    cl_status;

    db_status =  IICXfree_all_cbs( IIcx_fe_thread_main_cb->fe_thread_list );
    if (DB_FAILURE_MACRO(db_status))
    {
        IICXerror(GE_LOGICAL_ERROR, E_CX0001_INTERNAL, 1, 
            ERx("freeing thread CBs. fe_shutdown()."));
        return( db_status );
    }

    db_status =  IICXfree_all_cbs( 
                    IIcx_fe_thread_main_cb->fe_thread_cb_free_list );
    if (DB_FAILURE_MACRO(db_status))
    {
        IICXerror(GE_LOGICAL_ERROR, E_CX0001_INTERNAL, 1, 
            ERx("freeing CBs. fe_shutdown()."));
        return( db_status );
    }

    /* everything but the big-daddy main cb has been freed.  Now close
    ** the trace file, if needed. 
    */

    /* close the trace file) */
    db_status = IIxa_fe_shutdown();
    if (DB_FAILURE_MACRO(db_status))
        return( db_status );

    if ((cl_status = MEfree( (PTR)IIcx_fe_thread_main_cb )) != OK)
    {
      CVna((i4)cl_status, ebuf);
      IICXerror(GE_LOGICAL_ERROR, E_CX0003_CX_CB_FREE, 2, ebuf, 
                                     ERx("IIcx_fe_thread_main_cb"));
      return( E_DB_FATAL );
    }
    IIcx_fe_thread_main_cb = NULL;

    return( E_DB_OK );

} /* fe_shutdown */

/*
**   Name: xa_startup()
**
**   Description: 
**        The IICX XA RMI MAIN CB, XA SESSION MAIN CB, and XA XN MAIN CB are
**        created and initialized.
**
**   Inputs:
**       None.
**
**   Outputs:
**       Returns: E_DB_OK/WARN if successful.
**
**   History:
**       25-Aug-1992 - First written (mani)
**	 30-Sep-93  (mhughes)
**	    Added creation of ICAS XN MAIN CB
*/

static DB_STATUS  
xa_startup()
{
    char      ebuf[20];
    STATUS    cl_status;

    if ((IIcx_xa_rmi_main_cb != NULL) ||
        (IIcx_xa_xn_main_cb != NULL) ||
        (IIcx_xa_xn_thrd_main_cb != NULL))
    {
        IICXerror(GE_WARNING, E_CX0001_INTERNAL, 1, 
            ERx("Multiple Startups of CX facility. xa_startup()."));
        return( E_DB_WARN );
    }

    /* XA RMI structures */

    if ((IIcx_xa_rmi_main_cb = (IICX_XA_RMI_MAIN_CB *)MEreqmem(
         (u_i4)0, (u_i4)sizeof(IICX_XA_RMI_MAIN_CB), TRUE, 
         (STATUS *)&cl_status)) == NULL)
    {
        CVna((i4)cl_status, ebuf);
        IICXerror(GE_NO_RESOURCE, E_CX0002_CX_CB_ALLOC, 2, ebuf, 
                                   ERx("IIcx_xa_rmi_main_cb"));
        return( E_DB_FATAL );
    }

    IIcx_xa_rmi_main_cb->xa_rmi_list              = NULL;
    IIcx_xa_rmi_main_cb->num_open_rmis            = 0;


    /* XA XN structures */

    if ((IIcx_xa_xn_main_cb = (IICX_XA_XN_MAIN_CB *)MEreqmem(
         (u_i4)0, (u_i4)sizeof(IICX_XA_XN_MAIN_CB), TRUE, 
         (STATUS *)&cl_status)) == NULL)
    {
        CVna((i4)cl_status, ebuf);
        IICXerror(GE_NO_RESOURCE, E_CX0002_CX_CB_ALLOC, 2, ebuf, 
                                   ERx("IIcx_xa_xn_main_cb"));
        return( E_DB_FATAL );
    }

    IIcx_xa_xn_main_cb->xa_xn_list                = NULL;
    IIcx_xa_xn_main_cb->max_free_xa_xn_cbs        = 
                          IICX_XA_MAX_FREE_XA_XN_CBS;

    IIcx_xa_xn_main_cb->num_free_xa_xn_cbs        = 0;
    IIcx_xa_xn_main_cb->xa_xn_cb_free_list        = NULL;


    /* XA XN THRD structures */

    if ((IIcx_xa_xn_thrd_main_cb = (IICX_XA_XN_THRD_MAIN_CB *)MEreqmem(
         (u_i4)0, (u_i4)sizeof(IICX_XA_XN_THRD_MAIN_CB), TRUE, 
         (STATUS *)&cl_status)) == NULL)
    {
        CVna((i4)cl_status, ebuf);
        IICXerror(GE_NO_RESOURCE, E_CX0002_CX_CB_ALLOC, 2, ebuf, 
                                   ERx("IIcx_xa_xn_thrd_main_cb"));
        return( E_DB_FATAL );
    }

    IIcx_xa_xn_thrd_main_cb->xa_xn_thrd_list           = NULL;
    IIcx_xa_xn_thrd_main_cb->num_known_xids            = 0;
    IIcx_xa_xn_thrd_main_cb->max_free_xa_xn_thrd_cbs   = 
                          IICX_XA_MAX_FREE_XA_XN_THRD_CBS;

    IIcx_xa_xn_thrd_main_cb->num_free_xa_xn_thrd_cbs   = 0;
    IIcx_xa_xn_thrd_main_cb->xa_xn_thrd_cb_free_list   = NULL;

    return( E_DB_OK );

} /* xa_startup */

/*
**   Name: xa_shutdown()
**
**   Description: 
**        The IICX XA RMI MAIN CB, XA SESSION MAIN CB, and XA XN MAIN CB are
**        deleted, along with all the lists and sub-structures that are 
**        attached to them.
**
**   Inputs:
**       None.
**
**   Outputs:
**       Returns: E_DB_OK/WARN if successful.
**
**   History:
**       25-Aug-1992 - First written (mani)
*/

static DB_STATUS  
xa_shutdown()
{
    char                ebuf[20];
    DB_STATUS           db_status;
    STATUS              cl_status;
    IICX_CB             *temp_cb;
    IICX_CB             *temp1_cb;


    if ((IIcx_xa_rmi_main_cb == NULL) ||
        (IIcx_xa_xn_main_cb == NULL) ||
        (IIcx_xa_xn_thrd_main_cb == NULL))
    {
        IICXerror(GE_LOGICAL_ERROR, E_CX0001_INTERNAL, 1, 
            ERx("IIcx_xa_xxx_main_cbs NULL. Unexpected xa_shutdown()."));
        return( E_DB_FATAL );
    }


    /* Free XA RMI structures */

    if (IIcx_xa_rmi_main_cb->num_open_rmis > 0)
    {
        IICXerror(GE_WARNING, E_CX0001_INTERNAL, 1, 
            ERx("Some RMIs are still open. Unexpected. xa_shutdown()."));

       /* keep going */
    }

    db_status =  IICXfree_all_cbs( IIcx_xa_rmi_main_cb->xa_rmi_list );
    if (DB_FAILURE_MACRO(db_status))
    {
        IICXerror(GE_LOGICAL_ERROR, E_CX0001_INTERNAL, 1, 
            ERx("freeing RMI CBs. xa_shutdown()."));
        return( db_status );
    }


    if ((cl_status = MEfree( (PTR)IIcx_xa_rmi_main_cb )) != OK)
    {
      CVna((i4)cl_status, ebuf);
      IICXerror(GE_LOGICAL_ERROR, E_CX0003_CX_CB_FREE, 2, ebuf, 
                                     ERx("IIcx_xa_rmi_main_cb"));
      return( E_DB_FATAL );
    }

    IIcx_xa_rmi_main_cb = NULL;

    /* Free XA XN structures */

    db_status =  IICXfree_all_cbs( 
                    IIcx_xa_xn_main_cb->xa_xn_list );
    if (DB_FAILURE_MACRO(db_status))
    {
        IICXerror(GE_LOGICAL_ERROR, E_CX0001_INTERNAL, 1, 
            ERx("freeing XN CBs. xa_shutdown()."));
        return( db_status );
    }

    db_status =  IICXfree_all_cbs( 
                    IIcx_xa_xn_main_cb->xa_xn_cb_free_list );
    if (DB_FAILURE_MACRO(db_status))
    {
        IICXerror(GE_LOGICAL_ERROR, E_CX0001_INTERNAL, 1, 
            ERx("freeing CBs. xa_shutdown()."));
        return( db_status );
    }

    if ((cl_status = MEfree( (PTR)IIcx_xa_xn_main_cb )) != OK)
    {
      CVna((i4)cl_status, ebuf);
      IICXerror(GE_LOGICAL_ERROR, E_CX0003_CX_CB_FREE, 2, ebuf, 
                                     ERx("IIcx_xa_xn_main_cb"));
      return( E_DB_FATAL );
    }
    IIcx_xa_xn_main_cb = NULL;

    /* Free XA XN THRD structures */

    db_status =  IICXfree_all_cbs( 
                    IIcx_xa_xn_thrd_main_cb->xa_xn_thrd_list );
    if (DB_FAILURE_MACRO(db_status))
    {
        IICXerror(GE_LOGICAL_ERROR, E_CX0001_INTERNAL, 1, 
            ERx("freeing XN THRD CBs. xa_shutdown()."));
        return( db_status );
    }

    db_status =  IICXfree_all_cbs( 
                    IIcx_xa_xn_thrd_main_cb->xa_xn_thrd_cb_free_list );
    if (DB_FAILURE_MACRO(db_status))
    {
        IICXerror(GE_LOGICAL_ERROR, E_CX0001_INTERNAL, 1, 
            ERx("freeing XA XN THRD CBs. xa_shutdown()."));
        return( db_status );
    }

    if ((cl_status = MEfree( (PTR)IIcx_xa_xn_thrd_main_cb )) != OK)
    {
      CVna((i4)cl_status, ebuf);
      IICXerror(GE_LOGICAL_ERROR, E_CX0003_CX_CB_FREE, 2, ebuf, 
                                     ERx("IIcx_xa_xn_thrd_main_cb"));
      return( E_DB_FATAL );
    }
    IIcx_xa_xn_thrd_main_cb = NULL;

    return( E_DB_OK );

} /* xa_shutdown */







