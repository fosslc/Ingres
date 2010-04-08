/*
** Copyright (c) 1989, 2001 Ingres Corporation
*/
/**
** Name: GWRMS.H - Definitions for the VMS GWRMS Gateway Exits
**
** Description:
**      The file contains the structure and constant defintions used by
**      the RMS gateway exit.  Structs are:
**		GWRMS_RSB
**		GWRMS_XREL
**		GWRMS_XATT
**		GWRMS_XIDX
**		GWRMS_XTCB
**	Constants are:
**		Gwrms_xatt_tidp -- GLOBALREF'd here, defined in gwrms.c
**
** History:
**      30-May-1989 (alexh)
**          Created
**	23-dec-89 (paul)
**	    Added support for an RMS specific TCB. This will contain the
**	    information needed to do datatype conversions.
**	24-dec-89 (paul)
**	    Added goffset to GWRMS_XATT. Need to know where the data is.
**	30-jan-90 (linda)
**	    Change definition of GWRMS_XIDX struct (represents the tuple layout
**	    for iigw02_index table).
**	8-may-90 (linda)
**	    Change layout of GWRMS_XATT record.  Remove "gprecision" field and
**	    replace it with gprec_scale, which stores precision and scale in a
**	    single 2-byte quantity (to facilitate use of standard macros when
**	    reading/writing packed decimal datatype).
**	08-may-90 (alan)
**	    Added GWRMS_XSCB, gateway-specific session control block.
**	18-may-90 (linda)
**	    Change GWRMS_XATT one more time:  add "gflags" field, similar to
**	    the "attflags" field in iiattribute.  We need one of our own, used
**	    at present to indicate (a) whether an extended format was specified
**	    and (b) whether the column is a variant.  We could have used the
**	    "attflags" field, but they're getting low on bits anyway, & no
**	    reason not to have our own.
**	27-sep-90 (linda)
**	    Add a reference to Gwrms_xatt_tidp, a constant GWRMS_XATT defined
**	    in gwrms.c for use in implementing secondary indexes; also maximum
**	    lengths/values for certain Rms-Gateway-specific items.
**      07-jun-93 (schang)
**          add a few fields in gwrms_rsb for repeating group support
**      02-aug-92 (schang)
**          retrofit file sharing from 6.5
**          in gwrms_rsb, change "FAB fab" to "rms_fab_info *fab" for
**          RMS FAB sharing.
**      07-jun-93 (schang)
**          add a few field in gwrms_rsb for repeating group support
**      08-dec-93 (schang)
**          add structure definition for RFA buffering
**      16-may-96 (schang)
**          add II_RMS_CLOSE_FILE logical so that RMS file be close when no
**          body uses it.
**      03-mar-1999 (chash01)
**          integrate all changes since ingres 6.4
**	28-feb-2001	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from RMS GW code as the use is no longer allowed
**/

/*
**  Defines for RMS Gateway.
*/
#define	RMS_MAXKEY  255


/*
**  These constants are used when parsing extended formats.
*/
#define	RMS_MAXWRDS 10	    /* max tokens in extended format spec. */
#define	RMS_MAXPRMS 5	    /* max parameters in extended format spec. */
#define	RMS_MAXLIN  255	    /* max buffer size for extended format spec. */
#define	RMS_MAXNMLN 10	    /* max bytes in a number in extended format spec. */
/*
** 12-aug-93 (schang)
**   ii_rms_rrl read-regardless-lock bit pattern  0x1
** 27-sep-93 (schang)
**   ii_rms_readonly flag                         0x2
** 08-dec-93 (schang)
**   ii_rms_buffer_rfa flag                       0x4
**   also define the pointer array size for the address to be stuffed into
**   xrcb_handle field
** 11-jun-96 (schang)
**   define ii_rms_close_file, when this env logical is defined at startup
**   time, file will be closed when query is done AND at that time no other 
**   sessions are accessing the rms file. 
*/
#define II_RMS_RRL          0x00000001
#define II_RMS_READONLY     0x00000002
#define II_RMS_BUFFER_RFA   0x00000004
#define II_RMS_CLOSE_FILE   0x00000008

#define RMSMAXMEMPTR  2
#define RMSFABMEMIDX  0
#define RMSRFAMEMIDX  1
/* the xrcb_handle should be assigned the addrees of pointer */
/* array declared as PTR gwrms_memarray[RMSMAXMEMPTR];       */

struct gwrms_rfa
{
    struct gwrms_rfa *left;
    struct gwrms_rfa *right;
    u_i4               rfa0;
    u_i2               rfa4;
    u_i2               seqno;
    i1                 bias;
#define BALANCED       0           /* flags for tree balance */
#define LEFTHEAVY      1
#define RIGHTHEAVY     2
};

