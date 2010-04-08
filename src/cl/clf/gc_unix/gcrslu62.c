/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <systypes.h>
#include    <clconfig.h>
#include    <cs.h>
#include    <er.h>
#include    <gcccl.h>
#include    <cm.h>
#include    <me.h>
#include    <nm.h>
#include    <clpoll.h>
#include    <cv.h>
#include    <qu.h>
#include    <clsigs.h>
#include    <ex.h>
#include    <lo.h>
#include    <pm.h>                /* needed for 1.1 onward */
#include    <st.h>
#include    <errno.h>
 
# ifdef xCL_006_FCNTL_H_EXISTS
# include   <fcntl.h>
# endif
 
#if defined(any_aix)
#ifdef xCL_022_AIX_LU6_2
 
/* SNA include files */

#include       <luxsna.h>
#include        <iconv.h>

/* External variables */

/* External functions */

extern  int     a64l();
 
/* Forward functions */
 
static  VOID    GClu62sm();
static  VOID    lu62reg_pcb();
static  VOID    lu62unreg_pcb();
static  VOID    process_pcb_event();
static  VOID    process_exception();
static  STATUS  lu62req_to_snd();
static  STATUS  lu62prep_to_rcv();
static  int     lu62write();
static  STATUS  lu62flush();
static  STATUS  lu62register();
static  STATUS  lu62name();
static  VOID    lu62trace();
static  STATUS  AtoE();
 
/* LU6.2 specific Defines */
 
#define   GCLU62_LEVEL        1      /* level number */
#define   MAXARGS             4      /* args that a TP program can have */
#define   CONV_SND            1      /* conversation in send state */
#define   CONV_RCV            2      /* conversation in recv state */
#define   CONV_DEAL           3      /* conversation in deallocate state  */
#define   SND_TOKEN_TIME      50     /* ms we give up the token after SEND */
#define   RCV_TOKEN_TIME      2000   /* minimum value of idle timer */
#define   MAX_TOKEN_TIME    120000   /* maximum value of idle timer */

#define   ASCII               "IBM-037"
#define   EBCDIC              "ISO8859-1"

/* flags for lu62write function */ 

#define   nCD                 0      /* just write */
#define   CD                  1      /* write and change direction */

#define   EXT_IO   struct ext_io_str
#define   MYSTATE  (pcb->transaction_state==CONV_SND ?"CONV_SND":"CONV_RCV")
 
/* Per-connection (in the AIX LU6.2 sense) control block */
 
typedef struct _GC_CCB
{
    char    cp_name[MAX_TPN_LEN];         /* connnection profile name */
}   GC_CCB;
 
/* Per-driver control block */
 
typedef struct _GC_DCB
{
    char    tp_profile[MAX_TPN_LEN + 1];  /* tp profile name */
    char    tpn_name[MAX_TPN_LEN + 1];    /* tpn name */
}   GC_DCB;
 
/* Per-conversation control block */
 
typedef struct _GC_PCB
{
    GCC_P_PLIST  *ctl_parm_list;          /* connect/listen/disconn parm list */
    GCC_P_PLIST  *snd_parm_list;          /* send parm list */
    GCC_P_PLIST  *rcv_parm_list;          /* receive parm list */
    EXT_IO   ext;                         /* API argument-result structure */
    char     connection[MAX_TPN_LEN];     /* connnection name */
    char     rtpn_name[MAX_TPN_LEN];      /* rtpn name */
    u_char   protdrvr_level;              /* protocol driver level number */
    char     loc_buffer[80];              /* utility buffer */
    long     rid;                         /* SNA resource id */
    bool     send_op;                     /* last completed op a send? */
    bool     registered;                  /* registered for event? */
    bool     rcv_incompl;                 /* incomplete packet received? */
    int      fd;                          /* fd associated with conversation */
    int      bytes_read;                  /* # of bytes read so far */
    int      transaction_state;           /* State of connection */
    int      ret_val;                     /* return value from read() */
    int      idle_timer;                  /* idle timeout value */
}   GC_PCB;
 
/* Actions in GClu62sm. */
 
#define    GC_OPEN_REQ   0    /* OPEN request */
#define    GC_LISN_REQ   1    /* LISTEN request */
#define    GC_LISN_IND   2    /* LISTEN indication received */
#define    GC_LISN_DAT   3    /* LISTEN data received */
#define    GC_LISN_TKN   4    /* LISTEN wait for token */
#define    GC_LISN_CMP   5    /* LISTEN completion */
#define    GC_CONN_REQ   6    /* CONNECT request */
#define    GC_CONN_DAT   7    /* CONNECT data received */
#define    GC_CONN_TKN   8    /* CONNECT wait for token */
#define    GC_RECV_REQ   9    /* RECEIVE request */
#define    GC_RECV_CMP  10    /* RECEIVE completion */
#define    GC_SEND_REQ  11    /* SEND request */
#define    GC_SEND_TKN  12    /* SEND wait for token */
#define    GC_SEND_CMP  13    /* SEND completion */
#define    GC_DISC_REQ  14    /* DISCONNECT request */

/* Action names for tracing. */

static char *gc_names[] = {
    "OPEN_REQ", "LISN_REQ", "LISN_IND", "LISN_DAT", "LISN_TKN", "LISN_CMP",
    "CONN_REQ", "CONN_DAT", "CONN_TKN", "RECV_REQ", "RECV_CMP", "SEND_REQ",
    "SEND_TKN", "SEND_CMP", "DISC_REQ",
};
 
/* local variables */

static     i4            trace_level = 0;
static     i4            lrc;
static     GC_DCB        *dcb = 0;
static     GC_CCB        *ccb = 0;
static     int           listen_fd;
 
#define    GCTRACE(n)    if (trace_level >= n) (void)TRdisplay
 
