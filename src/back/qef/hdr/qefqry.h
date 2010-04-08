/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: QEFQRY.H - Constants and type for QEF.
**
** Description:
**      This file contains constants and type for canned queries.
**
** History: $Log-for RCS$
**      20-sep-88 (carl)
**          written
**      09-dec-88 (carl)
**	    modified for catalog definition changes
**      15-jan-89 (carl)
**	    added 1) constant SEL_105_II_DBCONSTANTS, 2) forward reference 
**	    QEC_L1_DBCONSTANTS and 3) new field l1_dbconstants_p to 
**	    QEQ_1CAN_QRY
**      25-jan-89 (carl)
**	    added SEL_204_MIN_UNIQUE_KEY_REQ and SEL_209_UNIQUE_KEY_REQ 
**      06-apr-89 (carl)
**	    added SEL_120_II_STATS_COUNT
**	19-nov-92 (teresa)
**	    addded TII_18_PROCEDURES, TDD_28_PROCEDURES
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*  Forward references */

typedef	struct	_QEC_D1_DBDEPENDS	 QEC_D1_DBDEPENDS;
typedef	struct	_QEC_D2_LDBIDS		 QEC_D2_LDBIDS;
typedef	struct	_QEC_D3_LDB_COLUMNS	 QEC_D3_LDB_COLUMNS;
typedef	struct	_QEC_D4_LDB_DBCAPS	 QEC_D4_LDB_DBCAPS;
typedef	struct	_QEC_D5_LONG_LDBNAMES	 QEC_D5_LONG_LDBNAMES;
typedef	struct	_QEC_D6_OBJECTS		 QEC_D6_OBJECTS;
typedef	struct	_QEC_D7_OBJECT_BASE	 QEC_D7_OBJECT_BASE;
typedef	struct	_QEC_D9_TABLEINFO	 QEC_D9_TABLEINFO;
typedef	struct	_QEC_D10_TREE		 QEC_D10_TREE;

typedef struct	_QEC_L1_DBCONSTANTS	 QEC_L1_DBCONSTANTS;
typedef	struct	_QEC_L2_ALT_COLUMNS	 QEC_L2_ALT_COLUMNS;
typedef	struct	_QEC_L3_COLUMNS		 QEC_L3_COLUMNS;
typedef	struct	_QEC_L4_DBCAPABILITIES	 QEC_L4_DBCAPABILITIES;
typedef	struct	_QEC_L5_HISTOGRAMS	 QEC_L5_HISTOGRAMS;
typedef	struct	_QEC_L6_INDEXES		 QEC_L6_INDEXES;
typedef	struct	_QEC_L7_INDEX_COLUMNS	 QEC_L7_INDEX_COLUMNS;
typedef struct	_QEC_L14_REGISTRATIONS	 QEC_L14_REGISTRATIONS;
typedef	struct	_QEC_L15_STATS		 QEC_L15_STATS;
typedef	struct	_QEC_L16_TABLES		 QEC_L16_TABLES;
typedef	struct	_QEC_L17_VIEWS		 QEC_L17_VIEWS;
typedef	struct	_QEC_L18_PROCEDURES 	 QEC_L18_PROCEDURES;

typedef	struct	_QEC_MIN_CAP_LVLS	 QEC_MIN_CAP_LVLS;
typedef	struct	_QEC_ALT_KEY_INFO	 QEC_ALT_KEY_INFO;


/*  Constants for indexing into column name table in QEDKON.ROC  */

