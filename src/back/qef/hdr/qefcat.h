/*
**Copyright (c) 2004 Ingres Corporation
*/

#ifndef QEFCAT_H_INCLUDED
#define QEFCAT_H_INCLUDED

/**
** Name: QEFCAT.H - Standard catalog structures for QEF.
**
** Description:
**      This file contains all the standard and STAR catalog structures.
**
** 	    QEC_1C_PLUS1	    - A 2-character array
** 	    QEC_8C_PLUS1	    - An 9-character array
** 	    QEC_16C_PLUS1	    - An 17-character array
** 	    QEC_24C_PLUS1	    - A 25-character structure
** 	    QEC_32C_PLUS1	    - A 33-character structure
** 	    QEC_228_PLUS1	    - A 229-character structure
** 	    QEC_256_PLUS1	    - A 257-character structure
** 	    QEC_D1_DBDEPENDS	    - Catalog IIDD_DDB_DBDEPENDS
** 	    QEC_D2_LDBIDS	    - Catalog IIDD_DDB_LDBIDS
** 	    QEC_D3_LDB_COLUMNS	    - Catalog IIDD_DDB_LDB_COLUMNS
** 	    QEC_D4_LDB_DBCAPS	    - Catalog IIDD_DDB_LDB_DBCAPS
** 	    QEC_D5_LONG_LDBNAMES    - Catalog IIDD_DDB_LONG_LDBNAMES
** 	    QEC_D6_OBJECTS	    - Catalog IIDD_DDB_OBJECTS
** 	    QEC_D7_OBJECT_BASE	    - Catalog IIDD_DDB_OBJECT_BASE
** 	    QEC_D9_TABLEINFO	    - Catalog IIDD_DDB_TABLEINFO
** 	    QEC_D10_TREE	    - Catalog IIDD_DDB_TREE
**	    QEC_L1_DBCONSTANTS	    - Catalog II[DD_]DBCONSTANTS
** 	    QEC_L2_ALT_COLUMNS	    - Catalog II[DD_]ALT_COLUMNS
** 	    QEC_L3_COLUMNS	    - Catalog II[DD_]COLUMNS
** 	    QEC_L4_DBCAPABILITIES   - Catalog II[DD_]DBCAPABILITIES
** 	    QEC_L5_HISTOGRAMS	    - Catalog II[DD_]HISTOGRAMS
** 	    QEC_L6_INDEXES	    - Catalog II[DD_]INDEXES
** 	    QEC_L7_INDEX_COLUMNS    - Catalog II[DD_]INDEX_COLUMNS 
** 	    QEC_L14_REGISTRATIONS   - Catalog II[DD_]REGISTRATIONS
** 	    QEC_L15_STATS	    - Catalog II[DD_]STATS 
** 	    QEC_L16_TABLES	    - Catalog II[DD_]TABLES 
** 	    QEC_L17_VIEWS	    - Catalog II[DD_]VIEWS
** 	    QEC_L18_PROCEDURES 	    - Catalog II[DD_]PROCEDURES
** 	    QEC_MIN_LDB_CAPS	    - For retrieving minimum levels 
**				      IIDD_DDB_LDB_DBCAPS
** 	    QEC_LDB_CAPS_PTRS	    - For accessing catalog IIDD_DBCAPABILITIES
** 	    QEC_INDEX_ID	    - Index id
** 	    QEC_LINK		    - Structure for creating or destroying a 
**				      link
**
** History: $Log-for RCS$
**      20-jul-88 (carl)
**          written
**      10-dec-88 (carl)
**	    modified for catalog definition changes
**      23-jan-89 (carl)
**	    added new fields to QEC_D2_LDBIDS
**      25-jan-89 (carl)
**	    added qec_m5_uniqkey to QEC_MIN_CAP_LVLS, QEC_06_UPPER_LDB
**	    to QEC_LINK
**      04-mar-89 (carl)
**	    added catalog column counts
**      06-apr-89 (carl)
**	    added qec_28_iistats_cnt to QEC_LINK
**      12-sep-89 (carl)
**	    added 7.0 new fields to QEC_L3_COLUMNS
**      28-sep-89 (carl)
**	    rollback 7.0 new field changes to QEC_L3_COLUMNS
**      17-oct-90 (carl)
**	    added comment to clarify usage of QEC_D2_LDBIDS.d2_3_ldb_database
**	14-jul-92 (fpang)
**	    Fixed compiler warnings.
**	17-may-93 (davebf)
**	    Put extra UDT fields back in QEC_L3_COLUMNS
**	23-jul-93 (rganski)
**	    Added new iistats columns to QEC_L15_STATS, and changed
**	    QEC_15I_CC_STATS_9 to QEC_15I_CC_STATS_13.
**	22-nov-93 (rganski)
**	    Added QEC_15I_CC_STATS_9, which is needed when LDB is pre-6.5.
**      27-jul-96 (ramra01)
**          Standard catalogue changes for alter table project. 
**	29-jan-1997 (hanch04)
**	    Changed QEC_NEW_STDCAT_LEVEL to 800
**	26-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Added reltcpri to IITABLES.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      09-jan-2009 (stial01)
**          Rename QEC_32C_PLUS1 -> QEC_DBNAMESTR
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**          changed l16_22_loc_name from 24C_PLUS1 to 33 sizeof(DB_LOC_NAME)+1
**	2-Dec-2010 (kschendel) SIR 124685
**	    Prototype fixes.
**/


