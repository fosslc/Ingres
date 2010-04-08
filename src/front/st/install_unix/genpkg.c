/*
** Copyright (c) 2004, 2010 Ingres Corporation. All rights reserved.
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
# include <gv.h>
# include <iplist.h>
# include <erst.h>
# include <uigdata.h>
# include <stdprmpt.h>
# include <sys/stat.h>
# include <ip.h>
# include <ipcl.h>
# include "genpkg.h"

/*
**  Name: genpkg.c
**  Purpose:
**	 Parses package config files to populate structures for generating
**	 RPM SPEC files or DEB Control files.
**
**  NOTE:
**	Many of the functions contained in the module were originally
**	created for the creation and manipulation of RPM SPEC files.
**	As such many of the structures and funtions refer to SPEC
**	files where as they are now dealing with the generation of
**	both RPM SPEC and DEB Control files.
**
**	The SPECS structure represent the package being created and
**	the FEATURES structure represents the ingres components 
**	contained in that package.
** 
**  History:
**	21-Jun-2007 (hanje04)
**	    SIR 118420
**	    Created from genrpm.c
**	03-Feb-2010 (hweho01)
**	    Fix RPM installation error "iisuspat:command not found" from  
**	    the distributions that contain no spatial package, only list      
**	    iisuspat script and description in DBMS spec if option for   
**	    spatail is specified.     
**	19-Feb-2010 (hanje04)
**	    SIR 123296
**	    Add support for LSB builds
*/


/**** Data definitions ****/
/**** spec and substitution list maintenance ****/
GLOBALDEF SPECS *specsFirst = 0;
GLOBALDEF SPECS *specsLast = 0;
GLOBALDEF SUBS *subsFirst = 0;
GLOBALDEF SUBS *subsLast = 0;

/**** required config file substitutions ****/
GLOBALDEF SUBS *subsBaseName;
GLOBALDEF SUBS *subsVersion;
GLOBALDEF SUBS *subsRelease;
GLOBALDEF SUBS *subsPrefix;
GLOBALDEF SUBS *subsEmbPrefix;
GLOBALDEF SUBS *subsMDBPrefix;
GLOBALDEF SUBS *subsMDBVersion;
GLOBALDEF SUBS *subsMDBRelease;
GLOBALDEF SUBS *subsDocPrefix;

/* Globalrefs */
GLOBALREF char *license_type;
GLOBALREF LIST distPkgList;
GLOBALREF bool spatial;


/*
** Name: gen_pkg_specs_add()
**
** Purpose:
**	Add a package file definition.
**
** Inputs:
**	char *name(in) - unique part of the spec file name
**
** Returns:
**  SPECS* of the newly created definition
**  NULL if creation failed
**
*/
SPECS*
gen_pkg_specs_add(char *name)
{
    SIZE_TYPE nameLength = STlength(name);
    SPECS *specs = (SPECS *) MEreqmem(0, sizeof(SPECS) + nameLength + 1, TRUE, NULL);
    if (specs == NULL) {
        SIfprintf(stderr, ERx("spec file segment allocation failed\n"));
        return(NULL);
    }

    specs->name = &((char *)specs)[sizeof(SPECS)];
    STmove(name, EOS, nameLength + 1, specs->name);

    if (specsLast) {
        specsLast->next = specs;
    } else {
        specsFirst = specs;
    }
    specsLast = specs;

    return specs;
}

