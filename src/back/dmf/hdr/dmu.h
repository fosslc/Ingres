/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DMU.H - Utility interface definitions.
**
** Description:
**      This file describes the interface to the Utility level
**	services.
**
** History:
**	6-jul-1992 (bryanp)
**	    Prototyping DMF.
**	30-October-1992 (rmuth)
**	    Add dmu_convert.
**      22-jul-1996 (ramra01 for bryanp)
**          Added dmu_atable().
**	19-may-1998 (nanpr01)
**	    Added function prototype.
**	12-Jul-2006 (jonj)
**	    Add dmuIndexSetup()
**/

FUNC_EXTERN DB_STATUS	dmu_alter(DMU_CB    *dmu_cb);
FUNC_EXTERN DB_STATUS	dmu_create(DMU_CB   *dmu_cb);
FUNC_EXTERN DB_STATUS	dmu_destroy(DMU_CB  *dmu_cb);
FUNC_EXTERN DB_STATUS	dmu_index(DMU_CB    *dmu_cb);
FUNC_EXTERN DB_STATUS	dmu_pindex(DMU_CB    *dmu_cb);
FUNC_EXTERN DB_STATUS	dmu_modify(DMU_CB   *dmu_cb);
FUNC_EXTERN DB_STATUS	dmu_relocate(DMU_CB *dmu_cb);
FUNC_EXTERN DB_STATUS	dmu_show(DMU_CB     *dmu_cb);
FUNC_EXTERN DB_STATUS	dmu_tabid(DMU_CB    *dmu_cb);
FUNC_EXTERN DB_STATUS	dmu_convert(DMU_CB  *dmu_cb);
FUNC_EXTERN DB_STATUS	dmu_atable(DMU_CB   *dmu_cb);	
FUNC_EXTERN DB_STATUS	dmuIndexSetup(DMU_CB   		*dmu,
				      PTR		indx_cb);	
