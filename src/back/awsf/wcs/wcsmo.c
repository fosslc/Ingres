/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
** wcsmo.c -- MO module for wcs
**
** Description
**
**
**
**
** Functions:
**
**      wcs_loc_id_get
**      wcs_loc_name_get
**      wcs_loc_path_get
**      wcs_loc_type_get
**      wcs_loc_ext_get
**      wcs_loc_counter_get
**
** History:
**      02-Mar-98 (fanra01)
**          Created.
**
*/
# include <compat.h>
# include <sp.h>
# include <gl.h>
# include <erglf.h>
# include <mo.h>
# include <ddfcom.h>

# include <wcslocation.h>

STATUS wcs_floc_define (void);
STATUS wcs_floc_attach (char * name, WCS_LOCATION* wcs_loc_info);
VOID wcs_floc_detach (char * name);

MO_GET_METHOD wcs_loc_id_get;
MO_GET_METHOD wcs_loc_name_get;
MO_GET_METHOD wcs_loc_path_get;
MO_GET_METHOD wcs_loc_type_get;
MO_GET_METHOD wcs_loc_ext_get;
MO_GET_METHOD wcs_loc_counter_get;

GLOBALREF char wcs_floc_index[];

GLOBALREF MO_CLASS_DEF wcs_loc_class[];

/*
** Name:    wcs_floc_define
**
** Description:
**      Define the MO class for wcs location.
**
**
** Inputs:
**      None.
**
** Outputs:
**      None.
**
** Returns:
**      OK                  successfully completed
**      MO_NO_MEMORY        MO_MEM_LIMIT exceeded
**      MO_NULL_METHOD      null get/set methods supplied when permission state
**                          they are allowed
**      !OK                 system specific error
**
** History:
**      02-Jul-98 (fanra01)
**          Created.
**
*/
STATUS
wcs_floc_define (void)
{
    return (MOclassdef (MAXI2, wcs_loc_class));
}

/*
** Name:    wcs_floc_define
**
** Description:
**      Make a location instance based on the filename.
**
** Inputs:
**      None.
**
** Outputs:
**      None.
**
** Returns:
**      OK                  successfully completed
**      !OK                 system specific error
**
** History:
**      02-Jul-98 (fanra01)
**          Created.
**
*/
STATUS
wcs_floc_attach (char * name, WCS_LOCATION* wcs_loc_info)
{
    return (MOattach (MO_INSTANCE_VAR, wcs_floc_index, name,
                (PTR)wcs_loc_info));
}

/*
** Name: wcs_floc_detach
**
** Description:
**
**      Wrapper for MOdetach.
**
** Inputs:
**
**      name            name for file
**
** Outputs:
**
**      None
**
** Returns:
**      None.
**
** History:
**      02-Jul-98 (fanra01)
**          Created.
**
*/
VOID
wcs_floc_detach (char * name)
{
    MOdetach (wcs_floc_index, name);
}