/*}
** Name: QEC_1C_PLUS1 - A 2-character array.
**
** Description:
**      This structure defines a 2-character array.
**      
** History:
**      20-jul-88 (carl)
**          written
*/

typedef char	QEC_1C_PLUS1[1 + 1];


/*}
** Name: QEC_8C_PLUS1 - An 9-character array.
**
** Description:
**      This structure defines an 9-character array.
**      
** History:
**      20-jul-88 (carl)
**          written
*/

typedef char	QEC_8C_PLUS1[QEK_8_CHAR_SIZE + 1];


/*}
** Name: QEC_16C_PLUS1 - An 17-character array.
**
** Description:
**      This structure defines an 17-character array.
**      
** History:
**      20-jul-88 (carl)
**          written
*/

typedef char	QEC_16C_PLUS1[QEK_16_STOR_SIZE + 1];


/*}
** Name: QEC_24C_PLUS1 - A 25-character structure.
**
** Description:
**      This structure defines a 25-character array.
**      
** History:
**      20-jul-88 (carl)
**          written
*/

typedef char	QEC_24C_PLUS1[QEK_24_LOCN_SIZE + 1];

/*}
** Name: QEC_32C_PLUS1 - A (32+1) character structure.
**
** Description:
**      This structure defines a (32+1) character array.
**      
** History:
**      20-jul-88 (carl)
**          written
*/

typedef char	QEC_32C_PLUS1[32 + 1];


/*}
** Name: QEC_228_PLUS1 - A 229-character structure.
**
** Description:
**      This structure defines a 229-character array.
**      
** History:
**      20-jul-88 (carl)
**          written
**      04-mar-88 (carl)
**	    changed back to fixed character string
*/

typedef char	QEC_228_PLUS1[QEK_228_HIST_SIZE + 1];
						


/*}
** Name: QEC_240_PLUS1 - A 241-character structure.
**
** Description:
**      This structure defines a 241-character array.
**      
** History:
**      20-jul-88 (carl)
**          written
*/

typedef char	QEC_240_PLUS1[QEK_240_TEXT_SIZE + 2 + 1];

/*}
** Name: QEC_256_PLUS1 - A 257-character structure.
**
** Description:
**      This structure defines a 257-character array.
**      
** History:
**      20-jul-88 (carl)
**          written
*/

typedef char	QEC_256_PLUS1[QEK_256_VIEW_SIZE + 2 + 1];


/*}
** Name: QEC_1024_PLUS1 - A 1025-character structure.
**
** Description:
**      This structure defines a 1025-character array.
**      
** History:
**      20-jul-88 (carl)
**          written
*/

typedef char	QEC_1024_PLUS1[QEK_1024_TREE_SIZE + 2 + 1];


/*}
** Name: QEC_D1_DBDEPENDS - Catalog IIDD_DDB_DBDEPENDS.
**
** Description:
**      This structure matches the schema of IIDD_DDB_DBDEPENDS.
**      
** History:
**      20-jul-88 (carl)
**          written
*/

struct _QEC_D1_DBDEPENDS
{
    i4	    d1_1_inid1;		    /* independent object's id1 */
    i4	    d1_2_inid2;		    /* independent object's id2 */
    i4	    d1_3_itype;		    /* independent object's type, DB_TABLE (0),
				    ** DB_VIEW (17), DB_CRT_LINK (100) */
    i4	    d1_4_deid1;		    /* dependent object's id1 */
    i4	    d1_5_deid2;		    /* dependent object's id2 */
    i4	    d1_6_dtype;		    /* dependent object's type, DB_VIEW (17),
				    ** DB_PROT (19) */
    i4	    d1_7_qid;		    /* permit number if any, (used for dropping 
				    ** permit), 0 unless d1_6_dtype == DB_PROT 
				    */
#define	QEC_01D_CC_DBDEPENDS_7	7	
} ;


/*}
** Name: QEC_D2_LDBIDS - Catalog IIDD_DDB_LDBIDS.
**
** Description:
**      This structure matches the schema of IIDD_DDB_LDBIDS.
**      
** History:
**      20-jul-88 (carl)
**          written
**      23-jan-89 (carl)
**	    added new fields 
**      17-oct-90 (carl)
**	    added comment to clarify usage of d2_3_ldb_database
*/

struct _QEC_D2_LDBIDS
{
    DB_NODE_STR     d2_1_ldb_node;	/* node name */
    QEC_32C_PLUS1   d2_2_ldb_dbms;	/* dbms name */
    QEC_256_PLUS1   d2_3_ldb_database;	/* database name, alias is stored here
					** if the original name has more than 
					** 32 characters which would be stored 
					** in iidd_ddb_long_ldbnames */
    QEC_8C_PLUS1    d2_4_ldb_longname;	/* "Y" or "N" */
    i4		    d2_5_ldb_id;	/* assigned ldb id */
    QEC_8C_PLUS1    d2_6_ldb_user;	/* "Y" if notion of user name exists, 
					** "N" otherwise */
    QEC_8C_PLUS1    d2_7_ldb_dba;	/* "Y" if notion of DBA exists, "N" 
					** otherwise */
    DB_OWN_STR      d2_8_ldb_dbaname;	/* DBA's name if such notion exists */

#define	    QEC_02D_CC_LDBIDS_8	    8	
} ;


