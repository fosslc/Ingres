/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: RDFDDB.H - DDB data structures use by RDF.
**
** Description:
**      This file contains structures used by RDF in distributed mode.
**
** History:
**      30-aug-88 (mings)
**	    initial creation.
**	14-jun-91 (teresa)
**	    define RDD_TID_LEVEL for bug 34684.
**	14-apr-92 (teresa)
**	    remove RDD_DDB_DESC, RDD_LDB_DESC for SYBIL.
**	14-jul-92 (fpang)
**	    Fixed compiler warnings.
**	01-dec-92 (teresa)
**	    define RDD_18_COL.
**	18-jan-93 (rganski)
**	    Added 4 new fields to RDC_STATS: column_domain, is_complete,
**	    stat_version and hist_data_length. These correspond to existing
**	    sdomain and scomplete columns and new sversion and
**	    shistlength columns of iistatistics. For Character Histogram
**	    Enhancements project.
**	02-mar-93 (barbara)
**	    Defined RDD_OPENSQL_LEVEL and RDD_DELIMITED_NAME_CASE for Star
**	    support of delimited ids.
**	02-sep-93 (barbara)
**	    Defined RDD_REAL_USER_CASE for new capability DB_REAL_USER_CASE.
**	06-mar-96 (nanpr01)
**	    Defined RDD_51_COL for iidd_tables catalogue change for passing
**	    pagesize. Also bump up the RDD_QRY_LENGTH to 1100 so that a 
**	    qualified select can be sent instead of select *.
**	25-jul-96 (ramra01)
**	    Defined RDD_53_COL for iidd_tables catalog change for passing
**	    relversion and reltotwid as part of alter table add/drop col.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**      01-oct-2010 (stial01) (SIR 121123 Long Ids)
**          Store blank trimmed names in DMT_ATT_ENTRY
**/

/*
**  Forward and/or External typedef/struct references.
*/

typedef struct _RDD_OBJ_ID  		RDD_OBJ_ID;
typedef struct _RDC_TABLES		RDC_TABLES;
typedef struct _RDC_COLUMNS      	RDC_COLUMNS;
typedef struct _RDC_VIEWS               RDC_VIEWS;
typedef struct _RDC_INDEXES		RDC_INDEXES;
typedef struct _RDC_IDX_COLUMNS		RDC_IDX_COLUMNS;
typedef struct _RDC_STATS		RDC_STATS;
typedef struct _RDC_HISTOGRAMS		RDC_HISTOGRAMS;
typedef struct _RDC_KEY_COLUMNS	        RDC_KEY_COLUMNS;
typedef struct _RDC_LDBID		RDC_LDBID;
typedef struct _RDC_DBCAPS		RDC_DBCAPS;
typedef struct _RDC_MAP_COLUMNS		RDC_MAP_COLUMNS;
typedef struct _RDC_OBJECTS		RDC_OBJECTS;
typedef struct _RDC_LOCTABLE		RDC_LOCTABLE;
typedef struct _RDC_TREE		RDC_TREE;
typedef struct _RDC_DBDEPENDS		RDC_DBDEPENDS;
typedef struct _RDD_RCB			RDD_RCB;
typedef	struct _RDD_DEPENDS		RDD_DEPENDS;

/*
**	Max number of DDB.
*/
#define RDD_MAXDDB                      20

/*
**	Max number of LDB.
*/
#define RDD_MAXLDB		        100

/*
**	Max number of retry when error occurs.
*/
#define RDD_MAXRETRY		        15

/*
**	Definition of object types
*/
#define RDD_NO_OBJ			7

/*
**      Max length of query text.
*/
#define RDD_QRY_LENGTH (1100 + DB_TAB_MAXNAME + DB_ATT_MAXNAME + DB_OWN_MAXNAME)

/*
**      Max number of ldb capabilities currently supported.
*/
#define RDD_MAXCAPS                     15

