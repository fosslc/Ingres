/*
**Copyright (c) 1986, 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<st.h>
#include	<lo.h>
#include	<si.h>
#include	<er.h>
#include	<te.h>		 
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ade.h>
#include	<frmalign.h>
#include	<fdesc.h>
#include	<abfrts.h>
#include	<iltypes.h>
#include	<ilops.h>
#include	<ioi.h>
#include	<ifid.h>
#include	<ilrf.h>
#include	<ilrffrm.h>
#include	<ilerror.h>
#include	"il.h"
#include	"if.h"
#include	"ilgvars.h"
#include	"ilargs.h"

/**
** Name:	ilgetargs.c -	Get command line arguments
**
** Description:
**	Gets command line arguments.
**
** History:
**	11-Nov-1993 (tad)
**		Bug #56449
**		Replace %x with %p for pointer values where necessary.
**	Revision 6.2  89/12/14  joe
**	Upgraded for 6.2 interpreter.
**
**	Revision 5.1  86/05/21  arthur
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

GLOBALDEF ILARGS	IIITARGS;

/*
** Name:	IIITparseArgs() -	Process command line arguments
**
** Description:
**	Processes command line arguments.
**
** Inputs:
**	argc		Number of arguments
**	argv		Argument vector
**
** Outputs:
**	args		ILARGS structure to assign the different
**			arguments.
**
** History:
**	21-May-1986 (agh)
**		First written
**	14-dec-1988 (joe)  Updated for 6.2.
**	13-feb-1989 (bobm)  Added support for run-time file and start on
**		procedure.
**	19-mar-1990 (jhw)  Modified to allow start procedure, frame, and the
**		start object name to all be specified separately.
**	08/16/91 (emerson)
**		Added logic to map +c flag into new ilconnect member
**		of ILARGS.  (Bug 37878).
**	26-may-92 (leighb) DeskTop Porting Change:
**		Added code to set GUI title correctly inside a GUITITLE ifdef.
*/
VOID
IIITparseArgs ( argc, argv, args )
i4	argc;
char	**argv;
ILARGS	*args;
{
	char	*dump = NULL;	/* from -I flag */
	char	*debug = NULL;	/* from -D flag */
	char	*test = NULL;	/* from -Z flag */
	char	*tfile = NULL;	/* string used to enable use of parent's
				   keystroke file by this process */
	bool	gotdb = FALSE;
	bool	gotinfile = FALSE;
	bool	gotframe = FALSE;
	STATUS	rval;

	args->ilstartproc = FALSE;
	args->ilstartframe = FALSE;
	args->ildatabase = NULL;
	args->ilinfile = NULL;
	args->ilstartname = NULL;
	args->ilxflag = args->iluname = args->ilconnect = ERx("");
	args->ilrunmode = ILNRMNORMALRUNMODE;
	IIOtiTrcInit(argv);
	while ( --argc > 0 )
	{
    		switch (**++argv)
    		{
    		  case '+':
    			switch ( *(*argv + 1) )
    			{
			  case 'c':
				args->ilconnect = *argv + 2;
				break;

			  default:
				IIOerError(ILE_FATAL, ILE_FLAGBAD, *argv,
						(char *) NULL
				);
				break;
    			}
			break;
    		  
    		  case '-':
    			switch ( *(*argv + 1) )
    			{
			  case 'a':
			  case 'A':
				IIOapplname = *argv + 2;
#ifdef	GUITITLE						 
				TEsetTitle(IIOapplname);	 
#endif								 
				break;

			  case 'd':
				/* Note:  -d<dbname> is required even if -X
				** is supplied since the interpreter will
				** start up a separate session for the user.
				*/
				if (gotdb)
				{
					IIOerError(ILE_FATAL, ILE_DUPARG,
						ERget(F_IT0002_database),
						(char *) NULL);
				}
				else
				{
					args->ildatabase = *argv + 2;
					gotdb = TRUE;
				}
				break;

			  case 'D':
				debug = *argv + 2;
				break;

			  case 'e':
			  case 'E':
			  case 'r':
				if (gotinfile)
				{
					IIOerError(ILE_FATAL, ILE_DUPARG,
						ERget(F_IT0000_executable),
						(char *) NULL);
				}
				else
				{
					args->ilinfile = *argv + 2;
					gotinfile = TRUE;
				}
				break;

			  case 'p':
			  case 'P':
				if ( args->ilstartproc || args->ilstartframe )
				{
					IIOerError(ILE_FATAL, ILE_DUPARG,
						ERget(F_IT0001_frame),
						(char *) NULL);
				}
				else
				{
					args->ilstartproc = TRUE;
					if ( *(*argv + 2) != EOS )
					{
						args->ilstartname = *argv + 2;
						gotframe = TRUE;
					}
				}
				break;

			  case 'f':
			  case 'F':
				if ( args->ilstartproc || args->ilstartframe )
				{
					IIOerError(ILE_FATAL, ILE_DUPARG,
						ERget(F_IT0001_frame),
						(char *) NULL);
				}
				else
				{
					args->ilstartframe = TRUE;
					if ( *(*argv + 2) != EOS )
					{
						args->ilstartname = *argv + 2;
						gotframe = TRUE;
					}
				}
				break;

			  case 'I':
				dump = *argv + 2;
				break;

			  case 'k':
			  case 'K':
				/*
				** String used to enable use of parent's
				** keystroke file by this process.
				*/
				tfile = *argv + 2;
				break;

			  case 'l':
			  case 'L':
				IIOdebug = TRUE;
				if ( *(*argv + 2 ) != EOS )
				{
				    LOCATION	loc;
				    char	lbuf[MAX_LOC+1];

				    _VOID_ STlcopy( *argv + 2, lbuf,
							sizeof(lbuf)
				    );
				    if ( LOfroms(FILENAME, lbuf, &loc) != OK
				    	|| (rval = SIopen(&loc, ERx("w"),
							&(args->ildbgfile)))
						!= OK )
				    {
					SIfprintf(stderr,
					    "Can't open debugging file (%s)\n SIopen returns %p",
						*argv + 2,
						rval
					);
				    }
				}
				else
				{
				    args->ildbgfile = stdout;
				}
				break;

			  case 'M':
			  {
			    char	*cp;

			    cp = *argv + 2;
			    if (STbcompare(cp, 0, "normal", 0, TRUE) == 0)
			    {
				args->ilrunmode = ILNRMNORMALRUNMODE;
			    }
			    else if (STbcompare(cp, 0, "test", 0, TRUE) == 0)
			    {
				args->ilrunmode = ILTRMTESTRUNMODE;
			    }
			  }
			  break;

			  case 'R':
				/*
				** Set up trace flags.  No action needed here.
				*/
				break;
			
			  case 'T':
				/*
				** Name of trace file.
				*/
				IIOtoTrcOpen(*argv + 2);
				break;

    			  case 'u':
    			  case 'U':
    				args->iluname = *argv;
    				break;
			
			  case 'X':
				args->ilxflag = *argv;
				break;
		
			  case 'Z':
				test = *argv + 2;
				break;

			  default:
				IIOerError(ILE_FATAL, ILE_FLAGBAD, *argv,
						(char *) NULL
				);
				break;
    			}
			break;
    		  
    		  default:
			if (!gotdb)
			{
				args->ildatabase = *argv;
				gotdb = TRUE;
			}
			else if (!gotframe)
			{
				args->ilstartname = *argv;
				gotframe = TRUE;
			}
			else if (!gotinfile)
			{
				args->ilinfile = *argv;
				gotinfile = TRUE;
			}
			else
			{
				IIOerError(ILE_FATAL, ILE_COMLINE,
						(char *) NULL);
			}
			break;
    		}
	}

	if (!gotdb || !gotinfile || !gotframe)
		IIOerError(ILE_FATAL, ILE_COMLINE, (char *) NULL);

	/*
	** If tfile is NULL, then the interpreter is being run as
	** a separate process.  In this case, the testing files
	** if present can be open in the normal way.  If tfile is
	** not NULL, then the interpreter is being called as a subprocess
	** from the ABF GO command during a test mode run.  In this case,
	** the tests assume (because of how the GO command works in the
	** normal case without an interpreter) that GO runs in the same
	** process an ABF.  Thus, the keystrokes for the application being
	** run are in the same file as the keystrokes for ABF.  The tfile
	** is a special way for the two processes to use the same keystroke
	** files.  In this case, the abdbgnterp call doesn't open
	** the files since FEtfile will do the right thing to set
	** up the test files.
	*/
	if (tfile == NULL)
	{
		if (test != NULL)
			FEtest(test);
		IIARidbgInit(IIOapplname, dump, debug, test);
	}
	else
	{
		abdbgnterp(IIOapplname, dump, debug, test);
		FEtfile(tfile);
	}
}
