/*
** Copyright 2006, 2008 Ingres Corporation. All rights reserved.
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

/* Ingres headers */
# include <compat.h>
# include <er.h>
# include <gl.h>
# include <cm.h>
# include <lo.h>
# include <me.h>
# include <pm.h>
# include <si.h>
# include <st.h>
# include <tr.h>

# if defined( xCL_GTK_EXISTS ) && defined( xCL_RPM_EXISTS )

/* RPM headers */
# include <rpmcli.h>
# include <rpmdb.h>
# include <rpmts.h>
# include <rpmlib.h>
# include <rpmlog.h>

# include <gtk/gtk.h>

# include <gip.h>
# include <gipdata.h>
# include <giprpm.h>

char	corefname[MAX_FNAME] = "\0"; /* core package filename */
# define RPM_OPEN_MODE "r.ufdio"
# define SYMBOL_TABLE "/ingres/files/symbol.tbl"

/* string for matching Ingres package names */
static char *ing_pkg_name_pattern[] = {
		"ca-ingres",
		"ca-ingres-[[:upper:]]([[:digit:]]|[[:upper:]])",
		"ingres2006",
		"ingres2006-[[:upper:]]([[:digit:]]|[[:upper:]])",
		"ingres",
		"ingres-[[:upper:]]([[:digit:]]|[[:upper:]])",
		NULL
	};
	
/* RPM defines */
/* libPopt Options table for rpmCli */
static struct poptOption optTbl[] = {
    { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmcliAllPoptTable, 0,
	"Commmon options for all rpm modes and executables:",
	NULL },
      POPT_AUTOALIAS
      POPT_AUTOHELP
      POPT_TABLEEND
    };

/* static function prototypes */
static STATUS gip_findAuxLoc( instance * );

/*
** Name: rpmqry.c	Module for quering RPM database and package info.
**
** Desription:
**	Functions used by GUI installer on Linux to query RPM package info
**	and the local RPM DB via the RPM C API.
**
** History:
**	02-Oct-2006 (hanje04)
**	    SIR 116877
**	    created.
**	25-Oct-2006 (hanje04)
**	    SIR 116877
**	    If we detect a non-renamed instance. Bump the default instance ID
**	    to prevent renaming RPMs to embed II
**	06-Nov-2006 (hanje04)
**	    SIR 116877
**	    Make sure code only gets built on platforms that can build it.
**	13-Nov-2006 (hanje04)
**	    SIR 116877
**	    Don't return an error from find_and_compare_existing_instance() if
**	    no instances are found.
**	17-Nov-2006 (hanje04)
**	    BUG 117149
**	    SD 111992
**	    Make sure inst_state and ug_mode are initialized properly
**	    if no existing instances are found.
**	21-Nov-2006 (hanje04)
**	    Correct typo in find_and_compare_existing_instance()
**	22-Nov-2006 (hanje04)
**	    BUG 117179
**	    SD 111975
**	    Use LO functions instead of dirent structure to check 
**	    file types when seaching for packages to install.
**    28-Nov-2006 (hanje04)
**	    Remove "realloc()" from code as it was causing problems when memory
**	    areas were moved under the covers. Shouldn't have been using it
**	    anyway. This means also altering the way existing_instances is 
**	    accessed.
**    02-Feb-2007 (hanje04)
**	    SIR 116911
**	    Replace PATH_MAX with MAX_LOC.
**    20-Apr-2007 (hanje04)
**	    BUG 118090
**	    SD 116383
**	    Add new function gip_findAuxLoc() to query database and log file
**	    locations of all existing instances. If they differ from II_SYSTEM
**	    we now store them in the updated instance structure so they can
**	    be removed should the user select the remove ALL files option.
**	    Previously, only those under II_SYSTEM were removed.
**	    Also clean up 
**	11-May-2007 (hanje04)
**	    SIR 118090
**	    In gip_findAuxLoc() make sure values returned by PMget() are
**	    checked before trying to use them.
**	23-Jul-2008 (hweho01)
**	    Change package name to ingres for 9.2 release.
**      10-Oct-2008 (hanje04)
**          Bug 120984
**          SD 131178
**          Update use of CHECK_PKGNAME_VALID() in find_files_to_install()
**	    after macro was changed.
**	26-Jul-2009 (hanje04)
**	    Bug 122395
**	    Use a more portable method for reported RPM errors.
*/

