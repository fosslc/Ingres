/*
** Copyright (c) 2004, 2005 Ingres Corporation
*/

/**
** Name: ifftrace
**
** Description:
**  Module provides functions to enable tracing globally or per session.
**(E
**      iff_trace_open  - Opens a trace file
**      iff_trace_close - Close a trace file
**      iff_trace       - Context based tracing
**      IFFtrace        - Enable/disable tracing
**)E
**
** History:
**      24-Mar-2004 (fanra01)
**          History comments added.
**          Enclose strings in required ERx macro.
**      24-Apr-2004 (fanra01)
**          Update trace messages to force error messages.
**      23-Aug-2004 (fanra01)
**          Sir 112892
**          Update comments for documentation purposes.
**          Pass status from return of exposed functions through
**          iff_return_status to decrease granularity of error codes.
**      12-Oct-2004 (fanra01)
**          Imbed example code under the example comment section.
**      24-Jan-2005 (fanra01)
**          Bug 113790
**          Update sample code embedded within the comments.
**      07-Feb-2005 (fanra01)
**          Sir 113881
**          Merge Windows and UNIX sources.
**      01-Aug-2005 (fanra01)
**          Bug 114967
**          Add semaphore protection reference counts used in opening and
**          closing the trace file.  Trace file is per process and cannot be
**          changed while opened.
**          Updated trace messages.
**/
# include <compat.h>
# include <er.h>
# include <ex.h>
# include <pc.h>
# include <qu.h>
# include <si.h>
# include <st.h>
# include <tr.h>
# include <mu.h>

# include <iiapi.h>

# include "iff.h"
# include "iffint.h"

# define MAX_FMT    256
# define MAX_TRC    512

static i4           trcref = 0;
static i4           trclvl = 0;
static char*        trcind[] = 
{
    ERx("LOG "),
    ERx("ERR "),
    ERx("API "),
    ERx("CALL"),
    NULL
};

GLOBALDEF MU_SEMAPHORE  ifftrace_sem;

/*{
** Name: iff_trace_open  - Opens a trace file
**
** Description:
**  Opens a trace output file and maintains a reference count
**  of requests.
**      
** Inputs:
**      path        file specification to trace output
**      level       global trace level
**      global      indicator of trace scope
**      
** Outputs:
**      None
**      
** Returns:
**      OK          command completed successfully
**      !OK         error creating trace output file
**      
** History:
**      09-Mar-2004 (fanra01)
**          Created.
}*/
STATUS
iff_trace_open( char* path, i4 level, i4 global )
{
    STATUS status = OK;
    CL_ERR_DESC system_status;
    PID pid;

    /*
    ** Include PID in trace messages.
    */
    PCpid(&pid);
    
    /*
    ** Protect reference count update
    */
    MUp_semaphore( &ifftrace_sem );
    if (trcref == 0)
    {
        if (global == TRUE)
        {
            trclvl = (level > 0) ? level : 0;
        }
        if( path == NULL)
        {
            EXsignal( IFF_EX_BAD_PARAM, 0, 0 );
        }
        status = TRset_file( TR_F_APPEND, path, STlength(path), 
           &system_status );
        TRdisplay(ERx("%@ %08x-%08x --- :TR_F_APPEND\n"), pid, PCtid() );
        if (status != OK)
        {
            EXsignal( IFF_EX_LOG_FAIL, 0, 0 );
        }
    }
    trcref+=1;
    TRdisplay(ERx("%@ %08x-%08x --- :Trace begin %s\n"), pid, PCtid(),
        (global == TRUE) ? "global" : "local");
    MUv_semaphore( &ifftrace_sem );
    return(status);
}

/*{
** Name: iff_trace_close - Close a trace file
**
** Description:
**  Closes a trace output file and maintains a reference count
**  of closures.
**      
** Inputs:
**      global      indicator of trace scope
**                  file is closed immediately if set.
**      
** Outputs:
**      None
**      
** Returns:
**      None.
**
** History:
**      09-Mar-2004 (fanra01)
**          Created.
}*/
VOID
iff_trace_close( i4 global )
{
    CL_ERR_DESC system_status;
    PID pid;

    /*
    ** Include PID in trace messages.
    */
    PCpid(&pid);

    /*
    ** Protect reference count update
    */
    MUp_semaphore( &ifftrace_sem );
    trcref-=1;
    TRdisplay(ERx("%@ %08x-%08x --- :Trace end %s\n"), pid, PCtid(),
        (global == TRUE)?"global":"local" );
    if (trcref == 0)
    {
        TRdisplay(ERx("%@ %08x-%08x --- :TR_F_CLOSE\n"), pid, PCtid() );
        TRset_file( TR_F_CLOSE, NULL, 0, &system_status );
    }
    MUv_semaphore( &ifftrace_sem );
}

