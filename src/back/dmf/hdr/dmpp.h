/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DMPP.H - Describes a Physical Page.
**
** Description:
**      This file describes structures and constants that pertain to a 
**	page on disk.  This describes a generic data page, other types of
**	pages must maintain the same fixed portion, but can change the
**	variable portion.
**
** History:
**      22-oct-1985 (derek)
**          Created new for Jupiter.
**	30-sep-1988 (rogerk)
**	    Added DMPP_FULLCHAIN bit in page status for Hash Overflow changes.
**      18-jun-1991 (paull)
**          Added new page definitions as part of the Multiple Page formats
**          development.
**	29-apr-1991 (Derek)
**	    Added new page types constants.
**	08-jul-1992 (mikem)
**	    removed "$Log-for RCS$" comment.
**	07-oct-1992 (jnash)
**	    6.5 Recovery Project.
**	      - Substitute page_checksum for page_version.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**      06-may-1996 (thaju02)
**          New Page Format Project.
**            - Added DMPP_V2_PAGE structure.
**            - Renamed DMPP_PAGE to DMPP_V1_PAGE.
**            - Created DMPP_PAGE union of page formats.
**	      - Defined macros to access page header fields.
**	      - Created DMPP_TUPLE_HDR structure.
**	20-may-1996 (ramra01)
**	    Created DMPP_TUPLE_INFO structure     
**      03-june-1996 (stial01)
**          Added DMPP_INIT_TUPLE_INFO_MACRO
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          DMPP_TUPLE_HDR: Rename lk_owner_id -> tran_id
**          DMPP_TUPLE_INFO: Add tran_id, removed unused tup_flags
**          Removed unecessary DMPP_INIT_TUPLE_INFO_MACRO
**      27-feb-1997 (stial01)
**          Added DMPP_CLEAN
**	06-jun-1998 (nanpr01)
**	    Converted the macros to v1 and v2 macros so that from selective
**	    places they can be called directly to save the pagesize comparison.
**	01-dec-1998 (nanpr01)
**	    Get rid of DMPP_TUPLE_INFO structure. Use DMPP_TUPLE_HDR instead.
**      04-Feb-1999 (hanal04)
**          Added DMPP_REC_PAGE_LOG_ADDR_MACRO to allow a page lsn to be
**          recorded in a local LG_LSN structure. b94849.
**	10-may-1999 (walro03)
**	    Remove obsolete version string i39_vme.
**	16-aug-1999 (nanpr01)
**	    Fix up the typo in macro.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-feb-2001 (stial01)
**          Define DMPP_TUPLE_HDR as a union (Variable Page Type SIR 102955)
**          Also added macros to extract info from tuple header
**      09-mar-2001 (stial01)
**          Changes for Variable Page Type SIR 102955
**      28-may-2003 (stial01)
**          Cleaned up DMPP_VPT_CLEAR_HDR_MACRO,DMPP_VPT_DELETE_CLEAR_HDR_MACRO
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**	13-Apr-2006 (jenjo02)
**	    Add DMPP_CLUSTERED page status bit.
**      07-nov-2007 (stial01)
**          Added DMPP_TPERPAGE_MACRO
**      14-oct-2009 (stial01)
**          Move btree page types into DMPP_PAGE, update page macros 
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add DMPP_VPT_ADDR_TRAN_ID_MACRO, PAGE_STAT define
**	    DMPP_CREAD page_stat flag.
*/

/*
**  Defines of other constants.
*/
#define                 DMPP_DIRECT     0x0001L
#define                 DMPP_PRIM       0x0002L /* Primary page */
#define                 DMPP_OVFL       0x0004L /* Overflow page */
#define                 DMPP_FULLCHAIN  0x0008L /* Ovfl chain is full */
#define                 DMPP_CHKSUM     0x0010L /* Page is checksum'd */
#define                 DMPP_DATA       0x0020L /* Data page */
#define                 DMPP_LEAF       0x0040L /* Btree leaf page */
#define                 DMPP_FREE       0x0080L /* Btree unused page */
#define                 DMPP_MODIFY     0x0100L /* Page is dirty */
#define                 DMPP_PATCHED    0x0200L /* Patch Table chgd a data page-
                                                ** it will be necessary to throw
                                                ** away all tuples on this page
                                                ** if file still not readable.*/
#define                 DMPP_FHDR       0x0400L /* Free space header page. */
#define                 DMPP_FMAP       0x0800L /* Free space map page. */
#define                 DMPP_CHAIN      0x1000L /* Btree page on ovfl chain */
#define                 DMPP_INDEX      0x2000L /* Btree index page */
#define                 DMPP_ASSOC      0x4000L /* Btree associated data page */
#define                 DMPP_SPRIG      0x8000L /* Btree parent of leaf page */

/*
** The following bits are valid only for V2 page_stat which is defined
** as DM_PAGESTAT (u_i4)
*/
#define                 DMPP_CLEAN     0x10000L /* Page needs to be cleaned */
#define                 DMPP_CLUSTERED 0x20000L /* Clustered Btree Leaf */
#define                 DMPP_CREAD     0x40000L /* CR page for MVCC */

/* String values of above bits: */
#define PAGE_STAT "\
DIRECT,PRIM,OVFL,FULLCHAIN,CHKSUM,DATA,LEAF,FREE,MODIFY,PATCHED,\
FHDR,FMAP,CHAIN,INDEX,ASSOC,SPRIG,CLEAN,CLUSTERED,CREAD"


/*}
** Name: DMPP_V1_PAGE - The layout of a Physical Data Page (version 1).
**
** Description:
**      This defines the layout of the generic portion of a data page.
**
** History:
**     22-oct-1985 (derek)
**          Created new for Jupiter.
**	30-sep-1988 (rogerk)
**	    Added DMPP_FULLCHAIN bit in page status.  This is set on primary
**	    HASH pages to indicate that there is no free space in the overflow
**	    chain.  It is used for performance reasons - no part of the system
**	    should depend on the bit to be set or not set in order to work
**	    correctly.
**	08-Aug-1989 (teg)
**	    Added DMPP_PATCHED page_stat bitmap value for the Table Patch 
**	    operation.
**     18-jun-91 (paull)
**         Converted to header plus hidden contents for MPF development.
**	02-Feb-1990 (Derek)
**	    Added DMPP_BITMAP to page_stat to be used by the BTREE free page
**	    bitmap pages.
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedef to quiesce the ANSI C 
**	    compiler warnings.
**	07-oct-1992 (jnash)
**	    6.5 Recovery Project.
**	      - Add DMPP_CHKSUM indicating that page is checksum'd
**	      - Substitute page_checksum for page_version.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**      06-may-1996 (thaju02)
**          New Page Format Project.
**            - Changed structure name from DMPP_PAGE to DMPP_V1_PAGE.
**            - Typedef'd structure.
*/
typedef struct _DMPP_V1_PAGE
{
    DM_PAGENO       page_main;          /* The next main page. */
    DM_PAGENO  	    page_ovfl;		/* The next overflow page. */
    DM_PAGENO  	    page_page;		/* The page # of this page. */
    u_i2	    page_stat;		/* Page status bits. */
    u_i2            page_seq_number;    /* The sequence number associated with
                                        ** a deferred update cursor within a 
                                        ** transaction. */
    DB_TRAN_ID	    page_tran_id;	/* The transaction id that last changed
					** this page.
					*/
    LG_LSN	    page_log_addr;	/* The log sequence number of the log
					** record which describes the most
					** recent update made to this page. This
					** log sequence number is used by our
					** recovery protocols to determine
					** whether a particular log record's
					** changes have already been applied
					** to this page.
					*/
    u_i2	    page_checksum;	/* Checksum of all bytes on page */
    u_i2	    page_next_line;	/* Next available line number 
                                        ** in table.  Also
					** the page_line_tab[page_next_line] 
                                        ** contains
					** the number of free bytes 
                                        ** left on the page.
					*/
#define                 DMPP_V1_PAGE_OVERHEAD    36

    char	    page_contents[DM_PG_SIZE   - DMPP_V1_PAGE_OVERHEAD];
					/* The ammount of space on the
					** page for line tables and tuples.
					*/
} DMPP_V1_PAGE;

