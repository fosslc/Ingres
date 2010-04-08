/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: asct.c
**
** Description:
**
**      Provides the functions for a logging and tracing thread.
**
**      A fixed number of buffers are created in memory.  Each trace statement
**      requests a buffer and fills it with trace information in the context
**      of the tracer.  The memory is a contiguous block divided into fixed
**      size elements.  The index of the element determines the message slot.
**
**      The trace thread periodically writes the message buffers to disk.
**      The thread is woken either on timeout or on threshold hit.
**
**      asct_open_file      Opens a trace file for appending.
**      asct_get_tracesym   Gets environment or cbf values.
**      asct_initialize     Setup for memory and semaphores.
**      asct_trace_flg      Determines if trace should be logged according to
**                          flag.
**      asct_get_buf        Returns the next available message slot.
**      asct_trace_event    Processing performed after a TRformat.
**      asct_write          Write memory buffer to file.
**      asct_flush          Scan memory for buffers to write.
**      asct_terminate      Free semaphores and memory.
**      asct_session_set    Save thread identifier.
**      asct_session_get    Get thread identifier.
**
** History:
**      01-Mar-1999 (fanra01)
**          Created.
**      12-Jun-2000 (fanra01)
**          Bug 101345
**          Cleaned compiler warning.
*/
# include <compat.h>
# include <cs.h>
# include <cv.h>
# include <er.h>
# include <lo.h>
# include <me.h>
# include <nm.h>
# include <si.h>
# include <st.h>
# include <tr.h>

# include <gl.h>
# include <pm.h>

# include <iicommon.h>
# include <ddfcom.h>

# include <asct.h>

#define WITH_RECORD(address, type, field) \
    ((type *)((char*)(address) - (char*)(&((type *)0)->field)))

FUNC_EXTERN STATUS asct_class_define (void);

GLOBALREF   char* asctlogfile;          /* logfile path and filename */
GLOBALREF   i4    asctlogsize;          /* number of buffer in to store */
GLOBALREF   i4    asctthreshold;        /* number of messages before write */
GLOBALREF   i4    asctlogtimeout;       /* number of seconds to write */
GLOBALREF   i4    ascttracemask;        /* types of messages requested */
GLOBALREF   i4    asctsavemask;

/*
** Name: asct_open_file
**
** Description:
**      Function opens the specified file for appending.
**
** Inputs:
**      filename    path and filename of the requested file.
**
** Outputs:
**      fdesc       the file descriptor for the file.
**
** Returns:
**      OK          success
**      FAIL        failure.  fdesc is also NULL
**
** History:
**      01-Mar-1999 (fanra01)
**          Created.
**	17-Jun-2004 (schka24)
**	    Filename comes from env var, length-limit it.
*/
STATUS
asct_open_file( char* filename, FILE** fdesc )
{
    STATUS      status = FAIL;
    LOCATION    loc;
    char        locbuf[MAX_LOC+1];

    if (fdesc)
    {
        *fdesc = NULL;

        STlcopy( filename, locbuf, sizeof(locbuf)-1 );
        if ((status = LOfroms( FILENAME&PATH, locbuf, &loc )) == OK)
        {
            status = SIopen( &loc, "a", fdesc );
        }
        return(status);
    }
    return(status);
}

/*
** Name: asct_get_tracesym
**
** Description:
**      Function reads configuration parameters from either NM or PM and
**      returns the variables.  Order of precedence is environment, symbol.tbl
**      variable, config.dat.
**
** Inputs:
**      nmvar       Environment or symbol table symbol.
**      pmstr       PM configuration string.
**
** Outputs:
**      value       Variable value.
**
** Returns:
**      OK          success
**      FAIL        failure.  value is also NULL
**
** History:
**      24-Feb-1999 (fanra01)
**          Created based on function from gcf.
*/
static STATUS
asct_get_tracesym( char* nmvar, char *pmstr, char **value )
{
    STATUS status = FAIL;

    if (value != NULL)
    {
        *value = NULL;

        NMgtAt( nmvar, value );
        if ( *value != NULL && **value != EOS)
        {
            status = OK;
        }
        else
        {
            if (pmstr && *pmstr)
            {
                status = PMget( pmstr, value );
                if (status == PM_NOT_FOUND)
                {
                    status = OK;
                }
            }
        }
    }
    return (status);
}

