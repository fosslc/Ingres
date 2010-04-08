/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: ddcdata.c
**
** Description: Global data for ddc facility.
**
** History:
**      13-Jul-98 (fanra01)
**          Created.
**
*/
# include <compat.h>
# include <sp.h>
# include <gl.h>
# include <erglf.h>
# include <mo.h>

# include <dbaccess.h>

MO_GET_METHOD ddc_dbname_get;
MO_GET_METHOD ddc_dbtimeout_get;

GLOBALDEF char db_access_name[] = { "exp.ddf.ddc.db_access_index" };

GLOBALDEF MO_CLASS_DEF db_classes[] =
{
    { MO_INDEX_CLASSID | MO_CDATA_CLASSID, db_access_name,
      0,
      MO_READ, 0, 0,
      MOstrget, MOnoset, 0, MOname_index },
    { MO_CDATA_INDEX, "exp.ddf.ddc.db_driver",
      MO_SIZEOF_MEMBER(DB_CACHED_SESSION, driver),
      MO_READ, db_access_name, CL_OFFSETOF(DB_CACHED_SESSION, driver),
      MOuintget, MOnoset, 0, MOidata_index },
    { MO_CDATA_INDEX, "exp.ddf.ddc.db_name",
      0,
      MO_READ, db_access_name, 0,
      ddc_dbname_get, MOnoset, 0, MOidata_index },
    { MO_CDATA_INDEX, "exp.ddf.ddc.db_used",
      MO_SIZEOF_MEMBER(DB_CACHED_SESSION, used),
      MO_READ, db_access_name, CL_OFFSETOF(DB_CACHED_SESSION, used),
      MOuintget, MOnoset, 0, MOidata_index },
    { MO_CDATA_INDEX, "exp.ddf.ddc.timtout",
      MO_SIZEOF_MEMBER(DB_CACHED_SESSION, timeout_end),
      MO_READ, db_access_name, CL_OFFSETOF(DB_CACHED_SESSION, timeout_end),
      ddc_dbtimeout_get, MOnoset, 0, MOidata_index },
    {0}
};
