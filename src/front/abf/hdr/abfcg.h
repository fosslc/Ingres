/*
** Copyright (c) 2004 Ingres Corporation
** All rights reserved.
*/


/*
** Name: abfcg.h	-	Definitions used in both ABF and code generator
**
** Description:
**	Contains definitions used in both the ABF runtime system
**	and the code generator.
**
** History:
**	29-may-1987 (agh)
**		First written.
**
*/

/*
** The following constants define the offsets into the qg_base array
** within a QDESC structure.  When a query is passed between frames,
** the 2 bases refer to the DB_DATA_VALUE arrays within the calling
** and called frames.  When a query is run within a single frame,
** then only the zero-th base is relevant.
*/
# define	ABCG_ORIGBASE		0	/* base in calling frame */
# define	ABCG_CALLEEBASE		1	/* base in callee */