/*}
** Name: QEC_D3_LDB_COLUMNS - Catalog IIDD_DDB_LDB_COLUMNS.
**
** Description:
**      This structure matches the schema of IIDD_DDB_LDB_COLUMNS.
**      
**
** History:
**      20-jul-88 (carl)
**          written
*/

struct _QEC_D3_LDB_COLUMNS
{
    DB_TAB_ID	    d3_1_obj_id;	/* object id */
    DB_ATT_STR      d3_2_lcl_name;	/* local column name */
    i4		    d3_3_seq_in_row;	/* sequence of column in schema */

#define	QEC_03D_CC_LDB_COLUMNS_4    4	
} ;


/*}
** Name: QEC_D4_LDB_DBCAPS - Catalog IIDD_DDB_LDB_DBCAPS.
**
** Description:
**      This structure matches the schema of IIDD_DDB_LDB_DBCAPS.
**      
** History:
**      20-jul-88 (carl)
**          written
*/

struct _QEC_D4_LDB_DBCAPS
{
    i4		    d4_1_ldb_id;	/* assigned LDB id */
    DB_CAP_STR      d4_2_cap_cap;	/* capability */
    DB_CAPVAL_STR   d4_3_cap_val;	/* value of capability */
    i4		    d4_4_cap_lvl;	/* numeric value of capability */

#define	QEC_04D_CC_LDB_DBCAPS_4	    4	
} ;


/*}
** Name: QEC_D5_LONG_LDBNAMES - Catalog IIDD_DDB_LONG_LDBNAMES.
**
** Description:
**      This structure matches the schema of IIDD_DDB_LONG_LDBNAMES.
**      
** History:
**      20-jul-88 (carl)
**          written
*/

struct _QEC_D5_LONG_LDBNAMES
{
    QEC_256_PLUS1   d5_1_ldb_name;	/* long ldb name */
    i4		    d5_2_ldb_id;	/* assinged ldb id */
    DB_DB_STR       d5_3_ldb_alias;	/* "iildb_" + textual equivalent of 
					** above id */

#define	QEC_05D_CC_LONG_LDBNAMES_3  3	
} ;


/*}
** Name: QEC_D6_OBJECTS - Catalog IIDD_DDB_OBJECTS.
**
** Description:
**      This structure matches the schema of IIDD_DDB_OBJECTS.
**      
** History:
**      20-jul-88 (carl)
**          written
*/

struct _QEC_D6_OBJECTS
{
    DB_OBJ_STR      d6_1_obj_name;	/* object name */
    DB_OWN_STR      d6_2_obj_owner;	/* object owner */
    DB_TAB_ID	    d6_3_obj_id;	/* object's id */
    DB_QRY_ID	    d6_4_qry_id;	/* query text id */
    DB_DATE_STR     d6_5_obj_cre;	/* creation date */
    QEC_8C_PLUS1    d6_6_obj_type;	/* L, T, V, (I, P, R) */
    DB_DATE_STR     d6_7_obj_alt;	/* alter date of object */
    QEC_8C_PLUS1    d6_8_sys_obj;	/* Y or N */
    QEC_8C_PLUS1    d6_9_to_expire;	/* Y or N */
    DB_DATE_STR     d6_10_exp_date;	/* expiration date, valid if above is
					** Y */
#define	    QEC_06D_CC_OBJECTS_12   12	
} ;


/*}
** Name: QEC_D7_OBJECT_BASE - Catalog IIDD_DDB_OBJECT_BASE.
**
** Description:
**      This structure matches the schema of IIDD_DDB_OBJECT_BASE.
**      
** History:
**      20-jul-88 (carl)
**          written
*/

struct _QEC_D7_OBJECT_BASE
{
    i4	    d7_1_obj_base;		/* current value of object base */

#define	QEC_07D_CC_OBJECT_BASE_1    1	
} ;


/*}
** Name: QEC_D9_TABLEINFO - Catalog IIDD_DDB_TABLEINFO
**				    
**
** Description:
**      This structure matches the schema of IIDD_DDB_TABLEINFO.
**      
** History:
**      20-jul-88 (carl)
**          written
*/

struct _QEC_D9_TABLEINFO
{
    DB_TAB_ID	    d9_1_obj_id;	/* object's id */
    QEC_8C_PLUS1    d9_2_lcl_type;	/* "T", "V", or "I"*/
    DB_TAB_STR      d9_3_tab_name;	/* local table name */
    DB_OWN_STR      d9_4_tab_owner;	/* local table owner */
    DB_DATE_STR     d9_5_cre_date;	/* creation date */
    DB_DATE_STR     d9_6_alt_date;	/* alteration date */
    i4		    d9_7_rel_st1;	/* timestamp 1 */
    i4		    d9_8_rel_st2;	/* timestamp 2 */
    QEC_8C_PLUS1    d9_9_col_mapped;	/* "Y" or "N" */
    i4		    d9_10_ldb_id;	/* LDB id */

#define	QEC_09D_CC_TABLEINFO_11	    11	
} ;