/*
** Name: gen_pkg_specs_feature_add()
**
** Purpose:
**	Add a feature to a package file definition.
**
** Inputs:
**	SPECS *specs(in) - spec definition to add feature to
**	char *feature(in) - feature to add
**
** Returns:
**	FEATURES* of the newly added feature definition
**	NULL if creation failed
**
*/
FEATURES*
gen_pkg_specs_features_add(SPECS *specs, char *feature)
{
    SIZE_TYPE featureLength = STlength(feature);
    FEATURES *nfeature = (FEATURES *)MEreqmem(0,
					sizeof(FEATURES) +
					featureLength + 1,
					TRUE,
					NULL);
    if (nfeature == NULL)
    {
        SIfprintf(stderr,
		    ERx("spec file nfeature segment allocation failed\n"));
        return(NULL);
    }

    nfeature->feature = &((char *)nfeature)[sizeof(FEATURES)];
    STmove(feature, EOS, featureLength + 1, nfeature->feature);
    
    if (specs->featureLast)
        specs->featureLast->next = nfeature;
    else
        specs->featureFirst = nfeature;
    
    specs->featureLast = nfeature;

    return nfeature;
}

/*
** Name: gen_pkg_subs_add()
**
** Purpose:
**	Add a substitution variable to the substitution list.
**
** Inputs:
**	char *sub(in) - substitution name/key
**	char *value(in) - value to substitute in place of the name/key
**
** Returns:
**	SUBS* of the newly added substitution definition
**	NULL if creation failed
**
*/
SUBS*
gen_pkg_subs_add(char *sub, char *value)
{
    i4 subLength = STlength(sub);
    i4 valueLength = -1;
    SUBS *subs = (SUBS *) MEreqmem(0, sizeof(SUBS) + subLength + 1, TRUE, NULL);

    if (subs == NULL) {
        SIfprintf(stderr, ERx("substitution spec file segment allocation failed\n"));
        return(NULL);
    }

    /* copy values into new subs structure */
    subs->sub = &((char *)subs)[sizeof(SUBS)];
    STmove(sub, EOS, subLength + 1, subs->sub);

    /*
    ** Check for Licensing case where value in rpmconfig may have been
    ** over ridden by -l flag
    */
    if ( license_type && STcompare(sub, "%rpm_license%") == 0)
	valueLength = STlength(license_type);
    else
        valueLength = STlength(value);

    /* allocate value separately since we may modify it */
    if (!(subs->value = MEreqmem(0, valueLength + 1, TRUE, NULL))) {
        SIfprintf(stderr, ERx("substitution spec value allocation failed\n"));
        MEfree((PTR) subs);
        return(NULL);
    }

    /* Again check for special case */
    if ( license_type && STcompare(sub, "%rpm_license%") == 0)
	STmove(license_type, EOS, valueLength + 1, subs->value);
    else
	STmove(value, EOS, valueLength + 1, subs->value);

    subs->valueLength = valueLength;
    subs->subLength = subLength;
    subs->diff = valueLength - subLength;

    if (subsLast) {
        subsLast->next = subs;
    } else {
        subsFirst = subs;
    }
    subsLast = subs;

    return subs;
}

/*
** Name: gen_pkg_subs_get()
**
** Purpose:
**	Retrieve a substite structure given a substitute name.
**
** Inputs:
**	char *sub(in) - substitute name
**
** Returns:
**	SUBS* of the requested definition
**	NULL if not found
**
*/
SUBS*
gen_pkg_subs_get(char *sub)
{
    SUBS *at;
    for (at = subsFirst; at; at = at->next)
        if (STequal(at->sub, sub))
            return at;
    return(NULL);
}

/*
** Name: gen_pkg_subs_set()
**
** Purpose:
**	Set the value of a substitution given a substitute name
**	and value.  Adds a substitution to the list if it does
**	not exist.
**
** Inputs:
**	char *sub(in) - substitute name
** 	 *value(in) - value of the substitution
**
** Returns:
**	SUBS* of the specified definition
**	NULL if creation failed
**
*/
SUBS*
gen_pkg_subs_set(char *sub, char *value)
{
    SIZE_TYPE valueLength = STlength(value);
    SUBS *subs = gen_pkg_subs_get(sub);
    char *new_value;
    if (!subs)
        return gen_pkg_subs_add(sub, value);
    
    /* allocate space for new value */
    new_value = MEreqmem(0, valueLength + 1, TRUE, NULL);
    if (!new_value) {
        SIfprintf(stderr, ERx("substitution new spec value allocation failed\n"));
        return 0;
    }

    STmove(value, EOS, valueLength + 1, new_value);
    MEfree((PTR) subs->value);
    subs->value = new_value;
    subs->valueLength = valueLength;
    subs->diff = subs->valueLength - STlength(subs->sub);
    
    return(subs);
}

