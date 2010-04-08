/*
**  Copyright (c) 1995 Ingres Corporation
**
**  Name: ckpdb   - take checkpoint of a database
**
**  Usage: 
**	ckpdb 	[-d] [+j|-j] [-l] [-mdevice] [-table=tbl1,tbl2,...]
**		[-uusername] [-v] [+w|-w] dbname
**
**  Description:
**	This program provides the INGRES `ckpdb' command by
**	executing the journaling support program, passing along the
**	command line arguments.
** 
**  Exit Status:
**	OK	ckpdb (dmfjsp) succeeded.
**	FAIL	ckpdb (dmfjsp) failed.	
**
**  History:
**	18-jul-1995 (wonst02)
**		Created.  (based upon ingstart.c)
**	08-may-1996 (sarjo01)
**              For bug 76253: added restrictions for tape dev checkpoint 
**              1) must use -l, +w
**              2) no table level checkpoint to tape because of
**                 ntbackup limitation. 
**	08-apr-97 (mcgem01)
**	        Changed CA-OpenIngres to OpenIngres.
**	08-jan-1998 (somsa01)
**		Run dmfjsp via PCexec_suid(). (Bug #87751)
**	20-apr-98 (mcgem01)
**		Changed OpenIngres to Ingres.
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
	int	iarg, ibuf, ichr;
        int     minus_m = FALSE;
        int     minus_l = FALSE;
        int     minus_t = FALSE;
        int     plus_w  = FALSE;

	/*
	 *	Put the program name and first parameter in the buffer.
	 */
	STprintf(buf, ERx( "dmfjsp ckpdb" ));

	switch(argc)
	{
	    case 1:
		break;

		/*
		 *	Append any args from the command line, e.g., 
		 *		dmfjsp ckpdb -table=tbl1,tbl2 dbname
		 */
	    default:
		ibuf = sizeof(ERx("dmfjsp ckpdb")) - 1;
		for (iarg = 1; (iarg < argc) && (ibuf < MAXBUF); iarg++) {
                    if (STbcompare(argv[iarg], 2, "-m", 2, TRUE) == 0)
                        minus_m = TRUE;
                    else if (STbcompare(argv[iarg], 2, "-l", 2, TRUE) == 0) 
                        minus_l = TRUE;
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
            "Partial Checkpoint to Tape Device is not currently\n"
            "supported by Ingres for Windows NT. Please see\n"
            "Release Notes for further information.\n");
	        PCexit(FAIL);
            }
            if (minus_l == FALSE || plus_w == FALSE)
            {
                SIprintf(
        "The parameters -l (exclusive lock) and +w (wait for DB not in use)\n"
        "MUST be used if -m (tape device) has also been specified.\n"
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