/*{
** Name: GCpdrvr    - Protocol driver routine
**
** Description:
**    GCpdrvr is the model for the protocol driver routines which interface
**    to the local system facilities through which access to specific
**    protocols is offered.  In a given environment there may be multiple
**    protocol drivers, depending on the number of separate protocols which
**    are to be supported and the specific implementation of protocol drivers
**    in the environment.  This is an implementation issue, and is dealt with
**    by appropriate construction of the Protocol Control Table (PCT) by the
**    initialization routine GCpinit().
**
**    Protocol driver routines are not called directly by the CL client, but
**    rather are invoked by pointer reference through the Protocol Control
**    Table.  This is therefore a model specification for all protocol
**    drivers, whose names are not actually known outside the CL.  They are
**    known externally only by an identifier in the PCT.
**    
**    All protocol drivers have an indentical interface, through which a set
**    of service functions is offered to clients.  Two parameters are passed
**    by the caller: a function identifier and a parameter list pointer.  The
**    function ID specifies which of the functions in the repertoire is being
**    requested.  The parm list supplies the parameters required for
**    execution of the function.  It contains both input and output
**    parameters.  It has a constant section, and a section containing
**    parameters specific to the requested function.  The interface for all
**    routines is specified here instead of with each routine, as is
**    customary, since it is identical for all.
**
**    It is the responsibility of a single protocol driver to map the service
**    interface for a particular protocol, as provided by a specific
**    environment, to the model interface specified here.  In particular, this
**    includes the management of asynchronous events in the specified way.
**    The protocol driver interface is asynchronous in the sense that when a
**    function is invoked by the client, control is returned prior to
**    completion of the function.  Notification of completion is provided
**    to the client by means of a callback routine provided by the client as a
**    service request parameter.  This routine is to be invoked when the
**    requested service is complete.
**
**    It is to be noted that execution control is guaranteed to pass to the
**    CL routine GCc_exec, and remain there for the duration of server
**    execution.  It is possible for protocol driver service requests to be
**    invoked prior to the time GCc_exec receives control.  It is not
**    required that these service requests complete until control passes to
**    GCc_exec.  This enables the detection and allowance of external
**    asynchronous events to reside in a place to which control is guaranteed
**    to pass subsequent to a service request, if this is appropriate in a
**    particular environment.  This is not required, and if the environment
**    permits, the entire mechanism of dealing with external asynchronous
**    events may be implemented in the protocol driver.  It is also to be
**    noted that when a CL client's callback routine is invoked, it is
**    guaranteed that control will be returned from that invocation when the
**    client's processing is complete, and that the client will not wait or
**    block on some external event.
**
**
** Inputs:
**      Function_code                   Specifies the requested function
**      GCC_OPEN        ||
**        GCC_CONNECT        ||
**        GCC_DISCONNECT  ||
**        GCC_SEND        ||
**        GCC_RECEIVE        ||
**        GCC_LISTEN
**      parameter_list                  A pointer to a set of parameters
**                    qualifying the requested function.
**                    See gcccl.h for description.
**
** Outputs:
**      Contained in the parameter list, described above.
**
**    Returns:
**        STATUS:
**        OK
**        FAIL
**
** History:
**    19-Aug-91 (cmorris)
**        Shell copied from gccasync.
**    20-Jan-92 (gautam)
**        Rewritten for the RS/6000 AIX 3.1 SNA LU6.2 support
**    20-May-92 (gautam)
**        Change addressing scheme for outgoing connections and listen address
**    17-Sep-92 (gautam)
**        Correct SEGV problem that occurs on an invalid TP profile
**    01-Feb-93 (johnr)
**        Added a test for xCL_022_AIX_LU6.2 for ris_us5.
**        It must be ris_us5 and have LU6.2 both to compile.
**    15-Dec-92 (gautam)
**        Better indentation, and added more documentation. Optimization of 
**        the token passing timeout according to the last data operation.
**    17-Dec-92 (gautam)
**        Added PMget instead of NMgtAt() for getting the listen port.
**    11-Jan-93 (edg)
**        Use gcu_get_tracesym() to get trace level.  Changed II_GCCL_LU62_TRACE
**        to simply II_GCCL_TRACE as in other files.
**    18-Jan-93 (edg)
**        Replaced gcu_get_tracesym() with inline code.
**    15-Feb-93 (brucek)
**        Substantially re-written.
**    26-apr-1993 (fredv)
**      Moved <clsigs.h> before <ex.h> to avoid redefine of EXCONTINUE 
**              in the system header sys/context.h of AIX. include st.h
**      15-jul-93 (ed)
**          adding <gl.h> after <compat.h>
**    26-Oct-93 (brucek)
**         Removed NMgtAt support for listen address completely.
**         Added support for "NO_PORT" flag to avoid listening.
**      23-nov-93 (vijay)
**         remove erroneous redefn of val.
**      07-oct-1993 (tad)
**          Bug #56449
**          Changed %x to %p for pointer values.
**      13-oct-1994 (canor01)
**          cast parameters in writex() function to match AIX 4.1 prototypes
**      09-feb-1995 (chech02)
**          Added rs4_us5 for AIX 4.1.
**      07-jul-1995 (kosma01)
**          Undefined system's CONV_SEND, as it was conflicting with OpIng's.
**      11-aug-1995 (kosma01)
**          Inadvertenly added undefine of CONV_SEND as a comment. This update
**          changes the comment to a statement undefining CONV_SEND.
**      28-sep-1995 (thoro01)
**          IFDEFed CONV_SEND for ris_us5 because that define causes problems
**          with AIX compiles.
**    06-Feb-96 (tlong)
**        Largely rewritten to accommodate and conform to SNA Server 2.1
**        and Server 2.2 features.  Reported bugs # 63249 and 74247 fixed.
**        Numerous unreported bugs fixed.  Anachronisms removed.  Unused and
**        unneeded variables removed.  Undid the preceeding CONV_SEND mess. 
**        Reformatted.
**      27-apr-1999 (hanch04)
**          replace STindex with STchr
**    22-Mar-99 (hweho01)
**        Added support for AIX 64-bit (ris_u64) platform.
**	11-Nov-1999 (jenjo02)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	    The latter causes a wasted ___errno function call.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-jul-2001 (stephenb)
**	    Add support for i64_aix
**      11-dec-2002 (loera01)  SIR 109237
**          Set option flag in GCC_P_LIST to 0 (remote).
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
*/
STATUS
GClu62(func_code, parms)
i4  func_code;
GCC_P_PLIST *parms;
{
    /*
    ** Client is always remote.
    */
    parms->options = 0;

    /*
    ** Compute initial state based on function code. 
    */
    switch (func_code)
    {
        case GCC_OPEN:          parms->state = GC_OPEN_REQ; break;
        case GCC_LISTEN:        parms->state = GC_LISN_REQ; break;
        case GCC_CONNECT:       parms->state = GC_CONN_REQ; break;
        case GCC_SEND:          parms->state = GC_SEND_REQ; break;
        case GCC_RECEIVE:       parms->state = GC_RECV_REQ; break;
        case GCC_DISCONNECT:    parms->state = GC_DISC_REQ; break;
    }

    GCTRACE(1) ("GClu62:\t0x%p function_code %d.....\n", parms, func_code);

    parms->generic_status = OK;
    CL_CLEAR_ERR( &parms->system_status );
 
    /* Start sm. */
 
    GClu62sm(parms);
    return OK;
}