/*
** Name: gen_pkg_cmclean ()
**
** Purpose:
**	Strip leading and trailing whitespace from a string.
**	Remove repeated whitespace and replace with a single space.
**
** Inputs:
**	char *str(in/out) - string to clean
**
** Returns:
**	char* of the cleaned string
**
*/
char *
gen_pkg_cmclean (char* str)
{
    i4 state = 0;
    char* to = str;
    char* at = str;

    while (CMwhite(at))
        CMnext(at);

    while (*at != EOS) {
        switch (state) {
            case 0:
                /* normal text */
                if (CMwhite(at)) {
                    /* switch to whitespace mode */
                    state = 1;
                    break;
                }
                CMcpychar(at, to);
                CMnext(to);
                break;
            case 1:
                /* whitespace mode */
                if (!CMwhite(at)) {
                    state = 0;
                    *to = ' ';
                    CMnext(to);
                    CMcpychar(at, to);
                    CMnext(to);
                }
                break;
            default:
                /* Huh? Unreachable */
                *to++ = ' ';
                *to = EOS;
                return(str);
        }
        CMnext(at);
    }
    *to = EOS;
    return(str);
}

/*
** Name: gen_pkg_strsubs()
**
** Purpose:
**	Process spec file template line substitutions
**
** Inputs:
**	char *str(in/out) - buffer to replace substitutions in
**	i4 maxCount(in) - maximum size of buffer
**
** Returns:
**	OK if substitutions were completed
**	FAIL if an error was encountered, str will still be
**	properly terminated
**
*/
STATUS
gen_pkg_strsubs( char* str, i4 maxCount )
{
    SUBS *subs;
    char *match, *loc, *at;
    SIZE_TYPE currentLength = STlength(str);
    i4 lp;

    for(subs = subsFirst; subs; subs = subs->next)
    {
        match = STindex(at = str, subs->sub, 0);
        while (match != NULL)
	{
	    if ( !STxcompare( match, subs->subLength, subs->sub,
				subs->subLength, FALSE, FALSE))
	    {
		/* 
		** Check special cases first
  		** do we need to skip this line?
		*/
		if ( !STcompare( subs->value, "<skip>" ) )
		    return( IP_SKIP_LINE );
		
		/* matched first character and sub, replace if possible */
		if (currentLength + subs->diff > maxCount)
		{
		    SIfprintf(stderr,
			"substitution would exceed maximum line length:\n   '%s' <- '%s'\n   %s\n",
			subs->sub,
			subs->value, str);
			return(FAIL);
		}
		if (subs->diff > 0)
		{
		    loc = match + subs->diff;
		    /* FIXME wrong overlap for ST/MEcopy */
		    memmove(loc, match, 1 + (currentLength - (match - str)));
		}
		else if (subs->diff < 0)
		{
		    loc = match - subs->diff;
		    MEcopy(loc, 1 + (currentLength - (loc - str)), match);
		}
		MEcopy(subs->value, subs->valueLength, match);
		currentLength += subs->diff;
		match = STindex(at, subs->sub, 0);
	    }
	    else
	    {
		/* matched first character but not the sub, advance */
		match = STindex(++at, subs->sub, 0);
	    }
        }
    }

    return (OK);
}

/*
** Name: gen_pkg_config_read_sub()
**
** Purpose:
**	Process a substitution line and add the value to the
**	substitution list.  If the value is not present an
**	empty string is provided.
**
**	The line is in the format:
**	    <sub name>[ <sub value>[ <ignored text]]
**
** Inputs:
**	char *line(in/out) - text line
**
** Returns:
**	OK on success
**	other values on failure
**
*/
STATUS
gen_pkg_config_read_sub( char *line )
{
    char *nextWord = line;
    while (*nextWord != EOS && !CMwhite(nextWord))
        CMnext(nextWord);
    if (*nextWord) {
        *nextWord = EOS;
        CMnext(nextWord);
    }
    return !gen_pkg_subs_set(line, nextWord);
}