#define	    COL_00_ALTER_DATE		0
#define	    COL_01_COLUMN_NAME		1
#define	    COL_02_CREATE_DATE		2
#define	    COL_03_TABLE_NAME		3
#define	    COL_04_TABLE_OWNER		4
#define	    COL_05_TABLE_TYPE		5
#define	    COL_06_CDB_NODE		6
#define	    COL_07_CDB_DBMS		7
#define	    COL_08_CDB_NAME		8
#define	    COL_09_DDB_NAME		9
#define	    COL_10_ACCESS		10
#define	    COL_11_NAME			11
#define	    COL_12_BASE_NAME		12
#define	    COL_13_BASE_OWNER		13
#define	    COL_14_STATUS		14
#define	    COL_15_DBNAME		15
#define	    COL_16_USRNAME		16
#define	    COL_17_OWN			17		
#define	    COL_18_DBSERVICE		18
#define	    COL_19_DB_ID		19
#define	    COL_20_LDB_ID		20
#define	    COL_21_LDB_NODE		21
#define	    COL_22_LDB_DBMS		22
#define	    COL_23_LDB_LONGNAME		23
#define	    COL_24_LDB_DATABASE		24
#define	    COL_25_TREETABBASE		25
#define	    COL_26_TREETABIDX		26
#define	    COL_27_TABLE_STATS		27
#define	    COL_28_LONG_LDB_NAME	28
#define	    COL_29_LONG_LDB_ID		29
#define	    COL_30_LONG_LDB_ALIAS	30
#define	    COL_31_CAP_CAPABILITY	31
#define	    COL_32_MODIFY_DATE		32
#define	    COL_33_CAP_LEVEL		33
#define	    COL_34_CAP_VALUE		34
#define	    COL_35_KEY_ID		35
#define	    COL_36_OBJECT_NAME		36
#define	    COL_37_OBJECT_OWNER		37
#define	    COL_38_OBJECT_BASE		38
#define	    COL_39_OBJECT_INDEX		39
#define	    COL_40_INDEX_NAME		40
#define	    COL_41_INDEX_OWNER		41
#define	    COL_42_INID1		42
#define	    COL_43_INID2		43
#define	    COL_44_DEID1		44
#define	    COL_45_DEID2		45
#define	    COL_46_PROCEDURE_NAME	46
#define	    COL_47_PROCEDURE_OWNER	47
#define	    COL_48_DATABASE_NAME	48
#define	    COL_49_DATABASE_OWNER	49
#define	    COL_50_DATABASE_SERVICE	50
#define	    COL_51_DATABASE_ID		51
#define     COL_52_USER_NAME		52
#define     COL_53_INTERNAL_STATUS	53


/*  Constants for indexing into the DDB catalog names table in QEDKON.ROC,
**  also used for ids to order canned SELECT * queries on DDB catalogs */

#define	    TDD_00_UNUSED		0
					/* unused */
#define	    TDD_01_ALT_COLUMNS		1
					/* IIDD_ALT_COLUMNS */
#define	    TDD_02_COLUMNS		2
					/* IIDD_COLUMNS */
#define	    TDD_03_DBCAPABILITIES	3
					/* IIDD_DBCAPABILITIES (not used) */
#define	    TDD_04_DBCONSTANTS		4   
					/* IIDD_DBCONSTANTS (not used) */
#define	    TDD_05_DDB_DBDEPENDS	5
					/* IIDD_DDB_DBDEPENDS */
#define	    TDD_06_DDB_LDBIDS		6
					/* IIDD_DDB_LDBIDS */
#define	    TDD_07_DDB_LDB_COLUMNS	7
					/* IIDD_DDB_LDB_COLUMNS */
#define	    TDD_08_DDB_LDB_DBCAPS	8
					/* IIDD_DDB_LDB_DBCAPS */
#define	    TDD_14_DDB_LONG_LDBNAMES	14
					/* IIDD_DDB_LONG_LDBNAMES */
#define	    TDD_15_DDB_OBJECTS		15
					/* IIDD_DDB_OBJECTS */
#define	    TDD_16_DDB_OBJECT_BASE	16
					/* IIDD_DDB_OBJECT_BASE */
#define	    TDD_18_DDB_TABLEINFO	18
					/* IIDD_DDB_TABLEINFO */
#define	    TDD_19_DDB_TREE		19
					/* IIDD_DDB_TREE */
#define	    TDD_20_HISTOGRAMS		20
					/* IIDD_HISTOGRAMS */
#define	    TDD_21_INDEXES		21
					/* IIDD_INDEXES */
#define	    TDD_22_INDEX_COLUMNS	22
					/* IIDD_INDEX_COLUMNS */
#define	    TDD_23_UNUSED		23
					/* unused */
#define	    TDD_24_INTEGRITIES		24
					/* IIDD_INTEGRITIES */