static VOID
GClu62sm(parms)
GCC_P_PLIST *parms;
{
    GC_PCB              *pcb;
 
    struct allo_str     allo_str;
    struct deal_str     dealloc;
 
    int                 n_wrote; 
    int                 n_read;
    int                 writ;
    int                 lng;
    char                connection_name[MAX_TPN_LEN];

    struct get_parms    get_parms;
    char                *argptr[MAXARGS];
    int                 argcount;
    int                 i = 0;
 
    pcb = (GC_PCB *) parms->pcb;
 
top:
    GCTRACE(2) ("GClu62sm:\t0x%x state %s pcb 0x%x\n", parms, 
        gc_names[parms->state], pcb);
 
    switch (parms->state)
    {
    case GC_OPEN_REQ:
        /*
        ** Initial entry to OPEN request.
        */

        /* Set protocol driver trace level */
        lu62trace();
 
        /* Set to ignore SIGUSR1 */
        i_EXsetothersig(SIGUSR1, SIG_IGN);
 
        /* Allocate and initialize per-driver control block */
        parms->pce->pce_dcb = MEreqmem(0, sizeof(*dcb), TRUE, (STATUS *) 0);
        if (!parms->pce->pce_dcb)
        {
            parms->generic_status = GC_OPEN_FAIL;
            goto complete;
        }

        dcb = (GC_DCB *)parms->pce->pce_dcb;
 
        /* Allocate and initialize per-connection control block */
        ccb = (GC_CCB *) MEreqmem(0, sizeof(*ccb), TRUE, (STATUS *) 0);
        if (!ccb)
        {
            parms->generic_status = GC_OPEN_FAIL;
            goto complete;
        }
 
        /* Obtain listen address */
        if (lu62name(parms->pce->pce_pid, connection_name, 
                         dcb->tp_profile, dcb->tpn_name) != OK)
        {
            GCTRACE(1) ("GClu62sm: lu62name failure! (protocol: %s)\n",
                                          parms->pce->pce_pid);
            parms->generic_status = GC_OPEN_FAIL;
            goto complete;
        }
 
        GCTRACE(1)("cp_name = %s, tp_profile = %s, tpn_name = %s\n",
            connection_name, dcb->tp_profile, dcb->tpn_name);

        STpolycat(5, connection_name, ".", dcb->tp_profile, ".", dcb->tpn_name,
                parms->function_parms.open.lsn_port);

        if (connection_name[0] != '/')
            STcopy("/dev/sna/", ccb->cp_name);
        else
            ccb->cp_name[0] = '\0';

        STcat(ccb->cp_name, connection_name);
 
        /*
        ** Open the SNA connection 
        */

        if ((listen_fd = open(ccb->cp_name, O_RDWR | O_NDELAY)) < 0)
        {
            GCTRACE(1) ("GClu62sm: Cannot open %s errno %d\n",
                connection_name, errno);
            listen_fd = 0;
            parms->generic_status = GC_OPEN_FAIL;
            goto complete;
        }
 
        GCTRACE(2) ("GClu62_LSN:\t0x%p lsn_fd = %d\n", parms, listen_fd);

        /*
        ** See whether this is a "no listen" protocol.
        ** If not, set up to receive incoming FMH5 requests.
        */

        if(!parms->pce->pce_flags & PCT_NO_PORT)
        {
            if (lu62register(dcb, ccb) != OK)
            {
                parms->generic_status = GC_OPEN_FAIL;
                goto complete;
            }
        }

        goto complete;

    case GC_LISN_REQ:
        /* 
        **  Initial entry to LISTEN request.
        **  Register the listen FD for exception indication.
        */

        parms->state = GC_LISN_IND;
        iiCLfdreg(listen_fd, FD_EXCEPT, process_exception, parms, -1);
        return;
 
    case GC_LISN_IND:
        /*
        ** An FMH5 attach request has come in for a particular connection.
        ** Issue ioctl to GET PARAMETERS and get char ptrs to the args.
        */

        if (ioctl(listen_fd, GET_PARAMETERS, &get_parms) == -1)
        {
            GCTRACE(1) ("ioctl(GET_PARAMETERS) failed errno %d\n", errno);
            parms->generic_status = GC_LISTEN_FAIL;
            goto complete;
        }
        GCTRACE(4) ("ioctl(GET_PARAMETERS) OK\n");
 
        if (get_parms.gparms_size == 0)
        {
            GCTRACE(1) ("ioctl(GET_PARAMETERS) - no parameters %d\n");
            parms->generic_status = GC_LISTEN_FAIL;
            goto complete;
        }
 
        /*  Get char ptrs to the args.  */

        argptr[0] = &get_parms.gparms_data[0];
        argcount = 1;

        while (i <= get_parms.gparms_size)
        {
            while (get_parms.gparms_data[i] != 0x20 && 
                        get_parms.gparms_size >= i)
            {
                i++;
            }

            if (i <= get_parms.gparms_size)
            {
                get_parms.gparms_data[i] = 0;
                i++;
                argptr[argcount] = &get_parms.gparms_data[i];
                argcount++;
            }
        }

        GCTRACE(1) ("argc=%d,argv[0]=%s,argv[1]=%s argv[2]=%s\n",
            argcount, argptr[0], argptr[1], argptr[2]);
 
        /* Allocate a new pcb for the new user */

        parms->pcb = MEreqmem(0, sizeof(*pcb), TRUE, (STATUS *) 0);
        if (!parms->pcb)
        {
            parms->generic_status = GC_LISTEN_FAIL;
            goto complete;
        }
        pcb = (GC_PCB *) parms->pcb;

        pcb->ctl_parm_list = parms;
        pcb->idle_timer = RCV_TOKEN_TIME;
        STcopy(ccb->cp_name, pcb->connection);

        pcb->rid = a64l(argptr[2]);

        /* Open an fd for the new conversation */
        if ((pcb->fd = open(pcb->connection, O_RDWR)) < 0)
        {
            GCTRACE(1) ("GClu62_LISN: Cannot open %s errno %d\n",
                pcb->connection, errno);
            parms->generic_status = GC_LISTEN_FAIL;
            pcb->fd = 0;
            goto complete;
        }
        GCTRACE(4) ("OPEN successful for fd = %d \n", pcb->fd);

        /*
        **  Allocate conversation
        */

        MEfill(sizeof(allo_str), '\0', &allo_str);

        allo_str.type = MAPPED_CONV;
        allo_str.return_control = WHEN_SESSION_ALLOC;
        allo_str.sync_level = SYNCL_NONE;
        allo_str.pip = PIP_NO;
        allo_str.svc_tpn_flag = SERVICE_TP_NO;
        allo_str.security = SECUR_NONE;

        allo_str.rid = pcb->rid;
 
        if (ioctl(pcb->fd, ALLOCATE, &allo_str) < 0)
        {
            GCTRACE(1) ("GClu62_LISN: ALLOCATE failure %s errno %d\n",
               pcb->connection, errno);
            parms->generic_status = GC_LISTEN_FAIL;
            goto complete;
        }
        GCTRACE(4) ("GClu62_LISN: ALLOCATE OK\n");
        /*
        **  Register for READ
        */

        pcb->transaction_state = CONV_RCV;
        parms->state = GC_LISN_DAT;
        lu62reg_pcb(pcb, -1);
        return;
 
    case GC_LISN_DAT:
        /*
        **  We've received initial data.  Maybe.
        */
 
        if ((n_read = pcb->ret_val) == -1)
        {
            parms->generic_status = GC_LISTEN_FAIL;
            goto complete;
        }
 
        if (pcb->ext.rid != pcb->rid)
        {
            GCTRACE(1) ("GClu62_LISN:\t0x%p rid's don't match\n", parms);
            parms->generic_status = GC_LISTEN_FAIL;
            goto complete;
        }
 
        GCTRACE(2) ("GClu62_LISN: Read %d from remote %s\n",
                 n_read, pcb->connection);
 
        /*
        ** If we have'nt received anything rerun the state machine with
        ** state = GC_LISN_DAT (unchanged).
        */
        if (pcb->ret_val == 0)
        {
            lu62reg_pcb(pcb, -1);
            return;
        }
 
        /*
        ** Check received data
        */
 
        pcb->protdrvr_level = (u_char)GCLU62_LEVEL;
        if (pcb->protdrvr_level > pcb->loc_buffer[0])
            pcb->protdrvr_level = pcb->loc_buffer[0];
 
        /* 
        ** If send token not received yet, wait for it;
        ** otherwise, send response message back.
        */

        if (pcb->transaction_state != CONV_SND)
            parms->state = GC_LISN_TKN; 
        else
            parms->state = GC_LISN_CMP;
 
        goto top;
 
    case GC_LISN_TKN:
        /* 
        **  We're waiting for the send token.
        */
 
        if (pcb->ret_val == -1)
        {
            parms->generic_status = GC_LISTEN_FAIL;
            goto complete;
        }
 
        if (pcb->ext.rid != pcb->rid)
        {
            GCTRACE(1) ("GClu62_LSN:\t0x%p rid's don't match\n", parms);
            parms->generic_status = GC_LISTEN_FAIL;
            goto complete;
        }
 
        /* 
        ** If still waiting for send token, re-register for READ
        */
        if (pcb->transaction_state != CONV_SND)
        {
            lu62reg_pcb(pcb, -1);
            return;
        }
 
        /*
        ** Otherwise, send response message back
        */

        parms->state = GC_LISN_CMP;
        goto top;
 
    case GC_LISN_CMP:
        /*
        **  We now have the send token, and will send back response message.
        */

        MEfill (sizeof(pcb->loc_buffer), '\0', pcb->loc_buffer);
        pcb->loc_buffer[0] = pcb->protdrvr_level;
        lng = STlength(pcb->loc_buffer); 

        if ( (n_wrote = lu62write(parms, pcb->loc_buffer, lng, CD)) == 0)
        {
            parms->generic_status = GC_LISTEN_FAIL;
            goto complete;
        }
 
        GCTRACE(2) ("GClu62_LISN: Wrote %d to remote\n", n_wrote);
        GCTRACE(2) ("\t contrl:%d, rq_to_snd_rcvd:%d, state:%s\n",
            pcb->ext.output.what_control_rcvd, pcb->ext.output.rq_to_snd_rcvd,
            MYSTATE);
 
        pcb->transaction_state = CONV_RCV;
 
        pcb->ctl_parm_list = 0;
        goto complete;
    /* end case GC_LISN_CMP */

    case GC_CONN_REQ:
        /*  
        **  Initial entry to CONNECT request.
        **
        **  Allocate a new pcb, open the connection, allocate the
        **  conversation, and send the initial data.
        **
        */
 
        /* Set protocol driver trace level */
        lu62trace();
 
        /* Allocate per-connection control block */
        parms->pcb = MEreqmem(0, sizeof(*pcb), TRUE, (STATUS *) 0);
        if (!parms->pcb)
        {
            parms->generic_status = GC_CONNECT_FAIL;
            goto complete;
        }
        pcb = (GC_PCB *) parms->pcb;

        pcb->ctl_parm_list = parms;
        pcb->idle_timer = RCV_TOKEN_TIME;
 
        if (parms->function_parms.connect.node_id[0] != '/')
            STcopy("/dev/sna/", pcb->connection);
        else
            pcb->connection[0] = '\0';

        STcat(pcb->connection, parms->function_parms.connect.node_id);

        /*
        **  Open fd for new conversation.
        */

        if ((pcb->fd = open(pcb->connection, O_RDWR | O_NDELAY)) < 0)
        {
            GCTRACE(1) ("GClu62_CONN: Cannot open %s errno %d\n",
                pcb->connection, errno);
            pcb->fd = 0;
            parms->generic_status = GC_CONNECT_FAIL;
            goto complete;
        }
        else
        {
            GCTRACE(1) ("GClu62_CONN: Open OK %s errno %d\n",
                pcb->connection, errno);
        }
 
        /*
        ** Allocate conversation 
        */
        MEfill(sizeof(allo_str), '\0', &allo_str);
        allo_str.type = MAPPED_CONV;
        allo_str.return_control = WHEN_SESSION_ALLOC;
        allo_str.sync_level = SYNCL_NONE;
        allo_str.pip = PIP_NO;
        allo_str.security = SECUR_NONE;
        allo_str.svc_tpn_flag = SERVICE_TP_NO;

        STcopy (parms->function_parms.connect.port_id, pcb->rtpn_name);
        lrc = AtoE(parms->function_parms.connect.port_id, allo_str.tpn);
        if (lrc != 0)
        {
            GCTRACE(1) ("GClu62_CONN: translate failure, lrc=%d\n", lrc);
            parms->generic_status = GC_CONNECT_FAIL;
            goto complete;

        }
        else
            GCTRACE(4) ("GClu62_CONN: translate OK, lrc=%d\n", lrc);

        if (ioctl(pcb->fd, ALLOCATE, &allo_str) < 0)
        {
            GCTRACE(1) ("GClu62_CONN: ALLOCATE failure with %s errno %d\n",
                pcb->connection, errno);
            parms->generic_status = GC_CONNECT_FAIL;
            goto complete;
        }
        else
            GCTRACE(1) ("GClu62_CONN: ALLOCATE OK with %s, rid %d\n",
                parms->function_parms.connect.port_id, allo_str.rid);
 
        pcb->rid = allo_str.rid;
        pcb->transaction_state = CONV_SND;

        /*
        ** Do initial send to peer 
        */

        MEfill(sizeof(pcb->loc_buffer), '\0', &pcb->loc_buffer);
        pcb->loc_buffer[0] = GCLU62_LEVEL;
        lng = STlength(pcb->loc_buffer);

        if ((n_wrote = lu62write(parms, pcb->loc_buffer, lng, CD)) == 0)
        {
                parms->generic_status = GC_CONNECT_FAIL;
                goto complete;
        }

        GCTRACE(2) ("GClu62_CONN: wrote initial message(%d) to %s\n",
                    n_wrote, pcb->rtpn_name);

        pcb->transaction_state = CONV_RCV;
 
        /*
        ** Register for READ
        */
 
        parms->state = GC_CONN_DAT;
        lu62reg_pcb(pcb, -1);
        return;
 
    case GC_CONN_DAT:
        /*
        **  We've received response message.  Maybe.
        */

        if ((n_read = pcb->ret_val) < 0)
        {
            if (errno == EINTR)
            {
                /*  treat interrupted as zero bytes read and just rerun
                **  the state machine.
                */
                GCTRACE(4)("Read interrupted\n");

                lu62reg_pcb(pcb, -1);
                return;
            }
            else
            {
                parms->generic_status = GC_CONNECT_FAIL;
                goto complete;
            }
        }

        if (pcb->ext.rid != pcb->rid)
        {
            GCTRACE(1) ("GClu62sm:\t0x%p rid's don't match\n", parms);
            parms->generic_status = GC_CONNECT_FAIL;
            goto complete;
        }
 
        GCTRACE(2) ("GClu62_CONN: Read %d from remote %s\n",
            n_read, pcb->connection);
 
        /*
        ** If we have'nt received anything rerun the state machine with
        ** state = GC_CONN_DAT (unchanged).
        */

        if (n_read == 0)
        {
            lu62reg_pcb(pcb, -1);
            return;
        }
 
        /*
        ** Check received data
        */
 
        pcb->protdrvr_level = (u_char)GCLU62_LEVEL;
        if (pcb->protdrvr_level > pcb->loc_buffer[0])
            pcb->protdrvr_level = pcb->loc_buffer[0];
 
        /* 
        ** If send token not received yet, wait for it;
        ** otherwise, we're all done.
        */

        if (pcb->transaction_state != CONV_SND)
        {
            lu62reg_pcb(pcb, -1);
            parms->state = GC_CONN_TKN;
            return;
        }

        pcb->ctl_parm_list = 0;
        goto complete;
 
    case GC_CONN_TKN:
        /*  
        **  We're waiting for the send token before we complete the connect.
        */

        if ((n_read = pcb->ret_val) < 0)
        {
            parms->generic_status = GC_CONNECT_FAIL;
            goto complete;
        }
 
        if (pcb->ext.rid != pcb->rid)
        {
            GCTRACE(1) ("GClu62_CONN:\t0x%p rid's don't match\n", parms);
            parms->generic_status = GC_CONNECT_FAIL;
            goto complete;
        }
 
        /* 
        ** If still waiting for send token, re-register for receive
        */

        if (pcb->transaction_state != CONV_SND)
        {
            lu62reg_pcb(pcb, -1);
            return ;
        }
 
        pcb->ctl_parm_list = 0;
        goto complete;

    case GC_RECV_REQ:
        /*  
        **  Initial entry to RECEIVE request.
        */
 
        if (!pcb->fd)
        {
            parms->generic_status == GC_CONNECT_FAIL;
            goto complete;
        }

        pcb->bytes_read  = 0;
 
        if (!pcb->registered)
            lu62reg_pcb(pcb, -1);

        parms->state = GC_RECV_CMP;
        pcb->rcv_parm_list = parms;
        return;
 
    case GC_RECV_CMP:
        /*
        **  Receive has completed, one way or another.
        */
 
        if (!pcb->fd)
        {
            parms->generic_status == GC_CONNECT_FAIL;
            goto complete;
        }
        if (pcb->ret_val < 0)
        {
            GCTRACE(1) ("GClu62_RCV:\t0x%x readx failed; status %d, state %s\n",
                parms, pcb->ret_val, MYSTATE);

            parms->generic_status = GC_RECEIVE_FAIL;
        }
        else
        {
            GCTRACE(4) ("GClu62_RCV:\t0x%x read %d bytes from remote\n",
                parms, pcb->bytes_read);

            parms->buffer_lng = pcb->bytes_read;
            pcb->send_op = FALSE;
        }

        goto complete;

    case GC_SEND_REQ:
        /* 
        **  Initial entry to SEND request.
        */
        GCTRACE(4) ("GC_SND_REQ:\t0x%p send size %d, state = %s\n",
            parms, parms->buffer_lng, MYSTATE);

        if (!pcb->fd)
        {
            parms->generic_status == GC_CONNECT_FAIL;
            goto complete;
        }
 
        switch (pcb->transaction_state)
        {
        case CONV_SND:
            /*
            ** We're in send state so go ahead and send
            */
            parms->state = GC_SEND_CMP;
            goto top;
 
        case CONV_RCV:
            /*
            **  Can't do send right now; send a request-to-send and
            **  hold on to the send parms till the token comes in.
            */
            if (lu62req_to_snd(pcb) != OK)
            {
                GCTRACE(2) ("GClu62_SND:\t0x%x req-to-send failed\n", parms);
                parms->generic_status = GC_SEND_FAIL;
                goto complete;
            }

            parms->state = GC_SEND_TKN;
            goto top;
        }
 
    case GC_SEND_TKN:
        /*
        ** Want to do a send, but waiting for the send token.
        */

        GCTRACE(3) ("GClu62_SND:\t0x%x queuing up send\n", parms);

        if (!pcb->registered)
            lu62reg_pcb(pcb, -1);
 
        pcb->snd_parm_list = parms;
        parms->state = GC_SEND_CMP;
        return;
 
    case GC_SEND_CMP:
        /*
        **  Ready to send data
        */

        if (!pcb->fd)
        {
            parms->generic_status == GC_CONNECT_FAIL;
            goto complete;
        }
        if ((n_wrote =
             lu62write(parms, parms->buffer_ptr, parms->buffer_lng, nCD)) < 0)
        {
            parms->generic_status = GC_SEND_FAIL;
            pcb->fd = 0;
            goto complete;
        }
        else if (n_wrote == 0)    /* can't send now, wait a bit */
        {
            if (!pcb->registered)
                lu62reg_pcb(pcb, SND_TOKEN_TIME);
            pcb->snd_parm_list = parms;
            return;
        }

        GCTRACE(4) ("GClu62_SND:\t0x%x wrote %d\n", parms, n_wrote);
        GCTRACE(2) ("\t contrl:%d, rq_to_snd_rcvd:%d, state:%s\n",
            pcb->ext.output.what_control_rcvd, 
            pcb->ext.output.rq_to_snd_rcvd, MYSTATE);
 
        if (pcb->ext.output.rq_to_snd_rcvd == REQUEST_TO_SEND_RECEIVED)
        {
        /*
        ** Send completed.  Give up token if request to send received,
        ** otherwise just flush output.
        */
            if (lu62prep_to_rcv(pcb) != OK)
            {
                parms->generic_status = GC_SEND_FAIL;
                goto complete;
            }
        }
 
        /*
        ** Set timeout for giving up send token.
        */
        pcb->send_op = TRUE;
        pcb->idle_timer = RCV_TOKEN_TIME;

        if (pcb->transaction_state == CONV_SND)
        {
            if (pcb->registered)
                lu62unreg_pcb(pcb);

            lu62reg_pcb(pcb, SND_TOKEN_TIME);
        }
 
        goto complete;

    case GC_DISC_REQ:
      {
        STATUS s;

        /*
        **  Initial entry to DISCONNECT request.
        */

        if (!pcb)
            goto complete;
 
        MEfill(sizeof(dealloc), '\0', &dealloc);
        dealloc.rid = pcb->rid;
        dealloc.deal_flag = DISCARD;
 
        /* Do not issue a deallocate if we're in CONV_DEAL state */
        if (pcb->transaction_state != CONV_DEAL)
        {
            if (pcb->transaction_state == CONV_SND)
                dealloc.type = DEAL_FLUSH;
            else
                dealloc.type = DEAL_ABEND ;
 
            if (ioctl(pcb->fd, DEALLOCATE, &dealloc) < 0)
            {
                /*  We don't really care about this error message anyway  */
                GCTRACE(1)("GClu62_DISCON: DEALLOC failed: errno %d\n",errno);
            }
            GCTRACE(4)("GClu62_DISCON: ioctl DEALLOC * OK *\n");
        }
 
        /*
        ** Unregister this pcb, free pcb, zero fields.
        */
        if (pcb->fd > 0)
        {
            lu62unreg_pcb(pcb);
            close(pcb->fd); 
        }

        /*
        ** These can't be valid after deallocating and will cause
        ** a cascade of errors if followed, so get rid of them.
        */
        pcb->fd = 0;
        pcb->ctl_parm_list = 0;
        pcb->snd_parm_list = 0;
        pcb->rcv_parm_list = 0;

        s = MEfree(pcb);

        parms->pcb = 0;
        pcb = 0;
        GCTRACE(4)("GClu62_DISCON: pcb free status = 0x%x\n", s);
 
        goto complete;
      }

    default:
        GCTRACE(1)("GClu62sm: Default case - shouldn't be here !!\n");
        goto complete;
    }

complete:
    /*
    ** Drive completion routine. 
    */
 
    GCTRACE(1)("GClu62sm:\t0x%p\tCOMPLETE %s status %x\n", parms,
                  gc_names[parms->state], parms->generic_status);
 
    (*parms->compl_exit) (parms->compl_id);
}

