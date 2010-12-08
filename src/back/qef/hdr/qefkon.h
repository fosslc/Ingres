/*
**Copyright (c) 2004 Ingres Corporation
*/

#ifndef QEFKON_H_INCLUDED
#define QEFKON_H_INCLUDED

/**
**
**  Name: QEFKON.H - Internal QEF constants 
**
**  Description:
**      This file contains all the constants used by DDB QEF.
**
**  History:    $Log-for RCS$
**      05-jul-88 (carl)
**          written
**      03-jun-90 (carl)
**	    removed define QEK_050TP_SAVE_TMP_TABLES
**      02-aug-90 (carl)
**	    added QEK_70_LEN
**	14-jun-91 (teresa)
**	    added QEK_14CAP_TID_LEVEL for bug 34684
**	11-nov-92 (fpang)
**	    Added QEK_605CAP_LEVEL for temp table support.
**	01-mar-93 (barbara)
**	    Added QEK_15CAP_OPENSQL_LEVEL and QEK_16CAP_DELIMITED_NAME_CASE
**	    for Star delimited id support.
**	02-sep-93 (barbara)
**	    Added QEK_17CAP_REAL_USER_NAME_CASE for Star delimited id support.
**	    Also added QEK_12NAME_OPENSQL_LEVEL to index into IIQE_27_cap_names.
**	    This table is used when selecting the minimum capability from
**	    iidd_ddb_ldb_dbcaps and again when updating iidd_ddb_capabilities.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-Jan-2009 (kibro01) b121521
**	    Add 910 capability level
**	2-Dec-2010 (kschendel)
**	    Multi-include protection.
**/


/*  Miscellaneous constants */

#define	    QEK_0_TIME_QUANTUM		0

#define	    QEK_5_SIZE_OF_PREFIX	5	/* string "long_" */
#define	    QEK_6_SIZE_OF_INGRES	6	/* string "INGRES" */

#define	    QEK_0MAX_NDX_COUNT		30	/* used for promoting indexes */
#define	    QEK_1MAX_DEP_COUNT		30	/* used for views */

#define	    QEK_600CAP_LEVEL		600	/* lowest level for 
						** capabilities */
#define	    QEK_601CAP_LEVEL		601	/* next lowest level */
#define	    QEK_605CAP_LEVEL		605	/* Ingres 65 */
#define	    QEK_910CAP_LEVEL		910	/* Ingres 9.1.0 */


#define	    QEK_0DBMS_INGRES_Y		0	/* INGRES DBMS_TYPE */
#define	    QEK_1DBMS_INGRES_N		1	/* non-INGRES DBMS_TYPE */

#define	    QEK_0FOR_N			0	/* NO */
#define	    QEK_1FOR_Y			1	/* YES */

#define	    QEK_0IS_LO			0	/* YES */
#define	    QEK_1IS_HI			1	/* NO */

#define	    QEK_0AS_P			0	/* P for PHYSICAL_SOURCE */
#define	    QEK_1AS_T			1	/* T for PHYSICAL_SOURCE */


/*  TPF-specific constants  */

#define	    QEK_0TPF_NONE		0	/* none */
#define	    QEK_2TPF_READ		2	/* retrievals on an LDB */
#define	    QEK_3TPF_UPDATE		3	/* updates on an LDB */
#define	    QEK_4TPF_1PC		4	/* used to fake 2PC for internal
						** catalog updates, to be 
						** changed to QEK_3TPF_UPDATE
						** when 2PC is implemented */


/*  Size constants  */

#define	    QEK_1_CHAR_SIZE		1	/* 1-char size */
#define	    QEK_8_CHAR_SIZE		8	/* 8-char size */
#define	    QEK_16_STOR_SIZE		16	/* storage size */
#define	    QEK_24_LOCN_SIZE		24	/* location size */
#define	    QEK_228_HIST_SIZE 		228	/* histogram text segment size 
						*/
