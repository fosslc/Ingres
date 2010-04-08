/*
** Copyright (c) 1989, 2008 Ingres Corporation
*/

/**
** Name: DM0D.H - Defines Dump routines and structures.
**
** Description:
**      This file describes the constants and structures needed to interface
**	to the dump routines.
**
** History:
**      06-jan-1989 (Edhsu)
**          Created for Terminator.
**	 6-dec-1989 (rogerk)
**	    Added declaration of dm0d_config_restore.
**	7-July-1992 (rmuth)
**	    Prototyping DMF.
**	15-mar-1993 (jnash)
**	    Reassign the value of DM0D_CB to be unique.
**      14-sep-1994 (andyw)
**          Partial Backup/Recovery project:
**              Added prototype for dm0d_filename
**	06-may-1996 (nanpr01 for stial01)
**	    Increase the DM0D_MRZ to be in sync with DM0L_MRZ.
**	    Value increase to 66000 from 4096.
**	03-sep-1998 consi01 bug 91957 problem INGSRV 463
**	    Changes to support large dump records spanning blocks
**      21-jan-1999 (hanch04)
**          replace i4  and i4 with i4
**      10-May-1999 (stial01)
**          Added direction to dm0d_open prototype, Removed direction from 
**          dm0d_read prototype (related change 438561 for B91957
**      12-Jul-2000 (zhahu02)
**	    Increase the DM0D_MRZ to be in sync with DM0L_MRZ.
**	    Value increase to 66100 from 66000. (B102098).
**      08-Oct-2002 (hanch04)
**          Replace the i4 for length with a SIZE_TYPE (size_t) to allow
**          64-bit memory allocation.
**      01-sep-2004 (stial01)
**	    Support cluster online checkpoint
**      05-Aug-2005 (hanje04)
**          Back out change 478041 as it should never have been X-integrated
**          into main in the first place.
**	30-may-2008 (joea)
**	    Change prototype for dm0d_merge.
**	18-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code.
**/

/*
**  Constants for mode parameter of DM0D_OPEN
*/

#define		DM0D_M_READ		    1L
#define		DM0D_M_WRITE		    2L


/*
**  Constants for flag parameter of DM0D_CREATE
*/
#define		DM0D_MERGE		    1L


/*
**  Forward type definitions.
*/

typedef struct _DM0D_CTX    DM0D_CTX;
typedef struct _DM0D_HEADER DM0D_HEADER;
typedef struct _DM0D_CNODE  DM0D_CNODE;
typedef struct _DM0D_CFILE  DM0D_CFILE;
typedef struct _DM0D_CS     DM0D_CS;

/*}
** Name: DM0D_CTX - Dump context.
**
** Description:
**      This structure defines the information needed performa dump
**	operations.
**
** History:
**      06-jan-1989 (Edhsu)
**          Created for Terminator.
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedefs to quiesce the ANSI C 
**	    compiler warnings.
**	15-mar-1993 (jnash)
**	    Reassign the value of DM0D_CB to be unique.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	13-jan-94 (swm)
**          Bug #58635
**	    Deleted dm0d_2_reserved which is unused to make DM0D_CTX header
**	    consistent with standard DM_OBJECT header.
**	06-may-1996 (nanpr01 for stial01)
**	    Increase the DM0D_MRZ to be in sync with DM0L_MRZ.
**	    Value increase to 66000 from 4096.
**	03-sep-1998 consi01 bug 91957 problem INGSRV 463
**	    Added dm0d_next_recnum for use during backward read of record
**	    spanning blocks
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Define DM0D_CB as DM_DM0D_CB.
*/
struct _DM0D_CTX
{
    DM0D_CTX	    *dm0d_next;		    /* Next dump context. */
    DM0D_CTX	    *dm0d_prev;		    /* Next dump context. */
    SIZE_TYPE	    dm0d_length;  	    /* Length of control block. */
    i2              dm0d_type;              /* Type of control block for
                                            ** memory manager. */
#define                 DM0D_CB             DM_DM0D_CB
    i2              dm0d_s_reserved;        /* Reserved for future use. */
    PTR             dm0d_l_reserved;        /* Reserved for future use. */
    PTR             dm0d_owner;             /* Owner of control block for
                                            ** memory manager.  DM0D will be
                                            ** owned by the server. */
    i4         dm0d_ascii_id;          /* Ascii identifier for aiding
                                            ** in debugging. */
#define                 DM0D_ASCII_ID       CV_C_CONST_MACRO('D', 'M', '0', 'D')
    char	    *dm0d_record;	    /* Record buffer. */
#define			DM0D_MRZ		66100
    char	    *dm0d_page;		    /* Page buffer. */
    char	    *dm0d_next_byte;	    /* Next byte within buffer. */
    i4	    dm0d_bytes_left;	    /* Number of bytes left 
                                            ** in buffer. */
    i4         dm0d_next_recnum;       /* Next record on the page to return
                                            ** - used in backward reads */
    i4	    dm0d_sequence;	    /* Dump page sequence number. */
#define                 DM0D_EXTREME            -1
    i4	    dm0d_end_sequence;	    /* Last Dump block number.*/
    i4	    dm0d_dmp_seq;	    /* Dump sequence number. */
    i4	    dm0d_ckp_seq;	    /* Checkpoint sequence number. */
    i4	    dm0d_bksz;		    /* Dump block size. */
    i4      dm0d_direction;         /* dm0d_read direction */
#define                 DM0D_FORWARD            0
#define                 DM0D_BACKWARD           1
    DB_DB_NAME	    dm0d_dbname;	    /* Database name. */
    u_i2	    dm0d_offset[512];	    /* Read backward index */
    i4	    dm0d_cur_index;	    /* current record */
    JFIO            dm0d_jfio;		    /* JF context. */
};