/*
** Name: asct_initialize
**
** Description:
**      Function reads configuration parameters and sets up the ice server
**      trace log.  If the path and filename are not setup the memory is
**      initialised anyway as the thread can still be started from ima.
**
** Inputs:
**      None.
**
** Outputs:
**      None.
**
** Returns:
**      E_DB_OK     success
**      E_DB_WARN   logfilename not specified thread won't start.
**      E_DB_ERROR  failure
**
** History:
**      24-Feb-1999 (fanra01)
**          Created.
*/
DB_STATUS
asct_initialize()
{
    STATUS      status  = OK;
    DB_STATUS   dbstat  = E_DB_WARN;
    char*       logfile = NULL;
    char*       logsize = NULL;
    char*       msgsize = NULL;
    char*       threshold = NULL;
    char*       logtimeout = NULL;
    char*       trcmsk = NULL;
    u_i4        memsize = 0;

    MEfill (sizeof(ASCTFR), 0, &asctentry);
    MEfill (sizeof(ASCTFL), 0, &asctfile);

    if (asctlogsize   == 0) asctlogsize     = MAX_DEFAULT_TRACE;
    if (asctthreshold == 0) asctthreshold   = ASCT_THRESHOLD;
    if (asctlogtimeout== 0) asctlogtimeout  = ASCT_FLUSH_TIMROUT;
    if (ascttracemask == 0)
    {
        ascttracemask   = ASCT_LOG;
        asctsavemask    = ASCT_LOG;
    }
    if (asctlogmsgsize== 0) asctlogmsgsize  = MAX_TRACE_MSG_SIZE;

    do
    {
        if ((status = asct_get_tracesym( "II_ICESVR_TRACE_MASK",
            "!.ice_trc_mask", &trcmsk )) != OK)
        {
            break;
        }
        else
        {
            if (trcmsk && *trcmsk)
            {
                status = CVan( trcmsk, (i4*)&ascttracemask );
                if (status != OK)
                    break;
                else
                    asctsavemask = ascttracemask;
            }
        }
        if ((status = asct_get_tracesym( "II_ICESVR_TRACE_SIZE",
            "!.ice_trc_size", &logsize) ) != OK)
        {
            break;
        }
        else
        {
            if (logsize && *logsize)
            {
                status = CVan( logsize, (i4*)&asctlogsize );
                if (status != OK)
                    break;
            }
        }
        if ((status = asct_get_tracesym( "II_ICESVR_TRACE_MSGSIZE",
            "!.ice_trc_msgsize", &msgsize )) != OK)
        {
            break;
        }
        else
        {
            if (msgsize && *msgsize)
            {
                status = CVan( msgsize, (i4*)&asctlogmsgsize );
                if (status != OK)
                    break;
            }
        }
        if ((status = asct_get_tracesym( "II_ICESVR_TRACE_TIMEOUT",
            "!.ice_trc_timout", &logtimeout) ) != OK)
        {
            break;
        }
        else
        {
            if (logtimeout && *logtimeout)
            {
                status = CVan( logtimeout, (i4*)&asctlogtimeout );
                if (status != OK)
                    break;
            }
        }
        if ((status = asct_get_tracesym( "II_ICESVR_TRACE_THRESHOLD",
            "!.ice_trc_threshold", &threshold) ) != OK)
        {
            break;
        }
        else
        {
            if (threshold && *threshold)
            {
                status = CVan( threshold, (i4*)&asctthreshold );
                if (status != OK)
                    break;
            }
        }
        if ((status=CSi_semaphore( &asctfile.flsem, CS_SEM_SINGLE ))
            != OK)
        {
            break;
        }
        asctfile.entry      = &asctentry;
        asctfile.state      = ASCT_FLUSH_IDLE;
        asctfile.start      = 0;
        asctfile.end        = asctlogsize;
        asctfile.maxflush   = 0;
        asctfile.startp     = 0;
        asctfile.endp       = asctthreshold;
        asctfile.trigger    = asctthreshold;
        asctfile.threshold  = asctthreshold;

        if ((status=CSi_semaphore( &asctentry.freesem, CS_SEM_SINGLE) )
            != OK)
        {
            break;
        }
        asctentry.logsize = asctlogsize;

        memsize = asctlogsize * asctlogmsgsize;
        if ((asctbuf = (PASCTBF)MEreqmem( 0, memsize, TRUE,
            &status) ) == NULL)
        {
            break;
        }
    }
    while (FALSE);
    if (status == OK)
    {
        status = asct_get_tracesym( "II_ICESVR_LOG", "!.ice_trc_log", &logfile );
        if ((status == OK) && (logfile != NULL) && (*logfile))
        {
            asctlogfile = STalloc (logfile);
            if ((status = asct_open_file( asctlogfile, &asctfile.fdesc )) == OK)
            {
                dbstat = E_DB_OK;
                asct_trace( ASCT_LOG )( ASCT_TRACE_PARAMS,
                    "Initialize %s buffers=%d size=%d timeout=%d threshold=%d",
                    asctlogfile, asctlogsize, asctlogmsgsize, asctlogtimeout,
                    asctthreshold );
            }
        }
        else
        {
            ascttracemask = 0;  /* disable writing to memory */
        }
    }
    status = asct_class_define();
    if (status != OK)
    {
        ascttracemask = 0;      /* disable writing to memory */
        dbstat = E_DB_ERROR;
    }
    return (dbstat);
}

