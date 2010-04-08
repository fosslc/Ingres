/*
** Copyright (c) 2007 Ingres Corporation. All rights reserved.
*/

/*
**  Name: genpkg.h
**
**  Purpose: 
**	Header for SPEC/Control file generation process.
** 
**  History:
**	21-Jun-2007 (hanje04)
**	    SIR 118420
**	    Created from genrpm.c
**	02-Mar-2010 (hanje04)
**	    SIR 123296 
**	    Add LSB location defines
*/

/**** defines ****/
# define SETUID_MASK    04000
/* return codes for gen_pkg_strsubs() */
# define IP_SKIP_LINE		(STATUS)2
# define IP_WRITE_SULIST	(STATUS)3
# define READ_BUF	1024
# define LSBBINDIR	"usr/bin"
# define LSBLIBDIR	"usr/lib"
# define LSBLIBEXECDIR	"usr/libexec"
# define LSBDATADIR	"usr/share"

/**** type definitions ****/
typedef struct _FEATURES
{
    struct _FEATURES    *next;          /* next feature */
    char                *feature;       /* feature name */
} FEATURES;

typedef struct _SPECS {
    struct _SPECS       *next;          /* next spec file definition */
    char                *name;          /* spec package name */
    FEATURES            *featureFirst;  /* list of features the spec file includes */
    FEATURES            *featureLast;   /* last item in the list */
} SPECS;

typedef struct _SUBS {
    struct _SUBS        *next;          /* next substitution */
    char                *sub;           /* string to substitute */
    char                *value;         /* value to substitute with */
    int                 diff;           /* strlen(value) - strlen(sub) */
    int                 valueLength;    /* strlen(value) */
    int                 subLength;      /* strlen(sub); */
} SUBS;

typedef enum {
    RPM_HLP_FMT,
    DEB_HLP_FMT
} pkg_help_format;

/* Global refs */
GLOBALREF char manifestDir[ MAX_LOC + 1 ];

/**** prototypes ****/
STATUS gen_rpm_spec();
STATUS gen_deb_spec();
SPECS* gen_pkg_specs_add(char *name);
FEATURES* gen_pkg_specs_features_add(SPECS *specs, char *feature);
SUBS* gen_pkg_subs_add(char *sub, char *value);
SUBS* gen_pkg_subs_get(char *sub);
SUBS* gen_pkg_subs_set(char *sub, char *value);
char * gen_pkg_cmclean (char* str);
STATUS gen_pkg_strsubs( char* str, int maxCount );
STATUS gen_pkg_config_read_sub( char *line );
STATUS gen_pkg_config_read_esub( char *line );
SPECS* gen_pkg_config_read_spec( char *line );
STATUS gen_pkg_create_path(char *path, char *errmsg);
STATUS gen_pkg_config_read( char *filenameConfig ); 
STATUS gen_pkg_config_readline (FILE* file, char* line, int maxCount);
STATUS gen_pkg_hlp_file( SPECS *specs, FILE *outFile, pkg_help_format fmt );
int gen_pkg_su_files( SPECS *specs, char *sulist, int buflen );





