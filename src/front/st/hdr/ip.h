/*
**  Copyright (c) 2004 Ingres Corporation
**
**  ip.h -- main header file for "Installation of Packages" module.
**
**  History:
**
**	xx-xxx-1992 (jonb)
**		Created.
**	17-mar-93 (jonb)
**		Added support for "Permission" declaration at the package
**		and part levels.
**	22-mar-93 (jonb)
**	        Removed list-handling function prototypes.
**	1-apr-93 (kellyp)
**		Update VERSION_ID.  Added argument to ip_delete_inst_pkg()
**		prototype.  Created ip_distrib_dev() prototype.
**	2-apr-93 (kellyp)
**		Added member "dynamic" to FILBLK structure.
**	13-apr-93 (kellyp)
**		Updated VERSION_ID.
**	14-apr-93 (kellyp)
**		Updated VERSION_ID.
**	20-apr-93 (kellyp)
**		Added member "weight" to FILBLK structure.
**	27-apr-93 (kellyp)
**		Updated VERSION_ID.
**	01-jul-93 (vijay)
**		Bump up VERSION_ID to match erst.h.
**	13-jul-93 (tyler)
**		Made changes required to make manifest language portable.
**		Also added history comments for 6 previous changes by
**		kellyp.  Cleaned up format.
**	23-jul-93 (tyler)
**		Incremented VERSION_ID for upcoming integration.
**	04-aug-93 (tyler)
**		Remove "volume" field from PKGBLK and PRTBLK structures.
**	19-sep-93 (kellyp)
**		Added setup to the FILE structure.
**	19-sep-93 (kellyp)
**		A new revision to add previous History comment.
**	20-aug-93 (kellyp)
**		Added "setup" field to FILBLK structure
**	24-sep-93 (tyler)
**		Added lost History.
**	19-oct-93 (tyler)
**		Removed GENERATE_VERSION symbol.
**	02-nov-93 (tyler)
**		Added definition for PREFER directive (reference block type).
**		Removed ununsed weight attribute.
**	11-nov-93 (tyler)
**		Moved (most) platform specific definitions into ipcl.h
**		(IP Compatibility Layer header) and removed prototypes
**		for defunct functions.
**	01-dec-93 (tyler)
**		Added argument to ip_parse() prototype.
**	05-jan-94 (tyler)
**		Removed support for SUBSUME keyword.
**	22-feb-94 (tyler)
**		Added member "visited" to PKGBLK structure to fix BUG
**		58928.
**	22-mar-95 (athda01)
**		Added prototype for new function ip_lochelp (b67639)
**	11-nov-95 (hanch04)
**		Added new token VISIBLEPKG
**	19-march-1996 (angusm)
**		Add prototypes for ip_overlay, ip_find_file
**	19-feb-1996 (boama01)
**		Added 'aggregate' member to _PKGBLK struct, for new AGGREGATE
**		pkg attribute which allows OR-ed part capabilities/licensing.
**	21-sep-2001 (gupsh01)
**		Added function definitions for mkresponse option for ingbuild.
**	05-Jan-2006 (bonro01)
**		Remove i18n, and documentation from default ingres.tar
**       9-May-2007 (hanal04) bug 117075
**              Added mode parameter to ip_open_response()
*/

/*
** Unfortunately, it's not possible to move the definitions of PERMISSION
** into ipcl.h, since this file refers the PERMISSION type, and ipcl.h
** depends on data structures declared in this file. 
*/

# ifdef VMS

typedef char * PERMISSION;
# define NULL_PERMISSION NULL

# else /* VMS */

typedef i4  PERMISSION;
# define NULL_PERMISSION -1

# endif /* VMS */

/* The name of the manifest file... */

# define RELEASE_MANIFEST	ERx( "release.dat" )

/* The name of the installation descriptor file... */

# define INSTALLATION_MANIFEST	ERx( "install.dat" )

/* The name of the special OEM authorization file... */

# define OEM_AUTH_FILE		ERx( "OEMSTRING" )

/* The ID of the only user allowed to run the install program... */

# define REQD_USER_ID		ERx( "ingres" )

/* The maximum number of verification errors before we complain... */

# define VER_ERR_MAX		5

/* Maximum Manifest/Package/Part file line length */

# define MAX_MANIFEST_LINE	80	

# ifndef uchar
# define uchar char
# endif

/*
** Package block structure. 
*/

