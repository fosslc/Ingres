# include <compat.h>
# include <gl.h>
# include <lo.h>
# include <dl.h>
# include <si.h>
# include <pc.h>
# include <ds.h>
# include <ut.h>
# include <nm.h>
# include <st.h>

/*
** History:
**	27-may-97 (mcgem01)
**	    Clean up compiler warnings.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
*/
/*
PROGRAM = dltest
NEEDLIBS = COMPAT
*/

char *module = "dltstmod";

char *lookups[] = {
    "returns_one",
    "returns_zero",
    "returns_minus_one",
    "returns_string",
    NULL
};

char *srcs[] = {
    "dltstmod.c",
    NULL
};

char *objs[] = {
# ifdef NT_GENERIC
    "dltstmod.obj",
# endif
# ifdef UNIX
    "dltstmod.o",
# endif
    NULL
};

char *module1 = "dltstmod1";
# ifdef NT_GENERIC
char *modfname = "dltstmod1.dll";
# endif /* NT_GENERIC */
# ifdef UNIX
char *modfname = "dltstmod1.so.2";
# endif /* UNIX */

char *lookups1[] = {
    "returns_onehundred",
    NULL
};

char *srcs1[] = {
    "dltstmod1.c",
    NULL
};

char *objs1[] = {
# ifdef NT_GENERIC
    "dltstmod1.obj",
# endif /* NT_GENERIC */
# ifdef UNIX
    "dltstmod1.o",
# endif /* UNIX */
    NULL
};

char *module2 = "dltstmod2";
# ifdef NT_GENERIC
char *modfname2 = "dltstmod2.dll";
# endif /* NT_GENERIC */
# ifdef UNIX
char *modfname2 = "dltstmod2.so.2";
# endif /* UNIX */

char *lookups2[] = {
    "returns_ninetynine",
    NULL
};

char *srcs2[] = {
    "dltstmod2.cc",
    NULL
};

char *objs2[] = {
# ifdef NT_GENERIC
    "dltstmod2.obj",
# endif /* NT_GENERIC */
# ifdef UNIX
    "dltstmod2.o",
# endif /* UNIX */
    NULL
};

char *errmod = "dltsterr";

char *errlookups[] = {
    "errfunc",
    NULL
};

char *errsrcs[] = {
    "dltsterr.c",
    NULL
};

char *errobjs[] = {
# ifdef NT_GENERIC
    "dltsterr.obj",
# endif /* NT_GENERIC */
# ifdef UNIX
    "dltsterr.o",
# endif /* UNIX */
    NULL
};

char *erroutput = "dltsterr.out";

