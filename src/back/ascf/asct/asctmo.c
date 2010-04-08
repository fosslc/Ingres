/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: asctmo.c
**
** Description:
**
**      Provides methods for ima.
**
**      asct_class_define       Register the asct MO class.
**      asct_start_set          Start a trace thread.
**      asct_suspend_set        Suspend a trace thread.
**      asct_flush_set          Issue a flush request.
**      asct_logfile_set        Change the name of a logfile.
**      asct_threshold_set      Change the flush trigger point.
**      asct_logmem_resize      Resize trace memory array.
**      asct_logsize_set        Change number of log entries.
**      asct_logmsgsize_set     Change size of log entry.
**      asctfl_state_get        Return string representation of flush state.
**      asct_tracemask_get      Return string representation of trace mask.
**
** History:
**      03-Mar-1999 (fanra01)
**          Created.
**      18-Jun-1999 (fanra01)
**          Add VARS to mask description.
*/
# include <compat.h>
# include <gl.h>
# include <cs.h>
# include <cv.h>
# include <er.h>
# include <ex.h>
# include <lo.h>
# include <me.h>
# include <pc.h>
# include <si.h>
# include <sl.h>
# include <st.h>
# include <tr.h>
# include <mo.h>

# include <iicommon.h>
# include <dbdbms.h>
# include <ulf.h>
# include <ulm.h>
# include <adf.h>
# include <gca.h>

# include <dmf.h>
# include <dmccb.h>
# include <dmtcb.h>
# include <qsf.h>
# include <ddb.h>
# include <opfcb.h>
# include <qefmain.h>
# include <qefrcb.h>
# include <qefcb.h>
# include <scf.h>
# include <duf.h>
# include <dudbms.h>
# include <dmrcb.h>
# include <copy.h>
# include <qefqeu.h>
# include <qefcopy.h>

# include <sc.h>
# include <sca.h>
# include <scc.h>
# include <scs.h>
# include <scd.h>
# include <sc0e.h>
# include <sc0m.h>
# include <sce.h>
# include <scfcontrol.h>

# include <asct.h>

FUNC_EXTERN STATUS asct_open_file (char* filename, FILE** fdesc);
FUNC_EXTERN DB_STATUS asct_flush (DB_ERROR* dberr, i4 type);

GLOBALREF   char*   asctlogfile;
GLOBALREF   ASCTFR  asctentry;
GLOBALREF   ASCTFL  asctfile;
GLOBALREF   PASCTBF asctbuf;
GLOBALREF   i4      asctlogmsgsize;

GLOBALREF   i4    asctlogsize;
GLOBALREF   i4    asctthreshold;
GLOBALREF   i4    asctlogtimeout;
GLOBALREF   i4    ascttracemask;
GLOBALREF   i4    asctsavemask;

GLOBALREF SC_MAIN_CB     *Sc_main_cb;    /* Core structure for SCF */

MO_SET_METHOD asct_start_set;
MO_SET_METHOD asct_suspend_set;
MO_SET_METHOD asct_flush_set;
MO_SET_METHOD asct_logfile_set;
MO_SET_METHOD asct_logsize_set;
MO_SET_METHOD asct_logmsgsize_set;
MO_SET_METHOD asct_threshold_set;

GLOBALREF MO_CLASS_DEF asct_class[];


/*
** Name:    asct_class_define
**
** Description:
**      Define the MO class for asct.
**
**
** Inputs:
**      None.
**
** Outputs:
**      None.
**
** Returns:
**      OK                  successfully completed
**      MO_NO_MEMORY        MO_MEM_LIMIT exceeded
**      MO_NULL_METHOD      null get/set methods supplied when permission state
**                          they are allowed
**      !OK                 system specific error
**
** History:
**      03-Mar-1999 (fanra01)
**          Created.
**
*/
STATUS
asct_class_define( void )
{
    return(MOclassdef (MAXI2, asct_class ));
}

