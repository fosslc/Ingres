/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: gvint.h
**
** Description:
**      Internal definitions and declarations for version facility.
**
** History:
**      01-Mar-2004 (fanra01)
**          Add history.
**          Add version and platform objects.
**      10-Mar-2004 (fanra01)
**          Add MIB strings for config.dat values.
**      23-Mar-2004 (fanra01)
**          Add MIB string for instance/installation id.
**      01-Jun-2005 (fanra01)
**          SIR 114614
**          Add MIB string for build level and system path value.
*/
# define GV_MIB_VERSTRING           "exp.clf.gv.version"
# define GV_MIB_ENVSTRING           "exp.clf.gv.env"
# define GV_MIB_MAJORVERS           "exp.clf.gv.majorvers"
# define GV_MIB_MINORVERS           "exp.clf.gv.minorvers"
# define GV_MIB_GENLEVEL            "exp.clf.gv.genlevel"
# define GV_MIB_BLDLEVEL            "exp.clf.gv.bldlevel"
# define GV_MIB_BYTETYPE            "exp.clf.gv.bytetype"
# define GV_MIB_HW                  "exp.clf.gv.hw"
# define GV_MIB_OS                  "exp.clf.gv.os"
# define GV_MIB_PATCHLVL            "exp.clf.gv.patchlvl"
# define GV_MIB_INSTANCE            "exp.clf.gv.instance"
# define GV_MIB_TCPPORT             "exp.clf.gv.tcpport"
# define GV_MIB_SQL92_CONFORMANCE   "exp.clf.gv.sql92_conf"
# define GV_MIB_LANGUAGE            "exp.clf.gv.language"
# define GV_MIB_CHARSET             "exp.clf.gv.charset"
# define GV_MIB_PROTENTRY           "exp.clf.gv.protentry"
# define GV_MIB_PROTPORT            "exp.clf.gv.protport"
# define GV_MIB_SYSTEM              "exp.clf.gv.system"

# define GV_MIB_CNF_INDEX           "exp.clf.gv.cnf_index"
# define GV_MIB_CNF_NAME            "exp.clf.gv.cnf_name"
# define GV_MIB_CNF_VALUE           "exp.clf.gv.cnf_value"

# define GV_MIB_CNFDAT_INDEX        "exp.clf.gv.cnfdat_index"
# define GV_MIB_CNFDAT_NAME         "exp.clf.gv.cnfdat_name"
# define GV_MIB_CNFDAT_VALUE        "exp.clf.gv.cnfdat_value"

/*
** Name: ING_CONFIG
**
** Description:
**      Structure to hold configuration data.
**
**      param_name  Name of parameter
**      param_type  Type of parameter for conversion.
**      param_len   Length of parameter value storage
**      param_val   Parameter value
**
** History:
**      03-Feb-2004 (fanra01)
**          SIR 111718
**          Created.
**      12-Feb-2004 (fanra01)
**          Replaced POINTER type with generic type.
*/
typedef struct _ing_cnf ING_CONFIG;

struct _ing_cnf
{
    char*       param_name;
    i4          param_len;
    PTR         param_val;
};