/*}
** Name: QEC_D10_TREE - Catalog IIDD_DDB_TREE
**				    
**
** Description:
**      This structure is used to insert into IIDD_DDB_TREE.
**      
** History:
**      20-jul-88 (carl)
**          written
*/

struct _QEC_D10_TREE
{
    i4		    d10_1_treetabbase;	/* same as base of base object */
    i4		    d10_2_treetabidx;	/* same as index of base object */
    i4		    d10_3_treeid1;	/* high portion of creation timestamp */
    i4		    d10_4_treeid2;	/* low portion of creation timestamp */
    i2		    d10_5_treeseq;	/* sequence number of tuple in tree
					** representation */
    i2		    d10_6_treemode;	/* DB_VIEW (17), DB_PROT (19) or
					** DB_INTG (20) */
    i2		    d10_7_treevers;	/* tree version number */
    QEC_1024_PLUS1  d10_8_treetree;	/* tree segment */
    i4		    d10_9_treesize;	/* size of above varchar segment, NOT 
					** a column of the IIDD_DDB_TREE 
					** catalog */
#define	    QEC_10D_CC_TREE_9	    9	
} ;


/*}
** Name: QEC_L1_DBCONSTANTS - Catalog IIDD_DBCONSTANTS.
**
** Description:
**      This structure matches the schema of IIDBCONSTANTS and IIDD_DBCONSTANTS.
**      
** History:
**      15-jan-89 (carl)
**          written
*/

struct _QEC_L1_DBCONSTANTS
{
    DB_OWN_STR     l1_1_usr_name;	/* user name */
    DB_OWN_STR     l1_2_dba_name;	/* DBA name */

#define	QEC_01I_CC_DBCONSTANTS_2    2	
} ;


/*}
** Name: QEC_L2_ALT_COLUMNS - Catalog IIDD_ALT_COLUMNS.
**
** Description:
**      This structure matches the schema of IIDD_ALT_COLUMNS.
**      
** History:
**      20-jul-88 (carl)
**          written
*/

struct _QEC_L2_ALT_COLUMNS
{
    DB_TAB_STR      l2_1_tab_name;	/* table name */
    DB_OWN_STR      l2_2_tab_owner;	/* table owner */
    i4		    l2_3_key_id;	/* an arbitrary number to identify 
					** the key uniquely */
    DB_ATT_STR      l2_4_col_name;	/* column name */
    i2		    l2_5_seq_in_key;	/* sequence of column with key, from 
					** 1 */

#define	QEC_02I_CC_ALT_COLUMNS_5    5	
} ;


/*}
** Name: QEC_L3_COLUMNS - Catalog IIDD_COLUMNS.
**
** Description:
**      This structure matches the schema of IIDD_COLUMNS.
**      
** History:
**      20-jul-88 (carl)
**          written
**      12-sep-89 (carl)
**	    added 7.0 new fields
**      28-sep-89 (carl)
**	    rollback 7.0 new fields
*/

struct _QEC_L3_COLUMNS
{
    DB_TAB_STR      l3_1_tab_name;	/* table name */
    DB_OWN_STR      l3_2_tab_owner;	/* table owner */
    DB_ATT_STR      l3_3_col_name;	/* column name */
    DB_TYPE_STR     l3_4_data_type;	/* data type of column */
    i4		    l3_5_length;	/* length of column */
    i4		    l3_6_scale;		/* scale of column */
    QEC_8C_PLUS1    l3_7_nulls;		/* Y or N */
    QEC_8C_PLUS1    l3_8_defaults;	/* Y or N */
    i4		    l3_9_seq_in_row;	/* sequence of column in row */
    i4		    l3_10_seq_in_key;	/* column's sequence in primary key */
    QEC_8C_PLUS1    l3_11_sort_dir;	/* A(scending) or D(escending);
					** the standard catalog document
					** has this field as CHAR(1) and
					** INGRES has it as TEXT(1), must
					** provide buffer big enough for
					** RQF */
    i4		    l3_12_ing_datatype;	/* INGRES internal data type of column 
					*/
#define	QEC_03I_CC_COLUMNS_12	   12

    /* fields supporting UDTs in non-Star iicolumns */

    DB_TYPE_STR     l3_13_internal_datatype;
    i4		    l3_14_internal_length;
    i4		    l3_15_internal_ingtype;
    QEC_8C_PLUS1    l3_16_system_maintained;
  
#define	QEC_03I_CC_COLUMNS_16	   16
} ;


/*}
** Name: QEC_L4_DBCAPABILITIES - Catalog IIDD_DBCAPABILITIES.
**
** Description:
**      This structure matches the schema of IIDD_DBCAPABILITIES.
**      
** History:
**      20-jul-88 (carl)
**          written
*/

struct _QEC_L4_DBCAPABILITIES
{
    DB_CAP_STR     l4_1_cap_cap;	/* capability */
    DB_CAPVAL_STR  l4_2_cap_val;	/* value of above */

#define	QEC_04I_CC_DBCAPABILITIES_2 2
} ;


/*}
** Name: QEC_L5_HISTOGRAMS - Catalog IIDD_HISTOGRAMS.
**
** Description:
**      This structure matches the schema of IIDD_HISTOGRAMS.
**      
** History:
**      20-jul-88 (carl)
**          written
*/

