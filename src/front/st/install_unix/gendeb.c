/*
** Copyright (c) 2007 Ingres Corporation. All rights reserved.
*/
# include <compat.h>
# include <si.h>
# include <st.h>
# include <lo.h>
# include <ex.h>
# include <er.h>
# include <gc.h>
# include <cm.h>
# include <nm.h>
# include <gl.h>
# include <sl.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>
# include <ui.h>
# include <te.h>
# include <tr.h>
# include <gv.h>
# include <pc.h>
# include <iplist.h>
# include <erst.h>
# include <uigdata.h>
# include <stdprmpt.h>
# include <sys/stat.h>
# include <ip.h>
# include <ipcl.h>
# include "genpkg.h"
# include "genpkgdata.h"

/*
**  Name: gendeb.c
**
**  Purpose:
**	Generates control files for Ingres DEB packages. Contains only
**	one public function:
**
**	   gen_deb_cntrl()
**
**	which is the entry point for this module. 
** 
**  History:
**	21-Jun-2007 (hanje04)
**	    SIR 118420
**	    Created.
**      10-Mar-2008 (bonro01)
**          Include Spatial object package into standard ingbuild, RPM and DEB 
**          install images instead of having a separate file.
*/

# define DEFAULT_SCRIPT_PERMISSION      "s:re,o:rwed,g:re,w:re"

static char * gen_deb_dir_name( char *buff, char *directory, SPECS *specs );
static STATUS gen_deb_template( SPECS *specs );
static STATUS gen_deb_file_list( SPECS *specs, FILE *fileList );
static STATUS gen_deb_script( SPECS *specs, i4 script );
static STATUS gen_deb_set_script_perm( char *filename );
static STATUS gen_deb_pkg_files( SPECS *specs );

char	*pkgInstLoc = "/opt/Ingres/IngresII";
char	*debScrInterp = "/bin/bash";
static char	*debConfigScripts[] = { 
				# define PREINST 0
				"preinst", 
				# define POSTINST 1
				"postinst", 
				# define PRERM 2
				"prerm", 
				# define POSTRM 3
				"postrm" };

/*
** Name: gen_deb_cntrl()
**
** Purpose:
**	Entry point for DEB control files generation routines for
**	'buildrel' utility
**
** Inputs:
**	None directly but requires debconfig file to be present under
**	$II_MANIFEST_DIR. See front!st!specials_unix_vms debconfig.ccpp
**	for details.
**
** Returns:
**	OK on success
**	status value on failure
**
** History:
**	21-Jun-2007 (hanje04)
**	    Created.
*/
STATUS
gen_deb_cntrl()
{
    STATUS	status;
    char	buff[MAX_LOC + 1];
    char	*cp = NULL;
    SPECS	*specs;

    /*
    ** locate config file, buildrel has already checked
    ** II_MANIFEST_DIR is set.
    */
    NMgtAt( "II_MANIFEST_DIR", &cp );
    STprintf( buff, "%s/debconfig", cp );

     /* defaults */
    if (!gen_pkg_subs_set( "%rpm_basename%", "Ingres" ) ||
	 !gen_pkg_subs_set( "%rpm_version%", "1.0.0000-00" ) ||
	 !gen_pkg_subs_set( "%rpm_prefix%", pkgInstLoc ) ||
	 !gen_pkg_subs_set( "%rpm_buildroot%", pkgBuildroot ) )
        return(FAIL);

    /* read the configuration file */
    status = gen_pkg_config_read(buff);
    if (status != OK)
        return(status);

    /* release is optional */
    subsRelease = gen_pkg_subs_get( "%rpm_release%" );

    /* check for basename */
    if ( NULL == ( subsBaseName = gen_pkg_subs_get( "%rpm_basename%" ) ) )
    {
        SIfprintf( stderr,
	    "rpm_basename not declared in spec generation config file\n" );
	return(FAIL);
    }

    /* check for version */
    if ( NULL == ( subsVersion = gen_pkg_subs_get( "%rpm_version%" ) ) )
    {
	SIfprintf(stderr,
	    "rpm_version not declared in spec configuration config file\n" );
        return(FAIL);
    }

    /* check for prefix */
    if (NULL == (subsPrefix = gen_pkg_subs_get(ERx("%rpm_prefix%")))) {
        SIfprintf(stderr, ERx("rpm_prefix not declared in spec configuration config file\n"));
        return(FAIL);
    }

    /* check for Doc prefix */ 
    if (NULL == (subsDocPrefix = gen_pkg_subs_get(ERx("%rpm_doc_prefix%")))) {        SIfprintf(stderr, ERx("rpm_doc_prefix not declared in spec configuration config file\n"));
        return(FAIL);
    }

    /* generate spec files from templates */
    for ( specs = specsFirst; specs; specs = specs->next )
    {
	/* create output directory */
	gen_deb_dir_name( buff, pkgBuildroot, specs );
	if ( gen_pkg_create_path( buff,
	     "Failed to create deb output directory:\n" ) != OK )
            return(FAIL);
	status = gen_deb_pkg_files( specs );
        if (status)
            return(status);
    }

    return(OK);


}