/*
** Name: DMPP_V2_PAGE - The layout of a Physical Data Page (version 2).
**
** Description:
**      This defines the layout of the generic portion of a data page.
**
** History:
**      06-may-1996 (thaju02)
**          Created for New Page Format Project.
*/
typedef struct _DMPP_V2_PAGE
{
    DM_PAGESTAT    page_stat;           /* Page status bits. */
    u_i4           page_jnl_filseq;     /* Journal file sequence */
    DM_PAGENO      page_main;           /* The next main page. */
    i4	           page_bufid;          /* LG buffer containing page_lga */
    DM_PAGENO      page_ovfl;           /* The next overflow page. */
    u_i4           page_spare2;         /* 64 bit tid project */
    DM_PAGENO      page_page;           /* The page # of this page. */
    DM_PGSZTYPE    page_sztype;		/* Page size + type */
    DM_PGSEQNO     page_seq_number;     /* The sequence number associated with
                                        ** a deferred update cursor within a
                                        ** transaction. */
    u_i2           page_lg_id;          /* LG id of last-changing tran_id */
    DM_PGCHKSUM    page_checksum;       /* Checksum of all bytes on page */
    DB_TRAN_ID     page_tran_id;        /* The transaction id that last changed
                                        ** this page.
                                        */
    LG_LSN         page_log_addr;       /* The log sequence number of the log
                                        ** record which describes the most
                                        ** recent update made to this page. This
                                        ** log sequence number is used by our
                                        ** recovery protocols to determine
                                        ** whether a particular log record's
                                        ** changes have already been applied
                                        ** to this page.
                                        */
    u_i2           page_next_line;      /* Next available line number
                                        ** in table.  Also
                                        ** the page_line_tab[page_next_line]
                                        ** contains
                                        ** the number of free bytes
                                        ** left on the page.
                                        */
    u_i2	   page_spare3;		/* Unused */
    u_i4	   page_jnl_block;	/* Journal block of last page update */
    LG_LA	   page_lga;		/* Physical log address of last page update */
#define                 DMPP_V2_PAGE_OVERHEAD    76
 
#define                 DMPP_FREEBYTES         0
 
    char           page_contents[DM_PG_SIZE   - DMPP_V2_PAGE_OVERHEAD];
                                        /* The ammount of space on the
                                        ** page for line tables and tuples.
                                        */
} DMPP_V2_PAGE;


/*}
** Name: DM1B_V1_INDEX - BTREE index page header layout (version 1).
**
** Description:
**      This structure contains the layout of the BTREE index
**      page.  The first four fields are common to those of 
**      the DMPP_PAGE structure.
**
**	Not visible in this structure are the actual (tid,key) or
**	(pageno,key) pairs, which come after the sequence table.
**	Also, the first two (tid,key) slots are reserved to encode
**	the range of key values ending up in the page.
**	The tid part tells if the endpoint of the range is infinity.
**	If not, the key part contains the discriminator for that end.
**
** History:
**     17-0ct-85 (jennifer)
**          Documented to Jupiter.
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedef to quiesce the ANSI C 
**	    compiler warnings.
**	07-oct-1992 (jnash)
**	    6.5 Recovery Project
**	      - Substitute page_checksum for page_version.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project (phase 2):
**	      - Added btree index page field "bt_split_lsn".
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	06-mar-1996 (stial01 for bryanp)
**	    This structure just describes the page header. Btree pages are
**		variable length now, so don't try to use sizeof(DM1B_INDEX)
**		to find out the page size --  use tcb_rel.relpgsize instead.
**	06-may-1996 (thaju02)
**	    New Page Format Project.
**	      - Renamed structure from DM1B_INDEX to DM1B_V1_INDEX.
**	      - Typedef'd structure.
*/
typedef struct _DM1B_V1_INDEX
{
    DM_PAGENO	page_main;		/* Next page pointer. */
    DM_PAGENO	page_ovfl;		/* Next overflow page. */
    DM_PAGENO	page_page;		/* Page number of this page */
    u_i2	page_stat;		/* Page status. */
    u_i2	page_seq_number;	/* Identifies the Update sequence
					** of a Transancation.  Each deferred
					** cursor gets a new update sequence.
					*/
    DB_TRAN_ID	page_tran_id;		/* Transaction id.*/
    LG_LSN	page_log_addr;		/* The log sequence number of the log
					** record which describes the most
					** recent update made to this page. This
					** log sequence number is used by our
					** recovery protocols to determine
					** whether a particular log record's
					** changes have already been applied
					** to this page.
					*/
    u_i2	page_checksum;		/* Checksum of all bytes on page */
    u_i2	bt_padalign;
    i4		bt_nextpage;		/* Forward sideways pointer. */
    LG_LSN	bt_split_lsn;		/* Log Address of the most recent split
					** operation which affected the page */
    i2		bt_tid_size;		/* Size of tids */
    char	bt_unused[26];		/* For possible future use. */
    DM_PAGENO	bt_data;		/* Associated Data Page number. */
    i4		bt_kids;		/* Number of children or tids on page */
    DM_V1_LINEID  bt_sequence[2];       /* Array of pointers to (key,tid) pairs.
					** This array is set to size 2 to match 
                                        ** what "sizeof" gives as the size 
                                        ** of the structure (it rounds 
                                        ** up to a word-aligned size).
                                        ** Discovered on Pyramid by Mike 
                                        ** Berrow on 08-03-85. */
   char		bt_pagedata[4];
} DM1B_V1_INDEX;

