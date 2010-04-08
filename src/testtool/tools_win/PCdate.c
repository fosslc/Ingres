/* LOCALTIM.C: This program uses time to get the current time 

 * and then uses localtime to convert this time to a structure 
 * representing the local time. The program converts the result 
 * from a 24-hour clock to a 12-hour clock and determines the 
 * proper extension (AM or PM).
 */

#include <stdio.h>
#include <time.h>

void main( void )
{
        struct tm *newtime;
        time_t long_time;

        time( &long_time );                /* Get time as long integer. */
        newtime = localtime( &long_time ); /* Convert to local time. */

        printf( "%s", asctime( newtime ));
}