/*
** Name: gen_pkg_config_read_esub()
**
** Purpose:
**	Process an environment variable substitution line.
**	Used to set a substitution to an environment variable value.
**	If the variable does not exist, it is treated as a normal
**	substitution.
**
**	The line is in the format:
**	    <sub name>[ <env variable name>[ <ignored text]]
**
** Inputs:
**	char *line(in/out) - text line
**
** Returns:
**	OK on success
**	other values on failure
**
*/
STATUS
gen_pkg_config_read_esub( char *line )
{
    char *nextWord = line, *esub = NULL;
    while (*nextWord != EOS && !CMwhite(nextWord))
        CMnext(nextWord);
    if (*nextWord != EOS) {
		*nextWord = EOS;
        CMnext(nextWord);
    }
	
	/* get the environment variable */
	NMgtAt(nextWord, &esub);
	if (esub == NULL) {
		STprintf(ERx("warning: '%s' <- '%s' environment substitution failed\n"), line, nextWord);
	}

    return !gen_pkg_subs_set(line, esub == NULL ? nextWord : esub);
}

/*
** Name: gen_pkg_config_read_spec()
**
** Purpose: 
**	Process a config file spec entry line.  If the unique part
**	of the spec name is not specified the spec entry will not
**	contain a unique part.  Use only up to one of these per
**	spec file definition.
**
**	The line is in the format:
**  [<spec name>]
**
** Inputs:
**	char *line(in/out) - text line
**
** Returns:
**	OK on success
**	other values on failure
**
*/
SPECS*
gen_pkg_config_read_spec( char *line )
{
    while (CMwhite(line))
        CMnext(line);
    return gen_pkg_specs_add(line);
}


/*
** Name: gen_pkg_create_path()
**
** Purpose:
**	Create a complete path.  Optionally displays an error
**	message if the creation fails.
**
** Inputs:
**	char *path(in) - path to create
**	char *errmsg(in) - displays this message on failure
**
** Returns:
**	OK if successful
**	other values on failure
**
*/
STATUS
gen_pkg_create_path(char *path, char *errmsg)
{
	char *pptr = path;
	char delim;
	STATUS status;

	/* advance over any leading slashes */
	if (*pptr != EOS && *pptr == '/')
		CMnext(pptr);

	while (*pptr != EOS) {
		while (*pptr != EOS && *pptr != '/')
			CMnext(pptr);
		delim = *pptr;
		*pptr = '\0';
		status = OK;
		if( IPCLcreateDir( path ) != OK )
			status = FAIL;	 /* if we fail to create the 
				 		directory, log it */
			
		*pptr = delim;
		CMnext(pptr);
	}

	if (errmsg != NULL && status) /* If we completely fail then report it */
	{
	    SIfprintf(stderr, errmsg);
	    SIfprintf(stderr, ERx("\t%s\n\n"), path);
	}

	return(status);
}