/*
**      iidbcapabilities.cap_capability.  Each constant indicates an offset
**	    into Iird_caps lookup table where the cap_capability name is 
**	    defined.
*/
#define RDD_DISTRIBUTED			0   /* Iird_caps[0]*/
#define RDD_DB_NAME_CASE		1   /* Iird_caps[1]*/
#define RDD_UNIQUE_KEY_REQ		2   /* Iird_caps[2]*/
#define RDD_CAP_INGRES			3   /* Iird_caps[3]*/
#define RDD_QUEL_LEVEL                  4   /* Iird_caps[4]*/
#define RDD_SQL_LEVEL			5   /* Iird_caps[5]*/
#define RDD_COMMON_SQL_LEVEL		6   /* Iird_caps[6]*/
#define RDD_SAVEPOINTS		        7   /* Iird_caps[7]*/
#define RDD_DBMS_TYPE			8   /* Iird_caps[8]*/
#define RDD_OWNER_NAME			9   /* Iird_caps[9]*/
#define RDD_PHYSICAL_SOURCE		10   /* Iird_caps[10]*/
#define	RDD_TID_LEVEL			11   /* Iird_caps[11]*/
#define	RDD_OPENSQL_LEVEL		12   /* Iird_caps[12]*/
#define	RDD_DELIMITED_NAME_CASE		13   /* Iird_caps[13]*/
#define RDD_REAL_USER_CASE		14   /* Iird_caps[14]*/
/* NOTE:  when you add a constant to this section, be sure to bump RDD_MAXCAPS*/


/*
**      Number of columns.
*/
#define RDD_01_COL			1
#define RDD_02_COL			2
#define RDD_03_COL                      3
#define RDD_04_COL			4
#define RDD_05_COL                      5
#define RDD_06_COL			6
#define RDD_07_COL			7
#define RDD_08_COL		        8
#define RDD_09_COL			9
#define RDD_10_COL			10
#define RDD_11_COL			11
#define RDD_12_COL                      12
#define RDD_17_COL			17
#define RDD_18_COL			18
#define RDD_19_COL			19
#define RDD_20_COL			20
#define RDD_50_COL			50
#define RDD_51_COL			51
#define RDD_53_COL			53

/*
**      Type of user.
*/

#define RDD_UNKNOWN			0
#define RDD_INGRES			1
#define RDD_DBA	                        2
#define RDD_USER			3

/*
**      Type of request.
*/
#define RDD_BY_NAME			1
#define RDD_BY_ID			2

/*
**     Number fo Star catalogs
*/
#define RDD_NO_CATALOGS			33

/*
**     Number assigned to each catalog
*/
#define RDD_0_ALTERNAT_KEYS		0
#define RDD_1_ALT_COLUMNS		1
#define RDD_2_COLUMNS  			2
#define RDD_3_DBCAPABILITIES		3
#define RDD_4_DBCONSTANTS		4
#define RDD_5_DDB_DBDEPENDS		5
#define RDD_6_DDB_LDBIDS		6
#define RDD_7_DDB_LDB_COLUMNS		7
#define RDD_8_DDB_LDB_DBCAPS		8
#define RDD_9_DDB_LDB_INDEXES		9
#define RDD_10_DDB_LDB_INGTABLES	10
#define RDD_11_DDB_LDB_KEYCOLUMNS	11
#define RDD_12_DDB_LDB_PHYTABLES	12
#define RDD_13_DDB_LDB_XCOLUMNS		13
#define RDD_14_DDB_LDB_LDBNAMES		14
#define RDD_15_DDB_OBJECTS		15
#define RDD_16_DDB_OBJECT_BASE		16
#define RDD_17_DDB_QRYTEXT		17
#define RDD_18_DDB_TABLEINFO		18
#define RDD_19_DDB_TREE			19
#define RDD_20_HISTOGRAMS		20
#define RDD_21_INDEXES			21
#define RDD_22_INDEX_COLUMNS		22
#define RDD_23_INGRES_TABLES		23
#define RDD_24_INTEGRITIES		24
#define RDD_25_KEY_COLUMNS		25
#define RDD_26_MULTI_LOCATIONS		26
#define RDD_27_PERMITS			27
#define RDD_28_PHYSICAL_TABLES		28
#define RDD_29_PROCEDURES		29
#define RDD_30_STATS			30
#define RDD_31_TABLES			31
#define RDD_32_VIEWS			32

/*
**	Length of char strings
*/
#define	RDD_1_CHAR			1
#define RDD_8_CHAR			8
#define RDD_16_CHAR			16

