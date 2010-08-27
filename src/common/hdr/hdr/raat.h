/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: raat.h	defines for raat call interface
**
** Description:
**	This file contains definitions for the RAAT routines.
##
## History:
##	3-apr-95 (stephenb)
##	    First written
##	18-apr-95 (lewda02)
##	    Add some vital fields to table handle.
##	19-apr-95 (lewda02)
##	    Define a flag to use in table handle "status" field.
##	25-apr-95 (lewda02)
##	    Add defines necessary for additional record code to build.
##	27-apr-95 (stephenb)
##	    Make raat.h indipendent by including all necesary structs
##	    and defines in this file
##	5-may-95 (stephenb/lewda02)
##	    added more record operation flags.
##	10-may-95 (lewda02/shust01)
##	    Changed naming convention.
##	19-may-95 (lewda02/shust01)
##	    Added a mask for access to only the lock flags.
##	 6-jun-95 (shust01)
##	    Define flags, variables needed for prefetching.
##	 7-jun-95 (shust01)
##	    Added an internal flag for repositioning.
##          Added row_request - how many rows user wishes to prefetch
##          Added row_actual  - how many rows actually prefetched
##	14-sep-95 (thaju02/shust01)
##	    Added #defines for datatypes. added pointer for internal_buffer
##	    and internal_buffer_size, needed for blob implementation.
##	    Added RAAT_BLOB_CB structure, needed for blob implementation.
##	    Added prototypes for IIcraat_blob_get() and IIcraat_blob_put().
##	16-nov-1995 (canor01)
##	    Added define for ME_FASTPATH_ALLOC for use with MEadvise().
##	23-nov-95 (emmag)
##	    Added IIcraat_init() and IIcraat_end().
##	08-jul-96 (toumi01)
##	    Modified to support axp.osf (64-bit Digital Unix).
##	    These changes are to handle that fact that on this
##	    platform sizeof(PTR)=8, sizeof(long)=8, sizeof(int)=4.
##	16-oct-96 (cohmi01)
##	    Add items for performance improvement - RAAT_BAS_RECDATA
##	    request allows reading base data directly from sec index.       
##	06-mar-96 (thaju02)
##	    Added tbl_pgsize.
##      28-may-97 (dilma04)
##          Added row locking flags.
##      01-dec-97 (stial01)
##          Added RAAT_VERSION_3, TABLE_SEND_BKEY, RAAT_INTERNAL_BKEY
##      26-may-98 (shust01)
##          Added tbl_cachepri to RAAT_TBL_ENTRY to reflect change done
##          to DMT_TBL_ENTRY in dmtcb.h.  bug 91016.
##      25-may-99 (hweho01)
##          Defined int to int for AIX 64-bit platform (ris_u64).
##
##      15-may-2000 (stial01)
##          Remove Smart Disk code
##	21-jan-1999 (hanch04)
##	    replace nat and longnat with i4
##	31-aug-2000 (hanch04)
##	    cross change to main
##	    replace nat and longnat with i4
##	31-aug-2000 (hanch04)
##	    replace i4 with int
##	26-Mar-2001 (hanch04)
##	    Updated to match the backend structures.
##	13-oct-2001 (somsa01)
##	    Updated to work properly on 64-bit platforms.
##	05-nov-2001 (somsa01)
##	    Only define SIZE_TYPE if it is not defined.
##  aug-2009 (stephenb)
##		Add proto for allocate_big_buffer()
##	8-Sep-2009 (bonro01)
##	    Remove undefined STATUS and i4 types.
##	1-Feb-2010 (hweho01) (SIR 123241)
##	    Added attr_srid and attr_geomtype into the RAAT_ATT_ENTRY 
##	    structure, so it is consistent with DMT_ATT_ENTRY in  
##	    dmtcb.h after change 501821.  
##	8-Jun-2010 (hweho01) Sir: 121123 and 122403 
##	    Avoid SEGV in raat tests, update the RAAT control    
##	    structures, so they match with the fields of control  
##	    blocks in dmtcb.h after the recent enhancements   
##	    such as long ID and column encryption supports.
##
*/

/* if 64bit memory access is supported */
# ifndef SIZE_TYPE
# if defined(size_t) || defined(_SIZE_T_DEFINED)
# define SIZE_TYPE      size_t
# else
# define SIZE_TYPE      long
# endif
# endif

