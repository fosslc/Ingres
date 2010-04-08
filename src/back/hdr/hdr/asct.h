/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: asct.h
**
** Description:
**      Definitions, types and prototypes for the tracing thread.
**
** History:
**      15-Mar-99 (fanra01)
**          Created.
**      10-May-1999 (fanra01)
**          Add prototype for asct_flush.
**          Add thread id define.
**      18-Jun-1999 (fanra01)
**          Add ASCT_VARS for tracing variable values.
**      12-Jun-2000 (fanra01)
**          Bug 101345
**          Cleaned compiler warning by adding cast to output function
**          parameter asct_trace_event.
*/

# ifndef ASCT_INCLUDED
# define ASCT_INCLUDED

# define    MAX_TRACE_MSG_SIZE  384
# define    MAX_DEFAULT_TRACE   4000
# define    ASCT_THRESHOLD      1000
# define    ASCT_FLUSH_TIMROUT  10

# define    ASCT_LOG            0x0001
# define    ASCT_QUERY          0x0002
# define    ASCT_INFO           0x0004
# define    ASCT_INIT           0x0008
# define    ASCT_PARSER         0x0010
# define    ASCT_EXEC           0x0020
# define    ASCT_VARS           0x0040

# define    ASCT_MSG_SIZE        (asctlogmsgsize - sizeof (ASCTTH))

# define    ICE_LOGTRCTHREAD_NAME " <Tracing Thread>               "

typedef CS_SEMAPHORE        ASCTSEM;

typedef struct __ASCTFile           ASCTFL;
typedef ASCTFL*                     PASCTFL;

typedef struct __ASCTFreeBuffer     ASCTFR;
typedef ASCTFR*                     PASCTFR;

typedef struct __ASCTTraceHeader    ASCTTH;
typedef ASCTTH*                     PASCTTH;

typedef union __ASCTTraceBuffer     ASCTBF;
typedef ASCTBF* PASCTBF;

struct __ASCTFile
{
    CS_SID          self;               /* session identifier */
    FILE*           fdesc;              /* log file descriptor */
    PASCTFR         entry;              /* pointer to free buffer structure */
    i4              state;              /* invoke of flush */
# define ASCT_FLUSH_IDLE    0x0000
# define ASCT_BUSY_FLUSH    0x0001
# define ASCT_POLLED_FLUSH  0x0002
# define ASCT_EVENT_FLUSH   0x0004
# define ASCT_FORCED_FLUSH  0x0008
# define ASCT_IMA_FLUSH     0x0010
    i4              start;              /* start of buffer array */
    i4              end;                /* end of buffer array */
    ASCTSEM         flsem;              /* protect the flush points */
    i4              maxflush;           /* highest number of flushes */
    i4              trigger;            /* flush trigger */
    i4              startp;             /* flush start point */
    i4              endp;               /* flush end point */
    i4              threshold;          /* threshold flush number */
};

struct __ASCTFreeBuffer
{
    ASCTSEM         freesem;            /* protect counter for free buffer */
    i4              freebuf;            /* index for next free buffer */
    i4              logsize;            /* number of elements in array */
};

struct __ASCTTraceHeader
{
    i4              id;
    SYSTIME         stamp;              /* time stamp for trace buffer */
    CS_THREAD_ID    thread;             /* thread identifier of caller */
    i4              tlen;               /* length of message in buffer */
    i4              state;              /* state of current buffer     */
# define ASCT_AVAILABLE     0x0000
# define ASCT_FILL_PENDING  0x0001
# define ASCT_FILL_COMPLT   0x0002
# define ASCT_FLUSH_PENDING 0x0004
    i4              tfill[2];           /* padding */
    char            tbuf[1];            /* start of trace buffer */
};

union __ASCTTraceBuffer
{
    ASCTTH  hdr;
    char    buf[MAX_TRACE_MSG_SIZE];
};

/*
** x is the mask
** asct_trace(EXEC)(ASCT_TRACE_PARAMS, "",,,,,);
*/
# define ASCT_TRACE_PARAMS (i4(*)(PTR, i4, char *))asct_trace_event, &asctfile, \
                           asct_get_buf(&asctentry), \
                           (asctlogmsgsize - sizeof (ASCTTH))
# define asct_trace(x)  if (asct_trace_flg(x)) TRformat

/*
** Global data
*/
GLOBALREF ASCTFR    asctentry;
GLOBALREF ASCTFL    asctfile;
GLOBALREF PASCTBF   asctbuf;
GLOBALREF i4        asctlogmsgsize;
GLOBALREF i4        asctlogtimeout;

/*
** Function prototypes
*/
FUNC_EXTERN DB_STATUS asct_initialize( void );

FUNC_EXTERN bool   asct_trace_flg( i4 traceflag );

FUNC_EXTERN PTR    asct_get_buf( PASCTFR entry );

FUNC_EXTERN STATUS asct_trace_event( PASCTFL arg, i4 len, char* buf );

FUNC_EXTERN DB_STATUS asct_flush( DB_ERROR* dberr, i4 type );

FUNC_EXTERN STATUS asct_terminate( void );

FUNC_EXTERN VOID asct_session_set( CS_SID sess );

FUNC_EXTERN CS_SID asct_session_get( void );

# endif /* ASCT_INCLUDED */
