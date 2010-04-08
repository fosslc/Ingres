/*
** Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    05-Sep-2000 (hanch04)
**        replace nat and longnat with i4
*/
#ifndef COLLISIO_H_INCLUDED
#define COLLISIO_H_INCLUDED

#include <compat.h>
#include <gl.h>
#include <iicommon.h>

/**
** Name:     - collision structures
**
**      Created based on structures in collisn.h.
**/

#define TX_INSERT  1
#define TX_UPDATE  2
#define TX_DELETE  3

/* RPdb_error_check error information */

typedef struct
{
    i4  errorno;
    i4  rowcount;
} DBEC_ERR_INFO;

/* edit_type values for RPedit_name */

#define EDNM_ALPHA 1
#define EDNM_DELIMIT   2
#define EDNM_SLITERAL  3
#define EDNM_CLITERAL  4

/* return values for RPdb_error_check */

#define DBEC_ERROR     1
#define DBEC_DEADLOCK      2
/* Maximum length of a delimited name */

#define DLMNAME_LENGTH 68

typedef struct
{
    i2  sourcedb;
    i4  transaction_id;
    i4  sequence_no;
    i4  table_no;
    i2  old_source_db;
    i4  old_transaction_id;
    i4  old_sequence_no;
    char TransTime[26];
    i4  nResolvedCollision;
} COLLISION;

typedef struct _collisions_found
{
    i2  local_db;
    i2  remote_db;
    i4 type;       /* INSERT, UPDATE, DELETE */
    bool    resolved;
    i2  nSvrTargetType;
    COLLISION   db1;
    COLLISION   db2;
} COLLISION_FOUND;

typedef struct _visualcolonneinfo
{
	char	column_name[DB_MAXNAME+1];
	long	key_sequence;
	char	dataColSource[DB_MAXNAME+1];
	char	dataColTarget[DB_MAXNAME+1];
}VISUALCOL;

typedef struct _visualsequence
{
	char	table_name[DB_MAXNAME+1];
	char	table_owner[DB_MAXNAME+1];
	int		nb_Col;
	i4		sequence_no;
	i2		cdds_no;
	i4		type;	/* INSERT, UPDATE, DELETE */
	i4		transaction_id; //COLLISION_FOUND.db1.transaction_id
	i4		old_transaction_id; // COLLISION_FOUND.db1.old_transaction_id
	char	Sourcedb[DB_MAXNAME+1];		//SESSION.database_name
	char	Targetdb[DB_MAXNAME+1];		//SESSION.database_name
	char	Localdb[DB_MAXNAME+1];		//SESSION.database_name
	char	SourceVnode[DB_MAXNAME+1];	//SESSION.vnode_name
	char	TargetVnode[DB_MAXNAME+1];	//SESSION.vnode_name
	char	LocalVnode[DB_MAXNAME+1];	//SESSION.vnode_name
	char	SourceTransTime[26];
	char	TargetTransTime[26];
	int		nSourceDB;
	int		nTblNo;
	int		SourceResolvedCollision;
	int		nSvrTargetType;
}VISUALSEQUENCE;

/**     Created based on rpu.h in replicator library.
**/

/* flag values for RPdb_error_check */

#define DBEC_SINGLE_ROW    (1 << 0)
#define DBEC_ZERO_ROWS_OK  (1 << 1)

#define DBEC_DEADLOCK      2
/**
** Name:    tblobjs.h - table objects
**
** Description:
**  Defines table object name constants and types.
**
**      Created based on tblobjs.h .
**/

#define TBLOBJ_MIN_TBLNO   1       /* min table number */
#define TBLOBJ_MAX_TBLNO   99999   /* max table number */
#define TBLOBJ_TBLNO_NDIGITS   5   /* max digits in table no. */

#define TBLOBJ_INS_FUNC    1       /* insert table function */
#define TBLOBJ_UPD_FUNC    2       /* update table function */
#define TBLOBJ_DEL_FUNC    3       /* delete table function */
#define TBLOBJ_QRY_FUNC    4       /* generic query table func. */

#define TBLOBJ_ARC_TBL     5       /* archive table */
#define TBLOBJ_SHA_TBL     6       /* shadow table */
#define TBLOBJ_TMP_TBL     7       /* temporary table */

#define TBLOBJ_REM_INS_PROC    8   /* remote insert procedure */
#define TBLOBJ_REM_UPD_PROC    9   /* remote update procedure */
#define TBLOBJ_REM_DEL_PROC    10  /* remote delete procedure */

#define TBLOBJ_SHA_INDEX1  11      /* shadow table index 1 */

#define TBLOBJ_NOBJ_TYPES  11      /* number of object types */

typedef char *TBLOBJ_NAMES[TBLOBJ_NOBJ_TYPES];  /* table objects array */

#define     MAX_COLS    IISQ_MAX_COLS
#define     DLNM_SIZE   128

#endif

