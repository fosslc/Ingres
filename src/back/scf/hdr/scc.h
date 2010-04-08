/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: SCC.H - This file describes the data structures used by SCC
**
** Description:
**      This file describes the data structures which are used and 
**      manipulated primarily by the SCF communications module (SCC).
[@comment_line@]...
**
** History:
**      17-Jan-86 (fred)
**          Created on jupiter
**	29-nov-89 (fred)
**	    Added support for large datatypes.
**	21-mar-90 (neil)
**	    Added cscb_evdata and defined SCC_GCEV_MSG for alerts.
**	2-Jul-1993 (daveb)
**	    extern public entries.  Will prototype later.
**	9-nov-93 (robf)
**          Add scc_prompt prototype
**	23-sep-1997 (canor01)
**	    Preliminary changes for Frontier.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-Jan-2001 (jenjo02)
**	    Added (SCD_SCB*)scb parameter to protocols for
**	    scc_error(), scc_trace(), scc_relay().
**      12-Apr-2004 (stial01)
**          Define scg_length as SIZE_TYPE.
**/

/*
**  Forward and/or External typedef/struct references.
*/

typedef struct _SCC_GCMSG SCC_GCMSG;

/*}
** Name: SCC_GCQUE - A queue structure for SCC_GCMSG.
**
** Description:
**      This structure holds the next and prev fields for a queue of SCC_GCMSG
**      elements.
[@comment_line@]...
**
** History:
**      25-oct-88 (eric)
**          created
[@history_template@]...
*/
typedef struct _SCC_GCQUE
{
    SCC_GCMSG   *scg_next;
    SCC_GCMSG   *scg_prev;
} SCC_GCQUE;

/*}
** Name: SCC_GCMSG - Message queue member for SCF/GCA processing
**
** Description:
**      This structure holds messages destined for the FE.  A list
**      of these is constructed as queries are processed, and sent
**      back to the FE as necessary.
**
** History:
**      09-Jul-1987 (fred)
**          Created.
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedef to quiesce the ANSI C 
**	    compiler warnings.
**      29-Apr-1992 (fred)
**	    Added support for large datatypes.  W/in this file, this is the
**	    addition of the SCG_INCOMPLETE_MASK bit for the SCC_GCMSG field,
**	    which indicates by its presence that the message is incomplete,
**	    and that, therefore, the end of data indicator in GCF should not be
**	    turned on.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
[@history_template@]...
*/
struct _SCC_GCMSG
{
    SCC_GCMSG         *scg_next;        /* Forward link */
    SCC_GCMSG	      *scg_prev;
    SIZE_TYPE	      scg_length;	/* Length, including portion not seen */
    i2                scg_type;		/* control block type */
#define                 SC_SCG_TYPE     0x200	/* make it obvious in dumps */
    i2                scg_s_reserved;   /* used by mem mgmt perhaps */
    PTR		      scg_l_reserved;
    PTR               scg_owner;        /* owner of block */
    i4                scg_tag;          /* to look for in a dump (ick) */
#define                 SCG_TAG         CV_C_CONST_MACRO('s','c','g','_')
    i4		      scg_mask;		/* various bits of status */

		/* SCG_NODEALLOC_MASK is set when the message should *not*
		** be automatically deallocated. If not set, then the message
		** will be deallocated when the message has been sent
		** and GCF is finished with it.
		*/
#define			SCG_NODEALLOC_MASK	    0x1

		/* SCG_QDONE_DEALLOC_MASK indicates that the message should
		** be deallocated when the current query is completely
		** finished. Note that SCG_NODEALLOC_MASK must also be
		** specified.
		*/
#define			SCG_QDONE_DEALLOC_MASK	    0x2
		/*
		** SCG_NOT_EOD_MASK indicates that the message should not have
		** gca_end_of_data set to TRUE. It is used for partial messages
		** relayed from the LDB in direct connect mode.  This is also
		** used for large datatype requests, where the entire tuple
		** won't fit into memory, much less into a single message
		** portion.
		*/
#define			SCG_NOT_EOD_MASK	    0x4

    PTR		      scg_mdescriptor;	/* Tuple descriptor for send/recieve */
    i4		      scg_mtype;        /* Message type */
    i4		      scg_msize;	/* size of message */
    PTR		      scg_marea;	/* formatted message buffer */
};

