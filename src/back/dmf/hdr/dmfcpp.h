/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DMFCPP.H - Used by Ckpdb.
**
** Description:
**      This file contains the header information of the dmfcpp.c
**      module.
**
** History:
**      07-jan-1995 (alison)
**		Created.
**      06-feb-1995 (stial01)
**              Added CPP_VIEW_TABLE and CPP_GATEWAY table to distinguish 
**              from core table.
**      08-mar-1995 (stial01)
**              Bug 67327: added CPP_INDEX_REQUIRED, CPP_BLOB_REQUIRED
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      08-Oct-2002 (hanch04)
**          Replace the i4 for length with a SIZE_TYPE (size_t) to allow
**          64-bit memory allocation.
**      07-apr-2005 (stial01)
**          Changes for queue management in ckpdb
**      01-Aug-2005 (sheco02)
**          Fixed mistakes in x-integration change 478041.
**      05-Aug-2005 (hanje04)
**          Back out change 478041 as it should never have been X-integrated
**          into main in the first place.
**      23-oct-2007 (stial01)
**          Support table level CPP/RFP for partitioned tables (b119315)
**/

/*
**  This section includes all forward references required
*/
typedef struct _DMF_CPP		DMF_CPP;	/* Checkpoint context. */
typedef struct _CPP_TBLHCB	CPP_TBLHCB;
typedef struct _CPP_TBLCB	CPP_TBLCB;
typedef struct _TBL_HASH_ENTRY  TBL_HASH_ENTRY;
typedef struct _CPP_LOC_MASK    CPP_LOC_MASK;
typedef struct _CPP_FNMS        CPP_FNMS;
typedef struct _CPP_QUEUE	CPP_QUEUE;

/*}
** Name: CPP_QUEUE - CPP queue definition.
**
** Description:
**      This structure defines the CPP queue.
**
** History:
**     07-apr-2005 (stial01)
**          Created.
*/
struct _CPP_QUEUE
{
    CPP_QUEUE		*q_next;	    /*	Next queue entry. */
    CPP_QUEUE		*q_prev;	    /*  Previous queue entry. */
};


/*
** Name: CPP_TBLCB - Checkpoint control block holds info on a table being
**		     checkpointed.
**
** Description:
**
**
** History
**      07-jan-1995 (alison)
**          Partial Checkpoint:
**	  	Created.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Define TBL_CB as DM_TBL_CB.
*/
struct    _CPP_TBLCB
{
    CPP_TBLCB	    *tblcb_q_next;	     /* TBL hash queue for base */
    CPP_TBLCB	    *tblcb_q_prev;
    SIZE_TYPE	    tblcb_length;
    i2		    tblcb_type;
#define	    TBL_CB			DM_TBL_CB
    i2		    tblcb_s_reserved;
    PTR		    tblcb_l_reserved;
    PTR		    tblcb_owner;
    i4	    tblcb_ascii_id;
#define	CPP_TBLCB_ASCII_ID		 CV_C_CONST_MACRO('#', 'T', 'B', 'L')
    CPP_QUEUE	    tblcb_totq;			/* Queue of all tables */
    CPP_QUEUE	    tblcb_invq;			/* Queue of invalid TCB's */
    DB_TAB_NAME     tblcb_table_name;
    DB_OWN_NAME     tblcb_table_owner;
    DB_TAB_ID       tblcb_table_id;
    DB_STATUS       tblcb_table_err_status;
    i4         tblcb_table_err_code;
    u_i4       tblcb_table_status;
#define	       CPP_TABLE_OK	       0x0000L  /* Default status */
#define        CPP_CKPT_TABLE          0x0001L  /* Ok to checkpoint */
#define        CPP_INVALID_TABLE       0x0002L  /* No info avail */
#define        CPP_FAILED_TABLE        0x0004L  /* Failed checkpointing */
#define	       CPP_CATALOG             0x0008L  /* System Catalog table */
#define	       CPP_USER_SPECIFIED      0x0010L  /* User specified table */
#define        CPP_VIEW                0x0020L  /* View                 */
#define        CPP_GATEWAY             0x0040L  /* Gateway table        */
#define        CPP_TBL_CKP_DISALLOWED  0x0080L  /* tbl ckp disallowed */
#define        CPP_INDEX_REQUIRED      0x0100L  /* This is an index for */
						/* a user specified tbl */
#define        CPP_BLOB_REQUIRED       0x0200L  /* This is a blob for   */
						/* a user specified tbl */
#define        CPP_PART_REQUIRED       0x0400L  /* This is partition for*/
						/* a user specified tbl */
#define        CPP_IS_PARTITIONED      0x0800L  /* Partitioned table */
#define        CPP_PARTITION           0x1000L  /* Table is a partition */
};