/*
** Name : DM1B_V2_INDEX - BTREE index page header layout (version 2).
**
** Description :
**
** History :
** 	06-may-1996 (thaju02)
**	    Created.  New Page Format Project.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add page_lg_id, page_jnl_filseq, page_jnl_block,
**	    page_lga.
*/
typedef struct _DM1B_V2_INDEX
{
    DM_PAGESTAT page_stat;              /* Page status. */
    u_i4        page_spare1;            /* 64 bit tid project */
    DM_PAGENO   page_main;              /* Next page pointer. */
    i4	        page_bufid;             /* LG buffer containing page_lga */
    DM_PAGENO   page_ovfl;              /* Next overflow page. */
    u_i4        page_spare3;            /* 64 bit tid project */
    DM_PAGENO   page_page;              /* Page number of this page */
    DM_PGSZTYPE page_sztype;            /* Page size + type */
    DM_PGSEQNO  page_seq_number;        /* Identifies the Update sequence
                                        ** of a Transancation.  Each deferred
                                        ** cursor gets a new update sequence.
                                        */
    u_i2        page_spare4;            /* spare for i4 checksum */
    DM_PGCHKSUM page_checksum;          /* Checksum of all bytes on page */
    DB_TRAN_ID  page_tran_id;           /* Transaction id.*/
    LG_LSN      page_log_addr;          /* The log sequence number of the log
                                        ** record which describes the most
                                        ** recent update made to this page. This
                                        ** log sequence number is used by our
                                        ** recovery protocols to determine
                                        ** whether a particular log record's
                                        ** changes have already been applied
                                        ** to this page.
                                        */
    u_i4        page_spare5;            /* 64 bit tid project */
    DM_PAGENO   bt_nextpage;            /* Forward sideways pointer. */
    LG_LSN      bt_split_lsn;           /* Log Address of the most recent split
                                        ** operation which affected the page */
    u_i4        bt_clean_count;         /* The number of clean-ups done to
					** this page since page_log_addr
					** was updated 
					*/
    i4          bt_attcnt;              /* Attribute information on page */
    i2		bt_tid_size;		/* Size of tids */
    u_i2        page_lg_id;             /* LG id of last updating tran_id */
    u_i4        page_jnl_block;		/* Journal block of last update */
    LG_LA       page_lga;		/* Physical log addr of last pg update*/
    u_i4        page_jnl_filseq;	/* Journal file sequence */
					/* previously reserved for 64 bit tid */
    DM_PAGENO   bt_data;                /* Associated Data Page number. */
    i4          bt_kids;                /* Number of children or tids on page */
    DM_V2_LINEID  bt_sequence[2];       /* Array of pointers to (key,tid) pairs.
                                        ** This array is set to size 2 to match
                                        ** what "sizeof" gives as the size
                                        ** of the structure (it rounds
                                        ** up to a word-aligned size).
                                        ** Discovered on Pyramid by Mike
                                        ** Berrow on 08-03-85. */
   char         bt_pagedata[4];
} DM1B_V2_INDEX;


/*
** Name: DMPP_PAGE - union of page format structures.
**
** Description:
**      This defines the union of the data page format structures.
**
** History:
**      06-may-1996 (thaju02)
**          Created for New Page Format Project.
*/
union _DMPP_PAGE
{
    DMPP_V1_PAGE        dmpp_v1_page;
    DMPP_V2_PAGE        dmpp_v2_page;
    DM1B_V1_INDEX       dmpp_v1_index;
    DM1B_V2_INDEX       dmpp_v2_index;
};


typedef struct _DMPP_PAGETYPE
{
    i4  dmpp_tuphdr_size;	/* Tuple header size */
    i4  dmpp_lowtran_offset;	/* Offset of low tranid in tup hdr */
    i4	dmpp_hightran_offset;	/* Offset of high tranid in tup hdr */
    i4  dmpp_seqnum_offset;	/* Offset of deferred cursor seq# in tup hdr */
    i4  dmpp_vernum_offset;	/* Offset of alter table version in tup hdr */
    i4  dmpp_seghdr_offset;	/* Offset of next segment information */
    i4  dmpp_lgid_offset;	/* Offset of lg_id in tup hdr */
    i1  dmpp_row_locking;	/* True if page type supports row locking */
    i1  dmpp_has_versions;	/* True if page type supports alter table */
    i1  dmpp_has_seghdr;	/* True if page type supports rows span pages*/
    i1  dmpp_has_lgid;		/* True if page type supports lg_id */
    i1  dmpp_has_rowseq;	/* True if deferred cursor seq# in row hdr */
} DMPP_PAGETYPE;

struct _DMPP_SEG_HDR
{
    DM_PAGENO	seg_next;		/* page # for next segment if any */
    i4		seg_len;		/* length of this segment FIX ME do we need this*/
};

/*
** This V1 header is not really a 'header'. It overlays deleted tuples
** on V1 pages when PHYSICAL page locking (etabs, core catalogs
*/
typedef struct _DMPP_TUPLE_HDR_V1
{
    char        low_tran[sizeof (u_i4)];
} DMPP_TUPLE_HDR_V1;

/* This is original V2 tuple header format */
typedef struct _DMPP_TUPLE_HDR_V2
{
    char	high_tran[sizeof (u_i4)];
    char	low_tran[sizeof (u_i4)];
    char        lg_id[sizeof (u_i2)];      /* lg_id associated with low_tran */
    char        spare1[sizeof (u_i2)];     /* future expansion */
    char        sequence_num[sizeof (u_i4)];
    char        ver_number[sizeof (u_i2)]; /* alter table row version number */
    char        spare2[sizeof (u_i2)];      /* spare to align */
    char        spare3[sizeof (u_i4)];     /* future expansion */
} DMPP_TUPLE_HDR_V2;

/* This header can be used on tables enabled for row locking & alter table*/
typedef struct _DMPP_TUPLE_HDR_V3
{
    char        low_tran[sizeof (u_i4)];
    char 	ver_number[sizeof (u_i2)]; /* alter table row version number */
} DMPP_TUPLE_HDR_V3;

/* This header can be used on indexes enabled for row locking */
typedef struct _DMPP_TUPLE_HDR_V4
{
    char        low_tran[sizeof (u_i4)];
} DMPP_TUPLE_HDR_V4;

/*
** This header can be used on tables enabled for row locking & alter table
** and rows spanning pages
*/
typedef struct _DMPP_TUPLE_HDR_V5
{
    char        low_tran[sizeof (u_i4)];
    char 	ver_number[sizeof (u_i2)]; /* alter table row version number */
    char	seg_hdr[sizeof (DMPP_SEG_HDR)]; /* next segment information */
} DMPP_TUPLE_HDR_V5;

/*
** This header can be used on tables and indexes enabled for row locking    
** and alter table.
*/
typedef struct _DMPP_TUPLE_HDR_V6
{
    char        low_tran[sizeof (u_i4)];
    char        sequence_num[sizeof (u_i4)];
    char	lg_id[sizeof (u_i2)];	   /* lg_id associated with low_tran */
    char 	ver_number[sizeof (u_i2)]; /* alter table row version number */
} DMPP_TUPLE_HDR_V6;