/*}
** Name: DM0D_HEADER - Every Dump page is begins with this header.
**
** Description:
**      This structure is recorded on the beginning of each dump page.
**	The structure is used to make various checks about the consistency
**	of the dump file, and information about the version of this
**	dump format.
**
** History:
**      06-jan-1989 (Edhsu)
**          Created for Terminator.
**	03-sep-1998 consi01 bug 91957 problem INGSRV 463
**	    Changed unused dh_extra to dh_first_rec to support records
**	    spanning blocks.
*/
struct _DM0D_HEADER
{
    i4         dh_type;            /* The type of DUMP page. */
#define			DH_V1_S1	    0x10001
    DB_DB_NAME	    dh_dbname;		/* Database name. */
    i4	    dh_seq;		/* DUMP page sequence number. */
    i4	    dh_dmp_seq;		/* DUMP file sequence. */
    i4	    dh_ckp_seq;		/* Checkpoint sequence. */
    i4	    dh_offset;		/* Offset to last used byte of page. */
    i4	    dh_first_rec;	/* Offset to the first record in the 
					** dump block that begins there (ie
					** is not a continuation of a record
					** from the previous block. */
};

/*}
** Name: DM0D_CNODE - Node context.
**
** Description:
**      This structure defines the information needed to keep node
**	information during a merge of node DUMPs into standard
**      DUMPs.
**
** History:
**      06-jan-1989 (Edhsu)
**          Created for Terminator.
*/
struct _DM0D_CNODE
{
    i4         count;                  /* Number of DUMP files
                                            ** associated with this node. */
    i4         state;                  /* Current state of this entry. */
#define                 DM0D_CVALID         2
#define                 DM0D_CINVALID       4
    i4         cur_fil_seq;            /* Current file sequence number. */
    i4         last_fil_seq;           /* Last file sequence number. */

    DM0D_CTX	    *cur_dmp;		    /* Pointer to DUMP context. */
    LG_LSN	    lsn;		    /* current log sequence number for
					    ** this journal file. */
    i4	    l_record;		    /* length of journal file record */
    char	    *record;		    /* pointer to allocated record
					    ** buffer */
};

/*}
** Name: DM0D_CFILE - File context.
**
** Description:
**      This structure defines the information needed to keep file
**	information during a merge of node DUMP files into standard
**      DUMPs.
**
** History:
**      06-jan-1989 (Edhsu)
**          Created for Terminator.
*/
struct _DM0D_CFILE
{
    i4         node_id;                /* Node-id of Node that 
                                            ** created associated file. */
    i4         seq;                    /* Sequence number of file. */
    DM_FILE         filename;               /* Filename. */
    i4         l_filename;             /* Length of file name. */
};

/*}
** Name: DM0D_CS - Store File context.
**
** Description:
**      This structure defines the information needed to pass to the
**	store files routine when getting the list of files to merge.
**
** History:
**      06-jan-1989 (Edhsu)
**          Created for Terminator.
*/
struct _DM0D_CS
{
    i4         index;                  /* Number of entries. */
    i4         first;                  /* First valid sequence number.*/
    i4         last;                   /* Last valid sequence number. */
    DM0D_CFILE      *file_array;            /* Pointer to array where 
                                            ** entries can be stored. */
};

/*
**  Forward and/or External function references.
*/
FUNC_EXTERN DB_STATUS	dm0d_close(
				DM0D_CTX	**jctx,
				DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS	dm0d_create(
				i4         flag,
				char            *device,
				i4             l_device,
				i4         dmp_seq,
				i4         bksz,
				i4         blkcnt,
				i4         node_id,
				DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS	dm0d_delete(
				i4         flag,
				char            *device,
				i4             l_device,
				i4         dmp_seq,
				DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS	dm0d_open(
				i4         flag,
				char            *device,
				i4             l_device,
				DB_DB_NAME      *db_name,
				i4         bksz,
				i4         dmp_seq,
				i4         ckp_seq,
				i4         sequence,
				i4         mode,
				i4         node_id,
				i4         direction,
				DM0D_CTX        **jctx,
				DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS	dm0d_read(
				DM0D_CTX        *jctx,
				PTR             *record,
				i4         *l_record,
				DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS	dm0d_write(
				DM0D_CTX        *jctx,
				PTR             record,
				i4         l_record,
				DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS	dm0d_update(
				DM0D_CTX        *jctx,
				i4         *sequence,
				DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS	dm0d_truncate(
				DM0D_CTX        *jctx,
				DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS	dm0d_scan(
				DM0D_CTX	*jctx,
				DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS	dm0d_config_restore(
				DMP_DCB         *dcb,
				i4         flags,
				i4         ckp_seq,
				DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS	dm0d_d_config(
				DMP_DCB         *dcb,
				DM0C_CNF        *cnf,
				i4         ckp_seq,
				DB_ERROR	*dberr );

FUNC_EXTERN VOID        dm0d_filename(
                                i4             flag,
                                i4             dmp_seq,
                                i4             node_id,
                                DM_FILE             *filename);
 
FUNC_EXTERN DB_STATUS dm0d_merge(
			DM0C_CNF	*cnf,
			DB_ERROR	*dberr );