typedef struct _PKGBLK
{
	char		*file;		/* Name of package descriptor file */
	char		*name;		/* Description of the package */
	char		*version;	/* Version or release string */
	char		*feature;	/* Short name used for install subdir */
	char		*description;	/* Full description file or NULL */
	LIST		prtList;	/* List of part blocks */
	LIST		refList;	/* List of depended-on packages. */
	i4		actual_size;	/* Size of package minus included */
	i4		apparent_size;	/* Total size with included */
	i4		nfiles;		/* Number of files in the package. */
# ifdef OMIT
	bool		delete;		/* Used during package removal */
# endif /* OMIT */
	bool		visited;	/* Used during size computation */
	i4		state;		/* Setup state; valid states are: */

# define NOT_INSTALLED		0
# define NOT_SETUP		1
# define INSTALLED 		2
# define NEWLY_INSTALLED	3
# define UNDEFINED 		4

	i4		image_cksum;	/* Checksum of the image file */
	i4		image_size;	/* Size of the image file in bytes */
	uchar		selected;	/* Selection method - valid methods: */ 

# define NOT_SELECTED		0
# define SELECTED		1
# define INCLUDED		2
# define DESELECTED		3

	i4		visible;	/*
					** This flag determines whether
					** package is visible, invisible,
					** or conditionally visible.  It's
					** value must be a capability bit
					** (number) or one of the following:
					*/
# define VISIBLEPKG		997
# define VISIBLE		998
# define INVISIBLE		999
# define NOT_BIAS		500

	char		tag[20];	/* Contents of "Install?" field */
	bool		aggregate;	/* Pkg is aggregate of OR-ed parts */
	bool		hide;		/* Package is ommited from build */
} PKGBLK;

/*
** Part block structure.
*/

typedef struct _PRTBLK
{
	char		*file;		/* Name of part descriptor file */
	char		*name;		/* Name of part; not used for much */
	char		*version;	/* Version level of part */
	i4		size;		/* Total size of part */
	char		*preload;	/* Pre-load program - NULL if none */
	char		*delete;	/* Delete program name - NULL if none */
	PERMISSION	permission;	/* Default file permission */
	LIST		refList;	/* List of setup programs */
	LIST		capList;	/* List of capability bits */
	LIST		filList;	/* List of files in part */
} PRTBLK;

/*
** File block structure.
*/ 

typedef struct _FILBLK
{
	char 		*name;		/* Target file name */
	char 		*build_file;	/* Build area source file name */
	char 		*build_dir;	/* Build area source directory */
	char		*directory;	/* Target directory (i4ive) */
	char		*generic_dir;	/* Target directory (generic) */
	char 		*volume;	/* Release media target volume */ 
	PERMISSION	permission_is;	/* Observed permission on file */
	PERMISSION	permission_sb;	/* Permission file should have */
	i4		checksum;	/* File checksum */
	i4		size;		/* File size (bytes) */
	bool		custom;		/* Is file customizable? */
	bool		dynamic;	/* Is file checksum dynamic? */
	bool		setup;		/* Is this a setup program? */
	bool		exists;		/* Does file exist in the build area? */
	bool 		link;		/* Is file a Unix link? */
} FILBLK;

/*
** Reference block structure.
*/

typedef struct _REFBLK
{
	char		*name;		/* Name of referenced object */
	char		*version;	/* Version in "need" reference */
	uchar		comparison;	/*
					** Result of comparison of version
					** number of what's being installed
					** and required number; valid results
					** are:
					*/
# define COMP_EQ 1
# define COMP_LE 2
# define COMP_LT 3
# define COMP_GE 4
# define COMP_GT 5

	uchar		type;		/* Type of reference */

# define INCLUDE	0
# define NEED		1
# define SETUP		2
# define PREFER		3

} REFBLK;

/*
**  Structure defining a parse token...
*/

typedef struct _MAP
{
        char *token;
        i4  id;
} MAP;

/*
**  Codes returned from ip_sysstat()...
*/

# define IP_NOSYS	0
# define IP_SYSUP	1
# define IP_SYSDOWN	2

/*
**  Parameter controlling the action of ip_find_package()...
*/

# define INSTALLATION   0
# define DISTRIBUTION	1

/*
**  Parameter controlling the action of ip_setup() and ip_preload()...
*/

# define _CHECK   0
# define _PERFORM 1
# define _DISPLAY 2

/*
**  Special return code from ip_parse()...
*/

# define PARSE_FAIL -99

/*
**  The following is from cl/clf/ci/cilocal.h, so I guess I'm not supposed
**  to know about it.  Why do you suppose that is?
*/

# define        CI_EROFFSET             (E_CL_MASK  + E_CI_MASK)
 
