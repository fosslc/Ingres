/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** File: silocal.h
**
** History:
**	20-jun-95 (emmag)
**	    Renamed SI.h because of a conflict with gl\hdr\hdr\si.h on
**	    platforms which don't support case sensitivity in file names.
**      24-jul-1998 (rigka01)
**          Cross integrate bug 86989 from 1.2 to 2.0 codeline
**          bug 86989: When passing a large CHAR parameter with report,
**          the generation of the report fails due to parm too big
**          Add SI_MAXOUT
*/

# define MAXSTRINGBUF   80

# define SI_BAD_SYNTAX  (E_CL_MASK | E_SI_MASK | 1)
# define SI_CAT_DIR     (E_CL_MASK | E_SI_MASK | 2)
# define SI_CAT_NONE    (E_CL_MASK | E_SI_MASK | 3)
# define SI_CANT_OPEN    (E_CL_MASK | E_SI_MASK | 4)

#define SI_MAXOUT 4096

# ifdef NT_GENERIC
GLOBALREF BOOL SIchar_mode;
# endif