#define	    TDD_25_UNUSED		25
					/* unused */
#define	    TDD_27_PERMITS		27
					/* IIDD_PERMITS */
#define	    TDD_28_PROCEDURES		28
					/* iidd_procedures */
#define	    TDD_29_REGISTRATIONS	29
					/* IIDD_REGISTRATIONS */
#define	    TDD_30_STATS		30
					/* IIDD_STATS */
#define	    TDD_31_TABLES		31
					/* IIDD_TABLES */
#define	    TDD_32_VIEWS		32
					/* IIDD_VIEWS */

/*  Constants for indexing into the LDB table name table in QEDKON.ROC, also
**  used for ids to order canned SELECT * queries on STANDARD catalogs */

#define	    TII_00_UNUSED		0
					/* unused */
#define	    TII_01_ALT_COLUMNS		1
					/* IIALT_COLUMNS */
#define	    TII_02_DBCAPABILITIES	2
					/* IIDBCAPABILITIES */
#define	    TII_03_DBCONSTANTS		3   
					/* IIDBCONSTANTS */
#define	    TII_04_COLUMNS		4
					/* IICOLUMNS */
#define	    TII_05_DBDEPENDS		5
					/* IIDBDEPENDS */
#define	    TII_06_HISTOGRAMS		6
					/* IIHISTOGRAMS */
#define	    TII_07_INDEXES		7
					/* IIINDEXES */
#define	    TII_08_INDEX_COLUMNS	8
					/* IIINDEX_COLUMNS */
#define	    TII_09_UNUSED		9
					/* unused */
#define	    TII_10_UNUSED		10
					/* unused */
#define	    TII_11_INTEGRITIES		11
					/* IIINTEGRITIES */
#define	    TII_12_PERMITS		12
					/* IIPERMITS */
#define	    TII_13_PHYSICAL_TABLES	13
					/* IIPHYSICAL_TABLES */
#define	    TII_16_STATS		16
					/* IISTATS */
#define	    TII_17_TABLES		17
					/* IITABLES */
#define	    TII_18_PROCEDURES		18
					/* IIPROCEDURES */
#define	    TII_19_VIEWS		19
					/* IIVIEWS */
#define	    TII_20_DATABASE		20
					/* IIDATABASE in (IIDBDB) */
#define	    TII_21_USER			21
					/* IIUSER in (IIDBDB) */
#define	    TII_22_DBACCESS		22
					/* IIDBACCESS in (IIDBDB) */
#define	    TII_23_STAR_CDBS		23
					/* IISTAR_CDBS in (IIDBDB) */
#define	    TII_24_IIDATABASE_INFO	24
					/* IIDATABASE_INFO in (IIDBDB) */
#define	    TII_25_IIUSERS		25
					/* IIUSERS in (IIDBDB) */

/*}
** Name: QEQ_0CAN_ID - Canned query id
**
** Description:
**      Used to order a canned query.
**      
** History:
**      20-jul-88 (carl)
**          written
**      25-jan-89 (carl)
**	    added SEL_204_MIN_UNIQUE_KEY_REQ and SEL_209_UNIQUE_KEY_REQ 
**	19-nov-92 (teresa)
**	    added SEL_118_II_PROCEDURES
**	21-dec-92 (teresa)
**	    defined DEL_528_DD_PROCEDURES
**	01-mar-93 (barbara)
**	    Added UPD_805_OPN_SQL_LVL and SEL_210_OPN_SQL_LVL.
**	27-may-93 (barbara)
**	    Added SEL_121_II_COLNAMES.
**	02-sep-93 (barbara)
**	    Changed SEL_210_OPN_SQL_LVL into SEL_210_MIN_OPN_SQL_LEVEL and
**	    added SEL_211_OPN_SQL_LVL.  One is used for querying MIN(cap_level)
**	    from iidd_ddb_ldb_dbcaps for OPEN/SQL_LEVEL and one is used for
**	    getting all OPEN/SQL_LEVEL tuples from that iidd_ddb_ldb_dbcaps.
*/

typedef i4	QEQ_0CAN_ID;

/* SELECT * queries on STAR-specific catalogs  */

