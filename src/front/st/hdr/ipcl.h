/*
**  Copyright (c) 2004 Ingres Corporation
**
**  ipcl.h - header for the IP compatibility layer
**
**  History:
**
**	16-nov-93 (tyler)
**		Created.
**	10-dec-93 (tyler)
**		Added prototype for IPCL_LOfroms().
**	02-Aug-2004 (hanje04)
**		Replace gtar with pax for Open source.
**	17-sep-2004 (stephenb)
**	 	Solaris pax doesn't support compression
**	20-Oct-2004 (bonro01)
**		HP pax does not support compression either.
**	12-Jan-2005 (hweho01)
**	        Added archive file name for AIX.	
**	17-Jan-2005 (lazro01)
**	        UnixWare pax does not support compression.
**	15-Mar-2005 (bonro01)
**		Added archive file name for Solaris AMD64
**	09-Aug-2005 (hweho01)
**		Added archive file name for HP Tru64.
**	13-Feb-2006 (hweho01)
**		Modify archive file name for AIX.
**	31-May-2006 (hweho01)
**		Modify archive file name for HP Tru64 platform.
**	5-Apr-2007 (bonro01)
**		Rearrange INSTALL_ARCHIVE code to match defaults in
**		buildrel.c and ipcl.c
**    26-Apr-2007 (hweho01)
**            Update the archive file name for AIX and Tru64 platforms. 
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
*/

# ifdef VMS

# define TEMP_DEV	"SYS$SCRATCH"
# define TEMP_DIR	NULL

# else /* VMS */

# if defined(LNX)
# define INSTALL_ARCHIVE ERx( "install.pgz" )
# elif defined(any_aix)
# define INSTALL_ARCHIVE ERx( "install.tar.Z" )
# else
# define INSTALL_ARCHIVE ERx( "install.pax.Z" )
# endif

# define TEMP_DEV	NULL
# define TEMP_DIR	"/tmp"

# endif /* VMS */

typedef void IPCL_ERR_FUNC( char * );

bool		IPCLsetDevice( char * );
char		*IPCLbuildPath( char *, IPCL_ERR_FUNC * );
PERMISSION	IPCLbuildPermission( char *, IPCL_ERR_FUNC * );
void		IPCLcleanup( PKGBLK * );
STATUS		IPCLcreateDir( char * );
STATUS		IPCLinstall( PKGBLK * );
STATUS		IPCLtransfer( LIST *, char *);
STATUS		IPCLverify( PKGBLK * );
void		IPCL_LOaddPath( LOCATION *, char * );
STATUS		IPCL_LOfroms( LOCTYPE, char *, LOCATION * );
STATUS		IPCL_LOmove( char *, char * );
void		IPCLsetEnv( char *, char *, bool );
void		IPCLclearTmpEnv( void );
void		IPCLresetTmpEnv( void );