STATUS
query_rpm_package_info( int argc,
			char *argv[],
			UGMODE *state,
			i4 *count )
{
    Header		file_hdr;
    poptContext		context; /* libpopt context, used by rpmcli */
    rpmRC		rpmrc; /* RPM return code */

    /* initialize rpm CLI */
    context = rpmcliInit(argc, argv, optTbl);

    if ( context == NULL )
    {
	TRdisplay("Error: Could not initialize RPM CLI\nAborting...");
	exit(FAIL);
    }

    /* query the packages in the save set */
    rpmrc = init_saveset_info( &file_hdr, "rpm" );

    if ( rpmrc != RPMRC_OK )
    {
	TRdisplay( "ERROR: Failed to initialize save set\n" );
	context = rpmcliFini(context);
	return(FAIL);
    }

    /* search for existing instances and set inst_state appropriately */
    rpmrc=find_and_compare_existing_instance(file_hdr,
						state, 
  						count,
						NULL);
    if ( rpmrc != RPMRC_OK )
    {
	TRdisplay( "ERROR: Error occured searching for Ingres instances\n" );
	context = rpmcliFini(context);
	return(1);
    }

    /* free RPM context becaue we're done querying for now */
    context = rpmcliFini(context);

    return( OK );
}

rpmRC
find_and_compare_existing_instance( Header file_hdr, 
					UGMODE *state, 
					i4 	*count,
					gchar *instance_loc )
{
    Header		inst_hdr; /* RPM header for installed packages */
    rpmts		ts; /* RPM transaction set */
    rpmdbMatchIterator	iter; /* iterator for RPM patern matching */
    char		*ingname, *ingvers, *ingrel;  /* ingres version info */
    const char		**ingname_ptr=(const char **)&ingname;
    const char		**ingvers_ptr=(const char **)&ingvers;
    const char		**ingrel_ptr=(const char **)&ingrel;
    int			numdirs=0;
    int			i; /* misc counters */
    int			inst_count=0;

    DBG_PRINT( "Searching for instances...\n");
	
    /* initialize RPM */
    ts=rpmtsCreate();
	
    for ( i=0 ; ing_pkg_name_pattern[i] != NULL ; i++ )
    {
	/* initialize iterator */
	iter=rpmtsInitIterator( ts, RPMTAG_NAME, NULL, 0);

	/* NEED BETTER ERROR HANDLING HERE */
	if ( iter == NULL )
	{
	    TRdisplay( "Failed to initialize trasaction set iterator\n" );
	    return(RPMRC_FAIL);
	}
	
	DBG_PRINT( "Seaching with this pattern: %s\n", ing_pkg_name_pattern[i] );
	if ( rpmdbSetIteratorRE( iter, RPMTAG_NAME, RPMMIRE_DEFAULT,
				    ing_pkg_name_pattern[i] ) )
	{
	    TRdisplay("Failed to set instance search masks\n");
	    return(RPMRC_FAIL);
	}

	/* query database */
	while ( ( inst_hdr=rpmdbNextIterator( iter ) ) != NULL )
	{
	    if ( !headerNVR( inst_hdr, ingname_ptr, ingvers_ptr, ingrel_ptr ) )
	    {
    		char	**dirslist; 
		char	*dirslist_ptr;
		char 	tmpbuf[MAX_LOC];
		char 	inst_id[3];
		int 	inst_idx;
		Header	pkgs_hdr;
		rpmdbMatchIterator	sub_iter;
		int		pkg_idx=1;
		u_i2	memtag;
		STATUS	mestat;

		/* 
		** found an instance so bump the count, turn on upgrade
		** and allocate space to store its info_ptr
		*/
		inst_count++;
		inst_idx=inst_count - 1;
		*state|=UM_TRUE;
	
		/*
		** Get tag for each instance, request the memory
		** and store the tag
		*/
		memtag = MEgettag();
		existing_instances[inst_idx] = (instance *)MEreqmem( memtag, 
					   	sizeof(instance),
						TRUE,
						&mestat );
		existing_instances[inst_idx]->memtag = memtag ;
		
  	
		/* return error if we failed to get memory */
		if ( mestat != OK )
		    return( RPMRC_FAIL );

		/* initialize package info */
		if ( selected_instance == NULL )
		    selected_instance = *existing_instances ;

		existing_instances[ inst_idx ]->installed_pkgs=PKG_CORE;
		
		/* compare version with rpm file and inst_state accordingly */
		if ( inst_count > 1 )
		{
		    *state|=UM_MULTI;
		}
			
		switch( rpmVersionCompare( file_hdr, inst_hdr ) )
		{
		    case INST_RPM_SAME:
			*state|=UM_MOD;
			existing_instances[ inst_idx ]->action = UM_MOD;
			/* check if it's a renamed package */
			break;
		    case INST_RPM_OLDER:
			*state|=UM_UPG;
			existing_instances[ inst_idx ]->action = UM_UPG;
			break;
		}					
		DBG_PRINT("Found %d\n", inst_count );
		/* NEED GOOD ERROR HANDLING HERE */
		/* Currently no way to deal with missing or corrupt instances */
		/* get the instance ID from the II_SYSTEM */
		numdirs=get_pkg_dir_info( inst_hdr, &dirslist_ptr );
		dirslist=(char **)dirslist_ptr;				
		if ( numdirs > 0 )
		    get_instid( dirslist[0], inst_id );
		else
		    TRdisplay( "could not determine directory info\n\n" );
		
		/* store the package basename info */
		STcopy( ingname, existing_instances[ inst_idx ]->pkg_basename );

		if ( is_renamed( ingname ) )
		/* it's been renamed, mark it as such */
		    existing_instances[ inst_idx ]->action |= UM_RENAME;
		else
		{
		     /* if not, new install will need to rename */
		    *state |= UM_RENAME;

		    /*
		    ** also bump the default inst ID to I1. We never want to
		    ** rename to embed II
		    */
		    dfltID[1] = '1';
		}

		DBG_PRINT( "package %s %s been renamed\n", ingname,
			existing_instances[ inst_idx ]->action & UM_RENAME ?
			"has" : "has not" );

		/* search for the rest of the packages */
		while ( packages[pkg_idx] != NULL )
		{
    		    char	*pkgname, *pkgvers, *pkgrel;  /* pkg info */
    		    const char	**pkgname_ptr=(const char **)&pkgname;
    		    const char	**pkgvers_ptr=(const char **)&pkgvers;
    		    const char	**pkgrel_ptr=(const char **)&pkgrel;
		    char	search_string[20];
		    char	namebuf[20];
		    char	*sfx_ptr=namebuf;

		    /* copy ingname locally so we can play with it */
		    STcopy(ingname, namebuf);

		    /*
		    ** if the package name has the old format, skip the first
		    ** skip the 'ca-'
		    */
		    if ( STstrindex( namebuf, "ca-ingres", 0, FALSE ) )
		        sfx_ptr=sfx_ptr + 3;
				
		    /*
		    ** look for '-' separator in package name if it's there we
		    ** have a re-named set of packages to search for.
		    */
		    while ( *sfx_ptr != EOS )
		    {	
			if ( *sfx_ptr == '-' )
			{
			    /*
			    ** found the separator so split the string and
			    ** point to the suffix
			    */
			    *sfx_ptr = EOS ;
			    sfx_ptr++;
			    break;
			}
			sfx_ptr++;
		    }

		    /* set the package search string  */
		    if ( *sfx_ptr == EOS )
	       		STprintf( search_string, "%s-%s",
				namebuf,
				packages[pkg_idx]->pkg_name );
		    else
			STprintf( search_string, "%s-%s-%s",
				namebuf,
				packages[pkg_idx]->pkg_name,
				sfx_ptr );
		 
		    DBG_PRINT( "Searching for rest of packages with %s\n",
							search_string );
		    sub_iter=rpmtsInitIterator( ts, RPMTAG_NAME, 
						search_string, 0);
		
		    while ( ( pkgs_hdr=rpmdbNextIterator( sub_iter ) ) != NULL )
		    {
			if ( !headerNVR( pkgs_hdr, 
					pkgname_ptr,
					pkgvers_ptr,
					pkgrel_ptr ) )
			{
			    DBG_PRINT( "%s is also installed\n", pkgname );
			    existing_instances[ inst_idx ]->installed_pkgs |= 
							packages[pkg_idx]->bit;
			}
		    }
		    sub_iter = rpmdbFreeIterator(sub_iter);
		    /* cue up the next package */
		    pkg_idx++;
		}
			
		/* load info into instance_info */
		STprintf( existing_instances[ inst_idx ]->instance_name,
				"Ingres %s", inst_id );
		existing_instances[ inst_idx ]->instance_ID =
				existing_instances[ inst_idx ]->instance_name +
				( STlen(
				    existing_instances[ inst_idx ]->instance_name
				    ) - 2 );
		STprintf( existing_instances[ inst_idx ]->version, "%s-%s", 
				ingvers, ingrel );
		STcopy(  dirslist[0], existing_instances[ inst_idx ]->inst_loc);
		DBG_PRINT( "Instance Name: %s\nVersion: %s\nII_SYSTEM: %s\n",
				existing_instances[ inst_idx ]->instance_name,
				existing_instances[ inst_idx ]->version,
				existing_instances[ inst_idx ]->inst_loc );
			
		/*
		** Check for log and database locations that aren't
		** under II_SYSTEM and store them too.
		*/
		if ( gip_findAuxLoc( existing_instances[ inst_idx ] ) != OK )
# define LOCATE_AUXLOC_FAIL "Failed to locate database and log locations for %s instance\n"
		    TRdisplay( LOCATE_AUXLOC_FAIL,
				existing_instances[ inst_idx ]->instance_name );
		if ( existing_instances[ inst_idx ]->datalocs == NULL )
		    DBG_PRINT( "datalocs is NULL, no aux locations found\n" );

		
		/* free the array containing the results */
		headerFreeData( dirslist_ptr, RPM_STRING_ARRAY_TYPE ); 
	    }
	    else
	    {
		TRdisplay( "Failed to read header info\n" );
		return(RPMRC_FAIL);
	    }
	}
	/* free up iterator */
    	iter = rpmdbFreeIterator(iter);
    }
    
    /* done, free up transaction set */
    ts = rpmtsFree(ts);
	
    *count=inst_count;

    if ( *count == 0 )
    {
	/*
	** No existing installations found so force
	** new installation mode
	*/
	inst_state |= UM_INST;
	ug_mode |= UM_INST;
    }

    return( RPMRC_OK );

}