# define		RAAT_MAXNAME		256
# define		RAAT_MAXKEYS		32
# define		RAAT_LOC_MAXNAME	32
# define		RAAT_OWN_MAXNAME	32

# define		ME_FASTPATH_ALLOC	3  /* for MEadvise() */

/*
** RAAT operation codes
*/
/*
** Session operations
*/
# define	RAAT_SESS_START		1L
# define	RAAT_SESS_END		2L
/*
** Transaction operations
*/
# define	RAAT_TX_BEGIN		10L
# define	RAAT_TX_COMMIT		11L
# define	RAAT_TX_ABORT		12L
/*
** Table operations
*/
# define	RAAT_TABLE_OPEN		20L
# define	RAAT_TABLE_CLOSE	21L
# define	RAAT_TABLE_LOCK		22L
/*
** Record operations
*/
# define	RAAT_RECORD_GET		30L
# define	RAAT_RECORD_PUT		31L
# define	RAAT_RECORD_POSITION	32L
# define	RAAT_RECORD_DEL		33L
# define	RAAT_RECORD_REPLACE	34L
# define	RAAT_BLOB_GET		35L
# define	RAAT_BLOB_PUT		36L

/*
** Miscellaneous constants
*/
#define         RAAT_DEF_PGHDR          40L


/*}
** Name: RAAT_TAB_ID - Table id.
**
** Description:
**      This structure defines an id which uniquely identifies a table.
##
## History:
##	27-apr-95 (stephenb)
##	    Copied from DM_TAB_ID and changed to generic datatypes
*/
typedef struct  _RAAT_TAB_ID
{
    unsigned int            db_tab_base;
    unsigned int            db_tab_index;
} RAAT_TAB_ID;
typedef struct _RAAT_TAB_ID       RAAT_DEF_ID;

/*}
** Name: RAAT_TAB_TIMESTAMP - Timestamp on table.
**
** Description:
**      This structure defines a timestamp on a table, for telling when it
**      was created, modified, etc.
**
## History:
##	27-apr-95 (stephenb)
##	    copied from DM_TAB_TIMESTAMP and changed to standard datatypes
*/
typedef struct
{
    unsigned int            db_tab_high_time;
    unsigned int            db_tab_low_time;
} RAAT_TAB_TIMESTAMP;
/*}
** Name: RAAT_QRY_ID - ID for query text stored in iiqrytext relation
**
** Description:
**      This structure defines the ID for the query text stored in the IIQRYTEXT
**      relation.  It's really just a timestamp.
**
## History:
##	27-apr-95 (stephenb)
##	    copied from DM_QUERY_ID and changed to standard datatypes.
*/
typedef struct _RAAT_QRY_ID
{
    unsigned int            db_qry_high_time;
    unsigned int            db_qry_low_time;
}   RAAT_QRY_ID;

/*}
** Name: RAAT_ATT_ENTRY - Structure for table attribute show information.
**
** Description:
**      This typedef defines the stucture needed for table attribute 
**      information.  If attribute information is requested in the table 
**      show call, the RAAT_ATT_ENTRY structure is returned in the 
**      buffer pointed to by RAAT_SHOW_CB.**
##
## History:
##	27-apr-95 (stephenb)
##	    Copied from DM_ATT_ENTRY and changed to standard datatypes.
##	18-nov-96 (hanch04)
##	    Update structure to include att_intlid
##	14-Jan-2005 (drivi01)
##	    Added another field to the RAAT_ATT_ENTRY structure called
##	    att_collID to be consistent with DMT_ATT_ENTRY struture in a back
##	    as per change #474345
*/
typedef struct
{
    char	 att_name[RAAT_MAXNAME];     /* Name of the attribute. */
    int	 att_number;		   /* Attribute number. */
    int	 att_offset;		   /* Offset to attribute. */
    int      att_type;              /* Data type for attribute. */
#define		 RAAT_DTE_TYPE    (short)  3  /* "date" */
#define		 RAAT_MNY_TYPE    (short)  5  /* "money" */
#define		 RAAT_DEC_TYPE    (short) 10  /* "decimal" */
#define		 RAAT_CHA_TYPE    (short) 20  /* "char" */
#define		 RAAT_VCH_TYPE    (short) 21  /* "varchar" */
#define		 RAAT_LVCH_TYPE	  (short) 22  /* "long varchar" */
#define		 RAAT_BYTE_TYPE   (short) 23  /* Byte */
#define		 RAAT_LBYTE_TYPE  (short) 25  /* Long byte */
#define		 RAAT_INT_TYPE    (short) 30  /* "i" */
#define		 RAAT_FLT_TYPE    (short) 31  /* "f" */
#define		 RAAT_CHR_TYPE    (short) 32  /* "c" */
#define		 RAAT_TXT_TYPE    (short) 37  /* "text" */

    int      att_width;		   /* Storage size of attribute. */
    int      att_prec;              /* Attribute precision. */
    int      att_flags;             /* set to RAAT_F_NDEFAULT if 
                                           ** attribute is non-nullable and
                                           ** non-defaultable. This can also
					   ** be set to RAAT_F_RULE if rules
					   ** are applied.
					   */
    int	 att_key_seq_number;	   /* Sequence number of attribute in primary key. */
    RAAT_DEF_ID	    att_defaultID;	   /* default id */
    char	    *att_default;	   /* points at storage for the default,
					   ** null if def tuple not read yet */
    int         att_intlid;            /* internal attribute id not altered
                                           ** by any drop column ajustments that
                                           ** effect att_number.
                                           */
    int		att_collID;	       /* collation ID */
    int         attr_srid;             /* Spatial reference ID */
    short       attr_geomtype;         /* Geometry type code */
    short       att_encflags;
    int         att_encwid; 
}   RAAT_ATT_ENTRY;