#define RMSRFALISTSIZE 2500
struct gwrms_rfa_chunk 
{
    struct gwrms_rfa_chunk *next;
    struct gwrms_rfa rfa_list[RMSRFALISTSIZE];
};

/* schang : global rfa pool handler */
struct gwrms_rfapool_handle
{
    struct gwrms_rfa       *rfahdr;
    struct gwrms_rfa_chunk *memlist;
    CS_SEMAPHORE           rfacntrl;
};

/* schang : session level table handle */
struct gwrms_table_array
{
    i4  tab_id;                 /* must indicate a base table */
    i4  use_count;              /* Do not release unless it is 0 */
    struct gwrms_rfa *rfalist;  /* must be NULL it no RFA buffered */
};    

#define RMSSTKINITSIZE 250
#define RMSMAXTABNO  32
typedef struct
{
    struct gwrms_rfa **stack;
    i4  stacksize;
    struct gwrms_table_array tab_array[RMSMAXTABNO];
}GWRMS_TAB_HANDLE; 
    
 
/*}
** Name: GWRMS_RSB - GWRMS Gateway Record sequence block
**
** Description:
**	A record sequence block contains all information needed by the RMS
**	gateway exits to process record manipulation requests efficiently.
**
**	In current desgin, both RMS FAB and RAB are included. It is assumed
**	that there is no need to share FAB's. If such sharing is required, the
**	gateway exit interface must be modified to include the analogs of TCB
**	and RCB. (One reason FAB is not shared is because FAB is not an exact
**	analog of TCB.)
**
** History:
**	31-Mar-1989 (alexh)
**          Created
**	6-aug-91 (rickh)
**	    Added state word.
**      07-jun-93 (schang)
**          add a few field in gwrms_rsb for repeating group support
*/
typedef struct
{
    f8	    *lock;		/* lock word */

    i4	     rsb_state;		/* status flags */

#define	GWRMS_RAB_POSITIONED	0x01	/* RAB is positioned on a record */

    PTR	    rms_buffer;		/* Record buffer for reading/writing RMS file */
    i4	     rms_bsize;		/* Size of RMS record buffer */
    PTR	    low_key;		/* low key value */
    int	    low_key_len;	/* low key length */
    PTR	    high_key;		/* high key value */
    int	    high_key_len;	/* high key length */
    i4	    key_ref;		/* RMS key reference */
    unsigned short low_rfa[3];	/* low bound RFA. For reposition. */
    struct  rms_fab_info *fab;	/* RMS FAB */
    struct  RAB rab;		/* RMS RAB */
    /* following data area for repeating group */
    bool    rptg_is_rptg;       /* signal record having repeating group */
#define GWRMS_REC_RPTG_NULL     0x00    /* repeat group or not unknown. */
#define GWRMS_REC_IS_RPTG       0x01    /* is a repeating group */
#define GWRMS_REC_NOT_RPTG      0x02    /* is not a repeating group */


    i4      rptg_seq_no;        /* index into repeating group */
    i4      rptg_size;          /* calculated repeating group size */
    i4      rptg_count;         /* number of repeating members */
    PTR     rptg_buffer;        /* reserve for write repeating group */
} GWRMS_RSB;

/*}
** Name: GWRMS_XREL - RMS Gateway Relation
**
** Description:
**      This defines the row in the RMS iigw02_relation catalog.
**
** History:
**	May-30-1989 (alexh)
**	    Created
**	Dec-13-1989 (linda)
**	    Change data structures to match extended relation tuples.
*/
typedef struct
{
    DB_TAB_ID	xrel_tab_id;	    /* table id */
    char	file_name[255];	    /* name of data file */
} GWRMS_XREL;

/*}
** Name: GWRMS_XATT - RMS Gateway Attribute extended catalog.
**
** Description:
**      This defines the row in the RMS iigw02_attribute catalog.
**
** History:
**	May-30-1989 (alexh)
**          Created
**	Dec-13-1989 (linda)
**	    Change data structures to match extended attribute tuples.
*/
typedef struct
{
    DB_TAB_ID	xatt_tab_id;	/* table id */
    i2          xattid;         /* attribute number */
    i4		gtype;		/* external attribute type */
				/* type ids are from ADF */
    i4		glength;	/* external attribute size */
    i4		goffset;	/* offset of field in rms file */
    i2		gprec_scale;	/* precision (high byte) and scale (low byte) */
    i2		gflags;		/* flags */
} GWRMS_XATT;

/*}
** Name: GWRMS_XIDX - RMS Gateway index extended catalog
**
** Description:
**      This defines the row in the RMS iigw02_relation catalog.
**
** History:
**	May-30-1989 (alexh)
**          Created
**	Dec-13-1989 (linda)
**	    Change data structures to match extended index tuples.
*/
typedef struct
{
    DB_TAB_ID	xidx_idx_id;	/* index table id */
    i2		key_ref;	/* RMS key ref. # */
} GWRMS_XIDX;