/*
** Name: gen_pkg_config_read()
**
** Purpose:
**	Process package generation config file.  This file contains
**	all of the spec file definitions and substitutions.
**
** Inputs:
**	char *filenameConfig(in) - file name of the config file
**	usually rpmconfig or debconfig
**
** Returns:
**	OK on success
**	other values on failure
**
*/
STATUS
gen_pkg_config_read( char *filenameConfig )
{
    FILE *file;
    char line[MAX_LOC + 1];
    i4 rval;
    LOCATION loc;
    FEATURES *ncontain = 0;
    SPECS *cspecs = 0;
    SUBS *nsubs = 0;
    i4 state = 0; /* initial */

    STcopy(filenameConfig, line);
    LOfroms(PATH & FILENAME, line, &loc);

    if ( SIfopen( &loc, "r", SI_TXT, 0, &file ) )
    {
        SIfprintf(stderr,
		 "failed to open config file '%s' for reading\n",
		 filenameConfig);
        return(FAIL);
    }

    rval = !gen_pkg_config_readline( file, line, sizeof(line));
        while (rval)
        {
                switch (state) {
            case 0: /* init */
                if (!STbcompare(ERx("$sub "), 5, line, 5, 0)) {
                    /* add substitution */
                    if (gen_pkg_config_read_sub(&line[5])) {
                        SIclose(file);
                        return(FAIL);
                    }
                } else if (!STbcompare(ERx("$spec"), 5, line, 5, 0)) {
                    /* process spec file section */
                    state = 1; /* switch to spec:features */
                    if (!(cspecs = gen_pkg_config_read_spec(&line[5]))) {
                        SIclose(file);
                        return(FAIL);
                    }
                } else if (!STbcompare(ERx("$esub "), 6, line, 6, 0)) {
                                        /* add environment substitution */
                                        if (gen_pkg_config_read_esub(&line[6])) {
                                                SIclose(file);
                                                return(FAIL);
                                        }
                                }
                break;
            case 1: /* in spec:features */
                if (STbcompare(ERx("$feature "), 9, line, 9, 0)) {
                    state = 0;
                    cspecs = 0;
                    continue;
                }
                gen_pkg_specs_features_add(cspecs, &line[9]);
                break;
            default: /* Huh? Unreachable */
                SIclose(file);
                return(FAIL);
                }
        rval = !gen_pkg_config_readline (file, line, sizeof(line));
        }

    SIclose(file);
    return(0);
}

/*
** Name: gen_pkg_config_readline()
**
** Purpose:
**	Read a config file line and toss comments and blanks.
**	Only "real lines" should be returned.
**	Strip leading, trailing blanks, repeated spaces/tabs.
**
**	Note: SIgetrec() does not provide the capability of
**	detecting long lines.
**
** Inputs:
**	file(in) - filename to read from (current position)
**	maxCount(in) - maximum characters to read into buffer
**
** Outputs:
**	line(out) - buffer to store line in
**
** Returns:
**	OK if a line was successfully read
**	FAIL on error
**
**
*/
STATUS
gen_pkg_config_readline (FILE* file, char* line, i4 maxCount)
{
   
    line[maxCount-1] = EOS;
    while (OK == SIgetrec(line, maxCount, file))
    {
  	if (line[maxCount-1] != EOS)
	{
	    line[maxCount-1] = EOS;
            SIfprintf( stderr, "warning: maximum line length encountered\n" );
        }
        gen_pkg_cmclean(line);

	if (line[0] != EOS && line[0] != '#')
                        return(OK);
    }
    return(FAIL);
}