/*
** Name: process_pcb_event 
** 
** Driven whenever a non-listen fd is selected for READ.
*/

static VOID
process_pcb_event(pcb, status)
GC_PCB  *pcb;
STATUS  status;    
{
    GCC_P_PLIST *parms;
    char        *rcv_ptr;
    int         rcv_len;
    int         lerrno;

    GCTRACE(2)("process_pcb_event: pcb: 0x%p status %d \n", pcb, status);
 
    pcb->registered = FALSE;
    pcb->ret_val = 0;
 
    /*
    **  If we've timed out, give up the send token and re-register if
    **  necessary (i.e. if there's a receive outstanding).
    */

    if (status != OK)
    {
        status = lu62prep_to_rcv(pcb);

        if (status && (parms = pcb->rcv_parm_list))
        {
            pcb->rcv_parm_list = 0;
            GClu62sm(parms);
            return;
        }
 
        if (pcb->rcv_parm_list)
            lu62reg_pcb(pcb, -1);

        return;
    }
 
    /* 
    ** Otherwise, we have something to read.
    **
    ** If we are processing a connect or listen request, use the 
    ** local buffer in the pcb.
    **
    ** Otherwise, if we've got an outstanding receive request, use the
    ** buffer from that parm list to receive any data/indicators.
    **
    ** If none of the above, put up a dummy read to try and get the
    ** send token, if available.
    */

    if (pcb->ctl_parm_list)
    {
        rcv_ptr = pcb->loc_buffer;
        rcv_len = sizeof(pcb->loc_buffer);
    }
    else if (pcb->rcv_parm_list)
    {
        rcv_ptr = pcb->rcv_parm_list->buffer_ptr + pcb->bytes_read;
        rcv_len = pcb->rcv_parm_list->buffer_lng - pcb->bytes_read;
    }
    else
    {
        rcv_ptr = pcb->loc_buffer;
        rcv_len = 0;
    }

    MEfill(sizeof(pcb->ext), '\0', &pcb->ext);

    pcb->ext.input.deallocate = PROCESS_WITHOUT_DEALLOCATE;
    pcb->ext.input.fill = RECEIVE_FILL_BUFFER;
    pcb->ext.rid = pcb->rid;
    pcb->transaction_state = CONV_RCV;
 
    while (1)
    {
        GCTRACE(4)("READing for %d bytes\n", rcv_len);
        pcb->ret_val = readx(pcb->fd, rcv_ptr, rcv_len, (long)&pcb->ext);

        if (pcb->ret_val < 0)
            lerrno = errno;
        else
            lerrno = 0;

        GCTRACE(2)("process_pcb_event: fd %d read %d\n", pcb->fd, pcb->ret_val);
        GCTRACE(2)("\t rq_to_snd:%d, control:%d, data_rcvd:%d\n",
            pcb->ext.output.rq_to_snd_rcvd, pcb->ext.output.what_control_rcvd,
            pcb->ext.output.what_data_rcvd);

        if (lerrno == EINTR)             /* try again on interrupt */
        {
            GCTRACE(4)("process_pcb_event: read interrupted\n");
            continue;
        }
        else                            /* exit on error or some data */
        {
            if (pcb->ret_val < 0)
                GCTRACE(1)("process_pcb_event: READ ERROR %d\n", lerrno);

            break;
        }
    }

    /*
    **  Check return status and received indicators.
    */

    if (pcb->ext.output.what_control_rcvd  == NORMAL_DEALLOCATE_RECEIVED)
    {
        pcb->transaction_state = CONV_DEAL;
    }

    pcb->rcv_incompl = FALSE;
    if (pcb->ret_val >= 0)
    {
        pcb->bytes_read += pcb->ret_val;
 
        /* if data incomplete indication received, make note of it */
        if (pcb->ext.output.what_data_rcvd  == INCOMPLETE_DATA_RECEIVED)
            pcb->rcv_incompl = TRUE;
 
        /* if data incomplete indication not received, make note of it */
        if (pcb->ext.output.what_data_rcvd  == COMPLETE_DATA_RECEIVED)
            pcb->rcv_incompl = FALSE;
 
        /* if send token received, change our state */
        if (pcb->ext.output.what_control_rcvd == SEND_RECEIVED)
            pcb->transaction_state = CONV_SND;
    }

    /*
    **  If connect or listen request, drive fsm and get out.
    */

    if (pcb->ctl_parm_list)
    {
        GClu62sm(pcb->ctl_parm_list);
        return;
    }

    /*
    **  If there's an outstanding receive request, either drive it
    **  (if we've received an entire message), or re-register for read.
    */

    if (parms = pcb->rcv_parm_list)
    {
        if (pcb->ext.output.what_data_rcvd  == COMPLETE_DATA_RECEIVED || lerrno)
        {
            pcb->rcv_parm_list = 0;
            GClu62sm(parms);
        }
        else
        {
            if (!pcb->registered)
                lu62reg_pcb(pcb, -1);
        }
    }

    /*
    **  If send token received, either drive a queued send, or set a
    **  timer to give up the token after an appropriate interval.
    **
    **  The timer algorithm is as follows:
    **      a) If the last successful operation on this conversation 
    **         was a send, the token is immediately sent back.
    **      2) If the last successful operation on this conversation
    **         was a receive, we set the timeout value to RCV_TOKEN_TIME
    **         to begin with, and for every successive idle "cycle"
    **         (when we get the token back with no data having been
    **         received), we multiply the timeout value by 2 until
    **         MAX_TOKEN_TIME has been reached.
    */

    if (pcb->ret_val > 0)
        pcb->idle_timer = RCV_TOKEN_TIME;

    if (pcb->ext.output.what_control_rcvd == SEND_RECEIVED)
    {
        if (parms = pcb->snd_parm_list)
        {
            pcb->snd_parm_list = 0;
            GClu62sm(parms);
        }
        else
        {
            if (pcb->send_op)
            {
                lu62prep_to_rcv(pcb);
            }
            else
            {
                lu62reg_pcb(pcb, pcb->idle_timer);

                if ((pcb->idle_timer *= 2) > MAX_TOKEN_TIME)
                    pcb->idle_timer = MAX_TOKEN_TIME;
            }
        }
    }

    /*
    **  If there's still an outstanding send request, make sure we're
    **  registered for READ (to pick up the send token).
    **
    **  However, note that if we're not already registered (which 
    **  must mean no receive request is outstanding), but there IS
    **  an incomplete-data indication, we shouldn't register right
    **  now, since our dummy (zero-length) readx will never receive
    **  the data in the channel and we'll just keep selecting the fd
    **  over and over.  In this case, we'll just have to wait for a
    **  receive request to get posted before we re-register for READ.
    **  In other words, the send token is backed up behind some data,
    **  and we won't get to it until the data has been received.
    */

    if (pcb->snd_parm_list && !pcb->registered && !pcb->rcv_incompl)
    {
        GCTRACE(3)("process_pcb_event: registering to send\n");
        lu62reg_pcb(pcb, -1);
    }
        
    return;
}

