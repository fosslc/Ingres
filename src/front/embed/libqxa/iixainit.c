
/*
** Copyright (c) 2004 Ingres Corporation
*/


# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<er.h>
# include	<st.h>
# include	<cicl.h>
# include       <iicx.h>
# include       <iicxinit.h>  
# include       <iixautil.h>
# include       <iixainit.h>
# include       <generr.h>
# include       <erlq.h>

/**
**  Name: iixainit.h - Has the startup and shutdown routines, as well as any
**                     global structures/pointers/etc.
**
**  Description:
**      This module contains startup/shutdown routines for LIBQXA. It ensures
**      startup/init of all XA-related data structures, including those in
**      in the IICX sub-component.
**	
**  Defines:
**
**	IIxa_startup
**      IIxa_shutdown
**      
**  Notes:
**	<Specific notes and examples>
-*
**  History:
**	08-sep-1992	- First written (mani)
**	24-feb-1993 (larrym)
**	    removed IIxa_shutdown_done; not needed. 
**	09-may-2000 (thoda04)
**	    removed CI check for XA (TUXEDO, CICS, ENCINA).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
*/

GLOBALDEF  i4   IIxa_startup_done = FALSE;

/*
**   Name: IIxa_startup()
**
**   Description: 
**        The FE THREAD MAIN CB is created and initialized.
**        The IICX subcomponent is started.
**           -- FE thread mgmt CBs are created initialized.
**           -- XA data structures are created/initialized.
**
**        This routine, if called multiple times, must do the initialization
**        only ONCE for the lifetime of the process.
**
**   Inputs:
**       None.
**
**   Outputs:
**       Returns: E_DB_OK/WARN if successful. 
**
**   History:
**	25-Aug-1992 - First written (mani)
**	20-nov-1992 (larrm)
**	    Added call to IIxa_fe_set_libq_handler to allow libq to call
**	    libqxa routines.
**	28-jan-1994 (larrym)
**	    added CI check.
**	09-may-2000 (thoda04)
**	    removed CI check for XA (TUXEDO, CICS, ENCINA).
*/

DB_STATUS
IIxa_startup()
{
    DB_STATUS   status;
    STATUS	cl_status;

    if (!IIxa_startup_done) {

        /* startup the CX facility */
        status = IICXstartup();
        if (DB_FAILURE_MACRO(status))
            return( status );

        /* setup process logicals for use by both LIBQ and LIBQXA */
        status = IIxa_fe_setup_logicals();
        if (DB_FAILURE_MACRO(status))
            return( status );

	/* set LIBQ callback pointer */
	status = IIxa_fe_set_libq_handler(TRUE);
	if (DB_FAILURE_MACRO(status))
	    return( status );

	/* push shutdown routine on exit handler stack */
	PCatexit (IIxa_shutdown);

        IIxa_startup_done = TRUE;
    }
    else {
        IIxa_error(GE_WARNING, E_LQ00D0_XA_INTERNAL, 1, 
                     ERx("Multiple Startups of XA facility. Unexpected"));
        return( E_DB_WARN );
    }  
    
    return( E_DB_OK );

} /* IIxa_startup */



/*
**   Name: IIxa_shutdown()
**
**   Description: 
**        This function deletes all process-scoped data structures.
**
**   Inputs:
**       None.
**
**   Outputs:
**       Returns: E_DB_OK/WARN if successful shutdown. 
**
**   History:
**       25-Aug-1992 - First written (mani)
**	13-jan-1993 (larrym)
**	    added call to IIxa_fe_shutdown, which for now just closes the
**	    trace file.
**	16-jun-1993 (larrym)
**	    removed error message if IIxa_shutdown called more than once.
**	23-jul-1993 (larrym)
**	    move IIxa_fe_shutdown to CXshutdown to maximize tracing.
**      17-Jul-2007 (hanal04) Bug 118759
**          De-register the IIxa_fe_qry_trace_handler during XA_SHUTDOWN.
*/

DB_STATUS
IIxa_shutdown()
{
    DB_STATUS   status;

    if (IIxa_startup_done) {
        
        /* shutdown the CX facility */
        status = IICXshutdown();
        if (DB_FAILURE_MACRO(status))
            return( status );

	/* reset LIBQ callback pointer */
	status = IIxa_fe_set_libq_handler(FALSE);
	if (DB_FAILURE_MACRO(status))
	    return( status );

        /* disable the IIxa_fe_qry_trace_handler */
        status = IILQqthQryTraceHdlr(IIxa_fe_qry_trace_handler, NULL, FALSE);
        if (DB_FAILURE_MACRO(status))
            return( status );

        IIxa_startup_done = FALSE;
    }
    /* else, just return */
    
    return( E_DB_OK );

} /* IIxa_shutdown */

