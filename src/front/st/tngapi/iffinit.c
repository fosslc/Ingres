/*
** Copyright (c) 2004, 2005 Ingres Corporation
*/

/**
** Name: iffinit
**
** Description:
**  Module provides the functions to initialize the IFF API and its
**  associated communications interface.
**
**  During initialization three memory areas are allocated:
**      Internal context structure (ING_INS_CTX).
**      Returned context structure (ING_CTX).
**      Communications message buffer.      
**
**  Internal tracing can be enabled during initialization by the setting of
**  two environment variables:
**(E
**      II_IFF_LOG      path and filename to output log file.
**      II_IFF_TRACE    level of tracing.
**                      (see iff.h for values)
**
**  IFFinitialize   - Initializes the internal interfaces
**  IFFterminate    - Terminates and frees internal interfaces
**)E
**
** History:
**      SIR 111718
**      08-Mar-2004 (fanra01)
**          Created.
**      24-Mar-2004 (fanra01)
**          Add headers for external references.
**          Enclose strings in required ERx macro.
**      23-Apr-2004 (fanra01)
**          Update trace messages to force message on errors.
**      23-Aug-2004 (fanra01)
**          Sir 112892
**          Update comments for documentation purposes.
**      24-Sep-2004 (fanra01)
**          Sir 113152
**          Pass status from return of exposed functions through
**          iff_return_status to decrease granularity of error codes.
**      12-Oct-2004 (fanra01)
**          Modified comments to reflect return status changes.
**          Imbed example code under the example comment section.
**      24-Jan-2005 (fanra01)
**          Bug 113790
**          Update sample code embedded within the comments.
**      07-Feb-2005 (fanra01)
**          Sir 113881
**          Merge Windows and UNIX sources.
**      01-Aug-2005 (fanra01)
**          Bug 114967
**          Serialize GCA initialization.
**/
# include <compat.h>
# include <cv.h>
# include <er.h>
# include <ex.h>
# include <me.h>
# include <nm.h>
# include <qu.h>
# include <si.h>

# include <gl.h>
# include <sl.h>
# include <iicommon.h>
# include <mu.h>
# include <gca.h>
# include <gcn.h>
# include <gcm.h>
# include <gcu.h>

# include "iff.h"
# include "iffint.h"
# include "iffgca.h"

/*
** Synchronization objects
*/
static i4           iffseminit = 0;
GLOBALDEF MU_SEMAPHORE  iffinit_sem;
GLOBALREF MU_SEMAPHORE  ifftrace_sem;
GLOBALREF MU_SEMAPHORE  iffconfig_sem;

/*
** Name: iff_exhandler
**
** Description:
**  Exception handler for IFFinitialize
**      
** Inputs:
**      ex_args
**      
** Outputs:
**      None.
**      
** Returns:
**      EXDECLARE
**
** History:
**      08-Mar-2004 (fanra01)
**          Created.
*/
static STATUS
iff_exhandler(
EX_ARGS     *ex_args)
{
    char    buffer[256];
    if(EXsys_report(ex_args, buffer) != FALSE)
    {
        iff_trace( IFF_TRC_ERR, IFF_TRC_ERR, ERx("System exception: %s\n"),
            buffer );
    }
    else
    {
        iff_trace( IFF_TRC_ERR, IFF_TRC_ERR, ERx("Programmed exception\n") );
    }
    return (EXDECLARE);
}