/*
** Name: asct_trace_flg
**
** Description:
**
** Inputs:
**      traceflag   Requested level of tracing
**
** Outputs:
**      None
**
** Returns:
**      TRUE        trace message
**      FALSE       no action required.
**
** History:
**      24-Feb-1999 (fanra01)
**          Created.
*/
bool
asct_trace_flg( i4 traceflags )
{
    bool trace = FALSE;

    if (traceflags & ascttracemask)
    {
        trace = TRUE;
    }
    return (trace);
}

/*
** Name: asct_get_buf
**
** Description:
**      Obtains the next available message slot for writing.  The message is
**      header is updated with time stamp and thread information.
**
** Inputs:
**      entry   pointer to the free buffer structure for synchronisation and
**              index.
**
** Outputs:
**      .freebuf    free buffer index incremented.
**
**
** Returns:
**      msgbuf      valid buffer pointer if successful.
**      NULL        on failure.
**
** History:
**      24-Feb-1999 (fanra01)
**          Created.
**	06-Apr-2001 (bonro01)
**	    Inserted CSswitch() into while() spin-loop to prevent infinite
**	    looping when running with ingres threads.
*/
PTR
asct_get_buf( PASCTFR entry )
{
    STATUS  status = OK;
    i4      msgnum;
    i4      maxnum;
    PASCTBF trcbuf;
    PTR     msgbuf = NULL;

    if ((status = CSp_semaphore( 0, &asctentry.freesem )) == OK)
    {
        msgnum = asctentry.freebuf;
        maxnum = asctentry.logsize;
        if (msgnum < maxnum)
        {
            trcbuf = &asctbuf[msgnum];
            /*
            ** if the returned buffer is busy force a flush and wait for
            ** flush to complete.
            */
            if (trcbuf->hdr.state != ASCT_AVAILABLE)
            {
                asctfile.state |= ASCT_FORCED_FLUSH;
                CSresume( asct_session_get() );
                while (trcbuf->hdr.state != ASCT_AVAILABLE)
                    CSswitch();
                status = CSp_semaphore( TRUE, &asctfile.flsem );
                status = CSv_semaphore( &asctfile.flsem );
            }
            asctentry.freebuf = (asctentry.freebuf+1)%maxnum;
        }
        status = CSv_semaphore( &asctentry.freesem );
        if (status == OK)
        {
            trcbuf->hdr.id = msgnum;
            TMnow (&trcbuf->hdr.stamp);
            trcbuf->hdr.thread = CS_get_thread_id();
            trcbuf->hdr.tfill[0] = (i4)-1;
            trcbuf->hdr.tfill[1] = (i4)-2;
            trcbuf->hdr.state = ASCT_FILL_PENDING;
            msgbuf = &trcbuf->hdr.tbuf[0];
        }
    }
    return (msgbuf);
}