/*
** Name: process_exception 
** 
** Driven whenever a listen fd is selected for EXCEPTION
*/
static VOID
process_exception(parms, status)
GCC_P_PLIST     *parms;
STATUS          status;    
{

    GCTRACE(2)("0x%p process_exception: entry status %d \n", parms, status);

    /*
    **  Drive fsm with listen parm list.
    */

    GClu62sm(parms);
    return;
}

/*
** Name: lu62reg_pcb - Register conversation for event notification
**
** Description:
**      Registers an fd for selection on event.
**
** Inputs:
**      pcb 
**      timeout for event
**
** Returns:
**      none
*/
static VOID
lu62reg_pcb(pcb, timeout)
GC_PCB  *pcb;
i4 timeout;
{
    GCTRACE(2)("lu62reg_pcb: pcb 0x%p, fd %d, timeout %d\n",
        pcb, pcb->fd, timeout);
    if (pcb && pcb->fd) 
    {
        iiCLfdreg(pcb->fd, FD_READ, process_pcb_event, pcb, timeout);
        pcb->registered = TRUE;
    }
    else
        GCTRACE(1)("TRYing to register a non-conversation\n");

    GCTRACE(2)("lu62reg_pcb: OUT\n");
}

/*
** Name: lu62unreg_pcb - Remove conversation from event list 
**
** Description:
**      Removes an fd from event list 
**
** Inputs:
**      pcb 
**
** Returns:
**      none
*/
static VOID
lu62unreg_pcb(pcb)
GC_PCB *pcb;
{
 
    GCTRACE(2)("lu62unreg_pcb: pcb 0x%p, fd %d\n", pcb, pcb->fd);
    (VOID)iiCLfdreg(pcb->fd, FD_READ, (VOID (*)) 0, (PTR) 0, -1);
    pcb->registered = FALSE;
}