/*
** Name: gen_pkg_hlp_file()
**
** Purpose:
**	Write help files associted with a given SPEC list to open FILE.
**
** History:
**	29-Jun-2007 (hanje04)
**	    Created.
*/
STATUS
gen_pkg_hlp_file( SPECS *specs, FILE *outFile, pkg_help_format fmt )
{
    FEATURES	*feat;
    LOCATION	hlploc;
    char	hlpbuf[MAX_LOC];
    char	wbuff[READ_BUF];
    char	*rbuff;
    FILE	*hlpFile;
    const char	*hlpfilefmt = "%s/%s.hlp";
    i4		byteswritten = 0;
    STATUS	status;

    /* initialize */
    if ( ! SIisopen( outFile ) )
   	return( FAIL );

    /* need to indent help for deb control files */
    if ( fmt == DEB_HLP_FMT )
    {
	wbuff[0] = ' ';
	rbuff = &wbuff[1];
    }
    else
	rbuff = wbuff;
   

    /*
    ** scroll through the features in the spec list and check if a help file
    ** exists for them. If it does, write the contents to the outFile
    */
    for ( feat = specs->featureFirst ; feat ; feat = feat->next )
    {

        if( (!spatial) && ( STequal( feat->feature, ERx("spatial") ) ||
                            STequal( feat->feature, ERx("spatial64")) ) )          
        {
                 continue;
        }

	STprintf( hlpbuf, hlpfilefmt, manifestDir, feat->feature );
	LOfroms( PATH & FILENAME, hlpbuf, &hlploc );

        if ( LOexist( &hlploc ) == OK )
	{
	    if ( SIfopen( &hlploc, "r", SI_TXT, 0, &hlpFile ) )
	    {
		SIfprintf( stderr,
			    "Failed to open help file for reading:\n%s.hlp\n",
			    feat->feature );
		return( FAIL );
	    }

	    /* write contents of hlp file to outFile */
	    while( SIgetrec( rbuff, READ_BUF - 1, hlpFile ) != ENDFILE )
	    {
		/* no blank lines for deb control files */
		if ( fmt == DEB_HLP_FMT && *rbuff == '\n' )
		    STcopy( ".\n", rbuff ); 

		SIfprintf( outFile, "%s", wbuff );
		/*
		if ( SIfprintf( outFile, "%s", buff ) != OK )
		{
		    perror( "Failed to write help file" );
		    SIclose( hlpFile );
		    return( FAIL );
		}
		*/
		SIflush( outFile );
	    }

	    /* add final line */
	    if ( fmt == DEB_HLP_FMT )
		SIfprintf( outFile, " .\n" );
	    else
		SIfprintf( outFile, "\n" );

	    /* done */
	    SIclose( hlpFile );
	}
    }

    return( OK );
}

/*
** Name: gen_pkg_su_file()
**
** Purpose:
**	Find setup files associated with a given "SPECS" structure and
**	write them to the buffer that is passed in. Return the size
**	of the string written.
** History:
**      29-Jun-2007 (hanje04)
**          Created.
**	15-Jan-2010 (hanje04)
**	    Don't add -s to subuf, should be in template if needed (which it
**	    sometimes isn't.
*/
int
gen_pkg_su_files( SPECS *specs, char *sulist, i4 buflen )
{
    FEATURES	*features;
    PKGBLK	*pkg;
    PRTBLK	*prt;
    REFBLK	*ref;
    int		featlen;
    char	subuf[256] = "" ;

    /* sanity check */
    if ( specs == NULL )
	return( FAIL );

    /*
    ** Scan through package list, find the packages contained in
    ** the SPEC and write out the setup files contained within.
    */
    SCAN_LIST(distPkgList, pkg)
    {
	if( (!spatial) && ( STequal( pkg->feature, ERx("spatial") ) ||
		            STequal( pkg->feature, ERx("spatial64")) ) )		
	{
		 continue;
	}
        featlen = STlength( pkg->feature );

        for ( features = specs->featureFirst; features;
		features = features->next)
	{

            if ( ! STxcompare(pkg->feature,
		    featlen,
		    features->feature,
		    featlen, TRUE, FALSE ) )
	    {
		if( ip_list_is_empty( &pkg->prtList ) )
		    continue;
                

		SCAN_LIST(pkg->prtList, prt)
		    SCAN_LIST(prt->refList, ref)
		    {
			/* add seperater if buffer not empty */
			if ( ! *subuf == EOS )
			    STcat( subuf, ":" );

			STcat( subuf, ref->name );
		    }
	    } /* find matches */
	} /* scan SPEC for features */
    } /* Scan distribution */

    /* check passed in buffer is big enough to hold list */
    if ( buflen < STlen(subuf) )
    {
	SIfprintf( stderr, "ERROR: Buffer too short to hold setup list\nBuffer is: %d bytes\nList is: %d bytes\n", sizeof( *sulist ), STlen(subuf) );
	return( -1 );
    }

    /* write buffer to file */
    STcopy( subuf, sulist );
    return( STlen( sulist ) );
}
