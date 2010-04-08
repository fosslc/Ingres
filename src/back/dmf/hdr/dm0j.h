/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <jf.h>

/**
** Name: DM0J.H - Defines journal routines and structures.
**
** Description:
**      This file describes the constants and structures needed to interface
**	to the journal routines.
**
** History:
**      04-aug-1986 (Derek)
**          Created for Jupiter.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    6-jun-1991 (bryanp)
**		Add dm0j_node_id to the DM0J_CTX structure for error logging.
**	07-july-1992 (jnash)
**	    Add DMF function prototypes.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Added support for backward reading of journal files for rollforward
**		undo.  Added next_recnum value in the journal context structure
**		and a jh_first_rec offset in the journal block header.
**	    Added dm0j_backread routine.
**	    Added direction argument to dm0j_open.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Modify calling sequence to dm0j_merge so that it can be called
**		by auditdb, thus allowing us to remove the duplicated code in
**		dmfatp.c.
**		Journal file merging is now done by Log Sequence
**		    numbers, not by Log Page Stamps. DM0J_CNODE structure now
**		    has lsn, record, and l_record fields to hold a record and
**		    its Log Sequence Number for each journal file being merged.
**	23-aug-1993 (bryanp)
**	    Correct prototype for dm0j_merge().
**      14-sep-1994 (andyw)
**          Partial Backup/Recovery Project:
**              Added prototype for dm0j_filename.
**	10-apr-1995 (rudtr01)
**	    Bug #63923 - changed value of DM0J_MRZ from 4096 to 4100 to 
**	    accomodate updates of 2008-byte types.
**      06-mar-1996 (stial01)
**          Variable Page Size project:
**          Changed DM0J_MRZ to be consistent with LG_MAX_RSZ (66000)
**          It should be the same as the size of the largest log record.
**	15-jan-1999 (nanpr01)
**	    Pass pointer to a pointer to dm0j_close.
**      12-Jul-2000 (zhahu02)
**          Changed DM0J_MRZ to be consistent with LG_MAX_RSZ (66000-> 66100).
**          (B102098).
**      11-jul-2002 (stial01)
**          Changed DM0J_MRZ to be consistent with LG_MAX_RSZ 
**      08-Oct-2002 (hanch04)
**          Replace the i4 for length with a SIZE_TYPE (size_t) to allow
**          64-bit memory allocation.
**      05-Aug-2005 (hanje04)
**          Back out change 478041 as it should never have been X-integrated
**          into main in the first place.
**      16-jul-2008 (stial01)
**          Added DM0J_RFP_INCR for incremental rollforwarddb
**	18-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add function prototype for dm0j_position().
**	    Add #include <jf.h>, deleted DM0J_MRZ; LG_MAX_RSZ will do 
**	    just fine. Add DM0J_KEEP_CTX.
**	
[@history_template@]...
**/

/*
**  Constants for mode parameter of DM0J_OPEN
*/

#define		DM0J_M_READ		    1L
#define		DM0J_M_WRITE		    2L

/*
**  Constants for flag parameter of DM0J_CREATE, dm0j_open
*/

#define		DM0J_MERGE		    1L
#define		DM0J_RFP_INCR		    2L
#define		DM0J_KEEP_CTX		    4L	/* dm0j_open only */

/*
**  Forward type definitions.
*/

typedef struct _DM0J_CTX    DM0J_CTX;
typedef struct _DM0J_HEADER DM0J_HEADER;
typedef struct _DM0J_CNODE  DM0J_CNODE;
typedef struct _DM0J_CFILE  DM0J_CFILE;
typedef struct _DM0J_CS     DM0J_CS;

