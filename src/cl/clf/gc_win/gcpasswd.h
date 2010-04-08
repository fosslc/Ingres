/*
** Copyright (c) 1996, 2009 Ingres Corporation
**
** Name: GCPASSWD.H
**
** Description:
**	Contains defines and/or variables used by gcpasswd.c (function
**	GCpasswd()).
**
** History:
**      13-Nov-96 (fanra01)
**	    Created.
**	17-Dec-2009 (Bruce Lunsford) Sir 122536
**	    Add support for long identifiers.  Set GC_L_USER_ID to 
**	    GC_USERNAME_MAX rather than hard-coded 32.  GC_USERNAME_MAX
**	    is new Ingres CL #define for max username for the OS; does
**	    not include byte for EOS.
**
*/
#include "gcdlg.h"

/***  Following GC_L* lengths include a byte for EOS  ***/
#define GC_L_NODE_ID	64
#ifdef GC_USERNAME_MAX
#define GC_L_USER_ID	GC_USERNAME_MAX + 1
#else
#define GC_L_USER_ID	32
#endif
#define	GC_L_PASSWORD	64

#define	WM_VNODE_CACHE_LOOKUP	WM_USER+256
#define WM_VNODE_CACHE_STORE	WM_USER+257
#define WM_VNODE_CACHE_DELETE	WM_USER+258
#define WM_VNODE_CACHE_CLEAR	WM_USER+259
#define WM_VNODE_CACHE_DIAG		WM_USER+260

#define IIPROMPTONCE "IIPROMPTONCE"
#define IIPROMPT1 "IIPROMPT1"
#define IIPROMPTALL "IIPROMPTALL"

#define	ERRMSG	"Neither the Username nor Password\r\nfields can be blank"
#define IIW3VNODECACHEWINDOW "iiw3VnodeCacheWindow"
#define IIWEVNODECACHEPROG	"iiw3vche.exe"


typedef struct t_vnodecache
{
	char vnode[GC_L_NODE_ID];
	char username[GC_L_USER_ID];
	char password[GC_L_PASSWORD];
} VNODECACHE;
