/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: TPFQRY.H - Constants and type for 2PC queries
**
** Description:
**      This file contains constants and type for the 2PC canned queries.
**
** History: $Log-for RCS$
**      15-oct-89 (carl)
**          written
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*}
** Name: TPQ_CANNED_ID - Canned query id
**
** Description:
**      Used to order a canned query.
**      
** History:
**      15-oct-89 (carl)
**          written
*/

typedef i4	TPQ_CANNED_ID;


/*  DELETE queries on 2PC-related catalogs */

#define	    DEL_10_DXLOG_BY_DXID		(TPQ_CANNED_ID)  10
#define	    DEL_11_DXLOG_BY_STARID		(TPQ_CANNED_ID)  11

#define	    DEL_20_DXLDBS_BY_DXID		(TPQ_CANNED_ID)  20


/*  INSERT queries on 2PC-related catalogs */

#define	    INS_10_DXLOG			(TPQ_CANNED_ID)  10

#define	    INS_20_DXLDBS			(TPQ_CANNED_ID)  20


/*  SELECT queries on 2PC-related catalogs */

#define	    SEL_10_DXLOG_CNT			(TPQ_CANNED_ID)  10
#define	    SEL_11_DXLOG_ALL			(TPQ_CANNED_ID)  11

#define	    SEL_20_DXLDBS_CNT_BY_DXID		(TPQ_CANNED_ID)  20
#define	    SEL_21_DXLDBS_ALL_BY_DXID		(TPQ_CANNED_ID)  21

#define	    SEL_30_CDBS_CNT			(TPQ_CANNED_ID)  30
#define	    SEL_31_CDBS_ALL			(TPQ_CANNED_ID)  31

#define	    SEL_40_DBCAPS_BY_2PC		(TPQ_CANNED_ID)  40

#define	    SEL_50_CLUSTER_FUNC			(TPQ_CANNED_ID)  50

#define	    SEL_60_TABLES_FOR_DXLOG		(TPQ_CANNED_ID)  60


/* UPDATE queries on 2PC-related catalogs  */

#define	    UPD_10_DXLOG_STATE			(TPQ_CANNED_ID)  10


/*}
** Name: TPQ_QRY_CB - Structure for ordering a canned query
**
** Description:
**      Used for ordering DELETE, SELECT, INSERT, UPDATE query.
**      
** History:
**      15-oct-89 (carl)
**          written
*/

typedef struct _TPQ_QRY_CB
{
#define	TPQ_MAX_BIND_COUNT  TPC_01_CC_DXLOG_13
					/* must be set to the column count of
					 * the largest row of all catalogs
					 */

    TPQ_CANNED_ID    qcb_1_can_id;	/* unique query id */

    union
    {
	PTR                  u0_ptr;	/* generic pointer */
	TPC_D1_DXLOG	    *u1_dxlog_p;
	TPC_D2_DXLDBS	    *u2_dxldbs_p;
	TPC_I1_STARCDBS	    *u3_starcdbs_p;
	TPC_L1_DBCAPS	    *u4_dbcaps_p;
    }		     qcb_3_ptr_u;	/* union of ptrs to catalog structures 
					*/
    DD_LDB_DESC	    *qcb_4_ldb_p;	/* ptr to LDB descriptor */
    bool    	     qcb_5_eod_b;	/* TRUE if end of data, FALSE if
    					** data is pending */
    i4		     qcb_6_col_cnt;	/* used to specify number of columns 
					** in result tuple */
    char	    *qcb_7_qry_p;	/* used for constructing query text */
    i4		     qcb_8_dv_cnt;	/* DB_DATA_VALUE count, 0 if not
					** used */
    DB_DATA_VALUE    qcb_9_dv[TPQ_MAX_BIND_COUNT];	
					/* array for DB_DATA_VALUEs */
    i4               qcb_10_total_cnt;	/* result for select count(*) */
    RQB_BIND	     qcb_11_rqf_bind[TPQ_MAX_BIND_COUNT];
					/* array of maximum number of RQF bind 
					 * elements for fetching catalog 
					 * columns */
    i4		     qcb_12_select_cnt;	/* number of tuples read for a catalog
					 * query, e.g., from iistar_cdbs,
					 * iidd_ddb_dxldbs */
}   TPQ_QRY_CB;