#define	    SEL_000_DD_UNUSED			(QEQ_0CAN_ID)  0
#define	    SEL_001_DD_ALT_COLUMNS		(QEQ_0CAN_ID)  1
#define	    SEL_002_DD_COLUMNS			(QEQ_0CAN_ID)  2
#define	    SEL_003_DD_DBCAPABILITIES		(QEQ_0CAN_ID)  3
#define	    SEL_005_DD_DDB_LDBIDS_BY_NAME	(QEQ_0CAN_ID)  5
#define	    SEL_006_DD_DDB_LDBIDS_BY_ID		(QEQ_0CAN_ID)  6
#define	    SEL_007_DD_DDB_LDB_COLUMNS		(QEQ_0CAN_ID)  7
#define	    SEL_008_DD_DDB_LDB_DBCAPS		(QEQ_0CAN_ID)  8
#define	    SEL_009_DD_DDB_DBDEPENDS_BY_IN	(QEQ_0CAN_ID)  9
#define	    SEL_010_DD_DDB_DBDEPENDS_BY_DE	(QEQ_0CAN_ID)  10
#define	    SEL_014_DD_DDB_LONG_LDBNAMES	(QEQ_0CAN_ID)  14
#define	    SEL_015_DD_DDB_OBJECTS_BY_NAME	(QEQ_0CAN_ID)  15
#define	    SEL_016_DD_DDB_OBJECT_BASE		(QEQ_0CAN_ID)  16
#define	    SEL_018_DD_DDB_TABLEINFO		(QEQ_0CAN_ID)  18
#define	    SEL_019_DD_PROCEDURES		(QEQ_0CAN_ID)  19
#define	    SEL_020_DD_HISTOGRAMS		(QEQ_0CAN_ID)  20
#define	    SEL_021_DD_INDEXES			(QEQ_0CAN_ID)  21
#define	    SEL_022_DD_INDEX_COLUMNS		(QEQ_0CAN_ID)  22
#define	    SEL_023_DD_UNUSED			(QEQ_0CAN_ID)  23
#define	    SEL_025_DD_UNUSED			(QEQ_0CAN_ID)  25
#define	    SEL_028_DD_UNUSED			(QEQ_0CAN_ID)  28
#define	    SEL_030_DD_STATS			(QEQ_0CAN_ID)  30
#define	    SEL_031_DD_TABLES			(QEQ_0CAN_ID)  31
#define	    SEL_032_DD_VIEWS			(QEQ_0CAN_ID)  32
#define	    SEL_035_DD_DDB_OBJECTS_BY_ID	(QEQ_0CAN_ID)  35
#define	    SEL_036_DD_LDB_OBJECT_COUNT		(QEQ_0CAN_ID)  36


/* SELECT * queries on STANDARD catalogs (DDB or LDB) */

#define	    SEL_100_II_UNUSED			(QEQ_0CAN_ID)  100
#define	    SEL_101_II_ALT_COLUMNS		(QEQ_0CAN_ID)  101
#define	    SEL_102_II_COLUMNS			(QEQ_0CAN_ID)  102
#define	    SEL_104_II_DBCAPABILITIES		(QEQ_0CAN_ID)  104
#define	    SEL_105_II_DBCONSTANTS		(QEQ_0CAN_ID)  105
#define	    SEL_106_II_HISTOGRAMS		(QEQ_0CAN_ID)  106
#define	    SEL_107_II_INDEXES			(QEQ_0CAN_ID)  107
#define	    SEL_108_II_INDEX_COLUMNS		(QEQ_0CAN_ID)  108
#define	    SEL_109_II_UNUSED			(QEQ_0CAN_ID)  109
#define	    SEL_110_II_UNUSED			(QEQ_0CAN_ID)  110
#define	    SEL_113_II_PHYSICAL_TABLES		(QEQ_0CAN_ID)  113
#define	    SEL_116_II_STATS			(QEQ_0CAN_ID)  116
#define	    SEL_117_II_TABLES			(QEQ_0CAN_ID)  117
#define	    SEL_118_II_PROCEDURES		(QEQ_0CAN_ID)  118
#define	    SEL_119_II_VIEWS			(QEQ_0CAN_ID)  119
#define	    SEL_120_II_STATS_COUNT		(QEQ_0CAN_ID)  120
#define	    SEL_121_II_COLNAMES			(QEQ_0CAN_ID)  121