int
get_pkg_dir_info( Header header, char **pkgdirs )
{
    void	*pointer;
    int		type;
    int		set_size=0;

    /* query directory info for given package header */
    if ( headerGetEntry( header, RPMTAG_INSTPREFIXES, &type, &pointer, &set_size ) )
    {
	if ( type == RPM_STRING_ARRAY_TYPE )
	{
	    *pkgdirs=pointer;
	    return set_size;
	}
    }
    else
	return 0;
}

int
get_pkg_arch_info( Header header, char **arch )
{
    void	*pointer;
    int		type;
    int		set_size=0;

    /* query directory info for given package header */
    if ( headerGetEntry( header, RPMTAG_ARCH, &type, &pointer, &set_size ) )
    {
	if ( type == RPM_STRING_TYPE )
	{
	    *arch=pointer;
	    return( set_size );
	}
    }
    else
	return( -1 );
}

int
get_instid( char *inst_loc, char *inst_id )
{
    char	tbl_loc[MAX_LOC];
    char	readbuf[16];
    FILE	*symbol_tbl;
    

    /* get table location */
    STprintf( tbl_loc, "%s\%s", inst_loc, SYMBOL_TABLE );

    /* open symbol table */
    symbol_tbl=fopen( tbl_loc, "r" );
    if ( symbol_tbl == NULL )
    {
	    perror("Failed to open symbol table ");
	    return(0);
    }
	

    while ( fgets( readbuf, 16, symbol_tbl ) != NULL )
    {
	if ( strcmp( "II_INSTALLATION", readbuf ) == 0 )
	{
	   char *ptr;

	    /* found II_INSTALLATION so read again to get the value */
		fgets( readbuf, 5, symbol_tbl ) ;

		ptr=readbuf;
	   /* trim the white space */
	   while ( isblank((int)*ptr) )
		ptr++ ;
	   
	   /* copy to output and add the EOS */
	   inst_id[0]=*ptr++;
	   inst_id[1]=*ptr;
	   inst_id[2]=EOS;
	   break;
	}
    }

    /* close the file */
    fclose( symbol_tbl );
  
    /* return OK */
    return(0);
    
}

