/*
** Copyright (C) 1999,2004 Ingres Corporation All Rights Reserved.
*/

/*
** Name: gvdata
**
** Description:
**      MIB object class description.
**
LIBRARY = IMPCOMPATLIBDATA
**
** History:
**      03-Feb-2004 (fanra01)
**          SIR 111718
**          Created.
**      01-Mar-2004 (fanra01)
**          SIR 111718
**          Add fields to return version string and platform
**          description.
**      08-Mar-2004 (fanra01)
**          Removed GVgetnetportaddr.
**      10-Mar-2004 (fanra01)
**          SIR 111718
**          Add gv_cnfdat_classes to register config.dat parameters as
**          IMA objects.
**      23-Mar-2004 (fanra01)
**          Add GV_MIB_INSTANCE entry to gv_classes.
**      08-Apr-2004 (fanra01)
**          Addition of the MO_ANY_READ permission to allow unprivileged
**          access.
**	09-Jul-2004 (hanje04)
**	    Removed include of erflg.h as it was causing cyclic dependancies
**	    for Jam and the file compiles without it.
**      01-Jun-2005 (fanra01)
**          SIR 114614
**          Add class entry for system path value.
*/

# include <compat.h>
# include <gv.h>
# include <gl.h>

# include <sp.h>
# include <mo.h>

# include "gvint.h"

static char gv_cnf_index[] = { GV_MIB_CNF_INDEX };
static char gv_cnfdat_index[] = { GV_MIB_CNFDAT_INDEX };

FUNC_EXTERN MO_GET_METHOD GVgetverstr;
FUNC_EXTERN MO_GET_METHOD GVgetenvstr;
FUNC_EXTERN MO_GET_METHOD GVgetversion;
FUNC_EXTERN MO_GET_METHOD GVgetpatchlvl;
FUNC_EXTERN MO_GET_METHOD GVgettcpportaddr;
FUNC_EXTERN MO_GET_METHOD GVgetinstance;
FUNC_EXTERN MO_GET_METHOD GVgetlanportaddr;
FUNC_EXTERN MO_GET_METHOD GVgetconformance;
FUNC_EXTERN MO_GET_METHOD GVgetlanguage;
FUNC_EXTERN MO_GET_METHOD GVgetcharset;
FUNC_EXTERN MO_GET_METHOD GVgetsystempath;

