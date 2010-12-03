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
**	1-Dec-2010 (kschendel) SIR 124685
**	    Kill CL_PROTOTYPED (always on now).
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
	    LOCATION	    *head,
	    LOCATION	    *tail, 
	    LOCATION	    *result
);

FUNC_EXTERN STATUS  LOchange(
	    LOCATION	    *loc
);

FUNC_EXTERN STATUS  LOclear(
	    LOCATION	    *loc
);

FUNC_EXTERN STATUS  LOcompose(
	    char	    *dev,
	    char	    *path,
	    char	    *fprefix,
	    char	    *fsuffix,
	    char	    *version,
	    LOCATION	    *loc
);

FUNC_EXTERN VOID   LOcopy(
	    LOCATION	    *srcloc, 
	    char	    *destlocbuf, 
	    LOCATION	    *destloc
);

FUNC_EXTERN STATUS  LOcreate(
	    LOCATION	    *dirloc
);

FUNC_EXTERN STATUS  LOdelete(
	    LOCATION	    *loc
);

FUNC_EXTERN STATUS  LOdetail(
	    LOCATION	    *loc, 
	    char	    dev[], 
	    char	    path[], 
	    char	    fprefix[], 
	    char	    fsuffic[], 
	    char	    version[]
);

FUNC_EXTERN STATUS  LOendlist(
	    LOCATION	    *loc
);

FUNC_EXTERN STATUS  LOexist(
	    LOCATION	    *loc
);

FUNC_EXTERN STATUS  LOfaddpath(
	    LOCATION	    *head, 
	    char	    *tail, 
	    LOCATION	    *result
);

FUNC_EXTERN STATUS  LOfroms(
	    LOCTYPE	    what, 
	    char	    buf[], 
	    LOCATION	    *loc
);

FUNC_EXTERN STATUS  LOfstfile(
	    char	    *filename, 
	    LOCATION	    *loc
);

FUNC_EXTERN STATUS  LOgt(
	    char	    buf[], 
	    LOCATION	    *loc
);

FUNC_EXTERN STATUS  LOfakepwd(
	    char	    buf[], 
	    LOCATION	    *loc
);

FUNC_EXTERN STATUS  LOgtfile(
            LOCATION	    *loc,
            LOCATION        *fileloc
);

FUNC_EXTERN STATUS  LOinfo(
	    LOCATION	    *loc, 
	    i4		    *flags, 
	    LOINFORMATION   *locinfo
);

FUNC_EXTERN STATUS  LOingdir(
	    char	    *dbname, 
	    char	    *what, 
	    char	    *dirname
);

FUNC_EXTERN STATUS  LOingpath(
	    char	    *area, 
	    char	    *dbname, 
	    char	    *what, 
	    LOCATION	    *fullpath
);

FUNC_EXTERN STATUS  LOmkingpath(
	    char	    *area, 
	    char	    *what, 
	    LOCATION	    *loc
);

FUNC_EXTERN STATUS  LOisdir(
	    LOCATION	    *dirloc, 
	    i2		    *flag
);

FUNC_EXTERN bool    LOisfull(
	    LOCATION	    *loc
);

FUNC_EXTERN STATUS  LOlast(
	    LOCATION	    *loc, 
	    SYSTIME	    *t
);

FUNC_EXTERN STATUS  LOlist(
	    LOCATION	    *inputloc, 
	    LOCATION	    *outputloc
);

FUNC_EXTERN STATUS  LOpurge(
	    LOCATION	    *loc, 
	    i4		    keep
);

FUNC_EXTERN STATUS  LOrename(
	    LOCATION	    *oldfname, 
	    LOCATION	    *newfname
);

#ifdef NT_GENERIC
FUNC_EXTERN STATUS  LOsetperms(
	    char	*path, 
	    i4		perms
);
#endif  /* NT_GENERIC */

FUNC_EXTERN STATUS  LOsize(
	    LOCATION	    *loc, 
	    OFFSET_TYPE	    *loc_size
);

FUNC_EXTERN STATUS  LOstfile(
	    LOCATION	    *filename, 
	    LOCATION	    *loc
);

FUNC_EXTERN VOID  LOtos(
	    LOCATION	    *loc, 
	    char	    **string
);

FUNC_EXTERN VOID  LOtoes(
	    LOCATION	    *loc, 
	    char	    *psChar,
	    char	    *pszIn
);

FUNC_EXTERN STATUS  LOuniq(
	    char	    *pat, 
	    char	    *suffix, 
	    LOCATION	    *path
);

FUNC_EXTERN bool    LOisvalid(
	    LOCATION	    *loc,
        STATUS* status);

FUNC_EXTERN STATUS  LOwcard(
	    LOCATION	    *loc,
	    char	    *fprefix,
	    char	    *fsuffix,
	    char	    *version,
	    LO_DIR_CONTEXT  *lc
);

FUNC_EXTERN STATUS  LOwend(
	    LO_DIR_CONTEXT  *lc
);

FUNC_EXTERN STATUS  LOwhat(
	    LOCATION	    *loc, 
	    LOCTYPE	    *flag
);

FUNC_EXTERN STATUS  LOwnext(
	    LO_DIR_CONTEXT  *lc, 
	    LOCATION	    *loc
);

FUNC_EXTERN STATUS  LOerrno(
	    i4              errnum 
);

#endif /* LO_INCLUDE */