rpmRC
get_new_pkg_hdr( Header *file_hdr)
{
    FD_t	pkg_fd;
    rpmRC	rc;
    rpmts	ts;
    char    pathbuf[MAX_LOC + 256];

    /* check everythings been initialized properly */
    if ( *corefname == '\0' )
	return( RPMRC_FAIL );

    /* Construct the full path to the core  RPM package */
    STprintf( pathbuf, "%s/%s/%s",
		new_pkgs_info.file_loc,
		new_pkgs_info.format,
		corefname );
    pkg_fd=Fopen( pathbuf, RPM_OPEN_MODE );

    if ( pkg_fd == NULL || Ferror(pkg_fd) )
    {
	rpmlog( RPMLOG_ERR, ("Failed to open %s\n%s\n"),
	    pathbuf, Fstrerror(pkg_fd) );
	if ( pkg_fd )
	    Fclose(pkg_fd);

	return(RPMRC_FAIL);
    }

    /* create transaction set for querying file */
    ts=rpmtsCreate();

    /* read the header */
    rc = rpmReadPackageFile( ts, pkg_fd, pathbuf, file_hdr );

    /* close the file and return */
    ts=rpmtsFree(ts);

    Fclose(pkg_fd);
    return(RPMRC_OK);
    
}
    
rpmRC
load_saveset_from_rpm_hdr ( Header rpmheader, saveset *target_info )
{
    char	*pkgname, *pkgvers, *pkgrel, *pkgarch;  /* pkg info */
    const char	**pkgname_ptr=(const char **)&pkgname;
    const char	**pkgvers_ptr=(const char **)&pkgvers;
    const char	**pkgrel_ptr=(const char **)&pkgrel;
	
    /* as we've called this function it's an RPM saveset */
    STcopy( "rpm", target_info->format );

    /* get name, version and release from header */	
    headerNVR( rpmheader, pkgname_ptr, pkgvers_ptr, pkgrel_ptr );
    STcopy( pkgname, target_info->pkg_basename );
    STprintf( target_info->version, "%s-%s", pkgvers, pkgrel );

    /* get architecture RPM header */
    get_pkg_arch_info( rpmheader, &pkgarch );

    if ( get_pkg_arch_info( rpmheader, &pkgarch ) < 0 )
	return( RPMRC_FAIL );

    STcopy( pkgarch, target_info->arch );

    return( RPMRC_OK );
}

