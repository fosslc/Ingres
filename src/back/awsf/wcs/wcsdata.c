/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: wcsdata.c
**
** Description: Global data for wcs facility.
**
** History:
**      02-Jul-98 (fanra01)
**          Created.
**      21-Oct-98 (fanra01)
**          Update method for returning path.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPICELIBDATA
**	    in the Jamfile.
**
*/

/*
**
LIBRARY = IMPICELIBDATA
**
*/

# include <compat.h>
# include <sp.h>
# include <gl.h>
# include <erglf.h>
# include <mo.h>

# include <wcslocation.h>

MO_GET_METHOD wcs_loc_id_get;
MO_GET_METHOD wcs_loc_name_get;
MO_GET_METHOD wcs_loc_path_get;
MO_GET_METHOD wcs_loc_type_get;
MO_GET_METHOD wcs_loc_ext_get;
MO_GET_METHOD wcs_loc_counter_get;

GLOBALDEF char wcs_floc_index[] = { "exp.wsf.wcs.location.index" };

GLOBALDEF MO_CLASS_DEF wcs_loc_class[] =
{
    { MO_INDEX_CLASSID | MO_CDATA_CLASSID, wcs_floc_index,
      MO_SIZEOF_MEMBER(WCS_LOCATION, name),
      MO_READ, 0, CL_OFFSETOF(WCS_LOCATION, name),
      MOstrpget, MOnoset, 0, MOidata_index },
    { MO_CDATA_INDEX, "exp.wsf.wcs.location.id",
      MO_SIZEOF_MEMBER(WCS_LOCATION, id),
      MO_READ, wcs_floc_index, CL_OFFSETOF(WCS_LOCATION, id),
      MOuintget, MOnoset, 0, MOidata_index },
    { MO_CDATA_INDEX, "exp.wsf.wcs.location.path",
      MO_SIZEOF_MEMBER(WCS_LOCATION, path),
      MO_READ, wcs_floc_index, CL_OFFSETOF(WCS_LOCATION, path),
      MOstrpget, MOnoset, 0, MOidata_index },
    { MO_CDATA_INDEX, "exp.wsf.wcs.location.type",
      MO_SIZEOF_MEMBER(WCS_LOCATION, type),
      MO_READ, wcs_floc_index, CL_OFFSETOF(WCS_LOCATION, type),
      MOuintget, MOnoset, 0, MOidata_index },
    { MO_CDATA_INDEX, "exp.wsf.wcs.location.extension",
      MO_SIZEOF_MEMBER(WCS_LOCATION, extensions),
      MO_READ, wcs_floc_index, CL_OFFSETOF(WCS_LOCATION, extensions),
      MOstrpget, MOnoset, 0, MOidata_index },
    {0}
};