/*
**	typedef of char arrays
*/
typedef char		RDD_1CHAR[RDD_1_CHAR];
typedef char		RDD_8CHAR[RDD_8_CHAR];
typedef char		RDD_16CHAR[RDD_16_CHAR];

/*}
** Name: RDD_OBJ_ID - descriptor identify object.
**
** Description:
**      This structure contains object name, object owner with null terminates.
**
** History:
**      30-aug-88 (mings)
*/
struct _RDD_OBJ_ID
{
    i4		    name_length;               /* name length */ 
    char  	    tab_name[DB_TAB_MAXNAME + 1];  /* object name */
    i4		    owner_length;	       /* owner length */
    char	    tab_owner[DB_OWN_MAXNAME + 1]; /* object owner */
};

/*}
** Name: RDC_TABLES - structure defines iidd_tables
**
** Description:
**      This structure defines iidd_tables field by field.
**
** History:
**      13-sep-88 (mings)
**	06-mar-96 (nanpr01)
**	   Added table_pagesize for variable page size project.
*/
struct _RDC_TABLES
{
	DD_TAB_NAME	table_name;
	DD_OWN_NAME	table_owner;
	DD_DATE		create_date;
	DD_DATE		alter_date;
	RDD_8CHAR	table_type;
	RDD_8CHAR	table_subtype;
	RDD_8CHAR	table_version;
	RDD_8CHAR	system_use;
        RDD_8CHAR	stats;
	RDD_8CHAR	indexes;
	RDD_8CHAR	rdonly;
	i4        	num_rows;
	RDD_16CHAR	storage;
	RDD_8CHAR	compressed;
	RDD_8CHAR	duplicate_rows;
	RDD_8CHAR	uniquerule;
	i4  		number_pages;
	i4  		overflow;
	i4  		row_width;
        i4		expire_date;
	DD_DATE  	modify_date;
	DB_LOC_NAME	location_name;
	RDD_8CHAR	integrities;
	RDD_8CHAR	permits;
	RDD_8CHAR	alltoall;
	RDD_8CHAR	rettoall;
	RDD_8CHAR	journal;
	RDD_8CHAR	view_base;
	RDD_8CHAR	multi_location;
	i2  		ifillpct;
	i2  		dfillpct;
	i2  		lfillpct;
	i4  		minpages;
	i4  		maxpages;
	u_i4		stamp1;
	u_i4		stamp2;
	i4  		reltid;
	i4  		reltidx;	    
	i4		table_pagesize;
	i2		table_relversion;
	i4		table_reltotwid;
};

/*}
** Name: RDC_COLUMNS - structure defines iidd_columns.
**
** Description:
**      This structure defines iidd_columns field by field.
**
** History:
**      13_sep_88 (mings)
*/
struct _RDC_COLUMNS
{
    DD_TAB_NAME	table_name;
    DD_OWN_NAME	table_owner;
    DD_ATT_NAME	name;
    char	datatype[DB_TYPE_MAXLEN];
    i4  	length;
    i4  	scale;
    RDD_8CHAR	nulls;
    RDD_8CHAR	defaults;
    i4  	key_sequence;
    i4  	sequence;
    RDD_8CHAR	sort_direction;
    i2		ingdatatype;
};

/*}
** Name: RDC_VIEWS - structure defines iidd_views.
**
** Description:
**	This structure defines iidd_views field by field.
**
** History:
**      13-sep-88 (mings)
*/
struct _RDC_VIEWS
{
    DD_TAB_NAME	table_name;
    DD_OWN_NAME	table_owner;
    RDD_8CHAR	dml;
    RDD_8CHAR	check_option;
    i4  	sequence;
    char	text[243];
};

/*}
** Name: RDC_INDEXES - structure defines iidd_indexes.
**
** Description:
**      This structure defines iidd_indexes field by field.
**
** History:
**      13-SEP-88 (mings)
*/
struct _RDC_INDEXES
{
    DD_TAB_NAME	index_name;
    DD_OWN_NAME	index_owner;
    DD_DATE	create_date;
    DD_TAB_NAME	base_name;
    DD_OWN_NAME	base_owner;
    RDD_16CHAR	storage;
    RDD_8CHAR	compressed;
    RDD_8CHAR	uniquerule;
    i4		index_pagesize;
};

