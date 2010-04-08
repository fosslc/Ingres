/*
**	Copyright (c) 2004 Ingres Corporation
*/
/**
**  Name: evset.h - Evidence set handling interface definitions and constants
**
**  Description:
**     This header file contains the interface definitions, types and constants
**     required to use the evidence set handling routines.
**
**  History:
**	23-feb-1996 -- Initial port to Open Ingres
**	13-Mch-1996 (prida01)
**	    Add extra dump defines.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** Constants
**
** EVSET_MAX_PATH maximum size of a filename (from limits.h)
*/

#define EVSET_MAX_PATH  1024

/*
** Type definitions
*/

typedef struct IIEVSET_DETAILS
{
    int id;               /* Evidence set identifier */
    long created;         /* Time created (in secs since 00:00:00 Jan 1 1970) */
    char description[80]; /* Description supplied at create time */
} EVSET_DETAILS;

typedef struct IIEVSET_ENTRY
{
    short type;                     /* Type of entry */
#define EVSET_TYPE_FILE 0
#define EVSET_TYPE_VAR  1
    short flags;                    /* Flags */
#define EVSET_DELETED   0
#define EVSET_TEXT      1
#define EVSET_BINARY    2
#define EVSET_SYSTEM    4
    char description[80];           /* Description supplied at create time */
    char value[EVSET_MAX_PATH];     /* Filename or variable value */
} EVSET_ENTRY;

#define OUTPUT_BUFFER 	2048
#define EVIDENCE_LIMIT (128*1024)
#define EVSET_SHOW_ENV_CMD "ingprenv"
/*
** Interface definitions
*/

FUNC_EXTERN STATUS
EVSetCreate();

FUNC_EXTERN STATUS
EVSetDelete();

FUNC_EXTERN STATUS
EVSetExport();

FUNC_EXTERN STATUS
EVSetImport();

FUNC_EXTERN STATUS
EVSetList();

FUNC_EXTERN STATUS
EVSetCreateFile();

FUNC_EXTERN STATUS
EVSetDeleteFile();

FUNC_EXTERN STATUS
EVSetFileList();

FUNC_EXTERN STATUS
EVSetLookupVar();

/*
** STATUS values
*/
/*This value is the same as DG_CLASS << 16  in front!hdr!hdr */
#define DG_ERR_MASK		0xe50000
#define E_UNKNOWN_FLAG          (DG_ERR_MASK + 0x1)
#define E_UNKNOWN_EVSET         (DG_ERR_MASK + 0x2)
#define E_NO_EVSET              (DG_ERR_MASK + 0x3)
#define E_EXPORT_CREATE         (DG_ERR_MASK + 0x4)
#define E_COPY_FAIL            	(DG_ERR_MASK + 0x5)
#define E_SYNTAX_ERROR       	(DG_ERR_MASK + 0x7)
#define E_UNKNOWN_ACTION    	(DG_ERR_MASK + 0x8)
#define E_USAGE            	(DG_ERR_MASK + 0x9)
#define E_VIEW_BINARY_FAIL	(DG_ERR_MASK+  0x10)
#define E_OPEN_FAIL       	(DG_ERR_MASK + 0x11)
#define E_CREATE_FAIL    	(DG_ERR_MASK + 0x12)
#define E_UNKNOWN_TYPE  	(DG_ERR_MASK + 0x13)
#define E_NOT_INGRES   		(DG_ERR_MASK + 0x14)
#define E_EXCEPTION   		(DG_ERR_MASK + 0x15)
#define E_NO_II_EXCEPTION      	(DG_ERR_MASK + 0x16)
#define E_EVSET_FULL          	(DG_ERR_MASK + 0x17)
#define E_DELETE_FAIL        	(DG_ERR_MASK + 0x18)
#define E_CREATE_EVSET_FAIL 	(DG_ERR_MASK + 0x19)
#define E_PIPE_FAILED      	(DG_ERR_MASK + 0x20)
#define E_WRITE_FAILED    	(DG_ERR_MASK + 0x21)
#define E_EVSET_CORRUPT  	(DG_ERR_MASK + 0x22)       
#define E_EVSET_NOMORE   	(DG_ERR_MASK + 0x23)      
#define E_PARAM_ERROR    	(DG_ERR_MASK + 0x24)     
#define E_EVSET_FILE_DEL_FAIL 	(DG_ERR_MASK + 0x25)

#define EV_PSF_DIAGNOSTICS	0xfffffffb
#define EV_DMF_DIAGNOSTICS	0xfffffffc
#define EV_QSF_DIAGNOSTICS	0xfffffffd
#define EV_DUMP_ON_FATAL	0xfffffffe
#define EV_SIGVIO_DUMP		0xffffffff

/* This is the structure used to link the ex to scs
** we cannot call scs directly for ex so we call via
** the cs.
*/
typedef struct II_DIAG_LINK{
	i4     type;
#define DMF_OPEN_TABLES         1
#define SCF_CURR_QUERY          2
#define DMF_SELF_DIAGNOSTICS    3
#define QSF_SELF_DIAGNOSTICS    4
#define PSF_SELF_DIAGNOSTICS    5
#define DIAG_DUMP_STACKS        6
	VOID    (*output)();
	VOID    (*error)();
	VOID    *scd;
	PTR     filename;
} DIAG_LINK;

