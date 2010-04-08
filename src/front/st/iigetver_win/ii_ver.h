/*
** Copyright (c) 1999, 2001 Ingres Corporation
*/

/*
**  Exported function:
**
**  Name: ii_ver.h
**
**  Description:
**	header file for II_GetVersion().
**
**  History:
**	23-mar-99 (cucjo01)
**	    Created
**	    Header file for version information
**	24-aug-2000 (rodjo04)
**	    Added error return code II_ERROR_II_SYSTEM_DIR_NOT_EXIST 
**	    Header file for version information
**	25-mar-99
**	    Added function prototype / definition
**	27-dec-2001 (somsa01)
**	    Changed II_ERROR_CANNOT_FIND_DBMS_VERSION_STRING to
**	    II_ERROR_CANNOT_FIND_VERSION_STRING
**      17-jun-2002 (fanra01)
**          Bug 108047
**          Update function type declaration EP_II_GETVERSION to include a
**          call convention definition.  This specifies that the function
**          follows the C++ calling convention.
*/

/* Return Codes */
#define II_OK                                      0
#define II_ERROR_INVALID_PARAMETER_PASSED          1
#define II_ERROR_CANNOT_FIND_II_SYSTEM             2
#define II_ERROR_CANNOT_OPEN_CONFIG_DAT            3
#define II_ERROR_CANNOT_FIND_VERSION_STRING        4
#define II_ERROR_II_SYSTEM_DIR_NOT_EXIST           5

#define MAX_STRING_LEN                           512

/* Structure Definition */
typedef struct _II_VERSIONINFO II_VERSIONINFO;

struct _II_VERSIONINFO
{
    char szII_SYSTEM[MAX_STRING_LEN];
    char szII_VERSIONSTRING[MAX_STRING_LEN];
};

/* Function Definition */
typedef int (WINAPI *EP_II_GETVERSION) (II_VERSIONINFO *);