/*
** Name: CPP_TBLHCB - Ckpdb internal control block holding information
**                    on tables being checkpointed.
**
** Description:
**
**
** History
**      07-jan-1995 (alison)
**          Partial Checkpoint:
**		Created.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Define TBL_HCB_CB as DM_TBL_HCB_CB.
*/
struct      _CPP_TBLHCB
{
    CPP_TBLCB	    *tblhcb_q_next;	
    CPP_TBLCB	    *tblhcb_q_prev;
    SIZE_TYPE	    tblhcb_length;
    i2		    tblhcb_type;
#define		TBL_HCB_CB		DM_TBL_HCB_CB
    i2              tblhcb_s_reserved;         /* Reserved for future use. */
    PTR             tblhcb_l_reserved;         /* Reserved for future use. */
    PTR             tblhcb_owner;              /* Owner of control block for
                                               ** memory manager.  HCB will be
                                               ** owned by the server. 
					       */
    i4         tblhcb_ascii_id;           
#define  CPP_TBLHCB_ASCII_ID    	CV_C_CONST_MACRO('#', 'H', 'C', 'B')
    i4	    tblhcb_no_hash_buckets;
    CPP_TBLCB	    *tblhcb_huserq_next;       /* Queue of tables supplied by 
					       ** the user
					       */
    CPP_QUEUE	    tblhcb_htotq;		/* Queue of all tables */
    CPP_QUEUE	    tblhcb_hinvq;		/* Queue of invalid TCB's */
    struct    _TBL_HASH_ENTRY
    {
	CPP_TBLCB   *tblhcb_headq_next;
	CPP_TBLCB   *tblhcb_headq_prev;
    } tblhcb_hash_array[1];
};

/*
** Name: CPP_TBLHCB - Ckpdb internal control block holding information
**                    on tables being checkpointed.
**
** Description:
**
**
** History
**      07-jan-1995 (alison)
**	    Partial Backup Project:
**		Created.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Define CPP_LOCM_CB as DM_CPP_LOCM_CB.
*/
struct      _CPP_LOC_MASK
{
    i4         *cpp_locm_next;         /* Not used. */
    i4         *cpp_locm_prev;         /* Not used. */
    SIZE_TYPE       cpp_locm_length;        /* Length of control block. */
    i2              cpp_locm_type;          /* Type of control block for
                                            ** memory manager. 
					    */
#define                 CPP_LOCM_CB         DM_CPP_LOCM_CB
    i2              cpp_locm_s_reserved;    /* Reserved for future use. */
    PTR             cpp_locm_l_reserved;    /* Reserved for future use. */
    PTR             cpp_locm_owner;         /* Owner of control block for
                                            ** memory manager.  SVCB will be
                                            ** owned by the server. 
					    */
    i4         cpp_locm_ascii_id;      
#define           CPP_LOCM_ASCII_ID       CV_C_CONST_MACRO('L', 'O', 'C', 'M')

/*
** The folloing field points to a bitmap of locations which will be used
** during checkpoint.
** The map should be accessed using the BT calls in the CL; bit 0 corresponds
** to extent array element 0, and so forth.
**
** The space for the bit maps will be allocated dynamically at the end of this
** structure as soon as the number of config file extents is known.
*/
    i4     locm_bits;          /* number of bits in each following field */
    i4     *locm_r_allow;      /* locs used during the checkpoint */
};



/*
** Name: CPP_FNMS - Ckpdb internal control block holding information
**                  on filenames being checkpointed.
**
** Description:
**
**
** History
**      07-jan-1995 (alison)
**	    Partial Backup Project:
**		Created.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Define CPP_FNMS_CB as DM_CPP_FNMS_CB.
*/
struct      _CPP_FNMS
{
    i4         *cpp_fnms_next;       /* Not used. */
    i4         *cpp_fnms_prev;       /* Not used. */
    SIZE_TYPE       cpp_fnms_length;      /* Length of control block. */
    i2              cpp_fnms_type;        /* Type of control block for
                                           ** memory manager. 
					   */
#define                 CPP_FNMS_CB        DM_CPP_FNMS_CB 
    i2              cpp_fnms_s_reserved;  /* Reserved for future use. */
    PTR             cpp_fnms_l_reserved;  /* Reserved for future use. */
    PTR             cpp_fnms_owner;       /* Owner of control block for
                                           ** memory manager.  SVCB will be
                                           ** owned by the server. 
					   */
    i4         cpp_fnms_ascii_id;      
#define           CPP_FNMS_ASCII_ID       CV_C_CONST_MACRO('F', 'N', 'M', 'S')

/*
** The following field points to a character string containing filenames
** separated by blanks.        
** The space for the structure will be allocated at the end of this structure
*/
    char        *cpp_fnms_str;            /* file names for a location */
};

/*}
** Name: DMF_CPP - Checkpoint context.
**
** Description:
**      This structure contains information needed by the checkpoint routines.
**
** History:
**       7-jan-1994 (alison)
**          Created.
**	
[@history_template@]...
*/
struct _DMF_CPP
{
    DMF_JSX		*cpp_jsx;            /* Pointer to journal context. */
    DMCKP_CB		cpp_dmckp;	     /* Holds CK context */
    CPP_TBLCB		*cpp_cur_tblcb;      /*
					     ** Holds the tblcb for the current 
					     ** journal or dump record that is
					     ** being applied. Only relevent for
					     ** table level checkpoint.
					     */
    CPP_TBLHCB		*cpp_tblhcb_ptr;     /* Structure containing all 
					     ** information on tables
					     */
    CPP_LOC_MASK	*cpp_loc_mask_ptr;   /* Ptr to mask indicating what
					     ** locations we are interested
					     ** in
					     */
    i4             cpp_fcnt_at_loc[DM_LOC_MAX];  
					     /* Number of interesting files
					     ** at each location
					     */
    CPP_FNMS            *cpp_fnames_at_loc[DM_LOC_MAX];
					     /* Struct containing all file
					     ** names we are interested in
					     ** at each location
					     */
    i4             cpp_flen_at_loc[DM_LOC_MAX];
					     /* Length of corresponding   
					     ** cpp_fnames_at_loc entry
					     */
};