/*
** Name: gen_deb_dir_name()
**
** Purpose:
**	Generate directory name to be used to assemble a given DEB package.
**
** Inputs:
**	char *buff - buffer to contain directory path
**	char *directory - root dir to create new dir under
**	SPECS *specs - package info for dir path to be created
**
** Returns:
**	char *buff - newly created path name	
**
** History:
**	21-Jun-2007 (hanje04)
**	    Created.
*/
static char *
gen_deb_dir_name( char *buff, char *directory, SPECS *specs )
{
    STprintf(buff, "%s/%s-%s/%s/DEBIAN/",
	    directory,
	    subsBaseName->value,
	    subsVersion->value,
	    *specs->name ? specs->name : "core");

    return( buff );
}

/*
** Name: gen_deb_pkg_files()
**
** Purpose:
**	Write control file, file list and scripts for given package.
**	Control file is generated from package header:
**	    $ING_BUILD/files/debhdrs
**
**	File list is generated from the manifest files:
**	    $II_MANIFEST_DIR
**
**	Scripts are generated from the scripts templates:
**	    $ING_BUILD/utility
**	
**
** Inputs:
**	SPECS *specs - package info for package control files being created
**
** Returns:
**	OK on success
**	FAIL on failure
**
** History:
**	21-Jun-2007 (hanje04)
**	    Created.
*/
static STATUS
gen_deb_pkg_files( SPECS *specs )
{
    char buff[READ_BUF + 1];
    char *cp = NULL;
    FILE *filePkgHdr, *fileControl, *fileList;
    LOCATION loc;
    STATUS status;

    /* set the spec instance name */
    STprintf(buff, "%s%s", *specs->name ?
                "-" : "", *specs->name ?
                    specs->name : "");
    if (!gen_pkg_subs_set("%rpm_specname%", buff))
        return(FAIL);

    if ( *specs->name != EOS )
	if (!gen_pkg_subs_set("%pkg_name%", specs->name ))
            return(FAIL);

    /* ING_BUILD must be defined, buildrel.c depends on this */
    NMgtAt("ING_BUILD", &cp);

    /* open control output file */
    gen_deb_dir_name(buff, pkgBuildroot, specs);
    STcat( buff, "control" );
    LOfroms(PATH & FILENAME, buff, &loc);

    if (SIfopen(&loc, "w", SI_TXT, 0, &fileControl))
    {
        SIfprintf(stderr,
         "could not write '%s', correct config file or II_DEB_BUILDROOT\n",
         buff);
        return(FAIL);
    }

    /* Need separate control and filelist files for debs */
    /* open header file to generate control file */
    STprintf(buff, ERx("%s/%s/%s%s%s%s"),
                        cp,
            ERx("files/debhdrs"),
            "ingres",
            *specs->name != EOS ? ERx("-") : ERx(""),
            specs->name,
            ERx(".pkghdr"));
    LOfroms(PATH & FILENAME, buff, &loc);

    if (SIfopen(&loc, ERx("r"), SI_TXT, 0, &filePkgHdr))
    {
        SIfprintf(stderr,
         "template '%s' not found, correct config file or missing template\n",
         buff);
        SIclose(fileControl);
        return(FAIL);
    }

    while (1)
    {
        buff[READ_BUF + 1] = EOS;
        if (OK != SIgetrec(buff, sizeof(buff), filePkgHdr))
            break;

        /* trim white space */
        STtrmwhite(buff);

        /* perform substitutions */
        status = gen_pkg_strsubs(buff, sizeof(buff));
        if ( status == IP_SKIP_LINE )
            continue;
        else if (status) {
            SIclose(fileControl);
            SIclose(filePkgHdr);
            return(status);
        }

        /* write to control file */
        SIfprintf(fileControl, "%s\n", buff);
    }
    SIclose( filePkgHdr );
    SIflush( fileControl );

    /* next add package descriptions from the package hlp files */
    if ( gen_pkg_hlp_file( specs, fileControl, DEB_HLP_FMT ) != OK )
    {
	SIfprintf( stderr, 
		"Failed to write help info to Control file\n" );
	SIclose( fileControl );
	return( FAIL );
    }
    SIclose(fileControl);

    /* now generate the file list */
    buff[0] = '\0';
    gen_deb_dir_name(buff, pkgBuildroot, specs);
    STcat( buff, "filelist" );
    LOfroms(PATH & FILENAME, buff, &loc);

    if (SIfopen(&loc, "w", SI_TXT, 0, &fileList))
    {
        SIfprintf(stderr,
         "could not write '%s', correct config file or II_DEB_BUILDROOT\n",
         buff);
        SIclose(filePkgHdr);

        return(FAIL);
    }
  
    gen_deb_file_list ( specs, fileList );
    SIclose( fileList );

    /* now write the setup scripts */
    if ( OK != gen_deb_script( specs, PREINST ) )
	return( FAIL );
    if ( OK != gen_deb_script( specs, POSTINST ) )
	return( FAIL );
    if ( OK != gen_deb_script( specs, PRERM ) )
	return( FAIL );

    return( OK );
}