struct _QEC_L5_HISTOGRAMS
{
    DB_TAB_STR      l5_1_tab_name;	/* table name */
    DB_OWN_STR      l5_2_tab_owner;	/* table owner */
    DB_ATT_STR      l5_3_col_name;	/* column name */
    i4		    l5_4_txt_seq;	/* sequence of histogram, numbered from
					** 1 */
    QEC_228_PLUS1   l5_5_txt_seg;	/* histogram data, created by optimzedb
					*/
#define	QEC_05I_CC_HISTOGRAMS_5	   5
} ;


/*}
** Name: QEC_L6_INDEXES - Catalog IIDD_DDB_LDB_INDEXES and IIDD_INDEXES.
**
** Description:
**      This structure matches the schema of IIDD_DDB_LDB_INDEXES and 
**  IIDD_INDEXES.
**      
** History:
**      20-jul-88 (carl)
**          written
**	06-mar-96 (nanpr01)
**	    Standard catalogue changes for variable page size project.
*/

struct _QEC_L6_INDEXES
{
    DB_TAB_STR      l6_1_ind_name;	/* index name */
    DB_OWN_STR      l6_2_ind_owner;	/* index owner */
    DB_DATE_STR     l6_3_cre_date;	/* creation date */
    DB_TAB_STR      l6_4_base_name;	/* base table name */
    DB_OWN_STR      l6_5_base_owner;	/* base table owner */
    QEC_16C_PLUS1   l6_6_storage;	/* HEAP, ISAM, BTREE, etc */
    QEC_8C_PLUS1    l6_7_compressed;	/* Y or N */
    QEC_8C_PLUS1    l6_8_uniquerule;	/* U for unique, D for duplicate key 
					** values, or blank for unknown */
    i4		    l6_9_pagesize;	/* page size of the index */
#define	QEC_06I_CC_INDEXES_8	   8
#define	QEC_06I_CC_INDEXES_9	   9
} ;


/*}
** Name: QEC_L7_INDEX_COLUMNS - Catalog IIDD_DDB_LDB_XCOLUMNS and
**				IIDD_INDEX_COLUMNS.
**
** Description:
**      This structure matches the schema of IIDD_DDB_LDB_XCOLUMNS and
**  IIDD_INDEX_COLUMNS.
**      
**
** History:
**      20-jul-88 (carl)
**          written
*/

struct _QEC_L7_INDEX_COLUMNS
{
    DB_TAB_STR      l7_1_ind_name;	/* index name */
    DB_OWN_STR      l7_2_ind_owner;	/* index owner */
    DB_ATT_STR      l7_3_col_name;	/* column name */
    i4		    l7_4_key_seq;	/* sequence of column within key */
    QEC_8C_PLUS1    l7_5_sort_dir;	/* A(scending) or D(escending) */

#define	QEC_07I_CC_INDEX_COLUMNS_5  5	
} ;


/*}
** Name: QEC_L14_REGISTRATIONS - Catalog IIDD_REGISTRATIONS
**				    
**
** Description:
**      This structure is used to insert into IIDD_REGISTRATIONS.
**      
**
** History:
**      07-nov-88 (carl)
**        written
*/

struct _QEC_L14_REGISTRATIONS
{
    DB_OBJ_STR      l14_1_obj_name;	/* object name */
    DB_OWN_STR      l14_2_obj_owner;	/* object owner */
    QEC_8C_PLUS1    l14_3_dml;		/* S for SQL, Q for QUEL*/
    QEC_8C_PLUS1    l14_4_obj_type;	/* T, V, I for table, view, index */
    QEC_8C_PLUS1    l14_5_obj_subtype;	/* L, N for link, native */
    i4		    l14_6_sequence;	/* sequence for text field, from 1 */
    DD_PACKET	   *l14_7_pkt_p;	/* ptr to text segment provided by PSF 
					*/
#define	QEC_14I_CC_REGISTRATIONS_7  7	
}   ;


/*}
** Name: QEC_L15_STATS - Catalog IISTATS 
**
** Description:
**      This structure matches the schema of IISTATS.
**      
**
** History:
**      19-jul-88 (carl)
**          written
**	23-jul-93 (rganski)
**	    Added new iistats columns, and changed
**	    QEC_15I_CC_STATS_9 to QEC_15I_CC_STATS_13.
**	22-nov-93 (rganski)
**	    Added QEC_15I_CC_STATS_9, which is needed when LDB is pre-6.5.
**	    Also added QEC_NEW_STATS_LEVEL, which indicates which version (605)
**	    the new iistats columns first appear in;  QEC_DEFAULT_DOMAIN,
**	    QEC_DEFAULT_COMPLETE, and QEC_DEFAULT_HIST_LEN, which define the
**	    default values for their respective fields - these defaults are
**	    used when the LDB is pre-6.5.
*/

