#ifndef _EVSET_H_INCLUDED
#define _EVSET_H_INCLUDED

/*
** Copyright (c) 1998, 2008 Ingres Corporation
**
**  Name: evset.h - Evidence set handling interface definitions and constants
**
**  Description:
**     This header file contains the interface definitions, types and constants
**     required to use the evidence set handling routines.
**
**  History:
**     19-May-1998 (horda03) - X-Integration of change 27896.
**        Initial VMS creation. Modified to mimic UNIX definitions
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	03-dec-2008 (joea)
**	    Add II_DIAG_LINK from Unix version.
**      27-nov-2009 (stegr01)
**          prevent multiple includes
**      18-Feb-2009 (horda03)
**          Add OUTPUT_BUFFER, EVIDENCE_LIMIT and EVSET_SHOW_ENV_CMD.
*/

/*
** Constants
**
** EVSET_MAX_PATH maximum size of a filename (from limits.h)
*/

#define EVSET_MAX_PATH  1024
#define OUTPUT_BUFFER   2048    
#define EVIDENCE_LIMIT (128*1024)
#define EVSET_SHOW_ENV_CMD "show log"

/*
** Type definitions
*/

typedef struct IIEVSET_DETAILS
{
    i4 id;               /* Evidence set identifier */
    u_i4 created;         /* Time created (in secs since 00:00:00 Jan 1 1970) */
    char description[80]; /* Description supplied at create time */
} EVSET_DETAILS;

typedef struct IIEVSET_ENTRY
{
    i2 type;                     /* Type of entry */
#define EVSET_TYPE_FILE 2
#define EVSET_TYPE_VAR  1
    i2 flags;                    /* Flags */
#define EVSET_DELETED   0
#define EVSET_TEXT      1
#define EVSET_BINARY    2
#define EVSET_SYSTEM    4
    char description[80];           /* Description supplied at create time */
    char value[EVSET_MAX_PATH];     /* Filename or variable value */
} EVSET_ENTRY;


typedef struct iistack_entry
{
	char		Vstring[100];
	u_i4		sp;
	u_i4		pc;
	u_i4		args[8];
} EV_STACK_ENTRY;


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
#define DG_ERR_MASK             0xe50000

#define E_UNKNOWN_FLAG          (DG_ERR_MASK + 0x1)
#define E_UNKNOWN_EVSET         (DG_ERR_MASK + 0x2)
#define E_NO_EVSET              (DG_ERR_MASK + 0x3)
#define E_EXPORT_CREATE         (DG_ERR_MASK + 0x4)
#define E_COPY_FAIL             (DG_ERR_MASK + 0x5)
#define E_SYNTAX_ERROR          (DG_ERR_MASK + 0x7)
#define E_UNKNOWN_ACTION        (DG_ERR_MASK + 0x8)
#define E_USAGE                 (DG_ERR_MASK + 0x9)
#define E_VIEW_BINARY_FAIL      (DG_ERR_MASK + 0x10)
#define E_OPEN_FAIL             (DG_ERR_MASK + 0x11)
#define E_CREATE_FAIL           (DG_ERR_MASK + 0x12)
#define E_UNKNOWN_TYPE          (DG_ERR_MASK + 0x13)
#define E_NOT_INGRES            (DG_ERR_MASK + 0x14)
#define E_EXCEPTION             (DG_ERR_MASK + 0x15)
#define E_NO_II_EXCEPTION       (DG_ERR_MASK + 0x16)
#define E_EVSET_FULL            (DG_ERR_MASK + 0x17)
#define E_DELETE_FAIL           (DG_ERR_MASK + 0x18)
#define E_CREATE_EVSET_FAIL     (DG_ERR_MASK + 0x19)
#define E_PIPE_FAILED           (DG_ERR_MASK + 0x20)
#define E_WRITE_FAILED          (DG_ERR_MASK + 0x21)
#define E_EVSET_CORRUPT         (DG_ERR_MASK + 0x22)
#define E_EVSET_NOMORE          (DG_ERR_MASK + 0x23)
#define E_PARAM_ERROR           (DG_ERR_MASK + 0x24)
#define E_EVSET_FILE_DEL_FAIL   (DG_ERR_MASK + 0x25)

#define EV_PSF_DIAGNOSTICS      0xfffffffb
#define EV_DMF_DIAGNOSTICS      0xfffffffc
#define EV_QSF_DIAGNOSTICS      0xfffffffd
#define EV_DUMP_ON_FATAL        0xfffffffe
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

#endif