/*
** Name: gen_deb_file_list()
**
** Purpose:
**	Generate file list for given SPEC from manifest files.
**
** Inputs:
**	SPECS *specs - package info for package list file being created
**	FILE *fileList - open FILE structure for package file list file
**
** Returns:
**	OK on success
**	FAIL on failure
**
** History:
**	21-Jun-2007 (hanje04)
**	    Created.
*/
static STATUS
gen_deb_file_list( SPECS *specs, FILE *fileList )
{
    FEATURES *features;
    PKGBLK *package;
    PRTBLK *part;
    FILBLK *file;
    LOCATION loc;
    i4 packageFeatureLength;
    char buff[ READ_BUF + 1 ];
    char *filename;
    bool embcdone = FALSE;

    /* Scroll through package list and dump out the files for the SPEC */
    SCAN_LIST(distPkgList, package)
    {
        packageFeatureLength = STlength(package->feature);

        if( (!spatial) && ( STequal( package->feature, ERx("spatial") ) ||
                            STequal( package->feature, ERx("spatial64")) ) )
        {
                continue;
        }

        for (features = specs->featureFirst;
		 features; features = features->next)
	{
            if ( !STxcompare( package->feature, packageFeatureLength,
			 features->feature, packageFeatureLength, TRUE, FALSE) )
	    {
                SCAN_LIST(package->prtList, part)
		 {
                    if ( ! STcompare( part->name, "emb-c" ) )
                    {
                        if ( embcdone )
                           continue;

                        else
                           embcdone=TRUE;
                    }

                    SCAN_LIST(part->filList, file)
		    {
                        /* compose file name */
                        STcopy( file->build_dir, buff );
                        LOfroms( PATH, buff, &loc );
                        LOfstfile( file->build_file != NULL ?
				    file->build_file :
				    file->name, &loc );
                        LOtos( &loc, &filename );
                        /* location info is different for doc package */
                        if ( ! STcompare( part->name, "documentation" ) )
                        {
			    /*
			    ** as we're not using the first part of the dir name
			    ** in the doc location, make sure we preserve
			    ** everything after ingres/files/english
			    */
			    char namebuf[50]; /* filename and any needed dirs */
			    char *lastdir = STrindex( file->directory, "/", 0 );

			    /* do we need to append a dir to the filename */
			    if ( STcompare( ++lastdir, "english" ) )
				STprintf( namebuf, "%s/%s", 
					   lastdir, file->name );
			    else
				STcopy( file->name, namebuf );

			    SIfprintf(fileList,
					"%s/%s-%s/%s\n",
					subsDocPrefix->value,
					subsBaseName->value,
					subsVersion->value,
					namebuf);
                        }
                        else
                        {
                         SIfprintf(fileList,
                          "%s/%s/%s\n",
                          STcompare( specs->name, "documentation" ) ?
                                subsPrefix->value :
                                subsDocPrefix->value,
                          file->directory,
                          file->name);
                        }
                    }
                }
            }
        }
    }

    return(0);
}