/*
** Name: asct_start_set
**
** Description:
**      Function takes the log file path string and opens the file and
**      then starts the thread.
**
** Inputs:
**      offset      not used
**      len         size of buffer
**      buf         pointer to buffer with filename
**      osize       size of associated object
**      object      pointer to flush file structure
**
** Outputs:
**      None.
**
** Returns:
**      OK  success
**      !0  failure
**
** History:
**      03-Mar-1999 (fanra01)
**          Created.
*/
STATUS
asct_start_set( i4 offset, i4 len, char* buf, i4 osize, PTR object )
{
    STATUS  status = FAIL;
    PASCTFL asctfl = (PASCTFL)object;
    GCA_LS_PARMS        local_crb;
    u_i4 memsize = 0;
    i4	priority;
    CL_SYS_ERR sys_err;

    if ((asctlogfile == NULL) && (asctfl->self == 0))
    {
        if ((asctlogfile = STalloc(buf)) != NULL)
        {
            if ((status = asct_open_file( asctlogfile, &asctfl->fdesc )) == OK)
            {
                asct_trace( ASCT_LOG )( ASCT_TRACE_PARAMS,
                    "Initialize %s buffers=%d size=%d timeout=%d threshold=%d",
                    asctlogfile, asctlogsize, asctlogmsgsize, asctlogtimeout,
                    asctthreshold );

                local_crb.gca_status = 0;
                local_crb.gca_assoc_id = 0;
                local_crb.gca_size_advise = 0;
                local_crb.gca_user_name = ICE_LOGTRCTHREAD_NAME;
                local_crb.gca_account_name = 0;
                local_crb.gca_access_point_identifier = "NONE";
                local_crb.gca_application_id = 0;

                priority = Sc_main_cb->sc_max_usr_priority - 2;
                if (priority < Sc_main_cb->sc_min_priority)
                    priority = Sc_main_cb->sc_min_priority;

                status = CSadd_thread( priority,
                    (PTR) &local_crb, SCS_TRACE_LOG,
                    (CS_SID*)NULL, &sys_err );

                if (status != OK)
                {
                    sc0e_0_put( status, 0 );
                    sc0e_0_put( E_SC0386_LOGTRC_THREAD_ADD, 0 );
                    status = E_DB_ERROR;
                }
                else
                {
                    ascttracemask = asctsavemask;
                }
            }
        }
    }
    return (status);
}

/*
** Name: asct_suspend_set
**
** Description:
**      Suspend the flush thread.
**
** Inputs:
**      offset      not used
**      len         size of buffer
**      buf         pointer to buffer with filename
**      osize       size of associated object
**      object      pointer to flush file structure
**
** Outputs:
**      None.
**
** Returns:
**      OK  success
**      !0  failure
**
** History:
**      03-Mar-1999 (fanra01)
**          Created.
*/
STATUS
asct_suspend_set( i4 offset, i4 len, char* buf, i4 osize, PTR object )
{
    STATUS status = OK;
    return (status);
}

/*
** Name: asct_flush_set
**
** Description:
**      Force a flush of memory to file.
**
** Inputs:
**      offset      not used
**      len         not used
**      buf         not used
**      osize       not used
**      object      not used
**
** Outputs:
**      None.
**
** Returns:
**      OK      success
**      !0      failure
**
** History:
**      03-Mar-1999 (fanra01)
**          Created.
*/
STATUS
asct_flush_set( i4 offset, i4 len, char* buf, i4 osize, PTR object )
{
    DB_ERROR dberr;
    return (asct_flush( &dberr, ASCT_IMA_FLUSH ));
}

/*
** Name: asct_logfile_set
**
** Description:
**      Change the log path and filename
**
** Inputs:
**      offset      not used
**      len         size of buffer
**      buf         pointer to buffer with filename
**      osize       not used
**      object      not used
**
** Outputs:
**      None.
**
** Returns:
**      OK      success
**      !0      failure
**
** History:
**      03-Mar-1999 (fanra01)
**          Created.
*/
STATUS
asct_logfile_set( i4 offset, i4 len, char* buf, i4 osize, PTR object )
{
    STATUS status = FAIL;

    if (asctlogfile != NULL)
    {
        MEfree( asctlogfile );
        asctlogfile = NULL;
    }
    if ((asctlogfile = STalloc( buf )) != NULL)
    {
        if (asctfile.fdesc != NULL)
        {
            status = SIclose( asctfile.fdesc );
        }
        status = asct_open_file( asctlogfile, &asctfile.fdesc );
    }
    return (status);
}

/*
** Name: asct_threshold_set
**
** Description:
**      Change the threshold.
**
** Inputs:
**      offset      not used
**      len         size of buffer
**      buf         pointer to buffer with threshold value
**      osize       size of associated object
**      object      pointer to threashold variable.
**
** Outputs:
**      None.
**
** Returns:
**      OK      success
**      !0      failure
**
** History:
**      03-Mar-1999 (fanra01)
**          Created.
*/
STATUS
asct_threshold_set( i4 offset, i4 len, char* buf, i4 osize, PTR object )
{
    STATUS status = MOintset( offset, len, buf, osize, object );

    if (status == OK)
    {
        asctfile.threshold = asctthreshold;
    }
    return (status);
}

