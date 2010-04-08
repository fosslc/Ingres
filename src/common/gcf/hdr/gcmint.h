/*
** Copyright (c) 1992, Ingres Corporation
*/

/**
** Name: GCMINT.H - GCM internal structures and definitions
**
** Description:
**      This file contains the declarations of functions and 
**	definitions of data structures used internally by GCM.
**
** History: 
**	11-Sep-92 (brucek)
**	    Extracted from gcagcm.c.
**	22-Sep-92 (brucek)
**	    Use ANSI C function prototypes.
**	14-Mar-94 (seiwald)
**	    Altered gcm_response() interface so as not to take a SVC_PARMS.
**	20-Nov-95 (gordy)
**	    Updated prototypes.
**	 3-Sep-96 (gordy)
**	    Added GCA control blocks.
**	10-Apr-97 (gordy)
**	    Separated GCA and CL parts of SVC_PARMS.
**	27-Jan-98 (gordy)
**	    Added trusted & user_name parameters to gcm_response() to
**	    validate user privileges.
**	21-May-98 (gordy)
**	    Pass in length of buffers to gcm_response().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/*
**  forward and/or external function references.
*/

FUNC_EXTERN i4		gcm_put_str( char *p, char *s );
FUNC_EXTERN i4		gcm_put_int( char *p, i4  i );
FUNC_EXTERN i4		gcm_put_long( char *p, i4 i );
FUNC_EXTERN i4		gcm_get_str( char *p, char **ss, i4  *l );
FUNC_EXTERN i4		gcm_get_int( char *p, i4  *i );
FUNC_EXTERN i4		gcm_get_long( char *p, i4 *i );
FUNC_EXTERN VOID	gcm_reg_trap( GCA_SVC_PARMS *svc_parms );
FUNC_EXTERN VOID	gcm_unreg_trap( GCA_SVC_PARMS *svc_parms );
FUNC_EXTERN STATUS	gcm_set_delivery( GCA_SVC_PARMS *svc_parms );
FUNC_EXTERN STATUS	gcm_deliver( VOID );
FUNC_EXTERN STATUS	gcm_response( GCA_CB *, GCA_ACB *, 
				      bool trusted, char *user_name,
				      GCA_MSG_HDR *in_hdr, char *in_data,
				      i4  in_len, GCA_MSG_HDR *out_hdr, 
				      char *out_data, i4  out_len );