/*
** Name: gen_deb_script()
**
** Purpose:
**	Generate DEB package scripts for given SPEC structure.
**
** Inputs:
**	SPECS *specs - package info for package list file being created
**	i4 script - which script to create (from debConfigScripts[] )
**
** Returns:
**	OK on success
**	FAIL on failure
**
** History:
**	21-Jun-2007 (hanje04)
**	    Created.
*/
static STATUS
gen_deb_script( SPECS *specs, i4 script )
{
    char fbuff[READ_BUF + 1];
    char subuf[50];
    char template[25];
    char *cp = NULL;
    i4   sulen;
    FILE *fileScript, *filePkgHdr;
    LOCATION loc;
    STATUS status;

    /* open template file */
    NMgtAt("ING_BUILD", &cp);

    /*
    ** different templates used for some of the packages work out what
    ** we need first before proceeding
    */
    switch ( script )
    {
	case PREINST:
	    if ( ! *specs->name )
		STprintf(fbuff,
			    "%s/files/debscripts/core_%s.template",
			    cp, debConfigScripts[script] );
	    else if ( ! STcompare( specs->name, "dbms" ) )
		STprintf(fbuff,
			    "%s/files/debscripts/%s_%s.template",
			    cp, specs->name, debConfigScripts[script] );
	    else
		STprintf(fbuff,
			    "%s/files/debscripts/%s.template",
			    cp, debConfigScripts[script] );
	    break;
	case POSTINST:
	    if ( ! *specs->name )
		STprintf(fbuff,
			    "%s/files/debscripts/core_%s.template",
			    cp, debConfigScripts[script] );
	    else if ( !  STcompare( specs->name, "net" ) ) 
		STprintf(fbuff,
			    "%s/files/debscripts/%s_%s.template",
			    cp, specs->name, debConfigScripts[script] );
	    else
		STprintf(fbuff,
			    "%s/files/debscripts/%s.template",
			    cp, debConfigScripts[script] );
	    break;
	case PRERM:
	    if ( ! *specs->name )
		STprintf(fbuff,
			    "%s/files/debscripts/core_%s.template",
			    cp, debConfigScripts[script] );
	    else
		STprintf(fbuff,
			    "%s/files/debscripts/%s.template",
			    cp, debConfigScripts[script] );
	    break;
	case POSTRM:
	    STprintf(fbuff,
			"%s/files/debscripts/%s.template",
			cp, debConfigScripts[script] );
	    break;
	default:
	    /* invalid script */
	    return( FAIL );
		
    }

    LOfroms(PATH & FILENAME, fbuff, &loc);

    if (SIfopen(&loc, "r", SI_TXT, 0, &filePkgHdr))
    {
        SIfprintf(stderr,
         "template '%s' not found, correct config file or missing template\n",
         fbuff);
        return(FAIL);
    }

    /* open output script */
    gen_deb_dir_name(fbuff, pkgBuildroot, specs);
    STcat( fbuff, debConfigScripts[script] );
    LOfroms(PATH & FILENAME, fbuff, &loc);

    if (SIfopen(&loc, "w", SI_TXT, 0, &fileScript))
    {
        SIfprintf(stderr,
         "could not write '%s', correct config file or II_DEB_BUILDROOT\n",
         fbuff);
	SIclose( filePkgHdr );
        return(FAIL);
    }

    /* first line of script must be the interpreter */
    SIfprintf( fileScript, "#!%s\n", debScrInterp );
    
    /* define the setup scripts for this package */
    subuf[0] = EOS;
    sulen = gen_pkg_su_files( specs, subuf, sizeof( subuf ) );
    if ( sulen >= 0 )
	gen_pkg_subs_set( "%pkg_sulist%", subuf );
    else
    {
	SIfprintf( stderr, "Failed to set setup script list for %s package\n",
		    *specs->name ? specs->name : "core" );
	return( FAIL );
    }

    while (1)
    {
        char buff[READ_BUF + 1] = "";
	/*
	** Scroll through template line by line,
	** replace the tokens and write out to the script
	*/
        if (OK != SIgetrec(buff, sizeof(buff), filePkgHdr))
            break;

        /* trim white space */
        STtrmwhite(buff);

        /* perform substitutions */
        status = gen_pkg_strsubs(buff, sizeof(buff));
        if ( status == IP_SKIP_LINE )
            continue;
        else if (status) {
            SIclose(fileScript);
            SIclose(filePkgHdr);
            return(status);
        }

        /* write to script */
        SIfprintf(fileScript, "%s\n", buff);
    }

    SIclose(filePkgHdr);
    SIclose(fileScript);

    /* set permissions */
    if ( OK != gen_deb_set_script_perm( fbuff ) )
	return( FAIL );

    return( OK );
}

/*
** Name: gen_deb_script_perm()
**
** Purpose:
**	Set correct permissions on DEB package scripts
**
** Inputs:
**	char	*filename - path to script to set permissions on.
**
** Returns:
**	OK on success
**	FAIL on failure
**
** History:
**	21-Jun-2007 (hanje04)
**	    Created.
*/
static STATUS
gen_deb_set_script_perm( char *filename )
{
    char	cmdbuf[ 20 + MAX_LOC ];

    /* sanity checking */
    if ( filename == NULL || *filename == EOS || 
	STlen( filename ) > ( 20 + MAX_LOC ) )
	return( FAIL );

    /* construct command line */
    STprintf( cmdbuf, "chmod %o %s",
		IPCLbuildPermission( DEFAULT_SCRIPT_PERMISSION, NULL ),
		filename ); 

    return( ip_cmdline( cmdbuf, E_ST0205_chmod_failed ) );
}