/*}
** Name: GWRMS_XTCB  - RMS Gateway table control block
**
** Description:
**	GWRMS_XTCB is an extension to the GWF TCB that contains RMS specific
**	information.
**
** History:
**	23-dec-89 (paul)
**	    Created to support the RMS gateway
*/
typedef struct
{
    PTR		xtcb_variantcx;	/* CX executed to determine if this record is
				** a qualifying variant. If this expression
				** returns TRUE, this record qualifies; FALSE
				** indicates this record belongs to another
				** variant.  This field is not used at this
				** time but is included to indicate where this
				** functionality will be placed.
				*/
    PTR		xtcb_fromrmscx;	/* The CX used to convert records from RMS
				** format to ingres format.
				*/
    PTR		xtcb_tormscx;	/* The CX used to convert records from INGRES
				** format to RMS format.
				*/
    PTR		xtcb_idxcx;	/* The CX used to convert a 'key' from INGRES
				** format to RMS format.
				*/
} GWRMS_XTCB;

/*}
** Name: GWRMS_XSCB - RMS Gateway session control block
**
** Description:
**	GWRMS_XSCB is an extension to the GWF GW_SESSION structure that 
**	contains RMS specific session information.
**
** History:
**	08-may-90 (alan)
**	    Created to support the RMS gateway
*/
typedef struct
{
    struct  dsc$descriptor_s  gws_username_dsc; /* Username String Descriptor */
} GWRMS_XSCB;

/*
**  Gwrms_xatt_tidp --	pointer to RMS Gateway tidp tuple in iigwX_attribute
**			catalog
*/
GLOBALREF   GWRMS_XATT	Gwrms_xatt_tidp;


/*
** 02-aug-93 (schang)
**     Everything below this line is retrofit from 6.5 for 
**     supporting file sharing.
*/

/*}
** Name: RMS_FAB_INFO - RMS Gateway file control block information block
**                      to be chained together as unused (free) chache.
**
** Description:
**      Structure that is to be used to keep all information about an open
**      RMS file, including RMS FAB.  This structure is to be shared by all
**      sessions in RMS server.
**
** History:
**      14-oct-1992 (schang)
**        created
**      12-aug-1993 (schang)
**        add rmskey[RMSMAXKEY]
**      25-oct-1993 (schang)
**        add     short file_hdr_size;  This is per file information but can
**        be obtained only through RFA.
**      11-jun-96 (schang)
**        remove file_hdr_size, and make maximum rms keys to be 24
*/
#define RMSMAXKEY 24

struct rms_fab_info
{
    struct rms_fab_info *next;      /* next rms_fab_info          */
    short count;                    /* usage count field          */
    short hash_id;                  /* hash table entry number    */
    short is_remote;                /* file opened through DECNet */
    char  fname[256];
    struct FAB rmsfab;              /* real FAB used by VMS/RMS   */
    struct XABKEY rmskey[RMSMAXKEY];/* store up to 24 rms key info */
};


/*}
** Name: RMS_FAB_HASH_ENT - Hash table for RMS Gateway file control block
**              information block (rms_fab_info.)  
**
** Description:
**      Hash table element structure that is to be used to keep rms_fab_info
**      when a file is opened.  The field of hash_control is for locking of
**      hash_entry.  Note that the real hashing table is defined in the next
**      data structure.  What defined here is just a template for hashing 
**      table element.
**
** History:
**      14-oct-1992 (schang)
**        created
*/

struct rms_fab_hash_ent
{
    struct rms_fab_info *hash_entry;/* ptr to rms_fab_info chain   */
    CS_SEMAPHORE  hash_control;     /* semaphore for this entry    */
};


/*}
** Name: RMS_FAB_HANDLE - the structure that contains all information about
**        file caching.
**
** Description:
**        this handle contains : pointer to fab cache hash table, from there
**        a specifc in-use fab cache can be found; a pointer to unused fab
**        cache list; a semaphore structure for critical region control of
**        unused cache list control; and a pointer to the fab cache when first
**        allocated, this field is needed to free memory when server shutdown.
**        A pointer field to rms_fab_handle is added in GWRMS per server 
**        handle, Gwf_facility.
**
** History:
**      14-oct-1992 (schang)
**        created
*/

struct rms_fab_handle
{
  struct rms_fab_hash_ent *hash_table;   /* hash table pointer */
  CS_SEMAPHORE  free_fab_control;        /* semaphore for free list */
  struct rms_fab_info *free_fabs;        /* free rms_fab_info list  */
  struct rms_fab_info *fab_cache;        /* fab cache pointer */
  i4 num_hash_ent;
  i4 num_fab_cache;
};

