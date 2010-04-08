/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DMSCB.H - Typedefs for DMF Sequence Object support.
**
** Description:
** 
**      This file contains the control block used for all
**	Sequence Generator DML operations initiated by QEU.
**
** History:    
**    05-Mar-2002 (jenjo02)
**	    Created.
*/

/*}
** Name:  DMS_CB - DMF sequence generator call control block.
**
** Description:
**      This typedef defines the control block required for
**      the following DMF utility operations:
**      
**	DMS_OPEN_SEQ - Open a sequence generator
**	DMS_CLOSE_SEQ- Close a sequence generator
**	DMS_NEXT_SEQ - Get next/current value of sequence
**
** History:
**	05-Mar-2002 (jenjo02)
**	    Created.
**	20-May-2003 (bonro01)
**	    Changed length to SIZE_TYPE because this header was missed by
**	    change 459881 for bug 108932.  SIZE_TYPE needed for 64-bit
**	    platforms.
**      05-Aug-2005 (hanje04)
**          Back out change 478041 as it should never have been X-integrated
**          into main in the first place.
**	6-Jul-2006 (kschendel)
**	    Comment update.
*/
typedef struct _DMS_CB
{
    PTR             q_next;
    PTR             q_prev;
    SIZE_TYPE       length;                 /* Length of control block. */
    i2		    type;                   /* Control block type. */
#define                 DMS_SEQ_CB          15
    i2              s_reserved;
    PTR             l_reserved;
    PTR             owner;
    i4         	    ascii_id;             
#define                 DMS_ASCII_ID        CV_C_CONST_MACRO('#', 'D', 'M', 'S')
    DB_ERROR        error;                  /* Common DMF error block. */
    i4         	    dms_flags_mask;         /* Operation modifier: */
#define		DMS_NEXT_VALUE	    0x0001L
#define		DMS_CURR_VALUE	    0x0002L
#define		DMS_CREATE	    0x0010L
#define		DMS_ALTER	    0x0020L
#define		DMS_DROP	    0x0040L
#define		DMS_DML		(DMS_NEXT_VALUE | DMS_CURR_VALUE)
#define		DMS_DDL 	(DMS_CREATE | DMS_ALTER | DMS_DROP)
    PTR		    dms_tran_id;	    /* Transaction ID */
    PTR 	    dms_db_id;              /* DMF database identifier. */
					/* Really an ODCB pointer, so it's the
					** same for all threads of a session.
					*/
    DM_DATA	    dms_seq_array;	    /* Sequences to operate on */
}   DMS_CB;

/*}
** Name:  DMS_SEQ_ENTRY - Description of one element of dms_seq_arrray
**
** Description:
**	This structure is used to identity the Sequences to 
**	be operated on.
**
**      The DMS_CB.dms_seq_array field contains a pointer to an array
**      containing entries of this type.
**
** History:
**    05-Mar-2002 (jenjo02)
**	    Created.
*/
typedef struct _DMS_SEQ_ENTRY
{
    DB_NAME	    seq_name;		    /* Name of the sequence */
    DB_OWN_NAME	    seq_owner;		    /* Owner of the sequence */
    DB_TAB_ID	    seq_id;		    /* Sequence id/version */
    DB_STATUS	    seq_status;		    /* Status of DDL operation */
    PTR		    seq_seq;		    /* Bound DML_SEQ */
    PTR		    seq_cseq;		    /* Txn's DML_CSEQ */
    DM_DATA	    seq_value;	    	    /* Where to return value */
}   DMS_SEQ_ENTRY;