/*
** Name: asct_logmem_resize
**
** Description:
**      Takes the current log size and log message size resizes the memory.
**      Flushes remaining messages to file and performs the resize.
**
** Inputs:
**      None.
**
** Outputs:
**      None.
**
** Returns:
**      OK      success
**      !0      failure
**
** History:
**      03-Mar-1999 (fanra01)
**          Created.
*/
static STATUS
asct_logmem_resize()
{
    STATUS status = OK;
    DB_ERROR dberr;
    u_i4 memsize = 0;

    /*
    ** Hold semaphore to stop anyone tracing
    */
    if ((status = CSp_semaphore(0, &asctentry.freesem )) == OK)
    {
        /*
        ** Issue a flush
        */
        if ((status = asct_flush( &dberr, ASCT_IMA_FLUSH )) == OK)
        {
            MEfree( (PTR)asctbuf );
            asctfile.end = asctlogsize;
            asctfile.maxflush = 0;
            asctfile.startp = 0;
            asctthreshold = asctlogsize/2;
            asctfile.endp = asctthreshold;
            asctfile.trigger = asctthreshold;
            asctfile.threshold = asctthreshold;

            asctentry.freebuf = 0;
            asctentry.logsize = asctlogsize;
            memsize = asctlogsize * asctlogmsgsize;
            asctbuf = (PASCTBF)MEreqmem( 0, memsize, TRUE,&status );
        }
        status = CSv_semaphore( &asctentry.freesem );
    }
    return (status);
}

/*
** Name: asct_logsize_set
**
** Description:
**      Change number of entries in the message memory array.
**
** Inputs:
**      offset      not used
**      len         size of buffer
**      buf         pointer to buffer with log size value.
**      osize       size of associated object
**      object      pointer to logsize variable.
**
** Outputs:
**      None.
**
** Returns:
**      OK  success
**      !0  failure
**
** History:
**      03-Mar-1999 (fanra01)
**          Created.
*/
STATUS
asct_logsize_set( i4 offset, i4 len, char* buf, i4 osize, PTR object )
{
    STATUS status = MOintset( offset, len, buf, osize, object );

    if (status == OK)
    {
        status = asct_logmem_resize();
    }
    return (status);
}

/*
** Name: asct_logmsgsize_set
**
** Description:
**      Change size of entries in the message memory array.
**
** Inputs:
**      offset      not used
**      len         size of buffer
**      buf         pointer to buffer with log message size value.
**      osize       size of associated object
**      object      pointer to log message size variable.
**
** Outputs:
**      None.
**
** Returns:
**      OK      success
**      !0      failure
**
** History:
**      03-Mar-1999 (fanra01)
**          Created.
*/
STATUS
asct_logmsgsize_set( i4 offset, i4 len, char* buf, i4 osize, PTR object )
{
    STATUS status = MOintset( offset, len, buf, osize, object );

    if (status == OK)
    {
        status = asct_logmem_resize();
    }
    return (status);
}

/*
** Name: asctfl_state_get
**
** Description:
**      Returns string representation of state flags.
**
** Inputs:
**      offset      not used
**      size        size of associated object
**      object      pointer to flush file structure
**      lbuf        size of buf
**      buf         buffer for string
**
** Outputs:
**      None.
**
** Returns:
**      OK      success
**      !0      failure
**
** History:
**      03-Mar-1999 (fanra01)
**          Created.
*/
STATUS
asctfl_state_get( i4 offset, i4 size, PTR object, i4 lbuf, char *buf )
{
    STATUS status = OK;
    char format_buf [50];
    PASCTFL asctfl = (PASCTFL)object;

    MEfill( sizeof(format_buf), (u_char)0, format_buf );
    if (asctfl->state != 0)
    {
        TRformat( NULL, 0, format_buf, sizeof(format_buf), "%v",
                "BUSY, POLLED,EVENT,FORCED,IMA",  asctfl->state );
    }
    else
    {
        STcopy( "IDLE", format_buf );
    }
    return (MOstrout( MO_VALUE_TRUNCATED, format_buf, lbuf, buf ));
}

/*
** Name: asct_tracemask_get
**
** Description:
**      Returns string representation of mask flags.
**
** Inputs:
**      offset      not used
**      size        size of associated object
**      object      pointer to flush file structure
**      lbuf        size of buf
**      buf         buffer for string
**
** Outputs:
**      None.
**
** Returns:
**      OK      success
**      !0      failure
**
** History:
**      03-Mar-1999 (fanra01)
**          Created.
**      18-Jun-1999 (fanra01)
**          Add VARS to mask description.
*/
STATUS
asct_tracemask_get( i4 offset, i4 size, PTR object, i4 lbuf, char *buf )
{
    STATUS status = OK;
    char format_buf [50];
    i4   mask = *(i4 *)object;

    MEfill( sizeof(format_buf), (u_char)0, format_buf );

    if (mask != 0)
    {
        TRformat( NULL, 0, format_buf, sizeof(format_buf), "%v",
            "LOG, QUERY, INFO, INIT, PARSER, EXEC, VARS",  mask );
    }
    else
    {
        STcopy( "DISABLED", format_buf );
    }
    return (MOstrout( MO_VALUE_TRUNCATED, format_buf, lbuf, buf ));
}
