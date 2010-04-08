/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DMM.H - Database Maintenance Routines interface definitions.
**
** Description:
**      This file describes the interface to the DMM Maintenance
**	services.
**
** History:
**	 7-jul-1992 (rogerk)
**	    Created for DMF Prototyping.
**	24-nov-92 (rickh)
**	    Added dmm_init_catalog_templates( ).
**	14-dec-92 (jrb)
**	    Change protos for dmm_add_create and dmm_loc_validate.
**	07-feb-94 (swm)
**	    Bug #59609
**	    Changed type of arglist from i4  * to PTR * in dmm_putlist
**	    function declaration since it is used to pass an array of
**	    pointers. This avoids pointer truncation on platforms where
**	    sizeof(PTR) > sizeofof(i4), eg. ap_osf.
**	12-mar-1999 (nanpr01)
**	    Integrated Support for raw locations.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	12-Mar-2001 (jenjo02)
**	    Removed raw_start, raw_size from dmm_loc_validate()
**	    prototype, not used.
**	19-mar-2001 (stephenb)
**	    Add ucollation parm to dmm_add_create
**	05-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code.
**/

FUNC_EXTERN DB_STATUS	dmm_alter(DMM_CB *dmm_cb);
FUNC_EXTERN DB_STATUS	dmm_create(DMM_CB *dmm_cb);
FUNC_EXTERN DB_STATUS	dmm_add_location(DMM_CB *dmm_cb);
FUNC_EXTERN DB_STATUS	dmm_delete(DMM_CB *dmm_cb);
FUNC_EXTERN DB_STATUS   dmm_destroy(DMM_CB *dmm_cb);
FUNC_EXTERN DB_STATUS   dmm_finddbs(DMM_CB *dmm_cb);
FUNC_EXTERN DB_STATUS	dmm_list(DMM_CB *dmm_cb);
FUNC_EXTERN DB_STATUS	dmm_del_location(DMM_CB *dmm_cb);


FUNC_EXTERN DB_STATUS	dmm_add_create(
				    DML_XCB		*xcb,
				    DB_DB_NAME		*db_name,
				    DB_OWN_NAME		*db_owner,
				    i4		db_id,
				    i4		service,
				    i4		access,
				    DB_LOC_NAME		*location_name,
				    i4		l_root,
				    char		*root,
				    i4		loc_count,
				    DMM_LOC_LIST	*loc_list[],
				    char		*collation,
				    char		*ucollation,
				    DB_ERROR		*dberr);

FUNC_EXTERN DB_STATUS	dmm_loc_validate(
				    i4		type,
				    DB_DB_NAME		*dbname,
				    i4		l_area,
				    char		*area,
				    char		*area_dir,
				    i4		*l_area_dir,
				    char		*path,
				    i4		*l_path,
				    i4		*flags,
				    DB_ERROR		*dberr);
FUNC_EXTERN DB_STATUS	dmm_do_del(
				    char		*path,
				    u_i4		pathlen,
				    char		*fil,
				    u_i4		fillen,
				    DB_ERROR		*dberr);

FUNC_EXTERN DB_STATUS	dmm_message();

FUNC_EXTERN DB_STATUS	dmm_makelist(
				    DMP_DCB		*dcb,
				    DML_XCB		*xcb,
				    i4		flag,
				    DB_TAB_NAME		*table,
				    DB_OWN_NAME		*owner,
				    DB_ATT_NAME		*att_name,
				    char		*path,
				    i4		pathlen,
				    i4		*tuple_cnt,
				    DB_ERROR		*dberr);

FUNC_EXTERN DB_STATUS	dmm_putlist(
				    PTR			*arglist,
				    char		*name,
				    i4			name_len,
				    CL_ERR_DESC		*sys_err);

FUNC_EXTERN DB_STATUS	dmm_init_catalog_templates( );