struct _QEC_L15_STATS
{
    DB_TAB_STR      l15_1_tab_name;	/* table name */
    DB_OWN_STR      l15_2_tab_owner;	/* table owner */
    DB_ATT_STR      l15_3_col_name;	/* column name */
    DB_DATE_STR     l15_4_cre_date;	/* create date */
    f4		    l15_5_num_uniq;	/* number of unique values in column */
					/* should be f8 according to standard */
    f4		    l15_6_rept_factor;	/* repitition faction */
					/* should be f8 according to standard */
    QEC_8C_PLUS1    l15_7_has_uniq;	/* Y or N */
    f4		    l15_8_pct_nulls;	/* percentages of table that contains
					** NULL in this column */
					/* should be f8 according to standard */
    i4		    l15_9_num_cells;	/* number of cells in the histogram */
    i4		    l15_10_col_domain;	/* column domain number */
#define	QEC_DEFAULT_DOMAIN	0
    QEC_8C_PLUS1    l15_11_is_complete; /* Y or N - complete flag */
#define	QEC_DEFAULT_COMPLETE	"N       "
    QEC_8C_PLUS1    l15_12_stat_vers;	/* version number for these stats */
    i4		    l15_13_hist_len;	/* Length of histogram values */
#define	QEC_DEFAULT_HIST_LEN	0

#define	QEC_15I_CC_STATS_9   	9
#define	QEC_15I_CC_STATS_13   	13
#define	QEC_NEW_STATS_LEVEL	605
} ;


/*}
** Name: QEC_L16_TABLES - Catalog IITABLES 
**
** Description:
**      This structure matches the schema of IITABLES.
**      
**
** History:
**      20-jul-88 (carl)
**          written
**	06-mar-96 (nanpr01)
**	    Standard catalogue changes for variable page size project.
**      27-jul-96 (ramra01)
**          Standard catalogue changes for alter table project.
**	    relversion and reltotwid fields added.
**	26-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Added reltcpri to IITABLES.
*/

struct _QEC_L16_TABLES
{
    DB_TAB_STR      l16_1_tab_name;	/* table name */
    DB_OWN_STR      l16_2_tab_owner;	/* table owner */
    DB_DATE_STR     l16_3_cre_date;	/* create date */
    DB_DATE_STR     l16_4_alt_date;	/* alter date */
    QEC_8C_PLUS1    l16_5_tab_type;	/* T, L, V or I */
    QEC_8C_PLUS1    l16_6_sub_type;	/* L (link), T (table) or blank for 
					** unknown */
    QEC_8C_PLUS1    l16_7_version;	/* object version */
    QEC_8C_PLUS1    l16_8_sys_use;	/* S, U or blank for unknown */

    /* the following are PHYSICAL columns, default values are -1 for numeric
    ** and blank for character columns */

    QEC_8C_PLUS1    l16_9_stats;	/* Y, N or blank for unknown */
    QEC_8C_PLUS1    l16_10_indexes;	/* Y, N or blank for unknown */
    QEC_8C_PLUS1    l16_11_readonly;	/* Y, N or blank for unknown */
    i4		    l16_12_num_rows;	/* number of rows, -1 if unknown */
    QEC_16C_PLUS1   l16_13_storage;	/* HEAP, etc., blank if unknown */
    QEC_8C_PLUS1    l16_14_compressed;	/* Y, N or blank for unknown */
    QEC_8C_PLUS1    l16_15_dup_rows;	/* U, D or blank */
    QEC_8C_PLUS1    l16_16_uniquerule;	/* U, D or blank */
    i4		    l16_17_num_pages;	/* number of pages, -1 if unknown */
    i4		    l16_18_overflow;	/* number of overflow pages, -1 if
					** unknown */

    /* the following are INGRES columns, default values are 0 for numeric
    ** and blank for character columns */

    i4		    l16_19_row_width;	/* row width */
    i4		    l16_20_tab_expire;	/* INGRES _bintime expiration date */
    DB_DATE_STR     l16_21_mod_date;	/* modification date of table */
    DB_LOC_STR      l16_22_loc_name;	/* location name, blank if unknown */
    QEC_8C_PLUS1    l16_23_integrities;	/* Y or blank */
    QEC_8C_PLUS1    l16_24_permits;	/* Y or blank */
    QEC_8C_PLUS1    l16_25_all_to_all;	/* Y or N */
    QEC_8C_PLUS1    l16_26_ret_to_all;	/* Y or N */
    QEC_8C_PLUS1    l16_27_journalled;	/* Y, N or C if to be enabled on the
					** check point */
    QEC_8C_PLUS1    l16_28_view_base;	/* Y, N or blank for unknown */
    QEC_8C_PLUS1    l16_29_multi_loc;	/* Y, N or blank for unknown */
    i2		    l16_30_ifillpct;	/* fill factor of index pages */
    i2		    l16_31_dfillpct;	/* fill factor of data pages */
    i2		    l16_32_lfillpct;	/* fill factor of leaf pages */
    i4		    l16_33_minpages;	/* minpages parameter */
    i4		    l16_34_maxpages;	/* maxpages parameter */
    i4		    l16_35_rel_st1;	/* timestamp 1 */
    i4		    l16_36_rel_st2;	/* timestamp 2 */
    i4		    l16_37_reltid;	/* internal id1 */
    i4		    l16_38_reltidx;	/* internal id2 */
    i4		    l16_39_pagesize;	/* page size of the table*/
    i2		    l16_40_relversion;  /* rel version for alt tab */
    i4		    l16_41_reltotwid;   /* rel total width alt tab */
    i2		    l16_42_reltcpri;    /* table cache priority */

#define	    QEC_16I_CC_TABLES_38    38	
#define	    QEC_16I_CC_TABLES_42    42	
#define	    QEC_013I_CC_PHYSICAL_TABLES_13  13
#define	    QEC_013I_CC_PHYSICAL_TABLES_14  14
#define	    QEC_CAT_COL_COUNT_MAX   42	/* number of columns in the 
					** largest catalogs, iitables 
					** and iidd_tables, used for
					** allocating space for catalog
					** manipulation arrays and RQF 
					** bind arrays */
#define	QEC_NEW_STDCAT_LEVEL	800     /* This is required for adding
					** pagesize in std. cat interface.
					** This allows prior versions to work.
					*/
} ;


