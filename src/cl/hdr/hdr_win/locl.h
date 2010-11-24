/******************************************************************************
**
** Copyright (c) 1985, 2003 Ingres Corporation
**
******************************************************************************/

#ifndef LO_INCLUDED
#define LO_INCLUDED
#include <tm.h>

/******************************************************************************
**
** Name:        lo.h -  Compatibility Library Location Module Interface
**                      Definition.
**
** Description:
**      Contains declarations and definitions that define the interface for
**      the locations (LO) module of the CL.
**
** History:
**	15-sep-1997 (canor01)
**	    Increase LO_NM_LEN to 128 (from 32).
**	17-dec-1997 (canor01)
**	    Change maximums to those defined in system header files to 
**	    support standard and UNC-style filenames.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	28-mar-2001 (mcgem01)
**	    Add raw location indicator.
**	20-may-2001 (somsa01 for jenjo02)
**	    Added return codes so that LO callers can discriminate
**	    actual error from blanket "FAIL" when needed.
**	21-oct-2001 (somsa01)
**	    Added LO_IS_RAW return code.
**	22-may-2003 (somsa01)
**	    Changed the type of li_size to OFFSET_TYPE in LOINFORMATION.
**	24-jun-2003 (somsa01)
**	    Removed prototype for LOerrno(), as it is now in lo.h.
**	05-apr-2006 (drivi01)
**	     Removed defintions of extended types.
**  	05-Jul-2005 (fanra01)
**           Bug 114804
**           Moved LO status codes into lo.h for consistent values on all
**           platforms.
**
**      10-apr-2006 (horda03) BUG 115913
**          Extend fix for Windows.
**	08-Nov-2010 (kiria01) SIR 124685
**	    In cleaning up PSF errors, added LOCATION_INIT as a supported
**	    initial value for LOCATION.
******************************************************************************/


/******************************************************************************
**
** Defined Constants.
**
******************************************************************************/


/*
** Symbols used in arguments to LOfroms special values chosen so that and's
** (&'s) of them will produce different values.
*/

#define NODE            06
#define PATH            05
#define FILENAME        03

/*
** type used by flags to LOfroms and LOwhat
*/

#define LOCTYPE         i4

/*
** symbols returned in flag argument to LOlist
*/

#define ISDIR           01
#define ISFILE          02

/*
** maximum buffer length of location buffer
*/

#define MAX_LOC        (_MAX_PATH + 1)

/*
** Maximum buffer length for parts made by an LOdetail() call.
** Assume a FAT file system and no separators (e.g. ':') or
** string termination.
*/

#define LO_DEVNAME_MAX          1
#define LO_PATH_MAX             _MAX_PATH
#define LO_FSUFFIX_MAX          _MAX_EXT
#define LO_FVERSION_MAX         0
#define LO_FPREFIX_MAX          _MAX_FNAME
#define LO_FILENAME_MAX         _MAX_FNAME

/*
** $(ING_PATH) relative directories (used by LOingpath)
*/

#define LO_DB           "data\\"
#define LO_CKP          "ckp\\"
#define LO_JNL          "jnl\\"
#define LO_WORK         "work\\"
#define LO_DMP          "dmp\\"
#define JNLACTIVE       "jnl\\active\\"
#define JNLFULL         "jnl\\full\\"
#define JNLEXPIRED      "jnl\\expired\\"

/*
** Special filename that defines a RAW location.
** Note that this must have the same value as DM2F_RAW_LINKNAME
** in back/dmf/hdr/dm2f.h
*/
# define LO_RAW		"iirawdevice"

/*
** symbol indicating support for concurent lo_delete
*/

#define LO_CONCUR_DELETE  1

/*
** OS filename parameters
*/

#define LO_NM_LEN       _MAX_PATH
#define LO_EXT_LEN      0
#define LO_OBJ_EXT      "obj"
#define LO_EXE_EXT      "exe"
#define LO_NM_CASE      0       /* not case sensitive */
#define LO_NM_VERS      0       /* no versions */

#define LO_FINDBUF_SIZE (sizeof(WIN32_FIND_DATA))

/******************************************************************************
** Type Definitions.
******************************************************************************/

/*
** Structure for LOwcard directory searches
*/

typedef struct _LO_DIR_CONTEXT
{
    char            *locbuf;
    char            path[MAX_LOC];
    char            buffer[MAX_LOC];
    HANDLE          hdir;
    WIN32_FIND_DATA find_buffer;
} LO_DIR_CONTEXT;

#define LO_FINDBUF_SIZE (sizeof(WIN32_FIND_DATA))

/*
** structure of a location
*/

typedef struct _LOCATION
{
        char    *node;
        char    *path;
        char    *fname;
        char    *string;
        LOCTYPE desc;
        LO_DIR_CONTEXT  *fp;
} LOCATION;
#define LOCATION_INIT {NULL,NULL,NULL,NULL,0,NULL}


/*
** structure for LOinfo calls.
*/

typedef struct _LOINFORMATION
{
		i4     li_type;
#define         LO_IS_DIR       1
#define         LO_IS_FILE      2
#define         LO_IS_MEM       3
/* extended types for LO_I_XTYPE */
#define         LO_IS_LNK       4
#define         LO_IS_CHR       5
#define         LO_IS_BLK       6
#define         LO_IS_FIFO      7
#define         LO_IS_SOCK      8
#define         LO_IS_UNKNOWN   9

		i4     li_perms;       /* the permissions, as bit flags */
#define         LO_P_WRITE      0x01
#define         LO_P_READ       0x02
#define         LO_P_EXECUTE    0x04
#define         LO_I_XTYPE      0x10    /* extended file type query */
#define         LO_I_LSTAT      0x20    /* stat links and not referenced files */
/* NOTE: LO_I_ALL requests LO_I_TYPE and not LO_I_XTYPE, so to do all
   queries and to do the extended type query, use (LO_I_ALL|LO_I_XTYPE) */

		OFFSET_TYPE      li_size;
		SYSTIME li_last;        /* time of last access */
} LOINFORMATION;

/*
** bits for LOinfo requests.
*/

#define LO_I_TYPE       0x01
#define LO_I_PERMS      0x02
#define LO_I_SIZE       0x04
#define LO_I_LAST       0x08
#define LO_I_ALL        (LO_I_TYPE | LO_I_PERMS | LO_I_SIZE | LO_I_LAST)

#endif