typedef struct _DMPP_TUPLE_HDR_V7
{
    char        low_tran[sizeof (u_i4)];
    char        sequence_num[sizeof (u_i4)];
    char	lg_id[sizeof (u_i2)];	   /* lg_id associated with low_tran */
    char 	ver_number[sizeof (u_i2)]; /* alter table row version number */
    char	seg_hdr[sizeof (DMPP_SEG_HDR)]; /* next segment information */
} DMPP_TUPLE_HDR_V7;
/*
** Name: DMPP_TUPLE_HDR - tuple header prefixed to every row on page.
** 
** Note there are some assumptions in the code that new row headers 
** on V2 pages should never be bigger thean DMPP_TUPLE_HDR_V2
** (see dm2u_maxreclen)
**
** History:
**	06-may-1996 (thaju02)
**	    Created for New Page Format Project.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Rename lk_owner_id -> tran_id
*/
union _DMPP_TUPLE_HDR
{
    DMPP_TUPLE_HDR_V1    hdr1; /* page type V1 tuple header (overlays tup) */
    DMPP_TUPLE_HDR_V2    hdr2; /* page type V2 tuple header */
    DMPP_TUPLE_HDR_V3    hdr3; /* page type V3 tuple header */
    DMPP_TUPLE_HDR_V4    hdr4; /* page type V4 tuple header */
    DMPP_TUPLE_HDR_V5    hdr5; /* page type V5 tuple header */
    DMPP_TUPLE_HDR_V6    hdr6; /* page type V6 tuple header */
    DMPP_TUPLE_HDR_V7    hdr7; /* page type V7 tuple header */
};

#define DMPP_VPT_CLEAR_HDR_MACRO(page_type, hdr)		\
{								\
    if (page_type == DM_PG_V3)				\
    {								\
	((DMPP_TUPLE_HDR_V3 *)hdr)->low_tran[0] = '\0';		\
	((DMPP_TUPLE_HDR_V3 *)hdr)->low_tran[1] = '\0';		\
	((DMPP_TUPLE_HDR_V3 *)hdr)->low_tran[2] = '\0';		\
	((DMPP_TUPLE_HDR_V3 *)hdr)->low_tran[3] = '\0';		\
	((DMPP_TUPLE_HDR_V3 *)hdr)->ver_number[0] = '\0';	\
	((DMPP_TUPLE_HDR_V3 *)hdr)->ver_number[1] = '\0';	\
    }								\
    else if (page_type == DM_PG_V4)				\
    {								\
	((DMPP_TUPLE_HDR_V4 *)hdr)->low_tran[0] = '\0';		\
	((DMPP_TUPLE_HDR_V4 *)hdr)->low_tran[1] = '\0';		\
	((DMPP_TUPLE_HDR_V4 *)hdr)->low_tran[2] = '\0';		\
	((DMPP_TUPLE_HDR_V4 *)hdr)->low_tran[3] = '\0';		\
    }								\
    else if (page_type == DM_PG_V2)				\
    {								\
	MEfill(sizeof(DMPP_TUPLE_HDR_V2), '\0', (PTR)(hdr));	\
    }								\
    else if (page_type == DM_PG_V5)				\
    {								\
	MEfill(sizeof(DMPP_TUPLE_HDR_V5), '\0', (PTR)(hdr));	\
    }								\
    else if (page_type == DM_PG_V6)				\
    {								\
	((DMPP_TUPLE_HDR_V6 *)hdr)->low_tran[0] = '\0';		\
	((DMPP_TUPLE_HDR_V6 *)hdr)->low_tran[1] = '\0';		\
	((DMPP_TUPLE_HDR_V6 *)hdr)->low_tran[2] = '\0';		\
	((DMPP_TUPLE_HDR_V6 *)hdr)->low_tran[3] = '\0';		\
	((DMPP_TUPLE_HDR_V6 *)hdr)->lg_id[0] = '\0';		\
	((DMPP_TUPLE_HDR_V6 *)hdr)->lg_id[1] = '\0';		\
	((DMPP_TUPLE_HDR_V6 *)hdr)->ver_number[0] = '\0';	\
	((DMPP_TUPLE_HDR_V6 *)hdr)->ver_number[1] = '\0';	\
    }								\
    else if (page_type == DM_PG_V7)				\
    {								\
	MEfill(sizeof(DMPP_TUPLE_HDR_V7), '\0', (PTR)(hdr));	\
    }								\
}

#define DMPP_VPT_DELETE_CLEAR_HDR_MACRO(page_type, hdr)		\
{								\
	DMPP_VPT_CLEAR_HDR_MACRO(page_type, hdr)		\
}

/*
** Macros used to access page fields.
*/

#define DMPP_V1_GET_PAGE_STAT_MACRO(pgptr) 	 \
        	(pgptr)->dmpp_v1_page.page_stat
 
#define DMPP_V2_GET_PAGE_STAT_MACRO(pgptr) 	 \
		(pgptr)->dmpp_v2_page.page_stat 
 
#define DMPP_VPT_GET_PAGE_STAT_MACRO(page_type,pgptr) \
        (page_type != DM_COMPAT_PGTYPE ? 	 \
	 DMPP_V2_GET_PAGE_STAT_MACRO(pgptr) :	 \
	 DMPP_V1_GET_PAGE_STAT_MACRO(pgptr))
 
#define DMPP_V1_SET_PAGE_STAT_MACRO(pgptr, value) 	 \
    {						 	 \
	(pgptr)->dmpp_v1_page.page_stat |= (u_i2)value;	\
    }
 
#define DMPP_V2_SET_PAGE_STAT_MACRO(pgptr, value) 	 \
    {							 \
        (pgptr)->dmpp_v2_page.page_stat |= value;	 \
    }
 
#define DMPP_VPT_SET_PAGE_STAT_MACRO(page_type, pgptr, value) \
    {							 \
        if (page_type != DM_COMPAT_PGTYPE)		 \
	    DMPP_V2_SET_PAGE_STAT_MACRO(pgptr, value)	 \
        else 						 \
	    DMPP_V1_SET_PAGE_STAT_MACRO(pgptr, value)	 \
    }
 
#define DMPP_V1_INIT_PAGE_STAT_MACRO(pgptr, value)		\
    {								\
	(pgptr)->dmpp_v1_page.page_stat = (u_i2)value;	\
    }
 
#define DMPP_V2_INIT_PAGE_STAT_MACRO(pgptr, value)		\
    {								\
        (pgptr)->dmpp_v2_page.page_stat = value;		\
    }
 
#define DMPP_VPT_INIT_PAGE_STAT_MACRO(page_type, pgptr, value)	\
    {								\
        if (page_type != DM_COMPAT_PGTYPE)		 	\
	    DMPP_V2_INIT_PAGE_STAT_MACRO(pgptr, value)		\
        else							\
	    DMPP_V1_INIT_PAGE_STAT_MACRO(pgptr, value)		\
    }
 
#define DMPP_V1_CLR_PAGE_STAT_MACRO(pgptr, value)		\
    {								\
	(pgptr)->dmpp_v1_page.page_stat &= ~(u_i2)value;	\
    }