/* Miscellaneous SELECT ... queries */

#define	    SEL_200_MIN_COM_SQL		(QEQ_0CAN_ID)  200
						/* selecting minimum CAP_VALUE 
						** of COMMON/SQL_LEVEL FROM 
						** IIDD_DDB_LDB_DBCAPS */
#define	    SEL_201_MIN_ING_QUEL	(QEQ_0CAN_ID)  201
						/* selecting minimum CAP_VALUE 
						** of INGRES/QUEL_LEVEL FROM 
						** IIDD_DDB_LDB_DBCAPS */
#define	    SEL_202_MIN_ING_SQL		(QEQ_0CAN_ID)  202
						/* selecting minimum CAP_VALUE 
						** of INGRES/SQL_LEVEL FROM 
						** IIDD_DDB_LDB_DBCAPS */
#define	    SEL_203_MIN_SAVEPTS		(QEQ_0CAN_ID)  203
						/* selecting minimum CAP_VALUE 
						** of SAVEPOINTS FROM 
						** IIDD_DDB_LDB_DBCAPS */
#define	    SEL_204_MIN_UNIQUE_KEY_REQ	(QEQ_0CAN_ID)  204
						/* selecting minimum CAP_VALUE 
						** of UNIQUE_KEY_REQ FROM 
						** IIDD_DDB_LDB_DBCAPS */
#define	    SEL_205_COM_SQL_LVL		(QEQ_0CAN_ID)  205
						/* selecting tuple with minimal 
						** COMMON/SQL_LEVEL value FROM 
						** IIDD_DDB_LDB_DBCAPS */
#define	    SEL_206_ING_QUEL_LVL	(QEQ_0CAN_ID)  206
						/* selecting tuple with minimal
						** INGRES/QUEL_LEVEL value FROM 
						** IIDD_DDB_LDB_DBCAPS */
#define	    SEL_207_ING_SQL_LVL		(QEQ_0CAN_ID)  207
						/* selecting tuple with minimal
						** INGRES/SQL_LEVEL value FROM 
						** IIDD_DDB_LDB_DBCAPS */
#define	    SEL_208_SAVEPOINTS		(QEQ_0CAN_ID)  208
						/* selecting tuple with minimal
						** SAVEPOINTS value FROM 
						** IIDD_DDB_LDB_DBCAPS */
#define	    SEL_209_UNIQUE_KEY_REQ	(QEQ_0CAN_ID)  209
						/* selecting tuple with minimal
						** UNIQUE_KEY_REQ value FROM 
						** IIDD_DDB_LDB_DBCAPS */
#define	    SEL_210_MIN_OPN_SQL_LVL	(QEQ_0CAN_ID)  210
						/* selecting minimum CAP_VALUE
						** of OPEN/SQL_LEVEL FROM 
						** IIDD_DDB_LDB_DBCAPS */
#define	    SEL_211_OPN_SQL_LVL		(QEQ_0CAN_ID)  211
						/* selecting tuple with minimal
						** OPEN/SQL_LEVEL value from
						** IIDD_DDB_LDB_DBCAPS */


/*  DELETE IIDD_ catalog queries */

