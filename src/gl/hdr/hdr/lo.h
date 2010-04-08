/*
** Copyright (c) 2004 Ingres Corporation
*/
#ifndef LO_INCLUDE
#define LO_INCLUDE
#include    <locl.h>

/**CL_SPEC
** Name:	LO.h	- Define LO function externs
**
** Specification:
**
** Description:
**	Contains LO function externs
**
** History:
**	2-jun-1993 (ed)
**	    initial creation to define func_externs in glhdr
**	13-jun-1993 (ed)
**	    make safe for multiple inclusions
**	24-jan-1994 (gmanning)
**	    Change 'char *destlocbuf[]' to 'char *destlocbuf' in prototype.
**	29-sep-1998 (canor01)
**	    Add prototype for LOgtfile().
**	28-feb-2000 (somsa01)
**	    Changed argument type to LOsize().
**      05-Jun-2000 (hanal04) Bug 101667 INGDBA 65
**          Added LOfakepwd().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	15-Oct-2001 (jenjo02)
**	    Added prototype for LOmkingpath().
**	22-Oct-2001 (somsa01)
**	    For NT, added prototype for LOsetperms().
**	12-apr-2002 (abbjo03)
**	    Add prototype for LOisvalid().
**	21_May-2003 (bonro01)
**	    Add prototype for LOerrno() to prevent SEGV
**  01-Jul-2005 (fanra01)
**      Bug 114804
**      Add a status output to LOisvalid.
**      Moved LO status codes into lo.h for consistent values on all
**      platforms.
**	28-feb-08 (smeke01) b120003
**	    Added prototype for LOtoes.
*/

/*
** LO return status codes.
*/
/* LO return status codes. */

# define LO_OK          0
# define LO_ERR_CLASS   (E_CL_MASK + E_LO_MASK)

# define LO_ADD_BAD_SYN (LO_ERR_CLASS + 0x01)   /* LOaddpath:Bad syntax in
                                                ** arguments to LOaddpath --
                                                ** tail shouldn't begin at root
                                                */
# define LO_FR_BAD_SYN  (LO_ERR_CLASS + 0x02)   /* LOfroms:String argument to
                                                ** LOfroms has bad syntax.
                                                */
# define LO_NO_SUCH     (LO_ERR_CLASS + 0x03)   /* No such location exists. */
# define LO_NO_PERM     (LO_ERR_CLASS + 0x04)   /* You do not have permision
                                                ** to access the location with
                                                ** this call.
                                                */
# define LO_NO_SPACE    (LO_ERR_CLASS + 0x05)   /* Not enough space on file
                                                ** system.
                                                */
# define LO_NOT_FILE    (LO_ERR_CLASS + 0x06)   /* Location argument was not a
                                                ** FILENAME.
                                                */
# define LO_NULL_ARG    (LO_ERR_CLASS + 0x07)   /* Illegal null pointer was
                                                ** passed to location routine.
                                                */
# define LO_CANT_TELL   (LO_ERR_CLASS + 0x08)
# define LONOSAVE       (LO_ERR_CLASS + 0x09)   /* LOreset called without
                                                ** matching call to LOsave
                                                */
# define LO_TOO_LONG    (LO_ERR_CLASS + 0x0A)   /* LOgt got location bigger
                                                ** than MAX_LOC
                                                */
# define LO_NOT_PATH    (LO_ERR_CLASS + 0x0B)   /* Location argument was not a
                                                ** PATH.
                                                */
# define LO_BAD_DEVICE  (LO_ERR_CLASS + 0x0C)   /* The device component of a
                                                ** location is not valid.
                                                */
# define LO_NEST_DIR    (LO_ERR_CLASS + 0x0D)   /* LOdelete can't delete nested
                                                ** directories
                                                */
# define LO_IS_RAW      (LO_ERR_CLASS + 0x0E)   /* LOingpath: location is
                                                ** writable RAW device
                                                */
                                                

FUNC_EXTERN STATUS  LOaddpath(
#ifdef CL_PROTOTYPED
	    LOCATION	    *head,
	    LOCATION	    *tail, 
	    LOCATION	    *result
#endif
);

FUNC_EXTERN STATUS  LOchange(
#ifdef CL_PROTOTYPED
	    LOCATION	    *loc
#endif
);

FUNC_EXTERN STATUS  LOclear(
#ifdef CL_PROTOTYPED
	    LOCATION	    *loc
#endif
);

FUNC_EXTERN STATUS  LOcompose(
#ifdef CL_PROTOTYPED
	    char	    *dev,
	    char	    *path,
	    char	    *fprefix,
	    char	    *fsuffix,
	    char	    *version,
	    LOCATION	    *loc
#endif
);

FUNC_EXTERN VOID   LOcopy(
#ifdef CL_PROTOTYPED
	    LOCATION	    *srcloc, 
	    char	    *destlocbuf, 
	    LOCATION	    *destloc
#endif
);

FUNC_EXTERN STATUS  LOcreate(
#ifdef CL_PROTOTYPED
	    LOCATION	    *dirloc
#endif
);

FUNC_EXTERN STATUS  LOdelete(
#ifdef CL_PROTOTYPED
	    LOCATION	    *loc
#endif
);

FUNC_EXTERN STATUS  LOdetail(
#ifdef CL_PROTOTYPED
	    LOCATION	    *loc, 
	    char	    dev[], 
	    char	    path[], 
	    char	    fprefix[], 
	    char	    fsuffic[], 
	    char	    version[]
#endif
);