#define DMPP_V2_CLR_PAGE_STAT_MACRO(pgptr, value)		\
    {								\
        (pgptr)->dmpp_v2_page.page_stat &= ~value;		\
    }

#define DMPP_VPT_CLR_PAGE_STAT_MACRO(page_type, pgptr, value)	\
    {								\
        if (page_type != DM_COMPAT_PGTYPE)			\
	    DMPP_V2_CLR_PAGE_STAT_MACRO(pgptr, value)		\
        else 							\
	    DMPP_V1_CLR_PAGE_STAT_MACRO(pgptr, value)		\
    }

#define DMPP_V1_GET_PAGE_MAIN_MACRO(pgptr) 			\
        (pgptr)->dmpp_v1_page.page_main
 
#define DMPP_V2_GET_PAGE_MAIN_MACRO(pgptr) 			\
	(pgptr)->dmpp_v2_page.page_main
 
#define DMPP_VPT_GET_PAGE_MAIN_MACRO(page_type, pgptr) 		\
        (page_type != DM_COMPAT_PGTYPE ? 	  		\
	 DMPP_V2_GET_PAGE_MAIN_MACRO(pgptr) :			\
	 DMPP_V1_GET_PAGE_MAIN_MACRO(pgptr))
 
#define DMPP_V1_SET_PAGE_MAIN_MACRO(pgptr, value) 		\
    {							 	\
	(pgptr)->dmpp_v1_page.page_main = value;	 	\
    }

#define DMPP_V2_SET_PAGE_MAIN_MACRO(pgptr, value) 		\
    {							 	\
        (pgptr)->dmpp_v2_page.page_main = value;	 	\
    }

#define DMPP_VPT_SET_PAGE_MAIN_MACRO(page_type, pgptr, value) 	\
    {							 	\
        if (page_type != DM_COMPAT_PGTYPE) 		 	\
	    DMPP_V2_SET_PAGE_MAIN_MACRO(pgptr, value)		\
        else 						 	\
	    DMPP_V1_SET_PAGE_MAIN_MACRO(pgptr, value)		\
    }

/*
**  History:
**	06-may-1996 (thaju02)
**	    Prematurely created, for possible future
**	    use (64 bit tid project).
*/
#define DMPP_VPT_SIZEOF_PAGE_MAIN_MACRO(page_type, pgptr)	\
        (page_type != DM_COMPAT_PGTYPE ? 	  		\
	(i4)(sizeof((pgptr)->dmpp_v2_page.page_main)) :	\
        (i4)(sizeof((pgptr)->dmpp_v1_page.page_main)))
 
#define DMPP_VPT_ADDR_PAGE_MAIN_MACRO(page_type, pgptr)		\
        (page_type != DM_COMPAT_PGTYPE ? 	  		\
	&((pgptr)->dmpp_v2_page.page_main) :			\
        &((pgptr)->dmpp_v1_page.page_main))
 
#define DMPP_VPT_GET_PAGE_OVFL_MACRO(page_type, pgptr) 		\
        (page_type != DM_COMPAT_PGTYPE ? 			\
	(pgptr)->dmpp_v2_page.page_ovfl : 			\
        (pgptr)->dmpp_v1_page.page_ovfl)
 
#define DMPP_VPT_SET_PAGE_OVFL_MACRO(page_type, pgptr, value) \
     {								\
        if (page_type != DM_COMPAT_PGTYPE) 			\
          (pgptr)->dmpp_v2_page.page_ovfl = value; 		\
        else 							\
	  (pgptr)->dmpp_v1_page.page_ovfl = value;		\
     }
 
#define DMPP_VPT_ADDR_PAGE_OVFL_MACRO(page_type, pgptr)		\
        (page_type != DM_COMPAT_PGTYPE ? 			\
	&(pgptr)->dmpp_v2_page.page_ovfl :			\
        &(pgptr)->dmpp_v1_page.page_ovfl)
 
#define DMPP_V1_GET_PAGE_PAGE_MACRO(pgptr) 			\
        	(pgptr)->dmpp_v1_page.page_page
 
#define DMPP_V2_GET_PAGE_PAGE_MACRO(pgptr) 			\
		(pgptr)->dmpp_v2_page.page_page 
 
#define DMPP_VPT_GET_PAGE_PAGE_MACRO(page_type, pgptr) 		\
        (page_type != DM_COMPAT_PGTYPE ? 			\
	 DMPP_V2_GET_PAGE_PAGE_MACRO(pgptr) :			\
	 DMPP_V1_GET_PAGE_PAGE_MACRO(pgptr)) 

#define DMPP_V1_SET_PAGE_PAGE_MACRO(pgptr, value)		\
     {								\
	(pgptr)->dmpp_v1_page.page_page = value;		\
     }
 
#define DMPP_V2_SET_PAGE_PAGE_MACRO(pgptr, value)		\
     {								\
        (pgptr)->dmpp_v2_page.page_page = value; 		\
     }
 
#define DMPP_VPT_SET_PAGE_PAGE_MACRO(page_type, pgptr, value)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE) 			\
	    DMPP_V2_SET_PAGE_PAGE_MACRO(pgptr, value)		\
        else 							\
	    DMPP_V1_SET_PAGE_PAGE_MACRO(pgptr, value)		\
     }
 
#define DMPP_VPT_ADDR_PAGE_PAGE_MACRO(page_type, pgptr)      	\
        (page_type != DM_COMPAT_PGTYPE ?                  	\
        &(pgptr)->dmpp_v2_page.page_page :			\
        &(pgptr)->dmpp_v1_page.page_page)
 
#define DMPP_V2_GET_PAGE_SZTYPE_MACRO(pgptr)			\
	(pgptr)->dmpp_v2_page.page_sztype

#define DMPP_V2_SET_PAGE_SZTYPE_MACRO(page_type, page_size, pgptr)\
     {								\
        (pgptr)->dmpp_v2_page.page_sztype = page_size + page_type;\
     } 
 
#define DMPP_VPT_SET_PAGE_SZTYPE_MACRO(page_type, page_size, pgptr) \
     {								\
        if (page_type != DM_COMPAT_PGTYPE) 			\
	    DMPP_V2_SET_PAGE_SZTYPE_MACRO(page_type, page_size, pgptr)\
     } 
 
#define DMPP_V1_GET_PAGE_SEQNO_MACRO(pgptr) 			\
        (pgptr)->dmpp_v1_page.page_seq_number
 
#define DMPP_V2_GET_PAGE_SEQNO_MACRO(pgptr) 			\
        (pgptr)->dmpp_v2_page.page_seq_number 
 
#define DMPP_VPT_GET_PAGE_SEQNO_MACRO(page_type, pgptr) 		\
        (page_type != DM_COMPAT_PGTYPE ? 			\
	 DMPP_V2_GET_PAGE_SEQNO_MACRO(pgptr) :			\
	 DMPP_V1_GET_PAGE_SEQNO_MACRO(pgptr)) 