/*{
** Name: iff_trace       - Context based tracing
**
** Description:
**  Trace function  -   prefix of timestamp, process id and indicator
**  to formatted output
**      
** Inputs:
**      trc         Trace message level
**      tracelvl    Trace level setting
**      fmt         Message format
**      
** Outputs:
**      None
**      
** Returns:
**      None
**      
** History:
**      09-Mar-2004 (fanra01)
**          Created.
**      01-Aug-2005 (fanra01)
**          Add PID to trace messages.
}*/
VOID
iff_trace( i4 trc, i4 tracelvl, char* fmt, ... )
{
    char    lbuf[MAX_FMT];
    char    mbuf[MAX_TRC];
    PID     pid;
    va_list p; 

    if ((trc <= tracelvl) || (trc <= trclvl))
    {
        PCpid(&pid);
        va_start(p, fmt);
        sprintf(lbuf, ERx("%%%%@ %08X-%08X %s:%s"), pid, PCtid(), trcind[trc], fmt);
        vsprintf( mbuf, lbuf, p );
        TRdisplay( mbuf );
    }
}

/*
** Name: iff_exhandler
**
** Description:
**  Exception handler for IFFtrace
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
        iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
            ERx("IFFtrace system exception: %s\n"), buffer);
    }
    else
    {
        iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
            ERx("IFFtrace programmed exception\n"));
    }
    return (EXDECLARE);
}

/*{
** Name: IFFtrace        - Enable/disable tracing
**
** Description:
**  Context level trace.
**  Function enables and disables tracing.
**  
** Inputs:
**      ctx     Context structure
**      enable  flag toggles tracing on or off
**      level   trace level setting. Ignored if enable is FALSE
**      path    file specification to trace output. Ignored if enable is FALSE
**
** Outputs:
**      None
**      
** Returns:
**      OK                      If trace started or stopped successfully
**      E_CL2101_TR_BADCREATE   Error creating trace output file
**      E_CL2101_TR_BADOPEN     Error opening trace output file
**
** Example:
**  # include <iff.h>
**
**  static II_CHAR* filespec = { "ifftrace.log" };
**  static II_INT4  level = IFF_TRC_CALL;
**
**  ING_CTX*    ctx = NULL;
**
**  if ((status = IFFinitialize( &ctx )) == IFF_SUCCESS)
**  {
**      status = IFFtrace( ctx, IFF_TRC_ON, level, filespec );
**
**      status = IFFtrace( ctx, IFF_TRC_OFF, 0, NULL );
**  }
**
** History:
**      20-Aug-2004 (fanra01)
**          Commented.
**      24-Sep-2004 (fanra01)
**          Add iff_return_status call.
**      24-Jan-2005 (fanra01)
**          Update sample code embedded within the comments.
}*/
STATUS
IFFtrace( ING_CTX* ctx, II_INT4 enable, II_INT4 level, II_CHAR* path )
{
    STATUS      status = OK;
    EX_CONTEXT  context;
    ING_INS_CTX*    ins_ctx;
    CL_ERR_DESC system_status;
    PID pid;

    /*
    ** Include PID in trace messages.
    */
    PCpid( &pid );
    
    EXCEPT_BEGIN
    {
        if(EXdeclare( iff_exhandler, &context ) != OK)
        {
            EXdelete();
            switch(trcref)
            {
                default:
                    trcref -=1;
                case 0:
                    if (trcref == 0)
                    {
                        TRdisplay(
                            ERx("%@ %08x-%08x --- :IFFtrace: exception close trace\n"),
                            pid, PCtid() );
                        TRset_file( TR_F_CLOSE, NULL, 0, &system_status ); 
                    }
                    break;
            }
            return(iff_return_status( status ));
        }
        if (ctx == NULL)
        {
            status = IFF_ERR_PARAM;
            EXsignal( IFF_EX_BAD_PARAM, 0, 0 );
        }
        ins_ctx = (ING_INS_CTX*)ctx->instance_context;
        if( ins_ctx->state < IFF_INITIALIZED)
        {
            status = IFF_ERR_STATE;
            EXsignal( IFF_EX_BAD_STATE, 0, 0 );
        }
        switch(enable)
        {
            case IFF_TRC_ON:
                if (path == NULL)
                {
                    status = IFF_ERR_PARAM;
                    EXsignal( IFF_EX_BAD_PARAM, 0, 0 );
                }
                if ((status = iff_trace_open( path, level, FALSE )) == OK)
                {
                    ins_ctx->trclvl = level;
                    iff_trace( IFF_TRC_LOG, 0,
                        ERx("%p - Trace start local level %d\n"), ctx, level );
                }
                break;
            case IFF_TRC_OFF:
                iff_trace( IFF_TRC_LOG, 0, ERx("%p - Trace stop local\n"),
                    ctx );
                iff_trace_close( FALSE );
                ins_ctx->trclvl = 0;
                break;
            default:
                status = FAIL;
                EXsignal( IFF_EX_BAD_PARAM, 0, 0 );
                break;
        }
    }
    EXCEPT_END
    EXdelete();
    return(iff_return_status( status ));
}
