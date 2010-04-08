/*
** Copyright (c) 1995, 2008 Ingres Corporation
*/
/*
** HANDY.H -- Header file for iiCL functions located in the "handy" directory.
**
** This provides a reentrant interface to various functions accessed 
** throughout the CL.
**
** History:
**      30-may-95 (tutto01)
**	Created.
**	04-Dec-1995 (walro03/mosjo01)
**		"const" causes compile errors on DG/UX (dg8_us5 and dgi_us5).
**		Replaced with READONLY.
**	03-jun-1996 (canor01)
**	    iiCLgetpwuid() and iiCLgetpwnam() need to be passed pointers
**	    to struct passwd.
**	01-may-1997 (toumi01)
**	    Add length parm to iiCLfcvt and iiCLecvt prototypes to support
**	    current POSIX revision calls to fcvt_r and ecvt_r.
**	15-Jan-1999 (fanra01)
**          Add prototype for iiCLttyname.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	01-dec-2008 (joea)
**	    Exclude iiCLgetpwnam prototype (which has incorrect usage of 
**	    READONLY) from being visible on VMS.
*/


FUNC_EXTERN struct dirent *iiCLreaddir(
		DIR *dirp,
		struct dirent *readdir_buf
);

#ifndef VMS
FUNC_EXTERN struct passwd *iiCLgetpwnam(
		READONLY char *name,
		struct passwd *pwd,
		char *pwnam_buf,
		int size
);
#endif

#ifndef DESKTOP
FUNC_EXTERN struct passwd *iiCLgetpwuid(
		uid_t uid,
		struct passwd *pwd,
		char *pwuid_buf,
		int size
);
#endif /* DESKTOP */

FUNC_EXTERN struct hostent *iiCLgethostbyname(
		char *name,
		struct hostent *result,
		char *buffer,
		int buflen,
		int *h_errnop
);

FUNC_EXTERN struct hostent *iiCLgethostbyaddr(
		char *addr,
		int len,
		int type,
		struct hostent *result,
		char *buffer,
		int buflen,
		int *h_errnop
);

FUNC_EXTERN char *iiCLfcvt(
		double value,
		int ndigit,
		int *decpt,
		int *sign,
		char *buf,
		int len
);

FUNC_EXTERN char *iiCLecvt(
                double value,
                int ndigit,
                int *decpt,
                int *sign,
                char *buf,
		int len
);

FUNC_EXTERN STATUS iiCLttyname (
		i4 filedesc,
		char* buf,
		i4 size
);
