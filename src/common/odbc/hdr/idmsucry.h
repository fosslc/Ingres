/*
**  Copyright (c) 2004 Ingres Corporation
*/

/*
**  Name: IDMSUCRY
**
**  Description:
**	Password encrytion routine prototypt
**
**  History:
**	15-dec-1993 (rosda01)
**	    Initial coding
**	17-feb-1994 (thoda04)
**	    Unix port
**	03-jun-1994 (rosda01)
**	    Unix port
**	01-apr-2002 (loera01)
**	    Added template for IIODcryp_pwcrypt.
**	08-apr-2002 (somsa01)
**	    Corrected previous changes.
*/

#ifndef _INC_IDMSUCRY
#define _INC_IDMSUCRY

/* avoid global name conflicts under non-Windows environments */
#ifndef NT_GENERIC
#define pwcrypt                    IIODcryp_pwcrypt
int IIODcryp_pwcrypt (const char *, char *);
#else
int pwcrypt (const char *, char *);
#endif /* NT_GENERIC */

#endif