/*
** Name: lu62write
**
** Description:
**      Writes data to partner 
**
** Inputs:
**      pcb, buffer ptr, buffer length 
**
** Returns:
**      bytes written or 0 on error
*/
static int
lu62write(parms, buf, len, cdflag)
GCC_P_PLIST *parms;
char *buf;
int len;
int cdflag;
{
    GC_PCB *pcb = (GC_PCB *)parms->pcb;
    int writ = 0;
    int rc = 0;

    MEfill (sizeof(pcb->ext), '\0', &pcb->ext);
    pcb->ext.input.confirm = SEND_WITHOUT_CONFIRM;
    pcb->ext.input.deallocate = PROCESS_WITHOUT_DEALLOCATE;
    pcb->ext.input.flush_flag = SEND_AND_FLUSH;
    pcb->ext.input.non_block_write = NON_BLOCK_WRITE;
    pcb->ext.input.fmh_data = FMH_APPLICATION_DATA;
    pcb->ext.input.psh_data = SEND_WITHOUT_PSH_DATA;
    pcb->ext.input.prepare_to_receive = SEND_WITHOUT_PREP_TO_RECEIVE;
    if (cdflag)
        pcb->ext.input.prepare_to_receive = SEND_AND_PREP_TO_RECEIVE;

    pcb->ext.rid = pcb->rid;
    rc = len;
    while (1)
    {
        writ = writex(pcb->fd, buf, len, (long)&pcb->ext);
        if (writ == 0)
        {
            GCTRACE(2)("Write blocking, registering\n");
            rc = 0;
            break;
        }
        else if (writ == len)              
        {
            break;
        }
        else if (writ < 0)          
        {
            if (errno == EINTR || errno == EAGAIN)
            {
                GCTRACE(4)("Write interrupted\n");
                continue;
            }
            else
            {   /* on error, break */ 
                GCTRACE(1) ("GClu62sm: Write ERROR  %d\n", errno);
                rc = -1;
                break;
            }
        }
        else if (writ < len)
        {
            len -= writ;
            buf += writ;
            continue;
        }
    }
    GCTRACE(4) ("GClu62sm: wrote %d octets\n", rc);
    return rc;

} /* end lu62write */