#define DMPP_V1_SET_PAGE_SEQNO_MACRO(pgptr, value) 		\
     {								\
	 (pgptr)->dmpp_v1_page.page_seq_number = value;	\
     }
 
#define DMPP_V2_SET_PAGE_SEQNO_MACRO(pgptr, value) 		\
     {								\
         (pgptr)->dmpp_v2_page.page_seq_number = value; 	\
     }
 
#define DMPP_VPT_SET_PAGE_SEQNO_MACRO(page_type, pgptr, value) 	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE) 			\
	    DMPP_V2_SET_PAGE_SEQNO_MACRO(pgptr, value)		\
        else 							\
	    DMPP_V1_SET_PAGE_SEQNO_MACRO(pgptr, value)		\
     }
 
#define DMPP_VPT_GET_PAGE_CHKSUM_MACRO(page_type, pgptr) 	\
        (page_type != DM_COMPAT_PGTYPE ? 			\
        (pgptr)->dmpp_v2_page.page_checksum : 		\
        (pgptr)->dmpp_v1_page.page_checksum)

#define DMPP_VPT_SET_PAGE_CHKSUM_MACRO(page_type, pgptr, value) 	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE) 			\
          (pgptr)->dmpp_v2_page.page_checksum = value; 	\
        else 							\
	  (pgptr)->dmpp_v1_page.page_checksum = value;	\
     }
 
#define DMPP_VPT_GET_PAGE_TRAN_ID_MACRO(page_type, pgptr)	\
        (page_type != DM_COMPAT_PGTYPE ? 			\
        (pgptr)->dmpp_v2_page.page_tran_id :			\
        (pgptr)->dmpp_v1_page.page_tran_id)
 
#define DMPP_V1_SET_PAGE_TRAN_ID_MACRO(pgptr, tran_id) 		\
     {								\
        STRUCT_ASSIGN_MACRO(tran_id, (pgptr)->dmpp_v1_page.page_tran_id); \
     }
 
#define DMPP_V2_SET_PAGE_TRAN_ID_MACRO(pgptr, tran_id) 		\
     {								\
        STRUCT_ASSIGN_MACRO(tran_id, (pgptr)->dmpp_v2_page.page_tran_id); \
     }
 
#define DMPP_VPT_SET_PAGE_TRAN_ID_MACRO(page_type, pgptr, tran_id)\
     {								\
        if (page_type != DM_COMPAT_PGTYPE) 			\
	    DMPP_V2_SET_PAGE_TRAN_ID_MACRO(pgptr, tran_id)	\
        else    						\
	    DMPP_V1_SET_PAGE_TRAN_ID_MACRO(pgptr, tran_id)	\
     }
 
#define DMPP_V1_GET_TRAN_ID_LOW_MACRO(pgptr)			\
	(pgptr)->dmpp_v1_page.page_tran_id.db_low_tran

#define DMPP_V2_GET_TRAN_ID_LOW_MACRO(pgptr)			\
	(pgptr)->dmpp_v2_page.page_tran_id.db_low_tran 

#define DMPP_VPT_GET_TRAN_ID_LOW_MACRO(page_type, pgptr)		\
	(page_type != DM_COMPAT_PGTYPE ?				\
	 DMPP_V2_GET_TRAN_ID_LOW_MACRO(pgptr) :			\
	 DMPP_V1_GET_TRAN_ID_LOW_MACRO(pgptr))

#define DMPP_V1_GET_TRAN_ID_HIGH_MACRO(pgptr)			\
	(pgptr)->dmpp_v1_page.page_tran_id.db_high_tran

#define DMPP_V2_GET_TRAN_ID_HIGH_MACRO(pgptr)			\
	(pgptr)->dmpp_v2_page.page_tran_id.db_high_tran 

#define DMPP_VPT_GET_TRAN_ID_HIGH_MACRO(page_type, pgptr)	\
	(page_type != DM_COMPAT_PGTYPE ?				\
	 DMPP_V2_GET_TRAN_ID_HIGH_MACRO(pgptr) :		\
	 DMPP_V1_GET_TRAN_ID_HIGH_MACRO(pgptr))

#define DMPP_VPT_GET_PAGE_LOG_ADDR_MACRO(page_type, pgptr, log_addr) \
        (page_type != DM_COMPAT_PGTYPE ? 			\
	STRUCT_ASSIGN_MACRO((pgptr)->dmpp_v2_page.page_log_addr,log_addr): \
        STRUCT_ASSIGN_MACRO((pgptr)->dmpp_v1_page.page_log_addr,log_addr))
 
#define DMPP_VPT_SET_PAGE_LOG_ADDR_MACRO(page_type, pgptr, log_addr) \
     {								\
        if (page_type != DM_COMPAT_PGTYPE) 			\
          STRUCT_ASSIGN_MACRO(log_addr,(pgptr)->dmpp_v2_page.page_log_addr);\
        else    						\
          STRUCT_ASSIGN_MACRO(log_addr,(pgptr)->dmpp_v1_page.page_log_addr);\
     } 
#define DMPP_VPT_REC_PAGE_LOG_ADDR_MACRO(page_type, log_addr, pgptr) \
     {                                                          \
        if (page_type != DM_COMPAT_PGTYPE)                       \
          STRUCT_ASSIGN_MACRO((pgptr)->dmpp_v2_page.page_log_addr,log_addr);\
        else                                                    \
          STRUCT_ASSIGN_MACRO((pgptr)->dmpp_v1_page.page_log_addr,log_addr);\
     }
#define DMPP_VPT_GET_LOG_ADDR_LOW_MACRO(page_type, pgptr)	\
	(page_type != DM_COMPAT_PGTYPE ?				\
	(pgptr)->dmpp_v2_page.page_log_addr.lsn_low :	\
	(pgptr)->dmpp_v1_page.page_log_addr.lsn_low)

#define DMPP_VPT_GET_LOG_ADDR_HIGH_MACRO(page_type, pgptr)	\
	(page_type != DM_COMPAT_PGTYPE ?				\
	(pgptr)->dmpp_v2_page.page_log_addr.lsn_high :	\
	(pgptr)->dmpp_v1_page.page_log_addr.lsn_high)

#define DMPP_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, pgptr) 	\
        (page_type != DM_COMPAT_PGTYPE ? 			\
        (LG_LSN *)&((pgptr)->dmpp_v2_page.page_log_addr) : 	\
        (LG_LSN *)&((pgptr)->dmpp_v1_page.page_log_addr))

/* Additions for MVCC */

#define DMPP_VPT_IS_CR_PAGE(page_type, pgptr) \
        (page_type != DM_COMPAT_PGTYPE \
	  ? (pgptr)->dmpp_v2_page.page_stat & DMPP_CREAD \
	  : 0)

#define DMPP_VPT_GET_PAGE_LG_ID_MACRO(page_type, pgptr) \
	((page_type != DM_COMPAT_PGTYPE) \
	    ? (pgptr)->dmpp_v2_page.page_lg_id \
	    : 0 )