/*}
** Name: RAAT_IDX_ENTRY - Structure for table index information.
**
** Description:
**      This typedef defines the structure needed for the index information.
**      If index information is requested in the table show call,
**      a RAAT_IDX_ENTRY structure is returned in the buffer pointed to by
**      RAAT_SHW_CB.dmt_index_array.
**
**      Note:  Included structures are defined in DMF.H if they start with
**      DM_, otherwise they are defined as global DBMS types.
##
## History:
##	27-apr-95 (stephenb)
##	    Copied from DM_IDX_ENTRY and changed to standard datatypes.
##	18-nov-96 (hanch04)
##	    Update structure to include idx_pgsize
*/
typedef struct _RAAT_IDX_ENTRY
{
    char         idx_name[RAAT_MAXNAME];      /* Internal name of index. */    
    RAAT_TAB_ID    idx_id;                 /* Internal table identifier */
    int      idx_attr_id[RAAT_MAXKEYS];/* Ordinal number of attribute. */
    int      idx_array_count;        /* Number of attributes in index. */
    int	 idx_key_count;	    /* Number of keys in index. */
    int      idx_dpage_count;        /* Number of data pages in index. */
    int      idx_ipage_count;        /* Number of index pages 
                                            ** in index. */
    int      idx_status;
#define		RAAT_I_STATEMENT_LEVEL_UNIQUE	0x01L /* Does this index support
						      ** a statement level
						      ** unique index index
						      */
#define RAAT_I_INCONSISTENT            0x02L /* Index is inconsistent with
                                            ** corresponding base table */
    int      idx_storage_type;       /* Storage structure. */
    int		 idx_pgsize;             /* index page size    */

}   RAAT_IDX_ENTRY;