#define	    DEL_500_DD_UNUSED		    (QEQ_0CAN_ID)  500
#define	    DEL_501_DD_ALT_COLUMNS	    (QEQ_0CAN_ID)  501
#define	    DEL_502_DD_COLUMNS		    (QEQ_0CAN_ID)  502
#define	    DEL_505_DD_DDB_DBDEPENDS	    (QEQ_0CAN_ID)  505
#define	    DEL_506_DD_DDB_LDBIDS	    (QEQ_0CAN_ID)  506
#define	    DEL_507_DD_DDB_LDB_COLUMNS	    (QEQ_0CAN_ID)  507
#define	    DEL_508_DD_DDB_LDB_DBCAPS	    (QEQ_0CAN_ID)  508
#define	    DEL_514_DD_DDB_LONG_LDBNAMES    (QEQ_0CAN_ID)  514
#define	    DEL_515_DD_DDB_OBJECTS	    (QEQ_0CAN_ID)  515
#define	    DEL_516_DD_DDB_OBJECT_BASE	    (QEQ_0CAN_ID)  516
#define	    DEL_518_DD_DDB_TABLEINFO	    (QEQ_0CAN_ID)  518
#define	    DEL_519_DD_DDB_TREE		    (QEQ_0CAN_ID)  519
#define	    DEL_520_DD_HISTOGRAMS	    (QEQ_0CAN_ID)  520
#define	    DEL_521_DD_INDEXES		    (QEQ_0CAN_ID)  521
#define	    DEL_522_DD_INDEX_COLUMNS	    (QEQ_0CAN_ID)  522
#define	    DEL_523_DD_UNUSED		    (QEQ_0CAN_ID)  523
#define	    DEL_525_DD_UNUSED		    (QEQ_0CAN_ID)  525
#define	    DEL_528_DD_PROCEDURES	    (QEQ_0CAN_ID)  528
#define	    DEL_529_DD_REGISTRATIONS	    (QEQ_0CAN_ID)  529
#define	    DEL_530_DD_STATS		    (QEQ_0CAN_ID)  530
#define	    DEL_531_DD_TABLES		    (QEQ_0CAN_ID)  531
#define	    DEL_532_DD_VIEWS		    (QEQ_0CAN_ID)  532


/*  INSERT IIDD_ catalog queries */

#define	    INS_600_DD_UNUSED		    (QEQ_0CAN_ID)  600
#define	    INS_601_DD_ALT_COLUMNS	    (QEQ_0CAN_ID)  601
#define	    INS_602_DD_COLUMNS		    (QEQ_0CAN_ID)  602
#define	    INS_605_DD_DDB_DBDEPENDS	    (QEQ_0CAN_ID)  605
#define	    INS_606_DD_DDB_LDBIDS	    (QEQ_0CAN_ID)  606
#define	    INS_607_DD_DDB_LDB_COLUMNS	    (QEQ_0CAN_ID)  607
#define	    INS_608_DD_DDB_LDB_DBCAPS	    (QEQ_0CAN_ID)  608
#define	    INS_614_DD_DDB_LONG_LDBNAMES    (QEQ_0CAN_ID)  614
#define	    INS_615_DD_DDB_OBJECTS	    (QEQ_0CAN_ID)  615
#define	    INS_616_DD_DDB_OBJECT_BASE	    (QEQ_0CAN_ID)  616
#define	    INS_618_DD_DDB_TABLEINFO	    (QEQ_0CAN_ID)  618
#define	    INS_619_DD_DDB_TREE		    (QEQ_0CAN_ID)  619
#define	    INS_620_DD_HISTOGRAMS	    (QEQ_0CAN_ID)  620
#define	    INS_621_DD_INDEXES		    (QEQ_0CAN_ID)  621
#define	    INS_622_DD_INDEX_COLUMNS	    (QEQ_0CAN_ID)  622
#define	    INS_623_DD_UNUSED		    (QEQ_0CAN_ID)  623
#define	    INS_625_DD_UNUSED		    (QEQ_0CAN_ID)  625
#define	    INS_628_DD_UNUSED		    (QEQ_0CAN_ID)  628
#define	    INS_629_DD_REGISTRATIONS	    (QEQ_0CAN_ID)  629
#define	    INS_630_DD_STATS		    (QEQ_0CAN_ID)  630
#define	    INS_631_DD_TABLES		    (QEQ_0CAN_ID)  631
#define	    INS_632_DD_VIEWS		    (QEQ_0CAN_ID)  632
#define	    INS_633_DD_PROCEDURES	    (QEQ_0CAN_ID)  633


/*  UPDATE IIDD_ catalog queries */

#define	    UPD_716_DD_DDB_OBJECT_BASE		(QEQ_0CAN_ID)  716
						/* for orering UPDATE ... query 
						** on IIDD_DDB_OBJECT_CASE */