/*}
** Name: DM0J_CTX - Journal context.
**
** Description:
**      This structure defines the information needed performa journal
**	operations.
**
** History:
**      04-ayg-1986 (Derek)
**          Created for Jupiter.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    6-jun-1991 (bryanp)
**		Add dm0j_node_id to the DM0J_CTX structure for error logging.
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedefs to quiesce the ANSI C 
**	    compiler warnings.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Added support for backward reading of journal files for rollforward
**		undo.  Added next_recnum value in the journal context structure.
**	05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	13-jan-94 (swm)
**          Bug #58635
**	    Deleted dm0j_2_reserved which is unused to make DM0J_CTX header
**	    consistent with standard DM_OBJECT header.
**	10-apr-1995 (rudtr01)
**	    Bug #63923 - changed value of DM0J_MRZ from 4096 to 4100 to 
**	    accomodate updates of 2008-byte types.
**      06-mar-1996 (stial01)
**          Variable Page Size project:
**          Changed DM0J_MRZ to be consistent with LG_MAX_RSZ (66000)
**          It should be the same as the size of the largest log record.
**      18-feb-2004 (stial01)
**          Increased DM0J_MRZ to be consistent with LG_MAX_RSZ to support
**          larger row size for page size 64k. (no longer limited to maxi2) 
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Define DM0J_CB as DM_DM0J_CB.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add dm0j_last_lsn for dm0j_position.
**	    Deleted DM0J_MRZ - use LG_MAX_RSZ instead.
**	03-Feb-2010 (jonj)
**	    Added dm0j_opened, TRUE when dm0j_jfio has been opened.
[@history_template@]...
*/
struct _DM0J_CTX
{
    DM0J_CTX	    *dm0j_next;		    /* Next journal context. */
    DM0J_CTX	    *dm0j_prev;		    /* Next journal context. */
    SIZE_TYPE	    dm0j_length;  	    /* Length of control block. */
    i2              dm0j_type;              /* Type of control block for
                                            ** memory manager. */
#define                 DM0J_CB             DM_DM0J_CB
    i2              dm0j_s_reserved;        /* Reserved for future use. */
    PTR             dm0j_l_reserved;        /* Reserved for future use. */
    PTR             dm0j_owner;             /* Owner of control block for
                                            ** memory manager.  DM0J will be
                                            ** owned by the server. */
    i4         dm0j_ascii_id;          /* Ascii identifier for aiding
                                            ** in debugging. */
#define                 DM0J_ASCII_ID       CV_C_CONST_MACRO('D', 'M', '0', 'J')
    char	    *dm0j_record;	    /* Record buffer. */
    char	    *dm0j_page;		    /* Page buffer. */
    char	    *dm0j_next_byte;	    /* Next byte within buffer. */
    i4	    dm0j_bytes_left;	    /* Number of bytes left 
                                            ** in buffer. */
    i4	    dm0j_next_recnum;	    /* Next record on the page to return
					    ** - used in backward reads */
    LG_LSN	    dm0j_last_lsn;	    /* Last LSN read on this page */
    i4	    dm0j_sequence;	    /* Journal page sequence number. */
#define			DM0J_EXTREME		-1
    i4	    dm0j_jnl_seq;	    /* Journal sequence number. */
    i4	    dm0j_ckp_seq;	    /* Checkpoint sequence number. */
    i4	    dm0j_bksz;		    /* Journal block size. */
    i4	    dm0j_direction;	    /* dm0j_read direction */
#define			DM0J_FORWARD		0
#define			DM0J_BACKWARD		1
    DB_DB_NAME	    dm0j_dbname;	    /* Database name. */
    i4	    dm0j_node_id;	    /* Node ID if this journal file is
					    ** a VAXCluster node journal file;
					    ** -1 if this journal file is a
					    ** standard journal file. This field
					    ** is used only for error logging.
					    */
    DB_TAB_ID	    dm0j_sf_tabid;	    /*
					    ** Tabid if journal file is
					    ** for online modify
					    */
    bool	    dm0j_opened;	    /* TRUE if file opened */
    JFIO            dm0j_jfio;		    /* JF context. */
};

/*}
** Name: DM0J_HEADER - Every journal page is begins with this header.
**
** Description:
**      This structure is recorded on the beginning of each journal page.
**	The structure is used to make various checks about the consistency
**	of the journal file, and information about the version of this
**	journal format.
**
** History:
**      20-sep-1986 (Derek)
**          Created for Jupiter.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Added support for backward reading of journal files for rollforward
**		undo.  Added a jh_first_rec offset in the journal block header.
[@history_template@]...
*/
struct _DM0J_HEADER
{
    i4         jh_type;            /* The type of journal page. */
#define			JH_V1_S1	    0x10001
    DB_DB_NAME	    jh_dbname;		/* Database name. */
    i4	    jh_seq;		/* Journal page sequence number. */
    i4	    jh_jnl_seq;		/* Journal file sequence. */
    i4	    jh_ckp_seq;		/* Checkpoint sequence. */
    i4	    jh_offset;		/* Offset to last used byte of page. */
    i4	    jh_first_rec;	/* Offset to the first record in the 
					** journal block that begins there (ie
					** is not a continuation of a record
					** from the previous block. */
    LG_LSN	jh_low_lsn;	/* Lowest LSN in page */
    LG_LSN	jh_high_lsn;	/* Highest LSN in page */
};

