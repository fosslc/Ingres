/*
**Copyright (c) 2004 Ingres Corporation
**
**
**	dma.h	- DMF auditing routines.
**
** History:
**	20-jan-1993 (unknown)
**	    Created.
**	10-mar-94 (stephenb)
**	    Added DB_DB_NAME to proto.
**	23-may-1994 (andys)
**	    Remove bogus trigraph from comments.
**	26-may-1993 (robf)
**	    Added prototypes for new security functions
**	23-june-1998(angusm)
**	    Add prototype for dma_write_audit1
**	 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Dec-2008 (jonj)
**	    SIR 120874: dma_? auditing functions converted to DB_ERROR *
*/
DB_STATUS
dma_write_audit (
       SXF_EVENT,           
       SXF_ACCESS,        
       char *,         	/* Object name */
       i4,		/* Object name length */
       DB_OWN_NAME*,    /* Object owner */
       i4,         /* Message ID */
       bool,       	/* FORCE flag */
       DB_ERROR*,   	/* Error handler */
       DB_DB_NAME*	/* database name */
);

DB_STATUS
dma_write_audit1 (
       SXF_EVENT,           
       SXF_ACCESS,        
       char *,         	/* Object name */
       i4,		/* Object name length */
       DB_OWN_NAME*,    /* Object owner */
       i4,         /* Message ID */
       bool,       	/* FORCE flag */
       DB_ERROR*,   	/* Error handler */
       DB_DB_NAME*,	/* database name */
       PTR,		/* details text ptr */
       i4,		/* text length */
       i4		/* detail integer */
);

DB_STATUS
dma_row_access (
	i4,	/* Access type */
	DMP_RCB*, /* RCB being accessed */
	char*,	  /* Tuple record */
	char*,    /* Old tuple for updates */
	i4*,	  /* Result of MAC comparison */
	DB_ERROR* /* err_code location */
);

DB_STATUS
dma_table_access (
	i4,	/* Access type */
	DMP_RCB*, /* RCB being accessed */
	i4*,	  /* Result of MAC comparison */
	bool,	  /* Do auditing or not  */
	DB_ERROR*  /* err_code location */
);

/*
** Access types to be used in access parameters
*/
# define DMA_ACC_SELECT	 0x001	/* Select */
# define DMA_ACC_INSERT  0x002	/* Insert */
# define DMA_ACC_BYTID   0x004	/* Access by TID */
# define DMA_ACC_BYKEY   0x008	/* Access by Key */
# define DMA_ACC_UPDATE  0x010	/* Update */
# define DMA_ACC_DELETE  0x020  /* Delete */

# define DMA_ACC_NOPRIV  0	/* Can't do it */
# define DMA_ACC_PRIV    1	/* Can do it */
# define DMA_ACC_POLY    2	/* Can only do it by polyinstantiation */
