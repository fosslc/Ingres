/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DM0LLCTX.H - Logging system control blocks.
**
** Description:
**      This structure describes the control blocks used by the logging
**	system.
**
**	A DMP_LCTX structure is created by calling dm0l_allocate. In a single
**	node logging system (that is, everywhere except the VAXCluster), each
**	process needs a DMP_LCTX to access the local logging system. In a
**	multi-node logging system, most processes still only access the local
**	logging system and hence still only have a single DMP_LCTX instance.
**	The CSP process, however, manages simultaneous recovery of multiple
**	nodes, and hence manages multiple LCTX structures.
**
** History:
**      24-jul-1986 (Derek)
**          Created for Jupiter.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**	    Enhancements for multi-node cluster support. Information about the
**	    current state of the log file, such as BOF, EOF, and last CP, which
**	    we formerly kept in other recovery structures, we now keep here.
**	    This helps support operations such as multi-node restart recovery,
**	    which need to simultaneously access log files from multiple nodes.
**	23-aug-1993 (rogerk)
**	    Added lctx_bkcnt field to allow users to reference the log file
**	    size.  This facilitates log address arithmetic.
[@history_template@]...
**/

/*}
** Name: DM0LLCTX - The logging system read context.
**
** Description:
**      This structure decribes the control block used by the logging routines
**	to position and read from the log file.
**
** History:
**      24-jul-1986 (Derek)
**          Created for Jupiter.
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedef to quiesce the ANSI C 
**	    compiler warnings.
**	26-apr-1993 (bryanp)
**	    Enhancements for multi-node cluster support. Information about the
**	    current state of the log file, such as BOF, EOF, and last CP, which
**	    we formerly kept in other recovery structures, we now keep here.
**	    This helps support operations such as multi-node restart recovery,
**	    which need to simultaneously access log files from multiple nodes.
**	    Added a node_id field to the lctx to hold the logfile's node ID.
**	23-aug-1993 (rogerk)
**	    Added lctx_bkcnt field to allow users to reference the log file
**	    size.  This facilitates log address arithmetic.
**	05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	30-nov-93 (swm)
**          Bug #58635
**	    Changed type of lctx_1_reserved and lctx_2_reserved from i4 to
**	    i4 * to match corresponding standard DMF header elements.
**      08-Oct-2002 (hanch04)
**          Replace the i4 for length with a SIZE_TYPE (size_t) to allow
**          64-bit memory allocation.
**	30-Mar-2005 (jenjo02)
**	    Added LCTX_WAS_RECOVERED lctx_status flag, set
**	    during cluster recovery if recovery on that
**	    log was really needed.
**	03-may-2005 (hayke02)
**	    Add pool_type to match obj_pool_type in DM_OBJECT.
**	25-Jul-2005 (sheco02)
**	    Fixed the typo caused by the previous x-integration 478041.
**      05-Aug-2005 (hanje04)
**          Back out change 478041 as it should never have been X-integrated
**          into main in the first place.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Define LCTX_CB as DM_LCTX_CCB.
*/
struct _DMP_LCTX
{
    i4	    *lctx_1_reserved;	    /* Not used. */
    i4	    *lctx_2_reserved;	    /* Not used. */
    SIZE_TYPE	    lctx_length;	    /* Length of control block. */
    i2              lctx_type;              /* Type of control block for
                                            ** memory manager. */
#define                 LCTX_CB             DM_LCTX_CB
    i2              lctx_s_reserved;        /* Reserved for future use. */
    PTR             lctx_l_reserved;        /* Reserved for future use. */
    PTR             lctx_owner;             /* Owner of control block for
                                            ** memory manager.  lctx will be
                                            ** owned by the server. */
    i4         lctx_ascii_id;          /* Ascii identifier for aiding
                                            ** in debugging. */
#define                 LCTX_ASCII_ID	    CV_C_CONST_MACRO('L', 'C', 'T', 'X')
    DM_MUTEX	    lctx_mutex;		    /* Lctx mutex. */
    i4	    lctx_status;	    /* Context status. */
#define			LCTX_NOLOG	    0x01L
#define			LCTX_WAS_RECOVERED  0x02L
    i4	    lctx_lgid;		    /* Logging identifier of owner. */
    i4	    lctx_lxid;		    /* Logging system internal
					    ** transaction id. */
    i4	    lctx_dbid;		    /* Logging system default 
					    ** database id. */
    LG_HEADER	    lctx_lg_header;	    /* buffer space for log ehader */
    char	    *lctx_header;	    /* LG log record header. */
    i4	    lctx_l_area;	    /* Length of context area. */
    PTR		    lctx_area;		    /* Pointer to logging context area.
					    ** Normally this follows the control
					    ** block in memory. */
    LG_LA	    lctx_cp;		    /* Last known consistency point in
					    ** this log file. */
    LG_LA	    lctx_bof;		    /* begin of file */
    LG_LA	    lctx_eof;		    /* end of file. */
    i4	    lctx_bksz;		    /* Block size of log file. */  
    i4	    lctx_bkcnt;		    /* Block count of log file. */  
    i4	    lctx_node_id;	    /* node ID for this logfile. In
					    ** a non-cluster, this is set by
					    ** LGshow(LG_A_NODEID) when the
					    ** lctx is allocated. In a multi-
					    ** node logging system, the CSP
					    ** process manages the setting of
					    ** lctx_node_id.
					    */
};
