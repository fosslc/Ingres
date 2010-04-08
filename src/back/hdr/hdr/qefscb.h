/*
**Copyright (c) 2004 Ingres Corporation
**
*/

/**
** Name: QEFSCB.H - QEF system control block
**
** Description:
**      This control block is the system wide QEF control block.
**
** History: $Log-for RCS$
**      24-mar-86 (daved)
**          written
**	20-aug-88 (carl)
**	    added qef_v1_distrib
**	26-sep-88 (puree)
**	    modify QEF_S_CB for QEF in-memory sort.
**      14-jul-89 (jennifer)
**          added indicator of server state, for example running
**          C2SECURE to QEF_S_CB.
**	28-Oct-92 (jhahn)
**	    Added value lock for handling set input procedures.
**	06-mar-96 (nanpr01)
**	    Added new field qef_maxtup in the QEF_S_CB structure for 
**	    increased tuple size project.
**	01-Apr-1998 (hanch04)
**	    Added qef_qescb_offset.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    Added GLOBALREF for Qef_s_cb.
**	    Added GET_QEF_CB macro to obtain session's QEF_CB*
**	    from SCF's SCB instead of calling SCU_INFORMATION to
**	    do it.
**      19-Feb-2004 (hanal04) Bug 109118 INGSRV2000
**          Update GET_QEF_CB macro to make it Admin thread aware.
**          The Admin thread has no QEF_CB and has a sid of 1.
**          casting through this sid causes a SEGV.
**      12-Apr-2004 (stial01)
**          Define qef_length as SIZE_TYPE.
**      7-oct-2004 (thaju02)
**          Replace i4 with SIZE_TYPE for memory pool > 2Gig.
**/

/*
**  Forward and/or External typedef/struct references.
*/

typedef struct _QEF_S_CB QEF_S_CB;

/*
** Macro to get pointer to this session's QEF_CB
** directly from SCF's SCB:
*/
#define	GET_QEF_CB(sid) \
        (sid == CS_ADDER_ID ? (QEF_CB *)NULL : \
	*(QEF_CB**)((char *)sid + Qef_s_cb->qef_qescb_offset))

GLOBALREF	QEF_S_CB	*Qef_s_cb;


/*}
** Name: QEF_S_CB - QEF system control block
**
** Description:
**      This control block is used for creating and managing the
** server wide QEF control structure.
**
** History:
**	29-apr-86 (daved)
**          written
**	20-aug-88 (carl)
**	    added new field qef_v1_distrib
**	26-sep-88 (puree)
**	    changes for QEF in-memory sort
**      14-jul-89 (jennifer)
**          added indicator of server state, for example running
**          C2SECURE to QEF_S_CB.
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedef to quiesce the ANSI C 
**	    compiler warnings.
**	28-Oct-92 (jhahn)
**	    Added value lock for handling set input procedures.
**	03-feb-93 (andre)
**	    added qef_rdffcb
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	06-mar-96 (nanpr01)
**	    Added new field qef_maxtup in the QEF_S_CB structure for 
**	    increased tuple size project.
**	28-jan-00 (inkdo01)
**	    Added qef_hash_maxmem for hash join implementation.
**	04-Nov-2004 (jenjo02)
**	    Added QEF_S_IS_MT, set if OS-threaded server.
**	24-Oct-2005 (schka24)
**	    Add memory-nap delay max (in milliseconds).
**	6-Mar-2007 (kschendel) SIR 122512
**	    Add hash read/write buffer goal sizes.
**      7-jun-2007 (huazh01)
**          Add qef_no_dependency_chk for the config parameter which switches
**          ON/OFF the fix to b112978.
*/
struct _QEF_S_CB
{
    /* standard stuff */
    QEF_S_CB        *qef_next;      /* The next control block */
    QEF_S_CB        *qef_prev;      /* The previous one */
    SIZE_TYPE       qef_length;     /* size of the control block */
    i2              qef_type;       /* type of control block */
#define QEFSCB_CB    7
    i2              qef_s_reserved;
    PTR             qef_l_reserved;
    PTR             qef_owner;
    i4              qef_ascii_id;   /* to view in a dump */
#define QEFSCB_ASCII_ID  CV_C_CONST_MACRO('Q', 'E', 'S', 'C')
    PTR             qef_ulhid;      /* ULH table id for DSH cache */
    PTR		    qef_qefcb;	    /* PTR to list of QEC_CBs */
    SIZE_TYPE	    qef_dsh_maxmem; /* max amount of memory for DSHs */
    i4	    	    qef_sort_maxmem;/* max amount of memory for SORT */
    i4	    	    qef_hash_maxmem;/* max amount of memory for hash joins */
    bool            qef_init;       /* TRUE if initialized */
    i4              qef_qpmax;      /* max number of QPs */
    i4		    qef_sess_max;   /* max number of active sessions */
    i4         qef_maxtup;     /* maximum tuple size in installation */
    ULM_RCB         qef_d_ulmcb;    /* ulm_cb template for DSH memory */
    ULM_RCB	    qef_s_ulmcb;    /* ulm_cb template for SORT memory */
    QEF_FUNC        qef_func[QEF_MAX_FUNC]; 
                                    /* routines to perform QEF functions */
    ULT_VECTOR_MACRO(QEF_SNB, QEF_SNVAO) qef_trace; /* server trace flag */   
    CS_SEMAPHORE    qef_sem;	    /* semaphore for access to this struct */
    i4         qef_state;      /* State indicators, for example
                                    ** QEF_S_C2SECURE is set if running 
                                    ** a C2 server.  This is a mask.
                                    */
#define             QEF_S_C2SECURE  0x0001
#define             QEF_S_IS_MT     0x0002 /* OS-threaded server */
    /* STAR stuff */
    DB_DISTRIB	    qef_v1_distrib; /* DB_DSTYES implies DDB server in effect,
				    ** initialized by QEC_INITIALIZE */
    i4   qef_drop_procedure_value_lock;    /* Value lock for checking when
					   ** we need to verify for the
					   ** existence of the stored
					   ** procedures that a query plan
					   ** uses.
					   */
    i4  qef_procedure_dropped; /* Flag to indicate that a procedure
			       ** has been dropped within the
			       ** current transaction.
			       */
    PTR	qef_rdffcb;		    /* RDF's facility control block */
    i4              qef_qescb_offset;   /* Offset of QEF_CB in SCB */
    i4		    qef_max_mem_nap;	/* Max delay for memory in ms */
    BITFLD          qef_no_dependency_chk:1;  /* used for the config parameter
                                              ** which switches ON/OFF the fix
                                              ** to b112978. */ 

    u_i4	    qef_hash_cmp_threshold; /* Consider compressing if
					** hashop row is at least this big.
					*/
    u_i4	    qef_hashjoin_min;	/* Minimum hash join reservation */
    u_i4	    qef_hashjoin_max;	/* Maximum hash join reservation */
/* The following sizes are configuration "suggested" sizes;  qee is free to
** fiddle with them as needed for any given hash join or aggregation.
** Both sizes are in bytes, and qec initialization ensures that they are
** a multiple of the fundamental hash file page size HASH_PGSIZE
** (which is defined in a qef header).
*/
    u_i4	    qef_hash_rbsize;	/* Suggested read buffer size */
    u_i4	    qef_hash_wbsize;	/* Suggested write buffer size */
};
