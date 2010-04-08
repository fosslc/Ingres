/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: ASCD.H - Definitions and prototypes for ascd. 
**
** Description:
**      Function prototypes for ascd specific functions.
**
** History: $Log-for RCS$
**      18-Jan-1999 (fanra01)
**          Created.
**      12-Feb-99 (fanra01)
**          Add prototype for ascd_init_sngluser.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

# ifndef ASCD_INCLUDED
# define ASCD_INCLUDED
/* FUNC_EXTERNS */

FUNC_EXTERN DB_STATUS ascd_initiate( CS_INFO_CB  *csib );
FUNC_EXTERN DB_STATUS ascd_terminate(void);
FUNC_EXTERN DB_STATUS ascd_alloc_scb(SCD_SCB  **scb_ptr,
				    GCA_LS_PARMS  *crb,
				    i4  thread_type );
FUNC_EXTERN DB_STATUS ascd_init_sngluser(SCF_CB* scf_cb);

FUNC_EXTERN DB_STATUS ascd_dealloc_scb( SCD_SCB *scb );
FUNC_EXTERN VOID ascd_disconnect( SCD_SCB *scb );
FUNC_EXTERN DB_STATUS ascd_dbinfo_fcn(ADF_DBMSINFO *dbi,
					DB_DATA_VALUE *dvi,
					DB_DATA_VALUE *dvr,
					DB_ERROR *error );
FUNC_EXTERN DB_STATUS ascd_adf_printf( char *cbuf );
FUNC_EXTERN DB_STATUS xscd_init_sngluser( SCF_CB  *scf_cb);
FUNC_EXTERN STATUS ascd_reject_assoc( GCA_LS_PARMS  *crb, STATUS error );
FUNC_EXTERN STATUS ascd_get_assoc( GCA_LS_PARMS *crb, i4  sync );
FUNC_EXTERN VOID ascd_note( DB_STATUS status, i4   culprit );

FUNC_EXTERN DB_STATUS ascd_dbinfo_fcn(ADF_DBMSINFO *dbi, DB_DATA_VALUE *dvi,
			DB_DATA_VALUE *dvr, DB_ERROR *error );
FUNC_EXTERN DB_STATUS ascd_adf_printf( char *cbuf );

# endif /* ASCD_INCLUDED */
