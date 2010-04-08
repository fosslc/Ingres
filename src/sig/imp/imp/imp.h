/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: imp.h
**
## History:
##      02-Jul-2002 (fanra01)
##          Sir 108159
##          Added to distribution in the sig directory.
*/
#define TRUE			1
#define FALSE			0
#define NO_ROWS			0

#define	SERVER_NAME_LENGTH	65
#define	SESSION_ID_LENGTH	33
#define	SHORT_STRING		11
#define SHUTDOWN_HARD		1
#define SHUTDOWN_SOFT		2
#define SHUTDOWN_CLOSE		3
#define SHUTDOWN_OPEN		4

#define	IMA_SESSION		1
#define IIDBDB_SESSION		2
#define USER_SESSION		3

#define RSB_TABLE		2
#define RSB_PAGE		3

#define BARCHART_LENGTH		70

/*
** lock types - stolen from gl!hdr!lk.h
*/
#define LK_DATABASE             1
#define LK_TABLE                2
#define LK_PAGE                 3
#define LK_EXTEND_FILE          4
#define LK_BM_PAGE              5
#define LK_CREATE_TABLE         6
#define LK_OWNER_ID             7
#define LK_CONFIG               8
#define LK_DB_TEMP_ID           9
#define LK_SV_DATABASE          10
#define LK_SV_TABLE             11
#define LK_SS_EVENT             12
#define LK_TBL_CONTROL          13
#define LK_JOURNAL              14
#define LK_OPEN_DB              15
#define LK_CKP_DB               16
#define LK_CKP_CLUSTER          17
#define LK_BM_LOCK              18
#define LK_BM_DATABASE          19
#define LK_BM_TABLE             20
#define LK_CONTROL              21
#define LK_EVCONNECT            22
#define LK_AUDIT                23
#define LK_ROW                  24

#define	LK_DATABASE_NAME	"Database"
#define LK_TABLE_NAME		"Table"
#define LK_PAGE_NAME		"Page"
#define LK_EXTEND_FILE_NAME	"Extend"
#define LK_BM_PAGE_NAME		"BM Page"
#define LK_CREATE_TABLE_NAME	"Create Table"
#define LK_OWNER_ID_NAME	"Owner ID"
#define LK_CONFIG_NAME		"Config"
#define LK_DB_TEMP_ID_NAME	"Temp ID"
#define LK_SV_DATABASE_NAME	"SV Database"
#define LK_SV_TABLE_NAME	"SV Table"
#define LK_SS_EVENT_NAME	"SV Table"
#define LK_TBL_CONTROL_NAME	"Table Control"
#define LK_JOURNAL_NAME		"Journal"
#define LK_OPEN_DB_NAME		"Open DB"
#define LK_CKP_DB_NAME		"CKP DB"
#define LK_CKP_CLUSTER_NAME	"CKP Cluster"
#define LK_BM_LOCK_NAME		"BM Lock"
#define LK_BM_DATABASE_NAME	"BM Database"
#define LK_BM_TABLE_NAME	"BM Table"
#define LK_CONTROL_NAME		"Control"
#define LK_EVCONNECT_NAME	"Event Connect"
#define LK_AUDIT_NAME		"Audit"
#define LK_ROW_NAME		"Row"

/*
** Log Process Status - stolen from lgdef.h
*/
#define                 LPB_ACTIVE          0x000001
#define                 LPB_MASTER          0x000002
#define                 LPB_ARCHIVER        0x000004
#define                 LPB_FCT             0x000008
#define                 LPB_RUNAWAY         0x000010
#define                 LPB_SLAVE           0x000020
#define                 LPB_CKPDB           0x000040
#define                 LPB_VOID            0x000080
#define                 LPB_SHARED_BUFMGR   0x000100
#define                 LPB_IDLE            0x000200
#define                 LPB_DEAD            0x000400
#define                 LPB_DYING           0x000800
#define                 LPB_FOREIGN_RUNDOWN 0x001000
#define                 LPB_CPAGENT         0x002000
