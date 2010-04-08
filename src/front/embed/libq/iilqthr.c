/*
** Copyright (c) 2008 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<me.h>
# include	<nm.h>
# include	<pc.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include       <generr.h>
# include	<iisqlca.h>
# include	<iilibq.h>
# include       <erlq.h>

/*
**  Name: iilqthr.c
**
**  Description:
**	Routines to support multi-threading and thread-local-storage.
**
**  Defines:
**	IILQthThread()	- Get control block for current thread.
**	IIsqlca()	- Get SQLCA for current thread.
**
**  History
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	08-jan-98 (mcgem01)
**	    Make sqlca a GLOBALREF rather than an extern.
**	24-mar-1999 (somsa01)
**	    Don't reset ii_gl_seterr to IIret_err() unless it is not
**	    defined. Also, filled in missing comments.
**	16-feb-2001 (kinte01)
**	    For VMS don't declare IISQLCA as GLOBALDEF.  From comment
**	    header in iisqlca.c - VMS Note: Even on VMS where GLOBALDEF 
**	    is the rule for declarations, we do not use the GLOBALDEF 
**	    storage class, as we must make this visible to user's "extern" 
**	    reference (who do not go through the compat.h which redefines 
**	    that class to globalref).  In fact, the ESQL/C preprocessors on 
**	    VMS, running under the compatlib (with the -c flag) make sure 
**	    that the SQLCA is extern and not globalref.
**      23-March-2007 (Ralph Loen) SIR 118032
**          Added II_sqlca() for compatibility with case-insensitive
**          languages.
**	10-Jun-2008 (lunbr01)  bug b120432
**	    Moved ck of II_VCH_COMPRESS_ON variable here to do once
**	    at initialization rather than for each query, which was
**	    bad for performance.
*/

#ifdef VMS
    IISQLCA	sqlca;
#else
    GLOBALREF	IISQLCA		sqlca;
#endif

static	bool		initialized = FALSE;
static	bool		init_failed = FALSE;
static	METLS_KEY	lbq_tls ZERO_FILL;
static	II_THR_CB	error_cb ZERO_FILL;

/*
** Name: IILQthThread
**
** Description:
**	Returns the thread-local-storage control block for the
**	current thread.  Global initialization occurs on the
**	first call to this routine.  The thread-local-storage
**	control block is allocated on the first call to this
**	routine by each thread.
**
**	Each thread is assigned a unique SQLCA. For backward
**	compatibility with single-threaded systems, the first
**	thread is assigned the global SQLCA.  Each subsequent
**	thread is assigned a thread-local-storage SQLCA.
**
**	If any error occurs (shouldn't happen), an error message
**	is displayed and the current process is terminated.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	II_THR_CB *	Pointer to current threads control block.
**
** History:
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	10-Jun-2008 (lunbr01)  bug xxxxx
**	    Moved ck of II_VCH_COMPRESS_ON variable here to do once
**	    at initialization rather than for each query, which was
**	    bad for performance.
*/

