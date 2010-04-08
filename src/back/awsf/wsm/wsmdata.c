/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: wsmdata.c
**
** Description: Global data for wsm facility.
**
** History:
**      29-Jun-98 (fanra01)
**          Created.
**      11-Sep-1998 (fanra01)
**          Corrected case for actsession and usrsession to match piccolo.
**	07-Jul-2003 (devjo01)
**	    Use MOsidget instead of MPptrget for 'exp.wsf.usr.trans.connect'.
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
# include <actsession.h>
# include <usrsession.h>

MO_GET_METHOD usr_trans_owner_get;
MO_GET_METHOD usr_curs_owner_get;
MO_GET_METHOD usr_curs_query_get;
MO_GET_METHOD usr_name_get;
MO_GET_METHOD usr_timeout_get;
MO_GET_METHOD usr_trans_sess_get;

GLOBALDEF char act_index_name[] = { "exp.wsf.act.index" };

GLOBALDEF MO_CLASS_DEF act_classes[] =
{
    { MO_INDEX_CLASSID | MO_CDATA_CLASSID, act_index_name,
      MO_SIZEOF_MEMBER(ACT_SESSION, name),
      MO_READ, 0, CL_OFFSETOF( ACT_SESSION, name),
      MOstrpget, MOnoset, 0, MOidata_index },
    { MO_CDATA_INDEX, "exp.wsf.act.host",
      MO_SIZEOF_MEMBER( ACT_SESSION, host ),
      MO_READ, act_index_name, CL_OFFSETOF( ACT_SESSION, host ),
      MOstrpget, MOnoset, 0, MOidata_index },
    { MO_CDATA_INDEX, "exp.wsf.act.query",
      MO_SIZEOF_MEMBER( ACT_SESSION, query ),
      MO_READ, act_index_name, CL_OFFSETOF( ACT_SESSION, query ),
      MOstrpget, MOnoset, 0, MOidata_index },
    { MO_CDATA_INDEX, "exp.wsf.act.err_count",
      MO_SIZEOF_MEMBER( ACT_SESSION, error_counter ),
      MO_READ, act_index_name, CL_OFFSETOF( ACT_SESSION, error_counter ),
      MOuintget, MOnoset, 0, MOidata_index },
    {0}
};

GLOBALDEF char usr_index_name[] = { "exp.wsf.usr.index" };

GLOBALDEF MO_CLASS_DEF usr_classes[] =
{
    { MO_INDEX_CLASSID | MO_CDATA_CLASSID, usr_index_name,
      MO_SIZEOF_MEMBER(USR_SESSION, name),
      MO_READ, 0, CL_OFFSETOF(USR_SESSION, name),
      MOstrpget, MOnoset, 0, MOidata_index },
    { MO_CDATA_INDEX, "exp.wsf.usr.ice_user",
      MO_SIZEOF_MEMBER( USR_SESSION, user ),
      MO_READ, usr_index_name, CL_OFFSETOF( USR_SESSION, user ),
      usr_name_get, MOnoset, 0, MOidata_index },
    { MO_CDATA_INDEX, "exp.wsf.usr.requested_count",
      MO_SIZEOF_MEMBER( USR_SESSION, requested_counter ),
      MO_READ, usr_index_name, CL_OFFSETOF( USR_SESSION, requested_counter ),
      MOuintget, MOnoset, 0, MOidata_index },
    { MO_CDATA_INDEX, "exp.wsf.usr.timeout",
      MO_SIZEOF_MEMBER( USR_SESSION, timeout_end ),
      MO_READ, usr_index_name, CL_OFFSETOF( USR_SESSION, timeout_end ),
      usr_timeout_get, MOnoset, 0, MOidata_index },
    {0}
};

GLOBALDEF char usr_trans_index[] = { "exp.wsf.usr.trans.index" };

GLOBALDEF MO_CLASS_DEF usr_trans_classes[] =
{
    { MO_INDEX_CLASSID | MO_CDATA_CLASSID, usr_trans_index,
      0,
      MO_READ, 0, 0,
      MOpvget, MOnoset, 0, MOidata_index },
    { MO_CDATA_INDEX, "exp.wsf.usr.trans.name",
      MO_SIZEOF_MEMBER( USR_TRANS, trname ),
      MO_READ, usr_trans_index, CL_OFFSETOF( USR_TRANS, trname ),
      MOstrpget, MOnoset, 0, MOidata_index },
    { MO_CDATA_INDEX, "exp.wsf.usr.trans.connect",
      MO_SIZEOF_MEMBER( USR_TRANS, session ),
      MO_READ, usr_trans_index, CL_OFFSETOF( USR_TRANS, session ),
      MOsidget, MOnoset, 0, MOidata_index },
    { MO_CDATA_INDEX, "exp.wsf.usr.trans.owner",
      MO_SIZEOF_MEMBER( USR_TRANS, usr_session ),
      MO_READ, usr_trans_index, CL_OFFSETOF( USR_TRANS, usr_session ),
      usr_trans_owner_get, MOnoset, 0, MOidata_index },
    {0}
};

GLOBALDEF char usr_curs_index[] = { "exp.wsf.usr.cursor.index" };

GLOBALDEF MO_CLASS_DEF usr_curs_classes[] =
{
    { MO_INDEX_CLASSID | MO_CDATA_CLASSID, usr_curs_index,
      0,
      MO_READ, 0, 0,
      MOpvget, MOnoset, 0, MOidata_index },
    { MO_CDATA_INDEX, "exp.wsf.usr.cursor.name",
      MO_SIZEOF_MEMBER( USR_CURSOR, name ),
      MO_READ, usr_curs_index, CL_OFFSETOF( USR_CURSOR, name ),
      MOstrpget, MOnoset, 0, MOidata_index },
    { MO_CDATA_INDEX, "exp.wsf.usr.cursor.query",
      MO_SIZEOF_MEMBER( USR_CURSOR, query ),
      MO_READ, usr_curs_index, CL_OFFSETOF( USR_CURSOR, query ),
      usr_curs_query_get, MOnoset, 0, MOidata_index },
    { MO_CDATA_INDEX, "exp.wsf.usr.cursor.owner",
      0,
      MO_READ, usr_curs_index, 0,
      usr_curs_owner_get, MOnoset, 0, MOidata_index },
    {0}
};