/*            NO LONGER USED but RETAINED ANYWAY         */

/*
** Name: lu62flush - flush send buffers
**
** Returns:
**    status = OK or FAIL
*/
static STATUS
lu62flush(pcb)
GC_PCB   *pcb;
{
    struct flush_str flush_str;
 
    MEfill(sizeof(flush_str), '\0', &flush_str);
    flush_str.rid = pcb->rid;
 
    if (ioctl(pcb->fd, FLUSH, &flush_str) < 0)
    {
        GCTRACE(1) ("lu62flush: ioctl(FLUSH) fails errno %d\n", errno);
        return FAIL;
    }
    GCTRACE(4) ("lu62flush: ioctl(FLUSH) done \n");
    return OK;
}

/*
** Name: lu62prep_to_rcv - Toss the send token to our partner
**
** Returns:
**    status = OK or FAIL
*/
static STATUS
lu62prep_to_rcv(pcb)
GC_PCB *pcb;
{
    struct prep_str prep_str;
    i4  lrc;

    if (pcb->transaction_state == CONV_RCV)
        return OK;   
 
    MEfill(sizeof(prep_str), '\0', &prep_str);
    prep_str.rid = pcb->rid;
    prep_str.type = PREP_TO_RECEIVE_FLUSH;
    while (1)
    {
        lrc  = ioctl(pcb->fd, PREPARE_TO_RECEIVE, &prep_str) ;
        if (lrc  < 0)
        {
             if (errno == EINTR)
             {
                 GCTRACE(4)("Prep_to_recv interrupted\n");
                 continue;
             }
             else
             {
                 GCTRACE(1)("lu62prep_to_rcv: pcb: %x, ERROR %d\n", pcb, errno);
                 pcb->fd = 0;
                 return FAIL ;
             }
        }
        else
        {
             GCTRACE(4)("lu62prep_to_rcv: OK!\n");
             break;
        }
    }
    GCTRACE(3)("lu62prep_to_rcv done\n");
    pcb->transaction_state = CONV_RCV;
    return OK;

} /* end prep_to_rcv */