/*{
** Name: IFFinitialize   - Initializes the internal interfaces
**
** Description:
**  Initialization of the IFF API.
**  Allocates memory for the internal context and initializes
**  the communications API.
**
**  Enables tracing for the whole of the IFF API.
**  Set
**(E
**      II_IFF_LOG      path and filename for output.
**      II_IFF_TRACE    level of tracing.
**)E
**
**  Should an unhandled error be detected the memory that has already been
**  allocated is freed.
**
**  The calling application should not access the memory allocated by this
**  function.
**
** Inputs:
**      ctx         Pointer address initialized to NULL for receiving the
**                  context memory.
**
** Outputs:
**      ctx         Context memory that should be used in subsequent API
**                  calls.
**      
** Returns:
**      status      IFF_SUCCESS             command completed 
**                  IFF_ERR_FAIL            error with no extra info 
**                  IFF_ERR_RESOURCE        error allocating resource
**                  IFF_ERR_PARAM           invalid paramter 
**                  IFF_ERR_STATE           invalid state 
**                  IFF_ERR_LOG_FAIL        unable to open trace log 
**                  IFF_ERR_COMMS           error making connection 
**
** Example:
**  # include <iff.h>
**
**  ING_CTX*    ctx = NULL;
**
**  if ((status = IFFinitialize( &ctx )) != IFF_SUCCESS)
**  {
**      printf( "Error: %d\n", status );
**  }
**
** History:
**      08-Mar-2004 (fanra01)
**          Created.
**      24-Jan-2005 (fanra01)
**          Update sample code embedded within the comments.
**      01-Aug-2005 (fanra01)
**          Add synchronization objects.
**          Protect GCA initialization and trace setup.
}*/
STATUS
IFFinitialize( ING_CTX** ctx )
{
    STATUS      status = OK;
    EX_CONTEXT  context;
    ING_CTX*    t1 = NULL;
    ING_INS_CTX* t2 = NULL;
    char*       trace = NULL;
    char*       log = NULL;
    i4          trc;
    
    EXCEPT_BEGIN
    {
        if(EXdeclare( iff_exhandler, &context ) != OK)
        {
            EXdelete();
            iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
                ERx("%08x = IFFinitialize(%p);\n"),
                status, ctx );
            if (t1 != NULL)
            {
                MEfree( (PTR)t1 );
            }
            if (t2 != NULL)
            {
                MEfree( (PTR)t2 );
            }
            /*
            ** Free semaphores on error
            */
            if (iffseminit == 1)
            {
                MUv_semaphore( &iffinit_sem );
                MUr_semaphore( &iffinit_sem );
                MUr_semaphore( &ifftrace_sem );
                MUr_semaphore( &iffconfig_sem );
            }
            return(iff_return_status( status ));
        }

        /*
        ** Initialize synchronization objects.
        */
        if (iffseminit == 0)
        {
            if ((status = MUw_semaphore( &iffinit_sem, ERx("iffinit") ))!=OK)
            {
                EXsignal( IFF_EX_SEM_ERROR, 0, 0 );
            }
            if ((status = MUw_semaphore( &ifftrace_sem, ERx("ifftrace") ))!=OK)
            {
                EXsignal( IFF_EX_SEM_ERROR, 0, 0 );
            }
            if ((status = MUw_semaphore( &iffconfig_sem, ERx("iffconfig") ))!=OK)
            {
                EXsignal( IFF_EX_SEM_ERROR, 0, 0 );
            }
            iffseminit = 1;
        }
        /*
        ** Moved global trace setup to after semaphores are created as trace
        ** calls contain reference counters.
        ** Will be unable to trace semaphore creation errors.
        */
        NMgtAt( ERx("II_IFF_LOG"), &log );
        if (log && *log)
        {
            NMgtAt( ERx("II_IFF_TRACE"), &trace );
            if (trace && *trace)
            {
                CVal( trace, &trc );
                status = iff_trace_open( log, trc, TRUE );
            }
        }
        
        if ((ctx == NULL) || (*ctx != NULL))
        {
            status = IFF_ERR_PARAM;
            iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
                ERx("Invalid context pointer\n") );
            EXsignal( IFF_EX_BAD_PARAM, 0, 0 );
        }
        iff_trace( IFF_TRC_API, IFF_TRC_API,
                ERx("IFFinitialize( %p );\n"), ctx );
        
        /*
        ** Serialize GCA initialization
        */
        MUp_semaphore( &iffinit_sem );
            
        if ((t1 = (ING_CTX*)MEreqmem( 0, sizeof(ING_CTX), TRUE, &status ))
            == NULL)
        {
            iff_trace( IFF_TRC_ERR, IFF_TRC_ERR, ERx("MEreqmem error %s\n"),
                status );
            EXsignal( IFF_EX_RESOURCE, 0, 0 );
        }
        else
        {
            if ((t2 = (ING_INS_CTX*)MEreqmem( 0, sizeof(ING_INS_CTX), TRUE,
                &status )) == NULL)
            {
                iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
                    ERx("MEreqmem error %s\n"), status );
                EXsignal( IFF_EX_RESOURCE, 0, 0 );
            }
            else
            {
                if ((status = iff_gca_init()) == OK)
                {
                    t2->metag = MEgettag();
                    t2->msg_buff = MEreqmem( t2->metag, iff_msg_max(), TRUE,
                        &status );
                    if (t2->msg_buff == NULL)
                    {
                        iff_trace( IFF_TRC_ERR, IFF_TRC_ERR, 
                            ERx("MEreqmem error %s\n"), status );
                        EXsignal( IFF_EX_RESOURCE, 0, 0 );
                    }
                    t1->instance_context = (ING_INS_CTX*)t2;
                    t2->state = IFF_INITIALIZED;
                    *ctx = t1;
                }
                else
                {
                    iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
                        ERx("Communication initialization error %x\n"), status );
                    EXsignal( IFF_EX_COMMS_ERR, 0, 0 );
                }
            }
        }
        MUv_semaphore( &iffinit_sem );
    }
    EXCEPT_END
    EXdelete();
    return(iff_return_status( status ));
}