rpmRC
init_saveset_info( Header *core_fhdr, const char *pkgformat )
{
    rpmRC	rpmrc;

    /* Sanity check arguments */
    if ( pkgformat == NULL )
	return( RPMRC_FAIL );

    /* store initial values */
    STcopy( PKG_BASENAME, new_pkgs_info.pkg_basename );
    STcopy( pkgformat, new_pkgs_info.format );
    new_pkgs_info.pkgs = PKG_NONE;

    /* find packages in saveset */
    rpmrc = find_files_to_install();
    if ( rpmrc != RPMRC_OK )
    {
	TRdisplay( "ERROR: Failed to find packages to install\n" );
	return( rpmrc );
    }

    /* Read header info from RPM files */
    rpmrc=get_new_pkg_hdr( core_fhdr );
    if ( rpmrc != RPMRC_OK )
    {
	TRdisplay( "ERROR: Failed to read RPM file headers\n" );
	return( rpmrc );
    }

    rpmrc = load_saveset_from_rpm_hdr( *core_fhdr, &new_pkgs_info );
    if ( rpmrc != RPMRC_OK )
    {
	TRdisplay( "ERROR: Failed to load saveset info from RPM header\n" );
	return( rpmrc );
    }
 
    return( RPMRC_OK );
  
}

