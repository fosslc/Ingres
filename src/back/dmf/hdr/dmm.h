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
**      29-Apr-2010 (stial01)
**          Added DM_CORE_REL_CNT, DM_CORE_RINDEX_CNT
**      12-may-2010 (stephenb)
**          Added DM_RELID_FIELD_NO and DM_RELOWNER_FIELD_NO to avoid
**          hard-coding those values in dm2d.c
**      l4-May-2010 (stial01)
**          Fix DM_RELID_FIELD_NO/DM_RELOWNER_FIELD_NO
**	03-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    dmm_message() is static in dmmfind
**	    Prototyped dmm_del_config(), dmm_config()
**/

/*
** location of relid and relowner fields in iirelation. These values are
** used to bootsrap iirel_idx when the database is opened in dm2d.c
*/
#define DM_RELID_FIELD_NO	52
#define DM_RELOWNER_FIELD_NO	51
/*
** DM_CORE_REL_CNT The number of core catalogs initialized in dmm routines
** DM_CORE_RINDEX_CNT The number of core catalogs initialized in dmm routines
** iirelation,iirel_idx,iiattribute, iiindex
*/
#define DM_CORE_REL_CNT	4
#define DM_CORE_RINDEX_CNT DM_CORE_REL_CNT

FUNC_EXTERN DB_STATUS	dmm_alter(DMM_CB *dmm_cb);
FUNC_EXTERN DB_STATUS	dmm_create(DMM_CB *dmm_cb);
FUNC_EXTERN DB_STATUS	dmm_add_location(DMM_CB *dmm_cb);
FUNC_EXTERN DB_STATUS	dmm_delete(DMM_CB *dmm_cb);
FUNC_EXTERN DB_STATUS   dmm_destroy(DMM_CB *dmm_cb);
FUNC_EXTERN DB_STATUS   dmm_finddbs(DMM_CB *dmm_cb);
FUNC_EXTERN DB_STATUS	dmm_list(DMM_CB *dmm_cb);
FUNC_EXTERN DB_STATUS	dmm_del_location(DMM_CB *dmm_cb);
FUNC_EXTERN DB_STATUS	dmm_del_config(DMM_CB *dmm_cb);
FUNC_EXTERN DB_STATUS	dmm_config(DMM_CB *dmm_cb);


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

FUNC_EXTERN DB_STATUS	dmm_init_catalog_templates(void);