/*}
** Name: RDC_IDX_COLUMNS  - structure defines iidd_index_columns.
**
** Description:
**      This structure defines iidd_index_columns field by field.
**
** History:
**      13-sep-88 (mings)
*/
struct _RDC_IDX_COLUMNS
{
    DD_TAB_NAME	index_name;
    DD_OWN_NAME	index_owner;
    DD_ATT_NAME	column_name;
    i4  	keysequence;
    RDD_8CHAR	direction;
};

/*}
** Name: RDC_STATS - structure defines iidd_stats.
**
** Description:
**      This structure defines fields in iidd_stats.
**
** History:
**      13-sep-88 (mings)
**	18-jan-93 (rganski)
**	    Added 4 new fields to iidd_stats: column_domain, is_complete,
**	    stat_version and hist_data_length. These correspond to existing
**	    sdomain and scomplete columns and new sversion and
**	    shistlength columns of iistatistics. For Character Histogram
**	    Enhancements project.
*/
struct _RDC_STATS
{
    DD_TAB_NAME	table_name;
    DD_OWN_NAME	table_owner;
    DD_ATT_NAME	column_name;
    DD_DATE	create_date;
    f4		num_unique;
    f4		rept_factor;
    RDD_8CHAR	has_unique;
    f4		pct_nulls;
    i2  	num_cells;
    i2		column_domain;
    RDD_8CHAR	is_complete;
    RDD_8CHAR	stat_version;
    i2		hist_data_length;
};

/*}
** Name: RDC_HISTOGRAMS - structure defines fields in iidd_histograms.
**
** Description:
**      This structure defines fields in iidd_histograms.
**
** History:
*/
struct _RDC_HISTOGRAMS
{
    DD_TAB_NAME	table_name;
    DD_OWN_NAME	table_owner;
    DD_ATT_NAME	column_name;
    i2  	sequence;
    char	optdata[228];
};

/*}
** Name: RDC_KEY_COLUMNS - structure defines iidd_key_columns.
**
** Description:
**      This structure defines fields in iidd_key_columns.
**
** History:
**      13-sep-88 (mings)
*/
struct _RDC_KEY_COLUMNS
{
    DD_TAB_NAME	table_name;
    DD_OWN_NAME	table_owner;
    DD_ATT_NAME	column_name;
    i2  	keysequence;
    RDD_1CHAR	direction;
};

/*}
** Name: RDC_LDBID - structure defines iidd_ddb_ldbids.
**
** Description:
**      This structure defines fields in iidd_ddb_ldbids.
**
** History:
**      13-sep-88 (mings)
*/
struct _RDC_LDBID
{
    DD_NODE_NAME	ldb_node;
    char	ldb_dbms[DB_TYPE_MAXLEN];
    DD_DB_NAME	ldb_database;
    RDD_8CHAR	ldb_longname;
    i4     ldb_id;
    RDD_8CHAR	ldb_dba;
    DD_OWN_NAME	ldb_dbaname;
    DD_OWN_NAME	ldb_sys_owner;
};

/*}
** Name: RDC_DBCAPS - structure defines iidd_ddb_ldb_dbcaps.
**
** Description:
**      This structure defines fields in iidd_ddb_ldb_dbcaps.
**
** History:
**      14-jan-89 (mings)
*/
struct _RDC_DBCAPS
{
    i4	ldb_id;
    char	cap_capability[DB_CAP_MAXLEN + 1];
    char	cap_value[DB_CAPVAL_MAXLEN + 1];
    i4	cap_level;
};

/*}
** Name: RDC_DDB_COLUMNS - structure defines iidd_ddb_ldb_columns.
**
** Description:
**      This structure defines fields in iidd_ddb_ldb_columns.
**
** History:
**      13-sep-88 (mings)
*/
struct _RDC_MAP_COLUMNS
{
    u_i4    	object_base;
    u_i4	object_index;
    DD_ATT_NAME	local_column;
    i4          column_sequence;
};

/*}
** Name: RDC_OBJECTS - structure defines iidd_ddb_objects.
**
** Description:
**      This structure defines fields in iidd_ddb_objects catalog.
**
** History:
**      13-sep-88 (mings)
*/
struct _RDC_OBJECTS
{
    DD_OBJ_NAME	object_name;
    DD_OWN_NAME	object_owner;
    u_i4	object_base;
    u_i4        object_index;
    u_i4	qid1;
    u_i4	qid2;
    DD_DATE	create_date;
    RDD_8CHAR	object_type;
    DD_DATE	alter_date;
    RDD_8CHAR	system_object;
    RDD_8CHAR	to_expire;
    DD_DATE	expire_date;
};