rpmRC
find_files_to_install( void )
{
    gchar		pkgdir[MAX_LOC];
    rpmRC		rpmrc;
    LOCATION		pkgloc, entloc;
    i2			flag;
    STATUS		lorc;

    /* construct package file location */
    STprintf( pkgdir, "%s/%s",
		new_pkgs_info.file_loc,
		new_pkgs_info.format );

    /* convert to LO format */ lorc = LOfroms( PATH, pkgdir, &pkgloc );

    if ( lorc != OK )
	return( RPMRC_FAIL );

    /* scroll through the files with a "new_pkgs_info.format" suffix */
    while( LOlist( &pkgloc, &entloc ) == OK )
    {
	char *sufptr;
 	char *bnameptr;

	/* check what we've found */
	if ( LOisdir( &entloc, &flag ) != OK )
	{
	    char	*pkgptr;
	    char	pkgbuf[MAX_FNAME];
		
	    /* copy the file name locally to stop it being trashed */
	    LOtos( &entloc, &pkgptr );
	    STlcopy( pkgptr, pkgbuf, MAX_FNAME );

	    /* locate the suffix */
	    sufptr = STrindex( pkgbuf, ".", 0 ); /* suffix */
	    if ( sufptr == NULL )
		continue; /* no '.', skip file */

	    sufptr++;

	    if ( CHECK_PKGNAME_VALID( pkgbuf, sufptr ) == OK ) /* valid pkg ? */
	    {
		/* add package to list */
		rpmrc = add_pkg_to_pkglst( pkgbuf, &new_pkgs_info.pkgs );
		if ( rpmrc != RPMRC_OK )
		    break;
	    }
	}
    }
    return( rpmrc );
}

rpmRC
add_pkg_to_pkglst( char *filename, 
		   PKGLST *pkglst )
{
    char	*pkgname_ptr;
    gboolean	foundpkg = FALSE;
    int		i = 1;
    /* sanity check */
    if ( filename == NULL || pkglst == NULL )
	return( RPMRC_FAIL );

    /* 
    ** Ingres package filenames (except core) are of the following format:
    **
    **	pkg_basename-pkgname-version-release.arch.format
    **
    ** Determine "pkgname" and compare it with the list of known packages
    */
    /* find pkgname */
    pkgname_ptr = STindex( filename, "-", 0 );
    pkgname_ptr++;

    /* scroll through known packages */
    while( packages[i] != NULL )
    {
	if ( strncmp( pkgname_ptr,
		packages[i]->pkg_name,
		STlen( packages[i]->pkg_name ) ) == 0 )
	{
	    /* found known package, add it to list */
	    foundpkg = TRUE;
	    *pkglst |= packages[i]->bit;
	    break;
	}
	i++;
    }

    if ( foundpkg == FALSE )
    {
	i = 0;
	/* scroll through license packages */
	while( licpkgs[i] != NULL )
	{
	    if ( strncmp( pkgname_ptr,
		licpkgs[i]->pkg_name,
		STlen( licpkgs[i]->pkg_name ) ) == 0 )
	    {
		/* found known package, add it to list */
		foundpkg = TRUE;
		*pkglst |= licpkgs[i]->bit;
		break;
	    }
	    i++;
	}
    }

    /* if it didn't match anything, check for core package */
    if ( foundpkg == FALSE )
    {
	char	tmpbuf[256];
	char	*bnptr;

	/* copy filename locally as we need to mess with it */
	STlcopy( filename, tmpbuf, 256 );

	/*
	** Ingres core package name is of the following format:
	**
	**	pkg_basename-version-release.arch.format
	*/
	/* Determine what should be the base name */
	i = 0;
	while ( i < 2 )
	{
	    bnptr = STrindex( tmpbuf, "-", 0 );
	    *bnptr = '\0';
	    i++;
 	}

	/* if it's the core package tmpbuf should now contain pkg_basename */
	if ( STcompare( tmpbuf, new_pkgs_info.pkg_basename ) == 0 )
	{
	    /* it's the core package */
	    new_pkgs_info.pkgs |= PKG_CORE;
	    STcopy( filename, corefname );
	}
	else
	{
	    TRdisplay( "ERROR: Invalid package:\n%s\n", filename );
	    return( RPMRC_FAIL );
	}
    }	
   return( RPMRC_OK );
}

