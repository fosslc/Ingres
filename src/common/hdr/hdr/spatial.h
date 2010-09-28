/*
** Copyright (c) 2010 Ingres Corporation
*/

#ifndef SPATIAL_H_
#define SPATIAL_H_

/**
** Name: SPATIAL.H - Types and defines used for Ingres Geospatial
**
** Description:
**	This file contains types, definitions and function prototypes
**	used by Ingres Geospatial throughout back and common.
**
** History:
**	26-Feb-2010 (troal01)
**	    Created and moved symbols from common/adf/hdr/aduspatial.h
**	    and back/hdr/hdr/pslgeo.h
**	15-Mar-2010 (troal01)
**	    Added DB_SPATIAL_REF_SYS struct to map the spatial_ref_sys
**	    system catalog to a struct image.
*/


/*
 * Used for the iigeomnames and iigeomdimensions SQL functions
 */
struct geom_types_struct
{
    i4 type_code;
    const char *type_name;
    i2 dimensions;
};
/*
 * Maps Ingres DB_TYPE code to OGC geometry type code
 */
typedef struct _GEOM_TYPE
{
	i4 db_type;
	i2 geom_type;
} GEOM_TYPE;

/*
 * spatial_ref_sys row image
 */
typedef struct _DB_SPATIAL_REF_SYS
{
	i4   srs_srid;
#define SRS_SRID_COL 1
	char srs_auth_name[256 + 1]; //varchar(256) + null delim
	i4   srs_auth_id;
	char srs_srtext[2048 + 1]; //varchar(2048) + null delim
	char srs_proj4text[1024 + 1]; //varchar(2048) + null delim
} DB_SPATIAL_REF_SYS;

/*
 * Since the OGC standard allows NULL in all columns except SRID we need
 * some special handling for these rows. Additionally the varchar datatype
 * introduces additional complexity, here's the calculations:
 * SRID: 4 bytes
 * auth_name: 2 + 256 + 1 = 259 bytes (length, text, nullable)
 * auth_id: 4 + 1 = 5 bytes (nullable)
 * srtext: 2 + 2048 + 1 = 2051 bytes (length, text, nullable)
 * proj4text: 2 + 1024 + 1 = 1027 bytes (length, text, nullable)
 */
#define SRS_ROW_SIZE 3346
/*
 * If the structure of spatial_ref_sys changes, these must be updated.
 * These takes into account that the first two bytes are length for
 * VARCHAR. 
 */
#define SRS_AUTH_NAME_OFFSET 6
#define SRS_AUTH_ID_OFFSET 263
#define SRS_SRTEXT_OFFSET 270
#define SRS_PROJ4TEXT_OFFSET 2321

typedef char SRS_ROW [SRS_ROW_SIZE];

/*
 * This table maps the OGC geometry type code to type name
 * and # of dimensions this type has. It currently resides
 * in common/adf/adu/aduspatial.c
 */
GLOBALCONSTREF struct geom_types_struct geometry_types[];
/*
 * Table with mapping, currently resides in
 * common/adf/adu/aduspatial.c
 */
GLOBALCONSTREF GEOM_TYPE geom_type_mapping[];

/*
 * Global geospatial symbols
 */
#define GEOM_TYPE_UNDEFINED -1
#define SRID_UNDEFINED -1
#define FULL_PRECISION -1 //Used for astext when rounding or not
#define GENERIC_GEOMETRY -42 //Used in _fromText/_fromWKB functions to
                             //indicate abstract geometry type
/*
 * Current storage version for geospatial data and list of
 * old storage versions. This is necessary for handling the data
 * in case we decide to change the storage format. If the storage
 * data format is changed YOU MUST ENSURE that the macros below
 * are also updated if necessary!
 */
#define SPATIAL_DATA_V1 1
#define CURRENT_SPATIAL_DATA_V SPATIAL_DATA_V1

/*
 * Location in the LOB data of version and SRID
 */
#define GEOM_VERSION_LOCATION (sizeof(u_i2))
#define GEOM_SRID_LOCATION (GEOM_VERSION_LOCATION + sizeof(i2))

/*
 * Macros to extract version and SRID out of the data.
 * These have to be called at the start of the geospatial
 * data pointer. LOBs use the first two bytes to store the
 * LOB length, it is of type u_i2, hence the added sizeof(u_i2)
 * See DB_TEXT_STRING in common/hdr/hdr/iicommon.h
 */
/*
 * GEOM_VERS_ASSIGN_MACRO will take a ptr and assign it to an i2
 * pointer.
 */
#define GEOM_VERS_ASSIGN_MACRO(ptr, vers) (I2ASSIGN_MACRO(*(((char *) (ptr)) + GEOM_VERSION_LOCATION), vers))
/*
 * GEOM_SRID_ASSIGN_MACRO will take a ptr and assign it to an i4
 * pointer.
 */
#define GEOM_SRID_ASSIGN_MACRO(ptr, srid) (I4ASSIGN_MACRO(*(((char *) (ptr)) + GEOM_SRID_LOCATION), srid))

/*
 * Overwrites the current SRID within LOB data with the
 * provided i4 srid
 */
#define GEOM_SRID_COPY_MACRO(ptr, srid) (MEcopy(&srid, sizeof(i4), (((char *) (ptr)) + GEOM_SRID_LOCATION)))

/*
 * Prototypes
 */
FUNC_EXTERN bool db_datatype_is_geom(i4 db_datatype);

#endif /* SPATIAL_H_ */
