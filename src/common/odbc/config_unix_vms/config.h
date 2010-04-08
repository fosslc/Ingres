/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: config.h
**
** Description:
**      Internal unixODBC configuration data structures and definitions.
**
** History:
**      02-Jun-03 (loera01)
**          Created.
**      28-jan-04 (loera01)
**          Added VMS port.
*/

#ifndef __ODBC_CONFIG_H
#define __ODBC_CONFIG_H

/*
** Maximum record length of ODBC configuration data.
*/

# define OCFG_MAX_STRLEN               256

/*
** Name: OCFG_HDR - Header for ODBC configuration handle
**
** Description:
**      This structure defines the handle type
**      unixODBC configuration routines.
**
** History:
**      02-Jun-03 (loera01)
**          Created.
*/

typedef struct _OCFG_HDR
{
    i4 ocfg_id;
# define OCFG_DRIVER   1     /* Driver ID */
# define OCFG_DSN      2     /* DSN ID */
} OCFG_HDR; 

/*
** Name: OCFG_SECT - Unformatted ODBC section list
**
** Description:
**      This structure defines a linked list for an unformatted section of 
**      an ODBC configuration file.
**
** History:
**      02-Jun-03 (loera01)
**          Created.
*/

typedef struct _OCFG_SECT
{
    QUEUE sect_q;                /* Placeholder for section queue */
    char *sect_name;             /* Section name */
    BOOL recognized;             /* Recognized as valid Ingres driver */
    i4   data_count;             /* Count of unformatted attributes */
    QUEUE data_q;                /* Linked list of unformatted attributes */
} OCFG_SECT;

/*
** Name: ATTR_DATA - Unformatted ODBC configuration attribute list
**
** Description:
**      This structure defines the linked list of unformatted attributes.
**
** History:
**      02-Jun-03 (loera01)
**          Created.
*/

typedef struct _ATTR_DATA
{ 
    QUEUE data_q;                  /* Placeholder for data queue */
    char *data_name;               /* Name of unformatted entry */
    char *data_value;              /* Value of unformatted entry */
} ATTR_DATA;

/*
** Name: OCFG_INFO - Formatted ODBC section list 
**
** Description:
**      This structure defines the linked list for a formatted section of
**      unixODBC attributes.
**
** History:
**      02-Jun-03 (loera01)
**          Created.
*/

typedef struct _OCFG_INFO
{
    QUEUE info_q;             /* Placeholder for info queue */
    char *info_name;          /* Formatted section name */
    i4 format_count;          /* Count of formatted attributes */
    QUEUE format_q;           /* Linked list of formatted attributes */
} OCFG_INFO;

/*
** Name: ATTR_FORMAT - Formatted list of ODBC attributes
**
** Description:
**      This structure defines the linked list of formatted attributes.
**
** History:
**      02-Jun-03 (loera01)
**          Created.
*/

typedef struct _ATTR_FORMAT
{  
    QUEUE format_q;            /* Linked list placeholder */
    i4 format_id;              /* Formatted attribute */
    char *format_value;        /* Value of the attribute */
} ATTR_FORMAT;

/*
** Name: OCFG_CB - ODBC Configuration Control Block
**
** Description:
**      This structure defines the internal contents of the driver or
**      data source handle passed to the unixODBC configuration routines.
**
** History:
**      02-Jun-03 (loera01)
**          Created.
*/

typedef struct _OCFG_CB
{
    OCFG_HDR hdr;                     /* Identifies driver or DSN handle */
    char path[OCFG_MAX_STRLEN + 1];   /* Path of config file */
    i4 ocfg_sect_count;               /* Count of section info */
    QUEUE sect_q;                     /* Linked list of sections */
    i4 ocfg_info_count;               /* Count of formatted section info */ 
    QUEUE info_q;                     /* Linked list of formatted sections */
} OCFG_CB;

/*
** Name: OCFG_STATIC - Master control block for ODBC config cache
**
** Description:
**      This structure defines the cached data for the ODBC configuration
**      database.  
**
** History:
**      02-Jun-03 (loera01)
**          Created.
*/

typedef struct _OCFG_STATIC
{
    OCFG_CB drv;            /* Driver control block */
    OCFG_CB dsn;            /* DSN control block */
} OCFG_STATIC;

GLOBALREF OCFG_STATIC ocfg_static;

/*
** Miscellaneous defines.
*/
# define SECTION_FORMAT "[%s]\n"
# define ATTRIB_FORMAT "%s=%s\n"
# ifdef VMS
# define SYS_PATH "SYS$SHARE"
# else
# define SYS_PATH "/usr/local/etc"
# endif /* VMS */
/*
** Internal ODBC configuration function templates. 
*/
i4 getAttributeID( char **attribTable, char * attrName );

char *getAttributeValue( char **attribTable, i4 attrID );

STATUS readConfigData(OCFG_CB *ocfg_cb);

STATUS formatConfigInfo(OCFG_CB *ocfg_cb);

BOOL driverIsInstalled( OCFG_CB *ocfg_cb, char *driverName );

BOOL isDriverDef( ATTR_ID *attrList, i4 count );

STATUS addInstalledDriver ( OCFG_CB *ocfg_cb, char * driverName );

STATUS addDataSources ( OCFG_CB *ocfg_cb, char * dsnName );

STATUS addConfigOptions ( OCFG_CB *ocfg_cb );

OCFG_SECT *addSection ( OCFG_CB *ocfg_cb, char *sectName );

OCFG_INFO *addInfo ( OCFG_CB *ocfg_cb, char *infoName );

ATTR_DATA *addData ( OCFG_SECT *section, char *dataName, char* dataValue );

ATTR_FORMAT *addFormat ( OCFG_INFO *info, i4 id, char *formatValue );

STATUS rewriteINI ( OCFG_CB *ocfg_cb );

STATUS emptyCache( OCFG_CB * ocfg_cb );

bool validDSN( OCFG_CB *ocfg_cb, char *dsnName );

#endif /* __ODBC_CONFIG_H */