/*}
** Name: SCC_CSCB - Communications Session control block
**
** Description:
**      This area describes the section in a session control block
**      which is used primarily by the system control communications
**      module.  Information kept here may be used by others, but
**      is under the control of the communications module.
**
** History:
**     15-Jan-86 (fred)
**          Created on jupiter.
**	21-mar-90 (neil)
**	    Added cscb_evmsg & cscb_evdata for alerts.
**	30-May-2009 (kiria01) SIR 121665
**	    Added cscb_batch_count for batch processing
**	October 2009 (stephenb)
**	    Batch processing: add cscb_eog_error and cscs_in_group
*/
typedef struct _SCC_CSCB
{
    i4		    cscb_version;	/* what v of ingres do they speak */
    i4		    cscb_new_stream;	/* starting a new stream */
			/*
			** This field also contains a pointer to the last
			** message sent, so that it can be easily found to be
			** deleted.
			*/
    i4		    cscb_assoc_id;	/* GCF association id */
    i4		    cscb_comm_size;	/* Size of "natural" communications */
    i4		    cscb_dsize;		/* Size of descriptor buffer */
    SCC_GCQUE	    cscb_mnext;		/* list of messages to send */
    GCA_PARMLIST    cscb_gcp;		/* parameter list */
    GCA_RV_PARMS    cscb_gci;		/* input request block */
    GCA_SD_PARMS    cscb_gco;		/* output request block */
    GCA_RV_PARMS    cscb_gce;		/* expedited request block */
    SCC_GCMSG	    cscb_rspmsg;	/* response message location */
    SCC_GCMSG	    cscb_rpmmsg;	/* return proc message location */
    SCC_GCMSG	    cscb_evmsg;		/* Event message */
    SCC_GCMSG	    cscb_dscmsg;	/* descriptor message */
    GCA_RE_DATA	    *cscb_rdata;	/* normal response block */
    GCA_RP_DATA	    *cscb_rpdata;	/* procedure response block */
    GCA_EV_DATA	    *cscb_evdata;	/* Event message notification block.
					** Really points to a buffer of size
					** SCC_GCEV_MSG.
					*/
    char	    *cscb_inbuffer;	/* to get input into */
    char	    *cscb_dbuffer;	/* descriptor buffer */
    char	    *cscb_darea;	/* descriptor area */
    SCC_GCMSG	    *cscb_outbuffer;	/* standard output loc */
    char	    *cscb_tuples;	/* formatted location */
    i4         cscb_tuples_max;    /* max. length of outbuf data area */
    i4         cscb_tuples_len;    /* current length of out message */
    char	    cscb_intr_buffer[64];
    i4		    cscb_different;	/* Is the FE on a different type CPU */
    i4		    cscb_incomplete;	/*
					** This field holds the message type of
					** the last message, if that message was
					** incomplete.  If the last message was
					** not incomplete, then this field will
					** be zero.
					**
					** This field is of importance for the
					** following reason.  Once an incomplete
					** message has been sent, no other
					** message can be sent until that
					** message is completed.  Therefore, if
					** this field is non-zero, then no
					** messages can be sent until a message
					** of the same type, marked complete, is
					** found.  This assumes that the
					** remainder of the DBMS will send the
					** completion for this message as the
					** next message of the same type.
					*/

	/* Cscb_qdone is a queue of messages to be deallocated
	** when the "query is finished". In most cases, ie. all DDL and the
	** set oriented DML, the query is finished at the expected point.
	** In the cursor cases, the "query is finished" when the piticular
	** cursor command (open, fetch, etc) is finished, rather when the
	** whole cursor is finished.
	*/
    SCC_GCQUE	    cscb_qdone;

    char            *cscb_msgbuf;       /* message buffer */
    char            *cscb_msgbuf_root;  /* root of the buffer */
    i4         cscb_msgbuf_length; /* length of the buffer */
    i4         cscb_msgbuf_free;   /* free space in the buffer */
    u_i4	    cscb_batch_count;
    bool	cscb_eog_error; /* error was severe enough to terminate group */
    bool	cscb_in_group; /* processing a group */
}   SCC_CSCB;

/*
** Name: SCC_GCEV_MSG - Dummy event message (for allocation sizes)
**
** Description:
**	This structure is only defined to allocate a single GCA_EVENT message
**	which is variable length.  Because of the varying length fields the
**	structure includes the GCA_EV_DATA structure plus place holders for
**	the run-time determined values.  This is only allocated once in scdmain
**	so some extra space does not really impact anything, plus the
**	pre-allocation reduces CPU overhead.
**
**	It is important to put this in a structure so that when it is "picked"
**	out of a larger allocation (scdmain) it does not throw the alignment
**	off.
**
** History:
**      21-mar-90 (neil)
**          Created for alerters.
*/
typedef struct
{
    GCA_EV_DATA		scg_ev_dummy;		/* Known length fields */
    DB_ALERT_NAME	scg_evname_dummy;	/* Pad out with real names */
    DB_DATE		scg_evdate_dummy;	/* And real date */
    char		scg_evval_dummy[DB_EVDATA_MAX];	/* Value, in case */
} SCC_GCEV_MSG;


/* Func externs */

FUNC_EXTERN DB_STATUS	scc_error( i4  operation, SCF_CB *scf_cb,
				   SCD_SCB *scb );
FUNC_EXTERN DB_STATUS	scc_relay( SCF_CB *scf_cb,
				   SCD_SCB *scb );
FUNC_EXTERN DB_STATUS	scc_trace( SCF_CB *scf_cb,
				   SCD_SCB *scb );
FUNC_EXTERN STATUS	scc_recv( SCD_SCB *scb, i4  sync );
FUNC_EXTERN STATUS	scc_send( SCD_SCB *scb, i4  sync );
FUNC_EXTERN VOID	scc_dispose( SCD_SCB *scb );
FUNC_EXTERN VOID	scc_fa_notify( SCD_SCB *scb );
FUNC_EXTERN VOID	scc_gcomplete( SCD_SCB *scb );
FUNC_EXTERN VOID	scc_iprime( SCD_SCB *scb );

FUNC_EXTERN DB_STATUS scc_prompt( SCD_SCB *scf_cb, 
    char 	*mesg, 
    i4  	mesglen, 
    i4   	replylen,
    bool 	noecho,
    bool 	password,
    i4		timeout
) ;