/*
** Name: lu62req_to_snd - Request the send token from our partner
**
** Returns:
**    status = OK or FAIL
*/
static STATUS
lu62req_to_snd(pcb)
GC_PCB *pcb;
{
    if (ioctl(pcb->fd, REQUEST_TO_SEND, pcb->rid) < 0)
    {
        GCTRACE(1)("lu62req_to_snd: fail_errno %d\n", errno);
        return FAIL ;
    }
    GCTRACE(4)("lu62req_to_snd done\n"); 
    return OK ;
}

/*
** Name: lu62register - registers the listen fd for incoming connections
**
** Returns:
**    status = OK or FAIL
*/
static STATUS
lu62register(dcb, ccb)
GC_DCB  *dcb;
GC_CCB  *ccb;
{
    struct alloc_listen alloc_listen_str;
    char buffer[128];
    STATUS status = OK;
 
    MEfill(sizeof(struct alloc_listen), '\0', &alloc_listen_str);
    alloc_listen_str.num_tpn = 1;
    STcopy(dcb->tp_profile, alloc_listen_str.tpn_profile_name);
 
    AtoE(dcb->tpn_name, &alloc_listen_str.tpn_list[0]);

    GCTRACE(3) ("lu62register:ALLOCATE_LISTEN for %s;%s \n",
            alloc_listen_str.tpn_profile_name, alloc_listen_str.tpn_list[0]);
 
    if (ioctl(listen_fd, ALLOCATE_LISTEN, &alloc_listen_str) < 0)
    {
        GCTRACE(1) ("lu62register:ALLOCATE_LISTEN for %s failure errno %d\n",
            alloc_listen_str.tpn_list[0], errno);
        status = FAIL;
    }

    if (alloc_listen_str.tpn_mask != 0)
    {
        GCTRACE(1)("lu62reg:ALLOC_LISTEN for %s fail_errno %d tpn_mask != 0\n",
            alloc_listen_str.tpn_list[0], errno);
        status = FAIL;
    }

    if (status == OK)
    {
        GCTRACE(4) ("lu62register: registered for connection %s tp %s %s\n",
            ccb, alloc_listen_str.tpn_list[0], 
            alloc_listen_str.tpn_profile_name);
    }

    return status;
}

/*
** Name: lu62name - determine Connection profile name and TP name
**
** Description:
**    Returns our connection_profile, tp_profile and tp_name 
**    Uses the value of ii.(host).gcc.*.sna_lu62.port
**
** Inputs:
**    Protocol ID
**    Buffers for connection_profile, tp_profile, and tp_name
**
** Returns:
**    OK if successful
**    FAIL if not
**
*/
static STATUS
lu62name(protocol_id, cp_name, tp_profile, tp_name)
char     *protocol_id;
char     *cp_name;
char     *tp_profile;
char     *tp_name;
{
    char *inst;
    char *val = 0;
    char pmsym[128];
    char *period;
 
    /*
    ** Port listen PM resource lines are of the form
    **  ii.(host).gcc.*.sna_lu62.port:   conn_profile.tp_profile.tp_name
    */

    STprintf(pmsym, "!.%s.port", protocol_id);

    /* check to see if entry for given user */
    if (PMget (pmsym,  &val) != OK)
        return FAIL;
 
    /* 
    ** Now split up name into conn_profile, tp_profile and tp_name 
    ** The format is conn_profile.tp_profile.tp_name 
    */
 
    /* get cp_profile */
    period = STchr(val, '.');        
    if (period == NULL)
    {
        return FAIL;
    }
    else
    {
        /* Copy up to period */
        STncpy( cp_name, val, period - val);
	cp_name[ period - val ] = EOS;
    }
 
    /* get tp_profile */
    val = ++period;
    period = STchr(val, '.');        
    if (period == NULL)
    {
        return FAIL;
    }
    else
    {
        /* Copy up to period */
        STncpy( tp_profile, val, period - val);
	tp_profile[ period - val ] = EOS;
    }
 
    /* get tp_name */
    ++period ;
    STcopy(period, tp_name); 
    return OK ;
}

/*
** Name: lu62trace - determine trace levels for protocol driver
**
** Description:
**   Sets the protocol driver trace level.
**
** Inputs:
**   None
**
** Returns:
**   None
**
*/
static VOID
lu62trace()
{
    char                *trace;
 
    /* Get trace value */

    NMgtAt("II_GCCL_TRACE", &trace);
    if (trace && *trace)
    {
        trace_level = atoi(trace);
    }
 
    return;
}

static STATUS
AtoE(inbuf, outbuf)
char * inbuf;
char * outbuf;
{
#if defined(any_aix)
const char *ip;
#else
char *ip;
#endif
char *op;
unsigned long InBytesLeft;
unsigned long OutBytesLeft;
iconv_t cd;
size_t r;
STATUS s = OK;

        cd = iconv_open (ASCII, EBCDIC);
        InBytesLeft = strlen(inbuf);

        OutBytesLeft = InBytesLeft;
        memset (outbuf, 0, sizeof(outbuf));
        ip = inbuf;
        op = outbuf;
        r = iconv(cd, &ip, &InBytesLeft, &op, &OutBytesLeft);
        if (r == -1)
        {
                GCTRACE(1) ("A-E translate failure\n");
                s = FAIL;
        }
        iconv_close(cd);
        return s;
}
 
#endif /* xCL_022_AIX_LU6_2 */
#endif /* aix */