II_THR_CB *
IILQthThread()
{
    II_THR_CB	*thr_cb;
    IISQLCA	*th_sqlca = NULL;	/* Most threads use tls sqlca */
    PTR		ptr;
    STATUS	status;
    STATUS	error_status = OK;

    /*
    ** If initialization has failed, we won't
    ** be able to get the actual tls control
    ** block.  Just return a default static 
    ** control block so that processing of the 
    ** failure can proceed.
    */
    if ( init_failed )  return( &error_cb );

    /*
    ** GLobal initialization needs to be done prior to
    ** the start of multi-threading, but Libq has no
    ** single entry point where this can be guaranteed
    ** to be done.  This function will be called before
    ** anything of interest occurs in Libq, so we do the
    ** global initialization here.  There is a problem
    ** with concurrency, but MEtls_get() will fail if 
    ** MEtls_create() has not completed and there is 
    ** only a very small timing window which would allow
    ** two threads into the global intialization.  A
    ** semaphore which can be statically initialized is
    ** needed.
    */ 
    if ( ! initialized )
    {
        char *compress_sym;

	/*
	** Global initialization.
	*/
	initialized = TRUE;
	if ( (status = MEtls_create( &lbq_tls )) != OK )
	{
	    error_status = E_LQ00C8_TLS_ALLOC;
	    goto init_failure;
	}

	PCatexit( IIlqExit );		/* Push cleanup routine */
	IILQreReadEmbed();              /* Read LIBQ logicals */

	/*
	** If IIglbcb->ii_gl_seterr is not set, set it to the default.
	*/
	if (!IIglbcb->ii_gl_seterr)
	    IIglbcb->ii_gl_seterr = IIret_err;

	th_sqlca = &sqlca;		/* First thread gets global sqlca */
	MUi_semaphore( &IIglbcb->ii_gl_sem );
	MUi_semaphore( &IIglbcb->ii_gl_sstate.ss_sem );
	MUi_semaphore( &IIglbcb->iigl_msgtrc.ii_tr_sem );
	MUi_semaphore( &IIglbcb->iigl_svrtrc.ii_tr_sem );
	MUi_semaphore( &IIglbcb->iigl_qrytrc.ii_tr_sem );

	/* Call IIUGinit to initialize character set attribute table. */
	if ( IIUGinit() != OK )  goto init_failure;

        NMgtAt( ERx("II_VCH_COMPRESS_ON"), &compress_sym);
        if (compress_sym != NULL && (*compress_sym))
            if (*compress_sym == 'n' || *compress_sym == 'N')
                IIglbcb->ii_gl_flags |= II_G_NO_COMP;
    }

    status = MEtls_get( &lbq_tls, &ptr );
    if ( status != OK )  
    {
	error_status = E_LQ00C8_TLS_ALLOC;
	goto init_failure;
    }

    if ( ! ptr )
    {
	/*
	** Thread initialization.
	**
	** Initialize the thread-local-storage control block.
	*/
	ptr = MEreqmem( 0, sizeof( II_THR_CB ), TRUE, &status );
	thr_cb = (II_THR_CB *)ptr;

	status = MEtls_set( &lbq_tls, ptr );
	if ( ! ptr  ||  status != OK )  
	{
	    error_status = E_LQ00C8_TLS_ALLOC;
	    goto init_failure;
	}

	/*
	** Each thread needs a unique sqlca.  For single-
	** threaded applications, the global sqlca should
	** be used.  In multi-threaded applications, the
	** first thread uses the global sqlca, subsequent
	** threads use the thread-local-storage sqlca.
	*/
	thr_cb->ii_th_sqlca_ptr = th_sqlca ? th_sqlca : &thr_cb->ii_th_sqlca;

	/*
	** Initialize the ADF CB for the default (unconnected) session.
	*/
	thr_cb->ii_th_session = &thr_cb->ii_th_defsess;
	if ( IILQasAdfcbSetup( thr_cb ) != OK )  goto init_failure;
    }

    return( (II_THR_CB *)ptr );

  init_failure:

    /*
    ** Processing the initialization failure
    ** can result in recursive calls to this
    ** routine.  Setup a default control block
    ** to be used while processing the failure.
    */
    init_failed = TRUE;
    error_cb.ii_th_sqlca_ptr = &error_cb.ii_th_sqlca;
    error_cb.ii_th_session = &error_cb.ii_th_defsess;
    IILQasAdfcbSetup( &error_cb );

    /*
    ** We cannot continue.  Output error and exit.
    */
    if ( error_status != OK )
	IIlocerr( GE_NO_RESOURCE, error_status, II_ERR, 0, NULL );

    IIabort();
    PCexit( -1 );
    return( NULL );
}


/*
** Name: IIsqlca
**
** Description:
**	Returns the SQLCA assigned to the current thread.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	IISQLCA *	Threads SQLCA structure.
**
** History:
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
*/

IISQLCA *
IIsqlca()
{
    II_THR_CB	*thr_cb = IILQthThread();

    return( thr_cb->ii_th_sqlca_ptr );
}


/*
** Name: II_sqlca
**
** Description:
**	Returns the SQLCA assigned to the current thread.  This function
**      is exactly the same as IIsqlca(), but with a slightly different
**      name, so that case-insensitive languages can retrieve the SQLCA
**      value.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	IISQLCA *	Threads SQLCA structure.
**
** History:
**	20-Mar-07 (Ralph Loen)
**	    Cloned from IIsqlca().
*/

IISQLCA *
II_sqlca()
{
    II_THR_CB	*thr_cb = IILQthThread();

    return( thr_cb->ii_th_sqlca_ptr );
}
