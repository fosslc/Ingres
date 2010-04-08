# include <stdio.h>
# include <string.h>
 
char *IIdldatecreated = "24-dec-1997";
 
char *IIdlident = "Vers 1";
 
int returns_ninetynine( void );
 
extern "C" {
int
IIdllookupfunc( void *handle, char *name, void **retval )
{
    char *locname = name;
 
    if ( *name == '!' )
        locname++;
    if ( strcmp( locname, "returns_ninetynine" ) == 0 )
    {
	*retval = returns_ninetynine;
	return ( 0 );
    }
    else
    {
	*retval = NULL;
	return ( 1 );
    }
	
}
}

int
returns_ninetynine()
{
    return ( 99 );
}