# define        CI_NOSTR                1 + CI_EROFFSET
# define        CI_TOOLITTLE            2 + CI_EROFFSET
# define        CI_TOOBIG               3 + CI_EROFFSET
# define        CI_BADCHKSUM            4 + CI_EROFFSET
# define        CI_BADEXPDATE           5 + CI_EROFFSET
# define        CI_BADSERNUM            6 + CI_EROFFSET
# define        CI_BADCPUMODEL          7 + CI_EROFFSET
# define        CI_BADERROR             8 + CI_EROFFSET
# define        CI_BADKEY               9 + CI_EROFFSET
# define        CI_NOCLUSTER            10 + CI_EROFFSET
# define        CI_NOCAP                20 + CI_EROFFSET

/*
** modes for ip_overlay
*/
# define	PATCHIN			1
# define	PATCHOUT		0
/*
** The following needed to be migrated into ipcl.c:
*/

FUNC_EXTERN void	IPCLdeleteDir( char * );

/*
** The following should be implemented as a pre-install program for
** the server.
*/
FUNC_EXTERN i4          ip_sysstat( void );

/*
**  Function prototypes...
*/

FUNC_EXTERN void        ip_add_inst_pkg( PKGBLK * ); 
FUNC_EXTERN PTR         ip_alloc_blk( i4  ); 
FUNC_EXTERN STATUS	ip_batch( i4  , char ** );
FUNC_EXTERN char *	ip_bit2ci( i4  );
FUNC_EXTERN void	ip_clear_auth( void );
FUNC_EXTERN STATUS	ip_cmdline( char *, ER_MSGID );
FUNC_EXTERN bool	ip_compare_name( char *, PKGBLK *);
FUNC_EXTERN bool	ip_comp_version( char *, uchar , char *);
FUNC_EXTERN void	ip_delete_dist_pkg( PKGBLK * );
FUNC_EXTERN bool	ip_delete_inst_pkg( PKGBLK *, char * );
FUNC_EXTERN bool	ip_dep_popup( PKGBLK *, PKGBLK *, char * );
FUNC_EXTERN void	ip_describe( char * );
FUNC_EXTERN void        ip_display_msg( char * );
FUNC_EXTERN void        ip_display_transient_msg( char * );
FUNC_EXTERN STATUS      ip_file_info( LOCATION *, char *, i4  *, i4  * );
FUNC_EXTERN STATUS      ip_file_info_comp( LOCATION *, i4  *, i4  *, bool );
FUNC_EXTERN char *	ip_find_dir( PKGBLK *, char * );
FUNC_EXTERN PKGBLK *    ip_find_feature( char *, i4  );
FUNC_EXTERN FILBLK *	ip_find_file(char *, i4);
FUNC_EXTERN PKGBLK *    ip_find_package( char *, i4  );
FUNC_EXTERN void        ip_forms( bool );
FUNC_EXTERN void        ip_init_authorization( void );
FUNC_EXTERN void        ip_init_installdir( void );
FUNC_EXTERN void	ip_install( char * );
FUNC_EXTERN char *	ip_install_loc( void );
FUNC_EXTERN char *	ip_install_file( char * );
FUNC_EXTERN bool	ip_is_visible( PKGBLK * );
FUNC_EXTERN bool	ip_is_visiblepkg( PKGBLK * );
FUNC_EXTERN STATUS	ip_licensed( PKGBLK * );
FUNC_EXTERN i4		ip_listpick(char *, 
				    ER_MSGID, ER_MSGID, ER_MSGID, ER_MSGID);
FUNC_EXTERN void	ip_lochelp( char *, char * );	/* b67639 athda01 */
FUNC_EXTERN STATUS	ip_oemstring( char * );
FUNC_EXTERN VOID	ip_overlay( PKGBLK *, i4);
FUNC_EXTERN STATUS      ip_parse( i4, LIST *, char **, char **, bool );
FUNC_EXTERN STATUS	ip_preserve_cust_files( void );
FUNC_EXTERN bool	ip_preload( PKGBLK *, i4  );
FUNC_EXTERN bool	ip_resolve( char * );
FUNC_EXTERN void	ip_set_auth( char * );
FUNC_EXTERN bool        ip_setup( PKGBLK *, i4  );
FUNC_EXTERN char *      ip_stash_string( char * ); 
FUNC_EXTERN void	ip_environment( void );
FUNC_EXTERN bool	ip_verify_auth( void );
FUNC_EXTERN STATUS	ip_verify_image( PKGBLK * );
FUNC_EXTERN STATUS	ip_verify_files( PKGBLK * );
FUNC_EXTERN STATUS      ip_write_manifest( i4, char *, LIST *, char *,
				char * );
FUNC_EXTERN STATUS      ip_open_response( char *dir, char *filename, 
                            FILE **resp_file, char *mode);
FUNC_EXTERN STATUS	ip_write_response( char *var, char *value, 
			    FILE *resp_file);
FUNC_EXTERN STATUS	ip_close_response( FILE *resp_file);
FUNC_EXTERN STATUS 	check_pkg_response(char *pkg_name);