/*}
** Name: RDC_LOCTABLE - structure defines iidd_ddb_tableinfo.
**
** Description:
**      This structure defines fields in iidd_ddb_tableinfo catalog.
**
** History:
**      13-sep-88 (mings)
*/
struct _RDC_LOCTABLE
{
    u_i4	object_base;
    u_i4	object_index;
    RDD_8CHAR	local_type;
    DD_TAB_NAME table_name;
    DD_OWN_NAME	table_owner;
    DD_DATE	table_date;
    DD_DATE	table_alter;
    u_i4	stamp1;
    u_i4	stamp2;
    RDD_8CHAR	columns_mapped;
    i4	ldb_id;
};

/*}
** Name: RDC_TREE - structure defines iidd_ddb_tree.
**
** Description:
**      This structure defines fields in iidd_ddb_tree catalog.
**	Note that this structure is different from DB_IITREE on
**	varchar field.
**
** History:
**      13-sep-88 (mings)
*/
struct _RDC_TREE
{
    i4		treetabbase;
    i4		treetabidx;
    i4		treeid1;
    i4	        treeid2;
    i2		treeseq;
    i2		treemode;
    i2		treevers;
    char	treetree[1027];	/* this is varchar type.
				** RQF will strip off first two 
				** bytes for field size and null
				** terminated. 1024+3 */
};

/*}
** Name: RDC_DBDEPENDS - structure defines iidd_ddb_dbdepends.
**
** Description:
**      This structure defines fields in iidd_ddb_dbdepends catalog.
**
** History:
**      13-sep-88 (mings)
*/
struct _RDC_DBDEPENDS
{
    i4		inid1;
    i4		inid2;
    i4		itype;
    i4	        deid1;
    i4		deid2;
    i4		dtype;
    i4		qid;
};

/*}
** Name: RDD_RCB - distributed information request block.
**
** Description:
**      This structure contains information about distributed status.
**
** History:
**      30-aug-88 (mings)
*/
struct _RDD_RCB
{
    i4	    rdd_types_mask;
#define			RDD_OBJECT	0x0001L
				    /* retrieve object info */
#define			RDD_ATTRIBUTE   0x0002L
				    /* retrieve attribute info */
#define			RDD_MAPPEDATTR  0x0004L
				    /* retrieve mapped attribute info */
#define                 RDD_INDEX       0x0008L
				    /* retrieve index info */
#define			RDD_STATISTICS  0x0010L
				    /* retrieve statistics info */
#define			RDD_HSTOGRAM    0x0020L
				    /* retrieve statistics info */
#define			RDD_VTUPLE      0x0040L
				    /* retrieve view tuple */
    i4	    rdd_ddstatus;   
				    /* distributed status */
#define			RDD_FETCH	0x0001L
				    /* tuple fetching */
#define			RDD_SEMAPHORE	0x0002L
				    /* holding semaphore */
#define			RDD_NOTFOUND	0x0004L
				    /* no tuple was found */
    DMT_ATT_ENTRY   **rdd_attr;	    /* pointer to array of pointers */
    char	    *rdd_attr_names; /* attr names */
    i4		    rdd_attr_nametot; /* size of name memory */
    DD_COLUMN_DESC  **rdd_mapattr;  /* pointer to array of pointers */
    DMT_IDX_ENTRY   **rdd_indx;	    /* pointer to array of pointers */
    RDD_HISTO	    *rdd_statp;     /* array of histo info */ 
    RQB_BIND	    *rdd_bindp;	    /* pointer to binding info */
    RDC_TREE	    *rdd_ttuple;    /* buffer for retrived tree tuple */
};

/*}
** Name: RDD_DEPENDS - structure defines a dependency object.
**
** Description:
**	This structure is used to store and link dependency objects.
**
** History:
**      13-sep-88 (mings)
*/
struct _RDD_DEPENDS
{
    RDD_DEPENDS	*rdd_next_p;	    /* pointer to next dependency object */
    DB_TAB_ID	rdd_tab_id;	    /* dependency object id */
};
