/*
** Copyright (c) 1995, 2008 Ingres Corporation
*/
#include <compat.h>
#include <si.h>
#include <st.h>
#include <lo.h>
#include <pc.h>
#include <nm.h>
#include <cv.h>
#include <ex.h>
#include <tr.h>
#include <ssdef.h>
#include <starlet.h>
#include <astjacket.h>

FUNC_EXTERN VOID EXcore();
/**
**  Name: EXdump - Dumping routines
**
**  Description:
**      This module contains EXdump a routine for causing a dump of the current
**      process and EXdumpInit a routine for initialising it
**
**  VMS History:
**	01-Sep-1995 (mckba01 CA)
**		Initial VMS Port.
**      18-May-1998 (horda03)
**         Ported to OI 1.2/01.
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes and
**	   external function references
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**      23-Jul-2002 (horda03)
**         Only indicate that EXdump not initialised ONCE.
**         Thus prevent filling trace files with redundant messages.
**	02-dec-2008 (joea)
**	   Add GLOBALDEF for Ex_diag_link.
**      22-dec-2008 (stegr01)
**         Itanium VMS port.
*/

/*
** Errors to be ignored
**
** This structure contains a series of (from,to) pairs which define which
** ranges of errors should be ignored.
**
** The valid field is set to EXDUMP_INIT if the structure is valid
**
** The table is fixed size to try and avoid problems from store corruptions
** If we fall off the end of the table when checking an error number we assume
** it is not ignored and the problem management server will do the real check.
** - The main errors we are trying to filter are the user SQL errors which
**   start from 0 and are therefore pretty much guaranteed to fit
*/

#define MAP_SIZE 1000

GLOBALDEF     VOID    (*Ex_diag_link)();     /*NULL or CSdiagDumpSched */

static struct IIERRMAP
{
    	i4 valid;
#define EXDUMP_INIT CV_C_CONST_MACRO('E','X','O','K')
    	i4 map_size;
    	struct IIMAP
    	{
       		u_i4 from;
       		u_i4 to;
    	} map[MAP_SIZE];
} errmap={0};


static i4	to_hex();

/*{
**  Name: EXdump - Collect evidence for current process
**
**  Description:
**      This routine causes any required evidence (as defined by the problem 
**      management database) to be collected.
**
**      If EXdumpInit has been called then a list of all errors for which no
**      evidence is required is available and this is scanned to filter out
**      any errors which require no evidence (saving a call to the problem 
**      management server)
**
**      If EXdumpInit has not been called then no dump is taken
**
**  Inputs:
**      u_i4 error        The reason we are dumping (an INGRES error
**                                 number or exception)
**
**  Side effects:
**     All signals are blocked while the dump is being taken
**     The entire server is suspended while the process is being dumped
**
**  History:
**     02-Sep-1993 (paulb ICL)   
**        Created.
**     31-may-1995 (prida01)
**	  add PCfork PCexit and make empty functions for compilation compatibility
*/


VOID
EXdump(error)
u_i4 error;
{
    	char 		buffer[10];
        static i4       log_not_init = 1;
    	i4 		pid;
    	i4 		c,i;
    	i4 		status;
	i4		ast_value;


    	/* Is the dump system initialised ? */

    	if(errmap.valid != EXDUMP_INIT)
    	{
		if (log_not_init)
		{
		   TRdisplay("EXdump diagnostics not Initialised\n");
		   log_not_init = 0;
		}
        	return;
    	}

    	/* Do we want to ignore this error ? */

    	for(c=0;c<errmap.map_size && error>=errmap.map[c].from;++c)
    	{
        	if(error>=errmap.map[c].from && error<=errmap.map[c].to)
		{
#ifdef xDEBUG
			TRdisplay("Ignore error %d\n",error);
#endif
            		return;
		}
    	}


    	/* Block all signals */

	ast_value = sys$setast(0);

    	EXcore(error);
 
   	/* Release signals and continue */
 
 	if (ast_value == SS$_WASSET)
     		sys$setast(1);
}




/*{
**  Name: EXdumpInit - Initialise evidence collection
**
**  Description:
**      This routine builds a list of all errors for which no evidence is
**      required from the problem management database.
**
**      This list is used by the EXdump routine to filter out any errors
**      for which no evidence is required (saving a call to the problem
**      management server)
**
**      The path of the problem management server is also constructed to
**      save having to do it later (when we are less sure of our environment)
**
**  History:
**     13-Sep-1993 (paulb ICL)
**        Created.
**  VMS History:
**	01-Sep-1995 (mckba01 CA)
**		Initial VMS Port.
*/

VOID
EXdumpInit()
{
    	FILE 		*iiexcept;
    	char 		buffer[200];
    	LOCATION 	my_loc;
    	PTR 		path;

    	errmap.valid=0;

    	/* Open problem database file (if it exists) */

    	if(NMloc(FILES,FILENAME,"iiexcept.opt",&my_loc)==OK &&
       	SIfopen(&my_loc,"r",SI_TXT,sizeof(buffer),&iiexcept)==OK)
    	{
        	errmap.map_size=0;

        	while(SIgetrec(buffer,sizeof(buffer),iiexcept)==OK &&
              	errmap.map_size < MAP_SIZE)
        	{
            		/* Only interested in IGNORE actions */

            		if(buffer[16]=='I')
            		{
                		u_i4 from=0;
                		u_i4 to=0;
                		i4 c;

                		/* Decode from and to values */

                		for(c=0;c<8;++c)
                    			from=(from*16)+to_hex(&buffer[c]);
                		for(c=8;c<16;++c)
                    			to=(to*16)+to_hex(&buffer[c]);

                		/* Remember for future reference */

                		errmap.map[errmap.map_size].from = from;
                		errmap.map[errmap.map_size].to = to;
                		errmap.map_size++;
				TRdisplay("ignore %d - %d\n",from,to);
            		}
        	}

        	errmap.valid = EXDUMP_INIT;

        	SIclose(iiexcept);
    	}
	else
		TRdisplay("Cannot open iiexcept.opt\n");
}


/*{
**  Name: to_hex - Convert an character digit to a number 0-15
**
**  Description:
**      This routine is used to convert a character representation of
**      a hex digit into a number in the range 0-15
**
**  Inputs:
**      char *pointer       Pointer to the digit to convert
**
**  Returns:
**      -1       Invalid hex digit
**     0-15      Value of hex digit
**
**  History:
**     08-Sep-1993 (paulb ICL)
**        Created.
*/

static
i4
to_hex(pointer)
PTR pointer;
{
    i4 ret_val;

    switch(*pointer)
    {
       case '0':
           ret_val=0;
           break;
       case '1':
           ret_val=1;
           break;
       case '2':
           ret_val=2;
           break;
       case '3':
           ret_val=3;
           break;
       case '4':
           ret_val=4;
           break;
       case '5':
           ret_val=5;
           break;
       case '6':
           ret_val=6;
           break;
       case '7':
           ret_val=7;
           break;
       case '8':
           ret_val=8;
           break;
       case '9':
           ret_val=9;
           break;
       case 'a':
       case 'A':
           ret_val=10;
           break;
       case 'b':
       case 'B':
           ret_val=11;
           break;
       case 'c':
       case 'C':
           ret_val=12;
           break;
       case 'd':
       case 'D':
           ret_val=13;
           break;
       case 'e':
       case 'E':
           ret_val=14;
           break;
       case 'f':
       case 'F':
           ret_val=15;
           break;
       default:
          ret_val= -1;
    }

    return(ret_val);
}
