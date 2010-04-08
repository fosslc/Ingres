# if defined(su4_us5) && defined(INCLUDE_LOOKUP_FCN)
# include <dlfcn.h>

char *IIdldatecreated = "today";

char *IIdlident = "Vers 1";


int
IIdllookupfunc( void *handle, char *name, void **retval )
{
    char *locname = name;

    if ( *name == '!' )
	locname++;
    *retval = dlsym( handle, locname );
    if ( *retval == (char *) 0 )
	return ( 1 );
    else
	return ( 0 );
}
# endif /* defined(su4_us5) && defined(INCLUDE_LOOKUP_FCN) */

int
returns_one()
{
    return ( 1 );
}

int
returns_zero()
{
    return ( 0 );
}

int
returns_minus_one()
{
    return ( -1 );
}

char *
returns_string()
{
    static char *localstring = "String";

    return ( localstring );
}