/*
** Name: asct_trace_event
**
** Description:
**      Function signals the completion of tracing to the buffer.
**      Also checks to see if the trigger has been hit and resumes the thread.
**
** Inputs:
**      asctfl  pointer to the flush file structure.
**      len     bytes written to buffer.
**      buf     pointer to buffer
**
** Outputs:
**      None.
**
** Returns:
**      OK      success
**      !0      failure.
**
** History:
**      24-Feb-1999 (fanra01)
**          Created.
*/
STATUS
asct_trace_event( PASCTFL asctfl, i4 len, char* buf )
{
    STATUS  status = OK;
    i4      trigger;
    i4      state;
    PASCTTH hdr = WITH_RECORD( buf, ASCTTH, tbuf );

    if (hdr->state & ASCT_FILL_PENDING)
    {
        hdr->tlen = len;
        hdr->state = ASCT_FILL_COMPLT;

        if ((status == OK) &&
            ((status = CSp_semaphore( 0, &asctfl->flsem )) == OK))
        {
            trigger = asctfl->trigger;
            state = asctfl->state;
            /*
            ** Only resume once for the event.
            */
            if ((hdr->id == trigger) && ((state & ASCT_EVENT_FLUSH) == 0))
            {
                asctfl->state |= ASCT_EVENT_FLUSH;
                status = CSv_semaphore( &asctfl->flsem );
                CSresume( asct_session_get() );
                /* signal a flush here */
            }
            else
            {
                status = CSv_semaphore( &asctfl->flsem );
            }
        }
    }
    return(status);
}

/*
** Name: asct_write
**
** Description:
**      Function writes buffer to disk.  Passed to TRformat.
**
** Inputs:
**      fdesc   file descriptor
**      len     length to write
**      buffer  buffer to write
**
** Outputs:
**      None
**
** Returns:
**      OK  success
**      !0 failure
**
** History:
**      24-Feb-1999 (fanra01)
**          Created.
*/
STATUS
asct_write( FILE* fdesc, i4 len, char* buffer )
{
    STATUS  status;
    i4 counter = 0;

    status = SIwrite( len, buffer, &counter, fdesc );
    return (status);
}

