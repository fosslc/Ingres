/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: wpsdata.c
**
** Description: Global data for wps facility.
**
** History:
**      02-Mar-98 (fanra01)
**          Created.
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

# include <wpsfile.h>

MO_GET_METHOD cache_timeout_get;
MO_GET_METHOD cache_wps_name_get;
MO_GET_METHOD cache_wps_loc_name_get;
MO_GET_METHOD cache_wps_status_get;
MO_GET_METHOD cache_wps_counter_get;
MO_GET_METHOD cache_wps_exist_get;

GLOBALDEF char wps_cache_index[] = { "exp.wsf.wps.cache.index" };

GLOBALDEF MO_CLASS_DEF wps_cache[] =
{
	{ MO_INDEX_CLASSID | MO_CDATA_CLASSID, wps_cache_index,
      0,
      MO_READ, 0, 0,
      MOpvget, MOnoset, 0, MOidata_index },
    { MO_CDATA_INDEX, "exp.wsf.wps.cache.name",
      MO_SIZEOF_MEMBER(WCS_FILE_INFO, name),
      MO_READ, wps_cache_index, CL_OFFSETOF(WCS_FILE_INFO, name),
      cache_wps_name_get, MOnoset, 0, MOidata_index },
    { MO_CDATA_INDEX, "exp.wsf.wps.cache.loc_name",
      MO_SIZEOF_MEMBER(WCS_LOCATION, name),
      MO_READ, wps_cache_index, CL_OFFSETOF(WCS_LOCATION, name),
      cache_wps_loc_name_get, MOnoset, 0, MOidata_index },
    { MO_CDATA_INDEX, "exp.wsf.wps.cache.status",
      MO_SIZEOF_MEMBER(WCS_FILE_INFO, status),
      MO_READ, wps_cache_index, CL_OFFSETOF(WCS_FILE_INFO, status),
      cache_wps_status_get, MOnoset, 0, MOidata_index },
    { MO_CDATA_INDEX, "exp.wsf.wps.cache.file_counter",
      MO_SIZEOF_MEMBER(WCS_FILE_INFO, counter),
      MO_READ, wps_cache_index, CL_OFFSETOF(WCS_FILE_INFO, counter),
      cache_wps_counter_get, MOnoset, 0, MOidata_index },
    { MO_CDATA_INDEX, "exp.wsf.wps.cache.exist",
      MO_SIZEOF_MEMBER(WCS_FILE_INFO, exist),
      MO_READ, wps_cache_index, CL_OFFSETOF(WCS_FILE_INFO, exist),
      cache_wps_exist_get, MOnoset, 0, MOidata_index },
    { MO_CDATA_INDEX, "exp.wsf.wps.cache.owner",
      MO_SIZEOF_MEMBER(WPS_FILE, key),
      MO_READ, wps_cache_index, CL_OFFSETOF(WPS_FILE, key),
      MOstrpget, MOnoset, 0, MOidata_index },
    { MO_CDATA_INDEX, "exp.wsf.wps.cache.timeout",
      MO_SIZEOF_MEMBER(WPS_FILE, timeout_end),
      MO_READ, wps_cache_index, CL_OFFSETOF(WPS_FILE, timeout_end),
      cache_timeout_get, MOnoset, 0, MOidata_index },
    { MO_CDATA_INDEX, "exp.wsf.wps.cache.in_use",
      MO_SIZEOF_MEMBER(WPS_FILE, used),
      MO_READ, wps_cache_index, CL_OFFSETOF(WPS_FILE, used),
      MOuintget, MOnoset, 0, MOidata_index },
    { MO_CDATA_INDEX, "exp.wsf.wps.cache.req_count",
      MO_SIZEOF_MEMBER(WPS_FILE, count),
      MO_READ, wps_cache_index, CL_OFFSETOF(WPS_FILE, count),
      MOuintget, MOnoset, 0, MOidata_index },
    {0}
};