FUNC_EXTERN STATUS  LOendlist(
#ifdef CL_PROTOTYPED
	    LOCATION	    *loc
#endif
);

FUNC_EXTERN STATUS  LOexist(
#ifdef CL_PROTOTYPED
	    LOCATION	    *loc
#endif
);

FUNC_EXTERN STATUS  LOfaddpath(
#ifdef CL_PROTOTYPED
	    LOCATION	    *head, 
	    char	    *tail, 
	    LOCATION	    *result
#endif
);

FUNC_EXTERN STATUS  LOfroms(
#ifdef CL_PROTOTYPED
	    LOCTYPE	    what, 
	    char	    buf[], 
	    LOCATION	    *loc
#endif
);

FUNC_EXTERN STATUS  LOfstfile(
#ifdef CL_PROTOTYPED
	    char	    *filename, 
	    LOCATION	    *loc
#endif
);

FUNC_EXTERN STATUS  LOgt(
#ifdef CL_PROTOTYPED
	    char	    buf[], 
	    LOCATION	    *loc
#endif
);

FUNC_EXTERN STATUS  LOfakepwd(
#ifdef CL_PROTOTYPED
	    char	    buf[], 
	    LOCATION	    *loc
#endif
);

FUNC_EXTERN STATUS  LOgtfile(
#ifdef CL_PROTOTYPED
            LOCATION	    *loc,
            LOCATION        *fileloc
#endif
);

FUNC_EXTERN STATUS  LOinfo(
#ifdef CL_PROTOTYPED
	    LOCATION	    *loc, 
	    i4		    *flags, 
	    LOINFORMATION   *locinfo
#endif
);

FUNC_EXTERN STATUS  LOingdir(
#ifdef CL_PROTOTYPED
	    char	    *dbname, 
	    char	    *what, 
	    char	    *dirname
#endif
);

FUNC_EXTERN STATUS  LOingpath(
#ifdef CL_PROTOTYPED
	    char	    *area, 
	    char	    *dbname, 
	    char	    *what, 
	    LOCATION	    *fullpath
#endif
);

FUNC_EXTERN STATUS  LOmkingpath(
#ifdef CL_PROTOTYPED
	    char	    *area, 
	    char	    *what, 
	    LOCATION	    *loc
#endif
);

FUNC_EXTERN STATUS  LOisdir(
#ifdef CL_PROTOTYPED
	    LOCATION	    *dirloc, 
	    i2		    *flag
#endif
);

FUNC_EXTERN bool    LOisfull(
#ifdef CL_PROTOTYPED
	    LOCATION	    *loc
#endif
);

FUNC_EXTERN STATUS  LOlast(
#ifdef CL_PROTOTYPED
	    LOCATION	    *loc, 
	    SYSTIME	    *t
#endif
);

FUNC_EXTERN STATUS  LOlist(
#ifdef CL_PROTOTYPED
	    LOCATION	    *inputloc, 
	    LOCATION	    *outputloc
#endif
);

FUNC_EXTERN STATUS  LOpurge(
#ifdef CL_PROTOTYPED
	    LOCATION	    *loc, 
	    i4		    keep
#endif
);

FUNC_EXTERN STATUS  LOrename(
#ifdef CL_PROTOTYPED
	    LOCATION	    *oldfname, 
	    LOCATION	    *newfname
#endif
);

#ifdef NT_GENERIC
FUNC_EXTERN STATUS  LOsetperms(
#ifdef CL_PROTOTYPED
	    char	*path, 
	    i4		perms
#endif
);
#endif  /* NT_GENERIC */

FUNC_EXTERN STATUS  LOsize(
#ifdef CL_PROTOTYPED
	    LOCATION	    *loc, 
	    OFFSET_TYPE	    *loc_size
#endif
);

FUNC_EXTERN STATUS  LOstfile(
#ifdef CL_PROTOTYPED
	    LOCATION	    *filename, 
	    LOCATION	    *loc
#endif
);

FUNC_EXTERN VOID  LOtos(
#ifdef CL_PROTOTYPED
	    LOCATION	    *loc, 
	    char	    **string
#endif
);

FUNC_EXTERN VOID  LOtoes(
#ifdef CL_PROTOTYPED
	    LOCATION	    *loc, 
	    char	    *psChar,
	    char	    *pszIn
#endif
);

FUNC_EXTERN STATUS  LOuniq(
#ifdef CL_PROTOTYPED
	    char	    *pat, 
	    char	    *suffix, 
	    LOCATION	    *path
#endif
);

FUNC_EXTERN bool    LOisvalid(
	    LOCATION	    *loc,
        STATUS* status);

FUNC_EXTERN STATUS  LOwcard(
#ifdef CL_PROTOTYPED
	    LOCATION	    *loc,
	    char	    *fprefix,
	    char	    *fsuffix,
	    char	    *version,
	    LO_DIR_CONTEXT  *lc
#endif
);

FUNC_EXTERN STATUS  LOwend(
#ifdef CL_PROTOTYPED
	    LO_DIR_CONTEXT  *lc
#endif
);

FUNC_EXTERN STATUS  LOwhat(
#ifdef CL_PROTOTYPED
	    LOCATION	    *loc, 
	    LOCTYPE	    *flag
#endif
);

FUNC_EXTERN STATUS  LOwnext(
#ifdef CL_PROTOTYPED
	    LO_DIR_CONTEXT  *lc, 
	    LOCATION	    *loc
#endif
);

FUNC_EXTERN STATUS  LOerrno(
#ifdef CL_PROTOTYPED
	    i4              errnum 
#endif
);

#endif /* LO_INCLUDE */