/*
** Name: asct_flush
**
** Description:
**      Function called to attempt to flush buffers to file.
**
** Inputs:
**      type
**
** Outputs:
**      .err_code
**
** Returns:
**      OK  success
**      !0 failure
**
** History:
**      24-Feb-1999 (fanra01)
**          Created.
**      12-Jun-2000 (fanra01)
**          Add cast to the output function.
*/
DB_STATUS
asct_flush( DB_ERROR* dberr, i4 type )
{
    STATUS  status = OK;
    PASCTFL asctfl = &asctfile;
    PASCTBF trcbuf;
    i4      end;
    i4      threshold;
    i4      startp;
    i4      endp;
    i4      i;
    i4      cnt;
    char    tmdecode[30];
    char    trcstr[80];
    char*   msgbuf;

    if ((status = CSp_semaphore( 0, &asctfl->flsem )) == OK)
    {
        /*
        ** read values under semaphore protection
        */
        startp  = asctfl->startp;
        endp    = asctfl->endp;
        end     = asctfl->end;
        threshold=asctfl->threshold;

        asctfl->state |= ASCT_BUSY_FLUSH | type;
        if ((status = CSv_semaphore( &asctfl->flsem )) == OK)
        {
            /*
            ** from the start point to the end point wrapping at the
            ** end of buffer array.
            */
            for (cnt=0, i=startp; (i != endp) && (status == OK);
                i=((i+1)%end), cnt++)
            {
                trcbuf=&asctbuf[i];
                if (trcbuf->hdr.state & ASCT_FILL_COMPLT)
                {
                    trcbuf->hdr.state = ASCT_FLUSH_PENDING;
                    msgbuf = &trcbuf->hdr.tbuf[0];
                    TMstr(&trcbuf->hdr.stamp, tmdecode);
                    TRformat( (i4(*)(PTR, i4, char *))asct_write, asctfl->fdesc, trcstr,
                        sizeof(trcstr), "%s %08x %14v: ", tmdecode,
                        trcbuf->hdr.thread, " ,POLL, EVENT, FORCED, IMA",
                        asctfl->state );
                    status = asct_write( asctfl->fdesc, trcbuf->hdr.tlen,
                        msgbuf );
                    status = asct_write( asctfl->fdesc, 1, "\n" );
                    trcbuf->hdr.state = ASCT_AVAILABLE;
                }
                else
                {
                    /*
                    ** hit a buffer not ready for flushing
                    */
                    break;
                }
            }
            if (cnt > 0)
            {
                SIflush (asctfl->fdesc);
            }
            if (status == OK)
            {
                /*
                ** Update the flush parameters.
                */
                if ((status = CSp_semaphore( 0, &asctfl->flsem )) == OK)
                {
                    asctfl->trigger = (startp+threshold)%end;
                    if (cnt > asctfl->maxflush)
                    {
                        asctfl->maxflush = cnt;
                    }
                    asctfl->startp = i;
                    asctfl->endp = (i+threshold)%end;
                    asctfl->state = ASCT_FLUSH_IDLE;
                    status = CSv_semaphore( &asctfl->flsem );
                }
            }
        }
    }
    if (status == OK)
    {
        dberr->err_code = E_DB_OK;
    }
    else
    {
        GSTATUS gstatus = GSTAT_OK;
        gstatus = DDFStatusAlloc( status );
        DDFStatusFree( TRC_EVERYTIME, &gstatus );

        dberr->err_code = status;

        return(E_DB_ERROR);
    }
    return(status);
}

/*
** Name: asct_terminate
**      Release resources created for the thread.
**
** Description:
**
** Inputs:
**      None.
**
** Outputs:
**      None.
**
** Returns:
**      OK  success
**      !0  failure
**
** History:
**      24-Feb-1999 (fanra01)
**          Created.
*/
STATUS
asct_terminate()
{
    STATUS  status;
    DB_ERROR dberr;

    if ((status = CSp_semaphore( 0, &asctentry.freesem )) == OK)
    {
        asct_flush( &dberr, ASCT_FORCED_FLUSH );
        status = CSv_semaphore( &asctentry.freesem );
    }
    CSr_semaphore( &asctentry.freesem );
    CSr_semaphore( &asctfile.flsem );

    SIflush( asctfile.fdesc );
    SIclose( asctfile.fdesc );
    MEfree( (PTR)asctbuf );
    return( status );
}

/*
** Name: asct_session_set
**
** Description:
**      Function updates the value of the session id.
**
** Inputs:
**      sess    cs session id
**
** Outputs:
**      None
**
** Returns:
**      None.
**
** History:
**      24-Feb-1999 (fanra01)
**          Created.
*/
VOID
asct_session_set( CS_SID sess )
{
    asctfile.self = sess;
    return;
}

/*
** Name: asct_session_get
**
** Description:
**      Returns the flush thread session id.
**
** Inputs:
**      None.
**
** Outputs:
**      None.
**
** Returns:
**      session id
**      No error return.
**
** History:
**      24-Feb-1999 (fanra01)
**          Created.
*/
CS_SID
asct_session_get()
{
    return ( asctfile.self );
}
