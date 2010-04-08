/**
** Name:    dl32.h -	Dynamic Linking Definitions File.
**
**
** History:
**      08/11/92 puree
**          written for OS/2 PM.
**      09/92    valerier
**          modified for Windows NT
**		11-mar-94 (rion)
**			Removed defines for error codes.  Use ones in erclf.h
**		4-may-94 (leighb)
**			Added DLwin32s() prototype.
**	19-nov-1996 (donc)
**	    Include erclf.h
**	    
*/

#include <erclf.h>

/*
** capability symbols
*/
#define  DL_MAX_MOD_NAME        8
#define  DL_LOADING_WORKS       TRUE
#define  DL_UNLOADINGS_WORKS    TRUE

/*
** misc. symbol
*/
# define DL_DATE_NAME    "DL_DATE_NAME"
# define DL_IDENT_NAME   "DL_IDENT_NAME"
# define DL_IDENT_STRING "DL_IDENT_STRING"

/*
** error codes
**
** The error codes seen by the clients of CL are in erclf.h
** in common\hdr\hdr.
*/
#define DL_FUNC_NOT_FOUND       E_CL0826_DL_FUNC_NOT_FOUND
/*  A function requested is not part of this DL set associated with
**  the handle.
*/
#define DL_MOD_COUNT_EXCEEDED   E_CL0823_DL_MOD_COUNT_EXCEEDED
/*  No more than one DL set may be prepared for binding at one time.  A
**  request for a second DLprepare (without and intervening successful
**  call to DLunload will fail.
*/
#define DL_MOD_NOT_FOUND        E_CL0824_DL_MOD_NOT_FOUND
/*  One or more of the DL module could not be found
*/
#define DL_NOT_IMPLEMENTED      E_CL0816_DL_NOT_IMPLEMENTED
/*  The DL mechanism is not implemented on this platform.
*/
#define DL_WRONG_VERSION_MOD    E_CL0813_DL_VERSION_WRONG
/*  The module didn't contain a version or the versions didn't match.
*/

/*
** function prototypes
*/

STATUS DLbind(PTR handle, char *sym, PTR *addr, CL_ERR_DESC *err);
STATUS DLcreate(char *exename, char *vers, char *dlmodname, 
        char *in_objs[], char *in_libs[], char *exp_fncs[], 
        CL_ERR_DESC *err);
STATUS DLdelete(char *dlmodname, CL_ERR_DESC *err);
STATUS DLnameavail(char *dlmodname, CL_ERR_DESC *err);
STATUS DLprepare(char *vers, char *dlmodname, char *syms[], PTR *handle,
        CL_ERR_DESC *err);
STATUS DLunload(PTR handle, CL_ERR_DESC *err);
BOOL   DLwin32s(void);

#define ADF_CONTEXT	1
#define CL_CONTEXT	2
#define LIBQ_CONTEXT	3