#define	    QEK_240_TEXT_SIZE 		240	/* iidd_ddb_registrations text 
						** segment length */
#define	    QEK_256_VIEW_SIZE		256	/* view text segment size */
#define	    QEK_1024_TREE_SIZE		1024	/* tree segment size */


/*  Column count constants */

#define	    QEK_0_COL_COUNT	0
#define	    QEK_1_COL_COUNT	1
#define	    QEK_2_COL_COUNT	2
#define	    QEK_3_COL_COUNT	3
#define	    QEK_4_COL_COUNT	4
#define	    QEK_5_COL_COUNT	5
#define	    QEK_6_COL_COUNT	6
#define	    QEK_7_COL_COUNT	7
#define	    QEK_8_COL_COUNT	8
#define	    QEK_9_COL_COUNT	9
#define	    QEK_10_COL_COUNT	10
#define	    QEK_11_COL_COUNT	11
#define	    QEK_12_COL_COUNT	12
#define	    QEK_13_COL_COUNT	13
#define	    QEK_14_COL_COUNT	14
#define	    QEK_15_COL_COUNT	15
#define	    QEK_16_COL_COUNT	16
#define	    QEK_17_COL_COUNT	17
#define	    QEK_18_COL_COUNT	18
#define	    QEK_19_COL_COUNT	19
#define	    QEK_20_COL_COUNT	20


/*  Length constants */

#define	    QEK_005_LEN		5
#define	    QEK_010_LEN		10
#define	    QEK_015_LEN		15
#define	    QEK_050_LEN		50
#define	    QEK_055_LEN		55
#define	    QEK_070_LEN		70
#define	    QEK_100_LEN		100
#define	    QEK_200_LEN		200
#define	    QEK_300_LEN		300
#define	    QEK_400_LEN		400
#define	    QEK_500_LEN		500
#define	    QEK_600_LEN		600
#define	    QEK_700_LEN		700
#define	    QEK_800_LEN		800
#define	    QEK_900_LEN		900


/*  Capability codes */

#define	    QEK_0CAP_UNKNOWN		(i4) 0
					    /* unrecognized capability */
#define	    QEK_1CAP_COMMONSQL_LEVEL	(i4) 1
					    /* COMMON/SQL_LEVEL */
#define     QEK_2CAP_DBMS_TYPE		(i4) 2
					    /* DBMS_TYPE */
#define     QEK_3CAP_DB_NAME_CASE	(i4) 3
					    /* DB_NAME_CASE */
#define     QEK_4CAP_DISTRIBUTED	(i4) 4
					    /* DISTRIBUTED */
#define	    QEK_5CAP_INGRES		(i4) 5
					    /* INGRES */
#define	    QEK_6CAP_INGRESQUEL_LEVEL	(i4) 6
					    /* INGRES/QUEL_LEVEL */
#define	    QEK_7CAP_INGRESSQL_LEVEL	(i4) 7
					    /* INGRES/SQL_LEVEL */
#define	    QEK_8CAP_SAVEPOINTS		(i4) 8
					    /* SAVEPOINTS */
#define	    QEK_9CAP_SLAVE2PC		(i4) 9
					    /* SLAVE2PC */
#define	    QEK_10CAP_TIDS		(i4) 10
					    /* TIDS */
#define     QEK_11CAP_UNIQUE_KEY_REQ	(i4) 11
					    /* UNIQUE_KEY_REQ */
#define     QEK_12CAP_OWNER_NAME	(i4) 12
					    /* OWNER_NAME */
#define     QEK_13CAP_PHYSICAL_SOURCE	(i4) 13
					    /* PHYSICAL_SOURCE */
#define	    QEK_14CAP_TID_LEVEL		(i4) 14
					    /* TID_LEVEL */
#define	    QEK_15CAP_OPENSQL_LEVEL	(i4) 15
					    /* OPEN/SQL_LEVEL */
#define	    QEK_16CAP_DELIM_NAME_CASE	(i4) 16
					    /* DB_DELIMITED_CASE */