/*}
** Name: RAAT_TBL_ENTRY - Structure returning table information  for DMT_SHOW.
**
** Description:
**      This typedef defines the structure needed to describe a table.
**      If table information is requested in the table describe call, the
**      RAAT_TBL_ENTRY structure is returned in the buffer pointed to by
**      RAAT_SHOW_CB.dmt_table.
**
**      Note:  Included structures are defined in DMF.H if they start with
**      DM_, otherwise they are defined as global DBMS types.
##
## History:
##	27-apr-95 (stephenb)
##	    copied from DM_TBL_ENTRY and changed to generic datatypes.
##	18-nov-96 (hanch04)
##	    Update structure to include RTree
##      26-may-98 (shust01)
##          Added tbl_cachepri to RAAT_TBL_ENTRY to reflect change done
##          to DMT_TBL_ENTRY in dmtcb.h.  bug 91016.
##	13-Dec-04 (drivi01)
##	    Propagated change in structure DMT_TBL_ENTRY (change #466441) 
##	    to RAAT_TBL_ENTRY.
*/
typedef struct _RAAT_TBL_ENTRY
{
    RAAT_TAB_ID	    tbl_id;		         /* Table identifier. */
    char            tbl_name[RAAT_MAXNAME];      /* Table name. */
    char            tbl_owner[RAAT_OWN_MAXNAME]; /* Table owner. */
    int         tbl_loc_count;                   /* Count of locations. */
    char        tbl_location[RAAT_LOC_MAXNAME];  /* First Table location name.*/
    char        tbl_filename[RAAT_LOC_MAXNAME];  /* Table file name. */
    int         tbl_attr_count;         /* Table attribute count. */
    int         tbl_index_count;        /* Count of indexes on table. */
    int         tbl_width;              /* Table record width. */
    int         tbl_data_width;         /* Table logical data width. */
    int         tbl_storage_type;       /* Table storage structure. */
#define                RAAT_HEAP_TYPE        3L
#define                RAAT_ISAM_TYPE        5L
#define                RAAT_HASH_TYPE        7L
#define                RAAT_BTREE_TYPE       11L
    int         tbl_status_mask;         /* Table status. */
#define                RAAT_CATALOG          0x000001L
#define                RAAT_RD_ONLY          0x000002L
#define                RAAT_PROTECTION       0x000004L
#define                RAAT_INTEGRITIES      0x000008L
#define                RAAT_CONCURRENCY      0x000010L
#define                RAAT_VIEW             0x000020L
#define                RAAT_BASE_VIEW        0x000040L
#define                RAAT_IDX              0x000080L
#define                RAAT_BINARY           0x000100L
#define                RAAT_COMPRESSED       0x000200L
#define		       RAAT_INDEX_COMP	    0x000400L
					    /* RAAT_INDEX_COMP is on if the
					    ** table or index uses index
					    ** compression. Currently, only
					    ** Btrees support this.
					    */
#define                RAAT_ALL_PROT         0x001000L
                                            /* If this bit is set in
                                            ** tbl_status_mask then
					    ** you do not have all to all
                                            ** protection, if it is off
                                            ** then you do have all to all
                                            ** protection. */
#define                RAAT_RETRIEVE_PRO     0x002000L
                                            /* If this bit is set in
                                            ** tbl_status_mask then
					    ** you do not have ret to all
                                            ** protection, if it is off
                                            ** then you do have ret to all
                                            ** protection. */
#define                RAAT_EXTENDED_CAT     0x004000L
#define		       RAAT_JON		    0x008000L
#define                RAAT_UNIQUEKEYS       0x010000L
#define                RAAT_IDXD             0x020000L
#define                RAAT_JNL              0x040000L
#define                RAAT_ZOPTSTATS        0x100000L
#define                RAAT_DUPLICATES       0x200000L
#define                RAAT_MULTIPLE_LOC     0x400000L
#define                RAAT_GATEWAY          0x800000L
#define                RAAT_RULE	     0x01000000L /* Rules apply to tbl*/
#define		       RAAT_SYS_MAINTAINED 0x02000000L
#define		       RAAT_GW_NOUPDT	  0x04000000L
#define                RAAT_COMMENT        0x08000000L	/* comments on tbl or 
							** a col in the tbl */
#define		       RAAT_SYNONYM       0x10000000L	/* synonyms on tbl */
#define		       RAAT_VGRANT_OK	 0x20000000L
#define                RAAT_SECURE        0x40000000L
					    /*
					    ** owner may grant permits on this
					    ** view
					    */
    int        tbl_record_count;        /* Table estimated record count. */
    int        tbl_page_count;          /* Estimated page count in table. */
    int        tbl_dpage_count;         /* Estimated data page count 
                                            ** in table. */
    int        tbl_opage_count;         /* Estimated overflow page count
                                            ** in table */
    int        tbl_ipage_count;         /* Estimated index page count 
                                            ** in table. */
    int        tbl_lpage_count;         /* Estimated leaf page count */
    int        tbl_alloc_pg_count;      /* Estimated of the total number
					    ** of pages allocated to the
					    ** table
					     */
    int        tbl_tperpage;            /* Estimated tuples per page */
    int        tbl_kperpage;            /* Estimated keys per page */
    int        tbl_kperleaf;            /* Estimated keys per leaf */
    int        tbl_ixlevels;            /* Estimated number of index levels */
    int	       tbl_expiration;          /* Table expiration date. */
    RAAT_TAB_TIMESTAMP
                   tbl_date_modified;       /* Table last modified date. */
    int     
		   tbl_create_time;         /* Time table created. */
    RAAT_QRY_ID      tbl_qryid;             /* Associated query id of query
                                            ** text for views. 
					    ** Since the qryid field is
					    ** meaningless for RAAT which only
					    ** works on base tables, this will
					    ** used to pass a version number
					    ** between back and front to allow
					    ** determination of available calls
					    ** from products like ManMan that
					    ** may be at a newer level than
					    ** the backend.
					    */
#define                RAAT_VERSION_2       2 /* added RAAT_BAS_RECDATA */
#define                RAAT_VERSION_3       3 /* changes for INTERNAL_REPOS */

    int        tbl_struct_mod_date;     /* Date structure was modified. */
    int        tbl_i_fill_factor;       /* Fill factor for index pages. */
    int	   tbl_d_fill_factor;       /* Fill factor for data pages. */
    int        tbl_l_fill_factor;       /* Fill factor for BTREE leaf
                                            ** pages. */
    int        tbl_min_page;            /* Minimum number of pages 
                                            ** specified for modify. */
    int        tbl_max_page;            /* Maximum number of pages 
                                            ** specified for modify. */
    int	   tbl_relgwid;		    /* gateway id */
    int	   tbl_relgwother;	    /* other, gateway-specific info. */
    int	    tbl_temporary;	    /* non-zero means that this table
					    ** is a temporary table */
    int		   tbl_comp_type;	    /* Compression scheme */
#define		       RAAT_C_NONE	  0L
#define		       RAAT_C_STANDARD	  1L
#define		       RAAT_C_OLD	  2L
    int		   tbl_pg_type;		    /* The page type for this table */
#define		       RAAT_PG_STANDARD	  0L
#define		       RAAT_PG_SYSCAT	  2L
    int	   tbl_2_status_mask;
#define		       RAAT_TEXTENSION	    0x00000001L
					    /*
					    ** Set at table creation time if
					    ** this table is an extension of
					    ** another table -- currently for
					    ** support of peripheral attributes.
					    */
#define		       RAAT_HAS_EXTENSIONS   0x00000002L
					    /*
					    ** Set at table creation time if
					    ** this table has any extensions
					    */
#define RAAT_PERSISTS_OVER_MODIFIES 0x00000004L /* persists across modifies*/
#define RAAT_SUPPORTS_CONSTRAINT    0x00000008L /* this table/index directly
						** supports a UNIQUE constraint
						** and certain characteristics
						** cannot be changed
						*/
#define RAAT_SYSTEM_GENERATED	    0x00000010L /* created by the system */
#define RAAT_NOT_DROPPABLE	    0x00000020L /* not user-droppable */
#define RAAT_STATEMENT_LEVEL_UNIQUE 0x00000040L /* check uniqueness at stmt end */
#define RAAT_READONLY		    0x00000080L /* Readonly Table */	
#define RAAT_CONCURRENT_ACCESS      0x00000100L
					/*
					** Used by an Index, to indicate that
					** it is being created in concurrent
					** access mode
					*/
#define RAAT_ROW_SEC_AUDIT	    0x00002000L /* Table has row security
						** auditing
						*/
#define RAAT_ALARM                   0x00004000L /* Table may have security
						** alarms.
						*/
#define RAAT_LOGICAL_INCONSISTENT    0x00010000L /* Table has log. incons.
                                                */
#define RAAT_PHYSICAL_INCONSISTENT    0x00020000L /* Table has phy. incons.
                                                */
#define RAAT_INCONSIST (DMT_LOGICAL_INCONSISTENT | DMT_PHYSICAL_INCONSISTENT)
    int            tbl_allocation;	     /* number of pages to initially
					     ** allocate to the table */
    int            tbl_extend;               /* number of pages to add when
					     ** extending the table */
    int            tbl_pgsize;
    int        tbl_attr_high_colno;      /* highest internal column id
                                             ** obtained from att_intl_id
                                             */
 
    short	   tbl_nparts;		     /*Number of physical partitions */
    short	   tbl_ndims;	             /*Number of partitioning*/
    short          tbl_encflags;             /* encryption flags .. */
#define                 DMT_ENCRYPTED   0x0001L /* .. must match relencflags! */
#define                 DMT_AES128      0x0002L
#define                 DMT_AES192      0x0004L
#define                 DMT_AES256      0x0008L
    short          tbl_dimension;            /* RTree dimension         */
    short          tbl_hilbertsize;          /* RTree Hilbertsize       */
    short          tbl_rangesize;            /* RTree Range Size        */
    short          tbl_cachepri;             /* Cache priority          */
    short          tbl_rangedt;              /* RTree Range Data type   */
#define RAAT_MAXRTREE_DIM 4		     /* Max # RTREE dimensions */
    double         tbl_range[RAAT_MAXRTREE_DIM * 2]; /* RTree range */
}   RAAT_TBL_ENTRY;
/*
** Name: RAAT_TABLE_HANDLE
**
** Description:
**	Handle to open table, contains needed table information
##
## History:
##	14-apr-95 (stephenb)
##	    Written as a shell (placeholder) to enable compilation
##	18-apr-95 (lewda02)
##	    Add some vital fields for table information.
##	19-apr-95 (lewda02)
##	    Define flag to indicate open table.
##	 6-jun-95 (shust01)
##	    Define flags, variables needed for prefetching.
##	16-oct-96 (cohmi01)
##	    Add flag to indicate availability of base table data via sec.
*/
typedef struct _RAAT_TABLE_HANDLE
{
    SIZE_TYPE		table_rcb;	/* RCB pointer needed by back-end */
    char		table_name[RAAT_MAXNAME];  /* name of table */
    int			table_status;	/* status bit flags */
#define			TABLE_OPEN		0x00000001L
#define			TABLE_PREFETCH  	0x00000002L
#define			TABLE_CURRFETCH 	0x00000004L
#define			TABLE_PREV_BUFF 	0x00000008L
#define			TABLE_BAS_RECDATA	0x00000010L
#define         	TABLE_SEND_BKEY         0x00000020L
    char		*fetch_buffer;
    int			fetch_size;
    int			fetch_pos;
    int			fetch_actual;
    int			fetch_error;
    RAAT_TBL_ENTRY	table_info;	/* iirelation tuple for table */
    RAAT_ATT_ENTRY	*table_att;	/* iiattributes tuples for table */
    RAAT_IDX_ENTRY	*table_idx;	/* iiindex tuples for table */
} RAAT_TABLE_HANDLE;