/*
** Name: gip_findAuxLoc()
**
** Description:
**	Probes config.dat and symbol.tbl for a given instance to determine
**	all database and log file locations that are NOT under II_SYSTEM.
**
** Inputs:
**	*inst -	Pointer to instance structure for existing
**		instance.
**
** Outputs:
**	*inst - As above.
**
** Returns:
**	OK	No errors orrured.
**	status	From failing CL call.	
**
** History:
**	20-Apr-2007 (hanje04)
**	    Created.
**	11-May-2007 (hanje04)
**	    SIR 118090
**	    Make sure values returned by PMget() are checked before trying to
**	    use them.
**	04-Jan-2008 (hanje04)
**	    BUG 119700
**	    Need to set II_SYSTEM before calling PMget() not just II_CONFIG.
*/
static STATUS
gip_findAuxLoc( instance *inst )
{
    char	pathbuf[MAX_LOC];
    char	readbuf[MAX_LOC];
    LOCATION	symbloc;
    FILE	*symbol_tbl;
    auxloc 	**locptr=NULL;
    STATUS	status;
    i4		i;
    SIZE_TYPE	inst_loc_len = STlen(inst->inst_loc) ;
    

    /* get table location */
    STprintf( pathbuf, "%s\%s", inst->inst_loc, SYMBOL_TABLE );
    status = LOfroms( PATH & FILENAME, pathbuf, &symbloc );

    if ( status != OK )
	return( status );

    /* open symbol table */
    status = SIopen( &symbloc, "r", &symbol_tbl );
    if ( status != OK  )
    {
	    DBG_PRINT("Failed to open symbol table ");
	    return(status);
    }
	
    /* initialize counters */
    inst->numdlocs = inst->numllocs = 0;

    /* check for database locations */
    locptr = &inst->datalocs;
    for ( i = 1 ; dblocations[i] != NULL ; i++ )
    {
	SIZE_TYPE	readlen = STlen( dblocations[i]->symbol ) + 1;

	/* reset file pointer before we start */
	status = SIfseek( symbol_tbl, (OFFSET_TYPE)0, 0);

	while ( SIgetrec( readbuf, readlen, symbol_tbl ) == OK )
	{
	    if ( STcompare( dblocations[i]->symbol, readbuf ) == 0 )
	    {
		char *ptr;

		/*
		** found location so up the count and read again
		** to get the value
		*/
		DBG_PRINT( "Found %s location, checking...\n",
			dblocations[i]->symbol );
		SIgetrec( readbuf, MAX_LOC, symbol_tbl ) ;

		/* trim the white space. trailing... */
		STtrmwhite( readbuf );

		/* ...and leading */
		ptr=readbuf;
		while ( *ptr != '/' )
		    ptr++ ;
	   
		/* if the location differs from II_SYSTEM, add it to the list */
		if ( STcompare( ptr, inst->inst_loc ) != 0 )
		{
		    inst->numdlocs++;

		    /* find end of location list */
		    while ( *locptr != NULL )
		        *locptr = (*locptr)->next_loc ;

		    /* allocate memory for location */
		    *locptr = (auxloc *)MEreqmem( inst->memtag,
				    sizeof( auxloc ),
				    TRUE,
				    &status );
		    /* fill values */
		    (*locptr)->idx = i ;
		    STcopy( ptr, (*locptr)->loc ) ;

		    DBG_PRINT( "%s added as %s location for %s instance\n",
				(*locptr)->loc,
				dblocations[i]->name,
				inst->instance_name );
		}

		break;
	    }
	} /* while reading symbol table */
    } /* dblocations */

    /* get log loctations */
    locptr = &inst->loglocs;

    /* set II_SYSTEM so we can query config.dat */
    gip_setIISystem( inst->inst_loc );
    DBG_PRINT( "II_CONFIG set to %s\n", pathbuf );

    if ( PMinit() != OK )
	DBG_PRINT("Error returned by PMinit()\n" );

    if ( PMload( NULL, (PM_ERR_FUNC *) NULL ) != OK )
	TRdisplay( "Failed to load config data, cannot get log locations\n" );
    else
    {
	char	*pmval = NULL, *host;	
	char	tmp_buf[256];
	i4	logparts = 0;
	i4	j = 1;
	
	host = PMhost();
	STprintf( tmp_buf, ERx( "%s.%s.rcp.log.log_file_parts" ),
		    SystemCfgPrefix,
		    host );
	PMget( tmp_buf, &pmval );
	logparts = pmval ? atoi( pmval ) : 0 ;

	while( j <= logparts )
	{
	    /* primary log */
	    STprintf( tmp_buf, ERx( "%s.%s.rcp.log.log_file_%d" ),
		    SystemCfgPrefix,
		    host,
		    j );
	    PMget( tmp_buf, &pmval );

	    if ( ! *pmval )
	    {
		TRdisplay( "Failed to determine primary log location for the %s instance\n", inst->instance_name );
		break;
	    }

	    if ( STcompare( pmval, inst->inst_loc ) != 0 )
	    {
		inst->numllocs++;

		if ( *locptr != NULL )
		    *locptr = (*locptr)->next_loc ;

		/* allocate memory for location */
		*locptr = (auxloc *)MEreqmem( inst->memtag,
			sizeof( auxloc ),
			TRUE,
			&status );

		/* fill values */
		(*locptr)->idx = 0 ;
		STcopy( pmval, (*locptr)->loc ) ;

		DBG_PRINT( "%s added as log location for %s instance\n",
			(*locptr)->loc,
			inst->instance_name );

		
		/* dual log if we have one */
		STprintf( tmp_buf, ERx( "%s.%s.rcp.log.dual_log_%d" ),
		    SystemCfgPrefix,
		    host,
		    j );
		PMget( tmp_buf, &pmval );

		if ( *pmval )
		{
		    if ( *locptr != NULL )
			*locptr = (*locptr)->next_loc ;

		    /* allocate memory for location */
		    *locptr = (auxloc *)MEreqmem( inst->memtag,
			    sizeof( auxloc ),
			    TRUE,
			    &status );
 
		    (*locptr)->idx = 1 ;
		    STcopy( pmval, (*locptr)->loc ) ;

		    DBG_PRINT( "%s added as dual log location for %s instance\n"
			, (*locptr)->loc,
			inst->instance_name );
		}

		
	    }
	    j++;
	} /* logparts */

	
    }
    
    /* cleanup */
    gip_unSetIISystem();
    /* release PM session  */
    PMfree();
    /* close the file */
    SIclose( symbol_tbl );

  
    /* return OK */
    return( OK );
}
# endif /* xCL_GTK_EXISTS & xCL_RPM_EXISTS */