/*}
** Name: QEC_L17_VIEWS - Catalog IIDD_VIEWS.
**
** Description:
**      This structure is used to insert into IIDD_VIEWS.
**      
**
** History:
**      20-jul-88 (carl)
**          written
*/

struct _QEC_L17_VIEWS
{
    DB_TAB_STR      l17_1_tab_name;	/* table name */
    DB_OWN_STR      l17_2_tab_owner;	/* table owner */
    QEC_8C_PLUS1    l17_3_dml;		/* S for SQL, Q for QUEL*/
    QEC_8C_PLUS1    l17_4_chk_option;	/* Y, N, or blank for unknown */
    i4		    l17_5_sequence;	/* sequence for text field, from 1 */
    QEC_256_PLUS1   l17_6_txt_seg;	/* text segment of view statement */
    i4		    l17_7_txt_size;	/* size of above varchar segment, NOT 
					** a column of the IIDD_VIEWS catalog */
#define	    QEC_17I_CC_VIEWS_7	   7	
} ;

/*}
** Name: QEC_L18_PROCEDURES - Catalog IIPROCEDURS
**
** Description:
**      This structure matches the schema of IIPROCEDURES.
**      
**
** History:
**      19-nov-92 (teresa)
**	    Initial Creation
*/

struct _QEC_L18_PROCEDURES
{
    DB_DBP_STR      l18_1_proc_name;	/* object name */
    DB_OWN_STR      l18_2_proc_owner;	/* object owner */
    DB_DATE_STR     l18_3_cre_date;	/* create date */
    QEC_8C_PLUS1    l18_4_proc_subtype;	/* N (i4ive) or I (imported) */
    /* note this table also has text_sequence and text_subtype fields, but
    ** we never import these fields */    
    i4		    l18_5_sequence;	/* sequence for text field, from 1 */
    DD_PACKET	   *l18_6_pkt_p;	/* ptr to text segment provided by PSF*/

#define	    QEC_18I_CC_PROC_4	    4	    /* we read 4 columns */
#define	    QEC_18IA_CC_PROC_6	    6	    /* we write 6 columns */
} ;

/*}
** Name: QEC_MIN_CAP_LVLS - Structure for storing minimum levels of LDB
**			    capabilities
**
** Description:
**      This structure is used to store computed minimum capability levels.
**  Note that i2 is returned by INGRES in reponse to COUNT.
**      
**
** History:
**      20-jul-88 (carl)
**          written
**      25-jan-89 (carl)
**	    added qec_m5_uniqkey
**	01-mar-93 (barbara)
**	    added qec_m6_opn_sql (OPEN/SQL_LEVEL)
*/

struct _QEC_MIN_CAP_LVLS
{
    i4		     qec_m1_com_sql;	/* of COMMON/SQL_LEVEL */
    i4		     qec_m2_ing_quel;	/* of INGRES/QUEL_LEVEL */
    i4		     qec_m3_ing_sql;	/* of INGRES/SQL_LEVEL */
    i4		     qec_m4_savepts;	/* of SAVEPONTS, 0 (NO) or 1 (YES) */
    i4		     qec_m5_uniqkey;	/* of UNIQUE_KEY_REQ, 0 (NO) or 1 (YES)
					*/
    i4		     qec_m6_opn_sql;	/* of OPEN/SQL_LEVEL */
} ;


/*}
** Name: QEC_INDEX_ID - Index id
**
** Description:
**      This structure is used to store the name and owner of an index.
**      
**
** History:
**      20-jul-88 (carl)
**          written
*/

typedef struct _QEC_INDEX_ID
{
    DB_TAB_STR      qec_i1_name;	/* name */
    DB_OWN_STR      qec_i2_owner;	/* owner */
    DB_TAB_STR      qec_i3_given;	/* given name */
}   QEC_INDEX_ID;


/*}
** Name: QEC_DEPEND_VIEW - Dependent view information
**
** Description:
**      This structure contains the object id and query id for a view.
**      
** History:
**      10-nov-88 (carl)
**          written
*/

typedef struct _QEC_DEPEND_VIEW
{
    DB_TAB_ID	    qec_1_obj_id;	/* object id */
    DB_QRY_ID	    qec_2_qry_id;	/* query id */
}   QEC_DEPEND_VIEW;


/*}
** Name: QEC_LONGNAT_TO_I4 - Union of i4 and i4.
**
** Description:
**      This structure is used to coerce a i4 to an i4.
**      
** History:
**      19-nov-88 (carl)
**          written
*/

