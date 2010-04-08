/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
**  Print Environment variable
**
**  History.
**      17-Jul-95 (fanra01)
**          Created.
**      26-apr-96 (musro02)
**          Added include for compat.h to correct undefined symbol NULL.
**          This made the include of stdlib.h unnecessary
**      26-Aug-97 (merja01)
**          Re-added definition of stdlib.h.  It is need on the axp_osf
**          platform.
**
*/
#include <stdlib.h>	
#include <compat.h>	/* for NULL */

/*
**  prenv
**
**  Description
**      Program takes the environment variable name as the argument passed
**      and uses it to obtain the value.
**      If the environment variable exists its value is displayed and the
**      program returns 0 otherwise the program returns 1.
*/

int main ( int argc , char ** argv)
{
char * env;
int status = 0;

    if (argc != 2)
    {
        printf("Usage : %s <environment variable>\n", *argv);
        status = 1;
    }
    if ((env = getenv(argv[1])) == NULL)
    {
        status = 1;
    }
    else
    {
        printf("%s",env);
    }
    return(status);
}
