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
**      07-may-2010 (coomi01)
**          Move SI result codes to si.h
*/

# define MAXSTRINGBUF   80

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
