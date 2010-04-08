/*
** File: silocal.h
**
** History:
**	20-jun-95 (emmag)
**	    Renamed SI.h because of a conflict with gl\hdr\hdr\si.h on
**	    platforms which don't support case sensitivity in file names.
**	26-jan-1998 (canor01)
**	    Add prototype for GetOpenRoadStyle().
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
*/

# define MAXSTRINGBUF   80

# define SI_BAD_SYNTAX  (E_CL_MASK | E_SI_MASK | 1)
# define SI_CAT_DIR     (E_CL_MASK | E_SI_MASK | 2)
# define SI_CAT_NONE    (E_CL_MASK | E_SI_MASK | 3)
# define SI_CANT_OPEN    (E_CL_MASK | E_SI_MASK | 4)

GLOBALREF BOOL SIchar_mode;
FUNC_EXTERN BOOL SIget_charmode();
FUNC_EXTERN BOOL GetOpenRoadStyle();

FUNC_EXTERN VOID SIdofrmt(
#ifdef CL_PROTOTYPED
	i4              calltype,
	FILE            *outarg,
	char            *fmt,
	va_list         ap
#endif
);