/*
** Name: RAAT_KEY_ATTR
**
** Description:
**	This structure describes each key attribute used in keyed access 
**	to tables, an array of 'n' of these will be used to uniquely
**	describe a key, the RAAT_KEY structure is used to reference this array.
##
## History:
##	10-apr-95 (stephenb)
##	    Initial creation.
*/
typedef struct  _RAAT_KEY_ATTR           /* key attribute structure */
{
    int         attr_number;            /* col number in table of this key */
    int         attr_operator;          /* comparison operator */
#define         RAAT_EQ                  1 /* get equal to key */
#define         RAAT_LTE                 2 /* get less than or equal to key */
#define         RAAT_GTE                 3 /* get greater than or equal to key*/
    char        *attr_value;            /* value to compare */
} RAAT_KEY_ATTR;
/*
** Name: RAAT_KEY
**
** Description:
**	This structure is a place holder for they array of key attributes
**	used to perform keyed access.
##
## History:
##	10-apr-95 (stephenb)
##	    Initial Creation.
*/
typedef struct  _RAAT_KEY                /* array of key attributes */
{
    RAAT_KEY_ATTR        *key_attr;      /* pointer to arry of key attributes */
    int                 attr_count;     /* number of attributes in array */
} RAAT_KEY;

/*
** Name: RAAT_CB - RAAT control block
**
** Description:
**	RAAT control block, this control block contains all the information
**	needed to call the RAAT from a user program.
##
## History:
##	3-apr-95 (stephenb)
##	    First written.
##	27-apr-95 (lewda02)
##	    Changed some fields from pointer to long.
##	19-may-95 (lewda02/shust01)
##	    Added a mask for access to only the lock flags.
##	 7-jun-95 (shust01)
##	    Added an internal flag for repositioning.
##          Added row_request - how many rows user wishes to prefetch
##          Added row_actual  - how many rows actually prefetched
##	29-sep-95 (thaju02)
##	    Added definition of RAAT_TEMP_TBL for implementing 
##	    temporary tables.
##	16-oct-96 (cohmi01)
##	    Add flag to request base data directly from sec index.       
##      28-may-97 (dilma04)
##          Added row locking flags.
*/
typedef struct _RAAT_CB
{
    char        	*dbname;
    char        	*username;
    char        	*password;
    SIZE_TYPE	     	session_id;
    char        	*tabname;
    RAAT_TABLE_HANDLE	*table_handle;
    char        	*record;
    RAAT_KEY		*key;
    int			length;
    int			recnum;
    int			flag;
/* Transaction flags */
#define         RAAT_TX_READ		0x0001L /* this is a read transaction */
#define         RAAT_TX_WRITE           0x0002L /* This is a write transaction*/
/* Locking Flags */
#define         RAAT_LK_XT              0x0004L /* Take exclusive table lock */
#define         RAAT_LK_ST              0x0008L /* take a shared table lock */
#define         RAAT_LK_XP              0x0010L /* take exclusive page locks */
#define         RAAT_LK_SP              0x0020L /* take shared page locks */
#define         RAAT_LK_XR              0x0040L /* take exclusive row locks */
#define         RAAT_LK_SR              0x0080L /* take shared row locks */
#define         RAAT_LK_NL              0x0100L /* do not take locks */
#define		RAAT_LK_MASK	(RAAT_LK_XT|RAAT_LK_ST|RAAT_LK_XP|RAAT_LK_SP|RAAT_LK_XR|RAAT_LK_SR|RAAT_LK_NL)
/* Index flags */
#define         RAAT_BAS_RECNUM         0x0200L /* use index to return 
					       ** base table pos */
/* record flags */

#define         RAAT_REC_CURR           0x0400L /* get current record */
#define         RAAT_REC_NEXT           0x0800L /* get next record */
#define         RAAT_REC_PREV           0x1000L /* get previous record */
#define		RAAT_REC_BYNUM		0x2000L /* get by record number */
#define		RAAT_REC_REPOS		0x4000L /* repos to last 
					       ** RAAT_RECORD_POSITION */
#define		RAAT_REC_FIRST		0x8000L /* position to first record */
#define		RAAT_REC_LAST          0x10000L /* position to last record */
/* Table Flags */
#define		RAAT_TBL_INFO	       0x20000L /* get table info on open */
#define		RAAT_TEMP_TBL          0x40000L /* temporary table */

/* more index flags */
#define		RAAT_BAS_RECDATA       0x80000L /* get data from base table */

/* internal flags */
#define         RAAT_INTERNAL_BKEY  0x40000000L /* internal use only */
#define		RAAT_INTERNAL_REPOS 0x80000000L /* internal use only */
    int			row_request;
    int			row_actual;
    int			err_code;
    char 		*internal_buf;
    int			internal_buf_size;
    RAAT_TABLE_HANDLE	*bas_table_handle;	/* only if RAAT_BAS_RECDATA */
} RAAT_CB;