#define DMPP_V2_SET_PAGE_LG_ID_MACRO(pgptr, lg_id) 		\
	(pgptr)->dmpp_v2_page.page_lg_id = (u_i2)lg_id;

#define DMPP_VPT_SET_PAGE_LG_ID_MACRO(page_type, pgptr, lg_id)	\
	if (page_type != DM_COMPAT_PGTYPE) 			\
	    DMPP_V2_SET_PAGE_LG_ID_MACRO(pgptr, lg_id)

#define DMPP_VPT_GET_PAGE_LSN_MACRO(page_type, pgptr) \
        (page_type != DM_COMPAT_PGTYPE \
          ? (pgptr)->dmpp_v2_page.page_log_addr \
          : ((DMPP_V1_PAGE *)pgptr)->page_log_addr)

#define DMPP_VPT_SET_PAGE_LRI_MACRO(page_type, pgptr, lri) \
	if (page_type != DM_COMPAT_PGTYPE) \
	{ \
	    ((DMPP_V2_PAGE*)pgptr)->page_log_addr = (lri)->lri_lsn; \
	    ((DMPP_V2_PAGE*)pgptr)->page_lg_id = (lri)->lri_lg_id; \
	    ((DMPP_V2_PAGE*)pgptr)->page_lga = (lri)->lri_lga; \
	    ((DMPP_V2_PAGE*)pgptr)->page_jnl_filseq = (lri)->lri_jfa.jfa_filseq; \
	    ((DMPP_V2_PAGE*)pgptr)->page_jnl_block = (lri)->lri_jfa.jfa_block; \
	    ((DMPP_V2_PAGE*)pgptr)->page_bufid = (lri)->lri_bufid; \
        } \
	else \
	    ((DMPP_V1_PAGE*)pgptr)->page_log_addr = (lri)->lri_lsn;

#define DMPP_VPT_GET_PAGE_LRI_MACRO(page_type, pgptr, lri) \
	if (page_type != DM_COMPAT_PGTYPE) \
	{ \
	    (lri)->lri_lsn = ((DMPP_V2_PAGE*)pgptr)->page_log_addr; \
	    (lri)->lri_lg_id = ((DMPP_V2_PAGE*)pgptr)->page_lg_id; \
	    (lri)->lri_lga = ((DMPP_V2_PAGE*)pgptr)->page_lga; \
	    (lri)->lri_jfa.jfa_filseq = ((DMPP_V2_PAGE*)pgptr)->page_jnl_filseq; \
	    (lri)->lri_jfa.jfa_block = ((DMPP_V2_PAGE*)pgptr)->page_jnl_block; \
	    (lri)->lri_bufid = ((DMPP_V2_PAGE*)pgptr)->page_bufid; \
        } \
	else \
	{ \
	    (lri)->lri_lsn = ((DMPP_V1_PAGE*)pgptr)->page_log_addr; \
	    (lri)->lri_lg_id = 0; \
	    (lri)->lri_lga.la_sequence = 0; \
	    (lri)->lri_lga.la_block = 0; \
	    (lri)->lri_lga.la_offset = 0; \
	    (lri)->lri_jfa.jfa_filseq = 0; \
	    (lri)->lri_jfa.jfa_block = 0; \
	    (lri)->lri_bufid = 0; \
	}

#define DMPP_VPT_GET_PAGE_LGA_MACRO(page_type, pgptr, lga) \
	if ( page_type != DM_COMPAT_PGTYPE ) \
	    lga = (pgptr)->dmpp_v2_page.page_lga; \
	else \
	    lga.la_sequence = lga.la_block = lga.la_offset = 0;

#define DMPP_VPT_PAGE_HAS_LGA_MACRO(page_type, pgptr) \
	(page_type != DM_COMPAT_PGTYPE \
	  ? ((DMPP_V2_PAGE*)pgptr)->page_lga.la_block \
	  : 0)

#define DMPP_VPT_GET_PAGE_JNL_FILSEQ_MACRO(page_type, pgptr) \
	(page_type != DM_COMPAT_PGTYPE \
	  ? ((DMPP_V2_PAGE*)pgptr)->page_jnl_filseq \
	  : 0)

#define DMPP_VPT_GET_PAGE_JNL_BLOCK_MACRO(page_type, pgptr) \
	(page_type != DM_COMPAT_PGTYPE \
	  ? ((DMPP_V2_PAGE*)pgptr)->page_jnl_block \
	  : 0)

#define DMPP_VPT_GET_PAGE_BUFID_MACRO(page_type, pgptr) \
	(page_type != DM_COMPAT_PGTYPE \
	  ? ((DMPP_V2_PAGE*)pgptr)->page_bufid \
	  : 0)
	    
/* End of MVCC additions */

 
#define DMPP_V1_GET_PAGE_NEXT_LINE_MACRO(pgptr)			\
        	(pgptr)->dmpp_v1_page.page_next_line
 
#define DMPP_V2_GET_PAGE_NEXT_LINE_MACRO(pgptr)			\
        	(pgptr)->dmpp_v2_page.page_next_line 
 
#define DMPP_VPT_GET_PAGE_NEXT_LINE_MACRO(page_type, pgptr)	\
        (page_type != DM_COMPAT_PGTYPE ?				\
	 DMPP_V2_GET_PAGE_NEXT_LINE_MACRO(pgptr) :		\
         DMPP_V1_GET_PAGE_NEXT_LINE_MACRO(pgptr))
 
#define DMPP_V1_SET_PAGE_NEXT_LINE_MACRO(pgptr, value)		\
     {								\
         (pgptr)->dmpp_v1_page.page_next_line = value;	\
     } 

#define DMPP_V2_SET_PAGE_NEXT_LINE_MACRO(pgptr, value)		\
     {								\
         (pgptr)->dmpp_v2_page.page_next_line = value;	\
     } 

#define DMPP_VPT_SET_PAGE_NEXT_LINE_MACRO(page_type, pgptr, value)\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)			\
	    DMPP_V2_SET_PAGE_NEXT_LINE_MACRO(pgptr, value)	\
	else							\
	    DMPP_V1_SET_PAGE_NEXT_LINE_MACRO(pgptr, value)	\
     } 

#define DMPP_V1_INCR_PAGE_NEXT_LINE_MACRO(pgptr)		\
     {								\
        (pgptr)->dmpp_v1_page.page_next_line++;		\
     }

#define DMPP_V2_INCR_PAGE_NEXT_LINE_MACRO(pgptr)		\
     {								\
        (pgptr)->dmpp_v2_page.page_next_line++;		\
     }

#define DMPP_VPT_INCR_PAGE_NEXT_LINE_MACRO(page_type, pgptr)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)			\
	    DMPP_V2_INCR_PAGE_NEXT_LINE_MACRO(pgptr)		\
	else							\
	    DMPP_V1_INCR_PAGE_NEXT_LINE_MACRO(pgptr)		\
     }

