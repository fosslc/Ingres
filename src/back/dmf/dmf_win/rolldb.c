/*
**  Copyright (c) 1995 Ingres Corporation
**
**  Name: rollforwarddb   - roll forward a database
**
**  Usage: 
**	rollforwarddb [+c] [-c] [#c ] [+j] [-j] [-mdevice] [-uusername]
**		[-v] [+w|-w] [-bdd-mmmm-yyyy[:hh:mm:ss]] [-edd-mmmm-yyyy[:hh:mm:ss]]
**		[-table=] [-nosecondary_index] [-noblobs] [-on_error_continue]
**		[-relocate -location= -new_location=] [-statistics]
**		dbname
**
**  Description:
**	This program provides the INGRES `rollforwarddb' command by
**	executing the journaling support program, passing along the
**	command line arguments.
** 
**  Exit Status:
**	OK	rollforwarddb (dmfjsp) succeeded.
**	FAIL	rollforwarddb (dmfjsp) failed.	
**
**  History:
**	18-jul-1995 (wonst02)
**		Created.  (based upon ingstart.c)
**      08-may-1996 (sarjo01)
**              For bug 76253: added restrictions for tape dev restore
**              1) must use +w
**              2) no table level restore to tape because of
**                 ntbackup limitation.
**    08-apr-97 (mcgem01)
**            Changed CA-OpenIngres to OpenIngres.
**	12-jan-1998 (somsa01)
**		Run dmfjsp via PCexec_suid(). (Bug #87751)
**      20-apr-98 (mcgem01)
**              Changed OpenIngres to Ingres.
**
*/
/*
**
PROGRAM = rollforwarddb
**
*/

# include <compat.h>
# include <er.h>
# include <pc.h>
# include <st.h>


void
main(int argc, char *argv[])
{
#define 	MAXBUF	4095

	char 	buf[ MAXBUF+1 ];
	int		iarg, ibuf, ichr;
        int     minus_m = FALSE;
        int     minus_t = FALSE;
        int     plus_w  = FALSE;

	/*
	 *	Put the program name and first parameter in the buffer.
	 */
	STprintf(buf, ERx( "dmfjsp rolldb" ));

	switch(argc)
	{
	    case 1:
		break;

		/*
		 *	Append any args from the command line, e.g., 
		 *		dmfjsp rolldb -table=tablename dbname
		 */
	    default:
		ibuf = sizeof(ERx("dmfjsp rolldb")) - 1;
		for (iarg = 1; (iarg < argc) && (ibuf < MAXBUF); iarg++) {
                    if (STbcompare(argv[iarg], 2, "-m", 2, TRUE) == 0)
                        minus_m = TRUE;
                    else if (STbcompare(argv[iarg], 2, "-t", 2, TRUE) == 0)
                        minus_t = TRUE;
                    else if (STbcompare(argv[iarg], 2, "+w", 2, TRUE) == 0)
                        plus_w = TRUE;
		    buf[ibuf++] = ' ';
		    for (ichr = 0; 
		    	 (argv[iarg][ichr] != '\0') && (ibuf < MAXBUF); 
		    	 ichr++, ibuf++)
		    	buf[ibuf] = argv[iarg][ichr];
		}
		buf[ibuf] = '\0';
	}
        if (minus_m == TRUE)
        {
            if (minus_t == TRUE)
            {
                SIprintf(
            "Partial Rollforward from a Tape device is not currently\n"
            "supported by Ingres for Windows NT. Please see\n"
            "Release Notes for further information.\n");
                PCexit(FAIL);
            }
            if (plus_w == FALSE)
            {
                SIprintf(
        "The parameter +w (wait for DB not in use) MUST\n"
        "be used if -m (tape device) has also been specified.\n"
        "Please see Release Notes for further information.\n");
                PCexit(FAIL);
            }
        }
	/*
	 *	Execute the command.
	 */
	if( PCexec_suid(&buf) != OK )
	    PCexit(FAIL);

	PCexit(OK);
}