GLOBALDEF MO_CLASS_DEF gv_classes[] = 
{
    {
    0,
    GV_MIB_VERSTRING,
	0,
    MO_ANY_READ,
    0,
    0,
    GVgetverstr,
    MOnoset,
    0,
    MOcdata_index
    },
    {
    0,
    GV_MIB_ENVSTRING,
	0,
    MO_ANY_READ,
    0,
    0,
    GVgetenvstr,
    MOnoset,
    0,
    MOcdata_index
    },
    {
    0,
    GV_MIB_MAJORVERS,
	MO_SIZEOF_MEMBER( ING_VERSION, major ),
    MO_ANY_READ,
    0,
    GV_M_MAJOR,
    GVgetversion,
    MOnoset,
    0,
    MOcdata_index
    },
    {
    0,
    GV_MIB_MINORVERS,
	MO_SIZEOF_MEMBER( ING_VERSION, minor ),
    MO_ANY_READ,
    0,
    GV_M_MINOR,
    GVgetversion,
    MOnoset,
    0,
    MOcdata_index
    },
    {
    0,
    GV_MIB_GENLEVEL,
	MO_SIZEOF_MEMBER( ING_VERSION, genlevel ),
    MO_ANY_READ,
    0,
    GV_M_GENLVL,
    GVgetversion,
    MOnoset,
    0,
    MOcdata_index
    },
    {
    0,
    GV_MIB_BLDLEVEL,
	MO_SIZEOF_MEMBER( ING_VERSION, bldlevel ),
    MO_ANY_READ,
    0,
    GV_M_BLDLVL,
    GVgetversion,
    MOnoset,
    0,
    MOcdata_index
    },
    {
    0,
    GV_MIB_BYTETYPE,
	MO_SIZEOF_MEMBER( ING_VERSION, byte_type ),
    MO_ANY_READ,
    0,
    GV_M_BYTE,
    GVgetversion,
    MOnoset,
    0,
    MOcdata_index
    },
    {
    0,
    GV_MIB_HW,
	MO_SIZEOF_MEMBER( ING_VERSION, hardware ),
    MO_ANY_READ,
    0,
    GV_M_HW,
    GVgetversion,
    MOnoset,
    0,
    MOcdata_index
    },
    {
    0,
    GV_MIB_OS,
	MO_SIZEOF_MEMBER( ING_VERSION, os ),
    MO_ANY_READ,
    0,
    GV_M_OS,
    GVgetversion,
    MOnoset,
    0,
    MOcdata_index
    },
    {
    0,
    GV_MIB_PATCHLVL,
    0,
    MO_ANY_READ,
    0,
    0,
    GVgetpatchlvl,
    MOnoset,
    0,
    MOcdata_index
    },
    {
    0,
    GV_MIB_INSTANCE,
    0,
    MO_ANY_READ,
    0,
    0,
    GVgetinstance,
    MOnoset,
    0,
    MOcdata_index
    },
    {
    0,
    GV_MIB_TCPPORT,
    0,
    MO_ANY_READ,
    0,
    0,
    GVgettcpportaddr,
    MOnoset,
    0,
    MOcdata_index
    },
    {
    0,
    GV_MIB_SQL92_CONFORMANCE,
    0,
    MO_ANY_READ,
    0,
    0,
    GVgetconformance,
    MOnoset,
    0,
    MOcdata_index
    },
    {
    0,
    GV_MIB_LANGUAGE,
    0,
    MO_ANY_READ,
    0,
    0,
    GVgetlanguage,
    MOnoset,
    0,
    MOcdata_index
    },
    {
    0,
    GV_MIB_CHARSET,
    0,
    MO_ANY_READ,
    0,
    0,
    GVgetcharset,
    MOnoset,
    0,
    MOcdata_index
    },
    {
    0,
    GV_MIB_SYSTEM,
    0,
    MO_ANY_READ,
    0,
    0,
    GVgetsystempath,
    MOnoset,
    0,
    MOcdata_index
    },
    { 0 }
};

GLOBALDEF MO_CLASS_DEF gv_cnf_classes[] =
{
  {
  MO_INDEX_CLASSID|MO_CDATA_INDEX,
  gv_cnf_index,
  0,
  MO_ANY_READ,
  0,
  0,
  MOptrget,
  MOnoset,
  0,
  MOidata_index
  },
  {
  MO_CDATA_INDEX,
  GV_MIB_CNF_NAME,
  MO_SIZEOF_MEMBER( ING_CONFIG, param_name ),
  MO_ANY_READ,
  gv_cnf_index,
  CL_OFFSETOF( ING_CONFIG, param_name ),
  MOstrpget,
  MOnoset,
  0,
  MOidata_index
  },
  {
  MO_CDATA_INDEX,
  GV_MIB_CNF_VALUE,
  MO_SIZEOF_MEMBER( ING_CONFIG, param_val ),
  MO_ANY_READ,
  gv_cnf_index,
  CL_OFFSETOF( ING_CONFIG, param_val ),
  MOstrpget,
  MOnoset,
  0,
  MOidata_index
  },
  { 0 }
};

GLOBALDEF MO_CLASS_DEF gv_cnfdat_classes[] =
{
  {
  MO_INDEX_CLASSID|MO_CDATA_INDEX,
  gv_cnfdat_index,
  0,
  MO_ANY_READ,
  0,
  0,
  MOptrget,
  MOnoset,
  0,
  MOidata_index
  },
  {
  MO_CDATA_INDEX,
  GV_MIB_CNFDAT_NAME,
  MO_SIZEOF_MEMBER( ING_CONFIG, param_name ),
  MO_ANY_READ,
  gv_cnfdat_index,
  CL_OFFSETOF( ING_CONFIG, param_name ),
  MOstrpget,
  MOnoset,
  0,
  MOidata_index
  },
  {
  MO_CDATA_INDEX,
  GV_MIB_CNFDAT_VALUE,
  MO_SIZEOF_MEMBER( ING_CONFIG, param_val ),
  MO_ANY_READ,
  gv_cnfdat_index,
  CL_OFFSETOF( ING_CONFIG, param_val ),
  MOstrpget,
  MOnoset,
  0,
  MOidata_index
  },
  { 0 }
};