#define DMPP_V1_PAGE_CONTENTS_MACRO(pgptr) 			\
          	(pgptr)->dmpp_v1_page.page_contents
 
#define DMPP_V2_PAGE_CONTENTS_MACRO(pgptr) 			\
                (pgptr)->dmpp_v2_page.page_contents 
 
#define DMPP_VPT_PAGE_CONTENTS_MACRO(page_type, pgptr) 		\
        ((page_type != DM_COMPAT_PGTYPE) ? 			\
	 DMPP_V2_PAGE_CONTENTS_MACRO(pgptr) :			\
	 DMPP_V1_PAGE_CONTENTS_MACRO(pgptr))
 
#define DMPP_VPT_SIZEOF_TUPLE_HDR_MACRO(page_type)			\
	((page_type == DM_PG_V3) ? (i4)sizeof(DMPP_TUPLE_HDR_V3) :	\
	 (page_type == DM_PG_V4) ? (i4)sizeof(DMPP_TUPLE_HDR_V4) :	\
	 (page_type == DM_PG_V5) ? (i4)sizeof(DMPP_TUPLE_HDR_V5) :	\
	 (page_type == DM_PG_V6) ? (i4)sizeof(DMPP_TUPLE_HDR_V6) :	\
	 (page_type == DM_PG_V7) ? (i4)sizeof(DMPP_TUPLE_HDR_V7) :	\
	 (page_type == DM_PG_V1) ? 0 :					\
	 (i4)sizeof(DMPP_TUPLE_HDR_V2))

/* Does this page type support row locking? */
#define DMPP_VPT_PAGE_HAS_ROW_LOCKING(page_type) 			\
	((page_type == DM_PG_V1) ? 0 : 1)

/* Page type has versions if tuple header has alter table row version */
#define DMPP_VPT_PAGE_HAS_VERSIONS(page_type) 			\
    ((page_type == DM_PG_V2 || page_type == DM_PG_V3 ||	\
    page_type == DM_PG_V5 || page_type == DM_PG_V6 || page_type == DM_PG_V7) \
    ? 1 : 0)

/* Page type supports segmented data if not DM_COMPAT_PGTYPE */
#define DMPP_VPT_PAGE_HAS_SEGMENTS(page_type) 			\
	((page_type == DM_PG_V5 || page_type == DM_PG_V7) ? 1 : 0)

/* Page type has lg_id if tuple header has lg_id  */
#define DMPP_VPT_PAGE_HAS_LG_ID(page_type) 			\
    ((page_type == DM_PG_V6 || page_type == DM_PG_V7 || page_type == DM_PG_V2)\
    ? 1 : 0)

#define  DMPP_VPT_GET_PAGE_1STFREE_MACRO(page_type, page)	\
    ((page_type != DM_COMPAT_PGTYPE) ?                  		\
    ((DMPPC_V2_CONTENTS *)((page)->dmpp_v2_page.page_contents))->temp_1stfree: \
    ((DMPPC_V1_CONTENTS *)((page)->dmpp_v1_page.page_contents))->temp_1stfree)
 
#define  DMPP_VPT_SET_PAGE_1STFREE_MACRO(page_type, page, value)	\
     {								\
	if (page_type != DM_COMPAT_PGTYPE)                         	\
	  ((DMPPC_V2_CONTENTS *)					\
		((page)->dmpp_v2_page.page_contents))->temp_1stfree = value;\
	else                                                	\
	  ((DMPPC_V1_CONTENTS *)					\
		((page)->dmpp_v1_page.page_contents))->temp_1stfree = value;\
     }
 
#define  DMPP_VPT_INCR_PAGE_1STFREE_MACRO(page_type, page, value)\
     {								\
	if (page_type != DM_COMPAT_PGTYPE)                         	\
	  ((DMPPC_V2_CONTENTS *)					\
	     ((page)->dmpp_v2_page.page_contents))->temp_1stfree += value; \
	else                                                	\
	  ((DMPPC_V1_CONTENTS *)					\
	     ((page)->dmpp_v1_page.page_contents))->temp_1stfree +=value; \
     }
 
 
#define  DMPP_VPT_DECR_PAGE_1STFREE_MACRO(page_type, page, value)\
     {								\
	if (page_type != DM_COMPAT_PGTYPE)                         	\
	  ((DMPPC_V2_CONTENTS *)					\
	      ((page)->dmpp_v2_page.page_contents))->temp_1stfree -= value; \
	else                                                	\
	  ((DMPPC_V1_CONTENTS *)					\
	      ((page)->dmpp_v1_page.page_contents))->temp_1stfree -= value; \
     }

/* any change to this macro will probably require change to DM1B counterpart */
/*
** For MVCC, check also for crow_locking(), return TRUE if
** page in question is a CR page.
*/
#define DMPP_SKIP_DELETED_ROW_MACRO(r, page, lg_id, low_tran)		\
	((!row_locking(r) && !crow_locking(r)) || !low_tran 	\
	|| DMPP_VPT_IS_CR_PAGE(r->rcb_tcb_ptr->tcb_rel.relpgtype, page) \
	|| low_tran == r->rcb_tran_id.db_low_tran		\
	|| !(LSN_GTE(DMPP_VPT_ADDR_PAGE_LOG_ADDR_MACRO(		\
	r->rcb_tcb_ptr->tcb_rel.relpgtype, page), &r->rcb_oldest_lsn)) ? \
	TRUE : IsTranActive(lg_id, low_tran) ? FALSE : TRUE)

/* any change to this macro will probably require change to DM1B counterpart */
#define DMPP_DUPCHECK_NEED_ROW_LOCK_MACRO(r, page, lg_id, low_tran)	\
	((!row_locking(r) && !crow_locking(r)) || !low_tran 	\
	|| DMPP_VPT_IS_CR_PAGE(r->rcb_tcb_ptr->tcb_rel.relpgtype, page) \
	|| low_tran == r->rcb_tran_id.db_low_tran		\
	|| !(LSN_GTE(DMPP_VPT_ADDR_PAGE_LOG_ADDR_MACRO(		\
	r->rcb_tcb_ptr->tcb_rel.relpgtype, page), &r->rcb_oldest_lsn)) ? \
	FALSE : IsTranActive(lg_id, low_tran) ? TRUE : FALSE)


#define DMPP_TPERPAGE_MACRO(page_type, page_size, relwid)	\
((((page_size - DMPPN_VPT_OVERHEAD_MACRO(page_type))/(relwid +	\
DM_VPT_SIZEOF_LINEID_MACRO(page_type) +				\
DMPP_VPT_SIZEOF_TUPLE_HDR_MACRO(page_type)))) > DM_TIDEOF ?	\
DM_TIDEOF:							\
((page_size - DMPPN_VPT_OVERHEAD_MACRO(page_type))/(relwid +	\
DM_VPT_SIZEOF_LINEID_MACRO(page_type) +				\
DMPP_VPT_SIZEOF_TUPLE_HDR_MACRO(page_type))))