#define	    QEK_17CAP_REAL_USER_CASE	(i4) 17
					    /* DB_REAL_USER_CASE */

/*  Capability value codes */

#define	    QEK_0VAL_UNKNOWN		(i4) 0
					    /* unrecognized capability value */
#define	    QEK_1VAL_Y			(i4) 1
					    /* Y */
#define	    QEK_2VAL_N			(i4) 2
					    /* N */
#define	    QEK_3VAL_NONE		(i4) 3
					    /* NONE */
#define	    QEK_4VAL_STAR		(i4) 4
					    /* STAR */
#define	    QEK_5VAL_LOWER		(i4) 5
					    /* LOWER */
#define	    QEK_6VAL_MIXED		(i4) 6
					    /* MIXED */
#define	    QEK_7VAL_UPPER		(i4) 7
					    /* UPPER */
#define	    QEK_8VAL_NO			(i4) 8
					    /* NO */
#define	    QEK_9VAL_YES		(i4) 9
					    /* YES */
#define	    QEK_10VAL_QUOTED		(i4) 10
					    /* QUOTED */
#define	    QEK_11VAL_P			(i4) 11
					    /* P */


/*  Constants for indexing into table of capability names in QEDKON.ROC  */

#define	    QEK_0NAME_COMMONSQL_LEVEL	0
					    /* COMMON/SQL_LEVEL */
#define     QEK_1NAME_DBMS_TYPE		1
					    /* DBMS_TYPE */
#define     QEK_2NAME_DB_NAME_CASE	2
					    /* DB_NAME_CASE */
#define     QEK_3NAME_DISTRIBUTED	3
					    /* DISTRIBUTED */
#define	    QEK_4NAME_INGRES		4
					    /* INGRES */
#define	    QEK_5NAME_INGRESQUEL_LEVEL	5
					    /* INGRES/QUEL_LEVEL */
#define	    QEK_6NAME_INGRESSQL_LEVEL	6
					    /* INGRES/SQL_LEVEL */
#define	    QEK_7NAME_SAVEPOINTS	7
					    /* SAVEPOINTS */
#define	    QEK_8NAME_SLAVE2PC		8
					    /* SLAVE2PC */
#define	    QEK_9NAME_TIDS		9
					    /* TIDS */
#define     QEK_10NAME_UNIQUE_KEY_REQ	10
					    /* UNIQUE_KEY_REQ */
#define     QEK_11NAME_OWNER_NAME	11
					    /* OWNER_NAME */
#define     QEK_12NAME_OPENSQL_LEVEL	12
					    /* OPEN/SQL_LEVEL */


/*  Constants for indexing into SQL keyword table in QEDKON.ROC  */

#define	    SQL_00_AND	    0
#define	    SQL_01_DELETE   1
#define	    SQL_02_FROM	    2
#define	    SQL_03_INTO	    3
#define	    SQL_04_INSERT   4
#define	    SQL_05_LIKE	    5
#define	    SQL_06_OR	    6
#define	    SQL_07_SELECT   7
#define	    SQL_08_SET	    8
#define	    SQL_09_UPDATE   9
#define	    SQL_10_VALUES   10
#define	    SQL_11_WHERE    11


/*  Constants for indexing into the table of constant names in QEDKON.ROC  */

#define	    QEK_0CON_DBA_NAME	    0
#define	    QEK_1CON_USER_NAME	    1


/*}
** Name: QEK_K1_STR_CODE - string and associated code
**
** Description:
**      This structure is used for constructing tables with entries containing
**  a string and its associated code.
**
** History:
**      23-jun-88 (carl)
**          written
*/

typedef struct _QEK_K1_STR_CODE
{
    char	*qek_s1_str_p;	    /* string */
    i4		 qek_s2_code;	    /* associated code */
}   QEK_K1_STR_CODE;


#endif /* QEFKON_H_INCLUDED */