typedef struct _QEC_LONGNAT_TO_I4
{
    union
    {
	i4	    qec_1_i4;	/* long i4  */
	i4	    qec_2_i4;		/* i4 */
    }	qec_i4_i4;
}   QEC_LONGNAT_TO_I4;


/*}
** Name: QEC_LINK - Structure for creating or destroying a link.
**
** Description:
**      This structure contains information for creating or destroying a link.
**      
**
** History:
**      20-jul-88 (carl)
**          written
**      06-apr-89 (carl)
**	    added qec_28_iistats_cnt 
*/

typedef struct _QEC_LINK
{
    QED_DDL_INFO	*qec_1_ddl_info_p;	/* ptr to parser's input info */
    QEC_D9_TABLEINFO    *qec_2_tableinfo_p;	/* table info of object if link
						*/
    i4			 qec_3_ldb_id;		/* LDB id, 0 if new participant */
    i4			 qec_4_col_cnt;		/* number of columns */
    DB_DB_STR		 qec_5_ldb_alias;	/* LDB alias if name exceeds
						** sizeof(DB_DB_NAME) */
    QEQ_1CAN_QRY	*qec_6_select_p;	/* structure ptr union for 
						** SELECT queries */
    QEC_D2_LDBIDS	*qec_7_ldbids_p;	/* ptr to internal LDB id info 
						*/
    QEC_D5_LONG_LDBNAMES
			*qec_8_longnames_p;	/* ptr to internal long name 
						** info */
    QEC_L16_TABLES	*qec_9_tables_p;	/* ptr to IITABLES structure */
    i4		 qec_10_haves;		/* indicators of table 
						** properties */

#define	    QEC_01_ALT_COLUMNS	    0x0001L	/* ON if object has alternate 
						** keys in the form of 
						** IIALT_COLUMNS */
#define	    QEC_02_INDEXES	    0x0002L	/* ON if object has indexes */

#define	    QEC_03_INGRES	    0x0004L	/* ON if object is managed by 
						** INGRES */
#define	    QEC_04_LONG_LDBNAME	    0x0008L	/* ON if object's LDB has 
						** a long name > 32 chars */
#define	    QEC_05_STATS	    0x0010L	/* ON if object has statistics
						*/
#define	    QEC_06_UPPER_LDB	    0x0020L	/* ON if LDB uses only upper-
						** case names */
#define	    QEC_07_CREATE	    0x0040L	/* ON if creating, off if
						** destroying */
#define	    QEC_08_NO_IITABLES	    0x0080L	/* ON if index of non-INGRES 
						** gateway has no IITABLES
						** entry, off otherwise */
#define	    QEC_09_LDBCAPS	    0x0100L	/* ON if IIDD_DDB_LDB_DBCAPS
						** has new member tuples added,
						** off otherwise */
#define	    QEC_10_USE_PHY_SRC	    0x0200L	/* ON if physical information 
						** comes from LDB's 
						** iiphysical_tables, OFF
						** if all from iitables */
#define	    QEC_11_LDB_DIFF_ARCH    0x0400L	/* ON if LDB has a different
						** architecture than STAR's
						** (implying that statistics
						** should be propagated, OFF
						** otherwise */
    i4			 qec_11_ldb_obj_cnt;	/* number of objects LDB has in 
						** DDB */
    QEC_L6_INDEXES	*qec_12_indexes_p;	/* for storing IIINDEXES 
						** inforamtion */
    QEC_D6_OBJECTS	*qec_13_objects_p;	/* ptr to IIDD_DDB_OBJECTS 
						** structure */
    i4			 qec_15_ndx_cnt;	/* number of indexes of a local 
						** table, 0 if none */
    QEC_INDEX_ID	*qec_16_ndx_ids_p;	/* READ-ONLY ptr to index id 
						** space, MUST NOT change */
    i4			 qec_17_ts1;		/* current time stamp 1 */
    i4			 qec_18_ts2;		/* current time stamp 2 */
    DD_LDB_DESC		*qec_19_ldb_p;		/* LDB hosting the local table 
						*/
    struct _RQB_BIND	*qec_20_rqf_bind_p;	/* ptr to array of RQF bind 
						** elements for fetching 
						** tuple columns */
    QEQ_1CAN_QRY	*qec_21_delete_p;	/* structure ptr union for
						** DELETE queries */
    QEQ_1CAN_QRY	*qec_22_insert_p;	/* structure ptr union for
						** INSERT queries */
    QEQ_1CAN_QRY	*qec_23_update_p;	/* structure ptr union for
						** UPDATE queries */
    DB_DATE_STR		qec_24_cur_time;	/* current time */
    QEC_MIN_CAP_LVLS	*qec_25_pre_mins_p;	/* for computing minimum
						** LDB capability levels 
						** before introduction of
						** new LDB into DDB */
    QEC_MIN_CAP_LVLS	*qec_26_aft_mins_p;	/* for computing minimum
						** LDB capability levels 
						** after withdrawal of
						** LDB from DDB */
    i4			 qec_27_select_cnt;	/* number of tuples read on s
						** canned SELECT query, for
						** inter-routine communication
						** purpose */
    i4			 qec_28_iistats_cnt;	/* number of IISTATS tuples 
						** for current LDB table */
}   QEC_LINK;


#endif /* QEFCAT_H_INCLUDED */
