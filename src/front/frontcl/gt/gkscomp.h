/*
**	Copyright (c) 2004 Ingres Corporation
*/
#include	<ex.h>
#include	<si.h>

#undef min
#undef max
#define RTIOK 0
#define RTIFAIL 1
#define RTISTATUS i4
#undef OK

#include "gks.h"
#include "cchart.h"

# define	min(x,y)		((x) < (y) ? (x) : (y))

/*
**	TTYLOC is the workstation location for the terminal.  On UNIX
**	we open a separate pointer because VE calls ioctl and destroys
**	forms mode if we simply use standard output ("-" means stdout).
**
**	27-Feb-89 (Mike S) Add names of null devices
**
**	26-jan-89 (Mike S) Add terminal name for DG
**
**	11-oct-86 (bab) Fix for bug 10806
**		Changed '-' to 'tt' on VMS to avoid closing file buffers
**		before their time.
*/
# ifdef vms
#  define	TTYLOC 	ERx("tt")
#  define	NULLDEV ERx("nla0:")
# endif
# ifdef DGC_AOS
#  define	TTYLOC 	ERx("@console")
#  define	NULLDEV ERx("@null")
# endif
# ifndef TTYLOC
#  define	TTYLOC 	ERx("/dev/tty")
#  define	NULLDEV ERx("/dev/null")
# endif