/*{
** Name: IFFterminate    - Terminates and frees internal interfaces
**
** Description:
**  Release the communications interface and free memory
**  associated with the API session.
**
** Inputs:
**      ctx         Context memory allocated by a call IFFinitialize.
**      
** Outputs:
**      ctx         Nulled on successful return.
**      
** Returns:
**      status      IFF_SUCCESS             command completed 
**                  IFF_ERR_FAIL            error with no extra info 
**                  IFF_ERR_RESOURCE        error allocating resource
**                  IFF_ERR_PARAM           invalid paramter 
**                  IFF_ERR_STATE           invalid state 
**                  IFF_ERR_LOG_FAIL        unable to open trace log 
**                  IFF_ERR_COMMS           error making connection 
**
** Example:
**  # include <iff.h>
**
**  ING_CTX*    ctx = NULL;
**
**  if ((status = IFFinitialize( &ctx )) == IFF_SUCCESS)
**  {
**      status = IFFterminate( &ctx );
**  }
**
** History:
**      08-Mar-2004 (fanra01)
**          Created
**      01-Aug-2005 (fanra01)
**          Serialize GCA termination.
}*/
STATUS
IFFterminate( ING_CTX** ctx )
{
    STATUS      status = OK;
    EX_CONTEXT  context;
    ING_INS_CTX* t1 = NULL;
    
    EXCEPT_BEGIN
    {
        if(EXdeclare( iff_exhandler, &context ) != OK)
        {
            EXdelete();
            iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
                ERx("%08x = IFFterminate(%p);\n"),
                status, ctx );
            if (iffseminit == 1)
            {
                MUv_semaphore( &iffinit_sem );
            }
            iff_trace_close( TRUE );
            return(iff_return_status( status ));
        }
        if ((ctx == NULL) || (*ctx == NULL))
        {
            status = IFF_ERR_PARAM;
            EXsignal( IFF_EX_BAD_PARAM, 0, 0 );
        }
        if (iffseminit == 1)
        {
            MUp_semaphore( &iffinit_sem );
        }
        if ((t1 = (ING_INS_CTX*)((*ctx)->instance_context)) != NULL)
        {
            if (t1->state < IFF_INITIALIZED)
            {
                status = IFF_ERR_STATE;
                iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
                    ERx("IFFterminate Invalid state %d\n"), t1->state );
                EXsignal( IFF_EX_BAD_STATE, 0, 0 );
            }
        }
        else
        {
            status = IFF_ERR_PARAM;
            iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
                ERx("IFFterminate Invalid context\n") );
            EXsignal( IFF_EX_BAD_PARAM, 0, 0 );
        }
        iff_trace( IFF_TRC_API, t1->trclvl, ERx("IFFterminate(%p)\n"), ctx );
        if ((status = iff_gca_term()) != OK)
        {
            iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
                ERx("Communications termination error %x\n"), status );
            EXsignal( IFF_EX_COMMS_ERR, 0, 0 ); 
        }
        if (t1->trclvl > 0)
        {
            IFFtrace( *ctx, IFF_TRC_OFF, 0, NULL );
        }
        if (iffseminit == 1)
        {
            MUv_semaphore( &iffinit_sem );
        }
        MEfreetag( t1->metag );
        MEfree( (PTR)t1 );
        MEfree( (PTR)*ctx );
        *ctx = NULL;
        iff_trace_close( TRUE );
    }
    EXCEPT_END
    EXdelete();
    return(iff_return_status( status ));
}

