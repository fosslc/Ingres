/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# define	QGSPSIZE		20

/*}
** Name:	QGSPCHUNK	- A block  (chunk) of QRY_SPECs for a QGSAVESPEC
**
** Description:
**	This is a block of QRY_SPECs that are pointed at by a QGSAVESPEC.
**	A QGSPCHUNK contains QGSPSIZE QRY_SPECs.  If also has a pointer
**	to a QGSPCHUNK so that a list of QGSPCHUNKs can be pointed to
**	by a single QGSAVESPEC. 
**
** History:
**	22-may-1987 (Joe)
**		Initial Version
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
typedef struct _QGSPCHUNK
{
    struct _QGSPCHUNK	*qgsp_next;
    QRY_SPEC		qgsp_specs[QGSPSIZE];
} QGSPCHUNK;

/*}
** Name:	QGSAVESPEC	- Structure to hold QRY_SPECs
**
** Description:
**	This structure is used to hold the saved QRY_SPECs for a single
**	QS_VALGEN QRY_SPEC.  It points at a list of QGSPCHUNKs that contain
**	the actual QRY_SPECs.  Most of the information in this structure
**	is used to maintain the list of QGSPCHUNKs.
**
** Members:
**	qgsp_numcur	Contains the offset of the next available
**			QRY_SPEC in the QGSPCHUNK pointed to by qgsp_cur.
**			If this is >= QGSPSIZE, then the current chunk
**			is full.
**
**	qgsp_first	This points to the first QGSPCHUNK in the list
**			of QGSPCHUNKs.
**
**	qgsp_cur	This points to the current chunk in the list of
**			QGSPCHUNKs.  It is the one that contains the
**			next available slot (unless it is full,  in which
**			case a new chunk will need to added to the list
**			if another slot is needed).
**
** History:
**	22-may-1987 (Joe)
**		Initial Version
*/
typedef struct
{
    i4		qgsp_numcur;
    QGSPCHUNK	*qgsp_first;
    QGSPCHUNK	*qgsp_cur;
} QGSAVESPEC;

/*}
** Name:	QGSAVELIST	- A node for a list of saved QRY_SPECs
**
** Description:
**	This is the node for a list of saved QRY_SPECS.  Each element
**	in the list points to a group of saved QRY_SPECs.
**
** History:
**	22-may-1987 (Joe)
**		Initial Version
*/
typedef struct _QGSAVELIST
{
    struct _QGSAVELIST	*qgsl_next;
    QGSAVESPEC		*qgsl_specs;
} QGSAVELIST;

/**
** Name:    qgi.h -	Query Generator Module
**				Internal State Structure Definition.
** Description:
**	This file contains the declarations of internal data structures
**	and constants for the QG module.
**
** History:
**	9-jun-1987 (Joe)
**		Added qg_dbvs field.  If the target list of a version 0
**		query (which is mapped to a file) has a %a, then
**		qg_dbvs will point to an array of dbvs. Note that if
**		the target list has one %a than all elements must be %a.
**		The data pointers of the dbvs will point at the buffer
**		used to map the query to a file.
*/

typedef struct qgint
{
    i4			qg_state;
    FILE		*qg_file;
    char		*qg_buffer;
    i4			qg_bsize;
    PTR			*qg_margv;
    i4			qg_mcnt;
    i4			*qg_msize;
    char		*qg_lbuf;
    LOCATION		qg_loc;
    i4			qg_tupcnt;	/* Number of rows retrieve so far */
    i2			qg_wtag;
    QGSAVELIST		*qg_where;	
    DB_DATA_VALUE	*qg_dbvs;
} QSTRUCT;

QSTRUCT	*IIQG_alloc();

/*
** Values for the state.
*/
# define	QS_INIT		01
# define	QS_IN		02
# define	QS_INDETAIL	010
# define	QS_UNIQUE	020
# define	QS_MAP		0100
# define	QS_QBF		0200
# define	QS_OSL		01000
# define	QS_SAVWHERE	02000	/* If where clause should be saved */