#define	    UPD_731_DD_TABLES			(QEQ_0CAN_ID)  731
						/* for orering UPDATE ... query 
						** on IIDD_TABLES to toggle 
						** the TABLE_STATS column */
/*  special UPDATE ... queries */

#define	    UPD_800_COM_SQL_LVL			(QEQ_0CAN_ID)  800
						/* for updating COMMON/SQL_LEVEL
						** tuple */
#define	    UPD_801_ING_QUEL_LVL		(QEQ_0CAN_ID)  801
						/* for updating INGRES/QUEL_LEVEL
						** tuple */
#define	    UPD_802_ING_SQL_LVL			(QEQ_0CAN_ID)  802
						/* for updating INGRES/SQL_LEVEL
						** tuple */
#define	    UPD_803_SAVEPOINTS			(QEQ_0CAN_ID)  803
						/* for updating SAVEPOINTS
						** tuple */
#define	    UPD_804_UNIQUE_KEY_REQ		(QEQ_0CAN_ID)  804
						/* for updating UNIQUE_KEY_REQ
						** tuple */
#define	    UPD_805_OPN_SQL_LVL			(QEQ_0CAN_ID)  805
						/* for updating OPEN/SQL_LEVEL
						** tuple */

/*}
** Name: QEQ_1CAN_QRY - Structure for ordering a canned query
**
** Description:
**      Used for ordering SELECT, INSERT, UPDATE query.
**      
** History:
**      20-jul-88 (carl)
**          written
*/

typedef struct _QEQ_1CAN_QRY
{
    i4		     qeq_c1_can_id;	/* unique query id */
    RQB_BIND	    *qeq_c2_rqf_bind_p;	/* ptr to RQF bind elements */
    union
    {
	QEC_D1_DBDEPENDS	    *d1_dbdepends_p;
	QEC_D2_LDBIDS		    *d2_ldbids_p;
	QEC_D3_LDB_COLUMNS	    *d3_ldb_columns_p;
	QEC_D4_LDB_DBCAPS	    *d4_ldb_dbcaps_p;
	QEC_D5_LONG_LDBNAMES	    *d5_long_ldbnames_p;
	QEC_D6_OBJECTS		    *d6_objects_p;
	QEC_D7_OBJECT_BASE	    *d7_object_base_p;
	QEC_D9_TABLEINFO	    *d9_tableinfo_p;
	QEC_D10_TREE		    *d10_tree_p;

	QEC_L1_DBCONSTANTS	    *l1_dbconstants_p;
	QEC_L2_ALT_COLUMNS	    *l2_alt_columns_p;
	QEC_L3_COLUMNS		    *l3_columns_p;
	QEC_L4_DBCAPABILITIES	    *l4_dbcapabilities_p;
	QEC_L5_HISTOGRAMS	    *l5_histograms_p;
	QEC_L6_INDEXES		    *l6_indexes_p;
	QEC_L7_INDEX_COLUMNS	    *l7_index_columns_p;

	QEC_L14_REGISTRATIONS	    *l14_registrations_p;
	QEC_L15_STATS		    *l15_stats_p;
	QEC_L16_TABLES		    *l16_tables_p;
	QEC_L17_VIEWS		    *l17_views_p;
	QEC_L18_PROCEDURES	    *l18_procedures_p;
	QEC_ALT_KEY_INFO	    *alt_key_info_p;
	QEC_MIN_CAP_LVLS	    *min_cap_lvls_p;
    }		     qeq_c3_ptr_u;	/* union of ptrs to tuple structures */
    DD_LDB_DESC	    *qeq_c4_ldb_p;	/* ptr to LDB descriptor */
    bool    	     qeq_c5_eod_b;	/* TRUE if end of data, FALSE if
    					** data is pending */
    i4		     qeq_c6_col_cnt;	/* number of columns in tuple */
    char	    *qeq_c7_qry_p;	/* used for query text buffer */
    i4		     qeq_c8_dv_cnt;	/* DB_DATA_VALUE count, 0 if not
					** used */
    DB_DATA_VALUE   *qeq_c9_dv_p;	/* ptr to first of DB_DATA_VALUE
					** array, NULL if not used  */
}   QEQ_1CAN_QRY;