/*}
** Name: DM0J_CNODE - Node context.
**
** Description:
**      This structure defines the information needed to keep node
**	information during a merge of node journals into standard
**      journals.
**
** History:
**      24-jun-1987 (Jennifer)
**          Created for Jupiter.
**	26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Journal file merging is now done by Log Sequence
**		    numbers, not by Log Page Stamps. DM0J_CNODE structure now
**		    has lsn, record, and l_record fields to hold a record and
**		    its Log Sequence Number for each journal file being merged.
[@history_template@]...
*/
struct _DM0J_CNODE
{
    i4         count;                  /* Number of journal files
                                            ** associated with this node. */
    i4         state;                  /* Current state of this entry. */
#define                 DM0J_CVALID         2
#define                 DM0J_CINVALID       4
    i4         cur_fil_seq;            /* Current file sequence number. */
    i4         last_fil_seq;           /* Last file sequence number. */

    DM0J_CTX	    *cur_jnl;		    /* Pointer to journal context. */
    LG_LSN	    lsn;		    /* current log sequence number for
					    ** this journal file. */
    i4	    l_record;		    /* length of journal file record */
    char	    *record;		    /* pointer to allocated record
					    ** buffer. */
};

/*}
** Name: DM0J_CFILE - File context.
**
** Description:
**      This structure defines the information needed to keep file
**	information during a merge of node journal files into standard
**      journals.
**
** History:
**      24-jun-1987 (Jennifer)
**          Created for Jupiter.
[@history_template@]...
*/
struct _DM0J_CFILE
{
    i4         node_id;                /* Node-id of Node that 
                                            ** created associated file. */
    i4         seq;                    /* Sequence number of file. */
    DM_FILE         filename;               /* Filename. */
    i4         l_filename;             /* Length of file name. */
};

/*}
** Name: DM0J_CS - Store File context.
**
** Description:
**      This structure defines the information needed to pass to the
**	store files routine when getting the list of files to merge.
**
** History:
**      24-jun-1987 (Jennifer)
**          Created for Jupiter.
[@history_template@]...
*/
struct _DM0J_CS
{
    i4         index;                  /* Number of entries. */
    i4         first;                  /* First valid sequence number.*/
    i4         last;                   /* Last valid sequence number. */
    DM0J_CFILE      *file_array;            /* Pointer to array where 
                                            ** entries can be stored. */
};


 
/*
**  External function references.
*/

FUNC_EXTERN DB_STATUS dm0j_close(
			DM0J_CTX	**jctx,
			DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS dm0j_create(
			i4		flag,
			char		*device,
			i4		l_device,
			i4		jnl_seq,
			i4		bksz,
			i4		blkcnt,
			i4		node_id,
			DB_TAB_ID	*sf_tab_id,
			DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS dm0j_delete(
			i4		flag,
			char		*device,
			i4		l_device,
			i4		jnl_seq,
			DB_TAB_ID	*sf_tab_id,
			DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS dm0j_merge(
			DM0C_CNF	*cnf,
			DMP_DCB		*dcb,
			PTR		process_ptr,
			DB_STATUS	(*process_record)
					    (PTR process_ptr,
					    PTR record,
					    i4 *err_code),
			DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS dm0j_open(
			i4		flag,
			char		*device,
			i4		l_device,
			DB_DB_NAME	*db_name,
			i4		bksz,
			i4		jnl_seq,
			i4		ckp_seq,
			i4		sequence,
			i4		mode,
			i4		node_id,
			i4		direction,
			DB_TAB_ID	*sf_tab_id,
			DM0J_CTX	**jctx,
			DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS dm0j_position(
			DB_TRAN_ID	*tran_id,
			DM0J_CTX	**jctx,
			DMP_DCB		*dcb,
			PTR		CRhdr,
			PTR		*LogRecord,
			i4		*jread,
			i4		*jhit,
			DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS dm0j_read(
			DM0J_CTX	*jctx,
			PTR		*record,
			i4		*l_record,
			DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS dm0j_backread(
			DM0J_CTX	*jctx,
			PTR		*record,
			i4		*l_record,
			DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS dm0j_truncate(
			DM0J_CTX	*jctx,
			DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS dm0j_update(
			DM0J_CTX	*jctx,
			i4		*sequence,
			DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS dm0j_write(
			DM0J_CTX	*jctx,
			PTR		record,
			i4		l_record,
			DB_ERROR	*dberr );

FUNC_EXTERN VOID dm0j_filename(
                        i4             flag,
                        i4             jnl_seq,
                        i4             node_id,
                        DM_FILE             *filename,
			DB_TAB_ID	*sf_tab_id);
