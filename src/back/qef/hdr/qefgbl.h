/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
**
**  Name: QEFGLB.H - QEF global variables
**
**  Description:
**      This file contains all the global variables that are used by the QEF 
**  server.
**
**  History:    $Log-for RCS$
**      05-jul-88 (carl)
**          written
**      05-aug-90 (carl)
**	    removed following obsolete GLOBALREFs
**		IIQE_c0_iildb[], 
**		IIQE_c19_conn_scb,
**		IIQE_c22_ddb_desc, IIQE_c23_conn_info, 
**		IIQE_c25_1ldb_info, IIQE_c26_obj_desc
**/

GLOBALREF   char	IIQE_c1_iidbdb[]; 

GLOBALREF   char	IIQE_c2_abort[]; 

GLOBALREF   char	IIQE_c3_commit[];

GLOBALREF   char	IIQE_c4_rollback[];

GLOBALREF   char	IIQE_c5_low_ingres[];

GLOBALREF   char	IIQE_c6_cap_ingres[];

GLOBALREF   char	*IIQE_c7_sql_tab[];

GLOBALREF   char	*IIQE_c8_ddb_cat[];

GLOBALREF   char	*IIQE_c9_ldb_cat[];

GLOBALREF   char	*IIQE_10_col_tab[];

GLOBALREF   char	*IIQE_11_con_tab[];