int
main( int argc, char **argv )
{
    CL_ERR_DESC err;
    PTR handle;
    PTR handle1;
    PTR handle2;
    PTR addr;
    STATUS status;
    int (*int_fcn)();
    char *(*str_fcn)();
    int int_ret;
    char *cp_ret;
    LOCATION srcfile;
    char srcbuf[MAX_LOC+1];
    LOCATION objfile;
    char objbuf[MAX_LOC+1];
    bool pristine;
    LOCATION modloc;
    char modbuf[MAX_LOC+1];
    LOCATION dlloc;
    char dlbuf[MAX_LOC+1];
    LOCATION errloc;
    char errbuf[MAX_LOC+1];
    LOCATION nameloc;
    char namebuf[MAX_LOC+1];
    char *nameptr;
    char *parms[2];
    i4  flags;
    STATUS mod2status = OK;
    
    /* find the working directory */
    NMgtAt( "IIDLDIR", &cp_ret );
    if ( cp_ret == NULL || *cp_ret == EOS )
    {
	SIprintf( "%sDLDIR not set\n", SystemVarPrefix );
	PCexit( 1 );
    }

    /* save the directory name into a larger buffer for LO */
    STcopy( cp_ret, srcbuf );

    /* initialize a LOCATION with the directory */
    LOfroms( PATH, srcbuf, &srcfile );

    /* initialize the object file LOCATION with same directory */
    LOcopy( &srcfile, objbuf, &objfile );
    LOcopy( &srcfile, modbuf, &modloc );
    LOcopy( &srcfile, dlbuf, &dlloc );
    LOcopy( &srcfile, errbuf, &errloc );
    
    /* add filenames to LOCATION */
    LOfstfile( srcs[0], &srcfile );
    LOfstfile( objs[0], &objfile );
    LOfstfile( erroutput, &errloc );
    
    /* compile with error */
#ifdef NT_GENERIC
    parms[0] = "-g";
#else NT_GENERIC
    parms[0] = "-Zi";
#endif NT_GENERIC
    parms[1] = NULL;
    status = UTcompile_ex( &srcfile, &objfile, &errloc, parms, FALSE, &pristine, &err );
    if ( status != OK )
    {
        SIprintf( "Compile error test returns: " );
        if ( status == UT_CO_FILE )
            SIprintf( 
		"\n  utcom.def file not found or II_COMPILE_DEF not set\n" );
        else if ( status == UT_CO_IN_NOT )
            SIprintf( "source file not found.\n" );
        else
            SIprintf( "%d\n", status );
        SIprintf( "Contents of error file:\n" );
        SIprintf( "------------------------------------------------------\n" );
        SIcat( &errloc, stdout );
        SIprintf( "------------------------------------------------------\n" );
        SIprintf( "\n" );
    }
    else
    {
        SIprintf( "Test of compile error returned OK.\n" );
        SIprintf( "Contents of error file:\n" );
        SIprintf( "------------------------------------------------------\n" );
        SIcat( &errloc, stdout );
        SIprintf( "------------------------------------------------------\n" );
        SIprintf( "\n" );
    }

    /* add filenames to LOCATION */
    LOfstfile( srcs[0], &srcfile );
    LOfstfile( objs[0], &objfile );

    /* do a good compile */
#ifdef NT_GENERIC
    parms[0] = "-Zi";
#else NT_GENERIC
    parms[0] = "-g";
#endif NT_GENERIC
    status = UTcompile_ex( &srcfile, &objfile, NULL, parms, FALSE, &pristine, &err );
    if ( status != OK )
    {
	SIprintf( "Error compiling module: %d\n", status );
	PCexit( 1 );
    }

    LOfstfile( srcs1[0], &srcfile );
    LOfstfile( objs1[0], &objfile );

    status = UTcompile( &srcfile, &objfile, NULL, &pristine, &err );
    if ( status != OK )
    {
	SIprintf( "Error compiling module1\n" );
	PCexit( 1 );
    }

    LOfstfile( srcs2[0], &srcfile );
    LOfstfile( objs2[0], &objfile );

    mod2status = UTcompile( &srcfile, &objfile, NULL, &pristine, &err );
    if ( mod2status != OK )
    {
	SIprintf( "Error compiling module2\n" );
	SIprintf( "This is not an error if C++ is not installed.\n" );
	SIprintf( "Skipping further processing of this module\n\n" );
    }

    /* link first module into shared library */
    if ( DLcreate( NULL, NULL, module, objs, NULL, lookups, &err) != OK )
    {
	SIprintf( "Error creating module\n" );
	PCexit( 1 );
    }

    if ( DLcreate_loc( NULL, NULL, module1, objs1, NULL, lookups1, 
		       &dlloc, NULL, &errloc, FALSE, NULL, &err) != OK )
    {
	SIprintf( "Error creating module1\n" );
	PCexit( 1 );
    }
    SIprintf( "Contents of DLcreate_loc() error file for module1:\n" );
    SIprintf( "------------------------------------------------------\n" );
    SIcat( &errloc, stdout );
    SIprintf( "------------------------------------------------------\n" );
    SIprintf( "\n" );

    if ( mod2status == OK )
    {
        if ( DLcreate_loc( NULL, NULL, module2, objs2, NULL, lookups2, 
		           &dlloc, NULL, &errloc, FALSE, "C++", &err) != OK )
        {
	    SIprintf( "Error creating module2\n" );
	    PCexit( 1 );
        }
        SIprintf( "Contents of DLcreate_loc() error file for module2:\n" );
        SIprintf( "------------------------------------------------------\n" );
        SIcat( &errloc, stdout );
        SIprintf( "------------------------------------------------------\n" );
        SIprintf( "\n" );
    }

    STcopy( module1, namebuf );
    LOfroms( FILENAME, namebuf, &nameloc );
    DLconstructloc( &nameloc, namebuf, &nameloc, &err );
    LOtos( &nameloc, &nameptr );
    SIprintf( "Full path for %s is \"%s\"\n", module1, nameptr );

    LOcopy( &dlloc, namebuf, &nameloc );
    LOfstfile( module, &nameloc );
    DLconstructloc( &nameloc, namebuf, &nameloc, &err);
    LOtos( &nameloc, &nameptr );
    SIprintf( "Full path for %s is \"%s\"\n", module, nameptr );

    /* test an error */
    SIprintf( "Testing DLprepare of non-existant module.\n" );
    if ( DLprepare((char *)NULL, "XXX", lookups, &handle, &err) == OK )
    {
	SIprintf( "Error:  returned success, should be failure.\n" );
	PCexit( 1 );
    }

    /* load the modules into the executable */
    SIprintf( "Now loading the real modules.\n" );
    if ( DLprepare((char *)NULL, module, lookups, &handle, &err) != OK )
    {
	SIprintf( "Error loading module\n" );
	PCexit( 1 );
    }
    if ( mod2status == OK &&
         DLprepare((char *)NULL, module2, lookups2, &handle2, &err) != OK )
    {
	SIprintf( "Error loading module2\n" );
	PCexit( 1 );
    }
    putenv( "IIDLDIR=XXX" );
    flags=0;
    if ( DLprepare_loc((char *)NULL, module1, lookups1, &dlloc, flags,
                       &handle1, &err) != OK )
    {
	SIprintf( "Error loading module1\n" );
	PCexit( 1 );
    }

    /* get the address of a function and test it */
    if ( DLbind(handle1, "returns_onehundred", &addr, &err) != OK )
    {
	/* "returns_onehundred" function isn't there */
	DLunload( handle, &err );
	PCexit( 1 );
    }
    int_fcn = (int (*)()) addr;
    int_ret = (*int_fcn)();
    SIprintf( "\"returns_onehundred\" returns %d\n", int_ret );
    if ( int_ret != 100 )
    {
	/* function didn't correctly return 100 */
	PCexit( 1 );
    }

    if ( mod2status == OK )
    {
        if ( DLbind(handle2, "returns_ninetynine", &addr, &err) != OK )
        {
	    SIprintf( "Error binding \"returns_ninetynine\"\n" );

	    /* "returns_ninetynine" function isn't there */
	    DLunload( handle2, &err );
	    PCexit( 1 );
        }
        int_fcn = (int (*)()) addr;
        int_ret = (*int_fcn)();
        SIprintf( "\"returns_ninetynine\" returns %d\n", int_ret );
        if ( int_ret != 99 )
        {
	    /* function didn't correctly return 99 */
	    PCexit( 1 );
        }
    }

    if ( DLbind(handle, "returns_one", &addr, &err) != OK )
    {
	/* "returns_one" function isn't there */
	DLunload( handle, &err );
	PCexit( 1 );
    }
    int_fcn = (int (*)()) addr;
    int_ret = (*int_fcn)();
    SIprintf( "\"returns_one\" returns %d\n", int_ret );
    if ( int_ret != 1 )
    {
	/* function didn't correctly return 1 */
	PCexit( 1 );
    }

    if ( DLbind(handle, "returns_zero", &addr, &err) != OK )
    {
	/* "returns_zero" function isn't there */
	DLunload( handle, &err );
	PCexit( 1 );
    }
    int_fcn = (int (*)()) addr;
    int_ret = (*int_fcn)();
    SIprintf( "\"returns_zero\" returns %d\n", int_ret );
    if ( int_ret != 0 )
    {
	/* function didn't correctly return 0 */
	PCexit( 1 );
    }

    if ( DLbind(handle, "returns_minus_one", &addr, &err) != OK )
    {
	/* "returns_minus_one" function isn't there */
	DLunload( handle, &err );
	PCexit( 1 );
    }
    int_fcn = (int (*)()) addr;
    int_ret = (*int_fcn)();
    SIprintf( "\"returns_minus_one\" returns %d\n", int_ret );
    if ( int_ret != -1 )
    {
	/* function didn't correctly return -1 */
	PCexit( 1 );
    }

    if ( DLbind(handle, "returns_string", &addr, &err) != OK )
    {
	/* "returns_string" function isn't there */
	DLunload( handle, &err );
	PCexit( 1 );
    }
    str_fcn = (char *(*)()) addr;
    cp_ret = (*str_fcn)();
    SIprintf( "\"returns_string\" returns \"%s\"\n", cp_ret );
    if ( STcompare(cp_ret, "String") != OK )
    {
	/* function didn't correctly return "String" */
	PCexit( 1 );
    }

    /* functions were called successfully, now unload the modules */
    if ( DLunload( handle, &err ) != OK )
    {
	SIprintf( "Error unloading module.\n" );
	PCexit( 1 );
    }
    if ( DLunload( handle1, &err ) != OK )
    {
	SIprintf( "Error unloading module.\n" );
	PCexit( 1 );
    }
    if ( mod2status == OK && 
         DLunload( handle2, &err ) != OK )
    {
	SIprintf( "Error unloading module2.\n" );
	PCexit( 1 );
    }

    if ( DLdelete_loc( module1, &dlloc, &err ) != OK )
    {
	SIprintf( "Error deleting module1.\n" );
	PCexit( 1 );
    }

    SIprintf( "Successfully executed.\n" );
    PCexit( 0 );
}
