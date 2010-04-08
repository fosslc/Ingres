
/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	qry.h	-	Column and table descriptors for graphics
**
** Description:
**
** History:
**	10/02/92 (dkh) - Added support for owner.table, etc.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

typedef struct
{
	char		*name;		/* column name */
	i4		type;		/* graphics data type (GRDV_...) */
	DB_DATA_VALUE	db;		/* ADT retrieval area for INGRES */
	DB_DATA_VALUE	dbf;		/* ADT retrieval area for field */
	ADI_FI_ID	funcid;		/* function id, if needed */
	DB_DATA_VALUE	res;		/* RESULT dbd value if func needed */
	bool		qflag;		/* TRUE if in query */
	bool		xflag;		/* TRUE if x column */
	bool		fld;		/* TRUE if dbf valid */
	bool		supported;	/* TRUE if datatype is supported */
} COLDATA;


/*  Table/view descriptor */
typedef struct t_att
{
	struct t_att *link;
	i2 tag;
	char *name;
	i4 colnum;
	i4 scolnum;
	i4 skip;
	COLDATA col[DB_GW2_MAX_COLS];
} TABLEATT;

