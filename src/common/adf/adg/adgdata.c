/*
**	Copyright (c) 2004 Ingres Corporation
*/


#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <spatial.h>

/*
** Name:	adgdata.c
**
** Description:	Global data for ADG facility.
**
** History:
**      12-Dec-95 (fanra01)
**          Created.
**	23-sep-1996 (canor01)
**	    Updated.
**	11-jun-2001 (somsa01)
**	    To save stack space, we now dynamically allocate space for
**	    the min / max arrays in adg_startup().
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPADFLIBDATA
**	    in the Jamfile.
*/

/*
**
LIBRARY = IMPADFLIBDATA
**
*/

/*
**  Data from adginit.c
*/

GLOBALDEF	ADF_SERVER_CB	*Adf_globs = NULL;  /* ADF's global server CB.*/


/*
**  Data from adgstartup.c
*/

GLOBALDEF   u_char	*Chr_min ZERO_FILL;
GLOBALDEF   u_char	*Chr_max ZERO_FILL;
GLOBALDEF   u_char	*Cha_min ZERO_FILL;
GLOBALDEF   u_char	*Cha_max ZERO_FILL;
GLOBALDEF   u_char	*Txt_max ZERO_FILL;
GLOBALDEF   u_char	*Vch_min ZERO_FILL;
GLOBALDEF   u_char	*Vch_max ZERO_FILL;
GLOBALDEF   u_char	*Lke_min ZERO_FILL;
GLOBALDEF   u_char	*Lke_max ZERO_FILL;
GLOBALDEF   u_char	*Bit_min ZERO_FILL;
GLOBALDEF   u_char	*Bit_max ZERO_FILL;

/*
 * Data for Ingres Geospatial
 */


/*
 * geometry types table, used by adu_geom_name and adu_geom_dimensions for now
 * might or might not be obsolete once a more efficient implementation is made.
 */
GLOBALCONSTDEF struct geom_types_struct geometry_types[] =
{
    { 0,  "GEOMETRY", 2},
    { 1,  "POINT", 2},
    { 2,  "LINESTRING", 2},
    { 3,  "POLYGON", 2},
    { 4,  "MULTIPOINT", 2},
    { 5,  "MULTILINESTRING", 2},
    { 6,  "MULTIPOLYGON", 2},
    { 7,  "GEOMCOLLECTION", 2},
    { 13,  "CURVE", 2},
    { 14,  "SURFACE", 2},
    { 15,  "POLYHEDRALSURFACE", 2},
    { 1000,  "GEOMETRYZ", 3},
    { 1001,  "POINTZ", 3},
    { 1002,  "LINESTRINGZ", 3},
    { 1003,  "POLYGONZ", 3},
    { 1004,  "MULTIPOINTZ", 3},
    { 1005,  "MULTILINESTRINGZ", 3},
    { 1006,  "MULTIPOLYGONZ", 3},
    { 1007,  "GEOMCOLLECTIONZ", 3},
    { 1013,  "CURVEZ", 3},
    { 1014,  "SURFACEZ", 3},
    { 1015,  "POLYHEDRALSURFACEZ", 3},
    { 2000,  "GEOMETRY", 3},
    { 2001,  "POINTM", 3},
    { 2002,  "LINESTRINGM", 3},
    { 2003,  "POLYGONM", 3},
    { 2004,  "MULTIPOINTM", 3},
    { 2005,  "MULTILINESTRINGM", 3},
    { 2006,  "MULTIPOLYGONM", 3},
    { 2007,  "GEOMCOLLECTIONM", 3},
    { 2013,  "CURVEM", 3},
    { 2014,  "SURFACEM", 3},
    { 2015,  "POLYHEDRALSURFACEM", 3},
    { 3000,  "GEOMETRYZM", 4},
    { 3001,  "POINTZM", 4},
    { 3002,  "LINESTRINGZM", 4},
    { 3003,  "POLYGONZM", 4},
    { 3004,  "MULTIPOINTZM", 4},
    { 3005,  "MULTILINESTRINGZM", 4},
    { 3006,  "MultiPolygonZM", 4},
    { 3007,  "GEOMCOLLECTIONZM", 4},
    { 3013,  "CURVEZM", 4},
    { 3014,  "SURFACEZM", 4},
    { 3015,  "POLYHEDRALSURFACEZM", 4},
    { -1,   "NULL", -1 }
};
/*
 * geo_type_mapping table
 */

GLOBALCONSTDEF GEOM_TYPE geom_type_mapping[] =
{
        { DB_GEOM_TYPE, 0 }, //Geometry type, not instantiable.
        { DB_POINT_TYPE, 1 },
        { DB_LINE_TYPE, 2 },
        { DB_POLY_TYPE, 3 },
        { DB_MPOINT_TYPE, 4 },
        { DB_MLINE_TYPE, 5 },
        { DB_MPOLY_TYPE, 6 },
        { DB_GEOMC_TYPE, 7 }, //GeometryCollection
        { -1, GEOM_TYPE_UNDEFINED}
};