typedef struct _RAAT_BLOB_CB
{
    char       	*coupon;		/* pointer to coupon */
    char       	*data;
    int		data_len;
    int		datatype;
    char       	*work_area;
    int		continuation;		/* continuation flag */
    int		end_of_data;
    int		actual_len;
} RAAT_BLOB_CB;


int IIraat_session_start(RAAT_CB *);
int IIraat_session_end(RAAT_CB *);
int IIraat_tx_begin(RAAT_CB *);
int IIraat_tx_commit(RAAT_CB *);
int IIraat_tx_abort(RAAT_CB *);
int IIraat_table_open(RAAT_CB *);
int IIraat_table_lock(RAAT_CB *);
int IIraat_table_close(RAAT_CB *);
int IIraat_record_get(RAAT_CB *);
int IIraat_record_put(RAAT_CB *);
int IIraat_record_position(RAAT_CB *);
int IIraat_record_delete(RAAT_CB *);
int IIraat_record_replace(RAAT_CB *);

int IIcraat_blob_put(RAAT_CB *, RAAT_BLOB_CB *);
int IIcraat_blob_get(RAAT_CB *, RAAT_BLOB_CB *);

int IIcraat_strtodate(char *, char *);
int IIcraat_datetostr(char *, char *, int);
int IIcraat_moneytostr(char *, char *, int);
int IIcraat_strtomoney(char *, char *);

int IIcraat_init(RAAT_CB *);
int IIcraat_end(RAAT_CB *);
int allocate_big_buffer(RAAT_CB *, int);
